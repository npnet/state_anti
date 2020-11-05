/***************************Copyright QYKJ   2015-06-18*************************
  *@brief   : CRC校验函数头文件
  *@notes   : 2016.10.28 CGQ 创建   
*******************************************************************************/
#ifndef		__CRC_H_
#define		__CRC_H_


typedef enum
{
    CRC_USA     = 0x8005,
    CRC_CCITI   = 0x1021,
    CRC_RTU     = 0xA001,     
}CRCStandard_e;

unsigned short crc16_standard(CRCStandard_e std, const unsigned char *data, int len);


#endif  //__CRC_H_
/******************************************************************************/

