
#include <inc/lib.h>

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: <message>", then causes a breakpoint exception,
 * which causes JOS to enter the JOS kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	// Print the panic message
	cprintf("\033[1;35;47m[%08x] user panic in %s at %s:%d: ",
		sys_getenvid(), binaryname, file, line);
	vcprintf(fmt, ap);
	cprintf("\033[0m\n");

	exit();

	// Cause a breakpoint exception
	while (1)
		asm volatile("int3");
}

