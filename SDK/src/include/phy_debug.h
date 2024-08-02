/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        phy_debug.h
 * Purpose:     PHY layer related debug interface
 * History:     11/26/2014, created by ypzhou
 */

#ifndef __PHY_DEBUG_H__
#define __PHY_DEBUG_H__

#include "trios.h"
#include "phy.h"
#include "framectrl.h"
#include "phy_config.h"
#include "term.h"

typedef struct {
	uint32_t snr_esti	:1;
	uint32_t mm		:3;
	uint32_t fecr		:2;
	uint32_t copy		:4;
	uint32_t snr_level	:3;
	uint32_t mode		:4;	/* Test Mode */

#define PHY_RATE_TEST		(1)
#define PHY_CONSIST_TEST	(2)
#define PHY_RFPLC_TEST		(3)
#define PHY_RFPLC_SECURITY	(4)
#define PHY_PLC2RF_TEST		(5)
	uint32_t testcase	:6;	/* Test Case */
	uint32_t		:9;

	uint32_t mpdu;
	uint32_t __mpdu;
	uint32_t bp;

	uint32_t state		:4;
	uint32_t substate	:4;
	uint32_t phase		:8;
	uint32_t ld_phase	:1;
	uint32_t ack		:1;
	uint32_t dump		:1;	/* dump all frame data */
	uint32_t 		:13;

	uint32_t dur;
	uint32_t delay;

	uint32_t snid;
	uint16_t tei;

	uint32_t sfo_not_first;
	uint32_t sfo_bts;

	phy_frame_info_t fi;

	double snr_ave;
	void *dump_addr;
	int32_t rx_gain;

	uint32_t time;
	uint32_t rx_timeout;
	uint32_t success;
	uint32_t unmatch;
	uint32_t lost;
	uint32_t error;
	uint32_t ival_ifs;
	double   rate;

	phy_signal_cap_t sc;
	uint32_t dbg_end_addr;
	
#if defined(STD_DUAL)
	uint32_t addr_s;
	uint32_t addr_e;
	uint32_t addr_m;
#define WPHY_CAP_IDLE		0 
#define WPHY_CAP_SIG_ERR	1
#define WPHY_CAP_HDR_ERR	2
#define WPHY_CAP_PLD_ERR	3
#define WPHY_CAP_SIG		4
#define WPHY_CAP_HDR		5
#define WPHY_CAP_PLD		6
#define WPHY_CAP_NO_SIG		7
	uint32_t cap		:8;
	uint32_t cap_len	:24;

	uint32_t cap_type;

	uint32_t loop_blkz;
	uint32_t loop_phr_mcs;
	uint32_t loop_pld_mcs;
	uint32_t loop_count;

	uint32_t interval;

	ostimer_t tmr;
#else
	uint32_t cap		:8;
	uint32_t		:24;
#endif

	uint8_t hdr[32];
	uint8_t pld[524*4] __CACHEALIGNED;

#if (!STATIC_MASTER && !STATIC_NODE)
	struct {
		uint32_t snid;
		uint16_t tei;

		uint32_t mpdu;
		uint32_t __mpdu;
		phy_frame_info_t fi;

		uint8_t hdr[32];
		uint8_t pld[524] __CACHEALIGNED;
	} bcn;

	struct {
		uint32_t wait_ssn_bmp;
		uint32_t recv_ssn_bmp;

		uint32_t etime;

		uint8_t phr_mcs;
		uint8_t pld_mcs;
		uint8_t pbsz;
	} p2r;
#endif
} phy_testinfo_t;

#define CAP_ALIGN_SIZE	(128 * 12)
#if defined(UNICORN8M) || defined(ROLAND9_1M) || defined(VENUS8M)
#define CAP_SIZE	(1024 * 1024 + CAP_ALIGN_SIZE)
#elif defined(UNICORN2M) || defined(ROLAND1_1M) || defined(VENUS2M)
#if defined(VENUS_V5) && defined(ARMCM33)
#define CAP_SIZE	(128 * 1024 + CAP_ALIGN_SIZE)
#else
#define CAP_SIZE	(256 * 1024 + CAP_ALIGN_SIZE)
#endif
#else
#define CAP_SIZE	(0)
#endif
extern uint32_t cap_mem[];

#define phy_debug_printf(fmt, arg...) debug_printf(&dty, DEBUG_PHY, fmt, ##arg)
#define wphy_debug_printf(fmt, arg...) debug_printf(&dty, DEBUG_WPHY, fmt, ##arg)

typedef struct {
	uint16_t event          :8;
	uint16_t		:8;

	union {
		struct {
			uint16_t hdr_symn       :4;
			uint16_t hdr_mm         :2;
			uint16_t hdr_size       :1;
			uint16_t valid          :3;
			uint16_t                :6;
		};

		uint16_t hdr;	
	};

	union {
		struct {
			uint32_t pbc            :6;
			uint32_t pbsz		:2;
			uint32_t fec_rate       :3;
			uint32_t copy           :4;
			uint32_t mm             :4;
			uint32_t symn           :12;
			uint32_t		:1;
		};

		uint32_t pld;
	};

 	uint32_t time;
	uint32_t curr;
	uint32_t stime;
	uint32_t etime;
} phy_trace_t;

#define NR_PHY_TRACE_MAX	32

enum {
	PHY_TRACE_SCHE_TX = 1,
	PHY_TRACE_IMMD_TX,
	PHY_TRACE_SCHE_RX,
	PHY_TRACE_IMMD_RX,
	PHY_TRACE_TX_END,
	PHY_TRACE_TX_FAIL,
	PHY_TRACE_RX_HDR,
	PHY_TRACE_RX_PLD_START,
	PHY_TRACE_RX_END,
	PHY_TRACE_RX_END_TMOT,
	PHY_TRACE_RX_PLD_END,
	PHY_TRACE_RX_ACE_END,
	PHY_TRACE_RX_HDR_FAIL,
	PHY_TRACE_RESET,
	PHY_TRACE_RESET_BEFORE,
	PHY_TRACE_RX_TMOT,
	PHY_TRACE_PD_END,
	PHY_TRACE_HOLD,
	PHY_TRACE_TRY_TX,
	PHY_TRACE_TRY_RX,
	PHY_TRACE_TRY_RX_AT,
	PHY_TRACE_DO_NEXT,
	PHY_TRACE_SKIP,
	PHY_TRACE_NBNI_BEG,
	PHY_TRACE_NBNI_OK,
	PHY_TRACE_NBNI_FAIL,
	PHY_TRACE_PD_FAIL,
	PHY_TRACE_TX_RSP,
	PHY_TRACE_RX_RSP,
	PHY_TRACE_CSMA_BACKOFF,
	PHY_TRACE_TRY_CTD,
	PHY_TRACE_SCHE_CTD,
	PHY_TRACE_ABDN_CTD,
	PHY_TRACE_SCHE_CTD_ERR,
	PHY_TRACE_TRY_RX_ACK,
	PHY_TRACE_SKIP_TX,
};

typedef struct wphy_trace_s {
	uint32_t event		:8;
	uint32_t valid		:1;
	uint32_t phr_mcs	:3;
	uint32_t pld_mcs	:3;
	uint32_t blkz		:3;
	uint32_t pbc		:3;
	uint32_t		:11;
	uint32_t time;
	uint32_t curr;
	uint32_t stime;
	uint32_t etime;
} wphy_trace_t;

#define NR_WPHY_TRACE_MAX	64

enum {
	WPHY_TRACE_SCHE_TX = 1,
	WPHY_TRACE_IMMD_TX,
	WPHY_TRACE_SCHE_RX,
	WPHY_TRACE_IMMD_RX,
	WPHY_TRACE_TX_END,
	WPHY_TRACE_TX_FAIL,
	WPHY_TRACE_RX_HDR,
	WPHY_TRACE_RX_PLD_BEGIN,
	WPHY_TRACE_RX_END,
	WPHY_TRACE_RX_END_TMOT,
	WPHY_TRACE_RX_PLD_END,
	WPHY_TRACE_RX_HDR_FAIL,
	WPHY_TRACE_RX_HDR_ERR,
	WPHY_TRACE_RESET,
	WPHY_TRACE_RX_TMOT,
	WPHY_TRACE_PD_FAIL,
	WPHY_TRACE_PD_END,
	WPHY_TRACE_TRY_TX,
	WPHY_TRACE_TRY_RX,
	WPHY_TRACE_TRY_RX_AT,
	WPHY_TRACE_DO_NEXT,
	WPHY_TRACE_SKIP,
};

extern void phy_trace_show(tty_t *term);
extern void phy_trace_evt(uint32_t event, phy_hdr_info_t *hi, phy_pld_info_t *pi, 
			  uint32_t time, uint32_t curr, uint32_t stime, uint32_t etime);
extern void phy_debug_init(void);

extern void phy_statis_show(xsh_t *xsh);
extern void phy_statis_clear(xsh_t *xsh);

extern void wphy_debug_init(void);
extern void wphy_trace_evt(uint32_t event, wphy_hdr_info_t *hi, wphy_pld_info_t *pi, 
			   uint32_t time, uint32_t curr, uint32_t stime, uint32_t etime);
extern void wphy_trace_show(tty_t *term);

extern void wphy_show_statis(tty_t * term);
extern void wphy_clear_statis(void);

#endif	/* end of __PHY_DEBUG_H__ */
