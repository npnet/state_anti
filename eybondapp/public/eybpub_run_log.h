/******************************************************************************           
* name:             eybpub_run_log.h           
* introduce:        头文件
* author:           Luee                                     
******************************************************************************/ 
#ifndef _EYBPUB_RUN_LOG_H_
#define _EYBPUB_RUN_LOG_H_

#include "grid_tool.h"
#include "history.h"

#define log_headoffset_len      64   
#define log_head_len            4   

// log行长度
#define log_line_len            64     
// 历史文件大小
#define log_file_size           ((100)*(log_line_len)+(log_headoffset_len))
#define log_pointer_size    ((log_file_size-log_headoffset_len)/log_line_len)

typedef struct {
  u16 file_logw_pointer;
  u16 file_logr_pointer;
  u8 arry[];
}log_head_t;

extern u16 sw_loghead_rp;


//extern log_head_t *log_head;

s32 log_init(void);
void log_save(char *str, ...);
void log_clean(void);
u16_t log_get(Buffer_t *buf);
//void log_reset(void);
//void print_log_size(void);

#endif /* _EYBPUB_RUN_LOG_H_ */





/********************** COPYRIGHT 2014-2100, eybond ***************************
 * @File    : eybpub_run_log.h
 * @Author  : 
 * @Date    : 2020-08-20
 * @Brief   :
 ******************************************************************************/
/*
#ifndef __EYBPUB_RUN_LOG_H__
#define __EYBPUB_RUN_LOG_H__

#include "eyblib_typedef.h"

// 日志头信息便移地址
#define log_headoffset_len  (128)

// 日志行长度
#define log_line_len        (128)
// 日志文件大小
#define log_file_size       ((150)*(1024)+(log_headoffset_len))
#define log_pointer_size    ((log_file_size-log_headoffset_len)/log_line_len)

// 服务器获取日志最大的条数
#define log_max_line        20

typedef struct {
  u16_t file_logw_pointer;
  u16_t file_logr_pointer;
  u8_t arry[];
} log_head_t;

extern u16_t sw_loghead_rp;
extern log_head_t *log_head;

int log_init(void);
void log_save(char *str, ...);
void log_clean(void);
u16_t log_get(Buffer_t *buf);
void log_reset(void);
void print_log_size(void);

#endif  // __EYBPUB_RUN_LOG_H__
*/