/**********************    COPYRIGHT 2014-2100, QIYI    ************************ 
 * @File    : Key.c
 * @Author  : CGQ
 * @Date    : 2018-01-12
 * @Brief   : 
 ******************************************************************************/
#ifdef _PLATFORM_BC25_
#include "ql_system.h"
#endif

#include "eybpub_utility.h"
#include "eybpub_watchdog.h"
#include "eybpub_Status.h"
#include "eybpub_Debug.h"
#include "eybpub_SysPara_File.h"
#include "eybpub_run_log.h"

#include "eybpub_Key.h"

#ifdef _PLATFORM_BC25_
#ifndef KEY_PORT		//mike 20200805
#define KEY_PORT (PINNAME_UART2_CTS)
#endif
/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Key_init(void)
{
	s32_t ret = 0;
	ret = Ql_GPIO_Init(KEY_PORT, PINDIRECTION_IN, PINLEVEL_HIGH, PINPULLSEL_PULLUP);
//	APP_DEBUG("Key port GPIO Init ret:%d direction: %d\r\n", ret, Ql_GPIO_GetDirection(KEY_PORT));
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Key_scan(void)
{
	static int cnt = 0;

	if (Ql_GPIO_GetLevel(KEY_PORT) == 0)
	{
		if (++cnt > 5)
		{			
			APP_DEBUG("Reset key user.\r\n");
            log_save("Reset key user.");
			Beep_On(1);
//			SysPara_default();      //mike 20200824
            parametr_default();
			Watchdog_stop();
            Ql_Reset(0);
		}
	}
	else
	{
		cnt = 0;
	}
}
#endif

#ifdef _PLATFORM_L610_
/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Key_init(void)
{
	s32_t ret = 0;
}

/*******************************************************************************
 Brief    : void
 Parameter: 
 return   : 
*******************************************************************************/
void Key_scan(void)
{
	static int cnt = 0;
}
#endif
/******************************************************************************/
