/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_base_multitrans.h
 * Purpose:	
 * History:
 * Author : PanYu
 */


#ifndef __APP_MULTI_TRANS_H__
#define __APP_MULTI_TRANS_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "DataLinkGlobal.h"


#define  MULTITRANS_DEALYSECOND     10
#define  MULTITRANS_INTERVAL        2            //1s间隔

#define  MAX_CTRL_TIMER_NUM         80           //最大的控制定时器数量

#define  GUARD_MULTI_TIME  (60*1000) //one=1ms guard multi_trans_one_timer 60S

#define  CONCURRENCYSIZE            25          //并发抄表默认条数

#define  CONTASKSENDMAX             12           //最大入队失败重试次数

typedef enum
{
	UNEXECUTED,
	EXECUTING,
	NONRESP,
}TASK_STATE_e;

typedef enum
{
    F1F1_READMETER,
	AFN13F1_READMETER,
	AFN02F1_READMETER,
    AFN14F1_READMETER,

    SLAVEREGISTER_QUERY,
    SLAVEREGISTER_LOCK,
    COLLECTDATA_QUERY,//采集功能是否支持查询
    
    MODULE_ID_QUERY,

	INDENTIFICATION_QUERY,
	PHASEPOSITION_QUERY,
	CLOCK_INFO_QUERY,
	F1F100_READMETER,
}TASK_SEND_TYPE_e;


typedef struct{
	list_head_t link;
    list_head_t vertical_link;

	uint16_t lid:6;
	uint16_t nr:10	;
	uint16_t thr;
    void (*TaskAddOne)(void );
    void (*TaskRoundPro)(void );

    ostimer_t 	*Suspend_timer;		/*暂停并发节奏定时器*/
    ostimer_t 	*Guard_timer;		/*保护并发队列定时器*/
}multi_trans_header_t;

typedef struct{
	list_head_t link;
    ostimer_t 	*ctrl_timer;		/*单个任务并发定时器*/
    uint32_t     seq;
    
}multi_ctrl_timer_t;


//并发任务列表全局变量
typedef struct{
	
    uint8_t	   Valid     :  1;   //valid    0 incalid  1 valid
	uint8_t    Send      :  1;   //result1  0 unstart  1 success 
	uint8_t    Result    :  1;   //result2  0 unstart  1 success 
	uint8_t    Res	     :  5;
}__PACKED CCO_TASK_TEI_RECORD;


//并发任务点位、流程、缓存区配置
typedef struct{
    //当前任务控制器

    uint8_t      lid;
    uint8_t      CrntNodeAdd[6];
    uint16_t     CurrentIndex;   //
    int8_t       ReTransmitCnt;  //剩余重试次数
    uint8_t      DealySecond;	 /* 延迟启动时间*/ 
    uint8_t      CycleNum;       //最大轮次
    
    void (*TaskNext)();
    
    CCO_TASK_TEI_RECORD     taskPolling[MAX_WHITELIST_NUM]; //任务TEI有效、结果                     
}__PACKED  CONCURRENCY_CTRL_INFO_t;



extern multi_trans_header_t  HIGHEST_PRIORITY_LIST;	extern ostimer_t *highest_priority_timer;
extern multi_trans_header_t  F1F1_TRANS_LIST;  	    extern ostimer_t *f1f1_trans_timer ;
extern multi_trans_header_t  F14F1_TRANS_LIST; 		extern ostimer_t *f14f1_trans_timer ;
extern multi_trans_header_t  REGISTER_QUER_LIST; 	extern ostimer_t *register_quer_timer ;
extern multi_trans_header_t  UPGRADE_START_LIST;    extern ostimer_t *upgrade_start_timer;
extern CONCURRENCY_CTRL_INFO_t  CTRL_UPGRADE_START_Info;

extern multi_trans_header_t  UPGRADE_QUERY_LIST;    extern ostimer_t *upgrade_query_timer;
extern CONCURRENCY_CTRL_INFO_t  CTRL_UPGRADE_QUERY_Info;

extern multi_trans_header_t  UPGRADE_STATION_INFO_QUERY_LIST;    extern ostimer_t *upgrade_station_info_query_timer;
extern CONCURRENCY_CTRL_INFO_t  CTRL_UPGRADE_STATION_INFO_QUERY_Info;

extern multi_trans_header_t  INDENTIFICATION_COLLECT_BF_LIST;extern ostimer_t *indentification_multi_timer;
extern multi_trans_header_t  PHASE_POSITION_BF_LIST; extern ostimer_t *phaseposition_multi_timer;

extern multi_trans_header_t  MODULE_ID_QUERY_LIST;  extern ostimer_t *moduleid_query_timer;
extern CONCURRENCY_CTRL_INFO_t  CTRL_MODULE_ID_QUERY_Info;

extern multi_trans_header_t  QUERYSWORVALUE_LIST;   extern ostimer_t *query_sworvalue_timer ;

extern multi_trans_header_t  SETSWORVALUE_LIST;		extern ostimer_t *set_sworvalue_timer ;

typedef void (*TaskUpProc)(void *pTaskPrmt, void *pUplinkData);

typedef struct multi_trans_s {
	list_head_t link;

	uint16_t  lid     :4;					/* lid越大优先级越高，越排在链表前面*/	
	uint16_t  SrcTEI :12;					/* 如果为抄控器，值为抄控器TEI*/ 
    uint16_t  DeviceTimeout;			    /* 设备超时时间*/
	uint8_t   Framenum;                     /* 串口帧序号*/
    
    uint8_t   MsgPort;                      /* 区分376.2上行给集中器还是抄控器*/
    uint8_t   SendRetryCnt;                 /* 链路层发送失败次数，防止入队失败，准备重传，尽量不要直接丢弃任务 */
    uint8_t   Res[1];
    
    uint8_t   Addr[6];
    uint8_t   CnmAddr[6];

	uint8_t   State          :3;            /* 执行状态*/
    uint8_t   ProcCnt        :4;            /* 执行次数：发消息处理前+1，消息处理时-1，删除条目时需要确认处理次数为0*/
    uint8_t   GuardFlag      :1;            /* 定时器防止丢消息：update重置为0, CB 内设置为1*/
	uint8_t   SendType;
	uint16_t  StaIndex;                     /* 自有任务，TEI下标，CCO任务;白名单下标*/
	
	uint16_t  DatagramSeq;					/* 载波帧序号*/
    uint8_t    DealySecond;					/* 延迟启动时间*/
    uint8_t    ReTransmitCnt :5;             /* 重发次数*/
    uint8_t    BroadcastFlag :2;             /* 广播标志*/
    uint8_t    BroadcastNow  :1;             /* 正在广播标志*/
	
	uint8_t   DltNum;						/* 携带报文数量*/
    uint8_t   ProtoType;					/* 通信协议类型 */
	uint16_t  FrameLen;					    /* 报文长度 */

    void (*TaskPro)(void *pTaskPrmt);    //载波或无线下行
    //void (*TaskSendFail)(void *pTaskPrmt);    //发送失败
	void (*TaskUpPro)(void *pTaskPrmt, void *pUplinkData);//上行处理
    void (*EntryListfail)(int32_t err_code, void *pTaskPrmt);
    int32_t (*EntryListCheck)(multi_trans_header_t *list, struct multi_trans_s *new_list);
	void (*TimeoutPro)(void *pTaskPrmt);

	multi_trans_header_t	 *pMultiTransHeader;	/*任务链表头*/
    multi_ctrl_timer_t       *pTimerHeader;         /*单独控制定时器*/
	ostimer_t 				  *CRT_timer;		    /**/
	
	uint8_t   FrameUnit[0];				/* 报文内容 */
} __PACKED multi_trans_t;

typedef struct
{
	multi_trans_header_t *pListHeader;
    
    uint8_t       Upoffset;
    uint8_t       Downoffset;
    uint8_t       Cmplen;    
    uint8_t       Res1[1];
    
	//uint16_t      AsduLength;
	//uint8_t       Asdu[0];
} MULTI_TASK_UP_t;

typedef struct manage_link_header_s {
	list_head_t vertical_link;
	
	uint16_t nr;
	uint16_t thr;
}manage_link_header_t;

typedef struct{
	list_head_t link;

	uint16_t nr;
	uint16_t thr;
    
}manage_timer_t;


int8_t manage_link_init(void);

int8_t multi_trans_timer_init(void);
void multi_trans_put_list(multi_trans_t meter,uint8_t *payload);
void multi_trans_find_plc_task_up_info(MULTI_TASK_UP_t *pMultiTaskUp, void *pUplinkData);
uint8_t  multi_trans_check_broadcast_num(multi_trans_t  *pOneTask);
uint8_t multi_trans_add_task_form_poll(CONCURRENCY_CTRL_INFO_t *pConInfo);
uint8_t multi_trans_add_con_result(  CONCURRENCY_CTRL_INFO_t *pConInfo,uint16_t Index);
void multi_trans_ctrl_plc_task_round(multi_trans_header_t *list,CONCURRENCY_CTRL_INFO_t *pConInfo);
void multi_trans_stop_plc_task(void *buf);
U8 multi_trans_register_list_full(void);
void multi_trans_find_plc_multi_task_send_fail(MULTI_TASK_UP_t *pMultiTaskUp, void *pUplinkData);
void get_info_from_net_set_poll(CONCURRENCY_CTRL_INFO_t *pConInfo);
uint8_t multi_trans_task_self_init(multi_trans_header_t *pHeader,
                                        uint16_t nr, uint16_t thr, uint8_t lid, 
                                        void* TaskAddOne,void* TaskRoundPro,ostimer_t 	*pSuspend_timer,
                                        ostimer_t 	**pGuard_timer);

#endif

