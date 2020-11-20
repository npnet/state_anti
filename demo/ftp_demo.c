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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('T', 'T', 'S', 'Y')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

int FTP_STATUS = 0;
extern void test_printf(void);




static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


void ftp_read()
{
	char *ftppath = "qwer.txt";
	char *localpath = "/FFS/qwer.txt";

	int ret = fibo_ftp_read(ftppath,localpath);
	if (ret == 1)
		OSI_PRINTFE("ftp_read!");
	else 
		OSI_PRINTFE("ftp_read fail!");
}


void ftp_close()
{
	int ret = fibo_ftp_close();
	if (ret == 1)
		OSI_PRINTFE("ftp_close!");
	else 
		OSI_PRINTFE("ftp_close fail!");
}

void ftpThreadEntry_firmware(void *param)
{
	while(1){
		if(FTP_STATUS == 1)
		{   
			ftp_read();
			break;
		}
		else
			OSI_PRINTFE("TEST_TYPE value is setting error, please set again!");
		fibo_taskSleep(1000);
	}
	while(1){
		if(FTP_STATUS == 2)
		{   
			ftp_close();	
			break;
		}
		else
		OSI_PRINTFE("TEST_TYPE value is setting error, please set again!");
		fibo_taskSleep(1000);
	}
	while(1){
		if(GAPP_SIG_FTPREAD_SUCCESS)
			break;
		fibo_taskSleep(1000);
	}
	fibo_thread_delete();
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	int test = 1;
    OSI_PRINTFI("coapapi wait network");
    reg_info_t reg_info;

    while (test)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            test = 0;
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);
            OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        }
    }

    fibo_taskSleep(5 * 1000);
    char *url = "47.110.234.36"; 
    char *username = "ftpuser1"; 
    char *passwd = "test";
	
    fibo_taskSleep(5000);
    if(1 == fibo_ftp_open(url,username,passwd))
    {
        OSI_LOGI(0,"FTPOPEN");
    }
    else
    {
        OSI_LOGI(0,"FTP OPEN FAIL");
    }
	fibo_thread_create(ftpThreadEntry_firmware, "ota-thread", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);  
	while(1){
		fibo_taskSleep(1000);
	}
	fibo_thread_delete();
	
	
}
static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
    case GAPP_SIG_FTPOPEN_SUCCESS:
    {
        OSI_LOGI(0, "ftpopen", 0);
		FTP_STATUS = 1;
    }
	break;
	case GAPP_SIG_FTPWRITE_SUCCESS:
	{
        OSI_LOGI(0, "write success", 0);
		
    }
	break;
	case GAPP_SIG_FTPREAD_SUCCESS:
	{
		OSI_LOGI(0, "read success", 0);
		FTP_STATUS = 2;
	}
	break;
	case GAPP_SIG_FTPCLOSE_SUCCESS:
	{
		OSI_LOGI(0, "ftpclose success", 0);
	}
    default:
    {
        break;
    }
    }
    OSI_LOGI(0, "test %d",sig);
}
static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
    fibo_thread_create(prvThreadEntry, "mythread", 1024 * 4, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

