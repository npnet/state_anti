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

typedef enum
{
    AUSTREAM_FORMAT_UNKNOWN, ///< placeholder for unknown format
    AUSTREAM_FORMAT_PCM,     ///< raw PCM data
    AUSTREAM_FORMAT_WAVPCM,  ///< WAV, PCM inside
    AUSTREAM_FORMAT_MP3,     ///< MP3
    AUSTREAM_FORMAT_AMRNB,   ///< AMR-NB
    AUSTREAM_FORMAT_AMRWB,   ///< AMR_WB
} auStreamFormat_t;
extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}


#define TEST_FILE_NAME  "/FFS/airply.mp3"
static uint8_t *Testbuff=NULL;

void Callplayend(void)
{
	OSI_LOGI(0, "audio helloworld.");
	if (Testbuff!=NULL)
	{
		free(Testbuff);
		Testbuff=NULL;
	}
}
static void StreamPlayTask(void *ctx)
{	
	//uint32_t Read=0;
	//uint32_t Write=0;
    int fd = vfs_open(TEST_FILE_NAME, O_RDONLY);
	if (fd <=0)
	{
		return;
	}
	OSI_LOGI(0, "audio file open");
    int32_t filesize=vfs_file_size(TEST_FILE_NAME);
	if (filesize<=0)
	{
		return;
	}
	OSI_LOGI(0, "audio file size=%d",filesize);
	//Write=filesize;	
    Testbuff = (uint8_t *)malloc((filesize+128)*sizeof(uint8_t));
	if (Testbuff==NULL)
	{
		return;
	}
	OSI_LOGI(0, "audio file buff is avalid");
	int bytes = vfs_read(fd, Testbuff, filesize);
	OSI_LOGI(0, "audio file read bytes=%d",bytes);
	if (bytes!=filesize)
	{
		return;
	}
    OSI_LOGI(0, "audio file fibo_audio_stream_play");
    vfs_close(fd);
    fibo_audio_mem_play(AUSTREAM_FORMAT_MP3, Testbuff, filesize, Callplayend);
    
    //fibo_audio_stream_play(AUSTREAM_FORMAT_PCM,buff,&Read,&Write,(filesize+128), 20);


    OSI_LOGD(0, "AT_CmdFunc_CAMTask osiThreadExit");
    osiThreadExit();

}

int audio_test(void *param)
{
    OSI_LOGI(0, "audio image enter, param");
    osiThreadCreate("audplay", StreamPlayTask, NULL, OSI_PRIORITY_NORMAL, 2048, 4);
    return 0;
}

void audio_tone_test(void)
{
	for (int i = 0; i < 22; i++)
	{
		OSI_LOGI(0, "audio_tone_test i = %d", i);
		fibo_tone_play(1, i, 1, 1);
		fibo_taskSleep(1000);

		fibo_tone_play(1, i, 5, 4);
		fibo_taskSleep(1000);
	}
	return;
}

void audio_acoustic_echo_canceler_test(void)
{
	OSI_LOGI(0, "audio_acoustic_echo_canceler_test start");

	char param[900] = {0};

	INT32 ret = 0;

	/*AT+CAVQE=0,0,2,
	"010000000100a600d61fcc0c3c00140001000f00010001000100ff7f00400000cc0cc57000000a000a000a003c0a00000100cc0c
	0020cc0c717d98790000000001004801050002003500ff7f9859050001003200ff7f3300ff7f0100500000005b014602"*/

	char param1[]="000000000100a600d61fcc0c3c00140001000f00010001000100ff7f00400000cc0cc57000000a000a000a003c0a00000100cc0c0020cc0c717d98790000000001004801050002003500ff7f9859050001003200ff7f3300ff7f0100500000005b014602";

	OSI_LOGI(0, "strlen %d", strlen(param1));
	
	ret = fibo_set_acoustic_echo_canceler(0, 0, 2, param1, strlen(param1));

	OSI_LOGI(0, "ret %d", ret);

	/*AT+CAVQE=0,0,3,""*/
	ret = fibo_get_acoustic_echo_canceler(0, 0, 3, param);

	OSI_LOGI(0, "ret1 %d, len/%d", ret, strlen(param));

	OSI_PRINTFI("param %s", param);

	OSI_LOGI(0, "audio_acoustic_echo_canceler_test end");
}

static void prvThreadEntry(void *param)
{
	fibo_taskSleep(10 *1000);
	audio_test(NULL);
	OSI_LOGI(0,"fibo_audio_play application thread enter, param 0x%x", param);
	INT8 amrtest[] = "happy.amr";
	INT8 mp3test[] = "airply.mp3";
	INT8 wavtest[] = "play.wav";
	INT32 ret;
	INT32 type = 1;

#if 0
    char fname[][128] = {
        "/FFS/0.mp3",
        "/FFS/1.mp3",
        "/FFS/2.mp3",
    };
    int32_t num = 0;

    num = sizeof(fname) / sizeof(fname[0]);

    fibo_audio_list_play(fname, num);
#endif

fibo_taskSleep(10 *1000);
	/* *.amr format audio file play and stop demo*/
	ret = fibo_audio_play(type,amrtest);
	osiDebugEvent(0x00001190);
	if(ret < 0)
	{
		osiDebugEvent(0x00001191);
		OSI_LOGI(0, "fibo_audio_play failed");
	}
	
	fibo_taskSleep(3 *1000);
	ret = fibo_audio_pause();
	if(ret < 0)
	{
		OSI_LOGI(0, "fibo_audio_pause failed");
		osiDebugEvent(0x00001199);
	}
	
	fibo_taskSleep(3 *1000);
	ret = fibo_audio_resume();
	if(ret < 0)
	{
		OSI_LOGI(0, "fibo_audio_resume failed");
		osiDebugEvent(0x00001100);
	}
	
	fibo_taskSleep(5 *1000);
	ret = fibo_audio_stop();
	if(ret < 0)
	{
		osiDebugEvent(0x00001192);
		OSI_LOGI(0, "fibo_audio_stop failed");
	}

	/* *.mp3 format audio file play and stop demo*/
	/* *.mp3 format audio file pause and resume please Refer to the above code*/
	fibo_taskSleep(3 *1000);
	ret = fibo_audio_play(type,mp3test);
	osiDebugEvent(0x00001193);
	if(ret < 0)
	{
		osiDebugEvent(0x00001194);
		OSI_LOGI(0, "fibo_audio_play failed");
	}
	
	fibo_taskSleep(3 *1000);
	ret = fibo_audio_stop();
	if(ret < 0)
	{
		osiDebugEvent(0x00001195);
		OSI_LOGI(0, "fibo_audio_stop failed");
	}

	/* *.wav format audio file play and stop demo*/
	/* *.mp3 format audio file pause and resume please Refer to the above code*/
	fibo_taskSleep(3 *1000);
	ret = fibo_audio_play(type,wavtest);
	osiDebugEvent(0x00001196);
	if(ret < 0)
	{
		osiDebugEvent(0x00001197);
		OSI_LOGI(0, "fibo_audio_play failed");
	}
	
	fibo_taskSleep(400);
	ret = fibo_audio_stop();
	if(ret < 0)
	{
		osiDebugEvent(0x00001198);
		OSI_LOGI(0, "fibo_audio_stop failed");
	}
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
