/**
 * @file     gatt.h
 * @brief    -
 * @details
 * @mainpage
 * @version  V1.0.0
 * @date     2019-10-11
 * @license  Copyright (C) 2019 Unisoc Communications Inc
 */

#ifndef _GATT_H_
#define _GATT_H_

#include "bt_gatt.h"

//GAP AD type from Generic Access Profile of Assigned Numbers
#define GAP_ADTYPE_FLAGS 0x01                                                      //!< Discovery Mode: @ref GAP_ADTYPE_FLAGS_MODES
#define GAP_ADTYPE_16BIT_MORE 0x02                                            //!< Service: More 16-bit UUIDs available
#define GAP_ADTYPE_16BIT_COMPLETE 0x03                                    //!< Service: Complete list of 16-bit UUIDs
#define GAP_ADTYPE_32BIT_MORE 0x04                                            //!< Service: More 32-bit UUIDs available
#define GAP_ADTYPE_32BIT_COMPLETE 0x05                                    //!< Service: Complete list of 32-bit UUIDs
#define GAP_ADTYPE_128BIT_MORE 0x06                                          //!< Service: More 128-bit UUIDs available
#define GAP_ADTYPE_128BIT_COMPLETE 0x07                                  //!< Service: Complete list of 128-bit UUIDs
#define GAP_ADTYPE_LOCAL_NAME_SHORT 0x08                              //!< Shortened local name
#define GAP_ADTYPE_LOCAL_NAME_COMPLETE 0x09                        //!< Complete local name
#define GAP_ADTYPE_POWER_LEVEL 0x0A                                         //!< TX Power Level: 0xXX: -127 to +127 dBm
#define GAP_ADTYPE_OOB_CLASS_OF_DEVICE 0x0D                         //!< Simple Pairing OOB Tag: Class of device (3 octets)
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_HASHC 0x0E               //!< Simple Pairing OOB Tag: Simple Pairing Hash C (16 octets)
#define GAP_ADTYPE_OOB_SIMPLE_PAIRING_RANDR 0x0F               //!< Simple Pairing OOB Tag: Simple Pairing Randomizer R (16 octets)
#define GAP_ADTYPE_SM_TK 0x10                                                     //!< Security Manager TK Value
#define GAP_ADTYPE_SM_OOB_FLAG 0x11                                        //!< Secutiry Manager OOB Flags
#define GAP_ADTYPE_SLAVE_CONN_INTERVAL_RANGE 0x12            //!< Min and Max values of the connection interval (2 octets Min, 2 octets Max) (0xFFFF indicates no conn interval min or max)
#define GAP_ADTYPE_SIGNED_DATA 0x13                                        //!< Signed Data field
#define GAP_ADTYPE_SERVICES_LIST_16BIT 0x14                           //!< Service Solicitation: list of 16-bit Service UUIDs
#define GAP_ADTYPE_SERVICES_LIST_128BIT 0x15                         //!< Service Solicitation: list of 128-bit Service UUIDs
#define GAP_ADTYPE_SERVICE_DATA 0x16                                       //!< Service Data
#define GAP_ADTYPE_APPEARANCE 0x19                                          //!< Appearance
#define GAP_ADTYPE_ADVERTISING_INTERVAL  0x1A                         //!<Advertising Interval?
#define GAP_ADTYPE_LE_BLUETOOTH_DEVICE_ADDRESS 0x1B  //!<LE Bluetooth Device Address?
#define GAP_ADTYPE_LE_ROLE 0x1C                                            //!<LE Role?
#define GAP_ADTYPE_MANUFACTURER_SPECIFIC 0xFF                    //!< Manufacturer Specific Data: first 2 octets contain the Company Identifier Code followed by the additional manufacturer specific data

//GAP Appearance Characteristic from Assigned Numbers of Bluetooth.org
/*!!!NOTE:The values are composed of a category (10-bits) and sub-categories (6-bits)!!!.*/
#define GAP_APPEARANCE_CHAR_UNKNOW                                                      0
#define GAP_APPEARANCE_CHAR_GENERIC_PHONE                                         1//64
#define GAP_APPEARANCE_CHAR_GENERIC_COMPUTER                                   2 //128

#define GAP_APPEARANCE_CHAR_GENERIC_WATCH                                        3//192
#define GAP_APPEARANCE_CHAR_SUB_SPORTS_WATCH                                             1//193

#define GAP_APPEARANCE_CHAR_GENERIC_CLOCK                                          4 //256
#define GAP_APPEARANCE_CHAR_GENERIC_DISPLAY                                      5//320
#define GAP_APPEARANCE_CHAR_GENERIC_REMOTE_CONTROL                       6//384
#define GAP_APPEARANCE_CHAR_GENERIC_EYE_GLASSES                              7//448
#define GAP_APPEARANCE_CHAR_GENERIC_TAG                                              8//512
#define GAP_APPEARANCE_CHAR_GENERIC_KEYRING                                       9//576
#define GAP_APPEARANCE_CHAR_GENERIC_MEDIA_PLAYER                            10//640
#define GAP_APPEARANCE_CHAR_GENERIC_BARCODE_SCANNER                     11//704

#define GAP_APPEARANCE_CHAR_GENERIC_THERMOMETER                             12//768
#define GAP_APPEARANCE_CHAR_SUB_THERMOMETER_EAR                                        1//769

#define GAP_APPEARANCE_CHAR_GENERIC_HEART_RATE_SENSOR                  13//832
#define GAP_APPEARANCE_CHAR_SUB_HEART_RATE_SENSOR_BELT                           1//833

#define GAP_APPEARANCE_CHAR_GENERIC_BLOOD_PRESSURE                        14//896
#define GAP_APPEARANCE_CHAR_SUB_BLOOD_PRESSURE_ARM                                  1//897
#define GAP_APPEARANCE_CHAR_SUB_BLOOD_PRESSURE_WRIST                               2//898

#define GAP_APPEARANCE_CHAR_GENERIC_HID                                               15//960
#define GAP_APPEARANCE_CHAR_SUB_HID_KEYBOARD                                              1//961
#define GAP_APPEARANCE_CHAR_SUB_HID_MOUSE                                                    2//962
#define GAP_APPEARANCE_CHAR_SUB_HID_JOYSTICK                                                3//963
#define GAP_APPEARANCE_CHAR_SUB_HID_GAMEPAD                                                4//964
#define GAP_APPEARANCE_CHAR_SUB_HID_DIGITIZER_TABLET                                5//965
#define GAP_APPEARANCE_CHAR_SUB_HID_CARD_READER                                        6//966
#define GAP_APPEARANCE_CHAR_SUB_HID_DIGITAL_PEN                                         7//967
#define GAP_APPEARANCE_CHAR_SUB_HID_BARCODE_SCANNER                                8//968

#define GAP_APPEARANCE_CHAR_GENERIC_GLUCOSE_METER                          16//1024

#define GAP_APPEARANCE_CHAR_GENERIC_RUNNING_WALKING_SENSOR       17//1088
#define GAP_APPEARANCE_CHAR_SUB_RUNNING_WALKING_SENSOR_IN_SHOE           1//1089
#define GAP_APPEARANCE_CHAR_SUB_RUNNING_WALKING_SENSOR_ON_SHOE           2//1090
#define GAP_APPEARANCE_CHAR_SUB_RUNNING_WALKING_SENSOR_ON_HIP             3//1091

#define GAP_APPEARANCE_CHAR_GENERIC_CYCLING                                        18//1152
#define GAP_APPEARANCE_CHAR_SUB_CYCLING_COMPUTER                                           1//1153
#define GAP_APPEARANCE_CHAR_SUB_CYCLING_SPEED_SENSOR                                   2//1154
#define GAP_APPEARANCE_CHAR_SUB_CYCLING_CADENCE_SENSOR                              3//1155
#define GAP_APPEARANCE_CHAR_SUB_CYCLING_POWER_SENSOR                                  4//1156
#define GAP_APPEARANCE_CHAR_SUB_CYCLING_SPEED_CADENCE_SENSOR                  5//1157

#define GAP_APPEARANCE_CHAR_GENERIC_PULSE_OXIMETER                         49//3136
#define GAP_APPEARANCE_CHAR_SUB_OXIMETER_FINGERTIP                                        1//3137
#define GAP_APPEARANCE_CHAR_SUB_OXIMETER_WRIST_WORMN                                 2//3138

#define GAP_APPEARANCE_CHAR_GENERIC_WEIGHT_SCALE                             50//3200

#define GAP_APPEARANCE_CHAR_GENERIC_OUTDOOR_SPORTS_ACTIVITY       81//5184
#define GAP_APPEARANCE_CHAR_SUB_LOCATION_DISPLAY_DEVICE                               1//5185
#define GAP_APPEARANCE_CHAR_SUB_LOCATION_NAVIGATION_DISPLAY_DEVICE         2//5186
#define GAP_APPEARANCE_CHAR_SUB_LOCATION_POD                                                     3//5187
#define GAP_APPEARANCE_CHAR_SUB_LOCATION_NAVIGATION_POD                              4//5188


//PRIVACY Flag characteristic
#define GAP_PERIPHERAL_PRIVACY_FLAG_DISABLE      0
#define GAP_PERIPHERAL_PRIVACY_FLAG_ENABLE      1

//Central Address Resolution
#define GAP_ADDRESS_RESOLUTION_NOT_SUPPORT   0
#define GAP_ADDRESS_RESOLUTION_SUPPORT           1




//GATT primary service UUID
#define ATT_UUID_GAP                0x1800   //Generic Access
#define ATT_UUID_GATT               0x1801   //Generic Attribute
#define ATT_UUID_IAS                0x1802   //Immediate Alert Service
#define ATT_UUID_LLS                0x1803   //Link Loss Service
#define ATT_UUID_TPS                0x1804   //Tx Power Service
#define ATT_UUID_CTS                0x1805   //Current Tim??e Service
#define ATT_UUID_RTUS               0x1806   //Reference Time Update Service
#define ATT_UUID_NDCS               0x1807   //Next DST Change Service
#define ATT_UUID_GLS                   0x1808   //Glucose Service
#define ATT_UUID_HTS                0x1809   //Health Thermometer Service
#define ATT_UUID_DIS                      0x180A   //Device Information Service
#define ATT_UUID_HRS                0x180D   //Heart Rate Service
#define ATT_UUID_PASS               0x180E   //Phone Alert Status Service
#define ATT_UUID_BAS                0x180F   //Battery Service
#define ATT_UUID_BLS                0x1810   //Blood Pressure Service
#define ATT_UUID_ANS                0x1811   //Alert Notification Service
#define ATT_UUID_HIDS               0x1812   //HID Service
#define ATT_UUID_SCPS               0x1813   //Scan Parameters Service
#define ATT_UUID_RSCS               0x1814   //Running Speed and Cadence Service
#define ATT_UUID_AIOS               0x1815   //Automation IO Service
#define ATT_UUID_CSCS               0x1816   //Cycling Speed and Cadence Service
#define ATT_UUID_CPS                0x1818   //Cycling Power Service
#define ATT_UUID_LNS                0x1819   //Location and Navigation Service
#define ATT_UUID_ESS                0x181A   //Environmental Sensing Service
#define ATT_UUID_BCS                0x181B   //Body Composition Service
#define ATT_UUID_UDS                0x181C   //User Data Service
#define ATT_UUID_WSS                0x181D   //Weight Scale Service
#define ATT_UUID_BMS                0x181E   //Bond Management Service
#define ATT_UUID_CGMS               0x181F   //Continuous Glucose Monitoring Service
#define ATT_UUID_IPSS               0x1820   //Internet Protocol Support Service
#define ATT_UUID_IPS                0x1821   //Indoor Positioning Service
#define ATT_UUID_PLXS               0x1822   //Pulse Oximeter Service


//GATT Characteristic Types
#define ATT_UUID_DEVICENAME         0x2A00
#define ATT_UUID_APPEARANCE         0x2A01
#define ATT_UUID_PRIVACY_FLAG              0x2A02
#define ATT_UUID_RECONN_ADDR               0x2A03
#define ATT_UUID_CONN_PARAMETER          0x2A04
#define ATT_UUID_SERVICE_CHANGE     0x2A05
#define ATT_UUID_ALERT_LEVEL                  0x2A06
#define ATT_UUID_CENTRAL_ADDRESS_RESOLUTIION     0x2AA6
#define ATT_UUID_RESOLVABLE_PRIVATE_ADDRESS       0x2AC9

#define ATT_UUID_GLUCOSE_MEASUREMENT      0x2A18
#define ATT_UUID_BATTERY_LEVEL                    0x2A19
#define ATT_UUID_GLUCOSE_MEASUREMENT_CONTEXT      0x2A34
#define ATT_UUID_GLUCOSE_FEATURE                  0x2A51
#define ATT_UUID_RECORD_ACCESS_CONTROL_POINT      0x2A52


//device information service characteristics uuid
#define ATT_UUID_SYSTEM_ID                 0x2A23
#define ATT_UUID_MODEL_NUMBER       0x2A24
#define ATT_UUID_SERIAL_NUMBER      0x2A25
#define ATT_UUID_FIRMWARE_REVISION  0x2A26
#define ATT_UUID_HARDWARE_REVISION  0x2A27
#define ATT_UUID_SOFTWARE_REVISION  0x2A28
#define ATT_UUID_MANUFACTURER_NAME  0x2A29
#define ATT_UUID_REGISTRATION_CERTIFICATE   0x2A2A
#define ATT_UUID_PNP_ID             0x2A50
#define ATT_UUID_INDOOR_POSITION_CONFIG             0x2AAD

// HID Service characteristics uuid
#define ATT_UUID_HID_PROTOCOL_MODE  0x2A4E
#define ATT_UUID_HID_REPORT         0x2A4D
#define ATT_UUID_HID_REPORT_MAP     0x2A4B
#define ATT_UUID_HID_KEY_INPUT      0x2A22
#define ATT_UUID_HID_KEY_OUTPUT     0x2A32
#define ATT_UUID_HID_MOUSE_INPUT    0x2A33
#define ATT_UUID_HID_INFORMATION    0x2A4A
#define ATT_UUID_HID_CONTROL        0x2A4C

// Scan Parameters Service characteristics uuid
#define ATT_UUID_SCAN_INTERVAL      0x2A4F
#define ATT_UUID_SCAN_REFRESH       0x2A31

//Tx Power Service characteristics uuid
#define ATT_UUID_TX_POWER_LEVEL     0x2A07



//x should larger than 6 and less than 17
#define ATT_PM_KEY_SIZE(x)          (((x-7)<<12)&0xf000)



//characteristic property definitions

//gatt client request opcode
#define GATT_PRIMARY_SERVICE_REQ            (0x0001<<8)
#define GATT_RELATIONSHIP_REQ               (0x0002<<8)
#define GATT_CHAR_REQ                       (0x0003<<8)
#define GATT_DISCOVER_DESCRIPTOR_REQ        (0x0004<<8)
#define GATT_CHAR_VALUE_READ_REQ            (0x0005<<8)
#define GATT_CHAR_VALUE_WRITE_REQ       (0x0006<<8)
#define GATT_CHAR_DESCRIPTOR_REQ            (0x0007<<8)

//subcode of each gatt client request
#define GATT_PS_ALL         0x01
#define GATT_PS_BY_UUID 0x02

#define GATT_CHAR_ALL       0x01
#define GATT_CHAR_BY_UUID   0x02

#define GATT_CHAR_VR            0x01
#define GATT_CHAR_VR_BY_UUID    0x02
#define GATT_CHAR_VR_LONG       0x03
#define GATT_CHAR_VR_MULTIPLE   0x04

#define GATT_CHAR_WWP           0x01
#define GATT_CHAR_SWWP      0x02
#define GATT_CHAR_W         0x03
#define GATT_CHAR_W_LONG        0x04
#define GATT_CHAR_RELIABLE_W    0x05

#define GATT_CHAR_DES_READ_S    0x01
#define GATT_CHAR_DES_READ_L    0x02
#define GATT_CHAR_DES_WRITE_S   0x03
#define GATT_CHAR_DES_WRITE_L   0x04

//message id to upper layer apps
#define GATT_CONNECT_CNF            0x11
#define GATT_DISCONNECT_IND     0x12
#define GATT_CLIENT_REQ_RSP     0x13
#define GATT_NOTIFICATION           0x14
#define GATT_INDICATION         0x15
#define GATT_IND_TIMEOUT            0x16
#define GATT_WRITE_NOTIFY           0x17
#define GATT_INDICATION_CONF        0x18
#define GATT_NOTIFICATION_CNF        0x19
#define GATT_EXCHAGE_MTU_CNF    0x30
#define GATT_CLIENT_SUPPORT         1

#define GATT_EXCHAGE_MTU_CNF            0x30
#define GATT_FIND_PROVISION_SERVICE     0x31
#define GATT_MESH_CHAR_DATA             0x32
#define GATT_FIND_PROXY_SERVICE         0x33
#define GATT_MESH_PROXY_CHAR_DATA       0x34
typedef int (*gatt_module_state_cb_t)(UINT16 msg_id, void *param);

typedef struct
{
    bdaddr_t address;
    UINT16 aclHandle;
    UINT16 mtu;
    UINT8 result;
} gatt_connect_cnf_t;

typedef struct
{
    UINT16 aclHandle;
    UINT16 result;
    UINT8 reason;
} gatt_disconn_ind_t;

typedef struct
{
    UINT16 aclHandle;
} gatt_srv_ind_timeout_t;

typedef struct
{
    UINT16 uuid;
    UINT16 handle;
    UINT32 length;
    UINT8 *value;
} gatt_chara_desc_t;

typedef struct
{
    UINT16 handle;
    UINT16 uuid;
    UINT16 valueHandle;
    UINT8 properties;
    int descNum;
    UINT8 *value;
    gatt_chara_desc_t *pDescList;
} gatt_chara_t;

typedef struct
{
    UINT16 startHandle;
    UINT16 endHandle;
    UINT16 uuid;
    UINT16 charNum;
    gatt_chara_t *pCharaList;
} gatt_prime_service_t;

typedef struct
{
    UINT16 reqCode;
    UINT16 module;
    UINT16 startHandle;
    UINT16 endHandle;
    UINT16 uuid;
    UINT8  uuid_l[16];
    UINT16 offset;
    UINT16 attValueLen;
    UINT8  uuidFmt;
    UINT16 acl_handle;
    UINT8 *attValue;
} gatt_client_req_t;

typedef struct
{
    UINT16 reqCode;
    UINT16 payLoadLen;
    UINT8   result;
    UINT8   contiueFlag;
    UINT8 *rspPayLoad;
} gatt_client_req_rsp_t;

typedef struct
{
    UINT16 length;
    UINT8 *value;
} gatt_server_noti_t;

typedef struct
{
    UINT16 aclHandle;
    UINT16 reqCode;
    UINT16 length;
    UINT16 attHandle;
    UINT8 *data;
} gatt_server_req_t;



typedef struct
{
    //UINT8 properties;
    //UINT16 handle;
    //UINT8 uuid[2];
    UINT8 value[19];
} gatt_chara_def_long_t;

typedef struct
{
    UINT8 value[6];
} gatt_include_value_short_t;

typedef struct
{
    UINT8 value[20];
} gatt_include_value_long_t;

typedef struct
{
    UINT16  configurationBits;
    UINT16 aclHandle;
} gatt_chara_ccb_t;

typedef struct
{
    UINT8 value[7];
} gatt_chara_format_t;

typedef struct
{
    UINT16 min_conn_interval;
    UINT16 max_conn_interval;
    UINT16 slave_latency;
    UINT16 conn_timeout;
} gatt_chara_pcp_t;

typedef struct
{
    //UINT16 startHandle;
    //UINT16 endHandle;
    UINT8 handle[4];
} gatt_chara_sc_t;


typedef struct _GATT_LE_MSG_INFO_T
{
    bdaddr_t addr;
    UINT16 acl_handle;
    UINT8 result;
} GATT_LE_MSG_INFO_T;



/* le data message structure */
typedef struct st_gatt_data_msg
{
    unsigned char service_size;
    gatt_element_t *service;
} gatt_data_msg_t;


typedef struct
{
    unsigned char advLen;
    unsigned char advData[31];
} gatt_adv_data_t;


typedef struct
{
    UINT8 result;
    UINT8 size;
    UINT8 addr_type;
    bdaddr_t address;
} gatt_white_list_struct;

typedef enum
{
    READ_LIST_SIZE,
    CLEAR_LIST,
    ADD_DEVE_LIST,
    REMOVE_DEVE_LIST,
    READ_LOCAL_RESO_ADDR,
    READ_PEER_RESO_ADDR,
    RESOLVING_LIST_ENABLE,
    SET_RESOLVING_ADDR_TIMEOUT,
} enum_ble_resolve_list_opcode;

typedef struct
{
    enum_ble_resolve_list_opcode opcode ;
    UINT8 enable ;
    UINT8 timeout;
    UINT8 Peer_IdentityAddrType;
    UINT8 Peer_IdentityAddr[6];
    UINT8 PeerIrk[16];
    UINT8 LocalIrk[16];
} gatt_randram_addr_struct;

typedef struct
{
    UINT32  flag;
    UINT16 aclHandle;
    UINT16 cid;
    UINT8   module;
    UINT32 secBitmap;
    bdaddr_t address;
    UINT16 pendingOpcode;
} gatt_app_module_t;

typedef struct
{
    UINT8   appModuleNum;
    gatt_app_module_t *appArray;
} gatt_info_t;

//gatt server api
int gatt_add_service(gatt_element_t *pElementArray, UINT16 numElement);
int gatt_server_req(gatt_server_req_t *req);
void ble_gatt_init_server(UINT16 module, gatt_module_state_cb_t cb);
UINT32 gatt_add_sdp_record(void);
UINT16 gatt_find_atthandle_by_uuid(UINT16 uuid);
UINT16 gatt_find_atthandle_by_luuid(UINT8 *uuid);
void ble_gatt_send_notification(gatt_le_data_info_t *data_info, int type);
gatt_app_module_t *find_module_by_address(bdaddr_t *address);

void gatt_add_basic_service(void);

//gatt client api
int gatt_unregister_module(UINT16 module);

int ble_gatt_client_req(gatt_client_req_t *pClientReq);
int gatt_exchange_mtu(UINT16 acl_handle, UINT16 mtu);

extern char gap_dn[22];

#pragma pack(1)
typedef struct
{
    UINT8 flags[3];
    UINT8 uuid_list[4];
    UINT8 service_data[22];
} GATT_Unprovisioned_Beacon_PDU;
#pragma pack()

#endif
