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




// 抄表数据发送请求数据结构，上下行抄表数据发送都使用，各种抄表模式都使用
typedef struct
{
    U8       ReadMode;         // 抄表模式，区分集中器主动抄表、路由主动抄表和集中器并发抄表
    U8       Direction;        // 下行方向为0，上行方向为1
	U16      dtei;             // 目的TEI值，用于抄控器抄表数据回复

    U8       Timeout;          // 设备超时时间，抄表下行帧时有效
    U8       PacketIner;       // 并发抄表时为报文间隔时间，当前固定为5，单位为10ms
    U16      OptionWord;       // 下行为0，上行时有效，在并发抄表方式时，bit15~bit0为报文应答状态

    U8       ResponseState;    // 应答状态，上行时有效，当前固定为0
    U8       ProtocolType;     // 转发规约类型，0：透明传输；1：645-97；2：645-2007；3：698.45，其它保留。
	U16		 DatagramSeq;      // 报文序号，STA应答时必须与CCO发送下行报文中的序号保持一致
	
    U8       DstMacAddr[MACADRDR_LENGTH];  // 目的MAC地址，白名单中的通讯地址，表地址的反序
    U8       SrcMacAddr[MACADRDR_LENGTH];  // 源MAC地址
	
    U8      SendType;
	U8      CfgWord         : 4;			//bit4:未应答重试标志，bit5:否认重试标志，bit6~bit7:最大重试次数
	U8      res         : 4;
	U16      AsduLength;       // 抄表应用报文长度

	U8       Asdu[0];          // 抄表应用报文
} __PACKED READ_METER_REQ_t;


// 升级时文件数据块发送请求结构
typedef struct
{
    U32      UpgradeID;      // 升级ID
    U32      BlockSeq;       // 数据快编号

    U16      BlockSize;      // 数据块大小
    U8       TransMode;      // 传输模式，单播、全网广播、代理广播等
	U8       IsCjq;          // 以前用来区分表模块与采集器，现在不使用了
	
    U8       DstMacAddr[MACADRDR_LENGTH];   // 目标模块MAC地址
    U8       SrcMacAddr[MACADRDR_LENGTH];   // 源MAC地址

    U8       DataBlock[0];	 // 文件传输数据块
} __PACKED FILE_TRANS_REQ_t;


// 事件上报数据发送请求，下行CCO给STA确认也使用
typedef struct
{
    U16    Direction  :4;   // 0：下行方向，指CCO发送给STA的报文；1：上行方向，指STA上报CCO的报文。
    U16    PrmFlag    :4;   // 0：来自从动站；1：来自启动站。
    U16    FunCode    :4;   // 功能码，下行为1~4，上行1~3，
	U16    sendType   :4;   // 发送类型，广播单播等，如e_FULL_BROADCAST_FREAM_NOACK
    U16    EvenDataLen;     // 事件数据长度
    
	U16    PacketSeq;       // 报文序号
    U8     MeterAddr[6];    // 事件上报电能表地址
    U8     DstMacAddr[MACADRDR_LENGTH];   // 上报目的MAC地址，上行为CCO地址，下行为STA MAC地址
    U8     SrcMacAddr[MACADRDR_LENGTH];

    U8     EventData[0];    // 事件数据
} __PACKED EVENT_REPORT_REQ_t;

// 零火线电流异常设置数据格式
typedef struct
{

    U16    Direction : 4;   // 0：下行方向，指CCO发送给STA的报文；1：上行方向，指STA上报CCO的报文。
    U16    PrmFlag : 4;   // 0：来自从动站；1：来自启动站。
    U16    FunCode : 4;   // 保留
    U16    sendType : 4;   // 发送类型，广播单播等，如e_FULL_BROADCAST_FREAM_NOACK

    U16    PacketSeq;       // 报文序号
    U8		DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];//源MAC地址
    U8 	  CommandType;//命令类型
    U8 	  EvenDataLen;  	//数据长度
    U8 	  EventData[0];    // 数据内容
}__PACKED LiveLineZeroLineEnabled_t;

// 确认否认报文发送请求，确认否认暂时未使用
typedef struct
{
    U8    Direct;           // 0：下行方向，指CCO发送给STA方向；１：上行方向，指STA上报CCO方向的报文；
    U8    AckOrNak;         // 0：否认，1：确认。
    U8    ReportSeq;        // 确认或否认的报文序号
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];
} __PACKED ACK_NAK_REQ_t;


// 查询ID信息请求和响应数据结构，上下行都使用
typedef struct
{
    U8         IdType;      // 请求ID类型，APP_GW3762_CHIP_ID = 1，    APP_GW3762_MODULE_ID = 2；
    U8         direct;      // 方向位，下行为0，上行为1
	U16        destei;      // 目的TEI，一般不用

    U16        AsduLength;   // ID信息内容数据长度，上行有效，下行缺省为0
    U16        PacketSeq;    // 包序号
    
    U8         DstMacAddr[MACADRDR_LENGTH];
    U8         SrcMacAddr[MACADRDR_LENGTH];

    U8         Asdu[0];     // 下行为空，上行为返回ID信息
} __PACKED QUERY_IDINFO_REQ_RESP_t;


// 台区识别报文发送请求，上下行都使用
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


// 抄控器数据发送结构，上下行都使用
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

// 抄控器数据透传串口发送结构，上下行都使用
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

// 启动从节点主动注册发送
typedef struct
{
    U32       PacketSeq;      // 应用层报文序号，从节点主动注册发起一次，具有相同的报文序号
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED SLAVE_REGISTER_START_REQ_t;


// 注册查询从节点请求报文，锁定、水表查询也使用
typedef struct
{
    U8    RegisterPrmt;   // 注册参数，0，查询注册结果；1，启动注册；2，锁定；4，查寻水表
    U8    ForcedResFlag;  // 强制标志，0，非强制，1，强制
    U32    PacketSeq;      // 报文序号，启动一次从节点主动注册使用同一个序号
    U8    Res;

    U16   AsduLength;    // 暂时未使用
    U16   Res1;
    
    U8    SrcMacAddr[MACADRDR_LENGTH];
    U8    DstMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];      // 暂时未使用
} __PACKED REGISTER_QUERY_REQ_t;


// 停止从节点主动注册请求
typedef struct
{
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED SLAVE_REGISTER_STOP_REQ_t;


// 校时数据报文发送请求，广播校时只有下行
typedef struct
{
    U16    DataLength;    // 校时数据长度
    U16    Res;
    
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];

    U8     TimeData[0];   // 校时数据内容
} __PACKED TIME_CALIBRATE_REQ_t;


// 通讯测试报文发送请求，通讯测试报文发送只有下行
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

// 开始升级发送请求
typedef struct
{
    U32    UpgradeID;   // 升级ID
    
    U16    UpgradeTimeout;    // 升级时间窗，即升级最大超时时间
    U16    BlockSize;         // 升级块大小，一般为400字节
    
    U32    FileSize;          // 升级文件总长度
    U32    FileCrc;           // 升级文件CRC32值
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_START_REQ_t;


// 升级状态查询请求
typedef struct
{    
    U32     StartBockIndex;   // 查询开始块序号
    U32     UpgradeID;        // 升级ID

    U16     QueryBlockCount;   // 查询块数，0xFFFF为查询所有块
    U16     Res;
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];
} UPGRADE_STATUS_QUERY_REQ_t;


// 停止升级报文发送请求
typedef struct
{
    U32      UpgradeID;      // 升级ID
    
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_STOP_REQ_t;


// 发送执行升级下行报文请求
typedef struct
{
    U32    UpgradeID;      // 升级ID
    U32    TestRunTime;    // 测试运行时间，单位为秒，0表示不需要试运行时间
    
    U16    WaitResetTime;  // 等待复位时间，单位为秒，注意这个时间在STA端有最大最小限制
    U16    Res;
    
    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_PERFORM_REQ_t;


// 站点信息查询下行报文发送请求
typedef struct
{
    U16     InfoListNum;  // 请求信息列表数
    U16     InfoListLen;  // 信息列表数据长度
    
    U8      DstMacAddr[MACADRDR_LENGTH];
    U8      SrcMacAddr[MACADRDR_LENGTH];

    U8      InfoList[0];
} __PACKED STATION_INFO_QUERY_REQ_t;


// 原自己扩展的台区识别信息采集请求
typedef struct
{
    U8    Direct;
    U8    GatherType;
    U16   AsduLength;
    
    U8    DstMacAddr[MACADRDR_LENGTH];
    U8    SrcMacAddr[MACADRDR_LENGTH];

    U8    Asdu[0];
} __PACKED ZONEAREA_INFO_REQ_t;


// 注册查询响应上行报文发送
typedef struct
{
    U8    Status;       // 响应状态
    U8    MeterCount;     // 表数量
    U8    DeviceType;     // 设备类型
	U8	  RegisterParameter;   // 注册参数，与下行对应，0，查询注册结果；1，启动注册；2，锁定；4，查寻水表

    U16   MeterInfoLen;  // 注册表信息长度
    U16   Res;

    U32   PacketSeq;     // 应用层报文序号
    
    U8    DeviceAddr[6];   // 设备地址，采集器时为采集器地址，模块时为电能表地址，注意地址顺序
    U8    DeviceId[6];    // STA的其它唯一ID，可能为非BCD码
    
    U8    SrcMacAddr[6];
    U8    DstMacAddr[6];
    
    U8    MeterInfo[0];         // 每块表8个字节，长度由表数量决定
} __PACKED REGISTER_QUERY_RSP_t;


// 开始升级上行响应报文发送
typedef struct
{
    U8       StartResultCode;  // 开始升级上行结果码，0为成功，其它值为失败
    U8       Res1;
    U16      Res2;
    
    U32      UpgradeID;     // 升级Id
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED UPGRADE_START_RSP_t;


// 升级状态查询响应上行报文发送
typedef struct
{
    U32    UpgradeID;        // 升级ID
    U32    StartBlockIndex;   // 查询开始块序号
    
    U16    ValidBlockCount;   // 在位图中有效的块数
    U8     UpgradeStatus;     // 0-空闲状、1-接收进行态、2-接收完成态、3-升级进行态、4-试运行态
    U8     BitMapLen;     // 位图数据长度

    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];
    
    U8     BlockInfoBitMap[0];   // 传输位图数据
} __PACKED UPGRADE_STATUS_QUERY_RSP_t;


// 站点信息查询响应上行报文发送
typedef struct
{
    U32    UpgradeID;   // 升级ID
    
    U16    InfoListNum;   // 响应信息列表数，与请求相对应
    U16    InfoDataLen;   // 信息数据长度

    U8     DstMacAddr[MACADRDR_LENGTH];
    U8     SrcMacAddr[MACADRDR_LENGTH];

    U8     InfoData[0];   // 响应站点信息数据
} __PACKED STATION_INFO_QUERY_RSP_t;






void ApsReadMeterRequest(READ_METER_REQ_t *pReadMeterReq);

void ApsFileTransRequest(FILE_TRANS_REQ_t *pFileTransReq);
void ApsEventReportRequest(EVENT_REPORT_REQ_t      *pEventReportReq);
void ApsAckNakRequest(ACK_NAK_REQ_t *pAckNakReq);
void ApsQueryIdInfoReqResp(QUERY_IDINFO_REQ_RESP_t *pQueryIdInfoReq);
void ApsIndentificationRequset(INDENTIFICATION_REQ_t         *pIndentificationReq);
void ApsRdCtrlRequest(RD_CTRL_REQ_t *pRdCtrlReq);
void ApsRdCtrlToUartRequest(RD_CTRLT_TO_UART_REQ_t *pRdCtrlReq);

//离线指示使用
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
	U16      stei;                             // 记录源TEI地址，用于抄控器抄表
	U16      ApsSeq;                           // 应用层帧头序号，下行与对应上行的序号必须相同

    U8       ReadMode;                         // 集中器主动抄表、主动并发抄表或路由主动抄表
    U8       Timeout;                          // 设备超时时间 由CCO给定，单位：100毫秒。 其中采集器超时时间是电能表时超时时间的两倍。
    U8       CfgWord;                           //配置字 点抄 路由保留，并发：bit4：未应答重试标志；bit5: 否认重试标志；bit6~bit7: 最大重试次数≤3
    U8       OptionWord;                        //选项字 点抄 路由bit0:方向位，其他保留;并发:STA与设备进行连续多报文交互时,报文间间隔单位是10毫秒
	
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];

    U16      AsduLength;	
    U8       ProtocolType;
	U8       Res1;

	U8       Asdu[0];
} __PACKED READ_METER_IND_t;

//typedef struct
//{
//	U16      stei;                             // 记录源TEI地址，用于抄控器抄表
//	U16      ApsSeq;                           // 应用层帧头序号，下行与对应上行的序号必须相同
//
//    U8       ReadMode;                         // 集中器主动抄表、主动并发抄表或路由主动抄表
//    U8       ResponseState;                    // 应答状态
//    U16      OptionWord;                       //选项字 点抄 路由bit0:byte0保留置0；byte1 bit0方向位bit7～bit1保留置0;并发:bit15～bit0：报文应答状态
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
//    U16      res          :12;      //plc到Rf物理层回传模式参数 PHR_MCS：4bit， PSDU_MCS：4bit， PbSIZE：4bit
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

