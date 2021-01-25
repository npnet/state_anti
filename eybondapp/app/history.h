/******************************************************************************           
* name:             history.h           
* introduce:        头文件
* author:           Luee                                     
******************************************************************************/ 
#ifndef _HISTORY_H_
#define _HISTORY_H_

#include "grid_tool.h"
#include "StateGrid.h"

#define history_headoffset_len  STATE_GRID_CMD_SIZE   

// 历史行长度
#define history_line_len        STATE_GRID_CMD_SIZE   
// 历史文件大小
#define history_file_size       ((100)*(STATE_GRID_CMD_SIZE)+(history_headoffset_len))
#define history_pointer_size    ((history_file_size-history_headoffset_len)/history_line_len)

// 服务器获取历史最大的条数
#define history_max_line        20

typedef struct {
  u16 file_logw_pointer;
  u16 file_logr_pointer;
  u8 arry[];
} history_head_t;

extern history_head_t *history_head;

s32 history_init(void);
void history_put(history_head_t *head,  Buffer_t *buf);
s32 history_get(history_head_t *head,  Buffer_t *buf);

#endif /* _HISTORY_H_ */