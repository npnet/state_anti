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

void rec_check_task(void);

void rec_check_task()
{
    UINT8 recovery_flag = 1;
    INT32 get_ret       = 0;
    while(1)
    {
        fibo_taskSleep(1000);        
        get_ret = fibo_gpio_get(RECOVERY,&recovery_flag);

        log_d("\r\nrecovery_flag is %d\r\n",recovery_flag);

        if(1 == get_ret)
        {
            log_d("\r\nget_ret success\r\n");
        }

        if(-1 == get_ret)
        {
            log_d("\r\nget_ret fail\r\n");
        }

        if(0 == recovery_flag)
        {
            log_d("\r\nrecovery_flag...\r\n");
        }
    }
}

void * appimg_enter(void *param)
{
    gpio_init();				//gpio初始化

    uart_init(UART1,&uart1_cfg, uart1_recv_cb, NULL);
    uart_init(UART3,&uart3_cfg, uart3_recv_cb, NULL); 

	elog_init();
 	elog_start();

    uart1_cfg_update();         //更新串口配置

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

    INT32 adc_init_ret = fibo_hal_adc_init();        //ADC打开接口
    if(0 == adc_init_ret)
    {
        log_d("\r\nadc_init success\r\n"); 
    }

    if(adc_init_ret < 0)
    {
        log_d("\r\nadc_init fail\r\n");   
    }

    fibo_thread_create(net_task,          "NET TASK",          1024*8*5, NULL, OSI_PRIORITY_NORMAL);
    fibo_thread_create(device_update_task,"DEVICE UPDATE TASK",1024*8*3, NULL, OSI_PRIORITY_NORMAL);
    fibo_thread_create(feed_dog_task,     "FEED DOG TASK",     1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    fibo_thread_create(rec_check_task,    "REC  CHECK TASK",   1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}