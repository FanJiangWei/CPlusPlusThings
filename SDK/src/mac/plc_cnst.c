/*
 * Copyright: (c) 2016-2020, 2020 zc Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc_cnst.c
 * Purpose:	PLC construct frame for net-layer.
 * History:
 *
 */
#include "plc_cnst.h"
#include "plc.h"
#include "phy_sal.h"
#include "csma.h"
#include "mem_zc.h"
#include "framectrl.h"
#include "phy_sal.h"
#include "printf_zc.h"
#include "timeslot_equalization.h"

#include "Beacon.h"
#include "DataLinkGlobal.h"

#include "ZCsystem.h"

#include "DataLinkInterface.h"
#include "app_sysdate.h"
#include "Datalinkdebug.h"
#include "DatalinkTask.h"
#include "SchTask.h"
#include "Scanbandmange.h"


uint8_t plc_can_not_send_message()
{
    if( HPLC.testmode == PHYTPT || 
        HPLC.testmode == PHYCB || 
        HPLC.testmode == MACTPT || 
        HPLC.testmode == RF_PLCPHYCB || 
        HPLC.testmode == PLC_TO_RF_CB || 
        HPLC.testmode == TEST_MODE)
    {
        return TRUE;
    }
    // if(!(HPLC.worklinkmode & 0x01))
    // {
    //     return TRUE;
    // }

    return FALSE;
}

static U8 get_tmi(U8 tmi)
{
#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
    return ((tmi & 0xf) != 0xD) ? tmi : (16 | ((tmi >> 4) & 0x0f));
#elif defined(STD_2016) || defined(STD_DUAL)
    return ((tmi & 0xf) != 0xF) ? tmi : (16 | ((tmi >> 4) & 0x0f));
#endif
}

static __inline__ int32_t get_pi_by_len_dt(phy_pld_info_t *pi,uint8_t *tmi,uint16_t *pb_len,uint16_t len,uint8_t dt,uint8_t band)
{
    tonemap_t *tm;
    *tmi = 0xff;
    switch (band)
    {
        case 0:
#if defined(STD_2016) || defined(STD_DUAL)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?9:
            (len>PB264_BODY_SIZE)?9:
            (len>PB136_BODY_SIZE)?12:
            (len>PB72_BODY_SIZE)?6:
            13;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE_NO_CRC_BEACON)?0xff:
            (len>PB136_BODY_SIZE_NO_CRC_BEACON)?9:6;
#elif defined(STD_SPG) || defined(STD_GD) || defined(STD_2012)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?1:
            (len>PB136_BODY_SIZE)?9:
            4;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE)?0xff:
            (len>PB136_BODY_SIZE)?9:4;
#endif
            break;
        case 1:
#if defined(STD_2016) || defined(STD_DUAL)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?1:
            (len>PB264_BODY_SIZE)?9:
            (len>PB136_BODY_SIZE)?12:
            (len>PB72_BODY_SIZE)?6:
            13;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE_NO_CRC_BEACON)?0xff:
            (len>PB136_BODY_SIZE_NO_CRC_BEACON)?9:6;
#elif defined(STD_SPG) || defined(STD_GD) || defined(STD_2012)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?1:
            (len>PB136_BODY_SIZE)?9:
            4;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE)?0xff:
            (len>PB136_BODY_SIZE)?9:4;
#endif
            break;
        case 2:
#if defined(STD_2016) || defined(STD_DUAL)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?1:
            (len>PB264_BODY_SIZE)?9:
            (len>PB136_BODY_SIZE)?12:
            (len>PB72_BODY_SIZE)?6:
            13;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE_NO_CRC_BEACON)?0xff:
            (len>PB136_BODY_SIZE_NO_CRC_BEACON)?9:6;
#elif defined(STD_SPG) || defined(STD_GD) || defined(STD_2012)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?1:
            (len>PB136_BODY_SIZE)?9:
            4;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE)?0xff:
            (len>PB136_BODY_SIZE)?9:4;
#endif
            break;
        case 3:
#if defined(STD_2016) || defined(STD_DUAL)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?0x6F:
            (len>PB264_BODY_SIZE)?1:
            (len>PB136_BODY_SIZE)?11:
            (len>PB72_BODY_SIZE)?6:
            13;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE_NO_CRC_BEACON)?0xff:
                (len>PB136_BODY_SIZE_NO_CRC_BEACON)?1:6;
#elif defined(STD_SPG) || defined(STD_GD) || defined(STD_2012)
            if(dt == DT_SOF)
            *tmi=(len>4*PB520_BODY_SIZE)?0xff:
            (len>PB520_BODY_SIZE)?1:
            (len>PB136_BODY_SIZE)?9:
            4;
            else if(dt == DT_BEACON)
            *tmi=(len>PB520_BODY_SIZE)?0xff:
            (len>PB136_BODY_SIZE)?9:4;
#endif
            break;
        default :
            
            //break;
            return -6;
    }

	if(*tmi == 0xff)
	{
		return -1;
	}
    if (!(tm = get_sys_tonemap((uint32_t)(get_tmi(*tmi)))))
    {
        return -2;
    }
    pi->pbsz     = tm->pbsz;
    pi->fec_rate = tm->fec_rate;
    pi->copy     = tm->copy;
    pi->mm       = tm->mm;
    pi->robo     = tm->robo;
    pi->tmi      = tm->tmi;
    pi->link     = NULL;

    if(dt == DT_SOF)
    {
    	pi->pbc      = len/body_size[pi->pbsz] + (len%body_size[pi->pbsz]?1:0);
        pi->crc32_en  = 0;
        pi->crc32_ofs = 0;
    }
    else if(dt == DT_BEACON)
    {
        pi->pbc  = 1;
        pi->crc32_en  = 1;
        pi->crc32_ofs = 7;
    }


    if(len == 0 || pi->pbc == 0)
    {
        return -3;
    }
    //over throld,return fail;
    if (pi->pbc && (pi->pbc < tm->pbc_min || pi->pbc > tm->pbc_max))
    {
        return -4;
    }

    if (tm->mm)
    {
        pi->symn = robo_pb_to_pld_sym(pi->pbc, tm->robo);
        pi->bat  = NULL;
    }
    else
    {
        pi->symn = robo_pb_to_pld_sym(pi->pbc, tm->robo);
        pi->bat  = tm->bat;
    }
    *pb_len = pi->pbc*pb_bufsize(pi->pbsz);


    if(pi->symn>0x1ff)
        return -5;

    return 0;
}
uint16_t packet_beacon_paload(uint8_t *payload, U8 phase, U8 SimpBeaconFlag)
{
#if defined(STATIC_MASTER)
     //调用信标帧组包接口
    if(SimpBeaconFlag == 0)     //标准信标帧
    {
        return PackCenternBeacon(payload, phase);
    }
    else                        //无线精简信标帧
    {
        return PackCenternBeaconSimp(payload, phase);
    }
#else

    uint16_t i;
     for(i=0;i<512;i++)
     {
         payload[i]=i;
     }
    return 512;
#endif
}

/*payload construct infomation fill into fch*/
static int32_t construct_plc_payload_frame(mbuf_t *mbuf ,uint8_t *payload,uint16_t len,phy_pld_info_t *pi)
{
	uint8_t dt,i;
    phy_frame_info_t *pf;
    bph_t *bph;
    pbh_t *pbh;
	frame_ctrl_t *fc ;
	
	fc= mbuf->fi.head;
	dt =  get_frame_dt(fc);
	pf =  &mbuf->fi;
	__memcpy(&mbuf->fi.pi,pi,sizeof(phy_pld_info_t));
	
	pf->length	= pi->pbc * pb_bufsize(pi->pbsz);

			  
	if (dt == DT_BEACON)
    {
        bph        = (bph_t *)pf->payload;
        fill_pb_desc(bph, 1, 1, pi->pbsz, 1, 0);
//        bph->som   = 1;
//        bph->eom   = 1;
//        bph->pbsz  = pi->pbsz;
//        bph->crc   = TRUE;//for tx crc32 - flag. ture:cal fill in pos(own 4bytes+129/513+crc32+crc)
//        bph->crc32 = 0;   // for tx no mean
        if(pi->pbc  !=  1)
        {
            return -3;
        }
        __memcpy(pf->payload + sizeof(pb_desc_t),payload,len);
    }else{
	   pbh = (pbh_t *)pf->payload;
	   for (i = 0; i < pi->pbc; ++i)
	   {
//		   pbh->som  = i == 0;
//		   pbh->eom  = i == pi->pbc - 1;
//		   pbh->pbsz = pi->pbsz;
//		   pbh->crc  = FALSE; //it can't set true,body_size[pbh->pbsz] last 4 bytes be set crc32 .
           fill_pb_desc(pbh, i == 0, i == pi->pbc - 1, pi->pbsz, 0, 0);
           pbh->ssn  = i;
	   	   #if defined(STD_SPG)
			   pbh->resv = 0;
           #elif defined(STD_2016) || defined(STD_DUAL)
			   pbh->mfsf = i == 0;
			   pbh->mfef = i == pi->pbc - 1;
	  	   #endif
		   
           __memcpy((unsigned char *)pbh + sizeof(pbh_t),payload + i * (body_size[pi->pbsz]),len>body_size[pi->pbsz]?body_size[pi->pbsz]:len);
           len -= body_size[pi->pbsz];
		   //dump_zc(1,"aa->",DEBUG_PHY,(unsigned char *)pbh + sizeof(pbh_t), pb_bufsize(pi->pbsz));
		   pbh = (void *)pbh + pb_bufsize(pi->pbsz);
	   }
    }
	dhit_write_back((uint32_t)pf->payload, pf->length);
	return 0;
}




mbuf_hdr_t *packet_beacon(uint32_t stime,uint8_t phase,uint8_t *payload,uint16_t len,uint32_t bpc)
{
	uint16_t pb_len;
	uint8_t tmi;
	phy_pld_info_t pi;
	int32_t ret;

    if(plc_can_not_send_message())
    {
        return NULL;
    }

	if((ret=get_pi_by_len_dt(&pi,&tmi,&pb_len,len,DT_BEACON,CURR_B->idx))<0)
	{
		printf_s("ERROR pi:%d\n",ret);
		return NULL;
	}

	mbuf_hdr_t *mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t)+pb_len,"pbcn",MEM_TIME_OUT);

	mbuf_t *buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t)); 
	beacon_ctrl_t *fc = mbuf_hdr->buf->fi.head = (mbuf_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t)); 
	mbuf_hdr->buf->fi.payload = (mbuf_t *)((uint8_t *)mbuf_hdr->buf->fi.head + sizeof(frame_ctrl_t)); 

    buf->bandidx = CURR_B->idx;     //用于发送时判断当前频段是否合法。不合法丢弃数据

	buf->snid  = HPLC.snid;
	buf->lid   = 1;
	buf->stime = stime;
	buf->bct   = bpc;
	//buf->rgain;
	//buf->snr;
	//buf->snr_level
	//buf->acki		
	//buf->backoff	
	//buf->ufs		
	//buf->priority 
	//buf->retrans   
	//buf->res
	
	mbuf_hdr->work = NULL;
	
	fc->dt     = DT_BEACON;
    #if defined(STD_2016) || defined(STD_DUAL)
	fc->access = 0;
	fc->std    = 0;
	#elif  defined(STD_SPG)
	fc->access = 1;
	fc->std    = 1;
	fc->bpc    = bpc;
	#endif
	fc->snid   = HPLC.snid;
	fc->stei   = HPLC.tei; 
	fc->tmi    = tmi;
	fc->phase  = phase;
	fc->symn = pi.symn;

	mbuf_hdr->buf->fi.hi.symn = CURR_B->symn;
	mbuf_hdr->buf->fi.hi.mm   = CURR_B->mm;
	mbuf_hdr->buf->fi.hi.size = CURR_B->size;
	mbuf_hdr->buf->fi.hi.pts  = TRUE;	// if pts is true, then stime is decided by phy layer,
	mbuf_hdr->buf->fi.hi.pts_off = offset_of(beacon_ctrl_t, bts);

	mbuf_hdr->buf->cnfrm = NULL;
    mbuf_hdr->buf->retrans = 2;
	
	construct_plc_payload_frame(mbuf_hdr->buf,payload,len,&pi);	
	return mbuf_hdr;
}
plc_work_t *put_next_beacon(void)
{
	
	plc_work_t *mbuf_hdr;

	mbuf_hdr = zc_malloc_mem(sizeof(plc_work_t)+sizeof(mbuf_t),"NXBC",MEM_TIME_OUT);

	if(!mbuf_hdr)
        return NULL;
	
#if STATIC_MASTER
    //在下一次规划时隙之前计算网间协调需要退避的时间
#if defined(NEW_CCORD_ALG)
    BeaconStopOffset = GetBeaconOffset();
#endif
	mbuf_hdr->work = packet_beacon_period;
#elif STATIC_NODE
    mbuf_hdr->work = packet_csma_bpts;
#endif
	return mbuf_hdr;

}

#if 0//STATIC_NODE
void  put_beacon_inlist(mbuf_hdr_t *mbuf_hdr)
{	 uint32_t cpu_sr;
	/*insert list head*/
	cpu_sr = OS_ENTER_CRITICAL();
	list_add_tail(&mbuf_hdr->link,&BEACON_FRAME_LIST);
	OS_EXIT_CRITICAL(cpu_sr);
}
#endif 


mbuf_hdr_t * packet_sof(uint8_t priority,uint16_t dtei,uint8_t *payload,uint16_t len,uint8_t retrans ,uint8_t tf, uint8_t phase,plc_work_t *cnfrm)
{
	uint16_t pb_len;
	uint8_t tmi;
	phy_pld_info_t pi;
	int32_t ret;

    // if(plc_can_not_send_message())
    // {
    //     return NULL;
    // }
	
	if((ret = get_pi_by_len_dt(&pi,&tmi,&pb_len,len,DT_SOF,CURR_B->idx))<0)
	{
		printf_s("pi ret:%d\n",ret);
		return NULL;
	}

	mbuf_hdr_t *mbuf_hdr = NULL;

	//printf_s("pb_len : %d pbsz : %d\n",pb_len , pi.pbsz);
	
	mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t)+pb_len,"psof",MEM_TIME_OUT);

    if (mbuf_hdr == NULL)
    {
        return NULL;
    }
    

	mbuf_t *buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t)); 
	sof_ctrl_t *fc = mbuf_hdr->buf->fi.head = (mbuf_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t)); 
	mbuf_hdr->buf->fi.payload = (mbuf_t *)((uint8_t *)mbuf_hdr->buf->fi.head + sizeof(frame_ctrl_t)); 

	

	buf->snid  = HPLC.snid;
	buf->lid   = 1;
	//buf->stime;
	//buf->rgain;
	//buf->snr;
	//buf->snr_level
	buf->acki    = dtei==0xfff?0:1;	
	buf->backoff = 2;	
	//buf->ufs		
	buf->priority = priority;
    if(dtei == 0xfff)
    {
        buf->retrans = retrans;
    }
    else
    {
        buf->retrans  = retrans? retrans:3;
    }
	//buf->res

	mbuf_hdr->work = NULL;	

	fc->dt     = DT_SOF;
    #if defined(STD_2016) || defined(STD_DUAL)
	fc->access = 0;
	fc->bcsti  = buf->acki==0?1:0;
	fc->enc    = 0;
	fc->std    = 0;
	#elif defined(STD_SPG)
	fc->access  = 1;
	fc->rprq    = 0;
	fc->crc     = 0;
	fc->tf      = tf;
	fc->std    = 1;
	#endif
    fc->snid   = HPLC.snid;
	fc->stei   = HPLC.tei; 
	fc->dtei   = dtei;
	fc->lid    = buf->lid;
	fc->pbc    = pi.pbc;
	fc->repeat = 0;
	fc->tmi    = tmi&0xf;
	fc->tmiext = tmi>>4;
	fc->symn   = pi.symn;
	fc->fl     = pld_sym_to_time(fc->symn) + 
			PHY_CIFS  + 
		((get_sof_bcsti((sof_ctrl_t *)fc) == 1)?0:(SACK_FRAME_DUR(CURR_B->symn) + 
			PHY_RIFS));

//作为代理节点转发关联回复需要重发，应用层广播报文重发台体测试不通过。
        MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)payload;
        U16                     byCmdID = 0;

        if(pMacHeader->MacProtocolVersion == e_MAC_VER_STANDARD)
        {
            if(pMacHeader->MsduType == e_NETMMSG)
            {
                byCmdID = pMacHeader->Msdu[0] + (pMacHeader->Msdu[1] << 8);
                //针对路由请求，转发3次，防止台体收不到
                if(byCmdID == e_MMeRouteRequest)
                {
                    buf->retrans = 3;
                }
            }
                
        }


//    if(buf->cnfrm)      //作为代理节点转发关联回复需要重发，应用层广播报文重发台体测试不通过。
//    {
//        MACDATA_CONFIRM_t *pcfm = (MACDATA_CONFIRM_t *)buf->cnfrm->buf;
//        if(fc->dtei == 0xfff && pcfm->MsduType == e_APS_FREAME)
//            buf->retrans = 0;
//    }
	
	mbuf_hdr->buf->fi.hi.symn = CURR_B->symn;
	mbuf_hdr->buf->fi.hi.mm   = CURR_B->mm;
	mbuf_hdr->buf->fi.hi.size = CURR_B->size;

	#if defined(STATIC_MASTER)
    if(fc->dtei==0xfff && mbuf_hdr->phase == e_UNKNOWN_PHASE)   //如果是广播（不包括一跳关联回复），需要至少在每个相线都发一次
    {
        buf->retrans = buf->retrans < 3 ? 3 : buf->retrans;
    }
	#endif
	//HPLC.bpts_cnt = pb_len;

	construct_plc_payload_frame(mbuf_hdr->buf,payload,len,&pi);	


    mbuf_hdr->buf->cnfrm = cnfrm;
    mbuf_hdr->phase      = phase;           //设置数据发送相位
    mbuf_hdr->buf->bandidx = CURR_B->idx;     //用于发送时判断当前频段是否合法。不合法丢弃数据

	return mbuf_hdr;
}
phy_frame_info_t *packet_sack(sof_ctrl_t *head,uint8_t result,int8_t snr)
{
	HPLC.fi.head = HPLC.hdr;
	sack_ctrl_t *sack;

    memset(HPLC.hdr, 0, sizeof(HPLC.hdr));

	sack = HPLC.fi.head;

	sack->dt	 = DT_SACK;
	
	#if defined(STD_SPG)
	sack->access = 1;
	sack->std    = 1;
    #elif defined(STD_2016) || defined(STD_DUAL)
 	sack->stei   = head->dtei;
	sack->access = 0;
	sack->load	 = TX_MNG_LINK.nr+TX_DATA_LINK.nr;
	sack->ext_dt = 0;		
	sack->snr	 = snr;//need set 
	sack->std    = 0;
	#endif
	sack->snid	 = head->snid;
	sack->result = result&0xf;	
 	sack->state	 = result>>4;
	sack->dtei	 = head->stei;
	sack->pbc	 = head->pbc;

	HPLC.fi.hi.symn = CURR_B->symn;
	HPLC.fi.hi.mm   = CURR_B->mm;
	HPLC.fi.hi.size = CURR_B->size;
    HPLC.fi.hi.pts = 0;                 //载波和无线ACK共用全局变量组帧。防止出现错误，需要手动清空标志

	return &HPLC.fi;
}

#if STATIC_MASTER
mbuf_hdr_t * packet_coord(uint16_t dur,uint16_t bso ,uint32_t neighbour,uint8_t bef,uint16_t beo)
{
	mbuf_hdr_t *mbuf_hdr;
	mbuf_t *buf;
	coord_ctrl_t *fc;

    if(plc_can_not_send_message())
    {
        return NULL;
    }
	
	mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t),"CORD",MEM_TIME_OUT);
	buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t)); 
	fc = mbuf_hdr->buf->fi.head = (mbuf_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t)); 
	 

	buf->snid  = HPLC.snid;
	buf->lid   = 1;
	//buf->stime;
	//buf->rgain;
	//buf->snr;
	//buf->snr_level
	buf->acki = 0; 
	//buf->backoff;	
	//buf->ufs		
	//buf->priority;
	buf->retrans = 1; 
	//buf->res

	mbuf_hdr->work = NULL;	
	
	fc->dt	   = DT_COORD;
    #if defined(STD_2016) || defined(STD_DUAL)
	fc->access = 0;
	fc->std   = 0;
	#elif defined(STD_SPG)
	fc->access = 1;
	fc->bef = bef;
	fc->beo = beo;
	fc->std   = 1;
	#endif
	fc->snid   = HPLC.snid;
	fc->dur   = dur;
	fc->bso   = bso;
	fc->neighbour   = neighbour;
#if defined(STD_DUAL)
#ifdef HPLC_HRF_PLATFORM_TEST

    fc->rfchan_id = getHplcRfChannel();//WPHY_CHANNEL;
    fc->rfoption  = getHplcOptin();
#endif
#endif

	mbuf_hdr->buf->fi.hi.symn = CURR_B->symn;
	mbuf_hdr->buf->fi.hi.mm   = CURR_B->mm;
	mbuf_hdr->buf->fi.hi.size = CURR_B->size;
	
	mbuf_hdr->buf->cnfrm = NULL;
	
	mbuf_hdr->buf->fi.pi.link = NULL;
    buf->bandidx = CURR_B->idx;     //用于发送时判断当前频段是否合法。不合法丢弃数据

	return mbuf_hdr;
}

#endif
/*给抄控器用*/
mbuf_hdr_t * packet_sack_ctl(uint8_t ext_dt,uint8_t *addr ,uint16_t stei,uint32_t snid)
{
	mbuf_hdr_t *mbuf_hdr;
	mbuf_t *buf;
	frame_ctrl_t *fc;
	sack_search_ctrl_t *fc_srch;
	sack_syh_ctrl_t *fc_syh;
	
	mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t),"SCKC",MEM_TIME_OUT);
	buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t)); 
	fc = mbuf_hdr->buf->fi.head = (mbuf_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t)); 
	fc_srch = (sack_search_ctrl_t *)fc;
	fc_syh = (sack_syh_ctrl_t *)fc;

	buf->snid  = snid;
	buf->lid   = 1;
	//buf->stime;
	//buf->rgain;
	//buf->snr;
	//buf->snr_level
	buf->acki = 0; 
	//buf->backoff;	
	//buf->ufs		
	//buf->priority;
	buf->retrans = 1; 
	//buf->res

	mbuf_hdr->work = NULL;	

	fc->dt	   = DT_SACK;
    #if defined(STD_2016) || defined(STD_DUAL)
	fc->access = 0;
	fc->std   = 0;
	#elif defined(STD_SPG)
	fc->access = 1;
	fc->std   = 1;	
	#endif
	
	fc_srch->ext_dt = ext_dt;

	if(ext_dt == 1)
	{
		__memcpy(fc_srch->addr,addr,6);
		fc_srch->stei = stei;
		#if defined(STD_SPG)
		fc_srch->result = 0;
		fc_srch->state= 0;
		fc_srch->dtei = 0;//need set 
		fc_srch->pbc = 0;
		#endif
	}
	else if(ext_dt == 2)
	{
		fc_syh->stei = stei;
        fc_syh->snid = snid;
		mbuf_hdr->buf->fi.hi.pts  = TRUE;	// if pts is true, then stime is decided by phy layer,
		mbuf_hdr->buf->fi.hi.pts_off = offset_of(sack_syh_ctrl_t, ts);
		#if defined(STD_SPG)
		fc_srch->result = 0;
		fc_srch->state= 0;
		fc_srch->dtei = 0;//need set 
		fc_srch->pbc = 0;
		#endif
	}

	
	mbuf_hdr->buf->fi.hi.symn = CURR_B->symn;
	mbuf_hdr->buf->fi.hi.mm   = CURR_B->mm;
	mbuf_hdr->buf->fi.hi.size = CURR_B->size;
    mbuf_hdr->phase           = PHASE_NONE;

	return mbuf_hdr;
}




void packet_beacon_period(work_t *work)
{

#if defined(STATIC_MASTER)
	U32 tick_offset = 0;
	tick_offset = get_phy_tick_time();
    uint16_t i;
    uint32_t nowtimetick;
    BEACON_PERIOD_t *beacon_info;
    HPLC.noise_proc++;

    beacon_info = zc_malloc_mem(sizeof(BEACON_PERIOD_t),"PROD",MEM_TIME_OUT);

    //获取系统当前时间
    SysDate_t SysDate = {0};
	GetBinTime(&SysDate);

    //TODO  获取时隙信息。

    if(g_TimeSlotParam.pNonCenterInfo)
        zc_free_mem(g_TimeSlotParam.pNonCenterInfo);


    memset(&g_TimeSlotParam, 0, sizeof(TIME_SLOT_PARAM_t));

    g_TimeSlotParam.pNonCenterInfo = (U8 *)zc_malloc_mem(1024, "NCBC", MEM_TIME_OUT);

    getSlotInfoForBeacon(&g_TimeSlotParam);

    //信标周期在时隙规划完成后自加,这个位置是否可以先自加
    NwkBeaconPeriodCount++;


    beacon_info->beacon_slot = g_TimeSlotParam.beacon_slot;
    beacon_info->beacon_slot_nr = g_TimeSlotParam.beacon_slot_nr;

    nowtimetick = get_phy_tick_time();
//    beacon_info->beacon_start_time = time_after_eq(g_TimeSlotParam.beacon_start, nowtimetick) ? g_TimeSlotParam.beacon_start : nowtimetick + PHY_US2TICK(1500);
    beacon_info->beacon_start_time = g_TimeSlotParam.beacon_start ? g_TimeSlotParam.beacon_start : nowtimetick + PHY_US2TICK(1500);

    net_printf("[%02d-%02d-%02d %02d:%02d:%02d]PCNT =0x%08x Ns=%02x,NRT=%d ",
    SysDate.Year+2000,SysDate.Mon,SysDate.Day,SysDate.Hour,SysDate.Min,SysDate.Sec,
    NwkBeaconPeriodCount,nnib.FormationSeqNumber,nnib.NextRoutePeriodStartTime);
    net_printf("nNum=%dbea_nr=%dslot=%dA=%dB=%dC=%d\n",
    DeviceListHeader.nr, beacon_info->beacon_slot_nr, beacon_info->beacon_slot
    , g_TimeSlotParam.csma_slot_a, g_TimeSlotParam.csma_slot_b, g_TimeSlotParam.csma_slot_c);


    beacon_info->beacon_sent_nr = 0;
    beacon_info->ext_bpts = 0;


    beacon_info->csma_duration_a = g_TimeSlotParam.csma_slot_a;
    beacon_info->csma_duration_b = g_TimeSlotParam.csma_slot_b;
    beacon_info->csma_duration_c = g_TimeSlotParam.csma_slot_c;
    beacon_info->csma_slot  = g_TimeSlotParam.csma_time_slot;

    beacon_info->bcsma_duration_a = 0;
    beacon_info->bcsma_duration_b = 0;
    beacon_info->bcsma_duration_c = 0;
    beacon_info->bcsma_slot = 0;
    beacon_info->bcsma_lid    = 0;

    beacon_info->beacon_curr = NULL;

    //设置CCO 发送无线信标类型
    beacon_info->RfBeaconType = g_TimeSlotParam.RfBeaconType;
    beacon_info->RfBeaconSlotCnt = g_TimeSlotParam.RfBeaconSlotCnt;
    beacon_info->CCObcn_slot_nr = g_TimeSlotParam.CCObcn_slot_nr;

    if(beacon_info->RfBeaconSlotCnt > 4)
    {
        beacon_info->beacon_slot_nr += (beacon_info->RfBeaconSlotCnt - 4);
        beacon_info->CCObcn_slot_nr += (beacon_info->RfBeaconSlotCnt - 4);


        g_TimeSlotParam.beacon_slot_nr = beacon_info->beacon_slot_nr;
        g_TimeSlotParam.CCObcn_slot_nr = beacon_info->CCObcn_slot_nr;
    }

    NON_CENTRAL_BEACON_INFO_t *pNonData = NULL;
    for(i = 0; i<beacon_info->beacon_slot_nr; i++)
    {
        beacon_info->beacon_slot_info[i].type  = 0;
//        beacon_info->beacon_slot_info[i].phase = i < 3 ? (i +1) : 1;//0==i?1:1==i?2:2==i?3:1;
        if(i < g_TimeSlotParam.CCObcn_slot_nr)
        {
            beacon_info->beacon_slot_info[i].phase = (i +1) % 4;
        }
        else
        {
            pNonData = (NON_CENTRAL_BEACON_INFO_t *)(g_TimeSlotParam.pNonCenterInfo + ((i-g_TimeSlotParam.CCObcn_slot_nr) * sizeof(NON_CENTRAL_BEACON_INFO_t)));
            beacon_info->beacon_slot_info[i].phase = getNextTeiPhase(pNonData->SrcTei);

        }
    }

    net_printf("%p,RfBeaconType:%d, RfBcnSlotCnt:%d, beacon_slot_nr:%d, ccobcnCnt:%d\n"
                ,pNonData, beacon_info->RfBeaconType, beacon_info->RfBeaconSlotCnt, beacon_info->beacon_slot_nr,beacon_info->CCObcn_slot_nr);


//    phy_printf("register_beacon_slot    beacon start time : %d ms \n", PHY_TICK2MS(beacon_info->beacon_start_time));
    if(getHplcTestMode() == NORM)
        register_beacon_slot(beacon_info);

    zc_free_mem(beacon_info);

    //时隙分配完成需要释放，非中央信息地址需要释放
    zc_free_mem(g_TimeSlotParam.pNonCenterInfo);
    g_TimeSlotParam.pNonCenterInfo = NULL;

//    //信标周期在时隙规划完成后自加,这个位置是否可以先自加
//    NwkBeaconPeriodCount++;

    //刷新信标发送周期定时器  保护10s 如果链路层任务挂掉会自动复位，否则任务丢失使用定时器触发
//    modify_SendBenconPeriodTimer(15*1000);
    modify_sendcenterbeacon_timer(20*1000 + nnib.BeaconPeriodLength);



    //在信标时隙开始修改频段，防止组帧时tmi出现问题。
    if(TRUE == nnib.BandChangeState)
    {
        if(nnib.BandRemianTime ==0)
        {
            DefwriteFg.FREQ = TRUE;
            DefaultSettingFlag = TRUE;
            changeband(DefSetInfo.FreqInfo.FreqBand);
            nnib.BandChangeState =0;
        }
    }

    if(GlRFChangeOCtimes > 0)
    {
        GlRFChangeOCtimes--;
    }
    else if(GlRFChangeOCtimes < 0)
    {
        GlRFChangeOCtimes = OC_CHANGE_TIMES;
    }

    //在信标时隙开始修改频段，防止组帧时mcs出现问题。
    if(TRUE == nnib.RfChannelChangeState)
    {
        if(nnib.RfChgChannelRemainTime == 0)
        {
            HPLC.option = DefSetInfo.RfChannelInfo.option;
            HPLC.rfchannel = DefSetInfo.RfChannelInfo.channel;
            net_printf("set rfchannel:<%d,%d>\n",HPLC.option, HPLC.rfchannel);
            nnib.RfChannelChangeState = 0;
            DefwriteFg.RfChannel = TRUE;
            DefaultSettingFlag = TRUE;
            GlRFChangeOCtimes = OC_CHANGE_TIMES;
            DstCoordRfChannel = 0xff;
        }
    }
    // extern xsh_t g_vsh;
    // extern void docmd_os(command_t *cmd, xsh_t *xsh);
    // g_vsh.argv[0] = "os";
    // g_vsh.argv[1] = "top";
    // command_t cmd;
    // docmd_os(&cmd,&g_vsh);
     net_printf("bct : %d\n",PHY_TICK2US(time_diff(get_phy_tick_time(), tick_offset)));


#endif
}



#if STATIC_NODE

void packet_csma_bpts(plc_work_t *mbuf_hdr)
{
	BEACON_PERIOD_t *beacon_info;
	mbuf_t *buf;
    U16 i;

	if(mbuf_hdr)
		buf = (mbuf_t *)mbuf_hdr->buf;
	else
		buf = NULL;
	if(buf&&time_after(get_phy_tick_time(),buf->stime))
	{
		debug_printf(&dty, DEBUG_COORD, "drop csma %dms\n",PHY_TICK2MS(get_phy_tick_time()-buf->stime));	
		return;
	}

	beacon_info = zc_malloc_mem(sizeof(BEACON_PERIOD_t),"PROD",MEM_TIME_OUT);

	beacon_info->beacon_start_time = buf?buf->stime:get_phy_tick_time();

    debug_printf(&dty, DEBUG_COORD, "bcn start (%d)\n",PHY_TICK2MS(beacon_info->beacon_start_time));

	beacon_info->ext_bpts = 1;

    beacon_info->beacon_slot     = GetBeaconTimeSlot();

    extern ostimer_t *g_SendsyhtimeoutTimer;  //配合抄控器
    beacon_info->beacon_slot_nr  = (zc_timer_query(g_SendsyhtimeoutTimer) == TMR_RUNNING) ? 0 : 200;
    beacon_info->beacon_sent_nr  = 0;
    beacon_info->RfBeaconSlotCnt  = 1;
    beacon_info->RfBeaconType  = e_BEACON_HPLC;

    for(i = 0; i<beacon_info->beacon_slot_nr; i++)
    {
        beacon_info->beacon_slot_info[i].type  = 0;
        beacon_info->beacon_slot_info[i].phase = i < 3 ? (i +1) : 1;//0==i?1:1==i?2:2==i?3:1;
        //beacon_info->beacon_slot_info[i].stei  = i>3?1:i-2;
    }

    debug_printf(&dty, DEBUG_COORD, "csma start (%d)\n", PHY_TICK2MS(beacon_info->beacon_start_time+beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot)));

    beacon_info->csma_duration_a = 100;
    beacon_info->csma_duration_b = 100;
    beacon_info->csma_duration_c = 100;
    beacon_info->csma_slot  = 100;

	register_beacon_slot(beacon_info);
	zc_free_mem(beacon_info);

}
#endif
















































