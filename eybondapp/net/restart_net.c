/******************************************************************************           
* name:             restart_net.c           
* introduce:        重启网络连接
* author:           Luee                                     
******************************************************************************/ 
#include "restart_net.h"
#include "eybpub_watchdog.h"
#include "eyblib_list.h"

//宏定义
//#define CMD_FEEDDOG             1
#define single_sim_card         0
#define DEF_IP_MAX_SIZE         46
#define DEF_PORT_MAX_SIZE       6
#define U16PORT                 502

#define SIM_CHECK_TIMES         6
#define SIM_REGISTER_TIMES      60
#define TCP_CONNECT_TIMES       60
#define REALTIME_CHECK_NET_INTERVAL 60

#define GET_SIMSTATUS           1
#define SIM_REGISTER            2
#define ACTIVE_PDP              3
#define TCP_CONNECTION          4
#define CREATE_SOCKET           5


static char HOSTNAME[]={"www.shinemonitor.com"};  
static char PING_HOSTNAME[]={"www.baidu.com"};

//外部变量
extern u32_t EYBNET_TASK;
extern u8_t FeedFlag;

//全局变量
u32_t g_EventFlag = 0;
u8_t g_RemoteIp[DEF_IP_MAX_SIZE]       = {0};    //IP
u8_t g_RemotePort[DEF_PORT_MAX_SIZE]   = {"502"};    //端口号 0-65535

u32_t EYBRELINK_TASK=0;
static u8_t relink_flag=0;          //=1重连网络
static u8_t relink_index=0;
static u8_t sim_check_times=0;
static u8_t sim_register_times=0;
static u8_t tcp_connect_times=0;

//函数宣告
static s32_t relink_per1s(void);
static void relink_init(void);

/******************************************************************************            
 * name:             static s32_t relink()          
 * introduce:        检测到消息，重连网络        
 * parameter:        none                 
 * return:           =0 驻网成功        
 * author:           Luee                                              
 *****************************************************************************/
static s32_t relink_per1s(void)
{
    s32_t ret;
    s32_t msg;

    u8_t simstatus;
    s32_t simret;

    u8_t ip[50];
    reg_info_t sim_reg_info;

    int launch_tcp_ret = 1;
    static u8_t tcp_connect_times=0;
    

    if(relink_flag){
        switch(relink_index){
        //检测SIM卡是否插入
        case GET_SIMSTATUS:
            simret=fibo_get_sim_status(&simstatus);
            if((simstatus==1)&&(simret==0)){
            //SIM卡已插入
            APP_PRINT("\r\nsim is insert !\r\n");
            msg=NET_MSG_SIM_READY;
            fibo_queue_put(EYBNET_TASK,&msg,0);
            sim_check_times=0;
            //下一步：SIM卡注册
            relink_index=SIM_REGISTER;
            sim_register_times=0;
            }
            else{
            APP_PRINT("\r\nsim no checked,please insert sim & retry\r\n");
            sim_check_times++;
            APP_PRINT("\r\nsim check times=%d\r\n",sim_check_times);
            if(sim_check_times>=SIM_CHECK_TIMES){
                //确认sim卡没有插入，退出
                APP_PRINT("\r\nexit sim check\r\n");
                msg=NET_MSG_SIM_FAIL;
                fibo_queue_put(EYBNET_TASK,&msg,0);
                GSMLED_Off();
                APP_PRINT("\r\nin network fail,as sim card isn't inserted\r\n");
                sim_check_times=0;
                relink_flag=0;      
                }
            }
        break;

        //SIM卡注册
        case SIM_REGISTER:
	        fibo_getRegInfo(&sim_reg_info, 0);
		    if(1 == sim_reg_info.nStatus){
                //SIM卡已注册
                APP_PRINT("\r\n\sim regitster success\r\n");
                sim_register_times=0;
                //下一步：激活PDP
                relink_index=ACTIVE_PDP; 
		    }
            else{
                APP_PRINT("\r\nsim register processing\r\n");
                sim_register_times++;
                APP_PRINT("\r\nsim register times=%d\r\n",sim_register_times);
                if(sim_register_times>=SIM_REGISTER_TIMES){
                //确认sim注册失败，退出
                APP_PRINT("\r\nexit sim register\r\n");
                APP_PRINT("\r\nin network fail,as sim card register fail\r\n");
                GSMLED_Off();
                sim_register_times=0;
                relink_flag=0;  
                }
            }
        break;

        case ACTIVE_PDP:
            //激活PDP连接
		    fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
            //确定已连接成功
            APP_PRINT("\r\nactiving pdp\r\n");
            //下一步：TCP连接
            relink_index=TCP_CONNECTION;
            tcp_connect_times=0;  
        break;

        //TCP连接
        case TCP_CONNECTION:
             launch_tcp_ret = launch_tcp_connection();
            if(0 == launch_tcp_ret){
                APP_PRINT("\r\ntcp connect success\r\n");
                GSMLED_On();
                tcp_connect_times=0;

                msg=NET_MSG_GSM_READY;
                fibo_queue_put(EYBNET_TASK,&msg,0);
                APP_PRINT("\r\nin network success!!!\r\n");

                //下一步：驻网成功退出
                relink_flag=0;
            }

            if(-1 == launch_tcp_ret){
                APP_PRINT("\r\ntcp connect fail,relink......\r\n");
                GSMLED_Off();
            
                tcp_connect_times++;
                APP_PRINT("\r\ntcp connect times=%d\r\n",tcp_connect_times);
                if(tcp_connect_times>=TCP_CONNECT_TIMES){
                //确认tcp连接失败，退出
                APP_PRINT("\r\nexit tcp connection\r\n");
                APP_PRINT("\r\nin network fail,as tcp connect fail\r\n");
                tcp_connect_times=0;
                relink_flag=0;  
                }
            }
        break;

        default:
        break;
        }   //swtich end
    }
}

/******************************************************************************            
 * name:             s32_t relink_init()          
 * introduce:        检测到消息，重连网络        
 * parameter:        none                 
 * return:           =0 驻网成功        
 * author:           Luee                                              
 *****************************************************************************/
static void relink_init(void)
{
    relink_flag=1;
    relink_index=GET_SIMSTATUS;
    sim_check_times=0;
    sim_register_times=0;
    tcp_connect_times=0;
}


/******************************************************************************            
 * name:             s32_t restart_net()          
 * introduce:        检测到消息，重连网络        
 * parameter:        none                 
 * return:           =0 驻网成功        
 * author:           Luee                                              
 *****************************************************************************/
s32_t restart_net(void)
{
    u32_t ret;
    static u32_t times=0;
    s32_t msg;

    times++;
    APP_PRINT("\r\nget queue : restart net times=%d\r\n",times);
    //检测SIM卡插入否？
    ret=get_simstatus();
    if(ret==0){  
        //SIM检测OK
        msg=NET_MSG_SIM_READY;
        fibo_queue_put(EYBNET_TASK,&msg,0);
    }
    else{
        msg=NET_MSG_SIM_FAIL;
        fibo_queue_put(EYBNET_TASK,&msg,0);
        APP_PRINT("\r\nin network fail,as sim card isn't inserted\r\n");
        return -1;
    }
    //检查SIM注册，激活PDP
    ret=active_pdp();
    if(ret!=0){
        APP_PRINT("\r\nin network fail,as sim card register fail\r\n");
        return -1;
    }
    //连接主机
    ret=tcp_connection();
    if(ret==0){
        //dns解析成功，建立socket
        msg=NET_MSG_GSM_READY;
        fibo_queue_put(EYBNET_TASK,&msg,0);
        APP_PRINT("\r\nin network success!!!\r\n");
        return 0;
    }
    else{
        APP_PRINT("\r\nin network fail,as tcp connect fail\r\n");
        msg=NET_MSG_GSM_FAIL;
        fibo_queue_put(EYBNET_TASK,&msg,0);
        return -1;
    }
}

/******************************************************************************            
 * name:             s32_t get_simstatus(void)        
 * introduce:        得到SIM卡插拔状态        
 * parameter:        none                 
 * return:           =0 ：sim插入          
 * author:           Luee                                              
 *******************************************************************************/
s32_t get_simstatus(void)
{
    u8_t simstatus;
    s32_t simret;
    u8_t no_insert = 1;
    u8_t sim_check_times=0;
    while(no_insert){
        //得到SIM卡插拔状态   
        simret=fibo_get_sim_status(&simstatus);
        //延时1S
        #ifdef CMD_FEEDDOG
        Watchdog_feed();
		fibo_taskSleep(500);
        #else
        fibo_taskSleep(1000);
        #endif
		if((simstatus==1)&&(simret==0)){
            //SIM卡已插入
			no_insert = 0;
            APP_PRINT("\r\nsim is insert !\r\n"); 
            return 0;       
		}
        else{
            APP_PRINT("\r\nsim no checked,please insert sim & retry\r\n");
        }
        sim_check_times++;
        APP_PRINT("\r\nsim check times=%d\r\n",sim_check_times);
        if(sim_check_times>=SIM_CHECK_TIMES){
            //确认sim卡没有插入，退出
            APP_PRINT("\r\nexit sim check\r\n");
            no_insert = 0;
            return -1;
        }
    }   //end of while
}   //end of get_simstatus

/******************************************************************************            
 * name:             s32_t active_pdp(void)        
 * introduce:        检测SIM卡是否已注册成功，然后激活PDP        
 * parameter:        none                 
 * return:           =0 sim注册和PDP激活成功          
 * author:           Luee                                              
 *******************************************************************************/
s32_t active_pdp(void)
{
    //判断SIM卡是否成功注册网络
    u8_t ip[50];
    reg_info_t sim_reg_info;
    u8_t sim_register_times=0;
    u8_t no_reg = 1;

    while(no_reg){
        //得到SIM卡注册信息，并将数据放在结构体sim_reg_info
	    fibo_getRegInfo(&sim_reg_info, 0);
        //延时1S
		#ifdef CMD_FEEDDOG
        Watchdog_feed();
		fibo_taskSleep(500);
        #else
        fibo_taskSleep(1000);
        #endif
		if(1 == sim_reg_info.nStatus){
            //SIM卡已注册
			no_reg = 0;
            //激活PDP连接
			fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
			Watchdog_feed();
		    fibo_taskSleep(500);
            //确定已连接成功
            APP_PRINT("\r\nsim register network success & active pdp\r\n");
            return 0;
		}
        else{
            APP_PRINT("\r\nsim register processing\r\n");
        }
        sim_register_times++;
        APP_PRINT("\r\nsim register times=%d\r\n",sim_register_times);
        if(sim_register_times>=SIM_REGISTER_TIMES){
            //确认sim注册失败，退出
            APP_PRINT("\r\nexit sim register\r\n");
            no_reg = 0;
            return -1;
        }
    }   //end of while
}   //end of active_pdp

/*******************************************************************************            
 * name:             s32_t tcp_connection(void)        
 * introduce:        TCP连接函数调用     
 * parameter:        none                 
 * return:           =0 TCP连接成功         
 * author:           Luee                        
 * changetime:                            
 *******************************************************************************/
s32_t tcp_connection(void)
{
    APP_PRINT("\r\ntcp connecting\r\n");

    int tcp_cycle_flag = 1;
    int launch_tcp_ret = 1;
    u8_t tcp_connect_times=0;

    #ifdef CMD_FEEDDOG
        Watchdog_feed();
		fibo_taskSleep(500);
        #else
        fibo_taskSleep(1000);
        #endif
    while(tcp_cycle_flag){
        launch_tcp_ret = launch_tcp_connection();

        if(0 == launch_tcp_ret){
            APP_PRINT("\r\ntcp connect success\r\n");
            tcp_cycle_flag = 0;
            GSMLED_On();
            return 0;

        }

        if(-1 == launch_tcp_ret){
            APP_PRINT("\r\ntcp connect fail,relink......\r\n");
            GSMLED_Off();
        }

        tcp_connect_times++;
        APP_PRINT("\r\ntcp connect times=%d\r\n",tcp_connect_times);
        if(tcp_connect_times>=TCP_CONNECT_TIMES){
            //确认tcp连接失败，退出
            APP_PRINT("\r\nexit tcp connection\r\n");
            tcp_cycle_flag = 0; 
            return -1;
        }
    }
}

/*******************************************************************************            
 * name:             s32_t launch_tcp_connection(void)       
 * introduce:        TCP连接函数调用     
 * parameter:        none                 
 * return:           none         proc_app_task
 * author:                                   
 * changetime:       2020.10.27                     
 *******************************************************************************/
s32_t launch_tcp_connection(void)
{
    char 		*buf  			= NULL;							//参数值buff
    UINT16		len 			= 64;   
    UINT16 		u16Port			= U16PORT;   
    ip_addr_t   addr_para;
    s32_t       dns_ret         = -1;

    s8_t             socketid; 

    if(1 == g_EventFlag){
		u16Port = atoi((char*)g_RemotePort); 
        dns_ret = fibo_getHostByName((char*)g_RemoteIp,&addr_para,1,single_sim_card);//0成功 小于0失败 

        if(0 == dns_ret){
            APP_PRINT("\r\nproduction test dns getHostByName success\r\n");
        }

        if(0 < dns_ret){
            APP_PRINT("\r\nproduction test dns getHostByName fail\r\n");
            return -1;
        }

		APP_PRINT("\r\nproduction test u16Port:%d\r\n",u16Port);
		APP_PRINT("\r\nproduction test g_RemoteIp:%s\r\n",g_RemoteIp);
	}
    else{
        //服务器端口
        u16Port = atoi((char*)g_RemotePort); 
        APP_PRINT("\r\nu16port= %d\r\n",u16Port);
         
        //服务器ip地址
        dns_ret = fibo_getHostByName((char *)&HOSTNAME,&addr_para,1,single_sim_card);//0成功 小于0失败

        APP_PRINT("\r\nhosthome is %s\r\n",HOSTNAME);
        
        if(0 == dns_ret){
            APP_PRINT("\r\ndns getHostByName success\r\n"); //dns解析成功，接下来建立sockets
            //return 0;
        }
        
        if(0 < dns_ret){
            APP_PRINT("\r\ndns getHostByName fail\r\n");
            return -1;                                      //dsn解析错误，直接返回
        } 
        //fibo_free(buf);
    }
 
    //sockets建立
    GAPP_TCPIP_ADDR_T addr;
    memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
    socketid                    = fibo_sock_create(0);//0-TCP,1-UDP
    addr.sin_port               = swap_hl8(u16Port);
    addr.sin_addr.u_addr.ip4    = addr_para.u_addr.ip4;
    addr.sin_addr.type          = AF_INET;
    INT32 retcode               = fibo_sock_connect(socketid, &addr);
    if(0 == retcode){
        APP_PRINT("\r\nfibo_sock_connect success\r\n");
        return 0;
    }
    else{
        APP_PRINT("\r\nfibo_sock_connect fail\r\n");
        return -1;
    } 
}

/*******************************************************************************            
 * name:             void realtime_check_net(void)      
 * introduce:        实时检测4G驻网    
 * parameter:        none                 
 * return:           none         
 * author:           Luee                        
 * changetime:                            
 *******************************************************************************/
void realtime_check_net(void)
{
    static u32_t check_net_counter=0;
    int msg=0;
    static s32_t ping_ret=-1;
    static ping_times=0;
    static u32_t times=0;
    static u8_t realtime_check_net_index=0;

    check_net_counter++;
    switch(realtime_check_net_index)
    {
        //ping
        case 0:
            if(check_net_counter>=REALTIME_CHECK_NET_INTERVAL)       //60s
            {
                check_net_counter=0;
                times++;
                APP_PRINT("\r\nrealtime checking network times=%d\r\n",times);
                //ping www.baidu.com 
                ping_ret=fibo_mping(1,&PING_HOSTNAME,4,32,64,0,4000);
                realtime_check_net_index=1;    
            }
        break;

        //ping result
        case 1:
            if(check_net_counter>=5)       //5s
            {
                check_net_counter=0;
                APP_PRINT("\r\nping_result:ping_ret=%d\r\n",ping_ret);
                realtime_check_net_index=0;   
                if(ping_ret==0){
                    ping_times=0;
                    APP_PRINT("\r\nping www.baidu.com is success\r\n");
                }
                else{
                    //重连网络
                    if(relink_flag==0){
                        msg=RELINK_RESTART_ID;
                    fibo_queue_put(EYBRELINK_TASK,&msg,0);
                    }
                
                    ping_times++;
                    APP_PRINT("\r\nping www.baidu.com fail &put restart network queue times=%d\r\n",ping_times);
                    if(ping_times>=6){
                         Watchdog_stop();         //停止喂狗，硬重启
                         APP_PRINT("\r\nFeedFlag =%d\r\n",FeedFlag);
                         APP_PRINT("\r\nstop watchdog,will hardward reset!!!\r\n");
                         while(1);
                    }

                }

            }
        break;
    }
}

/*******************************************************************************            
 * name:             static void relink_timer_callback(void *param)      
 * introduce:        1s定时器    
 * parameter:        none                 
 * return:           none         
 * author:           Luee                        
 * changetime:                            
 *******************************************************************************/
static void relink_timer_callback(void *param) 
{
    s32_t msg=RELINK_TIMER_ID;
    fibo_queue_put(EYBRELINK_TASK,&msg,0);
}

/*******************************************************************************            
 * name:             void proc_relink_task (void *param)      
 * introduce:        实时检测4G驻网    
 * parameter:        none                 
 * return:           none         
 * author:           Luee                        
 * changetime:                            
 *******************************************************************************/
void proc_relink_task (s32_t relink)
{
    u32_t relink_timer;
    u32_t relink_time_interval=1000;
    u32_t ret;
    u32_t msg;

    ListHandler_t list_relink_t;

    relink_timer = fibo_timer_period_new(relink_time_interval, relink_timer_callback, NULL);  // 注册实时联网Timer
    if (relink_timer == 0) {
    APP_PRINT("Register relink timer failed!!\r\n");
    }

    EYBRELINK_TASK = fibo_queue_create(5, sizeof(int));

    //开机重连网络
    msg=RELINK_RESTART_ID;
    fibo_queue_put(EYBRELINK_TASK,&msg,0);

    APP_PRINT("\r\nrelink task run...\r\n");
    //list_init(&netSendPakege);    // 初始化网络发送队列
    while (1) {
    fibo_queue_get(EYBRELINK_TASK, (void *)&msg, 0);
    switch (msg) {
      case RELINK_RESTART_ID:
        relink_init();
/*
        ret=restart_net();
        if (ret==0)
        {
           APP_PRINT("\r\nrestart network is successful\r\n");
        }
        else
        {
         APP_PRINT("\r\nrestart network is fail\r\n");
         GSMLED_Off();
        } 
*/
        break;
    case RELINK_TIMER_ID:  // 从APP task传递过来的定时器(1000 ms)消息
        realtime_check_net();
        relink_per1s();
        break;
      default:
        break;
    }    
  }
}





