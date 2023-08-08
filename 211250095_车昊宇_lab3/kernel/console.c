
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			      console.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
						    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
	回车键: 把光标移到第一列
	换行键: 把光标前进到下一行
*/


#include "type.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"

PRIVATE void set_cursor(unsigned int position);
PRIVATE void set_video_start_addr(u32 addr);
PRIVATE void flush(CONSOLE* p_con);

/*======================================================================*
			   init_screen
 *======================================================================*/
PUBLIC void init_screen(TTY* p_tty)
{
	int nr_tty = p_tty - tty_table;
	p_tty->p_console = console_table + nr_tty;

	int v_mem_size = V_MEM_SIZE >> 1;	/* 显存总大小 (in WORD) */

	int con_v_mem_size                   = v_mem_size / NR_CONSOLES;
	p_tty->p_console->original_addr      = nr_tty * con_v_mem_size;
	p_tty->p_console->v_mem_limit        = con_v_mem_size;
	p_tty->p_console->current_start_addr = p_tty->p_console->original_addr;

	/* 默认光标位置在最开始处 */
	p_tty->p_console->cursor = p_tty->p_console->original_addr;

	if (nr_tty == 0) {
		/* 第一个控制台沿用原来的光标位置 */
		p_tty->p_console->cursor = disp_pos / 2;
		disp_pos = 0;
	}
	else {
		out_char(p_tty->p_console, nr_tty + '0');
		out_char(p_tty->p_console, '#');
	}

	set_cursor(p_tty->p_console->cursor);
}


/*======================================================================*
			   is_current_console
*======================================================================*/
PUBLIC int is_current_console(CONSOLE* p_con)
{
	return (p_con == &console_table[nr_current_console]);
}
PRIVATE void recordCursor(CONSOLE* p,u32 pos){
	int index = p->cursor_record.idx;//取出当前光标位置被记录在数组的下标
	p->cursor_record.idx++;
	p->cursor_record.content[index] = pos;
}
PRIVATE u32 popCursor(CONSOLE* p){
	p-> cursor_record.idx--;
	u32 char_ = p->cursor_record.content[p->cursor_record.idx];
	return char_;
}
PRIVATE void Search_(CONSOLE* p_con){
	int len = p_con->cursor-p_con->finalCursorBeforeEsc; 
	if(len == 0 || len > p_con->finalCursorBeforeEsc){
		return;
	}
	for(int i =  0; i < p_con->finalCursorBeforeEsc;i++){
		int flag = 1;
		for(int j = 0 ; j< len ;j++){
			if(*(u8*)(V_MEM_BASE + i * 2 + j * 2) !=*(u8*)(V_MEM_BASE + p_con->finalCursorBeforeEsc * 2 + j * 2)){
				flag = 0;//如果遇到不匹配
				break;
			}
			if(*(u8*)(V_MEM_BASE + i * 2 + j * 2) == ' '&&
				*(u8*)(V_MEM_BASE + i * 2 + j * 2 + 1)!=*(u8*)(V_MEM_BASE + p_con->finalCursorBeforeEsc * 2 + j * 2 + 1)){
				flag = 0;//如果遇到不匹配
				break;
			}
		}
		if(flag == 1){
            for(int k = 0; k < len; k++){
                if(*(u8*)(V_MEM_BASE + i * 2 + k * 2) != ' ')
                    *(u8*)(V_MEM_BASE + i * 2 + k * 2 + 1) = RED;
            }
        }
	}
}
PRIVATE void CtrlZ(CONSOLE* p_con){
	if(mode == 0){
		clean_screen();
		p_con->cursor_record.idx = 0;
		p_con->cursor = disp_pos / 2;
		flush(p_con);
		//p_con->char_record.idx -= 2; //减去z并前移一个位置
    	//if(p_con->char_record.idx < 0){ 
        	//p_con->char_record.idx = 0;
        	//return;
    	//}
    	//for(int i = 0; i < p_con->char_record.idx; i++){
        	//out_char(p_con, p_con->char_record.content[i]);
    	//}
	}
	return;
}
/*======================================================================*
			   out_char
 *======================================================================*/
PUBLIC void out_char(CONSOLE* p_con, char ch)
{
	u8* p_vmem = (u8*)(V_MEM_BASE + p_con->cursor * 2);
	if(mode == 2&&ch != '\r'){
		return;
	}
	switch(ch) {
	case '\n':
		if(mode == 0){
			if (p_con->cursor < p_con->original_addr +
		    p_con->v_mem_limit - SCREEN_WIDTH) {
			recordCursor(p_con,p_con->cursor);
            p_con->cursor = p_con->original_addr + SCREEN_WIDTH * ((p_con->cursor - p_con->original_addr)/SCREEN_WIDTH+1);
			}
		}else{
			mode = 2;
			Search_(p_con);
		}
		break;
	case '\b':
		if(mode == 1){
			if(p_con->cursor_record.content[p_con->cursor_record.idx-1]<p_con->finalCursorBeforeEsc){
				break;
			}
		}
		p_con-> cursor_record.idx--;
		u32 pos = p_con->cursor_record.content[p_con->cursor_record.idx];
		if (p_con->cursor > p_con->original_addr && p_con->cursor_record.idx != 0) {
			int i = 0;
            while (p_con->cursor > pos){
                p_con->cursor--;
                *(p_vmem - 2 - 2 * i) = ' ';
                *(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
                i++;
            }
		}
		break;
	case '\t':
        if(p_con->cursor < p_con->original_addr + p_con->v_mem_limit - 4){//当前控制台的光标位置<当前控制台对应显存位置+当前控制台所占显存大小-tab长度
            recordCursor(p_con,p_con->cursor);
            for(int i = 0; i < 4; i++){
                *p_vmem++ = ' ';
                *p_vmem++ = 0x1;
                p_con->cursor++;
            }
        }
        break;
	case '\r':
		if(mode == 0){
			//p_con->char_record.esc_index = p_con->char_record.idx;
            p_con->finalCursorBeforeEsc = p_con->cursor;
			mode = 1;
		}else if(mode == 1){
			return;	
		}else{
			if (p_con->cursor > p_con->original_addr){
                int i = 0;
                while(p_con->cursor > p_con->finalCursorBeforeEsc){
                    u32 idx = popCursor(p_con); 
                    while(p_con->cursor > idx){
                        p_con->cursor--;
                        *(p_vmem - 2 - 2 * i) = ' ';
                        *(p_vmem - 1 - 2 * i) = DEFAULT_CHAR_COLOR;
                        i++;
                    }
                }
            }
            for(int i = 0; i < p_con->cursor; i++){
                if (*(u8*)(V_MEM_BASE + i * 2 + 1) == RED){
                    *(u8*)(V_MEM_BASE + i * 2 + 1) = DEFAULT_CHAR_COLOR;
                }
            }
			mode = 0;
		}
		break;
	//case 'Z':
    //case 'z':
        //if(ctrl){
            //CtrlZ(p_con);
          //  break;
        //}
	default:
		if (p_con->cursor <
		    p_con->original_addr + p_con->v_mem_limit - 1) {
			recordCursor(p_con, p_con->cursor);
			if(mode == 0 && p_con->cursor > p_con -> finalCursorBeforeEsc){
				p_con->finalCursorBeforeEsc = p_con->cursor;
			}
			*p_vmem++ = ch;
			if(mode == 0){
				*p_vmem++ = DEFAULT_CHAR_COLOR;
			}else if(mode ==1 ){
				if(ch!=' '){
					*p_vmem++ = RED;
				}else {
					*p_vmem++ = DEFAULT_CHAR_COLOR;
				}
				
			}
			p_con->cursor++;
		}
		break;
	}

	while (p_con->cursor >= p_con->current_start_addr + SCREEN_SIZE) {
		scroll_screen(p_con, SCR_DN);
	}

	flush(p_con);
}

/*======================================================================*
                           flush
*======================================================================*/
PRIVATE void flush(CONSOLE* p_con)
{
	if (is_current_console(p_con)) {
		set_cursor(p_con->cursor);
		set_video_start_addr(p_con->current_start_addr);
	}
}

/*======================================================================*
			    set_cursor
 *======================================================================*/
PRIVATE void set_cursor(unsigned int position)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, CURSOR_H);
	out_byte(CRTC_DATA_REG, (position >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, CURSOR_L);
	out_byte(CRTC_DATA_REG, position & 0xFF);
	enable_int();
}

/*======================================================================*
			  set_video_start_addr
 *======================================================================*/
PRIVATE void set_video_start_addr(u32 addr)
{
	disable_int();
	out_byte(CRTC_ADDR_REG, START_ADDR_H);
	out_byte(CRTC_DATA_REG, (addr >> 8) & 0xFF);
	out_byte(CRTC_ADDR_REG, START_ADDR_L);
	out_byte(CRTC_DATA_REG, addr & 0xFF);
	enable_int();
}



/*======================================================================*
			   select_console
 *======================================================================*/
PUBLIC void select_console(int nr_console)	/* 0 ~ (NR_CONSOLES - 1) */
{
	if ((nr_console < 0) || (nr_console >= NR_CONSOLES)) {
		return;
	}

	nr_current_console = nr_console;

	flush(&console_table[nr_console]);
}

/*======================================================================*
			   scroll_screen
 *----------------------------------------------------------------------*
 滚屏.
 *----------------------------------------------------------------------*
 direction:
	SCR_UP	: 向上滚屏
	SCR_DN	: 向下滚屏
	其它	: 不做处理
 *======================================================================*/
PUBLIC void scroll_screen(CONSOLE* p_con, int direction)
{
	if (direction == SCR_UP) {
		if (p_con->current_start_addr > p_con->original_addr) {
			p_con->current_start_addr -= SCREEN_WIDTH;
		}
	}
	else if (direction == SCR_DN) {
		if (p_con->current_start_addr + SCREEN_SIZE <
		    p_con->original_addr + p_con->v_mem_limit) {
			p_con->current_start_addr += SCREEN_WIDTH;
		}
	}
	else{
	}

	flush(p_con);
}

