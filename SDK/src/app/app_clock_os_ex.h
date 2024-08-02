#ifndef __APP_CLOCK_OS_EX_H
#define __APP_CLOCK_OS_EX_H
#include "aps_layer.h"
#include "app_698p45.h"
#include "app_gw3762.h"
#include "datalinkdebug.h"
#include "app_sysdate.h"
//#include <time.h>

#define  STATUSPERIOD    1440//24*60
#define  DAYCTLPERIOD    1440//24*60
#define  MONCTLPERIOD    43200 //24*60*30
#define  CURVECTLPERIOD  15
#define  MINCTLPERIOD    1

#define  CHECK_CLOCK_PERIC  (15*60)   //S
#define  CAL_TIME_PERIC     (12*60*60)

#define  TIME_DIFFER_INIT     (30*60+1)	//30分钟+1秒
#define  MINUTE_GATHER_OK 1
#define  MINUTE_GATHER_NO 0


/******************************* 日期时间 *******************************/
extern SysDate_t SysDate;
extern U8 YMDFlag;

extern ostimer_t *RTCReqTimer ;
//extern U8  MType;
typedef struct{
    U16   ProtocolVer                :6;
    U16   HeaderLen                  :6;
    U16   Reserve1                   :4;

	U8    RTCClock[6]                ;
    U32   NTBtime                    ;

    U8       Asdu[0];
}__PACKED RTC_TIMESYNC_HEADER_t;


typedef struct{
    U8       RTCClock[6]                ;
	U16      AsduLength;  
    U32      NTBtime;
 
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
    U32      SendType;

	U8       Asdu[0];
}__PACKED RTC_TIMESYNC_REQUEST_t;

typedef struct{
    U16       Status;
    U16      AsduLength; 
    U8       Asdu[0];
}__PACKED RTC_TIMESYNC_CONFIRM_t;

typedef struct{
    U8       RTCClock[6]                ;
    U16      AsduLength;
    U32      NTBtime                    ;
	U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
 	U32      SendType;
 	
    U8       Asdu[0];
}__PACKED RTC_TIMESYNC_INDICATION_t;
typedef struct
{
	U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];

}__PACKED RTC_TIMESYNC_RSP_t;
typedef struct
{
	U8      RtcMacaddr[MACADRDR_LENGTH];
	U8 		Res[2];
}__PACKED RTCSYCADDR;

void get_time_from_645(U8 *buf, U16 len, SysDate_t *SysDate);
U8 FunJudgeTime(SysDate_t Date);
U8 IsRTCLeapYear (U16 Year);
void DatetimesToSysDate(SysDate_t *SysDateTemp,date_time_s Date);

#if defined(STATIC_MASTER)

extern U8 SysTemTimeInitflag;

extern RTCSYCADDR pRtcSycAddr[MAX_WHITELIST_NUM];
extern void cco_set_sync_rtc(SysDate_t SysDateTemp);
extern void cco_quick_sync_rtc();
extern void RtcClockinit();
extern ostimer_t *SetSlaveModuleTimeSyncTimer;
#endif
extern void get_other_date_time_s(date_time_s SysDateTemp,S32 DiffSec,date_time_s *SysDateOther);
extern void get_date_time_s(date_time_s *SysDateTemp);
extern void ostm2OutClock(SysDate_t *_tRtc, sys_time_t  *tmbuf);
extern sys_time_t *GetCurrOsLocalTime(void);
void SysTemTimeInit();
void SysTemTimeSet(SysDate_t *pSysDate);
U32 GetBinTimeSec(SysDate_t SysDateNow);
extern void SetBcdTime(SysDate_t SysDateNow);
extern void GetBinTime(SysDate_t *SysDateTemp);
extern void   GetInfoForm698Frame(U8 *pdata , U16 datalen ,U8 *addr, U8 *addrLen, U8 *addrType, U8 *controlcode , U16 *len , U8 *FENum, U8 *userData);
extern void SysDateToDatetimes(SysDate_t SysDateTemp,date_time_s *Date);
void read_clock_recive_645pro(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);
void read_clock_recive_698pro(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);

void DltMeterType645Encode(U8* MeterAddr, U8 Protocol,U8 *Logo,U8 LogoLen,U8 *ReadMeterType,U8 *ReadMeterTypeLen);

void DltMeterType698Encode(U8* MeterAddr, U8 Protocol,U8 *ReadMeterType,U16 *ReadMeterTypeLen);

//以下代码有待商议
void ApsModuleTimesynsRequest(RTC_TIMESYNC_REQUEST_t *pRTCTimeSyncRequestReq);
void ApsRTCTimeSynResponse(RTC_TIMESYNC_RSP_t *pRtcTimeSynResp);


void SetDateTimeReq(struct work_s * work);

#if defined(STATIC_NODE)
#define DAY_OF_FOURYEAR     (365*3+366)
#define DAY_OF_YEAR         365
#define RTC_BASE_YEAR       00
#define RTC_BASE_YEAR_0       00

#define SECOND_OF_DAY       (24*60*60)
#define SECOND_OF_HOUR      (60*60)
#define SECOND_OF_MINUTE    60
/*
enum{
	e_meter_faster = 1,	//电能表超出集中器时间
	e_Jzq_faster,	//电能表低于集中器时间
};

*/

extern SysDate_t  SysDate_Now;
extern ostimer_t *EventReportClockOvertimer;

extern void ReadDLT698Time(U8 Baud);
extern void clock_os_event_report_proc(work_t *work);

void put_read_meter_to_serial_list(U8 *pTimeData, U16 dataLen, U8 CjqFlag, U32 baud,U8 lastFlag,U8 protocol);
void node_clock_os_pro(U8 *buf, U16 len, U8 Cjqflag, U8 baud);
void clock_os_event_report_ind(U16 Seq);
extern void ReadDLT645Time(U8 Baud);
void PacketDLT645Frame(U8 *pData,U8 *len,U8 *addr,U8 *logo ,U8 logolen);

void GetInfoForm698Frame(U8 *pdata , U16 datalen ,U8 *addr, U8 *addrLen, U8 *addrType, U8 *controlcode , U16 *len , U8 *FENum, U8 *userData);

//#endif

#endif

#endif 

