/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : selfUpdate.c
 * @Author  : CGQ
 * @Date    : 2017-12-14
 * @Brief   : 
 ******************************************************************************/
#include "selfUpdate.h"
#include "Debug.h"
#include "memory.h"
#include "watchdog.h"
#include "Ql_FOTA.h"
#include "Ql_GPIO.h"
#include "Ql_ERROR.h"

extern ST_ExtWatchdogCfg* Ql_WTD_GetWDIPinCfg(void);


#define SELF_BUFFE_SIZE   (0x200)

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
int Update_Self(File_t *file)
{
	int ret;
	int len;
	ST_FotaConfig *ft = memory_apply(sizeof(ST_FotaConfig));
	u8 *buffer = memory_apply(SELF_BUFFE_SIZE);

	if (ft == null || buffer == null || file == null)
	{
		log_save("memory full!");
		goto ERR;
	}
	
	ft->Q_gpio_pin1 = Ql_WTD_GetWDIPinCfg()->pinWtd1;
	ft->Q_feed_interval1 = 100;
	ft->Q_gpio_pin2 = Ql_WTD_GetWDIPinCfg()->pinWtd2;
	ft->Q_feed_interval2 = 100;
	ft->reserved1 = 0;
	ft->reserved2 = 0;

	Watchdog_feed();
	ret = Ql_FOTA_Init(ft);
	if (ret != QL_RET_OK)
	{
		APP_DEBUG("FOTA init Err :%d\r\n", ret);
		goto ERR;
	}

	do {
		len = File_read(file, buffer, SELF_BUFFE_SIZE);

		if (len < 0)
		{
			goto ERR;
		}
		ret = Ql_FOTA_WriteData(len, (s8*)buffer);
		if (ret != QL_RET_OK)
		{
			APP_DEBUG("FOTA Write Err: %d \r\n", ret);
			goto ERR;
		}
		Watchdog_feed();
	} while (len == SELF_BUFFE_SIZE);

	ret = Ql_FOTA_Finish();
	ret = Ql_FOTA_Update();
	if (ret != QL_RET_OK)
	{
		APP_DEBUG("FOTA update Err :%d\r\n", ret);
	} 
	
ERR:
	memory_release(ft);
	memory_release(buffer);
	memory_release(file);
    
	return ret;	
}


/*********************************FILE END*************************************/

