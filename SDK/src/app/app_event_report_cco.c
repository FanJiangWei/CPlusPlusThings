/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */
#include "ZCsystem.h"
#include "flash.h"
#include "mem_zc.h"
#include "app.h"
#include "app_base_multitrans.h"
#include "app_read_jzq_cco.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_cco_clock_overproof_event.h"
#include "app_power_onoff.h"
#include "app_exstate_verify.h"
#include "Datalinkdebug.h"
#include "app_698p45.h"
#include "app_698client.h"
#include "app_dltpro.h"
#include "app_sysdate.h"
#include "app_meter_verify.h"
#include "app_clock_os_ex.h"
#include "netnnib.h"
#include "app_cnf.h"
#include "app_common.h"
#include "app_off_grid.h"
#include "app_current_abnormal_event_cco.h"

ostimer_t *clean_bitmap_timer = NULL;

#if defined(STATIC_MASTER)

EventReportStatus_e    g_EventReportStatus = e_3762EVENT_REPORT_ALLOWED;

U8  g_CcoReportCnt = 0;
U8	g_ReportFlag=FALSE;

U8  g_PowerStaFlag = 0;

ostimer_t *cco_event_send_timer = NULL;

extern U8  Gw3762TopUpReportData[512];

//采集器主动抄读事件上报
#ifndef CJQ_ACTIVE_READ_EVENT
#define CJQ_ACTIVE_READ_EVENT

ostimer_t *eventreportaddrlisttimer = NULL;
static void eventreportaddrlisttimerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"ExStateVerifyTimerCB",MEM_TIME_OUT);
	work->work = event_report_list_proc;
    work->msgtype = EVENT_REPORT_LIST;
	post_app_work(work);    
}

void modify_event_report_addr_list_timer()
{
	if(zc_timer_query(eventreportaddrlisttimer) == TMR_RUNNING)
	{
		return;
	}
	if(eventreportaddrlisttimer == NULL)
	{
		eventreportaddrlisttimer = timer_create(TIMER_UNITS,
									0, 
									TMR_TRIGGER, 
									eventreportaddrlisttimerCB, 
									NULL, 
									"eventreportaddrlisttimerCB"
									);
	}
	else
	{
		timer_modify(eventreportaddrlisttimer,
					TIMER_UNITS,
					0, 
					TMR_TRIGGER, 
					eventreportaddrlisttimerCB, 
					NULL, 
					"eventreportaddrlisttimerCB",
					TRUE);
	}
	timer_start(eventreportaddrlisttimer);
}


void event_report_list_proc(work_t *work)
{
	U16 ii;
	U8  addr_0[6] = {0};
	U8  start_flag = FALSE;
	for(ii = 0;ii<EVENT_REPORT_NUM;ii++)
	{
		if(memcmp(event_report_addr_list_t[ii].MacAddr,addr_0,6) != 0)
		{
			if(event_report_addr_list_t[ii].RemainTimeEvent > 0)
			{
				event_report_addr_list_t[ii].RemainTimeEvent--;
				start_flag = TRUE;
			}
			else
			{
				memset((U8*)&event_report_addr_list_t[ii],0x00,sizeof(EventReportInfo_t));
			}
		}
	}
	if(start_flag == TRUE)
		modify_event_report_addr_list_timer();
}

U8 check_event_report_by_addr(U8 *Addr,U16 seq)
{
	U8 null_flag = FALSE;
	U8 index_null = 0;
	U8 addr_0[6] = {0};
	U16 ii;
	
	modify_event_report_addr_list_timer();
	
	for(ii = 0;ii<EVENT_REPORT_NUM;ii++)
	{
		if (memcmp(event_report_addr_list_t[ii].MacAddr,addr_0,6) == 0 && null_flag == FALSE)
		{
			index_null = ii;
			null_flag = TRUE;
		}
		if ((memcmp(event_report_addr_list_t[ii].MacAddr,Addr,6) == 0)&&(event_report_addr_list_t[ii].Seq == seq))
		{
			return TRUE;
		}
	}
	if (null_flag == TRUE)
	{
		__memcpy(event_report_addr_list_t[index_null].MacAddr,Addr,6);
		event_report_addr_list_t[index_null].Seq = seq;
		event_report_addr_list_t[index_null].RemainTimeEvent = EVENT_REPORT_LIFE_TIME;
	}
	return FALSE;
}
#endif

static void event_reply_aps(U8 *pDstAddr, U8 *pSrcAddr, U8 funCode , U16 seq)
{    
    EVENT_REPORT_REQ_t *pEventReportRequest_t = (EVENT_REPORT_REQ_t*)zc_malloc_mem(sizeof(EVENT_REPORT_REQ_t),"Event reply to sta",MEM_TIME_OUT);

    pEventReportRequest_t->Direction = e_DOWNLINK;
    pEventReportRequest_t->PrmFlag   = 0;
    pEventReportRequest_t->FunCode  = funCode;
	pEventReportRequest_t->PacketSeq  = seq ;
    __memcpy(pEventReportRequest_t->MeterAddr,pDstAddr,6);
    ChangeMacAddrOrder(pEventReportRequest_t->MeterAddr);

    __memcpy(pEventReportRequest_t->DstMacAddr,pDstAddr,6);
    __memcpy(pEventReportRequest_t->SrcMacAddr,pSrcAddr,6);

    pEventReportRequest_t->EvenDataLen = 0;
    
    ApsEventReportRequest(pEventReportRequest_t);

    zc_free_mem(pEventReportRequest_t);

    return;
}

U8 renew_power_falg(BitMapPowerInfo_t *powerinfo,U8 state,U8 reportflag,U8 time)
{
    if(powerinfo->RemainTime > 0)
    {
        return 0;
    }
    else
    {
        powerinfo->PowerFlag = state;
        powerinfo->PowerReportFlag = reportflag;
        powerinfo->RemainTime = time;
        return 1;
    }
}

static U8 porcpower_flag(BitMapPowerInfo_t *powerinfo)
{
    if(powerinfo->PowerReportFlag == 1)
    {
        powerinfo->PowerReportFlag = 0;
        return 1;
    }
    else
    {
        return 0;
    }
}
U8 sub_power_remain_time(BitMapPowerInfo_t *powerinfo,U8 *Addr)
{
    if(powerinfo->RemainTime > 0)
    {
        powerinfo->RemainTime--;
        return 1;
    }
    else
    {
        powerinfo->PowerFlag = 0 ;
        powerinfo->PowerReportFlag = 0 ;
    }
    
    return 0;
}


/*************************************************************************
 * 函数名称 :	BitMapTransToArray(U8 *SrcBitMap , U8  SrcBitMapLen ,U16 StartIndex , U8 *Array  )
 * 函数说明 :	BitMapTransToArray重新转换
 * 参数说明 :	SrcBitMap - 原位图；SrcBitMapLen-原位图长度；StartIndex-原位图起始位置
 *					      Array	- 目标数组
 *					      
 * 返回值		:	6 & 0 为重发次数，针对停电情况，交叉发送
 *************************************************************************/

static U8  bit_map_trans_to_array(U8 *SrcBitMap , U16  SrcBitMapLen ,U16 StartIndex , BitMapPowerInfo_t *Array  )
{
    U16 reportcnt = 0 ;
    U16 ii;
    U32 bitmaplen = SrcBitMapLen * 8;
    U16 max_len = (MAX_WHITELIST_NUM + 2) / 8 + (((MAX_WHITELIST_NUM + 2) % 8) == 0 ? 0 : 1);
    BitMapPowerInfo_t *DstArray = ( BitMapPowerInfo_t *)Array ;

    if (bitmaplen > max_len)
    {
        return 0;
    }
    reportcnt = 0;
    for(ii=0; ii<bitmaplen; ii++)
    {
        if(SrcBitMap[ii/8] & (0x01 << ii%8))
        {
            reportcnt += renew_power_falg(&DstArray[ii+StartIndex-2],1,1,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
        }
    }
    if(reportcnt > 0)
    {
        return 6;
    }
    else
    {
        return 0;
    }

}

/*************************************************************************
 * 函数名称 :	Cjq2MapTransPro(U8 *SrcBitMap , U16  SrcBitMapLen )
 * 函数说明 :	Cjq2MapTransPro重新转换
 * 参数说明 :	SrcBitMap - 原位图；SrcBitMapLen-原位图长度；
 *					     	- 滤重位图
 *					     - 合并至目的位图
 * 返回值		:	5 & 0 为重发次数，针对停电情况，交叉发送
 *************************************************************************/

U8  cjq2_map_trans_pro(U8 *SrcBitMap , U16  SrcBitMapLen,U16 num,U8 flag )
{
    U16 ii ,jj;
    U16 reportcnt=0 ;
    U16 FirstNullSeq = 0xFFFF;
    U16 FindMeterSeq = 0xFFFF;
    U8 buf[6]={0,0,0,0,0,0};
    BitMapPowerInfo_t *DstArray = NULL;
    
    if(SrcBitMapLen!=0&&SrcBitMapLen/7 != num)
    {
        return 0;
    }
    if(flag==e_ADDR_EVENT_POWER_OFF)
    {
        DstArray = ( BitMapPowerInfo_t *)&power_off_addr_list;
        //reportcnt += RenewPowerFlag(&PowerOffAddrList[jj],SrcBitMap[ii*7+6],0x01,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
    }
    else
    {
        DstArray = ( BitMapPowerInfo_t *)&power_on_addr_list;
        //reportcnt += RenewPowerFlag(&PowerOffAddrList[jj],SrcBitMap[ii*7+6],0x01,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
    }
    
    for(ii=0; ii<(SrcBitMapLen)/7; ii++)
    {
        FirstNullSeq = 0xFFFF;
        FindMeterSeq = 0xFFFF;
        for(jj=0; jj<POWERNUM; jj++)
        {
            //找到重复地址，更新停复电状态
            if(FindMeterSeq == 0xFFFF && memcmp(&SrcBitMap[ii*7],power_addr_list[jj].MacAddr,6)==0)
            {
                reportcnt += renew_power_falg(&DstArray[jj],SrcBitMap[ii*7+6],0x01,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
                
                //直接跳出，无需找空
                FindMeterSeq = jj;
                break;
            }
            //找到空地址
            if(FirstNullSeq == 0xFFFF && memcmp(power_addr_list[jj].MacAddr,buf,6)==0)
            {
                FirstNullSeq = jj;
            }
            if(FindMeterSeq != 0xFFFF && FirstNullSeq != 0xFFFF)
            {
                break;
            }
        }
        //添加新的地址
        if(FindMeterSeq == 0xFFFF && FirstNullSeq != 0xFFFF)
        {
            __memcpy(power_addr_list[FirstNullSeq].MacAddr, &SrcBitMap[ii*7], 6);
            reportcnt += renew_power_falg(&DstArray[FirstNullSeq],SrcBitMap[ii*7+6],0x01,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
        }
    }
    if(reportcnt > 0)
    {
        return 6;
    }
    else
    {
        return 0;
    }
}


static U8 event_cco_power_cal(U8 FunCode, U8 *pEvent,U16  pEventLen)
{
    if(0 == pEventLen)
    {
        app_printf("pEventLen is null\n");
        return 0;
    }
	U16   rcvLen = 0;
	//U16   ii =0 ;
	U16    len,cnt = 0;
	U8    powereventtype;
	U16   startTEI;
	powereventtype = pEvent[rcvLen];
    
	// BitMap Trans
	if(
#ifdef STA_POWER_ON
	    powereventtype == e_BITMAP_EVENT_POWER_ON || 
#endif
	   powereventtype == e_BITMAP_EVENT_POWER_OFF)
	{                                
		//PowerStaFlag = powereventtype;
		startTEI = (pEvent[rcvLen+1])| pEvent[rcvLen+2]<<8;
		rcvLen += 3;
        
        //获取系统当前时间
        SysDate_t SysDateTemp;
        GetBinTime(&SysDateTemp);
        
		app_printf("[%04d-%02d-%02d	%02d:%02d:%02d]",
        SysDateTemp.Year+2000, SysDateTemp.Mon,SysDateTemp.Day,SysDateTemp.Hour,SysDateTemp.Min,SysDateTemp.Sec);
        
		app_printf("stTEI=%d,Fg=%d:",startTEI,g_PowerStaFlag);
		if(powereventtype ==e_BITMAP_EVENT_POWER_OFF)
        {
            app_printf("OFF\n");
        }
        else
        {
            app_printf("ON\n") ;  
        }      
		app_printf("Cd%d\n",FunCode);
        
		if(startTEI != 0 && pEvent - 3 != 0)
		{
	        len = pEventLen - 3;
            if(powereventtype == e_BITMAP_EVENT_POWER_ON)
            {
                cnt =  bit_map_trans_to_array(&pEvent[rcvLen] ,len, startTEI,
				power_on_bitmap_list );
            }
            else 
            {
				cnt =  bit_map_trans_to_array(&pEvent[rcvLen] ,len, startTEI,
				power_off_bitmap_list );
            }
			g_CcoReportCnt = MAX(cnt,g_CcoReportCnt);                      
		}
	}
    //CJQ2 BitMap Trans
    else if(
#ifdef STA_POWER_ON
	    powereventtype == e_ADDR_EVENT_POWER_ON || 
#endif
	    powereventtype == e_ADDR_EVENT_POWER_OFF)
    {
		U16  renum ;
		renum = (pEvent[rcvLen+1])| pEvent[rcvLen+2]<<8;
		rcvLen += 3;
		len = pEventLen - 3;
		U8	meteraddr[6]={0};
        
		//采集器停复电汇聚
		if(e_CJQ2_ACT_CCO == FunCode)
		{
			app_printf("///len=%d,renum=%d,type=%d",len,renum,powereventtype);
			cnt = cjq2_map_trans_pro( &pEvent[rcvLen] ,len, renum,powereventtype);
			app_printf("CjqProcnt=%d",cnt);
        }
        else
        {
        	//STA复电换为TEI存储
			if(powereventtype == e_ADDR_EVENT_POWER_ON)
			{
				__memcpy(meteraddr,&pEvent[rcvLen]  ,6);
				ChangeMacAddrOrder(meteraddr);
				*pEvent = 1;
				startTEI = APP_GETTEI_BYMAC(meteraddr);
                if(startTEI < (MAX_WHITELIST_NUM+2))
                {
    				cnt =  bit_map_trans_to_array(pEvent ,1, startTEI,
    				power_on_bitmap_list );
                }
			}
		}
	    g_CcoReportCnt =  MAX(cnt,g_CcoReportCnt);
		
	}
	else
	{
		return 0 ;
	}
    if(g_CcoReportCnt)
    {
	    app_printf("rcnt = %d\n",g_CcoReportCnt);
    }
	return g_CcoReportCnt ;
}



extern U8 check_whitelist_by_addr(U8 *addr);


static void event_cco_report_Ind(EVENT_REPORT_IND_t *pEventReportInd)
{
    //U8 null_addr[6]={0,0,0,0,0,0};
	//U8	default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    AppProtoType_e proto=APP_T_PROTOTYPE;
    //U16 ii;

    
    
    if(g_EventReportStatus == e_3762EVENT_REPORT_BUFF_FULL )
    {
        event_reply_aps(pEventReportInd->SrcMacAddr, pEventReportInd->DstMacAddr, e_CCO_ACK_BUF_FULL_STA,pEventReportInd->PacketSeq);
    }
    else if(g_EventReportStatus == e_3762EVENT_REPORT_FORBIDEN)
    {
        event_reply_aps(pEventReportInd->SrcMacAddr, pEventReportInd->DstMacAddr, e_CCO_ISSUE_FORBID_STA_REPORT,pEventReportInd->PacketSeq);
        //app_printf("FORBIDEN\n");
    }
    else if(g_EventReportStatus == e_3762EVENT_REPORT_ALLOWED)
    {
        if(check_whitelist_by_addr(pEventReportInd->MeterAddr) == FALSE)
        {
            if(PROVINCE_ANHUI != app_ext_info.province_code) //todo: != PROVINCE_ANHUI 安徽停电上报不能使用档案过滤
            {
            	return;
        	}
        }
        app_printf("pl: %d,Fe= %d\n",FlashInfo_t.scJZQProtocol,pEventReportInd->FunCode);
    
        if(FlashInfo_t.scJZQProtocol == e_GD2016_MSG)
        {
            #if defined(STD_GD)
            EventReport(pEventReportInd->EventData,pEventReportInd->EventDataLen);
            #endif
        }
        else if(FlashInfo_t.scJZQProtocol == e_GW13762_MSG)
        {
            dump_buf(pEventReportInd->EventData,pEventReportInd->EventDataLen);
            if(e_METER_ACT_CCO == pEventReportInd->FunCode)  // 表事件上报 
            {
                U8 UpCode;              
                U8 addr[6] = {0};
				U8 EventReportClockoverLen = 0;

				event_reply_aps(pEventReportInd->SrcMacAddr, pEventReportInd->DstMacAddr, e_CCO_CNF_STA,pEventReportInd->PacketSeq);
                if(Check645Frame(pEventReportInd->EventData,pEventReportInd->EventDataLen,&EventReportClockoverLen,addr,&UpCode)==TRUE)            
                {                 
                    if(UpCode == 0xD1)          // 过滤0xD1的645 控制码      
                    {     
                        app_printf("UpCode 0xD1!\n");      
                        return;     
                    }             
                }
                else if(Check698Frame(pEventReportInd->EventData,pEventReportInd->EventDataLen,NULL,addr,NULL)==TRUE)
                {    
                    
                }
                
				if(serial_cache_list_ide_num() > SERIAL_IDE_NUM)
				{
				    app_printf("Cd=%x,%x,Len=%d\n", UpCode, (*(pEventReportInd->EventData+EventReportClockoverLen+9)), EventReportClockoverLen);
	                if(UpCode == 0x81 && ((*(pEventReportInd->EventData+EventReportClockoverLen+9)) == 0x09))
	                {
	                    //超差事件上报滤重上报
                        clock_overproof_event_report(pEventReportInd);
	                }
	                else
	                {
	                    //采集器事件滤重上报
	                    // if(check_event_report_by_addr(addr,pEventReportInd->PacketSeq) == FALSE)
	                    {
                            app_gw3762_up_afn06_f5_report_slave_event(TRUE,
	                                                APP_GW3762_M_SLAVETYPE ,
	                                                proto,
	                                                pEventReportInd->MeterAddr,
	                                                pEventReportInd->EventDataLen,
	                                                pEventReportInd->EventData);
	                    }
	                }
					
				}
				else
				{
					app_printf("serial_num1 %d\n",serial_cache_list_ide_num());
				}
            }
            else if(e_STA_ACT_CCO == pEventReportInd->FunCode||e_CJQ2_ACT_CCO == pEventReportInd->FunCode)   // 停复电上报
            {
                if(PROVINCE_JIANGSU == app_ext_info.province_code)
                {
                    U8 addr[6]={0};
                    if(Check698Frame(pEventReportInd->EventData,pEventReportInd->EventDataLen,NULL,addr,NULL)==TRUE)
                    {
                        app_printf("CcoProcEventReportInd seq= %d\n",pEventReportInd->PacketSeq);
                        dump_buf(addr,6);
                        if(check_event_report_by_addr(addr,pEventReportInd->PacketSeq) == FALSE)
                        {
                            app_gw3762_up_afn06_f5_report_slave_event(TRUE,
                                                        APP_GW3762_HPLC_MODULE,//APP_GW3762_M_SLAVETYPE ,
                                                        proto,
                                                        pEventReportInd->MeterAddr,
                                                        pEventReportInd->EventDataLen,
                                                        pEventReportInd->EventData);
                            event_reply_aps(pEventReportInd->SrcMacAddr, pEventReportInd->DstMacAddr, e_CCO_CNF_STA,pEventReportInd->PacketSeq);
                            return;
                        }
                    }
                }

                dump_buf(pEventReportInd->SrcMacAddr,6);
                //提升混测兼容性
                //if(0 != memcmp(default_addr ,pEventReportInd->SrcMacAddr ,6 ))//&&(e_STA_EVENT_POWER_ON == pEventReportInd->EvenData.pPayload[0]))
                if(pEventReportInd->SendType == e_UNICAST_FREAM)
                {
                    event_reply_aps(pEventReportInd->SrcMacAddr, pEventReportInd->DstMacAddr, e_CCO_CNF_STA,pEventReportInd->PacketSeq);
                    //goto *free;  //无需复电取消注释即可
                }
                if(PROVINCE_HEBEI == app_ext_info.province_code)
                {
                    U8 event_type = *pEventReportInd->EventData;
                    app_printf("Event Reprot: event_type = %d\n", event_type);
                    U16 i = 0;
                    if(event_type == e_3_PHASE_METER_SEQ)
                    {
                        U8 *meter_addr = pEventReportInd->EventData + 1;
                        U8 meter_seq_state = *(pEventReportInd->EventData + 7);
                        net_printf("meter_seq_state = %d, meter_addr = ", meter_seq_state);
                        dump_buf(meter_addr, MAC_ADDR_LEN);

                        ChangeMacAddrOrder(meter_addr);
                        for(i = 0; i < MAX_WHITELIST_NUM; i++)
                        {
                            if(0 == memcmp(DeviceTEIList[i].MacAddr, meter_addr, 6))
                            {
                                DeviceTEIList[i].UP_ERR = meter_seq_state + 1;
                                DeviceTEIList[i].DeviceType = e_3PMETER;
                                //DeviceTEIList[i].UP_ERR = TRUE;
                                DeviceTEIList[i].LNerr = meter_seq_state;
                                switch(DeviceTEIList[i].Phase)
                                {
                                case e_A_PHASE:
                                    if(meter_seq_state)//ACB
                                    {
                                        DeviceTEIList[i].F31_D5 = 1;
                                        DeviceTEIList[i].F31_D6 = 0;
                                        DeviceTEIList[i].F31_D7 = 0;
                                    }
                                    else//ABC
                                    {

                                        DeviceTEIList[i].F31_D5 = 0;
                                        DeviceTEIList[i].F31_D6 = 0;
                                        DeviceTEIList[i].F31_D7 = 0;
                                    }

                                    break;
                                case e_B_PHASE:
                                    if(meter_seq_state)//BAC
                                    {
                                        DeviceTEIList[i].F31_D5 = 0;
                                        DeviceTEIList[i].F31_D6 = 1;
                                        DeviceTEIList[i].F31_D7 = 0;
                                    }
                                    else//BCA
                                    {
                                        DeviceTEIList[i].F31_D5 = 1;
                                        DeviceTEIList[i].F31_D6 = 1;
                                        DeviceTEIList[i].F31_D7 = 0;
                                    }
                                    break;
                                case e_C_PHASE:
                                    if(meter_seq_state)//CBA
                                    {
                                        DeviceTEIList[i].F31_D5 = 1;
                                        DeviceTEIList[i].F31_D6 = 0;
                                        DeviceTEIList[i].F31_D7 = 1;
                                    }
                                    else//CAB
                                    {
                                        DeviceTEIList[i].F31_D5 = 0;
                                        DeviceTEIList[i].F31_D6 = 0;
                                        DeviceTEIList[i].F31_D7 = 1;
                                    }
                                    break;
                                default:
                                    DeviceTEIList[i].F31_D5 = 0;
                                    DeviceTEIList[i].F31_D6 = 0;
                                    DeviceTEIList[i].F31_D7 = 0;
                                    break;
                                }

                                break;
                            }
                        }

                        if(i >= MAX_WHITELIST_NUM)
                        {
                            app_printf("Update Device UP_ERR Error!  dev is not in WhiteList\n");
                        }
                        else
                        {
                            app_printf("Find Dev ： ");
                            dump_buf(DeviceTEIList[i].MacAddr, MAC_ADDR_LEN);
                            app_printf("update Devicde UP_ERR  To %d,F31_D5=%d,F31_D6=%d,F31_D7=%d\n", DeviceTEIList[i].UP_ERR, DeviceTEIList[i].F31_D5, DeviceTEIList[i].F31_D6, DeviceTEIList[i].F31_D7);
                        }
                        //更新设备列表，相序状态；
                    }
                    else
                    {
                        event_cco_power_cal(pEventReportInd->FunCode, pEventReportInd->EventData, pEventReportInd->EventDataLen);

                        //report and clean Timer refresh
                        if(g_CcoReportCnt != 0)
                        {
                            g_ReportFlag = TRUE;
                            g_CcoReportCnt = 0;
                            if(TMR_STOPPED == zc_timer_query(cco_event_send_timer))
                            {
                                modify_cco_event_send_timer(15 * TIMER_UNITS);
                            }

                            if(TMR_STOPPED == zc_timer_query(clean_bitmap_timer))
                            {
                                timer_start(clean_bitmap_timer);
                            }
                            app_printf("CCOpower report------/\n");
                        }
                    }
                }
                else
                {

                    event_cco_power_cal(pEventReportInd->FunCode, pEventReportInd->EventData, pEventReportInd->EventDataLen);
 
                    //report and clean Timer refresh
                    if(g_CcoReportCnt != 0)
                    {
                        g_ReportFlag=TRUE;
                        g_CcoReportCnt = 0;
                        if(TMR_STOPPED == zc_timer_query(cco_event_send_timer))
					    {
                            modify_cco_event_send_timer(15*TIMER_UNITS);
					    }
                    
                        if(TMR_STOPPED == zc_timer_query(clean_bitmap_timer))
					    {
						    timer_start(clean_bitmap_timer);
					    }
                        app_printf("CCOpower report------/\n");
                    }
                }
#if defined(SHAAN_XI)
                //零火线异常电流上报
                if(zero_live_line_event_cal(pEventReportInd->FunCode, pEventReportInd->EventData, pEventReportInd->EventDataLen) != 0)
				{
					g_LiveLineZeroLineReportFlag=TRUE;
					g_LiveLineZeroLineReportCnt = 0;
					if(TMR_STOPPED == zc_timer_query(liveline_zeroline_cco_event_send_timer))
					{
						modify_zero_live_line_cco_power_event_send_timer(15*TIMER_UNITS);
					}
                    else
                    {
                        app_printf("liveline_zeroline_cco_event_send_timer is not STOPPED\n");
                    }
                    
                    if(TMR_STOPPED == zc_timer_query(LiveLineZeroLine_clean_bitmap_timer))
					{
					    
						timer_start(LiveLineZeroLine_clean_bitmap_timer);
					}
					app_printf("LiveLineZeroLine report------/\n");
				}
#endif                
            }
        }
    }
    else
    {
        app_printf("EventReportStatus = %d!",g_EventReportStatus);
    }
    
    return;
}



static void event_cco_power_send(work_t *work)
{
    U16   byLen = 0,ii;
    U8    bitmcountzz = 0;//255
    U8    addrcountzz = 0;
    U8 ff_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8 null_addr[6] = {0};
    U8 meteraddr[6];
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    BitMapPowerInfo_t *DstArray = NULL;
    U8  devicetype;

	if(serial_cache_list_ide_num() <= SERIAL_IDE_NUM)
	{
		modify_cco_event_send_timer(200);
		app_printf("serial_num2 %d\n",serial_cache_list_ide_num());
		return;
	}
	
    g_PowerStaFlag = 0;
    
    if(g_ReportFlag==TRUE)
    {
		byLen ++;	//首字节作为协议类型
		//STA report off
        for(ii=0; ii<MAX_WHITELIST_NUM; ii++)
        {
            //找到可上报的类型
            if(g_PowerStaFlag == 0)
            {
                //report power off
                if(power_off_bitmap_list[ii].PowerReportFlag && power_on_bitmap_list[ii].PowerReportFlag)
                {
                    if(power_off_bitmap_list[ii].RemainTime <= power_on_bitmap_list[ii].RemainTime)
                    {
                        g_PowerStaFlag = e_BITMAP_EVENT_POWER_OFF;
                        DstArray = ( BitMapPowerInfo_t *)power_off_bitmap_list;
                        Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_OFF;
                        devicetype = APP_GW3762_HPLC_MODULE;
                    }
                    else
                    {
                        g_PowerStaFlag = e_BITMAP_EVENT_POWER_ON;
                        DstArray = ( BitMapPowerInfo_t *)power_on_bitmap_list;
                        Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_ON;
                        devicetype = APP_GW3762_HPLC_MODULE;
                    }
                }
                else if(power_off_bitmap_list[ii].PowerReportFlag)
                {
                    g_PowerStaFlag = e_BITMAP_EVENT_POWER_OFF;
                    DstArray = ( BitMapPowerInfo_t *)power_off_bitmap_list;
                    Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_OFF;
                    devicetype = APP_GW3762_HPLC_MODULE;
                }
                //STA report on
                else if(power_on_bitmap_list[ii].PowerReportFlag)
                {
                    g_PowerStaFlag = e_BITMAP_EVENT_POWER_ON;
                    DstArray = ( BitMapPowerInfo_t *)power_on_bitmap_list;
                    Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_ON;
                    devicetype = APP_GW3762_HPLC_MODULE;
                }
                else
                {
                        continue;
                }
            }

            else
            {
                if(power_off_bitmap_list[ii].PowerReportFlag && power_on_bitmap_list[ii].PowerReportFlag)
                {
                    if((e_BITMAP_EVENT_POWER_OFF == g_PowerStaFlag) && (power_on_bitmap_list[ii].RemainTime <= power_off_bitmap_list[ii].RemainTime))
                    {
                        continue;
                    }
                    else if((e_BITMAP_EVENT_POWER_ON == g_PowerStaFlag) && (power_on_bitmap_list[ii].RemainTime >= power_off_bitmap_list[ii].RemainTime))
                    {
                        continue;
                    }
                    else
                    {
                        ;
                    }
                }
            }
            if(DstArray != NULL && porcpower_flag(&DstArray[ii]))
            {
                if((g_PowerStaFlag != 0)&&(bitmcountzz < MAXBMPOWERNUM))
                {
					DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp_t);
                    if(memcmp(GetDeviceListTemp_t.MacAddr, ff_addr, 6)!=0)//???? TEI
                    {
                        __memcpy(meteraddr,GetDeviceListTemp_t.MacAddr,6);
                        ChangeMacAddrOrder(meteraddr);
						//STA report need byLen ++ 
                        __memcpy(&Gw3762TopUpReportData[byLen], meteraddr, 6);                
                        byLen+= 6;
                        bitmcountzz ++;
                        if(bitmcountzz >= MAXBMPOWERNUM)
                        {
                            break;
                        }
                    }
            	}
			    off_grid_set_reset_reason(g_PowerStaFlag, ii);
            }
		}
        
        if(g_PowerStaFlag == 0)
        {
         	DstArray = NULL;
			for(ii=0; ii<POWERNUM; ii++)
            {
            	if(g_PowerStaFlag == 0)
                {
                    //report power off
                    if(power_off_addr_list[ii].PowerReportFlag && power_on_addr_list[ii].PowerReportFlag)
                    {
                        if(power_off_addr_list[ii].RemainTime <= power_on_addr_list[ii].RemainTime)
                        {
                            g_PowerStaFlag = e_ADDR_EVENT_POWER_OFF;
                            DstArray = ( BitMapPowerInfo_t *)power_off_addr_list;
                            Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_OFF;
                            devicetype = APP_GW3762_C_SLAVETYPE;
                        }
                        else
                        {
                            g_PowerStaFlag = e_ADDR_EVENT_POWER_ON;
                            DstArray = ( BitMapPowerInfo_t *)power_on_addr_list;
                            Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_ON;
                            devicetype = APP_GW3762_C_SLAVETYPE;
                        }
                    }
                    else if(power_off_addr_list[ii].PowerReportFlag)
                    {
                        g_PowerStaFlag = e_ADDR_EVENT_POWER_OFF;
                        DstArray = ( BitMapPowerInfo_t *)power_off_addr_list;
                        Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_OFF;
                        devicetype = APP_GW3762_C_SLAVETYPE;
                    }
                    else if(power_on_addr_list[ii].PowerReportFlag)
                    {
                        g_PowerStaFlag = e_ADDR_EVENT_POWER_ON;
                        DstArray = ( BitMapPowerInfo_t *)power_on_addr_list;
                        Gw3762TopUpReportData[0] = e_BITMAP_EVENT_POWER_ON;
                        devicetype = APP_GW3762_C_SLAVETYPE;
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    if(power_off_addr_list[ii].PowerReportFlag && power_on_addr_list[ii].PowerReportFlag)
                    {
                        if((e_ADDR_EVENT_POWER_OFF == g_PowerStaFlag) && (power_on_addr_list[ii].RemainTime <= power_off_addr_list[ii].RemainTime))
                        {
                            continue;
                        }
                        else if((e_ADDR_EVENT_POWER_ON == g_PowerStaFlag) && (power_on_addr_list[ii].RemainTime >= power_off_addr_list[ii].RemainTime))
                        {
                            continue;
                        }
                        else
                        {
                            ;
                        }
                    }
                }
                if(DstArray != NULL && porcpower_flag(&DstArray[ii]))
                {
                    if((g_PowerStaFlag != 0)&&(addrcountzz < MAXBMPOWERNUM))
                    {
    					if(memcmp(null_addr,power_addr_list[ii].MacAddr,6)==0)
                    	{
        					continue;
        				}
                        __memcpy(&Gw3762TopUpReportData[byLen], power_addr_list[ii].MacAddr, 6);
                        byLen+= 6;
    					Gw3762TopUpReportData[byLen++] = DstArray[ii].PowerFlag ;
                        addrcountzz ++;
                        if(addrcountzz >= MAXADDRPOWERNUM)
                        {
                            break;
                        }
                	}
                }
            }
        }
        app_printf("PowerFlag=%d,cnt=%d,addr=%d\n",g_PowerStaFlag,bitmcountzz,addrcountzz);
        
        if(g_PowerStaFlag > 0)
        {
			app_gw3762_up_afn06_f5_report_slave_event(		FALSE,
														devicetype ,
														APP_POWER_EVENT,
														0,
														byLen,
														Gw3762TopUpReportData);
            
            //app_printf("POWER BITMAP Data=\n");
            //dump_buf(Gw3762TopUpReportData,byLen);
        }
		//cal next times
		if(g_PowerStaFlag)
        {      
            modify_cco_event_send_timer(2*TIMER_UNITS);
	        g_ReportFlag = TRUE;//如果有上报，等待下一次确认是否需要上报
        }
    }
}



static void cco_clean_bitmap(work_t *work)
{
    U16 ii;
    U8 stopcleanflag = 0;
    
    //STA BitMap
    for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
    {
        //STA off
        if(sub_power_remain_time(&power_off_bitmap_list[ii] ,NULL))
        {
            stopcleanflag = 1;
        }
        if(sub_power_remain_time(&power_on_bitmap_list[ii] ,NULL))
        {
            stopcleanflag = 1;
        }
        
        //ADDR off
        if(ii < POWERNUM)
        {
            if(power_off_addr_list[ii].RemainTime == 0&&power_on_addr_list[ii].RemainTime == 0)
            {
                memset(power_addr_list[ii].MacAddr,0x00,6);
            }
            if(sub_power_remain_time(&power_off_addr_list[ii] ,NULL))
            {
                stopcleanflag = 1;
            }
            if(sub_power_remain_time(&power_on_addr_list[ii] ,NULL))
            {
                stopcleanflag = 1;
            }
        }
    }
    
    if(stopcleanflag == FALSE)
    {
        g_ReportFlag=FALSE;
        timer_stop(cco_event_send_timer,TMR_NULL);
        timer_stop(clean_bitmap_timer,TMR_NULL);
    }

    return;
}




static void event_cco_power_send_timerCB(struct ostimer_s *ostimer, void *arg)
{
	work_t *work = zc_malloc_mem(sizeof(work_t),"event_cco_power_send",MEM_TIME_OUT);
    work->work = event_cco_power_send;
    work->msgtype = CCO_EVTSEND;
	post_app_work(work);	
}

static void cco_clean_bitmap_timerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"cco_clean_bitmap",MEM_TIME_OUT);
	work->work = cco_clean_bitmap;
    work->msgtype = CCO_CLBIT;
	post_app_work(work);

}


void modify_cco_event_send_timer(uint32_t first)
{
	if(NULL != cco_event_send_timer)
	{
		timer_modify(cco_event_send_timer,
				first,
				10,
				TMR_TRIGGER ,//TMR_TRIGGER
				event_cco_power_send_timerCB,
				NULL,
                "ccopowereventSendtimerCB",
                TRUE);
	}
	else
	{
		sys_panic("cco_event_send_timer is null\n");
	}
	timer_start(cco_event_send_timer);
}



U8 cco_eventreport_timer_init(void)
{
    
    APP_UPLINK_HANDLE(cco_event_report_hook, event_cco_report_Ind);

    if(cco_event_send_timer == NULL)
    {
        cco_event_send_timer = timer_create(2*TIMER_UNITS,
	                    0,
	                    TMR_TRIGGER ,//TMR_TRIGGER
	                    event_cco_power_send_timerCB,
	                    NULL,
	                    "ccopowereventSendtimerCB"
	                   );
    }
#if defined(SHAAN_XI)
    if(PROVINCE_SHANNXI==app_ext_info.province_code)
    {
        if(liveline_zeroline_cco_event_send_timer == NULL)
        {
            liveline_zeroline_cco_event_send_timer = timer_create(200,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            zero_live_line_cco_power_event_send_timer_cb,
                            NULL,
                            "zero_live_line_cco_power_event_send_timer_cb"
                        );
        }
    }
#endif
    return TRUE;
}

void cco_clean_bitmap_timer_init(void)
{
    if(clean_bitmap_timer == NULL)
    {
        clean_bitmap_timer = timer_create(CCO_CLEAN_PERIOD_TIME*TIMER_UNITS,
                            CCO_CLEAN_PERIOD_TIME*TIMER_UNITS,
                            TMR_PERIODIC ,//TMR_TRIGGER
                            cco_clean_bitmap_timerCB,
                            NULL,
                            "cleanbitmapCB"
                           );
    }
    #ifdef SHAAN_XI
    if(LiveLineZeroLine_clean_bitmap_timer == NULL)
    {
        LiveLineZeroLine_clean_bitmap_timer = timer_create(CCO_CLEAN_PERIOD_TIME*TIMER_UNITS,
                            CCO_CLEAN_PERIOD_TIME*TIMER_UNITS,
                            TMR_PERIODIC ,//TMR_TRIGGER
                            zero_fire_line_cco_clean_bitmap_timer_cb,
                            NULL,
                            "cleanbitmapCB"
                           );
    }
    #endif
}

#endif

