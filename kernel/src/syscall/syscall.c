#include "include/mmu.h"
#include "include/irq.h"
#include "include/video.h"
#include "include/x86.h"
#include "include/pcb.h"
#include "include/fcb.h"
#include "pcb_struct.h"
#include "include/semaphore.h"

//#include "include/types.h"

void serial_printc(char ch);
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *v, int c, size_t n);

#define KEYCODE 0
#define VMEMORY 1
#define SERIAL	2
#define FORK 	3
#define SLEEP 	4
#define EXIT 	5
#define THREAD	6
#define SEM_OPEN	7
#define SEM_WAIT	8
#define SEM_POST	9
#define SEM_CLOSE	10
#define F_OPEN      11
#define F_READ      12
#define F_WRITE     13
#define F_LSEEK     14
#define F_CLOSE     15


extern uint32_t code;
void do_syscall(struct TrapFrame *tf) {
	switch(tf->eax) {
		case KEYCODE:
			tf->eax = code;
			code = 0;
			break;
		case VMEMORY: //绘制显存
			memcpy(VMEM_ADDR, (void *)tf->ebx, SCR_SIZE);
			break;
		case SERIAL: //串口输出
			serial_printc((char)tf->ebx);
			break;
		case FORK:	//fork			
	//		tf->eax = fork();	
			fork();
			break;
		case SLEEP: //sleep
			sleep(tf->ebx);
			break;
		case EXIT:
			Exit();
			break;
		case THREAD:
			create_thread((uint32_t *)tf->ebx);
			break;
		case SEM_OPEN:
			sem_open((sem_t *)tf->ebx, (int)tf->ecx, (bool)tf->edx);
			break;
		case SEM_WAIT:
			sem_wait((sem_t *)tf->ebx);
			break;
		case SEM_POST:
			sem_post((sem_t *)tf->ebx);
			break;
		case SEM_CLOSE:
			sem_close((sem_t *)tf->ebx);
			break;

		case F_OPEN:
			open((char *)tf->ebx, tf->ecx);
			break;
		case F_READ:
			read(tf->ebx, (char *)tf->ecx, tf->edx);
			break;
		case F_WRITE:
			write(tf->ebx, (char *)tf->ecx, tf->edx);
			break;
		case F_LSEEK:
			lseek(tf->ebx, tf->ecx, tf->edx);
			break;
		case F_CLOSE:
			close(tf->ebx);
			break;

		default: printk("Unhandled system call: id = %d", tf->eax);
	}
}
