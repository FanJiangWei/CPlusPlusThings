/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        uart.h
 * Purpose:
 * History:	4/30/2007, created by jetmotor.
 */

#ifndef _UART_H
#define _UART_H

#include "include/types.h"
#include "trios.h"

#define UART_QUE_BUF_SIZE_MAX	1024
#define TASK_UART_STK_SZ	1024
#define TASK_UART_PRIO		63

/**
 * uart_que_t - A cyclic buffer for uart rx or tx processing. Notice: there's a trick to
 *	initialize its `rp' and `wp' and then determine its empty or full when used as
 *	a read_q or write_q.
 *
 * @buf:
 * @rp:		read pointer
 * @wp:		write pointer
 * @m:		mutex
 * @res:	resource
 */
typedef struct uart_que_s {
	uint8_t buf[UART_QUE_BUF_SIZE_MAX];
	int32_t rp;
	int32_t wp;

	event_t mutex;
	event_t empty;
	event_t full;
} uart_que_t;

#define uart_que_empty(uartq)		((uartq)->rp == (uartq)->wp)
#define uart_que_full(uartq, wp)	(((wp) = (((uartq)->wp+1) & (UART_QUE_BUF_SIZE_MAX-1))) == (uartq)->rp)

typedef struct uart_reg_s {
	union {
		unsigned int rxb;	/* DLAB=0: receive buffer, read-only */
		unsigned int txb;	/* DLAB=0: transmit buffer, write-only */
		unsigned int dll;	/* DLAB=1: divisor, least-significant byte latch (was write-only?) */
	} w0;

	union {
		unsigned int ier;	/* DLAB=0: interrupt-enable register (was write-only?) */
		unsigned int dlm;	/* DLAB=1: divisor, most-significant byte latch (was write-only?) */
	} w1;

	union {
		unsigned int iir;	/* DLAB=0: interrupt identification register, read-only */
		unsigned int fcr;	/* DLAB=0: FIFO control register, write-only */
		unsigned int afr;	/* DLAB=1: alternate function register */
	} w2;

	unsigned int lcr;		/* line control-register, write-only */
	unsigned int mcr;		/* modem control-regsiter, write-only */
	unsigned int lsr;		/* line status register, read-only */
	unsigned int msr;		/* modem status register, read-only */
	unsigned int scr;		/* scratch regsiter, read/write */
} uart_reg_t;

#define IR_CARRIER_MOD_OR	0
#define IR_CARRIER_MOD_AND	1
typedef struct ir_cfg_s {
	uint32_t enable :1;
	uint32_t mode :1;
	uint32_t tx_invert :1;
	uint32_t rx_invert :1;
	uint32_t remote :1;
	uint32_t rx_cnt :10;
	uint32_t high :8;
	uint32_t low :8;
	uint32_t :1;

	uint32_t freq;
} ir_cfg_t;

typedef struct uart_dev_s {
	uint16_t idx;
	int8_t   rxpin;
	int8_t   txpin;
	uint32_t base;

	volatile uart_reg_t *reg;
} uart_dev_t;

extern uart_dev_t uart[];
#define uart_reg(idx)	(uart[idx].reg)


/*  UART present.  */
enum {
	UART0 = 0,	/* used to debug output */
	UART1,		/* used to communication */
	UART2,
#if defined(FPGA_VERSION) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
	UART3,
	UART4,
#endif
#if defined(VENUS_V3) || defined(VENUS_V5)
	UART5,
	UART6,
	UART7,
#endif
	UART_NR,
};

#define _RXB(ur)			((ur)->w0.rxb)
#define _TXB(ur)			((ur)->w0.txb)
#define _DLL(ur)			((ur)->w0.dll)
#define _IER(ur)			((ur)->w1.ier)
#define _DLM(ur)			((ur)->w1.dlm)
#define _IIR(ur)			((ur)->w2.iir)
#define _FCR(ur)			((ur)->w2.fcr)
#define _AFR(ur)			((ur)->w2.afr)
#define _LCR(ur)			((ur)->lcr)
#define _MCR(ur)			((ur)->mcr)
#define _LSR(ur)			((ur)->lsr)
#define _MSR(ur)			((ur)->msr)
#define _SCR(ur)			((ur)->scr)

/* IER bits */
#define IER_RDA				0x01	/* Received data available Interrup enable */
#define IER_THRE			0x02	/* Transmitter holding register empty Interrup enable */
#define IER_RLS				0x04	/* Receiver line status register change Interrupt enable */
#define IER_MS				0x08	/* Modem status register change Interrupt enable */
#define IER_RTO				0x10	/* Receive fifo non-empty timeout interrupt */
#define IER_TWM				0x40	/* The bytes number in transmit fifo is lower than watermark interrupt */
#define IER_RX_MASK			(IER_RDA | IER_RLS | IER_RTO)

/* FCR bits */
#define	FCR_RX_FIFO_RESET		0x02
#define	FCR_TX_FIFO_RESET		0x04
#define	FCR_DMA_MODE_SELECT		0x08
#define	FCR_RX_TRIGGER_LSB		0x40
#define	FCR_RX_TRIGGER_MSB		0x80

/* IIR bits */
#define IIR_MASK			0x0E
#define	IIR_TX_HOLD_REG_EMPTY		0x02	
#define	IIR_RX_DATA_AVAILABLE		0x04
#define	IIR_RX_LINE_STATUS		0x06
#define IIR_RX_DATA_TIMEOUT		0x0C

/* LCR bits */
#define WORD_LENGTH(n)			(((n)-5) & 0x3)
#define LCR_STOP_BIT_ENABLE		0x04
#define LCR_PARITY_ENABLE		0x08
#define LCR_EVEN_PARITY			0x10
#define LCR_FORCE_PARITY		0x20
#define LCR_XMIT_BREAK			0x40
#define LCR_DLAB_ENABLE			0x80

/* LSR Bits */
#define LSR_DR				0x01 /* Receive Data Ready (RDR) indicator (Receive FIFO non-empty) */
#define LSR_OE				0x02 /* Overrun Error(OE) indicator */
#define LSR_PE				0x04 /* Parity Error(PE) indicator */
#define LSR_FE				0x08 /* Framing Error(FE) indicator */
#define LSR_BI				0x10 /* Break Interrupt(BI) indicator */
#define LSR_TFE				0x20 /* Transmit FIFO empty */
#define LSR_TE				0x40 /* Transmitter Empty indicator (Transmit FIFO empty && the last bit of the last byte already transmitted) */
#define LSR_EI				0x80 /* At least one parity error, Rx FIFO error */
#define LSR_RTO				0x100/* Receive fifo non-empty timeout */
#define LSR_TWM				0x200/* Transmit fifo underflow */
#define LSR_ERR				(LSR_OE | LSR_PE | LSR_FE | LSR_BI | LSR_EI)

#define TX_READY(u)			(_LSR(u) & LSR_TFE)
#define UART_TX_FIFO_MAX 		256
#define UART_RX_FIFO_MAX 		256

/* bit3 : LC_PE, parity enable
 * bit4 : LC_EP, b1: Even parity, b0: Odd parity */
#define PARITY_NONE	(0x0)	// bit3 = 0
#define PARITY_ODD	(0x1)	// bit3 = 1. bit4 = 0
#define PARITY_EVEN	(0x3)	// bit3 = 1, bit4 = 1 

#define DATABITS_5	(5)
#define DATABITS_6	(6)
#define DATABITS_7	(7)
#define DATABITS_8	(8)

#define STOPBITS_1	(0)
#define STOPBITS_1_5	(1)	// 1.5 stop bits when 5-bit character length selected and 2 bits otherwise

static __inline__ int32_t meter_uart_check_rda_int(uint32_t ier, uint32_t iifc, uint32_t lsr)
{
	return ((ier & IER_RDA) && ((iifc & 0x1) == 0) && ((iifc & IIR_MASK) == IIR_RX_DATA_AVAILABLE));
}

static __inline__ int32_t meter_uart_check_thre_int(uint32_t ier, uint32_t iifc, uint32_t lsr)
{
	// return ((ier & IER_THRE) && (lsr & LSR_TFE));
	return ((ier & IER_THRE) && ((iifc & 0x1) == 0) &&
		((iifc & IIR_MASK) == IIR_TX_HOLD_REG_EMPTY) && (lsr & LSR_TFE));
}

static __inline__ int32_t meter_uart_check_rls_int(uint32_t ier, uint32_t iifc, uint32_t lsr)
{
	// return ((ier & IER_RLS) && (lsr & LSR_ERR));
	return ((ier & IER_RLS) && (((iifc & 0x1) == 0 &&
		(iifc & IIR_MASK) == IIR_RX_LINE_STATUS) || (lsr & LSR_ERR)));
}

static __inline__ int32_t meter_uart_check_rto_int(uint32_t ier, uint32_t iifc, uint32_t lsr)
{
	// return ((ier & IER_RTO) && (lsr & LSR_RTO));
	return ((ier & IER_RTO) && ((iifc & 0x1) == 1) && (lsr & LSR_RTO));
}

static __inline__ int32_t meter_uart_check_twm_int(uint32_t ier, uint32_t iifc, uint32_t lsr)
{
	// return ((ier & IER_TWM) && (lsr & LSR_TWM));
	return ((ier & IER_TWM) && ((iifc & 0x1) == 1) && (lsr & LSR_TWM));
}


/* uart_init - initialize the uart
 * @idx: uart idx
 * @baudrate: 
 */
extern void uart_init(uint32_t idx, uint32_t baudrate, int32_t parity, int32_t stop, int32_t bits);
/* uart_flush_rx - uart reset rx fifo
 * @idx: uart idx
 */
extern void uart_flush_rx(uint32_t idx);
/* uart_flush_tx - uart reset tx fifo
 * @idx:
 */
extern void uart_flush_tx(uint32_t idx);
/* uart_putc - put a character to uart
 * @idx: uart idx
 * @ch: the character
 */
extern void uart_putc(uint32_t idx, char ch);
/* uart_putc_non_block - put a char to uart non-block
 * @idx: uart idx
 * @ch: the character
 * @return: 0 - put the char successful
 *	   <0 - block 
 */
extern int32_t uart_putc_non_block(uint32_t idx, char ch);
/* uart_puts - put string to uart & conversion LF to CR-LF
 * @idx: uart idx
 * @string: the string
 */
extern void uart_puts(uint32_t idx, char *string);
/* uart_getc - get a character from uart within required time
 * @idx: uart idx
 * @timeout: required time
 * @return: < 0 - get character error
 *          > 0 - the character
 */
extern int32_t uart_getc(uint32_t idx, uint32_t timeout);
/* uart_getc_non_block - get a char from uart with non-block
 * @idx: uart idx
 * @return: <0 - block, get char fail
 *	    >0 - the character
 */
extern int32_t uart_getc_non_block(uint32_t idx);
/* uart_getline - get string from uart within requried time
 * @idx: uart idx
 * @buf: the buffer for storing received string
 * @size: the size of buffer
 * @timeout: required time(unit: ms)
 * @return: <= 0 - get string error
 *           > 0 - the length of received string
 */
extern int32_t uart_getline(uint32_t idx, char *buf, uint32_t size, uint32_t timeout);
/* uart_get_tf_cnt - get the currently used size of transmit fifo (the max size of transmit fifo is 256)
 * @idx: uart idx
 * @return: used size
 */
extern uint32_t uart_get_tf_cnt(uint32_t idx);
/* uart_get_rf_cnt - get the currently used size of receive fifo 
 * @idx: uart idx
 * @return: used size
 */
extern uint32_t uart_get_rf_cnt(uint32_t idx);
/* uart_ctrlc - get the character and judge whether it is ASCII of '^C' (Ctrl - c)
 * @idx: uart idx
 * @return: 1 - TRUE, 0 - FALSE
 */
extern int32_t uart_ctrlc(uint32_t idx);
/* uart_set_txbuf - set tx buf 
 * @idx: uart idx
 * @val: buf
 */
extern void uart_set_txbuf(uint32_t idx, uint32_t val);
/* uart_get_rxbuf - get uart output character 
 * @idx: uart idx
 * @return: character
 */
extern char uart_get_rxbuf(uint32_t idx);
/* uart_get_int_enable - get interrupt enable register value
 * @idx: uart idx
 * @return: interrupt enable reg
 */
extern uint32_t uart_get_int_enable(uint32_t idx);
/* uart_set_int_enable - set interrupt enable register
 * @idx: uart idx
 * @ier_val: interrupt enable register val
 */
extern void uart_set_int_enable(uint32_t idx, uint32_t ier_val);
/* uart_get_int_info - get interrupt information register
 * @idx: uart idx
 * @return: register value
 */
extern uint32_t uart_get_int_info(uint32_t idx);
/* uart_set_fifo_ctrl - set fifo control redister
 * @idx: uart idx
 * @val: register value
 */
extern void uart_set_fifo_ctrl(uint32_t idx, uint32_t val);

/* uart_get_line_ctrl - get line control regoster value
 * @idx: uart idx
 * @return: register val
 * */
extern uint32_t uart_get_line_ctrl(uint32_t idx);
/* uart)set_line_ctrl - set line control register
 * @uart: uart dev
 * @lcr_val: register value
 */
extern void uart_set_line_ctrl(uint32_t idx, uint32_t lcr_val);
/* uart_get_line_status - get line status register value
 * @idx: uart idx
 * @return: register val
 */
extern uint32_t uart_get_line_status(uint32_t idx);
/* uart_rx_ready - check uart rx status if ready
 * @idx: uart idx
 * @return: non-zero = ready, zero = not ready
 * */
extern int32_t uart_rx_ready(uint32_t idx);
/* uart_tx_ready - check uart tx status if ready
 * @idx: uart idx
 * @return: non-zero = ready, zero = not ready
 * */
extern int32_t uart_tx_ready(uint32_t idx);
/* uart_set_rx_fifo_threshold - set rx fifo thd
 * @idx: uart idx
 * @thd: threshold value(watermark) range: (0~15)
 * input: level 0~15 0:1 1:16 2:32 3:48 ... 14:16*14=224 15:256
 * 0, 15 is special
 */
extern void uart_set_rx_fifo_threshold(uint32_t idx, uint8_t threshold);
/* uart_set_tx_fifo_threshold - set tx fifo thd
 * @idx: uart idx
 * @thd: threshold value(watermark) range:(1~255)
 */
extern void uart_set_tx_fifo_threshold(uint32_t idx, uint8_t threshold);
/* uart_set_rx_fifo_non_empty_time
 * @idx: uart idx
 * @time: timeout time
 * timeout: 150/uart_clk*time
 * eg. uart_clk=50M time=0x3ff=1023 timeout=150/50*1024=3072(us)
 */
extern void uart_set_rx_fifo_non_empty_time(uint32_t idx, uint16_t time);
/* uart_set_rx_data_avail_int - set received data available interrupt
 * @idx: uart idx
 * @int_flag: interrupt flag enable(TRUE) or disable(FALSE)
 */
extern void uart_set_rx_data_avail_int(uint32_t idx, uint32_t int_flag);
/* uart_set_tx_hold_reg_empty_int - set transmitter holding register interrupt
 * @idx: uart idx
 * @int_flag: interrupt flag enable(TRUE) or disable(FALSE)
 */
extern void uart_set_tx_hold_reg_empty_int(uint32_t idx, uint32_t int_flag);
/* uart_set_rx_fifo_non_empty_int - set receive fifo non-empty timeout interrupt
 * @idx: uart idx
 * @int_flag: interrupt flag enable(TRUE) or disable(FALSE)
 */
extern void uart_set_rx_fifo_non_empty_int(uint32_t idx, uint32_t int_flag);
/* uart_set_tx_watermark_int - set transmit watermark interrupt
 * @idx: uart idx
 * @int_flag: interrupt flag enable(TRUE) or disable(FALSE)
 */
extern void uart_set_tx_watermark_int(uint32_t idx, uint32_t int_flag);
extern void uart_set_rx_line_status_int(uint32_t idx, uint32_t int_flag);

/* uart_clear_mintr - clear uart module interrupt
 * @idx: uart idx
 */
extern void uart_clear_mintr(uint32_t idx);

extern void uart_enable_auto_baud(uint32_t idx);
extern void uart_disable_auto_baud(uint32_t idx);
extern uint32_t uart_get_auto_baud(uint32_t idx);
extern void uart_clr_auto_baud(uint32_t idx);
extern void uart_ir_enable(uint32_t idx, uint32_t en);
extern int32_t uart_ir_cfg(uint32_t idx, ir_cfg_t *cfg);
#endif	/* end of _UART_H */

