#include "io.h"

/* 将buf指向的内存中,偏移为offset,大小为filesz的段加载到虚拟地址为vaddr的内存 */
static bool segment_load_buf(uint8_t *buf, uint32_t offset, uint32_t filesz, uint32_t vaddr)
{
   uint32_t vaddr_first_page = vaddr & 0xfffff000;               // vaddr地址所在的页框
   uint32_t size_in_first_page = PG_SIZE - (vaddr & 0x00000fff); // 加载到内存后,文件在第一个页框中占用的字节大小
   uint32_t occupy_pages = 0;
   /* 若一个页框容不下该段 */
   if (filesz > size_in_first_page)
   {
      uint32_t left_size = filesz - size_in_first_page;
      occupy_pages = DIV_ROUND_UP(left_size, PG_SIZE) + 1; // 1是指vaddr_first_page
   }
   else
   {
      occupy_pages = 1;
   }

   /* 为进程分配内存 */
   uint32_t page_idx = 0;
   uint32_t vaddr_page = vaddr_first_page;
   while (page_idx < occupy_pages)
   {
      uint32_t *pde = pde_ptr(vaddr_page);
      uint32_t *pte = pte_ptr(vaddr_page);

      /* 如果pde不存在,或者pte不存在就分配内存.
       * pde的判断要在pte之前,否则pde若不存在会导致
       * 判断pte时缺页异常 */
      if (!(*pde & 0x00000001) || !(*pte & 0x00000001))
      {
         if (get_a_page_for_init(vaddr_page) == NULL)
         {
            return false;
         }
      } // 如果原进程的页表已经分配了,利用现有的物理页,直接覆盖进程体
      vaddr_page += PG_SIZE;
      page_idx++;
   }
   memcpy((void *)vaddr, buf + offset, filesz);
   return true;
}

/* 从裸盘上lba扇区加载sec_cnt扇区大小的用户程序,成功则返回程序的起始地址,否则返回-1 */
int32_t load_lba(uint32_t lba, uint32_t sec_cnt)
{
   int32_t ret = -1;
   struct Elf32_Ehdr elf_header;
   struct Elf32_Phdr prog_header;
   memset(&elf_header, 0, sizeof(struct Elf32_Ehdr));

   uint8_t *buf = sys_malloc(sec_cnt * 512);
   asm volatile("call rd_disk_m_32"
                :
                : "a"(lba), "b"(buf), "c"(sec_cnt)
                : "memory", "edx", "esi", "edi");
   inb(0x1f7); //处理硬盘中断

   memcpy(&elf_header, buf, sizeof(struct Elf32_Ehdr));

   /* 校验elf头 */
   if (memcmp(elf_header.e_ident, "\177ELF\1\1\1", 7) || elf_header.e_type != 2 || elf_header.e_machine != 3 || elf_header.e_version != 1 || elf_header.e_phnum > 1024 || elf_header.e_phentsize != sizeof(struct Elf32_Phdr))
   {
      ret = -1;
      goto done;
   }

   Elf32_Off prog_header_offset = elf_header.e_phoff;
   Elf32_Half prog_header_size = elf_header.e_phentsize;

   /* 遍历所有程序头 */
   uint32_t prog_idx = 0;
   uint8_t *bufp;
   while (prog_idx < elf_header.e_phnum)
   {
      memset(&prog_header, 0, prog_header_size);

      /* 将文件的指针定位到程序头 */
      bufp = buf + prog_header_offset;

      /* 只获取程序头 */
      memcpy(&prog_header, bufp, prog_header_size);

      /* 如果是可加载段就调用segment_load加载到内存 */
      if (PT_LOAD == prog_header.p_type)
      {
         if (!segment_load_buf(buf, prog_header.p_offset, prog_header.p_filesz, prog_header.p_vaddr))
         {
            ret = -1;
            goto done;
         }
      }

      /* 更新下一个程序头的偏移 */
      prog_header_offset += elf_header.e_phentsize;
      prog_idx++;
   }
   ret = elf_header.e_entry;
done:
   sys_free(buf);
   return ret;
}

/* 用path指向的程序替换当前进程 */
int32_t sys_execv(const char *path, const char *argv[])
{
   char *argv_stack[16] = {0};
   char arg_string_stack[1024] = {0};
   uint32_t argc = 0;
   while (argv[argc])
   {
      argc++;
   }
   char *cur_string = arg_string_stack;
   uint32_t i;
   for (i = 0; i < argc; i++)
   {
      memcpy(cur_string, argv[i], strlen(argv[i]));
      argv_stack[i] = cur_string;
      cur_string += strlen(argv[i]) + 1;
   }
   int32_t entry_point = load(path);
   if (entry_point == -1)
   { // 若加载失败则返回-1
      return -1;
   }
   char **argv_heap = sys_malloc(16 * sizeof(char *));
   char *arg_string_heap = sys_malloc(1024);
   memcpy(argv_heap, argv_stack, 16 * sizeof(char *));
   memcpy(arg_string_heap, arg_string_stack, 1024);
   int32_t gap = arg_string_heap - arg_string_stack;
   for (i = 0; i < argc; i++)
   {
      argv_heap[i] += gap;
   }

   struct task_struct *cur = running_thread();
   /* 修改进程名 */
   memcpy(cur->name, path, TASK_NAME_LEN);
   cur->name[TASK_NAME_LEN - 1] = 0;

   struct intr_stack *intr_0_stack = (struct intr_stack *)((uint32_t)cur + PG_SIZE - sizeof(struct intr_stack));
   /* 参数传递给用户进程 */
   intr_0_stack->ebx = (int32_t)argv_heap;
   intr_0_stack->ecx = argc;
   intr_0_stack->eip = (void *)entry_point;
   /* 使新用户进程的栈地址为最高用户空间地址 */
   intr_0_stack->esp = (void *)0xc0000000;

   /* exec不同于fork,为使新进程更快被执行,直接从中断返回 */
   asm volatile("movl %0, %%esp; jmp intr_exit"
                :
                : "g"(intr_0_stack)
                : "memory");
   return 0;
}