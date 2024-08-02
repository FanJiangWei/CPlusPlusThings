#include "flash.h"
#include "app_clock_os_ex.h"
#include "aps_layer.h"
#include "app_gw3762.h"
#include "app_base_multitrans.h"
#include "app_sysdate.h"
#include "Datalinkdebug.h"
#include "app_698client.h"
#include "app_clock_sta_timeover_report.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "app_time_calibrate.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_cnf.h"
#include "app_698client.h"
#include "app_dltpro.h"
#include "SchTask.h"



#if defined(STATIC_NODE) 
ostimer_t *TimeOverControlTimer = NULL;
ostimer_t *TimeOverCtrlTimer = NULL;
CLK_OVER_PROOF	DifferenceTime = {0};		//本地时钟和电能表时钟的差值

U8	TimeOverReportState = e_TIMEOVER_NULL;
EventReportSave_t event_report_clockover_save_t;

//超差事件上报定时器回调
static void sta_event_report_clk_overtimer_cb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"ExStateVerifyTimerCB",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	work->work = clock_os_event_report_proc;
    work->msgtype = CLOCK_OS_EVENT;
	post_app_work(work);  
}

void sta_creat_event_report_clk_overtimer(U32 MStimes)
{
    if(EventReportClockOvertimer)
    {
    	timer_modify(EventReportClockOvertimer,
        				MStimes/*/10*/,
                    	0,
                        TMR_TRIGGER ,//TMR_TRIGGER
                        sta_event_report_clk_overtimer_cb,
                        NULL,
                        "EventReportClockOvertimerCB",
						TRUE
                    );
    }
    else
    {
		EventReportClockOvertimer = timer_create(30*TIMER_UNITS,
									0,
									TMR_TRIGGER ,//TMR_TRIGGER
									sta_event_report_clk_overtimer_cb,
									NULL,
									"EventReportClockOvertimerCB"
								   );
	}
	timer_start(EventReportClockOvertimer);

}

//返回值e_TIMEROVER_NO_REPORT：差值不满足上报，1：差值满足上报，但是模块当前不在网，2：已上报
static U8 sta_time_over_event_report(S32 TimeDeviation)
{
	U8 	eventlogo[9]={0x04, 0x00, 0x15, 0x99};
	U8  CtrlCode = 0x81;
	U8	len;
	U8  StaTEI = 0;
	U8 sendbuf[50];
	U32 MaxTimeDiff = 9999*60*60+60*60+60; //上报最大差值为9999小时60分钟60秒

	len = 0;
	memset(sendbuf, 0x00, sizeof(sendbuf));

	if(abs(TimeDeviation) <= (DevicePib_t.Over_Value * 60)) //小于预定值直接返回不上报
	{
		return e_TIMEROVER_NO_REPORT;
	}	
	
	if(TimeDeviation > 0)
	{
		eventlogo[8]= e_Jzq_faster;
	}
	else
	{
		eventlogo[8]= e_meter_faster;	
	}	
	
	if(abs(TimeDeviation) >= MaxTimeDiff)
	{
		DifferenceTime.LHour = 99;
		DifferenceTime.HHour = 99;
		DifferenceTime.Min = 60;
		DifferenceTime.Sec = 60;
	}
	else
	{
		DifferenceTime.LHour = abs(TimeDeviation)/60/60/100;
		DifferenceTime.HHour = abs(TimeDeviation)/60/60%100;
		DifferenceTime.Min = (abs(TimeDeviation)%(60*60))/60;
		DifferenceTime.Sec = abs(TimeDeviation)%60;
	}

		
	app_printf("DifferenceTime is %x:%x:%x%x \n",DifferenceTime.LHour,DifferenceTime.HHour,DifferenceTime.Min,DifferenceTime.Sec);
	bin_to_bcd((U8*)&DifferenceTime, (U8*)&DifferenceTime, sizeof(DifferenceTime));

	__memcpy(&eventlogo[4], (U8 *)&DifferenceTime, sizeof(DifferenceTime));
    Add0x33ForData(&eventlogo[0], sizeof(DifferenceTime)+1+4);
	Packet645Frame(sendbuf,&len,DevicePib_t.DevMacAddr,CtrlCode,eventlogo,sizeof(eventlogo));
	event_report_clockover_save_t.len = len;
	__memcpy(event_report_clockover_save_t.buf,sendbuf,event_report_clockover_save_t.len);

	app_printf("event_report_clockover_save_t.buf %d->", event_report_clockover_save_t.len);
	dump_buf(event_report_clockover_save_t.buf, event_report_clockover_save_t.len);

	//net_nnib_ioctl(NET_GET_TEI,&StaTEI);
	if(CheckSTAOnlineSate() == FALSE)
	{
		app_printf("CheckSTAOnlineSate!\n");
        return e_TIMEROVER_REPORT_OFFLINE;
	}
	if((GetTEI() < 10) && (GetTEI() > 1))
	{
		sta_creat_event_report_clk_overtimer(100*StaTEI);
	}
	else if(GetTEI() >= 10)
	{
		sta_creat_event_report_clk_overtimer(rand()%(180*1000));
	}



	return e_TIMEROVER_REPORTING;
}

//STA上报超差值
static void sta_timeout_value_report(void)
{
	if(DevicePib_t.Over_Value != 0)
	{
		if(PIB_TIMEDEVIATION != DevicePib_t.TimeDeviation)
		{
			U8	TimeOverReportStatus;
			SysDate_t dateNow;

			TimeOverReportStatus = 0;
			GetBinTime(&dateNow);
			
			TimeOverReportStatus = sta_time_over_event_report(DevicePib_t.TimeDeviation);
			if(e_TIMEROVER_NO_REPORT == TimeOverReportStatus)		//差值不满足超差事件上报
			{
				U32 NowTime = os_rtc_time()/(24*60*60);
				NowTime *= (24*60*60);
				DevicePib_t.TimeReportDay = NowTime;
				staflag = TRUE;
			}
			else if(e_TIMEROVER_REPORT_OFFLINE == TimeOverReportStatus)	//差值满足超差事件上报但模块不在网
			{
				staflag = TRUE;
			}
		}
	}
	else
	{
		app_printf("Turn Off TimeOver_EventReport!!!\n");
	}
}


static void sta_cali_time_over_SM(work_t *work)
{
	extern sys_time_t meterDate;
	switch (TimeOverReportState)
	{
		case e_TIMEOVER_READMETER_TIME_FIRST:
		case e_TIMEOVER_READMETER_TIME_SECOND:
		{
			app_printf("TimeOverReportStateMachine:READMETER\n");
			
			meterSeconds = 0;
			memset((U8*)&meterDate, 0x00, sizeof(sys_time_t));
			//ReadClockWayState = e_READCLOCK_NULL;
			//ReadClockState = e_READMETER_CLOCK;
			//timer_start(readMeterClockTimer);	//readMeterClockTimerCB
			
			#if defined(ZC3750STA)
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				sta_read_meter_clk();  //这里不跳转，直接进行读表，状态由状态切换状态机改变
			#endif
			
			break;
		}
		case e_TIMEOVER_AUTOCALIBRATE:		//自动校时状态
		{
			app_printf("TimeOverReportStateMachine:AUTOCALIBRATE\n");
			if(Function_Use_Switch.AutoCaliState == 1)
			{
				sta_auto_cali_time();
			}
			break;
		}
		case e_TIMEOVER_EVENTREPORT:		//超差事件上报状态
		{
			app_printf("TimeOverReportStateMachine:EVENTREPORT\n");
			if(Function_Use_Switch.DeviationReport == 1)
			{
				sta_timeout_value_report();
			}
			break;	
	    }
	}
	if(e_TIMEOVER_NULL != TimeOverReportState)
	{
		timer_start(TimeOverCtrlTimer);
	}
}


static void sta_time_over_ctrl_pro(work_t *work)
{
	static U8 	readmetercnter = 0;

	switch (TimeOverReportState)
	{
		case e_TIMEOVER_READMETER_TIME_FIRST:  //第一次读表
		{

			if(0<=meterDate.year && 0<= meterDate.mon && 0<=meterDate.day && g_ReadMeterResFlag == e_READ_METER_TIME_SUCCESS)   //读表成功
			{
				if(TimeCalOn == DevicePib_t.AutoCalibrateSw)
				{
					TimeOverReportState = e_TIMEOVER_AUTOCALIBRATE;
				}
				else
				{
					if(PROVINCE_SHANDONG==app_ext_info.province_code)   //山东使用
					{
						TimeOverReportState = e_TIMEOVER_NULL;
					}
					else   		//国网基础
					{
						TimeOverReportState = e_TIMEOVER_EVENTREPORT;
					}
				}
			}
			else
			{
				TimeOverReportState = e_TIMEOVER_READMETER_TIME_FIRST;
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				readmetercnter++;
			}
			if(readmetercnter > 2)    //读表失败，直接跳出
			{
				TimeOverReportState = e_TIMEOVER_NULL;   
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				readmetercnter = 0;
			}
			break;
		}
		case e_TIMEOVER_AUTOCALIBRATE:
		{
			TimeOverReportState = e_TIMEOVER_READMETER_TIME_SECOND;
			break;
		}
		case e_TIMEOVER_READMETER_TIME_SECOND:   //第二次读表
		{
			if(0<=meterDate.year && 0<= meterDate.mon && 0<=meterDate.day && g_ReadMeterResFlag==e_READ_METER_TIME_SUCCESS)   //读表成功
			{
				TimeOverReportState = e_TIMEOVER_EVENTREPORT;
				app_printf("Read finish,chage state to EVENTREPORT\n");
			}
			else
			{
				TimeOverReportState = e_TIMEOVER_READMETER_TIME_SECOND;
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				app_printf("Second read meter time\n");
				readmetercnter++;
			}
			if(readmetercnter > 2)    //>2次读表失败，直接跳出
			{
				TimeOverReportState = e_TIMEOVER_NULL;   
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				readmetercnter = 0;
				app_printf("read meter times>2\n");
			}
			break;
		}
		case e_TIMEOVER_EVENTREPORT:   //山东不要这个分支
		{
			TimeOverReportState = e_TIMEOVER_NULL;
			break;
		}
		default:
		{
			TimeOverReportState = e_TIMEOVER_NULL;
			break;
		}
	}
	
	if(e_TIMEOVER_NULL != TimeOverReportState)
	{
		timer_start(TimeOverControlTimer);	
		app_printf("TimeOverReportState2 = %d\n", TimeOverReportState);
	}
}



void sta_time_over_control_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	U32 NowTime = os_rtc_time()/(24*60*60);
	NowTime *= (24*60*60);
	//DevicePib_t.TimeReportDay = 1000;
	app_printf("DevicePib_t.TimeReportDay = %d, NowTime = %d\n", DevicePib_t.TimeReportDay, NowTime);
	if(DevicePib_t.TimeReportDay != NowTime)
	{
		if(e_TIMEOVER_NULL != TimeOverReportState)
		{
			work_t *work = zc_malloc_mem(sizeof(work_t),"AppReadMeterClock",MEM_TIME_OUT);
			if(NULL == work)
			{
				return;
			}
			work->work = sta_cali_time_over_SM;
			work->msgtype = READ_METER_CLOCK;
			post_app_work(work); 
		}
	}	
	else
		app_printf("DevicePib_t.TimeReportDay==NowTime\n");
	return ;
}

void sta_time_over_ctrl_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	U32 NowTime = os_rtc_time()/(24*60*60);
	NowTime *= (24*60*60);
	
	if(DevicePib_t.TimeReportDay != NowTime)
	{
		work_t *work = zc_malloc_mem(sizeof(work_t),"TimeOverCtrllTimerCB",MEM_TIME_OUT);
		if(NULL == work)
		{
			return;
		}
		work->work = sta_time_over_ctrl_pro;
		work->msgtype = READ_METER_CLOCK;
		post_app_work(work);
	}
}

void sta_time_over_init(void)
{
	
	TimeOverCtrlTimer = timer_create(3*TIMER_UNITS,				//todo gcl  山东定时时间为7秒
		                            0,
		                            TMR_TRIGGER ,//TMR_TRIGGER
		                            sta_time_over_ctrl_timer_cb,
		                            NULL,
		                            "TimeOverCtrlTimerCB"
		                           );	

	TimeOverControlTimer = timer_create(1*TIMER_UNITS,
									0,
									TMR_TRIGGER ,//TMR_TRIGGER
									sta_time_over_control_timer_cb,
									NULL,
									"TimeOverControlTimerCB"
								   );
								   
	if(PROVINCE_HEILONGJIANG == app_ext_info.province_code)
		Function_Use_Switch.DeviationReport = 1; 	   //根据省份选择，默认开
	else
		Function_Use_Switch.DeviationReport = 0; 	   //根据省份选择，默认开
}







































#endif




