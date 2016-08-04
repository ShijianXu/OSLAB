#include "include/types.h"
#include "include/string.h"
#include "include/fcb.h"
#include "pcb_struct.h"

#define MAX_FCB 256
#define MAX_ACT_INODE	256
#define ELF_OFFSET_IN_DISK 100*(1<<10)
#define NR_FP	20
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

struct FCB fcb_table[MAX_FCB];	//系统打开文件表项
struct act_inode inode_table[MAX_ACT_INODE];	//活动inode表

int strcmp(const char *s1, const char *s2);
int printk(const char *fmt, ...);
void readseg(unsigned char *, int, int);
void writeseg(unsigned char *, int, int);
void writesect(void *src, int offset);
extern struct map bitmap;
extern struct dir direct;
extern struct PCB *current;

void fcb_init()
{
	int i;
	for(i = 0; i < MAX_FCB; i ++)
	{
		fcb_table[i].f_state = s_close;
		fcb_table[i].act_inode_index = 0;
		fcb_table[i].inode_bitoffset = 0;
		fcb_table[i].ch_offset = 0;
		fcb_table[i].buf_offset = 0;
	}
}

int fcb_alloc()
{
	int fcb_index = -1;
	for(int i= 0; i < MAX_FCB; i ++)
	{
		if(fcb_table[i].f_state == s_close)
		{
			fcb_index = i;
			break;
		}
	}
	return fcb_index;
}

void fcb_release()
{
	;
}

void inode_init()	//初始化活动inode表
{
	for(int i = 0; i < MAX_ACT_INODE; i ++)
	{
		inode_table[i].i_count = 0;
		for(int j = 0; j < NR_INODE_ENTRY; j ++)
			inode_table[i].data.data_block_offset[j] = -1;
	}
}

int inode_alloc()	//活动inode分配
{
	for(int i = 0; i < MAX_ACT_INODE; i ++)
	{
		if(inode_table[i].i_count == 0)
		{
			return i;
		}
	}
	return -1;		//没有可以分配的了
}

void open(char *filename, int mode)
{
	bool find = false;
	int i;
	for(i = 0; i < NR_DIR_ENTRY; i ++)
	{
		if(strcmp(filename, direct.entry[i].filename) == 0)
		{
			find = true;
			printk("I HAVE FOUND THE FILE\n");
			break;
		}
	}
	//检索到文件在目录中
	if(find)
	{
		//先分配活动inode,再拷贝到活动inode中
		//实际上同一个文件如果已经被打开过则不应该再复制inode
		//我没做
		int index = inode_alloc();
	printk("the active inode index is %d\n", index);
		int inode_offset = direct.entry[i].inode_offset;
		readseg((uint8_t *)&inode_table[index].data, 512, ELF_OFFSET_IN_DISK + inode_offset * 512);
//		for(int i = 0; i < NR_INODE_ENTRY; i ++)
//		{
//			if(inode_table[index].data.data_block_offset[i] != -1)
	printk("%d\n", inode_table[index].data.data_block_offset[0]);
//		}

		inode_table[index].i_count += 1;
		//这里跳过访问权限检查，不做这一步
		//为文件分配系统打开文件表项
		int fcb_index = fcb_alloc();
	printk("the fcb index is %d\n", fcb_index);
		fcb_table[fcb_index].f_state = mode;
		fcb_table[fcb_index].act_inode_index = index;
		fcb_table[fcb_index].inode_bitoffset = inode_offset;
		fcb_table[fcb_index].ch_offset = 0;
		//为文件分配用户打开文件表项，填写fcb_table索引值，返回fp
		int fd = 0;
		for(fd = 0; fd < NR_FP; fd++)
		{
			if(current->fcb[fd] == -1)
			{
				current->fcb[fd] = fcb_index;
				current->tf->eax = fd;
	printk("the fd is %d\n", fd);
				break;
			}
		}
	}
	else
	{
		printk("ERROR:No such file in derectory!\n");
		current->tf->eax = -1;	//返回值	
	}
}

void read(int fd, char *buf, int count)
{
	int fcb_index = current->fcb[fd];
	int inode_index = fcb_table[fcb_index].act_inode_index;
	int ch_offset = fcb_table[fcb_index].ch_offset;

	char temp_buf[513];
	memset(temp_buf, 0, sizeof(temp_buf));
	
	int nr = ch_offset / 512;
	int start = ch_offset % 512;
	int total = ch_offset + count;
	int num = total / 512;
	int dif = num - nr;

	if(dif == 0)
	{
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr] * 512);
		int len = count < sizeof(temp_buf) - start ? count : sizeof(temp_buf) - start;
		strncpy(buf, temp_buf + start, len);
		
		fcb_table[fcb_index].ch_offset = total;
printk("ch_offset %d\n", fcb_table[fcb_index].ch_offset);
		current->tf->eax = len;
	}
	else if(dif == 1)
	{
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr] * 512);
		strncpy(buf, temp_buf + start, 512 - start);

		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[num] * 512);
		strncpy(buf + 512-start, temp_buf, total - num * 512);
		
		fcb_table[fcb_index].ch_offset = total;
printk("ch_offset %d\n", fcb_table[fcb_index].ch_offset);
		current->tf->eax = 512-start + total - num*512;
	}
	else
	{
		//dif >= 2
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr] * 512);
		strncpy(buf, temp_buf + start, 512 - start);
		for(int i = nr + 1; i < num; i ++)
		{
			readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[i] * 512);
			strcpy(buf + 512-start + (i-nr-1)*512, temp_buf);
		}
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[num] * 512);
		strncpy(buf + 512-start + (dif-1) * 512  , temp_buf, total - num * 512);

		fcb_table[fcb_index].ch_offset = total;
printk("ch_offset %d\n", fcb_table[fcb_index].ch_offset);
		current->tf->eax = total - nr * 512 - start;
	}
}

void write(int fd, char *buf, int count)
{
	int fcb_index = current->fcb[fd];
	int inode_index = fcb_table[fcb_index].act_inode_index;
	int ch_offset = fcb_table[fcb_index].ch_offset;
	
	int nr = ch_offset / 512;
	int start = ch_offset % 512;	//这个碎片位于第nr个块
	int num = (ch_offset + count) / 512;
	int end = (ch_offset +count) % 512;

	char temp_buf[513];
	int rest = 512 - start;
	if(rest > count)
	{
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
		strncpy(temp_buf+start, buf, count);
printk("%s\n", temp_buf);
		writeseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
//		writesect(temp_buf, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
	}
	else if(count - rest < 512)
	{
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
		strncpy(temp_buf+start, buf, rest);
printk("%s\n", temp_buf);
		writeseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
		
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr + 1]*512);
		strncpy(temp_buf, buf+rest, count - rest);
printk("%s\n", temp_buf);
		writeseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr + 1]*512);
	}
	else
	{
		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
		strncpy(temp_buf+start, buf, rest);
printk("%s\n", temp_buf);
		writeseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[nr]*512);
	
		for(int i = nr+1; i < num; i ++)
		{
			readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[i]*512);
			strncpy(temp_buf, buf+rest + (i-nr-1)*512, 512);
printk("%s\n", temp_buf);
			writeseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[i]*512);
		}

		readseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[num]*512);
		strncpy(temp_buf, buf+rest+(num-nr-1)*512, end);
printk("%s\n", temp_buf);
		writeseg((uint8_t *)temp_buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[num]*512);
	}
}

void lseek(int fd, int offset, int whence)
{
	int fcb_index = current->fcb[fd];
	int inode_index = fcb_table[fcb_index].act_inode_index;
	int i = 0;
	switch(whence)
	{
		case SEEK_SET:
			fcb_table[fcb_index].ch_offset = offset;
printk("offset %d\n", fcb_table[fcb_index].ch_offset);
			break;
		case SEEK_CUR:
			fcb_table[fcb_index].ch_offset += offset;
printk("offset %d\n", fcb_table[fcb_index].ch_offset);
			break;
		case SEEK_END:
			fcb_table[fcb_index].ch_offset = 0;
			for(; i < NR_INODE_ENTRY; i ++)
			{
				if(inode_table[inode_index].data.data_block_offset[i] != -1)
					fcb_table[fcb_index].ch_offset += 512;
				else
					break;
			}
			fcb_table[fcb_index].ch_offset -= 512;
			i -= 1;
			char buf[514];
			readseg((uint8_t *)buf, 512, ELF_OFFSET_IN_DISK + inode_table[inode_index].data.data_block_offset[i] * 512);
			fcb_table[fcb_index].ch_offset += (strlen(buf) + offset);
printk("offset %d\n", fcb_table[fcb_index].ch_offset);
			break;
	}
}

void close(int fd)
{
	int fcb_index = current->fcb[fd];
	current->fcb[fd] = -1;
	int inode_index = fcb_table[fcb_index].act_inode_index;
//	int inode_offset = fcb_table[fcb_index].inode_bitoffset;
	fcb_table[fcb_index].f_state = s_close;
	fcb_table[fcb_index].act_inode_index = 0;
	fcb_table[fcb_index].inode_bitoffset = 0;
	fcb_table[fcb_index].ch_offset = 0;

	inode_table[inode_index].i_count = 0;
//	write_seg()
}
