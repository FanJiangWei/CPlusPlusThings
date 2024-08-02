#ifndef    _Beacon_h
#define    _Beacon_h

#include "types.h"
#include "mbuf.h"
#include "ZCsystem.h"
#define	MAX_BEACONA_LEN  520


#define  GD_STA_ABILITY_ENTRY_LENGTH   0X16
#define  GD_ROUTE_PARM_ENTRY_LENGTH    0X22
#define  STA_ABILITY_ENTRY_LENGTH   15
#define  ROUTE_PARM_ENTRY_LENGTH    10

#define CenterSlotCount     							3  //中央信标占用的时隙数


#define BandCsmaSliceLength  							0 //
#define BEACONPERIODLENGTH  						    2000//0x7d0//0x1338//5s,???MS  初始周期和不允许关联周期
#define BEACONPERIODLENGTH1  						    5000//0x7d0//0x1338//5s,???MS  JOIN_most状态

#define BEACONPERIODLENGTH_S  					(BEACONPERIODLENGTH/1000)//???S
#define RoutePeriodTime_FormationNet     			100 //?????????
#define RoutePeriodTime_Normal           			100//0x64//?????????
#define RoutePeriodTime_Test           			20//0x64//?????????

#define RoutePeriodTime_150           			150//0x64//?????????
#define RoutePeriodTime_100_300           		200//0x64//?????????
#define RoutePeriodTime_300_500           		300//0x64//?????????
#define RoutePeriodTime_500           			400//0x64//?????????

#define CSMA_SLOT_SLICE       							50 //10Ms unit


#define JOIN_CSMA_LEN 						    	3420//??????????
#define NORMAL_CSMA_LEN 						    (5500 + 6000)//5500//?????CSMA?????
#define NORMAL_CSMA_LEN_3 						    (3000 + 3000)//5500//?????CSMA?????
#define NORMAL_CSMA_LEN_2 						    (2000 + 1000)//5500//?????CSMA?????

#define MINCSMASLICE								1200

#define MAXSLICE								    (14500)

#define LOWPOWER_MAXSLICE								    (10000)

#define ONLINETIME									RoutePeriodTime_Normal*2//?????2?????
#define OFFLINETIME								RoutePeriodTime_Normal*8//?????8?????
#define RoutePeriodTimeMaxSendBeaconCount  (RoutePeriodTime_Normal/BEACONPERIODLENGTH_S)
#define	MaxPayloadSoltNum							   207//215 长度预留了功率和频率切换的信标条目
#define	MAXPCONUM									   188//195


#define NoRelationTimes								   1 // 不允许关联次数
#define MaxSendBeaconCountWithoutRespond  			   1//	最大未关联入网信标周期

#define	AFTER_FINEWORK_COUNT 							30
#define	STAMPUNIT 										25000



extern U32   NwkBeaconPeriodCount;//网络层记录的信标周期
extern U32   SimBeaconPeriodCount;//网络层记录的精简信标周期
extern U8	 g_BeaconSlotTime;
extern U8    g_RelationSwitch;

U8 			 g_SendCenterBeaconCountWithoutRespond; //确定是否还有能组进网的节点，当其值超过最大限制，确定组网动作已经完成

extern ostimer_t					 g_SendCenterBeaconTimer;

//时隙分配使用的全局参数结构体
typedef struct{

    U32 beacon_start;

    U16 beacon_slot;
    U16 beacon_slot_nr;

    U16 csma_slot_a;
    U16 csma_slot_b;

    U16 csma_slot_c;
    U16 csma_time_slot;

    U8  RfBeaconType;     //cco 发送无线信标类型： 标准：e_STRAD_BEACON e_SIMP_BEACON 精简：e_SIMP_BEACON
    U8  NonCenterCnt;
    U8  RfBeaconSlotCnt;
    U8  CCObcn_slot_nr;   //中央信标时隙数量。

    U8 *pNonCenterInfo;

}__PACKED TIME_SLOT_PARAM_t;
extern TIME_SLOT_PARAM_t g_TimeSlotParam;
extern U32 BeaconStopOffset;

//数据帧接收指示
//typedef struct
//{
//    wpbh_t  pbh;
//    U8	    LinkId;
//    S8    gain;
//    U16    DesTEI;
//    U16    UpTEI;
//    U32	   snid;
//    U8	   ResendFlg;
//    U16    mpduLength;
//    U8     Mpdu[0];
//} __PACKED MACDATA_INDICATION_t;
//信标帧接收指示
typedef struct
{

    U32	 SNID;//sender SNID
    U16	 UpTEI;
    U8	 Phase;//
    S8    gain;
    U16    beaconLength;
    U8     beacondata[0];
} __PACKED BEACON_INDICATION_t ;

//网络协调帧接收指示
typedef struct
{
    U32	    SNID;
    U32     NbNet;					//已经占用的邻居SNID
    U16     Duration;				//邻居信道持续时间
    U16     SendStartOffset;			//带宽开始偏移  //
    U16     MyDuration;				//本信道持续时间
    U16     MyStartOffset;			//带宽开始偏移  //
} __PACKED COORD_INDICATION_t ;

//SACK指示
typedef struct
{
    U8   	AckFlag;//ACK / NOACK
    U8        Status;
    U32	   	SNID;
    U16 		SrcTEI;
    U16 		DesTEI;
} __PACKED SACK_INDICATION_t ;



typedef enum
{
    e_STA_NODETYPE = 0x01,
    e_PCO_NODETYPE = 0x02,
    e_CCO_NODETYPE = 0x04,
    e_UNKNOWN_NODETYPE = 0x0f,
} NodeType_e;


typedef enum
{
    e_DISCOVERY_BEACON = 0  ,
    e_PCO_BEACON        	 ,
    e_CCO_BEACON 			 ,
} BEACON_TYPE_e;

typedef enum
{
    e_STA_ABILITY_TYPE              = 0x00,     //站点能力
    e_ROUTE_PARM_TYPE               = 0x01,     //路有参数
    e_BAND_CHANGE_TYPE              = 0x02,     //切换band
    e_RF_ROUTE_PARM_TYPE            = 0X03,     //无线路由参数条目
    e_RF_CHANNEL_CHANGE_TYPE        = 0X04,     //无线信道变更条目
    e_SIMPBEACON_ABILITY_TYPE       = 0X05,     //精简信标帧站点信息及时隙条目
    e_LINKMODE_SET_ENITY            = 0xB1,     //链路设置条目
    e_SLOT_ALLOCATION_TYPE          = 0xC0,     //时隙分配条目
    e_AREA_IDENTITY,
    e_UNLOCK_ENTRY                  = 0xBF,     //
    e_POWERLEVELENITY               = 0xE1,     //功率设置条目
    
} BEACON_ENTRY_TYPE_e;
typedef enum
{
    e_STRAD_BEACON = 0,         //标准信标帧
    e_SIMP_BEACON,              //精简信标帧
}BEACON_FLAG_e;

//无线信标发送类型
typedef enum
{
    e_BEACON_HPLC                   = 0x00,
    e_BEACON_RF                     = 0x01,
    e_BEACON_HPLC_RF                = 0x02,
    e_BEACON_HPLC_RF_SIMPLE         = 0x03,
    e_BEACON_HPLC_RF_SIMPLE_CSMA    = 0x04,
}RF_BEACON_TYPE_e;



#if defined(STD_DUAL)
//精简信标帧载荷
typedef struct
{

    U8      beaconType:         	3;				//信标类型
    U8      Networkflag: 			1;				//组网标志位
    U8      SimpBeaconFlag:         1;              //精简信标标志
    U8		Reserve:				1;              //保留
    U8      Relationshipflag:		1;              //开始关联标志位
    U8      UseBeaconflag:			1;              //信标使用标志位
    U8      Networkseq;                             //组网序列号
    U8		CCOmacaddr[MACADRDR_LENGTH];            //CCO MAC 地址
    U32     BeaconPeriodCount;                      //信标周期计数
    U8		BeaconEntryCount;                       //信标条目总数量
} __PACKED RF_BEACON_HEADER_t;
//无线路由参数条目
typedef struct
{
    U8   BeaconEntryType;                           //信标条目头（条目类型）
    U8   EntryLength;                               //信标条目长度

    U8   DiscoverPeriod;                            //无线上发现列表周期长度， 单位：1s
    U8   RfRcvRateOldPeriod;                        //无线接收率老化周期个数， 单位：无线发现列表周期。
} __PACKED RF_ROUTE_PARM_STRUCT_t;

//无线信道变更条目
typedef struct
{
    U8   BeaconEntryType;
    U8   EntryLength;

    U8   RfChannel;                                 //目标信道
    U32  ChannelChangeRemainTime;                   //信道切换剩余时间 单位：1ms
    U8   RfOption                   :2;             //目标无线信道的option模式
    U8                              :6;             //保留
} __PACKED RF_CHANNEL_CHANGE_STRUCT_t;

//精简信标站点能力及时隙条目
typedef struct
{
    U8   BeaconEntryType;
    U8   EntryLength;

    U32  Tei                        :12;            //站点TEI
    U32  ProxyTei                   :12;            //代理站点TEI
    U32  NodeType                   :4;             //角色
    U32  Level                      :4;             //层级

    U8   MacAddr[6];                                //站点MAC地址
    U8   RfCount                    :4;             //链路上RF跳数
    U8                              :4;             //保留

    U32  CsmaSlotStart;                             //CSMA时隙开始时间
    U16  CsmaSlotLength;                            //CSMA时隙长度
}__PACKED RF_ABILITY_SLOT_t;


typedef struct
{
    list_head_t link;
    U16 TEI          :12;
    U16 BeaconType   :1;
    U16 RfBcnType    :3;
    U16 ParentTei    :12;
    U16 Linktype     :1;
    U16 ModuleType   :3;

    U8  rfBcnLen;  //计算无线信标帧长占用几个载波时隙
    U8  res[3];

}__PACKED NoCenterNodeInfo_t;
#endif

typedef struct
{

    U8      beaconType:         	3;				//信标类型
    U8      Networkflag: 			1;				//组网标志位
    U8      SimpBeaconFlag:         1;              //精简信标标志
    U8		Reserve:				1;              //保留
    U8      Relationshipflag:		1;              //开始关联标志位
    U8      UseBeaconflag:			1;              //信标使用标志位
    U8      Networkseq;                             //组网序列号
    U8		CCOmacaddr[MACADRDR_LENGTH];            //CCO MAC 地址
    U32     BeaconPeriodCount;                      //信标周期计数
    U8      RfChannel;                              //无线信道编号
    U8      RfOption               :2;              //无线option
    U8      res                    :6;              //保留
    U8		Reserve1[6]			 ;                  //保留
    U8		BeaconEntryCount;                       //信标条目总数量
} __PACKED BEACON_HEADER_t;

//站点能力条目
typedef struct
{
    U8   BeaconEntryType;                           //信标条目头（条目类型）
    U8   EntryLength;                               //信标条目长度

    U32	 TEI				: 12;                   //站点 TEI
    U32	 ProxyTEI			: 12;                   //代理站点 TEI
    U32	 PathSuccessRate	: 8;                    //路径最低通信成功率

    U8   SrcMacAddr[MACADRDR_LENGTH];               //站点MAC地址


    U8   NodeType          : 4;                     //站点角色
    U8   StaLevel          : 4;                     //站点网络层级

    U8	  WithProxyLQI;                             //与代理站点通信质量

    U8   Phase             	: 2;                    //站点所属相线
    U8   RfCount          	: 4;                    //链路上RF跳数
    U8   Reserve          	: 2;                    //保留
} __PACKED STA_ABILITY_STRUCT_t;

//路由参数条目
typedef struct
{
    U8   BeaconEntryType;                           //信标条目头（条目类型）
    U8   EntryLength;                               //信标条目长度

    U16  RoutePeriod;                               //路由周期
    U16  RouteEvaluateRemainTime;                   //路由评估剩余时间
    U16  ProxyDiscoveryPeriod;                      //代理站点发现列表周期
    U16  StaionDiscoveryPeriod;                     //发现站点发现列表周期
} __PACKED ROUTE_PARM_STRUCT_t;


//频段通知条目
typedef struct
{
    U8   BeaconEntryType;
    U8   EntryLength;

    U8   DstBand;
    U32  BandChangeRemainTime;
} __PACKED BAND_CHANGE_STRUCT_t;


//时隙分配条目
typedef struct
{
    U8   BeaconEntryType;                           //信标条目头（条目类型）
    U16  EntryLength;                               //信标条目长度

    U8   NonCentralBeaconSlotCount;                 //非中央信标时隙总数

    U8   CentralSlotCount		: 4;                //中央信标时隙总数
    U8   CsmaSlotPhaseCount     : 2;                //CSMA时隙支持的相线个数
    U8   Reserve				: 2;                //保留

    U8   Reserve1;

    U8   ProxySlotCount;                            //代理信标时隙个数
    U8   BeaconSlotTime;                            //信标时隙长度        单位1ms
    U8   CsmaSlotSlice;                             //CSMA 时隙分片长度  单位10ms
    U8   BindCsmaSlotPhaseCout;                     //绑定 CSMA 时隙相线个数
    U8   BindCsmaSlotLinkId;                        //绑定 CSMA 时隙链路标识符
    U8   TdmaSlotLength;                            //TDMA 时隙长度
    U8   TdmaSlotLinkId;                            //TDMA 时隙链路标识符
    U32  BeaconPeriodStartTime;                     //信标周期起始网络基准时
    U32  BeaconPeriodLength;                        //信标周期长度        1ms
    U16  RfBeaconLength          :10;               //RF信标时隙长度      1ms
    U16  Reserve2                :6;                //保留
    U8	 info[0];                                   //非中央信标信息
} __PACKED SLOT_ALLOCATION_STRUCT_t;

//非中央信标信息
typedef struct
{
    U16  SrcTei           : 12;                      //指定发送信标的站点的 TEI  低8bit
    U16  BeaconType       : 1;                      //信标类型
    U16  RfBeaconType     : 3;                      //无线信标标志
} __PACKED NON_CENTRAL_BEACON_INFO_t;

//CSMA 时隙信息字段
typedef struct
{
    U32  CsmaSlotLength   : 24;                     //CSMA 时隙的长度 单位： 1 毫秒
    U32  Phase            : 2 ;                     //CSMA 时隙相线
    U32  Reserve          : 6 ;                     //保留

} __PACKED CSMA_SLOT_INFO_t;


//功率设置条目
typedef struct
{
    U8    BeaconEntryType;
    U16   EntryLength;
    U16	  TGAINDIG;
    U16	  TGAINANA;
}__PACKED POWERSET_STRUCT_t;

//链路切换条目
typedef struct
{
    U8    BeaconEntryType;
    U8    EntryLength;
    U8 	  LinkMode;             //0x00：默认0，不进行设置；0x01：单载波工作模式；0x02：单无线工作模式；0x03：双模工作模式。其他：保留
    U16	  ChangeTime;           //unit:s
    U16	  DurationTime;         //unit:s
}__PACKED LINKMODESET_STRUCT_t;



typedef struct
{
  U16 HadSendStaNum;
  U16 ThistimeSendStaNum;
  U16 MaxListenTimes; //listen state use
  U16 CurrentListenTimes; //listen state use

}__PACKED BEACON_SLOT_SCHEDULE;

extern BEACON_SLOT_SCHEDULE	BeaconSlotSchedule_t;


typedef enum
{
    e_JoinIdel = 0 , //组网刚开始
    e_JoinStart    , //模块开始入网
    e_JoinListen 	, //STA安排一轮发现信标之后有新节点入网
    e_JoinFinish_Most, //组网完成，安排一轮STA发送发现信标后没有新节点入网。
    e_JoinFinish_All  //组网完成，e_JoinFinish_Most状态下，路由周期结束置该状态
} MACHINE_e;


typedef struct
{
  U8  	JoinMachine;  //?????????
  U16	LastLevelInNetNum; //??????????
  U16	ThisLevelJoinNum; //?????????


}__PACKED JOIN_CTRL;

extern JOIN_CTRL	JoinCtrl_t;



void ProcBeaconIndication(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc32);



#if defined(STATIC_MASTER)
uint16_t PackCenternBeacon(uint8_t *payload, U8 phase);
uint16_t PackCenternBeaconSimp(uint8_t *payload, U8 phase);
void getSlotInfoForBeacon(TIME_SLOT_PARAM_t *pTimeSlotParam);
#endif


#if defined(STATIC_NODE)

void change_band_timer_init(void);


void SavePowerlevel();

ostimer_t g_ChangeChlTimer ; //切换频段定时器

#endif




#endif


