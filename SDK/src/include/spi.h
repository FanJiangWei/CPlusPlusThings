/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	spi.h
 * Purpose:	spi master and slave manipulation
 * History:
 *	Dec 6, 2016	tgni	    Create
 */

#ifndef __SPI_H__
#define __SPI_H__

#include "types.h"
#include "gpio.h"
#include "dma.h"

#define SPI_MST0_IDX	0
#if !(defined(LIGHTELF2M) || defined(LIGHTELF8M) || defined(UNICORN2M) || defined(UNICORN8M))
#define SPI_MST1_IDX	1
#define SPI_MST2_IDX	2
#endif
#if !(defined(LIGHTELF2M) || defined(LIGHTELF8M))
#define SPI_SLV_IDX	3
#endif

#define CSS_AUTO	(0)
#define CSS_MANUAL	(1)

#define BIT_CSS0	0
#define BIT_CSS1	1
#define BIT_CSS2	2
#define BIT_CSS3	3

#define	MASK_CSS0	(1 << BIT_CSS0)
#define	MASK_CSS1	(1 << BIT_CSS1)
#define MASK_CSS2	(1 << BIT_CSS2)
#define MASK_CSS3	(1 << BIT_CSS3)

#define SPI_MODE_NORMAL	0
#define SPI_MODE_DUAL	1
#define SPI_MODE_QUAL	2

#define SPI_TRSF_MODE_TR	0
#define SPI_TRSF_MODE_TX	1
#define SPI_TRSF_MODE_RX	2

#define SPI_DIR_OUTPUT		0
#define SPI_DIR_INPUT		1

struct spi_dev_s;
typedef struct spi_trx_s {
	uint32_t 	dbytes		:3;
	uint32_t 	use_dma		:1;	/* use dma to do current transmission */
	uint32_t 	intr_en		:1;	/* current transmission use interrupt mode */
	uint32_t	circular	:1;	/* 1: current transmission is contionuous mode and the field
						 * of len is used as notify bytes. */
	uint32_t	cs_stop		:1;	/* whether inactive cs after last operation */
	uint32_t	mode		:2;	/* 0: normal spi, 1: dual spi, 2: qual spi */
	uint32_t	cb_err		:1;
	uint32_t			:22;


	int32_t		len;			/* bytes tobe transmission,
						 * in dma circular mode this field need to set to half of sz_buf */
	int32_t 	sz_buf;			/* size of buffer, only used in circular mode */
	void		*addr;
	void		*alloc_addr;		/* addr = (alloc_addr + CACHE_LINE_SIZE) & ~(CACHE_LINE_SIZE - 1) @ dma mode */

	dma_addr_t	sa;
	dma_addr_t	da;

	uint32_t	rp;			/* offset of field 'addr' @ spi_write */
	uint32_t	wp;			/* offset of field 'addr' @ spi_read */

	int32_t		(*cb)(struct spi_dev_s *dev);	/* valid @ dma mode and inter mode */
	void		*cb_arg;
} spi_trx_t;

typedef struct spi_dev_s {
	gpio_dev_t	gpio;
	char		*name;

	uint8_t		css;
	uint8_t		idx;
	uint8_t		spi_slv;		/* 1: spi slave 0: spi master */

	uint32_t	sck;			/* unit: hz */

	uint32_t	io_inited	:1;
	uint32_t	irq_inited	:1;
	uint32_t	cpol		:1;
	uint32_t	cpha		:1;
	uint32_t	irq_level	:1;
	uint32_t	fifo_depth	:8;
	uint32_t	soft_cs_en	:1;
	uint32_t			:18;

	gpio_desc_t	css_io;			/* valid @ soft_cs_en is 1 */

	spi_trx_t	trx;
	dma_t		*dma;
} spi_dev_t;

/* spi_busy_wait - wait spi not busy
 */
extern void spi_busy_wait(spi_dev_t *dev);

/* spi_set_data - set spi data
 * @bnum: number of bytes
 * @data: opcode or data
 */
extern void spi_set_data(spi_dev_t *dev, uint8_t bnum, uint32_t data);

/* spi_set_data - set spi data easy, no busy wait
 * @bnum: number of bytes
 * @data: opcode or data
 */
extern void spi_set_data_easy(spi_dev_t *dev, uint8_t bnum, uint32_t data);

/* spi_get_data - get spi data with number
 * @bnum: number of bytes
 * @return: data
 */
extern uint32_t spi_get_data(spi_dev_t *dev, uint8_t bnum);

/* spi_get_data_easy - get spi data
 * @return: data
 */
extern uint32_t spi_get_data_easy(spi_dev_t *dev);

/* spi_set_wdata_msb - spi transmit msb first or not, this will affect read and write both.
 * @msb:
 */
extern void spi_set_wdata_msb(spi_dev_t *dev, uint8_t msb);

/* spi_set_qual_write - enable qual page program write
 */
extern void spi_set_qual_write(spi_dev_t *dev);

/* spi_set_mode - set spi transfer mode
 * @mode: SPI_MODE_NORMAL
 *        SPI_MODE_DUAL
 *        SPI_MODE_QUAL
 */
extern void spi_set_mode(spi_dev_t *dev, uint8_t mode);

/* spi_set_ahb_mode - set spi ahb read mode
 * @mode: SPI_MODE_NORMAL
 *        SPI_MODE_DUAL
 *        SPI_MODE_QUAL
 */
extern void spi_set_ahb_mode(spi_dev_t *dev, uint8_t mode, uint8_t cmd);

/* spi_set_op -
 */
extern void spi_set_op(spi_dev_t *dev, uint32_t op);

/* spi_set_pad_mode -
 */
extern void spi_set_pad_mode(spi_dev_t *dev, uint32_t mode);

/* spi_set_css_mode - control work mode of #cs signal of flash.
 * @mode: CSS_AUTO:   auto control #cs signal's level by setting css auto stop.
 *	  CSS_MANUAL: manual control spi #cs signal.
 * @mask: spi css line mask
 */
extern void spi_set_css_mode(spi_dev_t *dev, uint8_t mode, uint8_t mask);

/* spi_set_css_auto_stop - control work
 * @en: ENABLE:  #cs goes low, when data finished, #cs auto goes high.
 *      DISABLE: #cs goes low
 * note:
 *	css must be at CSS_AUTO mode.
 */
extern void spi_set_css_auto_stop(spi_dev_t *dev, uint8_t en);

/* spi_get_css - get spi #cs
 * @return: spi #cs status
 */
extern uint8_t spi_get_css(spi_dev_t *dev);

/* spi_set_css - set spi #cs
 * @en: 0|1: low or high.
 */
extern void spi_set_css(spi_dev_t *dev, uint8_t en);

/* spi_set_cp - set spi CPHA and CPOL
 * @cpha:
 * @cpol:
 */
extern void spi_set_cp(spi_dev_t *dev, uint8_t cpha, uint8_t cpol);

/* spi_set_sck - set spi SCK
 * @sck:
 */
extern void spi_set_sck(spi_dev_t *dev, uint32_t sck);


extern void spi_enable(spi_dev_t *dev);
extern void spi_disable(spi_dev_t *dev);
extern uint32_t spi_is_enable(spi_dev_t *dev);

/* spi_set_width - spi transfer word width
 */
extern int32_t spi_set_width(spi_dev_t *dev, uint8_t dbytes);

/* spi_set_trsf_mode -
 */
extern int32_t spi_set_trsf_mode(spi_dev_t *dev, uint8_t mode);

/* spi_set_cs_active - active cs, master only
 */
extern void spi_set_cs_active(spi_dev_t *dev);

/* spi_set_cs_inactive - inactive cs, master only
 */
extern void spi_set_cs_inactive(spi_dev_t *dev);

/* spi_clr_rx_fifo -
 */
extern void spi_clr_rx_fifo(spi_dev_t *dev);

/* spi_get_rx_fifo_cont -
 */
extern uint32_t spi_get_rx_fifo_cont(spi_dev_t *dev);

/* spi_get_tx_fifo_cont -
 */

extern uint32_t spi_get_tx_fifo_cont(spi_dev_t *dev);
/* spi_init -
 */
extern int32_t spi_init(spi_dev_t *dev);

/* spi_read - spi master receive data from slave
 */
extern int32_t spi_read(spi_dev_t *dev, spi_trx_t *trx);

/* spi_write - spi master transmit trx->addr's data to slave
 */
extern int32_t spi_write(spi_dev_t *dev, spi_trx_t *trx);

/* spi_write_read - spi master transmit trx->addr's data to slave and
 * data received from slave are also stored into trx->addr.
 */
extern int32_t spi_write_read(spi_dev_t *dev, spi_trx_t *trx);

/* spi_clk_cs_high - spi master output clock but set cs inactive
 */
extern int32_t spi_clk_cs_high(spi_dev_t *dev);

extern int32_t spi_slave_read(spi_dev_t *dev, spi_trx_t *trx);
extern int32_t spi_slave_write(spi_dev_t *dev, spi_trx_t *trx);
extern int32_t spi_slv_stop(spi_dev_t *dev);
#if defined(VENUS_V2) || defined(VENUS_V3) || defined(VENUS_V4) || defined(VENUS_V5) || defined(FPGA_VERSION)	/* r5344 */
extern void spi_set_adc_wait_gap(uint8_t spi_idx, uint32_t gap);
extern void spi_set_adc_trsf_smp(uint8_t spi_idx, uint32_t smp);
extern void spi_set_adc_auto_en(uint8_t spi_idx, uint8_t enable);
#if defined(VENUS_V2) || defined(VENUS_V3) || defined(VENUS_V4)
extern __LOCALTEXT void spi_set_quad(uint8_t idx, uint8_t rsv);
#elif defined(VENUS_V5) || defined(FPGA_VERSION)
extern void spi_set_dual(uint8_t idx);
extern void spi_set_quad(uint8_t idx, uint8_t xip_en);
#endif
#endif
#endif	/* end of __SPI_H__ */
