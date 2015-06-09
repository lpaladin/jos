#include <inc/lib.h>

void pgfault_handler(struct UTrapframe *utf)
{
	cprintf("AoYi: PageFault when eip = 0x%08x!\n", utf->utf_eip);
	if (uvpt[PGNUM(utf->utf_fault_va)] & PTE_INDISK)
		assert(!swap_back_page((void *)utf->utf_fault_va));
	else
		panic("AoYi: What the fuck happened?! va=%x pte=%x", utf->utf_fault_va, uvpt[PGNUM(utf->utf_fault_va)]);
}

char buf[PGSIZE];

void
umain(int argc, char **argv)
{
	int i;
	set_pgfault_handler(pgfault_handler);

	for (i = 0; i < PGSIZE; i++)
		buf[i] = i;

	cprintf("AoYi: Begin to manually swap page[0x%08x].\n", buf);
	assert(!swap_page_to_disk(buf));
	cprintf("AoYi: Page swapped.\n", buf);


	for (i = 0; i < PGSIZE; i++)
		if (buf[i] != i)
			panic("AoYi: WTF this page does not equal to the past!");

	cprintf("AoYi: GOOD!\n");
}

