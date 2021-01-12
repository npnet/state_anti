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

// #include "SAJ.h"
// #include "Hanergy.h"
#include "StateGrid.h"
#include "CommonServer.h"

typedef int (*ServerCheck)(void);

typedef struct {
  ServerCheck check;
  const CommonServer_t *api;
} CommonServerTab_t;

static int common(void);

static const CommonServerTab_t serverTab[] = {
//  {Hanergy_check, &HanergyServer},
//  {SAJ_check, &SAJ_API},
  {StateGrid_check, &StateGrid_API},
  {common, null},
};

static u8_t sPort;
static int overtime;
static CommonServerTab_t  const *server;

static void output(Buffer_t *buf);
static void dataProcess(u8_t port, Buffer_t *buf);

int common(void) {
  return 0;
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
  u16_t relinkTime = 0;

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
        CommonServer_close();
        sPort = 0xff;  //
        server = null;
      case APP_MSG_TIMER_ID:
        ret = Net_status(sPort);
        APP_DEBUG("socket[%d] status %d relinkTime %d\r\n", sPort, ret, relinkTime);
        if (ret == 0xFF && relinkTime++ > 50) {
//        relinkTime = 0;
          if (server == null) {
            for (ret = 0; ret < sizeof(serverTab) / sizeof(serverTab[0]); ret++) {
              // Select remote server
              APP_DEBUG("CommonServer[%d] check!!!!!\r\n", ret);
              if (serverTab[ret].check() >= 0) {
                server = &serverTab[ret];
                break;
              }
            }
          }

          if (server != null && server->api != null) {
            // Configuration the protocol initial parameters
            ServerAddr_t *serverAddr = server->api->getAddr();

            server->api->init();
            APP_DEBUG("server %s init!\r\n", server->api->name);
            if (serverAddr != null) {
              overtime = 0;
              sPort = Net_connect(serverAddr->type, serverAddr->addr, serverAddr->port, dataProcess);
              memory_release(serverAddr);
            }
          } else {
            break;
          }
        }else {
          if (server == NULL) {
            break;
          }
          if (overtime++ > server->api->waitTime) {
            overtime = 20;
            Net_close(sPort);
          }
        }
        if (server != null && server->api != null) {
          server->api->run(ret);
        }
      case COMMON_SERVER__DATA_PROCESS:
        if (server != null && server->api != null) {
          server->api->process();
        }
        break;
      default:
        break;
    }
  }
}
/*******************************************************************************
  * @brief
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
void CommonServerDataSend(Buffer_t *buf) {
  if (buf != null && buf->payload != null && buf->lenght > 0) {
//    output(buf);
    Net_send(sPort, buf->payload, buf->lenght);
  }
}

static void dataProcess(u8_t port, Buffer_t *buf) {
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
  Net_close(sPort);
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

