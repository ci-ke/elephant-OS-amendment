#include "exec.h"

struct task_struct *init_pcb;

/* 创建用户进程 */
void process_execute(uint32_t lba, uint32_t sec_cnt, char *name)
{
   /* pcb内核的数据结构,由内核来维护进程信息,因此要在内核内存池中申请 */
   struct task_struct *thread = get_kernel_pages(1);
   init_pcb = thread;
   init_thread(thread, name, default_prio);
   create_user_vaddr_bitmap(thread);

   thread->pgdir = create_page_dir();
   block_desc_init(thread->u_block_desc);

   page_dir_activate(thread);
   void *entry = (void *)load_lba(lba, sec_cnt);

   uint32_t pagedir_phy_addr = 0x100000; // 默认为内核的页目录物理地址,也就是内核线程所用的页目录表
                                         /* 更新页目录寄存器cr3,使新页表生效 */
   asm volatile("movl %0, %%cr3"
                :
                : "r"(pagedir_phy_addr)
                : "memory");
   /*这里不把页目录改回来的话后面初始化硬盘会出错，因为它去0x475地址读数据了，
   那里改为0xc0000475的话这里页目录也可以不改回来*/

   thread_create(thread, start_process, entry);
   enum intr_status old_status = intr_disable();
   ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
   list_append(&thread_ready_list, &thread->general_tag);

   ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
   list_append(&thread_all_list, &thread->all_list_tag);
   intr_set_status(old_status);
}