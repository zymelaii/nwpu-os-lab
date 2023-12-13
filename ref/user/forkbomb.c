#include <user/stdio.h>
#include <user/syscall.h>

static size_t random_seed;
inline static size_t rand() { return (((random_seed = random_seed * 214013L + 2531011L) >> 16) & 0x7fff);}

void bomb_salve(void)
{
	fork_ack();
	
	random_seed = get_ticks() & 0x7fff;
	
	for (int i = 0 ; i < (get_pid() & 0xff) ; i++)
		rand();
	
	for (int i = (rand() & 0xf) ; i > 0 ; i--) {
		int pid = fork();
		if (pid == 0)
			bomb_salve();
		for (int i = rand() ; i > 0 ; i--)
			; // do nothing
	}
	
	if (rand() % 2 == 1) {
		while (wait(NULL) >= 0);
	}
	exit(0);
}

void bomb_master(void)
{
	random_seed = get_ticks() & 0x7fff;
	while (1) {
		int pid = fork();
		if (pid == 0) 
			bomb_salve();
		for (int i = rand() ; i > 0 ; i--)
			; // do nothing
		while (wait(NULL) >= 0);
	}
}

int main()
{
	bomb_master();
}

