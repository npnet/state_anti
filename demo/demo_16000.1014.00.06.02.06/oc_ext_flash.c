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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('E', 'T', 'F', 'L')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"




static void prvThreadEntry(void *param)
{	

    bool por=false;
	UINT32 ulId; 
	UINT32 ulCapacity;
	
    if (!fibo_ext_flash_poweron(1, 0, 5))
    {
		OSI_LOGI(0, "fibo_ext_flash_poweron fail");
    }
	if(0!=fibo_ext_flash_init(1))
	{
		OSI_LOGI(0, "fibo_ext_flash_init fail");
	}
	if (0!=fibo_ext_flash_info(&ulId, &ulCapacity))
	{
		OSI_LOGI(0, "fibo_ext_flash_info fail");
	}
	OSI_LOGI(0, "fibo_ext_flash_info ID=0x%x Cap=0x%x.",ulId,ulCapacity);
	UINT8 *writeBuf=(UINT8 *)malloc(0x10000*(sizeof(UINT8)));
    if (writeBuf ==NULL)
	{
		OSI_LOGI(0, "fibo_ext_flash_info malloc fail");
	}
	for (UINT32 i=0; i<0x10000; i++)
	{
		writeBuf[i]=0x55;
	}

 	if (0!=fibo_ext_flash_erase(0, ulCapacity))
	{
		OSI_LOGI(0, "fibo_ext_flash_erase fail");
	}
    osiThreadSleep(2000);
	if (0!=fibo_ext_flash_write(0, writeBuf, 0x10000))
	{
		OSI_LOGI(0, "fibo_ext_flash_write fail");
	}
	else
	{
		OSI_LOGI(0, "fibo_ext_flash_write success");
	}
	
	while(1)
    { 

		 osiThreadSleep(500);	
    }
    OSI_LOGD(0, "prvThreadEntry osiThreadExit");
    osiThreadExit();

}


void* appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(prvThreadEntry, "mythread", 4096, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
