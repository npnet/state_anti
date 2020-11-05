/********************** COPYRIGHT 2014-2100, eybond ***************************
 * @File    : parameter_number_and_value.c
 * @Author  :
 * @Date    : 2020-08-20
 * @Brief   :
 ******************************************************************************/
#include "ql_fs.h"
#include "ql_error.h"
#include "ql_stdlib.h"
#include "ql_memory.h"

#include "eyblib_typedef.h"
#include "eyblib_swap.h"
#include "eyblib_HashMD5.h"
// #include "eyblib_r_stdlib.h" // mike 20200828
// #include "eyblib_memory.h"

#include "parameter_number_and_value.h"
#include "data_collector_parameter_table.h"
#include "struct_type.h"

#include "eybpub_Debug.h"
#include "eybpub_SysPara_File.h"

#ifndef __TEST_FOR_UFS__
#define __TEST_FOR_UFS__
#endif

/*----------------------------------------------------------------------------*
 *                             Global Vars                                    *
 *----------------------------------------------------------------------------*/
// extern int number_of_array_elements;

/*----------------------------------------------------------------------------*
 *                             Local Vars                                     *
 *----------------------------------------------------------------------------*/
#define g_recName_parameter_a  "paraconfig_file_a.ini"  // 文件名
static int    g_iFd_parameter_a = -1;  // 文件描述符
static u8_t     *parameter_a_md5        = NULL;
static u8_t     *parameter_a_md5_s      = NULL;
static u32_t    a_md5_verify_len        = 0;
static char     *parameter_a_value      = NULL;         // 参数值
static char     *parameter_a_value_buf  = NULL;
static u16_t    parameter_a_len         = 64;

#define g_recName_parameter_b   "paraconfig_file_b.ini"  // 文件名
static int    g_iFd_parameter_b = -1;  // 文件描述符
static u8_t     *parameter_b_md5        = NULL;
static u8_t     *parameter_b_md5_s      = NULL;
static char     *parameter_b_value_buf  = NULL;

// 生成MD5值
static u32_t parameter_a_MD5Verify_Func(u8_t bHash[], u32_t Verify_Len, u32_t DataBlock_Len);
static u32_t parameter_b_MD5Verify_Func(u8_t bHash[], u32_t Verify_Len, u32_t DataBlock_Len);

void parameter_a_module(void) {       // 依据PDT参数表重建参数文件A
//  APP_DEBUG("\r\n%s()\r\n", __func__);
  s32_t file_a_size = Ql_FS_GetSize(g_recName_parameter_a);     // 删除a
  APP_DEBUG("file_a_size is %ld\r\n", file_a_size);
  if (file_a_size >= 0) {
    int delete = Ql_FS_Delete(g_recName_parameter_a);
    APP_DEBUG("delete is %d\r\n", delete);
  }

  g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_CREATE_ALWAYS);
  a_md5_verify_len = 0;

  if (g_iFd_parameter_a < 0) {
    APP_DEBUG("Create parameter_configuration_file_a failed\r\n");
  }
  Ql_FS_Close(g_iFd_parameter_a);

  APP_DEBUG("number_of_array_elements: %d\r\n", number_of_array_elements);

  char a_number_char[3] = {0};
  s32_t ret = 0;
  u32 writenLen = 0;
  int j = 0;

  g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_WRITE);
  if (g_iFd_parameter_a < 0) {
    APP_DEBUG("Open parameter_configuration_file_a failed\r\n");
  }
  ret = Ql_FS_Seek(g_iFd_parameter_a, 0, QL_FS_FILE_END);
  APP_DEBUG("Seek to file end ret: %d\r\n", ret);

  for (j = 0; j < number_of_array_elements; j++) {
    Ql_sprintf(a_number_char, "%03d", PDT[j].num);

    parameter_a_value_buf = Ql_MEM_Alloc(sizeof(char) * 64);
    Ql_memset(parameter_a_value_buf, 0, sizeof(char) * 64);

    PDT[j].rFunc(&PDT[j], parameter_a_value_buf, &parameter_a_len);

//    APP_DEBUG("PDT[%d]parameter:%s len: %d\r\n", j, parameter_a_value_buf, parameter_a_len);

    parameter_a_value = Ql_MEM_Alloc(sizeof(char) * parameter_a_len);
    Ql_memset(parameter_a_value, 0, parameter_a_len);
    Ql_memcpy(parameter_a_value, parameter_a_value_buf, parameter_a_len);

    ret = Ql_FS_Write(g_iFd_parameter_a, (u8 *)"para", 4, &writenLen);
//    APP_DEBUG("PDT[%d] Write para ret: %d len: %d\r\n", j, ret, writenLen);
    ret = Ql_FS_Write(g_iFd_parameter_a, (u8 *)a_number_char, 3, &writenLen);  // 写参数编号到文件
//    APP_DEBUG("PDT[%d] Write number ret: %d len: %d\r\n", j, ret, writenLen);
    ret = Ql_FS_Write(g_iFd_parameter_a, (u8 *)"=", 1, &writenLen);       // 等号
//    APP_DEBUG("PDT[%d] Write = ret: %d len: %d\r\n", j, ret, writenLen);
    ret = Ql_FS_Write(g_iFd_parameter_a, (u8 *)parameter_a_value, parameter_a_len, &writenLen);  // 写参数值到文件
//    APP_DEBUG("PDT[%d] Write para value ret: %d len: %d\r\n", j, ret, writenLen);
    ret = Ql_FS_Write(g_iFd_parameter_a, (u8 *)"\0\r\n", 3, &writenLen);  // 回车换行
//    APP_DEBUG("PDT[%d] Write end line ret: %d len: %d\r\n", j, ret, writenLen);

    a_md5_verify_len = a_md5_verify_len + 4 + 3 + 1 + parameter_a_len + 3;

    if (parameter_a_value != NULL) {
      Ql_MEM_Free(parameter_a_value);
      parameter_a_value = NULL;
    }

    if (parameter_a_value_buf != NULL) {
      Ql_MEM_Free(parameter_a_value_buf);
      parameter_a_value_buf = NULL;
    }
//  Ql_FS_Flush(g_iFd_parameter_a);
//  cm_sleep(10);
  }

  Ql_FS_Flush(g_iFd_parameter_a);
  Ql_FS_Close(g_iFd_parameter_a);

  file_a_size = Ql_FS_GetSize(g_recName_parameter_a);
  APP_DEBUG("file_a_size is %ld after write\r\n", file_a_size);

  parameter_a_md5 = Ql_MEM_Alloc(sizeof(u8_t) * 32);  // MD5值
  Ql_memset(parameter_a_md5, 0, sizeof(u8_t) * 32);
  parameter_a_md5_s = Ql_MEM_Alloc(sizeof(u8_t) * 32);  // MD5值
  Ql_memset(parameter_a_md5_s, '\0', sizeof(u8_t) * 32);
  parameter_a_MD5Verify_Func(parameter_a_md5, a_md5_verify_len, 512);

//  Swap_hexChar((char*)parameter_a_md5_s, parameter_a_md5, 16, 0);
  hextostr(parameter_a_md5, parameter_a_md5_s, 16);
  APP_DEBUG("parameter_a_md5_s %s \r\n", parameter_a_md5_s);

  g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_WRITE);
  if (g_iFd_parameter_a < 0) {
    APP_DEBUG("open parameter_configuration_file_a failed\r\n");
  }
  ret = Ql_FS_Seek(g_iFd_parameter_a, 0, QL_FS_FILE_END);
  APP_DEBUG("Seek to file end ret: %d\r\n", ret);

  ret = Ql_FS_Write(g_iFd_parameter_a, parameter_a_md5_s, 32, &writenLen);  // 写内容到文件
  APP_DEBUG("Write md5 ret: %d len: %d\r\n", ret, writenLen);
  Ql_FS_Flush(g_iFd_parameter_a);
  Ql_FS_Close(g_iFd_parameter_a);

  file_a_size = Ql_FS_GetSize(g_recName_parameter_a);
  APP_DEBUG("file_a_size is %ld after write md5\r\n", file_a_size);

  if (parameter_a_md5 != NULL) {
    Ql_MEM_Free(parameter_a_md5);
    parameter_a_md5 = NULL;
  }

  if (parameter_a_md5_s != NULL) {
    Ql_MEM_Free(parameter_a_md5_s);
    parameter_a_md5_s = NULL;
  }
  APP_DEBUG("parameter_a_module ok\r\n");
}

// 生成MD5值
static u32_t parameter_a_MD5Verify_Func(u8_t bHash[], u32_t Verify_Len, u32_t DataBlock_Len) {
  MD5_t  md5;
  Hash_MD5Init(&md5);
  static u8_t s_pTempMd5[512] = {0};
  u32_t dwOff = 0;
  s32_t ret = 0;
  u32 readenLen = 0;

  APP_DEBUG("Verify_Len is %ld\r\n", Verify_Len);
  while (dwOff < Verify_Len) {
    Ql_memset(s_pTempMd5, 0, sizeof(s_pTempMd5));
    g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_ONLY);
    if (g_iFd_parameter_a < 0) {
      APP_DEBUG("Open paraconfig_file_a failed\r\n");
      break;
    }
    ret = Ql_FS_Seek(g_iFd_parameter_a, dwOff, QL_FS_FILE_BEGIN);
    if (ret != QL_RET_OK) {
      APP_DEBUG("Seek paraconfig_file_a failed\r\n");
    }
    ret = Ql_FS_Read(g_iFd_parameter_a, s_pTempMd5, DataBlock_Len, &readenLen);
    APP_DEBUG("offset:%d, read ret:%d len:%d\r\n", dwOff, ret, readenLen); // mike 20200827
    if ((Verify_Len - dwOff) < DataBlock_Len) {
      DataBlock_Len = Verify_Len - dwOff;
    }
    Hash_MD5Update(&md5, s_pTempMd5, DataBlock_Len);
    dwOff += DataBlock_Len;
    Ql_FS_Close(g_iFd_parameter_a);
  }
  Hash_MD5Final(bHash, &md5);
  return 0;
}

// 生成MD5值
static u32_t parameter_b_MD5Verify_Func(u8_t bHash[], u32_t Verify_Len, u32_t DataBlock_Len) {
  MD5_t  md5;
  Hash_MD5Init(&md5);
  static u8_t s_pTempMd5[512] = {0};
  u32_t dwOff = 0;
  s32_t ret = 0;
  u32 readenLen = 0;

  APP_DEBUG("Verify_Len is %ld\r\n", Verify_Len);
  while (dwOff < Verify_Len) {
    Ql_memset(s_pTempMd5, 0, sizeof(s_pTempMd5));
    g_iFd_parameter_b = Ql_FS_Open(g_recName_parameter_b, QL_FS_READ_ONLY);
    if (g_iFd_parameter_b < 0) {
      APP_DEBUG("Open paraconfig_file_b failed\r\n");
      break;
    }
    ret = Ql_FS_Seek(g_iFd_parameter_b, dwOff, QL_FS_FILE_BEGIN);
    if (ret != QL_RET_OK) {
      APP_DEBUG("Seek paraconfig_file_b failed\r\n");
    }
    ret = Ql_FS_Read(g_iFd_parameter_b, s_pTempMd5, DataBlock_Len, &readenLen);
    APP_DEBUG("offset:%d, read ret:%d len:%d\r\n", dwOff, ret, readenLen); // mike 20200827
    if ((Verify_Len - dwOff) < DataBlock_Len) {
      DataBlock_Len = Verify_Len - dwOff;
    }
    Hash_MD5Update(&md5, s_pTempMd5, DataBlock_Len);
    dwOff += DataBlock_Len;
    Ql_FS_Close(g_iFd_parameter_b);
  }
  Hash_MD5Final(bHash, &md5);
  return 0;
}

void parameter_init(void) { // 依据参数文件A初始化PDT表
  char n_buf[sizeof(char) * 3] = {0};
  u8_t number = 0;
  char number_char[4] = {0};
  char *number_char_buff = n_buf;
  char *parameter_value = NULL;  // 参数值
  u16_t len = 64;  // 长度
  u8 temporary_char = 0;

  u32_t dwOff = 0;  // 偏移量
  u32_t equalPlace = 0;  // 等号
  u32_t newlinePlace = 0;  // 换行符
  s32_t ret = 0;
  u32 readenLen = 0;
  int j = 0;

  s32_t file_a_size = Ql_FS_GetSize(g_recName_parameter_a);

  while (dwOff < (file_a_size - 32)) {
    g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_ONLY);  // 按字节读文件A
    ret = Ql_FS_Seek(g_iFd_parameter_a, dwOff, QL_FS_FILE_BEGIN);
//    APP_DEBUG("Seek to site %d ret: %d\r\n", dwOff, ret);
    ret = Ql_FS_Read(g_iFd_parameter_a, &temporary_char, 1, &readenLen);
//    APP_DEBUG("Read 1 byte ret: %d len: %d\r\n", ret, readenLen);
    Ql_FS_Close(g_iFd_parameter_a);

    if ('=' == temporary_char) {    // 读到等于号时，读取等号前三位数据，获取参数编号
      equalPlace = dwOff;
      g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_ONLY);
      ret = Ql_FS_Seek(g_iFd_parameter_a, equalPlace - 3, QL_FS_FILE_BEGIN);
//      APP_DEBUG("Seek to site %d ret: %d\r\n", dwOff, ret);
      Ql_memset(number_char_buff, 0, sizeof(char) * 3);
      ret = Ql_FS_Read(g_iFd_parameter_a, (u8 *)number_char_buff, 3, &readenLen);
//      APP_DEBUG("Read number ret: %d len: %d %s\r\n", ret, readenLen, number_char_buff);
      Ql_FS_Close(g_iFd_parameter_a);
      Ql_memcpy(number_char, number_char_buff, 3);

      number = Swap_charNum(number_char);
//      APP_DEBUG("Read number: %d \r\n", number);
      Ql_memset(number_char, 0, 4);
    }

    if ('\r' == temporary_char) {   // 读到回车符号时，读取等号后到回车符前的数据，获取参数信息
      newlinePlace = dwOff;
      g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_ONLY);
      ret = Ql_FS_Seek(g_iFd_parameter_a, equalPlace + 1, QL_FS_FILE_BEGIN);    // 返回等号后一位
//      APP_DEBUG("Seek to site %d ret: %d\r\n", dwOff, ret);
      if (parameter_value != NULL) {
        Ql_MEM_Free(parameter_value);
        parameter_value = NULL;
      }
      parameter_value = Ql_MEM_Alloc(sizeof(char) * (newlinePlace - equalPlace - 1));
      ret = Ql_FS_Read(g_iFd_parameter_a, (u8 *)parameter_value, newlinePlace - equalPlace - 1, &readenLen);
//      APP_DEBUG("Read parameter_value ret: %d len: %d\r\n", ret, readenLen);
      Ql_FS_Close(g_iFd_parameter_a);
    }

    if (('\n' == temporary_char)) { // 读取到换行符号时，将读取到的数据填写到PDT表对应的参数中
      for (j = 0; j < number_of_array_elements; j++) {
        if (number == PDT[j].num) {
          Ql_memset((&PDT[j])->a, 0, sizeof((&PDT[j])->a));
          PDT[j].wFunc(&PDT[j], parameter_value, &len);
//          APP_DEBUG("Set PDT[%d] : %s len =%d\r\n", j, parameter_value, len);
          if (parameter_value != NULL) {
            Ql_MEM_Free(parameter_value);
            parameter_value = NULL;
          }
          break;
        }
      }
    }
    dwOff++;
  }
  if (parameter_value != NULL) {
    Ql_MEM_Free(parameter_value);
    parameter_value = NULL;
  }

  for (j = 0; j < number_of_array_elements; j++) {
//    APP_DEBUG("%03d:%s len:%d\r\n", PDT[j].num, PDT[j].a, Ql_strlen(PDT[j].a));
  }

  APP_DEBUG("Parameter init OK!!\r\n");
}

void live_a_and_b(void) {   // 同步两个配置文件
  s32_t file_a_size = Ql_FS_GetSize(g_recName_parameter_a);
  APP_DEBUG("file_a_size is %ld\r\n", file_a_size);

  s32_t file_b_size = Ql_FS_GetSize(g_recName_parameter_b);
  APP_DEBUG("file_b_size is %ld\r\n", file_b_size);

  if ((file_a_size > 32) && (file_b_size > 32)) {   // 这个文件都存在
    APP_DEBUG("a b are all live\r\n");
    a_compare_b();    // 比较两个参数文件
    parameter_init();
  }

  if ((!(file_a_size > 32)) && (!(file_b_size > 32))) { // 两个文件都不存在
    APP_DEBUG("a b are all not live\r\n");
    parametr_default();     // 依据default生成PDT表, 生成配置a、b文件
  }

  if ((file_a_size > 32) && (!(file_b_size > 32))) {  // a存在b不存在
    APP_DEBUG("a is live b is not live\r\n");
    a_copy_to_b();
    parameter_init();
  }

  if ((!(file_a_size > 32)) && (file_b_size > 32)) {  // b存在a不存在
    APP_DEBUG("a is not live b is live\r\n");
    b_copy_to_a();
    parameter_init();
  }
}

// 比较两个参数文件
void a_compare_b(void) {
  s32_t ret = 0;
  u32 readenLen = 0;
  s32_t file_a_size = 0, file_b_size = 0;

  file_a_size = Ql_FS_GetSize(g_recName_parameter_a);
  APP_DEBUG("file_a_size is %ld\r\n", file_a_size);

  parameter_a_md5 = Ql_MEM_Alloc(sizeof(u8_t) * 32);
  Ql_memset(parameter_a_md5, 0, sizeof(u8_t) * 32);
  parameter_a_md5_s = Ql_MEM_Alloc(sizeof(u8_t) * 32);
  Ql_memset(parameter_a_md5_s, 0, sizeof(u8_t) * 32);

  parameter_a_MD5Verify_Func(parameter_a_md5, file_a_size - 32, 512);   // 得到文件A的MD5值
  hextostr(parameter_a_md5, parameter_a_md5_s, 16);
  APP_DEBUG("parameter_a_md5_s %s\r\n", parameter_a_md5_s);
  if (parameter_a_md5 != NULL) {
    Ql_MEM_Free(parameter_a_md5);   // mike 释放parameter_a_md5
    parameter_a_md5 = NULL;
  }  // save to here

  file_b_size = Ql_FS_GetSize(g_recName_parameter_b);
  APP_DEBUG("file_b_size is %ld\r\n", file_b_size);

  parameter_b_md5 = Ql_MEM_Alloc(sizeof(u8_t) * 32);
  Ql_memset(parameter_b_md5, 0, sizeof(u8_t) * 32);
  parameter_b_md5_s = Ql_MEM_Alloc(sizeof(u8_t) * 32);
  Ql_memset(parameter_b_md5_s, 0, sizeof(u8_t) * 32);

  parameter_b_MD5Verify_Func(parameter_b_md5, file_b_size - 32, 512);   // 得到文件B的MD5值
  hextostr(parameter_b_md5, parameter_b_md5_s, 16);
  APP_DEBUG("parameter_b_md5_s %s\r\n", parameter_b_md5_s);
  if (parameter_b_md5 != NULL) {
    Ql_MEM_Free(parameter_b_md5);
    parameter_b_md5 = NULL;
  }

  int compute_compare_result = 0;           // 对比两个文件的MD5数据
  compute_compare_result = Ql_strcmp((char *)parameter_a_md5_s, (char *)parameter_b_md5_s);
  u8_t *para_a_md5 = NULL;
  u8_t *para_b_md5 = NULL;

  if (0 == compute_compare_result) {
    APP_DEBUG("compute value para_a_md5 = compute value para_b_md5\r\n");
  } else {  // 当两个文件的MD5值不同时
    APP_DEBUG("compute value para_a_md5 != compute value para_b_md5\r\n");

    para_a_md5 = Ql_MEM_Alloc(sizeof(u8_t) * 32);
    Ql_memset(para_a_md5, 0, sizeof(u8_t) * 32);

    file_a_size = Ql_FS_GetSize(g_recName_parameter_a);
    g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_ONLY);  // 读取记录在文件A中的原始MD5值
    ret = Ql_FS_Seek(g_iFd_parameter_a, file_a_size - 32, QL_FS_FILE_BEGIN);
    APP_DEBUG("Seek to file %d site ret: %d\r\n", file_a_size - 32, ret);
    ret = Ql_FS_Read(g_iFd_parameter_a, para_a_md5, 32, &readenLen);
    APP_DEBUG("Read para_a_md5 from a ret: %d len: %d\r\n", ret, readenLen);
    Ql_FS_Close(g_iFd_parameter_a);

    int compute_compare_result_a = 0;
    compute_compare_result_a = Ql_strcmp((char *)parameter_a_md5_s,
                                         (char *)para_a_md5);  // 对比文件A的MD5值，判断是否被非法更改过

    para_b_md5 = Ql_MEM_Alloc(sizeof(u8_t) * 32);
    Ql_memset(para_b_md5, 0, sizeof(u8_t) * 32);

    file_b_size = Ql_FS_GetSize(g_recName_parameter_b);
    g_iFd_parameter_b = Ql_FS_Open(g_recName_parameter_b, QL_FS_READ_ONLY);     // 读取记录在文件B中的原始MD5值
    ret = Ql_FS_Seek(g_iFd_parameter_b, file_b_size - 32, QL_FS_FILE_BEGIN);
    APP_DEBUG("Seek to file %d site ret: %d\r\n", file_b_size - 32, ret);
    ret = Ql_FS_Read(g_iFd_parameter_b, para_b_md5, 32, &readenLen);
    APP_DEBUG("Read para_b_md5 from b ret: %d len: %d\r\n", ret, readenLen);
    Ql_FS_Close(g_iFd_parameter_b);

    int compute_compare_result_b = 0;
    compute_compare_result_b = Ql_strcmp((char *)parameter_b_md5_s,
                                         (char *)para_b_md5);         // 对比文件B的MD5值，判断是否被非法更改过

    if (0 == compute_compare_result_a) {
      APP_DEBUG("compute value para_a_md5 = Existing value para_a_md5\r\n");
    } else {
      APP_DEBUG("compute value para_a_md5 != Existing value para_a_md5\r\n");
      //文件拷贝 b->a
      b_copy_to_a();
    }

    if (0 == compute_compare_result_b) {
      APP_DEBUG("compute value para_b_md5 = Existing value para_b_md5\r\n");
    } else {
      APP_DEBUG("compute value para_b_md5 != Existing value para_b_md5\r\n");
      // 文件拷贝 a->b
      a_copy_to_b();
    }
    if (para_a_md5 != NULL) {
      Ql_MEM_Free(para_a_md5);
      para_a_md5 = NULL;
    }
    if (para_b_md5 != NULL) {
      Ql_MEM_Free(para_b_md5);
      para_b_md5 = NULL;
    }
    // 最后一种可能
  }

  if (parameter_a_md5_s != NULL) {
    Ql_MEM_Free(parameter_a_md5_s);
    parameter_a_md5_s = NULL;
  }
  if (parameter_b_md5_s != NULL) {
    Ql_MEM_Free(parameter_b_md5_s);
    parameter_b_md5_s = NULL;
  }
}

// 把文件A拷贝到文件B
void a_copy_to_b(void) {
  s32_t ret = 0;
  u32 readenLen = 0, writenLen = 0;

  s32_t file_b_size = Ql_FS_GetSize(g_recName_parameter_b); // 删除B
  APP_DEBUG("file_b_size is %ld\r\n", file_b_size);
  if (file_b_size >= 0) {
    int delete = Ql_FS_Delete(g_recName_parameter_b);
    APP_DEBUG("delete is %d\r\n", delete);
  }

  s32_t file_a_size = Ql_FS_GetSize(g_recName_parameter_a);
  g_iFd_parameter_b = Ql_FS_Open(g_recName_parameter_b, QL_FS_CREATE_ALWAYS);   // 建立B，并拷贝A的内容
  if (g_iFd_parameter_b < 0) {
    APP_DEBUG("Create parameter_configuration_file_b failed\r\n");
  }
  Ql_FS_Close(g_iFd_parameter_b);
  g_iFd_parameter_b = Ql_FS_Open(g_recName_parameter_b, QL_FS_READ_WRITE);
  if (g_iFd_parameter_b < 0) {
    APP_DEBUG("Open parameter_configuration_file_b failed\r\n");
  }

  g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_ONLY);
  parameter_a_value_buf = Ql_MEM_Alloc(sizeof(char) * file_a_size);
  Ql_memset(parameter_a_value_buf, 0, sizeof(char)*file_a_size);
  ret = Ql_FS_Read(g_iFd_parameter_a, (u8 *)parameter_a_value_buf, file_a_size, &readenLen);
  APP_DEBUG("Read data from a ret: %d len: %d\r\n", ret, readenLen);
  Ql_FS_Close(g_iFd_parameter_a);

  ret = Ql_FS_Seek(g_iFd_parameter_b, 0, QL_FS_FILE_END);
  APP_DEBUG("Seek to file b end ret: %d\r\n", ret);
  ret = Ql_FS_Write(g_iFd_parameter_b, (u8 *)parameter_a_value_buf, file_a_size, &writenLen);
  APP_DEBUG("Write data to b ret: %d len: %d\r\n", ret, writenLen);
  Ql_FS_Flush(g_iFd_parameter_b);
  Ql_FS_Close(g_iFd_parameter_b);

  file_b_size =  Ql_FS_GetSize(g_recName_parameter_b);
  APP_DEBUG("a_copy_to_b ok file_b_size is %ld\r\n", file_b_size);

  if (parameter_a_value_buf != NULL) {
    Ql_MEM_Free(parameter_a_value_buf);
    parameter_a_value_buf = NULL;
  }
}

// 把文件B拷贝到文件A
void b_copy_to_a(void) {
  s32_t ret = 0;
  u32 readenLen = 0, writenLen = 0;

  s32_t file_a_size = Ql_FS_GetSize(g_recName_parameter_a); // 删除A
  APP_DEBUG("file_a_size is %ld\r\n", file_a_size);
  if (file_a_size >= 0) {
    int delete = Ql_FS_Delete(g_recName_parameter_a);
    APP_DEBUG("delete is %d\r\n", delete);
  }

  s32_t file_b_size = Ql_FS_GetSize(g_recName_parameter_b);
  g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_CREATE_ALWAYS);   // 建立A，并拷贝B的内容
  if (g_iFd_parameter_a < 0) {
    APP_DEBUG("Create parameter_configuration_file_a failed\r\n");
  }
  Ql_FS_Close(g_iFd_parameter_a);
  g_iFd_parameter_a = Ql_FS_Open(g_recName_parameter_a, QL_FS_READ_WRITE);
  if (g_iFd_parameter_a < 0) {
    APP_DEBUG("Open parameter_configuration_file_a failed\r\n");
  }

  g_iFd_parameter_b = Ql_FS_Open(g_recName_parameter_b, QL_FS_READ_ONLY);
  if (g_iFd_parameter_b < 0) {
    APP_DEBUG("Open parameter_configuration_file_a failed\r\n");
  }
  parameter_b_value_buf = Ql_MEM_Alloc(sizeof(char) * file_b_size);
  Ql_memset(parameter_b_value_buf, 0, sizeof(char)*file_b_size);
  ret = Ql_FS_Read(g_iFd_parameter_b, (u8 *)parameter_b_value_buf, file_b_size, &readenLen);
  APP_DEBUG("Read data from b ret: %d len: %d\r\n", ret, readenLen);
  Ql_FS_Close(g_iFd_parameter_b);

  ret = Ql_FS_Seek(g_iFd_parameter_a, 0, QL_FS_FILE_END);
  APP_DEBUG("Seek to file a end ret: %d\r\n", ret);
  ret = Ql_FS_Write(g_iFd_parameter_a, (u8 *)parameter_b_value_buf, file_b_size, &writenLen);
  APP_DEBUG("Write data to a ret: %d len: %d\r\n", ret, writenLen);
  Ql_FS_Flush(g_iFd_parameter_a);
  Ql_FS_Close(g_iFd_parameter_a);

  file_a_size =  Ql_FS_GetSize(g_recName_parameter_a);
  APP_DEBUG("b_copy_to_a ok file_a_size is %ld\r\n", file_a_size);

  if (parameter_b_value_buf != NULL) {
    Ql_MEM_Free(parameter_b_value_buf);
    parameter_b_value_buf = NULL;
  }
}

