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


static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
		//fibo_PDPActive  ip address resopnse event
		case GAPP_SIG_PDP_ACTIVE_ADDRESS:
		{
			UINT8 cid = (UINT8)va_arg(arg, int);
			char *addr = (char *)va_arg(arg, char *);
			if(addr != NULL)
			{
				OSI_PRINTFI("sig_res_callback  cid = %d, addr = %s ", cid, addr);
			}
			else 
			{
				OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
			}
			va_end(arg);
		}
		break;

		default:
		break;
    }
}

static void at_res_callback(UINT8 *buf, UINT16 len)
{
    OSI_PRINTFI("FIBO <--%s", buf);
}

static FIBO_CALLBACK_T user_callback = {
	.fibo_signal = sig_res_callback,
    .at_resp = at_res_callback};


static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

	UINT8 ip[50];
	int test = 1;
	reg_info_t reg_info;
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	char apn[] = "we-adc.abc123.com.abc.123";

    fibo_set_initial_pdp(1,3,apn,0,NULL,NULL,0);
	fibo_taskSleep(1000);
	fibo_cfun_zero();
	fibo_taskSleep(1000);
	fibo_cfun_one();
	
	while(test)
	{
	    fibo_getRegInfo(&reg_info, 0);
		fibo_taskSleep(1000);
		OSI_PRINTFI("[%s-%d]pow(1.5,1.5)=%f", __FUNCTION__, __LINE__,pow(1.5,1.5));
		if(reg_info.nStatus==1)
		{
			test = 0;
			fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
			fibo_taskSleep(1000);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}

    while(1)
    {
		fibo_taskSleep(10000);
		test_printf();
	}
	
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
