#include <user/stdio.h>

int main()
{
	int cntInput = 0;
        char input[512] = {0};
	while (1) {
            char c = getchar();
	    printf("%c",c);
	    fflush();
            if (c != '\n') {
		input[cntInput++] = c;
	    } else { 
                input[cntInput++] = '\0';
                break;
            }
        }
	printf("%s\n",input);
	return 0;
}