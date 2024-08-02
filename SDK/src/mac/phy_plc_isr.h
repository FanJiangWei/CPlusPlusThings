/*
 * Copyright: (c) 2019, 2019 ZC Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        phy_zc.c
 * Purpose:     demo procedure for using 
 * History:
 */
 
#ifndef __PHY_PLC_ISR_H__
#define __PHY_PLC_ISR_H__

#include "trios.h"
#include "sys.h"
#include "framectrl.h"
#include "phy_config.h"
//#include "phy_sal.h"
#include "vsh.h"
#include "mem_zc.h"
#include "plc.h"

sfo_handle_t HSFO;


typedef struct {
	uint32_t print		:8;
	uint32_t snr_esti	:1;
	uint32_t mm		:3;
	uint32_t fecr		:2;
	uint32_t copy		:4;
	uint32_t snr_level	:3;
	uint32_t phase		:4;
	uint32_t		:7;
	uint32_t mpdu;
	uint32_t bp;

	double snr_ave;

	uint32_t state		:4;
	uint32_t substate	:4;
	uint32_t len		:24;

	void *payload;
	uint32_t dur;
	uint32_t delay;
	frame_ctrl_t head;
	frame_ctrl_t head_b;
	phy_pld_info_t pi;

	frame_ctrl_t beacon;
	uint32_t sfo_not_first;
	uint32_t sfo_bts;

	phy_frame_info_t pf;
	phy_hdr_info_t rx;
	phy_signal_cap_t sc;

	uint32_t mem[(NR_TONE_MAX >> 3) + 1];
	uint32_t rx_gain;

	uint16_t stei;
	uint16_t snid;
	uint32_t bpc;

	uint8_t hdr[32];
	uint8_t pld[524*4] __CACHEALIGNED;
} phy_nib_t;










__LOCALTEXT __isr__ uint8_t noise_proc_fn(bpts_t *bpts);
// __LOCALTEXT __isr__ void phy_notch_freq_set(phy_evt_param_t *para);

void phy_plc_init(void);






















#endif 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
