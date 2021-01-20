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
#include "grid_tool.h"

#include "fibo_opencpu.h"
#include "L610Net_TCP_EYB.h"
#include "CommonServer.h"

#define LOGIN_ID      0x00
#define REGISTER_ID   0x01
#define UPLOAD_ID     0x04
#define HEARTBEAT_ID  0x99

#define LOGIN_ID_ACK      1
#define REGISTER_ID_ACK   1
#define UPLOAD_ID_ACK     1
#define HEARTBEAT_ID_ACK  2


L610Net_t *SSLNet;

s32 sslsock=-1;
static u8 ssl_index=0;

static int ssl_socket(void);

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

/*
int SSL_modeCfg(void) {
  int result = 0;
  static const char cfg0[] = "AT+QIFGCNT=1\r\n";
  static const char cfg1[] = "AT+QIMUX=1\r\n";
  static const char cfg2[] = "AT+QIREGAPP\r\n";

  APP_DEBUG("%s\r\n", cfg0);
  return result;
}
*/


/*****************************************************************************                     
* introduce:        ssl 初始化      
* parameter:        none                 
* return:           none       
* author:           Luee                                              
*****************************************************************************/
void SSL_init(void) 
{
    int result;
    SSLNet = null;
}


/*****************************************************************************                     
* introduce:        ssl socket ,need running in timer 1s api       
* parameter:        none                 
* return:           none       
* author:           Luee                                              
*****************************************************************************/
static int ssl_socket(void) 
{
    int ret;
    static u8 ssl_counter=0;

    if(ssl_counter)
      ssl_counter--;

    switch(ssl_index){
      case 0:
        //如果需要验证服务器的证书，将这个值设置为１，否则设置为０
        fibo_set_ssl_chkmode(1);
        fibo_write_ssl_file("TRUSTFILE", STATEGRID_CA_FILE, sizeof(STATEGRID_CA_FILE) - 1);
        //执行后需延时10S
        ssl_counter=20;   //*500ms
        ssl_index=1;
        break;

      case 1:
        if(ssl_counter)
          break;
        
        ssl_index=3;    
        //fibo_ssl_sock_close(sslsock);
        sslsock = fibo_ssl_sock_create();
        if (sslsock == -1){
          APP_DEBUG("\r\ncreate ssl sock failed\r\n");
          break;
        }
        APP_DEBUG("\r\nsslsock ID= %d\r\n", sslsock);
        ret = fibo_ssl_sock_connect(sslsock, "gfyfront.esgcc.com.cn", 19020);
        if(ret==0){
          APP_DEBUG("\r\n-->ssl socket connet succes!!!")
          ssl_index=2;
        }   
      break;
      //ssl socket connect success,handle
      case 2: 
        ssl_index=0;
        return 0;
        //登录
        //stateGrid_login();
        //ssl_rec();

        //注册
        //log_d("\r\nregister state grid\r\n");
        //fibo_taskSleep(1000);
        //stateGrid_register();
        //ssl_rec();
      break;

      //ssl socket connect fail,handle
      case 3: 
        ssl_index=0;
        return -1;
      break;

      default:
      break;
    }
}

/******************************************************************************                     
* introduce:        ssl socket create      
* parameter:        none                 
* return:           none       
* author:           Luee                                              
*****************************************************************************/
static int ssl_socket2(void) 
{
    int ret=-1;

    //如果需要验证服务器的证书，将这个值设置为１，否则设置为０
    fibo_set_ssl_chkmode(1);
  
    fibo_write_ssl_file("TRUSTFILE", STATEGRID_CA_FILE, sizeof(STATEGRID_CA_FILE) - 1);

    fibo_taskSleep(10000);
    
    //fibo_ssl_sock_close(sslsock);
    sslsock = fibo_ssl_sock_create();
    if (sslsock == -1){
        APP_DEBUG("\r\n-->create ssl sock failed\r\n");
        //send_message(queue_l610net,L610NET_SSLFAIL_ID,0,0,0);
    }
    else{
        APP_DEBUG("\r\n-->fibossl sslsock %d\r\n", sslsock);
        SSLNet->socketID=sslsock;
        ret = fibo_ssl_sock_connect(SSLNet->socketID, SSLNet->ipStr, SSLNet->port);  
        APP_DEBUG("\r\n-->fibossl sys_sock_connect %d\r\n", ret);
        if(ret==0)
            APP_DEBUG("\r\n-->ssl socket connet succes!!!");
            //send_message(queue_l610net,L610NET_SSLOK_ID,0,0,0);
    }
    return ret;   
}


void ssl_init(void) 
{
    //int result;
    SSLNet = null;
}

/*
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
*/
int SSL_Open(L610Net_t *net)
{
    int ret;

    SSLNet = net;
    //建立SSL SOCKET连接
    return ssl_socket();
}

/******************************************************************************                    
 * introduce:        ssl 数据接收     
 * parameter:        none                 
 * return:           返回ret,=-1接收失败，否则返回数据长度       
 * author:           Luee                                              
 *****************************************************************************/

s32 ssl_rec(void)
{
    s32 ret=-1;
    u8 rerec=1;
    u8 recbuf[64] = {0};
    u8 index=0;
    u16 rec_len=0;
    static u16 ssl_relink_times=0;

    while(rerec){
    APP_DEBUG("\r\nssl receiving\r\n");
    rec_len = fibo_ssl_sock_recv(sslsock, recbuf, sizeof(recbuf));
    log_d("\r\nfibossl sys_sock_recv %d\r\n", rec_len);
    if (rec_len > 0){
        APP_DEBUG("\r\n-->ssl receive buf:%x\r\n",recbuf);
        print_buf((UINT8 *)recbuf, rec_len);
        rerec=0;
        //得到功能码地址
        //消息头（4）+采集终端ID（长度1+内容）+ 控制域（1）+消息体（功能码1+数据）
        index=4+1+recbuf[4]+1;
        switch(recbuf[index]){
          case LOGIN_ID:
            if(recbuf[index+1]==LOGIN_ID_ACK){
              APP_DEBUG("\r\n-->state grid login success\r\n");
              ret=rec_len;
            }else{
              APP_DEBUG("\r\n-->state grid login fail\r\n");
            }
          break;
          case REGISTER_ID:
            if(recbuf[index+1]==REGISTER_ID_ACK){
              APP_DEBUG("\r\n-->state grid register success\r\n");
              ret=rec_len;
            }else{
              APP_DEBUG("\r\n-->state grid register fail\r\n");
            }
          break;
          case UPLOAD_ID:
            if(recbuf[index+1]==UPLOAD_ID_ACK){
              APP_DEBUG("\r\n-->state grid upload success\r\n");
              ret=rec_len;    //测试SSL断网重连
            }else{
              APP_DEBUG("\r\n-->state grid upload fail\r\n");
            }
          break;
          case HEARTBEAT_ID:
            if(recbuf[index+1]==HEARTBEAT_ID_ACK){
              APP_DEBUG("\r\n-->state grid heartbeat success\r\n");
              ret=rec_len;
            }else{
              APP_DEBUG("\r\n-->state grid heartbeat fail\r\n");
            }
          break;
          default:
          break;
        }
       // send_message(queue_l610net,L610NET_SSLOK_ID,0,0,0);
        }
    else{
       log_d("\r\n-->ssl receive fail\r\n"); 
       rerec=0;
      //send_message(queue_l610net,L610NET_SSLFAIL_ID,0,0,0);
        }
    }
    //连接失败重连
    if(ret==-1){
      APP_DEBUG("\r\n-->ssl recieve fail times:%d\r\n",ssl_relink_times);
      if(ssl_relink_times++>30){
        ssl_relink_times=0;
        ssl_relink();
      }
    }
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

