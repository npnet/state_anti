#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "uart_operate.h"
#include "elog.h"
#include "common_data.h"
#include "watch_dog.h"
#include "gpio_operate.h"


#include "fibo_opencpu.h"
//#include "eyblib_typedef.h"
//#include "eybapp_appTask.h"
//#include "L610_conn_ali_net.h"
//#include "ginlong_monitor.pb-c.h"
//#include "eyblib_list.h"
//#include "eybond.h"
//#include "uart_operate.h"
//#include "ali_data_packet.h"
#include "protobuf-c.h"
#include "eybpub_debug.h"

typedef enum
{
	//eybapp_appTask
	NET_MSG_RIL_READY,
	NET_MSG_RIL_FAIL,
	NET_MSG_SIM_READY,
	NET_MSG_SIM_FAIL,
	NET_MSG_GSM_READY,
	NET_MSG_GSM_FAIL,
	NET_MSG_NET_READY,
	NET_MSG_NET_FAIL,
	APP_CMD_BEEP_ID,
	DEVICE_IO_ID,
	DEBUG_MSG_ID,
	APP_MSG_DEVTIMER_ID,
	APP_MSG_WDG_ID,
	APP_MSG_TIMER_ID,
	//
	
	//eybond.c proc_eybond_task
	APP_MSG_UART_READY,
	SYS_PARA_CHANGE,	
	EYBOND_DATA_PROCESS,
	EYBOND_CMD_REPORT,
	//
} MSG_ID;


typedef struct
{
	u32  message;
} ST_MSG;



#define MAX_MSG_COUNT 5   //队列中可存储的消息最大个数
#define MSG_SIZE (sizeof(ST_MSG))  //存的消息的大小
UINT32 app_task_msg_handle ; //队列消息句柄
UINT32 eybond_task_msg_handle ;
#endif