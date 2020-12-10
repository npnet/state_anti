/**********************    COPYRIGHT 2014-2100,  Eybond ************************
 * @File    : L610Net.h
 * @Author  : MWY
 * @Date    : 2020-08-04
 * @Brief   :
 ******************************************************************************/
#ifndef __L610_NET_H_
#define __L610_NET_H_

#include "4G_net.h"

/*****************************************************************
* define OceanConnect state for timer state_step
******************************************************************/
typedef enum {
  STATE_NW_QUERY_STATE = 0,
  STATE_OC_NCFG,
  STATE_OC_GET_NCFG,
  STATE_OC_NNMI,
  STATE_OC_NCDPOPEN,
  STATE_OC_NMSTATUS,
  STATE_OC_GET_NMSTATUS,
  STATE_OC_NMGS,
  STATE_OC_NMGR,
  STATE_OC_NCDPCLOSE,
  STATE_TOTAL_NUM
} OC_State_e;

/*
typedef enum
{
	M26_SUCCESS,
	M26_WAIT,
	M26_DNS_FAIL,
	M26_IP_OK,
	M26_SOCKET_FAIL,
	M26_CONNECT_FAIL,
    M26_CONNECTING,
	M26_CLOSE,
}M26NetState_e;
*/

typedef struct {
  u8_t    flag;     // link user flag
  u8_t    mode;     // link connect mode
  u16_t   port;     // net port
  u32_t   ip;       // link inter net IP -> ___.___.___.___
  s32_t   socketID;  // socket id
//  Enum_OCREG_State ocreg_status;  // oc register status
  char  ipStr[52];        // user connet IP/DNS, lake
  NetDataCallback callback;  // rcveData callback fun
} L610Net_t;

extern OC_State_e m_OCActState;

void L610Net_init(void);
u8_t L610Net_open(u8_t mode, char *ip, u16_t port, NetDataCallback netCallback);
void L610Net_close();
int  L610Net_send(u8_t *data, u16_t len);
void L610Net_closeAll(void);
void L610Net_manage(void);
u8_t L610Net_status();
void L610Net_ready(void);
// L610Net_t *L610Socket(s32_t socketid);

#endif  // __L610_NET_H_
/******************************************************************************/
