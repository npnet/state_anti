
       
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
#define FIBO_AT_RES_MAX_LENGTH (1024)

#define in_range(c, lo, up) ((u8_t)c >= lo && (u8_t)c <= up)
//#define isdigit(c) in_range(c, '0', '9')
//#define isxdigit(c) (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
//#define islower(c) in_range(c, 'a', 'z')
//#define isspace(c) (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

#define HTONL(x) (x) = htonl((INT32)x)
char ip_test[60]={0x00, };
char g_send_test[4096] = {0};

typedef struct{

	int iconn;
	INT8 sid;

}lyd_socket_t;



typedef struct{
lyd_socket_t *skts;
}lyd_socket_proc_t;

typedef struct{
char * thread_name;
UINT16 port;

}thread_t;

lyd_socket_proc_t g_test;
lyd_socket_proc_t *g_test_ptr = NULL;

thread_t skt_thread_name[]=
{
	{"thread_1",30001},
	{"thread_2",30002},
	{"thread_3",30003},
	{"thread_4",30004},
	{"thread_5",30005},
	{"thread_6",30006},
	{"thread_7",30007},
	{"thread_8",30008},
	{"thread_9",30009},
	NULL,
};

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
	}
	break;

	//fibo_PDPRelease /fibo_asyn_PDPRelease pdp deactive status report
	case GAPP_SIG_PDP_RELEASE_IND:
	{
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("sig_res_callback  cid = %d", cid);
		va_end(arg);
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

static void at_res_callback(UINT8 *buf, UINT16 len)
{
    OSI_PRINTFI("FIBO <--%s", buf);
}



static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback,
	.at_resp = at_res_callback};

unsigned long htonl(unsigned long n)
{
    return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
}

unsigned short htons(unsigned short n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

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



static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}



INT32 lyd_socket_conn(UINT16 port,int32_t *conn)
{
	int i = 0;
	i = (int)port - (int)skt_thread_name[0].port;

	lyd_socket_t *skt ;
	skt = &(g_test_ptr->skts[i]);

   INT32 slRet = 0;
   int NODELAY;
   INT32 Ret = -1;


   slRet = fibo_sock_create(GAPP_IPPROTO_TCP);
   
   skt->iconn = 0;

   if (slRet < 0)
   {
      return -1;
   }
   else
   {      
      skt->sid = slRet;      
      
      GAPP_TCPIP_ADDR_T remote;
      
      OSI_PRINTFI("lyd socket , port=%d", port);
	  NODELAY = 1;
	  fibo_sock_setOpt(skt->sid,IPPROTO_TCP,TCP_NODELAY,&NODELAY,sizeof(NODELAY));

      remote.sin_port = htons(port);
      ip4addr_aton("111.231.250.105", &remote.sin_addr.u_addr.ip4);
      remote.sin_addr.u_addr.ip4.addr = htonl(remote.sin_addr.u_addr.ip4.addr);
      remote.sin_addr.type = 0;

      Ret = fibo_sock_connect(slRet, &remote); 
      
      skt->iconn = Ret >= 0;

	  *conn  =  Ret;

      OSI_PRINTFI("lyd socket connect ret = %d", Ret);

      if (skt->iconn)
      {

		{
			OSI_PRINTFI("[%s-%d]connect is OK.", __FUNCTION__, __LINE__);
			
			memset(g_send_test, '1', 20); 
			INT32 ret = fibo_sock_send(skt->sid, (UINT8 *)g_send_test, 20);
			if(ret < 0){
				OSI_PRINTFI("[%s-%d]send error, errno = %d", __FUNCTION__, __LINE__, errno);
			}
			OSI_PRINTFI("[%s-%d]port = %d , send buffer = %s", __FUNCTION__, __LINE__,port, g_send_test);
			fibo_taskSleep(2000);

		}

      }
      else
      {
          fibo_sock_close(skt->sid);
      }
   }
   return Ret;
}

 void lyd_skt_thread(void* arg)
 {
	 UINT16 Port = (UINT16) arg;
	 int i = 0;
	 i = (int)Port - (int)skt_thread_name[0].port;
 
	 lyd_socket_t* skt ;
	 skt = &(g_test_ptr->skts[i]);
 
	 uint8_t recv[2048];
	 OSI_PRINTFI("[%s-%d]Port = %d,current_thread[%d] %s", __FUNCTION__, __LINE__,Port,i,skt_thread_name[i].thread_name);
 
	 while (true)
	 {
	     INT32 conn = -1;
	     lyd_socket_conn(skt_thread_name[i].port,&conn);

		  OSI_PRINTFI("[%s-%d]current_thread %s", __FUNCTION__, __LINE__,skt_thread_name[i].thread_name);
		 
		 if (conn >= 0 )
		 {
		     memset(recv,0,2048);
			 int32_t rlen = fibo_sock_recv(skt->sid, recv, sizeof(recv));
 
			 if (rlen > 0)
			 {
				 OSI_PRINTFI("[%s-%d]port = %d ,recv buffer = %s", __FUNCTION__, __LINE__,Port, recv);
				 skt->iconn = false;
				 fibo_sock_close(skt->sid);
			 }
			 fibo_taskSleep(1000);
		 }
		 else
		 {
			 fibo_taskSleep(2000);
		 }
	 }	
	 fibo_thread_delete();
 
 }

  
 
 
 void lyd_socket_init(void)
   {
   
	  //g_test = (lyd_socket_proc_t*)malloc(sizeof(lyd_socket_proc_t));
      g_test_ptr = &g_test;
	  g_test_ptr->skts = (lyd_socket_t*)malloc(sizeof(lyd_socket_t) * 4);
   
	  memset(g_test_ptr->skts, 0 , sizeof(lyd_socket_t) * 4);
   
   
	  for (INT32 k = 0; k < 4; k++)
	  {
		  fibo_thread_create((void *)lyd_skt_thread,
									   skt_thread_name[k].thread_name,
									   8192,
									   (void*)skt_thread_name[k].port,
									   OSI_PRIORITY_NORMAL
									   );  
	  }
	  
   }


static void pdp_active_thread(void *param)
{
	int test =1;	
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
	reg_info_t reg_info;

    while (test)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            test = 0;
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);
            OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        }
    }
	while(1)
	{
		fibo_taskSleep(10000);
	}
	
	fibo_thread_delete();
}


void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
	fibo_thread_create(pdp_active_thread, "mythread2", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
	lyd_socket_init();
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}



/*************************************END**************************************/
