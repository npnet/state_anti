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
	int ret;
	char data[64] = {0};
	OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }
	
	fibo_inner_flash_init();
	
	ret = fibo_inner_flash_read(0x340000-0x1000, data, sizeof(data));
	if(ret == 0){
		OSI_PRINTFE("read OK : data = %s", data);
	}
	
	memset(data, 0, sizeof(data));
	strcpy(data, "chaixiaoqiang is OK,d affjksjkfskfsfsdfjk");
	fibo_inner_flash_erase(0x340000-0x1000, 0x1000);
	ret = fibo_inner_flash_write(0x340000-0x1000, data, sizeof(data));
	if(ret == 0){
		OSI_PRINTFE("write OK : data = %s", data);
	}
	
	ret = fibo_inner_flash_read(0x340000-0x1000, data, sizeof(data));
	if(ret == 0){
		OSI_PRINTFE("read OK : data = %s", data);
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
