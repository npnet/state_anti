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

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


static uint8_t get_mcc_mnc_from_plmn(uint8_t *plmn, uint8_t *mcc, uint8_t *mnc)
{
    if (plmn == NULL || mcc == NULL || mnc == NULL)
    {
        return 0;
    }
    //64 F0 10
    mcc[0] = plmn[0] & 0x0F;
    mcc[1] = plmn[0] >> 4;
    mcc[2] = plmn[1] & 0x0F;

    mnc[0] = plmn[2] & 0x0F;
    mnc[1] = plmn[2] >> 4;
    mnc[2] = plmn[1] >> 4;

    if (mnc[2] > 9)
    {
        mnc[2] = 0;
        return 2;
    }
    else
    {
        return 3;
    }
}

key_callback get_cellinfo_ex_cb(void *param)
{

    if(param == NULL)
    {
		OSI_PRINTFI("[%s-%d]GET cellinfo FAIL", __FUNCTION__, __LINE__);
		return NULL;
	}
	uint8_t sMCC[3] = {0};
	uint8_t sMNC[3] = {0};
	uint8_t sMNC_Number = 0;
    opencpu_cellinfo_t *cellinfo = (opencpu_cellinfo_t *)param;
	if(cellinfo->curr_rat >=4)
	{
	    int cellid_s = cellinfo->lte_cellinfo.lteScell.enodeBID << 8 | cellinfo->lte_cellinfo.lteScell.cellId;
		sMNC_Number = get_mcc_mnc_from_plmn(&cellinfo->lte_cellinfo.lteScell.plmn[0], &sMCC[0], &sMNC[0]);
		
	    OSI_PRINTFI("[%s-%d]LTE service MCC=%d%d%d,MNC=%d%d,CELL_ID = %lX,TAC=%X", __FUNCTION__, __LINE__,sMCC[0],sMCC[1],sMCC[2],sMNC[0],sMNC[1],cellid_s,cellinfo->lte_cellinfo.lteScell.tac);
        if(cellinfo->lte_cellinfo.lteNcellNum > 0)
        {
            for(int i =0; i<cellinfo->lte_cellinfo.lteNcellNum;i++)
            {
                memset(sMCC,0,3);
				memset(sMNC,0,3);
                int cellid_nb = cellinfo->lte_cellinfo.lteNcell[i].cellAccRelInfo.enodeBID << 8 | cellinfo->lte_cellinfo.lteNcell[0].cellAccRelInfo.cellId;
				sMNC_Number = get_mcc_mnc_from_plmn(&cellinfo->lte_cellinfo.lteNcell[i].cellAccRelInfo.plmn[0], &sMCC[0], &sMNC[0]);
				int tac = cellinfo->lte_cellinfo.lteNcell[i].cellAccRelInfo.tac;
				OSI_PRINTFI("[%s-%d]LTE Ncell[%d]MCC=%d%d%d,MNC=%d%d, CELL ID = %lX,TAC=%X", __FUNCTION__, __LINE__,i,sMCC[0],sMCC[1],sMCC[2],sMNC[0],sMNC[1],cellid_nb,tac);
			}
        }
	}
	else
	{
	    
	    sMNC_Number = get_mcc_mnc_from_plmn(&cellinfo->gsm_cellinfo.pCurrCellInfo.nTSM_LAI[0], &sMCC[0], &sMNC[0]);
		int cellid = cellinfo->gsm_cellinfo.pCurrCellInfo.nTSM_CellID[0]<<8|cellinfo->gsm_cellinfo.pCurrCellInfo.nTSM_CellID[1];
		int lac = cellinfo->gsm_cellinfo.pCurrCellInfo.nTSM_LAI[3]<<8|cellinfo->gsm_cellinfo.pCurrCellInfo.nTSM_LAI[4];
	    OSI_PRINTFI("[%s-%d]GSM service MCC=%d%d%d,MNC=%d%d,CELL_ID = %lX,TAC=%X", __FUNCTION__, __LINE__,sMCC[0],sMCC[1],sMCC[2],sMNC[0],sMNC[1],cellid,lac);
		if(cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCellNUM>0)
		{
		    for(int i =0; i<cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCellNUM;i++)
		    {
				sMNC_Number = get_mcc_mnc_from_plmn(&cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[i].nTSM_LAI[0], &sMCC[0], &sMNC[0]);
				int cellid_n = cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[i].nTSM_CellID[0]<<8|cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[i].nTSM_CellID[1];
				int lac_n = cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[i].nTSM_LAI[3]<<8|cellinfo->gsm_cellinfo.pNeighborCellInfo.nTSM_NebCell[i].nTSM_LAI[4];
				OSI_PRINTFI("[%s-%d]GSM nbcell[%d] MCC=%d%d%d,MNC=%d%d,CELL_ID = %lX,TAC=%X", __FUNCTION__, __LINE__,i,sMCC[0],sMCC[1],sMCC[2],sMNC[0],sMNC[1],cellid_n,lac_n);
			}
		}
       
		OSI_PRINTFI("[%s-%d]GET cellinfo for GSM cell info", __FUNCTION__, __LINE__);
	}
	return NULL;
   
}


static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

    for (int n = 0; n < 10; n++)
    {
        OSI_LOGI(0, "hello world %d", n);
        fibo_taskSleep(1000);
    }

	int test = 1;

    while(1)
    {
		OSI_PRINTFI("[%s-%d]cellinfo_ex begin", __FUNCTION__, __LINE__);	
		//opencpu_cellinfo_t cellinfo;
		//memset(&cellinfo,0,sizeof(opencpu_cellinfo_t));
		//if(test)
		{
			//fibo_getCellInfo(&cellinfo, 0);
		}
		test = 0;
		//fibo_taskSleep(20*1000);
		fibo_getCellInfo_ex(get_cellinfo_ex_cb, 0);
		fibo_taskSleep(20*1000);
	}

	test_printf();
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4*2, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
