/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : device.h
  *@notes   : 2017.04.20 CGQ
*******************************************************************************/
#ifndef __DEVICE_H_
#define __DEVICE_H_

#ifdef _PLATFORM_L610_
#include "oc_uart.h"
#endif

#include "eyblib_typedef.h"
#include "eyblib_list.h"
#include "DeviceIO.h"

#define DEVICE_ACK_SIZE     (0x400)     // device default data ack data rcveice buffer size
#define DEVICE_CMD_ID       (0x0F8000)  //
#define DEVICE_RESTART_ID   (0x0F8010)
#define DEVICE_PV_SCAN_ID   (0x0F80F0)
#define DEVICE_PV_GET_ID    (DEVICE_PV_SCAN_ID + 1)

#ifdef _PLATFORM_BC25_
#define EYBDEVICE_TASK      subtask2_id
#endif

typedef void (*DeviceCallBack)(ListHandler_t *cmdList);

typedef enum {
  DEVICE_MOBUS = 0x5433,
  DEVICE_TRANS = 0x5400,
  DEVICE_GRID = 0x5430,
  DEVICE_ARTI = 0x5432,
} DeviceType_e;

typedef struct Device {
  u8_t lock;        //device lock
  u16_t type;       //device type
  void *explain;    //
#ifdef _PLATFORM_BC25_
  ST_UARTDCB *cfg;  //device config msg
#endif
#ifdef _PLATFORM_L610_
  hal_uart_config_t *cfg;
#endif  
  u8_t (*callBack)(struct Device *dev);  //command end call back function
  ListHandler_t cmdList;  //device command tab
} Device_t; //

typedef struct DeviceCmd {
  u8_t  state;
  u16_t waitTime;
  Buffer_t ack;     //
  Buffer_t cmd;     //
} DeviceCmd_t;      //

extern ListHandler_t DeviceList;

void Device_add(Device_t *device);
void Device_inset(Device_t *device);
void Device_remove(Device_t *device);
void Device_clear(void);
void DeviceCmd_clear(Device_t *dev);
void Device_removeType(u16_t type);

#endif  // __DEVICE_H_
/******************************************************************************/

