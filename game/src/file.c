#include "include/syscall.h"
#include "include/string.h"
int printf(const char *fmt, ...);
int syscall(int id, ...);

int open(char *filename, int mode);
int read(int fd, char *buf, int count);
int write(int fd, char *buf, int count);
void lseek(int fd, int offset, int whence);
void close(int fd);

#define mod_r	0
#define mod_w	1
#define mod_rw	2
#define SEEK_SET    0
#define SEEK_CUR    1
#define SEEK_END    2

void file_main()
{
	int fd = open("in.txt", mod_r);
	if(fd == -1)
	{
		printf("file open failed\n");
		while(1);
	}
//	printf("haha\n");
//	char buf3[2048];
//	lseek(fd, 10, SEEK_SET);
//	read(fd, buf3, 10);
//	printf("the 10 bytes are %s\n", buf3);

	lseek(fd, 0, SEEK_END);
	char buf2[1024] = "XuShijian!";
	write(fd, buf2, strlen(buf2));

	lseek(fd, -11, SEEK_END);
	char buf[513];
	memset(buf, 0, sizeof(buf));
	int nr = read(fd, buf, 11);
	printf("nr = %d,  %s\n", nr, buf);

	close(fd);
	while(1);
}
