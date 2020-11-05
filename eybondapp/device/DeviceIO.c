/**********************    COPYRIGHT 2014-2100,  Eybond ************************
 * @File    : DeviceIO.c
 * @Author  : CGQ
 * @Date    : 2017-12-05
 * @Brief   :
 ******************************************************************************/
#ifdef _PLATFORM_BC25_
#include "ql_uart.h"
#include "ql_timer.h"
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_error.h"

#include "ril_oceanconnect.h"
#endif

#ifdef _PLATFORM_L610_

#endif
// #include "eyblib_memory.h"       // mike 20200828
// #include "eyblib_r_stdlib.h"
#include "eybpub_utility.h"
#include "eybpub_Debug.h"
#include "eybpub_run_log.h"
#include "eybpub_SysPara_File.h"

#include "eybapp_appTask.h"
#include "BC25Net.h"
#include "DeviceIO.h"
#include "Device.h"

#include "Protocol.h"

static DeviceInfo_t *s_device;
static DeviceInfo_t *s_lockDevice;
static Buffer_t rcveBuf;

s8_t g_UARTIO_AT_enable = 0;

static void end(DeviceAck_e e);
static void overtimeCallback(u32 timerId, void *param);

#ifdef _PLATFORM_BC25_
static ST_UARTDCB *IOCfg = null;
static void UARTIOCallBack(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void *customizedPara);
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void DeviceIO_init(ST_UARTDCB *cfg) {
  s32_t ret = 0;

  if (cfg == null) { // Init device IO UART setting with default 9600 setting
    s_device = null;  // Clear old data
    s_lockDevice = null;
    IOCfg = null;
    rcveBuf.payload = null;

    ProtocolAttr_t attr;
    ST_UARTDCB SetCfg;
    if (protocolAttrGet(DEVICE_PROTOCOL, &attr) != 0) { // 获取默认配置的采集口UART设置
      SetCfg = UART_9600_N1;
    } else {
      if (attr.cfg.baudrate == 0) {
        APP_DEBUG("DEVICE_PROTOCAL UART config is NULL\r\n");
        SetCfg.baudrate = 9600;
        SetCfg.dataBits = DB_8BIT;
        SetCfg.stopBits = SB_ONE;
        SetCfg.parity = PB_NONE;
        SetCfg.flowCtrl = FC_NONE;
      } else {
        SetCfg.baudrate = attr.cfg.baudrate;
        SetCfg.dataBits = attr.cfg.dataBits;
        SetCfg.stopBits = attr.cfg.stopBits;
        SetCfg.parity = attr.cfg.parity;
        SetCfg.flowCtrl = attr.cfg.flowCtrl;
      }
    }
    APP_DEBUG("#%d-%d-%d-%d#\r\n", SetCfg.baudrate, SetCfg.dataBits, SetCfg.stopBits, SetCfg.parity);

    ret = Ql_Timer_Register(DEVICE_OVERTIME_ID, overtimeCallback, null);   // mike 重复注册定时器?
    if (ret < QL_RET_OK) {
      APP_DEBUG("Register DEVICE Overtime timer(%d) failed!! ret = %d\r\n", DEVICE_OVERTIME_ID, ret);
    } else {
      APP_DEBUG("Register DEVICE Overtime timer(%d) successfully!! ret = %d\r\n", DEVICE_OVERTIME_ID, ret);
    }

    ret = Ql_UART_Register(DEVICE_IO_PORT, UARTIOCallBack,  NULL);
    if (ret < QL_RET_OK) {
      APP_DEBUG("Register DEVICE UART failed!!port:%d ret:%d\r\n", DEVICE_IO_PORT, ret);
    } else {
      APP_DEBUG("Register DEVICE UART successfully!!port:%d\r\n", DEVICE_IO_PORT);
    }

    ret = Ql_UART_OpenEx(DEVICE_IO_PORT, &SetCfg);
    if (ret < QL_RET_OK) {
      APP_DEBUG("Open DEVICE UART failed!!port:%d ret:%d\r\n", DEVICE_IO_PORT, ret);
    } else {
      APP_DEBUG("Open DEVICE UART successfully!!port:%d\r\n", DEVICE_IO_PORT);
    }
  } else if (IOCfg == null || Ql_memcmp(IOCfg, cfg, sizeof(ST_UARTDCB)) != 0) {
    Ql_UART_Close(DEVICE_IO_PORT);
    IOCfg = cfg;  // Init with input setting
    APP_DEBUG("input #%d-%d-%d-%d#\r\n", cfg->baudrate, cfg->dataBits, cfg->stopBits, cfg->parity);
    APP_DEBUG("setting #%d-%d-%d-%d#\r\n", IOCfg->baudrate, IOCfg->dataBits, IOCfg->stopBits, IOCfg->parity);

    ret = Ql_UART_OpenEx(DEVICE_IO_PORT, IOCfg);
    if (ret < QL_RET_OK) {
      APP_DEBUG("Open DEVICE UART failed!!port:%d ret:%d\r\n", DEVICE_IO_PORT, ret);
    } else {
      APP_DEBUG("Open DEVICE UART successfully!!port:%d\r\n", DEVICE_IO_PORT);
    }
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
ST_UARTDCB *DeviceIO_cfgGet(void) {
  return IOCfg;
}

/*******************************************************************************
 Brief    : void
 Parameter:
 return   :
*******************************************************************************/
static void UARTIOCallBack(Enum_SerialPort port, Enum_UARTEventType msg, bool level, void *customizedPara) {
  switch (msg) {
    case EVENT_UART_READY_TO_READ: {
      if (port == DEVICE_IO_PORT) {
        if (rcveBuf.payload != null) {    // 设备串口接收的数据统一在这里释放内存
          Ql_MEM_Free(rcveBuf.payload);
          rcveBuf.payload = NULL;
          rcveBuf.lenght = 0;
          rcveBuf.size = 0;
        }
        rcveBuf.payload = Ql_MEM_Alloc(SERIAL_RX_BUFFER_LEN);
        rcveBuf.size = SERIAL_RX_BUFFER_LEN;
        rcveBuf.lenght =  Ql_UART_Read(port, rcveBuf.payload,  rcveBuf.size);
        Ql_UART_ClrRxBuffer(DEVICE_IO_PORT);
        APP_DEBUG("rcveBuf len:%d size:%d!!\r\n", rcveBuf.lenght, rcveBuf.size);
        if (rcveBuf.lenght != 0) {
//          APP_DEBUG("rcveBuf :%s size:%d!!\r\n", rcveBuf.payload);
        }
        if(Ql_strncmp(CUSTOMER, "0A5", 3) == 0) {
          if (Ql_strncmp((char *)rcveBuf.payload, "AT+", 3) == 0
              && g_UARTIO_AT_enable == 0) { // 获取到硕日私有AT查询指令
            char strTemp[32] = {0};
            Ql_memset(strTemp, '\0', 32);
            Buffer_t srne_buf;
            srne_buf.lenght = 0;
            srne_buf.size = 0;
            srne_buf.payload = NULL;
            if (Ql_strncmp((char *)rcveBuf.payload, "AT+PN", 5) == 0) {
              parametr_get(DEVICE_PNID, &srne_buf);
              Ql_strcpy(strTemp, "PN:");
            } else if (Ql_strncmp((char *)rcveBuf.payload, "AT+IMEI", 7) == 0) {
              parametr_get(DEVICE_UID_IMEI, &srne_buf);
              Ql_strcpy(strTemp, "IMEI:");
            } else if (Ql_strncmp((char *)rcveBuf.payload, "AT+CCID", 7) == 0) {
              parametr_get(GPRS_CCID_ADDR, &srne_buf);
              Ql_strcpy(strTemp, "CCID:");
            } else if (Ql_strncmp((char *)rcveBuf.payload, "AT+CSQ", 6) == 0) {
              parametr_get(GPRS_CSQ_VALUE, &srne_buf);
              Ql_strcpy(strTemp, "CSQ:");
            } else if (Ql_strncmp((char *)rcveBuf.payload, "AT+LINK", 7) == 0) {
              if (BC25Net_status() == REGISTERED_AND_OBSERVED) {
                Uart_write((u8_t *)"LINK:1\r\n", 8);
              } else {
                Uart_write((u8_t *)"LINK:0\r\n", 8);
              }
              return;
            }
            if (srne_buf.payload != NULL) {
              if (srne_buf.lenght >= 25) {
                Ql_strncat(strTemp, (char *)srne_buf.payload, 25);
                Ql_strcat(strTemp, "\r\n");
              } else {
                Ql_strcat(strTemp, (char *)srne_buf.payload);
                Ql_strcat(strTemp, "\r\n");
              }
              Uart_write((u8_t *)strTemp, Ql_strlen(strTemp));
              Ql_MEM_Free(srne_buf.payload);
              srne_buf.payload = NULL;
              return;
            }
            return;
          }
        }
        APP_DEBUG("no AT rcveBuf len:%d size:%d!!\r\n", rcveBuf.lenght, rcveBuf.size);
        if (g_UARTIO_AT_enable == 0) {  // 过滤主串口的主动输入
          if (Ql_strncmp((char *)rcveBuf.payload, "SET_TEST=ON", 11) == 0) { // 使能主串口的主动输入
            APP_DEBUG("Enable MSG from UART port:%d!!\r\n", port);
            g_UARTIO_AT_enable = 1;
            Uart_write((u8_t *)"TEST=ON,OK\r\n", 12);
            return;
          }
          if (s_device == null || s_device->buf->payload == null) {
            APP_DEBUG("Cancel MSG %s from UART port:%d!!\r\n", rcveBuf.payload,
                      port);  // 默认忽略处理没有负载时采集口上报的数据
            return;
          } else {  // 有负载时将接收到的数据传递给负载设备处理
            s_device->buf->lenght = rcveBuf.lenght;
            Ql_memcpy(s_device->buf->payload, rcveBuf.payload, s_device->buf->lenght);
            APP_DEBUG("s_device buf len:%d size:%d!!\r\n", s_device->buf->lenght, s_device->buf->size);
          }
        } else {  // 生产测试AT指令打开后
          if (Ql_strncmp((char *)rcveBuf.payload, "SET_TEST=OFF", 12) == 0) {
            APP_DEBUG("Disable MSG from UART port:%d!!\r\n", port);
            g_UARTIO_AT_enable = 0;
            Uart_write((u8_t *)"TEST=OFF,OK\r\n", 13);
            return;
          }
          if (Ql_strncmp((char *)rcveBuf.payload, "SET_TEST=ON", 11) == 0) {
            APP_DEBUG("MSG from UART port is Enabled:%d!!\r\n", port);
            Uart_write((u8_t *)"TEST=ON,OK\r\n", 12);
            return;
          }
          if (Ql_strncmp((char *)rcveBuf.payload, "#Get", 4) == 0) {
            APP_DEBUG("Cancel MSG %s from UART port:%d!!\r\n", rcveBuf.payload, port);
            return;
          }
        }
      }
      end(DEVICE_ACK_FINISH);
    }
    break;
    case EVENT_UART_READY_TO_WRITE:
      break;
    default:
      break;
  }
}

/*******************************************************************************
  * @brief
  * @param  None
  * @retval None
*******************************************************************************/
void DeviceIO_reset(void) {
  Ql_UART_Close(DEVICE_IO_PORT);
  s32_t ret = 0;
  ret = Ql_Timer_Stop(DEVICE_OVERTIME_ID);    // mike 20200910
  if (ret != QL_RET_OK) {
    APP_DEBUG("Stop OVERTIME error!! ret:%d\r\n", ret);
  } else {
    APP_DEBUG("Stop OVERTIME Success!!\r\n");
  }
  s_lockDevice = null;
  IOCfg = null;
  if (rcveBuf.payload != NULL) {    // mike 20200828
    Ql_MEM_Free(rcveBuf.payload);
    rcveBuf.payload = null;
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
DeviceAck_e DeviceIO_write(DeviceInfo_t *hard, u8_t *pData, mcu_t lenght) {
  DeviceAck_e result = DEVICE_ACK_FINISH;
  s32_t ret = 0;
  if (g_UARTIO_AT_enable == 1) {    // SET_TEST=ON时，屏蔽采集指令
    APP_DEBUG("AT TEST from UARTIO: %d!!\r\n", g_UARTIO_AT_enable);
    return DEVICE_ACK_FINISH;
  }
  if (hard == null || hard->buf->payload == null) {
    result = DEVICE_ACK_PRAR_ERR;
  } else if (s_device == null || hard == s_device) {
    if (lenght == 0 || pData == null) {
      result = DEVICE_ACK_PRAR_ERR;
    } else { //
      int i = 0;
      i = Ql_UART_Write(DEVICE_IO_PORT, pData, lenght);
      if (i == lenght) {
        APP_DEBUG("Uart send success: %d!!\r\n", i);
        s_device = hard;
        s_device->buf->lenght = 0;
        ret = Ql_Timer_Start(DEVICE_OVERTIME_ID, hard->waitTime, TRUE);
        if (ret < 0) {
          log_save("Start DEVICE_OVERTIME_ID timer failed, ret = %d", ret);
        }
        result = DEVICE_ACK_FINISH;
      } else {
        // log_saveAbnormal("Uart send fail !!", i);     // mike 20200824
        APP_DEBUG("Uart send fail: %d!!\r\n", i);
        log_save("Uart send fail: %d!!", i);
        result = DEVICE_ACK_HARD_FAULT;
      }
    }
  } else if (s_lockDevice != null) {
    result = DEVICE_ACK_LOCK;
  } else {
    result = DEVICE_ACK_BUSY;
  }

  return result;
}

void Uart_write(u8_t *data, u16_t len) {
  if (len > 0 && data != null) {
    Ql_UART_Write(DEVICE_IO_PORT, data, len);
  }
}

/*******************************************************************************
  * @brief  device opt end
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static void end(DeviceAck_e e) {
  s32_t ret = 0;
  ret = Ql_Timer_Stop(DEVICE_OVERTIME_ID);  // mike 20200829
  if (ret != QL_RET_OK) {
    APP_DEBUG("Stop OVERTIME error!! ret:%d\r\n", ret);
  } else {
//  APP_DEBUG("Stop OVERTIME Success!!\r\n");
  }

  if (null != s_device && s_device->callback != null) {
    APP_DEBUG("device callback!!\r\n");
    s_device->callback(e);  // 执行设备列表中的callback函数
  } else if (e != DEVICE_ACK_OVERTIME) {
    Ql_OS_SendMessage(EYBAPP_TASK, DEVICE_IO_ID, (u32)&rcveBuf, (u32)((void *)wrtie));
  }
  s_device = s_lockDevice;
}
#endif

#ifdef _PLATFORM_L610_
static hal_uart_config_t *IOCfg = null;
static void UARTIOCallBack(hal_uart_port_t uart_port, UINT8 *data, UINT16 len, void *arg);
void DeviceIO_init(hal_uart_config_t *cfg) {
}
hal_uart_config_t *DeviceIO_cfgGet(void) {
}
static void UARTIOCallBack(hal_uart_port_t uart_port, UINT8 *data, UINT16 len, void *arg) {
}
void DeviceIO_reset(void) {
}
DeviceAck_e DeviceIO_write(DeviceInfo_t *hard, u8_t *pData, mcu_t lenght) {
}
void Uart_write(u8_t *data, u16_t len) {
}
static void end(DeviceAck_e e) {
}
#endif

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void  DeviceIO_lock(DeviceInfo_t *hard) {
  s_lockDevice = hard;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
DeviceInfo_t *DeviceIO_lockState(void) {
  return s_lockDevice;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void  DeviceIO_unlock(void) {
  s_lockDevice = null;
  s_device = null;
}

/*******************************************************************************
  * @brief  overtime callback
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static void overtimeCallback(u32_t timerId, void *param) {
  APP_DEBUG("overtimeCallback!!\r\n");
  end(DEVICE_ACK_OVERTIME);
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static void wrtie(Buffer_t *buf) {
  APP_DEBUG("Device Uart callback: %s len:%d!!\r\n", buf->payload, buf->lenght);
//  APP_DEBUG("rcveBuf: %s len:%d!!\r\n", rcveBuf.payload, rcveBuf.lenght);
  Uart_write(buf->payload, buf->lenght);
}

/*********************************FILE END*************************************/

