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
//闂傚倷娴囬妴鈧柛瀣崌閹綊宕崟顒佸創闂佺粯鎸搁崐鍨潖濞差亶鏁冮柕蹇婃濡偤鎮峰⿰鍕凡妞わ箓娼ч悾鐑藉箮閽樺）鈺呮煥閺冨洤袚妞ゆ柨绉瑰铏规兜閸滀礁娈愰梺鍝ュУ閻楁粓鎳為柆宥嗗殤妞ゆ帒鍊块埞蹇涙⒑鐠恒劌娅愰柟鍑ゆ嫹
static u8_t anti_ack(Device_t *dev)
{

    DeviceCmd_t *cmd;
	uint8_t len = 0;
    cmd = (DeviceCmd_t *)dev->cmdList.node->payload;
    APP_DEBUG("anti ack buf:\r\n");
    APP_DEBUG("cmd->ack.payload[0]=%d",cmd->ack.payload[0]);
    //print_buf(cmd->ack.payload, cmd->ack.lenght);
    
    if(cmd->ack.payload[0]==METER_ADDR){
        //闂傚倷娴囬妴鈧柛瀣崌閹綊宕崟顒佸創闂佺粯鎸搁崐鍧楀蓟閿濆缍栨い鎾跺Х閵堢兘姊洪棃娑欘棞闁哥喐娼欓悾鐑藉醇閺囩喎鈧鈧懓澹婇崰妞剧昂
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倸鍊烽悞锕併亹閸愵亞鐭撻柣銏⑶归拑鐔封攽閻樺弶澶勯柛瀣ㄥ姂閹宕烽鐑嗏偓宀勬煕閵堝洤浠遍柟顔筋殔閳藉鈻嶉搹顐㈢伌鐎殿噮鍓氶幏鍛喆閸曨厾肖闂備浇顫夐崕鎶藉疮閸噮鍤曢柟鎯板Г閻撴瑧绱掔€ｎ偄顕滈柣顓燁殜閺岀喖鐓幓鎺嗗亾濠靛绠氶柛鎰靛枛缁€瀣亜閹扳晛鐏柡鍡愬劦閺岋絾鎯旈姀鐘叉灆闂佺懓鍤栭幏锟�
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    //dev->cfg = null;  // 闂傚倸鍊烽悞锕€顭垮Ο鑲╃煋闁割偅娲橀崑顏堟煕閳╁喚鐒介柍缁樻閺屽秷顧侀柛鎾存皑缁瑦寰勯幇鍨櫆闂佸憡渚楅崢鎼佸箰閸愵喗鐓熼柣鎰嚟閳藉鏌ｉ婵堢獢妞ゃ垺鑹鹃～婵堟崉閾忚鍞堕梻浣规偠閸庡姊介崟顒佸弿闁靛繈鍊栭悡鐔兼煏婵炲灝鍔氭い蹇ｅ墯閵囧嫰寮埀顒€煤濡警鍤楅柛鏇ㄥ灡閺呮悂鏌ㄩ悤鍌涘
    if (ModbusDevice.cfg == null) {
          dev->cfg = (ST_UARTDCB *)ModbusDevice.head->hardCfg;
        } else {
          dev->cfg = ModbusDevice.cfg;
        }
    
    
    dev->callBack = anti_ack;  // 闂備浇宕垫慨宕囩矆娴ｈ娅犲ù鐘差儐閸嬵亪鏌涢埄鍐姇闁绘帒顭烽悡顐﹀炊瑜庨幑锝夋煟閹邦剨韬柟顔款潐閹峰懘鎮烽弶鍨戞俊鐐€х€靛矂宕滈悢鑲烘稒绗熼埀顒勫春閳ь剚銇勯幒鎴濃偓褰掑疮閸濆娊褰掓晲閸パ冨闂佷紮缍佹禍璺侯嚕閸洖鐓涢柛灞剧〒閸欏棗鈹戦悙鎻掔骇闁绘濮撮悾宄扳枎閹炬潙娈熼梺闈涱檧闂勫嫰顢欓敓锟�
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂備浇宕垫慨鎶芥倿閿曞倸纾块柟璺哄閸ヮ剦鏁嗛柛鏇ㄥ亞椤旀劙姊洪崨濠勭畵閻庢凹鍣ｅ顐﹀Ω閳哄倻鍘电紓浣割儐鐎笛囨儗濡ゅ懏鐓欓棅顒佸絻閳ь剚绻堥獮濠囨倷閸濆嫮锛滃┑鐘诧工閸熲晠宕犻敓锟�
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    Device_inset(dev);    // 闂備浇顕х换鎰崲閹邦儵娑樷攽閸℃劏鍋撻幒妤€绀堝ù锝囨嚀閺嗩偅绻涙潏鍓ф偧闁硅櫕鎹囬崺娑㈠籍閸喎浠╁┑鐐村灦钃辨繝鈧导瀛樺€甸柛顭戝亞椤ｈ尙绱掗崒娑樻诞濠碘€崇埣瀹曞爼濡歌濞堟悂姊洪懡銈呅￠柣鎺炵畵瀹曟洟骞庨崜鍨喘椤㈡宕熼銈呮憢闂備浇顕栭崹搴ㄥ礃閵婏箒绶evicelist闂傚倷鐒︾€笛呯矙閹寸偟闄勯柡鍌涱儥濞尖晠鏌ㄩ弴鐐测偓褰掑磻閵娾晜鐓熸い蹇撳閸旀iceCmdSend婵犵數濮伴崹鐓庘枖濞戞埃鍋撳鐓庢珝妤犵偛鍟撮弫鎾绘晸閿燂拷
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
    cmd->cmd.payload = memory_apply(cmd->cmd.size);  // 闂傚倸鍊烽悞锕併亹閸愵亞鐭撻柣銏⑶归拑鐔封攽閻樺弶澶勯柛瀣ㄥ姂閹宕烽鐑嗏偓宀勬煕閵堝洤浠遍柟顔筋殔閳藉鈻嶉搹顐㈢伌鐎殿噮鍓氶幏鍛喆閸曨厾肖闂備浇顫夐崕鎶藉疮閸噮鍤曢柟鎯板Г閻撴瑧绱掔€ｎ偄顕滈柣顓燁殜閺岀喖鐓幓鎺嗗亾濠靛绠氶柛鎰靛枛缁€瀣亜閹扳晛鐏柡鍡愬劦閺岋絾鎯旈姀鐘叉灆闂佺懓鍤栭幏锟�
    r_memcpy(cmd->cmd.payload, data_ptr, cmd->cmd.size);

    APP_PRINT("cmd.payload:");
    print_buf(cmd->cmd.payload,cmd->cmd.size);

    dev->cfg = (ST_UARTDCB *)&UART_9600_N1;  
       
    dev->callBack = anti_ack;  // 闂備浇宕垫慨宕囩矆娴ｈ娅犲ù鐘差儐閸嬵亪鏌涢埄鍐姇闁绘帒顭烽悡顐﹀炊瑜庨幑锝夋煟閹邦剨韬柟顔款潐閹峰懘鎮烽弶鍨戞俊鐐€х€靛矂宕滈悢鑲烘稒绗熼埀顒勫春閳ь剚銇勯幒鎴濃偓褰掑疮閸濆娊褰掓晲閸パ冨闂佷紮缍佹禍璺侯嚕閸洖鐓涢柛灞剧〒閸欏棗鈹戦悙鎻掔骇闁绘濮撮悾宄扳枎閹炬潙娈熼梺闈涱檧闂勫嫰顢欓敓锟�
   // dev->callBack = NULL;  
    //dev->explain = esp;   // 闂備浇宕垫慨鎶芥倿閿曞倸纾块柟璺哄閸ヮ剦鏁嗛柛鏇ㄥ亞椤旀劙姊洪崨濠勭畵閻庢凹鍣ｅ顐﹀Ω閳哄倻鍘电紓浣割儐鐎笛囨儗濡ゅ懏鐓欓棅顒佸絻閳ь剚绻堥獮濠囨倷閸濆嫮锛滃┑鐘诧工閸熲晠宕犻敓锟�
    dev->type = DEVICE_ARTI;       //DEVICE_TRANS;

    list_init(&dev->cmdList);
    list_bottomInsert(&dev->cmdList, cmd);
    //Device_inset(dev);    // 闂備浇顕х换鎰崲閹邦儵娑樷攽閸℃劏鍋撻幒妤€绀堝ù锝囨嚀閺嗩偅绻涙潏鍓ф偧闁硅櫕鎹囬崺娑㈠籍閸喎浠╁┑鐐村灦钃辨繝鈧导瀛樺€甸柛顭戝亞椤ｈ尙绱掗崒娑樻诞濠碘€崇埣瀹曞爼濡歌濞堟悂姊洪懡銈呅￠柣鎺炵畵瀹曟洟骞庨崜鍨喘椤㈡宕熼銈呮憢闂備浇顕栭崹搴ㄥ礃閵婏箒绶evicelist闂傚倷鐒︾€笛呯矙閹寸偟闄勯柡鍌涱儥濞尖晠鏌ㄩ弴鐐测偓褰掑磻閵娾晜鐓熸い蹇撳閸旀iceCmdSend婵犵數濮伴崹鐓庘枖濞戞埃鍋撳鐓庢珝妤犵偛鍟撮弫鎾绘晸閿燂拷
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
* introduce:   闂傚倸鍊搁崐鎼佸磹閹间礁纾瑰瀣椤愪粙鏌ㄩ悢鍝勑㈢痪鎯ь煼閺屾盯寮撮妸銉㈠亾閻樻祴妲堟俊顖涚矋濡啫鐣烽悢鐓庣厸闁逞屽墮鏁堟俊銈勮兌缁♀偓闂侀潧楠忕徊鍓ф兜閻愵兙浜滈柟瀛樼箖椤ョ偟绱掗崒姘毙у┑陇鍩栧鍕偓锝庡墮楠炴劙姊绘担鍛靛綊寮甸鍌滅煓闁硅揪瀵岄弫瀣節婵犲倹鍣界痪鎯с偢閺屽秷顧侀柛鎾跺枛瀹曟椽鍩€椤掍降浜滈柟鐑樺灥椤忣亪鏌嶉柨瀣伌鐎殿喖鐖奸幃娆撳级閹搭厸鍋撳⿰鍫熺厱闊洦鎹佺€氱増銇勯鈥冲姷妞わ箒娅曢妵鍕Ω閵夛箑娈楅梺璇″櫍缁犳牕鐣疯ぐ鎺濇晝闁挎繂鎷嬮埀顒佹崌濮婃椽鎳￠妶鍛€炬繝銏㈡嚀濡稓鍒掗敐澶嬪€烽柣鎴炃氶幏娲⒑閸涘﹦鈽夐柨鏇樺劜瀵板嫰宕熼娑氬幈濠碘槅鍨板﹢閬嶆儗濞嗘垟鍋撶憴鍕闁告梹鐟╁顐﹀箻缂佹ɑ娅㈤梺璺ㄥ櫐閹凤拷     
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
        device_addr=online_dev_addr_tab[addr_index];    //闂佽娴烽弫鍛婃櫠濡ゅ懌鈧啴宕ㄩ弶鎴炴К閻庡厜鍋撻柍褜鍓熼獮澶愭偋閸喎顎撶紓浣割儓濞夋洖鐣濆☉銏♀拻濞撴艾娲ゆ禍妤呮煠鐎圭姵纭鹃柍缁樻崌楠炲鏁傞悾灞藉Х闂備胶纭堕崜婵嬨€冮崨顖滅濞撴埃鍋撻柡灞剧洴閺佹劙宕橀妸銉€虫繝鐢靛Л閸嬫捇鏌熺紒銏犳灈闂佽￥鍊濋弻宥堫檪闁瑰嚖鎷�
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
    //r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐宄懊归崶顒夋晪鐟滃秹婀侀梺缁樺灱濡嫰寮告笟鈧弻娑樷槈濮楀牊鏁惧┑鐐叉噽婵灚绌辨繝鍥ч柛娑卞枛濞咃繝姊洪柅鐐茶嫰婢ь噣鏌涢悢鍛婄稇闁伙絿鍏樻俊鎼佸煛娴ｆ椽鐛撻梻浣烘嚀椤曨參宕戦悙鍨涘彚闂傚倷娴囬褎顨ラ幖浣稿偍婵犲﹤鐗嗙壕濠氭煙閸撗呭笡闁绘挻娲熼弻锟犲炊閵夈儱顬堟繝鈷€灞芥灈闂囧鏌ｅ鈧褔宕濈€ｎ剛纾肩紓浣诡焽閳洘绻涢悡搴ｇ鐎规洘绮忛ˇ鏌ユ煕閳轰胶鐒告慨濠冩そ瀹曨偊濡烽妷锔锯偓顓㈡⒑閸涘﹥鈷愰柛銊ョ仢閻ｅ嘲煤椤忓嫮鍔﹀銈嗗笂闂勫秵绂嶅⿰鍫熺厵闁告繂瀚ˉ婊兠瑰⿰鍫㈢暫婵﹥妞藉畷顐﹀Ψ閵夛妇鈧儳鈹戦悙鎻掓倯闁荤啿鏅犻妴浣割潩閼稿灚娅滄繝銏ｅ煐钃遍柡鍛箞濮婃椽骞愭惔銏╂⒖濠碘槅鍋勭€氼厾绮嬪澶嬬劶鐎广儱妫岄幏娲⒑閸涘﹥灏扮憸鏉垮暞缁傚秴鈹戦崼銏紲闁荤姴娲﹁ぐ鍐敂椤撶偐鏀介柍銉ョ－閸╋綁鏌℃担瑙勫磳闁轰焦鎹囬弫鎾绘晸閿燂拷
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
                //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ巻鍋撴潻婵嬫⒑闁偛鑻晶鎾煛鐏炲墽銆掗柍褜鍓ㄧ紞鍡涘磻閸涱厾鏆︾€光偓閸曨剛鍘靛銈嗘閸嬫劗绮旈搹鍏夊亾鐟欏嫭澶勯柛瀣工閻ｇ兘鎮㈢喊杈ㄦ櫌婵炶揪绲挎灙濠㈢懓绉瑰濠氬磼濞嗘帒鍘＄紓渚囧櫘閸ㄥ爼鐛幇鏉跨濞达絽鎽滈敍娆愮節閻㈤潧校闁煎綊绠栭幃鐐哄垂椤愮姳绨婚梺鐟版惈缁夊墎鎷规导瀛樼厱闁绘棃顥撶粻濠氭煛鐏炲墽娲撮柟顔规櫅閻ｇ兘宕惰閹蜂即鏌ｆ惔銈庢綈婵炲弶锕㈠畷婵嗏枎閹捐泛绁﹂梺鍝勭Р閸斿秵鍒婇幘顔界厽闁绘梻鈷堥弳鎺楁煕閹存粎鐭欐慨濠勭帛閹峰懐绮欏▎鐐棏闂備胶枪椤戝懘骞婃惔锝呭疾闂備礁鎼粙渚€宕戦崱妯尖枖鐎广儱鎮胯ぐ鎺撳亗閹艰揪绲鹃幉濂告⒑缂佹ɑ灏版繛鑼枛瀵寮撮悢椋庣獮濠碘槅鍨靛▍锝嗙缁嬪簱鏀介柨娑樺濡炬悂鏌涢悩宕囧⒌闁糕晝鍋ら獮瀣晜缂佹ɑ娅旈梻浣虹《閸撴繈鏁嬪銈呴獜閹凤拷
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
                        r_memcpy(send_buf,antibuf.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐宄懊归崶顒夋晪鐟滃秹婀侀梺缁樺灱濡嫰寮告笟鈧弻娑樷槈濮楀牊鏁惧┑鐐叉噽婵灚绌辨繝鍥ч柛娑卞枛濞咃繝姊洪柅鐐茶嫰婢ь噣鏌涢悢鍛婄稇闁伙絿鍏樻俊鎼佸煛娴ｆ椽鐛撻梻浣烘嚀椤曨參宕戦悙鍨涘彚闂傚倷娴囬褎顨ラ幖浣稿偍婵犲﹤鐗嗙壕濠氭煙閸撗呭笡闁绘挻娲熼弻锟犲炊閵夈儱顬堟繝鈷€灞芥灈闂囧鏌ｅ鈧褔宕濈€ｎ剛纾肩紓浣诡焽閳洘绻涢悡搴ｇ鐎规洘绮忛ˇ鏌ユ煕閳轰胶鐒告慨濠冩そ瀹曨偊濡烽妷锔锯偓顓㈡⒑閸涘﹥鈷愰柛銊ョ仢閻ｅ嘲煤椤忓嫮鍔﹀銈嗗笂闂勫秵绂嶅⿰鍫熺厵闁告繂瀚ˉ婊兠瑰⿰鍫㈢暫婵﹥妞藉畷顐﹀Ψ閵夛妇鈧儳鈹戦悙鎻掓倯闁荤啿鏅犻妴浣割潩閼稿灚娅滄繝銏ｅ煐钃遍柡鍛箞濮婃椽骞愭惔銏╂⒖濠碘槅鍋勭€氼厾绮嬪澶嬬劶鐎广儱妫岄幏娲⒑閸涘﹥灏扮憸鏉垮暞缁傚秴鈹戦崼銏紲闁荤姴娲﹁ぐ鍐敂椤撶偐鏀介柍銉ョ－閸╋綁鏌℃担瑙勫磳闁轰焦鎹囬弫鎾绘晸閿燂拷
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
                        //婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繐霉閸忓吋缍戦柛銊ュ€块弻锝夊箻瀹曞洤鍝洪梺鍝勵儐閻楁鎹㈠☉銏犵闁绘劘灏欓崝浼存⒑缁嬫鍎愰柟鍛婃倐閿濈偛鈹戠€ｎ偄浜楅柟鍏肩暘閸ㄦ槒銇愭惔顫箚闁靛牆娲ゅ暩闂佺ǹ顑嗛惄顖氱暦椤栫儐鏁嶆繝濠傚鎼村﹤鈹戦悩缁樻锭妞ゆ垵鎳橀幃娆愮節閸ャ劎鍘繝鐢靛€崘銊︽闁哥喐鎮傚濠氬磼濮橆兘鍋撻悜鑺ュ殑闁告挷鐒﹂崗婊堟煕椤愶絾绀€缂佲偓閸喓绡€闂傚牊渚楅崕宥夋煃瑜滈崜娆撳疮閺夋垹鏆﹂柕濞炬櫓閺佸秵绻濇繝鍌涘婵¤尙枪閳规垿鎮欑€涙ê闉嶉梺绯曟櫅閸熸潙鐣烽幋锕€绠荤紓浣诡焽閸樻悂鏌ｈ箛鏇炰粶濠⒀嗘鐓ら柟缁㈠枟閻撴稑霉閿濆洦鍤€濠殿喖绉堕埀顒冾潐濞叉﹢鏁冮姀鈥茬箚闁归棿绀佸敮濡炪倖妫冮弫顕€宕戦幘璇茬妞ゆ棁袙閹峰搫鈹戦鐐殌婵炲眰鍊濋幃鐐裁洪鍛幈闂侀潧艌閺呮瑩骞夐悙顒夋闁绘劖娼欐慨宥団偓瑙勬礃閿氭い鏂跨箻椤㈡瑩宕楃喊鍗炴偑闂傚倸鍊风欢姘焽瑜戞晶婵嬫⒑閸濆嫭鍣虹紒顔芥尭閻ｇ兘鎮界喊妯轰壕闁挎繂楠搁弸鐔兼煟閹惧瓨绀冮柕鍥у椤㈡﹢鎮㈡搴濇樊濠电偛鐡ㄧ划宥囨崲閸愵喖桅闁告洦鍨板Λ姗€鎮归幁鎺戝鐟滃酣骞堥妸锔剧瘈闁告劏鏂傛禒銏ゆ倵鐟欏嫭绀堥柛蹇旓耿閵嗕礁顫滈埀顒勫箖閵堝棙濯撮梺鍨儏閻撴洟姊婚崒姘偓鐑芥嚄閸洖绠犻柟鎹愵嚙閸氬綊鏌″搴″箹缂佺媭鍨堕弻銊╂偆閸屾稑顏�
                        
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
    currentDevice = list_nextData(&DeviceList, currentDevice);      //闂傚倸鍊搁崐宄懊归崶顒夋晪鐟滃秹婀侀梺缁樺灱濡嫰寮告笟鈧弻娑樷槈濮楀牊鏁惧┑鐐叉噽婵灚绌辨繝鍥ч柛娑卞枛濞咃繝姊洪柅鐐茶嫰婢ь噣鏌涢悢鍛婄稇闁伙絿鍏樻俊鎼佸煛娴ｆ椽鐛撻梻浣烘嚀椤曨參宕戦悙鍨涘彚闂傚倷娴囬褎顨ラ幖浣稿偍婵犲﹤鐗嗙壕濠氭煙閸撗呭笡闁绘挸鍟伴幉绋库堪閸繄顦梺缁樻閸嬫劕鐣垫笟鈧悡顐﹀炊閵婏妇锛曢梺绋款儐閹瑰洭寮幇顓熷劅闁斥晛鍟版禍顏嗙磽閸屾瑨鍏屽┑顕€娼ч～婵嬪Ω閳轰胶鐤呯紓浣割儏缁ㄩ亶宕奸鍫熺厱闊洦姊圭€氭洜绱撳鍕獢妤犵偛鍟粋鎺斺偓锝庝簻缁愭稒绻濋悽闈浶㈤悗姘煎墴閻涱喖顫滈埀顒€顫忕紒妯诲闁告稑锕ら弳鍫㈢磽娴ｇ瓔鍤欓柛濠傤煼閸┾偓妞ゆ帊绀侀崵顒勬煕閹捐泛鏋涚€规洘妞介崺鈧い鎺嶉檷娴滄粓鏌熼崫鍕棞濞存粍鍎抽—鍐Χ鎼粹€崇闂佹悶鍨洪悡锟犲箖妤ｅ啯鍊婚柦妯侯槺椤撴椽姊洪幐搴㈩梿濠殿喓鍊曢埢宥夊閵堝棌鎷洪柣鐘充航閸斿苯鈻嶉幇鐗堢厵闁告垯鍊栫€氾拷
    if(NULL != currentDevice){
        currentCmd = list_nextData(&currentDevice->cmdList, currentCmd);    //闂傚倸鍊搁崐宄懊归崶顒夋晪鐟滃秹婀侀梺缁樺灱濡嫰寮告笟鈧弻娑樷槈濮楀牊鏁惧┑鐐叉噽婵灚绌辨繝鍥ч柛娑卞枛濞咃繝姊洪柅鐐茶嫰婢ь噣鏌涢悢鍛婄稇闁伙絿鍏樻俊鎼佸煛娴ｆ椽鐛撻梻浣烘嚀椤曨參宕戦悙鍨涘彚闂傚倷娴囬褎顨ラ幖浣稿偍婵犲﹤鐗嗙壕濠氭煙閸撗呭笡闁绘挻娲熼弻鐔煎级閸噮鏆㈢紓浣戒含婢ф骞堥妸褎鍠嗛柛鏇炵仛閻や礁鈹戦纭烽練婵炲拑缍侀獮澶愬箻椤旇偐顦板銈嗗姂閸ㄧ顣介梻鍌氬€风粈渚€骞楀⿰鍫濈獥閹肩补妲呴悞浠嬫⒔閸ヮ剚鍎楀Δ锝呭暞閳锋垿鏌涘☉姗堝姛闁宠棄顦甸弻锝嗗箠闁告梹鍨甸悾鐑藉箣閿旇姤娅滈梺鍛婂搸閸婃洜绱炴繝鍥ф瀬闁圭増婢橀悙濠囨煏婵炲灝鍔滈柣锝庡墴濮婄粯鎷呴崨濠傛殘闂佸搫鐭夌紞浣哥暦閹版澘鍐€妞ゆ挾鍊ｉ敃鍌涚厱闁哄洢鍔岄悘锟犳煛閸涱喚鐭掗柟顔肩秺瀹曞爼濡搁妷褏銈烽梻浣稿暱閸㈡煡顢栨径濠庢綎闁惧繗顫夌€氭岸鏌嶉妷銉э紞濞寸姭鏅犻弻锝嗘償閵忕姴姣堥梺鍝ュУ瀹€绋跨暦濮樿泛绠抽柟瀛樻⒐閻庡姊洪悷閭﹀殶闁稿绋撴竟鏇㈩敍閻愮补鎷洪梻鍌氱墛閼冲棜銇愰幒鎴狅紵闂佽鍎煎Λ鍕ч弻銉︾厸闁告劑鍔庢晶娑㈡煃闁垮绗掗棁澶愭煥濠靛棙鍣洪悹鎰ㄥ墲缁绘繈鍩€椤掍胶顩烽悗锝庡亞閸橀亶姊洪崫鍕偓钘夆枖閺囥垺鍊堕梺顒€绉甸悡鍐喐濠婂牆绀堟慨妯挎硾缁€鍕喐閻楀牆绗掗柛灞诲妼铻栭柨婵嗘噹閺嗘瑩鏌ｉ幒鎴犱粵闁靛洤瀚伴獮鎺楀幢濡炴儳顥氶梻鍌欐祰椤曆呮崲閹存繍娼╅柕濞炬櫅缁愭鈹戦悩鎻掍簽闁告艾顑夐弻娑㈠Ψ閵忊剝鐝栭梺缁樺笒濞尖€愁潖濞差亜鎹舵い鎾跺仜婵″搫顪冮妶鍐ㄥ婵☆偅绻傞锝夊蓟閵夈儳顔愭繛杈剧到濠€閬嶅矗閸℃稒鈷戠紓浣股戠粈鈧梺绋匡工濠€閬嶅焵椤掍胶鍟查柟鍑ゆ嫹  
        if(NULL != currentCmd){
            APP_DEBUG("get device data cmd success\r\n");
            print_buf(currentCmd->cmd.payload,sizeof(currentCmd->cmd.payload));
            
            modbus_wr2_t *modbus_buf=(modbus_wr2_t *)currentCmd->cmd.payload;
            
            if((modbus_buf->fun & MODBUS_RTU_FUN_ER) == MODBUS_RTU_FUN_ER){
                //闂傚倸鍊搁崐鎼佸磹閹间礁纾圭€瑰嫭鍣磋ぐ鎺戠倞妞ゆ巻鍋撴潻婵嬫⒑闁偛鑻晶鎾煛鐏炲墽銆掗柍褜鍓ㄧ紞鍡涘磻閸涱厾鏆︾€光偓閸曨剛鍘靛銈嗘閸嬫劗绮旈搹鍏夊亾鐟欏嫭澶勯柛瀣工閻ｇ兘鎮㈢喊杈ㄦ櫌婵炶揪绲挎灙濠㈢懓绉瑰濠氬磼濞嗘帒鍘＄紓渚囧櫘閸ㄥ爼鐛幇鏉跨濞达絽鎽滈敍娆愮節閻㈤潧校闁煎綊绠栭幃鐐哄垂椤愮姳绨婚梺鐟版惈缁夊墎鎷规导瀛樼厱闁绘棃顥撶粻濠氭煛鐏炲墽娲撮柟顔规櫅閻ｇ兘宕惰閹蜂即鏌ｆ惔銈庢綈婵炲弶锕㈠畷婵嗏枎閹捐泛绁﹂梺鍝勭Р閸斿秵鍒婇幘顔界厽闁绘梻鈷堥弳鎺楁煕閹存粎鐭欐慨濠勭帛閹峰懐绮欏▎鐐棏闂備胶枪椤戝懘骞婃惔锝呭疾闂備礁鎼粙渚€宕戦崱妯尖枖鐎广儱鎮胯ぐ鎺撳亗閹艰揪绲鹃幉濂告⒑缂佹ɑ灏版繛鑼枛瀵寮撮悢椋庣獮濠碘槅鍨靛▍锝嗙缁嬪簱鏀介柨娑樺濡炬悂鏌涢悩宕囧⒌闁糕晝鍋ら獮瀣晜缂佹ɑ娅旈梻浣虹《閸撴繈鏁嬪銈呴獜閹凤拷
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
                        r_memcpy(send_buf,currentCmd->cmd.payload,meter_rec_buf->bytes);    //闂傚倸鍊搁崐宄懊归崶顒夋晪鐟滃秹婀侀梺缁樺灱濡嫰寮告笟鈧弻娑樷槈濮楀牊鏁惧┑鐐叉噽婵灚绌辨繝鍥ч柛娑卞枛濞咃繝姊洪柅鐐茶嫰婢ь噣鏌涢悢鍛婄稇闁伙絿鍏樻俊鎼佸煛娴ｆ椽鐛撻梻浣烘嚀椤曨參宕戦悙鍨涘彚闂傚倷娴囬褎顨ラ幖浣稿偍婵犲﹤鐗嗙壕濠氭煙閸撗呭笡闁绘挻娲熼弻锟犲炊閵夈儱顬堟繝鈷€灞芥灈闂囧鏌ｅ鈧褔宕濈€ｎ剛纾肩紓浣诡焽閳洘绻涢悡搴ｇ鐎规洘绮忛ˇ鏌ユ煕閳轰胶鐒告慨濠冩そ瀹曨偊濡烽妷锔锯偓顓㈡⒑閸涘﹥鈷愰柛銊ョ仢閻ｅ嘲煤椤忓嫮鍔﹀銈嗗笂闂勫秵绂嶅⿰鍫熺厵闁告繂瀚ˉ婊兠瑰⿰鍫㈢暫婵﹥妞藉畷顐﹀Ψ閵夛妇鈧儳鈹戦悙鎻掓倯闁荤啿鏅犻妴浣割潩閼稿灚娅滄繝銏ｅ煐钃遍柡鍛箞濮婃椽骞愭惔銏╂⒖濠碘槅鍋勭€氼厾绮嬪澶嬬劶鐎广儱妫岄幏娲⒑閸涘﹥灏扮憸鏉垮暞缁傚秴鈹戦崼銏紲闁荤姴娲﹁ぐ鍐敂椤撶偐鏀介柍銉ョ－閸╋綁鏌℃担瑙勫磳闁轰焦鎹囬弫鎾绘晸閿燂拷
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
                        //婵犵數濮烽弫鍛婃叏閻戣棄鏋侀柛娑橈攻閸欏繐霉閸忓吋缍戦柛銊ュ€块弻锝夊箻瀹曞洤鍝洪梺鍝勵儐閻楁鎹㈠☉銏犵闁绘劘灏欓崝浼存⒑缁嬫鍎愰柟鍛婃倐閿濈偛鈹戠€ｎ偄浜楅柟鍏肩暘閸ㄦ槒銇愭惔顫箚闁靛牆娲ゅ暩闂佺ǹ顑嗛惄顖氱暦椤栫儐鏁嶆繝濠傚鎼村﹤鈹戦悩缁樻锭妞ゆ垵鎳橀幃娆愮節閸ャ劎鍘繝鐢靛€崘銊︽闁哥喐鎮傚濠氬磼濮橆兘鍋撻悜鑺ュ殑闁告挷鐒﹂崗婊堟煕椤愶絾绀€缂佲偓閸喓绡€闂傚牊渚楅崕宥夋煃瑜滈崜娆撳疮閺夋垹鏆﹂柕濞炬櫓閺佸秵绻濇繝鍌涘婵¤尙枪閳规垿鎮欑€涙ê闉嶉梺绯曟櫅閸熸潙鐣烽幋锕€绠荤紓浣诡焽閸樻悂鏌ｈ箛鏇炰粶濠⒀嗘鐓ら柟缁㈠枟閻撴稑霉閿濆洦鍤€濠殿喖绉堕埀顒冾潐濞叉﹢鏁冮姀鈥茬箚闁归棿绀佸敮濡炪倖妫冮弫顕€宕戦幘璇茬妞ゆ棁袙閹峰搫鈹戦鐐殌婵炲眰鍊濋幃鐐裁洪鍛幈闂侀潧艌閺呮瑩骞夐悙顒夋闁绘劖娼欐慨宥団偓瑙勬礃閿氭い鏂跨箻椤㈡瑩宕楃喊鍗炴偑闂傚倸鍊风欢姘焽瑜戞晶婵嬫⒑閸濆嫭鍣虹紒顔芥尭閻ｇ兘鎮界喊妯轰壕闁挎繂楠搁弸鐔兼煟閹惧瓨绀冮柕鍥у椤㈡﹢鎮㈡搴濇樊濠电偛鐡ㄧ划宥囨崲閸愵喖桅闁告洦鍨板Λ姗€鎮归幁鎺戝鐟滃酣骞堥妸锔剧瘈闁告劏鏂傛禒銏ゆ倵鐟欏嫭绀堥柛蹇旓耿閵嗕礁顫滈埀顒勫箖閵堝棙濯撮梺鍨儏閻撴洟姊婚崒姘偓鐑芥嚄閸洖绠犻柟鎹愵嚙閸氬綊鏌″搴″箹缂佺媭鍨堕弻銊╂偆閸屾稑顏�
                        
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
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾剧懓顪冪€ｎ亜顒㈡い鎰Г閹便劌顫滈崱妤€鈷掗梺缁樺笂缁瑩寮婚妶鍡樺弿闁归偊鍏橀崑鎾诲冀椤愮喎浜炬慨姗嗗墻閻撳ジ鏌″畝瀣М鐎殿噮鍣ｅ畷鎺戭潩椤戣法甯涚紓鍌氬€风拋鏌ュ磻閹炬剚鐔嗛柤鎼佹涧婵洨绱掗悩渚綈缂佺粯鐩弫鎰償閳ユ剚娼诲┑鐘殿暯閳ь剛鍋ㄩ崑銏ゆ煛鐏炲墽鈽夋い顐ｇ箞椤㈡宕掑┃鐐妼铻栭柣姗€娼ф禒锕傛煟濡や緡娈旀い顓炴穿缁犳稑鈽夊Ο纰辨Ф闁荤喐绮岀换妤呭箺椤愶絿绡€闁汇垽娼ф牎濡炪倖姊归悧鐘茬暦鐟欏嫬顕遍柡澶嬪殾瑜旈弻鐔兼⒒鐎靛壊妲梺钘夊暟閸犳牠寮婚敐澶婄睄闁稿本顨嗙€氭盯姊烘潪鎵槮缂佸缍婂璇测槈閵忕姷鍔撮梺鍛婂姉閸嬫捇鎮惧畡鎵虫斀闁绘﹩鍋勬禍鐐箾閺夋垵鎮戞繛鍏肩懇瀹曟﹢鍩€椤掑嫭鈷戦柣鎰閸旀粓鏌＄€ｎ亗鍋㈢€殿喓鍔嶇粋鎺斺偓锝庡亞閸樹粙姊鸿ぐ鎺戜喊闁告鏅槐鐐哄箣閿旂晫鍘介棅顐㈡储閸庢娊鎮鹃悽鍛婄厸鐎光偓閳ь剟宕伴幘璺哄灊婵炲棙鎸哥粻浼村箹濞ｎ剙鐏╅柡瀣灩缁辨捇宕掑▎鎴ｇ獥闂佸摜濮甸〃鍛扮亱闂佸憡鍔戦崝搴ㄥ汲閿曗偓閳规垿宕掑┃鎾崇秺瀹曪繝骞庨懞銉у幗闂佸搫鍟ú锕傛嚈鐟欏嫪绻嗛柛娆忣槸婵秵鎱ㄦ繝鍕笡闁瑰嘲鎳橀幃婊兾熼悜妯兼殮濠碉紕鍋戦崐鏍洪敃鍌氱煑闁告劑鍔庨弳锕傛煙鏉堥箖妾柛瀣缁辨帒螖閸曗斁鍋撻埀顒勬煕鐎ｎ偅宕岀€规洏鍔戝鍫曞箣閿濆棙鍟洪梻鍌欑窔濞佳嚶ㄩ埀顒€鈹戦垾铏枠鐎规洩缍佸畷鍗炩槈濞嗗本瀚肩紓鍌氬€烽悞锕佹懌缂備讲鍋撻柍褜鍓熷铏圭矙閸栤€充紣濠电姭鍋撻梺顒€绉撮悞鍨亜閹哄秷鍏岄柛鐔哥叀閺岀喖宕欓妶鍡楊伓0,
    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾剧懓顪冪€ｎ亝鎹ｉ柣顓炴閵嗘帒顫濋敐鍛婵°倗濮烽崑鐐烘偋閻樻眹鈧線寮撮姀鈩冩珕闂佽姤锚椤︻喚绱旈弴銏♀拻濞达綀娅ｉ妴濠囨煕閹惧绠為柟钘夊€婚埀顒婄秵閸犳牠鎮欐繝鍐︿簻闁瑰搫绉烽崗宀勬煕閺傝法鍩ｉ柟顔筋殜閻涱噣宕归鐓庮潛闂備胶鎳撻崯璺ㄦ崲濮椻偓楠炲啫螖閸滀焦鏅㈤梺鍛婃处閸忔﹢骞忓ú顏呪拺闁告稑锕﹂埥澶愭煥閺囶亜顩紒杈ㄦ尭椤繈鎳滈悽闈涘箞闂佺懓鍚嬮悾顏堝垂婵犳艾鐭楅柍褜鍓熼幃妤冩喆閸曨剛顦ㄩ梺鎼炲妿閺佸鏁愰悙娴嬪牚闁割偅绻勯崝锕€顪冮妶鍡楃瑨閻庢凹鍓熼幏鎴︽偄閸忚偐鍘繝鐢靛仧閸嬫挸鈻嶉崱娑欑厱閻庯綆浜濋ˉ銏ゆ煛鐏炲墽銆掑ù鐙呯畵瀹曟粏顦俊鎻掔墕椤啴濡堕崨瀵稿椽闂佺懓鍤栭幏锟�
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
                        if (0x0212 == (online->code))           //濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌熼梻瀵割槮缁炬儳顭烽弻锝夊箛椤掍焦鍎撻梺鎼炲妼閸婂潡寮诲☉銏╂晝闁挎繂妫涢ˇ銉х磽娴ｅ搫小闁告濞婂濠氭偄閸忓皷鎷婚柣搴＄仛閻℃洜绮婚悙鐑樷拺闁硅偐鍋涢崝鎾煕閹惧鎳呮繛鎴犳暬閸┾偓妞ゆ帒瀚埛鎴︽偣閸ヮ亜鐨虹紒鐘插级娣囧﹪宕ｆ径濠冭癁闂佹寧绻勯崑鐐差嚗閸曨剛绡€閹肩补鍓濋鍌炴⒒娴ｅ憡鍟炴繛璇х畵瀹曞綊骞嶉鍙ョ瑝濠电偞鍨堕悷锝嗙濠婂牊鐓欓柟瑙勫姈绾儳霉閻樺啿绗х紒杈ㄦ崌瀹曟帒鈽夊▎蹇曨暡闂備礁鐤囬～澶愬垂閸ф绠栨繛鍡樻尰閸ゅ姊婚崼鐔衡姇闂佹鍘剧槐鎾诲磼濞嗘劗銈伴悗娈垮枟閹搁箖宕氶幒鎴旀瀻闊洦娲樺▓鎯ь渻閵堝棛澧遍柛瀣⊕閸庮偊姊绘担鍝ユ瀮婵℃ぜ鍔戦幊妤呭醇閺囩偟顦梺鎸庢磵閸嬫挻銇勯鍕殻濠碘€崇埣瀹曞崬螖閸愵亝鍣梻鍌欑劍鐎笛兠鸿箛娑樼？闁规儼妫勯悞鍨亜閹哄秷鍏岄柛鐔哥叀閺岀喖宕欓妶鍡楊伓
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
    //Swap_numChar(buf_value,onlineDeviceList.count);     //灏�16杩涘埗鏁板勾杞负瀛楃涓�
    //buf_value = Swap_charNum((char *) * (int *)buf.payload);   //灏嗗瓧绗︿覆杞负16杩涘埗
    buf_value = Swap_charNum((char *)buf.payload);   //灏嗗瓧绗︿覆杞负16杩涘埗
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
#if 0  //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鎯у⒔閹虫捇鈥旈崘顏佸亾閿濆簼绨奸柟鐧哥秮閺岋綁顢橀悙鎼闂侀潧妫欑敮鎺楋綖濠靛鏅查柛娑卞墮椤ユ艾鈹戞幊閸婃鎱ㄩ悜钘夌；闁绘劗鍎ら崑瀣煟濡崵婀介柍褜鍏涚欢姘嚕閹绢喖顫呴柍鈺佸暞閻濇牠姊绘笟鈧埀顒傚仜閼活垱鏅堕弶娆剧唵閻熸瑥瀚粈瀣煙椤旀儳鍘存鐐诧攻缁绘繈宕掑⿰鍛呫劑姊虹拠鈥虫灀闁哄懏鐩、娆掔疀濞戣鲸鏅╅梺缁樻尭妤犳瓕鐏囩紓鍌氬€搁崐鎼佸磹閹间礁纾归柛婵勫劤閻捇鏌ｉ姀鐘冲暈闁稿顑夐弻锟犲炊閿濆棗娅氶梺閫炲苯澧惧┑鈥虫喘楠炴垿宕熼姣尖晠鏌ㄩ弴妤€浜剧紒鍓ц檸閸ㄨ泛顫忛搹鍦＜婵☆垵娅ｆ禒鎼佹煢閸愵喕鎲鹃柡宀€鍠栭幃婊堝箣閹烘挸鏋ゆ繝娈垮枛閿曘劌鈻嶉敐鍥у灊婵炲棙鍨跺畷澶愭煏婵炑冭嫰閺佽偐绱撻崒姘偓椋庢閿熺姴绐楁俊銈呮噹缁犳煡鏌涢妷鎴濊嫰濞堛劍绻濋悽闈浶ｉ柤鍦亾閸庮偊姊绘担绋挎毐闁圭⒈鍋婂畷鎰板川婵犲嫷娲搁梺缁樺姉閸庛倝鍩涢幋鐘电＜閻庯綆鍋勯婊勭節閳ь剟骞嶉鍓э紲闁诲函缍嗛崑鍕倶鐎涙ǜ浜滈柕蹇婂墲椤ュ牓鏌℃担瑙勫磳闁轰焦鎹囬弫鎾绘晸閿燂拷 1
    s8_t S;     //缂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鐐劤缂嶅﹪寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閻愵剙鍔ょ紓宥咃躬瀵鏁愭径濠勵吅闂佹寧绻傞幉娑㈠箻缂佹鍘辨繝鐢靛Т閸婂綊宕戦妷鈺傜厸閻忕偠顕ф慨鍌溾偓娈垮枟閹告娊骞冨▎寰濆湱鈧綆浜欐竟鏇㈡⒑閸撴彃浜濇繛鍙夌墵閹偤宕归鐘辩盎闂佺懓鎼Λ妤佺妤ｅ啯鈷戦柛婵勫劚閺嬫垿鏌熼崨濠傗枙闁诡喕鍗抽、姘跺焵椤掆偓閻ｇ兘宕奸弴銊︽櫌婵犮垼娉涢鍡椻枍閹剧粯鈷掑ù锝呮啞閸熺偤鏌ｉ悤鍌滅М鐎规洘娲熼獮搴ㄦ嚍閵夈儰鐢绘繝鐢靛█濞佳囶敄閸℃稑鐓曢柟鐑樺殮瑜版帗鏅查柛鏇ㄥ櫘閸斿懘姊虹悰鈥充壕婵炲鍘ч悺銊╂偂濞嗘劗绠鹃柤濂割杺閸炶櫣绱掗妸褎娅曢柍褜鍓欓悘姘熆濮椻偓閺佸啴鏁傞悙顒夋綗闂佸湱鍎ら崵锕傛偄閸濆嫭鐎抽柡澶婄墑閸旀垿寮⿰鍫熲拺闁煎鍊曢弸鎴犵磼椤旂厧顒㈤柡鍛版硾铻栭柛鎰ㄦ櫆濞堟澘鈹戞幊閸婃洟骞婅箛娑樼；闁稿瞼鍋為悡蹇擃熆鐠虹儤顥炴繛鍛閺岋綁骞樼捄鐑樼亪闂佸搫鐭夌换婵嗙暦閸洖鐓涘ù锝夘棑閹规洟姊绘笟鈧埀顒傚仜閼活垱鏅堕鐣岀瘈闁逞屽墴閺屽棗顓奸崨顖氬Е婵＄偑鍊栫敮濠囨嚄閸洖鐤柡灞诲劜閻撴瑩鏌涢幋娆忊偓鏍偓姘炬嫹
    s8_t  E;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鎯у⒔閹虫捇鈥旈崘顏佸亾閿濆簼绨奸柟鐧哥秮閺岋綁顢橀悙鎼闂傚洤顦甸弻銊モ攽閸♀晜效婵炲瓨鍤庨崐婵嬪蓟閵堝绾ч柟绋块娴犳挳鎮楀▓鍨灈闁绘牜鍘ч悾鐑芥偂鎼存ɑ顫嶅┑鈽嗗灟鐠€锕傛倵瀹勬壋鏀介柣鎰▕閸ょ喎鈹戦鐐毄闁哥姴锕ら濂稿炊閵娿儱绨ユ繝鐢靛█濞佳囶敄閸℃稑纾婚柕濞炬櫆閳锋帡鏌涢銈呮灁闁崇鍎甸弻娑樷攽閸ヨ埖鐣跺銈庝簻閸熷瓨淇婇崼鏇炲耿婵☆垳鈷堝Σ鎵磽閸屾瑧璐伴柛鐘崇墪闇夐柣鎴ｆ缁€鍡涙煙閻戞ɑ灏电紒鈾€鍋撴繝娈垮枟閿曗晠宕滃☉銏╂晩闁圭儤顨嗛埛鎴犳喐閻楀牆淇柡鍡樺笧缁辨帡鍩€椤掍焦濯寸紒顖涙礃閻忎線姊洪棃娴ュ牓寮插⿰鍫濈厱闁硅揪闄勯崑鈩冪箾閸℃绠版い蹇婃櫊閺屾盯寮崶銊с€愰梺瀹狀潐閸ㄥ潡銆佸▎鎾充紶闁告洏鍔屾禍鐐節闂堟稒锛嶉柛銈嗘礋閺岋紕浠︾拠鎻掑缂備胶濮烽幊鎾绘箒闂佹寧绻傞幊蹇曟嫻閿熺姵鐓曟繛鍡楃箲椤ャ垺鎱ㄦ繝鍛仩缂佽鲸甯掕灒闁煎鍊ゅΣ顒勬⒒娴ｅ懙褰掓晝閵堝拑鑰块梺顒€绉撮悞鍨亜閹哄秷鍏岄柛鐔哥叀閺岀喖宕欓妶鍡楊伓
    u32_t F;    //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳婀遍埀顒傛嚀鐎氼參宕崇壕瀣ㄤ汗闁圭儤鍨归崐鐐差渻閵堝棗绗掓い锔垮嵆瀵煡顢旈崼鐔蜂画濠电姴锕ら崯鎵不缂佹﹩娈介柣鎰綑閻忔潙鈹戦鐟颁壕闂備線娼ч悧鍡涘箠閹扮増鍋柍褜鍓氭穱濠囨倷椤忓嫧鍋撻弽顬稒鎷呴懖婵囩洴瀹曠喖顢楁担绋垮Τ濠电姷鏁告慨鏉懨洪敃鍌氱厱闁瑰濮甸崰鎰版煟濡も偓閻楀棛绮幒鎳ㄧ懓饪伴埀顒勫础閹惰棄钃熸繛鎴欏灪閸嬫劙鏌熸０浣哄妽闁伙絽鐖煎鐑樺濞嗘垶鍋ч梺绋跨箲閿氭い鏇稻缁绘繂顫濋鈹垮姂閺屻劑寮埀顒勫磿閹剁晫宓佺€广儱顦伴埛鎴︽煕濠靛棗顏存俊鍙夋倐閺岋絽螖閳ь剙螞濞嗘挸鐤鹃柤鍝ユ暩椤╃兘鎮楅敐搴′簻濞寸姴銈搁弻鈩冨緞婵犲嫬顣堕梺鍛婃煥缁夌懓鐣烽敐鍫㈢杸婵炴垶鐟ユ禒顓炩攽閻愬弶顥滅紒缁樺姍椤㈡棃顢曢敂鐣屽幈濠电偛妫楅懟顖涚妤ｅ啯鈷掗柛灞剧懅椤︼箓鏌熺喊鍗炰喊闁诡垰鏈鍕暆閳ь剛绮堟繝鍋綊鏁愰崨顓ф闂佹悶鍊栧ú姗€濡甸崟顖氬嵆婵°倐鍋撳ù婊堢畺閹鎲撮崟顒傤槰闁汇埄鍨辩敮锟犳晲閻愭祴鏀介悗锝呯仛閺呯偤鏌ｉ悩鍏呰埅闁告柨鐬肩划顓㈡晬閸曨厾锛濋梺绋挎湰閼归箖鍩€椤掍焦鍊愮€规洘鍔欓幃婊堟嚍閵夈儲鐣遍梻浣藉亹鏋柣蹇斿哺瀹曪綀绠涘☉娆戝幐婵犮垼娉涢鍛存倿妤ｅ啯鐓曢柕鍫濇缁€瀣煛鐏炲墽娲存い銏℃礋閺佹劙宕卞▎妯恍氭繝纰樺墲椤ㄥ懘骞婂Ο渚綎婵炲樊浜滃婵嗏攽閻樻彃鏆欐い锔规櫊濮婃椽骞栭悙鎻掝瀴婵犮垻鎳撳Λ婵嬪箖椤曗偓楠炲棜顧侀柛姘儏椤法鎹勬笟顖氬壋闂佸憡锕㈡禍璺侯潖濞差亝顥堟繛鎴ｉ哺鏁堥梻浣规偠閸斿酣寮拠鑼殾妞ゆ牜鍎愰弫宥夋煟閹邦垰鐨洪柨娑氬枎椤啴濡堕崨顖滎唶濠碘槅鍋呯粙鎾寸珶閺囩喓顩烽悗锝庡亞閸橀亶鏌ｈ箛鏇炰粶濠⒀傜矙楠炲﹪宕ㄩ杞扮盎闂佹寧鏌ㄩ妶绋款瀶椤曗偓閺屾洟宕惰椤忣剚銇勯敂璇叉珝闁哄本绋撻埀顒婄秵閸嬪懎鐣风仦鐐弿濠电姴鍟妵婵嬫煙椤旀儳鍘寸€殿喖鐖奸獮鎰償椤斿吋鏆忛梻鍌氬€烽懗鍫曞箠閹捐鍚归柡宥庡幖缁狀垶鏌ㄩ悤鍌涘
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
#elif  1 //闂傚倸鍊搁崐鎼佸磹閹间礁纾归柟闂寸绾惧綊鏌熼梻瀵割槮缁炬儳缍婇弻鐔兼⒒鐎靛壊妲紒鎯у⒔閹虫捇鈥旈崘顏佸亾閿濆簼绨奸柟鐧哥秮閺岋綁顢橀悙鎼闂侀潧妫欑敮鎺楋綖濠靛鏅查柛娑卞墮椤ユ艾鈹戞幊閸婃鎱ㄩ悜钘夌；闁绘劗鍎ら崑瀣煟濡崵婀介柍褜鍏涚欢姘嚕閹绢喖顫呴柍鈺佸暞閻濇牠姊绘笟鈧埀顒傚仜閼活垱鏅堕弶娆剧唵閻熸瑥瀚粈瀣煙椤旀儳鍘存鐐诧攻缁绘繈宕掑⿰鍛呫劑姊虹拠鈥虫灀闁哄懏鐩、娆掔疀濞戣鲸鏅╅梺缁樻尭妤犳瓕鐏囩紓鍌氬€搁崐鎼佸磹閹间礁纾归柛婵勫劤閻捇鏌ｉ姀鐘冲暈闁稿顑夐弻锟犲炊閿濆棗娅氶梺閫炲苯澧惧┑鈥虫喘楠炴垿宕熼姣尖晠鏌ㄩ弴妤€浜剧紒鍓ц檸閸ㄨ泛顫忛搹鍦＜婵☆垵娅ｆ禒鎼佹煢閸愵喕鎲鹃柡宀€鍠栭幃婊堝箣閹烘挸鏋ゆ繝娈垮枛閿曘劌鈻嶉敐鍥у灊婵炲棙鍨跺畷澶愭煏婵炑冭嫰閺佽偐绱撻崒姘偓椋庢閿熺姴绐楁俊銈呮噹缁犳煡鏌涢妷鎴濊嫰濞堛劍绻濋悽闈浶ｉ柤鍦亾閸庮偊姊绘担绋挎毐闁圭⒈鍋婂畷鎰板川婵犲嫷娲搁梺缁樺姉閸庛倝鍩涢幋鐘电＜閻庯綆鍋勯婊勭節閳ь剟骞嶉鍓э紲闁诲函缍嗛崑鍕倶鐎涙ǜ浜滈柕蹇婂墲椤ュ牓鏌℃担瑙勫磳闁轰焦鎹囬弫鎾绘晸閿燂拷 濠电姷鏁告慨鐑藉极閸涘﹥鍙忛柣鎴ｆ閺嬩線鏌涘☉姗堟敾闁告瑥绻橀弻锝夊箣閿濆棭妫勯梺鍝勵儎缁舵岸寮婚悢鍏尖拻閻庨潧澹婂Σ顔剧磼閹冣挃缂侇噮鍨抽幑銏犫槈閵忕姷顓哄┑鐐叉缁绘帗绂掓ィ鍐┾拺缂佸顑欓崕宥夋煕婵犲啰绠炵€殿喖顭峰鎾偄閾忚鍟庨柣鐔哥矊缁夌懓鐣烽鐑嗘晝闁挎棁妫勯埀顒冩硾閳规垿鎮╅崣澶婎槱闂佹娊鏀遍崹鍧楀蓟閻旇櫣鐭欓柟绋垮瀹曞磭绱撴担鍝勑ｉ柟绋款煼婵＄敻宕熼娑欐珕闁荤姴娲╃亸娆愮椤栨粎纾藉〒姘搐閺嬫稒銇勯鐘插幋闁靛棔绀侀埥澶愬閳ユ枼鍋撻悜鑺ュ€甸柣銏㈡閻熼偊鍤曢柟鎯板Г閳锋帒霉閿濆牊顏犻悽顖涚⊕缁绘盯宕ｆ径灞解拰閻庤娲橀崝娆忕暦閻戠瓔鏁囨繛鎴灻兼竟鏇炩攽椤旂瓔鐒介柛瀣尵閳ь剚鐔幏锟�

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


           