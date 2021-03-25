/******************************************************************************           
* name:             anti_reflux.c           
* introduce:        anti-reflux 
* author:           Luee                                     
******************************************************************************/ 
#include "fibo_opencpu.h"
#include "eyblib_memory.h"
#include "eyblib_r_stdlib.h"
#include "eyblib_swap.h"

#include "eybpub_utility.h"
#include "eybpub_Debug.h"
#include "eybpub_run_log.h"
#include "eybpub_SysPara_File.h"
#include "eybpub_data_collector_parameter_table.h"

#include "grid_tool.h"
#include "anti_reflux.h"
#include "Modbus.h"
#include "ModbusDevice.h"
#include "Protocol.h"
#include "_0409.h"
#include "_040B.h"
#include "MosoAdj.h"
#include "eyblib_CRC.h"
#include "eybapp_appTask.h"

#define REALTIME_METER_READ_COUNTER 10


static u8 device_addr=1;

typedef struct {
  u8_t addr;
  u8_t fun;
  u16_t st_addr;
  u16_t num;
  u8_t bytes;
  u16_t payload[];
} modbus_wr2_t;

#pragma pack(1)
typedef struct {
  u8_t addr;
  u8_t fun;
  u16_t st_addr;
  u16_t num;
  u8_t bytes;
  u16_t apowerl;
  u16_t apowerh;
  u16_t bpowerl;
  u16_t bpowerh;
  u16_t cpowerl;
  u16_t cpowerh;
  u16_t totall;
  u16_t totalh;
  u16_t crc16;
} modbus_wr_t;
#pragma pack()

typedef struct {
  u8_t addr;
  u8_t fun;
  u16_t st_addr;
  u16_t num;
  u16_t crc16;
} modbus_wr_response_t;

#pragma pack(1)
typedef struct {
  u8_t addr;
  u8_t fun;
  u16_t st_addr;
  u16_t num;
  u16_t crc16;
} modbus_rd_t;
#pragma pack()

typedef struct {
  u8_t addr;
  u8_t fun;
  u8_t bytes;
  u16_t apowerl;
  u16_t apowerh;
  u16_t bpowerl;
  u16_t bpowerh;
  u16_t cpowerl;
  u16_t cpowerh;
  u16_t totall;
  u16_t totalh;
  u16_t crc16;
} modbus_rd_response_t;

typedef struct {
  u8_t addr;
  u8_t fun;
  u8_t err;
  u16_t crc16;
} modbus_err_t;

//u8_t anti_data[64]={0};
//buf_t *anti_buf=(buf_t *)anti_data;


//////////////////////////////////////
typedef struct
{
    Device_t *dev;
}InvertList_t;

static MeterValue_t meterPower;
static ListHandler_t invertDevice;
static buf_t anti_ack_buf={0};
static u8 online_dev_count=0;
static u8 addr_index=0;
static u8 online_dev_addr_tab[64];


static void Anti_reflux_init(void);
static u8_t checkDeviceOnline(void);
static u8_t ArtiReflux_InvDataAck(Device_t *dev);
static u8_t invterLockStatus(void *load, void *val);
static u8_t invterUnlock(void *load, void *val);
static u8_t ArtiReflux_meterDataAck(Device_t *dev);
static u8_t invterAdd(void *load, void *val);
static u8_t invterRemove(void *load, void *val);
static void deviceDel(Device_t *dev);
static void anti_relex_data_process(void);
void meter_read(void);
static void realtime_meter_read(void);
static u8_t anti_ack(Device_t *dev);
static u8_t anti_trans(u8 *data_ptr ,u8 data_len);
static u8 get_online_count(void);

static const AntiReflux_t *deviceTab[] = {
//static const AntiReflux_t deviceTab[] = {
     &MOSO_0211,
     &MOSO_0212,
     &Device_0409,
     &Device_040B
};



/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
void proc_anti_reflux_task(s32_t taskId)
{
    ST_MSG msg;

    APP_DEBUG("anti-reflux task run!!\r\n");
    Anti_reflux_init();

    //while(1){
    //    fibo_taskSleep(500);
    //}

    while(1){
        //fibo_taskSleep(500);
        fibo_queue_get(ANTI_REFLUX_TASK, (void *)&msg, 0);
        switch (msg.message) {
        case ANTI_REFLUX_DATA_PROCESS:
            anti_relex_data_process();
        break;
        case ANTI_REFLUX_TIMER_ID:
            APP_DEBUG("anti reflux time running\r\n");
            get_online_count();
            realtime_meter_read();          
        break;
        }   
    }
    fibo_thread_delete();
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
//收到逆变器数据应答
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //收到电表回复
        APP_DEBUG("get meter return");
#ifdef GETANTIBUF_FROMACK
        get_antibuf(cmd->ack.payload,cmd->ack.lenght); 
#endif  
    }	
    //Device_remove(dev);
    list_nodeDelete(&dev->cmdList, cmd);
    return 0;
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static u8_t anti_trans(u8 *data_ptr ,u8 data_len)
{
    Device_t *dev;
    DeviceCmd_t *cmd;

    APP_DEBUG("anti reflux trans\r\n");

    dev = list_nodeApply(sizeof(Device_t));
    cmd = list_nodeApply(sizeof(DeviceCmd_t));

    cmd->waitTime = 2000;       //1500;     // 1500=1.5 sec
    cmd->state = 0;

    cmd->ack.size = DEVICE_ACK_SIZE;
    cmd->ack.payload = memory_apply(cmd->ack.size);
    cmd->ack.lenght = 0;
    cmd->cmd.size = data_len;
    cmd->cmd.lenght = cmd->cmd.size;
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 重新申请存放输入指令的内存
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    //out_put_buffer((char *)cmd->cmd.payload,cmd->cmd.size);
    //APP_PRINT("\r\n");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = null;  // 配置一个执行该指令的设备
    dev->callBack = anti_ack;  // 设置透传设备需要的返回函数
   // dev->callBack = NULL;  // 设置透传设备需要的返回函数
    //dev->explain = esp;   // 设备携带指令节点
    dev->type = DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
   // Device_inset(dev);    // 将需要执行指令的设备放入Devicelist，交由deviceCmdSend处理
    Device_add(dev);
    return 1;

}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static u8 get_online_count(void)
{
    DeviceOnlineHead_t *online = null;
    u8 i;

    i=0;
    if(onlineDeviceList.count==0){
        r_memset(online_dev_addr_tab,0,64);
        online_dev_count=0;
        return online_dev_count;
    }
    if (onlineDeviceList.count==online_dev_count){
        return online_dev_count;
    }
    online_dev_count=onlineDeviceList.count;
    r_memset(online_dev_addr_tab,0,64);
    do{
        online = list_nextData(&onlineDeviceList, online);
        if(online!=null){
            online_dev_addr_tab[i]=online->addr;
            i++;
        }  
    }while(online!=null && i<64); 
    APP_DEBUG("online_dev_addr_tab:\r\n");
    print_buf(online_dev_addr_tab,sizeof(online_dev_addr_tab));  
    return online_dev_count;
}


/*******************************************************************************            
* introduce:   闂侇偄妫楄ぐ澶嬫交閺傛寧绀€闁轰胶澧楀畵浣瑰緞閸曨厽鍊�     
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static void anti_relex_data_process(void)
{
    static Device_t *currentDevice = NULL;
    static DeviceCmd_t *currentCmd = NULL;

    DeviceOnlineHead_t *online = null;

    static u8 online_count=0;
 
    //no meter rec data ,or no online device
    if(rcveBuf.payload[0]!=METER_ADDR){     
        return;
    }

    if(online_dev_count==0)
        return;
    else{
        if(++addr_index>=online_dev_count)
            addr_index=0;
        device_addr=online_dev_addr_tab[addr_index];    //得到要发送的逆变器地址
    }
	       
    APP_DEBUG("MODBUS_DATA_GET\r\n");
    print_buf(antibuf.payload,antibuf.lenght);         
    modbus_wr2_t *modbus_buf=(modbus_wr2_t *)antibuf.payload;
    APP_DEBUG("modbus_buf->fun=%d modbus_buf->addr=%d \r\n",modbus_buf->fun,modbus_buf->addr);

    modbus_rd_response_t *meter_rec_buf=(modbus_rd_response_t *)antibuf.payload;
    u8 *send_buf=memory_apply(25);
    r_memset(send_buf,'\0',25);
    modbus_wr_t *anti_send_buf=(modbus_wr_t *)send_buf;
    APP_DEBUG("meter_rec_buf->fun=%d meter_rec_buf->addr=%d meter_rec_buf->bytes=%d \r\n",meter_rec_buf->fun,meter_rec_buf->addr,meter_rec_buf->bytes);

    //send_buf =(u8 *)anti_send_buf+7;
    //antibuf.payload =(u8 *)meter_rec_buf+3;
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //閻庣數澧楅〃鍛村春瀹€鍕仺濞达絽鍟ㄩ埀顒€鍟村顐︽偋閸繄銈�
    r_memcpy(&anti_send_buf->bytes+1,&meter_rec_buf->bytes+1,meter_rec_buf->bytes);

    anti_send_buf->addr=device_addr;          
    anti_send_buf->fun=MODBUS_RTU_FUN_WR;
    anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
    anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
    anti_send_buf->bytes=meter_rec_buf->bytes;
    anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
    //Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
    anti_trans((u8_t *)send_buf, sizeof(modbus_wr_t));
    
    APP_DEBUG("send anti reflux data\r\n");
    print_buf(send_buf,25);
    memory_release(send_buf);

}

 /*          
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂佽　鍋撻悹鍝勬惈閻撳倻鈧鍠栭崐鎼佹偉閸洖绠伴柛銉檮婵拷
            }else{
                
                if(modbus_buf->fun==MODBUS_RTU_FUN_RD){
                    modbus_rd_response_t *meter_rec_buf=(modbus_rd_response_t *)antibuf.payload;
                    APP_DEBUG("meter_rec_buf->fun=%d meter_rec_buf->addr=%d meter_rec_buf->bytes=%d \r\n",meter_rec_buf->fun,meter_rec_buf->addr,meter_rec_buf->bytes);
                    //u8 send_buf[25]={0};
                    u8 *send_buf=fibo_malloc(25);
                    modbus_wr_t *anti_send_buf=(modbus_wr_t *)send_buf;
                    if(modbus_buf->addr==METER_ADDR && meter_rec_buf->bytes>=2 && meter_rec_buf->bytes<=16){
                        
                        APP_DEBUG("get meter data success\r\n");
                        //print_buf(antibuf.payload,sizeof(antibuf.payload));

                        send_buf +=7;
                        //meter_rec_buf+=3;
                        antibuf.payload +=3;
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //閻庣數澧楅〃鍛村春瀹€鍕仺濞达絽鍟ㄩ埀顒€鍟村顐︽偋閸繄銈�
                        anti_send_buf->addr=device_addr;          
                        anti_send_buf->fun=MODBUS_RTU_FUN_WR;
                        anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
                        anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
                        anti_send_buf->bytes=meter_rec_buf->bytes;
                        anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
                        Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
                        
                        APP_DEBUG("send anti reflux data\r\n");
                        print_buf(send_buf,sizeof(send_buf));
                        fibo_free(send_buf);
                    }
                }
                if(modbus_buf->fun==MODBUS_RTU_FUN_WR){
                    
                    reg_addr=ENDIAN_BIG_LITTLE_16(modbus_buf->st_addr);
                    if(reg_addr==ANTI_REFLUX_REG_ADDR){
                        //婵犮垼娉涚€氼噣骞冩繝鍥ㄧ劵闁糕剝顨呯紞渚€鏌涢敐鍐ㄥ閻㈩垱鎸冲銊╂嚋閸偅顔嶉梺鍦閹凤拷
                        
                    }
                }
            }   //else
 //       }   //if(NULL != currentCmd)
 //   }   //if(NULL != currentDevice)
}   //static void anti_relex_data_process(void)
*/

/*
static void anti_relex_data_process(void)
{
    static Device_t *currentDevice = NULL;
    static DeviceCmd_t *currentCmd = NULL;
    u16 reg_addr;
    //s_device->buf->payload        
    APP_DEBUG("MODBUS_DATA_GET\r\n");
    currentDevice = list_nextData(&DeviceList, currentDevice);      //閻庣數澧楅〃鍛村春瀹€鈧幏瀣箚瑜滃Σ鐢告煠閸濆嫬鈧宕戦敓锟�
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //閻庣數澧楅〃鍛村春瀹€鍐︿汗闁规儳鍟块·鍛存偣娴ｇ懓鍔ゆい锔诲墴閹嫮绮欑捄銊ョ暔闂佹眹鍔岀€氼厾鈧灚纰嶇粋鎺楁晸閿燂拷  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂佽　鍋撻悹鍝勬惈閻撳倻鈧鍠栭崐鎼佹偉閸洖绠伴柛銉檮婵拷
            }else{
                
                if(modbus_buf->fun==MODBUS_RTU_FUN_RD){
                    
                    modbus_rd_response_t *meter_rec_buf=(modbus_rd_response_t *)currentCmd->cmd.payload;
                    
                    //u8 send_buf[25]={0};
                    u8 *send_buf=fibo_malloc(25);
                    modbus_wr_t *anti_send_buf=(modbus_wr_t *)send_buf;
                    if(modbus_buf->addr==METER_ADDR && meter_rec_buf->bytes>=2 && meter_rec_buf->bytes<=16){
                        
                        APP_DEBUG("get meter data success\r\n");
                        send_buf +=7;
                        //meter_rec_buf+=3;
                        currentCmd->cmd.payload +=3;
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //閻庣數澧楅〃鍛村春瀹€鍕仺濞达絽鍟ㄩ埀顒€鍟村顐︽偋閸繄銈�
                        anti_send_buf->addr=device_addr;          
                        anti_send_buf->fun=MODBUS_RTU_FUN_WR;
                        anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
                        anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
                        anti_send_buf->bytes=meter_rec_buf->bytes;
                        anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
                        Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
                        fibo_free(send_buf);
                    }
                }
                if(modbus_buf->fun==MODBUS_RTU_FUN_WR){
                    
                    reg_addr=ENDIAN_BIG_LITTLE_16(modbus_buf->st_addr);
                    if(reg_addr==ANTI_REFLUX_REG_ADDR){
                        //婵犮垼娉涚€氼噣骞冩繝鍥ㄧ劵闁糕剝顨呯紞渚€鏌涢敐鍐ㄥ閻㈩垱鎸冲銊╂嚋閸偅顔嶉梺鍦閹凤拷
                        
                    }
                }
            }   //else
        }   //if(NULL != currentCmd)
    }   //if(NULL != currentDevice)
}   //static void anti_relex_data_process(void)
*/
/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
void meter_read(void)
{
    u8 meter_buf[8]={0};
    modbus_rd_t *meter_read_t=(modbus_rd_t *) meter_buf;

    meter_read_t->addr=METER_ADDR;
    meter_read_t->fun=0x03;
    meter_read_t->st_addr=ENDIAN_BIG_LITTLE_16(METER_REG_ADDR);
    meter_read_t->num=ENDIAN_BIG_LITTLE_16(8);
    meter_read_t->crc16=crc16_standard(CRC_RTU, (u8_t *)meter_read_t, sizeof(modbus_rd_t) - sizeof(meter_read_t->crc16));

    //Uart_write(meter_buf, sizeof(modbus_rd_t));
    anti_trans(meter_buf, sizeof(modbus_rd_t));
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static void realtime_meter_read(void)
{
    static u8 counter=REALTIME_METER_READ_COUNTER;

    if(counter)
        counter--;
    if(counter==0 && online_dev_count!=0){
        counter=REALTIME_METER_READ_COUNTER;
        meter_read();
    }
}

/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static u8_t checkDeviceOnline(void)
{
    static int onlineDeviceCount = 0;
    //闂備線娼荤拹鐔煎礉鎼淬劌鍚归幖杈剧稻婵ジ鏌熼鍡楁湰鏍″┑鐐村灦閹稿摜绮旈崼鏇熷仱闁瑰鍋熼々鏌ユ煥閻曞倹瀚�0,
    //闂備礁鎼悧鍡浰囬柆宥呮瀬闁告縿鍎查崑姗€鏌ㄩ悤鍌涘
    if (onlineDeviceList.count !=  onlineDeviceCount 
        && (MeterDevice.head != null)){
        onlineDeviceCount = 0;

        if (onlineDeviceList.count > 1){
            int i;
            
            AntiRefluxDeviceInfo_t *info;
            Device_t *meterDev = null;
            DeviceOnlineHead_t *online = null;

            online = list_nextData(&onlineDeviceList, null);
            Device_removeType(DEVICE_ARTI);
            list_delete(&invertDevice);

            while (online != null)
            {
                for (i = 0; i < sizeof(deviceTab)/sizeof(deviceTab[0]); i++)
                {
                    if (deviceTab[i]->tab.code == online->code)
                    {
                        //if (0x0200 == (online->code & 0x0200))
                        if (0x0212 == (online->code))           //濠电偞鍨堕幐鎼佀囬崡鐏昏寰勯幇顓炰缓婵炶揪绲介崯銊╁焵椤掑﹥瀚�
                        {
                            InvertList_t *deviceList;
                            Device_t *invsterDev = null;

                            invsterDev = list_nodeApply(sizeof(Device_t));
                            info = memory_apply(sizeof(AntiRefluxDeviceInfo_t));

                            info->addr = online->addr;
                            info->aux =(AntiReflux_t *) &deviceTab[i]->tab;
                            invsterDev->cfg = null;
                            invsterDev->callBack = ArtiReflux_InvDataAck;
                            invsterDev->explain = info;
                            invsterDev->type = DEVICE_ARTI;
                            list_init(&invsterDev->cmdList);
                            deviceList = list_nodeApply(sizeof(InvertList_t));
                            deviceList->dev = invsterDev;
                            list_bottomInsert(&invertDevice, deviceList);

                            Modbus_GetCmdAdd(&invsterDev->cmdList, online->addr, deviceTab[i]->tab.cmdTab, deviceTab[i]->tab.count*sizeof(ModbusGetCmd_t));
                        }
                        else if (0x0400 == (online->code & 0x0400))
                        {
                            if (meterDev == null)
                            {
                                meterDev = list_nodeApply(sizeof(Device_t));
                                info = memory_apply(sizeof(AntiRefluxDeviceInfo_t));

                                info->addr = online->addr;
                                info->aux = (AntiReflux_t *) &deviceTab[i]->tab;
                                meterDev->cfg = null;
                                meterDev->callBack = ArtiReflux_meterDataAck;
                                meterDev->explain = info;
                                meterDev->type = DEVICE_ARTI;
                                list_init(&meterDev->cmdList);
                            }

                            Modbus_GetCmdAdd(&meterDev->cmdList, online->addr, deviceTab[i]->tab.cmdTab, deviceTab[i]->tab.count*sizeof(ModbusGetCmd_t));
                        }
                    }
                }
                online = list_nextData(&onlineDeviceList, online);
            }

            if ((meterDev != null)
                && (invertDevice.count > 0)
            )
            {
                onlineDeviceCount = onlineDeviceList.count;
                Device_add(meterDev);
                list_trans(&invertDevice, invterAdd, null);
            }
            else
            {
                deviceDel(meterDev);
                list_trans(&invertDevice, invterRemove, null);
                list_delete(&invertDevice);
            }   
        }
        return 1;
    }

    return 0;
}

static u8_t invterAdd(void *load, void *val)
{
    InvertList_t *deviceList = load;

    Device_add(deviceList->dev);

    return 1;
}

static u8_t invterRemove(void *load, void *val)
{
    InvertList_t *deviceList = load;

    deviceDel(deviceList->dev);

    return 1;
}

static void deviceDel(Device_t *dev)
{
    if (dev != null)
    {
        DeviceCmd_clear(dev);
        list_nodeDelete(null, dev);
    }
}

/*******************************************************************************
  * @note   inv Data Collect
  * @param  None
  * @retval None
*******************************************************************************/
//static void ArtiReflux_InvDataAck(Device_t *dev)
static u8_t ArtiReflux_InvDataAck(Device_t *dev)
{
    AntiRefluxDeviceInfo_t *info = (AntiRefluxDeviceInfo_t*)dev->explain;
    u8_t result = 0;

    list_trans(&dev->cmdList, modbusDataCheck, &result);

    if (result == 0)
    {
        if (meterPower.state == 1)
        {
            info->aux->func(invertDevice.count, dev, &meterPower);
            if (meterPower.state != 1)
            {
                return result;
            }
        }
    }

    dev->lock = 0;
    return result;
}

static u8_t invterLockStatus(void *load, void *val)
{
    InvertList_t *invert = load;
    u8_t *pVal = val;

    if (invert->dev->lock == 0)
    {
        *pVal = 1;

        return 0;
    }

    return 1;
}

static u8_t invterUnlock(void *load, void *val)
{
    InvertList_t *invert = load;

    invert->dev->lock = 0;

    return 1;
}

//static void ArtiReflux_meterDataAck(Device_t *dev)
static u8_t ArtiReflux_meterDataAck(Device_t *dev)
{
    AntiRefluxDeviceInfo_t *info = (AntiRefluxDeviceInfo_t*)dev->explain;
    u8_t result = 0;

    dev->lock = 0;
    list_trans(&dev->cmdList, modbusDataCheck, &result);

    if (result == 0)
    {
        u8_t flag = 0;

        info->aux->func(0, dev, &meterPower);
        list_trans(&invertDevice, invterLockStatus, &flag);
        if (flag == 0)
        {
            APP_DEBUG("Invter unlock!!\r\n");
            list_trans(&invertDevice, invterUnlock, null);
        }
    }
    return result;
}

/*******************************************************************************
  * @note   data check
  * @param  None
  * @retval None
*******************************************************************************/
u8_t modbusDataCheck(void *load, void *changeData)
{
    int ret;
    DeviceCmd_t *cmd= (DeviceCmd_t*)load;

    if (0 == cmd->state)
    {
		ret = Modbus_CmdCheck(&cmd->cmd, &cmd->ack);
		cmd->state = (u8_t)ret;
		if (ret != 0)
		{
            *((u8_t*)changeData) = 1;
        }
    }
    else
    {
        *((u8_t*)changeData) = 1;
    }
 
	return 1;
}

/*******************************************************************************
  * @note   None
  * @param  None
  * @retval None
*******************************************************************************/
static void Anti_reflux_init(void)
{
    antibuf.payload=NULL;
    meterPower.state = 0;
    list_init(&invertDevice);
}

int float2int(u32_t data)
{
#if 0  //闂傚倸鍊风粈渚€骞栭锕€纾瑰ù鐘差儜缂嶆牕顭跨捄鐑樻拱闁稿繑绮撻弻銊╂偆閸屾稑顏� 1
    s8_t S;     //缂傚倸鍊搁崐鐑芥倿閿曞倶鈧啴骞囬鍙ョ瑝闂佺鎻徊缁樼瑜版帗鐓ラ柣鏇炲€圭€氾拷
    s8_t  E;    //闂傚倸鍊风粈浣革耿鏉堛劎浠氭繝鐢靛仩椤曟粎绮婚幘鎰佸殨闁规儼濮ら弲鎼佹煥閻曞倹瀚�
    u32_t F;    //闂傚倷娴囬褏鎹㈤幇顔藉床闁归偊鍠楀畷鏌ユ煙鏉堝墽鐣辨慨瑙勭叀閺岋綁寮崒姘粯缂備胶濮村鍫曞Φ閸曨垰绫嶉柛銉戝啯鍎紓鍌欑椤р偓闁瑰嚖鎷�
    u32_t carry;
    int val;

    S = data&0x80000000 ?  -1 : 1;
    E = ((data>>23) & 0xFF) - 127;
    F = data &0x7FFFFF;
    
   
    for (val = F, carry = 1; val > 0; val /= 10 )
    {
        carry *= 10;
    }

    if (E >= 0 && E < 32)
    {
        val = (F + carry)/((carry/10)>>E);
        val = (val+5)/10;
    }
    else if (E == -1)
    {
        val = (F + carry)/(carry/5);
        val = (val+5)/10;
    }
    else
    {
        val = 0;
    }

    return val*S;
#elif  1 //闂傚倸鍊风粈渚€骞栭锕€纾瑰ù鐘差儜缂嶆牕顭跨捄鐑樻拱闁稿繑绮撻弻銊╂偆閸屾稑顏� 濠电姷鏁搁崑娑㈡偤閵娧冨灊闁挎繂顦粻顖炴煥閻曞倹瀚�

    int val;
    float f;

    r_memcpy(&f, &data, sizeof(f));

    val = f + 0.5;

    return val;
#else
    int val;
    float *pF;

    pF = (float *)&data;

    val = *pF + 0.5;

    return val;
#endif
}


/******************************************************************************/


           