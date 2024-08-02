#ifndef _APP_CLOCK_STA_TIMEROVER_REPORT_H__
#define _APP_CLOCK_STA_TIMEROVER_REPORT_H__
#include "app_event_report_sta.h"

#if defined(STATIC_NODE) 

typedef enum
{
	e_TIMEROVER_NO_REPORT,	//超差事件不上报
	e_TIMEROVER_REPORT_OFFLINE,	//超差事件上报但不在网
	e_TIMEROVER_REPORTING,		//超差事件正在上报
}TIMEROVER_REPORT_STATUS_e;

typedef enum
{
	e_TIMEOVER_NULL,						//初始化
	e_TIMEOVER_READMETER_TIME_FIRST,		//第一次读电表时钟
	e_TIMEOVER_AUTOCALIBRATE,				//自动校时
	e_TIMEOVER_READMETER_TIME_SECOND,		//第二次读电表时钟
	e_TIMEOVER_EVENTREPORT,					//超差事件上报
}TIMEOVER_REPORT_STATE_e;	//超差事件上报状态机状态

ostimer_t *TimeOverCtrlTimer;
extern ostimer_t *TimeOverControlTimer;
extern U8	TimeOverReportState;
extern EventReportSave_t event_report_clockover_save_t;

void sta_time_over_control_timer_cb(struct ostimer_s *ostimer, void *arg);

void sta_time_over_init(void);

void sta_time_over_ctrl_timer_cb(struct ostimer_s *ostimer, void *arg);

void sta_creat_event_report_clk_overtimer(U32 MStimes);


#endif






















#endif















