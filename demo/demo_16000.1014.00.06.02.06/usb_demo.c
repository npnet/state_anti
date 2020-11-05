/* Copyright (C) 2018 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * This software is supplied "AS IS" without any warranties.
 * RDA assumes no responsibility or liability for the use of the software,
 * conveys no license or title under any patent, copyright, or mask work
 * right to the product. RDA reserves the right to make changes in the
 * software without notification.  RDA also make no representation or
 * warranty that such application will be suitable for the specified use
 * without further testing or modification.
 */

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


extern void test_printf(void);

int usbState = 0;

typedef enum
{
    DRV_SERIAL_EVENT_BROKEN = (1 << 3),      ///< usb port broken
    DRV_SERIAL_EVENT_READY = (1 << 4),       ///< usb port ready
    DRV_SERIAL_EVENT_OPEN = (1 << 5),        ///< open by host
    DRV_SERIAL_EVENT_CLOSE = (1 << 6),       ///< close by host
}usbEvent;


UINT32 usbdev_daemon_sem = 0;

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}
void recv_cb(atDevice_t *th, void *buf, size_t size, void *arg)
{
    char *buffer = malloc(size + 1);
    memset(buffer, 0, size + 1);
    strncpy(buffer, (const char *)buf, size);
    OSI_PRINTFI("usbdev test:%s", buffer);
}

int UsbDev_Event (int arg)
{
	usbState = arg;
        OSI_PRINTFI("usbdev report msg  %d\n", usbState);	
	switch (usbState)
	{
	     case DRV_SERIAL_EVENT_BROKEN:
	     {
	         OSI_PRINTFI("the usbdev had broken!");	    
		   break;
	     }
	    case DRV_SERIAL_EVENT_READY:
	    {
	         OSI_PRINTFI("the usbdev had ready!");	    
		   break;
	    }
	    case DRV_SERIAL_EVENT_OPEN:
	   {
	        OSI_PRINTFI("the usbdev had opened! ");
	        fibo_sem_signal(usbdev_daemon_sem);
		 break;
	   }
	   case DRV_SERIAL_EVENT_CLOSE:
	   {
	        OSI_PRINTFI("the usbdev had closed!");
		 break;
	   }
	  default:
	  {
	       OSI_PRINTFI("usbdev unkonw state!");
	  	usbState = 0;
		break;
	  }
	}
	return 0;
}


static void prvThreadEntry(void *param)
{
    char *test = "test usb";
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);
    atDevice_t *th;
	
    usbdev_daemon_sem = fibo_sem_new(0);    
    th = FIBO_usbDevice_init(DRV_NAME_USRL_COM6, recv_cb);
    FIBO_usbDevice_State_report(th, (Report_UsbDev_Event)UsbDev_Event);
    fibo_taskSleep(5000);
    while(1)
    {
          if(usbState == DRV_SERIAL_EVENT_OPEN)
          {
               //OSI_LOGI(0, "usbDev had opend start send %d", usbState);
	        FIBO_usbDevice_send(th, (void *)test, strlen(test));
          }
	   else if(usbState == DRV_SERIAL_EVENT_BROKEN)
	   {
	        FIBO_usbDevice_Deinit(th);
	        th = FIBO_usbDevice_init(DRV_NAME_USRL_COM6, recv_cb);
	        FIBO_usbDevice_State_report(th, (Report_UsbDev_Event)UsbDev_Event);	   
	        OSI_LOGI(0, "usbDev had broken %d", usbState);
		 fibo_sem_wait(usbdev_daemon_sem);  
	   }
	   else
	   {
	        OSI_LOGI(0, "usbDev state %d", usbState);
	   }
	    fibo_taskSleep(2000);
	    //FIBO_usbDevice_Deinit(th);
    }
    FIBO_usbDevice_Deinit(th);
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
//    fibo_thread_create(usbDaemon, "usbDaemon", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

