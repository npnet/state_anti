/******************************************************************************           
* name:             anti_reflux.h           
* introduce:        头文件
* author:           Luee                                     
******************************************************************************/ 
#ifndef _ANTI_REFLUX_H_
#define _ANTI_REFLUX_H_

#include "grid_tool.h"
#include "anti_reflux.h"
#include "Modbus.h"
#include "ModbusDevice.h"
#include "Protocol.h"
#include "Device.h"

#define MSG_ID_USER_ANTI_REFLUX_START     0xC000
#define ANTI_REFLUX_DATA_PROCESS        MSG_ID_USER_ANTI_REFLUX_START + 0xF60
#define ANTI_REFLUX_TIMER_ID            MSG_ID_USER_ANTI_REFLUX_START + 0xF61

typedef struct
{
    u8_t state;
    s32_t power;
}MeterValue_t;

typedef struct
{
    ModbusGetCmdTab_t tab;
    void (*func)(u16_t deviceCount, Device_t *dev, MeterValue_t *meter);
}AntiReflux_t;

typedef struct
{
    u8_t addr;
    AntiReflux_t *aux;
}AntiRefluxDeviceInfo_t;

void proc_antiReflux_task(s32_t taskId);
u8_t modbusDataCheck(void *load, void *changeData);
int float2int(u32_t data);


void proc_anti_reflux_task(s32_t taskId);

#endif /* _ANTI_REFLUX_H_ */