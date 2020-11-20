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

typedef enum
{
	GATT_DISCONNECT = 0x00,
	GATT_CONNECT,
}GATT_CONNECT_STAT;


static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


static gatt_chara_def_short_t wifi_mgr_chara = {{
	ATT_CHARA_PROP_READ | ATT_CHARA_PROP_WRITE,
	0,
	0,
	0x2a06&0xff,
	0x2a06>>8
}};
static gatt_chara_def_short_t wifi_mgr_chara1 = {{
	ATT_CHARA_PROP_READ | ATT_CHARA_PROP_NOTIFY,
	0,
	0,
	0x2a00&0xff,
	0x2a00>>8
}};


static gatt_chara_def_short_t wifi_mgr_chara2 = {{
	ATT_CHARA_PROP_READ | ATT_CHARA_PROP_WRITE,
	0,
	0,
	0x2906&0xff,
	0x2906>>8
}};


char wifimgr_value[100] = "123456789012345678901234567890";
char wifimgr_ccc_cfg[100] = {0};
char device_name[100] = "8910_ble";
char notify_test[100] = "notify_test";

uint8_t test_service[2] = {0x1810&0xff, 0x1810 >> 8};
uint8_t add_service[2] = {0x180d&0xff, 0x180d >> 8};

UINT16 acl_handle = 0;
UINT8 gatt_connect_status = GATT_DISCONNECT;



char temp_data[512] ={0};
UINT8 data_write_callback(void *param)
{
	//UINT8 datalen = 0;
	OSI_PRINTFI("[AUTO_BLE][%s:%d]wifimgr_value=%s", __FUNCTION__, __LINE__,wifimgr_value);
	#if 0
	memcpy(temp_data,wifimgr_value,pAttr->valueLen);
	memset(wifimgr_value,0,sizeof(wifimgr_value));
	memcpy(wifimgr_value,temp_data,pAttr->valueLen);
	#endif
	return 0;
	
}


UINT8 wifi_changed_cb(void *param)
{
	OSI_PRINTFI("[AUTO_BLE][%s:%d] param = %p,wifimgr_value=%s", __FUNCTION__, __LINE__,param,wifimgr_value);
	gatt_le_data_info_t notify;
	notify.att_handle = wifi_mgr_chara1.value[2] << 8 | wifi_mgr_chara1.value[1];
	memcpy((char *)&device_name[0],"notify_test",strlen((char *)"notify_test"));
	OSI_PRINTFI("[AUTO_BLE][%s:%d] device_name = %s", __FUNCTION__, __LINE__,param,device_name);
	notify.length = sizeof(device_name);
	notify.data = (UINT8 *)&(device_name)[0];
	notify.acl_handle = acl_handle;
	return 0;
}
UINT8 wifimgr_value_write_cb(void *param)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d] param = %p,wifimgr_value=%s", __FUNCTION__, __LINE__,param,wifimgr_value);
	return 0;
}

UINT8 wifimgr_value_read_cb(void *param)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d] param = %p,wifimgr_value=%s,wifimgr_value:%s", __FUNCTION__, __LINE__,param,wifimgr_value,wifimgr_ccc_cfg);
	return 0;
}

uint8_t fibo_send_notify(uint16_t datalen, uint8_t *data)
{
	memset(device_name,0,sizeof(device_name));
	memcpy(device_name,data,datalen);
	gatt_le_data_info_t notify;
	notify.att_handle = wifi_mgr_chara1.value[2] << 8 | wifi_mgr_chara1.value[1];
	notify.length = datalen;
	notify.data = (UINT8 *)&device_name[0];
	notify.acl_handle = acl_handle;//no effect ,one connect acl_handle
	fibo_ble_notify((gatt_le_data_info_t *)&notify,GATT_NOTIFICATION);
	OSI_PRINTFI("[AUTO_BLE][%s:%d] notify:%s", __FUNCTION__, __LINE__,data);
	return 0;
}

gatt_element_t config_wifi_service[]={

	{
		//creat service and fill UUID
	    sizeof(test_service),
	   	ATT_PM_READABLE,
	    {ATT_UUID_PRIMARY},
	    ATT_FMT_SHORT_UUID | ATT_FMT_GROUPED,
	    (void *)test_service,
	    NULL,
	    NULL
	},
	{
		//creat chara and fill permission
	    sizeof(wifi_mgr_chara),
		ATT_PM_READABLE,
	    {ATT_UUID_CHAR},
	    ATT_FMT_SHORT_UUID | ATT_FMT_FIXED_LENGTH,
	    (void *)&wifi_mgr_chara,
	    wifi_changed_cb,//cb
	    wifimgr_value_read_cb//read callback
	},
	{
	    sizeof(wifimgr_value),
		ATT_PM_READABLE | ATT_PM_WRITEABLE,
	    {0x2a06},
	    ATT_FMT_SHORT_UUID | ATT_FMT_WRITE_NOTIFY | ATT_FMT_FIXED_LENGTH,
	    (void *)wifimgr_value,
	    data_write_callback,
	    NULL
	},
	{
		//des
	    sizeof(wifimgr_ccc_cfg),
	    ATT_PM_READABLE | ATT_PM_WRITEABLE,
	    {ATT_UUID_CLIENT},
	    ATT_FMT_SHORT_UUID | ATT_FMT_WRITE_NOTIFY|ATT_FMT_FIXED_LENGTH,
	    (void *)wifimgr_ccc_cfg,
	    wifi_changed_cb,
	    NULL
	},
	{
	    sizeof(wifi_mgr_chara1),
		ATT_PM_READABLE,
	    {ATT_UUID_CHAR},
	    ATT_FMT_SHORT_UUID | ATT_FMT_FIXED_LENGTH,
	    (void *)&wifi_mgr_chara1,
	    NULL,
	    NULL
	},
	{
	    sizeof(device_name),
		ATT_PM_READABLE,
	    {0x2a00},
	    ATT_FMT_SHORT_UUID | ATT_FMT_WRITE_NOTIFY | ATT_FMT_FIXED_LENGTH,
	    (void *)device_name,
	    NULL,
	    NULL
	},
	{
	    sizeof(wifimgr_ccc_cfg),
	    ATT_PM_READABLE | ATT_PM_WRITEABLE,
	    {ATT_UUID_CLIENT},
	    ATT_FMT_SHORT_UUID | ATT_FMT_WRITE_NOTIFY|ATT_FMT_FIXED_LENGTH,
	    (void *)wifimgr_ccc_cfg,
	    wifi_changed_cb,
	    NULL
	},
	{
		//creat service and fill UUID
	    sizeof(add_service),
	   	ATT_PM_READABLE,
	    {ATT_UUID_PRIMARY},
	    ATT_FMT_SHORT_UUID | ATT_FMT_GROUPED,
	    (void *)add_service,
	    NULL,
	    NULL
	},
	{
		//creat chara and fill permission
	    sizeof(wifi_mgr_chara2),
		ATT_PM_READABLE,
	    {ATT_UUID_CHAR},
	    ATT_FMT_SHORT_UUID | ATT_FMT_FIXED_LENGTH,
	    (void *)&wifi_mgr_chara2,
	    wifi_changed_cb,//cb
	    wifimgr_value_read_cb//read callback
	},
	{
		//fill chara value 
	    sizeof(wifimgr_value),
		ATT_PM_READABLE | ATT_PM_WRITEABLE,
	    {0x2906},
	    ATT_FMT_SHORT_UUID | ATT_FMT_WRITE_NOTIFY | ATT_FMT_FIXED_LENGTH,
	    (void *)wifimgr_value,
	    data_write_callback,
	    NULL
	},
	{
		//des
	    sizeof(wifimgr_ccc_cfg),
	    ATT_PM_READABLE | ATT_PM_WRITEABLE,
	    {ATT_UUID_CLIENT},
	    ATT_FMT_SHORT_UUID | ATT_FMT_WRITE_NOTIFY|ATT_FMT_FIXED_LENGTH,
	    (void *)wifimgr_ccc_cfg,
	    wifi_changed_cb,
	    NULL
	},

};

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
	   
		//fibo_PDPActive /fibo_asyn_PDPActive  pdp active status report
		case GAPP_SIG_PDP_ACTIVE_IND:
		{
			UINT8 cid = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("sig_res_callback  cid = %d", cid);
			va_end(arg);

		}
		break;

		//fibo_PDPRelease /fibo_asyn_PDPRelease pdp deactive status report
		case GAPP_SIG_PDP_RELEASE_IND:
		{
			UINT8 cid = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("sig_res_callback  cid = %d", cid);
			va_end(arg);

		}
		break;

		//GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND
		case GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND:
		{
		
			UINT8 cid = (UINT8)va_arg(arg, int);
			UINT8 state = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("[%s-%d]cid = %d,state = %d", __FUNCTION__, __LINE__,cid,state);
			va_end(arg);
		}
		break;

		//PDP in active state, deactive indicator received from modem  
		case GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND:
		{
		
			UINT8 cid = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("[%s-%d]cid = %d", __FUNCTION__, __LINE__,cid);
			va_end(arg);
		}
		break;

		case GAPP_SIG_BLE_SET_ADV_IND:
		{
			UINT8 type = (UINT8)va_arg(arg, int);
			UINT8 state = (UINT8)va_arg(arg, int);
			OSI_PRINTFI("[AUTO_BLE][%s:%d]type=%d,state=%d", __FUNCTION__, __LINE__,type,state);				
		}
		break;
		case GAPP_SIG_BLE_CONNECT_IND:
		{
			int type = (int)va_arg(arg, int);
			int state = (int)va_arg(arg, int);
			UINT8 addr_type = (UINT8 )va_arg(arg, int);
			UINT8 *addr = (UINT8 *)va_arg(arg, UINT8 *);
			va_end(arg);
			OSI_PRINTFI("[AUTO_BLE][%s:%d]type=%d,state=%d,%d,%s", __FUNCTION__, __LINE__,type,state,addr_type,addr);

            acl_handle = (int)(type);
			if(state == 0)
			{
				fibo_ble_enable_dev(1); // open broadcast

			}
		}
		break;
		case GAPP_SIG_BLE_SCAN_REPORT:
		{
			OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
			UINT8 type = (UINT8)va_arg(arg, int);
			char *add = (char *)va_arg(arg, char *);
			char *name = (char *)va_arg(arg, char *);
			if(name != NULL)
			{
			    
				if(!strcasecmp("xiaomi_df123",name))
				{
				    fibo_ble_scan_enable(0);
					//OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
					fibo_taskSleep(500);
					OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,add=%s", __FUNCTION__, __LINE__,name,add);
					//fibo_ble_connect(1,type,add);
					//fibo_ble_scan_enable(1);
					break;
				}
			}
		}
		break;
		
	    default:
	    {
	        break;
	    }
    }
    OSI_LOGI(0, "test");
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	UINT32 size;

	fibo_taskSleep(10000);

    fibo_bt_onoff(1);
	fibo_taskSleep(2000);
	//fibo_bt_onoff(0);
	//fibo_taskSleep(2000);
	//fibo_bt_onoff(1);
	//fibo_taskSleep(2000);
	fibo_ble_set_read_name(0,(uint8_t *)"8910_ble",0); // set ble name 
	size = sizeof(config_wifi_service)/sizeof(gatt_element_t);
	fibo_ble_add_service_and_characteristic(config_wifi_service,size); //create serive and characteristic
	fibo_taskSleep(2000);

	fibo_ble_scan_enable(1);

	fibo_taskSleep(20000);

	fibo_ble_enable_dev(1); // open broadcast
	fibo_taskSleep(20000);
#if 0
	fibo_send_notify(3,"abc");

	fibo_taskSleep(20000);

	fibo_send_notify(3,"WER");

	fibo_taskSleep(20000);

	fibo_ble_enable_dev(0); // close broadcast

	fibo_taskSleep(2000);

	char *adv_data = NULL;
	adv_data = fibo_malloc(20);
	memcpy(adv_data,"0406090102",10);

	fibo_ble_set_dev_data(5,adv_data);

	fibo_ble_enable_dev(1); // open broadcast



    fibo_taskSleep(20000);
	fibo_ble_enable_dev(0); // close broadcast
    memcpy(adv_data,"050209010205",12);
	fibo_ble_set_dev_data(6,adv_data); // set broadcast data

	fibo_ble_enable_dev(1); // open broadcast



	fibo_taskSleep(20000);
	fibo_ble_enable_dev(0); // close broadcast
    fibo_ble_set_dev_param(6,60,80,0,0,-1,7,0,NULL);
	fibo_ble_enable_dev(1); // open broadcast
#endif
	while(1)
	{
        OSI_LOGI(0, "hello world %d");
        fibo_taskSleep(10000);
	}

	test_printf();
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
