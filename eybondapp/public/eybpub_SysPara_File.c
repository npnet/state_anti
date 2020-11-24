/*******************************************************************************
 * @Author       : Chen Zhu Liang
 * @Date         : 2020-06-02 19:41:07
 * @LastEditTime : 2020-07-03 15:05:56
 * @LastEditors  : Chen Zhu Liang
 * @Description  :
 * @FilePath     : public\eybpub_SysPara_File.c
 * @可以输入预定的版权声明、个性签名、空行等
*******************************************************************************/
#ifdef _PLATFORM_BC25_
#include "custom_feature_def.h"
#include "ril.h"
#include "ril_util.h"
#include "ril_system.h"
#include "ril_network.h"
#include "ril_oceanconnect.h"

#include "ql_type.h"
#include "ql_stdlib.h"
#include "ql_memory.h"
#include "ql_gprs.h"
#endif

#ifdef _PLATFORM_L610_
#include "fibo_opencpu.h"
#endif

#include "eybpub_SysPara_File.h"
#include "eybpub_watchdog.h"
#include "eybpub_Debug.h"
#include "eybpub_run_log.h"
#include "eybpub_utility.h"
#include "eybpub_data_collector_parameter_table.h"
#include "eybpub_parameter_number_and_value.h"

#include "eyblib_r_stdlib.h"
// #include "eyblib_memory.h"       // mike 20200828
#include "eyblib_swap.h"
#include "eyblib_list.h"

#include "DeviceIO.h"
#include "Device.h"
#include "Protocol.h"
#include "eybond.h"

#ifdef _PLATFORM_BC25_
#include "NB_net.h"
#endif
#ifdef _PLATFORM_L610_
#include "4G_net.h"
#endif

static u8_t Para_Init_flag = 0;
// static char para_value[64] = {0};  // 参数值

#ifdef _PLATFORM_BC25_
ServerAddr_t *ServerAdrrGet(u8_t num) {
  Buffer_t buf;
//  Buffer_t portBuf;   // mike 20200828
  ServerAddr_t *serverAddr = null;

  Ql_memset(&buf, 0, sizeof(Buffer_t));
  parametr_get(num, &buf);  // TODO获取的buf长度有问题？

  if (buf.payload != null && buf.lenght > 5) {
    ListHandler_t cmdStr;
    int len = 0;

    r_strsplit(&cmdStr, (char *) buf.payload, ':');

    if (cmdStr.count > 0) {
      len = Ql_strlen((char *) * (int *) cmdStr.node->payload);
//      serverAddr = memory_apply(sizeof(ServerAddr_t) + len);  // mike 20200828
      serverAddr = Ql_MEM_Alloc(sizeof(ServerAddr_t) + len);
      Ql_strcpy(serverAddr->addr, (char *) * (int *) cmdStr.node->payload);
      serverAddr->type = 1;

      if (cmdStr.count > 1) {
        serverAddr->port = Swap_charNum(
                             (char *) * (int *) cmdStr.node->next->payload);
        // TODO 有问题，一直会进入这里
      }

      /*      if (num == HANERGY_SERVER_ADDR) {           // mike disable Hanrgy sever 24# command for NB platform 20200827
              serverAddr->port = 8081;
            } else {
              serverAddr->port = 502;
            }  */

      if (cmdStr.count > 2) {
        /*        if (r_strfind("UDP", (char*) *(int*) cmdStr.node->next->next->payload) >= 0) {
                  serverAddr->type = 0;
                } else if (r_strfind("SSL", (char*) *(int*) cmdStr.node->next->next->payload) >= 0) {
                  serverAddr->type = 2;
                } */
        if (Ql_strstr((char *) * (int *) cmdStr.node->next->next->payload, "UDP") >= 0) {
          serverAddr->type = 0;
        } else if (Ql_strstr((char *) * (int *) cmdStr.node->next->next->payload, "SSL") >= 0) {
          serverAddr->type = 2;
        }
      }
    }
    list_delete(&cmdStr);
  }

  if (serverAddr->port == 0) {
//    memory_release(serverAddr);
    if (serverAddr != NULL) {
      Ql_MEM_Free(serverAddr);
      serverAddr = null;
    }
  }

//  memory_release(portBuf.payload);        // mike 20200828
//  memory_release(buf.payload);            // mike 20200828
  if (buf.payload != NULL) {
    Ql_MEM_Free(buf.payload);
    buf.lenght = 0;
    buf.size = 0;
  }

  return serverAddr;
}

void parametr_get(u32_t number, Buffer_t *databuf) {
  char *buf_value = NULL;
  u16_t len = 0;
  if (databuf == NULL) {
    return;
  }
//  APP_DEBUG("para_meter get begin:%d!\r\n", number);
  int j = 0;
  for (j = 0; j < number_of_array_elements; j++) {
    if (number == PDT[j].num) {
      APP_DEBUG("para_meter[%d]num = %d!\r\n", j, number);
//      Ql_memset(para_value, 0, sizeof(para_value));
      buf_value = Ql_MEM_Alloc(sizeof(char) * 64);
      if (buf_value == NULL) {
        APP_DEBUG("MEM Alloc Error\r\n");
        return;
      }
      Ql_memset(buf_value, 0, sizeof(char) * 64);
      if (number == 5 || number == 6 || number == 11 || number == 16\
          || number == 49 || number == 50 || number == 51 || number == 52\
          || number == 55 || number == 56 || number == 58 ||  number == 54) {
        switch (number) {
          case 5:  // 软件版本号
            Ql_strcpy(buf_value, defaultPara[j].para);
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 6:  // 硬件版本号
            Ql_strcpy(buf_value, defaultPara[j].para);
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 11:  // 是否有设备在线
            Ql_strcpy(buf_value, "1");
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 16: {
            s32_t ret = 0;
            ST_Addr_Info_t addr_info;
            Ql_memset(&addr_info, 0, sizeof(ST_Addr_Info_t));
            ret = Ql_GetLocalIPAddress(1, &addr_info);
            if (GPRS_PDP_SUCCESS == ret) {
              APP_DEBUG("Get localIPAddress successfully: %s\r\n", addr_info.addr);
            } else {
              APP_DEBUG("Get localIPAddress failed,ret = %d\r\n", ret);
            }
            Ql_snprintf(buf_value, 64, "%s", addr_info.addr);
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
          }
          break;
          case 49:  // 查询网络注册信息
            // cm_gprs_getcregstate(&creg);
            // cm_itoa(creg,creg_char,10);
            // memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            // PDT[j].wFunc(&PDT[j],creg_char, &len);
          {
            s32_t ret  = RIL_AT_FAILED;
            Enum_OCREG_State OC_reg_state = UNINITIALISED;
            ret = RIL_OCEANCONNECT_GET_NMSTATUS((s8_t *)&OC_reg_state);
            if (RIL_AT_SUCCESS != ret) {
              APP_DEBUG("OC query regstate failure,ret = %d\r\n", ret);
            }
            switch (OC_reg_state) {
              case REGISTERED_AND_OBSERVED:
                Ql_strcpy(buf_value, "0");
                break;
              case REGISTERING:
                Ql_strcpy(buf_value, "1");
                break;
              case REJECTED_BY_SERVER:
                Ql_strcpy(buf_value, "1");
                break;
              case TIMEOUT_AND_RETRYING:
                Ql_strcpy(buf_value, "1");
                break;
              case REGISTERED:
                Ql_strcpy(buf_value, "0");
                break;
              case DEREGISTERED:
                Ql_strcpy(buf_value, "1");
                break;
              case RESUMPTION_FAILED:
                Ql_strcpy(buf_value, "1");
                break;
              case UNINITIALISED:
              default:
                Ql_strcpy(buf_value, "1");
                break;
            }
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
          }
          break;
          case 50:  //GPRS 网络注册状态
            // cm_gprs_getcgregstate(&cgreg);
            // cm_itoa(cgreg,cgreg_char,10);
            // memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            // PDT[j].wFunc(&PDT[j],cgreg_char, &len);
            break;
          case 51:  //固件编译日期
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], MAKE_DATE, &len);
            break;
          case 52:  //固件编译时间
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], MAKE_TIME2, &len);
            break;
          case 54:  //获取日志
//            databuf->payload = memory_apply(log_line_len * log_max_line);
            databuf->payload = Ql_MEM_Alloc(log_line_len * log_max_line);
            PDT[j].rFunc(&PDT[j], databuf->payload, &databuf->lenght);
            // log_i(databuf->payload);
            APP_DEBUG("\r\n");
            APP_DEBUG("%s", databuf->payload);
            return;
            break;
          case 55:
            // CSQ =cm_get_signalLevel();
            // cm_itoa(CSQ,CSQ_char,10);
            // cm_get_signalLevel(para_value);   //mike 20200824
          {
            ST_CSQ_Reponse csq_status;
            RIL_NW_GetCSQ(&csq_status);
//            Ql_snprintf(buf_value, 64, "%d,%d", csq_status.rssi, csq_status.ber);
            Ql_snprintf(buf_value, 64, "%d", csq_status.rssi);
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
          }
          break;
          case 56:  //通信卡CCID
            // cm_get_iccid(para_value);
            RIL_SIM_GetCCID(buf_value);
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 58:  //CPUID IMEI
            // cm_get_imei(para_value);
            RIL_GetIMEI(buf_value);
            Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          default:
            break;
        }
      }

      Ql_memset(buf_value, 0, sizeof(char) * 64);
      PDT[j].rFunc(&PDT[j], buf_value, &len);
//      APP_DEBUG("%d=%s len=%d\r\n", PDT[j].num, buf_value, len);
      if (len > 0) {
//        APP_DEBUG("%d=%s len %d\r\n", PDT[j].num, buf_value, len);
        databuf->size = len;
        databuf->lenght = len;
        databuf->payload = Ql_MEM_Alloc(databuf->size + 1);
        if (databuf->payload == NULL) {
          APP_DEBUG("sys para databuf payload apply fail\r\n");
        } else {
          Ql_memset(databuf->payload, 0, databuf->size + 1);
          Ql_memcpy(databuf->payload, buf_value, databuf->lenght);
        }
      } else {
        APP_DEBUG("para fail!\r\n");
      }
      if (buf_value != NULL) {
        Ql_MEM_Free(buf_value);
        buf_value = NULL;
      }
      break;
    }
  }

//  APP_DEBUG("index: %d totle: %d\r\n", j, number_of_array_elements);
  if (j >= number_of_array_elements) {
    databuf->size = 0;
    databuf->lenght = 0;
    databuf->payload = NULL;
  }
}

u8_t parametr_set(u32_t number, Buffer_t *data) {
  int j = 0;
  u8_t ret = 0x00;
  char *str = NULL;
  for (j = 0; j < number_of_array_elements; j++) {
    if (number == PDT[j].num) {
      APP_DEBUG("para[%d] number is %d Old value is %s\r\n", j, number, PDT[j].a);
      APP_DEBUG("para[%d] number is %d New value will be %s\r\n", j, number, data->payload);
      str = Ql_MEM_Alloc(sizeof(char) * 64);
      if (str == NULL) {
        APP_DEBUG("MEM Alloc Error\r\n");
        ret = 0x03;
        return ret;
      }
      Ql_memset(str, 0, sizeof(str));
      Ql_strncpy(str, (char *)data->payload, Ql_strlen((char *)data->payload));
      APP_DEBUG("para[%d] number is %d str %s\r\n", j, number, str);
      switch (number) {  // 01/02/03/07/08/12/14/29/34/57 生产是必须写的参数
        case 0:
        case 4:
        case 5:
        case 6:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 27:
        case 28:
        case 49:
        case 50:
        case 51:
        case 52:
        case 55:
        case 56:
        case 58:
          APP_DEBUG("para_meter can't be set \r\n");
          ret = 0x02;
          break;
        case 14: {
        } 
        break;
        case 29:
          switch (*(data->payload)) {
            case '1':  // 采集器重启
//              Watchdog_stop();
              log_save("System Software Reset!");
              Ql_Reset(0);  // mike 20200909
              break;
            case '2':  // 恢复出厂设置 -- 指示按defult参数表恢复，生产时写的数据如果改了就恢复不了了
              log_save("System Para Reset!");
              parametr_default();
              Ql_OS_SendMessage(EYBDEVICE_TASK, SYS_PARA_CHANGE, number, 0);
              break;
            case '3':  // 看门狗停狗重启
              log_save("System Hardware Reset!");
              Watchdog_stop();
              break;
            default:
              break;
          }
          ret = 0x00;
          APP_DEBUG("PARA %d=%s\r\n", PDT[j].num, data->payload);
          return ret;
          break;
        case 34: {
          ListHandler_t uartCfgStr;
          r_strsplit(&uartCfgStr, str, '-');
          int tab[4] = {0};
          int i = 0;
          Node_t *node;
          node = uartCfgStr.node;
          do {
            tab[i++] = Swap_charNum((char *) * (int *)node->payload);
            APP_DEBUG("\r\nBand %d, %d.\r\n", i, tab[i - 1]);
            node = node->next;
          } while (node != uartCfgStr.node && i < 4);
          ST_UARTDCB uart;
          if ((tab[0] > 2000 && tab[0] < 1000000)   // 波特率
              && (tab[1] > 4 && tab[1] < 9)  // 数据位
              && (tab[2] > 0 && tab[2] < 4)  // 停止位
              && (tab[3] >= 0 && tab[3] < 5)) {   // 校验位
            uart.baudrate = tab[0];
            uart.dataBits = tab[1];
            uart.stopBits = tab[2];
            uart.parity = tab[3];
            uart.flowCtrl = FC_NONE;
          } else if (tab[0] > 2000 && tab[0] < 1000000) {
            uart.baudrate = tab[0];
            uart.dataBits = DB_8BIT;
            uart.stopBits = SB_ONE;
            uart.parity = PB_NONE;
            uart.flowCtrl = FC_NONE;
          } else {
            ret = 0x02;
            break;
          }
          list_delete(&uartCfgStr);   // mike 20200922
          char str_temp_1[64] = {0};
          char str_temp_2[64] = {0};
          u16_t len = 0;
          char *P1 = NULL;
          Ql_memset(str, 0, sizeof(str));
          Ql_strncpy(str, (char *)data->payload, Ql_strlen((char *)data->payload));
          Ql_memset(str_temp_1, 0, sizeof(str_temp_1));
          Ql_memset(str_temp_2, 0, sizeof(str_temp_2));
          Ql_sprintf(str_temp_2, "#%d-%d-%d-%d#", uart.baudrate, uart.dataBits, uart.stopBits, uart.parity);
          for (i = 0; i < number_of_array_elements; i++) {
            if (14 == PDT[i].num) {
              PDT[i].rFunc(&PDT[i], str_temp_1, &len);
              P1 = Ql_strstr(str_temp_1, "#");
              if (P1) {
                *P1 = '\0';   // 去掉#号
                len = Ql_strlen(str_temp_1);
                APP_DEBUG("%s len:%d\r\n", str_temp_1, len);
                Ql_strcat(str_temp_1, str_temp_2);
                len = Ql_strlen(str_temp_1);
                APP_DEBUG("%s len:%d\r\n", str_temp_1, len);
                Ql_memset((&PDT[i])->a, 0, sizeof((&PDT[i])->a));
                PDT[i].wFunc(&PDT[i], str_temp_1, &(len));
              }
            }
          }
        }
        break;
        default:
          break;
      }

      if (ret == 0x00) {
        //这里只是写入成功，但最后是不是更新成功未知
        ret = 0x00;
        //防止写入字符串过长
        if (data->lenght >= sizeof((&PDT[j])->a)) {
          APP_DEBUG("para so long!\r\n");
          ret = 0x03;
          break;
        }
        Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
//        PDT[j].wFunc(&PDT[j], data->payload, &(data->lenght));
        PDT[j].wFunc(&PDT[j], str, &(data->lenght));

        if (number == 21) {
          log_save("set para 21: %s", (&PDT[j])->a);
        }
        APP_DEBUG("PARA %d=%s\r\n", PDT[j].num, str);
        break;
      }
      if (str != NULL) {
        Ql_MEM_Free(str);
        str = NULL;
      }
    }
    if (j >= number_of_array_elements) {
      APP_DEBUG("para_meter %d is wrongth\r\n", number);
      ret = 0x03;
    }
  }
  // start_timer();
  if (ret == 0x00) {
    parameter_a_module();
    a_copy_to_b();
    parameter_init();  // 保持统一

//    if (number == DEVICE_MONITOR_NUM || number == DEVICE_PROTOCOL || number == METER_PROTOCOL || number == DEVICE_UART_SETTING) { // mike 20201029
    if (number == DEVICE_MONITOR_NUM || number == DEVICE_PROTOCOL || number == DEVICE_UART_SETTING) {
      Ql_OS_SendMessage(EYBDEVICE_TASK, SYS_PARA_CHANGE, number, 0);
//    } else if (number == NB_SERVER_ADDR || number == NB_SERVER_PORT) {
//      Ql_OS_SendMessage(EYBOND_TASK, SYS_PARA_CHANGE, number, 0);
    } else if (number == DEVICE_PNID || number == NB_SERVER_ADDR || number == NB_SERVER_PORT) {
      Net_close();
    }
  }
  return ret ;
}

void main_parametr_update(void) { // 由于APP固件升级会让系统保存的参数不匹配，需要实时更新
  u16_t len = 0;
  int j = 0;
  int number = 0;
  char *buf_value = NULL;
  buf_value = Ql_MEM_Alloc(sizeof(char) * 64);
  if (buf_value == NULL) {
    APP_DEBUG("MEM Alloc Error\r\n");
    return;
  }
  Ql_memset(buf_value, 0, sizeof(char) * 64);

  APP_DEBUG("main_parametr_update!\r\n");
  for (j = 0; j < number_of_array_elements; j++) {
    number = PDT[j].num;
//    Ql_memset(para_value, 0, sizeof(para_value));
    if (number == 5 || number == 6 || number == 11 || number == 49 || number == 50 \
        || number == 51 || number == 52 || number == 55 || number == 56 || number == 58) {
      switch (number) {
        case 5:  //软件版本号
          Ql_strcpy(buf_value, defaultPara[j].para);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 6:  //硬件版本号
          Ql_strcpy(buf_value, defaultPara[j].para);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 11:  //是否有设备在线
          Ql_strcpy(buf_value, "1");
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 49:  //查询网络注册信息
          // cm_gprs_getcregstate(&creg);
          // cm_itoa(creg,creg_char,10);
          // r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          // PDT[j].wFunc(&PDT[j],creg_char, &len);
          break;
        case 50:  //GPRS 网络注册状态
          // cm_gprs_getcgregstate(&cgreg);
          // cm_itoa(cgreg,cgreg_char,10);
          // r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          // PDT[j].wFunc(&PDT[j],cgreg_char, &len);
          break;
        case 51:  //固件编译日期
          Ql_strcpy(buf_value, defaultPara[j].para);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(defaultPara[j].para);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 52:  //固件编译时间
          Ql_strcpy(buf_value, defaultPara[j].para);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(defaultPara[j].para);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 55:
          // CSQ =cm_get_signalLevel();
          // cm_itoa(CSQ,CSQ_char,10);
          // cm_get_signalLevel(para_value);    //mike 20200824
        {
          ST_CSQ_Reponse csq_status;
          RIL_NW_GetCSQ(&csq_status);
//          Ql_snprintf(buf_value, 64, "%d,%d", csq_status.rssi, csq_status.ber);
          Ql_snprintf(buf_value, 64, "%d", csq_status.rssi);
          len = Ql_strlen(buf_value);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          PDT[j].wFunc(&PDT[j], buf_value, &len);
        }
        break;
        case 56:  //通信卡CCID
          // cm_get_iccid(para_value);  //mike 20200824
          RIL_SIM_GetCCID((char *)buf_value);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 58:  //CPUID IMEI
          // cm_get_imei(para_value);  //mike 20200824
          RIL_GetIMEI((char *)buf_value);
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = Ql_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        default:
          break;
      }
    }
  }
  if (buf_value != NULL) {
    Ql_MEM_Free(buf_value);
    buf_value = NULL;
  }
  parameter_a_module();
  a_copy_to_b();
  parameter_init();  // 保持统一
}

/* void parametr_default(void) {   // mike 依据default配置重新生成PDT表
  APP_DEBUG("parametr_default\r\n");
  u16_t len = 0;
  int i = 0;
  for (i = 0; i < number_of_array_elements; i++) {
    if (Ql_strlen(defaultPara[i].para) <= 0) {
      continue;
    }
    Ql_memset((&PDT[i])->a, 0, sizeof((&PDT[i])->a));
    len = Ql_strlen(defaultPara[i].para);
    PDT[i].wFunc(&PDT[i], (void *)defaultPara[i].para, &len);
  }
  parameter_a_module();
  a_copy_to_b();
  parameter_init();  // 保持统一
} */

void GET_ALL_data(void) {
  int j = 0;
  char *buf  = NULL;
  u16_t len = 64;

  for (j = 0; j < number_of_array_elements; j++) {
    //日志系统
    if (PDT[j].num == 54) {
      continue;
    }

    buf = Ql_MEM_Alloc(sizeof(char) * 64);
    Ql_memset(buf, 0, sizeof(char) * 64);
    PDT[j].rFunc(&PDT[j], buf, &len);
    if (Ql_strlen(buf) > 0) {
      APP_PRINT("%d=%s\r\n", PDT[j].num, buf);
    }
    if (buf != NULL) {
      Ql_MEM_Free(buf);
      buf = NULL;
    }
  }
  Para_Init_flag = 1;
}
#endif

#ifdef _PLATFORM_L610_
ServerAddr_t *ServerAdrrGet(u8_t num) {
  Buffer_t buf;
  ServerAddr_t *serverAddr = null;  
  r_memset(&buf, 0, sizeof(Buffer_t));
  parametr_get(num, &buf);  // TODO获取的buf长度有问题?
  
  if (buf.payload != null && buf.lenght > 5) {
    ListHandler_t cmdStr;
    int len = 0;
  
    r_strsplit(&cmdStr, (char *) buf.payload, ':');
  
    if (cmdStr.count > 0) {
      len = r_strlen((char *) * (int *) cmdStr.node->payload);
      serverAddr = fibo_malloc(sizeof(ServerAddr_t) + len);
      r_strcpy(serverAddr->addr, (char *) * (int *) cmdStr.node->payload);
      serverAddr->type = 1;
  
      if (cmdStr.count > 1) {
      serverAddr->port = Swap_charNum((char *) * (int *) cmdStr.node->next->payload);
      // TODO 有问题，一直会进入这里
      }
  
/*    if (num == HANERGY_SERVER_ADDR) {           // mike disable Hanrgy sever 24# command for NB platform 20200827
        serverAddr->port = 8081;
      } else {
        serverAddr->port = 502;
      }  */

      if (cmdStr.count > 2) {
/*        if (r_strfind("UDP", (char*) *(int*) cmdStr.node->next->next->payload) >= 0) {
            serverAddr->type = 0;
          } else if (r_strfind("SSL", (char*) *(int*) cmdStr.node->next->next->payload) >= 0) {
            serverAddr->type = 2;
          } */
        if (r_strstr((char *) * (int *) cmdStr.node->next->next->payload, "UDP") != NULL) {
          serverAddr->type = 0;
        } else if (r_strstr((char *) * (int *) cmdStr.node->next->next->payload, "SSL") != NULL) {
          serverAddr->type = 2;
        }
      }
    }
    list_delete(&cmdStr);
  }
  
  if (serverAddr->port == 0) {
    if (serverAddr != NULL) {
      fibo_free(serverAddr);
      serverAddr = null;
    }
  }  

  if (buf.payload != NULL) {
    fibo_free(buf.payload);
    buf.lenght = 0;
    buf.size = 0;
  }

  return serverAddr;
}

void parametr_get(u32_t number, Buffer_t *databuf) {
  char *buf_value = NULL;
  u16_t len = 0;
  if (databuf == NULL) {
    return;
  }
//  APP_DEBUG("para_meter get begin:%d!\r\n", number);
  int j = 0;
  for (j = 0; j < number_of_array_elements; j++) {
    if (number == PDT[j].num) {
      APP_DEBUG("para_meter[%d]num = %ld!\r\n", j, number);
//      Ql_memset(para_value, 0, sizeof(para_value));
      buf_value = fibo_malloc(sizeof(char) * 64);
      if (buf_value == NULL) {
        APP_DEBUG("MEM Alloc Error\r\n");
        return;
      }
      r_memset(buf_value, 0, sizeof(char) * 64);
      if (number == 5 || number == 6 || number == 11 || number == 16\
          || number == 49 || number == 50 || number == 51 || number == 52\
          || number == 55 || number == 56 || number == 58 ||  number == 54) {
        switch (number) {
          case 5:  // 软件版本号
            r_strcpy(buf_value, defaultPara[j].para);
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 6:  // 硬件版本号
            r_strcpy(buf_value, defaultPara[j].para);
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 11:  // 是否有设备在线
            r_strcpy(buf_value, "1");
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 16: {  // 查询IP地址
          }
            break;
          case 49:  // 查询网络注册信息
            break;
          case 50:  // GPRS 网络注册状态
            break;
          case 51:  // 固件编译日期
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], MAKE_DATE, &len);
            break;
          case 52:  // 固件编译时间
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], MAKE_TIME2, &len);
            break;
          case 54:  // 获取日志
            return;
            break;
          case 55:  {  // 获取CSQ值
            s8_t nrssi = 0, nber = 0;
            fibo_get_csq((INT32 *)&nrssi, (INT32 *)&nber);
            snprintf(buf_value, 64, "%d", nrssi);
            len = r_strlen(buf_value);
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
          }
            break;
          case 56:  // 通信卡CCID
            fibo_get_ccid((u8_t *)buf_value);
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            len = r_strlen(buf_value);
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          case 58:  // CPUID IMEI
            fibo_get_imei((u8_t *)buf_value, 0);
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            len = r_strlen(buf_value);
            PDT[j].wFunc(&PDT[j], buf_value, &len);
            break;
          default:
            break;
        }
      }

      r_memset(buf_value, 0, sizeof(char) * 64);
      PDT[j].rFunc(&PDT[j], buf_value, &len);
//      APP_DEBUG("%d=%s len=%d\r\n", PDT[j].num, buf_value, len);
      if (len > 0) {
//        APP_DEBUG("%d=%s len %d\r\n", PDT[j].num, buf_value, len);
        databuf->size = len;
        databuf->lenght = len;
        databuf->payload = fibo_malloc(databuf->size + 1);
        if (databuf->payload == NULL) {
          APP_DEBUG("sys para databuf payload apply fail\r\n");
        } else {
          r_memset(databuf->payload, 0, databuf->size + 1);
          r_memcpy(databuf->payload, buf_value, databuf->lenght);
        }
      } else {
        APP_DEBUG("para fail!\r\n");
      }
      if (buf_value != NULL) {
        fibo_free(buf_value);
        buf_value = NULL;
      }
      break;
    }
  }
    
//  APP_DEBUG("index: %d totle: %d\r\n", j, number_of_array_elements);
  if (j >= number_of_array_elements) {
    databuf->size = 0;
    databuf->lenght = 0;
    databuf->payload = NULL;
  }
}

u8_t parametr_set(u32_t number, Buffer_t *data) {
  int j = 0;
  u8_t ret = 0x00;
  char *str = NULL;
  for (j = 0; j < number_of_array_elements; j++) {
    if (number == PDT[j].num) {
      APP_DEBUG("para[%d] number is %ld Old value is %s\r\n", j, number, PDT[j].a);
      APP_DEBUG("para[%d] number is %ld New value will be %s\r\n", j, number, data->payload);
      str = fibo_malloc(sizeof(char) * 64);
      if (str == NULL) {
        APP_DEBUG("MEM Alloc Error\r\n");
        ret = 0x03;
        return ret;
      }
      r_memset(str, 0, sizeof(str));
      strncpy(str, (char *)data->payload, r_strlen((char *)data->payload));
      APP_DEBUG("para[%d] number is %ld str %s\r\n", j, number, str);
      switch (number) {  // 01/02/03/07/08/12/14/29/34/57 生产是必须写的参数
        case 0:
        case 4:
        case 5:
        case 6:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 27:
        case 28:
        case 49:
        case 50:
        case 51:
        case 52:
        case 55:
        case 56:
        case 58:
          APP_DEBUG("para_meter can't be set \r\n");
          ret = 0x02;
          break;
        case 14: {
        } 
          break;
        case 29:
          switch (*(data->payload)) {
            case '1':  // 采集器重启
              log_save("System Software Reset!");
              fibo_softReset();
              break;
            case '2':  // 恢复出厂设置 -- 指示按defult参数表恢复，生产时写的数据如果改了就恢复不了了
              log_save("System Para Reset!");
              parametr_default();
              Eybpub_UT_SendMessage(EYBDEVICE_TASK, SYS_PARA_CHANGE, 0, 0);
              break;
            case '3':  // 看门狗停狗重启
              log_save("System Hardware Reset!");
              Watchdog_stop();
              break;
            default:
              break;
          }
          ret = 0x00;
          APP_DEBUG("PARA %d=%s\r\n", PDT[j].num, data->payload);
          return ret;
          break;
        case 34: {
          ListHandler_t uartCfgStr;
          r_strsplit(&uartCfgStr, str, '-');
          int tab[4] = {0};
          int i = 0;
          Node_t *node;
          node = uartCfgStr.node;
          do {
            tab[i++] = Swap_charNum((char *) * (int *)node->payload);
            APP_DEBUG("\r\nBand %d, %d.\r\n", i, tab[i - 1]);
            node = node->next;
          } while (node != uartCfgStr.node && i < 4);
          ST_UARTDCB uart;
          if ((tab[0] > 2000 && tab[0] < 1000000)   // 波特率
              && (tab[1] > 4 && tab[1] < 9)  // 数据位
              && (tab[2] > 0 && tab[2] < 4)  // 停止位
              && (tab[3] >= 0 && tab[3] < 5)) {   // 校验位
            uart.baudrate = tab[0];
            uart.dataBits = tab[1];
            uart.stopBits = tab[2];
            uart.parity = tab[3];
            uart.flowCtrl = FC_NONE;
          } else if (tab[0] > 2000 && tab[0] < 1000000) {
            uart.baudrate = tab[0];
            uart.dataBits = HAL_UART_DATA_BITS_8;
            uart.stopBits = HAL_UART_STOP_BITS_1;
            uart.parity = HAL_UART_NO_PARITY;
            uart.flowCtrl = FC_NONE;
          } else {
            ret = 0x02;
            break;
          }
          list_delete(&uartCfgStr);   // mike 20200922
          char str_temp_1[64] = {0};
          char str_temp_2[64] = {0};
          u16_t len = 0;
          char *P1 = NULL;
          r_memset(str, 0, sizeof(str));
          strncpy(str, (char *)data->payload, r_strlen((char *)data->payload));
          r_memset(str_temp_1, 0, sizeof(str_temp_1));
          r_memset(str_temp_2, 0, sizeof(str_temp_2));
          snprintf(str_temp_2, sizeof(str_temp_2), "#%ld-%d-%d-%d#", uart.baudrate, uart.dataBits, uart.stopBits, uart.parity);
          for (i = 0; i < number_of_array_elements; i++) {
            if (14 == PDT[i].num) {
              PDT[i].rFunc(&PDT[i], str_temp_1, &len);
              P1 = r_strstr(str_temp_1, "#");
              if (P1) {
                *P1 = '\0';   // 去掉#号
                len = r_strlen(str_temp_1);
                APP_DEBUG("%s len:%d\r\n", str_temp_1, len);
                r_strcat(str_temp_1, str_temp_2);
                len = r_strlen(str_temp_1);
                APP_DEBUG("%s len:%d\r\n", str_temp_1, len);
                r_memset((&PDT[i])->a, 0, sizeof((&PDT[i])->a));
                PDT[i].wFunc(&PDT[i], str_temp_1, &(len));
              }
            }
          }
        }
          break;
        default:
          break;
      }
  
      if (ret == 0x00) {
        //这里只是写入成功，但最后是不是更新成功未知
        ret = 0x00;
        //防止写入字符串过长
        if (data->lenght >= sizeof((&PDT[j])->a)) {
          APP_DEBUG("para so long!\r\n");
          ret = 0x03;
          break;
        }
        r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
        PDT[j].wFunc(&PDT[j], str, &(data->lenght));
 
        if (number == 21) {
          log_save("set para 21: %s", (&PDT[j])->a);
        }
        APP_DEBUG("PARA %d=%s\r\n", PDT[j].num, str);
        break;
      }
      if (str != NULL) {
        fibo_free(str);
        str = NULL;
      }
    }
    if (j >= number_of_array_elements) {
      APP_DEBUG("para_meter %ld is wrongth\r\n", number);
      ret = 0x03;
    }
  }
  // start_timer();
  if (ret == 0x00) {
    parameter_a_module();
    a_copy_to_b();
    parameter_init();  // 保持统一
    if (number == DEVICE_MONITOR_NUM || number == DEVICE_PROTOCOL || number == DEVICE_UART_SETTING) {
      Eybpub_UT_SendMessage(EYBDEVICE_TASK, SYS_PARA_CHANGE, 0, 0);
    } else if (number == DEVICE_PNID || number == NB_SERVER_ADDR || number == NB_SERVER_PORT) {
      Net_close();
    }
  }
  return ret ;
}

void main_parametr_update(void) { // 由于APP固件升级会让系统保存的参数不匹配，需要实时更新
  u16_t len = 0;
  int j = 0;
  int number = 0;
  char *buf_value = NULL;
  buf_value = fibo_malloc(sizeof(char) * 64);
  if (buf_value == NULL) {
    APP_DEBUG("MEM Alloc Error\r\n");
    return;
  }
  r_memset(buf_value, 0, sizeof(char) * 64);
    
  APP_DEBUG("main_parametr_update!\r\n");
  for (j = 0; j < number_of_array_elements; j++) {
    number = PDT[j].num;
    if (number == 5 || number == 6 || number == 11 || number == 49 || number == 50 \
        || number == 51 || number == 52 || number == 55 || number == 56 || number == 58) {
      switch (number) {
        case 5:  // 软件版本号
          r_strcpy(buf_value, defaultPara[j].para);
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 6:  // 硬件版本号
          r_strcpy(buf_value, defaultPara[j].para);
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 11:  // 是否有设备在线
          r_strcpy(buf_value, "1");
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 49:  // 查询网络注册信息
          // cm_gprs_getcregstate(&creg);
          // cm_itoa(creg,creg_char,10);
          // r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          // PDT[j].wFunc(&PDT[j],creg_char, &len);
          break;
        case 50:  // GPRS 网络注册状态
          // cm_gprs_getcgregstate(&cgreg);
          // cm_itoa(cgreg,cgreg_char,10);
          // r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          // PDT[j].wFunc(&PDT[j],cgreg_char, &len);
          break;
        case 51:  // 固件编译日期
          r_strcpy(buf_value, defaultPara[j].para);
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(defaultPara[j].para);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 52:  // 固件编译时间
          r_strcpy(buf_value, defaultPara[j].para);
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(defaultPara[j].para);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 55:  // 获取CSQ值
          {
            s8_t nrssi = 0, nber = 0;
            fibo_get_csq((INT32 *)&nrssi, (INT32 *)&nber);
            snprintf(buf_value, 64, "%d", nrssi);
            len = r_strlen(buf_value);
            r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
            PDT[j].wFunc(&PDT[j], buf_value, &len);
          }
          break;
        case 56:  // 通信卡CCID
          fibo_get_ccid((u8_t *)buf_value);
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        case 58:  // CPUID IMEI
          fibo_get_imei((u8_t *)buf_value, 0);
          r_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          len = r_strlen(buf_value);
          PDT[j].wFunc(&PDT[j], buf_value, &len);
          break;
        default:
          break;
      }
    }
  }
  if (buf_value != NULL) {
    fibo_free(buf_value);
    buf_value = NULL;
  }
  parameter_a_module();
  a_copy_to_b();
  parameter_init();  // 保持统一
}

void GET_ALL_data(void) {
  int j = 0;
  char *buf  = NULL;
  u16_t len = 64;
    
  for (j = 0; j < number_of_array_elements; j++) {
    //日志系统
    if (PDT[j].num == 54) {
      continue;
    }
    
    buf = fibo_malloc(sizeof(char) * 64);
    r_memset(buf, 0, sizeof(char) * 64);
    PDT[j].rFunc(&PDT[j], buf, &len);
    if (r_strlen(buf) > 0) {
      APP_PRINT("%d=%s\r\n", PDT[j].num, buf);
    }
    if (buf != NULL) {
      fibo_free(buf);
      buf = NULL;
    }
  }
  Para_Init_flag = 1;
}
#endif

void SysPara_init(void) {
  APP_DEBUG("SysPara_init\r\n");
  Para_Init_flag = 0;
  live_a_and_b();
  main_parametr_update();  //
}

u8_t SysPara_Get_State(void) {
  return Para_Init_flag;
}

void parametr_default(void) {   // mike 依据default配置重新生成PDT表
  APP_DEBUG("parametr_default\r\n");
  u16_t len = 0;
  int i = 0;
  for (i = 0; i < number_of_array_elements; i++) {
    if (r_strlen(defaultPara[i].para) <= 0) {
      continue;
    }
    r_memset((&PDT[i])->a, 0, sizeof((&PDT[i])->a));
    len = r_strlen(defaultPara[i].para);
    PDT[i].wFunc(&PDT[i], (void *)defaultPara[i].para, &len);
  }
  parameter_a_module();
  a_copy_to_b();
  parameter_init();  // 保持统一
}

/******************************************************************************/
