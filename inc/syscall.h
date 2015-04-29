#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

// #define USE_SYSENTER // 决定是否使用 sysenter 指令（Lab3 Challenge3）

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_exofork,
	SYS_env_set_status,
	SYS_env_set_pgfault_upcall,
	SYS_yield,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	NSYSCALLS
};

#endif /* !JOS_INC_SYSCALL_H */
