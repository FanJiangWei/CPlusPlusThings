/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     Apstask.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __APS_LAYER_H__
#define __APS_LAYER_H__

//#include "ZCsystem.h"
#include "types.h"
#include "que.h"
#include "app_base_multitrans.h"



#define    PROTOCOL_VERSION_NUM     1

#define    NO_REPLY_RETRY_FLAG      1           // 未应答重试标志
#define    NACK_RETRY_FLAG          1          // 否认重试标志
#define    MAX_RETRY_NUM            1          // 最大重试次数

#define    APS_PACKET_CTRLWORD     0

// app link id define  
#define    READ_METER_LID                             4
#define    TIME_CALIBRATE_LID                         3
#define    EVENT_REPORT_LID                           3
#define    COMMU_TEST_LID                             3
#define    AUTHENTICATION_SECURITY_LID                3
#define    RDCTRL_CCO_LID                             3
#define    RDCTRL_TO_UART_LID                         3
#define    SLAVE_REGISTER_LID                         2
#define    UPGRADE_LID                                2
#define    INDENTIFICATION_LID                        2

#define    STA_INFO_LIST_MAXNUM    6

#define    APS_MALLOC_PACKET_SIZE      64
#define    APS_RESERVE_PACKET_SIZE    60


#define aps_printf(fmt, arg...)  debug_printf(&dty, DEBUG_APP, fmt, ##arg)

typedef enum
{
    e_READMETER_PACKET_PORT        = 0x11,
    e_UPGRADE_PACKET_PORT          = 0x12,
    e_AUTHENTICATION_SECURITY_PORT = 0x1A,
    e_INDENTIFICATION_AREA_PORT    = 0x11,
} ApsPacketPort_e;

typedef enum
{
    e_CONCENTRATOR_ACTIVE_RM = 0x01,
    e_ROUTER_ACTIVE_RM,
    e_CONCENTRATOR_ACTIVE_CONCURRENT,
    e_TIME_CALIBRATE,
    e_COMMU_TEST = 0x06,
    e_EVENT_REPORT = 0x08,
    e_ZONEAREA_INFO_GATHER = 0x09,
    e_SLAVE_REGISTER_QUERY = 0x11,
    e_SLAVE_REGISTER_START,
    e_SLAVE_REGISTER_STOP,
    e_ACK_NAK = 0x20,
    e_DATA_AGGREGATE_RM = 0x21,
    e_TIME_OVER_MANAGE = 0x28,
    e_TIME_OVER_QYERY,
    e_UPGRADE_START = 0x30,
    e_UPGRADE_STOP,
    e_FILE_TRANS,
    e_FILE_TRANS_LOCAL_BROADCAST,
    e_UPGRADE_STATUS_QUERY,
    e_UPGRADE_PERFORM,
    e_STATION_INFO_QUERY,
    e_RDCTRL_CCO = 0x40,
    e_RDCTRL_TO_UART = 0x41,
    e_RTC_TIME_SYNC = 0x70,//江苏独立使用
    e_AUTHENTICATION_SECURITY = 0xA0,
   	e_INDENTIFICATION_AREA = 0xA1,
    e_QUERY_ID_INFO = 0xA2,
    e_EXACT_CALIBRATE_TIME,
    
    e_ZERO_FIRE_LINE_ENABLED = 0xB0,
    e_MODULE_TIME_SYNC = 0xB1,
    e_CURVE_PROFILE_CFG = 0xB2,
	e_CURVE_DATA_CONCRRNT_READ = 0xB3,
	e_NODE_SGS = 0xC1,
	e_SYS_IDLE,
} ApsPacketID_e;
	




typedef enum
{
    e_DOWNLINK     = 0,
    e_UPLINK       = 1,
}DIRECTION_e;

typedef enum
{
    e_QUERY_REGISTER_RESULT = 0,				//查询从节点注册结果命令
    e_START_SLAVE_REGISTER   = 1,				//启动从节点主动注册命令
    e_LOCK_CMD = 2,								//锁定命令
    e_CMD_RES = 3,								//保留
    e_QUERY_WATER_CMD = 4,						//查询水表命令
    e_QUERY_COLLECTDATA = 5,					//查询分钟冻结命令 //TODO：江苏独立
} REGISTER_PARAMETER_e;

typedef enum
{
	e_CCO_CNF_STA=1,
	e_CCO_ISSUE_PERMIT_STA_REPORT,
	e_CCO_ISSUE_FORBID_STA_REPORT,
	e_CCO_ACK_BUF_FULL_STA,
}EVENT_REPORT_DWN_LINK_FUN_CODE_e;
typedef enum
{
	e_METER_ACT_CCO=1,
    e_STA_ACT_CCO ,
    e_CJQ2_ACT_CCO , 
}EVENT_REPORT_UP_LINK_FUN_CODE_e;
typedef enum
{
    e_APP_PACKET_FORWARD_UART = 1,
    e_APP_PACKET_FORWARD_CARRIER,
    e_PHY_TRANSPARENT_MODE,                 //HPLC 物理层透传
    e_PHY_RETURN_MODE,                      //HPLC 物理层回传
    e_MAC_TRANSPARENT_MODE,                 //MAC层透传
    e_CARRIER_BAND_CHANGE,                  //载波频段切换
    e_TONEMASK_CFG,                         //ToneMask配置
    e_RFCHANNEL_CFG,                        //无线信道切换 Option:O2字节的4-7bit,信道号:03字节
    e_WPHY_RETURN_MODE,                     //RF物理层回传
    e_WPHY_TRANSPARENT_MODE,                //RF物理层透传
    e_RF_HPLC_RETURN_MODE,                  //RF/PLC物理层回传
    e_HPLC_TO_RF_RETURN_MODE,                //PLC2RF物理层回传
    e_SAFETEST_MODE = 13,                   //数据域： 1:SHA256, 2:SM3, 3:ECC签名, 4:ECC验签 。。。。12：SM3-CBC解密测试模式
    e_RESERVE,
}TEST_MODE_CFG_e;




typedef struct
{
    U8      action;
} __PACKED NETWORK_ACTION_t;



typedef struct
{
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED SLAVE_REGISTER_START_IND_t;



typedef struct
{
    U8    Status;
} __PACKED SLAVE_REGISTER_START_CFM_t;




typedef struct
{
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED SLAVE_REGISTER_STOP_IND_t;



typedef struct
{
    U32      Status;
} __PACKED FILE_TRANS_CFM_t;



typedef struct
{
    U32      Status;
} __PACKED TIME_CALIBRATE_CFM_t;


typedef struct
{
    U8      Status;
    U16     PacketID;
    U16     ApsSeq;
} __PACKED READ_METER_CFM_t;





























typedef struct
{
    U8    Direct;
    U8    GatherType;
    U16   AsduLength;
    
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
}__PACKED ZONEAREA_INFO_IND_t;










typedef struct
{
	U16   stei;
    U16   DatagramSeq;

    U32   SNID;
    
	U8    Featuretypes;
	U8    Collectiontype;
    
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

	U16   payloadlen;
    U8    payload[0];
}__PACKED INDENTIFICATION_IND_t;


typedef struct
{
    U8         IdType;
    U8         Res1;
    U16        Res2;

    U16        PacketSeq;
	U16        Srctei;
    
    U8         DstMacAddr[MACADRDR_LENGTH];
    U8         SrcMacAddr[MACADRDR_LENGTH];
} __PACKED QUERY_IDINFO_IND_t;


typedef struct
{
    U8    IdType;
    U8    Status;
    U16   AsduLength;
    
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
}__PACKED QUERY_IDINFO_CFM_t;




typedef struct
{
	U8    protocol;
    U16   len;
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
}__PACKED RD_CTRL_IND_t;

typedef struct
{
    U8    Status;
}__PACKED RD_CTRL_CFM_t;



// APS layer frame header define

typedef struct
{
    U32    PacketPort      :8;
    U32    PacketID        :16;
    U32    PacketCtrlWord  : 8;

	U8     Apdu[0];
} __PACKED APDU_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    ResponseState              : 4;
    U32    DataProType                : 4;
    U32    DataLength                 : 12;
    U32    PacketSeq                  : 16;
    U32    OptionWord                 : 16;

	U8     Payload[0];
} __PACKED READMETER_UPLINK_HEADER_t;


typedef struct
{
    U32    ProtocolVer     : 6;
    U32    HeaderLen       : 6;
    U32    CfgWord         : 4;
    U32    DataProType     : 4;
    U32    DataLength      : 12;
    U32    PacketSeq       : 16;
    U32    DeviceTimeout   : 8;
    U32    OptionWord      : 8;

	U8     Payload[0];
} __PACKED READMETER_DOWNLINK_HEADER_t;



typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    ForcedResFlag           : 1;
    U32    RegisterParameter        : 3;
    U32    Reserve                        : 16;
    U32    PacketSeq;
} __PACKED REGISTER_START_HEADER_t;



typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Status                     : 1;
    U32    RegisterParameter          : 3;
    U32    MeterCount                 : 8;
    U32    DeviceType                 : 8;
    
    U8     DeviceAddr[6];
    U8     DeviceId[6];
    U32    PacketSeq;
    U32    Reserve;
    U8     SrcMacAddr[6];
    U8     DstMacAddr[6];

    U8     MeterInfo[0];
} __PACKED REGISTER_QUERY_UP_HEADER_t;


typedef struct
{
    U16    ProtocolVer     : 6;
    U16    HeaderLen       : 6;
    U16    Directbit       : 1;
    U16    Startbit        : 1;
    U16    Phase           : 2;
    U16    PacketSeq;
	U8     Macaddr[6];
	U8     Featuretypes;
	U8     Collectiontype;

	U8     Payload[0];
} __PACKED INDENTIFICATION_NEW_HEADER_t;

typedef struct
{
    U32    ProtocolVer             : 6;
    U32    HeaderLen               : 6;
    U32    ForcedResFlag           : 1;
    U32    RegisterParameter       : 3;
    U32    Reserve                 : 16;
    U32    PacketSeq;
    U8     SrcMacAddr[6];
    U8     DstMacAddr[6];

    U8     Payload[0];
} __PACKED REGISTER_QUERY_DWN_HEADER_t;



typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve1                    : 4;
    U32    Reserve2                    : 16;
    U32    PacketSeq;
} __PACKED REGISTER_STOP_HEADER_t;

typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve1                    : 4;
    U32    Reserve2                    : 4;
    U32    DataLength                 : 12;

    U8     TimeData[0];
} __PACKED TIME_CALIBRATE_HEADER_t;

typedef struct
{
    U32    ProtocolVer               : 6;
    U32    HeaderLen                 : 6;
    U32    TestModeCfg               : 4;   // 测试模式、频段切换、ToneMask、无线信道切换、MCS
    U32    ProtocolType              : 4;
    U32    DataLength                : 12;  // 测试模式持续时间、频段值、ToneMask值、Option值和无线信道号、MCS值

    U16    SafeTestMode              :4;
//    U16    res                       :12;   //plc到Rf物理层回传模式参数 PHR_MCS：4bit， PSDU_MCS：4bit， PbSIZE：4bit
    U16    phr_mcs                   :4;
    U16    psdu_mcs                  :4;
    U16    pbsize                    :4;

    U8     Asdu[0];
} __PACKED COMMU_TEST_HEADER_t;

typedef struct
{
    U32    ProtocolVer               : 6;
    U32    HeaderLen                 : 6;
    //    U32    res                       : 4;   // 测试模式、频段切换、ToneMask、无线信道切换、MCS
    U32    TestModeCfg                       : 4;   // 测试模式、频段切换、ToneMask、无线信道切换、MCS
    U32    ProtocolType              : 4;
    U32    DataLength                : 12;  // 测试模式持续时间、频段值、ToneMask值、Option值和无线信道号、MCS值

    U8     Asdu[0];
} __PACKED SAFE_TEST_HEADER_t;

typedef SAFE_TEST_HEADER_t APS_TESET_HEADER_t;

typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                      : 20;
    U32    UpgradeID;
    U16    UpgradeTimeout;
    U16    BlockSize;
    U32    FileSize;
    U32    FileCrc;
} UPGRADE_START_DWN_HEADER_t;

typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                      : 12;
    U32    StartResultCode          : 8;
    U32    UpgradeID;
} __PACKED UPGRADE_START_UP_HEADER_t;

typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                      : 20;
    U32    UpgradeID;
} __PACKED UPGRADE_STOP_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                      : 4;
    U32    BlockSize                   : 16;
    U32    UpgradeID;
    U32    BlockSeq;

    U8     DataBlock[0];
} __PACKED FILE_TRANS_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                    : 4;
    U32    QueryBlockCount            : 16;
    U32    StartBockIndex;
    U32    UpgradeID;
} UPGRADE_STATUS_DWN_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    UpgradeStatus           : 4;
    U32    ValidBlockCount         : 16;
    U32    StartBlockIndex;
    U32    UpgradeID;

    U8     BlockInfoBitMap[0];
} __PACKED UPGRADE_STATUS_UP_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                      : 4;
    U32    WaitResetTime           : 16;
    U32    UpgradeID;
    U32    TestRunTime;
} __PACKED UPGRADE_PERFORM_HEADER_t;


typedef struct
{    U32    ProtocolVer                : 6;    
	 U32    HeaderLen                  : 6;    
	 U32    Reserve                    : 4;    
	 U32    Res                        : 8;    
	 U32    InfoListNum                : 8;

     U8     InfoList[0];
} __PACKED STATION_INFO_DWN_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Reserve                    : 12;
    U32    InfoListNum                : 8;
    U32    UpgradeID;

    U8     InfoData[0];
} __PACKED STATION_INFO_UP_HEADER_t;


typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Dir                        : 1;
    U32    Prm                        : 1;
    U32    FunCode                    : 6;
    U32    EventDataLen               : 12;
    U16    PacketSeq;
    U8     MeterAddr[6];

    U8     EventData[0];
} __PACKED EVENT_REPORT_HEADER_t;

typedef struct
{
    U32    ProtocolVer                : 6;
    U32    HeaderLen                  : 6;
    U32    Dir                               : 1;
    U32    AckOrNak                    : 1;
    U32    Reserve                      : 2;
    U32    PacketSeq                   : 16;
    U16    OptionWord;
} __PACKED ACK_NAK_HEADER_t;

typedef struct
{
    U32   ProtocolVer                : 6;
    U32   HeaderLen                  : 6;
    U32   Dir                        : 1;
    U32   GaterhType                 :3;
    U32   DataLength                 :16;

    U8    Asdu[0];
}__PACKED ZONEAREA_INFO_HEADER_t;

typedef struct
{
    U32   ProtocolVer                 :6;
    U32   HeaderLen                  :6;
    U32   Dir                              :1;
    U32   IdType                        :3;
    U32   PacketSeq                   :16;

    U8    Asdu[0];
}__PACKED QUERY_IDINFO_HEADER_t;

typedef struct
{
	U8   protocol;
	U16  len;

    U8   Asdu[0];
}__PACKED RDCTRL_HEADER_t;

typedef struct
{
	U8   Protocol;
    U8   Dir                        :1;
    U8   Res                        :3;
    U32  Baud;
    U32  Res2;
	U16  Len;

    U8   Asdu[0];
}__PACKED RDCTRL_TO_UART_HEADER_t;

typedef struct
{
	U8		cmd:4;
	U8		res:4;
}__PACKED QUERY_WATER_HEADINFO_t;






typedef struct{
	uint8_t feature;

	uint8_t payload[0];
}INDENTIFICATION_FEATURE_IND_t;













typedef struct
{
    ApsPacketID_e   PacketId;
    void (* Func)   (work_t *work);
}__PACKED ApsPacketIdFunc;
extern U32	 ApsRecvRegMeterSeq;
extern U32	 ApsSendRegMeterSeq;

extern U32   ApsSendPacketSeq;
extern U32   ApsRecvPacketSeq;

extern U16   ApsEventSendPacketSeq ;
extern U16   ApsEventRecvPacketSeq ;

extern  U8   ApsHandle;


//extern U8 *MdbPush(MDB_t  *mdb, U16 len);
//extern void MdbReserve(MDB_t  *mdb, U16 len);
//extern int MallocMdb(MDB_t  *mdb, U16  size);
//extern U8 *MdbPull(MDB_t  *mdb, U16 len);
void UpAsduSeq();

U8 CheckSTAOnlineSate();

void ApsPostPacketReq(work_t *work, U16 msduLen, U16 dtei, U8 *pDstMacAddr, U8 *pSrcMacAddr, U8 SendType, U32 handle, U8 lid);



void ProcMsduDataIndication(work_t *work);

void ProcMsduDataConfirm(work_t *work);

















#endif

