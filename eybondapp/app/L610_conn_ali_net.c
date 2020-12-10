#include "fibo_opencpu.h"
#include "L610_conn_ali_net.h"
#include "ginlong_monitor.pb-c.h"
#include "eybpub_Debug.h"

#include "eybpub_SysPara_File.h"
#include "ali_data_packet.h"
#include "eybpub_utility.h"
#include "eybapp_appTask.h"
#include "Device.h"
#include "L610Net_TCP_EYB.h"
// #include "restart_net.h"

UINT32 g_lock = 0;
static u32_t send_timer = 0;
static u32_t work_tick_timer = 0;
bool is_ali_conn_success = false;
static bool is_rev_data_complete = false;

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
            APP_PRINT("Payload: payload_len:%ld, payload:%s\r\n", topic_info->payload_len, topic_info->payload);

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
            APP_PRINT("Payload: payload_len:%ld\r\n", topic_info->payload_len);
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
static void send_data_callback(void *arg)
{
    if(is_rev_data_complete&&is_ali_conn_success)
    {
        send_monitor_packet(arg);
    }
}

static void tick1s_callback(void *arg)
{
    uint64_t tick = (*(uint64_t*)arg)++;
    if(0 == (tick+1) % 3600)  //每隔一小时保存总共运行时间
    {
        Buffer_t data;
        memset(&data,0,sizeof(data));
        uint64_t total_time = *(get_total_tick());
		total_time = total_time + *(get_current_working_tick());
        char temp_total_time[21] = {0};
        snprintf(temp_total_time, sizeof(temp_total_time)-1, "%llu", total_time);
        data.payload = (u8_t *)temp_total_time;
        data.lenght = strlen(temp_total_time);
        data.size   = strlen(temp_total_time);
        parametr_set(97, &data);
    }

}

void mqtt_conn_ali_task(void *param) {
  APP_PRINT("aliyun task run...\r\n");  
  int ret = 0;
  void* aliyun_mqtt_thread_handle = NULL;  
  bool ret_lock = false;

  char host[64] = {0};
  UINT8 ip[50];
  UINT8 cid_status;
  INT8 cid = 1;
  CFW_SIM_ID sim_id = CFW_SIM_0;		
  g_lock = fibo_sem_new(1);
   
  while (1) {
    ST_MSG msg;
    static Device_t *currentDevice = NULL;
    static DeviceCmd_t *currentCmd = NULL;
    r_memset(&msg, 0, sizeof(ST_MSG));

    fibo_queue_get(ALIYUN_TASK, (void *)&msg, 0);
    switch (msg.message) {
      case APP_MSG_UART_READY:
        load_config_para();
        memcpy(host, "iot-auth.aliyun.com", strlen("iot-auth.aliyun.com"));
        ret = fibo_aliyunMQTT_cloudauth(para.product_key, para.device_name, para.device_secret, host, NULL);
        if (ret == false) {
          APP_DEBUG("mqtt cloudauth failed\r\n");
        }
        break;
      case NET_MSG_DNS_READY: // get PDP active message
        APP_DEBUG("aliyun mqttapi get network\r\n");
        APP_DEBUG("mqtt connect ali start ...\r\n");
        aliyun_mqtt_thread_handle = fibo_aliyunMQTT_cloudConn(80,0,4,(iotx_mqtt_event_handle_func_fpt)fibo_aliyunMQTT_connect_callback);
        if (aliyun_mqtt_thread_handle == NULL) {
          APP_DEBUG("aliyun mqtt connect failed\r\n");
        } else {
          APP_DEBUG("aliyun mqtt connect finish\r\n");
          is_ali_conn_success = true;
        }
        send_timer = fibo_timer_period_new(get_data_upload_cycle()*1000*60, send_data_callback, aliyun_mqtt_thread_handle);//数据上传周期 单位分钟
        if (send_timer == 0) {
          APP_DEBUG("Register send timer(%ld) fail", send_timer);
        }
        work_tick_timer = fibo_timer_period_new(1*1000, tick1s_callback, get_current_working_tick());//本次上电工作时间 1S加一次
        if (work_tick_timer == 0) {
          APP_DEBUG("Register work tick timer(%ld) fail", work_tick_timer);
        }
        break;
      case NET_MSG_DNS_FAIL:
        APP_DEBUG("aliyun mqttapi get network fail\r\n");
        break;
      case MODBUS_DATA_GET: {
        currentDevice = list_nextData(&DeviceList, currentDevice);    // 定时获取列表中需要执行指令的设备节点
        if(NULL != currentDevice) {
          currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);  // 找到当前执行设备需要执行的指令
          if(NULL != currentCmd) {
            APP_DEBUG("data cmd success\r\n");
            out_put_buffer((char *)currentCmd->cmd.payload,currentCmd->cmd.lenght);
            if(currentCmd->cmd.payload[2] == 0x0B) {
              if(currentCmd->cmd.payload[3] == 0xB7) { // address 2999 28个字节 
                inverter_data1 = (Inverter_Packet1 *)((u8_t *)currentCmd->ack.payload+3);
              } else if(currentCmd->cmd.payload[3] == 0xC5) {
                inverter_data2 = (Inverter_Packet2 *)((u8_t *)currentCmd->ack.payload+3);
              } else if(currentCmd->cmd.payload[3] == 0xD2) {
                inverter_data3= (Inverter_Packet3 *)((u8_t *)currentCmd->ack.payload+3);
              } else if(currentCmd->cmd.payload[3] == 0xDF) {
                inverter_data4= (Inverter_Packet4 *)((u8_t *)currentCmd->ack.payload+3);
              } else if(currentCmd->cmd.payload[3] == 0xEC) {
                inverter_data5= (Inverter_Packet5 *)((u8_t *)currentCmd->ack.payload+3);
              } else if(currentCmd->cmd.payload[3] == 0xF9) {
                inverter_data6= (Inverter_Packet6 *)((u8_t *)currentCmd->ack.payload+3);                                
                is_rev_data_complete = true;
              }
            }
            APP_DEBUG("data ack success\r\n");
            out_put_buffer((char *)currentCmd->ack.payload,currentCmd->ack.lenght);
            APP_DEBUG("\r\n");
          } else {
            APP_DEBUG("can not find command\r\n");
          }
        } else {
          APP_DEBUG("can not find device\r\n");
        } 
        APP_DEBUG("rev moudbus data\r\n");
        break;
      }
      default:
        break;
    }
  }
  fibo_thread_delete();
} 

/* void mqtt_conn_ali_task(void *param)
{
    APP_PRINT("aliyun task run...\r\n");
    fibo_sem_wait(g_SemFlag);
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
		
    load_config_para();
    memcpy(host, "iot-auth.aliyun.com", strlen("iot-auth.aliyun.com"));
  	
    ret = fibo_aliyunMQTT_cloudauth(para.product_key, para.device_name, para.device_secret, host, NULL);
    if (ret == false)
    {
        APP_PRINT("mqtt cloudauth failed\r\n");
    }

    // wait PDP active
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
    }else
	{
		APP_PRINT("aliyun mqtt connect finish\r\n");
		is_ali_conn_success = true;
   	}    

    send_timer = fibo_timer_period_new(get_data_upload_cycle()*1000*60, send_data_callback, aliyun_mqtt_thread_handle);//数据上传周期 单位分钟
    if (send_timer == 0)
    {
        APP_PRINT("Register send timer(%ld) fail", send_timer);
    }

    work_tick_timer = fibo_timer_period_new(1*1000, tick1s_callback, get_current_working_tick());//本次上电工作时间 1S加一次
    if (work_tick_timer == 0)
    {
        APP_PRINT("Register work tick timer(%ld) fail", work_tick_timer);
    }


    while (1)
    {
        ST_MSG msg;
        static Device_t *currentDevice = NULL;
        static DeviceCmd_t *currentCmd = NULL;
        r_memset(&msg, 0, sizeof(ST_MSG));

        fibo_queue_get(ALIYUN_TASK, (void *)&msg, 0);
        switch (msg.message)
        {
            case MODBUS_DATA_GET:

                currentDevice = list_nextData(&DeviceList, currentDevice);    // 定时获取列表中需要执行指令的设备节点
                if(NULL != currentDevice)
                {
                    currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);  // 找到当前执行设备需要执行的指令
                    if(NULL != currentCmd)
                    {
                        APP_PRINT("data cmd success\r\n");
                        out_put_buffer((char *)currentCmd->cmd.payload,currentCmd->cmd.lenght);

                        if(currentCmd->cmd.payload[2] == 0x0B)
                        {

                            if(currentCmd->cmd.payload[3] == 0xB7)//address 2999 28个字节
                            {
                                inverter_data1 = (Inverter_Packet1 *)((u8_t *)currentCmd->ack.payload+3);
                            }
                            else if(currentCmd->cmd.payload[3] == 0xC5)
                            {
                                inverter_data2 = (Inverter_Packet2 *)((u8_t *)currentCmd->ack.payload+3);
                            }
                            else if(currentCmd->cmd.payload[3] == 0xD2)
                            {
                                inverter_data3= (Inverter_Packet3 *)((u8_t *)currentCmd->ack.payload+3);
                            }
                            else if(currentCmd->cmd.payload[3] == 0xDF)
                            {
                                inverter_data4= (Inverter_Packet4 *)((u8_t *)currentCmd->ack.payload+3);
                            }
                            else if(currentCmd->cmd.payload[3] == 0xEC)
                            {
                                inverter_data5= (Inverter_Packet5 *)((u8_t *)currentCmd->ack.payload+3);
                            }
                            else if(currentCmd->cmd.payload[3] == 0xF9)
                            {
                                inverter_data6= (Inverter_Packet6 *)((u8_t *)currentCmd->ack.payload+3);                                
                                is_rev_data_complete = true;
                            }
                        }


                        APP_PRINT("data ack success\r\n");
                        out_put_buffer((char *)currentCmd->ack.payload,currentCmd->ack.lenght);
                        APP_PRINT("\r\n");



                    }
                    else
                    {
                        APP_PRINT("can not find command\r\n");
                    }
                }
                else
                {
                    APP_PRINT("can not find device\r\n");
                }

                APP_PRINT("rev moudbus data\r\n");
                break;

            default:
                break;
        }

    }


    fibo_thread_delete();
} */
