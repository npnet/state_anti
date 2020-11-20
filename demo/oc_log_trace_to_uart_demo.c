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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('Q', 'E', 'K', 'J')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

extern void test_printf(void);


#define LOG_TEST(A,B,format, ...) sys_debug(A,B,"[LOG_TEST][%s:%d]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

void uart_recv(hal_uart_port_t uart_port, UINT8 *data, UINT16 len, void *arg)
{
    OSI_PRINTFI("uartapi recv uart_port=%d len=%d, data=%s", uart_port, len, (char *)data);
}


void sys_debug(hal_uart_port_t uart_port,hal_uart_config_t *uart_config,INT8 *fmt,...)
{
   va_list ap;
   hal_uart_config_t drvcfg ;
   INT8 trace[1024] = {0};

   va_start(ap,fmt);
   fibo_vsnprintf(trace, 1024, fmt, ap);
   va_end(ap);
   
   //fibo_log_uart_output(uart_port,uart_config ,trace);
      
    if( uart_port < 0 || uart_port >= 3 )
    {
        return;
	}

	if(trace == NULL)
	{
        return ;
	}

	if(uart_config == NULL)
	{
	    memset(&drvcfg,0,sizeof(hal_uart_config_t));
		drvcfg.baud = 115200;
		drvcfg.parity = HAL_UART_NO_PARITY;
		drvcfg.data_bits = HAL_UART_DATA_BITS_8;
		drvcfg.stop_bits = HAL_UART_STOP_BITS_1;
		drvcfg.rx_buf_size = UART_RX_BUF_SIZE;
		drvcfg.tx_buf_size = UART_TX_BUF_SIZE;
		fibo_hal_uart_init(uart_port, &drvcfg, uart_recv, NULL);
	}
	else
	{
		fibo_hal_uart_init(uart_port, uart_config, uart_recv, NULL);
	}

	fibo_hal_uart_put(uart_port, (UINT8 *)trace, strlen(trace));

   return;
}



static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

    for (int n = 0; n < 10; n++)
    {
       // LOG_TEST(0,NULL);
        LOG_TEST(0,NULL,"hello world,%d,%d,%d",n,n+1,n+2);
		OSI_LOGI(0, "hello world %u", n);
        fibo_taskSleep(1000);
    }

    char *pt = (char *)fibo_malloc(512);
    if (pt != NULL)
    {
        OSI_LOGI(0, "malloc address %u", (unsigned int)pt);
        fibo_log_uart_output(0, NULL, "hello world");
        fibo_free(pt);
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
