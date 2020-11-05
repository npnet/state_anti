/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : UpdateTask.c
 * @Author  : CGQ
 * @Date    : 2017-12-14
 * @Brief   : 
 ******************************************************************************/
#include "UpdateTask.h"
#include "Debug.h"
#include "ql_system.h"
#include "selfUpdate.h"
#include "typedef.h"
#include "deviceUpdate.h"
#include "log.h"
#include "Device.h"


/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void proc_update_task(s32 taskId)
{
	int ret;
	ST_MSG msg;
	
	APP_DEBUG("Update task run!!\r\n");
	

	while (TRUE)
	{
		Ql_OS_GetMessage(&msg);

		switch(msg.message)
		{
			case SELF_UPDATE_ID:
				Ql_Sleep(3000);
				log_save("App satrt update");
				ret = Update_Self((File_t*)msg.param1);
				if (ret != 0)
				{
					log_saveAbnormal("M26 app update fail: ", ret);
				}
				break;
			case DEVICE_UPDATE_ID:
				ret = Update_startDevice((File_t*)msg.param1, (DeviceType_t*)msg.param2);
				if (ret != 0)
				{
					log_saveAbnormal("Device update fail: ", ret);
					break;
				}
			case DEVICE_UPDATE_READY_ID:
				log_save("Device update ready");
			    Ql_Sleep(2000);
				Ql_OS_SendMessage(DEVICE_TASK, DEVICE_UPDATE_READY_ID, 0, 0);
				break;
			case DEVICE_UPDATE_END_ID:
				Update_end();
				break;//*/
			default:
				break;
		}
	}
}

/*********************************FILE END*************************************/

