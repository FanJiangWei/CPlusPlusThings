#include "ZCsystem.h"
#include "app.h"
#include "app_cco_clock_overproof_event.h"
#include "printf_zc.h"
#include "Datalinkdebug.h"
#if defined(STATIC_MASTER)

#ifndef CLOCK_OVER_EVENT_REPORT
#define CLOCK_OVER_EVENT_REPORT

ostimer_t *clock_overproof_event_delduplitimer = NULL;
ostimer_t *clock_overproof_event_ctrl_timer = NULL;

static U8 check_clock_overproof_event_list(U8 *Addr , U16 EventApsSeq,U16 *num)
{
	U16 ii = 0;
	U8  findflag = FALSE;
	U16 findnullseq = 0;
	U8  null_addr[6] = {0};
	
	for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
	{
		if(FALSE == findflag && memcmp(null_addr,clock_overproof_event_list[ii].MacAddr,6) == 0)
		{
			findflag = TRUE;
			findnullseq = ii;
		}
		if(memcmp(Addr,clock_overproof_event_list[ii].MacAddr,6) == 0)
		{
			clock_overproof_event_list[ii].num++;
			*num = clock_overproof_event_list[ii].num;
			return FALSE;
		}
	}
	app_printf("ii = %d\n",ii);
	if(ii == MAX_WHITELIST_NUM)
	{
		__memcpy(clock_overproof_event_list[findnullseq].MacAddr,Addr,6);
		clock_overproof_event_list[findnullseq].RemainTimeEvent = 12;
		clock_overproof_event_list[findnullseq].ApsSeq = EventApsSeq;
		clock_overproof_event_list[findnullseq].num = 0;
		*num = clock_overproof_event_list[findnullseq].num;
	}
	return TRUE;	
}

//缓存CCO要给集中器上报的超差事件
static U16 cache_clock_overproof_event_info(U8 LenNull_flag, AppGw3762SlaveType_e devtype, AppProtoType_e proto, U8 *meterAddr, U16 len,
                                       U8 *data, MESG_PORT_e port,U8 evclockover_Flg)
{
	U16 ii;
 	for(ii = 0; ii < MAX_CTJ_EVREPORTCLOCKOVER_NUM; ii++)
 	{
		if(0 == clock_overproof_event_info[ii].evclockover_Flg)
		{
			clock_overproof_event_info[ii].LenNull_flag = LenNull_flag;
			clock_overproof_event_info[ii].devtype = devtype;
			clock_overproof_event_info[ii].proto = proto;
			__memcpy(clock_overproof_event_info[ii].meterAddr, meterAddr, 6);
			clock_overproof_event_info[ii].len = len;
			__memcpy(clock_overproof_event_info[ii].data, data, len);
			clock_overproof_event_info[ii].port = port;
			clock_overproof_event_info[ii].evclockover_Flg = evclockover_Flg;
			app_printf("CTJEvRtClk[%d] save success!!!\n", ii);

			break;
		}
	}

	return ii;
}

static void clock_overproof_event_delduplitimerCB(struct ostimer_s * ostimer, void * arg)
{
	app_printf("This is EvtRetClkOverDeduplitimerCB!\n");
	U16 ii = 0;
	U8	flg = 0;

	for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
	{
		if(clock_overproof_event_list[ii].RemainTimeEvent == 0)
		{
			memset(&clock_overproof_event_list[ii],0x00,sizeof(ClockOverProofEventList_t));
		}
		if(clock_overproof_event_list[ii].RemainTimeEvent > 0)
		{
			clock_overproof_event_list[ii].RemainTimeEvent--;
			flg = 1;
		}
	}

	if(1 == flg)
	{
		timer_start(clock_overproof_event_delduplitimer);
	}
}

static void clock_overproof_event_proc(work_t *work)
{
	U16 ii = 0;

/*	if(TMR_RUNNING == zc_timer_query(BcdTimeCaltimer))
	{
		timer_start(CTJEvtRetClkOvertimer);
		return;
	}*/
	
	for(ii = 0; ii < MAX_CTJ_EVREPORTCLOCKOVER_NUM; ii++)
	{
		if(1 == clock_overproof_event_info[ii].evclockover_Flg)
		{
			if(SERIAL_CACHE_LIST.nr < (SERIAL_CACHE_LIST.thr-10))
			{
				app_gw3762_up_afn06_f5_report_slave_event(clock_overproof_event_info[ii].LenNull_flag,
									clock_overproof_event_info[ii].devtype ,
									clock_overproof_event_info[ii].proto,
									clock_overproof_event_info[ii].meterAddr,
									clock_overproof_event_info[ii].len,
									clock_overproof_event_info[ii].data
									//CTJ_EvReportClockList[ii].port
									);
				
				memset(&(clock_overproof_event_info[ii].LenNull_flag), 0x00, sizeof(ClockOverProofEventInfo_t)); 
				app_printf("EvRtClk[%d] report!!!\n", ii);
			}
			
			break;
		}
	}
	if(MAX_CTJ_EVREPORTCLOCKOVER_NUM == ii)
	{
		timer_stop(clock_overproof_event_ctrl_timer, TMR_NULL);
	}
	else
	{	
		timer_start(clock_overproof_event_ctrl_timer);
	}
}

static void clock_overproof_event_ctrl_timerCB(struct ostimer_s * ostimer, void * arg)
{
	work_t *work = zc_malloc_mem(sizeof(work_t),"CTJEvtRetClkOvertimerCB",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	work->work = clock_overproof_event_proc;
    work->msgtype = CJQEVTRETCLKOVER;
	post_app_work(work);  
}

//清空CCO给集中器上报超差事件列表
void clear_clock_overproof_event_ctrl_list()
{
	memset((U8*)&clock_overproof_event_info, 0x00, sizeof(clock_overproof_event_info));
	
	if(TMR_STOPPED != zc_timer_query(clock_overproof_event_ctrl_timer))
	{
		app_printf("timer_stop(clock_overproof_event_ctrl_timer)!\n");
		timer_stop(clock_overproof_event_ctrl_timer,TMR_NULL);
	}
}

//清空CCO给集中器上报超差事件列表
void clear_clock_overproof_event_list()
{
	memset((U8*)&clock_overproof_event_list,0x00,sizeof(clock_overproof_event_list));
}


void  clock_overproof_event_report(EVENT_REPORT_IND_t *pEventReportInd)
{
	/* 台体测试下CCO不上报时钟超差事件 */
	if (HPLC.scanFlag == FALSE)
	{
		return;
	}
	
    U16 ii = 0;
    AppProtoType_e proto=APP_T_PROTOTYPE;
    U16 EventReportClockover_Num = 0;
    
	{	
		app_printf("This STA addr is ->");
		dump_buf(pEventReportInd->SrcMacAddr, 6);
		
		if(check_clock_overproof_event_list(pEventReportInd->SrcMacAddr,pEventReportInd->PacketSeq,&EventReportClockover_Num) == FALSE)
		{	
			app_printf("over num = %d\n", EventReportClockover_Num);
		}
		else
		{
			//app_printf("->This STA is not reported ClockOver!\n");
			app_printf("not over num = %d\n", EventReportClockover_Num);

			ii = cache_clock_overproof_event_info(FALSE, APP_GW3762_M_SLAVETYPE, proto, pEventReportInd->MeterAddr, pEventReportInd->EventDataLen, pEventReportInd->EventData, e_APP_MSG, 1);
			if(MAX_CTJ_EVREPORTCLOCKOVER_NUM == ii)
			{
				app_printf("CTJ_EvReportClockList is full!\n");
			}
			else
			{
				if(TMR_RUNNING != zc_timer_query(clock_overproof_event_ctrl_timer))
				{
					timer_start(clock_overproof_event_ctrl_timer);
				}
				if(TMR_RUNNING != zc_timer_query(clock_overproof_event_delduplitimer))
				{
					timer_start(clock_overproof_event_delduplitimer);
				}
			}

		}

	}
    return ;
}

void clock_overproof_event_timer_init()
{
	if(NULL == clock_overproof_event_ctrl_timer)
	{
		//CCO每隔500ms给集中器上报一帧超差事件上报
		clock_overproof_event_ctrl_timer = timer_create(500, 
					0, 
					TMR_TRIGGER, 
					clock_overproof_event_ctrl_timerCB, 
					NULL, 
					"CTJEvtRetClkOvertimerCB"
					);
	}
	if(NULL == clock_overproof_event_delduplitimer)
	{
		//超差事件上报去重定时器
		clock_overproof_event_delduplitimer = timer_create(60*60*1000,
	                0,
	                TMR_TRIGGER ,//TMR_TRIGGER
	                clock_overproof_event_delduplitimerCB,
	                NULL,
	                "clock_overproof_event_delduplitimerCB"
	               );
	}
}
#endif
#endif