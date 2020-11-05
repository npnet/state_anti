#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static camastae_t camctx = {0};

//#define SAVE_IMAGE_TO_SDCARD
#undef SAVE_IMAGE_TO_SDCARD
#define camd_assert(b)  \
    do                  \
{                   \
    if (!(b))       \
    osiPanic(); \
} while (0)

uint8_t *g_Databuf = NULL;

extern void test_printf(void);
static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

#ifdef SAVE_IMAGE_TO_SDCARD
static void savetoSD(char *filename,uint8_t *in_buff,uint32_t len)
{
    int fd =0;


    fd = vfs_open(filename, O_RDWR | O_CREAT | O_TRUNC);
    if (fd<0)
    {
        OSI_LOGI(0, "cam open %s error numer %d", filename,errno);
        return;
    }


    OSI_LOGI(0, "cam write image in sdcard");
    ssize_t size = vfs_write(fd, in_buff, len);
    if (size != len)
    {
        OSI_LOGI(0, "cam write size error %d", size);
    }
    vfs_close(fd);
    OSI_LOGI(0, "cam write data size %d", size);

}
#endif

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    uint16_t *pCamPreviewDataBuffer = NULL;
    uint8_t aucResult[4096];
    uint32_t ulReslen;
    int type = 0;
    bool ret = false;

#ifdef SAVE_IMAGE_TO_SDCARD
    static int count=0;
    char achFileName[20];
    uint32_t ulSweepSize=0;
#endif
    fibo_camera_StartPreview();
    while (1)
    {
        //osiThreadSleep(50);
        memset(aucResult, 0, sizeof aucResult);
        ulReslen = 0;
        if (fibo_camera_GetPreviewBuf(&pCamPreviewDataBuffer) == 0)
        {

#ifdef SAVE_IMAGE_TO_SDCARD
            count++;
            memset(achFileName, 0, 20);
            sprintf(achFileName, "/sdcard0/%d.bin", count);
            ulSweepSize = camctx.sweepsize;
            savetoSD(achFileName, pCamPreviewDataBuffer, ulSweepSize);
            if (count > 20)
                break;
#endif
            ret = fibo_sweep_code(&camctx, pCamPreviewDataBuffer, g_Databuf, aucResult, &ulReslen, &type);
            if (ret)
            {
                if (ulReslen > 0)
                {
                    OSI_LOGI(0, "Decode content %d", ulReslen);
                    break;
                }
            }
            else
            {
                memset(pCamPreviewDataBuffer, 0, camctx.CamDev.nPixcels);
                fibo_camera_PrevNotify((uint16_t *)pCamPreviewDataBuffer);
                continue;
            }
        }
    }

    memset(g_Databuf, 0, camctx.sweepsize+32);
    free(g_Databuf);
    g_Databuf = NULL;

    OSI_LOGD(0, "sweep code osiThreadExit");
    test_printf();
    fibo_thread_delete();
}

static void prvcamctx_init(camastae_t *st)
{
    memset(&st->CamDev, 0, sizeof(CAM_DEV_T));
    memset(&st->lcddev, 0, sizeof(lcdSpec_t));
    st->camTask = NULL;
    st->Decodestat = 0;
    st->Openstat = false;
    st->MemoryState = true;
    st->gCamPowerOnFlag = false;
    st->issnapshot = false;
    st->height = 0;
    st->width = 0;
    st->sweepsize=0;
    st->times=0;
    /*
       if (NULL == g_CamMutex)
       {
       g_CamMutex = osiMutexCreate();
       }
       */
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    //(60,80), (120,160), (240,320), (480,640)
    uint32_t height = 480;
    uint32_t width = 640;

    prvInvokeGlobalCtors();
    OSI_LOGI(0, "sweep code example entry");

    if (((height == 60) && (width == 80)) ||
            ((height == 120) && (width == 160)) ||
            ((height == 240) && (width == 320)) ||
            ((height == 480) && (width == 640)))
    {
        if (!camctx.Openstat)
        {
            prvcamctx_init(&camctx);

            camctx.sweepsize = height * width;
            if ((g_Databuf=(uint8_t *)malloc(camctx.sweepsize + 32)) == NULL)
            {
                OSI_LOGI(0, "malloc for sweep code buf failed");
                return;
            }

            if (0 != fibo_camera_init())
            {
                OSI_LOGI(0, "Camera init fail");
                return;
            }

            fibo_camera_GetSensorInfo(&camctx.CamDev);
            OSI_LOGI(0, "cam read img_width %d, img_height %d", camctx.CamDev.img_width, camctx.CamDev.img_height);

            if (!camctx.gCamPowerOnFlag)
            {
                //prvLcdInit();
                camctx.height = height;
                camctx.width = width;
                camctx.times = camctx.CamDev.img_width/width;
                camctx.gCamPowerOnFlag = true;
                camctx.Openstat = true;
                OSI_LOGI(0, "sweepCode:open height=%d,width=%d,times=%d,ulSweepSize=%d.",height,width,camctx.times,camctx.sweepsize);

                fibo_thread_create(prvThreadEntry, "sweep code", 1024*80, &camctx, OSI_PRIORITY_NORMAL);
            }
        }
    }
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
