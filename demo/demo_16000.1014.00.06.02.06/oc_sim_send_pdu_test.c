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

typedef void (*osiEventCallback_t)(void *ctx, const osiEvent_t *event);
#define FIBO_SIM_LOG(format, ...) OSI_PRINTFI("[FIBO_SIM][%s:%d]" format, __FUNCTION__, __LINE__, ##__VA_ARGS__)


extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

static int _hexToAscii(uint8_t v)
{
    if (v >= 0 && v <= 9)
        return v + '0';
    if (v >= 0xa && v <= 0xf)
        return v + 'A' - 10;
    return -1;
}


static int cfwBytesToHexStr(const void *src, size_t size, void *dst)
{
    if (size == 0)
        return 0;

    if (src == NULL || dst == NULL)
        return -1;

    const uint8_t *psrc = (const uint8_t *)src;
    uint8_t *pdst = dst;
    for (size_t n = 0; n < size; n++)
    {
        uint8_t lo = *psrc & 0xf;
        uint8_t hi = (*psrc++ >> 4) & 0xf;
        *pdst++ = _hexToAscii(hi);
        *pdst++ = _hexToAscii(lo);
    }

    *pdst = '\0';
    return size * 2;
}


static void fibo_sim_response_cb(atCommand_t *cmd, const osiEvent_t *event)
{
	const CFW_EVENT *cfw_event = (const CFW_EVENT *)event;
	if (cfw_event->nType == 0)
	{
		uint16_t length = cfw_event->nParam2;
		FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP: nParam2 = %d!", length);
		uint8_t *TPDU = (uint8_t *)cfw_event->nParam1;
		if (length > 0)
		{
			//atMemFreeLater((void *)cfw_event->nParam1);
			//free(cmd);
			//cmd =NULL;
			FIBO_SIM_LOG();
		}

		if (length > 260)
		{
			FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP failed: length = %d", length);
			//free(cmd);
			//cmd =NULL;
			return;
		}
		else if (length == 0)
		{
			uint16_t sw = cfw_event->nParam1;
			char response[16] = {0};
			sprintf(response, "%X",sw);
			FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP:response = %s length = %d", response,strlen(response));
			//free(cmd);
			//cmd =NULL;
			return;
		}
		else
		{
			uint16_t sw = cfw_event->nParam2 >> 16;
			uint16_t len = (cfw_event->nParam2 & 0xFFFF) << 1;
			FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP: length = %d", len);
			len += 12 + 4 + 1; 
			char *response = malloc(len);
			if (response == NULL)
			{
				FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP Malloc failed!");
				//free(cmd);
			    //cmd =NULL;
				return;
			}
			FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP: update length = %d", len);
			memset(response, 0, len);
			uint16_t i = sprintf(response, "%d, ", (length + 2) << 1);
			length = cfwBytesToHexStr(TPDU, length, response + i);
			if (length == 0)
			{
				FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP SUL_hex2ascii failed!");
				free(response);
				//free(cmd);
			    //cmd =NULL;
				return;
			}
			i += sprintf(response + i + length, "%X", sw);

			FIBO_SIM_LOG("EV_CFW_SIM_TPDU_COMMAND_RSP: return data length = %d, i = %d,response = %s", length, i,response);
			free(response);
			//free(cmd);
			//cmd =NULL;
		}
	}
}



static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

	ip_addr_t addr;

	int test =1;
	reg_info_t reg_info;
	memset(&reg_info,0,sizeof(reg_info_t));
	while(test)
	{
		fibo_getRegInfo(&reg_info, 0);
		fibo_taskSleep(1000);
		if(reg_info.nStatus==1)
		{
            //fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            {
				pdp_profile_t pdp_profile;
			    char *pdp_type = "ipv4v6";
			    char *apn = "3gnet";
			    memset(&pdp_profile, 0, sizeof(pdp_profile));   
			    pdp_profile.cid = 4;
			    memcpy(pdp_profile.nPdpType, pdp_type, strlen((char *)pdp_type));
			    memcpy(pdp_profile.apn, apn, strlen((char *)apn));
			    fibo_asyn_PDPActive(1, &pdp_profile, 0);

			}
			test = 0;
			fibo_taskSleep(5000);
			OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
		}
	}
	fibo_taskSleep(1000);

    //fibo_http_get();
    atCommand_t *cmd = NULL;
	cmd = fibo_malloc(sizeof(atCommand_t));
	if(cmd != NULL)
	{
		fibo_sim_SendApdu(0,0,"00A40804047FFF6F07",18,fibo_sim_response_cb, (void *)cmd);
		
		fibo_taskSleep(1000);
		
		fibo_sim_SendApdu(0,0,"00B0000009",10,fibo_sim_response_cb, (void *)cmd);
		
		fibo_taskSleep(1000);

	}



	while(1)
	{
		OSI_LOGI(0, "hello world");
		fibo_taskSleep(5*1000);
	}

	test_printf();
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
