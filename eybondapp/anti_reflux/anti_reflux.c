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
#include "ieee754_float.h"

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
//闂佽　鍋撻悹鍝勬惈閻撳倿姊洪銏☆棞鐟滄澘顦靛畷鎶藉Ω閿旇姤顔嶉梺纭咁嚃閸犳牜鑺遍懠顒傞┏闁跨噦鎷�
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    //print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //闂佽　鍋撻悹鍝勬惈閻撳倿鏌ｉ～顓犵シ闁靛棗鍟村畷鍫曟倷瀹割喗些
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂備焦褰冪粔鐢稿蓟婵犲洦鍋ㄩ悗鍦閸ょ偤鎮楀☉娅虫垿寮担瑙勭秶闁规儳鍟垮鎶芥煙缁嬫寧鐭楅柟韬插€濋幆鍐礋椤愩垺鏆ラ柣搴㈠喕閹凤拷
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = null;  // 闂備焦婢樼粔鍫曟偪閸℃鈻旈柍褜鍓氱粙澶愵敂閸涱厸鎸冮柣鐐寸☉閻線顢氬璺虹闁搞儻闄勬慨銏ゆ煟閵娿儱顏い鏃€娲樺鍕晸閿燂拷
    dev->callBack = anti_ack;  // 闁荤姳绀佹晶浠嬫偪閸℃稒鐒婚煫鍥ф捣閻愬﹪鎮规担鐟板姢妞わ富鍓熷Λ渚€鍩€椤掑倹鍟哄ù锝囧劋閻ｉ亶寮堕埡鍌涚叆婵炲弶鐗犲畷娆撴嚍閵夛附顔�
   // dev->callBack = NULL;  // 闁荤姳绀佹晶浠嬫偪閸℃稒鐒婚煫鍥ф捣閻愬﹪鎮规担鐟板姢妞わ富鍓熷Λ渚€鍩€椤掑倹鍟哄ù锝囧劋閻ｉ亶寮堕埡鍌涚叆婵炲弶鐗犲畷娆撴嚍閵夛附顔�
    //dev->explain = esp;   // 闁荤姳鐒﹂崕鎶剿囬鍕闁告繂瀚弫銊╂煙缁嬫寧鐭楅柟韬插€濋幊鐐哄磼濠婂啩鍖�
    dev->type = DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    //list_topInsert(&dev->cmdList, cmd);
   // Device_inset(dev);    // 闁诲繐绻愬Λ婵嗐€掗崜浣瑰暫濞达絿鎳撻埛鏃堟偠濞戞ɑ婀伴悗鍨缁傛帡濡堕崶銊ф殸闁荤姳鐒﹂崕鎶剿囬鍕哗闁诡垎鍐ㄦ辈Devicelist闂佹寧绋戞總鏂款潩閿曞倹鍋ㄩ柣顏勫劚viceCmdSend婵犮垼娉涚€氼噣骞冮敓锟�
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
* introduce:   闂傚倸鍊搁崐椋庢閿熺姴纾婚柛鏇ㄥ€犲☉妯滄棃宕熼埡鈧Ч妤佺節閵忥絾纭炬い鎴濇缁傚秹濡舵径瀣幐闂佺ǹ鏈粙鎴︻敋濠婂懐纾奸柍褜鍓熼崺鈧い鎺戝閳锋垿寮堕悙鏉戭€滈柛蹇撹嫰椤儻顦虫い銊ユ嚇閹箖宕归锝咁€撻梺鑽ゅ枑婢瑰棛绮ｉ悙鐑樷拺闁告稑锕ユ径鍕煕濡湱鐭欑€规洘鍔欓弫鎾绘晸閿燂拷     
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
    //if(rcveBuf.payload[0]!=METER_ADDR){  
    if(antibuf.payload[0]!=METER_ADDR){    
        return;
    }

    if(online_dev_count==0)
        return;
    else{
        if(++addr_index>=online_dev_count)
            addr_index=0;
        device_addr=online_dev_addr_tab[addr_index];    //閻庣數澧楅〃鍛村春瀹€鈧幉鐗堟媴缁嬭法宀涢梻渚囧亗閼宠泛鈻撻幋锔界劵闁糕剝顨呯紞渚€鏌涢敐鍐ㄥ婵犫偓閹绢喖閿ら柍褝鎷�
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
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倷娴囬褍霉閻戣棄鏋侀柛娑橈攻濞呯姵淇婇妶鍛櫡闁逞屽墮閸熸潙鐣烽妸鈺佺骇闁瑰鍋炶ⅲ闂佽楠搁崢婊堝磻閹剧粯鐓曢柡鍥ュ妼婢ь喖霉閻橀潧鍔嬬紒缁樼⊕濞煎繘宕滆閸╁矂姊洪崫銉ユ瀭闁告梹鍨垮畷娲焵椤掍降浜滈柟鍝勬娴滈箖姊洪崫銉ユ瀾婵炲吋鐟╅、姘舵晲婢舵ɑ鏅濋梺鎸庢濡嫭绂嶉柆宥嗏拺闁告挻褰冩禍婵堢磼鐠囨彃顏┑鈥崇埣閺佹捇鏁撻敓锟�
    r_memcpy(&anti_send_buf->bytes+1,&meter_rec_buf->bytes+1,meter_rec_buf->bytes);
    r_memset(antibuf.payload,0,sizeof(modbus_rd_response_t));
    float a=IEEE754_to_Float(&anti_send_buf->bytes+1);
    float b=IEEE754_to_Float(&anti_send_buf->bytes+5);
    float c=IEEE754_to_Float(&anti_send_buf->bytes+9);
    float t=IEEE754_to_Float(&anti_send_buf->bytes+13);
    APP_DEBUG("A POWER=%f KW\r\n",a);
    APP_DEBUG("B POWER=%f KW\r\n",b);
    APP_DEBUG("C POWER=%f KW\r\n",c);
    APP_DEBUG("T POWER=%f KW\r\n",t);

    anti_send_buf->addr=device_addr;          
    anti_send_buf->fun=MODBUS_RTU_FUN_WR;
    anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
    anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
    anti_send_buf->bytes=meter_rec_buf->bytes;
    anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
    //Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
    anti_trans((u8_t *)send_buf, sizeof(modbus_wr_t));
    //meter_data_sent=0;              //闂佹椿婢€缁插濡撮崘顔兼瀬闁绘鐗嗙粊锔锯偓瑙勭摃鐏忣亪鐛箛娑樼煑闁哄秲鍔岄悘鍥煕閺冩挻瀚�
    
    APP_DEBUG("send anti reflux data\r\n");
    memory_release(send_buf);
    

}

 /*          
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐宄懊归崶顒€违闁逞屽墴閺屾稓鈧綆鍋呭畷宀勬煙椤旇偐绉虹€规洦鍋婂畷鐔碱敆娴ｇǹ澹嶉梻鍌欒兌缁垶骞愰幖浣哥９濠电姵鑹鹃悞鍨亜閹哄秶璐伴柛鐔风箻閺屾盯鎮╁畷鍥р拰閻庤娲橀崝娆撶嵁閺嶃劍濯撮柣鐔碱暒閸戜粙姊绘担绋款棌闁稿鎳庣叅闁哄稁鍋嗘稉宥吤归悡搴ｆ憼闁绘挻娲熼弻鏇熷緞濡櫣浠稿┑锛勮檸閸犳牠鍩為幋锔绘晪闁糕剝锚椤忥拷
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倷娴囬褍霉閻戣棄鏋侀柛娑橈攻濞呯姵淇婇妶鍛櫡闁逞屽墮閸熸潙鐣烽妸鈺佺骇闁瑰鍋炶ⅲ闂佽楠搁崢婊堝磻閹剧粯鐓曢柡鍥ュ妼婢ь喖霉閻橀潧鍔嬬紒缁樼⊕濞煎繘宕滆閸╁矂姊洪崫銉ユ瀭闁告梹鍨垮畷娲焵椤掍降浜滈柟鍝勬娴滈箖姊洪崫銉ユ瀾婵炲吋鐟╅、姘舵晲婢舵ɑ鏅濋梺鎸庢濡嫭绂嶉柆宥嗏拺闁告挻褰冩禍婵堢磼鐠囨彃顏┑鈥崇埣閺佹捇鏁撻敓锟�
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
                        //婵犵數濮烽弫鍛婃叏娴兼潙鍨傞柣鎾崇岸閺嬫牗绻涢幋鐐茬劰闁稿鎸搁～婵嬫偂鎼淬垻褰庢俊銈囧Х閸嬫盯宕婊勫床婵犻潧顑呴悙濠囨煏婵炑冨暙鍟搁梻鍌氬€烽懗鍓佹兜閸洖绀堟繝闈涱儍閳ь剙鍟村畷銊╊敍濠婂拋妲稿┑鐐存尰閸╁啴宕戦幘缁樼厸閻忕偛澧藉ú鎾煛娴ｇ懓濮嶇€规洘锕㈡俊鎼佸Ψ椤旈敮鍋撻幘顔解拺婵懓娲ら悞娲煕閵娾晙鎲炬鐐村姍瀹曟ê顔忛鍏碱吙闂備線娼ц墝闁哄懏绮撳畷鐢碘偓锝庡枟閻撴洘銇勯鐔风仴濞存粍绻冮妵鍕棘鐠恒剱褎鎱ㄦ繝鍐┿仢鐎规洜鍏橀、姗€鎮ゆ担閿嬪煕闂傚倷鑳堕幊鎾诲吹閺嶎厼绠柨鐕傛嫹
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //闂傚倷娴囬褍霉閻戣棄鏋侀柛娑橈攻濞呯姵淇婇妶鍛櫡闁逞屽墮閸熸潙鐣烽妸鈺佺骇闁瑰鍋炶ⅲ闂佽楠搁崢婊堝磻閹剧粯鐓冪憸婊堝礈閻旇偐宓侀煫鍥ㄦ磵閸嬫捇鏁愭惔鈩冪亪缂備胶濮靛銊╁箟缁嬪簱鍫柛蹇旀嫕缂嶄線骞冩禒瀣窛濠电姴瀚獮姗€姊绘担鍛婂暈缂佽鍊婚埀顒佸嚬閸撶喖宕洪埀顒併亜閹哄棗浜惧銈庡幖閸㈡煡鎮鹃悜钘夌闁挎棁濮ゅ▍銏ゆ⒑鐠恒劌娅愰柟鍑ゆ嫹
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //闂傚倷娴囬褍霉閻戣棄鏋侀柛娑橈攻濞呯姵淇婇妶鍛櫡闁逞屽墮閸熸潙鐣烽妸鈺佺骇闁瑰鍋炶ⅲ闂佽楠搁崢婊堝磻閹剧粯鐓曢柟鏉垮缁辩増鎱ㄧ憴鍕垫疁婵﹨娅ｉ幉鎾礋椤愩垹袘闂備礁鎽滈崰搴☆焽闄囬惌楣冩⒑閸涘﹦鈽夐柣掳鍔戝畷鎴︽晲閸氥倕缍婇弫鎰板炊閵娿儲鐣梻浣告啞閺岋綁宕愰弽顓炍﹂柛鏇ㄥ灡閺呮粓鎮归崶銊ョ祷闁冲嘲顑夊铏规嫚閳ュ磭浠╅柣搴㈠嚬閸犳岸宕氶幒鎴旀瀻闁瑰鍋涚壕顖炴⒑闂堟胆褰掑磿閹惰棄姹查柡鍐ㄧ墛閳锋帒霉閿濆懏璐℃繝鈧禒瀣厱闁哄倽娉曢悘閬嶆煃瑜滈崜姘跺礄瑜版帒鍌ㄥΔ锝呭暙閻掑灚銇勯幒鍡椾壕闂佽绻戝銊╁窗婵犲偆鍚嬮柛銉㈡櫇閻掑潡姊洪崷顓炲妺妞ゃ劌妫濆鏌ュ箹娴ｅ湱鍙嗛梺缁樻礀閸婂湱鈧熬鎷�  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐宄懊归崶顒€违闁逞屽墴閺屾稓鈧綆鍋呭畷宀勬煙椤旇偐绉虹€规洦鍋婂畷鐔碱敆娴ｇǹ澹嶉梻鍌欒兌缁垶骞愰幖浣哥９濠电姵鑹鹃悞鍨亜閹哄秶璐伴柛鐔风箻閺屾盯鎮╁畷鍥р拰閻庤娲橀崝娆撶嵁閺嶃劍濯撮柣鐔碱暒閸戜粙姊绘担绋款棌闁稿鎳庣叅闁哄稁鍋嗘稉宥吤归悡搴ｆ憼闁绘挻娲熼弻鏇熷緞濡櫣浠稿┑锛勮檸閸犳牠鍩為幋锔绘晪闁糕剝锚椤忥拷
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //闂傚倷娴囬褍霉閻戣棄鏋侀柛娑橈攻濞呯姵淇婇妶鍛櫡闁逞屽墮閸熸潙鐣烽妸鈺佺骇闁瑰鍋炶ⅲ闂佽楠搁崢婊堝磻閹剧粯鐓曢柡鍥ュ妼婢ь喖霉閻橀潧鍔嬬紒缁樼⊕濞煎繘宕滆閸╁矂姊洪崫銉ユ瀭闁告梹鍨垮畷娲焵椤掍降浜滈柟鍝勬娴滈箖姊洪崫銉ユ瀾婵炲吋鐟╅、姘舵晲婢舵ɑ鏅濋梺鎸庢濡嫭绂嶉柆宥嗏拺闁告挻褰冩禍婵堢磼鐠囨彃顏┑鈥崇埣閺佹捇鏁撻敓锟�
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
                        //婵犵數濮烽弫鍛婃叏娴兼潙鍨傞柣鎾崇岸閺嬫牗绻涢幋鐐茬劰闁稿鎸搁～婵嬫偂鎼淬垻褰庢俊銈囧Х閸嬫盯宕婊勫床婵犻潧顑呴悙濠囨煏婵炑冨暙鍟搁梻鍌氬€烽懗鍓佹兜閸洖绀堟繝闈涱儍閳ь剙鍟村畷銊╊敍濠婂拋妲稿┑鐐存尰閸╁啴宕戦幘缁樼厸閻忕偛澧藉ú鎾煛娴ｇ懓濮嶇€规洘锕㈡俊鎼佸Ψ椤旈敮鍋撻幘顔解拺婵懓娲ら悞娲煕閵娾晙鎲炬鐐村姍瀹曟ê顔忛鍏碱吙闂備線娼ц墝闁哄懏绮撳畷鐢碘偓锝庡枟閻撴洘銇勯鐔风仴濞存粍绻冮妵鍕棘鐠恒剱褎鎱ㄦ繝鍐┿仢鐎规洜鍏橀、姗€鎮ゆ担閿嬪煕闂傚倷鑳堕幊鎾诲吹閺嶎厼绠柨鐕傛嫹
                        
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
//test
/*
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
    if(counter==0){
        counter=REALTIME_METER_READ_COUNTER;
        meter_data_sent=1;
        meter_overtime=METER_OVERTIME;
        meter_read();
    }
}
*/

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
    //闂傚倸鍊搁崐鎼佸磹妞嬪孩顐芥慨姗嗗墻閻掍粙鏌ゆ慨鎰偓鏍偓姘煼閺岋綁寮崒姘粯缂備讲鍋撳鑸靛姇缁犺绻涢敐搴″濠碘€炽偢閺屾稑顫濋鍌溞ㄥΔ鐘靛仦閻楁顭囪箛娑樼鐟滃繗鎽繝鐢靛У椤旀牠宕规导鏉懳ч柟闂寸閽冪喖鏌ｉ弮鍌楁嫛闁轰礁绉归弻娑㈠焺閸愮偓鐣峰┑鐘亾濞村吋娼欓崘鈧梺鐐藉劜閺嬪ジ寮ㄦ禒瀣厽闁归偊鍘界紞鎴︽煟韫囥儳鐣甸柡宀€鍠撶划娆撳箰鎼淬垹鏋戠紓鍌欑贰閸犳螞閸愩劎鏆﹀┑鍌溓归崡鎶芥煟閺冨洦鑵规俊鍙夊姍濮婄粯鎷呴悜妯烘畬濡炪倖娲﹂崣鍐ㄧ暦閹达附鍋勭紒妤勩€€閸嬫捇宕ㄩ弶鎴ｆ憰闂侀潧顧€婵″洭宕ｉ崱娑欌拺缂備焦蓱缁€鈧梺绋匡工濠€閬嶅焵椤掍胶鍟查柟鍑ゆ嫹0,
    //闂傚倸鍊搁崐鎼佸磹妞嬪海鐭嗗〒姘ｅ亾妤犵偞鐗犻、鏇㈡晜閽樺缃曢梻浣虹《閸撴繈鎽傜€ｎ喖鐐婃い鎺嶈兌閸斿爼鎮楅獮鍨姎闁瑰啿绻橀幃妤咁敆閸曨兘鎷洪梺鍛婄☉閿曪妇绱撳鑸电厱閹兼番鍨婚崣鈧悗瑙勬礃閸ㄧ敻锝炲⿰鍫濈劦妞ゆ帒瀚拑鐔兼煏婵炵偓娅嗛柛瀣閺屾稓浠﹂崜褉妲堝銈呴獜閹凤拷
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
                        if (0x0212 == (online->code))           //濠电姷鏁告慨鐑藉极閹间礁纾婚柣鎰惈閸ㄥ倿鏌涢锝嗙缂佺姳鍗抽弻鐔兼⒒鐎垫瓕绠為梺鎼炲労閸撴瑧娑甸埀顒勬⒑鐠囪尙绠叉俊鍙夊浮閿濈偞寰勬繝搴℃闂佸憡娲﹂崹鎵不濞戞瑣浜滈柟鎹愭硾娴犳帞绱撻崒娑欏碍闁宠鍨块幃娆撴嚋闂堟稒閿紓鍌欐祰瀵挾鍒掑▎蹇曟殾妞ゆ牜鍋涙儫闂佸疇妗ㄩ懗鍫曞礉閿曗偓椤啴濡堕崱妤冪懆闂佹寧娲忛崕鎶藉焵椤掍胶鍟查柟鍑ゆ嫹
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
#if 0  //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾剧懓顪冪€ｎ亝鎹ｉ柣顓炴閵嗘帒顫濋敐鍛婵°倗濮烽崑鐐烘偋閻樻眹鈧線寮撮姀鈩冩珖闂侀€炲苯澧板瑙勬礋閹兘骞婃繝鍌滅Ш闁诡喒鏅涢蹇涱敊閻撳骸螚缂傚倸鍊搁崐鍝ョ矓閻㈠憡鍋嬮柡鍥ｆ噰閳ь剨濡囬幑鍕Ω閿曗偓绾绢垶姊虹紒妯虹仸閽冮亶鏌熼悜鎴掓喚婵﹨娅ｇ划娆戞崉閵娧屽敼缂傚倷绶￠崰妤呭箲閸パ屽殨濠电姵鑹炬儫闂佸啿鎼崐鍛婄閻愮儤鈷戠紒瀣濠€鎵磼鐎ｎ偄鐏存い銏℃閺佹捇鏁撻敓锟� 1
    s8_t S;     //缂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻锝夊箣閿濆憛鎾绘煕婵犲倹鍋ラ柡灞诲姂瀵挳鎮欏ù瀣壕闁割偅娲栭悞鍨亜閹哄棗浜鹃梺鍝ュ枑閹告娊鎮伴鈧畷鍫曨敆婢跺娅撻梻浣告啞閻燂綁宕曢幎鑺ュ仭婵犻潧顑嗛埛鎺懨归敐鍕劅闁衡偓娴煎瓨鐓欐繛鑼额唺缁ㄧ晫鈧灚婢橀敃锔炬閹烘嚦鐔哄枈鏉堛劑鏁滈梺鑽ゅ枑缁孩鏅跺Δ鍐╂殰婵°倕鎳忛崑鍌炴煏婢跺棙娅嗛柣鎾跺枛閺岋繝宕堕埡浣风捕闂侀€炲苯澧繝鈧柆宥呯劦妞ゆ帊鑳堕崯鏌ユ煙閸戙倖瀚�
    s8_t  E;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾剧懓顪冪€ｎ亝鎹ｉ柣顓炴闇夐柨婵嗩槹娴溿倝鏌ら弶鎸庡仴鐎殿喖鐖煎畷鐓庘槈濡警鐎峰┑鐐差嚟婵數鍠婂鍥ㄥ床婵犻潧顑嗛崑銊╂⒒閸喓鈼ラ柛婵囶殔椤啴濡堕崱妯碱槰缂備緡鍠栧ù鐑藉礆閹烘挾绡€婵﹩鍓涢鎺楁⒑瑜版帩鏆掔紒鈧担绯曟灁闁靛ň鏅滈埛鎴︽偡濞嗗繐顏╅柛鏃囨硾閳规垿顢欓崫鍕ㄥ亾濠靛棭鍤曢柡灞诲劚缁犵懓霉閿濆懏璐￠柛娆忔濮婅櫣绱掑Ο鑽ゎ槬闂佺ǹ锕ゅ﹢閬嶅焵椤掍胶鍟查柟鍑ゆ嫹
    u32_t F;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ帒顦伴弲顏堟偡濠婂啰绠绘鐐村灴婵偓闁靛牆鎳愰悿鈧俊鐐€栧Λ浣肝涢崟顒佸劅濠电姴娲﹂埛鎴犳喐閻楀牆绗掑ù婊€鍗抽弻娑㈡偐閹颁焦鐣堕梺浼欑悼閸忔ê顕ｆ繝姘ㄩ柨鏃€鍎抽獮宥夋⒑閸濆嫷妲搁柣妤€妫欓弲鑸电鐎ｎ偄浠奸柡澶婄墑閸斿秹宕ｈ箛娑欏仭婵炲棗绻愰顐︽煕濞嗗苯浜鹃梻鍌欑閹碱偊鎯屾径宀€绀婂ù锝呭閸ゆ洘銇勯弴妤€浜鹃悗瑙勬礃鐢帡锝炲┑瀣垫晞閻犳亽鍔岀粭锛勭磽閸屾艾鈧悂宕愰悜鑺ュ殑闁肩ǹ鐏氶崣蹇涙煛婢跺鐒鹃柛銈嗘礋閺屾盯顢曢敐鍡欘槬婵℃鎳樺娲川婵犲啫顦╅梺鎼炲妽婢瑰棝鎮幆褜鍚嬪璺侯儑閸橀亶姊洪棃娑辨Ч闁搞劎鏁诲畷顖烆敍閻愯尙锛熷銈呯箰濡稓澹曟禒瀣厱閻忕偛澧介幊鍛亜閿斿ジ妾柕鍥у椤︻噣鏌涚€ｎ偅宕屾慨濠冩そ閹兘寮堕幐搴敤闂備胶鎳撻崵鏍箯閿燂拷
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
#elif  1 //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾剧懓顪冪€ｎ亝鎹ｉ柣顓炴閵嗘帒顫濋敐鍛婵°倗濮烽崑鐐烘偋閻樻眹鈧線寮撮姀鈩冩珖闂侀€炲苯澧板瑙勬礋閹兘骞婃繝鍌滅Ш闁诡喒鏅涢蹇涱敊閻撳骸螚缂傚倸鍊搁崐鍝ョ矓閻㈠憡鍋嬮柡鍥ｆ噰閳ь剨濡囬幑鍕Ω閿曗偓绾绢垶姊虹紒妯虹仸閽冮亶鏌熼悜鎴掓喚婵﹨娅ｇ划娆戞崉閵娧屽敼缂傚倷绶￠崰妤呭箲閸パ屽殨濠电姵鑹炬儫闂佸啿鎼崐鍛婄閻愮儤鈷戠紒瀣濠€鎵磼鐎ｎ偄鐏存い銏℃閺佹捇鏁撻敓锟� 濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌熼梻瀵割槮缁惧墽绮换娑㈠箣濞嗗繒浠鹃梺绋款儍閸婃繈寮婚弴鐔虹鐟滃秹宕锔藉€跺┑鐘叉处閳锋垿鏌熺粙鎸庢崳缂佺姵鎸婚妵鍕晜鐠囪尙浠紓渚囧枛椤兘銆佸☉銏″€烽悗鐢登瑰鎶芥⒒娴ｈ櫣甯涙繛鍙夌墵瀹曟劙宕烽娑樹壕婵鍋撶€氾拷

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


           