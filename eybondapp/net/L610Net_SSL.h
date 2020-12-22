/**********************    COPYRIGHT 2014-2100,  Eybond ************************ 
 * @File    : L610Net_SSL.h
 * @Author  : CGQ
 * @Date    : 2018-10-25
 * @Brief   : 
 ******************************************************************************/
#ifndef __L610_NET_SSL_H_
#define __L610_NET_SSL_H_

#include "eyblib_typedef.h"
#include "L610Net_TCP_EYB.h"

#define CA_FILE_FLAG            (0xAA550099)
#define CA_FILE_NAME            "/ca_file.dat"              // ca文件名
#define CA_STATUSFILE_NAME      "/ca_file.status"           // ca文件名

void SSL_init(void);
int  SSL_Open(L610Net_t *net);
void SSLHandler_TCPConnect(const char* strURC, void* reserved);
int  SSL_Send(s32_t socketId, u8_t* pData, s32_t dataLen);
void SSLHandler_DataRcve(const char* strURC, void* reserved);

#endif //__L610_NET_SSL_H_
/******************************************************************************/
