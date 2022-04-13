#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall-init.h"
#include "ide.h"
#include "fs.h"

#include "list.h"

extern struct list thread_ready_list;
extern struct task_struct *init_pcb;

/*负责初始化所有模块 */
void init_all()
{
   put_str("init_all\n");
   idt_init();    // 初始化中断
   mem_init();    // 初始化内存管理系统
   thread_init(); // 初始化线程相关结构
   list_remove(&init_pcb->general_tag);
   timer_init();    // 初始化PIT
   console_init();  // 控制台初始化最好放在开中断之前
   keyboard_init(); // 键盘初始化
   tss_init();      // tss初始化
   syscall_init();  // 初始化系统调用
   intr_enable();   // 后面的ide_init需要打开中断
   ide_init();      // 初始化硬盘
   filesys_init();  // 初始化文件系统
   list_append(&thread_ready_list, &init_pcb->general_tag);
}