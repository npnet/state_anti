uint8_t g_pcm_buff[320*55*2] = {0};
uint32_t pcm_len = 0;
uint32_t Wp = 0;
uint32_t Rp = 0;

static void UpdateAudio(void *param)
{
	int32_t fd = 0;
	int32_t bytes = 0;
	uint8_t *Audio_data = NULL;
	uint32_t i = 0;
	char FileName[] = "/FFS/test.pcm";

	fd = vfs_open(FileName, O_RDONLY);
	if (fd < 0)
	{
		OSI_PRINTFI("[%s %d] open %s error", __FUNCTION__, __LINE__, FileName);
		return;
	}
	bytes = vfs_file_size(FileName);
	OSI_PRINTFI("[%s %d] open %s success,len %d", __FUNCTION__, __LINE__, FileName, bytes);
	
	Audio_data = (uint8_t *)malloc(bytes);
	if (Audio_data == NULL)
	{
		OSI_PRINTFI("[%s %d] malloc fail", __FUNCTION__, __LINE__);
		return;
	}
	
	vfs_read(fd, Audio_data, bytes);
	vfs_close(fd);
	
	memset(g_pcm_buff, 0, sizeof(g_pcm_buff));

	for(i = 0; i < 3; i++) //test
	{		
		memcpy(&g_pcm_buff[Wp], Audio_data, bytes);
		Wp += bytes;
		while(1)
		{
			osiThreadSleep(20);
			if (Rp == Wp)
			{	Rp = 0;			
				break;
			}		
		}
		osiThreadSleep(50);
	}

	OSI_PRINTFI("[%s %d] write data %d", __FUNCTION__, __LINE__, pcm_len);
	osiThreadExit();
}

static void AudioPlay(void *param)
{

	fibo_audio_stream_play(AUSTREAM_FORMAT_PCM, g_pcm_buff, &Rp, &Wp, sizeof(g_pcm_buff), 300);

	while (1)
	{
		osiThreadSleep(100);
	}
	osiThreadExit();
}

int stream_play(void *param)
{
	OSI_PRINTFI("Entry [%s %d]", __FUNCTION__, __LINE__);

	osiThreadCreate("stream_play", AudioPlay, NULL, OSI_PRIORITY_NORMAL, 1024 * 10, 4);

	osiThreadCreate("stream_play", UpdateAudio, NULL, OSI_PRIORITY_NORMAL, 1024 * 10, 4);

	return 0;
}