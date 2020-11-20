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


#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "fibo_opencpu.h"

extern void test_printf(void);

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

char xml_file[]={"<?xml version=\"1.0\" encoding=\"UTF-8\"?><root><dev nPclType=\"IEC104\" nByteOrder=\"2\" nDevModel=\"EM000\"><point nDevIndex=\"1\" nDevType=\"25\" nDevModel=\"NEWSIV\"/><fast>testx</fast></dev></root>"};
char xmal_value[200] = {0};

void start_func(void *userData, const XML_Char *name, const XML_Char **atts)
{
    int i;
    char *data = (char *)userData;

    OSI_PRINTFI("start name = %s \n",(char *)name);
    for(i = 0;atts[i];i+=2)
    {
        OSI_PRINTFI("%s = %s \n",(char *)atts[i],(char *)atts[i+1]);
    }
    OSI_PRINTFI("start name over!\n");
}

void end_func(void *userData, const XML_Char *name)
{
    char *data = (char *)userData;
    OSI_PRINTFI("end name = %s \n",(char *)name);
}

void deal_data(void *userData, const XML_Char *s, int len)
{
    char *data = userData;
    char *buff;
    OSI_PRINTFI("deal data!\n");
    OSI_PRINTFI("all data = %s \n",(char *)s);
    buff = (char *)malloc(len+1);
    if(buff)
    {
        memset(buff,0,len+1);
        memcpy(buff,s,len);
        OSI_PRINTFI("we need data = %s\n",buff);
        free(buff);
    }
    OSI_PRINTFI("deal data over! \n");
}

void xml_test()
{
    XML_Parser parser = fibo_xml_parser_create(NULL);
    if (parser == NULL)
    {
        OSI_LOGI(0, "fibo_xml_parser_create error\n");
    }

    fibo_xml_set_user_data(parser, xmal_value);
    fibo_xml_set_element_handle(parser, start_func, end_func);
    fibo_xml_set_character_data_handler(parser, deal_data);

    if(fibo_xml_parse(parser, xml_file, strlen(xml_file), XML_TRUE) == XML_STATUS_ERROR)
    {
        OSI_LOGI(0, "fibo_xml_parse error\n");
    }
    
    fibo_xml_parser_free(parser);
}

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);

    while(1)
    {
        xml_test();
        fibo_taskSleep(3000);
    }
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

