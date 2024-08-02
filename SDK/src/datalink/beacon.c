#include "Beacon.h"
#include "framectrl.h"
#include "DataLinkGlobal.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "timeslot_equalization.h"
#include "SyncEntrynet.h"

#include "csma.h"
#include "plc_cnst.h"
#include "Scanbandmange.h"
//#include "SyncEntrynet.h"
#include "DatalinkTask.h"
#include "DataLinkGlobal.h"

#include "datalinkdebug.h"


#include "SchTask.h"

#include "wlc.h"
#include "power_manager.h"
#include "app_rdctrl.h"
#include "app_event_report_cco.h"
#include "app_gw3762_ex.h"


U32    		NwkBeaconPeriodCount;//网络层记录的信标周期
U32         SimBeaconPeriodCount;//网络层记录的精简信标周期
U8			g_BeaconSlotTime;
U8           g_RelationSwitch;  //关联标志
U8 			 g_SendCenterBeaconCountWithoutRespond = 0;

BEACON_SLOT_SCHEDULE	BeaconSlotSchedule_t;
JOIN_CTRL	JoinCtrl_t;

ostimer_t					 g_SendCenterBeaconTimer ;

TIME_SLOT_PARAM_t g_TimeSlotParam;
U32 BeaconStopOffset;



////////////////////////////////信标组帧使用的本地函数//////////////////////////////

static void SetStaAbilityEntry(STA_ABILITY_STRUCT_t *pStaAblity)
{
    pStaAblity->BeaconEntryType = e_STA_ABILITY_TYPE;
    pStaAblity->EntryLength =  STA_ABILITY_ENTRY_LENGTH;

    pStaAblity->TEI = GetTEI();
    pStaAblity->ProxyTEI = GetProxy();
    __memcpy(pStaAblity->SrcMacAddr, nnib.MacAddr, 6);
    pStaAblity->PathSuccessRate = 100;

    pStaAblity->NodeType = GetNodeType();
    pStaAblity->StaLevel = nnib.NodeLevel;

    pStaAblity->WithProxyLQI = nnib.WithProxyLQI;
    pStaAblity->Phase = nnib.PossiblePhase;
#ifdef HPLC_HRF_PLATFORM_TEST
    pStaAblity->RfCount = nnib.RfCount;  
#endif

}


////////////////////////功率设置/////////////////////////////////////////
void SetPowerLevel(S16 dig,S16 ana)
{
    //phy_tgain_t tgain;//0x00080006;
    //phy_ioctl(PHY_CMD_GET_TGAIN, &tgain);
    if(HPLC.dig != dig || HPLC.ana != ana)
    {
        HPLC.dig=dig;
        HPLC.ana=ana;
        //phy_ioctl(PHY_CMD_SET_TGAIN, &tgain);
        //phy_ioctl(PHY_CMD_GET_TGAIN, &tgain);
        app_printf("set HPLC.ana = %d HPLC.dig = %d \n",HPLC.ana,HPLC.dig);
//        zc_phy_reset();
    }

}

/**
 * @brief setRfAbilityEntry
 * @param pSlotAllocation
 * @param pRfAbility
 * @param CsmaSlotDur
 */
static void setRfAbilityEntry(RF_ABILITY_SLOT_t *pRfAbility, U32 CsmaSlotStart, U32 CsmaSlotDur)
{
    pRfAbility->BeaconEntryType = e_SIMPBEACON_ABILITY_TYPE;
    pRfAbility->EntryLength     = sizeof(RF_ABILITY_SLOT_t);
    pRfAbility->Tei             = GetTEI();
    pRfAbility->ProxyTei        = GetProxy();
    pRfAbility->NodeType        = GetNodeType();
    pRfAbility->Level           = nnib.NodeLevel;
    __memcpy(pRfAbility->MacAddr, nnib.MacAddr, 6);
    pRfAbility->RfCount         = nnib.RfCount;

    pRfAbility->CsmaSlotStart   = CsmaSlotStart;
    pRfAbility->CsmaSlotLength  = CsmaSlotDur;
}



#if defined(STATIC_NODE)




void SavePowerlevel()
{
    U8	anaoffset =0;
    phy_tgain_t tgain;
    phy_ioctl(PHY_CMD_GET_TGAIN, &tgain);
    if(HPLC.band_idx ==0)
    {
        anaoffset = 0;
    }
    else if(HPLC.band_idx ==1)
    {
        anaoffset = 4;
    }
    else if(HPLC.band_idx ==2)    
    {
        anaoffset = 6;
    }
    else if(HPLC.band_idx ==3)
    {
        anaoffset = 8;
    }
    if(DefSetInfo.FreqInfo.tgainana !=(tgain.ana+anaoffset) || DefSetInfo.FreqInfo.tgaindig != tgain.dig)
    {
        DefSetInfo.FreqInfo.tgaindig = tgain.dig;
        DefSetInfo.FreqInfo.tgainana =  (tgain.ana+ anaoffset)>10? 10:(tgain.ana+ anaoffset);
        DefwriteFg.FREQ = TRUE;
        DefaultSettingFlag = TRUE;
        app_printf("SavePowerlevel HPLC.band_idx = %d, anaoffset = %d\n", HPLC.band_idx, anaoffset);

    }

}





///////////////////////////////远程切换频段////////////////////////////////

ostimer_t g_ChangeBandTimer ; //切换频段定时器

static void WaitChangeBandFun( ostimer_t *ostimer, void *arg)
{
       net_printf("-----------band change----------\n");
//        changeband(DefSetInfo.FreqInfo.FreqBand);

       work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(int8_t),"CGBD",MEM_TIME_OUT);
       work->work = changeBandReq;
	   work->msgtype = CHAN_BAND;
       work->buf[0] = DefSetInfo.FreqInfo.FreqBand;
       post_datalink_task_work(work);
}


void change_band_timer_init(void)
{
    if(g_ChangeBandTimer.fn == NULL)
    {
        timer_init(&g_ChangeBandTimer,
                   10,
                   10,
                   TMR_TRIGGER,//TMR_TRIGGER
                   WaitChangeBandFun,
                   NULL,
                   "WaitChangeBandFun"
                   );
    }

    return;

}

static void ModifyChangeBandtimer(U32 frist)
{
   if(frist==0)return;
   if(g_ChangeBandTimer.fn == NULL)
   {
       timer_init(&g_ChangeBandTimer,
                  frist,
                  10,
                  TMR_TRIGGER,//TMR_TRIGGER
                  WaitChangeBandFun,
                  NULL,
                  "WaitChangeBandFun"
                  );
       timer_start(&g_ChangeBandTimer);
   }
   else
   {
       timer_modify(&g_ChangeBandTimer,
               frist,
               10,
               TMR_TRIGGER ,//TMR_TRIGGER
               WaitChangeBandFun,
               NULL,
               "WaitChangeBandFun",
               TRUE);
       timer_start(&g_ChangeBandTimer);
   }
}


static void CCO_changeSTAband(U8 DstBand,U32 BandChangeRemainTime)
{
    net_printf("HPLC.band_idx = %d\n", HPLC.band_idx);
   if(HPLC.band_idx != DstBand)
   {
       if(TMR_STOPPED == timer_query(&g_ChangeBandTimer))
       {
          net_printf("-----------g_ChangeBandTimer after %d ms----\n",BandChangeRemainTime);
          DefSetInfo.FreqInfo.FreqBand = DstBand;
          DefwriteFg.FREQ = TRUE;
          DefaultSettingFlag = TRUE;
          ModifyChangeBandtimer(BandChangeRemainTime);

       }
   }
}

////////////////////////////////////远程切换无线信道参数////////////////////////////////////
ostimer_t g_ChangeChlTimer ; //切换频段定时器

static void WaitChangeChlFun( ostimer_t *ostimer, void *arg)
{
       net_printf("-----------rf chl change----------\n");

       work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(RF_OPTIN_CH_t),"CGCHL",MEM_TIME_OUT);
       RF_OPTIN_CH_t *chlInfo = (RF_OPTIN_CH_t *)work->buf;
       work->work = ChangRfChlReq;
       work->msgtype = CHAN_BAND;
       chlInfo->option  = DefSetInfo.RfChannelInfo.option ;
       chlInfo->channel = DefSetInfo.RfChannelInfo.channel;
       post_datalink_task_work(work);
}


void change_chl_timer_init(void)
{
    if(g_ChangeChlTimer.fn == NULL)
    {
        timer_init(&g_ChangeChlTimer,
                   10,
                   10,
                   TMR_TRIGGER,//TMR_TRIGGER
                   WaitChangeChlFun,
                   NULL,
                   "WaitChangeChlFun"
                   );
    }

    return;

}

static void ModifyChangeChltimer(U32 frist)
{
   if(frist==0)return;
   if(g_ChangeChlTimer.fn == NULL)
   {
       timer_init(&g_ChangeChlTimer,
                  frist,
                  10,
                  TMR_TRIGGER,//TMR_TRIGGER
                  WaitChangeChlFun,
                  NULL,
                  "WaitChangeChlFun"
                  );
       timer_start(&g_ChangeChlTimer);
   }
   else
   {
       timer_modify(&g_ChangeChlTimer,
               frist,
               10,
               TMR_TRIGGER ,//TMR_TRIGGER
               WaitChangeChlFun,
               NULL,
               "WaitChangeChlFun",
               TRUE);
       timer_start(&g_ChangeChlTimer);
   }
}


static void CCO_changeSTARfChannel(U8 Option, U8 channel, U32 RemainTime)
{
   if(Option != getHplcOptin() || channel != getHplcRfChannel())
   {
        net_printf("g_ChangeChlTimer after %d ms\n",RemainTime);
        DefSetInfo.RfChannelInfo.option = Option;
        DefSetInfo.RfChannelInfo.channel = channel;

        //接收到剩余变更时间为0的信道切换，如果定时器未启动，默认设置剩余变更时间为100ms，并启动定时器
        if(RemainTime == 0 && timer_query(&g_ChangeChlTimer) != TMR_RUNNING)
        {
            RemainTime = 100;
        }

        ModifyChangeChltimer(RemainTime);
   }
   else if(timer_query(&g_ChangeChlTimer) == TMR_RUNNING)
   {
        timer_stop(&g_ChangeChlTimer,TMR_NULL);
   }
}

////////////////////////////////////远程切换链路工作模式参数////////////////////////////////////
ostimer_t g_change_linkmode_timer ; //切换工作模式定时器
static void modify_change_linkmode_timer(U32 frist);

static void wait_change_linkmode_fun( ostimer_t *ostimer, void *arg)
{
    
    
    if(nnib.DurationTime > 0)
    {
        net_printf("lm chg %d,back %ds\n",nnib.LinkMode,nnib.DurationTime);
        modify_change_linkmode_timer(nnib.DurationTime);
        HPLC.worklinkmode = nnib.LinkMode;
        nnib.DurationTime = 0;
        if((nnib.LinkType == e_RF_Link&&HPLC.worklinkmode == e_link_mode_plc) ||
            (nnib.LinkType == e_HPLC_Link&&HPLC.worklinkmode == e_link_mode_rf))
        {
                g_ReqReason = e_change_linkmode;
                net_printf("e_change_linkmode\n");
                ClearNNIB();
        }
    }
    else
    {
        net_printf("lm chg %d\n",e_link_mode_dual);
        HPLC.worklinkmode = e_link_mode_dual;
        nnib.LinkMode = e_link_mode_dual;
    }
}


void change_linkmode_timer_init(void)
{
    if(g_change_linkmode_timer.fn == NULL)
    {
        timer_init(&g_change_linkmode_timer,
                   10,
                   10,
                   TMR_TRIGGER,//TMR_TRIGGER
                   wait_change_linkmode_fun,
                   NULL,
                   "wait_change_linkmode_fun"
                   );
    }
    return;
}

static void modify_change_linkmode_timer(U32 frist)
{
   if(frist==0)return;
   if(g_change_linkmode_timer.fn == NULL)
   {
       timer_init(&g_change_linkmode_timer,
                  frist*TIMER_UNITS,
                  10,
                  TMR_TRIGGER,//TMR_TRIGGER
                  wait_change_linkmode_fun,
                  NULL,
                  "wait_change_linkmode_fun"
                  );
       timer_start(&g_change_linkmode_timer);
   }
   else
   {
       timer_modify(&g_change_linkmode_timer,
               frist*TIMER_UNITS,
               10,
               TMR_TRIGGER ,//TMR_TRIGGER
               wait_change_linkmode_fun,
               NULL,
               "wait_change_linkmode_fun",
               TRUE);
       timer_start(&g_change_linkmode_timer);
   }
}


static void CCO_change_STA_linkmode(U8 LinkMode, U16 ChangeTime, U16 DurationTime)
{
   if(LinkMode > 0 && LinkMode <= e_link_mode_dual && ChangeTime > 0 && DurationTime > 0)
   {
       if(TMR_STOPPED == timer_query(&g_change_linkmode_timer) || (timer_remain(&g_change_linkmode_timer) > ChangeTime*1000))
       {
          net_printf("chg lm after %ds,con %ds\n",ChangeTime,DurationTime);
          modify_change_linkmode_timer(ChangeTime);
       }
   }
}


static U16 GetSlot(BEACON_HEADER_t		*pBeaconheader,U16 beaconlen,SLOT_ALLOCATION_STRUCT_t *pSlotAllocation, U8 *RF_BeaconFlag, U8 *RfBcnCnt)
{
    U8  beaconType;
    U16 i,TEI,slotSeq=0;
    U32 Delaytime;
    U8  findFlag = 0;
    NON_CENTRAL_BEACON_INFO_t 			*pNonCB;
    //对于入网节点在信标周期内如果只听到了发现信标，不处理，发现信标中没有非中央信标信息字段
    if(pBeaconheader->beaconType == e_PCO_BEACON  || pBeaconheader->beaconType == e_CCO_BEACON)
    {
        pNonCB  = (NON_CENTRAL_BEACON_INFO_t *)((U8 *)pSlotAllocation + sizeof(SLOT_ALLOCATION_STRUCT_t));

        for(i = 0; i < pSlotAllocation->NonCentralBeaconSlotCount ; i++)
        {
            TEI = pNonCB->SrcTei;
            if(TEI == GetTEI() && findFlag == 0)
            {
                slotSeq = i + pSlotAllocation->CentralSlotCount; //说明被安排发送信标
                beaconType = pNonCB->BeaconType;

                // 兼容思凌科安排错误的无线信标标志
                if (pNonCB->RfBeaconType > e_BEACON_HPLC_RF_SIMPLE_CSMA)
                {
                    net_printf("RfBeaconType <%d> err, reset\n", pNonCB->RfBeaconType);
                    pNonCB->RfBeaconType = e_BEACON_HPLC_RF_SIMPLE_CSMA;
                }
                
                if(RF_BeaconFlag)
                {
                    *RF_BeaconFlag = pNonCB->RfBeaconType;
                }

                Delaytime = (slotSeq * pSlotAllocation->BeaconSlotTime)*STAMPUNIT+pSlotAllocation->BeaconPeriodStartTime;
//                if(  ((int)(Delaytime +5*STAMPUNIT - get_phy_tick_time()) < 0)  )
                if((Delaytime +5*STAMPUNIT - get_phy_tick_time()) < 0)
                {
                    net_printf("slot over time=%d  ms\n",PHY_TICK2MS(get_phy_tick_time()) - (Delaytime +5*STAMPUNIT));
                    return 0;
                }
                //从节点根据中央信标中的信息，更新自己的类型
                if(beaconType == e_DISCOVERY_BEACON)
                {
                    SetNodeType(e_STA_NODETYPE);
                }
                else if(beaconType == e_PCO_BEACON)
                {
                    SetNodeType(e_PCO_NODETYPE);
                }

                if(pNonCB->RfBeaconType == e_BEACON_HPLC_RF_SIMPLE_CSMA)
                {
                    //                net_printf("BcnType:%d, RfBcnType:%d\n", pNonCB->BeaconType, pNonCB->RfBeaconType);
                    break;
                }
                else //if(pNonCB->RfBeaconType == e_BEACON_RF )
                {
                    *RfBcnCnt += 1;
                }

                findFlag = 1;
            }
            else if(findFlag)
            {
                if(pNonCB->RfBeaconType == e_BEACON_HPLC || pNonCB->RfBeaconType == e_BEACON_HPLC_RF_SIMPLE_CSMA || GetTEI() == pNonCB->SrcTei)
                {
                    if(RfBcnCnt)
                    {
                        *RfBcnCnt += 1;
                    }

                    if (GetTEI() == pNonCB->SrcTei)
                    {
                        //给多分配了载波时隙，用来发送无线标准信标
                        if(RF_BeaconFlag && (*RF_BeaconFlag == e_BEACON_HPLC))
                        {
                            *RF_BeaconFlag = e_BEACON_HPLC_RF;
                        }   
                    }
                }
                else
                {
                    if(RfBcnCnt)
                    {
                        *RfBcnCnt += 1;
                    }
                    break;
                }
            }
            pNonCB++;
        }
    }
    return slotSeq;
}

//指针关系

//						beaconlen								  //
//pbeacon--------pNonCenterBeacon-----------pMoveCsma------------end
//											  pcams

/*无线信标标志
 * 0x00：仅发送高速载波信标
 * 0x01：仅发送无线标准信标
 * 0x02：发送高速载波信标，并在该时隙结束后发送无线标准信标
 * 0x03：发送高速载波信标，并在该时隙结束后发送无线精简信标
 * 0x04：发送高速载波信标，并在CSMA 时隙发送无线精简信标
 * */
static void RelayBeacon(BEACON_HEADER_t* pBeaconheader, U16 beaconlen
                        , SLOT_ALLOCATION_STRUCT_t *pSlotAllocation
                        , STA_ABILITY_STRUCT_t *pStaAblity, U8 RFBeaconFlag, U32 CsmaSlotDur)
{

    U8	*pcams;
    U16 movelen;
    U8 *pNonCenterBeacon;

    U8 *pRfSimpBeaconHeader = NULL;
    U16 RfBeaconLen = 0;

    pNonCenterBeacon = (U8 *)((U8 *)pSlotAllocation + sizeof(SLOT_ALLOCATION_STRUCT_t));

    pBeaconheader->beaconType = e_PCO_BEACON;


    //从节点根据中央信标中的信息，更新自己的类型
    if (GetNodeType() == e_STA_NODETYPE)
    {
        pBeaconheader->beaconType = e_DISCOVERY_BEACON;
    }

    if(1)  //转发标准信标
    {
        if(pBeaconheader->beaconType == e_DISCOVERY_BEACON)
        {
            pcams = pNonCenterBeacon + pSlotAllocation->NonCentralBeaconSlotCount * 2;
            movelen = beaconlen - (pcams -(U8*)pBeaconheader);
            __memmove(pNonCenterBeacon, pcams, movelen);
            beaconlen -= pSlotAllocation->NonCentralBeaconSlotCount*2;
            pSlotAllocation->EntryLength -= pSlotAllocation->NonCentralBeaconSlotCount * 2;
        }
        SetStaAbilityEntry(pStaAblity);
    }

    if(RFBeaconFlag == e_BEACON_HPLC_RF_SIMPLE || RFBeaconFlag == e_BEACON_HPLC_RF_SIMPLE_CSMA)     //需要发送精简信标
    {
        RfBeaconLen = sizeof(RF_BEACON_HEADER_t) + sizeof(RF_ABILITY_SLOT_t);
        pRfSimpBeaconHeader = zc_malloc_mem(RfBeaconLen, "RFBCN", MEM_TIME_OUT);
        RF_BEACON_HEADER_t *pHeader = (RF_BEACON_HEADER_t *)pRfSimpBeaconHeader;
        RF_ABILITY_SLOT_t *pRfAbility = (RF_ABILITY_SLOT_t *)(pRfSimpBeaconHeader + sizeof(RF_BEACON_HEADER_t));

        __memcpy(pHeader, pBeaconheader, sizeof(RF_BEACON_HEADER_t));
        pHeader->BeaconEntryCount = 1;
        pHeader->SimpBeaconFlag = e_SIMP_BEACON;

        U32 CsmaSlotStart = pSlotAllocation->BeaconPeriodStartTime \
                            +(pSlotAllocation->CentralSlotCount+pSlotAllocation->NonCentralBeaconSlotCount)*PHY_MS2TICK(pSlotAllocation->BeaconSlotTime);

        setRfAbilityEntry(pRfAbility, CsmaSlotStart, CsmaSlotDur);
    }


    //TODO 入队 加入到发送列表
    if(RFBeaconFlag == e_BEACON_HPLC || RFBeaconFlag == e_BEACON_HPLC_RF || RFBeaconFlag == e_BEACON_HPLC_RF_SIMPLE || RFBeaconFlag == e_BEACON_HPLC_RF_SIMPLE_CSMA)
    {
        entry_tx_beacon_list_data(0, beaconlen, (U8 *)pBeaconheader, pBeaconheader->BeaconPeriodCount, &BEACON_FRAME_LIST);
        if(RFBeaconFlag == e_BEACON_HPLC_RF)
        {
            entry_rf_beacon_list_data(0, beaconlen, (U8 *)pBeaconheader, pBeaconheader->BeaconPeriodCount, &WL_BEACON_FRAME_LIST);
        }
        if(RFBeaconFlag == e_BEACON_HPLC_RF_SIMPLE)                                   //在无线信标时隙发送精简信标帧
        {
            //TODO
            entry_rf_beacon_list_data(0, RfBeaconLen, pRfSimpBeaconHeader, pBeaconheader->BeaconPeriodCount, &WL_BEACON_FRAME_LIST);
        }
        if(RFBeaconFlag == e_BEACON_HPLC_RF_SIMPLE_CSMA)                             //在CSMA时隙竞争发送精简信标帧
        {
            entry_rf_beacon_list_data_csma(0, RfBeaconLen, pRfSimpBeaconHeader, pBeaconheader->BeaconPeriodCount, &WL_TX_MNG_LINK);
        }

    }
    else if(RFBeaconFlag == e_BEACON_RF)            //发送无线标准信标
    {
        //TODO 添加到无线信标列表
        entry_rf_beacon_list_data(0, beaconlen, (U8 *)pBeaconheader, pBeaconheader->BeaconPeriodCount, &WL_BEACON_FRAME_LIST);
    }


    if(pRfSimpBeaconHeader)
        zc_free_mem(pRfSimpBeaconHeader);


    return;

}


U32  CSMA_LEN_A,  CSMA_LEN_B,  CSMA_LEN_C;

static void GetBeaconInfo(SLOT_ALLOCATION_STRUCT_t *pSlotAllocation, U8 beaconType,U32 *CSMADuration,U32 *BindDuration)
{
    U8 i;
    CSMA_SLOT_INFO_t *pCsmaSlotInfo,*pBindCsmaSlotInfo;

    //初始化
    *CSMADuration = *BindDuration = 0;
    CSMA_LEN_A = CSMA_LEN_B = CSMA_LEN_C = 0;

    NON_CENTRAL_BEACON_INFO_t *pNonCenterBeacon = (NON_CENTRAL_BEACON_INFO_t *)((U8 *)pSlotAllocation + sizeof(SLOT_ALLOCATION_STRUCT_t));

    if(beaconType == e_DISCOVERY_BEACON )
    {
        pCsmaSlotInfo = (void *)pNonCenterBeacon;
    }
    else
    {
        if(pSlotAllocation->NonCentralBeaconSlotCount > 0)
        {
            pCsmaSlotInfo =  (CSMA_SLOT_INFO_t *)( (U8 *)pNonCenterBeacon +
                                                   (pSlotAllocation->NonCentralBeaconSlotCount * sizeof(NON_CENTRAL_BEACON_INFO_t) )
                                                   );
        }
        else
        {
            pCsmaSlotInfo = (void *)pNonCenterBeacon;
        }
    }


    if(pSlotAllocation->CsmaSlotPhaseCount > 0)
    {
        for(i = 0; i < pSlotAllocation->CsmaSlotPhaseCount ; i++)
        {
            *CSMADuration += pCsmaSlotInfo->CsmaSlotLength;
            switch(pCsmaSlotInfo->Phase)
            {
            case e_A_PHASE:
                CSMA_LEN_A = pCsmaSlotInfo->CsmaSlotLength;
                break;
            case e_B_PHASE:
                CSMA_LEN_B = pCsmaSlotInfo->CsmaSlotLength;
                break;
            case e_C_PHASE:
                CSMA_LEN_C = pCsmaSlotInfo->CsmaSlotLength;
                break;
            default:
                break;
            }

            pCsmaSlotInfo = (CSMA_SLOT_INFO_t *) ((U8 *)pCsmaSlotInfo + sizeof(CSMA_SLOT_INFO_t));
        }

    }

    if(*CSMADuration >= 15000 )
    {
        net_printf("error----------CSMASliceDuration :%d-----------------------\n",*CSMADuration);
    }

    if(pSlotAllocation->BindCsmaSlotPhaseCout > 0)
    {
        pBindCsmaSlotInfo = (CSMA_SLOT_INFO_t *)((U8 *)pCsmaSlotInfo + sizeof(CSMA_SLOT_INFO_t));
        for(i = 0; i < pSlotAllocation->BindCsmaSlotPhaseCout ; i++)
        {
            BindDuration += pBindCsmaSlotInfo->CsmaSlotLength;

            pBindCsmaSlotInfo = (CSMA_SLOT_INFO_t *) ((U8 *)pBindCsmaSlotInfo + sizeof(CSMA_SLOT_INFO_t));
        }
    }

}

typedef struct
{
    U32 BeaconPeriodStartTime;

    U16 slotSeq;
    U16 BeaconSlotTime;

    U16 BeaconSlotCount;
    U16 CsmaSlotSlice;

    U8  BindCsmaSlotLinkId;
    U8  RfBeaconType;
    U8  RfBeaconCnt;
    U8  res;
}BCN_SLOT_INFO_t;

void dataLikTimeSlotEntry(BCN_SLOT_INFO_t slotInfo)
{
    //时隙分配
    uint16_t i;
    BEACON_PERIOD_t *beacon_info;

    if( !HPLC.sfo_not_first )
    {
        net_printf("Time is Not Sync !!!\n");
        return;
    }
    beacon_info = zc_malloc_mem(sizeof(BEACON_PERIOD_t),"PROD",MEM_TIME_OUT);

    beacon_info->beacon_start_time = slotInfo.BeaconPeriodStartTime;

    beacon_info->beacon_slot = slotInfo.BeaconSlotTime;
    beacon_info->beacon_slot_nr = slotInfo.BeaconSlotCount;// 中央信标 + 代理信标？

    for(i = 0;i<beacon_info->beacon_slot_nr;i++)
    {
        beacon_info->beacon_slot_info[i].type  = TS_BEACON;
        if(slotInfo.slotSeq == i)
        {
            beacon_info->beacon_slot_info[i].phase = (0 == nnib.PossiblePhase) ? 1 : nnib.PossiblePhase;
        }
        else
        {
            beacon_info->beacon_slot_info[i].phase = 0==i?1:1==i?2:2==i?3:1;
        }
        //beacon_info->beacon_slot_info[i].stei  = i>3?1:i-2;
    }
    beacon_info->beacon_curr = NULL;
    beacon_info->beacon_sent_nr = slotInfo.slotSeq;            //slotSeq？
    beacon_info->ext_bpts = 0;

    net_printf("b_slot_nr=%d,b_sent_nr=%d\n"
               , beacon_info->beacon_slot_nr
               , beacon_info->beacon_sent_nr);


    net_printf("CSMA_A=%d,B=%d,C=%d,slot=%d\n"
               , CSMA_LEN_A
               , CSMA_LEN_B
               , CSMA_LEN_C
               , slotInfo.CsmaSlotSlice);

    beacon_info->csma_duration_a = CSMA_LEN_A;
    beacon_info->csma_duration_b = CSMA_LEN_B;
    beacon_info->csma_duration_c = CSMA_LEN_C;
    beacon_info->csma_slot  = slotInfo.CsmaSlotSlice;
    beacon_info->bcsma_duration_a = 0;
    beacon_info->bcsma_duration_b = 0;
    beacon_info->bcsma_duration_c = 0;
    beacon_info->bcsma_slot = 0;
    beacon_info->bcsma_lid    = slotInfo.BindCsmaSlotLinkId;
    beacon_info->beacon_curr = NULL;
    beacon_info->RfBeaconType = slotInfo.RfBeaconType;
    beacon_info->RfBeaconSlotCnt = slotInfo.RfBeaconCnt;

//    net_printf("beacon_start_time : %08x, System Time Tick : %08x \n"
//               , (beacon_info->beacon_start_time)
//               , (get_phy_tick_time()));
//    if(CURR_BEACON)
//    {
//        net_printf("CURR_BEACON-> end = %08d\n", PHY_TICK2MS(CURR_BEACON->end));
//    }

    register_beacon_slot(beacon_info);
//	net_printf("after slot set : %08x,\n",get_phy_tick_time());

    zc_free_mem(beacon_info);

}

#endif

//信标条目指针集合
typedef struct{
    STA_ABILITY_STRUCT_t 		*pStaAbility;       //站点能力
    ROUTE_PARM_STRUCT_t 		*pRouteParm;        //路有参数
    BAND_CHANGE_STRUCT_t		*pBandChange;       //切换band
    RF_ROUTE_PARM_STRUCT_t      *pRfRouteParm;      //无线路由参数条目
    RF_CHANNEL_CHANGE_STRUCT_t  *PRfChannelChange;  //无线信道变更条目
    RF_ABILITY_SLOT_t           *pRfAbilitySlot;    //精简信标帧站点信息及时隙条目
    SLOT_ALLOCATION_STRUCT_t 	*pSlotAllocation;   //时隙分配条目
    POWERSET_STRUCT_t			*pPowerSet;         //功率设置条目
    LINKMODESET_STRUCT_t        *pLinkModeSet;      //链路切换条目
}__PACKED BEACON_ENTRY_GATHER_t;


static void GetEntryHeader(U8                           BeaconEntryCount,
                           U8                           *BeaconPayload,
                           BEACON_ENTRY_GATHER_t        *pBeaconEntry,
                           U16  len,
                           U16	*beaconlen)
{
    U8				entryNumber,i;
    U16				length = 0;
    U16				temp_len = 0;
    U8				*pBeaconload   = BeaconPayload;
    entryNumber = BeaconEntryCount;

    memset(pBeaconEntry, 0, sizeof(BEACON_ENTRY_GATHER_t));

//    net_printf("entryNumber = %d \n", entryNumber);

    for(i = 0; i < entryNumber; i++)
    {
//        net_printf("entry type: %02x\n", pBeaconload[length]);
        switch(pBeaconload[length])
        {
        case e_STA_ABILITY_TYPE: //站点能力条目
        {
            pBeaconEntry->pStaAbility = (STA_ABILITY_STRUCT_t *)&pBeaconload[length];
            length += sizeof(STA_ABILITY_STRUCT_t);

            break;
        }
        case e_SLOT_ALLOCATION_TYPE://时隙分配条目
        {
            pBeaconEntry->pSlotAllocation = (SLOT_ALLOCATION_STRUCT_t *)&pBeaconload[length];
            length += pBeaconEntry->pSlotAllocation->EntryLength;

            break;
        }
        case e_ROUTE_PARM_TYPE://路由参数条目
        {
            pBeaconEntry->pRouteParm = (ROUTE_PARM_STRUCT_t *)&pBeaconload[length];
            length += sizeof(ROUTE_PARM_STRUCT_t);
            //routerParmFlag = TRUE;
            break;
        }
        case e_RF_ROUTE_PARM_TYPE://无线路由参数条目
        {
            pBeaconEntry->pRfRouteParm = (RF_ROUTE_PARM_STRUCT_t *)&pBeaconload[length];
            length += sizeof(RF_ROUTE_PARM_STRUCT_t);
            //routerParmFlag = TRUE;
            break;
        }
        case e_SIMPBEACON_ABILITY_TYPE://无线路由参数条目
        {
            pBeaconEntry->pRfAbilitySlot = (RF_ABILITY_SLOT_t *)&pBeaconload[length];
            length += sizeof(RF_ABILITY_SLOT_t);
            //routerParmFlag = TRUE;
            break;
        }
        case e_RF_CHANNEL_CHANGE_TYPE://无线信道变更条目
        {
#if defined(STATIC_NODE)
            pBeaconEntry->PRfChannelChange = (RF_CHANNEL_CHANGE_STRUCT_t *)&pBeaconload[length];
#endif
            length += sizeof(RF_CHANNEL_CHANGE_STRUCT_t);
            //routerParmFlag = TRUE;
            break;
        }
        case e_BAND_CHANGE_TYPE://频率切换条目
        {
#if defined(STATIC_NODE)
            pBeaconEntry->pBandChange = (BAND_CHANGE_STRUCT_t*)&pBeaconload[length];
#endif
            length += sizeof(BAND_CHANGE_STRUCT_t);

            break;

        }
        case e_POWERLEVELENITY://设置功率
        {
#if defined(STATIC_NODE)
            pBeaconEntry->pPowerSet = (POWERSET_STRUCT_t *)&pBeaconload[length];
#endif
            length += sizeof(POWERSET_STRUCT_t);;

            break;
        }
        case e_LINKMODE_SET_ENITY://设置连接模式
        {
#if defined(STATIC_NODE)
            pBeaconEntry->pLinkModeSet = (LINKMODESET_STRUCT_t *)&pBeaconload[length];
#endif
            length += sizeof(LINKMODESET_STRUCT_t);;

            break;
        }
        default :
            net_printf("Bn=%02x ",pBeaconload[length]);
            

            if(pBeaconload[length] >= 0xc1 && pBeaconload[length] <= 0xff)
            {
                temp_len = (length + (pBeaconload[length+1]|(pBeaconload[length+2]<<8)));
            }
            else
            {
                temp_len = (length + pBeaconload[length+1]);
            }

            
            if(temp_len > len)
            {
                length = temp_len;
                net_printf("entryNumber %d,i %d,temp_len %d err,len %d\n",entryNumber,i,temp_len,len);
            }
            else
            {
                length = temp_len;
                net_printf("temp_len %d\n",temp_len);
            }
            break;

        }
    }
    *beaconlen = length;
}

//精简信标帧，单独处理
static void ProcSimpBeaconIndication(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc32)
{
    BEACON_ENTRY_GATHER_t   BeaconEntry;
    RF_BEACON_HEADER_t  *pBeaconHeader;

    U8 *pBeaconPload;
    U16 beaconlen;

    pBeaconHeader = (RF_BEACON_HEADER_t *)pld;

    pBeaconPload = pld + sizeof(RF_BEACON_HEADER_t);

    //获取条目信息
    GetEntryHeader(pBeaconHeader->BeaconEntryCount, pBeaconPload, &BeaconEntry, len, &beaconlen);
    if(BeaconEntry.pRfAbilitySlot == NULL)          //精简信标帧必选信标条目
    {
        net_printf("pRfAbilitySlot is NULL\n");
        return;
    }
#if defined(STATIC_NODE)
	
	if(!checkAccessControlList(BeaconEntry.pRfAbilitySlot->MacAddr, 1) )
	{
	   if(AccessListSwitch ==TRUE)
	   {
		   net_printf("beacon checkAccessControlList!:%02x %02x %02x %02x %02x %02x\n",BeaconEntry.pRfAbilitySlot->MacAddr[0],
			   BeaconEntry.pRfAbilitySlot->MacAddr[1],
			   BeaconEntry.pRfAbilitySlot->MacAddr[2],
			   BeaconEntry.pRfAbilitySlot->MacAddr[3],
			   BeaconEntry.pRfAbilitySlot->MacAddr[4],
			   BeaconEntry.pRfAbilitySlot->MacAddr[5]);
		   return;
	   }
	}
#endif

    UpdateRfNbInfo(buf->snid, BeaconEntry.pRfAbilitySlot->MacAddr, BeaconEntry.pRfAbilitySlot->Tei, BeaconEntry.pRfAbilitySlot->Level,BeaconEntry.pRfAbilitySlot->NodeType, BeaconEntry.pRfAbilitySlot->ProxyTei,
                   0, buf->rssi, buf->snr, pBeaconHeader->BeaconEntryCount,  BeaconEntry.pRfAbilitySlot->RfCount, DT_BEACON);

#if defined(STATIC_MASTER)
    if(buf->snid != GetSNID())
    {
        //记录邻居网络CCOMAC地址
        UpdateNbNetInfo(buf->snid,pBeaconHeader->CCOmacaddr, buf->rfoption, buf->rfchannel);
        //主节点做网络维护时，接收到其他网络的信标需要过滤掉
        return;
    }

    SetDeviceList(buf->snid,BeaconEntry.pRfAbilitySlot->MacAddr,BeaconEntry.pRfAbilitySlot->Tei);

#elif defined(STATIC_NODE)

    if(FALSE==STA_JudgeOfflineBybeacon(pBeaconHeader->Networkseq,pBeaconHeader->CCOmacaddr,
                                       BeaconEntry.pRfAbilitySlot->Tei,BeaconEntry.pRfAbilitySlot->Level,BeaconEntry.pRfAbilitySlot->NodeType, buf))
    {
//        net_printf("FALSE==STA_JudgeOfflineBybeacon \n");
        return;
    }

    if(SimBeaconPeriodCount == pBeaconHeader->BeaconPeriodCount)
    {
        return;
    }
    SimBeaconPeriodCount = pBeaconHeader->BeaconPeriodCount;

    net_printf("cco=%02x%02x%02x%02x%02x%02x,TEI=0x%04x-PCT=0x%08x-State=%d-seq=%02x,CSMA=%08x %d PTei=%03x\n"
            ,pBeaconHeader->CCOmacaddr[0],pBeaconHeader->CCOmacaddr[1],pBeaconHeader->CCOmacaddr[2]
            ,pBeaconHeader->CCOmacaddr[3],pBeaconHeader->CCOmacaddr[4],pBeaconHeader->CCOmacaddr[5]
            ,BeaconEntry.pRfAbilitySlot->Tei
            ,SimBeaconPeriodCount
            ,nnib.NodeState
            ,pBeaconHeader->Networkseq
            ,BeaconEntry.pRfAbilitySlot->CsmaSlotStart
            ,BeaconEntry.pRfAbilitySlot->CsmaSlotLength
            ,GetProxy());


     //TODO  调用时隙分配接口, 未入网节点或者是入网节点两个信标周期未收到标准信标，可以使用精简信标规划时隙。
    if(GetNodeState() != e_NODE_ON_LINE || time_diff(SimBeaconPeriodCount, NwkBeaconPeriodCount) >= 2)
    {
        if(BeaconEntry.pRfAbilitySlot->CsmaSlotLength < 10000)
        {
            BCN_SLOT_INFO_t slotINfo;
            memset(&slotINfo,0x00,sizeof(BCN_SLOT_INFO_t));
            if(time_after(BeaconEntry.pRfAbilitySlot->CsmaSlotStart, get_phy_tick_time()))
            {
                slotINfo.BeaconPeriodStartTime = get_phy_tick_time();//BeaconEntry.pRfAbilitySlot->CsmaSlotStart;
                slotINfo.BeaconSlotTime = PHY_TICK2MS(BeaconEntry.pRfAbilitySlot->CsmaSlotStart - slotINfo.BeaconPeriodStartTime);
                slotINfo.BeaconSlotCount = 1;
            }
            else
            {
                slotINfo.BeaconPeriodStartTime = BeaconEntry.pRfAbilitySlot->CsmaSlotStart;
                slotINfo.BeaconSlotTime = 0;
                slotINfo.BeaconSlotCount = 0;
            }
            slotINfo.slotSeq = 0;
            slotINfo.CsmaSlotSlice = BeaconEntry.pRfAbilitySlot->CsmaSlotLength;
            slotINfo.BindCsmaSlotLinkId = 0;
            slotINfo.RfBeaconType = 0;

            CSMA_LEN_A = BeaconEntry.pRfAbilitySlot->CsmaSlotLength;
            CSMA_LEN_B = 0;
            CSMA_LEN_C = 0;


            dataLikTimeSlotEntry(slotINfo);
        }   
    }
    


    //TODO 规划时隙。只有csma时隙

    //更新邻居网络无线路由参数
//    updateNeiNetRfParam(buf->snid, wphy_get_option(), WPHY_CHANNEL);

    if(GetNodeState() != e_NODE_ON_LINE)        //离线节点处理
    {
        if(pBeaconHeader->Relationshipflag == FALSE)
        {
            net_printf("Relationshipflag is False\n");
            return;
        }
        if(BeaconEntry.pRfAbilitySlot->Level >= 15)
        {
            net_printf("Lv %d Error\n");
            return;
        }
        if(!CheckBlackNegibor(buf->snid))
        {
            net_printf("Net %08x In Black\n", buf->snid);
            return;
        }

        //启动组网
//        if(HPLC.sfo_not_first == 0)
//        {
//            net_printf("waring:sfo not ready!\n");
//            return;
//        }

        if(pBeaconHeader->Relationshipflag == FALSE)
        {
            net_printf("Relationshipflag is False\n");
            return;
        }
        if( BeaconEntry.pRfAbilitySlot->Level == 0x0f)//听到节点为信标为15跳不响应
        {
            net_printf("STA Level > 15 \n");
//            return;
        }
//        if(BeaconEntry.pRfAbilitySlot->CsmaSlotLength > 250 && BeaconEntry.pRfAbilitySlot->CsmaSlotLength < 10000)
//        {
//            U32  /*nowtime,*/delaytime;
////            nowtime = PHY_TICK2MS(get_phy_tick_time());
//            delaytime = rand()%(BeaconEntry.pRfAbilitySlot->CsmaSlotLength - 250);

////            if(time_after(BeaconEntry.pRfAbilitySlot->CsmaSlotStart, nowtime) && (nowtime < BeaconEntry.pRfAbilitySlot->CsmaSlotStart))
////            {
////               delaytime +=( BeaconEntry.pRfAbilitySlot->CsmaSlotStart-nowtime);
////            }

//            StartSTAnetwork(delaytime, pBeaconHeader->Networkseq);
//        }
//        else
        {
            StartSTAnetwork(0, pBeaconHeader->Networkseq);
        }

    }
    else                                        //入网节点处理
    {

    }

#endif





}

void ProcBeaconIndication(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc32)
{
   U16                           beaconlen;
   BEACON_HEADER_t				*pBeaconheader;

   BEACON_ENTRY_GATHER_t   BeaconEntry;
#if defined(STATIC_NODE)
   U16							slotSeq;
   U32 							CSMADuration,BindDuration;
   U8                           RfBeaconFlag = 0; //无线信标标志
   U8                           RfBcnCnt = 0; //无线信标标志
#endif
   U8							 ErrMac[6]={0,0,0,0,0,0};


    pBeaconheader = (BEACON_HEADER_t * )pld;                                        //信标载荷

    //TODO process fch //需要对帧进行过滤

    if(buf->LinkType == e_HPLC_Link)
    {
        beacon_ctrl_t *pframe_ctrl = (beacon_ctrl_t *)buf->fi.head;  //帧控制
        UpdateNbInfo(buf->snid, NULL, pframe_ctrl->stei,0, 0, 0, 0, buf->rgain,0,0,DT_BEACON); //更新邻居站点增益
    }
    else
    {
        rf_beacon_ctrl_t *pFrameCtrl = (rf_beacon_ctrl_t *)buf->fi.head;  //帧控制
        UpdateRfNbInfo(buf->snid, NULL, pFrameCtrl->stei, 0, 0, 0, 0, buf->rssi, buf->snr, 0, 0, DT_BEACON);
    }
#if defined(STATIC_NODE)

    AddNeighborNet(buf, DT_BEACON);//更新邻居网络

    if(buf->LinkType == e_HPLC_Link)
    {
        ScanBandManage(e_RECEIVE, buf->snid);
    }
    else                //无线扫频
    {
        ScanRfChMange(e_RECEIVE, buf->snid);
    }

    
   if(nnib.MacAddrType == e_UNKONWN)//没有设置地址前不申请加入网络
   {
       net_printf("nnib.MacAddrType == e_UN\n");
       return;
   }

   StartsyncTEI(buf->snid); //同步流程管理

   StartSTABcnSync(buf->snid);//等到信标启动同步频偏流程

#endif

	//CRC校验
	if(crc32 != 0)
    {
		return;
	}


   
   //异常数据过滤，SNID不合法，长度异常,CCO地址不合法
   if(memcmp(pBeaconheader->CCOmacaddr,ErrMac,6 )==0)
   {
   		net_printf("macaddr is err\n");
		return;
   }
   if(buf->snid==0)
   {
   		net_printf("recev beacon SNID is err\n");
		return;
   }
   if(len ==0 || len>517) //<<<
   {
       net_printf("recev beacon len is =%d\n",len);
	   return;
   }


//   net_printf("simp BCN Flag:%d\n", pBeaconheader->SimpBeaconFlag);
   if(pBeaconheader->SimpBeaconFlag == e_SIMP_BEACON)       //精简信标帧处理
   {
       ProcSimpBeaconIndication(buf, pld, len, crc32);
       return;
   }




//   dump_buf(pld, sizeof(BEACON_HEADER_t));

   //解析信标条目 ，站点能力，时隙分析、路由参数、频率切换(抄控器、解锁条目，设置功率)条目等
   GetEntryHeader(pBeaconheader->BeaconEntryCount, pld+sizeof(BEACON_HEADER_t), &BeaconEntry, len, &beaconlen);

   if(BeaconEntry.pStaAbility == NULL || BeaconEntry.pSlotAllocation ==NULL || BeaconEntry.pRouteParm==NULL)
   {
       net_printf("error : pStaAbility == NULL || pSlotAllocation ==NULL || pRouteParm==NULL\n");
       return;
   }

#if defined(STATIC_NODE)

   if(!checkAccessControlList(BeaconEntry.pStaAbility->SrcMacAddr, 1) )
   {
       if(AccessListSwitch ==TRUE)
       {
           net_printf("beacon checkAccessControlList!:%02x %02x %02x %02x %02x %02x\n",BeaconEntry.pStaAbility->SrcMacAddr[0],
               BeaconEntry.pStaAbility->SrcMacAddr[1],
               BeaconEntry.pStaAbility->SrcMacAddr[2],
               BeaconEntry.pStaAbility->SrcMacAddr[3],
               BeaconEntry.pStaAbility->SrcMacAddr[4],
               BeaconEntry.pStaAbility->SrcMacAddr[5]);
//           net_printf("----------beacon checkAccessControlList!:%02x %02x %02x %02x %02x %02x\n",
//               AccessControlListrow.AccessControlList[0].MacAddr[0],
//               AccessControlListrow.AccessControlList[0].MacAddr[1],
//               AccessControlListrow.AccessControlList[0].MacAddr[2],
//               AccessControlListrow.AccessControlList[0].MacAddr[3],
//               AccessControlListrow.AccessControlList[0].MacAddr[4],
//               AccessControlListrow.AccessControlList[0].MacAddr[5],
//               AccessControlListrow.AccessControlList[0].PermitOrDeny);
           return;
       }

   }
#endif





   //更新邻居表 CCO AND sta
   if(buf->LinkType == e_HPLC_Link)
   {
       U8 phase = 0;
       beacon_ctrl_t *pframe_ctrl = (beacon_ctrl_t *)buf->fi.head;  //帧控制
       if(BeaconEntry.pStaAbility->TEI == CCO_TEI)
           phase = pframe_ctrl->phase;
       else
           phase = BeaconEntry.pStaAbility->Phase;

//       net_printf("snid :%08x, tei:%03x:%03x, mac:%02x%02x%02x%02x%02x%02x\n"
//                  , buf->snid
//                  , pframe_ctrl->stei, BeaconEntry.pStaAbility->TEI
//                  , BeaconEntry.pStaAbility->SrcMacAddr[0]
//                  , BeaconEntry.pStaAbility->SrcMacAddr[1]
//                  , BeaconEntry.pStaAbility->SrcMacAddr[2]
//                  , BeaconEntry.pStaAbility->SrcMacAddr[3]
//                  , BeaconEntry.pStaAbility->SrcMacAddr[4]
//                  , BeaconEntry.pStaAbility->SrcMacAddr[5]);

       UpdateNbInfo(buf->snid,BeaconEntry.pStaAbility->SrcMacAddr,BeaconEntry.pStaAbility->TEI,BeaconEntry.pStaAbility->StaLevel,
                    BeaconEntry.pStaAbility->NodeType,BeaconEntry.pStaAbility->ProxyTEI, 0, buf->rgain, phase,pBeaconheader->BeaconPeriodCount,DT_BEACON);
   }
   else
   {
       UpdateRfNbInfo(buf->snid, BeaconEntry.pStaAbility->SrcMacAddr, BeaconEntry.pStaAbility->TEI, BeaconEntry.pStaAbility->StaLevel,BeaconEntry.pStaAbility->NodeType, BeaconEntry.pStaAbility->ProxyTEI,
                      0, buf->rssi, buf->snr, pBeaconheader->BeaconEntryCount,  BeaconEntry.pStaAbility->RfCount, DT_BEACON);
   }


#if defined(STATIC_MASTER)//更新设备列表 cco




   if(buf->snid != GetSNID())
   {
       //更新邻居网络节点信息，信道冲突处理
       UpdateNbNetInfo(buf->snid,pBeaconheader->CCOmacaddr, pBeaconheader->RfOption, pBeaconheader->RfChannel);
       //主节点做网络维护时，接收到其他网络的信标需要过滤掉
       return;
   }

   SetDeviceList(buf->snid,BeaconEntry.pStaAbility->SrcMacAddr,BeaconEntry.pStaAbility->TEI);
   //adddealtime(BeaconEntry.pStaAbility->TEI,pBeaconheader->BeaconPeriodCount,PHY_TICK2MS(tick2-tick1));


#elif defined(STATIC_NODE)


   //更新无线网络信道参数。
//   net_printf("snid:%08x, Op:%d,Ch:%d\n", buf->snid,pBeaconheader->RfOption, pBeaconheader->RfChannel);
   if(pBeaconheader->RfChannel != 0 && pBeaconheader->RfOption != 0)
   {
       updateNeiNetRfParam(buf->snid, pBeaconheader->RfOption, pBeaconheader->RfChannel);
   }
  

   if(FALSE==STA_JudgeOfflineBybeacon(pBeaconheader->Networkseq,pBeaconheader->CCOmacaddr,
                                      BeaconEntry.pStaAbility->TEI,BeaconEntry.pStaAbility->StaLevel,BeaconEntry.pStaAbility->NodeType, buf))
   {
       //net_printf("FALSE==STA_JudgeOfflineBybeacon \n");
       return;
   }

   if(NwkBeaconPeriodCount == pBeaconheader->BeaconPeriodCount)
   {
       return;
   }
   /*if(time_before_eq(pBeaconheader->BeaconPeriodCount, NwkBeaconPeriodCount))
   {
       return;
   }*/
   HPLC.noise_proc++;//送检打开
   NwkBeaconPeriodCount = SimBeaconPeriodCount = pBeaconheader->BeaconPeriodCount;
    GetBeaconInfo(BeaconEntry.pSlotAllocation,pBeaconheader->beaconType,&CSMADuration,&BindDuration);
   net_printf("cco=%02x%02x%02x%02x%02x%02x,TEI=0x%04x-NwP=0x%08x-State=%d-seq=%02x,NRSTM=%d %d,Lseq=%02x,Ptei=%03x\n"
		,pBeaconheader->CCOmacaddr[0],pBeaconheader->CCOmacaddr[1],pBeaconheader->CCOmacaddr[2]
		,pBeaconheader->CCOmacaddr[3],pBeaconheader->CCOmacaddr[4],pBeaconheader->CCOmacaddr[5]
        ,BeaconEntry.pStaAbility->TEI
	    ,NwkBeaconPeriodCount
	    ,nnib.NodeState
	    ,pBeaconheader->Networkseq
        ,BeaconEntry.pRouteParm->RouteEvaluateRemainTime
		,nnib.NextRoutePeriodStartTime
        ,nnib.FormationSeqNumber
        ,GetProxy());

   slotSeq =GetSlot(pBeaconheader,beaconlen,BeaconEntry.pSlotAllocation, &RfBeaconFlag, &RfBcnCnt);


   net_printf("slotSeq=%d,RfFlg=%d,RfCnt=%d\n", slotSeq, RfBeaconFlag, RfBcnCnt);

   //设置路由周期、信标时隙
   if(BeaconEntry.pRouteParm !=NULL)//设置组网参数和时隙参数
   {
       if(nnib.RoutePeriodTime != BeaconEntry.pRouteParm->RoutePeriod)
       {
           nnib.RoutePeriodTime   = BeaconEntry.pRouteParm->RoutePeriod;//设置路由周期
           nnib.StaOfflinetime    = nnib.RoutePeriodTime * 2;
           nnib.RecvDisListTime   = nnib.RoutePeriodTime * 4;
           nnib.RfRecvDisListTime = nnib.RoutePeriodTime * 4;
       }
       nnib.HeartBeatTime 		= 	 (nnib.RoutePeriodTime * 1000) / 8; //心跳检测周期
       nnib.SuccessRateReportTime = nnib.RoutePeriodTime * 4; //通信成功率上报周期
       nnib.SendDiscoverListTime = nnib.NodeType == e_PCO_NODETYPE ? BeaconEntry.pRouteParm->ProxyDiscoveryPeriod:BeaconEntry.pRouteParm->StaionDiscoveryPeriod;
       nnib.SendDiscoverListTime = (nnib.SendDiscoverListTime == 0 ? (BEACONPERIODLENGTH * 5): nnib.SendDiscoverListTime);
       nnib.SuccessRateZerocnt = nnib.RoutePeriodTime * 4;
//       nnib.StaOfflinetime = nnib.RoutePeriodTime * 2;
//       nnib.RecvDisListTime = nnib.RoutePeriodTime * 4;
   }

   //无线路由参数条目
   if(BeaconEntry.pRfRouteParm)
   {
//       if(nnib.RfDiscoverPeriod != BeaconEntry.pRfRouteParm->DiscoverPeriod && TMR_RUNNING == zc_timer_query(RfDiscoverytimer))
//           StartRfDiscovery();
        if(BeaconEntry.pRfRouteParm->DiscoverPeriod)
            nnib.RfDiscoverPeriod = BeaconEntry.pRfRouteParm->DiscoverPeriod;
        if(BeaconEntry.pRfRouteParm->RfRcvRateOldPeriod)
            nnib.RfRcvRateOldPeriod = BeaconEntry.pRfRouteParm->RfRcvRateOldPeriod;

       net_printf("RfDiscoverPeriod:%d,old:%d\n", nnib.RfDiscoverPeriod,nnib.RfRcvRateOldPeriod);
   }

   if(BeaconEntry.pSlotAllocation !=NULL)//有时隙条目需要更新MAC信标周期信息
   {
       nnib.BeaconLQIFlag = pBeaconheader->UseBeaconflag;
       nnib.BindCSMASlotPhaseCount = BeaconEntry.pSlotAllocation->BindCsmaSlotPhaseCout;
       nnib.CSMASlotPhaseCount = BeaconEntry.pSlotAllocation->CsmaSlotPhaseCount;

       //TODO  调用时隙分配接口
       if(CSMADuration < 15000)
       {
           BCN_SLOT_INFO_t slotINfo;
            memset(&slotINfo,0x00,sizeof(BCN_SLOT_INFO_t));
           slotINfo.BeaconPeriodStartTime = BeaconEntry.pSlotAllocation->BeaconPeriodStartTime;
           slotINfo.slotSeq = slotSeq;
           slotINfo.BeaconSlotTime = BeaconEntry.pSlotAllocation->BeaconSlotTime;
           slotINfo.BeaconSlotCount = BeaconEntry.pSlotAllocation->NonCentralBeaconSlotCount + BeaconEntry.pSlotAllocation->CentralSlotCount;
           slotINfo.CsmaSlotSlice = BeaconEntry.pSlotAllocation->CsmaSlotSlice * 10;
           slotINfo.BindCsmaSlotLinkId = BeaconEntry.pSlotAllocation->BindCsmaSlotLinkId;
           slotINfo.RfBeaconType = RfBeaconFlag;
           slotINfo.RfBeaconCnt  = RfBcnCnt;


           dataLikTimeSlotEntry(slotINfo);
       }
   }
   else
   {
       net_printf("pSlotAllocation == NULL \n");
   }


    //无线信道变更,
    if(BeaconEntry.PRfChannelChange)
    {
        //TODO 启动定时器，变更无线信道
        CCO_changeSTARfChannel(BeaconEntry.PRfChannelChange->RfOption, BeaconEntry.PRfChannelChange->RfChannel
                                , BeaconEntry.PRfChannelChange->ChannelChangeRemainTime);

        net_printf("CCO_changeSTARfChannel,op:%d,ch:%d,time:%d\n"
                                , BeaconEntry.PRfChannelChange->RfOption
                                , BeaconEntry.PRfChannelChange->RfChannel
                                , BeaconEntry.PRfChannelChange->ChannelChangeRemainTime);
    }

   //未入网节点启动入网流程
//   if(GetNodeState() == e_NODE_OFF_LINE)//未入网节点
   if(GetNodeState() != e_NODE_ON_LINE)//未入网节点
   {
//	   if( !HPLC.sfo_not_first )
//	   {
//		   net_printf("Time is Not Sync tick !!!\n");
//		   return;
//	   }
        //入网阶段，如果信道切换错误，通过信标修正
        if(GetWorkState()==e_BEACONSYNC && buf->snid == GetSNID())
        {
            if(pBeaconheader->RfChannel != 0 && pBeaconheader->RfOption != 0 )
            { 
                if(pBeaconheader->RfChannel != getHplcRfChannel() || pBeaconheader->RfOption != getHplcOptin())
                {
                    net_printf("innet set op:%d,chl: %d\n",pBeaconheader->RfOption, pBeaconheader->RfChannel);
                    //根据中央信标同步无线信道号
                    setHplcOptin(pBeaconheader->RfOption);
                    setHplcRfChannel(pBeaconheader->RfChannel);
                }
            }
        }

       //抄控器接管后不发关联请求
       if (TRUE == rd_ctrl_info.period_sendsyh_flag)
       {
          return;
       }

       if(pBeaconheader->Relationshipflag == FALSE)
       {
           net_printf("Relationshipflag is False\n");
           return;
       }
       if( BeaconEntry.pStaAbility->StaLevel == 0x0f)//听到节点为信标为15跳不响应
       {
           net_printf("STA Level > 15 \n");
           return;
       }
       //在网络黑名单中 不发起关联请求
       if(!CheckBlackNegibor(buf->snid))
       {
           net_printf("net %08x in balck net table!!!\n", buf->snid);
           return;
       }
       //设置功率
       if(BeaconEntry.pPowerSet !=NULL)
       {
           SetPowerLevel(BeaconEntry.pPowerSet->TGAINDIG,BeaconEntry.pPowerSet->TGAINANA);
       }
       //启动入网
//       if(HPLC.sfo_is_done == 0) //需要同步后入网    影响台体测试
//	   {
//	   		net_printf("waring:sfo not ready!\n");
//			return;
//	   }
       if(CSMADuration > 250 && CSMADuration < 15000)  //预留250ms让关联汇总能够发出
       {
           U32 delaytime;
		   delaytime = rand()%(CSMADuration - 250);

#if 0
           U32  nowtime = PHY_TICK2MS(get_phy_tick_time());
           U32  csmaStart=PHY_TICK2MS(BeaconEntry.pSlotAllocation->BeaconPeriodStartTime)+
                   (BeaconEntry.pSlotAllocation->CentralSlotCount+BeaconEntry.pSlotAllocation->NonCentralBeaconSlotCount)*BeaconEntry.pSlotAllocation->BeaconSlotTime;
           if(time_after(csmaStart, nowtime) && (nowtime < csmaStart))
           {
              delaytime +=( csmaStart-nowtime);
           }
#endif

           StartSTAnetwork(delaytime,pBeaconheader->Networkseq); //从TDMA结束时间算起
       }
       else
       {
            StartSTAnetwork(0,pBeaconheader->Networkseq);
       }
   }
   else//入网节点
   {

       //更新无线信道编号   接收到载波信标时才同步无线信道。
//       net_printf("rf op:%d,chl: %d\n",pBeaconheader->RfOption, pBeaconheader->RfChannel);
       if(pBeaconheader->RfChannel != 0 && pBeaconheader->RfOption != 0 && buf->LinkType == e_HPLC_Link)
       { 
            if(pBeaconheader->RfChannel != getHplcRfChannel() || pBeaconheader->RfOption != getHplcOptin())
            {
                net_printf("up rf op:%d,chl: %d\n",pBeaconheader->RfOption, pBeaconheader->RfChannel);
                //根据中央信标同步无线信道号
                setHplcOptin(pBeaconheader->RfOption);
                setHplcRfChannel(pBeaconheader->RfChannel);
            }
       }

	   //频段切换
       if( BeaconEntry.pBandChange != NULL)
	   {
           CCO_changeSTAband(BeaconEntry.pBandChange->DstBand,BeaconEntry.pBandChange->BandChangeRemainTime);

           net_printf("CCO_changeSTAband,Band=%d,Ctm=%d\n",
                                 BeaconEntry.pBandChange->DstBand, BeaconEntry.pBandChange->BandChangeRemainTime);
	   }
       //存储功率 芯片送检需要关闭
       if(BeaconEntry.pPowerSet != NULL)
       {
           SetPowerLevel(BeaconEntry.pPowerSet->TGAINDIG,BeaconEntry.pPowerSet->TGAINANA);
           SavePowerlevel();
       }
       //工作模式切换
       if( BeaconEntry.pLinkModeSet !=NULL)
       {
        
           
           nnib.LinkModeStart = TRUE;
           nnib.LinkMode = BeaconEntry.pLinkModeSet->LinkMode;
           nnib.ChangeTime =  BeaconEntry.pLinkModeSet->ChangeTime;
           nnib.DurationTime =  BeaconEntry.pLinkModeSet->DurationTime;
           
           CCO_change_STA_linkmode(BeaconEntry.pLinkModeSet->LinkMode,BeaconEntry.pLinkModeSet->ChangeTime,BeaconEntry.pLinkModeSet->DurationTime);

           net_printf("CCO chg STA lm,lm=%d,Ctm=%ds,Dtm=%ds\n",
                                BeaconEntry.pLinkModeSet->LinkMode,BeaconEntry.pLinkModeSet->ChangeTime,BeaconEntry.pLinkModeSet->DurationTime);
       }

        if(abs(BeaconEntry.pRouteParm->RouteEvaluateRemainTime-nnib.NextRoutePeriodStartTime)>3  && 
              (BeaconEntry.pRouteParm->RouteEvaluateRemainTime)>=10 && 
              (nnib.NextRoutePeriodStartTime >= 10))
		{
				nnib.SynRoutePeriodFlag = TRUE;
                nnib.NextRoutePeriodStartTime = BeaconEntry.pRouteParm->RouteEvaluateRemainTime;
				net_printf("Set NextRoutePeriodStartTime=0x%02x\n",nnib.NextRoutePeriodStartTime);
		}	
        if(BeaconEntry.pStaAbility)     //已入网节点更新到CCO 链路上的无线跳数
        {
            if(GetProxy() == BeaconEntry.pStaAbility->TEI)
            {
                nnib.RfCount = BeaconEntry.pStaAbility->RfCount;

                if(nnib.LinkType == e_RF_Link)
                {
                    nnib.RfCount = BeaconEntry.pStaAbility->RfCount + 1;
                }
            }
        }
       //启动网络维护命令帧发送(心跳、发现列表、通信成功率上报)
      StartNetworkMainten(pBeaconheader->Networkflag);
    //    StartNetworkMainten(TRUE);  //台体发送信标组网完成为0，入网后启动管理消息发送


       //转发信标
       if(slotSeq && HPLC.sfo_is_done == 1)
       {
            //STA 根据安排发送的信标类型来感知组网方式
            if(RfBeaconFlag == e_BEACON_HPLC)           //只安排发送载波信标，默认是载波组网方式
            {
                nnib.NetworkType = e_SINGLE_HPLC;
            }
            else if(RfBeaconFlag == e_BEACON_RF)        //只安排发送无线信标，默认是无线组网方式
            {
                nnib.NetworkType = e_SINGLE_HRF;
            }
            else                                        //安排载波和无线发送信标，默认是混合组网方式
            {
                nnib.NetworkType = e_DOUBLE_HPLC_HRF;
            }
            RelayBeacon(pBeaconheader,len,BeaconEntry.pSlotAllocation,BeaconEntry.pStaAbility, RfBeaconFlag, CSMADuration);
       }

   }
   #endif

   return;
}


#if defined(STATIC_MASTER)
/**
 * @brief ListClear     清空链表 释放内存，防止内存泄漏
 * @param dstList       需要被清空的链表头
 */
void ListClear(list_head_t *dstList)
{
    list_head_t *pos, *n;
    NoCenterNodeInfo_t *pEntry;

    if(list_empty(dstList))
    {
        // printf_s("ListClear empty\n");
        return;
    }

    list_for_each_safe(pos, n, dstList)
    {
        pEntry  = list_entry(pos, NoCenterNodeInfo_t, link);
        list_del(pos);
        zc_free_mem(pEntry);
    }
}
/**
 * @brief hasRfChild
 * @param TEI
 * @param pListHeder
 * @return
 */
U8 hasRfChild(U16 TEI, list_head_t *pListHeder)
{
    list_head_t *n, *pos;
    NoCenterNodeInfo_t *pEntry;

    if(list_empty(pListHeder))
    {
        return FALSE;
    }

    list_for_each_safe(pos, n, pListHeder)
    {
        pEntry = list_entry(pos, NoCenterNodeInfo_t, link);
        if(pEntry->ParentTei == TEI && pEntry->Linktype == e_RF_Link)
            return TRUE;
    }
    return FALSE;
}
/**
 * @brief  时隙安排过程中，如果父节点没有安排发送无线标准信标。将父节点安排在当前节点前面
 * @note
 * @param  *pNode:
 * @param  *pListHeader:
 * @retval None
 */
void findProxyNode(NoCenterNodeInfo_t *pNode, list_head_t *pListHeader)
{
     list_head_t *n, *pos;
    NoCenterNodeInfo_t *pEntry;

    if(pNode->ParentTei == CCO_TEI)
    {
        return;
    }

    list_for_each_safe(pos, n, pListHeader)
    {
        pEntry = list_entry(pos, NoCenterNodeInfo_t, link);
        if(pNode->ParentTei == pEntry->TEI)
        {
            list_del(pos);                      //从源链表重取出。
            list_add(pos, pNode->link.prev);    //将父节点插入到当前节点前面。

            //继续查找父节点的代理节点
            findProxyNode(pEntry, pListHeader);
        }
    }
}

/**
 * @brief  查询链表中是否还又pco节点
 * @note   如果没有pco节点，切可复用时隙不够，可以复用STA载波时隙
 * @param  *srcList:    原始链表
 * @retval TRUE：链表中还又PCO节点， FALSE：链表中没有pco节点
 */
U8  haspcoNode(list_head_t *srcList)
{
    list_head_t *n, *pos;
    NoCenterNodeInfo_t *pEntry;

    list_for_each_safe(pos, n, srcList)
    {
        pEntry = list_entry(pos, NoCenterNodeInfo_t, link);
        if(pEntry->BeaconType == e_PCO_BEACON)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief CheckRfChild      判断当前节点是否是无线分支上的节点
 * @param mTei
 * @param ChildTei
 * @return
 */
static U8 CheckRfChild(U16 mTei, U16 ChildTei)
{
    U8 i;
    U16  seq;

    seq= ChildTei-2;

    for(i=0; i<=MaxDeepth; i++)
    {
        if(DeviceTEIList[seq].ParentTEI == CCO_TEI) //爬到CCO
        {
            return FALSE;
        }
        else if(DeviceTEIList[seq].ParentTEI == mTei) //上爬过程中碰到自己
        {
            if(DeviceTEIList[seq].LinkType == e_RF_Link)
                return TRUE;
            else
                return FALSE;
        }
        seq = DeviceTEIList[seq].ParentTEI-2;
        if(seq >= MAX_WHITELIST_NUM)
            return FALSE;
    }

    return FALSE;
}

/**
 * @brief   从源链表中查找可复用的载波时隙
 * @note    如果当前节点为最后一个PCO 节点，可复用STA载波时隙
 * @param  *pEntry:      当前需要发送无线标准信标帧的节点
 * @param  *srcList:     原始链表
 * @param  RfBcnLen:     需要复用的载波时隙数量，无线标准信标帧长占用信标时隙个数
 * @retval  返回找到的可复用的节点数量。
 */
U8 findReuseNode(list_head_t *destList, list_head_t *srcList,  NoCenterNodeInfo_t *pEntrysrc)
{
    list_head_t *n, *pos;
    NoCenterNodeInfo_t *pEntry;

    U8 cnt = 0;

    list_for_each_safe(pos, n, srcList)
    {

        if(cnt >= pEntrysrc->rfBcnLen)
        {
            return cnt;
        }
        pEntry = list_entry(pos, NoCenterNodeInfo_t, link);
        if(pEntry->RfBcnType == e_BEACON_HPLC || pEntry->RfBcnType == e_BEACON_HPLC_RF_SIMPLE)
        {

            if(CheckRfChild(pEntrysrc->TEI, pEntry->TEI))        //是无线直连站点分支下的节点，无法复用载波时隙
            {
                continue;
            }


            if(pEntry->BeaconType == e_PCO_BEACON)
            {
                cnt++;

                list_del(pos);
                list_add_tail(pos, destList);

                pEntry->RfBcnType = e_BEACON_HPLC_RF_SIMPLE_CSMA;
            }
            else
            {
                if(haspcoNode(srcList))
                {
                    break;
                }
                else                    //没有PCO 可以复用STA ，保证pco都安排在STA前面
                {
                    cnt++;

                    list_del(pos);
                    list_add_tail(pos, destList);
                    pEntry->RfBcnType = e_BEACON_HPLC_RF_SIMPLE_CSMA;
                }
            }
        }

    }

    return cnt;
}

/**
 * @brief setRfBcnType
 * @param pListHeder
 */
U8 standRfBcnCnt;
U8 simpRfBcnCnt ;
void setRfBcnType(list_head_t *pListHeder)
{
    list_head_t *n, *pos;
    NoCenterNodeInfo_t *pEntry;

    if(list_empty(pListHeder))
    {
        net_printf("list_empty\n");
        return;
    }

    list_for_each_safe(pos, n, pListHeder)
    {
        pEntry = list_entry(pos, NoCenterNodeInfo_t, link);

        if(GetNetworkType() == e_SINGLE_HPLC)
        {
            pEntry->RfBcnType = e_BEACON_HPLC;
        }
        else if(GetNetworkType() == e_SINGLE_HRF)
        {
            pEntry->RfBcnType = e_BEACON_RF;
        }
        else if(pEntry->BeaconType == e_DISCOVERY_BEACON)            //叶子节点
        {
            #if 1
            if(pos->next == pListHeder)
            {
                pEntry->RfBcnType = e_BEACON_HPLC_RF_SIMPLE_CSMA;
            }
            else
            {
                pEntry->RfBcnType = e_BEACON_HPLC_RF_SIMPLE;
            }
            pEntry->rfBcnLen = simpRfBcnCnt;
            #else
            if(pEntry->ModuleType == e_MODETYPE_HPLC)           //单载波模块
            {
                pEntry->RfBcnType = e_RFBEACON_HPLC;
            }
            else if(pEntry->ModuleType == e_MODETYPE_RF)           //叶子节点发送无线精简信标帧
            {
                pEntry->RfBcnType = e_RFBEACON_HPLC_SIMRF;
            }
            #endif
        }
        else if(pEntry->BeaconType == e_PCO_BEACON)           //PCO节点，如果安排直连无线站点发送信标，则安排PCO发送无线标准信标
        {
            if(hasRfChild(pEntry->TEI, pListHeder))
            {
                pEntry->RfBcnType = e_BEACON_HPLC_RF;
                pEntry->rfBcnLen = standRfBcnCnt;
            }
            else
            {
                pEntry->RfBcnType = e_BEACON_HPLC_RF_SIMPLE;
                pEntry->rfBcnLen = simpRfBcnCnt;
            }
        }

    }
}

/**
 * @brief  时隙规划算法，根据无线信标发送类型和时隙分配规则。安排信标时隙
 * @note
 * @param  *srcList:        本周起需要发送信标节点信息，原始链表
 * @param  *destList:       最终时隙规划完成后，信标时隙安排情况。
 * @retval None
 */
void RfNoCenterInfoTrans(list_head_t *srcList, list_head_t *destList)
{
    if(srcList == NULL || destList == NULL)
    {
        net_printf("List Addr is NULL \n");
        return;
    }

    if(list_empty(srcList))
    {
        net_printf("srcList empty!\n");
        return;
    }

    //分配节点无线信标类型
    setRfBcnType(srcList);

    list_head_t *n, *pos;
    NoCenterNodeInfo_t *pEntry;
    U8 i;

//    U8  RfLen = 1;                  //假设无线标准帧长占用2个信标时隙。
//    U8  RfsimLen = 1;               //无线精简信标占用一个信标时隙。


    list_for_each_safe(pos, n, srcList)
    {
        pEntry = list_entry(pos, NoCenterNodeInfo_t, link);


        //step 1 找出需要发送无线标准信标的节点。
        if(pEntry->RfBcnType == e_BEACON_HPLC_RF || pEntry->RfBcnType == e_BEACON_RF)       //找到需要发送无线标准信标的节点
        {
            list_del(pos);                      //从源链表中删除

            list_add_tail(pos, destList);            //插入到目的链表
            findProxyNode(pEntry, srcList);     //聪源链表中查找代理节点，插入到当前节点前面

            //查找可复用载波时隙的节点
            U8 cnt = findReuseNode(destList, srcList, pEntry);
            if (cnt == 0 && nnib.NetworkType != e_SINGLE_HRF)
            {
                //如果没有可复用的载波时隙，第一个时隙片安排只发送载波信标
                net_printf("err-----cnt:%d, NetworkType:%d\n", cnt, nnib.NetworkType);
                pEntry->RfBcnType = e_BEACON_HPLC;
            }
            if(cnt < pEntry->rfBcnLen)
            {
                //给当前节点多安排  RfLen - cnt 个信标时隙 插到链表最后面
                for(i = 0; i < pEntry->rfBcnLen - cnt; i++)
                {
                    NoCenterNodeInfo_t *pNode = (NoCenterNodeInfo_t *)zc_malloc_mem(sizeof(NoCenterNodeInfo_t), "PCO", MEM_TIME_OUT);

                    pNode->TEI          =   pEntry->TEI        ;
                    pNode->BeaconType   =   pEntry->BeaconType ;
//                    pNode->RfBcnType    =   pEntry->RfBcnType  ;
                    pNode->RfBcnType    =   e_BEACON_RF  ;
                    pNode->ParentTei    =   pEntry->ParentTei  ;
                    pNode->Linktype     =   pEntry->Linktype   ;
                    pNode->ModuleType   =   pEntry->ModuleType ;

                    list_add_tail(&pNode->link, destList);            //插入到目的链表

                }
            }

            n = srcList->next;   // 链表被修改，每次循环从头遍历
        }

    }

    if(!list_empty(srcList))        //将最后剩下的节点 追加到destlist链表
    {
        list_for_each_safe(pos, n, srcList)
        {
            list_del(pos);
            list_add_tail(pos, destList);
        }
    }

}
/**
 * @brief Get the Beacon Payload Len object     516-108 = 408（非中央信标时隙字节长度）  
 *                                              408/3 = 136 最多支持136个PCO同时发送无线标准信标
 * 
 * @return U16                                  最大长度为108bytes。
 */
static U16 GetBeaconPayloadLen()
{

    //信标帧头长度
    U16 len = sizeof(BEACON_HEADER_t);      //21 bytes for header

    //设置站点能力条目
    len+= sizeof(STA_ABILITY_STRUCT_t);     //15 bytes 

    //路由参数条目
    len+= sizeof(ROUTE_PARM_STRUCT_t);      //10 bytes

    //无线路由参数
    len+= sizeof(RF_ROUTE_PARM_STRUCT_t);   //4 bytes

    //无线信道变更条目
    if(TRUE == nnib.RfChannelChangeState)   //8 bytes
    {
        len += sizeof(RF_CHANNEL_CHANGE_STRUCT_t);
    }
    //频段变更通知条目
    if(TRUE == nnib.BandChangeState)        //7 bytes 
    {
        len += sizeof(BAND_CHANGE_STRUCT_t);
    }

    //设置功率
    if(TRUE == nnib.powerlevelSet)          //7 bytes
    {
        len += sizeof(POWERSET_STRUCT_t);
    }

    //设置链路工作模式
    if(TRUE == nnib.LinkModeStart)          //7 bytes
    {
        len += sizeof(LINKMODESET_STRUCT_t);
    }

    //时隙设置条目                          //23+6 bytes
   len += sizeof(SLOT_ALLOCATION_STRUCT_t) + 6;   //使用时，需要加上非中央信标信息长度




    return len;
}

/**
 * @brief GetBcnSlotCnt         计算无线信标占用载波时隙个数。
 * @param pTimeSlotParam
 * @param staNum
 * @param pcoNum
 * @param srcList
 */
void GetBcnSlotCnt(TIME_SLOT_PARAM_t *pTimeSlotParam, U16 staNum, U16 pcoNum)
{
    U32 standRfBcnLen = PHY_TICK2US(wphy_get_frame_dur(getHplcOptin(), 2, 2, GetBeaconPayloadLen() + (pcoNum + staNum) * 2, 1)) + 1000 ;
    U32 simpRfBcnLen = PHY_TICK2US(wphy_get_frame_dur(getHplcOptin(), 2, 2, 40, 1)) + 1000;

    net_printf("BcnPdLen:%d,pNum:%d,sNum:%d\n", GetBeaconPayloadLen() + (pcoNum + staNum) * 2, pcoNum, staNum);

    standRfBcnCnt = (standRfBcnLen / (pTimeSlotParam->beacon_slot *1000));
    if(standRfBcnLen % (pTimeSlotParam->beacon_slot *1000))
    {
        standRfBcnCnt += 1;
    }
    
    simpRfBcnCnt = (simpRfBcnLen / (pTimeSlotParam->beacon_slot *1000));
    if(simpRfBcnLen % (pTimeSlotParam->beacon_slot *1000))
    {
        simpRfBcnCnt += 1;
    }

    net_printf("standRfBcnCnt : %d, simpRfBcnCnt:%d\n", standRfBcnCnt, simpRfBcnCnt);

}
#if defined(POWER_MANAGER)
extern LOW_POWER_CTRL_t LWPR_INFO;
#endif

static U8 BeaconSlotSchedule(BEACON_SLOT_SCHEDULE *pBeaconSlotSchedule_t, TIME_SLOT_PARAM_t *pTimeSlotParam)
{
    NON_CENTRAL_BEACON_INFO_t 	*pNonCenterBeaconX;
    NON_CENTRAL_BEACON_INFO_t   *pNonCenterBeacon = (NON_CENTRAL_BEACON_INFO_t *)pTimeSlotParam->pNonCenterInfo;
    U8 NonCentralBeaconSlotCount=0;
	U16 BeaconOffset=0;
    U16 tei,i;
    U8	BeaconStaNum;
	U16 staNum=0;
    U16 pcoNum=0;

    U32 CsmaSliceLength = BEACONPERIODLENGTH;

    getStaPcoNum();

    //无线信标时隙分配算法用到的链表头
    list_head_t srcList;
    list_head_t dstList;
    INIT_LIST_HEAD(&srcList);
    INIT_LIST_HEAD(&dstList);


//#if defined(STATIC_MASTER)
//    pTimeSlotParam->RfBeaconType = e_BEACON_HPLC_RF_SIMPLE;
//    pTimeSlotParam->RfBeaconSlotCnt = MAX(simpRfBcnCnt, 3);
//#endif


    switch(JoinCtrl_t.JoinMachine)
    {
        case e_JoinIdel://组网刚开始
            CsmaSliceLength = BEACONPERIODLENGTH - 60;
            if( (nnib.discoverSTACount +nnib.PCOCount) > 0)
            {
                net_printf("JoinCtrl_t.JoinMachine = e_JoinStart\n");
                JoinCtrl_t.JoinMachine = e_JoinStart;
                pBeaconSlotSchedule_t->ThistimeSendStaNum =0;
                g_SendCenterBeaconCountWithoutRespond=0;
                
                //第一个节点入网后退出低功耗模式
        	    if(check_lowpower_states_hook != NULL)
                {   
                    check_lowpower_states_hook();
                }
            }
            else
            {
                JoinCtrl_t.LastLevelInNetNum =0;
                JoinCtrl_t.ThisLevelJoinNum =0;
                nnib.BeaconPeriodLength = BEACONPERIODLENGTH;

                {
                    //计算无线信标帧长
                    GetBcnSlotCnt(pTimeSlotParam, staNum, pcoNum);
                    if(GetNetworkType() == e_SINGLE_HPLC)
                    {
                        pTimeSlotParam->RfBeaconType = e_BEACON_HPLC;
                        pTimeSlotParam->RfBeaconSlotCnt = MAX(simpRfBcnCnt, 3);
                    }
                    else if(GetNetworkType() == e_SINGLE_HRF)
                    {
                        pTimeSlotParam->RfBeaconType = e_BEACON_RF;
                        pTimeSlotParam->RfBeaconSlotCnt = MAX(standRfBcnCnt, 3);
                    }
                    else
                    {
                        pTimeSlotParam->RfBeaconType = e_BEACON_HPLC_RF_SIMPLE;
                        pTimeSlotParam->RfBeaconSlotCnt = MAX(simpRfBcnCnt, 3);
                    }
                }
                //低功耗模式下，信标周期长度，不小于10秒
                #if defined(POWER_MANAGER)
                if(TRUE == LWPR_INFO.LowPowerStates)
                {
                    nnib.BeaconPeriodLength =  LOWPOWER_MAXSLICE;
                }
                #endif
                net_printf("lptyBLen=%d\n",nnib.BeaconPeriodLength);
                return NonCentralBeaconSlotCount =0;
            }
        break;
        case e_JoinStart://从模块开始入网

            if(g_SendCenterBeaconCountWithoutRespond>=MaxSendBeaconCountWithoutRespond)//连续几轮没有新的节点入网
            {
                g_SendCenterBeaconCountWithoutRespond=0;
                //计算本次入网数量
                //JoinCtrl_t.ThisLevelJoinNum =nnib.discoverSTACount +nnib.PCOCount-JoinCtrl_t.LastLevelInNetNum;
                if(nnib.discoverSTACount +nnib.PCOCount > JoinCtrl_t.LastLevelInNetNum)
                {
                    JoinCtrl_t.ThisLevelJoinNum =nnib.discoverSTACount +nnib.PCOCount-JoinCtrl_t.LastLevelInNetNum;
                }
                else
                {
                    JoinCtrl_t.ThisLevelJoinNum = 0;
                }
                JoinCtrl_t.LastLevelInNetNum = nnib.discoverSTACount +nnib.PCOCount;
                SetNodeMachine(e_IsSendBeacon,e_HasSendBeacon);
                if(JoinCtrl_t.ThisLevelJoinNum)//本轮有新入网节点，开始下一层组网
                {
                    JoinCtrl_t.JoinMachine = e_JoinListen;

                    g_RelationSwitch = FALSE;//关闭允许关联标志
                    net_printf("JoinCtrl_t.JoinMachine will be e_JoinListen，ThisLevelJoinNum=%d，LastLevelInNetNum=%d \n",JoinCtrl_t.ThisLevelJoinNum,JoinCtrl_t.LastLevelInNetNum);


                    SetNodeMachine(e_NewNode,e_IsSendBeacon);

                    //在listen阶段，尽量安排本轮发现的所有新入节点,信标周期根据时隙数量变化
                   pBeaconSlotSchedule_t->HadSendStaNum =0;
                   pBeaconSlotSchedule_t->ThistimeSendStaNum = MIN( MaxPayloadSoltNum-nnib.PCOCount,JoinCtrl_t.ThisLevelJoinNum);
                   pBeaconSlotSchedule_t->MaxListenTimes  = NoRelationTimes;
                   pBeaconSlotSchedule_t->CurrentListenTimes =0;

                   CsmaSliceLength = MINCSMASLICE + 100;

                }
                else//说明组网完成
                {
                    net_printf("JoinCtrl_t.JoinMachine will be e_JoinFinish_Most\n");
                    pBeaconSlotSchedule_t->HadSendStaNum =0;
                    JoinCtrl_t.JoinMachine = e_JoinFinish_Most;
  
//                    nnib.RoutePeriodTime = RoutePeriodTime_Normal;
//                    nnib.NextRoutePeriodStartTime = nnib.RoutePeriodTime;//初始化路由周期剩余时间
                    //nnib.SendDiscoverListCount =0;
//                    nnib.BeaconPeriodLength = BEACONPERIODLENGTH1;
                    CsmaSliceLength = BEACONPERIODLENGTH1;
                    net_printf("most  nnib.BeaconPeriodLength : %d\n", nnib.BeaconPeriodLength);
                    pBeaconSlotSchedule_t->ThistimeSendStaNum = MIN( nnib.discoverSTACount-pBeaconSlotSchedule_t->HadSendStaNum,20);
                }
            }
            else
            {
                if(pBeaconSlotSchedule_t->HadSendStaNum >= JoinCtrl_t.ThisLevelJoinNum) //发完一轮所有发现信标
                {
                    pBeaconSlotSchedule_t->HadSendStaNum =0; //清零后重头再发
                    g_SendCenterBeaconCountWithoutRespond++;

                }
                pBeaconSlotSchedule_t->ThistimeSendStaNum = MIN( JoinCtrl_t.ThisLevelJoinNum-pBeaconSlotSchedule_t->HadSendStaNum,MaxPayloadSoltNum-nnib.PCOCount);
                net_printf("e_JoinStarting,ThisLevelJoinNum=%d ,HadSendStaNum=%d\n",JoinCtrl_t.ThisLevelJoinNum,pBeaconSlotSchedule_t->HadSendStaNum);

                //设置信标周期
                //if(pBeaconSlotSchedule_t->ThistimeSendStaNum)
                {
                    CsmaSliceLength = JOIN_CSMA_LEN ;//JOIN 状态将信标周期放大,根据实际安排信标数计算
                }

            }
        break;
        case e_JoinListen:

            if(pBeaconSlotSchedule_t->HadSendStaNum >= JoinCtrl_t.ThisLevelJoinNum) //发完一轮所有发现信标
            {
                pBeaconSlotSchedule_t->HadSendStaNum =0;
                if(pBeaconSlotSchedule_t->CurrentListenTimes  < pBeaconSlotSchedule_t->MaxListenTimes)//发送下一轮
                {

                   pBeaconSlotSchedule_t->CurrentListenTimes++;
				   net_printf("----CurrentListenTimes++\n");
				   CsmaSliceLength = MINCSMASLICE + 100 ;
                }
                else//轮数发完进入下一个状态
                {
                    JoinCtrl_t.JoinMachine = e_JoinStart;
                    g_RelationSwitch = TRUE;//允许关联标志

                    CsmaSliceLength = JOIN_CSMA_LEN ;//JOIN 状态将信标周期放大,根据实际安排信标数计算
                    net_printf("JoinCtrl_t.JoinMachine will be e_JoinStart ,ThisLevelJoinNum=%d ,%d\n",JoinCtrl_t.ThisLevelJoinNum,CsmaSliceLength);

                }

            }
            pBeaconSlotSchedule_t->ThistimeSendStaNum = MIN( MaxPayloadSoltNum-nnib.PCOCount,JoinCtrl_t.ThisLevelJoinNum-pBeaconSlotSchedule_t->HadSendStaNum);
        break;
        case e_JoinFinish_Most:
            //完成组网后每包最多安排20个发现信标
            if(pBeaconSlotSchedule_t->HadSendStaNum >= nnib.discoverSTACount) //发完一轮所有发现信标
            {
                pBeaconSlotSchedule_t->HadSendStaNum =0; //清零后重头再发

            }
			CsmaSliceLength = JOIN_CSMA_LEN ;
            pBeaconSlotSchedule_t->ThistimeSendStaNum = MIN( nnib.discoverSTACount-pBeaconSlotSchedule_t->HadSendStaNum,20);
        break;
        case e_JoinFinish_All:
            //完成组网后每包最多安排5个发现信标

            if(pBeaconSlotSchedule_t->HadSendStaNum >= nnib.discoverSTACount) //发完一轮所有发现信标
            {
                pBeaconSlotSchedule_t->HadSendStaNum =0; //清零后重头再发

            }

//            BeaconStaNum =20;

//            extern U16  timercnt;
//            if(timercnt >AFTER_FINEWORK_COUNT)
//            {
//                BeaconStaNum = 10;
//            }
//            else
            {
                BeaconStaNum =MIN(MaxPayloadSoltNum - nnib.PCOCount, 34);
                //BeaconStaNum =34;
            }

            pBeaconSlotSchedule_t->ThistimeSendStaNum = MIN( nnib.discoverSTACount-pBeaconSlotSchedule_t->HadSendStaNum,BeaconStaNum);

            CsmaSliceLength = (GetDeviceNum() >=100 ? NORMAL_CSMA_LEN :
                                                            GetDeviceNum() >=50 ?NORMAL_CSMA_LEN_3 : NORMAL_CSMA_LEN_2);

            net_printf("csmalen : %d\n", CsmaSliceLength);


        break;
        default:

            break;
    }
    U8 level =1,num=0;
    net_printf("PCO=%d tei:",nnib.PCOCount);

    for(i=0;i< nnib.PCOCount;i++)
    {



            pNonCenterBeaconX = (NON_CENTRAL_BEACON_INFO_t*)&pNonCenterBeacon[BeaconOffset];
            tei = SelectAppointProxyNodebyLevel(num,level);

            if(tei !=0xffff)
            {
                pNonCenterBeaconX->SrcTei = tei;

                pNonCenterBeaconX->BeaconType =e_PCO_BEACON;

                if(DeviceTEIList[tei-2].ModuleType == e_HPLC_Module)    //单载波模块只发送送载波信标
                {
                    pNonCenterBeaconX->RfBeaconType = e_BEACON_HPLC;
                }
                // else if(GetNetworkType() == e_SINGLE_HPLC)
                // {
                //     pNonCenterBeaconX->RfBeaconType = e_BEACON_HPLC;
                // }
                // else if(GetNetworkType() == e_SINGLE_HRF)
                // {
                //     pNonCenterBeaconX->RfBeaconType = e_BEACON_RF;
                // }
                else
                {
                    pNonCenterBeaconX->RfBeaconType = e_BEACON_HPLC_RF;
                }
                BeaconOffset +=2;
                num++;
                pcoNum++;
                //net_printf("tei=%04x p=%d",tei,DeviceTEIList[tei - 2].LogicPhase);
				net_printf("%03x ",tei);


                NoCenterNodeInfo_t *pNode = (NoCenterNodeInfo_t *)zc_malloc_mem(sizeof(NoCenterNodeInfo_t), "PCO", MEM_TIME_OUT);
                if(pNode)
                {
                   pNode->TEI          = tei;
                   pNode->BeaconType   = e_PCO_BEACON;
//                   pNode->RfBcnType    = e_BEACON_HPLC_RF;
                   pNode->ParentTei    = DeviceTEIList[tei - 2].ParentTEI;
                   pNode->Linktype     = DeviceTEIList[tei - 2].LinkType;
                   pNode->ModuleType   = DeviceTEIList[tei - 2].ModuleType;

                   list_add_tail(&pNode->link, &srcList);
                }

            }
            else
            {
                level++;
                num =0;
                i--;

            }


    }
    net_printf("\n");

    ////非中央时隙字段中STA
    net_printf("STA=%d :",nnib.discoverSTACount);
    for(i=pBeaconSlotSchedule_t->HadSendStaNum; i<= pBeaconSlotSchedule_t->ThistimeSendStaNum+pBeaconSlotSchedule_t->HadSendStaNum; i++)
    {

        pNonCenterBeaconX = (NON_CENTRAL_BEACON_INFO_t*)&pNonCenterBeacon[BeaconOffset];
        if(JoinCtrl_t.JoinMachine == e_JoinFinish_Most || JoinCtrl_t.JoinMachine == e_JoinFinish_All)
        {
            if( (i) == pBeaconSlotSchedule_t->ThistimeSendStaNum+pBeaconSlotSchedule_t->HadSendStaNum
                                &&(nnib.discoverSTACount>=20)) //
            {
                tei = SelectAppointHighstlevelSTANode();
                //pBeaconSlotSchedule_t->ThistimeSendStaNum = pBeaconSlotSchedule_t->ThistimeSendStaNum-1;//因为安排了一个最外侧的叶子，少安排了一个计划内的节点
            }
            else
            {
                if(i == pBeaconSlotSchedule_t->ThistimeSendStaNum+pBeaconSlotSchedule_t->HadSendStaNum )
                continue;
                tei = SelectAppointSTANode(i);
            }

        }
        else
        {
            if(i == pBeaconSlotSchedule_t->ThistimeSendStaNum+pBeaconSlotSchedule_t->HadSendStaNum )
            continue;
            tei = SelectAppointNewSTANode(i);
        }
        if(tei !=0xffff)
        {
            pNonCenterBeaconX->SrcTei = tei;

            pNonCenterBeaconX->BeaconType =e_DISCOVERY_BEACON;
            if(DeviceTEIList[tei-2].ModuleType == e_HPLC_Module)
            {
                pNonCenterBeaconX->RfBeaconType =e_BEACON_HPLC;                 //单载波模块只发送载波信标帧
            }
            // else if(GetNetworkType() == e_SINGLE_HPLC)
            // {
            //     pNonCenterBeaconX->RfBeaconType = e_BEACON_HPLC;
            // }
            // else if(GetNetworkType() == e_SINGLE_HRF)
            // {
            //     pNonCenterBeaconX->RfBeaconType = e_BEACON_RF;
            // }
            else
            {
                //STA 发送无线精简信标。根据信标发送是否在信标时隙区分 e_BEACON_HPLC_RF_SIMPLE/e_BEACON_HPLC_RF_SIMPLE_CSMA
                pNonCenterBeaconX->RfBeaconType =e_BEACON_HPLC_RF_SIMPLE;       //默认在信标时隙发送精简信标
            }
            BeaconOffset +=2;
            staNum++;
            net_printf("%03x ",tei);
//            net_printf("tei=%04x p=%d",tei,DeviceTEIList[tei - 2].LogicPhase);

            NoCenterNodeInfo_t *pNode = (NoCenterNodeInfo_t *)zc_malloc_mem(sizeof(NoCenterNodeInfo_t), "STA", MEM_TIME_OUT);
            if(pNode)
            {
               pNode->TEI          = tei;
               pNode->BeaconType   = e_DISCOVERY_BEACON;
//                   pNode->RfBcnType    = ;
               pNode->ParentTei    = DeviceTEIList[tei - 2].ParentTEI;
               pNode->Linktype     = DeviceTEIList[tei - 2].LinkType;
               pNode->ModuleType   = DeviceTEIList[tei - 2].ModuleType;

               list_add_tail(&pNode->link, &srcList);
            }


        }
        else
        {
            //U16 ii=0,jj=0;
            //pBeaconSlotSchedule_t->ThistimeSendStaNum--;
            net_printf("not found New Node ,i=%d \n",i);
            break;

        }

    }
    net_printf("\n");
    //printf_s("BeaconOffset %d",BeaconOffset);
#if defined(STATIC_MASTER)

    //计算无线信标帧长
    GetBcnSlotCnt(pTimeSlotParam, staNum, pcoNum);


    //设置CCO 无线信标类型
    if(GetNetworkType() == e_SINGLE_HPLC)
    {
        pTimeSlotParam->RfBeaconType = e_BEACON_HPLC;
        pTimeSlotParam->RfBeaconSlotCnt = MAX(simpRfBcnCnt, 3);
    }
    else if(GetNetworkType() == e_SINGLE_HRF)
    {
        pTimeSlotParam->RfBeaconType = e_BEACON_RF;
        pTimeSlotParam->RfBeaconSlotCnt = MAX(standRfBcnCnt, 3);
    }
    else if(hasRfChild(CCO_TEI, &srcList))   //安排无线子节点发送信标，安排当前节点发送无线标准信标，否则发送无线精简信标
    {
        pTimeSlotParam->RfBeaconType = e_BEACON_HPLC_RF;
        pTimeSlotParam->RfBeaconSlotCnt = MAX(standRfBcnCnt, 3);
    }
    else
    {
        pTimeSlotParam->RfBeaconType = e_BEACON_HPLC_RF_SIMPLE;
        pTimeSlotParam->RfBeaconSlotCnt = MAX(simpRfBcnCnt, 3);
    }
#endif
    //无线信标时隙规划
    RfNoCenterInfoTrans(&srcList, &dstList);
    U16 NonCenterCnt = 0;
    //TODO  时隙分配完成后按照顺序填到非中央信标字段
    if(!list_empty(&dstList))
    {
        pNonCenterBeaconX = (NON_CENTRAL_BEACON_INFO_t*)pNonCenterBeacon;
        list_head_t *pos, *n;
        NoCenterNodeInfo_t *pEntry;
        list_for_each_safe(pos, n, &dstList)
        {
            pEntry = list_entry(pos, NoCenterNodeInfo_t, link);

            pNonCenterBeaconX->SrcTei = pEntry->TEI;
            pNonCenterBeaconX->BeaconType = pEntry->BeaconType;
            pNonCenterBeaconX->RfBeaconType = pEntry->RfBcnType;

            pNonCenterBeaconX++;

            NonCenterCnt++;

            list_del(pos);
            zc_free_mem(pEntry);

        //    net_printf("<%03x, %d> ", pEntry->TEI, pEntry->RfBcnType);
        }
    //    net_printf("\n");
    }

    //printf_s("pNonCenterInfo <%d>:%p\n", NonCenterCnt,pNonCenterBeaconX);
//    dump_level_buf(DEBUG_MDL, (U8*)pNonCenterBeacon, 2*NonCenterCnt);
    //清空链表
    ListClear(&srcList);
    ListClear(&dstList);



    //增加已经发送的发现站点的数量
    if(e_JoinFinish_All == JoinCtrl_t.JoinMachine)//组网完成状态下，发现信标安排5轮更换,防止同步不上
    {
            nnib.beaconSendTime++;
            if(nnib.beaconSendTime >=1)
            {
                nnib.beaconSendTime =0;
                pBeaconSlotSchedule_t->HadSendStaNum +=pBeaconSlotSchedule_t->ThistimeSendStaNum;
            }
    }
    else
    {
        pBeaconSlotSchedule_t->HadSendStaNum +=pBeaconSlotSchedule_t->ThistimeSendStaNum;
    }

    net_printf("TNum =%d sta=%d\n",pBeaconSlotSchedule_t->ThistimeSendStaNum,staNum);
//    NonCentralBeaconSlotCount = staNum+nnib.PCOCount;
    NonCentralBeaconSlotCount = NonCenterCnt;
    nnib.BeaconPeriodLength = (NonCentralBeaconSlotCount + 3) * g_BeaconSlotTime + CsmaSliceLength;
    if(nnib.BeaconPeriodLength < 2000)
    {
        nnib.BeaconPeriodLength = 2000;
    }
    if(nnib.BeaconPeriodLength > MAXSLICE)
    {
        nnib.BeaconPeriodLength = MAXSLICE;
    }

    //双模台体测试，CCO同步测试项，测试过程需要保证信标周期不变
    if(NonCentralBeaconSlotCount < 10)
        nnib.BeaconPeriodLength = 2000;
    
    return NonCentralBeaconSlotCount;
}

//函数名称：设置路由参数通知条目

static void SetRouteParmEntry( ROUTE_PARM_STRUCT_t 	*pRouteParm)
{



    pRouteParm->RoutePeriod = nnib.RoutePeriodTime;
#if defined(STATIC_MASTER)
    if(nnib.Networkflag != TRUE)
    {
        pRouteParm->RouteEvaluateRemainTime = 0;
    }
    else
#endif
    {
        if(nnib.RoutePeriodTime - nnib.NextRoutePeriodStartTime < 10 || nnib.NextRoutePeriodStartTime < 10)
        {
            pRouteParm->RouteEvaluateRemainTime = 0;//nnib.NextRoutePeriodStartTime;
        }
        else
        {
            pRouteParm->RouteEvaluateRemainTime = nnib.NextRoutePeriodStartTime;
        }
    }

    pRouteParm->BeaconEntryType = e_ROUTE_PARM_TYPE;
    pRouteParm->EntryLength	 = ROUTE_PARM_ENTRY_LENGTH;
    pRouteParm->ProxyDiscoveryPeriod = nnib.RoutePeriodTime/10;
    pRouteParm->StaionDiscoveryPeriod = pRouteParm->ProxyDiscoveryPeriod;//*2;

}
#ifdef HPLC_HRF_PLATFORM_TEST
/**
 * @brief SetRfRouteParam       设置无线路由参数条目
 * @param pRfRouteParam         无线路由参数条目数据地址
 */
static void SetRfRouteParam(RF_ROUTE_PARM_STRUCT_t *pRfRouteParam)
{
    pRfRouteParam->BeaconEntryType    = e_RF_ROUTE_PARM_TYPE;
    pRfRouteParam->EntryLength	      = sizeof(RF_ROUTE_PARM_STRUCT_t);
    pRfRouteParam->DiscoverPeriod     = nnib.RfDiscoverPeriod;
    pRfRouteParam->RfRcvRateOldPeriod = nnib.RfRcvRateOldPeriod;
}

static void SetRfChnChangeEntry(RF_CHANNEL_CHANGE_STRUCT_t *pRfChnChgParam)
{
    pRfChnChgParam->BeaconEntryType             = e_RF_CHANNEL_CHANGE_TYPE;
    pRfChnChgParam->EntryLength                 = sizeof(RF_CHANNEL_CHANGE_STRUCT_t);
    pRfChnChgParam->RfChannel                   = DefSetInfo.RfChannelInfo.channel;
    pRfChnChgParam->RfOption                    = DefSetInfo.RfChannelInfo.option;
    pRfChnChgParam->ChannelChangeRemainTime     = nnib.RfChgChannelRemainTime*1000;
}  
#endif

static void SetbandChangeEntry(BAND_CHANGE_STRUCT_t * pBandChange)
{

    pBandChange->BeaconEntryType = e_BAND_CHANGE_TYPE;
    pBandChange->EntryLength = sizeof(BAND_CHANGE_STRUCT_t);
    pBandChange->DstBand = DefSetInfo.FreqInfo.FreqBand;
    pBandChange->BandChangeRemainTime =  nnib.BandRemianTime*1000;

}
static void SetpowerEntry(POWERSET_STRUCT_t * pPowerSet)
{

    pPowerSet->BeaconEntryType = e_POWERLEVELENITY;
    pPowerSet->EntryLength = sizeof(POWERSET_STRUCT_t);
    pPowerSet->TGAINDIG = nnib.TGAINDIG;
    pPowerSet->TGAINANA = nnib.TGAINANA;

}

static void set_linkmode_entry(LINKMODESET_STRUCT_t * pLinkModeSet)
{

    pLinkModeSet->BeaconEntryType = e_LINKMODE_SET_ENITY;
    pLinkModeSet->EntryLength = sizeof(LINKMODESET_STRUCT_t);
    pLinkModeSet->LinkMode = nnib.LinkMode;
    pLinkModeSet->ChangeTime = nnib.ChangeTime;
    pLinkModeSet->DurationTime = nnib.DurationTime;
}

static void SetsoltEntry(SLOT_ALLOCATION_STRUCT_t		*pSlotAllocation,U32 *CsmaSliceLength)
{
#if 0
    U8								i;
    U16 							A_Num=0,B_Num=0,C_Num=0,LevelA_B_C=0;
    U32 							CsmaSliceLength_A,CsmaSliceLength_B,CsmaSliceLength_C;
    NON_CENTRAL_BEACON_INFO_t		*pNonCenterBeacon;
    CSMA_SLOT_INFO_t				*pCsmaSlotInfo;

    pSlotAllocation->BeaconEntryType = e_SLOT_ALLOCATION_TYPE;
    pSlotAllocation->EntryLength = sizeof(SLOT_ALLOCATION_STRUCT_t);
    pSlotAllocation->CentralSlotCount = CenterSlotCount;//A\B\C

    pSlotAllocation->CsmaSlotPhaseCount = nnib.CSMASlotPhaseCount;

    pSlotAllocation->ProxySlotCount = nnib.PCOCount;

    pSlotAllocation->BeaconSlotTime = g_BeaconSlotTime ;//

    pSlotAllocation->CsmaSlotSlice	= CSMA_SLOT_SLICE;
    pSlotAllocation->BindCsmaSlotPhaseCout = 0;
    pSlotAllocation->BindCsmaSlotLinkId = 0;
    pSlotAllocation->TdmaSlotLength = 0;
    pSlotAllocation->TdmaSlotLinkId = 0;

    pSlotAllocation->Reserve = 0;
    //非中央时隙安排
    pNonCenterBeacon = (NON_CENTRAL_BEACON_INFO_t*) pSlotAllocation->info;
    pSlotAllocation->NonCentralBeaconSlotCount = BeaconSlotSchedule(&BeaconSlotSchedule_t,(U8*)pNonCenterBeacon);


//    modify_sendcenterbeacon_timer(nnib.BeaconPeriodLength);

    nnib.BeaconPeriodLength -= (nnib.BeaconPeriodLength % 10);

    //dump_buf((U8*)pNonCenterBeacon,pSlotAllocation->NonCentralBeaconSlotCount*2);
    pSlotAllocation->EntryLength += pSlotAllocation->NonCentralBeaconSlotCount*2;

    pSlotAllocation->BeaconPeriodLength = nnib.BeaconPeriodLength;



    //CSMA时隙信息,计算一跳点A\B\C比例
    LevelA_B_C = GetPhaseParm(&A_Num,&B_Num,&C_Num);
    //CSMA总时间
    *CsmaSliceLength= nnib.BeaconPeriodLength -(pSlotAllocation->NonCentralBeaconSlotCount + CenterSlotCount) * g_BeaconSlotTime;

    net_printf("nnib.BeaconPeriodLength : %d Noncentral beacon slot count : %d A_Num=%d,B_Num=%d,C_Num=%d,LevelA_B_C=%D\n",
                nnib.BeaconPeriodLength,pSlotAllocation->NonCentralBeaconSlotCount,A_Num,B_Num,C_Num,LevelA_B_C);
    if(LevelA_B_C>0)
    {
        CsmaSliceLength_A = 200 + (A_Num* (*CsmaSliceLength-MINCSMASLICE) )/LevelA_B_C;
        CsmaSliceLength_B = 200 + (B_Num* (*CsmaSliceLength-MINCSMASLICE) )/LevelA_B_C;
        CsmaSliceLength_C = *CsmaSliceLength-CsmaSliceLength_A-CsmaSliceLength_B;
        net_printf("CsmaSliceLengt=%d,A=%d,B=%d,C=%d\n",*CsmaSliceLength,CsmaSliceLength_A,CsmaSliceLength_B,CsmaSliceLength_C);
    }
    else
    {
        CsmaSliceLength_A = *CsmaSliceLength/3;
        CsmaSliceLength_B = CsmaSliceLength_A;
        CsmaSliceLength_C = *CsmaSliceLength-CsmaSliceLength_A-CsmaSliceLength_B;
    }


    net_printf("CsmaSliceLengt=%d,A=%d,B=%d,C=%d\n",*CsmaSliceLength,CsmaSliceLength_A,CsmaSliceLength_B,CsmaSliceLength_C);

    pCsmaSlotInfo = (CSMA_SLOT_INFO_t *)((U8*)pSlotAllocation+pSlotAllocation->EntryLength);
    for(i = 0; i < nnib.CSMASlotPhaseCount; i++)
    {

        pCsmaSlotInfo->Phase = i + 1;
        switch(pCsmaSlotInfo->Phase)
        {
            case e_A_PHASE:
                #if defined(STD_2016)
                pCsmaSlotInfo->CsmaSlotLength = CsmaSliceLength_A;
                #elif defined(STD_GD)
                pCsmaSlotInfo->CsmaSlotLength = CsmaSliceLength_A*10;
                #endif

            break;
            case e_B_PHASE:
                #if defined(STD_2016)
                pCsmaSlotInfo->CsmaSlotLength = CsmaSliceLength_B;
                #elif defined(STD_GD)
                pCsmaSlotInfo->CsmaSlotLength = CsmaSliceLength_B*10;
                #endif

            break;
            case e_C_PHASE:
                #if defined(STD_2016)
                pCsmaSlotInfo->CsmaSlotLength = CsmaSliceLength_C;
                #elif defined(STD_GD)
                pCsmaSlotInfo->CsmaSlotLength = CsmaSliceLength_C*10;
                #endif

            break;
        }
        pSlotAllocation->EntryLength += 4;
        pCsmaSlotInfo++;
    }

    //绑定CSMA时隙信息
    for(i = 0; i < nnib.BindCSMASlotPhaseCount; i++)
    {
        pCsmaSlotInfo->CsmaSlotLength = BandCsmaSliceLength ;
        pCsmaSlotInfo->Phase = i + 1;
        pSlotAllocation->EntryLength += 4;
    }


    pSlotAllocation->BeaconPeriodStartTime = PHY_US2TICK(1500)+get_phy_tick_time();

    if(HPLC.next_beacon_start)
    {
        pSlotAllocation->BeaconPeriodStartTime = HPLC.next_beacon_start;
    }
#else
    U8								i;
    NON_CENTRAL_BEACON_INFO_t		*pNonCenterBeacon;
    CSMA_SLOT_INFO_t				*pCsmaSlotInfo;

    pSlotAllocation->BeaconEntryType = e_SLOT_ALLOCATION_TYPE;            // 时隙分配
    pSlotAllocation->EntryLength = sizeof(SLOT_ALLOCATION_STRUCT_t);      // 时隙长度
    pSlotAllocation->CentralSlotCount = g_TimeSlotParam.CCObcn_slot_nr;   // A\B\C中央信标时隙总数
    pSlotAllocation->CsmaSlotPhaseCount = nnib.CSMASlotPhaseCount;        // CSMA时隙支持的相线个数
    pSlotAllocation->ProxySlotCount = nnib.PCOCount;                      // 代理信标时隙个数
    pSlotAllocation->BeaconSlotTime = g_TimeSlotParam.beacon_slot;        // 信标时隙长度
    pSlotAllocation->CsmaSlotSlice = g_TimeSlotParam.csma_time_slot / 10; // csma时隙分片长度
    pSlotAllocation->BindCsmaSlotPhaseCout = 0;                           // 绑定CSMA时隙相线个数
    pSlotAllocation->BindCsmaSlotLinkId = 0;                              // 绑定CSMA时隙链路标识符
    pSlotAllocation->TdmaSlotLength = 0;                                  // TDMA时隙长度
    pSlotAllocation->TdmaSlotLinkId = 0;                                  // TDMA时隙链路标识符

#ifdef HPLC_HRF_PLATFORM_TEST
    //无线信标时隙长度
    //pSlotAllocation->RfBeaconLength = g_TimeSlotParam.beacon_slot;//g_TimeSlotParam.beacon_slot*(pSlotAllocation->CentralSlotCount + g_TimeSlotParam.NonCenterCnt);
#endif
    pSlotAllocation->Reserve = 0;
    // 非中央时隙安排
    pNonCenterBeacon = (NON_CENTRAL_BEACON_INFO_t *)pSlotAllocation->info;
    // 如果存在非中央信标时隙
    pSlotAllocation->NonCentralBeaconSlotCount = 0;

    //    net_printf("g_TimeSlotParam.NonCenterCnt = %d, g_TimeSlotParam.pNonCenterInfo = %p\n"
    //               , g_TimeSlotParam.NonCenterCnt
    //               , g_TimeSlotParam.pNonCenterInfo);
    //时隙分为，中央信标时隙，非中央信标时隙（代理信标时隙，发现信标时隙），CSMA时隙，绑定CSMA时隙
    if ((g_TimeSlotParam.NonCenterCnt > 0) && (g_TimeSlotParam.pNonCenterInfo != NULL))
    {
        pSlotAllocation->NonCentralBeaconSlotCount = g_TimeSlotParam.NonCenterCnt;
        __memcpy(pNonCenterBeacon, g_TimeSlotParam.pNonCenterInfo, g_TimeSlotParam.NonCenterCnt * sizeof(NON_CENTRAL_BEACON_INFO_t));
    }

    //dump_buf((U8*)pNonCenterBeacon,pSlotAllocation->NonCentralBeaconSlotCount*2);
    pSlotAllocation->EntryLength += pSlotAllocation->NonCentralBeaconSlotCount*2;

    //中央信标个数增加，重新计算信标长度。
    pSlotAllocation->BeaconPeriodLength = nnib.BeaconPeriodLength + (pSlotAllocation->CentralSlotCount -3) * pSlotAllocation->BeaconSlotTime;


    //CSMA时隙
    pCsmaSlotInfo = (CSMA_SLOT_INFO_t *)((U8*)pSlotAllocation+pSlotAllocation->EntryLength);
    for(i = 0; i < nnib.CSMASlotPhaseCount; i++)
    {

        pCsmaSlotInfo->Phase = i + 1;
        switch(pCsmaSlotInfo->Phase)
        {
            case e_A_PHASE:
                #if defined(STD_2016)
                pCsmaSlotInfo->CsmaSlotLength = g_TimeSlotParam.csma_slot_a;
                #elif defined(STD_GD)
                pCsmaSlotInfo->CsmaSlotLength = g_TimeSlotParam.csma_slot_a * 10;
                #endif

            break;
            case e_B_PHASE:
                #if defined(STD_2016)
                pCsmaSlotInfo->CsmaSlotLength = g_TimeSlotParam.csma_slot_b;
                #elif defined(STD_GD)
                pCsmaSlotInfo->CsmaSlotLength = g_TimeSlotParam.csma_slot_b * 10;
                #endif

            break;
            case e_C_PHASE:
                #if defined(STD_2016)
                pCsmaSlotInfo->CsmaSlotLength = g_TimeSlotParam.csma_slot_c;
                #elif defined(STD_GD)
                pCsmaSlotInfo->CsmaSlotLength = g_TimeSlotParam.csma_slot_c * 10;
                #endif

            break;
        }
        pSlotAllocation->EntryLength += 4;
        pCsmaSlotInfo++;
    }


    //绑定CSMA时隙信息
    for(i = 0; i < nnib.BindCSMASlotPhaseCount; i++)
    {
        pCsmaSlotInfo->CsmaSlotLength = BandCsmaSliceLength ;
        pCsmaSlotInfo->Phase = i + 1;
        pSlotAllocation->EntryLength += 4;
    }


    pSlotAllocation->BeaconPeriodStartTime = g_TimeSlotParam.beacon_start;

#endif

}

uint16_t PackCenternBeaconSimp(uint8_t *payload, U8 phase)
{
    U8								*pBeaconTxBuffer;
    RF_BEACON_HEADER_t              *pBeaconHeader_t;
    U16 							BeaconMMdLength = 0;
    RF_ABILITY_SLOT_t               *pStaAbility;

    U32 CsmaSlotStart;
    U16 CsmaSlotDur  ;

    pBeaconTxBuffer= zc_malloc_mem(512,"pBTBuf_s",MEM_TIME_OUT);
    pBeaconHeader_t = (RF_BEACON_HEADER_t *)pBeaconTxBuffer;

    pBeaconHeader_t->beaconType = e_CCO_BEACON;
    pBeaconHeader_t->Networkflag = nnib.Networkflag;
    pBeaconHeader_t->Relationshipflag =g_RelationSwitch;// TRUE;
    pBeaconHeader_t->Networkseq = nnib.FormationSeqNumber;
    pBeaconHeader_t->UseBeaconflag = nnib.BeaconLQIFlag;//TRUE;
    pBeaconHeader_t->BeaconPeriodCount = NwkBeaconPeriodCount;
    __memcpy(pBeaconHeader_t->CCOmacaddr, nnib.CCOMacAddr, MACADRDR_LENGTH);
    pBeaconHeader_t->Relationshipflag =g_RelationSwitch;// 再次赋值，有可能在BeaconSlotSchedule改变此值
    pBeaconHeader_t->Networkflag = nnib.Networkflag;//同上

    pBeaconHeader_t->SimpBeaconFlag = e_SIMP_BEACON;

    //精简信标帧载荷中仅定义了精简信标站点信息及时隙条目
    pBeaconHeader_t->BeaconEntryCount = 1;

    pStaAbility = (RF_ABILITY_SLOT_t *)(pBeaconTxBuffer + sizeof(RF_BEACON_HEADER_t));

    CsmaSlotStart = g_TimeSlotParam.beacon_start + PHY_MS2TICK(g_TimeSlotParam.beacon_slot_nr * g_TimeSlotParam.beacon_slot);
    CsmaSlotDur = g_TimeSlotParam.csma_slot_a + g_TimeSlotParam.csma_slot_b + g_TimeSlotParam.csma_slot_c;
    setRfAbilityEntry(pStaAbility, CsmaSlotStart, CsmaSlotDur);

    BeaconMMdLength += sizeof(RF_ABILITY_SLOT_t);


    //TODO
    __memcpy(payload, pBeaconTxBuffer, BeaconMMdLength + sizeof(RF_BEACON_HEADER_t));
    zc_free_mem(pBeaconTxBuffer);
    return BeaconMMdLength + sizeof(RF_BEACON_HEADER_t);
}

//中央信标，	定时器驱动，发消息到本任务中执行。
uint16_t PackCenternBeacon(uint8_t *payload, U8 phase)
{
    U8								*pBeaconMMd,*pBeaconTxBuffer;

    U16 							BeaconMMdLength = 0;
    U32 							CsmaSliceLength;
    STA_ABILITY_STRUCT_t			*pStaAbility;
    SLOT_ALLOCATION_STRUCT_t		*pSlotAllocation;

    ROUTE_PARM_STRUCT_t 			*pRouteParm;
    BAND_CHANGE_STRUCT_t			*pBandChange;
    POWERSET_STRUCT_t				*pPowerSet;
    LINKMODESET_STRUCT_t			*pLinkModeSet;
    BEACON_HEADER_t 				*pBeaconHeader_t;
    
#ifdef HPLC_HRF_PLATFORM_TEST
    RF_ROUTE_PARM_STRUCT_t          *pRfRouteParam;
    RF_CHANNEL_CHANGE_STRUCT_t      *pRfChannelSet;
#endif

    pBeaconTxBuffer= zc_malloc_mem(512,"pBTBuf_n",MEM_TIME_OUT);
    pBeaconHeader_t = (BEACON_HEADER_t *)pBeaconTxBuffer;
    pBeaconMMd		=  &pBeaconTxBuffer[sizeof(BEACON_HEADER_t)];

    pBeaconHeader_t->beaconType = e_CCO_BEACON; // 中央信标
    // pBeaconHeader_t->Networkflag = nnib.Networkflag;    //组网完成标志
    // pBeaconHeader_t->Relationshipflag =g_RelationSwitch;// 关联标志;
    pBeaconHeader_t->Networkseq = nnib.FormationSeqNumber;                   // 组网序号
    pBeaconHeader_t->UseBeaconflag = nnib.BeaconLQIFlag;                     // 信标使用标志;
    pBeaconHeader_t->BeaconPeriodCount = NwkBeaconPeriodCount;               // 信标周期计数
    __memcpy(pBeaconHeader_t->CCOmacaddr, nnib.CCOMacAddr, MACADRDR_LENGTH); // CCO地址
    pBeaconHeader_t->Relationshipflag = g_RelationSwitch;                    // 再次赋值，有可能在BeaconSlotSchedule改变此值
    pBeaconHeader_t->Networkflag = nnib.Networkflag;                         // 组网完成标志
    pBeaconHeader_t->SimpBeaconFlag = e_STRAD_BEACON; // 精简信标标志
#ifdef HPLC_HRF_PLATFORM_TEST
    pBeaconHeader_t->RfChannel = getHplcRfChannel(); // WPHY_CHANNEL;//wphy_hz_to_ch(wphy_get_option(), wphy_get_fckhz() * 1000);//wphy_get_channel();
    pBeaconHeader_t->RfOption = getHplcOptin();
#endif

    nnib.SendDiscoverListTime = nnib.RoutePeriodTime/10;
    nnib.SendDiscoverListTime = (nnib.SendDiscoverListTime == 0 ? (BEACONPERIODLENGTH * 5): nnib.SendDiscoverListTime);
    nnib.RfDiscoverPeriod   = MAX(10, nnib.RoutePeriodTime/5);      //每个路由周期发送5帧无线发现列表。
    nnib.RfRcvRateOldPeriod = nnib.RoutePeriodTime/nnib.RfDiscoverPeriod;//4;

    pBeaconHeader_t->BeaconEntryCount = 0;//信标条目总数

    //设置站点能力
    pStaAbility = (STA_ABILITY_STRUCT_t *)&pBeaconMMd[BeaconMMdLength];
    SetStaAbilityEntry(pStaAbility);
    pStaAbility->Phase = phase;
    BeaconMMdLength += sizeof(STA_ABILITY_STRUCT_t);
    pBeaconHeader_t->BeaconEntryCount++;

    //设置路由参数条目
    pRouteParm = (ROUTE_PARM_STRUCT_t *)&pBeaconMMd[BeaconMMdLength];
    BeaconMMdLength += sizeof(ROUTE_PARM_STRUCT_t);
    SetRouteParmEntry(pRouteParm);
    pBeaconHeader_t->BeaconEntryCount++;
    
#ifdef HPLC_HRF_PLATFORM_TEST
    //无线路由参数
    pRfRouteParam = (RF_ROUTE_PARM_STRUCT_t *)&pBeaconMMd[BeaconMMdLength];
    BeaconMMdLength += sizeof(RF_ROUTE_PARM_STRUCT_t);
    SetRfRouteParam(pRfRouteParam);
    pBeaconHeader_t->BeaconEntryCount++;

    if(TRUE == nnib.RfChannelChangeState)
    {
        if(nnib.RfChgChannelRemainTime == 0)
        {
//            HPLC.option = DefSetInfo.RfChannelInfo.option;
//            HPLC.rfchannel = DefSetInfo.RfChannelInfo.channel;
//            net_printf("set rfchannel:<%d,%d>\n",HPLC.option, HPLC.rfchannel);

//            nnib.RfChannelChangeState = 0;
//            DefwriteFg.RfChannel = TRUE;
//            DefaultSettingFlag = TRUE;

//            DstCoordRfChannel = 0xff;
        }
        else
        {
            net_printf("RF_CHANNEL_CHANGE : <%d,%d>\n",DefSetInfo.RfChannelInfo.option, DefSetInfo.RfChannelInfo.channel);
            pRfChannelSet = (RF_CHANNEL_CHANGE_STRUCT_t *) &pBeaconMMd[BeaconMMdLength];
            SetRfChnChangeEntry(pRfChannelSet);
            BeaconMMdLength += sizeof(RF_CHANNEL_CHANGE_STRUCT_t);
            pBeaconHeader_t->BeaconEntryCount++;
        }
    }
#endif

    //设置频段变更通知条目
    if(TRUE == nnib.BandChangeState)
    {

        if(nnib.BandRemianTime ==0)
        {
//            DefwriteFg.FREQ = TRUE;
//            DefaultSettingFlag = TRUE;
//            changeband(DefSetInfo.FreqInfo.FreqBand);
//            nnib.BandChangeState =0;
        }
        else
        {
            pBandChange = (BAND_CHANGE_STRUCT_t *) &pBeaconMMd[BeaconMMdLength];
            SetbandChangeEntry(pBandChange);
            BeaconMMdLength += sizeof(BAND_CHANGE_STRUCT_t);
            pBeaconHeader_t->BeaconEntryCount++;
        }
    }

    //设置功率
    if(TRUE == nnib.powerlevelSet)
    {
        pPowerSet = (POWERSET_STRUCT_t *) &pBeaconMMd[BeaconMMdLength];
        SetpowerEntry(pPowerSet);
        BeaconMMdLength += sizeof(POWERSET_STRUCT_t);
        pBeaconHeader_t->BeaconEntryCount++;
    }
	//设置链路工作模式
    if(TRUE == nnib.LinkModeStart)
    {
        //if(nnib.ChangeTime == 0)
        {
            
        }
        //else
        {
            pLinkModeSet = (LINKMODESET_STRUCT_t *) &pBeaconMMd[BeaconMMdLength];
            set_linkmode_entry(pLinkModeSet);
            BeaconMMdLength += sizeof(LINKMODESET_STRUCT_t);
            pBeaconHeader_t->BeaconEntryCount++;
        }
    }

    //时隙设置条目
    pSlotAllocation = (SLOT_ALLOCATION_STRUCT_t *) &pBeaconMMd[BeaconMMdLength];
    SetsoltEntry(pSlotAllocation,&CsmaSliceLength);
    BeaconMMdLength += pSlotAllocation->EntryLength;
    pBeaconHeader_t->BeaconEntryCount++;

//    NwkBeaconPeriodCount++;
//    nnib.SendDiscoverListCount +=1;//发送列表的次数包含信标,是否应该在CONFIRM中++

    //TODO
    __memcpy(payload, pBeaconTxBuffer, BeaconMMdLength + sizeof(BEACON_HEADER_t));
    zc_free_mem(pBeaconTxBuffer);
    //printf_s("bec_h:%d\n",(BeaconMMdLength + sizeof(BEACON_HEADER_t)));
    return BeaconMMdLength + sizeof(BEACON_HEADER_t);
}



void setCsmaSlotForCoord(U32 *CsmaSliceLength_A, U32 *CsmaSliceLength_C, U32 *CsmaSliceLength_B, U32 CsmaSliceLength)
{

    U8 i, j;
    U8 cnt1 = 0;
    U8 coordCnt;

        coordCnt = (nnib.BeaconPeriodLength / 1000) + ((nnib.BeaconPeriodLength % 1000) ? 1 : 0);

        U32 *sort[3];
        sort[0] = CsmaSliceLength_A;
        sort[1] = CsmaSliceLength_B;
        sort[2] = CsmaSliceLength_C;

//        net_printf("init : A=%d, B=%d, C=%d, coordCnt = %d, PeriodLength = %d\n"
//                   ,*sort[0], *sort[1], *sort[2], coordCnt, nnib.BeaconPeriodLength);

        //排序
        for(i = 0; i < 2; i++)
        {
            for(j = i + 1; j < 3; j++)
            {
                if(*sort[i] > *sort[j])
                {
                    U32 *tmp = sort[i];
                    sort[i] = sort[j];
                    sort[j] = tmp;
                }
            }
        }

//        net_printf("sort : %d, %d, %d, cnt = %d\n"
//                   ,*sort[0], *sort[1], *sort[2], (*sort[0] / 100) * 3);

        if((*sort[0] / 100) * 3 < coordCnt) //如果发送的数量不够
        {
            cnt1 = coordCnt / 3;
            *sort[0] = cnt1 * 100;
            if(*sort[1] <= *sort[0])
            {
//                *sort[1] = *sort[0];
                *sort[1] = *sort[0] + (coordCnt % 3 ? 100 : 0);
            }
            *sort[2] = CsmaSliceLength - *sort[0] - *sort[1];
        }


}

void getSlotInfoForBeacon(TIME_SLOT_PARAM_t *pTimeSlotParam)
{
    U16 							A_Num=0,B_Num=0,C_Num=0,LevelA_B_C=0;
    U32 							CsmaSliceLength_A,CsmaSliceLength_B,CsmaSliceLength_C;

    //需要在时隙条目组帧的时候释放

    // U32 beacon_start;
    //U16 beacon_slot;
    //U16 beacon_slot_nr;
    //
    //U16 csma_slot_a;
    //U16 csma_slot_b;
    //U16 csma_slot_c;
    //U8  csma_time_slot;
    //
    //U8 *pNonCenterInfo;

    //初始化

    g_BeaconSlotTime = GetBeaconTimeSlot();



    pTimeSlotParam->CCObcn_slot_nr = pTimeSlotParam->beacon_slot_nr = CenterSlotCount;//A\B\C
    pTimeSlotParam->beacon_slot = g_BeaconSlotTime ;//
    pTimeSlotParam->csma_time_slot	= 10 * 10;
    //非中央时隙安排
    pTimeSlotParam->NonCenterCnt = BeaconSlotSchedule(&BeaconSlotSchedule_t, pTimeSlotParam);
    pTimeSlotParam->beacon_slot_nr += pTimeSlotParam->NonCenterCnt;

//    net_printf("pTimeSlotParam->NonCenterCnt = %d\n", pTimeSlotParam->NonCenterCnt);
//        net_printf("pTimeSlotParam->RfBeaconType = %d\n", pTimeSlotParam->RfBeaconType);

    //CSMA时隙信息,计算一跳点A\B\C比例
    LevelA_B_C = GetPhaseParm(&A_Num,&B_Num,&C_Num);
    //CSMA总时间
    U32 CsmaSliceLength= nnib.BeaconPeriodLength -(pTimeSlotParam->beacon_slot_nr) * g_BeaconSlotTime;

   net_printf("ANum=%dB=%dC=%dE=%D\n",A_Num,B_Num,C_Num,LevelA_B_C);
    if(LevelA_B_C>0)
    {
        CsmaSliceLength_A = 400 + (A_Num* (CsmaSliceLength-MINCSMASLICE) )/LevelA_B_C;
        CsmaSliceLength_B = 400 + (B_Num* (CsmaSliceLength-MINCSMASLICE) )/LevelA_B_C;
        CsmaSliceLength_C = CsmaSliceLength-CsmaSliceLength_A-CsmaSliceLength_B;
//        net_printf("CsmaSliceLengt=%d,A=%d,B=%d,C=%d\n",CsmaSliceLength,CsmaSliceLength_A,CsmaSliceLength_B,CsmaSliceLength_C);
    }
    else
    {
        CsmaSliceLength_A = CsmaSliceLength/3;
        CsmaSliceLength_B = CsmaSliceLength_A;
        CsmaSliceLength_C = CsmaSliceLength-CsmaSliceLength_A-CsmaSliceLength_B;
    }
    //为网间协调加入的修改
    setCsmaSlotForCoord(&CsmaSliceLength_A, &CsmaSliceLength_C, &CsmaSliceLength_B, CsmaSliceLength);

    pTimeSlotParam->csma_slot_a = CsmaSliceLength_A;
    pTimeSlotParam->csma_slot_b = CsmaSliceLength_B;
    pTimeSlotParam->csma_slot_c = CsmaSliceLength_C;

    //net_printf("CsmaSliceLengt=%d ,A=%d, B=%d, C=%d,per=%d\n",CsmaSliceLength,CsmaSliceLength_A,CsmaSliceLength_B,CsmaSliceLength_C,nnib.BeaconPeriodLength);


    /*pTimeSlotParam->beacon_start = PHY_US2TICK(1500)+get_phy_tick_time();
    if(pTimeSlotParam->beacon_start < HPLC.next_beacon_start)
    {
        pTimeSlotParam->beacon_start = HPLC.next_beacon_start;
    }*/
    /*net_printf("last beacon end time : %d ms, next beacon start time  %d ms , back off %d ms\n"
             , PHY_TICK2MS(HPLC.next_beacon_start)
             , PHY_TICK2MS(HPLC.next_beacon_start + BeaconStopOffset)
             , (BeaconStopOffset));*/
    pTimeSlotParam->beacon_start = HPLC.next_beacon_start + PHY_MS2TICK(BeaconStopOffset);
    BeaconStopOffset = 0;
}
#endif
