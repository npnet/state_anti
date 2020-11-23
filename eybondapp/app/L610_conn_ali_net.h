#ifndef _L610_CONN_ALI_NET_H_
#define _L610_CONN_ALI_NET_H_
extern char product_key[64] ;
extern char device_name[64] ;
extern char device_secret[64] ;


extern char pub_topic[];
extern char sub_topic[];
void mqtt_conn_ali_task(void *param);
#endif