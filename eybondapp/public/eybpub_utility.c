/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Quectel Co., Ltd. 2013
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   utility.c 
 *
 * Project:
 * --------
 *   OpenCPU
 *
 * Description:
 * ------------
 *   The APIs are used to parse string.
 *
 * Author:
 * -------
 * -------
 *
 *============================================================================
 *             HISTORY
 *----------------------------------------------------------------------------
 * 
 ****************************************************************************/

#ifdef _PLATFORM_BC25_
#include "ql_type.h"
#include "ql_uart.h"
#include "ql_stdlib.h"
#endif

#ifdef _PLATFORM_L610_
#endif

#include "eybpub_utility.h"

#ifdef _PLATFORM_BC25_
s32 ReadSerialPort(Enum_SerialPort port,u8* pBuffer, u32 bufLen)
{
    s32 rdLen = 0;
    s32 rdTotalLen = 0;
    if (NULL == pBuffer || 0 == bufLen)
    {
        return -1;
    }
    Ql_memset(pBuffer, 0x0, bufLen);
    while (1)
    {
        rdLen = Ql_UART_Read(port, pBuffer + rdTotalLen, bufLen - rdTotalLen);
        if (rdLen <= 0)  // All data is read out, or Serial Port Error!
        {
            break;
        }
		// Continue to read...
        rdTotalLen += rdLen;
//		Ql_Delay_ms(20);//Avoid serial data is divided into multiple packages.
    }
    if (rdLen < 0) // Serial Port Error!
    {
        return -99;
    }
    return rdTotalLen;
}
#endif

#ifdef _PLATFORM_L610_
s32_t ReadSerialPort(hal_uart_port_t port, u8_t* pBuffer, u32_t bufLen) {
  s32_t rdLen = 0;
  s32_t rdTotalLen = 0;
  return rdTotalLen;
}
#endif