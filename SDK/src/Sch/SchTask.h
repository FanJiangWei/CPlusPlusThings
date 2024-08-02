/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	mac.h
 * Purpose:	mac interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __SCHTASK_H__
#define __SCHTASK_H__

#include "ZCsystem.h"

/***********************************************Task*******************************************/

//SchTask
#define TASK_SCH_STK_SZ 512
#define TASK_PRIO_SCH 61
U32 SchTaskWork[TASK_SCH_STK_SZ];
S32 PidSchTask;

event_t mutexSch_t;

void MutexSchCreate();

/************************************************************************************/
#if defined(ZC3750CJQ2)
//CJQTask
#define TASK_CJQ_STK_SZ 0
#define TASK_PRIO_CJQ 52
U32 CJQTaskWork[TASK_CJQ_STK_SZ];
S32 PidCJQTask;
#endif



//#define    PROVINCE_GW              0x00U               /*����*/
//#define    PROVINCE_BEIJING         0x01U               /*������*/
//#define    PROVINCE_TIANJIN         0x02U               /*�����*/
#define    PROVINCE_SHANGHAI        0x03U               /*�Ϻ���*/
#define    PROVINCE_CHONGQING       0x04U               /*������*/
#define    PROVINCE_HEBEI           0x05U               /*�ӱ�ʡ*/
#define    PROVINCE_SHANXI          0x06U               /*ɽ��ʡ*/
//#define    PROVINCE_LIAONING        0x07U               /*����ʡ*/
//#define    PROVINCE_JILIN           0x08U               /*����ʡ*/
#define    PROVINCE_HEILONGJIANG    0x09U               /*������ʡ*/
#define    PROVINCE_JIANGSU         0x10U               /*����ʡ*/
//#define    PROVINCE_ZHEJIANG        0x11U               /*�㽭ʡ*/
#define    PROVINCE_ANHUI           0x12U               /*����ʡ*/
//#define    PROVINCE_FUJIAN          0x13U               /*����ʡ*/
//#define    PROVINCE_JIANGXI         0x14U               /*����ʡ*/
#define    PROVINCE_SHANDONG        0x15U               /*ɽ��ʡ*/
#define    PROVINCE_HENAN           0x16U               /*����ʡ*/
//#define    PROVINCE_HUBEI           0x17U               /*����ʡ*/
#define    PROVINCE_HUNAN           0x18U               /*����ʡ*/
//#define    PROVINCE_GUANGDONG       0x19U               /*�㶫ʡ*/
//#define    PROVINCE_HAINAN          0x20U               /*����ʡ*/
//#define    PROVINCE_SICHUAN         0x21U               /*�Ĵ�ʡ*/
//#define    PROVINCE_GUIZHOU         0x22U               /*����ʡ*/
//#define    PROVINCE_YUNNAN          0x23U               /*����ʡ*/
#define    PROVINCE_SHANNXI         0x24U               /*����ʡ*/
//#define    PROVINCE_GANSU           0x25U               /*����ʡ*/
//#define    PROVINCE_QINGHAI         0x26U               /*�ຣʡ*/
//#define    PROVINCE_TAIWAN          0x27U               /*̨��ʡ*/
//#define    PROVINCE_NEIMENGGU       0x28U               /*���ɹ�������*/
//#define    PROVINCE_GUANGXI         0x29U               /*����׳��������*/
//#define    PROVINCE_XIZANG          0x30U               /*����������*/
//#define    PROVINCE_NINGXIA         0x31U               /*���Ļ���������*/
//#define    PROVINCE_XINJIANG        0x32U               /*�½�ά���������*/
//#define    PROVINCE_JIBEI           0x33U               /*����*/
//#define    PROVINCE_AOMEN           0x34U               /*�����ر�������*/

#define    PROVINCE_NULL            0x35U

#define    VOLUME_PRODUCTION        0                   //����
#define    INSPECT_PRODUCTION       1                   //�ͼ�

#define ECC_KEY_STRING_LEN  	(164U)   /**< ECC��Կ�����ȣ���Կ��161��+ �Ӻ�У�飨1����*/
#define SM2_KEY_STRING_LEN      (164U)   /**< SM2��Կ�����ȣ���Կ��161��+ �Ӻ�У�飨1����*/

typedef struct{
    U8 ModuleID[11];
	U8 ModuleIDFattributes;
    U8 cs;
}__PACKED MODULE_ID_INFO_t;

typedef struct{
    double		PhySFODefault;
    U8			PhySFOFattributes;
    U8          cs;
}__PACKED PHY_SFO_INFO_t;

typedef struct{
    U8         ChipID[24];
	U8		   Used;	
    U8		   CS;
    
}__PACKED CHIP_ID_FLASH_t;


typedef struct{
    U8         Vender[2];
    U8         ProductFunc;
    U8         ChipCode;
    U8         ProductType;
    U8         Protocol;
    U8         Category[2];
    U8         HardwareVer[2];
    U16        ProductTime;     // Y 0~6bits   M 8~10bits   D 11~15bits
    U8		   edgetype;		
    U8		   Devicetype;
    U8		   Used;			
    U8		   CS;
    
}__PACKED HARDWARE_FEATURE_t;

typedef struct{
    U8         FreqBand;     //Ƶ��
    S16        tgaindig;
    S16        tgainana;
    U8         Res;
    U8		   Used;			
    U8		   CS;
} __PACKED FLASH_FREQ_INFO_t;

typedef struct{
    U8         AddrMode;                                //��ַģʽ
    U8         DeviceBaseAddr[MACADRDR_LENGTH];			//�е�ַģʽΪ�ɼ������м���ʹ��ģ��ID����
    U8         CCOBaseMode;                             //CCO��ģʽ
    U8         CCOBaseAddr[MACADRDR_LENGTH];			//�е�ַģʽΪCCOʹ��ģ��ID����
    U8		   Used;			
    U8		   CS;
} __PACKED DEVICE_ADDR_INFO_t;
typedef struct{
    U32        UseMode             :1;           //�Ƿ�����Ӧ�ù��ܿ���
    U32        DebugeMode          :1;			 //����ģʽ����
    U32        NoiseDetectSWC      :1;			 //������⿪��
    U32        WhitelistSWC        :1;			 //����������
    U32        UpgradeMode         :1;			 //����ģʽ����
    U32        AODVSWC             :1;			 //AODV����
    U32        EventReportSWC      :1;			 //�¼��ϱ�����
    U32        ModuleIDGetSWC      :1;			 //ģ��ID��ȡ����
    U32        PhaseSWC            :1;			 //��λʶ�𿪹�
    U32        IndentifySWC        :1;			 //����̨��ʶ��Ĭ�Ͽ�������
    U32        DataAddrFilterSWC   :1;			 //��������֡��ַ���˿���
    U32        NetSenseSWC         :1;			 //������֪����
    U32        SignalQualitySWC    :1;			 //376.2������Ϣ���ź�Ʒ�ʿ���
    U32        SetBandSWC          :1;			 //376.2����Ƶ�ο���
    U32        SwPhase             :1;			 //���࿪��
    U32		   oop20EvetSWC		   :1;			 //20�����¼��ϱ����أ�1��ʾ��20����ģʽ�ϱ���0��ʾ�Զ�״̬��ģʽ�ϱ�
    
    U32		   oop20BaudSWC		   :1;			 //20��������Э�̿��أ�1��ʾ���ã�0��ʾ��ֹ
    U32        JZQBaudSWC          :1;			 //JZQ�����ʹ��ܿ��أ�
    U32        MinCollectSWC       :1;           //���Ӳɼ�����
    U32        IDInfoGetModeSWC    :1;			 //ģ���ϵ�ID��ȡ���ܿ��أ�1��ʾ�ͼ�򿪣�0��ʾ��������
    U32        TransModeSWC        :1;           //���ֵ��ز���˫ģ��������Ĵ���ģʽ1��ʾ˫ģ,0��ʾ���ز�
    U32        AddrShiftSWC	       :1;	         //������ַ�л����أ����ת����ģ��ʹ�ã�
    U32        Res                 :10;			 //����
    
    U32        Res1;
    U16        Res2;
    U8		   Used;			
    U8		   CS;
} __PACKED FUNTION_SWITCH_t;
typedef struct
{
    U8	      ucVendorCode[2];
    U8	      ucChipCode[2];
    U8	      ucDay;
    U8	      ucMonth;
    U8	      ucYear;
    U8	      ucVersionNum[2];
	U8        res;
	U8		  Used;			
    U8		  CS;
} __PACKED OUT_MF_VERSION_t;
typedef struct
{
	U8        UseMode;              //ʹ�ñ�־
    U8        ConcurrencySize;		//������������
    U8        ReTranmitMaxNum;		//�س�����
    U8        ReTranmitMaxTime;     //�س�ʱ��
    
    U8        BroadcastConRMNum;    //�㲥��������
    U8        AllBroadcastType;     //�㲥����
    U8        JZQBaudCfg;			//���������ڲ�����
	U8        res[11];
	U8        Used;			
    U8		  CS;
} __PACKED PARAMETER_CFG_t;

typedef struct
{
    U16   option;
    U16   channel;
	U8    resv;
	U8	  Used;			
    U8	  CS;
}__PACKED Rf_CHANNEL_INFO_t;

typedef struct{
    MODULE_ID_INFO_t         ModuleIDInfo;
    PHY_SFO_INFO_t		     PhySFOInfo;
	CHIP_ID_FLASH_t 		 ChipIdFlash_t;
	HARDWARE_FEATURE_t 		 Hardwarefeature_t;  //1
    FLASH_FREQ_INFO_t        FreqInfo;           //2
	DEVICE_ADDR_INFO_t       DeviceAddrInfo;     //3
	FUNTION_SWITCH_t         FunctionSwitch;     //4������
    OUT_MF_VERSION_t         OutMFVersion;
	PARAMETER_CFG_t          ParameterCFG;       //5������
    Rf_CHANNEL_INFO_t        RfChannelInfo;
}__PACKED DEFAULT_SETTING_INFO_t;


typedef enum
{
    e_ECC_KEY = 0,
    e_SM2_KEY
}ECC_SM2_KEY_t;

typedef union
{
    INT8U buf[ECC_KEY_STRING_LEN];
    struct
    {
        INT8U ecc_public_key[64];               /**< ECC��Կ */
        INT8U ecc_identity_signature[64];       /**< ECC���ǩ�� */
        INT8U ecc_private_key[32];              /**< ECC˽Կ */
        INT8U ecc_curve_flag;                   /**< ECC���߱�ʶ */
        INT8U used;                             /**< ʹ�ñ�־��Z'*/
        INT8U resv;                             /**< ����*/
        INT8U cs;                               /**< �Ӻ�У�� */
    }value;
} __PACKED ECC_KEY_STRING_t;

typedef union
{
    INT8U buf[SM2_KEY_STRING_LEN];
    struct
    {
        INT8U sm2_public_key[64];               /**< SM2��Կ */
        INT8U sm2_identity_signature[64];       /**< SM2���ǩ�� */
        INT8U sm2_private_key[32];              /**< SM2˽Կ */
        INT8U sm2_curve_flag;                   /**< SM2���߱�ʶ */
        INT8U used;                             /**< ʹ�ñ�־��Z'*/
        INT8U resv;                             /**< ����*/
        INT8U cs;                               /**< �Ӻ�У�� */
    }value;
} __PACKED SM2_KEY_STRING_t;

typedef struct 
{
   ECC_KEY_STRING_t ecc_key_string;
   SM2_KEY_STRING_t sm2_key_string;
} __PACKED ECC_SM2_KEY_STRING_t;

extern DEFAULT_SETTING_INFO_t DefSetInfo;


typedef struct{
	U32	 MID         :1;
	U32	 PhySFO      :1;	
	U32	 CID         :1;
	U32	 HWFeatrue   :1;
    U32  FREQ        :1;
	U32  DevAddr     :1;
	U32  FunSWC      :1;
	U32  OMF         :1;
	U32  ParaCFG     :1;
    U32  RfChannel   :1;
	U32  Res		 :6;
	
    U32	 Res1        :16;
}__PACKED DEFAULT_WRITE_FLAG_t;

extern DEFAULT_WRITE_FLAG_t	DefwriteFg;


typedef enum
{
     DEF_ERR                    =0x00,
     HARDWARE_DEF 				=0x01,
	 FLASH_FREQ_DEF 			=0x02,
	 DEVICE_ADDR_DEF 			=0x03,
	 FUNC_SWITCH_DEF 			=0x04,
	 PARAMETER_CFG              =0x05,
	 MODULEID_DEF				=0x06,
	 PHYSFO_DEF					=0x07,
	 CHIPID_DEF					=0x08,
	 OUTVER_DEF					=0x09,
	 RfCHANNEL_DEF				=0x0A,
}DEFAULT_ITEM_e;


#define REBOOT_INFO_MAX 10

typedef struct
{
    U32     Cnt[REBOOT_INFO_MAX];
    U32     Reason[REBOOT_INFO_MAX];
    U8	    Cs;
    U8	    Used;
    U8      Res[2];
}__PACKED REBOOT_INFO_t;

#define  APP_EXT_INFO_SIZE      (51U)
#define  APP_EXT_INFO_MAX_SIZE  (510U)

#define  APP_EXT_FUNC_SWITCH_ADDR   (EXT_INFO_ADDR + 19U)
#define  APP_EXT_FUNC_SWITCH_LEN    sizeof(FUNTION_SWITCH_t)//(12U)
#define  APP_EXT_PARAM_CFG_ADDR     (APP_EXT_FUNC_SWITCH_ADDR + APP_EXT_FUNC_SWITCH_LEN)
#define  APP_EXT_PARAM_CFG_LEN      sizeof(PARAMETER_CFG_t)//(20U)

#define  APP_EXT_FUNC_SWITCH_OFFSET     (19U)
#define  APP_EXT_PARAM_CFG_OFFSET       (APP_EXT_FUNC_SWITCH_OFFSET + APP_EXT_FUNC_SWITCH_LEN)

typedef struct
{
    union
    {
        INT8U                   buf[APP_EXT_INFO_SIZE];
        struct
        {
            INT16U              ex_info_crc16;              /*CRC16У�飬�����ֶ�����������*/
            INT16U              ex_info_len;                /*���ȣ���crc16�ֶ�*/
            INT32U              water_mark;                 /*ˮӡ��0xAABBCCDD*/
            INT8U               province_code;              /*ʡ�ݱ�ʶ��BCD*/
            INT8U               version[2];                 /*�ڲ�����汾��BCD��С��*/
            INT8U               date[3];                    /*�ڲ�������ڣ�BCD��С��*/
            INT8U               out_version[2];             /*�ⲿ����汾��BCD��С��*/
            INT8U               out_date[3];                /*�ⲿ�汾���ڣ�BCD��С��*/
            FUNTION_SWITCH_t    func_switch;                /*Ӧ�ù��ܿ���-12�����*/
            PARAMETER_CFG_t     param_cfg;                  /*��������-20�����*/
        };
    };
} __PACKED AppExtInfo_t;
extern AppExtInfo_t app_ext_info;

void execp_reoot();

void SchTaskInit();


uint8_t DefaultSettingFlag;
uint8_t  SetDefaultInfo();
uint32_t  ReadDefaultSettingInfo(DEFAULT_SETTING_INFO_t* pDefaultinfo);
extern U8 setoutversion(U8 *vender,U8 *chipversion,U8 *date,U8 *version);

extern U8 ModuleID[11];
extern OUT_MF_VERSION_t OutVerFromBin;
extern U8    ImghdrData[40];
#if defined(STATIC_MASTER)



extern U32 reboot_cnt;

extern uint8_t sysflag;
extern uint8_t whitelistflag;
extern uint8_t CCOImageWriteFlag;
extern uint8_t CcoUpgrdCtrlInfoWrFlag;
void SaveInfo(uint8_t *sysinfo_flag ,uint8_t *whitelist_flag);
void Readsysinfo(void);

void CcoWriteImage(void);
void CcoLoadSlaveImage(void);
void CcoLoadSlaveImage1(void);

void CcoUpgrdCtrlInfoWrite(void);
void CcoUpgrdCtrlInfoRead(void);

#endif

void ReadBoardCfgInfo(void);
U8 ReadDefaultInfo(void);
U8 ReadExtInfo(void);
U8 ReadFreqInfo(void);


void ReadOutVersion(void);
void ReadBinOutVersion(U8 *Data);


#if defined(STATIC_NODE)

void SaveAccessInfo(void);

void ReadAccessInfo(void);

void WriteUpgradeImage(void);

void SaveUpgradeStatusInfo(void);
void ReadStaUpgradeStatusInfo(void);

extern uint8_t staflag;

extern uint8_t UpgradeStaFlag;
extern uint8_t ImageWriteFlag;
extern uint8_t 	check_upgrade_info;
extern uint8_t 	reset_run_image;



#endif

uint32_t ReadFreqSetInfo(void* info);

extern REBOOT_INFO_t  RebootInfo;
void check_reboot_info(U32 RebooReason);

/**
* @brief	image�Ƿ�Я��ext_info���
* @param	void
* @return	TRUE-Я��ext_info��FALSE-��Я��ext_info
*******************************************************************************/
INT8U app_check_ext_info(void);

/**
* @brief	��ȡEXT_INFO����Ϣ�������м��
* @param	void
* @return	void
*******************************************************************************/
void app_ext_info_read_and_check(void);

/**
* @brief	������ɺ�дext_info
* @param	void
* @return	void
*******************************************************************************/
void app_ext_info_write_after_upgrade(void);

/**
* @brief	д����
* @param	void *p_data: ��д����
* @return	void
*******************************************************************************/
INT8U app_ext_info_write_param(INT8U *p_data, INT16U len);
/**
 * @brief   ���������ŵ���Ϣ
 * 
 */
void save_Rf_channel(U8 option, U8 channel);

#endif /* __MAC_H__ */
