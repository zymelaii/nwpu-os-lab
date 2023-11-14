#include <assert.h>
#include <mmu.h>

#include <kern/kmalloc.h>
#include <kern/trap.h>

static phyaddr_t malloc_4k_p = 64 * MB;

/*
 * 分配物理内存，每次分配4kb，一页
 * 分配的物理内存区间为64MB~128MB
 */
phyaddr_t
phy_malloc_4k(void)
{
	assert(malloc_4k_p < 128 * MB);

	phyaddr_t addr = malloc_4k_p;
	malloc_4k_p += PGSIZE;
	
	return addr;
}