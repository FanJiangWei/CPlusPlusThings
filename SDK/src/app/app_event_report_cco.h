/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_event_report_cco.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_EVENT_REPORT_CCO_H__
#define __APP_EVENT_REPORT_CCO_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "aps_interface.h"
#include "app.h"
#include "app_meter_bind_sta.h"
#include "app_common.h"
#include "app_gw3762.h"

#define STA_POWER_ON
#define POWERNUM         1015

#define MAXBMPOWERNUM    40
#define MAXADDRPOWERNUM  36

#define EVENT_REPORT_NUM	    1000//过滤最大存储事件上报数
#define EVENT_REPORT_LIFE_TIME  240

#define TIMER_UNITS      1000
extern ostimer_t *clean_bitmap_timer;

#if defined(STATIC_MASTER)

#define    CCO_CLEAN_BITMAP_TIME			240
#define    CCO_CLEAN_PERIOD_TIME            4

typedef struct{
    U8       PowerFlag             : 1;
    U8       PowerReportFlag       : 1;
    U8       RemainTime            : 6 ;
}__PACKED BitMapPowerInfo_t;


typedef struct{
    U8              MacAddr[6];
}__PACKED PowerAddrList_t;

typedef struct{
    U8 		MacAddr[6];
	U16 	Seq;
    U16		RemainTimeEvent;
}EventReportInfo_t;


BitMapPowerInfo_t power_off_bitmap_list[MAX_WHITELIST_NUM];
BitMapPowerInfo_t power_on_bitmap_list[MAX_WHITELIST_NUM];

PowerAddrList_t     power_addr_list[POWERNUM];
BitMapPowerInfo_t   power_off_addr_list[POWERNUM];
BitMapPowerInfo_t   power_on_addr_list[POWERNUM];


void modify_cco_event_send_timer(uint32_t first);

U8 cco_eventreport_timer_init(void);
void cco_clean_bitmap_timer_init(void);

EventReportInfo_t event_report_addr_list_t[EVENT_REPORT_NUM];
U8 check_event_report_by_addr(U8 *Addr,U16 seq);
void modify_event_report_addr_list_timer();
void event_report_list_proc(work_t *work);
U8 renew_power_falg(BitMapPowerInfo_t *powerinfo,U8 state,U8 reportflag,U8 time);
U8 sub_power_remain_time(BitMapPowerInfo_t *powerinfo,U8 *Addr);


#endif
#endif

