#ifdef _PLATFORM_L610_
#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('Q', 'E', 'K', 'J')

#include "eybpub_Debug.h"
#include "eybpub_Status.h"
#include "eybpub_utility.h"
#include "eybpub_watchdog.h"
#include "eybpub_run_log.h"
#include "eybpub_Key.h"
#include "eybpub_adc.h"

#include "4G_net.h"
#include "eybapp_appTask.h"
#include "Device.h"
#include "DeviceIO.h"
#include "eybond.h"
#include "L610_conn_ali_net.h"
#include "restart_net.h"

UINT32 EYBAPP_TASK = 0;
UINT32 EYBNET_TASK = 0;
UINT32 EYBDEVICE_TASK = 0;
UINT32 EYBOND_TASK = 0;
UINT32 ALIYUN_TASK = 0;

static void prvInvokeGlobalCtors(void) {
  extern void (*__init_array_start[])();
  extern void (*__init_array_end[])();

  size_t count = __init_array_end - __init_array_start;
  for (size_t i = 0; i < count; ++i) {
    __init_array_start[i]();
  }
}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg) {
  APP_DEBUG("sig_res_callback sig:%d\r\n", sig);
  switch (sig) {
    // fibo_PDPActive  ip address resopnse event
    case GAPP_SIG_PDP_ACTIVE_ADDRESS: {
      UINT8 cid = (UINT8)va_arg(arg, int);
      char *addr = (char *)va_arg(arg, char *);
      if(addr != NULL) {
        OSI_PRINTFI("sig_res_callback  cid = %d, addr = %s ", cid, addr);
        APP_DEBUG("sig_res_callback GAPP_SIG_PDP_ACTIVE_ADDRESS cid = %d, addr = %s\r\n", cid, addr);
      } else  {
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        APP_DEBUG("sig_res_callback addr is NULL\r\n");
      }
      va_end(arg);
	}
	  break;
    case GAPP_SIG_REPORT_URC_TEXT: {
      UINT8 CGREG = (UINT8)va_arg(arg, int);
      APP_DEBUG("sig_res_callback GAPP_SIG_REPORT_URC_TEXT cid = %d", CGREG);
	  va_end(arg);
    }
      break;
    default:
      break;
  }
}

static void at_res_callback(UINT8 *buf, UINT16 len) {
  OSI_PRINTFI("FIBO <--%s", buf);
//  APP_DEBUG("FIBO <--%s len:%d\r\n", buf, len);
  fibo_hal_uart_put(DEBUG_PORT, buf, len);
}

static FIBO_CALLBACK_T user_callback = {
  .fibo_signal = sig_res_callback,
  .at_resp = at_res_callback
};

void * appimg_enter(void *param) {
  OSI_LOGI(0, "application image enter");
  Debug_init(); // 上电配置串口
  DeviceIO_STinit(NULL);//上电配置设备口
  APP_PRINT("Eybond 4G L610 Application!!\r\n");
  APP_PRINT("Version: %s\r\n", FWVERSION);
  APP_PRINT("Time: %s\r\n", MAKE_TIME);

  INT8 *hardware_version = NULL;
  INT8 *software_version = NULL;

  hardware_version   = fibo_get_hw_verno();               //获取当前的硬件版本(客户定制)
  software_version   = fibo_get_sw_verno();               //获取当前的软件版本(客户定制)
  APP_PRINT("hardware_version %s\r\n",hardware_version);
  APP_PRINT("SDK version %s\r\n",software_version);
  
  Watchdog_init();
  NetLED_Init();
  GSMLED_Init();
  deviceLEDInit();
  Beep_Init();
  Key_init();
  ADC_Init();
    
  prvInvokeGlobalCtors();
  
  UINT32 net_thread_id = 0;
  UINT32 app_thread_id = 0;
  UINT32 dev_thread_id = 0;
  UINT32 eyb_thread_id = 0;
  UINT32 relink_thread_id = 0;
  UINT32 ali_thread_id = 0;

  EYBAPP_TASK = fibo_queue_create(5, sizeof(ST_MSG));
  EYBNET_TASK = fibo_queue_create(5, sizeof(ST_MSG));
  EYBDEVICE_TASK = fibo_queue_create(5, sizeof(ST_MSG));
  EYBOND_TASK = fibo_queue_create(5, sizeof(ST_MSG));
  ALIYUN_TASK = fibo_queue_create(5, sizeof(ST_MSG));

  fibo_thread_create_ex(proc_relink_task, "Eybond RELINK TASK", 1024*8*2, NULL, OSI_PRIORITY_NORMAL, &relink_thread_id);
  fibo_thread_create_ex(proc_net_task,    "Eybond NET TASK",    1024*8*2, NULL, OSI_PRIORITY_NORMAL, &net_thread_id);
  fibo_thread_create_ex(proc_app_task,    "Eybond APP TASK",    1024*8*2, NULL, OSI_PRIORITY_NORMAL, &app_thread_id);
  fibo_thread_create_ex(proc_device_task, "Eybond DEVICE TASK", 1024*8*2, NULL, OSI_PRIORITY_NORMAL, &dev_thread_id);
  fibo_thread_create_ex(proc_eybond_task, "Eybond CMD TASK",    1024*8*2, NULL, OSI_PRIORITY_NORMAL, &eyb_thread_id);
  fibo_thread_create_ex(mqtt_conn_ali_task,"MQTT CONN ALI TASK",1024*8*2, NULL, OSI_PRIORITY_NORMAL, &ali_thread_id);

  Eybpub_UT_SendMessage(EYBAPP_TASK, APP_MSG_UART_READY, 0, 0);
  Eybpub_UT_SendMessage(EYBNET_TASK, APP_MSG_UART_READY, 0, 0);
  Eybpub_UT_SendMessage(EYBDEVICE_TASK, APP_MSG_UART_READY, 0, 0);
  Eybpub_UT_SendMessage(EYBOND_TASK, APP_MSG_UART_READY, 0, 0);
  Eybpub_UT_SendMessage(ALIYUN_TASK, APP_MSG_UART_READY, 0, 0);

  return (void *)&user_callback;
}

void appimg_exit(void) {
  OSI_LOGI(0, "application image exit");
}

#endif
