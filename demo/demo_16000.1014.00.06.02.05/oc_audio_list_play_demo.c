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
#include "mutil_app_common.h"
#include "fibo_opencpu_comm.h"
#include "osi_log.h"
const char f_1_name[][128]  = {
    "/FFS/alipay.amr",
    "/FFS/collection.amr",
};

//const char f_2_name[][128]  = {
//    "/FFS/1.mp3",
//    "/FFS/13.mp3",
//    "/FFS/16.mp3",
//};
const char f_2_name[][128]  = {
    "/FFS/9.mp3",
    "/FFS/billion.mp3",
    "/FFS/9.mp3",
    "/FFS/thousand.mp3",
    "/FFS/9.mp3",
    "/FFS/hundred.mp3",
    "/FFS/9.mp3",
    "/FFS/10.mp3",
    "/FFS/9.mp3",
    "/FFS/tenthousand.mp3",
    "/FFS/9.mp3",
    "/FFS/hundred.mp3",
    "/FFS/9.mp3",
    "/FFS/10.mp3",
    "/FFS/9.mp3",
    "/FFS/dot.mp3",
    "/FFS/9.mp3",
    "/FFS/9.mp3",
    "/FFS/16.mp3",
};

const char f_3_name[][128]  = {
    "/FFS/thanks.amr",
    "/FFS/presence.amr",
};

int num_1 = 0;
int num_2 = 0;
int num_3 = 0;
int count = 0;
#if 0
static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
#if 0
    const char fname[][128] = {
        "/FFS/0.amr",
        "/FFS/1.amr",
        "/FFS/2.amr",
        "/FFS/3.amr",
    };
#else
    //const char f_1_name[][128]  = {
    //    "/FFS/alipay.amr",
    //    "/FFS/collection.amr",
    //};
    //const char f_2_name[][128]  = {
    //    "/FFS/1.mp3",
    //    "/FFS/13.mp3",
    //    "/FFS/16.mp3",
    //};

    //const char f_2_name[][128] = {
    //    "/FFS/0.mp3",
    //    "/FFS/1.mp3",
    //    "/FFS/2.mp3",
    //    "/FFS/3.mp3",
    //    "/FFS/4.mp3",
    //    "/FFS/5.mp3",
    //    "/FFS/6.mp3",
    //    "/FFS/7.mp3",
    //    "/FFS/8.mp3",
    //    "/FFS/9.mp3",
    //    "/FFS/10.mp3",
    //    "/FFS/11.mp3",
    //    "/FFS/12.mp3",
    //    "/FFS/13.mp3",
    //    "/FFS/14.mp3",
    //    "/FFS/15.mp3",
    //    "/FFS/16.mp3",
    //};

#endif

    num_1 = sizeof(f_1_name) / sizeof(f_1_name[0]);
    fibo_audio_list_play(f_1_name, num_1);

    osiThreadSleep(200);
    fibo_thread_delete();
}
#endif
static void sig_recv_callback(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    //osiThreadSleep(150);
    switch (sig)
    {
        case GAPP_SIG_AUDIO_LIST_PLAY_END:
            {
                //osiThreadSleep(100);
                if (count >= 2)
                {
                    break;
                }
                if (count == 0) {
                    num_2  = sizeof(f_2_name) / sizeof(f_2_name[0]);
                    if (!fibo_get_audio_status()) {
                        fibo_audio_list_play(f_2_name, num_2);
                        count++;
                        OSI_LOGI(0, "audio mp3 list play end");
                    }
                }

                if (count == 1) {
                    num_3  = sizeof(f_3_name) / sizeof(f_3_name[0]);
                    if (!fibo_get_audio_status()) {
                        fibo_audio_list_play(f_3_name, num_3);
                        count++;
                        OSI_LOGI(0, "audio last list play end");
                    }
                }
            }
            break;

        default:
            break;
    }
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = sig_recv_callback
};

static void prvThreadEntry(void *param)
{
    osiThreadSleep(5000);
    int lRet = -1;

    num_1 = sizeof(f_1_name) / sizeof(f_1_name[0]);
    lRet = fibo_audio_list_play(f_1_name, num_1);

    num_2 = sizeof(f_2_name) / sizeof(f_2_name[0]);
    while (1) {
        if (!fibo_get_audio_status()) {
            lRet = fibo_audio_list_play(f_2_name, num_2);
            if(lRet == 0)
                break;
        }
    }

    num_3 = sizeof(f_3_name) / sizeof(f_3_name[0]);
    while (1) {
        if(!fibo_get_audio_status()) {
            lRet = fibo_audio_list_play(f_3_name, num_3);
            if(lRet == 0)
                break;
        }
    }

    osiThreadSleep(2000);
    fibo_thread_delete();
}
void * appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    //return (void *)&user_callback;
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
