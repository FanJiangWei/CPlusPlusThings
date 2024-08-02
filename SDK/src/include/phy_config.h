/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        phy_config.h
 * Purpose:     PHY layer related config interface
 * History:     12/01/2014, created by ypzhou
 */

#ifndef __PHY_CONFIG_H__
#define __PHY_CONFIG_H__

#include "trios.h"
#include "phy.h"
#include <string.h>
#include "vsh.h"

typedef struct robo_s {
	uint16_t sec_size;

	uint16_t sec_num	:11;
	uint16_t padding	:1;
	uint16_t css_gap	:4;

	uint16_t sym_size;
	uint16_t last_sym_size;
#if defined(UNICORN2M) || defined(UNICORN8M) || defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M) || defined(FPGA_VERSION)
	uint16_t intl_ncopy	:4;
	uint16_t intl_nsc	:10;
	uint16_t		:2;
	uint16_t sc_pad;
	
	uint8_t map[NR_TONE_MAX];
	uint32_t scmask2[NR_TONE_MAX >> 5];
#endif
} robo_t;


#if defined(FPGA_VERSION)
#define PHY_NOTCH_BAND_MAX	(2)
#define NR_NOTCH_TONE_MAX	(32)
#define NR_TONE_EACH_BAND	(16)
#elif defined(ASIC_VERSION)
#define PHY_NOTCH_BAND_MAX	(8)
#define NR_NOTCH_TONE_MAX	(80)
#define NR_TONE_EACH_BAND	(32)
#endif


typedef struct profile_s {
	uint8_t phase_tbl	:1;	/* phase table: 0: STD_2012 1: STD_2016 */
	uint8_t hdr_sym_ofs	:1;	/* hdr symbol offset select, 0: STD_2012 1: STD_2016 */
	uint8_t intl_en		:1;	/* interleave en: 0: disable 1: enable */
	uint8_t intl_mode	:1;	/* interleave mode 0: BPLC 1: HPAV */
	uint8_t scrm_mode	:1;	/* 0: scramble before encode and after decode
					   1: scramble after encode and before decode */
	uint8_t scrm_hdr_en	:1;	/* 0: disable hdr scramble, 1: enable hdr scramble */
	uint8_t poly_mode	:1;	/* 0: mode0, 1: mode1 */
	uint8_t pb_format	:1;	/* the pb package format, pb520 for example,
					   0:  516 + 1(reserved) + 3(crc)
					   1:  517 + 3(crc) */
	uint8_t robo_mode	:1;	/* robo calculate way 0: STD_2012, 1: STD_2016 */
	uint8_t syncp		:5;
	uint8_t sc_pad_en	:1;
	uint8_t tm_ver		:1;	/* tonemap version: 0: STD_SPG, 1: STD_2016 */

	uint16_t log2_n;
	uint8_t  num_sec1;
	uint8_t  num_sec2;
	uint16_t num_ri;
	uint16_t eos_off        :10;
	uint16_t syncm		:3;
	uint16_t		:3;
	uint16_t pre_echo_offset_hdr;
	uint16_t pre_echo_offset_pld;

	uint16_t num_gi_hd;
	uint16_t num_gi_df;
	uint16_t num_gi;
	uint16_t num_sc;

	uint16_t scm_start;
	uint16_t scm_end;
	uint16_t nr_notch_band;
	phy_notch_band_t nband[PHY_NOTCH_BAND_MAX];
} profile_t;
extern profile_t CURR_P;

typedef struct {
	uint16_t tone		:12;
	uint16_t		:4;

	int8_t   gain;
	uint8_t                 :8;
} phy_sin_info_t;

typedef struct {
	int32_t  gain;

#define CAP_STOP_AT_HDR		(1)
#define CAP_STOP_AT_END		(2)
#define CAP_STOP_AT_HDR_ERR	(3)
	uint8_t stop		:2;	/* capture stop postion */
	uint8_t bw		:1;	/* bandwidth 0: 12bits, 1: 10bits */
	uint8_t lpbk		:1;	/* memory loopback or not */
	uint8_t			:4;

	void *saddr;
	uint32_t len;
} phy_signal_cap_t;

extern uint32_t phy_set_symb_dur(uint16_t *symb_dur);
extern int32_t phy_config_init(void);
extern void update_phy_profile(void);

extern void phy_profile_show(xsh_t *xsh);

extern uint32_t stop_debug_mode();
extern void start_debug_mode(void *saddr, uint32_t len, uint32_t lpbk);
extern int32_t phy_start_cap(phy_hdr_info_t *hi, phy_signal_cap_t *sc, evt_notifier_t ntf[], uint32_t nr_ntf);
__isr__ extern int32_t is_pulse_noise(void);

#endif	/* end of __PHY_CONFIG_H__ */

