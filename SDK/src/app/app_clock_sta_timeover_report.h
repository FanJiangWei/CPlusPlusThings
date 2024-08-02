#ifndef _APP_CLOCK_STA_TIMEROVER_REPORT_H__
#define _APP_CLOCK_STA_TIMEROVER_REPORT_H__
#include "app_event_report_sta.h"

#if defined(STATIC_NODE) 

typedef enum
{
	e_TIMEROVER_NO_REPORT,	//�����¼����ϱ�
	e_TIMEROVER_REPORT_OFFLINE,	//�����¼��ϱ���������
	e_TIMEROVER_REPORTING,		//�����¼������ϱ�
}TIMEROVER_REPORT_STATUS_e;

typedef enum
{
	e_TIMEOVER_NULL,						//��ʼ��
	e_TIMEOVER_READMETER_TIME_FIRST,		//��һ�ζ����ʱ��
	e_TIMEOVER_AUTOCALIBRATE,				//�Զ�Уʱ
	e_TIMEOVER_READMETER_TIME_SECOND,		//�ڶ��ζ����ʱ��
	e_TIMEOVER_EVENTREPORT,					//�����¼��ϱ�
}TIMEOVER_REPORT_STATE_e;	//�����¼��ϱ�״̬��״̬

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















