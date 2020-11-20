#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Y', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static void prvThreadEntry(void *param)
{
    uint8_t id[4] = {0};
    int32_t num = 0;
    lcd_reg_t lcd_regs[] = {
        {0x11, 0, NULL},
        {0xFF, 1, (uint8_t *)"\x78"},
        {0xB2, 5, (uint8_t *)"\x0C\x0C\x00\x33\x33"},
        {0x35, 1, (uint8_t *)"\x00"},
        {0x36, 1, (uint8_t *)"\x00"},
        {0x3A, 1, (uint8_t *)"\x05"},
    };

    num = sizeof(lcd_regs) / sizeof(lcd_reg_t);

    if (0 != fibo_lcd_init())
    {
        OSI_PRINTFI("[%s %d] lcd init fail ", __FUNCTION__, __LINE__);
    }

    fibo_lcd_regs_init_cus(lcd_regs, num);

    fibo_lcd_write_reg_cus(0xB2, (uint8_t *)("\x0C\x0C\x00\x33\x33"), 5);

    fibo_lcd_read_reg_cus(0x04, id, 4);

    OSI_PRINTFI("[%s %d] lcd id 0x%02X 0x%02X 0x%02X 0x%02X", __FUNCTION__, __LINE__, id[3], id[2], id[1], id[0]);

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

