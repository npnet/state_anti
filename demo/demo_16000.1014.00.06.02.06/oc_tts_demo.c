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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('T', 'T', 'S', 'Y')

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

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

    INT8 *pUtf8 = "E8BF99E9878CE698AF545453E6B58BE8AF95"; /*UTF8 这里是TTS测试*/
    INT8 *pGb2312 = "D5E2C0EFCAC7545453B2E2CAD4"; /*GB2312 这里是TTS测试*/
    INT8 *pUnicode = "D98FCC912F665400540053004B6DD58B"; /* UNICODE 这里是TTS测试 */

    fibo_taskSleep(5000);
    if(GAPP_TTS_SUCCESS == fibo_tts_start((const UINT8 *)pUtf8, CTTS_STRING_ENCODE_TYPE_UTF8))
    {
        OSI_LOGI(0,"TTS UTF8 play success");
    }
    else
    {
        OSI_LOGI(0,"TTS UTF8 play error");
    }

    while(fibo_tts_is_playing())
    {
        OSI_LOGI(0,"TTS wait play!");
        fibo_taskSleep(1000);
    }

    if(GAPP_TTS_SUCCESS == fibo_tts_start((const UINT8 *)pGb2312, CTTS_STRING_ENCODE_TYPE_GB2312))
    {
        OSI_LOGI(0,"TTS GB2312 play success");
    }
    else
    {
        OSI_LOGI(0,"TTS GB2312 play error");
    }

    while(fibo_tts_is_playing())
    {
        OSI_LOGI(0,"TTS wait play!");
        fibo_taskSleep(1000);
    }

    if(GAPP_TTS_SUCCESS == fibo_tts_start((const UINT8 *)pUnicode, CTTS_STRING_ENCODE_TYPE_UNICODE))
    {
        OSI_LOGI(0,"TTS UNICODE play success");
    }
    else
    {
        OSI_LOGI(0,"TTS UNICODE play error");
    }

    fibo_thread_delete();
}
static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
    case GAPP_SIG_TTS_END:
    {
        OSI_LOGI(0, "ttsfinish", 0);
    }
    default:
    {
        break;
    }
    }
    OSI_LOGI(0, "test %d",sig);
}
static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
    fibo_thread_create(prvThreadEntry, "mythread", 1024 * 4, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
