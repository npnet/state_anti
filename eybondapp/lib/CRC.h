/***************************Copyright QYKJ   2015-06-18*************************
  *@brief   : CRCУ�麯��ͷ�ļ�
  *@notes   : 2016.10.28 CGQ ����   
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

