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

#define REALTIME_METER_READ_COUNTER 60     //1m
#define REALTIME_METER_READ_COUNTER_REFLUX 5     //5S anti reflux running
#define METER_OVERTIME  120          //2M


static u8 device_addr=1;
static u8 anti_reflux_en=0;         //=1 anti reflux enables
static u8 anti_refluxing=0;         //=1 anti reflux running
static float anti_threshold=0;      //anti reflux threshold
static u16 counter=REALTIME_METER_READ_COUNTER_REFLUX;

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
static u8 addr_index=0xff;
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
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    //print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //闂傚倸鍊搁崐宄懊归崶顒€违闁逞屽墴閺屾稓鈧綆鍋呭畷宀勬煙椤旇偐绉虹€规洦鍋婂畷鐔碱敆娴ｇǹ澹嶉梻鍌欒兌缁垶骞愰幖浣哥９闁秆勵殔閽冪喖鏌ㄥ┑鍡╂缂傚秵鐗楅妵鍕箻鐠虹洅銉╂煏閸垹鍘存慨濠冩そ濡啫鈽夊▎妯活棧闂備礁鎽滈崰鎰焽濞嗘挻鍋╅柣鎴ｆ闁卞洭鏌￠崶鈺佹瀻闁逞屽墴椤ユ捇鍩€椤掑倹鍤€濠㈢懓锕畷鏉款渻閸撗勬
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倸鍊搁崐鎼佸磹閻戣姤鍊块柨鏇氶檷娴滃綊鏌涢幇鍏哥敖闁活厽鎹囬弻锝夊閳惰泛缍婇幏鎴︽偄鐏忎焦鏂€闂佺粯锚瀵埖寰勯崟顖涚厱閻庯絻鍔屾慨鍌炴煙椤旂瓔娈滅€规洜鍏橀、姗€鎮滈崱蹇撲壕鐎光偓閸曨剛鍘遍梺闈涚墕濞层倖绂掗柆宥嗙厵妞ゆ梻鐡斿▓鏃堟煃閽樺妲搁柍璇茬Ч閹煎綊顢曢姀顫礉闁诲孩顔栭崳顕€宕滃璺虹闁告侗鍨遍崰鍡涙煕閺囥劌骞橀懖鏍⒒閸屾瑦绁版い顐㈩樀瀹曟洟骞庨挊澶屾焾闂佸壊鐓堥崳顕€宕甸弴銏＄厵闁诡垱婢樿闂佺粯鎸婚悷褏妲愰幒鏂哄亾閿濆骸浜滄い鏇熺矒閺岋綁顢橀悤浣圭杹闂佸搫鐬奸崰鏍偂椤掑嫬绠甸柟鍝勬娴滅偓绻濋棃娑卞剰缂佺姵宀搁弻娑㈠箛闂堟稒鐏堢紓浣插亾閻庯綆鍋佹禍婊堟煙閹佃櫕娅呴柣蹇ｄ邯閺岋繝宕遍幇顒€濮﹂梺鍝勭焿缁查箖骞嗛弮鍫濐潊闁绘ê寮堕悘鍡涙⒒娴ｇ儤鍤€闁搞倖鐗犻獮蹇涙晸閿燂拷
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    //dev->cfg = null;  // 闂傚倸鍊搁崐鎼佸磹閻戣姤鍊块柨鏇楀亾妞ゎ厼鐏濊灒闁兼祴鏅濋悡瀣⒑閸撴彃浜濇繛鍙夛耿瀹曟垿顢旈崼鐔哄幈闂佹枼鏅涢崰姘舵倿娴犲鐓冪紓浣股戦ˉ鍫ユ煛鐏炵晫啸妞ぱ傜窔閺屾盯骞樼€涙娈ょ紓渚囧枟閻燂箑顕ラ崟顖氱疀闁割煈鍋呭▍鍡涙⒒娴ｅ憡鍟炲〒姘殜瀹曘垽骞栨担鍝ヮ唶闂佸憡鍔﹂崰妤呮偂閻斿吋鐓熼柟閭﹀墻閸ょ喖鏌嶉挊澶樻█闁哄矉缍侀、妯衡攽閸垻宕叉俊鐐€戦崹娲嚌妤ｅ喛缍栨繝闈涚墛瀹曞鏌ц箛姘煎殐闁哥偛鐖煎缁樻媴鐟欏嫬浠╅梺绋块椤嘲顫忔禒瀣妞ゆ帊绀佸鍧楁⒑闂堟稓绠為柛濠冪墵閹繝鎮㈤崗鑲╁帾婵犵數鍋涢悘婵嬪礉濮橆厹浜滈煫鍥风到婢ь垶鏌曢崶褍顏€殿噮鍣ｉ崺鈧い鎺嗗亾閻撱倖淇婇娆掝劅闁搞倖顨婇弻娑㈠即閵娿儳浠梺鍝勬噺閹倿寮婚妸鈺傚亞闁稿本绋戦锟�
    if (ModbusDevice.cfg == null) {
          dev->cfg = (ST_UARTDCB *)ModbusDevice.head->hardCfg;
        } else {
          dev->cfg = ModbusDevice.cfg;
        }
    
    
    dev->callBack = anti_ack;  // 闂傚倸鍊峰ù鍥х暦閸偅鍙忕€规洖娲ㄩ惌鍡椕归敐鍫綈婵炲懐濮撮湁闁绘ê妯婇崕鎰版煕鐎ｅ吀閭柡灞剧洴閸╁嫰宕橀浣割潓闂備胶绮敮鎺椻€﹂悜鑺ュ亗妞ゆ劧绠戦悙濠勬喐鎼淬劌绠查柨婵嗩槹閻撶喖鏌熼柇锕€澧鹃棅顒夊墴閺岀喖顢欏▎鐐秷闂佺懓鍢查幊姗€骞冮悜钘夌骇闁割煈鍟╅幋鐐扮箚闁绘劦浜滈埀顑應鍋撻棃娑氱劯鐎规洘绮撻幃銏ゆ嚃閻戞鈯曠紒妤冨枛閸┾偓妞ゆ帒瀚弰銉╂煃瑜滈崜姘跺Φ閸曨垰绠抽柟瀛樼妇閸嬫捁銇愰幒鎴犳焾闂佸憡绻傛繛濠呫亹閹烘挻娅滈梺鎼炲劀閸愵煈鐎遍梻鍌欐缁鳖喚绱炴担鍦洸閻犺桨璀﹂崵鏇㈡煕椤愶絾绀€闁绘挻鐩弻娑氫沪閸撗佲偓鎺楁煕濞嗗繑顥滈柍瑙勫灴閹瑩骞撻幒鏃堢崜闂備胶绮〃鍫熸叏閹绢喗鍋╃€瑰嫭澹嬮弸搴ㄦ煙閻愵剚缍戞繛鍫㈠枛濮婃椽妫冨☉杈€嗛梻鍌氬鐎氫即銆佸▎鎾存櫢闁跨噦鎷�
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂傚倸鍊峰ù鍥х暦閸偅鍙忛柟鎯板Г閸婂潡鏌ㄩ弴鐐测偓鍝ュ閸ф鐓欓悹鍝勬惈椤ョ偤鏌涢妷顔煎⒒闁轰礁妫濋弻娑㈠即閵娿儰绨藉銈嗘⒐閸旀瑥顫忓ú顏勫窛濠电姴瀚悾鐢告煟鎼淬垹鍤柛锝忕到椤曪綁顢曢敃鈧儫闂佸啿鎼崐濠氬储閻㈢數纾藉ù锝呭閸庢劙鎮楃粭娑樻处閸庢淇婇妶鍛櫤闁绘挻鐟╁Λ鍛搭敆娴ｅ摜绁烽梺閫炲苯澧紒璇茬墦閻涱喗绻濋崶銊モ偓鐑芥煕濠靛棗顏柨娑欑矊閳规垿鎮╃拠褍浼愰梺鍝ュ暱閺呯姴鐣烽悩缁樻櫢闁跨噦鎷�
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    Device_inset(dev);    // insert to head,realtime send
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倸鍊搁崐鎼佸磹閻戣姤鍊块柨鏇氶檷娴滃綊鏌涢幇鍏哥敖闁活厽鎹囬弻锝夊閳惰泛缍婇幏鎴︽偄鐏忎焦鏂€闂佺粯锚瀵埖寰勯崟顖涚厱閻庯絻鍔屾慨鍌炴煙椤旂瓔娈滅€规洜鍏橀、姗€鎮滈崱蹇撲壕鐎光偓閸曨剛鍘遍梺闈涚墕濞层倖绂掗柆宥嗙厵妞ゆ梻鐡斿▓鏃堟煃閽樺妲搁柍璇茬Ч閹煎綊顢曢姀顫礉闁诲孩顔栭崳顕€宕滃璺虹闁告侗鍨遍崰鍡涙煕閺囥劌骞橀懖鏍⒒閸屾瑦绁版い顐㈩樀瀹曟洟骞庨挊澶屾焾闂佸壊鐓堥崳顕€宕甸弴銏＄厵闁诡垱婢樿闂佺粯鎸婚悷褏妲愰幒鏂哄亾閿濆骸浜滄い鏇熺矒閺岋綁顢橀悤浣圭杹闂佸搫鐬奸崰鏍偂椤掑嫬绠甸柟鍝勬娴滅偓绻濋棃娑卞剰缂佺姵宀搁弻娑㈠箛闂堟稒鐏堢紓浣插亾閻庯綆鍋佹禍婊堟煙閹佃櫕娅呴柣蹇ｄ邯閺岋繝宕遍幇顒€濮﹂梺鍝勭焿缁查箖骞嗛弮鍫濐潊闁绘ê寮堕悘鍡涙⒒娴ｇ儤鍤€闁搞倖鐗犻獮蹇涙晸閿燂拷
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = (ST_UARTDCB *)&UART_9600_N1;  
       
    dev->callBack = anti_ack;  // 闂傚倸鍊峰ù鍥х暦閸偅鍙忕€规洖娲ㄩ惌鍡椕归敐鍫綈婵炲懐濮撮湁闁绘ê妯婇崕鎰版煕鐎ｅ吀閭柡灞剧洴閸╁嫰宕橀浣割潓闂備胶绮敮鎺椻€﹂悜鑺ュ亗妞ゆ劧绠戦悙濠勬喐鎼淬劌绠查柨婵嗩槹閻撶喖鏌熼柇锕€澧鹃棅顒夊墴閺岀喖顢欏▎鐐秷闂佺懓鍢查幊姗€骞冮悜钘夌骇闁割煈鍟╅幋鐐扮箚闁绘劦浜滈埀顑應鍋撻棃娑氱劯鐎规洘绮撻幃銏ゆ嚃閻戞鈯曠紒妤冨枛閸┾偓妞ゆ帒瀚弰銉╂煃瑜滈崜姘跺Φ閸曨垰绠抽柟瀛樼妇閸嬫捁銇愰幒鎴犳焾闂佸憡绻傛繛濠呫亹閹烘挻娅滈梺鎼炲劀閸愵煈鐎遍梻鍌欐缁鳖喚绱炴担鍦洸閻犺桨璀﹂崵鏇㈡煕椤愶絾绀€闁绘挻鐩弻娑氫沪閸撗佲偓鎺楁煕濞嗗繑顥滈柍瑙勫灴閹瑩骞撻幒鏃堢崜闂備胶绮〃鍫熸叏閹绢喗鍋╃€瑰嫭澹嬮弸搴ㄦ煙閻愵剚缍戞繛鍫㈠枛濮婃椽妫冨☉杈€嗛梻鍌氬鐎氫即銆佸▎鎾存櫢闁跨噦鎷�
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂傚倸鍊峰ù鍥х暦閸偅鍙忛柟鎯板Г閸婂潡鏌ㄩ弴鐐测偓鍝ュ閸ф鐓欓悹鍝勬惈椤ョ偤鏌涢妷顔煎⒒闁轰礁妫濋弻娑㈠即閵娿儰绨藉銈嗘⒐閸旀瑥顫忓ú顏勫窛濠电姴瀚悾鐢告煟鎼淬垹鍤柛锝忕到椤曪綁顢曢敃鈧儫闂佸啿鎼崐濠氬储閻㈢數纾藉ù锝呭閸庢劙鎮楃粭娑樻处閸庢淇婇妶鍛櫤闁绘挻鐟╁Λ鍛搭敆娴ｅ摜绁烽梺閫炲苯澧紒璇茬墦閻涱喗绻濋崶銊モ偓鐑芥煕濠靛棗顏柨娑欑矊閳规垿鎮╃拠褍浼愰梺鍝ュ暱閺呯姴鐣烽悩缁樻櫢闁跨噦鎷�
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    
    //anti reflux running 
    if(anti_refluxing){
        Device_inset(dev);    // insert to head,realtime send
    }
    else{
        Device_add(dev);        //add to tail
    }

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
* introduce:   闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁惧墽鎳撻—鍐偓锝庝簻椤掋垺銇勯幇顏嗙煓闁哄被鍔戦幃銏ゅ传閸曟垯鍨婚惀顏堝箚瑜滈悡濂告煛鐏炲墽娲寸€殿喗鎸虫俊鎼佸Ψ閵忕姳澹曢梺缁樕戠粊鏉懳ｉ崼鐔剁箚妞ゆ牗绋撻惌瀣繆椤愩垹鏆欓柣锝囧厴閹垽鎮℃惔锝呭箰闂備線鈧偛鑻晶顕€寮崼鐔剁箚闁靛牆瀚崗宀€绱掗埦鈧崑鎾绘⒒娓氣偓濞佳勵殽韫囨洖绶ら柛鎾楀嫬鍘归梺缁樺姦閸忔瑦绂嶅⿰鍫熺厵閻庢稒顭囩粻鏍ㄣ亜閵壯冧户缂佽鲸甯″畷鎺戭潩濮ｆ鍐ｆ斀闂勫洭宕洪弽褜鍤楅柛鏇ㄥ幐閸嬫捇鏁愭惔鈥愁潻濡ょ姷鍋為崝娆忣潖缂佹ɑ濯撮柛娑㈡涧缂嶅﹤顕ｉ悽鍓叉晢闁稿本绮庨悡鎾绘⒑绾懏褰ч悗闈涚焸瀵偆鈧綆鍋嗙弧鈧繝鐢靛Т閸婂綊宕抽悾宀€妫柟顖嗕礁浠梺鍝勮嫰缁夌兘篓娓氣偓閺屾盯骞樼捄鐑樼亪閻庤娲樺浠嬪春閳ь剚銇勯幒宥夋濞存粍绮撻弻鐔兼倻濡櫣浠村銈呮禋娴滎亪寮诲澶嬬叆閻庯綆浜炴导宀勬倵濞堝灝鏋涢柣鏍с偢楠炲啫鈻庨幘宕囬獓闂佽鎯岄崢鎼佸磻閹炬枼鍫柛顐ゅ枔閸橀亶妫呴銏″闁归€涜兌閳ь剚姘ㄦ晶妤呭Φ閸曨垼鏁囬柍銉ュ暱婵嘲顪冮妶蹇曠暢婵炲懏娲熸俊鐢稿礋椤栨鈺呮煏婢舵稓鐣辨繛鍫燁殜濮婅櫣鎷犻垾铏彎缂備胶濮甸悧鏇㈡偩閻ゎ垬浜归柟鐑樼箖閺呮繈姊洪幐搴ｇ畵闁瑰嘲顑夐崺鈧い鎺嶇劍瀹曞本鎱ㄦ繝鍐┿仢闁圭绻濇俊鍫曞川椤旈敮鍋撻悙顒傜瘈闁靛繈鍨洪崵鈧┑鈽嗗亝缁嬫捇宕氶幒妤佹櫜濠㈣泛顑呴埀顒傚厴閺岋綁骞嬮悙鍐╁哺楠炲繐煤椤忓應鎷洪梺鍛婄☉閿曪箓鍩ユ径鎰叆闁哄洦锚閸旀粎鈧灚婢樼€氭澘鐣烽悢纰辨晬婵炴垶鑹鹃獮鍫熺節绾版ɑ顫婇柛銊︽緲閿曘垽鏌嗗鍡楀壒濠电偛妫欓崹鐔煎磻閹捐埖鍠嗛柛鏇ㄥ墰椤︺劑姊洪崨濠冣拹闁荤啿鏅涢锝夘敃閿曗偓缁犺崵绱撴担鑲℃垵鈻嶉姀銈嗏拺閻犳亽鍔屽▍鎰版煙閸戙倖瀚�     
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
        device_addr=online_dev_addr_tab[addr_index];    //闂傚倷娴囬褍霉閻戣棄鏋侀柛娑橈攻濞呯姵淇婇妶鍛櫡闁逞屽墮閸熸潙鐣烽妸鈺佺骇闁瑰鍋炶ⅲ闂佽楠搁崢婊堝磻閹剧粯鐓冪憸婊堝礈閻旇偐宓佸鑸靛姈閸嬪鏌涢銈呮瀻妞ゅ孩鎸剧槐鎾存媴閸撴彃鍓靛┑鐐差槹濞叉牠鎮惧┑鍡忔闁靛繆妾ч幏缁樼箾閹剧澹樻繛灞傚€栫粋宥呂旈崨顔惧幘闁诲骸婀辨慨鐢垫兜妤ｅ啯鐓冪紓浣股戝畷灞绢殽閻愬樊鍎旈柡浣稿€块幃鍓т沪閽樺－銉╂⒒閸屾瑨鍏岀痪顓炵埣瀹曟粌鈹戠€ｃ劉鍋撻崘顔煎窛妞ゆ牗绮庨ˇ顖涚箾閹炬潙鐒归柛瀣崌閺岋紕浠﹂崜褎鍒涢梺杞扮劍閸旀瑥鐣峰鈧俊鎼佸Ψ椤旈敮鍋撻搹顐ょ瘈闁汇垽娼цⅴ闂佺ǹ顑嗛幑鍥蓟閻旇櫣纾奸柕蹇曞У閻忓牓姊绘担鏂ら獜闁稿﹥绻堝璇差吋閸偅顎夐梻浣烘嚀閸ゆ牠骞忛敓锟�
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
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ帒顦伴弲顏堟偡濠婂啰效婵犫偓娓氣偓濮婅櫣绱掑Ο铏逛紘濠碘槅鍋勭€氭澘顕ｉ崨濠勭懝闁逞屽墴瀵鈽夊Ο閿嬵潔濠殿喗顨呴悧濠囧极閹€鏀介柣鎰级閸ｈ棄鈹戦鑲╀粵缂佸矁椴哥换婵嬪炊椤儸鍥ㄧ厱婵炴垵宕弸娑欑箾閸滃啰绡€婵﹥妞介弻鍛存倷閼艰泛顏繝鈷€灞芥珝闁哄本鐩幃銏ゅ川婵犲嫮鈻忛梻浣风串缁插潡宕楀Ο璁崇箚闁归棿绀侀悡娑樏归敐鍡樸仢闁绘稒鎹囧缁樻媴閻戞ê娈屽銈嗘处閸欏啫鐣烽幋锔藉€烽柛銊︾☉瑜版岸姊婚崒姘偓宄懊归崶顒夋晪鐟滃酣銆冮妷鈺佺濞达絿枪閸嬪秴鈹戦悩璇у伐闁绘妫涙竟鏇熺節濮橆厾鍘甸梺鍛婃寙閸涱厾顐奸梻浣虹帛閹歌煤閻旂厧钃熼柨鐔哄Т閻愬﹪鏌曟径鍫濆姎妞ゎ剙鐗婄换婵嬪煕閳ь剛浠﹂懞銉т邯闂傚倸娲らˇ鐢稿蓟閿濆拋娼ㄩ柍褜鍓欓…鍥樄鐎规洘绻勯埀顒婄秵閸撴稓澹曢懖鈺冪＝濞达綀顕栭悞浠嬫煃椤栨稒绀冪紒缁樼洴閹剝鎯旈敐鍥跺晪闁诲氦顫夊ú妯煎垝韫囨蛋鍥蓟閵夛妇鍘遍梺瀹犳〃閼冲爼鎮為崨濠冨弿濠电姴鍟妵婵堚偓瑙勬处閸嬪﹥淇婇悜钘壩ㄩ柨鏃堟暜閸嬫捇顢橀姀鈾€鎷洪梺鍛婄☉閿曘儵鍩涢幇鐗堢厱闁靛ǹ鍎虫禒銏ゆ煟閿濆懎妲婚悡銈嗐亜韫囨挸顏柛鏃撶畱椤啴濡堕崱妤冪憪闂傚倸瀚粔鐢电矉瀹ュ應鍫柛顐ゅ枔閸樼敻姊洪崨濠勭畵閻庢凹鍙冭棢婵犲﹤鍘鹃悷鎵冲牚闁割偁鍨婚弳顐⑩攽椤旂》鏀绘俊鐐舵閻ｇ兘顢曢敃鈧敮闂侀潧顧€婵″洭鍩€椤掍礁鍔ら柍瑙勫灴閹瑩骞撻幒鎾斥偓顖炴⒑閼姐倕鏆遍柡鍛█婵″瓨鎷呴崜鍙夊兊闂佽偐枪閻忔艾鈻嶅⿰鍕瘈闁靛骏绲介悡鎰版嫅闁秵鐓涢柛娑卞枤缁犵偞鎱ㄦ繝鍐┿仢妤犵偞鍔栭幆鏃堝閳哄倵鎸呭┑鐘殿暯濡插懘宕戦崟顓涘亾濮樼厧骞樼紒顔碱儏椤撳吋寰勭€ｎ剙濮搁柣搴＄畭閸庡崬螞瀹€鍕婵炲樊浜濋埛鎴︽煕濞戞﹫鏀婚悘蹇斿閹叉悂寮堕崹顔芥缂備礁鍊哥粔鎾煘閹达箑閱囬柕蹇ｆ緛缁辨煡姊洪懡銈呅㈡繛璇х畳閵囨劙宕橀鍛櫆濡炪倖鎸鹃崑鎰板绩娴犲鐓冮柕澶堝劤閿涘秹鏌涢埡瀣М闁哄备鍓濋幏鍛喆閸曨偆锛撻梻浣芥〃閻掞箓骞戦崶顒€鏋侀柟鍓х帛閺呮悂鏌ㄩ悤鍌涘
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
        anti_refluxing=0;       //turn off anti refulx running
        APP_DEBUG("no over anti reflux threshold\r\n")
        return;
    }

    //turn on anti reflux running
    if(anti_refluxing==0){
        anti_refluxing=1;
        counter=REALTIME_METER_READ_COUNTER_REFLUX;
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
                //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳婀遍埀顒傛嚀鐎氼參宕崇壕瀣ㄤ汗闁圭儤鍨归崐鐐差渻閵堝棗鍧婇柛瀣尰濞艰鈹戠€ｎ偀鎷洪梻渚囧亞閸嬫盯鎳熼娑欐珷闁圭虎鍠楅悡娑㈡倶閻愭彃鈷旈柕鍡樺浮閺屽秷顧侀柛鎾卞妿缁辩偤宕卞☉妯硷紱闂佸憡渚楅崢楣冨汲閿旈敮鍋撻崗澶婁壕闂佸憡娲﹂崜娑㈠储闂堟侗娓婚柕鍫濇椤ュ棝鏌涚€ｎ偄濮夌紒顔芥閹煎綊宕楁径濠佸闁荤喐鐟ョ€氼厽寰勯崟顖涚厱閻庯綆浜峰銉╂煟閿濆洤鍘撮柟顔哄灮閸犲﹥娼忛妸锔界彍婵犵數鍋犻幓顏嗗緤閹稿海浠氬┑鐘灮閹虫挾绮旈悷閭︽綎濠电姵鑹剧壕鍏肩箾閸℃ê绗掗柛姗堢磿缁辨挻绗熼崶褎鐝梺鎼炲妼閻栧ジ鎮伴鈧獮鍥级鐠侯煈鍟囧┑鐐舵彧缁蹭粙骞楀⿰鍫熸櫖婵炲棙鍔楃弧鈧梺姹囧灲濞佳勭墡闂備胶鍘х紞濠勭不閺嶎厼绠栭柣鎰惈閸ㄥ倹銇勯幇顔夹＄紒銊ヮ煼濮婃椽鎮烽悧鍫熷創缂備礁顦晶搴ㄥ箯鐟欏嫬顕遍悗娑欘焽閸橀亶姊虹紒妯活棃妞ゃ儲鎸剧划缁樼節濮橆厾鍘搁柣蹇曞仜婢ц棄煤閹绢喗鐓欐い鏃囶潐濞呭懘鏌ｉ敐鍥у幋鐎规洘鍎奸ˇ顕€鏌熼摎鍌氬祮闁哄矉绲鹃幆鏃堝Χ鎼淬垻绉锋繝鐢靛仜瀵爼鏁冮姀鐘垫殾婵犻潧妫岄弸搴ㄦ煙閹规劖纭剧紒渚婄畵濮婃椽宕崟顓夌娀鏌涢弬璺ㄐｉ柛鎺戯躬楠炴﹢顢欓悾灞藉箺闂備胶绮濠氬煕閸儱姹查柟鐑橆殕閻撴洟鏌熺€涙鐭岄柣顓熺懄閹便劍绻濋崟顓炵闂佺懓鍢查幊鎰垝濞嗗繆鏋庨柣鎰靛墻濡繘姊婚崒娆掑厡閺嬵亝銇勯幋婵囧殗妤犵偛锕ラ幆鏃堟晲閸涱厾鏌ч梻鍌氬€风粈渚€骞栭銈囩煓濞撴埃鍋撶€规洘鍨垮畷鍗炍熺亸鏍ㄧ亙闁诲骸绠嶉崕閬嶅箖閼愁垬浜归柟鐑樻尭娴滄鏌熼懝鐗堝涧缂佹煡绠栭獮澶嬬附閸涘ň鎷虹紓鍌欑劍钃遍悘蹇曞缁绘盯鎳犻鈧弸娑氣偓娈垮枛椤嘲顕ｉ幘顔藉亜濡炲娴烽悰顔界節绾版ɑ顫婇柛銊╂涧閳诲秹鏁愰崱娆樻祫缂備礁顑呯花閬嶅绩娴犲鐓ユ繛鎴灻顒佷繆閻愵剚鍊愰柡灞剧洴閹晛鐣烽崶褉鎷伴梻浣虹《閺呮繈宕戦妶澶屽祦閻庯綆鍠楅弲婊呯磽娴ｈ偂鎴濃枍閺冨牊鈷掑ù锝堟閵嗗﹪鏌涢幘瀵哥疄闁轰礁顑呴—鍐Χ閸涘宕梺鐟板殩閹凤拷
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ帒顦伴弲顏堟偡濠婂啰效婵犫偓娓氣偓濮婅櫣绱掑Ο铏逛紘濠碘槅鍋勭€氭澘顕ｉ崨濠勭懝闁逞屽墴瀵鈽夊Ο閿嬵潔濠殿喗顨呴悧濠囧极閹€鏀介柣鎰级閸ｈ棄鈹戦鑲╀粵缂佸矁椴哥换婵嬪炊椤儸鍥ㄧ厱婵炴垵宕弸娑欑箾閸滃啰绡€婵﹥妞介弻鍛存倷閼艰泛顏繝鈷€灞芥珝闁哄本鐩幃銏ゅ川婵犲嫮鈻忛梻浣风串缁插潡宕楀Ο璁崇箚闁归棿绀侀悡娑樏归敐鍡樸仢闁绘稒鎹囧缁樻媴閻戞ê娈屽銈嗘处閸欏啫鐣烽幋锔藉€烽柛銊︾☉瑜版岸姊婚崒姘偓宄懊归崶顒夋晪鐟滃酣銆冮妷鈺佺濞达絿枪閸嬪秴鈹戦悩璇у伐闁绘妫涙竟鏇熺節濮橆厾鍘甸梺鍛婃寙閸涱厾顐奸梻浣虹帛閹歌煤閻旂厧钃熼柨鐔哄Т閻愬﹪鏌曟径鍫濆姎妞ゎ剙鐗婄换婵嬪煕閳ь剛浠﹂懞銉т邯闂傚倸娲らˇ鐢稿蓟閿濆拋娼ㄩ柍褜鍓欓…鍥樄鐎规洘绻勯埀顒婄秵閸撴稓澹曢懖鈺冪＝濞达綀顕栭悞浠嬫煃椤栨稒绀冪紒缁樼洴閹剝鎯旈敐鍥跺晪闁诲氦顫夊ú妯煎垝韫囨蛋鍥蓟閵夛妇鍘遍梺瀹犳〃閼冲爼鎮為崨濠冨弿濠电姴鍟妵婵堚偓瑙勬处閸嬪﹥淇婇悜钘壩ㄩ柨鏃堟暜閸嬫捇顢橀姀鈾€鎷洪梺鍛婄☉閿曘儵鍩涢幇鐗堢厱闁靛ǹ鍎虫禒銏ゆ煟閿濆懎妲婚悡銈嗐亜韫囨挸顏柛鏃撶畱椤啴濡堕崱妤冪憪闂傚倸瀚粔鐢电矉瀹ュ應鍫柛顐ゅ枔閸樼敻姊洪崨濠勭畵閻庢凹鍙冭棢婵犲﹤鍘鹃悷鎵冲牚闁割偁鍨婚弳顐⑩攽椤旂》鏀绘俊鐐舵閻ｇ兘顢曢敃鈧敮闂侀潧顧€婵″洭鍩€椤掍礁鍔ら柍瑙勫灴閹瑩骞撻幒鎾斥偓顖炴⒑閼姐倕鏆遍柡鍛█婵″瓨鎷呴崜鍙夊兊闂佽偐枪閻忔艾鈻嶅⿰鍕瘈闁靛骏绲介悡鎰版嫅闁秵鐓涢柛娑卞枤缁犵偞鎱ㄦ繝鍐┿仢妤犵偞鍔栭幆鏃堝閳哄倵鎸呭┑鐘殿暯濡插懘宕戦崟顓涘亾濮樼厧骞樼紒顔碱儏椤撳吋寰勭€ｎ剙濮搁柣搴＄畭閸庡崬螞瀹€鍕婵炲樊浜濋埛鎴︽煕濞戞﹫鏀婚悘蹇斿閹叉悂寮堕崹顔芥缂備礁鍊哥粔鎾煘閹达箑閱囬柕蹇ｆ緛缁辨煡姊洪懡銈呅㈡繛璇х畳閵囨劙宕橀鍛櫆濡炪倖鎸鹃崑鎰板绩娴犲鐓冮柕澶堝劤閿涘秹鏌涢埡瀣М闁哄备鍓濋幏鍛喆閸曨偆锛撻梻浣芥〃閻掞箓骞戦崶顒€鏋侀柟鍓х帛閺呮悂鏌ㄩ悤鍌涘
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
                        //婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繘鏌ｉ幋锝嗩棄闁哄绶氶弻娑樷槈濮楀牊鏁鹃梺鍛婄懃缁绘劙婀侀梺绋跨箰閸氬绱為幋锔界厱闁靛ǹ鍎遍埀顒€娼″濠氭晲婢跺﹦顔掗悗瑙勬礀濞层倝宕ú顏呪拺闁告繂瀚烽崕鎰版煟濡や緡娈橀柟骞垮灩閳藉濮€閻樻鍚呴梻浣虹帛閸旀浜稿▎鎾崇濞寸厧鐡ㄩ埛鎴犵磼鐎ｎ偒鍎ラ柛搴㈠姍閺岀喖宕ㄦ繝鍐ㄢ偓鎰版煥濠靛牆浠滈柍瑙勫灩閳ь剨缍嗛崑鍕濡ゅ懏鐓欓柛蹇氬亹閺嗘﹢鏌涢妸锔筋潡闁靛洦鍔栭幆鏃堬綖椤撶姷鐣鹃梻渚€娼ч悧鍡椕洪妶鍛瘎闂傚倷鑳舵灙妞ゆ垵妫濋幆鍕敍濮樿鲸娈惧銈嗙墱閸庢劙寮鍡欑瘈濠电姴鍊搁鈺呭箹閺夋埊宸ラ柍瑙勫灴閹晝绱掑Ο濠氭暘婵＄偑鍊栭崹鐢稿箠濮椻偓楠炲啫鈻庨幇顔剧槇闂佹悶鍎崝搴ㄥ储椤忓懐绡€闁汇垽娼ч埀顒夊灦瀹曟﹢濡搁敂绛嬪敳闂備礁鎽滈崰鎰板箖閸屾凹娼栧┑鐘宠壘绾惧吋鎱ㄥ鍡楀幋闁稿鎹囬幃婊堟嚍閵夈儲鐣遍梻浣告啞閹哥兘鎮為敃鍌氱婵犲﹤鐗婇悡鏇熴亜閹板墎绋荤紒鈧埀顒傜磽娴ｆ彃浜鹃梺鍓插亞閸犳挾寮ч埀顒勬⒒閸屾氨澧涘〒姘殜瀹曟洖顓兼径瀣帗閻熸粍绮撳畷婊冣枎閹惧磭鏌堥梺鍝勵槹閸ㄥ綊寮抽敃鍌涚厪濠电偟鍋撳▍鎾绘煛娴ｅ摜校缂佺粯绻冪换婵嬪磼濞戞﹩鈧稑鈹戦檱鐏忔瑦鐏欓梺瀹狀潐閸ㄥ潡骞冨▎鎴斿亾濞戞顏堟瀹ュ鈷戠紒顖涙礃濞呭懘鏌涢悢鍛婄稇闁伙絿鍏橀獮瀣晝閳ь剛绮婚懡銈囩＝濞达綀顕栭悞浠嬫煕濡粯鍊愰柡宀嬬秬缁犳盯寮撮悙鎵崟濠电姭鎷冮崱姗嗘闁绘挶鍊濋弻鐔虹磼閵忕姵鐏嶉梺缁樻尰缁嬫垿婀侀梺鎸庣箓濞诧箓宕甸埀顒佺節濞堝灝鏋涚紒澶婄埣閸┾偓妞ゆ帒鍠氬鎰箾閸欏鑰块柡浣稿暣婵偓闁炽儴灏欑粻姘舵⒑瑜版帗锛熺紒鈧担鍛婃殰濠碉紕鍋戦崐鏍涢崘顔兼瀬妞ゆ洍鍋撶€规洘鍨块獮妯兼嫚閼碱剦鍟嬫俊鐐€栧Λ浣筋暰闂佺懓鍢查幖顐﹀煘閹达富鏁婇柣鎰靛墮濞堝苯鈹戦悙鑼勾闁稿﹥绻堥獮鍐倷鐟佷焦妞介、鏃堝川椤栨艾绠為梻鍌欑窔濞佳嗗闂佸搫鎳忛悷鈺呯嵁婢舵劖鍊锋い鎺戭槹椤旀棃姊虹紒妯哄婵炲吋鐟﹂幈銊ヮ吋閸ワ絽浜鹃悷娆忓缁€鍐煥濮橆厹浜滈柡鍌濇硶缁犵粯銇勯姀锛勬噰鐎规洘顨堥崰濠囧础閻愭潙浜奸梻鍌氬€搁崐鎼佸磹妞嬪孩顐芥慨姗嗗墻閻掔晫鎲搁幋鐐存珷婵犻潧顑嗛埛鎴︽煕濠靛棗顏柛锝堟缁辨帡顢欓懞銉ョ闂佷紮绲块崗姗€骞冮悾灞芥瀳婵☆垵妗ㄦ竟鏇㈡⒑閹稿海绠撳Δ鐘虫倐瀵悂鎮㈤崗鑲╁幗闂佽鍎抽悺銊х矆閸愵喗鐓忛柛銉戝喚浼冨銈冨灪閿曘垽骞冮姀鈽嗘Ч閹煎瓨绻冨Ο濠冪節閻㈤潧浠滈柣掳鍔庨崚鎺戭吋閸ャ劌搴婇梺鍛婂姦閸犳牗顢婇梻浣告啞濞诧箓宕归弶璇炬稑顫滈埀顒勫箖瑜版帒绠掗柟鐑樺灥椤ュ酣鎮峰⿰鍐弰妤犵偛鐗撴俊鎼佹晜閸撗呮闂備礁鎲￠崝蹇涘棘閸屾稓顩烽柕蹇嬪€栭崐鐢告偡濞嗗繐顏紒鈧崼銉︾厱闊洦妫戦懓鍧楁煏閸℃洜顦︽い顐ｇ矒閸┾偓妞ゆ帒瀚粻鏍煏閸繃顥炲┑顖涙尦濮婃椽宕归鍛壈闂佺粯鎸诲ú鐔奉潖婵犳艾纾兼慨姗嗗厴閸嬫捇鎮滈懞銉ユ畱闂佸壊鍋呭ú鏍不閻樼粯鐓欓柟瑙勫姦閸ゆ瑩鏌涘顒傜Ш闁哄备鈧剚鍚嬮幖绮光偓宕囶啇缂傚倷鑳舵刊顓㈠垂閸洖钃熼柕濞炬櫆閸嬪棝鏌涚仦鍓р槈妞ゅ骏鎷�
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ帒顦伴弲顏堟偡濠婂啰效婵犫偓娓氣偓濮婅櫣绱掑Ο铏逛紘濠碘槅鍋勭€氭澘顕ｉ崨濠勭懝闁逞屽墴瀵鈽夊Ο閿嬵潔濠殿喗顨呴悧濠囧极閹€鏀介柣鎰级閸ｈ棄鈹戦鑲╀粵缂佸矁椴哥换婵嬪炊椤儸鍥ㄧ厱婵炴垵宕弸娑欑箾閸滃啰绡€婵﹥妞介弻鍛存倷閼艰泛顏繝鈷€灞芥珝闁哄本鐩幃銏ゅ川婵犲嫮鈻忛梻浣风串缁插潡宕楀Ο璁崇箚闁归棿绀侀悡娑樏归敐鍡樸仢闁绘稒鎹囧缁樻媴閻戞ê娈屽銈嗘处閸欏啫鐣烽幋锔藉€烽柛銊︾☉瑜版岸姊婚崒姘偓宄懊归崶顒夋晪鐟滃酣銆冮妷鈺佺濞达絿枪閸嬪秴鈹戦悩璇у伐闁绘妫涙竟鏇熺節濮橆厾鍘甸梺鍛婃寙閸涱厾顐奸梻浣虹帛閹告悂宕导鏉戠疅缂佸绨遍崼顏堟煕椤愶絿绠樻い锔诲弮濮婅櫣绱掑Ο娲绘⒖闂佺ǹ顑嗛崝鏇㈡偩閸偆鐟归柍褜鍓熼幃锟狀敃閿曗偓閻愬﹪鏌曟繝蹇擃洭闁挎稒娲熷铏圭矙濞嗘儳鍓遍梺鍦嚀濞差厼顕ｉ锕€绠涙い鎾跺枎閸斿懘姊洪弬銉︽珔闁哥喓澧楃粋宥夘敂閸℃瑧锛濋梺绋挎湰閻熴劑宕楃仦瑙ｆ斀妞ゆ洍鍋撴繛浣冲浄缍栨繝闈涱儏鎯熼梺瀹犳〃閼冲爼鎮块崨顖滅＝濞达絽澹婇崕蹇曠磼閵娾晙鎲剧€规洖銈搁、鏃堝醇閻斿搫骞堥梻濠庡亜濞诧箑顫忛崷顓涘亾濮橆厽绀堢紒杈ㄦ尭椤撳ジ宕熼鐘靛床婵°倗濮烽崑娑㈠疮椤栨粎鐭夐柟鐑樻煛閸嬫捇鏁愭惔婵堢泿缂備焦鍔栫粙鎺旀崲濠靛鍋ㄩ梻鍫熷垁閵忋倖鍊垫慨妯煎帶婢ф挳鏌ｅ☉鍗炴灈妞ゎ偅绮撻崺鈧い鎺嗗亾妞ゎ偄绻掔槐鎺懳熺拠宸偓鎾绘⒑閸涘﹦鈽夐柨鏇樺€濆鎶藉醇閵忋垻锛濇繛杈剧悼閻℃棃宕靛▎鎾寸厱濠电姴鍋嗛悡濂告煕閳规儳浜炬俊鐐€栫敮濠勭矆娓氣偓瀹曠敻顢楅崟顒傚幈闂佽宕樺▔娑㈠几濞戞埃鍋撶憴鍕婵＄偘绮欏畷娲焵椤掍降浜滈柟鍝勭Ч濡惧嘲霉濠婂嫮鐭掗柡宀€鍠栧畷顐﹀礋椤掑顥ｅ┑鐐茬摠缁秹宕曢幎瑙ｂ偓鏃堝礃椤斿槈褔骞栫划鍏夊亾瀹曞浂鍟囬梻鍌欑劍閹爼宕瑰ú顏呭亗闁跨喓濮寸粻鏍旈敐鍛殲闁稿﹤顭烽弻锕€螣娓氼垱楔濡炪倖鎸诲钘夘潖濞差亜绠伴幖娣焺濮婃寧绻濆▓鍨灁闁稿﹥娲熼崺銏狀吋婢跺⿴娼婇梺闈涚墕濡矂骞忓ú顏呯厽闁绘ê鍘栭懜顏堟煕閺傝儻瀚伴柍璇茬Ч楠炲洭鎮ч崼銏犲箥闂備礁鎲￠崹顖炲磹閺嶎偀鍋撳鐐
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ帒顦伴弲顏堟偡濠婂啰效婵犫偓娓氣偓濮婅櫣绱掑Ο铏逛紘濠碘槅鍋勭€氭澘顕ｉ崨濠勭懝闁逞屽墴瀵鈽夊Ο閿嬵潔濠殿喗顨呴悧濠囧极閹€鏀介柣鎰级閸ｈ棄鈹戦鑲╀粵缂佸矁椴哥换婵嬪炊椤儸鍥ㄧ厱婵炴垵宕弸娑欑箾閸滃啰绡€婵﹥妞介弻鍛存倷閼艰泛顏繝鈷€灞芥珝闁哄本鐩幃銏ゅ川婵犲嫮鈻忛梻浣风串缁插潡宕楀Ο璁崇箚闁归棿绀侀悡娑樏归敐鍡樸仢闁绘稒鎹囧缁樻媴閻戞ê娈屽銈嗘处閸欏啫鐣烽幋锔藉€烽柛銊︾☉瑜版岸姊婚崒姘偓宄懊归崶顒夋晪鐟滃酣銆冮妷鈺佺濞达絿枪閸嬪秴鈹戦悩璇у伐闁绘妫涙竟鏇熺節濮橆厾鍘甸梺鍛婃寙閸涱厾顐奸梻浣虹帛閹歌煤閻旂厧钃熼柣鏃傚帶缁狙囨煕椤垵娅橀柡鍡愬灮缁辨挻鎷呴幋鎺戞儓婵犫拃鍕垫疁妤犵偛鐗撴俊姝岊槼闁哥姴妫濋弻娑㈠即閻愬吀绮甸梺姹囧€撶粈渚€鍩為幋锔绘晩缁绢厾鍏樼欢鏉戔攽閻愬弶瀚呯紓宥勭窔閻涱喗寰勯幇顒傤啋濡炪倖妫侀崑鎰八囬弶娆炬富闁靛牆妫楁慨鍌炴煕閵娧屾敯妞わ絼绮欏濠氬磼濮橆兘鍋撴搴ｇ焼濞撴埃鍋撴鐐搭殔楗即宕煎┑鍫㈠炊闂佺鍋愮悰銉ノｉ崨瀛樺€垮ù鐘差儐閳锋棃鏌涢妷顔煎闁稿孩顨呰灃闁挎繂鎳庨弳鐐烘煃闁垮鐏撮柡灞剧☉閳藉顫滈崼婵嗩潬闂備礁鐤囧Λ鍕囬悽绋胯摕闁挎繂妫楃粻鐘绘⒑閸涘﹥鈷愰柛銊ф暬閹箖鎮滈挊澶岊吅闂佹寧妫佹慨銈呪枍濠婂牊鈷戦柛娑橈工閹兼悂鏌涙繝鍐╃缂佽京鍋炵换婵嬪炊瑜庨悗顒勬⒑閸︻厼顣兼繝銏★耿閹瑦绻濋崶銊у帾婵犵數鍋涢悘婵嬪礉濠婂牊鐓熼柨婵嗛婢у瓨鎱ㄦ繝鍕笡闁瑰嘲鎳樺畷銊︾節閸屾稒鐣奸梻鍌欑閹碱偊鎯屾径宀€绀婂ù锝呮憸閺嗭箓鏌熼悧鍫熺凡闁告劏鍋撴俊鐐€栭幐楣冨磹閿濆鏅濋柛灞剧〒閸橀亶姊洪崫鍕偍闁告柨鐭傞幃姗€鏁撻悩宕囧幐闂佸憡渚楅崰姘舵儗閹烘鐓欐い鏃囧亹缁夎櫣鈧娲栭悥鍏间繆閹间礁唯鐟滃繘濡堕悜鑺モ拻濞达絿枪閺嗛亶鏌涢姀锛勫弨妞ゃ垺鐗楀鍕節鎼淬垻绋侀梻浣瑰劤缁绘锝炴径灞稿亾濮橆厼鍝洪柡灞界Ч婵＄兘濡疯缁辩偞绻涚€电ǹ袥闁哄懐濞€瀵鏁愰崱妯哄妳闂侀潧绻掓慨鏉懶掗崼銉︹拺闁告縿鍎辨牎閻庡厜鍋撶紒瀣硶閺嗭附鎱ㄥΟ鎸庣【缂佺姵濞婇弻鐔衡偓娑櫳戦埛鎰版煟鎼粹槅鐓兼慨濠冩そ閹兘鏌囬敃鈧▓鍫曟⒑缁嬫鍎愮紒瀣尰缁旂喖寮撮姀鈺傛櫖闂佺粯鍔楃悰銉╁箯濞差亝鈷掗柛灞炬皑婢ф盯鏌ら崘鍙夘棥闁靛洦鍔欓獮鎺楀箣閻欏懐鍚归梻鍌欐祰椤宕曢悡骞盯宕熼宓洤钃熼柕澶涘閸樻悂姊洪崨濠傚闁告柨瀛╅弲璺衡槈閵忥紕鍘撻梻浣哥仢椤戝懐绮幒妤侇梿濠㈣埖鍔栭悡銉︾節闂堟稒顥為柛锝嗘そ閹綊骞囬妸銉モ拤缂備胶绮换鍫ュ春閳ь剚銇勯幒宥堝厡妞も晝鍏橀幃妤呮晲鎼粹€茬敖闂佸憡锕㈡禍璺侯潖濞差亜浼犻柛鏇ㄥ亐閸嬫捇鎸婃径鍡樼亙闂佸搫娲㈤崹娲磹閸洘鈷戞い鎺嗗亾缂佸鏁婚幃锟犲礃椤忓懎鏋戝┑鐘诧工閻楀棛绮堥崼鐔稿弿婵☆垱瀵х涵鍓х磼閳ь剟宕熼鐔锋瀾闂佺粯顨呴悧鍡欑箔閹烘鐓曢悘鐐额嚙婵″ジ鎽堕弽顓熺叆婵犻潧妫欓崳褰掓煛閸℃鎳囬柡宀嬬秮楠炴帡骞嬮悩杈╅┏闂備線娼уú銈団偓姘嵆閻涱噣骞掑Δ鈧獮銏′繆閻愭潙鍔ゆい銉﹀哺濮婂宕掑▎鎰偘濡炪倖娉﹂崨顔煎簥闂佺懓鐡ㄧ换宥咁焽閳哄懏鐓忓┑鐐靛亾濞呭懐绱掗幇顓ф當闁宠鍨块幃鈺呭箵閹哄秶鏁栭梻浣告啞閼归箖顢栨径鎰摕婵炴垯鍨瑰敮闂侀潧绻嗛崜婵嬫偟閺嶎厽鈷戠紓浣姑粭鎺撶箾鐏忔牑鍋撻幇浣圭稁濠电偛妯婃禍婊堝箲閼哥偣浜滈柟鎹愭硾娴犳粌鈹戦垾铏儓妞ゎ亜鍟存俊鍫曞礃閵娿儺鐎虫繝纰樻閸嬪懐鎹㈤崒鐑嗘晪闁挎繂顦拑鐔兼煏婢跺牆鍔ゆい鏃€鍔栫换娑欐綇閸撗冨煂濠电姭鍋撻梺顒€绉撮惌妤呮煕閳╁啰鈯曢柍閿嬪灩缁辨挻鎷呴懖鈩冨灩缁牓鍩€椤掑嫭鈷戠紒瀣皡瀹搞儲绻濋埀顒勬焼瀹ュ懐鍔﹀銈嗗笂閼冲爼宕弻銉︾厵闁告垯鍊栫€氾拷  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳婀遍埀顒傛嚀鐎氼參宕崇壕瀣ㄤ汗闁圭儤鍨归崐鐐差渻閵堝棗鍧婇柛瀣尰濞艰鈹戠€ｎ偀鎷洪梻渚囧亞閸嬫盯鎳熼娑欐珷闁圭虎鍠楅悡娑㈡倶閻愭彃鈷旈柕鍡樺浮閺屽秷顧侀柛鎾卞妿缁辩偤宕卞☉妯硷紱闂佸憡渚楅崢楣冨汲閿旈敮鍋撻崗澶婁壕闂佸憡娲﹂崜娑㈠储闂堟侗娓婚柕鍫濇椤ュ棝鏌涚€ｎ偄濮夌紒顔芥閹煎綊宕楁径濠佸闁荤喐鐟ョ€氼厽寰勯崟顖涚厱閻庯綆浜峰銉╂煟閿濆洤鍘撮柟顔哄灮閸犲﹥娼忛妸锔界彍婵犵數鍋犻幓顏嗗緤閹稿海浠氬┑鐘灮閹虫挾绮旈悷閭︽綎濠电姵鑹剧壕鍏肩箾閸℃ê绗掗柛姗堢磿缁辨挻绗熼崶褎鐝梺鎼炲妼閻栧ジ鎮伴鈧獮鍥级鐠侯煈鍟囧┑鐐舵彧缁蹭粙骞楀⿰鍫熸櫖婵炲棙鍔楃弧鈧梺姹囧灲濞佳勭墡闂備胶鍘х紞濠勭不閺嶎厼绠栭柣鎰惈閸ㄥ倹銇勯幇顔夹＄紒銊ヮ煼濮婃椽鎮烽悧鍫熷創缂備礁顦晶搴ㄥ箯鐟欏嫬顕遍悗娑欘焽閸橀亶姊虹紒妯活棃妞ゃ儲鎸剧划缁樼節濮橆厾鍘搁柣蹇曞仜婢ц棄煤閹绢喗鐓欐い鏃囶潐濞呭懘鏌ｉ敐鍥у幋鐎规洘鍎奸ˇ顕€鏌熼摎鍌氬祮闁哄矉绲鹃幆鏃堝Χ鎼淬垻绉锋繝鐢靛仜瀵爼鏁冮姀鐘垫殾婵犻潧妫岄弸搴ㄦ煙閹规劖纭剧紒渚婄畵濮婃椽宕崟顓夌娀鏌涢弬璺ㄐｉ柛鎺戯躬楠炴﹢顢欓悾灞藉箺闂備胶绮濠氬煕閸儱姹查柟鐑橆殕閻撴洟鏌熺€涙鐭岄柣顓熺懄閹便劍绻濋崟顓炵闂佺懓鍢查幊鎰垝濞嗗繆鏋庨柣鎰靛墻濡繘姊婚崒娆掑厡閺嬵亝銇勯幋婵囧殗妤犵偛锕ラ幆鏃堟晲閸涱厾鏌ч梻鍌氬€风粈渚€骞栭銈囩煓濞撴埃鍋撶€规洘鍨垮畷鍗炍熺亸鏍ㄧ亙闁诲骸绠嶉崕閬嶅箖閼愁垬浜归柟鐑樻尭娴滄鏌熼懝鐗堝涧缂佹煡绠栭獮澶嬬附閸涘ň鎷虹紓鍌欑劍钃遍悘蹇曞缁绘盯鎳犻鈧弸娑氣偓娈垮枛椤嘲顕ｉ幘顔藉亜濡炲娴烽悰顔界節绾版ɑ顫婇柛銊╂涧閳诲秹鏁愰崱娆樻祫缂備礁顑呯花閬嶅绩娴犲鐓ユ繛鎴灻顒佷繆閻愵剚鍊愰柡灞剧洴閹晛鐣烽崶褉鎷伴梻浣虹《閺呮繈宕戦妶澶屽祦閻庯綆鍠楅弲婊呯磽娴ｈ偂鎴濃枍閺冨牊鈷掑ù锝堟閵嗗﹪鏌涢幘瀵哥疄闁轰礁顑呴—鍐Χ閸涘宕梺鐟板殩閹凤拷
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ帒顦伴弲顏堟偡濠婂啰效婵犫偓娓氣偓濮婅櫣绱掑Ο铏逛紘濠碘槅鍋勭€氭澘顕ｉ崨濠勭懝闁逞屽墴瀵鈽夊Ο閿嬵潔濠殿喗顨呴悧濠囧极閹€鏀介柣鎰级閸ｈ棄鈹戦鑲╀粵缂佸矁椴哥换婵嬪炊椤儸鍥ㄧ厱婵炴垵宕弸娑欑箾閸滃啰绡€婵﹥妞介弻鍛存倷閼艰泛顏繝鈷€灞芥珝闁哄本鐩幃銏ゅ川婵犲嫮鈻忛梻浣风串缁插潡宕楀Ο璁崇箚闁归棿绀侀悡娑樏归敐鍡樸仢闁绘稒鎹囧缁樻媴閻戞ê娈屽銈嗘处閸欏啫鐣烽幋锔藉€烽柛銊︾☉瑜版岸姊婚崒姘偓宄懊归崶顒夋晪鐟滃酣銆冮妷鈺佺濞达絿枪閸嬪秴鈹戦悩璇у伐闁绘妫涙竟鏇熺節濮橆厾鍘甸梺鍛婃寙閸涱厾顐奸梻浣虹帛閹歌煤閻旂厧钃熼柨鐔哄Т閻愬﹪鏌曟径鍫濆姎妞ゎ剙鐗婄换婵嬪煕閳ь剛浠﹂懞銉т邯闂傚倸娲らˇ鐢稿蓟閿濆拋娼ㄩ柍褜鍓欓…鍥樄鐎规洘绻勯埀顒婄秵閸撴稓澹曢懖鈺冪＝濞达綀顕栭悞浠嬫煃椤栨稒绀冪紒缁樼洴閹剝鎯旈敐鍥跺晪闁诲氦顫夊ú妯煎垝韫囨蛋鍥蓟閵夛妇鍘遍梺瀹犳〃閼冲爼鎮為崨濠冨弿濠电姴鍟妵婵堚偓瑙勬处閸嬪﹥淇婇悜钘壩ㄩ柨鏃堟暜閸嬫捇顢橀姀鈾€鎷洪梺鍛婄☉閿曘儵鍩涢幇鐗堢厱闁靛ǹ鍎虫禒銏ゆ煟閿濆懎妲婚悡銈嗐亜韫囨挸顏柛鏃撶畱椤啴濡堕崱妤冪憪闂傚倸瀚粔鐢电矉瀹ュ應鍫柛顐ゅ枔閸樼敻姊洪崨濠勭畵閻庢凹鍙冭棢婵犲﹤鍘鹃悷鎵冲牚闁割偁鍨婚弳顐⑩攽椤旂》鏀绘俊鐐舵閻ｇ兘顢曢敃鈧敮闂侀潧顧€婵″洭鍩€椤掍礁鍔ら柍瑙勫灴閹瑩骞撻幒鎾斥偓顖炴⒑閼姐倕鏆遍柡鍛█婵″瓨鎷呴崜鍙夊兊闂佽偐枪閻忔艾鈻嶅⿰鍕瘈闁靛骏绲介悡鎰版嫅闁秵鐓涢柛娑卞枤缁犵偞鎱ㄦ繝鍐┿仢妤犵偞鍔栭幆鏃堝閳哄倵鎸呭┑鐘殿暯濡插懘宕戦崟顓涘亾濮樼厧骞樼紒顔碱儏椤撳吋寰勭€ｎ剙濮搁柣搴＄畭閸庡崬螞瀹€鍕婵炲樊浜濋埛鎴︽煕濞戞﹫鏀婚悘蹇斿閹叉悂寮堕崹顔芥缂備礁鍊哥粔鎾煘閹达箑閱囬柕蹇ｆ緛缁辨煡姊洪懡銈呅㈡繛璇х畳閵囨劙宕橀鍛櫆濡炪倖鎸鹃崑鎰板绩娴犲鐓冮柕澶堝劤閿涘秹鏌涢埡瀣М闁哄备鍓濋幏鍛喆閸曨偆锛撻梻浣芥〃閻掞箓骞戦崶顒€鏋侀柟鍓х帛閺呮悂鏌ㄩ悤鍌涘
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
                        //婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繘鏌ｉ幋锝嗩棄闁哄绶氶弻娑樷槈濮楀牊鏁鹃梺鍛婄懃缁绘劙婀侀梺绋跨箰閸氬绱為幋锔界厱闁靛ǹ鍎遍埀顒€娼″濠氭晲婢跺﹦顔掗悗瑙勬礀濞层倝宕ú顏呪拺闁告繂瀚烽崕鎰版煟濡や緡娈橀柟骞垮灩閳藉濮€閻樻鍚呴梻浣虹帛閸旀浜稿▎鎾崇濞寸厧鐡ㄩ埛鎴犵磼鐎ｎ偒鍎ラ柛搴㈠姍閺岀喖宕ㄦ繝鍐ㄢ偓鎰版煥濠靛牆浠滈柍瑙勫灩閳ь剨缍嗛崑鍕濡ゅ懏鐓欓柛蹇氬亹閺嗘﹢鏌涢妸锔筋潡闁靛洦鍔栭幆鏃堬綖椤撶姷鐣鹃梻渚€娼ч悧鍡椕洪妶鍛瘎闂傚倷鑳舵灙妞ゆ垵妫濋幆鍕敍濮樿鲸娈惧銈嗙墱閸庢劙寮鍡欑瘈濠电姴鍊搁鈺呭箹閺夋埊宸ラ柍瑙勫灴閹晝绱掑Ο濠氭暘婵＄偑鍊栭崹鐢稿箠濮椻偓楠炲啫鈻庨幇顔剧槇闂佹悶鍎崝搴ㄥ储椤忓懐绡€闁汇垽娼ч埀顒夊灦瀹曟﹢濡搁敂绛嬪敳闂備礁鎽滈崰鎰板箖閸屾凹娼栧┑鐘宠壘绾惧吋鎱ㄥ鍡楀幋闁稿鎹囬幃婊堟嚍閵夈儲鐣遍梻浣告啞閹哥兘鎮為敃鍌氱婵犲﹤鐗婇悡鏇熴亜閹板墎绋荤紒鈧埀顒傜磽娴ｆ彃浜鹃梺鍓插亞閸犳挾寮ч埀顒勬⒒閸屾氨澧涘〒姘殜瀹曟洖顓兼径瀣帗閻熸粍绮撳畷婊冣枎閹惧磭鏌堥梺鍝勵槹閸ㄥ綊寮抽敃鍌涚厪濠电偟鍋撳▍鎾绘煛娴ｅ摜校缂佺粯绻冪换婵嬪磼濞戞﹩鈧稑鈹戦檱鐏忔瑦鐏欓梺瀹狀潐閸ㄥ潡骞冨▎鎴斿亾濞戞顏堟瀹ュ鈷戠紒顖涙礃濞呭懘鏌涢悢鍛婄稇闁伙絿鍏橀獮瀣晝閳ь剛绮婚懡銈囩＝濞达綀顕栭悞浠嬫煕濡粯鍊愰柡宀嬬秬缁犳盯寮撮悙鎵崟濠电姭鎷冮崱姗嗘闁绘挶鍊濋弻鐔虹磼閵忕姵鐏嶉梺缁樻尰缁嬫垿婀侀梺鎸庣箓濞诧箓宕甸埀顒佺節濞堝灝鏋涚紒澶婄埣閸┾偓妞ゆ帒鍠氬鎰箾閸欏鑰块柡浣稿暣婵偓闁炽儴灏欑粻姘舵⒑瑜版帗锛熺紒鈧担鍛婃殰濠碉紕鍋戦崐鏍涢崘顔兼瀬妞ゆ洍鍋撶€规洘鍨块獮妯兼嫚閼碱剦鍟嬫俊鐐€栧Λ浣筋暰闂佺懓鍢查幖顐﹀煘閹达富鏁婇柣鎰靛墮濞堝苯鈹戦悙鑼勾闁稿﹥绻堥獮鍐倷鐟佷焦妞介、鏃堝川椤栨艾绠為梻鍌欑窔濞佳嗗闂佸搫鎳忛悷鈺呯嵁婢舵劖鍊锋い鎺戭槹椤旀棃姊虹紒妯哄婵炲吋鐟﹂幈銊ヮ吋閸ワ絽浜鹃悷娆忓缁€鍐煥濮橆厹浜滈柡鍌濇硶缁犵粯銇勯姀锛勬噰鐎规洘顨堥崰濠囧础閻愭潙浜奸梻鍌氬€搁崐鎼佸磹妞嬪孩顐芥慨姗嗗墻閻掔晫鎲搁幋鐐存珷婵犻潧顑嗛埛鎴︽煕濠靛棗顏柛锝堟缁辨帡顢欓懞銉ョ闂佷紮绲块崗姗€骞冮悾灞芥瀳婵☆垵妗ㄦ竟鏇㈡⒑閹稿海绠撳Δ鐘虫倐瀵悂鎮㈤崗鑲╁幗闂佽鍎抽悺銊х矆閸愵喗鐓忛柛銉戝喚浼冨銈冨灪閿曘垽骞冮姀鈽嗘Ч閹煎瓨绻冨Ο濠冪節閻㈤潧浠滈柣掳鍔庨崚鎺戭吋閸ャ劌搴婇梺鍛婂姦閸犳牗顢婇梻浣告啞濞诧箓宕归弶璇炬稑顫滈埀顒勫箖瑜版帒绠掗柟鐑樺灥椤ュ酣鎮峰⿰鍐弰妤犵偛鐗撴俊鎼佹晜閸撗呮闂備礁鎲￠崝蹇涘棘閸屾稓顩烽柕蹇嬪€栭崐鐢告偡濞嗗繐顏紒鈧崼銉︾厱闊洦妫戦懓鍧楁煏閸℃洜顦︽い顐ｇ矒閸┾偓妞ゆ帒瀚粻鏍煏閸繃顥炲┑顖涙尦濮婃椽宕归鍛壈闂佺粯鎸诲ú鐔奉潖婵犳艾纾兼慨姗嗗厴閸嬫捇鎮滈懞銉ユ畱闂佸壊鍋呭ú鏍不閻樼粯鐓欓柟瑙勫姦閸ゆ瑩鏌涘顒傜Ш闁哄备鈧剚鍚嬮幖绮光偓宕囶啇缂傚倷鑳舵刊顓㈠垂閸洖钃熼柕濞炬櫆閸嬪棝鏌涚仦鍓р槈妞ゅ骏鎷�
                        
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
    

    if(counter)
        counter--;
    if(meter_overtime)
        meter_overtime--;
    
    if(meter_overtime==0){
        meter_data_sent=0;
    }
    if(counter==0 && online_dev_count!=0 && meter_data_sent==0){
        counter=REALTIME_METER_READ_COUNTER;
        if(anti_refluxing)
            counter=REALTIME_METER_READ_COUNTER_REFLUX;
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
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鎯у⒔閹虫捇鈥旈崘顏佸亾閿濆簼绨绘い鎺嬪灪閵囧嫰骞囬姣挎捇鏌熸笟鍨妞ゎ偅绮撳畷鍗炍旈埀顒勫煕閹烘鈷戠紓浣姑粭鍌滅磼椤旂晫鎳囩€殿喖顭锋俊鍫曞幢濡搫浼庨梻浣哥秺閸嬪﹪宕楀鈧畷鎴﹀箻鐠囨彃鍞ㄥ銈嗗姉閸犲孩绂嶉悙顒佸弿婵妫楁晶濠氭煟閹剧偨鍋㈤柡灞糕偓宕囨殕閻庯綆鍓涜ⅵ闁诲孩顔栭崳顕€宕抽敐鍛殾闁圭儤鍩堝鈺傘亜閹达絾纭堕悽顖涚〒缁辨捇宕掑顑藉亾妞嬪孩濯奸柡灞诲劚绾惧鏌熼悙顒€澧柣鏂挎閺屻倝骞栨担瑙勯敪婵犳鍠栧ú銊ф閹烘鍋愬〒姘煎灡缂嶅牏绱撴担铏瑰笡闁烩晩鍨跺顐﹀箛椤撶喎鍔呴梺鐐藉劜閸撴艾顭囩拠娴嬫斀闁绘ɑ顔栭弳顖炴煃瑜滈崜娑㈠磻閵娾晛纾婚柕蹇嬪€栭悡娑㈡倶閻愭彃鈷旈柍钘夘槹閵囧嫰顢曢敐鍥╃杽濡炪們鍨洪〃濠傜暦閹烘垟鏀﹂柣鎰典簻婵″ジ鎽堕弽顓熺厽婵せ鍋撴繛浣冲嫮顩烽柨鏇炲€归悡鐔镐繆閵堝嫮璐版繛鍫熸⒐閵囧嫰顢橀悙瀵糕敍缂備胶濮电粙鎴﹀煡婢跺á鐔哄枈鏉堛剱銈夋⒑閼姐倕鏋戠紒顔肩灱閹广垹螖閸涱厾顔戝銈嗗姧缁茶法寮ч埀顒勬⒑濮瑰洤鐏叉繛浣冲嫮澧″┑锛勫亼閸婃牕顫忚ぐ鎺撳亱闁绘ǹ灏欓弳锕傛偡濞嗗繐顏╂い鏇憾閺屸剝寰勭€ｎ亝顔曢悷婊勬瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鈧娲寠婢跺﹥娈堕梺鍝ュУ閻楃姴顕ｆ繝姘櫜濠㈣泛锕﹂惈鍕⒑缁嬫寧婀版い銊ユ閳ь剚纰嶉惄顖氼潖閻戞ɑ濮滈柟娈垮櫘濡差喚绱撴担鍓叉Ц缂傚秴锕ら锝囨嫚濞村顫嶉梺闈涚箳婵兘宕濋幘顔解拺闁告稑锕ゆ慨澶愭煕鐎ｎ偅宕岄柟顔藉劤閻ｏ繝骞嶉搹顐ｆ澑闂備胶绮敃鈺呭磻閸曨剛顩查柣鎰靛墰缁犻箖鏌℃径瀣仴闁诡喗鍨剁换娑㈠礂閼测晜鍣伴悗瑙勬礃閿曘垽宕洪埀顒併亜閹烘垵顏柍閿嬪灴閺岋綁骞囬娑辨闂佸憡姊圭划鎾诲蓟閿涘嫧鍋撻敐搴濈暗闁稿鍨婚埀顒侇問閸犳捇宕濆鍥╃焿闁圭儤鏌￠崑鎾绘晲鎼粹€茬敖闂佸憡眉缁瑥顫忔ウ瑁や汗闁圭儤鍨抽崰濠囨⒑閸涘⿴鐒介柡鍜佸亞濡叉劙鎮欓崫鍕吅闂佹寧姊婚弲顐﹀储娴犲顥婃い鎰╁灪閸屻劑鏌涙惔銏犫枙闁诡噣绠栭幃浠嬪川婵犲嫬骞愰柣搴″帨閸嬫捇鏌嶈閸撶喎鐣锋导鏉戠閻犲搫鎼悘濠傗攽閻愬弶顥為柟绋挎憸缁粯瀵奸弶鎴狀啇濠电儑缍嗛崜娆撴倶閳哄懏鐓涢悗锝庡亜閻忊晝绱掓潏銊﹀磳鐎规洘甯掗埢搴ㄥ箣閿濆洨宕堕梻鍌欑閹芥粍鎱ㄩ悽鎼炩偓鍐川閹殿喕缃曢梻鍌欑閹诧繝宕濋幋锕€绀夐幖娣妼濮规煡鏌ㄩ弴妤€浜鹃梺瀹狀潐閸ㄥ灝鐣烽幒鎴旀敠闁规儳纾粔铏光偓瑙勬穿缁绘繈鐛惔銊﹀殟闁靛／鍐ㄧ闂傚倷绀侀幖顐﹀疮椤愨挌娲晝閸屾稑娈橀柣鐔哥懃鐎氼亞鎹㈤崱娑欑厱婵炲棗娴氬Σ绋库攽椤旇偐校闁逛究鍔嶇换婵嬪礋椤撶偟顐奸梻浣烘嚀閸㈡煡骞婂鈧獮鍐ㄢ堪閸忓墽鍠栭幃婊兾熼崗鍏碱唴濠电姷顣槐鏇㈠磻閹达箑纾归柡宥庣亹濞差亝鏅濋柛灞炬皑閻撴垿姊洪崨濠傚闁告柨閰ｅ鎶芥晝閸屾稓鍘甸柡澶婄墦缁犳牕顬婇鈧弻娑氣偓锝庝簻椤忣偆绱掓潏銊ョ瑨閾绘牠鏌涢弴妤佹澒闁稿鎹囬崺鈧い鎺戝閻撴洟鎮楅敐搴′簼鐎规洖鐬奸埀顒冾潐濞插繘宕濋幋婵愬殨闁割偅娲栫粻锝夋煥濠靛棙顥為柛鐔告そ濮婂宕掑▎鎴犵崲濠电偘鍖犻崵韬插姂閸┾偓妞ゆ巻鍋撻柍瑙勫灴閸ㄩ箖鎼归銏＄亷闁诲氦顫夊ú鈺冪礊娴ｅ摜鏆﹂柛妤冨亹濡插牊绻涢崱妤佹拱閻庢俺鍋愮槐鎾诲磼濮橆兘鍋撻悜鑺ュ€块柨鏇氱劍閹冲瞼绱撻崒娆掝唹闁稿鎹囬弻宥堫檨闁告挾鍠庨～蹇涙惞閸︻厾鐓撻梺鍛婄墤閳ь剙鍘栫槐锝嗙節閻㈤潧袥闁稿鎹囧娲敆閳ь剛绮旈幘顔藉€块柛顭戝亖娴滄粓鏌熼崫鍕ラ柛蹇撶焸閺屾盯鎮㈤崫銉ュ绩闂佸搫鐬奸崰鏍х暦濞嗘挸围闁糕剝顨忔导锟�0,
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鎯у⒔閹虫捇鈥旈崘顏佸亾閿濆簼绨奸柟鐧哥秮閺岋綁顢橀悙鎼闂侀潧妫欑敮鎺楋綖濠靛鏅查柛娑卞墮椤ユ艾鈹戞幊閸婃鎱ㄩ悜钘夌；闁绘劗鍎ら崑瀣煟濡崵婀介柍褜鍏涚欢姘嚕閹绢喖顫呴柍鈺佸暞閻濇洟姊绘担钘壭撻柨姘亜閿旇鏋ょ紒杈ㄦ瀵挳濮€閳锯偓閹风粯绻涙潏鍓хК婵炲拑缍佹俊瀛樼節閸ャ劎鍘遍梺瑙勫劤椤曨厾绮婚悙鐑樼厵闁芥ê顦埀顒€顭烽崺鈧い鎺戯功缁夌敻鏌涢悩宕囧⒌闁诡喗鐟︾换婵嬪礃閿旇法鐩庨梻浣烘嚀閹碱偆绮旈悜钘夌鐎光偓閸曨剛鍘遍梺鍝勫€藉▔鏇㈠春閿濆鐓欐い鏃傜摂濞堟粓鏌ｅ☉鍗炴珝鐎规洖缍婇、娆撴偂鎼搭喗缍撻梻鍌氬€烽懗鍫曞箠閹捐鐤悹鎭掑妽瀹曞弶鎱ㄥ璇蹭壕濡ょ姷鍋涢崯顐ョ亙闂佸憡绮堥悞锕傚疾閵忋倖鈷戦柛娑橈攻婢跺嫰鏌涜箛鏃撹€挎鐐茬箰鐓ゆい蹇撴噳閹锋椽姊洪崨濠勨槈闁挎洩绠撻崺銉﹀緞閹邦厾鍙嗛梺鍝勬祩娴滄粓銆呴鍌滅＜閺夊牄鍔嶇亸顓熴亜椤愶絿绠為柟铏矒閹粙妫冨☉妯肩杽闂傚倷鑳堕幊鎾诲触鐎ｎ喗鍋╂い蹇撶墕閸ㄥ倸鈹戦悩瀹犲闁活厽顨婇弻宥堫檨闁告挾鍠栭獮鍐ㄎ旈崘鈺佹瀭闂佸憡娲﹂崜娑⑺囬妸鈺傗拺闁硅偐鍋涙俊鍧楁煛娴ｅ壊鐓奸柡浣瑰姍閹瑥霉鐎ｎ亞澧梻浣稿閸嬪懐鎹㈤崟顖氱闁挎洍鍋撴い顏勫暣婵″爼宕卞Δ鍐噯闂佽瀛╅崙褰掑礈閻旂厧绠柟杈鹃檮閸嬪嫰鏌涜箛姘汗闁告﹩浜濈换婵嬫偨闂堟稐鎴烽梺绋款儐閹告悂鍩㈠澶婂耿婵炴垶鐟ч崢閬嶆煟鎼搭垳绉靛ù婊勭箞钘熼柕蹇嬪€栭悡娑㈡倶閻愭彃鈷旈柕鍡樺笒闇夐柣娆忔噽閻ｇ數鈧娲樼划蹇浰囬鎴掔箚闁圭粯甯炴晶鏇熴亜椤忓嫬鏆ｅ┑鈥崇埣瀹曘劎鈧數枪濡炰粙姊绘担鐑樺殌闁搞倖鐗犻獮蹇涙晸閿燂拷
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
                        if (0x0212 == (online->code))           //濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌涘☉姗堟敾闁告瑥绻橀弻锝夊箣閿濆棭妫勯梺鍝勵儎缁舵岸寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ゆい顓犲厴瀵鏁愭径濠勭杸濡炪倖甯婇悞锕傚磿閹剧粯鈷戦柟鑲╁仜婵″ジ鏌涙繝鍌涘仴鐎殿喛顕ч埥澶愬閳哄倹娅囬梻浣瑰缁诲倸螞濞戔懞鍥Ψ瑜忕壕钘壝归敐鍛儓鐏忓繘姊洪崨濠庢畷濠电偛锕ら锝嗙節濮橆厼浜滈梺绋跨箰閻ㄧ兘骞忔繝姘厽閹艰揪绱曟禒娑㈡煟閳╁啯绀堢紒顔碱煼閹瑩鎮滃Ο閿嬪闂備胶顢婇崑鎰板磻濞戙垹绀夐柟缁㈠枟閻撴洟鏌熼幆褜鍤熼柟鍐叉噺缁绘盯骞嬮悩铏瘓闂佺硶鏂侀崑鎾愁渻閵堝棗绗掗悗姘煎弮閸╂盯骞嬮敂钘変化闂佹悶鍎烘禍婊堟儍閾忓湱纾奸柣妯诲絻缁狙兦庨崶褝韬€规洩绲惧鍕節閸愵厾妾ㄩ梻鍌欑劍鐎笛呮崲閸曨垰纾婚柣鎰▕閸ゆ鏌涢弴銊ュ缂佲檧鍋撻梺纭呭亹鐞涖儵宕滃┑瀣垫晩闁稿瞼鍋為埛鎺懨归敐鍛暈闁哥喓鍋炵换娑氭嫚瑜忛悾鐢碘偓瑙勬礀缂嶅﹪鐛澶樻晩闁告瑣鍎抽悷婵囩節閻㈤潧浠﹂柛銊ョ埣閹兘鏁愰崱娆樻锤濠电姴锕ら悧濠囨偂濞嗘挻鐓欓悷娆忓婵牏鐥鐔峰姢闂囧鏌ｅΟ鍝勬毐缂佹う鍛＜閺夊牄鍔嶅畷宀€鈧娲樼敮鎺楀煡婢跺ň鏋庨煫鍥ㄦ处閺嗭繝姊婚崒娆戭槮闁汇倕娲敐鐐村緞閹邦剙鐎梺绉嗗嫷娈旂紒鐘崇墬缁绘盯宕卞Ο璇查瀺闂佹悶鍊曢澶婎潖婵犳艾閱囬柣鏃囥€€婵洭姊绘担璇″劌闁告ê澧藉Σ鎰板箻鐠囪尙锛滃┑鐐叉閸旀濡舵导瀛樺€垫繛鍫濈仢閺嬬喖鏌熼幖浣虹暫鐎规洘宀搁獮鎺楀箣閺冣偓閻庡妫呴銏″婵炲弶锚閳绘捇骞嗚濞撳鏌曢崼婵囶棤濠⒀囦憾閺屾稓鈧綆鍋嗛埊鏇㈡煕鎼搭喖浜版慨濠勭帛閹峰懘宕妷锔锯偓顔尖攽閳╁啨浠犻柛鏃€鍨块獮濠偽旈崨顓㈠敹闂佸搫娲ㄩ崑鐔妓囬鈧娲箰鎼淬垻锛曢梺绋款儐閹稿濡甸崟顖ｆ晜闁告洦鍋呭▓缁樼節绾版ǚ鍋撳畷鍥х厽閻庤娲栧畷顒冪亙闂佸憡鍔︽禍婵嬪闯椤栫偞鈷掗柛灞剧懅閸斿秹鎮楃粭娑樺幘妤﹁法鐤€婵炴垶顭囬敍鐔兼⒑鐟欏嫬鍔舵俊顐㈠閹偤宕归鐘辩盎闂佺懓鎼粔鐑藉礂瀹€鍕厱闁绘柨鎽滈崣鈧梺鍝勭灱閸犳牕鐣峰▎鎾澄ч柛鈩冾殢娴硷拷
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
* introduce:        
* parameter:                       
* return:                 
* author:           Luee                                                    
*******************************************************************************/
void updata_anti_reflux_para(void)
{
    Buffer_t buf;
    u32 buf_value;
    float f;

    //get para 96,anti reflux enable or disable?
    SysPara_Get(96, &buf);
    buf_value = Swap_charNum((char *)buf.payload);   
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

    updata_anti_reflux_para();
}

int float2int(u32_t data)
{
#if 0  //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ょ紓宥咃躬瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鍓х＜闁诡垎鍐ｆ寖闂佺娅曢幑鍥灳閺冨牆绀冩い蹇庣娴滈箖鏌ㄥ┑鍡欏嚬缂併劌銈搁弻鐔兼儌閸濄儳袦闂佸搫鐭夌紞渚€銆佸鈧幃娆撳箹椤撶噥妫ч梻鍌欑窔濞佳兾涘▎鎴炴殰闁圭儤顨愮紞鏍ㄧ節闂堟侗鍎愰柡鍛叀閺屾稑鈽夐崡鐐差潻濡炪們鍎查懝楣冨煘閹寸偛绠犻梺绋匡攻椤ㄥ棝骞堥妸鈺傚€婚柦妯侯槺閿涙盯姊虹紒妯哄闁稿簺鍊濆畷鎴犫偓锝庡枟閻撶喐淇婇婵嗗惞婵犫偓娴犲鐓冪憸婊堝礂濞戞碍顐芥慨姗嗗墻閸ゆ洟鏌熺紒銏犳灈妞ゎ偄鎳橀弻宥夊煛娴ｅ憡娈查梺缁樼箖閻楃姴顫忕紒妯肩懝闁逞屽墴閸┾偓妞ゆ帒鍊告禒婊堟煠濞茶鐏￠柡鍛埣瀵泛鈻庨崜褍鏁搁梺鑽ゅ枑閻熴儳鈧凹鍘剧划鍫⑩偓锝庡枟閻撴瑦銇勯弮鈧崕鎶藉储鐎涙﹩娈介柣鎰嚋閺€鑽ょ磼缂佹绠炵€规洘甯掗オ浼村川閸涱偄濮傛慨濠呮閹风娀鍨鹃搹顐や簮闂備礁鎼幊蹇涙儎椤栫偑鈧礁鈻庨幒鏃傛澑濠电偞鍨兼ご鎼佸疾閳哄懏鈷戠紓浣股戠亸顓炍旈悩宕囨憙闁诲繐娲ㄧ槐鎾诲磼濮橆兘鍋撻幖浣哥９闁归棿绀佺壕褰掓煙闂傚顦︾痪鎯х秺閺屾稑鈹戦崟顐㈠Б闂佹椿鍘介幑鍥蓟閿濆顫呴柣妯哄暱閺嗗牓姊虹粙娆惧剰妞ゆ垵顦靛濠氭晸閻樿尙鍊為梺鎸庣箓濡鈻嶅鑸碘拺闂侇偆鍋涢懟顖涙櫠閹€鏀介柍銉ㄦ珪閸犳ɑ顨ラ悙鏉戠伌鐎规洜鍠栭、娑樞掔亸鏍ㄦ珚闁哄被鍔戝鏉懳旈埀顒佺閸撗呯＜闁告挆鍡橆€楅梺鎼炲姀濞夋盯锝炶箛娑欐優闁革富鍘鹃敍婊冣攽閳藉棗鐏ユ繛鍜冪稻缁傛帡骞栨担鍦弮闂佸憡鍔﹂崰鏇㈠箟妤ｅ啯鐓涚€光偓閳ь剟宕伴弽顓炵畺婵犲﹤鐗嗙粻锝夋煙閻戞ɑ灏柡瀣ㄥ€栫换婵嗏枔閸喗鐏堥梺鎸庢磸閸斿矂鍩㈠澶嬫櫜闁搞儜鍐喊婵犵數鍋涘Λ娆撳垂鐠鸿櫣鏆﹀鑸靛姈閻撳繐鈹戦悙鎴濆暢鐎氫即鏌℃担钘変汗缂佽鲸鎹囧畷鎺戭潩椤戣棄浜惧瀣椤愪粙鏌ㄩ悢鍝勑㈢紒鎰殕娣囧﹪濡堕崨顔兼缂備胶濮甸悡锟犲蓟濞戙垹唯闁瑰瓨绻嗙€氱増绻涢崼娑樺缂佺粯绻堥幃浠嬫濞磋缍侀弻銈夊捶椤撶偘澹曢梺绋垮瘨閸嬪﹤顫忕紒妯诲缂佸瀵уВ鎰版⒑閸︻厸鎷￠柛瀣工閻ｇ兘骞囬弶鍨獩婵犵數濮寸€氬嘲煤閹间焦鈷戠紓浣姑慨澶愭煕鎼存稑鈧繈宕哄☉銏犵闁绘鏁搁敍婊堟煟鎼搭垳绉甸柛瀣椤㈡艾饪伴崟顓犵槇闂侀€炲苯澧存鐐茬Ч椤㈡瑩宕滆缁辨煡姊虹拠鎻掑毐缂傚秴妫濆畷鎴﹀礋椤栨稑鈧爼鎮楀☉娅辨粍绂嶅⿰鍫熺厪闊洤锕ゆ晶鍙夈亜閵夈儳澧﹂柡灞稿墲閹峰懐鎲撮崟顐わ紦闂備浇妗ㄩ悞锕傚箲閸ヮ剙鏋侀柟鍓х帛閺呮悂鏌ㄩ悤鍌涘 1
    s8_t S;     //缂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ょ紓宥咃躬瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鍓х＜闁绘劦鍓欓崝銈囩磽瀹ュ拑韬€殿喖顭烽幃銏ゅ礂鐏忔牗瀚介梺璇查叄濞佳勭珶婵犲伣锝夘敊閸撗咃紲闂佺粯鍔﹂崜娆撳礉閵堝洨纾界€广儱鎷戦煬顒傗偓娈垮枛椤兘寮幇顓炵窞濠电姴瀚烽崥鍛存⒒娴ｇ懓顕滅紒璇插€块獮澶娾槈閵忕姷顔掔紓鍌欑劍椤洭宕㈡潏銊х瘈闁汇垽娼у瓭闂佺ǹ锕ょ紞濠傜暦閹达箑唯闁冲搫鍊婚崢鎼佹煟韫囨洖浠╂い鏇嗗嫭鍙忛柛灞惧閸嬫挸鈻撻崹顔界亶闂佺懓鎲℃繛濠囩嵁閸愩劉鏋庣€电増绻傚﹢閬嶅焵椤掑﹦绉靛ù婊勭懄缁旂喖寮撮姀鈾€鎷洪梺鍛婃尰瑜板啯绂嶅┑鍥╃闁告瑥顦辨晶鐢告煙椤旀儳浠辩€规洖缍婇、鏇㈡偐鏉堚晝娉块梻鍌欒兌閹虫捇骞栭鈶芥稑螖娴ｄ警娲告俊銈忕到閸燁垶鍩涢幋锔界厱婵犻潧瀚崝姘舵煛鐎ｎ偄鐏撮柡宀€鍠栧畷銊︾節閸屾鐏嗛梻浣筋嚃閸犳洟宕￠幎濮愨偓浣割潩鐠鸿櫣鍔﹀銈嗗坊閸嬫捇鏌ｉ敐鍥у幋鐎规洖銈稿鎾Ω閿旇姤鐝滄繝鐢靛О閸ㄧ厧鈻斿☉銏╂晞闁糕剝銇涢弸宥夋煙閸撗呭笡闁抽攱甯掗湁闁挎繂鎳忛崯鐐烘煕閻斿搫浠遍柡宀嬬秮閹倝宕掑⿰鍛呮粓鎮楃憴鍕婵炶尙鍠栭悰顔芥償閵婏箑娈熼梺闈涱槶閸庝即鎮界紒妯肩瘈闁汇垽娼ч埢鍫熺箾娴ｅ啿娴傞弫鍕煕閳╁啰鈽夐柣鎾存礋閺岀喖鎮滃Ο鐑橆唴閻熸粎澧楃敮妤呭疾閺屻儲鐓曢柡鍥ュ妼濞呮﹢鏌涢弬鎸庡殗婵﹨娅ｉ幃浼村灳閸忓懎顥氭繝鐢靛仜椤曨參宕㈣閹椽濡搁埡鍌氫簵濠电偛妫欓崝妤冪不妤ｅ啯鐓犲┑鍌氬閺夋椽鏌涢悙鑸殿仩缂佽鲸甯℃俊姝岊槼婵炲懏娲熼弻宥堫檨闁告挻鐟╅幃妯侯潩椤掑鍞靛┑顔姐仜閸嬫捇鏌℃担绋挎殻闁轰礁鍊块幃娆擃敆婢跺绋愰梻鍌欑濠€閬嶅磿閵堝鍚归柨鏇炲€归崑鍕煕濠靛棗顏柣搴㈠▕閺屸剝寰勬繝鍕暥闂佸憡姊归崹鍨嚕椤愨懇鍫柛顐ゅ暱閹锋椽姊洪悡搴綗闁稿﹥娲熷鎼佸箣閻樼數锛滃銈嗘⒒閸樠囶敆閵忋倖鐓涢柛娑氬绾箖鎽堕弽顓熺厱闁硅埇鍔嶅▍鍡樼箾閸喐绶查柍瑙勫灦楠炲﹪鏌涙繝鍐╃妤犵偛锕ㄧ粻娑樷槈濡》绱遍梻浣呵归惉濂稿磻閻愮儤鍋傞煫鍥ㄦ惄閻斿棝鎮归搹鐟板妺妞ゃ儳鍋炵换娑㈠川椤撶噥妫﹂梺鍝勭焿缂嶄線鐛Ο鍏煎磯闁绘垶顭囨禍顏堟⒒娴ｅ憡鎯堥柣顓烆槺閹广垹鈹戦崱娆愭闂佸壊鍋呭ú鏍偂濞戞◤褰掓晲婢舵ɑ顥栭梺纭咁潐濞茬喎顫忕紒妯肩懝闁逞屽墴閸┾偓妞ゆ帒鍊告禒婊堟煠濞茶鐏￠柡鍛埣椤㈡瑩鎮惧畝鈧惁鍫ユ⒑闁偛鑻晶鎾煛鐏炶姤顥滄い鎾炽偢瀹曘劑顢涘顑洖鈹戦敍鍕杭闁稿﹥鐗滈弫顔界節閸ャ劌娈戦梺鍓插亝濞叉牠鎮块鈧弻锛勪沪鐠囨彃濮曢梺缁樻尰閻熲晠寮诲☉銏犵婵炲棗绻嗛崑鎾诲冀椤愮喎浜炬慨妯煎亾鐎氾拷
    s8_t  E;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ょ紓宥咃躬瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鍓х＜闁诡垎鍐ｆ寖闂佺娅曢幑鍥灳閺冨牆绀冩い蹇庣娴滈箖鏌ㄥ┑鍡欏嚬缂併劌銈搁弻鐔兼儌閸濄儳袦闂佸搫鐭夌紞渚€銆佸鈧幃娆撳箹椤撶噥妫ч梻鍌氬€稿ú銈壦囬悽绋胯摕闁靛ǹ鍎弨浠嬫煕閳锯偓閺呮粍鏅ユ繝鐢靛仜閻°劑宕垫惔銊ョ９婵犻潧顑呴拑鐔兼煏閸繍妲哥紒鐙欏洦鐓欑紒瀣健椤庢霉閻樿櫕灏﹂柟顔筋殔閳绘捇宕归鐣屼邯闂備胶绮悧婊堝储瑜旈幃楣冩倻閼恒儱浜楅柟鐓庣摠钃辨い顐㈢Т閳规垿鍩ラ崱妤冧户闁荤姭鍋撻柨鏇炲€归崐鐢碘偓鐟板婢瑰寮ㄦ禒瀣厽闁归偊鍓氶埢鏇㈡煕閵堝洤鏋庨柍瑙勫灴椤㈡瑩鎮欓浣圭槑闂備礁鎽滄慨鎾晝閵堝鏁囧┑鍌溓归悙濠囨煏婵炲灝鍔氱紒銊ｅ劜缁绘繈鎮介棃娑掓瀰濠电偘鍖犻崶鑸垫櫈闂佺硶鍓濈粙鎴犲婵犳碍鐓忓┑鐐靛亾濞呭棝鏌嶉柨瀣诞闁哄本鐩、鏇㈠Χ閸涱喚浜栭梻浣哥－椤戞洟宕曢悽绋胯摕婵炴垶菤閺€浠嬫煕閵夈劌鐓愰柣锝堟硾椤啴濡舵惔婵堢泿闂佸摜鍠庨悺銊︾┍婵犲洤閱囬柡鍥╁仜閼板灝鈹戦埥鍡楃仯闁冲嘲鐗嗗嵄闁规鍠氱壕浠嬫煕鐏炲墽鎳呴悹鎰嵆閺屾盯鎮╁畷鍥р吂闂傚洤顦甸弻锝夊箣閿濆棭妫勭紓浣插亾闁糕剝绋掗悡娆撴煟閹寸倖鎴犱焊閻㈢數纾奸柍閿亾闁稿鎸荤换婵嗏枔閸喗鐏嶉梺鎸庢磵閺呯姴鐣峰⿰鍐ｆ闁靛繆鏅滈弲鈺呮⒑閸︻厼鍔嬫い銊ユ閸╂盯骞嬮悩鍐叉瀾闂佺粯顨呴悧鍡樼┍椤栫偞鐓涢柛鈩兠粭褏绱掓潏銊ユ诞闁糕斁鍋撳銈嗗笂閻掞附鍒婄€靛摜纾兼い鏍ㄧ⊕缁€鍐煟韫囧海绐旀慨濠冩そ濡啫霉閵夈儳澧︾€殿喗褰冮オ浼村醇濠靛牆骞堥梻浣侯攰閹活亪姊介崟顖氱；闁斥晛鍟扮粻楣冩煕閳╁喚娈樼紒鐘靛閵囧嫯绠涙繝鍐╃彋闂佸搫鏈惄顖氼嚕椤曗偓瀹曞爼濡歌閳ь剚鍔欏铏光偓鍦濞兼劙鏌涢妸銉﹀仴闁靛棔绀侀埢搴ㄥ箻閸忓懐鐐婇梻浣告啞濞插繘宕濈仦鍓ь洸闁绘劦鍓涚弧鈧梻鍌氱墛缁嬫帡鏁嶅澶嬬厱闁靛牆妫欑粈瀣煛瀹€瀣？濞寸媴濡囬幏鐘诲箵閹烘埈娼涚紓鍌氬€烽懗鑸垫叏閻戣棄绠犻柟鍓х帛缁犳帡姊绘担鐟邦嚋缂佽鍊块獮濠呯疀閺囩喎顏搁梺璺ㄥ枔婵敻鎮￠弴鐔虹闁糕剝顨堢粻鍙夈亜閵夛絽鐏柟渚垮妽缁绘繈宕ㄩ鍛摋缂傚倷娴囨ご鍝ユ暜閹烘洜浜介梻浣哄帶椤洟宕愰妶鍛傦綁顢楅崟顑芥嫽婵炶揪绲介幊娆掋亹閹烘挻娅囬梺闈涚墕閹锋垿鎳撻崸妤佲拺妞ゆ巻鍋撶紒澶嬫尦閹偤宕归鐘辩盎闂佺懓鎼粔鐑藉礂瀹€鍕厱闁绘柨鎽滈崣鈧梺鍝勭灱閸犳牕鐣峰▎鎾澄ч柛鈩冾殢娴硷拷
    u32_t F;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ゆ繝鈧柆宥呯劦妞ゆ帒鍊归崵鈧柣搴㈠嚬閸欏啫鐣峰畷鍥ь棜閻庯絻鍔嬪Ч妤呮⒑閸︻厼鍔嬮柛銊ョ秺瀹曟劙鎮欏顔藉瘜闂侀潧鐗嗗Λ妤冪箔閹烘挶浜滈柨鏂跨仢瀹撳棛鈧鍠楅悡锟犮€侀弮鍫濋唶闁绘棁娓归悽缁樼節閻㈤潧孝闁挎洏鍊濆畷顖炲箥椤斿彞绗夌紓鍌欑劍閿曗晛鈻撴禒瀣厽闁归偊鍘界紞鎴︽煟韫囨梹缍戦柍瑙勫灴椤㈡瑩鎮锋０浣割棜闂傚倸鍊风欢姘焽瑜旈幃褔宕卞☉妯肩枃闂佽澹嗘晶妤呭磻椤忓牊鐓冪憸婊堝礈濮橆厾鈹嶅┑鐘叉处閸婇攱銇勮箛鎾愁仱闁稿鎹囧浠嬧€栭浣衡姇闁瑰嘲鎳橀幊鏍р攽閸モ晜鍒涢悗瑙勬礈閸犳牠銆佸Δ浣瑰缂佸鐏濋煢濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柡澶嬪殮濞差亝鏅濋柛灞炬皑閸橀亶姊洪悷閭﹀殶濠殿喚鏁诲畷浼村箛閻楀牏鍘藉┑掳鍊愰崑鎾绘煟濡も偓濡稓鍒掗銏犵闁圭偨鍔庨幊鎾汇偑娴兼潙鐒垫い鎺戝绾偓闂佽鍎煎Λ鍕嫅閻斿摜绠鹃柟瀛樼懃閻忣亪鏌涚€ｎ偄濮嶉柡宀€鍠愰敍鎰媴閸濆嫬顬夐梻浣风串缁蹭粙鎮ラ悡搴綎闁绘垶锚椤曡鲸绻涢崱妯虹仸闁稿瑪鍥ㄢ拺缂佸娉曠粻鏌ユ煥濮橆厹浜滈柡鍥朵簽缁嬭崵绱掔紒妯肩畵妞ゎ偅绻堥、鏍煘閸喖顫囬梺鍝勮閸旀垵顕ｉ鈧崺鈧い鎺戝绾惧潡鏌熼崜浣规珪鐎规挷鑳堕埀顒€绠嶉崕閬嵥囨导鏉戠厱闁硅揪闄勯悡鏇熺節闂堟稒顥滄い蹇撶摠娣囧﹪宕ｆ径瀣偓鎰版煛瀹€瀣埌閾绘牠鏌嶈閸撴瑨鐏嬪┑鐐叉閹告悂鎮挎ィ鍐╃厾闁告縿鍎查弳鈺傘亜閳哄啫鍘撮柟顔筋殜閺佹劖鎯旈垾鑼泿濠电偛顕慨鎾Χ閹间礁钃熼柍鈺佸暙缁剁偛鈹戦悩鎻掝仼妞わ絽鐖煎娲川婵犲啰鍙嗙紓浣割槺閹虫捇鎮鹃悜鑺ユ櫜闁割偁鍨婚弶绋库攽閻愭潙鐏﹂柣鐔村劜缁傛帡顢橀悙鈺傛杸闂佺粯鍔曞鍫曀夊⿰鍛＜缂備焦锚婵秵銇勯姀鈩冾棃妞ゃ垺娲熼弫鍌炴偩鐏炶棄绠炲┑鐘垫暩閸嬫稑螞濡ゅ懏鍤愭い鏍ㄧ〒椤╂彃螖閿濆懎鏆為柍閿嬪浮閺屾稓浠﹂崜褎鍣銈忚缁犳捇寮婚悢鍝勬瀳闁告鍋橀崰濠囨⒑鐠団€崇仩闁哄牜鍓欓銉╁礋椤愩倖娈曢梺閫炲苯澧扮紒顔肩墛缁绘繈宕戦姘辩Ш闁轰焦鍔欏畷銊╊敇瑜庨ˉ鎴︽⒒娴ｈ鍋犻柛濠冪墪鐓ゆ慨妞诲亾濠碉紕鏁诲畷鐔碱敍濮橆剙绁繝寰锋澘鈧劙宕戦幘瓒佺懓饪伴崼銏㈡毇闂佽鍠楅〃鍫ュ箟閹绢喖绀嬫い鎺戝亞濡蹭即姊哄Ч鍥х労闁搞劏浜弫顕€鏁撻悩铏珳闂佺粯鍔栫粊鎾绩娴犲鍊甸柨婵嗘噽娴犳盯鏌￠崨顖氫槐闁哄矉缍侀幃鈺呭礂閸涙澘鐒婚梻浣告啞閺屻劑鎯夐懖鈺佸灊妞ゆ挶鍨洪弲顒勬煕閺囥劌骞橀柨娑欑箞濮婅櫣绮欓幐搴㈡嫳闂佺厧缍婄粻鏍春閳ь剚銇勯幒宥囧妽闁稿﹥鍔楅埀顒冾潐濞叉﹢宕濆▎鎾崇畺婵犲﹤鐗婇崵宥夋煏婢跺牆鍔滈柣锝変憾濮婄粯鎷呴挊澶夋睏闁哄浜弻锝堢疀閺傚灝鎽甸悗瑙勬穿缂嶁偓缂佺姵绋戦埥澶娾枎閹存繂绠版繝鐢靛О閸ㄧ厧鈻斿☉銏╂晞闁告稑鐡ㄩ崐鍨旈敐鍛殲闁绘挻娲熼弻鏇㈠醇濠靛浂妫＄紓浣插亾閻庯綆鍠楅悡娑㈡倶閻愭彃鈷旀繛鎻掔摠閵囧嫰濮€閳╁啰顦伴梺杞扮劍閸旀瑥鐣烽崡鐐╂瀻婵☆垱浜跺顓犵瘈缁剧増锚婢у弶銇勯妸銉﹀殗妤犵偛锕よ灒濞撴凹鍨辩紞搴♀攽閻愬弶鈻曞ù婊勭矊椤斿繐鈹戦崱蹇旀杸闂佺粯蓱瑜板啴寮冲▎鎰╀簻闁挎棁顫夊▍濠冩叏婵犲啯銇濇鐐寸墵閹瑩骞撻幒婵堚偓鏉戔攽閻橆喖鐏柟铏尭铻炴繝闈涱儏缁犳牗銇勯弴妤€浜惧Δ鐘靛仜濡粓篓娓氣偓閺屾稑顫濋澶婂壈濡炪値鍙€濞夋洟骞戦崟顒傜懝妞ゆ牗鑹炬竟瀣⒒娴ｅ憡鍟為柨鏇樺灪缁傚秶鎹勬笟顖涚稁濠电偛妯婃禍婵嬎夐崼鐔虹闁硅揪缍侀崫娲极閸儲鈷掑ù锝堫潐閸嬬娀鏌涢弬鍧楀弰鐎殿噮鍋嗛幏鐘绘嚑椤掍焦顔曟俊鐐€栭悧婊堝磿閹版澘鏋佺€广儱顦伴悡鐔兼煙闁箑鐏犻柣銊︽そ閺屻劌鈽夊顒佺€诲銈庝簻閸熷瓨淇婇崼鏇炲窛妞ゆ牗绮犻崬鑸电節绾版ɑ顫婇柛瀣噽缁瑩骞樼€靛摜褰鹃梺鍝勬川閸犳捇銆呴悜鑺ュ€甸柨婵嗛娴滅偤鏌涘鈧禍鍫曞蓟閿濆牏鐤€闁哄洨鍋樼划鑸电節閳封偓閸屾粎鐓撳Δ鐘靛仜閿曨亜鐣烽妸鈺婃晩閺夌偞澹嗛惄搴ㄦ⒒娴ｇ懓顕滈柡灞诲姂婵″墎绮欏▎鍓р偓鑸点亜閺囨浜鹃梺鍝勬湰濞茬喎鐣烽幆閭︽Щ濡炪倕娴氶崜姘跺Φ閸曨垱鏅滈悹鍥у级閻濇繈姊洪崫鍕拱缂佸鎹囬崺鈧い鎺戯功缁夌敻鏌涚€ｎ亝鍣归柣锝夘棑娴狅箓鎮欓鈧鎸庣節閻㈤潧孝闁哥噥鍨舵俊闈涒攽鐎ｎ偆鍘靛銈嗘⒐閸庢娊宕㈢€电硶鍋撳▓鍨灈闁绘牕銈搁悰顕€骞囬鐔峰妳濡炪倖鏌ㄩ崥瀣汲韫囨稒鈷掗柛灞捐壘閳ь剛鍏橀幊妤呭醇閺囩偟鐤囬梺瑙勫礃椤曆囧触瑜版帗鐓涚€广儱楠搁獮鏍磼閻欌偓閸ㄥ爼寮婚妸鈺傚亞闁稿本绋戦锟�
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
#elif  1 //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ょ紓宥咃躬瀵鎮㈤崗灏栨嫽闁诲酣娼ф竟濠偽ｉ鍓х＜闁诡垎鍐ｆ寖闂佺娅曢幑鍥灳閺冨牆绀冩い蹇庣娴滈箖鏌ㄥ┑鍡欏嚬缂併劌銈搁弻鐔兼儌閸濄儳袦闂佸搫鐭夌紞渚€銆佸鈧幃娆撳箹椤撶噥妫ч梻鍌欑窔濞佳兾涘▎鎴炴殰闁圭儤顨愮紞鏍ㄧ節闂堟侗鍎愰柡鍛叀閺屾稑鈽夐崡鐐差潻濡炪們鍎查懝楣冨煘閹寸偛绠犻梺绋匡攻椤ㄥ棝骞堥妸鈺傚€婚柦妯侯槺閿涙盯姊虹紒妯哄闁稿簺鍊濆畷鎴犫偓锝庡枟閻撶喐淇婇婵嗗惞婵犫偓娴犲鐓冪憸婊堝礂濞戞碍顐芥慨姗嗗墻閸ゆ洟鏌熺紒銏犳灈妞ゎ偄鎳橀弻宥夊煛娴ｅ憡娈查梺缁樼箖閻楃姴顫忕紒妯肩懝闁逞屽墴閸┾偓妞ゆ帒鍊告禒婊堟煠濞茶鐏￠柡鍛埣瀵泛鈻庨崜褍鏁搁梺鑽ゅ枑閻熴儳鈧凹鍘剧划鍫⑩偓锝庡枟閻撴瑦銇勯弮鈧崕鎶藉储鐎涙﹩娈介柣鎰嚋閺€鑽ょ磼缂佹绠炵€规洘甯掗オ浼村川閸涱偄濮傛慨濠呮閹风娀鍨鹃搹顐や簮闂備礁鎼幊蹇涙儎椤栫偑鈧礁鈻庨幒鏃傛澑濠电偞鍨兼ご鎼佸疾閳哄懏鈷戠紓浣股戠亸顓炍旈悩宕囨憙闁诲繐娲ㄧ槐鎾诲磼濮橆兘鍋撻幖浣哥９闁归棿绀佺壕褰掓煙闂傚顦︾痪鎯х秺閺屾稑鈹戦崟顐㈠Б闂佹椿鍘介幑鍥蓟閿濆顫呴柣妯哄暱閺嗗牓姊虹粙娆惧剰妞ゆ垵顦靛濠氭晸閻樿尙鍊為梺鎸庣箓濡鈻嶅鑸碘拺闂侇偆鍋涢懟顖涙櫠閹€鏀介柍銉ㄦ珪閸犳ɑ顨ラ悙鏉戠伌鐎规洜鍠栭、娑樞掔亸鏍ㄦ珚闁哄被鍔戝鏉懳旈埀顒佺閸撗呯＜闁告挆鍡橆€楅梺鎼炲姀濞夋盯锝炶箛娑欐優闁革富鍘鹃敍婊冣攽閳藉棗鐏ユ繛鍜冪稻缁傛帡骞栨担鍦弮闂佸憡鍔﹂崰鏇㈠箟妤ｅ啯鐓涚€光偓閳ь剟宕伴弽顓炵畺婵犲﹤鐗嗙粻锝夋煙閻戞ɑ灏柡瀣ㄥ€栫换婵嗏枔閸喗鐏堥梺鎸庢磸閸斿矂鍩㈠澶嬫櫜闁搞儜鍐喊婵犵數鍋涘Λ娆撳垂鐠鸿櫣鏆﹀鑸靛姈閻撳繐鈹戦悙鎴濆暢鐎氫即鏌℃担钘変汗缂佽鲸鎹囧畷鎺戭潩椤戣棄浜惧瀣椤愪粙鏌ㄩ悢鍝勑㈢紒鎰殕娣囧﹪濡堕崨顔兼缂備胶濮甸悡锟犲蓟濞戙垹唯闁瑰瓨绻嗙€氱増绻涢崼娑樺缂佺粯绻堥幃浠嬫濞磋缍侀弻銈夊捶椤撶偘澹曢梺绋垮瘨閸嬪﹤顫忕紒妯诲缂佸瀵уВ鎰版⒑閸︻厸鎷￠柛瀣工閻ｇ兘骞囬弶鍨獩婵犵數濮寸€氬嘲煤閹间焦鈷戠紓浣姑慨澶愭煕鎼存稑鈧繈宕哄☉銏犵闁绘鏁搁敍婊堟煟鎼搭垳绉甸柛瀣椤㈡艾饪伴崟顓犵槇闂侀€炲苯澧存鐐茬Ч椤㈡瑩宕滆缁辨煡姊虹拠鎻掑毐缂傚秴妫濆畷鎴﹀礋椤栨稑鈧爼鎮楀☉娅辨粍绂嶅⿰鍫熺厪闊洤锕ゆ晶鍙夈亜閵夈儳澧﹂柡灞稿墲閹峰懐鎲撮崟顐わ紦闂備浇妗ㄩ悞锕傚箲閸ヮ剙鏋侀柟鍓х帛閺呮悂鏌ㄩ悤鍌涘 濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌涘☉姗堟敾闁告瑥绻橀弻锝夊箣閿濆棭妫勯梺鍝勵儎缁舵岸寮诲☉妯锋婵鐗婇弫楣冩⒑閸涘﹦鎳冪紒缁橈耿瀵鏁愭径濠勵吅闂佹寧绻傚Λ顓炍涢崟顖涒拺闁告繂瀚烽崕搴ｇ磼閼搁潧鍝虹€殿喖顭烽幃銏ゅ礂鐏忔牗瀚介梺璇查叄濞佳勭珶婵犲伣锝夘敊閸撗咃紲闂佽鍨庨崘锝嗗瘱缂傚倷绶￠崳顕€宕归幎钘夌闁靛繒濮Σ鍫ユ煏韫囨洖啸妞ゆ挸鎼埞鎴︽倷閸欏妫炵紓浣虹帛鐢绮嬮幒鎾卞亝闁告劏鏂侀幏铏圭磽娴ｅ壊鍎愭い鎴炵懇瀹曟洖顓兼径瀣幈婵犵數濮撮崯鎵不閻愮鍋撳▓鍨灈妞ゎ厼鍢查锝夊箻椤旇棄浜滈梺鎯х箺椤曟牠宕惔銊︾厽闁绘柨鎽滈惌濠勭磼婢跺本鍤€闁伙絿鍏橀、娑㈡倻閸℃ɑ娅囬梻浣瑰濡礁螞閸曨垰鐒垫い鎺戝暞绾箖鏌嶇憴鍕伌闁诡喒鏅犲畷锝嗗緞婵犲孩袩闂傚倷鐒︽繛濠囧绩闁秴鍨傞柛褎顨呴拑鐔兼煟閺冨洦顏犻柣顓熺懇閺岀喓绮欓崹顕呭妷閻庤娲栫壕顓犳閹惧瓨濯撮柛婵嗗珔閿濆鐓欑紒瀣儥閻撶厧鈹戦敍鍕毈鐎规洜鍠栭、娑樷槈濞嗘劗褰嗛梻浣藉吹婵潙煤閳哄啩鐒婃繛鍡樺姉椤╅攱銇勯弽銊х煂缁炬崘妫勯妴鎺戭潩椤掍焦鎮欓梺鍝勵儐缁嬫帡濡甸崟顖ｆ晣闁绘ɑ褰冮獮瀣⒑闂堟稒顥欑紒鈧笟鈧崺銉﹀緞閹邦剦娼婇梺鐐藉劜閺嬪ジ宕戦幘缁樺€婚柤鎭掑劚閳ь剛鏁婚弻锝夊閵忊槅浠梺鑽ゅ枛閸嬪﹪宕甸弴銏＄厵闁诡垱婢樿闂佹娊鏀辩敮鎺楁箒闂佹寧绻傞悧濠囶敂閻樼粯鍋ㄦい鏍ㄧ〒閳洜绱掔紒妯兼创鐎规洩绲惧鍕沪鐟欙絾瀚查梺璇叉唉椤煤濮椻偓瀹曟繂鈻庤箛鏇熸闂佺粯鍨归悺鏃堝极閸ャ劎绠鹃柟瀵镐紳閸忚偐鐝堕柡鍥╁亹閺€鑺ャ亜閺冨倻鎽傞柣鎺嶇矙閺屾稓鈧綆鍋呯亸鐢告煃瑜滈崜姘舵偄椤掑嫬绠柨鐕傛嫹

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


           