#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "uart_operate.h"
#include "elog.h"
#include "common_data.h"
#include "net_task.h"
#include "parameter_number_and_value.h"
#include "watch_dog.h"
#include "run_log.h"
#include "xmodem_server.h"
#include "gpio_operate.h"

extern hal_uart_config_t   uart1_cfg;
extern hal_uart_config_t   uart3_cfg;

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

void * appimg_enter(void *param)
{
    gpio_init();				//gpio初始化

    uart_init(UART1,&uart1_cfg, uart1_recv_cb, NULL);
    uart_init(UART3,&uart3_cfg, uart3_recv_cb, NULL); 

	elog_init();
 	elog_start();

    log_init();

    COMMON_DataInit();
    prvInvokeGlobalCtors();

    INT8 *hardware_version = NULL;
    INT8 *software_version = NULL;

    hardware_version   = fibo_get_hw_verno();               //获取当前的硬件版本（客户定制）
    software_version   = fibo_get_sw_verno();               //获取当前的软件版本（客户定制）
    INT32 disret = fibo_watchdog_disable();

    for (int j = 0; j < 2; j++)
    {
        log_d("\r\nhardware_version %s\r\n",hardware_version); 
        log_d("\r\nsoftware_version %s\r\n",software_version); 
        log_d("\r\ndisret is %d\r\n",disret); 
    }

    fibo_thread_create(net_task,          "NET TASK",          1024*8*5, NULL, OSI_PRIORITY_NORMAL);
    fibo_thread_create(device_update_task,"DEVICE UPDATE TASK",1024*8*3, NULL, OSI_PRIORITY_NORMAL);
    fibo_thread_create(feed_dog_task,     "FEED DOG TASK",     1024*8*2, NULL, OSI_PRIORITY_NORMAL);

    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}