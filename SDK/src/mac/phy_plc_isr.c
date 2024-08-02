/*
 * Copyright: (c) 2016-2020, 2019 ZC Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        phy_plc_isr.c
 * Purpose:     procedure for using libphy.a
 * History:
 */

#include "trios.h"
#include "sys.h"
#include "framectrl.h"
#include "phy_config.h"
#include "phy_sal.h"
#include "vsh.h"
#include "sys.h"
#include "mem_zc.h"
#include "phy_plc_isr.h"
#include "plc.h"
#include "printf_zc.h"
#include "csma.h"
#include "plc_cnst.h"
#include "dev_manager.h"
#include "dl_mgm_msg.h"
#include "plc_io.h"
#include "Schtask.h"
//#include "DataLinkInterface.h"
#include "app_dltpro.h"
#include "meter.h"

#define PHY_NIB_STOP		0
#define PHY_NIB_TX		    1
#define PHY_NIB_RX		    2
#define PHY_NIB_CAP		3



__LOCALTEXT static int32_t do_beacon_hdr(phy_frame_info_t *pf, void *context)
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

    pf->pi.pbsz      = tm->pbsz;
    pf->pi.fec_rate  = tm->fec_rate;
    pf->pi.copy      = tm->copy;
    pf->pi.mm        = tm->mm;
    pf->pi.robo	     = tm->robo;
    pf->pi.tmi       = tmi;
    pf->pi.pbc       = 1;
    pf->pi.crc32_en  = 1;
    pf->pi.crc32_ofs = 7;

	pf->length = pb_bufsize(pf->pi.pbsz) * pf->pi.pbc;

    return OK;
}

__LOCALTEXT static int32_t do_sof_hdr(phy_frame_info_t *pf, void *context)
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

__LOCALTEXT static int32_t do_sack_hdr(phy_frame_info_t *pf, void *context)
{
	pf->length = 0;
	return OK;
}

__LOCALTEXT static int32_t do_coord_hdr(phy_frame_info_t *pf, void *context)
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

__LOCALTEXT static __isr__ void isr_hdr_plc_rx(phy_frame_info_t *pf, void *context)
{
    frame_ctrl_t *head = (frame_ctrl_t *)pf->head;

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

__LOCALTEXT static __isr__ int32_t phy_clock_sync(phy_evt_param_t *para, void *arg)
{
	beacon_ctrl_t *beacon;
	int32_t sfo_int;
	int32_t diff, interval;
	int32_t Clock_Sync_time = 1000;
    uint32_t now_tick = 0;

    static uint32_t last_recv_plc_tick = 0;
    static uint32_t last_recv_pco_tick = 0;

    if(para->fail)
    {
        return 0;
    }

	beacon = (beacon_ctrl_t *)para->fi->head;
	if (beacon->dt != DT_BEACON)
		return 0;

	#if defined(STATIC_MASTER)
	if(HPLC.testmode == NORM)
	{
        return 0;
	}
	#endif
	
    //if((HPLC.snid != beacon->snid || (HPLC.sfotei != beacon->stei && HPLC.sfotei !=0xfff)) && HPLC.testmode == 0)
	if((HPLC.snid != beacon->snid ) && HPLC.testmode == 0)
    {
    	//net_printf("HPLC.snid=%08x,HPLC.sfotei=%02x\n",HPLC.snid,HPLC.sfotei);
    	#if defined(STATIC_NODE)
    	//入网的情况下,台区识别维护时差使用
    	if(HPLC.snid != beacon->snid && HPLC.tei !=0)
    	{
            if(maintain_net_tick)maintain_net_tick(beacon->snid ,beacon->bts, para->stime);
    	}
		#endif
		return 0;
    }
    //未入网时，始终进行同步
    if(GetNodeState() != e_NODE_ON_LINE)
    {
        last_recv_plc_tick = 0;
        last_recv_pco_tick = 0;
    }
    now_tick = get_phy_tick_time();
    //入网后，30秒内过滤非父节点的信标；
    if((HPLC.sfotei != beacon->stei && GetNodeState() == e_NODE_ON_LINE) && last_recv_pco_tick !=0 \
        && time_after(last_recv_pco_tick + PHY_MS2TICK(30000),now_tick))
    {
        return 0;
    }

    //如果15秒内收到过载波信标，不进行无线tick同步
    if(para->event == WPHY_EVT_PHR && last_recv_plc_tick !=0 \
        && time_after(last_recv_plc_tick + PHY_MS2TICK(15000),now_tick))
    {
        return 0;
    }

    //30 秒未收到载波信标，且当前频偏超过 +-50ppm 强制重置频偏
    if(time_before(last_recv_plc_tick + PHY_MS2TICK(30000),now_tick) && abs(phy_get_sfo(SFO_DIR_TX)) > 50)
    {
        last_recv_plc_tick = now_tick;
        phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
    }
    

    //记录上一次载波信标接收时刻
    if(para->event == PHY_EVT_RX_HDR)
    {
        last_recv_plc_tick = now_tick;
    }
    ////记录上一次父节点信标接收时刻
    if(GetProxy() == beacon->stei )
    {
        last_recv_pco_tick = now_tick;
    }
    diff = (int32_t)time_diff(beacon->bts, para->stime);
    interval = (int32_t)time_diff(now_tick, HPLC.sfo_bts);
	//Clock_Sync_time = 2000;
    Clock_Sync_time = ((HPLC.testmode == NORM || HPLC.testmode == MONITOR)?1500: 900);
    #if defined(STATIC_NODE)
    if(FactoryTestFlag == TRUE)
    {
        Clock_Sync_time = 99;
    }
    #endif
    if (HPLC.sfo_not_first && interval < PHY_US2TICK(Clock_Sync_time*1000) )
    {
        return 0;
    }
	net_printf("diff=%d,ival=%d,bts %08x,stime %08x,event %d,sfo %.3f\n",
        diff, interval, beacon->bts, para->stime, para->event,phy_get_sfo(SFO_DIR_TX));
	if (diff > -0x8000 && diff < 0x8000) 
    {
        set_phy_tick_adj(diff);
        if( GetNodeState() != e_NODE_ON_LINE || 
            para->event == PHY_EVT_RX_HDR || 
            (nnib.LinkType == e_RF_Link && para->event == WPHY_EVT_PHR) )
        {
            if (phy_iterate_sfo(&sfo_int, beacon->bts, para->stime, &HSFO) == OK)
            {
                phy_set_sfo(SFO_DIR_TX|SFO_DIR_RX, sfo_int, HSFO.adjust);
                //net_printf("diff=%d,set sfo\n",diff);
            }
        }
        HPLC.sfo_is_done = 1;
		
	} else {
//    if(para->event == PHY_EVT_RX_HDR ||
//        (para->event == WPHY_EVT_PHR && diff < -0x20000 && diff > 0x20000) )
//    {
    	set_phy_tick_time(get_phy_tick_time() + diff);
    	//phy_printf("set tick=%08x\n",get_phy_tick_time());
        // net_printf("diff=%d,set_phy_tick,evt:%d\n",diff, para->event);
//        if(para->event == WPHY_EVT_PHR)
//        {
//            phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
//        }
        //释放当前时隙信息，发送到任务中处理
        //做成函数指针的形式，在上层注册，下层提供注册函数接口。
        if(sync_func_deal)
            sync_func_deal();
//        }
	}
//    HPLC.timeSyncFlag = 1;

	HPLC.sfo_not_first = 1;

    HPLC.sfo_bts       = get_phy_tick_time();//beacon->bts;

	return 0;
}


static __isr__ void reset_notch_filter(void)
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

static __isr__ void test_rx_nbni(phy_evt_param_t *para, void *arg)
{
    phy_nbni_info_t *nbni;
    phy_notch_freq_t notch;
    evt_notifier_t ntf[4];
    uint32_t timeout;

    
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

        // if (nbni->freq && nbni->sir < -2)  // 抓取测试数据使用
        // {
        //     rgain  = 30;
        //     phy_signal_cap_t cap;

        //     cap.stop  = 0;
        //     cap.bw    = 0;
        //     cap.gain  = rgain;
        //     cap.saddr = NULL;
        //     cap.len   = 0;

        //     if ( phy_start_cap(&HPLC.hi, &cap, NULL, 0) != OK) {
        //             debug_printf(&dty, DEBUG_PHY, "Start cap failed\n");
        //     } else {
        //             debug_printf(&dty, DEBUG_PHY, "Start cap with gain: %d\n", rgain);
        //     }

        //     extern __LOCALTEXT void phy_pmd_clock_always_on();
        //     extern __LOCALTEXT void phy_pmd_clock_auto_switch();
        //     phy_pmd_clock_always_on();
        //     dump_mem(&tty, 0, 0xd3100000, 0x8000, 1);
        //     phy_pmd_clock_auto_switch();
        //     tty.op->tputs(&tty, "\n");
        // }

		if (para->notch_freq == 500000) {
			notch.freq = 500000; 
			notch.idx  = 1;
			phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);

			// notch.freq = 1500000; 
			notch.freq = nbni->sir < 0 ? nbni->freq : 1500000; // 通过软件解决谐波notch不完全的问题。
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

    fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, arg);
    fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR, csma_rx_hd, arg);
    fill_evt_notifier(&ntf[2], PHY_EVT_RX_END, csma_rx_end, arg);
    fill_evt_notifier(&ntf[3], PHY_EVT_RX_FIN, swc_rx_frame, arg);


    timeout = get_phy_tick_time() + PHY_CIFS;

    ld_set_rx_phase_zc(PHY_PHASE_ALL);
	phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf));

    return;
}

// __LOCALTEXT __isr__ void phy_notch_freq_set(phy_evt_param_t *para)
// {
//     #if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
//     reset_notch_filter();
//     if(para->notch_freq == 1000000 || para->notch_freq == 5000)
//     {
//         phy_notch_freq_t notch;
//         notch.freq = para->notch_freq * 3;//3000000;
//         notch.idx = 1;
//         phy_ioctl(PHY_CMD_SET_NOTCH_FREQ, &notch);
//     }
//     #endif
// }
__LOCALTEXT __isr__ uint8_t noise_proc_fn(bpts_t *bpts)
{
	int32_t ret;
    evt_notifier_t ntf[4];
	uint32_t timeout;

    fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, bpts);
    fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR, csma_rx_hd,    bpts);
    fill_evt_notifier(&ntf[2], PHY_EVT_RX_END, csma_rx_end,   bpts);
    fill_evt_notifier(&ntf[3], PHY_EVT_RX_FIN, swc_rx_frame,  bpts);

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
	//printf_s("@in");
    if ((ret = phy_start_cap(&HPLC.hi, &cap, NULL, 0)) != OK) {
        debug_printf(&dty, DEBUG_PHY, "Start cap failed: %d\n", ret);
        goto exit;
    }

	if (is_pulse_noise()) {
		debug_print(&dty, DEBUG_PHY, "Pulse noise detected\n");
		mode = PHY_PERF_MODE_B;
		phy_ioctl(PHY_CMD_SET_PERF_MODE, &mode);
        goto exit;
    }
#endif
	reset_notch_filter();
	if ((ret = phy_start_rx_nbni(&HPLC.hi, test_rx_nbni)) != OK) {
        debug_printf(&dty, DEBUG_PHY, "Start rx nbni failed: %d\n", ret);
		goto exit;
	}
    return 0;

exit:

#if defined(UNICORN2M) || defined(UNICORN8M) || defined(ROLAND1_1M) || defined(ROLAND9_1M) 
	if(HPLC.scanFlag)
	{
		//phy_printf("is_pulse_noise,scanFlag =%d\n",HPLC.scanFlag);
		mode = PHY_PERF_MODE_B;
		phy_ioctl(PHY_CMD_SET_PERF_MODE, &mode);
	}
#endif

	timeout = get_phy_tick_time() + PHY_CIFS;

	if(bpts)
	{
		bpts->time =  timeout + PHY_CIFS;
	
		if(time_after(timeout,bpts->end))
		{
			hplc_do_next();
			return 2;
		}
	}

    ld_set_rx_phase_zc(PHY_PHASE_ALL);
    if((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0)
    {
        printf_s("noise_proc_fn rx:%d\n", ret);
        hplc_do_next();
        return 0;
    }
	return 1;
}

void phy_plc_init(void)
{
	
	phy_dec_clk_t clk = 1;
//	phy_sal_init(GetHplcBand());
    phy_sal_init(2);

	if (phy_ioctl(PHY_CMD_SET_DEC_CLK, &clk) < 0)
		printf_s("phy set dec clk error!\n");



	register_phy_hdr_isr(isr_hdr_plc_rx);

	phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
	
    register_clock_sync_hook(phy_clock_sync, NULL);

    //过零中断注册函数，需要物理层初始化完成(phy_init)之后才能调用。
    register_zero_cross_hook(EVT_ZERO_CROSS0, ZeroCrossISRA, &ZeroData[e_A_PHASE-1]);      //ZEROA
#if defined(ROLAND1_1M) || defined(ROLAND9_1M) || defined(MIZAR1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
#if !defined(ZC3750CJQ2)
    register_zero_cross_hook(EVT_ZERO_CROSS1, ZeroCrossISRA, &ZeroData[e_B_PHASE-1]);      //ZEROB
    register_zero_cross_hook(EVT_ZERO_CROSS2, ZeroCrossISRA, &ZeroData[e_C_PHASE-1]);      //ZEROC
#endif
#endif
#if defined(ROLAND9_1M)|| defined(MIZAR9M) || defined(VENUS8M)
    register_zero_cross_hook(EVT_ZERO_CROSS3, ZeroCrossISR, NULL);      //ZERO

#endif

    Check_ACDC_timer_init();//过零注册完成后启动检测
	
	HPLC.rx.zombie = 1;
    HPLC.rf_rx.zombie = 1;
	//向来:按pb520算出来的， fc+pld+ifs
	//向来:如果算出来的大于15ms就用算出来的值乘以2， 如果小于就用15ms
	HPLC.beacon_dur = PHY_MS2TICK(15);
	HPLC.snid = 0;
	
    HPLC.scanFlag = TRUE;
    HPLC.noise_proc = 0xf;
	HPLC.sfotei = 0xfff;
	if(0)
	{
		HPLC.testmode = RD_CTRL;                //抄控器
		HPLC.tei = CTRL_TEI_GW;					//抄控器
		U32 BaudNum = 9600;
		MeterUartInit(g_meter, BaudNum);
	}
	else
	{
		HPLC.testmode = NORM;
		HPLC.tei = 0;
	}

#if defined(STATIC_MASTER)
    HPLC.next_beacon_start = 0;
	setSwPhase(app_ext_info.func_switch.SwPhase);

#endif
#if MEM_PMA
		HPLC.size_a_min = MEM_SIZE_A;
		HPLC.size_b_min = MEM_SIZE_B;
		HPLC.size_c_min = MEM_SIZE_C;
		HPLC.size_d_min = MEM_SIZE_D;
		HPLC.size_e_min = MEM_SIZE_E;
		HPLC.size_f_min = MEM_SIZE_F;

		HPLC.size_a_max = 0;
		HPLC.size_b_max = 0;
		HPLC.size_c_max = 0;
		HPLC.size_d_max = 0;
		HPLC.size_e_max = 0;
		HPLC.size_f_max = 0;
#endif
    
// #if STATIC_MASTER
// //#if defined(ZC3951CCO)
// //    gpio_pins_off(NULL, LD0_CTRL);
// //#endif
//     hplc_do_next();
// #elif STATIC_NODE
//     //packet_csma_bpts(NULL);
// 	hplc_do_next();
// #endif
	return;
}









































