extern struct task_struct *init_pcb;

/* 将init进程的地址vaddr与用户池中的物理地址关联,仅支持一页空间分配 */
void *get_a_page_for_init(uint32_t vaddr)
{
    struct pool *mem_pool = &user_pool;
    lock_acquire(&mem_pool->lock);

    /* 先将虚拟地址对应的位图置1 */
    int32_t bit_idx = -1;

    bit_idx = (vaddr - init_pcb->userprog_vaddr.vaddr_start) / PG_SIZE;
    ASSERT(bit_idx >= 0);
    bitmap_set(&init_pcb->userprog_vaddr.vaddr_bitmap, bit_idx, 1);

    void *page_phyaddr = palloc(mem_pool);
    if (page_phyaddr == NULL)
    {
        lock_release(&mem_pool->lock);
        return NULL;
    }
    page_table_add((void *)vaddr, page_phyaddr);
    lock_release(&mem_pool->lock);
    return (void *)vaddr;
}
