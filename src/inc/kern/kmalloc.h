#ifndef MINIOS_KERN_KMALLOC_H
#define MINIOS_KERN_KMALLOC_H

#include <type.h>

void		phy_free_4k(phyaddr_t v);
phyaddr_t	phy_malloc_4k(void);

void		kfree(void *v);
void *		kmalloc(size_t n);

#endif /* MINIOS_KERN_KMALLOC_H */