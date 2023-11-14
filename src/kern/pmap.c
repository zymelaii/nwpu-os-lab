#include <assert.h>
#include <mmu.h>

#include <kern/kmalloc.h>
#include <kern/pmap.h>

/*
 * 初始化进程页表的内核部分
 * 将3GB ~ 3GB + 128MB的线性地址映射到0 ~ 128MB的物理地址
 */
void
map_kern(phyaddr_t cr3)
{
	// 初始化pde（页目录表）
	// 由于cr3是物理地址，需要进行一步转化转化到线性地址才能访存
	uintptr_t *pde_ptr = (uintptr_t *)K_PHY2LIN(cr3);
	// 基地址在3GB处
	pde_ptr += PDX(3 * GB);
	// 一个页目录项映射4MB的内存，计算需要初始化的页目录项数目
	int pde_num = (128 * MB) / (4 * MB);
	// 被映射的物理地址
	phyaddr_t phy_addr = 0;
	while (pde_num--) {
		// 对于每一个页目录项申请一页用于页表
		phyaddr_t pte_phy = phy_malloc_4k();
		// 保证申请出来的物理地址是4k对齐的
		assert(PGOFF(pte_phy) == 0);
		// 初始化页目录项，权限是存在(P)|写(W)|用户能够访问(U)
		*pde_ptr++ = pte_phy | PTE_P | PTE_W | PTE_U;

		// 接下来对新申请的页表初始化页表项
		// 由于申请的pte_phy是物理地址，需要进行一步转化转化到线性地址才能访存
		uintptr_t *pte_ptr = (uintptr_t *)K_PHY2LIN(pte_phy);
		// 初始化页表的所有页表项
		int pte_num = NPTENTRIES;
		while (pte_num--) {
			// 初始化页表项，权限是存在(P)|写(W)，用户不能访问
			// 直接线性映射物理页
			*pte_ptr++ = phy_addr | PTE_P | PTE_W;
			// 换下一个物理页
			phy_addr += PGSIZE;
		}
	}
	// 经常assert是个好习惯
	assert(phy_addr == 128 * MB);
}