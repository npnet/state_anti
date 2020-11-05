/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : UpdateTask.h
 * @Author  : CGQ
 * @Date    : 2017-12-14
 * @Brief   : 
 ******************************************************************************/
#ifndef __UPDATE_TASK_
#define __UPDATE_TASK_

#include "ql_type.h"

#define APP_TASK_ID		5

#define UPDATE_TASK			subtask4_id
#define SELF_UPDATE_ID      (0x00F600)
#define DEVICE_UPDATE_ID	(0x00F000)



void proc_update_task(s32 taskId);


#endif //__UPDATE_TASK_
/******************************************************************************/
