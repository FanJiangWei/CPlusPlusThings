
#include <string.h>
#include "mac_common.h"
#include "plc_cnst.h"
#include "mem_zc.h"
#include "printf_zc.h"

#include <csma.h>
#include <plc.h>


#include <wl_csma.h>
#include <wlc.h>


maintain_nettick_func maintain_net_tick;

/* get time solt */
beacon_t * hplc_pop_beacon(list_head_t* pBeaconList)
{
    uint32_t cpu_sr;
    beacon_t *beacon;
    cpu_sr = OS_ENTER_CRITICAL();
    if(pBeaconList->next == NULL)
    {
        INIT_LIST_HEAD(pBeaconList);
    }
    if (!list_empty(pBeaconList)) {
        beacon = list_entry(pBeaconList->next, beacon_t, link);
        list_del(pBeaconList->next);
        debug_printf(&dty, DEBUG_COORD, "pop beacon %d (%d %d, %d)\n",beacon->nr_bpts,PHY_TICK2MS(beacon->start), PHY_TICK2MS(beacon->end), PHY_TICK2MS(get_phy_tick_time()));
    } else {
        beacon = NULL;
    }

    OS_EXIT_CRITICAL(cpu_sr);
    return beacon;
}
/*staticvoid genal_drain_beacon(list_head_t *pBeaconList, beacon_t *pCurBeacon, U8 CurFlag)
{
    uint32_t cpu_sr;
    list_head_t *pos, *n;
    beacon_t *beacon;
    cpu_sr = OS_ENTER_CRITICAL();
    list_for_each_safe(pos, n, pBeaconList) {
        beacon = list_entry(pos, beacon_t, link);
        list_del(pos);
        beacon->put(beacon);
    }
    if (pCurBeacon && CurFlag)
    {
        pCurBeacon->put(pCurBeacon);
        pCurBeacon = NULL;
    }
    OS_EXIT_CRITICAL(cpu_sr);
    return;
}
*/ 
void hplc_drain_beacon(U8 CurFlag)
{
//    genal_drain_beacon(&BEACON_LIST, CURR_BEACON, CurFlag);
    uint32_t cpu_sr;
    list_head_t *pos, *n;
    beacon_t *beacon;
    cpu_sr = OS_ENTER_CRITICAL();
    list_for_each_safe(pos, n, &BEACON_LIST) {
        beacon = list_entry(pos, beacon_t, link);
        list_del(pos);
        beacon->put(beacon);
    }
    if (CURR_BEACON && CurFlag)
    {
        CURR_BEACON->put(CURR_BEACON);
        CURR_BEACON = NULL;
    }
    OS_EXIT_CRITICAL(cpu_sr);
    return;
}

void wl_drain_beacon(U8 CurFlag)
{
//    genal_drain_beacon(&WL_BEACON_LIST, WL_CURR_BEACON, CurFlag);
    uint32_t cpu_sr;
    list_head_t *pos, *n;
    beacon_t *beacon;
    cpu_sr = OS_ENTER_CRITICAL();
    list_for_each_safe(pos, n, &WL_BEACON_LIST) {
        beacon = list_entry(pos, beacon_t, link);
        list_del(pos);
        beacon->put(beacon);
    }
    if (WL_CURR_BEACON && CurFlag)
    {
        WL_CURR_BEACON->put(WL_CURR_BEACON);
        WL_CURR_BEACON = NULL;
    }
    OS_EXIT_CRITICAL(cpu_sr);
    return;
}


void Clean_All_time_slot(U8 CurFlag)
{
    //清空载波时隙
    hplc_drain_beacon(CurFlag);
    //清空无线时隙
    wl_drain_beacon(CurFlag);

}

#if defined(DATALINKDEBUG_SW)
#define max_msg_record_num 20
typedef struct msg_trace_s
{
    U8 func ;  //函数
    U8 opty;  //操作类型
    U8 res[2];


    U32 call_tick;  //函数的调用tick
    U32 execu_tick; //操作链表的tick

    U16 nrbefor;   //执行前条目数
    U16 nrafter;  //执行后条目数


}msg_trace_t;

typedef struct msg_trace_buf_s
{
    U8  index;
    msg_trace_t msg_trace[max_msg_record_num];

}msg_trace_buf_t;

msg_trace_buf_t msg_trace_buf={0};

#endif

void msg_trace_show(tty_t *term)
{
#if defined(DATALINKDEBUG_SW)
      U8 i=0;
      term->op->tprintf(term,"idx\topty\tfunc\tbfr\taft\tcall\t\texec\n");
      for(i=0;i<max_msg_record_num;i++)
      {
        term->op->tprintf(term,"%d\t%s\t%d\t%d\t%d\t%08x\t%08x\n",i
                                        ,msg_trace_buf.msg_trace[i].opty==add_head?"AdHd":
                                        msg_trace_buf.msg_trace[i].opty==add_lid?"AdLd":
                                            msg_trace_buf.msg_trace[i].opty==del_lid?"Deld":
                                                msg_trace_buf.msg_trace[i].opty==cln_list?"clen":"uk"
                                        ,msg_trace_buf.msg_trace[i].func
                                        ,msg_trace_buf.msg_trace[i].nrbefor
                                        ,msg_trace_buf.msg_trace[i].nrafter
                                        ,msg_trace_buf.msg_trace[i].call_tick
                                        ,msg_trace_buf.msg_trace[i].execu_tick);
      }
#endif
}

//op=0表示删除,1表示增加
void msg_trace_record_func(U8 op ,tx_link_t *tx_link,U8 func)
{
#if defined(DATALINKDEBUG_SW)
    if(tx_link !=&TX_MNG_LINK)
    {
        return;
    }
    if(TX_MNG_LINK.nr >= TX_MNG_LINK.thr && op==1)
    {
        return;
    }
    msg_trace_buf.index = (msg_trace_buf.index+1)%max_msg_record_num;
    msg_trace_buf.msg_trace[msg_trace_buf.index].func = func;
    msg_trace_buf.msg_trace[msg_trace_buf.index].call_tick =
            msg_trace_buf.msg_trace[msg_trace_buf.index].execu_tick= get_phy_tick_time();
    msg_trace_buf.msg_trace[msg_trace_buf.index].nrbefor =
            msg_trace_buf.msg_trace[msg_trace_buf.index].nrafter = TX_MNG_LINK.nr;
#endif

}
void msg_trace_record_execute(U8 opty,tx_link_t *tx_link)
{
#if defined(DATALINKDEBUG_SW)
    if(tx_link !=&TX_MNG_LINK)
    {
        return;
    }
    msg_trace_buf.msg_trace[msg_trace_buf.index].execu_tick= get_phy_tick_time();
    msg_trace_buf.msg_trace[msg_trace_buf.index].nrafter = TX_MNG_LINK.nr;
    msg_trace_buf.msg_trace[msg_trace_buf.index].opty = opty;
#endif
}



void tx_link_clean(tx_link_t *tx_link)
{
   uint32_t cpu_sr;
   list_head_t *pos,*node;
   mbuf_hdr_t  *mbuf_n;
   cpu_sr = OS_ENTER_CRITICAL();
//   msg_trace_record_func(0,tx_link,2);
   list_for_each_safe(pos,node,&tx_link->link) {
       mbuf_n = list_entry(pos, mbuf_hdr_t, link);
       list_del(pos);
       zc_free_mem(mbuf_n);
       --tx_link->nr;
   }
   msg_trace_record_execute(cln_list,tx_link);
   OS_EXIT_CRITICAL(cpu_sr);
}
/*initial a list*/
 void phy_tx_link_init(tx_link_t *tx_list,uint16_t thr)
 {
	 memset(tx_list, 0, sizeof(tx_link_t));
	 tx_list->thr = thr;
	 INIT_LIST_HEAD(&tx_list->link);	
 }
 /*Insert an entry into the queue head*/
  int32_t tx_link_enq_head(list_head_t *link,tx_link_t *tx_list)
 {
	 uint32_t cpu_sr;
	/*insert list head*/
	
	cpu_sr = OS_ENTER_CRITICAL();
	list_add(link,&tx_list->link);
	tx_list->nr++;
	OS_EXIT_CRITICAL(cpu_sr);
	return 0;
 }



 /*Insert an entry into the queue*/
  int32_t tx_link_enq_by_lid(mbuf_hdr_t *mbuf,tx_link_t *tx_list)
 {
 	uint32_t cpu_sr;
	list_head_t *pos,*node,*p;
	mbuf_hdr_t  *mbuf_n;

	if(!mbuf)
		return -DATA_NULL;
	
#if STATIC_NODE
	if(!CURR_BEACON)
		packet_csma_bpts(NULL);
#endif
	
	
	cpu_sr = OS_ENTER_CRITICAL();
	/*over threshold , drop the msg*/
	if(tx_list->nr >= tx_list->thr)
	{
		OS_EXIT_CRITICAL(cpu_sr);
		return -LIST_FULL;
	}
	
	/*insert list by lid*/
    list_for_each_safe_reverse(pos, node,&tx_list->link) {
		mbuf_n = list_entry(pos, mbuf_hdr_t, link);
        if(time_before_eq(mbuf->buf->lid,mbuf_n->buf->lid))
		{
			p= &mbuf_n->link;
			goto ok;
		}
	}
	p=&tx_list->link;
	
   ok:
   	mbuf->buf->fi.pi.link = tx_list;
	
	list_add_tail(&mbuf->link, p);
	
	++tx_list->nr;
	
	OS_EXIT_CRITICAL(cpu_sr);
	return ENTERY_OK;
 }
/*get an entry from the queue*/
 mbuf_hdr_t *tx_link_deq(tx_link_t *tx_link, U8 phase)
{
    if(list_empty(&tx_link->link))
    {
        return NULL;
    }
 	uint32_t cpu_sr;
	list_head_t *pos,*node;
	mbuf_hdr_t  *mbuf_n;
	cpu_sr = OS_ENTER_CRITICAL();
	list_for_each_safe(pos,node,&tx_link->link) {
		mbuf_n = list_entry(pos, mbuf_hdr_t, link);

        if((phase != PHASE_NONE) && 
           (mbuf_n->phase != phase) && 
           (mbuf_n->phase != PHASE_NONE))
        {
            continue;
        }

		list_del(&mbuf_n->link);
		--tx_link->nr;
		OS_EXIT_CRITICAL(cpu_sr);
		return mbuf_n;
	}
	OS_EXIT_CRITICAL(cpu_sr);
	return NULL;
}


__LOCALTEXT __isr__ void bpts_pushback_pf(mbuf_hdr_t *mpdu)
{
	if(!mpdu)
		return;

	if(mpdu->buf->retrans>0 && mpdu->buf->fi.pi.link)
	{
//        printf_s("mpdu = %08x, mpdu->link : 0x%08x ; pi.link : 0x%08x\n",data_to_mem(mpdu, 0), &mpdu->link,mpdu->buf->fi.pi.link);
		// msg_trace_record_func(1,mpdu->buf->fi.pi.link,0);
		tx_link_enq_head(&mpdu->link,mpdu->buf->fi.pi.link);
		return;
	}

     if(mpdu->buf->cnfrm)
     {
        MACDATA_CONFIRM_t *pCfm = (MACDATA_CONFIRM_t *)mpdu->buf->cnfrm->buf;
        pCfm->status = mpdu->buf->acki?e_NO_ACK:e_SUCCESS;
         if(pCfm->status == e_NO_ACK)
         {
             pCfm->MpduData = mpdu;
             post_plc_work(mpdu->buf->cnfrm);
             return;
         }
         pCfm->MpduData = NULL;
        //printf_s("post cfm:%p\n", mpdu->buf->cnfrm);
        post_plc_work(mpdu->buf->cnfrm);
     }
	zc_free_mem(mpdu);
}

__LOCALTEXT  uint32_t backoff_mask(uint8_t backoff,uint8_t priority)
{	
	uint32_t randpma;
	int32_t pma,i;
	pma = 2;

	if(backoff > 8)
	{
		return 0;
	}
	for(i=2;i<=backoff;i++){
		pma  *= 2;
	}	

	randpma = rand()%pma + (10-priority*2);

	//phy_printf("<%d>",randpma);
	
	return (randpma *= (10-priority*2));
}

sync_func sync_func_deal;
void register_sync_func(sync_func func)
{
    sync_func_deal = func;
}
void register_nettick_func(maintain_nettick_func func)
{
    maintain_net_tick = func;
}

uint8_t check_bpts_op(void *opAddr)
{
    if(opAddr == (void *)&csma_send)
        return TRUE;
    if(opAddr == (void *)&tdma_recv)
        return TRUE;
    if(opAddr == (void *)&wl_csma_send)
        return TRUE;
    if(opAddr == (void *)&wl_tdma_recv)
        return TRUE;
    #if defined(STATIC_MASTER)
    if(opAddr == (void *)&wl_beacon_send)
        return TRUE;
    if(opAddr == (void *)&beacon_send)
        return TRUE;
    #else
    if(opAddr == (void *)&rbeacon_send)
        return TRUE;
    if(opAddr == (void *)&wl_rbeacon_send)
        return TRUE;
    #endif


    return FALSE;
}

plc_post_func pPlcPostMsg;

void register_plc_msg_func(plc_post_func func)
{
	pPlcPostMsg = func;
}
void post_plc_work(work_t *work)
{
	if(pPlcPostMsg)
    {
		pPlcPostMsg(work);
	}
	else
	{
        zc_free_mem(work);		
	}
	return;
}
