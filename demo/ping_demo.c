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

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }
    char test[22] = "ipv6.baidu.com";
    fibo_taskSleep(10000);
    fibo_mping(2, test, 255, 0, 0, 0, 0);
    fibo_taskSleep(10000);
    fibo_mping(0, 0, 0, 0, 0, 0, 0);
    char *pt = (char *)fibo_malloc(512);
    if (pt != NULL)
    {
        OSI_LOGI(0, "malloc address %u", (unsigned int)pt);
        fibo_free(pt);
    }

	test_printf();
    fibo_thread_delete();
}
static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
    case GAPP_SIG_MPING_RECV:
    {
        char *val1 = va_arg(arg,char *);
        OSI_PRINTFI("%s", val1);
        break;
    }
    case GAPP_SIG_MPINGSTAT_RECV:
    {
        char *val1 = va_arg(arg,char *);
        OSI_PRINTFI("%s", val1);
        break;
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
    fibo_thread_create(prvThreadEntry, "mythread", 1024, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}


void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
