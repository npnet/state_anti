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

static UINT32 g_lock = 0;
void* g_fibo_aliyun_MQTT_threadhandle = NULL;

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


void fibo_aliyunMQTT_connect_callback(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
	iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
	char* ptopic = NULL;
	OSI_PRINTFI("[%s] enter!!!",__func__);
    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* print topic name and topic message */
            OSI_PRINTFI("Message Arrived:");
            ptopic = (char*)malloc(sizeof(char) * (topic_info->topic_len + 1));
			if (ptopic == NULL)
			{
				OSI_PRINTFI("ptopic malloc null");
				break;
			}
			memset(ptopic, 0x0, sizeof(char) * (topic_info->topic_len + 1));
			memcpy(ptopic, topic_info->ptopic, topic_info->topic_len);
			OSI_PRINTFI("topic: topic_len:%d, topic:%s", topic_info->topic_len, ptopic);
            OSI_PRINTFI("Payload: payload_len:%d, payload:%s", topic_info->payload_len, topic_info->payload);
			for (uint8_t ucloop = 0; ucloop < topic_info->payload_len; ucloop++)
			{
				OSI_PRINTFI("%02x", topic_info->payload[ucloop]);
			}
			free(ptopic);
			ptopic = NULL;
            break;
		case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
			{
				OSI_PRINTFI("aliyun mqtt subscribe success!");
				fibo_sem_signal(g_lock);
				break;
			}
		case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
		case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
			{
				OSI_PRINTFI("aliyun mqtt subscribe fail!");
			}
			break;
		case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
			{
				OSI_PRINTFI("aliyun mqtt unsubscribe success!");	
				fibo_sem_signal(g_lock);
				break;
			}
		case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
		case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
			{
				OSI_PRINTFI("aliyun mqtt unsubscribe fail!");	
				break;
			}
		case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
			{
				OSI_PRINTFI("aliyun mqtt publish success!");		
				fibo_sem_signal(g_lock);
				break;
			}
		case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
		case IOTX_MQTT_EVENT_PUBLISH_NACK:
			{
				OSI_PRINTFI("aliyun mqtt publish fail!");						
				break;
			}
        default:
            break;
    }
}

void fibo_aliyunMQTT_sub_callback(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
	iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
	char* ptopic = NULL;
	OSI_PRINTFI("[%s] enter!!!",__func__);
    switch (msg->event_type) {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:
            /* print topic name and topic message */
            OSI_PRINTFI("Message Arrived:");
            ptopic = (char*)malloc(sizeof(char) * (topic_info->topic_len + 1));
			if (ptopic == NULL)
			{
				OSI_PRINTFI("ptopic malloc null");
				break;
			}
			memset(ptopic, 0x0, sizeof(char) * (topic_info->topic_len + 1));
			memcpy(ptopic, topic_info->ptopic, topic_info->topic_len);
			OSI_PRINTFI("topic: topic_len:%d, topic:%s", topic_info->topic_len, ptopic);
            OSI_PRINTFI("Payload: payload_len:%d, payload:%s", topic_info->payload_len, topic_info->payload);
			for (uint8_t ucloop = 0; ucloop < topic_info->payload_len; ucloop++)
			{
				OSI_PRINTFI("%02x", topic_info->payload[ucloop]);
			}
			free(ptopic);
			ptopic = NULL;
            break;
        default:
            break;
    }
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    int ret = 0;
	void* aliyun_mqtt_thread_handle = NULL;

    OSI_PRINTFI("aliyun mqttapi wait network");
	bool ret_lock = false;

	char product_key[64] = {0};
	char device_name[64] = {0};
	char device_secret[64] = {0};
	char host[64] = {0};
	UINT8 ip[50];
	UINT8 cid_status;
	INT8 cid = 1;
	CFW_SIM_ID sim_id = CFW_SIM_0;
	
	memcpy(product_key, "a1klgAXldch", strlen("a1klgAXldch"));
	memcpy(device_name, "867567040119318", strlen("867567040119318"));
	memcpy(device_secret, "qkXG2vvq4Minw7y3eMdrE2F3UoATqmzA", strlen("qkXG2vvq4Minw7y3eMdrE2F3UoATqmzA"));
	memcpy(host, "iot-auth.aliyun.com", strlen("iot-auth.aliyun.com"));
	
	ret = fibo_aliyunMQTT_cloudauth(product_key, device_name, device_secret, host, NULL);
	if (ret == false)
	{
		OSI_PRINTFI("aliyunmqtt [%s-%d], auth failed!", __FUNCTION__, __LINE__);	
		fibo_thread_delete();
		return;
	}

	/* wait PDP active */
	while (1)
	{
		fibo_PDPStatus(cid,ip,&cid_status,sim_id);
		if (cid_status == 1)
		{
			break;
		}		
		fibo_taskSleep(2000);
	}
	g_lock = fibo_sem_new(1);
    OSI_PRINTFI("aliyunmqtt [%s-%d]", __FUNCTION__, __LINE__);	
    aliyun_mqtt_thread_handle = fibo_aliyunMQTT_cloudConn(80,0,4,(iotx_mqtt_event_handle_func_fpt)fibo_aliyunMQTT_connect_callback);
	if (aliyun_mqtt_thread_handle == NULL)
	{
		OSI_PRINTFI("[%s]%d aliyun mqtt connect failed", __FUNCTION__, __LINE__);	
		fibo_thread_delete();
		return;
	}
	g_fibo_aliyun_MQTT_threadhandle = aliyun_mqtt_thread_handle;
	OSI_PRINTFI("aliyun mqttapi connect finish");

	ret = fibo_aliyunMQTT_cloudSub(aliyun_mqtt_thread_handle, "/a1klgAXldch/867567040119318/user/get", 1, fibo_aliyunMQTT_sub_callback);
    if (ret == false)
    {
        OSI_PRINTFI("aliyun mqttapi sub failed");
		fibo_thread_delete();
		return;
    }

	ret_lock = fibo_sem_try_wait(g_lock, 60000);
	if (ret_lock == false)
	{
		OSI_PRINTFI("[%s]%d wait sub failed", __FUNCTION__, __LINE__);	
		fibo_thread_delete();
		return;
	}
	OSI_PRINTFI("aliyun mqttapi sub success");

	
	ret = fibo_aliyunMQTT_cloudPub(aliyun_mqtt_thread_handle,"/a1klgAXldch/867567040119318/user/get",1,"pubtest 20200721 1");
	if (ret == false)
	{
		OSI_PRINTFI("aliyun mqttapi pub failed");
	}
	OSI_PRINTFI("aliyun mqttapi pub success");


	ret_lock = fibo_sem_try_wait(g_lock, 60000);
	if (ret_lock == false)
	{
		OSI_PRINTFI("[%s]%d wait pub failed", __FUNCTION__, __LINE__);	
		fibo_thread_delete();
		return;
	}
	OSI_PRINTFI("aliyun mqttapi pub success");

	uint8_t pub_payload[22] = {0x01,0x01,0x00,0x64,0x14,0x33,0x03,0xe8,0x00,0x0f,0x42,0x40,0x00,0x73,0x43,0x78,0x85,0xc4,0x0f,0xf1,0x30,0xbb};
	ret = fibo_aliyunMQTT_cloudPub_FixedLen(aliyun_mqtt_thread_handle,"/a1klgAXldch/867567040119318/user/get",1,pub_payload, 22);
	if (ret == false)
	{
		OSI_PRINTFI("aliyun mqttapi pub fixedlen failed");
	}
	OSI_PRINTFI("aliyun mqttapi pub fixed len success");


	ret_lock = fibo_sem_try_wait(g_lock, 60000);
	if (ret_lock == false)
	{
		OSI_PRINTFI("[%s]%d wait pub failed", __FUNCTION__, __LINE__);	
		fibo_thread_delete();
		return;
	}
	OSI_PRINTFI("aliyun mqttapi pub success");

	ret = fibo_aliyunMQTT_cloudUnsub(aliyun_mqtt_thread_handle,"/a1klgAXldch/867567040119318/user/get");
	if (ret == false)
	{
		OSI_PRINTFI("aliyun mqttapi unsub failed");
	}
	OSI_PRINTFI("aliyun mqttapi unsub success");

	ret_lock = fibo_sem_try_wait(g_lock, 60000);
	if (ret_lock == false)
	{
		OSI_PRINTFI("[%s]%d wait unsub failed", __FUNCTION__, __LINE__);	
		fibo_thread_delete();
		return;
	}

	ret = fibo_aliyunMQTT_cloudDisconn(&aliyun_mqtt_thread_handle);
	if (ret == false)
	{
		OSI_PRINTFI("aliyun mqttapi disconn failed");
	}
	OSI_PRINTFI("aliyun mqttapi disconn success");
	
    while (1)
    {
        fibo_taskSleep(1 * 1000);
    }

    fibo_thread_delete();
}
void *appimg_enter(void *param)
{   OSI_LOGI(0, "application image enter, param 0x%x", param);    
	prvInvokeGlobalCtors();
	fibo_thread_create(prvThreadEntry, "mqtt-thread", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);    
	return NULL;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
