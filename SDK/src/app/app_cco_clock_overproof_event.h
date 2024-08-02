/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_event_report_cco.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_CCO_CLOCK_OVERPROOF_EVENT_H__
#define __APP_CCO_CLOCK_OVERPROOF_EVENT_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "aps_interface.h"
#include "app.h"
#include "app_meter_bind_sta.h"
#include "app_common.h"
#include "app_gw3762.h"

#if defined(STATIC_MASTER)
#define    MAX_CTJ_EVREPORTCLOCKOVER_NUM	300
typedef struct{
    U8       MacAddr[6];
    U16      RemainTimeEvent;
    U16      ApsSeq;
	U16      num;
}ClockOverProofEventList_t;


//CCO给集中器进行超差事件上报缓存结构体
typedef struct{
	U8 LenNull_flag;
	AppGw3762SlaveType_e devtype; 
	AppProtoType_e proto; 
	U8 meterAddr[6];
	U16 len;
	U8 data[30];
	MESG_PORT_e port;
	U8 evclockover_Flg;
}ClockOverProofEventInfo_t;

ClockOverProofEventList_t clock_overproof_event_list[MAX_WHITELIST_NUM];
ClockOverProofEventInfo_t clock_overproof_event_info[MAX_CTJ_EVREPORTCLOCKOVER_NUM];

extern ostimer_t *clock_overproof_event_delduplitimer;
extern ostimer_t *clock_overproof_event_ctrl_timer;

void clear_clock_overproof_event_ctrl_list();
void clear_clock_overproof_event_list();
void  clock_overproof_event_report(EVENT_REPORT_IND_t *pEventReportInd);
void clock_overproof_event_timer_init();





#endif
#endif