/**************Copyright(C)锟斤拷2015-2026锟斤拷Shenzhen Eybond Co., Ltd.***************
  *@brief   : _040B.c 
  *@notes   : 2018.11.16 CGQ 
*******************************************************************************/
//#include "antiReflux.h"
//#include "Debug.h"
//#include "device.h"
//#include "modbus.h"
//#include "r_stdlib.h"
//#include "Swap.h"
//#include "Protocol.h"
//#include "log.h"

#include "fibo_opencpu.h"
#include "eyblib_memory.h"
#include "eyblib_r_stdlib.h"
#include "eyblib_swap.h"

#include "eybpub_utility.h"
#include "eybpub_Debug.h"
#include "eybpub_run_log.h"
#include "eybpub_SysPara_File.h"
#include "eybpub_data_collector_parameter_table.h"

#include "grid_tool.h"
#include "anti_reflux.h"
#include "Modbus.h"
#include "ModbusDevice.h"
#include "Protocol.h"

static void  meter_040BGet(u16_t deviceCount, Device_t *dev, MeterValue_t *meter);

static const ModbusGetCmd_t _040B[] = {
    {0x03, 0x1920, 0x1921},
};

const AntiReflux_t Device_040B = {
    PROTOCOL_TABER(_040B, 0, 0x040B), meter_040BGet
};


static void  meter_040BGet(u16_t deviceCount, Device_t *dev, MeterValue_t *meter)
{
    DeviceCmd_t *cmd = list_nextData(&dev->cmdList, null);

    meter->state = 1;
    meter->power = float2int((cmd->ack.payload[3]<<24) | (cmd->ack.payload[4]<<16) |(cmd->ack.payload[5]<<8) | (cmd->ack.payload[6]));
    
    APP_DEBUG("Meter power  %ld w\r\n",  meter->power);
}

/******************************************************************************/

