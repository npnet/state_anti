/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : Eybond
  *@notes   : 2017.12.09 CGQ
*******************************************************************************/
#ifdef _PLATFORM_BC25_
#include "ql_system.h"
#include "ql_memory.h"
#include "ql_stdlib.h"
#include "NB_net.h"
#endif

#ifdef _PLATFORM_L610_
#include "4G_net.h"
#endif

#include "eyblib_list.h"
#include "eyblib_swap.h"
#include "eyblib_algorithm.h"
#include "eyblib_r_stdlib.h"
#include "CRC.h"

#include "eybpub_Debug.h"
#include "eybpub_SysPara_File.h"
#include "eybpub_run_log.h"
#include "Clock.h"

#include "eybond.h"
// #include "ESP_Update.h"

#include "eybapp_appTask.h"
#include "Device.h"

// #include "typedef.h"
// #include "memory.h"
// #include "SysPara.h"
// #include "r_stdlib.h"

// #include "modbus.h"
#include "ModbusDevice.h"
#include "Modbus.h"
#include "Sineng.h"

// #include "FlashFIFO.h"
// #include "FlashEquilibria.h"
// #include "FlashHard.h"

// #define MAX_CMD_LEN       (0xA00)    // mike LWM2M 消息的payload字段最大只有512字节
#define MAX_CMD_LEN       (488)         // 除去#PN#包头20字节，上传的modbus信息最多只有492，取488字节做安全限制

// #define HISTORY_SPACE_ADDR   (FLASH_EYBOND_HISTORY_ADDR)
// #define HISTORY_SPACE_SIZE   (4096)
// #define HISTORY_AREA_ADDR    (HISTORY_SPACE_ADDR + HISTORY_SPACE_SIZE)
// #define HISTORY_AREA_SIZE    (FLASH_EYBOND_HISTORY_SIZE - HISTORY_SPACE_SIZE)

// static char EybondServer[] = "www.shinemonitor.com:502";
static ListHandler_t rcveList;
static u8_t sPort;
static u16_t overtime_ESP;  // mike 20200918
static u16_t relinkCnt;
static u32_t m_timeCheck_EYB = 0;

// static u16_t histprySaveSpace;
// static int historySaveCnt;
// static FlashFIFOHead_t historyHead;
// static FlashEquilibria_t saveSpace;

static void NetMsgAck_Eybond(Buffer_t *buf);
static void output(Buffer_t *buf);
static void onlineReport(void);

static void ESP_process(void);
static u8_t espCmp(void *src, void *dest);
// static u8_t heartbeat(ESP_t *esp);
static u8_t paraGet(ESP_t *esp);
static u8_t paraSet(ESP_t *esp);
static u8_t devtrans(ESP_t *esp);
static u8_t devtransAck(Device_t *dev);
static u8_t deviceDataGet(ESP_t *esp);
static u8_t specialData_receive(ESP_t *esp);
// static u8_t commnuicationData(ESP_t *esp);
// static u8_t historyData(ESP_t *esp);
// static void historySave(void);

/* const funcationTab_t funTab[] = {
    {EYBOND_HEARTBEAT,  heartbeat},
    {EYBOND_GET_PARA,   paraGet},
    {EYBOND_SET_PATA,   paraSet},
    {EYBOND_TRANS,      trans},
    {EYBOND_GET_DEVICE_PARA, deviceDataGet},
    {EYBOND_GET_DEVICE_HISTORY, historyData},
    {EYBOND_REPORT_SPECIAL,         specialData_receive},
    {EYBOND_GET_COMMUNICATION, commnuicationData},
    {EYBOND_FILE_UPDATE,    Update_file},
    {EYBOND_SOFT_UPDATE,    Update_soft},
    {EYBOND_DEVICE_UPDATE,  Update_device},
    {EYBOND_DEVICE_UPDATE_STATE, Update_deviceState},
    {EYBOND_DEVICE_UPDATE_CANCEL, Update_deviceCancel},
    {EYBOND_UPDATE_INFO, Update_info},
    {EYBOND_UPDATE_DATA_SEND, Update_dataRcve},
    {EYBOND_UPDATE_DATA_STATE, Update_rcveState},
    {EYBOND_UPDATE_DATA_CHECK, Update_dataCheck},
    {EYBOND_UPDATE_EXIT, Update_exit},
}; */
const funcationTab_t funTab[] = {
//  {EYBOND_HEARTBEAT,  heartbeat},
  {EYBOND_GET_PARA,   paraGet},
  {EYBOND_SET_PATA,   paraSet},
  {EYBOND_TRANS,      devtrans},
  {EYBOND_GET_DEVICE_PARA, deviceDataGet},
  {EYBOND_REPORT_SPECIAL,  specialData_receive},
//  {EYBOND_GET_COMMUNICATION, commnuicationData},
};

#ifdef _PLATFORM_BC25_
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void proc_eybond_task(s32_t taskId) {
//  u8_t ret = 0;
//  u8 count = 0;
  ST_MSG msg;
  u32_t uStackSize = 0;
  relinkCnt = 0;
  sPort = 0xff;
  m_timeCheck_EYB = 0;
  list_init(&rcveList);

//  FlashFIFO_init(&historyHead, HISTORY_AREA_ADDR, HISTORY_AREA_SIZE);
//  FlashEquilibria_init(&saveSpace, HISTORY_SPACE_ADDR, HISTORY_SPACE_SIZE,sizeof(histprySaveSpace), &histprySaveSpace);
//  if (histprySaveSpace < 10 ||  histprySaveSpace > (60*8))
//  {
//    histprySaveSpace = 60*5;   //
//    FlashEquilibria_clear(&saveSpace);
//    FlashEquilibria_write(&saveSpace, &histprySaveSpace);
//  }
//  historySaveCnt = histprySaveSpace;
  int timeReport = 0;

  while (TRUE) {
    Ql_OS_GetMessage(&msg);
    switch (msg.message) {
      case APP_MSG_UART_READY:
        APP_PRINT("Eybond task run...\r\n");
        break;
      case SYS_PARA_CHANGE:
        break;
      case APP_MSG_DEVTIMER_ID: {
        Buffer_t buf;
        buf.lenght = 0;
        parametr_get(DEVICE_REPORT_TIME, &buf);   // mike 20200922
        if (buf.payload != null && buf.lenght > 0) {
          timeReport = Swap_charNum((char *)buf.payload);
        }
        if (timeReport == 0) {
          timeReport = 300;
        } 
        if (m_timeCheck_EYB == 10) {
          APP_DEBUG("Time for device report:%ld %ld!!\r\n", timeReport, m_timeCheck_EYB);
        }
        if (buf.payload != NULL) {
          Ql_MEM_Free(buf.payload);
          buf.payload = NULL;
          buf.lenght = 0;
          buf.size = 0;
        }
        m_timeCheck_EYB++;
        if (m_timeCheck_EYB >= timeReport * 1) {
          uStackSize = Ql_OS_GetCurrenTaskLeftStackSize();
          APP_DEBUG("eybond task stack size:%ld!!\r\n", uStackSize);
          m_timeCheck_EYB = 0;
//          onlineReport();
          Ql_OS_SendMessage(EYBOND_TASK, EYBOND_CMD_REPORT, 0, 0);
        }
      }
      break;
      case EYBOND_DATA_PROCESS:
        APP_DEBUG("Get EYBOND_DATA_PROCESS!!\r\n");
        ESP_process();
        break;
      case EYBOND_CMD_REPORT:
        onlineReport();
        break;
      default:
        break;
    }
  }
}
#endif

#ifdef _PLATFORM_L610_
void proc_eybond_task(s32_t taskId) {
  u32_t uStackSize = 0;
  relinkCnt = 0;
  sPort = 0xff;
  m_timeCheck_EYB = 0;
  list_init(&rcveList);

  int timeReport = 0;

  while (1) {
  }
}

#endif
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
void ESP_callback(u8_t port, Buffer_t *buf) { // 网络数据解析
//  overtime_ESP = 0;
  sPort = port;
  ESP_cmd(buf, NetMsgAck_Eybond);
//  memory_release(buf);
  if (buf->payload != NULL) {   // 释放申请的输入指令内存
    Ql_MEM_Free(buf->payload);
    buf->payload = NULL;
  }
}

static void NetMsgAck_Eybond(Buffer_t *buf) { // 网络数据解析返回结果函数
  if (buf != null && buf->payload != null && buf->lenght > 0) {
    APP_DEBUG("NetMsgAck_Eybond\r\n");
    output(buf);
    Net_send(sPort, buf->payload, buf->lenght);
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static void output(Buffer_t *buf) {
  u32_t displayNum = 0;
  APP_DEBUG("Eybond len: %d value: \r\n", buf->lenght);
  if (buf->lenght < 512) {
//    char *str = memory_apply(buf->lenght * 3 + 8);
    u8_t *str = Ql_MEM_Alloc(buf->lenght * 3 + 8);
    if (str != null) {
//      int l = Swap_hexChar((char *)str, buf->payload, buf->lenght, ' ');  // mike 重点检测函数
      hextostr(buf->payload, str, buf->lenght);   // 和Swap_hexChar是反的
      int l = r_strlen((char *)str);
      while (l) {
        if (l >= 16 * 3) {
          Debug_output(str + displayNum, 16 * 3);
//          Print_output(str + displayNum, 16 * 3);
          l -= 16 * 3;
          displayNum += 16 * 3;
        } else {
          Debug_output(str + displayNum, l);
//          Print_output(str + displayNum, l);
          l = 0;
        }
        Debug_output((u8_t *)"\r\n", 2);
//        Print_output((u8_t *)"\r\n", 2);
      }
//      Debug_output(str, l);
//      APP_DEBUG("\r\n");
//      memory_release(str);
      if (str != NULL) {
        Ql_MEM_Free(str);
      }
    }
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
void ESP_close(void) {
  list_delete(&rcveList);
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
u8_t ESP_cmd(Buffer_t *buf, AckCh ch) {
  u8_t e = 0;
  APP_DEBUG("ESP_cmd\r\n");
  output(buf);
  if (buf == null || buf->payload == null || ch == null) {
    e = 1;
  } else if (buf->lenght < sizeof(EybondHeader_t)) {
    e = 2;
  } else {
    ESP_t *esp = list_nodeApply(buf->lenght + sizeof(ESP_t) - sizeof(EybondHeader_t));  // 申请指令节点

    if (esp == null) {
      e = 3;
      log_save("ESP memory apply fail!!");
    } else {
      esp->PDULen = buf->lenght;
      esp->waitCnt = 0;
      esp->ack = ch;        // set NetMsgAck_Eybond，设置指令节点的返回函数
      // r_memcpy(&esp->head, buf->payload, buf->lenght);
      r_memcpy(&esp->head, buf->payload, buf->lenght); // 把指令放入列表节点的内存
      e = 0;
      list_topInsert(&rcveList, esp);   // 把指令插入列表节点
#ifdef _PLATFORM_BC25_
      Ql_OS_SendMessage(EYBOND_TASK, EYBOND_DATA_PROCESS, 0, 0);
#endif

#ifdef _PLATFORM_L610_
#endif
    }
  }

  return e;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
u8_t ESP_check(Buffer_t *buf) {
  APP_PRINT("ESP_check\r\n");
  EybondHeader_t *esp = (EybondHeader_t *)buf->payload;
  u16_t len = ENDIAN_BIG_LITTLE_16(esp->msgLen) + sizeof(EybondHeader_t) - 2;

  if (len != buf->lenght) {
    return 1;
  }

  return 0;
}


/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static u8_t espCmp(void *src, void *dest) {
  return r_memcmp(src, dest, sizeof(ESP_t));
}
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static void ESP_process(void) {
  ESP_t *esp = (ESP_t *)list_nextData(&rcveList, null); // 查找列表中的指令节点

  if (null == esp) {
    return;
  }

  APP_DEBUG("ESP=pdu len:%04X, waitCnt:%04X\r\n", esp->PDULen, esp->waitCnt);
  APP_DEBUG("ESP=head serail:%04X, code:%04X\r\n", esp->head.serial, esp->head.code);
  APP_DEBUG("ESP=head msg len:%04X, addr:%02X func:%02X\r\n", esp->head.msgLen, esp->head.addr, esp->head.func);

  while (esp != null && esp->waitCnt != 0) {
    if (++esp->waitCnt > ESP_WAIT_CNT) { //wait prcesso overtime
      list_nodeDelete(&rcveList, esp);
      return;
    }
    esp = (ESP_t *)list_nextData(&rcveList, esp);
  }

  if (esp != null) {
    overtime_ESP = 0;
    relinkCnt = 0;
    funcationTab_t *exe = (funcationTab_t *)ALG_binaryFind(esp->head.func, (void *)funTab,
                          (void *)(&funTab[sizeof(funTab) / sizeof(funTab[0])]), sizeof(funTab[0]));    // 找到执行对应指令的函数
    if (exe != null) {
      if (0 != exe->fun(esp)) { // 把指令节点传递具体的函数
        esp->waitCnt++;
        return;
      }
    }
  }
  list_nodeDelete(&rcveList, esp);  // 删除未执行的指令节点
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
/* static u8_t heartbeat(ESP_t *esp) {
#pragma pack(1)
  typedef struct {
    u8_t year;
    u8_t month;
    u8_t day;
    u8_t hour;
    u8_t min;
    u8_t secs;
  } rcve_t;
#pragma pack()

  rcve_t *para = (rcve_t *)(esp->PDU);
  EybondHeader_t *ack;
  Buffer_t buf;
  Clock_t time;

  time.year = para->year + 2000;
  time.month = para->month;
  time.day = para->day;
  time.hour = para->hour;
  time.min = para->min;
  time.secs = para->secs;

  Clock_Set(&time);

  // SysPara_Get(2, &buf);
  parametr_get(2, &buf);
  // ack = memory_apply(sizeof(EybondHeader_t) + 18);
  ack = Ql_MEM_Alloc(sizeof(EybondHeader_t) + 18);
  if (ack != null) {
  // r_memcpy(ack, &esp->head, sizeof(EybondHeader_t));
    Ql_memcpy(ack, &esp->head, sizeof(EybondHeader_t));
    if (buf.payload == null || (buf.lenght != 14 &&  buf.lenght != 18)) {
      APP_DEBUG("PN ERR: %d, %s\r\n", buf.lenght, buf.payload);
      buf.lenght = 14;
      Ql_memcpy(ack + 1, "EYBONDERR00000", 14);
  // SysPara_auth();
    } else {
  // r_memcpy(ack + 1, buf.payload,  buf.lenght);
      Ql_memcpy(ack + 1, buf.payload,  buf.lenght);
    }
  // memory_release(buf.payload);
    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
    }
    ack->msgLen = ENDIAN_BIG_LITTLE_16(buf.lenght + 2);
    buf.lenght = sizeof(EybondHeader_t) + buf.lenght;
    buf.payload = (u8_t *)ack;
    esp->ack(&buf);
  }
  // memory_release(buf.payload);
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }
  return 0;
} */

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static u8_t paraGet(ESP_t *esp) {
  u8_t i;
  u16_t len;
  Buffer_t *paraBufTab;
  Buffer_t *ackBufTab;
  Buffer_t buf;
  u8_t *ackBuf;
  EybondHeader_t *ackHead;

  u8_t *para = esp->PDU;

  i = esp->PDULen - sizeof(EybondHeader_t);

  // ackBufTab = memory_apply(i * sizeof(Buffer_t) + 8);
  ackBufTab = Ql_MEM_Alloc(i * sizeof(Buffer_t) + 8);
  ackBuf = (u8_t *)ackBufTab;
  paraBufTab = ackBufTab;
  len = 0;

  while (i--) {
    buf.lenght = 0;
    buf.payload = null;

    // SysPara_Get(*para, &buf);
    parametr_get(*para, &buf);
    len += buf.lenght;
    // r_memcpy(ackBuf, &buf, sizeof(Buffer_t));
    r_memcpy(ackBuf, &buf, sizeof(Buffer_t));
    ackBuf += sizeof(Buffer_t);
    para++;
  }

  i = esp->PDULen - sizeof(EybondHeader_t);

  buf.lenght = i * 2 + len + (i * sizeof(EybondHeader_t));
  // buf.payload = memory_apply(buf.lenght + 20);
  buf.payload = Ql_MEM_Alloc(buf.lenght + 20);
  ackBuf = buf.payload;
  para = esp->PDU;

  while (i--) {
    ackHead = (EybondHeader_t *)ackBuf;
    // r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    ackBuf += sizeof(EybondHeader_t);
    if (ackBufTab->lenght > 0 && ackBufTab->payload != null) {
      *ackBuf++ = 0;
      *ackBuf++ = *para++;
      // r_memcpy(ackBuf, ackBufTab->payload, ackBufTab->lenght);
      r_memcpy(ackBuf, ackBufTab->payload, ackBufTab->lenght);
      ackBuf += ackBufTab->lenght;
    } else {
      *ackBuf++ = 1;
      *ackBuf++ = *para++;
      ackBufTab->lenght = 0;
    }
    //byte alignment
    *(((u8_t *)ackHead) + 4) = (ackBufTab->lenght + 4) >> 8;
    *(((u8_t *)ackHead) + 5) = (ackBufTab->lenght + 4); //

    // ackHead->msgLen = ENDIAN_BIG_LITTLE_16(ackBufTab->lenght + 4);
    // memory_release(ackBufTab->payload);
    if (ackBufTab->payload != NULL) {
      Ql_MEM_Free(ackBufTab->payload);
      ackBufTab->payload = NULL;
    }
    ackBufTab++;
  }
  esp->ack(&buf);
//  memory_release(paraBufTab);
//  memory_release(buf.payload);
  if (paraBufTab != NULL) {
    Ql_MEM_Free(paraBufTab);
  }
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }
  return 0;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static u8_t paraSet(ESP_t *esp) {
  Buffer_t buf;
  int ret = 0, i = 0;

  EybondHeader_t *ackHead;
  u8_t *para = esp->PDU;

  APP_DEBUG("ESP=pdu len:%04X, waitCnt:%04X\r\n", esp->PDULen, esp->waitCnt);
  APP_DEBUG("ESP=head serail:%04X, code:%04X\r\n", esp->head.serial, esp->head.code);
  APP_DEBUG("ESP=head msg len:%04X, addr:%02X func:%02X\r\n", esp->head.msgLen, esp->head.addr, esp->head.func);

  buf.lenght = ENDIAN_BIG_LITTLE_16(esp->head.msgLen) - 3;
  for (i = 0; i < (buf.lenght + 1); i++) {
    APP_DEBUG("ESP=PDU %02X\r\n", esp->PDU[i]);
  }

  buf.payload = Ql_MEM_Alloc(buf.lenght + 1);
  buf.size = buf.lenght + 1;
  r_memcpy(buf.payload, para + 1, buf.lenght);
  buf.payload[buf.lenght] = '\0';
//  buf.payload = para + 1;
  APP_DEBUG("Set para: %d %s\r\n", *para, buf.payload);
  ret = parametr_set(*para, &buf);  // 设置参数

  if (buf.payload != NULL) {    // free old buf
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }

  ackHead = &esp->head;
  ackHead->msgLen = ENDIAN_BIG_LITTLE_16(4);
  para[1] = para[0];
  para[0] = ret;

  buf.lenght = sizeof(EybondHeader_t) + 2;
  buf.payload = Ql_MEM_Alloc(buf.lenght + 1);
  //  buf.payload = (u8_t *)ackHead;
  r_memcpy(buf.payload, (u8_t *)ackHead, buf.lenght);
  esp->ack(&buf);   // 设置指令成功、失败返回
  APP_DEBUG("Set para callback %d %s\r\n", buf.lenght, buf.payload);
  //  memory_release(buf.payload);
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }

  return 0;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static u8_t devtrans(ESP_t *esp) {
  Device_t *dev;
  DeviceCmd_t *cmd;

  APP_DEBUG("devtrans\r\n");

  dev = list_nodeApply(sizeof(Device_t));
  cmd = list_nodeApply(sizeof(DeviceCmd_t));

  cmd->waitTime = 1500;     // 1500=1.5 sec
  cmd->state = 0;
//  cmd->ack.size = 0;      // mike 20200926
//  cmd->ack.payload = null;  // 指令返回数据初始化
  cmd->ack.size = DEVICE_ACK_SIZE;
  cmd->ack.payload = Ql_MEM_Alloc(cmd->ack.size);
  cmd->ack.lenght = 0;
  cmd->cmd.size = ENDIAN_BIG_LITTLE_16(esp->head.msgLen) - 2;
  cmd->cmd.lenght = cmd->cmd.size;
  // cmd->cmd.payload = memory_apply(cmd->cmd.size);
  cmd->cmd.payload = Ql_MEM_Alloc(cmd->cmd.size);   // 重新申请存放输入指令的内存
  // r_memcpy(cmd->cmd.payload, esp->PDU, cmd->cmd.size);
  r_memcpy(cmd->cmd.payload, esp->PDU, cmd->cmd.size);

  output(&cmd->cmd);

  dev->cfg = null;  // 配置一个执行该指令的设备
  dev->callBack = devtransAck;  // 设置透传设备需要的返回函数
  dev->explain = esp;   // 设备携带指令节点
  dev->type = DEVICE_TRANS;

  list_init(&dev->cmdList);
  list_bottomInsert(&dev->cmdList, cmd);
  Device_inset(dev);    // 将需要执行指令的设备放入Devicelist，交由deviceCmdSend处理

  return 1;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static u8_t devtransAck(Device_t *dev) {
  ESP_t *esp;
  EybondHeader_t *ackHead;
  DeviceCmd_t *cmd;
  Buffer_t buf;
  APP_DEBUG("devtransAck\r\n");

  esp = list_find(&rcveList, espCmp, dev->explain); // 找到指令列表中的指令节点

  if (esp != null) {
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;

    buf.lenght = cmd->ack.lenght + sizeof(EybondHeader_t);
    // buf.payload = memory_apply(buf.lenght);
    buf.payload = Ql_MEM_Alloc(buf.lenght);

    ackHead = (EybondHeader_t *)buf.payload;
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    r_memcpy(ackHead + 1, cmd->ack.payload, cmd->ack.lenght);
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(cmd->ack.lenght + 2);
    esp->ack(&buf); // 执行NetMsgAck_Eybond
    // memory_release(buf.payload);
    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
    }
    list_nodeDelete(&rcveList, esp);
  }
  Device_remove(dev);

  return 0;
}

/*******************************************************************************
  * @brief
  * @note   2018.12.28 CGQ add,overTime, 防止设备采集模块无有效采集命令导致平台设备不上线。
  * @param  None
*******************************************************************************/
//TODO 数据过滤功能需要继续了解
static u8_t deviceDataGet(ESP_t *esp) {
  static u8_t overTime = 0;

  u8_t *ackBuf;
  Buffer_t buf;
  u8_t *endAddrH = null;
  u8_t *endAddrL = null;
  u16_t regAddr;
  EybondHeader_t *ackHead = null;
  Node_t *tail;
  DeviceOnlineHead_t *head;

  overTime++;
  buf.lenght = 0;
  buf.size = MAX_CMD_LEN;
  // buf.payload = memory_apply(buf.size);
  buf.payload = Ql_MEM_Alloc(buf.size);

  tail = onlineDeviceList.node;

  if (tail != null && buf.payload != null) {
    APP_DEBUG("enter the upload device data\r\n");
    do {
      Node_t *node;

      head = (DeviceOnlineHead_t *)tail->payload;
      node = head->cmdList.node;
      tail = tail->next;
      if (node != null) {
        CmdBuf_t *cmd;
        u8_t *para;
        u16_t loadLen = 0;
        u8_t cmdLen = 0;
        u8_t code = 0;
        u16_t updateFlag = 0;

        ackBuf = &buf.payload[buf.lenght];
        do {
          cmd = (CmdBuf_t *)node->payload;
          node = node->next;

          /* 检测数据是变化 */
          updateFlag++;

          if ((cmd->state & (1 << 0)) == 0) {
            cmd->state |= (1 << 0);
          }

          if (code != cmd->fun) {
            code = cmd->fun;
            if (ackHead != null) {
              r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
              ackHead->addr = head->addr;
              ackHead->code = ENDIAN_BIG_LITTLE_16(head->code);
              ackHead->msgLen = ENDIAN_BIG_LITTLE_16(loadLen + 4);
              para = (u8_t *)(ackHead + 1);
              *para++ = 0x01;
              *para++ = 0x02;
            }
            ackHead = (EybondHeader_t *)ackBuf;
            ackBuf += sizeof(EybondHeader_t) + 2;
            loadLen = 0;
          }
          if (((head->flag & 0x01) == 0 || ((head->flag >> 1) != 0 && ((cmdLen >> 4) == (head->flag >> 1))))
              && endAddrH != null && ((((*endAddrH << 8) | *endAddrL) + 1) == cmd->startAddr)) {
            *endAddrH = cmd->endAddr >> 8;
            *endAddrL = cmd->endAddr;
          } else {
            regAddr = ENDIAN_BIG_LITTLE_16(cmd->startAddr);
            r_memcpy(ackBuf, &regAddr, 2);
            ackBuf += 2;
            regAddr = ENDIAN_BIG_LITTLE_16(cmd->endAddr);
            r_memcpy(ackBuf, &regAddr, 2);
            endAddrH = ackBuf++;
            endAddrL = ackBuf++;
            loadLen += 4;
          }
          cmdLen = (cmd->endAddr - cmd->startAddr + 1) << 1; // User modbus 01 02 code
          r_memcpy(ackBuf, cmd->buf.payload, cmd->buf.lenght);
          r_memset(&ackBuf[cmd->buf.lenght], 0, cmdLen - cmd->buf.lenght);
          ackBuf += cmdLen;
          loadLen += cmdLen;
        } while (node != head->cmdList.node && head->cmdList.node != null);

        if (updateFlag != 0) {
          r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
          ackHead->addr = head->addr;
          ackHead->code = ENDIAN_BIG_LITTLE_16(head->code);
          ackHead->msgLen = ENDIAN_BIG_LITTLE_16(loadLen + 4);
          para = (u8_t *)(ackHead + 1);
          if (tail == onlineDeviceList.node) {
            *para++ = 0x00;
          } else {
            *para++ = 0x01;
          }
          if ((int)ackHead == (int)&buf.payload[buf.lenght]) {
            *para++ = 0x00;
          } else {
            *para++ = 0x01;
          }
          ackHead = null;
          buf.lenght += ackBuf - &buf.payload[buf.lenght];
        } else {
          r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
          ackHead->addr = head->addr;
          ackHead->code = ENDIAN_BIG_LITTLE_16(head->code);
          ackHead->msgLen = ENDIAN_BIG_LITTLE_16(4);
          para = (u8_t *)(ackHead + 1);
          if (tail == onlineDeviceList.node) {
            *para++ = 0x00;
          } else {
            *para++ = 0x01;
          }
          *para++ = 0;
          ackHead = null;
          buf.lenght = para - buf.payload;
        }

        if (buf.lenght > 800) {
          esp->ack(&buf);
          buf.lenght = 0;
        }
        overTime = 0;
      }
    } while (tail != onlineDeviceList.node && onlineDeviceList.node != null);
    // historySaveCnt = histprySaveSpace + 15;
  } else {
    u8_t *para;
    APP_DEBUG("buf malloc failed!!\r\n");
    buf.lenght = sizeof(EybondHeader_t) + 2;
    // buf.payload = memory_apply(buf.lenght);
    ackHead = (EybondHeader_t *)buf.payload;
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(4);
    para = (u8_t *)(ackHead + 1);
    para[0] = 2;
    para[1] = 0;
  }
  esp->ack(&buf);
  // memory_release(buf.payload);
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }
  if (overTime > 5) {
    log_save("No device online reboot!");
#ifdef _PLATFORM_BC25_
    Ql_OS_SendMessage(EYBDEVICE_TASK, DEVICE_RESTART_ID, 0, 0);
#endif
    overTime = 0;
  }

  return 0;
}
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
/* static u8_t historyData(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
    u8_t flag;
        u8_t spaceH;
        u8_t spaceL;
    }rcve_t;
#pragma pack()

    Buffer_t buf;
    EybondHeader_t *ackHead;
  rcve_t *para = (rcve_t*)(esp->PDU);

  if (histprySaveSpace != ((para->spaceH<<8) | para->spaceL))
  {
    histprySaveSpace = ((para->spaceH<<8) | para->spaceL);
    FlashEquilibria_write(&saveSpace, &histprySaveSpace);
  }

  buf.size = MAX_CMD_LEN;
  buf.payload = memory_apply(buf.size);
  ackHead = (EybondHeader_t *)buf.payload;
  while (buf.payload)
  {
        buf.lenght = FlashFIFO_get(&historyHead, &buf);

    if (buf.lenght)
    {
      if (crc16_standard(CRC_RTU, buf.payload, buf.lenght) == 0)
      {
        ackHead->serial = esp->head.serial;
        ackHead->func = esp->head.func;
        buf.payload[8] = 0x01;
        buf.lenght -= 2;

        break;
      }
    }
    else
    {
      r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
      ackHead->msgLen = ENDIAN_BIG_LITTLE_16(3);
      buf.payload[8] = 0x00;
      buf.lenght = sizeof(EybondHeader_t) + 1;
            break;
    }
  }
    esp->ack(&buf);
  memory_release(buf.payload);
    return 0;
} */

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
/* static void historySave(void)
{
#pragma pack(1)
    typedef struct
    {
    EybondHeader_t head;
    u8_t ret;
        u8_t year;
        u8_t month;
        u8_t day;
        u8_t hour;
        u8_t min;
        u8_t secs;
    }save_t;
#pragma pack()

    Buffer_t buf;
  u16_t regAddr;
    u8_t *endAddrH = null;
  u8_t *endAddrL = null;
    save_t *save = null;
  Node_t *tail;
    DeviceOnlineHead_t *head;
  Clock_t *time;

  if (onlineDeviceList.node == null)
  {
    return;
  }
  log_save("history save!");
  buf.lenght = 0;
    buf.size = MAX_CMD_LEN;
    buf.payload = memory_apply(buf.size);
    tail = onlineDeviceList.node;
    save = (save_t*)buf.payload;

    if (buf.payload != null)
    {
    historySaveCnt = histprySaveSpace;
    time = Clock_get();
    save->year = time->year - 2000;
    save->month = time->month;
    save->day = time->day;
    save->hour = time->hour;
    save->min = time->min;
    save->secs = time->secs;
        do {
            Node_t *node;

            head = (DeviceOnlineHead_t*)tail->payload;
            node = head->cmdList.node;
            tail = tail->next;
            if (node != null)
            {
                CmdBuf_t *cmd;
                u8_t *para;
        u8_t cmdLen;
                u8_t code = 0;
        u8_t *pCode = null;
                u16_t updateFlag = 0;

                para = (u8*)(save+1);
                do {
                    cmd = (CmdBuf_t*)node->payload;
                    node = node->next;
                    if ((cmd->state&(1<<0)) == 0)
                    {
                        cmd->state |= (1<<0);
                        updateFlag++;
                    }
                    if (code != cmd->fun)
                    {
            code = cmd->fun;
                        if (pCode == null)
                        {
              pCode = para;
                            *para++ = 0x02;
                        }
            else
            {
              *pCode = 0x00;
            }
                    }
                    if (((head->flag&0x01) == 0 || ((head->flag>>1) != 0 && ((cmdLen>>4) == (head->flag>>1))))
              && endAddrH != null && ((((*endAddrH<<8) | *endAddrL) + 1) == cmd->startAddr))
                    {
                        *endAddrH = cmd->endAddr>>8;
            *endAddrL = cmd->endAddr;
                    }
                    else
                    {
            regAddr = ENDIAN_BIG_LITTLE_16(cmd->startAddr);
                        r_memcpy(para, &regAddr, 2);
            para += 2;
            regAddr = ENDIAN_BIG_LITTLE_16(cmd->endAddr);
            r_memcpy(para, &regAddr, 2);
                        endAddrH = para++;
            endAddrL = para++;
                    }
          cmdLen = (cmd->endAddr - cmd->startAddr + 1)<<1; // User modbus 01 02 code
                    r_memcpy(para, cmd->buf.payload, cmd->buf.lenght);
          r_memset(&para[cmd->buf.lenght], 0, cmdLen - cmd->buf.lenght);
                    para += cmdLen;
                } while (node != head->cmdList.node && head->cmdList.node != null);

                if (updateFlag != 0)
                {
                    buf.lenght = para - buf.payload;
                    save->head.addr = head->addr;
                    save->head.code = ENDIAN_BIG_LITTLE_16(head->code);
                    save->head.msgLen = ENDIAN_BIG_LITTLE_16(buf.lenght - sizeof(EybondHeader_t) + 2);
                    regAddr = crc16_standard(CRC_RTU, buf.payload, buf.lenght);
                    *para++ = regAddr;
                    *para++ = regAddr>>8;
                    buf.lenght += 2;
                    FlashFIFO_put(&historyHead, &buf);
                }
            }
        }while (tail != onlineDeviceList.node && onlineDeviceList.node != null);
  }

    memory_release(buf.payload);
} */
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
/* void HistoryDataClean(void)
{
  FlashFIFO_clear(&historyHead);
} */

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static void onlineReport(void) {
  u16_t regAddr;
  static u16_t nIndex_MSG = 0;
  Buffer_t buf;
  Node_t *tail;
  DeviceOnlineHead_t *head;
  if (onlineDeviceList.node == null) {
    return;
  }

  APP_DEBUG("online device Report!\r\n");
  buf.lenght = 0;
  buf.size = MAX_CMD_LEN;
  buf.payload = Ql_MEM_Alloc(buf.size);
  tail = onlineDeviceList.node;

  if (buf.payload != null) {
    do {
      Node_t *node;
      EybondHeader_t ackHead;
      int nIndex = 0;
      u8_t code = 0;
      head = (DeviceOnlineHead_t *)tail->payload;
      APP_DEBUG("online device head: addr %02X flag %02X code %04X!\r\n", head->addr, head->flag, head->code);
      APP_DEBUG("online device head cmdList: count %04X rsv %04X!\r\n", head->cmdList.count, head->cmdList.rsv);
      node = head->cmdList.node;
      tail = tail->next;
      if (node != null) {
        CmdBuf_t *cmd;
        u16_t cmdLen = 0;
        u16_t updateFlag = 0;
        nIndex = 0;
        do {  // 将一个设备所有采集指令的返回数据拷贝到一起
          cmd = (CmdBuf_t *)node->payload;
          node = node->next;
//          APP_DEBUG("cmd state %02X update Flag %d\r\n",cmd->state, updateFlag);
          if ((cmd->state & (1 << 0)) == 0) {
            cmd->state |= (1 << 0);
            updateFlag++;
          }
          APP_DEBUG("cmd state %02X update Flag %d\r\n", cmd->state, updateFlag);
          if (nIndex_MSG >= 65534) {
            nIndex_MSG = 0;
          }

          if (code != cmd->fun) {
            code = cmd->fun;
            r_memset(&ackHead, 0, sizeof(EybondHeader_t));
            r_memset(buf.payload, 0, MAX_CMD_LEN);
            regAddr = ENDIAN_BIG_LITTLE_16(nIndex_MSG);
            r_memcpy(&ackHead.serial, &regAddr, 2);    // 信息序号，上电后从0开始，2字节
            regAddr = ENDIAN_BIG_LITTLE_16(head->code);
            r_memcpy(&ackHead.code, &regAddr, 2);      // 设备编码 2字节，使用14号配置的设备协议编码
            // 2字节: 逆变器上报数据长度 +   2(addr + func) + 2(状态码+寄存器类型) + 4(start + end)
            cmdLen = (cmd->endAddr - cmd->startAddr + 1) << 1;
            cmdLen = cmdLen + 2 + 2 + 4;
            regAddr = ENDIAN_BIG_LITTLE_16(cmdLen);
            r_memcpy(&ackHead.msgLen, &regAddr, 2);
            ackHead.addr = 1;       // 设备地址, 1字节
            ackHead.func = 0x32;    // 功能码，1字节
            r_memcpy(&buf.payload[nIndex], &ackHead, sizeof(EybondHeader_t));
            nIndex = sizeof(EybondHeader_t);
            if (tail != onlineDeviceList.node && onlineDeviceList.node != null) {
              buf.payload[nIndex] = 1;     // 状态码, 1字节
            } else {
              buf.payload[nIndex] = 0;
            }
            nIndex += 1;
            buf.payload[nIndex] = 0x00;     // 寄存器类型 1字节
            nIndex += 1;
//            APP_DEBUG("Init len %04X %04X\r\n", cmdLen, ackHead.msgLen);
          } else {
            cmdLen = cmdLen + ((cmd->endAddr - cmd->startAddr + 1) << 1) + 4;
            regAddr = ENDIAN_BIG_LITTLE_16(cmdLen);
            r_memcpy(&ackHead.msgLen, &regAddr, 2);
//            APP_DEBUG("%04X %04X\r\n", cmdLen, ackHead.msgLen);
          }

          regAddr = ENDIAN_BIG_LITTLE_16(cmd->startAddr);
          r_memcpy(&buf.payload[nIndex], &regAddr, 2);
          nIndex += 2;
          regAddr = ENDIAN_BIG_LITTLE_16(cmd->endAddr);
          r_memcpy(&buf.payload[nIndex], &regAddr, 2);
          nIndex += 2;

//          APP_DEBUG("%02X %02X, %04X %04X %04X %04X\r\n", cmd->state, cmd->fun, cmd->startAddr, cmd->endAddr, ((cmd->endAddr - cmd->startAddr + 1)<<1), cmd->crc);
          r_memcpy(&buf.payload[nIndex], cmd->buf.payload, cmd->buf.lenght);
          nIndex += cmd->buf.lenght;
          buf.lenght = nIndex;
//          output(&buf);
        } while (node != head->cmdList.node && head->cmdList.node != null);
        r_memcpy(&buf.payload[0], &ackHead, sizeof(EybondHeader_t));
        buf.lenght = nIndex;
        output(&buf);
        Net_send(sPort, buf.payload, buf.lenght);
        nIndex_MSG++;
        r_memset(&ackHead, 0, sizeof(EybondHeader_t));
        r_memset(buf.payload, 0, MAX_CMD_LEN);
//        regCRC = crc16_standard(CRC_RTU, cmd->buf.payload, cmd->buf.lenght);
//        APP_DEBUG("cmd CRC: %04X\r\n", regCRC);
//        Ql_memcpy(&buf.payload[nIndex], &regCRC, 2);
//        buf.lenght += 2;

        if (updateFlag != 0) {
//          buf.lenght = Ql_strlen((char *)buf.payload);
//          regAddr = crc16_standard(CRC_RTU, buf.payload, buf.lenght);
//          buf.lenght += 2;
//          APP_DEBUG("%s\r\n", buf.payload);
        }
//      Ql_memset(buf.payload, 0, cmdLen - cmd->buf.lenght);
      }
    } while (tail != onlineDeviceList.node && onlineDeviceList.node != null);
  }
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }
}

void specialData_send(void) {
  APP_DEBUG("specialdata report\r\n");

#pragma pack(1)
  typedef struct {
    u16_t serial;       //Serial number     报文编号
    u16_t code;         //device code       设备编码
    u16_t msgLen;       //meseger lenght    数据长度
    u8_t  addr;         //device addresss   设备地址
    u8_t  func;         //funcation code    功能码
    u8_t  PDU;          //user Data         数据区
  } RPT_t;
#pragma pack()

  RPT_t rpt;

  rpt.serial  =   ENDIAN_BIG_LITTLE_16(0xF001);   //报文编号  取值范围:[0xF001,F900]
  rpt.code    =   0xFFFF;                         //设备编码
  rpt.msgLen  =   ENDIAN_BIG_LITTLE_16(3);        //数据长度
  rpt.addr    =   0xFF;                           //设备地址
  rpt.func    =   0x16;                           //功能码
  rpt.PDU     =   0x01;                           //数据区    数采器电源中断

  Buffer_t buf;
  buf.size = 0;
  buf.lenght = 9;
  buf.payload = (u8_t *)&rpt;

  output((Buffer_t *)&buf);
  Net_send(sPort, (u8_t *)&rpt, 9);
}

extern u8_t g_ret_flag;

static u8_t specialData_receive(ESP_t *esp) {
  EybondHeader_t *head = &esp->head;
  u8_t send_buf[9] = {0xF0, 0x01, 0xFF, 0xFF, 0x00, 0x03, 0xFF, 0x16, 0x01};
  g_ret_flag = r_memcmp(head, send_buf, 9);
  APP_DEBUG("g_ret_flag = %d\r\n", g_ret_flag);
  return 0;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
/* static u8_t commnuicationData(ESP_t *esp) {
#pragma pack(1)
  typedef struct {
    u8_t state;
  } rcve_t;
#pragma pack()
  EybondHeader_t *ack;
  Buffer_t buf;
  u16_t code;
  rcve_t *para = (rcve_t *)(esp->PDU);

  Sineng_PVData(esp->head.addr, para->state, &code, &buf);
  if (buf.lenght == 0 || buf.payload == null) {
    ack = &esp->head;
    ack->msgLen = ENDIAN_BIG_LITTLE_16(3);
    para->state = 0x11;
    buf.lenght = sizeof(EybondHeader_t) + 1;
    buf.payload = (u8_t *)ack;
  } else {
//    ack = memory_apply(sizeof(EybondHeader_t) + buf.lenght);
    ack = Ql_MEM_Alloc(sizeof(EybondHeader_t) + buf.lenght);
    Ql_memcpy(ack, &esp->head, sizeof(EybondHeader_t));
    Ql_memcmp(ack + 1, buf.payload, buf.lenght);
    ack->code = code;
    ack->msgLen = buf.lenght + 2;
//    memory_release(buf.payload);
    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
    }
    buf.lenght += sizeof(EybondHeader_t);
    buf.payload = (u8_t *)ack;
  }

  esp->ack(&buf);
//    memory_release(buf.payload);
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.payload = NULL;
  }
  return 1;
} */

/******************************************************************************/
