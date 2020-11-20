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

typedef struct {
    auStreamFormat_t format;
    uint8_t *audio_buff;
    uint32_t Wp;
    int32_t fd;
    int32_t size;
}RecordContext_t;
static RecordContext_t grecordctx = {};

static int RmAudiofile(const char *filename)
{
    int lRet=0;
    char file_fold[8] = "/audio/";

    DIR *dir = vfs_opendir(file_fold);
    if (dir != NULL)
    {
        struct dirent *ent;
        while ((ent = vfs_readdir(dir)) != NULL)
        {
            if (ent->d_type == DT_REG)
            {
                if (NULL != strstr(ent->d_name,filename))
                {
                    //sprintf(path,"%s/%s",gAtAudioDir,ent->d_name);
                    vfs_unlink(filename);
                    lRet=1;
                    break;
                }
            }
        }
        vfs_closedir(dir);
    }
    else
    {
        return (-2);
    }
    OSI_LOGI(0, "audio recorder delete exist file %d .", lRet);
    return lRet;
}


static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

    RecordContext_t *d = &grecordctx;
    d->format = AUSTREAM_FORMAT_PCM;
    d->size  =4096;
    d->audio_buff = NULL;
    d->Wp = 0;
    d->fd = 0;

    int ret = 0;
    int32_t leftspace = 0;
    int32_t Rp_pos = 0;

    //just for test
    int32_t filesize = 0;
    int32_t max_len = 4096*3;

    int test_count = 5;
    while (test_count)
    {
        bool audio_status = fibo_get_audio_status();
        if (audio_status)
        {
            OSI_LOGI(0, "audio busy now.");
            return;
        }

        /* to save record to audio_1.pcm */
        char Filename[32] = "/audio/audio_1.pcm";

        d->audio_buff = (uint8_t *)malloc(4096);

        // add fs protect,when left 200KB size, can't start record.
        struct statvfs fst;
        if (vfs_statvfs("/", &fst) == 0)
        {
            uint32_t availsize = fst.f_bavail * fst.f_bsize;
            if (availsize < (1024 * 100)*2)
            {
                OSI_LOGI(0, "audio record don't have enough memory size, availsize=%d.",availsize);
                return ;
            }
            else
            {
                OSI_LOGI(0, "audio record memory size is enough, availsize=%d.",availsize);
            }
        }

        //rm exist file
        if (0 > RmAudiofile(Filename))
        {
            OSI_LOGI(0, "delete file error.");
        }

        /* create new file to save record */
        d->fd = vfs_open(Filename, O_RDWR | O_CREAT | O_TRUNC);
        if (!d->fd)
        {
            OSI_LOGI(0, "create file failed");
        }

        ret = fibo_audio_recorder_stream_start(d->format, d->audio_buff, &(d->Wp), d->size, 5);

        if (ret < 0)
        {
            OSI_LOGI(0, "audio recorder stream start error %d", ret);

            fibo_audio_recorder_stream_stop();

            free(d->audio_buff);

            d->audio_buff = NULL;
            d->Wp = 0;

            return ;
        }

        OSI_LOGI(0, "audio recorder stream start success");

        while (1)
        {
            if(Rp_pos != d->Wp)
            {
                filesize = vfs_file_size(Filename);
                if (filesize >= max_len)
                {
                    fibo_audio_recorder_stream_stop();

                    free(d->audio_buff);
                    d->audio_buff = NULL;

                    d->Wp = 0;
                    vfs_close(d->fd);
                    break;
                }

                OSI_LOGI(0, "audio recorder start to save pcm");
                if (Rp_pos < d->Wp)
                {
                    OSI_LOGI(0, "audio recorder read position < write");
                    vfs_write(d->fd, &(d->audio_buff[Rp_pos]), (d->Wp - Rp_pos - 1));
                    Rp_pos = d->Wp;
                }
                else
                {
                    OSI_LOGI(0, "audio recorder read position > write");
                    leftspace = d->size - Rp_pos;
                    OSI_LOGI(0, "audio recorder leftspace %d", leftspace);
                    vfs_write(d->fd, &(d->audio_buff[Rp_pos]), leftspace);
                    Rp_pos = 0;
                    vfs_write(d->fd, &(d->audio_buff[Rp_pos]), (d->Wp - Rp_pos - 1));
                }
            }
            else
            {
                OSI_LOGI(0, "audio recorder,There is no pcm data from mic,wait moment.");
                osiThreadSleep(2);
                continue;
            }
        }
        OSI_LOGI(0, "audio recorder test count %d\n", test_count);
        test_count--;
        osiThreadSleep(200);
    }
    fibo_thread_delete();
}

void * appimg_enter(void *param)
{
    OSI_LOGI(0, "recorder application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024*4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
