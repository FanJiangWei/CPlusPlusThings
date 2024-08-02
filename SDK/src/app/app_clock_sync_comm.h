#ifndef _APP_CLOCK_SYNC_COMM_H_
#define _APP_CLOCK_SYNC_COMM_H_
#include "app_sysdate.h"
//#include "app_eventreport.h"
//#include "app_multitrans.h"

#define _698_FRAME_TIME_POS			9	 //698广播校时帧时间数据起始点
#define CCO_DEFAULT_REQUEST_TIME_PERIOD		120  //单位：分钟

#define CCO_START_REQUEST_TIME				30   //单位：秒
typedef enum 
{
    e_AUTO_TIMING_SWITCH = 0x01,
    e_OVER_TIME_VALUE_SET = 0x03,
    e_CLOCK_MAINTAIN_INFO,
}TIME_OVER_MANAGE_TYPE_e;
/*
typedef struct
{	
    U8 check;
	U8 len;
    U8 cnt;
    U8 Macadddr[6];
	U8 buf[255];
}QUERY_CLKORVALUE_SAVE_t;

QUERY_CLKORVALUE_SAVE_t Query_ClkOrValue_save_t;
typedef struct
{	
    U8 clk_cnt;
	U8 clk_flag;
	U8 buf[50];
	U16 bufLen;
    U8 OverValue_cnt;
    U8 OverValue_flag;
    U8 MacAddr[6];
    U8 Value;
}SET_CLKORVALUE_SAVE_t;

SET_CLKORVALUE_SAVE_t set_ClkOrValue_save_t;*/



typedef struct
{
    U16   ProtocolVer                :6;
    U16   HeaderLen                  :6;
    U16   Res                        :4;
    U8    Macaddr[6];
    
    U16   Type                       :8;
	U16   AsduLength				 :8;
    U8    Asdu[0];
}__PACKED CLOCK_MAINTAIN_HEADER_t;

typedef struct
{
	U32    ProtocolVer				:6;
	U32	   HeaderLen                :6;
	U32    DataRelayLen				:12;
	U32    PacketSeq   				:8;
	U32    NTB;
	//U32    AsduLength;
	U8     Asdu[0];
}__PACKED  TIMER_CALIBRATE_HEADER_t;

typedef struct
{
    U16   ProtocolVer                :6;
    U16   HeaderLen                  :6;
    U16   Res                        :4;
    
    U8    Macaddr[6];
    U8    Res1;
    
	U8    Asdu[0];
}__PACKED QUERY_CLKSWOROVER_VALUE_HEADER_t;

typedef struct
{
    U16   ProtocolVer                :6;
    U16   HeaderLen                  :6;
    U16   Res                        :4;
	U8    Macaddr[6];
	
    U8	  AutoSwState;
    U8    Res1;
    U8    Time_Over_Value;
    
    //U8    Res2;
	U8    Asdu[0];
}__PACKED QUERY_CLKSWOROVER_VALUE_UP_HEADER_t;

typedef struct
{
	U8	Type;
	U8  SendType;
	U16 AsduLength;
 
    U8 DstMacAddr[MACADRDR_LENGTH];
    U8 SrcMacAddr[MACADRDR_LENGTH];
    U8 Asdu[0];
} __PACKED CLOCK_MAINTAIN_REQ_t;


/*时钟超差管理*/
typedef struct
{
	
	U8 SendType;
	U16 AsduLength;
	
	U8 PacketType;
	U8 CLK_SwState;
	U8 DstMacAddr[MACADRDR_LENGTH];
	U8 SrcMacAddr[MACADRDR_LENGTH];
	U8 Asdu[0];  
	
}__PACKED CLK_OVER_MANAGE_REQ_t;

typedef struct
{
	
	U8 packetType;
	U8 SendType;
	U16  AsduLength;
	
	U8 DstMacAddr[MACADRDR_LENGTH];
	U8 SrcMacAddr[MACADRDR_LENGTH];
	U8 Asdu[0];
}__PACKED TIMER_CALIBRATE_REQ_t;


typedef struct
{
	
	U8 SendType;
	U16 AsduLength;
	
	U8 Macaddr[6];
	U8 DstMacAddr[MACADRDR_LENGTH];
	U8 SrcMacAddr[MACADRDR_LENGTH];
	U8 Asdu[0];  
}__PACKED QUERY_CLKSWOROVER_VALUE_REQ_t;


typedef struct
{
	U16   AsduLength;
	U16   Res;
	
	U32   NTB;
	U8	  Asdu[0];
}__PACKED TIMER_CALIBRATE_IND_t;

typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;
	
	U8		 Type;
	U8       Res[3];
	
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
    U8		 Asdu[0];
} __PACKED CLOCK_MAINTAIN_IND_t;
typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;
	
	///U8		 Type;
	
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
    U8		 Asdu[0];
} __PACKED CLOCK_MAINTAIN_CFM_t;

typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;
	U8		 Type;
	U8       Res[3];
	
	U8		 DstMacAddr[MACADRDR_LENGTH];
	U8 		 SrcMacAddr[MACADRDR_LENGTH];
	U8		 Asdu[0];
}__PACKED CLOCK_MAINTAIN_RSP_t;



typedef struct
{
	U16 AsduLength;
	U8	AutoSwState;
	U8  Time_Over_Value;
	
	U8 	Macaddr[6];
	U8  Res[2];

	
	U8	DstMacAddr[MACADRDR_LENGTH];
	U8 	SrcMacAddr[MACADRDR_LENGTH];
	U8  Asdu[0]; 
}__PACKED QUERY_CLKSWOROVER_VALUE_CFM_t;

typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;

	
	U8 		 Macaddr[6];
	U8		 AutoSwState;
	U8       Time_Over_Value;
	U8		 DstMacAddr[MACADRDR_LENGTH];
	U8 		 SrcMacAddr[MACADRDR_LENGTH];
	U8		 Asdu[0];
}__PACKED QUERY_CLKSWOROVER_VALUE_IND_t;

typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;
	
	
	U8 		 Macaddr[6];
	U8		 AutoSwState;
	U8       Time_Over_Value;
	U8		 DstMacAddr[MACADRDR_LENGTH];
	U8 		 SrcMacAddr[MACADRDR_LENGTH];
	U8		 Asdu[0];
}__PACKED QUERY_CLKSWOROVER_VALUE_RESPONSE_t;

typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;

	U8		TimeType;
	U8  	Res[3];
	
	U8		 Asdu[0];
}__PACKED CALIBRATE_TIME_OVER_REQ_t;

typedef struct
{
	U16 	 AsduLength;
	U16 	 SendType;
	
	U8		ReadClockType;
	U8  	Res[3];
	U8		 Asdu[0];
}__PACKED READMETER_CLOCK_REQ_t;

//陕西，湖南使用
typedef struct{
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
    U8       DateLen;
    U8       Date[0];	 //日期
    
}__PACKED MODULE_TIMESYNC_REQ_t;
typedef struct{
    U32   ProtocolVer                :6;
    U32   HeaderLen                  :6;
    U32   Reserve1                   :4;
	U32   Reserve2                   :4;
    U32   DataLength                 :12;
    U8    Data[0]                       ;
}__PACKED MODULE_TIMESYNC_HEADER_t;


//自动校时开关功能状态
typedef enum
{
	TimeCalOff =0x00,
	TimeCalOn,
}AUTO_TIMING_SWITCH_e;
typedef enum
{
	e_READCLOCK_NULL,				//初始化
	e_READMETER_CLOCK,				//读电表时钟
	e_CLOCKMAINTAIN_MANAGE, 		//时钟管理中的自动校时
	e_STA_CLOCKRECOVER, 			//从模块时钟恢复
}READMETER_CLOCK_STATE_e;	//读取电能表时钟的状态机状态

enum
{
	e_Jzq_faster,	//电能表低于集中器时间
	e_meter_faster,	//电能表超出集中器时间
};


typedef struct
{
 	U8  LHour;
 	U8  HHour;
 	U8  Min;
 	U8  Sec;
}__PACKED CLK_OVER_PROOF;

extern sys_time_t meterDate ;
extern U32 meterSeconds ;
extern U8 TimeCalFlag;

//extern void ClockMaintainManageInd(work_t *work) ;
extern void sta_time_manager_init();
extern void ApsExactTimeCalibrateRequest(TIMER_CALIBRATE_REQ_t *TimerCalBrateReq);
extern void sta_cali_time_diff(sys_time_t *MeterDate, S32 *TimeDiffence, U32 *MeterSeconds);
//extern int32_t QuerySwORValueEntryListCheck(multi_trans_header_t *list, multi_trans_t *new_list);
//extern  void QuerySwORValueEntryListfail(int32_t err_code, void *pTaskPrmt);
extern  void cco_query_sw_or_value_timeout(void *pTaskPrmt);

//陕西湖南使用
void ApsModuleTimeSyncRequest(MODULE_TIMESYNC_REQ_t *pModuleTimeSyncReq_t);
void ApsClockMaintainRequest(CLOCK_MAINTAIN_REQ_t *pClockMaintainReq_t);
void ApsQueryClkSwAndOverValueRequest(QUERY_CLKSWOROVER_VALUE_REQ_t *pQuery_CLKSwOROverValueReq);
void ApsSetCLKSWRequest(CLK_OVER_MANAGE_REQ_t      *pSetClkSwReq);
void ApsExactTimeCalibrateRequest(TIMER_CALIBRATE_REQ_t *TimerCalBrateReq);
void ClockSetReplyAps(U8 *pDstAddr, U8 *pSrcAddr);
void ApsQueryClkSwAndOverValueResp(QUERY_CLKSWOROVER_VALUE_RESPONSE_t *pQuerySwORValueRsp);
void ApsSetClkSwAndOverValueResp(CLOCK_MAINTAIN_RSP_t *pSetSwAndOverValueResp);


#endif

