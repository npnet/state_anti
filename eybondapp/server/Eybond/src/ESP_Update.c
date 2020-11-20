 /**************Copyright(C)��2015-2026��Shenzhen Eybond Co., Ltd.***************
  *@brief   : eybond soft update process
  *@notes   : 2017.09.11 CGQ 
*******************************************************************************/
#include "ql_stdlib.h"
#include "ql_system.h"

#include "eyblib_memory.h"
#include "eyblib_swap.h"
#include "eyblib_CRC.h"

#include "eybpub_Debug.h"

#include "ESP_Update.h" 
#include "eybond.h"
#include "DeviceUpdate.h"

#include "file.h"

#include "Modbus.h"
#include "updatetask.h"

#include "FlashHard.h"
#include "StateGridData.h"
#include "SSL.h"

#define FILE_FLAG_ADDR	(FLASH_UPDATE_FILE_ADDR) // POINT_TAB_FILE_ADDR// 
#define FILE_SAVE_ADDR	(FILE_FLAG_ADDR + 0x02000)
#define FILE_MAX_SIZE	(FLASH_UPDATE_FILE_SIZE)

File_t *update = null;
DeviceType_t *deviceType = null;
u32_t updateID = 0;
u32_t fileFlagAddr = FILE_FLAG_ADDR;

static void enterUpdate(void);

/*******************************************************************************
  * @brief  
  * @note   None 0x20
  * @param  None
*******************************************************************************/
u8_t Update_file(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
		u8_t type;
		u8_t sizeHH;
        u8_t sizeHL;
        u8_t sizeLH;
        u8_t sizeLL;
        u8_t sliceCntH;
		u8_t sliceCntL;
        u8_t sliceSizeH; 
		u8_t sliceSizeL;
    }Rcve_t;
    typedef struct
    {
        u8_t state;
    }Ack_t;
#pragma pack()  
	u16_t blockNum;
    int i;
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    EybondHeader_t *ackHead = &esp->head;
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);


    updateID = 0;
    blockNum = (rcvePara->sliceCntH<<8)|rcvePara->sliceCntL;
    i = rcvePara->sizeHH<<24 | rcvePara->sizeHL<<16 | rcvePara->sizeLH << 8 | rcvePara->sizeLL;
    APP_DEBUG("fiile Size %x \r\n", i);
    if (blockNum == 0)
    {
        ackPara->state = 0;
    }
    else if (i > 0x10000)
    {
        ackPara->state = 1;
    }
    else
    {
        memory_release(update);
        update = null;
        if (rcvePara->type == 0x10) //CA file
        {
            fileFlagAddr = CA_FILE_ADDR;
            update = File_init(fileFlagAddr + 0x2000, blockNum, \
                    (rcvePara->sliceSizeH<<8)|rcvePara->sliceSizeL);
        }
        else if (rcvePara->type == 0x20) //point file
        {
            fileFlagAddr = POINT_TAB_FILE_ADDR;
            update = File_init(fileFlagAddr + 0x2000, blockNum, \
                    (rcvePara->sliceSizeH<<8)|rcvePara->sliceSizeL);
        }

        if (update != null)
        {
            update->area = rcvePara->type ;
            ackPara->state = 0;
        }
        else
        {
            ackPara->state = 1;
        }
    }

    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    
    return 0;
}

/*******************************************************************************
  * @brief  
  * @note   None 0x21
  * @param  None
*******************************************************************************/
u8_t Update_soft(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u16_t sliceCnt;
        u16_t sliceSize; 
    }Rcve_t;
    typedef struct
    {
        u8_t state;
    }Ack_t;
#pragma pack()  
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    EybondHeader_t *ackHead = &esp->head;
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);

	updateID = SELF_UPDATE_ID;
	Swap_bigSmallShort(&rcvePara->sliceCnt);
	
	if (rcvePara->sliceCnt == 0)
	{
		if ((update != null)
			&& (update->area == 0)
			&& (0 == File_validCheck(update))
			)
		{
			enterUpdate();
			ackPara->state = 0;
		}
		else
		{
			ackPara->state = 1;
		}
	}
	else
	{
		memory_release(update);
        fileFlagAddr = FILE_FLAG_ADDR;
	    update = File_init(FILE_SAVE_ADDR, (rcvePara->sliceCnt), ENDIAN_BIG_LITTLE_16(rcvePara->sliceSize));
	    if (update != null)
	    {
			update->area = 0;
	        ackPara->state = 0;
	    }
	    else
	    {
			ackPara->state = 1;
	    }
	}
    
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    
    return 0;
}

/*******************************************************************************
  * @brief  
  * @note   None 0x22
  * @param  None
*******************************************************************************/
u8_t Update_device(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
		u8_t type;
		u8_t bandrateH;
		u8_t bandrateL;
        u8_t sliceCntH;
		u8_t sliceCntL;
        u8_t sliceSizeH; 
		u8_t sliceSizeL;
    }Rcve_t;
    typedef struct
    {
        u8_t state;
    }Ack_t;
#pragma pack()  
	u16_t blockNum;
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    EybondHeader_t *ackHead = &esp->head;
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);

	memory_release(deviceType);
	deviceType = memory_apply(sizeof(DeviceType_t));
	if (deviceType != null)
	{
		deviceType->addr = esp->head.addr;
		deviceType->type = rcvePara->type;
		deviceType->bandrate = (rcvePara->bandrateH << 8) | rcvePara->bandrateL;
		updateID = DEVICE_UPDATE_ID;
		
		blockNum = (rcvePara->sliceCntH<<8)|rcvePara->sliceCntL;
		if (blockNum == 0)
		{
			if ((update != null)
				&& (update->area == 1)
				&& (0 == File_validCheck(update))
				)
			{
				enterUpdate();
				ackPara->state = 0;
			}
			else
			{
				ackPara->state = 1;
			}
		}
		else
		{  
			memory_release(update);
            fileFlagAddr = FILE_FLAG_ADDR;
    		update = File_init(FILE_SAVE_ADDR, blockNum, \
						(rcvePara->sliceSizeH<<8)|rcvePara->sliceSizeL);
	
		    if (update != null)
		    {
				update->area = 1;
		   		ackPara->state = 0;
		    }
			else
			{
				ackPara->state = 1;
			}
		}
	}
	else
	{
		ackPara->state = 1;
	}	
    
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    
    return 0;
}

/*******************************************************************************
  * @brief  
  * @note   0x23
  * @param  None
*******************************************************************************/
u8_t Update_deviceState(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u8_t process;
    }Ack_t;
#pragma pack()   
    Buffer_t buf;
    EybondHeader_t *ackHead;
    Ack_t *ackPara;
    
    ackHead = memory_apply(sizeof(EybondHeader_t) + sizeof(Ack_t) + 1);
	ackPara = (Ack_t*)(ackHead + 1);
    
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
	ackPara->process = Update_state();

	buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    return 0;
}

/*******************************************************************************
  * @brief  
  * @note   ox24
  * @param  None
*******************************************************************************/
u8_t Update_deviceCancel(ESP_t *esp)
{
    #pragma pack(1)
    typedef struct
    {
        u8_t result;
    }Ack_t;
#pragma pack()   
    Buffer_t buf;
    EybondHeader_t *ackHead;
    Ack_t *ackPara;
    
    ackHead = memory_apply(sizeof(EybondHeader_t) + sizeof(Ack_t) + 1);
	ackPara = (Ack_t*)(ackHead + 1);
    
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
	ackPara->result = 0;
	Update_end();

	buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    return 0;
}

/*******************************************************************************
  * @brief  
  * @note   None  0x25
  * @param  None
*******************************************************************************/
u8_t Update_info(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u8_t result;
    }Rcve_t;
    typedef struct
    {
        u8_t flag;
        u8_t state;
        u8_t md5[32];
    }Ack_t;
#pragma pack()   
    Buffer_t buf;
    EybondHeader_t *ackHead;
    Ack_t *ackPara;
    Rcve_t *rcve;
    
    rcve = (Rcve_t*)esp->PDU;
    ackHead = memory_apply(sizeof(EybondHeader_t) + sizeof(Ack_t) + 1);
    ackPara = (Ack_t*)(ackHead + 1);
    
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    
    if (update == null)
    {
        u32_t flagAddr = FILE_FLAG_ADDR;
        File_t *file = memory_apply(sizeof(File_t));

        if (rcve->result == 0x10)
        {
            flagAddr = CA_FILE_ADDR; 
        }
        else if (rcve->result == 0x20)
        {
            flagAddr = POINT_TAB_FILE_ADDR;
        }
        File_state(file, flagAddr);
		r_memset(ackPara->md5, '0', sizeof(ackPara->md5));
		if (file->area != esp->PDU[0] || (0 != File_validCheck(file)))
		{
			ackPara->state = 0x02;
		}
		else if (File_Check(file) != 0)
		{
			ackPara->state = 0x01;
		}
		else 
		{
			ackPara->state = 0;
			update = file;
			Swap_hexChar((char*)ackPara->md5, file->md5, 16, 0);
			file = null;
		}
        //get flash recode file flag
        ackPara->flag = esp->PDU[0];
        memory_release(file);
    }
    
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    
    return 0;
}
/*******************************************************************************
  * @brief  
  * @note   None 0x26
  * @param  None
*******************************************************************************/
u8_t Update_dataRcve(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u16_t sliceNum;
        u8_t  data[];
    }Rcve_t;
    typedef struct
    {
        u16_t sliceNum;
        u8_t state;
    }Ack_t;
#pragma pack()  
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    EybondHeader_t *ackHead = &esp->head;
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);
    u16_t offset = ENDIAN_BIG_LITTLE_16(rcvePara->sliceNum);
    u16_t dataLen = ENDIAN_BIG_LITTLE_16(esp->head.msgLen) - 2 - 4;
    
    
    if ((0 == crc16_standard(CRC_RTU, rcvePara->data, dataLen + 2))
        && (0 == File_rcve(update, offset, rcvePara->data, dataLen)) //����д��ɹ�
        )
    {
        ackPara->state = 0;
    }
    else
    {
        ackPara->state = 1;
    }
    ackPara->sliceNum = ENDIAN_BIG_LITTLE_16(offset);
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    
    return 0;    
}

/*******************************************************************************
  * @brief  0x27
  * @note   None
  * @param  None
*******************************************************************************/
u8_t Update_rcveState(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u16_t sliceCnt;
    }Rcve_t;
    typedef struct
    {
        u8_t state;
        u8_t  data[];
    }Ack_t;
#pragma pack()  
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    u16_t sliceCount = ENDIAN_BIG_LITTLE_16(rcvePara->sliceCnt);
    u16_t size = (sliceCount>>3) + ((sliceCount&0x07) ? 1 : 0);
    EybondHeader_t *ackHead = memory_apply(sizeof(EybondHeader_t) + sizeof(Ack_t) + size);
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);
    
    r_memcpy(ackHead, &esp->head, sizeof(EybondHeader_t));
    
    if (update == null || update->sliceStateLen > size)
    {
        r_memset(ackPara->data, 0x00, size);
        ackPara->state = 1;
		memory_release(update);
    }
    else
    {
        ackPara->state = 0;
        r_memcpy(ackPara->data, update->sliceState, update->sliceStateLen);
    }
    
    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2 + size);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t) + size;
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    
    return 0;
}
/*******************************************************************************
  * @brief  0x28
  * @note   None
  * @param  None
*******************************************************************************/
u8_t Update_dataCheck(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u8_t size[4];
        char  md5[32];
    }Rcve_t;
    typedef struct
    {
        u8_t state;
    }Ack_t;
#pragma pack()  
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    EybondHeader_t *ackHead = &esp->head;
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);
    char md5[64];

	
    update->size = (rcvePara->size[0]<<24)|(rcvePara->size[1]<<16)|(rcvePara->size[2]<<8)|(rcvePara->size[3]);
	r_memcpy(md5, rcvePara->md5, sizeof(rcvePara->md5));	
	md5[sizeof(rcvePara->md5)] = '\0';
    Swap_charHex(update->md5, md5);

      File_save(update, fileFlagAddr);
    ackPara->state = File_Check(update);
	
	if (ackPara->state == 0)
	{
		enterUpdate();
	}

    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    memory_release(update);  
    update = null;
    return 0;
}
/*******************************************************************************
  * @brief  
  * @note   None
  * @param  None
*******************************************************************************/
u8_t Update_exit(ESP_t *esp)
{
#pragma pack(1)
    typedef struct
    {
        u32_t state;
    }Rcve_t;
    typedef struct
    {
        u8_t state;
    }Ack_t;
#pragma pack()  
    Buffer_t buf;
    Rcve_t *rcvePara = (Rcve_t*)esp->PDU;
    EybondHeader_t *ackHead = &esp->head;
    Ack_t *ackPara = (Ack_t*)(ackHead + 1);
    
    
    ackPara->state = 0;

    ackHead->msgLen = ENDIAN_BIG_LITTLE_16(sizeof(Ack_t) + 2);
    buf.lenght = sizeof(EybondHeader_t) + sizeof(Ack_t);
    buf.payload = (u8_t*)ackHead;
    esp->ack(&buf);
    memory_release(buf.payload);
    memory_release(update);  
	memory_release(deviceType);  
    update = null;
	deviceType = null;
    return 0;
}

/*******************************************************************************
  * @brief  
  * @note   None
  * @param  None
*******************************************************************************/
static void enterUpdate(void)
{
    if (updateID != 0)
    {
        Ql_OS_SendMessage(UPDATE_TASK, updateID, (u32)update, (u32)deviceType);
        update = null;
        deviceType = null;
    }
}

/******************************************************************************/

