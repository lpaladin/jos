// Lab 4 挑战 5：允许用户处理更多异常
// 设置除零异常处理程序

#include <inc/lib.h>

static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < sizeof(excnames) / sizeof(excnames[0]))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + 16)
		return "Hardware Interrupt";
	return "(unknown trap)";
}

void
handler(struct UTrapframe *utf)
{
	uint32_t err = utf->utf_err;
	cprintf("I raised an exception, trap type is %s\n", trapname(err));
	sys_env_destroy(sys_getenvid());
}

void
umain(int argc, char **argv)
{
	int zero = 0;
	set_oe_handler(handler);
	cprintf("1/0 is %08x!\n", 1 / zero);
}
