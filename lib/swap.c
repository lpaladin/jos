
// 最终项目：换页
#include <inc/lib.h>
#define PAGEFILE_PGCOUNT 1024
#define PAGEFILE_SIZE (PAGEFILE_PGCOUNT + 1) * PGSIZE

char buf[PGSIZE];

int
swap_page_to_disk(void *va)
{
	int fd = open("/pagefile", O_CREAT | O_RDWR), i;
	if (fd > 0)
	{
		struct Stat statbuf;
		fstat(fd, &statbuf);
		if (statbuf.st_size != PAGEFILE_SIZE)
		{
			// 重新创建文件
			seek(fd, 0);

			// 空闲位图
			i = PAGEFILE_PGCOUNT / 8;
			memset(buf, -1, i);
			memset(buf + i, 0, PGSIZE - i);
			write(fd, buf, PGSIZE);

			// 写入余下的页面
			memset(buf, 0, PGSIZE);
			for (i = 0; i < PAGEFILE_PGCOUNT; i++)
				write(fd, buf, PGSIZE);
		}

		// 交换入磁盘
		seek(fd, 0);

		// 先读出空闲位图
		read(fd, buf, PGSIZE);
		for (i = 0; i < PAGEFILE_PGCOUNT; i += 8)
			if (buf[i])
			{
				char j, c;
				for (j = 0, c = 1; j < 8; j++, c <<= 1)
					if (buf[i] & c)
					{
						pte_t pte = uvpt[PGNUM(va)];

						// 找到了！
						seek(fd, (i + j) * PGSIZE);
						write(fd, va, PGSIZE);
						sys_page_unmap(0, va);

						// 保存偏移量入PTE
						sys_set_pte_pafield(va, (i + j) * PGSIZE, 
							PTE_INDISK | (pte & PTE_SYSCALL & ~PTE_P));
						close(fd);
						return 0;
					}
			}

		// 没地方了……
		close(fd);
		return -E_NO_DISK;
	}
	else
		return fd;
}

int
swap_back_page(void *va)
{
	int fd = open("/pagefile", O_RDWR), i;
	if (fd > 0)
	{
		struct Stat statbuf;
		pte_t pte = uvpt[PGNUM(va)];

		fstat(fd, &statbuf);
		if (statbuf.st_size != PAGEFILE_SIZE)
		{
			return -E_FAULT;
		}

		// 交换入磁盘
		seek(fd, 0);

		// 读出磁盘内页序号
		i = PTE_ADDR(pte) / PGSIZE;

		// 先读出空闲位图，看看是不是非空闲
		read(fd, buf, PGSIZE);
		if (!(buf[i / 8] & (1 << i % 8)))
		{
			close(fd);
			return -E_FAULT;
		}

		// 读出对应页写回内存！
		if (sys_page_alloc(0, va, (pte | PTE_P) & PTE_SYSCALL & ~PTE_INDISK) < 0)
		{
			close(fd);
			return -E_NO_MEM;
		}

		seek(fd, i * PGSIZE);
		read(fd, va, PGSIZE);

		// 好了
		close(fd);
		return 0;
	}
	else
		return fd;
}