#include "uart_operate.h"
#include "fibo_opencpu_comm.h"
#include "oc_uart.h"
#include "elog.h"
#include "debug_cmd.h"

UINT16 	uart1_recv_len					    =	 0;			//返回实际接收的字节数
char 	uart1_recv_data[UART_BUFFER_SIZE]   =   {0}; 

UINT16	uart2_recv_len	                    =    0;			//返回实际接收的字节数
char	uart2_recv_data[UART_BUFFER_SIZE]	=	{0}; 

//串口1默认配置
hal_uart_config_t   uart1_cfg  = {
    .baud                 = 9600,                   //波特率9600
    .parity               = HAL_UART_NO_PARITY,     //无校验
    .data_bits            = HAL_UART_DATA_BITS_8,   //8位数据位
    .stop_bits            = HAL_UART_STOP_BITS_1,   //1位停止位
    .rx_buf_size          = UART_RX_BUF_SIZE,       //接收缓冲区大小
    .tx_buf_size          = UART_TX_BUF_SIZE,       //发送缓冲区大小
};

//串口2默认配置
hal_uart_config_t   uart2_cfg  = {
    .baud                 = 115200,                 //波特率115200
    .parity               = HAL_UART_NO_PARITY,     //无校验
    .data_bits            = HAL_UART_DATA_BITS_8,   //8位数据位
    .stop_bits            = HAL_UART_STOP_BITS_1,   //1位停止位
    .rx_buf_size          = UART_RX_BUF_SIZE,       //接收缓冲区大小
    .tx_buf_size          = UART_TX_BUF_SIZE,       //发送缓冲区大小
};



//串口1接收回调
void uart1_recv_cb(hal_uart_port_t uart_port, UINT8 *data, UINT16 len, void *arg)
{
    log_d("uartapi recv uart_port=%d len=%d, data=%s", uart_port, len, (char *)data);

	uart1_recv_len = len;

	int cmp = 1;
	char *buf="AT+TEST";
	cmp = memcmp(data,buf,7);

	if(0 == cmp)
	{
		char uart1_recv_data_copy[UART_BUFFER_SIZE]={0};
		memcpy(uart1_recv_data_copy,data,uart1_recv_len); 
		STRCMD_AtTest(uart1_recv_data_copy);
		memset(uart1_recv_data, 0, sizeof(uart1_recv_data)); 
		uart1_recv_len = 0;
	}

	// dev_access_info();  硕日专用
}

//串口2接收回调
void uart2_recv_cb(hal_uart_port_t uart_port, UINT8 *data, UINT16 len, void *arg)
{
    log_d("uartapi recv uart_port=%d len=%d, data=%s", uart_port, len, (char *)data);

	uart2_recv_len = len;

	char uart2_recv_data_copy[UART_BUFFER_SIZE]={0};

	if(uart2_recv_len>5)
	{
		memcpy(uart2_recv_data_copy,data,uart2_recv_len); 
		uart_set_get_para(uart2_recv_data_copy);	
	}
}

//串口开
INT32 uart_init(hal_uart_port_t uart_port, hal_uart_config_t *uart_config, uart_input_callback_t recv_cb, void *arg)
{
    INT32   ret = fibo_hal_uart_init(uart_port,uart_config,recv_cb,arg);
    return  ret;
}

//串口写
INT32 uart_write(hal_uart_port_t uart_port, UINT8 *buff, UINT32 len)
{
    INT32   ret = fibo_hal_uart_put(uart_port,buff,len);
    return  ret;
}

//串口关
INT32 uart_deinit(hal_uart_port_t uart_port)
{
    INT32   ret = fibo_hal_uart_deinit(uart_port);
    return  ret;
}