#include "DataLinkGlobal.h"
#include "phy.h"
#include "dl_mgm_msg.h"
#include "string.h"
#include "printf_zc.h"
#include  "Beacon.h"
#include "DataLinkInterface.h"
#include "app_common.h"
#include "dev_manager.h"
//#include "cjqTask.h"

#include "SyncEntrynet.h"
#include "DatalinkTask.h"

#include "plc.h"
#include "Datalinkdebug.h"
#include "app_rdctrl.h"

#include "app_gw3762.h"
#include "app_gw3762_ex.h"
#include "ZCsystem.h"
#include "Scanbandmange.h"
#include "SchTask.h"
#include "app_dltpro.h"
#include "app_off_grid.h"
#include "app_area_indentification_common.h"
#include "app_data_freeze_cco.h"
#include "app_read_cjq_list_sta.h"


//extern phy_demo_t PHY_DEMO;

#if defined(STATIC_MASTER)
DEVICE_TEI_LIST_t DeviceTEIList[MAX_WHITELIST_NUM];
WHITE_MAC_LIST_t 						WhiteMacAddrList[MAX_WHITELIST_NUM];
WHITE_MAC_CJQ_MAP_LIST_t                WhiteMaCJQMapList[MAX_WHITELIST_NUM];
FLASH_WHITE_LIST_t FlashWhiteList;


NEIGHBOR_NET_t  	NeighborNet[MaxSNIDnum];

#else

U8 g_ReqReason = 0;

STA_NEIGHBOR_NET_t StaNeigborNet_t[MaxSNIDnum];
#endif



#if defined(STATIC_MASTER)
U16   ChildStaIndex[1000]; //遍历子节点时使用
U16   ChildCount = 0;
#endif


//NEIGHBOR_DISCOVERY_TABLE_ENTRY_t NeighborDiscoveryTableEntry[MAX_NEIGHBOR_NUMBER];
DATALINK_TABLE_t	NeighborDiscoveryHeader;
//DATALINK_TABLE_t 	RouteTableHeader;
/*无线邻居表，链表头*/
DATALINK_TABLE_t	RfNeighborDiscoveryHeader;

ROUTE_TABLE_ENTRY_t NwkRoutingTable[NWK_MAX_ROUTING_TABLE_ENTRYS];

//DATALINK_TABLE_t         ROUTE_TABLE_LINK;
ROUTE_TABLE_RECD_t    	 NwkAodvRouteTable[NWK_AODV_ROUTE_TABLE_ENTRYS];
ostimer_t 	g_WaitReplyTimer; //等待路由应答超时
ostimer_t 	g_ReplyTimer;   //路由应答优化超时时间


ostimer_t	*Discoverytimer=NULL;
static void modify_Discoverytimer(uint32_t first);



ROUTING_DISCOVERY_TABLE_ENTRY_t 	RouteDiscoveryTable[MAX_DISCOERY_TABLE_ENTRYS];
ROUTING_DISCOVERY_TABLE_ENTRY_t		RouteDiscoveryEntryOne;

ACCESS_CONTROL_LIST_ROW_t  AccessControlListrow;
U8 AccessListSwitch= FALSE;

NET_MANAGE_PIB_t 	nnib={
	.MacAddrType=e_UNKONWN
}; //网络层属性库; //网络层属性库



void StartMmDiscovery();


U8  LEGAL_CHECK(U32 data,U32 downthr,U32 upthr) 
{
	return  (data>downthr && data<upthr);
}
int8_t datalink_table_add_by_tei_size(DATALINK_TABLE_t * head, list_head_t *new_list,U16 tei)
{ 
    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry;
    list_head_t *p=NULL;
    if(head->nr >= head->thr)
    {
        // net_printf("faild : full!!\n");
        return -1;
    }

    /*insert list by tei*/
    list_for_each_safe(pos, node,&head->link)
    {
        TableEntry = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(!p)
        {
            if(time_after_eq(TableEntry->NodeTEI,tei))
            {
                p= &TableEntry->link;
                goto ok;
            }
        }

    }

    if(!p)
        p=&head->link;

ok:

    list_add_tail(new_list, p);
    head->nr++;
    return 0;
}
/**
 * @brief datalink_table_add_by_lkType  根据链路类型。分别按照通信质量对邻居表排序保存
 * @param head                          邻居表头
 * @param new_list                      新增邻居表节点
 * @param lkTp                          邻居表链路类型
 * @return
 */
int8_t  datalink_table_add_by_lkType(DATALINK_TABLE_t * head, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *newEntry,U8 lkTp)
{
    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry;
    list_head_t *p=NULL;
    list_head_t *new_list = &newEntry->link;
    if(head->nr >= head->thr)
    {
        pos = head->link.prev;
        TableEntry = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(lkTp == e_HPLC_Link) //载波邻居表满时需要判断当前要加入的邻居节点通信质量是否比最后一个节点好
        {
            if((newEntry->HplcInfo.GAIN/newEntry->HplcInfo.RecvCount) > (TableEntry->HplcInfo.GAIN/TableEntry->HplcInfo.RecvCount))
            {
                return -1;
            }
            else
            {
                if(datalink_table_del(head, pos, TRUE) < 0) //删除信号质量最差的节点
                {
                    return -1;
                }
                else
                {
                    p=&head->link;      //尾部添加
                    goto ok;
                }
            }
        }
        else if(lkTp == e_RF_Link)
        {
            if(newEntry->HrfInfo.DownRssi < -90)    //信号强度不过门限值不添加
                return -1;

            if(newEntry->HrfInfo.DownSnr < TableEntry->HrfInfo.DownSnr) //SNR小不添加
                return -1;
            else if(newEntry->HrfInfo.DownSnr == TableEntry->HrfInfo.DownSnr
                    && newEntry->HrfInfo.DownRssi < TableEntry->HrfInfo.DownRssi) //SNR相同，信号强度差不添加
                return -1;

            if(datalink_table_del(head, pos, TRUE) < 0) //删除信号质量最差的节点
            {
                return -1;
            }
            else
            {
                p=&head->link;      //尾部添加
                goto ok;
            }
        }
    }

    /*insert list by tei*/
    list_for_each_safe(pos, node,&head->link)
    {
        TableEntry = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(!p)
        {
            if(lkTp == e_HPLC_Link)
            {
                S32 newGain = newEntry->HplcInfo.GAIN/newEntry->HplcInfo.RecvCount;
                S32 nodeGain = TableEntry->HplcInfo.GAIN/TableEntry->HplcInfo.RecvCount;
                if(newGain >= nodeGain)
                {
                    p= &TableEntry->link;
                    goto ok;
                }
            }
            else if(lkTp == e_RF_Link)
            {
                if(newEntry->HrfInfo.DownSnr == TableEntry->HrfInfo.DownSnr)
                {
                    if(newEntry->HrfInfo.DownRssi <= TableEntry->HrfInfo.DownRssi)
                    {
                        p= &TableEntry->link;
                        goto ok;
                    }
                }
                else if(newEntry->HrfInfo.DownSnr < TableEntry->HrfInfo.DownSnr)
                {
                    p= &TableEntry->link;
                    goto ok;
                }
            }
        }

    }

    if(!p)
        p=&head->link;

ok:

    list_add_tail(new_list, p);
    head->nr++;
    return 0;
}


int8_t datalink_table_add(DATALINK_TABLE_t * head, list_head_t *new_list)
{	 
   if(head->nr >= head->thr)	
   {		
	//    net_printf("faild : full!!\n");		  
	   return -1;	 
   }
   list_add_tail(new_list, &head->link);	
   head->nr++;
   return 0;
}

int8_t datalink_table_del(DATALINK_TABLE_t * head, list_head_t *new_list, U8 freeFlag)
{	 
    if(NULL == new_list)
    {
        net_printf("def null\n");		 
        return -1;
    }
    else
    {
		list_del(new_list);
        if(freeFlag)
        {
            zc_free_mem(new_list);
        }
    }

    if(head->nr>0)
    {
        head->nr--;
    	// return 0;
    }
    else
    {
        net_printf("err nr:0\n");		  
	    // return -1;
    }

    return 0;
}

U8 checkAccessControlList(U8* SourceAddr, U8 Status)
{
    unsigned char  i;
    for(i=0; i<MAX_ACL_TABLE_ENTRYS; i++)
    {
        if( (memcmp(AccessControlListrow.AccessControlList[i].MacAddr,SourceAddr,6) ==0) &&
                (AccessControlListrow.AccessControlList[i].PermitOrDeny == Status))
        {
            return TRUE;
        }
    }
    return FALSE;
}
U8 checkAccessControlListByTEI(U16 tei, U32 SNID, U8 Status)
{
    U8 macaddr[6]={0XAA,0XAA,0XAA,0XAA,0XAA,0XAA};
    U8  i;
    SearchMacInNeighborDisEntrybyTEI_SNID(tei , SNID, macaddr);

    for(i=0; i<MAX_ACL_TABLE_ENTRYS; i++)
    {
        if( (memcmp(AccessControlListrow.AccessControlList[i].MacAddr,macaddr,6) ==0) &&
             (AccessControlListrow.AccessControlList[i].PermitOrDeny == Status)
                   )
        {
                    return TRUE;
        }
    }
    return FALSE;

}

/*******************************获取和设置NNIB接口函数***************************/

#if defined(STATIC_MASTER)

//增加组网序号
void IncreaseFromatSeq()
{
	nnib.FormationSeqNumber++;
    sysflag = TRUE;
    SaveInfo(&sysflag, &whitelistflag);

}
#endif

void ClearINIT(void)
{
    nnib.MacAddrType = e_UNKONWN;

    app_printf("ClearINIT DevType=%d\n",nnib.DeviceType);
    
#if  defined(MIZAR1M)|| defined(MIZAR9M) || defined(VENUS2M)|| defined(VENUS8M)
#if defined(STD_DUAL)
    HPLC.worklinkmode = e_link_mode_dual;
    nnib.NetworkType = e_DOUBLE_HPLC_HRF;   //组网方式，默认0:混合组网
#endif
#else 
    HPLC.worklinkmode = e_link_mode_plc;
#endif
    net_printf("InitLinkMode = %d\n", HPLC.worklinkmode);

#if defined(STATIC_NODE)
    ReadAccessInfo();

    nnib.PhasebitA=0;
    nnib.PhasebitB=0;
    nnib.PhasebitC=0;
    nnib.PhaseJuge =0;
#if  defined(MIZAR1M)|| defined(MIZAR9M) || defined(VENUS2M)|| defined(VENUS8M)
#if defined(STD_DUAL)
    nnib.ModuleType = e_HPLC_RF_Module;
#endif
#else
    nnib.ModuleType = e_HPLC_Module;
#endif
    

    ChangeReq_t.Result = TRUE;

    memset(StaNeigborNet_t, 0xff, sizeof(StaNeigborNet_t));
#if defined(ZC3750CJQ2)
    nnib.Resettimes = CollectInfo_st.ResetTime;
    nnib.Resettimes = (nnib.Resettimes+1)%0x0f;
    CollectInfo_st.ResetTime = nnib.Resettimes;
#elif defined(ZC3750STA)
    nnib.Resettimes = DevicePib_t.ResetTime;
    nnib.Resettimes = (nnib.Resettimes+1)%0x0f;
    DevicePib_t.ResetTime = nnib.Resettimes;    
#endif
    staflag =TRUE;
	ApsEventSendPacketSeq = (nnib.Resettimes<<8)+nnib.Resettimes;
    net_printf("---nnib.Resettimes=%d\n",nnib.Resettimes);


    nnib.WorkState = e_LISEN;
	//ScanBandManage(e_INIT);
	#if defined(STD_2016)
    timer_init(&g_ReplyTimer,
               ROUTE_RELAY_TIME,
               0,
               TMR_TRIGGER,
               SendLinkRequest,
               NULL,
               "g_ReplyTimer"
               );
    //AODV应答超时定时器
    timer_init(&g_WaitReplyTimer,
               1000,
               0,
               TMR_TRIGGER,
               WaitReplyCmdOut,
               NULL,
               "g_WaitReplyTimer"
               );



	#endif

#else
	
    memset(NeighborNet, 0xff, sizeof(NeighborNet));
    nnib.BeaconLQIFlag = TRUE;

    nnib.RfConsultEn = 1;                   //无线信道协商标志，默认1：允许信道协商。

    timer_init(&g_ChangeSNIDTimer,
                 1805000,
                 0,
                 TMR_TRIGGER,
                 ChangeSNIDFun,
                 NULL,
                 "g_ChangeSNIDTimer"
                 );

    ChangeRfChannelTimerInit();

 #endif

    NwkBeaconPeriodCount = 0;
    //初始化信标参数全局变量
    memset(&g_TimeSlotParam, 0, sizeof(TIME_SLOT_PARAM_t));
    memset(&ZeroData, 0, sizeof(ZERODATA_t) * PHASENUM);


}
 void ClearNNIB(void)
 {
 #if defined(STATIC_MASTER)

    nnib.NodeType = e_CCO_NODETYPE;
    nnib.NodeLevel = 0;

    nnib.PCOCount = nnib.discoverSTACount = 0;

    /*
    if(getHplcTestMode() == RD_CTRL)
        nnib.StaTEI = CTRL_TEI_GW;
    else
        nnib.StaTEI = CCO_TEI;
    */

    SetProxy(0);
    nnib.PossiblePhase = e_A_PHASE; //

    nnib.NodeState = e_NODE_ON_LINE;
    nnib.SendDiscoverListTime = BEACONPERIODLENGTH * 5; //每个信标周期的CSMA时隙发送一次
    nnib.RoutePeriodTime =RoutePeriodTime_FormationNet;//路由周期初始化

    __memcpy(nnib.MacAddr, FlashInfo_t.ucJZQMacAddr, 6);

    __memcpy(nnib.CCOMacAddr, nnib.MacAddr, 6);
    ChangeMacAddrOrder(nnib.CCOMacAddr);
    ChangeMacAddrOrder(nnib.MacAddr);

    nnib.CSMASlotPhaseCount = CenterSlotCount;
    g_RelationSwitch =TRUE; //默认允许关联

    net_printf("-------------------ClearNNIB---------------\n");
    
	CleanDevListlink();

    memset(&JoinCtrl_t, 0, sizeof(JoinCtrl_t));
    memset(&BeaconSlotSchedule_t, 0, sizeof(BeaconSlotSchedule_t));
    nnib.ProxyNodeUplinkRatio =0;
    nnib.ProxyNodeDownlinkRatio =0;
    nnib.LowestSuccRatio =100;
    nnib.beaconSendTime =0;


    BeaconStopOffset = 0;

#endif

#if defined(STATIC_NODE)
    //U16 ii=0;

    nnib.NodeState = e_NODE_OUT_NET;

    nnib.FormationSeqNumber = 0;
    nnib.FinishSuccRatioCal = 0;
    nnib.NodeLevel = 0;
    nnib.NodeType = e_UNKNOWN_NODETYPE;
    SetProxy(0xfff);
    nnib.ProxyNodeDownlinkRatio = 0;
    nnib.ProxyNodeUplinkRatio = 0;

    nnib.last_My_SED = 0;
    nnib.PossiblePhase = e_A_PHASE; //a
    nnib.BackupPhase1  = 0; //b
    nnib.BackupPhase2  = 0; //c相
    memset(nnib.ParentMacAddr, 0xff, 6);


    /*
    if(getHplcTestMode() == RD_CTRL)
        nnib.StaTEI = CTRL_TEI_GW;
    else
        nnib.StaTEI = 0x00;
    */
	if(getHplcTestMode() ==  RD_CTRL)
	{
		SetPib(0, CTRL_TEI_GW);
	}
	else
	{
		SetPib(0, 0);
	}

    //离网重启启动同步流程
    SetWorkState(e_LISEN);
    ScanBandManage(e_LEAVE, 0);
    //启动无线信道扫频流程
    ScanRfChMange(e_LEAVE, 0);

    nnib.SynRoutePeriodFlag = FALSE;

    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
    HPLC.sfo_is_done=0;
    HPLC.sfo_not_first = 0;
    HPLC.snid = 0;

    net_printf("---------------ClearNNIB----------------\n");


    //BeaconTimeSlotSet(0,0,0,0,0,0,0,0,0);
    InitHistoryPlanRecord();
    nnib.LProxyNodeUplinkRatio   = nnib.ProxyNodeUplinkRatio   = INITIAL_SUCC_RATIO;
    nnib.LProxyNodeDownlinkRatio = nnib.ProxyNodeDownlinkRatio = INITIAL_SUCC_RATIO;

//    memset(&gBcnRgainInfo, 0, sizeof(BCN_RGAIN_ASSESS_t));

    //防止STA发送错误的代理变更
    if(TMR_RUNNING == zc_timer_query(g_WaitChageProxyCnf))
    {
        timer_stop(g_WaitChageProxyCnf, TMR_NULL);
    }
    Trigger2app(TO_OFFLINE);


    //Test
//    U8 NodeMac[6] = {0x12,0x23,0x34,0x45,0x56,0x67};
//    SetMacAddrRequest(NodeMac, e_METER, e_MAC_METER_ADDR);
    nnib.RoutePeriodTime =RoutePeriodTime_FormationNet;//路由周期初始化(只接收无线精简信息，无法获取路由周期信息)
    //初始化心跳维护
    nnib.HeartBeatTime = (nnib.RoutePeriodTime * 1000) / 8;
    //无线发现列表维护参数初始化。  
    nnib.RfDiscoverPeriod = MAX(10, nnib.RoutePeriodTime/5);
    nnib.RfRcvRateOldPeriod = nnib.RoutePeriodTime/nnib.RfDiscoverPeriod;//4;

    nnib.SuccessRateZerocnt = 0;

#endif
    nnib.FristRoutePeriod =FALSE;

    nnib.Networkflag = FALSE;
    nnib.NextRoutePeriodStartTime = nnib.RoutePeriodTime;

    nnib.SendDiscoverListCount =0;
    nnib.powerlevelSet = FALSE;
    nnib.BandChangeState = FALSE;
    nnib.RfCount = 0;
    nnib.RfOnlyFindBtPco = TRUE;

    
    CleanDiscoveryTablelink();//memset(NeighborDiscoveryTableEntry, 0xff, sizeof(NeighborDiscoveryTableEntry));
    memset(NwkRoutingTable, 0xff, sizeof(NwkRoutingTable));
    memset(NwkAodvRouteTable, 0xff, sizeof(NwkAodvRouteTable));
    memset(RouteDiscoveryTable, 0xff, sizeof(RouteDiscoveryTable));
    memset(MsduDataSeq, 0xff, sizeof(MsduDataSeq));
    memset(AsscMsdu, 0xff, sizeof(AsscMsdu));
	tx_link_clean(&TX_MNG_LINK);
	tx_link_clean(&TX_DATA_LINK);	

	tx_link_clean(&WL_TX_MNG_LINK);
	tx_link_clean(&WL_TX_DATA_LINK);	
	
	StopNetworkMainten();

    return;
 }
U32 GetSNID(void)
{
   return HPLC.snid;
}
void SetPib(U32 snid,U16 tei)
{
    //PHY_DEMO.snid = snid;
    //PHY_DEMO.stei = tei;
	net_printf("snid=%08x,tei=%02x\n",snid,tei);
	if(getHplcTestMode() ==  RD_CTRL)
		HPLC.tei  = CTRL_TEI_GW;
	else
    	HPLC.tei  = tei;
    HPLC.snid = snid;
}
void SetNodeState(U8 state)
{
    nnib.NodeState = state;
}
U8 GetNodeState()
{
    return nnib.NodeState;
}
void SetWorkState(U8 state)
{
    nnib.WorkState = state;
}
U8 GetWorkState()
{
    return nnib.WorkState;
}

void SetSnidLkTp(U8 state)
{
    nnib.SnidlnkTp = state;
}
U8 GetSnidLkTp()
{
    return nnib.SnidlnkTp;
}

void SetNodeType(U8 state)
{
    nnib.NodeType = state;
}
U8 GetNodeType()
{
   return nnib.NodeType;
}
U8 GetPowerType()
{
	return nnib.PowerType;
}

U8* GetNnibMacAddr(void)
{
    return nnib.MacAddr;
}

U8* GetCCOAddr(void)
{
    return nnib.CCOMacAddr;
}

U16 GetTEI(void)
{
   return HPLC.tei;
}
void SetTEI(U16 tei)
{
   //PHY_DEMO.stei = tei;
   HPLC.tei = tei;
}

U16 GetProxy(void)
{
   return HPLC.sfotei;
}
void SetProxy(U16 sfotei)
{

   HPLC.sfotei = sfotei;
}

void SetNetworkflag(U8 state)
{
    nnib.Networkflag = state;
#if defined(STATIC_MASTER)
    if( PROVINCE_HUNAN == app_ext_info.province_code || PROVINCE_SHANNXI == app_ext_info.province_code)
    {
        if(TRUE == state)
        {
            cco_modify_curve_req_clock_timer(3, BroadcastCycleCfg.BroadcastCycle*60*(BroadcastCycleCfg.CycleUnit==1?1:60), TMR_PERIODIC);
        }
    }
#endif
}

   U8 GetNetworkflag()
 {
    return nnib.Networkflag;
 }
 void SetnnibDevicetype(U8 type)
{
    nnib.DeviceType= (type ==e_CKQ)? e_CKQ:
                     (type ==e_JZQ)? e_JZQ:
                     (type ==e_METER)? e_METER:
                     (type ==e_RELAY)? e_RELAY:
                     (type ==e_CJQ_2)? e_CJQ_2:
                     (type ==e_CJQ_1)? e_CJQ_1:
                     (type ==e_3PMETER)? e_3PMETER:
                      e_UNKW;

    app_printf("SetnnibDevicetype nnib.DeviceType=%d\n",nnib.DeviceType);

}
U8 GetnnibDevicetype()
{
    return nnib.DeviceType;

}
void SetnnibEdgetype(U8 type)
{

    nnib.Edge= (type ==e_COLLECT_NEGEDGE)? e_COLLECT_NEGEDGE:
                 (type ==e_COLLECT_POSEDGE)? e_COLLECT_POSEDGE:
                 (type ==e_COLLECT_DBLEDGE)? e_COLLECT_DBLEDGE:
                  e_RESET;
}
U8 GetnnibEdgetype()
{
    return nnib.Edge;

}

U16 GetRoutePeriodTime()
{
    return nnib.RoutePeriodTime;
}

void SetRoutePeriodTime(U16 sec)
{
	if(nnib.RoutePeriodTime != sec)
	{
		nnib.RoutePeriodTime = sec;
        if(TMR_RUNNING == zc_timer_query(Discoverytimer))
		StartMmDiscovery();
		
	}
    
}


U16 GetNextRoutePeriodTime()
{
    return nnib.NextRoutePeriodStartTime;
}

void SetNextRoutePeriodTime(U16 sec)
{
    nnib.NextRoutePeriodStartTime = sec;
}

U8 GetBeaconTimeSlot()
{
    if(getHplcOptin() == 3)//Option 3时，信标时隙大小为50ms，不分频段，现场使用需要注意效率
    {
        return 50;
    }
    switch (HPLC.band_idx) {
    case 0:
        return 15;
        break;
    case 1:
        return 20;
        break;
    case 2:
    case 3:
        return 25;
        break;
    default:
        return 20;
        break;
    }

}

void setHplcBand(U8 bandID)
{
    HPLC.band_idx = bandID;
}
U8   GetHplcBand()
{
    return HPLC.band_idx;
}

void setHplcTestMode(U32 mode)
{
    HPLC.testmode = mode;
}
U32  getHplcTestMode()
{
    return HPLC.testmode;
}

void setPossiblePhase(U8 phase)
{
    HPLC.phase = nnib.PossiblePhase = phase;
}
U8   getPossiblePhase()
{
    return nnib.PossiblePhase;
}

void setSwPhase(U8 sw)
{
	HPLC.sw_phase = sw;
}


#if defined(STD_DUAL)
/**
 * @brief setHplcOptin
 * @param option
 */
void setHplcOptin(U8 option)
{
    HPLC.option = option;
}
/**
 * @brief setHplcRfChannel
 * @param rfchannel
 */
void setHplcRfChannel(U32 rfchannel)
{
    HPLC.rfchannel = rfchannel;
}
/**
 * @brief getHplcOptin
 * @return
 */
U8 getHplcOptin()
{
    return HPLC.option;
}
/**
 * @brief getHplcRfChannel
 * @return
 */
U32 getHplcRfChannel()
{
    return HPLC.rfchannel;
}

/**
 * @brief               检查无线信道参数合法性
 * 
 * @param option        option  1-3
 * @param channel       channel： option 1:信道号 1-40; option 2:信道号 1-80; option 3:信道号 1-200
 * @return U8           TRUE:参数合法； FALSE：参数不合法
 */
U8 checkRfChannel(U8 option, U16 channel)
{
    switch (option)
    {
        case 1:
            if(channel < 1 || channel >40)
                return FALSE;
            break;
        case 2:
            if(channel < 1 || channel >80)
                return FALSE;
            break;
        case 3:
            if(channel < 1 || channel >200)
                return FALSE;
            break;
        default:
            return FALSE;
    }

    return TRUE;
}

#endif

/********************************Neighborlist func**********************************/

/********************************一跳节点根据信标接收增益评估逻辑相位***********************/
#define  ACTIVE_GAIN_DIFF  3  //相位评估有效差值

BCN_RGAIN_ASSESS_t gBcnRgainInfo;
/**
 * @brief GetBcnAccessPhase
 * @param gain
 * @param phase
 * @param BeaconPeriodCount
 * @return
 */
U8 GetBcnAccessPhase(S8 gain, U8 phase, U8 BeaconPeriodCount)
{
    S8 again = 0x7F;
    S8 bgain = 0x7F;
    S8 cgain = 0x7F;

    U8 retphase = e_UNKNOWN_PHASE;

//    static U32 mSnid = snid;
//    if(mSnid != snid)
//    {
//        memset(&gBcnRgainInfo, 0, sizeof(BCN_RGAIN_ASSESS_t));
//    }

    static U32 lastBpcA = 0;
    static U32 lastBpcB = 0;
    static U32 lastBpcC = 0;

    switch(phase)
    {
    case 1:
        if(lastBpcA == BeaconPeriodCount && GetNodeState() != e_NODE_OUT_NET)
             return retphase;
        lastBpcA = BeaconPeriodCount;

        gBcnRgainInfo.rgain.rgain_a += gain;
        gBcnRgainInfo.rgain.a_cnt++;
        break;
    case 2:

        if(lastBpcB == BeaconPeriodCount && GetNodeState() != e_NODE_OUT_NET)
            return retphase;
        lastBpcB = BeaconPeriodCount;

        gBcnRgainInfo.rgain.rgain_b += gain;
        gBcnRgainInfo.rgain.b_cnt++;
        break;
    case 3:

        if(lastBpcC == BeaconPeriodCount && GetNodeState() != e_NODE_OUT_NET)
            return retphase;
        lastBpcC = BeaconPeriodCount;

        gBcnRgainInfo.rgain.rgain_c += gain;
        gBcnRgainInfo.rgain.c_cnt++;
        break;
    default:
        break;
    }

    if(gBcnRgainInfo.rgain.a_cnt)
    {
        again = (gBcnRgainInfo.rgain.rgain_a + gBcnRgainInfo.avge.avge_a * gBcnRgainInfo.avge.a_cnt)/(gBcnRgainInfo.rgain.a_cnt + gBcnRgainInfo.avge.a_cnt);
    }

    if(gBcnRgainInfo.rgain.b_cnt)
    {
        bgain = (gBcnRgainInfo.rgain.rgain_b + gBcnRgainInfo.avge.avge_b * gBcnRgainInfo.avge.b_cnt)/(gBcnRgainInfo.rgain.b_cnt + gBcnRgainInfo.avge.b_cnt);
    }

    if(gBcnRgainInfo.rgain.c_cnt)
    {
        cgain = (gBcnRgainInfo.rgain.rgain_c + gBcnRgainInfo.avge.avge_c * gBcnRgainInfo.avge.c_cnt)/(gBcnRgainInfo.rgain.c_cnt + gBcnRgainInfo.avge.c_cnt);
    }

    if(GetNodeState() == e_NODE_OUT_NET)
    {
        retphase =  (again < bgain) ? ((again < cgain) ? e_A_PHASE : e_C_PHASE) : ((bgain < cgain) ? e_B_PHASE : e_C_PHASE);

        net_printf("again : %d, bgain : %d, cgain : %d, phase : %d\n", again, bgain, cgain, retphase);
    }

    if(gBcnRgainInfo.rgain.a_cnt >= MAX_ASSESS_CNT || gBcnRgainInfo.rgain.b_cnt >= MAX_ASSESS_CNT || gBcnRgainInfo.rgain.c_cnt >= MAX_ASSESS_CNT)
    {
        //判断个相线增益差值，防止在噪声监测过程中，设置固定增益导致评估异常。
        if(abs(again - bgain) > ACTIVE_GAIN_DIFF || abs(again - cgain) > ACTIVE_GAIN_DIFF || abs(bgain - cgain) > ACTIVE_GAIN_DIFF)
            retphase =  (again < bgain) ? ((again < cgain) ? e_A_PHASE : e_C_PHASE) : ((bgain < cgain) ? e_B_PHASE : e_C_PHASE);
        

        gBcnRgainInfo.avge.a_cnt = gBcnRgainInfo.rgain.a_cnt;
        gBcnRgainInfo.avge.b_cnt = gBcnRgainInfo.rgain.b_cnt;
        gBcnRgainInfo.avge.c_cnt = gBcnRgainInfo.rgain.c_cnt;


        gBcnRgainInfo.avge.avge_a = gBcnRgainInfo.avge.a_cnt > 0 ? (gBcnRgainInfo.rgain.rgain_a / gBcnRgainInfo.rgain.a_cnt): 0x7F;
        gBcnRgainInfo.avge.avge_b = gBcnRgainInfo.avge.b_cnt > 0 ? (gBcnRgainInfo.rgain.rgain_b / gBcnRgainInfo.rgain.b_cnt): 0x7F;
        gBcnRgainInfo.avge.avge_c = gBcnRgainInfo.avge.c_cnt > 0 ? (gBcnRgainInfo.rgain.rgain_c / gBcnRgainInfo.rgain.c_cnt): 0x7F;

        net_printf("again : %d, bgain : %d, cgain : %d, phase : %d\n", again, bgain, cgain, retphase);
        net_printf("back rgain : a: %d<%d>,  b: %d<%d>, c: %d<%d>\n"
                   , gBcnRgainInfo.avge.avge_a
                   , gBcnRgainInfo.avge.a_cnt
                   , gBcnRgainInfo.avge.avge_b
                   , gBcnRgainInfo.avge.b_cnt
                   , gBcnRgainInfo.avge.avge_c
                   , gBcnRgainInfo.avge.c_cnt);

        memset(&gBcnRgainInfo.rgain, 0, sizeof(BCN_RGAIN_INFO_t));
    }

    return retphase;

}

 int8_t NeighborDiscovery_link_init(void)
{
	INIT_LIST_HEAD(&NeighborDiscoveryHeader.link);
	NeighborDiscoveryHeader.nr = 0;
	NeighborDiscoveryHeader.thr = MAX_NEIGHBOR_NUMBER/2;	

    //无线
    INIT_LIST_HEAD(&RfNeighborDiscoveryHeader.link);
    RfNeighborDiscoveryHeader.nr = 0;
    RfNeighborDiscoveryHeader.thr = MAX_NEIGHBOR_NUMBER/2;
	return TRUE;
}

static void Init_NeighborDisEntry(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *cfg, U8 *MacAddr, U32 SNID, U16 NodeTEI, U8 NodeDepth, U8 NodeType,U16 Proxy,U8 Phase, S8 GAIN)
{
//		net_printf("addr new sta=0x%02x,snid=%08x\n",NodeTEI,SNID);
		if(MacAddr != NULL)
		{
			__memcpy(cfg->MacAddr,MacAddr,6);
		    cfg->NodeDepth = NodeDepth;
            cfg->NodeType = NodeType;
            cfg->Phase = Phase;
			if( NodeTEI !=1 && Proxy == GetTEI()  && SNID == GetSNID())
	        {
	            cfg->Relation =  e_CHILD;

	        }
	        else
	        {
	            cfg->Relation = e_NEIGHBER;
	        }
		}

        cfg->LinkType = e_HPLC_Link;
        cfg->MsduSeq = 0xffff;      //防止过滤第一帧msdu序号为0的报文
        cfg->ResetTimes = 0xff;      //防止过滤第一帧msdu序号为0的报文

        cfg->SNID = SNID;
        cfg->NodeTEI = NodeTEI;
        cfg->BKRouteFg =  0;
        cfg->Proxy = Proxy;
        cfg->HplcInfo.GAIN = GAIN;
        cfg->HplcInfo.RecvCount = 1;

        cfg->HplcInfo.UplinkSuccRatio = INITIAL_SUCC_RATIO;
        cfg->HplcInfo.DownlinkSuccRatio = INITIAL_SUCC_RATIO;
        cfg->HplcInfo.LastUplinkSuccRatio = INITIAL_SUCC_RATIO;
        cfg->HplcInfo.LastDownlinkSuccRatio = INITIAL_SUCC_RATIO;
        cfg->HplcInfo.My_REV = 0;
        cfg->HplcInfo.PCO_SED = 0;
        cfg->HplcInfo.PCO_REV = 0;
        cfg->HplcInfo.ThisPeriod_REV =0;
        cfg->RemainLifeTime = nnib.RoutePeriodTime==0? RoutePeriodTime_Normal:nnib.RoutePeriodTime;
        cfg->ResetTimes = 0;
        cfg->LastRst = 0;
		cfg->BeaconCount = 0;
}

/*函数名称：邻居表的更新
邻居表的更新原则
1.已存在的节点，更新信息
2.不存在的节点，首先判断是否已满，未满直接添加，已满则剔除
3.剔除原则，剔除不是本网络SNID的邻居节点中增益最大的节点，前者不存在是，剔除本网络中层级最高的增益最大点

邻居表更新的时机：
1.sof: 
错误sof:只用于评估增益,生存时间
正确sof：接收到长MAC头的MPDU其携带MAC,使用MAC更新邻居表中的增益信息和NodeDepth、NodeType、Proxy、Phase信息 ; 短MAC头(多为aps数据)使用SNID和TEI更新增益信息生存时间
2.接收到信标
错误beacon:只用于评估增益、生存时间
正确beacon：使用MAC更新邻居表中的增益信息和NodeDepth、NodeType、Proxy、Phase信息

3.几个特殊网络命令
*/


void UpdateNbInfo(U32 SNID, U8 *MacAddr, U16 NodeTEI, U8 NodeDepth, U8 NodeType, U16 Proxy, U8 Relation, S8 GAIN, U16 Phase,U8 BeaconPeriodCount,U8 Frametype)
{


    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry;

//    static U8 beaconPhase = 0;

    list_head_t *pos,*node;

    if(nnib.NodeState != e_NODE_ON_LINE && Frametype != DT_BEACON)
    {
        return;
    }

    if(NodeTEI==0)
    {
        return;
    }

    if(MacAddr !=NULL) //使用mac地址查找
    {
        
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            TableEntry = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(SNID == TableEntry->SNID && TableEntry->NodeTEI == NodeTEI)
            {
                if(Frametype == DT_BEACON) //信标类型需要过滤重复信标,用于统计
                {
                    //听到邻居节点在其他相位的信标，并且信号质量更好，更新邻居信息。
                    if(Phase != 0 && SNID==GetSNID())
                    {
                        if(NodeTEI == CCO_TEI)
                        {
//                            if( (BeaconPeriodCount != TableEntry->BeaconCount || beaconPhase != Phase))
//                            {
//                                beaconPhase = Phase;

                                U8 retPhase = GetBcnAccessPhase(GAIN, Phase, BeaconPeriodCount);
                                if(retPhase > 0)
                                {
                                    TableEntry->Phase = retPhase;
                                }

                                if(GetProxy() == CCO_TEI && getPossiblePhase() != TableEntry->Phase)
                                {
                                    setPossiblePhase(TableEntry->Phase);
                                    net_printf("level 1 STA sync CCO phase %d\n", TableEntry->Phase);
                                }
//                            }
                        }
                        else
                        {
                            #if defined(STATIC_MASTER)
                            DeviceTEIList[NodeTEI - 2].LogicPhase = Phase;
                            #else
                            if(NodeTEI == GetProxy())
                            {
                                if(getPossiblePhase() != Phase)
                                {
                                    setPossiblePhase(Phase);
                                    net_printf("STA sync proxy phase %d\n", Phase);
                                }
                            }
                            #endif

                            TableEntry->Phase = Phase;
                        }
                    }
                    if(BeaconPeriodCount == TableEntry->BeaconCount)
                    {
                        return;
                    }
                    TableEntry->BeaconCount = BeaconPeriodCount;
                    if(nnib.BeaconLQIFlag)
                    {
                        TableEntry->HplcInfo.ThisPeriod_REV++;
                    }
                   //net_printf("ThisPeriod_REV [%03x]: %d, bpc:%08x\n",NodeTEI, TableEntry->HplcInfo.ThisPeriod_REV, BeaconPeriodCount);
                }
                if(0 != Relation)
                {
                    TableEntry->Relation = Relation;
                }
                __memcpy(TableEntry->MacAddr,MacAddr,6);
                TableEntry->NodeType = NodeType;
                TableEntry->NodeDepth = NodeDepth;
                TableEntry->Proxy = Proxy;
                //                TableEntry->Phase = Phase;    //<<<
                if( NodeTEI !=1 && Proxy == GetTEI() && SNID == GetSNID())
                {
                    TableEntry->Relation =  e_CHILD;
                    TableEntry->BKRouteFg =  0;
                }

                if(GetNodeState() == e_NODE_ON_LINE && GetSNID() == SNID) //在线的节点更新邻居表角色
                {
                    UpDataNeighborRelation(TableEntry, Proxy, NodeDepth, TableEntry->childUpRatio, TableEntry->childDownRatio);
                }

                
                return;

				
			}
			
		}
        
			
	}
	else
	{
        
		list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)//已存在的节点，根据参数更新信息
	    {
            TableEntry = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
	        if(SNID == TableEntry->SNID && TableEntry->NodeTEI == NodeTEI)
	        {
	        	
                TableEntry->HplcInfo.GAIN += GAIN;
                TableEntry->HplcInfo.RecvCount++;
	            TableEntry->RemainLifeTime = nnib.RoutePeriodTime==0? RoutePeriodTime_Normal:nnib.RoutePeriodTime;
                
	            return;
	        }
	    }
        
		
    }
    TableEntry=zc_malloc_mem(sizeof(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t), "table",MEM_TIME_OUT);
	Init_NeighborDisEntry(TableEntry, MacAddr, SNID, NodeTEI, NodeDepth, NodeType,Proxy, Phase, GAIN);
	if(datalink_table_add_by_tei_size(&NeighborDiscoveryHeader,&TableEntry->link,TableEntry->NodeTEI)==-1)
	{
		zc_free_mem(TableEntry);
	}
	return;
	



}




/*函数名称：更新邻居表中的生存时间，有秒定时器触发调用
当某个邻居节点从邻居表中消失时：
1.若消失的节点是路由表中的下一跳时，将此条路由标记为失效路由（此处理可删除，路由表在其他地方更新时,将覆盖本条路由，状态也置位有效）
2.若消失的邻居节点是代理节点时，发起代理变更（正常情况下，在消失之前本节点已经发起变更，已改变了和原代理节点的关系，这里触发代理变更属于保护措施）
*/
void UpDataNeighborListTime()
{

	list_head_t *pos,*node;//,*pos1,*node1;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
	//ROUTE_TABLE_ENTRY_t  			 *RouteTable;

    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
	{
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
		DiscvryTable->RemainLifeTime--;
		if(DiscvryTable->RemainLifeTime ==0)
		{
            route_table_invalid(DiscvryTable->NodeTEI);
            route_activelik_clean(DiscvryTable->NodeTEI, e_HPLC_Link);
#if defined(STATIC_NODE)
            U8 Relation = DiscvryTable->Relation;
            if(Relation == e_PROXY && nnib.LinkType == e_HPLC_Link)
			{
				net_printf("waring------PROXY is del in neighbor list,SendProxyChangeReq-------");
				
				sendproxychangeDelay(e_ProxyLossInNb);	//优化拓扑
				
				nnib.ProxyNodeUplinkRatio = 0;
				nnib.ProxyNodeDownlinkRatio = 0;
            }
#endif
            datalink_table_del(&NeighborDiscoveryHeader,&DiscvryTable->link, TRUE);

		}

	}
    
}


/***********************无线邻居表***************************/
/**
 * @brief Init_NeighborDisEntry     新增无线邻居表节点信息初始化函数
 * @param cfg                       申请的节点地址
 * @param MacAddr                   邻居表MAC地址
 * @param SNID                      邻居表所属网络ID
 * @param NodeTEI                   邻居表TEI
 * @param NodeDepth                 邻居表网络层级
 * @param NodeType                  邻居表类型
 * @param Proxy                     邻居表父节点TEI
 * @param RSSI                      数据接收信号强度
 * @param SNR                       数据接收信噪比
 */
static void Init_RfNeighborDisEntry(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *cfg, U8 *MacAddr, U32 SNID, U16 NodeTEI, U8 NodeDepth, U8 NodeType,U16 Proxy,S8 RSSI, S8 SNR, U8 rfCount)
{
    if(MacAddr != NULL)
    {
        __memcpy(cfg->MacAddr, MacAddr, 6);
        cfg->NodeDepth = NodeDepth;
        cfg->NodeType = NodeType;
        if(NodeTEI != 1 && Proxy == GetTEI() && GetSNID() == SNID)
        {
            cfg->Relation = e_CHILD;
        }
        else
        {
            cfg->Relation = e_NEIGHBER;
        }
    }



    cfg->LinkType = e_RF_Link;
    cfg->MsduSeq = 0xffff;      //防止过滤第一帧msdu序号为0的报文
    cfg->ResetTimes = 0xff;      //防止过滤第一帧msdu序号为0的报文

    cfg->SNID = SNID;
    cfg->NodeTEI = NodeTEI;
    cfg->Proxy = Proxy;
    cfg->BKRouteFg = 0;
    cfg->HrfInfo.UpdateIndex = 0;
#if 0
    memset(cfg->HrfInfo.RcvMap, 0, sizeof(cfg->HrfInfo.RcvMap));
#else //初始化无线下行接受率为90
    U8 i;
    for(i = 0; i < (RF_RCVMAP * 25)/100; i++)
    {
        bitmap_set_bit(cfg->HrfInfo.RcvMap, i);
    }
#endif
    cfg->HrfInfo.DownRcvRate = (bitmap_get_one_nr(cfg->HrfInfo.RcvMap, sizeof(cfg->HrfInfo.RcvMap)) * 100) / RF_RCVMAP;;
    cfg->HrfInfo.UpRcvRate = 0;
    cfg->HrfInfo.NotUpdateCnt = 0;

    cfg->HrfInfo.DownRssiCnt = 1;
//    cfg->HrfInfo.DownRssiCnt = MIN(MAX_RSSI_SNR_CNT, cfg->HrfInfo.DownRssiCnt  +1);
    cfg->HrfInfo.DownRssi = RSSI;
    cfg->HrfInfo.DownSnr = SNR;
    cfg->HrfInfo.DownSrnCnt = 1;
//    cfg->HrfInfo.DownSrnCnt = MIN(MAX_RSSI_SNR_CNT, cfg->HrfInfo.DownSrnCnt  +1);

    cfg->RemainLifeTime = nnib.RoutePeriodTime==0? RoutePeriodTime_Normal:nnib.RoutePeriodTime;

    cfg->HrfInfo.DicvPeriodCntDown       = MAX(10, nnib.RfDiscoverPeriod);//cfg->HrfInfo.RfDiscoverPeriod   = 10;            //test
    cfg->HrfInfo.RcvRateOldPeriodCntDown = MAX(4, nnib.RfRcvRateOldPeriod);//cfg->HrfInfo.RfRcvRateOldPeriod = 4;       //test



}

/**
 * @brief UpdateRfNbInfo        更新无线邻居表信息
 * @param SNID                  网络号
 * @param MacAddr               邻居表MAC地址
 * @param NodeTEI               邻居表TEI
 * @param NodeDepth             邻居表网络层级
 * @param NodeType              邻居表节点类型
 * @param Proxy                 邻居表父节点TEI
 * @param Relation              角色信息
 * @param RSSI                  接收信号强度
 * @param SNR                   信噪比
 * @param BeaconPeriodCount     信标周期计数
 * @param Frametype             帧类型
 */
void UpdateRfNbInfo(U32 SNID, U8 *MacAddr, U16 NodeTEI, U8 NodeDepth, U8 NodeType, U16 Proxy, U8 Relation, S8 RSSI, S8 SNR, U8 BeaconPeriodCount, U8 rfCount, U8 Frametype)
{
    list_head_t *pos,*n;//,*pos1,*node1;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    //ROUTE_TABLE_ENTRY_t  			 *RouteTable;

    if(nnib.NodeState != e_NODE_ON_LINE && Frametype != DT_BEACON)
    {
        return;
    }

    if(NodeTEI == 0)
    {
//        net_printf("TEI %d Erro!\n");
        return;
    }
    if(MacAddr)
    {
		if(!checkAccessControlList(MacAddr, 1) )
		{
			if(AccessListSwitch ==TRUE)
			{
			   net_printf("----------NbInfo checkAccessControlList!:%02x %02x %02x %02x %02x %02x\n",MacAddr[0],
			       MacAddr[1],
			       MacAddr[2],
			       MacAddr[3],
			       MacAddr[4],
			       MacAddr[5]);
			   return;
			}
		}
        list_for_each_safe(pos, n, &RfNeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(SNID == DiscvryTable->SNID && DiscvryTable->NodeTEI == NodeTEI)
            {
                if(Frametype == DT_BEACON)
                {
                    if(BeaconPeriodCount == DiscvryTable->BeaconCount)
                    {
                        return;
                    }
                    //                TableEntry->BeaconCount = BeaconPeriodCount;
                }
                if(0 != Relation)
                {
                    DiscvryTable->Relation = Relation;
                }
                __memcpy(DiscvryTable->MacAddr, MacAddr, 6);
                DiscvryTable->NodeDepth = NodeDepth;
                DiscvryTable->NodeType = NodeType ;
                DiscvryTable->Proxy = Proxy;

                if(NodeTEI != 1 && Proxy == GetTEI() && SNID == GetSNID())
                {
                    DiscvryTable->Relation = e_CHILD;
                    DiscvryTable->BKRouteFg =  0;
                }

                if(GetNodeState() == e_NODE_ON_LINE && GetSNID() == SNID)
                {
                    //更新角色信息。
                    UpDataRfNeighborRelation(DiscvryTable, Proxy, NodeDepth, DiscvryTable->childUpRatio, DiscvryTable->childDownRatio);
                }

                DiscvryTable->RemainLifeTime = nnib.RoutePeriodTime==0? RoutePeriodTime_Normal:nnib.RoutePeriodTime;

                return;
            }



        }

    }
    else
    {
        list_for_each_safe(pos, n, &RfNeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(DiscvryTable->SNID == SNID && DiscvryTable->NodeTEI == NodeTEI)
            {
                if(DiscvryTable->HrfInfo.DownRssiCnt >= MAX_RSSI_SNR_CNT)
                {
                    DiscvryTable->HrfInfo.DownRssiCnt = 1;
                    DiscvryTable->HrfInfo.DownRssi = RSSI;
                }
                else
                {
                    DiscvryTable->HrfInfo.DownRssiCnt++;
                    DiscvryTable->HrfInfo.DownRssi = (((S32)(DiscvryTable->HrfInfo.DownRssi) * (DiscvryTable->HrfInfo.DownRssiCnt-1)) + RSSI)/DiscvryTable->HrfInfo.DownRssiCnt;
                }

                if(DiscvryTable->HrfInfo.DownSrnCnt >= MAX_RSSI_SNR_CNT)
                {
                    DiscvryTable->HrfInfo.DownSrnCnt = 1;
                    DiscvryTable->HrfInfo.DownSnr = SNR;
                }
                else
                {
                    DiscvryTable->HrfInfo.DownSrnCnt++;
                    DiscvryTable->HrfInfo.DownSnr = (((S32)(DiscvryTable->HrfInfo.DownSnr) * (DiscvryTable->HrfInfo.DownSrnCnt - 1)) + SNR)/DiscvryTable->HrfInfo.DownSrnCnt;
                }
//                DiscvryTable->RemainLifeTime = nnib.RoutePeriodTime==0? RoutePeriodTime_Normal:nnib.RoutePeriodTime;
                return;
            }
        }
    }

    DiscvryTable = zc_malloc_mem(sizeof(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t), "rfTable", MEM_TIME_OUT);
    Init_RfNeighborDisEntry(DiscvryTable, MacAddr, SNID, NodeTEI, NodeDepth, NodeType, Proxy, RSSI, SNR, rfCount);
    if(datalink_table_add_by_tei_size(&RfNeighborDiscoveryHeader,&DiscvryTable->link,DiscvryTable->NodeTEI)==-1)
    {
        zc_free_mem(DiscvryTable);
    }
    return;

}
/**
 * @brief UpDataRfNeighborListTime
 * 更新无线邻居表生存时间，老化处理.在链路层秒级维护定时器中使用
 */
void UpDataRfNeighborListTime()
{
    list_head_t *pos,*node;//,*pos1,*node1;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    //ROUTE_TABLE_ENTRY_t  			 *RouteTable;

    list_for_each_safe(pos, node, &RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        DiscvryTable->RemainLifeTime--;
        if(DiscvryTable->RemainLifeTime == 0)       //从邻居表中消失
        {
            #if defined(STATIC_NODE)
            if(DiscvryTable->Relation == e_PROXY && nnib.LinkType == e_RF_Link)   //父节点消失，发起代理变更
            {

                //TODO 发起代理变更
                net_printf("Proxy is del from neighbor list\n");
                sendproxychangeDelay(e_ProxyLossInNb);	//优化拓扑

                nnib.ProxyNodeUplinkRatio = 0;
                nnib.ProxyNodeDownlinkRatio = 0;
            }
            #endif

            route_table_invalid(DiscvryTable->NodeTEI);
            route_activelik_clean(DiscvryTable->NodeTEI, e_RF_Link);

            datalink_table_del(&RfNeighborDiscoveryHeader,&DiscvryTable->link, TRUE);

            continue;
        }

//        net_printf("DicvPeriodCntDown:%d, RcvRateOldPeriodCntDown:%d\n", DiscvryTable->HrfInfo.DicvPeriodCntDown, DiscvryTable->HrfInfo.RcvRateOldPeriodCntDown);
        DiscvryTable->HrfInfo.DicvPeriodCntDown--;
        if(DiscvryTable->HrfInfo.DicvPeriodCntDown == 0)
        {
            //接收率老化
            {
                DiscvryTable->HrfInfo.UpdateIndex = (DiscvryTable->HrfInfo.UpdateIndex + 1)%RF_RCVMAP;
                bitmap_clr_bit(DiscvryTable->HrfInfo.RcvMap, DiscvryTable->HrfInfo.UpdateIndex);
                DiscvryTable->HrfInfo.DownRcvRate = (bitmap_get_one_nr(DiscvryTable->HrfInfo.RcvMap, sizeof(DiscvryTable->HrfInfo.RcvMap)) * 100) / RF_RCVMAP;
                //刷新周期发现列表周期计数
                if(DiscvryTable->Relation == e_PROXY)
                {
                    //TODO 如果是代理节点且通信质量变差，需要发起代理变更
                }
                DiscvryTable->HrfInfo.DicvPeriodCntDown = nnib.RfDiscoverPeriod;//DiscvryTable->HrfInfo.RfDiscoverPeriod;
            }


            DiscvryTable->HrfInfo.RcvRateOldPeriodCntDown--;
            if(DiscvryTable->HrfInfo.RcvRateOldPeriodCntDown == 0)
            {
                //老化上行接收率为0
                DiscvryTable->HrfInfo.UpRcvRate = 0;

                //刷新计数
                DiscvryTable->HrfInfo.RcvRateOldPeriodCntDown = nnib.RfRcvRateOldPeriod;//DiscvryTable->HrfInfo.RfRcvRateOldPeriod;
            }



            if(DiscvryTable->Relation == e_PROXY && nnib.LinkType== e_RF_Link)
            {
                nnib.ProxyNodeUplinkRatio = DiscvryTable->HrfInfo.UpRcvRate;
                nnib.ProxyNodeDownlinkRatio = DiscvryTable->HrfInfo.DownRcvRate;
            }
        }
    }
}




//更新邻居表中接收邻居的次数
/*void UpdataNbStaRecvCnt(U32 _SNID, U8 *_MacAddr, U16 _NodeTEI)
{
    U16 nbSeq;

    nbSeq = GetNeighborSeq(_NodeTEI);
    if(nbSeq != INVALID)
    {
        NeighborDiscoveryTableEntry[nbSeq].ThisPeriod_REV++;
//        #if defined(STATIC_NODE)
//                net_printf("UpdataNbStaRecvCnt TEI : %04x, NwkBeaconPeriodCount = %08x, this ThisPeriod_REV = %d\n"
//                         , _NodeTEI, NwkBeaconPeriodCount, NeighborDiscoveryTableEntry[nbSeq].ThisPeriod_REV);
//        #else
//                extern U32 CCORcvBeaconPeriodCount;
//                net_printf("UpdataNbStaRecvCnt TEI : %04x, NwkBeaconPeriodCount = %08x, this ThisPeriod_REV = %d\n"
//                         , _NodeTEI, CCORcvBeaconPeriodCount, NeighborDiscoveryTableEntry[nbSeq].ThisPeriod_REV);
//        #endif
    }
}*/
U8  UpdataNbSendCntForBeacon()
{
    static U32 lastBeaconPeriodCount = 0;
//    if(bpc != NwkBeaconPeriodCount)
//    {
//        return FALSE;
//    }
    if(nnib.BeaconLQIFlag)
    {
        if(lastBeaconPeriodCount == NwkBeaconPeriodCount)
            return TRUE;

        lastBeaconPeriodCount = NwkBeaconPeriodCount;
        nnib.SendDiscoverListCount++;
//        net_printf(">>>     Discover SendCnt %d, %08x\n", nnib.SendDiscoverListCount, NwkBeaconPeriodCount);
    }
	return TRUE;
}

//冻结邻居表中上个路由周期接到其他节点的命令（beacon+discoverlist）次数。
void FrozenNeighborstaRecv()
{
	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        DiscvryTable->HplcInfo.My_REV = DiscvryTable->HplcInfo.ThisPeriod_REV;
        DiscvryTable->HplcInfo.ThisPeriod_REV = 0;

        DiscvryTable->LastRst = DiscvryTable->ResetTimes;
    }
    
}


U32 UpdataNeighborNet(U32 SNID)
{

    return 0;
}

/*函数描述：在邻居表中使用TEI寻找对应的MAC地址*/
void SearchMacInNeighborDisEntrybyTEI_SNID(U16 TEI , U32 SNID ,U8 *pMacAddr)
{
	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == SNID)
        {
            __memcpy(pMacAddr, DiscvryTable->MacAddr, 6);
            return;
        }
    }

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == SNID)
        {
            __memcpy(pMacAddr, DiscvryTable->MacAddr, 6);
            return;
        }
    }
    
}


//删除邻居表
void DelDiscoveryTableByTEI(U16 TEI)
{
    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == GetSNID())
        {
            datalink_table_del(&NeighborDiscoveryHeader,&DiscvryTable->link, TRUE);
            break;
        }
    }

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == GetSNID())
        {
            datalink_table_del(&RfNeighborDiscoveryHeader,&DiscvryTable->link, TRUE);
            break;
        }
    }

    return;
    
}

void CleanDiscoveryTablelink()
{
	 list_head_t *pos,*node;
     ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

	 list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
	 {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        datalink_table_del(&NeighborDiscoveryHeader, &DiscvryTable->link, TRUE);
//		return;
     }

     list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
     {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        datalink_table_del(&RfNeighborDiscoveryHeader, &DiscvryTable->link, TRUE);
//		return;
     }
     
	NeighborDiscovery_link_init();
}



/*函数描述：确定TEI是否在自己的邻居表中*/
//<<< 需要查找载波和无线邻居表，并返回查询邻居表是无线还是载波
U8 IsInNeighborDiscoveryTableEntry(U16 TEI )
{

	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    U8 null_addr[6] = {0};

    if(AccessListSwitch && nnib.AODVRouteUse)    //访问控制列表启动时，如果路由被删除，测试AODV功能
    {
        return FALSE;
    }

    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == GetSNID()
                && 0 != memcmp(DiscvryTable->MacAddr, null_addr, 6))
        {
            
            return TRUE;
        }
    }
    
    return FALSE;
}




//函数描述：在邻居表中使用TEI寻找对应的站点层级数
U8 SearchDepthInNeighborDiscoveryTableEntry(U16 TEI, U8 LinkType)
{
	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    DATALINK_TABLE_t *neib_list_header;
    
    if(LinkType == e_HPLC_Link)
    {
        neib_list_header = &NeighborDiscoveryHeader;
    }
    else 
    {
        neib_list_header = &RfNeighborDiscoveryHeader;
    }
  
    list_for_each_safe(pos, node,&neib_list_header->link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == GetSNID())
        {            
            return DiscvryTable->NodeDepth ;
        }
    }
    
    return 0x0f;
}


//函数名称:获取子节点数量 返回 邻居表数量
//<<< 存在子节点只能载波通信或者是无线通信
U16 GetChildCount()
{
    U16 ChildCount = 0;
	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->Relation == e_CHILD)
        {
            ChildCount++;
        }
    }
    
    return ChildCount;
}


//函数名称:根据TEI获取在邻居表中的序号

ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t* GetNeighborByTEI(U16 TEI , U8 linkType)
{
	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable=NULL;

    if(linkType == e_HPLC_Link)
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == GetSNID())
            {

                return DiscvryTable;
            }
        }
    }
    else if(linkType == e_RF_Link)
    {
        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(DiscvryTable->NodeTEI == TEI && DiscvryTable->SNID == GetSNID())
            {

                return DiscvryTable;
            }
        }
    }
    
    return NULL;
}
//通过序号获取一条完整的device条目信息，提供给其他层使用
/*U8 Get_Neighbor_All(U16 Seq , NEIGHBOR_DISCOVERY_TABLE_ENTRY_t* NeighborListTemp)
{
    if(Seq<MAX_NEIGHBOR_NUMBER)
    {
        __memcpy((U8*)NeighborListTemp,(U8*)&NeighborDiscoveryTableEntry[Seq],sizeof(NEIGHBOR_DISCOVERY_TABLE_ENTRY_t));
        return TRUE;
    }
    return FALSE;
}*/



//函数名称:获取邻居表中数量 返回 邻居表数量，仅表示处于同一网络中的邻居节点，提供给其他层使用

U16 GetNeighborCount()
{

    return NeighborDiscoveryHeader.nr;
}

//函数名称:获取邻居表中的位图 返回 邻居表数量,此返回值只在发起的第一次有效
U16 GetNeighborBmp(U8 *OnlineTEIbmp, U16 *ByteNum)
{
    U8 Byteoffset, Bitoffset;
    U16 NeighborNum = 0;
    U16 maxTei = 0;

	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
	{
       DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
       if(  DiscvryTable->SNID == GetSNID())
	   	{
	   		Byteoffset = DiscvryTable->NodeTEI / 8;
            Bitoffset = DiscvryTable->NodeTEI % 8;
            Bitoffset = 1 << Bitoffset;
            OnlineTEIbmp[Byteoffset] = OnlineTEIbmp[Byteoffset] | Bitoffset;
            NeighborNum++;

            if(DiscvryTable->NodeTEI > maxTei)
                maxTei = DiscvryTable->NodeTEI;
	   	}
		
	}

    //发送心跳报文增加无线邻居节点信息
    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
       DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
       if(  DiscvryTable->SNID == GetSNID())
        {
            Byteoffset = DiscvryTable->NodeTEI / 8;
            Bitoffset = DiscvryTable->NodeTEI % 8;
            Bitoffset = 1 << Bitoffset;
            OnlineTEIbmp[Byteoffset] = OnlineTEIbmp[Byteoffset] | Bitoffset;
            NeighborNum++;

            if(DiscvryTable->NodeTEI > maxTei)
                maxTei = DiscvryTable->NodeTEI;
        }

    }
    

    if(ByteNum != NULL)
    {
        if(maxTei)
            *ByteNum = maxTei/8 + 1;
        else
            *ByteNum = 0;
    }
    return NeighborNum;
}

#if defined(STATIC_MASTER)
U16 GetChildStaBmpByteNum(U8 *pStaBmpBuf)
{
    U8 	ByteNum;
    U16 i, MaxTEI = 0;
    for(i = 0; i < ChildCount; i++)
    {
        ChildStaIndex[i] += 2;		//将序号转换为TEI
        if(ChildStaIndex[i] > MaxTEI)
        {
            MaxTEI = ChildStaIndex[i];
        }
        pStaBmpBuf[ChildStaIndex[i]/8] |= (0x01 << (ChildStaIndex[i]%8));
    }
    if(MaxTEI)
    {
        ByteNum = MaxTEI / 8 + 1;
    }
    else
    {
        ByteNum = 0;
    }
    return ByteNum;
}

//判断对方是否能听到我
U8 NeighborNetCanHearMe(U32 NbNetID,U32 WithNid)
{
#if defined(STD_2016)|| defined(STD_DUAL)

    U8 j,i;

    if(WithNid == HPLC.snid)
    {
        return TRUE;
    }

    for(j = 0; j < MaxSNIDnum; j++)
    {
        if(NeighborNet[j].SendSNID	==	NbNetID)
        {

            break;
        }
    }
    if(j==MaxSNIDnum) //new nb
    {

        return FALSE;
    }
    else  //old nb
    {
        for(i=0;i<MaxSNIDnum;i++)
        {
            if(NeighborNet[j].NbSNID[i]	==	HPLC.snid)
            {

                return TRUE;
            }
        }
    }
    return FALSE;
#elif defined(STD_GD)

    if(   (WithNid>>(NbNetID-1)) &0x00000001)
    {
        return TRUE;
    }
    return FALSE;

#endif

}

void StartRfChlChgeForCCO()
{
//    uint8_t m,i;

    if(zc_timer_query(g_ChangeRfChannelTimer) == TMR_RUNNING ||  nnib.RfChannelChangeState == 1)
    {
//        net_printf("rfchruning\n");
        return;
    }

    if(!GetRfConsultEn())
    {
        return;
    }

    if(TMR_STOPPED == zc_timer_query(g_ChangeRfChannelTimer))
    {
        if(g_ChangeRfChannelTimer == NULL)
        {
            g_ChangeRfChannelTimer = timer_create(60*1000,
                                                  0,
                                                  TMR_TRIGGER,
                                                  ChangeRfChannelTimerCB,
                                                  NULL,
                                                  "ChangeRfChannelTimerCB");

        }else
        {
            timer_modify(g_ChangeRfChannelTimer,
                         60*1000,
                         0,
                         TMR_TRIGGER,
                         ChangeRfChannelTimerCB,
                         NULL,
                         "ChangeChannelPlcTimerCB",
                         TRUE);
            timer_start(g_ChangeRfChannelTimer);
        }
        if(g_ChangeRfChannelTimer)
        {
            net_printf("start g_ChangeRfChannelTimer\n");
            timer_start(g_ChangeRfChannelTimer);
        }
    }
//    for(m=0; m < e_op_end; m++)
//    {
//        if(scanRfChArry[m].option == getHplcOptin() && scanRfChArry[m].channel == getHplcRfChannel())
//            continue;

//        for(i=0; i<MaxSNIDnum; i++)
//        {
//            if(NeighborNet[i].SendChannl == scanRfChArry[m].channel && NeighborNet[i].SendOption == scanRfChArry[m].option)
//                break;
//        }
//        if(i >= MaxSNIDnum)
//        {
//            if(nnib.RfChannelChangeState == 0)
//            {

//                nnib.RfChannelChangeState = 1;
//                nnib.RfChgChannelRemainTime = 90;       //15S后变更无线信道

//                DefSetInfo.RfChannelInfo.option = scanRfChArry[m].option;
//                DefSetInfo.RfChannelInfo.channel =  scanRfChArry[m].channel;
//                net_printf("ChangeChannelPlcFun <%d,%d>\n",DefSetInfo.RfChannelInfo.option,DefSetInfo.RfChannelInfo.channel);

//            }
//        }

//    }
}


//更新邻居网络主节点地址
void UpdateNbNetInfo(U32 snid, U8 *ccoMac, U8 rfoption, U8 rfchannel)
{
    //记录邻居网络CCOMAC地址
    U8 i;
    U8 findIndx = 0xff;
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(snid == NeighborNet[i].SendSNID)
        {
            findIndx = i;
            break;
        }
        else if(NeighborNet[i].SendSNID == 0xffffffff && findIndx == 0xff)
        {
            findIndx = i;
        }
    }
    if(findIndx != 0xff)
    {
        __memcpy(NeighborNet[i].CCOMacAddr, ccoMac, MACADRDR_LENGTH);
        NeighborNet[i].SendChannl = rfchannel;
        NeighborNet[i].SendOption = rfoption;
        NeighborNet[i]. LifeTime = SNID_LIFE_TIME;
    }

    setRfChlHaveNetFlag(rfoption, rfchannel);

//    net_printf("my mac:%02x%02x%02x%02x%02x%02x\n"
//               , nnib.CCOMacAddr[0], nnib.CCOMacAddr[1], nnib.CCOMacAddr[2]
//               , nnib.CCOMacAddr[3], nnib.CCOMacAddr[4], nnib.CCOMacAddr[5]);
//    net_printf("Nb mac:%02x%02x%02x%02x%02x%02x\n"
//               , NeighborNet[i].CCOMacAddr[0], NeighborNet[i].CCOMacAddr[1], NeighborNet[i].CCOMacAddr[2]
//               , NeighborNet[i].CCOMacAddr[3], NeighborNet[i].CCOMacAddr[4], NeighborNet[i].CCOMacAddr[5]);

    if(comcmpMac(nnib.CCOMacAddr, NeighborNet[i].CCOMacAddr) == TRUE) //本节点mac地址较大
    {
//        net_printf("my mac large\n");
        return;
    }

//    net_printf("my rf:<%d,%d>\n", getHplcOptin(), getHplcRfChannel());
//    net_printf("nb rf:<%d,%d>\n", rfoption, rfchannel);
    if(rfoption == getHplcOptin() &&  rfchannel == getHplcRfChannel())
    {
        StartRfChlChgeForCCO();
    }

}

#endif
//函数名称:获取位图字节大小
//U16 GetNeighborBmpByteNum()
//{
//    U8 	ByteNum;
//    U16 MaxTEI = 0;
//	list_head_t *pos,*node;

//    NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    
//	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
//	{
//	   DiscvryTable = list_entry(pos, NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
//	   if(	DiscvryTable->SNID == GetSNID() && DiscvryTable->My_REV !=0)
//	   {
//	        if(DiscvryTable->NodeTEI > MaxTEI)
//	        {
//	            MaxTEI = DiscvryTable->NodeTEI;
//	        }
//	   }
//    }
    
//    if(MaxTEI)
//    {
//        ByteNum = MaxTEI / 8 + 1;
//    }
//    else
//    {
//        ByteNum = 0;
//    }
//    return ByteNum;

//}



#define PLANNUM						12//5
U8		HistoryPlanRecord[PLANNUM]={0};

//选择代理入网时使用，因为先评估N个周期才入网可认为邻居表是短时间内静态的
typedef enum
{
    //e_LEVEL_LOW_P=0,//等级最低的PCO，信号质量需要过门限值

    e_CCO               =0,                 //CCO，信号质量需要过门限值
    e_LEVEL_LOW_P       = 2,                //等级最低的PCO，信号质量需要过门限值
    e_SNR_BEST_P        = 4,                // 信号质量最好的PCO，即离自己最近的PCO
    e_SNR_BEST_S        = 6,                //信号质量最好的STA
    e_LEVEL_LOW_S       = 8,                //等级最低的STA,信号质量需要过门限值
    e_GAIN_BEST_N       = 10,               //载波低保，不考虑门限

    //增加无线代理选择策略
    e_RF_CCO            = 1,                //无线CCO，信号质量需要过门限值
    e_RF_LEVEL_LOW_P    = 3,                //无线等级最低的PCO，信号质量需要过门限值
    e_RF_SNR_BEST_P     = 5,                //无线信号质量最好的PCO，即离自己最近的PCO
    e_RF_SNR_BEST_S     = 7,                //无线信号质量最好的STA
    e_RF_LEVEL_LOW_S    = 9,                //无线等级最低的STA,信号质量需要过门限值
    e_RF_SNR_BSET_N        = 11,               //无线低保，不考虑门限


}REQ_PLAN_e;
//4-3-3-1-1
typedef enum
{
    e_CCO_times=2,
    e_LEVEL_LOW_P_times=2,//e_LEVEL_LOW_P,对应的尝试次数初始值
    e_SNR_BEST_P_times=2, //e_SNR_BEST_S,//对应的尝试次数初始值
    e_SNR_BEST_S_times=1, //e_SNR_BEST_S, //对应的尝试次数初始值
    e_LEVEL_LOW_S_times=1,//e__LEVEL_LOW_S,对应的尝试次数初始值
  //	e_NO_ACCESSNODE_times=1,//e_NO_ACCESSNODE, //对应的尝试次数初始值

    //无线代理选择策略
    e_RF_CCO_times=2,
    e_RF_LEVEL_LOW_P_times=2,//e_RF_LEVEL_LOW_P,对应的尝试次数初始值
    e_RF_SNR_BEST_P_times=2, //e_RF_SNR_BEST_P ,//对应的尝试次数初始值
    e_RF_SNR_BEST_S_times=1, //e_RF_SNR_BEST_S , //对应的尝试次数初始值
    e_RF_LEVEL_LOW_S_times=1,//e_RF_LEVEL_LOW_S,对应的尝试次数初始值

}REQ_PLAN_CHANCE_e;

void InitHistoryPlanRecord()
{
    HistoryPlanRecord[e_CCO         ] = e_CCO_times;
    HistoryPlanRecord[e_LEVEL_LOW_P ] = e_LEVEL_LOW_P_times;
    HistoryPlanRecord[e_SNR_BEST_P  ] = e_SNR_BEST_P_times;
    HistoryPlanRecord[e_LEVEL_LOW_S ] = e_LEVEL_LOW_S_times;
    HistoryPlanRecord[e_SNR_BEST_S  ] = e_SNR_BEST_S_times;
    HistoryPlanRecord[e_GAIN_BEST_N ] = 1;

    //新增无线代理选择策略
    HistoryPlanRecord[e_RF_CCO        ] = e_RF_CCO_times       ;
    HistoryPlanRecord[e_RF_LEVEL_LOW_P] = e_RF_LEVEL_LOW_P_times;
    HistoryPlanRecord[e_RF_SNR_BEST_P ] = e_RF_SNR_BEST_P_times ;
    HistoryPlanRecord[e_RF_SNR_BEST_S ] = e_RF_SNR_BEST_S_times ;
    HistoryPlanRecord[e_RF_LEVEL_LOW_S] = e_RF_LEVEL_LOW_S_times;
    HistoryPlanRecord[e_RF_SNR_BSET_N ] = 1;

}

U16 SearchBestBackupProxyNode(U16 LASTTEI)
{
    U8	nodeDeep;
    list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    U8 null_addr[6] = {0};

    U16	UPLvsuccRatio=0;
    U16 UPLvTei = INVALID;
    U16	SLvsuccRatio=0;
    U16 SLvTei = INVALID;

    U16 DeepTei = INVALID;


    nodeDeep =  nnib.NodeLevel;

    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->SNID != GetSNID() || 0 == memcmp(null_addr, DiscvryTable->MacAddr, 6))
            continue;

        if(GetProxy() != CCO_TEI && LASTTEI != CCO_TEI )
        {
            if(DiscvryTable->NodeTEI == CCO_TEI
                    && DiscvryTable->HplcInfo.UplinkSuccRatio > BACKUP_SUCC_RATIO )
            {
                return DiscvryTable->NodeTEI;
            }
        }

        //找最低等级的PCO或STA
        if(LASTTEI !=DiscvryTable->NodeTEI
            &&DiscvryTable->Relation == e_UPPERLEVEL
            && DiscvryTable->HplcInfo.UplinkSuccRatio > BACKUP_SUCC_RATIO)
        {
            if( DiscvryTable->NodeDepth < nodeDeep)
            {
                nodeDeep = DiscvryTable->NodeDepth;
                DeepTei = DiscvryTable->NodeTEI;

            }
        }

        //找上一跳PCO中通信成功率最高的
        if(LASTTEI !=DiscvryTable->NodeTEI
                &&DiscvryTable->Relation == e_UPLEVEL
                && DiscvryTable->HplcInfo.UplinkSuccRatio > BACKUP_SUCC_RATIO )
        {
            if(	DiscvryTable->HplcInfo.UplinkSuccRatio> UPLvsuccRatio)
            {
                UPLvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;
                UPLvTei = DiscvryTable->NodeTEI;
            }
        }

        //找同级PCO或STA
        if(LASTTEI !=DiscvryTable->NodeTEI
                && DiscvryTable->Relation == e_SAMELEVEL
                &&DiscvryTable->HplcInfo.UplinkSuccRatio > BACKUP_SUCC_RATIO)
        {
            if(	DiscvryTable->HplcInfo.UplinkSuccRatio > SLvsuccRatio)
            {
                SLvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;
                SLvTei = DiscvryTable->NodeTEI;
            }
        }
    }

    if(DeepTei != INVALID)
        return DeepTei;
    if(UPLvTei != INVALID)
        return UPLvTei;
    if(SLvTei != INVALID)
        return SLvTei;

     return INVALID;
}


#if 0
/**
 * @brief SearchFiveBackupRfProxyNodeAtEvaluate     代理变更时选择无线代理节点
 * @param BackupProxyNode
 * @param Reason
 * @return
 */
__SLOWTEXT U8 SearchFiveBackupRfProxyNodeAtEvaluate(U8 *BackupProxyNode,U8 Reason)
{
    U8 					j = 0;
    U8					result = 0;
    PROXY_INFO_t 		BackupTEI[5];
    U8					ChangeRatio =0;
    U8  nullmac[6]={0};


    list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;


    memset(BackupTEI, 0x00, sizeof(BackupTEI));

    U16 ccoTei = INVALID;
    U8 pcoNodeDeep = nnib.NodeLevel;
    U8 staNodeDeep = nnib.NodeLevel;
    U16 pcoDpTei = INVALID;
    U16 staDpTei = INVALID;

    U8 pcoUpsuccRatio = 0;
    U16 pcoUpTei = INVALID;
    U8 staUpsuccRatio = 0;
    U16 staUpTei = INVALID;
    U8 pcoSlvsuccRatio = 0;
    U16 pcoSlvTei = INVALID;
    U8 staSlvsuccRatio = 0;
    U16 staSlvTei = INVALID;

    U16 mNeiberCnt = 0;
//    extern void printNeighborDiscoveryTableEntry();
//    printNeighborDiscoveryTableEntry();

//    memset(nullmac,0xff,6);
    if(Reason == e_FindBetterProxy )
    {
        ChangeRatio = NICE_SUCC_RATIO;
    }
    else
    {
        ChangeRatio = NICE_SUCC_RATIO-20; //%30为可以通信的链路门限
    }

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)//先找CCO
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->SNID != GetSNID())
        {
            continue;
        }

        mNeiberCnt++;


        if(GetProxy() == DiscvryTable->NodeTEI && nnib.LinkType == e_RF_Link)
            continue;

        if(!(DiscvryTable->HrfInfo.UpRcvRate > ChangeRatio
             && DiscvryTable->HrfInfo.DownRcvRate > ChangeRatio
             && DiscvryTable->NodeDepth < MaxDeepth
             /*&& GetProxy() != DiscvryTable->NodeTEI*/))
        {
            continue;
        }

        //CCO
        if(DiscvryTable->NodeTEI == CCO_TEI && memcmp(DiscvryTable->MacAddr,nullmac,6)!=0 && ccoTei == INVALID)
        {
            ccoTei = DiscvryTable->NodeTEI;
            result = TRUE;
        }

        //找最低等级的PCO
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {
            if( DiscvryTable->NodeDepth < pcoNodeDeep)
            {
                pcoNodeDeep = DiscvryTable->NodeDepth;
                pcoDpTei = DiscvryTable->NodeTEI;
                result = TRUE;
            }
        }

        //找最低等级的STA
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_STA_NODETYPE)
        {
            if( DiscvryTable->NodeDepth < staNodeDeep)
            {
                staNodeDeep = DiscvryTable->NodeDepth;
                staDpTei = DiscvryTable->NodeTEI;
                result = TRUE;
            }
        }

        //找上一跳PCO中通信成功率最高的
            if(DiscvryTable->Relation == e_UPLEVEL
                    && DiscvryTable->NodeType == e_PCO_NODETYPE)
            {

                if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > pcoUpsuccRatio)
                {
                    pcoUpsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                    pcoUpTei = DiscvryTable->NodeTEI;
                    result = TRUE;

                }
            }

        //找上一跳STA
            if(DiscvryTable->Relation == e_UPLEVEL
                    && DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > staUpsuccRatio)
                {
                    staUpsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                    staUpTei = DiscvryTable->NodeTEI;
                    result = TRUE;

                }
            }

        //找同级PCO
            if(DiscvryTable->Relation == e_SAMELEVEL
                    && DiscvryTable->NodeType == e_PCO_NODETYPE)
            {
                if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > pcoSlvsuccRatio)
                {
                    pcoSlvsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                    pcoSlvTei = DiscvryTable->NodeTEI;
                    result = TRUE;

                }
            }

        //找同级STA
        if(nnib.NodeLevel == 1)	// 1跳因为父节点只有1，且有可能听不到同级PCO
        {
            if(DiscvryTable->Relation == e_SAMELEVEL
                    && DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > staSlvsuccRatio)
                {
                    staSlvsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                    staSlvTei = DiscvryTable->NodeTEI;
                    result = TRUE;
                }
            }
        }
    }

    if(result)
    {
        if(ccoTei    != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = ccoTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
        if(pcoDpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = pcoDpTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
        if(staDpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = staDpTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
        if(pcoUpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = pcoUpTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
        if(staUpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = staUpTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
        if(pcoSlvTei != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = pcoSlvTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
        if(staSlvTei != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = staSlvTei;
            BackupTEI[j++].LinkType = e_RF_Link;
        }
    }


    if(result == FALSE&& mNeiberCnt ==1 )
    {
        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            if(j >= 5)
            {
                break;
            }
             DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->SNID == GetSNID())
            {
                if(DiscvryTable->Relation == e_SAMELEVEL
                        /*&& DiscvryTable->UplinkSuccRatio >=INITIAL_SUCC_RATIO
                        && DiscvryTable->DownlinkSuccRatio >=INITIAL_SUCC_RATIO */)
                {
                    BackupTEI[j].ParatTEI = DiscvryTable->NodeTEI;
                    BackupTEI[j++].LinkType = e_RF_Link;
                    result = TRUE;
                }
            }
        }
    }

    __memcpy(BackupProxyNode, (void *)BackupTEI, 10);

    net_printf("Proxy Tei Search : ");
    dump_level_buf(DEBUG_MDL, BackupProxyNode, 10);

    return result;
}

U8 SearchFiveBackupProxyNodeAtEvaluate(PROXY_INFO_t *BackupProxyNode,U8 Reason, U8 proxyNum)
{
    U8 					j = 0;
    U8					result = 0;
    PROXY_INFO_t 		BackupTEI[5];
    U8					ChangeRatio =0;
    U8  nullmac[6]={0};


    list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;


    memset(BackupTEI, 0x00, sizeof(BackupTEI));

    U16 ccoTei = INVALID;
    U8 pcoNodeDeep = nnib.NodeLevel;
    U8 staNodeDeep = nnib.NodeLevel;
    U16 pcoDpTei = INVALID;
    U16 staDpTei = INVALID;

    U8 pcoUpsuccRatio = 0;
    U16 pcoUpTei = INVALID;
    U8 staUpsuccRatio = 0;
    U16 staUpTei = INVALID;
    U8 pcoSlvsuccRatio = 0;
    U16 pcoSlvTei = INVALID;
    U8 staSlvsuccRatio = 0;
    U16 staSlvTei = INVALID;

    U16 mNeiberCnt = 0;

//    memset(nullmac,0xff,6);
    if(Reason == e_FindBetterProxy )
    {
        ChangeRatio = NICE_SUCC_RATIO;
    }
    else
    {
        ChangeRatio = NICE_SUCC_RATIO-20; //%30为可以通信的链路门限
    }

    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)//先找CCO
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->SNID != GetSNID())
        {
            continue;
        }

        mNeiberCnt++;

        if(GetProxy() == DiscvryTable->NodeTEI && nnib.LinkType == e_HPLC_Link)
            continue;

        if(!(DiscvryTable->HplcInfo.UplinkSuccRatio > ChangeRatio
             && DiscvryTable->HplcInfo.DownlinkSuccRatio > ChangeRatio
             && DiscvryTable->NodeDepth < MaxDeepth
             /*&& GetProxy() != DiscvryTable->NodeTEI*/))
        {
            continue;
        }

        //CCO
        if(DiscvryTable->NodeTEI == CCO_TEI && memcmp(DiscvryTable->MacAddr,nullmac,6)!=0 && ccoTei == INVALID)
        {
            ccoTei = DiscvryTable->NodeTEI;
            result = TRUE;
        }

        //找最低等级的PCO
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {
            if( DiscvryTable->NodeDepth < pcoNodeDeep)
            {
                pcoNodeDeep = DiscvryTable->NodeDepth;
                pcoDpTei = DiscvryTable->NodeTEI;
                result = TRUE;
            }
        }

        //找最低等级的STA
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_STA_NODETYPE)
        {
            if( DiscvryTable->NodeDepth < staNodeDeep)
            {
                staNodeDeep = DiscvryTable->NodeDepth;
                staDpTei = DiscvryTable->NodeTEI;
                result = TRUE;
            }
        }

        //找上一跳PCO中通信成功率最高的
            if(DiscvryTable->Relation == e_UPLEVEL
                    && DiscvryTable->NodeType == e_PCO_NODETYPE)
            {

                if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > pcoUpsuccRatio)
                {
                    pcoUpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                    pcoUpTei = DiscvryTable->NodeTEI;
                    result = TRUE;

                }
            }

        //找上一跳STA
            if(DiscvryTable->Relation == e_UPLEVEL
                    && DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > staUpsuccRatio)
                {
                    staUpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                    staUpTei = DiscvryTable->NodeTEI;
                    result = TRUE;

                }
            }

        //找同级PCO
            if(DiscvryTable->Relation == e_SAMELEVEL
                    && DiscvryTable->NodeType == e_PCO_NODETYPE)
            {
                if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > pcoSlvsuccRatio)
                {
                    pcoSlvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                    pcoSlvTei = DiscvryTable->NodeTEI;
                    result = TRUE;

                }
            }

        //找同级STA
        if(nnib.NodeLevel == 1)	// 1跳因为父节点只有1，且有可能听不到同级PCO
        {
            if(DiscvryTable->Relation == e_SAMELEVEL
                    && DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > staSlvsuccRatio)
                {
                    staSlvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                    staSlvTei = DiscvryTable->NodeTEI;
                    result = TRUE;
                }
            }
        }
    }

    if(result)
    {
        if(ccoTei    != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = ccoTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
        if(pcoDpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = pcoDpTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
        if(staDpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = staDpTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
        if(pcoUpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = pcoUpTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
        if(staUpTei  != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = staUpTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
        if(pcoSlvTei != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = pcoSlvTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
        if(staSlvTei != INVALID && j < 5)
        {
            BackupTEI[j].ParatTEI = staSlvTei;
            BackupTEI[j++].LinkType = e_HPLC_Link;
        }
    }

    if(result == FALSE&& mNeiberCnt ==1 )
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            if(j >= 5)
            {
                break;
            }
             DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->SNID == GetSNID())
            {
                if(DiscvryTable->Relation == e_SAMELEVEL
                        && DiscvryTable->HplcInfo.UplinkSuccRatio >=INITIAL_SUCC_RATIO
                        && DiscvryTable->HplcInfo.DownlinkSuccRatio >=INITIAL_SUCC_RATIO )
                {
                    BackupTEI[j].ParatTEI = DiscvryTable->NodeTEI;
                    BackupTEI[j++].LinkType = e_HPLC_Link;
                    result = TRUE;
                }
            }
        }
    }

    __memcpy(BackupProxyNode, (void *)BackupTEI, 10);

    return result;
}
#else

U8  Get_back_proxy(PROXY_INFO_t pcoDpTei, PROXY_INFO_t pcoUpTei, PROXY_INFO_t staUpTei, PROXY_INFO_t staDpTei,
                   PROXY_INFO_t pcoSlvTei, PROXY_INFO_t ccoTei, PROXY_INFO_t staSlvTei, PROXY_INFO_t *BackupTEI, U8 linktype)
{
    U8 result = FALSE;

    if(BackupTEI == NULL)
        return 0;

    //代理变更适配组网类型
    {
        if((linktype == e_HPLC_Link && nnib.NetworkType == e_SINGLE_HRF)  || (linktype == e_RF_Link && nnib.NetworkType == e_SINGLE_HPLC))
        {
            return 0;
        }
    }

    if(!IS_BROADCAST_TEI(ccoTei.ParatTEI) && (ccoTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[0], &ccoTei, sizeof(PROXY_INFO_t));
        result = TRUE;
    }
    if(!IS_BROADCAST_TEI(pcoDpTei.ParatTEI) && (pcoDpTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[1], &pcoDpTei, sizeof(PROXY_INFO_t));
        result = TRUE;
    }
    if(!IS_BROADCAST_TEI(staDpTei.ParatTEI) && (staDpTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[2], &staDpTei, sizeof(PROXY_INFO_t) );
        result = TRUE;
    }
    if(!IS_BROADCAST_TEI(pcoUpTei.ParatTEI) && (pcoUpTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[3], &pcoUpTei, sizeof(PROXY_INFO_t));
        result = TRUE;
    }
    if(!IS_BROADCAST_TEI(staUpTei.ParatTEI) && (staUpTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[4], &staUpTei, sizeof(PROXY_INFO_t));
        result = TRUE;
    }
    if(!IS_BROADCAST_TEI(pcoSlvTei.ParatTEI) && (pcoSlvTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[5], &pcoSlvTei, sizeof(PROXY_INFO_t));
        result = TRUE;
    }
    if(!IS_BROADCAST_TEI(staSlvTei.ParatTEI) && (staSlvTei.LinkType == linktype))
    {
        __memcpy(&BackupTEI[6], &staSlvTei, sizeof(PROXY_INFO_t));
        result = TRUE;
    }

    return result;
}

/**
 * @brief   本轮代理变更尝试过的代理列表
 * 
 */
PROXY_INFO_t g_Proxy_Try_list[5];

/**
 * @brief   清空本轮已经尝试过的代理列表
 * 
 */
void clean_proxy_try_list()
{
    memset(g_Proxy_Try_list, 0x00, sizeof(g_Proxy_Try_list));
}

/**
 * @brief               判断当前选择的备选代理本轮是否已经尝试过
 *  
 * @param tei           备选代理TEI
 * @param linkType      备选代理链路类型
 * @return U8           TRUE：本轮已经试过， FALSE： 本轮未尝试
 */
U8 check_proxy_is_try(U16 tei, U8 linkType)
{
    U8 i;
    for (i = 0; i < NELEMS(g_Proxy_Try_list); i++)
    {
        if(g_Proxy_Try_list[i].ParatTEI == 0)
            continue;

        if (g_Proxy_Try_list[i].ParatTEI == tei && g_Proxy_Try_list[i].LinkType == linkType)
        {
            return TRUE;
        }
    }

    return FALSE;
}
/**
 * @brief               将尝试过的代理添加到列表中
 * 
 * @param tei           正在尝试变更的代理TEI
 * @param linkType      正在尝试变更的代理链路类型
 * @return U8           是否添加成功
 */
U8 inser_proxy_try_list(U16 tei, U8 linkType)
{
    U8 i;
    for (i = 0; i < NELEMS(g_Proxy_Try_list); i++)
    {
        if(g_Proxy_Try_list[i].ParatTEI == 0)
        {
            g_Proxy_Try_list[i].ParatTEI = tei;
            g_Proxy_Try_list[i].LinkType = linkType;
            return TRUE;
        }
    }

    return FALSE;
}
/**
 * @brief       //根据条件，判断当前选择代理是否符合
 * 
 * @return U8   //TRUE:符合， FALSE:不符合
 */
U8 check_Proxy_is_nice(U8 Reason, U8 level, U8 LinkType)
{
    //不是拓扑优化，不根据代理原因判断
    if(Reason != e_FindBetterProxy)
    {
        return TRUE; 
    }

    //如果同链路类型代理或者是载波入网的节点，拓扑优化必须为层级降低优化
    if(nnib.LinkType == LinkType || nnib.LinkType == e_HPLC_Link)
    {
        if(level < (nnib.NodeLevel - 1))
        {
            return TRUE;
        }
    }
    else if(nnib.LinkType == e_RF_Link)     //如果无线入网节点，选择载波代理，可以选择与当前代理同级的节点
    {
        if(level <= (nnib.NodeLevel - 1))
        {
            return TRUE;
        }
    }

    return FALSE;
}
/**
 * @brief SearchFiveBackupProxyNodeAtEvaluate_dual  代理变更选择备用代理节点
 * @param BackupProxyNode                           备用代理节点信息存储地址
 * @param Reason                                    代理变更原因
 * @return
 */
U8 SearchFiveBackupProxyNodeAtEvaluate(PROXY_INFO_t *BackupProxyNode,U8 Reason, U8 proxyNum)
{
    U8 					j = 0;
    U8					result = 0;
    PROXY_INFO_t 		BackupTEI[7];
    U8					ChangeRatio =0;
    U8					RfChangeRatio =0;
    U8  nullmac[6]={0};


    list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    if(Reason != e_FialToTry)
    {
        clean_proxy_try_list();
    }
    else
    {
        Reason = ChangeReq_t.Reason;
    }

    
    U8 Rf_Ratio_wht_rft = 2;  //代理选择时，无线成功率加权系数，成功率需要大于载波成功率的2倍

    memset(BackupTEI, 0x00, sizeof(BackupTEI));

    PROXY_INFO_t ccoTei;
    ccoTei.ParatTEI = BROADCAST_TEI;
    U16 ccoUpsuccRatio = 0;

    U8 pcoNodeDeep = nnib.NodeLevel;
    PROXY_INFO_t pcoDpTei;
    pcoDpTei.ParatTEI = BROADCAST_TEI;
    U16 pcoDpsuccRatio = 0;   //增加成功率为了确定相同节点载波和无线哪个链路通信质量好

    U8 staNodeDeep = nnib.NodeLevel;
    PROXY_INFO_t staDpTei;
    staDpTei.ParatTEI = BROADCAST_TEI;
    U16 staDpsuccRatio = 0;   //增加成功率为了确定相同节点载波和无线哪个链路通信质量好

    U16 pcoUpsuccRatio = 0;
    PROXY_INFO_t pcoUpTei;
    pcoUpTei.ParatTEI = BROADCAST_TEI;

    U16 staUpsuccRatio = 0;
    PROXY_INFO_t staUpTei;
    staUpTei.ParatTEI = BROADCAST_TEI;

    U16 pcoSlvsuccRatio = 0;
    PROXY_INFO_t pcoSlvTei;
    pcoSlvTei.ParatTEI = BROADCAST_TEI;

    U16 staSlvsuccRatio = 0;
    PROXY_INFO_t staSlvTei;
    staSlvTei.ParatTEI= BROADCAST_TEI;

    U16 mHPLCNeiberCnt = 0;
    U16 mHRFNeiberCnt = 0;

//    memset(nullmac,0xff,6);
    if(Reason == e_FindBetterProxy )
    {
        ChangeRatio = NICE_SUCC_RATIO;
        RfChangeRatio = 75;
    }
    else
    {
        ChangeRatio = NICE_SUCC_RATIO-20; //%30为可以通信的链路门限
        RfChangeRatio = 50;
    }

    U8 findPlcPco  = FALSE;
    U8 findHrfPco  = FALSE;


    //从载波邻居表中选择新的代理
    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)//先找CCO
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->SNID != GetSNID())
        {
            continue;
        }

        if(0 == memcmp(DiscvryTable->MacAddr, nullmac, 6))
        {
            continue;
        }

        mHPLCNeiberCnt++;

        if(GetProxy() == DiscvryTable->NodeTEI && nnib.LinkType == e_HPLC_Link)
            continue;

        if(!check_Proxy_is_nice(Reason, DiscvryTable->NodeDepth, e_HPLC_Link))
        {
            continue;
        }

        if(check_proxy_is_try(DiscvryTable->NodeTEI, e_HPLC_Link))
        {
            continue;
        }

        if(!(DiscvryTable->HplcInfo.UplinkSuccRatio > ChangeRatio
             && DiscvryTable->HplcInfo.DownlinkSuccRatio > ChangeRatio
             && DiscvryTable->NodeDepth < MaxDeepth
             /*&& GetProxy() != DiscvryTable->NodeTEI*/))
        {
            continue;
        }

        //如果是网络拓扑优化，需要参考上个路由周期的通信成功率备份,减少代理频繁的来回变更
        if(Reason == e_FindBetterProxy)
        {
            if(!(DiscvryTable->HplcInfo.LastUplinkSuccRatio > ChangeRatio
             && DiscvryTable->HplcInfo.LastDownlinkSuccRatio > ChangeRatio))
             {
                continue;
             }
        }

        //CCO
        if(DiscvryTable->NodeTEI == CCO_TEI && memcmp(DiscvryTable->MacAddr,nullmac,6)!=0 && ccoTei.ParatTEI == BROADCAST_TEI)
        {
            ccoTei.ParatTEI = DiscvryTable->NodeTEI;
            ccoTei.LinkType = e_HPLC_Link;
            ccoUpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
            findPlcPco = TRUE;

            continue;
        }

        //找最低等级的PCO
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {
            if( DiscvryTable->NodeDepth < pcoNodeDeep)
            {
                pcoNodeDeep = DiscvryTable->NodeDepth;
                pcoDpTei.ParatTEI = DiscvryTable->NodeTEI;
                pcoDpTei.LinkType = e_HPLC_Link;
                pcoDpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                findPlcPco = TRUE;
            }
        }

        //找最低等级的STA
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_STA_NODETYPE)
        {
            if( DiscvryTable->NodeDepth < staNodeDeep)
            {
                staNodeDeep = DiscvryTable->NodeDepth;
                staDpTei.ParatTEI = DiscvryTable->NodeTEI;
                staDpTei.LinkType = e_HPLC_Link;
                staDpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                findPlcPco = TRUE;
            }
        }

        //找上一跳PCO中通信成功率最高的
        if(DiscvryTable->Relation == e_UPLEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {

            if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > pcoUpsuccRatio)
            {
                pcoUpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                pcoUpTei.ParatTEI = DiscvryTable->NodeTEI;
                pcoUpTei.LinkType = e_HPLC_Link;
                findPlcPco = TRUE;

            }
        }

        //找上一跳STA
        if(DiscvryTable->Relation == e_UPLEVEL
                && DiscvryTable->NodeType == e_STA_NODETYPE)
        {
            if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > staUpsuccRatio)
            {
                staUpsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                staUpTei.ParatTEI = DiscvryTable->NodeTEI;
                staUpTei.LinkType = e_HPLC_Link;
                findPlcPco = TRUE;

            }
        }

        //找同级PCO
        if(DiscvryTable->Relation == e_SAMELEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {
            if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > pcoSlvsuccRatio)
            {
                pcoSlvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                pcoSlvTei.ParatTEI = DiscvryTable->NodeTEI;
                pcoSlvTei.LinkType = e_HPLC_Link;
                findPlcPco = TRUE;

            }
        }

        //找同级STA
        if(nnib.NodeLevel == 1)	// 1跳因为父节点只有1，且有可能听不到同级PCO
        {
            if(DiscvryTable->Relation == e_SAMELEVEL
                    && DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if(  (DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio) > staSlvsuccRatio)
                {
                    staSlvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio+DiscvryTable->HplcInfo.DownlinkSuccRatio;
                    staSlvTei.ParatTEI = DiscvryTable->NodeTEI;
                    staSlvTei.LinkType = e_HPLC_Link;
                    findPlcPco = TRUE;
                }
            }
        }
    }

    if(findPlcPco)
    {
        if(Get_back_proxy(pcoDpTei, pcoUpTei, staUpTei, staDpTei, pcoSlvTei, ccoTei, staSlvTei, BackupTEI, e_HPLC_Link))
        {
            net_printf("Hplc-P\n");
            result = TRUE;
        }
    }

    //增加权重
    ccoUpsuccRatio *= Rf_Ratio_wht_rft;
    pcoDpsuccRatio *= Rf_Ratio_wht_rft;
    staDpsuccRatio *= Rf_Ratio_wht_rft;
    pcoUpsuccRatio *= Rf_Ratio_wht_rft;
    staUpsuccRatio *= Rf_Ratio_wht_rft;
    pcoSlvsuccRatio *= Rf_Ratio_wht_rft;
    staSlvsuccRatio *= Rf_Ratio_wht_rft;


    //从无线邻居表中选择新的代理
    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)//先找CCO
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->SNID != GetSNID())
        {
            continue;
        }

        if(0 == memcmp(DiscvryTable->MacAddr, nullmac, 6))
        {
            continue;
        }

        mHRFNeiberCnt++;


        if(GetProxy() == DiscvryTable->NodeTEI && nnib.LinkType == e_RF_Link)
            continue;

        if(!check_Proxy_is_nice(Reason, DiscvryTable->NodeDepth, e_RF_Link))
        {
            continue;
        }

        if(check_proxy_is_try(DiscvryTable->NodeTEI, e_RF_Link))
        {
            continue;
        }

        if(!(DiscvryTable->HrfInfo.UpRcvRate > RfChangeRatio
             && DiscvryTable->HrfInfo.DownRcvRate > RfChangeRatio
             && DiscvryTable->NodeDepth < MaxDeepth
             /*&& GetProxy() != DiscvryTable->NodeTEI*/))
        {
            continue;
        }

        //如果是网络拓扑优化，需要参考上个路由周期的通信成功率备份,减少代理频繁的来回变更
        if(Reason == e_FindBetterProxy)
        {
            if(!(DiscvryTable->HrfInfo.LUpRcvRate > RfChangeRatio
             && DiscvryTable->HrfInfo.LDownRcvRate > RfChangeRatio))
             {
                continue;
             }
        }

        //CCO
        if(DiscvryTable->NodeTEI == CCO_TEI && memcmp(DiscvryTable->MacAddr,nullmac,6)!=0)
        {
            //如果载波和无线邻居表中都有CCO 比较接收成功率，选择通信质量好的链路
            if(ccoTei.ParatTEI != BROADCAST_TEI)
            {
                if((DiscvryTable->HrfInfo.DownRcvRate + DiscvryTable->HrfInfo.UpRcvRate) > ccoUpsuccRatio)
                    ccoTei.LinkType = e_RF_Link;;
            }
            else
            {
                ccoTei.ParatTEI = DiscvryTable->NodeTEI;
                ccoTei.LinkType = e_RF_Link;
                findHrfPco = TRUE;
            }

            continue;
        }

        //找最低等级的PCO
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {
            //载波和无线邻居表中有相同节点，比较接收率。选择通信质量好的链路
            if(DiscvryTable->NodeTEI == pcoDpTei.ParatTEI)
            {
                if((DiscvryTable->HrfInfo.DownRcvRate + DiscvryTable->HrfInfo.UpRcvRate) > pcoDpsuccRatio)
                {
                    pcoDpTei.LinkType = e_RF_Link;
                }
            }else if( DiscvryTable->NodeDepth < pcoNodeDeep)
            {
                pcoNodeDeep = DiscvryTable->NodeDepth;
                pcoDpTei.ParatTEI = DiscvryTable->NodeTEI;
                pcoDpTei.LinkType = e_RF_Link;
                findHrfPco = TRUE;
            }
        }

        //找最低等级的STA
        if(DiscvryTable->Relation == e_UPPERLEVEL
                && DiscvryTable->NodeType == e_STA_NODETYPE)
        {
            if(DiscvryTable->NodeTEI == staDpTei.ParatTEI)
            {
                if((DiscvryTable->HrfInfo.DownRcvRate + DiscvryTable->HrfInfo.UpRcvRate) > staDpsuccRatio)
                {
                    staDpTei.LinkType = e_RF_Link;
                }
            }
            else if( DiscvryTable->NodeDepth < staNodeDeep)
            {
                staNodeDeep = DiscvryTable->NodeDepth;
                staDpTei.ParatTEI = DiscvryTable->NodeTEI;
                staDpTei.LinkType = e_RF_Link;
                findHrfPco = TRUE;
            }
        }

        //找上一跳PCO中通信成功率最高的
        if(DiscvryTable->Relation == e_UPLEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {

            if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > pcoUpsuccRatio)
            {
                pcoUpsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                pcoUpTei.ParatTEI = DiscvryTable->NodeTEI;
                pcoUpTei.LinkType = e_RF_Link;
                findHrfPco = TRUE;

            }
        }

        //找上一跳STA
        if(DiscvryTable->Relation == e_UPLEVEL
                && DiscvryTable->NodeType == e_STA_NODETYPE)
        {
            if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > staUpsuccRatio)
            {
                staUpsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                staUpTei.ParatTEI = DiscvryTable->NodeTEI;
                staUpTei.LinkType = e_RF_Link;
                findHrfPco = TRUE;

            }
        }

        //找同级PCO
        if(DiscvryTable->Relation == e_SAMELEVEL
                && DiscvryTable->NodeType == e_PCO_NODETYPE)
        {
            if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > pcoSlvsuccRatio)
            {
                pcoSlvsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                pcoSlvTei.ParatTEI = DiscvryTable->NodeTEI;
                pcoSlvTei.LinkType = e_RF_Link;
                findHrfPco = TRUE;

            }
        }

        //找同级STA
        if(nnib.NodeLevel == 1)	// 1跳因为父节点只有1，且有可能听不到同级PCO
        {
            if(DiscvryTable->Relation == e_SAMELEVEL
                    && DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if(  (DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate) > staSlvsuccRatio)
                {
                    staSlvsuccRatio = DiscvryTable->HrfInfo.UpRcvRate+DiscvryTable->HrfInfo.DownRcvRate;
                    staSlvTei.ParatTEI = DiscvryTable->NodeTEI;
                    staSlvTei.LinkType = e_RF_Link;
                    findHrfPco = TRUE;
                }
            }
        }
    }

    if(findHrfPco)
    {
        if(Get_back_proxy(pcoDpTei, pcoUpTei, staUpTei, staDpTei, pcoSlvTei, ccoTei, staSlvTei, BackupTEI, e_RF_Link))
        {
            net_printf("Hrf-P\n");
            result = TRUE;
        }
    }

    net_printf("result :%d, PLCNbCnt:%d, RfNbCnt:%d\n", result, mHPLCNeiberCnt, mHRFNeiberCnt);


    if(result == FALSE&& mHPLCNeiberCnt ==1 )
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            if(j >= NELEMS(BackupTEI))
            {
                break;
            }
             DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->SNID == GetSNID())
            {
                if(DiscvryTable->Relation == e_SAMELEVEL
                        && DiscvryTable->HplcInfo.UplinkSuccRatio >=INITIAL_SUCC_RATIO
                        && DiscvryTable->HplcInfo.DownlinkSuccRatio >=INITIAL_SUCC_RATIO )
                {
                    BackupTEI[j].ParatTEI = DiscvryTable->NodeTEI;
                    BackupTEI[j++].LinkType = e_HPLC_Link;
                    result = TRUE;
                }
            }
        }
    }

    if(result == FALSE&& mHRFNeiberCnt ==1 )
    {
        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            if(j >= NELEMS(BackupTEI))
            {
                break;
            }
             DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->SNID == GetSNID())
            {
                if(DiscvryTable->Relation == e_SAMELEVEL
                        /*&& DiscvryTable->HplcInfo.UplinkSuccRatio >=INITIAL_SUCC_RATIO
                        && DiscvryTable->HplcInfo.DownlinkSuccRatio >=INITIAL_SUCC_RATIO */)
                {
                    BackupTEI[j].ParatTEI = DiscvryTable->NodeTEI;
                    BackupTEI[j++].LinkType = e_RF_Link;
                    result = TRUE;
                }
            }
        }
    }

    if(result)
    {
        U8 i;
        j = 0;
        for (i = 0; i < NELEMS(BackupTEI); i++)
        {
            if(BackupTEI[i].ParatTEI == 0)
                continue;

            // __memcpy(BackupProxyNode + j, &BackupTEI[i], sizeof(PROXY_INFO_t));
            BackupProxyNode[j].ParatTEI =BackupTEI[i].ParatTEI;
            BackupProxyNode[j].LinkType =BackupTEI[i].LinkType;

            if(j == 0)
            {
                inser_proxy_try_list(BackupTEI[i].ParatTEI, BackupTEI[i].LinkType);
            }

            if(++j >= proxyNum)
                break;
        }
        dump_level_buf(DEBUG_MDL, (U8 *)BackupProxyNode, sizeof(PROXY_INFO_t)*j);
    }

    // __memcpy(BackupProxyNode, (void *)BackupTEI, 10);

    return result;
}

#endif


static U8 CheckRepeatBackUp(PROXY_INFO_t *BackupTEI, U16 TEI)
{
    U8 i;
    for(i=0;i<5;i++)
    {
        if(BackupTEI[i].ParatTEI == TEI)
        {
            return FALSE;
        }
    }
    return TRUE;
}


U8 GetPLanAinfo() //CCO 是否尝试完成
{
    return HistoryPlanRecord[1];
}


U8  ForAssociateReqSearchProxyNode(U16 *BackupProxyNode, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *Pdiscry)
{

//    U8  selectPlan,firstPlan=0;
    U16 i;
    PROXY_INFO_t BackupTEI[5];
    U8  nullmac[6]={0};
    U8 backSeq = 0;


    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    U8 zeroPlanCnt=0;

    net_printf("---------HistoryPlanRecord=");
    for(i = 0; i < PLANNUM; i++)
    {
        net_printf("%d-", HistoryPlanRecord[i]);
        if(HistoryPlanRecord[i] == 0)
        {
            zeroPlanCnt++;
        }
    }
    net_printf("---------\n");

    //超过四个方案尝试失败，或者是载波或者是无线低保尝试失败。
    if(zeroPlanCnt > 4 || HistoryPlanRecord[e_GAIN_BEST_N] == 0 || HistoryPlanRecord[e_RF_SNR_BSET_N] == 0)
    {
        net_printf("planerr:%d,%d,%d\n", zeroPlanCnt, HistoryPlanRecord[e_GAIN_BEST_N],  HistoryPlanRecord[e_RF_SNR_BSET_N]);
        return FALSE;
    }



//    for(i=0;i<PLANNUM;i++)
//    {
//        if(HistoryPlanRecord[i]==0) //当某一方案尝试失败后，切换到下一个父节点加入网络
//        {
//            firstPlan = (firstPlan+1)%PLANNUM;
//            if(i==PLANNUM-1)return FALSE;
//        }
//        else
//        {
//            break;
//        }
//    }
//    selectPlan = firstPlan;

    //选定首选方案后为节点找5个代理
//    net_printf("---------firstPlan=%d------------\n",firstPlan);


    PROXY_INFO_t TmpBackupTEI[PLANNUM];
    memset(TmpBackupTEI, 0, sizeof(TmpBackupTEI));
    memset(BackupTEI, 0, sizeof(BackupTEI));

    S8 Pco_lowsnr=0x7f, Sta_lowsnr=0x7f;
    U8 Pco_nodeDeep = 0x0f; S8 Pco_lowsnrForDeep=128;
    U8 Sta_nodeDeep = 0x0f; S8 Sta_lowsnrForDeep=128;

    S8 Gain_ave = 0x7f;

    //低保
    S8 Gain_best = 0x7f;

    //选择载波代理节点
    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if( GetSNID() == DiscvryTable->SNID && memcmp( DiscvryTable->MacAddr,nullmac,6)!=0)
        {
            Gain_ave = DiscvryTable->HplcInfo.GAIN/ DiscvryTable->HplcInfo.RecvCount;

            //低保
            if(Gain_ave < Gain_best)
            {
                TmpBackupTEI[e_GAIN_BEST_N].ParatTEI =  DiscvryTable->NodeTEI;
                TmpBackupTEI[e_GAIN_BEST_N].LinkType =  e_HPLC_Link;
                Gain_best = Gain_ave;
            }

//            net_printf("tei:%03x, Gain_ave:%d, NodeDepth:%d\n", DiscvryTable->NodeTEI, Gain_ave, DiscvryTable->NodeDepth);
            if(!(Gain_ave <=LOWST_RXGAIN &&  DiscvryTable->NodeDepth < MaxDeepth))
            {
                continue;
            }

            //CCO
            if( DiscvryTable->NodeType == e_CCO_NODETYPE)
            {
                TmpBackupTEI[e_CCO].ParatTEI =  DiscvryTable->NodeTEI;
                TmpBackupTEI[e_CCO].LinkType =  e_HPLC_Link;
            }

            //e_LEVEL_LOW_P
            if( DiscvryTable->NodeType == e_PCO_NODETYPE)
            {
                if(  DiscvryTable->NodeDepth < Pco_nodeDeep ||
                     (  DiscvryTable->NodeDepth == Pco_nodeDeep && Gain_ave <Pco_lowsnrForDeep) )
                {
                    Pco_nodeDeep =  DiscvryTable->NodeDepth;
                    Pco_lowsnrForDeep = Gain_ave;
                    TmpBackupTEI[e_LEVEL_LOW_P].ParatTEI = DiscvryTable->NodeTEI;
                    TmpBackupTEI[e_LEVEL_LOW_P].LinkType =  e_HPLC_Link;
                }
            }

            //e_SNR_BEST_P
            if(DiscvryTable->NodeType == e_PCO_NODETYPE)
            {
                if( Gain_ave< Pco_lowsnr)
                {
                    Pco_lowsnr = Gain_ave;
                    TmpBackupTEI[e_SNR_BEST_P].ParatTEI = DiscvryTable->NodeTEI;
                    TmpBackupTEI[e_SNR_BEST_P].LinkType =  e_HPLC_Link;
                }
            }

            //e_LEVEL_LOW_S
            if(DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if( DiscvryTable->NodeDepth < Sta_nodeDeep ||
                        (  DiscvryTable->NodeDepth == Sta_nodeDeep && Gain_ave <Sta_lowsnrForDeep) )
                {
                    Sta_nodeDeep = DiscvryTable->NodeDepth;
                    Sta_lowsnrForDeep = Gain_ave;
                    TmpBackupTEI[e_LEVEL_LOW_S].ParatTEI = DiscvryTable->NodeTEI;
                    TmpBackupTEI[e_LEVEL_LOW_S].LinkType =  e_HPLC_Link;

                }
            }

            //e_SNR_BEST_S
            if(  GetSNID() == DiscvryTable->SNID && memcmp(DiscvryTable->MacAddr,nullmac,6)!=0 )
            {
                if(DiscvryTable->NodeType == e_STA_NODETYPE)
                {
                    if( Gain_ave < Sta_lowsnr)
                    {
                        Sta_lowsnr = Gain_ave;
                        TmpBackupTEI[e_SNR_BEST_S].ParatTEI = DiscvryTable->NodeTEI;
                        TmpBackupTEI[e_SNR_BEST_S].LinkType =  e_HPLC_Link;
                    }
                }
            }
        }
    }


    Pco_lowsnr=RF_RSSI_INVAL;
    Sta_lowsnr=RF_RSSI_INVAL;
    Pco_nodeDeep = 0x0f;
    Pco_lowsnrForDeep=RF_RSSI_INVAL;
    Sta_nodeDeep = 0x0f;
    Sta_lowsnrForDeep=RF_RSSI_INVAL;

    //无线低保
    S8 snr_best = RF_RSSI_INVAL;
    S8 rssi_best = RF_RSSI_INVAL;

    //选择无线代理节点
    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if( GetSNID() == DiscvryTable->SNID && memcmp( DiscvryTable->MacAddr,nullmac,6)!=0)
        {

            //无线低保
            if(snr_best < DiscvryTable->HrfInfo.DownSnr || ((snr_best == DiscvryTable->HrfInfo.DownSnr) && (rssi_best < DiscvryTable->HrfInfo.DownRssi)))
            {
                TmpBackupTEI[e_RF_SNR_BSET_N].ParatTEI =  DiscvryTable->NodeTEI;
                TmpBackupTEI[e_RF_SNR_BSET_N].LinkType =  e_RF_Link;

                snr_best  = DiscvryTable->HrfInfo.DownSnr;
                rssi_best = DiscvryTable->HrfInfo.DownRssi;
            }

//            net_printf("tei:%03x, rssi:%d, snr:%d, deep:%d\n", DiscvryTable->NodeTEI, DiscvryTable->HrfInfo.DownRssi,DiscvryTable->HrfInfo.DownSnr, DiscvryTable->NodeDepth);
            if(!(DiscvryTable->HrfInfo.DownRssi > LOWST_RXRSSI &&  DiscvryTable->NodeDepth < MaxDeepth))
            {
                continue;
            }

            //CCO
            if( DiscvryTable->NodeType == e_CCO_NODETYPE)
            {
                TmpBackupTEI[e_RF_CCO].ParatTEI =  DiscvryTable->NodeTEI;
                TmpBackupTEI[e_RF_CCO].LinkType =  e_RF_Link;
            }

            //e_LEVEL_LOW_P
            if( DiscvryTable->NodeType == e_PCO_NODETYPE)
            {
                if(  DiscvryTable->NodeDepth < Pco_nodeDeep ||
                     (  DiscvryTable->NodeDepth == Pco_nodeDeep && DiscvryTable->HrfInfo.DownRssi  > Pco_lowsnrForDeep) )
                {
                    Pco_nodeDeep =  DiscvryTable->NodeDepth;
                    Pco_lowsnrForDeep = DiscvryTable->HrfInfo.DownRssi;
                    TmpBackupTEI[e_RF_LEVEL_LOW_P].ParatTEI = DiscvryTable->NodeTEI;
                    TmpBackupTEI[e_RF_LEVEL_LOW_P].LinkType =  e_RF_Link;

                }
            }

            //e_SNR_BEST_P
            if(DiscvryTable->NodeType == e_PCO_NODETYPE)
            {
                if( DiscvryTable->HrfInfo.DownSnr > Pco_lowsnr)
                {
                    Pco_lowsnr = DiscvryTable->HrfInfo.DownSnr;
                    TmpBackupTEI[e_RF_SNR_BEST_P].ParatTEI = DiscvryTable->NodeTEI;
                    TmpBackupTEI[e_RF_SNR_BEST_P].LinkType =  e_RF_Link;
                }
            }

            //e_LEVEL_LOW_S
            if(DiscvryTable->NodeType == e_STA_NODETYPE)
            {
                if( DiscvryTable->NodeDepth < Sta_nodeDeep ||
                        (  DiscvryTable->NodeDepth == Sta_nodeDeep && DiscvryTable->HrfInfo.DownRssi > Sta_lowsnrForDeep) )
                {
                    Sta_nodeDeep = DiscvryTable->NodeDepth;
                    Sta_lowsnrForDeep = DiscvryTable->HrfInfo.DownRssi;
                    TmpBackupTEI[e_RF_LEVEL_LOW_S].ParatTEI = DiscvryTable->NodeTEI;
                    TmpBackupTEI[e_RF_LEVEL_LOW_S].LinkType =  e_RF_Link;

                }
            }

            //e_SNR_BEST_S
            if(  GetSNID() == DiscvryTable->SNID && memcmp(DiscvryTable->MacAddr,nullmac,6)!=0 )
            {
                if(DiscvryTable->NodeType == e_STA_NODETYPE)
                {
                    if( DiscvryTable->HrfInfo.DownSnr > Sta_lowsnr)
                    {
                        Sta_lowsnr = DiscvryTable->HrfInfo.DownSnr;
                        TmpBackupTEI[e_RF_SNR_BEST_S].ParatTEI = DiscvryTable->NodeTEI;
                        TmpBackupTEI[e_RF_SNR_BEST_S].LinkType =  e_RF_Link;
                    }
                }
            }
        }
    }



    dump_buf((U8*)TmpBackupTEI, sizeof(TmpBackupTEI));


    for(i=0;i<PLANNUM;i++)
    {
        if(backSeq >= 5)
            break;

        if(TmpBackupTEI[i].ParatTEI != 0 && HistoryPlanRecord[i] > 0)   //有尝试次数的方案才会尝试入网。
        {
            if(CheckRepeatBackUp(BackupTEI, TmpBackupTEI[i].ParatTEI))
            {
                if(backSeq == 0)
                {
                    HistoryPlanRecord[i]--; //被选定的入网方案，尝试次数减1
                    net_printf("---------firstPlan=%d------------\n",i);
                }
                __memcpy(&BackupTEI[backSeq++], &TmpBackupTEI[i], sizeof(PROXY_INFO_t));
            }
        }

//        net_printf("selectPlan=%d\n",selectPlan);
//        selectPlan =(selectPlan+1)%PLANNUM;
    }

//    HistoryPlanRecord[firstPlan]--;

    if(BackupTEI[0].ParatTEI != 0)
    {
        DiscvryTable = GetNeighborByTEI(BackupTEI[0].ParatTEI, BackupTEI[0].LinkType);
        if(DiscvryTable == NULL)
            return FALSE;

        __memcpy(Pdiscry,DiscvryTable,sizeof(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t));
        dump_buf((U8*)BackupTEI, sizeof(BackupTEI));
        __memcpy(BackupProxyNode,BackupTEI,sizeof(BackupTEI));
        return TRUE;
    }

    return FALSE;

}


//////////////提供给上层访问的函数///////////////

/*需要检查当前设置和自身状态是否一致*/
void	SetMacAddrRequest(U8 *MacAddr, U8 DeviceType,U8 MacType)
{
    U16 seed=0;

    __memcpy(nnib.MacAddr, MacAddr, 6);
    ChangeMacAddrOrder(nnib.MacAddr);

#if defined(STATIC_MASTER)
    __memcpy(nnib.CCOMacAddr, nnib.MacAddr, 6);


#elif defined(ZC3750STA)
    nnib.MacAddrType = e_MAC_METER_ADDR;
#ifdef TH2CJQ
    nnib.DeviceType = DeviceType;
#endif
    if(nnib.DeviceType == e_UNKW) //falsh中如果没有存储型号，使用过零判断一次
    {
        if((nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC)>=1)
        {
            nnib.PowerType =TRUE;
            app_printf("----this is strong power--\n");
            nnib.DeviceType = (nnib.PhasebitB+nnib.PhasebitC) >= 1? e_3PMETER: e_METER;
        }
        else
        {
            nnib.DeviceType = e_METER;
            nnib.PowerType =FALSE;
        }
    }
#elif defined(ZC3750CJQ2)

    nnib.DeviceType = DeviceType;
    nnib.MacAddrType = MacType;
    net_printf("DeviceType=%d.MacAddrType=%d\n",DeviceType,MacType);

 #endif

    seed = (nnib.MacAddr[0]*2)+(nnib.MacAddr[1]*3)+(nnib.MacAddr[2]*4)+(nnib.MacAddr[3]*5)+(nnib.MacAddr[4]*6)+(nnib.MacAddr[5]*7);
    if(seed)
    {
        srand(seed);
    }
    else
    {
        srand(cpu_get_cycle());
    }

#if defined(STATIC_MASTER)
    SetPib(rand() & 0x00FFFFFF,CCO_TEI);

#else
    nnib.ManageMsgSeqNum = (rand() & 0xFFFFFFFF);
    net_printf("MMsgSnum: %08x\n", nnib.ManageMsgSeqNum);
#endif

    g_MsduSequenceNumber = rand();
    nnib.AssociaRandomNum = rand();


}


void	QueryMacAddrRequest()
{


}

#if defined(STATIC_MASTER)


void DeviceListHeaderINIT()
{
	INIT_LIST_HEAD(&DeviceListHeader.link);
	DeviceListHeader.nr = 0;
	DeviceListHeader.thr = MAX_WHITELIST_NUM;	
}

U8 CheckTheMacAddrLegal(U8 *pMacAddress,U8 devicetype,U8 addrtype,U16 *WhiteSeq)
{

    //<<< test
//    return ASSOC_SUCCESS;

    U16 i;
    U8  MACADDR[6];
    U8  Result;
    U8  null_addr[6] = {0};
    
    *WhiteSeq = 0xFFF;
    Result = NOTIN_WHITELIST;
    //过滤全00节点
    if(memcmp(null_addr, pMacAddress, 6) == 0)
    {
        Result = NOTIN_WHITELIST;
        return Result;
    }
    if(devicetype == e_RELAY || (devicetype==e_CJQ_2 && addrtype!=e_MAC_METER_ADDR) ||	(devicetype==e_CJQ_1 && addrtype!=e_MAC_METER_ADDR ))
    {
        Result = ASSOC_SUCCESS;
    }
    if(FlashInfo_t.usJZQRecordNum==0)
    {
         Result = NO_WHITELIST;
    }
    if(  STARegisterFlag == TRUE || app_ext_info.func_switch.WhitelistSWC != 1)
    {
        Result = ASSOC_SUCCESS;
    }
    __memcpy(MACADDR, pMacAddress, 6);
    ChangeMacAddrOrder(MACADDR);
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(memcmp(WhiteMacAddrList[i].MacAddr, MACADDR, 6) == 0)
        {
            Result = ASSOC_SUCCESS;
            *WhiteSeq = i;
            break;
        }
    }

    return Result;
}
//检测TEI是否合法
U8 CheckTheTEILegal(U16 StaTEI)
{
    if(StaTEI < 2)
    {
        return FALSE;
    }
    if(StaTEI > MAX_WHITELIST_NUM)
    {
        return FALSE;
    }
    if(DeviceTEIList[StaTEI - 2].NodeTEI == StaTEI)
    {
        return TRUE;
    }
    return FALSE;
}



//return 
//0:childen node <=1
//1：childen node >1
U8  CountproxyNum(U16 TEI)
{
	U8 CountofChild=0;
	list_head_t *pos,*node;
	DEVICE_TEI_LIST_t *DeviceEach;
	list_for_each_safe(pos, node,&DeviceListHeader.link)
	{
	   DeviceEach = list_entry(pos, DEVICE_TEI_LIST_t, link);
        if(DeviceEach->ParentTEI == TEI)
        {
            CountofChild++;
			if(CountofChild>1)
			return TRUE;
        }	   
	   
	}
	return FALSE;

}


/*函数名称：寻找以某一节点的父节点做为父节点的节点数量
参数值：父节点TEI
返回值：以该父节点为父节点的个数
*/
U16 CountOfUseMyParentNodeAsParent(U16 parentTEI)
{
    U16 j;
    U16 CountofChild=0;
    for(j=0;j<NWK_MAX_ROUTING_TABLE_ENTRYS;j++)
    {
        if(DeviceTEIList[j].ParentTEI == parentTEI)
        {
            CountofChild++;
        }
    }
    return CountofChild;
}


void AddWhiteListCnmAddrbyTEI(U8 *MeterAddr,U16 WhiteSeq,U16 TEI,U8 Devicetype,U8 MacType)
{
    U8	macAddr[6];

    if( (Devicetype ==e_CJQ_2) || (Devicetype ==e_CJQ_1))
    {
        if(MacType != e_MAC_METER_ADDR)
        {
            app_printf("MacType != METER\n");
            return;
        }
    }
    __memcpy(macAddr,MeterAddr,6);
    ChangeMacAddrOrder(macAddr);
    //对比MAC地址
    if(WhiteSeq < MAX_WHITELIST_NUM && memcmp(macAddr,WhiteMacAddrList[WhiteSeq].MacAddr,6)==0)
    {
        //            WHLPTST
        __memcpy(WhiteMacAddrList[WhiteSeq].CnmAddr,MeterAddr,6);
        WhiteMacAddrList[WhiteSeq].TEI = TEI;
    }
    
}

void DelWhiteListCnmAddr(U8 *CmdAddr,U16 WhiteSeq)
{
    U8	macAddr[6]={0,0,0,0,0,0};
    
        //对比MAC地址
    if(WhiteSeq < MAX_WHITELIST_NUM && memcmp(CmdAddr,WhiteMacAddrList[WhiteSeq].CnmAddr,6)==0)
        {
//            WHLPTST
        __memcpy(WhiteMacAddrList[WhiteSeq].CnmAddr,macAddr,6);
        WhiteMacAddrList[WhiteSeq].waterRfTh=0;
        WhiteMacAddrList[WhiteSeq].SetResult=0;
//            WHLPTED
        memset(WhiteMaCJQMapList[WhiteSeq].CJQAddr,0x00,6);
    }
}


/**/
void AddDeviceListEach(DATALINK_TABLE_t * head, U8 *pmac,U16 tei)
{
    if(IS_VALID_TEI(tei) == FALSE)
    {
        return;
    }
    DEVICE_TEI_LIST_t *addEntry = (DEVICE_TEI_LIST_t *)&DeviceTEIList[tei - 2];
	if(-1 == datalink_table_add(head, &addEntry->link ) )
	{
        printf_s("AddDevList Err!\n");
        memset(&DeviceTEIList[tei - 2], 0xff, sizeof(DEVICE_TEI_LIST_t));
	}
}

void DelDeviceListEach(U16 tei)
{
    if(IS_VALID_TEI(tei) == FALSE)
	{
		return;
	}
	
    DEVICE_TEI_LIST_t *DeviceEach = (DEVICE_TEI_LIST_t *)&DeviceTEIList[tei-2];
	if(tei == DeviceEach->NodeTEI)
	{
        datalink_table_del(&DeviceListHeader, &DeviceEach->link, FALSE);
        memset(&DeviceTEIList[tei-2],0xff,sizeof(DEVICE_TEI_LIST_t));
		return;	
	}
	
	printf_s("waring:not found tei=%02x\n",tei);
}

void CleanDevListlink()
{

    U16 i = 0;
    do
    {
        memset((U8 *)&DeviceTEIList[i].MacAddr, 0xff, sizeof(DEVICE_TEI_LIST_t)-sizeof(list_head_t));
        //INIT_LIST_HEAD(&DeviceTEIList[i].link);
    }while(++i<MAX_WHITELIST_NUM);

	 list_head_t *pos,*node;
     DEVICE_TEI_LIST_t *DeviceEach;
	 list_for_each_safe(pos, node,&DeviceListHeader.link)
	 {
        DeviceEach = list_entry(pos, DEVICE_TEI_LIST_t, link);
        datalink_table_del(&DeviceListHeader, &DeviceEach->link, FALSE);
		//return;
	 }	
     DeviceListHeaderINIT();
}


void getStaPcoNum()
{
  /*  list_head_t *pos, *n;
    DEVICE_LIST_EACH *pNode;
    list_for_each_safe(pos, n, &DeviceListHeader.link)
    {
        pNode = list_entry(pos, DEVICE_LIST_EACH, link);
		if(pNode->NodeTEI>CCO_TEI &&  pNode->NodeTEI< MAX_WHITELIST_NUM+1)
		{
	        if(DeviceTEIList[pNode->NodeTEI -2].NodeType == e_STA_NODETYPE)
	        	
	        	nnib.discoverSTACount++;

	        if(DeviceTEIList[pNode->NodeTEI -2].NodeType == e_PCO_NODETYPE)
	            nnib.PCOCount++;		
		}

    }*/
    U16 i;
    nnib.discoverSTACount = nnib.PCOCount = 0;
    for(i=0;i<MAX_WHITELIST_NUM;i++)
    {
        if(DeviceTEIList[i].NodeType == e_STA_NODETYPE)
            nnib.discoverSTACount++;

        if(DeviceTEIList[i].NodeType == e_PCO_NODETYPE)
            nnib.PCOCount++;
    }

}

//通过序号设置一条完整的device条目信息
U8 Set_DeviceList_COLLECT(U16 Seq , U8 DATA)
{
	//app_printf("Set_DeviceList_COLLECT\n");
	if(Seq<MAX_WHITELIST_NUM)
	{
		DeviceTEIList[Seq].CollectNeed = DATA;
		return TRUE;
	}
    return FALSE;


}

//离网感知停电原因记录
U8 set_device_list_power_off(U16 Seq)
{
	if(Seq<MAX_WHITELIST_NUM)
	{
	    if(DeviceTEIList[Seq].NodeTEI != 0xffff)
        {   
		    DeviceTEIList[Seq].power_off_fg = TRUE;
			app_printf("DeviceTEIList[ %d ].PowerOffRec = %d \n",Seq,e_POWER_OFF);
		    return TRUE;
        }
	}
	return FALSE;
}

//离网感知停电原因清除
U8 reset_device_list_power_off(U16 Seq)
{
	if(Seq<MAX_WHITELIST_NUM)
	{
	    if(DeviceTEIList[Seq].NodeTEI !=0xffff)
        {   
        	if(DeviceTEIList[Seq].NodeState == e_NODE_ON_LINE)
		    	DeviceTEIList[Seq].power_off_fg = FALSE;
			app_printf("DeviceTEIList[ %d ].PowerOffRec = %d \n",Seq,DeviceTEIList[Seq].power_off_fg);
		    return TRUE;
        }
	}
	return FALSE;
}

U8 set_device_list_white_seq(U16 Seq,U16 WhiteSeq)
{
	if(DeviceTEIList[Seq].NodeTEI != BROADCAST_TEI && DeviceTEIList[Seq].WhiteSeq != WhiteSeq)
	{
		DeviceTEIList[Seq].WhiteSeq = WhiteSeq;
		return TRUE;
	}
	return FALSE;
}


/**
 * @brief CheckHasRfChild   检查当前PCO下是否有通过无线入网的节点。
 * @param TEI               PCO TEI
 * @return                  TRUE：有无线子节点， FALSE : 无
 */
U8 CheckHasRfChild(U16 TEI)
{
    U16 i;

    for(i = 0; i< MAX_WHITELIST_NUM; i++)
    {
        if(DeviceTEIList[i].ParentTEI == TEI && DeviceTEIList[i].LinkType == e_RF_Link)
            return TRUE;
    }

    return FALSE;

}
/**
 * @brief Get the Rf Consult En object  获取信道协商使能
 * 
 * @return U8 
 */
U8 GetRfConsultEn()
{
    return nnib.RfConsultEn;
}
/**
 * @brief Set the Rf Consult En object  设置信道协商使能
 * 
 * @param en 
 */
void SetRfConsultEn(U8 en)
{
    nnib.RfConsultEn = en == 1 ? 1 : 0;
}
/**
 * @brief Set the Network Type object
 * 
 * @param type 0:混合组网，1:单载波组网，2:单无线组网
 */
void SetNetworkType(U8 type)
{
    nnib.NetworkType = type;
}
/**
 * @brief Get the Network Type object
 * 
 * @return U8 0:混合组网，1:单载波组网，2:单无线组网
 */
U8 GetNetworkType()
{
    return nnib.NetworkType;
}


/*函数名称：分配入网节点TEI
参数值：入网节点的macaddr
返回值：分配的TEI值
*/
#if 1
U16 DT_AssignTEI(U8 *pMacAddr)
{
	U16 i;
	list_head_t *pos,*node;
	DEVICE_TEI_LIST_t *DeviceEach;
   //如果已经分配过此MAC地址的设备，使用之前分配的
	list_for_each_safe(pos, node,&DeviceListHeader.link)
	{
		DeviceEach = list_entry(pos, DEVICE_TEI_LIST_t, link);
	   if(memcmp(DeviceEach->MacAddr , pMacAddr, 6) == 0)
	   {

		   //DeviceTEIList[DeviceEach->NodeTEI-2].ModeNeed = e_NeedGet; //获取模块ID
		   DeviceEach->ModeNeed = e_NeedGet; //获取模块ID
		   DeviceEach->Phase = e_UNKNOWN_PHASE;
           DeviceEach->Edgeinfo = 2;
		   DeviceEach->CurveNeed = e_NeedGet; //获取曲线冻结数据
		   //陕西，湖南使用
		   if(PROVINCE_HUNAN==app_ext_info.province_code || PROVINCE_SHANNXI==app_ext_info.province_code)
		   {
			   if(app_assoc_renew_one_hook != NULL)
			   		app_assoc_renew_one_hook();
		   }
		   
		   if(DeviceEach->ParentTEI > CCO_TEI)
		   {
			   if(DeviceTEIList[DeviceEach->ParentTEI-2].NodeType	== e_PCO_NODETYPE)
			   {
				   //if(CountOfUseMyParentNodeAsParent(DeviceTEIList[DeviceEach->NodeTEI-2].ParentTEI) == 1)
				   if(CountproxyNum(DeviceEach->ParentTEI) == FALSE)
				   {
					   DeviceTEIList[DeviceEach->ParentTEI-2].NodeType = e_STA_NODETYPE;
					   nnib.discoverSTACount++;
					   if(nnib.PCOCount >0)nnib.PCOCount--;
					   app_printf("PCO -- DT_AssignTEI\n");
				   }
			   }
		   }
		   return DeviceEach->NodeTEI-2;
	   }
   }
	//如果没有分配，在网络设备列表中找已经出网的表代替
	for(i = 0; i < MAX_WHITELIST_NUM; i++)
	{
		if(DeviceTEIList[i].NodeState == e_NODE_OUT_NET)
		{
			nnib.discoverSTACount++;

			DeviceTEIList[i].NodeMachine= e_NewNode; //刚入网节点标记为新入网状态
			g_SendCenterBeaconCountWithoutRespond = 0;
			DeviceTEIList[i].Phase = e_UNKNOWN_PHASE;//获取相位
			DeviceTEIList[i].LNerr = 2;
			DeviceTEIList[i].Edgeinfo = 2;
			DeviceTEIList[i].F31_D5 = 0;
			DeviceTEIList[i].F31_D6 = 0;
			DeviceTEIList[i].F31_D7 = 0;

            DeviceTEIList[i].HasRfChild = FALSE;
			DeviceTEIList[i].CurveNeed = e_NeedGet; //获取曲线冻结数据

			//陕西，湖南使用
			if(PROVINCE_HUNAN==app_ext_info.province_code || PROVINCE_SHANNXI==app_ext_info.province_code)
		   	{
				if(app_assoc_renew_one_hook != NULL)
			   		app_assoc_renew_one_hook();
			}
			
			AddDeviceListEach(&DeviceListHeader,pMacAddr,i+2);
            //__memcpy(DeviceTEIList[i].MacAddr, pMacAddr, MACADRDR_LENGTH);      //记录入网节点MAC地址。

			return i;
		}
	}
	return INVALID;

}
#else


U16 AssignTEI(U8 *pMacAddr)
{
    U16 i;
    //如果已经分配过此MAC地址的设备，使用之前分配的
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(memcmp(DeviceTEIList[i].MacAddr , pMacAddr, 6) == 0)
        {

            DeviceTEIList[i].ModeNeed = e_NeedGet; //获取模块ID
            if(DeviceTEIList[i].ParentTEI > CCO_TEI)
            {
                if(DeviceTEIList[DeviceTEIList[i].ParentTEI-2].NodeType  == e_PCO_NODETYPE)
                {
                    //if(CountOfUseMyParentNodeAsParent(DeviceTEIList[i].ParentTEI) == 1)
                    if(CountproxyNum(DeviceTEIList[i].ParentTEI) == FALSE)
                    {
                        DeviceTEIList[DeviceTEIList[i].ParentTEI-2].NodeType = e_STA_NODETYPE;
                        nnib.discoverSTACount++;
                        if(nnib.PCOCount>0)nnib.PCOCount--;
                    }
                }
            }
            return i;
        }
    }
    //如果没有分配，在网络设备列表中找已经出网的表代替
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(DeviceTEIList[i].NodeState == e_NODE_OUT_NET)
        {
            nnib.discoverSTACount++;
//            DeviceTEIList[i].DeviceType = e_STA_NODETYPE;
            DeviceTEIList[i].NodeMachine= e_NewNode; //刚入网节点标记为新入网状态
            g_SendCenterBeaconCountWithoutRespond = 0;
            DeviceTEIList[i].Phase = e_UNKNOWN_PHASE;//获取相位
            DeviceTEIList[i].LNerr = 2;
            DeviceTEIList[i].Edgeinfo = 2;
            DeviceTEIList[i].F31_D5 = 0;
            DeviceTEIList[i].F31_D6 = 0;
            DeviceTEIList[i].F31_D7 = 0;
            //<<<
            return i;
        }
    }
    return INVALID;
}
#endif
void SetDeviceList(U32     SNID,U8 *SrcMacAddr,U16 TEI)
{
     if(SNID !=GetSNID()){return;} //cco暂时不接收其他网路
     if(TEI > CCO_TEI)//防止数据越界
     {
        if(DeviceTEIList[TEI-2].NodeState != e_NODE_OUT_NET)//网内更新 时间
        {
            DeviceTEIList[TEI-2].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
            DeviceTEIList[TEI-2].NodeState = e_NODE_ON_LINE;
        }
     }

}

/*函数名称:更新网内节点的状态
如果在一个完整的心跳周期（两个路由周期）时间内，CCO发现某个STA站点的活跃次数为0，则CCO判断该STA站点离线
如果STA站点被判断处于离线状态，并且在离线状态下，连续四个完整的心跳周期（八个路由周期）时间内，CCO接收到该STA站点的报文个数为0，则CCO判断STA处于未入网状态
*/
void UpDataDeviceTEIlist()
{
    U16 i,j;
    for(i=0;i<MAX_WHITELIST_NUM;i++)
    {
		if(DeviceTEIList[i].NodeState == e_NODE_OUT_NET)
		{
			continue;
		}
        if(DeviceTEIList[i].DurationTime>0)
        {
            DeviceTEIList[i].DurationTime--;
        }
        else//时间到来时要切换状态
        {
            if(DeviceTEIList[i].NodeState == e_NODE_ON_LINE)
            {
                DeviceTEIList[i].NodeState = e_NODE_OFF_LINE;
                DeviceTEIList[i].DurationTime = nnib.RoutePeriodTime*8;// + nnib.RoutePeriodTime/2;
                off_grid_on_to_off(&DeviceTEIList[i]);			//离网感知在网变更离网
                ReacordDeepth(0xfc,DeviceTEIList[i].NodeTEI,DeviceTEIList[i].ParentTEI
                			 ,DeviceTEIList[i].ParentTEI,DeviceTEIList[i].NodeDepth,DeviceTEIList[i].NodeType,0);
            }
            else
            {
                U16 	ParentTEI;
                ParentTEI = DeviceTEIList[i].ParentTEI;
                if(DeviceTEIList[i].NodeType == e_PCO_NODETYPE)
                {
                    if(nnib.PCOCount>0)nnib.PCOCount--;
                }
                else
                {
                    if(nnib.discoverSTACount>0)nnib.discoverSTACount--;
                }
                ReacordDeepth(0xfe,DeviceTEIList[i].NodeTEI,DeviceTEIList[i].ParentTEI
                			 ,DeviceTEIList[i].ParentTEI,DeviceTEIList[i].NodeDepth,DeviceTEIList[i].NodeType,0);

                //<< 双模台体测试新增
                //SendMMeDelayLeaveOfNum(DeviceTEIList[i].MacAddr,1, 0, e_LEAVE_WAIT_SELF_OUTLINE);
                //回收TEI
                DelWhiteListCnmAddr(DeviceTEIList[i].MacAddr,DeviceTEIList[i].WhiteSeq);
				DelDeviceListEach(DeviceTEIList[i].NodeTEI);
//                    memset(&DeviceTEIList[i],0xff,sizeof(DEVICE_TEI_LIST_t));

                if(ParentTEI>CCO_TEI)//检查是否还有已ParentTEI作为父节点的
                {
                    for(j=0;j<MAX_WHITELIST_NUM;j++)
                    {
                        if(DeviceTEIList[j].ParentTEI == ParentTEI )break;
                    }
                    if(j==MAX_WHITELIST_NUM)
                    {
                        if(DeviceTEIList[ParentTEI-2].NodeType == e_PCO_NODETYPE)
                        {
                            DeviceTEIList[ParentTEI-2].NodeType = e_STA_NODETYPE;
                            if(nnib.PCOCount>0)nnib.PCOCount--;
                            nnib.discoverSTACount++;
                        }

                    }

                }

            }
        }
		off_grid_up_data(&DeviceTEIList[i]);
    }
}


//函数名称：查询指定TEI 的直连STA,将STA的TEI放入pteiBuf中，并且返回数量

U16 SearchAllDirectSTA(U16 TEI , U8 *pteiBuf)
{
    U16 i = 0, num = 0;
    NODE_ROUTE_RABLE_t *pNodeRouteInfo;
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(TEI ==  DeviceTEIList[i].ParentTEI)
        {
            if(DeviceTEIList[i].NodeType == e_STA_NODETYPE)
            {
                pNodeRouteInfo = (NODE_ROUTE_RABLE_t *)pteiBuf;
                pNodeRouteInfo->NodeTei= DeviceTEIList[i].NodeTEI;
                pNodeRouteInfo->LinkType= DeviceTEIList[i].LinkType;
                pteiBuf += 2;
                num++;
            }

        }
    }
    return num;
}

//检查是否出现错误的拓扑结构
U8 CheckErrtopology(U16 NewSta)
{
    U8 i;
    U16	 seq = NewSta-2;
    for(i=0;i<=MaxDeepth;i++)
    {
        if(DeviceTEIList[seq].ParentTEI == CCO_TEI)
        {
            return TRUE;
        }

        seq = DeviceTEIList[seq].ParentTEI-2;
        if(seq >=MAX_WHITELIST_NUM)
            return FALSE;
    }

    return FALSE;
}

#if 0
U8 ReturnFlag;
//函数名称:查询所有子站点,只有经过此节点使用递归遍历要查询的站点所中继的所有站点
void SearchAllChildStation(U16  StaTEI)
{
    U16  i;
    U16  startIndex = ChildCount;
    U16  stopIndex;
    DEVICE_TEI_LIST_t   *pDeviceTei;

    if(ReturnFlag)
        return;

    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        pDeviceTei = &DeviceTEIList[i];

        if(pDeviceTei->ParentTEI == StaTEI)
        {
            if(CheckErrtopology(pDeviceTei->NodeTEI) !=TRUE)
            {
                net_printf("-----SearchAllChildStation return----\n");
                ReturnFlag =TRUE;
                return;
            }
            ChildStaIndex[ChildCount++] = i;
        }
    }

    stopIndex = ChildCount;
    if(stopIndex > startIndex)
    {
        for(i = startIndex; i < stopIndex; i++)
        {
            pDeviceTei = &DeviceTEIList[ChildStaIndex[i]];
            if(pDeviceTei->NodeType == e_PCO_NODETYPE)
            {
                SearchAllChildStation(pDeviceTei->NodeTEI);
            }
        }
    }

    return;
}

#else

//判断SRCTEI到CCO是否经过DSTTEI
U8 RouteByThisNode(U16 DSTTEI,U16 SRCTEI )
{
	U8 i;
	U16 starttei;
	starttei = SRCTEI;
	for(i=0;i<15;i++)
	{

		starttei = DeviceTEIList[starttei-2].ParentTEI ;
		if(starttei == DSTTEI)
		{
			return TRUE;
		}
		if(DeviceTEIList[starttei-2].NodeDepth <= DeviceTEIList[DSTTEI-2].NodeDepth) 
		{
			return FALSE;
		}

		if(starttei == CCO_TEI)
		{
			return FALSE;
		}
	}
	return FALSE;
}

U8 ChildMaxDeepth = 0;


void DT_SearchAllChildStation(U16   			StaTEI)
{
	list_head_t *pos,*node;
	DEVICE_TEI_LIST_t *DeviceEach;
	list_for_each_safe(pos, node,&DeviceListHeader.link)
	{
		DeviceEach = list_entry(pos, DEVICE_TEI_LIST_t, link);
		//if(DeviceTEIList[DeviceEach->NodeTEI-2].NodeDepth<= DeviceTEIList[StaTEI-2].NodeDepth)
		if(DeviceEach->NodeDepth <= DeviceTEIList[StaTEI-2].NodeDepth)
		continue;
		if(RouteByThisNode(StaTEI,DeviceEach->NodeTEI))
		{
			ChildStaIndex[ChildCount++] = DeviceEach->NodeTEI-2;
            ChildMaxDeepth = (DeviceEach->NodeDepth > ChildMaxDeepth?DeviceEach->NodeDepth : ChildMaxDeepth);
		}
	}
}
#endif


//函数描述：使用MAC地址搜索对应的TEI
U16 SearchTEIDeviceTEIList(U8 *pMacAddr)
{

	list_head_t *pos,*node;
	DEVICE_TEI_LIST_t *DeviceEach;
	list_for_each_safe(pos, node,&DeviceListHeader.link)
	{
		DeviceEach = list_entry(pos, DEVICE_TEI_LIST_t, link);
		if(memcmp(DeviceEach->MacAddr,pMacAddr,6)==0)
		{
			return DeviceEach->NodeTEI;
		}
	}
	return INVALID;

}

//
U8 SearchMACDeviceTEIList(U16 TEI,U8 *pMacAddr)
{


    if(TEI <INVALID && TEI >=2)
    {
        __memcpy(pMacAddr, DeviceTEIList[TEI-2].MacAddr, 6);
        return TRUE;
    }
    return FALSE;
}

//-------------------------------------------------------------------------------------------------
// 函数名：U8 CalculatePHASE(U16 TEI, U32 ntb, U32 *staNtb, U8 numa, U8 numb, U8 numc, U8 *Phase)
// 
// 功能描述：
//     本函数用于计算单相相位，识别三相是否缺相，是否逆相序等
// 参数：
//     U16 TEI      识别节点的TEI值，根据此值去相应的DeviceTEIList条目中保存信息
//     U32 ntb      CCO记录的与上报基值最相近的过零点NTB值
//     U32 *staNtb  STA上报上来的多个过零点值，由基值开始
//     U8 numa      相线1告知数量
//     U8 numb      相线2告知数量
//     U8 numc      相线3告知数量
//     U8 *Phase    指针变量传递一个U8字节地址进去用于存放相位识别结果
//     
// 返回值：
//     U8    返回值表示是否逆相序或零火反接
//-------------------------------------------------------------------------------------------------
U8 CalculatePHASE(U16 TEI, U32 ntb, U32 *staNtb, U8 numa, U8 numb, U8 numc, U8 *Phase)
{
    U8 Phase1=0,Phase2=0,Phase3=0,LNstate=0;
    U16 Adiff=0;
    U16 Bdiff=0;
    U16 Cdiff=0;

    if(TEI>CCO_TEI && TEI<MAX_WHITELIST_NUM+1)
    {
        DeviceTEIList[TEI-2].F31_D0 = (numa==0?0:1);
        DeviceTEIList[TEI-2].F31_D1 = (numb==0?0:1);
        DeviceTEIList[TEI-2].F31_D2 = (numc==0?0:1);
    }
    else
    {
        return 0;
    }

//    //判断第一出线相位
    if(DeviceTEIList[TEI-2].F31_D0)
    {
        LNstate = CalculateNTBDiff(ntb, &staNtb[1], numa, &Phase1, &Adiff);
        
        if(LNstate ==  DeviceTEIList[TEI-2].Edgeinfo  || DeviceTEIList[TEI-2].Edgeinfo == 2)
        {
            DeviceTEIList[TEI-2].LNerr = 0;

            if(PROVINCE_HEBEI == app_ext_info.province_code)
            {
                if(Phase1 != 0 && DeviceTEIList[TEI - 2].UP_ERR == 2)//河北,2为异常
                {
                    switch(Phase1)
                    {
                    case e_A_PHASE: //ACB
                        DeviceTEIList[TEI - 2].F31_D5 = 1;
                        DeviceTEIList[TEI - 2].F31_D6 = 0;
                        DeviceTEIList[TEI - 2].F31_D7 = 0;
                        break;
                    case e_B_PHASE://BAC
                        DeviceTEIList[TEI - 2].F31_D5 = 0;
                        DeviceTEIList[TEI - 2].F31_D6 = 1;
                        DeviceTEIList[TEI - 2].F31_D7 = 0;
                        break;
                    case e_C_PHASE://CBA
                        DeviceTEIList[TEI - 2].F31_D5 = 1;
                        DeviceTEIList[TEI - 2].F31_D6 = 0;
                        DeviceTEIList[TEI - 2].F31_D7 = 1;
                        break;
                    default:
                        net_printf("hebei, error!phase1=%d\n", Phase1);
                        DeviceTEIList[TEI - 2].F31_D5 = 1;
                        DeviceTEIList[TEI - 2].F31_D6 = 1;
                        DeviceTEIList[TEI - 2].F31_D7 = 1;
                        break;
                    }
                    net_printf("1CalculatePHASE,F31_D5=%d,F31_D6=%d,F31_D7=%d\n", DeviceTEIList[TEI - 2].F31_D5, DeviceTEIList[TEI - 2].F31_D6, DeviceTEIList[TEI - 2].F31_D7);
                    *Phase = Phase1;
                    return LNstate;
                }
                else if(Phase1 != 0 && DeviceTEIList[TEI - 2].UP_ERR == 1)//河北,1为正常
                {
                    switch(Phase1)
                    {
                    case e_A_PHASE: //ABC
                        DeviceTEIList[TEI - 2].F31_D5 = 0;
                        DeviceTEIList[TEI - 2].F31_D6 = 0;
                        DeviceTEIList[TEI - 2].F31_D7 = 0;
                        break;
                    case e_B_PHASE://BCA
                        DeviceTEIList[TEI - 2].F31_D5 = 1;
                        DeviceTEIList[TEI - 2].F31_D6 = 1;
                        DeviceTEIList[TEI - 2].F31_D7 = 0;
                        break;
                    case e_C_PHASE://CAB
                        DeviceTEIList[TEI - 2].F31_D5 = 0;
                        DeviceTEIList[TEI - 2].F31_D6 = 0;
                        DeviceTEIList[TEI - 2].F31_D7 = 1;
                        break;
                    default:
                        net_printf("hebei, error!phase1=%d\n", Phase1);
                        DeviceTEIList[TEI - 2].F31_D5 = 1;
                        DeviceTEIList[TEI - 2].F31_D6 = 1;
                        DeviceTEIList[TEI - 2].F31_D7 = 1;
                        break;
                    }
                    net_printf("2CalculatePHASE,F31_D5=%d,F31_D6=%d,F31_D7=%d\n", DeviceTEIList[TEI - 2].F31_D5, DeviceTEIList[TEI - 2].F31_D6, DeviceTEIList[TEI - 2].F31_D7);
                    *Phase = Phase1;
                    return LNstate;
                }
            }
        }
        else
        {
            DeviceTEIList[TEI-2].LNerr = 1;
            net_printf("TEI=%d LN A ERR\n",TEI);
            goto xianxu;   //A相线逆序时，如果是三相表，不再判断BC。
        }
        net_printf("Phase1=%d\n",Phase1);
    }
    if(DeviceTEIList[TEI-2].F31_D1)//第二出线相位
    {
        LNstate=CalculateNTBDiff( ntb ,&staNtb[numa+1],numb,&Phase2,&Bdiff);
        if( LNstate ==  DeviceTEIList[TEI-2].Edgeinfo  || DeviceTEIList[TEI-2].Edgeinfo ==2)
        {
            DeviceTEIList[TEI-2].LNerr = 0;
        }
        else
        {
            DeviceTEIList[TEI-2].LNerr = 1;
            net_printf("TEI=%d LN B ERR\n",TEI);
            goto xianxu;// 当A相正常，B相异常时，不再判断C相
        }
        net_printf("Phase2=%d\n",Phase2);
    }
    if(DeviceTEIList[TEI-2].F31_D2)//第三出线相位
    {
        LNstate=CalculateNTBDiff( ntb ,&staNtb[numa+numb+1],numc,&Phase3,&Cdiff);
        if( LNstate ==  DeviceTEIList[TEI-2].Edgeinfo  || DeviceTEIList[TEI-2].Edgeinfo ==2)
        {
            DeviceTEIList[TEI-2].LNerr = 0;
        }
        else
        {
            DeviceTEIList[TEI-2].LNerr = 1;
            net_printf("TEI=%d LN C ERR\n",TEI);
        }
        net_printf("Phase3=%d\n",Phase3);
    }

    /*
        D7		D6		D5		相序
        0		0		0		ABC
        0		0		1		ACB
        0		1		0		BAC
        0		1		1		BCA
        1		0		0		CAB
        1		0		1		CBA
        1		1		0		NL
        1		1		1		保留
    */
xianxu:
    if(numb!=0 || numc !=0 || DeviceTEIList[TEI-2].DeviceType == e_3PMETER)//三相表
    {
        if(DeviceTEIList[TEI-2].LNerr == 1)
        {
            DeviceTEIList[TEI-2].F31_D5 = 0;
            DeviceTEIList[TEI-2].F31_D6 = 1;
            DeviceTEIList[TEI-2].F31_D7 = 1;
        }
        else if((Phase1 == e_A_PHASE || Phase1 ==e_UNKNOWN_PHASE)  &&
            (Phase2 == e_B_PHASE || Phase2 ==e_UNKNOWN_PHASE)  &&
            (Phase3 == e_C_PHASE || Phase3 ==e_UNKNOWN_PHASE))
        {
            DeviceTEIList[TEI-2].F31_D5 = 0;
            DeviceTEIList[TEI-2].F31_D6 = 0;
            DeviceTEIList[TEI-2].F31_D7 = 0;
        }
        else if((Phase1 == e_A_PHASE || Phase1 ==e_UNKNOWN_PHASE)  &&
            (Phase2 == e_C_PHASE || Phase2 ==e_UNKNOWN_PHASE)  &&
            (Phase3 == e_B_PHASE || Phase3 ==e_UNKNOWN_PHASE)	)
        {
            DeviceTEIList[TEI-2].F31_D5 = 1;
            DeviceTEIList[TEI-2].F31_D6 = 0;
            DeviceTEIList[TEI-2].F31_D7 = 0;
        }
        else if((Phase1 == e_B_PHASE || Phase1 ==e_UNKNOWN_PHASE)  &&
            (Phase2 == e_A_PHASE || Phase2 ==e_UNKNOWN_PHASE)  &&
            (Phase3 == e_C_PHASE || Phase3 ==e_UNKNOWN_PHASE)	)
        {
            DeviceTEIList[TEI-2].F31_D5 = 0;
            DeviceTEIList[TEI-2].F31_D6 = 1;
            DeviceTEIList[TEI-2].F31_D7 = 0;
        }
        else if( (Phase1 == e_B_PHASE || Phase1 ==e_UNKNOWN_PHASE)  &&
            (Phase2 == e_C_PHASE || Phase2 ==e_UNKNOWN_PHASE)  &&
            (Phase3 == e_A_PHASE || Phase3 ==e_UNKNOWN_PHASE)	)
        {
            DeviceTEIList[TEI-2].F31_D5 = 1;
            DeviceTEIList[TEI-2].F31_D6 = 1;
            DeviceTEIList[TEI-2].F31_D7 = 0;
        }
        else if((Phase1 == e_C_PHASE || Phase1 ==e_UNKNOWN_PHASE)  &&
            (Phase2 == e_A_PHASE || Phase2 ==e_UNKNOWN_PHASE)  &&
            (Phase3 == e_B_PHASE || Phase3 ==e_UNKNOWN_PHASE)	)
        {
            DeviceTEIList[TEI-2].F31_D5 = 0;
            DeviceTEIList[TEI-2].F31_D6 = 0;
            DeviceTEIList[TEI-2].F31_D7 = 1;
        }
        else if( (Phase1 == e_C_PHASE || Phase1 ==e_UNKNOWN_PHASE)  &&
            (Phase2 == e_B_PHASE || Phase2 ==e_UNKNOWN_PHASE)  &&
            (Phase3 == e_A_PHASE || Phase3 ==e_UNKNOWN_PHASE)	)
        {
            DeviceTEIList[TEI-2].F31_D5 = 1;
            DeviceTEIList[TEI-2].F31_D6 = 0;
            DeviceTEIList[TEI-2].F31_D7 = 1;
        }
            
        app_printf("F31_D5=%d F31_D6=%d F31_D7=%d \n",DeviceTEIList[TEI-2].F31_D5,DeviceTEIList[TEI-2].F31_D6,DeviceTEIList[TEI-2].F31_D7);
        app_printf("Adiff=%d,Bdiff=%d,Cdiff=%d\n",Adiff,Bdiff,Cdiff);
        //三相中任意一相,未出现零火反接，
        if((DeviceTEIList[TEI-2].LNerr != 1)
            &&((DeviceTEIList[TEI-2].F31_D0 == DeviceTEIList[TEI-2].F31_D1 && DeviceTEIList[TEI-2].F31_D1==TRUE && abs(Adiff-Bdiff) >SELF_DIFF_RANGE )
            || (DeviceTEIList[TEI-2].F31_D0 == DeviceTEIList[TEI-2].F31_D2 && DeviceTEIList[TEI-2].F31_D0==TRUE && abs(Adiff-Cdiff)>SELF_DIFF_RANGE )
            || (DeviceTEIList[TEI-2].F31_D1 == DeviceTEIList[TEI-2].F31_D2 && DeviceTEIList[TEI-2].F31_D2==TRUE && abs(Bdiff-Cdiff)>SELF_DIFF_RANGE )
           ))
        {
            *Phase = 0;
            app_printf("phase data invalid\n");
        }
        else
        {
            if(DeviceTEIList[TEI-2].F31_D0)
            {
                *Phase = Phase1;
            }
            else
            {
                if(PROVINCE_HEBEI == app_ext_info.province_code)
                {
                    if(Phase1 != e_UNKNOWN_PHASE)
                    {
                        *Phase = Phase1;
                    }
                    else
                    {
                        *Phase = e_A_PHASE;
                    }
                }
                else
                {
                    *Phase = e_A_PHASE;
                }
            }
        }
        return 0;
    }
    else //单相表将第一出线相位结果返回
    {
        *Phase = Phase1;
        return LNstate;
    }

    return 0;
}


U8 SaveModeId(U8 *macaddr , U8* info)
{
    U16 usIdx=0;
//    WHLPTST
    for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
    {

        if(0==memcmp(macaddr, DeviceTEIList[usIdx].MacAddr, 6))
        {

            if(DeviceTEIList[usIdx].NodeTEI == BROADCAST_TEI)
            {
//                WHLPTED
                return  e_NeedGet ;
            }
            if( 0==memcmp(info, DeviceTEIList[usIdx].ModeID, sizeof(DeviceTEIList[usIdx].ModeID) ) || DeviceTEIList[usIdx].ModeNeed ==e_InitState)
            {
                DeviceTEIList[usIdx].ModeNeed =e_HasGet;
                __memcpy( DeviceTEIList[usIdx].ModeID ,info,sizeof(DeviceTEIList[usIdx].ModeID));
                net_printf("NodeTEI=%d e_HasGet\n",DeviceTEIList[usIdx].NodeTEI);
                dump_buf(DeviceTEIList[usIdx].ModeID,sizeof(DeviceTEIList[usIdx].ModeID));
            }
            else
            {
                DeviceTEIList[usIdx].ModeNeed =e_NeedReport;
                __memcpy( DeviceTEIList[usIdx].ModeID ,info,sizeof(DeviceTEIList[usIdx].ModeID));
                net_printf("NodeTEI=%d e_NeedReport\n",DeviceTEIList[usIdx].NodeTEI);
                dump_buf(DeviceTEIList[usIdx].ModeID,sizeof(DeviceTEIList[usIdx].ModeID));
            }
//            WHLPTED
            return DeviceTEIList[usIdx].ModeNeed;
        }

    }
//    WHLPTED
    return  e_NeedGet ;
}


//取设备列表个数
U16 GetDeviceNum(void)
{
        return (nnib.PCOCount+nnib.discoverSTACount);
}
/**安徽
 * @brief GetDeviceNumForApp        app层获取设备列表数量，过滤不在白名单设备（二采有地址模式/中继器）
 * @return
 */
U16 GetDeviceNumForApp(void)
{
    U16 i;
    U16 DevNum = 0;
    U8 err_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        //过滤中继器和地址模式入网的二采设备
        if(0 == memcmp(DeviceTEIList[i].MacAddr, err_mac, MAC_ADDR_LEN))
        {
            continue;
        }
        if(devicelist_ban_type(DeviceTEIList[i]))
        {
            continue;
        }

        DevNum++;
    }

    return DevNum;
}

//通过序号获取一条完整的device条目信息
U8 Get_DeviceList_All(U16 Seq , DEVICE_TEI_LIST_t* DeviceListTemp)
{
    if(Seq<MAX_WHITELIST_NUM)
    {
        __memcpy((U8*)DeviceListTemp,(U8*)&DeviceTEIList[Seq],sizeof(DEVICE_TEI_LIST_t));
        if(!IS_BROADCAST_TEI(DeviceListTemp->NodeTEI))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//通过序号设置一条完整的device条目信息
U8 Set_DeviceList_AREA(U16 TEI , U8 DATA)
{
    if(TEI<MAX_WHITELIST_NUM && TEI>1)
    {
        DeviceTEIList[TEI-2].AREA_ERR =DATA;
        return TRUE;
    }
    return FALSE;


}


U8 get_devicelist_area(U16 seq, U8* data)
{
    if(seq < MAX_WHITELIST_NUM)
    {
        *data = DeviceTEIList[seq].AREA_ERR;
        return TRUE;
    }
    return FALSE;
}


void Reset_DeviceList_AREA_ERR()
{
    U16 i;
	for(i=0;i<MAX_WHITELIST_NUM;i++)
	{
	    if(DeviceTEIList[i].NodeTEI !=0xfff)
        {   
		    DeviceTEIList[i].AREA_ERR = 3;
        }
	}
    net_printf("Reset_DeviceList_AREA_ERR\n!");
}


//通过meteraddr获取有一条完整的device条目信息
U8 Get_DeviceList_All_ByAdd(U8 *meteraddr , DEVICE_TEI_LIST_t* DeviceListTemp)
{
	U8 tempaddr[6];
	list_head_t *pos,*node;
	DEVICE_TEI_LIST_t *DeviceEach;

	__memcpy(tempaddr,meteraddr,6);
    ChangeMacAddrOrder(tempaddr);
	list_for_each_safe(pos, node,&DeviceListHeader.link)
	{
	 	DeviceEach = list_entry(pos, DEVICE_TEI_LIST_t, link);
	    if(0==memcmp(tempaddr, DeviceEach->MacAddr, 6) )
        {
            //__memcpy(DeviceListTemp, &DeviceTEIList[DeviceEach->NodeTEI -2], sizeof(DEVICE_TEI_LIST_t));
            __memcpy(DeviceListTemp, DeviceEach, sizeof(DEVICE_TEI_LIST_t));
	    	return TRUE;
	    }
	}
	return FALSE;
}


//查询的对应的SEQ,网表中是否存在
U8	Get_DeviceListVaild(U16 Seq)
{
    if(Seq >=MAX_WHITELIST_NUM)
    {
        return FALSE;
    }
    if(DeviceTEIList[Seq].NodeTEI != BROADCAST_TEI)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

U8 SaveEdgeinfo(U16 TEI,U8 Edgeinfo)
{
    if(DeviceTEIList[TEI-2].NodeTEI != BROADCAST_TEI && DeviceTEIList[TEI-2].Edgeinfo==2) //Edgeinfo沿信息	0表示和CCO相同，1不相同，2未知
    {

        DeviceTEIList[TEI-2].Phase      = e_UNKNOWN_PHASE; //当收集回来沿信息的时候，清空相位信息，等待再次要相位
        DeviceTEIList[TEI-2].Edgeinfo	=Edgeinfo	;

        return 1;
    }

    return 0;
}

//将节点入网后的状态机从一个状态转换为另一个状态
void SetNodeMachine(U8 OldState, U8 NewState)
{
    U16 i;
    //如果已经分配过此MAC地址的设备，使用之前分配的
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(DeviceTEIList[i].NodeMachine == OldState)
        {
            DeviceTEIList[i].NodeMachine = NewState;

        }
    }
}


U16 SelectAppointProxyNodebyLevel(U8 Seq,U8 level)
{
    U16 i, j = 0;
    //如果已经分配过此MAC地址的设备，使用之前分配的
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {

        if(DeviceTEIList[i].NodeType == e_PCO_NODETYPE && DeviceTEIList[i].NodeDepth == level)
        {

            if( j == Seq )
            {

                return DeviceTEIList[i].NodeTEI;
            }
            j++;
        }
    }

    return INVALID;
}

//在设备列表中寻找层级最高的STA
U16 SelectAppointHighstlevelSTANode()
{
    U16 i,tei=INVALID;
    U8  maxlevel=0;
    for(i=0;i<MAX_WHITELIST_NUM;i++)
    {
        if(DeviceTEIList[i].NodeType == e_STA_NODETYPE&&(DeviceTEIList[i].NodeTEI != BROADCAST_TEI))
        {
            if(  DeviceTEIList[i].NodeDepth > maxlevel)
            {
                maxlevel = DeviceTEIList[i].NodeDepth;
                tei = DeviceTEIList[i].NodeTEI;
            }
        }
    }
    for(i=0;i<MAX_WHITELIST_NUM;i++)
    {
        if(DeviceTEIList[i].NodeType == e_STA_NODETYPE && DeviceTEIList[i].NodeTEI != BROADCAST_TEI
                    && DeviceTEIList[i].NodeDepth == maxlevel  && DeviceTEIList[i].NodeTEI>tei)
        {
            tei = DeviceTEIList[i].NodeTEI;
        }
    }
    return tei;
}


//函数名称:在设备列表中找指定序号的角色为子节点TEI
U16 SelectAppointSTANode(U16 Seq)
{
    U16 i, j = 0;
    //如果已经分配过此MAC地址的设备，使用之前分配的
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(DeviceTEIList[i].NodeType == e_STA_NODETYPE)
        {
            if( j == Seq )
            {
                return DeviceTEIList[i].NodeTEI;

            }
            j++;
        }
    }

    return INVALID;
}

//函数名称:在设备列表中找指定序号的角色为子节点TEI,并且新入网
U16 SelectAppointNewSTANode(U16 Seq)
{
    U16 i, j = 0;
    //如果已经分配过此MAC地址的设备，使用之前分配的
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(DeviceTEIList[i].NodeType == e_STA_NODETYPE && DeviceTEIList[i].NodeMachine == e_IsSendBeacon)
        {
            if( j == Seq )
            {
                return DeviceTEIList[i].NodeTEI;

            }
            j++;
        }
    }

    return INVALID;
}

//获取一跳节点中A\B\C，并返回1跳点的总数
U16 GetPhaseParm(U16 *A_Num,U16 *B_Num,U16 *C_Num)
{

    U16	i, Num=0;
#if 0
    for(i = 0; i < MAX_WHITELIST_NUM; i++)
    {
        if(DeviceTEIList[i].NodeDepth == 1)
        {
            Num++;
            if(DeviceTEIList[i].LogicPhase == e_A_PHASE)
            {
                *A_Num =*A_Num+1;
            }
            if(DeviceTEIList[i].LogicPhase == e_B_PHASE)
            {
                *B_Num =*B_Num+1;
            }
            if(DeviceTEIList[i].LogicPhase == e_C_PHASE)
            {
                *C_Num =*C_Num+1;
            }
        }
    }
#else

    //组网完成之后，csma时隙分配按照时隙各相位节点数量比例分配
    if(GetNetworkflag() == TRUE)
    {

//        for(i = 0; i < MAX_WHITELIST_NUM; i++)
//        {
//            if(!IS_BROADCAST_TEI(DeviceTEIList[i].NodeTEI))
//            {
//                Num++;
//                U16 nextTei = NwkRoutingTable[DeviceTEIList[i].NodeTEI - 1].NextHopTEI;
//                if(DeviceTEIList[i].NodeDepth  > 1 && (!IS_BROADCAST_TEI(nextTei)) && nextTei > 1)          //更新逻辑相位信息
//                {
//                    DeviceTEIList[i].LogicPhase = DeviceTEIList[nextTei - 2].LogicPhase;
//                }

//                if(DeviceTEIList[i].LogicPhase == e_A_PHASE)
//                {
//                    *A_Num =*A_Num+1;
//                }
//                if(DeviceTEIList[i].LogicPhase == e_B_PHASE)
//                {
//                    *B_Num =*B_Num+1;
//                }
//                if(DeviceTEIList[i].LogicPhase == e_C_PHASE)
//                {
//                    *C_Num =*C_Num+1;
//                }
//            }
//        }


        for(i = 0; i < NWK_MAX_ROUTING_TABLE_ENTRYS; i++)
        {
            U16 destTei = i + 1;
            U16 nextTei = NwkRoutingTable[i].NextHopTEI;
            if(!IS_BROADCAST_TEI(nextTei))
            {
                if(DeviceTEIList[nextTei - 2].NodeDepth == 1)
                {
                    Num++;
                    switch(DeviceTEIList[nextTei - 2].LogicPhase)
                    {
                    case e_A_PHASE:
                        *A_Num =*A_Num+1;
                        break;
                    case e_B_PHASE:
                        *B_Num =*B_Num+1;
                        break;
                    case e_C_PHASE:
                        *C_Num =*C_Num+1;
                        break;
                    default:
                        break;
                    }

                    DeviceTEIList[destTei -2].LogicPhase = DeviceTEIList[nextTei - 2].LogicPhase;

                }
            }
        }
    }
    else    //组网完成之前CSMA时隙分配使用均分
    {
        *A_Num = *B_Num = *C_Num = 1;
        Num = 3;
    }
#endif
    return Num;

}
static void CCO_UpDataNeighborNetTime()
{
    U8	i = 0;
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(NeighborNet[i].SendSNID != 0xffffffff)
        {
            if(NeighborNet[i].LifeTime > 0)
            {
                NeighborNet[i].LifeTime--;
            }
            else
            {
                memset(&NeighborNet[i], 0xff, sizeof(NEIGHBOR_NET_t));
                if(nnib.NbSNIDnum > 0)
                    nnib.NbSNIDnum--;
            }
        }

    }
}
#endif







//启动发现列表发送
void StartMmDiscovery()
{
    modify_Discoverytimer(rand()%(nnib.SendDiscoverListTime*1000)+1);
}

static void SendDiscoverReq(work_t * work)
{
//    printf_s("dis st:%08x\n", get_phy_tick_time());
    SendDiscoverNodeList();
//    printf_s("dis ed:%08x\n", get_phy_tick_time());
}

static void DiscoverytimerCB( struct ostimer_s *ostimer, void *arg)
{

    if(nnib.NetworkType != e_SINGLE_HRF)        //纯无线组网方式，不发送载波发现列表
    {
        work_t *work = zc_malloc_mem(sizeof(work_t), "SDIS", MEM_TIME_OUT);
        work->work = SendDiscoverReq;
        work->msgtype = SEND_DIS;
        post_datalink_task_work(work);
    }
//    printf_s("dis st:%08x\n", get_phy_tick_time());
//	SendDiscoverNodeList();
    modify_Discoverytimer(nnib.SendDiscoverListTime*1000);
    //modify_Discoverytimer(100);

//    printf_s("dis ed:%08x\n", get_phy_tick_time());

}

static void modify_Discoverytimer(uint32_t first)
{
    if(Discoverytimer == NULL && first == 0)
        return;

    if(Discoverytimer == NULL)
    {
        Discoverytimer = timer_create(
                   first,
                   first,
                   TMR_TRIGGER,//TMR_TRIGGER
                   DiscoverytimerCB,
                   NULL,
                   "DiscoverytimerCB"
                   );
    }
    if(first == 0) //0直接进入回调
    {
       if(zc_timer_query(Discoverytimer) ==TMR_STOPPED)
       {
           timer_start(Discoverytimer);
       }
    //    timer_stop(Discoverytimer, TMR_CALLBACK);

    }
    else
    {
        timer_modify(Discoverytimer,
                first,
                first,
                TMR_TRIGGER ,//TMR_TRIGGER
                DiscoverytimerCB,
                NULL,
                "DiscoverytimerCB",
                TRUE);
        timer_start(Discoverytimer);
    }

}


//启动无线发现列表发送
ostimer_t	*RfDiscoverytimer=NULL;
static void modify_RfDiscoverytimer(uint32_t ms);
__SLOWTEXT void StartRfDiscovery()
{
    if(nnib.RfDiscoverPeriod != 0)
        modify_RfDiscoverytimer(rand()%(nnib.RfDiscoverPeriod*1000)+1);
}

static void SendRfDiscoverReq(work_t * work)
{
//    printf_s("dis st:%08x\n", get_phy_tick_time());
    SendMMeRFDiscoverNodeList();
//    printf_s("dis ed:%08x\n", get_phy_tick_time());
}

static void RfDiscoverytimerCB( struct ostimer_s *ostimer, void *arg)
{
    if(HPLC.testmode != NORM)
    {
        return;
    }
    if(nnib.NetworkType != e_SINGLE_HPLC)       //在纯载波组网方式下，不发送无线发现列表
    {
        work_t *work = zc_malloc_mem(sizeof(work_t), "RFDIS", MEM_TIME_OUT);
        work->work = SendRfDiscoverReq;
        work->msgtype = SEND_DIS;
        post_datalink_task_work(work);
    }
//    printf_s("dis st:%08x\n", get_phy_tick_time());
//	SendDiscoverNodeList();
    modify_RfDiscoverytimer(nnib.RfDiscoverPeriod*1000);
    //modify_Discoverytimer(100);

//    printf_s("dis ed:%08x\n", get_phy_tick_time());

}

static void modify_RfDiscoverytimer(uint32_t ms)
{
    if(RfDiscoverytimer == NULL && ms == 0)
        return;

    if(RfDiscoverytimer == NULL)
    {
        RfDiscoverytimer = timer_create(
                   ms,
                   ms,
                   TMR_TRIGGER,//TMR_TRIGGER
                   RfDiscoverytimerCB,
                   NULL,
                   "DiscoverytimerCB"
                   );
    }
    if(ms == 0) //0直接进入回调
    {
       if(zc_timer_query(RfDiscoverytimer) ==TMR_STOPPED)
       {
           timer_start(RfDiscoverytimer);
       }
    //    timer_stop(RfDiscoverytimer, TMR_CALLBACK);

    }
    else
    {
        timer_modify(RfDiscoverytimer,
                ms,
                ms,
                TMR_TRIGGER ,//TMR_TRIGGER
                RfDiscoverytimerCB,
                NULL,
                "RfDiscoverytimerCB",
                TRUE);
        timer_start(RfDiscoverytimer);
    }

}






#if defined(STATIC_NODE)

/******************************STA自身的在线离线管理**************************************/

 ostimer_t	 *JudgeProxyTypetimer=NULL;
 static void JudgeProxyTypetimerCB( struct ostimer_s *ostimer, void *arg)
 {
 	net_printf("proxy e_ProxyChangeRole\n");
	g_ReqReason = e_ProxyChangeRole;
    ClearNNIB();

 }

 static void modify_JudgeProxyTypetimer(uint32_t first)
 {
    if(JudgeProxyTypetimer == NULL && first == 0)
    {
        return;
    }

     if(JudgeProxyTypetimer == NULL)
     {
         JudgeProxyTypetimer = timer_create(first,
                                 first,
                                 TMR_TRIGGER,//TMR_TRIGGER
                                 JudgeProxyTypetimerCB,
                                 NULL,
                                 "network_timerCB"
                                );
     }
     if(first == 0) //0直接进入回调
     {
        if(zc_timer_query(JudgeProxyTypetimer) ==TMR_STOPPED)
        {
            timer_start(JudgeProxyTypetimer);
        }
        // timer_stop(JudgeProxyTypetimer, TMR_CALLBACK);

     }
     else
     {
         timer_modify(JudgeProxyTypetimer,
                 first,
                 first,
                 TMR_TRIGGER ,//TMR_TRIGGER
                 JudgeProxyTypetimerCB,
                 NULL,
                 "network_timerCB",
                 TRUE
                 );
         timer_start(JudgeProxyTypetimer);
     }

 }
/*-----------------------------离线指示使用----------------------------------*/

ostimer_t 	g_LeaveIndTimer;		//离线延时时间超时
static void LeaveNetAndClean(struct ostimer_s *ostimer, void *arg)
{
    g_ReqReason = e_RecvLeaveInd;
//    RecordAreaNotifyBuff(e_Clnnib,e_RecvLeaveInd,nnib.DestSnid,0,e_NODE_OFF_LINE);
    ClearNNIB();//模块出网
    net_printf("------------------------LeaveNetAndClean-------------------\n");
}
void modify_LeaveIndTimer(U32 first)
{
    if(first == 0)
        first = 2;

    if(g_LeaveIndTimer.fn == NULL)
    {
        timer_init(&g_LeaveIndTimer,
                   first,
                   first,
                   TMR_TRIGGER,
                   LeaveNetAndClean,
                   NULL,
                   "g_LeaveIndTimer"
                   );
    }
    if(TMR_RUNNING == timer_query(&g_LeaveIndTimer))
    {
        net_printf("g_LeaveIndTimer is Running!\n");
    }
    else
    {
        timer_modify(&g_LeaveIndTimer,
                first,
                first,
                TMR_TRIGGER ,//TMR_TRIGGER
                LeaveNetAndClean,
                NULL,
                "g_LeaveIndTimer",
                 TRUE
                );
        timer_start(&g_LeaveIndTimer);
        net_printf("----------------g_LeaveIndTimer------DelayTime = %d---------\n", first);
    }
}


/**********************************代理变更回复超时使用**********************************************/

static void WaitChangeProxycnfTask(work_t *work)
{
    ChangeReq_t.RetryTime++;
    if(ChangeReq_t.RetryTime>3)//代理变更多次尝试仍然失败
    {
        ChangeReq_t.RetryTime =0;
        ChangeReq_t.Result =FALSE;
        if(ChangeReq_t.Reason == e_LevelToHigh /*|| ChangeReq_t.Reason == e_KeepSyncCCO*/) //代理变更失败，离线
        {
            net_printf("^^ChangeReq_t.Reason =%d^^\n",ChangeReq_t.Reason);

            // g_ReqReason = e_ProxyChgfail;

            // if(ChangeReq_t.Reason == e_LevelToHigh)
            {
                g_ReqReason = e_LevHig;
            }

            ChangeReq_t.Reason =0;
            //            RecordAreaNotifyBuff(e_Clnnib,e_LevHig,nnib.DestSnid,0,e_NODE_OFF_LINE);
            ClearNNIB();

        }
        // else if(ChangeReq_t.Reason != e_KeepSyncCCO)        //尝试三次后代理变更失败，立即告知CCO。
        // {
        //     ChangeReq_t.RetryTime++;
        //     ChangeReq_t.Reason = e_KeepSyncCCO;
        //     SendProxyChangeReq(e_KeepSyncCCO);
        // }
    }
    else if(ChangeReq_t.RetryTime == 1) //本轮第一次发送代理变更
    {
        SendProxyChangeReq(ChangeReq_t.Reason);
    }
    else  if(ChangeReq_t.Reason != e_KeepSyncCCO)              //代理变更等待回复超时，选择下一个备用代理发送代理变更
    {
        SendProxyChangeReq(e_FialToTry);
    }
}

ostimer_t	*g_WaitChageProxyCnf =NULL; //等待代理变更回复
void WaitChangeProxycnfFun(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t), "WaitChangeProxycnf", MEM_TIME_OUT);
    work->work = WaitChangeProxycnfTask;
    post_datalink_task_work(work);
}

void modify_mgm_changesendtimer(U32 frist)
{
	if(frist == 0)
	{
		frist = 5;
	}
	
    if(g_WaitChageProxyCnf == NULL)
    {
        g_WaitChageProxyCnf = timer_create(
                   frist*1000,
                   frist*1000,
                   TMR_TRIGGER ,//TMR_TRIGGER
                   WaitChangeProxycnfFun,
                   NULL,
                   "WaitChangeProxycnfFun"
                   );
    }
    else
    {
        timer_modify(g_WaitChageProxyCnf,
                     frist*1000,
                     frist*1000,
                     TMR_TRIGGER ,//TMR_TRIGGER
                     WaitChangeProxycnfFun,
                     NULL,
                     "WaitChangeProxycnfFun",
                     TRUE);
    }

    timer_start(g_WaitChageProxyCnf);

}



 /*函数名称：从模块判断自身是否要离线
a)STA站点在加入网络后，如果在一个完整的心跳周期（两个路由周期）内，收不到任何信标帧；
b)STA站点在连续4个路由周期内，如果与代理站点的通信成功率为0；
c)STA站点收到CCO的“组网序列号”与自身记录的“组网序列号”不同；（信标中实现）
d)STA站点接收到离线指示报文，指示自己离线；
e)一级STA站点，如果检测到CCO的MAC地址发生变化，且已经连续一个周期；（未按照标准，目前检测CCO地址变化时,上报网络冲突，CCO为macadd未变SNID变化）
f)STA站点发现本站点的代理站点角色变为了发现站点已经连续一个路由周期（未实现）
g)本站点的层级超过最大层级限制（15级），站点需要离线。(信标中实现)

h)自己增加的保護策略
 */
void STA_JudgeOfflineBytimer()
{
    if( GetNodeState() != e_NODE_ON_LINE)return;
    if(nnib.StaOfflinetime >0)// 2个路由周期未听到信标
    {
        nnib.StaOfflinetime--;
//        net_printf("StaOfflinetime-%d\n",nnib.StaOfflinetime);
    }
    else
    {
        net_printf("------------------StaOfflinetime------------------\n");
        g_ReqReason = e_NotHearBeacon;
        //RecordAreaNotifyBuff(e_Clnnib,e_NotHearBeacon,nnib.DestSnid,0,e_NODE_OFF_LINE);
        ClearNNIB();
        return;
    }
    if(nnib.beaconProtect >0) //此机制为保护STA一段时间内未发送信标
    {

    }
    else
    {

    }

    //无线链路四个路由周期未收到发现列表，主动离线。
    if(nnib.LinkType == e_RF_Link)
    {
        if(nnib.RfRecvDisListTime > 0)
        {
            nnib.RfRecvDisListTime--;
//            net_printf("RfRecvDisListTime-%d\n",nnib.RfRecvDisListTime);
        }
        else
        {
            if(SlaveUpgradeFileInfo.StaUpgradeStatus != e_UPGRADE_IDLE)//HRF 台体升级
            {
                nnib.RfRecvDisListTime ++;
            }
            else
            {
                net_printf("------------------RfRecvDisListTime------------------\n");
                g_ReqReason = e_NohearRfDisList;
                ClearNNIB();
            }
        }
    }
//    static U16 SucRateZeroCnt = 0;
////    net_printf("SucRateZeroCnt : %d ProxyNodeUplinkRatio:%d, ProxyNodeDownlinkRatio:%d\n", SucRateZeroCnt, nnib.ProxyNodeUplinkRatio , nnib.ProxyNodeDownlinkRatio);
//    if(nnib.NodeState == e_NODE_ON_LINE && nnib.SuccessRateZerocnt != 0 && (nnib.ProxyNodeUplinkRatio == 0 && nnib.ProxyNodeDownlinkRatio == 0))
//    {
//        SucRateZeroCnt++;
//        if(SucRateZeroCnt >= nnib.SuccessRateZerocnt)
//        {
//            net_printf("------------------SucRateZeroCnt=%d------------------\n", SucRateZeroCnt);
//            ClearNNIB(); //<<< STA升级测试
//            SucRateZeroCnt = 0;
//        }
//    }
//    else
//    {
//        SucRateZeroCnt = 0;
//    }

    if( nnib.Networkflag != TRUE)
        return;
    if(nnib.LinkType == e_HPLC_Link)
    {
        if(nnib.RecvDisListTime >0)//4个周期没有听到发现列表  ,nnib.RecvDisListTime参数更新时需要判断接收到的邻居节点位图中是否有自己
        {
            nnib.RecvDisListTime--;
            /*if(nnib.RecvDisListTime == nnib.RoutePeriodTime || nnib.RecvDisListTime == nnib.RoutePeriodTime * 2)
            {
                SendProxyChangeReq(e_NoRecvDisList);
            }*/

        }
        else
        {
            net_printf("------------------RecvDisListTime------------------\n");
            g_ReqReason = e_NohearDisList;
            //RecordAreaNotifyBuff(e_Clnnib,e_NohearDisList,nnib.DestSnid,0,e_NODE_OFF_LINE);

            ClearNNIB();
        }
    }

}

/************不同CCO帧控制和paload异常拼帧处理***************/
U8 MacConflictNum = 0;
ostimer_t *MacConflict_timer = NULL;
void MacConflict_timerCB(struct ostimer_s *ostimer, void *arg)
{
     MacConflictNum = 0;
}
static void modify_MacConflict_timer(uint32_t Sec)
{
    if(Sec == 0)
        return;

    if(MacConflict_timer == NULL)
    {
        MacConflict_timer = timer_create(Sec*100,
                                 0,
                                 TMR_TRIGGER ,//TMR_TRIGGER
                                 MacConflict_timerCB,
                                 NULL,
                                 "MacConflict_timerCB"
                                );
    }
    else
    {
        timer_modify(MacConflict_timer,
                 Sec*100,
                 10,
                 TMR_TRIGGER ,//TMR_TRIGGER
                 MacConflict_timerCB,
                 NULL,
                 "MacConflict_timerCB"
                 , TRUE);
    }

    if(MacConflict_timer == NULL)
    {
         sys_panic("MacConflict_timer is null\n");
    }
    timer_start(MacConflict_timer);

    return;
}
 
U8 NidChangeNum = 0;
ostimer_t *NidChange_timer = NULL;
void NidChange_timerCB(struct ostimer_s *ostimer, void *arg)
{
    NidChangeNum = 0;
}
static void modify_NidChange_timer(uint32_t Sec)
{
    if(Sec == 0)
        return;

    if(NidChange_timer == NULL)
    {
        NidChange_timer = timer_create(Sec*1000,
                                 0,
                                 TMR_TRIGGER ,//TMR_TRIGGER
                                 NidChange_timerCB,
                                 NULL,
                                 "NidChange_timerCB"
                                );
    }
    else
    {
        timer_modify(NidChange_timer,
                 Sec*1000,
                 10,
                 TMR_TRIGGER ,//TMR_TRIGGER
                 NidChange_timerCB,
                 NULL,
                 "NidChange_timerCB"
                 , TRUE);
    }
    if(NidChange_timer == NULL)
    {
        sys_panic("NidChange_timer is null\n");
    }
    timer_start(NidChange_timer);

    return;
}



 //由信标触发的在线管理,返回值FALSE表示处理信标时，执行完本函数结束，TRUE相反
U8 STA_JudgeOfflineBybeacon(U8 FormationSeqNumber,U8 *CCOmacaddr,U16 scrtei,U16 level ,U8 beacontype, mbuf_t *buf)
{

    //net_printf("SNID = %08x, GetSNID() = %08x, nodeState = %02x, beacontype = %d\n", SNID, GetSNID(), GetNodeState(), beacontype);

    if(GetNodeState() != e_NODE_ON_LINE )
    {
        if(GetSNID() == buf->snid)
        {
            return TRUE;//未入网节点需要启动入网，有后续动作
        }
        else
        {
            return FALSE;//不是入网的网络则无后续动作
        }
    }
    //在线模块管理自身状态
    if( buf->snid ==GetSNID() ) //收到本网络信标
    {
        nnib.StaOfflinetime = nnib.RoutePeriodTime*2; //听到信标后更新自己的生存时间 更新离线计数
        if(memcmp(CCOmacaddr, nnib.CCOMacAddr, 6) != 0 )//网络冲突
        {
            if(MacConflictNum == 0)
            {
                modify_MacConflict_timer(ERR_BEACON_CLEAN_TIME);
            }

            MacConflictNum++;
            if(MacConflictNum >= REV_ERR_BEACON_NUM)
            {
                //启动上报定时器,启动离线定时器，连续上报一路由周期后离线
                SendNetworkConflictReport(CCOmacaddr, 0, TRUE);
                MacConflictNum = 0;
            }
            
        }
        if((memcmp(CCOmacaddr, nnib.CCOMacAddr, 6) == 0 ) && FormationSeqNumber != nnib.FormationSeqNumber)//组网序号变化
        {
            g_ReqReason = e_FormationSeq;
            net_printf("!e_FormationSeq\n");
            ClearNNIB();
            return FALSE;
        }
        if(scrtei ==GetProxy()) //收到父节点信标：更新层级,更新自身状态
        {
            //判断层级
            nnib.NodeLevel = level+1;
            if(nnib.NodeLevel > MaxDeepth)
            {
                g_ReqReason = e_LevHig;
                net_printf("!e_LevHig\n");
                ClearNNIB();
                return FALSE;
            }
            //判断角色,父节点若连续1/10路由周期为发现节点则在回调函数中离线
            if(beacontype == e_DISCOVERY_BEACON)
            {
                net_printf("prox is Sta\n");
                //启动定时器
                if(JudgeProxyTypetimer == NULL || TMR_STOPPED == zc_timer_query(JudgeProxyTypetimer)  )
                {
                    modify_JudgeProxyTypetimer(nnib.RoutePeriodTime*100);
                }
            }
            else
            {
                //关闭定时器
                if(JudgeProxyTypetimer != NULL && TMR_RUNNING == zc_timer_query(JudgeProxyTypetimer))
                {
                    timer_stop(JudgeProxyTypetimer, TMR_NULL);
                }
            }
        }
        return TRUE;
    }
    else
    {
        if( memcmp(CCOmacaddr, nnib.CCOMacAddr, 6) == 0)//snid变化,此情况是CCO收到冲突后，调整自身SNID导致
        {
            if(NidChangeNum == 0)
                modify_NidChange_timer(ERR_BEACON_CLEAN_TIME);

            NidChangeNum++;
            if(NidChangeNum >= REV_ERR_BEACON_NUM)
            {
                NidChangeNum = 0;
                
                g_ReqReason = e_CCOnidChage;
                net_printf("!e_CCOnidChage\n");
                ClearNNIB();
            }
        }
        else
        {
            if(buf->LinkType == e_RF_Link)       //无线信道冲突上报
            {
                if(TMR_RUNNING == timer_query(&g_ChangeChlTimer))       //如果正在切换信道不上报冲突
                {
                    net_printf("RfBand chg now\n");
                    return FALSE;
                }
                if(getHplcOptin() != buf->rfoption || getHplcRfChannel() != buf->rfchannel)     //链路层信道已经切换，物理层还没有切换时，不进行冲突上报。
                {
                    net_printf("RfBand chgd net\n");
                    return FALSE;
                }
                SendMMeRFChannelConflictReport(CCOmacaddr, 0, TRUE, buf->rfoption, buf->rfchannel);
            }
        }
        return FALSE;
    }
}

///////////////////////////STA的心跳\发现列表(CCO也使用)\通信成功率上报////////////////////////////////



ostimer_t	*sendheartbeattimer=NULL;
ostimer_t	*SucessReporttimer=NULL;

static void modify_heatrbeattimer(uint32_t first);
static void modify_SucessReporttimer(uint32_t first);


//判断是否有发送心跳的资格
static U8 IsNeedHeartBreak()
{
	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    if(nnib.NodeType == e_PCO_NODETYPE)
    {
        
		list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
		{
            DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->Relation == e_CHILD)
            {

                if(DiscvryTable->NodeType == e_PCO_NODETYPE )
                {
                    return FALSE;
                }
            }

        }

        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->Relation == e_CHILD)
            {

                if(DiscvryTable->NodeType == e_PCO_NODETYPE )
                {
                    return FALSE;
                }
            }

        }
        
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}


static void sendheartbeattimerCB( struct ostimer_s *ostimer, void *arg)
{
    if(nnib.NodeType == e_PCO_NODETYPE)
    {
        //发送心跳
      //  SendHeartBeatCheck(FALSE);
        work_t *work = zc_malloc_mem(sizeof(work_t), "SHTBT", MEM_TIME_OUT);
    	work->work = SendHeartBeatCheck;
		work->msgtype = SEND_HTBT;
    	post_datalink_task_work(work);
    }
    modify_heatrbeattimer(nnib.HeartBeatTime/**1000*/);

}

static void modify_heatrbeattimer(uint32_t first)
{
    if(sendheartbeattimer == NULL && first == 0)
    {
    	net_printf("waring:sendheartbeattimer\n");
     	return;	
    }
       

    if(sendheartbeattimer == NULL)
    {
        sendheartbeattimer = timer_create(
                   first,
                   first,
                   TMR_TRIGGER,//TMR_TRIGGER
                   sendheartbeattimerCB,
                   NULL,
                   "sendheartbeattimerCB"
                   );
    }
    if(first == 0) //0直接进入回调
    {
        if(zc_timer_query(sendheartbeattimer) ==TMR_STOPPED)
        {
            timer_start(sendheartbeattimer);
        }
    //    timer_stop(sendheartbeattimer, TMR_CALLBACK);

    }
    else
    {
        timer_modify(sendheartbeattimer,
                first,
                first,
                TMR_TRIGGER ,//TMR_TRIGGER
                sendheartbeattimerCB,
                NULL,
                "sendheartbeattimerCB",
                TRUE);
        timer_start(sendheartbeattimer);
    }

}


static void SucessReporttimerCB( struct ostimer_s *ostimer, void *arg)
{
    //SendSuccessRateReport();
    work_t *work = zc_malloc_mem(sizeof(work_t), "SSUC", MEM_TIME_OUT);
    work->work = SendSuccessRateReport;
	work->msgtype = SEND_SUC;
    post_datalink_task_work(work);



    modify_SucessReporttimer(nnib.SuccessRateReportTime*1000);
}

static void modify_SucessReporttimer(uint32_t first)
{
    if(SucessReporttimer == NULL && first == 0)
    {
    	net_printf("waring:sendheartbeattimer\n");
		return;
    }


    if(SucessReporttimer == NULL)
    {
        SucessReporttimer = timer_create(
                   first,
                   first,
                   TMR_TRIGGER,//TMR_TRIGGER
                   SucessReporttimerCB,
                   NULL,
                   "SucessReporttimerCB"
                   );
    }
    if(first == 0) //0直接进入回调
    {
        if(zc_timer_query(SucessReporttimer) ==TMR_STOPPED)
        {
            timer_start(SucessReporttimer);
        }
        // timer_stop(SucessReporttimer, TMR_CALLBACK);

    }
    else
    {
        timer_modify(SucessReporttimer,
                first,
                first,
                TMR_TRIGGER ,//TMR_TRIGGER
                SucessReporttimerCB,
                NULL,
                "SucessReporttimerCB",
                TRUE);
        timer_start(SucessReporttimer);
    }

}





//心跳管理启动
void StartMmHeartBeat()
{  
	if(IsNeedHeartBreak())
	{
       modify_heatrbeattimer(rand()%(nnib.HeartBeatTime/**1000*/)+1); //首次1/8路由周期内随机
	}
	else
	{
       modify_heatrbeattimer(nnib.HeartBeatTime/**2000*/+(15-nnib.NodeLevel)*2000+1); //不满足发送心跳的条件时，等待1/4路由周期+层级
	}
    
}



//启动通信成功率上报
void StartMmSucessReport()
{
    modify_SucessReporttimer(rand()%(nnib.SuccessRateReportTime*1000)+1);
}
void StartNetworkMainten(U8 flag)
{
    if(HPLC.scanFlag == FALSE)      //台体测试时，解决台体发送信标携带组网完成标志不为1的问题。
    {
        static U8 cnt = 0;

        if(cnt < 5)  //为了HRF台体,前五个信标周期，不发发现列表及心跳等
        {
            cnt++;
            return ;
        }
        else
        {
            flag = TRUE;
        }
    }

    if( nnib.Networkflag != flag  && flag==TRUE)
	{
		StartMmHeartBeat();
		StartMmDiscovery();
		StartMmSucessReport();
        StartRfDiscovery();
	}

	nnib.Networkflag =flag;	
}

void UpDataHeartBeat()
{
   if(sendheartbeattimer != NULL)
   {
        if(zc_timer_query(sendheartbeattimer) == TMR_RUNNING)
        {
            modify_heatrbeattimer(nnib.HeartBeatTime/**2000*/+(15-nnib.NodeLevel)*2000);
        }
   }
}

/**
 * @brief   show neigborNet Info
 * 
 */
void Sta_Show_NbNet()
{
    U8	i,jj;
    jj = 0;
    net_printf("NbSNID\t\tLifeTime\tSameRatio\tDiffTime\tBlacktime\tErrCode\t\tband\toption\trfchannel\n");
    for(i=0;i<MaxSNIDnum;i++)
     {

        if(StaNeigborNet_t[i].NbSNID !=0xffffffff)
        {

            net_printf("%08x\t%d\t\t%d\t\t%d\t\t%d\t\t%s\t%d\t%02x\t%03x\n",
            StaNeigborNet_t[i].NbSNID,
            StaNeigborNet_t[i].LifeTime,
            StaNeigborNet_t[i].SameRatio,
            StaNeigborNet_t[i].DiffTime,
            StaNeigborNet_t[i].Blacktime,
            StaNeigborNet_t[i].ErrCode==0?"vailbsnid":
            StaNeigborNet_t[i].ErrCode==e_NotInlist?"e_NotInlist":
            StaNeigborNet_t[i].ErrCode==e_BadLink?"e_BadLink":"Uk",
            StaNeigborNet_t[i].bandidx,
            StaNeigborNet_t[i].option,
            StaNeigborNet_t[i].RfChannel);
            jj++;
        }
    }
    net_printf("StaNeigborNet_t num : %d\n",jj);
}

/********************************Neighbornet func**********************************/
 //更新邻居网络时仅仅更新生存时间,SNID不存在表内则新增
 //更新邻居网络信息中的生存时间
 void STA_UpDataNeighborNetTime()
 {
     U8  i = 0;
     for(i = 0; i < MaxSNIDnum; i++)
     {
         if(StaNeigborNet_t[i].NbSNID != 0xffffffff)
         {
             if(StaNeigborNet_t[i].LifeTime > 0)
             {
                 StaNeigborNet_t[i].LifeTime--;
                 
                if(StaNeigborNet_t[i].Blacktime > 0)
                    StaNeigborNet_t[i].Blacktime--;
                else
                    StaNeigborNet_t[i].ErrCode=0;
             }
             else
             {
                 memset(&StaNeigborNet_t[i], 0xff, sizeof(STA_NEIGHBOR_NET_t));
             }
         }
     }


 }


 void AddNeighborNet(mbuf_t *buf,U8 type)
{
    U16 i;
    U8 nonpos=0xff;
    if( buf->snid == 0 || buf->snid == 0xffffff)
    {
        return ;
    }
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(StaNeigborNet_t[i].NbSNID == buf->snid)
        {
            StaNeigborNet_t[i].LifeTime = SNID_LIFE_TIME;
            if(type == DT_BEACON && buf->LinkType == e_HPLC_Link)
            {
                StaNeigborNet_t[i].bandidx = buf->bandidx; //GetHplcBand();
            }
            if(buf->LinkType == e_RF_Link && 
                ((type == DT_BEACON) || (StaNeigborNet_t[i].option == 0x0f && StaNeigborNet_t[i].RfChannel == 0xfff)))
            {
                StaNeigborNet_t[i].option   = buf->rfoption; 
                StaNeigborNet_t[i].RfChannel = buf->rfchannel; 
            }
            nonpos=0xee;
            break;
        }
        else if(StaNeigborNet_t[i].NbSNID == 0xffffffff)
        {
            nonpos = (nonpos==0xff?i:nonpos);
        }
    }
    if(i==MaxSNIDnum)
    {
        nonpos = (nonpos==0xff?0:nonpos);
        StaNeigborNet_t[nonpos].NbSNID = buf->snid;
        StaNeigborNet_t[nonpos].LifeTime = SNID_LIFE_TIME;
        StaNeigborNet_t[nonpos].SameRatio =0;
        StaNeigborNet_t[nonpos].DiffTime=0;
        StaNeigborNet_t[nonpos].Blacktime =0;
        StaNeigborNet_t[nonpos].ErrCode=0;
        if(buf->LinkType == e_HPLC_Link)
        {
            StaNeigborNet_t[nonpos].bandidx = buf->bandidx; //GetHplcBand();
        }
        else if(buf->LinkType == e_RF_Link)
        {
            StaNeigborNet_t[nonpos].option   = buf->rfoption; 
            StaNeigborNet_t[nonpos].RfChannel = buf->rfchannel; 
        }
    	 
    }	 
 }
 /**
 * @brief updateNeiNetRfParam       更新网络无线信道参数
 * @param snid                      目标网络SNID号
 * @param option                    无线信道参数 option
 * @param channel                   无线信道编号
 */
void updateNeiNetRfParam(U32 snid, U8 option, U16 channel)
{
    U8 i;
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(StaNeigborNet_t[i].NbSNID == snid)
        {
            StaNeigborNet_t[i].option = option;
            StaNeigborNet_t[i].RfChannel = channel;
            //设置当前频点存在网络标志。
            setRfChlHaveNetFlag(option, channel);
            return;
        }
    }


//    net_printf("Not Find snid : %08x\n", snid);

    return;
}

//static void freeBlackNetForTest()
//{
//    U8 i;
//    U8 netCnt = 0;
//    U8 findIndex = 0;
//    for(i=0;i<MaxSNIDnum;i++)
//    {
//        if(StaNeigborNet_t[i].NbSNID ){
//            netCnt++;
//            findIndex = i;
//        }
//    }

//    if(netCnt == 1 && StaNeigborNet_t[findIndex].ErrCode == e_BadLink)
//    {
//        StaNeigborNet_t[findIndex].ErrCode = 0;
//        StaNeigborNet_t[findIndex].Blacktime = 0;

//    }



//}



//返回值 TRUE:不在黑名单      -------FALSE:在黑名单中
U8 CheckBlackNegibor(U32 SNID)
{
    U8 i;

//    freeBlackNetForTest();  //<<< for rf test

    for(i=0;i<MaxSNIDnum;i++)
    {
        if(StaNeigborNet_t[i].NbSNID != SNID)
            continue;

//        if(StaNeigborNet_t[i].bandidx != GetHplcBand())
//            return FALSE;

        if (StaNeigborNet_t[i].ErrCode != 0)
	    {
			return FALSE;
        }

//        if(SNID != 0x03)
//            return FALSE;
    }
	return TRUE;
}

/**
 * @brief CheckNbNetBaud    选择网络是用来判断当前选择网络频段是否和当前频段一致
 * @param SNID
 * @return  一致：TRUE ， 不一致：FALSE
 */
U8 CheckNbNetHPLCBaud(U32 SNID)
{
    U8 i;

    for(i=0;i<MaxSNIDnum;i++)
    {
        if(StaNeigborNet_t[i].NbSNID == SNID)
        {
            if (StaNeigborNet_t[i].bandidx == GetHplcBand())
            {
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}
/**
 * @brief CheckNbNetRfBaud  选择网路时用来判断当前网络信道是否跟当前信道一致
 * @param SNID
 * @return  一致：TRUE ， 不一致：FALSE
 */
U8 CheckNbNetHRfBaud(U32 SNID)
{
    U8 i;

    for(i=0;i<MaxSNIDnum;i++)
    {
        if(StaNeigborNet_t[i].NbSNID == SNID)
        {
            if ((StaNeigborNet_t[i].option == getHplcOptin() && StaNeigborNet_t[i].RfChannel == getHplcRfChannel())
                    || StaNeigborNet_t[i].bandidx == 0xff)      //选择当前信道或者时只有无线能听到的网络进行入网。
            {
                return TRUE;
            }
            break;
        }
    }
    return FALSE;
}

/**
 * @brief NoHaveAvliableNet     查看当前载波频段是否还有可用网络
 * @return
 */
U8 NoHaveAvliableNet()
{
	U8 i;
	for(i=0;i<MaxSNIDnum;i++)
    {
		//有可用网络
        if ( StaNeigborNet_t[i].ErrCode == 0 &&StaNeigborNet_t[i].NbSNID != 0xffffffff
            && GetHplcBand() == StaNeigborNet_t[i].bandidx)
        {
			return FALSE;
        }

    }
	return TRUE;
}
/**
 * @brief NoHaveHrfNet  查看当前无线信道上是否还有可用网络
 * @return
 */
U8 NoHaveHrfNet()
{
    U8 i;
    for(i=0;i<MaxSNIDnum;i++)
    {
        //有可用网络
        if ( StaNeigborNet_t[i].ErrCode == 0 &&StaNeigborNet_t[i].NbSNID != 0xffffffff
            && getHplcOptin() == StaNeigborNet_t[i].option && getHplcRfChannel() == StaNeigborNet_t[i].RfChannel)
        {
            return FALSE;
        }

    }
    return TRUE;
}

//static __SLOWTEXT U8 checkSnidInHPLCNeibor(U16 snid)
//{
//    list_head_t *pos, *n;
//    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pNodeInfo;
//    list_for_each_safe(pos, n, &NeighborDiscoveryHeader.link)
//    {
//        pNodeInfo = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
//        if(pNodeInfo->SNID == snid)
//            return TRUE;
//    }

//    return FALSE;
//}

void FreeBadlinkNid(U8 bandidx)
{
	U8 i;
    for(i=0;i<MaxSNIDnum;i++)
    {

        if(StaNeigborNet_t[i].ErrCode == e_BadLink && StaNeigborNet_t[i].NbSNID !=0xffffffff && StaNeigborNet_t[i].bandidx ==bandidx)
		{
            //if(checkSnidInHPLCNeibor(StaNeigborNet_t[i].NbSNID))            //<<< for rf Test
            {
                StaNeigborNet_t[i].ErrCode =0;
                StaNeigborNet_t[i].Blacktime =0;
                // memset((U8*)&StaNeigborNet_t[i],0xff,sizeof(STA_NEIGHBOR_NET_t) );
            }

		}
    }
}

void FreeHRfBadLinkNid()
{
    U8 i;
    for(i=0;i<MaxSNIDnum;i++)
    {
        if(StaNeigborNet_t[i].ErrCode == e_BadLink && StaNeigborNet_t[i].NbSNID !=0xffffffff && \
                StaNeigborNet_t[i].option == getHplcOptin() && StaNeigborNet_t[i].RfChannel == getHplcRfChannel()
                && StaNeigborNet_t[i].bandidx == 0xff)
        {
            StaNeigborNet_t[i].ErrCode =0;
            StaNeigborNet_t[i].Blacktime =0;
            // memset((U8*)&StaNeigborNet_t[i],0xff,sizeof(STA_NEIGHBOR_NET_t) );
        }

    }
}

void AddBlackNeigbor(U32 SNID, U32 ReassociaTime, U8 Reason)
{
    extern sfo_handle_t HSFO;
    U16 i;
    U8 firstIndex = 0xFF;

    Change_next_net_option();

    for(i=0;i<MaxSNIDnum;i++)
    {
        if(StaNeigborNet_t[i].NbSNID == SNID)
        {
            StaNeigborNet_t[i].Blacktime = ReassociaTime;
            StaNeigborNet_t[i].NbSNID =SNID;
            StaNeigborNet_t[i].ErrCode = Reason;
            //StaNeigborNet_t[i].bandidx = GetHplcBand();
            net_printf("flush black =%08x %d S  ErrCode = %d\n,",SNID,ReassociaTime, Reason);
            firstIndex = 0xFF;
            break;
        }
        else if(StaNeigborNet_t[i].NbSNID == 0xffffffff)
        {
            if(firstIndex == 0xFF)
                firstIndex = i;
        }
   }
    if(firstIndex != 0xFF)
    {
        StaNeigborNet_t[firstIndex].Blacktime = ReassociaTime;
        StaNeigborNet_t[firstIndex].NbSNID =SNID;
        StaNeigborNet_t[firstIndex].ErrCode = Reason;
//		StaNeigborNet_t[firstIndex].bandidx = GetHplcBand();
        net_printf("add black =%08x %d S  ErrCode = %d\n,",SNID,ReassociaTime, Reason);
    }
	if(NoHaveAvliableNet() && NoHaveHrfNet()) //在某一个频段无可用网络时，切换频段，并且将e_BadLink类型的黑名单清空，等待下次有机会转过来重新加入。
	{		
        net_printf("NoHaveHplcNet\n");
        ScanBandCallBackNow();
        RfScanCallBackNow();
	}
    else
    {
        net_printf("this band has other net,recover power\n");
        changepower(HPLC.band_idx);//恢复功率
        modify_network_timer(0);
    }

    // if(NoHaveHrfNet()) //在某一个频段无可用网络时，切换频段，并且将e_BadLink类型的黑名单清空，等待下次有机会转过来重新加入。
    // {
    //     net_printf("NoHaveHrfNet\n");
    //     RfScanCallBackNow();
    // }

    return;
}


//U8 GetNeighborNetNum()
//{
//    U8 i,num=0;
//    for(i=0;i<MaxSNIDnum;i++)
//    {
//        if(StaNeigborNet_t[i].NbSNID != 0xffffffff)
//        {
//            num++;
//        }
//    }
//    return num;

//}

#endif






void StopNetworkMainten()
{
    if(Discoverytimer !=NULL)
        timer_stop(Discoverytimer, TMR_NULL);
    if(RfDiscoverytimer != NULL)
        timer_stop(RfDiscoverytimer, TMR_NULL);
#if defined(STATIC_NODE)
    if(sendheartbeattimer !=NULL)
        timer_stop(sendheartbeattimer, TMR_NULL);
    if(SucessReporttimer !=NULL)
        timer_stop(SucessReporttimer, TMR_NULL);
#endif
}

/*函数名称：维护和邻居点的关系属性
通过接收到发现列表调用此函数
*/
void UpDataNeighborRelation(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry, U16 proxyTEI, U8 Level,U8 UplinkRatio,U8 DownlinkRatio)
{
    //子节点有可能使用代理变更,做为代理节点需要判断自己的子节点是否发生变化
    TableEntry->BKRouteFg = TRUE;

    #if 1
    if((TableEntry->Relation == e_CHILD && proxyTEI == GetTEI()) || 
        (TableEntry->Relation == e_PROXY && GetProxy() == TableEntry->NodeTEI))  //子节点父节点角色信息记录无误
    {
        if(TableEntry->Relation == e_CHILD)     //保存子节点的上下行通信成功率，等待上报
        {
            TableEntry->childUpRatio   = UplinkRatio;
            TableEntry->childDownRatio = DownlinkRatio;
            TableEntry->HplcInfo.UplinkSuccRatio = DownlinkRatio;
            TableEntry->HplcInfo.DownlinkSuccRatio = UplinkRatio;
        }
    }
    else        //更新角色信息
    {
        if(proxyTEI == GetTEI())
        {
            TableEntry->Relation = e_CHILD;
        }
        else if (GetProxy() == TableEntry->NodeTEI)
        {
            TableEntry->Relation = e_PROXY;
        }
        else
        {
            if ( (Level+1) == nnib.NodeLevel   )
            {
                TableEntry->Relation = e_UPLEVEL; //上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if(Level == nnib.NodeLevel)
            {
                TableEntry->Relation = e_SAMELEVEL; //同级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if ( (Level+2) <= nnib.NodeLevel )
            {
                TableEntry->Relation =   e_UPPERLEVEL;//上上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else
            {
                TableEntry->Relation =   e_EERROUTE;//错误的路由
                TableEntry->BKRouteFg = FALSE;


            }
        }
    }
    #else
    if(TableEntry->Relation == e_CHILD)
    {
        if(proxyTEI != GetTEI())//如果已经发生变化
        {
            if ( (Level+1) == nnib.NodeLevel   )
            {
                TableEntry->Relation = e_UPLEVEL; //上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if(Level == nnib.NodeLevel)
            {
                TableEntry->Relation = e_SAMELEVEL; //同级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if ( (Level+2) <= nnib.NodeLevel )
            {
                TableEntry->Relation =   e_UPPERLEVEL;//上上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else
            {
                TableEntry->Relation =   e_EERROUTE;//错误的路由
                TableEntry->BKRouteFg = FALSE;


            }
        }
        else //保存子节点的上下行通信成功率，等待上报
        {
            TableEntry->childUpRatio   = UplinkRatio;
            TableEntry->childDownRatio = DownlinkRatio;
            TableEntry->HplcInfo.UplinkSuccRatio = UplinkRatio;
            TableEntry->HplcInfo.DownlinkSuccRatio = DownlinkRatio;
        }

    }
    //将邻居表中e_NEIGHBER型的邻居重新定义
    else if (TableEntry->Relation != e_PROXY) //怀疑拓扑拉长和对邻居的关系定义不及时更新有关
    {
        if(GetProxy() == TableEntry->NodeTEI)
        {
            TableEntry->Relation = e_PROXY;
        }
        else if (Level +1== nnib.NodeLevel   )
        {
            TableEntry->Relation = e_UPLEVEL; //上级备份路由
//            TableEntry->BKRouteFg = TRUE;
        }
        else if(Level == nnib.NodeLevel)
        {

            TableEntry->Relation = e_SAMELEVEL; //同级备份路由
//            TableEntry->BKRouteFg = TRUE;
        }
        else if ( (Level+2) <= nnib.NodeLevel )
        {
            TableEntry->Relation =	e_UPPERLEVEL;//上上级备份路由
//            TableEntry->BKRouteFg = TRUE;
        }
        else
        {
            TableEntry->Relation =	e_EERROUTE;//错误的路由
            TableEntry->BKRouteFg = FALSE;
        }

    }
    #endif

}

__SLOWTEXT void UpDataRfNeighborRelation(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry, U16 proxyTEI, U8 Level,U8 UplinkRatio,U8 DownlinkRatio)
{
    //子节点有可能使用代理变更,做为代理节点需要判断自己的子节点是否发生变化
    TableEntry->BKRouteFg = TRUE;

    #if 1
    if((TableEntry->Relation == e_CHILD && proxyTEI == GetTEI()) || 
        (TableEntry->Relation == e_PROXY && GetProxy() == TableEntry->NodeTEI))  //子节点父节点角色信息记录无误
    {
        if(TableEntry->Relation == e_CHILD)     //保存子节点的上下行通信成功率，等待上报
        {
            TableEntry->childUpRatio  =UplinkRatio;              
            TableEntry->childDownRatio=DownlinkRatio; 
        }
    }
    else        //更新角色信息
    {
        if(proxyTEI == GetTEI())
        {
            TableEntry->Relation = e_CHILD;
        }
        else if (GetProxy() == TableEntry->NodeTEI)
        {
            TableEntry->Relation = e_PROXY;
        }
        else
        {
            if ( (Level+1) == nnib.NodeLevel   )
            {
                TableEntry->Relation = e_UPLEVEL; //上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if(Level == nnib.NodeLevel)
            {
                TableEntry->Relation = e_SAMELEVEL; //同级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if ( (Level+2) <= nnib.NodeLevel )
            {
                TableEntry->Relation =   e_UPPERLEVEL;//上上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else
            {
                TableEntry->Relation =   e_EERROUTE;//错误的路由
                TableEntry->BKRouteFg = FALSE;


            }
        }
    }
    #else

    if(TableEntry->Relation == e_CHILD)
    {
        if(proxyTEI != GetTEI())//如果已经发生变化
        {
            if ( (Level+1) == nnib.NodeLevel   )
            {
                TableEntry->Relation = e_UPLEVEL; //上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if(Level == nnib.NodeLevel)
            {
                TableEntry->Relation = e_SAMELEVEL; //同级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else if ( (Level+2) <= nnib.NodeLevel )
            {
                TableEntry->Relation =   e_UPPERLEVEL;//上上级备份路由
//                TableEntry->BKRouteFg = TRUE;
            }
            else
            {
                TableEntry->Relation =   e_EERROUTE;//错误的路由
                TableEntry->BKRouteFg = FALSE;


            }
        }
        else //保存子节点的上下行通信成功率，等待上报
        {
            TableEntry->childUpRatio  =UplinkRatio;              
            TableEntry->childDownRatio=DownlinkRatio; 
        }

    }
    //将邻居表中e_NEIGHBER型的邻居重新定义
    else if (TableEntry->Relation != e_PROXY) //怀疑拓扑拉长和对邻居的关系定义不及时更新有关
    {
        if(GetProxy() == TableEntry->NodeTEI)
        {
            TableEntry->Relation = e_PROXY;
        }
        else if (Level +1== nnib.NodeLevel   )
        {
            TableEntry->Relation = e_UPLEVEL; //上级备份路由
//            TableEntry->BKRouteFg = TRUE;
        }
        else if(Level == nnib.NodeLevel)
        {

            TableEntry->Relation = e_SAMELEVEL; //同级备份路由
//            TableEntry->BKRouteFg = TRUE;
        }
        else if ( (Level+2) <= nnib.NodeLevel )
        {
            TableEntry->Relation =	e_UPPERLEVEL;//上上级备份路由
//            TableEntry->BKRouteFg = TRUE;
        }
        else
        {
            TableEntry->Relation =	e_EERROUTE;//错误的路由
            TableEntry->BKRouteFg = FALSE;
        }

    }
    #endif

}



#if defined(STATIC_MASTER)

//CCO的路由周期设置
void selfAdaptionRoutePeriodTime()
{
    U16 nodenum;
    nodenum = GetDeviceNum();
    SetRoutePeriodTime( nnib.RoutePeriodTime == RoutePeriodTime_Test  ? RoutePeriodTime_Test:
                        nodenum<100  ? RoutePeriodTime_Normal:
						nodenum <120 ? RoutePeriodTime_150:
                        //nodenum <300 ? RoutePeriodTime_100_300:
                        //nodenum <500 ? RoutePeriodTime_300_500:
                        nodenum <130 ? RoutePeriodTime_100_300:
                        nodenum <140 ? RoutePeriodTime_300_500:
                        RoutePeriodTime_500);

    SetNextRoutePeriodTime(nnib.RoutePeriodTime);
}

#endif


#if defined(STATIC_NODE)

/*函数名称:为入网的节点选择代理
根据通讯成功率和层级数来设定代理节点
*/
void SelectBetterProxyNode()
{
	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    //通信成功率不合格更换代理节点
    if( ((nnib.ProxyNodeDownlinkRatio <  DOWN_LINK_LOWEST_RATIO)  || (nnib.ProxyNodeUplinkRatio <  UP_LINK_LOWEST_RATIO))&&
            ((nnib.LProxyNodeDownlinkRatio <  DOWN_LINK_LOWEST_RATIO)  || (nnib.LProxyNodeUplinkRatio <  UP_LINK_LOWEST_RATIO))
            )
    {
        //载波入网，或者是无线入网并接收到足够的无线发现列表
        if(nnib.LinkType == e_HPLC_Link || (nnib.LinkType == e_RF_Link && nnib.RfOnlyFindBtPco == FALSE))
        {
            net_printf("lk:%d,Ratio:%d,%d,%d,%d\n"
                        , nnib.LinkType
                        , nnib.ProxyNodeDownlinkRatio
                        , nnib.ProxyNodeUplinkRatio
                        , nnib.LProxyNodeDownlinkRatio
                        , nnib.LProxyNodeUplinkRatio );
            sendproxychangeDelay(e_CommRateLow);
            return;
        }
        else
        {
            sendproxychangeDelay(e_FindBetterProxy); 
        }
    }
    else
    {
        
        //选取通信质量较好的载波节点作为新的代理
		list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            //邻居表中有符合要求且等级比当前父节点更低
            DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if( /*(DiscvryTable->NodeDepth < (nnib.NodeLevel-1)) && */ 
                    (DiscvryTable->SNID == GetSNID())
                 )
            {
                if(!check_Proxy_is_nice(e_FindBetterProxy, DiscvryTable->NodeDepth, e_HPLC_Link))
                {
                    continue;
                }

                if(DiscvryTable->HplcInfo.DownlinkSuccRatio >NICE_SUCC_RATIO &&
                   DiscvryTable->HplcInfo.UplinkSuccRatio >NICE_SUCC_RATIO&&
                   DiscvryTable->HplcInfo.LastDownlinkSuccRatio >NICE_SUCC_RATIO &&
                   DiscvryTable->HplcInfo.LastUplinkSuccRatio >NICE_SUCC_RATIO)
                {
                    net_printf("PlcBtrPco\n");
#if defined(STD_2016)
                    sendproxychangeDelay(e_FindBetterProxy);	//优化拓扑
#elif defined(STD_GD)
                    GD_SendProxyChangeReq(e_FindBetterProxy);
#endif
                    return;
                }
            }
        }

        //选取通信成功率较高的无线节点作为新的代理

        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            //邻居表中有符合要求且等级比当前父节点更低
            DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if( /*(DiscvryTable->NodeDepth < (nnib.NodeLevel-1))&&*/
                (DiscvryTable->SNID == GetSNID()) )
            {
                if(!check_Proxy_is_nice(e_FindBetterProxy, DiscvryTable->NodeDepth, e_RF_Link))
                {
                    continue;
                }

                if(DiscvryTable->HrfInfo.UpRcvRate >NICE_SUCC_RATIO &&
                   DiscvryTable->HrfInfo.DownRcvRate >NICE_SUCC_RATIO &&
                   DiscvryTable->HrfInfo.LUpRcvRate >NICE_SUCC_RATIO &&
                   DiscvryTable->HrfInfo.LDownRcvRate >NICE_SUCC_RATIO
                   )
                {
                    net_printf("RfBtrPco\n");
#if defined(STD_2016)
                    sendproxychangeDelay(e_FindBetterProxy);	//优化拓扑
#elif defined(STD_GD)
                    GD_SendProxyChangeReq(e_FindBetterProxy);
#endif
                    return;
                }
            }
        }
        

    }
    //如果没有找到更好的父节点
    if(ChangeReq_t.Result ==FALSE)//上周期代理变更失败
    {
#if defined(STD_2016)
        sendproxychangeDelay(e_KeepSyncCCO); //为了保持主从拓扑一致
#elif defined(STD_GD)
        GD_SendProxyChangeReq(e_KeepSyncCCO);
#endif
    }

}

/*计算和代理的通信成功率
上周期自己发送发现列表的次数
上周期邻居点接收到的发现列表次数
上周期邻居点发送的发现列表次数
上周期邻居点听到自己发送的发现列表的次数
最后更新上周期的通信成功率
上行成功率= 节点的代理听到自己的次数（从代理节点的发现列表中获取）/自己发送的次数(每个路由周期结束后保存)

下行成功率= 自己收到代理的次数(每个路由周期结束后保存)/代理发送的次数(从代理节点的发现列表中获取)
*/
void ProxySuccRatio(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable )
{
    U16  Ration=0;

    //记录上一个周期的通信成功率
    DiscvryTable->HplcInfo.LastDownlinkSuccRatio = DiscvryTable->HplcInfo.DownlinkSuccRatio;
    DiscvryTable->HplcInfo.LastUplinkSuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;

    if(AccessListSwitch && nnib.LinkType == e_HPLC_Link)
    {
        nnib.LProxyNodeUplinkRatio = nnib.ProxyNodeUplinkRatio;
        nnib.ProxyNodeUplinkRatio = DiscvryTable->HplcInfo.UplinkSuccRatio=0;

        nnib.LProxyNodeDownlinkRatio = nnib.ProxyNodeDownlinkRatio;
        nnib.ProxyNodeDownlinkRatio = DiscvryTable->HplcInfo.DownlinkSuccRatio=0;
    }

    if(nnib.last_My_SED !=0)
        //和某一个备用代理的上行通信成功率
    {
        Ration =( DiscvryTable->HplcInfo.PCO_REV * 100 /nnib.last_My_SED);
        if(Ration >100)Ration =100;
        DiscvryTable->HplcInfo.UplinkSuccRatio  = Ration;
        if(DiscvryTable->Relation == e_PROXY && nnib.LinkType == e_HPLC_Link)//如果是真正的代理
        {
            nnib.LProxyNodeUplinkRatio = nnib.ProxyNodeUplinkRatio;
            nnib.ProxyNodeUplinkRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;

        }
    }
    if(DiscvryTable->HplcInfo.PCO_SED != 0)//和某一个备用代理的下行通信成功率
    {

        Ration =( DiscvryTable->HplcInfo.My_REV * 100 / DiscvryTable->HplcInfo.PCO_SED);
        if(Ration >100)Ration =100;
        DiscvryTable->HplcInfo.DownlinkSuccRatio  = Ration;
        if(DiscvryTable->Relation == e_PROXY && nnib.LinkType == e_HPLC_Link)//如果是真正的代理
        {
            nnib.LProxyNodeDownlinkRatio = nnib.ProxyNodeDownlinkRatio;
            nnib.ProxyNodeDownlinkRatio = DiscvryTable->HplcInfo.DownlinkSuccRatio;
        }

    }
        return;
}

//每个路由周期结束时，计算和代理及备用代理的通信成功率，并且发起代理变更
U8 CalculateSuccRatioBulidNet()
{

	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

	U8 Listenme=TRUE;
    net_printf("TEI\tRcvM\tMSnd\tRcvP\tPSnd\t\n");

    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->SNID != GetSNID())
            continue;

        if(DiscvryTable->Relation == e_PROXY ||DiscvryTable->Relation == e_SAMELEVEL ||
           DiscvryTable->Relation == e_UPLEVEL ||DiscvryTable->Relation == e_UPPERLEVEL)	//做为子节点计算和代理节点的通信成功率
        {
            if(nnib.FristRoutePeriod ==TRUE)//第一次路由周期不做计算
            {
                net_printf("0x%04x\t%d\t%d\t%d\t%d\n"
                           ,DiscvryTable->NodeTEI
                           ,DiscvryTable->HplcInfo.PCO_REV
                           ,nnib.last_My_SED
                           ,DiscvryTable->HplcInfo.My_REV
                           ,DiscvryTable->HplcInfo.PCO_SED);

                ProxySuccRatio(DiscvryTable);

                if(DiscvryTable->Relation == e_PROXY && DiscvryTable->ResetTimes == DiscvryTable->LastRst)
                {
                    if(DiscvryTable->HplcInfo.PCO_REV ==0 && DiscvryTable->HplcInfo.PCO_SED !=0)//PCO听不到自己
                    {
						Listenme = FALSE;

                    }
                }
            }

        }
        DiscvryTable->HplcInfo.PCO_SED = 0;
        DiscvryTable->HplcInfo.PCO_REV = 0;
        DiscvryTable->LastRst = DiscvryTable->ResetTimes;

    }
    

    if(Listenme ==FALSE && nnib.LinkType == e_HPLC_Link)
	{
		sendproxychangeDelay(e_ProxyNoListenme);
		return FALSE;
	}
    return TRUE;
}

/**
 * @brief scanStopTimer     停止扫频
 * @param event          测试模式、抄控器、入网
 */
void scanStopTimer(uint8_t event)
{
    if(event == e_TEST)
    {
        HPLC.scanFlag = FALSE;
    }
    if(GetScanBandStatus())
    {
        stopScanBandTimer();
    }

    //入网不停止扫频是因为需要获取到网路的无线信道参数。信标中没有option
    //载波可以停止是因为，关联确认和汇总中携带网络载波频段。可以入网后设置频段
    // if(event != e_ONLINEEVENT)
    {
#if defined(STD_DUAL)
        if(getScanRfTimerState() == TMR_RUNNING)
        {
            StopScanRfTimer();
        }
#endif
    }

}

/**
 * @brief   记录上个路由周期的无线邻居节点上下行成功率
 * 
 */
static void Back_Rf_LastRout_RcvRatio()
{
    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        DiscvryTable->HrfInfo.LUpRcvRate = DiscvryTable->HrfInfo.UpRcvRate;
        DiscvryTable->HrfInfo.LDownRcvRate = DiscvryTable->HrfInfo.DownRcvRate;
    }
}

#endif

void MaintainNet(void)
{
#if defined(STATIC_NODE)
    static U8 RoutePeriodCnt = 0;
	//U8  macaddr[6];
	//U16 ii;
	//memset(macaddr,0xff,6);
	net_printf("lastPeriodSendtCount=%d,C_My_SED=%d\n",nnib.last_My_SED,nnib.SendDiscoverListCount);
	/*net_printf("Seq\tSNID\tMacAddr\t\tTEI\tType\tDepth\tRltn\tPhase\tLULSR\tLDLSR\tULSR\tDLSR\tMYRCV\tPCOSED\tThisRCV\tPCORCV\tRLT\n");
	for(ii=0;ii<MAX_NEIGHBOR_NUMBER;ii++)
	{
	  if(0==memcmp(DiscvryTable->MacAddr,macaddr,6))continue;
	  net_printf("%d\t%04x\t%02x%02x%02x%02x%02x%02x\t%04x\t%02x\t%02x\t%02x\t%02x\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%04x\n",
			  ii,
			  DiscvryTable->SNID,
			  DiscvryTable->MacAddr[0],
			  DiscvryTable->MacAddr[1],
			  DiscvryTable->MacAddr[2],
			  DiscvryTable->MacAddr[3],
			  DiscvryTable->MacAddr[4],
			  DiscvryTable->MacAddr[5],
			  DiscvryTable->NodeTEI,
			  DiscvryTable->NodeType, 	
			  DiscvryTable->NodeDepth,	
			  DiscvryTable->Relation,    
			  DiscvryTable->Phase, 
			  DiscvryTable->LastUplinkSuccRatio,
			  DiscvryTable->LastDownlinkSuccRatio,
	
			  DiscvryTable->UplinkSuccRatio,
			  DiscvryTable->DownlinkSuccRatio,
			  DiscvryTable->My_REV,
			  DiscvryTable->PCO_SED,
			  DiscvryTable->ThisPeriod_REV,
			  DiscvryTable->PCO_REV,
			  DiscvryTable->RemainLifeTime);	  
	}*/


    if(nnib.FristRoutePeriod ==TRUE)//每周期结束检查是否可优化拓扑
    {
        //每个路由周期结束，无线入网的节点保留上个路由周期的上下行成功率。
        if(nnib.LinkType == e_RF_Link)
        {
            nnib.LProxyNodeDownlinkRatio = nnib.ProxyNodeDownlinkRatio;
            nnib.LProxyNodeUplinkRatio   = nnib.ProxyNodeUplinkRatio;
        }

        //无线邻居节点接受率备份
        Back_Rf_LastRout_RcvRatio();

        if(CalculateSuccRatioBulidNet())
        {
            SelectBetterProxyNode(); //
        }

        
    }
    else if(nnib.Networkflag ==TRUE)
    {
    	
        if(RoutePeriodCnt >=1)
        {
            nnib.FristRoutePeriod = TRUE;
			RoutePeriodCnt =0;
			
        }
		else
		{
			RoutePeriodCnt++;
		}

    }
#elif defined(STATIC_MASTER)
	if(JoinCtrl_t.JoinMachine == e_JoinFinish_Most)
	{
		JoinCtrl_t.JoinMachine = e_JoinFinish_All;
		StartMmDiscovery();
        StartRfDiscovery();
		SetNetworkflag(TRUE);
		Trigger2app(TO_WORKFINISH);
	}
    selfAdaptionRoutePeriodTime();//自适应路由周期

#endif


    nnib.last_My_SED = nnib.SendDiscoverListCount;
	if(GetNetworkflag()==TRUE)
    nnib.SendDiscoverListCount = 0;

    //每个路由周期检查自己是否有父节点，如果没有的话，出网，以后再添加
    FrozenNeighborstaRecv();
}

void PeriodTask()
{

	
    if(1)
    {
#if defined(STATIC_MASTER)


        if(nnib.NextRoutePeriodStartTime > 0) //更新路由周期剩余时间
        {
            nnib.NextRoutePeriodStartTime--;
        }
        else
        {
            if(!nnib.FristRoutePeriod)
                nnib.FristRoutePeriod =TRUE; //第一个路由周期结束
            MaintainNet();
        }

        if(nnib.BandChangeState && nnib.BandRemianTime>0) //切换频率剩余时间
        {
            nnib.BandRemianTime--;
        }
        if(nnib.RfChannelChangeState && nnib.RfChgChannelRemainTime > 0)
        {
            nnib.RfChgChannelRemainTime--;
            net_printf("RfChgChannelRemainTime : %d\n", nnib.RfChgChannelRemainTime);
        }
        if(nnib.LinkModeStart && nnib.ChangeTime > 0) //工作模式切换剩余时间
        {
            nnib.ChangeTime--;
            if(nnib.ChangeTime == 0)
            {
                nnib.LinkModeStart = FALSE;
            }
        }
#else
		execp_reoot(); //static node execp protect

#endif
        //UpAsduSeq();//更新APS数据MSDU，防止风暴

        if( (nnib.NodeState == e_NODE_ON_LINE) && (nnib.Networkflag == TRUE) )//添加定时任务的条件
        {
#if defined(STATIC_NODE)
	        static U16 rfPxyOnlyGetBtrCnt = 0;
	        static U16 rfPxyaddNum = 0;

            if(nnib.RfDiscoverPeriod && (nnib.RoutePeriodTime/nnib.RfDiscoverPeriod))
            {   
                rfPxyOnlyGetBtrCnt = RF_RCVMAP / (nnib.RoutePeriodTime/nnib.RfDiscoverPeriod);
            }
            else
            {
                net_printf("RfDiscoverPeriod:%d, RoutePeriodTime:%d\n", nnib.RfDiscoverPeriod, nnib.RoutePeriodTime);
            }

            
            // if(nnib.SynRoutePeriodFlag == TRUE)//已经在信标中同步了周期时
            // {
                if(nnib.NextRoutePeriodStartTime > 0) //更新路由周期剩余时间
                {
                    nnib.NextRoutePeriodStartTime--;
                }
                else
                {
                    if(rfPxyaddNum > rfPxyOnlyGetBtrCnt && nnib.RfOnlyFindBtPco)
                    {
                        nnib.RfOnlyFindBtPco = FALSE;
                        net_printf("rfprxy %d, %d\n", rfPxyaddNum, rfPxyOnlyGetBtrCnt);
                    }
                    else
                    {
                        rfPxyaddNum++;
                    }
                    net_printf("------NextRoutePeriodStartTime is Over!----------\n");
                    nnib.NextRoutePeriodStartTime= nnib.RoutePeriodTime;
                    MaintainNet(); //更新本路由周期内计算通信成功率的参数
                }
            // }


#endif
        }
    }

}


///////////////////////*定时更新任务，维护数据链路层各种列表*//////////////////////////////////
void UpDataFunCB(work_t *work)
{
    #if defined(STATIC_MASTER)
    CCO_UpDataNeighborNetTime();//CCO更新邻居网络信息
    UpDataDeviceTEIlist();//更新设备列表
//    SendNetCoord(); //网监协调一秒发送一次
    updateChlHaveNetLeftTime();
    #elif defined(STATIC_NODE)
    STA_JudgeOfflineBytimer();//从模块判断自身是否要离线
    STA_UpDataNeighborNetTime();//从模块更新邻居网络信息
//    UpReportID();
    //黑名单管理
    #endif
    UpDataRoutingDiscoveryTableEntry();//更新路由发现表
    UpDataNeighborListTime(); //更新邻居节点信息 //组网完成后更新邻居表的生存时间,生存时间S单位 ，组网完成前也可更新
    //更新路由周期剩余时间，在路由周期结束时，冻结本周期和邻居间的收到次数，
    //计算通信成功率，决定是否发起代理变更，并检查是否有更优代理，CCO路由周期时间调整
    //周期性发送的网络维护命令 发现列表和心跳，通信成功率
    //STA管理自己的在线状态（在线，离线）
    UpDataRfNeighborListTime();

    //周期任务
    PeriodTask();


    //
    SystenRunTime++;
}

U8 checkNNIB()
{
	U8 i;
	if(nnib.NodeState !=e_NODE_ON_LINE && nnib.NodeState !=e_NODE_OFF_LINE && nnib.NodeState !=e_NODE_OUT_NET)
	{
		return FALSE; 
	}
	for(i = 0 ; i < 6 ; i++)
	{
		if(nnib.MacAddr[i] >>4 >=10 || (nnib.MacAddr[i] & 0xf)>= 10)
		{
			app_printf("Not BCD format,set fail\n");
			return FALSE;//非BCD格式
		}
	}
	return TRUE;
	
}
//创建秒定时器驱动，刷新各类信息
ostimer_t	DatalinkUpdatatimer;
static void DatalinkUpdatatimerCB( struct ostimer_s *ostimer, void *arg)
{
    work_t *work= zc_malloc_mem(sizeof(work), "DatalinkUpdatatimerCB", MEM_TIME_OUT);
    work->work = UpDataFunCB;
    work->msgtype = UPDATE_CB;
    post_datalink_task_work(work);
}

static void modify_DatalinkUpdatatimer(uint32_t first)
{

    if(DatalinkUpdatatimer.fn == NULL)
    {
        timer_init(&DatalinkUpdatatimer,
                   first,
                   first,
                   TMR_PERIODIC,//TMR_TRIGGER
                   DatalinkUpdatatimerCB,
                   NULL,
                   "DatalinkUpdatatimerCB"
                   );
    }
    if(first == 0) //0直接进入回调
    {
       if(timer_query(&DatalinkUpdatatimer) ==TMR_STOPPED)
       {
           timer_start(&DatalinkUpdatatimer);
       }
       timer_stop(&DatalinkUpdatatimer, TMR_CALLBACK);

    }
    else
    {
        timer_modify(&DatalinkUpdatatimer,
                first,
                first,
                TMR_PERIODIC ,//TMR_TRIGGER
                DatalinkUpdatatimerCB,
                NULL,
                "DatalinkUpdatatimerCB",
                TRUE);
        timer_start(&DatalinkUpdatatimer);
    }

}




/*函数名称：启动列表维护
数据链路层初始化时，调用此函数，启动列表维护
*/
void StartDatalinkGlobalUpdata()
{
    //启动定时器维护列表
    //if(timer_query(&DatalinkUpdatatimer) == TMR_STOPPED )
    {
        modify_DatalinkUpdatatimer(1000);
    }
}


/********************************router table********************************/
/*由网络命令和生命定时器更新管理*/


/**
 * @brief ifHasRoteLkTp     检查路由表中是否存在指定链路类型的路由信息
 * @param linkType          指定的链路类型
 * @return
 */
U8 ifHasRoteLkTp(U8 linkType)
{
    U16 i;
    U8 RouteNum = 0;
    for(i=0;i<MAX_WHITELIST_NUM; i++)
    {
        if(IS_BROADCAST_TEI(NwkRoutingTable[i].NextHopTEI))
            continue;

        RouteNum++;

        if(NwkRoutingTable[i].LinkType  == linkType || NwkRoutingTable[i].ActiveLk & (1 << linkType))
            return TRUE;
    }

    if(RouteNum == 0)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief getRouteTableLinkType     获取目的TEI的路由表链路类型信息
 * @param tei
 * @return
 */
U8 getRouteTableLinkType(U16 tei)
{
    if(IS_BROADCAST_TEI(tei))
    {
        return 0xff;
    }

    if(IS_BROADCAST_TEI(NwkRoutingTable[tei -1].NextHopTEI))
    {
        return 0xff;
    }

    return NwkRoutingTable[tei -1].LinkType;
}

//更新路由表
/**
 * @brief UpdateRoutingTableEntry       更新路由表信息
 * @param DestTEI                       目的TEI
 * @param NextHopTEI                    下一跳TEI
 * @param RouteError                    路由错误
 * @param RouteState                    路由状态
 * @param LinkType                      链路类型 0：HPLC 1:HRF
 * @return
 */
U8 UpdateRoutingTableEntry(U16 DestTEI, U16 NextHopTEI, U8 RouteError, U8 RouteState, U8 LinkType)
{
    if(DestTEI < 1 || DestTEI >= NWK_MAX_ROUTING_TABLE_ENTRYS)
        return FALSE;

    if(NwkRoutingTable[DestTEI - 1].NextHopTEI == 0xfff)
    {
        NwkRoutingTable[DestTEI - 1].ActiveLk = 0;
    }

    NwkRoutingTable[DestTEI - 1].NextHopTEI = NextHopTEI;
    NwkRoutingTable[DestTEI - 1].RouteState = RouteState;
    NwkRoutingTable[DestTEI - 1].RouteError = RouteError;
    NwkRoutingTable[DestTEI - 1].LinkType   = LinkType;
    NwkRoutingTable[DestTEI - 1].ActiveLk   = (1 << LinkType);

    return TRUE;
}

 //在路由表中寻找下一跳
U16 SearchNextHopAddrFromRouteTable(U16 DestAddress,U8 Aactive)
{
    if(is_rd_ctrl_tei(DestAddress))
        return DestAddress;

    if(DestAddress < 1 || DestAddress >= NWK_MAX_ROUTING_TABLE_ENTRYS)
        return INVALID;

    if(NwkRoutingTable[DestAddress-1].NextHopTEI == BROADCAST_TEI)
        return INVALID;

    if(nnib.AODVRouteUse == TRUE)
    {
        if(NwkRoutingTable[DestAddress-1].RouteState == Aactive)
            return NwkRoutingTable[DestAddress-1].NextHopTEI;
    }
    else
    {
        return NwkRoutingTable[DestAddress-1].NextHopTEI;
    }

    return INVALID;
}


void RepairNextHopAddr(U16 NexTEI, U8 linkType)
{
     if(NexTEI < 1 || NexTEI >= NWK_MAX_ROUTING_TABLE_ENTRYS)
         return;

     if(NwkRoutingTable[NexTEI-1].NextHopTEI == BROADCAST_TEI)
     {
         NwkRoutingTable[NexTEI-1].LinkType   = linkType;
         NwkRoutingTable[NexTEI-1].ActiveLk   = (1 << linkType);
         NwkRoutingTable[NexTEI-1].NextHopTEI = NexTEI;
         NwkRoutingTable[NexTEI-1].RouteError = 0;
         NwkRoutingTable[NexTEI-1].RouteState = ACTIVE_ROUTE;
     }
     else
     {
         NwkRoutingTable[NexTEI-1].ActiveLk  |= (1 << linkType);
     }
}

//路由表中下一跳失效
void RepairNextHopAddrInActive( U16 NexTEI)
{
    U16 i;
    for(i = 0; i < NWK_MAX_ROUTING_TABLE_ENTRYS; i++)
    {
        if(NwkRoutingTable[i].NextHopTEI == NexTEI && NwkRoutingTable[i].RouteState == INACTIVE_ROUTE)
        {
            NwkRoutingTable[i].RouteState = ACTIVE_ROUTE;
            break;
        }
    }
}

U8 AddErrorCounts(U16 nextHopAddr)
{
    U16 i;
    for(i=0; i<NWK_MAX_ROUTING_TABLE_ENTRYS; i++)
    {
        if(NwkRoutingTable[i].NextHopTEI == nextHopAddr)
        {
            NwkRoutingTable[i].RouteError++;
            if(NwkRoutingTable[i].RouteError >= MAX_ROUTE_FAILTIME)
            {
                memset(&NwkRoutingTable[i], 0xff, sizeof(ROUTE_TABLE_ENTRY_t));
                return TRUE;
            }
        }
    }

    return FALSE;
}

U8  IsInRouteDiscoveryTable(U16 byRouteRequestId, U16 DstTEI)
{
    U8 i;

    for (i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if( (RouteDiscoveryTable[i].RouteRequestID == byRouteRequestId)
                && (RouteDiscoveryTable[i].DstTEI == DstTEI))
        {
            return TRUE;
        }
    }

    return FALSE;
}

S8 IsLessRelayInRreqCmd(U16 byRouteRequestId, U16 DstTEI, U8 RemainRadius)
{
    U8 i;

    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if(RouteDiscoveryTable[i].DstTEI == DstTEI)
        {
            if(RouteDiscoveryTable[i].RemainRadius < RemainRadius)
            {
                return -1;
            }
            else if(RouteDiscoveryTable[i].RemainRadius > RemainRadius)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }

    return -1;
}

U8 AddRoutingDiscoveryTableEntry(ROUTING_DISCOVERY_TABLE_ENTRY_t *byTableEntry)
{
    U8   i;

    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if(  RouteDiscoveryTable[i].RouteRequestID  == byTableEntry->RouteRequestID
                && RouteDiscoveryTable[i].DstTEI == byTableEntry->DstTEI)
        {
            __memcpy(&RouteDiscoveryTable[i], byTableEntry, sizeof(ROUTING_DISCOVERY_TABLE_ENTRY_t));
            return TRUE;

        }
    }

    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if(RouteDiscoveryTable[i].SrcTEI == INVALID)
        {
            __memcpy(&RouteDiscoveryTable[i], byTableEntry, sizeof(ROUTING_DISCOVERY_TABLE_ENTRY_t));
            return TRUE;
        }

    }
    return FALSE;
}

void UpDataRoutingDiscoveryTableEntry()
{
    U8	 i;
    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {

        if(RouteDiscoveryTable[i].SrcTEI != INVALID)
        {
            if(RouteDiscoveryTable[i].ExpirationTime>0)
            {
                RouteDiscoveryTable[i].ExpirationTime--;
            }
            else
            {
               memset(&RouteDiscoveryTable[i],0xff,sizeof(ROUTING_DISCOVERY_TABLE_ENTRY_t));
            }

        }
    }
}

void *GetRouteDiscoveryTableEntry(U16 RouteRequestID)
{
    U8 i;

    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if(RouteDiscoveryTable[i].RouteRequestID  == RouteRequestID )
        {
            return &RouteDiscoveryTable[i];
        }
    }

    return NULL;

}

U16	 GetRouteIDBySrcTEI(U16 SrcTEI)
{
    U16 i;

    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if(RouteDiscoveryTable[i].SrcTEI  == SrcTEI )
        {
            return RouteDiscoveryTable[i].RouteRequestID;
        }
    }

    return INVALID;
}


U8 DleteRouteDiscoveryTableEntry(U16 RouteRequestId)
{
    U8 i;

    for(i = 0; i < MAX_DISCOERY_TABLE_ENTRYS; i++)
    {
        if(RouteDiscoveryTable[i].RouteRequestID  == RouteRequestId )
        {
            memset(&RouteDiscoveryTable[i], 0xFF, sizeof(ROUTING_DISCOVERY_TABLE_ENTRY_t));
            return TRUE;
        }
    }

    return FALSE;
}


/**************************************AODV***********************************/
U8 UpdateAodvRouteTableEntry(ROUTE_TABLE_RECD_t *pTableEntry, U8 dir)
{

    //0-保存上行路由 , 1-保存下行路由

    __memcpy(&NwkAodvRouteTable[dir], pTableEntry, sizeof(ROUTE_TABLE_RECD_t));

    return TRUE;

}

 U16 SearchNextHopFromAodvRouteTable(U16 DestAddress, U8 Status)
{
    U16 i;
    for(i = 0; i < NWK_AODV_ROUTE_TABLE_ENTRYS; i++)
    {
        if((NwkAodvRouteTable[i].DestTEI == DestAddress) && (NwkAodvRouteTable[i].RouteState == Status))
            return NwkAodvRouteTable[i].NextHopTEI;
    }
    return INVALID;
}

#if defined(STATIC_MASTER)
/*********************** CCO重新组网使用 *********************/

void RenetworkDataLinkFunc(work_t *work)
{
    nnib.FormationSeqNumber++;
    sysflag=TRUE;
    whitelistflag = TRUE;    
    ClearNNIB();
    mutex_post(&mutexSch_t);
}
void RenetworkAppFunc(work_t *work)
{
    U16 i=0;

    WHLPTST //共享存储内容保护
//    RouterInfo_t.CrnRMRound	 =  0;
//    RouterInfo_t.CrnMeterIndex =  0;
    do
    {
        memset(&WhiteMacAddrList[i].CnmAddr,0,8);
    }while(++i<MAX_WHITELIST_NUM);
    WHLPTED//释放共享存储内容保护

    
#if 0
    do
    {
        memset(&report_water_t[i],0,sizeof(WATER_REPORT_t));
    }while(++i<WATER_NUM);
#endif

    //发消息到链路层执行
    work_t *work_link = zc_malloc_mem(sizeof(work_t), "RENETD", MEM_TIME_OUT);
    work_link->work = RenetworkDataLinkFunc;

    post_datalink_task_work(work_link);

}
ostimer_t *Renetworkingtimer = NULL;
void RenetworkingtimerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t), "RENETA", MEM_TIME_OUT);
    work->work = RenetworkAppFunc;
    post_app_work(work);
}
/************************CCO 网络冲突处理*********************/
ostimer_t g_ChangeSNIDTimer ;

void ChangeSNIDFun(struct ostimer_s *ostimer, void *arg)
{
    timer_stop(&g_ChangeSNIDTimer, TMR_NULL);
    net_printf("-----------------------ChangeSNIDFun and Clean 30miniter-----------------------------\n");
    ClearNNIB();
#if defined(STD_2016)
    SetPib(rand() & 0x00FFFFFF,CCO_TEI);
#elif defined(STD_GD)
    SetSNIDFormation();
#endif
}


/***********************无线信道冲突处理**********************/
ostimer_t *g_ChangeRfChannelTimer = NULL;
RF_NEIGHBOR_NET_t RfNbrNet[MaxChannelNum];
U8 DstCoordRfChannel = 0xff;


void ChangeChannelPlcFun(work_t *work)
{
#if 0
	if(DstCoordRfChannel == 0xff)
	{
		uint8_t i,CurrChnl;

        CurrChnl = getHplcRfChannel();//WPHY_CHANNEL;//wphy_get_channel()+1;
		for(i=0; i<MaxSNIDnum; i++)
		{
			if(i == CurrChnl)
				continue;
            if(NeighborNet[i].SendChannl == CurrChnl)
				continue;
			break;
		}
		if(i == MaxSNIDnum)
		{
			return;
		}

//		DstCoordRfChannel = i;

        if(nnib.RfChannelChangeState == 0)
        {
            nnib.RfChannelChangeState = 1;
            nnib.RfChgChannelRemainTime = 30;       //15S后变更无线信道

            DefSetInfo.RfChannelInfo.option = 2;
            DefSetInfo.RfChannelInfo.channel =  i;
        }
        printf_s("ChangeChannelPlcFun DstCoordRfChannel:%d\n",DstCoordRfChannel);
//		timer_modify(g_ChangeRfChannelTimer,
//					15*1000,
//					0,
//					TMR_TRIGGER,
//					ChangeChannelPlcTimerCB,
//					NULL,
//					"ChangeChannelPlcTimerCB",
//					TRUE);
//		timer_start(g_ChangeRfChannelTimer);
	}
	else
	{
//		HPLC.option = 2;
//	    HPLC.rfchannel = DstCoordRfChannel;

//		wphy_set_option(HPLC.option);
//	    wphy_set_channel(HPLC.rfchannel);
////		printf_s("ChangeChannelPlcFun set channel:%d\n",HPLC.rfchannel);

//		timer_delete(g_ChangeRfChannelTimer);
//		g_ChangeRfChannelTimer = NULL;
	}
#else
    U8 m,i;
    for(m=0; m < e_op_end; m++)
    {
        if(scanRfChArry[m].option == getHplcOptin() && scanRfChArry[m].channel == getHplcRfChannel())
            continue;

        if(scanRfChArry[m].option == 0 || scanRfChArry[m].channel == 0)
            continue;

        for(i=0; i<MaxSNIDnum; i++)
        {
            if(NeighborNet[i].SendChannl == scanRfChArry[m].channel && NeighborNet[i].SendOption == scanRfChArry[m].option)
                break;
        }
        if(i >= MaxSNIDnum)
        {
            if(nnib.RfChannelChangeState == 0)
            {

                nnib.RfChannelChangeState = 1;
                nnib.RfChgChannelRemainTime = 300;       //300S后变更无线信道

                DefSetInfo.RfChannelInfo.option = scanRfChArry[m].option;
                DefSetInfo.RfChannelInfo.channel =  scanRfChArry[m].channel;
                net_printf("ChangeChannelPlcFun <%d,%d>\n",DefSetInfo.RfChannelInfo.option,DefSetInfo.RfChannelInfo.channel);

            }
        }

    }
#endif
}

void ChangeChannelPlcTimerCB(struct ostimer_s *ostimer, void *arg)
{
	work_t *work = zc_malloc_mem(sizeof(work_t), "CBPCB", MEM_TIME_OUT);

	work->work = ChangeChannelPlcFun;
	post_datalink_task_work(work);
}


uint8_t get_valid_channel(RF_OPTIN_CH_t *pRfparam)
{
//    uint8_t i;
//    uint8_t j;
    uint8_t m;
    uint8_t CurrChnl;

    CurrChnl = getHplcRfChannel();//WPHY_CHANNEL;//wphy_get_channel()+1;
    U8 option = wphy_get_option();


    net_printf("option:%d, CurrChnl:%d\n", option, CurrChnl);

    for(m=0; m < e_op_end; m++)
	{
#if 1
        if(scanRfChArry[m].option == option && scanRfChArry[m].channel == CurrChnl)
            continue;

        if(scanRfChArry[m].option == 0 || scanRfChArry[m].channel == 0)
            continue;

        if(scanRfChArry[m].HaveNet == FALSE)
        {
            pRfparam->option = scanRfChArry[m].option;
            pRfparam->channel = scanRfChArry[m].channel;
            return TRUE;
        }
#else
        if(scanRfChArry[m].option == option && scanRfChArry[m].channel == CurrChnl)
			continue;
        for(i=0; i<MaxChannelNum; i++)
		{
            if(scanRfChArry[m].option == RfNbrNet[i].SendOption && scanRfChArry[m].channel == RfNbrNet[i].SendChannl)
                break;
            for(j=0; j<MaxChannelNum; j++)
			{
                if(scanRfChArry[m].option == RfNbrNet[i].NbrOption[j] && scanRfChArry[m].channel == RfNbrNet[i].NbrChannl[j])
					break;
			}
            if(j < MaxChannelNum)
				break;
		}
		if(i == MaxChannelNum)
		{
            pRfparam->option = scanRfChArry[m].option;
            pRfparam->channel = scanRfChArry[m].channel;
            return TRUE;
		}
#endif
	}
    return FALSE;
}

void ChangeChannelRfFun(work_t *work)
{
	net_printf("RF Ch chgCbFun\n");

	//check nbr net channel
//	HPLC.option = 2;
//    HPLC.rfchannel = get_valid_channel();
//	app_printf("set rfchannel:%d\n",HPLC.rfchannel);

//    wphy_set_option(HPLC.option);
//    wphy_set_channel(HPLC.rfchannel);

//	timer_delete(g_ChangeRfChannelTimer);
//	g_ChangeRfChannelTimer = NULL;

    if(nnib.RfChannelChangeState == 0)
    {
        RF_OPTIN_CH_t rfparam;
        if(get_valid_channel(&rfparam))
        {
            nnib.RfChannelChangeState = 1;
            nnib.RfChgChannelRemainTime = 300;       //15S后变更无线信道

            DefSetInfo.RfChannelInfo.option = rfparam.option;
            DstCoordRfChannel = DefSetInfo.RfChannelInfo.channel = rfparam.channel;
        }
    }
}
void ChangeRfChannelTimerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t), "ChgRfCh", MEM_TIME_OUT);
	work->work = ChangeChannelRfFun;
	post_datalink_task_work(work);
}
void ChangeRfChannelTimerInit()
{
    memset(&RfNbrNet[0].SendChannl, 0xff, sizeof(RfNbrNet));
    /*timer_init(&g_ChangeRfChannelTimer,
                 30*60*1000,
                 0,
                 TMR_TRIGGER,
                 ChangeRfChannelTimerCB,
                 NULL,
                 "ChgRfCh"
                 );*/
}

#endif


/*******************CCO切相位使用*****************************/
U8 getTeiPhase(U16 tei)
{
    if(is_rd_ctrl_tei(tei))
    {
        return e_UNKNOWN_PHASE;
    }
    if(GET_TEI_VALID_BIT(tei) == BROADCAST_TEI)
    {
        return e_UNKNOWN_PHASE;
    }
#if defined(STATIC_MASTER)
        return DeviceTEIList[tei-2].LogicPhase;
#else
        return getPossiblePhase();
#endif
}

U8 getNextTeiPhase(U16 tei)
{
    if(is_rd_ctrl_tei(tei))
        return e_UNKNOWN_PHASE;

#if defined(STATIC_MASTER)
    U16 nextTEI = SearchNextHopAddrFromRouteTable(tei, ACTIVE_ROUTE);
    return getTeiPhase(nextTEI);
#else
    return getTeiPhase(tei);
#endif
}




void route_activelik_clean(U16 tei, U8 LinkType)
{
    if(tei<0 || tei >= NWK_MAX_ROUTING_TABLE_ENTRYS)
        return;

    NwkRoutingTable[tei - 1].ActiveLk &= ~(1 << LinkType);
}

void route_table_invalid(U16 tei)
{
    U16 i;
    for(i=0; i<NWK_MAX_ROUTING_TABLE_ENTRYS; i++)
    {
        if(NwkRoutingTable[i].NextHopTEI == tei)
            NwkRoutingTable[i].RouteState = INACTIVE_ROUTE;
    }
}
#if defined(STATIC_MASTER)
void ccocleanrnnib(work_t *work)
{
    ClearNNIB();
    SetPib(rand()&0x00ffffff, CCO_TEI);
}
void ccocleanrnnibreq()
{
    work_t *work = zc_malloc_mem(sizeof(work_t), "clnnib", MEM_TIME_OUT);
    work->work = ccocleanrnnib;
    post_datalink_task_work(work);
}
#endif



