/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
	user_mem_assert(curenv, s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//	-E_NO_FREE_ENV if no free environment is available.
//	-E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
	// Create the new environment with env_alloc(), from kern/env.c.
	// It should be left as env_alloc created it, except that
	// status is set to ENV_NOT_RUNNABLE, and the register set is copied
	// from the current environment -- but tweaked so sys_exofork
	// will appear to return 0.

	// LAB 4: Your code here.

	struct Env *e;
	int error;

	error = env_alloc(&e, curenv->env_id);
	if (error)
		return error;

	e->env_status = ENV_NOT_RUNNABLE;
	e->env_tf = curenv->env_tf;
	e->env_tf.tf_regs.reg_eax = 0;

	return e->env_id;
	// panic("sys_exofork not implemented");
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
	// Hint: Use the 'envid2env' function from kern/env.c to translate an
	// envid to a struct Env.
	// You should set envid2env's third argument to 1, which will
	// check whether the current environment has permission to set
	// envid's status.

	// LAB 4: Your code here.

	struct Env *env;
	int error;

	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE)
		return -E_INVAL;

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	env->env_status = status;
	return 0;
	// panic("sys_env_set_status not implemented");
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
	// LAB 4: Your code here.

	struct Env *env;
	int error;

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	env->env_pgfault_upcall = func;
	return 0;
	// panic("sys_env_set_pgfault_upcall not implemented");
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 4: Your code here.

	struct Env *env;
	struct PageInfo *p;
	int error;

	if ((perm & PTE_U) != PTE_U || (perm & PTE_P) != PTE_P ||
		(perm & ~PTE_SYSCALL) || (uint32_t) va >= UTOP || (uint32_t)(va) % PGSIZE != 0)
		return -E_INVAL;

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	p = page_alloc(ALLOC_ZERO);
	if (!p)
		return -E_NO_MEM;

	error = page_insert(env->env_pgdir, p, va, perm);

	if (error)
	{
		page_free(p);
		return -E_NO_MEM;
	}

	return 0;
	//panic("sys_page_alloc not implemented");
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 4: Your code here.

	struct Env *srcenv, *dstenv;
	pte_t *pte;
	struct PageInfo *p;
	int error;

	if ((perm & PTE_U) != PTE_U || (perm & PTE_P) != PTE_P ||
		(perm & ~PTE_SYSCALL) || (uint32_t) srcva >= UTOP || (uint32_t)(srcva) % PGSIZE != 0 ||
		(uint32_t) dstva >= UTOP || (uint32_t)(dstva) % PGSIZE != 0)
		return -E_INVAL;

	error = envid2env(srcenvid, &srcenv, true);
	if (error)
		return error;

	error = envid2env(dstenvid, &dstenv, true);
	if (error)
		return error;

	p = page_lookup(srcenv->env_pgdir, srcva, &pte);

	if (!p || (perm & PTE_W && !(*pte & PTE_W)))
		return -E_INVAL;

	return page_insert(dstenv->env_pgdir, p, dstva, perm);
	// panic("sys_page_map not implemented");
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 4: Your code here.

	struct Env *env;
	struct PageInfo *p;
	int error;

	if ((uint32_t) va >= UTOP || (uint32_t)(va) % PGSIZE != 0)
		return -E_INVAL;

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	page_remove(env->env_pgdir, va);
	return 0;
	// panic("sys_page_unmap not implemented");
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
	// LAB 4: Your code here.

	struct Env *dstenv;
	pte_t *pte;
	struct PageInfo *p;
	int error;

	error = envid2env(envid, &dstenv, false);
	if (error)
		return error;

	if (!dstenv->env_ipc_recving)
		return -E_IPC_NOT_RECV;

	if ((uint32_t)srcva < UTOP && (uint32_t)dstenv->env_ipc_dstva < UTOP)
	{
		// 发送内存映射

		if ((perm & PTE_U) != PTE_U || (perm & PTE_P) != PTE_P ||
			(perm & ~PTE_SYSCALL) || (uint32_t)(srcva) % PGSIZE != 0)
			return -E_INVAL;

		p = page_lookup(curenv->env_pgdir, srcva, &pte);

		if (!p || (perm & PTE_W && !(*pte & PTE_W)))
			return -E_INVAL;

		error = page_insert(dstenv->env_pgdir, p, dstenv->env_ipc_dstva, perm);
		if (error < 0)
			return error;

		dstenv->env_ipc_perm = perm;
	}
	else
		dstenv->env_ipc_perm = 0;

	dstenv->env_ipc_from = curenv->env_id;
	dstenv->env_ipc_value = value;
	dstenv->env_ipc_recving = false;

	// 标记返回
	dstenv->env_tf.tf_regs.reg_eax = 0;
	dstenv->env_status = ENV_RUNNABLE;

	return 0;
	// panic("sys_ipc_try_send not implemented");
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	// LAB 4: Your code here.
	
	if ((uint32_t)dstva < UTOP && (uint32_t)dstva % PGSIZE != 0)
		return -E_INVAL;

	curenv->env_ipc_dstva = dstva;
	curenv->env_ipc_recving = true;
	curenv->env_status = ENV_NOT_RUNNABLE;
	sched_yield();

	// panic("sys_ipc_recv not implemented");
	
	// 控制流不会到达这里的。
	return 0;
}

// Lab 4 挑战 4：实现进程的时空穿越

static struct Env saved_env;
static struct PageInfo *saved_pages[1024];
static pde_t saved_pgdir[NPDENTRIES];
static pte_t *saved_pgtab[NPDENTRIES];

// 保存进程状态的系统调用
// 注意整个系统中只有一个存储空间
// 如果有进程调用过，那么在没有使用这个状态恢复原本进程之前
// 不能再次分配
static int
sys_capture_state(envid_t envid)
{
	struct Env *env;
	struct PageInfo *p;
	pde_t *pgdir, pde;
	pte_t *pte;
	uint32_t temp, va, offset, pgcount = 0;
	int error;

	if (saved_pages[0])
	{
		if (envs[ENVX(saved_env.env_id)].env_status == ENV_DYING || envs[ENVX(saved_env.env_id)].env_status == ENV_FREE)
		{
			// 清空存储
			for (va = 0; va < UTOP;)
				if ((pde = saved_pgdir[PDX(va)]) & PTE_P)
				{
					page_free(pa2page(PADDR(saved_pgtab[PDX(va)])));
				}

			for (va = 0; va < UTOP;)
			{
				if ((pde = pgdir[PDX(va)]) & PTE_P)
				{
					for (offset = 0; offset < NPTENTRIES; offset++)
					{
						pte = (pte_t *)KADDR(PTE_ADDR(pde)) + PTX(va + offset * PGSIZE);
						if (*pte & PTE_P)
						{
							page_free(saved_pages[pgcount]);
							pgcount++;
						}
					}
				}
				va += PTSIZE;
			}

			saved_pages[0] = NULL;
			pgcount = 0;
		}
		else
			return -E_NO_MEM;
	}

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	saved_env = *env;

	pgdir = env->env_pgdir;

	memset(saved_pages, 0, sizeof(saved_pages));
	memset(saved_pgdir, 0, sizeof(saved_pgdir));
	memset(saved_pgtab, 0, sizeof(saved_pgtab));

	// 保存页目录
	memcpy(saved_pgdir, pgdir, PGSIZE);

	for (va = 0; va < UTOP; )
	{
		if ((pde = pgdir[PDX(va)]) & PTE_P)
		{
			// 保存页表
			cprintf("Saving PTE table...\n");
			saved_pgtab[PDX(va)] = memcpy(page2kva(page_alloc(0)), KADDR(PTE_ADDR(pde)), PGSIZE);
			for (offset = 0; offset < NPTENTRIES; offset++)
			{
				pte = (pte_t *)KADDR(PTE_ADDR(pde)) + PTX(va + offset * PGSIZE);
				if (*pte & PTE_P)
				{
					cprintf("Copying page content...\n");
					// 复制页面到临时存储
					saved_pages[pgcount] = page_alloc(0);
					memcpy(page2kva(saved_pages[pgcount]), KADDR(PTE_ADDR(*pte)), PGSIZE);
					pgcount++;
				}
			}
		}
		va += PTSIZE;
	}

	return 0;
}

// 恢复进程状态的系统调用
static int
sys_restore_state(envid_t envid)
{
	struct Env *env;
	struct PageInfo *p;
	pde_t *pgdir, pde;
	pte_t *pte;
	uint32_t temp, va, offset, pgcount = 0;
	int error;

	if (!saved_pages[0])
		return -E_INVAL;

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	if (env->env_id != saved_env.env_id)
		return -E_BAD_ENV;

	*env = saved_env;

	// 恢复页表
	cprintf("Restoring PTE table...\n");
	memcpy(pgdir = env->env_pgdir, saved_pgdir, PGSIZE);
	for (va = 0; va < UTOP; va += PTSIZE)
		if ((pde = pgdir[PDX(va)]) & PTE_P)
		{
			memcpy(KADDR(PTE_ADDR(pde)), saved_pgtab[PDX(va)], PGSIZE);
			page_free(pa2page(PADDR(saved_pgtab[PDX(va)])));
		}

	cprintf("Restoring pages...\n");
	for (va = 0; va < UTOP;)
	{
		if ((pde = pgdir[PDX(va)]) & PTE_P)
		{
			for (offset = 0; offset < NPTENTRIES; offset++)
			{
				pte = (pte_t *)KADDR(PTE_ADDR(pde)) + PTX(va + offset * PGSIZE);
				if (*pte & PTE_P)
				{
					cprintf("Copying page content...\n");
					// 恢复页面内容
					memcpy(KADDR(PTE_ADDR(*pte)), page2kva(saved_pages[pgcount]), PGSIZE);
					page_free(saved_pages[pgcount]);
					pgcount++;
				}
			}
		}
		va += PTSIZE;
	}

	saved_pages[0] = NULL;

	return 0;
}

// Lab 4 挑战 5：允许用户处理更多异常
// 设置异常处理程序
static int
sys_env_set_other_exception_upcall(envid_t envid, void *func)
{
	struct Env *env;
	int error;

	error = envid2env(envid, &env, true);
	if (error)
		return error;

	env->env_other_exception_upcall = func;
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	// panic("syscall not implemented");

	switch (syscallno)
	{
	case SYS_cputs:
		sys_cputs((char *)a1, a2);
		return 0;
	case SYS_cgetc:
		return sys_cgetc();
	case SYS_getenvid:
		return sys_getenvid();
	case SYS_env_destroy:
		return sys_env_destroy(a1);
	case SYS_yield:
		return sys_yield(), 0;
	case SYS_exofork:
		return sys_exofork();
	case SYS_env_set_status:
		return sys_env_set_status(a1, a2);
	case SYS_page_alloc:
		return sys_page_alloc(a1, (void *)a2, a3);
	case SYS_page_map:
		return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);
	case SYS_page_unmap:
		return sys_page_unmap(a1, (void *)a2);
	case SYS_env_set_pgfault_upcall:
		return sys_env_set_pgfault_upcall(a1, (void *)a2);
	case SYS_ipc_try_send:
		return sys_ipc_try_send(a1, a2, (void *)a3, a4);
	case SYS_ipc_recv:
		return sys_ipc_recv((void *)a1);
	case 128: // SYS_capture_state
		return sys_capture_state(a1);
	case 129: // SYS_restore_state
		return sys_restore_state(a1);
	case 130: // SYS_env_set_other_exception_upcall
		return sys_env_set_other_exception_upcall(a1, (void *)a2);
	default:
		return -E_NO_SYS;
	}
}

