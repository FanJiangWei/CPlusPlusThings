/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	  CJQTASK.h
 * Purpose:	CJQ interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __CJQTASK_H__
#define __CJQTASK_H__

#include "app_meter_search_cjq.h"
#include "SchTask.h"

#if defined(STATIC_NODE)

//event_t *Mutex_CJQ_485;

#define CJQ_INIT_BAND           2

#define CJQ_MEM_SIZE			5
#define METERNUM_MAX			32
#define SEARCHTIME				2
#define BAUD_BASE				1200
#define TIME_OUT				2
#define INTERVAL				60
#define POWER_TIME_OUT          1


#define CJQ_DLT645_BASE_LEN		12
#define CJQ_DLT645_HEAD2_POS	7
#define CJQ_DLT645_CTL_POS		8
#define CJQ_DLT645_LEN_POS		9
#define CJQ_DLT645_HEAD_VALUE	0x68
#define CJQ_DLT645_END_VALUE	0x16

//#define CJQ_SET_AS_RELAY        //采集器设置中继器模式




/*
typedef enum
{
	PROTOCOLBAUD_RES,
	PROTOCOLBAUD_072400,
	PROTOCOLBAUD_6989600,
	PROTOCOLBAUD_6982400,
	PROTOCOLBAUD_079600,
	PROTOCOLBAUD_971200,
	PROTOCOLBAUD_972400,
	PROTOCOLBAUD_6984800,
	PROTOCOLBAUD_074800,
	PROTOCOLBAUD_071200,
	PROTOCOLBAUD_974800,
	PROTOCOLBAUD_END,
}PROTOCOLBAUD_E;
*/
typedef enum
{
	COLLECT_ADDRMODE_NOADDR,
	COLLECT_ADDRMODE_ADDR,
}COLLECT_ADDRMODE_E;

typedef enum
{
	SEARCH_STATE_CHECKMETER,
	SEARCH_STATE_AA,
	SEARCH_STATE_TRAVERSE,
	SEARCH_STATE_07698,
	SEARCH_STATE_STOP,
	SEARCH_STATE_FULL,
}SEARCH_STATE_E;

typedef enum
{
	SEARCH_RESULT_TIMEOUT,
	SEARCH_RESULT_SUCCESS,
	SEARCH_RESULT_ERRCODE,
}SEARCH_RESULT_E;

#pragma pack(push)
#pragma pack(1)
typedef struct
{
	U8 MeterAddr[7];	//电表地址
	U8 Protocol	:3;		//规约 0缺省 1:97 2:07 3 698
	U8 BaudRate :5;		//波特率 1 1200 2 2400 4 4800 8 9600

	U8 rfThreshold;		//无线场强
	U8 PowerFlag;
}METER_INFO_ST;

typedef struct
{
	U8 WaterAddr[7];	//电表地址
}WATER_INFO_ST;

typedef struct
{
	U8	waterInfo[40];
	U8	len;
	U8  rssi;
}SINGLE_WATER_INFO_ST;

typedef struct
{
	U8 Vendorcode[2];	//厂商代码  //'C''Z'
	U8 ProductType[2];	//产品类型  //

	U8 Day;				//08
	U8 Month;			//09
	U8 Year;			//17
	U8 Version2;		//01

	U8 Version1;		//0.0
}VERSION_ST;


typedef struct
{
	U8 ResetTime;
	U8 Flag[2];					//判断是否为A55A
    U8 AddrMode	:1;				//0：无地址模式；1：有地址模式
    U8 Res		:7;				//保留

    U8 CollectAddr[MACADRDR_LENGTH];			//无地址模式为6个BB，有地址模式为采集器第一个搜到的表地址
    U8 SetCCOAddr[MACADRDR_LENGTH];			//无地址模式无效，有地址模式为配置的CCO地址,未配置为全0，支持任意网络入网

    VERSION_ST Version;			//厂商代码
    U8 Timeout;					//每次搜表延时时间 单位秒
    U8 Interval;				//搜表间隔 单位 分钟
    U8 MeterNum;				//搜到的电表的个数

	U32 ResetNum;

    METER_INFO_ST MeterList[METERNUM_MAX];//电表信息列表
    METER_INFO_ST power_on_meter_list[METERNUM_MAX];//电表信息列表

    U8 power_on_meter_num;
	U8 CrtUseListSeq;           //当前使用电表信息列表作为MAC地址的序号
    U8 PowerOFFflag;
	U8 Res1;
	
	FLASH_FREQ_INFO_t FreqInfo;           
	Rf_CHANNEL_INFO_t RfChannelInfo;
    U8 Cs;						//整个结构体校验和
}COLLECT_INFO_ST;


#pragma pack(pop)


//extern void MeterUartInit(meter_dev_t *dev, U32 Baud);
extern void uart_send(uint8_t *buffer, uint16_t len);

void CJQ_Flash_Save();

COLLECT_INFO_ST CollectInfo_st;

VERSION_ST* CJQ_Extend_ReadVersion();
void CJQ_Extend_ParaInit();
U8 CJQ_Extend_MeterNum();
METER_INFO_ST* CJQ_Extend_MeterInfo();
U8* CJQ_Extend_ReadBaseAddr();
void CJQ_Extend_SetBaseAddr(U8* Addr);
U8 CJQ_Extend_ReadInterval();
void CJQ_Extend_SetInterval(U8 time);
U8 CJQ_Extend_ReadResetNum();
void CJQ_Extend_ShowInfo();
void CJQ_AddMeterInfo(U8* Addr, U8 Protocol, U8 Baud);
U8 CJQ_CheckMeterExist(U8* Addr, U8 Protocol, U8 Baud);
void CJQ_Extend_ShowVersion();
void CJQ_PRINT_ON();
void CJQ_PRINT_OFF();
void CJQ_DelMeterInfoByIdx(U8 idx);
U8 CJQ_Extend_ReadTimeout();
void CJQ_Extend_SetTimeout(U8 time);
U8 CJQ_CheckMeterExistByAddr(U8* Addr, U8* Protocol,U8* Baud);
U8 CJQ_Func_JudgeBCD(U8 *buf, U8 len);




typedef void (*CJQRecvProc)(void);

#if defined(ZC3750CJQ2)
extern uint8_t cjq_changeaddrflag;

void MeterNumIsFull(SEARCH_CONTROL_ST *pCtrl);

void CJQ_AA_Begin(SEARCH_CONTROL_ST *pCtrl);
U8 CJQ_Func_GetProtocolBaud(SEARCH_CONTROL_ST *pCtrl,U8 *Protocol,U8 *Baud);

extern U8 cjq_read_addr[6];
U8	cmp_cjq_meterlist(U8 *meter);
U8 CJQ_Flash_Check(U8 *buf, U16 len);
void CJQ_Flash_Init(cjq_dev_t *dev);
void CJQ_Check_Begin(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Check_Succ(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Check_Errocde(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Check_Next(SEARCH_CONTROL_ST *pCtrl);
void CJQ_AA_Next(SEARCH_CONTROL_ST *pCtrl);
void CJQ_AA_Succ(SEARCH_CONTROL_ST *pCtrl);
void CJQ_AA_Errcode(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Check698_Next(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Check698_Succ(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Check698_Errcode(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Traverse_Next(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Traverse_Succ(SEARCH_CONTROL_ST *pCtrl);
void CJQ_Traverse_Errcode(SEARCH_CONTROL_ST *pCtrl);

#endif
#endif

#endif /* __ROUTERTASK_H__ */

