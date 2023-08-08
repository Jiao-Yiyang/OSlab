
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
char colors[3] = {'\01', '\03', '\02'};
char signs[3] = {'X', 'Z', 'O'};

//new
int mode_ = 2;//切换策略 3:解决饿死问题 1：读者优先 2：写者优先

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

	// initialization
	k_reenter = 0;
	ticks = 0;
	readers = 0;
	writers = 0;

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

PRIVATE read_proc(char proc, int slices, char color){
	p_proc_ready->status = 2;
	sleep_ms(slices * TIME_SLICE); // read time slice
}

PRIVATE	write_proc(char proc, int slices, char color){
	p_proc_ready->status = 2;
	sleep_ms(slices * TIME_SLICE); // write time slice
}

//new
void read_fair(char proc, int slices, char color){
	P(&S);
    P(&reader_Limit);
	P(&r_mutex);
	if (readers==0)
		P(&rw_mutex); // 有读者，禁止写
	
	V(&S);
	readers++;
	V(&r_mutex);
	
	read_proc(proc, slices, color);
	P(&r_mutex);
	readers--;
	if (readers==0)
		V(&rw_mutex); // 没有读者，可以开始写了
	V(&r_mutex);
   	V(&reader_Limit);
}

void write_fair(char proc, int slices, char color){
	P(&S);
	P(&rw_mutex);
	V(&S);
	// write
	write_proc(proc, slices, color);
	V(&rw_mutex);
}

// 读者优先
void read_rf(char proc, int slices, char color){
    P(&r_mutex);
    if (readers==0)
        P(&rw_mutex);
    readers++;
    V(&r_mutex);

	P(&reader_Limit);
    read_proc(proc, slices, color);
    V(&reader_Limit);

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);

}

void write_rf(char proc, int slices, char color){
    P(&rw_mutex);
    // 写过程
    write_proc(proc, slices, color);
    V(&rw_mutex);
}

// 写者优先
void read_wf(char proc, int slices, char color){
    P(&reader_Limit);
   
    P(&S);
    P(&r_mutex);
    if (readers==0)
        P(&rw_mutex);
    readers++;
    V(&r_mutex);
    V(&S);

    read_proc(proc, slices, color);

    P(&r_mutex);
    readers--;
    if (readers==0)
        V(&rw_mutex); // 没有读者，可以开始写了
    V(&r_mutex);

    V(&reader_Limit);
}

void write_wf(char proc, int slices, char color){
    P(&w_mutex);
    if (writers==0)
        P(&S);
    writers++;
    V(&w_mutex);

    P(&rw_mutex);
    write_proc(proc, slices, color);
    V(&rw_mutex);

    P(&w_mutex);
    writers--;
    if (writers==0)
        V(&S);
    V(&w_mutex);
}


/*======================================================================*
                               ReporterA
 *======================================================================*/
void ReporterA()
{
    sleep_ms(TIME_SLICE);
    char color = '\06';
    int time = 0;
    while (time <=20) {
		char index[2] = {time/10+'0', time%10+'0'};
		time++;
		if(index[0]!='0'){
			printf("\06%c", index[0]);
		}
		printf("\06%c ", index[1]);
		for (int i = NR_TASKS + 1; i < NR_PROCS + NR_TASKS; i++)
		{
			int proc_status = (proc_table+i)->status;
			printf("%c%c ", colors[proc_status], signs[proc_status]);
			
		}
		printf("\n");
			sleep_ms(TIME_SLICE);
    }
	while(1){

	}
}
/*======================================================================*
                               ReaderB
 *======================================================================*/
void ReaderB()
{
	while(1){
		if(mode_==3){
			read_fair('B', 2, colors[proStats[1]]);
		}else if(mode_ == 1){
			read_rf('B', 2, colors[proStats[1]]);
		}else{
			read_wf('B', 2, colors[proStats[1]]);
		}
		p_proc_ready->status = 1;
		sleep_ms(B_SLEEP_TIME * TIME_SLICE);
		p_proc_ready->status = 0;
	}

}

/*======================================================================*
                               ReaderC
 *======================================================================*/
void ReaderC()
{
	while(1){
		if(mode_==3){
			read_fair('C', 3, colors[proStats[2]]);
		}else if(mode_ == 1){
			read_rf('C', 3, colors[proStats[2]]);
		}else{
			read_wf('C', 3, colors[proStats[2]]);
		}
		p_proc_ready->status = 1;
		sleep_ms(C_SLEEP_TIME * TIME_SLICE);
		p_proc_ready->status = 0;
	}
}

/*======================================================================*
                               ReaderD
 *======================================================================*/
void ReaderD()
{
	while(1){
		if(mode_==3){
			read_fair('D', 3, colors[proStats[3]]);
		}else if(mode_ == 1){
			read_rf('D', 3, colors[proStats[3]]);
		}else{
			read_wf('D', 3, colors[proStats[3]]);
		}
		p_proc_ready->status = 1;
		sleep_ms(D_SLEEP_TIME * TIME_SLICE);
		p_proc_ready->status = 0;
	}

}

/*======================================================================*
                               WriterE
 *======================================================================*/
void WriterE()
{
	
	while(1){
	if(mode_==3){
		write_fair('E', 3, colors[proStats[4]]);
	}else if(mode_ == 1){
		write_rf('E', 3, colors[proStats[4]]);
	}else{
		write_wf('E', 3, colors[proStats[4]]);
	}
		p_proc_ready->status = 1;
		sleep_ms(E_SLEEP_TIME * TIME_SLICE);
		p_proc_ready->status = 0;
	}
}

/*======================================================================*
                               WriterF
 *======================================================================*/
void WriterF()
{
	while(1){
	if(mode_==3){
		write_fair('F', 4, colors[proStats[5]]);
	}else if(mode_ == 1){
		write_rf('F', 4, colors[proStats[5]]);
	}else{
		write_wf('F', 4, colors[proStats[5]]);
	}
		p_proc_ready->status = 1;
		sleep_ms(F_SLEEP_TIME * TIME_SLICE);
		p_proc_ready->status = 0;	
	}
	
}


