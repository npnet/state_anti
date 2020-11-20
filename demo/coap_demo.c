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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('C', 'O', 'A', 'P')

#include "fibo_opencpu.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "oc_coap.h"

extern void test_printf(void);

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

    int test = 1;
    OSI_PRINTFI("coapapi wait network");
    reg_info_t reg_info;

    while (test)
    {
        fibo_getRegInfo(&reg_info, 0);
        fibo_taskSleep(1000);
        OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        if (reg_info.nStatus == 1)
        {
            test = 0;
            fibo_PDPActive(1, NULL, NULL, NULL, 0, 0, NULL);
            fibo_taskSleep(1000);
            OSI_PRINTFI("[%s-%d]", __FUNCTION__, __LINE__);
        }
    }

    fibo_taskSleep(5 * 1000);

    INT8 token[] = "f_token"; // < 8 bytes
    UINT8 payload[] = "demo_payload";

    //如果是请求消息，这个必须设置为0
    UINT8 code_class = 0;

    //如果是请求，这个表示method，如果是回应消息，表示回应code detail
    fibo_coap_method_t method_code = FIBOCOM_COAP_GET;

    //打开一个coap客户端
    void *h = fibo_coap_open("coap.me", 5683, 0);

    //创建一个coap消息
    void *m = fibo_coap_message_create(1, FIBOCOM_COAP_MSG_CON, code_class, method_code, payload, sizeof(payload), token, sizeof(token) - 1);

    //添加uri option
    fibo_coap_message_add_uri(m, "/fibocom/test/haha?a=b");

    //添加其他option
    fibo_coap_message_add_option(m, FIBO_OPTIONS_ETAG, "etag", sizeof("etag") - 1);

    //发送消息
    fibo_coap_send(h, m);
    //消息不在使用，销毁
    fibo_coap_message_destroy(m);

    INT16 recv_msg_id = 0;
    fibo_copa_type_t recv_msg_type = 0;
    fibo_coap_method_t recv_code = 0;
    const UINT8 *recv_payload = NULL;
    INT32 payload_len = 0;
    const INT8 *recv_token = NULL;
    INT32 recv_token_len = 0;
    UINT8 recv_code_class = 0;
    // const INT8 *recv_option = NULL;
    // INT32 recv_option_len = 0;

    //接收消息
    OSI_PRINTFI("recv data start\n");
    void *recv_msg = fibo_coap_recv(h);
    OSI_PRINTFI("recv data finish\n");
    if (recv_msg != NULL)
    {
        char log_buff[1024] = {0};
        int offset = 0;
        //获取消息的信息
        fibo_coap_message_get_info(recv_msg, &recv_msg_id, &recv_msg_type, &recv_code_class, &recv_code, &recv_payload, &payload_len, &recv_token, &recv_token_len);
        // fibo_coap_message_get_option(recv_msg, FIBO_OPTIONS_ETAG, &recv_option, &recv_option_len);

        offset += snprintf(log_buff + offset, sizeof(log_buff) - offset, "msg_id:%d recv_msg_type=%d code_class=%d recv_code=%d ", recv_msg_id, recv_msg_type, recv_code_class, recv_code);
        offset += snprintf(log_buff + offset, sizeof(log_buff) - offset, "payload:%.*s ", payload_len, recv_payload);
        offset += snprintf(log_buff + offset, sizeof(log_buff) - offset, "token:%.*s ", recv_token_len, recv_token);
        // printf("recv_option:%.*s\n", recv_option_len, recv_option);
        OSI_PRINTFI("%s", log_buff);

        //使用完成之后必须销毁
        fibo_coap_message_destroy(recv_msg);
    }
    else
    {
        OSI_PRINTFI("recv data fail\n");
    }

    //关闭copa客户端
    fibo_coap_close(h);
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread-coap", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
