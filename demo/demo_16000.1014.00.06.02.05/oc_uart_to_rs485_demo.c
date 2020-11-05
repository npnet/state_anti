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
#include "oc_uart.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


void uart_recv(hal_uart_port_t uart_port, UINT8 *data, UINT16 len, void *arg)
{
    OSI_PRINTFI("uartapi recv uart_port=%d len=%d, data=%s", uart_port, len, (char *)data);
}
static uint8_t *tx_buff = NULL;
static void rs485Task(void *ctx)
{
    OSI_LOGI(0, "rs485 start");


    uint32_t lencout = 0;


    tx_buff = (uint8_t *)malloc(4096*sizeof(uint8_t));

	fibo_hal_pmu_setlevel(1,1);
	fibo_gpio_mode_set(28, 6);
    fibo_gpio_mode_set(29, 6);
	
	hal_uart_config_t drvcfg = {0};
    drvcfg.baud = 115200;
	drvcfg.parity = HAL_UART_NO_PARITY;
    drvcfg.data_bits = HAL_UART_DATA_BITS_8;
    drvcfg.stop_bits = HAL_UART_STOP_BITS_1;
    drvcfg.rx_buf_size = UART_TX_BUF_SIZE;
    drvcfg.tx_buf_size = UART_TX_BUF_SIZE;
    fibo_rs485_init(2, &drvcfg, 30, NULL, NULL);
	
    while(1)
    {
		uint32_t len=fibo_rs485_read(2,tx_buff,1024);
		if (len <= 0)
		{
			if (lencout>0)
			{
			fibo_rs485_write(2,tx_buff,lencout,true);
			lencout=0;
			}
            osiThreadSleep(10);
		}
		else
		{
			lencout+=len;
		}
    }
    osiThreadExit();
}


void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(rs485Task, "rs485test", 1024, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
