#include <assert.h>
#include <elf.h>
#include <string.h>
#include <mmu.h>

#include <kern/kmalloc.h>
#include <kern/pmap.h>

/*
 * 申请一个新的物理页，并更新page_list页面信息
 * 返回新申请的物理页面的物理地址
 */
static phyaddr_t
alloc_phy_page(page_node_t **page_list)
{
	phyaddr_t paddr = phy_malloc_4k();
	
	page_node_t *new_node = kmalloc(sizeof(page_node_t));
	new_node->nxt = *page_list;
	new_node->paddr = paddr;
	new_node->laddr = -1;
	*page_list = new_node;

	return paddr;
}

/*
 * MINIOS中比较通用的页表映射函数
 * 它将laddr处的虚拟页面映射到物理地址为paddr（如果paddr为-1则会自动申请一个新的物理页面）的物理页面
 * 并将pte_flag置位到页表项（页目录项标志位默认为PTE_P | PTE_W | PTE_U）
 * 这个函数中所有新申请到的页面信息会存放到page_list这个链表中
 */
static void
lin_mapping_phy(u32			cr3,
		page_node_t	**page_list,
		uintptr_t		laddr,
		phyaddr_t		paddr,
		u32			pte_flag)
{
	assert(PGOFF(laddr) == 0);

	uintptr_t *pde_ptr = (uintptr_t *)K_PHY2LIN(cr3);

	if ((pde_ptr[PDX(laddr)] & PTE_P) == 0) {
		phyaddr_t pte_phy = alloc_phy_page(page_list);
		memset((void *)K_PHY2LIN(pte_phy), 0, PGSIZE);
		pde_ptr[PDX(laddr)] = pte_phy | PTE_P | PTE_W | PTE_U;
	}

	phyaddr_t pte_phy = PTE_ADDR(pde_ptr[PDX(laddr)]);
	uintptr_t *pte_ptr = (uintptr_t *)K_PHY2LIN(pte_phy);

	phyaddr_t page_phy;
	if (paddr == (phyaddr_t)-1) {
		if ((pte_ptr[PTX(laddr)] & PTE_P) != 0)
			return;
		page_phy = alloc_phy_page(page_list);
		(*page_list)->laddr = laddr;
		
	} else {
		if ((pte_ptr[PTX(laddr)] & PTE_P) != 0)
			warn("this page was mapped before, laddr: %x", laddr);
		assert(PGOFF(paddr) == 0);
		page_phy = paddr;
	}
	pte_ptr[PTX(laddr)] = page_phy | pte_flag;
}

/*
 * 初始化进程页表的内核部分
 * 将3GB ~ 3GB + 128MB的线性地址映射到0 ~ 128MB的物理地址
 */
void
map_kern(u32 cr3, page_node_t **page_list)
{
	for (phyaddr_t paddr = 0 ; paddr < 128 * MB ; paddr += PGSIZE) {
		lin_mapping_phy(cr3,
				page_list,
				K_PHY2LIN(paddr),
				paddr,
				PTE_P | PTE_W | PTE_U);
	}
}

/*
 * 根据elf文件信息将数据搬迁到指定位置
 * 中间包含了对页表的映射，eip的置位
 * 这也是你们做实验五中最折磨的代码，可以看这份学习一下
 */
void
map_elf(pcb_t *p_proc, void *elf_addr)
{
	assert(p_proc->lock != 0);

	Elfhdr_t *eh = (Elfhdr_t *)elf_addr;

	Proghdr_t *ph = (Proghdr_t *)(elf_addr + eh->e_phoff);
	for (int i = 0 ; i < eh->e_phnum ; i++, ph++) {
		if (ph->p_type != PT_LOAD)
			continue;
		uintptr_t st = ROUNDDOWN(ph->p_va, PGSIZE);
		uintptr_t en = ROUNDUP(st + ph->p_memsz, PGSIZE);
		for (uintptr_t laddr = st ; laddr < en ; laddr += PGSIZE) {
			u32 pte_flag = PTE_P | PTE_U;
			if ((ph->p_flags & ELF_PROG_FLAG_WRITE) != 0)
				pte_flag |= PTE_W;
			lin_mapping_phy(p_proc->cr3,
					&p_proc->page_list,
					laddr,
					(phyaddr_t)-1,
					pte_flag);
		}
		memcpy(	(void *)ph->p_va,
			(const void *)eh + ph->p_offset,
			ph->p_filesz);
		memset(	(void *)ph->p_va + ph->p_filesz, 
			0, 
			ph->p_memsz - ph->p_filesz);
	}

	p_proc->user_regs.eip = eh->e_entry;
}

/*
 * 将用户栈映射到用户能够访问到的最后一个页面
 * (0xbffff000~0xc0000000)
 * 并将esp寄存器放置好
 */
void
map_stack(pcb_t *p_proc)
{
	assert(p_proc->lock != 0);

	lin_mapping_phy(p_proc->cr3,
			&p_proc->page_list,
			K_PHY2LIN(-PGSIZE),
			(phyaddr_t)-1,
			PTE_P | PTE_W | PTE_U);
	
	p_proc->user_regs.esp = K_PHY2LIN(0);
}

/*
 * 根据page_list回收所有的页面（包括回收页面节点）
 */
void
recycle_pages(page_node_t *page_list)
{
	for (page_node_t *prevp, *p = page_list ; p ;) {
		phy_free_4k(p->paddr);
		prevp = p, p = p->nxt;
		kfree(prevp);
	}
}