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
#include "char2negative.h"

#define REALTIME_METER_READ_COUNTER 90     //1.5m
#define METER_OVERTIME  180          //3M


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
static u8 online_dev_count=0xff;
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
            //para 96=1,enable anti reflux
            if(anti_reflux_en)
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
//闂傚倸鍊峰ù鍥Υ閳ь剟鏌涚€ｎ偅宕岄柟顔肩秺瀹曨偊宕熼浣稿壍闂備胶绮幐鎼佸磹閸噮娼栨繛宸憾閺佸啴鏌曡箛濠冾潑婵☆偆鍋ら幃宄扳堪閸曨剛鍑″銈忕畵濞佳囨偩閻戣棄绠柦妯猴級閳哄懏鐓ラ柡鍐ㄦ搐琚氬銈嗘煥缁夌懓顫忛搹瑙勫厹闁告粈绀佸▓鎰版⒑閸濄儱校闁绘绮撻幊鐐烘焼瀹ュ棗娈ゅ銈嗗笒閸婂潡鍩炶箛娑欌拺閻犳亽鍔屽▍鎰版煙閸戙倖瀚�
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    //print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //闂傚倸鍊峰ù鍥Υ閳ь剟鏌涚€ｎ偅宕岄柟顔肩秺瀹曨偊宕熼浣稿壍闂備胶绮幐鎼佸磹閸ф钃熼柨婵嗩檧缂嶆牗銇勯幘璺盒ラ柕鍫㈠厴濮婃椽妫冨☉娆樻闂佸摜鍠愬娆撴偩閻戣棄閱囬柡鍥╁枎閳ь剟顥撻埀顒傛嚀婢瑰﹪宕板鍓ф槀
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倸鍊搁崐鐑芥倿閿曚降浜归柛鎰典簽閻捇鏌ｉ姀鈶跺綊鎷戦悢灏佹斀闁绘ê寮舵径鍕煕鐎ｃ劌濮傞柟顔筋殜瀹曠兘顢橀悜鍡忓亾瀹€鍕厱闁靛牆娲ゆ禒閬嶆煙椤旂瓔娈旈柍钘夘槸閳诲秹鎼归銏紝閻庢鍣崜姘跺箯閸涱垱鍠嗛柛鏇ㄥ幘鑲栭梻鍌欐祰椤宕曢幎钘夌柈闁割煈鍣崵鏇㈡煙閹澘袚闁绘挻鐟х槐鎺斺偓锝庡亜椤曟粓鏌ｉ鐕佹疁闁哄瞼鍠栭悡顒勫箵閹哄棗浜炬繝闈涱儏缁犳岸鏌涢幇闈涙灈缂佲偓鐎ｎ偁浜滈柟鎵虫櫅閻忣亪鏌￠崱鎰姦闁哄矉绲鹃幆鏃堝閻樺弶鐏嗛梻浣烘嚀閸ゆ牠骞忛敓锟�
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    //dev->cfg = null;  // 闂傚倸鍊搁崐鐑芥倿閿曗偓椤灝螣閼测晝鐓嬮梺鍓插亝濞叉﹢宕戦鍫熺厱闁斥晛鍠氶悞浠嬫煃缂佹ɑ顥堥柡灞界Х椤т線鏌涢幘瀛樼殤缂侇喗鐟﹀鍕箛閸偅娅嗛梻浣告啞娓氭宕㈤幖浣哥闁告劦鍠楅悡鐔兼煟閹邦剦鍤熼柍钘夘樀閺岋綁顢樺┑鍫㈢崲濡炪們鍨洪懝楣冿綖濠靛牊宕夐柧蹇氼嚃閸炲爼姊绘担瑙勫仩闁稿骸顭峰浠嬪礋椤掍礁寮块梺闈涚箞閸婃牠鎮￠悢鍏肩厪濠电偛鐏濋崝姘亜韫囷絽澧柕鍥у瀵噣鍩€椤掆偓鐓ゆ俊顖欒閸ゆ鏌涢弴銊ョ仭闁哄懏鎮傞弻銊╂偆閸屾稑顏�
    if (ModbusDevice.cfg == null) {
          dev->cfg = (ST_UARTDCB *)ModbusDevice.head->hardCfg;
        } else {
          dev->cfg = ModbusDevice.cfg;
        }
    
    
    dev->callBack = anti_ack;  // 闂傚倷娴囧畷鍨叏瀹曞洨鐭嗗ù锝堫潐濞呯姴霉閻樺樊鍎愰柛瀣典邯閺屾盯鍩勯崘顏佸闂佺粯甯掗…鐑芥偂椤愶箑鐐婄憸搴ㄥ箲閿濆鐓熼柟閭﹀墾闊剟鏌熼娆炬綈闁瑰嘲鎳橀幃鐑藉级閸啩鎴炰繆閻愵亜鈧呪偓闈涚焸瀹曟粓鎮㈤懖鐑樼⊕缁楃喖鍩€椤掑嫬鏄ラ柍褜鍓氶妵鍕箳閹存績鍋撹ぐ鎺戠柈闁告繂濞婅ぐ鎺撴櫜闁搞儜鍐瀱闂備椒绱紞浣圭鐠轰警鍤曢柛顐ｆ礀閻撴盯鏌涚仦鍓с€掗柛娆忔閳规垿鎮欓幓鎺旈獓闂佺粯顨堟慨鎾偩瀹勬壋鏋庨柟鐐綑濞堢喖姊洪棃娑辨闂傚嫬瀚伴、娆撴晸閿燂拷
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂傚倷娴囧畷鍨叏閹惰姤鍊块柨鏇炲€哥壕鍧楁煙鐠哄搫顥為柛銉墻閺佸棝鏌涢弴銊ヤ簽妞ゆ梹鍔欏娲川婵犲嫮鐣甸柣搴㈠嚬閸ｏ絽顕ｉ锕€惟闁冲搫鍊婚崢鐢电磽娴ｅ壊鍎愰悗绗涘洦鍎楁俊銈呮噺閻撴瑩妫呴浣哥祷闁逞屽墯缁诲牓鐛繝鍥ㄥ€烽柛婵嗗閿涙粌鈹戦悩璇у伐闁哥啿鏅犲畷鐘绘晸閿燂拷
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    Device_inset(dev);    // 闂傚倷娴囬褏鎹㈤幇顔藉床闁归偊鍎靛☉妯锋斀闁糕剝鍔忛崑鎾诲箳濡も偓缁€鍫澝归敐鍥ㄥ殌闁哄棭鍋呯换娑欐綇閸撗勫仹闂佺娅曢幑鍥春濞戙垹绫嶉柛顐ゅ枎娴犫晛鈹戦悙鏉戠仸閽冭鲸绻濋埀顒佸鐎涙ê鈧敻鏌涢…鎴濅簽妞わ綀灏欑槐鎺楀磼濞戞ɑ璇炴繝纰樷偓宕囧煟鐎规洖鐖兼俊姝岊槼婵炲牊鎮傚娲嚒閵堝憛锟犳煟閹虹偟鐣电€规洘娲熼獮搴ㄥ礈閸喗鍠樻い銏☆殜瀹曠喖顢曢妶鍛啟闂傚倷娴囬鏍垂鎼淬劌绀冮柕濠忕畳缁额敧evicelist闂傚倸鍊烽悞锔锯偓绗涘懐鐭欓柟瀵稿仧闂勫嫰鏌￠崒娑卞劌婵炲皷鏅犻弻銊╁即閻愭祴鍋撹ぐ鎺戠；闁靛ň鏅滈悡鐔搞亜韫囨挸顏╅柛鏃€顒癷ceCmdSend濠电姷鏁告慨浼村垂閻撳簶鏋栨繛鎴炲焹閸嬫挸顫濋悡搴㈢彎濡ょ姷鍋涢崯鎾极閹剧粯鏅搁柨鐕傛嫹
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倸鍊搁崐鐑芥倿閿曚降浜归柛鎰典簽閻捇鏌ｉ姀鈶跺綊鎷戦悢灏佹斀闁绘ê寮舵径鍕煕鐎ｃ劌濮傞柟顔筋殜瀹曠兘顢橀悜鍡忓亾瀹€鍕厱闁靛牆娲ゆ禒閬嶆煙椤旂瓔娈旈柍钘夘槸閳诲秹鎼归銏紝閻庢鍣崜姘跺箯閸涱垱鍠嗛柛鏇ㄥ幘鑲栭梻鍌欐祰椤宕曢幎钘夌柈闁割煈鍣崵鏇㈡煙閹澘袚闁绘挻鐟х槐鎺斺偓锝庡亜椤曟粓鏌ｉ鐕佹疁闁哄瞼鍠栭悡顒勫箵閹哄棗浜炬繝闈涱儏缁犳岸鏌涢幇闈涙灈缂佲偓鐎ｎ偁浜滈柟鎵虫櫅閻忣亪鏌￠崱鎰姦闁哄矉绲鹃幆鏃堝閻樺弶鐏嗛梻浣烘嚀閸ゆ牠骞忛敓锟�
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = (ST_UARTDCB *)&UART_9600_N1;  
       
    dev->callBack = anti_ack;  // 闂傚倷娴囧畷鍨叏瀹曞洨鐭嗗ù锝堫潐濞呯姴霉閻樺樊鍎愰柛瀣典邯閺屾盯鍩勯崘顏佸闂佺粯甯掗…鐑芥偂椤愶箑鐐婄憸搴ㄥ箲閿濆鐓熼柟閭﹀墾闊剟鏌熼娆炬綈闁瑰嘲鎳橀幃鐑藉级閸啩鎴炰繆閻愵亜鈧呪偓闈涚焸瀹曟粓鎮㈤懖鐑樼⊕缁楃喖鍩€椤掑嫬鏄ラ柍褜鍓氶妵鍕箳閹存績鍋撹ぐ鎺戠柈闁告繂濞婅ぐ鎺撴櫜闁搞儜鍐瀱闂備椒绱紞浣圭鐠轰警鍤曢柛顐ｆ礀閻撴盯鏌涚仦鍓с€掗柛娆忔閳规垿鎮欓幓鎺旈獓闂佺粯顨堟慨鎾偩瀹勬壋鏋庨柟鐐綑濞堢喖姊洪棃娑辨闂傚嫬瀚伴、娆撴晸閿燂拷
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂傚倷娴囧畷鍨叏閹惰姤鍊块柨鏇炲€哥壕鍧楁煙鐠哄搫顥為柛銉墻閺佸棝鏌涢弴銊ヤ簽妞ゆ梹鍔欏娲川婵犲嫮鐣甸柣搴㈠嚬閸ｏ絽顕ｉ锕€惟闁冲搫鍊婚崢鐢电磽娴ｅ壊鍎愰悗绗涘洦鍎楁俊銈呮噺閻撴瑩妫呴浣哥祷闁逞屽墯缁诲牓鐛繝鍥ㄥ€烽柛婵嗗閿涙粌鈹戦悩璇у伐闁哥啿鏅犲畷鐘绘晸閿燂拷
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    //Device_inset(dev);    // 闂傚倷娴囬褏鎹㈤幇顔藉床闁归偊鍎靛☉妯锋斀闁糕剝鍔忛崑鎾诲箳濡も偓缁€鍫澝归敐鍥ㄥ殌闁哄棭鍋呯换娑欐綇閸撗勫仹闂佺娅曢幑鍥春濞戙垹绫嶉柛顐ゅ枎娴犫晛鈹戦悙鏉戠仸閽冭鲸绻濋埀顒佸鐎涙ê鈧敻鏌涢…鎴濅簽妞わ綀灏欑槐鎺楀磼濞戞ɑ璇炴繝纰樷偓宕囧煟鐎规洖鐖兼俊姝岊槼婵炲牊鎮傚娲嚒閵堝憛锟犳煟閹虹偟鐣电€规洘娲熼獮搴ㄥ礈閸喗鍠樻い銏☆殜瀹曠喖顢曢妶鍛啟闂傚倷娴囬鏍垂鎼淬劌绀冮柕濠忕畳缁额敧evicelist闂傚倸鍊烽悞锔锯偓绗涘懐鐭欓柟瀵稿仧闂勫嫰鏌￠崒娑卞劌婵炲皷鏅犻弻銊╁即閻愭祴鍋撹ぐ鎺戠；闁靛ň鏅滈悡鐔搞亜韫囨挸顏╅柛鏃€顒癷ceCmdSend濠电姷鏁告慨浼村垂閻撳簶鏋栨繛鎴炲焹閸嬫挸顫濋悡搴㈢彎濡ょ姷鍋涢崯鎾极閹剧粯鏅搁柨鐕傛嫹
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
* introduce:   闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾剧懓顪冪€ｎ亜顒㈡い鎰矙閺屻劑鎮㈤崫鍕戙垻鐥幆褜鐓奸柡灞剧洴瀵挳濡搁妷銏犱壕闁绘ɑ绁村Σ鍫熶繆椤栨稓鐭嬫俊顐㈠暙閻ｇ兘鎮㈤悡搴ｅ幐闂侀€炲苯澧弫鍫熶繆閵堝嫯鍏岀紒鈾€鍋撻梻渚€娼ф蹇曞緤閸撗勫厹闁绘劦鍏欐禍婊堟煙鐎涙绠栨い銉у仧缁辨帡宕掑姣櫻冣攽闄囬崺鏍ь嚗閸曨厸鍋撻敐搴″妤犵偞鍔欏缁樻媴閸涢潧缍婂鐢割敆閸屾粎鐓撻梺纭呮彧鐎靛矂寮€ｎ偆绡€濠电姴鍊归崳鐣岀棯閹佸仮闁哄苯绉烽¨渚€鏌涢幘璺烘灈鐎规洘妞介崺鈧い鎺嶉檷娴滄粓鏌熼悜妯虹仴妞ゅ浚浜弻宥夋煥鐎ｎ亞浼岄悗娈垮枛閻栧ジ骞冨▎鎾崇骇闁规惌鍘搁崑鎾斥堪閸喓鍘遍棅顐㈡处閹逛胶鈧氨澧楅妵鍕敇閳ュ啿濮峰銈忕畳濞呮洟濡甸崟顖毼╅柕澶涚畱濞堟姊虹拠鈥虫珝缂佺姵鐗曢悾鐤亹閹烘繃鏅濋梺鎸庣箓閹峰鍩€椤掍焦宕屾慨濠冩そ閹筹繝濡堕崨顔锯偓鐐節閵忋垺鍤€婵☆偅绋撻崚鎺楁晲婢跺鈧兘鏌ｉ幋鐐冩岸骞忓ú顏呪拺闁告稑锕﹂埥澶愭煥閺囨ê鍔滅€垫澘瀚板畷鐔碱敍濞戞艾骞堟繝纰樻閸ㄦ澘锕㈤柆宥嗗剹婵炲棙鍨熼崑鎾舵喆閸曨剛顦ㄩ梺鍛婃⒐閻熲晛顕ｉ锕€绠荤紓浣股戝▍銏ゆ⒑鐠恒劌娅愰柟鍑ゆ嫹     
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
        device_addr=online_dev_addr_tab[addr_index];    //闂備浇顕уù鐑藉极閸涘﹥娅犳俊銈呮噷閳ь剙鍟村畷銊╁级閹寸偞袣闁诲骸鍘滈崑鎾绘煃瑜滈崜鐔肩嵁婢舵劖鍋嬮柛顐ゅ枎椤庢挾绱撴担鍓插創婵炲娲栭悾婵嗏槈閵忊檧鎷绘繛鎾磋壘濞层倖绂嶅Δ鍛厾閻庡湱濮电涵楣冩煃缂佹ɑ宕屾鐐差儔閺佸倿鎮剧仦钘壭ラ梻鍌欒兌绾爼宕滃┑瀣ㄢ偓鍐川椤栨粎顦繛鎾村焹閸嬫捇鏌＄仦鍓ф创闁轰焦鍔欏畷姗€濡搁妷顔锯偓铏節閻㈤潧袥闁稿鎹囬弻鐔虹磼閵忕姵鐏堥梻浣斤骏閸婃繈寮诲鍫闂佺懓鍤栭幏锟�
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
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掑鏅悷婊冪Ч濠€渚€姊虹紒妯虹伇婵☆偄瀚板鍛婄瑹閳ь剟寮诲☉妯锋婵鐗婇弫鎯р攽閻愬弶鍣藉┑顔肩仛缁岃鲸绻濋崶顬囨煕濞戝崬鏋涙繛鍜冪節濮婃椽鏌呴悙鑼跺濠⒀屽櫍閺屾盯鎮㈤崨濠勭▏闂佷紮绲块崗妯讳繆閹间礁鐓涘ù锝嗘そ閻涙捇姊绘担鐑樺殌妞ゆ洦鍙冨畷鎴︽倷閸ㄦ稑褰氶梻鍌氬€峰ù鍥敋瑜庨〃銉╁箹娴ｇǹ鍋嶅┑鐘诧工閻楀棛澹曟繝姘厵闁告挆鍛闂佺粯鎸诲ú鐔煎蓟閿熺姴鐐婇柕澶堝劚椤牊绻濋埛鈧仦鑺ョ亪闂傚洤顦甸弻锝咁潨閳ь剙顭囪瀹曟繄鈧綆鍓涚壕鑲╃磽娴ｈ鐒介柍顖涙礃缁绘盯鎮℃惔锝囶啋閻庤娲樼划蹇浰囬弻銉︾厱闁宠桨鑳堕悞鍛婃叏婵犲啯銇濈€规洦鍋婃俊鐑藉Ψ閿旈敮鍋撻銏♀拺闁告稑锕ラ埛鎰版煕閵娿儳浠㈤柣锝呭槻鐓ゆい蹇撳閸旓箑顪冮妶鍡楃瑐闂傚嫬绉电粋宥呪堪閸喓鍘甸梺鍛婄箓鐎氼參藟濠婂厾鐟扳堪閸垻鏆┑顔硷攻濡炶棄鐣烽锕€唯闁靛濡囬埀顒佸劤閳规垿鎮欓幓鎺撳€梺鑽ゅ暱閺呯娀濡存担鍓叉僵闁肩ǹ鐏氬▍婊勭節閵忥絽鐓愰拑閬嶆煛閸涱喚绠炴慨濠冩そ楠炴劖鎯旈姀鈺傗挅婵犵妲呴崑鍕偓姘煎幘缁顓兼径瀣姸閻庡箍鍎卞Λ宀勫箯濞差亝鈷戦柛娑橈攻鐏忔壆鎲搁弶鍨殲缂佸倸绉撮埞鎴﹀醇閵忣澁绱查梺鑽ゅТ濞诧箒銇愰崘顕呮晜妞ゆ挾鍋愰弨浠嬫煃閵夈儳锛嶉柛鈺嬬秮閺屸剝鎷呯憴鍕３闂佽桨鐒﹂幑鍥极閹剧粯鏅搁柨鐕傛嫹
    r_memcpy(&anti_send_buf->bytes+1,&meter_rec_buf->bytes+1,meter_rec_buf->bytes);
    
    float a=IEEE754_to_Float(&anti_send_buf->bytes+1);
    float b=IEEE754_to_Float(&anti_send_buf->bytes+5);
    float c=IEEE754_to_Float(&anti_send_buf->bytes+9);
    float t=IEEE754_to_Float(&anti_send_buf->bytes+13);
    APP_DEBUG("A POWER=%f KW\r\n",a);
    APP_DEBUG("B POWER=%f KW\r\n",b);
    APP_DEBUG("C POWER=%f KW\r\n",c);
    APP_DEBUG("T POWER=%f KW\r\n",t);

    //any one >=anti_threshold
    if(a>anti_threshold && b>anti_threshold && c>anti_threshold && t>anti_threshold){
        meter_data_sent=0;
        APP_DEBUG("no over anti reflux threshold\r\n")
        return;
    }

    anti_send_buf->addr=device_addr;          
    anti_send_buf->fun=MODBUS_RTU_FUN_WR;
    anti_send_buf->st_addr=ENDIAN_BIG_LITTLE_16(ANTI_REFLUX_REG_ADDR);
    anti_send_buf->num= ENDIAN_BIG_LITTLE_16(8);
    anti_send_buf->bytes=meter_rec_buf->bytes;
    anti_send_buf->crc16=crc16_standard(CRC_RTU,(u8_t *)anti_send_buf,sizeof(modbus_wr_t)-sizeof(anti_send_buf->crc16));
    //Uart_write((u8_t *)send_buf, sizeof(modbus_wr_t));
    
    anti_trans((u8_t *)send_buf, sizeof(modbus_wr_t));
    r_memset(antibuf.payload,0,sizeof(modbus_rd_response_t));
    APP_DEBUG("send anti reflux data\r\n");
    memory_release(send_buf);
    

}

 /*          
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧湱鈧懓瀚崳纾嬨亹閹烘垹鍊炲銈嗗坊閸嬫挻娼诲┑瀣拺闂侇偆鍋涢懟顖涙櫠閹绢喗鐓涢悘鐐插⒔閵嗘帡鏌嶈閸撱劎绱為崱娑樼；闁告侗鍘鹃弳锔锯偓鍏夊亾闁告洦鍓涢崢闈涱渻閵堝棙顥嗛柛瀣姉缁棃鎼归崗澶婁壕閻熸瑥瀚径鍕煕鐎ｎ亷宸ラ柣锝囧厴閹垻鍠婃潏銊︽珜濠电偠鎻徊鎸庣仚婵犮垻鎳撶粔鐟邦潖婵犳艾纾兼繛鍡樺笒閸橈紕绱撴笟鍥ф珮闁搞劌鐖奸悰顕€骞囬弶璺啇婵炶揪绲介幗婊堟晬濞嗘劗绡€闁汇垽娼ф牎闂佺厧缍婄粻鏍箖閻愬搫鍨傛い鎰С缁ㄥ姊洪悷鐗堟儓缂佸澧庨幏瑙勫鐎涙鍘遍梺缁樻椤ユ挾绮绘繝姘厸閻忕偛澧藉ú鎾煙椤旇娅呴柣锝囧厴瀹曟儼顦柟铚傚嵆閺岋絾鎯旈妶搴㈢秷濠电偛寮堕敃銏犵暦濠靛棌鏋庨柟鎹愭硾缁侊箓姊洪崫鍕犻柛鏂跨У閸掑﹪骞橀鐣屽幗闂佺粯姊婚埛鍫ュ汲閹烘鐓曢柟瀛樼矌閻瑦鎱ㄦ繝鍕笡闁瑰嘲鎳愮划娆忊枎閻愵剦妫忛梻鍌欒兌鏋い鎴濇嚇楠炲﹥鎯旈敐鍛柧闂傚倷绀侀幖顐ょ矙娓氣偓瀹曟垿宕卞Ο灏栨灃閻庡箍鍎遍幃鑳亹閹烘挸浜楅柟鑹版彧缁查箖骞夋總鍛娾拺缂備焦蓱鐏忕増绻涢懠顒€鏋涚€殿喖顭峰鎾偄妞嬪海鐛繝纰樻閸ㄩ潧鈻嶉敐鍡欘浄缂佸绨遍弨浠嬫煥濞戞ê顏俊鐐倐閺屾盯鎮╁畷鍥р拰闂佺硶鏅濋崑銈夌嵁鐎ｎ喗鏅滅紓浣股戝▍鏃堟⒒娴ｈ櫣銆婇柛鎾寸箞閺佸顪冮妶鍛寸崪闁瑰嚖鎷�
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掑鏅悷婊冪Ч濠€渚€姊虹紒妯虹伇婵☆偄瀚板鍛婄瑹閳ь剟寮诲☉妯锋婵鐗婇弫鎯р攽閻愬弶鍣藉┑顔肩仛缁岃鲸绻濋崶顬囨煕濞戝崬鏋涙繛鍜冪節濮婃椽鏌呴悙鑼跺濠⒀屽櫍閺屾盯鎮㈤崨濠勭▏闂佷紮绲块崗妯讳繆閹间礁鐓涘ù锝嗘そ閻涙捇姊绘担鐑樺殌妞ゆ洦鍙冨畷鎴︽倷閸ㄦ稑褰氶梻鍌氬€峰ù鍥敋瑜庨〃銉╁箹娴ｇǹ鍋嶅┑鐘诧工閻楀棛澹曟繝姘厵闁告挆鍛闂佺粯鎸诲ú鐔煎蓟閿熺姴鐐婇柕澶堝劚椤牊绻濋埛鈧仦鑺ョ亪闂傚洤顦甸弻锝咁潨閳ь剙顭囪瀹曟繄鈧綆鍓涚壕鑲╃磽娴ｈ鐒介柍顖涙礃缁绘盯鎮℃惔锝囶啋閻庤娲樼划蹇浰囬弻銉︾厱闁宠桨鑳堕悞鍛婃叏婵犲啯銇濈€规洦鍋婃俊鐑藉Ψ閿旈敮鍋撻銏♀拺闁告稑锕ラ埛鎰版煕閵娿儳浠㈤柣锝呭槻鐓ゆい蹇撳閸旓箑顪冮妶鍡楃瑐闂傚嫬绉电粋宥呪堪閸喓鍘甸梺鍛婄箓鐎氼參藟濠婂厾鐟扳堪閸垻鏆┑顔硷攻濡炶棄鐣烽锕€唯闁靛濡囬埀顒佸劤閳规垿鎮欓幓鎺撳€梺鑽ゅ暱閺呯娀濡存担鍓叉僵闁肩ǹ鐏氬▍婊勭節閵忥絽鐓愰拑閬嶆煛閸涱喚绠炴慨濠冩そ楠炴劖鎯旈姀鈺傗挅婵犵妲呴崑鍕偓姘煎幘缁顓兼径瀣姸閻庡箍鍎卞Λ宀勫箯濞差亝鈷戦柛娑橈攻鐏忔壆鎲搁弶鍨殲缂佸倸绉撮埞鎴﹀醇閵忣澁绱查梺鑽ゅТ濞诧箒銇愰崘顕呮晜妞ゆ挾鍋愰弨浠嬫煃閵夈儳锛嶉柛鈺嬬秮閺屸剝鎷呯憴鍕３闂佽桨鐒﹂幑鍥极閹剧粯鏅搁柨鐕傛嫹
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
                        //濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌涘☉姗堟敾闁告瑥绻愰湁闁稿繐鍚嬬紞鎴︽煕閵娿儱鈧潡寮婚敐澶婄鐎规洖娲ら崫娲⒑閸濆嫷鍎愰柣妤侇殘閹广垹鈽夐姀鐘殿吅闂佺粯鍔樼亸娆撳礉娴煎瓨鈷戠紒瀣儥閸庢劙鏌熼崨濠冨€愰柨婵堝仜閳规垹鈧綆鍋勬禍妤呮煙閸忚偐鏆橀柛銊︽閵囨劖鎯旈～顓犵畾闂侀潧鐗嗗ú銈呮毄闂備胶枪椤戝棝鎯勯姘辨殾妞ゆ牜鍎愰弫宥嗙節婵犲倸顏╅幖鏉戯工閳规垿鎮╃紒妯婚敪濡炪倖鍨甸幊姗€骞冨▎鎰瘈闁搞儯鍔庨崢顏呯節閻㈤潧鈧垶宕橀妸锔筋唲闂佸摜鍠愰幃鍌氼潖婵犳艾纾兼慨姗嗗厴閸嬫捇鎮滈懞銉ユ畱闂佸憡鎸烽悞锕傚礂濠婂牊鐓曟い鎰剁稻缁€鈧紓浣插亾闁割偆鍠撶弧鈧梻鍌氱墛娓氭宕曞澶嬬厓鐟滄粓宕滃▎鎾崇柈闁哄鍨归弳锕傛煏婵炵偓娅撻柡浣哥У缁绘繃绻濋崒娑橆€涘┑陇灏欐灙闁宠鍨块幃娆戔偓娑櫭棄宥夋⒑缁洘娅呴柛鐔告綑閻ｇ兘骞嬮敃鈧粻鑽ょ磽娴ｈ鐒介柛妯绘倐閺岋綀绠涢弴鐐扮捕婵犫拃鍡橆棄閻撱倝鏌熺紒銏犳灍闁绘挻绋戦湁闁挎繂娲﹂崵鈧繝娈垮枛缁夊爼鍩€椤掑喚娼愭繛鍙夛耿閺佸啴濮€閳ヨ尙绠氶梺褰掓？缁€浣告暜婵＄偑鍊栧Λ鍐极椤曗偓瀹曟垿骞樼拠鑼啋濡炪倖妫佽闁瑰嘲鎼埞鎴︻敊閻愵剙娈屽┑鐐茬湴閸婃繈骞冮悙瑁佹椽顢旈崨顖氬箞闂備線娼ц墝闁哄懏鐟╅獮澶愭倷椤掑顔旈梺缁樺姈濞兼瑦鎱ㄥ鍥ｅ亾鐟欏嫭绀冮柨姘亜閺傝法绠绘い銏＄懇瀹曟鍠婇崡鐐村亼闂傚倸鍊搁崐椋庢濮橆剦鐒界憸鎴炴櫠濠靛鈷戦柛婵嗗閸ｈ櫣绱掗鑺ュ碍闁伙絿鍏橀幃鐣屽枈濡桨澹曢梺鎸庣箓妤犳悂寮搁悢鍏肩厽闁规儳鐡ㄧ粈鍐煏閸パ冾伃妞ゃ垺锕㈤幃銏☆槹鎼存繃妯婃繝鐢靛仜閻°劎鍒掑鍥ㄥ床闁告劦鍠栨闂佸憡娲﹂崹鏉课涘鈧幃褰掑箒閹烘垵顥庨悷婊冮叄楠炲牓濡搁敂鍓х槇闂佸憡鍔忛弬鍌涚閵忋倖鍊甸悷娆忓缁€鍫ユ煕韫囨棑鑰块柕鍡曠椤粓鍩€椤掑嫬绠栭柕鍫濇婵挳姊洪崹顕呭剰闁绘挻娲熷濠氬磼濮橆兘鍋撻悜鑺ュ殑闁割偅娲栫粻鐘绘煙閹规劦鍤欓柛姘秺閺屸€愁吋鎼粹€崇缂備胶濯崹鍫曞蓟閵娾晜鍋嗛柛灞剧☉椤忥拷
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掑鏅悷婊冪Ч濠€渚€姊虹紒妯虹伇婵☆偄瀚板鍛婄瑹閳ь剟寮诲☉妯锋婵鐗婇弫鎯р攽閻愬弶鍣藉┑顔肩仛缁岃鲸绻濋崶顬囨煕濞戝崬鏋涙繛鍜冪節濮婃椽鏌呴悙鑼跺濠⒀屽櫍閺屾盯鎮㈤崨濠勭▏闂佷紮绲块崗妯讳繆閹间礁鐓涘ù锝嗘そ閻涙捇姊绘担鐑樺殌妞ゆ洦鍙冨畷鎴︽倷閸ㄦ稑褰氶梻鍌氬€峰ù鍥敋瑜庨〃銉╁箹娴ｇǹ鍋嶅┑鐘诧工閻楀棛澹曟繝姘厵闁告挆鍛闂佺粯鎸搁崯浼村箟缁嬪簱鍫柛顐ｇ箘椤︻參姊虹紒妯活梿闁稿鍔曢悾鍨瑹閳ь剟鎮￠锕€鐐婇柕濠忓閿涙洟姊虹粙娆惧剱闁圭懓娲顐﹀箛椤撶喎鍔呴梺鏂ユ櫅閸熺増绂嶉鍡欑＝闁稿本鐟ㄩ崗灞解攽椤曗偓濞佳囷綖濠靛惟闁宠桨鑳堕悿鍛磽娴ｅ壊鍎忕紒銊╀憾瀹曞ジ顢旈崼鐔哄幈闂婎偄娲﹀鍦偓姘礈缁辨挸顓奸崟顓犵崲濡ょ姷鍋涢崯顖滅矉閹烘柡鍋撻敐搴濈盎缂佹劖绋掔换婵嬫偨闂堟刀銏ゆ倵濮樼厧澧撮柣娑卞枛椤粓鍩€椤掆偓椤繒绱掑Ο璇差€撻梺鍛婄☉閿曘倝寮抽崼銏㈢＝濞达絿鐡旈崵娆撴煕婵犲偆鐓奸柛鈹惧亾濡炪倖甯婄粈渚€宕甸鍕厱闁规崘娉涢弸娑氣偓瑙勬礃濡炰粙宕洪埀顒併亜閹哄秹妾峰ù婊勭矒閺岀喖宕崟顒夋婵炲瓨绮嶉崕鎶解€旈崘顔嘉ч幖绮光偓宕囶啇闂備焦鎮堕崹娲偂閿熺姴绠栧Δ锝呭暞閸婂鏌﹀Ο渚Ш妞ゆ挻妞藉娲箰鎼淬埄姊挎繝娈垮枔閸婃洟鍩㈠澶婎潊闁靛牆妫岄幏娲煟閻樺厖鑸柛鏂胯嫰閳诲秹骞囬悧鍫㈠幍闂佸憡鍨崐鏍偓姘炬嫹
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掑鏅悷婊冪Ч濠€渚€姊虹紒妯虹伇婵☆偄瀚板鍛婄瑹閳ь剟寮诲☉妯锋婵鐗婇弫鎯р攽閻愬弶鍣藉┑顔肩仛缁岃鲸绻濋崶顬囨煕濞戝崬鏋涙繛鍜冪節濮婃椽鏌呴悙鑼跺濠⒀屽櫍閺屾盯鎮㈤崨濠勭▏闂佷紮绲块崗妯讳繆閹间礁鐓涘ù锝嗘そ閻涙捇姊绘担鐑樺殌妞ゆ洦鍙冨畷鎴︽倷閸ㄦ稑褰氶梻鍌氬€峰ù鍥敋瑜庨〃銉╁箹娴ｇǹ鍋嶅┑鐘诧工閻楀棛澹曟繝姘厵闁告挆鍛闂佺粯鎸诲ú鐔煎蓟閻旂厧绾ч柛顭戝櫘閺嗐垻绱撴担鎴掑惈濠⒀勵殜楠炲牓濡歌閸犲棝鏌涢弴鐐典粵闁汇倓绀侀埞鎴︻敊绾兘绶村┑鐐叉嫅缂嶄線鐛径鎰妞ゆ棁鍋愰ˇ鏉款渻閵堝棗濮傞柛銊ь攰椤ｄ粙姊婚崒姘偓椋庣矆娓氣偓楠炴饪伴崼婵堢崶闁硅偐琛ュΣ鍛存倿娴犲鈷旈柛銉墯閸庢螖閿濆懎鏆為柍閿嬪灴閺屾稑鈽夊鍫濆闂佸疇妫勯ˇ鐢稿蓟閿濆棗绠犻梺鍛婃⒐閸ㄧ敻鎮鹃悜钘夌闁挎棁濮ゅ▍婊堟⒑閸涘﹤鎼搁柛濠冩礈缁辩偞绻濋崶褎鐎梺鍦濠㈡﹢鎮欐繝鍥ㄧ厪濠电偛鐏濋崝婊堟煟閿濆骸澧存慨濠勭帛閹峰懘宕ㄦ繝鍌涙畼闂備礁鎼惌澶岀礊娴ｅ摜鏆﹂柟鐗堟緲閸愨偓濡炪倖鎸鹃崐锝夋晝閸屾稓鍘遍梺鍝勬储閸斿矂鎮橀敓鐘崇厸闁告侗鍠氶惌鎺楁煙椤旇偐绉虹€规洖鐖兼俊鎼佸Ψ瑜忛妶鐑芥⒒娴ｇǹ鏆遍柛銏＄叀椤㈡牗寰勬繝搴㈢稁闂佹儳绻楅～澶屸偓姘哺閺屽秹濡烽妷褝绱炴繛瀵稿Л閺呯娀寮婚敐鍡樺劅闁靛繒濮村В鍫ユ⒑閸濄儱校鐎光偓缁嬭法鏆︽慨妯挎硾缁犳娊鏌熺€涙ɑ鈷愰柣搴☆煼濮婃椽鎮烽柇锕€娈堕梺绋款儐缁嬫挻绔熼弴銏╂晬闁绘劗琛ラ幏娲⒒閸屾氨澧涢柤鍐叉閵囨劙骞掗幋鐙呯吹闂備浇顫夐崕鐓幬涢崟顐嵮囧蓟閵夛妇鍘搁梺鍛婂姂閸斿孩鏅跺☉銏＄厓闂佸灝顑呯粭鎺楁婢舵劖鐓ユ繝闈涙閸ｆ椽鎮归幇銊ュ⒉缂佺粯绻堥崺鈧い鎺嶈兌椤╃兘鎮楅敐搴′簽闁告﹢浜跺娲传閸曨偀鍋撻挊澶嗘灃闁哄洢鍨洪崐鍫曟⒑椤掆偓缁夌敻鎮￠崘顏呭枑婵犲﹤鐗嗙粈鍫熸叏濡寧纭剧紒鈧崟顓熷枑闁绘鐗嗙粭鎺楁煕鐏炶濡奸摶鏍煥濠靛棙鍣归柡鍡樼懇閺岋綁骞掗幋鐘辩驳闂侀潧娲ょ€氫即鐛幒妤€骞㈡俊鐐村劤椤ユ岸姊婚崒娆愮グ妞ゆ泦鍛床闁瑰瓨绻嶅鈺呮煏婵炵偓娅呯紒鎰殔閳规垿鎮╅幓鎺嶇敖闂佸憡鑹鹃澶愬蓟濞戙垹唯闁靛繆鍓濋悵鏍⒑缂佹ê绗掓繛灏栤偓鎰佹綎婵炲樊浜滈幑鑸点亜閹捐泛浠滃┑鈥虫惈椤啴濡堕崘銊ヮ瀳濠碘槅鍋呯换鍌烆敋閿濆钃熼柕澶堝劤椤旀劖绻涙潏鍓у埌婵犫偓闁秴鐭楅柛鈩冪⊕閳锋垹绱撴担鑲℃垹绮堥埀顒勬⒑缁嬪尅宸ユ繝鈧柆宥呯劦妞ゆ帊鑳堕崯鏌ユ煙閸戙倖瀚�  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧湱鈧懓瀚崳纾嬨亹閹烘垹鍊炲銈嗗坊閸嬫挻娼诲┑瀣拺闂侇偆鍋涢懟顖涙櫠閹绢喗鐓涢悘鐐插⒔閵嗘帡鏌嶈閸撱劎绱為崱娑樼；闁告侗鍘鹃弳锔锯偓鍏夊亾闁告洦鍓涢崢闈涱渻閵堝棙顥嗛柛瀣姉缁棃鎼归崗澶婁壕閻熸瑥瀚径鍕煕鐎ｎ亷宸ラ柣锝囧厴閹垻鍠婃潏銊︽珜濠电偠鎻徊鎸庣仚婵犮垻鎳撶粔鐟邦潖婵犳艾纾兼繛鍡樺笒閸橈紕绱撴笟鍥ф珮闁搞劌鐖奸悰顕€骞囬弶璺啇婵炶揪绲介幗婊堟晬濞嗘劗绡€闁汇垽娼ф牎闂佺厧缍婄粻鏍箖閻愬搫鍨傛い鎰С缁ㄥ姊洪悷鐗堟儓缂佸澧庨幏瑙勫鐎涙鍘遍梺缁樻椤ユ挾绮绘繝姘厸閻忕偛澧藉ú鎾煙椤旇娅呴柣锝囧厴瀹曟儼顦柟铚傚嵆閺岋絾鎯旈妶搴㈢秷濠电偛寮堕敃銏犵暦濠靛棌鏋庨柟鎹愭硾缁侊箓姊洪崫鍕犻柛鏂跨У閸掑﹪骞橀鐣屽幗闂佺粯姊婚埛鍫ュ汲閹烘鐓曢柟瀛樼矌閻瑦鎱ㄦ繝鍕笡闁瑰嘲鎳愮划娆忊枎閻愵剦妫忛梻鍌欒兌鏋い鎴濇嚇楠炲﹥鎯旈敐鍛柧闂傚倷绀侀幖顐ょ矙娓氣偓瀹曟垿宕卞Ο灏栨灃閻庡箍鍎遍幃鑳亹閹烘挸浜楅柟鑹版彧缁查箖骞夋總鍛娾拺缂備焦蓱鐏忕増绻涢懠顒€鏋涚€殿喖顭峰鎾偄妞嬪海鐛繝纰樻閸ㄩ潧鈻嶉敐鍡欘浄缂佸绨遍弨浠嬫煥濞戞ê顏俊鐐倐閺屾盯鎮╁畷鍥р拰闂佺硶鏅濋崑銈夌嵁鐎ｎ喗鏅滅紓浣股戝▍鏃堟⒒娴ｈ櫣銆婇柛鎾寸箞閺佸顪冮妶鍛寸崪闁瑰嚖鎷�
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐鎼佸磹瀹勬噴褰掑炊椤掑鏅悷婊冪Ч濠€渚€姊虹紒妯虹伇婵☆偄瀚板鍛婄瑹閳ь剟寮诲☉妯锋婵鐗婇弫鎯р攽閻愬弶鍣藉┑顔肩仛缁岃鲸绻濋崶顬囨煕濞戝崬鏋涙繛鍜冪節濮婃椽鏌呴悙鑼跺濠⒀屽櫍閺屾盯鎮㈤崨濠勭▏闂佷紮绲块崗妯讳繆閹间礁鐓涘ù锝嗘そ閻涙捇姊绘担鐑樺殌妞ゆ洦鍙冨畷鎴︽倷閸ㄦ稑褰氶梻鍌氬€峰ù鍥敋瑜庨〃銉╁箹娴ｇǹ鍋嶅┑鐘诧工閻楀棛澹曟繝姘厵闁告挆鍛闂佺粯鎸诲ú鐔煎蓟閿熺姴鐐婇柕澶堝劚椤牊绻濋埛鈧仦鑺ョ亪闂傚洤顦甸弻锝咁潨閳ь剙顭囪瀹曟繄鈧綆鍓涚壕鑲╃磽娴ｈ鐒介柍顖涙礃缁绘盯鎮℃惔锝囶啋閻庤娲樼划蹇浰囬弻銉︾厱闁宠桨鑳堕悞鍛婃叏婵犲啯銇濈€规洦鍋婃俊鐑藉Ψ閿旈敮鍋撻銏♀拺闁告稑锕ラ埛鎰版煕閵娿儳浠㈤柣锝呭槻鐓ゆい蹇撳閸旓箑顪冮妶鍡楃瑐闂傚嫬绉电粋宥呪堪閸喓鍘甸梺鍛婄箓鐎氼參藟濠婂厾鐟扳堪閸垻鏆┑顔硷攻濡炶棄鐣烽锕€唯闁靛濡囬埀顒佸劤閳规垿鎮欓幓鎺撳€梺鑽ゅ暱閺呯娀濡存担鍓叉僵闁肩ǹ鐏氬▍婊勭節閵忥絽鐓愰拑閬嶆煛閸涱喚绠炴慨濠冩そ楠炴劖鎯旈姀鈺傗挅婵犵妲呴崑鍕偓姘煎幘缁顓兼径瀣姸閻庡箍鍎卞Λ宀勫箯濞差亝鈷戦柛娑橈攻鐏忔壆鎲搁弶鍨殲缂佸倸绉撮埞鎴﹀醇閵忣澁绱查梺鑽ゅТ濞诧箒銇愰崘顕呮晜妞ゆ挾鍋愰弨浠嬫煃閵夈儳锛嶉柛鈺嬬秮閺屸剝鎷呯憴鍕３闂佽桨鐒﹂幑鍥极閹剧粯鏅搁柨鐕傛嫹
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
                        //濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌涘☉姗堟敾闁告瑥绻愰湁闁稿繐鍚嬬紞鎴︽煕閵娿儱鈧潡寮婚敐澶婄鐎规洖娲ら崫娲⒑閸濆嫷鍎愰柣妤侇殘閹广垹鈽夐姀鐘殿吅闂佺粯鍔樼亸娆撳礉娴煎瓨鈷戠紒瀣儥閸庢劙鏌熼崨濠冨€愰柨婵堝仜閳规垹鈧綆鍋勬禍妤呮煙閸忚偐鏆橀柛銊︽閵囨劖鎯旈～顓犵畾闂侀潧鐗嗗ú銈呮毄闂備胶枪椤戝棝鎯勯姘辨殾妞ゆ牜鍎愰弫宥嗙節婵犲倸顏╅幖鏉戯工閳规垿鎮╃紒妯婚敪濡炪倖鍨甸幊姗€骞冨▎鎰瘈闁搞儯鍔庨崢顏呯節閻㈤潧鈧垶宕橀妸锔筋唲闂佸摜鍠愰幃鍌氼潖婵犳艾纾兼慨姗嗗厴閸嬫捇鎮滈懞銉ユ畱闂佸憡鎸烽悞锕傚礂濠婂牊鐓曟い鎰剁稻缁€鈧紓浣插亾闁割偆鍠撶弧鈧梻鍌氱墛娓氭宕曞澶嬬厓鐟滄粓宕滃▎鎾崇柈闁哄鍨归弳锕傛煏婵炵偓娅撻柡浣哥У缁绘繃绻濋崒娑橆€涘┑陇灏欐灙闁宠鍨块幃娆戔偓娑櫭棄宥夋⒑缁洘娅呴柛鐔告綑閻ｇ兘骞嬮敃鈧粻鑽ょ磽娴ｈ鐒介柛妯绘倐閺岋綀绠涢弴鐐扮捕婵犫拃鍡橆棄閻撱倝鏌熺紒銏犳灍闁绘挻绋戦湁闁挎繂娲﹂崵鈧繝娈垮枛缁夊爼鍩€椤掑喚娼愭繛鍙夛耿閺佸啴濮€閳ヨ尙绠氶梺褰掓？缁€浣告暜婵＄偑鍊栧Λ鍐极椤曗偓瀹曟垿骞樼拠鑼啋濡炪倖妫佽闁瑰嘲鎼埞鎴︻敊閻愵剙娈屽┑鐐茬湴閸婃繈骞冮悙瑁佹椽顢旈崨顖氬箞闂備線娼ц墝闁哄懏鐟╅獮澶愭倷椤掑顔旈梺缁樺姈濞兼瑦鎱ㄥ鍥ｅ亾鐟欏嫭绀冮柨姘亜閺傝法绠绘い銏＄懇瀹曟鍠婇崡鐐村亼闂傚倸鍊搁崐椋庢濮橆剦鐒界憸鎴炴櫠濠靛鈷戦柛婵嗗閸ｈ櫣绱掗鑺ュ碍闁伙絿鍏橀幃鐣屽枈濡桨澹曢梺鎸庣箓妤犳悂寮搁悢鍏肩厽闁规儳鐡ㄧ粈鍐煏閸パ冾伃妞ゃ垺锕㈤幃銏☆槹鎼存繃妯婃繝鐢靛仜閻°劎鍒掑鍥ㄥ床闁告劦鍠栨闂佸憡娲﹂崹鏉课涘鈧幃褰掑箒閹烘垵顥庨悷婊冮叄楠炲牓濡搁敂鍓х槇闂佸憡鍔忛弬鍌涚閵忋倖鍊甸悷娆忓缁€鍫ユ煕韫囨棑鑰块柕鍡曠椤粓鍩€椤掑嫬绠栭柕鍫濇婵挳姊洪崹顕呭剰闁绘挻娲熷濠氬磼濮橆兘鍋撻悜鑺ュ殑闁割偅娲栫粻鐘绘煙閹规劦鍤欓柛姘秺閺屸€愁吋鎼粹€崇缂備胶濯崹鍫曞蓟閵娾晜鍋嗛柛灞剧☉椤忥拷
                        
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

/*

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
*/


/*******************************************************************************            
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
static u8_t checkDeviceOnline(void)
{
    static int onlineDeviceCount = 0;
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁惧墽鎳撻—鍐偓锝庝簻椤掋垺銇勯幇顖毿撻柟渚垮妼椤粓宕卞Δ鈧埛鎺楁⒑缂佹ê绗傜紒顔界懇瀵濡堕崱妯哄伎闂佸綊鍋婇崗姗€宕戦幘璇插唨妞ゆ劗鍠庢禍鐐叏濮楀棗澧婚柣鎾炽偢閺屸€崇暆鐎ｎ剛袦閻庢鍣崳锝呯暦閹烘埈娼╂い鎴ｆ硶鐢稓绱撻崒姘偓椋庢媼閺屻儱纾婚柟鐐墯閻斿棝鏌ら幖浣规锭濠殿喖娲ㄧ槐鎺楁偐娓氼垱缍堢紓浣虹帛閻╊垶寮幇顓熷劅闁炽儲鍓氬璇测攽閻樻鏆柍褜鍓涢崑銊╁磻閵忋倖鐓涢悘鐐插⒔閳藉銇勯锝囩疄妞ゃ垺顨婂畷鎺戔攦閻愵亜濡奸摶鏍煟濮椻偓濞佳勭閿曞倹鐓熸俊銈勭贰濞堟梹銇勯鐐寸┛缂佺姵绋戦埥澶娢熺喊杈ㄐら梺鑽ゅ枑缁瞼鎹㈠Δ鍛妞ゆ劧绲跨弧鈧梺姹囧灲濞佳勭墡婵＄偑鍊栧褰掓偋閻樿尙鏆﹂悷娆忓椤曢亶鏌℃径瀣鐟滄棃寮婚悢鍏尖拻閻庨潧澹婂Σ顕€姊洪挊澶婃殶闁哥姵鐗犲濠氭晲婢跺﹦鐫勯梺绋挎湰椤ㄥ棛鈧碍鐩鐑樻姜閹殿噮妲紓浣割槸缂嶅﹤顕ｇ拠娴嬫闁靛繒濮烽崝鎾⒑閸涘﹤濮夐柛瀣崌閹儳鐣￠幍铏杸闂佺粯锕╅崑鍕閻愵剛绠鹃柡澶嬪灥閹垶绻涢崗鑲╂噰鐎规洘锕㈤崺鈧い鎺戝閳锋垿鏌ｉ幇顖涱棄闁告梹绮撻弻锛勨偓锝庝簵閸嬨垻鈧鍠撻崝宥囩矉閹烘柡鍋撻敐搴′簽闁告ü绮欏楦裤亹閹烘垳鍠婇梺鍛婎焽閺咁偆妲愰悙鍝勭闁挎梻鏅崢浠嬫椤愩垺鍌ㄩ柛搴㈠▕閹箖鎮介崨濠勫幐閻庡厜鍋撻柍褜鍓熷畷浼村箻鐠哄搫鐏婂┑鐐叉閹稿摜绮绘导鏉戠婵烇綆鍓欓悘鈺呮煛鐎ｎ偄鐏╃紒杈ㄦ崌瀹曟帒鈻庨幋锝囩崶闂備礁鎽滄慨鐢搞€冮崨鎵罕闂備礁鎲￠崝鎴﹀礉鎼淬劌姹查柨鏇楀亾闁宠鍨垮畷鎺戔攦閹惧磭绉虹€规洩绻濋獮搴ㄦ嚍閵壯冨箺闂備礁鎼崯顐⒚洪敃鍌涘殘閻熸瑥瀚换鍡涙煕濞嗗浚妲稿┑顔肩У閹便劍绻濋崟顓炵闂佺懓鍢查幊姗€骞冨⿰鍏剧喖鎮滃Ο鍏兼婵犵绱曢崑鎴﹀磹閺嶎灐娲晝閸屾氨鐓戦梺鍛婂姂閸斿酣寮抽敃鍌涚厵閺夊牓绠栧顕€鏌涚€ｎ亜顏紒杈ㄥ笒铻栭柛鏇楁杹閸嬫捇鍩€椤掑嫭鐓曢悗锝庡亝瀹曞瞼鈧娲忛崝鎴濐嚕閸洖绠ｉ柨婵嗘閸熸椽姊婚崒娆戠獢婵炰匠鍤躲劑鍩€椤掆偓閳规垿鍨鹃搹顐㈡灎閻庤娲╃紞浣哥暦閸楃偐妲堟繛鍡楁湰鐎氳偐绱撻崒姘偓鐑芥倿閿曚焦鎳岀紓鍌欒閸嬫捇鏌嶈閸撶喎顫忛搹鍦煓闁告牑鈧厖绱ｆ繝鐢靛Л閸嬫捇姊洪鈧粔鎾倿閸偁浜滈柟鍝勭Х閸忓矂鏌涢悢鍝ュ弨闁哄瞼鍠栧畷娆撳Χ閸℃浼�0,
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁惧墽鎳撻—鍐偓锝庝簼閹癸綁鏌ｉ鐐搭棞闁靛棙甯掗～婵嬫晲閸涱剙顥氬┑掳鍊楁慨鐑藉磻閻愮儤鍋嬮柣妯荤湽閳ь兛绶氬鎾閳╁啯鐝曢梻浣藉Г閿氭い锔诲枤缁辨棃寮撮姀鈾€鎷绘繛杈剧秬濞咃綁濡存繝鍥ㄧ厱闁规儳顕粻鐐烘煙閽樺鈧鍩€椤掑﹦绉甸柛鐘崇墵閹瑦绻濋崘锔跨盎闂佺懓鎼粔鐑藉礂瀹€鍕厱闁哄倽娉曢崺锝夋煙椤旂瓔娈滈柣娑卞櫍瀹曞綊顢欓悡搴經闂傚倷鑳堕幊鎾诲疮鐠恒劍宕叉慨妞诲亾妤犵偛鍟灃闁告粈鐒﹂弲銏ゆ⒑閸涘﹥澶勯柛蹇旓耿楠炲繐煤椤忓應鎷洪梺鍛婄☉閿曪箓鍩ユ径鎰叆闁哄浂浜滈々顒傜磼鏉堛劍灏い顐ｇ箞閹虫粓鎮介棃娑樼疄闂備胶鎳撻崥瀣偩椤忓牆鍨傚┑鐘宠壘閻鏌嶈閸撶喖骞冨Δ鍐╁枂闁告洦鍓涢ˇ銊╂⒑閹肩偛濡块柡浣割煼閺佹劙鎮欏ù瀣墯闂佸壊鍋呯换鍕礉閿曗偓椤啴濡堕崱妤冪懆闁诲孩鍑归崜鐔煎箯閹达附鍋勯柛蹇氬亹閸橆亝绻濋悽闈涗户闁稿鎸搁埢宥夊幢濞戞瑧鍘遍柣搴秵娴滄繈藟閵忋倖鐓涢悘鐐插⒔閵嗘帒霉閻欏懐鐣电€规洘绮忛ˇ顔戒繆閹绘帞澧曟い顏勫暣婵″爼宕ㄧ€电ǹ妞介梻浣烘嚀閸ゆ牠骞忛敓锟�
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
                        if (0x0212 == (online->code))           //婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繘鏌ｉ幋锝嗩棄闁哄绶氶弻鐔兼⒒鐎靛壊妲紒鐐劤椤兘寮婚敐澶婄疀妞ゆ帊鐒﹂崕鎾绘⒑閹肩偛濡奸柛濠傛健瀵鈽夐姀鈺傛櫇闂佹寧绻傚Λ娑⑺囬妷褏纾藉ù锝呮惈灏忛梺鍛婎殕婵炲﹤顕ｆ繝姘亜闁稿繐鐨烽幏濠氭煟鎼达紕浠涢柣鈩冩礈缁鎮欓悜妯锋嫼闂佺鍋愰崑娑㈠礉閹绢喗鐓曢柟鎯ь嚟閹冲懏绻涢幋鐘虫毈闁糕斁鍋撳銈嗗笒鐎氼參鍩涢幋锔藉仯闁搞儺浜滈惃铏圭磼閻樻彃绾уǎ鍥э躬瀹曪絾寰勬繝鍐檨闂備焦瀵х换鍕磻閻愬樊鍤楅柛鏇ㄥ墰缁♀偓闁硅偐琛ラ崜婵嬵敊閸岀偞鈷掑ù锝呮啞閸熺偞绻涚拠褏鐣电€规洖缍婇獮宥夘敊閸欍儳鐟濇繝鐢靛仦閸ㄥ爼鎮烽敐鍡欘洸婵犲﹤鐗婇悡娆撴煙鐟欏嫬濮堢痪顓熷劤闇夐柣妯哄暱缁椦呯磼鏉堛劍宕岀€规洘甯掗埥澶娾枎韫囨洦鏆￠梻鍌欑閻ゅ洭锝炴径鎰瀭闁秆勵殔缁犳牗绻涢崱妯诲鞍闁搞倕顑夊濠氬醇閻旇　濮囬梻浣诡儥閸樺墽妲愰幘璇茬＜婵炲棙鍔楅妶浼存倵濞堝灝鏋熼柟鎼佺畺瀹曟岸骞掗幋鏃€鐎婚棅顐㈡处濞叉ê鈻撻幆褜娓婚柕鍫濇婢ч亶鏌涚€ｎ偆鈯曢柛搴亰濮婄粯鎷呴崫銉︾€┑鈩冦仠閸旀垿骞婂Δ鍛唶闁哄洨鍋熼ˇ顕€姊洪幐搴㈢５闁稿鎸婚妵鍕敂閸曨偅娈绘繝纰樷偓宕囧煟鐎规洖宕灃闁告劦浜濋崳顖炴⒒閸屾瑧鍔嶉悗绗涘厾楦跨疀濞戞锛熼梺瑙勫劶濡嫰鎮為崹顐犱簻闁瑰搫绉烽崗宀勬煕閻斿摜鍙€闁哄瞼鍠栧畷娆撳Χ閸℃浼�
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
    float f;

    antibuf.payload=NULL;
    meterPower.state = 0;
    list_init(&invertDevice);

    //get para 96,anti reflux enable or disable?
    SysPara_Get(96, &buf);
    //Swap_numChar(buf_value,onlineDeviceList.count);     //鐏忥拷16鏉╂稑鍩楅弫鏉垮嬀鏉烆兛璐熺€涙顑佹稉锟�
    //buf_value = Swap_charNum((char *) * (int *)buf.payload);   //鐏忓棗鐡х粭锔胯鏉烆兛璐�16鏉╂稑鍩�
    buf_value = Swap_charNum((char *)buf.payload);   //鐏忓棗鐡х粭锔胯鏉烆兛璐�16鏉╂稑鍩�
    APP_DEBUG("anti_reflux_en=%ld\r\n",buf_value);
    anti_reflux_en=0;       
    if(buf_value)
        anti_reflux_en=1;   //anti reflux enable
    memory_release(buf.payload);

    //get para 97,anti reflux threshlod
    SysPara_Get(97, &buf);
    f=my_atof((char *)buf.payload);
    anti_threshold=f/1000;      //kw
    APP_DEBUG("anti_threshold=%f\r\n",anti_threshold);
    memory_release(buf.payload);

}

int float2int(u32_t data)
{
#if 0  //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閹冣挃闁硅櫕鎹囬垾鏃堝礃椤忎礁浜鹃柨婵嗙凹缁ㄥジ鏌熼惂鍝ョМ闁哄矉缍侀、姗€鎮欓幖顓燁棧闂備線娼уΛ娆戞暜閹烘缍栨繝闈涱儐閺呮煡鏌涘☉鍗炲妞ゃ儲鑹鹃埞鎴炲箠闁稿﹥顨嗛幈銊╂倻閽樺锛涢梺缁樺姉閸庛倝宕戠€ｎ喗鐓熸俊顖濆吹濠€浠嬫煃瑜滈崗娑氭濮橆剦鍤曢柟缁㈠枛椤懘鏌嶉埡浣告殲闁绘繃鐗犲缁樼瑹閳ь剟鍩€椤掑倸浠滈柤娲诲灡閺呭爼寮跺▎鍓у數闁荤喐鐟ョ€氼厾绮堢€ｎ喗鐓欐い鏃€鍎抽崢瀛橆殽閻愯鏀荤紒缁樼箞瀹曟帒饪伴崨鍛姂濮婅櫣鎷犻垾铏亐闂佸搫鎳忛惄顖炪€佸▎鎺旂杸婵炴垼椴搁弲鈺呮⒑缂佹ɑ灏Δ鐘崇摃閻忓洨绱撻崒姘偓鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌涘┑鍕姢闁活厽鎹囬弻锝夊閻樺啿鏆堥梺绋款儏椤戝寮婚敓鐘茬倞闁挎繂妫楀▍姘舵⒑闁偛鑻晶鎯р攽閳ヨ櫕鍠樻鐐村灴瀹曠喖顢涘В灏栨櫊閺屻劑寮村Δ鈧禍鍓х磼閸撗嗘闁搞劏娉涢～蹇涙惞閸︻厾锛滃┑鈽嗗灥濞咃絾绂掗幖浣圭參闁告劦鍠曢幉楣冩煛瀹€鈧崰鏍箖濠婂牆绠ｉ柟鐑樻尭閺嬨倖绻濆▓鍨灈闁挎洏鍔岄埢宥夋晲閸パ冪亰濠电偛妫欓崹璺虹暦婢舵劖鐓忓┑鐐戝啳瀚伴柡浣藉亹缁辨捇宕掑顑藉亾妞嬪孩顐介柨鐔哄Т缁愭淇婇妶鍛櫣缂佺姵鐓￠弻娑㈠Ψ閹存繆瀚版繛鍫涘妽缁绘繈鎮介棃娴讹綁鏌ら崷顓炰壕闁稿寒鍋婂缁樻媴缁嬫寧姣愰梺鍦拡閸嬪﹤鐣烽幇鏉垮窛濠电姴瀚峰ú鎼佹⒑缂佹ê濮夐柛搴涘€濋崺娑㈠箣閻樼數锛滈柣搴秵閸嬪嫰顢氬⿰鍕瘈闁逞屽墴楠炲秹顢欓崜褝绱查梺璇插嚱缂嶅棝宕戦崟顖涘€堕悗娑櫱滄禍婊堟煏韫囧﹤澧叉い銉ョ墦閺屸剝鎷呯憴鍕３闂佽桨鐒﹂幑鍥极閹剧粯鏅搁柨鐕傛嫹 1
    s8_t S;     //缂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ょ紓宥咃躬瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鍓х＜闁绘劦鍓欓崝銈囩磽瀹ュ拑韬€殿喖顭烽弫鎰緞婵犲嫷鍚呴梻浣瑰缁诲倿骞夊☉銏犵缂備焦顭囬崢杈ㄧ節閻㈤潧孝闁稿﹤缍婂畷鎴﹀Ψ閳哄倻鍘搁柣蹇曞仩椤曆勬叏閸屾壕鍋撳▓鍨灍闁瑰憡濞婇獮鍐ㄢ枎瀵版繂婀遍埀顒婄秵娴滄瑦绔熼弴銏♀拺闁告挻褰冩禍婵囩箾閸欏澧甸柟顔惧仱瀹曞綊顢曢悩杈╃泿闂備胶鎳撻幖顐⑽涘Δ浣侯洸濡わ絽鍟埛鎴︽煕濠靛嫬鍔氶柡瀣灴閺岀喖宕ㄦ繝鍌楁灆闂佽鍠曢崡鎶姐€佸璺虹劦妞ゆ巻鍋撻柣锝囧厴瀹曞ジ寮撮妸锔芥珜濠电姰鍨煎▔娑㈩敄閸℃せ鏋嶉柟鍓х帛閳锋帒霉閿濆懏鍟為柛鐔哄仱閺岋綁鎮ら崒婊呅滈悗瑙勬礃濞茬喖鐛惔銊﹀殟闁靛鍎伴悽缁樼節閻㈤潧鈻堟繛浣冲浂鏁勯柛鈩冪☉閻撴洟鏌熼悜妯烘鐟滅増甯楅弲鏌ユ煕閺囥劌娅橀柛鏂挎嚇濮婅櫣鎮伴垾鍏呭濠电偛顕崢褔鎮洪妸鈺傚亗婵炲棙鍔楃粻楣冩煠婵傚壊鏉洪柛鐐舵缁辨帡濡歌濞呮洟鏌嶈閸撴瑩鎮樺顒夌唵婵せ鍋撻柡浣稿暣閺佸倿鎮欓澶嬬稐闂備礁婀遍崕銈夊吹閿曞倹鍋勯柛婵嗗閻庢娊鏌℃径濠勫闁告梹鍨垮顐⑩堪閸啿鎷洪梺鐓庮潟閸婃洟寮搁幋鐘电＜妞ゆ梻鍘ч銏ゆ煛閸涚増纭鹃摶鏍煕閹般劍娅嗘繛鍫熸緲閳规垶骞婇柛濠冩礋楠炲﹨绠涘☉妯硷紱闂佺ǹ鐬奸崑鐐烘偂韫囨搩鐔嗛悹铏瑰劋椤ョ偞绻涢崨顓燁棦闁哄矉缍侀獮妯兼崉閻戞浜梻浣告惈閻鎹㈠┑鍡欐殾闁割偅娲栭悡娑樏归敐澶樻闁硅娲熷缁樼瑹閳ь剟鍩€椤掑倸浠滈柤娲诲灡閺呭爼顢欓悾宀€鐦堥梺閫炲苯澧撮柡灞芥椤撳ジ宕ㄩ姘曞┑锛勫亼閸婃牜鏁繝鍥ㄥ殑闁割偅娲栭悿顕€鏌＄仦璇插姕闁绘挻鐟╅弻娑㈠箣濞嗗繆鍋撻弽顐熷亾濮樼偓瀚�
    s8_t  E;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閹冣挃闁硅櫕鎹囬垾鏃堝礃椤忎礁浜鹃柨婵嗙凹缁ㄥジ鏌熼惂鍝ョМ闁哄矉缍侀、姗€鎮欓幖顓燁棧闂傚倸娲らˇ鐢稿蓟閵娿儮鏀介柛鈾€鏅滄晥濠电偛鐡ㄩ崵搴ㄥ磹濠靛钃熼柕鍫濐槸缁狙囨煙缁嬪潡顎楀ù鐘虫尦閹鈻撻崹顔界亪闂佺粯鐗滈崢褔鎮鹃悜鑺ュ亗閹煎瓨蓱椤秴鈹戦埥鍡楃仧閻犫偓閿曞倹鍊电€瑰嫭澹嬮弨浠嬫煟閹邦剚鈻曢柛銈囧枎閳规垿顢欓悙顒佹瘎闂佸摜濮撮敃銈夘敇婵傜ǹ鐐婇柕濞垮劚缁ㄣ儲绻濋悽闈涒枅婵炰匠鍥舵晞闁糕剝绋戠壕濠氭煏婵炵偓娅嗛柍閿嬪浮閺屾盯顢曢妶鍛亖闂佸磭顑曢崕鐢稿蓟濞戞ǚ鏀介柛銉ㄥ煐閻ｈ泛顪冮妶搴濈盎闁哥喎鐡ㄦ穱濠囧醇閺囩偛鑰垮┑鈽嗗灣閳峰牆危閹殿喚纾介柛灞剧懅鐠愪即鏌涢悩宕囧ⅹ闂囧鏌ｉ幋锝嗩棄缂佲偓閸℃稒鐓欓柣鎴炆戠亸鐢电磼閳锯偓閸嬫挻绻濆▓鍨灍闁挎洍鏅犲畷婊冣槈閵忊晜鏅╅梺鍦劋椤ㄥ棝鍩涢幋鐘冲枑闁绘鐗嗘穱顖炴煛閸℃ê绗х紒杈ㄥ浮閸┾偓妞ゆ帊鐒︽刊瀵哥磼椤栨稒绀冮柣蹇庣窔濮婃椽妫冨ù銉ョ墦瀵彃饪伴崼婵堝幈闂佺鎻梽鍕磻閳╁啰绠鹃柛鈩冾殘缁犵増銇勮箛濠冩珚闁哄本鐩顕€宕堕妸褋鈧劙姊虹€圭媭娼愰柛銊ユ健閵嗕礁鈻庨幘鍏呯炊闂佸憡娲忛崝灞剧閻愵剛绡€闂傚牊绋掗敍宥夋煕閵堝棙绀嬮柡宀嬬磿娴狅妇鎷犻幓鎺戭潛缂傚倷鑳舵慨鐑藉箠閹剧粯绠掗梻浣瑰缁诲倿骞婅箛鏇熷闁跨喓濮甸悡鏇熺箾閸℃绠叉い銉ｅ灪閹便劍绻濋崨顕呬哗缂備浇椴哥敮鎺曠亽闂佺厧顫曢崐銈呂ｉ鍕拻濞达絽鎳欒ぐ鎺撴櫇闁靛牆鎷戦懓鍧楁⒑椤掆偓缁夋挳鎮為崹顐犱簻闁瑰搫绉烽崗宀勬煕閻斿摜鍙€闁哄瞼鍠栧畷娆撳Χ閸℃浼�
    u32_t F;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤濠€閬嶅焵椤掑倹鍤€閻庢凹鍙冨畷宕囧鐎ｃ劋姹楅梺鍦劋閸ㄥ綊宕愰悙宸富闁靛牆妫楃粭鎺撱亜閿斿灝宓嗙€殿喗鐓￠、鏃堝醇閻旇渹鐢绘繝鐢靛Т閿曘倝宕幍顔句笉缂備焦锕╁▓浠嬫煟閹邦厽缍戦柣蹇旀綑閳规垿顢欓悷棰佸闂傚倷绶氬褔鎮ч崱娑樼疇闁规壆澧楅崑顏堟煃瑜滈崜姘┍婵犲洦鍊锋い蹇撳閸嬫捇寮介‖顒佺⊕閹峰懘鎳栧┑鍥╂创鐎规洜鍠栭、妤佹媴缁嬪灝韦婵犵數濮烽弫鍛婃叏閺夋嚚娲晝閸屾氨鍘遍梺鐟邦嚟婵敻宕伴幇鐗堢厽婵°倐鍋撻柣妤€妫涚划顓㈠箳閹炽劎鎳撻オ浼村焵椤掑嫬纭€闁规儼妫勯拑鐔哥箾閹存瑥鐏柛瀣姍閺岀喐锛愭担鍝勫闂佷紮绲介悥鐓庮潖閻戞ê顕辨繛鍡樺灦閸嬔囨⒑缁嬭法绠查柨姘亜閺囶亞绋荤紒缁樼箓椤繈顢栭埞鍨闁哄被鍔戝顕€鍩€椤掑嫬纾块柟鍓佹櫕瀹撲胶鈧箍鍎遍ˇ浼村煕閹达附鐓曟繝闈涙椤忓瓨淇婇崣澶嬪€愰柡宀嬬到铻栭柍褜鍓欒灋婵炲棙鎸搁悿楣冩煠閸濄儲鏆╂い鈺冨厴閹鏁愭惔鈥茬盎婵炲濮撮妶鎼佸蓟閳╁啫绶炲┑鐘插椤ｅ爼姊洪崨濠冪叆缂佸鎳撻悾鐑芥晲閸垻鏉稿┑鐐村灦閻熴儲绂掗鐐╂斀闁绘劕寮堕ˉ婊呯磼缂佹ê濮嶆い銏℃椤㈡洟鏁傞悾灞藉箞婵犵數鍋涘Λ妤呮嚐椤栨稓顩插Δ锝呭暞閳锋帡鏌涚仦鍓ф噮妞わ讣绠撻弻鐔哄枈閸楃偘鍠婇梺璇″灠閺堫剙顕ラ崟顐ゆ殕闁逞屽墰缁牊绻濋崑顖氱秺閺佹劙宕ㄩ褎顥戦梻浣规偠閸婃牕煤濮椻偓婵＄敻宕熼姘祮濠德板€愰崑鎾趁瑰⿰鍫㈢暫闁诡喗顨堥幉鎾礋椤掑偆妲伴梺姹囧焺閸ㄨ京鏁敓鐘虫櫜闁绘劖绁撮弨浠嬫倵閿濆懐浠涢柡鍛仱閺岋綁鎮╅崗鍛板焻闂佸憡鏌ㄩ惉鑲╁垝椤撱垺鏅柛鏇ㄥ幘閿涙繈姊虹粙鎸庢拱闁煎綊绠栭崺鈧い鎺嶇劍閸婃劗鈧娲橀崝娆撳箖濠婂牊鍤嶉柕澶堝劜閻ｉ亶姊绘担钘変汗閺嬵亪鏌ｈ箛鏂垮摵鐎规洩缍€缁犳稑鈽夊▎鎴濆箰濠电姰鍨煎▔娑㈩敄閸涘瓨鍊垮Δ锝呭暞閻撴洟鏌曢崼婵囶棡缂佲偓鐎ｎ喗鐓涢悘鐐插⒔濞插瓨銇勯姀鈩冪闁轰焦鍔欏畷鍗炩枎濡亶姘節绾版ê澧叉い銊ユ嚇楠炲﹤螣娓氼垱缍庡┑鐐叉▕娴滄粌顔忓┑鍡忔斀闁绘ɑ褰冮弳娆愩亜閿旇娅婃慨濠冩そ楠炴牠鎮欓幓鎺濈€村┑鐘灮閹虫挸螞濠靛绠栨い鏇楀亾妤犵偛妫滈¨渚€鏌涘顒夊剰妞ゎ叀娉曢幑鍕瑹椤栨艾澹嬮梻浣告啞閿曘垺绂嶇捄渚綎婵炲樊浜濋ˉ鍫熺箾閹达綁鍝洪弫鍫ユ⒒娴ｈ鍋犻柛鏂块叄瀵偆鎷犻懠顒佹濡炪倖鐗滈崕鎰板极瀹ュ鐓熼柟閭﹀灠閻ㄦ椽鏌ㄥ☉姘瀻妞ゎ亜鍟存俊鍫曞川椤栨粠鍞舵繝纰樻閸嬪懐绮欓幘瀵哥彾闁哄洨鍠撻々鐑芥倵閿濆骸浜為柛姗€浜堕弻锝堢疀閺囩偘绮舵繝鈷€鍌滅煓妤犵偛锕畷銊╊敊鏉炴壆鐩庨梻浣瑰閺屻劑濡剁粙娆剧€舵い鏇楀亾闁哄本娲熷畷鎯邦槻妞ゅ浚鍓氶妵鍕晜鐠囧弶鐝濋梺鍝勬湰缁嬫捇鍩€椤掑﹦绉甸柛瀣噹閻ｉ浠﹂悙顒€寮挎繝鐢靛Т閸燁垶濡靛┑瀣厵妞ゆ梹鍎抽崢瀵糕偓娈垮枛閻栧ジ鐛幇顓熷劅妞ゆ柨鍚嬮弳蹇涙⒒閸屾艾鈧兘鎳楅崼鏇炵疇闁规崘顕ч崥褰掓煛瀹ュ骸骞栫紒鐙€鍨堕弻銊╂偆閸屾稑顏�
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
#elif  1 //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閹冣挃闁硅櫕鎹囬垾鏃堝礃椤忎礁浜鹃柨婵嗙凹缁ㄥジ鏌熼惂鍝ョМ闁哄矉缍侀、姗€鎮欓幖顓燁棧闂備線娼уΛ娆戞暜閹烘缍栨繝闈涱儐閺呮煡鏌涘☉鍗炲妞ゃ儲鑹鹃埞鎴炲箠闁稿﹥顨嗛幈銊╂倻閽樺锛涢梺缁樺姉閸庛倝宕戠€ｎ喗鐓熸俊顖濆吹濠€浠嬫煃瑜滈崗娑氭濮橆剦鍤曢柟缁㈠枛椤懘鏌嶉埡浣告殲闁绘繃鐗犲缁樼瑹閳ь剟鍩€椤掑倸浠滈柤娲诲灡閺呭爼寮跺▎鍓у數闁荤喐鐟ョ€氼厾绮堢€ｎ喗鐓欐い鏃€鍎抽崢瀛橆殽閻愯鏀荤紒缁樼箞瀹曟帒饪伴崨鍛姂濮婅櫣鎷犻垾铏亐闂佸搫鎳忛惄顖炪€佸▎鎺旂杸婵炴垼椴搁弲鈺呮⒑缂佹ɑ灏Δ鐘崇摃閻忓洨绱撻崒姘偓鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌涘┑鍕姢闁活厽鎹囬弻锝夊閻樺啿鏆堥梺绋款儏椤戝寮婚敓鐘茬倞闁挎繂妫楀▍姘舵⒑闁偛鑻晶鎯р攽閳ヨ櫕鍠樻鐐村灴瀹曠喖顢涘В灏栨櫊閺屻劑寮村Δ鈧禍鍓х磼閸撗嗘闁搞劏娉涢～蹇涙惞閸︻厾锛滃┑鈽嗗灥濞咃絾绂掗幖浣圭參闁告劦鍠曢幉楣冩煛瀹€鈧崰鏍箖濠婂牆绠ｉ柟鐑樻尭閺嬨倖绻濆▓鍨灈闁挎洏鍔岄埢宥夋晲閸パ冪亰濠电偛妫欓崹璺虹暦婢舵劖鐓忓┑鐐戝啳瀚伴柡浣藉亹缁辨捇宕掑顑藉亾妞嬪孩顐介柨鐔哄Т缁愭淇婇妶鍛櫣缂佺姵鐓￠弻娑㈠Ψ閹存繆瀚版繛鍫涘妽缁绘繈鎮介棃娴讹綁鏌ら崷顓炰壕闁稿寒鍋婂缁樻媴缁嬫寧姣愰梺鍦拡閸嬪﹤鐣烽幇鏉垮窛濠电姴瀚峰ú鎼佹⒑缂佹ê濮夐柛搴涘€濋崺娑㈠箣閻樼數锛滈柣搴秵閸嬪嫰顢氬⿰鍕瘈闁逞屽墴楠炲秹顢欓崜褝绱查梺璇插嚱缂嶅棝宕戦崟顖涘€堕悗娑櫱滄禍婊堟煏韫囧﹤澧叉い銉ョ墦閺屸剝鎷呯憴鍕３闂佽桨鐒﹂幑鍥极閹剧粯鏅搁柨鐕傛嫹 婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繘鏌ｉ幋锝嗩棄闁哄绶氶弻娑樷槈濮楀牊鏁鹃梺鍛婄懃缁绘﹢寮婚敐澶婄闁挎繂妫Λ鍕⒑閸濆嫷鍎庣紒鑸靛哺瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鍓х＜闁诡垎鍐ｆ寖缂備緡鍣崹鎶藉箲閵忕姭妲堥柕蹇曞Х椤撳搫鈹戦悙鍙夘棞缂佺粯甯楃粋鎺撱偅閸愨斁鎷虹紓浣割儐椤戞瑩宕曞澶嬬厱濠电姴鍟扮粻鐐碘偓娈垮枛椤嘲顕ｉ幘顔藉亜闁惧繗顕栭崯搴ㄦ煟閻斿摜鐭婄紒澶屾嚀閻ｇ兘顢涢悜鍡樻櫇闂佹寧妫佸Λ鍕焵椤掑啯纭鹃柍瑙勫灴閹晠宕ｆ径濠庢П闂備焦濞婇弨閬嶅垂閸ф钃熼柣鏃囨閻瑩鏌熺粙鍨劉鐎规洖纾槐鎾存媴閸濆嫅锝夋煙缁嬫鐓煎┑锛勬暬瀹曠喖顢涘☉娆愮彆闂佽崵濮村ú鈺冧焊濞嗘劗顩锋い鏍ㄧ矌绾捐棄銆掑顒佹悙闁哄绋掗妵鍕敇閻樻彃骞嬮梺闈涙缁€渚€鍩ユ径鎰潊闁炽儲鏋奸崑鎾绘倻閼恒儱鈧敻鏌ｉ姀銏☆仮闁荤喖鍋婇崵鏇㈡煙閹澘袚闁抽攱甯掗湁闁挎繂鐗婇鐘绘偨椤栨稓鈯曠紒缁樼洴瀹曪絾寰勭仦瑙ｆ嫲闁诲氦顫夊ú姗€宕濆▎蹇曟殾闁绘垹鐡旈弫鍥ㄧ箾閹寸伝鍏肩珶閺囩偐鏀芥い鏃傜摂閻掍粙鏌涚€ｎ偅灏甸柍褜鍓氶悢顒勫箯閿燂拷

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


           