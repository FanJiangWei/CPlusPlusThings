#include "wl_csma.h"
#include "phy_sal.h"
#include "wlc.h"
#include "wphy_isr.h"
#include "wl_cnst.h"
#include "printf_zc.h"
#include "mem_zc.h"

#include "csma.h"
#include "app_commu_test.h"

#include "phy.h"
#include "phy_debug.h"


#if defined(STD_DUAL)

tx_link_t WL_TX_MNG_LINK;
tx_link_t WL_TX_DATA_LINK;
tx_link_t WL_TX_BCSMA_LINK;

/**************************************************************
 * 为无线部分增加接口
 * 
 * 
 *************************************************************/



__LOCALTEXT __isr__ mbuf_hdr_t * wl_bpts_get_pf(bpts_t *bpts)
{
	mbuf_hdr_t *mpdu;
	tx_link_t *link;
	uint32_t stime,etime;
	uint32_t random;
	uint8_t option = wphy_get_option();


	
	if(bpts->type == TS_BCSMA){
		/*if(list_empty(&bpts->pf_list))
			return NULL;
		mpdu = list_entry(bpts->pf_list.next,mbuf_hdr_t,link);
		list_del(&mpdu->link);*/
		if(!list_empty(&bpts->pf_list)){
			mpdu = list_entry(bpts->pf_list.next,mbuf_hdr_t,link);
			list_del(&mpdu->link);
			return mpdu;
		}
		link = &WL_TX_BCSMA_LINK;
		if(!(mpdu = tx_link_deq(link, PHASE_NONE)))
				return NULL;		
	}else{
		if(!list_empty(&bpts->pf_list)){
			mpdu = list_entry(bpts->pf_list.next,mbuf_hdr_t,link);
			list_del(&mpdu->link);
			return mpdu;
		}
		
		link = &WL_TX_MNG_LINK;
		if(!(mpdu = tx_link_deq(link, PHASE_NONE))){
			link = &WL_TX_DATA_LINK;
			if(!(mpdu = tx_link_deq(link, PHASE_NONE)))
				return NULL;
		}

	}


	stime = bpts->time;
	
	//wphy_printf("\nretrans : %d\n",mpdu->buf->retrans);

	if(bpts->type == TS_CSMA || bpts->type == TS_BCSMA){
		random   = backoff_mask(mpdu->buf->backoff,mpdu->buf->priority);
        if(random !=0 )
        {
			//printf_s("<%d>",random);
            bpts->time += (random*4*PMB_1SYM_DUR);
        }
	}

	etime = bpts->time + get_rf_frame_dur(&mpdu->buf->fi) + (mpdu->buf->acki ? (WPHY_RIFS_MAX + wphy_sack_frame_dur(option, mpdu->buf->fi.whi.phr_mcs)):WPHY_CIFS);
	
	if(time_after(etime,bpts->end)){
		//printf_d("no time!\n");
		bpts->time = stime;
		bpts_pushback_pf(mpdu);
		return NULL;
	}	
	/*if(mpdu->buf->ufs)
		update_frame_sof(mpdu);*/


		
	return mpdu;
}


/*csma_abandon是启动有超时时间的接收， 超时时间一般是设成扩展IFS， 
如果距离当前bpts->end不足这个时间就设置成bpts->end - ifs.
bpts->time 更新到超时时间+ifs*/
__LOCALTEXT __isr__ int32_t wl_csma_abandon(bpts_t *bpts)
{
    evt_notifier_t ntf[4];
	int32_t ret,j;
	j=0;
	uint32_t timeout;
    //uint32_t now = get_phy_tick_time();

	
	timeout =get_phy_tick_time() + WPHY_EIFS;
	if(bpts)
	{
		if(time_after(get_phy_tick_time(),bpts->end-getWphyBifsByPostFlag(bpts)))
		{
			//hplc_do_next();
			return -1;
		}
		
        if(time_after(timeout,bpts->end-getWphyBifsByPostFlag(bpts)))
		{
            timeout = bpts->end - getWphyBifsByPostFlag(bpts);
            if(time_after(get_phy_tick_time(),timeout))
			{
				//hplc_do_next();
				return -1;
            }
		}
		
        bpts->time = timeout + WPHY_CIFS;
	}
    j = get_phy_tick_time()-timeout;
	

    fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, bpts);
    fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wl_csma_rx_hd, bpts);
    fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wl_csma_rx_end, bpts);
    fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN, wl_swc_rx_frame, bpts);

	if ((ret = wphy_start_rx(NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
		 if (ret == -ETIME)
				++HPLC.rf_rx.statis.nr_late_csma_send;
		 else
         {
            ++HPLC.rf_rx.statis.nr_fail_csma_send;
                    
            wphy_trace_show(g_vsh.term);
            wphy_reset();
         }
         printf_s("wphy ca rx ret = %d ,%d\n",ret,j);
//         if(ret == -EBUSY)
//         {
//             wphy_reset();
//         }
//         hplc_do_next();
         return -1;
	}
	return 1;
}
/**
 * @brief rf_testmode_deal      回传模式处理接口
 * @param result                crc校验
 * @param para
 * @return
 */
__LOCALTEXT __isr__ int32_t rf_testmode_deal(uint8_t result,phy_evt_param_t *para)
{
    evt_notifier_t ntf[1];
    int32_t ret;
    uint32_t timeout;
    uint8_t dt = get_rf_frame_dt((rf_frame_ctrl_t *)para->fi->head);
    timeout = para->stime + para->dur + 2*WPHY_RIFS;

    //dump_zc(0,"hi",DEBUG_PHY,(uint8_t *)&para->fi->hi,sizeof(phy_hdr_info_t));
    //dump_zc(0,"pi",DEBUG_PHY,(uint8_t *)&para->fi->pi,sizeof(phy_pld_info_t));
    //phy_printf("para->fi->payload : 0x%08x para->fi->length : %d\n",para->fi->payload,para->fi->length);

#if defined(STD_2016) || defined(STD_DUAL)
    if(DT_BEACON == dt || (DT_SOF == dt && (result&0xf)))
        return -1;

#elif defined(STD_SPG)
    if(phy_cb_deal(dt,result,para))
    {
        return -1;
    }
#endif
    if(dt == DT_SOF)
    {      
        
        U8 *payload = NULL;
        payload = (U8 *)para->fi->payload;
        if(check_CB_FacTestMode_hook != NULL)
        {
            if(check_CB_FacTestMode_hook(payload,para->fi->length) == FALSE)
            {
                return -1;
            }
        }
    }


//    dump_printfs(para->fi->head, sizeof(rf_frame_ctrl_t));
//    dump_printfs(para->fi->payload, para->fi->length);

    fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_csma_tx_end, NULL);
    if ((ret = wphy_start_tx(para->fi, &timeout, &ntf[0], 1)) < 0) {
         if (ret == -ETIME) {
                ++HPLC.rf_tx.statis.nr_late[DT_SOF];
         } else {
                ++HPLC.rf_tx.statis.nr_fail[DT_SOF];
         }

         printf_s("WPHYCB ret = %d,timeout=%08x,tick%08x\n",ret,timeout,get_phy_tick_time());

         if(wl_csma_abandon(NULL) < 0)
            return -1;
    }
    return 0;
}

/**
 * @brief plc_to_rf_testmode_deal       通过无线回传载波接收到的数据
 * @param result                        校验
 * @param para                          数据
 * @return
 */
__LOCALTEXT __isr__ int32_t plc_to_rf_testmode_deal(uint8_t result, frame_ctrl_t *head, uint8_t *pld, uint16_t pld_len)
{
    evt_notifier_t ntf[1];
    int32_t ret;
    uint8_t dt = get_frame_dt(head);

    wphy_reset();

    //dump_zc(0,"hi",DEBUG_PHY,(uint8_t *)&para->fi->hi,sizeof(phy_hdr_info_t));
    //dump_zc(0,"pi",DEBUG_PHY,(uint8_t *)&para->fi->pi,sizeof(phy_pld_info_t));
    //phy_printf("para->fi->payload : 0x%08x para->fi->length : %d\n",para->fi->payload,para->fi->length);

#if defined(STD_2016) || defined(STD_DUAL)
    if(DT_BEACON == dt || (DT_SOF == dt && (result&0xf)))
        return -1;
#endif
    phy_frame_info_t *fi = NULL;
     mbuf_hdr_t *p = NULL;
    //无线组帧
    if(dt == DT_SOF)    //SOF帧回传
    {
        p = packet_sof_wl_forplc(head, pld, pld_len, g_phr_msc, g_psdu_mcs, g_pbsize);
        if(p == NULL)
        {
            return -1;
        }
        fi = &p->buf->fi;
    }
    else                //网间协调和ACK回传
    {
        HPLC.fi.head = HPLC.hdr;

        __memcpy(HPLC.fi.head, head, sizeof(rf_frame_ctrl_t));
        HPLC.fi.whi.phr_mcs = g_phr_msc;
        HPLC.fi.wpi.pld_mcs = g_psdu_mcs;
        HPLC.fi.wpi.blkz = g_pbsize;

        fi = &HPLC.fi;
    }

    if(fi == NULL)
    {
        return -1;
    }


    fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_csma_tx_end, NULL);
    if ((ret = wphy_start_tx(fi, NULL, &ntf[0], 1)) < 0) {
         printf_s("PHY2WPHYCB ret = %d,tick%08x\n",ret,get_phy_tick_time());
         if(wl_csma_abandon(NULL) < 0)
         {
             wl_do_next();
         }
    }

    if(p)
    {
        zc_free_mem(p);
    }
    return 0;
}

__LOCALTEXT  __isr__ void wl_set_sent_state(phy_evt_param_t *para,bpts_t *bpts)
{
	uint8_t dt=get_rf_frame_dt((rf_frame_ctrl_t *)para->fi->head);

	/*relese pf when expect sack recived after senting over*/
	/*set bpts->sent to be null,hpav do next will not release pf*/

	if(!bpts)
	{
		return;
	}
	if(bpts->async == 1)
	{
		if(dt == DT_SACK && expected_rf_sack((rf_sack_ctrl_t *)para->fi->head,bpts->stei,bpts->dtei,HPLC.snid))
		{
			bpts->async = 0;//-- senting pf can be released.
			wphy_printf("& rf get sack release\n");
		}
		else
		{
            if(bpts->sent)
            {
                bpts->sent->buf->backoff++;
                bpts->async = 0;
                bpts_pushback_pf(bpts->sent);
                bpts->sent = NULL;/*set bpts->sent to be null,hpav do next will not release pf*/
            }

		}
	}

}

__LOCALTEXT __isr__ void wl_csma_sig_tmot(phy_evt_param_t *para, void *arg)
{
	bpts_t *bpts;		
	int32_t ret;
	evt_notifier_t ntf[1];
	mbuf_hdr_t *pf;
	rf_frame_ctrl_t *fc;

	bpts = arg; 	

	/*no bpts*/
	if(!arg)
	{
		//wphy_printf("%\n");
		wl_do_next();
		return;	
	}
	/* waiting sack timeout*/
	if(bpts->async)
	{
		//wphy_printf("<sack %d  @ %dus>\n",bpts->sent?1:0,para->dur/25);
		bpts_pushback_pf(bpts->sent);
		bpts->sent = NULL;
		bpts->async = 0;
		
		wl_do_next();
		return;	
	}
	
	/*no data send*/
	if(!bpts->sent)
	{
		//wphy_printf("*");
		wl_do_next();
		return;	
	}
	/*have data to send*/
	pf = bpts->sent;
	fc = (rf_frame_ctrl_t *)pf->buf->fi.head;

	fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_csma_tx_end, bpts);
	if ((ret = wphy_start_tx(&pf->buf->fi, NULL, &ntf[0], 1)) < 0) {
		if (ret == -ETIME) {
            ++HPLC.rf_tx.statis.nr_late[fc->dt];
            ++HPLC.rf_tx.statis.nr_ts_late[bpts->type];
		} else {
            ++HPLC.rf_tx.statis.nr_fail[fc->dt];
                        
            wphy_trace_show(g_vsh.term);
            wphy_reset();
    }
		printf_s("wphy tx ret = %d-->",ret);
		bpts_pushback_pf(pf);
		bpts->sent = NULL;
		wl_do_next();
		return;
	}
}
__LOCALTEXT __isr__ void wl_csma_rx_end(phy_evt_param_t *para, void *arg)
{
	evt_notifier_t ntf[1];
	int32_t ret;
	uint32_t timeout;
	bpts_t *bpts = arg;
    uint8_t result, crc32;
	int8_t snr;
	
	result=0;
    snr = 0;


    //记录数据接收的信道号
    para->rfoption = wphy_get_option();
    para->rfchannel = wphy_get_channel();

    if(para->fail)
    {
//        printf_s("fail\n");
        goto next;
    }
	
    if(!para->fi)
    {
//        printf_s("para->fi:%p\n", para->fi);
        goto next;
    }
	if (!wheader_is_valid(para->fi))
	{
			if(bpts&&bpts->sent)
			{
				bpts->async =0;
				bpts_pushback_pf(bpts->sent);
				bpts->sent = NULL;
			}
            printf_s("header valid\n");
			goto next;
	}


	
	//macs_printf("%d ,%x \n",get_frame_dt(para->fi->head),get_frame_stei(para->fi->head));
	
	if(bpts)
	{
		//wphy_printf("dur @tt: %dus \n",PHY_TICK2US(bpts->time - get_phy_tick_time()));
		bpts->time = para->stime + para->dur + WPHY_RIFS;
	}
    //csma退避过程收到数据、并非等待sack收到数据
//    printf_s("bpts%08x,%08x,%d\n",bpts,bpts->sent,bpts->async);
	if(bpts&&bpts->sent&&!bpts->async)
	{
//        printf_s("pushback\n");
		bpts->sent->buf->backoff++;
		bpts_pushback_pf(bpts->sent);
		bpts->sent = NULL;
	}
	
	if(!para->fi->payload||0==para->fi->length)
	{
		para->fi->payload = NULL;
		para->fi->length = 0;
	}
	
	/* get sof result*/
     result = check_Rf_pb_crc(para->fi, &crc32, NULL,NULL);
     wphy_printf("crc : 0x%x \n",result);

     if(HPLC.testmode == WPHYCB || HPLC.testmode == RF_PLCPHYCB)
     {
        if(!rf_testmode_deal(result,para))
            return;
		
        goto next;
     }
//    if(para->fi->payload)
//		dhit_invalidate((uint32_t)para->fi->payload,para->fi->length);

	if(HPLC.testmode == MONITOR)
	{
		goto next;
	}
	
	/* send sack ,if get expected sof.*/
	if(get_rf_frame_dt((rf_frame_ctrl_t *)para->fi->head) == DT_SOF &&
		0 == get_rf_sof_bcsti((rf_sof_ctrl_t *)para->fi->head) &&
        expected_rf_sof((rf_sof_ctrl_t *)para->fi->head,HPLC.tei,HPLC.snid))
	{
		phy_frame_info_t *fi = packet_sack_wl((rf_sof_ctrl_t *)para->fi->head,result,snr);//packet sack  
		/* send sack */

        if(!(HPLC.worklinkmode & 0x02))     //如果是单载波模式，无线不回复ACK
            goto next;

//        goto next;  //退避测试，不回复ACK
		
        timeout = para->stime + get_rf_frame_dur(para->fi) + WPHY_RIFS + WPHY_RIFS/2;

        if(time_after(get_phy_tick_time()+PHY_US2TICK(800),timeout))
        {
            timeout = get_phy_tick_time()+PHY_US2TICK(800);
        }
//        timeout = para->stime + para->dur + WPHY_RIFS + WPHY_CIFS;
//        net_printf("wphy_rifs:%d, cifs:%d", WPHY_RIFS , WPHY_CIFS);

//        net_printf("%08x %d %08x %d %d\n"
//                   , para->stime, PHY_TICK2US(para->dur), get_phy_tick_time(), PHY_TICK2US(get_phy_tick_time() - para->stime)
//                   , (get_phy_tick_time() - timeout + PHY_US2TICK(200)));
		
		int32_t now = get_phy_tick_time()-timeout;
		fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_csma_tx_end, bpts); 
		if ((ret = wphy_start_tx(fi,&timeout, &ntf[0], 1)) < 0) {
            if (ret == -ETIME) {
                ++HPLC.rf_tx.statis.nr_late[DT_SACK];
            } else {
                ++HPLC.rf_tx.statis.nr_fail[DT_SACK];
                        
                wphy_trace_show(g_vsh.term);
                wphy_reset();
            }
             printf_s("WARNING sack tx: %d,%d\n", ret,now);
			 goto next;
		}
		 return;
	}
	/* if set bpts->sent to be NULL.
		if bpts->sent isn't NULL,there is a frame is senting*/
	wl_set_sent_state(para,bpts);
next:
	//wphy_printf("cs rx end\n");
	wl_do_next();
}
__LOCALTEXT __isr__ void wl_tdma_tx_end(phy_evt_param_t *para, void *arg)
{
	/*if time is enough,continue sending*/
    evt_notifier_t ntf[4];
	int32_t ret;
	bpts_t *bpts;

    
    if(arg == NULL)
    {
        wphy_printf("rf arg == NULL\n");
        wl_do_next();
        return;
    }
       


	bpts = arg;

    if(bpts->sent == NULL)
    {
        wphy_printf("rf sent == NULL\n");
        wl_do_next();
        return;
        //sys_panic("<tdma_tx_end> %s: %d\n", __func__, __LINE__);
    }

    bpts->time = para->stime + get_rf_frame_dur(para->fi) + WPHY_CIFS;
    wphy_printf("tx rfbcn\n");
    if(bpts->sent)
    {
        bpts->sent->sendflag = TRUE;
    }
    if(bpts->sent->buf->retrans > 0)
        bpts->sent->buf->retrans--;
    if (time_after(bpts->time + get_rf_frame_dur(para->fi), bpts->end-WPHY_CIFS) || bpts->sent->buf->retrans <= 0) {

        uint32_t timeout;
        if(time_before(get_phy_tick_time(), bpts->end-WPHY_CIFS))
        {
            timeout = bpts->end-WPHY_CIFS;

            fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, NULL);
            fill_evt_notifier(&ntf[1], WPHY_EVT_PHR,  wl_csma_rx_hd,   NULL);
            fill_evt_notifier(&ntf[2], WPHY_EVT_EORX,  wl_csma_rx_end,  NULL);
            fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN,  wl_swc_rx_frame, NULL);


            if ((ret = wphy_start_rx(NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
                if (ret == -ETIME)
                    ++HPLC.rf_rx.statis.nr_late[DT_BEACON];
                else
                {
                    ++HPLC.rf_rx.statis.nr_late[DT_BEACON];

                    wphy_trace_show(g_vsh.term);
                    wphy_reset();
                }

                printf_s("wtr rx ret = %d,%dus\n",ret,PHY_TICK2US(timeout));

                wl_do_next();
            }
        }
        else
        {
            wl_do_next();
        }
		return;
	}

    fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_tdma_tx_end, bpts);
    if ((ret = wphy_start_tx(&bpts->sent->buf->fi, &bpts->time, &ntf[0], 1)) < 0) {
         if (ret == -ETIME) {
                ++HPLC.rf_tx.statis.nr_late[DT_BEACON];
                ++HPLC.rf_tx.statis.nr_ts_late[bpts->type];
         } else {
                ++HPLC.rf_tx.statis.nr_fail[DT_BEACON];
                
                wphy_trace_show(g_vsh.term);
                wphy_reset();
         }
         printf_s("wl tdma_tx ret : %d\n",ret);
         wl_do_next();
    }
	return;
}


__LOCALTEXT __isr__ void wl_rx_frame(phy_evt_param_t *para, bpts_t *bpts)
{
    U8 dt;
    char *s;
    plc_work_t *work = NULL;
    rf_frame_ctrl_t *head;
    mbuf_t *buf;

    phy_frame_info_t *pf = para->fi;

    head = (rf_frame_ctrl_t *)pf->head;

    if (!wheader_is_valid(pf)) {
        return;
    }


    dt = get_rf_frame_dt(head);
    s= dt==DT_BEACON? "rbcn":
       dt==DT_SOF? "rsof":
       dt==DT_SACK? "rack":
       dt==DT_COORD? "rcod":
       "other";

    macs_printf("%s ,%x \n",s,get_rf_frame_stei(head));

//    if(filter_invalid(head,HPLC.tei,HPLC.snid) ==FALSE) //过滤无用数据,减少消息数量
//    return;


    work = zc_malloc_mem(sizeof(plc_work_t)+sizeof(mbuf_t)+sizeof(rf_frame_ctrl_t)+pf->length,s,MEM_TIME_OUT);
    
    if(work == NULL)
    {
        printf_s("rf work null\n");
        return;
    }

    buf = (mbuf_t *)work->buf;


    work->work = wl_link_layer_deal;

    buf->snid	= head->snid;
    buf->lid	= (bpts)?bpts->lid:0;
    buf->stime = para->stime;
    buf->rgain = para->rgain;
    buf->snr_level = 0;
    buf->snr = para->rf_snr;
    buf->rssi = para->rf_rssi;
    buf->LinkType = 1;  //标志无线接收数据
    buf->rfoption = para->rfoption;     //记录数据接收信道
    buf->rfchannel = para->rfchannel;
    /*work->buf->acki
    work->buf->backoff
    work->buf->ufs
    work->buf->priority
    work->buf->retrans */
    buf->fi.head = (uint8_t *)buf + sizeof(mbuf_t);
    __memcpy((uint8_t *)buf->fi.head,(uint8_t *)pf->head,sizeof(rf_frame_ctrl_t));
    buf->fi.payload = buf->fi.head + sizeof(rf_frame_ctrl_t);
    __memcpy((uint8_t *)buf->fi.payload,(uint8_t *)pf->payload,pf->length);
    buf->fi.length =  pf->length;
    __memcpy((uint8_t *)&buf->fi.whi,(uint8_t *)&pf->whi,sizeof(wphy_hdr_info_t));
    __memcpy((uint8_t *)&buf->fi.wpi,(uint8_t *)&pf->wpi,sizeof(wphy_pld_info_t));
//    work->msgtype = MPDUIND;
    post_plc_work(work);

    return;
}
__LOCALTEXT __isr__ void wl_swc_rx_frame(phy_evt_param_t *para, void *arg)
{
	if(para->fail)
	{
		return;
	}

    wl_rx_frame(para, arg);
}
__LOCALTEXT __isr__ void wl_csma_tx_end(phy_evt_param_t *para, void *arg)
{
	bpts_t *bpts = arg; 
	uint32_t timeout;
    evt_notifier_t ntf[4];
	int32_t ret;
	
//    wphy_printf("wl tx tick : %08x\n", para->stime);
    dump_zc(0,"$ wl tx : ",DEBUG_WPHY,para->fi->head,sizeof(rf_frame_ctrl_t));
	//dump_zc(0,"$  pi : ",DEBUG_WPHY,(U8*)&para->fi->pi,sizeof(phy_pld_info_t));

	if(!bpts)
	{
        wl_do_next();
        return;
	}
    
	if(bpts->sent)
    {
        bpts->sent->sendflag = TRUE;
    }
	/*retrans decrement*/
	if(bpts && bpts->sent&&bpts->sent->buf->retrans>0)
	{
		bpts->sent->buf->retrans--;
		//wphy_printf("retrans : %d\n",bpts->sent->buf->retrans);
	}
	/*need set repeat flag ,if sof need*/
	if(bpts && bpts->sent && DT_SOF == get_rf_frame_dt((rf_frame_ctrl_t *)para->fi->head))
	{
		set_rf_sof_repeat((rf_sof_ctrl_t *)bpts->sent->buf->fi.head);	
		bpts->async = get_rf_sof_bcsti((rf_sof_ctrl_t *)para->fi->head)?0:1;
		bpts->stei = get_rf_sof_stei((rf_sof_ctrl_t *)para->fi->head);
		bpts->dtei = get_rf_sof_dtei((rf_sof_ctrl_t *)para->fi->head);
	}
	//timeout = get_phy_tick_time()+ (bpts->async?(PHY_RIFS_MAX + SACK_FRAME_DUR(HPLC.hi.symn)):PHY_CIFS);

	timeout = para->stime + get_rf_frame_dur(para->fi) 
        +(bpts->async?(WPHY_RIFS_MAX/*PHY_MS2TICK(20)*/+ wphy_sack_frame_dur(wphy_get_option(), para->fi->whi.phr_mcs)):WPHY_CIFS);

    if(time_after(timeout,bpts->end-getWphyBifsByPostFlag(bpts)))
    {
        timeout = bpts->end - getWphyBifsByPostFlag(bpts);//WPHY_CIFS;
    }
	
    bpts->time = timeout + WPHY_CIFS;
	


	if(bpts&&bpts->async)
    {
        fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, bpts);
        fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wl_csma_rx_hd, bpts);
        fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wl_csma_rx_end, bpts);
        fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN, wl_swc_rx_frame, bpts);

        if ((ret = wphy_start_rx(NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
             if (ret == -ETIME)
                    ++HPLC.rf_rx.statis.nr_late_csma_send;
             else
             {
                ++HPLC.rf_rx.statis.nr_fail_csma_send;
                
                wphy_trace_show(g_vsh.term);
                wphy_reset();
             }
            printf_s("ERROR cswted rx ret = %d ,%08x,%08x\n",ret,timeout,get_phy_tick_time());
            bpts_pushback_pf(bpts->sent);
            bpts->sent = NULL;
             goto do_next;
        }

        return;
	}
	else
	{	
		bpts_pushback_pf(bpts->sent);
        bpts->sent = NULL;
    }


do_next:
	wl_do_next();
	return;
}
__LOCALTEXT __isr__ void wl_csma_rx_hd(phy_evt_param_t *para, void *arg)
{
	//在这里进行sfo和tick同步
}
__LOCALTEXT __isr__ int32_t wl_csma_send(bpts_t *bpts)
{
       uint32_t time, timeout;
       mbuf_hdr_t *pf;
       rf_frame_ctrl_t *fc;
       evt_notifier_t ntf[4];
       int32_t ret;

       time = bpts->time = get_phy_tick_time();
	   if(!(pf = wl_bpts_get_pf(bpts)))
		   return wl_csma_abandon(bpts);

       if(HPLC.testmode == WPHYTPT
               || HPLC.testmode == WPHYCB
               || HPLC.testmode == MACTPT
               || HPLC.testmode == RF_PLCPHYCB
               || HPLC.testmode == PLC_TO_RF_CB
              /* || (wphy_get_option() != pf->buf->rfoption)
               || (wphy_get_channel() != pf->buf->rfchannel)*/)
       {
           if(pf->buf->cnfrm)
               zc_free_mem(pf->buf->cnfrm);
           zc_free_mem(pf);

           return wl_csma_abandon(bpts);
       }

       if((wphy_get_option() != pf->buf->rfoption) || ((abs(wphy_get_channel() - pf->buf->rfchannel)) > 1))
       {
           printf_s("chl err:(%d,%d),(%d,%d),(%d)\n",wphy_get_option(),wphy_get_channel()
                    ,pf->buf->rfoption, pf->buf->rfchannel,wphy_hz_to_ch(wphy_get_option(), wphy_get_fckhz() * 1000));

           if(pf->buf->cnfrm)
               zc_free_mem(pf->buf->cnfrm);
           zc_free_mem(pf);

           return wl_csma_abandon(bpts);
       }

       if(!(HPLC.worklinkmode & 0x02))
       {
        //    if(pf->buf->cnfrm)
        //        zc_free_mem(pf->buf->cnfrm);
        //    zc_free_mem(pf);
            if(pf->buf->retrans > 0)
                pf->buf->retrans--;
            // wphy_printf("rf retrans %d\n", pf->buf->retrans);
            bpts_pushback_pf(pf);
            return wl_csma_abandon(bpts);
       }
	   
       fc = (rf_frame_ctrl_t *)pf->buf->fi.head;
       if (time == bpts->time) {     /* backoff time is 0 */
			  bpts->time = get_phy_tick_time();
              fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_csma_tx_end, bpts); 
              if ((ret = wphy_start_tx(&pf->buf->fi, NULL, &ntf[0], 1)) < 0) {
	                 if (ret == -ETIME) {
	                        ++HPLC.rf_tx.statis.nr_late[fc->dt];
	                        ++HPLC.rf_tx.statis.nr_ts_late[bpts->type];
	                 } else {
	                        ++HPLC.rf_tx.statis.nr_fail[fc->dt];
                            
                            wphy_trace_show(g_vsh.term);
                            wphy_reset();
	                 }
					 printf_s("rftx cs ret = %d,%-8dus,%d  \n",ret,PHY_TICK2US(get_phy_tick_time())-PHY_TICK2US(bpts->time),phy_get_status());
					 if(ret == -EBUSY) 
					 {
						ret = ret/(ret+EBUSY);
						wphy_reset();	
					 }
	                 bpts_pushback_pf(pf);
	                 return wl_csma_abandon(bpts);
              }
       } else {
                fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, wl_csma_sig_tmot, bpts);
                fill_evt_notifier(&ntf[1], WPHY_EVT_PHR, wl_csma_rx_hd, bpts);
                fill_evt_notifier(&ntf[2], WPHY_EVT_EORX, wl_csma_rx_end, bpts);
                fill_evt_notifier(&ntf[3], WPHY_EVT_EFIN, wl_swc_rx_frame, bpts);

                timeout = bpts->time - PMB_1SYM_DUR * 2;/*???*/
				int32_t nowtime = get_phy_tick_time()-timeout;
				//wphy_printf("csma\n");

              if ((ret = wphy_start_rx(NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
                     if (ret == -ETIME)
                            ++HPLC.rf_rx.statis.nr_late_csma_send;
                     else
                     {
                        ++HPLC.rf_rx.statis.nr_fail_csma_send;

                        wphy_trace_show(g_vsh.term);
                        wphy_reset();
                    }
 
                     bpts->time = time;
					 printf_s("rfrx ret = %d,%d,%d\n",ret,nowtime,PMB_1SYM_DUR);
					 
					 if(ret == -EBUSY) 
                     {
                         ret = ret/(ret+EBUSY);
                         wphy_reset();
                     }
                     bpts_pushback_pf(pf);
                     return wl_csma_abandon(bpts);
              }
       }
 
       bpts->sent = pf;
       return 1;
}

#endif //#if defined(STD_DUAL)
