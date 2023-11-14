#ifndef MINIOS_KERN_FS_H
#define MINIOS_KERN_FS_H

#include <type.h>

ssize_t	kern_read(int fd, void *buf, size_t count);
ssize_t	kern_write(int fd, const void *buf, size_t count);

void	read_file(const char *filename, void *dst);

#endif /* MINIOS_KERN_FS_H */