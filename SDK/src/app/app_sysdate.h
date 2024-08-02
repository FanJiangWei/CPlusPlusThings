#ifndef __APP_SYSDATA_H__
#define __APP_SYSDATA_H__
#include "app_698p45.h"


/******************************* 日期时间 *******************************/
typedef struct
{
    U8 Sec;
    U8 Min;
    U8 Hour;
    U8 Day;
    U8 Mon;
    U8 Year;
} __PACKED SysDate_t;

void SysTemTimeInit();
void SysTemTimeSet(SysDate_t *pSysDate);

void SetBinTime(date_time_s SysDateNow);
U32 GetBinTimeSec(SysDate_t SysDateNow);
void GetBinTime(SysDate_t *SysDateTemp);
void SysDateTotm(SysDate_t SysDateTemp,sys_time_t *Date);
void get_date_time_s(date_time_s *SysDateTemp);
void get_other_date_time_s(date_time_s SysDateTemp,S32 DiffSec,date_time_s *SysDateOther);
void tmTodate_times(date_time_s *SysDateTemp,sys_time_t  tmbuf);
void tmToSysDate(SysDate_t *_tRtc, sys_time_t tmbuf);


#if defined(ZC3750STA)
void ModifySecClockTimer(uint32_t Sec);
void SecClockTimerCB(struct ostimer_s *ostimer, void *arg);

#endif

/*
extern void GetOtherBinTime(SysDate_t *SysDateTemp, time_t LocalTime);
struct tm *GetCurrOsLocalTime(void);

void get_other_date_time_s(date_time_s SysDateTemp,S32 DiffSec,date_time_s *SysDateOther);
void datetimeToSysdate(SysDate_t *sTime, date_time_s dTime);
void OutClock2ostm(struct tm  *tmbuf, SysDate_t *_tRtc);
void ostm2OutClock(SysDate_t *_tRtc, struct tm  *tmbuf);


extern U32 GetSecondByDate(SysDate_t SysDateTemp);
extern void GetDateBySecond(SysDate_t *SysDateTemp, U32 second);

*/

#endif
