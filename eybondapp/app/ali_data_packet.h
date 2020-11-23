#ifndef __ALI_DATA_PACKET_H__
#define __ALI_DATA_PACKET_H__

//void collector_packet(GinlongMonitor__MCollector1 *ginlongmonitor__mcollector1);
//void collect_parse_pakcet(GinlongMonitor__MCollector1 * ginlongmonitor__mcollector1,uint32_t len,const char * payload);
//void printf_collector_packet(GinlongMonitor__MCollector1 *ginlongmonitor__mcollector1);
//
//void inverter_packet(void* aliyun_mqtt_thread_handlet);
//void inverter_parse_pakcet(GinlongMonitor__MInverter1 * ginlongmonitor__minverter1,uint32_t len ,const char * payload);
//void printf_inverter_packet(GinlongMonitor__MInverter1 *ginlongmonitor__minverter1);
//
void packet_init(void);
int send_monitor_packet(void *aliyun_mqtt_thread_handle);
#endif 