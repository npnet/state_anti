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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('C', 'O', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "oc_ws.h"

extern void test_printf(void);

static void ws_data1(void *t, void *data, UINT32 len, void *arg)
{
    OSI_PRINTFI("recv %.*s\n", len, (char *)data);
}

static void ws_close1(void *t, int reason, void *arg)
{
    OSI_PRINTFI("close %d\n", reason);
}

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

    int test = 1;
    OSI_PRINTFI("coapapi wait network");
    reg_info_t reg_info;

    while (test)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            test = 0;
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);
            OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        }
    }

    fibo_taskSleep(5 * 1000);

    fibo_ws_opt_t opt = {
        5, 10, true, NULL, "x3JJHMbDL1EzLkh9GBhXDw=="};

    void *c = fibo_ws_open("ws://127.0.0.1:9001", &opt, ws_data1, ws_close1, NULL);

    char data[] = "ws socket data test";
    fibo_ws_send_text(c, data);

    fibo_taskSleep(5000);
    fibo_ws_close(c);
    fibo_taskSleep(1000);
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread-ws", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
