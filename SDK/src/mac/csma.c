/*
 * Copyright: (c) 2006-2010, 2019 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	csma.c
 * Purpose:	PLC generic implementation suite
 * History: tridoctor csma.c
 * Author : panyu
 */
#include "csma.h"
#include "phy.h"
#include "phy_config.h"
#include "phy_debug.h"
#include "framectrl.h"
#include "que.h"
#include "list.h"
#include "plc.h"
#include "phy_plc_isr.h"
#include "phy_sal.h"
#include "Sys.h"
#include "mem_zc.h"
#include "Mbuf.h"
#include "plc_cnst.h"
#include "printf_zc.h"
#include "mac_common.h"
#include "types.h"



tx_link_t TX_MNG_LINK;
tx_link_t TX_DATA_LINK;
tx_link_t TX_BCSMA_LINK;



/*csma_abandon是启动有超时时间的接收， 超时时间一般是设成扩展IFS， 
如果距离当前bpts->end不足这个时间就设置成bpts->end - ifs.
bpts->time 更新到超时时间+ifs*/
__LOCALTEXT __isr__ int32_t csma_abandon(bpts_t *bpts)
{
    evt_notifier_t ntf[3];
    int32_t ret,j;
    j=0;
    uint32_t timeout;
    //uint32_t now = get_phy_tick_time();

    timeout = get_phy_tick_time() + PHY_EIFS;

    if(bpts)
    {
        if(time_after(get_phy_tick_time(),bpts->end-PHY_CIFS))
        {
            //hplc_do_next();
            return -1;
        }

        if(time_after(timeout,bpts->end-PHY_CIFS))
        {
            timeout = bpts->end - PHY_CIFS;
            if(time_after(get_phy_tick_time()+PHY_CIFS/2,timeout))
            {
                //hplc_do_next();
                return -1;
            }
        }
        bpts->time = timeout + PHY_CIFS;
    }

    j = timeout - get_phy_tick_time();

    fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, bpts);
    fill_evt_notifier(&ntf[1], PHY_EVT_RX_END, csma_rx_end, bpts);
    fill_evt_notifier(&ntf[2], PHY_EVT_RX_FIN, swc_rx_frame, bpts);

    bpts?ld_set_rx_phase_zc(TO_PHY_PHASE(bpts->phase)):ld_set_rx_phase_zc(PHY_PHASE_ALL);
    if ((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
         if (ret == -ETIME)
         {
            ++HPLC.rx.statis.nr_late_csma_send;
         }
         else
         {
            ++HPLC.rx.statis.nr_fail_csma_send;
            
            phy_trace_show(g_vsh.term);
            phy_reset();
         }

         printf_s("WARNING ca rx ret = %d ,%d, Status:%d\n",ret,j,phy_get_status());
         return -1;
    }
    return 1;
}
#if defined(STD_SPG)
__LOCALTEXT __isr__ int32_t phy_cb_deal(uint8_t dt,uint8_t result,phy_evt_param_t *para)
{
    uint8_t i;
    uint16_t j;
    bph_t *bph;
    uint8_t *p;

    if((DT_BEACON == dt || DT_SOF == dt)&&result&0xf)
        return -1;

    if(para->fi->payload)
    {
        //dhit_invalidate((uint32_t)para->fi->payload,para->fi->length);
        //need process data;

        if(DT_BEACON == dt)
        {
            bph 	   = (bph_t *)para->fi->payload;
            bph->som   = 1;
            bph->eom   = 1;
            bph->pbsz  = para->fi->pi.pbsz;
            bph->crc   = TRUE;//for tx crc32 - flag. ture:cal fill in pos(own 4bytes+129/513+crc32+crc)
            bph->crc32 = 0;   // for tx no mean
            p = para->fi->payload + sizeof(pb_desc_t);

            for(i=0;i<para->fi->pi.pbc;i++)
            {
                for(j=0;j<body_size[para->fi->pi.pbsz];j++)
                {
                    p[j] = ~p[j];
                }
            }
        }
        else if(DT_SOF == dt)
        {
            p = para->fi->payload + sizeof(pbh_t);
            for(i=0;i<para->fi->pi.pbc;i++)
            {
                for(j=0;j<body_size[para->fi->pi.pbsz];j++)
                {
                    p[j] = ~p[j];
                }
                p += sizeof(pbh_t);
            }
        }
        dhit_write_back((uint32_t)para->fi->payload,para->fi->length);
    }
    return 0;
}
#endif


__LOCALTEXT __isr__ int32_t testmode_deal(uint8_t result,phy_evt_param_t *para)
{
    evt_notifier_t ntf[1];
    int32_t ret;
    uint32_t timeout;
    uint8_t dt = get_frame_dt((frame_ctrl_t *)para->fi->head);
    timeout = para->stime + para->dur + 2*PHY_RIFS;

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

    fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, csma_tx_end, NULL);
    ld_set_tx_phase_zc(PHY_PHASE_ALL);
    if ((ret = phy_start_tx(para->fi, &timeout, &ntf[0], 1)) < 0) {
         if (ret == -ETIME) {
                ++HPLC.tx.statis.nr_late[DT_SOF];
         } else {
                ++HPLC.tx.statis.nr_fail[DT_SOF];

                phy_trace_show(g_vsh.term);
                phy_reset();
         }

         printf_s("PHYCB ret = %d,timeout=%08x,tick%08x\n",ret,timeout,get_phy_tick_time());

         if(csma_abandon(NULL) < 0)
            return -1;
    }
    return 0;
}


__LOCALTEXT __isr__ mbuf_hdr_t * bpts_get_pf(bpts_t *bpts)
{
    mbuf_hdr_t *mpdu;
    tx_link_t *link;
    uint32_t stime,etime;
    uint32_t random;

    if(time_after(bpts->time,bpts->end-MIN_FRAME_DUR(1)))
        return NULL;

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
        link = &TX_BCSMA_LINK;
        if(!(mpdu = tx_link_deq(link, bpts->phase)))
                return NULL;
    }else{
        if(!list_empty(&bpts->pf_list)){
            mpdu = list_entry(bpts->pf_list.next,mbuf_hdr_t,link);
            list_del(&mpdu->link);
            return mpdu;
        }

        link = &TX_MNG_LINK;
        if(!(mpdu = tx_link_deq(link, bpts->phase))){
            link = &TX_DATA_LINK;
            if(!(mpdu = tx_link_deq(link, bpts->phase)))
                return NULL;
        }

    }
    stime = bpts->time;

    //phy_printf("\nretrans : %d\n",mpdu->buf->retrans);

    if(bpts->type == TS_CSMA || bpts->type == TS_BCSMA){
        random   = backoff_mask(mpdu->buf->backoff,mpdu->buf->priority);
        if(random !=0 )
        {
            //printf_s("<%d>",random);
            bpts->time += (random*PMB_1SYM_DUR);
        }
    }
    etime = bpts->time + get_frame_dur(&mpdu->buf->fi)
        + (mpdu->buf->acki?(PHY_RIFS_MAX + SACK_FRAME_DUR(mpdu->buf->fi.hi.symn)):PHY_CIFS);

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


__LOCALTEXT __isr__ void csma_tx_end(phy_evt_param_t *para, void *arg)
{
    bpts_t *bpts = arg;
    uint32_t timeout;
    evt_notifier_t ntf[3];
    int32_t ret;
    phy_printf("%d ",get_frame_dt((frame_ctrl_t *)para->fi->head));
    dump_zc(0,"$  tx : ",DEBUG_PHY,para->fi->head,sizeof(frame_ctrl_t));

    if(!bpts)
    {
        hplc_do_next();
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
        //phy_printf("retrans : %d\n",bpts->sent->buf->retrans);
    }
    /*need set repeat flag ,if sof need*/
    if(bpts && bpts->sent && DT_SOF == get_frame_dt((frame_ctrl_t *)para->fi->head))
    {
        set_sof_repeat((sof_ctrl_t *)bpts->sent->buf->fi.head);
        bpts->async = get_sof_bcsti((sof_ctrl_t *)para->fi->head)?0:1;
        bpts->stei = get_sof_stei((sof_ctrl_t *)para->fi->head);
        bpts->dtei = get_sof_dtei((sof_ctrl_t *)para->fi->head);
    }
    //timeout = get_phy_tick_time()+ (bpts->async?(PHY_RIFS_MAX + SACK_FRAME_DUR(HPLC.hi.symn)):PHY_CIFS);

    timeout = para->stime + get_frame_dur(para->fi)
        + (bpts->async?(PHY_RIFS_MAX/*PHY_MS2TICK(20)*/ + SACK_FRAME_DUR(HPLC.hi.symn)):PHY_CIFS);

    if(time_after(timeout,bpts->end-PHY_CIFS))
    {
        timeout = bpts->end - PHY_RIFS;
    }

    bpts->time = timeout + PHY_CIFS;

    if(bpts&&bpts->async)
    {
        fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, bpts);
        fill_evt_notifier(&ntf[1], PHY_EVT_RX_END, csma_rx_end, bpts);
        fill_evt_notifier(&ntf[2], PHY_EVT_RX_FIN, swc_rx_frame, bpts);

        ld_set_rx_phase_zc(TO_PHY_PHASE(bpts->phase));
        if ((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
             if (ret == -ETIME)
             {
                    ++HPLC.rx.statis.nr_late_csma_send;
             }
             else
             {
                    ++HPLC.rx.statis.nr_fail_csma_send;

                    phy_trace_show(g_vsh.term);
                    phy_reset();
             }
            printf_s("ERROR csted rx ret = %d ,%08x,%08x\n",ret,timeout,get_phy_tick_time());
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
    hplc_do_next();
    return;
}
__LOCALTEXT __isr__ void csma_pd_tmot(phy_evt_param_t *para, void *arg)
{
    bpts_t *bpts;
    int32_t ret;
    evt_notifier_t ntf[1];
    mbuf_hdr_t *pf;
    frame_ctrl_t *fc;

    bpts = arg;

    /*no bpts*/
    if(!arg)
    {
        //phy_printf("%\n");
        hplc_do_next();
        return;
    }
    /* waiting sack timeout*/
    if(bpts->async)
    {
        //phy_printf("<sack %d  @ %dus>\n",bpts->sent?1:0,para->dur/25);
        bpts_pushback_pf(bpts->sent);
        bpts->sent = NULL;
        bpts->async = 0;

        hplc_do_next();
        return;
    }

    /*no data send*/
    if(!bpts->sent)
    {
        //phy_printf("*");
        hplc_do_next();
        return;
    }
    /*have data to send*/
    pf = bpts->sent;
    fc = (frame_ctrl_t *)pf->buf->fi.head;

    fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, csma_tx_end, bpts);
    ld_set_tx_phase_zc(TO_PHY_PHASE(pf->phase));
    if ((ret = phy_start_tx(&pf->buf->fi, NULL, &ntf[0], 1)) < 0) {
        if (ret == -ETIME) {
                ++HPLC.tx.statis.nr_late[fc->dt];
                ++HPLC.tx.statis.nr_ts_late[bpts->type];
        } 
        else {
                ++HPLC.tx.statis.nr_fail[fc->dt];

                extern void phy_trace_show(tty_t *term);
                phy_trace_show(g_vsh.term);
                phy_reset();
        }
        printf_s("tx ret = %d-->",ret);
        bpts_pushback_pf(pf);
        bpts->sent = NULL;
        hplc_do_next();
        return;
    }
}
extern U8 sof_cmb(pbh_t *pbh, uint16_t pbsz, uint8_t crc);
__LOCALTEXT __isr__ uint8_t check_pb_crc(phy_frame_info_t *pf,uint8_t *CRC32,uint32_t *pld , uint16_t *len , uint8_t *pkg)
{
    frame_ctrl_t *head;
    void *payload;
    pbh_t *pbh;
    uint8_t state;
//    uint32_t *crc32;
    uint8_t i;

    *CRC32 = 0;
    state = 0;
    payload = pf->payload;
    head = (frame_ctrl_t *)pf->head;

    if(pkg)
    {
        *pkg = FALSE;
    }

    if (payload&&pf->length)
    {
        if (head->dt == DT_BEACON)
        {
#if defined(VENUS2M) || defined(VENUS8M)
            state = pf->pi.crc24;

            if(pf->pi.crc32_en)
            {
                *CRC32 = pf->pi.crc32;
            }
#else

            bph_t *bph;

            bph = (bph_t *)payload;
            state=0x01;
            if (!bph->crc)//correct
            {
                state = 0x00;
                state |= (1<<4);
                //phy_debug_printf("beacon state = 0x%02x!\n",state);
            }

            *CRC32 = bph->crc32;
#endif

            if(pld && len)
            {
#if defined(VENUS2M) || defined(VENUS8M)
                __memcpy(pld,payload, body_size[pf->pi.pbsz] -3);
                *len = body_size[pf->pi.pbsz] -3;
#else
                #if defined(STD_2016)
                __memcpy(pld,payload+sizeof(bph_t),body_size[bph->pbsz]-3);
                *len = body_size[bph->pbsz]-3;
                #elif defined(STD_SPG)
                __memcpy(pld,payload+sizeof(bph_t),body_size[bph->pbsz]);
                *len = body_size[bph->pbsz];
                #endif
#endif
            }
        }
        else
        {
#if defined(VENUS2M) || defined(VENUS8M)
            uint8_t pbcrc = 0;
            for(i = 0, pbh = (pbh_t *)payload; i < pf->pi.pbc; ++i)
            {
                pbcrc = pf->pi.crc24 & (1 << i);
                if(!pbcrc)
                {
                    state |= (1<<(4+i));
                }
                else
                {
                    state |= 0x01;
                }

                if(pld && len)
                {
                    #if defined(STD_2016)
                    if(HPLC.testmode == PHYTPT)
                    {
                        __memcpy(pld,pbh+sizeof(pb_desc_t),pb_bufsize(pf->pi.pbsz)-sizeof(pb_desc_t));
                        *len += pb_bufsize(pf->pi.pbsz)-sizeof(pb_desc_t);
                        pld += (pb_bufsize(pf->pi.pbsz)-sizeof(pb_desc_t))/4;
                    }
                    else
                    {
                        __memcpy(pld,pbh+1,body_size[pf->pi.pbsz]);
                        *len += body_size[pf->pi.pbsz];
                        pld += body_size[pf->pi.pbsz]/4;
                    }
                    #endif
                
	                if(sof_cmb(pbh, pf->pi.pbsz, pbcrc))//net 处理时才会调用
	                {
	                    if(pkg)
	                    {
	                        *pkg = TRUE;
	                    }
	                }
				}
                pbh = (void *)pbh + pb_bufsize(pf->pi.pbsz);
            }
#else
            state = 0x00;
            for (i = 0, pbh = (pbh_t *)payload; i < get_frame_pbc(head); ++i)
            {
//                crc32 = (void *)pbh + pb_bufsize(pbh->pbsz) - 4;
                if (!pbh->crc)//correct
                {
                    state |= (1<<(4+i));
                    //phy_debug_printf("sof state = 0x%02x!\n",state);
                    //break;
                }
                else//error
                {
                    state |= 0x01;
                }

                if(pld && len)
                {
                    #if defined(STD_2016)
                    if(HPLC.testmode == PHYTPT)
                    {
                        __memcpy(pld,(uint8_t *)pbh+sizeof(pb_desc_t),pb_bufsize(pbh->pbsz)-sizeof(pb_desc_t));
                        *len += (pb_bufsize(pbh->pbsz)-sizeof(pb_desc_t));
                        pld += (pb_bufsize(pbh->pbsz)-sizeof(pb_desc_t))/4;
                    }
                    else
                    {
                        __memcpy(pld,pbh+1,body_size[pbh->pbsz]);
                        *len += body_size[pbh->pbsz];
                        pld += body_size[pbh->pbsz]/4;
                    }
                    //printf_s("pb_bufsize : %d, body_size: %d\n", pb_bufsize(pbh->pbsz), body_size[pbh->pbsz]);
                    //printf_s("pbh = %08x----->", pbh);
                    //dump_printfs((U8 *)pbh, pb_bufsize(pbh->pbsz));
                    if(sof_cmb(pbh, pbh->pbsz, pbh->crc))//net 处理时才会调用
                    {
                        if(pkg)
                        {
                            *pkg = TRUE;
                        }
                    }
                    #elif defined(STD_SPG)
                    __memcpy(pld,pbh+1,body_size[pbh->pbsz]);
                    *len += body_size[pbh->pbsz];
                    pld += body_size[pbh->pbsz]/4;
                    #endif

                }

//                pbh = (pbh_t *)(crc32 + 1);
                pbh = (void *)pbh + pb_bufsize(pbh->pbsz);
            }
#endif

//            *CRC32 = pbh->crc32;

        }
    }
    else
    {
        state = 0x00;
    }

    return state;
}


__LOCALTEXT  __isr__ void set_sent_state(phy_evt_param_t *para,bpts_t *bpts)
{
    uint8_t dt=get_frame_dt((frame_ctrl_t *)para->fi->head);

    /*relese pf when expect sack received after senting over*/
    /*set bpts->sent to be null,hpav do next will not release pf*/

    if(!bpts)
    {
        return;
    }
    if(bpts->async == 1)
    {
        if(dt == DT_SACK && expected_sack((sack_ctrl_t *)para->fi->head,bpts->stei,bpts->dtei,HPLC.snid))
        {
            bpts->async = 0;//-- senting pf can be released.
            phy_printf("& get sack release\n");
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

__LOCALTEXT __isr__ int32_t snr_esti(phy_frame_info_t *pf,int8_t *snr_ave)
{
    double snr;
    phy_tonemap_para_t tm;
    memset(&tm, 0, sizeof(phy_tonemap_para_t));

    if (pf->payload && pf->length)
    {
        tm.snr_tone = __malloc(NR_TONE_MAX);
        memset(tm.snr_tone, 0, NR_TONE_MAX);
        phy_pld_snr_esti(pf, &tm);
       /* PHY_DEMO.snr_level = tm.level;
        PHY_DEMO.mm        = tm.mm;
        PHY_DEMO.fecr      = tm.fecr;
        PHY_DEMO.copy      = tm.copy;
        PHY_DEMO.snr_ave   = phy_pld_snr_get(); //-5~30*/
        snr = tm.snr_ave;//phy_pld_snr_get();
        *snr_ave = (int8_t)snr;
        //phy_printf("snr : %.3f\n",snr);
        free(tm.snr_tone);
        return 0;
    }
    return -1;
}
__LOCALTEXT __isr__ void csma_rx_hd(phy_evt_param_t *para, void *arg)
{
   // REG32(0xf12001d0) = 0x02514514;
}
__LOCALTEXT __isr__ void csma_rx_end(phy_evt_param_t *para, void *arg)
{
    evt_notifier_t ntf[1];
    int32_t ret;
    uint32_t timeout;
    bpts_t *bpts = arg;
    uint8_t result;
    uint8_t crc32;
    int8_t snr;

    uint8_t phase = 0;

    //记录载波数据接收baud
    para->baud = CURR_B->idx;

    result=0;
    crc32=0;
    snr = 0;

    if (!header_is_valid(para->fi))
    {
            if(bpts&&bpts->sent)
            {
                bpts->async =0;
                bpts_pushback_pf(bpts->sent);
                bpts->sent = NULL;
            }
            /*#if defined(STATIC_MASTER)
            if(bpts->op ==tdma_recv)
            {
                phy_printf("ivd tick=0x%08x\n",get_phy_tick_time());
            }
            #endif*/
            goto next;
    }

    if(para->fail == 1)
    {
            /*#if defined(STATIC_MASTER)
            if(bpts->op ==tdma_recv)
            {
                phy_printf("fail tick=0x%08x\n",get_phy_tick_time());
            }
            #endif*/
            if(bpts&&bpts->sent)
            {
                bpts->async = 0;
                bpts_pushback_pf(bpts->sent);
                bpts->sent = NULL;
            }
        goto next;
    }


    //macs_printf("%d ,%x \n",get_frame_dt(para->fi->head),get_frame_stei(para->fi->head));

    if(bpts)
    {
        //phy_printf("dur @tt: %dus \n",PHY_TICK2US(bpts->time - get_phy_tick_time()));
        bpts->time = para->stime + para->dur + PHY_RIFS;

        if(bpts->sent)
            phase = bpts->sent->phase;
    }
	//csma退避过程收到数据、并非等待sack收到数据
    //phy_printf("bpts%p,%p,%d\n",bpts,bpts->sent,bpts->async);
    if(bpts&&bpts->sent&&!bpts->async)
    {
        bpts->sent->buf->backoff++;
        bpts_pushback_pf(bpts->sent);
        bpts->sent = NULL;
    }

    if(!para->fi->payload||0==para->fi->length)
    {
        para->fi->payload = NULL;
        para->fi->length = 0;
    }
#if defined(STD_2016)

    // phy_notch_freq_set(para);

	//物理层透传模式下+收到信标、 时间间隔为一个信标周期 噪声监测
	//正常模式模式下+收到信标、 时间间隔为十五个个信标周期 噪声监测
   if(((HPLC.noise_proc && HPLC.testmode == PHYTPT)||(HPLC.noise_proc>=0x0f && HPLC.testmode == NORM))&&
       get_frame_dt((frame_ctrl_t *)para->fi->head) == DT_BEACON)
   {
       HPLC.noise_proc = 0;
       ret = noise_proc_fn(arg);
       wphy_printf("nois pro : %d\n",ret);
       return;
   }
#elif defined(STD_SPG)
	//mac层透传模式下，时间间隔为一个信标周期 噪声监测
	//mac层正常模式下，时间间隔为15个个信标周期 噪声监测
    if((HPLC.noise_proc && HPLC.testmode == MACTPT)||
        (HPLC.noise_proc>=0x0f && HPLC.testmode == NORM && get_frame_dt((frame_ctrl_t *)para->fi->head) == DT_BEACON))
    {
        HPLC.noise_proc = 0;
//        if(para->fi->payload)
//            dhit_invalidate((uint32_t)para->fi->payload,para->fi->length);
        ret = noise_proc_fn(arg);
        app_printf("nois pro : %d\n",ret);
        return;
    }
#endif
    //snr_esti(para->fi,&snr);
    //phy_printf("snr : %d\n",snr);

    /* get sof result*/
    result = check_pb_crc(para->fi,&crc32,NULL,NULL,NULL);
    //phy_printf("%x,%d\n",result,para->rgain);

    if(HPLC.testmode == PHYCB || HPLC.testmode == RF_PLCPHYCB)
    {
        if(!testmode_deal(result,para))
            return;

        goto next;
    }
//    if(para->fi->payload)
//        dhit_invalidate((uint32_t)para->fi->payload,para->fi->length);

    if(HPLC.testmode == MONITOR)
    {
        goto next;
    }

    /* send sack ,if get expected sof.*/
    if(get_frame_dt((frame_ctrl_t *)para->fi->head) == DT_SOF &&
        0 == get_sof_bcsti((sof_ctrl_t *)para->fi->head) &&
        expected_sof((sof_ctrl_t *)para->fi->head,HPLC.tei,HPLC.snid))
    {
        phy_frame_info_t *fi = packet_sack((sof_ctrl_t *)para->fi->head,result,snr);//packet sack
        /* send sack */

         if(!(HPLC.worklinkmode & 0x01))        //如果是纯无线模式，载波不回复ACK
            goto next;


//        goto next;  //退避测试，不回复ACK

        timeout = para->stime + para->dur + PHY_RIFS+PHY_RIFS/2;
        //phy_printf("dur=%08x,stime=%08x,timeout=%08x,now=%08x,diff=%d\n",
                //para->dur,para->stime,timeout,get_phy_tick_time(),get_phy_tick_time()-timeout);
        if(time_after(get_phy_tick_time()+PHY_US2TICK(200),timeout))
        {
            timeout = get_phy_tick_time()+PHY_US2TICK(200);
        }

        int32_t now = get_phy_tick_time()-timeout;
        fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, csma_tx_end, bpts);
        ld_set_tx_phase_zc(TO_PHY_PHASE(phase));
        if ((ret = phy_start_tx(fi, &timeout, &ntf[0], 1)) < 0) {
             if (ret == -ETIME) {
                    ++HPLC.tx.statis.nr_late[DT_SACK];
             } else {
                    ++HPLC.tx.statis.nr_fail[DT_SACK];

                    phy_trace_show(g_vsh.term);
                    phy_reset();
             }
             printf_s("WARNING sack tx: %d,%d\n", ret,now);
             goto next;
        }
        return;
    }
    /* if set bpts->sent to be NULL.
        if bpts->sent isn't NULL,there is a frame is senting*/
    set_sent_state(para,bpts);
next:
    //phy_printf("cs rx end\n");
    hplc_do_next();
}
__LOCALTEXT __isr__ void tdma_tx_end(phy_evt_param_t *para, void *arg)
{
    /*if time is enough,continue sending*/
    evt_notifier_t ntf[4];
    int32_t ret;
    bpts_t *bpts;
    uint32_t end;

    bpts = arg;

    bpts->time = para->stime + get_frame_dur(para->fi) + PHY_CIFS*2;
#if 0//STATC_MASTER
    if(HPLC.noise_proc)
    {
        HPLC.noise_proc = 0;
        phy_printf("nois pro : %d \n",noise_proc_fn(arg));
        return;
    }
#endif

    phy_printf("tx bcn\n");

//    dump_zc(0, "bcn", DEBUG_PHY, para->fi->head, sizeof(sof_ctrl_t));
//    dump_zc(0, "bcn", DEBUG_PHY, para->fi->payload, para->fi->length);
    if(bpts->sent == NULL)
    {
        phy_printf("bpts->sent == NULL\n");
        hplc_do_next();
        return;
        //sys_panic("<tdma_tx_end> %s: %d\n", __func__, __LINE__);
    }
    bpts->sent->sendflag = TRUE;

    //在这里做成功率统计
    if(pSendBeaconCnt)
    {
        pSendBeaconCnt();
    }
    
    if(bpts->sent->buf->retrans > 0)
        bpts->sent->buf->retrans--;
    
    if(bpts->txflag)
    {
        end = bpts->end - PHY_CIFS*BCOLT_STOP_EARLY;
    }
    else
    {
        end = bpts->end - PHY_CIFS;
    }

    if (time_after(bpts->time + get_frame_dur(para->fi), end) || bpts->sent->buf->retrans <= 0) {

        uint32_t timeout = end;
        fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, NULL);
        fill_evt_notifier(&ntf[1], PHY_EVT_RX_HDR, csma_rx_hd, NULL);
        fill_evt_notifier(&ntf[2], PHY_EVT_RX_END, csma_rx_end, NULL);
        fill_evt_notifier(&ntf[3], PHY_EVT_RX_FIN, swc_rx_frame, NULL);

        ld_set_rx_phase_zc(TO_PHY_PHASE(bpts->phase));
        if ((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
             if (ret == -ETIME)
             {
                    ++HPLC.rx.statis.nr_late_csma_send;
             }
             else
             {
                    ++HPLC.rx.statis.nr_fail_csma_send;

                    phy_trace_show(g_vsh.term);
                    phy_reset();
             }

             printf_s("tdma rx ret = %d\n",ret);
             hplc_do_next();
        }

        return;
    }

    fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, tdma_tx_end, bpts);
    ld_set_tx_phase_zc(TO_PHY_PHASE(bpts->phase));
    if ((ret = phy_start_tx(&bpts->sent->buf->fi, &bpts->time, &ntf[0], 1)) < 0) {
         if (ret == -ETIME) {
                ++HPLC.tx.statis.nr_late[DT_BEACON];
                ++HPLC.tx.statis.nr_ts_late[bpts->type];
         } else {
                ++HPLC.tx.statis.nr_fail[DT_BEACON];

                phy_trace_show(g_vsh.term);
                phy_reset();
         }
         printf_s("tdma_tx ret : %d\n",ret);
         hplc_do_next();
    }
    return;
}
//__LOCALTEXT __isr__ void plc_rx_frame(phy_frame_info_t *pf, uint32_t stime, int32_t gain,bpts_t *bpts)
__LOCALTEXT __isr__ void plc_rx_frame(phy_evt_param_t *para, bpts_t *bpts)
{
    U8 dt;
    char *s;
    plc_work_t *work;
    frame_ctrl_t *head;
    mbuf_t *buf;

    phy_frame_info_t *pf = para->fi;

    head = (frame_ctrl_t *)pf->head;

    if (!header_is_valid(pf)) {
        return;
    }


    dt = get_frame_dt(head);
    s= dt==DT_BEACON? "rbcn":
       dt==DT_SOF? "rsof":
       dt==DT_SACK? "rack":
       dt==DT_COORD? "rcod":
       "other";


	if(filter_invalid(head,HPLC.tei,HPLC.snid) ==FALSE) //过滤无用数据,减少消息数量
	return;
    //单测无线
    //return;

    work = zc_malloc_mem(sizeof(plc_work_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t)+pf->length,s,MEM_TIME_OUT);
    if(work == NULL)
    {
        printf_s("work is null\n");
        return;
    }


    buf = (mbuf_t *)work->buf;

    if (dt == DT_SOF && HPLC.testmode == PHYTPT) 
    {
#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
        phy_printf("notch_freq = %d, pulse = %d\n", para->notch_freq, para->pulse);
#endif 
    }


    work->work = link_layer_deal;

    buf->snid	= head->snid;
    buf->lid	= (bpts)?bpts->lid:0;
    buf->stime = para->stime;
    buf->rgain = para->rgain;
    buf->snr_level = 0;
    buf->snr = 0;
    buf->LinkType = 0;  //??????????????
    buf->bandidx = para->baud;  //??????????????
    buf->phase = (bpts)?bpts->phase : 0;
    /*work->buf->acki
    work->buf->backoff
    work->buf->ufs
    work->buf->priority
    work->buf->retrans */
    buf->fi.head = (uint8_t *)buf + sizeof(mbuf_t);
    __memcpy((uint8_t *)buf->fi.head,(uint8_t *)pf->head,sizeof(frame_ctrl_t));
    buf->fi.payload = buf->fi.head + sizeof(frame_ctrl_t);
    __memcpy((uint8_t *)buf->fi.payload,(uint8_t *)pf->payload,pf->length);
    buf->fi.length =  pf->length;
    __memcpy((uint8_t *)&buf->fi.hi,(uint8_t *)&pf->hi,sizeof(phy_hdr_info_t));
    __memcpy((uint8_t *)&buf->fi.pi,(uint8_t *)&pf->pi,sizeof(phy_pld_info_t));
    // work->msgtype = MPDUIND;
    post_plc_work(work);
//    link_layer_deal(work);

    return;
}
__LOCALTEXT __isr__ void swc_rx_frame(phy_evt_param_t *para, void *arg)
{
    if(para->fail == 1)
    {
        return;
    }
//    plc_rx_frame(para->fi, para->stime, para->rgain,arg);
    plc_rx_frame(para, arg);
}
__LOCALTEXT __isr__ int32_t csma_send(bpts_t *bpts)
{
       uint32_t time, timeout;
       mbuf_hdr_t *pf;
       frame_ctrl_t *fc;
       evt_notifier_t ntf[3];
       int32_t ret;

       time = bpts->time = get_phy_tick_time();

#if defined(STATIC_NODE)
       //第一个csma时隙，先听100ms
       if(bpts->rxflag)
       {
           bpts->rxflag = 0;
           timeout = time + PHY_MS2TICK(20);
           if(time_after(timeout, bpts->end - PHY_CIFS))
               timeout = bpts->end - PHY_CIFS;

           fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, bpts);
           fill_evt_notifier(&ntf[1], PHY_EVT_RX_END, csma_rx_end, bpts);
           fill_evt_notifier(&ntf[2], PHY_EVT_RX_FIN, swc_rx_frame, bpts);


           ld_set_rx_phase_zc(TO_PHY_PHASE(bpts->phase));
           if ((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
                  if (ret == -ETIME)
                  {
                        ++HPLC.rx.statis.nr_late_csma_send;
                  }
                  else
                  {
                        ++HPLC.rx.statis.nr_fail_csma_send;

                        phy_trace_show(g_vsh.term);
                        phy_reset();
                  }

                  printf_s("csma send rx ret = %d\n",ret);
                  return -1;
           }
           return 1;
       }
#endif

       if(!(pf = bpts_get_pf(bpts)))
           return csma_abandon(bpts);

       if(HPLC.testmode == PHYTPT
               || HPLC.testmode == PHYCB
               || HPLC.testmode == MACTPT
               || HPLC.testmode == RF_PLCPHYCB
               || HPLC.testmode == PLC_TO_RF_CB
               /*|| CURR_B->idx != pf->buf->bandidx*/)
       {
           if(pf->buf->cnfrm)
               zc_free_mem(pf->buf->cnfrm);
           zc_free_mem(pf);

           return csma_abandon(bpts);
       }

       if(CURR_B->idx != pf->buf->bandidx)
       {
           printf_s("band err:%d,%d\n",CURR_B->idx,pf->buf->bandidx);
           if(pf->buf->cnfrm)
               zc_free_mem(pf->buf->cnfrm);
           zc_free_mem(pf);

           return csma_abandon(bpts);;
       }

       if(!(HPLC.worklinkmode & 0x01))
       {
        //    if(pf->buf->cnfrm)
        //        zc_free_mem(pf->buf->cnfrm);
        //    zc_free_mem(pf);
            if(pf->buf->retrans > 0)
                pf->buf->retrans--;
            // phy_printf("plc retrans %d\n", pf->buf->retrans);
            bpts_pushback_pf(pf);
            return csma_abandon(bpts);;
       }


       if((pf->phase != bpts->phase) && (pf->phase != PHASE_NONE))  //该数据无法在不对应的相位时隙中发送
       {
           bpts_pushback_pf(pf);
           return csma_abandon(bpts);
       }

       fc = (frame_ctrl_t *)pf->buf->fi.head;
       if (time == bpts->time) {     /* backoff time is 0 */
              bpts->time = get_phy_tick_time();
              fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, csma_tx_end, bpts);
              ld_set_tx_phase_zc(TO_PHY_PHASE(pf->phase));
              if ((ret = phy_start_tx(&pf->buf->fi, NULL, &ntf[0], 1)) < 0) {
                     if (ret == -ETIME) {
                            ++HPLC.tx.statis.nr_late[fc->dt];
                            ++HPLC.tx.statis.nr_ts_late[bpts->type];
                     } else {
                            ++HPLC.tx.statis.nr_fail[fc->dt];
                            
                            phy_trace_show(g_vsh.term);
                            phy_reset();
                     }
                     printf_s("tx cs ret = %d,%-8dus,%d  \n",ret,PHY_TICK2US(get_phy_tick_time())-PHY_TICK2US(bpts->time),phy_get_status());
                     bpts_pushback_pf(pf);
                     return csma_abandon(bpts);
              }
       } else {


              fill_evt_notifier(&ntf[0], PHY_EVT_PD_TMOT, csma_pd_tmot, bpts);
              fill_evt_notifier(&ntf[1], PHY_EVT_RX_END, csma_rx_end, bpts);
              fill_evt_notifier(&ntf[2], PHY_EVT_RX_FIN, swc_rx_frame, bpts);

                timeout = bpts->time - PMB_1SYM_DUR * 2;/*???*/
                int32_t nowtime = get_phy_tick_time()-timeout;
                //phy_printf("csma\n");


              ld_set_rx_phase_zc(TO_PHY_PHASE(bpts->phase));
              if ((ret = phy_start_rx(&HPLC.hi, NULL, &timeout, ntf, NELEMS(ntf))) < 0) {
                     if (ret == -ETIME)
                     {
                            ++HPLC.rx.statis.nr_late_csma_send;
                     }
                     else
                     {
                            ++HPLC.rx.statis.nr_fail_csma_send;

                            phy_trace_show(g_vsh.term);
                            phy_reset();
                     }

                     bpts->time = time;
                     printf_s("rx ret = %d,%d,%d\n",ret,nowtime,PMB_1SYM_DUR);
                     bpts_pushback_pf(pf);
                     return csma_abandon(bpts);
              }
       }

       bpts->sent = pf;
       return 1;
}


void phy_msg_show(xsh_t *xsh,tx_link_t *tx_link)
{
    list_head_t *pos;
    mbuf_hdr_t *txmsg;
    tty_t *term;

    term = xsh->term;

    term->op->tprintf(term, "lid\t\n");

    list_for_each(pos, &tx_link->link) {
        txmsg = list_entry(pos, mbuf_hdr_t, link);
        term->op->tprintf(term, "%-2d\t\n",
            txmsg->buf->lid);
    }
}







