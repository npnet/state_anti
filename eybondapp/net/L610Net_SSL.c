/**********************    COPYRIGHT 2014-2100,  Eybond *********************** 
 * @File    : L610Net_SSL.c
 * @Author  : CGQ
 * @Date    : 2018-10-25
 * @Brief   : 
 ******************************************************************************/
#include "L610Net_SSL.h"
// #include "custom_feature_def.h"
#include "eybpub_Debug.h"
#include "eyblib_memory.h"
#include "eyblib_r_stdlib.h"
// #include "Syspara.h"
#include "eybpub_File.h"
#include "eybpub_run_log.h"

L610Net_t *SSLNet;

/******************************************************************************
 Brief    : SSL Ca file write
 Parameter:
 return   :
*******************************************************************************/
static s32_t CAFileWrite_Handler(char* line, u32_t len, void* userData) {
  Buffer_t *buf = userData;
  char *head = null;

  APP_DEBUG("%s\r\n", line);

  return 1;
}

int SSL_CAFileWrite(void) {
  char *at = NULL;
  Buffer_t buf;
  int ret = 0;
//  File_t *file = memory_apply(sizeof(File_t));
//  File_state(file, CA_FILE_ADDR);
  return ret;
}

static s32_t CfgAck_Handler(char* line, u32_t len, void* userData) {
  char *head;
  APP_DEBUG("%s\r\n", line);
  return 1;
}

static s32_t CAFileRead_Handler(char* line, u32_t len, void* userData) {
  char *head = null;
  APP_DEBUG("%s\r\n", line);
  return 1;
}

int SSL_modeCfg(void) {
  int result = 0;
  static const char cfg0[] = "AT+QIFGCNT=1\r\n";
  static const char cfg1[] = "AT+QIMUX=1\r\n";
  static const char cfg2[] = "AT+QIREGAPP\r\n";

  APP_DEBUG("%s\r\n", cfg0);
  return result;
}

void SSL_init(void) {
  int result = 0;
  SSLNet = null;
}

int SSL_Open(L610Net_t *net) {
  int ret = 0;
  char *at = NULL;

  SSLNet = net;
  at = memory_apply(100);
  r_memset(at, 0, 100);

  APP_DEBUG("%s, %d\r\n", at, ret);
  memory_release(at);
    
  return ret;
}

/*******************************************************************************
 Brief    : SSL Ca file write
 Parameter: 
 return   : 
*******************************************************************************/
static s32_t dataSend_Handler(char* line, u32_t len, void* userData) {
  Buffer_t *buf = userData;
  char *head = null;

  APP_DEBUG("%s\r\n", line);

  return 1;
}

int SSL_Send(s32_t socketId, u8_t* pData, s32_t dataLen) {
  char *at = NULL;
  int ret = 0;
  int offset = 0;
  Buffer_t buf;

  at = memory_apply(100);
  if (at != null) {
     memory_release(at);
  }
  return ret;
}

/*******************************************************************************
  * @note   SLL TCP connect callbback
  * @param  None
  * @retval None
*******************************************************************************/
void SSLHandler_TCPConnect(const char* strURC, void* reserved) {
  APP_DEBUG("%s\r\n", strURC);
}

/*******************************************************************************
 Brief    : SSL Ca file write
 Parameter: 
 return   : 
*******************************************************************************/
static s32_t dataRcve_Handler(char* line, u32_t len, void* userData) {
  static Buffer_t dataBuf = {0};
  char *head = null;
  int offset = 0;

  APP_DEBUG("%s\r\n", line);

  return 0;
}

/*******************************************************************************
  * @note   SLL data rcveice Callback
  * @param  None
  * @retval None
*******************************************************************************/
void SSLHandler_DataRcve(const char* strURC, void* reserved) {
  int offset = 0;
  APP_DEBUG("%s\r\n", strURC);
}
/*********************************FILE END*************************************/

