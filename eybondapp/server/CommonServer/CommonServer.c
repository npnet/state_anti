/**************Copyright(C) 2015-2026 Shenzhen Eybond Co., Ltd.***************
  *@brief   : CommonServer.c
  *@notes   : 2018.10.23 CGQ
*******************************************************************************/
#ifdef _PLATFORM_M26_
#include "ql_system.h"

#include "net.h"
#endif

#ifdef _PLATFORM_L610_
#include "fibo_opencpu.h"
#include "4G_net.h"
#endif

#include "eyblib_memory.h"
#include "eyblib_swap.h"
#include "eyblib_algorithm.h"
#include "eyblib_r_stdlib.h"

#include "eybpub_Debug.h"
#include "eybpub_utility.h"
#include "eybpub_Clock.h"

#include "eybapp_appTask.h"
#include "Device.h"
#include "ModbusDevice.h"
#include "Modbus.h"
#include "eybpub_run_log.h"

// #include "SAJ.h"
// #include "Hanergy.h"
#include "StateGrid.h"
#include "CommonServer.h"
#include "L610Net_SSL.h"
#include "L610Net_TCP_EYB.h"
#include "eybpub_Status.h"

net_para_t eybnet_para;
net_para_t statenet_para;

typedef int (*ServerCheck)(void);

typedef struct {
  ServerCheck check;
  const CommonServer_t *api;
} CommonServerTab_t;

static int common(void);

static CommonServerTab_t serverTab[] = {
//  {Hanergy_check, &HanergyServer},
//  {SAJ_check, &SAJ_API},
  {StateGrid_check, &StateGrid_API},
  {common, null},
};

static const CommonServerTab_t server_con={
   StateGrid_check, &StateGrid_API
};

static u8_t sPort;
static int overtime;
//static CommonServerTab_t  const *server;
static CommonServerTab_t *server;

static u16_t relinkTime = 0;


static void output(Buffer_t *buf);
static void dataProcess(u8_t port, Buffer_t *buf);

int common(void) {
  return 0;
}

static u8 soft_reset_flag=0;
static u16 soft_reset_counter=0;

static void soft_reset_handle(void);


//////////////////////////////////////
//ssl 重连
void ssl_relink(void)
{
  sslsock=-1;
  relinkTime=0;
  overtime = 0;
  sPort=0;
  CommonServer_close();
  sPort = 0xff;  
  server = null;
  SSL_init();

  APP_DEBUG("\r\n-->state grid relink!!!\r\n");
}

void soft_reset_en(void)
{
  soft_reset_flag=1;
  soft_reset_counter=10;   //after 5s,reset

}

static void soft_reset_handle(void)
{
  if(soft_reset_flag){
    if(soft_reset_counter)
      soft_reset_counter--;
    APP_DEBUG("\r\n-->soft reset downcount:%d\r\n",soft_reset_counter);
    if(soft_reset_counter==0){
      soft_reset_flag=0;
      log_save("System Software Reset!");
      APP_DEBUG("\r\n-->System Software Reset!\r\n");
      fibo_softReset();
    }
  }
}

void clear_overtime(void){
  overtime=0;
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void proc_commonServer_task(s32_t taskId) {
  u8_t ret;
  ST_MSG msg;
  //u16_t relinkTime = 0;
  u8_t eybnet_index=0xff;
  s32 rets32;

  APP_PRINT("Common server task run!!\r\n");
  r_memset(&msg, 0, sizeof(ST_MSG));
  server = null;
  sPort = 0xFF;
  while (1) {
#ifdef _PLATFORM_M26_
    Ql_OS_GetMessage(&msg);
#endif
#ifdef _PLATFORM_L610_
    fibo_queue_get(COMMON_SERVER_TASK, (void *)&msg, 0);
#endif
    switch (msg.message) {
      case SYS_PARA_CHANGE:
        //CommonServer_close();
        //sPort = 0xff;  //
        //server = null;
        ssl_relink();
      case APP_MSG_TIMER_ID:
        //软件复位
        soft_reset_handle();

        //ret = Net_status(sPort);
        ret=state_status(sPort);
        APP_DEBUG("\r\n-->socket[%d] status %d relinkTime %d\r\n", sPort, ret, relinkTime);
        
        //APP_DEBUG("\r\n-->server=%ld\r\n",server);
        //50
        if (m_GprsActState == STATE_DNS_READY&&ret == 0xFF && relinkTime++ > 50) {
        //if (ret == 0xFF && relinkTime++ > 50) {   
          //if ((m_GprsActState == STATE_DNS_READY)&&(ret == 0xFF)) {   
          if (server == null) {
            //for (ret = 0; ret < sizeof(serverTab) / sizeof(serverTab[0]); ret++) {
              ret=0;        //仅有国网
              APP_DEBUG("\r\n-->CommonServer[%d] check!!!!!\r\n", ret);
              if (serverTab[ret].check() >= 0) {
              //if (server_con.check() >= 0) {
                APP_DEBUG("\r\n-->state grid is exist\r\n");
                server = &serverTab[ret];
                //server = &server_con;
                break;
              }
            //}
          }


          if (server != null && server->api != null) {
            // Configuration the protocol initial parameters
             APP_DEBUG("\r\n-->server != null && server->api != null\r\n"); 
            ServerAddr_t *serverAddr = server->api->getAddr();

            server->api->init();
            APP_DEBUG("\r\n-->server init %s !\r\n", server->api->name);
            if (serverAddr != null) {
              overtime = 0;
              APP_DEBUG("\r\n-->state grid:sPort=%d\r\n",sPort);
              APP_DEBUG("\r\n-->state grid netManage[sPort].flag=%d\r\n",netManage[sPort].flag);
              sPort = Net_connect(serverAddr->type, serverAddr->addr, serverAddr->port, dataProcess);
              memory_release(serverAddr);
              
            }
            
          } else {
              APP_DEBUG("\r\n-->server->api == null?\r\n");
            break;
            
          }

        }
        else{
            //if (overtime++ > server->api->waitTime){
            if (overtime++ > 660){
                overtime = 20;
						    //Net_close(sPort);
                ssl_relink();
					  }
        }
        if (server != null && server->api != null){
              server->api->run(ret);
              //if(Net_status(sPort)==L610_SUCCESS)
              //  rets32=ssl_rec();
        }

        case COMMON_SERVER__DATA_PROCESS:
          if (server != null && server->api != null){
              server->api->process();
            }
        break;
        //defualt:
        //break;
    }
  }
}
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/

/*
void CommonServerDataSend(Buffer_t *buf) {
  if (buf != null && buf->payload != null && buf->lenght > 0) {
//    output(buf);
    Net_send(sPort, buf->payload, buf->lenght);
  }
}

*/

/******************************************************************************                    
 * introduce:           
 * parameter:                      
 * return:                
 * author:           Luee                                              
 *****************************************************************************/
static void dataProcess(u8_t port, Buffer_t *buf) {
  output(buf);
	overtime = 0;
	sPort = port;
	if (server->api->add(buf, CommonServerDataSend) == 0)
    {
        Eybpub_UT_SendMessage(COMMON_SERVER_TASK, COMMON_SERVER__DATA_PROCESS, 0, 0);
    }
    memory_release(buf);

  /*
  output(buf);
  overtime = 0;
  sPort = port;
  if (server->api->add(buf, CommonServerDataSend) == 0) {
#ifdef _PLATFORM_M26_    
    Ql_OS_SendMessage(COMMON_SERVER_TASK, COMMON_SERVER__DATA_PROCESS, 0, 0);
#endif
#ifdef _PLATFORM_L610_
    Eybpub_UT_SendMessage(COMMON_SERVER_TASK, COMMON_SERVER__DATA_PROCESS, 0, 0);
#endif
  }
  memory_release(buf);
  */
}

/******************************************************************************                    
 * introduce:           
 * parameter:                      
 * return:                
 * author:           Luee                                              
 *****************************************************************************/
void state_rec_process(Buffer_t *buf)
{
  dataProcess(sPort,buf);

}

/******************************************************************************                     
* introduce:        向服务器发送数据通用接口       
* parameter:        none                 
* return:           none         
* author:           Luee                                              
*******************************************************************************/
void CommonServerDataSend(Buffer_t *buf)
{
	if (buf != null && buf->payload != null && buf->lenght > 0)
	{
        s32 ret;
        //overtime=0;
        //网络空闲才发送
       while(eybnet_para.send_status)
            fibo_taskSleep(50);   //200
        statenet_para.send_status=true;     //标志国网正在发送
        APP_DEBUG("\r\nssl sending\r\n");
        ret = fibo_ssl_sock_send(sslsock, (u8 *)buf->payload, buf->lenght);
        if(ret<0){
          APP_DEBUG("\r\n-->state grid ssl send fail ret=%ld\r\n", ret);
        }else{
          //APP_DEBUG("\r\n-->state grid ssl send success ret=%ld\r\n", ret);
          //log_hex((UINT8 *)buf->payload, buf->lenght);
          //print_buf((UINT8 *)buf->payload, buf->lenght);
        }
        statenet_para.send_status=false;    //发送结束
        //ret=ssl_rec();
	}
} 

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
static void output(Buffer_t *buf) {
  APP_DEBUG("%s len: %d\r\n", server->api->name,  buf->lenght);
  if (buf->lenght < 0x60) {
    u8_t *str = memory_apply(buf->lenght * 3 + 8);
    if (str != null) {
      int l = Swap_hexChar((char *)str, buf->payload, buf->lenght, ' ');
      Debug_output(str, l);
      APP_DEBUG("\r\n");
      memory_release(str);
    }
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
void CommonServer_DeviceInit(void) {
  if (server != null && server->api != null && server->api->deviceInit != null) {
    server->api->deviceInit();
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
void CommonServer_close(void) {
  //Net_close(sPort);
  state_close(sPort);
  if (server != null && server->api != null) {
    server->api->sclose();
  }
}

/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
*******************************************************************************/
void CommonServerData_clean(void) {
  int i;

  for (i = 0; i < sizeof(serverTab) / sizeof(serverTab[0]); i++) {
    if (serverTab[i].api != null && serverTab[i].api->clean != null) {
      serverTab[i].api->clean();
    }
  }
}
/******************************************************************************/

