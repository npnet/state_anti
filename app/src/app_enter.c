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
#include "uart_operate.h"
#include "elog.h"
#include "common_data.h"
#include "net_task.h"

extern hal_uart_config_t   uart1_cfg;
extern hal_uart_config_t   uart2_cfg;

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
    log_d("application thread enter, param 0x%x", param);
    //srand(100);

    //网络任务
	net_task();

    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    //gpio_init();				//gpio初始化

    INT32 uart1ret = uart_init(UART1,&uart1_cfg, uart1_recv_cb, NULL);
    INT32 uart2ret = uart_init(UART2,&uart2_cfg, uart2_recv_cb, NULL); 

	elog_init();
 	elog_start();

    if(0 == uart1ret)
    {
        log_d("uart1ret is %ld,uart1 init success", uart1ret);
    }
    else
    {
        log_d("uart1ret is %ld,uart1 init fail", uart1ret);
    }
    
    if(0 == uart2ret)
    {
        log_d("uart2ret is %ld,uart2 init success", uart2ret);
    }
    else
    {
        log_d("uart2ret is %ld,uart2 init fail", uart2ret);
    }
    
    //log_init();

    COMMON_DataInit();

	// live_a_and_b();
	// build_moment(CM_BUILD_TIME);
	// update_version();//更新版本

    log_d("application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);

    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
