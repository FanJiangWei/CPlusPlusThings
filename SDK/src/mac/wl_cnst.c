#include "wl_cnst.h"
#include "phy_sal.h"
#include "mac_common.h"
#include "plc.h"
#include "mem_zc.h"
#include "wphy_isr.h"
#include "printf_zc.h"
#include "DataLinkGlobal.h"
#include "DataLinkInterface.h"

#if defined(STD_DUAL)


uint8_t hrf_can_not_send_message()
{
    if( HPLC.testmode == WPHYTPT || 
        HPLC.testmode == WPHYCB || 
        HPLC.testmode == MACTPT || 
        HPLC.testmode == RF_PLCPHYCB || 
        HPLC.testmode == PLC_TO_RF_CB || 
        HPLC.testmode == TEST_MODE)
    {
        return TRUE;;
    }

    // if(!(HPLC.worklinkmode & 0x02))
    // {
    //     return TRUE;
    // }

    return FALSE;
}

static int32_t construct_wl_payload_frame(mbuf_t *mbuf ,uint8_t *payload,uint16_t len,wphy_pld_info_t *wpi)
{
    uint8_t dt;
    phy_frame_info_t *pf;
    wpbh_t *wpbh;
    rf_frame_ctrl_t *fc ;

    fc= mbuf->fi.head;
    dt = get_rf_frame_dt(fc);
    pf =  &mbuf->fi;
    __memcpy(&mbuf->fi.wpi,wpi,sizeof(wphy_pld_info_t));

    pf->length	= wphy_get_pbsz(wpi->blkz) * wpi->pbc;


    if(wpi->pbc  !=  1)
    {
        return -3;
    }

    if (dt == DT_BEACON)
    {
        mbuf->fi.wpi.crc32_en = 1;
        mbuf->fi.wpi.crc32_ofs = 7;                 //物理块倒序偏移7个字节指向crc32校验位
        __memcpy(pf->payload,payload,len);
    }else{
       wpbh = (wpbh_t *)pf->payload;
       wpbh->ssn = 0;
       wpbh->mfsf = 1;
       wpbh->mfef = 1;
       mbuf->fi.wpi.crc32_en = 0;
       mbuf->fi.wpi.crc32_ofs = 0;

       __memcpy((unsigned char *)wpbh + sizeof(wpbh_t), payload, len);
    }
    dhit_write_back((uint32_t)pf->payload, pf->length);
    return 0;
}

static __inline__ int32_t get_wpi_by_len_dt(wphy_pld_info_t *wpi,uint8_t *blkz,uint16_t len,uint8_t dt,uint8_t options)
{
    if(dt == DT_SOF)
    {
        *blkz = (len > PB520_BODY_SIZE)?0xff:
                (len > PB264_BODY_SIZE)?PB520_BODY_BLKZ:
                (len > PB136_BODY_SIZE)?PB264_BODY_BLKZ:
                (len > PB72_BODY_SIZE) ?PB136_BODY_BLKZ:
                (len > PB40_BODY_SIZE) ?PB72_BODY_BLKZ :
                (len > PB16_BODY_SIZE) ?PB40_BODY_BLKZ :PB16_BODY_BLKZ;
    }

    else if (dt == DT_BEACON)
    {
        *blkz = (len > (PB520_BODY_SIZE-3))?0xff:
                (len > (PB264_BODY_SIZE-3))?PB520_BODY_BLKZ:
                (len > (PB136_BODY_SIZE-3))?PB264_BODY_BLKZ:
                (len > (PB72_BODY_SIZE-3 ))?PB136_BODY_BLKZ:
                (len > (PB40_BODY_SIZE-3 ))?PB72_BODY_BLKZ :PB40_BODY_BLKZ;
    }

    if(*blkz == 0xff )
    {
        return -1;
    }

    wpi->blkz = *blkz;
    wpi->pld_mcs = HPLC.pld_mcs;//4;
    wpi->pbc     = 1;

    return 0;

}

phy_frame_info_t *packet_sack_wl(rf_sof_ctrl_t *head,uint8_t result,int8_t snr)
{
	HPLC.fi.head = HPLC.hdr;
	rf_sack_ctrl_t *sack;

    memset(HPLC.hdr, 0, sizeof(HPLC.hdr));

	sack = HPLC.fi.head;

	sack->dt	 = DT_SACK;
	
	#if defined(STD_SPG)
	sack->access = 1;
	sack->std    = 1;
	#elif defined(STD_2016)
 	sack->stei   = head->dtei;
	sack->access = 0;
	sack->load	 = WL_TX_MNG_LINK.nr+WL_TX_DATA_LINK.nr;
	sack->ext_dt = 0;		
	sack->rssi	 = snr;//need set 
	sack->std    = 0;
	#endif
	sack->snid	 = head->snid;
	sack->result = result;	
	sack->dtei	 = head->stei;

    HPLC.fi.whi.phr_mcs = HPLC.hdr_mcs;//(wphy_get_option() == WPHY_OPTION_3) ? 4 : 4;
    HPLC.fi.whi.pts = 0;        //载波和无线ACK共用全局变量组帧。防止出现错误，需要手动清空标志

//    wphy_printf("Send ack snid:%08x, stei:%04x, dtei:%04x, result:%d\n",sack->snid, sack->stei,sack->dtei, sack->result);

	return &HPLC.fi;
}

/*给抄控器用*/
mbuf_hdr_t * packet_sack_ctl_rf(uint8_t ext_dt,uint8_t *addr ,uint16_t stei,uint32_t snid)
{
	mbuf_hdr_t *mbuf_hdr;
	mbuf_t *buf;
	frame_ctrl_t *fc;
	sack_search_ctrl_t *fc_srch;
	sack_syh_ctrl_t *fc_syh;
	
	mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t),"SCKCRF",MEM_TIME_OUT);
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

	mbuf_hdr->buf->fi.whi.phr_mcs = HPLC.hdr_mcs;//(wphy_get_option() == WPHY_OPTION_3) ? 4 : 4;

	return mbuf_hdr;
}

mbuf_hdr_t *packet_beacon_wl(uint32_t stime,uint8_t *payload,uint16_t len,uint32_t bpc)
{
    uint16_t pb_len;
    uint8_t blkz;
    wphy_pld_info_t wpi;
    int32_t ret;
    uint8_t option = wphy_get_option();

    if(hrf_can_not_send_message()) 
    {
        return NULL;
    }

    if((ret=get_wpi_by_len_dt(&wpi,&blkz,len,DT_BEACON,option))<0)
    {
        printf_s("ERROR wpi:%d\n",ret);
        return NULL;
    }
    pb_len = wphy_get_pbsz(blkz);

    mbuf_hdr_t *mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(rf_beacon_ctrl_t)+pb_len,"rfpbcn",MEM_TIME_OUT);

    mbuf_t *buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t));
    rf_beacon_ctrl_t *fc = mbuf_hdr->buf->fi.head = (rf_beacon_ctrl_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t));
    mbuf_hdr->buf->fi.payload = ((uint8_t *)mbuf_hdr->buf->fi.head + sizeof(rf_beacon_ctrl_t));


    //用于发送时判断，当前信道是否合法，不合法丢弃数据。
    buf->rfoption = wphy_get_option();
    buf->rfchannel = wphy_get_channel();


    buf->snid  = HPLC.snid;
    buf->lid   = 1;
    buf->stime = stime;
    buf->bct   = bpc;

    mbuf_hdr->work = NULL;

    fc->dt     = DT_BEACON;
    fc->access = 0;
    fc->std    = 0;
    fc->snid   = HPLC.snid;
    fc->stei   = HPLC.tei;
    fc->mcs    = wpi.pld_mcs;
    fc->blkz   = blkz;

    mbuf_hdr->buf->fi.whi.phr_mcs = HPLC.hdr_mcs;//(option == WPHY_OPTION_3 ? 4 : 4);
    mbuf_hdr->buf->fi.whi.pts  = TRUE;	// if pts is true, then stime is decided by phy layer,
    mbuf_hdr->buf->fi.whi.pts_off = offset_of(rf_beacon_ctrl_t, bts);

    mbuf_hdr->buf->cnfrm = NULL;
    mbuf_hdr->buf->retrans = 0;     //台体磨合测试未连续收到10帧信标，打开重发

#if defined(STATIC_NODE)
    mbuf_hdr->buf->retrans = 0;
#endif

    construct_wl_payload_frame(mbuf_hdr->buf,payload,len,&wpi);
    return mbuf_hdr;
}

mbuf_hdr_t * packet_sof_wl(uint8_t priority,uint16_t dtei,uint8_t *payload,uint16_t len,uint8_t retrans ,uint8_t tf,plc_work_t *cnfrm)
{

    uint16_t pb_len;
	wphy_pld_info_t wpi;
	int32_t ret;
    uint8_t option = wphy_get_option();
    uint8_t blkz;

    // if(hrf_can_not_send_message()) 
    // {
    //     return NULL;
    // }

    if((ret = get_wpi_by_len_dt(&wpi, &blkz, len, DT_SOF, option)) < 0)
    {
        printf_s("wpi ret:%d,len %d\n",ret,len);
		return NULL;
    }

    pb_len = wphy_get_pbsz(blkz);

    mbuf_hdr_t *mbuf_hdr;

//    printf_s("macLen:%d, pb_len : %d blkz : %d\n",len, pb_len , wpi.blkz);
	
    mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(rf_frame_ctrl_t)+pb_len,"wpsof",MEM_TIME_OUT);
    if(mbuf_hdr == NULL)
    {
        return NULL;
    }
	mbuf_t *buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t)); 
    rf_sof_ctrl_t *fc = mbuf_hdr->buf->fi.head = (rf_sof_ctrl_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t));
    mbuf_hdr->buf->fi.payload = ((uint8_t *)mbuf_hdr->buf->fi.head + sizeof(rf_frame_ctrl_t));


    //用于发送时判断，当前信道是否合法，不合法丢弃数据。
    buf->rfoption = wphy_get_option();
    buf->rfchannel = wphy_get_channel();

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
    buf->retrans  = retrans? retrans:3;
	//buf->res
	mbuf_hdr->work = NULL;	

    fc->dt = DT_SOF;
    fc->access = 0;
    fc->snid   = HPLC.snid;
    fc->stei   = HPLC.tei;
    fc->dtei   = dtei;
    fc->lid    = buf->lid;
    fc->blkz   = wpi.blkz;//wphy_get_blkz(pb_len);
    fc->bcsti  = buf->acki==0?1:0;
    fc->repeat = 0;
    fc->enc    = 0;
    fc->mcs    = wpi.pld_mcs;
    fc->std    = 0;

#if defined(STATIC_NODE)        //重发关联请求会导致台体测试失败（台体拒绝STA入网后认为关联请求重发帧是再次关联请求导致失败）
//    if(fc->stei == 0)
//    {
//        buf->retrans = 0;
//    }

#endif
    {
        //作为代理节点转发关联回复需要重发，应用层广播报文重发台体测试不通过。
        MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)payload;
        if(pMacHeader->MacProtocolVersion == e_MAC_VER_STANDARD && !HPLC.scanFlag)
        {
            if(pMacHeader->DestTEI == 0xfff && pMacHeader->MsduType == e_APS_FREAME)
                buf->retrans = 0;
        }
    }

    mbuf_hdr->buf->fi.whi.phr_mcs = HPLC.hdr_mcs;//(option == WPHY_OPTION_3) ? 4 : 4;


    fc->fl   = wphy_get_frame_dur(option, mbuf_hdr->buf->fi.whi.phr_mcs
             , wpi.pld_mcs, wphy_get_pbsz(wpi.blkz), wpi.pbc) + WPHY_CIFS +
             fc->bcsti == 1 ? 0 : wphy_sack_frame_dur(option, mbuf_hdr->buf->fi.whi.phr_mcs) + WPHY_RIFS;
    mbuf_hdr->buf->cnfrm = cnfrm;

    construct_wl_payload_frame(mbuf_hdr->buf, payload, len, &wpi);

    return mbuf_hdr;
}

mbuf_hdr_t * packet_sof_wl_forplc(void *rfHead,uint8_t *payload,uint16_t len, uint8_t phr_mcs, uint8_t pld_mcs, uint8_t pbsize)
{
    uint16_t pb_len;
    wphy_pld_info_t wpi;
    uint8_t option = wphy_get_option();

    wpi.blkz = pbsize;
    wpi.pld_mcs = pld_mcs;
    wpi.pbc     = 1;

    pb_len = wphy_get_pbsz(pbsize);
    mbuf_hdr_t *mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(rf_frame_ctrl_t)+pb_len,"wpsof_plc",MEM_TIME_OUT);
    mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t));
    rf_sof_ctrl_t *fc = mbuf_hdr->buf->fi.head = (rf_sof_ctrl_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t));
    mbuf_hdr->buf->fi.payload = ((uint8_t *)mbuf_hdr->buf->fi.head + sizeof(rf_frame_ctrl_t));

    __memcpy(fc, rfHead, sizeof(rf_sof_ctrl_t));

    fc->blkz = wpi.blkz;
    fc->mcs  = wpi.pld_mcs;


    mbuf_hdr->buf->fi.whi.phr_mcs = phr_mcs;


    fc->fl   = wphy_get_frame_dur(option, mbuf_hdr->buf->fi.whi.phr_mcs
             , wpi.pld_mcs, wphy_get_pbsz(wpi.blkz), wpi.pbc) + WPHY_CIFS +
             fc->bcsti == 1 ? 0 : wphy_sack_frame_dur(option, mbuf_hdr->buf->fi.whi.phr_mcs) + WPHY_RIFS;

    construct_wl_payload_frame(mbuf_hdr->buf, payload, len, &wpi);

    return mbuf_hdr;
}
#if defined(STATIC_MASTER)
mbuf_hdr_t * packet_coord_wl()
{
    return NULL;
    mbuf_hdr_t *mbuf_hdr;
    mbuf_t *buf;
    rf_coord_ctrl_t *fc;

    if(hrf_can_not_send_message()) 
    {
        return NULL;
    }

    mbuf_hdr = zc_malloc_mem(sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(rf_coord_ctrl_t),"CORDW",MEM_TIME_OUT);
    buf = mbuf_hdr->buf = (mbuf_t *)((uint8_t *)mbuf_hdr + sizeof(mbuf_hdr_t));
    fc = mbuf_hdr->buf->fi.head = (rf_coord_ctrl_t *)((uint8_t *)mbuf_hdr->buf + sizeof(mbuf_t));


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

    fc->dt	      = DT_COORD;
    fc->access    = 0;
    fc->std       = 0;
    fc->snid      = HPLC.snid;
    __memcpy(fc->mac, GetCCOAddr(), 6);
    fc->rfchan_id   = DstCoordRfChannel==0xff?(/*wphy_get_channel()+1*//*WPHY_CHANNEL*/getHplcRfChannel()):DstCoordRfChannel;

    mbuf_hdr->buf->fi.whi.phr_mcs = HPLC.hdr_mcs;//(wphy_get_option() == WPHY_OPTION_3) ? 4 : 4;

    mbuf_hdr->buf->cnfrm = NULL;

    mbuf_hdr->buf->fi.pi.link = NULL;

    return mbuf_hdr;
}
#endif



#endif //#if defined(STD_DUAL)
