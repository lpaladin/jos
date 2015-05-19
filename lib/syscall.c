// System call stubs.

#include <inc/syscall.h>
#include <inc/lib.h>

// Lab 4 挑战 7：批量系统调用
bool in_batch_state = false;
int batch_syscall_count = 0;
uint32_t batch_callno[64], argustores[5][64], 
*batch_argu[5] = { argustores[0], argustores[1], argustores[2], argustores[3], argustores[4] };

#ifdef USE_SYSENTER

__attribute__((noinline)) static int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// sysenter 指令

	asm volatile(
		"push %%esi\n"
		"push %%ebp\n"
		"movl %%esp, %%ebp\n"
		"push %%esp\n"
		"leal _sysenter_end, %%esi\n"
		"sysenter\n"
		"_sysenter_end:\n"
		"pop %%esp\n"
		"pop %%ebp\n"
		"pop %%esi\n"
		: "=a" (ret)
		:
		"a" (num),
		"d" (a1),
		"c" (a2),
		"b" (a3),
		"D" (a4)
		: "cc", "memory");

#else

static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	if (in_batch_state && (num == SYS_page_map || num == SYS_page_unmap || num == SYS_page_alloc))
	{
		// 批量处理模式，先添加入缓存
		if (batch_syscall_count == 64)
		{
			// 系统调用缓存满了，提交之前的系统调用
			ret = syscall(131, 0, (uint32_t)batch_callno, (uint32_t)batch_argu, 0, 0, 0);
			batch_syscall_count = 0;
			if (ret < 0)
				return ret;
		}

		batch_callno[batch_syscall_count] = num;
		batch_argu[0][batch_syscall_count] = a1;
		batch_argu[1][batch_syscall_count] = a2;
		batch_argu[2][batch_syscall_count] = a3;
		batch_argu[3][batch_syscall_count] = a4;
		batch_argu[4][batch_syscall_count] = a5;
		batch_syscall_count++;
		return 0;
	}

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.

	asm volatile("int %1\n"
		: "=a" (ret)
		: "i" (T_SYSCALL),
		  "a" (num),
		  "d" (a1),
		  "c" (a2),
		  "b" (a3),
		  "D" (a4),
		  "S" (a5)
		: "cc", "memory");

#endif

	if(check && ret > 0)
		panic("syscall %d returned %d (> 0)", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	 return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	return syscall(SYS_page_alloc, 1, envid, (uint32_t) va, perm, 0, 0);
}

int
sys_page_map(envid_t srcenv, void *srcva, envid_t dstenv, void *dstva, int perm)
{
	return syscall(SYS_page_map, 1, srcenv, (uint32_t) srcva, dstenv, (uint32_t) dstva, perm);
}

int
sys_page_unmap(envid_t envid, void *va)
{
	return syscall(SYS_page_unmap, 1, envid, (uint32_t) va, 0, 0, 0);
}

// sys_exofork is inlined in lib.h

int
sys_env_set_status(envid_t envid, int status)
{
	return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0);
}

int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	return syscall(SYS_env_set_trapframe, 1, envid, (uint32_t) tf, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uint32_t) upcall, 0, 0, 0);
}

int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, int perm)
{
	return syscall(SYS_ipc_try_send, 0, envid, value, (uint32_t) srcva, perm, 0);
}

int
sys_ipc_recv(void *dstva)
{
	return syscall(SYS_ipc_recv, 1, (uint32_t)dstva, 0, 0, 0, 0);
}

int
sys_capture_state(envid_t envid)
{
	return syscall(128, 1, envid, 0, 0, 0, 0);
}

int
sys_restore_state(envid_t envid)
{
	return syscall(129, 1, envid, 0, 0, 0, 0);
}

int
sys_env_set_other_exception_upcall(envid_t envid, void *upcall)
{
	return syscall(130, 1, envid, (uint32_t)upcall, 0, 0, 0);
}

// 清空缓存、记录其后的系统调用
int
begin_batchcall()
{
	if (in_batch_state)
		return -E_INVAL;
	in_batch_state = true;
	batch_syscall_count = 0;
	return 0;
}

// 提交所有系统调用、清空缓存
int
end_batchcall()
{
	int32_t ret;
	if (!in_batch_state)
		return -E_INVAL;
	if (batch_syscall_count < 64)
		batch_callno[batch_syscall_count] = 0xFFFFFFFF;

	ret = syscall(131, 0, (uint32_t)batch_callno, (uint32_t)batch_argu, 0, 0, 0);
	batch_syscall_count = 0;
	in_batch_state = false;
	return ret;
}
