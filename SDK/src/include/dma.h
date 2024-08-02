/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	dma.h
 * Purpose:	DMA utilities interface
 * History:
 *	Oct 09, 2014	ypzhou	Create
 *      Feb 06, 2015    ypzhou  Modify interface get_dma that can specify the used irq
 */

#ifndef __DMA_H__
#define __DMA_H__

#include "types.h"
#include "trios.h"

#define DMA_IRQ			3

#define DMA_STATUS_DOING	0
#define DMA_STATUS_DONE		1
#define DMA_STATUS_ERR		2
#define DMA_STATUS_STOP		3
#define DMA_STATUS_IDLE		4
#define DMA_STATUS_NONE		255

#define DMA_ADDR_TYPE_INCREMENT		0
#define DMA_ADDR_TYPE_CONSTANT		1
#define DMA_ADDR_TYPE_GSCATTER		2
#define DMA_ADDR_TYPE_CIRCULAR		3

enum {
	FC_TYPE_INVALID = 0,
	FC_TYPE_PMI_TX,
	FC_TYPE_PMI_RX,
	FC_TYPE_CRYPT_TX,
	FC_TYPE_CRYPT_RX,
	FC_TYPE_CRC_RX,
	FC_TYPE_GEI_RX,
	FC_TYPE_GEI_TX,
	FC_TYPE_SSPI_TX,
	FC_TYPE_SSPI_RX,
	FC_TYPE_WPMA_TX,
	FC_TYPE_WPMA_RX,
	FC_TYPE_WPMD_TX,
	FC_TYPE_FFT_TX,
	FC_TYPE_FFT_RX,
    FC_TYPE_I2C_RX,
    FC_TYPE_I2C_TX,
	FC_TYPE_NR,		
};

#define DMA_BLKSIZE_0			(0)
#define DMA_BLKSIZE_16			(4)
#define DMA_BLKSIZE_32			(5)
#define DMA_BLKSIZE_64			(6)

#define DMA_NULL	0
#define DMA_CALLBACK	(1 << 1)

#define DMA_TSIZE_WORD		0
#define DMA_TSIZE_HALF		1
#define DMA_TSIZE_BYTE		2

typedef struct {
	void *addr;

	uint8_t  type;
	uint8_t  fc_type;
	uint16_t rsv;
} dma_addr_t;

typedef struct dma_s {
	uint8_t	  idx;
	uint8_t	  status;
	uint16_t  len;
	event_t	  cond;
	dma_addr_t da;
	dma_addr_t sa;

	uint8_t   chain_en;
	uint8_t   chain_idx;
	uint8_t   tsize;

	uint32_t  src_err;
	uint32_t  dst_err;
	uint32_t  timeout;
	uint32_t  total;
	__isr__ void (*fin)(struct dma_s *);
	void      *data;
	const char *name;
} dma_t;

#define NR_DMA_MAX	8
extern dma_t * dma_get_dma(uint8_t idx);
extern void dma_init(void);
/* dma_get_irq -  Get a DMA channel for data transportation within SOC.
 * @name: DMA name
 * @irq: the request IRQ line for DMA channel
 * @return: NULL if no more DMA channel vacancy
 *          handler of a valid DMA channel
 */
extern dma_t * dma_get_irq(const char *name, uint32_t irq);
#define dma_get(name) dma_get_irq(name, DMA_IRQ)
/* dma_put - Reclaim a DMA channel.
 * @dma: the DMA channel
 */
extern void dma_put(dma_t *dma);
/* dma_start - Try to initiate a DMA transportation from `@sa' to `@da' and enable it. 
 *	       The DMA operation result can be fetched through field `result' of `dma_t' 
 *             in callback function `@fin'.
 * @dma: the DMA channel
 * @da: destination address
 * @sa: source address
 * @len: DMA transportation length
 * @fin: callback function when DMA operation done
 * @data: augmented data for `@fin' (via `dma->data')
 * @return: ERROR if fail to initiate
 *          OK if succeed
 */
extern int32_t dma_start(dma_t *dma, dma_addr_t *da, dma_addr_t *sa, int32_t len, 
			 __isr__ void (*fin)(dma_t *), void *data);
/* dma_stop - Stop an already initiated DMA operation.
 * @dma: the DMA channel
 * @opt: DMA_CALLBACK/DMA_NULL
 */
extern void dma_stop(dma_t *dma, uint32_t opt);
/* dma_wait - Try to wait for an on-going DMA operation to complete in a CPU-relinquishing way. Note
 *            that this API can only be used within a task context.
 * @dma: the DMA channel
 * @timeout: 0 to wait forever, non-zero to wait for `@timeout' ms
 * @return: ERROR if DMA operation fail
 *          non-zero postive to indicate DMA transportation length
 */
extern int32_t dma_wait(dma_t *dma, uint32_t timeout);
/* dma_busy_wait - Try to wait for an on-going DMA operation to complete in a busy-waiting way.1
 * @dma: the DMA channel
 * @timeout: 0 to wait forever, non-zero to wait for `@timeout' ms
 * @return: ERROR if DMA operation fail
 *          non-zero postive to indicate DMA transportation length
 */
extern int32_t dma_busy_wait(dma_t *dma, uint32_t timeout);
/* dma_watch - Try to initiate a DMA transportaton from `@sa' to `@da' and wait for its completion in 
 *            a CPU-relinquishing way. Note that this API can only be used within a task context.
 * @dma: the DMA channel
 * @da: destination address
 * @sa: source address
 * @len: DMA transportation length
 * @return: ERROR if fail
 *          OK if timeout
 *          non-zero positive for DMA transportation length
 */
extern int32_t dma_watch(dma_t *dma, dma_addr_t *da, dma_addr_t *sa, int32_t len, uint32_t timeout);
/* fill_dma_addr - Initialize an instance of `dma_addr_t'.
 * @dma_addr:
 * @addr:
 * @type: DMA_ADDR_TYPE_INCREMENT or DMA_ADDR_TYPE_CONSTANT
 * @fc_type: type of flow control
 */
static __inline__ void fill_dma_addr(dma_addr_t *dma_addr, void *addr, int32_t type, uint32_t fc_type)
{
	dma_addr->addr    = addr;
	dma_addr->type    = type;
	dma_addr->fc_type = fc_type;
}

/* For internal usage ONLY.
 */
extern void dma_set_blksize(dma_t *dma, uint32_t _blksize);
extern void dma_set_timeout(dma_t *dma, uint32_t timeout);
extern void dma_set_priority(dma_t *dma, uint32_t pri);
extern uint32_t dma_get_priority(dma_t *dma);
extern uint32_t dma_is_busy(dma_t *dma);
extern int32_t __dma_start(dma_t *dma, dma_addr_t *da, dma_addr_t *sa, int32_t len, 
			      __isr__ void (*fin)(dma_t *), void *data);
extern void dma_exec(dma_t *dma);
extern void dma_restart(dma_t *dma);
extern void dma_set_sa(dma_t *dma, dma_addr_t *sa);
extern void dma_set_da(dma_t *dma, dma_addr_t *da);
extern void dma_set_soa(dma_t *dma, uint32_t soa);
extern void dma_set_doa(dma_t *dma, uint32_t doa);
extern void dma_set_len(dma_t *dma, int32_t len);
extern void dma_set_sa_reload(dma_t *dma, uint8_t en);
extern void dma_set_da_reload(dma_t *dma, uint8_t en);
extern void dma_set_len_reload(dma_t *dma, uint8_t en);

extern uint8_t addr_mapping[];
#define dma_addr_mapping(addr) ((uint32_t)(((addr) & 0x0fffffff) | (addr_mapping[(addr) >> 28]) << 28))


#endif	/* end of __DMA_H__ */

