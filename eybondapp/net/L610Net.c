/**********************    COPYRIGHT 2014-2100,  Eybond ************************
 * @File    : L610Net.c
 * @Author  : MWY
 * @Date    : 2020-08-04
 * @Brief   :
 ******************************************************************************/
#include "ql_stdlib.h"
#include "ql_gprs.h"
#include "ql_error.h"

#include "ql_memory.h"
#include "ql_stdlib.h"
#include "ql_urc_register.h"

#include "ril_util.h"
#include "ril_network.h"
#include "ril_oceanconnect.h"

#include "eyblib_swap.h"
#include "eybpub_Debug.h"
#include "eybpub_Syspara_File.h"
#include "eybpub_run_log.h"
#include "eybpub_status.h"

#include "L610Net.h"
#include "eybond.h"

#define MAX_GPRS_APN_LEN       100
u8_t g_strapnName[MAX_GPRS_APN_LEN] = {0};

OC_State_e m_OCActState;

/*****************************************************************
*  OceanConnect Param
******************************************************************/
#define OC_TIMER_PERIOD     1000
ST_NMGS_Param_t g_NMGS_Param_PN;
s8_t Update_PN_For_OCMSG();
// DEVICE_PNID
static u8_t g_NCFG_mode = OC_LIFETIME_CFG;
static u32_t g_NCFG_value = 900;    // 900/60 = 15分钟上报状态，类似心跳机制
//static u32_t g_NCFG_value = 86400;
static u8_t *g_pskid = NULL;

/*****************************************************************
* Server Param
******************************************************************/
#define SRVADDR_BUFFER_LEN  100
static u8_t  g_SrvADDR[SRVADDR_BUFFER_LEN] = "221.229.214.202\0";
static u32_t g_SrvPort = 5683;

#define THE_DEFAULT_OC_LIFETIME  (0)
static Enum_OCREG_State g_OC_nmstatus_value = UNINITIALISED;   // OC Server register state
static u32 g_OC_ncfg_lifetime = THE_DEFAULT_OC_LIFETIME;

static BC25OCNet_t netManage;
static u8_t registe = 0;
// static s32_t customPara = 0;

static u32_t OC_timer = APP_NET_OC_TIMER_ID;
static u32_t OC_time_Interval = 1000;
static void UserTimerOCcallback(u32 timerId, void *param);     // OC服务器下发数据callback

void BC25_OceanConnect_recv(u8 *buffer, u32 length);

static s32 OCREC_ATResponse_Handler(char *line, u32 len, void *userData) {
  if (Ql_RIL_FindLine(line, len, "OK")) {
    return  RIL_ATRSP_SUCCESS;
  } else if (Ql_RIL_FindLine(line, len, "ERROR")) {
    return  RIL_ATRSP_FAILED;
  } else if (Ql_RIL_FindString(line, len, "+CME ERROR")) {
    return  RIL_ATRSP_FAILED;
  } else if (Ql_RIL_FindString(line, len, "+CMS ERROR:")) {
    return  RIL_ATRSP_FAILED;
  } else if (Ql_RIL_FindString(line, len, "+NMGR:")) {
//    APP_PRINT("%s len=%d\r\n", line,len);
    BC25_OceanConnect_recv((u8 *)line, len);
    return RIL_ATRSP_CONTINUE;
  }
  return RIL_ATRSP_CONTINUE; //continue wait
}

static void UserTimerOCcallback(u32 timerId, void *param) {
  if (timerId == OC_timer) {
    s32 ret = RIL_AT_FAILED;
    char strAT[64];
    Ql_memset(strAT, 0, sizeof(strAT));
    Ql_sprintf(strAT, "AT+NMGR\r\n");
    ret = Ql_RIL_SendATCmd(strAT, Ql_strlen(strAT), OCREC_ATResponse_Handler, NULL, 0);
  }
}

/*******************************************************************************
 Brief    : BC25Net para init
 Parameter:
 return   :
*******************************************************************************/
void BC25Net_init(void) {
  s32_t ret = QL_RET_OK;
  m_OCActState = STATE_TOTAL_NUM;

  registe = 0;
  NetLED_Off();
  GSMLED_Off();
  Ql_memset(&netManage, 0, sizeof(netManage));

  Update_PN_For_OCMSG();

  ret = RIL_OCEANCONNECT_NNMI(OC_NNMI_BUFFER);
  if (RIL_AT_SUCCESS == ret) {
    APP_DEBUG("OC Set NNMI value %d success\r\n", OC_NNMI_BUFFER);
  } else {
    log_save("OC Set NNMI mode failure,ret = %d\r\n", ret);
  }
}

/*******************************************************************************
 Brief    : BC25Net_open
 Parameter:
 return   : connect number 1; oxFF: full no space
*******************************************************************************/
u8_t BC25Net_open(u8_t mode, char *ip, u16_t port, NetDataCallback netCallback) {
  netManage.flag = 1;
  netManage.mode = mode;
  netManage.port = port;
  netManage.callback = netCallback;
  netManage.ocreg_status = UNINITIALISED;
  netManage.socketID = -1;
  netManage.ip = 0;
  Ql_strcpy((char *)netManage.ipStr, ip);
  return netManage.flag;
}

/*******************************************************************************
 Brief    : BC25Net_status
 Parameter:
 return   :
*******************************************************************************/
u8_t BC25Net_status() {
  s32_t ret  = RIL_AT_FAILED;
  ret = RIL_OCEANCONNECT_GET_NMSTATUS((s8_t *)&netManage.ocreg_status);
  if (RIL_AT_SUCCESS == ret) {
//    APP_DEBUG("OC query regstate suc, value = %d\r\n", g_OC_nmstatus_value);
  } else {
    log_save("OC query regstate failure,ret = %d\r\n", ret);
  }
  if (netManage.ocreg_status == REGISTERED_AND_OBSERVED) {
    NetLED_On();
  } else {
    NetLED_Off();
  }
  return netManage.ocreg_status;
}

/*******************************************************************************
 Brief    : BC25Net_close
 Parameter:
 return   :
*******************************************************************************/
void BC25Net_close() {
  netManage.flag = 1;
  netManage.mode = 0;
  netManage.port = 0;
  netManage.callback = NULL;
  netManage.ocreg_status = UNINITIALISED;
  netManage.socketID = -1;
  netManage.ip = 0;
  Ql_strcpy((char *)netManage.ipStr, "");
  NetLED_Off();
  Update_PN_For_OCMSG();
  m_OCActState = STATE_OC_NCDPCLOSE;
}

/*******************************************************************************
 Brief    : BC25Net_ready
 Parameter:
 return   :
*******************************************************************************/
void BC25Net_ready(void) {
  s32_t ret = QL_RET_OK;
  // register  recv callback
//  ret = Ql_OC_Recv_Register(BC25_OceanConnect_recv);
  ret = Ql_Timer_Register(OC_timer, UserTimerOCcallback, NULL);  // 注册OC Timer
  if (ret < QL_RET_OK) {
    APP_DEBUG("Register OC timer(%d) failed!! ret = %d\r\n", OC_timer, ret);
  } else {
    APP_DEBUG("Register OC timer(%d) successfully!! ret = %d\r\n", OC_timer, ret);
  }

  ret = Ql_Timer_Start(OC_timer, OC_time_Interval, TRUE);
  if (ret < 0) {
    APP_DEBUG("Start OC timer failed, ret = %d\r\n", ret);
  } else {
    APP_DEBUG("Start OC timer(%d) successfully, timer interval = %d, ret = %d\r\n", OC_timer, OC_time_Interval, ret);
  }
//  APP_DEBUG("Register recv callback (%d).\r\n", ret);
  if (ret == QL_RET_OK) {
    registe = 1;
  } else {
    registe = 0;
    log_save("Fail to register OC Recv callback: %d", ret);
  }
  m_OCActState = STATE_NW_QUERY_STATE;
}

// static s32_t offset = 0;
static u32_t open_time = 0;
/*******************************************************************************
 Brief    : BC25Net_manage
 Parameter:
 return   :
*******************************************************************************/
void BC25Net_manage(void) {
  s32_t ret = 0;
//  APP_DEBUG("BC25 Net Manage\r\n");

  switch (m_OCActState) {
    case STATE_NW_QUERY_STATE: {
      APP_DEBUG("STATE_NW_QUERY_STATE\r\n");
      ST_Addr_Info_t addr_info;
      Ql_memset(&addr_info, 0, sizeof(ST_Addr_Info_t));
      ret = Ql_GetLocalIPAddress(1, &addr_info);
      if (GPRS_PDP_SUCCESS == ret) {
        APP_DEBUG("Get localIPAddress successfully: %s\r\n", addr_info.addr);
        m_OCActState = STATE_OC_NCFG;
      } else {
        APP_DEBUG("Get localIPAddress failed,ret = %d\r\n", ret);
      }
      break;
    }
    case STATE_OC_NCFG: {
      APP_DEBUG("STATE_OC_NCFG\r\n");
      //set lifetime
      ret = RIL_OCEANCONNECT_NCFG(g_NCFG_mode, g_NCFG_value);
      if (RIL_AT_SUCCESS == ret) {
        APP_DEBUG("OC Lifetime Config Successfully\r\n");
        m_OCActState = STATE_OC_GET_NCFG;
      } else {
        APP_DEBUG("OC_Lifetime Config Failure,ret = %d\r\n", ret);
      }
      break;
    }
    case STATE_OC_GET_NCFG: {
      APP_DEBUG("STATE_OC_NCFG\r\n");
      ret = RIL_OCEANCONNECT_GET_NCFG(g_NCFG_mode, (u32 *)&g_OC_ncfg_lifetime);
      if (RIL_AT_SUCCESS == ret) {
        APP_PRINT("OC get NCFG config suc, value = %d\r\n", g_OC_ncfg_lifetime);
        m_OCActState = STATE_OC_NCDPOPEN;
      } else {
        APP_DEBUG("OC get NCFG config failure,ret = %d\r\n", ret);
      }
      break;
    }
    case STATE_OC_NCDPOPEN: {
      APP_DEBUG("STATE_OC_NCDPOPEN %s:%d\r\n", g_SrvADDR, g_SrvPort);
      ret = RIL_OCEANCONNECT_NCDPOPEN(g_SrvADDR, g_SrvPort, g_pskid);
      if (RIL_AT_SUCCESS == ret) {
        m_OCActState = STATE_TOTAL_NUM;
        NetLED_On();
      } else {
        m_OCActState = STATE_TOTAL_NUM;
        APP_DEBUG("OC open request excute failure,check param,ret = %d\r\n", ret);
      }
      break;
    }
    case STATE_OC_NMSTATUS: {
      APP_DEBUG("STATE_OC_NMSTATUS\r\n");
      m_OCActState = STATE_OC_NMGS;
      break;
    }
    case STATE_OC_NMGS: {
      u8_t str_Init[128] = {0};
      Ql_memset(str_Init, 0, 128);
      Ql_snprintf((char *)str_Init, 128, "%s %s %s\r\n", FWVERSION, MAKE_TIME, BUILDINFO);
//      ret = BC25Net_send(str_Init, Ql_strlen((char *)str_Init));
      Net_send(g_SrvPort, str_Init, Ql_strlen((char *)str_Init));
      m_OCActState = STATE_TOTAL_NUM;
      break;
    }
    case STATE_OC_NCDPCLOSE: {
      APP_DEBUG("STATE_OC_NCDPCLOSE\r\n");
      ret = RIL_OCEANCONNECT_NCDPCLOSE();
      if (RIL_AT_SUCCESS == ret) {
        APP_DEBUG("OC close connection successfully!\r\n");
        m_OCActState = STATE_NW_QUERY_STATE;
        open_time = 0;
        NetLED_Off();
      } else {
        APP_DEBUG("OC close connection failure,ret = %d\r\n", ret);
      }
      break;
    }
    case STATE_TOTAL_NUM: {
//      APP_DEBUG("STATE_TOTAL_NUM\r\n");
      g_OC_nmstatus_value = BC25Net_status();
      // if open time more than 3min, stop and reopen  ??? mike: Why?
//      if (netManage.ocreg_status == REGISTERING) {
      if (g_OC_nmstatus_value == REGISTERING \
        || g_OC_nmstatus_value == REGISTERED_AND_OBSERVED \
        || g_OC_nmstatus_value == REGISTERED) {
        if (g_OC_nmstatus_value == REGISTERING) {
          open_time += OC_TIMER_PERIOD;
          if (open_time > (OC_TIMER_PERIOD * 180)) {
            m_OCActState = STATE_OC_NCDPCLOSE;  // 尝试三分钟注册，不成功才重试连接
            APP_DEBUG("OC opentime++, open_time = %d\r\n", open_time / 1000);
            open_time = 0;
          }
        }
      } else {
        APP_DEBUG("OC connection fail, status = %d\r\n", g_OC_nmstatus_value);
        m_OCActState = STATE_OC_NCDPCLOSE;
      }
      // else do nothing
      break;
    }
    default:
      break;
  }
//  offset = 0;
}

/*******************************************************************************
 Brief    : BC25Net_send
 Parameter:
 return   :
*******************************************************************************/
int BC25Net_send(u8_t *data, u16_t len) {       // LWM2M 模式数据上发
  int ret = -520;
  if (len >= 492) {
    APP_DEBUG("OC message must is less than 492 bytes\r\n");
    return ret;
  }

  g_OC_nmstatus_value = BC25Net_status();
  if (g_OC_nmstatus_value == REGISTERED_AND_OBSERVED || g_OC_nmstatus_value == REGISTERED) {
    char *strData = NULL;
    strData = Ql_MEM_Alloc(len * 2 + 1);
    if (strData == NULL) {
      ret = RIL_AT_UNINITIALIZED;
      return ret;
    }
    ST_NMGS_Param_t NMGS_Param_t;
    int i = 0, datalen = 0;;
    char strTemp[3] = {0};

    Ql_strcpy(strData, "");
    for (i = 0; i < len; i++) {
      Ql_memset(strTemp, 0, 3);
      Ql_snprintf(strTemp, 3, "%02X", data[i]);
      Ql_strcat(strData, strTemp);
    }
    Ql_strcat(strData, "\0");
    datalen = Ql_strlen(strData);

    APP_DEBUG("NB input data:%s len:%d\r\n", strData, len);

    NMGS_Param_t.oc_send_len = len + g_NMGS_Param_PN.oc_send_len;
    NMGS_Param_t.oc_send_type = OC_SEND_CON_RAI_0;
//    NMGS_Param_t.oc_send_type = OC_SEND_CON_RAI_1;
    NMGS_Param_t.oc_send_data = Ql_MEM_Alloc(NMGS_Param_t.oc_send_len * 2 + 1);
    if (NMGS_Param_t.oc_send_data == NULL) {
      if (strData != NULL) {
        Ql_MEM_Free(strData);
        strData = NULL;
      }
      ret = RIL_AT_UNINITIALIZED;
      return ret;
    }
//    hextostr(data, NMGS_Param_t.oc_send_data, len);
    Ql_strcpy((char *)NMGS_Param_t.oc_send_data, (char *)g_NMGS_Param_PN.oc_send_data);
    Ql_strcat((char *)NMGS_Param_t.oc_send_data, strData);
    Ql_strcat((char *)NMGS_Param_t.oc_send_data, "\0");
    APP_DEBUG("NB Sent %d %d %s\r\n", NMGS_Param_t.oc_send_len, Ql_strlen((char *)NMGS_Param_t.oc_send_data),
              NMGS_Param_t.oc_send_data);
    ret = RIL_OCEANCONNECT_NMGS(&NMGS_Param_t);
    if (RIL_AT_SUCCESS == ret) {
      APP_PRINT("OC send msg successfully\r\n");
    } else {
      APP_DEBUG("OC send msg failure,ret = %d\r\n", ret);
    }

    if (NMGS_Param_t.oc_send_data != NULL) {
      Ql_MEM_Free(NMGS_Param_t.oc_send_data);
      NMGS_Param_t.oc_send_data = NULL;
    }

    if (strData != NULL) {
      Ql_MEM_Free(strData);
      strData = NULL;
    }
  } else {
    ret = RIL_AT_UNINITIALIZED;
  }
  return ret;
}

void BC25_OceanConnect_recv(u8 *buffer, u32 length) {
  if (length >= 1024) {
    APP_PRINT("%ld\r\n", length);
    return;
  }
  APP_PRINT("%d,%s \r\n", length, buffer);
//  char strInput[1024] = {0};
  char *strInput = Ql_MEM_Alloc(1024);
  if (strInput == NULL) {
    APP_DEBUG("Ql_MEM_Alloc Error!!\r\n");
    return;
  }
  Ql_memset(strInput, 0x0, sizeof(strInput));
  char *strCmd = Ql_MEM_Alloc(1024);
  if (strCmd == NULL) {
    APP_DEBUG("Ql_MEM_Alloc Error!!\r\n");
    Ql_MEM_Free(strInput);
    return;
  }
  Ql_memset(strCmd, 0x0, sizeof(strCmd));

  char *p1 = NULL;
  char *p2 = NULL;
//  char *head = Ql_RIL_FindString((char *)buffer, length, "+NNMI:"); //continue wait
  char *head = Ql_RIL_FindString((char *)buffer, length, "+NMGR:"); //continue wait
  if (head) {
    Ql_memset(strInput, 0x0, sizeof(strInput));
    p1 = Ql_strstr(head, ":");
    p2 = Ql_strstr(p1 + 1, "\r\n");
    if ((p1 != NULL) && (p2 != NULL)) {
      Ql_memcpy((char *)strInput, p1 + 2, p2 - p1 - 2);
    }
  }
//  APP_DEBUG("%d, %s\r\n", Ql_strlen(strInput), strInput);

  head = NULL;
//  char strCmd[1024] = {0};
  head = Ql_strstr(strInput, (char *)g_NMGS_Param_PN.oc_send_data); // 在下发消息中查找PN号标识
  if (head != NULL) {
//    Ql_memset(strCmd, 0x0, sizeof(strCmd));
    Ql_memcpy((char *)strCmd, head + Ql_strlen((char *)g_NMGS_Param_PN.oc_send_data),
              (Ql_strlen(strInput) - Ql_strlen((char *)g_NMGS_Param_PN.oc_send_data)));
  }

  APP_PRINT("%d, %s\r\n", Ql_strlen(strCmd), strCmd);

  if (Ql_strlen(strCmd) % 2 != 0) {
    APP_PRINT("Error Cmd!!\r\n");
    if (strInput != NULL) {
      Ql_MEM_Free(strInput);
      strInput = NULL;
    }
    if (strCmd != NULL) {
      Ql_MEM_Free(strCmd);
      strCmd = NULL;
    }
    return;
  }
  Buffer_t cmdbuf;
  cmdbuf.size = 512 + 1;
  cmdbuf.lenght = Ql_strlen(strCmd) / 2;
  cmdbuf.payload = Ql_MEM_Alloc(512 + 1);   // 输入指令内存申请

  Swap_charHex(cmdbuf.payload, strCmd);  // 将hex字符转成hex数 123456 -->0x120x340x56(6 bytes --> 3 bytes)
  if (strInput != NULL) {
    Ql_MEM_Free(strInput);
    strInput = NULL;
  }
  if (strCmd != NULL) {
    Ql_MEM_Free(strCmd);
    strCmd = NULL;
  }
  ESP_callback(netManage.port, &cmdbuf);
//  m_OCActState = STATE_OC_NCDPCLOSE;
}

s8_t Update_PN_For_OCMSG() {
  s8_t ret = 1;
  Buffer_t buf;
  int i = 0;
  char strTemp[3] = {0};
  g_NMGS_Param_PN.oc_send_type = OC_SEND_CON_RAI_0;
//  g_NMGS_Param_PN.oc_send_type = OC_SEND_CON_RAI_1;
  buf.lenght = 0;
  buf.size = 0;
  parametr_get(DEVICE_PNID, &buf);  // NB注网成功后才能获取到真实参数
  if (buf.lenght > 2 && buf.payload != null) {
    if (g_NMGS_Param_PN.oc_send_data != NULL) {
      Ql_MEM_Free(g_NMGS_Param_PN.oc_send_data);
      g_NMGS_Param_PN.oc_send_data = NULL;
    }
    g_NMGS_Param_PN.oc_send_len = Ql_strlen((char *)buf.payload) + 2;
    g_NMGS_Param_PN.oc_send_data = Ql_MEM_Alloc(g_NMGS_Param_PN.oc_send_len * 2 + 1);
    Ql_strcpy((char *)g_NMGS_Param_PN.oc_send_data, "");
    Ql_memset(strTemp, 0, 3);
    Ql_snprintf(strTemp, 3, "%02X", '#');
    Ql_strcat((char *)g_NMGS_Param_PN.oc_send_data, strTemp);
    for (i = 0; i < Ql_strlen((char *)buf.payload); i++) {
      Ql_memset(strTemp, 0, 3);
      Ql_snprintf(strTemp, 3, "%02X", buf.payload[i]);
      Ql_strcat((char *)g_NMGS_Param_PN.oc_send_data, strTemp);
    }
    Ql_memset(strTemp, 0, 3);
    Ql_snprintf(strTemp, 3, "%02X", '#');
    Ql_strcat((char *)g_NMGS_Param_PN.oc_send_data, strTemp);
    Ql_strcat((char *)g_NMGS_Param_PN.oc_send_data, "\0");

    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
      buf.lenght = 0;
      buf.size = 0;
    }
  } else {
    g_NMGS_Param_PN.oc_send_len = 7;
    g_NMGS_Param_PN.oc_send_data = Ql_MEM_Alloc(7 * 2 + 1);
    Ql_strncpy((char *)g_NMGS_Param_PN.oc_send_data, "010548454c4c4f\0", 7 * 2 + 1);
    ret = 0;
  }

  u8_t tmpbuf[64];
  u32_t tmpPort = 0;
  Ql_memset(tmpbuf, 0x0, sizeof(tmpbuf));
  ret = RIL_NW_GetPDP((char *)tmpbuf);
  APP_DEBUG("PDP Value:%s\r\n", tmpbuf);

  Ql_memset(&buf, 0, sizeof(Buffer_t));
  parametr_get(GPRS_APN_ADDR, &buf);
  if (buf.lenght > 2 && buf.payload != null) {
    Ql_strcpy((char *)g_strapnName, (char *)buf.payload);  // mike 20200826
    APP_DEBUG("APN para Value:%s\r\n", g_strapnName);
    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
      buf.lenght = 0;
      buf.size = 0;
    }
// "IP","","","",0
//    if (Ql_strcmp((char *)tmpbuf, (char *)g_strapnName) != 0) {
//      ret = RIL_NW_SetPDP((char *)g_strapnName);
//      APP_DEBUG("Set PDP to APN para Value ret:%d\r\n", ret);
//      if (ret == RIL_ATRSP_SUCCESS) {
//          APP_DEBUG("Reboot for setting PDP to APN para Value:%s\r\n", g_strapnName);
//        Ql_Delay_ms(1000);
//        Ql_Reset(0);
//      }
//    }
  }

  Ql_memset(&buf, 0, sizeof(Buffer_t));
  parametr_get(NB_SERVER_ADDR, &buf);
  if (buf.lenght > 2 && buf.payload != null) {
    if ((Ql_strcmp((char *)buf.payload, "www.shinemonitor.com") != 0) \
      && (Ql_strcmp((char *)buf.payload, "solar.eybond.com") != 0)) {
      Ql_strcpy((char *)g_SrvADDR, (char *)buf.payload);
    }

    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
      buf.lenght = 0;
      buf.size = 0;
    }
  }

  Ql_memset(&buf, 0, sizeof(Buffer_t));
  parametr_get(NB_SERVER_PORT, &buf);
  if (buf.lenght > 2 && buf.payload != null) {
    tmpPort = Swap_charNum((char *)buf.payload);
    if (buf.payload != NULL) {
      Ql_MEM_Free(buf.payload);
      buf.payload = NULL;
      buf.lenght = 0;
      buf.size = 0;
    }
  }

  if (tmpPort != 0) {
    g_SrvPort = tmpPort;
  }
  APP_DEBUG("Server %s port %d\r\n", g_SrvADDR, g_SrvPort);

  return ret;
}

/*********************************FILE END*************************************/

