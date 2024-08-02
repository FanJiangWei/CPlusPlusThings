#ifndef __DATALINKGLOBAL_H__
#define __DATALINKGLOBAL_H__

#include "types.h"
#include "sys.h"
#include "phy_plc_isr.h"
#include "list.h"
#include "ZCsystem.h"
#include "ScanRfCHmanage.h"

//路由表状态
#define ACTIVE_ROUTE	0
#define INACTIVE_ROUTE  1

#define INVALID				0xffff

#define	   SNID_LIFE_TIME				  600 //邻居网络信息生存时间

#define MAX_ROUTE_FAILTIME               3  // 路由错误次数限制
#define MAX_DISCOERY_TABLE_ENTRYS  	  50//最大路由条目数
#define MAX_DISCOERY_EPIRATION_TIME    2 //最大生存时间

#if defined(STATIC_MASTER)
#define MAX_WHITELIST_NUM				 2040//最大白名单数量
#else
#define MAX_WHITELIST_NUM				 1015//最大白名单数量
#endif
#define NWK_MAX_ROUTING_TABLE_ENTRYS  MAX_WHITELIST_NUM //1015//最大路由条目数
#define NWK_AODV_ROUTE_TABLE_ENTRYS    2  //UP  AND DOWN


//#define RoutePeriodTime_Normal           			100//0x64//?????????

#define	UP_LINK_LOWEST_RATIO			  20//上行通信成功率最小值
#define	DOWN_LINK_LOWEST_RATIO		  20//下行通信成功率最小值
#define INITIAL_SUCC_RATIO			  	  20//初始通信成功率

#define NICE_SUCC_RATIO			  	  54//优化代理的通信成功率门限
#define BACKUP_SUCC_RATIO				  50//备份路由门限


#define	  BLACKNEIGBORLIFE				  200
#define   BADLINKTIME					  600

#define  	MAX_NET_MMDATA_LEN 		   1024  //net data max len

#define ROUTE_RELAY_TIME					10


#define	LOWST_RXLQI_PCO					  85// 台区识别是需要注意
#define	LOWST_RXLQI_STA					  80// 台区识别是需要注意
#define	LOWST_RXLQI_CCO					  85// 台区识别是需要注意
#define	LOWST_RXGAIN					  55// 接收增益门限值

#define LOWST_RXRSSI                      -80 //无线接收信号强度门限值


#define	REV_ERR_BEACON_NUM				   3// 收到错误信标记录3条
#define	ERR_BEACON_CLEAN_TIME			  22// 错误信标清除超时22S
#define MSDUSEQ_INITVALUE                 0 //MSDU序号初始值

/**********************Access Control list,exit in STA****************************/
extern U8 AccessListSwitch;
#define    MAX_ACL_TABLE_ENTRYS        3
typedef struct
{
    U8    MacAddr[MACADRDR_LENGTH];
    U8    PermitOrDeny;
}__PACKED ACCESS_CONTROL_LIST_t;

typedef struct
{
    U8 Number;
    ACCESS_CONTROL_LIST_t AccessControlList[MAX_ACL_TABLE_ENTRYS];
}__PACKED ACCESS_CONTROL_LIST_ROW_t;

extern ACCESS_CONTROL_LIST_ROW_t  AccessControlListrow;


extern U8 g_ReqReason;



/**********************Neighbor node Table,exit in CCO and STA*******************/

#if defined(STATIC_MASTER)
#define MAX_NEIGHBOR_NUMBER 			  MEM_NR_C//邻居表的最大数量
#else
#define MAX_NEIGHBOR_NUMBER 			  MEM_NR_C//邻居表的最大数量
#endif


//邻居表中和本节点的关系
typedef enum
{
    e_EERROUTE = 0, //错误路由
    e_SAMELEVEL, //同级备份路由
    e_UPLEVEL,  //上级备份路由
    e_PROXY , 	//选为代理
    e_UPPERLEVEL,//上上级备份路由
    e_CHILD,		//子节点  ，只有收到关联请求时，发起者才本节点的子节点
    e_NEIGHBER, //仅仅为邻居无路有关系，在备用代理足够的情况下才存在
} RELATION_TYPE_e;


typedef enum{
	e_DOUBLE_HPLC_HRF = 0,	//混合组网
	e_SINGLE_HPLC = 1,		//载波组网
	e_SINGLE_HRF = 2,		//无线组网
}F5F18_NETWORK_TYPE;

//邻居发现列表
typedef struct
{
	list_head_t link;
	
    U32	  SNID;
	
    U8    MacAddr[6];
    U16   NodeTEI;
    U8    NodeDepth  ; 	//层级
    U8    NodeType      :  4; 	//角色
    U8    Relation      :  4; //于本模块的关糿
    U8    Phase       	:  4; //相位
    U8    BKRouteFg		:  4;


    S32	  GAIN;	       	//增益累加和
    S32   RecvCount;  	//接收次数

	
    U16   Proxy;    //邻居节点的父节点


    U8    LastUplinkSuccRatio;          //记录上个周期的通信成功率，避免频繁的发生代理变更
    U8    LastDownlinkSuccRatio;        //记录上个周期的通信成功率，避免频繁的发生代理变更

    U8    UplinkSuccRatio;
    U8    DownlinkSuccRatio;

    U8   My_REV;//上周期内接收到邻居节点发送发现列表的次数 +beacon //My_REV
    U8   PCO_SED;//上周期内邻居节点发送的发现列表次数,从发现列表中直接获取 //PCO_SED
    U8   PCO_REV; //上周期内邻居节点听到本节点发送列表的次数,从发现列表中直接获取//PCO_REV
    U8   ThisPeriod_REV;//本周期内接收到邻居节点发送发现列表的次数 +beacon

    U16	  RemainLifeTime ;//生存时间
    U8	  ResetTimes	; //复位次数
    U8	  LastRst	; //上周期记录的复位次数
    U16	  MsduSeq; //邻居节点上一帧的序号
    U8    BeaconCount; //邻居信标周期计数
    U8    Res[16];
} __PACKED NEIGHBOR_DISCOVERY_TABLE_ENTRY_t;

//extern NEIGHBOR_DISCOVERY_TABLE_ENTRY_t NeighborDiscoveryTableEntry[MAX_NEIGHBOR_NUMBER];


typedef struct datalink_table_s{
	list_head_t link;
	
	uint16_t nr;
	uint16_t thr;
}DATALINK_TABLE_t;

extern DATALINK_TABLE_t	NeighborDiscoveryHeader;
//更新邻居表中接收邻居的次数
//void UpdataNbStaRecvCnt(U32 _SNID, U8 *_MacAddr, U16 _NodeTEI);
U8 UpdataNbSendCntForBeacon();

/*****************无线邻居表维护************************************/
#define RF_RCVMAP  16  //记录无线发现列表接收位图大小。32BIT  可选 8/16/32
typedef struct{
    list_head_t link;

    U32	  SNID;

    U8    MacAddr[6];
    U16   NodeTEI;

    U8    NodeDepth  ;              //层级
    U8    NodeType      :  4;       //角色
    U8    Relation      :  4;       //于本模块的关糿
    U8    Phase       	:  4;       //相位
    U8    BKRouteFg		:  4;
    U16   Proxy;                    //邻居节点的父节点


    U8    UpdateIndex;              //Rcvmap更新索引
    U8    RcvMap[RF_RCVMAP/8];      //无线发现列表接收位图
    U8    UpRcvRate;                //上行通信接收率，通过报文交换获取
    U8    DownRcvRate;              //下行通信接收，通过RcvMap位图计算得到
    U8    NotUpdateCnt;             //上行接收率未更新周期，上行接收率老化使用


    int8_t DownRssi;                //需要大于-90
    U16    DownRssiCnt;
    int8_t DownSnr;                 //>>-4有err, 2-4为佳, 需要验证
    U16    DownSrnCnt;
    int8_t UpRssi;
    int8_t UpSnr;

    U16	  RemainLifeTime ;          //生存时间
    U8	  ResetTimes	;           //复位次数
    U8	  LastRst	;               //上周期记录的复位次数
    U16	  MsduSeq;                  //邻居节点上一帧的序号
    U8    BeaconCount;              //邻居信标周期计数
    U8    RfCount;                  //邻居节点链路上无线跳数

    U8   RfDiscoverPeriod;          //无线上发现列表周期长度， 单位：1s
    U8   RfRcvRateOldPeriod;        //无线接收率老化周期个数， 单位：无线发现列表周期。
    U8   DicvPeriodCntDown;         //发现列表周期递减计数。接收率老化使用
    U8   RcvRateOldPeriodCntDown;   //老换周期个数递减计数，上行接收率老化使用
}__PACKED RF_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t;

extern DATALINK_TABLE_t	RfNeighborDiscoveryHeader;

typedef struct
{
    U8   My_REV;//上周期内接收到邻居节点发送发现列表的次数 +beacon //My_REV
    U8   PCO_SED;//上周期内邻居节点发送的发现列表次数,从发现列表中直接获取 //PCO_SED
    U8   PCO_REV; //上周期内邻居节点听到本节点发送列表的次数,从发现列表中直接获取//PCO_REV
    U8   ThisPeriod_REV;//本周期内接收到邻居节点发送发现列表的次数 +beacon

    U8    LastUplinkSuccRatio;          //记录上个周期的通信成功率，避免频繁的发生代理变更
    U8    LastDownlinkSuccRatio;        //记录上个周期的通信成功率，避免频繁的发生代理变更
    U8    UplinkSuccRatio;
    U8    DownlinkSuccRatio;

    S32	  GAIN;	       	//增益累加和
    S32   RecvCount;  	//接收次数

    U8   Res[4];

}__PACKED HPLC_NEIGHBOR_INOF_t;

typedef struct
{

    U8    RcvMap[RF_RCVMAP/8];      //无线发现列表接收位图
    U8    res[4 - RF_RCVMAP/8];

    U8    UpdateIndex;              //Rcvmap更新索引
    U8    LUpRcvRate;               //上个路由周期上行通信接受率，路由周期结束老化
    U8    LDownRcvRate;             //上个路由周期下行通信成功率，路由周期结束老化
    U8    UpRcvRate;                //上行通信接受率，通过报文交换获取
    U8    DownRcvRate;              //下行通信，通过RcvMap位图计算得到
    U8    NotUpdateCnt;             //上行接收率未更新周期，上行接收率老化使用
#define MAX_RSSI_SNR_CNT 10         //信号轻度和信噪比，移动平均滑动窗口大小 
    S8    DownRssi;                 //下行平均信号强度
    U16   DownRssiCnt;              //下行信号强度统计计数，用于计算平均信号强度
    S8    DownSnr;                  //下行平均信噪比
    U16   DownSrnCnt;               //下行信噪比统计计数，用于计算均信噪比
    S8    UpRssi;
    S8    UpSnr;


    U8   RfDiscoverPeriod;            //无线上发现列表周期长度， 单位：1s
    U8   RfRcvRateOldPeriod;        //无线接收率老化周期个数， 单位：无线发现列表周期。
    U8   DicvPeriodCntDown;         //发现列表周期递减计数。接收率老化使用
    U8   RcvRateOldPeriodCntDown;   //老换周期个数递减计数，上行接收率老化使用

    U8   DiscoverSeq;               //无线发现列表序号，过滤使用
    U8   Res[1];

}__PACKED HRF_NEIGHBOR_INOF_t;


typedef struct {

    list_head_t link;

    U32	  SNID;

    U8    MacAddr[6];
    U16   NodeTEI;

    U8    NodeDepth  ; 	//层级
    U8    NodeType      :  4; 	//角色
    U8    Relation      :  4; //于本模块的关糿
    U8    Phase       	:  4; //相位
    U8    BKRouteFg		:  4;
    U16   Proxy         :  12;    //邻居节点的父节点
    U16   LinkType      :  4;    //区分载波邻居还是无线邻居

    U16	  RemainLifeTime ;          //生存时间
    U8	  ResetTimes	;           //复位次数
    U8	  LastRst	;               //上周期记录的复位次数
    U16	  MsduSeq;                  //邻居节点上一帧的序号
    U8    BeaconCount;              //邻居信标周期计数
    U8    RfCount;                  //邻居节点链路上无线跳数

    U8    childUpRatio;             //记录子节点的上行成功率   
    U8    childDownRatio;           //记录子节点的下行成功率      

    //区分载波邻居和无线邻居表数据
    union
    {
        HRF_NEIGHBOR_INOF_t HrfInfo;
        HPLC_NEIGHBOR_INOF_t HplcInfo;
    };

    U8 data[0];

}__PACKED ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t;

/**********************Router Table,main router,exit in CCO and STA********************/
//路由衿
typedef struct
{
    U16     NextHopTEI	:12;
    U16     RouteState  : 1;
    U16     RouteError  :3;

    U16     LinkType    :2;
    U16     ActiveLk    :2;      //bit0:载波有效 bit1:无线有效
    U16     res         :12;
} __PACKED ROUTE_TABLE_ENTRY_t;
typedef struct
{
    U16     DestTEI		:12;
    U16     RouteState   : 3;
    U16     LinkType     : 1;
    U16     NextHopTEI	:12;
    U16     RouteError  :4;
} __PACKED ROUTE_TABLE_RECD_t;

extern ROUTE_TABLE_ENTRY_t NwkRoutingTable[NWK_MAX_ROUTING_TABLE_ENTRYS];

typedef enum{
    e_AODV_UP = 0,
    e_AODV_DOWN = 1,
}E_AODV_DIR_e;
extern  ROUTE_TABLE_RECD_t    	 NwkAodvRouteTable[NWK_AODV_ROUTE_TABLE_ENTRYS];


/**********************Router Discovery Table,aodv use it,exit in STA********************/

//路由发现衿
typedef struct
{
    U16   SrcTEI;
    U16   DstTEI;
    U16   UpTEI;
    U16   RouteRequestID;
    U8    RemainRadius;
    U8    ExpirationTime;
} __PACKED ROUTING_DISCOVERY_TABLE_ENTRY_t;
extern ROUTING_DISCOVERY_TABLE_ENTRY_t 	RouteDiscoveryTable[MAX_DISCOERY_TABLE_ENTRYS];
extern ROUTING_DISCOVERY_TABLE_ENTRY_t			RouteDiscoveryEntryOne;



/**********************CCO Neighbor net Table,use by Multi network coordination,exit in CCO*******************/
#define	   MaxSNIDnum					  48  //保存的邻居网络信息最大数釿

#if defined(STATIC_MASTER)
typedef struct
{
    U32   SendSNID;    //发送者的网络叿
    U32   NbSNID[MaxSNIDnum];  //发送者的邻居

    U16	  Duration;   //信道占用时间
    U16   SendStartOffset;			//下次信道占用的时闿/

    U16   MyStartOffset;			// 本节点下次占用还有多长时闿/
    U16	  LifeTime;			//S unit，生命时闿

    U8    SendChannl;
    U8    SendOption :2;
    U8              :6;
    U8    CCOMacAddr[6];

    U8    NbrChannl[MaxSNIDnum];
    U8    NbrOption[MaxSNIDnum];
    U8    Revs[2];
} __PACKED NEIGHBOR_NET_t;
extern NEIGHBOR_NET_t  	NeighborNet[MaxSNIDnum];

#define  MaxChannelNum                16
typedef struct
{
    U8    SendChannl;
    U8    SendOption;
    U8    CCOaddr[6];
    U8    NbrChannl[MaxChannelNum];
    U8    NbrOption[MaxChannelNum];
    U16   LifeTime;
}__PACKED RF_NEIGHBOR_NET_t;
extern RF_NEIGHBOR_NET_t RfNbrNet[MaxChannelNum];

extern U8 DstCoordRfChannel;
extern uint8_t get_valid_channel(RF_OPTIN_CH_t *pRfparam);
#endif

/**********************STA Neighbor net Table,use by net selected and estimate Nbnet ,exit in STA*******************/
typedef enum
{
    e_BadLink=1,
    e_NotInlist=2

}ACESS_ERR_e;

typedef struct
{
    U32   		NbSNID;  //发送者的邻居

    U16	  		LifeTime;			//S unit，生命时闿
    U16	  		SameRatio; //相似度记彿

    U32	  		DiffTime;	  //和本网络定差

	U16 		Blacktime;
    U8			ErrCode; //0,表示允许加入＿表示加入链路不好＿表示不在白名单中
    U8			bandidx; //表示在哪一个频段被加入黑名单

    U16         option    :4;
    U16         RfChannel :12;
    U16         res;
} __PACKED STA_NEIGHBOR_NET_t;

extern STA_NEIGHBOR_NET_t StaNeigborNet_t[MaxSNIDnum]; //入网和未入网均可使用

/**********************NNIB,exit in CCO and STA****************************/
//网络层属性库
typedef struct
{
    U8	  MacAddr[6];
    U8	  WorkState ;                          //表示模块处于监听、工作状怿
    U8	  SnidlnkTp ;                          //当前入网流程是无线还是载波入网；0：载波， 1：无线
    U8	  Networkflag;                         // 是否完成组网
    
    U8	  NodeState;                           //网内、网夿
    U8	  NodeLevel;
    U8	  PossiblePhase;
    U8	  BackupPhase1;
	
    U8	  BackupPhase2;
    U8	  PhasebitA:1;
    U8	  PhasebitB:1;
    U8	  PhasebitC:1;
    U8	  PhaseJuge:1;                         //过零中断启动检浿
    U8	  PhaseRes:4;
    U8	  DeviceType;
    U8	  MacAddrType;
	
    U32   AssociaRandomNum;
	
    U16   HardwareResetCount;
    U16   SoftwareResetCount;
	
    U8	  CCOMacAddr[6];
	U8	  FormationSeqNumber;
    U8	  Resettimes;
	
    U32   RouteUpdateSeqNumber;
    U32   ManageMsgSeqNum;

    U8	  ParentMacAddr[6];
    U8	  NodeType;
    U8	  WithProxyLQI;
	
	U32   LowestSuccRatio;

    U8	  BeaconLQIFlag;
    U8	  FinishSuccRatioCal;
    U8	  LProxyNodeUplinkRatio;
    U8	  LProxyNodeDownlinkRatio;
	
    U8	  ProxyNodeUplinkRatio;
    U8	  ProxyNodeDownlinkRatio;
    U8	  last_My_SED;                      //上个周期发现列表的数釿
    U8	  CSMASlotPhaseCount;


    U32      NextRoutePeriodStartTime;      //路由剩余时间

    U16      RoutePeriodTime;               //路由周期
    U8	  BindCSMASlotPhaseCount;
    U8      AODVRouteUse;


    U32   SendDiscoverListTime;             //发送发现列表报文周朿
    U32   HeartBeatTime;                    //心跳检测报文周朿
    U32   SuccessRateReportTime;            //通讯成功率上报周朿

    U8	  SendDiscoverListCount;            //当前路由周期发送的发现列表的数釿
    U8	  beaconProtect;                    //STA若一段時間沒有安排發送信標離線
    U8	  FristRoutePeriod;
    U8	  SynRoutePeriodFlag;               //表示在路由周期结束后是否已经和父节点再次同步

    U16   StaOfflinetime;
    U16   RecvDisListTime;

    U32   SuccessRateZerocnt;               //四个路由周期通信成功率为0 离线

    U32   BandRemianTime;                   //切换频段剩余时间

    U8	  beaconSendTime;
    U8	  BandChangeState;                  //是否需要更改频殿
    U8	  PowerType;                        //强弱电系绿
    U8	  Edge;                             //过零沿信恿

   
    U16	  TGAINDIG;                         //数字功率
    U16	  TGAINANA;                         //模拟功率

    U32   BeaconPeriodLength;
	
    U16	  discoverSTACount;
    U16   PCOCount;
	
    U8	  NbSNIDnum;
    U8	  powerlevelSet;                    //设置功率
    U8    ModuleType       :2;              //模块类型
    U8    LinkType         :1;              //链路类型。0：HPLC 1：无线
    U8    RfConsultEn      :1;              //信道协商是能  1：允许协商 0：不允许协商
    U8    RfOnlyFindBtPco  :1;
    U8    res              :4;
    U8    RfCount;                          //链路RF 跳数

    U8    RfDiscoverPeriod;                 //无线发现列表周期              根据网络规模可以设置 10-255秒
    U8    RfRcvRateOldPeriod;               //无线接收率老化周期             根据网络规模可以设置4-16个无线发现列表周期
    U16   RfChannelChangeState   : 1;       //是否需要变更无线信道标志
    U16   RfChgChannelRemainTime :15;       //无线信道切换剩余时间   uint:s

    U8 	  LinkModeStart;                    //启动工作模式切换
    U8 	  LinkMode    :4;                   //工作模式
    U8 	  NetworkType :4;                   //组网方式：0：混合组网，1：载波组网， 2:无线组网
    U16   RfRecvDisListTime;
    U16	  ChangeTime;                       //unit:s
    U16	  DurationTime;                     //unit:s

    

}  NET_MANAGE_PIB_t;
extern NET_MANAGE_PIB_t 	nnib; //网络层属性库

/**********************白名單****************************/

typedef struct
{
    U8 ucRelLevel			 : 4;			 //中继级别
    U8 ucSigQual			 : 4;			 //信号品质

    U8 ucPhase 			     : 3;			 //相位
    U8 ucGUIYUE			     : 3;			 //通信协议类型
    U8 LNerr 			     : 1;			 //零火反接
    U8 ucReserved			 : 1;
} __PACKED METER_INFO_t;

typedef struct
{
    U8  MacAddr[6];
    U8  WaterAddrH7;
    METER_INFO_t MeterInfo_t;
    U8  CnmAddr[6];
    U8  Result; 			  	//0 未完成 1 完成
    U8  waterRfTh;			    //水表场强
    U8  ModuleID[11];
    
    U16 SetResult  :1;		    //配置成功
    U16 IDState    :2;          //ID更新标志
    U16 Reserved1  :1;
    U16 TEI        :12;
    U8  Reserved[2];
} __PACKED WHITE_MAC_LIST_t;

extern WHITE_MAC_LIST_t WhiteMacAddrList[MAX_WHITELIST_NUM];

typedef struct
{
    U8 MacAddr[6];
    METER_INFO_t MeterInfo_t;
    
    U8 ModuleID[11];
    U8 SetResult  :1;		//配置成功
    U8 IDState    :2;       //ID更新标志
    U8 Reserved   :5;
} __PACKED WHITE_LIST_t;

typedef struct
{
    WHITE_LIST_t WhiteList[MAX_WHITELIST_NUM];
    U16          WhiteListNum;
    U16          CS16;
} __PACKED FLASH_WHITE_LIST_t;

extern FLASH_WHITE_LIST_t FlashWhiteList;



typedef struct
{
    U8  CJQAddr[6];
	U8  ReportFlag;     //上报标志
	U8  Res[2];
} __PACKED WHITE_MAC_CJQ_MAP_LIST_t;

extern WHITE_MAC_CJQ_MAP_LIST_t WhiteMaCJQMapList[MAX_WHITELIST_NUM];

//数据链路层管理列表整琿
typedef struct
{

    U8         HardwareVer[2];
    U8         InnerVer[2];
    U16        InnerBTime;
    U8		   edgetype;		//
    U32        reboot_reason;
    U8		   res[7];
} __PACKED MANUFACTORINFO_t;


typedef struct
{
    U16 ParatTEI   :12;
    U16 LinkType   :1;
    U16 res        :3;
}__PACKED PROXY_INFO_t;


/**********************Device list ,CCO topo,exit in CCO**********************/
#if defined(STATIC_MASTER)

typedef struct
{
    list_head_t 		link;
    
    U8    MacAddr[6];				//设备MAC地址
    U16	  DurationTime;      		//生存时间

    U16   NodeTEI	        :  12;    //TEI
    U16   DeviceType   		:  4; 	//设备类型
    U16   ParentTEI;         		//父节炿

    U32   NodeState     	:  4; 	//状怿
    U32   NodeType      	:  4; 	//角色类型
    
    U32   Phase         	:  3; 	//相位
    U32   LinkType          :  1;   //链路类型。0：HPLC 1：无线
    U32	  LNerr				:  2;   //零火反接  0表示正接＿表示反接＿表示未知
    U32	  Edgeinfo			:  2;	//沿信恿0表示下降沿，1上升沿，2未知
    
    U32   NodeMachine       :  2; 	//信标安排时使甿
    U32   Reset	       		:  4;	//复位次数
    U32   ModuleType        :  2;   //模块类型，0：单载波，1：HPLC+RF，2：单无线
    
    U32	  F31_D0			:1;     //接线柿
    U32	  F31_D1			:1;     //接线柿
    U32	  F31_D2			:1;     //接线柿
    U32	  F31_D5			:1;     //相序
    U32	  F31_D6			:1;     //相序
    U32	  F31_D7			:1;     //相序
    U32	  AREA_ERR	        :2;     //台区识别结果,山西台区识别有三种状态，未知，本台去，非本台去

    U8    NodeDepth; 	           //级数
    U8    UplinkSuccRatio;
    U8    DownlinkSuccRatio;
    U8    BootVersion;
    
    U8    SoftVerNum[2];
    U16   BuildTime;

    U8    ManufactorCode[2];
    U8    ChipVerNum[2];
    U8	  ManageID[24];

    U8    LogicPhase        :3;     //逻辑相位
    U8    HasRfChild        :1;     //记录是否存在无线链接的子节点
    U8    ModeNeed          :4;
    U8	  ModeID[11];
	
    U16   WhiteSeq          :12;
    U16   MacType           :4;
    U8   CollectNeed;      //是否支持采集功能
    U8	 CurveNeed;
	
	U8	  CCOMACSEQ;
	U8	  OntoOff;
	U8    OfftoOn;
	U8    Offtimes;
	
	U32   Offtime;
    U32	  UP_ERR : 2;    //三相表上报的逆相序
    U32          : 30;
	
	U8    Proxytimes;
	U8    power_off_fg;		//离网感知停电标志
    MANUFACTORINFO_t		ManufactorInfo;         // 18BYTE
}__PACKED DEVICE_TEI_LIST_t;

extern DEVICE_TEI_LIST_t DeviceTEIList[MAX_WHITELIST_NUM];


typedef struct
{
	list_head_t 		link;
	U16   NodeTEI	      ;    //TEI
	U8    MacAddr[6];				//设备MAC地址
}__PACKED DEVICE_LIST_EACH;

DATALINK_TABLE_t DeviceListHeader;


#endif




#if defined(STATIC_MASTER)
extern U16   ChildStaIndex[1000]; //遍历子节点时使用
extern U16   ChildCount;

#endif


U8  LEGAL_CHECK(U32 data,U32 downthr,U32 upthr) ;

U8 checkAccessControlList(U8* SourceAddr, U8 Status);
U8 checkAccessControlListByTEI(U16 tei, U32 SNID, U8 Status);

int8_t datalink_table_add(DATALINK_TABLE_t * head, list_head_t *new_list);
int8_t datalink_table_del(DATALINK_TABLE_t * head, list_head_t *new_list, U8 freeFlag);

U8 checkNNIB();

void ClearINIT(void);
void ClearNNIB(void);

U32 GetSNID(void);
void SetPib(U32 snid,U16 tei);
void SetNodeState(U8 state);
U8 GetNodeState();
void SetWorkState(U8 state);
U8 GetWorkState();
void SetSnidLkTp(U8 state);
U8 GetSnidLkTp();
void SetNodeType(U8 state);
U8 GetNodeType();
U8* GetNnibMacAddr(void);
U8* GetCCOAddr(void);
U16 GetTEI(void);
void SetTEI(U16 tei);
U16 GetProxy(void);
void SetProxy(U16 sfotei);

void SetNetworkflag(U8 state);

U8 GetNetworkflag();
void SetnnibDevicetype(U8 type);
U8 GetnnibDevicetype();
void SetnnibEdgetype(U8 type);
U8 GetnnibEdgetype();
U8 GetPowerType();

U16 GetRoutePeriodTime();
void SetRoutePeriodTime(U16 sec);
U16 GetNextRoutePeriodTime();
void SetNextRoutePeriodTime(U16 sec);
U8 GetBeaconTimeSlot();

void setHplcBand(U8 bandID);
U8   GetHplcBand();

void setHplcTestMode(U32 mode);
U32  getHplcTestMode();

void setPossiblePhase(U8 phase);
U8   getPossiblePhase();


void	QueryMacAddrRequest();

#if defined (STATIC_MASTER)
U8 ChildMaxDeepth;

/********************************设备列表**********************************/

void SetDeviceList(U32     SNID,U8 *SrcMacAddr,U16 TEI);

U8 SaveModeId(U8 *macaddr , U8* info ); //release
U16 SearchTEIDeviceTEIList(U8 *pMacAddr);//release
U8 Set_DeviceList_AREA(U16 TEI , U8 DATA);
U8 get_devicelist_area(U16 tei, U8* data);
U8 Set_DeviceList_COLLECT(U16 Seq , U8 DATA);

U8 set_device_list_power_off(U16 Seq);
U8 reset_device_list_power_off(U16 Seq);


void Reset_DeviceList_AREA_ERR();

U8 SaveEdgeinfo(U16 TEI,U8 Edgeinfo);//release
void SetNodeMachine(U8 OldState, U8 NewState);

U16 SelectAppointProxyNodebyLevel(U8 Seq,U8 level);
U16 SelectAppointHighstlevelSTANode();

U16 SelectAppointNewSTANode(U16 Seq);
U16 GetPhaseParm(U16 *A_Num,U16 *B_Num,U16 *C_Num);
U16 SelectAppointSTANode(U16 Seq);
U16 AssignTEI(U8 *pMacAddr);
U16 DT_AssignTEI(U8 *pMacAddr);

U16 SearchAllDirectSTA(U16 TEI , U8 *pteiBuf);
U8 CheckErrtopology(U16 NewSta);

void DelWhiteListCnmAddr(U8 *CmdAddr,U16 WhiteSeq);
void AddWhiteListCnmAddrbyTEI(U8 *MeterAddr,U16 WhiteSeq,U16 TEI,U8 Devicetype,U8 MacType);

U8 CheckTheTEILegal(U16 StaTEI);
U8 CheckTheMacAddrLegal(U8 *pMacAddress,U8 devicetype,U8 addrtype,U16 *WhiteSeq);
U16 CountOfUseMyParentNodeAsParent(U16 parentTEI);
U8  CountproxyNum(U16 TEI);

//void SearchAllChildStation(U16  StaTEI);

void DT_SearchAllChildStation(U16   			StaTEI);

U8 CalculatePHASE(U16 TEI,U32 ntb ,U32 *staNtb,U8 numa,U8 numb,U8 numc,U8 *Phase);

//取设备列表个数
U16 GetDeviceNum(void);
//app层获取设备列表数量，过滤不在白名单设备（二采有地址模式/中继器）
U16 GetDeviceNumForApp(void);
//通过序号获取一条完整的device条目信息
U8 Get_DeviceList_All(U16 Seq , DEVICE_TEI_LIST_t* DeviceListTemp);

//通过meteraddr获取有一条完整的device条目信息
U8 Get_DeviceList_All_ByAdd(U8 *meteraddr , DEVICE_TEI_LIST_t* DeviceListTemp);

//查询的对应的SEQ,网表中是否存在
U8	Get_DeviceListVaild(U16 Seq);//release

void DeviceListHeaderINIT();
void DelDeviceListEach(U16 tei);
void CleanDevListlink();
U8 set_device_list_white_seq(U16 Seq,U16 WhiteSeq);

void getStaPcoNum();

U8 CheckHasRfChild(U16 TEI);

U8 GetRfConsultEn();
void SetRfConsultEn(U8 en);

void SetNetworkType(U8 type);
U8 GetNetworkType();

#endif


#if defined (STATIC_NODE)

/******************************STA自身的在线离线管理**************************************/
void modify_LeaveIndTimer(U32 first);
extern ostimer_t	 *JudgeProxyTypetimer;
/**********************************代理变更回复超时使用**********************************************/
extern ostimer_t	*g_WaitChageProxyCnf; //等待代理变更回复
void modify_mgm_changesendtimer(U32 frist);


 /*函数名称：从模块判断自身是否要离线
a)STA站点在加入网络后，如果在一个完整的心跳周期（两个路由周期）内，收不到任何信标帧；
b)STA站点在连续4个路由周期内，如果与代理站点的通信成功率为0；
c)STA站点收到CCO的“组网序列号”与自身记录的“组网序列号”不同；（信标中实现）
d)STA站点接收到离线指示报文，指示自己离线；
e)一级STA站点，如果检测到CCO的MAC地址发生变化，且已经连续一个周期；（未按照标准，目前检测CCO地址变化时,上报网络冲突，CCO为macadd未变SNID变化）
f)STA站点发现本站点的代理站点角色变为了发现站点已经连续一个路由周期（未实现）
g)本站点的层级超过最大层级限制（15级），站点需要离线。(信标中实现)

h)自己增加的保護策略
 */
void STA_JudgeOfflineBytimer();

/*********************************网络维护相关函数**********************/
void StartNetworkMainten(U8 flag);

void InitHistoryPlanRecord();
U8 STA_JudgeOfflineBybeacon(U8 FormationSeqNumber, U8 *CCOmacaddr, U16 scrtei, U16 level , U8 beacontype, mbuf_t *buf);


///////////////////////////STA的心跳管理////////////////////////////////

//心跳管理启动
void UpDataHeartBeat();

void Sta_Show_NbNet();
void STA_UpDataNeighborNetTime();

 void AddNeighborNet(mbuf_t *buf,U8 type);
void updateNeiNetRfParam(U32 snid, U8 option, U16 channel);


void FreeBadlinkNid(U8 bandidx);
void FreeHRfBadLinkNid();

//设置邻居网络的属性，ErrCode !=0时，需要添加重新关联的时间
void AddBlackNeigbor(U32 SNID, U32 ReassociaTime, U8 Reason);

U8 CheckBlackNegibor(U32 SNID);

U8 CheckNbNetHPLCBaud(U32 SNID);
U8 CheckNbNetHRfBaud(U32 SNID);

//U8 GetNeighborNetNum();
U8 GetPLanAinfo();



void scanStopTimer(uint8_t event);

#endif

/*函数名称：启动列表维护
数据链路层初始化时，调用此函数，启动列表维护
*/
void StartDatalinkGlobalUpdata();

/********************************Neighbornet func**********************************/
#if defined(STATIC_MASTER)
U16 GetChildStaBmpByteNum(U8 *pStaBmpBuf);
//判断对方是否能听到我
U8 NeighborNetCanHearMe(U32 NbNetID,U32 WithNid);
//无线信道冲突处理函数
void StartRfChlChgeForCCO();
//更新邻居网络主节点地址
void UpdateNbNetInfo(U32 snid, U8 *ccoMac, U8 rfoption, U8 rfchannel);
#endif

void StopNetworkMainten();

U32 UpdataNeighborNet(U32 SNID);
void SearchMacInNeighborDisEntrybyTEI_SNID(U16 TEI , U32 SNID ,U8 *pMacAddr);
U16 GetNeighborBmp(U8 *OnlineTEIbmp, U16 *ByteNum);
U16 GetNeighborBmpByteNum();
ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *GetNeighborByTEI(U16 TEI , U8 linkType);


/*********************************Neighbornet list********************************/


/********************************一跳节点根据信标接收增益评估逻辑相位***********************/
#define MAX_ASSESS_CNT   20
typedef struct {
    S16 rgain_a;
    S16 a_cnt;

    S16 rgain_b;
    S16 b_cnt;

    S16 rgain_c;
    S16 c_cnt;
}__PACKED BCN_RGAIN_INFO_t;

typedef struct {
   S8 avge_a;
   S16 a_cnt;
   S8 avge_b;
   S16 b_cnt;
   S8 avge_c;
   S16 c_cnt;
}__PACKED RGAIN_AVGE_INFO_t;

typedef struct {
    BCN_RGAIN_INFO_t rgain;
    RGAIN_AVGE_INFO_t avge;
}__PACKED BCN_RGAIN_ASSESS_t;

extern BCN_RGAIN_ASSESS_t gBcnRgainInfo;
U8 GetBcnAccessPhase(S8 gain, U8 phase, U8 BeaconPeriodCount);




void UpdateNbInfo(U32 SNID, U8 *MacAddr, U16 NodeTEI, U8 NodeDepth, U8 NodeType, U16 Proxy, U8 Relation, S8 GAIN, U16 Phase, U8 BeaconPeriodCount, U8 Frametype);

U16 GetNeighborCount();


U8 IsInNeighborDiscoveryTableEntry(U16 TEI );

U16 SearchBestBackupProxyNode(U16 LASTTEI);
U8 SearchFiveBackupProxyNodeAtEvaluate(PROXY_INFO_t *BackupProxyNode,U8 Reason, U8 proxyNum);
//U8 SearchFiveBackupRfProxyNodeAtEvaluate(U8 *BackupProxyNode,U8 Reason);

U8 ForAssociateReqSearchProxyNode(U16 *BackupProxyNode, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *Pdiscry);

void	SetMacAddrRequest(U8 *MacAddr, U8 DeviceType,U8 MacType);

U8 SearchDepthInNeighborDiscoveryTableEntry(U16 TEI, U8 LinkType);
void UpDataNeighborRelation(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry, U16 proxyTEI, U8 Level,U8 UplinkRatio,U8 DownlinkRatio);
int8_t NeighborDiscovery_link_init(void);

void DelDiscoveryTableByTEI(U16 TEI);
void CleanDiscoveryTablelink();

/********************************无线邻居表维护接口*********************************/
void UpdateRfNbInfo(U32 SNID, U8 *MacAddr, U16 NodeTEI, U8 NodeDepth, U8 NodeType, U16 Proxy, U8 Relation, S8 RSSI, S8 SNR, U8 BeaconPeriodCount, U8 rfCount, U8 Frametype);
void UpDataRfNeighborListTime();
void UpDataRfNeighborRelation(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry, U16 proxyTEI, U8 Level, U8 UplinkRatio, U8 DownlinkRatio);

/********************************router table********************************/
U8 ifHasRoteLkTp(U8 linkType);
U8 getRouteTableLinkType(U16 tei);
U8 UpdateRoutingTableEntry(U16 DestTEI, U16 NextHopTEI, U8 RouteError, U8 RouteState, U8 LinkType);
//在路由表中寻找下一跳
U16 SearchNextHopAddrFromRouteTable(U16 DestAddress,U8 Aactive);
void RepairNextHopAddr(U16 NexTEI, U8 linkType);
//路由表中下一跳失效
void RepairNextHopAddrInActive( U16 NexTEI);
U8 AddErrorCounts(U16 nextHopAddr);
void route_activelik_clean(U16 tei, U8 LinkType);
void route_table_invalid(U16 tei);
U8  IsInRouteDiscoveryTable(U16 byRouteRequestId, U16 DstTEI);
S8 IsLessRelayInRreqCmd(U16 byRouteRequestId, U16 DstTEI, U8 RemainRadius);
U8 AddRoutingDiscoveryTableEntry(ROUTING_DISCOVERY_TABLE_ENTRY_t *byTableEntry);
void UpDataRoutingDiscoveryTableEntry();
void *GetRouteDiscoveryTableEntry(U16 RouteRequestID);
U16	 GetRouteIDBySrcTEI(U16 SrcTEI);
U8 DleteRouteDiscoveryTableEntry(U16 RouteRequestId);



/**************************************AODV***********************************/
U8 UpdateAodvRouteTableEntry(ROUTE_TABLE_RECD_t *pTableEntry, U8 dir);
U16 SearchNextHopFromAodvRouteTable(U16 DestAddress, U8 Status);

#if defined(STATIC_MASTER)
/*********************** CCO重新组网使用 *********************/
extern ostimer_t *Renetworkingtimer;
void RenetworkingtimerCB(struct ostimer_s *ostimer, void *arg);




void IncreaseFromatSeq();


/************************CCO 网络冲突处理*********************/
extern ostimer_t g_ChangeSNIDTimer;
void ChangeSNIDTimerCB(struct ostimer_s *ostimer, void *arg);
void ChangeSNIDFun(struct ostimer_s *ostimer, void *arg);

/***********************无线信道冲突处理**********************/
extern ostimer_t *g_ChangeRfChannelTimer;
void ChangeRfChannelTimerInit();
extern void ChangeRfChannelTimerCB(struct ostimer_s *ostimer, void *arg);
extern void ChangeChannelPlcTimerCB(struct ostimer_s *ostimer, void *arg);
#endif

/*******************CCO切相位使用*****************************/
U8 getTeiPhase(U16 tei);
U8 getNextTeiPhase(U16 tei);
void setSwPhase(U8 sw);

/*****************无线发现列表发送定时器*****************************/
ostimer_t	*RfDiscoverytimer;
__SLOWTEXT void StartRfDiscovery();

#if defined(STD_DUAL)
void setHplcOptin(U8 option);
void setHplcRfChannel(U32 rfchannel);
U8 getHplcOptin();
U32 getHplcRfChannel();
/**
 * @brief               检查无线信道参数合法性
 * 
 * @param option        option  1-3
 * @param channel       channel： option 1:信道号 1-40; option 2:信道号 1-80; option 3:信道号 1-200
 * @return U8           TRUE:参数合法； FALSE：参数不合法
 */
U8 checkRfChannel(U8 option, U16 channel);
#endif

//void modify_post_delay_timer(U8 first, work_t* work);

/*************************CCO自动规划时隙保护定时器****************************************/
//#if defined(STATIC_MASTER)
//ostimer_t *SendBenconPeriodTimer;
//void modify_SendBenconPeriodTimer(uint32_t ms);
//#endif

#endif

