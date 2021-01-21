/******************************************************************************           
* name:             history.h           
* introduce:        头文件
* author:           Luee                                     
******************************************************************************/ 
#ifndef _HISTORY_H_
#define _HISTORY_H_

#include "grid_tool.h"

#define history_headoffset_len  (128)

// 历史行长度
#define history_line_len        (128)
// 历史文件大小
#define history_file_size       ((150)*(1024)+(history_headoffset_len))
#define history_pointer_size    ((history_file_size-history_headoffset_len)/history_line_len)

// 服务器获取历史最大的条数
#define history_max_line        20

typedef struct {
  u16 file_logw_pointer;
  u16 file_logr_pointer;
  u8 arry[];
} history_head_t;

s32 history_init(void);

#endif /* _HISTORY_H_ */