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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('D', 'B', 'L', 'E')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "bt_abs.h"

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
		
	    default:
	    {
	        break;
	    }
    }
    OSI_LOGI(0, "test");
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};

static int AddrU8IntToStrings(char *_src, char *_des)
{
	sprintf(_des, "%02x%s%02x%s%02x%s%02x%s%02x%s%02x", _src[0], ":", _src[1], ":", _src[2], ":", _src[3], ":", _src[4], ":", _src[5]);
	return 0;
}

bt_status_t fibo_connection_state_change_cb(int conn_id, int connected, bdaddr_t *addr)
{
	OSI_PRINTFI("[AUTO_BLE][%s:%d]connected=%d,conn_id=%d", __FUNCTION__, __LINE__,connected,conn_id);
	if(connected == 1)
	{
       fibo_taskSleep(600);
       fibo_ble_client_discover_all_primary_service(28,conn_id);
	}
	if(connected == 0)
	{
         // maybe need scan again and connect again
         fibo_ble_scan_enable(1);
	}
	return BT_SUCCESS;
}
bt_status_t fibo_discover_service_by_uuid_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   gatt_prime_service_t *gatt_prime_service = (gatt_prime_service_t *)parma;
   char value[100]="00001000800000805f9b34fb";
   fibo_ble_client_read_char_value_by_uuid(28, NULL, 0xffe3, gatt_prime_service->startHandle, gatt_prime_service->endHandle, 0);
   OSI_PRINTFI("[AUTO_BLE][%s:%d]uuid=%x,%d", __FUNCTION__, __LINE__,gatt_prime_service->uuid, gatt_prime_service->charNum);
   if(gatt_prime_service->pCharaList != NULL)
   {
	   fibo_ble_client_write_char_value(28, gatt_prime_service->pCharaList->handle, value, sizeof("00001000800000805f9b34fb"), 0);
	   OSI_PRINTFI("[AUTO_BLE][%s:%d]uuid=%x", __FUNCTION__, __LINE__,gatt_prime_service->uuid);
   }
   return BT_SUCCESS;
}
bt_status_t fibo_discover_service_all_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   gatt_prime_service_t *gatt_prime_service = (gatt_prime_service_t *)parma;
   OSI_PRINTFI("[AUTO_BLE][%s:%d]uuid=%x,%d", __FUNCTION__, __LINE__,gatt_prime_service->uuid,gatt_prime_service->charNum);

   if(gatt_prime_service->uuid == 0xffe3)
   {
	   fibo_ble_client_discover_all_characteristic(28,gatt_prime_service->startHandle,gatt_prime_service->endHandle,0);

   }
   OSI_PRINTFI("[AUTO_BLE][%s:%d]uuid=%x", __FUNCTION__, __LINE__,gatt_prime_service->uuid);
   return BT_SUCCESS;
}
bt_status_t fibo_char_des_data (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
    att_server_t *att_server = (att_server_t *)parma;
	uint8 test_val[2] = {0};
	test_val[0]=0x01;
	test_val[1]=0x00;
	fibo_ble_client_write_char_value(28,14,test_val,2,0);
    return BT_SUCCESS;
}
bt_status_t fibo_char_data (void *parma, UINT8 more_data)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
    att_server_t *att_server = (att_server_t *)parma;
    uint8 i = 0;
    uint8 pair_len = att_server->lastRsp.payLoad[0];
    uint8 properties;
    uint8 test_val[100] = {0};
    uint16 value_handle = 0;
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	uint8 test_value[2] = {0};
     test_value[0]=0x01;
	 test_value[1]=0x00;

    fibo_ble_client_write_char_value(28,14,test_value,2,0);
    while(i<(att_server->lastRsp.length-1)/pair_len)
    {
        OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
        properties = att_server->lastRsp.payLoad[i * pair_len +3];
        value_handle = (att_server->lastRsp.payLoad[i * pair_len + 5] << 8)| att_server->lastRsp.payLoad[i * pair_len + 4];
        OSI_PRINTFI("properties = %x", properties);
        //0x04:write without response,0x10:notify,0x08:write,0x20:indicate,0x02:read,0x01:broadcast
        #if 0
        if(properties & 0x04)  //write with no response
        {
            OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
            test_val[0]= 0xEF;
            test_val[1]= 0xFE;
            test_val[2]= 0x00;
            test_val[3]= 0x00;
            test_val[4]= 0xC6;
            test_val[5]= 0xC7;
            fibo_ble_client_write_char_value_without_rsp(28,value_handle,test_val, 6,0);
        }
        
        if(properties & 0x08)  
        {
            OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
			uint8 test_value[2] = {0};
		     test_value[0]=0x01;
			 test_value[1]=0x00;

            fibo_ble_client_write_char_value(28,14,test_value,2,0);
        }
        #endif
		 if(properties & 0x10)
		 {
             //test_val[0]= 0x01;
			 fibo_ble_client_get_char_descriptor(28,14,14,0);
		 }
        i++;
        OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
    }
    return BT_SUCCESS;
}
bt_status_t fibo_read_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   att_server_t *att_server = (att_server_t *)parma;
   return BT_SUCCESS;
}
bt_status_t fibo_read_blob_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   att_server_t *att_server = (att_server_t *)parma;
   return BT_SUCCESS;
}
bt_status_t fibo_read_multi_cb (void *parma)
{
   OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
   att_server_t *att_server = (att_server_t *)parma;
   return BT_SUCCESS;
}
bt_status_t fibo_recv_notification_cb (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	att_server_t *att_server = (att_server_t *)parma;
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastReqPdu.attValuelen,att_server->lastReqPdu.attValue);
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastNoti.length,&att_server->lastNoti.payLoad[2]);
	for(int i =0;i<att_server->lastNoti.length;i++)
	{
		OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%x", __FUNCTION__, __LINE__,att_server->lastNoti.length,att_server->lastNoti.payLoad[i]);
	}
	
	return BT_SUCCESS;
}
bt_status_t fibo_recv_indication_cb (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	att_server_t *att_server = (att_server_t *)parma;
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastReqPdu.attValuelen,att_server->lastReqPdu.attValue);
	OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%s", __FUNCTION__, __LINE__,att_server->lastNoti.length,&att_server->lastNoti.payLoad[2]);
	for(int i =0;i<att_server->lastNoti.length;i++)
	{
		OSI_PRINTFI("[AUTO_BLE][%s:%d]%d,%x", __FUNCTION__, __LINE__,att_server->lastNoti.length,att_server->lastNoti.payLoad[i]);
	}

	return BT_SUCCESS;
}

int write_enable = 0;
bt_status_t fibo_write_cb (void *parma)
{
    OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	att_req_pdu_t *att_req_pdu = (att_req_pdu_t *)parma;
	write_enable =1;
	#if 1
	uint8 test_val[6] = {0};
	OSI_PRINTFI("[AUTO_BLE][%s:%d]att_req_pdu->attValuelen=%d,att_req_pdu->attValue=%s", __FUNCTION__, __LINE__,att_req_pdu->attValuelen,att_req_pdu->attValue);
	test_val[0]= 0xEF;
	test_val[1]= 0xFE;
	test_val[2]= 0x00;
	test_val[3]= 0x00;
	test_val[4]= 0xC6;
	test_val[5]= 0xC7;
	fibo_ble_client_write_char_value_without_rsp(28,16,test_val, 6,0);
	#endif

	return BT_SUCCESS;
}
bt_status_t fibo_write_rsp_cb (void *parma)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	 UINT16 *aclHandle = (UINT16 *)parma;
	 return BT_SUCCESS;
}
bt_status_t fibo_scan_cb (void *parma,UINT8 adv_len)
{
     //OSI_PRINTFI("[AUTO_BLE][%s:%d],adv_len=%d", __FUNCTION__, __LINE__,adv_len);
	 st_scan_report_info *scan_report = (st_scan_report_info *)parma;
	 OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,addr_type:%d", __FUNCTION__, __LINE__,scan_report->name,scan_report->addr_type);

	if(scan_report->name_length > 0)
	{
	    
		if(!strcasecmp("D0000031",scan_report->name))
		{
		    
			//OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
			

			if(scan_report->bdAddress.addr[0]==0 && scan_report->bdAddress.addr[1]==0 && scan_report->bdAddress.addr[2]==0 && scan_report->bdAddress.addr[3]==0 && scan_report->bdAddress.addr[4]==0 && scan_report->bdAddress.addr[5]==0)
			{
                //searching on going
                OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,addr_type:%d", __FUNCTION__, __LINE__,scan_report->name,scan_report->addr_type);
				fibo_ble_scan_enable(1);
			}
			else
			{
			    fibo_ble_scan_enable(0);
				//fibo_taskSleep(500);
				char addr[30] = {0};
				AddrU8IntToStrings(scan_report->bdAddress.addr,addr);
				OSI_PRINTFI("[AUTO_BLE][%s:%d] %s,add=%s,addr_type:%d", __FUNCTION__, __LINE__,scan_report->name,addr,scan_report->addr_type);
				fibo_ble_connect(1,scan_report->addr_type,addr);
				

			}
			//fibo_taskSleep(500);

		}
	}
	 return BT_SUCCESS;
}
void fibo_smp_pair_success_cb (void)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	 return ;
}
void fibo_smp_pair_failed_cb(void)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]", __FUNCTION__, __LINE__);
	 return ;
}
void fibo_att_error_cb (UINT8 error_code)
{
     OSI_PRINTFI("[AUTO_BLE][%s:%d]error_code=%d", __FUNCTION__, __LINE__,error_code);
	 return ;
}


btgatt_client_callbacks_t fibo_ble_client_callback={
	.connection_state_change_cb = fibo_connection_state_change_cb,
	.discover_service_by_uuid_cb = fibo_discover_service_by_uuid_cb,
	.discover_service_all_cb = fibo_discover_service_all_cb,
	.char_des_data = fibo_char_des_data,
	.char_data = fibo_char_data,
	.read_cb = fibo_read_cb,
	.read_blob_cb = fibo_read_blob_cb,
	.read_multi_cb = fibo_read_multi_cb,
	.recv_notification_cb = fibo_recv_notification_cb,
	.recv_indication_cb = fibo_recv_indication_cb,
	.write_cb = fibo_write_cb,
	.write_rsp_cb = fibo_write_rsp_cb,
	.scan_cb = fibo_scan_cb,
	.smp_pair_success_cb = fibo_smp_pair_success_cb,
	.smp_pair_failed_cb = fibo_smp_pair_failed_cb,
	.att_error_cb = fibo_att_error_cb,
};

const btgatt_callback_t fibo_ble_btgatt_callback={
	.client = &fibo_ble_client_callback,
	.server = NULL,
};

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

	fibo_taskSleep(10000);

    fibo_bt_onoff(1);
	fibo_taskSleep(2000);
	fibo_ble_set_read_name(0,(uint8_t *)"8910_ble",0); // set ble name 
	//size = sizeof(config_wifi_service)/sizeof(gatt_element_t);
	//fibo_ble_add_service_and_characteristic(config_wifi_service,size); //create serive and characteristic
	fibo_taskSleep(2000);

	fibo_ble_client_int((void *)&fibo_ble_btgatt_callback);

	fibo_ble_scan_enable(1);

	fibo_taskSleep(20000);

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

