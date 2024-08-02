/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	htmr.h
 * Purpose:	Miscellaneous hardware timers for application usage
 * History:
 *	Jan 5, 2015	jetmotor	Create
 */

#ifndef _HTMR_H_
#define _HTMR_H_

#include "types.h"
#include "trios.h"
#include "gpio.h"
#include "vsh.h"
#include "irq.h"
#include <string.h>


extern void htimer_init(void);

#define OUT_NOACTION	-1
#define OUT_LOW		0
#define OUT_HIGH	1
#define OUT_FLIP	2
#define OUT_PULSE	3

#define HTMR_MATCH_CNT		4
#define HTMR_CNT		4
#define HTMR_IO_OUT_CNT		4
#define HTMR_IO_ACT_CNT		2
#define HTMR_IO_CNT		2

typedef struct io_output_s {
	gpio_dev_t gpio;

	int32_t idx;
	int32_t act[HTMR_IO_ACT_CNT];
} io_output_t;

typedef struct htimer_s {
	int8_t   idx;
	uint32_t mbits;

	uint32_t match[HTMR_MATCH_CNT];
	void     (*mat_op[HTMR_MATCH_CNT])(struct htimer_s *);
	uint32_t capture;
	void     (*cap_op)(struct htimer_s *);

	io_output_t *io[HTMR_IO_CNT];
} htimer_t;

extern __SLOWTEXT void htmr_show(tty_t *term);
/* get_htimer - 
 * ok   : return htmr*
 * error: return NULL
 */
extern htimer_t *get_htimer();

/*input: htimer_t */
extern void put_htimer(htimer_t *htmr);
extern void start_htimer(htimer_t *htmr);
extern void stop_htimer(htimer_t *htmr);
extern __isr__ void isr_htimer(void *data, void *context);

/*get_htimer first
 *input: htimer_t->idx
 *return: htimer_t *
 */
extern htimer_t *get_htimer_point(uint32_t idx);

/*input: htimer_t->idx  match value, off[0-3] */
extern void set_htimer_mat(uint8_t idx, uint32_t match, int32_t n);

/*input: htimer_t->idx
 *desc: when match n timer, clear cnt count.
 */
extern void set_htimer_mat_clr(uint8_t idx, uint32_t n);

/*intput: htimer_t->idx, *gpio, nr_gpio[1-2]
 *return: htimer_t *
 *error:  return NULL
 */
extern htimer_t *get_htimer_io(uint8_t idx, gpio_dev_t gpio[], int32_t nr_gpio);

/*intput: htimer_t->idx, gpio_idx[0-1], act io_out[0-3]
 *return: htimer_t *
 *error:  return NULL
 */
extern htimer_t *set_htimer_io_act(uint8_t idx, int32_t gpio, int32_t act[]);

/*
 * input : htimer_t->idx, gpio_idx[0-1], output[0: low; 1: high]
 * return : 0 --> succ, -1 --> fail
 * */
extern int32_t set_htimer_io_out(uint8_t idx, int32_t gpio, int32_t output);

/*
 * input : htimer_t->idx, gpio_idx[0-1]
 * return : 0 --> low, 1 --> high, -1 --> fail
 * */
extern int32_t get_htimer_io_out(uint8_t idx, int32_t gpio);

/*input: htimer_t->idx match[0-3] mat_op
 *void     (*mat_op)(htimer_t *);
 */
extern void enable_htimer_mat_isr(uint8_t idx, uint32_t match, void (*mat_op)(htimer_t *));

/*external source only have 2(zc0, zc1).
 *so capture interrupt use zc1 source.
 *input 
 *idx:     htimer_t->idx
 *trigger: when rising edge trigger interrupt, trigger:1.
 *         when falling edge trigger interrupt, trigger:2.
 */
extern void enable_htimer_cap_isr(uint8_t idx, uint32_t trigger, void (*cap_op)(htimer_t *));

/*desc: when capture interrupt happen, get cat_cnt val 
 *return: cap_cnt
 */
extern uint32_t get_htimer_cap(uint32_t idx);

extern uint32_t get_htimer_intr_stat();
extern void clr_htimer_intr_stat(uint32_t val);
extern uint32_t get_htimer_cnt(uint32_t idx);
extern uint32_t get_htimer_intr_imsk(uint8_t idx);
extern uint32_t get_htimer_cap(uint32_t idx);

#define HTMR_HZ 		 (50000000)

#define HTMR_S_TICK 		 (HTMR_HZ)
#define HTMR_MS_TICK 		 (HTMR_S_TICK  / 1000)
#define HTMR_US_TICK 		 (HTMR_MS_TICK / 1000)

#define HTMR0_MAT_INTR            0
#define HTMR0_CAP_INTR            16

#endif	/* end of _HTMR_H_ */
