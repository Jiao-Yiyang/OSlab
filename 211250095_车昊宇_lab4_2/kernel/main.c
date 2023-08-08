
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


//new
int proStats[6] = {0, 0, 0, 0, 0, 0};
int prod[5];
int suc1;
int suc2;

//new

PRIVATE void init_tasks()
{
	init_screen(tty_table);
	clean(console_table);

	for(int i = 0; i < 6;i++){
		proStats[i] = 0;
	}
	// 表驱动，对应进程0, 1, 2, 3, 4, 5, 6
	int prior[7] = {1, 1, 1, 1, 1, 1, 1};
	for (int i = 0; i < 7; ++i) {
        proc_table[i].ticks    = prior[i];
        proc_table[i].priority = prior[i];
	}

	//new
	k_reenter = 0;
	ticks = 0;
	good1 = 0;
	good2 = 0;
	min_mutex1.value--;
	min_mutex2.value--;
	for(int i = 0 ; i < 5; i++){
		prod[i] = 0;
	}
	p_proc_ready = proc_table;
}
/*======================================================================*
                            kernel_main
 *======================================================================*/
PUBLIC int kernel_main()
{
	TASK*		p_task		= task_table;
	PROCESS*	p_proc		= proc_table;
	char*		p_task_stack	= task_stack + STACK_SIZE_TOTAL;
	u16		selector_ldt	= SELECTOR_LDT_FIRST;
	int i;
    u8              privilege;
    u8              rpl;
    int             eflags;
	for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
        if (i < NR_TASKS) {     /* 任务 */
                        p_task    = task_table + i;
                        privilege = PRIVILEGE_TASK;
                        rpl       = RPL_TASK;
                        eflags    = 0x1202; /* IF=1, IOPL=1, bit 2 is always 1 */
                }
                else {                  /* 用户进程 */
                        p_task    = user_proc_table + (i - NR_TASKS);
                        privilege = PRIVILEGE_USER;
                        rpl       = RPL_USER;
                        eflags    = 0x202; /* IF=1, bit 2 is always 1 */
                }
                
		strcpy(p_proc->p_name, p_task->name);	// name of the process
		p_proc->pid = i;			// pid
		//new
		p_proc->sleeping = 0;
		p_proc->blocked = 0;
		p_proc->status = 0;
		p_proc->ldt_sel = selector_ldt;

		memcpy(&p_proc->ldts[0], &gdt[SELECTOR_KERNEL_CS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[0].attr1 = DA_C | privilege << 5;
		memcpy(&p_proc->ldts[1], &gdt[SELECTOR_KERNEL_DS >> 3],
		       sizeof(DESCRIPTOR));
		p_proc->ldts[1].attr1 = DA_DRW | privilege << 5;
		p_proc->regs.cs	= (0 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ds	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.es	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.fs	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.ss	= (8 & SA_RPL_MASK & SA_TI_MASK) | SA_TIL | rpl;
		p_proc->regs.gs	= (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;

		p_proc->regs.eip = (u32)p_task->initial_eip;
		p_proc->regs.esp = (u32)p_task_stack;
		p_proc->regs.eflags = eflags;

		p_proc->nr_tty = 0;

		p_task_stack -= p_task->stacksize;
		p_proc++;
		p_task++;
		selector_ldt += 1 << 3;
	}

	init_tasks();

	init_clock();
    init_keyboard();

	restart();

	while(1){}
}



//new
void produce_proc(int slices,int No,int type){
	type == 1? good1++ : good2++;
	prod[No]++;
	sleep_ms(slices * TIME_SLICE); 
}

void consume_proc(int slices,int No,int type){
	type == 1? good1-- : good2--;
	prod[No]++;
	sleep_ms(slices * TIME_SLICE); 
}

void  produce(int slices,int No,int type){
	P(&max_mutex);
	//P(&cons_mutex);//添加了生产者和消费者互斥
	P(&prod_mutex);
	produce_proc(slices,No,type);
	V(&prod_mutex);
	//V(&cons_mutex);
	(type == 1)? V(&min_mutex1): V(&min_mutex2);
}	


void consume(int slices,int No,int type){
	(type == 2)? P(&min_mutex2): P(&min_mutex1);
	P(&cons_mutex);
	consume_proc(slices,No,type);
	V(&cons_mutex);
	V(&max_mutex);
}
/*======================================================================*
                               ReporterA
 *======================================================================*/
void ReporterA()
{
	sleep_ms(TIME_SLICE);
    int time = 0;
    while (time <= 20) {
		char index[2] = {time/10+'0', time%10+'0'};
		time++;
		if(index[0]!='0'){
			printf("\06%c", index[0]);
		}
		printf("\06%c ", index[1]);
		for (int i = NR_TASKS + 1; i < NR_PROCS + NR_TASKS; i++)
		{
			char temp[2] = {prod[i-2]/10+'0', prod[i-2]%10+'0'};
			if(temp[0]!='0'){
			printf("\06%c", temp[0]);
			}
			printf("\06%c ",temp[1]);
		}
		//printf("\04%c \04%c",good1+'0',good2+'0');
		printf("\n");
			sleep_ms(TIME_SLICE);
    }
	while(1){

	}
}
/*======================================================================*
                               
 *======================================================================*/
void ProducerB()
{
	while(1){
		produce(1,0,1);
		sleep_ms(B_SLEEP_TIME * TIME_SLICE);
	}

}

/*======================================================================*
                              
 *======================================================================*/
void ProducerC()
{
	while(1){
		produce(1,1,2);
		sleep_ms(C_SLEEP_TIME * TIME_SLICE);
	}
}

/*======================================================================*
                               
 *======================================================================*/
void  ConsumerD()
{
	while(1){
		consume(1,2,1);
		sleep_ms(D_SLEEP_TIME * TIME_SLICE);
	}

}

/*======================================================================*
                            
 *======================================================================*/
void ConsumerE()
{
	
	while(1){
		consume(1,3,2);
		sleep_ms(E_SLEEP_TIME * TIME_SLICE);
	}
}

/*======================================================================*
                               
 *======================================================================*/
void ConsumerF()
{
	while(1){
		consume(1,4,2);
		sleep_ms(F_SLEEP_TIME * TIME_SLICE);
	}
	
}


