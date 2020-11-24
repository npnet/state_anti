#include "fibo_opencpu.h"
#include "L610_conn_ali_net.h"
#include "ginlong_monitor.pb-c.h"
#include "eybpub_Debug.h"

#include "eybpub_SysPara_File.h"

//char product_key[64] = "a1zFSNAQ8G0" ;
//char device_name[64] = "device1";
//char device_secret[64]  = "ff378340b0453de58c2f06cb5f52b4a8";
//char pub_topic[] = "/a1zFSNAQ8G0/device1/user/push";



char product_key[64] = "a1IkBbp6n23" ;
char device_name[64] = "6012000000000002";
char device_secret[64]  = "6310534bac5cdf4cfbb40a60a6452de6";
char pub_topic[] = "/a1IkBbp6n23/6012000000000002/user/update";

char sub_topic[] = "/a1zFSNAQ8G0/device1/user/get" ;

UINT32 g_lock = 0;

static void fibo_aliyunMQTT_connect_callback(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{
    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
    char* ptopic = NULL;

    switch (msg->event_type)
    {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:

            /* print topic name and topic message */

            ptopic = (char*)malloc(sizeof(char) * (topic_info->topic_len + 1));
            if (ptopic == NULL)
            {
                APP_PRINT("ptopic malloc null\r\n");
                break;
            }
            memset(ptopic, 0x0, sizeof(char) * (topic_info->topic_len + 1));
            memcpy(ptopic, topic_info->ptopic, topic_info->topic_len);
            APP_PRINT("ali to device connect_callback topic: topic_len:%d, topic:%s\r\n", topic_info->topic_len, ptopic);
            APP_PRINT("Payload: payload_len:%d, payload:%s\r\n", topic_info->payload_len, topic_info->payload);

            free(ptopic);
            ptopic = NULL;
            break;
        case IOTX_MQTT_EVENT_SUBCRIBE_SUCCESS:
        {
            APP_PRINT("aliyun mqtt subscribe success!\r\n");
            fibo_sem_signal(g_lock);
            break;
        }
        case IOTX_MQTT_EVENT_SUBCRIBE_TIMEOUT:
        case IOTX_MQTT_EVENT_SUBCRIBE_NACK:
        {
            APP_PRINT("aliyun mqtt subscribe fail!\r\n");
        }
        break;
        case IOTX_MQTT_EVENT_UNSUBCRIBE_SUCCESS:
        {
            APP_PRINT("aliyun mqtt unsubscribe success!\r\n");
            fibo_sem_signal(g_lock);
            break;
        }
        case IOTX_MQTT_EVENT_UNSUBCRIBE_TIMEOUT:
        case IOTX_MQTT_EVENT_UNSUBCRIBE_NACK:
        {
            APP_PRINT("aliyun mqtt unsubscribe fail!\r\n");
            break;
        }
        case IOTX_MQTT_EVENT_PUBLISH_SUCCESS:
        {
            APP_PRINT("aliyun mqtt publish success!\r\n");
            fibo_sem_signal(g_lock);
            break;
        }
        case IOTX_MQTT_EVENT_PUBLISH_TIMEOUT:
        case IOTX_MQTT_EVENT_PUBLISH_NACK:
        {
            APP_PRINT("aliyun mqtt publish fail!\r\n");
            break;
        }
        default:
            break;
    }
}


static void fibo_aliyunMQTT_sub_callback(void *pcontext, void *pclient, iotx_mqtt_event_msg_pt msg)
{

    iotx_mqtt_topic_info_t     *topic_info = (iotx_mqtt_topic_info_pt) msg->msg;
    char* ptopic = NULL;
    switch (msg->event_type)
    {
        case IOTX_MQTT_EVENT_PUBLISH_RECEIVED:

            /* print topic name and topic message */

            ptopic = (char*)malloc(sizeof(char) * (topic_info->topic_len + 1));
            if (ptopic == NULL)
            {
                APP_PRINT("ptopic malloc null\r\n");
                break;
            }
            memset(ptopic, 0x0, sizeof(char) * (topic_info->topic_len + 1));
            memcpy(ptopic, topic_info->ptopic, topic_info->topic_len);
            APP_PRINT("ali to device sub_callback topic: topic_len:%d, topic:%s\r\n", topic_info->topic_len, ptopic);
            //APP_PRINT("Payload: payload_len:%d, payload:%s", topic_info->payload_len, topic_info->payload);
            APP_PRINT("Payload: payload_len:%d\r\n", topic_info->payload_len);
            //output(topic_info->payload,topic_info->payload_len);
            //APP_PRINT("Payload: payload_len:%d", topic_info->payload_len);
            //这里protobuf 解包数据放入 rcveList
            //ESP_callback(buf);
            free(ptopic);
            ptopic = NULL;
            break;
        default:
            break;
    }
}


void mqtt_conn_ali_task(void *param)
{
    APP_PRINT("mqtt connect ali start ...\r\n");
    int ret = 0;
    void* aliyun_mqtt_thread_handle = NULL;

    APP_PRINT("aliyun mqttapi wait network\r\n");
    bool ret_lock = false;


    char host[64] = {0};
    UINT8 ip[50];
    UINT8 cid_status;
    INT8 cid = 1;
    CFW_SIM_ID sim_id = CFW_SIM_0;

//  memcpy(product_key, "a1zFSNAQ8G0", strlen("a1zFSNAQ8G0"));
//  memcpy(device_name, "device1", strlen("device1"));
//  memcpy(device_secret, "ff378340b0453de58c2f06cb5f52b4a8", strlen("ff378340b0453de58c2f06cb5f52b4a8"));
    memcpy(host, "iot-auth.aliyun.com", strlen("iot-auth.aliyun.com"));

    ret = fibo_aliyunMQTT_cloudauth(product_key, device_name, device_secret, host, NULL);
    if (ret == false)
    {
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
    aliyun_mqtt_thread_handle = fibo_aliyunMQTT_cloudConn(80,0,4,(iotx_mqtt_event_handle_func_fpt)fibo_aliyunMQTT_connect_callback);
    if (aliyun_mqtt_thread_handle == NULL)
    {
        APP_PRINT("aliyun mqtt connect failed\r\n");
        fibo_thread_delete();
        return;
    }

    APP_PRINT("aliyun mqttapi connect finish\r\n");
    packet_init();
//  ret = fibo_aliyunMQTT_cloudSub(aliyun_mqtt_thread_handle, sub_topic, 1, fibo_aliyunMQTT_sub_callback);
//  if (ret == false)
//  {
//      APP_PRINT("aliyun mqttapi sub failed\r\n");
//      fibo_thread_delete();
//      return;
//  }
//
//  ret_lock = fibo_sem_try_wait(g_lock, 60000);
//  if (ret_lock == false)
//  {
//      APP_PRINT("[%s]%d wait sub failed\r\n");
//      fibo_thread_delete();
//      return;
//  }
//
//  APP_PRINT("aliyun mqttapi sub success\r\n");




// ret = fibo_aliyunMQTT_cloudUnsub(aliyun_mqtt_thread_handle,"/a1klgAXldch/867567040119318/user/get");
// if (ret == false)
// {
// APP_PRINT("aliyun mqttapi unsub failed");
// }
// APP_PRINT("aliyun mqttapi unsub success");

// ret_lock = fibo_sem_try_wait(g_lock, 60000);
// if (ret_lock == false)
// {
// APP_PRINT("[%s]%d wait unsub failed", __FUNCTION__, __LINE__);
// fibo_thread_delete();
// return;
// }

// ret = fibo_aliyunMQTT_cloudDisconn(&aliyun_mqtt_thread_handle);
// if (ret == false)
// {
// APP_PRINT("aliyun mqttapi disconn failed");
// }
// APP_PRINT("aliyun mqttapi disconn success");


    while (1)
    {
//        fibo_queue_get(ALIYUN_TASK, (void *)&msg, 0);
//        switch (msg)
//        {
//			case MODBUS_DATA_GET:
//				
//				break;
//		
//            default:
//                break;
//        }

#if 0
        Buffer_t setbuf = {0};
     
        char str[] = "800" ;
		setbuf.lenght =strlen(str);
		setbuf.size   =sizeof(str);
		setbuf.payload = str;
		parametr_set(24, &setbuf) ;
//		parametr_default();

		Buffer_t read_buf = {0};
		parametr_get(24, &read_buf);
		APP_PRINT("read_buf = %s \r\n",read_buf.payload);
#endif		

        send_monitor_packet(aliyun_mqtt_thread_handle);
        fibo_taskSleep(5 * 1000);

    }




    fibo_thread_delete();
}
