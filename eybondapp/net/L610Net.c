/**********************    COPYRIGHT 2014-2100,  Eybond ************************
 * @File    : L610Net.c
 * @Author  : MWY
 * @Date    : 2020-11-20
 * @Brief   :
 ******************************************************************************/
#ifdef _PLATFORM_L610_
#include "fibo_opencpu.h"

#include "eyblib_swap.h"
#include "eyblib_r_stdlib.h"

#include "eybpub_Debug.h"
#include "eybpub_SysPara_File.h"
#include "eybpub_run_log.h"
#include "eybpub_Status.h"
#include "eybpub_utility.h"

#include "L610Net.h"
#include "eybond.h"

#define MAX_GPRS_APN_LEN       100
u8_t g_strapnName[MAX_GPRS_APN_LEN] = {0};

OC_State_e m_OCActState;
s8_t Update_PN_For_OCMSG();

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
static u32_t g_OC_ncfg_lifetime = THE_DEFAULT_OC_LIFETIME;

static L610Net_t netManage;
static u8_t registe = 0;
// static s32_t customPara = 0;

// static u32_t OC_timer = APP_NET_OC_TIMER_ID;
static u32_t OC_time_Interval = 1000;
static void UserTimerOCcallback(u32_t timerId, void *param);     // OC服务器下发数据callback

void L610_OceanConnect_recv(u8_t *buffer, u32_t length);

static s32_t OCREC_ATResponse_Handler(char *line, u32_t len, void *userData) {
  return 0;
}

static void UserTimerOCcallback(u32_t timerId, void *param) {
}

/*******************************************************************************
 Brief    : L610Net para init
 Parameter:
 return   :
*******************************************************************************/
void L610Net_init(void) {
  s32_t ret = 0;
  m_OCActState = STATE_TOTAL_NUM;

  registe = 0;
  NetLED_Off();
  GSMLED_Off();
  r_memset(&netManage, 0, sizeof(netManage));

  Update_PN_For_OCMSG();
}

/*******************************************************************************
 Brief    : L610Net_open
 Parameter:
 return   : connect number 1; oxFF: full no space
*******************************************************************************/
u8_t L610Net_open(u8_t mode, char *ip, u16_t port, NetDataCallback netCallback) {
  netManage.flag = 1;
  netManage.mode = mode;
  netManage.port = port;
  netManage.callback = netCallback;
//  netManage.ocreg_status = UNINITIALISED;
  netManage.socketID = -1;
  netManage.ip = 0;
  r_strcpy((char *)netManage.ipStr, ip);
  return netManage.flag;
}

/*******************************************************************************
 Brief    : L610Net_status
 Parameter:
 return   :
*******************************************************************************/
u8_t L610Net_status() {
  return 0;
}

/*******************************************************************************
 Brief    : L610Net_close
 Parameter:
 return   :
*******************************************************************************/
void L610Net_close() {
  netManage.flag = 1;
  netManage.mode = 0;
  netManage.port = 0;
  netManage.callback = NULL;
//  netManage.ocreg_status = UNINITIALISED;
  netManage.socketID = -1;
  netManage.ip = 0;
  r_strcpy((char *)netManage.ipStr, "");
  NetLED_Off();
  Update_PN_For_OCMSG();
  m_OCActState = STATE_OC_NCDPCLOSE;
}

/*******************************************************************************
 Brief    : L610Net_ready
 Parameter:
 return   :
*******************************************************************************/
void L610Net_ready(void) {
  s32_t ret = 0;
  m_OCActState = STATE_NW_QUERY_STATE;
}

// static s32_t offset = 0;
static u32_t open_time = 0;
/*******************************************************************************
 Brief    : L610Net_manage
 Parameter:
 return   :
*******************************************************************************/
void L610Net_manage(void) {
  s32_t ret = 0;
  APP_DEBUG("L610 Net Manage\r\n");
/*  s32_t simret = 0;
  u8_t simstatus = 0;
  simret = fibo_get_sim_status(&simstatus);
  if ((simstatus == 1) && (simret == 0)) {
    // SIM卡已插入
    APP_DEBUG("sim is insert !\r\n");
    Eybpub_UT_SendMessage(EYBNET_TASK, NET_MSG_SIM_READY, 0, 0);
  } else {
    APP_DEBUG("sim no checked, please insert sim & retry\r\n");
    Eybpub_UT_SendMessage(EYBNET_TASK, NET_MSG_SIM_FAIL, 0, 0);
    Eybpub_UT_SendMessage(EYBNET_TASK, NET_MSG_GSM_FAIL, 0, 0);
    m_OCActState = STATE_TOTAL_NUM;
  } */

  reg_info_t sim_reg_info;
  fibo_getRegInfo(&sim_reg_info, 0);
  if (1 != sim_reg_info.nStatus) {
    Eybpub_UT_SendMessage(EYBNET_TASK, NET_MSG_GSM_FAIL, 0, 0);
    m_OCActState = STATE_TOTAL_NUM;
  }
  switch (m_OCActState) {
    case STATE_NW_QUERY_STATE: {
      APP_DEBUG("STATE_NW_QUERY_STATE\r\n");
      u8_t ip[50];
	  u8_t cid_status;
	  s8_t cid =1;	
      r_memset(&ip, 0, sizeof(ip));
      if (0 == fibo_PDPStatus(cid, ip,&cid_status, 0)) {
        APP_DEBUG("ip = %s,cid_status=%d\r\n", ip,cid_status);
        m_OCActState = STATE_OC_NCFG;
      }
      break;
    }
    case STATE_TOTAL_NUM: {
      break;
    }
    default:
      break;
  }
//  offset = 0;
}

/*******************************************************************************
 Brief    : L610Net_send
 Parameter:
 return   :
*******************************************************************************/
int L610Net_send(u8_t *data, u16_t len) {
  int ret = -520;
  return ret;
}

void L610_OceanConnect_recv(u8_t *buffer, u32_t length) {
  if (length >= 1024) {
    APP_PRINT("%ld\r\n", length);
    return;
  }
  APP_PRINT("%ld,%s \r\n", length, buffer);
}

s8_t Update_PN_For_OCMSG() {
  s8_t ret = 1;
  Buffer_t buf;
  int i = 0;
  char strTemp[3] = {0};  
  APP_DEBUG("Server %s port %ld\r\n", g_SrvADDR, g_SrvPort);
  return ret;
}
#endif
/*********************************FILE END*************************************/

