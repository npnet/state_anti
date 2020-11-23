#include "fibo_opencpu.h"
#include "ginlong_monitor.pb-c.h"
#include "eybpub_Debug.h"
#include "L610_conn_ali_net.h"

static GinlongMonitor__MNotification notification = {0} ;
static GinlongMonitor__MCollector1 collector1 	=  {0};
static GinlongMonitor__Packet	packet = {0};
static GinlongMonitor__Packet	*unpacket = NULL;
static GinlongMonitor__MInverter1 ** inverter = NULL;



uint64_t str_to_hex(char *str)
{
	uint64_t get_int=0 ;
	if(str==NULL)
		return 1 ;
	while(*str !='\0')
	{
		get_int = get_int*16;
		if( (*str>=0x30) && (*str<=0x39)) //0-9
		{
			get_int += *str-0x30;
		}
		else if( (*str>=0x41) && (*str<=0x46))//A-F
				get_int += (*str-0x41+10);
			else if((*str>=0x61) && (*str<=0x66))//a-f
				get_int += (*str-0x61+10);
			else
				return 0;
			str++;
			}
		return get_int ;
}
#if 0
void set_collector_packet_data(GinlongMonitor__MCollector1 *ginlongmonitor__mcollector1)
{
	int ret = 0;
	int buf_len  = 0;
	void *buf = NULL;
	uint8_t data[] = {0x50,0x9A,0x4C,0x24,0x65,0xD0};	
	uint32_t inverter_address[1] = {88};
	uint32_t weather_station_address[1] = {99};
	uint32_t confluence_box_address[1] = {66};

	//*ginlongmonitor__mcollector1 = GINLONG_MONITOR__M_COLLECTOR1__INIT;
	ginlongmonitor__mcollector1->is_realtime                   = 0;
	ginlongmonitor__mcollector1->collector_sn                  = str_to_hex("6012000000000002");
	ginlongmonitor__mcollector1->total_working_time            = 7200; //2h
	ginlongmonitor__mcollector1->current_working_time          = 3720; //1h2m
	ginlongmonitor__mcollector1->data_upload_cycle             = 300;  //5m10s
	ginlongmonitor__mcollector1->heartbeat_interval            = 5;
	ginlongmonitor__mcollector1->maximum_number                = 10;
	ginlongmonitor__mcollector1->actual_number                 = 1;
	ginlongmonitor__mcollector1->collector_version             = 0x28693657;
	ginlongmonitor__mcollector1->factory_time                  = 1605687903;//2020/11/18 16:25:3
	ginlongmonitor__mcollector1->rssi_level                    = 4;
	ginlongmonitor__mcollector1->rssi                          = 10;
	ginlongmonitor__mcollector1->connection_operator           = "wifi_test";
	ginlongmonitor__mcollector1->iccid                         ="89757";
	ginlongmonitor__mcollector1->lac                           = 30;
	ginlongmonitor__mcollector1->ci                            = 40;
	ginlongmonitor__mcollector1->position_latitude             = 100;
	ginlongmonitor__mcollector1->position_longitude            = 200;
	ginlongmonitor__mcollector1->connected_ssid                ="wifi_test";
	ginlongmonitor__mcollector1->mac.len                       = sizeof(data)/sizeof(data[0]);     //mac;//50-9A-4C-24-65-D0
	ginlongmonitor__mcollector1->mac.data                      = data;
	ginlongmonitor__mcollector1->lan_ip                        = 3232235786;//192.168.1.10
	ginlongmonitor__mcollector1->n_inverter_address            = 1;
	ginlongmonitor__mcollector1->inverter_address              = inverter_address;
	ginlongmonitor__mcollector1->n_weather_station_address     = 1;
	ginlongmonitor__mcollector1->weather_station_address       = weather_station_address;
	ginlongmonitor__mcollector1->n_confluence_box_address      = 1;
	ginlongmonitor__mcollector1->confluence_box_address        = confluence_box_address;
	ginlongmonitor__mcollector1->datalogger_update             = 0;
	ginlongmonitor__mcollector1->inverter_update               = 0;
	ginlongmonitor__mcollector1->inverter_update_addr          =44;

}

#endif
void set_collector_packet_data(GinlongMonitor__MCollector1 *ginlongmonitor__mcollector1)
{
	int ret = 0;
	int buf_len  = 0;
	void *buf = NULL;
	uint8_t data[] = {0x50,0x9A,0x4C,0x24,0x65,0xD0};	
	uint32_t inverter_address[1] = {257};
	uint32_t weather_station_address[1] = {99};
	uint32_t confluence_box_address[1] = {66};

	//ginlongmonitor__mcollector1->is_realtime                   = 0;
	ginlongmonitor__mcollector1->collector_sn                  = str_to_hex("6012000000000002");
	ginlongmonitor__mcollector1->total_working_time            = 1986960; //2h
	ginlongmonitor__mcollector1->current_working_time          = 3777; //1h2m
	ginlongmonitor__mcollector1->data_upload_cycle             = 60;  //5m10s
	ginlongmonitor__mcollector1->heartbeat_interval            = 120;
	ginlongmonitor__mcollector1->maximum_number                = 1;
	ginlongmonitor__mcollector1->actual_number                 = 1;
	ginlongmonitor__mcollector1->collector_version             = 554107154;
	ginlongmonitor__mcollector1->factory_time                  = 1557976525;//2020/11/18 16:25:3
	ginlongmonitor__mcollector1->rssi_level                    = 5;
	ginlongmonitor__mcollector1->rssi                          = 31;
	ginlongmonitor__mcollector1->connection_operator           = "China Mobile";
	ginlongmonitor__mcollector1->iccid                         ="898604071120C0015291";
	ginlongmonitor__mcollector1->lac                           = 22548;
	ginlongmonitor__mcollector1->ci                            = 29408;
	//ginlongmonitor__mcollector1->position_latitude             = 100;
	//ginlongmonitor__mcollector1->position_longitude            = 200;
	//ginlongmonitor__mcollector1->connected_ssid                ="wifi_test";
	//ginlongmonitor__mcollector1->mac.len                       = sizeof(data)/sizeof(data[0]);     //mac;//50-9A-4C-24-65-D0
	//ginlongmonitor__mcollector1->mac.data                      = data;
	//ginlongmonitor__mcollector1->lan_ip                        = 3232235786;//192.168.1.10
	ginlongmonitor__mcollector1->n_inverter_address            = 1;
	ginlongmonitor__mcollector1->inverter_address              = inverter_address;
	//ginlongmonitor__mcollector1->n_weather_station_address     = 1;
	//ginlongmonitor__mcollector1->weather_station_address       = weather_station_address;
	//ginlongmonitor__mcollector1->n_confluence_box_address      = 1;
	//ginlongmonitor__mcollector1->confluence_box_address        = confluence_box_address;
	//ginlongmonitor__mcollector1->datalogger_update             = 0;
	//ginlongmonitor__mcollector1->inverter_update               = 0;
	//ginlongmonitor__mcollector1->inverter_update_addr          =44;
    

}



#pragma pack(1)
typedef struct
{
	uint16_t product_model;
    uint16_t dsp_software_version;
	uint16_t tft_software_version;
	uint16_t ac_output_type;
	uint16_t dc_input_type;
	uint32_t pac;
	uint32_t total_dc_output_power;
	uint32_t e_total;//总发电量
	uint32_t e_month;
	uint32_t e_last_month;
	uint16_t e_today;
	uint16_t e_yesterday;
	uint16_t e_year_h;
	uint16_t e_year_l;
	uint16_t e_last_year_h;
	uint16_t e_last_year_l;
	uint16_t reserve;
	uint16_t u_pv1;
	uint16_t i_pv1;
	uint16_t u_pv2;
	uint16_t i_pv2;
	uint16_t u_pv3;
	uint16_t i_pv3;
	uint16_t u_pv4;
	uint16_t i_pv4;
	uint16_t reserve1;
	uint16_t reserve2;
	uint16_t dc_bus ;//直流母线电压
	uint16_t dc_bus_half;
	uint16_t u_ac1; //A相电压
	uint16_t u_ac2;
	uint16_t u_ac3;
	uint16_t i_ac1;//A相电流
	uint16_t i_ac2;
	uint16_t i_ac3;
	uint16_t reserve3;
	uint16_t reserve4;
	int16_t  inverter_temperature;
	uint16_t fac; //电网频率
	uint16_t current_state;
	uint16_t reserve5;
	uint16_t reserve6;
	uint16_t reserve7;
	uint16_t reserve8;
	uint16_t reserve9;
	uint16_t p_limit_set;//限功率实际值
	int16_t  p_factor_limit_set;//功率因数实际调节值
	uint16_t reserve10;
	uint16_t reserve11;
	uint16_t national_standards;
	uint16_t power_curve_version;//功率曲线号
	uint32_t reactive_power;//无功功率
	uint32_t apparent_power; 
	uint16_t real_time_power_factor;
	uint16_t inverter_sn_h;//逆变器序列高 4 位
	uint16_t inverter_sn_m1;//逆变器序列中 4 位
	uint16_t inverter_sn_m2;//逆变器序列中 4 位
	uint16_t inverter_sn_l;//逆变器序列低 4 位
	uint16_t reserve12;
	uint16_t reserve13;
	uint16_t sys_year;
	uint16_t sys_month;
	uint16_t sys_day;
	uint16_t sys_hour;
	uint16_t sys_minute;
	uint16_t sys_second;
}Inverter_Packet; 
#pragma pack()

Inverter_Packet inverter_data = {0};


void set_inverter_packet_data(GinlongMonitor__MInverter1  *ginlongmonitor__minverter1)
{
#if 1
	ginlongmonitor__minverter1->e_total 							=3000 ;//累计发电 :3MWh
	ginlongmonitor__minverter1->national_standards                  =1 ;
	ginlongmonitor__minverter1->ac_output_type                      =3 ;
	ginlongmonitor__minverter1->dc_input_type                       =19 ;
	ginlongmonitor__minverter1->current_state                       =4117 ;
	ginlongmonitor__minverter1->inverter_sn.len                     =0;
	ginlongmonitor__minverter1->inverter_sn.data                    =NULL;
	ginlongmonitor__minverter1->inverter_data_time                  =1605763149;
	ginlongmonitor__minverter1->inverter_temperature                =238 ;//IGBT内芯温度
	ginlongmonitor__minverter1->rs485_com_addr                      = 257;
	ginlongmonitor__minverter1->e_last_month                        =1000 ;
	ginlongmonitor__minverter1->u_pv4                               =9 ;
	ginlongmonitor__minverter1->inverter_sn2                        ="123456" ;
	
	ginlongmonitor__minverter1->inverter_software_version           =592669;
	ginlongmonitor__minverter1->u_pv1                               =2692;
	ginlongmonitor__minverter1->u_pv3                               =10 ;
	ginlongmonitor__minverter1->product_model                       =15;//型号
	ginlongmonitor__minverter1->u_pv2                               =7;

	ginlongmonitor__minverter1->pac 								=9 ;

	ginlongmonitor__minverter1->e_year                              =18000;
	ginlongmonitor__minverter1->e_today 							=50 ;
	ginlongmonitor__minverter1->e_month                             =150 ;
	ginlongmonitor__minverter1->e_total                             =18000 ; 

#endif

#if 0	
	ginlongmonitor__minverter1->e_total 							=(uint64_t)inverter_data.e_total ;//累计发电 :3MWh
	ginlongmonitor__minverter1->national_standards                  =(uint32_t)inverter_data.national_standards ;
	ginlongmonitor__minverter1->ac_output_type                      =(uint32_t)inverter_data.ac_output_type ;
	ginlongmonitor__minverter1->dc_input_type                       =(uint32_t)inverter_data.dc_input_type ;
	ginlongmonitor__minverter1->inverter_sn.len                     =0;
	ginlongmonitor__minverter1->inverter_sn.data                    =NULL;
	ginlongmonitor__minverter1->inverter_temperature                =(int32_t)inverter_data.inverter_temperature ;//IGBT内芯温度
	
	
	ginlongmonitor__minverter1->inverter_sn2                        ="123456" ;
	
	ginlongmonitor__minverter1->inverter_software_version           =(((uint32_t)inverter_data.dsp_software_version)<<16)+((uint32_t)inverter_data.tft_software_version);

	ginlongmonitor__minverter1->product_model                       =inverter_data.product_model;//型号
	ginlongmonitor__minverter1->pac 								=(int32_t)inverter_data.pac ;

	ginlongmonitor__minverter1->u_pv1                               =inverter_data.u_pv1;
	ginlongmonitor__minverter1->u_pv2                               =inverter_data.u_pv2;
	ginlongmonitor__minverter1->u_pv3                               =inverter_data.u_pv3 ;
	ginlongmonitor__minverter1->u_pv4								=inverter_data.u_pv4;
	ginlongmonitor__minverter1->i_pv1                               =inverter_data.i_pv1 ;
	ginlongmonitor__minverter1->i_pv2                               =inverter_data.i_pv2;
	ginlongmonitor__minverter1->i_pv3                               =inverter_data.i_pv3;
	ginlongmonitor__minverter1->i_pv4                               =inverter_data.i_pv4;
	

	ginlongmonitor__minverter1->e_year                              =(((uint64_t)inverter_data.e_year_h)<<16)+((uint64_t)inverter_data.e_year_l);
	ginlongmonitor__minverter1->e_today 							=(uint32_t)inverter_data.e_today ;
	ginlongmonitor__minverter1->e_month                             =(uint64_t)inverter_data.e_month;
	ginlongmonitor__minverter1->e_total                             =(uint64_t)inverter_data.e_total ; 


	
	ginlongmonitor__minverter1->u_ac1								=inverter_data.u_ac1 ;
	ginlongmonitor__minverter1->u_ac2                               =inverter_data.u_ac2 ;
	ginlongmonitor__minverter1->u_ac3                               =inverter_data.u_ac3;
	ginlongmonitor__minverter1->i_ac1								=inverter_data.i_ac1 ;	
	ginlongmonitor__minverter1->i_ac2                               =inverter_data.i_ac2 ;	
	ginlongmonitor__minverter1->i_ac3                               =inverter_data.i_ac3 ;
	ginlongmonitor__minverter1->fac                                 =inverter_data.fac;//频率
#endif
}



#if 0
void set_inverter_packet_data(GinlongMonitor__MInverter1  *ginlongmonitor__minverter1)
{
	int buf_len  = 0;
	int ret = 0;
	void *buf = NULL;

	uint8_t inverter_sn[] = {0xAA};
	uint8_t pv5_n_value[] = {0xAA};
	uint8_t iv_array_value[] = {0xAA};

	ginlongmonitor__minverter1->is_realtime                         = 0;
	ginlongmonitor__minverter1->inverter_data_time                  = 33027;
	ginlongmonitor__minverter1->rs485_com_addr                      = 257;
	ginlongmonitor__minverter1->product_model                       =15;
	
//	ginlongmonitor__minverter1->inverter_sn.len                     =sizeof(inverter_sn)/sizeof(inverter_sn[0]);
//	ginlongmonitor__minverter1->inverter_sn.data                    =inverter_sn;

	ginlongmonitor__minverter1->inverter_sn.len                     =0;
	ginlongmonitor__minverter1->inverter_sn.data                    =NULL;

	ginlongmonitor__minverter1->inverter_software_version           =0x78571834;
	ginlongmonitor__minverter1->ac_output_type                      =3 ;
	ginlongmonitor__minverter1->dc_input_type                       =19 ;
	ginlongmonitor__minverter1->inverter_meter_model                =8 ;
	ginlongmonitor__minverter1->pac                                 =8 ;
	ginlongmonitor__minverter1->e_today                             =50 ;
	ginlongmonitor__minverter1->e_yesterday                         =720;
	ginlongmonitor__minverter1->e_month                             =150 ;
	ginlongmonitor__minverter1->e_last_month                        =1000 ;
	ginlongmonitor__minverter1->e_year                              =2000;
	ginlongmonitor__minverter1->e_last_year                         =8 ;
	ginlongmonitor__minverter1->e_total                             =3000 ;//累计发电 :3MWh
	ginlongmonitor__minverter1->u_pv1                               =2692;
	ginlongmonitor__minverter1->i_pv1                               =10 ;
	ginlongmonitor__minverter1->u_pv2                               =7;
	ginlongmonitor__minverter1->i_pv2                               =5 ;
	ginlongmonitor__minverter1->u_pv3                               =10 ;
	ginlongmonitor__minverter1->i_pv3                               =8 ;
	ginlongmonitor__minverter1->u_pv4                               =9 ;
	ginlongmonitor__minverter1->i_pv4                               =8 ;
	ginlongmonitor__minverter1->u_ac1                               =8 ;
	ginlongmonitor__minverter1->i_ac1                               =8 ;

	ginlongmonitor__minverter1->inverter_temperature                =238 ;//IGBT内芯温度
	ginlongmonitor__minverter1->current_state                       =4117 ;
	ginlongmonitor__minverter1->fault1                              =8 ;
	ginlongmonitor__minverter1->fault2                              =8 ;
	ginlongmonitor__minverter1->fault3                              =8 ;
	ginlongmonitor__minverter1->fault4                              =8 ;
	ginlongmonitor__minverter1->fault5                              =8 ;
	ginlongmonitor__minverter1->fac                                 =8 ;
	ginlongmonitor__minverter1->national_standards                  =1 ;

	ginlongmonitor__minverter1->power_curve_version                 =8 ;
	ginlongmonitor__minverter1->reactive_power                      =8 ;
	ginlongmonitor__minverter1->apparent_power                      =8 ;
	ginlongmonitor__minverter1->power_factor                        =8 ;
	ginlongmonitor__minverter1->u_ac2                               =8 ;
	ginlongmonitor__minverter1->i_ac2                               =8 ;
	ginlongmonitor__minverter1->u_ac3                               =8 ;
	ginlongmonitor__minverter1->i_ac3                               =8 ;
	ginlongmonitor__minverter1->energy_storage_control              =8 ;
	ginlongmonitor__minverter1->storage_battery_voltage             =8 ;
	ginlongmonitor__minverter1->storage_battery_current             =8 ;
	ginlongmonitor__minverter1->llc_bus_voltage                     =8 ;
	ginlongmonitor__minverter1->bypass_ac_voltage                   =8 ;
	ginlongmonitor__minverter1->bypass_ac_current                   =8 ;
	ginlongmonitor__minverter1->battery_capacity_soc                =8 ;
	ginlongmonitor__minverter1->battery_health_soh                  =8 ;
	ginlongmonitor__minverter1->battery_voltage                     =8 ;
	ginlongmonitor__minverter1->bsttery_current                     =8 ;
	ginlongmonitor__minverter1->battery_charging_current            =8 ;
	ginlongmonitor__minverter1->battery_discharge_limiting          =8 ;
	ginlongmonitor__minverter1->battery_failure_information_01      =8 ;
	ginlongmonitor__minverter1->battery_failure_information_02      =8 ;
	ginlongmonitor__minverter1->family_load_power                   =8 ;
	ginlongmonitor__minverter1->bypass_load_power                   =8 ;
	ginlongmonitor__minverter1->battery_power                       =8 ;
	ginlongmonitor__minverter1->battery_total_charge_energy         =8 ;
	ginlongmonitor__minverter1->battery_today_charge_energy         =8 ;
	ginlongmonitor__minverter1->battery_yesterday_charge_energy     =8 ;
	ginlongmonitor__minverter1->battery_total_discharge_energy      =8 ;
	ginlongmonitor__minverter1->battery_today_discharge_energy      =8 ;
	ginlongmonitor__minverter1->battery_yesterday_discharge_energy  =8 ;
	ginlongmonitor__minverter1->grid_purchased_total_energy         =8 ;
	ginlongmonitor__minverter1->grid_purchased_today_energy         =8 ;
	ginlongmonitor__minverter1->grid_purchased_yesterday_energy     =8 ;
	ginlongmonitor__minverter1->grid_sell_total_energy              =8 ;
	ginlongmonitor__minverter1->grid_sell_today_energy              =8 ;
	ginlongmonitor__minverter1->grid_sell_yesterday_energy          =8 ;
	ginlongmonitor__minverter1->home_load_total_energy              =8 ;
	ginlongmonitor__minverter1->home_load_today_energy              =8 ;
	ginlongmonitor__minverter1->home_load_yesterday_energy          =8 ;
	ginlongmonitor__minverter1->standard_working_mode               =8 ;
	ginlongmonitor__minverter1->pv5_n_value.len                         =sizeof(pv5_n_value)/sizeof(pv5_n_value[0]); 
	ginlongmonitor__minverter1->pv5_n_value.data                        =pv5_n_value;
		
	ginlongmonitor__minverter1->iv_status_time                      =8 ;
	//ginlongmonitor__minverter1->iv_run_status                       =8 ;//锦浪说去掉
	ginlongmonitor__minverter1->iv_array_value.len                      =sizeof(iv_array_value)/sizeof(iv_array_value[0]); 
	ginlongmonitor__minverter1->iv_array_value.data						=iv_array_value ;
	ginlongmonitor__minverter1->inverter_sn2                        ="123456" ;
	ginlongmonitor__minverter1->u_init_gnd                          =8 ;
	ginlongmonitor__minverter1->dc_bus                              =8 ;
	ginlongmonitor__minverter1->dc_bus_half                         =8 ;
	ginlongmonitor__minverter1->p_limit_set                         =8 ;
	ginlongmonitor__minverter1->p_factor_limit_set                  =8 ;
	ginlongmonitor__minverter1->p_reactive_limit_set                =8 ;
	ginlongmonitor__minverter1->battery_type                        =8 ;
	ginlongmonitor__minverter1->soc_discharge_set                   =8 ;
	ginlongmonitor__minverter1->soc_charging_set                    =8 ;
	ginlongmonitor__minverter1->p_epm_set                           =8 ;
	ginlongmonitor__minverter1->epm_fail_safe                       =8 ;
	ginlongmonitor__minverter1->p_epm                               =8 ;
	ginlongmonitor__minverter1->psum                                =8 ;
	ginlongmonitor__minverter1->insulation_resistance               =8 ;

	// buf_len = ginlong_monitor__m__inverter1__get_packed_size(&ginlongmonitor__minverter1);

	// if ((buf = fibo_malloc(buf_len)) == NULL)
	// return -1;

	// ret = ginlong_monitor__m__inverter1__pack(&ginlongmonitor__minverter1, buf);
	// APP_PRINT("buf_len:%d\tpacked_size:%lu\r\n", buf_len, ret);
	// APP_PRINT("publish inverter data:\r\n");
	// output((const char *)buf,buf_len);

	// fibo_aliyunMQTT_cloudPub_FixedLen(aliyun_mqtt_thread_handlet,pub_topic,1,buf,buf_len);
	// fibo_free(buf);
}

#endif

void packet_init(void)
{	
	inverter = (GinlongMonitor__MInverter1**)fibo_malloc(sizeof(GinlongMonitor__MInverter1*));
	inverter[0]=(GinlongMonitor__MInverter1*)fibo_malloc(sizeof(GinlongMonitor__MInverter1));
	ginlong_monitor__packet__init(&packet);      
	ginlong_monitor__m__notification__init(&notification); 
	ginlong_monitor__m_collector1__init(&collector1);
	ginlong_monitor__m__inverter1__init(inverter[0]);
}


int send_monitor_packet(void *aliyun_mqtt_thread_handle)
{
	int buf_len  = 0;
	int ret = 0;
	void *buf = NULL;
	
	set_collector_packet_data(&collector1);
	set_inverter_packet_data(inverter[0]);
	notification.collector11 = &collector1;
	
	notification.n_inverter1      = 1;
	notification.inverter1 = inverter;
	
	packet.time  = 1605763149;
	packet.no    = 64;
    packet.sn    = str_to_hex("6012000000000002");
	packet.notification = &notification;	
	
	buf_len = ginlong_monitor__packet__get_packed_size(&packet);//得到包的大小

	if ((buf = fibo_malloc(buf_len)) == NULL)
		return -1;

	ret = ginlong_monitor__packet__pack(&packet, buf);//打包
	APP_PRINT("buf_len:%d\tpacked_size:%lu\r\n", buf_len, ret);
	APP_PRINT("publish packet data:\r\n");
	//output((const char *)buf,buf_len);

	unpacket = ginlong_monitor__packet__unpack(NULL,buf_len,buf);

		 APP_PRINT("unpacket->notification->inverter1->inverter_sn2 = %s\r\n",unpacket->notification->inverter1[0]->inverter_sn2);
		 APP_PRINT("unpacket->notification->inverter1->u_pv1 = %d\r\n",unpacket->notification->inverter1[0]->u_pv1);
		 APP_PRINT("unpacket->notification->inverter1->i_pv1 = %d\r\n",unpacket->notification->inverter1[0]->i_pv1);
		 APP_PRINT("unpacket->time = %d\r\n",unpacket->time);
		 APP_PRINT("unpacket->no = %d\r\n",unpacket->no);
		 APP_PRINT("unpacket->sn = %lld\r\n",unpacket->sn);
	fibo_aliyunMQTT_cloudPub_FixedLen(aliyun_mqtt_thread_handle,pub_topic,1,buf,buf_len);

	fibo_free(buf);

	return 0;
}


