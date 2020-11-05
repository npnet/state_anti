/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : Xmodem.h
 * @Author  : CGQ
 * @Date    : 2017-12-14
 * @Brief   : 
 ******************************************************************************/
#ifndef __X_MODEM_TRANS__
#define __X_MODEM_TRANS__

#include "ql_type.h"
#include "typedef.h"
#include "file.h"

#pragma pack(1)
typedef struct 
{
	u8 head;
	u8 num;
	u8 numF;
	u8 data[1024];
	u8 crcH;
	u8 crcL;
}Xmodem_t;
#pragma pack()


void Xmodem_start(File_t *file);
void Xmodem_end(void);
u8 Xmodem_progress(void);
int Xmodem_ack(Buffer_t *sendBuf, Buffer_t *ack);



#endif //__X_MODEM_TRANS__
/******************************************************************************/
