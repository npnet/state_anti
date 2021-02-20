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
#include "StateGrid.h"

#define LOGIN_ID      0x00
#define REGISTER_ID   0x01
#define UPLOAD_ID     0x04
#define HISTORY_ID    0x05
#define HEARTBEAT_ID  0x99

#define LOGIN_ID_ACK      1
#define REGISTER_ID_ACK   1
#define UPLOAD_ID_ACK     1
#define HISTORY_ID_ACK     1
#define HEARTBEAT_ID_ACK  2


L610Net_t *SSLNet;

s32 sslsock=-1;
static u8 ssl_index=0;

static s8 ssl_socket(void);
static void write_ssl_from_file(void);

static u8 STATEGRID_CA_FILE[]={"-----BEGIN CERTIFICATE-----\r\n"                                      \
                            "MIICuDCCAiGgAwIBAgIJAP+L+/yzpCHgMA0GCSqGSIb3DQEBBQUAMHQxCzAJBgNV\r\n" \
                            "BAYTAnpoMQswCQYDVQQIDAJkbDELMAkGA1UEBwwCZGwxDTALBgNVBAoMBHNvZnQx\r\n" \
                            "CzAJBgNVBAsMAmRsMREwDwYDVQQDDAhjanNlcnZlcjEcMBoGCSqGSIb3DQEJARYN\r\n" \
                            "Y2pAc2VydmVyLmNvbTAgFw0xNjA4MzEwOTE0MjhaGA8yMTE2MDgwNzA5MTQyOFow\r\n" \
                            "dDELMAkGA1UEBhMCemgxCzAJBgNVBAgMAmRsMQswCQYDVQQHDAJkbDENMAsGA1UE\r\n" \
                            "CgwEc29mdDELMAkGA1UECwwCZGwxETAPBgNVBAMMCGNqc2VydmVyMRwwGgYJKoZI\r\n" \
                            "hvcNAQkBFg1jakBzZXJ2ZXIuY29tMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKB\r\n" \
                            "gQDF8M2VsIjtwOOn2mNEqJ+yL7Lz2SeRVfnx/mmYs5fqYN//mz/LQtX6DLhuUIg3\r\n" \
                            "nehDa7sX1VQeFd7YuVCv7aKoUfHLllfy5MWq5leM+F2UOkH1IF6BSl+PRxgIwAEQ\r\n" \
                            "X2M69VhSCQva6p/dZs0pdn1GW2bGrp1WjNdwfH5qor+zLwIDAQABo1AwTjAdBgNV\r\n" \
                            "HQ4EFgQUxrYkWFqCIsmasiWXptWzhcDJABkwHwYDVR0jBBgwFoAUxrYkWFqCIsma\r\n" \
                            "siWXptWzhcDJABkwDAYDVR0TBAUwAwEB/zANBgkqhkiG9w0BAQUFAAOBgQAurlSk\r\n" \
                            "YBsorMV+4zPOH1unbdph5K1W0soUtLNfejWUWDEJQRoYA2UxPPbVYkzdEsTsbgnt\r\n" \
                            "ZeHF6gk1V9VmDhmVKFAy141iYlLjIJxNHVnhGZDot06oPsF+gZ1ymAzX6RB+I9GD\r\n" \
                            "N8BeBxfUcpriXykKg5MraSlMsjyeD1XhwZfJ/w==\r\n"                         \
                            "-----END CERTIFICATE-----\r\n"};

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
* introduce:            
* parameter:        none                 
* return:           none       
* author:           Luee                                             
*****************************************************************************/
static void write_ssl_from_file(void)
{
  s32 g_iFd_CA;
  static u8 *CA_value_buf  = NULL;

  g_iFd_CA = fibo_file_open(CA_FILE_NAME, FS_O_RDONLY);
  s32 file_CA_size =  fibo_file_getSize(CA_FILE_NAME); 
  CA_value_buf = fibo_malloc(sizeof(char)*file_CA_size);
  memset(CA_value_buf, 0, sizeof(char)*file_CA_size); 
  fibo_file_read(g_iFd_CA, (UINT8 *)CA_value_buf, file_CA_size);
  fibo_file_close(g_iFd_CA);
  fibo_write_ssl_file("TRUSTFILE", CA_value_buf, file_CA_size - 1);
  fibo_free(CA_value_buf);
  APP_DEBUG("\r\n-->state grid CA file get lenght=%ld",file_CA_size);
}

/*****************************************************************************                     
* introduce:        ssl socket ,need running in timer 1s api       
* parameter:        none                 
* return:           none       
* author:           Luee                                              
*****************************************************************************/
static s8 ssl_socket(void) 
{
    s8 ret=-1;
    static u8 ssl_counter=0;

    if(ssl_counter)
      ssl_counter--;

    switch(ssl_index){
      case 0:
        //如果需要验证服务器的证书，将这个值设置为１，否则设置为０
        fibo_set_ssl_chkmode(1);
        if (fibo_file_getSize(CA_FILE_NAME) >= 0 || fibo_file_exist(CA_FILE_NAME) == 1){
          write_ssl_from_file();
        }else
          fibo_write_ssl_file("TRUSTFILE", STATEGRID_CA_FILE, sizeof(STATEGRID_CA_FILE) - 1);
        //执行后需延时10S
        ssl_counter=20;   //*500ms*20
        ssl_index=1;
        break;

      case 1:
        if(ssl_counter)
          break;
        
        ssl_index=3; 
        ssl_counter=20;   //*500ms*20

        sslsock = fibo_ssl_sock_create();
        if (sslsock == -1){
            APP_DEBUG("\r\n-->state grid create ssl sock failed\r\n");
        }
        else{
            APP_DEBUG("\r\n-->state grid fibossl sslsock %ld\r\n", sslsock);
            SSLNet->socketID=sslsock;
            ret = fibo_ssl_sock_connect(SSLNet->socketID, SSLNet->ipStr, SSLNet->port);  
            APP_DEBUG("\r\n-->fibossl sys_sock_connect %d\r\n", ret);
            if(ret==0){
              ssl_index=2;
              APP_DEBUG("\r\n-->ssl socket connet succes!!!");
              //send_message(queue_l610net,L610NET_SSLOK_ID,0,0,0);
            }
        }
      break;
      //ssl socket connect success,handle
      case 2: 
        ssl_index=0;
        ret=0;
      break;

      //ssl socket connect fail,handle
      case 3: 
        ssl_counter=20;
        ssl_index=0;
        ret=-1;
      break;

      default:
      break;
    }
    return ret;
    //return -1;
}

/******************************************************************************                     
* introduce:        ssl socket create      
* parameter:        none                 
* return:           none       
* author:           Luee                                              
*****************************************************************************/
/*
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
        APP_DEBUG("\r\n-->fibossl sslsock %ld\r\n", sslsock);
        SSLNet->socketID=sslsock;
        ret = fibo_ssl_sock_connect(SSLNet->socketID, SSLNet->ipStr, SSLNet->port);  
        APP_DEBUG("\r\n-->fibossl sys_sock_connect %ld\r\n", ret);
        if(ret==0)
            APP_DEBUG("\r\n-->ssl socket connet succes!!!");
            //send_message(queue_l610net,L610NET_SSLOK_ID,0,0,0);
    }
    return ret;   
}
*/

void ssl_init(void) 
{
    //int result;
    SSLNet = null;
}

s8 SSL_Open(L610Net_t *net)
{
    SSLNet = net;
    //建立SSL SOCKET连接
    return ssl_socket();
}

/*******************************************************************************
 Brief    : M26Socket
 Parameter: 
 return   : 
*******************************************************************************/
/*
L610Net_t *M26Socket(s32 socketid, u8 *offset)
{
	int i;

	for (i = 0; i < sizeof(netManage)/sizeof(netManage[0]); i++)
	{
		if (netManage[i].flag == 1 && netManage[i].socketID == socketid)
		{
			*offset = i;
			return &netManage[i];
		}
	}
	
	return null;
}*/

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
    s32 rec_len=0;
    static u16 ssl_relink_times=0;

    while(rerec){
    APP_DEBUG("\r\nssl receiving\r\n");
    rec_len = fibo_ssl_sock_recv(sslsock, recbuf, sizeof(recbuf));
    APP_DEBUG("\r\nfibossl sys_sock_recv %ld\r\n", rec_len);
    if (rec_len > 0){
        APP_DEBUG("\r\n-->ssl receive buf:\r\n");
        print_buf((UINT8 *)recbuf, rec_len);
        rerec=0;
        //得到功能码地址
        //消息头（4）+采集终端ID（长度1+内容）+ 控制域（1）+消息体（功能码1+数据）
        index=4+1+recbuf[4]+1;
        switch(recbuf[index]){
          case LOGIN_ID:
            if(recbuf[index+1]==LOGIN_ID_ACK){
              APP_DEBUG("\r\n-->state grid login success!!!\r\n");
              log_save("\r\n-->state grid login success!!!\r\n");
              ret=rec_len;
              set_grid_step(1);
            }else{
              APP_DEBUG("\r\n-->state grid login fail\r\n");
            }
          break;
          case REGISTER_ID:
            if(recbuf[index+1]==REGISTER_ID_ACK){
              APP_DEBUG("\r\n-->state grid register success!!!\r\n");
              log_save("\r\n-->state grid register success!!!\r\n");
              ret=rec_len;
              set_grid_step(2);
            }else{
              APP_DEBUG("\r\n-->state grid register fail\r\n");
            }
          break;

          case UPLOAD_ID:
            if(recbuf[index+1]==UPLOAD_ID_ACK){
              APP_DEBUG("\r\n-->state grid upload success!!!\r\n");
              log_save("\r\n-->state grid upload success!!!\r\n");
              ret=rec_len;    //测试SSL断网重连
              set_grid_step(2);
            }else{
              APP_DEBUG("\r\n-->state grid upload fail\r\n");
            }
          break;

          case HISTORY_ID:
            if(recbuf[index+1]==HISTORY_ID_ACK){
              APP_DEBUG("\r\n-->state grid upload history success!!!\r\n");
              log_save("\r\n-->state grid upload history success!!!\r\n");
              ret=rec_len;    //测试SSL断网重连
              set_grid_step(2);
            }else{
              APP_DEBUG("\r\n-->state grid upload history fail\r\n");
            }
          break;
          
          case HEARTBEAT_ID:
            if(recbuf[index+1]==HEARTBEAT_ID_ACK){
              APP_DEBUG("\r\n-->state grid heartbeat success!!!\r\n");
              log_save("\r\n-->state grid heartbeat success!!!\r\n");
              ret=rec_len;
              set_grid_step(2);
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
    //收到有效ssl数据，心跳间隔时间清0
    //国网只需保证20分钟内有数据正确交换就不会断网
    if(ret>0){
      set_heartbeatSpace(0);
    }
    //连接失败重连
    if(ret<=0){
      APP_DEBUG("\r\n-->state grid ssl recieve fail times:%d\r\n",ssl_relink_times);
      log_save("\r\n-->state grid ssl recieve fail times:%d\r\n",ssl_relink_times);
      if(ssl_relink_times++>30){
        ssl_relink_times=0;
        ssl_relink();
      }
    }
    return ret;
}

/*
s32 ssl_rec(void)
{
    s32 ret=-1;
    u8 rerec=1;
    u8 recbuf[64] = {0};
    u8 index=0;
    s32 rec_len=0;
    static u16 ssl_relink_times=0;

    static Buffer_t dataBuf = {0};

  rec_len = fibo_ssl_sock_recv(sslsock, recbuf, sizeof(recbuf));
  APP_DEBUG("\r\nfibossl sys_sock_recv %ld\r\n", rec_len);
  if (rec_len > 0){
      APP_DEBUG("\r\n-->state grid ssl receive buf:\r\n");
      print_buf((UINT8 *)recbuf, rec_len);
      rerec=0;
      //得到功能码地址
      //消息头（4）+采集终端ID（长度1+内容）+ 控制域（1）+消息体（功能码1+数据）
      index=4+1+recbuf[4]+1;

    //
    dataBuf.size = rec_len;
    dataBuf.lenght = rec_len;
    dataBuf.payload = fibo_malloc(dataBuf.size);
    r_memcpy(dataBuf.payload, recbuf, rec_len);

    u8_t  port;

    L610Socket(SSLNet->socketID, &port);
    SSLNet->callback(port, &dataBuf);
    memory_release(dataBuf.payload);
  } 
  return rec_len;
}
*/
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

/******************************************************************************                    
 * introduce:        ssl 数据接收任务     
 * parameter:        none                 
 * return:           返回ret,=-1接收失败，否则返回数据长度       
 * author:           Luee                                              
 *****************************************************************************/
//static Buffer_t recbuf;
//static u8 sslbuf[64]={0};

s32 sslrec_task(void *param)
{
  s32 len;
  Buffer_t recbuf;
  //u8 sslbuf[64]={0};

  //recbuf.payload=sslbuf;

  while(1){
    fibo_taskSleep(500);
    recbuf.payload=fibo_malloc(100);
    r_memset(recbuf.payload, '\0', sizeof(recbuf.payload)); 
    len = fibo_ssl_sock_recv(sslsock, recbuf.payload, sizeof(recbuf.payload));
    if(len>0){
      recbuf.lenght=len;
      recbuf.size=len+4;
      state_rec_process(&recbuf);
    }
    fibo_free(recbuf.payload);
    return len;
  }
}

//fibo_thread_create(netrecv_task,      "NET RECV TASK",     1024*8*2, NULL, OSI_PRIORITY_NORMAL);
//    fibo_thread_create_ex(L610_TCP_Callback, (INT8 *)strTaskname, 1024*8*2, (void *)&nIndex, OSI_PRIORITY_NORMAL, &l610tcp_thread_id[nIndex]);
//void L610_TCP_Callback(u8_t *param) 
//void netrecv_task(void *param)




/*********************************FILE END*************************************/

