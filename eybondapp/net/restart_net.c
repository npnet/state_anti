/******************************************************************************           
* name:             restart_net.c           
* introduce:        重启网络连接
* author:           Luee                                     
******************************************************************************/ 
#include "restart_net.h"

//宏定义
#define single_sim_card         0
#define DEF_IP_MAX_SIZE         46
#define DEF_PORT_MAX_SIZE       6
#define U16PORT                 502

const char HOSTNAME[]={"www.baidu.com"};                

//全局变量
u32_t g_EventFlag = 0;
u8_t g_RemoteIp[DEF_IP_MAX_SIZE]       = {0};    //IP
u8_t g_RemotePort[DEF_PORT_MAX_SIZE]   = {"502"};    //端口号 0-65535


/******************************************************************************            
 * name:             void restart_net()          
 * introduce:        检测到消息，重连网络        
 * parameter:        none                 
 * return:           none         
 * author:           Luee                                              
 *****************************************************************************/
void restart_net(void)
{
    APP_PRINT("\r\nget queue : restart net\r\n");
    //检测SIM卡插入否？
    get_simstatus();
    //检查SIM注册，激活PDP
    active_pdp();
    //连接主机
    tcp_connection();
}

/******************************************************************************            
 * name:             void get_simstatus(void)        
 * introduce:        得到SIM卡插拔状态        
 * parameter:        none                 
 * return:           none         
 * author:           Luee                                              
 *******************************************************************************/
void get_simstatus(void)
{
    u8_t simstatus;
    s32_t simret;
    u8_t no_insert = 1;
    while(no_insert)
    {
        //得到SIM卡插拔状态   
        simret=fibo_get_sim_status(&simstatus);
        //延时1S
		fibo_taskSleep(1000);
		if((simstatus==1)&&(simret==0))
		{
            //SIM卡已插入
			no_insert = 0;
            APP_PRINT("\r\nsim is insert !\r\n");        
		}
        else
        {
            APP_PRINT("\r\nsim no checked,please insert sim & retry\r\n");
        }
    }   //end of while
}   //end of get_simstatus

/******************************************************************************            
 * name:             void active_pdp(void)        
 * introduce:        检测SIM卡是否已注册成功，然后激活PDP        
 * parameter:        none                 
 * return:           none         
 * author:           Luee                                              
 *******************************************************************************/
void active_pdp(void)
{
    //判断SIM卡是否成功注册网络
    u8_t ip[50];
    reg_info_t sim_reg_info;

    u8_t no_reg = 1;
    while(no_reg)
    {
        //得到SIM卡注册信息，并将数据放在结构体sim_reg_info
	    fibo_getRegInfo(&sim_reg_info, 0);
        //延时1S
		fibo_taskSleep(1000);
		if(1 == sim_reg_info.nStatus)
		{
            //SIM卡已注册
			no_reg = 0;
            //激活PDP连接
			fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
			fibo_taskSleep(1000);
            //确定已连接成功
            APP_PRINT("\r\nsim register net success & active pdp\r\n");
            return;
		}
        else
        {
            APP_PRINT("\r\nsim register processing\r\n");
        }
    }   //end of while
}   //end of active_pdp

/*******************************************************************************            
 * name:             void tcp_connection(void)        
 * introduce:        TCP连接函数调用     
 * parameter:        none                 
 * return:           none         
 * author:           Luee                        
 * changetime:                            
 *******************************************************************************/
void tcp_connection(void)
{
    APP_PRINT("\r\ntcp connecting\r\n");

    int tcp_cycle_flag = 1;
    int tcp_try_count  = 1;
    int launch_tcp_ret = 1;

    while(tcp_cycle_flag)
    {
        launch_tcp_ret = launch_tcp_connection();

        if(0 == launch_tcp_ret)
        {
            APP_PRINT("\r\nlaunch tcp success,tcp_try_count is %d\r\n",tcp_try_count);
            tcp_cycle_flag = 0;
            //srv_lamp_on();

        }

        if(-1 == launch_tcp_ret)
        {
            APP_PRINT("\r\nlaunch tcp fail,tcp_try_count is %d\r\n",tcp_try_count);
            //srv_lamp_off();
        }

        tcp_try_count++;
        fibo_taskSleep(1000);   //不能删
        if(60 == tcp_try_count) //60*1000 1分钟未联网成功则退出
        {
            tcp_cycle_flag = 0;   
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


    if(1 == g_EventFlag)
	{
		u16Port = atoi((char*)g_RemotePort); 
        dns_ret = fibo_getHostByName((char*)g_RemoteIp,&addr_para,1,single_sim_card);//0成功 小于0失败 

        if(0 == dns_ret)
        {
            APP_PRINT("\r\nproduction test dns getHostByName success\r\n");
            return 0;
        }

        if(0 < dns_ret)
        {
            APP_PRINT("\r\nproduction test dns getHostByName fail\r\n");
            return -1;
        }

		APP_PRINT("\r\nproduction test u16Port:%d\r\n",u16Port);
		APP_PRINT("\r\nproduction test g_RemoteIp:%s\r\n",g_RemoteIp);
	}
    else
    {
        //服务器端口
        u16Port = atoi((char*)g_RemotePort); 
        APP_PRINT("\r\nu16port= %d\r\n",u16Port);
         
        //服务器ip地址
        //buf = fibo_malloc(sizeof(char)*64);
        //memset(buf, 0, sizeof(char)*64);
        //char *buf = (char *)&HOSTNAME;
        dns_ret = fibo_getHostByName((char *)&HOSTNAME,&addr_para,1,single_sim_card);//0成功 小于0失败

        APP_PRINT("\r\nhosthome is %s\r\n",HOSTNAME);
        
        if(0 == dns_ret)
        {
            APP_PRINT("\r\ndns getHostByName success\r\n");
            return 0;
        }
        
        if(0 < dns_ret)
        {
            APP_PRINT("\r\ndns getHostByName fail\r\n");
            return -1;
        } 
        //fibo_free(buf);
    }

    /*
    //sockets建立
    GAPP_TCPIP_ADDR_T addr;
    memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
    socketid                    = fibo_sock_create(0);//0-TCP,1-UDP
    addr.sin_port               = swap_hl8(u16Port);
    addr.sin_addr.u_addr.ip4    = addr_para.u_addr.ip4;
    addr.sin_addr.type          = AF_INET;
    INT32 retcode               = fibo_sock_connect(socketid, &addr);
    if(0 == retcode)
    {
        APP_PRINT("\r\nfibo_sock_connect success\r\n");
        return 0;
    }
    else
    {
        APP_PRINT("\r\nfibo_sock_connect fail\r\n");
        return -1;
    } 
    */
}

