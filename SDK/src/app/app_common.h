
/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     Apstask.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */


#ifndef _APP_COMMON_H_
#define _APP_COMMON_H_
#include "types.h"
#include "aps_interface.h"


#define DEVTIMEOUT                 40//30//3000ms
#define PACKET_INTER               5  // 100ms

#define MAC_ADDR_LEN                   6

#define RETRANMITMAXNUM            6

#define   MAX_WIN_SIZE     40

#define    CONCURRENT_SWTICH_FLAG     	    0X00
#define    REGISTER_RUN_FLAG          	    0x01
#define    CARRIER_UPGRADE_FLAG       	    0x02
#define    OTHER_SWITCH_FLAG         	    0x03


#define MAXNUMMEM  80

typedef enum
{
    APP_T_PROTOTYPE,				/* ͸��ת�� */
    APP_97_PROTOTYPE,				/* DLT645-1997 */
    APP_07_PROTOTYPE,				/* DLT645-2007 */
    APP_698_PROTOTYPE,              /* DLT698.45 */
    APP_POWER_EVENT,                /* ͣ���¼��ϱ� */
    APP_BLACK_REPORT,               /* ̨������ */
    APP_LIVE_ZERO_ELEC_EVENT,       /* ������쳣 */
    APP_SINGLEMETER_PROTOTYPE,		/* ���������ϱ�ˮ��*/
    APP_DOUBLEMETER_PROTOTYPE,		/* ˫�������ϱ�ˮ��*/
    APP_GASMETER_PROTOTYPE,			/* ȼ����*/
    APP_HEATMEYET_PROTOTYPE,		/* �ȱ�*/
} AppProtoType_e;

typedef enum
{
	TRANS_PROTO_T,
	TRANS_PROTO_97,
	TRANS_PROTO_07,
	TRANS_PROTO_698,
	TRANS_PROTO_RF = 0x0F,
}CCO2STA_PROTO_E;


typedef enum
{
	e_METER_APP,
	e_CJQ1_APP,
	e_CJQ2_APP,
	e_UNKOWN_APP,
	e_SET_ADDR_APP=0xfe,

}__PACKED DeviceTypeApp_e;

typedef enum
{
    Generally = 0,					/* ���浥���������һ��֧�ֹ㲥 */
    AllBroadcast,					/* ȫ���㲥���� */
} BroadCastConRMType_e;

typedef struct
{
	U8 Addr[6];
	U8 Protocol;
	U8 ModeType    :4;
	U8 res         :4; 	
} __PACKED QUERY_SLAVE_REG_DATA_t;

typedef struct
{
	U8	cmd:4;
	U8	res:4;
	U16	totalNum;
	U8	currTotalNum;
}__PACKED SELLECT_WATER_HEAD_t;

typedef struct
{
	U8	cmd:4;
	U8	res:4;
	U16	totalNum;
	U8	currTotalNum;
}__PACKED GET_SINGLE_WATER_HEAD_t;

typedef struct
{
	U8	meterAddr[7];
	U8	rfThreshold;
}__PACKED SELLECT_WATER_INFO_t;

typedef struct
{
    U8	ucWorkState		: 1;		//����״̬��1:ѧϰ��0:����
    U8	ucRegState		: 1;		//ע������״̬��1:����0:������
    U8	ucReserved		: 2;
    U8	ucErrCode		: 4;		//�������
    U16 usBaudSpeed		: 15;		//ͨѶ����
    U16 usBaudUnit		: 1;		//���ʵ�λ��ʶ��0:bit/s��1:kbit/s
} __PACKED DCR_WORK_MODE_t;

typedef struct
{
    U8	ucVendorCode[2];
    U8	ucChipCode[2];
    U8	ucDay;
    U8	ucMonth;
    U8	ucYear;
    U8	ucVersionNum[2];
} __PACKED DCR_MANUFACTURER_t;


typedef struct
{
    //WHITE_MAC_LIST_t WhiteMacAddrList[MAX_WHITELIST_NUM];
    U16  usJZQRecordNum;   //�ӽڵ�����
	S8   scJZQProtocol;//e_GD2016_MSG,e_GW3762_MSG
    DCR_MANUFACTURER_t ManuFactor_t;
    
    DCR_WORK_MODE_t WorkMode_t;
    U8   ucJZQMacAddr[6];
    U8   ucJZQType;
    U8   ucEventReportFlag;//allow slave note report
    U8   ucSlaveReadTimeout;//�ӽڵ㳬ʱʱ��

    U8   ucMinuteCollect;
    U8 	 Res2[3];
} __PACKED FLASHINFO_t;

typedef struct
{
    U32  BaudRate;//������
} __PACKED RUNNING_INFO_t;

typedef struct
{
    U8   AppGw3762UpInfoFrameNum;
    U8   AppHandle;
} __PACKED APPPIB_t;

extern FLASHINFO_t FlashInfo_t;

extern APPPIB_t  AppPIB_t;
extern RUNNING_INFO_t  RunningInfo;
void ChangeMacAddrOrder(U8 *pMac);
unsigned short GetWord(unsigned char *ptr);
void  get_info_from_645_frame(U8 *pdata, U16 datalen, U8 *addr, U8 *controlcode, U8 *len, U8 *FENum);

#if defined(STATIC_MASTER)
void up_cnm_addr_by_read_meter(READ_METER_IND_t *pReadMeterInd, U8 *ScrMeterAddr);
#endif

#endif



