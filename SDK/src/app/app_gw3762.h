#ifndef _APP_GW3762_H_
#define _APP_GW3762_H_
#include "types.h"
#include "app_common.h"
#include "ProtocolProc.h"
#include "app_base_serial_cache_queue.h"


#if defined(STATIC_MASTER)

#define GW376209_PROTO_EN						1
#define GW376213_PROTO_EN						1
#define APP_RELAY_LEVEL_MAX					    15		/* 最大中继数 */
#define RG_WIN_SIZE                             1
#define RETRY_TIMES                             2
#define METER_NUM_SIZE                          13
#define ModuleIDLen                             11
#define BROADCAST_CONRM_NUM                     1    //需要防止翻转，设置为2，实际下发2条

/******************************** DLT645部分********************************/
#define APP_DLT645_HEAD_LEN					    1
#define APP_DLT645_ADDR_LEN					    6
#define APP_DLT645_CTRL_LEN					    1
#define APP_DLT645_LEN_LEN						1
#define APP_DLT645_BASE_LEN					    12

#define APP_DLT645_HEAD1_POS					0
#define APP_DLT645_ADDR_POS					    1
#define APP_DLT645_HEAD2_POS					7
#define APP_DLT645_CTRL_POS					    8
#define APP_DLT645_LEN_POS						9
#define APP_DLT645_DATA_POS					    10

#define APP_DLT645_HEAD_VALUE					0x68
#define APP_DLT645_END_VALUE					0x16

#define APP_DLT645_DATA_UNIT_LEN				256
/******************************* GW3762部分*******************************/
#define APP_GW3762_SPEED_NUM					1			/* AFN03F5/AFN03F10-波特率数量 */
#define APP_GW3762_UPDATE_WAITTIME				30			/* AFN03F10-升级操作等待时间。终端发送完最后一个升级数据包后，需要等待模块完成升级的时间长度*/
#define APP_GW3762_UPINFO_CHANFEAT				0			/* 电能表通道特征 */
#define APP_GW3762_UPINFO_CHANFLAG				0			/* 信道标识 */
#define APP_GW3762_UPINFO_CMDQUALITY			0			/* 末级命令信号特征 */
#define APP_GW3762_UPINFO_EVENTFLAG			    0			/* 事件标志 */
#define APP_GW3762_UPINFO_MODULEFLAG			0			/* 通信模块标识 */
#define APP_GW3762_UPINFO_PHASEFLAG			    0			/* 实测相线标识 */
#define APP_GW3762_UPINFO_RELAYLEVEL			0			/* 中继级别 */
#define APP_GW3762_UPINFO_REPLYQUALITY			0			/* 末级应答信号品质 */
#define APP_GW3762_UPINFO_ROUTERFLAG			0			/* 路由标识 */

#define BC_TIMEOUT								(0)	/* 广播校时定时 */

#define	APP_GW3762_METER_MAX		            MAX_WHITELIST_NUM //1015	/* AFN03F10/AFN10F1-一个小区的最大表数*/
#define APP_GW3762_FILETRANS_PACK_MAX		    1024			//AFN03F10-文件传输的最大单个数据包长度。AFN15-F1中，最大分包大小
#define APP_GW3762_BROADCAST_TIMEOUT			1800
#define APP_GW3762_CHAN_NUM						1			/* AFN03F5-信道数量 */

#define APP_GW3762_HEAD_LEN					    1
#define APP_GW3762_LEN_LEN					    2
#define APP_GW3762_CTRL_LEN					    1
#define APP_GW3762_INFO_LEN					    6
#define APP_GW3762_ADDR_LEN					    6
#define APP_GW3762_AFN_LEN					    1
#define APP_GW3762_FN_LEN					    2
#define APP_GW3762_BASE_LEN					    15
#define APP_GW3762_HEAD_VALUE				    0x68
#define APP_GW3762_END_VALUE				    0x16
#define APP_GW3762_DATA_UNIT_LEN			    2048
#define APP_GW3762_FRAME_MAX_LEN                3000
#define APP_GW3762_SLAVE_SUB_MAX			    5		/* AFN13F1-从节点附属节点最大数量 */

/* 信息域相关 */
#define APP_GW3762_UPINFO_ROUTERFLAG		    0		/* 路由标识 */
#define APP_GW3762_UPINFO_MODULEFLAG		    0		/* 通信模块标识 */
#define APP_GW3762_UPINFO_RELAYLEVEL		    0		/* 中继级别 */
#define APP_GW3762_UPINFO_CHANFLAG			    0		/* 信道标识 */
#define APP_GW3762_UPINFO_PHASEFLAG			    0		/* 实测相线标识 */
#define APP_GW3762_UPINFO_CHANFEAT			    0		/* 电能表通道特征 */
#define APP_GW3762_UPINFO_CMDQUALITY		    0		/* 末级命令信号特征 */
#define APP_GW3762_UPINFO_REPLYQUALITY		    0		/* 末级应答信号品质 */
#define APP_GW3762_UPINFO_EVENTFLAG			    0		/* 事件标志 */

#define JZQSETBAUD                              115
#define JS_BAUD115200                           115200
#define JS_BAUD9600                             9600

#define RETRANREGMAXNUM                         3
#define RETRANREGLOCKNUM                        5

extern U8 APP_GW3762_TRANS_MODE;
//  AFN10F4   Run status word define
//#define    EVENT_REPORT_FLAG                0x01
//#define    WORKING_FLAG                     0x01
//#define    ROUTE_STUDY_FINISH_FLAG     		0x01
// AFN10F4  work switch define
//#define    WORK_STATE_FLAG                  0x01
//#define    REGISTER_PERMIT_FLAG          	0x01
//#define    ZONE_DISTINGUISH_FLAG       		0x01
//#define    CONCURRENT_SWTICH_FLAG     	    0X00
//#define    REGISTER_RUN_FLAG          	    0x01
//#define    CARRIER_UPGRADE_FLAG       	    0x02
//#define    OTHER_SWITCH_FLAG         	    0x03


/******************************* 类型定义 *******************************/

//////////////////////////////////////////////////////////////////////////////
typedef struct
{
    U8 TransMode	: 6;					/* 通信方式 */
    U8 StartFlag	: 1;					/* 启动标志 */
    U8 TransDir		: 1;					/* 传输方向 */
} __PACKED AppGw3762CtrlField_t;

typedef struct
{
    U8 RouterFlag	: 1;					/* 路由标识 */
    U8 SubNodeFlag	: 1;					/* 附属节点标识 */
    U8 ModuleFlag	: 1;					/* 通信模块标识 */
    U8 ConflictCheck	: 1;				/* 冲突检测 */
    U8 RelayLevel	: 4;					/* 中继级别 */
    U8 ChanFlag		: 4;					/* 信道标识 */
    U8 CorrectFlag	: 4;					/* 纠错编码标识 */
    U8 CalcDataNum;						    /* 预计应答字节数 */
    U16 TransRate	: 15;				    /* 通信速率 */
    U16 RateFlag	: 1;					/* 速率单位标识 */
    U8 FrameNum;						    /* 报文序列号 */
} __PACKED AppGw3762DownInfoField_t;

typedef struct
{
    U8 RouterFlag	: 1;					/* 路由标识 */
    U8 Reserve1		: 1;					/* 预留 */
    U8 ModuleFlag	: 1;					/* 通信模块标识 */
    U8 Reserve2		: 1;					/* 预留 */
    U8 RelayLevel	: 4;					/* 中继级别 */
    U8 ChanFlag		: 4;					/* 信道标识 */
    U8 Reserve3		: 4;					/* 预留 */
    U8 PhaseFlag	: 4;					/* 实测相线标识 */
    U8 ChanFeat		: 4;					/* 电能表通道特征,需查看勘误表 */
    U8 CmdQuality	: 4;					/* 末级命令信号品质 */
    U8 ReplyQuality	: 4;					/* 末级应答信号品质 */
    U8 EventFlag	: 1;					/* 事件标识 */
    U8 LineErr		: 1;					/* 线路异常 */
	U8 AreaFlag		: 1;					/* 台区标识 */
	U8 res			: 5;					/* 保留 */
	
    U8 FrameNum;						    /* 报文序列号 */
} __PACKED AppGw3762UpInfoField_t;

typedef struct
{
    U8 SrcAddr[MAC_ADDR_LEN];				/* 源地址 */
    U8 DestAddr[MAC_ADDR_LEN];				/* 目的地址 */
    U8 RelayAddr[APP_RELAY_LEVEL_MAX][MAC_ADDR_LEN];	/* 中继地址 */
} __PACKED AppGw3762AddrField_t;

typedef struct
{
    union
    {
        U8 CtrlFieldAll;
        AppGw3762CtrlField_t CtrlField;
    };
    union
    {
        U8 InfoFieldAll[APP_GW3762_INFO_LEN];
        AppGw3762DownInfoField_t DownInfoField;
        AppGw3762UpInfoField_t UpInfoField;
    };
    AppGw3762AddrField_t AddrField;
    U8 AddrFieldNum;
    U8 Afn;
    U16 Fn;
    U16 DataUnitLen;
    U8 DataUnit[APP_GW3762_DATA_UNIT_LEN];
} __PACKED APPGW3762DATA_t;

typedef enum
{
    APP_GW3762_DOWN_DIR,
    APP_GW3762_UP_DIR,
} AppGw3762Dir_e;

typedef enum
{
    APP_GW3762_SLAVE_PRM,
    APP_GW3762_MASTER_PRM,
} AppGw3762Prm_e;

typedef enum
{
    APP_GW3762_AFN00 = 0x00,
    APP_GW3762_AFN01,
    APP_GW3762_AFN02,
    APP_GW3762_AFN03,
    APP_GW3762_AFN04,
    APP_GW3762_AFN05,
    APP_GW3762_AFN06,
    APP_GW3762_AFN10 = 0x10,
    APP_GW3762_AFN11,
    APP_GW3762_AFN12,
    APP_GW3762_AFN13,
    APP_GW3762_AFN14,
    APP_GW3762_AFN15,
    APP_GW3762_AFN20 = 0x20,
    APP_GW3762_AFNF0 = 0xF0,
    APP_GW3762_AFNF1,
} AppGw3762Afn_e;

typedef enum
{
    APP_GW3762_F1 = 1,
    APP_GW3762_F2,
    APP_GW3762_F3,
    APP_GW3762_F4,
    APP_GW3762_F5,
    APP_GW3762_F6,
    APP_GW3762_F7,
    APP_GW3762_F8,
    APP_GW3762_F9,
    APP_GW3762_F10,
    APP_GW3762_F11,
    APP_GW3762_F12,
    APP_GW3762_F13,
    APP_GW3762_F14,
    APP_GW3762_F15,
    APP_GW3762_F16 = 16,
    APP_GW3762_F17 = 17,
    APP_GW3762_F18 = 18,
    APP_GW3762_F20 = 20,
    APP_GW3762_F21 = 21,
    APP_GW3762_F31 = 31,
    APP_GW3762_F32 = 32,
    APP_GW3762_F33 = 33,
    APP_GW3762_F40 = 40,
    APP_GW3762_F56 = 56,//安徽查询CCO串口波特率
    APP_GW3762_F90 = 90, //查询时钟管理开关和超差阈值
    APP_GW3762_F92 = 92, 
    APP_GW3762_F93 = 93, //查询时钟维护周期
    APP_GW3762_F94 = 94,
    APP_GW3762_F100 = 100,
    APP_GW3762_F101 = 101,
    APP_GW3762_F102 = 102,
    APP_GW3762_F103 = 103,
    APP_GW3762_F104 = 104,
    APP_GW3762_F106 = 106,
    APP_GW3762_F111 = 111,
    APP_GW3762_F112 = 112,
    APP_GW3762_F130 = 130,
    APP_GW3762_F131 = 131,
    APP_GW3762_F200 = 200,
    APP_GW3762_F201 = 201,
    APP_GW3762_F225 = 225,
} AppGw3762Fn_e;

typedef enum
{
    e_HIGHEST_POWER,            //最高发射功率
    e_SECOND_HIGHEST_POWER,     //次高发射功率
    e_SECOND_LOWEST_POWER,      //次低发射功率
    e_LOWEST_POWER,             //最低发射功率
} APP_RF_POWER;

typedef enum
{
    APP_JZQ_TYPE_13,
    APP_JZQ_TYPE_09,
} APP_JZQ_TYPE_E;

/* AFN00F1 */
/* 命令状态 */
typedef enum
{
    APP_GW3762_N_CMDSTATE,
    APP_GW3762_Y_CMDSTATE,
} AppGw3762CmdState_e;

/* AFN00F1 */
/* 信道状态 */
typedef enum
{
    APP_GW3762_BUSY_CHANSTATE,
    APP_GW3762_IDLE_CHANSTATE,
} AppGw3762ChanState_e;

/* AFN00F2 */
/* 错误状态字 */
typedef enum
{
    APP_GW3762_TIMEOUT_ERRCODE,			    /* 通信超时 */
    APP_GW3762_DATAUNIT_ERRCODE,			/* 无效数据单元 */
    APP_GW3762_DATALEN_ERRCODE,			    /* 长度错误 */
    APP_GW3762_CHECKSUM_ERRCODE,			/* 校验错误 */
    APP_GW3762_INFOCLASS_ERRCODE,			/* 信息类不存在 */
    APP_GW3762_FORMAT_ERRCODE,			    /* 格式错误 */
    APP_GW3762_ADDRREPEAT_ERRCODE,		    /* 表号重复 */
    APP_GW3762_ADDRNONE_ERRCODE,			/* 表号不存在 */
    APP_GW3762_NOANSWER_ERRCODE,			/* 电表应用层无应答 */
#if (GW376213_PROTO_EN > 0)
    /* 9 */APP_GW3762_MASTERBUSY_ERRCODE,	/* 主节点忙 */
    /* 10 */APP_GW3762_NOSUPPORT_ERRCODE,	/* 主节点不支持此命令 */
    /* 11 */APP_GW3762_NOSLAVE_ERRCODE,		/* 从节点不应答 */
    /* 12 */APP_GW3762_NONWK_ERRCODE,		/* 从节点不在网内 */
#endif

    APP_GW3762_OUT_MAXCONCURRNUM_ERRCODE = 109,   /*	超出最大并发数*/
    APP_GW3762_OUT_MAX3762FRAMENUM_ERRCODE, /*超出376.2帧中最大电表协议报文条数*/
    APP_GW3762_METER_READING_ERRCODE,       /*此表正在抄读*/
    
} AppGw3762ErrCode_e;

/* AFN03F5 */
/* 周期抄表模式 */
typedef enum
{
    APP_GW3762_D_PERIODMETER,				/* 两种模式都支持 */
    APP_GW3762_C_PERIODMETER,				/* 支持集中器主导的周期抄表模式 */
    APP_GW3762_R_PERIODMETER,				/* 支持通信模块主导的周期抄表模式 */
} AppGw3762PeriodMeter_e;

/* AFN03F5 */
/* 主节点信道特征 */
typedef enum
{
    APP_GW3762_MW_CHANFEAT,				    /* 微功率无线 */
    APP_GW3762_SS_CHANFEAT,					/* 单相供电单相传输 */
    APP_GW3762_ST_CHANFEAT,					/* 单相供电三相传输 */
    APP_GW3762_TT_CHANFEAT,				    /* 三相供电三相传输 */
} AppGw3762ChanFeat_e;

/* AFN03F6 */
typedef enum
{
    APP_GW3762_N_MASTERDIST,
    APP_GW3762_Y_MASTERDIST,
} AppGw3762MasterDist_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_NC_CTRL_COMMMODE = 0,			/* 保留 */
    APP_GW3762_CENTER_NC_CTRL_COMMMODE = 1,		/* 集中式路由窄带载波通信 */
    APP_GW3762_PER_NC_CTRL_COMMMODE = 2,		/* 分布式路由窄带载波通信 */
    APP_GW3762_HPLC_CTRL_COMMMODE = 3,			/* HPLC宽带载波通信 */
    APP_GW3762_HPLC_HRF_CTRL_COMMMODE = 4,		/* HPLC+HRF双模通信 */
} AppGw3762CtrlMode_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_NC_COMMMODE = 1,			        /* 窄带电力线载波 */
    APP_GW3762_BC_COMMMODE = 2,				    /* 宽带电力线载波 */
    APP_GW3762_RF_COMMMODE = 3,				    /* 微功率无线 */
    APP_GW3762_HPLC_HRF_COMMMODE = 4,			/* HPLC+HRF双模通信 */
} AppGw3762CommMode_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_ROUTERMANAGE,			        /* 本地通信模块无路由管理功能 */
    APP_GW3762_Y_ROUTERMANAGE,			        /* 本地通信模块带有路由管理功能 */
} AppGw3762RouterManage_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_SLAVEINFO,					    /* 不需要下发从节点信息 */
    APP_GW3762_Y_SLAVEINFO,					    /* 需要下发从节点信息 */
} AppGw3762SlaveInfo_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_C_CYCLEMETER = 1,			/* 支持集中器主导的周期抄表模式 */
    APP_GW3762_R_CYCLEMETER,				/* 支持通信模块主导的周期抄表模式 */
    APP_GW3762_D_CYCLEMETER,				/* 两种模式都支持 */
} AppGw3762CycleMeter_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_TRANSDELAY,				/* 不支持向集中器提供传输延时参数 */
    APP_GW3762_Y_TRANSDELAY,				/* 支持向集中器提供传输延时参数 */
} AppGw3762TransDelay_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_R_FAILCHANGE = 1,			/* 表示通信模块自主切换待抄节点 */
    APP_GW3762_C_FAILCHANGE,				/* 表示集中器发起通知通信模块自主切换待抄节点 */
} AppGw3762FailChange_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_AFTER_BCCONFIRM,			    /* 表示广播命令在本地通信模块执行广播通信过程完毕后返回确认报文 */
    APP_GW3762_BEFORE_BCCONFIRM,			/* 表示广播命令在本地信道执行广播通信之前就返回确认报文 */
} AppGw3762BcConfirm_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_BCEXECUTE,				    /* 表示执行广播命令不需要信道标识 */
    APP_GW3762_Y_BCEXECUTE,					/* 表示执行广播命令要根据报文中的信道标识逐个来发送 */
} AppGw3762BcExecute_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_LOWVOL,					/* 表示未掉电 */
    APP_GW3762_Y_LOWVOL,					/* 表示掉电 */
} AppGw3762LowVol_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_BPS_RATEUNIT,				/* 表示bps */
    APP_GW3762_KBPS_RATEUNIT,				/* 表示kbps */
} AppGw3762RateUnit_e;

/* AFN05F10 */
typedef enum
{
    APP_GW3762_9600_BPS,				/* 表示9600bps */
    APP_GW3762_19200_BPS,				/* 表示19200kbps */
    APP_GW3762_38400_BPS,				/* 表示38400kbps */
    APP_GW3762_57600_BPS,				/* 表示57600kbps */
    APP_GW3762_115200_BPS,				/* 表示115200kbps */
} AppGw3762BaudRateUnit_e;

/* AFN06F3 */
/* 路由工作任务变动类型 */
typedef enum
{
    APP_GW3762_READ_TASKCHANGE = 1, //抄表
    APP_GW3762_SEARCH_TASKCHANGE,   //搜表
    APP_GW3762_AREA_TASKCHANGE,     //台区识别
} AppGw3762TaskChange_e;

/* AFN06F4 */
/* 从节点设备类型 */
typedef enum
{
    APP_GW3762_C_SLAVETYPE,			    /* 采集器 */
    APP_GW3762_M_SLAVETYPE,		        /* 电能表 */
    APP_GW3762_HPLC_MODULE,             /* HPLC通信单元*/
    APP_GW3762_NARROW_MODULE,           /*窄带载波通信单元*/
    APP_GW3762_WIRELESS_MODULE,         /*微功率无线通信单元*/
    APP_GW3762_DOUBLE_MODE,             /*双模通信单元*/
} AppGw3762SlaveType_e;

typedef enum
{
    APP_GW3762_S_ADDMETERRES,			/* 添加从节点成功 */
    APP_GW3762_MR_ADDMETERRES,			/* 电表重复 */
    APP_GW3762_MO_ADDMETERRES,			/* 超过最大电表数 */
    APP_GW3762_STDERR_ADDMETERRES,		/* 标准错误 */
    APP_GW3762_PROTOERR_ADDMETERRES,	/* 规约错误 */
} AppGw3762AddMeterRes_e;

/* AFN11F3 */
/* 工作状态,0-抄表,1-学习 */
typedef enum
{
    APP_GW3762_READ_WORKSTATE,
    APP_GW3762_STUDY_WORKSTATE,
} AppGw3762WorkState_e;

/*AFN15F1  FileTrans*/
typedef enum
{
    APP_GW3762_CLEAR_DOWNLOAD_FILE,
    APP_GW3762_LOCAL_MODULE_UPDATE = 0x03,
    APP_GW3762_SLAVE_CJQ_UPGRADE_START = 0x05,
    APP_GW3762_LOCAL_REMOTE_UPDATE = 0x07,
    APP_GW3762_SLAVE_MODULE_UPDATE = 0x08,
    APP_GW3762_SLAVE_UPGRADE_STOP   = 0x09,
    APP_GW3762_SLAVE_UPGRADE_START = 0x0A,
}AppGw3762FileTransId_e;

/*AFN10F40   read module ID*/
typedef enum
{
    APP_GW3762_RD_CTRL = 1,
    APP_GW3762_CCO_MODULE,
    APP_GW3762_METER_MODULE,
    APP_GW3762_REPEATER_MODULE,
    APP_GW3762_CJQ_II,
    APP_GW3762_CJQ_I,
}AppGw3762DeviceType_e;

typedef enum
{
    e_3762ZONE_DISTINGUISH_STOP = 0,
    e_3762ZONE_DISTINGUISH_STARTUP0203H = 1,    
    e_3762ZONE_DISTINGUISH_STARTUP010203H = 2,
}ZONE_DISTINGUISH_STATUS_e;

/*AFN03F12 10F7   read module ID*/
typedef enum
{
    COMBINATION_TEPEY ,     //组合格式
    BCD_TYPE,               //BCD
    BIN_TYPE,               //BIN
    ASCII_TYPE,             //ASCII
}ModuleIDFormat_e;

/*AFN03F12 10F7   read module ID*/
typedef enum
{
    MOUDLTID_NO_UP = 1,			/* 模块I未更新 */
    MOUDLTID_RENEW = 0,			/* 模块ID已更新 */
} ModuleIDRenewSate_e;

/*AFNF0F11 */
typedef enum
{
    UOGRADE_TASK = 1,			/* 升级*/
    REGISTER_TASK,			    /* 注册 */
    BITMAP_OFF_TASK,			/* 位图模式停电 */
    ADDR_OFF_TASK,			    /* 地址模式停电 */
} MesgType_e;

/******************************* 并发抄表支持的广播策略 *******************************/
//typedef enum
//{
//    Generally = 0,					/* 常规单播抄表，最后一次支持广播 */
//    AllBroadcast ,					/* 全部广播抄表 */
//} BroadCastConRMType_e;

/* AFN06F1 */
typedef struct
{
    U8 Addr[MAC_ADDR_LEN];
    AppProtoType_e ProtoType;
    U16 Index;
} __PACKED AppGw3762SlaveInfo_t;

/* AFN06F4 */
typedef struct
{
    U8 Addr[MAC_ADDR_LEN];
    U8 ProtoType;
} __PACKED AppGw3762SubInfo_t;

/* AFN03F5 */
/* 状态字 */
typedef struct
{
    U8	ucSpeedNum			: 4;			/* 速率数量n */
    U8	ucChanFeature		: 2;			/* 主节点信道特征 */
    U8	ucCycleReadMode		: 2;			/* 周期抄表模式 */
    U8	ucChanNum			: 4;			/* 信道数量 */
    U8	ucReserve			: 4;			/* 预留 */
    U16	usBaudSpeed			: 15;			/* 通讯速率*/
    U16	usBaudUnit			: 1;			/* 速率标识*/
} __PACKED APPGW3762AFN03F5_t;

/* AFN03F10 */
typedef struct
{
    U8 ucCommMode			: 4;
    U8 ucRouterManage		: 1;
    U8 ucSlaveInfo			: 1;
    U8 ucCycleMeter			: 2;

    U8 ucRouterTransDelay	: 1;
    U8 ucSlaveTransDelay	: 1;
    U8 ucBroadTransDelay	: 1;
    U8 ucFailChange			: 2;
    U8 ucBroadConfirm		: 1;
    U8 ucBroadExecute		: 2;

    U8 ucChannelNum			: 5;
    U8 ucALowVoltage		: 1;
    U8 ucBLowVoltage		: 1;
    U8 ucCLowVoltage		: 1;

    U8 ucRateNum			: 4;
    U8 ucMinuteCollect      : 1;//分钟级采集
    U8 ucReserve1           : 3;

    U8 ucReserve2;
    U8 ucReserve3;

    U8	ucSlaveReadTimeout;
    U16	usBroadCastTimeout;
    U16	usMsgMaxLen;
    U16	usForwardMsgMaxLen;
    U8	ucUpdateTime;
    U8	ucMainAddr[6];
    U16	usNodeMaxNum;
    U16	usCurNodeNum;
    U8	ucProtoDate[3];
    U8	ucProtoRecordDate[3];
    DCR_MANUFACTURER_t		ManuFactor_t;
    U16 BaudSpeed			: 15;			/* 通讯速率*/
    U16 BaudUnit			: 1;			/* 速率标识*/
} __PACKED APPGW3762AFN03F10_t;

/* AFN03/05F17 */
/* 状态字 */
typedef struct
{
    U8	ucRfOption			;			/* 无线调制方式 */
    U8	ucRfChannel 		;			/* 无线信道编号 */
    U8	ucRfConsultEn		;			/* 信道协商使能 */
    
} __PACKED APPGW3762AFN03F17_t;

typedef struct{
    U8    RecordLoc          : 1;
	U8    CourtsState          : 2;
	U8    Phase          : 3;
	U8    LNerr          : 1;
	U8    reserve          : 1;
    
}__PACKED APPGW3762AFN03F101_t;

enum{
	e_ACSAMPING_TPEY_645=1,
	e_ACSAMPING_TPEY_698=2,
};

typedef struct{
    U8    RunStatusRouter		:1;		//路由完成标志	1：路由学习完成；0：未完成
	U8    RunStatusWorking		:1;		//工作标志     1：正在工作；0：停止工作
	U8    RunStatusEvent		:1;		//上报事件标志 1为有从节点事件上报
	U8    RunStatusRes			:5;		// 纠错编码
	
    U16   SlaveNodeCount;				//从节点总数量
    U16   ReadedNodeCount;				//已抄从节点数量
    U16   RelayReadedCount;				//中继抄到从节点数量

	
    U8    WorkSwitchWork			:1; //工作状态1学习0抄表
	U8    WorkSwitchRegister		:1; //注册允许状态1允许0不允许
	U8    WorkSwitchEvent			:1; // 事件上报状态1允许0不允许
	U8    WorkSwitchZone			:1; //台区识别使能标志 1允许0不允许
	U8    BJZoneFinish				:1; //北京使用
	U8    WorkSwitchRes				:1; //
	U8    WorkSwitchStates			:2; //当前状态 00抄表,01搜表,10升级,11台区识别
	
    U16   CnmRate;						// 载波通信速率
    U8    RelayLevel[3];				// 1、2、3相中继级别
    U8    WorkStep[3];					// 1、2、3相工作步骤
}__PACKED APPGW3762AFN10F4_t;

typedef struct{
    
    U8    DeviceType            : 4;            //设备类型
    U8    Res1                  : 1;            //JS:分钟采集标志
    U8    Res2                  : 2;            //保留
    U8    Renewal               : 1;            //更新信息
	U8    VendorCode[2];
	U8    ModeIDLen;
	U8    ModeIDType;
	U8    ModeID[11];
}__PACKED APPGW3762AFN10F7_t;

typedef struct{
    
	U16   NodeTEI           : 12;
    U16   ModuleType        :  4;
	U16   ParentTEI         : 12;
    U16   Res               :  4;
    U8    NodeDepth     	:  4; 	//级数
	U8    NodeType      	:  3; 	//角色类型
	U8    LinkType      	:  1; 	//通道类型
}__PACKED APPGW3762AFN10F20_t;

typedef struct{
    
	U16   NodeTEI;
	U16   ParentTEI;
    U8    NodeDepth     	:  4; 	//级数
	U8    NodeType      	:  4; 	//角色类型
}__PACKED APPGW3762AFN10F21_t;

typedef struct{
    U8    Phase1          : 1;
	U8    Phase2          : 1;
	U8    Phase3          : 1;
	U8    MeterType       : 1;
	U8    LNerr           : 1;
	U8    order1          : 1;
	U8    order2          : 1;
	U8    order3          : 1;
	U8    reserve1         ;
}__PACKED APPGW3762AFN10F31_t;

typedef struct{
	U8    VersionNum[2];
	U8    Day;
    U8    Month;
	U8    Year;
    U8    VendorCode[2];
    U8    ChipCode[2];
}__PACKED APPGW3762AFN10F104_t;

typedef struct{
    
    U8    DevType;
	U8    ChipID[24];
    U8    VersionNum[2];
}__PACKED APPGW3762AFN10F112_t;

typedef struct
{
    U8 ValidFlag;
    U8 Afn02Flag; 
    U8 ProtoType;											/* 通信协议类型 */

#if (GW376213_PROTO_EN > 0)
    U8 DelayFlag;											/* 通信延时相关性标志 */
#endif
    U8 SlaveSubNum;											/* 从节点附属节点数量 */
    U8 SlaveSubAddr[APP_GW3762_SLAVE_SUB_MAX * MAC_ADDR_LEN];	/* 从节点附属节点地址 */

    U16 FrameLen;											/* 报文长度 */
    U8 FrameUnit[APP_GW3762_DATA_UNIT_LEN];				    /* 报文内容 */
    U8 Addr[MAC_ADDR_LEN];
    U8 CnmAddr[MAC_ADDR_LEN];
	U8 FrameSeq;
    //S8 ReTransmitCnt;
} __PACKED AppGw3762Afn13F1State_t;

typedef struct
{
    U8  Valid;
    U8 ProtoType;											/* 通信协议类型 */

	U8 Framenum;
    U16 FrameLen;											/* 报文长度 */
    U8   FrameUnit[APP_GW3762_DATA_UNIT_LEN];				/* 报文内容 */
    U8   Addr[MAC_ADDR_LEN];
    U8 CnmAddr[MAC_ADDR_LEN];
	U8 DltNum;
	U16 DatagramSeq;
    S8  DealySecond;
    S8 ReTransmitCnt;
} __PACKED AppGw3762AfnF1F1State_t;

typedef struct
{
    U8 ValidFlag;
    U8 ReadState;											/* 抄读标志 */
    U8 CmnRlyFlag;
    U8 ProtoType;

    U16 FrameLen;											/* 报文长度 */
    U8 FrameUnit[APP_GW3762_DATA_UNIT_LEN];				    /* 报文内容 */

    U8 SlaveSubNum;											/* 从节点附属节点数量 */
    U8 SlaveSubAddr[APP_GW3762_SLAVE_SUB_MAX * MAC_ADDR_LEN];	/* 从节点附属节点地址 */

    U8 Addr[MAC_ADDR_LEN];
    U8 CnmAddr[MAC_ADDR_LEN];
	U8 FrameSeq;
    //S8 ReTransmitCnt;
} __PACKED AppGw3762Afn14F1State_t;

typedef struct
{
    U16    CrnMeterIndex;
    U8      Addr[MAC_ADDR_LEN];
    U8      ProtoType;
    U16    CommuTime;

    U16     FrameLen;										/* 报文长度 */
    U8     FrameUnit[APP_GW3762_DATA_UNIT_LEN];				/* 报文内容 */

    S8     ReTransmitCnt;
} __PACKED AppGw3762Afn06F2State_t;

typedef struct
{
    U8 CmnPhase;
    U8 Addr[MAC_ADDR_LEN];
    U16 MeterIndex;
    S8 ReTransmitCnt;
} __PACKED AppGw3762Afn14F1Up_t;

typedef struct
{
    U8	ucStartTime[6];						//开始时间
    U8	ucDurationTime[2];					//持续时间
    U8	ucRetransTime;						//从节点重发次数
    U8	ucSlotNum;							//随机等待时间片个数
} __PACKED APPGW3762AFN11F5_ST;

typedef struct
{
    U16 usBaudSpeed : 15;		//通讯速率
    U16 usBaudUnit : 1;		//速率单位标识，0:bit/s，1:kbit/s  
}BAUD_INFO_t;
BAUD_INFO_t BaudInfo;


typedef struct
{
    U8    DstAddr[MAC_ADDR_LEN];
    U8    GatherType;
}__PACKED  APPGW3762AFNF0F8_t;

typedef struct
{
    U8 DeviceType			 : 4;			 //设备类型
    U8 Res			         : 3;			 //
    U8 Renewal 			     : 1;			 //更新信息
} __PACKED REPORT_DEVICE_TYPE_t;

typedef struct
{
    U8	MacAddr[6];
    U16 NodeTEI;

    U16 DeviceType   		:  4; 	//设备类型
    U16 Phase               :  3; 	//相位
    U16	LNerr				:  2;   //零火反接  0表示正接，1表示反接，2表示未知
    U16	Edgeinfo			:  2;	//沿信息	0表示和CCO相同，1不相同，2未知
    U16	res                 :  3;
    U16 NodeMachine         :  2; 	//入网的几种状态

    U16 NodeType            :  4; 	//角色类型
    U16 NodeState           :  4; 	//状态
    U16 NodeDepth           :  4; 	//级数
    U16 Reset               :  4;	//复位次数
    U16	DurationTime;               //生存时间

    U16 ParentTEI;                  //父节点
    U8  UplinkSuccRatio;
    U8  DownlinkSuccRatio;

    U8  SoftVerNum[2];
    U16 BuildTime;
    U8  BootVersion;

    U8  ManufactorCode[2];
    U8  ChipVerNum[2];
    U8	F31_D0              :1;
    U8	F31_D1              :1;
    U8	F31_D2              :1;
    U8                      :2;
    U8	F31_D5              :1;
    U8	F31_D6              :1;
    U8	F31_D7              :1;
    
    U8  AREA_ERR;

    U8  CCOMACSEQ;
    U8	ModeNeed;

    MANUFACTORINFO_t		ManufactorInfo;
} __PACKED APPGW3762AFNF005UP_UNIT1_t;

typedef struct
{
    U8	MacAddr[6];

    U16 NodeType            :  4; 	//角色类型
    U16 NodeState           :  4; 	//状态
    U16 NodeDepth           :  4; 	//级数
    U16 Reset               :  4;	//复位次数
    U16	DurationTime;               //生存时间

    U16 ParentTEI;                  //父节点
    U8  UplinkSuccRatio;
    U8  DownlinkSuccRatio;

    U8	F31_D0              :1;
    U8	F31_D1              :1;
    U8	F31_D2              :1;
    U8                      :2;
    U8	F31_D5              :1;
    U8	F31_D6              :1;
    U8	F31_D7              :1;
    
    U8  AREA_ERR;
    U8  CCOMACSEQ;
    U8	ModeNeed;
} __PACKED APPGW3762AFNF005UP_UNIT2_t;

typedef struct
{
    U32	  SNID                           ;

    U8    MacAddr[6]                     ;
    U16   NodeTEI                        ;

    U8    NodeType                   :  4; 	//角色
    U8    NodeDepth                  :  4; 	//层级
    U8    Relation                   :  4;   //于本模块的关?
    U8    Phase                      :  4;   //相位
    U8    BKRouteFg	                 ;
    S8    rgain;                             //GAIN/RecvCount

    U8    UplinkSuccRatio                ;
    U8    DownlinkSuccRatio              ;

    U16   My_REV                         ;   //上周期内接收到邻居节点发送发现列表的次数 +beacon //My_REV
    U16   PCO_SED                        ;   //上周期内邻居节点发送的发现列表次数,从发现列表中直接获取 //PCO_SED
    U16   PCO_REV                        ;    //上周期内邻居节点听到本节点发送列表的次数,从发现列表中直接获取//PCO_REV
    U16   ThisPeriod_REV                 ;   //本周期内接收到邻居节点发送发现列表的次数 +beacon
    U16   PerRoutePeriodRemainTime       ;

    U16	  RemainLifeTime                 ;   //生存时间
    U8	  ResetTimes	                 ;   //复位次数
    U8	  LastRst	                     ;    //上周期记录的复位次数
} __PACKED APPGW3762AFNF014UP_UNIT_t;

typedef struct
{
    U32 Index;
    void (* Func) (APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
} __PACKED AppGw3762ProcFunc;


/******************************* 变量声明 *******************************/
extern APPGW3762DATA_t AppGw3762DownData;
extern APPGW3762DATA_t AppGw3762UpData;
extern U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN];
extern AppGw3762Afn13F1State_t   AppGw3762Afn13F1State;
extern AppGw3762Afn14F1State_t AppGw3762Afn14F1State;
extern APPGW3762AFN10F4_t   AppGw3762Afn10F4State;
extern U8		STARegisterFlag;
extern U8       WIN_SIZE;
extern U8       IsCjqfile;

/******************************* 函数声明 *******************************/

#if 0//((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))

static void AppGw3762Reserve(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn00F1Sure(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn00F2Deny(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn01F1HardInit(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn01F2ParamInit(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn01F3DataInit(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn02F1DataTrans(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn03F1ManufCode(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F2Noise(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F3SlaveMonitor(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F4MasterAddr(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F5MasterStateSpeed(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F6MasterDisturb(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn03F7SlaveTimeout(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F8QueryRfParam(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F9QuerySlaveTimeout(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F10MasterMode(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F11MasterIndex(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F100QueryRssiThreshold(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn04F1SendTest(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn04F2SlaveRoll(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn04F3MasterTest(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn05F1SetMasterAddr(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F2SetSlaveReport(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F3StartBc(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn05F4SetSlaveMaxTimeout(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F5SetRfParam(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F100SetRssiThreshold(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F101SetMasterTime(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn10F1QuerySlaveNum(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F2QuerySlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F3QuerySlaveRelay(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F4RouterState(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F5FailSlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F6RegSlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn10F100QueryNetScale(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F101QuerySlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn11F1AddSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F2DelSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F3SetSlaveRelay(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F4SetWorkMode(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F5StartRegSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn11F6StopRegSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F100SetNetScale(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F101StartNetMaintain(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F102StartNetwork(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn12F1Restart(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn12F2Suspend(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn12F3Recover(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn13F1MonitorSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn14F1RouteReqRead(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn14F2RouterReqClock(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn14F3RouterReqRevise(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn15F1FileTrans(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762AfnF0F5Debug(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762AfnF0F7Debug(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);


#endif

#if ((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))
void app_gw3762_up_afn00_f1_sure(AppGw3762CmdState_e cmdstate, AppGw3762ChanState_e chanstate, U16 waittime, MESG_PORT_e port);
void app_gw3762_up_afn00_f2_deny(AppGw3762ErrCode_e errcode, MESG_PORT_e port);
void app_gw3762_up_afn00_f2_deny_by_seq(AppGw3762ErrCode_e errcode, U8 Seq, MESG_PORT_e port);
void app_gw3762_up_afn02_f1_up_frame(U8 *Addr, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
void app_gw3762_up_afn06_f2_report_read_data(U8 *Addr, U16 meterIndex, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port);

#if (GW376213_PROTO_EN > 0)
void app_gw3762_up_afn06_f3_report_router_change(AppGw3762TaskChange_e taskchange, MESG_PORT_e port);
void app_gw3762_up_afn06_f4_report_dev_type(U8 num, U8 *addr, AppProtoType_e proto,
        U16 index, AppGw3762SlaveType_e devtype, U8 totalnum, U8 curnum, AppGw3762SubInfo_t *info, MESG_PORT_e port);
void app_gw3762_up_afn06_f5_report_slave_event(U8 LenNull_flag,AppGw3762SlaveType_e devtype, AppProtoType_e proto,U8 *meterAddr, U16 len, U8 *data);
#endif

void app_gw3762_up_afn13_f1_up_frame(U8 *Addr, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
void app_gw3762_up_afn14_f1_up_frame(AppGw3762Afn14F1Up_t AppGw3762Afn14F1Up);
void app_gw3762_up_afn14_f2_router_req_clock(MESG_PORT_e port);

#if (GW376213_PROTO_EN > 0)
void renew_all_mode_id_state();
void save_mode_id_by_addr(U8 *macaddr , U8* info ,U8 infolen);
#endif

void app_gw3762_up_info_field_slave_pack(AppGw3762UpInfoField_t *upinfofield, U8 mode, U8 *Nodeaddr, U8 localseq);
void app_gw3762_up_info_field_master_pack(AppGw3762UpInfoField_t *upinfofield, U8 *Nodeaddr);
U16 app_gw3762_fn_bin_to_bs(AppGw3762Fn_e data);
U16 app_gw3762_fn_bs_to_bin(U16 data);
void app_gw3762_afn14_f4_up_frame(U8 type,U8 *ID, U16 len,U8 phase,MESG_PORT_e port);
void app_gw3762_up_afnf1_f1_up_frame(U8 *Addr, AppProtoType_e proto, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
U8 app_gw3762_up_frame_encode(APPGW3762DATA_t *Gw3762Data_t, U8 *data, U16 *len);
void proc_uart_2_gw13762_data(uint8_t *revbuf,uint16_t len);
void proc_app_gw13762_data(uint8_t *revbuf,uint16_t len);
void send_gw3762_frame(U8 *buffer, U16 len, U8 seqNumber, U8 active, U8 retryTimes, MESG_PORT_e port, SERIAL_LID_e lid);
S16 seach_seq_by_mac_addr(U8 *addr);
U8 get_protocol_by_addr(U8 *addr);
U8 devicelist_ban_type(DEVICE_TEI_LIST_t DeviceListTemp);
U8 check_whitelist_by_addr(U8 *addr);
void flash_timer_start(void);
void app_gw3762_up_afn03_f10_master_mode(U8 mode, MESG_PORT_e port);
#endif

#endif

#endif

