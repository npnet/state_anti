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

#define FIBO_BT_LOG(format, ...) OSI_PRINTFI("[AUTO_BT][%s:%d]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

typedef struct _bt_param{
    char bt_pair_state;
	char bt_connect_state;
	char on_flag;
	char off_flag;
	char bt_scan_complete_flag;
	FIBO_BT_DEVICE_INFO local_bt_info;
	FIBO_BT_DEVICE_T scan_bt_info_list;
	uint8_t scan_bt_number;
	FIBO_BT_DEVICE_T paired_bt_list;
	char *sendbuf;
	uint16_t send_data_len;
	char *recvbuf;
	uint16_t recv_buf_len;
}bt_param_t;

bt_param_t bt_param;

static bool bt_info_is_unstored(char *bt_addr_info,FIBO_BT_DEVICE_INFO bt_device[])
{
   int i = 0;
   if(bt_addr_info == NULL )
   {
       return 0;
   }
   for(i = 0; i<FIBO_BT_DEVICE_NUM; i++)
   {

	   if(!strcasecmp(bt_addr_info, &bt_device[i].addr[0]))
	   {
           return 0; // stored before
	   }

   }
   return 1;
}

char connect_bt_addr[ATC_MAX_BT_ADDRESS_STRING_LEN];
static void sig_res_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    FIBO_BT_LOG("sig=%d",sig);
    switch (sig)
    {
		//fibo_PDPActive  ip address resopnse event
		
		case GAPP_SIG_PDP_ACTIVE_ADDRESS:
		{
			UINT8 cid = (UINT8)va_arg(arg, int);
			char *addr = (char *)va_arg(arg, char *);
			va_end(arg);
			if(addr != NULL)
			{
				FIBO_BT_LOG("sig_res_callback  cid = %d, addr = %s ", cid, addr);
			}
			else 
			{
				FIBO_BT_LOG();
			}
			
		}
		break;
		case GAPP_SIG_BT_ON_IND:
		{
			char on_success_flag = (char)va_arg(arg, int);
			va_end(arg);
			fibo_taskSleep(100);
			if(on_success_flag == 1)
			{
				fibo_bt_scan_nearby(1,NULL);
			}
			

		}
		
		break;

		case GAPP_SIG_BT_OFF_IND:
		break;

		case GAPP_SIG_BT_VISIBILE_IND:
		break;

		case GAPP_SIG_BT_HIDDEN_IND:
		break;

		case GAPP_SIG_BT_SET_LOCAL_NAME_IND:
		break;

		case GAPP_SIG_BT_SET_LOCAL_ADDR_IND:
			FIBO_BT_LOG("GAPP_SIG_BT_SET_LOCAL_ADDR_IND");
		break;

		case GAPP_SIG_BT_SCAN_DEV_REPORT:
		{
			char *bt_addr = (char *)va_arg(arg, char *);
			char *bt_name = (char *)va_arg(arg, char *);
		    uint32_t bt_class = (uint32_t)va_arg(arg, int);
			va_end(arg);
			if(bt_info_is_unstored(bt_addr,bt_param.scan_bt_info_list.bt_device) && bt_param.scan_bt_number<30)
			{
				strncpy(bt_param.scan_bt_info_list.bt_device[bt_param.scan_bt_number].addr,bt_addr,strlen(bt_addr));
				strncpy(bt_param.scan_bt_info_list.bt_device[bt_param.scan_bt_number].name,bt_name,strlen(bt_name));
				bt_param.scan_bt_info_list.bt_device[bt_param.scan_bt_number].dev_class = bt_class;
				bt_param.scan_bt_number++;
				FIBO_BT_LOG("bt_name=%s, bt_addr = %s",bt_name,bt_addr);
				
			}
			
		}
			
		break;

		case GAPP_SIG_BT_INQUIRY_CANCEL_IND:
		break;

		case GAPP_SIG_BT_SCAN_COMP_IND:
		{
			FIBO_BT_LOG();
			uint32_t scan_complete_ind = (uint32_t)va_arg(arg, int);
			int i = 0;
			if(scan_complete_ind ==1)
			{
				for(i=0;i<bt_param.scan_bt_number;i++)
				{
					if(!strcasecmp("D0000003",bt_param.scan_bt_info_list.bt_device[i].name))
					{
						fibo_bt_pair(1,bt_param.scan_bt_info_list.bt_device[i].addr);
						break;
					}
				}
			}
		}
		break;

		case GAPP_SIG_BT_PAIR_COMPLETE_IND:
		{
			FIBO_BT_LOG();
			int i =0;
			
			uint32_t pair_complete_ind = (uint32_t)va_arg(arg, int);
			va_end(arg);
			for(i=0;i<bt_param.scan_bt_number;i++)
			{
				if(!strcasecmp("D0000003",bt_param.scan_bt_info_list.bt_device[i].name))
				{
					strcpy(connect_bt_addr,bt_param.scan_bt_info_list.bt_device[i].addr);
					break;
				}
			}
			
			if(pair_complete_ind == 1)
			{
			    bt_param.bt_pair_state = 1;
                fibo_bt_connect(1, "SPP",connect_bt_addr);
			}

		}		
		break;

		case GAPP_SIG_BT_DELETE_DEV_IND:
			FIBO_BT_LOG("GAPP_SIG_BT_DELETE_DEV_IND");
		break;

		case GAPP_SIG_BT_SSP_NUM_IND:
		break;

		case GAPP_SIG_BT_CONNECT_IND:
		{
			FIBO_BT_LOG();
			uint32_t connect_complete_ind = (uint32_t)va_arg(arg, int);
			va_end(arg);
            if(connect_complete_ind == 1)
			{
			    bt_param.bt_connect_state = 1;
				fibo_bt_send_data("SPP",0,"123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890",120);
			}

		}
		break ;

		case GAPP_SIG_BT_DISCONNECT_IND:
			FIBO_BT_LOG();
			uint32_t disconnect_complete_ind = (uint32_t)va_arg(arg, int);
			va_end(arg);
			FIBO_BT_LOG("disconnect_complete_ind=%d",disconnect_complete_ind);
		break;

		case GAPP_SIG_BT_DATA_RECIEVE_IND:
		{
			uint32_t data_len = (uint32_t)va_arg(arg, int);
			va_end(arg);
			uint16_t len = 0;
			FIBO_BT_LOG("data_len=%d",data_len);
			bt_param.recvbuf = fibo_malloc(data_len);
			if(bt_param.recvbuf != NULL)
			{
				fibo_bt_read_data("SPP", bt_param.recvbuf, &len);
				FIBO_BT_LOG("data=%s,data_len=%d",bt_param.recvbuf,len);

			}
			fibo_free(bt_param.recvbuf);
			bt_param.recvbuf = NULL;
			
		}
		break;

		default:
		break;
    }
}


static void at_res_callback(UINT8 *buf, UINT16 len)
{
    OSI_PRINTFI("FIBO <--%s", buf);
}

static FIBO_CALLBACK_T user_callback = {
	.fibo_signal = sig_res_callback,
    .at_resp = at_res_callback};


static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

    for (int n = 0; n < 10; n++)
    {
        FIBO_BT_LOG();
        fibo_taskSleep(1000);
    }

	char add[]="11:22:33:44:55:66";
	fibo_bt_set_read_addr(0,add);

	fibo_bt_onoff(1);

    while(1)
    {
        FIBO_BT_LOG("hello world %d");
        fibo_taskSleep(20000);
	}
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();
    memset(&bt_param,0,sizeof(bt_param));
    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
     return (void *)&user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
