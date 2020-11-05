/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : eybpub_utility.h
 * @Author  : MWY
 * @Date    : 2020-08-05
 * @Brief   : 
 ******************************************************************************/
#ifndef __EYBPUB_UTILITY_H_
#define __EYBPUB_UTILITY_H_

#ifdef _PLATFORM_BC25_
#include "ql_type.h"
#include "ql_uart.h"

#define NET_LED			(PINNAME_NETLIGHT)
#define GSM_LED			(PINNAME_UART2_RTS)
#define DEVICE_LED		(PINNAME_RI)
#define BEEP_PORT		(PINNAME_DCD)

#define KEY_PORT		(PINNAME_UART2_CTS)
#define WATCHDOG_PIN	(PINNAME_GPIO5)
// #define WATCHDOG_PIN	(PINNAME_GPIO2)
#define SERIAL_RX_BUFFER_LEN  1024

s32 ReadSerialPort(Enum_SerialPort port,u8* pBuffer, u32 bufLen);
#endif

#ifdef _PLATFORM_L610_
#include "oc_uart.h"

#define NET_LED			(0)
#define GSM_LED			(0)
#define DEVICE_LED		(0)
#define BEEP_PORT		(0)

#define KEY_PORT		(0)
#define WATCHDOG_PIN	(0)
#define SERIAL_RX_BUFFER_LEN  1024

s32_t ReadSerialPort(hal_uart_port_t port,u8_t* pBuffer, u32_t bufLen);
#endif

#endif //__EYBPUB_UTILITY_H_
/******************************************************************************/
