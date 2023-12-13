#ifndef MINIOS_KERN_FORK_H
#define MINIOS_KERN_FORK_H

#include <type.h>
#include <kern/process.h>

ssize_t	kern_fork(pcb_t *p_father);

#endif /* MINIOS_KERN_FORK_H */