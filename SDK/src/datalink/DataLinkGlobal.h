#ifndef __DATALINKGLOBAL_H__
#define __DATALINKGLOBAL_H__

#include "types.h"
#include "sys.h"
#include "phy_plc_isr.h"
#include "list.h"
#include "ZCsystem.h"
#include "ScanRfCHmanage.h"

//·�ɱ�״̬
#define ACTIVE_ROUTE	0
#define INACTIVE_ROUTE  1

#define INVALID				0xffff

#define	   SNID_LIFE_TIME				  600 //�ھ�������Ϣ����ʱ��

#define MAX_ROUTE_FAILTIME               3  // ·�ɴ����������
#define MAX_DISCOERY_TABLE_ENTRYS  	  50//���·����Ŀ��
#define MAX_DISCOERY_EPIRATION_TIME    2 //�������ʱ��

#if defined(STATIC_MASTER)
#define MAX_WHITELIST_NUM				 2040//������������
#else
#define MAX_WHITELIST_NUM				 1015//������������
#endif
#define NWK_MAX_ROUTING_TABLE_ENTRYS  MAX_WHITELIST_NUM //1015//���·����Ŀ��
#define NWK_AODV_ROUTE_TABLE_ENTRYS    2  //UP  AND DOWN


//#define RoutePeriodTime_Normal           			100//0x64//?????????

#define	UP_LINK_LOWEST_RATIO			  20//����ͨ�ųɹ�����Сֵ
#define	DOWN_LINK_LOWEST_RATIO		  20//����ͨ�ųɹ�����Сֵ
#define INITIAL_SUCC_RATIO			  	  20//��ʼͨ�ųɹ���

#define NICE_SUCC_RATIO			  	  54//�Ż������ͨ�ųɹ�������
#define BACKUP_SUCC_RATIO				  50//����·������


#define	  BLACKNEIGBORLIFE				  200
#define   BADLINKTIME					  600

#define  	MAX_NET_MMDATA_LEN 		   1024  //net data max len

#define ROUTE_RELAY_TIME					10


#define	LOWST_RXLQI_PCO					  85// ̨��ʶ������Ҫע��
#define	LOWST_RXLQI_STA					  80// ̨��ʶ������Ҫע��
#define	LOWST_RXLQI_CCO					  85// ̨��ʶ������Ҫע��
#define	LOWST_RXGAIN					  55// ������������ֵ

#define LOWST_RXRSSI                      -80 //���߽����ź�ǿ������ֵ


#define	REV_ERR_BEACON_NUM				   3// �յ������ű��¼3��
#define	ERR_BEACON_CLEAN_TIME			  22// �����ű������ʱ22S
#define MSDUSEQ_INITVALUE                 0 //MSDU��ų�ʼֵ

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
#define MAX_NEIGHBOR_NUMBER 			  MEM_NR_C//�ھӱ���������
#else
#define MAX_NEIGHBOR_NUMBER 			  MEM_NR_C//�ھӱ���������
#endif


//�ھӱ��кͱ��ڵ�Ĺ�ϵ
typedef enum
{
    e_EERROUTE = 0, //����·��
    e_SAMELEVEL, //ͬ������·��
    e_UPLEVEL,  //�ϼ�����·��
    e_PROXY , 	//ѡΪ����
    e_UPPERLEVEL,//���ϼ�����·��
    e_CHILD,		//�ӽڵ�  ��ֻ���յ���������ʱ�������߲ű��ڵ���ӽڵ�
    e_NEIGHBER, //����Ϊ�ھ���·�й�ϵ���ڱ��ô����㹻������²Ŵ���
} RELATION_TYPE_e;


typedef enum{
	e_DOUBLE_HPLC_HRF = 0,	//�������
	e_SINGLE_HPLC = 1,		//�ز�����
	e_SINGLE_HRF = 2,		//��������
}F5F18_NETWORK_TYPE;

//�ھӷ����б�
typedef struct
{
	list_head_t link;
	
    U32	  SNID;
	
    U8    MacAddr[6];
    U16   NodeTEI;
    U8    NodeDepth  ; 	//�㼶
    U8    NodeType      :  4; 	//��ɫ
    U8    Relation      :  4; //�ڱ�ģ��Ĺؼn
    U8    Phase       	:  4; //��λ
    U8    BKRouteFg		:  4;


    S32	  GAIN;	       	//�����ۼӺ�
    S32   RecvCount;  	//���մ���

	
    U16   Proxy;    //�ھӽڵ�ĸ��ڵ�


    U8    LastUplinkSuccRatio;          //��¼�ϸ����ڵ�ͨ�ųɹ��ʣ�����Ƶ���ķ���������
    U8    LastDownlinkSuccRatio;        //��¼�ϸ����ڵ�ͨ�ųɹ��ʣ�����Ƶ���ķ���������

    U8    UplinkSuccRatio;
    U8    DownlinkSuccRatio;

    U8   My_REV;//�������ڽ��յ��ھӽڵ㷢�ͷ����б�Ĵ��� +beacon //My_REV
    U8   PCO_SED;//���������ھӽڵ㷢�͵ķ����б����,�ӷ����б���ֱ�ӻ�ȡ //PCO_SED
    U8   PCO_REV; //���������ھӽڵ��������ڵ㷢���б�Ĵ���,�ӷ����б���ֱ�ӻ�ȡ//PCO_REV
    U8   ThisPeriod_REV;//�������ڽ��յ��ھӽڵ㷢�ͷ����б�Ĵ��� +beacon

    U16	  RemainLifeTime ;//����ʱ��
    U8	  ResetTimes	; //��λ����
    U8	  LastRst	; //�����ڼ�¼�ĸ�λ����
    U16	  MsduSeq; //�ھӽڵ���һ֡�����
    U8    BeaconCount; //�ھ��ű����ڼ���
    U8    Res[16];
} __PACKED NEIGHBOR_DISCOVERY_TABLE_ENTRY_t;

//extern NEIGHBOR_DISCOVERY_TABLE_ENTRY_t NeighborDiscoveryTableEntry[MAX_NEIGHBOR_NUMBER];


typedef struct datalink_table_s{
	list_head_t link;
	
	uint16_t nr;
	uint16_t thr;
}DATALINK_TABLE_t;

extern DATALINK_TABLE_t	NeighborDiscoveryHeader;
//�����ھӱ��н����ھӵĴ���
//void UpdataNbStaRecvCnt(U32 _SNID, U8 *_MacAddr, U16 _NodeTEI);
U8 UpdataNbSendCntForBeacon();

/*****************�����ھӱ�ά��************************************/
#define RF_RCVMAP  16  //��¼���߷����б����λͼ��С��32BIT  ��ѡ 8/16/32
typedef struct{
    list_head_t link;

    U32	  SNID;

    U8    MacAddr[6];
    U16   NodeTEI;

    U8    NodeDepth  ;              //�㼶
    U8    NodeType      :  4;       //��ɫ
    U8    Relation      :  4;       //�ڱ�ģ��Ĺؼn
    U8    Phase       	:  4;       //��λ
    U8    BKRouteFg		:  4;
    U16   Proxy;                    //�ھӽڵ�ĸ��ڵ�


    U8    UpdateIndex;              //Rcvmap��������
    U8    RcvMap[RF_RCVMAP/8];      //���߷����б����λͼ
    U8    UpRcvRate;                //����ͨ�Ž����ʣ�ͨ�����Ľ�����ȡ
    U8    DownRcvRate;              //����ͨ�Ž��գ�ͨ��RcvMapλͼ����õ�
    U8    NotUpdateCnt;             //���н�����δ�������ڣ����н������ϻ�ʹ��


    int8_t DownRssi;                //��Ҫ����-90
    U16    DownRssiCnt;
    int8_t DownSnr;                 //>>-4��err, 2-4Ϊ��, ��Ҫ��֤
    U16    DownSrnCnt;
    int8_t UpRssi;
    int8_t UpSnr;

    U16	  RemainLifeTime ;          //����ʱ��
    U8	  ResetTimes	;           //��λ����
    U8	  LastRst	;               //�����ڼ�¼�ĸ�λ����
    U16	  MsduSeq;                  //�ھӽڵ���һ֡�����
    U8    BeaconCount;              //�ھ��ű����ڼ���
    U8    RfCount;                  //�ھӽڵ���·����������

    U8   RfDiscoverPeriod;          //�����Ϸ����б����ڳ��ȣ� ��λ��1s
    U8   RfRcvRateOldPeriod;        //���߽������ϻ����ڸ����� ��λ�����߷����б����ڡ�
    U8   DicvPeriodCntDown;         //�����б����ڵݼ��������������ϻ�ʹ��
    U8   RcvRateOldPeriodCntDown;   //�ϻ����ڸ����ݼ����������н������ϻ�ʹ��
}__PACKED RF_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t;

extern DATALINK_TABLE_t	RfNeighborDiscoveryHeader;

typedef struct
{
    U8   My_REV;//�������ڽ��յ��ھӽڵ㷢�ͷ����б�Ĵ��� +beacon //My_REV
    U8   PCO_SED;//���������ھӽڵ㷢�͵ķ����б����,�ӷ����б���ֱ�ӻ�ȡ //PCO_SED
    U8   PCO_REV; //���������ھӽڵ��������ڵ㷢���б�Ĵ���,�ӷ����б���ֱ�ӻ�ȡ//PCO_REV
    U8   ThisPeriod_REV;//�������ڽ��յ��ھӽڵ㷢�ͷ����б�Ĵ��� +beacon

    U8    LastUplinkSuccRatio;          //��¼�ϸ����ڵ�ͨ�ųɹ��ʣ�����Ƶ���ķ���������
    U8    LastDownlinkSuccRatio;        //��¼�ϸ����ڵ�ͨ�ųɹ��ʣ�����Ƶ���ķ���������
    U8    UplinkSuccRatio;
    U8    DownlinkSuccRatio;

    S32	  GAIN;	       	//�����ۼӺ�
    S32   RecvCount;  	//���մ���

    U8   Res[4];

}__PACKED HPLC_NEIGHBOR_INOF_t;

typedef struct
{

    U8    RcvMap[RF_RCVMAP/8];      //���߷����б����λͼ
    U8    res[4 - RF_RCVMAP/8];

    U8    UpdateIndex;              //Rcvmap��������
    U8    LUpRcvRate;               //�ϸ�·����������ͨ�Ž����ʣ�·�����ڽ����ϻ�
    U8    LDownRcvRate;             //�ϸ�·����������ͨ�ųɹ��ʣ�·�����ڽ����ϻ�
    U8    UpRcvRate;                //����ͨ�Ž����ʣ�ͨ�����Ľ�����ȡ
    U8    DownRcvRate;              //����ͨ�ţ�ͨ��RcvMapλͼ����õ�
    U8    NotUpdateCnt;             //���н�����δ�������ڣ����н������ϻ�ʹ��
#define MAX_RSSI_SNR_CNT 10         //�ź���Ⱥ�����ȣ��ƶ�ƽ���������ڴ�С 
    S8    DownRssi;                 //����ƽ���ź�ǿ��
    U16   DownRssiCnt;              //�����ź�ǿ��ͳ�Ƽ��������ڼ���ƽ���ź�ǿ��
    S8    DownSnr;                  //����ƽ�������
    U16   DownSrnCnt;               //���������ͳ�Ƽ��������ڼ���������
    S8    UpRssi;
    S8    UpSnr;


    U8   RfDiscoverPeriod;            //�����Ϸ����б����ڳ��ȣ� ��λ��1s
    U8   RfRcvRateOldPeriod;        //���߽������ϻ����ڸ����� ��λ�����߷����б����ڡ�
    U8   DicvPeriodCntDown;         //�����б����ڵݼ��������������ϻ�ʹ��
    U8   RcvRateOldPeriodCntDown;   //�ϻ����ڸ����ݼ����������н������ϻ�ʹ��

    U8   DiscoverSeq;               //���߷����б���ţ�����ʹ��
    U8   Res[1];

}__PACKED HRF_NEIGHBOR_INOF_t;


typedef struct {

    list_head_t link;

    U32	  SNID;

    U8    MacAddr[6];
    U16   NodeTEI;

    U8    NodeDepth  ; 	//�㼶
    U8    NodeType      :  4; 	//��ɫ
    U8    Relation      :  4; //�ڱ�ģ��Ĺؼn
    U8    Phase       	:  4; //��λ
    U8    BKRouteFg		:  4;
    U16   Proxy         :  12;    //�ھӽڵ�ĸ��ڵ�
    U16   LinkType      :  4;    //�����ز��ھӻ��������ھ�

    U16	  RemainLifeTime ;          //����ʱ��
    U8	  ResetTimes	;           //��λ����
    U8	  LastRst	;               //�����ڼ�¼�ĸ�λ����
    U16	  MsduSeq;                  //�ھӽڵ���һ֡�����
    U8    BeaconCount;              //�ھ��ű����ڼ���
    U8    RfCount;                  //�ھӽڵ���·����������

    U8    childUpRatio;             //��¼�ӽڵ�����гɹ���   
    U8    childDownRatio;           //��¼�ӽڵ�����гɹ���      

    //�����ز��ھӺ������ھӱ�����
    union
    {
        HRF_NEIGHBOR_INOF_t HrfInfo;
        HPLC_NEIGHBOR_INOF_t HplcInfo;
    };

    U8 data[0];

}__PACKED ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t;

/**********************Router Table,main router,exit in CCO and STA********************/
//·����
typedef struct
{
    U16     NextHopTEI	:12;
    U16     RouteState  : 1;
    U16     RouteError  :3;

    U16     LinkType    :2;
    U16     ActiveLk    :2;      //bit0:�ز���Ч bit1:������Ч
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

//·�ɷ�����
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
#define	   MaxSNIDnum					  48  //������ھ�������Ϣ������Y

#if defined(STATIC_MASTER)
typedef struct
{
    U32   SendSNID;    //�����ߵ����煷
    U32   NbSNID[MaxSNIDnum];  //�����ߵ��ھ�

    U16	  Duration;   //�ŵ�ռ��ʱ��
    U16   SendStartOffset;			//�´��ŵ�ռ�õ�ʱ�]/

    U16   MyStartOffset;			// ���ڵ��´�ռ�û��ж೤ʱ�]/
    U16	  LifeTime;			//S unit������ʱ�]

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
    U32   		NbSNID;  //�����ߵ��ھ�

    U16	  		LifeTime;			//S unit������ʱ�]
    U16	  		SameRatio; //���ƶȼǏ�

    U32	  		DiffTime;	  //�ͱ����綨��

	U16 		Blacktime;
    U8			ErrCode; //0,��ʾ�������߱�ʾ������·���ã߱�ʾ���ڰ�������
    U8			bandidx; //��ʾ����һ��Ƶ�α����������

    U16         option    :4;
    U16         RfChannel :12;
    U16         res;
} __PACKED STA_NEIGHBOR_NET_t;

extern STA_NEIGHBOR_NET_t StaNeigborNet_t[MaxSNIDnum]; //������δ��������ʹ��

/**********************NNIB,exit in CCO and STA****************************/
//��������Կ�
typedef struct
{
    U8	  MacAddr[6];
    U8	  WorkState ;                          //��ʾģ�鴦�ڼ���������״��
    U8	  SnidlnkTp ;                          //��ǰ�������������߻����ز�������0���ز��� 1������
    U8	  Networkflag;                         // �Ƿ��������
    
    U8	  NodeState;                           //���ڡ����B
    U8	  NodeLevel;
    U8	  PossiblePhase;
    U8	  BackupPhase1;
	
    U8	  BackupPhase2;
    U8	  PhasebitA:1;
    U8	  PhasebitB:1;
    U8	  PhasebitC:1;
    U8	  PhaseJuge:1;                         //�����ж��������
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
    U8	  last_My_SED;                      //�ϸ����ڷ����б�����Y
    U8	  CSMASlotPhaseCount;


    U32      NextRoutePeriodStartTime;      //·��ʣ��ʱ��

    U16      RoutePeriodTime;               //·������
    U8	  BindCSMASlotPhaseCount;
    U8      AODVRouteUse;


    U32   SendDiscoverListTime;             //���ͷ����б����ܖc
    U32   HeartBeatTime;                    //������ⱨ���ܖc
    U32   SuccessRateReportTime;            //ͨѶ�ɹ����ϱ��ܖc

    U8	  SendDiscoverListCount;            //��ǰ·�����ڷ��͵ķ����б�����Y
    U8	  beaconProtect;                    //STA��һ�Εr�g�]�а��Űl���Ř��x��
    U8	  FristRoutePeriod;
    U8	  SynRoutePeriodFlag;               //��ʾ��·�����ڽ������Ƿ��Ѿ��͸��ڵ��ٴ�ͬ��

    U16   StaOfflinetime;
    U16   RecvDisListTime;

    U32   SuccessRateZerocnt;               //�ĸ�·������ͨ�ųɹ���Ϊ0 ����

    U32   BandRemianTime;                   //�л�Ƶ��ʣ��ʱ��

    U8	  beaconSendTime;
    U8	  BandChangeState;                  //�Ƿ���Ҫ����Ƶ��
    U8	  PowerType;                        //ǿ����ϵ��
    U8	  Edge;                             //����������

   
    U16	  TGAINDIG;                         //���ֹ���
    U16	  TGAINANA;                         //ģ�⹦��

    U32   BeaconPeriodLength;
	
    U16	  discoverSTACount;
    U16   PCOCount;
	
    U8	  NbSNIDnum;
    U8	  powerlevelSet;                    //���ù���
    U8    ModuleType       :2;              //ģ������
    U8    LinkType         :1;              //��·���͡�0��HPLC 1������
    U8    RfConsultEn      :1;              //�ŵ�Э������  1������Э�� 0��������Э��
    U8    RfOnlyFindBtPco  :1;
    U8    res              :4;
    U8    RfCount;                          //��·RF ����

    U8    RfDiscoverPeriod;                 //���߷����б�����              ���������ģ�������� 10-255��
    U8    RfRcvRateOldPeriod;               //���߽������ϻ�����             ���������ģ��������4-16�����߷����б�����
    U16   RfChannelChangeState   : 1;       //�Ƿ���Ҫ��������ŵ���־
    U16   RfChgChannelRemainTime :15;       //�����ŵ��л�ʣ��ʱ��   uint:s

    U8 	  LinkModeStart;                    //��������ģʽ�л�
    U8 	  LinkMode    :4;                   //����ģʽ
    U8 	  NetworkType :4;                   //������ʽ��0�����������1���ز������� 2:��������
    U16   RfRecvDisListTime;
    U16	  ChangeTime;                       //unit:s
    U16	  DurationTime;                     //unit:s

    

}  NET_MANAGE_PIB_t;
extern NET_MANAGE_PIB_t 	nnib; //��������Կ�

/**********************������****************************/

typedef struct
{
    U8 ucRelLevel			 : 4;			 //�м̼���
    U8 ucSigQual			 : 4;			 //�ź�Ʒ��

    U8 ucPhase 			     : 3;			 //��λ
    U8 ucGUIYUE			     : 3;			 //ͨ��Э������
    U8 LNerr 			     : 1;			 //��𷴽�
    U8 ucReserved			 : 1;
} __PACKED METER_INFO_t;

typedef struct
{
    U8  MacAddr[6];
    U8  WaterAddrH7;
    METER_INFO_t MeterInfo_t;
    U8  CnmAddr[6];
    U8  Result; 			  	//0 δ��� 1 ���
    U8  waterRfTh;			    //ˮ��ǿ
    U8  ModuleID[11];
    
    U16 SetResult  :1;		    //���óɹ�
    U16 IDState    :2;          //ID���±�־
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
    U8 SetResult  :1;		//���óɹ�
    U8 IDState    :2;       //ID���±�־
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
	U8  ReportFlag;     //�ϱ���־
	U8  Res[2];
} __PACKED WHITE_MAC_CJQ_MAP_LIST_t;

extern WHITE_MAC_CJQ_MAP_LIST_t WhiteMaCJQMapList[MAX_WHITELIST_NUM];

//������·������б����q
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
    
    U8    MacAddr[6];				//�豸MAC��ַ
    U16	  DurationTime;      		//����ʱ��

    U16   NodeTEI	        :  12;    //TEI
    U16   DeviceType   		:  4; 	//�豸����
    U16   ParentTEI;         		//���ڞ�

    U32   NodeState     	:  4; 	//״��
    U32   NodeType      	:  4; 	//��ɫ����
    
    U32   Phase         	:  3; 	//��λ
    U32   LinkType          :  1;   //��·���͡�0��HPLC 1������
    U32	  LNerr				:  2;   //��𷴽�  0��ʾ���ӣ߱�ʾ���ӣ߱�ʾδ֪
    U32	  Edgeinfo			:  2;	//������0��ʾ�½��أ�1�����أ�2δ֪
    
    U32   NodeMachine       :  2; 	//�ű갲��ʱʹ�m
    U32   Reset	       		:  4;	//��λ����
    U32   ModuleType        :  2;   //ģ�����ͣ�0�����ز���1��HPLC+RF��2��������
    
    U32	  F31_D0			:1;     //������
    U32	  F31_D1			:1;     //������
    U32	  F31_D2			:1;     //������
    U32	  F31_D5			:1;     //����
    U32	  F31_D6			:1;     //����
    U32	  F31_D7			:1;     //����
    U32	  AREA_ERR	        :2;     //̨��ʶ����,ɽ��̨��ʶ��������״̬��δ֪����̨ȥ���Ǳ�̨ȥ

    U8    NodeDepth; 	           //����
    U8    UplinkSuccRatio;
    U8    DownlinkSuccRatio;
    U8    BootVersion;
    
    U8    SoftVerNum[2];
    U16   BuildTime;

    U8    ManufactorCode[2];
    U8    ChipVerNum[2];
    U8	  ManageID[24];

    U8    LogicPhase        :3;     //�߼���λ
    U8    HasRfChild        :1;     //��¼�Ƿ�����������ӵ��ӽڵ�
    U8    ModeNeed          :4;
    U8	  ModeID[11];
	
    U16   WhiteSeq          :12;
    U16   MacType           :4;
    U8   CollectNeed;      //�Ƿ�֧�ֲɼ�����
    U8	 CurveNeed;
	
	U8	  CCOMACSEQ;
	U8	  OntoOff;
	U8    OfftoOn;
	U8    Offtimes;
	
	U32   Offtime;
    U32	  UP_ERR : 2;    //������ϱ���������
    U32          : 30;
	
	U8    Proxytimes;
	U8    power_off_fg;		//������֪ͣ���־
    MANUFACTORINFO_t		ManufactorInfo;         // 18BYTE
}__PACKED DEVICE_TEI_LIST_t;

extern DEVICE_TEI_LIST_t DeviceTEIList[MAX_WHITELIST_NUM];


typedef struct
{
	list_head_t 		link;
	U16   NodeTEI	      ;    //TEI
	U8    MacAddr[6];				//�豸MAC��ַ
}__PACKED DEVICE_LIST_EACH;

DATALINK_TABLE_t DeviceListHeader;


#endif




#if defined(STATIC_MASTER)
extern U16   ChildStaIndex[1000]; //�����ӽڵ�ʱʹ��
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

/********************************�豸�б�**********************************/

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

//ȡ�豸�б����
U16 GetDeviceNum(void);
//app���ȡ�豸�б����������˲��ڰ������豸�������е�ַģʽ/�м�����
U16 GetDeviceNumForApp(void);
//ͨ����Ż�ȡһ��������device��Ŀ��Ϣ
U8 Get_DeviceList_All(U16 Seq , DEVICE_TEI_LIST_t* DeviceListTemp);

//ͨ��meteraddr��ȡ��һ��������device��Ŀ��Ϣ
U8 Get_DeviceList_All_ByAdd(U8 *meteraddr , DEVICE_TEI_LIST_t* DeviceListTemp);

//��ѯ�Ķ�Ӧ��SEQ,�������Ƿ����
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

/******************************STA������������߹���**************************************/
void modify_LeaveIndTimer(U32 first);
extern ostimer_t	 *JudgeProxyTypetimer;
/**********************************�������ظ���ʱʹ��**********************************************/
extern ostimer_t	*g_WaitChageProxyCnf; //�ȴ��������ظ�
void modify_mgm_changesendtimer(U32 frist);


 /*�������ƣ���ģ���ж������Ƿ�Ҫ����
a)STAվ���ڼ�������������һ���������������ڣ�����·�����ڣ��ڣ��ղ����κ��ű�֡��
b)STAվ��������4��·�������ڣ���������վ���ͨ�ųɹ���Ϊ0��
c)STAվ���յ�CCO�ġ��������кš��������¼�ġ��������кš���ͬ�����ű���ʵ�֣�
d)STAվ����յ�����ָʾ���ģ�ָʾ�Լ����ߣ�
e)һ��STAվ�㣬�����⵽CCO��MAC��ַ�����仯�����Ѿ�����һ�����ڣ���δ���ձ�׼��Ŀǰ���CCO��ַ�仯ʱ,�ϱ������ͻ��CCOΪmacaddδ��SNID�仯��
f)STAվ�㷢�ֱ�վ��Ĵ���վ���ɫ��Ϊ�˷���վ���Ѿ�����һ��·�����ڣ�δʵ�֣�
g)��վ��Ĳ㼶�������㼶���ƣ�15������վ����Ҫ���ߡ�(�ű���ʵ��)

h)�Լ����ӵı��o����
 */
void STA_JudgeOfflineBytimer();

/*********************************����ά����غ���**********************/
void StartNetworkMainten(U8 flag);

void InitHistoryPlanRecord();
U8 STA_JudgeOfflineBybeacon(U8 FormationSeqNumber, U8 *CCOmacaddr, U16 scrtei, U16 level , U8 beacontype, mbuf_t *buf);


///////////////////////////STA����������////////////////////////////////

//������������
void UpDataHeartBeat();

void Sta_Show_NbNet();
void STA_UpDataNeighborNetTime();

 void AddNeighborNet(mbuf_t *buf,U8 type);
void updateNeiNetRfParam(U32 snid, U8 option, U16 channel);


void FreeBadlinkNid(U8 bandidx);
void FreeHRfBadLinkNid();

//�����ھ���������ԣ�ErrCode !=0ʱ����Ҫ������¹�����ʱ��
void AddBlackNeigbor(U32 SNID, U32 ReassociaTime, U8 Reason);

U8 CheckBlackNegibor(U32 SNID);

U8 CheckNbNetHPLCBaud(U32 SNID);
U8 CheckNbNetHRfBaud(U32 SNID);

//U8 GetNeighborNetNum();
U8 GetPLanAinfo();



void scanStopTimer(uint8_t event);

#endif

/*�������ƣ������б�ά��
������·���ʼ��ʱ�����ô˺����������б�ά��
*/
void StartDatalinkGlobalUpdata();

/********************************Neighbornet func**********************************/
#if defined(STATIC_MASTER)
U16 GetChildStaBmpByteNum(U8 *pStaBmpBuf);
//�ж϶Է��Ƿ���������
U8 NeighborNetCanHearMe(U32 NbNetID,U32 WithNid);
//�����ŵ���ͻ������
void StartRfChlChgeForCCO();
//�����ھ��������ڵ��ַ
void UpdateNbNetInfo(U32 snid, U8 *ccoMac, U8 rfoption, U8 rfchannel);
#endif

void StopNetworkMainten();

U32 UpdataNeighborNet(U32 SNID);
void SearchMacInNeighborDisEntrybyTEI_SNID(U16 TEI , U32 SNID ,U8 *pMacAddr);
U16 GetNeighborBmp(U8 *OnlineTEIbmp, U16 *ByteNum);
U16 GetNeighborBmpByteNum();
ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *GetNeighborByTEI(U16 TEI , U8 linkType);


/*********************************Neighbornet list********************************/


/********************************һ���ڵ�����ű�������������߼���λ***********************/
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

/********************************�����ھӱ�ά���ӿ�*********************************/
void UpdateRfNbInfo(U32 SNID, U8 *MacAddr, U16 NodeTEI, U8 NodeDepth, U8 NodeType, U16 Proxy, U8 Relation, S8 RSSI, S8 SNR, U8 BeaconPeriodCount, U8 rfCount, U8 Frametype);
void UpDataRfNeighborListTime();
void UpDataRfNeighborRelation(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *TableEntry, U16 proxyTEI, U8 Level, U8 UplinkRatio, U8 DownlinkRatio);

/********************************router table********************************/
U8 ifHasRoteLkTp(U8 linkType);
U8 getRouteTableLinkType(U16 tei);
U8 UpdateRoutingTableEntry(U16 DestTEI, U16 NextHopTEI, U8 RouteError, U8 RouteState, U8 LinkType);
//��·�ɱ���Ѱ����һ��
U16 SearchNextHopAddrFromRouteTable(U16 DestAddress,U8 Aactive);
void RepairNextHopAddr(U16 NexTEI, U8 linkType);
//·�ɱ�����һ��ʧЧ
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
/*********************** CCO��������ʹ�� *********************/
extern ostimer_t *Renetworkingtimer;
void RenetworkingtimerCB(struct ostimer_s *ostimer, void *arg);




void IncreaseFromatSeq();


/************************CCO �����ͻ����*********************/
extern ostimer_t g_ChangeSNIDTimer;
void ChangeSNIDTimerCB(struct ostimer_s *ostimer, void *arg);
void ChangeSNIDFun(struct ostimer_s *ostimer, void *arg);

/***********************�����ŵ���ͻ����**********************/
extern ostimer_t *g_ChangeRfChannelTimer;
void ChangeRfChannelTimerInit();
extern void ChangeRfChannelTimerCB(struct ostimer_s *ostimer, void *arg);
extern void ChangeChannelPlcTimerCB(struct ostimer_s *ostimer, void *arg);
#endif

/*******************CCO����λʹ��*****************************/
U8 getTeiPhase(U16 tei);
U8 getNextTeiPhase(U16 tei);
void setSwPhase(U8 sw);

/*****************���߷����б��Ͷ�ʱ��*****************************/
ostimer_t	*RfDiscoverytimer;
__SLOWTEXT void StartRfDiscovery();

#if defined(STD_DUAL)
void setHplcOptin(U8 option);
void setHplcRfChannel(U32 rfchannel);
U8 getHplcOptin();
U32 getHplcRfChannel();
/**
 * @brief               ��������ŵ������Ϸ���
 * 
 * @param option        option  1-3
 * @param channel       channel�� option 1:�ŵ��� 1-40; option 2:�ŵ��� 1-80; option 3:�ŵ��� 1-200
 * @return U8           TRUE:�����Ϸ��� FALSE���������Ϸ�
 */
U8 checkRfChannel(U8 option, U16 channel);
#endif

//void modify_post_delay_timer(U8 first, work_t* work);

/*************************CCO�Զ��滮ʱ϶������ʱ��****************************************/
//#if defined(STATIC_MASTER)
//ostimer_t *SendBenconPeriodTimer;
//void modify_SendBenconPeriodTimer(uint32_t ms);
//#endif

#endif

