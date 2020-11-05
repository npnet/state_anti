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

static void prvInvokeGlobalCtors(void)
{
    extern void (*__init_array_start[])();
    extern void (*__init_array_end[])();

    size_t count = __init_array_end - __init_array_start;
    for (size_t i = 0; i < count; ++i)
        __init_array_start[i]();
}

#define TEST_CA_FILE "-----BEGIN CERTIFICATE-----\r\n"                                      \
                     "MIID9TCCAt2gAwIBAgIJAOEIwHHcR9K7MA0GCSqGSIb3DQEBBQUAMIGPMQswCQYD\r\n" \
                     "VQQGEwJDTjEPMA0GA1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQK\r\n" \
                     "DAdmaWJvY29tMRAwDgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0\r\n" \
                     "LjM2MSQwIgYJKoZIhvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wIBcNMTkw\r\n" \
                     "ODMwMDY1MjUwWhgPMjExOTA4MDYwNjUyNTBaMIGPMQswCQYDVQQGEwJDTjEPMA0G\r\n" \
                     "A1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQKDAdmaWJvY29tMRAw\r\n" \
                     "DgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0LjM2MSQwIgYJKoZI\r\n" \
                     "hvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\r\n" \
                     "A4IBDwAwggEKAoIBAQC6UMQfxHL0oW9pY1cGvq5QPdw8OU7dX2YsCbPdEiXePKce\r\n" \
                     "E6AN3IKqOuZhEd1iIypXG2AywzIu9bd5w1d4COjjSC/Tpf2AKYw+jqfxHsQAvSKt\r\n" \
                     "Rvwp1wrS5IvWy8yEG9lNpyVJHBUWlVpU/tUf02MYYU5xUBS+mJE9Tc10j7kd/uV7\r\n" \
                     "aEfM0pYhm7VmHPWDHXeXj3LKYigjttNxUgpDh2UVpq6ejzzHA5T/k2+XtKtWu7Pb\r\n" \
                     "ag6lONzz6Zxya9htVLBy7I4uTFrcRPxNgc/KF2BuwEVc4rqGUZ4vpVdwmCyKGIua\r\n" \
                     "fvit1nsvnhvYMu01HhWuK6e3IO6hOpeyR1wk75ofAgMBAAGjUDBOMB0GA1UdDgQW\r\n" \
                     "BBTT9RodyqsY/C2WS/7k8GFWidQrlTAfBgNVHSMEGDAWgBTT9RodyqsY/C2WS/7k\r\n" \
                     "8GFWidQrlTAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUAA4IBAQCkg9dUYBRs\r\n" \
                     "uqCz71Q75B2n768icIeMfQmf969fNVK/mwaUnFxjqq+4Xw3zADdUdnTZ8FEfjAt2\r\n" \
                     "LQaxmsiGlM3KmUhXy/k1xKypIu2KecxEX/NqdG02SYcBmrIAP6ZxOxyyJZXbPRBt\r\n" \
                     "11W3e9+MsRFjRNDxvi5xPcBTu7padUXf7gZp/U8RTc9r0RzsTJu0oFx1Vl6B9m9Z\r\n" \
                     "4Ae7EshqUrGbnQMJ9XinPVMhuPB4UTc5H9F9ZEswkd/8fK1kXE2aD9LOUD3ITpfH\r\n" \
                     "h4UBb/UX3VY2eoLC6T5FzPggAcyxU/S2svZaq2+fSWvA7WpEYmTvzQTeT+y1BaUW\r\n" \
                     "9SoOHidKUkQe\r\n"                                                     \
                     "-----END CERTIFICATE-----\r\n"

#define TEST_CLIENT_CRT_FILE "-----BEGIN CERTIFICATE-----\r\n"                                      \
                             "MIID9TCCAt2gAwIBAgIJAOEIwHHcR9K7MA0GCSqGSIb3DQEBBQUAMIGPMQswCQYD\r\n" \
                             "VQQGEwJDTjEPMA0GA1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQK\r\n" \
                             "DAdmaWJvY29tMRAwDgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0\r\n" \
                             "LjM2MSQwIgYJKoZIhvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wIBcNMTkw\r\n" \
                             "ODMwMDY1MjUwWhgPMjExOTA4MDYwNjUyNTBaMIGPMQswCQYDVQQGEwJDTjEPMA0G\r\n" \
                             "A1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQKDAdmaWJvY29tMRAw\r\n" \
                             "DgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0LjM2MSQwIgYJKoZI\r\n" \
                             "hvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\r\n" \
                             "A4IBDwAwggEKAoIBAQC6UMQfxHL0oW9pY1cGvq5QPdw8OU7dX2YsCbPdEiXePKce\r\n" \
                             "E6AN3IKqOuZhEd1iIypXG2AywzIu9bd5w1d4COjjSC/Tpf2AKYw+jqfxHsQAvSKt\r\n" \
                             "Rvwp1wrS5IvWy8yEG9lNpyVJHBUWlVpU/tUf02MYYU5xUBS+mJE9Tc10j7kd/uV7\r\n" \
                             "aEfM0pYhm7VmHPWDHXeXj3LKYigjttNxUgpDh2UVpq6ejzzHA5T/k2+XtKtWu7Pb\r\n" \
                             "ag6lONzz6Zxya9htVLBy7I4uTFrcRPxNgc/KF2BuwEVc4rqGUZ4vpVdwmCyKGIua\r\n" \
                             "fvit1nsvnhvYMu01HhWuK6e3IO6hOpeyR1wk75ofAgMBAAGjUDBOMB0GA1UdDgQW\r\n" \
                             "BBTT9RodyqsY/C2WS/7k8GFWidQrlTAfBgNVHSMEGDAWgBTT9RodyqsY/C2WS/7k\r\n" \
                             "8GFWidQrlTAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUAA4IBAQCkg9dUYBRs\r\n" \
                             "uqCz71Q75B2n768icIeMfQmf969fNVK/mwaUnFxjqq+4Xw3zADdUdnTZ8FEfjAt2\r\n" \
                             "LQaxmsiGlM3KmUhXy/k1xKypIu2KecxEX/NqdG02SYcBmrIAP6ZxOxyyJZXbPRBt\r\n" \
                             "11W3e9+MsRFjRNDxvi5xPcBTu7padUXf7gZp/U8RTc9r0RzsTJu0oFx1Vl6B9m9Z\r\n" \
                             "4Ae7EshqUrGbnQMJ9XinPVMhuPB4UTc5H9F9ZEswkd/8fK1kXE2aD9LOUD3ITpfH\r\n" \
                             "h4UBb/UX3VY2eoLC6T5FzPggAcyxU/S2svZaq2+fSWvA7WpEYmTvzQTeT+y1BaUW\r\n" \
                             "9SoOHidKUkQe\r\n"                                                     \
                             "-----END CERTIFICATE-----\r\n"

#define TEST_CLIENT_KEY_FILE "-----BEGIN CERTIFICATE-----\r\n"                                      \
                             "MIID9TCCAt2gAwIBAgIJAOEIwHHcR9K7MA0GCSqGSIb3DQEBBQUAMIGPMQswCQYD\r\n" \
                             "VQQGEwJDTjEPMA0GA1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQK\r\n" \
                             "DAdmaWJvY29tMRAwDgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0\r\n" \
                             "LjM2MSQwIgYJKoZIhvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wIBcNMTkw\r\n" \
                             "ODMwMDY1MjUwWhgPMjExOTA4MDYwNjUyNTBaMIGPMQswCQYDVQQGEwJDTjEPMA0G\r\n" \
                             "A1UECAwGc2hhbnhpMQ0wCwYDVQQHDAR4aWFuMRAwDgYDVQQKDAdmaWJvY29tMRAw\r\n" \
                             "DgYDVQQLDAdmaWJvY29tMRYwFAYDVQQDDA00Ny4xMTAuMjM0LjM2MSQwIgYJKoZI\r\n" \
                             "hvcNAQkBFhV2YW5zLndhbmdAZmlib2NvbS5jb20wggEiMA0GCSqGSIb3DQEBAQUA\r\n" \
                             "A4IBDwAwggEKAoIBAQC6UMQfxHL0oW9pY1cGvq5QPdw8OU7dX2YsCbPdEiXePKce\r\n" \
                             "E6AN3IKqOuZhEd1iIypXG2AywzIu9bd5w1d4COjjSC/Tpf2AKYw+jqfxHsQAvSKt\r\n" \
                             "Rvwp1wrS5IvWy8yEG9lNpyVJHBUWlVpU/tUf02MYYU5xUBS+mJE9Tc10j7kd/uV7\r\n" \
                             "aEfM0pYhm7VmHPWDHXeXj3LKYigjttNxUgpDh2UVpq6ejzzHA5T/k2+XtKtWu7Pb\r\n" \
                             "ag6lONzz6Zxya9htVLBy7I4uTFrcRPxNgc/KF2BuwEVc4rqGUZ4vpVdwmCyKGIua\r\n" \
                             "fvit1nsvnhvYMu01HhWuK6e3IO6hOpeyR1wk75ofAgMBAAGjUDBOMB0GA1UdDgQW\r\n" \
                             "BBTT9RodyqsY/C2WS/7k8GFWidQrlTAfBgNVHSMEGDAWgBTT9RodyqsY/C2WS/7k\r\n" \
                             "8GFWidQrlTAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUAA4IBAQCkg9dUYBRs\r\n" \
                             "uqCz71Q75B2n768icIeMfQmf969fNVK/mwaUnFxjqq+4Xw3zADdUdnTZ8FEfjAt2\r\n" \
                             "LQaxmsiGlM3KmUhXy/k1xKypIu2KecxEX/NqdG02SYcBmrIAP6ZxOxyyJZXbPRBt\r\n" \
                             "11W3e9+MsRFjRNDxvi5xPcBTu7padUXf7gZp/U8RTc9r0RzsTJu0oFx1Vl6B9m9Z\r\n" \
                             "4Ae7EshqUrGbnQMJ9XinPVMhuPB4UTc5H9F9ZEswkd/8fK1kXE2aD9LOUD3ITpfH\r\n" \
                             "h4UBb/UX3VY2eoLC6T5FzPggAcyxU/S2svZaq2+fSWvA7WpEYmTvzQTeT+y1BaUW\r\n" \
                             "9SoOHidKUkQe\r\n"                                                     \
                             "-----END CERTIFICATE-----\r\n"

static void prvThreadEntry(void *param)
{
    OSI_LOGI(0, "application thread enter, param 0x%x", param);
    //srand(100);

    //如果需要验证服务器的证书，将这个值设置为１，否则设置为０
    fibo_set_ssl_chkmode(1);

    //如果上一步（fibo_set_ssl_chkmode）的值设置为１，则这里必须加载自己信任的ca证书，否则会连接失败
    fibo_write_ssl_file("TRUSTFILE", TEST_CA_FILE, sizeof(TEST_CA_FILE) - 1);

    //如果服务器也需要验证客户端，那么这里需要加载客户端的证书和私钥，否则会连接失败
    fibo_write_ssl_file("CAFILE", TEST_CLIENT_CRT_FILE, sizeof(TEST_CLIENT_CRT_FILE) - 1);
    fibo_write_ssl_file("CAKEY", TEST_CLIENT_KEY_FILE, sizeof(TEST_CLIENT_KEY_FILE) - 1);

    fibo_taskSleep(10000);

    UINT8 buf[32] = {0};
    UINT8 p[] = "xxxxxxxxxxxtest ssl";
    INT32 sock = fibo_ssl_sock_create();
    if (sock == -1)
    {
        OSI_LOGI(0, "create ssl sock failed");
        fibo_thread_delete();
    }
    OSI_LOGI(0, "fibossl sys_sock_connect %d", sock);
    int ret = fibo_ssl_sock_connect(sock, "47.110.234.36", 8887);
    OSI_LOGI(0, "fibossl sys_sock_connect %d", ret);

    ret = fibo_ssl_sock_send(sock, p, sizeof(p) - 1);
    OSI_LOGI(0, "fibossl sys_sock_send %d", ret);

    ret = fibo_ssl_sock_recv(sock, buf, sizeof(buf));
    OSI_LOGI(0, "fibossl sys_sock_recv %d", ret);
    if (ret > 0)
    {
        // buf[ret] = 0;
        OSI_LOGI(0, "fibossl sys_sock_send %s", (char *)buf);
    }

    test_printf();
    fibo_thread_delete();
}

void *appimg_enter(void *param)
{
    OSI_LOGI(0, "application image enter, param 0x%x", param);

    prvInvokeGlobalCtors();

    fibo_thread_create(prvThreadEntry, "mythread", 1024 * 4, NULL, OSI_PRIORITY_NORMAL);
    return 0;
}

void appimg_exit(void)
{
    OSI_LOGI(0, "application image exit");
}
