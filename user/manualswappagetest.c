#include <inc/lib.h>

void pgfault_handler(struct UTrapframe *utf)
{
	int ret;
	cprintf("AoYi: PageFault when eip = 0x%08x!\n", utf->utf_eip);
	if (uvpt[PGNUM(utf->utf_fault_va)] & PTE_INDISK)
	{
		if ((ret = swap_back_page((void *)utf->utf_fault_va)) < 0)
			panic("AoYi: swap_back_page failed (%e)", ret);
	}
	else
		panic("AoYi: What the fuck happened?! va=%x pte=%x", utf->utf_fault_va, uvpt[PGNUM(utf->utf_fault_va)]);
}

char buf[2][PGSIZE];

void
umain(int argc, char **argv)
{
	int i;
	set_pgfault_handler(pgfault_handler);

	for (i = 0; i < PGSIZE; i++)
		buf[0][i] = i % 128;
	for (i = 0; i < PGSIZE; i++)
		buf[1][i] = i % 77 + 23;

	cprintf("AoYi: Begin to manually swap pages[0x%08x].\n", buf);
	assert(!swap_page_to_disk(buf[0]));
	assert(!swap_page_to_disk(buf[1]));
	cprintf("AoYi: Pages swapped.\n");


	for (i = 0; i < PGSIZE; i++)
		if (buf[0][i] != i % 128)
			panic("AoYi: WTF the first page does not equal to the past!");

	for (i = 0; i < PGSIZE; i++)
		if (buf[1][i] != i % 77 + 23)
			panic("AoYi: WTF the second page does not equal to the past!");

	cprintf("AoYi: GOOD!\n");
}

