/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : selfUpdate.c
 * @Author  : CGQ
 * @Date    : 2017-12-14
 * @Brief   : 
 ******************************************************************************/
#ifdef _PLATFORM_M26_
#include "Ql_FOTA.h"
#include "Ql_GPIO.h"
#include "Ql_ERROR.h"
#endif

#ifdef _PLATFORM_L610_
#include "fibo_opencpu.h"
#endif

#include "SelfUpdate.h"
#include "eybpub_Debug.h"
#include "eyblib_memory.h"
#include "eybpub_watchdog.h"

#define SELF_BUFFE_SIZE   (0x200)

#ifdef _PLATFORM_M26_
extern ST_ExtWatchdogCfg* Ql_WTD_GetWDIPinCfg(void);

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
#endif

#ifdef _PLATFORM_L610_
/*******************************************************************************
 Brief    : void
 Parameter:
 return   :
*******************************************************************************/
int Update_Self(File_t *file){
  s16_t ret = -1;
  s16_t len = 0;
  APP_DEBUG("Update_Self\r\n");
  s32_t nfile_size = fibo_file_getSize(file->name);

  if (nfile_size <= 0) {   // 文件存在或长度为0
    APP_DEBUG("File %s is not existing\r\n", file->name);
    return ret;
  }

  APP_DEBUG("File %s size get: %ld file->size: %ld\r\n", file->name, nfile_size, file->size);

  u8_t *buffer = memory_apply(SELF_BUFFE_SIZE);
  if (buffer == NULL) {
    APP_DEBUG("memory_apply fail\r\n");
    return ret;
  }
  u8_t *data = memory_apply(file->size);
  u8_t *pdata = data;
  if (data == NULL) {
    memory_release(buffer);
    APP_DEBUG("memory_apply fail\r\n");
    return ret;
  }
  r_memset(data, 0, file->size);
  s32_t nfile_read = 0;

  do {
    r_memset(buffer, 0, SELF_BUFFE_SIZE);
    len = File_read(file, buffer, SELF_BUFFE_SIZE);
//    APP_DEBUG("Read %s file len %d data\r\n", file->name, len);
    if (len <= 0) {
      break;
    }
    nfile_read = nfile_read + len;
    r_memcpy(pdata, buffer, len);
    pdata = pdata + len;
  } while (len == SELF_BUFFE_SIZE);
  memory_release(buffer);
  APP_DEBUG("Get %s file data len %ld\r\n", file->name, nfile_read);
  if (nfile_read == file->size) {
    if(0 == fibo_app_check((INT8*)data, file->size)) {
      ret = fibo_ota_handle((INT8*)data, file->size);
      APP_DEBUG("app ota_ret is %d\r\n", ret);
    } else {
      ret = fibo_firmware_handle((INT8*)data, file->size);
      APP_DEBUG("sdk ota_ret is %d\r\n", ret);
    }
  }
  if (file != NULL) {   // 升级失败把传入的指针释放掉
    memory_release(file);
  }
  memory_release(data);
  return ret;
}
#endif
/*********************************FILE END*************************************/

