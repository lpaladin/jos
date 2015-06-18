#include <inc/lib.h>

char buf[10][PGSIZE] __attribute__((aligned(PGSIZE)));

void
umain(int argc, char **argv)
{
	int i, j;


	for (j = 0; j < 10; j++)
		for (i = 0; i < PGSIZE; i++)
			buf[j][i] = i % (67 + j) + j;

	cprintf("AoYi: Begin to manually swap pages[0x%08x].\n", buf);

	for (j = 0; j < 10; j++)
		assert(!swap_page_to_disk(buf[j]));

	cprintf("AoYi: Pages swapped.\n");


	for (j = 0; j < 10; j++)
		for (i = 0; i < PGSIZE; i++)
			if (buf[j][i] != i % (67 + j) + j)
				panic("AoYi: WTF page%d does not equal to the past!", j);

	cprintf("AoYi: GOOD!\n");
}

