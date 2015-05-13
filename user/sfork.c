
// Lab 4 挑战 6：实现共享内存的 fork

#include <inc/lib.h>

char str[1024] = "Initial string, not changed.";

void
umain(int argc, char **argv)
{
	cprintf("Hello from parent.\n");
	cprintf("parent: The string reads '%s'\n", str);
	if (sfork() == 0)
	{
		cprintf("Hello from child.\n");
		strcpy(str, "This string is changed by child.");
		exit();
	}
	else
	{
		sys_yield();
		sys_yield();
		sys_yield();
		cprintf("parent: The string reads '%s'\n", str);
	}
}

