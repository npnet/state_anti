/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : net.c for 4G modem
  *@notes   : 2020.08.04 MWY
******************************************************************************/
#ifdef _PLATFORM_L610_
#include "fibo_opencpu.h"
#endif

#include "eyblib_list.h"
#include "eyblib_typedef.h"
#include "eyblib_swap.h"

#include "eybpub_Debug.h"
#include "eybpub_Status.h"
#include "eybpub_utility.h"

// #include "eybpub_watchdog.h"
// #include "eybpub_run_log.h"

#include "4G_net.h"
// #include "L610Net.h"

#include "eybapp_appTask.h"

#include "restart_net.h"

typedef struct {
  u8_t port;
  Buffer_t buf;
} NetSend_t;

// #define NET_OVERTIME  (60*10)
#define NET_OVERTIME  (60)  // 1 min overtime for GSM register
// #define MEMOTY_SIZE  (160 * 1024)  // unit 1K
// static char testIP[64] = {0};
// static u16_t testPort;

static u32_t NetOvertime = 0;
static u32_t m_timeCheck = 0;

static ListHandler_t netSendPakege;
// static int netInTest(Buffer_t *buf, void_fun_bufp output);
// static s32_t m_simStat = SIM_STAT_UNSPECIFIED;
static void Net_sendData(void);

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void proc_net_task(s32_t taskId) {
  int ret = -1;
  ST_MSG msg;
  u8_t tmpbuf[64];

  static int netResetCnt = 0;
  s32_t cereg = 0;
  u32_t uStackSize = 0;

  // variable Initialization
  netResetCnt = 0;
  NetOvertime = 0;
  m_timeCheck = 0;

  r_memset(&msg, 0, sizeof(ST_MSG));
  //开机重连网络
  Eybpub_UT_SendMessage(EYBNET_TASK, NET_CMD_RESTART_ID, 0, 0);

  APP_PRINT("Net task run...\r\n");
  list_init(&netSendPakege);    // 初始化网络发送队列
  while (1) {
    fibo_queue_get(EYBNET_TASK, (void *)&msg, 0);
    switch (msg.message) {
      case APP_MSG_UART_READY:
        APP_DEBUG("Net task APP_MSG_UART_READY\r\n");
        L610Net_init();
        break;
      case NET_MSG_SIM_READY:
        APP_DEBUG("Net task NET_MSG_SIM_READY\r\n");
        break;
      case NET_MSG_SIM_FAIL:
        APP_DEBUG("Net task NET_MSG_SIM_FAIL\r\n");
        break;
      case NET_MSG_GSM_READY:
        APP_DEBUG("Net task NET_MSG_GSM_READY\r\n");
        GSMLED_On();
        L610Net_ready();
        Eybpub_UT_SendMessage(EYBAPP_TASK, NET_MSG_GSM_READY, 0, 0);
        break;
      case NET_MSG_GSM_FAIL:
        APP_DEBUG("Net task NET_MSG_GSM_FAIL\r\n");
        GSMLED_Off();
        Eybpub_UT_SendMessage(EYBAPP_TASK, NET_MSG_GSM_FAIL, 0, 0);
        break;
//    case NET_MSG_GSM_READY:
//        APP_PRINT("\r\ndns success!!!next ready to creat socket\r\n");
//        break;
      case APP_MSG_TIMER_ID:  // 从APP task传递过来的定时器(1000 ms)消息
//        APP_DEBUG("Net task get APP_USER_TIMER_ID:%ld\r\n", NetOvertime);
        L610Net_manage();
        break;
      default:
        break;
    }
  }
  fibo_thread_delete();
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
u8_t Net_connect(u8_t mode, char *ip, u16_t port, NetDataCallback netCallback) {
//  return BC25Net_open(mode, ip, port, netCallback);
  return 1;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void Net_close() {
//  BC25Net_close();
  return;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
u8_t Net_status() {
//  return BC25Net_status();
  return 1;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void Net_send(u8_t port, u8_t *pData, u16_t len) {
  NetSend_t *sendData = list_nodeApply(sizeof(NetSend_t) + len);
  if (sendData != null) {
    sendData->port = port;
    sendData->buf.lenght = len;
    sendData->buf.payload = (u8_t *)(sendData + 1);
    list_bottomInsert(&netSendPakege, sendData);
  } else {
    APP_DEBUG("memory apply full!!!");
  }
}
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static void Net_sendData(void) {
/*  NetSend_t *send = (NetSend_t *)list_nextData(&netSendPakege, null);
  if (send == null) {
    return;
  } else if (send->buf.lenght == 0) {
    list_nodeDelete(&netSendPakege, send);
  } else {
    int ret = 0;
  } */
}

/******************************************************************************/

