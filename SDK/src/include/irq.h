/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	irq.h
 * Purpose:	System-level interrupt request handling.
 * History:	11/07/2008, created by jetmotor
 */

#ifndef _IRQ_H
#define _IRQ_H


#include "types.h"
#include "list.h"


extern uint32_t CPU_IRQ_START, CPU_IRQ_STOP;
/*
 * Various CPUs provide various external interrupt lines for system usage. Typically, only a small continuous part
 * of all the external interrutp lines are utilized, which is indicated by `CPU_IRQ_START' and `CPU_IRQ_STOP'.
 *
 *                   CPU_IRQ_START               CPU_IRQ_STOP
 *                        |                           |
 *                        v                           v
 * CPU irq line:       0  1  2  3  4  5  6  7  8  9 ...
 *                   ------------------------------------------------------
 * MODULE irq lines:         ^
 *                           | dispatch a MODULE irq line to certain CPU irq line
 *                           |          _____ 
 *                           +------  0|_____|
 *                           ^        1|_____|
 *                           |        2|_____|
 *                           |        3|_____|
 *                           |         
 *                           |         ~     ~
 *                           |       24|_____|
 *                           |       25|_____|
 *                           +------ 26|_____|
 *
 *                             a CPU irq line can be multiplexed by any number of MODULE irq lines
 */

typedef void (*irq_handler_fp)(void *data, void *context);

#define NR_C_IRQ_MAX	15


enum {
#if defined(FPGA_VERSION) || defined(VENUS_V5)
	IRQ_PMD_PD = 0,
	IRQ_TICK,
	IRQ_WPMD_SIG,
	IRQ_PMA_HDR,
	IRQ_WPMA_HDR,
#else
	IRQ_PMD_PHY = 0,
	IRQ_TICK,
	IRQ_PMD_ERR,
	IRQ_PMA_HDR,
	IRQ_PMA_ERR,
#endif
	IRQ_MC = 5,
	IRQ_SPI,
	IRQ_GPIO_EXT,
	IRQ_UART0,
	IRQ_SDMA_CH0,
	IRQ_SDMA_CH1,
	IRQ_SDMA_CH2,
	IRQ_SDMA_CH3,
	IRQ_SDMA_CH4,
	IRQ_SDMA_CH5,
	IRQ_SDMA_CH6,
	IRQ_SDMA_CH7 = 16,
#if defined(LIGHTELF2M) || defined(LIGHTELF8M)
	IRQ_I2C = 17,
#elif defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(UNICORN2M) || defined(UNICORN8M) || defined(VENUS2M) || defined(VENUS8M) || defined(FPGA_VERSION)
	IRQ_SPI_SLAVE = 17,
#endif
	IRQ_GE = 18,
	IRQ_MDIO_MGDI,
	IRQ_UART1,
	IRQ_UART2,
	IRQ_ACU,
	IRQ_PMA_PMI,
	IRQ_GE_GEI,
	IRQ_HTMR = 25,
#if defined(FPGA_VERSION) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
	IRQ_I2C = 26,
	IRQ_SPI1,
	IRQ_SPI2,
#if !(defined(VENUS_V2) || defined(VENUS_V3) || defined(VENUS_V5) || defined(FPGA_VERSION))
	IRQ_UART3 = 29,
	IRQ_UART4,
#else
	IRQ_SPI3 = 29,
	IRQ_UART3,
	IRQ_UART4,
#if defined(VENUS_V3) || defined(VENUS_V5) || defined(FPGA_VERSION)
	IRQ_SPI4 = 32,
	IRQ_SPI5,
	IRQ_UART5,
	IRQ_UART6,
	IRQ_UART7,
	IRQ_SPI_SLAVE1,
#endif
#endif
#endif
	NR_M_IRQ_MAX
};


extern void irq_init(void);
/* request_irq - Dispatch a MODULE irq line onto a CPU irq line. Both the MODULE line and the CPU irq line are enbled.
 * @c_irq_idx: CPU irq line
 * @m_irq_idx: MODULE irq line
 * @handler: user provided irq handler
 * @data: the first (static) argument for `@handler'
 */
extern void request_irq(int32_t c_irq_idx, int32_t m_irq_idx, irq_handler_fp handler, void *data);
/* release_irq - Release a MODULE irq line from a CPU irq line. Both the MODULE irq line and the CPU irq line (if none
 *               of MODULE irq lines are attached) will be disabled.
 * @m_irq_idx: MODULE irq line
 */
extern void release_irq(int32_t m_irq_idx);
/* enable_irq - Enable certain MODULE irq line together with the attached CPU irq line.
 * @m_irq_idx: MODULE irq line
 * @context: the second (dynamic) argument for `@handler'.
 */
extern void enable_irq(int32_t m_irq_idx, void *context);
/* disable_irq - Disable certain MODULE irq line, and disable the CPU line if all the attached MODULE irq lines have
 *               been disabled.
 * @m_irq_idx: MODULE irq line
 */
extern void disable_irq(int32_t m_irq_idx);

typedef struct mirq_info_s {
	uint32_t time;

	struct {
		uint32_t idx;
		uint32_t in;
		uint32_t out;
		uint32_t cost;
	} m[NR_M_IRQ_MAX];
} mirq_info_t;

extern __SLOWTEXT void get_irq_count(mirq_info_t *info);


#endif	/* end of _IRQ_H */
