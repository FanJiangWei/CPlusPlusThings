/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_read_jzq_cco.h
 * Purpose:	
 * History:
 * Author : PanYu
 */

#ifndef __APP_READ_METER_H__
#define __APP_READ_METER_H__
#include "list.h"
#include "types.h"
//#include "app_base_multitrans.h"
#include "ZCsystem.h"
#include "ProtocolProc.h"
#include "aps_interface.h"

#define MAX_ROUND  15
#define	MAX_CJQNUM 100
//#define DEVTIMEOUT                 40//30//3000ms
#define WATERDEVTIMEOUT            40//1200ms
//#define PACKET_INTER               5  // 100ms
//#define DEVUARTTIMEOUT             35 // UNIT:100ms
#define DEVUARTTIMEOUT35             35 // UNIT:100ms 载波串口重传为0时的超时时间
#define DEVUARTTIMEOUT18           18 // UNIT:100ms 载波串口重传为1时的超时时间

typedef struct
{
	U8 Addr[6];
}__PACKED CJQ2_ADDR_t;

typedef struct
{
	U8  Cjq2TotalNum;//总个数
	U8  Cjq2Cnt;//当前查询位置
	CJQ2_ADDR_t Cjq2ADDR_t[MAX_CJQNUM];//采集器地址
}__PACKED CJQ2_INFO_t;

extern CJQ2_INFO_t Cjq2Info_t;

#if defined(STATIC_MASTER)


void jzq_read_cntt_act_read_meter_req(U8 localFrameSeq, U8* pMeterAddr, U8* pCnmAddr, U8 protoType, U16 frameLen, U8* pFrameUnit,
            U8 afn02Flag, MESG_PORT_e port);
void jzq_read_cntt_act_cncrnt_read_meter_req(U8 localFrameSeq, U8 readMeterNum, U8* pMeterAddr, U8* pCnmAddr, U8 protoType,
            U16 frameLen, U8* pFrameUnit, U8 BroadcastFlag, MESG_PORT_e port);
void jzq_read_cco_app_read_meter_ind_proc(READ_METER_IND_t* pReadMeterInd);
void jzq_read_app_read_meter_req(void* pTaskPrmt);
void jzq_read_cco_app_read_meter_cfm_proc(READ_METER_CFM_t *pReadMeterCfm);
void jzq_read_cntt_act_cncrnt_proc(void* pTaskPrmt, void* pUplinkData);
#endif

#endif