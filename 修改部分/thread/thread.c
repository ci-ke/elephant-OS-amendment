/* 初始化线程环境 */
void thread_init(void)
{
    put_str("thread_init start\n");

    list_init(&thread_ready_list);
    list_init(&thread_all_list);
    lock_init(&pid_lock);

    /* 先创建第一个用户进程:init */
    process_execute(350, 15, "init"); // 放在第一个初始化,这是第一个进程,init进程的pid为1

    /* 将当前main函数创建为线程 */
    make_main_thread();

    /* 创建idle线程 */
    idle_thread = thread_start("idle", 10, idle, NULL);

    put_str("thread_init done\n");
}