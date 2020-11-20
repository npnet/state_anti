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

//#include "fibo_opencpu.h"
#include "osi_log.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "basic_type.h"
#include "mutil_app_common.h"




static void sig_recv_callback(THIRD_APP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
		case GAPP_SIG_TEST1:
		{

			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);

		}
		break;

		default:
		break;
    }
}


static THIRD_APP_RECV_SIG_CALLBACK_T thrid_app_recv_sig_callback = {
	.fibo_signal = sig_recv_callback

};

void * appimg_enter(void *param)
{
    APP_SERRICES_T * g_service = NULL;
	g_service = (APP_SERRICES_T *)param;
    OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);

	g_service->sys_test_201();

    return (void *)&thrid_app_recv_sig_callback;
}

void appimg_exit(void)
{
    return;
}
