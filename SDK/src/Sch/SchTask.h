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



//#define    PROVINCE_GW              0x00U               /*国网*/
//#define    PROVINCE_BEIJING         0x01U               /*北京市*/
//#define    PROVINCE_TIANJIN         0x02U               /*天津市*/
#define    PROVINCE_SHANGHAI        0x03U               /*上海市*/
#define    PROVINCE_CHONGQING       0x04U               /*重庆市*/
#define    PROVINCE_HEBEI           0x05U               /*河北省*/
#define    PROVINCE_SHANXI          0x06U               /*山西省*/
//#define    PROVINCE_LIAONING        0x07U               /*辽宁省*/
//#define    PROVINCE_JILIN           0x08U               /*吉林省*/
#define    PROVINCE_HEILONGJIANG    0x09U               /*黑龙江省*/
#define    PROVINCE_JIANGSU         0x10U               /*江苏省*/
//#define    PROVINCE_ZHEJIANG        0x11U               /*浙江省*/
#define    PROVINCE_ANHUI           0x12U               /*安徽省*/
//#define    PROVINCE_FUJIAN          0x13U               /*福建省*/
//#define    PROVINCE_JIANGXI         0x14U               /*江西省*/
#define    PROVINCE_SHANDONG        0x15U               /*山东省*/
#define    PROVINCE_HENAN           0x16U               /*河南省*/
//#define    PROVINCE_HUBEI           0x17U               /*湖北省*/
#define    PROVINCE_HUNAN           0x18U               /*湖南省*/
//#define    PROVINCE_GUANGDONG       0x19U               /*广东省*/
//#define    PROVINCE_HAINAN          0x20U               /*海南省*/
//#define    PROVINCE_SICHUAN         0x21U               /*四川省*/
//#define    PROVINCE_GUIZHOU         0x22U               /*贵州省*/
//#define    PROVINCE_YUNNAN          0x23U               /*云南省*/
#define    PROVINCE_SHANNXI         0x24U               /*陕西省*/
//#define    PROVINCE_GANSU           0x25U               /*甘肃省*/
//#define    PROVINCE_QINGHAI         0x26U               /*青海省*/
//#define    PROVINCE_TAIWAN          0x27U               /*台湾省*/
//#define    PROVINCE_NEIMENGGU       0x28U               /*内蒙古自治区*/
//#define    PROVINCE_GUANGXI         0x29U               /*广西壮族自治区*/
//#define    PROVINCE_XIZANG          0x30U               /*西藏自治区*/
//#define    PROVINCE_NINGXIA         0x31U               /*宁夏回族自治区*/
//#define    PROVINCE_XINJIANG        0x32U               /*新疆维吾尔自治区*/
//#define    PROVINCE_JIBEI           0x33U               /*冀北*/
//#define    PROVINCE_AOMEN           0x34U               /*澳门特别行政区*/

#define    PROVINCE_NULL            0x35U

#define    VOLUME_PRODUCTION        0                   //量产
#define    INSPECT_PRODUCTION       1                   //送检

#define ECC_KEY_STRING_LEN  	(164U)   /**< ECC密钥串长度（密钥（161）+ 加和校验（1））*/
#define SM2_KEY_STRING_LEN      (164U)   /**< SM2密钥串长度（密钥（161）+ 加和校验（1））*/

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
    U8         FreqBand;     //频段
    S16        tgaindig;
    S16        tgainana;
    U8         Res;
    U8		   Used;			
    U8		   CS;
} __PACKED FLASH_FREQ_INFO_t;

typedef struct{
    U8         AddrMode;                                //地址模式
    U8         DeviceBaseAddr[MACADRDR_LENGTH];			//有地址模式为采集器或中继器使用模块ID倒序
    U8         CCOBaseMode;                             //CCO绑定模式
    U8         CCOBaseAddr[MACADRDR_LENGTH];			//有地址模式为CCO使用模块ID倒序
    U8		   Used;			
    U8		   CS;
} __PACKED DEVICE_ADDR_INFO_t;
typedef struct{
    U32        UseMode             :1;           //是否启用应用功能开关
    U32        DebugeMode          :1;			 //调试模式开关
    U32        NoiseDetectSWC      :1;			 //噪声检测开关
    U32        WhitelistSWC        :1;			 //白名单开关
    U32        UpgradeMode         :1;			 //升级模式开关
    U32        AODVSWC             :1;			 //AODV开关
    U32        EventReportSWC      :1;			 //事件上报开关
    U32        ModuleIDGetSWC      :1;			 //模块ID获取开关
    U32        PhaseSWC            :1;			 //相位识别开关
    U32        IndentifySWC        :1;			 //北京台区识别默认开启开关
    U32        DataAddrFilterSWC   :1;			 //抄表上行帧地址过滤开关
    U32        NetSenseSWC         :1;			 //离网感知开关
    U32        SignalQualitySWC    :1;			 //376.2上行信息域信号品质开关
    U32        SetBandSWC          :1;			 //376.2设置频段开关
    U32        SwPhase             :1;			 //切相开关
    U32		   oop20EvetSWC		   :1;			 //20版电表事件上报开关，1表示以20版电表模式上报，0表示以读状态字模式上报
    
    U32		   oop20BaudSWC		   :1;			 //20版电表波特率协商开关，1表示启用，0表示禁止
    U32        JZQBaudSWC          :1;			 //JZQ波特率功能开关；
    U32        MinCollectSWC       :1;           //分钟采集开关
    U32        IDInfoGetModeSWC    :1;			 //模块上电ID获取功能开关，1表示送检打开，0表示量产供货
    U32        TransModeSWC        :1;           //区分单载波和双模控制域里的传输模式1表示双模,0表示单载波
    U32        AddrShiftSWC	       :1;	         //入网地址切换开关（光伏转换器模块使用）
    U32        Res                 :10;			 //保留
    
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
	U8        UseMode;              //使用标志
    U8        ConcurrencySize;		//并发抄表并发数
    U8        ReTranmitMaxNum;		//重抄次数
    U8        ReTranmitMaxTime;     //重抄时间
    
    U8        BroadcastConRMNum;    //广播抄读条数
    U8        AllBroadcastType;     //广播类型
    U8        JZQBaudCfg;			//集中器串口波特率
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
	FUNTION_SWITCH_t         FunctionSwitch;     //4，保留
    OUT_MF_VERSION_t         OutMFVersion;
	PARAMETER_CFG_t          ParameterCFG;       //5，保留
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
        INT8U ecc_public_key[64];               /**< ECC公钥 */
        INT8U ecc_identity_signature[64];       /**< ECC身份签字 */
        INT8U ecc_private_key[32];              /**< ECC私钥 */
        INT8U ecc_curve_flag;                   /**< ECC曲线标识 */
        INT8U used;                             /**< 使用标志‘Z'*/
        INT8U resv;                             /**< 保留*/
        INT8U cs;                               /**< 加和校验 */
    }value;
} __PACKED ECC_KEY_STRING_t;

typedef union
{
    INT8U buf[SM2_KEY_STRING_LEN];
    struct
    {
        INT8U sm2_public_key[64];               /**< SM2公钥 */
        INT8U sm2_identity_signature[64];       /**< SM2身份签字 */
        INT8U sm2_private_key[32];              /**< SM2私钥 */
        INT8U sm2_curve_flag;                   /**< SM2曲线标识 */
        INT8U used;                             /**< 使用标志‘Z'*/
        INT8U resv;                             /**< 保留*/
        INT8U cs;                               /**< 加和校验 */
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
            INT16U              ex_info_crc16;              /*CRC16校验，除本字段外所有数据*/
            INT16U              ex_info_len;                /*长度，除crc16字段*/
            INT32U              water_mark;                 /*水印：0xAABBCCDD*/
            INT8U               province_code;              /*省份标识，BCD*/
            INT8U               version[2];                 /*内部软件版本，BCD，小端*/
            INT8U               date[3];                    /*内部软件日期，BCD，小端*/
            INT8U               out_version[2];             /*外部软件版本，BCD，小端*/
            INT8U               out_date[3];                /*外部版本日期，BCD，小端*/
            FUNTION_SWITCH_t    func_switch;                /*应用功能开关-12，大端*/
            PARAMETER_CFG_t     param_cfg;                  /*参数配置-20，大端*/
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
* @brief	image是否携带ext_info检查
* @param	void
* @return	TRUE-携带ext_info，FALSE-不携带ext_info
*******************************************************************************/
INT8U app_check_ext_info(void);

/**
* @brief	读取EXT_INFO区信息，并进行检查
* @param	void
* @return	void
*******************************************************************************/
void app_ext_info_read_and_check(void);

/**
* @brief	升级完成后，写ext_info
* @param	void
* @return	void
*******************************************************************************/
void app_ext_info_write_after_upgrade(void);

/**
* @brief	写参数
* @param	void *p_data: 待写数据
* @return	void
*******************************************************************************/
INT8U app_ext_info_write_param(INT8U *p_data, INT16U len);
/**
 * @brief   保存无线信道信息
 * 
 */
void save_Rf_channel(U8 option, U8 channel);

#endif /* __MAC_H__ */
