/*
 * Copyright: (c) 2006-2020, 2020 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        wphy_debug.c
 * Purpose:     wphy debug functions.
 * History:     
 */
#if defined(STD_DUAL)
#include "types.h"
#include "vsh.h"
#include "phy.h"
#include "phy_debug.h"
#include "phy_sal.h"
#include "framectrl.h"
#include "flash.h"
#include "printf_zc.h"
#include "plc.h"
#include "wlc.h"

typedef struct {
	uint32_t bp;
	uint32_t dur;
	uint32_t delay;

	uint32_t mpdu;
	
    uint8_t  rfcb_flag ;
    uint8_t  res[3];

	phy_frame_info_t fi;

//#if (!STATIC_MASTER && !STATIC_NODE)
	uint8_t hdr[32];
	uint8_t pld[524] __CACHEALIGNED;
//#endif
} wphy_testinfo_t;

wphy_testinfo_t wphy_test;

extern phy_rxtx_cnt_t wphy_rxtx_info;

void dump_info(tty_t *term, uint8_t option, uint8_t phr_mcs, uint8_t pld_mcs, uint32_t pbsz, uint8_t pbc)
{
	term->op->tprintf(term, "option:%d phr_mcs:%d pld_mcs:%d pbc:%d pbsz:",
			option, phr_mcs, pld_mcs, pbc);
	if (pbsz == 0)
		term->op->tprintf(term, "16\n");
	else if (pbsz == 1)
		term->op->tprintf(term, "40\n");
	else if (pbsz == 2)
		term->op->tprintf(term, "72\n");
	else if (pbsz == 3)
		term->op->tprintf(term, "136\n");
	else if (pbsz == 4)
		term->op->tprintf(term, "264\n");
	else if (pbsz == 5)
		term->op->tprintf(term, "520\n");
}

__LOCALTEXT int32_t wbeacon_hdr_proc(phy_frame_info_t *fi, void *context)
{
	rf_beacon_ctrl_t *bc = (rf_beacon_ctrl_t *)fi->head;

	if (bc->mcs >= WPHY_PLD_MCS_MAX)
		return -1;
	if (bc->blkz >= WPHY_BLKZ_MAX)
		return -2;

	fi->wpi.pld_mcs   = bc->mcs;
	fi->wpi.blkz      = bc->blkz;
	fi->wpi.pbc       = 1;
	fi->wpi.crc32_en  = 1;
	fi->wpi.crc32_ofs = 7;
	fi->length        = fi->wpi.pbc * wphy_get_pbsz(bc->blkz);

	return 0;
}

__LOCALTEXT int32_t wsof_hdr_proc(phy_frame_info_t *fi, void *context)
{
	rf_sof_ctrl_t *sof = (rf_sof_ctrl_t *)fi->head;

	if (sof->mcs >= WPHY_PLD_MCS_MAX)
		return -1;
	if (sof->blkz >= WPHY_BLKZ_MAX)
		return -2;

	fi->wpi.pld_mcs  = sof->mcs;
	fi->wpi.blkz     = sof->blkz;
	fi->wpi.pbc      = 1;
	fi->wpi.crc32_en = 0;
	fi->length       = fi->wpi.pbc * wphy_get_pbsz(sof->blkz);

	return 0;
}

__LOCALTEXT int32_t wsack_hdr_proc(phy_frame_info_t *fi, void *context)
{
	fi->wpi.crc32_en = 0;
	fi->wpi.pbc      = 0;
	fi->length       = 0;

	return 0;
}
__LOCALTEXT int32_t wcoord_hdr_proc(phy_frame_info_t *fi, void *context)
{
	fi->wpi.crc32_en = 0;
	fi->wpi.pbc      = 0;
	fi->length       = 0;

	return 0;
}

typedef int32_t (*wphy_hdr_proc)(phy_frame_info_t *fi, void *context);
__LOCALDATA static wphy_hdr_proc WHDR_PROC[] = {
	wbeacon_hdr_proc,	/* 0, DT_BEACON */
	wsof_hdr_proc,		/* 1, DT_SOF */
	wsack_hdr_proc,		/* 2, DT_SACK */
	wcoord_hdr_proc,	/* 3, DT_COORD */
	NULL,			/* 4 */
	NULL,			/* 5 */
	NULL,			/* 6 */
	NULL,			/* 7, DT_SILENT */
};

__LOCALTEXT __isr__ void wphy_hdr_isr_default(phy_frame_info_t *fi, void *context)
{
	frame_ctrl_t *head = (frame_ctrl_t *)fi->head;

	if (!wheader_is_valid(fi))
		goto out;

#if defined(STD_2016) || defined(STD_DUAL)
	if (head->access) {
#else
	if (!head->access) {
#endif
		fi->whi.valid = HDR_INVALID_PROC;
		goto out;
	}

	if (!WHDR_PROC[head->dt] || WHDR_PROC[head->dt](fi, context) != OK)
		fi->whi.valid = HDR_INVALID_PROC;
out:
	wphy_frame_rx(fi, context);
	return;
}

int32_t wfc_construct(frame_ctrl_t *head, uint8_t dt, uint32_t snid, uint16_t stei, uint16_t dtei, uint8_t mcs, uint8_t blkz)
{
	rf_beacon_ctrl_t *beacon;
	rf_sof_ctrl_t *sof;

	memset(head, 0, sizeof(rf_frame_ctrl_t));

	head->dt     = dt;
	head->access = 0;
	head->snid   = snid;
	head->std    = 0;

	switch (dt) {
	case DT_BEACON:
		beacon = (rf_beacon_ctrl_t *)head;
		beacon->bts   = 0;
		beacon->stei  = stei;
		beacon->mcs   = mcs;
		beacon->blkz  = blkz;
		break;
	case DT_SOF:
		sof         = (rf_sof_ctrl_t *)head;
		sof->stei   = stei;
		sof->dtei   = dtei;
		sof->lid    = 0;
		sof->fl     = 0;
		sof->blkz   = blkz;
		sof->bcsti  = 0;
		sof->repeat = 0;
		sof->enc    = 0;
		sof->mcs    = mcs;
		break;
	case DT_SACK:
	case DT_COORD:
		break;
	default:
		return ERROR;
	}

	return OK;
}


__isr__ void wphy_test_tx_end(phy_evt_param_t *para, void *arg)
{
	phy_frame_info_t *fi = para->fi;
	wphy_testinfo_t *test;
	rf_frame_ctrl_t *head;
	evt_notifier_t ntf;
	int32_t  ret;
	uint32_t mpdu, time, dur;

	test = (wphy_testinfo_t *)arg;
	mpdu = test->mpdu;
	head = (rf_frame_ctrl_t *)fi->head;


    if ((mpdu & 0xf) == 1) {
        wphy_debug_printf("-------------------------------------------------------------------------------\n");
        wphy_debug_printf("idx         frame     gain nid hmcs pmcs pbsz len      stime    curr\n");
        wphy_debug_printf("-------------------------------------------------------------------------------\n");
    }

    wphy_debug_printf("%-8d tf %-9s %-4d %-3d %-3d  %-3d   %-3d %08d %08x %08x\n",
             test->mpdu, frame2str(head->dt), 0, head->snid, fi->whi.phr_mcs, fi->wpi.pld_mcs,
             wphy_get_pbsz(fi->wpi.blkz), fi->length, para->stime, get_phy_tick_time());

    if (!test->mpdu)
        wphy_debug_printf("wphy tx frame done.\n");

    if(test->rfcb_flag)     //测试回传，发送完成之后进入接收状态
    {
        evt_notifier_t ntf[3];
        uint32_t cpu_sr, timeout = 0;
        extern void wphy_test_rx_end(phy_evt_param_t *para, void *arg);
        
        wphy_reset();
        fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wphy_test_rx_end, &wphy_test);
        fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wphy_test_rx_end, &wphy_test);
        fill_evt_notifier(&ntf[2], WPHY_EVT_EORX,wphy_test_rx_end, &wphy_test);

        timeout = get_phy_tick_time() + PHY_MS2TICK(1000);

        cpu_sr = OS_ENTER_CRITICAL();
        if ((ret = wphy_start_rx(NULL, timeout? &timeout : NULL, ntf, NELEMS(ntf))) != OK) {
            phy_debug_printf("start wrx frame failed, ret = %d\n", ret);
            OS_EXIT_CRITICAL(cpu_sr);
            return;
        }
        OS_EXIT_CRITICAL(cpu_sr);
    }

    else if (--test->mpdu > 0) {
		dur = wphy_get_frame_dur(wphy_get_option(), fi->whi.phr_mcs, fi->wpi.pld_mcs, wphy_get_pbsz(fi->wpi.blkz), fi->wpi.pbc);
		time = get_phy_tick_time() + dur +  WPHY_BIFS;//PHY_MS2TICK(wphy_test.bp) +
        if (1) {
                    if (wphy_test.bp) {
        //              if (time_after(para->stime + PHY_DEMO.bp, time)) {
        //                  time = para->stime + PHY_DEMO.bp;
        //              }
                        time = time + wphy_test.bp;
                    }
                }

		fill_evt_notifier(&ntf, WPHY_EVT_EOTX, wphy_test_tx_end, &wphy_test);
		if ((ret = wphy_start_tx(&test->fi, &time, &ntf, 1)) != OK) {
			wphy_debug_printf("start tx frame failed, ret = %d\n", ret);
			return;
		}
	}



	return;
}

static inline void fill_random_data(uint8_t *data, int32_t len)
{
	int32_t i;

	srand(cpu_get_cycle());
	for (i = 0; i < len; ++i)
		data[i] = rand();
	return;
}

void docmd_wphy_tx_start(xsh_t *xsh)
{
	tty_t *term = xsh->term;
	evt_notifier_t ntf;
	phy_frame_info_t *fi = &wphy_test.fi;
	wphy_hdr_info_t *whi = &fi->whi;
	wphy_pld_info_t *wpi = &fi->wpi;
	wpbh_t *wpbh;
	uint32_t i, cpu_sr, stime = 0;
	int32_t  ret;
	char *s;
	uint8_t dt = 0, option = 0;

	wphy_reset();

	option = wphy_get_option();
	whi->phr_mcs  = (option == WPHY_OPTION_3) ? 2 : 1; /* option3, mcs0,1 is invalid */
	wpi->pld_mcs  = 1;
	wpi->blkz     = 3; /* default pbsz 136 */
	wpi->crc32_en = 0;

	wphy_test.bp = 0;

	if (__strcmp(xsh->argv[3], "beacon") == 0) {
		dt = DT_BEACON;
		wpi->pbc = 1;
	} else if (__strcmp(xsh->argv[3], "sof") == 0) {
		dt = DT_SOF;
		wpi->pbc = 1;
	} else if (__strcmp(xsh->argv[3], "sack") == 0) {
		dt = DT_SACK;
		wpi->pbc = 0;
	} else if (__strcmp(xsh->argv[3], "coord") == 0) {
		dt = DT_COORD;
		wpi->pbc = 0;
	}


	wphy_test.mpdu = __strtoul(xsh->argv[4], NULL, 0);

	for (i = 5; i < xsh->argc; ++i) {
		if ((s = xsh_getopt_val(xsh->argv[i], "-pbsz="))) {
			if (__strcmp(s, "16") == 0)       wpi->blkz = 0;
			else if (__strcmp(s, "40") == 0)  wpi->blkz = 1;
			else if (__strcmp(s, "72") == 0)	wpi->blkz = 2;
			else if (__strcmp(s, "136") == 0) wpi->blkz = 3;
			else if (__strcmp(s, "264") == 0) wpi->blkz = 4;
			else if (__strcmp(s, "520") == 0) wpi->blkz = 5;
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-phr_mcs"))) {
			whi->phr_mcs = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-pld_mcs"))) {
			wpi->pld_mcs = __strtoul(s, NULL, 0);
		} else if ((s = xsh_getopt_val(xsh->argv[i], "-stime="))) {
			stime = __strtoul(s, NULL, 0);
        }else if ((s = xsh_getopt_val(xsh->argv[i], "-rfcb="))) {
            wphy_test.rfcb_flag = __strtoul(s, NULL, 0);
        }else if ((s = xsh_getopt_val(xsh->argv[i], "-bp="))) {
			wphy_test.bp = __strtoul(s, NULL, 0);
//			PHY_DEMO.bp = MAX(PHY_DEMO.bp, BEACON_PERIOD_MIN) * (PHY_TICK_FREQ / 1000);
		}

	}

	fi->head = &wphy_test.hdr;
	wfc_construct((frame_ctrl_t *)fi->head, dt, 0, 0, 0, wpi->pld_mcs, wpi->blkz);
	fi->payload = &wphy_test.pld;
	fi->length = wphy_get_pbsz(wpi->blkz) * wpi->pbc;

	if (wpi->pbc != 0) {
		if (dt == DT_BEACON) {
			wpi->crc32_en  = 1;
			wpi->crc32_ofs = 7;
			fill_random_data(fi->payload, fi->length - sizeof(bpcs_t) - sizeof(pbcs_t));
		} else {
			wpbh = (wpbh_t *)fi->payload; 	
			wpbh->ssn = 0;
			wpbh->mfsf = 1;
			wpbh->mfef = 1;
			fill_random_data((uint8_t *)(wpbh + 1), fi->length - sizeof(wpbh_t) - sizeof(pbcs_t));
		}
	}
	dhit_write_back((uint32_t)fi->payload, fi->length);
    
    term->op->tprintf(term, "wphy_test.bp:%d us!", wphy_test.bp);
    wphy_test.bp *= PHY_US2TICK(1);

    term->op->tprintf(term, "wphy_test.bp:%d tick!\n", wphy_test.bp);

	cpu_sr = OS_ENTER_CRITICAL();
	fill_evt_notifier(&ntf, WPHY_EVT_EOTX, wphy_test_tx_end, &wphy_test);
	if (stime)
		stime += get_phy_tick_time();
	if ((ret = wphy_start_tx(&wphy_test.fi, (stime) ? &stime : NULL, &ntf, 1)) != OK)
		term->op->tprintf(term, "start wtx frame failed, ret = %d\n", ret);
	OS_EXIT_CRITICAL(cpu_sr);
}

__isr__ void wphy_test_rx_frame(wphy_testinfo_t *test, phy_frame_info_t *fi, uint32_t stime, int32_t gain, uint32_t snr, int32_t rssi)
{
	rf_frame_ctrl_t *head = (rf_frame_ctrl_t *)fi->head;
	char *crc = "n/a";
	static int32_t count = 0;
	
	++test->mpdu;

	if (!wheader_is_valid(fi)) {
		wphy_debug_printf("%-8d rx hdr error, gain %d\n", test->mpdu, gain);
		return;
	}

	if ((count & 0xf) == 0) {
		wphy_debug_printf("-------------------------------------------------------------------------------\n");
		wphy_debug_printf("idx         frame     gain snr rssi nid hmcs pmcs pbsz len      stime    curr     crc\n");
		wphy_debug_printf("-------------------------------------------------------------------------------\n");
		++count;
	}

	++count;

	crc = fi->wpi.crc24 ? "err" : "ok";

	if (!fi->wpi.crc24) {
		if (fi->wpi.crc32_en)
			crc = fi->wpi.crc32 ? "err32" : "ok";
	} else {
		crc = "err";
	}

    if(test->rfcb_flag)
    {
//        dump_printfs(test->hdr, sizeof(rf_frame_ctrl_t) );
//        dump_printfs(fi->head, sizeof(rf_frame_ctrl_t) );

//        dump_printfs(test->pld, fi->length);
//        dump_printfs(fi->payload, fi->length);
        test->rfcb_flag = 0;


        if(0 == memcmp(test->hdr, fi->head, sizeof(rf_frame_ctrl_t) - 3) \
        && 0 == memcmp(test->pld, fi->payload, fi->length - 3))
        {
            wphy_printf("rf cb Test pass!\n");
        }
        else
        {
            wphy_printf("rf cb Test fail!\n");
        }
    }

	wphy_debug_printf("%-8d rf %-9s %-4d  %-3d %-3d %-3d %-3d  %-3d   %-3d %08d %08x %08x %-3s\n",
			 test->mpdu, frame2str(head->dt), gain, snr, rssi, head->snid, fi->whi.phr_mcs, fi->wpi.pld_mcs, 
			 wphy_get_pbsz(fi->wpi.blkz), fi->length, stime, get_phy_tick_time(), crc);
	return;
}

__isr__ void wphy_test_rx_end(phy_evt_param_t *para, void *arg)
{
	wphy_testinfo_t *test;
	int32_t  ret;
	evt_notifier_t ntf[3];

	test = (wphy_testinfo_t *)arg;

	fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wphy_test_rx_end, &wphy_test);
	fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wphy_test_rx_end, &wphy_test);
	fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wphy_test_rx_end, &wphy_test);

	switch (para->event) {
	case WPHY_EVT_PHR:
		break;
	case WPHY_EVT_SIG_TMOT:
		if ((ret = wphy_start_rx(NULL, NULL, ntf, NELEMS(ntf))) != OK) {
			phy_debug_printf("start wrx frame failed, ret = %d\n", ret);
			return;
		}
		break;
	case WPHY_EVT_EORX:
		if ((ret = wphy_start_rx(NULL, NULL, ntf, NELEMS(ntf))) != OK) {
			phy_debug_printf("start wrx frame failed, ret = %d\n", ret);
			return;
		}
		wphy_test_rx_frame(test, para->fi, para->stime, para->rgain, para->rf_snr, para->rf_rssi);
		break;
	default:
		break;
	}
}

void docmd_wphy_rx_start(xsh_t *xsh)
{
	evt_notifier_t ntf[3];
	uint32_t cpu_sr, timeout = 0;
	int32_t  ret, i;
	char *s;

	wphy_reset();


    extern int32_t demo_clock_sync(phy_evt_param_t *para, void *arg);
    register_clock_sync_hook(demo_clock_sync, NULL);

	fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wphy_test_rx_end, &wphy_test);
	fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wphy_test_rx_end, &wphy_test);
	fill_evt_notifier(&ntf[2], WPHY_EVT_EORX,wphy_test_rx_end, &wphy_test);

	for (i = 3; i < xsh->argc; ++i) {
		if ((s = xsh_getopt_val(xsh->argv[i], "-timeout="))) {
			timeout = __strtoul(s, NULL, 0);
		}
	}

	if (timeout)
		timeout = get_phy_tick_time() + PHY_MS2TICK(timeout);

	cpu_sr = OS_ENTER_CRITICAL();
	if ((ret = wphy_start_rx(NULL, timeout? &timeout : NULL, ntf, NELEMS(ntf))) != OK) {
		phy_debug_printf("start wrx frame failed, ret = %d\n", ret);
		OS_EXIT_CRITICAL(cpu_sr);
		return;
	}
	OS_EXIT_CRITICAL(cpu_sr);
}

void wphy_show_profile(tty_t *term)
{
	uint32_t fckhz;
	uint8_t  option, channel;

	option  = wphy_get_option();
	fckhz   = wphy_get_fckhz();
	channel = wphy_hz_to_ch(option, fckhz * 1000);

	term->op->tputs(term,   "Current wphy profile: \n");
	term->op->tprintf(term, "option:      %d\n",     option);
	term->op->tprintf(term, "khz:         %d kHz\n", fckhz);
    term->op->tprintf(term, "channel:     %d\n",     channel);



    term->op->tprintf(term, "BOARD_CFG->rf_config0 = %d\n", BOARD_CFG->rf_config0);
    term->op->tprintf(term, "BOARD_CFG->rf_config1 = %d\n", BOARD_CFG->rf_config1);
    term->op->tprintf(term, "BOARD_CFG->rf_config2 = %d\n", BOARD_CFG->rf_config2);
    term->op->tprintf(term, "BOARD_CFG->rf_config3 = %d\n", BOARD_CFG->rf_config3);

	return;
}


void docmd_wphy_rxtx_show(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	term->op->tprintf(term, "rf rx all %d,err %d\n",wphy_rxtx_info.rx_cnt, wphy_rxtx_info.rx_err_cnt );
    term->op->tprintf(term, "tx data %d,full %d\n",wphy_rxtx_info.tx_data_cnt,wphy_rxtx_info.tx_data_full);
    term->op->tprintf(term, "tx mng %d,full %d\n",wphy_rxtx_info.tx_mng_cnt,wphy_rxtx_info.tx_mng_full);

	return;
}
void docmd_wphy_rxtx_clear(xsh_t *xsh)
{
	tty_t *term = xsh->term;

	term->op->tprintf(term, "clear rxtx\n");
    wphy_rxtx_info.rx_cnt = 0;
    wphy_rxtx_info.rx_err_cnt = 0;
    wphy_rxtx_info.tx_data_cnt = 0;
    wphy_rxtx_info.tx_data_full = 0;
    wphy_rxtx_info.tx_mng_cnt = 0;
    wphy_rxtx_info.tx_mng_full = 0;
    docmd_wphy_rxtx_show(xsh);

	return;
}

#if defined(VENUS2M) || defined(VENUS8M)

wphy_clibr_data_t clibr_data;
// wphy_clibr_slave_data_t slave_clibr_data;
extern board_cfg_t BoardCfg_t;


#endif
extern flash_dev_t *FLASH;
extern void phy_msg_show(xsh_t *xsh,tx_link_t *tx_link);

param_t cmd_wphy_param_tab[] = {
	{ PARAM_ARG | PARAM_TOGGLE,		 "", "start|stop|set|show|clear|reset|ft" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "tx|rx", "start" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "sof|beacon|sack|coord", "tx" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num",     "",		   "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "phr_mcs", "",	           "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "pld_mcs", "",	           "sof" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "pbsz",  "16|40|72|136|264|520", "sof" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "stime", "start frame time",	   "sof" },
    { PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "rfcb", "rf cb flag",	   "sof" },
    { PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bp",   "Period(us)",    "sof" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num",     "",		  "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "phr_mcs", "",	          "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "pld_mcs", "",	          "beacon" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "pbsz", "16|40|72|136|264|520", "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "bp",   "Beacon Period(us)",    "beacon" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "stime", "start frame time",	  "beacon" },
    { PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "rfcb", "rf cb flag",    "beacon" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num",     "",		  "sack" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "phr_mcs", "",	          "sack" },

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB,                  "num",     "",                  "coord" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "phr_mcs", "",	          "coord" },

	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL,  "", "cap", "busy_rx" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB, "len",  "len",  "cap" },
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB, "tone", "", "sin" },

	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "timeout", "", "rx" },

    { PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "band|pa|mode", "set" },
    { PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB, "option", "1|2|3", "band" },
	{ PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "channel","",      "band" },
    { PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "dbm<-60~19>","",      "pa" },
    { PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "mode<0-3>","",      "mode" },
    { PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,  "",     "statis|trace|profile|msg|pa|mode|rxtx", "show" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,  "",     "statis|rxtx", "clear" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,  "",     "master|slave", "ft" },
	{ PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,  "",     "init|uinit|start|stop|get|set|write|read|state|reset",      "ft" },
	PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_wphy_desc,
	"wphy cmd", cmd_wphy_param_tab
	);

void docmd_wphy(command_t *cmd, xsh_t *xsh)
{
	tty_t *term = xsh->term;

	if (__strcmp(xsh->argv[1], "start") == 0) {
		if (__strcmp(xsh->argv[2], "tx") == 0) {
			docmd_wphy_tx_start(xsh);
		} else if (__strcmp(xsh->argv[2], "rx") == 0) {
			docmd_wphy_rx_start(xsh);
		}
	} else if (__strcmp(xsh->argv[1], "reset") == 0) {
	    //wphy_debug_init();
		while (wphy_reset() != OK)
			sys_delayms(1);
	} else if (__strcmp(xsh->argv[1], "show") == 0) {
		if (__strcmp(xsh->argv[2], "statis") == 0) {
			wphy_show_statis(term);
		} else if (__strcmp(xsh->argv[2], "trace") == 0) {
			wphy_trace_show(term);
		} else if (__strcmp(xsh->argv[2], "profile") == 0) {
			wphy_show_profile(term);
        } else if (__strcmp(xsh->argv[2], "msg") == 0) {
			phy_msg_show(xsh,&WL_TX_MNG_LINK);
			phy_msg_show(xsh,&WL_TX_DATA_LINK);
		} else if (__strcmp(xsh->argv[2], "pa") == 0) {
            term->op->tprintf(term, "wphy tgain:  %d\n", wphy_get_pa());
        } else if (__strcmp(xsh->argv[2], "mode") == 0) {
            //term->op->tprintf(term, "wphy mode:  %d\n", 0/*wphy_get_mode()*/);
        } else if (__strcmp(xsh->argv[2], "rxtx") == 0) {
            docmd_wphy_rxtx_show(xsh);
        }
	} else if (__strcmp(xsh->argv[1], "set") == 0) {
		if (__strcmp(xsh->argv[2], "band") == 0) {
			uint8_t option = __strtoul(xsh_getopt_val(xsh->argv[3], "-option="), NULL, 10); 
			term->op->tprintf(term, "set option:  %d\n", option);
			wphy_set_option(option);
             HPLC.option = option;
    
			if (xsh->argc == 5) {
				uint32_t channel  = __strtoul(xsh_getopt_val(xsh->argv[4], "-channel="), NULL, 10);
				term->op->tprintf(term, "set channel: %d\n", channel);
				wphy_set_channel(channel);
                HPLC.rfchannel = channel;
			}
		}
        else if(__strcmp(xsh->argv[2], "pa") == 0)
        {
            int16_t dbm = __atoi(xsh->argv[3]);
            if(dbm < -21 || dbm > 25)
            {
                term->op->tprintf(term, "Set tgain<%d> err!", dbm);
                return ;
            }
            wphy_set_pa(dbm);
            term->op->tprintf(term, "Set wphy tgain %d!\n", wphy_get_pa());
        }
        else if(__strcmp(xsh->argv[2], "mode") == 0)
        {
            uint16_t mode = __atoi(xsh->argv[3]);
            if(mode < 0 || mode > 3)
            {
                term->op->tprintf(term, "Set mode<%d> err!", mode);
                return ;
            }
            wphy_set_mode(mode);
            //set_pa_ldovs(200);//最新的SDK mode1比原来SDK增大了功率，主要给送检的时候提高发送频谱模板用的，杂散差一点的模块可能过不了，向来;20220103不再需要
            term->op->tprintf(term, "Set wphy mode %d!\n", mode);
        }
	} else if (__strcmp(xsh->argv[1], "clear") == 0) {
		if (__strcmp(xsh->argv[2], "statis") == 0) {
			wphy_clear_statis();
		} else if (__strcmp(xsh->argv[2], "rxtx") == 0) {
			docmd_wphy_rxtx_clear(xsh);
		}
	}
#if defined(VENUS2M) || defined(VENUS8M)
        else if (__strcmp(xsh->argv[1], "ft") == 0) {
		if (__strcmp(xsh->argv[2], "master") == 0) {
			if (__strcmp(xsh->argv[3], "init") == 0) {
                //被测模块校准初始化
                wphy_clibr_init();
            }else if (__strcmp(xsh->argv[3], "uinit") == 0) {
                //被测模块校准逆初始化
                wphy_clibr_uninit();
            }else if (__strcmp(xsh->argv[3], "start") == 0) {
                //被测模块校准开始
                wphy_clibr_start();
            }else if (__strcmp(xsh->argv[3], "stop") == 0) {
                //被测模块校准结束
                wphy_clibr_stop();
            }else if (__strcmp(xsh->argv[3], "get") == 0) {
                //被测模块获取校准数据
                wphy_get_clibr_data(&clibr_data);
                term->op->tprintf(term, "rf_config 0 %04x  1 %04x  2 %04x  3 %04x \n" ,clibr_data.rf_config0
                    , clibr_data.rf_config1, clibr_data.rf_config2, clibr_data.rf_config3);
            }else if (__strcmp(xsh->argv[3], "set") == 0) {
                //被测模块设置校准数据
                wphy_set_clibr_data(clibr_data);
            }else if (__strcmp(xsh->argv[3], "write") == 0) {
                //被测模块设置写校准数据
                int32_t state;
                /*
                state=flash_read_demo(&FLASH->spi ,(uint32_t)BOARD_CFG ,(uint32_t *)&BoardCfg_t,sizeof(board_cfg_t));
                FLASH_OK == state ? term->op->tprintf(term, "BOARD_CFG read OK!\n") : 
                    term->op->tprintf(term, "BOARD_CFG read ERROR!\n");
                    */
                term->op->tprintf(term, "BOARD_CFG old rf_config 0 %04x  1 %04x  2 %04x  3 %04x \n",BoardCfg_t.rf_config0
                     , BoardCfg_t.rf_config1, BoardCfg_t.rf_config2, BoardCfg_t.rf_config3);
                BoardCfg_t.rf_config0 = clibr_data.rf_config0;
            	BoardCfg_t.rf_config1 = clibr_data.rf_config1;
            	BoardCfg_t.rf_config2 = clibr_data.rf_config2;
            	BoardCfg_t.rf_config3 = clibr_data.rf_config3;
                term->op->tprintf(term, "BOARD_CFG new rf_config 0 %04x  1 %04x  2 %04x  3 %04x \n",BoardCfg_t.rf_config0 
                    , BoardCfg_t.rf_config1, BoardCfg_t.rf_config2,BoardCfg_t.rf_config3);
                os_sleep(100);
                state = zc_flash_write(FLASH, (uint32_t)BOARD_CFG_ADDR  ,(uint32_t)&BoardCfg_t ,sizeof(board_cfg_t));
                FLASH_OK == state ? term->op->tprintf(term, "BOARD_CFG write OK!\n") : 
                    term->op->tprintf(term, "BOARD_CFG write ERROR!\n");
            }else if (__strcmp(xsh->argv[3], "read") == 0) {
                //被测模块设置写校准数据
                int32_t state;

                state=zc_flash_read(FLASH ,(uint32_t)BOARD_CFG ,(uint32_t *)&BoardCfg_t ,sizeof(board_cfg_t));
                FLASH_OK == state ? term->op->tprintf(term, "BOARD_CFG read OK!\n") : 
                    term->op->tprintf(term, "BOARD_CFG read ERROR!\n");
                term->op->tprintf(term, "BOARD_CFG old rf_config 0 %04x  1 %04x  2 %04x  3 %04x \n",BoardCfg_t.rf_config0
                     , BoardCfg_t.rf_config1, BoardCfg_t.rf_config2, BoardCfg_t.rf_config3);
                //dump_tprintf(term, (uint8_t *)&  BoardCfg_t ,sizeof(board_cfg_t));
               
            }else if (__strcmp(xsh->argv[3], "reset") == 0) {
                //被测模块清除校准数据
                
                BoardCfg_t.rf_config0 = 0;
            	BoardCfg_t.rf_config1 = 0;
            	BoardCfg_t.rf_config2 = 0;
            	BoardCfg_t.rf_config3 = 0;
                clibr_data.rf_config0 = 0;
                clibr_data.rf_config1 = 0;
                clibr_data.rf_config2 = 0;
                clibr_data.rf_config3 = 0;
                term->op->tprintf(term, "BOARD_CFG reset rf_config 0 %04x  1 %04x  2 %04x  3 %04x \n",BoardCfg_t.rf_config0 
                    , BoardCfg_t.rf_config1, BoardCfg_t.rf_config2,BoardCfg_t.rf_config3);
            }else if (__strcmp(xsh->argv[3], "state") == 0) {
                //被测模块获取校准状态
                int32_t states = CLIBR_STATE_IDLE;
                states = wphy_get_clibr_state();
                term->op->tprintf(term, "master states: %s\n", states == CLIBR_STATE_IDLE?"CLIBR_STATE_IDLE":
                                                                states == CLIBR_STATE_RUNNING?"CLIBR_STATE_RUNNING":
                                                                    states == CLIBR_STATE_FIN?"CLIBR_STATE_FIN":
                                                                        "CLIBR_STATE_ERR");
            }
		} else if (__strcmp(xsh->argv[2], "slave") == 0) {
			if (__strcmp(xsh->argv[3], "init") == 0) {
                //标准模块校准初始化
                wphy_clibr_slave_init();
            }else if (__strcmp(xsh->argv[3], "uinit") == 0) {
                //标准模块校准逆初始化
                wphy_clibr_slave_uninit();
            }else if (__strcmp(xsh->argv[3], "start") == 0) {
                //标准模块校准开始
                wphy_clibr_slave_start();
            }else if (__strcmp(xsh->argv[3], "stop") == 0) {
                //标准模块校准结束
                wphy_clibr_slave_stop();
            }else if (__strcmp(xsh->argv[3], "get") == 0) {
                //标准模块获取校准数据
                // int32_t states = CLIBR_STATE_IDLE;
                // states = wphy_get_clibr_slave_data(&slave_clibr_data);
                // term->op->tprintf(term, "slave_clibr_data %d 0 %08x  1 %08x\n",states,slave_clibr_data.diff0, slave_clibr_data.diff1);
            }else if (__strcmp(xsh->argv[3], "set") == 0) {
                //无
                
            }else if (__strcmp(xsh->argv[3], "state") == 0) {
                //标准模块获取校准状态
                int32_t states = CLIBR_STATE_IDLE;
                states = wphy_get_clibr_slave_state();
                term->op->tprintf(term, "slave states: %s\n", states == CLIBR_STATE_IDLE?"CLIBR_STATE_IDLE":
                                                                states == CLIBR_STATE_RUNNING?"CLIBR_STATE_RUNNING":
                                                                    states == CLIBR_STATE_FIN?"CLIBR_STATE_FIN":
                                                                        "CLIBR_STATE_ERR");
            }
		}
	}
    #endif
	return;
}

void wphy_debug_init(void)
{
	register_wphy_hdr_isr(wphy_hdr_isr_default);
	memset(&wphy_test, 0, sizeof(wphy_test));

	return;
}
#endif
