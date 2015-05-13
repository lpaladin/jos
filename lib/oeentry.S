
// Lab 4 挑战 5：允许用户处理更多异常

#include <inc/mmu.h>
#include <inc/memlayout.h>

.text
.globl _oe_upcall
_oe_upcall:
	// Call the C other exception handler.
	pushl %esp			// function argument: pointer to UTF
	movl _oe_handler, %eax
	call *%eax
	addl $4, %esp			// pop function argument
	
	movl 0x30(%esp), %ebp
	addl $-4, %ebp
	movl %ebp, 0x30(%esp)

	movl 0x28(%esp), %eax
	movl %eax, 0(%ebp)


	movl 0x2c(%esp), %eax
	pushl %eax
	popf

	movl 0x24(%esp), %eax
	movl 0x20(%esp), %ecx
	movl 0x1c(%esp), %edx
	movl 0x18(%esp), %ebx
	movl 0x10(%esp), %ebp
	movl 0xc(%esp), %esi
	movl 0x8(%esp), %edi

	movl 0x30(%esp), %esp

	ret