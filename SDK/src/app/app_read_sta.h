/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_sta_readmeter.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_STA_READMETER_H__
#define __APP_STA_READMETER_H__
#include "list.h"
#include "types.h"
#include "aps_interface.h"


#if defined(ZC3750CJQ2) 
#define MaxRecordNum 10
#else
#define MaxRecordNum 2
#endif

#define MAX_INFO_UP_LEN  2048

typedef enum
{
	FIRST_ACTIVE    = 0x01,
	MIDDLE_ACTIVE   = 0x02,
	LAST_ACTIVE     = 0x04,
}READ_STATE_e;

typedef struct{
    U8   ReadMode;          /*载波抄表模式*/	
    U8   ProtocolType;      /*载波协议类型*/	
    U16  Dtei;              /*载波源tei*/	

    U16  ApsSeq;            /*APS序号*/	
    U8   FrameCnt;          /*帧序号*/
    U8   LastFlag;          /*最后一帧标志*/

    U8   DestAddr[6];
    U16  Res2;
}__PACKED STA_READMETER_INFO_t;

typedef struct{
    U8   Res            :   4;
    U8   NoAckRetry     :   1;
    U8   FalseRetry     :   1;
    U8   RetryCnt       :   2;
}__PACKED CFG_WORD_t;

typedef struct{
    U16  OptionWord;
    U16  RecvLen;
    
    U8   RecvBuff[MAX_INFO_UP_LEN];
}STA_READMETER_UPDATA_t;

/***************重抄直接根据序号回复**********************/
extern ostimer_t *Clear13F1RMdataTimer;
extern ostimer_t* MeterApsChecktimer;

typedef struct{
    U16 RM13F1ApsRecvPacketSeq;
    U16 RM13F1recivelen;
    U8  RM13F1recivebuf[MAX_INFO_UP_LEN];
}RM13F1RCVDATA_t;
extern RM13F1RCVDATA_t gRm13f1RecvDataInfo;

typedef struct{
    U8  Addr[6];
    U16 F1F1ApsSeq;
    U16 F1F1recivelen;
    U8  F1F1recivebuf[MAX_INFO_UP_LEN];
    U8  LifeTime;
    U8  OptionWord;
}RMF1F1RCVDATA_t;
//RMF1F1RCVDATA_t F1F1RecBuf[MaxRecordNum];

void sta_read_clear_13f1_rm_data_timer_init(void);
U8 sta_read_meter_match(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);
void sta_read_app_read_meter_ind_proc(READ_METER_IND_t* pReadMeterInd);
void sta_read_get_fe_num(U8* pdata, U16 datalen, U8* FENum);

void sta_read_meter_recv_deal(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq);
void sta_read_meter_put_list(READ_METER_IND_t *pReadMeterInd, U8 protocol, U8 baud, U16 timeout,
                                              U8 intervaltime, U8 retry, U16 frameSeq, U16 frameLen, U8 *pPayload, U8 lastFlag,U8 Test);


#endif