#ifndef __APP_DLTPRO_H__
#define __APP_DLTPRO_H__

#include "gpio.h"
#include "ZCsystem.h"
#include "cjqTask.h"
#include "app_upgrade_sta.h"
#include "app_upgrade_cco.h"
#include "app_meter_common.h"
#include "app_meter_bind_sta.h"

#define DL645EX_CTRCODE       	   0x1F
#define DL645EX_UP_CTRCODE   	   0x9F
#define PREAMBLE_CHAR_COUNT   	   0x04
#define FRAME_698_ADD_POS     5
#define DL645_CALITIME_CTRL_CODE   0x08
#define DL698_CALI_TIME_CTRL_CODE  0x43
#define DL698_BRD_ADDR_TYPE 	   0xc0   //广播

#define U8TOBCD(data) ((((data/ 10)<<4) & 0xF0 ) | ((data % 10) & 0x0F))
#define BCDTOU8(data) (((data>>4) *10 ) + (data & 0x0F))

#define DLT645_UN_KNOWN	     0	//未知645规约
#define DLT645_1997			1	//dlt6451997规约
#define DLT645_2007			2	//dlt6452007规约
#define DLT698           	3	//698

#define GW3762           	6	//GW376.2
#define GD2016           	7	//GW376.2


#define GW13_PORT			0x13	//国网13规约接口
#define GW09_PORT			0x09	//国网09规约接口

#define IO_CHECK_TIME       3      //30ms

#define  BAUDMETER_TIMEOUT  120    //1200ms

extern U16 RSPTIME;//1S



typedef enum
{
    e_READ_RELAY_ZONE			    =0X03,
    e_SET_RELAY_ZONE			    =0X04,
    e_READ_OUTER_VERSION 		    =0X07,
    e_READ_INNER_VERSION 			=0X16,
    e_SELF_CHECK_TEST 			    =0X22,
    e_DYNAMIAC_CHECK_TEST 		    =0X23,
    e_MEMORY_CHECK_TEST 		    =0X24,
    e_MEMORY_READ_TEST 			    =0X25,

    e_SET_BAND 					    =0X2D,
    e_SET_PHY_CB_MODE			    =0X2E,
    e_PLC_SEND_FRAME			    =0X2F,
    
	e_SET_OUTER_VENDORCODE_VERSION	=0X3A,
	
	e_REBOOT_DEVICE				    =0X44,
	
    e_WRITE_CHIP_ID				    =0X5B,
    e_WRITE_MODULE_ID			    =0X5C,
    e_READ_MODULE_ID			    =0X5D,
    e_READ_CHIP_ID				    =0X5E,
    e_WRITE_FREQUENCY_OFFSET	    =0X5F,
    e_WRITE_HARDWARE_DEF            =0X60,
    e_READ_HARDWARE_DEF             =0X61,

    e_TEST_MODE					    =0X72,

    e_RDCTRL_START_SEND_BEACON		=0X76,
    e_RDCTRL_QUERY_NB_NUM			=0X77,
    e_RDCTRL_CONNECT_CCO			=0X78,
    e_RDCTRL_CHANGE_BAND			=0X79,

    e_SET_DEVICE_TIMEOUT            =0X7A,
    e_READ_DEVICE_TIMEOUT           =0X7B,
    e_SET_SEND_POWER_PARA           =0X7C,
    e_READ_SEND_POWER_PARA          =0X7D,
    
    AREANOTIFY_BUFF					=0X80,
    LOCAL_UPGARDE					=0x81,
    BAUD_OPT						=0x82,
    NEIGHBROR_LIST					=0x83,
    e_RF_POWER_SET					=0x84,
    e_RF_SEND_FRAME					=0x85,
     
}Extend645_Control_Code;
typedef enum
{
     e_Code_ERR 				=0XCA,
	 e_DATA_ERR 				=0XCB,
	 e_MODE_ERR 				=0XCC,
}Extend645_Data_Err;


typedef enum
{
    BAUD_NULL ,
    BAUD_2400,
    BAUD_4800,
    BAUD_9600,
    BAUD_38400,
    BAUD_57600,
    BAUD_115200,
    BAUD_460800,
    BAUD_921600,
    BAUD_19200,
    BAUD_230400,
    
}UART1_BAUD_e;


    

typedef enum
{
	e_PHY_TPT,
	e_PHY_CB,
	e_MAC_TPT,
	e_WPHY_CB,
	e_RF_PLCPHYCB,
	e_PLC_TO_RF_CB,
}SET_STA_MODE_e;

enum
{
    e_IO_PROCESS = 0x00,
    e_IO_LOW     = 0x02,
    e_IO_HIGH    = 0x04,
};

typedef struct{
 uint8_t start1symbol;
 uint8_t addr[6];
 uint8_t start2symbol;
 uint8_t ctrlfiled;
 uint8_t datalen;
 uint8_t *data;
}DLT645_GENERAL_STRUCTURE_t;


extern    U8 FactoryTestFlag ;





#if defined(ZC3750CJQ2)
extern gpio_dev_t en485;
#define LED_RUN_ON()     gpio_pins_on(&JTAG, LED_RUN)
#define LED_RUN_OFF()    gpio_pins_off(&JTAG, LED_RUN)
#define LED_485_ON()     gpio_pins_on(&JTAG, EN485_CTRL)
#define LED_485_OFF()    gpio_pins_off(&JTAG, EN485_CTRL)
#define en485_ON()       gpio_pins_on(&en485, EN485_CTRL)
#define en485_OFF()  	 gpio_pins_off(&en485, EN485_CTRL)

#endif


enum
{
    SingleAddr_e,
    WildcardAddr_e,
    GroupAddr_e,
    BroadcastAddr_e,
}ADDRTYPE_e;
    
typedef struct
{
    U8 ResetTime;
    U8 BandMeterCnt;
    U8 Prtcl;
    U8 DevType;
    
    U8 DevMacAddr[6];
	U8 PowerOffFlag;
	U8 PowerEventReported;
	
	S32 TimeDeviation;//时间差
	U32 TimeReportDay;//超差上报的日（年月日时分秒的日）
	
	U8 AutoCalibrateSw;//自动校时开关
	U8 Over_Value;	   //超差阈值
	U8 HNOnLineFlg;		//湖南入网认证
    U8 MeterPrtcltype;//河南电表类型是否双协议

	FLASH_FREQ_INFO_t FreqInfo;           
	Rf_CHANNEL_INFO_t RfChannelInfo;	

    U8 ltu_addr[6];
    U8 Modularmode;
    U8 ltu_protocol;


    U8 Cs;
} __PACKED DEVICE_PIB_t;

extern DEVICE_PIB_t DevicePib_t;



typedef struct
{
    U8 ControlData ;
    U8 DataArea[4] ;
    void (* Func) (U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen);
    char *name;
}DLT_07_PROTOCAL;

typedef struct
{
    U8 CtrlCode 	: 5;					/* 功能码 */
    U8 OtherFlag	: 1;					/* 后续帧标志 */
    U8 SlaveAck	    : 1;					/* 从站应答标志 */
    U8 TransDir		: 1;					/* 传松方向 */
} __PACKED Dlt645CtrlField_t;


typedef struct{
    U16    OI;
    U8     RESULT;
    
}__PACKED DL698_event_type_s;

typedef struct{
    U16    OI;
    U8     MethodId;    // 方法标识
    U8     OperateMode; // 操作模式，默认为0
    U8     RESULT;
    
}__PACKED DL698_action_type_s;


extern ostimer_t *sta_bind_timer;
extern ostimer_t *IOChecktimer;

void bcd_to_bin(U8 *src, U8 *dest, U8 len);
void bin_to_bcd(U8 *src, U8 *dest, U8 len);
U8 FuncJudgeBCD(U8 *buf, U8 len);
U8 check_sum(U8 *buf, U16 len);
U16 ResetCRC16(void);
U16 CalCRC16(U16 CRC16Value,U8 *data ,int StartPos, int len);
void InitCRC32Table(void);
unsigned int CalCRC32(unsigned int crc,unsigned char *data, unsigned int len);
unsigned char Check645Frame(unsigned char *buff, U16 len,U8 *FEnum,U8 *addr, U8 *ctrl);
U8 Check645TypeByCtrlCode(U8 CtrlCode);
unsigned char Check698Frame(unsigned char *buff, U16 len,U8 *FEnum,U8 *addr, U8 *logicaddr);
unsigned char CheckT188Frame(unsigned char *buff, U16 len,U8 *addr);
U8 ScaleDLT(U8 Protocol,U8 *buf , U16 *offset,U16 *remainlen , U16 *len);
U8  Check645FrameNum(U8 *pdata, U16 len);

void Add0x33ForData(U8 *buf,U16 buflen);
void Sub0x33ForData(U8 *buf,U16 buflen);
void Packet645Frame(U8 *pData,U8 *len,U8 *addr,U8 ctrl,U8 *logo ,U8 logolen);
void Packet698Frame(U8 *pData,U16 *pDatalen,U8 ctrl,U8 addrtype,U8 addrlen,U8 *addr,U8 *logo ,U16 logolen, U8 CusAddr);
void Verify698Frame(U8 *pData,	U16 FrameLen,U8 feCnt);
U8 GetValueBy645Frame(U8 *id_log,U8 *buf ,U16 Len,U8 *ValueBuf,U8 *ValueBufLen);
//extern int8_t TestStateCheckFunc(U8 *pBuff, U16 BuffLen);
void Extend645PutList(U8 protocol, U16 timeout, U8 frameSeq, U16 frameLen, U8 *pPayload);

void ReadMeterInfo();
U8 WriteMeterInfo(void);

void TestFuncProc(U8 *data , U16 datalen);
void test_tx();

extern U8 ShakeFilter(U8 state , U8 totalnum,S16 *cnt);
extern U8 TotalCounter(U8 state , U16 totalnum,U16 DiffValue,U16 *HighNum,U16 *LowNum);
U8 CheckDefInfo(U8 * pdata ,U16 pdatalen);


#if defined(ZC3750CJQ2)

U8 WriteCJQ2Info(void);
void ReadSoftVersion(U8 *pMsgData);
void ReadSearchTableInterval(U8 *pMsgData);
void SetSearchTableInterval(U8 *pMsgData);
void ParameterInit(U8 *pMsgData);
void DataInit(U8 *pMsgData);
void ParameterAndDataInit(U8 *pMsgData);
void ReadMeterNum(U8 *pMsgData);
void ReadMeterAddr(U8 *pMsgData);
void ReadCollectorAddr(U8 *pMsgData);
void SetCollectorAddr(U8 *pMsgData);
void Read485Overtime(U8 *pMsgData);
void Set485Overtime(U8 *pMsgData);
void ReadCollectorRebootCount(U8 *pMsgData);
void Start_StopSearchMeter(U8 *pMsgData);

void ProcPrivateProForRed(U8 *pMsgData , U8 Msglength);
void ReadMeterForInfrared(U8 Protocol , U8 *FreamAddr , U8* Freamdata , U16 Freamlength);
void TestStaReadMeterTimeout (void *pMsgNode);
void TestStaReadMeterRecvDeal (void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq);

#endif


#if defined(ZC3750STA)


#define METER_COMU_JUDGE_PERIC  (15*60)   //S


void IOCheckCB(struct ostimer_s *ostimer, void *arg);
void Modify_MeterComuJudgetimer(U32 Sec);
void Check20MterComuRecover(void);


extern U8 PlugValue;
extern U8 ReSetValue;
extern U8 SETValue;
extern U8 EVEValue;

//extern U8	MeterPrtcltype;
#endif
enum
{
    SinglePrtcl_e,		//单协议表
    DoublePrtcl_e,		//双协议表-默认以698协议为主，首次抄表后变更为698或645，为了在首次抄表时进行判断是否有事件要上报
  	DoublePrtcl_698_e,	//双协议表-以698协议为主
  	DoublePrtcl_645_e,	//双协议表-以645协议为主
}PRTCLTYPE_e;			//电表协议类型，0：单协议表，1：双协议表
extern ostimer_t *calibration_report_timer;
void modify_calibration_report_timer();

void ProDLT645Extend(U8 *pMsgData , U16 Msglength);
void Extend645PutList(U8 protocol, U16 timeout, U8 frameSeq, U16 frameLen, U8 *pPayload);
U8 SetMacAddr(U8 *Addr);
U8 GetMacAddr(U8 *Addr);
U8 Check_CB_FacTestMode_Proc(uint8_t      *pData ,U16 DataLen);

void get_NNIB_info(U8 *data , U8 *byLen);
void process_698_extend(U8 *pMsgData , U16 Msglength);

typedef struct
{
    U32 extend_code;
    void (*Func)(U8 *data, U16 data_len, U8 *UpData, U16 *UpDataLen);
} __PACKED Extend698_t;

typedef enum
{
    local_upgrade = 0XA1,
    uart_forward = 0XA2,
    //保留
} ExtendCode;
typedef enum
{
    upgrade_success = 0X00,
    upgrade_fail = 0X02,
} UpgradeResult;
typedef enum
{
    no_data = 0X00,
    with_data = 0X01,
} OperationReturnResult;
extern U8 g_printf_state;
extern U8 Check_zc_698_extend(U8 *pDatabuf, U16 dataLen);
#endif

