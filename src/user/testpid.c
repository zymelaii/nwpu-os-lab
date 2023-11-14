#include <user/stdio.h>
#include <user/syscall.h>

int main()
{
	while (1) {
		printf("pid: %d!", get_pid());
		fflush();
		for (int i = 0 ; i < (int)1e8 ; i++)
			;//do nothing
	}
	return 0;
}