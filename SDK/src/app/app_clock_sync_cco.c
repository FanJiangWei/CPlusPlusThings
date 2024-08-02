#include "flash.h"
#include "app_clock_os_ex.h"
#include "aps_layer.h"
#include "app_gw3762.h"
#include "app_gw3762_ex.h"
#include "app_base_multitrans.h"
#include "app_sysdate.h"
#include "Datalinkdebug.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "app_alarm_clock.h"
#include "netnnib.h"
#include "app_dltpro.h"
#include "app.h"
#include "Datalinkdebug.h"
#include "app_area_indentification_common.h"
#include "SchTask.h"
#include "app_data_freeze_cco.h"
#include "app_deviceinfo_report.h"

#if defined(STATIC_MASTER)  

U8 ModuleTimeSyncFlag = FALSE;//陕西，湖南使用
BroadcastCycleCfg_t BroadcastCycleCfg;

//↓↓↓↓↓↓↓↓↓↓↓↓↓GW原来↓↓↓↓↓↓↓↓↓↓↓↓↓
U8 QuerySwORValueSeq = 0;
ostimer_t   *RpsQuery_clkorOvertimer = NULL;
ostimer_t   *clock_managetimer = NULL;
ostimer_t   *clockMaintaintimer = NULL;
ostimer_t   *readjzqclockTimer = NULL;

U16		    clock_maintenance_cycle = CCO_DEFAULT_REQUEST_TIME_PERIOD;
U8          ReadJZQclockSuccFlag = FALSE;
U8   		ReadJZQClockCnt = 0;
//↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑

////↓↓↓↓↓↓↓↓↓↓↓↓江苏使用↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
U8 CheckNewNodeTimerFlag = FALSE;  
ostimer_t *RTCTimeSyncSendTimer = NULL;
//↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑

//ADDR_EVENT_t EventAddrList[MAX_EVENT_REPORT_NUM];

static void cco_packet_ntb_time(U8* buf)
{
	 U8 len = 0; 
	 U32 NtbTime_now = 0;

	 NtbTime_now = get_phy_tick_time();
	    
	 buf[len++] += ((U8)(NtbTime_now&0xff));
	 buf[len++] += ((U8)((NtbTime_now>>8)&0xff));
	 buf[len++] += ((U8)((NtbTime_now>>16)&0xff));
	 buf[len++] += ((U8)((NtbTime_now>>24)&0xff));

	 //buflen = len;
	 app_printf("cco NtbTime_now = 0x%x\n", NtbTime_now);
	 dump_buf(buf, sizeof(NtbTime_now)); 
}

static void cco_clock_manage_req(work_t *work)
{
	U8	BCAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
	U8	clockMaintainInfo[11]; 
	date_time_s CCODate = {0};
	U8  SysTemp[7] = {0};
	U8  ccomac[6];
	U8  ByLen = 0;
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);
	CLOCK_MAINTAIN_REQ_t *pClockMaintainReq = NULL;
	pClockMaintainReq=zc_malloc_mem(sizeof(CLOCK_MAINTAIN_REQ_t)+ sizeof(clockMaintainInfo),"AppHLJclockMaintainReq",MEM_TIME_OUT);
	
	memset(clockMaintainInfo, 0x00, sizeof(clockMaintainInfo));
	cco_packet_ntb_time(clockMaintainInfo);
	get_date_time_s(&CCODate);
	app_printf("staDateTemp [ %d-%d-%d  %d:%d:%d ]\n",
				CCODate.year,CCODate.month,CCODate.day,CCODate.hour,CCODate.minute,CCODate.second);
				
	SysTemp[ByLen++] = CCODate.second;
	SysTemp[ByLen++] = CCODate.minute;
	SysTemp[ByLen++] = CCODate.hour;
	SysTemp[ByLen++] = CCODate.day;
	SysTemp[ByLen++] = CCODate.month;
	SysTemp[ByLen++] = (CCODate.year&0xff00) >> 8;
	SysTemp[ByLen++] = CCODate.year;
	
	__memcpy(&clockMaintainInfo[4], (U8*)&SysTemp, sizeof(date_time_s));
	dump_buf(clockMaintainInfo, sizeof(clockMaintainInfo));
	clockMaintainInfo[9]  = CCODate.year % 100;
	clockMaintainInfo[10] = CCODate.year / 100;
	bin_to_bcd(&clockMaintainInfo[4], &clockMaintainInfo[4], sizeof(date_time_s));
	
	pClockMaintainReq->Type = e_CLOCK_MAINTAIN_INFO;
	__memcpy(pClockMaintainReq->DstMacAddr, BCAddr, 6);
	__memcpy(pClockMaintainReq->SrcMacAddr, ccomac, 6);
	pClockMaintainReq->SendType = e_PROXY_BROADCAST_FREAM_NOACK;

	__memcpy(pClockMaintainReq->Asdu,clockMaintainInfo,sizeof(clockMaintainInfo));
	pClockMaintainReq->AsduLength = sizeof(clockMaintainInfo);
	app_printf("clockMaintainInfo->");
	dump_buf(clockMaintainInfo, sizeof(clockMaintainInfo));
	ApsClockMaintainRequest(pClockMaintainReq);
	zc_free_mem(pClockMaintainReq);
}


void cco_clock_maintain()
{
	work_t *work = zc_malloc_mem(sizeof(work_t),"AppReadMeterClock",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	work->work = cco_clock_manage_req;
	work->msgtype = CLOCK_MANANGE;
	post_app_work(work); 

}

void cco_query_sw_or_value_timeout(void *pTaskPrmt)
{
    
}

/*
U8 CheckEventReport(U8 *Addr)
{
	U8 NULLflag = FALSE;
	U8 NULLlistnum = 0;
	U8 NULL_Addr[6] = {0};
	U16 ii;
	for(ii = 0;ii<MAX_EVENT_REPORT_NUM;ii++)
	{
		if (memcmp(EventAddrList[ii].MacAddr,NULL_Addr,6) == 0 && NULLflag == FALSE)
		{
			NULLlistnum = ii;
			NULLflag = TRUE;
		}
		if (memcmp(EventAddrList[ii].MacAddr,Addr,6) == 0)
		{
			app_printf("Repetition in EventAddrList : ");
			dump_buf(EventAddrList[ii].MacAddr,6);
			app_printf("EventAddrList[%d] = %d\n",ii,EventAddrList[ii].RemainTimeEvent);
			return TRUE;
		}
	}
	if (NULLflag == TRUE)
	{
		app_printf("EventAddrList is NULL = %d\n",NULLlistnum);
		__memcpy(EventAddrList[NULLlistnum].MacAddr,Addr,6);
		EventAddrList[NULLlistnum].RemainTimeEvent = 150;
	}
	app_printf("CheckEventRepert is FALSE\n");
	return FALSE;
}
*/
void cco_query_clk_Sw_o_over_value(void *pTaskPrmt)
{
	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);
	QUERY_CLKSWOROVER_VALUE_REQ_t *pQuery_CLKSwOROverValueReq_t = NULL;
	multi_trans_t  *pQuerySwORValueTask = (multi_trans_t*)pTaskPrmt;
	//申请原语空间
	pQuery_CLKSwOROverValueReq_t = zc_malloc_mem(sizeof(QUERY_CLKSWOROVER_VALUE_REQ_t),"QueryClkSwAndOverValue", MEM_TIME_OUT);

	pQuery_CLKSwOROverValueReq_t->SendType = e_UNICAST_FREAM;
	__memcpy(pQuery_CLKSwOROverValueReq_t->Macaddr, pQuerySwORValueTask->CnmAddr, MACADRDR_LENGTH);
	__memcpy(pQuery_CLKSwOROverValueReq_t->DstMacAddr,pQuerySwORValueTask->CnmAddr, MACADRDR_LENGTH);
	__memcpy(pQuery_CLKSwOROverValueReq_t->SrcMacAddr,ccomac, MACADRDR_LENGTH);

	ApsQueryClkSwAndOverValueRequest(pQuery_CLKSwOROverValueReq_t);
//	Modify_Query_CLKSwOROverValueTimer(5*1000);
	zc_free_mem(pQuery_CLKSwOROverValueReq_t);
				
}

void cco_set_clk_sw(void *pTaskPrmt)
{
	U8 ccomac[6];
	U8  BCAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
	U8  FfAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);

	multi_trans_t *SetSwOrValueTaskPrmt = NULL;
	CLK_OVER_MANAGE_REQ_t* pSetClkSwRequset_t = NULL;
	pSetClkSwRequset_t = (CLK_OVER_MANAGE_REQ_t*)zc_malloc_mem( sizeof(CLK_OVER_MANAGE_REQ_t),"setCLkSw",MEM_TIME_OUT);
	SetSwOrValueTaskPrmt = (multi_trans_t *)pTaskPrmt;
	
	__memcpy(pSetClkSwRequset_t->DstMacAddr, SetSwOrValueTaskPrmt->CnmAddr, MACADRDR_LENGTH);
	__memcpy(pSetClkSwRequset_t->SrcMacAddr, ccomac,  MACADRDR_LENGTH);
	__memcpy(pSetClkSwRequset_t->Asdu,SetSwOrValueTaskPrmt->FrameUnit,SetSwOrValueTaskPrmt->FrameLen);
	pSetClkSwRequset_t->AsduLength = SetSwOrValueTaskPrmt->FrameLen;
	
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
		if(( 0 == memcmp(pSetClkSwRequset_t->DstMacAddr,BCAddr,MACADRDR_LENGTH)) || ( 0 == memcmp(pSetClkSwRequset_t->DstMacAddr,FfAddr,MACADRDR_LENGTH)))
		{
			pSetClkSwRequset_t->SendType  = e_PROXY_BROADCAST_FREAM_NOACK;	
		}
		else
		{
			pSetClkSwRequset_t->SendType  = e_UNICAST_FREAM;
		}
	}
	else
	{
		if( 0 == memcmp(pSetClkSwRequset_t->DstMacAddr,BCAddr,MACADRDR_LENGTH))
		{
			pSetClkSwRequset_t->SendType  = e_FULL_BROADCAST_FREAM_NOACK;
		}
		else
		{
			pSetClkSwRequset_t->SendType  = e_UNICAST_FREAM;
		}
	}

	ApsSetCLKSWRequest(pSetClkSwRequset_t);
	zc_free_mem(pSetClkSwRequset_t);
}

void cco_set_over_time_report_value(void *pTaskPrmt)
{
	U8  ccomac[6];
	CLOCK_MAINTAIN_REQ_t *SetOverTimeValueReq = NULL;	
	multi_trans_t *SetclkValueTaskPrmt = NULL;
	U8 BCAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
	U8 FfAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);
	SetclkValueTaskPrmt= (multi_trans_t*)pTaskPrmt;
	SetOverTimeValueReq=zc_malloc_mem(sizeof(CLOCK_MAINTAIN_REQ_t),"AppHLJclockMaintainReq",MEM_TIME_OUT);

	SetOverTimeValueReq->Type = e_OVER_TIME_VALUE_SET;
	__memcpy(SetOverTimeValueReq->DstMacAddr, SetclkValueTaskPrmt->CnmAddr, MAC_ADDR_LEN);
	__memcpy(SetOverTimeValueReq->SrcMacAddr, ccomac,    MAC_ADDR_LEN);
	app_printf("app_addr:\n");
	dump_buf(SetOverTimeValueReq->DstMacAddr, 6);
	__memcpy(SetOverTimeValueReq->Asdu,SetclkValueTaskPrmt->FrameUnit, SetclkValueTaskPrmt->FrameLen);
	SetOverTimeValueReq->AsduLength = SetclkValueTaskPrmt->FrameLen;
	
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
		if(( 0 == memcmp(SetOverTimeValueReq->DstMacAddr,BCAddr,MACADRDR_LENGTH)) || ( 0 == memcmp(SetOverTimeValueReq->DstMacAddr,FfAddr,MACADRDR_LENGTH)))
		{
			SetOverTimeValueReq->SendType  = e_PROXY_BROADCAST_FREAM_NOACK;
		}
		else
		{
			SetOverTimeValueReq->SendType  = e_UNICAST_FREAM;
		}
	}
	else
	{
		if( 0 == memcmp(SetOverTimeValueReq->DstMacAddr,BCAddr,MACADRDR_LENGTH))
		{
			SetOverTimeValueReq->SendType  = e_FULL_BROADCAST_FREAM_NOACK;
		}
		else
		{
			SetOverTimeValueReq->SendType  = e_UNICAST_FREAM;
		}
	}

	ApsClockMaintainRequest(SetOverTimeValueReq);
	zc_free_mem(SetOverTimeValueReq);
	
}

void cco_query_clk_or_value_cfm(QUERY_CLKSWOROVER_VALUE_CFM_t  * pQueryClkORValueCfm)
{
	app_printf("QueryClkORValueCfm!\n");
	MULTI_TASK_UP_t MultiTaskUp;
    MultiTaskUp.pListHeader =  &QUERYSWORVALUE_LIST;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(QUERY_CLKSWOROVER_VALUE_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pQueryClkORValueCfm->SrcMacAddr);
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pQueryClkORValueCfm);
    return;
}

void cco_set_clk_or_value_cfm(CLOCK_MAINTAIN_CFM_t  * pSetClkORValueCfm)
{
	app_printf("SetClkORValueCfm!\n");
	MULTI_TASK_UP_t MultiTaskUp;
    MultiTaskUp.pListHeader =  &SETSWORVALUE_LIST;
    
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(CLOCK_MAINTAIN_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pSetClkORValueCfm->SrcMacAddr);
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pSetClkORValueCfm);
    return;
}

void cco_cco_clk_maintain_proc(work_t *work) 
{
	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
  
	CLOCK_MAINTAIN_CFM_t * SetClkORValueCfm_t = NULL;

	//PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(CLOCK_MAINTAIN_CFM_t);
	SetClkORValueCfm_t = (CLOCK_MAINTAIN_CFM_t*)zc_malloc_mem(sizeof(CLOCK_MAINTAIN_CFM_t),  "QuerClkORValueCfm_t",MEM_TIME_OUT);

    __memcpy(SetClkORValueCfm_t->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(SetClkORValueCfm_t->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

	 if(cco_SetSwORValue_cfm_hook != NULL)
    {
        cco_SetSwORValue_cfm_hook(SetClkORValueCfm_t);
    }
    zc_free_mem(SetClkORValueCfm_t);
}

void query_clk_or_value_proc(work_t *work)
{
	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    
	app_printf("ind dstaddr : ");
    dump_buf(pMsduDataInd->DstAddress, 6);
    app_printf("ind srcaddr : ");
    dump_buf(pMsduDataInd->SrcAddress, 6);

	QUERY_CLKSWOROVER_VALUE_CFM_t * QuerClkORValueCfm_t = NULL;
	QUERY_CLKSWOROVER_VALUE_UP_HEADER_t *Query_ClkSwOrValueUpHeader = NULL;//上行帧头
	Query_ClkSwOrValueUpHeader = (QUERY_CLKSWOROVER_VALUE_UP_HEADER_t *)pApsHeader->Apdu;
	U16      PayloadLen = 0;

	PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(QUERY_CLKSWOROVER_VALUE_UP_HEADER_t);
	QuerClkORValueCfm_t = (QUERY_CLKSWOROVER_VALUE_CFM_t*)zc_malloc_mem(sizeof(QUERY_CLKSWOROVER_VALUE_CFM_t) + PayloadLen,  "QuerClkORValueCfm_t",MEM_TIME_OUT);
	__memcpy(QuerClkORValueCfm_t->Macaddr,Query_ClkSwOrValueUpHeader->Macaddr,6);
	QuerClkORValueCfm_t->AutoSwState = Query_ClkSwOrValueUpHeader->AutoSwState;
	//QuerClkORValueCfm_t->Res         = Query_ClkSwOrValueUpHeader->Res1;
	QuerClkORValueCfm_t->Time_Over_Value = Query_ClkSwOrValueUpHeader->Time_Over_Value;
	

    __memcpy(QuerClkORValueCfm_t->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(QuerClkORValueCfm_t->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

	 if(cco_QuerySwORValue_cfm_hook != NULL)
    {
        cco_QuerySwORValue_cfm_hook(QuerClkORValueCfm_t);
    }
    zc_free_mem(QuerClkORValueCfm_t);
}

//陕西湖南时钟同步部分

void cco_module_time_sync_req_aps(U8 *DateInfo)
{
    SysDate_t SysDateNow;
    //SysDateNow = *((SysDate_t *)DateInfo);
	__memcpy((U8*)&SysDateNow, DateInfo, sizeof(SysDate_t));

	if(SysDateNow.Year < 19 || SysDateNow.Mon == 0 || SysDateNow.Day == 0)
		return;
	
    //if(ModuleTimeSyncFlag ==TRUE)
    {
        ModuleTimeSyncFlag = FALSE; 
    }

    MODULE_TIMESYNC_REQ_t *pModuleTimeSyncReq_t=NULL;
   
    //申请原语空间
	pModuleTimeSyncReq_t=(MODULE_TIMESYNC_REQ_t*)zc_malloc_mem(sizeof(MODULE_TIMESYNC_REQ_t)+6, 
	                                                                                "ModuleTimeSyncReq",MEM_TIME_OUT);
    memset(pModuleTimeSyncReq_t->DstMacAddr, 0xFF, 6);
    __memcpy(pModuleTimeSyncReq_t->SrcMacAddr, FlashInfo_t.ucJZQMacAddr, 6);

    
    __memcpy(pModuleTimeSyncReq_t->Date, (U8 *)&SysDateNow, 6);
    pModuleTimeSyncReq_t->DateLen = 6;

	
    //发送消息
    ApsModuleTimeSyncRequest(pModuleTimeSyncReq_t);
    
    zc_free_mem(pModuleTimeSyncReq_t);
}

void cco_check_and_run_module_time_syns(U8 *DataUnit)
{
    if(zc_timer_query(clockMaintaintimer) == TMR_RUNNING)
    {
        app_printf("Module time syns.\n");
        cco_module_time_sync_req_aps(DataUnit);
    }
}

//江苏RTC时钟部分
#ifndef JIANGSU
#define JIANGSU
//快速同步RTC时钟
void cco_quick_sync_rtc()
{
	SysDate_t SysDateTemp = {0};
    U8  BCDstAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    GetBinTime(&SysDateTemp);
    bin_to_bcd((U8 *)&SysDateTemp,(U8 *)&SysDateTemp,sizeof(SysDate_t));
    cco_module_time_syns_request(SysDateTemp,BCDstAddr);
}
void cco_set_sync_rtc(SysDate_t SysDateTemp)
{
	//SysDate_t SysDateTemp = {0};
    U8  BCDstAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    //GetBinTime(&SysDateTemp);
    bin_to_bcd((U8 *)&SysDateTemp,(U8 *)&SysDateTemp,sizeof(SysDate_t));
    cco_module_time_syns_request(SysDateTemp,BCDstAddr);
}

void cco_set_sta_tim_sync_cb(struct ostimer_s *ostimer, void *arg)
{   
	app_printf("cco_set_sta_tim_sync_cb\n" );
	U8 addr[6] = {0};
	__memcpy(addr,(U8 *)arg,MACADRDR_LENGTH);
	dump_buf(addr, MACADRDR_LENGTH);
    //对于入网的节点先判断组网是否完成来确定是否要进行单播RTC时钟同步 
    //对于组网完成之后后续零散入网的节点进行RTC时钟单播校时
	SysDate_t SysDatenow;
    GetBinTime(&SysDatenow);
    bin_to_bcd((U8 *)&SysDatenow, (U8 *)&SysDatenow, sizeof(SysDate_t));
	app_printf("Module time syns.\n");
    cco_module_time_syns_request(SysDatenow,addr);
}

void cco_set_sta_time_sync_modify(uint32_t first,U8 *Addr)
{
    if(SetSlaveModuleTimeSyncTimer == NULL)
    {
        SetSlaveModuleTimeSyncTimer = timer_create(first*TIMER_UNITS, 
					0,
					TMR_TRIGGER ,//TMR_TRIGGER
					cco_set_sta_tim_sync_cb,
					Addr,
					"cco_set_sta_tim_sync_cb"
					);
    }
    else
    {
	    timer_modify(SetSlaveModuleTimeSyncTimer,
					first*TIMER_UNITS, 
					0,
					TMR_TRIGGER ,//TMR_TRIGGER
					cco_set_sta_tim_sync_cb,
					Addr,
					"cco_set_sta_tim_sync_cb",
					TRUE
					);
    }
	timer_start(SetSlaveModuleTimeSyncTimer);

}

void cco_module_time_syns_request(SysDate_t SysDateNow,U8 *addr)
{
    
	if(SysDateNow.Year < 19 || SysDateNow.Mon == 0 || SysDateNow.Day == 0)
		return;
	

	U8  BCDstAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    RTC_TIMESYNC_REQUEST_t *pRTCTimeSyncRequest_t=NULL;


    //申请原语空间
	pRTCTimeSyncRequest_t=zc_malloc_mem(sizeof(RTC_TIMESYNC_REQUEST_t),"pRTCTimeSyncRequest_t",MEM_TIME_OUT);
    __memcpy(pRTCTimeSyncRequest_t->DstMacAddr, addr, 6);
    __memcpy(pRTCTimeSyncRequest_t->SrcMacAddr, FlashInfo_t.ucJZQMacAddr, 6);
	
    __memcpy(pRTCTimeSyncRequest_t->RTCClock, (U8 *)&SysDateNow, 6);
	if(!memcmp(addr,BCDstAddr,6))
   	{
		pRTCTimeSyncRequest_t->SendType = e_PROXY_BROADCAST_FREAM_NOACK;
   	}
   	else
   	{
		pRTCTimeSyncRequest_t->SendType = e_UNICAST_FREAM;
   	}
    //发送消息
    ApsModuleTimesynsRequest(pRTCTimeSyncRequest_t);
    zc_free_mem(pRTCTimeSyncRequest_t);

}

static void cco_rtc_time_sync_send_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"RTCTimeSyncSendTimerCB",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	work->work = cco_rtc_time_sync_send_proc;
    work->msgtype =RTC_SYSC;
	post_app_work(work);   ;
}

void cco_modify_rtc_time_sync_send_timer(uint32_t Sec)
{
	if(NULL == RTCTimeSyncSendTimer)
    {
        
        RTCTimeSyncSendTimer = timer_create(Sec*TIMER_UNITS,
                            0,
                            TMR_TRIGGER ,
                            cco_rtc_time_sync_send_timer_cb,
                            NULL,
                            "RTCTimeSyncSendTimerCB"
                           );
    }   
    else
	{
		timer_modify(RTCTimeSyncSendTimer,
				Sec*TIMER_UNITS,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				cco_rtc_time_sync_send_timer_cb,
				NULL,
				"RTCTimeSyncSendTimerCB",
				TRUE
				);
	}
	app_printf("start RTCTimeSyncSendTimer %d Sec\n",Sec);
	timer_start(RTCTimeSyncSendTimer);
}

void cco_rtc_time_sync_send_proc(work_t* work)
{
	//extern U8 CheckCollectState();
	extern U8 newnodenetwork;
    //post 准备广播校时
    static U16  NetWorkNum = 0;
    static U8  SendTimes   = 0;
    static U16  Checkcnt    = 0;
    SysDate_t SysDateTemp = {0};
    U8  BCDstAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
    //RTCTimeSyncFlag = TRUE;
    if(CheckNewNodeTimerFlag == TRUE)
    {
    	//检查是否有新入网节点
    	if(GetDeviceNum()>NetWorkNum || newnodenetwork == TRUE)
        {
            app_printf("has new node ,RTCTimeSync\n");
            Checkcnt = 0;
            cco_modify_rtc_time_sync_send_timer(1);
        }
        else
        {
        	//没有新入网的15分钟重新检查
			cco_modify_rtc_time_sync_send_timer(CHECK_CLOCK_PERIC);
        	//CheckNewNodeTimerFlag = TRUE;
        	Checkcnt++;
        	app_printf("Checkcnt %d\n",Checkcnt);
        	if(Checkcnt == CAL_TIME_PERIC/CHECK_CLOCK_PERIC)//累计12小时无新节点入网，则启动一次全网时钟同步
            {
            	Checkcnt =0;
            	CheckNewNodeTimerFlag = FALSE;
    			cco_modify_rtc_time_sync_send_timer(1);
            }
			return;
        }


    }
    GetBinTime(&SysDateTemp);
    bin_to_bcd((U8 *)&SysDateTemp,(U8 *)&SysDateTemp,sizeof(SysDate_t));
    if(check_collect_state() == TRUE && ReadJZQclockSuccFlag == TRUE)//时钟无效时不同步时钟
	{
		newnodenetwork = FALSE;
		cco_module_time_syns_request(SysDateTemp,BCDstAddr);
	}
	else
	{
		//如果网内有不支持采集功能的节点，则15分钟后再次查询
		app_printf("one node haven't support collect!\n");
		cco_modify_rtc_time_sync_send_timer(CHECK_CLOCK_PERIC);
		return ;
	}
    
    SendTimes++;
    app_printf("RTCTimeSyncSendTimerCB %d times\n",SendTimes);
    //组网完成后和每12小时进行对时时，CCO使用代理广播向站点发送时钟报文，发3轮每轮间隔1min
	if(SendTimes>2 )
    {
        SendTimes = 0;
        //在组网完成时钟同步之后，启动定时器每30分钟检查一次是否有新节点加入
       // StaDataFreeze.STARTCFlag = TRUE;
        cco_modify_rtc_time_sync_send_timer(CHECK_CLOCK_PERIC);//METER_COMU_JUDGE_PERIC
        CheckNewNodeTimerFlag = TRUE;
    }
    else
    {
    	CheckNewNodeTimerFlag = FALSE;
        cco_modify_rtc_time_sync_send_timer(60);
    }
    NetWorkNum = GetDeviceNum();
}
#endif

static void curve_req_clock_proc(work_t *work)
{
	app_gw3762_up_afn14_f2_router_req_clock(e_UART1_MSG);
    //post 准备广播校时
    ModuleTimeSyncFlag = TRUE;
}

void cco_clk_maintain_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	app_printf("clockMaintaintimerCB\n");

	if((1U == FactoryTestFlag) || (getHplcTestMode() != NORM))
	{
		app_printf("test mode, not req jzq clock_1!\n");
		return;
	}

	if(PROVINCE_HUNAN==app_ext_info.province_code || PROVINCE_SHANNXI==app_ext_info.province_code)
	{
		work_t *work = zc_malloc_mem(sizeof(work_t),"curve_req_clock_proc",MEM_TIME_OUT);
		if(NULL == work)
		{
			return;
		}
		work->work = curve_req_clock_proc;
	    work->msgtype = CURVEREQCLOCK;
		post_app_work(work); 
		return;
	}
	else
	{
		app_gw3762_up_afn14_f2_router_req_clock(e_UART1_MSG);
		cco_modify_read_JZQ_clk_timer(5);//5S后去检查14F2请求集中器时钟是否成功
	}
	if(clockMaintaintimer != NULL)
	{
		timer_modify(clockMaintaintimer,
						clock_maintenance_cycle*60*TIMER_UNITS,
						0,
						TMR_TRIGGER ,//TMR_TRIGGER
						cco_clk_maintain_timer_cb,
						NULL,
						"clockMaintaintimerCB",
						TRUE
					);
	}
	else
	{
		clockMaintaintimer = timer_create(clock_maintenance_cycle*60*TIMER_UNITS,
							            0,
							            TMR_TRIGGER,//TMR_TRIGGER
							            cco_clk_maintain_timer_cb,
							            NULL,
							            "clockMaintaintimerCB"
							           );
	}
    
	timer_start(clockMaintaintimer);
}

void cco_modify_curve_req_clock_timer(U32 first,uint32_t interval,uint32_t opt)
{
    app_printf("clockMaintaintimer%d %d %d\n",first,interval,opt);
	if(TRUE != nnib.Networkflag)
		interval = 5*60;
	else
		interval = BroadcastCycleCfg.BroadcastCycle*60*(BroadcastCycleCfg.CycleUnit==1?1:60);

	if(0 == interval)
		interval = 4*60*60;	//默认周期4小时
	
    if(clockMaintaintimer == NULL)
    {
        clockMaintaintimer = timer_create(first*TIMER_UNITS,
	                    interval*TIMER_UNITS,
	                    opt ,//TMR_TRIGGER
	                    cco_clk_maintain_timer_cb,
	                    NULL,
	                    "cco_clk_maintain_timer_cb"
	                   );
    }
    else
    {
        timer_modify(clockMaintaintimer,
                        first*TIMER_UNITS,
                        interval*TIMER_UNITS,
                        opt ,//TMR_TRIGGER
                        cco_clk_maintain_timer_cb,
                        NULL,
                        "cco_clk_maintain_timer_cb",
                        TRUE
                       );
    }
    timer_start(clockMaintaintimer);
}
void cco_set_auto_curve_req(U8 BroadcastCycle,U8 CycleUnit)
{
    U32 retime = 0; 
    //start broadcastcycle task;
    if((BroadcastCycleCfg.BroadcastCycle != BroadcastCycle) 
		|| (BroadcastCycleCfg.CycleUnit != CycleUnit))
    {
    	BroadcastCycleCfg.BroadcastCycle = BroadcastCycle;
    	BroadcastCycleCfg.CycleUnit = CycleUnit;
		CCOCurveCfgFlag = TRUE;
    }
    app_printf("BroadcastCycleCfg.BroadcastCycle = %d,CycleUnit =%d\n",BroadcastCycleCfg.BroadcastCycle,BroadcastCycleCfg.CycleUnit);
    retime = BroadcastCycleCfg.BroadcastCycle*60*(BroadcastCycleCfg.CycleUnit==1?1:60);
    app_printf("retime = %d\n",retime);
    cco_modify_curve_req_clock_timer(1,retime,TMR_PERIODIC);
}


void cco_clk_cycle_req(void)
{
    U32  retime = 0;
	
	retime = BroadcastCycleCfg.BroadcastCycle*60*(BroadcastCycleCfg.CycleUnit==1?1:60);
	
    cco_modify_curve_req_clock_timer(1,retime,TMR_PERIODIC);
}
//江苏使用
static void cco_read_JZQ_clk_proc(work_t* work)
{
	if((1U == FactoryTestFlag) || (getHplcTestMode() != NORM))
	{
		app_printf("test mode, not req jzq clock_2!\n");
		return;
	}

	if(PROVINCE_JIANGSU==app_ext_info.province_code)  //江苏使用
	{
	    app_gw3762_up_afn14_f2_router_req_clock(e_UART1_MSG);
		if(ReadJZQclockSuccFlag == FALSE)
		{
			cco_modify_read_JZQ_clk_timer(10*60);
		}
		else
		{
			cco_modify_read_JZQ_clk_timer(CAL_TIME_PERIC);//要到时钟之后，每12小时要一次
		}

	}
	else   //国网/山东使用
	{
		ReadJZQClockCnt++;
		if(ReadJZQclockSuccFlag == FALSE && ReadJZQClockCnt < 3)
		{
			app_gw3762_up_afn14_f2_router_req_clock(e_UART1_MSG);
			cco_modify_read_JZQ_clk_timer(5);
		}
		else
		{	
			ReadJZQclockSuccFlag = FALSE;
			ReadJZQClockCnt =0;
			timer_stop(readjzqclockTimer,TMR_STOPPED);
		}
	}
}

static void cco_read_JZQ_clk_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"ReadJZQClockTimerCB",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	work->work = cco_read_JZQ_clk_proc;
    work->msgtype =READ_METER_CLOCK;
	post_app_work(work);  
}

void cco_modify_read_JZQ_clk_timer(uint32_t Sec)
{
	if(NULL == readjzqclockTimer)
	{
		
		readjzqclockTimer = timer_create(Sec*TIMER_UNITS,
							0,
							TMR_TRIGGER ,
							cco_read_JZQ_clk_timer_cb,
							NULL,
							"ReadJZQClockTimerCB"
						   );
	}	
	else
	{
		timer_modify(readjzqclockTimer,
				Sec*TIMER_UNITS,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				cco_read_JZQ_clk_timer_cb,
				NULL,
				"ReadJZQClockTimerCB",
				TRUE
				);
	}
	app_printf("ModifyReadJZQClockTimer %d Sec\n",Sec);
	timer_start(readjzqclockTimer);
}

static void  cco_clk_maintain_init()
{	
	if(clockMaintaintimer == NULL)
	{
		clockMaintaintimer = timer_create(CCO_START_REQUEST_TIME*TIMER_UNITS,  //第一次启动，10秒后获取集中器时间
							            0,
							            TMR_TRIGGER,//TMR_TRIGGER
							            cco_clk_maintain_timer_cb,
							            NULL,
							            "clockMaintaintimerCB"
							           );
	}
	else
	{
		timer_modify(clockMaintaintimer,
						clock_maintenance_cycle*60*TIMER_UNITS,
						0,
						TMR_TRIGGER ,//TMR_TRIGGER
						cco_clk_maintain_timer_cb,
						NULL,
						"clockMaintaintimerCB",
						TRUE
					);
	}
	app_printf("clock_maintenance_cycle : %d\n",clock_maintenance_cycle);
   	timer_start(clockMaintaintimer);
}

static void cco_read_JZQ_clk_init()
{
	cco_modify_read_JZQ_clk_timer(30);//上电30S后去要时钟					   
}
static void cco_rtc_sync_timer_init(void)
{
	RTCTimeSyncSendTimer = timer_create(60*TIMER_UNITS,
                            0,
                            TMR_TRIGGER ,
                            cco_rtc_time_sync_send_timer_cb,
                            NULL,
                            "RTCTimeSyncSendTimerCB"
                           );
}
void cco_time_manager_init()
{
	//江苏部分
	if(PROVINCE_JIANGSU==app_ext_info.province_code)  					//江苏使用
	{
		cco_read_JZQ_clk_init();
		alarm_clock_timer_init();//闹钟初始化
		cco_rtc_sync_timer_init();
	}
	else if(PROVINCE_HUNAN==app_ext_info.province_code || PROVINCE_SHANNXI==app_ext_info.province_code)
	{
		cco_clk_cycle_req();
	}
	else
	{
		cco_clk_maintain_init();	//时钟管理	
	}
}

void Module_Time_Sync_Ind(work_t *work)
{}

void sta_clk_maintain_proc(work_t *work) 
{}

#endif


