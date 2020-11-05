#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#ifdef _PLATFORM_L610_
#include "4G_net.h"
#include "eybapp_appTask.h"
// #include "Device.h"
// #include "eybond.h"
#endif

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter");
    prvInvokeGlobalCtors();

    UINT32 net_thread_id = 0;
    UINT32 app_thread_id = 0;

    fibo_thread_create_ex(proc_net_task,    "Eybond NET TASK",    1024*8*2, NULL, OSI_PRIORITY_NORMAL, &net_thread_id);
    fibo_thread_create_ex(proc_app_task,    "Eybond APP TASK",    1024*8*2, NULL, OSI_PRIORITY_NORMAL, &app_thread_id);
//    fibo_thread_create(proc_device_task, "Eybond DEVICE TASK", 1024*8*2, NULL, OSI_PRIORITY_NORMAL);
//    fibo_thread_create(proc_eybond_task, "Eybond CMD TASK",    1024*8*2, NULL, OSI_PRIORITY_NORMAL);
//    fibo_thread_create(device_update_task,"DEVICE UPDATE TASK",1024*8*3, NULL, OSI_PRIORITY_NORMAL);
//    fibo_thread_create(feed_dog_task,     "FEED DOG TASK",     1024*8*2, NULL, OSI_PRIORITY_NORMAL);
//    fibo_thread_create(rec_check_task,    "REC  CHECK TASK",   1024*8*2, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}