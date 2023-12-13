#ifndef MINIOS_KERN_EXEC_H
#define MINIOS_KERN_EXEC_H

#include <kern/process.h>

void init_pagetbl(pcb_t *p_proc);
ssize_t	kern_exec(pcb_t *p_proc, const char *pathname);

#endif /* MINIOS_KERN_EXEC_H */
