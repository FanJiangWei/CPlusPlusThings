#ifndef DATALINKDEBUG_H
#define DATALINKDEBUG_H

#include "types.h"
#include "Beacon.h"
#include "DataLinkGlobal.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "ZCsystem.h"
#include "app_common.h"

#if defined(STATIC_MASTER)
#define DATALINKDEBUG_SW 1
#else

#if defined(RISCV)
#define DATALINKDEBUG_SW 0
#else
#define DATALINKDEBUG_SW 1

#endif
#endif

extern	U32	RecordNumber;

#define FLOWNUM 30
typedef struct
{
    U16  MsduSeq[MAXNUMMEM];
    U8  starAODV[MAXNUMMEM];
    U8  mode[MAXNUMMEM];
    U16 DstTei[MAXNUMMEM];
    U16 NextTei[MAXNUMMEM];
}__PACKED RECORDMETER;


#define MMSGREACORDNUM 1 //����ʹ�ã��Ժ����ɾ��
typedef struct
{
    U32 PeriodCount;
    U8  Cmd;
    U16 SrcAddr;
    U16 DstAddr;
    U16 NextAddr;
    U8	Type; //��������е���ʲô��ɫ��Ŀ�꣬�����м��
    ROUTE_TABLE_RECD_t    RoutingTable[27];
}__PACKED MMSG_RECORD_t;

//extern MMSG_RECORD_t MmsgReacord[MMSGREACORDNUM];

/*
//ͳ����Ϣ

���20���ű����ڵ�ͳ����Ϣ
���һ��·�����ڵ�ͳ����Ϣ
�ϵ�����ʷͳ����Ϣ
*/
typedef struct
{
    U32    SendTime;        //unit:s
    U32    RecvTime;        //unit:s
    U32    SendNum;         //unit:cnt
    U32    RecvNum;         //unit:cnt
    U32    SendReCnt;       //unit:cnt
    U32    RecvReCnt;       //unit:cnt
    U32    SoltCnt;         //unit:cnt
    U32    CsmaTime;        //unit:ms
    U32    MaySendTime;     //unit:ms
    
}__PACKED LINKMODESET_INFO_t;



#if defined(ZC3951CCO) || defined(ZC3750STA)
#define AREA_NOTIFY_COUNT 30
#elif defined(ZC3750CJQ2)
#define AREA_NOTIFY_COUNT 30
#endif


typedef enum
{
	MSDU_REQ=1,

	UPDATE_CB,
	CLEAN_SOLT,
	//LK_LAYERDEAL,
	MAC_CFM,
	CHAN_BAND,
	NETKS,
	GATHER_IND,
	CENBEACON,
	PHASE_REQ,
	LEAVEL_REQ,
	SEND_DIS,
	PROC_BEACON,
    PROC_SOF,
	PROC_COORD,
	PROC_SACK,
	ERR_SOF,
	NOME_SOF,
	REAPEAT_SOF,
	APS_SOF,
	NETMMG,

	NXT_BPTS=60,
	MPDUIND,
	SEND_SUC,
	SEND_HTBT,
    CHAN_LINKMODE,

	STEP_1,
	STEP_2,
	STEP_3,
	STEP_4,
	STEP_5,
	STEP_6,
	STEP_7,
	STEP_8,
	STEP_9,
	STEP_10,
    STEP_11,

	STEP_12,
	STEP_13,
	STEP_14,
    STEP_15,
	STEP_16,
	STEP_17,
	STEP_18,
	STEP_19,

	

}dl_mg_type_e;

typedef enum
{
	PRE_PRINT=1,
	SENDTIME_REQ,
	CCO_EVTSEND,
	CCO_CLBIT,
	EVT_PROC,
	STA_CLBITPROC,
	EX_VER_PROC,
	JUDGE_PROC,
	SB_TASK_START,
	SB_TASK_COLLECT,
	SB_TASK_GET,
	SB_TASK_INFORM,
	MID_REQ,
	IN_MULTI_LIST,
	UP_MULTI_LIST,
	MULTI_GUARD,
	PHASE_RUN,
	POWEROFF_SEND,
	POWERON_SEND,
	POWERON_REPORT,
	POWEROFF_REPORT,
	ROUTER_RM,
	STA_REG,
	UPGRDCTL,
	DALKCFM,
	MSDUIND,
	TRIG2APP,
	PHASECMF,
	VTCOL,
	FRTCOL,
	ZAGATH,
	SREGCFM,
	CJQ2UP,
	ENREADM,
	UPREADM,
	ADDLF,
	DLELF,
	UPSCIF,
    IRSCIF,
    RPTWAIT_SEND,
    RTC_SYSC,
    CYCLE_DEL,
    STA_CURVE_GATHER,
    SET_STA_CURVE_CFG,
    CLOCK_OS_EVENT,
    READ_METER_CLOCK,
    CLOCK_MANANGE,
    CJQEVTRETCLKOVER,
    CURVEREQCLOCK,
    SETMODETIME,
    LOWPOWER_START,
    LOWPOWER_PROC,
	OFF_GRID,
	WAIT_06F4,
	EVENT_REPORT_LIST,
	NODECHANGE2APP,
	POWEROFF_ADDR_INFO,
	CALIBRATION_REPORT,
	AREA_RESULT_TIMEOUT,
	AREA_TASK_TIMEOUT,
	SEND_03F10,
	SEND_05F3
}app_mg_type_e;



typedef enum
{
    e_SnrCacl=0,
    e_NtbCacl,
    e_Clnnib

}AreaNotifyActive_e;

typedef struct
{
    U8	NWkSeq;
    U16 SNID;		  //��16λ
    U16 PeriodCount;  //��16λ
    U8	State;       //�Ƿ�����
    U8	Active;       //SNR������NTB������cleanNNIB
    U8  Reason;       //����������REQ_REASON
    U16	BeterSNID;    //����������SNID
    S8  SrnCCOPoint;     //CCO����
    S8  NtbCCOPoint;     //CCO����
    U8  Sameration;   //�������ƶ�
    U8	Result;       //��� �����ߣ�����
}__PACKED AREA_NOTIFY_RECORD_t;

typedef struct
{

    U16  				 CurrentEntry;
    AREA_NOTIFY_RECORD_t AreaNotifyReacod[AREA_NOTIFY_COUNT];

}__PACKED AREA_NOTIFY_BUFF;

extern AREA_NOTIFY_BUFF AreaNotifyBuff;

#if defined(STATIC_MASTER)
#define MaxRecord     1000
typedef struct
{
    U32 PeriodCount;
    U8  Cmd;
    U16 SrcAddr;
    U16 OldProxy;
    U8  OldProxyType;
    U16 NewProxy;
    U8  NewProxyType;
    U8	Newdepth;
    U8  NewType;
    U8	Reason;
    U16 OnlineNum;
    U16 OfflineNum;
	U16 allnum;
}__PACKED MMS_DEEPTH_t;


//extern MMS_DEEPTH_t  MmsgDeepth[MaxRecord];
#endif

extern mem_record_list_t *pDealNow;
extern mem_record_list_t *pAppDealNow;

#if DATALINKDEBUG_SW
#define MAXDEALNUM 20
typedef struct record_dealtime_s {

	U32   type;
    U32   tick;
    U32   maxdealtime;
    U32   lastdealtime;

}record_dealtime_t;

record_dealtime_t recordDealtime[MAXDEALNUM];



typedef struct record_mg_flow_s {
	U8	  taskid;
	U32   type;
    U32   posttick;
    U32   recvtick;
    U32   dealtick;
//	U32   dealtime;
	const char  *s;
}record_mg_flow_t;

record_mg_flow_t record_mg_flow[FLOWNUM];

extern U8   flow_num;
#endif

void show_mg_record(tty_t *term);
void mg_process_record(work_t *work,U8 taskid);

U8 mg_update_type(U8 type, U8 index);
void mg_process_record_init(work_t *work);
void mg_process_record_in(work_t *work,U8 taskid, U8 type);
void showdealtime(tty_t *term);



void recordArrayInit();

//ͳ�Ƽ�¼���г���ת���Լ����лظ�
void reacordmeter(U16 MsduSeq,U8 starAODV ,U8 mode,U16 DstTei,U16 NextTei);

//ÿ���ڵ��¼���������н��յ������������Ϣ���Լ�·�ɱ仯���̣�STAʹ��
void ReacordMmsgData(U8 Cmd,U16 SrcAddr ,U16 DstAddr , U16 NextAddr ,U8 Type);

//��¼STA��״̬�仯ʱ��ԭ��
void RecordStaStstus(U8 Active,U8  Reason,U16	BeterSNID,U8  Sameration,U8	Result);

//��¼������ÿһ�����˵ı�����
void ReacordDeepth(U16 Cmd,U16 SrcTei,U16 OldProxy,U16 NewProxy,U8 Newdepth,U8 NewType,U8 Reason);

void printNetDeviceAndRecord();

void RecordAreaNotifyBuff(U8 Active,U8  Reason,U16	BeterSNID,U8  Sameration,U8	Result);
void showrecordmeter(tty_t *term);
void printNetAQ(tty_t *term);
void printSwitch(U8 active);
void adddealtime(U16 tei,      U32 bct ,U32 dealtime);

void periodprinfo(work_t *work);

void printNetRouteInfo();
void printNeighborDiscoveryTableEntry();
void printDevListInfo();
void printNetInfo();



#endif // DETALINKDEBUG_H
