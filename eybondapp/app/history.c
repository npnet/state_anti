
/******************************************************************************           
* name:             history.c           
* introduce:        history device data save for state grid 
* author:           Luee                                     
******************************************************************************/ 
#include "history.h"

#define history_file  "/history_file.ini"        //文件名

s32     iFd_history = 0;                     //文件描述符
static char history_head_buf[history_headoffset_len];
//u8 run_history_buf[512];

//static char tempbuf[log_line_len] = {0};
history_head_t *history_head = (history_head_t *)history_head_buf;

u16_t sw_historyhead_rp = 0;


/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
s32 history_head_get(void) {
  s32_t ret = 0;
  s32_t readenLen = 0;
  iFd_history = fibo_file_open(history_file, FS_O_RDONLY);
  if (iFd_history >= 0) {
    ret = fibo_file_seek(iFd_history, 0, FS_SEEK_SET);
    readenLen = fibo_file_read(iFd_history, (u8_t *)history_head_buf, sizeof(history_head_buf));
    ret = fibo_file_close(iFd_history);
    APP_DEBUG("get:run log read po = %d,write po = %d,l = %ld\r\n", \
              history_head->file_logr_pointer, history_head->file_logw_pointer, readenLen);
    if (readenLen < sizeof(history_head_buf)) {
      APP_DEBUG("Read %s history file head error!\r\n", history_file);
      ret = -1;
    }
  } else {
    APP_DEBUG("Read %s history file head error!\r\n", history_file);
    ret = -1;
  }
  return ret;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
s32 history_init(void) {
  s32_t ret = 0;

  s32_t history_size = fibo_file_getSize(history_file);
  if ((fibo_file_exist(history_file)==1)&&(history_size > 0)) {
    APP_DEBUG("found old history file!\r\n");
    //头信息获取失败
    if (history_head_get() != 0) {
      APP_DEBUG("run history file head err!\r\n");
      fibo_file_delete(history_file);
      history_size = -1;
    } else if ((history_head->file_logw_pointer >= history_pointer_size) ||
               (history_head->file_logr_pointer >= history_pointer_size)) {
      //文件大小和当前读写指针不匹配
      APP_DEBUG("history log file size err!\r\n");
      fibo_file_delete(history_file);
      history_size = -1;
    }
  }

  history_size=-1;
  if ((fibo_file_exist(history_file)!=1)||(history_size <= 0)) {
    APP_DEBUG("run history file create\r\n");
    history_head->file_logw_pointer = 0;
    history_head->file_logr_pointer = 0;
    iFd_history = fibo_file_open(history_file, FS_O_RDWR | FS_O_CREAT | FS_O_TRUNC);   // 读写、创建
    if (iFd_history < 0) {
      APP_DEBUG("history file create error\r\n");
      return history_size;
    }
    ret = fibo_file_seek(iFd_history, 0, FS_SEEK_SET);
    if (ret < 0) {
      APP_DEBUG("history file create seek error\r\n");
      ret = fibo_file_close(iFd_history);
      if (ret < 0) {
        APP_DEBUG("history file create close error\r\n");
      }
      return history_size;
    }
    ret = fibo_file_write(iFd_history, (u8_t *)history_head_buf, sizeof(history_head_buf));
    if (ret < 0) {
      APP_DEBUG("history file create write head error\r\n");
      ret = fibo_file_close(iFd_history);
      if (ret < 0) {
        APP_DEBUG("history file create close error\r\n");
      }
      return history_size;
    }
    ret = fibo_file_fsync(iFd_history);
    if (ret < 0) {
      APP_DEBUG("history file create sync error\r\n");
      ret = fibo_file_close(iFd_history);
      if (ret < 0) {
        APP_DEBUG("history file create close error\r\n");
      }
      return history_size;
    }
    ret = fibo_file_close(iFd_history);
    if (ret < 0) {
      APP_DEBUG("history file create close error\r\n");
      return history_size;
    }
    history_size = fibo_file_getSize(history_file);
  }

  iFd_history = fibo_file_open(history_file, FS_O_RDONLY);
  
  ret = fibo_file_seek(iFd_history, 0, FS_SEEK_SET);
  history_size=fibo_file_read(iFd_history,(u8_t *)history_head_buf,sizeof(history_head_buf));

  history_size = fibo_file_getSize(history_file);
  APP_DEBUG("\r\n-->state grid hisory init lenght:%ld",history_size);
  
  ret = fibo_file_fsync(iFd_history);
  ret = fibo_file_close(iFd_history);

  sw_historyhead_rp = history_head->file_logr_pointer;
  return history_size;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
void history_put(history_head_t *head,  Buffer_t *buf)
{
  s32 ret=-1;
  s32 history_size;

  iFd_history = fibo_file_open(history_file, FS_O_RDWR);
  if (iFd_history < 0) {
    APP_DEBUG("open history file fail!\r\n");
    return;
  }
    
  ret = fibo_file_seek(iFd_history, head->file_logw_pointer * history_line_len + history_headoffset_len, FS_SEEK_SET);
  history_size=fibo_file_write(iFd_history, (u8_t *)buf->payload, buf->lenght);

  // 写指针加1
  ret = fibo_file_seek(iFd_history, 0, FS_SEEK_SET);
  head->file_logw_pointer = (head->file_logw_pointer + 1);    // % history_pointer_size;
  fibo_file_write(iFd_history,(u8_t *)history_head_buf,sizeof(history_head_buf));

  //history_size = fibo_file_getSize(history_file);
  APP_DEBUG("\r\n-->state grid hisory save lenght:%ld",history_size);
  
  ret = fibo_file_fsync(iFd_history);
  ret = fibo_file_close(iFd_history);

  history_init();
}


//  pdu.lenght = FlashFIFO_get(&historyHead, &pdu);
/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
s32 history_get(history_head_t *head,  Buffer_t *buf)
{
  s32 ret=-1;
  s32 history_size=-1;

  iFd_history = fibo_file_open(history_file, FS_O_RDWR);
  if (iFd_history < 0) {
    APP_DEBUG("open history file fail!\r\n");
    return ret;
  }
    
  ret = fibo_file_seek(iFd_history, head->file_logr_pointer * history_line_len + history_headoffset_len, FS_SEEK_SET);
  history_size=fibo_file_read(iFd_history, (u8_t *)buf->payload, history_line_len);

  // 读指针加1
  ret = fibo_file_seek(iFd_history, 0, FS_SEEK_SET);
  head->file_logr_pointer = (head->file_logr_pointer + 1)% history_pointer_size;
  fibo_file_write(iFd_history,(u8_t *)history_head_buf,sizeof(history_head_buf));

  //history_size = fibo_file_getSize(history_file);
  APP_DEBUG("\r\n-->state grid hisory read lenght:%ld",history_size);
  
  ret = fibo_file_fsync(iFd_history);
  ret = fibo_file_close(iFd_history);

  return history_size;
}