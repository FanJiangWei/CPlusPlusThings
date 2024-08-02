/*
 * Copyright: (c) 2006-2020, 2020 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        tick.h
 * Purpose:     system tick functions.
 * History:     
 */

#ifndef __TICK_DRV_H__
#define __TICK_DRV_H__

#include "types.h"
#include "sys.h"
#include "irq.h"

#define PHY_TICK_FREQ		25000000	/* 25MHZ */
#define PHY_TICKS_PER_SEC	PHY_TICK_FREQ
#define PHY_TICKS_PER_US	(PHY_TICKS_PER_SEC / 1000 / 1000)	/* 25 */
#define PHY_TICK2US(tick)	((tick) / PHY_TICKS_PER_US)
#define PHY_TICK2MS(tick)	(PHY_TICK2US(tick) / 1000)
#define PHY_US2TICK(us)		((us)   * PHY_TICKS_PER_US)
#define PHY_MS2TICK(ms)		PHY_US2TICK((ms) * 1000)

extern uint32_t phy_hdr_time;
static __inline__ uint32_t get_phy_hdr_time()
{
	return phy_hdr_time;
}


static __inline__ void set_phy_hdr_time(uint32_t time)
{
	phy_hdr_time = time;
}


typedef struct phy_evt_s {
	uint16_t idx;
	uint16_t src;
	void (*op)(struct phy_evt_s *);
	void *ins;

	uint32_t time;
	uint32_t curr;
} phy_evt_t;

struct phy_tmr_s {
	uint32_t idx;
	void (*op)(struct phy_tmr_s *);
	void *ins;

	uint32_t time;
	uint32_t curr;
};

typedef struct phy_tmr_s phy_tmr_t;
#if defined(FPGA_VERSION) || defined(MIZAR1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
#define NR_PHY_EVT_MAX	16
#define NR_PHY_TMR_MAX	16

#elif defined(ROLAND1_1M) || defined(ROLAND9_1M)
#define NR_PHY_EVT_MAX	16
#define NR_PHY_TMR_MAX	12

#else
#define NR_PHY_EVT_MAX	12
#define NR_PHY_TMR_MAX	12
#endif

typedef struct phy_evt_po_s {
	pool_t po;
	phy_evt_t evt[NR_PHY_EVT_MAX];
} phy_evt_po_t;

typedef struct phy_tmr_po_s {
	pool_t po;
	phy_tmr_t tmr[NR_PHY_TMR_MAX];
} phy_tmr_po_t;

extern phy_tmr_po_t phy_tmr_po;
extern phy_evt_po_t phy_evt_po;


extern __LOCALTEXT phy_evt_t *get_phy_evt();
extern __LOCALTEXT void put_phy_evt(phy_evt_t *evt);
extern __LOCALTEXT phy_tmr_t *get_phy_tmr();
extern __LOCALTEXT void put_phy_tmr(phy_tmr_t *tmr);
extern __LOCALTEXT void start_phy_tmr(phy_tmr_t *tmr, uint32_t time, void (*op)(phy_tmr_t *), void *ins);
extern __LOCALTEXT void stop_phy_tmr(phy_tmr_t *tmr);
extern __LOCALTEXT void subscribe_phy_evt(phy_evt_t *evt, uint32_t src, void (*op)(phy_evt_t *), void *ins);
extern __LOCALTEXT void unsubscribe_phy_evt(phy_evt_t *evt);
extern __LOCALTEXT void set_phy_tmr_time(phy_tmr_t *tmr, uint32_t time);

extern __LOCALTEXT int32_t phy_evt_is_pending(phy_evt_t *evt);
extern __LOCALTEXT int32_t phy_tmr_is_pending(phy_tmr_t *tmr);
extern __LOCALTEXT int32_t wait_phy_evt(phy_evt_t *evt, uint32_t *time);
extern __LOCALTEXT int32_t wait_phy_tmr(phy_tmr_t *tmr, uint32_t *time);
extern __LOCALTEXT uint32_t get_phy_evt_time(phy_evt_t *evt);
extern __LOCALTEXT uint32_t get_phy_tick_time();
extern void tick_init(uint32_t c_irq_level);
#endif
