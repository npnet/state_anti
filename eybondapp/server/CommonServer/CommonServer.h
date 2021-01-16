/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : CommonServer.c
  *@notes   : 2018.06.03 CGQ establish
*******************************************************************************/
#ifndef __COMMON_SERVER_H
#define __COMMON_SERVER_H

#ifdef _PLATFORM_M26_
#include "typedef.h"
#include "ql_type.h"
#include "SysPara.h"
#include "list.h"

#define COMMON_SERVER_TASK        subtask5_id
#define COMMON_SERVER__DATA_PROCESS        (0x010000)
#endif

#ifdef _PLATFORM_L610_
#include "eyblib_typedef.h"
#include "eyblib_list.h"
#include "eybpub_SysPara_File.h"
#define MSG_ID_USER_COMMONSERVER_START     0xA000
#define COMMON_SERVER__DATA_PROCESS        MSG_ID_USER_COMMONSERVER_START + 0xF10
#define COMMON_SERVER__EYBNET_READY        MSG_ID_USER_COMMONSERVER_START + 0xF11
#endif

#define SERVER_PROCESS_FREP                (1000)       //ms

typedef void (*DataAck)(Buffer_t *buf);
typedef ServerAddr_t *(*serverAddrGet)(void);
typedef void (*DataProcess)(ListHandler_t *cmdList);
typedef u8_t (*DataAdd)(Buffer_t *buf, DataAck ack);

typedef struct {
  char name[32] ;         // server name
  int waitTime;
  void_fun_void init;     // server Init
  void_fun_void deviceInit; // device Init;
  void_fun_u8 run;        // server run
  void_fun_void process;  // server data process
  DataAdd       add;      // server rcvedata Add
  serverAddrGet getAddr;  // Get server connect addr
  void_fun_void sclose;    // server close
  void_fun_void clean;    // server config or log clean
} CommonServer_t;

void proc_commonServer_task(s32_t taskId);
void CommonServerDataSend(Buffer_t *buf);
void CommonServer_DeviceInit(void);
void CommonServer_close(void);
void CommonServerData_clean(void);

#endif //__COMMON_SERVER_H
/******************************************************************************/

