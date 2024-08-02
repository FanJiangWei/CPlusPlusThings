/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        phy_sal.c
 * Purpose:     PHY layer related function for China Grid Standard Q/GDW 1376.3
 * History:     11/03/2014, created by ypzhou
 */

#include <string.h>
#include "phy_sal.h"
#include "plc.h"
#include "phy_debug.h"

const char *tonemap_name[] = {
	"ROBO_520_0",
	"ROBO_520_1",
	"ROBO_136_2",
	"ROBO_136_3",
	"ROBO_136_4",
	"ROBO_136_5",
	"ROBO_136_6",
	"ROBO_520_7",
	"ROBO_520_8",
	"ROBO_520_9",
	"ROBO_520_10",
	"ROBO_264_11",
	"ROBO_264_12",
	"ROBO_72_13",
	"ROBO_72_14",
	"NULL",
	"NULL",
	"EXT_520_1",
	"EXT_520_2",
	"EXT_520_3",
	"EXT_520_4",
	"EXT_520_5",
	"EXT_520_6",
	"NULL",
	"NULL",
	"NULL",
	"EXT_136_10",
	"EXT_136_11",
	"EXT_136_12",
	"EXT_136_13",
	"EXT_136_14",
};
tonemap_t m_tonemaps[NR_TONEMAP_MAX];

void config_sys_tonemap(tonemap_t *tm)
{
	if (!tm->valid)
		return;

	tm->fec_sz = pb_fecsize(tm->pbsz, tm->fec_rate) * tm->copy;

	if (!(tm->robo = calc_robo(tm->tmi, tm->pbsz, tm->fec_rate, tm->copy, tm->mm))) {
		debug_printf(&dty, DEBUG_PHY, "%s error: tmi %d pbsz %d fec_rate %d copy %d mm %d\n",
			     __func__, tm->tmi, tm->pbsz, tm->fec_rate, tm->copy, tm->mm);
		return;
	}

	return;
}


__LOCALTEXT tonemap_t *get_sys_tonemap(uint32_t tmi)
{
	if (tmi >= NR_TONEMAP_MAX)
		return NULL;

	if (!m_tonemaps[tmi].valid)
		return NULL;

	return &m_tonemaps[tmi];
}

void update_tonemap(void)
{
	tonemap_t *tm;
	int32_t i;

	tm = &m_tonemaps[0];

	for (i = 0; i < NR_TONEMAP_MAX; ++i)
		config_sys_tonemap(&tm[i]);

	return;
}

__SLOWTEXT void tonemap_init(void)
{
	tonemap_t *tm;
	int32_t i;

	memset(m_tonemaps, 0, sizeof(m_tonemaps));
	tm = &m_tonemaps[0];

#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
	fill_sys_tonemap(tm[TMI_ROBO_520_0],  TMI_ROBO_520_0,  PBSZ_520, 1, 1, FEC_RATE_1_2, 4,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_1],  TMI_ROBO_520_1,  PBSZ_520, 1, 4, FEC_RATE_1_2, 2,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_2],  TMI_ROBO_136_2,  PBSZ_136, 1, 1, FEC_RATE_1_2, 5,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_3],  TMI_ROBO_136_3,  PBSZ_136, 1, 4, FEC_RATE_1_2, 11, MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_4],  TMI_ROBO_136_4,  PBSZ_136, 1, 4, FEC_RATE_1_2, 7,  MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_5],  TMI_ROBO_136_5,  PBSZ_136, 1, 4, FEC_RATE_1_2, 11, MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_6],  TMI_ROBO_136_6,  PBSZ_136, 1, 4, FEC_RATE_1_2, 7,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_7],  TMI_ROBO_520_7,  PBSZ_520, 1, 1, FEC_RATE_1_2, 7,  MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_8],  TMI_ROBO_520_8,  PBSZ_520, 1, 1, FEC_RATE_1_2, 4,  MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_9],  TMI_ROBO_520_9,  PBSZ_520, 1, 1, FEC_RATE_1_2, 7,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_10], TMI_ROBO_520_10, PBSZ_520, 1, 4, FEC_RATE_1_2, 2,  MM_BPSK);

	fill_sys_tonemap(tm[TMI_EXT_520_1],   TMI_EXT_520_1,  PBSZ_520, 1, 4, FEC_RATE_16_18, 1, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_2],   TMI_EXT_520_2,  PBSZ_520, 1, 4, FEC_RATE_16_18, 2, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_3],   TMI_EXT_520_3,  PBSZ_520, 1, 4, FEC_RATE_1_2,   1, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_4],   TMI_EXT_520_4,  PBSZ_520, 1, 4, FEC_RATE_1_2,   2, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_5],   TMI_EXT_520_5,  PBSZ_520, 1, 4, FEC_RATE_1_2,   4, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_6],   TMI_EXT_520_6,  PBSZ_520, 1, 4, FEC_RATE_1_2,   1, MM_QPSK);
	fill_sys_tonemap(tm[TMI_EXT_136_10],  TMI_EXT_136_10, PBSZ_136, 1, 1, FEC_RATE_1_2,   5, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_136_11],  TMI_EXT_136_11, PBSZ_136, 1, 1, FEC_RATE_1_2,   2, MM_QPSK);
	fill_sys_tonemap(tm[TMI_EXT_136_12],  TMI_EXT_136_12, PBSZ_136, 1, 1, FEC_RATE_1_2,   2, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_136_13],  TMI_EXT_136_13, PBSZ_136, 1, 1, FEC_RATE_1_2,   1, MM_QPSK);
	fill_sys_tonemap(tm[TMI_EXT_136_14],  TMI_EXT_136_14, PBSZ_136, 1, 1, FEC_RATE_1_2,   1, MM_16QAM);
#elif defined(STD_2016) || defined(STD_DUAL)
	fill_sys_tonemap(tm[TMI_ROBO_520_0],  TMI_ROBO_520_0,  PBSZ_520, 1, 4, FEC_RATE_1_2, 4,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_1],  TMI_ROBO_520_1,  PBSZ_520, 1, 4, FEC_RATE_1_2, 2,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_2],  TMI_ROBO_136_2,  PBSZ_136, 1, 4, FEC_RATE_1_2, 5,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_3],  TMI_ROBO_136_3,  PBSZ_136, 1, 4, FEC_RATE_1_2, 11, MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_4],  TMI_ROBO_136_4,  PBSZ_136, 1, 4, FEC_RATE_1_2, 7,  MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_5],  TMI_ROBO_136_5,  PBSZ_136, 1, 4, FEC_RATE_1_2, 11, MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_136_6],  TMI_ROBO_136_6,  PBSZ_136, 1, 4, FEC_RATE_1_2, 7,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_7],  TMI_ROBO_520_7,  PBSZ_520, 1, 3, FEC_RATE_1_2, 7,  MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_8],  TMI_ROBO_520_8,  PBSZ_520, 1, 4, FEC_RATE_1_2, 4,  MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_9],  TMI_ROBO_520_9,  PBSZ_520, 1, 4, FEC_RATE_1_2, 7,  MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_520_10], TMI_ROBO_520_10, PBSZ_520, 1, 4, FEC_RATE_1_2, 2,  MM_BPSK);

	fill_sys_tonemap(tm[TMI_ROBO_264_11], TMI_ROBO_264_11, PBSZ_264, 1, 4, FEC_RATE_1_2, 7, MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_264_12], TMI_ROBO_264_12, PBSZ_264, 1, 4, FEC_RATE_1_2, 7, MM_BPSK);
	fill_sys_tonemap(tm[TMI_ROBO_72_13],  TMI_ROBO_72_13,  PBSZ_72,  1, 4, FEC_RATE_1_2, 7, MM_QPSK);
	fill_sys_tonemap(tm[TMI_ROBO_72_14],  TMI_ROBO_72_14,  PBSZ_72,  1, 4, FEC_RATE_1_2, 7, MM_BPSK);

	fill_sys_tonemap(tm[TMI_EXT_520_1],   TMI_EXT_520_1,  PBSZ_520, 1, 4, FEC_RATE_16_18, 1, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_2],   TMI_EXT_520_2,  PBSZ_520, 1, 4, FEC_RATE_16_18, 2, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_3],   TMI_EXT_520_3,  PBSZ_520, 1, 4, FEC_RATE_1_2,   1, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_4],   TMI_EXT_520_4,  PBSZ_520, 1, 4, FEC_RATE_1_2,   2, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_5],   TMI_EXT_520_5,  PBSZ_520, 1, 4, FEC_RATE_1_2,   4, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_520_6],   TMI_EXT_520_6,  PBSZ_520, 1, 4, FEC_RATE_1_2,   1, MM_QPSK);
	fill_sys_tonemap(tm[TMI_EXT_136_10],  TMI_EXT_136_10, PBSZ_136, 1, 4, FEC_RATE_1_2,   5, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_136_11],  TMI_EXT_136_11, PBSZ_136, 1, 4, FEC_RATE_1_2,   2, MM_QPSK);
	fill_sys_tonemap(tm[TMI_EXT_136_12],  TMI_EXT_136_12, PBSZ_136, 1, 4, FEC_RATE_1_2,   2, MM_16QAM);
	fill_sys_tonemap(tm[TMI_EXT_136_13],  TMI_EXT_136_13, PBSZ_136, 1, 4, FEC_RATE_1_2,   1, MM_QPSK);
	fill_sys_tonemap(tm[TMI_EXT_136_14],  TMI_EXT_136_14, PBSZ_136, 1, 4, FEC_RATE_1_2,   1, MM_16QAM);
#endif

	for (i = 0; i < NR_TONEMAP_MAX; ++i)
		config_sys_tonemap(&tm[i]);

	return;
}

double phy_real_rate(tonemap_t *tm, uint8_t pbc)
{
	uint32_t symn;
	double fl, rate;

	symn = robo_pb_to_pld_sym(pbc, tm->robo);
#ifdef DYNAMIC_CONN
	fl = PREAMBLE_DUR + FRAME_CTRL_DUR(CURR_B->symn) + pld_sym_to_time(symn) + 
		SACK_FRAME_DUR(CURR_B->symn) + PHY_CIFS;
#else
	fl = PREAMBLE_DUR + FRAME_CTRL_DUR(CURR_B->symn) + pld_sym_to_time(symn) + 
		PHY_CIFS;
#endif
	fl /= 25.0;
	rate = pbsz_val[tm->pbsz] * pbc * 8 / fl;

	return rate;
}

static void print_tonemap_info(tty_t *term, tonemap_t *tm)
{
	char str[4], sym[10];

	if (tm->pbc_min != tm->pbc_max) {
		__snprintf(str, sizeof(str), "%u~%u", tm->pbc_min, tm->pbc_max);
		__snprintf(sym, sizeof(sym), "%-4d~%-4d", robo_pb_to_pld_sym(tm->pbc_min, tm->robo), robo_pb_to_pld_sym(tm->pbc_max, tm->robo));
	} else {
		__snprintf(str, sizeof(str), "%u", tm->pbc_min);
		__snprintf(sym, sizeof(sym), "%-3d", robo_pb_to_pld_sym(tm->pbc_min, tm->robo));
	}

	term->op->tprintf(term, "%-4d %-14s %-4s %-4u %-5s %-7s %-3s %-9s %-8.4f~ %-8.4f\n", tm->tmi, 
			  tonemap_name[tm->tmi], pbsz_getstr(tm->pbsz), tm->copy, 
			  mm_getstr(tm->mm), fecr_getstr(tm->fec_rate), str, sym,
			  phy_real_rate(tm, tm->pbc_min), phy_real_rate(tm, tm->pbc_max));
}


void phy_tonemap_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;
	uint32_t i;
	tonemap_t *tm;

	term->op->tputs(term, "ROBO basic mode: \n");
	term->op->tputs(term, "TMI  ToneMapName    PBsz Copy MM    FECRate PBC SYM       Rate(Mbps)\n");
	term->op->tputs(term, "-----------------------------------------------------------------------------\n");
	for (i = 0; i < 16; ++i) {
		if (!(tm = get_sys_tonemap(i))) continue;
		print_tonemap_info(term, tm);
	}

	term->op->tputs(term, "\n");

	term->op->tputs(term, "ROBO extend mode: \n");
	term->op->tputs(term, "TMIE ToneMapName    PBsz Copy MM    FECRate PBC SYM       Rate(Mbps)\n");
	term->op->tputs(term, "-----------------------------------------------------------------------------\n");
	for (i = 16; i <= TMI_EXT_136_14; ++i) {
		if (!(tm = get_sys_tonemap(i))) continue;
		print_tonemap_info(term, tm);
	}

	term->op->tputs(term, "\n");
}

const uint16_t body_size[]=
#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
    {64,128,256,512};
#elif defined(STD_2016) || defined(STD_DUAL)
    {68,132,260,516};
#endif

phy_band_info_t BAND[] = {
#if defined(STD_2012) || defined(STD_GD)
	{
		.start = 100,
		.end   = 230,
		.symn  = 8,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 0,
	},
	{
		.start = 80,
		.end   = 490,
		.symn  = 2,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 1,
	},
#elif defined(STD_SPG)
	{
		.start = 80,
		.end   = 490,
		.symn  = 2,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 0,
	},
	{
		.start = 100,
		.end   = 230,
		.symn  = 8,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 1,
	},
	{
		.start = 32,
		.end   = 120,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 2,
	},
#elif defined(STD_2016) || defined(STD_DUAL)
	{
		.start = 80,
		.end   = 490,
		.symn  = 4,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 0,
	},
	{
		.start = 100,
		.end   = 230,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 1,
	},
	{
		.start = 32,
		.end   = 120,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 2,
	},
	{
		.start = 72,
		.end   = 120,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 3,

	},
	{
		.start = 123,
		.end   = 230,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 4,

	},
	{
		.start = 30,
		.end   = 90,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 5,

	},
	{
		.start = 10,
		.end   = 70,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 6,

	},
	{
		.start = 29,
		.end   = 98,
		.symn  = 12,
		.mm    = MM_QPSK,
		.size  = HDR_SIZE_16,
		.idx   = 7,

	},
#else
#error "Unknown std version!"
#endif
};

static phy_scmask_t MASK[] = {
#if defined(STD_2012) || defined(STD_GD)
	{
		.start   = 100,
		.end     = 230,
		.nr_band = 0,
	},
	{
		.start   = 80,
		.end     = 490,
		.nr_band = 0,
	},
#elif defined(STD_SPG)
	{
		.start   = 80,
		.end     = 490,
		.nr_band = 0,
	},
	{
		.start   = 100,
		.end     = 230,
		.nr_band = 0,
	},
	{
		.start   = 32,
		.end     = 120,
		.nr_band = 0,
	},
#elif defined(STD_2016) || defined(STD_DUAL)
	{
		.start   = 80,
		.end     = 490,
		.nr_band = 6,
	},
	{
		.start   = 100,
		.end     = 230,
		.nr_band = 4,
	},
	{
		.start   = 32,
		.end     = 120,
		.nr_band = 2,
	},
	{
		.start   = 72,
		.end     = 120,
		.nr_band = 3,
	},
	{
		.start   = 123,
		.end     = 230,
		.nr_band = 4,
	},
	{
		.start   = 30,
		.end     = 90,
		.nr_band = 5,
	},
	{
		.start   = 10,
		.end     = 70,
		.nr_band = 6,
	},
	{
		.start   = 29,
		.end     = 98,
		.nr_band = 7,
	},
#else
#error "Unknown std version!"
#endif
};

static phy_notch_band_t NBAND[][6] = {
#if defined STD_2016 || defined(STD_DUAL)
	{
		{
			.start = 81,
			.end   = 82,
		},
		{
			.start = 85,	
			.end   = 85,
		},
		{
			.start = 87,
			.end   = 88,
		},
		{
			.start = 92,
			.end   = 94,
		},
		{
			.start = 96,
			.end   = 97,
		},
		{
			.start = 102,
			.end   = 131,
		},
	},
	{
		{
			.start = 101,
			.end   = 101,
		},
		{
			.start = 110,
			.end   = 110,
		},
		{
			.start = 112,
			.end   = 113,
		},
		{
			.start = 117,	
			.end   = 117,
		},
	},
	{
		{
			.start = 43,
			.end   = 45,
		},
		{
			.start = 50,
			.end   = 51,
		},
	},
	{
		{
			.start = 75,
			.end   = 75,
		},	
		{
			.start = 77,
			.end   = 78,
		},
		{
			.start = 83,
			.end   = 83,
		},
	},
#endif
};

phy_band_info_t *CURR_B = NULL;
phy_scmask_t *CURR_M = NULL;

phy_band_info_t * phy_band_config(int32_t idx)
{
	phy_scmask_t scmask;
#if defined(STD_SPG)
	phy_phase_tbl_t phase_tbl;
	phy_hdr_sym_ofs_t ofs;
#endif

	if(idx >= (sizeof(BAND)/sizeof(phy_band_info_t)))
		return NULL;

	scmask.start   = BAND[idx].start;
	scmask.end     = BAND[idx].end;
	scmask.nr_band = 0;
	scmask.band    = NULL;
#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
	uint16_t symb_dur = (idx != 5) ? 1024 : 1024 * 3;
	phy_ioctl(PHY_CMD_SET_SYMB_DUR, &symb_dur);
#endif

#if defined(STD_SPG)
	if (idx == 0 || idx == 1) {
		phase_tbl = PHASE_TBL_2012;
		ofs = HDR_SYM_OFS_2012;
	} else if (idx == 2) {
		phase_tbl = PHASE_TBL_2016;
		ofs = HDR_SYM_OFS_2016;
	}

	if (phy_ioctl(PHY_CMD_SET_PHASE_TBL, &phase_tbl) < 0) {
		debug_printf(&dty, DEBUG_STA, "phy set phase table failed\n");
		return NULL;
	}
	if (phy_ioctl(PHY_CMD_SET_HDR_SYM_OFS, &ofs) < 0) {
		debug_printf(&dty, DEBUG_STA, "phy set hdr symbol offset failed\n");
		return NULL;
	}
#endif
	if (phy_ioctl(PHY_CMD_SET_SCMASK, &scmask) < 0) {
		debug_printf(&dty, DEBUG_STA, "phy set scmask failed\n");
		return NULL;
	}

	/* update tonemap after set scmask */
	update_tonemap();
	CURR_B = &BAND[idx];

	if(CURR_B)
	{
		HPLC.hi.symn = CURR_B->symn;
		HPLC.hi.mm   = CURR_B->mm;
		HPLC.hi.size = CURR_B->size;
	}

#if defined(ROLAND1_1M) || defined(ROLAND9_1M)
	phy_dcdc_ctr_t ctr;
	phy_pd_thrd_t pd;
	phy_agc2_gain_cfg_t agc2;
	phy_agc2_stdet_papr_t papr;
	phy_agc2_refp_st_dbmv_t dbmv;

	if (idx == 0) {
		pd.thrd   = 0x80;

		ctr.vctr  = 0x29;
		ctr.sfreq = 0x0;

		agc2.init_value = 66;
		agc2.max_value  = 66;

		dbmv.value = 47;
		papr.value = 16;
	} else {
		pd.thrd   = 0x100;

		ctr.vctr  = 0x0d;
		ctr.sfreq = 0x3;

		agc2.init_value = 60;
		agc2.max_value  = 60;

		dbmv.value = 45;
		papr.value = 18;
	}

	phy_ioctl(PHY_CMD_SET_DCDC_CTR, &ctr);
	phy_ioctl(PHY_CMD_SET_PD_THRD, &pd);
	phy_ioctl(PHY_CMD_SET_AGC2_GAIN_CFG, &agc2);
	phy_ioctl(PHY_CMD_SET_AGC2_STDET_PAPR, &papr);
	phy_ioctl(PHY_CMD_SET_AGC2_REFP_ST_DBMV, &dbmv);

#if defined(ROLAND9_1M)
	phy_perf_mode_t mode;
	phy_agc2_refp_dbmv_t rdbmv;

	phy_ioctl(PHY_CMD_GET_PERF_MODE, &mode);
	if (mode == PHY_PERF_MODE_B && idx == 0) {
		rdbmv.value = 0x2d;
		phy_ioctl(PHY_CMD_SET_AGC2_REFP_DBMV, &rdbmv);
	}
#endif
#elif defined(MIZAR1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
	phy_dcdc_ctr_t ctr;
	phy_pd_thrd_t pd;
	phy_agc2_gain_cfg_t agc2;
	phy_agc2_stdet_papr_t papr;
	phy_agc2_refp_st_dbmv_t dbmv;
	phy_ana_rx_bpf_t bpf;
	uint32_t imdet;

	if (idx == 0) {
		pd.thrd   = 0x80;
		ctr.vctr  = 0x29;
		ctr.sfreq = 0x0;
	} else {
#if defined(VENUS_V5)
		pd.thrd   = 0x80;
#else
		pd.thrd   = 0x100;
#endif
		ctr.vctr  = 0x0d;
		ctr.sfreq = 0x3;
	}

	if (idx == 0) {
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 5;
#elif defined(VENUS_V2) || defined(VENUS_V4)
		bpf.hpf_bw = 0x555;
		bpf.lpf_bw = 0;
#elif defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 5;
#else
		bpf.hpf_bw = 1;
		bpf.lpf_bw = 0;
#endif

		agc2.init_value = 74;
		agc2.max_value  = 74;
		imdet = 10;
	} else if (idx == 1) {
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 4;
#elif defined(VENUS_V2) || defined(VENUS_V4) 
		bpf.hpf_bw = 0x555;
		bpf.lpf_bw = 1;
#elif defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 4;
#else
		bpf.hpf_bw = 3;
		bpf.lpf_bw = 1;
#endif

		agc2.init_value = 69;
		agc2.max_value  = 69;
		imdet = 10;
	} else if (idx == 2) {
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 3;
		imdet = 10;
#elif defined(VENUS_V2) || defined(VENUS_V4)
		bpf.hpf_bw = 0x555;
		bpf.lpf_bw = 2;
		imdet = 8;
#elif  defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 3;
		imdet = 10;
#else
		bpf.hpf_bw = 1;
		bpf.lpf_bw = 2;
		imdet = 8;
#endif

		agc2.init_value = 64;
		agc2.max_value  = 73;
	} else if (idx == 3) {
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 2;
#elif defined(VENUS_V2) || defined(VENUS_V4)
		bpf.hpf_bw = 0x555;
		bpf.lpf_bw = 2;
#elif defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 2;
#else
		bpf.hpf_bw = 3;
		bpf.lpf_bw = 2;
#endif
		agc2.init_value = 58;
		agc2.max_value  = 58;
		imdet = 10;
	} else if (idx == 4) { //band 1 configure
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 4;
#elif defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 4;
#else
		bpf.hpf_bw = 3;
		bpf.lpf_bw = 1;
#endif

		agc2.init_value = 69;
		agc2.max_value  = 69;
		imdet = 10;
	} else if (idx == 5) {
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 3;
		imdet = 10;
#elif defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 3;
		imdet = 10;
#else
		bpf.hpf_bw = 0;
		bpf.lpf_bw = 2;
		imdet = 8;
#endif

		agc2.init_value = 64;
		agc2.max_value  = 73;
	} else if (idx == 6 || idx == 7) { //band 2 configure
#if defined(VENUS_V3)
		bpf.hpf_bw = 0x6666;
		bpf.lpf_bw = 3;
		imdet = 10;
#elif defined(VENUS_V5)
		bpf.hpf_bw = 0x24666;
		bpf.lpf_bw = 3;
		imdet = 10;
#else
		bpf.hpf_bw = 1;
		bpf.lpf_bw = 2;
		imdet = 8;
#endif
		agc2.init_value = 64;
		agc2.max_value  = 73;
	}

#if defined(VENUS2M) || defined(VENUS8M)
	if (idx == 3) {
		dbmv.value = 44;
	} else {
	#if defined(VENUS_V2) || defined(VENUS_V3) || defined(VENUS_V4)
		dbmv.value = 46;
	#else
		dbmv.value = 47;
	#endif
	}
#else
	dbmv.value = 45;
#endif
	papr.value = 24;

	phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
	phy_ioctl(PHY_CMD_SET_DCDC_CTR, &ctr);
	phy_ioctl(PHY_CMD_SET_PD_THRD, &pd);
	phy_ioctl(PHY_CMD_SET_AGC2_GAIN_CFG, &agc2);
	phy_ioctl(PHY_CMD_SET_AGC2_STDET_PAPR, &papr);
	phy_ioctl(PHY_CMD_SET_AGC2_REFP_ST_DBMV, &dbmv);
	phy_ioctl(PHY_CMD_SET_IMDET_THRD, &imdet);

	phy_agc2_stdet_t stdet;
	stdet.band = idx;
	phy_ioctl(PHY_CMD_SET_AGC2_STDET, &stdet);
	*(int *)(0xf1100098) = 0x1e1e1e1e; // 抗窄带 SDK ver_03-02-12-01 默认修改值为0x14141414
#endif

	return CURR_B;
}

void phy_set_band_info(int32_t idx, phy_band_info_t *band)
{
	__memcpy(&BAND[idx], band, sizeof(phy_band_info_t));
}

void phy_get_band_info(int32_t idx, phy_band_info_t *band)
{
	__memcpy(band, &BAND[idx], sizeof(phy_band_info_t));
}
void phy_gain_config(int8_t ana,int8_t dig)
{
    phy_tgain_t tgain;
    tgain.dig = dig;
    tgain.ana = ana;
	
	phy_ioctl(PHY_CMD_SET_TGAIN, &tgain);
}

phy_scmask_t * phy_scmask_config(int32_t idx)
{
	phy_scmask_t *scmask = &MASK[idx];

	scmask->band = NBAND[idx];

	if (CURR_B->idx != idx) {
		debug_printf(&dty, DEBUG_STA, "Try to set scmask#%d while curr band#%d!\n", idx, CURR_B->idx);
		return NULL;
	}

	if (phy_ioctl(PHY_CMD_SET_SCMASK, scmask) < 0) {
		debug_printf(&dty, DEBUG_STA, "phy set scmask failed\n");
		return NULL;
	}

	/* update tonemap after set scmask */
	update_tonemap();

	CURR_M = &MASK[idx];

	return CURR_M;
}


int32_t phy_set_std_ver(void)
{
	phy_intl_en_t intl_en;
	phy_intl_mode_t intl_mode;
	phy_scrm_mode_t scrm;
	phy_pb_fmt_t fmt;
	phy_robo_mode_t robo_mode;
	phy_tm_ver_t tm_ver;

	while (phy_reset() != OK)
		sys_delayus(100);

#if defined(STD_2012) || defined(STD_GD)
	phy_phase_tbl_t phase_tbl;
	phy_hdr_sym_ofs_t ofs;

	phase_tbl = PHASE_TBL_2012;
	if (phy_ioctl(PHY_CMD_SET_PHASE_TBL, &phase_tbl) < 0)
		return -1;

	ofs = HDR_SYM_OFS_2012;
	if (phy_ioctl(PHY_CMD_SET_HDR_SYM_OFS, &ofs) < 0)
		return -2;

	intl_en = INTLEAVER_DISABLE;
	if (phy_ioctl(PHY_CMD_SET_INTLEAVER_EN, &intl_en) < 0)
		return -3;

	intl_mode = INTLEAVER_BPLC;
	if (phy_ioctl(PHY_CMD_SET_INTLEAVER_MODE, &intl_mode) < 0)
		return -4;

	scrm.scrm_mode = 1;
	scrm.scrm_hdr_en = 0;
	scrm.poly_mode = SCRM_POLY_MODE1;
	if (phy_ioctl(PHY_CMD_SET_SCRM_MODE, &scrm) < 0)
		return -5;

	fmt = PB_FORMAT_0;
	if (phy_ioctl(PHY_CMD_SET_PB_FMT, &fmt) < 0)
		return -6;

	robo_mode = ROBO_MODE_0;
	if (phy_ioctl(PHY_CMD_SET_ROBO_MODE, &robo_mode) < 0)
		return -7;

	tm_ver = -1;
	if (phy_ioctl(PHY_CMD_SET_TM_VER, &tm_ver) < 0)
		return -8;

#elif defined(STD_SPG)
	intl_en = INTLEAVER_DISABLE;
	if (phy_ioctl(PHY_CMD_SET_INTLEAVER_EN, &intl_en) < 0)
		return -3;

	intl_mode = INTLEAVER_BPLC;
	if (phy_ioctl(PHY_CMD_SET_INTLEAVER_MODE, &intl_mode) < 0)
		return -4;

	scrm.scrm_mode = 1;
	scrm.scrm_hdr_en = 0;
	scrm.poly_mode = SCRM_POLY_MODE1;
	if (phy_ioctl(PHY_CMD_SET_SCRM_MODE, &scrm) < 0)
		return -5;

	fmt = PB_FORMAT_0;
	if (phy_ioctl(PHY_CMD_SET_PB_FMT, &fmt) < 0)
		return -6;

	robo_mode = ROBO_MODE_0;
	if (phy_ioctl(PHY_CMD_SET_ROBO_MODE, &robo_mode) < 0)
		return -7;

	tm_ver = TM_VER_0;
	if (phy_ioctl(PHY_CMD_SET_TM_VER, &tm_ver) < 0)
		return -8;
#elif defined(STD_2016) || defined(STD_DUAL)
	phy_phase_tbl_t phase_tbl;
	phy_hdr_sym_ofs_t ofs;

	phase_tbl = PHASE_TBL_2016;
	if (phy_ioctl(PHY_CMD_SET_PHASE_TBL, &phase_tbl) < 0)
		return -1;

	ofs = HDR_SYM_OFS_2016;
	if (phy_ioctl(PHY_CMD_SET_HDR_SYM_OFS, &ofs) < 0)
		return -2;

	intl_en = INTLEAVER_ENABLE;
	if (phy_ioctl(PHY_CMD_SET_INTLEAVER_EN, &intl_en) < 0)
		return -3;

	intl_mode = INTLEAVER_HPAV;
	if (phy_ioctl(PHY_CMD_SET_INTLEAVER_MODE, &intl_mode) < 0)
		return -4;

	scrm.scrm_mode = 0;
	scrm.scrm_hdr_en = 0;
	scrm.poly_mode = SCRM_POLY_MODE1;
	if (phy_ioctl(PHY_CMD_SET_SCRM_MODE, &scrm) < 0)
		return -5;

	fmt = PB_FORMAT_1;
	if (phy_ioctl(PHY_CMD_SET_PB_FMT, &fmt) < 0)
		return -6;

	robo_mode = ROBO_MODE_1;
	if (phy_ioctl(PHY_CMD_SET_ROBO_MODE, &robo_mode) < 0)
		return -7;

	tm_ver = TM_VER_1;
	if (phy_ioctl(PHY_CMD_SET_TM_VER, &tm_ver) < 0)
		return -8;
#endif
	return 0;
}

uint32_t wphy_ch_to_hz(uint8_t option, uint32_t ch)
{
#if 1
	if (option == WPHY_OPTION_1)
		return 471000000 + (ch - 1) * 1000000;
	else if (option == WPHY_OPTION_2)
		return 470500000 + (ch - 1) *  500000;
	else if (option == WPHY_OPTION_3)
		return 470100000 + (ch - 1) *  200000;
	else
		return 0;
#else
	if (option == WPHY_OPTION_1)
		return 921000000 + (ch - 1) * 1000000;
	else if (option == WPHY_OPTION_2)
		return 920500000 + (ch - 1) *  500000;
	else if (option == WPHY_OPTION_3)
		return 920100000 + (ch - 1) *  200000;
	else
		return 0;
#endif
}
uint32_t hz_ceil(int temp, int k)
{
	uint32_t result;
	result = 0;
	result = (temp + k - 1)/k;
    return result;
}
uint32_t wphy_hz_to_ch(uint8_t option, uint32_t hz)
{
#if 1
	int32_t khz = hz / 1000;

	if (option == WPHY_OPTION_1)
		return (khz < 471000) ? 1 : hz_ceil((khz - 471000) , 1000) + 1;//(khz - 471000) / 1000 + 1;
	else if (option == WPHY_OPTION_2)
		return (khz < 470500) ? 1 : hz_ceil((khz - 470500) , 500) + 1;//(khz - 470500) / 500 + 1;
	else if (option == WPHY_OPTION_3)
		return (khz < 470100) ? 1 : hz_ceil((khz - 470100) , 200) + 1;//(khz - 470100) / 200 + 1;
#else
	int32_t khz = hz / 1000;

	if (option == WPHY_OPTION_1)
		return (khz < 921000) ? 1 : (khz - 921000) / 1000 + 1;
	else if (option == WPHY_OPTION_2)
		return (khz < 920500) ? 1 : (khz - 920500) / 500 + 1;
	else if (option == WPHY_OPTION_3)
		return (khz < 920100) ? 1 : (khz - 920100) / 200 + 1;
#endif
	return 0;
}
__SLOWTEXT int32_t phy_sal_init(uint8_t band_index)
{
	int32_t ret;

	if (phy_init() < 0)
		return -EPERM;

	tonemap_init();

	if ((ret = phy_set_std_ver()) < 0) {
		printf_s("phy set std ver failed, ret=%d\n", ret);
		return -EPERM;
	}

    phy_band_config(band_index);
    HPLC.band_idx = band_index; //<<<
	print_s("phy sal init ok!\n");
	return OK;
}
