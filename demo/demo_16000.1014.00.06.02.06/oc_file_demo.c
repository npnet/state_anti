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


#define OC_TEST_FILE_DIR_PATH  "/file_test_dir"
#define OC_TEST_FILE_PATH  OC_TEST_FILE_DIR_PATH "/file_test.nv"


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
static int	test_fd = -1;
uint8_t result_buf[128] = "12345678901234567890abcdefghijklmnopqrstuvwxyz";
uint8_t test_buf[128] = {0};

int test_demo_handle(INT8 case_number)
{

    OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
    switch (case_number)
    {
    #if 0
    case 1:
    {
        UINT8 ip[50];
        memset(&ip, 0, sizeof(ip));
        if (fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip))
        {
            OSI_PRINTFI("[%s-%d] ip = %s", __FUNCTION__, __LINE__, ip);
        }
    }

    break;

    case 2:
    {
        pdp_profile_t pdp_profile;
        char *pdp_type = "ipv4v6";
        char *apn = "test";
        memset(&pdp_profile, 0, sizeof(pdp_profile));

        pdp_profile.cid = 2;
        //pdp_profile.nPdpType = (UINT8 *)&pdp_type;
        //pdp_profile.apn = (UINT8 *)&apn;
        memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
        memcpy(pdp_profile.apn, apn, strlen((char *)apn));
        fibo_asyn_PDPActive(1, &pdp_profile, 0);
    }
    break;

    case 3:
        fibo_PDPRelease(0, 0);
        break;

    case 4:
        fibo_asyn_PDPRelease(0, 2, 0);
        break;

    case 5:
    {
        UINT8 ip[50];
        memset(&ip, 0, sizeof(ip));
        if (0 == fibo_PDPStatus(1, ip, 0))
        {
            OSI_PRINTFI("[%s-%d] ip = %s", __FUNCTION__, __LINE__, ip);
        }
    }
    break;

    case 6:
    {
        INT32 sock_id;
        sock_id = fibo_sock_create(0);
        if (sock_id >= 0)
        {
            OSI_PRINTFI("[%s-%d] sock_id = %d", __FUNCTION__, __LINE__, sock_id);
        }
    }
    break;

    case 7:
    {
        INT32 sock_id;
        sock_id = fibo_sock_create(0);
        if (sock_id >= 0)
        {
            OSI_PRINTFI("[%s-%d] sock_id = %d", __FUNCTION__, __LINE__, sock_id);
        }
        fibo_sock_close(sock_id);
    }
    break;

    case 8:
    {
        INT32 sock_id;
        sock_id = fibo_sock_create(0);
        if (sock_id >= 0)
        {
            OSI_PRINTFI("[%s-%d] sock_id = %d", __FUNCTION__, __LINE__, sock_id);
        }
        fibo_sock_listen(sock_id);
    }

    break;

    case 9:
    {
        GAPP_TCPIP_ADDR_T addr;
        INT8 socketid;
        int rcvnum = 0;
        char rcvbuf[128];
        struct timeval timeout = {10, 0};
        struct timeval tmp;
        INT32 len;
        INT32 retcode, ret;

        memset(rcvbuf, 0, sizeof(rcvbuf));
        memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
        OSI_PRINTFI("demo case 9 sys_sock_create start :\n");
        socketid = fibo_sock_create(GAPP_IPPROTO_TCP);
        OSI_PRINTFI("demo case 9 sys_sock_create  retcode = %d\n", socketid);

        addr.sin_port = htons(35220);

        ip4addr_aton("111.231.250.105", &addr.sin_addr.u_addr.ip4);
        addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
        //ip4addr_aton("105.250.231.111", &addr.sin_addr.u_addr.ip4);

        //ret = fibo_sock_bind(socketid, &addr);
        //addr.sin_addr.addr = inet_addr("220.181.111.188");
        retcode = fibo_sock_connect(socketid, &addr);
        OSI_PRINTFI("demo case 9 sys_sock_connect addr.sin_addr.addr = %u\n", addr.sin_addr.u_addr.ip4.addr);

        OSI_PRINTFI("demo case 9 sys_sock_connect addr.sin_addr.addr = %u\n", addr.sin_addr.u_addr.ip4.addr);
        OSI_PRINTFI("demo case 9 sys_sock_create  retcode = %d\n", retcode);

        ret = fibo_sock_send(socketid, (UINT8 *)"demo opencpu test tcp ok", strlen("demo opencpu test tcp ok"));
        OSI_PRINTFI("demo case 9 sys_sock_send  retcode = %d\n", ret);
        ret = fibo_sock_setOpt(socketid, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
        OSI_PRINTFI("demo case 9 sys_sock_setOpt  ret = %d\n", ret);

        fibo_taskSleep((UINT32)1000);
        len = sizeof(tmp);
        ret = fibo_sock_getOpt(socketid, SOL_SOCKET, SO_RCVTIMEO, &tmp, &len);
        OSI_PRINTFI("demo case 9 sys_sock_getOpt tmp.sec =%d, tmp.usec =%d,  len = %d, ret = %d\n", tmp.tv_sec, tmp.tv_usec, len, ret);

        rcvnum = fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 64);
        OSI_PRINTFI("demo case 9 sys_sock_recv  rcvnum = %d, rcvbuf = %s\n", rcvnum, rcvbuf);
    }

    break;
    case 10:
    {
        GAPP_TCPIP_ADDR_T addr;
        INT8 socketid;
        int rcvnum = 0;
        char rcvbuf[128];
        char *sendbuf = "opencpu test udp ok";
        INT32 ret;

        memset(rcvbuf, 0, sizeof(rcvbuf));
        memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
        OSI_PRINTFI("demo case 10 sys_sock_create start :\n");
        socketid = fibo_sock_create(GAPP_IPPROTO_UDP);
        OSI_PRINTFI("demo case 10 sys_sock_create  retcode = %d\n", socketid);

        addr.sin_port = htons(35221);

        ip4addr_aton("111.231.250.105", &addr.sin_addr.u_addr.ip4);
        addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
        ret = fibo_sock_connect(socketid, &addr);
        OSI_PRINTFI("demo case 10 sys_sock_connect addr.sin_addr.addr = %u\n", addr.sin_addr.u_addr.ip4.addr);

        ret = fibo_sock_send2(socketid, (UINT8 *)sendbuf, strlen(sendbuf), &addr);

        OSI_PRINTFI("demo case 10 sys_sock_send2  retcode = %d\n", ret);

        rcvnum = fibo_sock_recvFrom(socketid, (UINT8 *)rcvbuf, 64, &addr);
        OSI_PRINTFI("demo case 10 fibo_sys_sock_recvfrom rcvnum = %d, rcvbuf = %s\n", rcvnum, rcvbuf);
        OSI_PRINTFI("demo case 10 sys_sock_create end:\n");
    }

    break;
    case 11:
    {
        GAPP_TCPIP_ADDR_T addr;
        char rcvbuf[128];
        //char *sendbuf = "opencpu test tcp ok";
        INT32 clientSocket, sSocket;
        INT32 result;
        memset(rcvbuf, 0, sizeof(rcvbuf));
        memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));

        OSI_PRINTFI("demo case 11 sys_sock_create start :\n");

        addr.sin_port = htons(35220);
        addr.sin_addr.u_addr.ip4.addr = htonl(0x7F000001);

        socketServerID = createSocketServer(addr, GAPP_IPPROTO_TCP);
        //socketid = fibo_sock_create(GAPP_IPPROTO_TCP);
        OSI_PRINTFI("demo case 11 sys_sock_create  retcode = %d\n", socketServerID);

        fibo_thread_create(accept_socketServer, "accept_socketServer", 1024, NULL, OSI_PRIORITY_NORMAL);
        fibo_taskSleep(500);
        // 3. create client socket
        clientSocket = fibo_sock_create(GAPP_IPPROTO_TCP);
        OSI_PRINTFI("demo case 11 sys_sock_create  clientSocket = %d\n", clientSocket);
        // 4. connect
        sSocket = fibo_sock_connect(clientSocket, &addr);
        if (sSocket < 0)
        {
            // need close client socket
            fibo_sock_close(clientSocket);
            fibo_sock_close(socketServerID);
            // delete thred
            fibo_thread_delete();
        }
        OSI_PRINTFI("demo case 11 sys_sock_create  sSocket = %d\n", sSocket);

        // 5. accept
        OSI_PRINTFI("demo case 11 sys_sock_create  acceptSocketID = %d\n", acceptSocketID);
        // sleep 500, no sleep bind again will fail

        // 6. close
        fibo_taskSleep(500);
        result = fibo_sock_close(clientSocket);
        OSI_PRINTFI("demo case 11 sys_sock_create  result = %d\n", result);
        //	sys_taskSleep(500);
        result = fibo_sock_close(acceptSocketID);

        OSI_PRINTFI("demo case 11 sys_sock_create  result = %d\n", result);
        //	sys_taskSleep(500);
        result = fibo_sock_close(socketServerID);

        OSI_PRINTFI("demo case 11 sys_sock_create  result = %d\n", result);

        OSI_PRINTFI("demo case 11 sys_sock_create end:\n");
    }

    break;
    case 12: //get cell info
    	{
    	    int i;
			opencpu_cellinfo_t opencpu_cellinfo;
			memset(&opencpu_cellinfo,0,sizeof(opencpu_cellinfo));
			OSI_PRINTFI("demo case 12");
			fibo_getCellInfo(&opencpu_cellinfo,0);
			if(&opencpu_cellinfo.lte_cellinfo != NULL)
			{
				OSI_PRINTFI("demo case 12 opencpu_cellinfo.lte_cellinfo.lteScell.dlEarfcn = %d\n",opencpu_cellinfo.lte_cellinfo.lteScell.dlEarfcn);
				if(opencpu_cellinfo.lte_cellinfo.lteNcellNum > 0)
				{
					 OSI_PRINTFI("demo case 12 opencpu_cellinfo.lte_cellinfo.lteNcellNum = %d\n",opencpu_cellinfo.lte_cellinfo.lteNcellNum);
				}
			}
			
			if(&opencpu_cellinfo.gsm_cellinfo.pCurrCellInfo != NULL)
			{
			    for(i=0;i<5;i++)
			    {
					OSI_PRINTFI("demo case 12 opencpu_cellinfo.gsm_cellinfo.pCurrCellInfo.nTSM_LAI[%d]=%x\n",i,opencpu_cellinfo.gsm_cellinfo.pCurrCellInfo.nTSM_LAI[i]);
				}
				
			}
			if(&opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo != NULL && opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo.nTSM_NebCellNUM > 0)
			{
				for(i=0;i<5;i++)
			    {
					OSI_PRINTFI("demo case 12 opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell.nTSM_LAI[%d]=%x\n",i,opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[0].nTSM_LAI[i]);
				}
			}
	    }
        break;
    case 13:
    	{
			reg_info_t reg_info;
			memset(&reg_info,0,sizeof(reg_info));
			OSI_PRINTFI("demo case 13");
			int ret =fibo_getRegInfo(&reg_info,0);
		    OSI_PRINTFI("demo case 13, ret = %d",ret);
			OSI_PRINTFI("demo case 13,reg_state = %d, curr_rat=%d",reg_info.nStatus ,reg_info.curr_rat);
			if(reg_info.nStatus == 1 || reg_info.nStatus ==5)
			{
				if(reg_info.curr_rat == 4 || reg_info.curr_rat ==7)
				{
					OSI_PRINTFI("demo case 13,LTE tac = %d ,cell id = %d",reg_info.lte_scell_info.tac,reg_info.lte_scell_info.cell_id);
				}
				else
				{
					OSI_PRINTFI("demo case 13,gsm lac = %d,cell id = %d",reg_info.gsm_scell_info.lac,reg_info.gsm_scell_info.cell_id);
				}
			}
			
	    }
        break;
    case 14:
    	{
    	   int ret;
		   UINT8 ip[50];
		   OSI_PRINTFI("demo case 14");
           memset(&ip, 0, sizeof(ip));
		   ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
		   fibo_taskSleep(2000);
		   ret = fibo_getHostByName("www.baidu.com",1,0);
		   OSI_PRINTFI("demo case 14 ret = %d",ret);
		   fibo_taskSleep(2000);
		   
	    }
        break;
		#endif

    case 15:
		{
			uint32_t ret_size1 = 0; 
			uint32_t ret_size2 = 0;
			uint32_t ret_size3 = 0;		
			int ret = -1;
			ret = fibo_get_heapinfo(&ret_size1,&ret_size2,&ret_size3);
			OSI_LOGI(0, "the heapinfo is: total size:%ld",ret_size1);
			OSI_LOGI(0, "the heapinfo is: avalid size:%ld",ret_size2);
			OSI_LOGI(0, "the heapinfo is: block size:%ld",ret_size3);
		}
		break;
    case 16:
		{
			int ret = -1;
			ret = fibo_file_mkdir(OC_TEST_FILE_DIR_PATH);
			if(ret == -1)
			{
				OSI_LOGI(0,"test mkdir failed");
				return ;
			}
		}
        break;
    case 17:
		{
			test_fd = fibo_file_open(OC_TEST_FILE_PATH,FS_O_WRONLY | FS_O_CREAT | FS_O_TRUNC);
			if(test_fd == -1)
			{
				OSI_LOGI(0,"test create file failed");
				return ;
			}
		}
        break;
    case 18:
		{
			int ret = -1;
			OSI_LOGI(0,"test file fd is %d" ,test_fd);
			ret = fibo_file_write(test_fd,result_buf,strlen(result_buf));
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_write failed");
				return ;
			}
			OSI_PRINTFI("test fibo_file_write is %s",result_buf);

		}
        break;
    case 19:
		{
			INT32 file_offset = 0;
			int ret = -1;
			file_offset = fibo_file_seek(test_fd,20,FS_SEEK_SET);
			if(file_offset == -1)
			{
				OSI_LOGI(0,"test fibo_file_seek failed");
				return ;
			}
			memset(test_buf,0,sizeof(test_buf));
			ret = fibo_file_read(test_fd,test_buf,strlen(result_buf)-20);
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_write failed");
				return ;
			}
			OSI_PRINTFI("test fibo_file_read is %s",test_buf);
		}
        break;
    case 20:
		{
			int ret = -1;
			ret = fibo_file_rename("/file_test_dir/file_test.nv","/file_test_dir/file_test_oc.bin");
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_rename failed");
				return ;
			}			
		}
        break;
    case 21:
		{
			int ret = -1;
			ret = fibo_file_getSize("/file_test_dir/file_test_oc.bin");
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_getSize failed");
				return ;
			}	
			
			OSI_LOGI(0,"test fibo_file_getSize is %d",ret);
		}
        break;
	case 22:
		{
			int ret =-1;
			ret = fibo_file_close(test_fd);
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_close failed");
				return ;
			}
		}
        break;	
    case 23:
		{
			
			test_fd = fibo_file_exist("/file_test_dir/file_test_oc.bin");
			if(test_fd == -1)
			{
				OSI_LOGI(0,"test fibo_file_exist not exist");
				return ;
			}
			
		}
        break;
    
    case 24:
		{
			int ret =- 1;
			ret = fibo_file_delete("/file_test_dir/file_test_oc.bin");
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_delete failed");
				return ;
			}
		}
        break;
    case 25:
		{
			int ret = -1;
			ret = fibo_file_rmdir("/file_test_dir");
			if(ret == -1)
			{
				OSI_LOGI(0,"test fibo_file_close failed");
				return ;
			}
		}
        break;
    case 26:
        break;
    case 27:
        break;
    case 28:
        break;
    default:
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        return 0;
        break;
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

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    int ret = 0;
    fibo_taskSleep(3000);
	OSI_PRINTFI("[%s-%d] osiThreadCurrent() = %p", __FUNCTION__, __LINE__,osiThreadCurrent());
	//for(int n=10;n>0;n--)
	#if 0
	{
	
		UINT8 ip[50];
		memset(&ip, 0, sizeof(ip));
		ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
	
		OSI_PRINTFI("[%s-%d] ret = %d,ip=%s", __FUNCTION__, __LINE__, ret,ip);
	
		fibo_taskSleep(5000);
	
		ret = fibo_PDPRelease(0, 0);
		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
		fibo_taskSleep(5000);
	
		{
			pdp_profile_t pdp_profile;
			char *pdp_type = "ipv4v6";
			char *apn = "test";
			memset(&pdp_profile, 0, sizeof(pdp_profile));
	
			pdp_profile.cid = 2;
			//pdp_profile.nPdpType = (UINT8 *)&pdp_type;
			//pdp_profile.apn = (UINT8 *)&apn;
			memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
			memcpy(pdp_profile.apn, apn, strlen((char *)apn));
			ret = fibo_asyn_PDPActive(1, &pdp_profile, 0);
	
		}
	
		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
		fibo_taskSleep(5000);
		
		ret=fibo_asyn_PDPRelease(0, 2, 0);
	
		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
		fibo_taskSleep(5000);
	
		ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
		fibo_taskSleep(5000);
	
		{
			GAPP_TCPIP_ADDR_T addr;
			INT8 socketid;
			int rcvnum = 0;
			char rcvbuf[128];
			struct timeval timeout = {10, 0};
			struct timeval tmp;
			INT32 len;
			INT32 retcode, ret;
	
			memset(rcvbuf, 0, sizeof(rcvbuf));
			memset(&addr, 0, sizeof(GAPP_TCPIP_ADDR_T));
			OSI_PRINTFI("demo case 9 sys_sock_create start :\n");
			socketid = fibo_sock_create(GAPP_IPPROTO_TCP);
			OSI_PRINTFI("[%s-%d] retcode = %d", __FUNCTION__, __LINE__, socketid);
	
			addr.sin_port = htons(35220);
	
			ip4addr_aton("111.231.250.105", &addr.sin_addr.u_addr.ip4);
			addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
			//ip4addr_aton("105.250.231.111", &addr.sin_addr.u_addr.ip4);
	
			//ret = fibo_sock_bind(socketid, &addr);
			//addr.sin_addr.addr = inet_addr("220.181.111.188");
			retcode = fibo_sock_connect(socketid, &addr);
	
			OSI_PRINTFI("[%s-%d] addr.sin_addr.u_addr.ip4.addr = %d , retcode = %d", __FUNCTION__, __LINE__, addr.sin_addr.u_addr.ip4.addr,retcode);
	
			ret = fibo_sock_send(socketid, (UINT8 *)"demo opencpu test tcp ok", strlen("demo opencpu test tcp ok"));
			OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
			ret = fibo_sock_setOpt(socketid, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
			OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
			fibo_taskSleep((UINT32)1000);
			len = sizeof(tmp);
			ret = fibo_sock_getOpt(socketid, SOL_SOCKET, SO_RCVTIMEO, &tmp, &len);
			OSI_PRINTFI("[%s-%d] tmp.sec =%d, tmp.usec =%d,  len = %d, ret = %d", __FUNCTION__, __LINE__,  tmp.tv_sec, tmp.tv_usec, len, ret);
	
			rcvnum = fibo_sock_recv(socketid, (UINT8 *)rcvbuf, 64);
			OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
	
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
			socketid = fibo_sock_create(GAPP_IPPROTO_UDP);
			OSI_PRINTFI("[%s-%d] socketid = %d", __FUNCTION__, __LINE__, socketid);
	
			addr.sin_port = htons(35221);
	
			ip4addr_aton("111.231.250.105", &addr.sin_addr.u_addr.ip4);
			addr.sin_addr.u_addr.ip4.addr = htonl(addr.sin_addr.u_addr.ip4.addr);
			ret = fibo_sock_connect(socketid, &addr);
			OSI_PRINTFI("[%s-%d] addr.sin_addr.addr = %", __FUNCTION__, __LINE__, addr.sin_addr.u_addr.ip4.addr);
			
	
			ret = fibo_sock_send2(socketid, (UINT8 *)sendbuf, strlen(sendbuf), &addr);
	
			OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
			rcvnum = fibo_sock_recvFrom(socketid, (UINT8 *)rcvbuf, 64, &addr);
			
			OSI_PRINTFI("[%s-%d] rcvnum = %d, rcvbuf = %s", __FUNCTION__, __LINE__, rcvnum, rcvbuf);
			OSI_PRINTFI("[%s-%d] success", __FUNCTION__, __LINE__);
		}
		fibo_taskSleep(5000);
		ret = fibo_PDPRelease(0, 0);
		OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__, ret);
	
		fibo_taskSleep(5000);

	}


	//test get cell info

	{
	    int i;
		opencpu_cellinfo_t opencpu_cellinfo;
		memset(&opencpu_cellinfo,0,sizeof(opencpu_cellinfo));
		OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
		fibo_getCellInfo(&opencpu_cellinfo,0);
		if(opencpu_cellinfo.curr_rat == 4 || opencpu_cellinfo.curr_rat == 7)
		{
			if(&opencpu_cellinfo.lte_cellinfo != NULL)
			{
				OSI_PRINTFI("[%s-%d] opencpu_cellinfo.lte_cellinfo.lteScell.dlEarfcn = %d", __FUNCTION__, __LINE__,opencpu_cellinfo.lte_cellinfo.lteScell.dlEarfcn);
				
				if(opencpu_cellinfo.lte_cellinfo.lteNcellNum > 0)
				{
					 OSI_PRINTFI("[%s-%d] opencpu_cellinfo.lte_cellinfo.lteNcellNum = %d", __FUNCTION__, __LINE__,opencpu_cellinfo.lte_cellinfo.lteNcellNum);	 
				}
			}		

		}

		if(opencpu_cellinfo.curr_rat == 2 || opencpu_cellinfo.curr_rat == 0)
		{
			if(&opencpu_cellinfo.gsm_cellinfo.pCurrCellInfo != NULL)
			{
				for(i=0;i<5;i++)
				{
					OSI_PRINTFI("[%s-%d] opencpu_cellinfo.gsm_cellinfo.pCurrCellInfo.nTSM_LAI[%d]=%x", __FUNCTION__, __LINE__,i,opencpu_cellinfo.gsm_cellinfo.pCurrCellInfo.nTSM_LAI[i]);				 
				}
				
			}
			if(&opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo != NULL && opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo.nTSM_NebCellNUM > 0)
			{
				for(i=0;i<5;i++)
				{
					OSI_PRINTFI("[%s-%d] gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell.nTSM_LAI[%d]=%x", __FUNCTION__, __LINE__,i,opencpu_cellinfo.gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[0].nTSM_LAI[i]);	
				}
			}

		}
	}
	fibo_taskSleep(5000);
	{
		reg_info_t reg_info;
		memset(&reg_info,0,sizeof(reg_info));
		OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
		ret = fibo_getRegInfo(&reg_info,0);
		OSI_PRINTFI("[%s-%d] ret=%d", __FUNCTION__, __LINE__,ret);
		OSI_PRINTFI("[%s-%d] reg_state = %d, curr_rat=%d", __FUNCTION__, __LINE__,reg_info.nStatus ,reg_info.curr_rat);
		if(reg_info.nStatus == 1 || reg_info.nStatus ==5)
		{
			if(reg_info.curr_rat == 4 || reg_info.curr_rat ==7)
			{
				OSI_PRINTFI("[%s-%d] LTE tac = %d ,cell id = %d",__FUNCTION__, __LINE__,reg_info.lte_scell_info.tac,reg_info.lte_scell_info.cell_id);
			}
			else
			{
				OSI_PRINTFI("[%s-%d] gsm lac = %d,cell id = %d", __FUNCTION__, __LINE__,reg_info.gsm_scell_info.lac,reg_info.gsm_scell_info.cell_id);
			}
		}
		
	}


    
	{
	   int ret;
	   ip_addr_t addr;
	   UINT8 ip[50];
	   OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
       memset(&ip, 0, sizeof(ip));
	   memset(&addr,0,sizeof(addr));
	   ret = fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, ip);
	   fibo_taskSleep(2000);
	   for(int n=0;n<10;n++)
	   {
		   ret = fibo_getHostByName("www.baidu.com",1,0);
		   fibo_taskSleep(1000);

	   }
	   
	   OSI_PRINTFI("[%s-%d] ret = %d", __FUNCTION__, __LINE__,ret);
	}
#endif
	
	fibo_taskSleep(5000);
	OSI_PRINTFI("[%s-%d] ", __FUNCTION__, __LINE__);
	fibo_thread_delete();

}
INT32 test_task_main(char *case_number)
{
    while (true)
    {
        if (1)
        {
            //g_app_testrunning = true;
            //testcase_info* testcase = find_testcase();
            //find_testcase();
            if (NULL != case_number)
            {
                //run_testcase(case_number);
                //print_result(testcase);
                fibo_taskSleep(5000);
                OSI_PRINTFI("Create test main task thread success %d,case_number = %s ", g_task_hd, case_number);
				fibo_thread_delete();
            }
        }
    }
    //fibo_thread_delete(g_task_hd);
    g_task_hd = -1;
}

static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    switch (sig)
    {
    case GAPP_SIG_OPENCPU_API_TEST:
    {
        UINT8 test = (UINT8)va_arg(arg, int);
        if (test == 1)
        {
            INT8 case_number = (INT8)va_arg(arg, int);
            OSI_PRINTFI("[%s-%d] case_number = %d", __FUNCTION__, __LINE__, case_number);
            test_demo_handle(case_number);
            va_end(arg);
        }

        if (test == 2)
        {

            char *case_number = (char *)va_arg(arg, int);
            OSI_PRINTFI("[%s-%d] case_number = %d", __FUNCTION__, __LINE__, case_number);
            //test_demo_handle(case_number);

            if (g_task_hd < 0)
            {
                g_task_hd = fibo_thread_create(test_task_main, TEST_TASK_NAME, 1024, case_number, OSI_PRIORITY_NORMAL);
                if (g_task_hd < 0)
                {
                    OSI_PRINTFI("Create test main task thread fail");
                }
                else
                {
                    OSI_PRINTFI("Create test main task thread success %d,case_number = %s ", g_task_hd, case_number);
                }
            }
            va_end(arg);
        }
    }
    break;
	
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
    fibo_thread_create(prvThreadEntry, "mythread", 1024*8*2, NULL, OSI_PRIORITY_HIGH);
    return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
