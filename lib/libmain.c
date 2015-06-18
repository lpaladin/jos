// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <inc/lib.h>

extern void umain(int argc, char **argv);

// Lab 4 挑战 6：实现共享内存的 fork
// 对 thisenv 的 hack
const volatile struct Env **_thisenv_addr;
const char *binaryname = "<unknown>";

void default_pgfault_handler(struct UTrapframe *utf)
{
	uint32_t addr = utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	pte_t pte;
	if (uvpd[PDX(addr)] & PTE_P)
		pte = uvpt[addr / PGSIZE];
	else
		panic("pgfault: bad addr [%x]", utf->utf_fault_va);

	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);

	if (pte & PTE_INDISK)
	{
		if ((err = swap_back_page((void *)addr)) < 0)
			panic("default_pgfault_handler: swap_back_page failed (%e)", err);
		return;
	}

	if (!(err & FEC_WR) || !(pte & PTE_COW))
		panic("default_pgfault_handler: unable to handle page fault, eip = %x, access = %x, pte = %x, addr = %x", utf->utf_eip, err, pte, addr);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	r = sys_page_alloc(0, (void *)PFTEMP, PTE_U | PTE_W | PTE_P);
	if (r < 0)
		panic("default_pgfault_handler: sys_page_alloc failed (%e)", r);

	memcpy((void *)PFTEMP, (void *)addr, PGSIZE);

	r = sys_page_map(0, (void *)PFTEMP, 0, (void *)addr, (pte & PTE_SYSCALL & ~PTE_COW) | PTE_W);
	if (r < 0)
		panic("default_pgfault_handler: sys_page_map failed (%e)", r);

	r = sys_page_unmap(0, (void *)PFTEMP);
	if (r < 0)
		panic("default_pgfault_handler: sys_page_unmap failed (%e)", r);

	// panic("pgfault not implemented");
}

void
libmain(int argc, char **argv)
{
	const volatile struct Env *_thisenv;
	_thisenv_addr = &_thisenv;

	// set thisenv to point at our Env structure in envs[].
	// LAB 3: Your code here.
	thisenv = envs + ENVX(sys_getenvid());

	// 设置全局缺页异常处理
	set_pgfault_handler(default_pgfault_handler);

	// Lab 4 挑战 7：批量系统调用
	// 记录缓存页号，用于让批量系统调用避开缓存页
	batch_begin_pgnum = ROUNDDOWN((uint32_t)&in_batch_state, PGSIZE) / PGSIZE,
		batch_end_pgnum = ROUNDUP((uint32_t)(batch_argu + 4), PGSIZE) / PGSIZE;

	// save the name of the program so that panic() can use it
	if (argc > 0)
		binaryname = argv[0];

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}

