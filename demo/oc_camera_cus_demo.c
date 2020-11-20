#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"


typedef struct {
    CAM_DEV_T camdev;
    lcdSpec_t lcddev;
}camprevExample_t;

static camprevExample_t camPrev;

static void prvThreadEntry(void *param)
{
	uint8_t addr = 0xf0;
    uint8_t data = 0x23;
    uint8_t value[8] = {0};
	int32_t num = 0;

	bool lRet = false;

	camera_reg_t cam_regs[] = {
					{0xf3, 0x83},
					{0xf5, 0x08},
					{0xf7, 0x01},
					{0xf8, 0x01},
					{0xf9, 0x4e},
					{0xfa, 0x00},
					{0xfc, 0x02},
					{0xfe, 0x02},
					{0x81, 0x03},
				};


	num = sizeof(cam_regs) / sizeof(camera_reg_t);

	if (0 != fibo_camera_init())
	{
	   OSI_PRINTFI("[%s %d] camera init fail ", __FUNCTION__, __LINE__);
	}

	fibo_camera_GetSensorInfo(&camPrev.camdev);
	
	if (camPrev.camdev.pNamestr != NULL)
	{
	   OSI_PRINTFI("[%s %d] SensorName %s", __FUNCTION__, __LINE__, camPrev.camdev.pNamestr);
	   if ((cam_regs == NULL) || (num == 0))
	   {
		   OSI_PRINTFI("[%s %d] customer camera regs num error", __FUNCTION__, __LINE__);
		   return;
	   }
	   else
	   {
		   OSI_PRINTFI("[%s %d] customer camera regs num %d", __FUNCTION__, __LINE__, num);
		   lRet = fibo_camera_regs_init_cus(camPrev.camdev.pNamestr, cam_regs, num);
		   if (false == lRet)
		   {
		   		OSI_PRINTFI("[%s %d] customer camera regs init failed", __FUNCTION__, __LINE__);
				return;
		   }
	   }
	}
	else
	{
	   OSI_PRINTFI("[%s %d] SensorName is NULL", __FUNCTION__, __LINE__);
	   return;
	}

	lRet = fibo_camera_write_reg_cus(camPrev.camdev.pNamestr, addr, data);
	if (false == lRet)
	{
		OSI_PRINTFI("[%s %d] Write to reg 0x%X failed", __FUNCTION__, __LINE__, addr);
		return;
	}

	lRet = fibo_camera_read_reg_cus(camPrev.camdev.pNamestr, addr, value);
	if (false == lRet)
	{
		OSI_PRINTFI("[%s %d] read reg 0x%x fail", __FUNCTION__, __LINE__, addr);
		return;
	}
	//fibo_camera_read_reg_cus(camPrev.camdev.pNamestr, 0xf0, value);
	OSI_PRINTFI("[%s %d] reg value 0x%X", __FUNCTION__, __LINE__, *value);
	
	fibo_thread_delete();;
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(prvThreadEntry, "mythread", 1024*8, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}

