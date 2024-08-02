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
#define    MaxDeepth  15				//���㼶��

#define AODV_MAX_DEPTH                      14 //�������
#define READMETER_UNICAST_MAX_RETRIES       10//����ҵ������ط�����

#define NWK_MMEMSG_MAX_RETRIES           5 //���������Ϣ�ط�������
#define NWK_UNICAST_MAX_RETRIES		     0 //����㵥������ط�����
#define NWK_LOCALCAST_MAX_RETRIES		 3 //����㱾�ع㲥����ط�����
#define NWK_FULLCAST_MAX_RETRIES         1 //�����ȫ���㲥����ط�����
#define NWK_FULLCAST_DEF_RETRIES         0 //Ĭ��0 ���ײ���֡��Ĭ�ϸ�ֵ3��
#define DELAY_LEAVE_TIME					10 //S

#define   CRCLENGTH 						4

#define   CCO_TEI       					1
#define	  ZERO_COLLOCT_NUM					13

#define  OC_CHANGE_TIMES                    5

extern U16 		g_MsduSequenceNumber;   //MSDU���
U8				   BroadcastAddress[6];

extern U32 g_CnfAssociaRandomNum, g_CnfManageMsgSeqNum;
extern U8 netMsgRecordIndex;

extern U8 ManageID[24];

U8 GlRFChangeOCtimes;


/******************�����������ʹ��*****************/
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

//��������
#if defined(STATIC_MASTER)

extern ostimer_t g_AssociGatherTimer;
extern ostimer_t g_RfAssociGatherTimer; /*���߻��ܶ�ʱ��*/


#endif


typedef enum
{
    e_MMeAssocReq  = 0x0000,                    // ��������
    e_MMeAssocCfm  = 0x0001,                    // �����ظ�
    e_MMeAssocGatherInd  = 0x0002,              // ��������ָʾ
    e_MMeChangeProxyReq  = 0x0003,              // ����������
    e_MMeChangeProxyCnf  = 0x0004,              // �������ظ�
    e_MMeChangeProxyBitMapCnf = 0x0005,         // �������ظ�
    e_MMeLeaveInd        = 0x0006,              // ����ָʾ
    e_MMeHeartBeatCheck  = 0x0007,              // �������
    e_MMeDiscoverNodeList = 0x0008,             // �����б�
    e_MMeSuccessRateReport = 0x0009,            // ͨ�ųɹ����ϱ�
    e_MMeNetworkConflictReport = 0x000a,        // �����ͻ�ϱ�
    e_MMeZeroCrossNTBCollectInd = 0x000b,		// ���� NTB �ɼ�ָʾ
    e_MMeZeroCrossNTBReport = 0x000c,           // ���� NTB �ϱ�
    e_MMeAreaNotifyNTBReport=0x000f,            //ȫ���ȹ���NTB��֪

    e_MMeRouteRequest  = 0x0050,                // ·������
    e_MMeRouteReply = 0x0051,                   // ·�ɻظ�
    e_MMeRouteError = 0x0052,                   // ·�ɴ���
    e_MMeRouteAck = 0x0053,                     // ·��Ӧ��
    e_MMeLinkConfirmRequest = 0x0054,           // ��·ȷ������
    e_MMeLinkConfirmResponse = 0x0055,          // ��·ȷ�ϻ�Ӧ

    e_MMeRfChannelConfilictReport = 0x0080      // �����ŵ���ͻ�ϱ�

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
    e_InitState=0xf,    //��ʼ״̬���ֽڳ���һ��

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
    ASSOC_SUCCESS,                // ��ʾ��������ɹ�
    NOTIN_WHITELIST,              // ��ʾ��վ�㲻�ڰ�������
    IN_BLACKLIST,                 // ��ʾ��վ���ں�������
    STA_FULL,                     // ��ʾ�����վ�������������
    NO_WHITELIST,                 // ��ʾû�����ð������б�
    PCO_FULL,                     // ��ʾ����վ�������������
    CHILD_FULL,                   // ��ʾ��վ�������������
    RESERVE ,                      // ����
    REPEAT_MAC,                   // ��ʾ�ظ��� MAC ��ַ
    DEPTH_OVER,                   // ��ʾ�������˲㼶
    RE_ASSOC_SUCCESS,             // ��ʾվ���ٴι������������ɹ�
    CYCLE_ASSOC,                  // ��ʾ�µ�վ����ͼ���Լ�����վ��Ϊ����������
    LOOP_TOP,                     // ��ʾ���������д��ڻ�·
    CCO_UNKNOWN_ERR,              // ��ʾ CCO ��δ֪ԭ�����
} AssocResult_e;


typedef enum
{
    e_Reset=0, //�����ϵ���߸�λ
    e_FormationSeq, //������Ÿı䵼��
    e_CCOnidChage, //CCOSNID�仯����
    e_LevelErr,   //���ڵ�Ĳ㼶15�����Լ�����
    e_RecvLeaveInd, //���յ�����ָʾ����
    e_NotHearBeacon, //��������û�������ű�
    e_NohearDisList, //4������û���������ڵ�ķ����б�
    e_NodeRouteLoss,
    e_ProxyChgfail,
    e_LevHig,
    e_ProxyChangeRole, //���ڵ��ɫ�任Ϊ���ֽڵ�
    e_reset_dog=20, //���Ź���λ  
    e_reset_power, //�͵�ѹ��λ
    e_reboot,     //������λ
    e_change_linkmode,     //�л���·
    e_NohearRfDisList, //4������û���������ڵ�ķ����б�
    e_elffail,          //CJQ��ַ��ƥ���Զ�����
}REQ_REASON;

//��ʾ������ԭ��
typedef enum
{
    e_CommRateLow =0, //��ԭ���ڵ�ͨ�ųɹ��ʵ�������
    e_NoRecvDisList , //����·������û���յ������б�
    e_ProxyLossInNb,  //���ڵ���ھӱ�����ʧ
    e_FindBetterProxy, //�ҵ��˸��õĸ��ڵ�
    e_KeepSyncCCO,   //���ֺ�CCOһ��
    e_RouteLoss,    //·�ɶ�ʧ
    e_ProxyNoListenme, //���ڵ��������Լ����������ڵ�
    e_ChangeNet,//�������
    e_ElftoElf, //�Լ����Լ�
    e_LevelToHigh, //�㼶�߳�15��
    e_FialToTry   //���ʧ�ܣ�����ʹ�ñ��ݴ���
}PROXYCHANGE_REASON;

typedef enum
{
    e_ColloctOne,
    e_ColloctAll,

} ColloctType_e;

typedef enum
{
    e_HalfPeriod,    // �����ڲɼ�
    e_OnePeriod,     // ȫ���ڲɼ�

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
    e_HPLC_Module = 0,      //HPLC��ģ
    e_HPLC_RF_Module,       //HPLC+RF ˫ģ
    e_RF_Module             //���ߵ�ģģ��
}NODULE_TYPE_e;

typedef struct
{
    U8	RetryTime;
    U16 DelayTime;
    U8  Reason;
    U8	Result; //������

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

//ֱ��վ��·�ɱ���Ϣ
typedef struct
{
    U16   NodeTei  :12;
    U16   LinkType :1;
    U16   res      :3;
}__PACKED NODE_ROUTE_RABLE_t;

//ֱ������վ��·�ɱ���Ϣ
typedef struct
{
    U16   NodeTei  :12;
    U16   LinkType :1;
    U16   res      :3;

    U16   ChildCount;
}__PACKED PROXY_ROUTE_RABLE_t;

//·�ɱ���Ϣ������ȷ����ʹ��
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


//��������ṹ
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
    U8    	ModuleType  :2;         //ģ������  0��HPLC�� 1��˫ģ 2�����ߵ�ģ
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

//����ȷ�Ͻṹ
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U8    	DstMacAddr[6];
    U8    	CCOMacAddr[6];
    U8   	Result;
    U8    	NodeDeep;
    U16    	NodeTEI      :12;
    U16     LinkType     :1;            //��·����    0��HPLC�� 1��HRF
    U16     HplcBand     :2;            //�ز�Ƶ��
    U16     res          :1;
    U16    	ProxyTEI	 :12;   //ccoΪվ�����Ĵ���TEI
    U16    	res2	 :4;        //
    U8    	SubpackageNum;//�ְܷ���
    U8    	SubpackageSeq;//�ְ����
    U32   	AssociaRandomNum;
    U32   	ReassociaTime;//���¹���ʱ��
    U32     ManageMsgSeqNum;//�˵������
    U32   	RouteUpdateSeqNumber;//·�����
    U8		Reserve[1];
    U8		ApplicationID[2];
    U8		Reserve2[1];
    ROUTE_TABLE_INFO_t  RouteTableInfo;
} __PACKED ASSOCIATE_CFM_CMD_t;

//��������ָʾ�ṹ
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8   	Result;
    U8    	NodeDeep;
    U8    	CCOMacAddr[6];

    U16    	ProxyTEI            :12; //cco�Լ���TEI
    U16    	HplcBand            :2;  //�ز�Ƶ��
    U16    	res                 :2;  //����
    U8    	FormationSeqNumber;//�������к�
    U8    	GatherNodeNum;//����վ����

   	U8		Reserve[1];
    U8		ApplicationID[2];
    U8		Reserve2[1];


    U8 		StationInfo[0];
} __PACKED ASSOCIATE_GATHER_IND_CMD_t;


// ������������
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
    U8    	Reserve 	: 2; //���������������ԭ��

    U8     Reserve1[3];
} __PACKED PROXYCHANGE_REQ_CMD_t;


// ������ȷ�ϱ���
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

// ������λͼȷ�ϱ���
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


// ��ʱ����ָʾ����
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16	  Result;
    U16   StaNum;
    U16   DelayTime;
    U8    Reserve[10];

    U8    MacAddress[0];
} __PACKED DELAY_LEAVE_LINE_CMD_t;


// ������ⱨ��
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16	  SrcTEI;
    U16   MostTEI;

    U16   MaxiNum;
    U16   BmpNumber;

    U8    TEIBmp[0];
} __PACKED HEART_BEAT_CHECK_CMD_t;


// ����·����Ϣ
typedef struct
{
    U16		UpTEI:12;
    U16		RouteType:4;
} __PACKED UpLinkRoute_t;


// �����б���
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

    U8	 WithProxyLQI;     //�ʹ���ڵ���ŵ�����
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



// �ɹ�����Ϣ
typedef struct
{

    U16		ChildTEI;
    U8		DownCommRate;
    U8		UpCommRate;
} __PACKED SUCCESSRATE_INFO_t;


// ͨ�ųɹ��ʱ���
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U16		NodeTEI;
    U16		NodeCount;

    U8		ChildInfo[0];
} __PACKED SUCCESSRATE_REPORT_CMD_t;


//�����ͻ�ϱ����ĸ�ʽ
typedef struct
{

    U16		RelayTEI: 12;
    U16		Res: 4;
    U8		CommRate;
    U8		LQI;

} __PACKED RELAY_LIST_t;


//·��������
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

//·�ɻظ�����
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


//·�ɴ�����
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;


    U8		RouteRepairVer;
    U32   	RouteRequestSeq;
    U8		Reserve;
    U8		UnArriveNum ; //���ɵ�?վ??��
    U8		UnArriveList[0];

} __PACKED ROUTE_ERR_t;

//·��?����
typedef struct
{

    MGM_MSG_HEADER_t MmsgHeader_t;

    U8		RouteRepairVer;
    U8		Reserve[3];
    U32   	RouteRequestSeq;
} __PACKED ROUTE_ACK_t;


//��·ȷ��������
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U8		RouteRepairVer;
    U32   	RouteRequestSeq;
    U8		Reserve;
    U8      ConfirmStaNum;
    U8      ConfirmStaList[0];
} __PACKED LINK_CONFIRM_REQUEST_t;

//��·ȷ�ϻ�Ӧ����
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


//�����ͻ�ϱ����ĸ�ʽ
typedef struct
{
    MGM_MSG_HEADER_t MmsgHeader_t;

    U8    CCOAddr[MACADRDR_LENGTH];
    U8    NBnetNumber; //�ھ��������
    U8    NIDWidth;

    U8    NBnetList[0];  //�ھ�������Ŀ

} __PACKED NETWORK_CONFLIC_REPORT_t;


//����NTB�ɼ�ֻ�Ǳ��ĸ�ʽ
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
    U32		FirstNTB;    //�ǲɼ������NTBֵԭʼ32BIT����8���ص�ֵ

    U8		NTBdiff[0];
} __PACKED ZERO_CROSSNTB_REPORT_t;

/************************��������***********************/
//�����ŵ���ͻ�ϱ�
typedef struct
{
    MGM_MSG_HEADER_t   MmsgHeader_t;

    U8 CCOAddr[6];
    U8 NBNetNum;
    U8 NBnetList[0];  //�ھ�������Ŀs
} __PACKED RFCHANNEL_CONFLIC_REPORT_t;

//���߷����б���ء�
enum{
    e_RF_DISCV_STAINFO    = 0,             //վ����Ϣ����
    e_RF_DISCV_ROUTEINFO,                   //վ��·����Ϣ
    e_RF_DISCV_NBCHLINFO_NOBITMAP,          //�ھӽڵ��ŵ���Ϣ��λͼ��
    e_RF_DISCV_NBCHLINFO_BITMAP             //�ھӽڵ��ŵ���Ϣλͼ��
};
//վ��������Ϣ�ṹ��
typedef struct
{
//    U8  unitType            :7;             //��Ԫ����  0��վ��������Ϣ
//    U8  unitLenType         :1;             //��Ԫ����  0��1�ֽ�  1��2�ֽ�
//    U8  unitLen;                            //��Ԫ���ݳ��ȡ� վ����ϢĬ��ʹ��һ���ֽ�

    U8  CCOMAC[6];                          //CCO MAC ��ַ
    U16 ProxyTEI            :12;            //����TEI
    U16 NodeType            :4;             //��ɫ

    U8  NodeLevel           :4;             //�㼶
    U8  RfCount             :4;             //��·RF����
    U8  ProxyUpRcvRate;                     //�������н�����
    U8  ProxyDownRcvRate;                   //�������н�����
    U8  MixRcvRate;                         //��·��С������

    U8   DiscoverPeriod;                    //�����Ϸ����б����ڳ��ȣ� ��λ��1s
    U8   RfRcvRateOldPeriod;                //���߽������ϻ����ڸ����� ��λ�����߷����б����ڡ�
}__PACKED RF_DISC_STAINFO_t;

//�����ŵ���Ϣ������Ϸ�λͼ��
#define  RF_NBINFOTYPELEN_GROUP_COUNT 10
typedef struct{
    U16 TEILen          :4;                 //TEI��Ϣ����             ��λ��bit
    U16 RcvRateLen      :4;                 //��������Ϣ����           ��λ��bit
    U16 SNRAvgLen       :4;                 //ƽ���������Ϣ����        ��λ��bit
    U16 RSSIAvgLen      :4;                 //�ź�ǿ����Ϣ����          ��λ��bit

    U16  InfoLen;                            //�ܳ���                   ��λ��bit
}__PACKED RF_DISC_NBINFOTYPELEN_t;
extern RF_DISC_NBINFOTYPELEN_t g_RfNbInfoTypeLenGroup[RF_NBINFOTYPELEN_GROUP_COUNT];
//�����ŵ���Ϣ�������λͼ��
#define  RF_NBINFOTYPELEN_MAP_GROUP_COUNT 11
typedef struct{
    U8 RcvRateLen      :4;                 //��������Ϣ����           ��λ��bit
    U8 SNRAvgLen       :4;                 //ƽ���������Ϣ����        ��λ��bit
    U8 RSSIAvgLen;                         //�ź�ǿ����Ϣ����          ��λ��bit

    U16  InfoLen;                            //�ܳ���                   ��λ��bit
}__PACKED RF_DISC_NBINFOTYPELEN_MAP_t;
extern RF_DISC_NBINFOTYPELEN_MAP_t g_RfNbInfoTypeLenGroupMap[RF_NBINFOTYPELEN_MAP_GROUP_COUNT];
typedef struct
{
//    MGM_MSG_HEADER_t   MmsgHeader_t;        //<<< ����֡Я�����߹����б��Ƿ���Ҫ δ���������Ϣ����???

    U8 MacAddr[6];                          //����վ��MAC��ַ
    U8 DiscoverSeq;                         //�����б�ͳ����� 0-255 ѭ������
    U8 ListInfo[0];                         //��Ϣ�� ��Ϣ��Ԫ���ͣ���Ϣ��Ԫ�������ͣ���Ϣ��Ԫ���ȣ���Ϣ��Ԫ����
}__PACKED RF_DISCOVER_LIST_t;


#define MAXREPORTNUM	100
#define MAXCOLLNUM 150

typedef struct
{
    U8		ColloctPeriod;   // �ɼ�����
    U8 	 	ColloctType;     // �ɼ�����
    U16		ColloctTEI;      // ��ǰ�ɼ�TEI
    U8		ColloctFlag;     // �Ƿ����
    U8		ColloctNumber;   // �����ܴ���
    U8		ColloctSeq;      // ��ǰ�������
    U32		ColloctResult[MAXCOLLNUM];//����NTB

} __PACKED ZERO_COLLECTIND_RESULT_t;


extern	ZERO_COLLECTIND_RESULT_t	Zero_Result;//CCO����ʱ���¼,STA CAN USE



/*******************************************AODV***************************/
typedef struct{
    U8     payload[1024];
    U16   payloadLen;
}AODV_BUFF_t;

extern AODV_BUFF_t    AodvBuff;
extern U8 			 g_AodvMsduLID;//APS�㴫������LID


void CreatMMsgMacHeaderForSig(MAC_RF_HEADER_t *pMacHeader, U8 msduType, U16 msduLen);

void CreatMMsgMacHeader(MAC_PUBLIC_HEADER_t *pMacHeader, U16 MsduLen, U8 *DestMacAddr , U16 DestTEI, U8 SendType,
                        U8 MaxResendTime, U8 ProxyRouteFlag, U8 Radius, U8 RemainRadius, U8 bradDirc,U8 MacAddrUse);

//��CSMAʱ϶Ҫ���͵����ݹ��� TX_MNG_LINK ���� TX_DATA_LINK������
void entry_tx_mng_list_data(U8 priority, U16 macLen, U8 *pMacLongHeader, U16 destTEI, U8 phase, tx_link_t *tx_list, work_t *cfm);

//�������ű�ͷ����ű�data��������BEACON_FRAME_LIST
void entry_tx_beacon_list_data(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, list_head_t *tx_list);

//��ת���������ű�֡���������ű��б��ϡ���׼֡�;����ű�֡�ֱ���ڲ�ͬ������
void entry_rf_beacon_list_data(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, list_head_t *tx_list);
void entry_rf_beacon_list_data_csma(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, tx_link_t *tx_list);
void entry_rf_msg_list_data(U8 priority, U16 macLen, U8 *pMacLongHeader, U16 destTEI, U8 ResendTime, tx_link_t *tx_list, work_t *cfm);
void entry_tx_coord_data();

//���ͷ����б�
void SendDiscoverNodeList(void);

//�ӽڵ������Ϣ�����ӿ�
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

//���ڵ������Ϣ�����ӿ�
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

//aodv ·��ʹ��
void SendMMeRouteReq( U16 DstTEI, U16 ScrTei, U16 Handle);
void SendLinkRequest( struct ostimer_s *ostimer, void *arg);
void SendLinkResponse(U32 RequestSeq, U8 DstTEI, U8 LIQ);
void SendMMeRouteReply();
void SendMMeRouteAck(U32 RouteRequestSeq, U16 DstTEI);
void SendMMeRouteError(U16 OriginalTEI,U16 DstTEI);
void WaitReplyCmdOut(struct ostimer_s *ostimer, void *arg);

void SendMMeRFDiscoverNodeList(void);
void ProcessMMeRFDiscoverNodeList(mbuf_t *buf, U8 *pld, U16 len, work_t *work);


//sof ֡���ݴ���ӿ�
void ProcessNetMgmMsg(mbuf_t *buf, U8 *pld, U16 len, work_t *work);

#endif


