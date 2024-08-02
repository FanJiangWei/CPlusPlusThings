/*
 * Copyright: (c) 2009-2021, 2021 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_cco_curve_gather.c
 * Company: 北京中宸微电子
 * Purpose:	
 * History:
 * Author : WWJ
 */

#include "app_data_freeze_cco.h"
#include "aps_layer.h"
#include "app_gw3762_ex.h"
#include "app_upgrade_cco.h"
#include "datalinkdebug.h"
#include "app.h"
#include "app_phase_position_cco.h"
#include "app_base_multitrans.h"
#include "netnnib.h"
#include "SchTask.h"
#include "app_gw3762_ex.h"
#include "app_read_jzq_cco.h"
#include "flash.h"
#include "app_event_report_cco.h"
#include "app_clock_sync_cco.h"

//#include "app_zero_fire_report_cco.h"

#if defined(STATIC_MASTER)

//#ifdef CURVE_GATHER


//ostimer_t *CurveGatherCtrlTimer = NULL;      // F1F100并发控制定时器
ostimer_t *SetSlaveCurveGatherTimer = NULL;   // 设置从节点曲线采集定时器

uint8_t CCOCurveCfgFlag = FALSE; //CCO曲线配置开关
U8 CurveGatherState = e_CURVE_GATHER_IDLE;

CurveGatherProfileSub_t CcoCurveGatherProfile;
CCOCurveGatherInfo_t CcoCurveGatherInfo;

void AddSlaveCurveCfgReq(void);
void CurveProfileCfgReqAps(void *pTaskPrmt);
void ApsModuleTimeSyncRequest(MODULE_TIMESYNC_REQ_t *pModuleTimeSyncReq_t);
void ApsCurveProfileCfgRequest(CURVE_PROFILE_CFG_REQ_t *pCurveProfileCfgReq);

curve_profile_cfg_cfm_fn  cco_curve_profile_cfg_cfm_hook = NULL;
app_assoc_renew_one_fn  app_assoc_renew_one_hook = NULL;

multi_trans_header_t  CURVE_PROFILE_CFG_LIST;     ostimer_t *curve_proflie_cfg_timer = NULL;
ostimer_t *curve_profile_cfg_guard_timer = NULL;

int8_t MODULE_CURVE_CFG_LIST_NUM = 2;

CONCURRENCY_CTRL_INFO_t  CTRL_CURVE_PROFILE_CFG_Info;

extern unsigned char    BroadcastExternAddr[6];

/*************************************************************************/

/*************************************************************************/
//SHANNXI
//03F102上行帧应答
U8 UART_Get_Sta_Curve_Profile_Cfg_By_Protocol(U8 Protocol,U8 *ItemSub, U16 *ItemSubLen)
{
	U8 NoUseItemSub[9];
	U8 bylen = 0;
    if(PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
	{
		U8 NouseItemSub_HUNAN[9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

		__memcpy(NoUseItemSub, NouseItemSub_HUNAN, sizeof(NoUseItemSub));
	}
	else if(PROVINCE_SHANNXI == app_ext_info.province_code) //SHANNXI
	{
		U8 NouseItemSub_SHANNXI[9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00};

		__memcpy(NoUseItemSub, NouseItemSub_SHANNXI, sizeof(NoUseItemSub));
	}
	
    if(CcoCurveGatherProfile.UseFlag == FALSE)
    {
        __memcpy(&ItemSub[bylen], NoUseItemSub , sizeof(NoUseItemSub));
        
        *ItemSubLen = sizeof(NoUseItemSub);
        return TRUE;
    }

	if(PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
	{
		ItemSub[bylen++] = 1 ;
		ItemSub[bylen++] = CcoCurveGatherProfile.ReadmeterCycle;
		*ItemSubLen += 2;
	}
	
    if(Protocol == DLT645_2007)
    {
        ItemSub[bylen++] = Protocol ;
        *ItemSubLen += 1;
        __memcpy(&ItemSub[bylen] , CcoCurveGatherProfile.Item645Sub , CcoCurveGatherProfile.Item645SubLen);
        *ItemSubLen += CcoCurveGatherProfile.Item645SubLen;
		bylen += CcoCurveGatherProfile.Item645SubLen;
        return TRUE;
    }
    else if(Protocol == DLT698)
    {
        ItemSub[bylen++] = Protocol ;
        *ItemSubLen += 1;
        __memcpy(&ItemSub[bylen] , CcoCurveGatherProfile.Item698Sub , CcoCurveGatherProfile.Item698SubLen);
        *ItemSubLen += CcoCurveGatherProfile.Item698SubLen;
		bylen += CcoCurveGatherProfile.Item698SubLen;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//载波打包使用
//HUNAN
U8 PLC_Get_Sta_Curve_Profile_Cfg_By_Protocol(U8 Protocol,U8 *ItemSub, U16 *ItemSubLen)
{
	U8 NoUseItemSub[9];
	U8 bylen = 0;
    if(PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
	{
		U8 NouseItemSub_HUNAN[9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};

		__memcpy(NoUseItemSub, NouseItemSub_HUNAN, sizeof(NoUseItemSub));
	}
	else if(PROVINCE_SHANNXI == app_ext_info.province_code) //SHANNXI
	{
		U8 NouseItemSub_SHANNXI[9] = {0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00};

		__memcpy(NoUseItemSub, NouseItemSub_SHANNXI, sizeof(NoUseItemSub));
	}
    if(CcoCurveGatherProfile.UseFlag == FALSE)
    {
        __memcpy(&ItemSub[bylen] , NoUseItemSub , sizeof(NoUseItemSub));
        
        *ItemSubLen = sizeof(NoUseItemSub);
        return TRUE;
    }
	ItemSub[bylen++] = 1 ;
	ItemSub[bylen++] = CcoCurveGatherProfile.ReadmeterCycle;
	*ItemSubLen += 2;
    if(Protocol == DLT645_2007)
    {
        ItemSub[bylen++] = Protocol ;
        *ItemSubLen += 1;
        __memcpy(&ItemSub[bylen] , CcoCurveGatherProfile.Item645Sub , CcoCurveGatherProfile.Item645SubLen);
        *ItemSubLen += CcoCurveGatherProfile.Item645SubLen;
		bylen += CcoCurveGatherProfile.Item645SubLen;
        return TRUE;
    }
    else if(Protocol == DLT698)
    {
        ItemSub[bylen++] = Protocol ;
        *ItemSubLen += 1;
        __memcpy(&ItemSub[bylen] , CcoCurveGatherProfile.Item698Sub , CcoCurveGatherProfile.Item698SubLen);
        *ItemSubLen += CcoCurveGatherProfile.Item698SubLen;
		bylen += CcoCurveGatherProfile.Item698SubLen;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void CCOCurveCfgProc()
{
	memset((U8 *)&BroadcastCycleCfg , 0x00 , sizeof(BroadcastCycleCfg_t));
    memset((U8 *)&CcoCurveGatherProfile , 0x00 , sizeof(CurveGatherProfileSub_t));

    if(TRUE == cco_read_curve_cfg((U8 *)&CcoCurveGatherInfo))
    {
        if(CcoCurveGatherInfo.CcoCurveGatherProfile.UseFlag == TRUE)
        {
            //需要恢复CCO曲线采集任务
            __memcpy((U8 *)&BroadcastCycleCfg , (U8 *)&CcoCurveGatherInfo.BroadcastCycleCfg , sizeof(BroadcastCycleCfg_t));
			__memcpy((U8 *)&CcoCurveGatherProfile , (U8 *)&CcoCurveGatherInfo.CcoCurveGatherProfile , sizeof(CurveGatherProfileSub_t));
            app_printf("BroadcastCycle = %d, CycleUnit = %d\n",
				                                  BroadcastCycleCfg.BroadcastCycle,BroadcastCycleCfg.CycleUnit);
			if(BroadcastCycleCfg.BroadcastCycle == 0 && BroadcastCycleCfg.CycleUnit == 0)
			{
                app_printf("BroadcastCycleCfg set default value.\n");
                BroadcastCycleCfg.BroadcastCycle = 4;
                BroadcastCycleCfg.CycleUnit = HOURS;
                
                CCOCurveCfgFlag = TRUE;
			}
            
            app_printf("BroadcastCycle= %d!\n",BroadcastCycleCfg.BroadcastCycle);
        }
    }
	else
	{
		CcoCurveGatherInfo.LiveLineZeroLineEnabled=0;//1;
		app_printf("BroadcastCycleCfg set default value.\n");
        BroadcastCycleCfg.BroadcastCycle = 4;
        BroadcastCycleCfg.CycleUnit = HOURS;

        CCOCurveCfgFlag = TRUE;
	}

    if(NULL == SetSlaveCurveGatherTimer)
    {
        SetSlaveCurveGatherTimer = timer_create(1000,
                            0,
                            TMR_TRIGGER,
                            SetSlaveCurveGatherTimerCB,
                            NULL,
                            "SetSlaveCurveGatherTimerCB"
                            );
    }
}

U8 GetSTACurveGatherCfgbySeq(U16 Index)
{
    //return CTRL_CURVE_PROFILE_CFG_Info.taskPolling[Index].Result;
    if(e_HasGet == DeviceTEIList[Index].CurveNeed)
    	return e_HasGet;
	else
		return 0;
}


void CtrlSlaveCurveCfgRound(void)
{
    multi_trans_ctrl_plc_task_round(&CURVE_PROFILE_CFG_LIST,&CTRL_CURVE_PROFILE_CFG_Info);
}

void slave_curve_cfg_start(work_t *work)
{
    app_printf("slave_curve_gather_start \n");
   
    //multi_task_set_bitmap(&CTRL_CURVE_PROFILE_CFG_Info);
	get_info_from_net_set_poll(&CTRL_CURVE_PROFILE_CFG_Info);
    
    CURVE_PROFILE_CFG_LIST.TaskAddOne = AddSlaveCurveCfgReq;
    CURVE_PROFILE_CFG_LIST.TaskRoundPro = CtrlSlaveCurveCfgRound;
    
    CTRL_CURVE_PROFILE_CFG_Info.lid = UPGRADE_LID;
    CTRL_CURVE_PROFILE_CFG_Info.CurrentIndex = 0;
    CTRL_CURVE_PROFILE_CFG_Info.ReTransmitCnt = 2;
    CTRL_CURVE_PROFILE_CFG_Info.DealySecond = 20;
    CTRL_CURVE_PROFILE_CFG_Info.CycleNum = 1;

    CTRL_CURVE_PROFILE_CFG_Info.TaskNext = SetSlaveCurveGather;//upgrade_start_renew_evt;
    
    AddSlaveCurveCfgReq();//添加第一条并发任务

    //ctrl_plc_multi_task_list_all_add(&UPGRADE_START_MULTI_TASK_LIST);
    //modify_plc_upgrade_start_timer(300);
    
}
//初始化并发任务
uint8_t multi_task_renew_bitmap(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    U16  ii;
    U8   State = FALSE;
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    app_printf("ST seq\ttei\tSend\tResult\n");
    for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp_t)==TRUE 
			&& GetDeviceListTemp_t.NodeTEI != 0xFFF)
        {
            pConInfo->taskPolling[ii].Valid = 1;
            if(GetDeviceListTemp_t.CurveNeed != e_HasGet)
            {
                pConInfo->taskPolling[ii].Send = 0;
                pConInfo->taskPolling[ii].Result = 0;
            }
            State = TRUE;
            
        }
        else
        {
            pConInfo->taskPolling[ii].Valid = 0;
        }
            
    }
    if(State == TRUE);
    return State;
    
}

void slave_curve_cfg_renew(work_t *work)
{
    app_printf("slave_curve_gather_renew \n");
   
    multi_task_renew_bitmap(&CTRL_CURVE_PROFILE_CFG_Info);
    
    CURVE_PROFILE_CFG_LIST.TaskAddOne = AddSlaveCurveCfgReq;
    CURVE_PROFILE_CFG_LIST.TaskRoundPro = CtrlSlaveCurveCfgRound;
    
    CTRL_CURVE_PROFILE_CFG_Info.lid = UPGRADE_LID;
    CTRL_CURVE_PROFILE_CFG_Info.CurrentIndex = 0;
    CTRL_CURVE_PROFILE_CFG_Info.ReTransmitCnt = 2;
    CTRL_CURVE_PROFILE_CFG_Info.DealySecond = 20;
    CTRL_CURVE_PROFILE_CFG_Info.CycleNum = 1;

    CTRL_CURVE_PROFILE_CFG_Info.TaskNext = SetSlaveCurveGather;//upgrade_start_renew_evt;
    
    AddSlaveCurveCfgReq();//添加第一条并发任务

    //ctrl_plc_multi_task_list_all_add(&UPGRADE_START_MULTI_TASK_LIST);
    //modify_plc_upgrade_start_timer(300);
    
}

void SetSlaveCurveGather(work_t *work)
{
    //U16 Index = 0,TaskNum = 0;
    //static U16 RecNum = 0;
    if(CurveGatherState == e_CURVE_CFG_SET_ALL)
    {
        
       // memset((U8 *)&CURVE_PROFILE_CFG_LIST,0x00,sizeof(CTRL_CURVE_PROFILE_CFG_Info));
        multi_trans_task_self_init(&CURVE_PROFILE_CFG_LIST, 0, (SET_PARAMETER(app_ext_info.param_cfg.ConcurrencySize,10,MAX_WIN_SIZE,10)), 3, NULL, NULL, NULL, &curve_proflie_cfg_timer);
        SetSlaveCurveGatherTimerModify(2000,slave_curve_cfg_start);
       
        CurveGatherState = e_CURVE_CFG_SET_RENEW;
    }
    else if(CurveGatherState == e_CURVE_CFG_SET_RENEW)
    {
        CurveGatherState = e_CURVE_GATHER_IDLE;
        
        SetSlaveCurveGatherTimerModify(2000,slave_curve_cfg_renew);
    }
    else if(CurveGatherState == e_CURVE_CFG_SET_RESET)
    {
        CurveGatherState = e_CURVE_GATHER_IDLE;
    }
    else
    {

    }
    if(CurveGatherState != e_CURVE_GATHER_IDLE)
    {
        //SetSlaveCurveGatherTimerModify(10*1000);
    }
    else
    {
        if(CurveGatherState != e_CURVE_GATHER_IDLE && CcoCurveGatherProfile.UseFlag != FALSE)
        {
            
            
        }
    }

}


void SetSlaveCurveGatherTimerCB(struct ostimer_s *ostimer, void *arg)
{
    //post;
    work_t *worknext = zc_malloc_mem(sizeof(work_t),"PPMTL",MEM_TIME_OUT);
    worknext->work = arg;
	worknext->msgtype = SET_STA_CURVE_CFG;
    post_app_work(worknext);
}
void SetSlaveCurveGatherTimerModify(uint32_t first,void *arg)
{
	timer_modify(SetSlaveCurveGatherTimer,
					first, 
					0,
					TMR_TRIGGER ,//TMR_TRIGGER
					SetSlaveCurveGatherTimerCB,
					arg,
					"SetSlaveCurveGatherTimerCB",
					TRUE
					);
	timer_start(SetSlaveCurveGatherTimer);
}

U16   ApsCurveProfileCfgSendPacketSeq = 0;
U16   ApsCurveProfileCfgRecvPacketSeq = 0xffff;


void CurveProfileCfgCfmProc(CURVE_PROFILE_CFG_CFM_t  * pCurveProfileCfgCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;

    
    MultiTaskUp.pListHeader =  &CURVE_PROFILE_CFG_LIST;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(CURVE_PROFILE_CFG_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pCurveProfileCfgCfm->SrcMacAddr);
    
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pCurveProfileCfgCfm);
    

    return;
}

//新入网的模块，5分钟后进行广播校时，且立即同步数据冻结方案,
//且10分钟后广播零火线异常使能标志
void app_assoc_renew_one_proc()
{
    if(PROVINCE_SHANNXI == app_ext_info.province_code || PROVINCE_HUNAN == app_ext_info.province_code)
	{
        if(timer_remain(clockMaintaintimer) > 5*60*TIMER_UNITS)
            cco_modify_curve_req_clock_timer(5*60, BroadcastCycleCfg.BroadcastCycle*60*(BroadcastCycleCfg.CycleUnit==1?1:60), TMR_PERIODIC);

        if(CcoCurveGatherInfo.CcoCurveGatherProfile.UseFlag == TRUE && TMR_STOPPED == zc_timer_query(SetSlaveCurveGatherTimer)
            && CcoCurveGatherInfo.CcoCurveGatherProfile.ReadmeterCycle > 0)
        { 
            //新入网STA，配置任务
            CurveGatherState = e_CURVE_CFG_SET_RENEW;
            SetSlaveCurveGatherTimerModify(60*TIMER_UNITS,slave_curve_cfg_renew);
        }
#ifdef SHAAN_XI
        if (PROVINCE_SHANNXI == app_ext_info.province_code && TRUE == nnib.Networkflag)
        {
            app_send_live_zero_enable_wait_timer();
        }
#endif
    }
}

static int32_t CurveProfileCfgEntryListCheck(multi_trans_header_t *list, multi_trans_t *new_list)
{     
	app_printf("list->nr = %d, list->thr= %d\n", list->nr, list->thr);
    if(list->nr >= list->thr)
    {
        return -1;
    }

	list_head_t *pos,*node;
    multi_trans_t  *mbuf_n;
    
    /*Check for duplicate meter address*/
    list_for_each_safe(pos, node,&list->link)
    {
        mbuf_n = list_entry(pos, multi_trans_t, link);
        dump_buf(mbuf_n->CnmAddr, 6);
        if(memcmp(mbuf_n->CnmAddr, new_list->CnmAddr, 6) == 0)
        {
            return -2;
        }
    }
    app_printf("Entry check.\n");
    dump_buf(new_list->CnmAddr, 6);

    return 0;
}

static void CurveProfileCfgEntryListfail(int32_t err_code, void *pTaskPrmt)
{
    app_printf("Entry ModuleReq fail, err = %d\n", err_code);
    
    //RegisterInfo_t.CrnSlaveIndex++;
    if(err_code == -2)
    {
        //RegisterInfo_t.Timeout = 1000;
        //modify_slave_register_timer(RegisterInfo_t.Timeout);
    }
    else if(err_code == -1)
    {
        //modify_slave_register_timer(0);
    }
	
    return;
}

static void CurveProfileCfgTimeout(void *pTaskPrmt)
{
    
}

static void CurveProfileCfgUpProc(void *pTaskPrmt, void *pUplinkData)
{
    CURVE_PROFILE_CFG_CFM_t  *pCurveProfileCfgCfm = (CURVE_PROFILE_CFG_CFM_t*)pUplinkData;

    app_printf("CurveProfileCfgUpProc!!! pCurveProfileCfgCfm->Result = %d\n", pCurveProfileCfgCfm->Result);
    if(pCurveProfileCfgCfm->Result == e_SUCCESS)
    {
		app_printf("Con daddr : ");
		dump_buf(pCurveProfileCfgCfm->DstMacAddr, 6);
		
		app_printf("Con saddr : ");
		dump_buf(pCurveProfileCfgCfm->SrcMacAddr, 6);
  
        DeviceList_ioctl(DEV_SET_CURVE,(void *)pCurveProfileCfgCfm->SrcMacAddr,(void *)&pCurveProfileCfgCfm->Result);   
    }

    return;
}


void AddSlaveCurveCfgReq(void)
{    
    app_printf("AddSlaveCurveCfgReq\n");
    
	if(multi_trans_add_task_form_poll(&CTRL_CURVE_PROFILE_CFG_Info))
	{  
        multi_trans_t SlaveCurveCfgReq;

        app_printf("CrnSlaveIndex = %d\n", CTRL_CURVE_PROFILE_CFG_Info.CurrentIndex);
        
        //Get_DeviceList_All(CTRL_CURVE_PROFILE_CFG_Info.CrnSlaveIndex,&GetDeviceListTemp_t);
        dump_buf(CTRL_CURVE_PROFILE_CFG_Info.CrntNodeAdd,6);
    	
    	SlaveCurveCfgReq.lid = UPGRADE_LID;
    	SlaveCurveCfgReq.SrcTEI = 0;
        SlaveCurveCfgReq.DeviceTimeout = DEVTIMEOUT;
        SlaveCurveCfgReq.Framenum = 0;
        //__memcpy(SlaveCurveCfgReq.Addr, AppGw3762Afn14F1State.Addr, MAC_ADDR_LEN);	
        __memcpy(SlaveCurveCfgReq.CnmAddr, CTRL_CURVE_PROFILE_CFG_Info.CrntNodeAdd, MAC_ADDR_LEN);

    	SlaveCurveCfgReq.State = UNEXECUTED;
        
        SlaveCurveCfgReq.SendType = SLAVEREGISTER_LOCK;
        SlaveCurveCfgReq.StaIndex = CTRL_CURVE_PROFILE_CFG_Info.CurrentIndex;     
        SlaveCurveCfgReq.DatagramSeq = 0;
    	SlaveCurveCfgReq.DealySecond = CURVE_PROFILE_CFG_LIST.nr == 0?0:1;
        SlaveCurveCfgReq.ReTransmitCnt = CTRL_CURVE_PROFILE_CFG_Info.ReTransmitCnt;
    	
        SlaveCurveCfgReq.DltNum = 1;
    	SlaveCurveCfgReq.ProtoType = 0;
    	SlaveCurveCfgReq.FrameLen  = 0;
        SlaveCurveCfgReq.EntryListCheck = CurveProfileCfgEntryListCheck;
    	SlaveCurveCfgReq.EntryListfail = CurveProfileCfgEntryListfail;
    	SlaveCurveCfgReq.TaskPro = CurveProfileCfgReqAps; 
        SlaveCurveCfgReq.TaskUpPro = CurveProfileCfgUpProc;
        SlaveCurveCfgReq.TimeoutPro = CurveProfileCfgTimeout;

        SlaveCurveCfgReq.pMultiTransHeader = &CURVE_PROFILE_CFG_LIST;
    	SlaveCurveCfgReq.CRT_timer = upgrade_start_timer;
        multi_trans_put_list(SlaveCurveCfgReq, NULL);
    }
    else
    {
        app_printf("CURVE_PROFILE_CFG_LIST.TaskRoundPro\n");
        CURVE_PROFILE_CFG_LIST.TaskRoundPro();
    }
}


void CurveProfileCfgReqAps(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    U8 Addr[6] = {0};
    U8 Protocol = 0;
   
    U8  *CurveCfg = zc_malloc_mem(200, "CurveCfg", MEM_TIME_OUT);
    U16 CurveCfgLen = 0;

    __memcpy(Addr ,pReadTask->CnmAddr,6 );
    
    ChangeMacAddrOrder(Addr);
    
    Protocol = get_protocol_by_addr(Addr);
    
    
    PLC_Get_Sta_Curve_Profile_Cfg_By_Protocol(Protocol , CurveCfg , &CurveCfgLen);
    app_printf("Protocol = %d, CurveCfgLen = %d!\n",Protocol, CurveCfgLen);
	if(CurveCfgLen > 3)
	{
	    CURVE_PROFILE_CFG_REQ_t *pCurveProfileCfgReq = NULL;
	    pCurveProfileCfgReq=(CURVE_PROFILE_CFG_REQ_t*)zc_malloc_mem(sizeof(CURVE_PROFILE_CFG_REQ_t)+CurveCfgLen, 
		                                                                                "CurveProfileCfgReqAps",MEM_TIME_OUT);
	    pCurveProfileCfgReq->PacketSeq = pReadTask->DatagramSeq ;//CurveConInfo.CycleInfo.DatagramSeq;
	    __memcpy(pCurveProfileCfgReq->DstMacAddr, pReadTask->CnmAddr, 6);
	    __memcpy(Addr, FlashInfo_t.ucJZQMacAddr, 6);
	    ChangeMacAddrOrder(Addr);
	    __memcpy(pCurveProfileCfgReq->SrcMacAddr, Addr, 6);

	    __memcpy(pCurveProfileCfgReq->Data, CurveCfg, CurveCfgLen);
	    
	    pCurveProfileCfgReq->DataLen = CurveCfgLen;
	    
	    //发送消息
	    ApsCurveProfileCfgRequest(pCurveProfileCfgReq);
	    
	    zc_free_mem(pCurveProfileCfgReq);
	}
    zc_free_mem(CurveCfg);
}

void ApsCurveProfileCfgRequest(CURVE_PROFILE_CFG_REQ_t *pCurveProfileCfgReq)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;

    CURVE_PROFILE_CFG_DOWN_HEADER_t  *pCurveCfgDownHeader = NULL;
    U16   msdu_length = 0;
    
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(CURVE_PROFILE_CFG_DOWN_HEADER_t) + pCurveProfileCfgReq->DataLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pCurveCfgDownHeader = (CURVE_PROFILE_CFG_DOWN_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_CURVE_PROFILE_CFG;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

	pCurveCfgDownHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pCurveCfgDownHeader->HeaderLen   = sizeof(CURVE_PROFILE_CFG_DOWN_HEADER_t);
    pCurveCfgDownHeader->Reserve1    = 0;
    pCurveCfgDownHeader->PacketSeq   = pCurveProfileCfgReq->PacketSeq;
    __memcpy(pCurveCfgDownHeader->SrcMacAddr, pCurveProfileCfgReq->SrcMacAddr, 6);
    __memcpy(pCurveCfgDownHeader->DstMacAddr, pCurveProfileCfgReq->DstMacAddr, 6);
	pCurveCfgDownHeader->Reserve2    = 0;
    pCurveCfgDownHeader->DataLength  = pCurveProfileCfgReq->DataLen;
	__memcpy(pCurveCfgDownHeader->Data, pCurveProfileCfgReq->Data, pCurveProfileCfgReq->DataLen);

	ApsPostPacketReq(post_work, msdu_length, 0, pCurveProfileCfgReq->DstMacAddr, 
                     pCurveProfileCfgReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    
    return;
}

static int32_t jzq_read_afn_f1f100_entrylist_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    list_head_t *pos,*node;
	multi_trans_t  *mbuf_n;
    
    if(list->nr >= list->thr)
    {
		return -1;
	}
                             
    /*Check for duplicate meter address*/
    list_for_each_safe(pos, node,&list->link)
    {
        mbuf_n = list_entry(pos, multi_trans_t, link);
        
        if(memcmp(mbuf_n->Addr, new_list->Addr, 6) == 0)
        {
            return -2;
		}
    }

    return 0;
}

static void jzq_read_app_gw3762_up_anff1f100_upf00_deny(int32_t err_code, void *pTaskPrmt)//,int8_t localseq)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    if(err_code == -1)
    {
        app_gw3762_up_afn00_f2_deny_by_seq(APP_GW3762_OUT_MAXCONCURRNUM_ERRCODE,pReadTask->Framenum, pReadTask->MsgPort);//109超过最大并发数
    }
	else if(err_code == -2)
	{
        app_gw3762_up_afn00_f2_deny_by_seq(APP_GW3762_METER_READING_ERRCODE,pReadTask->Framenum, pReadTask->MsgPort);//111此表正在抄读
	}

	return;
}

static void jzq_read_afn_f1f100_read_meter_timeout(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;

    app_gw3762_up_afnf1_f100_up_frame(pReadTask->Addr, pReadTask->ProtoType, 0 , 0, pReadTask->MsgPort,pReadTask->Framenum);
    return;
}

void CnttActCncrntF1F100ReadMeterReq(U8 localFrameSeq, U8 readMeterNum, U8 *pMeterAddr, U8 *pCnmAddr,U16 Index, U8 protoType, 
                                                      U16 frameLen, U8 *pFrameUnit,U8 BroadcastFlag, MESG_PORT_e port)
{
    multi_trans_t f1f1Readmeter;
	
	f1f1Readmeter.lid = READ_METER_LID;
	f1f1Readmeter.SrcTEI = 0;
    f1f1Readmeter.DeviceTimeout = DEVTIMEOUT;
    f1f1Readmeter.Framenum = localFrameSeq;
    __memcpy(f1f1Readmeter.Addr, pMeterAddr, MAC_ADDR_LEN);	
    __memcpy(f1f1Readmeter.CnmAddr, pCnmAddr, MAC_ADDR_LEN);

    f1f1Readmeter.MsgPort = port;

	f1f1Readmeter.State = UNEXECUTED;
	f1f1Readmeter.SendType = F1F100_READMETER;
    f1f1Readmeter.StaIndex = Index;
    f1f1Readmeter.DatagramSeq = ++ApsSendPacketSeq;
	f1f1Readmeter.DealySecond = F1F1_TRANS_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    f1f1Readmeter.ReTransmitCnt = (BroadcastFlag == AllBroadcast?1:RETRANMITMAXNUM);//未入网的节点支持并发抄表一次，但是仅支持BROADCAST_CONRM_NUM条
    f1f1Readmeter.BroadcastFlag = BroadcastFlag;

    f1f1Readmeter.DltNum = readMeterNum;
	f1f1Readmeter.ProtoType = protoType;
	f1f1Readmeter.FrameLen  = frameLen;
	
    f1f1Readmeter.EntryListCheck = jzq_read_afn_f1f100_entrylist_check;
	f1f1Readmeter.EntryListfail = jzq_read_app_gw3762_up_anff1f100_upf00_deny;
	f1f1Readmeter.TaskPro = jzq_read_app_read_meter_req; 
    f1f1Readmeter.TaskUpPro = jzq_read_cntt_act_cncrnt_proc;
	f1f1Readmeter.TimeoutPro = jzq_read_afn_f1f100_read_meter_timeout;
	
    f1f1Readmeter.pMultiTransHeader = &F1F1_TRANS_LIST;
	f1f1Readmeter.CRT_timer = f1f1_trans_timer;  
    multi_trans_put_list(f1f1Readmeter, pFrameUnit);

    return;
}





void cco_save_curve_cfg(void)
{   
    uint16_t cs16 = 0;
    __memcpy(&CcoCurveGatherInfo.BroadcastCycleCfg ,&BroadcastCycleCfg,sizeof(BroadcastCycleCfg_t));
    __memcpy(&CcoCurveGatherInfo.CcoCurveGatherProfile ,&CcoCurveGatherProfile,sizeof(CurveGatherProfileSub_t));
    cs16 = ResetCRC16();
    int len = 0;
    len = sizeof(CCOCurveGatherInfo_t) - 2;
	cs16 = CalCRC16(cs16,(U8 *)&CcoCurveGatherInfo, 0 , len);
    CcoCurveGatherInfo.Crc16 = cs16;
    //dump_buf((U8 *)&CcoCurveGatherInfo, sizeof(CCOCurveGatherInfo_t));
    
    app_printf("sizeof(CCOCurveGatherInfo_t) = %d,cs16 = %d\n", sizeof(CCOCurveGatherInfo_t),cs16);
	if(FLASH_OK == zc_flash_write(FLASH, CURVE_CFG, (U32)&CcoCurveGatherInfo, CURVE_CFG_LEN))
    {
        app_printf("CcoCurveGatherInfo zc_flash_write OK!\n");
    }
    else
    {
        app_printf("CcoCurveGatherInfo zc_flash_write ERROR!\n");
    }
}
U8 cco_read_curve_cfg(U8 *CCOCurveGatherCfg)
{
    uint16_t cs16 = 0;
    //CCOCurveGatherInfo_t CcoCurveGatherInfo;
	if(FLASH_OK == zc_flash_read(FLASH, CURVE_CFG, (U32 *)&CcoCurveGatherInfo, CURVE_CFG_LEN))
    {
        app_printf("CCOCurveInfo_t zc_flash_read OK!\n");
    }
    else
    {
        app_printf("CCOCurveInfo_t zc_flash_read ERROR!\n");
    }
    cs16 = ResetCRC16();
    U16 len = 0;
    len = sizeof(CCOCurveGatherInfo_t) - 2;
	cs16 = CalCRC16(cs16,(U8 *)&CcoCurveGatherInfo, 0 , len);
    if(cs16 == CcoCurveGatherInfo.Crc16)
    {
        __memcpy(CCOCurveGatherCfg , (U8 *)&CcoCurveGatherInfo , sizeof(CCOCurveGatherInfo_t));
        return TRUE;
    }
    else
    {
        
        app_printf("CCOCurveGatherInfo_t Crc16 is err!,cs16=%d\n",cs16);
        return FALSE;
    }
}
void cco_curve_init(void)
{
    CCOCurveCfgProc();
}

U8 SaveCurveCfg(U8 *macaddr , U8* info)
{
    U16 usIdx=0;
    for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
    {

        if(0==memcmp(macaddr, DeviceTEIList[usIdx].MacAddr, 6))
        {

            if(DeviceTEIList[usIdx].NodeTEI == BROADCAST_TEI)
            {
                return  e_NeedGet ;
            }
            if(info[0] == e_SUCCESS)
            {
                if(DeviceTEIList[usIdx].CurveNeed == 0xFF || DeviceTEIList[usIdx].CurveNeed ==e_NeedGet)
                {
                    DeviceTEIList[usIdx].CurveNeed =e_HasGet;
                    net_printf("CurveNeedTEI=%d e_HasGet\n",DeviceTEIList[usIdx].NodeTEI);
                }
            }
            return DeviceTEIList[usIdx].CurveNeed;
        }
    }
    return  e_NeedGet ;
}

void clearStaCurveStatus()
{
	U16 ii;
    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
    	if(DeviceTEIList[ii].NodeTEI != 0xFFF)
    		DeviceTEIList[ii].CurveNeed = e_NeedGet; //获取曲线冻结数据  
    }
}

//#endif
#endif

#if defined(STATIC_MASTER)

void CurveProfileCfgProc(work_t *work)
{
//#ifdef CURVE_GATHER

    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;    
    
         
    app_printf("ind daddr : ");
    dump_buf(pMsduDataInd->DstAddress, 6);
            
    app_printf("ind saddr : ");
    dump_buf(pMsduDataInd->SrcAddress, 6);
	
    if(memcmp(pMsduDataInd->SrcAddress,BroadcastExternAddr,6 ) ==0 )
    {
        return;
    }

    CURVE_PROFILE_CFG_UP_HEADER_t  *pCurveProfileCfgHeader = (CURVE_PROFILE_CFG_UP_HEADER_t*)pApsHeader->Apdu;
    CURVE_PROFILE_CFG_CFM_t  *pCurveProfileCfgCfm = NULL;
    U16      PayloadLen = 0;

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(CURVE_PROFILE_CFG_UP_HEADER_t);

    pCurveProfileCfgCfm = (CURVE_PROFILE_CFG_CFM_t*)zc_malloc_mem(sizeof(CURVE_PROFILE_CFG_CFM_t) + PayloadLen,
                                                                 "CurveProfileCfgCfm",MEM_TIME_OUT);
    app_printf("PayloadLen=%d, pCurveProfileCfgHeader->Result = %d\n",PayloadLen, pCurveProfileCfgHeader->Result);
    pCurveProfileCfgCfm->Result     = pCurveProfileCfgHeader->Result;    
    pCurveProfileCfgCfm->PacketSeq     = pCurveProfileCfgHeader->PacketSeq;
    
    __memcpy(pCurveProfileCfgCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pCurveProfileCfgCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
            
    
    if(cco_curve_profile_cfg_cfm_hook != NULL)
    {
        cco_curve_profile_cfg_cfm_hook(pCurveProfileCfgCfm);	//CurveProfileCfgCfmProc
    }
    
    zc_free_mem(pCurveProfileCfgCfm);
//#endif
}

#else
void CurveProfileCfgProc(work_t *work)
{
    ;
}

#endif


