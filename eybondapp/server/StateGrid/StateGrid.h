/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : StateGrid.h
  *@notes   : 2018.10.23 CGQ establish
              2019.06.27 CGQ 增加一个采集多电站需求
*******************************************************************************/
#ifndef __STATE_GRID_H
#define __STATE_GRID_H

#include "CommonServer.h"

#define STATE_GRID_ACK_OVERTIME        (30000)
#define STATE_GRID_CNT            (SERVER_PROCESS_FREP/STATE_GRID_ACK_OVERTIME)

#define STATE_GRID_CMD_SIZE       (450)

typedef enum {
  FS_START = 0,
  FS_END  = 1,
  FS_MID = 2,
  FS_ONCE = 3,
} FS_e;

typedef struct {
  u8_t sn  : 4; //message serial number
  u8_t con : 1; //confirm flag, 0: no, 1:yes
  u8_t fs  : 2; //message flag,
  u8_t dir : 1; //message
} Control_t;

typedef struct {
  s32_t lenght;    //message data lenght;
  u8_t  nameLen;   //device name lenght
  char *name;      //device name
  Control_t ctr;   //message control byte
  u8_t func;       //message funcation code
  Buffer_t pdu;
} StateGridCmd_t; //State Grid server data protocol

typedef struct {
  u16_t waitCnt;
  DataAck ack;
  StateGridCmd_t *cmd;
} StateGrid_t;

typedef u8_t (*StateGridFunc)(StateGrid_t *sg);
typedef struct {
  u16_t code;
  StateGridFunc func;
} StateGridTab_t;

extern const CommonServer_t  StateGrid_API;
int StateGrid_check(void);
u16_t StateGrid_station(u8_t addr, u32_t *No);
#endif //__STATE_GRID_H
/******************************************************************************/

