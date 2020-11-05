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
#define isdigit(c) in_range(c, '0', '9')
#define isxdigit(c) (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c) in_range(c, 'a', 'z')
#define isspace(c) (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

#define HTONL(x) (x) = htonl((INT32)x)

unsigned long htonl(unsigned long n)
{
    return ((n & 0xff) << 24) | ((n & 0xff00) << 8) | ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24);
}

unsigned short htons(unsigned short n)
{
    return ((n & 0xff) << 8) | ((n & 0xff00) >> 8);
}

void accept_socketServer()
{

    GAPP_TCPIP_ADDR_T addr;

    memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));

    OSI_PRINTFI("[%s-%d] start server...", __FUNCTION__, __LINE__);

    addr.sin_port = htons(3811);
    addr.sin_addr.u_addr.ip4.addr = htonl(0x7F000001);

    struct timeval opttime = {15, 0};
    fibo_sock_setOpt(socketServerID, SOL_SOCKET, SO_RCVTIMEO, &opttime, sizeof(opttime));
    OSI_PRINTFI("[%s-%d] start accept...", __FUNCTION__, __LINE__);
    acceptSocketID = fibo_sock_accept(socketServerID, &addr);

    OSI_PRINTFI("[%s-%d] acceptSocketID = %d", __FUNCTION__, __LINE__, acceptSocketID);

    while (1)
    {
        fibo_taskSleep(1000);
    }
}

INT32 createSocketServer(GAPP_TCPIP_ADDR_T addr, INT32 nProtocol)
{
    INT32 result;
    // 1. create socketserver
    socketServerID = fibo_sock_create(nProtocol);

    if (socketServerID < 0)
    {
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        return -1;
    }
    // 2. bind

    result = fibo_sock_bind(socketServerID, &addr);
    if (result < 0)
    {
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        return -1;
    }
    // 3. listen

    result = fibo_sock_listen(socketServerID);
    if (result < 0)
    {
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        return -1;
    }
    OSI_PRINTFI("[%s-%d] listen success", __FUNCTION__, __LINE__);
    return socketServerID;
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

const ip_addr_t ip6_addr_any = IPADDR6_INIT(0ul, 0ul, 0ul, 0ul);

#define in_range(c, lo, up)  ((u8_t)c >= lo && (u8_t)c <= up)
#define isprint(c)           in_range(c, 0x20, 0x7f)
#define isdigit(c)           in_range(c, '0', '9')
#define isxdigit(c)          (isdigit(c) || in_range(c, 'a', 'f') || in_range(c, 'A', 'F'))
#define islower(c)           in_range(c, 'a', 'z')
#define isspace(c)           (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')
#define xchar(i)             ((i) < 10 ? '0' + (i) : 'A' + (i) - 10)

/**
 * Check whether "cp" is a valid ascii representation
 * of an IPv6 address and convert to a binary address.
 * Returns 1 if the address is valid, 0 if not.
 *
 * @param cp IPv6 address in ascii representation (e.g. "FF01::1")
 * @param addr pointer to which to save the ip address in network order
 * @return 1 if cp could be converted to addr, 0 on failure
 */
int
ip6addr_aton(const char *cp, ip6_addr_t *addr)
{
  u32_t addr_index, zero_blocks, current_block_index, current_block_value;
  const char *s;

  /* Count the number of colons, to count the number of blocks in a "::" sequence
     zero_blocks may be 1 even if there are no :: sequences */
  zero_blocks = 8;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      zero_blocks--;
    } else if (!isxdigit(*s)) {
      break;
    }
  }

  /* parse each block */
  addr_index = 0;
  current_block_index = 0;
  current_block_value = 0;
  for (s = cp; *s != 0; s++) {
    if (*s == ':') {
      if (addr) {
        if (current_block_index & 0x1) {
          addr->addr[addr_index++] |= current_block_value;
        }
        else {
          addr->addr[addr_index] = current_block_value << 16;
        }
      }
      current_block_index++;
      current_block_value = 0;
      if (current_block_index > 7) {
        /* address too long! */
        return 0;
      }
      if (s[1] == ':') {
        if (s[2] == ':') {
          /* invalid format: three successive colons */
          return 0;
        }
        s++;
        /* "::" found, set zeros */
        while (zero_blocks > 0) {
          zero_blocks--;
          if (current_block_index & 0x1) {
            addr_index++;
          } else {
            if (addr) {
              addr->addr[addr_index] = 0;
            }
          }
          current_block_index++;
          if (current_block_index > 7) {
            /* address too long! */
            return 0;
          }
        }
      }
    } else if (isxdigit(*s)) {
      /* add current digit */
      current_block_value = (current_block_value << 4) +
          (isdigit(*s) ? (u32_t)(*s - '0') :
          (u32_t)(10 + (islower(*s) ? *s - 'a' : *s - 'A')));
    } else {
      /* unexpected digit, space? CRLF? */
      break;
    }
  }

  if (addr) {
    if (current_block_index & 0x1) {
      addr->addr[addr_index++] |= current_block_value;
    }
    else {
      addr->addr[addr_index] = current_block_value << 16;
    }
  }

  /* convert to network byte order. */
  if (addr) {
    for (addr_index = 0; addr_index < 4; addr_index++) {
      addr->addr[addr_index] = lwip_htonl(addr->addr[addr_index]);
    }
  }

  if (current_block_index != 7) {
    return 0;
  }

  return 1;
}

/**
 * Convert numeric IPv6 address into ASCII representation.
 * returns ptr to static buffer; not reentrant!
 *
 * @param addr ip6 address in network order to convert
 * @return pointer to a global static (!) buffer that holds the ASCII
 *         representation of addr
 */
char *
ip6addr_ntoa(const ip6_addr_t *addr)
{
  static char str[40];
  return ip6addr_ntoa_r(addr, str, 40);
}

/**
 * Same as ipaddr_ntoa, but reentrant since a user-supplied buffer is used.
 *
 * @param addr ip6 address in network order to convert
 * @param buf target buffer where the string is stored
 * @param buflen length of buf
 * @return either pointer to buf which now holds the ASCII
 *         representation of addr or NULL if buf was too small
 */
char *
ip6addr_ntoa_r(const ip6_addr_t *addr, char *buf, int buflen)
{
  u32_t current_block_index, current_block_value, next_block_value;
  s32_t i;
  u8_t zero_flag, empty_block_flag;

  i = 0;
  empty_block_flag = 0; /* used to indicate a zero chain for "::' */

  for (current_block_index = 0; current_block_index < 8; current_block_index++) {
    /* get the current 16-bit block */
    current_block_value = lwip_htonl(addr->addr[current_block_index >> 1]);
    if ((current_block_index & 0x1) == 0) {
      current_block_value = current_block_value >> 16;
    }
    current_block_value &= 0xffff;

    /* Check for empty block. */
    if (current_block_value == 0) {
      if (current_block_index == 7 && empty_block_flag == 1) {
        /* special case, we must render a ':' for the last block. */
        buf[i++] = ':';
        if (i >= buflen) {
          return NULL;
        }
        break;
      }
      if (empty_block_flag == 0) {
        /* generate empty block "::", but only if more than one contiguous zero block,
         * according to current formatting suggestions RFC 5952. */
        next_block_value = lwip_htonl(addr->addr[(current_block_index + 1) >> 1]);
        if ((current_block_index & 0x1) == 0x01) {
            next_block_value = next_block_value >> 16;
        }
        next_block_value &= 0xffff;
        if (next_block_value == 0) {
          empty_block_flag = 1;
          buf[i++] = ':';
          if (i >= buflen) {
            return NULL;
          }
          continue; /* move on to next block. */
        }
      } else if (empty_block_flag == 1) {
        /* move on to next block. */
        continue;
      }
    } else if (empty_block_flag == 1) {
      /* Set this flag value so we don't produce multiple empty blocks. */
      empty_block_flag = 2;
    }

    if (current_block_index > 0) {
      buf[i++] = ':';
      if (i >= buflen) {
        return NULL;
      }
    }

    if ((current_block_value & 0xf000) == 0) {
      zero_flag = 1;
    } else {
      buf[i++] = xchar(((current_block_value & 0xf000) >> 12));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf00) == 0) && (zero_flag)) {
      /* do nothing */
    } else {
      buf[i++] = xchar(((current_block_value & 0xf00) >> 8));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    if (((current_block_value & 0xf0) == 0) && (zero_flag)) {
      /* do nothing */
    }
    else {
      buf[i++] = xchar(((current_block_value & 0xf0) >> 4));
      zero_flag = 0;
      if (i >= buflen) {
        return NULL;
      }
    }

    buf[i++] = xchar((current_block_value & 0xf));
    if (i >= buflen) {
      return NULL;
    }
  }

  buf[i] = 0;

  return buf;
}


static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
    case GAPP_SIG_OPENCPU_API_TEST:
    
    break;

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

	//GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND
	case GAPP_SIG_PDP_ACTIVE_OR_DEACTIVE_FAIL_IND:
	{
	
		UINT8 cid = (UINT8)va_arg(arg, int);
		UINT8 state = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("[%s-%d]cid = %d,state = %d", __FUNCTION__, __LINE__,cid,state);
		va_end(arg);
	}

	//PDP in active state, deactive indicator received from modem  
	case GAPP_SIG_PDP_DEACTIVE_ABNORMALLY_IND:
	{
	
		UINT8 cid = (UINT8)va_arg(arg, int);
		OSI_PRINTFI("[%s-%d]cid = %d", __FUNCTION__, __LINE__,cid);
		va_end(arg);
	}
		
    default:
    {
        break;
    }
    }
    OSI_LOGI(0, "test");
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_res_callback};

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    int ret = 0;
    int test =1;
	OSI_PRINTFI("[%s-%d] osiThreadCurrent() = %p", __FUNCTION__, __LINE__,osiThreadCurrent());	
	reg_info_t reg_info;
	
	UINT8 ip[50];
	UINT8 cid_status;
	INT8 cid =1;
	while(test)
	{
		fibo_getRegInfo(&reg_info, 0);
		fibo_taskSleep(1000);
		if(reg_info.nStatus==1)
		{
		    memset(&ip, 0, sizeof(ip));
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
			test = 0;
			fibo_taskSleep(1000);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}

	fibo_taskSleep(1000);
    memset(&ip, 0, sizeof(ip));
    if (0 == fibo_PDPStatus(cid, ip,&cid_status, 0))
    {
        OSI_PRINTFI("[%s-%d] ip = %s,cid_status=%d", __FUNCTION__, __LINE__, ip,cid_status);
    }
	
	OSI_PRINTFI("[%s-%d] ip = %s,cid_status=%d,cid=%d", __FUNCTION__, __LINE__,ip,cid_status,cid);


	fibo_taskSleep(5000);
#if 1	
	{
        GAPP_TCPIP_ADDR_T addr;
        INT8 socketid;
        int rcvnum = 0;
        char rcvbuf[128];
        INT32 retcode, ret;
		int opt = 1;
		int *tmp_opt;
		int len = sizeof(int);


        memset(rcvbuf, 0, sizeof(rcvbuf));
        memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
        OSI_PRINTFI("[%s-%d]sys_sock_create start :\n", __FUNCTION__, __LINE__);		
		socketid = fibo_sock_create_ex(AF_INET,SOCK_STREAM,IPPROTO_IP);
        OSI_PRINTFI("[%s-%d]sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, socketid);

        addr.sin_port = htons(3410);
		ret = fibo_sock_setOpt(socketid, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(int));
		OSI_PRINTFI("[%s-%d]setopt  ret = %d\n", __FUNCTION__, __LINE__, ret);

        ip4addr_aton("47.110.234.36", &addr.sin_addr.u_addr.ip4);
        addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
        //ip4addr_aton("105.250.231.111", &addr.sin_addr.u_addr.ip4);
        retcode = fibo_sock_connect(socketid, &addr);
        OSI_PRINTFI("[%s-%d] sys_sock_connect addr.sin_addr.addr = %u\n", __FUNCTION__, __LINE__, addr.sin_addr.u_addr.ip4.addr);
        OSI_PRINTFI("[%s-%d] sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, retcode);

        ret = fibo_sock_send(socketid, (UINT8 *)"demo opencpu test tcp ok", strlen("demo opencpu test tcp ok"));
        OSI_PRINTFI("[%s-%d] sys_sock_send  retcode = %d\n", __FUNCTION__, __LINE__, ret);
		
        fibo_taskSleep((UINT32)1000);
		tmp_opt = fibo_malloc(sizeof(int));
		ret = fibo_sock_getOpt(socketid, SOL_SOCKET, SO_KEEPALIVE, tmp_opt, &len);
		OSI_PRINTFI("[%s-%d] fibo_sock_getOpt  ret = %d,tmp_opt = %d", __FUNCTION__, __LINE__, ret,*tmp_opt);
		fibo_free(tmp_opt);
       
        rcvnum = fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 64);
        OSI_PRINTFI("[%s-%d] sys_sock_recv  rcvnum = %d, rcvbuf = %s\n", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
    }
	
	fibo_taskSleep(5000);
	{
		GAPP_TCPIP_ADDR_T addr;
		INT8 socketid;
		int rcvnum = 0;
		char rcvbuf[128];
		char *sendbuf = "opencpu test udp ok";
		INT32 ret;

		memset(rcvbuf, 0, sizeof(rcvbuf));
		memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
		OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		socketid = fibo_sock_create_ex(AF_INET,SOCK_DGRAM,IPPROTO_IP);
		OSI_PRINTFI("[%s-%d] socketid = %d", __FUNCTION__, __LINE__, socketid);

		addr.sin_port = htons(5105);

		ip4addr_aton("47.110.234.36", &addr.sin_addr.u_addr.ip4);
		addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
		ret = fibo_sock_connect(socketid, &addr);
		OSI_PRINTFI("[%s-%d] addr.sin_addr.addr = %", __FUNCTION__, __LINE__, addr.sin_addr.u_addr.ip4.addr);
		

		ret = fibo_sock_send2(socketid, (UINT8 *)sendbuf, strlen(sendbuf), &addr);

		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);

		rcvnum = fibo_sock_recvFrom(socketid, (UINT8 *)rcvbuf, 64, &addr);
		
		OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
		OSI_PRINTFI("[%s-%d] success", __FUNCTION__, __LINE__);
	}
#endif

	
	fibo_taskSleep(1000);
	

	{
			GAPP_TCPIP_ADDR_T addr;
			INT8 socketid;
			int rcvnum = 0;
			char rcvbuf[128];
            INT32 retcode, ret;
			char *sendbuf = "opencpu test udp ok";
			memset(rcvbuf, 0, sizeof(rcvbuf));
			memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
			socketid = fibo_sock_create_ex(AF_INET6,SOCK_STREAM,IPPROTO_IP);
	        OSI_PRINTFI("[%s-%d]sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, socketid);

	        addr.sin_port = htons(3120);

	        ip6addr_aton("2400:3200:1600::202", &addr.sin_addr.u_addr.ip6);
			addr.sin_addr.type = AF_INET6;
	        retcode = fibo_sock_connect(socketid, &addr);
	        OSI_PRINTFI("[%s-%d] sys_sock_connect  retcode = %d\n", __FUNCTION__, __LINE__, retcode);

	        ret = fibo_sock_send(socketid, (UINT8 *)"demo opencpu test tcp ok", strlen("demo opencpu test tcp ok"));
	        OSI_PRINTFI("[%s-%d] sys_sock_send  retcode = %d\n", __FUNCTION__, __LINE__, ret);			
	        fibo_taskSleep((UINT32)1000);
	        rcvnum = fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 64);			
			OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
	}

	{
			GAPP_TCPIP_ADDR_T addr;
			INT8 socketid;
			int rcvnum = 0;
			char rcvbuf[128];
            INT32 retcode, ret;
			char *sendbuf = "opencpu test udp ok";
			memset(rcvbuf, 0, sizeof(rcvbuf));
			memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
			socketid = fibo_sock_create_ex(AF_INET6,SOCK_DGRAM,IPPROTO_IP);
	        OSI_PRINTFI("[%s-%d]sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, socketid);

	        addr.sin_port = htons(5105);

	        ip6addr_aton("2400:3200:1600::202", &addr.sin_addr.u_addr.ip6);
			addr.sin_addr.type = AF_INET6;
	        retcode = fibo_sock_connect(socketid, &addr);
	        OSI_PRINTFI("[%s-%d] sys_sock_connect  retcode = %d\n", __FUNCTION__, __LINE__, retcode);

	        ret = fibo_sock_send(socketid, (UINT8 *)"demo opencpu test udp ok", strlen("demo opencpu test UDP ok"));
	        OSI_PRINTFI("[%s-%d] sys_sock_send  retcode = %d\n", __FUNCTION__, __LINE__, ret);			
	        fibo_taskSleep((UINT32)1000);
	        rcvnum = fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 64);			
			OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
	}

	{
			GAPP_TCPIP_ADDR_T addr;
			INT8 socketid;
			int rcvnum = 0;
			char rcvbuf[128];
            INT32 retcode, ret;
			char *sendbuf = "opencpu test udp ok";
			memset(rcvbuf, 0, sizeof(rcvbuf));
			memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
			socketid = fibo_sock_create_ex(AF_INET6,SOCK_DGRAM,IPPROTO_IP);
	        OSI_PRINTFI("[%s-%d]sys_sock_create  retcode = %d\n", __FUNCTION__, __LINE__, socketid);

	        addr.sin_port = htons(5106);

	        ip6addr_aton("2400:3200:1600::202", &addr.sin_addr.u_addr.ip6);
			addr.sin_addr.type = AF_INET6;
	        //retcode = fibo_sock_connect(socketid, &addr);
	        OSI_PRINTFI("[%s-%d] sys_sock_connect  retcode = %d\n", __FUNCTION__, __LINE__, retcode);

	        ret = fibo_sock_send2(socketid, (UINT8 *)"demo opencpu test udp ok", strlen("demo opencpu test UDP ok"),&addr);
	        OSI_PRINTFI("[%s-%d] sys_sock_send  retcode = %d\n", __FUNCTION__, __LINE__, ret);			
	        fibo_taskSleep((UINT32)1000);
	        rcvnum = fibo_sock_recvFrom(socketid, (UINT8 *)rcvbuf, 64,&addr);			
			OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
	}

       


	while(1)
	{
		fibo_taskSleep(5000);
		OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
	}
	fibo_thread_delete();

}


void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    prvInvokeGlobalCtors();
    fibo_thread_create(prvThreadEntry, "mythread", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
