/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     aps_interface.h
 * Purpose:	 Aps interface
 * History:
 * june 24, 2020	panyu    Create
 */
#ifndef __APS_INTERFACE_H__
#define __APS_INTERFACE_H__
#include "types.h"
#include "DataLinkInterface.h"
#include "aps_layer.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_comm.h"




// �������ݷ����������ݽṹ�������г������ݷ��Ͷ�ʹ�ã����ֳ���ģʽ��ʹ��
typedef struct
{
    U8       ReadMode;         // ����ģʽ�����ּ�������������·����������ͼ�������������
    U8       Direction;        // ���з���Ϊ0�����з���Ϊ1
	U16      dtei;             // Ŀ��TEIֵ�����ڳ������������ݻظ�

    U8       Timeout;          // �豸��ʱʱ�䣬��������֡ʱ��Ч
    U8       PacketIner;       // ��������ʱΪ���ļ��ʱ�䣬��ǰ�̶�Ϊ5����λΪ10ms
    U16      OptionWord;       // ����Ϊ0������ʱ��Ч���ڲ�������ʽʱ��bit15~bit0Ϊ����Ӧ��״̬

    U8       ResponseState;    // Ӧ��״̬������ʱ��Ч����ǰ�̶�Ϊ0
    U8       ProtocolType;     // ת����Լ���ͣ�0��͸�����䣻1��645-97��2��645-2007��3��698.45������������
	U16		 DatagramSeq;      // ������ţ�STAӦ��ʱ������CCO�������б����е���ű���һ��
	
    U8       DstMacAddr[MACADRDR_LENGTH];  // Ŀ��MAC��ַ���������е�ͨѶ��ַ�����ַ�ķ���
    U8       SrcMacAddr[MACADRDR_LENGTH];  // ԴMAC��ַ
	
    U8      SendType;
	U8      CfgWord         : 4;			//bit4:δӦ�����Ա�־��bit5:�������Ա�־��bit6~bit7:������Դ���
	U8      res         : 4;
	U16      AsduLength;       // ����Ӧ�ñ��ĳ���

	U8       Asdu[0];          // ����Ӧ�ñ���
} __PACKED READ_METER_REQ_t;


// ����ʱ�ļ����ݿ鷢������ṹ
typedef struct
{
    U32      UpgradeID;      // ����ID
    U32      BlockSeq;       // ���ݿ���

    U16      BlockSize;      // ���ݿ��С
    U8       TransMode;      // ����ģʽ��������ȫ���㲥������㲥��
	U8       IsCjq;          // ��ǰ�������ֱ�ģ����ɼ��������ڲ�ʹ����
	
    U8       DstMacAddr[MACADRDR_LENGTH];   // Ŀ��ģ��MAC��ַ
    U8       SrcMacAddr[MACADRDR_LENGTH];   // ԴMAC��ַ

    U8       DataBlock[0];	 // �ļ��������ݿ�
} __PACKED FILE_TRANS_REQ_t;


// �¼��ϱ����ݷ�����������CCO��STAȷ��Ҳʹ��
typedef struct
{
    U16    Direction  :4;   // 0�����з���ָCCO���͸�STA�ı��ģ�1�����з���ָSTA�ϱ�CCO�ı��ġ�
    U16    PrmFlag    :4;   // 0�����ԴӶ�վ��1����������վ��
    U16    FunCode    :4;   // �����룬����Ϊ1~4������1~3��
	U16    sendType   :4;   // �������ͣ��㲥�����ȣ���e_FULL_BROADCAST_FREAM_NOACK
    U16    EvenDataLen;     // �¼����ݳ���
    
	U16    PacketSeq;       // �������
    U8     MeterAddr[6];    // �¼��ϱ����ܱ��ַ
    U8     DstMacAddr[MACADRDR_LENGTH];   // �ϱ�Ŀ��MAC��ַ������ΪCCO��ַ������ΪSTA MAC��ַ
    U8     SrcMacAddr[MACADRDR_LENGTH];

    U8     EventData[0];    // �¼�����
} __PACKED EVENT_REPORT_REQ_t;

// ����ߵ����쳣�������ݸ�ʽ
typedef struct
{

    U16    Direction : 4;   // 0�����з���ָCCO���͸�STA�ı��ģ�1�����з���ָSTA�ϱ�CCO�ı��ġ�
    U16    PrmFlag : 4;   // 0�����ԴӶ�վ��1����������վ��
    U16    FunCode : 4;   // ����
    U16    sendType : 4;   // �������ͣ��㲥�����ȣ���e_FULL_BROADCAST_FREAM_NOACK

    U16    PacketSeq;       // �������
    U8		DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];//ԴMAC��ַ
    U8 	  CommandType;//��������
    U8 	  EvenDataLen;  	//���ݳ���
    U8 	  EventData[0];    // ��������
}__PACKED LiveLineZeroLineEnabled_t;

// ȷ�Ϸ��ϱ��ķ�������ȷ�Ϸ�����ʱδʹ��
typedef struct
{
    U8    Direct;           // 0�����з���ָCCO���͸�STA���򣻣������з���ָSTA�ϱ�CCO����ı��ģ�
    U8    AckOrNak;         // 0�����ϣ�1��ȷ�ϡ�
    U8    ReportSeq;        // ȷ�ϻ���ϵı������
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];
} __PACKED ACK_NAK_REQ_t;


// ��ѯID��Ϣ�������Ӧ���ݽṹ�������ж�ʹ��
typedef struct
{
    U8         IdType;      // ����ID���ͣ�APP_GW3762_CHIP_ID = 1��    APP_GW3762_MODULE_ID = 2��
    U8         direct;      // ����λ������Ϊ0������Ϊ1
	U16        destei;      // Ŀ��TEI��һ�㲻��

    U16        AsduLength;   // ID��Ϣ�������ݳ��ȣ�������Ч������ȱʡΪ0
    U16        PacketSeq;    // �����
    
    U8         DstMacAddr[MACADRDR_LENGTH];
    U8         SrcMacAddr[MACADRDR_LENGTH];

    U8         Asdu[0];     // ����Ϊ�գ�����Ϊ����ID��Ϣ
} __PACKED QUERY_IDINFO_REQ_RESP_t;


// ̨��ʶ���ķ������������ж�ʹ��
typedef struct
{   
	//U8    protocol;
    //U16   HeaderLen;
	U8    Direct;
	U8    StartBit;
	U8    Phase;
	U8    Featuretypes;
    
	U8    Collectiontype;
	U8    sendtype;
    U16   Res;

    U16   DatagramSeq;
    U16   payloadlen;

    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

	
    U8    payload[0];
} __PACKED INDENTIFICATION_REQ_t;


// ���������ݷ��ͽṹ�������ж�ʹ��
typedef struct
{
    U16   len;
	U16   dtei;
    
	U8    protocol;
    U8    Res1;
    U16   Res2;

    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
} __PACKED RD_CTRL_REQ_t;

// ����������͸�����ڷ��ͽṹ�������ж�ʹ��
typedef struct
{
    
	U16   dtei;
    U16   len;
    
	U8    protocol;
    U8    dirflag;
    U16   Res2;

    U32   baud;

    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
} __PACKED RD_CTRLT_TO_UART_REQ_t;

// �����ӽڵ�����ע�ᷢ��
typedef struct
{
    U32       PacketSeq;      // Ӧ�ò㱨����ţ��ӽڵ�����ע�ᷢ��һ�Σ�������ͬ�ı������
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED SLAVE_REGISTER_START_REQ_t;


// ע���ѯ�ӽڵ������ģ�������ˮ���ѯҲʹ��
typedef struct
{
    U8    RegisterPrmt;   // ע�������0����ѯע������1������ע�᣻2��������4����Ѱˮ��
    U8    ForcedResFlag;  // ǿ�Ʊ�־��0����ǿ�ƣ�1��ǿ��
    U32    PacketSeq;      // ������ţ�����һ�δӽڵ�����ע��ʹ��ͬһ�����
    U8    Res;

    U16   AsduLength;    // ��ʱδʹ��
    U16   Res1;
    
    U8    SrcMacAddr[MACADRDR_LENGTH];
    U8    DstMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];      // ��ʱδʹ��
} __PACKED REGISTER_QUERY_REQ_t;


// ֹͣ�ӽڵ�����ע������
typedef struct
{
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED SLAVE_REGISTER_STOP_REQ_t;


// Уʱ���ݱ��ķ������󣬹㲥Уʱֻ������
typedef struct
{
    U16    DataLength;    // Уʱ���ݳ���
    U16    Res;
    
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];

    U8     TimeData[0];   // Уʱ��������
} __PACKED TIME_CALIBRATE_REQ_t;


// ͨѶ���Ա��ķ�������ͨѶ���Ա��ķ���ֻ������
typedef struct
{
    U8       ProtocolType;
    U8       TestModeCfg;
    U16      TimeOrCfgValue;

    U16      AsduLength;
    U16      Res;
    
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];

    U8       Asdu[0];
} __PACKED COMMU_TEST_REQ_t;

// ��ʼ������������
typedef struct
{
    U32    UpgradeID;   // ����ID
    
    U16    UpgradeTimeout;    // ����ʱ�䴰�����������ʱʱ��
    U16    BlockSize;         // �������С��һ��Ϊ400�ֽ�
    
    U32    FileSize;          // �����ļ��ܳ���
    U32    FileCrc;           // �����ļ�CRC32ֵ
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_START_REQ_t;


// ����״̬��ѯ����
typedef struct
{    
    U32     StartBockIndex;   // ��ѯ��ʼ�����
    U32     UpgradeID;        // ����ID

    U16     QueryBlockCount;   // ��ѯ������0xFFFFΪ��ѯ���п�
    U16     Res;
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];
} UPGRADE_STATUS_QUERY_REQ_t;


// ֹͣ�������ķ�������
typedef struct
{
    U32      UpgradeID;      // ����ID
    
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_STOP_REQ_t;


// ����ִ���������б�������
typedef struct
{
    U32    UpgradeID;      // ����ID
    U32    TestRunTime;    // ��������ʱ�䣬��λΪ�룬0��ʾ����Ҫ������ʱ��
    
    U16    WaitResetTime;  // �ȴ���λʱ�䣬��λΪ�룬ע�����ʱ����STA���������С����
    U16    Res;
    
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_PERFORM_REQ_t;


// վ����Ϣ��ѯ���б��ķ�������
typedef struct
{
    U16     InfoListNum;  // ������Ϣ�б���
    U16     InfoListLen;  // ��Ϣ�б����ݳ���
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];

    U8      InfoList[0];
} __PACKED STATION_INFO_QUERY_REQ_t;


// ԭ�Լ���չ��̨��ʶ����Ϣ�ɼ�����
typedef struct
{
    U8    Direct;
    U8    GatherType;
    U16   AsduLength;
    
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
} __PACKED ZONEAREA_INFO_REQ_t;


// ע���ѯ��Ӧ���б��ķ���
typedef struct
{
    U8    Status;       // ��Ӧ״̬
    U8    MeterCount;     // ������
    U8    DeviceType;     // �豸����
	U8	  RegisterParameter;   // ע������������ж�Ӧ��0����ѯע������1������ע�᣻2��������4����Ѱˮ��

    U16   MeterInfoLen;  // ע�����Ϣ����
    U16   Res;

    U32   PacketSeq;     // Ӧ�ò㱨�����
    
    U8    DeviceAddr[6];   // �豸��ַ���ɼ���ʱΪ�ɼ�����ַ��ģ��ʱΪ���ܱ��ַ��ע���ַ˳��
    U8    DeviceId[6];    // STA������ΨһID������Ϊ��BCD��
    
    U8    SrcMacAddr[6];
    U8    DstMacAddr[6];
    
    U8    MeterInfo[0];         // ÿ���8���ֽڣ������ɱ���������
} __PACKED REGISTER_QUERY_RSP_t;


// ��ʼ����������Ӧ���ķ���
typedef struct
{
    U8       StartResultCode;  // ��ʼ�������н���룬0Ϊ�ɹ�������ֵΪʧ��
    U8       Res1;
    U16      Res2;
    
    U32      UpgradeID;     // ����Id
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_START_RSP_t;


// ����״̬��ѯ��Ӧ���б��ķ���
typedef struct
{
    U32    UpgradeID;        // ����ID
    U32    StartBlockIndex;   // ��ѯ��ʼ�����
    
    U16    ValidBlockCount;   // ��λͼ����Ч�Ŀ���
    U8     UpgradeStatus;     // 0-����״��1-���ս���̬��2-�������̬��3-��������̬��4-������̬
    U8     BitMapLen;     // λͼ���ݳ���

    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
    
    U8     BlockInfoBitMap[0];   // ����λͼ����
} __PACKED UPGRADE_STATUS_QUERY_RSP_t;


// վ����Ϣ��ѯ��Ӧ���б��ķ���
typedef struct
{
    U32    UpgradeID;   // ����ID
    
    U16    InfoListNum;   // ��Ӧ��Ϣ�б��������������Ӧ
    U16    InfoDataLen;   // ��Ϣ���ݳ���

    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];

    U8     InfoData[0];   // ��Ӧվ����Ϣ����
} __PACKED STATION_INFO_QUERY_RSP_t;






void ApsReadMeterRequest(READ_METER_REQ_t *pReadMeterReq);

void ApsFileTransRequest(FILE_TRANS_REQ_t *pFileTransReq);
void ApsEventReportRequest(EVENT_REPORT_REQ_t      *pEventReportReq);
void ApsAckNakRequest(ACK_NAK_REQ_t *pAckNakReq);
void ApsQueryIdInfoReqResp(QUERY_IDINFO_REQ_RESP_t *pQueryIdInfoReq);
void ApsIndentificationRequset(INDENTIFICATION_REQ_t         *pIndentificationReq);
void ApsRdCtrlRequest(RD_CTRL_REQ_t *pRdCtrlReq);
void ApsRdCtrlToUartRequest(RD_CTRLT_TO_UART_REQ_t *pRdCtrlReq);

//����ָʾʹ��
void ApsSlaveLeaveIndRequst(LEAVE_IND_t *pData);



#if defined(STATIC_MASTER)  

void ApsSlaveRegStartRequest(SLAVE_REGISTER_START_REQ_t *pSlaveRegStartReq);
void ApsSlaveRegQueryRequest(REGISTER_QUERY_REQ_t               *pRegQueryReq);
void ApsSlaveRegStopRequest(SLAVE_REGISTER_STOP_REQ_t *pRegStopReq);

void ApsTimeCalibrateRequest(TIME_CALIBRATE_REQ_t *pTimeCalibrateReq);
void ApsCommuTestRequest(COMMU_TEST_REQ_t *pCommuTestReq);

void ApsUpgradeStartRequest(UPGRADE_START_REQ_t   *pUpgradeStartReq);

void ApsUpgradeStatusQueryRequest(UPGRADE_STATUS_QUERY_REQ_t *pUpgradeStatusReq);
void ApsUpgradeStopRequest(UPGRADE_STOP_REQ_t *pUpgradeStopReq);
void ApsUpgradePerformRequest(UPGRADE_PERFORM_REQ_t *pUpgradePerformReq);
void ApsStationInfoQueryRequest(STATION_INFO_QUERY_REQ_t *pStationInfoReq);


void ApsZoneAreaInfoReqeust(ZONEAREA_INFO_REQ_t *pZoneAreaInfoReq);


#endif


#if defined(STATIC_NODE)

void ApsSlaveRegQueryResponse(REGISTER_QUERY_RSP_t *pRegQueryResp);

void ApsUpgradeStartResponse(UPGRADE_START_RSP_t *pUpgradeStartResp);



void ApsUpgradeStatusQueryResponse(UPGRADE_STATUS_QUERY_RSP_t *pUpgradeStatusResp);

void ApsStationInfoQueryResponse(STATION_INFO_QUERY_RSP_t *pStationInfoResp);



#endif



typedef struct
{
	U16      stei;                             // ��¼ԴTEI��ַ�����ڳ���������
	U16      ApsSeq;                           // Ӧ�ò�֡ͷ��ţ��������Ӧ���е���ű�����ͬ

    U8       ReadMode;                         // ���������������������������·����������
    U8       Timeout;                          // �豸��ʱʱ�� ��CCO��������λ��100���롣 ���вɼ�����ʱʱ���ǵ��ܱ�ʱ��ʱʱ���������
    U8       CfgWord;                           //������ �㳭 ·�ɱ�����������bit4��δӦ�����Ա�־��bit5: �������Ա�־��bit6~bit7: ������Դ�����3
    U8       OptionWord;                        //ѡ���� �㳭 ·��bit0:����λ����������;����:STA���豸���������౨�Ľ���ʱ,���ļ�����λ��10����
	
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];

    U16      AsduLength;	
    U8       ProtocolType;
	U8       Res1;

	U8       Asdu[0];
} __PACKED READ_METER_IND_t;

//typedef struct
//{
//	U16      stei;                             // ��¼ԴTEI��ַ�����ڳ���������
//	U16      ApsSeq;                           // Ӧ�ò�֡ͷ��ţ��������Ӧ���е���ű�����ͬ
//
//    U8       ReadMode;                         // ���������������������������·����������
//    U8       ResponseState;                    // Ӧ��״̬
//    U16      OptionWord;                       //ѡ���� �㳭 ·��bit0:byte0������0��byte1 bit0����λbit7��bit1������0;����:bit15��bit0������Ӧ��״̬
//	
//    U8       DstMacAddr[MACADRDR_LENGTH];
//    U8       SrcMacAddr[MACADRDR_LENGTH];
//
//    U16      AsduLength;	
//    U8       ProtocolType;
//	U8       Res1;
//
//	U8       Asdu[0];
//} __PACKED READ_METER_IND_RESP_t;

typedef struct
{
    U16    DataLength;
    U16    Res;
    
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
    
    U8     TimeData[0];
} __PACKED TIME_CALIBRATE_IND_t;


typedef struct
{
    U8       ProtocolType;
    U8       TestModeCfg;    
    U16      TimeOrCfgValue;

    U16      AsduLength;
    U16      SafeTestMode :4;
//    U16      res          :12;      //plc��Rf�����ش�ģʽ���� PHR_MCS��4bit�� PSDU_MCS��4bit�� PbSIZE��4bit
    U16    phr_mcs        :4;
    U16    psdu_mcs       :4;
    U16    pbsize         :4;
    
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];

    U8       Asdu[0];
} __PACKED COMMU_TEST_IND_t;


typedef struct
{
	U8      Direct;
    U8      FunCode;
	U16    	PacketSeq;

    U8      SendType;
    U8      Res1;
    U16     Res2;

    U16     EventDataLen;
    U8      MeterAddr[6];
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];

    U8      EventData[0];
} __PACKED EVENT_REPORT_IND_t;


typedef struct
{
    U8    Status;
    U8    SearchMeterStatus;
    U8    MeterCount;
    U8    DeviceType;
    
	U8	  RegisterParameter;
    U8    Res1;
    U16   MeterInfoLen;

    U32   PacketSeq;
    
    U8    DeviceAddr[6];
    U8    DeviceId[6];
    U8    SrcMacAddr[6];
    U8    DstMacAddr[6];

    U8    MeterInfo[0];
} __PACKED REGISTER_QUERY_CFM_t;



typedef struct
{
    U8    ForcedResFlag;
	U8	  RegisterParameter;
    U16   AsduLength;

    U32   PacketSeq;
    
    U8    SrcMacAddr[MACADRDR_LENGTH];
    U8    DstMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
} __PACKED REGISTER_QUERY_IND_t;


typedef struct
{
   
    U8       Status;
    U8       StartResultCode;
    U32      UpgradeID;
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];    
} __PACKED UPGRADE_START_CFM_t;


typedef struct
{
	U32    UpgradeID;
    U16    UpgradeTimeout;    // unit  minute
    U16    BlockSize;
    U32    FileSize;
    U32    FileCrc;
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];	
} __PACKED UPGRADE_START_IND_t;


typedef struct
{
    U32    UpgradeID;
    U32    BlockSeq;

    U16    BlockSize;
    U8     TransMode;
	U8     IsCjq;

    U8     DataBlock[0];
} __PACKED FILE_TRANS_IND_t;


typedef struct
{
    U8     Status;
	U8     UpgradeStatus;
    U16    ValidBlockCount;

    U32    UpgradeID;
    
    U16    StartBlockIndex;
    U8     BitMapLen;
    U8     Res; 
    
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
    
    U8     BlockInfoBitMap[0];
} __PACKED UPGRADE_STATUS_QUERY_CFM_t;


typedef struct
{
    U16      QueryBlockCount;
    U16      Res;
    
    U32      StartBockIndex;    
    U32      UpgradeID;
    
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_STATUS_QUERY_IND_t;


typedef struct
{
    U16     InfoListNum;
    U16     InfoListLen;
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];

    U8      InfoList[0];
} __PACKED STATION_INFO_QUERY_IND_t;

typedef struct
{
    U16     InfoListNum;
    U16     InfoListLen;
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];

    U8      InfoList[0];
} __PACKED STATION_INFO_QUERY_CFM_t;


typedef struct
{
    U16    WaitResetTime;
    U16    Res;
    
    U32    UpgradeID;
    U32    TestRunTime;

    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_PERFORM_IND_t;


#define  APP_UPLINK_HANDLE(hook, fn)    ((hook) = (fn))

typedef void (* read_meter_ind_fn)(READ_METER_IND_t      *pReadMeterInd);
extern read_meter_ind_fn  sta_read_meter_ind_hook;
extern read_meter_ind_fn  cco_read_meter_ind_hook;

typedef void (*read_meter_cfm_fn)(READ_METER_CFM_t *pReadMeterCfm);
extern read_meter_cfm_fn  cco_read_meter_cfm_hook;


typedef void (* time_calibrate_ind_fn)(TIME_CALIBRATE_IND_t *pTimeCalibrateInd);
extern time_calibrate_ind_fn  time_calibrate_hook;



typedef void (* commu_test_ind_fn)(COMMU_TEST_IND_t *pCommuTestInd);
extern commu_test_ind_fn commu_test_ind_hook;


typedef void (* event_report_ind_fn)(EVENT_REPORT_IND_t *pEventReportInd);
extern event_report_ind_fn sta_event_report_hook;
extern event_report_ind_fn cco_event_report_hook;



typedef void (*register_query_cfm_fn)(REGISTER_QUERY_CFM_t *pRegisterQueryCfm);
extern register_query_cfm_fn cco_register_query_cfm_hook;


typedef void (*register_query_ind_fn)(REGISTER_QUERY_IND_t *pRegsiterQueryInd);
extern register_query_ind_fn  sta_register_query_ind_hook;


typedef void (*upgrade_start_cfm_fn)(UPGRADE_START_CFM_t       *pUpgradeStartCfm);
extern upgrade_start_cfm_fn upgrade_start_cfm_hook;

typedef void (*upgrade_start_ind_fn)(UPGRADE_START_IND_t       *pUpgradeStartInd);
extern upgrade_start_ind_fn upgrade_start_ind_hook;

typedef void (*upgrade_stop_ind_fn)(uint32_t upgradeID);
extern upgrade_stop_ind_fn upgrade_stop_ind_hook;

typedef void (*upgrade_file_trans_ind_fn)(FILE_TRANS_IND_t *pFileTransInd);
extern upgrade_file_trans_ind_fn  upgrade_filetrans_ind_hook;

typedef void (*upgrade_status_query_cfm_fn)(UPGRADE_STATUS_QUERY_CFM_t  *pUpgradeQueryCfm);
extern upgrade_status_query_cfm_fn upgrade_status_query_cfm_hook;


typedef void (*upgrade_status_query_ind_fn)(UPGRADE_STATUS_QUERY_IND_t *pUpgradeQueryInd);
extern upgrade_status_query_ind_fn upgrade_status_query_ind_hook;


typedef void (*upgrade_stainfo_query_ind_fn)(STATION_INFO_QUERY_IND_t *pStaInfoQueryInd);
extern upgrade_stainfo_query_ind_fn  upgrade_stainfo_query_ind_hook;

typedef void (*upgrade_stainfo_query_cfm_fn)(STATION_INFO_QUERY_CFM_t *pStaInfoQueryCfm);
extern upgrade_stainfo_query_cfm_fn  upgrade_stainfo_query_cfm_hook;

typedef void (*upgrade_perform_ind_fn)(UPGRADE_PERFORM_IND_t *pUpgradePerformInd);
extern upgrade_perform_ind_fn upgrade_perform_ind_hook;


typedef void (*moduleid_query_cfm_fn)(QUERY_IDINFO_CFM_t  *pQueryIdInfoCfm);
extern moduleid_query_cfm_fn  cco_moduleid_query_cfm_hook;
extern moduleid_query_cfm_fn  rdctrl_moduleid_query_cfm_hook;

typedef void (*moduleid_query_ind_fn)(QUERY_IDINFO_IND_t *pQueryIdInfoInd);
extern moduleid_query_ind_fn  sta_moduleid_query_ind_hook;

typedef void (*indentification_ind_fn)(INDENTIFICATION_IND_t *pIndentificationInd);
extern indentification_ind_fn  indentification_ind_hook;

typedef void (*rdctrl_ind_fn)(RD_CTRL_REQ_t *pRdCtrlInd);
extern rdctrl_ind_fn  rdctrl_ind_hook;

typedef void (*rdctrl_to_uart_ind_fn)(RD_CTRLT_TO_UART_REQ_t *pRdCtrlToUartInd);
extern rdctrl_to_uart_ind_fn  rdctrl_to_uart_ind_hook;

typedef void (*Query_SwORValue_ind_fn)(QUERY_CLKSWOROVER_VALUE_IND_t *pQuerySwORValueInd);
extern Query_SwORValue_ind_fn  sta_QuerySwORValue_ind_hook;

typedef void (*Query_SwORValue_cfm_fn)(QUERY_CLKSWOROVER_VALUE_CFM_t *pQuerySwORValueCfm);
extern Query_SwORValue_cfm_fn  cco_QuerySwORValue_cfm_hook;

typedef void (*Set_SwORValue_ind_fn)(CLOCK_MAINTAIN_IND_t *pSetSwORValueInd);
extern Set_SwORValue_ind_fn  sta_SetSwORValue_ind_hook;

typedef void (*Set_SwORValue_cfm_fn)(CLOCK_MAINTAIN_CFM_t *pSetSwORValueCfm);
extern Set_SwORValue_cfm_fn  cco_SetSwORValue_cfm_hook;










/*
typedef void (* uart_recv_fn)(uint8_t *buf, uint32_t len);
uart_recv_fn  uart1_recv;

void register_uart1_recv_proc(uart_recv_fn fn)
{
    uart1_recv = fn;

    return ;
}
*/




#endif

