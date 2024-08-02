/*
 * Copyright: (c) 2016-2020, 2016 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        phy_demo.c
 * Purpose:     demo procedure for using libphy.a
 * History:
 */
#include "trios.h"
#include "sys.h"
#include "framectrl.h"
#include "phy_config.h"
#include "phy_sal.h"
#include "vsh.h"
#include "mem_zc.h"
#include "phy_plc_isr.h"
#include "csma.h"
#include "plc_cnst.h"
#include "timeslot_equalization.h"
#include "mac_common.h"
#include "DataLinkInterface.h"
#include "crc.h"
#define NR_HDR_SYM_MAX		16

#define PHY_DEMO_STOP		0
#define PHY_DEMO_TX		1
#define PHY_DEMO_RX		2
#define PHY_DEMO_CAP		3

#define BEACON_PERIOD_MIN	40 /* ms */
#define phy_debug_printf(fmt, arg...) debug_printf(&dty, DEBUG_PHY, fmt, ##arg)

extern void phy_pmd_clock_always_on();
extern void phy_pmd_clock_auto_switch();


extern phy_rxtx_cnt_t phy_rxtx_info;

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
	uint8_t pld[524] __CACHEALIGNED;
} phy_demo_t;

phy_demo_t PHY_DEMO;
sfo_handle_t HSFO;
evt_notifier_t rx_hdr_evt;

static void docmd_phy_tx_start(xsh_t *xsh);
static void docmd_phy_tx_stop(xsh_t *xsh);
static void docmd_phy_rx_start(xsh_t *xsh);
static void docmd_phy_rx_stop(xsh_t *xsh);
static void docmd_phy_pma_show(xsh_t *xsh);
static void docmd_phy_pma_clear(xsh_t *xsh);
static void docmd_phy_perf_show(xsh_t *xsh);
static void docmd_phy_notch_show(xsh_t *xsh);
static void docmd_phy_perf_set(xsh_t *xsh);
static void docmd_phy_band_set(xsh_t *xsh);
static void phy_demo_tx_evt(phy_evt_param_t *para, void *arg);
static void phy_demo_rx_evt(phy_evt_param_t *para, void *arg);
static void phy_demo_rx_frame(phy_frame_info_t *pf, uint32_t stime, int32_t gain);
static int32_t frame_construct(frame_ctrl_t *head, uint8_t dt, uint8_t pbc, uint16_t dtei, uint16_t symn, 
			uint16_t hdr_symn, tonemap_t *tm);
static void phy_demo_stop(xsh_t *xsh);
static void docmd_phy_cap_stop(xsh_t *xsh);
static void docmd_phy_cap_start(xsh_t *xsh);
static void docmd_phy_cap_show(xsh_t *xsh);
static void docmd_phy_band_show(xsh_t *xsh);
static void docmd_phy_tgain_show(xsh_t *xsh);
static void docmd_phy_tgain_set(xsh_t *xsh);
static void docmd_phy_rfchannel_set(xsh_t *xsh);
static void docmd_phy_msg_set(xsh_t *xsh);
static void docmd_phy_mem_set(xsh_t *xsh);
static void docmd_sort_slot(xsh_t *xsh);
static void docmd_phy_test_set(xsh_t *xsh);

static void docmd_phy_rxtx_show(xsh_t *xsh);
static void docmd_phy_rxtx_clear(xsh_t *xsh);
static void docmd_phy_hplc_statis_show(xsh_t *xsh);
static void docmd_phy_hplc_statis_clear(xsh_t *xsh);
static void docmd_phy_me_show(xsh_t *xsh);

static void docmd_phy_lowpower_set(xsh_t *xsh);

static void phy_sfo_show_demo(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	term->op->tprintf(term, "tx sfo: %.3fppm\n", phy_get_sfo(SFO_DIR_TX));
	term->op->tprintf(term, "rx sfo: %.3fppm\n", phy_get_sfo(SFO_DIR_RX));
}

static void docmd_phy_scmask_set_demo(xsh_t *xsh)
{
	phy_scmask_t scmask;
	phy_notch_band_t nband[5];
	tty_t *term = xsh->term;

	scmask.band = &nband[0];
	if (phy_ioctl(PHY_CMD_GET_SCMASK, &scmask) != OK) {
		term->op->tputs(term, "phy ioctl get scmask failed\n");
		return;
	}

	scmask.start   = __strtoul(xsh->argv[3], NULL, 0);
	scmask.end     = __strtoul(xsh->argv[4], NULL, 0);

	if (phy_ioctl(PHY_CMD_SET_SCMASK, &scmask) != OK) {
		term->op->tputs(term, "phy ioctl set scmask failed\n");
		return;
	}

	update_tonemap();
	return;
}

static void docmd_phy_rgain_set_demo(xsh_t *xsh)
{
	int32_t rgain;		

	rgain = (xsh->argc > 3) ? __atoi(xsh->argv[3]) : 255;

	phy_ioctl(PHY_CMD_SET_RGAIN, &rgain);
}

static void docmd_phy_notch_set_demo(xsh_t *xsh)
{
	phy_notch_freq_t notch;
	tty_t *term = xsh->term;

	notch.freq = __strtoul(xsh->argv[3], NULL, 0);
	notch.idx  = __strtoul(xsh->argv[4], NULL, 0); 
	if (phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch) != OK) {
		term->op->tputs(term, "phy ioctl set notch freq failed\n");
		return;
	}
}

__LOCALTEXT __isr__ int32_t demo_clock_sync(phy_evt_param_t *para, void *arg)
{
	beacon_ctrl_t *beacon;
	int32_t sfo_int;
	int32_t diff, interval;

    if(para->fail)
    {
        return 0;
    }
    if(!header_is_valid(para->fi))
    {
        return 0;
    }

	
	beacon = (beacon_ctrl_t *)para->fi->head;
	if (beacon->dt != DT_BEACON)
		return 0;
	
	if (PHY_DEMO.snid && PHY_DEMO.snid != beacon->snid)
		return 0;

	diff = (int32_t)time_diff(beacon->bts, para->stime);
	interval = (int32_t)time_diff(beacon->bts, PHY_DEMO.sfo_bts);
	if (PHY_DEMO.sfo_not_first && interval < PHY_US2TICK(1000 * 1000))
		return 0;
	
	if (diff > -0x8000 && diff < 0x8000) {
		set_phy_tick_adj(diff);
		if (phy_iterate_sfo(&sfo_int, beacon->bts, para->stime, &HSFO) == OK)
			phy_set_sfo(SFO_DIR_TX|SFO_DIR_RX, sfo_int, HSFO.adjust);
	} else {
		set_phy_tick_time(get_phy_tick_time() + diff);
	}

	PHY_DEMO.sfo_not_first = 1;
	PHY_DEMO.sfo_bts       = beacon->bts;

	return 0;
}


__LOCALTEXT int32_t do_beacon_hdr(phy_frame_info_t *pf, void *context)
{
	beacon_ctrl_t *bcon = (beacon_ctrl_t *)pf->head;
	tonemap_t *tm;
	int32_t tmi;

	pf->pi.symn = get_beacon_symn(bcon);
	pf->pi.pbc  = get_beacon_pbc(bcon);
	tmi         = get_beacon_tmi(bcon);

	if (!(tm = get_sys_tonemap(tmi)))
		return ERROR;

	if (pf->pi.symn != robo_pb_to_pld_sym(pf->pi.pbc, tm->robo))
		return ERROR;

	pf->pi.pbsz     = tm->pbsz;
	pf->pi.fec_rate = tm->fec_rate;
	pf->pi.copy     = tm->copy;
	pf->pi.mm       = tm->mm;
	pf->pi.robo	= tm->robo;
	pf->pi.tmi      = tmi;
    pf->pi.crc32_en  = 1;
    pf->pi.crc32_ofs = 7;

	pf->length = pb_bufsize(pf->pi.pbsz) * pf->pi.pbc;

	return OK;
}

__LOCALTEXT int32_t do_sof_hdr(phy_frame_info_t *pf, void *context)
{
	sof_ctrl_t *sof = (sof_ctrl_t *)pf->head;
	tonemap_t *tm;
	int32_t tmi;

	if (!(pf->pi.symn = get_sof_symn(sof))) {
		pf->length = 0;
		return OK;
	}

	tmi = get_sof_tmi(sof);
	if (!(tm = get_sys_tonemap(tmi)))
		return ERROR;

	if (pf->pi.symn != robo_pb_to_pld_sym(sof->pbc, tm->robo))
		return ERROR;

	pf->pi.pbsz     = tm->pbsz;
	pf->pi.fec_rate = tm->fec_rate;
	pf->pi.copy     = tm->copy;
	pf->pi.mm       = tm->mm;
	pf->pi.bat      = NULL;
	pf->pi.pbc      = sof->pbc;
	pf->pi.robo	= tm->robo;
	pf->pi.tmi      = tmi;
	pf->length = pb_bufsize(pf->pi.pbsz) * pf->pi.pbc;

	return OK;
}

__LOCALTEXT int32_t do_sack_hdr(phy_frame_info_t *pf, void *context)
{
	pf->length = 0;
	return OK;
}

__LOCALTEXT int32_t do_coord_hdr(phy_frame_info_t *pf, void *context)
{
	pf->length = 0;
	return OK;
}

typedef int32_t (*phy_hdr_proc)(phy_frame_info_t *pf, void *context);
static phy_hdr_proc hdr_proc[] = {
	do_beacon_hdr,		/* 0, DT_BEACON */
	do_sof_hdr,		/* 1, DT_SOF */
	do_sack_hdr,		/* 2, DT_SACK */
	do_coord_hdr,		/* 3, DT_COORD */
	NULL,			/* 4, DT_SOUND */
	NULL,			/* 5 */
	NULL,			/* 6 */
	NULL,			/* 7, DT_SILENT */
};


__LOCALTEXT __isr__ void isr_hdr_demo_rx(phy_frame_info_t *pf, void *context)
{
	frame_ctrl_t *head = (frame_ctrl_t *)pf->head;

	if (!header_is_valid(pf))
		goto out;

#if defined(STD_2016) || defined(STD_DUAL)
	if (head->access) {
#else
	if (!head->access) {
#endif
		pf->hi.valid = HDR_INVALID_PROC;
		goto out;
	}

	if (!hdr_proc[head->dt] || hdr_proc[head->dt](pf, context) != OK) {
		pf->hi.valid = HDR_INVALID_PROC;
		goto out;
	}
out:
	phy_frame_rx(pf, context);
	return;
}


param_t cmd_phy_demo_param_tab[] = {
	{ PARAM_ARG | PARAM_TOGGLE, "", "start|stop|show|clear|set|reset|init|lowpower" },

	{ PARAM_ARG | PARAM_TOGGLE	| PARAM_SUB, "", "tx|rx|cap", "start" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "sof|beacon|sack", "tx" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num", "", "sof" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "pbsz", "72|136|264|520", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "pbc", "PB Number", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bat", "RT bitloading", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "tmi", "Tone Map Index", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "dtei", "Destination TEI", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bp", "Beacon Period(ms)", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "hs", "Header Symbol Number", "sof" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hm", "BPSK|QPSK", "sof" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hz", "16|32", "sof" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "crc", "ok|err", "sof" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "mac", "l|s", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "byte", "write one byte", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "len", "msdu len", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "stime", "start frame time", "sof" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num", "", "beacon" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "pbsz", "136|520", "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "pbc", "PB Number", "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "tmi", "Tone Map Index", "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "dtei", "Destination TEI", "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bp", "Beacon Period(ms)", "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "hs", "Header Symbol Number", "beacon" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hm", "BPSK|QPSK", "beacon" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hz", "16|32", "beacon" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "crc", "24|32", "beacon" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num", "", "sack" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "pbsz", "136|520", "sack" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "pbc", "PB Number", "sack" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "tmi", "Tone Map Index", "sack" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "dtei", "Destination TEI", "sack" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bp", "Beacon Period(ms)", "sack" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "hs", "Header Symbol Number", "sack" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hm", "BPSK|QPSK", "sack" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hz", "16|32", "beacon" },

	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "pbsz", "136|520", "rx" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bat", "RT bitloading", "rx" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "dur", "rx duration ticks", "rx" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "hs", "Header Symbol Number", "rx" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hm", "BPSK|QPSK", "rx" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hz", "16|32", "rx" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "saddr", "", "rx" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "len", "KB", "rx" },
	{ PARAM_FLG | PARAM_SUB     | PARAM_OPTIONAL, "snr", "estimate snr level & best tm para", "rx" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "zpoff", "", "rx" },

	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "gain",  "agc", "cap" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "hs",    "",    "cap" },

    { PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "tx|rx|cap", "stop" },

    { PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "sfo|pma|trace|perf|notch|cap|me|band|statis|tgain|msg|mem|sortslot|rxtx|hplc|tonemap", "show" },

	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "",    "pma|statis|msg|mem|rxtx|hplc", "clear" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,		"status",	"", "msg" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,		"addr",	"", "mem" },
	
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "",    "phase|scmask|sfo|gain|notch|perf|band|tgain|msg|mem|test|sortslot|rfchannel", "set" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "",    "a|b|c|all", "phase" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "start", "", "scmask" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "end",   "", "scmask" },
	{ PARAM_ARG | PARAM_NUMBER  | PARAM_SUB | PARAM_OPTIONAL, "ppm", "", "sfo" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "db", "", "gain" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,		  "freq",  "HZ",	 "notch" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "iir",   "index",	 "notch" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,		  "",	   "A|B|C|D",	 "perf" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,                  "",		"0|1|2|3|4|5|6|7",		"band" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "start",	"",			"band" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "end",	"",			"band" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "hs",		"Header Symbol Number", "band" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hm",		"BPSK|QPSK",		"band" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "hz",		"16|32",		"band" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,				 "dig", 	"", "tgain" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB , 				 "ana", 	"", "tgain" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,				 "opt", 	"", "rfchannel" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB , 				 "rfc", 	"", "rfchannel" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,				 "type", 	"0|1|2", "msg" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,				 "len", 	"", "msg" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,				 "size",	"", "mem" },
	{ PARAM_ARG | PARAM_STRING | PARAM_SUB ,				 "name",	"", "mem" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB ,				 "mode",	"", "test" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "",    "ana|bb",       "lowpower" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "",	"0|1|2",		"lowpower" },

	PARAM_EMPTY
};

CMD_DESC_ENTRY(cmd_phy_demo_desc, "", cmd_phy_demo_param_tab);
extern void phy_trace_show(tty_t *term);
extern void phy_statis_show(xsh_t *xsh);
extern void phy_statis_clear(xsh_t *xsh);
void docmd_phy_demo(command_t *cmd, xsh_t *xsh)
{
	if (__strcmp(xsh->argv[1], "start") == 0) {
		if (__strcmp(xsh->argv[2], "tx") == 0)
			docmd_phy_tx_start(xsh);
		else if (__strcmp(xsh->argv[2], "rx") == 0)
			docmd_phy_rx_start(xsh);
		else if (__strcmp(xsh->argv[2], "cap") == 0)
			docmd_phy_cap_start(xsh);
	} else if (__strcmp(xsh->argv[1], "stop") == 0) {
		if (__strcmp(xsh->argv[2], "tx") == 0)
			docmd_phy_tx_stop(xsh);
		else if (__strcmp(xsh->argv[2], "rx") == 0)
			docmd_phy_rx_stop(xsh);
		else if (__strcmp(xsh->argv[2], "cap") == 0)
			docmd_phy_cap_stop(xsh);
	} else if (__strcmp(xsh->argv[1], "show") == 0) {
		if (__strcmp(xsh->argv[2], "pma") == 0) {
			docmd_phy_pma_show(xsh);
		} else if (__strcmp(xsh->argv[2], "trace") == 0) {
			phy_trace_show(xsh->term);
		} else if (__strcmp(xsh->argv[2], "sfo") == 0) {
			phy_sfo_show_demo(xsh);
		} else if (__strcmp(xsh->argv[2], "perf") == 0) {
			docmd_phy_perf_show(xsh);	
		} else if (__strcmp(xsh->argv[2], "notch") == 0) {
			docmd_phy_notch_show(xsh);
		} else if (__strcmp(xsh->argv[2], "cap") == 0) {
			docmd_phy_cap_show(xsh);
        } else if (__strcmp(xsh->argv[2], "me") == 0) {
            docmd_phy_me_show(xsh);
        }else if (__strcmp(xsh->argv[2], "band") == 0) {
			docmd_phy_band_show(xsh);
		} else if (__strcmp(xsh->argv[2], "statis") == 0) {
            phy_statis_show(xsh);
		} else if (__strcmp(xsh->argv[2], "tgain") == 0) {
			docmd_phy_tgain_show(xsh);
		} else if (__strcmp(xsh->argv[2], "msg") == 0) {
			phy_msg_show(xsh,&TX_MNG_LINK);
			phy_msg_show(xsh,&TX_DATA_LINK);
		} else if (__strcmp(xsh->argv[2], "mem") == 0) {
			extern	void show_record_mem_list(xsh_t *xsh);
			show_record_mem_list(xsh);
		}else if(__strcmp(xsh->argv[2], "sortslot") == 0){

			extern list_head_t BEACON_LIST;
			beacon_t *beacon;	
				
			if (!list_empty(&BEACON_LIST)) {		
					beacon = list_entry(BEACON_LIST.next, beacon_t, link);		
                    phy_beacon_show(&beacon->bpts_list, xsh->term);
				}
		} else if (__strcmp(xsh->argv[2], "rxtx") == 0) {
			docmd_phy_rxtx_show(xsh);
		} else if (__strcmp(xsh->argv[2], "hplc") == 0) {
			docmd_phy_hplc_statis_show(xsh);
        } else if (__strcmp(xsh->argv[2], "tonemap") == 0) {
            phy_tonemap_show(xsh);
        }
	} else if (__strcmp(xsh->argv[1], "clear") == 0) {
		if (__strcmp(xsh->argv[2], "pma") == 0)
			docmd_phy_pma_clear(xsh);
		else if (__strcmp(xsh->argv[2], "statis") == 0)
            phy_statis_clear(xsh);
		else if(__strcmp(xsh->argv[2], "msg") == 0){

		}else if(__strcmp(xsh->argv[2], "mem") == 0){

			uint32_t addr;
			
			addr = __strtoul(xsh->argv[3], 0, 0);

			printf_s("p : 0x%08x , result : %d\n",addr,zc_free_mem((void *)addr));
		}else if(__strcmp(xsh->argv[2], "rxtx") == 0){
            docmd_phy_rxtx_clear(xsh);
		} else if(__strcmp(xsh->argv[2], "hplc") == 0){
            docmd_phy_hplc_statis_clear(xsh);
		}
		
	} else if (__strcmp(xsh->argv[1], "set") == 0) {
		if (__strcmp(xsh->argv[2], "phase") == 0) {
			if (__strcmp(xsh->argv[3], "a") == 0) {
				PHY_DEMO.phase = 1 << PHASE_A;
			} else if (__strcmp(xsh->argv[3], "b") == 0) {
				PHY_DEMO.phase = 1 << PHASE_B;
			} else if (__strcmp(xsh->argv[3], "c") == 0) {
				PHY_DEMO.phase = 1 << PHASE_C;
			} else if (__strcmp(xsh->argv[3], "all") == 0) {
				PHY_DEMO.phase = PHY_PHASE_ALL;
			}
		} else if (__strcmp(xsh->argv[2], "scmask") == 0) {
			docmd_phy_scmask_set_demo(xsh);
		} else if (__strcmp(xsh->argv[2], "sfo") == 0) {
            phy_set_sfo(SFO_DIR_TX | SFO_DIR_RX, xsh->argc > 3 ? __strtod(xsh->argv[3], NULL)*SFO_CONST_FACTOR : 0, HSFO.adjust);
		} else if (__strcmp(xsh->argv[2], "gain") == 0) {
			docmd_phy_rgain_set_demo(xsh);
		} else if (__strcmp(xsh->argv[2], "notch") == 0) {
			docmd_phy_notch_set_demo(xsh);
		} else if (__strcmp(xsh->argv[2], "perf") == 0) {
			docmd_phy_perf_set(xsh);
		} else if (__strcmp(xsh->argv[2], "band") == 0) {
			docmd_phy_band_set(xsh);
		}else if (__strcmp(xsh->argv[2], "tgain") == 0) {
			docmd_phy_tgain_set(xsh);
		}else if (__strcmp(xsh->argv[2], "msg") == 0) {
			docmd_phy_msg_set(xsh);
		}else if (__strcmp(xsh->argv[2], "mem") == 0) {
			docmd_phy_mem_set(xsh);
		}else if (__strcmp(xsh->argv[2], "test") == 0) {
			docmd_phy_test_set(xsh);
		}else if (__strcmp(xsh->argv[2], "sortslot") == 0)
		{
			docmd_sort_slot(xsh);
		}
		else if(__strcmp(xsh->argv[2], "rfchannel") == 0)
		{
			docmd_phy_rfchannel_set(xsh);
		}
	} else if (__strcmp(xsh->argv[1], "reset") == 0) {
		while (phy_reset() != OK)
			sys_delayms(1);
	}else if (__strcmp(xsh->argv[1], "init") == 0) {
		phy_init();
	}
    else if (__strcmp(xsh->argv[1], "lowpower") == 0) {
		docmd_phy_lowpower_set(xsh);
	}
}


void docmd_phy_pma_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;
	uint32_t val;

	phy_state_get(PHY_TX_HDR_CNT, &val);
	term->op->tprintf(term, "TX HDR CNT  : %-10u\n", val);

	phy_state_get(PHY_TX_PLD_CNT, &val);
	term->op->tprintf(term, "TX PLD CNT  : %-10u\n", val);

	phy_state_get(PHY_TX_PB136_CNT, &val);
	term->op->tprintf(term, "TX PB136 CNT: %-10u\n", val);

	phy_state_get(PHY_TX_PB520_CNT, &val);
	term->op->tprintf(term, "TX PB520 CNT: %-10u\n", val);

	phy_state_get(PHY_RX_HDR_CNT, &val);
	term->op->tprintf(term, "RX HDR CNT  : %-10u\n", val);

	phy_state_get(PHY_RX_PLD_CNT, &val);
	term->op->tprintf(term, "RX PLD CNT  : %-10u\n", val);

	phy_state_get(PHY_RX_PB136_CNT, &val);
	term->op->tprintf(term, "RX PB136 CNT: %-10u\n", val);

	phy_state_get(PHY_RX_PB520_CNT, &val);
	term->op->tprintf(term, "RX PB520 CNT: %-10u\n", val);

	phy_state_get(PHY_RX_HDR_ERR, &val);
	term->op->tprintf(term, "RX HDR ERR  : %-10u\n", val);

	phy_state_get(PHY_RX_PB16_ERR, &val);
	term->op->tprintf(term, "RX PB16 ERR : %-10u\n", val);

	phy_state_get(PHY_RX_PB32_ERR, &val);
	term->op->tprintf(term, "RX PB32 ERR : %-10u\n", val);

	phy_state_get(PHY_RX_PB136_ERR, &val);
	term->op->tprintf(term, "RX PB136 ERR: %-10u\n", val);

	phy_state_get(PHY_RX_PB520_ERR, &val);
	term->op->tprintf(term, "RX PB520 ERR: %-10u\n", val);
	return;
}

void docmd_phy_pma_clear(xsh_t *xsh)
{
	phy_state_get(PHY_CNT_CLEAR, NULL);
}

void docmd_execute_fail(tty_t *term)
{
	term->op->tputs(term, "Execute cmd fail: ");
}

U8 mac_buff[2100];
U16 mac_len=0;
U8 mac_slice = 0;
/*phy start tx sof|beacon|sack
*phy start tx sof num<int> [-pbsz=72|136|264|520] [-pbc=<int>] 
*[-bat=<int>] [-tmi=<int>] [-dtei=<int>] [-bp=<int>] [-hs=<int>] 
*[-hm=BPSK|QPSK] [-hz=16|32] [-crc=ok|err] [-mac=l|s] [-byte=<int>] 
*[-len=<int>] [-stime=<int>]
*num<int>                 :长mpdu时为发送次数，短mpdu时为发送帧数
*[-pbsz=72|136|264|520]   :pb块大小
*[-pbc=<int>]             :pb块数量
*[-bp=<int>]              :发送间隔（单位：us)
*[-crc=ok|err]            :是否需要msdu crc32计算ok:正确,err:错误(默认计算crc)
*[-mac=l|s]               :l:mac帧长帧头,s:mac帧短帧头(默认长帧头)
*[-byte=<int>]            :输入byte,msdu载荷全为byte(默认写0)
*[-len=<int>]             :msdu 长度(默认20)
*/
void docmd_phy_tx_start(xsh_t *xsh)
{
	tonemap_t *tm;
	tty_t *term = xsh->term;
	frame_ctrl_t *head;
	phy_pld_info_t *pi;
	phy_frame_info_t *pf;
	bph_t *bph;
	pbh_t *pbh;
	int32_t ret;
	uint32_t mpdu, stime = 0, cpu_sr;
	uint16_t dtei, i,msdulen;
	uint8_t dt, pbsz, pbc, tmi, crc, macuse,byte;
	char *s;
	evt_notifier_t ntf;

	phy_demo_stop(xsh);

	mpdu = __strtoul(xsh->argv[4], NULL, 0);
	if (!mpdu) return;

	pf = &PHY_DEMO.pf;
	pi = &PHY_DEMO.pf.pi;

	tmi  = 0xFF;
	pbsz = 0xFF;
	pbc  = 0;
	crc  = 1;
	macuse = 1;
	msdulen = 20;
	byte = 0;
	dtei           = 0;
	PHY_DEMO.delay = 0;
	PHY_DEMO.bp    = 0;

	pf->hi.symn    = CURR_B->symn;
	pf->hi.mm      = CURR_B->mm;
	pf->hi.size    = CURR_B->size;

	pi->bat        = NULL;
	for (i = 5; i < xsh->argc; ++i) {
		if ((s = xsh_getopt_val(xsh->argv[i], "-pbsz="))) {
#if defined(STD_2016) || defined(STD_DUAL)
			if (__strcmp(s, "72") == 0)	pbsz = PBSZ_72;
			else if (__strcmp(s, "136") == 0) pbsz = PBSZ_136;
			else if (__strcmp(s, "264") == 0) pbsz = PBSZ_264;
			else if (__strcmp(s, "520") == 0) pbsz = PBSZ_520;
#else
			if (__strcmp(s, "136") == 0) pbsz = PBSZ_136;
			else if (__strcmp(s, "520") == 0) pbsz = PBSZ_520;
#endif
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-pbc="))) {
			pbc = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-tmi="))) {
			tmi = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-dtei="))) {
			dtei = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-bp="))) {
			PHY_DEMO.bp = __strtoul(s, NULL, 0);
//			PHY_DEMO.bp = MAX(PHY_DEMO.bp, BEACON_PERIOD_MIN) * (PHY_TICK_FREQ / 1000);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hs="))) {
			if ((pf->hi.symn = __strtoul(s, NULL, 0)) < 1)
				pf->hi.symn = 1;
			else if (pf->hi.symn > NR_HDR_SYM_MAX)
				pf->hi.symn = NR_HDR_SYM_MAX;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hm="))) {
			pf->hi.mm = __strcmp(s, "BPSK") == 0 ? MM_BPSK : MM_QPSK;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hz="))) {
			pf->hi.size = __strcmp(s, "32") == 0 ? HDR_SIZE_32 : HDR_SIZE_16;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-crc="))) {
			crc = __strcmp(s, "ok") == 0 ? 1 : 0;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-stime="))) {
			stime = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-snid="))) {
			PHY_DEMO.snid = __strtoul(s, NULL, 0);
		}else if ((s = xsh_getopt_val(xsh->argv[i], "-mac="))) {
			macuse = __strcmp(s, "l") == 0 ? 1 : 0;
		}else if ((s = xsh_getopt_val(xsh->argv[i], "-byte="))) {
			byte = __strtoul(s, NULL, 0);
			byte = DEC2BCD(byte);
		}else if ((s = xsh_getopt_val(xsh->argv[i], "-len="))) {
			msdulen = __strtoul(s, NULL, 0);
		} 
		
	}

	if (tmi == 0xFF) {
		if (pbsz == 0xFF) {
			term->op->tputs(term, "either tmi or pbsz must be specified\n");
			return;
		}
#if defined(STD_2016) || defined(STD_DUAL)
		tmi = (pbsz == PBSZ_72) ? TMI_ROBO_72_14 :
			(pbsz == PBSZ_136) ? TMI_ROBO_136_2 :
			(pbsz == PBSZ_264) ? TMI_ROBO_264_12 : TMI_ROBO_520_1;
#else
		tmi = (pbsz == PBSZ_136) ? TMI_ROBO_136_2 : TMI_ROBO_520_1;
#endif
	}

	if (__strcmp(xsh->argv[3], "sof") == 0) {
		dt = DT_SOF;
	} else if (__strcmp(xsh->argv[3], "beacon") == 0) {
		dt = DT_BEACON;
        PHY_DEMO.bp = MAX(PHY_DEMO.bp, BEACON_PERIOD_MIN);
		if (pbc > 1) {
			term->op->tprintf(term, "Beacon frame's payload has 1 PB at most!\n");
			return;
		}
		if (tmi >= 16) {
			term->op->tprintf(term, "Beacon frame's payload must use non-extend tonemaps!");
			return;
		}
	} else if (__strcmp(xsh->argv[3], "sack") == 0) {
		dt = DT_SACK;
		if (pbc > 0) {
			term->op->tprintf(term, "SACK frame cann't have payload!\n");
			return;
		}
	} else if (__strcmp(xsh->argv[3], "silent") == 0) {
		dt = DT_SILENT;
	} else {
		phy_debug_printf("delimiter type not supported yet!");
		return;
	}
    phy_debug_printf("PHY_DEMO.bp:%d us!", PHY_DEMO.bp);
    PHY_DEMO.bp *= PHY_US2TICK(1);

    phy_debug_printf("PHY_DEMO.bp:%d tick!\n", PHY_DEMO.bp);


	if (!(tm = get_sys_tonemap(tmi))) {
		term->op->tprintf(term, "Tone map %u doesn't exist!\n", tmi);
		return;
	}
	pi->pbsz     = tm->pbsz;
	pi->fec_rate = tm->fec_rate;
	pi->copy     = tm->copy;
	pi->pbc      = pbc;
	pi->mm       = tm->mm;
	pi->robo     = tm->robo;
	pi->tmi      = tm->tmi;

	if (pbc && (pbc < tm->pbc_min || pbc > tm->pbc_max)) {
		term->op->tprintf(term, "Tone map %u's PB count can not be %u([%u, %u])!\n", 
			tmi, pbc, tm->pbc_min, tm->pbc_max);
		return;
	}

	if (tm->mm) {
		pi->symn = robo_pb_to_pld_sym(pbc, tm->robo);
	} else {
		pi->symn = rtbat_pb_to_pld_sym(pbc, tm->robo);
		pi->bat  = tm->bat;
	}

	pf->length  = pbc * pb_bufsize(pi->pbsz);
	pf->head    = PHY_DEMO.hdr;
	pf->payload = (pf->length) ? PHY_DEMO.pld : NULL;

	if (pf->payload) {
		if (dt == DT_BEACON) {
			bph        = (bph_t *)pf->payload;
            pi->crc32_en  = 1;
			pi->crc32_ofs = 7;
			fill_pb_desc(bph, 1, 1, pi->pbsz, 1, 0);
		} else {
			U16 MsduLen;
			U32 Crc32Datatest = 0;
			U8  MacLongLen = 0;
			MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)mac_buff;
			mac_slice = 0;
			memset(mac_buff, byte, sizeof(mac_buff));
			pMacHeader->MacProtocolVersion = e_MAC_VER_STANDARD;
			pMacHeader->MacAddrUseFlag = macuse;
			MsduLen = msdulen;
			pMacHeader->MsduLength_l = MsduLen;
			pMacHeader->MsduLength_h = MsduLen >> 8;

			MacLongLen = macuse ? 12 : 0;
			if(crc)
			{
				crc_digest(pMacHeader->Msdu + MacLongLen, MsduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
   				__memcpy(&pMacHeader->Msdu[MacLongLen+MsduLen], (U8 *)&Crc32Datatest, 4);
			}
			mac_len = sizeof(MAC_PUBLIC_HEADER_t) + MacLongLen + MsduLen + 4;
			if(mac_len > pbc*body_size[pi->pbsz])
			{
				mac_slice = 1;
				mpdu = mac_len / (pbc*body_size[pi->pbsz]);
				if(mac_len % (pbc*body_size[pi->pbsz]))
					mpdu ++;
			}
			pbh = (pbh_t *)pf->payload;
			for (i = 0; i < pbc; ++i) {
                fill_pb_desc(pbh, i == 0, i == pbc - 1, pi->pbsz, 0, 0);
				pbh->ssn  = i;
				pbh->mfsf = i == 0;
				if(mac_slice)
					pbh->mfef = 0;
				else
					pbh->mfef = i == pbc - 1;

				__memcpy((unsigned char *)pbh + sizeof(pbh_t),mac_buff + i * (body_size[pi->pbsz]),mac_len>body_size[pi->pbsz]?body_size[pi->pbsz]:mac_len);
				if(mac_len>body_size[pi->pbsz])
           			mac_len -= body_size[pi->pbsz];
				pbh = (void *)pbh + pb_bufsize(pi->pbsz);
			}
		}
	}

	frame_construct(pf->head, dt, pbc, dtei, pi->symn, pf->hi.symn, tm);
	dhit_write_back((uint32_t)pf->payload, pf->length);

	PHY_DEMO.mpdu     = mpdu;
	PHY_DEMO.state    = PHY_DEMO_TX;
	PHY_DEMO.substate = 1;

	head = (frame_ctrl_t *)pf->head;
	if (head->dt == DT_BEACON) {
		//bc = (beacon_ctrl_t *)head;
		pf->hi.pts = TRUE;	// if pts is true, then stime is decided by phy layer, 
					// and stime will be recorded into frame ctrl at the 
					// offset of 'pts_off'.
		pf->hi.pts_off = offset_of(beacon_ctrl_t, bts);
		pi->crc32_en  = 1;
        pi->crc32_ofs = 7;
	} else {
        pf->hi.pts  = FALSE;
        pi->crc32_en  = 0;
        pi->crc32_ofs = 0;
	}

	cpu_sr = OS_ENTER_CRITICAL();

	ld_set_tx_phase(PHY_DEMO.phase);

	fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_demo_tx_evt, NULL);
	if (stime) stime += get_phy_tick_time();

	if ((ret = phy_start_tx(&PHY_DEMO.pf, (stime) ? &stime : NULL, &ntf, 1)) != OK)
		term->op->tprintf(term, "start tx frame failed, ret = %d\n", ret);

	OS_EXIT_CRITICAL(cpu_sr);
}


int32_t frame_construct(frame_ctrl_t *head, uint8_t dt, uint8_t pbc, uint16_t dtei, uint16_t symn, 
			uint16_t hdr_symn, tonemap_t *tm)
{
	beacon_ctrl_t *beacon;
	sof_ctrl_t *sof;
	sack_ctrl_t *sack;

	memset(head, 0, sizeof(frame_ctrl_t));

	head->dt     = dt;
#if defined(STD_2016) || defined(STD_DUAL)
	head->access = 0;
#else
	head->access = 1;
#endif

	head->snid   = PHY_DEMO.snid;

	switch (dt) {
	case DT_BEACON:
		beacon = (beacon_ctrl_t *)head;
		beacon->stei  = PHY_DEMO.stei;
		beacon->tmi   = tm->tmi;
		beacon->symn  = symn;
		beacon->phase = PHASE_A;
		break;
	case DT_SOF:
		sof       = (sof_ctrl_t *)head;
		sof->stei = PHY_DEMO.stei;
		sof->dtei = dtei;
		sof->symn = symn;
		set_sof_tmi(sof, tm->tmi);
		set_sof_fl(sof, pld_sym_to_time(symn) + PHY_BIFS);
		sof->pbc   = pbc;
#	if defined(STD_GD) || defined(STD_SPG)
		sof->tf    = 1;
#	else
		sof->bcsti = (dtei == BCAST_TEI) ? 1 : 0;
#	endif
		break;
	case DT_SACK:
		sack       = (sack_ctrl_t *)head;
		sack->dtei = dtei;
		sack->pbc  = pbc;
		break;
	default:
		return ERROR;
	}

	return OK;
}

void docmd_phy_tx_stop(xsh_t *xsh)
{
	if (PHY_DEMO.state != PHY_DEMO_TX)
		return;

	PHY_DEMO.state = PHY_DEMO_STOP;
	if (PHY_DEMO.payload != NULL) {
		free(PHY_DEMO.payload);
		PHY_DEMO.payload = NULL;
	}
}


__isr__ void phy_demo_tx_evt(phy_evt_param_t *para, void *arg)
{
	frame_ctrl_t *head;
	int32_t ret;
	uint32_t mpdu, time, curr = 0, dur;
	evt_notifier_t ntf;

	mpdu = PHY_DEMO.mpdu;
	head = (frame_ctrl_t *)para->fi->head;

	if (--PHY_DEMO.mpdu > 0) {
		dur = PREAMBLE_DUR + FRAME_CTRL_DUR(para->fi->hi.symn) + pld_sym_to_time(para->fi->pi.symn);
		time = para->stime + dur + PHY_DEMO.delay + PHY_BIFS;

//		if (head->dt == DT_BEACON) {
        if (1) {
			if (PHY_DEMO.bp) {
//				if (time_after(para->stime + PHY_DEMO.bp, time)) {
//					time = para->stime + PHY_DEMO.bp;
//				}
                time = time + PHY_DEMO.bp;
			}
		}
		if(mac_slice)
		{
			uint8_t i,offset;
			phy_frame_info_t *pf;
			phy_pld_info_t *pi;
			pbh_t *pbh;
			pf = &PHY_DEMO.pf;
			pi = &PHY_DEMO.pf.pi;

			//__memmove(mac_buff, mac_buff + body_size[pi->pbsz], mac_len);
			pbh = (pbh_t *)pf->payload;
			offset = pbh->ssn+pi->pbc;//需加pb块数，因为pbh->ssn取的是第一个pb块的物理块头信息
			for (i = offset; i < pi->pbc+offset; ++i) {
				fill_pb_desc(pbh, 1, 1, pi->pbsz, 0, 0);
				pbh->ssn  = i;
				pbh->mfsf = 0;
				if(PHY_DEMO.mpdu<=1 && i==(pi->pbc+offset-1))
					pbh->mfef = 1;
				else
					pbh->mfef = 0;

				__memcpy((unsigned char *)pbh + sizeof(pbh_t),mac_buff + i * (body_size[pi->pbsz]),mac_len>body_size[pi->pbsz]?body_size[pi->pbsz]:mac_len);
				if(mac_len>body_size[pi->pbsz])
	       			mac_len -= body_size[pi->pbsz];
				pbh = (void *)pbh + pb_bufsize(pi->pbsz);
			}
			dhit_write_back((uint32_t)pf->payload, pf->length);
		}
		ld_set_tx_phase(PHY_DEMO.phase);

		fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_demo_tx_evt, NULL);

		if ((ret = phy_start_tx(&PHY_DEMO.pf, &time, &ntf, 1)) != OK) {
			phy_debug_printf("start tx frame failed, ret = %d\n", ret);
			return;
		}

		curr = get_phy_tick_time();
	}
	if ((mpdu & 0xf) == 1) {
		phy_debug_printf("------------------------------------------------------------------------\n");
		phy_debug_printf("idx         frame     hs   symn pbc tmi fl       len   next     curr\n");
		phy_debug_printf("------------------------------------------------------------------------\n");
	}

	phy_debug_printf("%-8d tx %-9s %-4d %-4d %-3d %-3d %-8d %-5d %08x %08x\n",
			 mpdu, frame2str(head->dt), para->fi->hi.symn, get_frame_symn(head), get_frame_pbc(head),
			 get_frame_tmi(head), get_frame_dur(para->fi), para->fi->length,
			 PHY_DEMO.mpdu > 0 ? time : 0, curr);

	if (!PHY_DEMO.mpdu)
	{
		mac_slice = 0;
		phy_debug_printf("tx frame done.\n");
	}

	return;
}


void docmd_phy_rx_start(xsh_t *xsh)
{
	uint32_t cpu_sr;
	uint32_t i, timeout;
	tty_t *term = xsh->term;
	char *s;
	evt_notifier_t ntf[3];

	phy_demo_stop(xsh);
	register_phy_hdr_isr(isr_hdr_demo_rx);
    register_clock_sync_hook(demo_clock_sync, NULL);
	

	PHY_DEMO.snr_esti = FALSE;
	PHY_DEMO.snr_level = 0;
	PHY_DEMO.snr_ave   = 0;
	PHY_DEMO.mm = 0;
	PHY_DEMO.fecr = 0;
	PHY_DEMO.copy = 0;
	PHY_DEMO.mpdu = 0;
	PHY_DEMO.state = PHY_DEMO_RX;
	PHY_DEMO.substate = 0;
	PHY_DEMO.rx.symn = CURR_B->symn;
	PHY_DEMO.rx.mm   = CURR_B->mm;
	PHY_DEMO.rx.size = CURR_B->size;
	PHY_DEMO.pi.bat  = NULL;
	for (i = 3; i < xsh->argc; ++i) {
		if ((s = xsh_getopt_val(xsh->argv[i], "-pbsz="))) {
			//pbsz = __strcmp(s, "136") == 0 ? PBSZ_136 : PBSZ_520;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-dur="))) {
			PHY_DEMO.dur = __strtoul(s, NULL, 0);
			PHY_DEMO.dur = PHY_US2TICK(PHY_DEMO.dur * 1000);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hs="))) {
			PHY_DEMO.rx.symn = __strtoul(s, NULL, 0);
			if (PHY_DEMO.rx.symn < 1)
				PHY_DEMO.rx.symn = 1;
			else if (PHY_DEMO.rx.symn > NR_HDR_SYM_MAX)
				PHY_DEMO.rx.symn = NR_HDR_SYM_MAX;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hm="))) {
			PHY_DEMO.rx.mm = __strcmp(s, "BPSK") == 0 ? MM_BPSK : MM_QPSK;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hz="))) {
			PHY_DEMO.rx.size = __strcmp(s, "32") == 0 ? HDR_SIZE_32 : HDR_SIZE_16;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-bat="))) {
		} else if (__strcmp(xsh->argv[i], "-snr") == 0) {
			PHY_DEMO.snr_esti = TRUE;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-zpoff="))) {
		}
	}

	if (PHY_DEMO.pi.bat) {
		term->op->tputs(term, "Not implemented\n");
		return;
	}

	fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, phy_demo_rx_evt, NULL);
	fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR,  phy_demo_rx_evt, NULL);
	fill_evt_notifier(&ntf[2], PHY_EVT_RX_END,  phy_demo_rx_evt, NULL);

	cpu_sr = OS_ENTER_CRITICAL();
	timeout = get_phy_tick_time() + PHY_DEMO.dur;

	ld_set_rx_phase(PHY_DEMO.phase);
	if (phy_start_rx(&PHY_DEMO.rx, NULL, PHY_DEMO.dur > 0 ? &timeout : NULL, ntf, NELEMS(ntf)) != OK)
		term->op->tprintf(term, "phy start rx failed\n");
	OS_EXIT_CRITICAL(cpu_sr);
}


void docmd_phy_rx_stop(xsh_t *xsh)
{
	if (PHY_DEMO.state != PHY_DEMO_RX)
		return;

	if (phy_reset() != OK) {
		xsh->term->op->tputs(xsh->term, "stop rx failed, please try again later!\n");
		return;
	}

	PHY_DEMO.state = PHY_DEMO_STOP;
	PHY_DEMO.rx_gain = 0;
}

__isr__ void reset_notch_filter(void)
{
	phy_notch_freq_t notch;

	notch.freq = 0; notch.idx = 0;
	phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);
	notch.freq = 0; notch.idx = 1;
	phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);
#if defined(MIZAR1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
	notch.freq = 0; notch.idx = 2;
	phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);
#endif
}

__isr__ void test_rx_nbni(phy_evt_param_t *para, void *arg)
{
	phy_nbni_info_t *nbni;
	phy_notch_freq_t notch;
	evt_notifier_t ntf[3];

// #if 1

#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
	phy_rgain_t rgain;
#endif
	if (!para->fail && (nbni = (phy_nbni_info_t *)para->fi->payload)) {
#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
#if defined(VENUS_V3)
		phy_ana_rx_bpf_t bpf;
		phy_ioctl(PHY_CMD_GET_ANA_RX_BPF, &bpf);
		bpf.hpf_bw = 0x6466;
#endif
		phy_ioctl(PHY_CMD_GET_RGAIN, &rgain);
		if(rgain == 36 || rgain == 37)
		{
			rgain = 36;
			phy_ioctl(PHY_CMD_SET_RGAIN, &rgain);
		}

		phy_debug_printf("freq: %d %d, sir: %f, rgain:%d, pulse:%d\n"
		, nbni->freq, para->notch_freq, nbni->sir, rgain, para->pulse);

		if (para->notch_freq == 500000) {
			notch.freq = 500000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

			notch.freq = 1500000; 
			notch.idx  = 2;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 1000000) {
			notch.freq = 1000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

			notch.freq = 3000000; 
			notch.idx  = 2;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 2000000) {
			notch.freq = 2000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 3000000) {
			notch.freq = 3000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 5000000) {
			notch.freq = 5000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 6000000) {
			notch.freq = 6000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 8000000) {
			notch.freq = 8000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		} else if (para->notch_freq == 15000000) {
			notch.freq = 15000000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

#if defined(VENUS_V3)
			phy_ioctl(PHY_CMD_SET_ANA_RX_BPF, &bpf);
#endif
			goto exit;
		}

		rgain = 255;
		phy_ioctl(PHY_CMD_SET_RGAIN, &rgain);

		if (nbni->sir >= 6)
			goto exit;

		if (para->pulse) {
			phy_debug_printf("detected pulse noise\n");
			goto exit;
		}

		notch.freq = 0; notch.idx = 1;

		if ((nbni->freq > 2000000 - 24414 * 2) && (nbni->freq < 2000000 + 24414 * 2)) {
			notch.freq = 2000000;
		} else if ((nbni->freq > 500000 - 24414 * 2) && (nbni->freq < 500000 + 24414 * 2)) {
			notch.freq = 500000;
		} else {
			notch.freq = nbni->freq;
		}
#else
		if (nbni->sir >= 0)
			goto exit;

		notch.freq = 0; notch.idx = 0;

		if ((nbni->freq > 500000 - 24414 * 2) && (nbni->freq < 500000 + 24414 * 2)) {
			notch.freq = 500000;
		} else if ((nbni->freq > 1000000 - 24414 * 2) && (nbni->freq < 1000000 + 24414 * 2)) {
			notch.freq = 1000000;
		} else if ((nbni->freq > 2000000 - 24414 * 2) && (nbni->freq < 2000000 + 24414 * 2)) {
			notch.freq = 2000000;
		} else if ((nbni->freq > 3000000 - 24414 * 2) && (nbni->freq < 3000000 + 24414 * 2)) {
			notch.freq = 3000000;
		} else if ((nbni->freq > 5000000 - 24414 * 2) && (nbni->freq < 5000000 + 24414 * 2)) {
			notch.freq = 5000000;
		} else if ((nbni->freq > 6000000 - 24414 * 2) && (nbni->freq < 6000000 + 24414 * 2)) {
			notch.freq = 6000000;
		} else if ((nbni->freq > 8000000 - 24414 * 2) && (nbni->freq < 8000000 + 24414 * 2)) {
			notch.freq = 8000000;
		/* 12.5-25MHz wave will overlap with 0-12.5MHz, if 10MHz noise detected, it will be 15MHz. */
		} else if ((nbni->freq > 10000000 - 24414 * 2) && (nbni->freq < 10000000 + 24414 * 2)) {
			notch.freq = 15000000;
		} else {
			notch.freq = nbni->freq;
		}
#endif

		if (notch.freq) {
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);
			//phy_ioctl(PHY_CMD_GET_RGAIN, &rgain);
			//phy_ioctl(PHY_CMD_SET_RGAIN, &rgain);
		}
	}

exit:
	fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, phy_demo_rx_evt, NULL);
	fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR,  phy_demo_rx_evt, NULL);
	fill_evt_notifier(&ntf[2], PHY_EVT_RX_END,  phy_demo_rx_evt, NULL);

	ld_set_rx_phase(PHY_DEMO.phase);
	phy_start_rx(&PHY_DEMO.rx, NULL, NULL, ntf, NELEMS(ntf));

	return;
}

__LOCALTEXT __isr__ void phy_demo_rx_evt(phy_evt_param_t *para, void *arg)
{
	evt_notifier_t ntf[3];
	frame_ctrl_t *head;
	uint32_t time, timeout;
	int32_t ret;

	fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, phy_demo_rx_evt, NULL);
	fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR,  phy_demo_rx_evt, NULL);
	fill_evt_notifier(&ntf[2], PHY_EVT_RX_END,  phy_demo_rx_evt, NULL);

	switch (para->event) {
	case PHY_EVT_PD_TMOT:
		if (!PHY_DEMO.substate) {
			phy_debug_printf("rx timeout at 0x%x(%.3fus)\n", para->stime, PHY_TICK2US(para->dur * 1.0));
		} else {
			time = para->stime - para->dur + SACK_FRAME_DUR(PHY_DEMO.rx.symn);
			timeout = time + PHY_DEMO.dur;

			ld_set_rx_phase(PHY_DEMO.phase);
			phy_start_rx(&PHY_DEMO.rx, &time, PHY_DEMO.dur > 0 ? &timeout : NULL, ntf, NELEMS(ntf));
			//curr = get_phy_tick_time();
			PHY_DEMO.substate = 0;
		}

		break;
	case PHY_EVT_RX_HDR:
		phy_ioctl(PHY_CMD_GET_RGAIN, &PHY_DEMO.rx_gain);
		break;

	case PHY_EVT_RX_END:
		if (para->fail)
			goto exit;

		if (PHY_DEMO.snr_esti && para->fi->payload != NULL) {
			phy_tonemap_para_t tm;

			memset(&tm, 0, sizeof(phy_tonemap_para_t));
			phy_pld_snr_esti(para->fi, &tm);
			PHY_DEMO.snr_level = tm.level;
			PHY_DEMO.mm        = tm.mm;
			PHY_DEMO.fecr      = tm.fecr;
			PHY_DEMO.copy      = tm.copy;
			PHY_DEMO.snr_ave   = tm.snr_ave;

			phy_debug_printf("snr_ave = %.3f, snr_level = %u\n", PHY_DEMO.snr_ave, PHY_DEMO.snr_level);
		}

		time = para->stime + para->dur;
		timeout = time + PHY_DEMO.dur;

		if (!header_is_valid(para->fi))
			goto exit;

		head = (frame_ctrl_t *)para->fi->head;
		
		/* noise detect */
		if (head->dt == DT_BEACON) {
#if defined(UNICORN2M) || defined(UNICORN8M) || defined(ROLAND1_1M) || defined(ROLAND9_1M) 
			phy_rgain_t rgain = 255;
			phy_perf_mode_t mode = PHY_PERF_MODE_A;
			phy_signal_cap_t cap;

			phy_ioctl(PHY_CMD_SET_PERF_MODE, &mode);
			phy_ioctl(PHY_CMD_SET_RGAIN, &rgain);

			cap.stop  = 0;
			cap.bw    = 0;
			cap.gain  = 0;
			cap.saddr = NULL;
			cap.len   = 0;

			if ((ret = phy_start_cap(&PHY_DEMO.rx, &cap, NULL, 0)) != OK) {
				phy_debug_printf("Start cap failed: %d\n", ret);
				goto exit;
			}
			
			if (is_pulse_noise()) {
				phy_debug_printf("Pulse noise detected\n");
				mode = PHY_PERF_MODE_B;
				phy_ioctl(PHY_CMD_SET_PERF_MODE, &mode);
				goto exit;
			}
#endif
			reset_notch_filter();
			if ((ret = phy_start_rx_nbni(&PHY_DEMO.rx, test_rx_nbni)) != OK) {
				phy_debug_printf("Start rx nbni failed: %d\n", ret);
				goto exit;
			}

			phy_demo_rx_frame(para->fi, para->stime, PHY_DEMO.rx_gain);
			return;
		}

exit:
		ld_set_rx_phase(PHY_DEMO.phase);
		if ((ret = phy_start_rx(&PHY_DEMO.rx, NULL, NULL, ntf, NELEMS(ntf))) < 0) {
			phy_debug_printf("phy start rx failed: %d\n", ret);
		}

		if (!para->fail)
		{
			phy_demo_rx_frame(para->fi, para->stime, PHY_DEMO.rx_gain);
			#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
			phy_debug_printf("nb freq: %d\n", para->notch_freq);
			#endif

		}
		PHY_DEMO.substate = 0;
		break;
	}
}


__isr__ void phy_demo_rx_frame(phy_frame_info_t *pf, uint32_t stime, int32_t gain)
{
	frame_ctrl_t *head = (frame_ctrl_t *)pf->head;
	void *payload = pf->payload;
	int32_t len = pf->length;
	char *crc = "n/a";
	static int32_t count = 0;

	if (!PHY_DEMO.substate) ++PHY_DEMO.mpdu;

        if ((count & 0xf) == 0) {
		phy_debug_printf("-------------------------------------------------------------------------------\n");
		phy_debug_printf("idx         frame     gain nid sym pbc tmi fl       len   stime    curr     crc\n");
		phy_debug_printf("-------------------------------------------------------------------------------\n");
		++count;
        }

	//if (unlikely(!head)) {
	if (!header_is_valid(pf)) {
		//phy_debug_printf("%-8d rx hdr error %d\n", PHY_DEMO.mpdu, gain);
		return;
	}

	++count;

	if (payload && pf->length > 0) {
		dhit_invalidate((uint32_t)payload, pf->length);

		crc = "ok";
#if 0
        uint32_t i;
        bph_t *bph;
        pbh_t *pbh;
        uint32_t *crc32;
		if (head->dt == DT_BEACON) {
			bph = (bph_t *)payload;
			if (bph->crc)
				crc = "err";
		} else {
			for (i = 0, pbh = (pbh_t *)payload; i < get_frame_pbc(head); ++i) {
				crc32 = (void *)pbh + pb_bufsize(pbh->pbsz) - 4;
				if (pbh->crc) {
					crc = "err";
					break;
				}
					
				pbh = (pbh_t *)(crc32 + 1);
			}
		}
#else
        if (pf->pi.crc24)
                crc = "err";
#endif
	}
	
	if (head->dt == DT_COORD) {
		coord_ctrl_t *coord = (coord_ctrl_t *)head;
#	if defined(STD_GD) || defined(STD_SPG) 
		phy_debug_printf("                      bmp: %x, dur: %d, bef: %d, beo: %d, bso: %d, std: %d\n",
			coord->neighbour, coord->dur * 25, coord->bef, coord->beo * 250, coord->bso * 250, coord->std);
#	elif defined(STD_2012)  
		phy_debug_printf("                      bmp: %x, dur: %d, cof: %d, bef: %d, beo: %d, bso: %d\n",
			(coord->snid_bmp_high << 6) | coord->snid_bmp_low, coord->dur * 25, coord->cof, coord->bef, coord->beo * 250, coord->bso * 250);
#	elif defined(STD_2016) || defined(STD_DUAL)
		phy_debug_printf("                      bmp: %x, dur: %d bso: %d\n",
			coord->neighbour, coord->dur * 1000, coord->bso * 1000);
#	endif
	} else {
		phy_debug_printf("%-8d rx %-9s %-4d %-3d %-3d %-3d %-3d %-8d %-5d %08x %08x %-3s\n",
				 PHY_DEMO.mpdu, frame2str(head->dt), gain, head->snid, get_frame_symn(head), get_frame_pbc(head),
				 get_frame_tmi(head), get_frame_dur(pf), len, stime, get_phy_tick_time(), crc);
        phy_rxtx_info.rx_cnt ++;
        if(pf->pi.crc24)
            phy_rxtx_info.rx_err_cnt ++;
	}
	
	return;
}

void docmd_phy_cap_stop(xsh_t *xsh)
{
	if (PHY_DEMO.state != PHY_DEMO_CAP)
		return;

	PHY_DEMO.state = PHY_DEMO_STOP;
	while (phy_reset() != OK)
		sys_delayms(1);

	return;
}


void docmd_phy_cap_start(xsh_t *xsh)
{
	char *s;
	tty_t *term = xsh->term;
	uint32_t i;
	phy_signal_cap_t *sc = &PHY_DEMO.sc;
	phy_hdr_info_t *hi;

	while (phy_reset() != OK)
		sys_delayms(1);

	hi       = &PHY_DEMO.pf.hi;
	hi->symn = CURR_B->symn;
	hi->mm   = CURR_B->mm;
	hi->size = CURR_B->size;
	hi->pts  = 0;

	memset(sc, 0, sizeof(phy_signal_cap_t));
	sc->gain = 255;
	sc->lpbk = TRUE;

	for (i = 3; i < xsh->argc; ++i) {
		if ((s = xsh_getopt_val(xsh->argv[i], "-gain="))) {
			sc->gain = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-hs="))) {
			hi->symn = __strtoul(s, NULL, 0);
			if (hi->symn < 1)
				hi->symn = 1;
			else if (hi->symn > NR_HDR_SYM_MAX)
				hi->symn = NR_HDR_SYM_MAX;
		}
	}

	ld_set_rx_phase(PHY_DEMO.phase);
	PHY_DEMO.state = PHY_DEMO_CAP;

	if (phy_start_cap(hi, sc, NULL, 0) != OK) {
		term->op->tprintf(term, "Capturing signal data failed\n");
		return;
	}

	return;
}

void docmd_phy_cap_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	phy_pmd_clock_always_on();
	dump_mem(term, 0, 0xd3100000, 0x8000, 1);
	phy_pmd_clock_auto_switch();
	term->op->tputs(term, "\n");

	return;
}

void phy_demo_stop(xsh_t *xsh)
{
	switch (PHY_DEMO.state) {
	case PHY_DEMO_TX:
		docmd_phy_tx_stop(xsh);
		break;
	case PHY_DEMO_RX:
		docmd_phy_rx_stop(xsh);
		break;
	case PHY_DEMO_CAP:
		docmd_phy_cap_stop(xsh);
		break;
	default:
		break;
	}
}


void docmd_phy_perf_set(xsh_t *xsh)
{
	tty_t *term   = xsh->term;
	int32_t mode;

	if (__strcmp(xsh->argv[3], "A") == 0)
		mode = PHY_PERF_MODE_A;
	else if (__strcmp(xsh->argv[3], "B") == 0)
		mode = PHY_PERF_MODE_B;
	else if (__strcmp(xsh->argv[3], "C") == 0)
		mode = PHY_PERF_MODE_C;
	else if (__strcmp(xsh->argv[3], "D") == 0)
		mode = PHY_PERF_MODE_D;
	
	if (phy_ioctl(PHY_CMD_SET_PERF_MODE, &mode) != OK) {
		term->op->tprintf(term, "phy ioctl set perf mode %s faild\n", xsh->argv[3]);
		return;
	}
}
extern U32  getHplcTestMode();

void docmd_phy_band_set(xsh_t *xsh)
{
	phy_band_info_t band;
	int32_t idx, i;
	char *s;
    tty_t *term   = xsh->term;

	if (__strcmp(xsh->argv[2], "band") == 0) {
		idx = __strtoul(xsh->argv[3], NULL, 0);			
		phy_get_band_info(idx, &band);
        HPLC.band_idx = idx;

		for (i = 4; i < xsh->argc; ++i) {
			if ((s = xsh_getopt_val(xsh->argv[i], "-start="))) {
				band.start = __strtoul(s, NULL, 0);
			} else if ((s = xsh_getopt_val(xsh->argv[i], "-end="))) {
				band.end  = __strtoul(s, NULL, 0);
			} else if ((s = xsh_getopt_val(xsh->argv[i], "-hs="))) {
				band.symn = __strtoul(s, NULL, 0);
			} else if ((s = xsh_getopt_val(xsh->argv[i], "-hm="))) {
				band.mm = __strcmp(s, "BPSK") == 0 ? MM_BPSK : MM_QPSK;
			} else if ((s = xsh_getopt_val(xsh->argv[i], "-hz="))) {
				band.size = __strcmp(s, "32") == 0 ? HDR_SIZE_32 : HDR_SIZE_16;
			}
		}
        if(getHplcTestMode() != NORM && phy_get_status() == PHY_STATUS_IDLE)
        {
            term->op->tprintf(term, "phy_get_status = %d\n",phy_get_status());
    		phy_set_band_info(idx, &band);
    		phy_band_config(idx);
        }
	} else if (__strcmp(xsh->argv[2], "scmask") == 0) {
		idx = __strtoul(xsh->argv[3], NULL, 0);			
		phy_scmask_config(idx);	
	}

	return;
}

void docmd_phy_rfchannel_set(xsh_t *xsh)
{
	tty_t *term   = xsh->term;

	U8 opt;
	U32 rfchannel;
	
	if (__strcmp(xsh->argv[2], "rfchannel") == 0) {
		
		opt	= __strtoul(xsh->argv[3], NULL, 0);
		rfchannel	= __strtoul(xsh->argv[4], NULL, 0);
		HPLC.option = opt;
		HPLC.rfchannel = rfchannel;
		wphy_set_option(HPLC.option);
		wphy_set_channel(HPLC.rfchannel);
		term->op->tprintf(term, "option %d , rfchannel %d \n",HPLC.option,HPLC.rfchannel);
		return;

	}
	return;
}


void docmd_phy_tgain_set(xsh_t *xsh)
{
	tty_t *term   = xsh->term;

	phy_tgain_t tgain;
	
	if (__strcmp(xsh->argv[2], "tgain") == 0) {
		
		tgain.dig   = __strtoul(xsh->argv[3], NULL, 0);
		tgain.ana     = __strtoul(xsh->argv[4], NULL, 0);

		if (phy_ioctl(PHY_CMD_SET_TGAIN, &tgain) != OK) {
			
		term->op->tprintf(term, "phy ioctl get scmask failed\n");
		return;
		
		}
	}
	term->op->tprintf(term, "tgain dig %d , ana %d \n",tgain.dig,tgain.ana);


	return;
}


static void docmd_phy_msg_set(xsh_t *xsh)
{
//	//tty_t *term   = xsh->term;
//	uint8_t *payload;
//	mbuf_hdr_t *p;
//	uint8_t type ;
//	uint16_t len,i;
//	uint8_t addr[6];
	
//	if (__strcmp(xsh->argv[2], "msg") == 0) {
//		memset(addr,0xAB,6);
//		type = __strtoul(xsh->argv[3], NULL, 0);
//		len = __strtoul(xsh->argv[4], NULL, 0);

//		if(type == 0)
//		{
//			payload=zc_malloc_mem(len,"SOFT",MEM_TIME_OUT);

//			for(i=0;i<len;i++)
//			{
//				payload[i]=i;
//			}

//            p =  packet_sof(1,0xfff,payload,len,1,0,0,NULL);
//			zc_free_mem(payload);
//			tx_link_enq_by_lid(p,len&0x1?(&TX_MNG_LINK):(&TX_DATA_LINK));
//		}
//		else if(type == 1)
//		{
//			p = packet_sack_ctl(1,addr,0xffd,HPLC.snid);
			
//			tx_link_enq_by_lid(p,&TX_MNG_LINK);
//		}
//		else if(type == 2)
//		{
//			p = packet_sack_ctl(2,addr,0xffd,HPLC.snid);
			
//			tx_link_enq_by_lid(p,&TX_DATA_LINK);
//		}
//	}


	return;
}
extern int convert_buf(uint8_t *buf, uint8_t *cmd, int len);

static void docmd_phy_mem_set(xsh_t *xsh)
{
//	tty_t *term   = xsh->term;

//	uint16_t size;
	
	
//	if (__strcmp(xsh->argv[2], "mem") == 0) {
		
//		size = __strtoul(xsh->argv[3], NULL, 0);

			
//		term->op->tprintf(term, "size %d , name %s  addr : %ld\n",
//			size,xsh->argv[4],
//			zc_malloc_mem(size,"asdf",0));
		
//		}

	return;
}
static void docmd_phy_test_set(xsh_t *xsh)
{
//	tty_t *term   = xsh->term;


//	if (__strcmp(xsh->argv[2], "test") == 0) {
		
//		HPLC.testmode = __strtoul(xsh->argv[3], NULL, 0);
			
//		term->op->tprintf(term, "mode %s \n",HPLC.testmode==PHYTPT?"PHYTPT":
//			HPLC.testmode==PHYCB?"PHYCB":
//				HPLC.testmode==MACTPT?"MACTPT":
//					HPLC.testmode==NORM?"NORM":
//						HPLC.testmode==MONITOR?"MONITOR":"UKN");
//		}

	return;
}

static void docmd_sort_slot(xsh_t *xsh)
{
	packet_beacon_period(NULL);
}
void docmd_phy_perf_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;
	phy_perf_mode_t mode;

	if (phy_ioctl(PHY_CMD_GET_PERF_MODE, &mode) != OK) {
		term->op->tprintf(term, "phy ioctl get perf mode failed\n");	
		return;
	}

	term->op->tprintf(term, "phy perf mode : %s\n", 
		(mode == PHY_PERF_MODE_A) ? "A" : (mode == PHY_PERF_MODE_B) ? "B" : (mode == PHY_PERF_MODE_C) ? "C" : "D");

	return;
}

void docmd_phy_notch_show(xsh_t *xsh)
{
	phy_notch_freq_t notch;
	tty_t *term = xsh->term;
	int32_t i;

	for (i = 0; i < 2; ++i) {
		notch.idx = i;
		if (phy_ioctl(PHY_CMD_GET_NOTCH_FREQ, &notch) != OK) {
			term->op->tprintf(term, "phy ioctl get notch freq failed\n");	
			return;
		}
		term->op->tprintf(term, "notch freq %d : %dHz\n", notch.idx, notch.freq);
	}

	return;
}

void docmd_phy_band_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	// term->op->tprintf(term, "CURR_B:    %d\n", CURR_B == &BAND[0] ? 0 : 
	// 					    CURR_B == &BAND[1] ? 1 : 
	// 					    CURR_B == &BAND[2] ? 2 : 3);
	term->op->tprintf(term, "CURR_B:    %d\n", CURR_B->idx);
	term->op->tprintf(term, "start:     %d\n", CURR_B->start);
	term->op->tprintf(term, "end:       %d\n", CURR_B->end);
	term->op->tprintf(term, "head symn: %d\n", CURR_B->symn);
	term->op->tprintf(term, "head mm:   %s\n", mm_getstr(CURR_B->mm));
	term->op->tprintf(term, "head size: %d\n", CURR_B->size ? 32 : 16);
	term->op->tprintf(term, "freq:      %.3f-%.3fMHz\n", CURR_B->start * 24.414 / 1000.0, CURR_B->end * 24.414 / 1000.0);

	return;
}

void docmd_phy_tgain_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	 phy_tgain_t tgain;

	if (phy_ioctl(PHY_CMD_GET_TGAIN, &tgain) != OK) {
			term->op->tprintf(term, "phy ioctl get tagin freq failed\n");	
			return;
		}
		term->op->tprintf(term, "tgain dig %d ,ana %d\n", tgain.dig,tgain.ana);

	return;
}

void docmd_phy_rxtx_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	term->op->tprintf(term, "plc rx all %d,err %d\n",phy_rxtx_info.rx_cnt, phy_rxtx_info.rx_err_cnt );
    term->op->tprintf(term, "tx data %d,full %d\n",phy_rxtx_info.tx_data_cnt,phy_rxtx_info.tx_data_full);
    term->op->tprintf(term, "tx mng %d,full %d\n",phy_rxtx_info.tx_mng_cnt,phy_rxtx_info.tx_mng_full);

	return;
}
void docmd_phy_rxtx_clear(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	term->op->tprintf(term, "clear rxtx\n");
    phy_rxtx_info.rx_cnt = 0;
    phy_rxtx_info.rx_err_cnt = 0;
    phy_rxtx_info.tx_data_cnt = 0;
    phy_rxtx_info.tx_data_full = 0;
    phy_rxtx_info.tx_mng_cnt = 0;
    phy_rxtx_info.tx_mng_full = 0;
    docmd_phy_rxtx_show(xsh);

	return;
}
void hplc_statis_show(xsh_t *xsh,STATICS_t statis)
{
    tty_t *term = xsh->term;

	term->op->tprintf(term, "nr_fatal_error %d\tnr_phy_reset_fail %d\n", statis.nr_fatal_error, statis.nr_phy_reset_fail);
    term->op->tprintf(term, "nr_late_csma_s %d\tnr_fail_csma_send %d\n", statis.nr_late_csma_send, statis.nr_fail_csma_send);
    term->op->tprintf(term, "nr_type \tBCN\tSOF\tSACK\tCORD\n");
    term->op->tprintf(term, "nr_late \t%d\t%d\t%d\t%d\n", statis.nr_late[0], statis.nr_late[1], statis.nr_late[2], statis.nr_late[3]);
    term->op->tprintf(term, "nr_ts_late \t%d\t%d\t%d\t%d\n", statis.nr_ts_late[0], statis.nr_ts_late[1], statis.nr_ts_late[2], statis.nr_ts_late[3]);
    term->op->tprintf(term, "nr_fail \t%d\t%d\t%d\t%d\n", statis.nr_fail[0], statis.nr_fail[1], statis.nr_fail[2], statis.nr_fail[3]);
    term->op->tprintf(term, "nr_ts_frame \t%d\t%d\t%d\t%d\n", statis.nr_ts_frame[0], statis.nr_ts_frame[1], statis.nr_ts_frame[2], statis.nr_ts_frame[3]);
}
void docmd_phy_hplc_statis_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;
    term->op->tprintf(term, "HPLC.tx.statis\n");
    hplc_statis_show(xsh,HPLC.tx.statis);
    term->op->tprintf(term, "HPLC.rx.statis\n");
    hplc_statis_show(xsh,HPLC.rx.statis);
    term->op->tprintf(term, "HPLC.rf_tx.statis\n");
    hplc_statis_show(xsh,HPLC.rf_tx.statis);
    term->op->tprintf(term, "HPLC.rf_rx.statis\n");
    hplc_statis_show(xsh,HPLC.rf_rx.statis);

	return;
}
void docmd_phy_me_show(xsh_t *xsh)
{
    tty_t *term = xsh->term;
#if defined(STATIC_MASTER)
    term->op->tprintf(term, "tx-mode\n");
#else
    term->op->tprintf(term, "monitor\n");
#endif
//    term->op->tprintf(term, "monitor\n");
}
void docmd_phy_hplc_statis_clear(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	term->op->tprintf(term, "clear hplc_statis\n");
    memset((U8 *)&HPLC.tx.statis,0x00,sizeof(STATICS_t));
    memset((U8 *)&HPLC.rx.statis,0x00,sizeof(STATICS_t));
    memset((U8 *)&HPLC.rf_tx.statis,0x00,sizeof(STATICS_t));
    memset((U8 *)&HPLC.rf_rx.statis,0x00,sizeof(STATICS_t));

	return;
}

void docmd_phy_lowpower_set(xsh_t *xsh)
{   
    tty_t *term = xsh->term;
    phy_ana_lpwr_t  ana_lpwr;
    phy_bb_lpwr_t   bb_lpwr;

	if (__strcmp(xsh->argv[2], "ana") == 0) {			
		ana_lpwr = __strtoul(xsh->argv[3], NULL, 0);			
		if ((ana_lpwr != PHY_ANA_LPWR_DISABLE && ana_lpwr != PHY_ANA_LPWR_LEVEL1 && ana_lpwr != PHY_ANA_LPWR_LEVEL2) \
            || phy_ioctl(PHY_CMD_SET_ANA_LPWR, &ana_lpwr) != OK) {
			term->op->tprintf(term, "set ana_lpwr %d failed\n",ana_lpwr);	
			return;
		}
        term->op->tprintf(term, "set ana_lpwr %d ok\n",ana_lpwr);
	} else if (__strcmp(xsh->argv[2], "bb") == 0) {
		bb_lpwr = __strtoul(xsh->argv[3], NULL, 0);			
		if ((bb_lpwr != 0 && bb_lpwr != 1) \
            || phy_ioctl(PHY_CMD_SET_BB_LPWR, &bb_lpwr) != OK) {
			term->op->tprintf(term, "set bb_lpwr %d failed\n",bb_lpwr);	
			return;
		}
        term->op->tprintf(term, "set bb_lpwr %d ok\n",bb_lpwr);
	}

	return;
}

void phy_demo_init(void)
{
	phy_dec_clk_t clk = 1;

	phy_sal_init(2);

	if (phy_ioctl(PHY_CMD_SET_DEC_CLK, &clk) < 0)
		printf_s("phy set dec clk error!\n");

	register_phy_hdr_isr(isr_hdr_demo_rx);

	phy_reset_sfo(&HSFO, 0, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
	register_clock_sync_hook(demo_clock_sync, NULL);

	memset(&PHY_DEMO, 0, sizeof(phy_demo_t));
	PHY_DEMO.phase = PHY_PHASE_ALL;
    //phy_txmsg_init();

	return;
}
