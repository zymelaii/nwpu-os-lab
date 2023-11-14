#include <assert.h>
#include <mmu.h>
#include <x86.h>

#include <kern/kmalloc.h>
#include <kern/pmap.h>
#include <kern/stdio.h>

/*
 * 页表小demo测试，用于让大家熟悉页表的相关理论
 */
void
pmap_test(void)
{
	phyaddr_t old_cr3 = rcr3();
	// 为了做测试，申请了一个新的物理页用于页表的根目录
	phyaddr_t new_cr3 = phy_malloc_4k();
	kprintf("cr3 physic addr: 0x%x\n", new_cr3);
	// 可以打印一下函数地址，看看它们线性地址实际的位置，你会发现与实验四的虚拟地址分布不太一样
	kprintf("map_kern: 0x%x, lcr3: 0x%x\n", map_kern, lcr3);
	// 这里映射内核区域，这样在加载完之后依然能够正常运行代码，否则加载完之后执行指令直接爆pf(page fault)
	// 如果不怕死可以直接注释这一行，你能够欣赏到精彩的pf
	map_kern(new_cr3);
	lcr3(new_cr3);
	
	// 在没有页表映射之前，任何对地址的读写操作都会爆pf，所以我们需要自己映射来实现地址的访存
	// 我们就假定往0x11451400地址处写入相关值，接下来要对该地址处做相关地址映射
	uintptr_t lin1_addr = 0x11451400;
	
	// 首先填充pde页表项，之前申请的new_cr3就是pde对应的页表，我们需要填充pde对应的页表项
	// K_PHY2LIN能够帮助我们将物理地址转化为能够访问得到该物理地址的线性地址
	phyaddr_t pde_phy = new_cr3;
	uintptr_t *pde_ptr = (uintptr_t *)K_PHY2LIN(pde_phy);
	phyaddr_t pte_phy = phy_malloc_4k();
	pde_ptr[PDX(lin1_addr)] = pte_phy | PTE_P | PTE_W | PTE_U;
	// 我们可以打印一下PDX(lin1_addr)和对应的pde页表项，这个物理页就是pte对应的页表，被描述在pde对应的页表项中
	kprintf("PDX(lin1_addr) = 0x%x, pde = 0x%x\n", 
			PDX(lin1_addr), pde_ptr[PDX(lin1_addr)]);

	// 接下来要填充pte页表项，操作与刚才差不多，基本上同样的操作再来一遍
	uintptr_t *pte_ptr = (uintptr_t *)K_PHY2LIN(pte_phy);
	phyaddr_t pg_phy = phy_malloc_4k();
	pte_ptr[PTX(lin1_addr)] = pg_phy | PTE_P | PTE_W | PTE_U;
	kprintf("PTX(lin1_addr) = 0x%x, pte = 0x%x\n\n", 
			PTX(lin1_addr), pte_ptr[PTX(lin1_addr)]);
	
	// 你可能会怀疑，刚才打印的PDX(lin1_addr)和PTX(lin1_addr)是个什么**玩意
	// 不过看完这一行的输出，可能你会有所感悟
	kprintf("PDX(lin1_addr) << 22 | PTX(lin1_addr) << 12 = 0x%x\n",
		PDX(lin1_addr) << PDXSHIFT | PTX(lin1_addr) << PTXSHIFT);
	// 接下来你就能愉悦对0x11451400进行读写了
	*(int *)lin1_addr = 1919810;
	// 不过我们重新打印一下pde和pte，？，怎么有点不太一样？是怎么回事？
	// 看起来硬件都访问到了lin1_addr对应的pde和pte，通过这两项找到真正的物理地址，然后对该地址做了修改
	// 但是这个改了值的含义是什么？这个的原因需要你自己RTFM，这里并不做相关解答。
	kprintf("pde = 0x%x, pte = 0x%x\n\n",
		pde_ptr[PDX(lin1_addr)], pte_ptr[PTX(lin1_addr)]);
	
	// 在读写之后我们，当然要查看是不是写进去了
	// 但是除了原来的0x11451400，我们可以用另一个线性地址访问到了同样的数据
	uintptr_t lin2_addr = K_PHY2LIN(pg_phy + PGOFF(lin1_addr));
	kprintf("lin1_addr:0x%x -> %d\n"
		"lin2_addr:0x%x -> %d\n\n", 
		lin1_addr, *(int *)lin1_addr,
		lin2_addr, *(int *)lin2_addr);
	
	// 还是有点摸不清硬件是怎么访问页表的？下面这行打印可能能够帮到你
	kprintf("*(phyaddr_t)(0x%x + 0x%x) => *(uintptr_t)(0x%x + 0x%x) = 0x%x\n"
		"*(phyaddr_t)(0x%x + 0x%x) => *(uintptr_t)(0x%x + 0x%x) = 0x%x\n\n",
		pde_phy, PDX(lin1_addr), pde_ptr, PDX(lin1_addr), pde_ptr[PDX(lin1_addr)],
		pte_phy, PTX(lin1_addr), pte_ptr, PTX(lin1_addr), pte_ptr[PTX(lin1_addr)]);
	
	// 当然，我们可以申请一个新的物理页替换掉原来的pte对应的页表
	phyaddr_t new_pg_phy = phy_malloc_4k();
	pte_ptr[PTX(lin1_addr)] = new_pg_phy | PTE_P | PTE_W | PTE_U;
	// 同样往相同的地址写入相关数据
	*(int *)lin1_addr = 114514;
	uintptr_t lin3_addr = K_PHY2LIN(new_pg_phy + PGOFF(lin1_addr));
	// 我们打印一下，结果肯定是lin1_addr和lin3_addr对应的值相同……吗？
	// 怎么回事？怎么lin2_addr对应的跟lin1_addr对应的值相同，lin3_addr对应的值一点都没改
	kprintf("lin1_addr:0x%x -> %d\n"
		"lin2_addr:0x%x -> %d\n"
		"lin3_addr:0x%x -> %d\n\n", 
		lin1_addr, *(int *)lin1_addr,
		lin2_addr, *(int *)lin2_addr,
		lin3_addr, *(int *)lin3_addr);
	
	// 是不是qemu出问题了？是不是该怀疑qemu，还是自己之前的操作失误了？
	// 但是查了一遍好像没啥问题，照理往lin1_addr写入数据lin3_addr能够马上看到
	// 这个时候出问题的实际上是tlb，在你修改已经在tlb中的页表项时，tlb中的缓存并不会直接刷新
	// 所以需要刷新一下tlb缓存，让硬件重新解析页表
	tlbflush();
	// 在刷新之后你会发现修改正常了
	*(int *)lin1_addr = 1919810;
	kprintf("lin1_addr:0x%x -> %d\n"
		"lin2_addr:0x%x -> %d\n"
		"lin3_addr:0x%x -> %d\n\n", 
		lin1_addr, *(int *)lin1_addr,
		lin2_addr, *(int *)lin2_addr,
		lin3_addr, *(int *)lin3_addr);
	lcr3(old_cr3);
	while(1);
}
