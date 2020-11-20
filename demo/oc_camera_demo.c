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

typedef struct
{
    CAM_DEV_T camdev;
    lcdSpec_t lcddev;
} camprevExample_t;

#define camd_assert(b)  \
    do                  \
    {                   \
        if (!(b))       \
            osiPanic(); \
    } while (0)

static camprevExample_t camPrev;

extern void test_printf(void);
static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

static void prvBlackPrint(void)
{
    OSI_LOGI(0, "cam LCD display black");

    lcdDisplay_t lcdRec;

    lcdRec.x = 0;
    lcdRec.y = 0;
    lcdRec.width = camPrev.lcddev.width;
    lcdRec.height = camPrev.lcddev.height;
    fibo_lcd_FillRect(&lcdRec, BLACK);
}

static void prvCameraPrint(void *buff)
{
    lcdFrameBuffer_t dataBufferWin;
    lcdDisplay_t lcdRec;
    uint32_t offset;
    //if the image is yuv422 ,offset = yaddr+(height-1)*width*2
    offset = camPrev.camdev.img_width * (camPrev.camdev.img_height - 1) * 2;
    dataBufferWin.buffer = (uint16_t *)((uint32_t)buff + offset);
    dataBufferWin.colorFormat = LCD_RESOLUTION_YUV422_UYVY;
    dataBufferWin.keyMaskEnable = false;

    dataBufferWin.region_x = 0;
    dataBufferWin.region_y = 0;
    dataBufferWin.height = camPrev.lcddev.height;
    dataBufferWin.width = camPrev.lcddev.width;
    dataBufferWin.widthOriginal = camPrev.camdev.img_width;
    dataBufferWin.rotation = 1;
    lcdRec.x = 0;
    lcdRec.y = 0;
    lcdRec.width = camPrev.lcddev.width;
    lcdRec.height = camPrev.lcddev.height; //because the image is 248*320,if rotation 90 degree ,just can show the 240*240,

    fibo_lcd_Setvideosize(camPrev.camdev.img_height, camPrev.camdev.img_width, camPrev.camdev.img_width);
    fibo_lcd_FrameTransfer(&dataBufferWin, &lcdRec);
    //osiDelayUS(1000 * 1000 * 3);
}

static void prvLcdInit(void)
{
    uint32_t uldevid;
    OSI_LOGI(0, "cam LCD init and start test");
    fibo_lcd_init();
    fibo_lcd_Getinfo(&uldevid, &camPrev.lcddev.width, &camPrev.lcddev.height);
    prvBlackPrint();
    OSI_LOGI(0, "cam lcd read width %d, height %d", camPrev.lcddev.width, camPrev.lcddev.height);
    osiThreadSleep(1000);
    //halPmuSwitchPower(HAL_POWER_BACK_LIGHT, true, true);
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    uint16_t *pCamPreviewDataBuffer = NULL;
    UINT32 ulTimer=0;
    
    fibo_camera_StartPreview();
    while (1)
    {
        if (fibo_camera_GetPreviewBuf(&pCamPreviewDataBuffer) == 0)
        {
            ulTimer++;
            //deal data
            OSI_LOGD(0, "cam deal data");
            prvCameraPrint(pCamPreviewDataBuffer);
            memset(pCamPreviewDataBuffer, 0, camPrev.camdev.nPixcels);
            fibo_camera_PrevNotify((uint16_t *)pCamPreviewDataBuffer);
        }

        if (ulTimer >=40)
        {   
            fibo_camera_StopPreview();
            pCamPreviewDataBuffer = NULL;
            if (fibo_camera_CaptureImage(&pCamPreviewDataBuffer)==0)
            {  
                OSI_LOGD(0, "cam capture start");
                prvBlackPrint();
                osiThreadSleep(1000);
                prvCameraPrint(pCamPreviewDataBuffer);
                osiThreadSleep(3000);
                OSI_LOGD(0, "cam capture end");
                prvBlackPrint();
                osiThreadSleep(1000);
            }
            fibo_camera_StartPreview();
            ulTimer=0;
        }
        osiThreadSleep(1);
    }
    OSI_LOGD(0, "camPrevTask osiThreadExit");
	test_printf();
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();
    OSI_LOGI(0, "Camera prev example entry");
    if (fibo_camera_init() != 0)
    {
        OSI_LOGI(0, "Camera init fail,maybe no memory");
        return 0;
    }

    fibo_camera_GetSensorInfo(&camPrev.camdev);
    OSI_LOGI(0, "cam read img_width %d, img_height %d", camPrev.camdev.img_width, camPrev.camdev.img_height);
    prvLcdInit();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*8, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
