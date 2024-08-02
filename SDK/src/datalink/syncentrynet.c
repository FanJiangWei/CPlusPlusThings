
#include "SyncEntrynet.h"
#include "framectrl.h"
#include "phy_plc_isr.h"
#include "DataLinkGlobal.h"
#include "printf_zc.h"
#include "dl_mgm_msg.h"
#include "DatalinkTask.h"
#include "Scanbandmange.h"
#include "datalinkdebug.h"

#include "ScanRfCHmanage.h"


#if defined(STATIC_NODE)

#define ESTIMATE_TIME								(5*1000)   //评估时间
#define WAITCFM_TIME								4000//500//5   //等待关联回复超时时间
#define ENTER_MAXTIME                               (30*1000)//从允许入网到设置时隙失败最大超时时间
#define MAX_ENTER_MAXTIME                           (75*1000)//从允许入网到设置时隙失败最大超时时间





ostimer_t *network_timer=NULL;

extern U8 CheckBlackNegibor(U32 SNID);







void StartSTAnetwork(U32 delaytime,U8 formationseq)
{
    net_printf("StartSTAnetwork : workState = %d,delaytime=%d\n"
               , GetWorkState(),delaytime);
    if(GetWorkState() == e_BEACONSYNC )
	{
		nnib.FormationSeqNumber = formationseq;
		SetWorkState(e_ENTER);
        modify_network_timer(delaytime);
	}

}
/**
 * @brief           收到信标后设置为同步状态
 * 
 * @param snid      接收到信标的网络号
 */
void StartSTABcnSync(U32 snid)
{
    // net_printf("StartSTABcnSync : workState = %d,snid:%08x\n"
    //            , GetWorkState(),snid);
    if(GetWorkState() == e_WAITBEACON && snid == GetSNID())
	{
		SetWorkState(e_BEACONSYNC);
        modify_network_timer(MAX_ENTER_MAXTIME);
	}

}







void StartsyncTEI(U32 SNID)
{
    // net_printf("StartsyncTEI : workState = %d,testmode:%d\n"
    //            , GetWorkState(),getHplcTestMode());
    if(GetWorkState() == e_LISEN && getHplcTestMode() == NORM)
	{
		if(CheckBlackNegibor(SNID)==FALSE) //黑名单中网络不启动同步
		{
            net_printf("net in black\n"); //
			return;
		}
        if(zc_timer_query(network_timer) != TMR_RUNNING) //管理定时是否运行
		{
           net_printf("Trigger SNID=0x%08x\n",SNID);
		   modify_network_timer(0);
		}
		
	}
}


void StopThisEntryNet()
{
    if(zc_timer_query(network_timer) == TMR_RUNNING)
        timer_stop(network_timer, TMR_NULL);
}

static U32 SelectBestNetAssoc()
{

    S8  gain =RF_RSSI_INVAL;
	U32 snid=0xffffffff;

	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    Sta_Show_NbNet();

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if( (!CheckBlackNegibor(DiscvryTable->SNID)) || DiscvryTable->NodeDepth >= 15)
        {
            continue;
        }

        if(!CheckNbNetHRfBaud(DiscvryTable->SNID))
        {
            continue;
        }

        if(DiscvryTable->HrfInfo.DownRssi > gain)
        {
            gain = DiscvryTable->HrfInfo.DownRssi;
            snid = DiscvryTable->SNID;

            SetSnidLkTp(e_RF_Link);

        }
        /*net_printf("SelectBestNetAssoc,NodeTEI=%02x,gain=%d,%d,%d,snid=%08x\n",NeighborDiscoveryTableEntry[i].NodeTEI,
                                                                        gain,NeighborDiscoveryTableEntry[i].GAIN,
                                                                        NeighborDiscoveryTableEntry[i].RecvCount,snid);*/
    }

    net_printf("gain:%d\n", gain);
    gain = abs(gain);
    net_printf("abs(gain):%d\n", gain);
    if(snid == 0xffffffff)
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if( (!CheckBlackNegibor(DiscvryTable->SNID)) || DiscvryTable->NodeDepth >= 15)
            {
                continue;
            }

            if(!CheckNbNetHPLCBaud(DiscvryTable->SNID))
            {
                continue;
            }

            if(gain > (DiscvryTable->HplcInfo.GAIN/DiscvryTable->HplcInfo.RecvCount))
            {
                gain = (DiscvryTable->HplcInfo.GAIN/DiscvryTable->HplcInfo.RecvCount);
                snid = DiscvryTable->SNID;
                SetSnidLkTp(e_HPLC_Link);

            }
            /*net_printf("SelectBestNetAssoc,NodeTEI=%02x,gain=%d,%d,%d,snid=%08x\n",NeighborDiscoveryTableEntry[i].NodeTEI,
                                                                            gain,NeighborDiscoveryTableEntry[i].GAIN,
                                                                            NeighborDiscoveryTableEntry[i].RecvCount,snid);*/
        }

    }


    
	net_printf("best snid =%08x\n",snid);
	return snid;
		
}


static U8 getNeiNetInfo(U32 snid, STA_NEIGHBOR_NET_t *pNetInfo)
{
    U16 i ;
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(StaNeigborNet_t[i].NbSNID == snid)
        {
            if(pNetInfo)
            {
               __memcpy(pNetInfo, &StaNeigborNet_t[i], sizeof(STA_NEIGHBOR_NET_t));
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}




//static 
void update_network_state(work_t *work)
{
	U32 bestsnid;
    net_printf("update_network_state : GetWorkState = %s \n", GetWorkState()==e_LISEN?"e_LISEN":
                                                              GetWorkState()==e_ESTIMATE?"e_ESTIMATE":
                                                              GetWorkState()==e_WAITBEACON?"e_WAITBEACONFAIL":
                                                              GetWorkState()==e_BEACONSYNC?"e_BEACONSYNC":
                                                              GetWorkState()==e_ENTER?"e_ENTER":"UK");

    STA_NEIGHBOR_NET_t staNeiNetInfo;

    switch (GetWorkState())
    {

    case e_LISEN://侦听阶段用于评估选择哪个网络进行同步
        //设置SNID
        SetWorkState(e_ESTIMATE);
        modify_network_timer(ESTIMATE_TIME);//等待超时
        //侦听阶段重新评估
        memset(&gBcnRgainInfo, 0, sizeof(BCN_RGAIN_ASSESS_t));
        break;
    case e_ESTIMATE: //评估完成,开始同步,此时需要选择信号增益最强的网络
        //设置SNID
        
        SetWorkState(e_WAITBEACON);
        modify_network_timer(ENTER_MAXTIME);//等待超时，若超时表示在完成评估后没有收到最佳网络的信标

        bestsnid = SelectBestNetAssoc();
        SetPib(bestsnid,0);
        SetProxy(0xfff); //入网同步阶段TEI采用FFF
        InitHistoryPlanRecord();

        //等待同步
        HPLC.sfo_not_first = 0;

        //载波主导，评估阶段强制停止载波扫频，停留600秒来尝试入网。
        modify_scanband_timer(MAX_STAY_TIME*1000);

        //当前频段没有可用网络，直接切换频段
        if(bestsnid == 0xffffffff)
        {
            ScanBandCallBackNow();
            RfScanCallBackNow();
            SetWorkState(e_LISEN);
            StopThisEntryNet();
            SetPib(0,0);
            return;
        }

        net_printf("SnidLkTp:%d\n", GetSnidLkTp());
        if(getNeiNetInfo(bestsnid, &staNeiNetInfo) && getHplcTestMode() == NORM && HPLC.scanFlag)  //获取当前选择邻居网络信息，切换无线信道
        {
            net_printf("HRF:cur=<%d,%d>, net=<%d,%d>, HPLC:cur=%d,net=%d\n"
                       , getHplcOptin(), getHplcRfChannel()
                       , staNeiNetInfo.option, staNeiNetInfo.RfChannel
                       , GetHplcBand(), staNeiNetInfo.bandidx);
//            if(GetSnidLkTp() == e_HPLC_Link)
            {
                if((staNeiNetInfo.RfChannel != 0xfff) && (staNeiNetInfo.option != 0x0f))
                {
                    //设置无线信道到目标网络, 此处切换无线信道是为了能获取目标网络无线邻居节点信息。
                    changRfChannel(staNeiNetInfo.option, staNeiNetInfo.RfChannel);
                    //选择目标网络后，停止无线扫频。载波扫频在自身流程里决定是否停止。 在入网失败后重新启动扫频
//                    rfScanStayCurCh();
                    StopScanRfTimer();
                }

                //载波频段需要切换到网络对应频段。应该不存在这种情况。
                if(staNeiNetInfo.bandidx != 0xff)
                {
                    changeband(staNeiNetInfo.bandidx);
                }
            }
//            else
//            { //通过无线链路选择得网路，如果当前频段和网络频段不一致。不在当前频段停留
//                if(staNeiNetInfo.bandidx != GetHplcBand())
//                    ScanBandCallBackNow();
//            }
        }

        break;
    case e_WAITBEACON:
    case e_BEACONSYNC:
        //添加黑名单，重新
        AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink);
        break;
    case e_ENTER: //找最佳网络节点入网

        if(GetNodeState() == e_NODE_ON_LINE) //入网后停止此流程
        {
        	net_printf("stop updata_network timer\n");
            return;
        }
        if(GetScanTiemrLeftTime() > WAITCFM_TIME || HPLC.scanFlag ==FALSE)   //需要判断切换频段的剩余时间是否足够
        {
            if(SendAssociateReq(0,g_ReqReason))
            {
                modify_network_timer(WAITCFM_TIME); //等待关联回复
            }
            else
            {
//                GetPLanAinfo()==0 ?
//                AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink)://加黑名单，第一个方案评估完后，仍没有合适的邻居
//                modify_network_timer(WAITCFM_TIME); //等待关联回复

                AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink);
            }
        }
        else
        {
            //如果是距离下一次切换band的时间不够一次入网请求，是否需要将网络加入到黑名单中？
            net_printf("%s: ChangBand left time is Not enough\n", __func__);
            AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink);
            // SetWorkState(e_LISEN);
            // ScanBandCallBackNow();
            // RfScanCallBackNow();
        }
        break;
    }
	
}

void network_timerCB( struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"NTWK",MEM_TIME_OUT);
	work->work = update_network_state;
	work->msgtype=NETKS;
	post_datalink_task_work(work);	
}

void modify_network_timer(uint32_t ms)
{
    if(network_timer == NULL)
	{
        network_timer = timer_create(
                   100,
                   100,
                   TMR_TRIGGER,//TMR_TRIGGER
                   network_timerCB,
                   NULL,
                   "network_timerCB"
                   );
	}
    if(ms == 0) //0直接进入回调
	{
       if(zc_timer_query(network_timer) ==TMR_STOPPED)
       {
           timer_start(network_timer);
       }
       timer_stop(network_timer, TMR_CALLBACK);
	   
	}
	else
	{
        timer_modify(network_timer,
                ms,
                ms,
				TMR_TRIGGER ,//TMR_TRIGGER
				network_timerCB,
				NULL,
                "network_timerCB",
                TRUE);
        timer_start(network_timer);
	}
	
}
/**
 * @brief       准备切换下一个网络，设置同步状态
 * 
 */
void Change_next_net_option()
{
    SetPib(0,0); //准备切换下一个网络			
    SetProxy(0xfff);
    SetWorkState(e_LISEN);
    StopThisEntryNet();
    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
    HPLC.sfo_is_done=0;

    //拉黑当前网络时，停止信道变更定时器。
    if(timer_query(&g_ChangeChlTimer) == TMR_RUNNING)
    {
        timer_stop(&g_ChangeChlTimer, TMR_NULL);
    }
    
    //切换网络时，需要恢复扫频信道
    if(HPLC.scanFlag)
    {
        set_rfscan_channel();
    }
}

#endif




