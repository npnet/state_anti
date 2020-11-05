/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : net.c for 4G modem
  *@notes   : 2020.08.04 MWY
******************************************************************************/
#include "eyblib_list.h"
#include "eyblib_typedef.h"
#include "eyblib_swap.h"

#include "eybpub_Debug.h"
#include "eybpub_Status.h"
// #include "eybpub_watchdog.h"
// #include "eybpub_run_log.h"

#include "4G_net.h"
#include "L610Net.h"

#include "eybapp_appTask.h"

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
//  ST_MSG msg;
  u8_t tmpbuf[64];

  static int netResetCnt = 0;
  s32_t cereg = 0;
  u32_t uStackSize = 0;

  /*variable Initialization*/
  netResetCnt = 0;
  NetOvertime = 0;
  m_timeCheck = 0;

  list_init(&netSendPakege);    // 初始化网络发送队列

  while (1) {
/*    Ql_OS_GetMessage(&msg);
    switch (msg.message) {
      case APP_MSG_UART_READY:
        APP_PRINT("Net task run...\r\n");
        break;
      case NET_CMD_RESTART_ID:  // BC25 restart
        break;
      case NET_CMD_AT_ID:  // mike 20200817 注销处理AT指令，转到APP task
        break;
      case NET_CMD_SENDDATA_ID:
      case APP_MSG_TIMER_ID: { // 从APP task传递过来的定时器(1000 ms)消息
        APP_DEBUG("Net task get APP_USER_TIMER_ID:%d\r\n", NetOvertime);
#if TEST_RF
#else
        APP_DEBUG("RF test off!!\r\n");
#endif
        m_timeCheck++;
        if (m_timeCheck >= 60) {
//          uStackSize = Ql_OS_GetCurrenTaskLeftStackSize();
          APP_DEBUG("net task stack size:%ld!!\r\n", uStackSize);
          m_timeCheck = 0;
        }
        BC25Net_manage();
      }
      break;
      default:
        break;
    }*/
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
u8_t Net_connect(u8_t mode, char *ip, u16_t port, NetDataCallback netCallback) {
  return BC25Net_open(mode, ip, port, netCallback);
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void Net_close() {
  BC25Net_close();
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
u8_t Net_status() {
  return BC25Net_status();
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
//    Ql_memcpy(sendData->buf.payload, pData, len);
//    Ql_OS_SendMessage(EYBNET_TASK, NET_CMD_SENDDATA_ID, 0, 0);
  } else {
    APP_DEBUG("memory apply full!!!");
  }
}
#ifndef _PLATFORM_L610_
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static void Net_sendData(void) {
  NetSend_t *send = (NetSend_t *)list_nextData(&netSendPakege, null);
  if (send == null) {
    // memory_trans(Debug_output);  // mike 20200805
    return;
  } else if (send->buf.lenght == 0) {
    list_nodeDelete(&netSendPakege, send);
  } else {
    int ret = 0;
    ret = BC25Net_send(send->buf.payload, send->buf.lenght);
    if (ret == RIL_AT_SUCCESS) {
      list_nodeDelete(&netSendPakege, send);
      APP_DEBUG("Send LWM2M data successfully.\r\n");
//      Ql_Delay_ms(1000);    // NB消息之间需要延时?
      NetOvertime = 0;
    } else if (ret == RIL_AT_FAILED) {
      APP_DEBUG("Send LWM2M data Fail.\r\n");
      return;
    } else if (ret == -520) {
      list_nodeDelete(&netSendPakege, send);
      APP_DEBUG("data is longer than 512.\r\n");
    } else {
      APP_DEBUG("Fail to connect to server, cause=%d\r\n", ret);
    }
  }
//  Ql_OS_SendMessage(EYBNET_TASK, NET_CMD_SENDDATA_ID, 2, 0);
}
#else
static void Net_sendData(void) {
}
#endif
/******************************************************************************/

