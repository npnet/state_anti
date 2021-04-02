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
#include "Protocol.h"

#define REALTIME_METER_READ_COUNTER 180     //2m
#define METER_OVERTIME  360          //3M


static u8 device_addr=1;
static u8 anti_reflux_en=0;         //=1 anti reflux enables
static float anti_threshold=0;      //anti reflux threshold

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
static u16 meter_overtime=0;


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
static u8_t meter_trans(u8 *data_ptr ,u8 data_len);
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
//闂備浇銆€閸嬫捇鎮归崫鍕儓闁绘挸鍊垮娲敃閵忊槅妫為悷婊勬緲椤﹂潧鐣烽幎钘壩╅柨鏃囧Г椤斿秹姊虹涵鍜佸殐闁哥姵鐗滈懞閬嶆嚑椤掑倿鈹忛梺璺ㄥ櫐閹凤拷
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    //print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //闂備浇銆€閸嬫捇鎮归崫鍕儓闁绘挸鍊块弻锝夛綖椤撶姷銈烽梺闈涙閸熸潙鐣烽崼鏇熷€风€瑰壊鍠椾簺
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
    Device_remove(dev);
    //list_nodeDelete(&dev->cmdList, cmd);
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

    cmd->waitTime = 6000;     // 1500=1.5 sec
    cmd->state = 0;

    cmd->ack.size = 64;     //DEVICE_ACK_SIZE;
    cmd->ack.payload = memory_apply(cmd->ack.size);
    cmd->ack.lenght = 0;
    cmd->cmd.size = data_len;
    cmd->cmd.lenght = cmd->cmd.size;
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倷鐒﹁ぐ鍐矓閻㈢ǹ钃熷┑鐘叉处閸嬨劑鎮楅崷顓烆€岄柛銈囧仱閹鈽夊▍铏灴瀵剚鎷呯憴鍕Ф闂佽鍎抽崯鍨嚕閹惰姤鐓欑紒瀣閻鏌熼煬鎻掆偓婵嬪箚閸愵喖绀嬫い鎰╁灪閺嗐儵鏌ｆ惔銏犲枙闁瑰嚖鎷�
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    //dev->cfg = null;  // 闂傚倷鐒﹀妯肩矓閸洘鍋柛鈩冾焽閳绘棃鏌嶈閸撴氨绮欐径鎰垫晜闁告侗鍘搁幐鍐煟閻愬鈽夐柣顓濈窔椤㈡艾顫濈捄铏诡唶闂佹悶鍎婚梽鍕叏閵忋倖鐓熼柕濞垮劚椤忣剚銇勯弮鈧ú妯侯嚗閸曨垱鏅搁柨鐕傛嫹
    if (ModbusDevice.cfg == null) {
          dev->cfg = (ST_UARTDCB *)ModbusDevice.head->hardCfg;
        } else {
          dev->cfg = ModbusDevice.cfg;
        }
    
    
    dev->callBack = anti_ack;  // 闂佽崵濮崇粈浣规櫠娴犲鍋柛鈩冪⊕閻掑鐓崶褎鎹ｉ柣鎰躬閹鎷呴悷鏉垮Б濡炪倧瀵岄崜鐔肺涙笟鈧崺鈧い鎺戝€归崯鍝劽归敐鍥у妺闁伙綁浜跺鍫曞煛閸屾稓鍙嗗┑鐐插级閻楃姴鐣峰▎鎾村殟闁靛闄勯锟�
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂佽崵濮抽悞锕傚磿閹跺壙鍥敆閸曨偆顔愰梺鍛婄箓鐎氼噣寮妸鈺傜厵缂佸瀵ч惌妤呮煙闊彃鈧繈骞婇悙鍝勭＜婵犲﹤鍟╅崠锟�
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    Device_inset(dev);    // 闂佽绻愮换鎰涘┑鍡愨偓鎺楀礈娴ｇ懓鏆繛杈剧悼閹虫捇鍩涢弮鍫熷仩婵炴垶蓱濠€浼存倵閸偆顣茬紒鍌涘浮婵″爼宕堕妸褎娈搁梺鑽ゅС閻掞箓宕曢幎鍓垮洭顢楅崟顐ゅ摋闂佽鍨庨崘銊﹁緢Devicelist闂備焦瀵х粙鎴炵附閺傛娼╅柨鏇炲€归崑銊╂煟椤忓嫬鍔歷iceCmdSend濠电姰鍨煎▔娑氣偓姘煎櫍楠炲啴鏁撻敓锟�
    //Device_add(dev);
    
    return 1;
}

static u8_t meter_trans(u8 *data_ptr ,u8 data_len)
{
    Device_t *dev;
    DeviceCmd_t *cmd;

    APP_DEBUG("anti reflux trans\r\n");

    dev = list_nodeApply(sizeof(Device_t));
    cmd = list_nodeApply(sizeof(DeviceCmd_t));

    cmd->waitTime = 6000;     // 1500=1.5 sec
    cmd->state = 0;

    cmd->ack.size = 64;     //DEVICE_ACK_SIZE;
    cmd->ack.payload = memory_apply(cmd->ack.size);
    cmd->ack.lenght = 0;
    cmd->cmd.size = data_len;
    cmd->cmd.lenght = cmd->cmd.size;
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倷鐒﹁ぐ鍐矓閻㈢ǹ钃熷┑鐘叉处閸嬨劑鎮楅崷顓烆€岄柛銈囧仱閹鈽夊▍铏灴瀵剚鎷呯憴鍕Ф闂佽鍎抽崯鍨嚕閹惰姤鐓欑紒瀣閻鏌熼煬鎻掆偓婵嬪箚閸愵喖绀嬫い鎰╁灪閺嗐儵鏌ｆ惔銏犲枙闁瑰嚖鎷�
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = (ST_UARTDCB *)&UART_9600_N1;  
       
    dev->callBack = anti_ack;  // 闂佽崵濮崇粈浣规櫠娴犲鍋柛鈩冪⊕閻掑鐓崶褎鎹ｉ柣鎰躬閹鎷呴悷鏉垮Б濡炪倧瀵岄崜鐔肺涙笟鈧崺鈧い鎺戝€归崯鍝劽归敐鍥у妺闁伙綁浜跺鍫曞煛閸屾稓鍙嗗┑鐐插级閻楃姴鐣峰▎鎾村殟闁靛闄勯锟�
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂佽崵濮抽悞锕傚磿閹跺壙鍥敆閸曨偆顔愰梺鍛婄箓鐎氼噣寮妸鈺傜厵缂佸瀵ч惌妤呮煙闊彃鈧繈骞婇悙鍝勭＜婵犲﹤鍟╅崠锟�
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    //Device_inset(dev);    // 闂佽绻愮换鎰涘┑鍡愨偓鎺楀礈娴ｇ懓鏆繛杈剧悼閹虫捇鍩涢弮鍫熷仩婵炴垶蓱濠€浼存倵閸偆顣茬紒鍌涘浮婵″爼宕堕妸褎娈搁梺鑽ゅС閻掞箓宕曢幎鍓垮洭顢楅崟顐ゅ摋闂佽鍨庨崘銊﹁緢Devicelist闂備焦瀵х粙鎴炵附閺傛娼╅柨鏇炲€归崑銊╂煟椤忓嫬鍔歷iceCmdSend濠电姰鍨煎▔娑氣偓姘煎櫍楠炲啴鏁撻敓锟�
    Device_add(dev);
    //Device_inset_anti(dev);

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
* introduce:   闂傚倸鍊搁崐鎼佸磹妞嬪孩顐介柨鐔哄Т绾惧鏌涢弴銊モ偓鐘测槈濡粍妫冨畷鐔煎煛閳ь剙效濡や胶绡€闁靛骏绲剧涵鐐亜閹存繃顥炵紒鍌氱Ч婵¤埖寰勭€ｎ剙骞愰梻浣呵归張顒傜矙閹达富鏁嬫繝濠傛噽绾惧ジ鏌嶈閸撶喖宕洪埀顒併亜閹烘垵顏柍閿嬪灴瀵爼鎮欓弶鎴偓婊堟煕韫囨捁瀚版い顓″劵椤﹁櫕銇勯妸銉﹀殗闁诡噣绠栧畷褰掝敃閿濆拋鈧捇姊洪懡銈呮瀾濠㈢懓妫涚划锝夋倷閻戞ǚ鎷洪梺鍛婄☉閿曘儲寰勯崟顖涚厱婵☆垰婀遍惌娆戔偓瑙勬礃閸旀瑩寮幘缁樻櫢闁跨噦鎷�     
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
        device_addr=online_dev_addr_tab[addr_index];    //闁诲海鏁告晶妤呫€冮崨鏉戞槬鐎光偓閳ь剟骞夐悧鍫熷缂佸娉曞畝娑㈡⒒娓氬洤浜楅柤瀹犳硾閳绘捇骞嬮敂鐣屽姷闂佺硶鍓濋〃鍛礊娓氣偓閺屾盯鏁愰崘銊ヮ瀳濠电姭鍋撻柟缁㈠枛闁裤倝鏌嶈閹凤拷
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
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊峰ù鍥敋瑜嶉湁闁绘垼妫勯弸渚€鏌涘☉姗堟敾婵炲懐濮垫穱濠囧Χ閸涱喖娅￠梺閫炲苯澧柛鐔告綑閻ｇ兘濡搁埡浣洪獓闂佺懓顕崑鐐垛叢闂備浇顕ф鎼佸储濠婂牆纾婚柟鍓х帛閻撴洟鏌￠崶銉ュ濠⒀屽枛闇夐柣姗€娼ч崝瀣磼缂佹鈯曟繛鐓庣箻瀹曟粏顦查柛鈺佺焸濮婃椽宕妷銉︾€梺鍛婃⒐閸ㄥ灝鐣峰ú顏勭劦妞ゆ帊闄嶆禍婊堟煙閸濆嫭顥滃ù婊堢畺濮婃椽宕妷銉︾€惧┑鐐插悑閻熲晠銆佸鑸垫櫜濠㈣埖蓱閺呮繈姊洪幐搴㈩梿婵☆偄瀚粋宥夋焼瀹ュ棌鎷洪梺鍛婃尰瑜板啯绂嶅┑鍫㈢＜閻犲洦褰冮顓炩攽閳ュ磭鍩ｉ柡浣规崌閺佹捇鏁撻敓锟�
    r_memcpy(&anti_send_buf->bytes+1,&meter_rec_buf->bytes+1,meter_rec_buf->bytes);
    
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
    //fibo_taskSleep(500);
    anti_trans((u8_t *)send_buf, sizeof(modbus_wr_t));
    r_memset(antibuf.payload,0,sizeof(modbus_rd_response_t));
    APP_DEBUG("send anti reflux data\r\n");
    memory_release(send_buf);
    

}

 /*          
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掆偓杩濋梺閫炲苯澧撮柡灞剧〒閳ь剨缍嗛崑鍛暦瀹€鍕厵妞ゆ棁鍋愮粔铏光偓瑙勬处閸嬪﹤鐣烽悢纰辨晢濞达絿枪婢瑰秹姊婚崒娆掑厡缂侇噮鍨堕獮鎰板箹娴ｅ摜锛欐繝鐢靛У閼归箖鎮為崹顐犱簻闁瑰搫绉剁拹浼存煕閻旈绠婚柡灞剧洴閹晛鐣烽崶褉鎷伴柣搴ゎ潐濞叉﹢宕濆▎鎾跺祦闁哄秲鍔嶆刊鎾煟閻旂⒈鏆掗柛鎴滅矙濮婄粯鎷呯粙娆炬闂佺ǹ顑呴幊搴ｅ弲闂佸搫绋侀崑鍡樼▔瀹ュ悿褰掓偂鎼达絾鎲奸梺缁樻尰濞茬喖寮婚弴鐔风窞婵☆垵娅ｆ禒绋库攽閿涘嫯妾搁柛鐘崇墵閸╃偤骞嬮敂缁樻櫔闂佺硶鍓濋敋妞ゅ骏鎷�
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊峰ù鍥敋瑜嶉湁闁绘垼妫勯弸渚€鏌涘☉姗堟敾婵炲懐濮垫穱濠囧Χ閸涱喖娅￠梺閫炲苯澧柛鐔告綑閻ｇ兘濡搁埡浣洪獓闂佺懓顕崑鐐垛叢闂備浇顕ф鎼佸储濠婂牆纾婚柟鍓х帛閻撴洟鏌￠崶銉ュ濠⒀屽枛闇夐柣姗€娼ч崝瀣磼缂佹鈯曟繛鐓庣箻瀹曟粏顦查柛鈺佺焸濮婃椽宕妷銉︾€梺鍛婃⒐閸ㄥ灝鐣峰ú顏勭劦妞ゆ帊闄嶆禍婊堟煙閸濆嫭顥滃ù婊堢畺濮婃椽宕妷銉︾€惧┑鐐插悑閻熲晠銆佸鑸垫櫜濠㈣埖蓱閺呮繈姊洪幐搴㈩梿婵☆偄瀚粋宥夋焼瀹ュ棌鎷洪梺鍛婃尰瑜板啯绂嶅┑鍫㈢＜閻犲洦褰冮顓炩攽閳ュ磭鍩ｉ柡浣规崌閺佹捇鏁撻敓锟�
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
                        //濠电姷鏁告慨鐑藉极閸涘﹥鍙忓ù鍏兼綑閸ㄥ倿鏌ｉ幘宕囧哺闁哄鐗楃换娑㈠箣閻愯尙鍔伴梺绋款儐閹告悂锝炲┑瀣亗閹兼番鍨昏ぐ搴繆閵堝洤啸闁稿鐩畷顖烆敍濠婂嫬搴婂┑鐘绘涧椤戝懘鎮欐繝鍥ㄧ厪濠电倯鍐ㄦ殭閸熸悂姊婚崒姘偓鐑芥嚄閸撲焦鍏滈柛顐ｆ礀缁€鍫熺節闂堟侗鍎嶉柍褜鍓欓崯鏉戠暦閵娾晩鏁嶆繝濠傛媼濡茬ǹ鈹戦悙瀛樺鞍闁糕晛鍟村畷鎴﹀箻缂佹鍘搁柣蹇曞仜婢ц棄煤閹绢喗鐓涘ù锝囨嚀婵秶鈧娲橀敃銏′繆閹间礁唯妞ゆ棃鏁崑鎾诲箻椤旇В鎷哄┑顔炬嚀濞层倝鎮炲ú顏呯厱闁靛ň鏅欓幉鐐殽閻愭潙濮嶇€规洘锚椤斿繘顢欓崗纰卞悪闂傚倷绶氬褑澧濋梺鍝勬噺缁挸鐣烽悽纰樺亾閿濆骸鏋熼柣鎾存礃閵囧嫰顢橀悢椋庝淮婵炲瓨绮嶇换鍐Φ閸曨垰妫橀悹鎭掑壉瑜庨幈銊︾節閸愨斂浠㈤悗瑙勬礈閸忔﹢銆佸鈧幃銈嗘媴闁垮鐓曢梻鍌氬€烽懗鍫曞箠閹捐鍚归柡宥庡幖缁狀垶鏌ㄩ悤鍌涘
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //闂傚倸鍊峰ù鍥敋瑜嶉湁闁绘垼妫勯弸渚€鏌涘☉姗堟敾婵炲懐濮垫穱濠囧Χ閸涱喖娅￠梺閫炲苯澧柛鐔告綑閻ｇ兘濡搁埡浣洪獓闂佺懓顕崑鐐垛叢闂備浇顕ф鎼佸储濠婂牆纾婚柟鍓х帛閻撳啰鎲稿⿰鍫濈闁绘棁鍋愬畵渚€鐓崶銊︾５闁稿鎹囬弫鎰償閳╁啰浜紓鍌欒兌婵潧顫濋妸鈺佺疅缂佸绨遍崼顏堟煕韫囨梹瀚曠紓宥勭窔楠炲啯绂掔€ｎ亜绐涙繝鐢靛Т鐎氼剟鐛鈧缁樻媴閸涘﹤鏆堢紓浣筋嚙閸婂鍩€椤掍礁鍤柛鎾跺枛瀹曟椽鍩€椤掍降浜滈柟鍝勬娴滄儳顪冮妶搴″箹闁搞垺鐓￠幃楣冩倻閽樺顓洪梺鎸庢婵倕鈻嶉姀銈嗏拺閻犳亽鍔屽▍鎰版煙閸戙倖瀚�
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //闂傚倸鍊峰ù鍥敋瑜嶉湁闁绘垼妫勯弸渚€鏌涘☉姗堟敾婵炲懐濮垫穱濠囧Χ閸涱喖娅￠梺閫炲苯澧柛鐔告綑閻ｇ兘濡搁埡浣洪獓闂佺懓顕崑鐐垛叢闂備浇顕ф鎼佸储濠婂牆纾婚柟鍓х帛閻撴洟鏌熼弶鍨暢缂佽京澧楅幈銊ф喆閸曞灚鐤佸┑顔硷龚濞咃綁骞夐幘顔肩妞ゆ劑鍨硅闂傚倷绀侀幗婊堝窗鎼粹槅鐒介梽鍥儗妤ｅ啯鈷戦柛娑橈功閳藉鏌ｆ幊閸旀垵鐣烽幋锔芥櫜闁告哎鍊曠紞濠囧极閹版澘鐐婇柕濞垮劜閻ｎ剟姊绘担鍛婂暈闁哄矉缍佸畷鎰板冀椤撶倣锕傛煕閺囥劌鐏￠柡鍛矒閹綊宕堕妸銉хシ闂佸啿鍢查澶婎潖閾忚瀚氶柍銉ョ－娴犫晠鏌ｆ惔銏犲毈闁哥姵宀稿畷姘跺箳閹存梹鐎婚梺鐟邦嚟閸嬫稓澹曢鐐粹拺闂傚牊鑳嗚ぐ鎺戠？闁规儼妫勫Ч鏌ユ煛閸愩劎澧涢柍閿嬪笒闇夐柨婵嗘噺鐠愨剝绻濋埀顒佺鐎ｎ偆鍘遍梺鍝勫€藉▔鏇㈡倶闁秵鐓冪憸婊堝礈濮樿泛绀勭憸鐗堝笒閸屻劌螖閿濆懎鏆欓柣鎺戠仛閵囧嫰骞掗崱妞惧闂備浇顕х换鎴濐潩閵娾晛绐楀┑鐘插亞閸氬鏌涢妷銏℃珖闁绘帒娼″娲捶椤撶偛濡哄銈冨妼濡繂顕ｉ弻銉ョ濞达絽婀遍崣鍡涙⒑缂佹ɑ绀€闁稿﹤婀遍埀顒佺啲閹凤拷  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掆偓杩濋梺閫炲苯澧撮柡灞剧〒閳ь剨缍嗛崑鍛暦瀹€鍕厵妞ゆ棁鍋愮粔铏光偓瑙勬处閸嬪﹤鐣烽悢纰辨晢濞达絿枪婢瑰秹姊婚崒娆掑厡缂侇噮鍨堕獮鎰板箹娴ｅ摜锛欐繝鐢靛У閼归箖鎮為崹顐犱簻闁瑰搫绉剁拹浼存煕閻旈绠婚柡灞剧洴閹晛鐣烽崶褉鎷伴柣搴ゎ潐濞叉﹢宕濆▎鎾跺祦闁哄秲鍔嶆刊鎾煟閻旂⒈鏆掗柛鎴滅矙濮婄粯鎷呯粙娆炬闂佺ǹ顑呴幊搴ｅ弲闂佸搫绋侀崑鍡樼▔瀹ュ悿褰掓偂鎼达絾鎲奸梺缁樻尰濞茬喖寮婚弴鐔风窞婵☆垵娅ｆ禒绋库攽閿涘嫯妾搁柛鐘崇墵閸╃偤骞嬮敂缁樻櫔闂佺硶鍓濋敋妞ゅ骏鎷�
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //闂傚倸鍊峰ù鍥敋瑜嶉湁闁绘垼妫勯弸渚€鏌涘☉姗堟敾婵炲懐濮垫穱濠囧Χ閸涱喖娅￠梺閫炲苯澧柛鐔告綑閻ｇ兘濡搁埡浣洪獓闂佺懓顕崑鐐垛叢闂備浇顕ф鎼佸储濠婂牆纾婚柟鍓х帛閻撴洟鏌￠崶銉ュ濠⒀屽枛闇夐柣姗€娼ч崝瀣磼缂佹鈯曟繛鐓庣箻瀹曟粏顦查柛鈺佺焸濮婃椽宕妷銉︾€梺鍛婃⒐閸ㄥ灝鐣峰ú顏勭劦妞ゆ帊闄嶆禍婊堟煙閸濆嫭顥滃ù婊堢畺濮婃椽宕妷銉︾€惧┑鐐插悑閻熲晠銆佸鑸垫櫜濠㈣埖蓱閺呮繈姊洪幐搴㈩梿婵☆偄瀚粋宥夋焼瀹ュ棌鎷洪梺鍛婃尰瑜板啯绂嶅┑鍫㈢＜閻犲洦褰冮顓炩攽閳ュ磭鍩ｉ柡浣规崌閺佹捇鏁撻敓锟�
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
                        //濠电姷鏁告慨鐑藉极閸涘﹥鍙忓ù鍏兼綑閸ㄥ倿鏌ｉ幘宕囧哺闁哄鐗楃换娑㈠箣閻愯尙鍔伴梺绋款儐閹告悂锝炲┑瀣亗閹兼番鍨昏ぐ搴繆閵堝洤啸闁稿鐩畷顖烆敍濠婂嫬搴婂┑鐘绘涧椤戝懘鎮欐繝鍥ㄧ厪濠电倯鍐ㄦ殭閸熸悂姊婚崒姘偓鐑芥嚄閸撲焦鍏滈柛顐ｆ礀缁€鍫熺節闂堟侗鍎嶉柍褜鍓欓崯鏉戠暦閵娾晩鏁嶆繝濠傛媼濡茬ǹ鈹戦悙瀛樺鞍闁糕晛鍟村畷鎴﹀箻缂佹鍘搁柣蹇曞仜婢ц棄煤閹绢喗鐓涘ù锝囨嚀婵秶鈧娲橀敃銏′繆閹间礁唯妞ゆ棃鏁崑鎾诲箻椤旇В鎷哄┑顔炬嚀濞层倝鎮炲ú顏呯厱闁靛ň鏅欓幉鐐殽閻愭潙濮嶇€规洘锚椤斿繘顢欓崗纰卞悪闂傚倷绶氬褑澧濋梺鍝勬噺缁挸鐣烽悽纰樺亾閿濆骸鏋熼柣鎾存礃閵囧嫰顢橀悢椋庝淮婵炲瓨绮嶇换鍐Φ閸曨垰妫橀悹鎭掑壉瑜庨幈銊︾節閸愨斂浠㈤悗瑙勬礈閸忔﹢銆佸鈧幃銈嗘媴闁垮鐓曢梻鍌氬€烽懗鍫曞箠閹捐鍚归柡宥庡幖缁狀垶鏌ㄩ悤鍌涘
                        
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
    //anti_trans(meter_buf, sizeof(modbus_rd_t));
    meter_trans(meter_buf, sizeof(modbus_rd_t));
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
    static u16 counter=REALTIME_METER_READ_COUNTER;

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
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾瑰瀣椤愯姤鎱ㄥ鍡楀⒒闁绘帊绮欓弻銈嗘叏閹邦兘鍋撻弽顐熷亾濮橆剦鐓奸柡宀嬬秮瀵噣宕掑顑跨帛缂傚倷璁查崑鎾愁熆閼搁潧濮囩紒鐘侯潐缁绘盯鏁愭惔鈥愁潻婵犵鈧偨鍋㈤柡灞剧☉椤繈顢楅崒婧炪劌螖閻橀潧浠﹂柣妤侇殔椤洩绠涘☉妯碱槶閻熸粌绻楅幗顐ｇ節閻㈤潧校妞ゆ梹鐗犲畷瑙勫閺夋嚦褔鏌熼梻瀵割槮闁藉啰鍠栭弻锝夊籍閸屾瀚涢梺杞扮缁夊綊寮诲☉銏犵労闁告劗鍋撻悾宄扳攽閻橆偄浜炬繛鏉戝悑濞兼瑩宕橀埀顒勬⒑閻愯棄鍔滈柡瀣偢瀵劍绂掔€ｎ偆鍘介梺褰掑亰閸樼晫绱為幋锔界厽闊洢鍎抽悾鐢告煛瀹€鈧崰鎾跺垝濞嗘挸绠伴幖娣灩閺嬫垹绱撻崒娆戣窗闁哥姵顨呰灋闁告劑鍔庨弳锕€鈹戦崒婧撳綊宕￠幎鑺ョ厽闁哄啫娲﹂懙瑙勪繆閸欏濮嶆慨濠勭帛閹峰懘鎮滃Ο鐑樼暚婵＄偑鍊栧ú锕傚矗閸愩劎鏆﹂柟杈鹃檮閸嬪嫮绱掑Δ鍕┾偓鈧柛瀣崌瀹曘劑寮堕幋锝嗘啺闂備線娼ч¨鈧┑鈥虫喘瀹曪綁宕卞☉娆屾嫼缂傚倷鐒﹁摫缂佲偓閳ь剟姊虹粙鍖″伐婵犫偓闁秴鐒垫い鎺嶈兌閸熸煡鏌熼崙銈嗗0,
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾瑰瀣捣閻棗銆掑锝呬壕濡ょ姷鍋為悧鐘汇€侀弴銏℃櫆闁芥ê顦純鏇㈡⒒娴ｈ櫣銆婇柛鎾寸箞閹藉倻鈧綆鍠栭悙濠冦亜閹哄秷鍏岄柛鏂跨埣閹鐛崹顔煎闂佺懓鍟跨换姗€骞冨Δ鍜佹晢闁告洦鍏橀幏娲⒑閸涘﹦鈽夐柨鏇缁辨挸顫濋懜鐢靛幈闁瑰吋鐣崹濠氬矗閳ь剟鎮楃憴鍕闁搞劎鏁婚敐鐐测堪閸繄鍔﹀銈嗗笒鐎氼剟鎷戦悢鍏肩厪濠电偟鍋撳▍鍡涙煕鐎ｎ亝顥㈤柡灞剧〒娴狅箓宕滆濡插牆顪冮妶鍛寸崪闁瑰嚖鎷�
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
                        if (0x0212 == (online->code))           //婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柟闂寸绾惧鏌ｉ幇顒佹儓闁搞劌鍊块弻娑㈩敃閿濆棛顦ョ紓浣哄С閸楁娊寮婚悢鍏尖拻閻庡灚鐡曠粻鐐烘⒑閹肩偛鍔撮柛鎾寸懅濞戠敻鍩€椤掑嫭鈷戦悹鍥皺缁犲弶淇婇崣澶婃诞闁挎繄鍋炲鍕節鎼粹剝顔傞梻浣告啞濞诧箓宕归幍顔句笉婵炴垶鐟ｆ禍婊堟煙閹规劖纭惧ù鐘冲笧缁辨捇宕掑☉娆忕闂佸疇顫夐崹鍧楀箖濞嗘挻鍤嬮梻鍫熺⊕闁款厾绱撻崒娆愮グ鐎殿喗鎸鹃崚鎺戔枎韫囨洘娈惧銈嗙墱閸嬫稒鍎梻浣哥枃濡椼劑鎳楅崼鏇炵闁挎洍鍋撴い顏勫暣婵″爼宕卞Δ鍐噯闂備焦瀵уú蹇涘磿閹惰棄鐒垫い鎺嶈兌閸熸煡鏌熼崙銈嗗
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
    Buffer_t buf;
    u32 buf_value;

    antibuf.payload=NULL;
    meterPower.state = 0;
    list_init(&invertDevice);

    //get para 96,anti reflux enable or disable?
    SysPara_Get(96, &buf);
    //Swap_numChar(buf_value,onlineDeviceList.count);     //将16进制数年转为字符串
    //buf_value = Swap_charNum((char *) * (int *)buf.payload);   //将字符串转为16进制
    buf_value = Swap_charNum((char *)buf.payload);   //将字符串转为16进制
    APP_DEBUG("PARA 120=%ld",buf_value);
    anti_reflux_en=0;       
    if(buf_value)
        anti_reflux_en=1;   //anti reflux enable
    memory_release(buf.payload);

    //get para 97,anti reflux threshlod
    SysPara_Get(97, &buf);
    memory_release(buf.payload);

}

int float2int(u32_t data)
{
#if 0  //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁惧墽鎳撻—鍐偓锝庝簼閹癸綁鏌ｉ鐐搭棞闁靛棙甯掗～婵嬫晲閸涱剙顥氬┑掳鍊楁慨鐑藉磻閻愮儤鍋嬮柣妯荤湽閳ь兛绶氬鎾閳╁啯鐝栭梻渚€鈧偛鑻晶鏉款熆鐟欏嫭绀嬮柟顔惧厴楠炲﹥绻濋崒婊呅ㄩ梺璇″枓閺呮盯顢欒箛娑辨晩闁绘挸楠歌灇缂傚倸鍊搁崐鎼佸磹閸濄儳鐭撻柣銏犳啞閸嬪鏌￠崶锝嗗櫚闁逞屽墾婵″洭骞戦崟顖毼╅柨鏇楀亾缁剧虎鍨跺铏圭磼濡櫣浠搁柦鍐憾閺岀喖鎮滈幋鎺撳枤濠殿喖锕ㄥ▍锝囧垝濞嗘垶宕夐柕濞у苯鏁肩紓鍌氬€风欢锟犲窗濡ゅ懎绠查柛銉戝苯娈ㄦ繝鐢靛У閼圭偓鍎梻浣稿暱閹碱偊宕愰崨濠勵洸闁绘劗鍎ら埛鎴犵磼鐎ｎ偄顕滄繝鈧幍顔剧＜閻庯綆鍋勯悘瀛樸亜閵忊剝顥堥柡浣规崌閺佹捇鏁撻敓锟� 1
    s8_t S;     //缂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚敐澶婄闁挎繂鎲涢幘缁樼厱濠电姴鍊归崑銉╂煛鐏炶濮傜€殿喗鎸抽幃娆徝圭€ｎ亙澹曢梺鍓插亝濞叉牠鎮為崹顐犱簻闁瑰搫妫楁禍楣冩⒑閸濄儱鏋戦柟鍛婂▕閹即顢氶埀顒€鐣烽崼鏇ㄦ晢濠㈣泛顑嗗▍鎾绘⒒娴ｅ憡鍟為柣鐕傜秮瀹曟洟骞庨懞銉ヤ画濠电娀娼ч鍡涘煕閹烘嚚褰掓晲閸曨噮鍔呴梺琛″亾濞寸厧鐡ㄩ悡娆愮箾閼奸鍞虹紒銊ф櫕閳ь剙鐏氬姗€鏁冮敂鐐潟闁圭儤鍤﹂悢鍝勬瀳閺夊牄鍔戦弫婊堟⒑閼姐倕鏋戠紒顔煎閺呰泛螖閸愨晜娈板┑掳鍊曢幊蹇涘磻閸岀偞鐓忓璺烘濞呭棝鏌ｉ幘璺烘灈闁哄矉绻濆畷鍫曞煛娴ｉ鎹曢梻渚€鈧偛鑻晶顔界節閳ь剟鏌嗗鍛姦濡炪倖甯婇懗鍫曞疮閺屻儲鐓欓柛鎴欏€栫€氾拷
    s8_t  E;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁惧墽鎳撻—鍐偓锝庝簼閹癸綁鏌ｉ鐐搭棞闂囧鏌ㄥ┑鍡╂Ч濞存嚎鍊濋弻銈夊级閹稿骸浠撮悗娈垮枛閻栫厧鐣烽悡搴樻婵☆垯璀﹂悗宄扳攽閻愬樊鍤熷┑顔炬暩閸犲﹤顓奸崶銊ュ簥濠电娀娼ч鍡涘磻閵娾晜鈷掗柛顐ゅ枔閳笺儵鏌涘┑鍥舵當妞ゎ亜鍟存俊鍫曞幢濡⒈妲扮紓鍌欑贰閸犳牕霉閻戣棄绀嗛柟鐑樻尵缁♀偓濠殿喗锕╅崜娑㈩敊閹烘鈷戠憸鐗堝俯閺嗘帞绱掗埀顒佹媴缁洘鐏侀梺闈浥堥弲婊堝煕閹达附鍋℃繛鍡楃箰椤忊晠鏌涢弮鍥ㄧ【闁宠鍨块、娆撳传閸曘劌浜炬繝闈涙－閸ゆ洟鏌＄仦璇插姎缂佺姷鎳撻湁闁挎繂鎳忕拹锟犳煕濞嗗繑顥㈡慨濠呮缁辨帒螣閼姐値妲梻浣呵归敃銈咃耿闁秴鐒垫い鎺嶈兌閸熸煡鏌熼崙銈嗗
    u32_t F;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧湱鈧懓瀚崳纾嬨亹閹烘垹鍊炲銈嗗笒椤︿即寮查鍫熷仭婵犲﹤鍟扮粻缁橆殽閻愭潙鐏村┑顔瑰亾闂侀潧鐗嗛幊鎰版偪閳ь剚淇婇悙顏勨偓鏍涙担鑲濇盯宕熼浣稿妳婵犵數濮村ú锕傚煕閹寸姵鍠愰柣妤€鐗嗙粭鎺懨瑰⿰鈧崡鎶藉蓟濞戙垺鍋愰柟棰佺劍閻ｅ爼姊烘导娆戞偧闁稿繑锚椤曪絾绻濆顑┿劑鏌ㄩ弮鈧崕鎶界嵁瀹ュ鈷戦柛婵嗗濡叉悂鏌ｅΔ鈧Λ娆撳疾閼哥數顩烽悗锝庡亜娴犲ジ鏌℃径濠勫闁告柨绉瑰畷锝堢疀濞戞瑥浠┑鐐叉缁绘劙顢旈锔界厱婵炲棗鑻禍楣冩⒒閸屾瑧顦﹂柟纰卞亰閹本寰勫畝鈧粈濠偯归敐鍛棌闁搞倖娲橀妵鍕即濡も偓娴滈箖鎮楃憴鍕閻㈩垱甯￠敐鐐测攽鐎ｅ灚鏅為柣鐘充航閸斿瞼绮敍鍕＝闁稿本鑹鹃埀顒佹倐瀹曟劙鎮滈懞銉ユ畱闂佽偐枪閻忔岸宕ｈ箛娑欑厸濠㈣泛顑呴悞楣冩煕閵堝棙绀嬮柡灞剧洴椤㈡洟鏁愰崱娆樻К濠碘剝顨呴幊妯侯潖濞差亜宸濆┑鐘插暙椤︹晠姊洪幖鐐插濠㈢懓妫濋幃顕€骞嗚閸氬顭跨捄渚剳闁告﹢浜跺娲濞戣鲸效闂佹悶鍔庨弫璇茬暦椤栫儐鏁嶉柣鎰皺閿涚喎顪冮妶鍛婵☆偅绋撴竟鏇熺鐎ｎ偆鍘遍柣蹇曞仜婢т粙骞婇崨顔轰簻闁挎柨銈稿顕€鏌曢崶褍顏い锔诲櫍閺屾稓鈧綆鍋呭畷灞炬叏婵犲啯銇濋柟顔惧厴瀵爼骞愭惔顔兼暏闂傚倷鑳堕幊鎾诲吹閺嶎厼绠柨鐕傛嫹
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
#elif  1 //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁惧墽鎳撻—鍐偓锝庝簼閹癸綁鏌ｉ鐐搭棞闁靛棙甯掗～婵嬫晲閸涱剙顥氬┑掳鍊楁慨鐑藉磻閻愮儤鍋嬮柣妯荤湽閳ь兛绶氬鎾閳╁啯鐝栭梻渚€鈧偛鑻晶鏉款熆鐟欏嫭绀嬮柟顔惧厴楠炲﹥绻濋崒婊呅ㄩ梺璇″枓閺呮盯顢欒箛娑辨晩闁绘挸楠歌灇缂傚倸鍊搁崐鎼佸磹閸濄儳鐭撻柣銏犳啞閸嬪鏌￠崶锝嗗櫚闁逞屽墾婵″洭骞戦崟顖毼╅柨鏇楀亾缁剧虎鍨跺铏圭磼濡櫣浠搁柦鍐憾閺岀喖鎮滈幋鎺撳枤濠殿喖锕ㄥ▍锝囧垝濞嗘垶宕夐柕濞у苯鏁肩紓鍌氬€风欢锟犲窗濡ゅ懎绠查柛銉戝苯娈ㄦ繝鐢靛У閼圭偓鍎梻浣稿暱閹碱偊宕愰崨濠勵洸闁绘劗鍎ら埛鎴犵磼鐎ｎ偄顕滄繝鈧幍顔剧＜閻庯綆鍋勯悘瀛樸亜閵忊剝顥堥柡浣规崌閺佹捇鏁撻敓锟� 婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繘鏌ｉ幋锝嗩棄闁哄绶氶弻鐔兼⒒鐎靛壊妲紒鎯у⒔缁垳鎹㈠☉銏犵婵炲棗绻掓禒楣冩⒑缁嬫鍎嶉柛濠冪箞瀵寮撮悢铏诡啎閻熸粌绉瑰畷顖烆敃閿旇棄鈧泛鈹戦悩鍙夊闁抽攱鍨块弻鐔虹矙閹稿孩宕崇紓浣哄У閹稿濡甸崟顖涙櫆閻犲洩灏欐禒顖滅磽娓氬洤鏋涙い顓犲厴閵嗕礁鈽夐姀鈥斥偓鐑芥倵閻㈢櫥鐟邦嚕閹惰姤鈷掑ù锝堟鐢稒绻涢崣澶屽⒌鐎规洘鍔欏畷鐑筋敇濞戞ü澹曞┑顔筋焽閸嬫挾鈧熬鎷�

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


           