#ifndef _APP_GW3762_H_
#define _APP_GW3762_H_
#include "types.h"
#include "app_common.h"
#include "ProtocolProc.h"
#include "app_base_serial_cache_queue.h"


#if defined(STATIC_MASTER)

#define GW376209_PROTO_EN						1
#define GW376213_PROTO_EN						1
#define APP_RELAY_LEVEL_MAX					    15		/* ����м��� */
#define RG_WIN_SIZE                             1
#define RETRY_TIMES                             2
#define METER_NUM_SIZE                          13
#define ModuleIDLen                             11
#define BROADCAST_CONRM_NUM                     1    //��Ҫ��ֹ��ת������Ϊ2��ʵ���·�2��

/******************************** DLT645����********************************/
#define APP_DLT645_HEAD_LEN					    1
#define APP_DLT645_ADDR_LEN					    6
#define APP_DLT645_CTRL_LEN					    1
#define APP_DLT645_LEN_LEN						1
#define APP_DLT645_BASE_LEN					    12

#define APP_DLT645_HEAD1_POS					0
#define APP_DLT645_ADDR_POS					    1
#define APP_DLT645_HEAD2_POS					7
#define APP_DLT645_CTRL_POS					    8
#define APP_DLT645_LEN_POS						9
#define APP_DLT645_DATA_POS					    10

#define APP_DLT645_HEAD_VALUE					0x68
#define APP_DLT645_END_VALUE					0x16

#define APP_DLT645_DATA_UNIT_LEN				256
/******************************* GW3762����*******************************/
#define APP_GW3762_SPEED_NUM					1			/* AFN03F5/AFN03F10-���������� */
#define APP_GW3762_UPDATE_WAITTIME				30			/* AFN03F10-���������ȴ�ʱ�䡣�ն˷��������һ���������ݰ�����Ҫ�ȴ�ģ�����������ʱ�䳤��*/
#define APP_GW3762_UPINFO_CHANFEAT				0			/* ���ܱ�ͨ������ */
#define APP_GW3762_UPINFO_CHANFLAG				0			/* �ŵ���ʶ */
#define APP_GW3762_UPINFO_CMDQUALITY			0			/* ĩ�������ź����� */
#define APP_GW3762_UPINFO_EVENTFLAG			    0			/* �¼���־ */
#define APP_GW3762_UPINFO_MODULEFLAG			0			/* ͨ��ģ���ʶ */
#define APP_GW3762_UPINFO_PHASEFLAG			    0			/* ʵ�����߱�ʶ */
#define APP_GW3762_UPINFO_RELAYLEVEL			0			/* �м̼��� */
#define APP_GW3762_UPINFO_REPLYQUALITY			0			/* ĩ��Ӧ���ź�Ʒ�� */
#define APP_GW3762_UPINFO_ROUTERFLAG			0			/* ·�ɱ�ʶ */

#define BC_TIMEOUT								(0)	/* �㲥Уʱ��ʱ */

#define	APP_GW3762_METER_MAX		            MAX_WHITELIST_NUM //1015	/* AFN03F10/AFN10F1-һ��С����������*/
#define APP_GW3762_FILETRANS_PACK_MAX		    1024			//AFN03F10-�ļ��������󵥸����ݰ����ȡ�AFN15-F1�У����ְ���С
#define APP_GW3762_BROADCAST_TIMEOUT			1800
#define APP_GW3762_CHAN_NUM						1			/* AFN03F5-�ŵ����� */

#define APP_GW3762_HEAD_LEN					    1
#define APP_GW3762_LEN_LEN					    2
#define APP_GW3762_CTRL_LEN					    1
#define APP_GW3762_INFO_LEN					    6
#define APP_GW3762_ADDR_LEN					    6
#define APP_GW3762_AFN_LEN					    1
#define APP_GW3762_FN_LEN					    2
#define APP_GW3762_BASE_LEN					    15
#define APP_GW3762_HEAD_VALUE				    0x68
#define APP_GW3762_END_VALUE				    0x16
#define APP_GW3762_DATA_UNIT_LEN			    2048
#define APP_GW3762_FRAME_MAX_LEN                3000
#define APP_GW3762_SLAVE_SUB_MAX			    5		/* AFN13F1-�ӽڵ㸽���ڵ�������� */

/* ��Ϣ����� */
#define APP_GW3762_UPINFO_ROUTERFLAG		    0		/* ·�ɱ�ʶ */
#define APP_GW3762_UPINFO_MODULEFLAG		    0		/* ͨ��ģ���ʶ */
#define APP_GW3762_UPINFO_RELAYLEVEL		    0		/* �м̼��� */
#define APP_GW3762_UPINFO_CHANFLAG			    0		/* �ŵ���ʶ */
#define APP_GW3762_UPINFO_PHASEFLAG			    0		/* ʵ�����߱�ʶ */
#define APP_GW3762_UPINFO_CHANFEAT			    0		/* ���ܱ�ͨ������ */
#define APP_GW3762_UPINFO_CMDQUALITY		    0		/* ĩ�������ź����� */
#define APP_GW3762_UPINFO_REPLYQUALITY		    0		/* ĩ��Ӧ���ź�Ʒ�� */
#define APP_GW3762_UPINFO_EVENTFLAG			    0		/* �¼���־ */

#define JZQSETBAUD                              115
#define JS_BAUD115200                           115200
#define JS_BAUD9600                             9600

#define RETRANREGMAXNUM                         3
#define RETRANREGLOCKNUM                        5

extern U8 APP_GW3762_TRANS_MODE;
//  AFN10F4   Run status word define
//#define    EVENT_REPORT_FLAG                0x01
//#define    WORKING_FLAG                     0x01
//#define    ROUTE_STUDY_FINISH_FLAG     		0x01
// AFN10F4  work switch define
//#define    WORK_STATE_FLAG                  0x01
//#define    REGISTER_PERMIT_FLAG          	0x01
//#define    ZONE_DISTINGUISH_FLAG       		0x01
//#define    CONCURRENT_SWTICH_FLAG     	    0X00
//#define    REGISTER_RUN_FLAG          	    0x01
//#define    CARRIER_UPGRADE_FLAG       	    0x02
//#define    OTHER_SWITCH_FLAG         	    0x03


/******************************* ���Ͷ��� *******************************/

//////////////////////////////////////////////////////////////////////////////
typedef struct
{
    U8 TransMode	: 6;					/* ͨ�ŷ�ʽ */
    U8 StartFlag	: 1;					/* ������־ */
    U8 TransDir		: 1;					/* ���䷽�� */
} __PACKED AppGw3762CtrlField_t;

typedef struct
{
    U8 RouterFlag	: 1;					/* ·�ɱ�ʶ */
    U8 SubNodeFlag	: 1;					/* �����ڵ��ʶ */
    U8 ModuleFlag	: 1;					/* ͨ��ģ���ʶ */
    U8 ConflictCheck	: 1;				/* ��ͻ��� */
    U8 RelayLevel	: 4;					/* �м̼��� */
    U8 ChanFlag		: 4;					/* �ŵ���ʶ */
    U8 CorrectFlag	: 4;					/* ��������ʶ */
    U8 CalcDataNum;						    /* Ԥ��Ӧ���ֽ��� */
    U16 TransRate	: 15;				    /* ͨ������ */
    U16 RateFlag	: 1;					/* ���ʵ�λ��ʶ */
    U8 FrameNum;						    /* �������к� */
} __PACKED AppGw3762DownInfoField_t;

typedef struct
{
    U8 RouterFlag	: 1;					/* ·�ɱ�ʶ */
    U8 Reserve1		: 1;					/* Ԥ�� */
    U8 ModuleFlag	: 1;					/* ͨ��ģ���ʶ */
    U8 Reserve2		: 1;					/* Ԥ�� */
    U8 RelayLevel	: 4;					/* �м̼��� */
    U8 ChanFlag		: 4;					/* �ŵ���ʶ */
    U8 Reserve3		: 4;					/* Ԥ�� */
    U8 PhaseFlag	: 4;					/* ʵ�����߱�ʶ */
    U8 ChanFeat		: 4;					/* ���ܱ�ͨ������,��鿴����� */
    U8 CmdQuality	: 4;					/* ĩ�������ź�Ʒ�� */
    U8 ReplyQuality	: 4;					/* ĩ��Ӧ���ź�Ʒ�� */
    U8 EventFlag	: 1;					/* �¼���ʶ */
    U8 LineErr		: 1;					/* ��·�쳣 */
	U8 AreaFlag		: 1;					/* ̨����ʶ */
	U8 res			: 5;					/* ���� */
	
    U8 FrameNum;						    /* �������к� */
} __PACKED AppGw3762UpInfoField_t;

typedef struct
{
    U8 SrcAddr[MAC_ADDR_LEN];				/* Դ��ַ */
    U8 DestAddr[MAC_ADDR_LEN];				/* Ŀ�ĵ�ַ */
    U8 RelayAddr[APP_RELAY_LEVEL_MAX][MAC_ADDR_LEN];	/* �м̵�ַ */
} __PACKED AppGw3762AddrField_t;

typedef struct
{
    union
    {
        U8 CtrlFieldAll;
        AppGw3762CtrlField_t CtrlField;
    };
    union
    {
        U8 InfoFieldAll[APP_GW3762_INFO_LEN];
        AppGw3762DownInfoField_t DownInfoField;
        AppGw3762UpInfoField_t UpInfoField;
    };
    AppGw3762AddrField_t AddrField;
    U8 AddrFieldNum;
    U8 Afn;
    U16 Fn;
    U16 DataUnitLen;
    U8 DataUnit[APP_GW3762_DATA_UNIT_LEN];
} __PACKED APPGW3762DATA_t;

typedef enum
{
    APP_GW3762_DOWN_DIR,
    APP_GW3762_UP_DIR,
} AppGw3762Dir_e;

typedef enum
{
    APP_GW3762_SLAVE_PRM,
    APP_GW3762_MASTER_PRM,
} AppGw3762Prm_e;

typedef enum
{
    APP_GW3762_AFN00 = 0x00,
    APP_GW3762_AFN01,
    APP_GW3762_AFN02,
    APP_GW3762_AFN03,
    APP_GW3762_AFN04,
    APP_GW3762_AFN05,
    APP_GW3762_AFN06,
    APP_GW3762_AFN10 = 0x10,
    APP_GW3762_AFN11,
    APP_GW3762_AFN12,
    APP_GW3762_AFN13,
    APP_GW3762_AFN14,
    APP_GW3762_AFN15,
    APP_GW3762_AFN20 = 0x20,
    APP_GW3762_AFNF0 = 0xF0,
    APP_GW3762_AFNF1,
} AppGw3762Afn_e;

typedef enum
{
    APP_GW3762_F1 = 1,
    APP_GW3762_F2,
    APP_GW3762_F3,
    APP_GW3762_F4,
    APP_GW3762_F5,
    APP_GW3762_F6,
    APP_GW3762_F7,
    APP_GW3762_F8,
    APP_GW3762_F9,
    APP_GW3762_F10,
    APP_GW3762_F11,
    APP_GW3762_F12,
    APP_GW3762_F13,
    APP_GW3762_F14,
    APP_GW3762_F15,
    APP_GW3762_F16 = 16,
    APP_GW3762_F17 = 17,
    APP_GW3762_F18 = 18,
    APP_GW3762_F20 = 20,
    APP_GW3762_F21 = 21,
    APP_GW3762_F31 = 31,
    APP_GW3762_F32 = 32,
    APP_GW3762_F33 = 33,
    APP_GW3762_F40 = 40,
    APP_GW3762_F56 = 56,//���ղ�ѯCCO���ڲ�����
    APP_GW3762_F90 = 90, //��ѯʱ�ӹ����غͳ�����ֵ
    APP_GW3762_F92 = 92, 
    APP_GW3762_F93 = 93, //��ѯʱ��ά������
    APP_GW3762_F94 = 94,
    APP_GW3762_F100 = 100,
    APP_GW3762_F101 = 101,
    APP_GW3762_F102 = 102,
    APP_GW3762_F103 = 103,
    APP_GW3762_F104 = 104,
    APP_GW3762_F106 = 106,
    APP_GW3762_F111 = 111,
    APP_GW3762_F112 = 112,
    APP_GW3762_F130 = 130,
    APP_GW3762_F131 = 131,
    APP_GW3762_F200 = 200,
    APP_GW3762_F201 = 201,
    APP_GW3762_F225 = 225,
} AppGw3762Fn_e;

typedef enum
{
    e_HIGHEST_POWER,            //��߷��书��
    e_SECOND_HIGHEST_POWER,     //�θ߷��书��
    e_SECOND_LOWEST_POWER,      //�εͷ��书��
    e_LOWEST_POWER,             //��ͷ��书��
} APP_RF_POWER;

typedef enum
{
    APP_JZQ_TYPE_13,
    APP_JZQ_TYPE_09,
} APP_JZQ_TYPE_E;

/* AFN00F1 */
/* ����״̬ */
typedef enum
{
    APP_GW3762_N_CMDSTATE,
    APP_GW3762_Y_CMDSTATE,
} AppGw3762CmdState_e;

/* AFN00F1 */
/* �ŵ�״̬ */
typedef enum
{
    APP_GW3762_BUSY_CHANSTATE,
    APP_GW3762_IDLE_CHANSTATE,
} AppGw3762ChanState_e;

/* AFN00F2 */
/* ����״̬�� */
typedef enum
{
    APP_GW3762_TIMEOUT_ERRCODE,			    /* ͨ�ų�ʱ */
    APP_GW3762_DATAUNIT_ERRCODE,			/* ��Ч���ݵ�Ԫ */
    APP_GW3762_DATALEN_ERRCODE,			    /* ���ȴ��� */
    APP_GW3762_CHECKSUM_ERRCODE,			/* У����� */
    APP_GW3762_INFOCLASS_ERRCODE,			/* ��Ϣ�಻���� */
    APP_GW3762_FORMAT_ERRCODE,			    /* ��ʽ���� */
    APP_GW3762_ADDRREPEAT_ERRCODE,		    /* ����ظ� */
    APP_GW3762_ADDRNONE_ERRCODE,			/* ��Ų����� */
    APP_GW3762_NOANSWER_ERRCODE,			/* ���Ӧ�ò���Ӧ�� */
#if (GW376213_PROTO_EN > 0)
    /* 9 */APP_GW3762_MASTERBUSY_ERRCODE,	/* ���ڵ�æ */
    /* 10 */APP_GW3762_NOSUPPORT_ERRCODE,	/* ���ڵ㲻֧�ִ����� */
    /* 11 */APP_GW3762_NOSLAVE_ERRCODE,		/* �ӽڵ㲻Ӧ�� */
    /* 12 */APP_GW3762_NONWK_ERRCODE,		/* �ӽڵ㲻������ */
#endif

    APP_GW3762_OUT_MAXCONCURRNUM_ERRCODE = 109,   /*	������󲢷���*/
    APP_GW3762_OUT_MAX3762FRAMENUM_ERRCODE, /*����376.2֡�������Э�鱨������*/
    APP_GW3762_METER_READING_ERRCODE,       /*�˱����ڳ���*/
    
} AppGw3762ErrCode_e;

/* AFN03F5 */
/* ���ڳ���ģʽ */
typedef enum
{
    APP_GW3762_D_PERIODMETER,				/* ����ģʽ��֧�� */
    APP_GW3762_C_PERIODMETER,				/* ֧�ּ��������������ڳ���ģʽ */
    APP_GW3762_R_PERIODMETER,				/* ֧��ͨ��ģ�����������ڳ���ģʽ */
} AppGw3762PeriodMeter_e;

/* AFN03F5 */
/* ���ڵ��ŵ����� */
typedef enum
{
    APP_GW3762_MW_CHANFEAT,				    /* ΢�������� */
    APP_GW3762_SS_CHANFEAT,					/* ���๩�絥�ഫ�� */
    APP_GW3762_ST_CHANFEAT,					/* ���๩�����ഫ�� */
    APP_GW3762_TT_CHANFEAT,				    /* ���๩�����ഫ�� */
} AppGw3762ChanFeat_e;

/* AFN03F6 */
typedef enum
{
    APP_GW3762_N_MASTERDIST,
    APP_GW3762_Y_MASTERDIST,
} AppGw3762MasterDist_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_NC_CTRL_COMMMODE = 0,			/* ���� */
    APP_GW3762_CENTER_NC_CTRL_COMMMODE = 1,		/* ����ʽ·��խ���ز�ͨ�� */
    APP_GW3762_PER_NC_CTRL_COMMMODE = 2,		/* �ֲ�ʽ·��խ���ز�ͨ�� */
    APP_GW3762_HPLC_CTRL_COMMMODE = 3,			/* HPLC����ز�ͨ�� */
    APP_GW3762_HPLC_HRF_CTRL_COMMMODE = 4,		/* HPLC+HRF˫ģͨ�� */
} AppGw3762CtrlMode_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_NC_COMMMODE = 1,			        /* խ���������ز� */
    APP_GW3762_BC_COMMMODE = 2,				    /* ����������ز� */
    APP_GW3762_RF_COMMMODE = 3,				    /* ΢�������� */
    APP_GW3762_HPLC_HRF_COMMMODE = 4,			/* HPLC+HRF˫ģͨ�� */
} AppGw3762CommMode_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_ROUTERMANAGE,			        /* ����ͨ��ģ����·�ɹ����� */
    APP_GW3762_Y_ROUTERMANAGE,			        /* ����ͨ��ģ�����·�ɹ����� */
} AppGw3762RouterManage_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_SLAVEINFO,					    /* ����Ҫ�·��ӽڵ���Ϣ */
    APP_GW3762_Y_SLAVEINFO,					    /* ��Ҫ�·��ӽڵ���Ϣ */
} AppGw3762SlaveInfo_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_C_CYCLEMETER = 1,			/* ֧�ּ��������������ڳ���ģʽ */
    APP_GW3762_R_CYCLEMETER,				/* ֧��ͨ��ģ�����������ڳ���ģʽ */
    APP_GW3762_D_CYCLEMETER,				/* ����ģʽ��֧�� */
} AppGw3762CycleMeter_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_TRANSDELAY,				/* ��֧���������ṩ������ʱ���� */
    APP_GW3762_Y_TRANSDELAY,				/* ֧���������ṩ������ʱ���� */
} AppGw3762TransDelay_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_R_FAILCHANGE = 1,			/* ��ʾͨ��ģ�������л������ڵ� */
    APP_GW3762_C_FAILCHANGE,				/* ��ʾ����������֪ͨͨ��ģ�������л������ڵ� */
} AppGw3762FailChange_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_AFTER_BCCONFIRM,			    /* ��ʾ�㲥�����ڱ���ͨ��ģ��ִ�й㲥ͨ�Ź�����Ϻ󷵻�ȷ�ϱ��� */
    APP_GW3762_BEFORE_BCCONFIRM,			/* ��ʾ�㲥�����ڱ����ŵ�ִ�й㲥ͨ��֮ǰ�ͷ���ȷ�ϱ��� */
} AppGw3762BcConfirm_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_BCEXECUTE,				    /* ��ʾִ�й㲥�����Ҫ�ŵ���ʶ */
    APP_GW3762_Y_BCEXECUTE,					/* ��ʾִ�й㲥����Ҫ���ݱ����е��ŵ���ʶ��������� */
} AppGw3762BcExecute_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_N_LOWVOL,					/* ��ʾδ���� */
    APP_GW3762_Y_LOWVOL,					/* ��ʾ���� */
} AppGw3762LowVol_e;

/* AFN03F10 */
typedef enum
{
    APP_GW3762_BPS_RATEUNIT,				/* ��ʾbps */
    APP_GW3762_KBPS_RATEUNIT,				/* ��ʾkbps */
} AppGw3762RateUnit_e;

/* AFN05F10 */
typedef enum
{
    APP_GW3762_9600_BPS,				/* ��ʾ9600bps */
    APP_GW3762_19200_BPS,				/* ��ʾ19200kbps */
    APP_GW3762_38400_BPS,				/* ��ʾ38400kbps */
    APP_GW3762_57600_BPS,				/* ��ʾ57600kbps */
    APP_GW3762_115200_BPS,				/* ��ʾ115200kbps */
} AppGw3762BaudRateUnit_e;

/* AFN06F3 */
/* ·�ɹ�������䶯���� */
typedef enum
{
    APP_GW3762_READ_TASKCHANGE = 1, //����
    APP_GW3762_SEARCH_TASKCHANGE,   //�ѱ�
    APP_GW3762_AREA_TASKCHANGE,     //̨��ʶ��
} AppGw3762TaskChange_e;

/* AFN06F4 */
/* �ӽڵ��豸���� */
typedef enum
{
    APP_GW3762_C_SLAVETYPE,			    /* �ɼ��� */
    APP_GW3762_M_SLAVETYPE,		        /* ���ܱ� */
    APP_GW3762_HPLC_MODULE,             /* HPLCͨ�ŵ�Ԫ*/
    APP_GW3762_NARROW_MODULE,           /*խ���ز�ͨ�ŵ�Ԫ*/
    APP_GW3762_WIRELESS_MODULE,         /*΢��������ͨ�ŵ�Ԫ*/
    APP_GW3762_DOUBLE_MODE,             /*˫ģͨ�ŵ�Ԫ*/
} AppGw3762SlaveType_e;

typedef enum
{
    APP_GW3762_S_ADDMETERRES,			/* ��Ӵӽڵ�ɹ� */
    APP_GW3762_MR_ADDMETERRES,			/* ����ظ� */
    APP_GW3762_MO_ADDMETERRES,			/* ����������� */
    APP_GW3762_STDERR_ADDMETERRES,		/* ��׼���� */
    APP_GW3762_PROTOERR_ADDMETERRES,	/* ��Լ���� */
} AppGw3762AddMeterRes_e;

/* AFN11F3 */
/* ����״̬,0-����,1-ѧϰ */
typedef enum
{
    APP_GW3762_READ_WORKSTATE,
    APP_GW3762_STUDY_WORKSTATE,
} AppGw3762WorkState_e;

/*AFN15F1  FileTrans*/
typedef enum
{
    APP_GW3762_CLEAR_DOWNLOAD_FILE,
    APP_GW3762_LOCAL_MODULE_UPDATE = 0x03,
    APP_GW3762_SLAVE_CJQ_UPGRADE_START = 0x05,
    APP_GW3762_LOCAL_REMOTE_UPDATE = 0x07,
    APP_GW3762_SLAVE_MODULE_UPDATE = 0x08,
    APP_GW3762_SLAVE_UPGRADE_STOP   = 0x09,
    APP_GW3762_SLAVE_UPGRADE_START = 0x0A,
}AppGw3762FileTransId_e;

/*AFN10F40   read module ID*/
typedef enum
{
    APP_GW3762_RD_CTRL = 1,
    APP_GW3762_CCO_MODULE,
    APP_GW3762_METER_MODULE,
    APP_GW3762_REPEATER_MODULE,
    APP_GW3762_CJQ_II,
    APP_GW3762_CJQ_I,
}AppGw3762DeviceType_e;

typedef enum
{
    e_3762ZONE_DISTINGUISH_STOP = 0,
    e_3762ZONE_DISTINGUISH_STARTUP0203H = 1,    
    e_3762ZONE_DISTINGUISH_STARTUP010203H = 2,
}ZONE_DISTINGUISH_STATUS_e;

/*AFN03F12 10F7   read module ID*/
typedef enum
{
    COMBINATION_TEPEY ,     //��ϸ�ʽ
    BCD_TYPE,               //BCD
    BIN_TYPE,               //BIN
    ASCII_TYPE,             //ASCII
}ModuleIDFormat_e;

/*AFN03F12 10F7   read module ID*/
typedef enum
{
    MOUDLTID_NO_UP = 1,			/* ģ��Iδ���� */
    MOUDLTID_RENEW = 0,			/* ģ��ID�Ѹ��� */
} ModuleIDRenewSate_e;

/*AFNF0F11 */
typedef enum
{
    UOGRADE_TASK = 1,			/* ����*/
    REGISTER_TASK,			    /* ע�� */
    BITMAP_OFF_TASK,			/* λͼģʽͣ�� */
    ADDR_OFF_TASK,			    /* ��ַģʽͣ�� */
} MesgType_e;

/******************************* ��������֧�ֵĹ㲥���� *******************************/
//typedef enum
//{
//    Generally = 0,					/* ���浥���������һ��֧�ֹ㲥 */
//    AllBroadcast ,					/* ȫ���㲥���� */
//} BroadCastConRMType_e;

/* AFN06F1 */
typedef struct
{
    U8 Addr[MAC_ADDR_LEN];
    AppProtoType_e ProtoType;
    U16 Index;
} __PACKED AppGw3762SlaveInfo_t;

/* AFN06F4 */
typedef struct
{
    U8 Addr[MAC_ADDR_LEN];
    U8 ProtoType;
} __PACKED AppGw3762SubInfo_t;

/* AFN03F5 */
/* ״̬�� */
typedef struct
{
    U8	ucSpeedNum			: 4;			/* ��������n */
    U8	ucChanFeature		: 2;			/* ���ڵ��ŵ����� */
    U8	ucCycleReadMode		: 2;			/* ���ڳ���ģʽ */
    U8	ucChanNum			: 4;			/* �ŵ����� */
    U8	ucReserve			: 4;			/* Ԥ�� */
    U16	usBaudSpeed			: 15;			/* ͨѶ����*/
    U16	usBaudUnit			: 1;			/* ���ʱ�ʶ*/
} __PACKED APPGW3762AFN03F5_t;

/* AFN03F10 */
typedef struct
{
    U8 ucCommMode			: 4;
    U8 ucRouterManage		: 1;
    U8 ucSlaveInfo			: 1;
    U8 ucCycleMeter			: 2;

    U8 ucRouterTransDelay	: 1;
    U8 ucSlaveTransDelay	: 1;
    U8 ucBroadTransDelay	: 1;
    U8 ucFailChange			: 2;
    U8 ucBroadConfirm		: 1;
    U8 ucBroadExecute		: 2;

    U8 ucChannelNum			: 5;
    U8 ucALowVoltage		: 1;
    U8 ucBLowVoltage		: 1;
    U8 ucCLowVoltage		: 1;

    U8 ucRateNum			: 4;
    U8 ucMinuteCollect      : 1;//���Ӽ��ɼ�
    U8 ucReserve1           : 3;

    U8 ucReserve2;
    U8 ucReserve3;

    U8	ucSlaveReadTimeout;
    U16	usBroadCastTimeout;
    U16	usMsgMaxLen;
    U16	usForwardMsgMaxLen;
    U8	ucUpdateTime;
    U8	ucMainAddr[6];
    U16	usNodeMaxNum;
    U16	usCurNodeNum;
    U8	ucProtoDate[3];
    U8	ucProtoRecordDate[3];
    DCR_MANUFACTURER_t		ManuFactor_t;
    U16 BaudSpeed			: 15;			/* ͨѶ����*/
    U16 BaudUnit			: 1;			/* ���ʱ�ʶ*/
} __PACKED APPGW3762AFN03F10_t;

/* AFN03/05F17 */
/* ״̬�� */
typedef struct
{
    U8	ucRfOption			;			/* ���ߵ��Ʒ�ʽ */
    U8	ucRfChannel 		;			/* �����ŵ���� */
    U8	ucRfConsultEn		;			/* �ŵ�Э��ʹ�� */
    
} __PACKED APPGW3762AFN03F17_t;

typedef struct{
    U8    RecordLoc          : 1;
	U8    CourtsState          : 2;
	U8    Phase          : 3;
	U8    LNerr          : 1;
	U8    reserve          : 1;
    
}__PACKED APPGW3762AFN03F101_t;

enum{
	e_ACSAMPING_TPEY_645=1,
	e_ACSAMPING_TPEY_698=2,
};

typedef struct{
    U8    RunStatusRouter		:1;		//·����ɱ�־	1��·��ѧϰ��ɣ�0��δ���
	U8    RunStatusWorking		:1;		//������־     1�����ڹ�����0��ֹͣ����
	U8    RunStatusEvent		:1;		//�ϱ��¼���־ 1Ϊ�дӽڵ��¼��ϱ�
	U8    RunStatusRes			:5;		// �������
	
    U16   SlaveNodeCount;				//�ӽڵ�������
    U16   ReadedNodeCount;				//�ѳ��ӽڵ�����
    U16   RelayReadedCount;				//�м̳����ӽڵ�����

	
    U8    WorkSwitchWork			:1; //����״̬1ѧϰ0����
	U8    WorkSwitchRegister		:1; //ע������״̬1����0������
	U8    WorkSwitchEvent			:1; // �¼��ϱ�״̬1����0������
	U8    WorkSwitchZone			:1; //̨��ʶ��ʹ�ܱ�־ 1����0������
	U8    BJZoneFinish				:1; //����ʹ��
	U8    WorkSwitchRes				:1; //
	U8    WorkSwitchStates			:2; //��ǰ״̬ 00����,01�ѱ�,10����,11̨��ʶ��
	
    U16   CnmRate;						// �ز�ͨ������
    U8    RelayLevel[3];				// 1��2��3���м̼���
    U8    WorkStep[3];					// 1��2��3�๤������
}__PACKED APPGW3762AFN10F4_t;

typedef struct{
    
    U8    DeviceType            : 4;            //�豸����
    U8    Res1                  : 1;            //JS:���Ӳɼ���־
    U8    Res2                  : 2;            //����
    U8    Renewal               : 1;            //������Ϣ
	U8    VendorCode[2];
	U8    ModeIDLen;
	U8    ModeIDType;
	U8    ModeID[11];
}__PACKED APPGW3762AFN10F7_t;

typedef struct{
    
	U16   NodeTEI           : 12;
    U16   ModuleType        :  4;
	U16   ParentTEI         : 12;
    U16   Res               :  4;
    U8    NodeDepth     	:  4; 	//����
	U8    NodeType      	:  3; 	//��ɫ����
	U8    LinkType      	:  1; 	//ͨ������
}__PACKED APPGW3762AFN10F20_t;

typedef struct{
    
	U16   NodeTEI;
	U16   ParentTEI;
    U8    NodeDepth     	:  4; 	//����
	U8    NodeType      	:  4; 	//��ɫ����
}__PACKED APPGW3762AFN10F21_t;

typedef struct{
    U8    Phase1          : 1;
	U8    Phase2          : 1;
	U8    Phase3          : 1;
	U8    MeterType       : 1;
	U8    LNerr           : 1;
	U8    order1          : 1;
	U8    order2          : 1;
	U8    order3          : 1;
	U8    reserve1         ;
}__PACKED APPGW3762AFN10F31_t;

typedef struct{
	U8    VersionNum[2];
	U8    Day;
    U8    Month;
	U8    Year;
    U8    VendorCode[2];
    U8    ChipCode[2];
}__PACKED APPGW3762AFN10F104_t;

typedef struct{
    
    U8    DevType;
	U8    ChipID[24];
    U8    VersionNum[2];
}__PACKED APPGW3762AFN10F112_t;

typedef struct
{
    U8 ValidFlag;
    U8 Afn02Flag; 
    U8 ProtoType;											/* ͨ��Э������ */

#if (GW376213_PROTO_EN > 0)
    U8 DelayFlag;											/* ͨ����ʱ����Ա�־ */
#endif
    U8 SlaveSubNum;											/* �ӽڵ㸽���ڵ����� */
    U8 SlaveSubAddr[APP_GW3762_SLAVE_SUB_MAX * MAC_ADDR_LEN];	/* �ӽڵ㸽���ڵ��ַ */

    U16 FrameLen;											/* ���ĳ��� */
    U8 FrameUnit[APP_GW3762_DATA_UNIT_LEN];				    /* �������� */
    U8 Addr[MAC_ADDR_LEN];
    U8 CnmAddr[MAC_ADDR_LEN];
	U8 FrameSeq;
    //S8 ReTransmitCnt;
} __PACKED AppGw3762Afn13F1State_t;

typedef struct
{
    U8  Valid;
    U8 ProtoType;											/* ͨ��Э������ */

	U8 Framenum;
    U16 FrameLen;											/* ���ĳ��� */
    U8   FrameUnit[APP_GW3762_DATA_UNIT_LEN];				/* �������� */
    U8   Addr[MAC_ADDR_LEN];
    U8 CnmAddr[MAC_ADDR_LEN];
	U8 DltNum;
	U16 DatagramSeq;
    S8  DealySecond;
    S8 ReTransmitCnt;
} __PACKED AppGw3762AfnF1F1State_t;

typedef struct
{
    U8 ValidFlag;
    U8 ReadState;											/* ������־ */
    U8 CmnRlyFlag;
    U8 ProtoType;

    U16 FrameLen;											/* ���ĳ��� */
    U8 FrameUnit[APP_GW3762_DATA_UNIT_LEN];				    /* �������� */

    U8 SlaveSubNum;											/* �ӽڵ㸽���ڵ����� */
    U8 SlaveSubAddr[APP_GW3762_SLAVE_SUB_MAX * MAC_ADDR_LEN];	/* �ӽڵ㸽���ڵ��ַ */

    U8 Addr[MAC_ADDR_LEN];
    U8 CnmAddr[MAC_ADDR_LEN];
	U8 FrameSeq;
    //S8 ReTransmitCnt;
} __PACKED AppGw3762Afn14F1State_t;

typedef struct
{
    U16    CrnMeterIndex;
    U8      Addr[MAC_ADDR_LEN];
    U8      ProtoType;
    U16    CommuTime;

    U16     FrameLen;										/* ���ĳ��� */
    U8     FrameUnit[APP_GW3762_DATA_UNIT_LEN];				/* �������� */

    S8     ReTransmitCnt;
} __PACKED AppGw3762Afn06F2State_t;

typedef struct
{
    U8 CmnPhase;
    U8 Addr[MAC_ADDR_LEN];
    U16 MeterIndex;
    S8 ReTransmitCnt;
} __PACKED AppGw3762Afn14F1Up_t;

typedef struct
{
    U8	ucStartTime[6];						//��ʼʱ��
    U8	ucDurationTime[2];					//����ʱ��
    U8	ucRetransTime;						//�ӽڵ��ط�����
    U8	ucSlotNum;							//����ȴ�ʱ��Ƭ����
} __PACKED APPGW3762AFN11F5_ST;

typedef struct
{
    U16 usBaudSpeed : 15;		//ͨѶ����
    U16 usBaudUnit : 1;		//���ʵ�λ��ʶ��0:bit/s��1:kbit/s  
}BAUD_INFO_t;
BAUD_INFO_t BaudInfo;


typedef struct
{
    U8    DstAddr[MAC_ADDR_LEN];
    U8    GatherType;
}__PACKED  APPGW3762AFNF0F8_t;

typedef struct
{
    U8 DeviceType			 : 4;			 //�豸����
    U8 Res			         : 3;			 //
    U8 Renewal 			     : 1;			 //������Ϣ
} __PACKED REPORT_DEVICE_TYPE_t;

typedef struct
{
    U8	MacAddr[6];
    U16 NodeTEI;

    U16 DeviceType   		:  4; 	//�豸����
    U16 Phase               :  3; 	//��λ
    U16	LNerr				:  2;   //��𷴽�  0��ʾ���ӣ�1��ʾ���ӣ�2��ʾδ֪
    U16	Edgeinfo			:  2;	//����Ϣ	0��ʾ��CCO��ͬ��1����ͬ��2δ֪
    U16	res                 :  3;
    U16 NodeMachine         :  2; 	//�����ļ���״̬

    U16 NodeType            :  4; 	//��ɫ����
    U16 NodeState           :  4; 	//״̬
    U16 NodeDepth           :  4; 	//����
    U16 Reset               :  4;	//��λ����
    U16	DurationTime;               //����ʱ��

    U16 ParentTEI;                  //���ڵ�
    U8  UplinkSuccRatio;
    U8  DownlinkSuccRatio;

    U8  SoftVerNum[2];
    U16 BuildTime;
    U8  BootVersion;

    U8  ManufactorCode[2];
    U8  ChipVerNum[2];
    U8	F31_D0              :1;
    U8	F31_D1              :1;
    U8	F31_D2              :1;
    U8                      :2;
    U8	F31_D5              :1;
    U8	F31_D6              :1;
    U8	F31_D7              :1;
    
    U8  AREA_ERR;

    U8  CCOMACSEQ;
    U8	ModeNeed;

    MANUFACTORINFO_t		ManufactorInfo;
} __PACKED APPGW3762AFNF005UP_UNIT1_t;

typedef struct
{
    U8	MacAddr[6];

    U16 NodeType            :  4; 	//��ɫ����
    U16 NodeState           :  4; 	//״̬
    U16 NodeDepth           :  4; 	//����
    U16 Reset               :  4;	//��λ����
    U16	DurationTime;               //����ʱ��

    U16 ParentTEI;                  //���ڵ�
    U8  UplinkSuccRatio;
    U8  DownlinkSuccRatio;

    U8	F31_D0              :1;
    U8	F31_D1              :1;
    U8	F31_D2              :1;
    U8                      :2;
    U8	F31_D5              :1;
    U8	F31_D6              :1;
    U8	F31_D7              :1;
    
    U8  AREA_ERR;
    U8  CCOMACSEQ;
    U8	ModeNeed;
} __PACKED APPGW3762AFNF005UP_UNIT2_t;

typedef struct
{
    U32	  SNID                           ;

    U8    MacAddr[6]                     ;
    U16   NodeTEI                        ;

    U8    NodeType                   :  4; 	//��ɫ
    U8    NodeDepth                  :  4; 	//�㼶
    U8    Relation                   :  4;   //�ڱ�ģ��Ĺ�?
    U8    Phase                      :  4;   //��λ
    U8    BKRouteFg	                 ;
    S8    rgain;                             //GAIN/RecvCount

    U8    UplinkSuccRatio                ;
    U8    DownlinkSuccRatio              ;

    U16   My_REV                         ;   //�������ڽ��յ��ھӽڵ㷢�ͷ����б�Ĵ��� +beacon //My_REV
    U16   PCO_SED                        ;   //���������ھӽڵ㷢�͵ķ����б����,�ӷ����б���ֱ�ӻ�ȡ //PCO_SED
    U16   PCO_REV                        ;    //���������ھӽڵ��������ڵ㷢���б�Ĵ���,�ӷ����б���ֱ�ӻ�ȡ//PCO_REV
    U16   ThisPeriod_REV                 ;   //�������ڽ��յ��ھӽڵ㷢�ͷ����б�Ĵ��� +beacon
    U16   PerRoutePeriodRemainTime       ;

    U16	  RemainLifeTime                 ;   //����ʱ��
    U8	  ResetTimes	                 ;   //��λ����
    U8	  LastRst	                     ;    //�����ڼ�¼�ĸ�λ����
} __PACKED APPGW3762AFNF014UP_UNIT_t;

typedef struct
{
    U32 Index;
    void (* Func) (APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
} __PACKED AppGw3762ProcFunc;


/******************************* �������� *******************************/
extern APPGW3762DATA_t AppGw3762DownData;
extern APPGW3762DATA_t AppGw3762UpData;
extern U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN];
extern AppGw3762Afn13F1State_t   AppGw3762Afn13F1State;
extern AppGw3762Afn14F1State_t AppGw3762Afn14F1State;
extern APPGW3762AFN10F4_t   AppGw3762Afn10F4State;
extern U8		STARegisterFlag;
extern U8       WIN_SIZE;
extern U8       IsCjqfile;

/******************************* �������� *******************************/

#if 0//((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))

static void AppGw3762Reserve(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn00F1Sure(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn00F2Deny(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn01F1HardInit(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn01F2ParamInit(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn01F3DataInit(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn02F1DataTrans(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn03F1ManufCode(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F2Noise(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F3SlaveMonitor(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F4MasterAddr(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F5MasterStateSpeed(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F6MasterDisturb(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn03F7SlaveTimeout(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F8QueryRfParam(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F9QuerySlaveTimeout(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F10MasterMode(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F11MasterIndex(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn03F100QueryRssiThreshold(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn04F1SendTest(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn04F2SlaveRoll(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn04F3MasterTest(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn05F1SetMasterAddr(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F2SetSlaveReport(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F3StartBc(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn05F4SetSlaveMaxTimeout(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F5SetRfParam(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F100SetRssiThreshold(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn05F101SetMasterTime(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn10F1QuerySlaveNum(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F2QuerySlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F3QuerySlaveRelay(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F4RouterState(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F5FailSlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F6RegSlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn10F100QueryNetScale(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn10F101QuerySlaveInfo(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn11F1AddSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F2DelSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F3SetSlaveRelay(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F4SetWorkMode(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F5StartRegSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn11F6StopRegSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F100SetNetScale(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F101StartNetMaintain(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn11F102StartNetwork(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn12F1Restart(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn12F2Suspend(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn12F3Recover(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn13F1MonitorSlave(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);

static void AppGw3762Afn14F1RouteReqRead(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762Afn14F2RouterReqClock(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#if (GW376213_PROTO_EN > 0)
static void AppGw3762Afn14F3RouterReqRevise(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
#endif

static void AppGw3762Afn15F1FileTrans(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762AfnF0F5Debug(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);
static void AppGw3762AfnF0F7Debug(APPGW3762DATA_t *Gw3762Data_t, MESG_PORT_e port);


#endif

#if ((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))
void app_gw3762_up_afn00_f1_sure(AppGw3762CmdState_e cmdstate, AppGw3762ChanState_e chanstate, U16 waittime, MESG_PORT_e port);
void app_gw3762_up_afn00_f2_deny(AppGw3762ErrCode_e errcode, MESG_PORT_e port);
void app_gw3762_up_afn00_f2_deny_by_seq(AppGw3762ErrCode_e errcode, U8 Seq, MESG_PORT_e port);
void app_gw3762_up_afn02_f1_up_frame(U8 *Addr, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
void app_gw3762_up_afn06_f2_report_read_data(U8 *Addr, U16 meterIndex, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port);

#if (GW376213_PROTO_EN > 0)
void app_gw3762_up_afn06_f3_report_router_change(AppGw3762TaskChange_e taskchange, MESG_PORT_e port);
void app_gw3762_up_afn06_f4_report_dev_type(U8 num, U8 *addr, AppProtoType_e proto,
        U16 index, AppGw3762SlaveType_e devtype, U8 totalnum, U8 curnum, AppGw3762SubInfo_t *info, MESG_PORT_e port);
void app_gw3762_up_afn06_f5_report_slave_event(U8 LenNull_flag,AppGw3762SlaveType_e devtype, AppProtoType_e proto,U8 *meterAddr, U16 len, U8 *data);
#endif

void app_gw3762_up_afn13_f1_up_frame(U8 *Addr, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
void app_gw3762_up_afn14_f1_up_frame(AppGw3762Afn14F1Up_t AppGw3762Afn14F1Up);
void app_gw3762_up_afn14_f2_router_req_clock(MESG_PORT_e port);

#if (GW376213_PROTO_EN > 0)
void renew_all_mode_id_state();
void save_mode_id_by_addr(U8 *macaddr , U8* info ,U8 infolen);
#endif

void app_gw3762_up_info_field_slave_pack(AppGw3762UpInfoField_t *upinfofield, U8 mode, U8 *Nodeaddr, U8 localseq);
void app_gw3762_up_info_field_master_pack(AppGw3762UpInfoField_t *upinfofield, U8 *Nodeaddr);
U16 app_gw3762_fn_bin_to_bs(AppGw3762Fn_e data);
U16 app_gw3762_fn_bs_to_bin(U16 data);
void app_gw3762_afn14_f4_up_frame(U8 type,U8 *ID, U16 len,U8 phase,MESG_PORT_e port);
void app_gw3762_up_afnf1_f1_up_frame(U8 *Addr, AppProtoType_e proto, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
U8 app_gw3762_up_frame_encode(APPGW3762DATA_t *Gw3762Data_t, U8 *data, U16 *len);
void proc_uart_2_gw13762_data(uint8_t *revbuf,uint16_t len);
void proc_app_gw13762_data(uint8_t *revbuf,uint16_t len);
void send_gw3762_frame(U8 *buffer, U16 len, U8 seqNumber, U8 active, U8 retryTimes, MESG_PORT_e port, SERIAL_LID_e lid);
S16 seach_seq_by_mac_addr(U8 *addr);
U8 get_protocol_by_addr(U8 *addr);
U8 devicelist_ban_type(DEVICE_TEI_LIST_t DeviceListTemp);
U8 check_whitelist_by_addr(U8 *addr);
void flash_timer_start(void);
void app_gw3762_up_afn03_f10_master_mode(U8 mode, MESG_PORT_e port);
#endif

#endif

#endif

