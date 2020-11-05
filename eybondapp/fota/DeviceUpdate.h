/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : DeviceUpdate.h
 * @Author  : CGQ
 * @Date    : 2017-12-14
 * @Brief   : 
 ******************************************************************************/
#ifndef __DEVICE_UPDATE_
#define __DEVICE_UPDATE_

#include "ql_type.h"
#include "File.h"

#define DEVICE_UPDATE_READY_ID 		(0x0F0000)
#define DEVICE_UPDATE_END_ID		(0x0F1000)

typedef struct 
{
	u8 addr;
	u8 type;
	u16 bandrate;
}DeviceType_t;


int  Update_startDevice(File_t *file, DeviceType_t *type);
void Update_ready(void);
u8   Update_state(void);
void Update_end(void);



#endif //__DEVICE_UPDATE_
/******************************************************************************/
