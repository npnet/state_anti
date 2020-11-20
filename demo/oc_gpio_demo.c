/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "oc_adc.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


void oc_gpioisrcallback(void *param)
{
	hal_gpioisr_t *isr_cfg = (hal_gpioisr_t*)param;
	fibo_gpio_get(24,&(isr_cfg->level));
	OSI_LOGI(0,"the 24 pin level is %d",isr_cfg->level);
	fibo_gpio_mode_set(5,0);
	fibo_gpio_set(5,isr_cfg->level);
}
void oc_gpioisrcallback1(void *param)
{
	hal_gpioisr_t *isr_cfg = (hal_gpioisr_t*)param;
	fibo_gpio_get(25,&(isr_cfg->level));
	fibo_gpio_pull_up_or_down(24,true);
	OSI_LOGI(0,"the 25 pin level is %d",isr_cfg->level);
	fibo_gpio_mode_set(5,0);
	fibo_gpio_set(5,isr_cfg->level);
}
void oc_gpioisrcallback2(void *param)
{
	hal_gpioisr_t *isr_cfg = (hal_gpioisr_t*)param;
	fibo_gpio_get(26,&(isr_cfg->level));
	OSI_LOGI(0,"the 26 pin level is %d",isr_cfg->level);
	fibo_gpio_pull_up_or_down(24,false);
	fibo_gpio_mode_set(5,0);
	fibo_gpio_set(5,isr_cfg->level);
}

static void prvThreadEntry(void *param)
{
	INT32 ret = -1;
	INT32  i =0;

    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    ret =fibo_lpg_switch(false);// first close lpg
    if(ret ==-1)
    {
		OSI_LOGI(0, "fibo_lpg_switch lpg failed");
		return;
	}
	#if 0
    ret = fibo_gpio_pull_disable(5);  // gpio13 is lpg 
    if(ret ==-1)
    {
		OSI_LOGI(0, "fibo_gpio_pull_disable lpg failed");
		return;
	}
	fibo_taskSleep(1000);
    OSI_LOGI(0, "reset lpg config lpg");
	ret = fibo_gpio_mode_set(5,0);  //  setting gpio13 to mode 0,mode 0 is gpio 
	if(ret ==-1)
	{
		OSI_LOGI(0, "fibo_gpio_mode_set lpg failed");
		return;
	}	
	OSI_LOGI(0, "set lpg pin output");
	ret = fibo_gpio_cfg(5,1);       // setting gpio13 to output
	if(ret ==-1)
	{
		OSI_LOGI(0, "fibo_gpio_cfg lpg failed");
		return;
	}	

	OSI_LOGI(0, "set lpg pin high");
	ret = fibo_gpio_set(5,1);	   // setting gpio13 output high
	if(ret ==-1)
	{
		OSI_LOGI(0, "fibo_gpio_set high lpg failed");
		return;
	}	

	fibo_taskSleep(5000); 
    OSI_LOGI(0, "set lpg pin low");
	ret = fibo_gpio_set(5,0);
	if(ret ==-1)
	{
		OSI_LOGI(0, "fibo_gpio_set low lpg failed");
		return;
	}	
	
	fibo_taskSleep(5000);	
    OSI_LOGI(0, "set lpg pin high");
	ret = fibo_gpio_set(5,1);
	if(ret ==-1)
	{
		OSI_LOGI(0, "fibo_gpio_set high lpg failed");
		return;
	}	
	fibo_taskSleep(5000);
	OSI_LOGI(0, "set lpg pin low");
	ret = fibo_gpio_set(5,0);
	if(ret ==-1)
	{
		OSI_LOGI(0, "fibo_gpio_set low lpg failed");
		return;
	}	
	OSI_LOGI(0, "fibo_gpio_pull_disable lpg");
	fibo_gpio_pull_disable(5);
    OSI_LOGI(0, "reset lpg config lpg");
	//fibo_lpg_switch(true);// first close lpg
	//#else
	oc_isr_t g_oc_ist_prt = {
		.is_debounce = true,
		.intr_enable = true,  
		.intr_level = false,   
		.intr_falling = true,   
		.inte_rising = true, 
		.callback = oc_gpioisrcallback,  
	};
	oc_isr_t g_oc_ist_prt1 = {
		.is_debounce = true,
		.intr_enable = true,  
		.intr_level = false,   
		.intr_falling = true,   
		.inte_rising = true, 
		.callback = oc_gpioisrcallback1,  
	};
	oc_isr_t g_oc_ist_prt2 = {
		.is_debounce = true,
		.intr_enable = true,  
		.intr_level = false,   
		.intr_falling = true,   
		.inte_rising = true, 
		.callback = oc_gpioisrcallback2,  
	};
	fibo_gpio_isr_init(24,&g_oc_ist_prt);	
	fibo_gpio_isr_init(25,&g_oc_ist_prt1);
	fibo_gpio_isr_init(26,&g_oc_ist_prt2);
	fibo_taskSleep(120000);
	fibo_taskSleep(20000);

	UINT8 value1 = 0;
	UINT8 value2 = 0;
	UINT8 value3 = 0;
	UINT8 value4 = 0;
	UINT8 value5 = 0;
	UINT8 value6 = 0;
	UINT8 value7 = 0;
	

	fibo_gpio_mode_set(25,0);
	fibo_gpio_mode_set(26,0);
	fibo_gpio_mode_set(27,0);
	fibo_gpio_mode_set(28,1);
	fibo_gpio_mode_set(29,1);
	fibo_gpio_mode_set(30,1);
	fibo_gpio_mode_set(31,1);

	fibo_gpio_cfg(25,0);
	fibo_gpio_cfg(26,0);
	fibo_gpio_cfg(27,0);
	fibo_gpio_cfg(28,0);
	fibo_gpio_cfg(29,0);
	fibo_gpio_cfg(30,0);
	fibo_gpio_cfg(31,0);

	while(1)
	{
		fibo_gpio_get(25,&value1);
		OSI_LOGI(0, "> fibo_gpio_get[25] = %d", value1);
		fibo_taskSleep(1000);
		fibo_gpio_get(26,&value2);
		OSI_LOGI(0, "> fibo_gpio_get[26] = %d", value2);
		fibo_taskSleep(1000);
		fibo_gpio_get(27,&value3);
		OSI_LOGI(0, "> fibo_gpio_get[27] = %d", value3);
		fibo_taskSleep(1000);
		fibo_gpio_get(28,&value4);
		OSI_LOGI(0, "> fibo_gpio_get[28] = %d", value4);
		fibo_taskSleep(1000);
		fibo_gpio_get(29,&value5);
		OSI_LOGI(0, "> fibo_gpio_get[29] = %d", value5);
		fibo_taskSleep(1000);
		fibo_gpio_get(30,&value6);
		OSI_LOGI(0, "> fibo_gpio_get[30] = %d", value6);
		fibo_taskSleep(1000);
		fibo_gpio_get(31,&value7);
		OSI_LOGI(0, "> fibo_gpio_get[31] = %d", value7);
		fibo_taskSleep(1000);
	}
	
	OSI_LOGI(0, "test sleep mode start,wait resigter 1min");
	fibo_taskSleep(60000);	
	ret = fibo_setSleepMode(3);
	OSI_LOGI(0, "test sleep mode step1,ret= %d",ret);
	fibo_taskSleep(30000);	
	ret = fibo_setSleepMode(0);	
	OSI_LOGI(0, "test sleep mode step2,ret= %d",ret);
	fibo_taskSleep(10000);	
	ret = fibo_setSleepMode(5);	
	OSI_LOGI(0, "test sleep mode step3,ret= %d",ret);
	fibo_taskSleep(30000);	
	ret = fibo_setSleepMode(0);
	OSI_LOGI(0, "test sleep mode step4,ret= %d",ret);
	#else
	OSI_LOGI(0, "test sleep mode start,wait resigter 1min");
	fibo_taskSleep(10000);	
	ret = fibo_getVbatStaticVol();
	OSI_LOGI(0, "test fibo_getVbatStaticVol ret= %d",ret);
	fibo_taskSleep(3000);
	ret = fibo_getVbatVolLevel();
	OSI_LOGI(0, "test fibo_getVbatVolLevel ret= %d",ret);
	fibo_taskSleep(3000);
	ret = fibo_getChargeCur();
	OSI_LOGI(0, "test fibo_getChargeCur ret= %d",ret);
	fibo_taskSleep(3000);
	{
		chargerinfo_t test_conf=
			{4200,1000,4000,3400,3600};
		bool result = false;
		result = fibo_setChargeConf(&test_conf);
		OSI_LOGI(0, "test fibo_setChargeConf ret= %d",result);
		for(i =0;i<10;i++)
		{
			fibo_taskSleep(2000);
			ret = fibo_getVbatStaticVol();
			OSI_LOGI(0, "test fibo_getVbatStaticVol ret= %d",ret);
			fibo_taskSleep(2000);
			ret = fibo_getChargeCur();
			OSI_LOGI(0, "test fibo_getChargeCur ret= %d",ret);
		}
		fibo_taskSleep(120000);
	}
	#endif
	fibo_thread_delete();
}

void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(prvThreadEntry, "mythread", 1024, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
