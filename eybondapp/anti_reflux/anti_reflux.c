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

#define REALTIME_METER_READ_COUNTER 60
#define METER_OVERTIME  120          //60S


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
static u8 meter_data_sent=0;      //=1 meter data have sent
static u8 meter_overtime=0;


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
//閺€璺哄煂闁棗褰夐崳銊︽殶閹诡喖绨茬粵锟�
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    //print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //閺€璺哄煂閻絻銆冮崶鐐差槻
        APP_DEBUG("get meter return");
#ifdef GETANTIBUF_FROMACK
        get_antibuf(cmd->ack.payload,cmd->ack.lenght); 
#endif  
    }
    else{
        //anti replay
        if(cmd->ack.payload[1]==0x10){
            modbus_wr_response_t *anti_ack_buf=(modbus_wr_response_t *)cmd->ack.payload;
            anti_ack_buf->st_addr=ENDIAN_BIG_LITTLE_16(anti_ack_buf->st_addr);
            APP_DEBUG("anti_ack_buf->st_addr=%d",anti_ack_buf->st_addr);
            if(anti_ack_buf->st_addr==ANTI_REFLUX_REG_ADDR){
                meter_data_sent=0;
                APP_DEBUG("anti replay\r\n");
            }
        }
        
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

    cmd->waitTime = 1500;     // 1500=1.5 sec
    cmd->state = 0;

    cmd->ack.size = 64;     //DEVICE_ACK_SIZE;
    cmd->ack.payload = memory_apply(cmd->ack.size);
    cmd->ack.lenght = 0;
    cmd->cmd.size = data_len;
    cmd->cmd.lenght = cmd->cmd.size;
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闁插秵鏌婇悽瀹狀嚞鐎涙ɑ鏂佹潏鎾冲弳閹稿洣鎶ら惃鍕敶鐎涳拷
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = null;  // 闁板秶鐤嗘稉鈧稉顏呭⒔鐞涘矁顕氶幐鍥︽姢閻ㄥ嫯顔曟径锟�
    dev->callBack = anti_ack;  // 鐠佸墽鐤嗛柅蹇庣炊鐠佹儳顦棁鈧憰浣烘畱鏉╂柨娲栭崙鑺ユ殶
   // dev->callBack = NULL;  // 鐠佸墽鐤嗛柅蹇庣炊鐠佹儳顦棁鈧憰浣烘畱鏉╂柨娲栭崙鑺ユ殶
    //dev->explain = esp;   // 鐠佹儳顦幖鍝勭敨閹稿洣鎶ら懞鍌滃仯
    dev->type = DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    //list_topInsert(&dev->cmdList, cmd);
   // Device_inset(dev);    // 鐏忓棝娓剁憰浣瑰⒔鐞涘本瀵氭禒銈囨畱鐠佹儳顦弨鎯у弳Devicelist閿涘奔姘﹂悽鐪僥viceCmdSend婢跺嫮鎮�
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
* introduce:   闂傚倷绶￠崑鍕涘Δ鍕╀汗濠㈣泛顑嗘禍銈夋煛閸屾稑顕滅紒鈧埀顒勬⒑鏉炴媽鍏屽褎顨呴悾鍨媴閻熸壆绐為梺鍛婃处閸樹粙宕愰敓锟�     
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
        device_addr=online_dev_addr_tab[addr_index];    //瀵版鍩岀憰浣稿絺闁胶娈戦柅鍡楀綁閸ｃ劌婀撮崸鈧�
    }
	       
    APP_DEBUG("MODBUS_DATA_GET\r\n");
    //print_buf(antibuf.payload,antibuf.lenght);         
    modbus_wr2_t *modbus_buf=(modbus_wr2_t *)antibuf.payload;
    APP_DEBUG("modbus_buf->fun=%d modbus_buf->addr=%d \r\n",modbus_buf->fun,modbus_buf->addr);

    modbus_rd_response_t *meter_rec_buf=(modbus_rd_response_t *)antibuf.payload;
    u8 *send_buf=memory_apply(25);
    r_memset(send_buf,'\0',25);
    modbus_wr_t *anti_send_buf=(modbus_wr_t *)send_buf;
    APP_DEBUG("meter_rec_buf->fun=%d meter_rec_buf->addr=%d meter_rec_buf->bytes=%d \r\n",meter_rec_buf->fun,meter_rec_buf->addr,meter_rec_buf->bytes);

    //send_buf =(u8 *)anti_send_buf+7;
    //antibuf.payload =(u8 *)meter_rec_buf+3;
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂佽娴烽弫鍛婃櫠濡ゅ懌鈧啴宕ㄩ弶鎴炴К閻庡厜鍋撻柛鏇ㄥ墮娴犵儤绻涙潏鍓у埌闁哥喆鍔戦崺鈧い鎺嗗亾闁哥喐娼欓锝夘敃閿旇棄浜遍梺鍓插亝缁诲嫰濡堕敓锟�
    r_memcpy(&anti_send_buf->bytes+1,&meter_rec_buf->bytes+1,meter_rec_buf->bytes);

    anti_send_buf->addr=device_addr;          
    anti_send_buf->fun=MODBUS_RTU_FUN_WR;
    anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
    anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
    anti_send_buf->bytes=meter_rec_buf->bytes;
    anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
    //Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
    anti_trans((u8_t *)send_buf, sizeof(modbus_wr_t));
    //meter_data_sent=0;              //閻絻銆冮弫鐗堝祦瀹歌尪骞忛崣鏍ㄥ灇閸旓拷
    
    APP_DEBUG("send anti reflux data\r\n");
    memory_release(send_buf);

}

 /*          
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倷娴囬妴鈧柛瀣崌閹綊宕崟顒佸創闂佺粯鎸搁崐濠氬焵椤掍緡鍟忛柛鐘崇墵瀹曟劙骞栨担鐟颁函闂佸壊鍋呭ú鏍不娴煎瓨鐓曢柕澶樺灦濡绢喖鈹戦鍡樺
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂佽娴烽弫鍛婃櫠濡ゅ懌鈧啴宕ㄩ弶鎴炴К閻庡厜鍋撻柛鏇ㄥ墮娴犵儤绻涙潏鍓у埌闁哥喆鍔戦崺鈧い鎺嗗亾闁哥喐娼欓锝夘敃閿旇棄浜遍梺鍓插亝缁诲嫰濡堕敓锟�
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
                        //婵犵數濮伴崹鐓庘枖濞戞埃鍋撳鐓庢珝妤犵偛鍟换婵嬪炊閵娧冨Х闂備胶纭堕崜婵嬨€冮崨顖滅濞撴埃鍋撻柡灞剧洴閺佹劙宕橀妸銉€撮梺姹囧焺閸ㄩ亶骞愰崘宸殨闁靛ň鏅滈崵瀣煕椤垵浜濇い鏂跨Ч濮婃椽宕烽鐕佷户闂佺懓鍤栭幏锟�
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //闂佽娴烽弫鍛婃櫠濡ゅ懌鈧啴宕ㄩ弶鎴炴К閻庡厜鍋撻柍褜鍓熼獮蹇曗偓锝庡枛缁犳氨鎲稿⿰鍏撅綁鎮介崨濠勫幘闂佸憡绻傜€氼剟鍩€椤掆偓椤嘲鐣烽幋锔芥櫢闁跨噦鎷�
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //闂佽娴烽弫鍛婃櫠濡ゅ懌鈧啴宕ㄩ弶鎴炴К閻庡厜鍋撻柛鎰剁細濮规姊虹憴鍕姢闁哥喎娼¤矾闁告稑鐡ㄩ崑锝吤归敐鍥ㄥ殌闁告柣鍊栭妵鍕晜鐠囨彃鈷嬮梺璇″灠鐎氼喚鍒掑▎鎴炲磯闁靛ǹ鍎抽弳鏃堟⒒娴ｅ湱婀介柛鏂跨灱閳ь剚鍑归崢楣冨焵椤掆偓閻忔氨鍠婂鍥╃焿闁圭儤顨嗛弲鎼佹煥閻曞倹瀚�  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倷娴囬妴鈧柛瀣崌閹綊宕崟顒佸創闂佺粯鎸搁崐濠氬焵椤掍緡鍟忛柛鐘崇墵瀹曟劙骞栨担鐟颁函闂佸壊鍋呭ú鏍不娴煎瓨鐓曢柕澶樺灦濡绢喖鈹戦鍡樺
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //闂佽娴烽弫鍛婃櫠濡ゅ懌鈧啴宕ㄩ弶鎴炴К閻庡厜鍋撻柛鏇ㄥ墮娴犵儤绻涙潏鍓у埌闁哥喆鍔戦崺鈧い鎺嗗亾闁哥喐娼欓锝夘敃閿旇棄浜遍梺鍓插亝缁诲嫰濡堕敓锟�
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
                        //婵犵數濮伴崹鐓庘枖濞戞埃鍋撳鐓庢珝妤犵偛鍟换婵嬪炊閵娧冨Х闂備胶纭堕崜婵嬨€冮崨顖滅濞撴埃鍋撻柡灞剧洴閺佹劙宕橀妸銉€撮梺姹囧焺閸ㄩ亶骞愰崘宸殨闁靛ň鏅滈崵瀣煕椤垵浜濇い鏂跨Ч濮婃椽宕烽鐕佷户闂佺懓鍤栭幏锟�
                        
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
    if(meter_overtime)
        meter_overtime--;
    
    if(meter_overtime==0){
        meter_data_sent=0;
    }
    if(counter==0 && online_dev_count!=0 && meter_data_sent==0){
        counter=REALTIME_METER_READ_COUNTER;
        meter_data_sent=1;
        meter_overtime=METER_OVERTIME;
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
    //闂傚倸鍊风欢姘焽閼姐倖瀚婚柣鏃傚帶缁€澶愬箹濞ｎ剙濡奸柛姘秺楠炴牗娼忛崜褏蓱婵犳鍨伴妶鎼佸蓟閻旂⒈鏁嶉柛鈩冾殕濠€浼村冀閳ユ枼鏀介柣鎰綑閻忥箓鏌熺粙鎸庢喐缂侇喗妫冨畷濂稿即閻旇渹妲愰梻浣烘嚀椤曨參宕戦悢绗衡偓鍛村蓟閵夛妇鍙嗛梺缁樻礀閸婂湱鈧熬鎷�0,
    //闂傚倸鍊风粈渚€骞栭锔藉亱闁糕剝铔嬮崶顒佺劶鐎广儱鎳忛悗顒勬⒑閸涘﹦缂氶柛搴㈢叀瀹曟垵顫滈埀顒勫蓟閵娾晜鍋嗛柛灞剧☉椤忥拷
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
                        if (0x0212 == (online->code))           //濠电姷鏁搁崑鐐哄垂閸洖绠伴柟闂寸蹈閸ヮ剙纭€闁诲繑妲掗～澶婎嚗閸曨垰绠涙い鎾跺仒缂傛挸鈹戦悙鑸靛涧缂佽弓绮欏畷顖炲Ω閳轰胶鍔﹀銈嗗笒閿曘儳鈧熬鎷�
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
#if 0  //闂傚倸鍊搁崐鎼佸磹妞嬪海鐭嗗〒姘ｅ亾妤犵偞鐗犻、鏇㈡晝閳ь剛澹曢悷鎳婂綊鎮╁顔煎妧缂傚倸绉甸悧鏇⑩€︾捄銊﹀磯闁绘垶蓱閹烽亶姊虹粙璺ㄧ缂侇喗鎹囧濠氬Ω閳哄倸浜為梺绋挎湰缁嬫垿顢旈敓锟� 1
    s8_t S;     //缂傚倸鍊搁崐鎼佸磹閹间礁纾归柣鎴ｅГ閸婂潡鏌ㄩ弴鐐测偓鍫曞焵椤掆偓閸熸挳鐛崶顒夋晩闁告瑣鍎抽悷婵嬫⒒娴ｄ警鏀伴柟娲讳簽瀵板﹦绱掑Ο纰辨锤閻熸粎澧楃敮妤呮偂閵夆晜鐓熼柡鍥╁仜閳ь剙婀遍埀顒佺啲閹凤拷
    s8_t  E;    //闂傚倸鍊搁崐鎼佸磹妞嬪海鐭嗗ù锝夋交閼板潡寮堕崼娑樺濞寸姵纰嶇换婵嬫偨闂堟稐鍝楀銈嗘礃缁海鍒掓繝姘闁归绀佸▓銊╂⒑鐟欏嫬鍔跺┑顔哄€濆鏌ュ箹娴ｅ湱鍙嗛梺缁樻礀閸婂湱鈧熬鎷�
    u32_t F;    //闂傚倸鍊搁崐宄懊归崶顒夋晪鐟滃繘骞戦姀銈呯疀妞ゆ棁妫勬惔濠囨⒑瑜版帒浜伴柛鐘愁殔閻ｇ兘寮婚妷锔惧幍闁哄鐗嗘晶浠嬫偩鏉堛劍鍙忛悷娆忓閸欌偓闂佸搫鐭夌紞浣割嚕椤曗偓瀹曟帒顫濋璺ㄥ笡缂傚倸鍊烽懗鑸垫叏閺夋埈鍤曢柛顐ｆ礀妗呴梺鍛婃处閸ㄦ壆鐚惧澶嬬厱闁靛鍨甸崯顖炲磿椤忓棛纾介柛灞剧懅椤︼附銇勮閸嬫捇姊洪悷鏉挎闁瑰嚖鎷�
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
#elif  1 //闂傚倸鍊搁崐鎼佸磹妞嬪海鐭嗗〒姘ｅ亾妤犵偞鐗犻、鏇㈡晝閳ь剛澹曢悷鎳婂綊鎮╁顔煎妧缂傚倸绉甸悧鏇⑩€︾捄銊﹀磯闁绘垶蓱閹烽亶姊虹粙璺ㄧ缂侇喗鎹囧濠氬Ω閳哄倸浜為梺绋挎湰缁嬫垿顢旈敓锟� 濠电姷鏁告慨鐑藉极閹间礁纾绘繛鎴欏灪閸嬨倝鏌曟繛褍鍟悘濠囨⒑閹稿海绠撴い锔诲灣缁顢涢悙瀵稿弳闂佺粯娲栭崐鍦偓姘炬嫹

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


           