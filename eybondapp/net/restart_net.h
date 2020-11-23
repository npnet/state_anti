/******************************************************************************           
* name:             restart_net.h           
* introduce:        头文件
* author:           Luee                                     
******************************************************************************/ 
#ifndef _RESTART_NET_H_
#define _RESTART_NET_H_

#include "fibo_opencpu.h"
#include "eyblib_typedef.h"
#include "eybpub_Debug.h"
#include "swap_hl.h"

extern u32_t g_EventFlag;

void restart_net(void);
void get_simstatus(void);
void active_pdp(void);
void tcp_connection(void);
s32_t launch_tcp_connection(void);

#endif /* _RESTART_NET_H_ */

