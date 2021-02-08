/******************************************************************************           
* name:             eybpub_run_log.c           
* introduce:        log save & read
* author:           Luee                                     
******************************************************************************/ 
#include "eybpub_run_log.h"
#include "grid_tool.h"

#define log_file  "/log_file.ini"        //文件名

s32 iFd_log = 0;                     //文件描述符
static char log_head_buf[log_headoffset_len];
log_head_t *log_head = (log_head_t *)log_head_buf;
u16 sw_loghead_rp;

static s32 log_head_get(void);
static s32 log_write(log_head_t *head,  Buffer_t *buf);
static u16 log_read(log_head_t *head,  Buffer_t *buf);

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static s32 log_head_get(void)
{
  s32 ret=0;
  s32 readlen=-1;

  iFd_log = fibo_file_open(log_file, FS_O_RDONLY);
  if (iFd_log >= 0) {
    ret = fibo_file_seek(iFd_log, 0, FS_SEEK_SET);
    readlen = fibo_file_read(iFd_log, (u8_t *)log_head_buf, sizeof(log_head_buf));
    ret = fibo_file_fsync(iFd_log);
    ret = fibo_file_close(iFd_log);
  }else{
    ret=-1;
  }
  return ret;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
s32 log_init(void)
{
  s32 ret = 0;

  //fibo_file_delete(log_file);

  s32 log_size = fibo_file_getSize(log_file);
  if (log_size > 0) {
    //头信息获取失败
    if (log_head_get() != 0) {
      APP_DEBUG("run history file head err!\r\n");
      fibo_file_delete(log_file);
      log_size = -1;
    } else if ((log_head->file_logw_pointer >= log_pointer_size) ||
               (log_head->file_logr_pointer >= log_pointer_size)) {
      //文件大小和当前读写指针不匹配
      APP_DEBUG("logtory log file size err!\r\n");
      fibo_file_delete(log_file);
      log_size = -1;
    }
  }

  if ((fibo_file_exist(log_file)!=1)||(log_size <= 0)) {
    log_head->file_logw_pointer = 0;
    log_head->file_logr_pointer = 0;
    iFd_log = fibo_file_open(log_file, FS_O_RDWR|FS_O_CREAT|FS_O_APPEND);//读写、创建、追加
    ret = fibo_file_seek(iFd_log, 0, FS_SEEK_SET);
    log_size = fibo_file_write(iFd_log, (u8_t *)log_head_buf, sizeof(log_head_buf));
    ret = fibo_file_fsync(iFd_log);
    ret = fibo_file_close(iFd_log);
  } 

  iFd_log = fibo_file_open(log_file, FS_O_RDONLY);
  ret = fibo_file_seek(iFd_log, 0, FS_SEEK_SET);
  ret = fibo_file_read(iFd_log, (u8_t *)log_head_buf, sizeof(log_head_buf));
  ret = fibo_file_fsync(iFd_log);
  ret = fibo_file_close(iFd_log);
  log_size = fibo_file_getSize(log_file);
  APP_DEBUG("\r\n-->state grid:log file read pointer = %d,write pointer = %d,lenght = %ld\r\n", \
              log_head->file_logr_pointer, log_head->file_logw_pointer, log_size);
    
  return log_size;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static s32 log_write(log_head_t *head,  Buffer_t *buf)
{
  s32 ret=-1;

  //数列log_buf是要存在文件中的数据
  //数据结构由结构体log_data组成，包括size lenght payload[]
  //size=size占用字节数+lenght占用字数+payload[]占用字节数
  //lenght=payload[]占用字节数
  u8 log_buf[log_line_len]={0};
  buf_t *log_data=(buf_t *)log_buf;

  APP_DEBUG("\r\n-->log put:buf->lenght=%d\r\n",buf->lenght);
  //日志每笔数据长度不能超过64
  if(buf->lenght>log_line_len)
    return ret;
  APP_DEBUG("\r\n-->log:input buf value:%s\r\n",buf->payload);

  //若写指针超出，删除文件，指针归零
  log_init();

  //得到国网历史数据写入BUF:history_bf
  log_data->lenght=buf->lenght;
  r_memcpy(log_data->payload,buf->payload, buf->lenght);
  log_data->size=sizeof(log_data->size)+sizeof(log_data->lenght)+log_data->lenght;

  iFd_log = fibo_file_open(log_file, FS_O_WRONLY|FS_O_APPEND);
  if (iFd_log < 0) {
    APP_DEBUG("\r\n-->log:open log file fail!\r\n");
    return ret;
  }
  //将日志数据log_buf写入文件log file  
  ret = fibo_file_seek(iFd_log, head->file_logw_pointer * log_line_len + log_headoffset_len, FS_SEEK_SET);
  ret=fibo_file_write(iFd_log, (u8_t *)log_buf, sizeof(log_buf));
  if(ret!=sizeof(log_buf)){
    APP_DEBUG("\r\n-->log: write fail,ret=%ld\r\n",ret);
    fibo_file_fsync(iFd_log);
    fibo_file_close(iFd_log);
    return ret;
  }

  // 写指针加1并写入文件
  fibo_file_seek(iFd_log, 0, FS_SEEK_SET);
  head->file_logw_pointer = (head->file_logw_pointer + 1) % log_pointer_size;
  fibo_file_write(iFd_log,(u8_t *)log_head_buf,sizeof(log_head_buf));

  APP_DEBUG("\r\n-->log: save lenght:%d,next w_printer:%d",log_data->lenght,head->file_logw_pointer); 
  APP_DEBUG("\r\n-->log:%s\r\n",buf->payload);
  fibo_file_fsync(iFd_log);
  fibo_file_close(iFd_log);
  return ret;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static u16 log_read(log_head_t *head,  Buffer_t *buf)
{
  s32 ret=-1;
  u16 log_size=0xffff;
  u8 log_buf[log_line_len]={0};
  buf_t *log_data=(buf_t *)log_buf;
  //确保指针在许可范围之内
  log_init();

  iFd_log = fibo_file_open(log_file, FS_O_RDONLY);
  if (iFd_log < 0) {
    APP_DEBUG("\r\n-->log:open log file fail!\r\n");
    return log_size;
  }

  //从文件log_file读取指针head->file_logr_pointer指定的数据，存入log_buf  
  ret = fibo_file_seek(iFd_log, head->file_logr_pointer * log_line_len + log_headoffset_len, FS_SEEK_SET);
  log_size=fibo_file_read(iFd_log, (u8_t *)log_buf, sizeof(log_buf));
  ret = fibo_file_fsync(iFd_log);
  ret = fibo_file_close(iFd_log);

  //得到返回数据
  log_size=log_data->lenght;
  if(log_size==0||log_size>log_line_len||(log_data->size!=log_size+4)){
    APP_DEBUG("\r\n-->log: read fail,log_size=%d\r\n",log_size);
    return log_size;
  }
  r_memcpy(buf->payload,log_data->payload,log_size);
  print_buf(log_buf,log_data->size);

  //循环读取日志
  head->file_logr_pointer = (head->file_logr_pointer + 1)% history_pointer_size;
  iFd_log = fibo_file_open(log_file, FS_O_WRONLY|FS_O_APPEND);
  ret = fibo_file_seek(iFd_log, 0, FS_SEEK_SET);
  fibo_file_write(iFd_log,(u8_t *)log_head_buf,sizeof(log_head_buf));
  ret = fibo_file_fsync(iFd_log);
  ret = fibo_file_close(iFd_log);

  APP_DEBUG("\r\n-->log: read lenght:%d,next r_printer:%d",log_data->lenght,head->file_logr_pointer); 
  return log_size;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
void log_save(char *str, ...) 
{
  
  s32 ret;
  Buffer_t buf;
  u16 pointer=0;                  //开始写地址
  u16 leftover=0;        //数据剩余长度
  Buffer_t write_buf;      //待写入结构体

  va_list args;

  // 该函数用于获取本地时间
  hal_rtc_time_t time;
  ret = fibo_getRTC(&time);
  time.year+=2000;
  if (ret < 0) {
    time.year = 2015;
    time.month = 1;
    time.day = 1;
    time.hour = 0;
    time.min = 0;
    time.sec = 0;
    time.wDay = 4;
  }

  u16_t offset = r_strlen(str);
  buf.lenght=offset;
  buf.payload=fibo_malloc(buf.lenght+32);
  //if (buf.lenght < 64 && null != (buf.payload = memory_apply(offset + 32)))
  if(buf.payload!=null){
      //Clock_t clock;
	    //Clock_timeZone(&clock);
      r_memset(buf.payload,'\0',buf.lenght);
      offset = Swap_numChar((char *)buf.payload, time.year);
      buf.payload[offset++] = '-';
      offset += Swap_numChar((char *)&buf.payload[offset], time.month);
      buf.payload[offset++] = '-';
      offset += Swap_numChar((char *)&buf.payload[offset], time.day);
      buf.payload[offset++] = ' ';
      
      offset += Swap_numChar((char *)&buf.payload[offset], time.hour);
      buf.payload[offset++] = ':';
      offset += Swap_numChar((char *)&buf.payload[offset], time.min);
      buf.payload[offset++] = ':';
      offset += Swap_numChar((char *)&buf.payload[offset], time.sec);
      buf.payload[offset++] = '>';
      buf.payload[offset++] = ' ';
      
      //offset += r_strcpy_len((char *)&buf.payload[offset], str);
      va_start(args, str);        
      sprintf((char *)&buf.payload[offset],str, args);
      va_end(args);   
      offset=r_strlen((char *)buf.payload);
	
      buf.lenght = offset + 1;

      APP_DEBUG("\r\n-->log: log input buf.payload=%s buf.lenght=%d\r\n",buf.payload,buf.lenght);

      //以上得到要写的数据放在:buf.payload
      //长度：buf.lenght
	    //FlashFIFO_put(&s_logeHead, &buf);
      pointer=0;                  //开始写地址
      leftover=buf.lenght;        //数据剩余长度
      write_buf.lenght=log_line_len;
      write_buf.payload=null;
      write_buf.payload=fibo_malloc(write_buf.lenght);

      if(write_buf.payload==null){
        APP_DEBUG("\r\n-->log:write_buf malloc fail\r\n");
      }else{
        APP_DEBUG("\r\n-->log:write_buf malloc success leftover=%d\r\n",leftover);

      while(leftover>log_line_len){
        APP_DEBUG("\r\n-->log:leftover(%d)>64\r\n",leftover);
        //得到写入结构体数据
        r_memset(write_buf.payload, 0, write_buf.lenght);
        r_memcpy(write_buf.payload,buf.payload+pointer,write_buf.lenght);
        //写入文件
        log_write(log_head,&write_buf);
        pointer+=log_line_len;    //下一组数据开始指针
        leftover-=log_line_len;   //剩余长度
      }


      APP_DEBUG("\r\n-->log:leftover(%d)<64\r\n",leftover);
      if(leftover){
        //得到写入结构体数据
        r_memset(write_buf.payload, 0, write_buf.lenght);
        write_buf.lenght=leftover;
        r_memcpy((void*)write_buf.payload,(void*)(buf.payload+pointer),(u32)write_buf.lenght);
        //写入文件
        log_write(log_head,&write_buf);
      }
      fibo_free(write_buf.payload);
      }


	    fibo_free(buf.payload);
  }
  
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
u16 log_get(Buffer_t *buf) 
{
  u16 ret=0;
  
  if(buf->payload==null){
    APP_DEBUG("\r\n-->log:buf->lenght=null\r\n");
    return ret;
  }
    
  ret=log_read(log_head,buf);
  if(ret==0xffff){
    ret=0;
  }
  buf->lenght=ret;
  return ret;
}

void log_clean(void) {
  fibo_file_delete(log_file);
  log_init();
}





/*******************************************************************************
 * @Author       : Chen Zhu Liang
 * @Date         : 2020-06-22 11:45:09
 * @LastEditTime : 2020-07-03 15:05:36
 * @LastEditors  : Chen Zhu Liang
 * @Description  :
 * @FilePath     : public\run_log.c
 * @可以输入预定的版权声明、个性签名、空行等
*******************************************************************************/
/*
#ifdef _PLATFORM_BC25_
#include "ql_fs.h"
#include "ql_error.h"
#include "ql_time.h"
#include "ql_stdlib.h"
#endif

#ifdef _PLATFORM_L610_
#include "stdio.h"
#include "fibo_opencpu.h"
#endif
#include "eyblib_r_stdlib.h"
// #include "eyblib_memory.h"       // mike 20200828

#include "eybpub_Debug.h"
#include "eybpub_run_log.h"
#include "eybpub_data_collector_parameter_table.h"
#include "eybpub_parameter_number_and_value.h"

#define run_log_a  "/run_log_a.ini"  //文件名
// #define run_log_b  "run_log_b.ini"

char log_head_buf[log_headoffset_len];
char log_head_b_buf[log_headoffset_len];
u8_t run_log_buf[512];

char temp_buf[log_line_len] = {0};
log_head_t *log_head = (log_head_t *)log_head_buf;

u16_t sw_loghead_rp = 0;

int iFd_run_log_a = 0;  //文件描述符

#ifdef _PLATFORM_BC25_
void print_log_size(void) {
  s32_t size = Ql_FS_GetSize(run_log_a);
  if (size > 0) {
    APP_DEBUG("log file size = %d\r\n", size);
  }
}

int log_head_update(void) {
  s32_t ret = 0;
  u32 writenLen = 0, readenLen = 0;
  iFd_run_log_a = Ql_FS_Open(run_log_a, QL_FS_READ_WRITE);
  if (iFd_run_log_a >= 0) {
    Ql_memset(log_head_b_buf, 0, sizeof(log_head_b_buf));
    ret = Ql_FS_Seek(iFd_run_log_a, 0, QL_FS_FILE_BEGIN);
    ret = Ql_FS_Write(iFd_run_log_a, (u8 *)log_head_buf, sizeof(log_head_buf), &writenLen);
    ret = Ql_FS_Seek(iFd_run_log_a, 0, QL_FS_FILE_BEGIN);
    ret = Ql_FS_Read(iFd_run_log_a, (u8 *)log_head_b_buf, sizeof(log_head_b_buf), &readenLen);

    APP_DEBUG("set:run log read po = %d,write po = %d,l = %d\r\n", log_head->file_logr_pointer, log_head->file_logw_pointer,
              writenLen);
    Ql_FS_Flush(iFd_run_log_a);
    Ql_FS_Close(iFd_run_log_a);
    log_head_t *log_b_head = (log_head_t *)log_head_b_buf;
    if (log_b_head->file_logw_pointer != log_head->file_logw_pointer || \
        log_b_head->file_logr_pointer != log_head->file_logr_pointer) {
      APP_DEBUG("update %s log file head error!\r\n", run_log_a);
      return -1;
    }
    if (writenLen < sizeof(log_head_buf) || \
        readenLen < sizeof(log_head_b_buf)) {   // 写/读log head 出错
      APP_DEBUG("Read/Write %s log file head error!\r\n", run_log_a);
      log_clean();
      return -1;
    }
  } else {  // 打开log文件出错
    APP_DEBUG("Open %s log file error!\r\n", run_log_a);
    log_clean();
    return -1;
  }
  return 0;
}

int log_head_get(void) {
  s32_t ret = 0;
  u32 readenLen = 0;
  iFd_run_log_a = Ql_FS_Open(run_log_a, QL_FS_READ_ONLY);
  if (iFd_run_log_a >= 0) {
    ret = Ql_FS_Seek(iFd_run_log_a, 0, QL_FS_FILE_BEGIN);
    ret = Ql_FS_Read(iFd_run_log_a, (u8 *)log_head_buf, sizeof(log_head_buf), &readenLen);
    Ql_FS_Close(iFd_run_log_a);
    APP_DEBUG("get:run log read po = %d,write po = %d,l = %d\r\n", \
              log_head->file_logr_pointer, log_head->file_logw_pointer, readenLen);
    if (readenLen < sizeof(log_head_buf)) {
      APP_DEBUG("Read %s log file head error!\r\n", run_log_a);
      ret = -1;
    }
  } else {
    APP_DEBUG("Open %s log file error!\r\n", run_log_a);
    ret = -1;
  }
  return ret;
}

int log_init(void) {
  s32_t ret = 0;
  u32 writenLen = 0;

  s32_t log_size_a = Ql_FS_GetSize(run_log_a);
  if (log_size_a > 0) {
    APP_DEBUG("found old log file!\r\n");
    //头信息获取失败
    if (log_head_get() != 0) {
      APP_DEBUG("run log file head err!\r\n");
      Ql_FS_Delete(run_log_a);
      log_size_a = -1;
    } else if ((log_head->file_logw_pointer > log_pointer_size) ||
               (log_head->file_logr_pointer > log_pointer_size)) {
      //文件大小和当前读写指针不匹配
      APP_DEBUG("run log file size err!\r\n");
      Ql_FS_Delete(run_log_a);
      log_size_a = -1;
    }
  }

  if (log_size_a <= 0) {
    APP_DEBUG("run log file create\r\n");
    log_head->file_logw_pointer = 0;
    log_head->file_logr_pointer = 0;
    iFd_run_log_a = Ql_FS_Open(run_log_a, QL_FS_CREATE);   // 读写、创建
    ret = Ql_FS_Seek(iFd_run_log_a, 0, QL_FS_FILE_BEGIN);
    ret = Ql_FS_Write(iFd_run_log_a, (u8 *)log_head_buf, sizeof(log_head_buf), &writenLen);
    Ql_FS_Flush(iFd_run_log_a);
    Ql_FS_Close(iFd_run_log_a);
    log_size_a = Ql_FS_GetSize(run_log_a);
  }
  sw_loghead_rp = log_head->file_logr_pointer;
  return log_size_a;
}

void log_save(char *str, ...) {
  u16_t  len = 0;
  va_list args;
//  u32_t run_log_size_a = 0;
//  u32_t run_log_size_b = 0;
  u16_t pointer_r = 0, pointer_w = 0;

  Ql_memset(temp_buf, 0, sizeof(temp_buf));

  pointer_r = log_head->file_logr_pointer;
  pointer_w = log_head->file_logw_pointer;

  s32_t ret = 0;
  u32 writenLen = 0;

  // 时间戳
  char time_stamp[23] = {0};
  // 该函数用于获取本地时间

  ST_Time time;

  if (Ql_GetRealLocalTime(&time) == NULL) {
    time.year = 2015;
    time.month = 1;
    time.day = 1;
    time.hour = 0;
    time.minute = 0;
    time.second = 0;
    time.timezone = 0;
  }

  APP_DEBUG("Ql_GetRealLocalTime(%d.%02d.%02d %02d:%02d:%02d timezone=%02d)=%d\r\n",
            time.year, time.month, time.day, time.hour, time.minute, time.second, time.timezone, ret);
  Ql_sprintf(time_stamp, "%04d-%02d-%02d %02d:%02d:%02d ", time.year, time.month,
             time.day, time.hour, time.minute, time.second);

  if (str == NULL || Ql_strlen(str) <= 0) {  // 不记录空log
    return;
  }

  Ql_memset(run_log_buf, 0, sizeof(run_log_buf));

  va_start(args, str);        // 兼容格式化输入log,保留这部分注释
//  len = vsprintf((char *)run_log_buf, str, args);
  Ql_vsprintf((char *)run_log_buf, str, args);
  va_end(args);

  len = Ql_strlen((char *)run_log_buf);

  APP_DEBUG("%s len %d\r\n", run_log_buf, len);

  if ((len + Ql_strlen(time_stamp) + 2) < (log_line_len - 1)) {
    Ql_strcpy(temp_buf, time_stamp);
    len = Ql_strlen((char *)temp_buf);
    Ql_strcpy(&temp_buf[len], (char *)run_log_buf);
    len = Ql_strlen((char *)temp_buf);
    APP_DEBUG("%s len %d\r\n", temp_buf, len);
    Ql_strcpy(&temp_buf[len], "\r\n");
    len = Ql_strlen((char *)temp_buf);
  }
  // 行长度溢出
  else {
    APP_DEBUG("run log file line full!\r\n");
    return;
//    goto END;
  }

  if (((log_head->file_logw_pointer + 1) % log_pointer_size) == log_head->file_logr_pointer) {
    APP_DEBUG("run log file full!\r\n");    // 文件满
    if (log_head->file_logr_pointer == sw_loghead_rp) {
      APP_DEBUG("run log tail after!\r\n"); // 跟踪日志进度
      sw_loghead_rp = (sw_loghead_rp + 1) % log_pointer_size;
    }
    log_head->file_logr_pointer = (log_head->file_logr_pointer + 1) % log_pointer_size;
    log_clean();
  }

  iFd_run_log_a = Ql_FS_Open(run_log_a, QL_FS_READ_WRITE);
  if (iFd_run_log_a < 0) {
    APP_DEBUG("open run log save fail!\r\n");
    return;
  }

  ret = Ql_FS_Seek(iFd_run_log_a, log_head->file_logw_pointer * log_line_len + log_headoffset_len, QL_FS_FILE_BEGIN);
  ret = Ql_FS_Write(iFd_run_log_a, (u8 *)temp_buf, log_line_len, &writenLen);
  Ql_FS_Flush(iFd_run_log_a);
  Ql_FS_Close(iFd_run_log_a);

  // 写指针加1
  log_head->file_logw_pointer = (log_head->file_logw_pointer + 1) % log_pointer_size;
  // 更新日志文件句柄
  if (log_head_update() != 0) {
    APP_DEBUG("log file update fail!\r\n");
    // 更新失败 恢复上一次设置
    log_head->file_logw_pointer = pointer_w;
    log_head->file_logr_pointer = pointer_r;
    log_reset();
  }
}

u16_t log_get(Buffer_t *buf) {
//  int check_a = 1;
  int log_size_a = 0;
  int length  = 0;
  s32_t ret = 0;
  u32 readenLen = 0;

  if (buf == NULL) {
    APP_DEBUG("log save buf is null\r\n");
    return 0;
  }
  
  log_size_a = Ql_FS_GetSize(run_log_a);
  if (log_size_a <= 0) {
    APP_DEBUG("run log file is null\r\n");
    log_clean();
    return 0;
  }

  iFd_run_log_a = Ql_FS_Open(run_log_a, QL_FS_READ_ONLY);
  Ql_memset(temp_buf, 0, sizeof(temp_buf));

//  APP_DEBUG("file_logw_pointer: %d sw_loghead_rp: %d\r\n", log_head->file_logw_pointer, sw_loghead_rp);

  if (log_head->file_logw_pointer != sw_loghead_rp) {  // 写的节点和读的节点不同才读
    ret = Ql_FS_Seek(iFd_run_log_a, sw_loghead_rp * log_line_len + log_headoffset_len, QL_FS_FILE_BEGIN);
    ret = Ql_FS_Read(iFd_run_log_a, (u8 *)temp_buf, log_line_len, &readenLen);
    if (readenLen > 0) {
      Ql_strcpy((char *)buf->payload, "LOG:");
      Ql_strcpy((char *)(buf->payload + Ql_strlen((char *)buf->payload)), temp_buf);
//      Ql_strcpy((char *)(buf->payload + Ql_strlen((char *)buf->payload)), "\r\n");
      length = Ql_strlen((char *)buf->payload);
    }
    // log_head->file_logr_pointer = (log_head->file_logr_pointer + 1)%log_pointer_size;
    sw_loghead_rp = (sw_loghead_rp + 1) % log_pointer_size;
//    APP_DEBUG("new sw_loghead_rp: %d\r\n", sw_loghead_rp);
  }
  Ql_FS_Close(iFd_run_log_a);

  return length;
}

void log_clean(void) {
  Ql_FS_Delete(run_log_a);
  log_init();
}
#endif

#ifdef _PLATFORM_L610_
void print_log_size(void) {
  s32_t size = fibo_file_getSize(run_log_a);
  if (size > 0) {
    APP_DEBUG("log file size = %ld\r\n", size);
  }
}

int log_head_update(void) {
  s32_t ret = 0;
  s32_t writenLen = 0, readenLen = 0;
  iFd_run_log_a = fibo_file_open(run_log_a, FS_O_RDWR);
  if (iFd_run_log_a >= 0) {
    r_memset(log_head_b_buf, 0, sizeof(log_head_b_buf));
    ret = fibo_file_seek(iFd_run_log_a, 0, FS_SEEK_SET);
    writenLen = fibo_file_write(iFd_run_log_a, (u8_t *)log_head_buf, sizeof(log_head_buf));
    ret = fibo_file_seek(iFd_run_log_a, 0, FS_SEEK_SET);
    readenLen = fibo_file_read(iFd_run_log_a, (u8_t *)log_head_b_buf, sizeof(log_head_b_buf));

    APP_DEBUG("set:run log read po = %d,write po = %d,l = %ld\r\n", log_head->file_logr_pointer, log_head->file_logw_pointer,
              writenLen);
    ret = fibo_file_fsync(iFd_run_log_a);
    ret = fibo_file_close(iFd_run_log_a);
    log_head_t *log_b_head = (log_head_t *)log_head_b_buf;
    if (log_b_head->file_logw_pointer != log_head->file_logw_pointer || \
        log_b_head->file_logr_pointer != log_head->file_logr_pointer) {
      APP_DEBUG("update %s log file head error!\r\n", run_log_a);
      return -1;
    }
    if (writenLen < sizeof(log_head_buf) || \
        readenLen < sizeof(log_head_b_buf)) {   // 写/读log head 出错
      APP_DEBUG("Read/Write %s log file head error!\r\n", run_log_a);
      log_clean();
      return -1;
    }
  } else {  // 打开log文件出错
    APP_DEBUG("Open %s log file error!\r\n", run_log_a);
    log_clean();
    return -1;
  }
  return 0;
}

int log_head_get(void) {
  s32_t ret = 0;
  s32_t readenLen = 0;
  iFd_run_log_a = fibo_file_open(run_log_a, FS_O_RDONLY);
  if (iFd_run_log_a >= 0) {
    ret = fibo_file_seek(iFd_run_log_a, 0, FS_SEEK_SET);
    readenLen = fibo_file_read(iFd_run_log_a, (u8_t *)log_head_buf, sizeof(log_head_buf));
    ret = fibo_file_close(iFd_run_log_a);
    APP_DEBUG("get:run log read po = %d,write po = %d,l = %ld\r\n", \
              log_head->file_logr_pointer, log_head->file_logw_pointer, readenLen);
    if (readenLen < sizeof(log_head_buf)) {
      APP_DEBUG("Read %s log file head error!\r\n", run_log_a);
      ret = -1;
    }
  } else {
    APP_DEBUG("Open %s log file error!\r\n", run_log_a);
    ret = -1;
  }
  return ret;
}

int log_init(void) {
  s32_t ret = 0;
  s32_t log_size_a = fibo_file_getSize(run_log_a);
  if (log_size_a > 0) {
    APP_DEBUG("found old log file!\r\n");
    //头信息获取失败
    if (log_head_get() != 0) {
      APP_DEBUG("run log file head err!\r\n");
      fibo_file_delete(run_log_a);
      log_size_a = -1;
    } else if ((log_head->file_logw_pointer > log_pointer_size) ||
               (log_head->file_logr_pointer > log_pointer_size)) {
      //文件大小和当前读写指针不匹配
      APP_DEBUG("run log file size err!\r\n");
      fibo_file_delete(run_log_a);
      log_size_a = -1;
    }
  }

  if (log_size_a <= 0) {
    APP_DEBUG("run log file create\r\n");
    log_head->file_logw_pointer = 0;
    log_head->file_logr_pointer = 0;
    iFd_run_log_a = fibo_file_open(run_log_a, FS_O_RDWR | FS_O_CREAT | FS_O_TRUNC);   // 读写、创建
    if (iFd_run_log_a < 0) {
      APP_DEBUG("log file create error\r\n");
      return log_size_a;
    }
    ret = fibo_file_seek(iFd_run_log_a, 0, FS_SEEK_SET);
    if (ret < 0) {
      APP_DEBUG("log file create seek error\r\n");
      ret = fibo_file_close(iFd_run_log_a);
      if (ret < 0) {
        APP_DEBUG("log file create close error\r\n");
      }
      return log_size_a;
    }
    ret = fibo_file_write(iFd_run_log_a, (u8_t *)log_head_buf, sizeof(log_head_buf));
    if (ret < 0) {
      APP_DEBUG("log file create write head error\r\n");
      ret = fibo_file_close(iFd_run_log_a);
      if (ret < 0) {
        APP_DEBUG("log file create close error\r\n");
      }
      return log_size_a;
    }
    ret = fibo_file_fsync(iFd_run_log_a);
    if (ret < 0) {
      APP_DEBUG("log file create sync error\r\n");
      ret = fibo_file_close(iFd_run_log_a);
      if (ret < 0) {
        APP_DEBUG("log file create close error\r\n");
      }
      return log_size_a;
    }
    ret = fibo_file_close(iFd_run_log_a);
    if (ret < 0) {
      APP_DEBUG("log file create close error\r\n");
      return log_size_a;
    }
    log_size_a = fibo_file_getSize(run_log_a);
  }
  sw_loghead_rp = log_head->file_logr_pointer;
  return log_size_a;
}

void log_save(char *str, ...) {
  u16_t  len = 0;
  va_list args;
  u16_t pointer_r = 0, pointer_w = 0;

  r_memset(temp_buf, 0, sizeof(temp_buf));
    
  pointer_r = log_head->file_logr_pointer;
  pointer_w = log_head->file_logw_pointer;
    
  s32_t ret = 0;
  u32_t writenLen = 0;
    
  // 时间戳
  char time_stamp[29] = {0};
  // 该函数用于获取本地时间
  hal_rtc_time_t time;
  ret = fibo_getRTC(&time);
  if (ret < 0) {
    time.year = 2015;
    time.month = 1;
    time.day = 1;
    time.hour = 0;
    time.min = 0;
    time.sec = 0;
    time.wDay = 4;
  }

  //s8_t ntimezone = fibo_getRTC_timezone();
    
//  APP_DEBUG("fibo_getRTC(20%02d-%02d-%02d %02d:%02d:%02d week=%d timezone=%02d)=%ld\r\n",
//             time.year, time.month, time.day, time.hour, time.min, time.sec, time.wDay, ntimezone, ret);

//  fibo_vsnprintf();
  snprintf(time_stamp, sizeof(time_stamp), "20%02d-%02d-%02d %02d:%02d:%02d ", time.year, time.month,
             time.day, time.hour, time.min, time.sec);
  if (str == NULL || r_strlen(str) <= 0) {  // 不记录空log
    return;
  }
    
  r_memset(run_log_buf, 0, sizeof(run_log_buf));
    
  va_start(args, str);        // 兼容格式化输入log,保留这部分注释
  fibo_vsnprintf((char *)run_log_buf, sizeof(run_log_buf), str, args);
  va_end(args);
    
  len = r_strlen((char *)run_log_buf);
    
//  APP_DEBUG("%s len %d\r\n", run_log_buf, len);
    
  if ((len + r_strlen(time_stamp) + 2) < (log_line_len - 1)) {
    r_strcpy(temp_buf, time_stamp);
    len = r_strlen((char *)temp_buf);
    r_strcpy(&temp_buf[len], (char *)run_log_buf);
    len = r_strlen((char *)temp_buf);
    APP_DEBUG("%s len %d\r\n", temp_buf, len);
    r_strcpy(&temp_buf[len], "\r\n");
    len = r_strlen((char *)temp_buf);
  } else {  // 行长度溢出
    APP_DEBUG("run log file line full!\r\n");
    return;
  }
    
  if (((log_head->file_logw_pointer + 1) % log_pointer_size) == log_head->file_logr_pointer) {
    APP_DEBUG("run log file full!\r\n");    // 文件满
    if (log_head->file_logr_pointer == sw_loghead_rp) {
      APP_DEBUG("run log tail after!\r\n"); // 跟踪日志进度
      sw_loghead_rp = (sw_loghead_rp + 1) % log_pointer_size;
    }
    log_head->file_logr_pointer = (log_head->file_logr_pointer + 1) % log_pointer_size;
    log_clean();
  }
    
  iFd_run_log_a = fibo_file_open(run_log_a, FS_O_RDWR);
  if (iFd_run_log_a < 0) {
    APP_DEBUG("open run log save fail!\r\n");
    return;
  }
    
  ret = fibo_file_seek(iFd_run_log_a, log_head->file_logw_pointer * log_line_len + log_headoffset_len, FS_SEEK_SET);
  writenLen = fibo_file_write(iFd_run_log_a, (u8_t *)temp_buf, log_line_len);
  if (writenLen != log_line_len) {
    APP_DEBUG("Write log %s fail len: %ld!\r\n", temp_buf, writenLen);
  }
  ret = fibo_file_fsync(iFd_run_log_a);
  ret = fibo_file_close(iFd_run_log_a);

  // 写指针加1
  log_head->file_logw_pointer = (log_head->file_logw_pointer + 1) % log_pointer_size;
  // 更新日志文件句柄
  if (log_head_update() != 0) {
    APP_DEBUG("log file update fail!\r\n");
    // 更新失败 恢复上一次设置
    log_head->file_logw_pointer = pointer_w;
    log_head->file_logr_pointer = pointer_r;
    log_reset();
  }
}

u16_t log_get(Buffer_t *buf) {
  int log_size_a = 0;
  int length  = 0;
  s32_t ret = 0;
  u32_t readenLen = 0;

  if (buf == NULL) {
    APP_DEBUG("log save buf is null\r\n");
    return 0;
  }

  log_size_a = fibo_file_getSize(run_log_a);
  if (log_size_a <= 0) {
    APP_DEBUG("run log file is null\r\n");
    log_clean();
    return 0;
  }

  iFd_run_log_a = fibo_file_open(run_log_a, FS_O_RDONLY);
  r_memset(temp_buf, 0, sizeof(temp_buf));
//  APP_DEBUG("file_logw_pointer: %d sw_loghead_rp: %d\r\n", log_head->file_logw_pointer, sw_loghead_rp);
  if (log_head->file_logw_pointer != sw_loghead_rp) {  // 写的节点和读的节点不同才读
    ret = fibo_file_seek(iFd_run_log_a, sw_loghead_rp * log_line_len + log_headoffset_len, FS_SEEK_SET);
    readenLen = fibo_file_read(iFd_run_log_a, (u8_t *)temp_buf, log_line_len);
//  APP_DEBUG("log %s\r\n", temp_buf);
    if (readenLen > 0) {
      r_strcpy((char *)buf->payload, "LOG:");
      r_strcpy((char *)(buf->payload + r_strlen((char *)buf->payload)), temp_buf);
//    r_strcpy((char *)(buf->payload + r_strlen((char *)buf->payload)), "\r\n");
      length = r_strlen((char *)buf->payload);
    }
//  log_head->file_logr_pointer = (log_head->file_logr_pointer + 1)%log_pointer_size;
    sw_loghead_rp = (sw_loghead_rp + 1) % log_pointer_size;
//  APP_DEBUG("new sw_loghead_rp: %d\r\n", sw_loghead_rp);
  }
  fibo_file_close(iFd_run_log_a);    
  return length;
}

void log_clean(void) {
  fibo_file_delete(run_log_a);
  log_init();
}
#endif

void log_reset(void) {
  if (log_head != NULL &&
      log_head->file_logr_pointer < log_pointer_size &&
      log_head->file_logw_pointer < log_pointer_size) {
    sw_loghead_rp = log_head->file_logr_pointer;
  }
}

*/

