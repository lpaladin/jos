#ifndef K_FAKE_ALLOC_H
#define K_FAKE_ALLOC_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif
#include <kern/pmap.h>

// 警告：该函数只会分配一页，参数无用
void *
fake_calloc(size_t n, size_t size)
{
	struct PageInfo *pp;

	// 屏蔽Warning
	n = size;

	pp = page_alloc(ALLOC_ZERO);

	return page2kva(pp);
}

void
fake_free(void *p)
{
	page_remove(kern_pgdir, p);
}
#endif