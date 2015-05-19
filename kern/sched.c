#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/spinlock.h>
#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/monitor.h>
#include <kern/kclock.h>

#define LOTTERY_SCHEDULER

void sched_halt(void);

// Lab 4 挑战 2：实现另一种调度机制

// 伪随机数产生器
uint32_t
rand()
{
	// 梅森旋转算法
	// 参考自：http://zh.wikipedia.org/wiki/%E6%A2%85%E6%A3%AE%E6%97%8B%E8%BD%AC%E7%AE%97%E6%B3%95
	static uint32_t MT[624], index = 0;
	static bool initialized = false;
	uint32_t i, y;

	if (!initialized)
	{
		// 初始化种子和状态
		MT[0] = mc146818_read(0) + mc146818_read(2) * 60 + mc146818_read(4) * 3600; // 当前系统时间
		for (i = 1; i < 624; i++)
			MT[i] = 0x6c078965 * (MT[i - 1] ^ (MT[i - 1] >> 30)) + i; // 初始化发生器的状态

		initialized = true;
	}

	if (index == 0)
	{
		// 生成随机数
		for (i = 0; i < 624; i++)
		{
			y = (MT[i] & 0x80000000) + (MT[(i + 1) % 624] & 0x7fffffff);
			MT[i] = MT[(i + 397) % 624] ^ (y >> 1);
			if (y % 2 != 0)
				MT[i] = MT[i] ^ 0x9908b0df;
		}
	}

	y = MT[index];
	y = y ^ (y >> 11);
	y = y ^ ((y << 7) & 0x9d2c5680);
	y = y ^ ((y << 15) & 0xefc60000);
	y = y ^ (y >> 18);

	index = (index + 1) % 624;
	return y;
}

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *cur = curenv;
	int base = 0, i, x = 0;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.

	// LAB 4: Your code here.

	if (cur)
		base = ENVX(cur->env_id);

#ifdef LOTTERY_SCHEDULER
	for (i = 0; i < NENV; i++)
		if (envs[i].env_status == ENV_RUNNABLE)
			x += envs[i].lottery_count;

	if (x == 0)
	{
		if (cur && cur->env_status == ENV_RUNNING)
			return env_run(cur);
		else
			goto sched_notfound;
	}

	x = rand() % x;

	while (true)
		for (i = 0; i < NENV; i++)
			if (envs[i].env_status == ENV_RUNNABLE)
			{
				x -= envs[i].lottery_count;
				if (x < 0)
					goto sched_found;
			}

	sched_found:
	env_run(envs + i);

	sched_notfound:
#else
	for (i = 0; i < NENV; i++)
		if (envs[(i + base) % NENV].env_status == ENV_RUNNABLE)
			env_run(&envs[(i + base) % NENV]);

	if (i == NENV && cur && cur->env_status == ENV_RUNNING)
		env_run(cur);
#endif

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
//
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if ((envs[i].env_status == ENV_RUNNABLE ||
		     envs[i].env_status == ENV_RUNNING ||
		     envs[i].env_status == ENV_DYING))
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile (
		"movl $0, %%ebp\n"
		"movl %0, %%esp\n"
		"pushl $0\n"
		"pushl $0\n"
		"sti\n"
		"1:\n"
		"hlt\n"
		"jmp 1b\n"
	: : "a" (thiscpu->cpu_ts.ts_esp0));
}

