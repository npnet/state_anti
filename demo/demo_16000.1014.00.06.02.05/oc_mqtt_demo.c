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

#define OSI_LOG_TAG OSI_MAKE_LOG_TAG('M', 'Q', 'T', 'T')

#include "fibo_opencpu.h"
#include "oc_mqtt.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

static UINT32 g_lock = 0;

INT8 client_id[] = "lens_0ptfvqWWCzblt9phGtddgOdO3z3";
INT8 host[] = "117.22.252.78";
UINT16 port = 8885;
UINT8 clear_session = 0;
UINT16 keepalive = 10;
UINT8 UseTls = 1;
BOOL run = false;
BOOL quit = true;

static void prvThreadEntry(void *param);

static void create_mqtt_connect()
{
    run = false;
    while (!quit)
    {
        fibo_taskSleep(1000);
    }
    fibo_thread_create(prvThreadEntry, "mqtt-thread", 1024 * 16, NULL, OSI_PRIORITY_NORMAL);
}

void user_signal_process(GAPP_SIGNAL_ID_T sig, va_list arg)
{
    OSI_PRINTFI("mqttapi RECV SIGNAL:%d", sig);
    char *topic;
    int8_t qos;
    char *message;
    uint32_t len;
    int ret = 1;
    switch (sig)
    {
    case GAPP_SIG_CONNECT_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi connect :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        if (ret != 1)
        {
            create_mqtt_connect();
        }
        break;
    case GAPP_SIG_CLOSE_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi close :%s", ret ? "ok" : "fail");
        create_mqtt_connect();
        break;
    case GAPP_SIG_SUB_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi sub :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_UNSUB_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi unsub :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_PUB_RSP:
        ret = va_arg(arg, int);
        OSI_PRINTFI("mqttapi pub :%s", ret ? "ok" : "fail");
        fibo_sem_signal(g_lock);
        break;
    case GAPP_SIG_INCOMING_DATA_RSP:
        //gapp_dispatch(GAPP_SIG_INCOMING_DATA_RSP, pub_msg->topic, pub_msg->qos, pub_msg->message, pub_msg->recv_len);
        topic = va_arg(arg, char *);
        qos = va_arg(arg, int);
        message = va_arg(arg, char *);
        len = va_arg(arg, uint32_t);
        OSI_PRINTFI("mqttapi recv message :topic:%s, qos=%d message=%s len=%d", topic, qos, message, len);
        break;
    default:
        break;
    }
}

static FIBO_CALLBACK_T user_callback = {
    .fibo_signal = user_signal_process};

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    int ret = 0;

    int test = 1;
    OSI_PRINTFI("mqttapi wait network");
    reg_info_t reg_info;
    quit = false;
    run = true;

    g_lock = fibo_sem_new(1);
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

    fibo_taskSleep(1 * 1000);
    OSI_PRINTFI("mqtt [%s-%d]", __FUNCTION__, __LINE__);
    fibo_mqtt_set("test", "123");
    fibo_set_ssl_chkmode(0);

    fibo_sem_wait(g_lock);
    do
    {
        ret = fibo_mqtt_connect(client_id, host, port, 1, keepalive, UseTls);
        fibo_taskSleep(2000);
    } while (ret < 0);

    OSI_PRINTFI("mqttapi connect finish");

    fibo_sem_wait(g_lock);
    OSI_PRINTFI("mqttapi start sub topic");
    ret = fibo_mqtt_sub("topic1", 1);
    if (ret < 0)
    {
        OSI_PRINTFI("mqttapi sub failed");
    }

    while (run)
    {
        fibo_taskSleep(1 * 1000);
    }
    quit = true;
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);
    create_mqtt_connect();
    return &user_callback;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
