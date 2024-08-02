/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */
#include "phy.h"
#include "phy_config.h"
#include "phy_debug.h"
#include "framectrl.h"
#include "que.h"
#include "list.h"
#include "plc.h"
#include "plc_cnst.h"
#include "cpu.h"
#include "phy_sal.h"
#include "csma.h"
#include "mem_zc.h"
#include "printf_zc.h"
#include "phy_plc_isr.h"
#include "DataLinkGlobal.h"
#include "Datalinkdebug.h"
#include "test.h"
#include "mac_common.h"


#if defined(STD_DUAL)
#include "wl_csma.h"
#endif

#if defined(STD_DUAL)
check_lowpower_states_fn  check_lowpower_states_hook = NULL;
#endif


double    PHY_SFO_DEF;
PLCDEALFUN_t pDealBeaconFunc;
PLCDEALFUN_t pDealSofFunc;
PLCDEALFUN_t pDealCoordFunc;
PLCDEALFUN_t pDealSackFunc;

plc_post_func pPlcPostMsg;

list_head_t BEACON_LIST;
beacon_t *CURR_BEACON;
list_head_t BEACON_FRAME_LIST;

phy_rxtx_cnt_t phy_rxtx_info = 
{
    .rx_cnt = 0,
    .rx_err_cnt = 0,
    .tx_data_cnt = 0,
    .tx_data_full = 0,
    .tx_mng_cnt = 0,
    .tx_mng_full = 0,
};

void mac_pma_init(void)
{
	mem_pool_init();

	INIT_LIST_HEAD(&BEACON_LIST);
	INIT_LIST_HEAD(&BEACON_FRAME_LIST);
	INIT_LIST_HEAD(&TX_MNG_LINK.link);
	TX_MNG_LINK.nr = 0;
	TX_MNG_LINK.thr = 40;
	INIT_LIST_HEAD(&TX_DATA_LINK.link);
	TX_DATA_LINK.nr = 0;
    #if defined(STATIC_MASTER)
	TX_DATA_LINK.thr = 40;
    #else
    TX_DATA_LINK.thr = 20;
    #endif
	INIT_LIST_HEAD(&TX_BCSMA_LINK.link);
	TX_BCSMA_LINK.nr = 0;
	TX_BCSMA_LINK.thr = 5;

	CURR_BEACON = NULL;
	pbh_cmb = NULL;
	
	pDealBeaconFunc = NULL;
	pDealSofFunc    = NULL;
	pDealCoordFunc  = NULL;
	pDealSackFunc   = NULL;

	pPlcPostMsg = NULL;
	sync_func_deal = NULL;

    pSendBeaconCnt = NULL;

	NeighborDiscovery_link_init();
	#if defined(STATIC_MASTER)
    DeviceListHeaderINIT();
	#endif
	memset(&HPLC,0,sizeof(HPLC_t));
	return;
}


/*******************************************************************************
  * @FunctionName: hplc_hrf_phy_reset 
  * 优化phy及wphy判定idle和wait状态再进行reset，检测时长控制，不可在接收时reset
  * 判断逻辑不可在中断函数中工作
  * @Author:       wwji
  * @DateTime:     2022年9月27日T11:26:10+0800
  * @Purpose:      
*******************************************************************************/
int32_t hplc_hrf_phy_reset()
{
    int32_t status;
    uint32_t cpu_sr;

    cpu_sr = OS_ENTER_CRITICAL();
    while ((status = phy_get_status()) != PHY_STATUS_IDLE
        && status != PHY_STATUS_WAIT_PD) {
        OS_EXIT_CRITICAL(cpu_sr);
        sys_delayus(100);
        cpu_sr = OS_ENTER_CRITICAL();
    }

    while (phy_reset()!= OK) {
        OS_EXIT_CRITICAL(cpu_sr);
        ++HPLC.rx.statis.nr_phy_reset_fail;
        sys_delayus(100);
        cpu_sr = OS_ENTER_CRITICAL();
    }
    OS_EXIT_CRITICAL(cpu_sr);
    
    return 0;
}


int32_t zc_phy_reset(void)
{
    while (1) {
		uint32_t status = phy_get_status();
		if (status == PHY_STATUS_IDLE || status == PHY_STATUS_WAIT_PD)
			break;
	}
    return phy_reset();
}

void put_bpts(bpts_t *curr_bpts)
{
	list_head_t *pos,*n;
	mbuf_hdr_t* mpdu;	
	if(!curr_bpts)
		return;	

	list_for_each_safe(pos, n, &curr_bpts->pf_list) {		
		mpdu = list_entry(pos, mbuf_hdr_t, link);		
		list_del(pos);	
		zc_free_mem(mpdu);
	}
	
    zc_free_mem(curr_bpts);
}

__LOCALTEXT __isr__ void hplc_do_next(void)
{
	list_head_t *pos, *n;
	bpts_t *curr_bpts;
//#if STATIC_NODE
	evt_notifier_t ntf[3];
	uint32_t timeout;
    phy_tgain_t tgain;
    uint32_t end;

    HPLC.rx.zombie = 0;         //进入donext调度，置0。
    
    phy_ioctl(PHY_CMD_GET_TGAIN,&tgain);
//#endif
	for (;;) {
		if (!CURR_BEACON) {
            if (!(CURR_BEACON = hplc_pop_beacon(&BEACON_LIST))) {
                if(HPLC.testmode == TEST_MODE)
                {
                    return;
                }
                if (CURR_B->idx != HPLC.band_idx)
                {
                    phy_band_config(HPLC.band_idx);
                }
                if (tgain.ana != HPLC.ana || tgain.dig != HPLC.dig)
                {
                    phy_gain_config(HPLC.ana,HPLC.dig);
                }
					
				fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, NULL);
				fill_evt_notifier(&ntf[1], PHY_EVT_RX_END,  csma_rx_end,  NULL);
				fill_evt_notifier(&ntf[2], PHY_EVT_RX_FIN,  swc_rx_frame, NULL);
				
				phy_trace_evt(PHY_TRACE_TRY_RX,
					      &HPLC.hi,
					      NULL,
					      0,
					      (timeout = get_phy_tick_time()),
					      0x11111111,
					      0x11111111);
				
				//没有beacon的时候其实这个随便设的，最好设成随机的， 如果固定周期跟cco发beacon的周期对上了就不好了
                timeout += HPLC.beacon_dur + PHY_US2TICK(rand() & 0xfffff);
                ld_set_rx_phase_zc(PHY_PHASE_ALL);
				if (phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf)) != OK) {
					++HPLC.rx.statis.nr_fatal_error;
                    HPLC.rx.zombie = 1;     //do next 调度失败。置zombie标志。规划时隙后可重新启动调度
					return;
				}
                HPLC.rx.zombie = 1;     //此时为无时隙接收状态。规划时隙后，需要复位接收机重新开始有时隙调度
				return;
            }
        }
		//phy_printf("HPAV.rx.zombie : %d\n",HPAV.rx.zombie);
		for (pos = CURR_BEACON->curr_pos, n = pos->next; pos != &CURR_BEACON->bpts_list; /*pos = n, CURR_BEACON->curr_pos = pos, n = pos->next*/) {
			curr_bpts = list_entry(pos, bpts_t, link);

			//it is used for cco send next beacon and sta have no bpts.
            if(HPLC.testmode == TEST_MODE)
            {
                goto next;
            }
			
			if (CURR_BEACON->next) 
			{	
				mbuf_t *buf;
				buf = (mbuf_t *)CURR_BEACON->next->buf;
                buf->stime = get_phy_tick_time();

                if (/*curr_bpts->post || */time_after(buf->stime, CURR_BEACON->end - PHY_MS2TICK(100)))
				{ 				

                        debug_printf(&dty, DEBUG_COORD,"post next of beacon post :%d (%d %d, %d), tick %08x\n",
						curr_bpts->post,
						PHY_TICK2MS(CURR_BEACON->start), 
						PHY_TICK2MS(CURR_BEACON->end),	
                        PHY_TICK2MS(buf->stime),
                        get_phy_tick_time());
                        buf->stime = CURR_BEACON->end;
                        //phy_printf("nt bcn\n");
                        CURR_BEACON->next->msgtype = NXT_BPTS;
						post_plc_work(CURR_BEACON->next);

                        CURR_BEACON->next = NULL;

                        if (CURR_B->idx != HPLC.band_idx)
                        {
                            phy_band_config(HPLC.band_idx);
//                            return;       //使用zombie后，return会使调度停止
                        }
                        if (tgain.ana != HPLC.ana || tgain.dig != HPLC.dig)
                        {
                            phy_gain_config(HPLC.ana,HPLC.dig);
                        }
				}			
			}
			
			if (curr_bpts->sent) 
			{			
                if(curr_bpts->sent->sendflag)
                {
				//mbuf_put(curr_bpts->sent);
                    if(curr_bpts->sent->buf->cnfrm)
                    {	
                        *(curr_bpts->sent->buf->cnfrm->buf)=e_SUCCESS;
                        curr_bpts->sent->buf->cnfrm->msgtype = MAC_CFM;
                        post_plc_work(curr_bpts->sent->buf->cnfrm);
                    }
				    zc_free_mem(curr_bpts->sent);
                }
                else
                {
                    bpts_pushback_pf(curr_bpts->sent);
                }
				curr_bpts->sent = NULL; 	
			}

			/* life time is over， del entry
			if (!curr_bpts->life--)
                goto next*/;
			/*illegal timeslot*/
            if(curr_bpts->txflag&&curr_bpts->type == TS_BEACON)
            {
                end = BCOLT_STOP_EARLY*PHY_CIFS;
            }
            else
            {
                end = PHY_CIFS;
            }
            if (time_after(get_phy_tick_time(), curr_bpts->end - end))
			{
				goto next;
			}
            CURR_BEACON->curr_pos = pos;
//            printf_s("curr_bpts->op<%08x> = %s\n", curr_bpts->op, curr_bpts->op == csma_send ? "csma_send" :
//                                                                             curr_bpts->op == tdma_recv ? "tdma_recv" :
//                                                                             #if defined(STATIC_MASTER)
//                                                                             curr_bpts->op == beacon_send ? "beacon_send" :
//                                                                             #else
//                                                                             curr_bpts->op == rbeacon_send ? "rbeacon_send" :
//                                                                             #endif
//                                                                             "UKW");
			if(!check_bpts_op((void *)curr_bpts->op))
            {
                // U8 a = 2;
			    // a = a/(a - a);
                // return;
                printf_s("op:%p, err\n", curr_bpts->op);
            }
			/*发送数据或者接收*/
			if (curr_bpts->op && curr_bpts->op(curr_bpts) > 0)/*csma_send or beacon_send*/	
            {
                return;
            }
            
            if (!time_after(get_phy_tick_time(), curr_bpts->end - end))
			{
                continue;       //防止时隙片时间未结束提前删除
            }
			
        next:
            if(debug_level & DEBUG_PHY)
            {
                macs_printf("\n<<%3d>>  %s\t %s\t %8dms  -%8dms- %8dms -%8dus %s\n\n",++HPLC.plc_bpts_cnt,
                            TS_BEACON==curr_bpts->type?"BCN":TS_TDMA==curr_bpts->type?"TDM":
                            TS_CSMA==curr_bpts->type?"CSM":TS_BCSMA==curr_bpts->type?"BCS":"UNK",
                            phase_str[curr_bpts->phase],
                            PHY_TICK2MS(get_phy_tick_time()),
                            //PHY_TICK2MS(curr_bpts->start),
                            PHY_TICK2MS(curr_bpts->end-curr_bpts->start),
                            PHY_TICK2MS(curr_bpts->end),
                            PHY_TICK2US(curr_bpts->end-get_phy_tick_time()),
                            CURR_BEACON->ext_bpts==1?"#":"");
            }
            
            pos = n;
            CURR_BEACON->curr_pos = pos; 
            n = pos->next;
            
			list_del(&curr_bpts->link);
            put_bpts(curr_bpts);/*release mem,or put in CURR_BEACON list*/
		}
		CURR_BEACON->put(CURR_BEACON);
		CURR_BEACON = NULL;
        HPLC.plc_bpts_cnt = 0;
	}
    HPLC.rx.zombie = 1;     //do next 调度失败。置zombie标志。规划时隙后可重新启动调度
	return;
}

/*push beacon solt entry into list,list is empty when zombie is 1.*/
void hplc_push_beacon(beacon_t *beacon)
{
	uint32_t cpu_sr;
    uint32_t last_tick = 0;
    uint32_t status;
    //uint32_t read_01 = 0;
    //uint32_t read_02 = 0;

	debug_printf(&dty, DEBUG_COORD, "push beacon %d (%d %d, %d)\n",
		     beacon->nr_bpts,
		     PHY_TICK2MS(beacon->start), PHY_TICK2MS(beacon->end), PHY_TICK2MS(get_phy_tick_time()));
    last_tick = get_phy_tick_time();
//#if MODULE_IOT
	while (1) {
		status = phy_get_status();
		if (status == PHY_STATUS_IDLE || status == PHY_STATUS_WAIT_PD)
        {      
			break;
        }
        else
        {
            if((PHY_TICK2MS(time_diff(get_phy_tick_time(), last_tick))) > 1000)
            {
                while (phy_reset() != OK) {
        			++HPLC.rx.statis.nr_phy_reset_fail;
        			sys_delayms(1);
                }
                last_tick = get_phy_tick_time();
                //memcpy(&read_01,(uint8_t *)0xf11001f0,sizeof(read_01));
                //memcpy(&read_02,(uint8_t *)0xf11001f4,sizeof(read_02));
			    printf_s("plc beacon phy_status %d,read %08x  %08x\n",status,*(uint8_t *)0xf11001f0,*(uint8_t *)0xf11001f4);
                extern void phy_trace_show(tty_t *term);
                phy_trace_show(g_vsh.term);
                HPLC.rx.zombie = 1;    //规划时隙异常reset后,需要重新启动调度
            }
        }      
        //sys_delayus(100);
    }
//#endif
	
    
	// if(CURR_BEACON&&CURR_BEACON->ext_bpts/*&&!beacon->ext_bpts*/)
    #if defined(STATIC_NODE)
    {
        if(beacon->ext_bpts || ( CURR_BEACON && !CURR_BEACON->ext_bpts))
        {
            hplc_drain_beacon(FALSE);
        }
        else
        {
            hplc_drain_beacon(TRUE);
            HPLC.rx.zombie = 1;    //STA 使用，如果是自动规划的时隙，会出现与调度信息不同步的问题。导致rxend操作sent异常复位。
        }
	}
    #else
        hplc_drain_beacon(FALSE);   //CCO 删除链表下挂的多余的时隙，当前正在使用的时隙保留
    #endif

    cpu_sr = OS_ENTER_CRITICAL();
	list_add_tail(&beacon->link,&BEACON_LIST);
	//phy_printf("HPLC.rx.zombie : %d\n",HPLC.rx.zombie);
    //OS_EXIT_CRITICAL(cpu_sr);
//     uint32_t cnt = 0;
//     if (HPLC.rx.zombie) {
// 		while (phy_reset() != OK) {
            
// 	        OS_EXIT_CRITICAL(cpu_sr);

// 			++HPLC.rx.statis.nr_phy_reset_fail;
// 			sys_delayms(1);
//             if(cnt++ > 2000)
//                 printf_s("fck:%d\n",cnt); 

//             cpu_sr = OS_ENTER_CRITICAL(); 
//         }
// 		phy_trace_evt(PHY_TRACE_DO_NEXT,
// 			      NULL,
// 			      0,
// 			      0,
// 			      get_phy_tick_time(),
// 			      1,
// 			      2);

// //        phy_trace_show(g_vsh.term);
// 		hplc_do_next();
// 	}
	OS_EXIT_CRITICAL(cpu_sr);

    // if(status == PHY_STATUS_IDLE && !HPLC.rx.zombie)
    // {
    //     printf_s("phy zombie\n");
    //     phy_trace_show(g_vsh.term);
    // }

    if (HPLC.rx.zombie) 
    {
        if(hplc_hrf_phy_reset() == 0)
        {
            phy_trace_evt(PHY_TRACE_DO_NEXT,
			      NULL,
			      0,
			      0,
			      get_phy_tick_time(),
			      1,
			      2);
            cpu_sr = OS_ENTER_CRITICAL();
            hplc_do_next();
	        OS_EXIT_CRITICAL(cpu_sr);
        }
    }
	

	return;
}

#if STATIC_MASTER
__LOCALTEXT __isr__ int32_t beacon_send(bpts_t *bpts)
{
    list_head_t *pos;
    mbuf_hdr_t *pf;
    uint32_t time,nowtime;
    int32_t ret;
    //beacon_ctrl_t *bc;
    evt_notifier_t ntf;
    //bph_t *bph;
    if (list_empty(&bpts->pf_list))
    {
//        phy_printf("bs no beacon \n");
        // return -1;
        return tdma_recv(bpts);
    }
    pos  = bpts->pf_list.next;
    pf   = list_entry(pos, mbuf_hdr_t, link);
    time = bpts->start;
    //bc   = (beacon_ctrl_t *)pf->buf->fi.head;

    nowtime = get_phy_tick_time();

    if(time_after(nowtime,time-PHY_CIFS))
        time = nowtime + PHY_CIFS;

    bpts->time = time + get_frame_dur(&pf->buf->fi) + PHY_CIFS;

    if(time_after(bpts->time,bpts->end))
    {
        list_del(pos);
        zc_free_mem(pf);
        return -1;
    }

    if(CURR_B->idx != pf->buf->bandidx)
    {
        printf_s("bcn band err:%d,%d\n",CURR_B->idx,pf->buf->bandidx);

        list_del(pos);
        zc_free_mem(pf);
        // return -1;        
        return tdma_recv(bpts);
    }

    macs_printf("phase : %d\n",1<<bpts->phase);

    // if(HPLC.testmode == PHYTPT || HPLC.testmode == PHYCB || HPLC.testmode == MACTPT || HPLC.testmode == RF_PLCPHYCB || HPLC.testmode == PLC_TO_RF_CB)
    // {
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;       
    //     return tdma_recv(bpts);
    // }

    // if(!(HPLC.worklinkmode & 0x01))
    // {
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;     
    //     return tdma_recv(bpts);
    // }

    if(plc_can_not_send_message())
    {
        list_del(pos);
        zc_free_mem(pf);
        // return -1;     
        return tdma_recv(bpts);
    }



    fill_evt_notifier(&ntf, PHY_EVT_TX_END, tdma_tx_end, bpts);
    ld_set_tx_phase_zc(1<<bpts->phase);
    if ((ret = phy_start_tx(&pf->buf->fi, &time, &ntf, 1)) < 0) {
            printf_s("tx beacon ret = %d %s @ %d, (%d %d,%d) fail\n",
                 ret,
                 phase_str[bpts->phase],
                 PHY_TICK2MS(bpts->start),
                 PHY_TICK2MS(bpts->beacon->start),
                 PHY_TICK2MS(bpts->beacon->end),
                 PHY_TICK2MS(get_phy_tick_time()));

        if(ret != -ETIME)
        {
            phy_trace_show(g_vsh.term);
            phy_reset();
        }
        
        return -1;
    }

    list_del(pos);
    bpts->sent = pf;
    return 1;
}
#endif

int32_t tdma_recv(bpts_t *bpts)
{
    evt_notifier_t ntf[4];
    int32_t ret;
    uint32_t timeout;


    fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, bpts);
    fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR,  csma_rx_hd,   bpts);
    fill_evt_notifier(&ntf[2], PHY_EVT_RX_END,  csma_rx_end,  bpts);
    fill_evt_notifier(&ntf[3], PHY_EVT_RX_FIN,  swc_rx_frame, bpts);

    if(bpts)
    {
        if(bpts->txflag)
        {
            timeout = bpts->end - PHY_RIFS*BCOLT_STOP_EARLY;
        }
        else
        {
            timeout = bpts->end-PHY_RIFS;
        }
        //phy_printf("star=0x%08x,end=0x%08x,RIFS=0x%08x,timeout=0x%08x,curt=0x%08x\n",bpts->start,bpts->end,PHY_RIFS,timeout,get_phy_tick_time());
    }
    else
    {
        timeout = get_phy_tick_time()+PHY_MS2TICK(20);
    }
        
    

    ld_set_rx_phase_zc(TO_PHY_PHASE(bpts == NULL?PHY_PHASE_ALL :bpts->phase));
    if ((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
         if (ret == -ETIME)
         {
            ++HPLC.rx.statis.nr_late[DT_BEACON];
         }
         else
         {
            ++HPLC.rx.statis.nr_late[DT_BEACON];
            phy_trace_show(g_vsh.term);
            phy_reset();
         }

         macs_printf("ptr rx ret = %d,%dus\n",ret,PHY_TICK2US(timeout));
         return -1;
    }
    return 1;
}
/*void update_rbeacon_frame(bpts_t *bpts, mbuf_hdr_t *pf)
{

}*/


__LOCALTEXT __isr__ int32_t rbeacon_send(bpts_t *bpts)
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
    list_head_t *pos, *n;

    list_for_each_safe(pos, n, &BEACON_FRAME_LIST) {
        pf  = list_entry(pos, mbuf_hdr_t, link);
/*		bc  = (beacon_ctrl_t *)pf->beg;
#if defined(STD_2016)
        bph = (bph_t *)(bc + 1);
        bp  = (beacon_pld_t *)(bph + 1);
        if (bp->bpc == bpts->beacon->bpc)
#else
        if (bc->bpc == bpts->beacon->bpc)
#endif*/
        extern U32   NwkBeaconPeriodCount;
        if(pf->buf->bct == NwkBeaconPeriodCount)
        {
            break;
        }
        else
        {
            list_del(pos);
            zc_free_mem(pf);
            //pos 被del掉之后prev 和 next 都被置为空!!!
            //pos = &BEACON_FRAME_LIST;
        }
    }

    if (pos == &BEACON_FRAME_LIST || bpts->dir != DIR_TX)
        return tdma_recv(bpts);



    //ld_set_tx_phase(bpts->phase != PHASE_NONE ? 1 << bpts->phase : PHY_PHASE_ALL);


    // if(HPLC.testmode == PHYTPT || HPLC.testmode == PHYCB || HPLC.testmode == MACTPT || HPLC.testmode == RF_PLCPHYCB || HPLC.testmode == PLC_TO_RF_CB)
    // {
    //     printf_s("test mode\n");
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;
    //     return tdma_recv(bpts);
    // }

    // if(!(HPLC.worklinkmode & 0x01))
    // {
    //     list_del(pos);
    //     zc_free_mem(pf);
    //     // return -1;
    //     return tdma_recv(bpts);
    // }

    if(plc_can_not_send_message())
    {
        list_del(pos);
        zc_free_mem(pf);
        // return -1;
        return tdma_recv(bpts);
    }

    fill_evt_notifier(&ntf, PHY_EVT_TX_END, tdma_tx_end, bpts);

    ld_set_tx_phase_zc(PHY_PHASE_ALL);
   if(time_after(get_phy_tick_time()+PHY_US2TICK(250),bpts->time))
    {
        bpts->time = get_phy_tick_time()+PHY_US2TICK(250);
    }

    dur  = PREAMBLE_DUR + FRAME_CTRL_DUR(CURR_B->symn)
            + pld_sym_to_time(pf->buf->fi.pi.symn) + PHY_BIFS;

    if (time_after(bpts->time + dur, bpts->end)) {

        if(pf->buf->cnfrm)
        {
            *(pf->buf->cnfrm->buf)=e_SOLT_ERR;
//            pf->buf->cnfrm->msgtype = MAC_CFM;
            post_plc_work(pf->buf->cnfrm);
        }
        
        list_del(pos);
        zc_free_mem(pf);
        return tdma_recv(bpts);
    }

    if ((ret = phy_start_tx(&pf->buf->fi, &bpts->time, &ntf, 1)) < 0) {
        if (ret == -ETIME)
        {
            bpts->time = get_phy_tick_time()+PHY_US2TICK(250);

            dur  = PREAMBLE_DUR + FRAME_CTRL_DUR(CURR_B->symn)
                + pld_sym_to_time(pf->buf->fi.pi.symn) + PHY_BIFS;

            if (time_after(bpts->time + dur, bpts->end)) {
                ++HPLC.tx.statis.nr_late[DT_BEACON];
                ++HPLC.tx.statis.nr_ts_late[bpts->type];
                printf_s("error nr_ts_late %d\n",ret);

                list_del(pos);
                zc_free_mem(pf);
                return tdma_recv(bpts);
            }
            if (phy_start_tx(&pf->buf->fi, &bpts->time, &ntf, 1) == OK)
                goto success;
        }
        else
        {
            phy_trace_show(g_vsh.term);
            phy_reset();
        }

        ++HPLC.tx.statis.nr_fail[DT_BEACON];

        return -1;
    }

success:
    ++HPLC.tx.statis.nr_ts_frame[bpts->type];
    list_del(pos);
    bpts->sent = pf;
    return 1;
}


extern 	void uart1_send(uint8_t *buf, uint32_t len);



pbh_cmb_t *pbh_cmb;

U8 sof_cmb(pbh_t *pbh, uint16_t pbsz, uint8_t crc)
{
    //printf_s("crc=%d,ssn=%d,mfsf=%d,mfef=%d\n",crc,pbh->ssn, pbh->mfsf,pbh->mfef);
    if(crc)        //crc错误不进入拼包流程
        return FALSE;
    if(/*pbh->som && pbh->eom && */(!pbh->mfsf || !pbh->mfef))//单独一个pb,并且有组包需求
	{
		if(pbh->mfsf)//第一包，需要组包
		{
			if(pbh_cmb)
				zc_free_mem(pbh_cmb);
										
            pbh_cmb = zc_malloc_mem(sizeof(pbh_cmb_t)+4*pb_bufsize(pbsz),"cmb",MEM_TIME_OUT);
			pbh_cmb->mfsf = 1;
			pbh_cmb->mfef = 0;
			pbh_cmb->ssn = 0;
            pbh_cmb->pbsz = pbsz;
			pbh_cmb->copy = 1; //需要copy到当前结构体;
			pbh_cmb->xcmb = 0;
			pbh_cmb->len = 0;
		}
		else//后续包
		{
			if(pbh_cmb)//等待后续帧
			{
                if((pbh_cmb->pbsz == pbsz)&&(pbh_cmb->ssn+1 == pbh->ssn))//bpsz一致，序号累加，是期望的下一帧。
				{
                    pbh_cmb->ssn = pbh->ssn;
					pbh_cmb->mfef = pbh->mfef;
					pbh_cmb->copy = 1;
				}
				else //非期望的后续帧
				{
					pbh_cmb->xcmb = 1;
				}
			}
			else//未收到第一帧，收到了后续帧
			{
				pbh_cmb = zc_malloc_mem(sizeof(pbh_cmb_t),"xcmb",MEM_TIME_OUT);
                pbh_cmb->xcmb = 1;
//                phy_printf("dont recv first bp , ssn = %d\n", pbh->ssn);
			}
		}
        return TRUE;
	}

    return FALSE;

}

void link_layer_deal(work_t *mbuf)
{
	uint8_t dt,result,crc32;
    uint32_t *pld;
	uint16_t pld_len;	
	mbuf_t *buf;
    U8 packegeFlag;
	buf = (mbuf_t *)mbuf->buf;
	pld_len = 0;
	/*if(buf->snid !=0x00000003 && buf->snid!=0)
	{
		return;
    }*/
	dt = get_frame_dt(buf->fi.head);
	pld = zc_malloc_mem(buf->fi.length,"deal",MEM_TIME_OUT);
    result = check_pb_crc(&buf->fi,&crc32,pld,&pld_len,&packegeFlag);
#if 0	
	uint8_t ext_sub;
	if(dt == DT_SACK)
		ext_sub = get_sack_ext_dt(buf->fi.head);
	phy_printf("@ %4d %s ",buf->rgain,
			DT_BEACON==dt?"beacon":
			DT_SOF==dt?"sof":
			DT_COORD==dt?"coord":
			DT_SACK==dt?(ext_sub==0?"sack":ext_sub==1?"srch":ext_sub==2?"syn":"uSack"):
			"unk");
	dump_zc(0,": ",DEBUG_PHY,buf->fi.head,sizeof(frame_ctrl_t));
	phy_printf("%4d pb : %x pcbcrc : %s , pldcrc32 : %s -> ",buf->fi.length,result>>4,(result&0x0f) == 0?"ok":"err",crc32==0?"ok":"err");
	//dump_zc(1,"pld: \n",DEBUG_PHY,mbuf->buf->fi.payload,mbuf->buf->fi.length);		
#endif
    /*组包的数据只有所有的pb都有正确的校验，才会交给net处理*/
    if(pbh_cmb && packegeFlag)
    {
        if(pbh_cmb->xcmb)//未按照帧序号收到后续帧。
        {
//            phy_printf("pbh_cmb = %08x\n", pbh_cmb);
            zc_free_mem(pbh_cmb);
            pbh_cmb = NULL;
            goto free;
        }
        if(pbh_cmb->copy)//有拷贝需求
        {
            if(!(result&0x0f))//校验正确
            {
                __memcpy(pbh_cmb->data+pbh_cmb->len,pld,pld_len);
                pbh_cmb->len += pld_len;

                zc_free_mem(pld);

                if(pbh_cmb->mfef)//组包结束，变更长度。
                {
                    pld_len = pbh_cmb->len;
                    pld = zc_malloc_mem(pld_len,"cmbv",MEM_TIME_OUT);
                    __memcpy(pld,pbh_cmb->data,pld_len);

                    zc_free_mem(pbh_cmb);
                    pbh_cmb = NULL;
                }
                else
                {
                    //等待后续包
                    return;
                }
            }
            else//校验不正确，全部清空
            {
                zc_free_mem(pbh_cmb);
                pbh_cmb = NULL;
            }
        }
        else//收到了没有组包需求的数据。
        {
            //正常处理。
        }

        //net_printf("\n\npbh_cmb\n\n\n");
    }


	/*else if(MACTPT == HPLC.testmode)
	{
		//按照数据mpdu长度透传传数据，长度需修改。
		
		uart1_send((uint8_t *)pld,pld_len);
		goto free;
	}*/

	
	if (dt == DT_COORD)
	{
		coord_ctrl_t *coord =  (coord_ctrl_t *)buf->fi.head;
		phy_printf("rx coord  dur: %x, bso: %x\n", coord->dur, coord->bso);
	}
	else
	{
        if(dt == DT_SOF)
        {
            crc32 = result&0x0f;
        }


        if(dt == DT_SOF)
        {
            phy_printf("rx %d %-9s %-04x(%03x) %-4d 0x%08x %-3d %-3d %-3d %-5d %08x %08x %-3s\n",
                       buf->phase/*buf->bandidx*/, frame2str(dt), get_frame_stei(buf->fi.head), get_frame_dtei(buf->fi.head), buf->rgain,
                       buf->snid, get_frame_symn(buf->fi.head), get_frame_pbc(buf->fi.head),
                       get_frame_tmi(buf->fi.head), buf->fi.length, buf->stime, get_phy_tick_time(), crc32==0?" ok":"err");
//            dump_level_buf(DEBUG_PHY, buf->fi.head, sizeof(frame_ctrl_t));
        }
        else if(dt == DT_BEACON)
        {
            phy_printf("rx %d %-9s %-04x %-4d 0x%08x %-3d %-3d %-3d %-5d %08x %08x %-3s\n",
                       ((beacon_ctrl_t *)buf->fi.head)->phase/*buf->bandidx*/,frame2str(dt), get_frame_stei(buf->fi.head), buf->rgain,
                       buf->snid, get_frame_symn(buf->fi.head), get_frame_pbc(buf->fi.head),
                       get_frame_tmi(buf->fi.head), buf->fi.length, buf->stime, get_phy_tick_time(), crc32==0?" ok":"err");
        }
		else
        phy_printf("rx %-9s %-04x %-04x %-4d 0x%08x %-3d %-3d %-3d %-5d %08x %08x %-3s\n",
            frame2str(dt), get_frame_stei(buf->fi.head), get_frame_dtei(buf->fi.head),buf->rgain,
			buf->snid, get_frame_symn(buf->fi.head), get_frame_pbc(buf->fi.head),
		    get_frame_tmi(buf->fi.head), buf->fi.length, buf->stime, get_phy_tick_time(), crc32==0?" ok":"err");
        
        phy_rxtx_info.rx_cnt ++;
        if(crc32 != 0)
        {
            phy_rxtx_info.rx_err_cnt ++;
        }
        //退出低功耗
        /*
        if(crc32 == 0)
        {
            if(check_lowpower_states_hook != NULL)
                check_lowpower_states_hook();
        }*/
			
	}

	

#if 0
		phy_printf("pld len : %d\n",pld_len);
		dump_zc(1,"pld deal: \n",DEBUG_PHY,(uint8_t *)pld,pld_len); 	
#endif
	
        if(PHYCB == HPLC.testmode || RF_PLCPHYCB == HPLC.testmode)
		{
			goto free;
			return;
		}
#if defined(STD_2016)
		else if(PHYTPT == HPLC.testmode )
		{
			if((result&0xf) == 0 && dt != DT_BEACON)
			{
			    uart1_send(buf->fi.head,sizeof(sof_ctrl_t));
				uart1_send((uint8_t *)pld,pld_len);
			}
            if(dt == DT_BEACON)
            {
                HPLC.noise_proc++;//送检时可以进行噪声检测
            }

			goto free;
        }
        else if(HPLC.testmode == PLC_TO_RF_CB)          //载波到rf的回传模式。
        {
    #if defined(STD_DUAL)
            plc_to_rf_testmode_deal(result,buf->fi.head, (uint8_t *)pld, pld_len);
            goto free;
    #endif
        }
        else if(MONITOR == HPLC.testmode && (HPLC.worklinkmode & 0x01))
        {
            spy_send(buf, (uint8_t *)pld, pld_len, crc32);
            goto free;
        }
#endif
//    if(CURR_B->idx != buf->bandidx)
//    {
//        net_printf("it's err band data\n");
//        goto free;
//    }


    if(!(HPLC.worklinkmode & 0x01))
    {
        goto free;
    }

    switch(dt)
    {
    case DT_BEACON:
//        mbuf->msgtype = PROC_BEACON;
        mg_update_type(PROC_BEACON, mbuf->record_index);
        if(pDealBeaconFunc)
            pDealBeaconFunc(buf,pld,pld_len,crc32,NULL);
        break;
    case DT_SOF   :
//        mbuf->msgtype = PROC_SOF;
        mg_update_type(PROC_SOF, mbuf->record_index);
        if(pDealSofFunc)
            pDealSofFunc(buf,pld,pld_len,result&0x0f,mbuf);
        break;
    case DT_COORD :
//        mbuf->msgtype = PROC_COORD;
        mg_update_type(PROC_COORD, mbuf->record_index);
        if(pDealCoordFunc)
            pDealCoordFunc(buf,pld,pld_len, 0,NULL);
        break;
    case DT_SACK  :
//        mbuf->msgtype = PROC_SACK;
        mg_update_type(PROC_SACK, mbuf->record_index);
        if(pDealSackFunc)
            pDealSackFunc(buf,pld,pld_len, 0,NULL);
        break;
    default:
        phy_printf("link_layer_deal dt = %d \n");
        break;
    }

	free:
	zc_free_mem(pld);
}

void register_plc_deal_Func(uint8_t type, PLCDEALFUN_t func)
{
    DT_BEACON==type?pDealBeaconFunc = func:
    DT_SOF==type?pDealSofFunc = func:
    DT_COORD==type?pDealCoordFunc = func:
    DT_SACK==type?pDealSackFunc = func: phy_printf("regist_plc_deal_Func type = %d  Error!\n");
}






















