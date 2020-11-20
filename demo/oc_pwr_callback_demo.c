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


static void close_printf(void * arg)
{
	OSI_PRINTFI("pwr_cb: module close call back run...\n");
	osiThreadSleep(3000);
	fibo_softPowerOff();
}


static void prvThreadEntry(void *param)
{	
	
    INT32 ret=0;
	OSI_PRINTFI("pwr_cb: set pwr_cb_ex\n");
	ret=fibo_set_pwr_callback_ex(close_printf, 4000, NULL);
	if (ret>0)
	{
		OSI_PRINTFI("pwr_cb: fibo_set_pwr_callback_ex success");
	}
    osiThreadSleep(2000);
	ret=fibo_set_pwr_callback(close_printf, 4000);
	if (ret>0)
	{
		OSI_PRINTFI("pwr_cb: fibo_set_pwr_callback success");
	}

    while (1)
    {
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
