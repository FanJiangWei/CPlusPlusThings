/*
 * Copyright: (c) 2006-2020, 2016 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	meter.h
 * Purpose:	Meter read and write
 * History:
 *	Oct 26, 2016	xxu   Create
 */

#ifndef _METER_H_
#define _METER_H_

#include "irq.h"
#include "uart.h"
#include "gpio.h"

#define BAUDRATE_1200	    1200
#define BAUDRATE_2400	    2400
#define BAUDRATE_4800	    4800
#define BAUDRATE_9600	    9600	
#define BAUDRATE_19200	    19200	
#define BAUDRATE_38400	    38400	
#define BAUDRATE_57600	    57600	
#define BAUDRATE_115200	    115200
#define BAUDRATE_230400	    230400	
#define BAUDRATE_460800	    460800	
#define BAUDRATE_921600	    921600	

#if defined(STATIC_MASTER)
//#define HIGH_FREQUENCY 		//台体高频采集,才需要定义，普通9600需要注释
#else
                            //STA无需设置	
#endif

#if defined(STATIC_NODE)
#define UART1_BAUDRATE	BAUDRATE_1200	/* used by meter */
#define UART2_BAUDRATE  BAUDRATE_1200
#elif defined(STATIC_MASTER)
#if defined(HIGH_FREQUENCY)
#define UART1_BAUDRATE	BAUDRATE_115200
#else
#define UART1_BAUDRATE	BAUDRATE_9600
#endif
#else
#define UART1_BAUDRATE	BAUDRATE_2400
#endif


#define printf_ir(fmt, arg...) {app_printf("[IR]");app_printf(fmt, ##arg);}




extern ostimer_t *RxLedtimer;


enum
{
    METER_RX_STATUS,
    METER_TX_STATUS,
    METER_BUTT_STATUS,
};
enum
{
    METER_IDLE_STATUS    = 0x00,
    METER_TX_EMPTY       = 0x01,
    METER_RX_16          = 0x02,
    METER_RX_OVER_FIFO   = 0x04,
    METER_RX_TIMEOUT     = 0x08,
	METER_RX_TOGETHER    = 0x10,
};
enum
{
    METER_IDLE,
    METER_TX_OK,
    METER_RX_OK,
    METER_RX_ERR,
};
enum
{
    METER_TX_IDLE,
    METER_TX_SET,
    METER_TX_FINISH,
};

#define METER_BUF_SIZE 			(2096)
typedef struct meter_dev_s
{
	gpio_dev_t	*gpio;
	uint8_t  uart;
	uint32_t baudrate;
	uint8_t  irq;
	uint8_t  mode; /* default 1; 0, polling_mode; 1, intr_mode */

	void    (*set_mode)(struct meter_dev_s *dev, uint8_t mode);
	int32_t (*master_read)(struct meter_dev_s *dev, uint8_t *buf, uint32_t len);
	int32_t (*master_read_timeout)(struct meter_dev_s *dev, uint8_t *buf, uint32_t len, uint32_t timeout);
	int32_t (*master_write)(struct meter_dev_s *dev, uint8_t *buf, uint32_t len);

	/* mode=1 intr_mode */
	event_t *uart_event;
	uint32_t rx_total, tx_total;
	uint8_t  tx_fifo_twm;
	uint8_t  send_buf[METER_BUF_SIZE];
	uint32_t send;
	uint32_t write;

	uint8_t  rx_fifo_rwm;
	uint8_t  recv_buf[METER_BUF_SIZE];
	uint32_t recv;
	uint32_t read;

	uint32_t debug_twm;
	uint32_t debug_tx;
	uint32_t debug_tx_empty;
	uint32_t debug_tx_twm;
	uint32_t debug_tx_full;

	uint32_t debug_rx;
	uint32_t debug_rx_timeout;
	uint32_t debug_rx_err;
	uint32_t debug_rx_full;
	uint32_t debug_patch;
    uint32_t status;
} meter_dev_t;

extern meter_dev_t *g_meter;

typedef enum
{
    e_Decode_Fail,
    e_Decode_Success,
} DecodeStatus_e;


extern meter_dev_t *g_infrared;

extern void meter_uart_init(gpio_dev_t *dev);

/* 
 * input: buf, len
 * return n:   total tx  (n)  byte 
 * n <= len
 * */
extern uint32_t meter_write_tx(meter_dev_t *dev, uint8_t *buf, uint32_t len);

/*
 * intput: buf
 * return n: total rx (n) byte to buf
 * n < METER_BUF_SIZE 
 * */
extern uint32_t meter_read_rx(meter_dev_t *dev, uint8_t *buf);

typedef void (* uart_recv_fn)(uint8_t *buf, uint32_t len);
typedef void (* uart_send_fn)(uint8_t *buf, uint32_t len);
extern void register_uart1_recv_proc(uart_recv_fn fn);
extern void uart1_send(uint8_t *buf, uint32_t len);
extern void test_mode_uart_init();
extern void test_mode_uart_exit();
extern void test_mode_send(uint8_t *buf, uint32_t len);
extern int32_t read_meter_addr(void);
extern int convert_buf(uint8_t *buf, uint8_t *cmd, int len);

extern void    meter_set_mode(meter_dev_t *dev, uint8_t mode);
extern int32_t meter_uart_read(meter_dev_t *dev, uint8_t *buf, uint32_t len);
extern int32_t meter_uart_write(meter_dev_t *dev, uint8_t *buf, uint32_t len);
extern int32_t meter_uart_read_timeout(meter_dev_t *dev, uint8_t *buf, uint32_t len, uint32_t timeout);
extern void meter_task_init();
extern void dump_buf(uint8_t *buf, uint32_t len);
extern void MeterUartInit(meter_dev_t *dev, uint32_t Baud);

extern __isr__ void meter_uart_tx(meter_dev_t *dev);
extern __isr__ void meter_uart_tx_empty(meter_dev_t *dev);
extern __isr__ void meter_uart_rx(meter_dev_t *dev);
extern void resetmeter(void);
extern void infrared_send(uint8_t *buffer, uint16_t len);
extern uint8_t PTCL_Decode(uint8_t sendflag,uint8_t *buf, uint32_t *len, uint8_t Protocol ,uint8_t port);
extern void uart_send(uint8_t *buffer, uint16_t len);
#endif	/* end of _METER_H_ */
