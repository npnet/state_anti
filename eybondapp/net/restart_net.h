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
#include "4G_net.h"

#define RELINK_ID_ST       0x060000    
#define RELINK_RESTART_ID   RELINK_ID_ST+0x11
#define RELINK_TIMER_ID     RELINK_ID_ST+0x12    



extern u32_t g_EventFlag;
extern u32_t EYBRELINK_TASK;

s32_t restart_net(void);
s32_t get_simstatus(void);
vs32_t active_pdp(void);
s32_t tcp_connection(void);
s32_t launch_tcp_connection(void);
void realtime_check_net(void);
void proc_relink_task (s32_t relink);

#endif /* _RESTART_NET_H_ */

