/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:		ProtocolProc.h
 * Purpose:	ProtocolProc interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __PROTOCOLPROC_H__
#define __PROTOCOLPROC_H__
#include "ZCsystem.h"
#define MALLOC 0
#define FREE   1

extern U32 wdtspy;

extern U32 max_p_addr[2];
/*
#if defined(record_malloc)

typedef struct
{
	U16  pos;
    U16  num[MAXNUMMEM];
	char *s[MAXNUMMEM];
    void *mallcaddr[MAXNUMMEM];
}__PACKED MEM_ADDR_t;

extern MEM_ADDR_t MallocMemAddr_t;
extern MEM_ADDR_t FreeMemAddr_t;

#endif
*/
extern U16 NUM;


/*

typedef struct
{
	U16  MsduSeq[MAXNUMMEM];
	U8  starAODV[MAXNUMMEM];
	U8  mode[MAXNUMMEM];
	U16 DstTei[MAXNUMMEM];
	U16 NextTei[MAXNUMMEM];
}__PACKED RECORDMETER;

extern RECORDMETER readmeter;

void reacordmeter(U16 MsduSeq,U8 starAODV ,U8 mode,U16 DstTei,U16 NextTei);
*/




/******************************************Protocol*****************************************/
typedef enum
{
    e_ZC_MSG,
    e_DLT645_MSG,
    e_DLT698_MSG,
    e_GW13762_MSG,
    e_T188_MSG,
    e_GD2016_MSG,
    e_PLC_2016,
    e_other=0xe0,
} MESG_PROTOCOL_e;

/********************************************Port*******************************************/
typedef enum
{
    e_PHY_MSG,//单向发
    e_MACS_MSG,//单向收
    e_MAC_MSG, //双向收发
    e_NET_MSG, //双向收发
    e_APS_MSG, //双向收发
    e_APP_MSG, //双向收发
    e_UART1_MSG,//单向发
    e_UART2_MSG,
} MESG_PORT_e;

typedef void (*DealFunc)(U8 *pMsgData);



typedef struct
{
    MESG_PORT_e 		SrcTask_e;
    MESG_PORT_e 		DesTask_e;
    MESG_PROTOCOL_e	Protocol_e;
    void (* Func) (MSG_t *pMsg);
} __PACKED TaskMsgFunc;

typedef enum
{
    e_UART_RECIVE_3762,
	e_RD_CTRL_UART_RECIVE_3762,
	e_PLC_RECIVE_3762,
} UART_3762_e;
typedef enum
{
    e_UART_RECIVE_GD2016,
} UART_GD2016_e;

typedef enum
{
    e_UART_RECIVE_645,
	e_UART_EVENT_REPORT_645,
} UART_645_e;
typedef enum
{
    e_UART_RECIVE_698,
	e_UART_EVENT_REPORT_698,
} UART_698_e;
typedef enum
{
	e_RD_CTRL,
}PHY_TO_APP_e;




#endif /* __PROTOCOLPROC_H__ */

