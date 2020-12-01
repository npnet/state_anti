
#include "ali_data_packet.h"

static GinlongMonitor__MNotification notification = {0} ;
static GinlongMonitor__MCollector1 collector1   =  {0};
static GinlongMonitor__Packet   packet = {0};
static GinlongMonitor__Packet   *unpacket = NULL;
static GinlongMonitor__MInverter1 ** inverter = NULL;

Inverter_Packet1 *inverter_data1 = NULL;
Inverter_Packet2 *inverter_data2 = NULL;
Inverter_Packet3 *inverter_data3 = NULL;
Inverter_Packet4 *inverter_data4 = NULL;
Inverter_Packet5 *inverter_data5 = NULL;
Inverter_Packet6 *inverter_data6 = NULL;



Para_Table para;

u16_t Swap_hex_Char( char *buf,const char * hex, u16_t len, char fill)
{
    u8_t i;
    u16_t l = 0;

    if(fill != 0)
    {
        l = (len << 1) + len;
    }
    else
    {
        l = len << 1;
    }
    while (len-- > 0)
    {
        i = ((*hex)&0xF0) >> 4;
        *buf++ = i > 9 ? i + ('A' - 10) : i + '0';
        i = (*hex++)&0x0F;
        *buf++ = i > 9 ? i + ('A' - 10) : i + '0';
        if (fill != 0)
        {
            *buf++ = fill;
        }
    }
    *buf = '\0';

    return l;
}

//0xA1B2 ----> 0x1A2B 变成
uint16_t byte_swap(uint16_t data)
{
    uint16_t temp = 0;
    temp = ((data & 0xF000)>>4) | ((data & 0xF00)<<4) | ((data & 0x00F0)>>4) | ((data & 0x000F)<<4);
    return temp;
}

//逆变器SN号字符串
void calc_inverter_str()
{
    uint16_t inverter_sn_h = 0 ;
    uint16_t inverter_sn_m1= 0;
    uint16_t inverter_sn_m2 = 0;
    uint16_t inverter_sn_l = 0;

    inverter_sn_h = byte_swap(inverter_data5->inverter_sn_h);
    inverter_sn_m1 = byte_swap(inverter_data5->inverter_sn_m1);
    inverter_sn_m2 = byte_swap(inverter_data5->inverter_sn_m2);
    inverter_sn_l = byte_swap(inverter_data5->inverter_sn_l);

    char hex[] = { (inverter_sn_h&0xff00)>>8,(inverter_sn_h&0xff),(inverter_sn_m1&0xff00)>>8,(inverter_sn_m1&0xff),(inverter_sn_m2&0xff00)>>8,(inverter_sn_m2&0xff),(inverter_sn_l&0xff00)>>8,(inverter_sn_l&0xff)};
    Swap_hex_Char(para.inverter_str,hex,sizeof(hex)/sizeof(hex[0]),0);
    para.inverter_str[15] = 0;
    APP_PRINT("sn : %s\r\n",para.inverter_str);
}

void out_put_buffer(const char *buf, uint32_t len)
{
    u32_t displayNum = 0;

    if (len < 512)
    {
        u8_t *str = fibo_malloc(len * 3 + 8);
        if (str != null)
        {
            int l = Swap_hex_Char((char *)str, buf, len, ' ');
            while (l)
            {
                if (l >= 16 * 3)
                {
                    Debug_output(str + displayNum, 16 * 3);
                    l -= 16 * 3;
                    displayNum += 16 * 3;
                }
                else
                {
                    Debug_output(str + displayNum, l);
                    l = 0;
                }
                Debug_output((u8_t *)"\r\n", 2);
            }

            if (str != NULL)
            {
                fibo_free(str);
            }
        }
    }
}




/*******************************************************************************
 * 将时间格式字符串转成时间戳形式
 * 如 2011-12-31 11:43:07 转成 1325302987
*******************************************************************************/

uint32_t get_tick(char *str_time)
{
    struct tm stm;
    int iY, iM, iD, iH, iMin, iS;

    APP_PRINT("tick = %s\r\n",str_time);
    memset(&stm,0,sizeof(stm));

    iY = atoi(str_time);
    iM = atoi(str_time+5);
    iD = atoi(str_time+8);
    iH = atoi(str_time+11);
    iMin = atoi(str_time+14);
    iS = atoi(str_time+17);

    stm.tm_year=iY-1900;
    stm.tm_mon=iM-1;
    stm.tm_mday=iD;
    stm.tm_hour=iH-8;
    stm.tm_min=iMin;
    stm.tm_sec=iS;

    return mktime(&stm);
}

/*******************************************************************************
 * 将字符串字符转成16进制数  如 "6032011300000003" --> 0x6032011300000003
*******************************************************************************/

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

/*******************************************************************************
 * 删除字符串中某个字符
*******************************************************************************/

char *del_char(char *str, char ch)
{
    unsigned char i=0,j=0;
    if((0==strlen(str))||str ==NULL)
        return NULL;

    while(str[i] != '\0')
    {
        if(str[i] != ch)
        {
            str[j++] = str[i];
        }
        i++;  //源一直移动
    }

    str[j] = '\0';
    return str;
}


/*******************************************************************************
 * 将5号参数固件版本中的'.'去掉,并转成16进制整数
 * uint32_t collector_version_convert()
*******************************************************************************/
uint32_t collector_version_convert(char * str)
{
    if(NULL != del_char(str,'.'))
        return(str_to_hex(str));
    else
        return 0;
}


void set_collector_packet_data(GinlongMonitor__MCollector1 *ginlongmonitor__mcollector1)
{

    if (ginlongmonitor__mcollector1 == NULL)
        return;

    ginlongmonitor__mcollector1->collector_sn                  = str_to_hex(para.device_name);
    ginlongmonitor__mcollector1->total_working_time            = para.total_working_time + para.current_working_tick;
    ginlongmonitor__mcollector1->current_working_time          = para.current_working_tick;
    ginlongmonitor__mcollector1->data_upload_cycle             = para.data_upload_cycle;
    ginlongmonitor__mcollector1->maximum_number                = 1;
    ginlongmonitor__mcollector1->actual_number                 = 1;
    // ginlongmonitor__mcollector1->collector_version             = para.software_ver;
    ginlongmonitor__mcollector1->collector_version             = 554107154;//采集器的版本需与锦浪沟通

    ginlongmonitor__mcollector1->factory_time                  = para.factory_time;
    ginlongmonitor__mcollector1->rssi                          = para.rssi;

    ginlongmonitor__mcollector1->is_realtime                   = 0;
}


/*
 *高8位与低8位数据交换
 *如 0xA1A2 --> 0xA2A1
 */
uint16_t H8_Swap_L8(uint16_t data)
{
    return ((data&0xff00)>>8) | ((data&0x00ff)<<8);
}

/**
 *两个16位数据变成一个32位数   
 *0x1234 0x4567 -->  0x34126745
 */
uint32_t data32_calc(uint16_t H16 ,uint16_t L16)
{
    uint32_t datah = 0;
	uint32_t datal = 0;
	datah = H8_Swap_L8(H16);
	datal = H8_Swap_L8(L16);
	return (datah << 16) | datal;
}


void set_inverter_packet_data(GinlongMonitor__MInverter1  *ginlongmonitor__minverter1)
{

    if (ginlongmonitor__minverter1 == NULL)
        return;
    calc_inverter_str();
    ginlongmonitor__minverter1->is_realtime                         = 0;  
    ginlongmonitor__minverter1->inverter_sn.len                     =0;
    ginlongmonitor__minverter1->inverter_sn.data                    =NULL;
    ginlongmonitor__minverter1->inverter_sn2                        =para.inverter_str ;
    ginlongmonitor__minverter1->inverter_temperature                =H8_Swap_L8(inverter_data4->inverter_temperature);
	ginlongmonitor__minverter1->product_model                       =H8_Swap_L8(inverter_data1->product_model);
    ginlongmonitor__minverter1->national_standards                  =H8_Swap_L8(inverter_data5->national_standards);
    ginlongmonitor__minverter1->u_pv1                               =H8_Swap_L8(inverter_data2->u_pv1);
    ginlongmonitor__minverter1->i_pv1                               =H8_Swap_L8(inverter_data2->i_pv1);
    ginlongmonitor__minverter1->u_pv2                               =H8_Swap_L8(inverter_data2->u_pv2);
    ginlongmonitor__minverter1->i_pv2                               =H8_Swap_L8(inverter_data2->i_pv2);
    ginlongmonitor__minverter1->u_pv3                               =H8_Swap_L8(inverter_data2->u_pv3);
    ginlongmonitor__minverter1->i_pv3                               =H8_Swap_L8(inverter_data3->i_pv3);
    ginlongmonitor__minverter1->u_pv4                               =H8_Swap_L8(inverter_data3->u_pv4);
    ginlongmonitor__minverter1->i_pv4                               =H8_Swap_L8(inverter_data3->i_pv4);
    ginlongmonitor__minverter1->u_ac1                               =H8_Swap_L8(inverter_data3->u_ac1);
    ginlongmonitor__minverter1->u_ac2                               =H8_Swap_L8(inverter_data3->u_ac2);
    ginlongmonitor__minverter1->u_ac3                               =H8_Swap_L8(inverter_data3->u_ac3);
    ginlongmonitor__minverter1->i_ac1                               =H8_Swap_L8(inverter_data3->i_ac1);
    ginlongmonitor__minverter1->i_ac2                               =H8_Swap_L8(inverter_data3->i_ac2);
    ginlongmonitor__minverter1->i_ac3                               =H8_Swap_L8(inverter_data3->i_ac3);
    ginlongmonitor__minverter1->fac                                 =H8_Swap_L8(inverter_data4->fac);
	ginlongmonitor__minverter1->e_today                             =H8_Swap_L8(inverter_data2->e_today);
	
    ginlongmonitor__minverter1->inverter_software_version           =data32_calc(inverter_data1->dsp_software_version,inverter_data1->tft_software_version);
    ginlongmonitor__minverter1->pac                                 =data32_calc(inverter_data1->pac_h,inverter_data1->pac_l);
    ginlongmonitor__minverter1->e_month                             =data32_calc(inverter_data1->e_month_h,inverter_data1->e_month_l);
    ginlongmonitor__minverter1->e_year                              =data32_calc(inverter_data2->e_year_h,inverter_data2->e_year_l);
    ginlongmonitor__minverter1->e_total                             =data32_calc(inverter_data1->e_total_h,inverter_data1->e_total_l);



}

void load_config_para(void)
{
    Buffer_t databuf;
    char software_ver[10]= {0};
    memset(&databuf,0,sizeof(databuf));
    parametr_get(5, &databuf);
    strncpy(software_ver,databuf.payload,databuf.lenght+1);//软件版本
    para.software_ver = collector_version_convert(software_ver);
    APP_PRINT("after of cov software_ver = %x\r\n",para.software_ver);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(92, &databuf);
    strncpy(para.product_key,databuf.payload,databuf.lenght+1);//product_key
    APP_PRINT("product_key = %s\r\n",para.product_key);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(93, &databuf);
    strncpy(para.device_name,databuf.payload,databuf.lenght+1);//device_name
    APP_PRINT("device_name = %s\r\n",para.device_name);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(94, &databuf);
    strncpy(para.device_secret,databuf.payload,databuf.lenght+1);//
    APP_PRINT("device_secret = %s\r\n",para.device_secret);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(95, &databuf);
    strncpy(para.pub_topic,databuf.payload,databuf.lenght+1);//
    APP_PRINT("pub_topic = %s\r\n",para.pub_topic);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(96, &databuf);
    strncpy(para.sub_topic,databuf.payload,databuf.lenght+1);//
    APP_PRINT("sub_topic = %s\r\n",para.sub_topic);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(97, &databuf);
    strncpy(para.TotoalWorkTime,databuf.payload,databuf.lenght+1);//
    APP_PRINT("TotoalWorkTime = %s\r\n",para.TotoalWorkTime);
    para.total_working_time = atoi(para.TotoalWorkTime);

    memset(&databuf,0,sizeof(databuf));
    parametr_get(82, &databuf);//设备数据上报周期
    para.data_upload_cycle = atoi(databuf.payload);
    APP_PRINT("data_upload_cycle = %d\r\n",para.data_upload_cycle);


    memset(&databuf,0,sizeof(databuf));
    parametr_get(07, &databuf);//出厂时间
    para.factory_time = get_tick(databuf.payload);
    APP_PRINT("factory_time = %ld\r\n",para.factory_time );

    memset(&databuf,0,sizeof(databuf));
    parametr_get(55, &databuf);//基站信号强度
    para.rssi = atoi(databuf.payload);
    APP_PRINT("rssi = %d\r\n",para.rssi );

}

int send_monitor_packet(void *aliyun_mqtt_thread_handle)
{
    int buf_len  = 0;
    int ret = 0;
    void *buf = NULL;

    inverter = (GinlongMonitor__MInverter1**)fibo_malloc(sizeof(GinlongMonitor__MInverter1*));
    inverter[0]=(GinlongMonitor__MInverter1*)fibo_malloc(sizeof(GinlongMonitor__MInverter1));


    ginlong_monitor__packet__init(&packet);
    ginlong_monitor__m__notification__init(&notification);
    ginlong_monitor__m_collector1__init(&collector1);
    ginlong_monitor__m__inverter1__init(inverter[0]);


    set_collector_packet_data(&collector1);
    set_inverter_packet_data(inverter[0]);
    notification.collector11 = &collector1;

    notification.n_inverter1      = 1;
    notification.inverter1 = inverter;

    packet.no++;
    packet.sn    = str_to_hex(para.device_name);
    packet.notification = &notification;

    buf_len = ginlong_monitor__packet__get_packed_size(&packet);//得到包的大小

    if ((buf = fibo_malloc(buf_len)) == NULL)
        return -1;

    ret = ginlong_monitor__packet__pack(&packet, buf);//打包
    
	APP_PRINT("mqtt send:");	  
	out_put_buffer((const char *)buf,buf_len);
    APP_PRINT("\r\n");
	
    fibo_aliyunMQTT_cloudPub_FixedLen(aliyun_mqtt_thread_handle,para.pub_topic,1,buf,buf_len);
    fibo_free(buf);
    fibo_free(inverter[0]);
    fibo_free(inverter);
    return 0;
}


uint32_t get_data_upload_cycle(void)
{
    return para.data_upload_cycle;
}

uint64_t* get_current_working_tick(void)
{
    return &para.current_working_tick;
}

uint64_t* get_total_tick(void)
{
    return &para.total_working_time;
}


