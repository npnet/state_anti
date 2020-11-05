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

#define SET_DEVICE_NUMBER 1
#define GET_DEVICE_NUMBER 0

#define DEVICE_LOG(format, ...) OSI_PRINTFI("[DEVICE_LOG][%s:%d]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

static void prvThreadEntry(void *param)
{
	OSI_LOGI(0,"application thread enter, param 0x%x", param);
	//INT8 set_device_NUM[] = "caiden\0chai\0fibocom";
	char ch [15] = {0x05,0x00,0x01,0x31,0x32,0x00,0x33,0x20,0x21,0x00,0x34,0x00,0x01,0x02,0x00};
	INT32 ret;
	INT32 setNUM = SET_DEVICE_NUMBER;
	//INT32 getNUM = GET_DEVICE_NUMBER;
	char *get_device_NUM;
	size_t length = 15;
	
	ret = fibo_set_get_deviceNUM(setNUM,ch,length);
	osiDebugEvent(0x00001190);
	if(ret < 0)
	{
		osiDebugEvent(0x00001191);
		OSI_LOGI(0, "fibo_set_get_deviceNUM failed");
	}	
	fibo_taskSleep(3 *1000);
	get_device_NUM = malloc(80);
	ret = fibo_get_device_num(get_device_NUM, length);
	//ret = fibo_set_get_deviceNUM(getNUM,get_device_NUM);
	if(ret < 0)
	{
		OSI_LOGI(0, "fibo_set_get_deviceNUM failed");
		osiDebugEvent(0x00001199);
	}
	DEVICE_LOG("read device number from fs get_device_NUM:%s",get_device_NUM);
	for(int i = 0 ; i < length ; i++)
	{
		DEVICE_LOG("get_device_NUM get_device_NUM[%d]=%d",i,get_device_NUM[i]);
	}
	fibo_thread_delete();
}




void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
