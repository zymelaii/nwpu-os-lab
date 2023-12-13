#include <assert.h>
#include <string.h>

#include <user/stdio.h>
#include <user/wait.h>
#include <user/syscall.h>


int main() {
	int childStatus = 0;
	while (1) {
		printf("\x1b[32mMINIOS\x1b[0m$ ");
		fflush();

		int cntInput = 0;
		char input[512] = {0};
		while (1) {
			char c = getchar();
			
			if (c == '\n') {
				printf("%c",c);
				fflush();
				input[cntInput++] = '\0';
				break;
			} else if (c == '\b') { 
				if (cntInput == 0)
					continue;
				printf("\b \b");
				fflush();
				cntInput--;
			} else {
				printf("%c",c);
				fflush();
				input[cntInput++] = c;
			}
		}

		if (strcmp(input, "?") == 0) {
			printf("%d\n",WEXITSTATUS(childStatus));
			continue;
		}

		int ForkRetVal = fork();
		if (ForkRetVal < 0) {
			printf("fork failed!\n");
			continue;
		}
		if (ForkRetVal == 0) {
			exec(input);
			printf("exec failed!\n");
			exit(1);
		} 

		int wait_pid;
		wait_pid = wait(&childStatus);
		if ((wait_pid = wait(&childStatus)) >= 0) {
			// do nothing
		}
	}
	return 0;
}
