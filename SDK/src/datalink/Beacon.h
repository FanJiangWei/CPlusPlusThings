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

#define CenterSlotCount     							3  //�����ű�ռ�õ�ʱ϶��


#define BandCsmaSliceLength  							0 //
#define BEACONPERIODLENGTH  						    2000//0x7d0//0x1338//5s,???MS  ��ʼ���ںͲ������������
#define BEACONPERIODLENGTH1  						    5000//0x7d0//0x1338//5s,???MS  JOIN_most״̬

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
#define	MaxPayloadSoltNum							   207//215 ����Ԥ���˹��ʺ�Ƶ���л����ű���Ŀ
#define	MAXPCONUM									   188//195


#define NoRelationTimes								   1 // �������������
#define MaxSendBeaconCountWithoutRespond  			   1//	���δ���������ű�����

#define	AFTER_FINEWORK_COUNT 							30
#define	STAMPUNIT 										25000



extern U32   NwkBeaconPeriodCount;//������¼���ű�����
extern U32   SimBeaconPeriodCount;//������¼�ľ����ű�����
extern U8	 g_BeaconSlotTime;
extern U8    g_RelationSwitch;

U8 			 g_SendCenterBeaconCountWithoutRespond; //ȷ���Ƿ�����������Ľڵ㣬����ֵ����������ƣ�ȷ�����������Ѿ����

extern ostimer_t					 g_SendCenterBeaconTimer;

//ʱ϶����ʹ�õ�ȫ�ֲ����ṹ��
typedef struct{

    U32 beacon_start;

    U16 beacon_slot;
    U16 beacon_slot_nr;

    U16 csma_slot_a;
    U16 csma_slot_b;

    U16 csma_slot_c;
    U16 csma_time_slot;

    U8  RfBeaconType;     //cco ���������ű����ͣ� ��׼��e_STRAD_BEACON e_SIMP_BEACON ����e_SIMP_BEACON
    U8  NonCenterCnt;
    U8  RfBeaconSlotCnt;
    U8  CCObcn_slot_nr;   //�����ű�ʱ϶������

    U8 *pNonCenterInfo;

}__PACKED TIME_SLOT_PARAM_t;
extern TIME_SLOT_PARAM_t g_TimeSlotParam;
extern U32 BeaconStopOffset;

//����֡����ָʾ
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
//�ű�֡����ָʾ
typedef struct
{

    U32	 SNID;//sender SNID
    U16	 UpTEI;
    U8	 Phase;//
    S8    gain;
    U16    beaconLength;
    U8     beacondata[0];
} __PACKED BEACON_INDICATION_t ;

//����Э��֡����ָʾ
typedef struct
{
    U32	    SNID;
    U32     NbNet;					//�Ѿ�ռ�õ��ھ�SNID
    U16     Duration;				//�ھ��ŵ�����ʱ��
    U16     SendStartOffset;			//����ʼƫ��  //
    U16     MyDuration;				//���ŵ�����ʱ��
    U16     MyStartOffset;			//����ʼƫ��  //
} __PACKED COORD_INDICATION_t ;

//SACKָʾ
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
    e_STA_ABILITY_TYPE              = 0x00,     //վ������
    e_ROUTE_PARM_TYPE               = 0x01,     //·�в���
    e_BAND_CHANGE_TYPE              = 0x02,     //�л�band
    e_RF_ROUTE_PARM_TYPE            = 0X03,     //����·�ɲ�����Ŀ
    e_RF_CHANNEL_CHANGE_TYPE        = 0X04,     //�����ŵ������Ŀ
    e_SIMPBEACON_ABILITY_TYPE       = 0X05,     //�����ű�֡վ����Ϣ��ʱ϶��Ŀ
    e_LINKMODE_SET_ENITY            = 0xB1,     //��·������Ŀ
    e_SLOT_ALLOCATION_TYPE          = 0xC0,     //ʱ϶������Ŀ
    e_AREA_IDENTITY,
    e_UNLOCK_ENTRY                  = 0xBF,     //
    e_POWERLEVELENITY               = 0xE1,     //����������Ŀ
    
} BEACON_ENTRY_TYPE_e;
typedef enum
{
    e_STRAD_BEACON = 0,         //��׼�ű�֡
    e_SIMP_BEACON,              //�����ű�֡
}BEACON_FLAG_e;

//�����ű귢������
typedef enum
{
    e_BEACON_HPLC                   = 0x00,
    e_BEACON_RF                     = 0x01,
    e_BEACON_HPLC_RF                = 0x02,
    e_BEACON_HPLC_RF_SIMPLE         = 0x03,
    e_BEACON_HPLC_RF_SIMPLE_CSMA    = 0x04,
}RF_BEACON_TYPE_e;



#if defined(STD_DUAL)
//�����ű�֡�غ�
typedef struct
{

    U8      beaconType:         	3;				//�ű�����
    U8      Networkflag: 			1;				//������־λ
    U8      SimpBeaconFlag:         1;              //�����ű��־
    U8		Reserve:				1;              //����
    U8      Relationshipflag:		1;              //��ʼ������־λ
    U8      UseBeaconflag:			1;              //�ű�ʹ�ñ�־λ
    U8      Networkseq;                             //�������к�
    U8		CCOmacaddr[MACADRDR_LENGTH];            //CCO MAC ��ַ
    U32     BeaconPeriodCount;                      //�ű����ڼ���
    U8		BeaconEntryCount;                       //�ű���Ŀ������
} __PACKED RF_BEACON_HEADER_t;
//����·�ɲ�����Ŀ
typedef struct
{
    U8   BeaconEntryType;                           //�ű���Ŀͷ����Ŀ���ͣ�
    U8   EntryLength;                               //�ű���Ŀ����

    U8   DiscoverPeriod;                            //�����Ϸ����б����ڳ��ȣ� ��λ��1s
    U8   RfRcvRateOldPeriod;                        //���߽������ϻ����ڸ����� ��λ�����߷����б����ڡ�
} __PACKED RF_ROUTE_PARM_STRUCT_t;

//�����ŵ������Ŀ
typedef struct
{
    U8   BeaconEntryType;
    U8   EntryLength;

    U8   RfChannel;                                 //Ŀ���ŵ�
    U32  ChannelChangeRemainTime;                   //�ŵ��л�ʣ��ʱ�� ��λ��1ms
    U8   RfOption                   :2;             //Ŀ�������ŵ���optionģʽ
    U8                              :6;             //����
} __PACKED RF_CHANNEL_CHANGE_STRUCT_t;

//�����ű�վ��������ʱ϶��Ŀ
typedef struct
{
    U8   BeaconEntryType;
    U8   EntryLength;

    U32  Tei                        :12;            //վ��TEI
    U32  ProxyTei                   :12;            //����վ��TEI
    U32  NodeType                   :4;             //��ɫ
    U32  Level                      :4;             //�㼶

    U8   MacAddr[6];                                //վ��MAC��ַ
    U8   RfCount                    :4;             //��·��RF����
    U8                              :4;             //����

    U32  CsmaSlotStart;                             //CSMAʱ϶��ʼʱ��
    U16  CsmaSlotLength;                            //CSMAʱ϶����
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

    U8  rfBcnLen;  //���������ű�֡��ռ�ü����ز�ʱ϶
    U8  res[3];

}__PACKED NoCenterNodeInfo_t;
#endif

typedef struct
{

    U8      beaconType:         	3;				//�ű�����
    U8      Networkflag: 			1;				//������־λ
    U8      SimpBeaconFlag:         1;              //�����ű��־
    U8		Reserve:				1;              //����
    U8      Relationshipflag:		1;              //��ʼ������־λ
    U8      UseBeaconflag:			1;              //�ű�ʹ�ñ�־λ
    U8      Networkseq;                             //�������к�
    U8		CCOmacaddr[MACADRDR_LENGTH];            //CCO MAC ��ַ
    U32     BeaconPeriodCount;                      //�ű����ڼ���
    U8      RfChannel;                              //�����ŵ����
    U8      RfOption               :2;              //����option
    U8      res                    :6;              //����
    U8		Reserve1[6]			 ;                  //����
    U8		BeaconEntryCount;                       //�ű���Ŀ������
} __PACKED BEACON_HEADER_t;

//վ��������Ŀ
typedef struct
{
    U8   BeaconEntryType;                           //�ű���Ŀͷ����Ŀ���ͣ�
    U8   EntryLength;                               //�ű���Ŀ����

    U32	 TEI				: 12;                   //վ�� TEI
    U32	 ProxyTEI			: 12;                   //����վ�� TEI
    U32	 PathSuccessRate	: 8;                    //·�����ͨ�ųɹ���

    U8   SrcMacAddr[MACADRDR_LENGTH];               //վ��MAC��ַ


    U8   NodeType          : 4;                     //վ���ɫ
    U8   StaLevel          : 4;                     //վ������㼶

    U8	  WithProxyLQI;                             //�����վ��ͨ������

    U8   Phase             	: 2;                    //վ����������
    U8   RfCount          	: 4;                    //��·��RF����
    U8   Reserve          	: 2;                    //����
} __PACKED STA_ABILITY_STRUCT_t;

//·�ɲ�����Ŀ
typedef struct
{
    U8   BeaconEntryType;                           //�ű���Ŀͷ����Ŀ���ͣ�
    U8   EntryLength;                               //�ű���Ŀ����

    U16  RoutePeriod;                               //·������
    U16  RouteEvaluateRemainTime;                   //·������ʣ��ʱ��
    U16  ProxyDiscoveryPeriod;                      //����վ�㷢���б�����
    U16  StaionDiscoveryPeriod;                     //����վ�㷢���б�����
} __PACKED ROUTE_PARM_STRUCT_t;


//Ƶ��֪ͨ��Ŀ
typedef struct
{
    U8   BeaconEntryType;
    U8   EntryLength;

    U8   DstBand;
    U32  BandChangeRemainTime;
} __PACKED BAND_CHANGE_STRUCT_t;


//ʱ϶������Ŀ
typedef struct
{
    U8   BeaconEntryType;                           //�ű���Ŀͷ����Ŀ���ͣ�
    U16  EntryLength;                               //�ű���Ŀ����

    U8   NonCentralBeaconSlotCount;                 //�������ű�ʱ϶����

    U8   CentralSlotCount		: 4;                //�����ű�ʱ϶����
    U8   CsmaSlotPhaseCount     : 2;                //CSMAʱ϶֧�ֵ����߸���
    U8   Reserve				: 2;                //����

    U8   Reserve1;

    U8   ProxySlotCount;                            //�����ű�ʱ϶����
    U8   BeaconSlotTime;                            //�ű�ʱ϶����        ��λ1ms
    U8   CsmaSlotSlice;                             //CSMA ʱ϶��Ƭ����  ��λ10ms
    U8   BindCsmaSlotPhaseCout;                     //�� CSMA ʱ϶���߸���
    U8   BindCsmaSlotLinkId;                        //�� CSMA ʱ϶��·��ʶ��
    U8   TdmaSlotLength;                            //TDMA ʱ϶����
    U8   TdmaSlotLinkId;                            //TDMA ʱ϶��·��ʶ��
    U32  BeaconPeriodStartTime;                     //�ű�������ʼ�����׼ʱ
    U32  BeaconPeriodLength;                        //�ű����ڳ���        1ms
    U16  RfBeaconLength          :10;               //RF�ű�ʱ϶����      1ms
    U16  Reserve2                :6;                //����
    U8	 info[0];                                   //�������ű���Ϣ
} __PACKED SLOT_ALLOCATION_STRUCT_t;

//�������ű���Ϣ
typedef struct
{
    U16  SrcTei           : 12;                      //ָ�������ű��վ��� TEI  ��8bit
    U16  BeaconType       : 1;                      //�ű�����
    U16  RfBeaconType     : 3;                      //�����ű��־
} __PACKED NON_CENTRAL_BEACON_INFO_t;

//CSMA ʱ϶��Ϣ�ֶ�
typedef struct
{
    U32  CsmaSlotLength   : 24;                     //CSMA ʱ϶�ĳ��� ��λ�� 1 ����
    U32  Phase            : 2 ;                     //CSMA ʱ϶����
    U32  Reserve          : 6 ;                     //����

} __PACKED CSMA_SLOT_INFO_t;


//����������Ŀ
typedef struct
{
    U8    BeaconEntryType;
    U16   EntryLength;
    U16	  TGAINDIG;
    U16	  TGAINANA;
}__PACKED POWERSET_STRUCT_t;

//��·�л���Ŀ
typedef struct
{
    U8    BeaconEntryType;
    U8    EntryLength;
    U8 	  LinkMode;             //0x00��Ĭ��0�����������ã�0x01�����ز�����ģʽ��0x02�������߹���ģʽ��0x03��˫ģ����ģʽ������������
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
    e_JoinIdel = 0 , //�����տ�ʼ
    e_JoinStart    , //ģ�鿪ʼ����
    e_JoinListen 	, //STA����һ�ַ����ű�֮�����½ڵ�����
    e_JoinFinish_Most, //������ɣ�����һ��STA���ͷ����ű��û���½ڵ�������
    e_JoinFinish_All  //������ɣ�e_JoinFinish_Most״̬�£�·�����ڽ����ø�״̬
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

ostimer_t g_ChangeChlTimer ; //�л�Ƶ�ζ�ʱ��

#endif




#endif


