/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_event_report_sta.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_EVENT_REPORT_STA_H__
#define __APP_EVENT_REPORT_STA_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "aps_interface.h"
#include "app.h"


#define EVENTREPORTCNT   6      //���һ�� �����´ι��˵��ж�
#define STA_CLEAN_BITMAP				        180
#define PowerOffBitMapLen (((MAX_WHITELIST_NUM+2)/8)+(((MAX_WHITELIST_NUM+2)%8)>0?1:0))

typedef enum
{
     e_BITMAP_EVENT_POWER_OFF   =1,
     e_BITMAP_EVENT_POWER_ON    =2,
     e_ADDR_EVENT_POWER_OFF     =3,
     e_ADDR_EVENT_POWER_ON      =4,
     e_LIVE_ZERO_ELEC_EVENT     =6,
     e_3_PHASE_METER_SEQ        = 0x0A,      //�ӱ���������״̬�¼��ϱ�
}StaEventPowerCutType_e;


typedef enum
{
    e_3762EVENT_REPORT_FORBIDEN  = 0,
    e_3762EVENT_REPORT_ALLOWED   = 1,
    e_3762EVENT_REPORT_BUFF_FULL   = 2,
} EventReportStatus_e;


typedef struct
{	
    U16  MeterEventSeq;
	U16  PowerSTATransEventSeq;
	U16  SuspectPowerOffEventSeq;
    U16  PowerOnEventSeq;
	U16  ElecEventSeq;
	U16  EventReportClockOverSeq;
}EventApsSeq_t;

typedef struct
{	
    U8  PowerSTATransCnt;
	U8  PowerOnSTACnt;
    U8  PowerOffCnt;

}StaReportCnt_t;

//extern EVENT_REPORT_STATUS_e    StaEventReportStatus;	
extern EventApsSeq_t aps_event_seq;
extern StaReportCnt_t sta_report_cnt;



#if defined(STATIC_NODE)
typedef enum
{
	e_with_power = 0,    // ��ͣ���ϱ�
	e_without_power,     // ����ͣ���ϱ�
}ModuleTypeCheck_e;


typedef enum
{
	e_plug_in = 0,    // ģ����λ
	e_plug_out,       // ģ��γ�
}ModulePlugState_e;

typedef enum
{
	e_vol_normal = 0,  // δ��⵽��Դ����
	e_vol_loss,        // ��⵽��Դ����
}VolDetectState_e;


typedef enum
{
    e_zc_lost = 0,    // ���㶪ʧ
	e_zc_losting,
    e_zc_get,         // ����δ��ʧ
}ZeroCrossState_e;

typedef enum
{
    e_power_is_ok = 0,
	e_power_off_now ,    //ͣ����Ϣ�Ϸ�
	e_power_off_last,    //ǰ��״̬��ͣ��
	e_power_on_now,      //����״̬�Ϸ�
}PowerState_e;

typedef enum
{
	e_reset_enable = 0,
	e_reset_disable,
}SoftResetState_e;

typedef enum
{
    e_event_none = 0,
    e_event_get,
}EventState_e;

typedef struct{
    ModuleTypeCheck_e   power_type;
    ModulePlugState_e   plug_state;
    VolDetectState_e    vol_detect_state;
    ZeroCrossState_e    zero_cross_state;
    SoftResetState_e    soft_reset_state;
}ModuleState_t;

typedef struct
{	
    U8  check;           // �¼�����־
    U8  flag;            // ��ͨ�¼���־���յ���������ʱ����
	U8  Oop20flag;       // 20���STA�¼���־
	U8  len;             // �¼��ϱ���Ϣ����

	U8  outlflag;			//δ����ǰ���¼�
	U8  cnt;
    U16 FilterRepeat;    // ���δ�յ�ȷ��֡�Ҹߵ�ƽδֹͣ������һ������ 5��ƴ�
    EventReportStatus_e  event_report_status; //�¼��ϱ�״̬
	U8  buf[255];        // �¼��ϱ���Ϣ������
}EventReportSave_t;

extern ostimer_t *event_report_timer;
extern ostimer_t *exstate_verify_timer;     // �ⲿ״̬��鶨ʱ��
extern ostimer_t *exstate_reset_timer;     // �ⲿ״̬�����λ��鶨ʱ��
extern ostimer_t *exstate_event_timer;     // �ⲿ״̬�����¼���鶨ʱ��

extern ModuleState_t module_state;
extern EventReportSave_t event_report_save;
extern EventReportStatus_e    g_StaEventReportStatus;



int8_t  exstate_verify_timer_init(void);
int8_t  exstate_reset_timer_init(void);
int8_t  exstate_event_timer_init(void);

void modify_exstate_verify_timer(U32 ms_time);


U8 sta_clean_bitmap_timer_init(void);
void modify_sta_clean_bitmap_timer(U32 MStimes);
extern void Sta20MeterRecvDeal(uint8_t *revbuf, uint16_t revlen);
int8_t  event_report_timer_init(void);
void modify_event_report_timer(U32 MStimes);

extern U8 Check698EventReport(U8* pDatabuf, U16 dataLen);

extern void sta_clean_bitmap(void);
#endif
#endif

