/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : Device.c
  *@notes   : 2017.12.05 CGQ
*******************************************************************************/
#ifdef _PLATFORM_BC25_
#include "ql_system.h"
#include "ql_stdlib.h"
#include "ql_timer.h"
#include "ql_memory.h"
#include "ql_error.h"
#endif

#include "eyblib_list.h"
#include "eyblib_swap.h"
// #include "eyblib_memory.h"  // mike 20200828

#include "eybpub_Debug.h"
#include "eybpub_Status.h"
#include "eybpub_watchdog.h"
#include "eybpub_SysPara_File.h"
#include "eybpub_run_log.h"
#include "eybapp_appTask.h"

#include "NB_Net.h"
#include "Device.h"
#include "DeviceIO.h"
#include "protocol.h"
// #include "selfUpdate.h"
// #include "deviceUpdate.h"
// #include "UpdateTask.h"
#include "Sineng.h"
#include "clock.h"
//#include "CommonServer.h"     //mike 20200804

#define DEVICE_LOCK     (0x5A)
#define DEVICE_UNLOCK   (0x00)

ListHandler_t DeviceList;  //

static DeviceCmd_t *currentCmd;     //
static u8_t currentStep;            //
static u16_t DeviceOvertime;        // device overtime cnt
static int watiTime;
static u32_t m_timeCheck_DEV = 0;
static u8_t device_cmp(void *src, void *dest);
static void device_callback(DeviceAck_e ack);
static void deviceCmdSend(void);
//static void deviceLEDInit(void);
//static void deviceLEDOn(void);
//static void deviceLEDOff(void);

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
#ifdef _PLATFORM_BC25_
void proc_device_task(s32 taskId) {
  ST_MSG msg;
  int deviceResetCnt = 0;
  u32_t uStackSize = 0;

  deviceResetCnt = 0;
  currentStep = 0;
  DeviceOvertime = 0;
  watiTime = 5;   // wait 5sec start get device date
  m_timeCheck_DEV = 0;

  while (TRUE) {
    Ql_OS_GetMessage(&msg);
    switch (msg.message) {
      case APP_MSG_UART_READY:  // DEBUG串口OK
        APP_DEBUG("Get APP_CMD_UART_READY MSG\r\n");
        APP_PRINT("Devce task run...\r\n");
        list_init(&DeviceList);
//        DeviceIO_init(null);
//        Protocol_init();
        break;
      case NET_MSG_RIL_READY:
        APP_DEBUG("Get NET_MSG_RIL_READY MSG\r\n");
        DeviceIO_init(null);
        Protocol_init();
        break;
      case NET_MSG_RIL_FAIL:
        break;
      case NET_MSG_SIM_READY:
        APP_DEBUG("Get NET_MSG_SIM_READY MSG\r\n");
        break;
      case NET_MSG_SIM_FAIL:
        break;
      case NET_MSG_GSM_READY:    // 注网OK
        APP_DEBUG("Get NET_MSG_GSM_READY MSG\r\n");
        break;
      case NET_MSG_GSM_FAIL:    // 注网FAIL
        break;
      case NET_MSG_NET_READY:   // 服务器OK
        APP_DEBUG("Get NET_MSG_NET_READY MSG\r\n");
        break;
      case NET_MSG_NET_FAIL:    // 服务器FAIL
        break;
      case DEVICE_RESTART_ID:
        APP_DEBUG("deviceResetCnt %d!!\r\n", deviceResetCnt);
        /*        if (0 != runTimeCheck(4, 19)) { // 路灯是晚上工作的，所以白天没有数据判断是正常的
                  deviceResetCnt = 0;
                  DeviceOvertime = 0;
                  watiTime = 10;
                  break;
                } else if (++deviceResetCnt > 12) {
                  APP_DEBUG("Device no command ack reboot!!\r\n");
                  log_save("Device no command ack reboot!!");
                  Watchdog_stop();
                  Ql_Reset(0);  // mike 20200909
                  break;
                } else {
                  log_save("Device modular reset!!");
                  APP_DEBUG("Device modular reset!!\r\n");
                  msg.param1 = DEVICE_MONITOR_NUM;
                } */
        if (++deviceResetCnt >= 12) {
          if (0 == runTimeCheck(4, 19)) {
            APP_PRINT("Time is from 4 to 19, not reboot!!\r\n");
            deviceResetCnt = 0;
            DeviceOvertime = 0;
            watiTime = 10;
            break;
          }
          APP_DEBUG("Device no command ack, reboot!!\r\n");
          log_save("Device no command ack, reboot!!");
          Watchdog_stop();
          Ql_Reset(0);  // mike 20200909
          break;
        } else {
          log_save("Device modular reset!!");
          APP_DEBUG("Device modular reset!!\r\n");
          msg.param1 = DEVICE_MONITOR_NUM;
        }
        DeviceOvertime = 0;
      case SYS_PARA_CHANGE:   //system parameter change
        if (g_UARTIO_AT_enable == 1) {
          APP_DEBUG("Device IO UART AT mode is enable, don't reset device!!\r\n");
          break;
        }
//        if (msg.param1 == DEVICE_MONITOR_NUM || msg.param1 == DEVICE_PROTOCOL || msg.param1 == METER_PROTOCOL // mike 20201029
          if (msg.param1 == DEVICE_MONITOR_NUM || msg.param1 == DEVICE_PROTOCOL
            || msg.param1 == DEVICE_UART_SETTING || msg.param1 == DEVICE_SYSTEM) {
          DeviceIO_reset();
          Protocol_clean();
          Device_clear();
//          memory_trans(Debug_output);   //mike 20200805
//          CommonServer_DeviceInit();    //mike 20200805
          Protocol_init();
          deviceLEDOff();
        }
        watiTime = 10;
        break;
      case APP_MSG_DEVTIMER_ID:     //mike 20200915
        m_timeCheck_DEV++;
        if (m_timeCheck_DEV >= 5) {
          uStackSize = Ql_OS_GetCurrenTaskLeftStackSize();
          APP_DEBUG("Device task stack size:%ld!!\r\n", uStackSize);
          m_timeCheck_DEV = 0;
        }
//        APP_DEBUG("Get APP_MSG_DEVTIMER_ID MSG,watiTime:%04X step:%d\r\n", watiTime, currentStep);
        if ((watiTime & 0x8000) == 0x8000 || --watiTime > 0) {
          break;
        }
      case DEVICE_CMD_ID:
        watiTime = 0x8000;
        deviceCmdSend();
        break;
//    case DEVICE_UPDATE_READY_ID: //Device update    //mike 20200804
//      Update_ready();
//      break;
      case DEVICE_PV_SCAN_ID:    //Device PV scan
        PV_Scan();
        break;
      case DEVICE_PV_GET_ID:   //Device PV Data Get
        PV_dataGet();
        break;
      default:
        break;
    }
  }
}
#endif
#ifdef _PLATFORM_L610_
void proc_device_task(s32_t taskId) {
}
#endif
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static u8_t deviceClear(void *payload, void *point) {
  DeviceCmd_clear((Device_t *)payload);
  return 1;
}
void Device_clear(void) {
  list_trans(&DeviceList, deviceClear, null);
  list_delete(&DeviceList);
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static u8_t deviceCmdClear(void *payload, void *point) {
  DeviceCmd_t *cmd = (DeviceCmd_t *)payload;
//  APP_DEBUG("deviceCmdClear\r\n");
  if (cmd->ack.payload != NULL) {
    Ql_MEM_Free(cmd->ack.payload);
    cmd->ack.payload = NULL;
  }

  if (cmd->cmd.payload != NULL) {
    Ql_MEM_Free(cmd->cmd.payload);
    cmd->cmd.payload = NULL;
  }
  return 1;
}

void DeviceCmd_clear(Device_t *dev) {
//  APP_DEBUG("DeviceCmd_clear\r\n");
  list_trans(&dev->cmdList, deviceCmdClear, null);
  list_delete(&dev->cmdList);
//  memory_release(dev->explain);       // mike 20200828
//  if (dev->explain != NULL) {         // mike 20200926
//    Ql_MEM_Free(dev->explain);
//    dev->explain = NULL;
//  }
  currentStep = 0;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void Device_add(Device_t *device) {
//  APP_DEBUG("Device_add\r\n");
  list_bottomInsert(&DeviceList, device);
  device->lock = DEVICE_UNLOCK;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void Device_inset(Device_t *device) {
//  APP_DEBUG("Device_inset\r\n");
  list_topInsert(&DeviceList, device);
  device->lock = DEVICE_UNLOCK;
  currentStep = 0;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void Device_remove(Device_t *device) {
  APP_DEBUG("Device_remove\r\n");
  Device_t *dev = list_find(&DeviceList, device_cmp, device);

  if (dev != null) {
    DeviceCmd_clear(dev);     // mike 20200915 打开透传会死机 与deviceClear冲突
    list_nodeDelete(&DeviceList, dev);
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static u8_t deviceTypeRemove(void *payload, void *point) {
  Device_t *dev = (Device_t *)payload;

  if (dev->type == *(u16_t *)point) {
    DeviceCmd_clear(dev);
    list_nodeDelete(&DeviceList, dev);
  }
  return 1;
}
void Device_removeType(u16_t type) {
  list_trans(&DeviceList, deviceTypeRemove, &type);
}

/*******************************************************************************
  * @brief  device_cmp
  * @note   None
  * @param  None
  * @retval
*******************************************************************************/
static u8_t device_cmp(void *src, void *dest) {
  Device_t *pSrc = (Device_t *)src;
  Device_t *pDest = (Device_t *)dest;

  return  Ql_memcmp(&pSrc->callBack, &pDest->callBack, (int)(pSrc + 1) - (int)(&pSrc->callBack));
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
DeviceInfo_t deviceInfo;
static void deviceCmdSend(void) {
//  static DeviceInfo_t deviceInfo;
  static Device_t *currentDevice;     //

//  APP_DEBUG("Device Step %d waitTime is %04X!!\r\n", currentStep, watiTime);
  switch (currentStep) {
    case 0:
      currentDevice = null;
      currentCmd = null;
      deviceInfo.callback = device_callback;
    case 1:
      currentDevice = list_nextData(&DeviceList, currentDevice);    // 定时获取列表中需要执行指令的设备节点
      if (currentDevice == null || currentDevice->lock == DEVICE_LOCK) {
        if (++DeviceOvertime >= 300) {
          APP_DEBUG("Device no command in 5 min, reset!!\r\n");
          log_save("Device no command in 5 min, reset!!");
          Ql_OS_SendMessage(EYBDEVICE_TASK, DEVICE_RESTART_ID, 0, 0);
        }
        APP_DEBUG("currentDevice is null or DEVICE_LOCK %d!!\r\n", DeviceOvertime);
        watiTime = 1;
      } else if (DeviceIO_lockState() == null) {
        APP_DEBUG("DeviceIO_lockState is null!!\r\n");
        currentStep = 3;
        currentCmd = null;
        if (currentDevice->cfg != null) {
          APP_DEBUG("currentDevice->cfg is %d!!\r\n", currentDevice->cfg->baudrate);
          DeviceIO_init(currentDevice->cfg);
          watiTime = 1;
        } else if (DeviceIO_cfgGet() == null) {
          APP_DEBUG("Uart No Init!!\r\n");
          log_save("Uart No Init!!");
          currentStep = 1;
        }
      } else {
        APP_DEBUG("Device output is lock!!\r\n");   //mike 20200817
        log_save("Device output is lock!!");
        watiTime = 10;
      }
      break;
    case 2:
      currentDevice->lock = DEVICE_LOCK;
      if (currentDevice->callBack == null
          || currentDevice->callBack(currentDevice) == 0) { // call ackProcess
        APP_DEBUG("Step from 2 to 1!!\r\n");
        currentStep = 1;
      } else {
        APP_DEBUG("Step from 2 to 3!!\r\n");
        currentStep  = 3; //设备数据再次更新，增快数据上线速度 CGQ 2019.04.30
      }

      break;
    case 3:
      currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);  // 找到当前执行设备需要执行的指令
      if (currentCmd == null) {
        currentStep--;
        break;
      }
      currentStep++;
      watiTime = 0;
      break;
    case 4:
      if (null != currentCmd) {
        DeviceAck_e e;
        deviceInfo.waitTime = currentCmd->waitTime;
        deviceInfo.buf = &currentCmd->ack;    // mike 20200926
//        APP_DEBUG("Device buf size:%d\r\n", deviceInfo.buf->size);
        /*        if (deviceInfo.buf->size == 0) {
                  deviceInfo.buf->size = DEVICE_ACK_SIZE;
                  deviceInfo.buf->payload = Ql_MEM_Alloc(DEVICE_ACK_SIZE);
                  currentCmd->ack.size = deviceInfo.buf->size;
                  currentCmd->ack.payload = deviceInfo.buf->payload;
                } */
        APP_DEBUG("Device buf size:%d\r\n", deviceInfo.buf->size);
//        APP_DEBUG("Device waittime:%d\r\n", deviceInfo.waitTime);
//        APP_DEBUG("Device buf length:%d\r\n", deviceInfo.buf->lenght);
//        APP_DEBUG("Device buf:%s\r\n", deviceInfo.buf->payload);

        e = DeviceIO_write(&deviceInfo, currentCmd->cmd.payload, currentCmd->cmd.lenght);   // 把指令写到串口
        watiTime = 10;
        if (DEVICE_ACK_FINISH != e) {
          APP_DEBUG("Device command send fail %02X\r\n", e);
//          log_saveAbnormal("Device command send fail ", e);     //mike 20200824
          log_save("Device command send fail %02X", e);
          if (e == DEVICE_ACK_HARD_FAULT) {
            DeviceIO_reset();
          }
          currentStep = 1;
        }
//        else {
//          APP_DEBUG("Device command send finish %02X\r\n", e);
//        }

        if (++DeviceOvertime > 180) {
          Ql_OS_SendMessage(EYBDEVICE_TASK, DEVICE_RESTART_ID, 0, 0);
        }
      }
      break;
    default:
      currentStep = 0;
      break;
  }

//  APP_DEBUG("watiTime %04X\r\n", watiTime);
  if (watiTime == 0x8000) {
    Ql_OS_SendMessage(EYBDEVICE_TASK, DEVICE_CMD_ID, 0, 0);
  }
}

/*******************************************************************************
  * @brief
  * @param  None
  * @retval None
*******************************************************************************/
static void device_callback(DeviceAck_e ack) {
  APP_DEBUG("ack %04X Step: %d state:%d \r\n", ack, currentStep, currentCmd->state);
  if (ack == DEVICE_ACK_OVERTIME) {
    deviceLEDOff();
#if TEST_RF
    if (++DeviceOvertime > 180) {
      Ql_OS_SendMessage(EYBDEVICE_TASK, DEVICE_RESTART_ID, 0, 0);
      return;
    }
#endif
    if (currentCmd->state != 0) {
      currentStep--;
    }
  } else {
    currentStep--;
    DeviceOvertime = 0;
    deviceLEDOn();
  }
  currentCmd->state = ack;
  Ql_OS_SendMessage(EYBDEVICE_TASK, DEVICE_CMD_ID, 0, 0);
}
/******************************************************************************/