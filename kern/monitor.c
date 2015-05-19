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
#include <kern/trap.h>
#include <kern/pmap.h>
#include <kern/env.h>
#include <kern/libdisasm/libdis.h>

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
	{ "memdump", "Dump the contents of a range of memory", mon_memdump },
	{ "testint", "Run an instruction 'int $<arg>'", mon_testint },
	{ "si", "Run the next instruction of current environment and stop", mon_si },
	{ "exit", "Switch back to the current environment", mon_exit }
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

#define MAX_LINE_FOR_MORE 23

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t ebp = read_ebp(), eip, i, line_count = 0;
	struct Eipdebuginfo info;
	cprintf("Stack backtrace:\n");
	for (; ebp != 0; ebp = READ_ADDR(ebp))
	{
		if ((line_count += 2) >= MAX_LINE_FOR_MORE)
		{
			cprintf("\033[1;31;47m--- Press any key for more ---\033[0m\n");
			getchar();
			line_count = 0;
		}
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

int
mon_testint(int argc, char **argv, struct Trapframe *tf)
{
	uint16_t id = strtol(argv[1], NULL, 10);

	// 运行一条 int $x 指令
	switch (id)
	{
#if 1
		case 0: __asm __volatile("int $0"); break;
		case 1: __asm __volatile("int $1"); break;
		case 2: __asm __volatile("int $2"); break;
		case 3: __asm __volatile("int $3"); break;
		case 4: __asm __volatile("int $4"); break;
		case 5: __asm __volatile("int $5"); break;
		case 6: __asm __volatile("int $6"); break;
		case 7: __asm __volatile("int $7"); break;
		case 8: __asm __volatile("int $8"); break;
		case 9: __asm __volatile("int $9"); break;
		case 10: __asm __volatile("int $10"); break;
		case 11: __asm __volatile("int $11"); break;
		case 12: __asm __volatile("int $12"); break;
		case 13: __asm __volatile("int $13"); break;
		case 14: __asm __volatile("int $14"); break;
		case 15: __asm __volatile("int $15"); break;
		case 16: __asm __volatile("int $16"); break;
		case 17: __asm __volatile("int $17"); break;
		case 18: __asm __volatile("int $18"); break;
		case 19: __asm __volatile("int $19"); break;
		case 20: __asm __volatile("int $20"); break;
		case 21: __asm __volatile("int $21"); break;
		case 22: __asm __volatile("int $22"); break;
		case 23: __asm __volatile("int $23"); break;
		case 24: __asm __volatile("int $24"); break;
		case 25: __asm __volatile("int $25"); break;
		case 26: __asm __volatile("int $26"); break;
		case 27: __asm __volatile("int $27"); break;
		case 28: __asm __volatile("int $28"); break;
		case 29: __asm __volatile("int $29"); break;
		case 30: __asm __volatile("int $30"); break;
		case 31: __asm __volatile("int $31"); break;
		case 32: __asm __volatile("int $32"); break;
		case 33: __asm __volatile("int $33"); break;
		case 34: __asm __volatile("int $34"); break;
		case 35: __asm __volatile("int $35"); break;
		case 36: __asm __volatile("int $36"); break;
		case 37: __asm __volatile("int $37"); break;
		case 38: __asm __volatile("int $38"); break;
		case 39: __asm __volatile("int $39"); break;
		case 40: __asm __volatile("int $40"); break;
		case 41: __asm __volatile("int $41"); break;
		case 42: __asm __volatile("int $42"); break;
		case 43: __asm __volatile("int $43"); break;
		case 44: __asm __volatile("int $44"); break;
		case 45: __asm __volatile("int $45"); break;
		case 46: __asm __volatile("int $46"); break;
		case 47: __asm __volatile("int $47"); break;
		case 48: __asm __volatile("int $48"); break;
		case 49: __asm __volatile("int $49"); break;
		case 50: __asm __volatile("int $50"); break;
		case 51: __asm __volatile("int $51"); break;
		case 52: __asm __volatile("int $52"); break;
		case 53: __asm __volatile("int $53"); break;
		case 54: __asm __volatile("int $54"); break;
		case 55: __asm __volatile("int $55"); break;
		case 56: __asm __volatile("int $56"); break;
		case 57: __asm __volatile("int $57"); break;
		case 58: __asm __volatile("int $58"); break;
		case 59: __asm __volatile("int $59"); break;
		case 60: __asm __volatile("int $60"); break;
		case 61: __asm __volatile("int $61"); break;
		case 62: __asm __volatile("int $62"); break;
		case 63: __asm __volatile("int $63"); break;
		case 64: __asm __volatile("int $64"); break;
		case 65: __asm __volatile("int $65"); break;
		case 66: __asm __volatile("int $66"); break;
		case 67: __asm __volatile("int $67"); break;
		case 68: __asm __volatile("int $68"); break;
		case 69: __asm __volatile("int $69"); break;
		case 70: __asm __volatile("int $70"); break;
		case 71: __asm __volatile("int $71"); break;
		case 72: __asm __volatile("int $72"); break;
		case 73: __asm __volatile("int $73"); break;
		case 74: __asm __volatile("int $74"); break;
		case 75: __asm __volatile("int $75"); break;
		case 76: __asm __volatile("int $76"); break;
		case 77: __asm __volatile("int $77"); break;
		case 78: __asm __volatile("int $78"); break;
		case 79: __asm __volatile("int $79"); break;
		case 80: __asm __volatile("int $80"); break;
		case 81: __asm __volatile("int $81"); break;
		case 82: __asm __volatile("int $82"); break;
		case 83: __asm __volatile("int $83"); break;
		case 84: __asm __volatile("int $84"); break;
		case 85: __asm __volatile("int $85"); break;
		case 86: __asm __volatile("int $86"); break;
		case 87: __asm __volatile("int $87"); break;
		case 88: __asm __volatile("int $88"); break;
		case 89: __asm __volatile("int $89"); break;
		case 90: __asm __volatile("int $90"); break;
		case 91: __asm __volatile("int $91"); break;
		case 92: __asm __volatile("int $92"); break;
		case 93: __asm __volatile("int $93"); break;
		case 94: __asm __volatile("int $94"); break;
		case 95: __asm __volatile("int $95"); break;
		case 96: __asm __volatile("int $96"); break;
		case 97: __asm __volatile("int $97"); break;
		case 98: __asm __volatile("int $98"); break;
		case 99: __asm __volatile("int $99"); break;
		case 100: __asm __volatile("int $100"); break;
		case 101: __asm __volatile("int $101"); break;
		case 102: __asm __volatile("int $102"); break;
		case 103: __asm __volatile("int $103"); break;
		case 104: __asm __volatile("int $104"); break;
		case 105: __asm __volatile("int $105"); break;
		case 106: __asm __volatile("int $106"); break;
		case 107: __asm __volatile("int $107"); break;
		case 108: __asm __volatile("int $108"); break;
		case 109: __asm __volatile("int $109"); break;
		case 110: __asm __volatile("int $110"); break;
		case 111: __asm __volatile("int $111"); break;
		case 112: __asm __volatile("int $112"); break;
		case 113: __asm __volatile("int $113"); break;
		case 114: __asm __volatile("int $114"); break;
		case 115: __asm __volatile("int $115"); break;
		case 116: __asm __volatile("int $116"); break;
		case 117: __asm __volatile("int $117"); break;
		case 118: __asm __volatile("int $118"); break;
		case 119: __asm __volatile("int $119"); break;
		case 120: __asm __volatile("int $120"); break;
		case 121: __asm __volatile("int $121"); break;
		case 122: __asm __volatile("int $122"); break;
		case 123: __asm __volatile("int $123"); break;
		case 124: __asm __volatile("int $124"); break;
		case 125: __asm __volatile("int $125"); break;
		case 126: __asm __volatile("int $126"); break;
		case 127: __asm __volatile("int $127"); break;
		case 128: __asm __volatile("int $128"); break;
		case 129: __asm __volatile("int $129"); break;
		case 130: __asm __volatile("int $130"); break;
		case 131: __asm __volatile("int $131"); break;
		case 132: __asm __volatile("int $132"); break;
		case 133: __asm __volatile("int $133"); break;
		case 134: __asm __volatile("int $134"); break;
		case 135: __asm __volatile("int $135"); break;
		case 136: __asm __volatile("int $136"); break;
		case 137: __asm __volatile("int $137"); break;
		case 138: __asm __volatile("int $138"); break;
		case 139: __asm __volatile("int $139"); break;
		case 140: __asm __volatile("int $140"); break;
		case 141: __asm __volatile("int $141"); break;
		case 142: __asm __volatile("int $142"); break;
		case 143: __asm __volatile("int $143"); break;
		case 144: __asm __volatile("int $144"); break;
		case 145: __asm __volatile("int $145"); break;
		case 146: __asm __volatile("int $146"); break;
		case 147: __asm __volatile("int $147"); break;
		case 148: __asm __volatile("int $148"); break;
		case 149: __asm __volatile("int $149"); break;
		case 150: __asm __volatile("int $150"); break;
		case 151: __asm __volatile("int $151"); break;
		case 152: __asm __volatile("int $152"); break;
		case 153: __asm __volatile("int $153"); break;
		case 154: __asm __volatile("int $154"); break;
		case 155: __asm __volatile("int $155"); break;
		case 156: __asm __volatile("int $156"); break;
		case 157: __asm __volatile("int $157"); break;
		case 158: __asm __volatile("int $158"); break;
		case 159: __asm __volatile("int $159"); break;
		case 160: __asm __volatile("int $160"); break;
		case 161: __asm __volatile("int $161"); break;
		case 162: __asm __volatile("int $162"); break;
		case 163: __asm __volatile("int $163"); break;
		case 164: __asm __volatile("int $164"); break;
		case 165: __asm __volatile("int $165"); break;
		case 166: __asm __volatile("int $166"); break;
		case 167: __asm __volatile("int $167"); break;
		case 168: __asm __volatile("int $168"); break;
		case 169: __asm __volatile("int $169"); break;
		case 170: __asm __volatile("int $170"); break;
		case 171: __asm __volatile("int $171"); break;
		case 172: __asm __volatile("int $172"); break;
		case 173: __asm __volatile("int $173"); break;
		case 174: __asm __volatile("int $174"); break;
		case 175: __asm __volatile("int $175"); break;
		case 176: __asm __volatile("int $176"); break;
		case 177: __asm __volatile("int $177"); break;
		case 178: __asm __volatile("int $178"); break;
		case 179: __asm __volatile("int $179"); break;
		case 180: __asm __volatile("int $180"); break;
		case 181: __asm __volatile("int $181"); break;
		case 182: __asm __volatile("int $182"); break;
		case 183: __asm __volatile("int $183"); break;
		case 184: __asm __volatile("int $184"); break;
		case 185: __asm __volatile("int $185"); break;
		case 186: __asm __volatile("int $186"); break;
		case 187: __asm __volatile("int $187"); break;
		case 188: __asm __volatile("int $188"); break;
		case 189: __asm __volatile("int $189"); break;
		case 190: __asm __volatile("int $190"); break;
		case 191: __asm __volatile("int $191"); break;
		case 192: __asm __volatile("int $192"); break;
		case 193: __asm __volatile("int $193"); break;
		case 194: __asm __volatile("int $194"); break;
		case 195: __asm __volatile("int $195"); break;
		case 196: __asm __volatile("int $196"); break;
		case 197: __asm __volatile("int $197"); break;
		case 198: __asm __volatile("int $198"); break;
		case 199: __asm __volatile("int $199"); break;
		case 200: __asm __volatile("int $200"); break;
		case 201: __asm __volatile("int $201"); break;
		case 202: __asm __volatile("int $202"); break;
		case 203: __asm __volatile("int $203"); break;
		case 204: __asm __volatile("int $204"); break;
		case 205: __asm __volatile("int $205"); break;
		case 206: __asm __volatile("int $206"); break;
		case 207: __asm __volatile("int $207"); break;
		case 208: __asm __volatile("int $208"); break;
		case 209: __asm __volatile("int $209"); break;
		case 210: __asm __volatile("int $210"); break;
		case 211: __asm __volatile("int $211"); break;
		case 212: __asm __volatile("int $212"); break;
		case 213: __asm __volatile("int $213"); break;
		case 214: __asm __volatile("int $214"); break;
		case 215: __asm __volatile("int $215"); break;
		case 216: __asm __volatile("int $216"); break;
		case 217: __asm __volatile("int $217"); break;
		case 218: __asm __volatile("int $218"); break;
		case 219: __asm __volatile("int $219"); break;
		case 220: __asm __volatile("int $220"); break;
		case 221: __asm __volatile("int $221"); break;
		case 222: __asm __volatile("int $222"); break;
		case 223: __asm __volatile("int $223"); break;
		case 224: __asm __volatile("int $224"); break;
		case 225: __asm __volatile("int $225"); break;
		case 226: __asm __volatile("int $226"); break;
		case 227: __asm __volatile("int $227"); break;
		case 228: __asm __volatile("int $228"); break;
		case 229: __asm __volatile("int $229"); break;
		case 230: __asm __volatile("int $230"); break;
		case 231: __asm __volatile("int $231"); break;
		case 232: __asm __volatile("int $232"); break;
		case 233: __asm __volatile("int $233"); break;
		case 234: __asm __volatile("int $234"); break;
		case 235: __asm __volatile("int $235"); break;
		case 236: __asm __volatile("int $236"); break;
		case 237: __asm __volatile("int $237"); break;
		case 238: __asm __volatile("int $238"); break;
		case 239: __asm __volatile("int $239"); break;
		case 240: __asm __volatile("int $240"); break;
		case 241: __asm __volatile("int $241"); break;
		case 242: __asm __volatile("int $242"); break;
		case 243: __asm __volatile("int $243"); break;
		case 244: __asm __volatile("int $244"); break;
		case 245: __asm __volatile("int $245"); break;
		case 246: __asm __volatile("int $246"); break;
		case 247: __asm __volatile("int $247"); break;
		case 248: __asm __volatile("int $248"); break;
		case 249: __asm __volatile("int $249"); break;
		case 250: __asm __volatile("int $250"); break;
		case 251: __asm __volatile("int $251"); break;
		case 252: __asm __volatile("int $252"); break;
		case 253: __asm __volatile("int $253"); break;
		case 254: __asm __volatile("int $254"); break;
		case 255: __asm __volatile("int $255"); break;
#endif
	}
	return 0;
}

int
mon_si(int argc, char **argv, struct Trapframe *tf)
{
	if (!tf)
	{
		cprintf("No environment running.\n");
		return 0;
	}

	// 开启单步并退出 monitor
	tf->tf_eflags |= FL_TF;
	return -1;
}

int
mon_exit(int argc, char **argv, struct Trapframe *tf)
{
	if (!tf)
	{
		cprintf("No environment running.\n");
		return 0;
	}

	// 关闭单步并退出 monitor
	tf->tf_eflags &= ~FL_TF;
	return -1;
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

	if (tf == NULL)
	{
		cprintf("Welcome to the JOS kernel monitor!\n");
		cprintf("Currently no environment running.\n");
		cprintf("Type 'help' for a list of commands.\n");
	} 
	else
	{
		if (!(tf->tf_eflags & FL_TF))
		{
			cprintf("Welcome to the JOS kernel monitor!\n");
			cprintf("Type 'help' for a list of commands.\n");
			print_trapframe(tf);
		}
		show_nextinstr(tf);
	}

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

// 反汇编下一条指令
void
show_nextinstr(struct Trapframe *tf)
{
	static char buf[1024] = { 0 };
	if (!tf)
		return;
	disassemble_init(0, ATT_SYNTAX);
	sprint_address(buf, 1023, (void *)tf->tf_eip);
	cprintf("\033[1;34;43mCurrent instruction [0x%08x]\033[0m: \033[1;37;41m%s\033[0m\n", tf->tf_eip, buf);
	disassemble_cleanup();
}