/**
 * @file     att.h
 * @brief    -
 * @details  
 * @mainpage 
 * @version  V1.0.0
 * @date     2019-10-11
 * @license  Copyright (C) 2019 Unisoc Communications Inc
 */


#ifndef _ATT_H_
#define _ATT_H_
#include "gatt.h"


#define ATT_ERROR_RSP               0x01
#define ATT_EXCHANGE_MTU_REQ        0x02
#define ATT_EXCHANGE_MTU_RSP        0x03
#define ATT_FIND_INFORMATION_REQ    0x04
#define ATT_FIND_INFORMATION_RSP    0x05
#define ATT_FIND_BY_TYPE_VALUE_REQ  0x06
#define ATT_FIND_BY_TYPE_VALUE_RSP  0x07
#define ATT_READ_BY_TYPE_REQ        0x08
#define ATT_READ_BY_TYPE_RSP        0x09
#define ATT_READ_REQ                0x0A
#define ATT_READ_RSP                0x0B
#define ATT_READ_BLOB_REQ           0x0C
#define ATT_READ_BLOB_RSP           0x0D
#define ATT_READ_MULTIPLE_REQ       0x0E
#define ATT_READ_MULTIPLE_RSP       0x0F
#define ATT_READ_BY_GROUP_TYPE_REQ  0x10
#define ATT_READ_BY_GROUP_TYPE_RSP  0x11
#define ATT_WRITE_REQ               0x12
#define ATT_WRITE_RSP               0x13
#define ATT_WRITE_CMD               0x52
#define ATT_SIGNED_WRITE_CMD        0xD2
#define ATT_PREPARE_WRITE_REQ       0x16
#define ATT_PREPARE_WRITE_RSP       0x17
#define ATT_EXECUTE_WRITE_REQ       0x18
#define ATT_EXECUTE_WRITE_RSP       0x19

#define ATT_HANDLE_VALUE_NOTIFICATION   0x1B
#define ATT_HANDLE_VALUE_INDICATION     0x1D
#define ATT_HANDLE_VALUE_CONFIRMATION   0x1E

//error code definitions
#define ATT_ERR_INVALID_HANDLE          0x01
#define ATT_ERR_READ_NOT_PERMITTED      0x02
#define ATT_ERR_WRITE_NOT_PERMITTED     0x03
#define ATT_ERR_INVALID_PDU             0x04
#define ATT_ERR_INSUFFICIENT_AUTHEN     0x05
#define ATT_ERR_REQUEST_NOT_SUPPORT     0x06
#define ATT_ERR_INVALID_OFFSET          0x07
#define ATT_ERR_INSUFFICIENT_AUTHOR     0x08
#define ATT_ERR_PREPARE_QUEUE_FULL      0x09
#define ATT_ERR_ATTRIBUTE_NOT_FOUND     0x0A
#define ATT_ERR_ATTRIBUTE_NOT_LONG      0x0B
#define ATT_ERR_INSUFFICIENT_EK_SIZE    0x0C
#define ATT_ERR_INVALID_ATTRI_VALUE_LEN 0x0D
#define ATT_ERR_UNLIKELY_ERROR          0x0E
#define ATT_ERR_INSUFFICIENT_ENCRYPTION 0x0F
#define ATT_ERR_UNSUPPORTED_GROUP_TYPE  0x10
#define ATT_ERR_INSUFFICIENT_RESOURCES  0x11
#define ATT_ERR_APPLICATION_ERROR          0x80

typedef struct att_prepare_write_cmd
{
    UINT16 handle;
    UINT16 length;
    UINT16 offset;
    UINT8 *attributeValue;
} att_prepare_write_cmd_t;

typedef struct att_client
{
    UINT16 aclhandle;           //acl handle
    UINT16  trustFlag;
    UINT16  cid;
    UINT16 mtu;
    UINT8  cmdNum;
    UINT8   indTimerId;
    UINT8  idleTimerId;
    UINT16 indHandle;
    bdaddr_t address;
    att_prepare_write_cmd_t *cmdArray;
} att_client_t;

typedef struct
{
    UINT16 aclHandle;
    UINT16 cid;
    UINT16 startHandle;
    UINT16 endHandle;
    UINT16 offset;
    UINT16 attValuelen;//attribute value length
    UINT16 sentLen; //value length currently sent
    UINT16 mtu;
    UINT8  uuid[16];
    UINT8  uuidFmt;
    UINT8   *attValue;//512
    void *tid;
    UINT8 flags;//used by Execute Write Request
} att_req_pdu_t;

typedef struct
{
    att_client_t *pClient;
    UINT16 start_handle;
    UINT16 end_handle;
    UINT16 uuid;
    UINT8 *attValue;
    UINT16 length;
} att_req_handle_t;

typedef struct att_list
{
    UINT16 handle;
    UINT16 valueLen;
    UINT16 proxy_addr;
    gatt_element_t *attribute;
    struct att_list *next;
} att_list_t;



typedef struct att_server_info
{
    UINT16 last_att_handle;
    UINT16 mtu;
    UINT16 clientNum;
    UINT16  module;
    gatt_module_state_cb_t moduleStateCb;
    att_client_t    *clientArray;
    att_list_t *p_att_db_head;
    att_list_t *p_att_db_tail;
} att_server_info_t;

#define ATT_REQ_TIMEOUT         5*100
#define MAX_MODULE_NUM          5

typedef int (*att_client_module_cb)(UINT32 msg, void *param, UINT8 finished, UINT16 acl_handle);

typedef struct general_rsp
{
    UINT16 length;
    UINT8 *payLoad;
} general_rsp_t;

typedef struct att_server
{
    UINT16 aclhandle;
    UINT16  cid;
    UINT16 mtu;
    att_req_pdu_t lastReqPdu;
    general_rsp_t lastRsp;
    general_rsp_t lastRspAlt;
    general_rsp_t lastNoti;
    UINT8   valueInd;
    UINT8   reqTimerId;
} att_server_t;

typedef struct
{
    UINT16 module;
    UINT16 uuid;
    att_client_module_cb moduleStateCb;
} att_client_module_info_t;

typedef struct att_client_info
{
    UINT8 srvNum;
    att_server_t *serverArray;      //server list
    att_client_module_info_t *p_module_list;
    att_client_module_info_t *p_current_module;
} att_client_info_t;

//att protocol
int att_channel_handler(UINT16 aclHandle, UINT8 *payLoad, UINT32 length);
int att_mtu_req(UINT16 cid, UINT16 mtu);
int att_mtu_rsp(UINT16 cid, UINT16 mtu);
int att_error_rsp(UINT16 cid, UINT8 reqOpCode, UINT16 attHandle, UINT8 errCode);
int att_find_info_req(UINT16 cid, UINT16 start_handle, UINT16 end_handle);
int att_find_info_rsp(UINT16 cid, UINT8 fmt, UINT8 *infoData, UINT16 length);
int att_find_by_type_value_rsp(UINT16 cid, UINT8 *handleInfoList, UINT16 length);
int att_find_type_value_req(UINT16 cid, UINT16 start_handle, UINT16 end_handle, UINT8 *uuid, UINT8 uuidFmt, void *value, UINT16 length);
int att_read_by_type_req(UINT16 cid, UINT16 start_handle, UINT16 end_handle, UINT8 *uuid, UINT8 uuidFmt);
int att_read_by_type_rsp(UINT16 cid, UINT8 pairLen, UINT8 *rspPayLoad, UINT16 payLoadLen);
int att_read_req(UINT16 cid, UINT16 handle);
int att_read_rsp(UINT16 cid, const UINT8 *attValue, UINT16 length);
int att_read_blob_req(UINT16 cid, UINT16 handle, UINT16 offset);
int att_read_blob_rsp(UINT16 cid, const UINT8 *attValue, UINT16 length);
int att_read_multiple_req(UINT16 cid, UINT16 *handles, UINT16 length);
int att_read_multiple_rsp(UINT16 cid, UINT8 *attValue, UINT16 length);
int att_read_by_group_type_rsp(UINT16 cid, UINT8 *attValue, UINT8 attValueLen, UINT16 totalLen);
int att_read_by_group_type_req(UINT16 cid, UINT16 start_handle, UINT16 end_handle, UINT8 *uuid, UINT8 uuidFmt);
int att_write_req(UINT16 cid, UINT16 handle, void *buf, UINT16 length);
int att_write_rsp(UINT16 cid);
int att_write_cmd(UINT16 cid, UINT16 handle, void *buf, UINT16 length);
int att_signed_write_cmd(UINT16 cid, UINT16 handle, void *buf, UINT16 length, UINT16 aclHandle);
int att_prepare_write_req(UINT16 cid, UINT16 handle, UINT16 offset, void *buf, UINT16 length);
int att_prepare_write_rsp(UINT16 cid, UINT16 handle, UINT16 offset, UINT8 *attValue, UINT16 length);
int att_execute_write_req(UINT16 cid, UINT8 flags);
int att_execute_write_rsp(UINT16 cid);
int att_handle_value_change(UINT16 cid, UINT16 handle, UINT8 opcode, void *value, UINT16 length);
int att_handle_value_confirmation(UINT16 cid);
int att_client_request(UINT8 opcode, att_req_pdu_t *reqPdu);
int att_rsp_decoder(UINT16 asrhandle, UINT8 opcode, UINT8 *payLoad, UINT16 length );

//att server
void att_server_init(void);
void att_srv_connect_ind(UINT16 aclHandle, UINT16 cid, bdaddr_t address);
void att_srv_disconnect_ind(UINT16 aclHandle, UINT16 cid, UINT8 reason);
void att_srv_handler(UINT16 aclhandle, UINT8 *payLoad, UINT32 length);
void att_srv_paired_ind(void *msg);
void att_srv_handle_value_change(UINT16 aclHandle, UINT16 handle, UINT8 opcode, UINT8 *value, UINT16 length);


void att_get_service_range(UINT16 uuid, UINT16 *start_handle, UINT16 *end_handle);
int att_add_attribute(gatt_element_t *pAttribute, UINT16 *handle);


//att client
void att_client_init(void);
void att_cli_connect_ind(UINT16 aclHandle, UINT16 cid, bdaddr_t addr);
void att_cli_disconnect_ind(UINT16 aclHandle, UINT16 cid, UINT8 reason);

void ble_gatt_init_client(UINT16 module, att_client_module_cb cb, UINT16 uuid);
void ble_att_set_current_module(UINT16 module);

//gatt internal
int gatt_connection_ind(void *tid, UINT16 aclHandle, UINT16 cid, UINT8 result, bdaddr_t addr);
void gatt_disconn_ind(UINT16 aclHandle, UINT16 cid, UINT8 result);
void gatt_exchange_mtu_rsp(UINT16 aclHandle, UINT16 mtu);
void gatt_client_rsp(UINT16 aclHandle, int result, UINT8 *rspPayLoad, UINT16 length, UINT8 continueFlag);
void gatt_server_notification(UINT16 aclHandle, UINT8 *payLoad, UINT16 length);


#endif
