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
/*static void _apiAlarmCB(void *userdata)
{
    OSI_PRINTFI("testststststst");
}*/

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

    char *pt = (char *)fibo_malloc(512);
    if (pt != NULL)
    {
        OSI_LOGI(0, "malloc address %u", (unsigned int)pt);
        fibo_free(pt);
    }

    fibo_thread_delete();
}

void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024, NULL, OSI_PRIORITY_NORMAL);
    hal_rtc_time_t tm;
    hal_rtc_time_t mt;
    memset(&tm, 0, sizeof(tm));
    memset(&mt, 0, sizeof(mt));
    tm.year = 51;
    tm.month = 12;
    tm.day = 29;
    tm.hour = 12;
    tm.min = 13;
    tm.sec = 14;
    int test1 = fibo_setRTC(NULL);
    OSI_LOGI(0, "testset____%d", test1);
    fibo_taskSleep(10000);
    int test2 = fibo_getRTC(NULL);
    OSI_LOGI(0, "testget____%d", test2);
    OSI_LOGI(0, "testprint____%d/%d/%d,%d:%d:%d,%d", mt.year, mt.month, mt.day, mt.hour, mt.min, mt.sec, mt.wDay);
    tm.year = 23;
    fibo_taskSleep(10000);
    test1 = fibo_setRTC(&tm);
    OSI_LOGI(0, "testset____%d", test1);
    fibo_taskSleep(10000);
    test2 = fibo_getRTC(&mt);
    OSI_LOGI(0, "testget____%d", test2);
    OSI_LOGI(0, "testprint____%d/%d/%d,%d:%d:%d,%d", mt.year, mt.month, mt.day, mt.hour, mt.min, mt.sec, mt.wDay);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
