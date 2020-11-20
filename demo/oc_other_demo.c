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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('O', 'T', 'H', 'R')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"




static void prvThreadEntry(void *param)
{	
	uint8_t pin_id = 135;
	uint8_t keyval;
	uint8_t keylast=0xff;
	uint8_t pwrkeyval;
	uint8_t pwrkeylast=0xf;
	uint8_t usbstatus=0;
	uint8_t usbstatuslast=0xff;
	
	uint8_t BrightLev=0;
	uint32_t outesc=0;
	char * time_tset="2020/04/15,09:41:13";
	uint8_t tim[64];
	int32_t Batvol=0;
	uint8_t BatRem=0;
	uint8_t chargestatus=0;
	uint8_t chargestatuslast=0xf;
	int16_t bootcause=0;
	bool audio_status=false;
	char get_device_NUM[11]={0};
	unsigned int adcarray[1]={0};
	unsigned int *padc = adcarray;

    keypad_info_t keypad = {0};
    while (1)
    {
        keypad = fibo_get_pwrkeypad_status();
        if (keypad.key_id == 0)
        {
            OSI_PRINTFI("keypad press_or_release=%d, long_or_short_press=%d\n", keypad.press_or_release, keypad.long_or_short_press);
            if (keypad.press_or_release == KEY_RELEASE)
            {
                OSI_PRINTFI("keypad pwrkey release");
                //when release, quit
                break;
            }
        }
    }
	 
    if (fibo_gpio_mode_set(pin_id,0)<0)
    {
        OSI_LOGI(0, "the lcd drvLedbackopen fibo_gpio_mode_set error");
    }
    if (fibo_gpio_set(pin_id, 1)<0)
    {
        OSI_LOGI(0, "the lcd drvLedbackopen fibo_gpio_set error");
    }
	if (!fibo_pwrkey_init())
	{
		OSI_LOGI(0, "fibo_pwrkey_init error");
	}

	fibo_pwmOpen();
	fibo_TY_wakeup_init(1,1);
	fibo_TY_wakeup_enable();

    while(1)
    {
         keyval=fibo_read_key();
		 if (keylast != keyval)
		 {
		 	keylast = keyval;
		 	OSI_LOGI(0, "OtherTask keyval=%d.",keyval);
		 }
		 pwrkeyval=fibo_get_pwrkeystatus();
		 if (pwrkeylast != pwrkeyval)
		 {
		 	pwrkeylast = pwrkeyval;
		 	OSI_LOGI(0, "OtherTask pwrkeyval=%d.",pwrkeyval);
		 }
		 usbstatus=fibo_get_Usbisinsert();
		 if (usbstatuslast != usbstatus)
		 {
		 	usbstatuslast = usbstatus;
		 	OSI_LOGI(0, "OtherTask usbstatus=%d.",usbstatus);
			if (usbstatus ==1)
			{
			 	fibo_SetBatChargSel(0);
				OSI_LOGI(0, "OtherTask enable charge.");
			}
		 }
		 bootcause = fibo_GetPowerOnCause();
		 OSI_LOGI(0, "OtherTask bootcause=%d.",bootcause);

   		 fibo_GetChipID(get_device_NUM);	
   		 OSI_PRINTFI("OtherTask read cfsn number : %s",(char *)get_device_NUM);
   
   		 audio_status = fibo_get_audio_status();
   		 OSI_LOGI(0, "OtherTask audio_status=%d.",audio_status);
   
   		 fibo_hal_adc_get_data_polling(1, padc);
   		 OSI_LOGI(0, "OtherTask fibo_adc adcval=%d",*padc);

		 OSI_LOGI(0, "OtherTask SetLcdBright lvl=%d.",BrightLev);
		 fibo_SetLcdBright(BrightLev);
		 BrightLev+=50;
		 
		 memset(tim, 0, 64);
		 fibo_Rtc2Second((uint8_t *)time_tset, &outesc);
		 OSI_LOGI(0, "OtherTask outesc=%d.",outesc);
		 fibo_Second2Rtc(outesc, tim);
		 OSI_PRINTFI("OtherTask play tim=%s.",(char *)tim);
		 
		 Batvol=fibo_GetBatVolResult();
		 OSI_LOGI(0, "OtherTask Batvol=%d.",Batvol);
		 BatRem=fibo_GetBatRemainlvl();
		 OSI_LOGI(0, "OtherTask BatRem=%d.",BatRem);
		 
		 chargestatus=fibo_GetBatChargPinStatus();
		 if (chargestatuslast != chargestatus)
		 {
		 	chargestatuslast = chargestatus;
		 	OSI_LOGI(0, "OtherTask chargestatus=%d.",chargestatus);
		 }		 
		 osiThreadSleep(100);	
    }
    OSI_LOGD(0, "prvThreadEntry osiThreadExit");
    osiThreadExit();

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
