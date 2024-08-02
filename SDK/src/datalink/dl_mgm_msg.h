/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     Apstask.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __DATALINK_MSG_H__
#define __DATALINK_MSG_H__

#include "types.h"
#include "DataLinkGlobal.h"
#include "DataLinkInterface.h"
#include "csma.h"

#define HPLC_HRF_PLATFORM_TEST


#define    NET_RELAY_LEVEL_MAX					15
#define    MaxDeepth  15				//最大层级数

#define AODV_MAX_DEPTH                      14 //最大跳数
#define READMETER_UNICAST_MAX_RETRIES       10//抄表业务最大重发次数

#define NWK_MMEMSG_MAX_RETRIES           5 //网络管理消息重发次数。
#define NWK_UNICAST_MAX_RETRIES		     0 //网络层单播最大重发次数
#define NWK_LOCALCAST_MAX_RETRIES		 3 //网络层本地广播最大重发次数
#define NWK_FULLCAST_MAX_RETRIES         1 //网络层全网广播最大重发次数
#define NWK_FULLCAST_DEF_RETRIES         0 //默认0 。底层组帧会默认赋值3次
#define DELAY_LEAVE_TIME					10 //S

#define   CRCLENGTH 						4

#define   CCO_TEI       					1
#define	  ZERO_COLLOCT_NUM					13

#define  OC_CHANGE_TIMES                    5

extern U16 		g_MsduSequenceNumber;   //MSDU序号
U8				   BroadcastAddress[6];

extern U32 g_CnfAssociaRandomNum, g_CnfManageMsgSeqNum;
extern U8 netMsgRecordIndex;

extern U8 ManageID[24];

U8 GlRFChangeOCtimes;


/******************关联请求过滤使用*****************/
typedef struct
{
    U16 msdu;
    U8  valid;
    U8 lifTime;
    U8  macAddr[6];
}__PACKED ZEROTEI_MSDU_t;
#define MAX_ASSCMSDU_NUM    50
#define ASSC_MASKTIME       5
ZEROTEI_MSDU_t AsscMsdu[MAX_ASSCMSDU_NUM];

//关联汇总
#if defined(STATIC_MASTER)

extern ostimer_t g_AssociGatherTimer;
extern ostimer_t g_RfAssociGatherTimer; /*无线汇总定时器*/


#endif


typedef enum
{
    e_MMeAssocReq  = 0x0000,                    // 关联请求
    e_MMeAssocCfm  = 0x0001,                    // 关联回复
    e_MMeAssocGatherInd  = 0x0002,              // 关联汇总指示
    e_MMeChangeProxyReq  = 0x0003,              // 代理变更请求
    e_MMeChangeProxyCnf  = 0x0004,              // 代理变更回复
    e_MMeChangeProxyBitMapCnf = 0x0005,         // 代理变更回复
    e_MMeLeaveInd        = 0x0006,              // 离线指示
    e_MMeHeartBeatCheck  = 0x0007,              // 心跳检测
    e_MMeDiscoverNodeList = 0x0008,             // 发现列表
    e_MMeSuccessRateReport = 0x0009,            // 通信成功率上报
    e_MMeNetworkConflictReport = 0x000a,        // 网络冲突上报
    e_MMeZeroCrossNTBCollectInd = 0x000b,		// 过零 NTB 采集指示
    e_MMeZeroCrossNTBReport = 0x000c,           // 过零 NTB 上报
    e_MMeAreaNotifyNTBReport=0x000f,            //全精度过零NTB告知

    e_MMeRouteRequest  = 0x0050,                // 路由请求
    e_MMeRouteReply = 0x0051,                   // 路由回复
    e_MMeRouteError = 0x0052,                   // 路由错误
    e_MMeRouteAck = 0x0053,                     // 路由应答
    e_MMeLinkConfirmRequest = 0x0054,           // 链路确认请求
    e_MMeLinkConfirmResponse = 0x0055,          // 链路确认回应

    e_MMeRfChannelConfilictReport = 0x0080      // 无线信道冲突上报

} ManageMsgType_e;




typedef enum
{


    e_WithOutPayload,
    e_WithPayload,

} PayloadType_e;


typedef enum
{

    e_CCOdiver,
    e_LEVEL_OVER,
    e_CHANGE_WHITELIST,
} LeaveReason_e;


typedef enum
{
    e_DYNAMIC,
    e_RES
} ProxyType_e;

typedef enum
{
    e_UNKNOWN_PHASE = 0,
    e_A_PHASE,
    e_B_PHASE,
    e_C_PHASE,

} DevicePHASE_e;

typedef enum
{
    e_HasSendBeacon = 0,
    e_IsSendBeacon,
    e_NewNode,

} NodeMachine_e;

typedef enum
{
    e_NeedGet = 0,
    e_HasGet,
    e_NeedReport,
    e_InitState=0xf,    //初始状态和字节长度一致

} ModeMachine_e;

typedef enum
{
    e_UNKW,
    e_CKQ = 1,
    e_JZQ,
    e_METER,
    e_RELAY,
    e_CJQ_2,
    e_CJQ_1,
    e_3PMETER,
} DeviceType_e;

typedef enum
{
    e_MAC_METER_ADDR,
    e_MAC_MODULE_ADDR,
    e_UNKONWN
} MacAddrType_e;


typedef enum
{
    e_NODE_ON_LINE = 1,
    e_NODE_OFF_LINE,
    e_NODE_OUT_NET = 0x0f,
} NodeStateType_e;

typedef enum
{
    e_CCO_SET = 0,
    e_DEPTH_OVER,
    e_NOTIN_WHITELIST,
} LeaveLineReason_e;

typedef enum
{
    e_NO_DEF = 1,
    e_DEF_1PMETER,
    e_DEF_3PMETER,
    e_BC_CHECK_1PMETER,
    e_BC_CHECK_3PMETER,
} DeviceTypeCheck_e;


typedef enum
{
    ASSOC_SUCCESS,                // 表示关联请求成功
    NOTIN_WHITELIST,              // 表示该站点不在白名单中
    IN_BLACKLIST,                 // 表示该站点在黑名单中
    STA_FULL,                     // 表示加入的站点个数超过上限
    NO_WHITELIST,                 // 表示没有设置白名单列表
    PCO_FULL,                     // 表示代理站点个数超过上限
    CHILD_FULL,                   // 表示子站点个数超过上限
    RESERVE ,                      // 保留
    REPEAT_MAC,                   // 表示重复的 MAC 地址
    DEPTH_OVER,                   // 表示超过拓扑层级
    RE_ASSOC_SUCCESS,             // 表示站点再次关联请求入网成功
    CYCLE_ASSOC,                  // 表示新的站点试图以自己的子站点为代理来入网
    LOOP_TOP,                     // 表示组网拓扑中存在环路
    CCO_UNKNOWN_ERR,              // 表示 CCO 端未知原因出错
} AssocResult_e;


typedef enum
{
    e_Reset=0, //重新上电或者复位
    e_FormationSeq, //组网序号改变导致
    e_CCOnidChage, //CCOSNID变化导致
    e_LevelErr,   //父节点的层级15级，自己出网
    e_RecvLeaveInd, //接收到离线指示出网
    e_NotHearBeacon, //两个周期没有听到信标
    e_NohearDisList, //4个周期没有听到父节点的发现列表
    e_NodeRouteLoss,
    e_ProxyChgfail,
    e_LevHig,
    e_ProxyChangeRole, //父节点角色变换为发现节点
    e_reset_dog=20, //看门狗复位  
    e_reset_power, //低电压复位
    e_reboot,     //主动复位
    e_change_linkmode,     //切换链路
    e_NohearRfDisList, //4个周期没有听到父节点的发现列表
    e_elffail,          //CJQ地址不匹配自动离线
}REQ_REASON;

//表示代理变更原因
typedef enum
{
    e_CommRateLow =0, //和原父节点通信成功率低于门限
    e_NoRecvDisList , //整个路由周期没有收到发现列表
    e_ProxyLossInNb,  //父节点从邻居表中消失
    e_FindBetterProxy, //找到了更好的父节点
    e_KeepSyncCCO,   //保持和CCO一致
    e_RouteLoss,    //路由丢失
    e_ProxyNoListenme, //父节点听不到自己，更换父节点
    e_ChangeNet,//变更网络
    e_ElftoElf, //自己变自己
    e_LevelToHigh, //层级高出15跳
    e_FialToTry   //变更失败，尝试使用备份代理
}PROXYCHANGE_REASON;

typedef enum
{
    e_ColloctOne,
    e_ColloctAll,

} ColloctType_e;

typedef enum
{
    e_HalfPeriod,    // 半周期采集
    e_OnePeriod,     // 全周期采集

} ColloctPeriod_e;
typedef enum
{
    e_HalfByte,
    e_OneByte,

} ByteType_e;

typedef enum
{
    e_HPLC_Link = 0,
    e_RF_Link,
    e_Dual_Link,
}LINK_TYPE_e;

typedef enum
{
    e_HPLC_Module = 0,      //HPLC单模
    e_HPLC_RF_Module,       //HPLC+RF 双模
    e_RF_Module             //无线单模模块
}NODULE_TYPE_e;

typedef struct
{
    U8	RetryTime;
    U16 DelayTime;
    U8  Reason;
    U8	Result; //变更结果

} __PACKED CHANGEReq_t;

extern CHANGEReq_t ChangeReq_t ;



typedef struct
{
    U16   MmsgType;
    U16   Reserve;
} __PACKED MGM_MSG_HEADER_t;

typedef struct
{
    U8    ResetCause;
    U8    BootVersion;

    U8    SoftVerNum[2];
    U16   BuildTime;
    U8    ManufactorCode[2];
    U8    ChipVerNum[2];

} __PACKED VERSION_INFO_t;

extern VERSION_INFO_t   VersionInfo ;

//直连站点路由表信息
typedef struct
{
    U16   NodeTei  :12;
    U16   LinkType :1;
    U16   res      :3;
}__PACKED NODE_ROUTE_RABLE_t;

//直连代理站点路由表信息
typedef struct
{
    U16   NodeTei  :12;
    U16   LinkType :1;
    U16   res      :3;

    U16   ChildCount;
}__PACKED PROXY_ROUTE_RABLE_t;

//路由表信息，关联确认中使用
typedef struct
{
    U16    DirectConnectNodeNum;
    U16    DirectConnectProxyNum;

    U16    RouteTableSize;
    U8     reserve[2];

    U8     RouteTable[0];
} __PACKED ROUTE_TABLE_INFO_t;


typedef struct
{
    uint16_t MMType;
    void (* Func) (mbuf_t *buf,  uint8_t *arg, uint16_t len);
} __PACKED NetMgmMsgFunc_t;


//关联请求结构
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8    	ApplyNodeMacAddr[MACADRDR_LENGTH];
    PROXY_INFO_t   	ParentTEI[5];

    U8    	PhaseInfo1 : 2;
    U8    	PhaseInfo2 	: 2;
    U8    	PhaseInfo3 	: 2;
    U8    	Reserve 	: 2;

    U8    	DeviceType;
    U8		MacAddrType;
    U8    	ModuleType  :2;         //模块类型  0：HPLC， 1：双模 2：无线单模
    U8      res         :6;
    U32   	AssociaRandomNum;
    MANUFACTORINFO_t		ManufactorInfo;
    VERSION_INFO_t VersionInfo;
    U16   	HardwareResetCount;
    U16   	SoftwareResetCount;
    U8    	ProxyType;
    U8    	Reserve2[3];
    U32     ManageMsgSeqNum;
    U8      ManageID[24];

} __PACKED ASSOCIATE_REQ_CMD_t;

//关联确认结构
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U8    	DstMacAddr[6];
    U8    	CCOMacAddr[6];
    U8   	Result;
    U8    	NodeDeep;
    U16    	NodeTEI      :12;
    U16     LinkType     :1;            //链路类型    0：HPLC， 1：HRF
    U16     HplcBand     :2;            //载波频段
    U16     res          :1;
    U16    	ProxyTEI	 :12;   //cco为站点分配的代理TEI
    U16    	res2	 :4;        //
    U8    	SubpackageNum;//总分包数
    U8    	SubpackageSeq;//分包序号
    U32   	AssociaRandomNum;
    U32   	ReassociaTime;//重新关联时间
    U32     ManageMsgSeqNum;//端到端序号
    U32   	RouteUpdateSeqNumber;//路径序号
    U8		Reserve[1];
    U8		ApplicationID[2];
    U8		Reserve2[1];
    ROUTE_TABLE_INFO_t  RouteTableInfo;
} __PACKED ASSOCIATE_CFM_CMD_t;

//关联汇总指示结构
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8   	Result;
    U8    	NodeDeep;
    U8    	CCOMacAddr[6];

    U16    	ProxyTEI            :12; //cco自己的TEI
    U16    	HplcBand            :2;  //载波频段
    U16    	res                 :2;  //保留
    U8    	FormationSeqNumber;//组网序列号
    U8    	GatherNodeNum;//汇总站点数

   	U8		Reserve[1];
    U8		ApplicationID[2];
    U8		Reserve2[1];


    U8 		StationInfo[0];
} __PACKED ASSOCIATE_GATHER_IND_CMD_t;


// 代理变更请求报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16    StaTEI;
    PROXY_INFO_t    NewProxyTei[5];
    U16    OldProxyTei;
    U8     ProxyType;
    U8     ChangeCause;

    U32    ManageMsgSeqNum;

    U8    	PhaseInfo1  : 2;
    U8    	PhaseInfo2 	: 2;
    U8    	PhaseInfo3 	: 2;
    U8    	Reserve 	: 2; //用作代理变更请求的原因

    U8     Reserve1[3];
} __PACKED PROXYCHANGE_REQ_CMD_t;


// 代理变更确认报文
typedef struct
{
    MGM_MSG_HEADER_t  MmsgHeader_t;

    U8	  Result;
    U8    AllPacketcount;
    U8    SubPacketSeqNum;
    U8    Reserve;

    U16   StaTEI        :12;
    U16   LinkType      :1;
    U16   res           :3;
    U16   ProxyTei;

    U32   ManageMsgSeqNum;
    U32   RouteUpdateSeqNum;
    U16   ChildStaCount;
    U8    Reserve1[2];

    U8	  StationEntry[0];
} __PACKED PROXYCHANGE_CFM_CMD_t;

// 代理变更位图确认报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8	  Result;
    U8    Reserve;
    U16   BitMapSize;

    U16   StaTEI        :12;
    U16   LinkType      :1;
    U16   res           :3;
    U16   ProxyTei;


    U32   ManageMsgSeqNum;
    U32   RouteUpdateSeqNum;
    U8    Reserve1[4];

    U8	  StationBitMap[0];
} __PACKED PROXYCHANGE_BITMAP_CFM_CMD_t;


// 延时离线指示报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16	  Result;
    U16   StaNum;
    U16   DelayTime;
    U8    Reserve[10];

    U8    MacAddress[0];
} __PACKED DELAY_LEAVE_LINE_CMD_t;


// 心跳检测报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16	  SrcTEI;
    U16   MostTEI;

    U16   MaxiNum;
    U16   BmpNumber;

    U8    TEIBmp[0];
} __PACKED HEART_BEAT_CHECK_CMD_t;


// 上行路由信息
typedef struct
{
    U16		UpTEI:12;
    U16		RouteType:4;
} __PACKED UpLinkRoute_t;


// 发现列表报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U32  NodeTEI		: 12;
    U32  ProxyTEI		: 12;
    U32  NodeType		: 4;
    U32  NodeDeepth	    : 4;

    U8   MacAddr[MACADRDR_LENGTH];
    U8   CCOAddr[MACADRDR_LENGTH];

    U8   PhaseInfo1 	: 2;
    U8   PhaseInfo2 	: 2;
    U8   PhaseInfo3 	: 2;
    U8   Reserve 	    : 2;

    U8	 WithProxyLQI;     //和代理节点的信道质量
    U8   ProxyNodeUplinkRatio;
    U8   ProxyNodeDownlinkRatio;

    U16	 NeighborNum;
    U8	 SendDiscoListCount;
    U8	 UpLinkRouteNum;

    U16  NextRoutePeriodStartTime;
    U16  BmpNumber;

    U8	 LowestSuccRatio;
    U8   Reserve1[3];

    U8   ListInfo[0];
} __PACKED DISCOVER_NODELIST_CMD_t;



// 成功率信息
typedef struct
{

    U16		ChildTEI;
    U8		DownCommRate;
    U8		UpCommRate;
} __PACKED SUCCESSRATE_INFO_t;


// 通信成功率报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16		NodeTEI;
    U16		NodeCount;

    U8		ChildInfo[0];
} __PACKED SUCCESSRATE_REPORT_CMD_t;


//网络冲突上报报文格式
typedef struct
{

    U16		RelayTEI: 12;
    U16		Res: 4;
    U8		CommRate;
    U8		LQI;

} __PACKED RELAY_LIST_t;


//路由请求报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8		RouteRepairVer;
    U32   	RouteRequestSeq;

    U8		Reserve:3;
    U8		RouteSelectFlg:1;
    U8		PayloadType:4;

    U8		PayloadLen;
    U8		Payload[0];

} __PACKED ROUTE_REQUEST_t;

//路由回复报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8		RouteRepairVer;
    U32   	RouteRequestSeq;
    U8		Reserve:4;
    U8		PayloadType:4;
    U8		PayloadLen;
    U8		Payload[0];

} __PACKED ROUTE_REPLY_t;


//路由错误报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8		RouteRepairVer;
    U32   	RouteRequestSeq;
    U8		Reserve;
    U8		UnArriveNum ; //不可到?站??量
    U8		UnArriveList[0];

} __PACKED ROUTE_ERR_t;

//路由?答报文
typedef struct
{

    MGM_MSG_HEADER_t MmsgHeader_t;

    U8		RouteRepairVer;
    U8		Reserve[3];
    U32   	RouteRequestSeq;
} __PACKED ROUTE_ACK_t;


//链路确认请求报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U8		RouteRepairVer;
    U32   	RouteRequestSeq;
    U8		Reserve;
    U8      ConfirmStaNum;
    U8      ConfirmStaList[0];
} __PACKED LINK_CONFIRM_REQUEST_t;

//链路确认回应报文
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8		RouteRepairVer;
    U8		Deepth;
    U8      LIQ;
    U8		RouteSelectFlg	: 1;
    U8      Reserve			: 7;
    U32   	RouteRequestSeq;

} __PACKED LINK_CONFIRM_RESPONSE_t;


//网络冲突上报报文格式
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U8    CCOAddr[MACADRDR_LENGTH];
    U8    NBnetNumber; //邻居网络个数
    U8    NIDWidth;

    U8    NBnetList[0];  //邻居网络条目

} __PACKED NETWORK_CONFLIC_REPORT_t;


//过零NTB采集只是报文格式
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16		NodeTEI		: 12;
    U16		Res			: 4;
    U8		ColloctStaion;
    U8		ColloctPeriod;

    U8		ColloctNumber;
    U8		Res1[3];
} __PACKED ZERO_CROSSNTB_COLLECTIND_t;


typedef struct
{
    MGM_MSG_HEADER_t   MmsgHeader_t;

    U16		NodeTEI		: 12;
    U16		Res			: 4;
    U8		ColloctNumber;
    U8		Phase1Num;
    U8		Phase2Num;
    U8		Phase3Num;
    U32		FirstNTB;    //是采集过零点NTB值原始32BIT右移8比特的值

    U8		NTBdiff[0];
} __PACKED ZERO_CROSSNTB_REPORT_t;

/************************无线新增***********************/
//无线信道冲突上报
typedef struct
{
    MGM_MSG_HEADER_t   MmsgHeader_t;

    U8 CCOAddr[6];
    U8 NBNetNum;
    U8 NBnetList[0];  //邻居网络条目s
} __PACKED RFCHANNEL_CONFLIC_REPORT_t;

//无线发现列表相关。
enum{
    e_RF_DISCV_STAINFO    = 0,             //站点信息属性
    e_RF_DISCV_ROUTEINFO,                   //站点路由信息
    e_RF_DISCV_NBCHLINFO_NOBITMAP,          //邻居节点信道信息非位图版
    e_RF_DISCV_NBCHLINFO_BITMAP             //邻居节点信道信息位图版
};
//站点属性信息结构体
typedef struct
{
//    U8  unitType            :7;             //单元类型  0：站点属性信息
//    U8  unitLenType         :1;             //单元类型  0：1字节  1：2字节
//    U8  unitLen;                            //单元内容长度。 站点信息默认使用一个字节

    U8  CCOMAC[6];                          //CCO MAC 地址
    U16 ProxyTEI            :12;            //代理TEI
    U16 NodeType            :4;             //角色

    U8  NodeLevel           :4;             //层级
    U8  RfCount             :4;             //链路RF跳数
    U8  ProxyUpRcvRate;                     //代理上行接收率
    U8  ProxyDownRcvRate;                   //代理下行接收率
    U8  MixRcvRate;                         //链路最小接收率

    U8   DiscoverPeriod;                    //无线上发现列表周期长度， 单位：1s
    U8   RfRcvRateOldPeriod;                //无线接收率老化周期个数， 单位：无线发现列表周期。
}__PACKED RF_DISC_STAINFO_t;

//无线信道信息类型组合非位图版
#define  RF_NBINFOTYPELEN_GROUP_COUNT 10
typedef struct{
    U16 TEILen          :4;                 //TEI信息长度             单位：bit
    U16 RcvRateLen      :4;                 //接收率信息长度           单位：bit
    U16 SNRAvgLen       :4;                 //平均信噪比信息长度        单位：bit
    U16 RSSIAvgLen      :4;                 //信号强度信息长度          单位：bit

    U16  InfoLen;                            //总长度                   单位：bit
}__PACKED RF_DISC_NBINFOTYPELEN_t;
extern RF_DISC_NBINFOTYPELEN_t g_RfNbInfoTypeLenGroup[RF_NBINFOTYPELEN_GROUP_COUNT];
//无线信道信息类型组合位图版
#define  RF_NBINFOTYPELEN_MAP_GROUP_COUNT 11
typedef struct{
    U8 RcvRateLen      :4;                 //接收率信息长度           单位：bit
    U8 SNRAvgLen       :4;                 //平均信噪比信息长度        单位：bit
    U8 RSSIAvgLen;                         //信号强度信息长度          单位：bit

    U16  InfoLen;                            //总长度                   单位：bit
}__PACKED RF_DISC_NBINFOTYPELEN_MAP_t;
extern RF_DISC_NBINFOTYPELEN_MAP_t g_RfNbInfoTypeLenGroupMap[RF_NBINFOTYPELEN_MAP_GROUP_COUNT];
typedef struct
{
//    MGM_MSG_HEADER_t   MmsgHeader_t;        //<<< 单跳帧携带无线管理列表是否需要 未定义管理消息类型???

    U8 MacAddr[6];                          //发送站点MAC地址
    U8 DiscoverSeq;                         //发现列表统计序号 0-255 循环递增
    U8 ListInfo[0];                         //信息域： 信息单元类型，信息单元长度类型，信息单元长度，信息单元内容
}__PACKED RF_DISCOVER_LIST_t;


#define MAXREPORTNUM	100
#define MAXCOLLNUM 150

typedef struct
{
    U8		ColloctPeriod;   // 采集周期
    U8 	 	ColloctType;     // 采集类型
    U16		ColloctTEI;      // 当前采集TEI
    U8		ColloctFlag;     // 是否采样
    U8		ColloctNumber;   // 采样总次数
    U8		ColloctSeq;      // 当前采样序号
    U32		ColloctResult[MAXCOLLNUM];//过零NTB

} __PACKED ZERO_COLLECTIND_RESULT_t;


extern	ZERO_COLLECTIND_RESULT_t	Zero_Result;//CCO采样时间记录,STA CAN USE



/*******************************************AODV***************************/
typedef struct{
    U8     payload[1024];
    U16   payloadLen;
}AODV_BUFF_t;

extern AODV_BUFF_t    AodvBuff;
extern U8 			 g_AodvMsduLID;//APS层传下来的LID


void CreatMMsgMacHeaderForSig(MAC_RF_HEADER_t *pMacHeader, U8 msduType, U16 msduLen);

void CreatMMsgMacHeader(MAC_PUBLIC_HEADER_t *pMacHeader, U16 MsduLen, U8 *DestMacAddr , U16 DestTEI, U8 SendType,
                        U8 MaxResendTime, U8 ProxyRouteFlag, U8 Radius, U8 RemainRadius, U8 bradDirc,U8 MacAddrUse);

//将CSMA时隙要发送的数据挂在 TX_MNG_LINK 或者 TX_DATA_LINK链表上
void entry_tx_mng_list_data(U8 priority, U16 macLen, U8 *pMacLongHeader, U16 destTEI, U8 phase, tx_link_t *tx_list, work_t *cfm);

//将代理信标和发现信标data挂在链表BEACON_FRAME_LIST
void entry_tx_beacon_list_data(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, list_head_t *tx_list);

//将转发得无线信标帧挂在无线信标列表上。标准帧和精简信标帧分别挂在不同的链表
void entry_rf_beacon_list_data(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, list_head_t *tx_list);
void entry_rf_beacon_list_data_csma(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, tx_link_t *tx_list);
void entry_rf_msg_list_data(U8 priority, U16 macLen, U8 *pMacLongHeader, U16 destTEI, U8 ResendTime, tx_link_t *tx_list, work_t *cfm);
void entry_tx_coord_data();

//发送发现列表
void SendDiscoverNodeList(void);

//从节点管理消息函数接口
#if defined(STATIC_NODE)

U8 SendAssociateReq(U16 DelayTime,U8 Reason);
void SendProxyChangeReq(U8 Reason);
void sendproxychangeDelay(U8 Reason);

void SendHeartBeatCheck(work_t *work_t);
void SendSuccessRateReport(work_t * work);
void SendNetworkConflictReport(U8 *pMacAddr, U16 DelayTime, U8 MacUse);
void SendMMeZeroCrossNTBReport(U8 MacUse);
void SendMMeRFChannelConflictReport(U8 *pMacAddr, U16 DelayTime, U8 MacUse, U8 option, U16 channel);

#endif

//主节点管理消息函数接口
#if defined(STATIC_MASTER)


void SendAssociateCfm(U8 Status, U8 *AssocDstMacAdd, U32 AssocRandomNum,
                                         U32 MMsgSeqNum , U8 Level, U16 TEI, PROXY_INFO_t ProxyInfo, U16 DelayTime, U8 MacUse);
void  SendAssociateGatherInd(U16 TEI);
void SendProxyChangeCnf(U16 OriginateTEI, U16 ProxyTEI, U32 ManageMsgSeq , U16 DelayTime, U8 MacUse,U8 Reason);
void SendProxyChangeBitMapCnf(U16 OriginateTEI, U16 ProxyTEI, U32 ManageMsgSeq, U16 DelayTime, U8 MacUse,U8 Reason);
void SendMMeDelayLeaveOfNum(U8 *MacAddr, U8 StaNum, U16 DelayTime,U8 DelType);
void SendMMeZeroCrossNTBCollectInd(U16 NodeTEI, U16 DelayTime, U8 MacUse);

void NetTaskGatherInd(work_t *work);

#endif




void ProcessMMeAssocCnf(mbuf_t *buf, U8 *pMacInd, U16 len);

//aodv 路由使用
void SendMMeRouteReq( U16 DstTEI, U16 ScrTei, U16 Handle);
void SendLinkRequest( struct ostimer_s *ostimer, void *arg);
void SendLinkResponse(U32 RequestSeq, U8 DstTEI, U8 LIQ);
void SendMMeRouteReply();
void SendMMeRouteAck(U32 RouteRequestSeq, U16 DstTEI);
void SendMMeRouteError(U16 OriginalTEI,U16 DstTEI);
void WaitReplyCmdOut(struct ostimer_s *ostimer, void *arg);

void SendMMeRFDiscoverNodeList(void);
void ProcessMMeRFDiscoverNodeList(mbuf_t *buf, U8 *pld, U16 len, work_t *work);


//sof 帧数据处理接口
void ProcessNetMgmMsg(mbuf_t *buf, U8 *pld, U16 len, work_t *work);

#endif


