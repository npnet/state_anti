/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : 4G_net.h for L610 modem
  *@notes   : 2020.10.03 MWY
*******************************************************************************/
#ifndef __4G_NET_H
#define __4G_NET_H

#include "eyblib_typedef.h"

#ifdef _PLATFORM_L610_
#define MSG_ID_USER_NET_START    0x2000
#define NET_CMD_SENDDATA_ID  MSG_ID_USER_NET_START + 0xF90
#define NET_CMD_RESTART_ID   MSG_ID_USER_NET_START + 0xF91
#define NET_CMD_AT_ID        MSG_ID_USER_NET_START + 0xF92

#define NET_MSG_GSM_READY    MSG_ID_USER_NET_START + 0xF93    // 基站网络连接OK
#define NET_MSG_GSM_FAIL     MSG_ID_USER_NET_START + 0xF94

#define NET_MSG_NET_READY    MSG_ID_USER_NET_START + 0xF95    // LWM2M服务器连接OK
#define NET_MSG_NET_FAIL     MSG_ID_USER_NET_START + 0xF96

#define NET_MSG_SIM_READY    MSG_ID_USER_NET_START + 0xF97    // SIM卡检测OK
#define NET_MSG_SIM_FAIL     MSG_ID_USER_NET_START + 0xF98

#define NET_MSG_RIL_READY    MSG_ID_USER_NET_START + 0xF99    // RIL SDK驱动OK
#define NET_MSG_RIL_FAIL     MSG_ID_USER_NET_START + 0xF9A
#endif

typedef void (*NetDataCallback)(Buffer_t *buf);

void proc_net_task(s32_t taskId);
u8_t Net_connect(u8_t mode, char *ip, u16_t port, NetDataCallback netCallback);
void Net_close();
void Net_send(u8_t port, u8_t * pData, u16_t len);
u8_t Net_status();
#endif  // __4G_NET_H
/******************************************************************************/

