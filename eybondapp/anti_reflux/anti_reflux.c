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
            //checkDeviceOnline();
            realtime_meter_read();
        break;
        }   
    }
    fibo_thread_delete();
}

/*******************************************************************************            
* introduce:   闁棗褰夋潻鏂挎礀閺佺増宓佹径鍕倞     
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static void anti_relex_data_process(void)
{
    static Device_t *currentDevice = NULL;
    static DeviceCmd_t *currentCmd = NULL;
    u16 reg_addr;

    //瀵版鍩岄崷銊у殠鐠佹儳顦崷鏉挎絻
    if(antibuf.payload[0]!=METER_ADDR){
        device_addr=antibuf.payload[0];        
        return;
    }

    //鐠佹儳顦崷銊у殠閹靛秴褰傞柅浣烘暩鐞涖劍鏆熼幑锟�
    //if(onlineDeviceList.count==0){
    //    return;
    //}
        
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
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //鐎电増顨呴崺宀勬偨娴ｅ啨鈧啴寮悧鍫濈ウ
    r_memcpy(&anti_send_buf->bytes+1,&meter_rec_buf->bytes+1,meter_rec_buf->bytes);

    anti_send_buf->addr=device_addr;          
    anti_send_buf->fun=MODBUS_RTU_FUN_WR;
    anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
    anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
    anti_send_buf->bytes=meter_rec_buf->bytes;
    anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
    Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
    
    APP_DEBUG("send anti reflux data\r\n");
    print_buf(send_buf,25);
    memory_release(send_buf);

}

 /*          
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闁衡偓鐠哄搫鐓傜€殿喖鍊搁悥鍫曞箰閸ワ附濮�
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //鐎电増顨呴崺宀勬偨娴ｅ啨鈧啴寮悧鍫濈ウ
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
                        //濠㈣泛瀚幃濠囨焻閸℃缍侀柛锝冨妽鐢挳寮ㄩ懜鍨闁圭櫢鎷�
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //鐎电増顨呴崺宀€鎷嬮幆褜妲甸柤鍝勫€婚崑锟�
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //鐎电増顨呴崺宀冦亹閹惧啿顤呴悹浣瑰劤椤︻剟鎯勭粙璺ㄥ畨闁汇劌瀚€垫碍绂掗敓锟�  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闁衡偓鐠哄搫鐓傜€殿喖鍊搁悥鍫曞箰閸ワ附濮�
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //鐎电増顨呴崺宀勬偨娴ｅ啨鈧啴寮悧鍫濈ウ
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
                        //濠㈣泛瀚幃濠囨焻閸℃缍侀柛锝冨妽鐢挳寮ㄩ懜鍨闁圭櫢鎷�
                        
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

    Uart_write(meter_buf, sizeof(modbus_rd_t));
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
    if(counter==0 && onlineDeviceList.count!=0){
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
    //闂侀潻璐熼崝搴ㄥ吹鎼达絾濯奸柟顖嗗本校婵炴垶鎸哥粔鍫曟偤閹寸偟顩查柨鐕傛嫹0,
    //闂佸搫鐗嗛ˇ閬嶅极閸濄儲鍋橀柨鐕傛嫹
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
                        if (0x0212 == (online->code))           //婵炴垶鎸搁ˇ鍗灻规径鎰仺濞达絽鍟ㄩ埀顒婃嫹
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
#if 0  //闂傚倷绀侀幖顐﹀磹娴犲缍栧璺烘湰閸忔粓鏌ㄩ悤鍌涘 1
    s8_t S;     //缂傚倸鍊烽悞锕傘€冮幇顔句笉闁硅揪绲绘禍褰掓煥閻曞倹瀚�
    s8_t  E;    //闂傚倷绀佸﹢杈ㄧ仚濠电偠顕滅粻鎾愁嚕閹惰姤鏅搁柨鐕傛嫹
    u32_t F;    //闂備浇顕х换鎰崲閹邦喗宕查柟杈剧畱濮规煡鏌ｉ弮鍌氬付缂佺姴寮堕妵鍕籍閸パ冩優缂備礁顧€閹凤拷
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
#elif  1 //闂傚倷绀侀幖顐﹀磹娴犲缍栧璺烘湰閸忔粓鏌ㄩ悤鍌涘 婵犵數鍋涢悺銊у垝閿濆绠柨鐕傛嫹

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


           