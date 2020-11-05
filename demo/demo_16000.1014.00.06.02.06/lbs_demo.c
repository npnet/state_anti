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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('F', 'I', 'B', 'O')

#include "osi_api.h"
#include "osi_log.h"
#include "osi_pipe.h"
#include "at_engine.h"
#include <string.h>
#include "fibo_opencpu.h"


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

static void prvVirtAtEntry(void *param)
{
    int ret = -1;	
	fibo_taskSleep(20000);
	UINT8 ip[50];
	memset(&ip, 0, sizeof(ip));
	reg_info_t reg_info;

	int test =1;
	while(test)
	{
	    fibo_getRegInfo(&reg_info, 0);
		fibo_taskSleep(1000);
		if(reg_info.nStatus==1)
		{
			test = 0;
		}
	}
	ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
	
	OSI_PRINTFI("[%s-%d] ret = %d,ip=%s", __FUNCTION__, __LINE__, ret,ip);
	
	fibo_taskSleep(1000);
	ret = fibo_lbs_set_key("bc3bd48feda96929daf3e78839a184ad");
	OSI_PRINTFI("[%s-%d] ret=%d", __FUNCTION__, __LINE__,ret);
	
	
    ret=fibo_lbs_get_gisinfo(6);
	OSI_PRINTFI("[%s-%d] ret=%d", __FUNCTION__, __LINE__,ret);
	fibo_taskSleep(10000);
    fibo_thread_delete();
}

static FIBO_CALLBACK_T user_callback = {
	.fibo_signal = sig_res_callback,
    .at_resp = at_res_callback};

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    //init
    fibo_thread_create(prvVirtAtEntry, "at", 1024*8*5, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

