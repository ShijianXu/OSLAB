#ifndef PCB_STRUCT_H
#define PCB_STRUCT_H

#include "./mmu.h"

#define KSTACK_SIZE 4096
#define NR_FP 	20
struct PCB { 
	struct TrapFrame *tf;
	uint8_t kstack[KSTACK_SIZE];
	
	int fcb[NR_FP];	//用户打开文件表项
	//可以存放20个fcb标示，该数组的项的索引值为返回的fp值

	int pid;                //Process id
	int ppid;               //Parent process id
	int state;              //Process state
	int time_count;         //the running times of Process 
	int sleep_time;         //Sleep count down
	uint32_t cr3;           //PDT base address
	int pcb_index;
	int block_sem_index;	//阻塞在那个信号量上,初始值为-1
};

#endif
