// User-level page fault handler support.
// Rather than register the C page fault handler directly with the
// kernel as the page fault handler, we register the assembly language
// wrapper in pfentry.S, which in turns calls the registered C
// function.

#include <inc/lib.h>

bool stack_allocated = false;

// Assembly language pgfault entrypoint defined in lib/pfentry.S.
extern void _pgfault_upcall(void);

// Pointer to currently installed C-language pgfault handler.
void (*_pgfault_handler)(struct UTrapframe *utf);

//
// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _pgfault_upcall routine when a page fault occurs.
//
void
set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
{
	int r;

	if (!stack_allocated)
	{
		if ((r = sys_page_alloc(0, (void *) (UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)))
			panic("set_pgfault_handler: page_alloc failed (%e)", r);
		stack_allocated = true;
	}

	if (_pgfault_handler == 0) {
		// First time through!
		// LAB 4: Your code here.

		if ((r = sys_env_set_pgfault_upcall(0, _pgfault_upcall)))
			panic("set_pgfault_handler: set_upcall failed (%e)", r);
		// panic("set_pgfault_handler not implemented");
	}

	// Save handler pointer for assembly to call.
	_pgfault_handler = handler;
}

// Lab 4 挑战 5：允许用户处理更多异常

extern void _oe_upcall(void);

void(*_oe_handler)(struct UTrapframe *utf);

// 设置用户态其他异常处理程序
void
set_oe_handler(void(*handler)(struct UTrapframe *utf))
{
	int r;

	if (!stack_allocated)
	{
		if ((r = sys_page_alloc(0, (void *)(UXSTACKTOP - PGSIZE), PTE_U | PTE_W | PTE_P)))
			panic("set_oe_handler: page_alloc failed (%e)", r);
		stack_allocated = true;
	}

	if (_oe_handler == 0) {
		if ((r = sys_env_set_other_exception_upcall(0, _oe_upcall)))
			panic("set_oe_handler: sys_env_set_other_exception_upcall failed (%e)", r);
		// panic("set_pgfault_handler not implemented");
	}

	// Save handler pointer for assembly to call.
	_oe_handler = handler;
}
