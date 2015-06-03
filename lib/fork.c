// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
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

	if (!(err & FEC_WR) || !(pte & PTE_COW))
		panic("pgfault: unable to handle page fault, eip = %x, access = %x, pte = %x, addr = %x", utf->utf_eip, err, pte, addr);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.

	r = sys_page_alloc(0, (void *)PFTEMP, PTE_U | PTE_W | PTE_P);
	if (r < 0)
		panic("pgfault: sys_page_alloc failed (%e)", r);

	memcpy((void *)PFTEMP, (void *) addr, PGSIZE);

	r = sys_page_map(0, (void *)PFTEMP, 0, (void *) addr, (pte & PTE_SYSCALL & ~PTE_COW) | PTE_W);
	if (r < 0)
		panic("pgfault: sys_page_map failed (%e)", r);

	r = sys_page_unmap(0, (void *)PFTEMP);
	if (r < 0)
		panic("pgfault: sys_page_unmap failed (%e)", r);

	// panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r, perm;
	void *addr = (void *)(pn * PGSIZE);

	// LAB 4: Your code here.
	perm = uvpt[pn] & PTE_SYSCALL;
	if ((perm & PTE_COW || perm & PTE_W) && !(perm & PTE_SHARE))
	{
		r = sys_page_map(0, addr, envid, addr, PTE_COW | PTE_U | PTE_P);
		if (r < 0)
			return r;

		r = sys_page_map(0, addr, 0, addr, PTE_COW | PTE_U | PTE_P);
		if (r < 0)
			return r;
	}
	else
	{
		r = sys_page_map(0, addr, envid, addr, perm);
		if (r < 0)
			return r;
	}
	// panic("duppage not implemented");
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t child;
	int error;
	uint32_t pdeid, pteid, temp;
	extern void _pgfault_upcall(void);

	set_pgfault_handler(pgfault);
	child = sys_exofork();
	if (child < 0)
		return child;
	else if (child == 0)
	{
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}

	// Lab 4 挑战 7：批量系统调用
	// 开始记录系统调用到缓存
	begin_batchcall();

	// COW方式映射所有非异常栈区域
	for (pdeid = 0; ; pdeid++)
		if (uvpd[pdeid] & PTE_P)
		{
			temp = pdeid * NPTENTRIES;
			for (pteid = 0; pteid < NPTENTRIES; pteid++)
			{
				if (temp + pteid >= UXSTACKTOP / PGSIZE - 1)
					goto copyend;
				if (temp + pteid == batch_begin_pgnum)
				{
					// 提交所有系统调用
					error = end_batchcall();
					if (error < 0)
						panic("fork: end_batchcall failed (%e)", error);
				}
				if (temp + pteid == batch_end_pgnum + 1)
				{
					// 开始记录系统调用到缓存
					begin_batchcall();
				}
				if (uvpt[temp + pteid] & PTE_P)
				{
					error = duppage(child, temp + pteid);
					if (error < 0)
						panic("fork: duppage failed (%e)", error);
				}
			}
		}

copyend:

	// 提交所有系统调用
	error = end_batchcall();
	if (error < 0)
		panic("fork: end_batchcall failed (%e)", error);

	// 复制异常栈
	error = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P);
	if (error < 0)
		panic("fork: sys_page_alloc failed (%e)", error);

	error = sys_page_map(child, (void *)(UXSTACKTOP - PGSIZE), 0, UTEMP, PTE_U | PTE_W | PTE_P);
	if (error < 0)
		panic("fork: sys_page_map failed (%e)", error);

	memcpy(UTEMP, (void *)(UXSTACKTOP - PGSIZE), PGSIZE);

	error = sys_page_unmap(0, UTEMP);
	if (error < 0)
		panic("fork: sys_page_unmap failed (%e)", error);

	// 设置页面错误回调
	error = sys_env_set_pgfault_upcall(child, _pgfault_upcall);
	if (error < 0)
		panic("fork: sys_env_set_pgfault_upcall failed (%e)", error);

	error = sys_env_set_status(child, ENV_RUNNABLE);
	if (error < 0)
		panic("fork: sys_env_set_status failed (%e)", error);

	return child;
	// panic("fork not implemented");
}

// Lab 4 挑战 6：实现共享内存的 fork
int
sfork(void)
{
	envid_t child;
	int error;
	uint32_t pdeid, pteid, temp;
	extern void _pgfault_upcall(void);

	set_pgfault_handler(pgfault);
	child = sys_exofork();
	if (child < 0)
		return child;
	else if (child == 0)
	{
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}

	// 直接映射方式映射所有非栈区域，COW方式映射栈
	for (pdeid = 0;; pdeid++)
		if (uvpd[pdeid] & PTE_P)
		{
			temp = pdeid * NPTENTRIES;
			for (pteid = 0; pteid < NPTENTRIES; pteid++)
			{
				if (temp + pteid >= UXSTACKTOP / PGSIZE - 1)
					goto copyend;
				if (uvpt[temp + pteid] & PTE_P)
				{
					if (temp + pteid >= USTACKTOP / PGSIZE - 1)
					{
						error = duppage(child, temp + pteid);
						if (error < 0)
							panic("sfork: duppage failed (%e)", error);
					}
					else
					{
						void *addr = (void *)((temp + pteid) * PGSIZE);
						error = sys_page_map(0, addr, child, addr, uvpt[temp + pteid] & PTE_SYSCALL);
						if (error < 0)
							panic("sfork: sys_page_map failed (%e)", error);
					}
				}
			}
		}

copyend:

	// 复制异常栈
	error = sys_page_alloc(child, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P);
	if (error < 0)
		panic("fork: sys_page_alloc failed (%e)", error);

	error = sys_page_map(child, (void *)(UXSTACKTOP - PGSIZE), 0, UTEMP, PTE_U | PTE_W | PTE_P);
	if (error < 0)
		panic("fork: sys_page_map failed (%e)", error);

	memcpy(UTEMP, (void *)(UXSTACKTOP - PGSIZE), PGSIZE);

	error = sys_page_unmap(0, UTEMP);
	if (error < 0)
		panic("fork: sys_page_unmap failed (%e)", error);

	// 设置页面错误回调
	error = sys_env_set_pgfault_upcall(child, _pgfault_upcall);
	if (error < 0)
		panic("fork: sys_env_set_pgfault_upcall failed (%e)", error);

	error = sys_env_set_status(child, ENV_RUNNABLE);
	if (error < 0)
		panic("fork: sys_env_set_status failed (%e)", error);

	return child;
}
