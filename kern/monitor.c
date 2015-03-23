// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line
#define READ_ADDR(addr) (*(uint32_t *)(addr))

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display a listing of function call frames", mon_backtrace },
	{ "showmappings", "Display page mappings", mon_showmappings },
	{ "chmappingperm", "Change permission of a page mapping", mon_chmappingperm },
	{ "memdump", "Dump the contents of a range of memory", mon_memdump }
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < NCOMMANDS; i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp(), eip, i;
	struct Eipdebuginfo info;
	cprintf("Stack backtrace:\n");
	for (; ebp != 0; ebp = READ_ADDR(ebp))
	{
		eip = READ_ADDR(ebp + 4);

		// 函数栈帧基本信息
		cprintf("  ebp %08x  eip %08x  args", ebp, eip);
		for (i = 2; i < 7; i++)
			cprintf(" %08x", READ_ADDR(ebp + i * 4));
		cputchar('\n');

		// 根据 eip 得出的函数定义信息
		debuginfo_eip(eip, &info);
		cprintf("         %s:%d: %.*s+%d\n", info.eip_file,
			info.eip_line, info.eip_fn_namelen, info.eip_fn_name, eip - info.eip_fn_addr);
	}
	return 0;
}

#define MAX_LINE_FOR_MORE 23

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t begin, end, curr, line_count = 0;
	if (argc != 3 && argc != 2)
	{
		// 参数数目错误
		cprintf("Invalid argument count.\n\n"
			"Display in a useful and easy-to-read format"
			" all of the physical page mappings (or lack thereof)"
			" that apply to a particular range of virtual/linear addresses"
			" in the currently active address space.\n\nUsage:"
			" showmappings <begin_address> [<end_address>]\n");
		return 0;
	}
	if (parse_hexaddr(argv[1], &begin) || (argc == 3 && parse_hexaddr(argv[2], &end)))
	{
		// 参数不是十六进制地址
		cprintf("Invalid argument format - address should be in hex format.\n");
		return 0;
	}
	if (argc == 2)
		end = begin;
	if (begin > end)
	{
		curr = end;
		end = begin;
		begin = curr;
	}
	begin = ROUNDDOWN(begin, PGSIZE);
	end = ROUNDUP(end, PGSIZE);

	// 表头
	cprintf(" Virt Addr  Phys Addr PS U W P\n");
	for (; begin <= end; begin += (support_pse && curr & PTE_PS) ? PTSIZE : PGSIZE)
	{
		if (++line_count == MAX_LINE_FOR_MORE)
		{
			cprintf("\033[1;31;47m--- Press any key for more ---\033[0m\n");
			getchar();
			line_count = 0;
		}
		// 虚拟地址
		cprintf("\033[1;30;46m0x%08x\033[0m \033[1;31;42m", begin);
		
		// 读取PDE
		curr = kern_pgdir[PDX(begin)];
		if (!(curr & PTE_P))
			cprintf("--NO-PDE--");
		else if (support_pse && curr & PTE_PS)
			cprintf("0x%08x", PTE_ADDR(curr) + (PTX(begin) << PGSHIFT));
		else
		{
			// 读取PTE
			curr = ((pte_t *)KADDR(PTE_ADDR(curr)))[PTX(begin)];
			if (!(curr & PTE_P))
				cprintf("--NO-PTE--");
			else
				cprintf("0x%08x", PTE_ADDR(curr));
		}

		// 权限和属性位
		cprintf("\033[0m  %d %d %d %d\n", !!(curr & PTE_PS), !!(curr & PTE_U), !!(curr & PTE_W), !!(curr & PTE_P));
	}
	return 0;
}

int
mon_chmappingperm(int argc, char **argv, struct Trapframe *tf)
{
	int i, has_addr = 0;
	uint32_t addr = 0, curr, addperm = 0, clearperm = 0; // 要设置和清除的权限位

	// 解析参数
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '+')
			switch (argv[i][1])
		{
			case 'w':
			case 'W':
				addperm |= PTE_W;
				break;
			case 'u':
			case 'U':
				addperm |= PTE_U;
				break;
			default:
				goto BadArg;
		}
		else if (argv[i][0] == '-')
			switch (argv[i][1])
		{
			case 'w':
			case 'W':
				clearperm |= PTE_W;
				break;
			case 'u':
			case 'U':
				clearperm |= PTE_U;
				break;
			default:
				goto BadArg;
		}
		else if (parse_hexaddr(argv[i], &addr))
			goto BadArg;
		else
			has_addr = true;
	}
	if (!has_addr)
		goto BadArg;

	// 读取PDE
	curr = kern_pgdir[PDX(addr)];
	if (!(curr & PTE_P))
		cprintf("No PDE exist for the specified address.\n");
	else if (support_pse && curr & PTE_PS)
	{
		cprintf("Mapping: \033[1;30;46m0x%08x\033[0m => \033[1;31;42m0x%08x\033[0m\n", addr, PTE_ADDR(curr) + (PTX(addr) << PGSHIFT));
		curr = kern_pgdir[PDX(addr)] = (curr & ~6) | ((addperm & ~clearperm) & 6);
		cprintf("Permission changed successfully: U = %d W = %d\n", !!(curr & PTE_U), !!(curr & PTE_W));
	}
	else
	{
		curr = ((pte_t *)KADDR(PTE_ADDR(curr)))[PTX(addr)];
		if (!(curr & PTE_P))
			cprintf("No PTE exist for the specified address.\n");
		else
		{
			cprintf("Mapping: \033[1;30;46m0x%08x\033[0m => \033[1;31;42m0x%08x\033[0m\n", addr, PTE_ADDR(curr));
			curr = ((pte_t *)KADDR(PTE_ADDR(curr)))[PTX(addr)] = (curr & ~6) | ((addperm & ~clearperm) & 6);
			cprintf("Permission changed successfully: U = %d W = %d\n", !!(curr & PTE_U), !!(curr & PTE_W));
		}
	}
	return 0;

BadArg:
	cprintf("Invalid argument format.\n\n"
		"Usage: chmappingperm [[+-][UuWw]] <virtual_address>\n");
	return 0;
}

#define DUMP_BYTE_PER_LINE 16

int
mon_memdump(int argc, char **argv, struct Trapframe *tf)
{
	int is_phys_addr;
	uint32_t begin, end, curr, page_end, line_count = 0, i;
	if (argc != 4)
	{
		// 参数数目错误
		cprintf("Invalid argument count.\n\n");
		goto BadArg;
	}
	if (argv[1][0] == 'p')
		is_phys_addr = true;
	else if (argv[1][0] == 'v')
		is_phys_addr = false;
	else
	{
		// 到底是虚拟地址还是物理地址？
		cprintf("Invalid argument format - must specify v or p.\n\n");
		goto BadArg;
	}
	if (parse_hexaddr(argv[2], &begin) || parse_hexaddr(argv[3], &end))
	{
		// 参数不是十六进制地址
		cprintf("Invalid argument format - address should be in hex format.\n\n");
		goto BadArg;
	}
	if (begin > end)
	{
		curr = end;
		end = begin;
		begin = curr;
	}

	if (is_phys_addr)
	{
		begin += KERNBASE;
		end += KERNBASE;
		if (begin < KERNBASE || end < KERNBASE)
		{
			// 物理地址越界
			cprintf("Invalid argument - physical address out of range.\n\n");
			goto BadArg;
		}
	}


	while (begin <= end)
	{
		// 读取PDE
		curr = kern_pgdir[PDX(begin)];
		if (!(curr & PTE_P))
		{
			cprintf("\033[1;30;46m0x%08x\033[0m <Invalid Memory>\n", begin);
			if (begin % PTSIZE)
				begin = ROUNDUP(begin, PTSIZE);
			else
				begin += PTSIZE;
		}
		else if (support_pse && curr & PTE_PS)
		{
			if (begin % PTSIZE)
				page_end = ROUNDUP(begin, PTSIZE);
			else
				page_end = begin + PTSIZE;

			// 这是一个4MB的页，拼命输出吧
			for (begin = ROUNDDOWN(begin, DUMP_BYTE_PER_LINE); begin <= page_end && begin <= end; begin += DUMP_BYTE_PER_LINE)
			{
				if (++line_count == MAX_LINE_FOR_MORE)
				{
					cprintf("\033[1;31;47m--- Press any key for more ---\033[0m\n");
					getchar();
					line_count = 0;
				}
				// 虚拟地址
				cprintf("\033[1;30;46m0x%08x\033[0m", begin);
				for (i = 0; i < DUMP_BYTE_PER_LINE; i++)
					cprintf(" %02x", ((unsigned char *) begin)[i]);
				cputchar('\n');
			}
		}
		else
		{
			// 读取PTE
			curr = ((pte_t *)KADDR(PTE_ADDR(curr)))[PTX(begin)];
			if (!(curr & PTE_P))
			{
				cprintf("\033[1;30;46m0x%08x\033[0m <Invalid Memory>\n", begin);

				if (begin % PGSIZE)
					begin = ROUNDUP(begin, PGSIZE);
				else
					begin += PGSIZE;
			}
			else
			{
				if (begin % PGSIZE)
					page_end = ROUNDUP(begin, PGSIZE);
				else
					page_end = begin + PGSIZE;

				// 这是一个4KB的页，输出吧
				for (begin = ROUNDDOWN(begin, DUMP_BYTE_PER_LINE); begin <= page_end && begin <= end; begin += DUMP_BYTE_PER_LINE)
				{
					if (++line_count == MAX_LINE_FOR_MORE)
					{
						cprintf("\033[1;31;47m--- Press any key for more ---\033[0m\n");
						getchar();
						line_count = 0;
					}
					// 虚拟地址
					cprintf("\033[1;30;46m0x%08x\033[0m", begin);
					for (i = 0; i < DUMP_BYTE_PER_LINE; i++)
						cprintf(" %02x", ((unsigned char *)begin)[i]);
					cputchar('\n');
				}
			}
		}
	}
	return 0;

BadArg:
	cprintf("Dump the contents of a range of memory given either a virtual or physical address range.\n"
		"Usage: memdump [vp] <begin_address> <end_address>\n"
		"  specify v for virtual address, or p for physical address.\n");
	return 0;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < NCOMMANDS; i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");


	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

// 解析十六进制地址
int
parse_hexaddr(const char *str, uint32_t *result)
{
	uint32_t temp = 0;
	char curr;
	for (; *str != 0; str++)
	{
		if ((curr = *str) == 'x')
		{
			if (temp != 0)
				return -1;
		}
		else if (curr >= '0' && curr <= '9')
			temp = (temp << 4) + curr - '0';
		else if (curr >= 'A' && curr <= 'F')
			temp = (temp << 4) + curr - 'A' + 10;
		else if (curr >= 'a' && curr <= 'f')
			temp = (temp << 4) + curr - 'a' + 10;
		else
			return -1;
	}
	*result = temp;
	return 0;
}