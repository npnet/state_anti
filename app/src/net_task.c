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
#include "elog.h"
#include "net_task.h"
// #include "ip4_addr.h"
#include "eybond_modbus_tcp_protocol.h"
 char hostname1[] = "solar.eybond.com";
// char hostname1[] = "www.shinemonitor.com";

INT8 socketid;
ip_addr_t addr1;

UINT8 ip_1[50];

char 			g_RecvData[2*1024] 	= {0};
UINT16 			g_RecvDataLen 		=  0;

char 			g_SendData[2*1024] 	= {0};
UINT16 			g_SendDataLen 		=  0;

/*机器大小端测试*/
/*
内存存储的大小端模式：
高字节位存储在高地址   ---称为小端
高字节位存储在低地址   ---称为大端
----无论大小端，都可以使用，但一个地方只能用一种否则会出错.
*/

int big_little_endian_test()
{
	int a;
	a = 0x10000001;			//指针测试
	char b = *((char*)(&a));
	if(b==0x10)
	log_d("\r\nbig mode\r\n");
	else if(b==0x01)
	log_d("\r\nlittle mode\r\n");
	return 0;
}

//网络任务
void net_task(void)
{
    big_little_endian_test();
    int i = 0;
    //判断是否成功注册网络
    reg_info_t reg_info1;
    INT32 retreg = fibo_getRegInfo(&reg_info1,0);
    log_d("retreg %d", retreg);
    log_d("reg_info1->curr_rat %d", reg_info1.curr_rat);//当前注册RAT
    log_d("reg_info1->nStatus %d", reg_info1.nStatus);//当前注册状态，0表示未注册，1表示注册上。
    log_d("reg_info1->lte_scell_info.tac %d", reg_info1.lte_scell_info.tac);
    log_d("reg_info1->lte_scell_info.cell_id %d", reg_info1.lte_scell_info.cell_id);
    log_d("reg_info1->gsm_scell_info.lac %d", reg_info1.gsm_scell_info.lac);
    log_d("reg_info1->gsm_scell_info.cell_id %d", reg_info1.gsm_scell_info.cell_id);

    while(1 != reg_info1.nStatus)
    {
        fibo_taskSleep(1000);         //延时2S执行一次
        i++;
       
        retreg = fibo_getRegInfo(&reg_info1,0);
		log_d("i = %d reg_info1->nStatus %d",i,reg_info1.nStatus);
        if(10 == i)
        {
            break;
        }
    }
    if(1 == reg_info1.nStatus)
    {
        log_d("register network success");
    }
    memset(&ip_1, 0, sizeof(ip_1));
    //激活PDP
    INT32 retpdp = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip_1);//返回0且IP不为0表示成功,否则失败。
    log_d("retpdp %d,ip_1=%s", retpdp,ip_1);

    for(int j = 0;j<5;j++)
    {
        retpdp = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip_1);//返回0且IP不为0表示成功,否则失败。
        log_d("retpdp %d,ip_1=%s", retpdp,(char *)ip_1);
        fibo_taskSleep(1000);         //延时2S执行一次
    }
    
    //域名解析
    INT32 rethost = fibo_getHostByName(hostname1,&addr1,1,0);//0成功 小于0失败
    log_d("rethost %d", rethost);
    log_d("addr1.type is %d", addr1.type);
    log_d("addr1.u_addr.ip4 is %lld", addr1.u_addr.ip4);
    log_d("addr1.u_addr.ip6 is %lld", addr1.u_addr.ip6);

    for(int k = 0;k<5;k++)
    {
        rethost = fibo_getHostByName(hostname1,&addr1,1,0);//0成功 小于0失败
        log_d("rethost %d", rethost);
        log_d("addr1.type is %d", addr1.type);
        log_d("addr1.u_addr.ip4 is %lld", addr1.u_addr.ip4);
        log_d("addr1.u_addr.ip6 is %lld", addr1.u_addr.ip6);
        fibo_taskSleep(1000);         //延时2S执行一次
    }

    //sockets建立
    GAPP_TCPIP_ADDR_T addr;

    memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));

    socketid = fibo_sock_create(0);//0-TCP,1-UDP

    addr.sin_port = htons(502);
  


    addr.sin_addr.u_addr.ip4 = addr1.u_addr.ip4;
    addr.sin_addr.type       = AF_INET;

    INT32 retcode            = fibo_sock_connect(socketid, &addr);

    log_d("retcode is %lld", retcode);


    INT32 g_SendDataLenTrue = 0;
    bool check_flag = false;
    while(1)
    {
        g_RecvDataLen = fibo_sock_recv(socketid, (UINT8 *)g_RecvData, sizeof(g_RecvData));
        log_hex((UINT8 *)g_RecvData,g_RecvDataLen);
        check_flag = data_frame_legal_checking((UINT8 *)g_RecvData);
        if(TRUE == check_flag )	/*判断数据帧是否为modbus_tcp*/
        {
            receiving_processing(g_RecvData[7]);
            memset(g_RecvData, 0, sizeof(g_RecvData));		//接收buff清零
            g_RecvDataLen = 0;								//接收长度清零
            g_SendDataLenTrue = fibo_sock_send(socketid, (UINT8 *)g_SendData, g_SendDataLen);
            log_hex((UINT8 *)g_SendData,g_SendDataLen);
            log_d("g_SendDataLenTrue is %lld", g_SendDataLenTrue);
        }
        else
        {
          log_d("wait data");  
          fibo_taskSleep(1000);         //延时2S执行一次
        }
    }
    //收发数据

    //按需closesockets
}