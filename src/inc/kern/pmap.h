#ifndef MINIOS_KERN_PMAP_H
#define MINIOS_KERN_PMAP_H

#include <type.h>
#include <elf.h>

void	map_kern(phyaddr_t cr3);
void    map_elf_program(phyaddr_t cr3, Proghdr_t *ph);

#endif
