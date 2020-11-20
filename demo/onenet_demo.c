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
    int i= 0;
    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }
    fibo_taskSleep(10000);
    int val1 = fibo_onenet_create(0,0,0,0);
    OSI_LOGI(0, "ttttt1 %d", val1);
    fibo_taskSleep(10000);
    uint8_t test[2] = {1,1};
    uint8_t testtss[1] = {1};
    int val2 = fibo_onenet_addobj(0, 3303, 2, test, 6, 1);
    int val22 = fibo_onenet_addobj(0, 3306, 1, testtss, 5, 0);
    OSI_LOGI(0, "ttttt2 %d,%d", val2, val22);
    fibo_taskSleep(2000);
    int val3 = fibo_onenet_open(0, 3600);
    OSI_LOGI(0, "ttttt3 %d", val3);
    while(i<5)
    {
        fibo_taskSleep(20000);
        int val0 = fibo_onenet_update(0, 3600, 1);
        OSI_LOGI(0, "ttttt0 %d", val0);
    }
    fibo_taskSleep(300000);
    int val4 = fibo_onenet_close(0);
    OSI_LOGI(0, "ttttt4 %d", val4);
    fibo_taskSleep(2000);
    int val5 = fibo_onenet_delete(0);
    OSI_LOGI(0, "ttttt5 %d", val5);
    char *pt = (char *)fibo_malloc(512);
    if (pt != NULL)
    {
        OSI_LOGI(0, "malloc address %u", (unsigned int)pt);
        fibo_free(pt);
    }

    fibo_thread_delete();
}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
    case GAPP_SIG_ONENET_EVENT_OBSERVER_REQUEST:
    {
        uint32_t ref = va_arg(arg,uint32_t);
        uint32_t mid = va_arg(arg,uint32_t);
        va_arg(arg,uint32_t);
        va_arg(arg,uint32_t);
        va_arg(arg,uint32_t);
        va_arg(arg,uint32_t);
        int val6 = fibo_onenet_observe_response(0, mid, 1);
        OSI_LOGI(0, "ttttt6 %d,%d", val6, mid);
    }
    case GAPP_SIG_ONENET_EVENT_DISCOVER_REQUEST:
    {
        uint32_t ref = va_arg(arg,uint32_t);
        uint32_t mid = va_arg(arg,uint32_t);
        uint32_t id = va_arg(arg,uint32_t);
        if(id == 3303)
        {
            char testss[] = "5700;5601;5602;5603;5604;5701;5605";
            int val7 = fibo_onenet_discover_response(0, mid, 1, 34, (uint8_t *)testss);
            OSI_LOGI(0, "ttttt7 %d,%d,%d", val7, mid, id);
        }
        else if(id == 3306)
        {
            char testss[] = "5850;5851;5852;5853;5750";
            int val7 = fibo_onenet_discover_response(0, mid, 1, 24, (uint8_t *)testss);
            OSI_LOGI(0, "ttttt7 %d,%d,%d", val7, mid, id);
        }
    }
    case GAPP_SIG_ONENET_EVENT_READ_REQUEST:
    {
        uint32_t ref = va_arg(arg,uint32_t);
        uint32_t mid = va_arg(arg,uint32_t);
        char testss[] = "9Y5FC";
        int val8 = fibo_onenet_read_response(0, mid, 1, 3303, 0, 5701, 1, 5, (uint8_t *)testss, 0);
        OSI_LOGI(0, "ttttt8 %d,%d,%d", val8, mid, ref);
    }
    case GAPP_SIG_ONENET_EVENT_WRITE_REQUEST:
    {
        uint32_t ref = va_arg(arg,uint32_t);
        uint32_t mid = va_arg(arg,uint32_t);
        int val9 = fibo_onenet_write_response(0, mid, 2);
        OSI_LOGI(0, "ttttt9 %d,%d,%d", val9, mid, ref);
    }
    case GAPP_SIG_ONENET_EVENT_EXECUTE_REQUEST:
    {
        uint32_t ref = va_arg(arg,uint32_t);
        uint32_t mid = va_arg(arg,uint32_t);
        int val10 = fibo_onenet_execute_response(0, mid, 2);
        OSI_LOGI(0, "ttttt10 %d,%d,%d", val10, mid, ref);
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

