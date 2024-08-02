#ifndef __APP_DATA_FREEZE_CCO_H__
#define __APP_DATA_FREEZE_CCO_H__
#include "ZCsystem.h"
#include "ProtocolProc.h"
#include "app_clock_sync_cco.h"
#include "app_base_multitrans.h"

//#define CURVE_GATHER 

//#ifdef CURVE_GATHER

typedef struct{

    U8       Status;
}__PACKED MODULE_TIMESYNC_CONFIRM_t;




typedef struct{

	U32      PacketSeq;
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
    U8       DataLen;
    U8       Data[0];	 //配置信息
}__PACKED CURVE_PROFILE_CFG_REQ_t;


typedef struct{

    U32      PacketSeq;
	U8       Result;
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
}__PACKED CURVE_PROFILE_CFG_CFM_t;

typedef struct{
    U16   ProtocolVer                :6;
    U16   HeaderLen                  :6;
    U16   Reserve1                   :4;
    U32   PacketSeq;
	U8    SrcMacAddr[6];
    U8    DstMacAddr[6];
    U16   Reserve2                   :4;
	U16   DataLength                 :12;
    U8    Data[0]                       ;
}__PACKED CURVE_PROFILE_CFG_DOWN_HEADER_t;


typedef struct{
    U16   ProtocolVer                :6;
    U16   HeaderLen                  :6;
    U16   Reserve1                   :4;
    U32   PacketSeq;
	U8    SrcMacAddr[6];
    U8    DstMacAddr[6];
    U8    Result;
	U16   Reserve2;
}__PACKED CURVE_PROFILE_CFG_UP_HEADER_t;
typedef enum
{
    e_CURVE_SUB_IDLE = 0,
    e_CURVE_SUB_USING,
    e_CURVE_SUB_FULL,
    
    
} CURVEP_SAVE_STATE_e;

#if defined(STATIC_MASTER)


typedef enum
{
    e_CURVE_GATHER_IDLE = 0,
    e_CURVE_CFG_SET_ALL,
    e_CURVE_CFG_SET_RECORD,
    e_CURVE_CFG_SET_RENEW,
    e_CURVE_CFG_SET_RESET,
    
} CURVEP_GATHER_STATE_e;


/*******************曲线存储数据项配置信息（05F103）*******************/

#define    MAX_DATA_LEN   206		//40*5+6
#define    MAX_DATA_COUNT   20		


typedef struct{
U8    UseFlag;
U8    ReadmeterCycle;
U8    Prtcl645;
U16   Item645SubLen;
U8    Item645Sub[MAX_DATA_LEN];
U8    Prtcl698;
U16   Item698SubLen;
U8    Item698Sub[MAX_DATA_LEN];
} __PACKED CurveGatherProfileSub_t;
extern CurveGatherProfileSub_t CcoCurveGatherProfile;

typedef struct{
    BroadcastCycleCfg_t BroadcastCycleCfg;
    CurveGatherProfileSub_t CcoCurveGatherProfile;
	U8 	  LiveLineZeroLineEnabled;
    U8    Res;
    U16   Crc16;
} __PACKED CCOCurveGatherInfo_t;

extern CCOCurveGatherInfo_t CcoCurveGatherInfo;
extern ostimer_t *SetSlaveCurveGatherTimer;


extern uint8_t CCOCurveCfgFlag;
extern U8 CurveGatherState;
extern BroadcastCycleCfg_t BroadcastCycleCfg;
extern CurveGatherProfileSub_t CcoCurveGatherProfile;
extern multi_trans_header_t  CURVE_PROFILE_CFG_LIST;
extern ostimer_t *curve_profile_cfg_guard_timer;
extern ostimer_t *curve_proflie_cfg_timer;
extern int8_t MODULE_CURVE_CFG_LIST_NUM;

typedef void (* app_assoc_renew_one_fn)();
extern app_assoc_renew_one_fn  app_assoc_renew_one_hook;

typedef void (*curve_profile_cfg_cfm_fn)(CURVE_PROFILE_CFG_CFM_t  *pCurveProfileCfgCfm);
extern curve_profile_cfg_cfm_fn  cco_curve_profile_cfg_cfm_hook;

U8 GetSTACurveGatherCfgbySeq(U16 Index);
void slave_curve_cfg_renew(work_t *work);
void SetSlaveCurveGather(work_t *work);

//void SetSlaveModuleTimeSyncModify(uint32_t first);
void SetSlaveCurveGatherTimerModify(uint32_t first,void *arg);

U8 PLC_Get_Sta_Curve_Profile_Cfg_By_Protocol(U8 Protocol,U8 *ItemSub, U16 *ItemSubLen);
U8 UART_Get_Sta_Curve_Profile_Cfg_By_Protocol(U8 Protocol,U8 *ItemSub, U16 *ItemSubLen);

void CurveProfileCfgCfmProc(CURVE_PROFILE_CFG_CFM_t  * pCurveProfileCfgCfm);
void app_assoc_renew_one_proc();

U8 cco_read_curve_cfg(U8 *CCOCurveGatherCfg);
void cco_save_curve_cfg(void);

void cco_curve_init(void);
U8 SaveCurveCfg(U8 *macaddr , U8* info);


#endif
//#endif
void CurveProfileCfgProc(work_t *work);

#if defined(STATIC_MASTER)
//void CurveProfileCfgProc(work_t *work);
void clearStaCurveStatus();
void SetSlaveCurveGatherTimerCB(struct ostimer_s *ostimer, void *arg);
extern void CnttActCncrntF1F100ReadMeterReq(U8 localFrameSeq, U8 readMeterNum, U8 *pMeterAddr, U8 *pCnmAddr,U16 Index, U8 protoType, 
                                                      U16 frameLen, U8 *pFrameUnit,U8 BroadcastFlag, MESG_PORT_e port);

#endif

#endif
