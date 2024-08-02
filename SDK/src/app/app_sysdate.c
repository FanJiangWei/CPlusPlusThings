#include "ZCsystem.h"
#include "app_698p45.h"
#include "datalinkdebug.h"
#include "app_sysdate.h"
#include "app_dltpro.h"
#include "app.h"
#include "app_clock_os_ex.h"
//#include "SchTask.h"
#include "app_deviceinfo_report.h"


void SetDateTimeReq(struct work_s * work)
{

    ulong_t secs = 0;
	sys_time_t *tmBuf = (sys_time_t*)work->buf;
	app_printf("SysTime:%d-%d-%d %d:%d:%d",tmBuf->year,tmBuf->mon, tmBuf->day, tmBuf->hour,tmBuf->min, tmBuf->sec);
	secs = time_to_secs(tmBuf->year, tmBuf->mon, tmBuf->day, tmBuf->hour,tmBuf->min, tmBuf->sec);
    os_set_time(secs);
	
	#if defined(STATIC_MASTER)
    if(SysTemTimeInitflag == TRUE && PROVINCE_JIANGSU == app_ext_info.province_code)//TODO:江苏独立
	{
       //extern   void cco_alarm_clock_task(U16 CltPeriod);
       cco_alarm_clock_task(SENDTIME06F4);
	}
	#endif
}
void tmTodate_times(date_time_s *SysDateTemp,sys_time_t  tmbuf)
{
    SysDateTemp->year   = tmbuf.year + 1900;
    SysDateTemp->month  = tmbuf.mon;
    SysDateTemp->day    = tmbuf.day;
    SysDateTemp->hour   = tmbuf.hour;
    SysDateTemp->minute = tmbuf.min;
    SysDateTemp->second = tmbuf.sec;
    return;
}
void tmToSysDate(SysDate_t *_tRtc, sys_time_t tmbuf)
{
	_tRtc->Sec  = tmbuf.sec;
	_tRtc->Min  = tmbuf.min;
	_tRtc->Hour = tmbuf.hour;
	_tRtc->Day  = tmbuf.day;
	_tRtc->Mon  = tmbuf.mon;
	_tRtc->Year = tmbuf.year;

	return;
}

void SysTemTimeInit()
{

    work_t *work=zc_malloc_mem(sizeof(work_t) + sizeof(sys_time_t), "SETTIME", MEM_TIME_OUT);
    sys_time_t  *tmbuf = (sys_time_t *)work->buf;
	
    tmbuf->sec = 0;
	tmbuf->min = 0;
	tmbuf->hour = 0;
	tmbuf->day = 1;
	tmbuf->mon = 1;
	tmbuf->year = 2023;

	
    work->work = SetDateTimeReq;
    work->msgtype = SENDTIME_REQ;

    post_app_work(work);
}
void SysTemTimeSet(SysDate_t *pSysDate)
{
	#if defined(STATIC_MASTER)
		extern U8 SysTemTimeInitflag;
		SysTemTimeInitflag = TRUE;
	#endif

    bcd_to_bin((U8 *)pSysDate, (U8 *)pSysDate, sizeof(SysDate_t));
	work_t *work=zc_malloc_mem(sizeof(work_t) + sizeof(sys_time_t), "SETTIME", MEM_TIME_OUT);
	sys_time_t  *tmbuf=(sys_time_t *)work->buf;
	tmbuf->sec = pSysDate->Sec;    //SECOND;
	tmbuf->min = pSysDate->Min;    //MINUTE;
	tmbuf->hour= pSysDate->Hour;   //HOUR;
	tmbuf->day = pSysDate->Day;    //DATE;
	tmbuf->mon  = pSysDate->Mon; //MONTH - 1;
	tmbuf->year = pSysDate->Year+2000;// + 2000 - 1900;

	
    work->work = SetDateTimeReq;
    work->msgtype = SENDTIME_REQ;

    post_app_work(work);
}


U32 GetBinTimeSec(SysDate_t SysDateNow)
{
    
	app_printf("SysDateNow before [ %d-%d-%d  %d:%d:%d ]\n",
				SysDateNow.Year,SysDateNow.Mon,SysDateNow.Day,SysDateNow.Hour,SysDateNow.Min,SysDateNow.Sec);
   
	sys_time_t pTm = {0};
				pTm.sec = SysDateNow.Sec;
				pTm.min = SysDateNow.Min;
				pTm.hour= SysDateNow.Hour;
				pTm.day = SysDateNow.Day;
				pTm.mon	= SysDateNow.Mon;
				pTm.year = SysDateNow.Year+2000;// + 2000 - 1900;
	ulong_t localTime;
	localTime = time_to_secs(pTm.year,pTm.mon,pTm.day,pTm.hour,pTm.min,pTm.sec);

	
    return localTime;
}

void GetBinTime(SysDate_t *SysDateTemp)
{
	U32 os_seconds = 0;
	sys_time_t temp_sys_time = {0};
	os_seconds = os_rtc_time();

	secs_to_time(&temp_sys_time, os_seconds);
	
	SysDateTemp->Year = temp_sys_time.year-2000;    //pTm->tm_year - 100;
    SysDateTemp->Mon = temp_sys_time.mon;   //pTm->tm_mon + 1;
    SysDateTemp->Day = temp_sys_time.day;   //pTm->tm_mday;
    SysDateTemp->Hour = temp_sys_time.hour;   //pTm->tm_hour;
    SysDateTemp->Min = temp_sys_time.min;   //pTm->tm_min;
    SysDateTemp->Sec = temp_sys_time.sec;   //pTm->tm_sec;

}
void SetBinTime(date_time_s SysDateNow)
{
	sys_time_t pTm = {0};
				pTm.sec = SysDateNow.second;
				pTm.min = SysDateNow.minute;
				pTm.hour= SysDateNow.hour;
				pTm.day = SysDateNow.day;
				pTm.mon	= SysDateNow.month;
				pTm.year = SysDateNow.year;// + 2000 - 1900;
	ulong_t localTime;
	localTime = time_to_secs(pTm.year,pTm.mon,pTm.day,pTm.hour,pTm.min,pTm.sec);
	app_printf("Sysdata: [ %d-%d-%d  %d:%d:%d ]\n",
				pTm.year,pTm.mon,pTm.day,pTm.hour,pTm.min,pTm.sec);
    os_set_time(localTime);
}

void get_date_time_s(date_time_s *SysDateTemp)
{

    sys_time_t pTm = {0};
    U32 LocalTime = os_rtc_time();	
	secs_to_time(&pTm, LocalTime);
	
    SysDateTemp->year = pTm.year;
    SysDateTemp->month = pTm.mon;
    SysDateTemp->day = pTm.day;
    SysDateTemp->hour = pTm.hour;
    SysDateTemp->minute = pTm.min;
    SysDateTemp->second = pTm.sec;

}

void get_other_date_time_s(date_time_s SysDateTemp,S32 DiffSec,date_time_s *SysDateOther)
{
    sys_time_t pTm = {0};
    pTm.sec =  SysDateTemp.second;
    pTm.min = SysDateTemp.minute;
    pTm.hour = SysDateTemp.hour;
    pTm.day = SysDateTemp.day;
    pTm.mon  = SysDateTemp.month;
    pTm.year = SysDateTemp.year+2000;     
	ulong_t localTime = 0;
	
	localTime = time_to_secs(pTm.year,pTm.mon,pTm.day,pTm.hour,pTm.min,pTm.sec);

    localTime += DiffSec;

    secs_to_time(&pTm, localTime);

    SysDateOther->year = pTm.year;
    SysDateOther->month = pTm.mon;
    SysDateOther->day = pTm.day;
    SysDateOther->hour = pTm.hour;
    SysDateOther->minute = pTm.min;
    SysDateOther->second = pTm.sec;
}

void SysDateTotm(SysDate_t SysDateTemp,sys_time_t *Date)
{

     Date->year  = SysDateTemp.Year+2000;// +2000 - 1900;  //645传上来的为8位数据格式
     Date->mon  = SysDateTemp.Mon;
     Date->day  = SysDateTemp.Day;
     Date->hour = SysDateTemp.Hour;
     Date->min  = SysDateTemp.Min;
     Date->sec  = SysDateTemp.Sec;

     
	 app_printf("SysDateTemp.Sec =%d\n",SysDateTemp.Sec);
     app_printf("Date->tm_sec =%d\n",Date->sec);
}


#if defined(ZC3750STA)
ostimer_t *SecClockTimer = NULL;

void ModifySecClockTimer(uint32_t Sec)
{
	if(NULL == SecClockTimer)
    {
        
        SecClockTimer = timer_create(Sec*100,
                            0,
                            TMR_TRIGGER ,
                            SecClockTimerCB,
                            NULL,
                            "SecClockTimerCB"
                           );
    }   
    else
	{
		timer_modify(SecClockTimer,
				Sec*100,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				SecClockTimerCB,
				NULL,
				"SecClockTimerCB",
				TRUE
				);
	}
	
	timer_start(SecClockTimer);
}


void SecClockTimerCB(struct ostimer_s *ostimer, void *arg)
{
	date_time_s SysDateTemp = {0};
    get_date_time_s(&SysDateTemp);
    
    
    //app_printf("%d\n",SysDateTemp.Sec);
    
    if(SysDateTemp.second == 0)
    {
 
   	}
    ModifySecClockTimer(1);
}

#endif



/*
void GetOtherBinTime(SysDate_t *SysDateTemp, time_t LocalTime)
{
    struct tm *pTm;
    pTm = localtime(&LocalTime);		//秒换日期
    SysDateTemp->Year = pTm->tm_year + 1900-2000;
    SysDateTemp->Mon = pTm->tm_mon + 1;
    SysDateTemp->Day = pTm->tm_mday;
    SysDateTemp->Hour = pTm->tm_hour;
    SysDateTemp->Min = pTm->tm_min;
    SysDateTemp->Sec = pTm->tm_sec;
	app_printf("SysDateNow before [ %d-%d-%d  %d:%d:%d ]\n",
				SysDateTemp->Year,SysDateTemp->Mon,SysDateTemp->Day,SysDateTemp->Hour,SysDateTemp->Min,SysDateTemp->Sec);
}
struct tm *GetCurrOsLocalTime(void)
{
	time_t LocalTime = os_time();
	
    return localtime(&LocalTime);
}

void OutClock2ostm(struct tm  *tmbuf, SysDate_t *_tRtc)
{
	tmbuf->tm_sec  = _tRtc->Sec;//SECOND;
    tmbuf->tm_min  = _tRtc->Min;//MINUTE;
    tmbuf->tm_hour = _tRtc->Hour;//HOUR;
    tmbuf->tm_mday = _tRtc->Day;//DATE;
    tmbuf->tm_mon  = _tRtc->Mon-1;//MONTH - 1;
    tmbuf->tm_year = _tRtc->Year  + 2000 - 1900;
	
	return;
}
void ostm2OutClock(SysDate_t *_tRtc, struct tm  *tmbuf)
{
	_tRtc->Sec  = tmbuf->tm_sec;
	_tRtc->Min  = tmbuf->tm_min;
	_tRtc->Hour = tmbuf->tm_hour;
	_tRtc->Day  = tmbuf->tm_mday;
	_tRtc->Mon  = tmbuf->tm_mon+1;
	_tRtc->Year = tmbuf->tm_year + 1900 - 2000;

	return;
}

U32 GetSecondByDate(SysDate_t SysDateTemp)
{
	struct tm pTm;
            pTm.tm_sec = SysDateTemp.Sec;
            pTm.tm_min = SysDateTemp.Min;
            pTm.tm_hour= SysDateTemp.Hour;
            pTm.tm_mday = SysDateTemp.Day;
            pTm.tm_mon  = SysDateTemp.Mon - 1;
            pTm.tm_year = SysDateTemp.Year  + 2000 - 1900;
    
    return mktime(&pTm);
}

void GetDateBySecond(SysDate_t *SysDateTemp, U32 second)
{
	SysDateTemp->Year = second/60/60/24/30/12;
	SysDateTemp->Mon = (second%(60*60*24*30*12))/(60*60*24*30);
	SysDateTemp->Day = (second%(60*60*24*30))/(60*60*24);
	SysDateTemp->Hour = (second%(60*60*24))/(60*60);
	SysDateTemp->Min = (second%(60*60))/(60);
	SysDateTemp->Sec = second%60;
}

void get_other_date_time_s(date_time_s SysDateTemp,S32 DiffSec,date_time_s *SysDateOther)
{
    struct tm pTm;
            pTm.tm_sec = SysDateTemp.second;
            pTm.tm_min = SysDateTemp.minute;
            pTm.tm_hour= SysDateTemp.hour;
            pTm.tm_mday = SysDateTemp.day;
            pTm.tm_mon  = SysDateTemp.month;
            pTm.tm_year = SysDateTemp.year;
    time_t localTime;
    localTime = mktime(&pTm);

    localTime += DiffSec;
    __memcpy((U8 *)&pTm ,(U8 *)localtime(&localTime),sizeof(struct tm));

    SysDateOther->year = pTm.tm_year;
    SysDateOther->month = pTm.tm_mon;
    SysDateOther->day = pTm.tm_mday;
    SysDateOther->hour = pTm.tm_hour;
    SysDateOther->minute = pTm.tm_min;
    SysDateOther->second = pTm.tm_sec;
}

void datetimeToSysdate(SysDate_t *sTime, date_time_s dTime)
{
	sTime->Sec = dTime.second;
    sTime->Min = dTime.minute;
    sTime->Hour = dTime.hour;
    sTime->Day = dTime.day;
    sTime->Mon = dTime.month;
    sTime->Year = dTime.year%100;
}

*/

