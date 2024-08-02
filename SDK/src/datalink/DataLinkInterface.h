
#ifndef __DATALINK_INTERFACE_H__
#define __DATALINK_INTERFACE_H__

#include "types.h"
#include "mbuf.h"
#include "ZCsystem.h"


#define    BROADCAST_TEI						0XFFF
#define    MAC_PROTOCOL_VER       			0   //MAC版本号 
#define    GET_TEI_VALID_BIT(X)     ((X) & BROADCAST_TEI) // #define    GET_TEI_VALID_BIT(X) (X)
#define    IS_BROADCAST_TEI(x)      ((GET_TEI_VALID_BIT(x) == BROADCAST_TEI) ? TRUE : FALSE)
#define    IS_VALID_TEI(x)      (((GET_TEI_VALID_BIT(x) >= 2) && (GET_TEI_VALID_BIT(x) <= (MAX_WHITELIST_NUM+2)) ? TRUE : FALSE))


#define MAXFILTER_NUM 40
#define   MASKTIME 40



//APP接口//////////////////////////////

#define   CTRL_TEI_GW      0xffe
#define   CTRL_TEI_BJ   0xffd

typedef struct
{
    U16     dtei            :12;
    U16     MacAddrUseFlag  :1;
    U16     Res1            :3;
    U16     Res2;
    
    U32 	Handle;

    U8      SendType;
    U8      LID;
    U16     MsduLength;
    
    U8      DstAddress[MACADRDR_LENGTH];   // The individual device address from which the MSDU src
    U8      SrcAddress[MACADRDR_LENGTH];   // The individual device address from which the MSDU originated

    U8      Msdu[0];
} __PACKED MSDU_DATA_REQ_t;




typedef struct
{
    U16     MsduLength;   // The number of octets comprising the MSDU being indicated
    U16     Stei;

    U8      SendType;
    U8      Res1;
    U16     Res2;

	U32     SNID;
    U8      DstAddress[MACADRDR_LENGTH];   // The individual device address from which the MSDU src
    U8      SrcAddress[MACADRDR_LENGTH];   // The individual device address from which the MSDU originated

    U8      Msdu[0];       // The pointer to the set of octets comprising the MSDU being
} __PACKED MSDU_DATA_IND_t;


typedef struct
{
    U32    MsduHandle;
    
    U8     Status;
    U8     Res1;
    U16    Res2;
} __PACKED MSDU_DATA_CFM_t;


typedef struct{
	U8  addr[6];

}PHASE_POSITION_REQ_t;

typedef struct{
	U8  addr[6];
    U8  phase;
    U8  LNerr;
}PHASE_POSITION_CFM_t;



typedef struct
{
    U8	 Resettimes;
    U8	 LifeTime;
    U16	 DataMsduSeq;
    U16 tei :12;
    U16 res :4;
} __PACKED MSDU_DATA_SEQ_t;

extern MSDU_DATA_SEQ_t MsduDataSeq[MAXFILTER_NUM];



//REQUEST  
void ProcMsduDataRequest(work_t *work);
void ProcSigMsduDataRequest(work_t *work);

//COMFIRM 指针声明,需要app初始化
extern void (*pProcessMsduCfm)(work_t *work);



//函数指针,INDCATION
extern void (*pProcessMsduDataInd)(work_t *work);

// 数据链路层相位识别完成确认
extern void (*pPhasePositionCfmProc)(work_t *work);   


typedef enum
{
  TO_ONLINE,
  TO_OFFLINE,
  TO_WORKFINISH,
  TO_NET_INIT_OK
} ACTION_TYPE_e;
  
 extern  void (*pNetStatusChangeInd)(work_t *work); //节点在数据链路层的状态发生变化时，通知APP


typedef struct
{

    U8    MacAddr[6];
    U8 	   DeviceType;
    U8 	   MacType;
    
} __PACKED S_MACADDR_REQUEST_t; //SET网络MAC地址和硬件设备类型设置和查询

#define		MAXLEVELNODE					50

typedef struct
{
    MDB_t   Mdb;
    U8		LevelNodeNum;

} __PACKED DEL_WHILTH_REQUEST_t;


typedef struct
{
    MDB_t   Mdb;

    U8		LevelNodeNum;	//如果是初始化档案，此值填AA
    U8		NodeMacAddr[MAXLEVELNODE][6];

} __PACKED DEL_WHILTH_LIST_t;

extern  DEL_WHILTH_LIST_t	DelWhilthList;


#define		MAXLEVELNODE					50
#define 	MAXLEVELCNT 					6 //自动删除时，发送离线指示最大轮次

////////////////////协议中mac层结构////////////////////////////////
//路由模式
typedef enum
{
    e_MainRoute = 0  ,
    e_AODVRoute        	 ,
    e_FristBackupRoute 			 ,
    e_SecondBproute,
    e_Broadcast,
   
} ROUTE_TYPE_e;



//信标周期中的时隙类型
typedef enum
{
    e_BEACON_SLICE = 0  ,
    e_CSMA_SLICE        	 ,
    e_TDMA_SLICE 			 ,
    e_BIND_CSMA_SLICE
} SLICE_TYPE_e;



typedef enum
{
    e_NETMMSG	= 0 ,   //网络层管理消息
    e_EXPAND      = 1 ,   //数据链路层扩展
    e_APS_FREAME	= 48 ,   //aps报文
    e_IP_FREAME		= 49 ,   //以太网报文
} MSDU_TYPE_e;


//发送类型
typedef enum
{
    e_UNICAST_FREAM = 0  ,
    e_FULL_BROADCAST_FREAM_NOACK ,
    e_LOCAL_BROADCAST_FREAM_NOACK  ,
    e_PROXY_BROADCAST_FREAM_NOACK  ,

} SEND_TYPE_e;

//代理主路径标识
typedef enum
{
    e_UNUSE_PROXYROUTE = 0  ,
    e_USE_PROXYROUTE
} PROXYROUTE_FLAG_e;

//路径修复标识
typedef enum
{
    e_NO_REPAIR  = 0  ,
    e_HAS_REPAIR
} REPAIR_FLAG_e;



//广播方向
typedef enum
{
    e_TWO_WAY  = 0  , //双向
    e_CCO_STA  ,     //下行广播
    e_STA_CCO
} BROADCAST_DIRECTION_e;


//MACADDR使用标识
typedef enum
{
    e_UNUSE_MACADDR = 0  ,
    e_USE_MACADDR
} MACADDR_USE_FLAG_e;

//MAC帧类型
typedef enum
{        
	LONG_MACHEADER =0  ,   
	SHORT_MACHEADER 
}MACHEADER_TYPE;

//MAC帧类型
typedef enum
{
    e_MAC_VER_STANDARD =0  ,   //标准帧
    e_MAC_VER_SINGLE           //单跳帧，只有无线支持
}MACHEADER_VER;


#if defined(STD_DUAL)
typedef enum
{
    e_RF_DISCOVER_LIST	= 0   ,   //发现列表消息
    e_MME_EXPAND        = 1   ,   //保留（用于管理消息类型扩展）
    e_RF_APS_FREAME	    = 128 ,   //aps报文
    e_RF_IP_FREAME	    = 129 ,   //IPV4 报文
} RF_MSDU_TYPE_e;
//单跳MAC帧头格式
typedef struct {
    U32 	MacProtocolVersion     :4;
    U32 	                       :4;
    U32 	MsduType               :8;
    U32 	MsduLen                :11;
    U32 	                       :5;

    U8      Msdu[0];
}__PACKED MAC_RF_HEADER_t;
#endif

//MAC短帧头结构
typedef struct
{

	U32 	MacProtocolVersion: 	4;
	U32 	ScrTEI: 				12;
	U32 	DestTEI:				12;
	U32 	SendType:				4;				//发送类型,广播、单播、ACK

	U32 	MaxResendTime:			5;
	U32 	Reserve:				3;
	U32 	MsduSeq:				16; 			//MSDU序号
	U32 	MsduType:				8;

	U32 	MsduLength_l:			8;
	U32 	MsduLength_h:			3;
	U32 	RestTims:				4;
	U32 	ProxyRouteFlag: 		1;			   //代理路径使用标志
	U32 	Radius: 				4;				//半径
	U32 	RemainRadius:			4;				//剩余半径
	U32 	BroadcastDirection: 	2;			//广播方向
	U32 	RouteRepairFlag:		1;			//路径修复标志
	U32 	MacAddrUseFlag: 		1;			//MAC地址标志
	U32 	Reserve1:				4;

	U32 	Reserve2:				8;
	U32 	FormationSeq:			8;		//组网序列号
	U32 	Reserve3:				8;
	U32 	Reserve4:				8;		//组网序列号

	U8		Msdu[0];
} __PACKED MAC_PUBLIC_HEADER_t;
//MAC长帧头结构
typedef struct
{
    MAC_PUBLIC_HEADER_t    MacPublicHeader;
	
    U8          SrcMacAddress[6];                //源MAC地址
    U8          DestMacAddress[6];               //目的MAC地址


    U8          Msdu[0];
} __PACKED MAC_LONGDATA_HEADER;


typedef struct{
    U16 delayTime;
    U8  StaNum;
    U8  DelType;
    U8  pMac[0];
}__PACKED LEAVE_IND_t;


// 

typedef struct
{
    U32    PacketPort         : 8;
    U32    PacketID            : 16;
    U32    PacketCtrlWord  : 8;
} __PACKED L_APS_HEADER_t;


event_t mutexNetShareProt_t;

#define  WHLPTST  mutex_pend(&mutexNetShareProt_t, 0);
#define  WHLPTED  mutex_post(&mutexNetShareProt_t);


void ProcPhaseDistinguishRequest(work_t *work);

void ProcMPDUDataIndication(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc, work_t *work);
//void ProcSackDataIndication(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc);
//void ProcBeaconIndication(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc);
void ProcPhyCoordIndication(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc);
U32 GetBeaconOffset();
U8 comcmpMac(U8 *pMac1, U8 *pMac2);

#define  NEW_CCORD_ALG

void Trigger2app(U8 state);

void ProcMpduDataConfirm(work_t *work);
//void ProBeaconConfirm(work_t *work);
//void ProCoordConfirm(work_t *work);
void MacDataRequst(MAC_PUBLIC_HEADER_t *pMacData, U16 MacLen, U16 Nextei,
                        U8 SendType, U32 Handle, U8 LinkId, U8 MSDUType, U8 ResendFlag, U16 DelayTime,U8 RouterMode,U8 DatalinkPhase);

//指定数据发送到载波或是无线
void MacDataRequstRf(MAC_PUBLIC_HEADER_t *pMacData, U16 MacLen, U16 Nextei,
                             U8 SendType, U32 Handle, U8 LinkId, U8 MSDUType, U8 ResendFlag, U16 DelayTime, U8 RouterMode);

void ProcLeaveIndRequset(work_t *work);


#if defined(STATIC_MASTER)
U32 getNbForCoord();
#endif

void CleanTimeSlotInd(work_t *work);
void CleanTimeSlotReq();


//mac to net
void link_layer_beacon_deal(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc32, work_t *work);
void link_layer_sof_deal(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc, work_t *work);
void link_layer_coord_deal(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc, work_t *work);
void link_layer_sack_deal(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc, work_t *work);

void ProcSackCtrlRequest(U8 dt, U8 *addr,U8 linktype);

#endif

