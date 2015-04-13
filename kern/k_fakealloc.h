#ifndef K_FAKE_ALLOC_H
#define K_FAKE_ALLOC_H
#ifndef JOS_KERNEL
# error "This is a JOS kernel header; user programs should not #include it"
#endif
#include <kern/pmap.h>

// ���棺���ļ���������к������ǳ��� Hack�������� Lab3 Challenge2

char area[64][1024];
bool status[64] = { 0 };

// ���棺�ú���ֻ�����һҳ����������
void *
fake_calloc(size_t n, size_t size)
{
	int i;
	for (i = 0; i < 64; i++)
		if (!status[i])
		{
			status[i] = true;
			memset(area[i], 0, 1024);
			return area[i];
		}

	return NULL;
}

void
fake_free(void *p)
{
	status[((char *)p - (char *)area[0]) / 1024] = false;
}
#endif