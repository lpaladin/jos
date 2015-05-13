// Lab 4 挑战 4：实现进程的时空穿越
// 测试用户程序

#include <inc/lib.h>

char strs[5][1024];

void
umain(int argc, char **argv)
{
	int i, j;
	char *str;

	for (i = 0;; i++)
	{
		if (i == 2)
		{
			cprintf("Saving state...\n");
			sys_capture_state(0);
		}
		else if (i == 5)
		{
			cprintf("Restoring state...\n");
			sys_restore_state(0);
		}
		cprintf("Current iteration: %d\n", i);
		str = readline("Please type in something:\n");
		strcpy(strs[i], str);
		cprintf("History (iteration 1~5):\n");
		for (j = 0; j < 5; j++)
			cprintf("%s\n", strs[j]);
	}
}

