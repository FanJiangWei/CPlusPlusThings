/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        app_698p45.h
 * Purpose:     procedure for using
 * History:
 */


#ifndef __APP_698P45_H__
#define __APP_698P45_H__

#include "types.h"


#define   MaxTimePeriodTableSize    12

#define  MAX_ASSOC_OAD_NUM          20
#define  RSD_SELECT3_NUM			5
#define  RCSD_RSD_NUM			    15

#define MS_USERTYPE_NUM			12
#define MS_USERADDR_NUM			12
#define MS_CONFIGIDX_NUM		12
#define MS_REGION_NUM			5

#define RSD_SELECT3_NUM			5

#define OBJ_RN_LEN				16
#define OBJ_MAC_LEN				4

#define COLLECT_PLAN_MAX_CSD    4

#define METER_ADDR_LEN                   6
#define CLIENT_ADDR                      1

typedef U16  OI_s;


typedef enum{
	e_NULL = 0,
	ARRAY,
	STRUCTURE,
	e_BOOL,
	BIT_STRING,
	DOUBLE_LONG,
	DOUBLE_LONG_UNSIGNED,
	OCTET_STRING = 9,
	VISIBLE_STRING,
	UTF8_STRING = 12,
	INTEGER = 15,
	DATA_LONG,
	UNSIGNED,
	LONG_UNSIGNED,
	LONG64 = 20,
	LONG64_UNSIGNED,
	ENUM,
	FLOAT32,
	FLOAT64,
	DATE_TIME,
	DATE_Type,
	TIME,
	DATE_TIME_S,
	
	OI = 80,
	OAD,
	ROAD,
	OMD,
	TI,
	TSA,
	MAC,
	RN,
	REGION,
	SCALER_UNIT,
	RSD,
	CSD,
	MS,
	SID,
	SID_MAC,
	COMDCB,
	RCSD,
}__PACKED data_type_e;

typedef enum{
	LINK_REQUEST = 1,
	LINK_RESPONSE = 129,
	
	CONNECT_REQUEST = 2,
	RELEASE_REQUEST,
	
	GET_REQUEST = 5,
	SET_REQUEST,
	ACTION_REQUEST,
	REPORT_RESPONSE,
	PROXY_REQUEST,
	
	SECURITY_REQUEST = 16,
	
	CONNECT_RESPONSE = 130,
	RELEASE_RESPONSE,
	RELEASE_NOTIFICATION,
	GET_RESPONSE,
	SET_RESPONSE,
	ACTION_RESPONSE,
	REPORT_NOTIFICATION,
	PROXY_RESPONSE,
	
	SECURITY_RESPONSE = 144,
}__PACKED APDU_TYPE_e;

typedef enum{
	e_Null = 0,

    e_VOLTAGE_NUM ,                              //20000200  ��ѹ
    e_ELECTRIC_CURRENT ,                         //20010200  ����
    e_ZERO_LINE_CURRENT ,                        //20010400  ���ߣ����򣩵���
    e_POWER_FACTOR ,                             //200A0200  ��������
    e_FREEZING_TIME ,                            //20210200  ����ʱ��
                                                          
    e_POSITIVE_ACTIVE_ELECTRIC_ENERGY ,          //00100200  �����й�����
    e_REVERSE_ACTIVE_ELECTRIC_ENERGY ,           //00200200  �����й�����
    e_TOTAL_ELECTRIC_ENERGY_OF_POSITIVE ,        //00100201  �����й��ܵ���
    e_TOTAL_ELECTRIC_ENERGY_OF_REVERSE ,         //00200201  �����й��ܵ���
                                                          
    e_COMBINED_REACTIVE_1_ELECTRIC_ENERGY ,      //00300200  ����޹�1����
    e_COMBINED_REACTIVE_2_ELECTRIC_ENERGY ,      //00400200  ����޹�2����
                                                      
    e_FIRST_QUADRANT_REACTIVE_ENERGY ,           //00500200  ��һ�����޹�����
    e_SECOND_QUADRANT_REACTIVE_ENERGY ,          //00600200  �ڶ������޹�����
    e_THIRD_QUADRANT_REACTIVE_ENERGY ,           //00700200  ���������޹�����
    e_FOURTH_QUADRANT_REACTIVE_ENERGY ,          //00800200  ���������޹�����
                                                          
    e_MAX_DEMAND_TIME_OF_POSITIVE_ACTIVE ,       //10100200  �����й��������������ʱ��
    e_MAX_DEMAND_TIME_OF_REVERSE_ACTIVE ,        //10200200  �����й��������������ʱ��
    e_MAX_DEMAND_TIME_OF_COMBINED_REACTIVE_I ,   //10300200  ����޹�I�������������ʱ��
    e_MAX_DEMAND_TIME_OF_COMBINED_REACTIVE_II ,  //10400200  ����޹�II�������������ʱ��
                                                          
    e_FIRST_QUADRANT_REACTIVE_TOTAL_ENERGY ,     //00500201  ��һ�����޹��ܵ���
    e_SECOND_QUADRANT_REACTIVE_TOTAL_ENERGY ,    //00600201  �ڶ������޹��ܵ���
    e_THIRD_QUADRANT_REACTIVE_TOTAL_ENERGY ,     //00700201  ���������޹��ܵ���
    e_FOURTH_QUADRANT_REACTIVE_TOTAL_ENERGY ,    //00800201  ���������޹��ܵ���
}__PACKED OAD_TYPE_e;

typedef enum{
    DLT698_SUCCESS,
    HARDWARE_FAIL,
    TEMP_INVALID,
    REFUSE_RW,
    OI_UNDEFINED,
    CLASS_INCONFORMITY,
    OI_NONEXISTENT,
    TYPE_MISMATCH,
    EXCEED_LIMIT,
    DATA_NOT_AVAILABLE,
    FRAMING_CANCEL,
    NOT_FRAMING_STATE,
}__PACKED DATA_ACCESS_RESULT_e;

typedef enum{
	e_OAD,
	e_ROAD,
}__PACKED descrpt_type_e;

typedef enum{
	e_SECOND,
	e_MINUTE,
	e_HOUR,
	e_DAY,
	e_MONTH,
	e_YEAR,
}__PACKED time_unit_e;

typedef enum{
	e_NO_METER,
	e_ALL_USER_ADDR,
	e_SET_OF_USER_TYPE,
	e_SET_OF_USER_ADDR,
	e_SET_CFG_SEQUENCE,
	e_SET_TYPE_REGION,
	e_SET_ADDR_REGION,
	e_SET_CFG_SEQ_REGION,
}__PACKED MS_e;

typedef enum{
	e_COMMON_COLLECT = 1,
	e_EVENT_COLLECT,
	e_TRANSPARENT_PLAN,
	e_REPORT_PLAN,
	e_SCRIPT_PLAN,	
}__PACKED plan_type_e;

typedef enum{
	e_NORMAL = 1,
	e_OUT_OF_SERVICE,
}__PACKED task_state_e;
    
typedef enum{
	e_NORMAL_RSD = 1,
	e_REPORT_SIMPLIFY_RSD,
}__PACKED rsd_type_e;

typedef enum{
	e_FRNTCLOSE_BCKOPEN,
	e_FRNTOPEN_BCKCLOSE,
	e_FRNTCLOSE_BCKCLOSE,
	e_FRNTOPEN_BCKOPEN,
}__PACKED time_period_e;

typedef enum{
	e_300BPS = 0,
	e_600BPS,
	e_1200BPS,
	e_2400BPS,
	e_4800BPS,
	e_7200BPS,
	e_9600BPS,
	e_19200BPS,
	e_38400BPS,
	e_57600BPS,
	e_115200BPS,
	e_SELFADAPTION = 255,
}__PACKED ENUMRATED_BAUD_e;
    
typedef enum{
	e_NONE = 0,
	e_ODF,
	e_EVEN,
}__PACKED ENUMRATED_PARITY_e;
    
typedef enum{
	e_DATA_5_BIT = 5,
	e_DATA_6_BIT,
	e_DATA_7_BIT,
	e_DATA_8_BIT,
}__PACKED ENUMRATED_DATA_e;
    
typedef enum{
	e_STOP_1_BIT = 1,
	e_STOP_2_BIT,
}__PACKED ENUMRATED_STOP_e;

typedef enum{
	e_NULL_FLUID = 0,
	e_HARD_FLUID,
	e_SOFT_FLUID,
}__PACKED ENUMRATED_FLUIDCTRL_e;

typedef struct
{
	data_type_e	 dataType;      // Ӧ���������������
	U16          dataLen;       // ��������ݳ��� ��Ա䳤���ݽ������⴦��
	U8           dataNum;       // ������߽ṹ��Ԫ�غ���
	U16          procLen;       // ��Խ������ݴ���ĳ���
}__PACKED DLT698_data_s;

typedef struct{
	time_unit_e    TimeUnit;
	U16            InterValue;
}__PACKED TI_s;

typedef struct{
    U16  year;
    U8   month;
    U8   day_of_month;
    U8   day_of_week;
    U8   hour;
    U8   minute;
    U8   second;
    U16  milliseconds;
}__PACKED obj_date_time;

typedef struct{
    U16  year;
    U8   month;
    U8   day_of_month;
    U8   day_of_week;
}__PACKED date_s;

typedef struct{
    U8   hour;
    U8   minute;
    U8   second;
}__PACKED time_s;

typedef struct{
    U16  year;    // ע�ⱨ�����Ǵ��
	U8   month;
	U8   day;
	U8   hour;
	U8   minute;
	U8   second;
}__PACKED date_time_s;

typedef struct
{
    U8	matrixing;	//����
	U8  unit;		//��λ
}__PACKED scaler_unit_s;

typedef struct
{
	U8	saLen;
	U8	saValue[17];
}__PACKED TSA_s;

typedef struct
{
	U8	dataType;
	U8	dataLen;
	U8	data[64];    //���ȴ���
}__PACKED DATA_s;

typedef struct{
	U16   OI;
	U8    AttriId      :5;   // ���Ա�ʶ
	U8    AttriFeature :3;   // ��������
	U8    AttriElementIdx;
}__PACKED OAD_s;

typedef struct{
    U8       OAD_Type;
    OAD_s    OAD;
}__PACKED OAD_LIST_s;

typedef struct{
    OAD_s    OAD;
    U8       AssocOADNum;	
    OAD_s    AssocOAD[MAX_ASSOC_OAD_NUM];
}__PACKED ROAD_s;

typedef struct{
    OI_s   OI;          // �����ʶ
    U8     MethodId;    // ������ʶ
    U8     OperateMode; // ����ģʽ��Ĭ��Ϊ0
}__PACKED OMD_s;      // Object Method Descriptor

typedef struct
{
	U8		 dataScope;
	DATA_s	 dataStart;
	DATA_s	 dataEnd;
}__PACKED REGION_s;

typedef struct
{
	U8           userTypeNum;
    U8           userType[MS_USERTYPE_NUM];
}__PACKED USERTYPEUNIT_s;

typedef struct
{
	U8           userAddrNum;
    TSA_s        userAddr[MS_USERADDR_NUM];
}__PACKED USERADDRUNIT_s;

typedef struct
{
	U8           configNum;
    U16          configIdx[MS_CONFIGIDX_NUM];
}__PACKED CONFIGUNIT_s;

typedef struct
{
	U8           RegionNum;
    REGION_s     Region[MS_REGION_NUM];
}__PACKED REGIONUNIT_s;

typedef union
{
    //���ȴ���
    USERTYPEUNIT_s  userTypeUnit;
    USERADDRUNIT_s  userAddrUnit;
    CONFIGUNIT_s    ConfigUnit;
    REGIONUNIT_s    RegionUnit; 
}__PACKED unionOBJ_MS;

typedef struct{
	MS_e         MS_type;
	unionOBJ_MS	 Ms;
}__PACKED MS_s;

typedef struct
{
	OAD_s    oad;
	DATA_s	 data;//����oad�õ����ݳ���
}__PACKED RSD_Selector1_s;

typedef struct
{
	OAD_s		oad;
	DATA_s	    dataStart;
	DATA_s	    dataEnd;
	DATA_s	    dataInterval;
}__PACKED RSD_Selector2_s;

typedef struct
{
	U8	             num;
	RSD_Selector2_s  rsdSelector2[RSD_SELECT3_NUM];//��������
}__PACKED RSD_Selector3_s;

typedef struct
{
	date_time_s     collectStarttime;
	MS_s			meterSet;		
}__PACKED RSD_Selector4_s;

typedef struct
{
	date_time_s	    collectSavetime;
	MS_s			meterSet;		
}__PACKED RSD_Selector5_s;

typedef struct
{
	date_time_s	    collectStarttime;
	date_time_s	    collectEndtime;
	TI_s            timeInterval;
	MS_s            meterSet;		
}__PACKED RSD_Selector6_s;

typedef struct
{
	date_time_s	    collectSave_s;
	date_time_s	    collectSave_e;
	TI_s            timeInterval;
	MS_s            meterSet;		
}__PACKED RSD_Selector7_s;

typedef struct
{
	date_time_s	    collectSucc_s;
	date_time_s     collectSucc_e;
	TI_s            timeInterval;
	MS_s            meterSet;		
}__PACKED RSD_Selector8_s;

typedef struct
{
    U8	lastNum;
}__PACKED RSD_Selector9_s;

typedef struct
{
    U8      lastNum;
	MS_s    meterSet;
}__PACKED RSD_Selector10_s;

typedef union
{
	RSD_Selector1_s 	select1;
	RSD_Selector2_s 	select2;
	RSD_Selector3_s 	select3;
	RSD_Selector4_s 	select4;
	RSD_Selector5_s 	select5;
	RSD_Selector6_s 	select6;
	RSD_Selector7_s 	select7;
	RSD_Selector8_s 	select8;
	RSD_Selector9_s 	select9;
	RSD_Selector10_s 	select10;
}__PACKED unionRSD_SELECTOR;

typedef struct
{
	U8                  choice;
	unionRSD_SELECTOR	rsdselector;
}__PACKED RSD_s;

typedef union
{
	OAD_s		oad;
	ROAD_s	    road;
}__PACKED unionCSD;

typedef struct
{
	U8          choice;
	unionCSD	unioncsd;
}__PACKED CSD_s;

typedef struct
{
	U8       num;
	CSD_s    csd[RCSD_RSD_NUM];
}__PACKED RCSD_s;

typedef struct
{
    U16	    tag;		//��ʶ
    U8      len;		//���ݳ���
    U8      data[255];	//��������
}__PACKED SID_s;

typedef struct
{
	U8	len;
	U8	data[OBJ_MAC_LEN];
}__PACKED MAC_s;

typedef struct
{
	SID_s		sid;
	MAC_s		mac;
}__PACKED SID_MAC_s;

typedef struct
{
	U8		baud;
	U8		parity;
    U8        data;
    U8        stop;
    U8   fluid;
}__PACKED CMMDCB_s;

typedef enum{
    e_LINK_MANAGE = 1,
    e_APP_DATA = 3,
}__PACKED func_code_e;
    
typedef struct{
    U8    FuncCode  :3;
    U8    Res       :2;
    U8    FrgmntFlg :1;
    U8    PrmBit    :1;
    U8    DirBit    :1;    
}__PACKED ctrl_field_s;

typedef struct{
    U8    AddrLen     :4;
    U8    LogicAddr   :2;
    U8    AddrType    :2;
    
    U8    Addr[0];
}__PACKED SA_s;

typedef struct{
    U8            StartChar;
    U16           FrameLen   :14;
    U16           Res        :2;
    ctrl_field_s  CtrlField;
    SA_s          SA;
}__PACKED dl698frame_header_s;

typedef struct{
    U16   ServiceId;
    void (* Func)(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen);
}__PACKED DLT698_SERVICE_FUNC;

typedef struct{
    U16   ServiceId;
    U8  (* Func)(U8 *pApdu, U16 ApduLen, U8 *p698Data);
}__PACKED DLT698_RESPONSE_PROC_FUNC;

typedef struct{
    OI_s  OI;
    U16   (*Func) (OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);
}__PACKED DLT698_OI_GET_FUNC;

typedef struct{
    U32   Oad;
    U8   (*Func) (U8 *pApdu, U16 *pProcLen, void *p698Data);
}__PACKED DLT698_OI_PARSE_FUNC;

typedef struct{
    U8    serviceSeq  :6;
    U8    res         :1;
    U8    servicePri  :1;
}__PACKED PIID_s;   // Priority and Invoke ID

U8 ParseCheck698Frame(U8* pDatabuf, U16 dataLen, dl698frame_header_s  ** ppFrameHeader, U8 ** ppApdu, U8 *pAddr, U16 *pApduLen);
void Dl698PacketFrame(APDU_TYPE_e apduType, U8 subType, PIID_s piid, U8 listNum, U8 securityFlag,
                                                        U8 *pTxApdu, U16 apduLen, U8 *recivebuf ,U16 *reciveLen);
U16 DLT698ConstructData(U8* pTxApdu, data_type_e datatype, U16 datalen, void* pElement, U8 dataNum);
U8 Server698FrameProc(U8* pDatabuf, U16 dataLen, U8 *pRetDataBuf ,U16 *pRetDataLen);
U8 Client698FrameProc(U8* pDatabuf, U16 dataLen, void *p698Data);
U8 ServerReport698FrameProc(U8* pDatabuf, U16 dataLen, void *p698Data);

#endif



	
