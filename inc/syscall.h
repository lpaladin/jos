#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

#define USE_SYSENTER // 决定是否使用 sysenter 指令（Lab3 Challenge3）

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
