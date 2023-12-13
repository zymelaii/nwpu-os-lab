#include <user/stdio.h>

int main()
{
	while(1) {
		u8 ch = getch();
		if (ch == 255)
			continue;
		printf("%c", ch);
		fflush();
	} 
}