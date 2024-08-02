/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_eventreport.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_POWER_ONOFF_H__
#define __APP_POWER_ONOFF_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "app_event_report_sta.h"

#if defined(STATIC_NODE)


extern ostimer_t *sta_power_off_report_timer;   // 停电上报定时器
extern ostimer_t *sta_power_on_report_timer;    // 复电上报定时器
extern ostimer_t *sta_power_off_send_timer;   // 停电发送定时器
extern ostimer_t *sta_power_on_send_timer;   // 复电发送定时器


extern U8  sta_power_off_bitmap_buff[PowerOffBitMapLen];            //STA去重位图
extern U8  sta_power_off_report_bitmap_buff[PowerOffBitMapLen];      //STA上报位图

extern U8   g_PowerEventTranspondFlag;
#if defined(ZC3750STA) || defined(ZC3750CJQ2)
#define POWEROFF_REPORT_NUM 30

#define POWERON_REPORT_NUM  20

#endif


void power_off_event_report(U8 FunCode,U8 EventType ,U8 sendType,U8 *event ,U16 eventlen ,U16 ApsSeq);
U8 check_power_state(void);
void recover_power_on_report(void);
void recive_power_on_cnf(void);


int8_t  powerevent_send_timer_init(void);
int8_t  poweron_report_timer_init(void);
int8_t  poweroff_report_timer_init(void);

void modify_power_off_send_timer(U32 MStimes);
void modify_power_on_send_timer(U32 MStimes);


#define POWER_OFF_INTER   10

#define MAX_POWEROFFNUM   100

#define MAX_POWEROFFTIME  40            //生存时间

#define MAX_POWEROFF_RETURN 3

extern U8  power_off_flag;

typedef struct
{
    U8 MeterAddr[MACADRDR_LENGTH];  //地址
    U16 ApsSeq;                 //序号
    
    U8 LifeTime;                //生存时间
    U8 CfmFlag;                 //接收到CCO确认
    U8 SendFlag;                //发送标志位
    U8 Times;                   //发送次数
}POWEROFFCHECK_INFO_t;

POWEROFFCHECK_INFO_t poweroffcheck_info[MAX_POWEROFFNUM];

void SetPowerOffInfoFlag(U16 Seq);
U8 check_power_off_info(U16 Seq,U8 *addr);
extern  U16 PowerOffMeterNum;
extern  void SetPowerOffInfoFlag(U16 Seq);
extern void poweroffverify();
void updatePowerOffSeq(work_t *work);
U8 check_power_off_and_plug_out();

#if defined(ZC3750CJQ2)

void cjq2_power_off_on_flow_protec_timer_modify(uint32_t Ms);

#endif
#endif


#endif

