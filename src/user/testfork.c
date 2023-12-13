#include <user/stdio.h>
#include <user/syscall.h>

int main()
{
	int pid = fork();
	if (pid > 0) {
		while (1) {
			printf("I'm fa, son pid = %d", pid);
			fflush();
			for (int i = 0 ; i < (int)1e8 ; i++)
				;//do nothing
		}
	} else if (pid == 0) {
		while (1) {
			printf("I'm son");
			fflush();
			for (int i = 0 ; i < (int)1e8 ; i++)
				;//do nothing
		}
	} else {
		printf("fork failure, errno=%d", pid);
        fflush();
	}
}
