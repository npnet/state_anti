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

/* 
  * set the value to execute different process 
  * 1 : test app update
  * 2 : test fimware update
  * 3 : test app download and update
  * 4 : test firmware download and update
  */
#define TEST_TYPE				1

#define def_max_ServerAdrLen	256		
#define def_max_VersionLen      50

typedef struct
{
		bool download_flag;
		uint8_t ServerAdrLen;
		uint8_t ServerAdr[def_max_ServerAdrLen];
		uint8_t VersionLen;
		uint8_t Version[def_max_VersionLen];
        uint8_t tag[def_max_VersionLen];
		uint8_t md5[33];
}OtaParaTypeDef;

struct pdphandle{
	UINT32 sem;
	char success;
};

OtaParaTypeDef otaData;
struct pdphandle g_pdp_handle;


static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

void app_download_test()
{
	int ret;
	char *url = "http://xatest.fibocom.com:8000/httpfota/test1/lobin/hello_flash.img";

	ret = fibo_app_dl(0, url, NULL, NULL, NULL);
	if(ret < 0){
		OSI_PRINTFE("fibo_app_dl fail.");
		goto fexit;
	}
	OSI_PRINTFE("fibo_app_dl is OK.");
	
	for (int n = 0; n < 5; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }

	ret = fibo_app_update();
	if(ret < 0){
		OSI_PRINTFE("fibo_app_update fail.");
		goto fexit;
	}
	OSI_PRINTFE("fibo_app_update is OK.");
fexit:
	return;
}

void firmware_download_test()
{
	int ret;
	char *url = "http://xatest.fibocom.com:8000/httpfota/test1/lobin/fota.bin";

	ret = fibo_firmware_dl(0, url, NULL, NULL, NULL);
	if(ret < 0){
		OSI_PRINTFE("fibo_firmware_dl fail.");
		goto fexit;
	}
	OSI_PRINTFE("fibo_firmware_dl is OK.");
	
	for (int n = 0; n < 5; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }

	ret = fibo_firmware_update();
	if(ret < 0){
		OSI_PRINTFE("fibo_firmware_update fail.");
		goto fexit;
	}
	OSI_PRINTFE("fibo_firmware_update is OK.");
fexit:
	return;
}

void app_update_test()
{
	int ret;
	char *url = "http://xatest.fibocom.com:8000/httpfota/test1/lobin/hello_flash.img";

	ret = fibo_open_ota(0, (UINT8 *)url, NULL, NULL, NULL);
	if(ret < 0){
		OSI_PRINTFE("fibo_open_ota fail.");
		goto fexit;
	}
	OSI_PRINTFE("fibo_open_ota is OK.");
	
	for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }
	
fexit:
	return;
}

void firmware_update_test()
{
	int ret;
	char *url = "http://xatest.fibocom.com:8000/httpfota/test1/lobin/fota.bin";

	ret = fibo_firmware_ota(0, (UINT8 *)url, NULL, NULL, NULL);
	if(ret < 0){
		OSI_PRINTFE("fibo_firmware_ota fail.");
		goto fexit;
	}
	OSI_PRINTFE("fibo_firmware_ota is OK.");
	
	for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }
	
fexit:
	return;
}



void otaThreadEntry_firmware(void *param)
{
	if(TEST_TYPE == 1)
		app_update_test();
	else if(TEST_TYPE == 2)
		firmware_update_test();
	else if(TEST_TYPE == 3)
		app_download_test();
	else if(TEST_TYPE == 4)
		firmware_download_test();
	else
		OSI_PRINTFE("TEST_TYPE value is setting error, please set again!");
	
	fibo_thread_delete();
}


static void prvThreadEntry(void *param)
{
	int 			ret = 0;
	int 			test = 1;
	reg_info_t 		reg_info;
	UINT8 			ip[50];
	UINT8 			cid_status;

	INT8 			cid = 1;
	CFW_SIM_ID 		sim_id = CFW_SIM_0;
	
    memset(&ip, 0, sizeof(ip));
	memset(&g_pdp_handle, 0, sizeof(g_pdp_handle));
	memset(&otaData, 0, sizeof(otaData));

	ret = fibo_PDPStatus(cid, ip, &cid_status, sim_id);
	if(ret != 0){
		OSI_PRINTFI("[%s-%d]cid or sim input param error.", 
								__FUNCTION__, __LINE__);
		goto p_exit;
	}

	g_pdp_handle.sem = fibo_sem_new(0);
	if(cid_status == 1){
		/* already active PDP */
	}
	else{
		while (test)
	    {
	        fibo_getRegInfo(&reg_info, 0);
	        fibo_taskSleep(1000);
	        OSI_PRINTFI("[%s-%d]now creg is not OK...", __FUNCTION__, __LINE__);
	        if (reg_info.nStatus == 1)
	        {
	        	OSI_PRINTFI("[%s-%d]now creg is OK, begin active ...", __FUNCTION__, __LINE__);
	            ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
	            OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	            fibo_sem_wait(g_pdp_handle.sem);
	            OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
				if(g_pdp_handle.success == 1){
	            	test = 0;
					fibo_sem_free(g_pdp_handle.sem);
				}
	        }
	    }
	}
	fibo_taskSleep(3000);
	fibo_thread_create(otaThreadEntry_firmware, "ota-thread", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);  

p_exit:
	fibo_thread_delete();
}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
	//fibo_PDPActive /fibo_asyn_PDPActive  pdp active status report
	case GAPP_SIG_PDP_ACTIVE_IND:
	{
		g_pdp_handle.success = 1;
		fibo_sem_signal(g_pdp_handle.sem);
		OSI_PRINTFI("pdp active is OK.");
	}
	break;

	//GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND
	case GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND:
	{
		g_pdp_handle.success = 0;
		fibo_sem_signal(g_pdp_handle.sem);	
		OSI_PRINTFI("pdp active is fail.");
	}

	//PDP in active state, deactive indicator received from modem  
	case GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND:
	{
			
	}
	
	//fibo_PDPRelease /fibo_asyn_PDPRelease pdp deactive status report
	case GAPP_SIG_PDP_RELEASE_IND:
	{
		
	}
	break;
		
    default:
    {
        break;
    }
    }
}


static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};


void * appimg_enter(void *param)
{
	OSI_LOGI(0, "application image enter, param 0x%x", param);
	prvInvokeGlobalCtors();
	fibo_thread_create(prvThreadEntry, "mythread", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
	return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
