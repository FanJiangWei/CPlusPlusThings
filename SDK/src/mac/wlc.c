#include "wlc.h"
#include "phy_sal.h"
#include "wl_csma.h"
#include "phy_debug.h"
#include "mem_zc.h"
#include "printf_zc.h"
#include "phy_plc_isr.h"
#include "phy.h"
#include "mac_common.h"
#include "wphy_isr.h"
#include "Datalinkdebug.h"
#include "test.h"
#include "wl_cnst.h"

#if defined(STD_DUAL)

list_head_t WL_BEACON_LIST;
beacon_t *WL_CURR_BEACON;
list_head_t WL_BEACON_FRAME_LIST;
phy_rxtx_cnt_t wphy_rxtx_info = 
{
    .rx_cnt = 0,
    .rx_err_cnt = 0,
    .tx_data_cnt = 0,
    .tx_data_full = 0,
    .tx_mng_cnt = 0,
    .tx_mng_full = 0,
};


int32_t zc_wphy_reset(void)
{
    while (1) {
		uint32_t status = wphy_get_status();
		if (status == WPHY_STATUS_IDLE || status == WPHY_STATUS_WAIT_SIG)
			break;
	}
    return wphy_reset();
}

uint32_t getWphyBifsByPostFlag(bpts_t *bpts)
{
    if (!bpts->post)
    {
        return WPHY_BIFS;
    }

    return WPHY_BIFS * 2;
}

/*******************************************************************************
  * @FunctionName: hplc_hrf_wphy_reset 
  * 优化phy及wphy判定idle和wait状态再进行reset，检测时长控制，不可在接收时reset
  * 判断逻辑不可在中断函数中工作
  * @Author:       wwji
  * @DateTime:     2022年9月27日T11:26:10+0800
  * @Purpose:      
*******************************************************************************/
int32_t hplc_hrf_wphy_reset()
{
#if 0
    zc_wphy_reset();
#else
    int32_t status;
    uint32_t cpu_sr;

    cpu_sr = OS_ENTER_CRITICAL();
    while ((status = wphy_get_status()) != WPHY_STATUS_IDLE
        && status != WPHY_STATUS_WAIT_SIG) {
        OS_EXIT_CRITICAL(cpu_sr);
        sys_delayus(100);
        cpu_sr = OS_ENTER_CRITICAL();
    }

    while (wphy_reset()!= OK) {
        OS_EXIT_CRITICAL(cpu_sr);
        ++HPLC.rf_rx.statis.nr_phy_reset_fail;
        sys_delayus(100);
        cpu_sr = OS_ENTER_CRITICAL();
    }
    OS_EXIT_CRITICAL(cpu_sr);
#endif
    
    return 0;
}
int32_t wl_tdma_recv(bpts_t *bpts)
{
    evt_notifier_t ntf[4];
	int32_t ret;
    uint32_t timeout;

	
    fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, bpts);
    fill_evt_notifier(&ntf[1], WPHY_EVT_PHR,  wl_csma_rx_hd,   bpts);
    fill_evt_notifier(&ntf[2], WPHY_EVT_EORX,  wl_csma_rx_end,  bpts);
    fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN,  wl_swc_rx_frame, bpts);

	if(bpts)
	{
        timeout = bpts->end-WPHY_RIFS;
		//phy_printf("star=0x%08x,end=0x%08x,RIFS=0x%08x,timeout=0x%08x,curt=0x%08x\n",bpts->start,bpts->end,PHY_RIFS,timeout,get_phy_tick_time());
	}
	
	else
        timeout = get_phy_tick_time()+PHY_MS2TICK(20);

	
    if ((ret = wphy_start_rx(NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
		 if (ret == -ETIME)
				++HPLC.rf_rx.statis.nr_late[DT_BEACON];
		 else
         {
            ++HPLC.rf_rx.statis.nr_late[DT_BEACON];

            wphy_trace_show(g_vsh.term);
            wphy_reset();
        }

		 printf_s("tr rfrx ret = %d\n",ret);
		 return -1;
	}	
	return 1;
}

__LOCALTEXT __isr__ int32_t wl_beacon_send(bpts_t *bpts)
{
	list_head_t *pos;
	mbuf_hdr_t *pf;
	uint32_t time,nowtime;
	int32_t ret;
	//beacon_ctrl_t *bc;
	evt_notifier_t ntf;
	//bph_t *bph;

    U32 framedur;
	if (list_empty(&bpts->pf_list))
    {
        wphy_printf("rf bs no beacon \n");
        // return -1;
        return wl_tdma_recv(bpts);
	}
	pos  = bpts->pf_list.next;
	pf   = list_entry(pos, mbuf_hdr_t, link);
	time = bpts->start;
	//bc   = (beacon_ctrl_t *)pf->buf->fi.head;

	nowtime = get_phy_tick_time();
	
	if(time_after(nowtime,time-WPHY_CIFS))
		time = nowtime + WPHY_CIFS;

    if(time_after(nowtime + PHY_US2TICK(800), time))
    {
        time = nowtime + PHY_US2TICK(800);
    }

    framedur = wphy_get_frame_dur(wphy_get_option(), pf->buf->fi.whi.phr_mcs, pf->buf->fi.wpi.pld_mcs
                                         ,wphy_get_pbsz(pf->buf->fi.wpi.blkz), pf->buf->fi.wpi.pbc) + WPHY_CIFS;

    bpts->time =time +framedur;

    if(time_after(bpts->time,bpts->end))
    {	
        list_del(pos);
		zc_free_mem(pf);
        wphy_printf("wlbcn no time\n"
                    "%08x\n"
                    "%08x\n"
                    "%08x\n"
                    "%08x\n"
                    "%08x\n"
                    "%08x\n"
                    , bpts->start
                    , nowtime
                    , time
                    , framedur
                    , bpts->time
                    , bpts->end);
        return -1;
	}	

    // if(HPLC.testmode == WPHYTPT || HPLC.testmode == WPHYCB || HPLC.testmode == MACTPT || HPLC.testmode == RF_PLCPHYCB || HPLC.testmode == PLC_TO_RF_CB)
    // {
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;
    //     return wl_tdma_recv(bpts);
    // }

    // if(!(HPLC.worklinkmode & 0x02))
    // {
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;
    //     return wl_tdma_recv(bpts);
    // }
    if(hrf_can_not_send_message())
    {
        list_del(pos);
        zc_free_mem(pf);
        // return -1;
        return wl_tdma_recv(bpts);
    }



	fill_evt_notifier(&ntf, WPHY_EVT_EOTX, wl_tdma_tx_end, bpts);
	if ((ret = wphy_start_tx(&pf->buf->fi, &time, &ntf, 1)) < 0) {
        printf_s("wl tx beacon ret = %d %s @ %d, (%d %d,%d) fail\n",
                ret,
                phase_str[bpts->phase],
                PHY_TICK2MS(bpts->start),
                PHY_TICK2MS(bpts->beacon->start),
                PHY_TICK2MS(bpts->beacon->end), 
                PHY_TICK2MS(get_phy_tick_time()));	
        if(ret != -ETIME)
        {
            wphy_trace_show(g_vsh.term);
            wphy_reset();
        }
		return -1;
	}
	
	list_del(pos);
    bpts->sent = pf;
	return 1;
}

__LOCALTEXT __isr__ int32_t wl_rbeacon_send(bpts_t *bpts)
{
	uint32_t dur;
	int32_t ret;
	evt_notifier_t ntf;
	mbuf_hdr_t *pf;
/*	beacon_ctrl_t *bc;
#if defined(STD_2016)
	beacon_pld_t *bp;
	bph_t *bph;
#endif*/
	list_head_t *pos;
	list_head_t *n;

	list_for_each_safe(pos,n, &WL_BEACON_FRAME_LIST) {
		pf  = list_entry(pos, mbuf_hdr_t, link);
/*		bc  = (beacon_ctrl_t *)pf->beg;
#if defined(STD_2016)
		bph = (bph_t *)(bc + 1);
		bp  = (beacon_pld_t *)(bph + 1);
		if (bp->bpc == bpts->beacon->bpc)
#else
		if (bc->bpc == bpts->beacon->bpc)
#endif*/
        extern uint32_t   NwkBeaconPeriodCount;
        if(pf->buf->bct == NwkBeaconPeriodCount)
        {
			break;
        }
        else
        {
            list_del(pos);
            zc_free_mem(pf);
        }
	}

    if (pos == &WL_BEACON_FRAME_LIST || bpts->dir != DIR_TX)
    {
        //wphy_printf("bpts->dir = %d\n", bpts->dir);
		return wl_tdma_recv(bpts);
    }
	


    //ld_set_tx_phase(bpts->phase != PHASE_NONE ? 1 << bpts->phase : PHY_PHASE_ALL);


    // if(HPLC.testmode == WPHYTPT || HPLC.testmode == WPHYCB || HPLC.testmode == MACTPT || HPLC.testmode == RF_PLCPHYCB || HPLC.testmode == PLC_TO_RF_CB)
    // {
    //     printf_s("test mode\n");
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;
    //     return wl_tdma_recv(bpts);
    // }

    // if(!(HPLC.worklinkmode & 0x02))
    // {
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;
    //     return wl_tdma_recv(bpts);
    // }

    if(hrf_can_not_send_message())
    {
        list_del(pos);
        zc_free_mem(pf);
        // return -1;
        return wl_tdma_recv(bpts);
    }

    fill_evt_notifier(&ntf, WPHY_EVT_EOTX, wl_tdma_tx_end, bpts);
	
   if(time_after(get_phy_tick_time()+PHY_US2TICK(500),bpts->time))
    {
        bpts->time = get_phy_tick_time()+PHY_US2TICK(500);
    }

    // dur = wphy_mini_frame_dur(wphy_get_option(), pf->buf->fi.whi.phr_mcs) + WPHY_BIFS;
    dur = get_rf_frame_dur(&pf->buf->fi) + WPHY_BIFS;

    if (time_after(bpts->time + dur, bpts->end)) {

         wphy_printf("rb snd no time\n");
         if(pf->buf->cnfrm)
         {
            *(pf->buf->cnfrm->buf)=e_SOLT_ERR;
            pf->buf->cnfrm->msgtype = MAC_CFM;
            post_plc_work(pf->buf->cnfrm);
         }
        
        list_del(pos);
        zc_free_mem(pf);
        return wl_tdma_recv(bpts);
    }

	if ((ret = wphy_start_tx(&pf->buf->fi, &bpts->time, &ntf, 1)) < 0) {
		if (ret == -ETIME)
		{
			bpts->time = get_phy_tick_time()+PHY_US2TICK(500);

			if (time_after(bpts->time + dur, bpts->end)) {
				++HPLC.rf_tx.statis.nr_late[DT_BEACON];
				++HPLC.rf_tx.statis.nr_ts_late[bpts->type];
				printf_s("error rf nr_ts_late %d\n",ret);
                
                list_del(pos);
                zc_free_mem(pf);
                return wl_tdma_recv(bpts);
			}
			if (wphy_start_tx(&pf->buf->fi, &bpts->time, &ntf, 1) == OK)
				goto success;
		}
        else
        {
            wphy_trace_show(g_vsh.term);
            wphy_reset();
        }

		++HPLC.rf_tx.statis.nr_fail[DT_BEACON];

        printf_s("rb tx ret:%d\n", ret);
		return -1;
	}

success:
	++HPLC.rf_tx.statis.nr_ts_frame[bpts->type];
	list_del(pos);
	bpts->sent = pf;
	return 1;
}


/*static*/ U8 setRfChannel()
{
//    printf_s("Mac Set rfband <%d,%d>, <%d,%d>\n", HPLC.option, HPLC.rfchannel, wphy_get_option(), WPHY_CHANNEL);

    U8 chgflag = FALSE;

    if(wphy_get_option() != HPLC.option)
    {
        wphy_set_option(HPLC.option);
        chgflag = TRUE;
    }
    if(/*wphy_get_channel()+1*/WPHY_CHANNEL != HPLC.rfchannel)
    {
        wphy_set_channel(HPLC.rfchannel);
        chgflag = TRUE;
    }

    return chgflag;

}

__LOCALTEXT __isr__ void wl_do_next(void)
{
    list_head_t *pos, *n;
    bpts_t *curr_bpts;
    //#if STATIC_NODE
    evt_notifier_t ntf[4];
    uint32_t timeout;


    HPLC.rf_rx.zombie = 0;

    //#endif
    for (;;)
    {
        if (!WL_CURR_BEACON)
        {
            if (!(WL_CURR_BEACON = hplc_pop_beacon(&WL_BEACON_LIST)))
            {
                // TODO("是否需要在这里切换option 和无线信道");

                if(HPLC.testmode == TEST_MODE)
                {
                    return;
                }

                setRfChannel();
                fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, NULL);
                fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wl_csma_rx_hd, NULL);
                fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wl_csma_rx_end, NULL);
                fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN, wl_swc_rx_frame, NULL);

                wphy_trace_evt(WPHY_TRACE_TRY_RX,
                               &HPLC.whi,
                               NULL,
                               0,
                               (timeout = get_phy_tick_time()),
                               0x11111111,
                               0x11111111);

                //没有beacon的时候其实这个随便设的，最好设成随机的， 如果固定周期跟cco发beacon的周期对上了就不好了
                timeout += HPLC.beacon_dur + PHY_US2TICK(rand() & 0xfffff);
                if (wphy_start_rx(NULL,  &timeout, ntf, NELEMS(ntf)) != OK)
                {
                    ++HPLC.rf_rx.statis.nr_fatal_error;
                    HPLC.rf_rx.zombie = 1;      //do next 调度失败。置zombie标志。规划时隙后可重新启动调度
                    return;
                }
                 HPLC.rf_rx.zombie = 1;         //无时隙接收状态，规划时隙后，需要复位接收机重新开始调度。
                return;
            }

        }
        if (list_empty(&WL_CURR_BEACON->bpts_list))
        {

            if(HPLC.testmode == TEST_MODE)
            {
                return;
            }

            setRfChannel();
            // TODO("是否需要在这里切换option 和无线信道");
            fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, NULL);
            fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wl_csma_rx_hd, NULL);
            fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wl_csma_rx_end, NULL);
            fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN, wl_swc_rx_frame, NULL);

            wphy_trace_evt(WPHY_TRACE_TRY_RX,
                           &HPLC.whi,
                           NULL,
                           0,
                           (timeout = get_phy_tick_time()),
                           0x11111111,
                           0x11111111);

            //没有beacon的时候其实这个随便设的，最好设成随机的， 如果固定周期跟cco发beacon的周期对上了就不好了
            timeout += HPLC.beacon_dur + PHY_US2TICK(rand() & 0xfffff);
            if (wphy_start_rx(NULL,  &timeout, ntf, NELEMS(ntf)) != OK)
            {
                ++HPLC.rf_rx.statis.nr_fatal_error;
                HPLC.rf_rx.zombie = 1;      //do next 调度失败。置zombie标志。规划时隙后可重新启动调度
                return;
            }
            HPLC.rf_rx.zombie = 1;
            return;
        }
        //wphy_printf("HPAV.rx.zombie : %d\n",HPAV.rx.zombie);
        for (pos = WL_CURR_BEACON->curr_pos, n = pos->next; pos != &WL_CURR_BEACON->bpts_list; /*pos = n, WL_CURR_BEACON->curr_pos = pos, n = pos->next*/)
        {
            curr_bpts = list_entry(pos, bpts_t, link);

            // it is used for cco send next beacon and sta have no bpts.

            if(HPLC.testmode == TEST_MODE)
            {
                goto next;
            }

            if (/*curr_bpts->post ||*/ time_after(get_phy_tick_time(), WL_CURR_BEACON->end - PHY_MS2TICK(100)))
            {
                 if(setRfChannel())
                 {
//                    return;       //使用zombie后，return会使调度停止
                 }
            }

            if (WL_CURR_BEACON->next)
            {
#if 1
                zc_free_mem(WL_CURR_BEACON->next);
                WL_CURR_BEACON->next = NULL;
#else
                mbuf_t *buf;
                buf = (mbuf_t *)WL_CURR_BEACON->next->buf;
                buf->stime = get_phy_tick_time();
                if (/*curr_bpts->post ||*/ time_after(buf->stime, WL_CURR_BEACON->end - PHY_MS2TICK(50)))
                {

                    debug_printf(&dty, DEBUG_COORD,"post next of beacon post :%d (%d %d, %d)\n",
                                 curr_bpts->post,
                                 PHY_TICK2MS(WL_CURR_BEACON->start),
                                 PHY_TICK2MS(WL_CURR_BEACON->end),
                                 PHY_TICK2MS(buf->stime));
                    buf->stime = WL_CURR_BEACON->end;

                    post_plc_work(WL_CURR_BEACON->next);

                    WL_CURR_BEACON->next = NULL;
                }
#endif
            }

            if (curr_bpts->sent)
            {
                if(curr_bpts->sent->sendflag)
                {
                    //free mem;
                    //mbuf_put(curr_bpts->sent);
                    if(curr_bpts->sent->buf->cnfrm)
                    {
                        *(curr_bpts->sent->buf->cnfrm->buf)=e_SUCCESS;
                        curr_bpts->sent->buf->cnfrm->msgtype = MAC_CFM;
                        post_plc_work(curr_bpts->sent->buf->cnfrm);
                    }
					//phy_printf("wfree:<%p>\n",(curr_bpts->sent));
                    zc_free_mem(curr_bpts->sent);
                }
                else
                {
					//phy_printf("wback:<%p>\n",(curr_bpts->sent));
                    bpts_pushback_pf(curr_bpts->sent);
                }
				curr_bpts->sent = NULL; 	
            }

            /* life time is over， del entry
			if (!curr_bpts->life--)
				goto next*/
            ;

//            printf_s("curr_bpts->op<%08x> = %s\n", curr_bpts->op, curr_bpts->op == wl_csma_send ? "wl_csma_send" :
//                                                                             curr_bpts->op == wl_tdma_recv ? "wl_tdma_recv" :
//                                                                             #if defined(STATIC_MASTER)
//                                                                             curr_bpts->op == wl_beacon_send ? "wl_beacon_send" :
//                                                                             #else
//                                                                             curr_bpts->op == wl_rbeacon_send ? "wl_rbeacon_send" :
//                                                                             #endif
//                                                                             "UKW");

            /*illegal timeslot*/
            if (time_after(get_phy_tick_time(), curr_bpts->end - getWphyBifsByPostFlag(curr_bpts))){
                goto next;
            }

//            if(time_after(curr_bpts->start, get_phy_tick_time()))
//            {
//                fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, NULL);
//                fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wl_csma_rx_hd, NULL);
//                fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wl_csma_rx_end, NULL);
//                fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN, wl_swc_rx_frame, NULL);

//                wphy_trace_evt(WPHY_TRACE_TRY_RX,
//                               &HPLC.whi,
//                               NULL,
//                               0,
//                               (timeout = get_phy_tick_time()),
//                               0x22222222,
//                               0x22222222);

//                //时隙时间还未到来
//                timeout = curr_bpts->start;
//                if (wphy_start_rx(NULL,  &timeout, ntf, NELEMS(ntf)) != OK)
//                {
//                    ++HPLC.rf_rx.statis.nr_fatal_error;
//                    return;
//                }
//            }

            WL_CURR_BEACON->curr_pos = pos;
            /*发送数据或者接收*/
            if (curr_bpts->op && curr_bpts->op(curr_bpts) > 0) /*csma_send or beacon_send*/
            {
                return;
            }
            
            if (!time_after(get_phy_tick_time(), curr_bpts->end - getWphyBifsByPostFlag(curr_bpts)))        
            {
               continue;        //防止时隙片时间未结束提前删除
            }

        next:
            if(debug_level & DEBUG_WPHY)
            {
                macs_printf("\n<<%3d>>  %s\t %8dms %8dms %8dms %8dms %s\n", ++HPLC.rf_bpts_cnt,
                            TS_BEACON == curr_bpts->type ? "BCN" :
                            TS_TDMA == curr_bpts->type ? "TDM":
                            TS_CSMA == curr_bpts->type   ? "CSM"                                                                                                                                   : "UNK",
                            //phase_str[curr_bpts->phase],
                            PHY_TICK2MS(curr_bpts->start),
                            PHY_TICK2MS(curr_bpts->end - curr_bpts->start),
                            PHY_TICK2MS(curr_bpts->end ),
                            PHY_TICK2MS(get_phy_tick_time()),
                            WL_CURR_BEACON->ext_bpts == 1 ? "#" : "");
            }

            pos = n;
            WL_CURR_BEACON->curr_pos = pos;
            n = pos->next;

            list_del(&curr_bpts->link);
            put_bpts(curr_bpts); /*release mem,or put in CURR_BEACON list*/
        }

        WL_CURR_BEACON->put(WL_CURR_BEACON);
        WL_CURR_BEACON = NULL;
        HPLC.rf_bpts_cnt = 0;
    }
    HPLC.rf_rx.zombie = 1;      //do next 调度失败。置zombie标志。规划时隙后可重新启动调度
    return;
}

/*push beacon solt entry into list,list is empty when zombie is 1.*/
void hplc_push_beacon_wl(beacon_t *beacon)
{
	uint32_t cpu_sr;
    uint32_t last_tick = 0;
    uint32_t status;
    
    debug_printf(&dty, DEBUG_COORD, "wl push beacon %d (%d %d, %d)\n",
		     beacon->nr_bpts,
		     PHY_TICK2MS(beacon->start), PHY_TICK2MS(beacon->end), PHY_TICK2MS(get_phy_tick_time()));
    last_tick = get_phy_tick_time();
//#if MODULE_IOT
	while (1) {
		status = wphy_get_status();
		if (status == WPHY_STATUS_IDLE || status == WPHY_STATUS_WAIT_SIG)
        {      
			break;
        }
        else
        {
            if((PHY_TICK2MS(time_diff(get_phy_tick_time(), last_tick))) > 1000)
            {
                while (wphy_reset() != OK) {
			        ++HPLC.rf_rx.statis.nr_phy_reset_fail;
			        sys_delayms(1);
                }
                last_tick = get_phy_tick_time();
                printf_s("wl beacon wphy_status %d,read %08x %08x %08x\n",status,*(uint8_t *)0xf150020c,*(uint8_t *)0xf1300000, *(uint8_t *)0xf1300008);
			    //printf_s("wl beacon wphy_status %d\n",status);
                extern void wphy_trace_show(tty_t *term);
                wphy_trace_show(g_vsh.term);
                HPLC.rf_rx.zombie = 1;    ////规划时隙异常reset后,需要重新启动调度
            }
        }      
    }
//#endif
	
	// if(WL_CURR_BEACON&&WL_CURR_BEACON->ext_bpts/*&&!beacon->ext_bpts*/)
    #if defined(STATIC_NODE)
    {
        if(beacon->ext_bpts || (WL_CURR_BEACON && !WL_CURR_BEACON->ext_bpts))
        {
            wl_drain_beacon(FALSE);
        }
        else
        {
            wl_drain_beacon(TRUE);
            HPLC.rf_rx.zombie = 1;    //STA 使用，如果是自动规划的时隙，会出现与调度信息不同步的问题。导致rxend操作sent异常复位。
        }
    }
    #else
        wl_drain_beacon(FALSE);   //CCO 删除链表下挂的多余的时隙，当前正在使用的时隙保留
    #endif
    
    cpu_sr = OS_ENTER_CRITICAL();
	list_add_tail(&beacon->link,&WL_BEACON_LIST);
    //OS_EXIT_CRITICAL(cpu_sr);
	//wphy_printf("HPLC.rx.zombie : %d\n",HPLC.rx.zombie);
    // if (HPLC.rf_rx.zombie) {
    //     while (wphy_reset() != OK) {
    //         OS_EXIT_CRITICAL(cpu_sr);
	// 		++HPLC.rf_rx.statis.nr_phy_reset_fail;
	// 		sys_delayms(1);
    //         cpu_sr = OS_ENTER_CRITICAL();
    //     }
    //     wphy_trace_evt(WPHY_TRACE_DO_NEXT,
	// 		      NULL,
	// 		      0,
	// 		      0,
	// 		      get_phy_tick_time(),
	// 		      1,
	// 		      2);
	// 	wl_do_next();
	// }
	OS_EXIT_CRITICAL(cpu_sr);

    // if(status == WPHY_STATUS_IDLE && !HPLC.rf_rx.zombie)
    // {
    //     printf_s("wphy zombie\n");
    //     wphy_trace_show(g_vsh.term);
    // }

    if (HPLC.rf_rx.zombie) {
        if(hplc_hrf_wphy_reset() == 0) 
        {
            wphy_trace_evt(WPHY_TRACE_DO_NEXT,
                    NULL,
                    0,
                    0,
                    get_phy_tick_time(),
                    1,
                    2);
            cpu_sr = OS_ENTER_CRITICAL();
            wl_do_next();
	        OS_EXIT_CRITICAL(cpu_sr);
        }
    }

	return;
}

/**
 * @brief check_Rf_pb_crc
 * @param pf
 * @param CRC32
 * @param pld
 * @param len
 * @return
 */
__LOCALTEXT __isr__ uint8_t check_Rf_pb_crc(phy_frame_info_t *pf, uint8_t *crc32, uint8_t *pld , uint16_t *len)
{
    rf_frame_ctrl_t *head;
    void *payload;
    uint8_t state;
    state = 0;
    payload = pf->payload;
    head = (rf_frame_ctrl_t *)pf->head;

    *crc32 = 1;

    if(head->dt == DT_COORD || head->dt == DT_SACK)
    {
        *crc32 = 0;
        return state;
    }

    if (payload&&pf->length)
    {
        state = pf->wpi.crc24;

        if (!pf->wpi.crc24) {
            if (pf->wpi.crc32_en)
            {
                *crc32 = pf->wpi.crc32;
            }
        } else
        {
//            wphy_printf("1 crc24 %d\n", pf->wpi.crc24);
            state = 1;
        }

        if(pld && len)
        {
            if(head->dt == DT_BEACON)
            {
//                wphy_printf("crc32_en:%d, crc32:%d\n", pf->wpi.crc32_en, pf->wpi.crc32);
                *len = wphy_get_pbsz(pf->wpi.blkz) - 7;
                __memcpy(pld,payload, *len);
            }
            else if(head->dt == DT_SOF)
            {
                if(HPLC.testmode == WPHYTPT)
                {
                    *len = wphy_get_pbsz(pf->wpi.blkz);
                    __memcpy(pld,payload, *len);
                }
                else
                {
                    *len = wphy_get_pbsz(pf->wpi.blkz) - 4;
                    __memcpy(pld,payload+sizeof(wpbh_t), *len);
                }
            }
        }
    }
    else
    {
        wphy_printf("pf->length %d\n", pf->length);
        state = 0x01;
    }


    return state;
}


extern 	void uart1_send(uint8_t *buf, uint32_t len);

/**
 * @brief wl_link_layer_deal
 * @param mbuf
 */
void wl_link_layer_deal(work_t *mbuf)
{
    uint8_t dt, result, crc32;
    uint8_t *pld;
    uint16_t pld_len = 0;
    mbuf_t *buf = (mbuf_t *)mbuf->buf;


    dt = get_rf_frame_dt(buf->fi.head);
    pld = zc_malloc_mem(buf->fi.length, "wdeal", MEM_TIME_OUT);
    result = check_Rf_pb_crc(&buf->fi, &crc32, pld, &pld_len);

    if(dt == DT_SOF)
        crc32 = result;

    wphy_debug_printf("rf(%d,%d) %-9s %03x %03x %-4d %-4d %-4d %-08x %-3d  %-3d   %-3d %08d %08x %08x %-3s\n",
             wphy_get_option(), wphy_get_channel(),
             frame2str(dt), get_rf_frame_stei(buf->fi.head),get_rf_frame_dtei(buf->fi.head), buf->rgain,
             buf->snr, buf->rssi, buf->snid, buf->fi.whi.phr_mcs,  (dt == DT_SACK ? ((rf_sack_ctrl_t *)buf->fi.head)->result : buf->fi.wpi.pld_mcs),
             wphy_get_pbsz( buf->fi.wpi.blkz),  buf->fi.length, buf->stime, get_phy_tick_time(), crc32==0?" ok":"err");
    if(DT_COORD != dt)
    {
        wphy_rxtx_info.rx_cnt ++;
        if(crc32 != 0)
        {
            wphy_rxtx_info.rx_err_cnt ++;
        }
    }
    //退出低功耗
    /*
    if(crc32 == 0)
    {
        if(check_lowpower_states_hook != NULL)
            check_lowpower_states_hook();
    }*/


    //TODO 回传
    if(HPLC.testmode == WPHYCB || HPLC.testmode == RF_PLCPHYCB) //回传模式不处理数据
    {
        goto free;
    }
    else if(HPLC.testmode == WPHYTPT)// 透传模式, 透传校验通过的数据，不透传beacon
    {
        if(/*dt != DT_BEACON &&*/ result == 0)
        {
            uart1_send(buf->fi.head,sizeof(rf_sof_ctrl_t));
            uart1_send((uint8_t *)pld,pld_len);
        }

        goto free;
    }
    else if(MONITOR == HPLC.testmode && (HPLC.worklinkmode & 0x02))
    {
        spy_send(buf, pld, pld_len,result);
        goto free;
    }


    if(!(HPLC.worklinkmode & 0x02))
    {
        goto free;
    }

// #if defined(STATIC_NODE)
//     //更新无线信道参数
//     updateNeiNetRfParam(buf->snid, buf->rfoption, buf->rfchannel);
// #endif


    //数据处理
    switch(dt)
    {
    case DT_BEACON:
        if(pDealBeaconFunc)
            pDealBeaconFunc(buf,pld,pld_len,crc32,NULL);
        break;
    case DT_SOF:
        if(pDealSofFunc)
            pDealSofFunc(buf,pld,pld_len,result,mbuf);
        break;
    case DT_COORD:
        if(pDealCoordFunc)
            pDealCoordFunc(buf,pld,pld_len, 0,NULL);
        break;
    case DT_SACK:
        if(pDealSackFunc)
            pDealSackFunc(buf,pld,pld_len, 0,NULL);
        break;
    default:
        break;
    }

    free:

    zc_free_mem(pld);

}

void wl_pma_init()
{
	INIT_LIST_HEAD(&WL_BEACON_LIST);
	INIT_LIST_HEAD(&WL_BEACON_FRAME_LIST);
	INIT_LIST_HEAD(&WL_TX_MNG_LINK.link);
	WL_TX_MNG_LINK.nr = 0;
	WL_TX_MNG_LINK.thr = 20;
	INIT_LIST_HEAD(&WL_TX_DATA_LINK.link);
	WL_TX_DATA_LINK.nr = 0;
    #if defined(STATIC_MASTER)
	WL_TX_DATA_LINK.thr = 40;
    #else
    WL_TX_DATA_LINK.thr = 20;
    #endif
	INIT_LIST_HEAD(&WL_TX_BCSMA_LINK.link);
	WL_TX_BCSMA_LINK.nr = 0;
	WL_TX_BCSMA_LINK.thr = 5;

    HPLC.hdr_mcs = 4;
    HPLC.pld_mcs = 4;

	WL_CURR_BEACON = NULL;
}

#endif //#if defined(STD_DUAL)

