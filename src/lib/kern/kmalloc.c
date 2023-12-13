#include <assert.h>
#include <mmu.h>
#include <x86.h>

#include <kern/stdio.h>
#include <kern/kmalloc.h>
#include <kern/sche.h>
#include <kern/trap.h>

#define PHYMEM_BEGIN (96 * MB)
#define PHYMEM_END (128 * MB)

static u32 phy_malloc_4k_lock;
static phyaddr_t phy_malloc_4k_p = PHYMEM_BEGIN;

#define PHYMEM_BITMAP_SIZE (((PHYMEM_END - PHYMEM_BEGIN) / PGSIZE + 31) / 32)
static u32 phy_mem_bitmap[PHYMEM_BITMAP_SIZE];

/*
 * 释放物理页面，这里并没有为你实现好free_4k的代码，
 * 你需要自己实现一个数据结构用于维护释放的页面，
 * 需要有回收资源的意识，这是一个系统必须有的功能。
 */
void
phy_free_4k(phyaddr_t paddr)
{
	assert(paddr % PGSIZE == 0);
	assert(paddr >= PHYMEM_BEGIN && paddr < PHYMEM_END);
	u32 index = (paddr - PHYMEM_BEGIN) / PGSIZE;
	u32 i = index / 32;
	u32 j = index % 32;
	assert(i < PHYMEM_BITMAP_SIZE);
	while(xchg(&phy_malloc_4k_lock, 1) == 1) { schedule(); }
	phy_mem_bitmap[i] &= ~(1 << j);
	xchg(&phy_malloc_4k_lock, 0);
}
/*
 * 分配物理页面，每次分配4kb，一页
 * 分配的物理内存区间为96MB~128MB
 */
phyaddr_t
phy_malloc_4k(void)
{
	while(xchg(&phy_malloc_4k_lock, 1) == 1)
		schedule();

	phyaddr_t paddr = 0;
	for (int i = 0; i < PHYMEM_BITMAP_SIZE; ++i) {
		if (phy_mem_bitmap[i] == ~(u32)0) { continue; }
		for (int j = 0; j < 32; ++j) {
			if (phy_mem_bitmap[i] & (1 << j)) { continue; }
			phy_mem_bitmap[i] |= 1 << j;
			paddr = (i * 32 + j) * PGSIZE + PHYMEM_BEGIN;
			break;
		}
		if (paddr != 0) { break; }
	}

	if (!(paddr >= PHYMEM_BEGIN && paddr < PHYMEM_END)) {
		kprintf("%x %x %x\n", paddr, PHYMEM_BEGIN, PHYMEM_END);
	}
	assert(paddr >= PHYMEM_BEGIN && paddr < PHYMEM_END);

	xchg(&phy_malloc_4k_lock, 0);
	return paddr;
}

struct header {
	struct header *ptr;
	size_t size;
};
typedef struct header Header;

static Header *freePtr;
static u32 malloc_lock;

/*
 * 释放kmalloc申请的内存
 */
void
kfree(void *v)
{
	Header *bp, *p;
	bp = (Header*)v - 1;

	while(xchg(&malloc_lock, 1) == 1)
		schedule();

	for (p = freePtr; !(bp > p && bp < p->ptr); p = p->ptr) {
		if (p >= p->ptr && (bp > p || bp < p->ptr)) {
			break;
		}
	}
	if (bp + bp->size == p->ptr) {
		bp->size += p->ptr->size;
		bp->ptr = p->ptr->ptr;
	} else {
		bp->ptr = p->ptr;
	}
	if (p + p->size == bp) {
		p->size += bp->size;
		p->ptr = bp->ptr;
	} else {
		p->ptr = bp;
	}
	freePtr = p;
free:
	xchg(&malloc_lock, 0);
}

/*
 * 分配内存，大小为n字节。
 * 分配物理内存区间为64MB~96MB
 */
void *
kmalloc(size_t n)
{
	Header *p, *prevP;
	size_t nUnits;
	void *ret;

	while(xchg(&malloc_lock, 1) == 1)
		schedule();

	nUnits = (n + sizeof(Header) - 1) / sizeof(Header) + 1;
	if ((prevP = freePtr) == 0) {
		freePtr = prevP = (void *)K_PHY2LIN(64 * MB);
		freePtr->ptr = freePtr;
		freePtr->size = (32 * MB) / sizeof(Header);
	}
	for (p = prevP->ptr ; ; prevP = p, p = p->ptr) {
		if (p->size >= nUnits) {
			if (p->size == nUnits) {
				prevP->ptr = p->ptr;
			} else {
				p->size -= nUnits;
				p += p->size;
				p->size = nUnits;
			}
			freePtr = prevP;
			ret = (void*)(p + 1);
			goto free;
		}
		if (p == freePtr) {
			panic("malloc failed!");
		}
	}
free:
	xchg(&malloc_lock, 0);
	return ret;
}
