
       
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

extern void test_printf(void);

INT32 acceptSocketID;
INT32 socketServerID;
INT32 g_task_hd = -1;
#define TEST_TASK_NAME "TEST_TASK"

#define in_range(c, lo, up) ((u8_t)c >= lo && (u8_t)c <= up)
//#define isdigit(c) in_range(c, '0', '9')
//#define isxdigit(c) (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
//#define islower(c) in_range(c, 'a', 'z')
//#define isspace(c) (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

#define HTONL(x) (x) = htonl((INT32)x)

unsigned long htonl(unsigned long n)
{
    return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
}

unsigned short htons(unsigned short n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

#if 1
int ip4addr_aton(const char *cp, ip4_addr_t *addr)
{
    u32_t val;
    u8_t base;
    char c;
    u32_t parts[4];
    u32_t *pp = parts;

    c = *cp;
    for (;;)
    {
        /*
     * Collect number up to ``.''.
     * Values are specified as for C:
     * 0x=hex, 0=octal, 1-9=decimal.
     */
        if (!isdigit(c))
        {
            return 0;
        }
        val = 0;
        base = 10;
        if (c == '0')
        {
            c = *++cp;
            if (c == 'x' || c == 'X')
            {
                base = 16;
                c = *++cp;
            }
            else
            {
                base = 8;
            }
        }
        for (;;)
        {
            if (isdigit(c))
            {
                val = (val * base) + (u32_t)(c - '0');
                c = *++cp;
            }
            else if (base == 16 && isxdigit(c))
            {
                val = (val << 4) | (u32_t)(c + 10 - (islower(c) ? 'a' : 'A'));
                c = *++cp;
            }
            else
            {
                break;
            }
        }
        if (c == '.')
        {
            /*
       * Internet format:
       *  a.b.c.d
       *  a.b.c   (with c treated as 16 bits)
       *  a.b (with b treated as 24 bits)
       */
            if (pp >= parts + 3)
            {
                return 0;
            }
            *pp++ = val;
            c = *++cp;
        }
        else
        {
            break;
        }
    }
    /*
   * Check for trailing characters.
   */
    if (c != '\0' && !isspace(c))
    {
        return 0;
    }
    /*
   * Concoct the address according to
   * the number of parts specified.
   */
    switch (pp - parts + 1)
    {

    case 0:
        return 0; /* initial nondigit */

    case 1: /* a -- 32 bits */
        break;

    case 2: /* a.b -- 8.24 bits */
        if (val > 0xffffffUL)
        {
            return 0;
        }
        if (parts[0] > 0xff)
        {
            return 0;
        }
        val |= parts[0] << 24;
        break;

    case 3: /* a.b.c -- 8.8.16 bits */
        if (val > 0xffff)
        {
            return 0;
        }
        if ((parts[0] > 0xff) || (parts[1] > 0xff))
        {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16);
        break;

    case 4: /* a.b.c.d -- 8.8.8.8 bits */
        if (val > 0xff)
        {
            return 0;
        }
        if ((parts[0] > 0xff) || (parts[1] > 0xff) || (parts[2] > 0xff))
        {
            return 0;
        }
        val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
        break;
    default:
        break;
    }
    if (addr)
    {
        ip4_addr_set_u32(addr, val);
    }
    return 1;
}

#endif


static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


UINT32 g_pdp_active_sem;
bool   test_pdp_active_flag;

UINT32 g_pdp_deactive_sem;
bool   test_pdp_deactive_flag;

UINT32 g_pdp_asyn_active_sem;
bool   test_pdp_asyn_active_flag;

UINT32 g_pdp_asyn_deactive_sem;
bool   test_pdp_asyn_deactive_flag;

UINT32 g_pdp_dns_active_sem;

char ip_test[60]={0x00, };



char g_send_test[4096] = {0};

static void prvThreadEntry_2(void *param)
{
    struct timeval tv;
    fd_set readfds;
    fd_set errorfds;
	fd_set writeset;
    GAPP_TCPIP_ADDR_T addr;
	INT8 socketid;
	int ret = -1;
	UINT8 ip[50];
	int test = 1;
	reg_info_t reg_info;
    UINT32 TICK1;
	UINT32 TICK2;
	int rcvnum = 0;
	int testend = 1;
	int testbegin = 1;	
    OSI_LOGI(0, "application thread enter, param 0x%x", param);


	while(test)
	{
	    fibo_getRegInfo(&reg_info, 0);
		fibo_taskSleep(1000);
		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		if(reg_info.nStatus==1)
		{
			test = 0;
			fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
			fibo_taskSleep(1000);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}

	INT32 retcode;
    char rcvbuf[2028];
	int NODELAY;
	memset(&addr,0,sizeof(addr));
	memset(rcvbuf,0,sizeof(rcvbuf));
	
	OSI_PRINTFI("[%s-%d]sys_sock_create start :\n", __FUNCTION__, __LINE__);
	socketid = fibo_sock_create(GAPP_IPPROTO_TCP);
	if(socketid <= 0){
		OSI_PRINTFI("[%s-%d]create socket failed, exit.", __FUNCTION__, __LINE__);
		goto error_exit;
	}
	OSI_PRINTFI("[%s-%d]create socket OK, socketid = %d\n", __FUNCTION__, __LINE__, socketid);

	NODELAY = 0;
	
	fibo_sock_setOpt(socketid,IPPROTO_TCP,TCP_NODELAY,&NODELAY,sizeof(NODELAY));

    fibo_taskSleep((UINT32)1000);

    addr.sin_port = htons(30000);

    ip4addr_aton("111.231.250.105", &addr.sin_addr.u_addr.ip4);
    addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
    retcode = fibo_sock_connect(socketid, &addr);
	if(retcode < 0){
		OSI_PRINTFI("[%s-%d]connect failed, exit.", __FUNCTION__, __LINE__);
		goto error_exit;
	}
	OSI_PRINTFI("[%s-%d]connect is OK.", __FUNCTION__, __LINE__);
	

    while(testend)
    {
    	FD_ZERO(&readfds);
		FD_ZERO(&errorfds);
		FD_ZERO(&writeset);
		FD_SET(socketid, &writeset);
		FD_SET(socketid, &readfds);
    	FD_SET(socketid, &errorfds);
		tv.tv_sec  = 5; //5
		tv.tv_usec = 0;
		ret = fibo_sock_lwip_select(socketid+1, &readfds, NULL, &errorfds, &tv);
		
		if(ret <= 0){
			if (ret < 0)
			{
				OSI_PRINTFI("[%s-%d]select ret < 0, error", __FUNCTION__, __LINE__);
			}
			if(ret == 0)
			{
				OSI_PRINTFI("[%s-%d]: select timeout.", __FUNCTION__, __LINE__);
			}
			continue;
		}
		if(FD_ISSET(socketid, &errorfds))	
		{
			int error = 0;
			int len = 4;
			OSI_PRINTFI("[%s-%d]socketid = %d, errno = %d\n", __FUNCTION__, __LINE__, socketid,errno);

			ret = fibo_sock_getOpt(socketid, SOL_SOCKET, SO_ERROR,&error, &len);
			OSI_PRINTFI("[%s-%d]fibo_sock_getOpt, error = %d\n", __FUNCTION__, __LINE__, error);

			fibo_taskSleep(1000);
			ret = fibo_sock_close(socketid);
			break;
		}
		else if(FD_ISSET(socketid, &readfds))
		{
	
			{
				memset(rcvbuf,0,sizeof(rcvbuf));
				if(testbegin)
				{
					TICK1 = fibo_getSysTick();
					testbegin =0;
				}
				rcvnum += fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 1800);

				if(testend && rcvnum >10*1024*1024)
				{
				    fibo_sock_close(socketid);
					testend = 0;
				}
			}

			
			if(rcvnum <= 0){
				OSI_PRINTFI("[%s-%d]recv error, errno = %d\n", __FUNCTION__, __LINE__, errno);
				goto error_exit;
			}
		}
	}

	if(testend ==0) 
	{
		TICK2 = fibo_getSysTick();
		float tmp1 = (((float)16384*rcvnum)/((TICK2-TICK1)*1024));
		float tmp2 = (float)(TICK2-TICK1)/16384;
		fibo_taskSleep(1000);
		OSI_PRINTFI("[%s-%d]rcvnum=%d,TICK2=%d,TICK1=%d,TICK2-TICK1=%d,speed=%fKBps,%fKbps,end_time=%f", __FUNCTION__, __LINE__,rcvnum,TICK2,TICK1,TICK2-TICK1,tmp1,8*tmp1,tmp2);
		rcvnum = 0;
	}


error_exit:
	fibo_taskSleep(5000);
	OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
	fibo_thread_delete();
}



static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {

	//fibo_PDPActive  ip address resopnse event
	case GAPP_SIG_PDP_ACTIVE_ADDRESS:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		char *addr = (char *)va_arg(arg, char *);
		if(addr != NULL)
		{
			OSI_PRINTFI("sig_res_callback  cid = %d, addr = %s ", cid, addr);
		}
		else 
		{
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
		va_end(arg);
	}
	break;

    //fibo_getHostByName event
	case GAPP_SIG_DNS_QUERY_IP_ADDRESS:
	{   
	    char *host = (char *)va_arg(arg, char *);
        char *ipv4_addr = (char *)va_arg(arg, char *);
		char *ipv6_addr = (char *)va_arg(arg, char *);

		if(host != NULL)
		{
            OSI_PRINTFI("sig_res_callback, host = %s ", host);
		}
		if(ipv4_addr != NULL)
		{
			OSI_PRINTFI("sig_res_callback, ipv4_addr = %s ", ipv4_addr);
			strcpy(ip_test, ipv4_addr);
			if(g_pdp_dns_active_sem != 0)
			{
				fibo_sem_signal(g_pdp_dns_active_sem);
			}
			
		}
		if(ipv6_addr != NULL)
		{
			OSI_PRINTFI("sig_res_callback, ipv6_addr = %s ", ipv6_addr);
		}
		else
		{
			OSI_PRINTFI("sig_res_callback, ip_addr is NULL");
		}

		va_end(arg);
	}
	break;

	//fibo_PDPActive /fibo_asyn_PDPActive  pdp active status report
	case GAPP_SIG_PDP_ACTIVE_IND:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("sig_res_callback  cid = %d", cid);
		va_end(arg);
		if(g_pdp_active_sem && test_pdp_active_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_active_sem);
			test_pdp_active_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
		if(g_pdp_asyn_active_sem && test_pdp_asyn_active_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_asyn_active_sem);
			test_pdp_asyn_active_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}
	break;

	//fibo_PDPRelease /fibo_asyn_PDPRelease pdp deactive status report
	case GAPP_SIG_PDP_RELEASE_IND:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("sig_res_callback  cid = %d", cid);
		va_end(arg);
		if(g_pdp_deactive_sem)
		{	
		    
		    fibo_sem_signal(g_pdp_deactive_sem);
			test_pdp_deactive_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
		if(g_pdp_asyn_deactive_sem && test_pdp_asyn_deactive_flag)
		{	
		    
		    fibo_sem_signal(g_pdp_asyn_deactive_sem);
			test_pdp_asyn_deactive_flag = 0;
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}
	break;

	case GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND:
	{
	    fibo_taskSleep(1000);
		OSI_PRINTFI("[%s-%d]receive  GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND", __FUNCTION__, __LINE__);
		UINT8 ip[50];	
		int ret;
		ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
		OSI_PRINTFI("[%s-%d] ret = %d,ip=%s", __FUNCTION__, __LINE__, ret,ip);
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

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
	fibo_thread_create(prvThreadEntry_2, "mythread2", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}



/*************************************END**************************************/
