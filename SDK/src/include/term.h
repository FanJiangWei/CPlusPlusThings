/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        term.h
 * Purpose:     Terminal data structure.
 * History:     4/30/2007, created by jetmotor.
 */

#ifndef _TERM_H
#define _TERM_H


#include "types.h"
#include "list.h"
#include "trios.h"


/**
 * Define the Key codes (ASCII control codes). The Ctrl key is used in conjunction with
 * displayable keys (including alphabetic and non-alphabetic keys) to have their ASICii
 * codes transformed into control codes. The shell command `showkey' gives the details.
 */
#define KEY_NUL		0	/* ^@ Null character */
#define KEY_SOH		1	/* ^A Start of heading, = console interrupt */
#define KEY_STX		2	/* ^B Start of text, maintenance mode on HP console */
#define KEY_ETX		3	/* ^C End of text */
#define KEY_EOT		4	/* ^D End of transmission, not the same as ETB */
#define KEY_ENQ		5	/* ^E Enquiry, goes with ACK; old HP flow control */
#define KEY_ACK		6	/* ^F Acknowledge, clears ENQ logon hand */
#define KEY_BEL		7	/* ^G Bell, rings the bell... */
#define KEY_BS		8	/* ^H Backspace, works on HP terminals/computers */
#define KEY_HT		9	/* ^I Horizontal tab, move to next tab stop */
#define KEY_LF		10	/* ^J Line Feed */
#define KEY_VT		11	/* ^K Vertical tab */
#define KEY_FF		12	/* ^L Form Feed, page eject */
#define KEY_CR		13	/* ^M Carriage Return*/
#define KEY_SO		14	/* ^N Shift Out, alternate character set */
#define KEY_SI		15	/* ^O Shift In, resume defaultn character set */
#define KEY_DLE		16	/* ^P Data link escape */
#define KEY_DC1		17	/* ^Q XON, with XOFF to pause listings; "okay to send". */
#define KEY_DC2		18	/* ^R Device control 2, block-mode flow control */
#define KEY_DC3		19	/* ^S XOFF, with XON is TERM=18 flow control */
#define KEY_DC4		20	/* ^T Device control 4 */
#define KEY_NAK		21	/* ^U Negative acknowledge */
#define KEY_SYN		22	/* ^V Synchronous idle */
#define KEY_ETB		23	/* ^W End transmission block, not the same as EOT */
#define KEY_CAN		24	/* ^X Cancel line, MPE echoes !!! */
#define KEY_EM		25	/* ^Y End of medium, Control-Y interrupt */
#define KEY_SUB		26	/* ^Z Substitute */
#define KEY_ESC		27	/* ^[ Escape, next character is not echoed */
#define KEY_FS		28	/* ^\ File separator */
#define KEY_GS		29	/* ^] Group separator */
#define KEY_RS		30	/* ^^ Record separator, block-mode terminator */
#define KEY_US		31	/* ^_ Unit separator */
#define KEY_BACKSPACE	127	/* Backspace (not a real control character...) */


#define ESC	"\x1b"

/**
 * Define the special functional keys
 */
#define VTAB		0
#define VPREV		1
#define VNEXT		2
#define NR_HOOK_MAX	3



#define TTY_QUE_BUF_SIZE_MAX	256

/**
 * tty_que_t - A cyclic buffer for tty rx or tx processing. Notice: there's a trick to
 *	initialize its `rp' and `wp' and then determine its empty or full when used as
 *	a read_q or write_q.
 *
 * @buf:
 * @rp:		read pointer
 * @wp:		write pointer
 * @m:		mutex
 * @res:	resource
 */
typedef struct tty_que_s{
	uint8_t buf[TTY_QUE_BUF_SIZE_MAX];
	int32_t rp;
	int32_t wp;

	event_t mutex;
	event_t empty;
	event_t full;
}tty_que_t;

#define tty_que_empty(ttyq)	((ttyq)->rp == (ttyq)->wp)
#define tty_que_full(ttyq, wp)	(((wp) = (((ttyq)->wp+1) & (TTY_QUE_BUF_SIZE_MAX-1))) == (ttyq)->rp)


struct tty_s;
/**
 * tty_getc - Read one character from the device that tty's running on.
 * @tty:	the tty
 * @timeout:	timeout in OS_TICKS, namely, MS_PER_OS_TICKS ms
 * @return:	the character
 *		-1, timeout
 */
typedef int32_t (*tty_getc_fp)(struct tty_s *tty, uint32_t timeout);
/**
 * tty_putc - Write one character into the device that tty's running on.
 * @tty:	the tty
 * @c:		the character
 * @return:
 */
typedef int32_t (*tty_putc_fp)(struct tty_s *tty, char c);
/**
 * tty_printf - Write one string into the device that tty's running on.
 * @tty:	the tty
 * @fmt:	output format string
 * @...:	variable arguments
 * @return:
 */
typedef int32_t (*tty_printf_fp)(struct tty_s *tty, char *fmt, ...);
/**
 * tty_puts - Write one string into the device that term is running on.
 * @term:	the term
 * @s:		the string
 * @return:
 */
typedef int32_t (*tty_puts_fp)(struct tty_s *tty, char *s);
/**
 * tty_puts - Write one buffer's content into the device that tty's running on.
 * @tty:	the tty
 * @buf:	the buffer
 * @size:	buffer size
 * @return:
 */
typedef int32_t (*tty_write_fp)(struct tty_s *tty, char *buf, int32_t size);

/**
 * dokey - Process one key stroke according to the current state of tty
 * @return:	>0, succeed;
 *		<0, fail, may make the bell ring
 */
typedef int32_t (*tty_dokey_fp)(struct tty_s *tty, char key);
/**
 * hook - Contex of tty providing external processing functions
 * @contex:	the contex
 */
typedef void (*context_hook_fp)(void *context);


#define TTY_BUF_SIZE_MAX		TTY_QUE_BUF_SIZE_MAX

#define TTY_SEEK_BEG	1
#define TTY_SEEK_CUR	2
#define TTY_SEEK_END	3

#define TTY_DRAIN_BEG	1
#define TTY_DRAIN_CUR	2

/**
 * tty_t - A data structure to manipulate terminal. Currently, tty_t is implemented as a vt100
 *	terminal through serial port. tty_t receives control sequences from the other end of
 *	theserial line, perform processing and then write back particular control sequences to
 *	the other end.
 *
 * @op:		tty operation suite
 * @read_q:	the raw input data
 * @write_q:	the output data
 * @done:	tty_t read line complete or not
 * @modified:	if redisplay necessary or not
 * @line:	the cooked line
 * @point:	the position where redisplay begins if necessary
 * @cursor:	the cursor position
 * @end:	the position where new characters appended
 * @prev_pos:	saved position
 * @killed:	place for the cooked line just been killed
 * @killed_size:number of characters in the killed cooked line
 * @handlers:	key stroke handlers, most of them are default handlers, some of them are special
 *		handlers such as tab, carriage return, etc.
 * @context:	the running environment
 * @hooks:	context provided hooks for some spectial functions
 */
typedef struct tty_s{
	struct {
		/* read one character from device */
		tty_getc_fp tgetc;
		/* write one character into device */
		tty_putc_fp tputc;
		/* write one string with format into device */
		tty_printf_fp tprintf;
		/* write one string into device */
		tty_puts_fp tputs;
		tty_puts_fp tputs_noblk;
		/* write one buffer into device */
		tty_write_fp twrite;

		int32_t (*ioctl)(struct tty_s *tty, int32_t request, ...);
		/* get one cooked line, it returns only when the line completes */
		int32_t (*getline)(struct tty_s *tty, char *buf, int32_t size);
		/* read the current cooked line's content, it returns immediately */
		int32_t (*read)(struct tty_s *tty, char *buf, int32_t size);
		/* seek the cursor position */
		int32_t (*seek)(struct tty_s *tty, int32_t offset, int32_t origin);
		/* insert string into cooked line */
		int32_t (*insert)(struct tty_s *tty, char *s);
		/* kill the current cooked line */
		void (*clrline)(struct tty_s *tty);
		/* redisplay according to the mode */
		void (*drain)(struct tty_s *tty, int32_t mode);
		/* will be deprecated */
		void (*setprefix)(struct tty_s *tty, int32_t len);
	} __op, *op;

	tty_que_t read_q;
	tty_que_t write_q;
	
	int32_t done;
	int32_t modified;
	
	char buf[TTY_BUF_SIZE_MAX];
	int32_t len;
	int32_t prefix;
	int32_t beg, end;		/* redisplay the content between the beginning and the end */
	int32_t cursor;
	int32_t line;

	int32_t x, y;
	int32_t max_x, max_y;

	char killed[TTY_BUF_SIZE_MAX];
	int32_t killed_size;

	tty_dokey_fp handlers[KEY_BACKSPACE + 1];

	void *context;
	context_hook_fp hooks[NR_HOOK_MAX];

	int16_t uart_idx;
	int16_t uart_irq;

	uint32_t irq;
	uint32_t mintr;
	uint32_t rda;
	uint32_t rto;
	uint32_t tfe;
	uint32_t twm;
	uint32_t full;
} tty_t;

#define TTY_PANIC()	sys_panic("<TTY panic> %s : %d\n", __func__, __LINE__)


extern tty_t tty;


extern void tty_init(tty_t *tty,
		     char **name,
		     int32_t uart,
		     int32_t irq,
		     tty_getc_fp tgetc,
		     tty_putc_fp tputc,
		     tty_printf_fp tprintf,
		     tty_puts_fp tputs,
		     tty_puts_fp tputs_noblk,
		     tty_write_fp twrite);

extern int32_t tty_dev_getc(tty_t *tty, uint32_t timeout);
extern int32_t tty_dev_putc(tty_t *tty, char c);
extern int32_t tty_dev_printf(tty_t *tty, char *fmt, ...);
extern int32_t tty_dev_puts(tty_t *tty, char *s);
extern int32_t tty_dev_write(tty_t *tty, char *buf, int32_t size);
extern int32_t tty_ioctl(struct tty_s *tty, int32_t request, ...);

__isr__ extern void isr_uart(void *data, void *context);
__isr__ extern void tty_master_write(tty_t *tty);
__isr__ extern void tty_master_read(tty_t *tty);

extern int32_t tty_slave_getc(tty_t *tty, uint32_t timeout);
extern int32_t tty_slave_putc(tty_t *tty, char c);
extern int32_t tty_slave_printf(tty_t *tty, char *fmt, ...);
extern int32_t tty_slave_puts(tty_t *tty, char *s);
extern int32_t tty_slave_write(tty_t *tty, char *buf, int32_t size);


#endif	/* end of _TERM_H */
