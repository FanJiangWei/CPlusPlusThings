/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_read_router_cco.h
 * Purpose:	
 * History:
 * Author : PanYu
 */

#ifndef __APP_ROUTER_READ_H__
#define __APP_ROUTER_READ_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "ProtocolProc.h"


#if defined(STATIC_MASTER)

typedef enum
{
    e_UNFINISH,
    e_FINISH,
} RESULT_e;

typedef enum{
	EVT_ROUTER_RESET,               // 路由重启
    EVT_ROUTER_RESTART,             // 路由恢复
    EVT_ROUTER_STOP,				// 路由暂停
    EVT_ROUTER_IDLE,                // 路由空闲

    EVT_ROUTER_REQ,                 //路由向集中器主动申请
    EVT_ROUTER_RESP,                //路由等待集中器下发抄读项
    EVT_APS_IND,                    //路由等待APS层上报抄表数据
    EVT_DATA_REPORT,                //路由数据上报集中器
}ROUTE_RD_STATE_e;

typedef enum{
    MAX_ROUND_FINISH,
    MAX_ROUND_UNFINISH,
    FREE_ROUND_FINISH,
    FREE_ROUND_UNFINISH,
    READING_STATUS,
}ROUTE_STATE_e;

typedef struct
{
    int8_t  State;
    int8_t  CrnRMRound;
    uint16_t CrnMeterIndex;

	uint32_t  Timeout;
    uint8_t   IdleFlag;
    uint8_t   Res;
} __PACKED ROUTER_ACTIVE_INFO_t;

extern ROUTER_ACTIVE_INFO_t RouterActiveInfo_t;

void router_read_meter(U8 localFrameSeq, U8 *pMeterAddr, U8 *pCnmAddr, U8 protoType, U16 frameLen, U8 *pFrameUnit, MESG_PORT_e port);
void router_evt_router_reset(void);
int8_t router_read_meter_init(void);
void router_read_refresh_timer(void);
void router_read_suspend_timer(U32 time1S);
void router_read_stop_timer(void);
void router_read_change_route_active_state(uint8_t state, uint32_t timeout);

#endif
#endif