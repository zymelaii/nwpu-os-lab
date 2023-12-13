#include <assert.h>

#include <user/stdio.h>
#include <user/syscall.h>

int main()
{
	ssize_t pid = fork();
	
	if (pid == 0) {
		int ret = exec("shell.bin");
		panic("exec failed! errno: %d", ret);
	}
	
	if (pid < 0)
		panic("init process fork failed errno: %d", pid);

	int wstatus;
	while (1) {
		pid = wait(&wstatus);
	}
}