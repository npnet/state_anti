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
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}
key_callback wifiscan_refponse_cb(void *param)
{
    if(param != NULL)
    {
		fibo_wifiscan_res_t *ap_info = (fibo_wifiscan_res_t *)param;
		OSI_PRINTFI("[%s-%d] ap_info->ap_num=%d", __FUNCTION__, __LINE__,ap_info->ap_num);
		OSI_PRINTFI("[%s-%d] ap_info[0].addr=%s", __FUNCTION__, __LINE__,ap_info->fibo_wifiscan_ap_info[0].ap_addr);
	}
	else 
	{
		OSI_PRINTFI("[%s-%d],wifiscan fail", __FUNCTION__, __LINE__);
	}

	fibo_wifiscan_off();

	return NULL;
}


static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

	fibo_taskSleep(10*1000);
	if(fibo_wifiscan_on()>=0)
	{
         fibo_wifiscan(wifiscan_refponse_cb);
	}

	while(1)
	{
		OSI_LOGI(0, "hello world");
		fibo_taskSleep(10*1000);
	}

	test_printf();
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
