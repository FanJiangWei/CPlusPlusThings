#include <string.h>
#include "types.h"
#include "ProtocolProc.h"
#include "sys.h"
#include "trios.h"
#include "app_698client.h"
#include "aps_layer.h"
#include "app_gw3762.h"
#include "app_dltpro.h"
//#include "app_cco_datafreeze.h"
#include "app_clock_os_ex.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "netnnib.h"
#include "dev_manager.h"
#include "app_read_sta.h"
#include "app_clock_sta_timeover_report.h"
#include "app_event_report_sta.h"
#include "app_event_report_cco.h"
#include "SchTask.h"
#include "app_common.h"

//#include "flash_sta_datafreeze.h"
//#include "app_698report.h"



//↓↓↓↓↓↓↓↓↓↓↓↓↓江苏CCO部分↓↓↓↓↓↓↓↓↓↓↓↓↓
#if defined(STATIC_MASTER)  
ostimer_t *RTCReqTimer = NULL;
U8 SysTemTimeInitflag = FALSE;


#endif
U8 RTCsysSuccessFlag = FALSE;//RTC时钟同步标志

ostimer_t *SetSlaveModuleTimeSyncTimer = NULL;   // 入网从节点校时控制定时器
RTCSYCADDR pRtcSycAddr[MAX_WHITELIST_NUM];
uint8_t CheckClockflag = FALSE;
OAD_s VolOad = { 0x2000 ,0x02 , 0x00 ,  0x00 };     //2000 0200  电压
U8 ReadVoltageItem[4]={0x33,0x32,0x34,0x35};//电压数据块
U8 Read97_AVoltageItem[2] = {0xb6,0x11};
U8 Read97_BVoltageItem[2] = {0xb6,0x12};
U8 Read97_CVoltageItem[2] = {0xb6,0x13};
/*
U8 Read97_VoltageItem[3][2] = { 	
								{0xb6,0x11},
								{0xb6,0x12},
								{0xb6,0x13},
							  };

*/

U32 GetSecByDateTimes(date_time_s SysDateTemp)
{
    
	//app_printf("SysDateNow before [ %d-%d-%d  %d:%d:%d ]\n",
	//			SysDateNow.Year,SysDateNow.Mon,SysDateNow.Day,SysDateNow.Hour,SysDateNow.Min,SysDateNow.Sec);
    ulong_t localTime;
	localTime = time_to_secs(SysDateTemp.year,SysDateTemp.month,SysDateTemp.day,SysDateTemp.hour,SysDateTemp.minute,SysDateTemp.second);
    return localTime;
}

void SetBcdTime(SysDate_t SysDateNow)
{
    bcd_to_bin((U8 *)&SysDateNow, (U8 *)&SysDateNow, sizeof(SysDate_t));
    ulong_t localTime;
    localTime =time_to_secs(SysDateNow.Year+2000,SysDateNow.Mon,SysDateNow.Day,SysDateNow.Hour,SysDateNow.Min,SysDateNow.Sec);
    os_set_time(localTime);
}

void DatetimesToSysDate(SysDate_t *SysDateTemp,date_time_s Date)
{
	 SysDateTemp->Year = Date.year - 2000;
     SysDateTemp->Mon = Date.month;
     SysDateTemp->Day = Date.day;
     SysDateTemp->Hour = Date.hour;
     SysDateTemp->Min = Date.minute;
     SysDateTemp->Sec = Date.second;
}

void SysDateToDatetimes(SysDate_t SysDateTemp,date_time_s *Date)
{
	 Date->year  = SysDateTemp.Year + 2000;
     Date->month  = SysDateTemp.Mon;
     Date->day  = SysDateTemp.Day;
     Date->hour = SysDateTemp.Hour;
     Date->minute = SysDateTemp.Min;
     Date->second  = SysDateTemp.Sec;

}

void ostm2OutClock(SysDate_t *_tRtc, sys_time_t  *tmbuf)
{
	_tRtc->Sec  = tmbuf->sec;
	_tRtc->Min  = tmbuf->min;
	_tRtc->Hour = tmbuf->hour;
	_tRtc->Day  = tmbuf->day;
	_tRtc->Mon  = tmbuf->mon;
	_tRtc->Year = tmbuf->year-2000;

	return;
}

sys_time_t *GetCurrOsLocalTime(void)
{
	U32 os_seconds = 0;
	static sys_time_t temp_sys_time = {0};
	os_seconds = os_rtc_time();

	secs_to_time(&temp_sys_time, os_seconds);

	return &temp_sys_time;
}

void OutClock2ostm(sys_time_t  *tmbuf, SysDate_t *_tRtc)
{
	tmbuf->sec  = _tRtc->Sec;//SECOND;
    tmbuf->min  = _tRtc->Min;//MINUTE;
    tmbuf->hour = _tRtc->Hour;//HOUR;
    tmbuf->day = _tRtc->Day;//DATE;
    tmbuf->mon  = _tRtc->Mon;//MONTH - 1;
    tmbuf->year = _tRtc->Year + 2000;
	
	return;
}

void getBinTimebySec(ulong_t LocalTime,SysDate_t* date)
{

	sys_time_t temp_sys_time = {0};
	
	secs_to_time(&temp_sys_time, LocalTime);

	
    date->Year = temp_sys_time.year-2000;
    date->Mon = temp_sys_time.mon;
    date->Day = temp_sys_time.day;
    date->Hour = temp_sys_time.hour;
    date->Min = temp_sys_time.min;
    date->Sec = temp_sys_time.sec;
}

//↓↓↓↓↓↓↓↓↓↓↓↓↓gw sta部分↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
#if defined(STATIC_NODE)  
SysDate_t  SysDate_Now = {0};
U8 	DayOfOneMonthValMAX[13]			=	{0,31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
ostimer_t *EventReportClockOvertimer = NULL;
U8 SerialSeq = 0;
U8 YMDFlag = FALSE;

U8 DLT645FrameFlag = FALSE;

uint32_t GetSecondFromDateTime(SysDate_t *_tRtc)
{//BIN,not BCD
	U8 y = 0;
	U32 sec = 0,fy = 0;
	U16 thisyearday = 0;
	U8 ii;
	y = (U8)(_tRtc->Year - RTC_BASE_YEAR);
	fy = y / 4;
	sec = fy * DAY_OF_FOURYEAR;
	if(y/100)//百年不润
	{	
		 sec -= y/100;
	}
	if(y/400)//四百年再润
	{	
		 sec += y/400;
	}
	y %= 4;
	sec += y * DAY_OF_YEAR;
	for(ii = 1;ii<_tRtc->Mon;ii++)
	{
		thisyearday += DayOfOneMonthValMAX[ii];
		if(ii == 2&&IsRTCLeapYear(_tRtc->Year))
		{
			thisyearday += 1;
		}
	}
	sec += thisyearday; 
	sec += _tRtc->Day - 1;
	sec *= 24;
	sec += _tRtc->Hour;
	sec *= 60;
	sec += _tRtc->Min;
	sec *= 60;
	sec += _tRtc->Sec;
	return sec;
}

void GetDateTimeFromSecond_0(U32 lSec, SysDate_t *_tRtc)
{
	uint16_t y = 0;
    U32 lDay = 0;
    U32 ii = 0;
    U32 lMothDay = 0;
    lDay = lSec / SECOND_OF_DAY;        /* 转为基于天的时间 */
    lSec = lSec % SECOND_OF_DAY;

    _tRtc->Year = RTC_BASE_YEAR_0;//基数年份
		y = lDay / DAY_OF_FOURYEAR;
		y *= 4;
		_tRtc->Year += y;
		lDay %= DAY_OF_FOURYEAR;
		if(y/100){//百年不润
			 lDay += y/100;
		}
		if(y/400){//四百年再润
			 lDay -= y/400;
		}
		y = lDay / DAY_OF_YEAR;
		_tRtc->Year += y;	 /* 得到年份 */
		lDay %= DAY_OF_YEAR;
        
		for(ii = 1;ii < 13;ii++)
        {
            lMothDay = DayOfOneMonthValMAX[ii];
            if(ii == 2&&IsRTCLeapYear(_tRtc->Year))
            {
                lMothDay += 1;
            }
            if(lDay < lMothDay)
            {
                _tRtc->Mon = ii - 1;
                _tRtc->Day = lDay;
                break;
            }
            lDay -= lMothDay; //DayOfOneMonthValMAX[ii];
        }      
		
    _tRtc->Hour = lSec / SECOND_OF_HOUR;
	lSec %= SECOND_OF_HOUR;
    _tRtc->Min  = lSec / SECOND_OF_MINUTE;
    _tRtc->Sec  = lSec % SECOND_OF_MINUTE;
		
}

void clock_os_event_report_proc(work_t *work)
{
	static U8 event_report_clockover_fre = 0;
	app_printf("StaEventReportStatus = %d, event_report_clockover_save_t.len = %d, event_report_clockover_fre = %d\n", 
		g_StaEventReportStatus, event_report_clockover_save_t.len, event_report_clockover_fre);
	if((g_StaEventReportStatus != e_3762EVENT_REPORT_FORBIDEN) && (event_report_clockover_save_t.len >= 12) && (event_report_clockover_fre < 5))
	{
		EVENT_REPORT_REQ_t *pEventReportRequest_t = NULL;
		U8 ccomac[6];
		net_nnib_ioctl(NET_GET_CCOADDR,ccomac);    
		pEventReportRequest_t = zc_malloc_mem(sizeof(EVENT_REPORT_REQ_t) + event_report_clockover_save_t.len,"EventReportRequest",MEM_TIME_OUT);
		
		pEventReportRequest_t->Direction = e_UPLINK;
		pEventReportRequest_t->PrmFlag	 = 1;
		pEventReportRequest_t->FunCode	 = e_METER_ACT_CCO;
		pEventReportRequest_t->PacketSeq = ++ApsEventSendPacketSeq;//重发
		aps_event_seq.EventReportClockOverSeq = pEventReportRequest_t->PacketSeq;
		__memcpy(pEventReportRequest_t->MeterAddr,DevicePib_t.DevMacAddr,6);
	    __memcpy(pEventReportRequest_t->DstMacAddr, ccomac, 6);	
	    __memcpy(pEventReportRequest_t->SrcMacAddr,DevicePib_t.DevMacAddr,6);
		//app_printf("event_report_save_t.len = %d\n",event_report_clockover_save_t.len);
		pEventReportRequest_t->EvenDataLen = event_report_clockover_save_t.len;
	    __memcpy(pEventReportRequest_t->EventData, event_report_clockover_save_t.buf, event_report_clockover_save_t.len);

		ApsEventReportRequest(pEventReportRequest_t);
		
		zc_free_mem(pEventReportRequest_t);
		sta_creat_event_report_clk_overtimer(30*1000);
		event_report_clockover_fre++;
	}
	else
	{
		app_printf("StaEventReportStatus = %d\n", g_StaEventReportStatus);
		app_printf("event_report_clockover_save_t.len = %d\n", event_report_clockover_save_t.len);
		app_printf("event_report_clockover_fre = %d\n", event_report_clockover_fre);
		memset(event_report_clockover_save_t.buf, 0x00, sizeof(event_report_clockover_save_t.buf));
        event_report_clockover_save_t.len = 0;
		event_report_clockover_fre = 0;
		timer_stop(EventReportClockOvertimer, TMR_NULL);
	}
}





void read_clock_recive_645pro(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
	add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;
	U8 ReadTimeRes = 0;
	U8 Addr[6] = {0};
	U8 ControlCode = 0;
	U8 len = 0;
	U8 FECount = 0;
	S32 Temp = 0;
	U8 readtimeYMDW[4]={0x34,0x34,0x33,0x37};//year month day w
	U8 readtimeHMS[4]={0x35,0x34,0x33,0x37};// hour minutes seconds
	
	get_info_from_645_frame(revbuf, revlen, Addr , &ControlCode , &len , &FECount);
    if(0 != memcmp(Addr , DevicePib_t.DevMacAddr , 6))
    {
        app_printf("addr error\r\n");
        //return;
    }
    
    if(0x91 == ControlCode&&0 == memcmp(revbuf + FECount + 10  , readtimeYMDW , sizeof(readtimeYMDW))) // readtimeYMDWlogo
    {
    	SysDate_Now.Year = revbuf[17]-0x33 ;
		SysDate_Now.Mon = revbuf[16]-0x33 ;
		SysDate_Now.Day = revbuf[15]-0x33 ;
		YMDFlag = TRUE; 
        app_printf("readtimeYMDW\r\n");
	}
	else if(0x91 == ControlCode&&0 == memcmp(revbuf + FECount + 10  , readtimeHMS , sizeof(readtimeHMS))) // readtimeYMDWlogo
    {
    	SysDate_Now.Hour   = revbuf[16]-0x33  ;
		SysDate_Now.Min    = revbuf[15]-0x33 ;
		SysDate_Now.Sec    = revbuf[14]-0x33 ;
		ReadTimeRes = 1;
        app_printf("readtimeHMS\r\n");
	}
	else
	{
		return;
	}
	extern void bcd_to_bin(U8 *src, U8 *dest, U8 len);
	if(pSerialMsg->StaReadMeterInfo.LastFlag == TRUE && YMDFlag ==TRUE)
	{
		YMDFlag = FALSE;
		if(FALSE == FuncJudgeBCD((U8*)&SysDate_Now, sizeof(SysDate_t)))
			return;
		bcd_to_bin((U8 *)&SysDate_Now, (U8 *)&SysDate_Now, sizeof(SysDate_t));
		if(FALSE == FunJudgeTime(SysDate_Now))
     		return;
		
		sta_cali_time_diff(&meterDate, &Temp, &meterSeconds);
		if(ReadTimeRes == 1)
		{
			ReadTimeRes = 0;
			g_ReadMeterResFlag = e_READ_METER_TIME_SUCCESS;
		}
		app_printf("645 meterSeconds = %d\n",meterSeconds);
		if(TimeCalFlag)
		{
			//if(Temp < 60*60 && Temp > -60*60)
			{
				DevicePib_t.TimeDeviation = Temp;
				staflag = TRUE;
			}
		}
		
		app_printf("DevicePib_t.TimeDeviation = %d\n",DevicePib_t.TimeDeviation);
	}
	//check_clock_offset_report(revbuf, revlen);
}



void ReadDLT645Time(U8 prtcl)
{
	U8  sendbuf[50];
	U8  len=0;
	U32 Baud = 0;
	U8  CtrlCode = 0x11;
	U8  readtimeYMDWlogo[4]={0x34,0x34,0x33,0x37};//year month day w
	U8  readtimeHMSlogo[4]={0x35,0x34,0x33,0x37};// hour minutes seconds

	Packet645Frame(sendbuf,&len,DevicePib_t.DevMacAddr,CtrlCode,readtimeYMDWlogo,sizeof(readtimeYMDWlogo));
	put_read_meter_to_serial_list(sendbuf,len,FALSE,Baud,FALSE,prtcl);
	len = 0;
	Packet645Frame(sendbuf,&len,DevicePib_t.DevMacAddr,CtrlCode,readtimeHMSlogo,sizeof(readtimeHMSlogo));
	put_read_meter_to_serial_list(sendbuf,len,FALSE,Baud,TRUE,prtcl);
}

void GetInfoForm698Frame(U8 *pdata , U16 datalen ,U8 *addr, U8 *addrLen, U8 *addrType, U8 *controlcode , U16 *len , U8 *FENum, U8 *userData)
{	 
	U8 FECount = 0;
	
	while(pdata[FECount] == 0xFE)
	{
		FECount++;
	}

    __memcpy(len , pdata+FECount+1 , sizeof(U16));
	*controlcode = pdata[FECount + 3];
	*addrLen = pdata[FECount + 4]&0x0f;
	if(NULL != addrType)
	{
		*addrType = (pdata[FECount + 4]>>6)&0x03;
	}
    __memcpy(addr , pdata+FECount+5, *addrLen+1);
    __memcpy(userData, pdata+FECount+5+(*addrLen)+1+2+1, (*len)-2-1-1-sizeof(addrLen)-1-2-2);
	//app_printf("userData Len = %d, userData->", (*len)-2-1-1-sizeof(addrLen)-1-2-2);
	dump_buf(userData, (*len)-2-1-1-sizeof(addrLen)-1-2-2);
	*FENum		= FECount;
	return;
}

void read_clock_recive_698pro(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
	U8 ReadTimeRes = 0;
	U8 Addr[7] = {0};
	U8 Addrlen = 0;
	U8 ControlCode = 0;
	U16 len = 0;
	U8 FECount = 0;
	U8 *userData = zc_malloc_mem(revlen, "userData", MEM_TIME_OUT);
	S32 Temp =0 ;
	U8 readtimelogo[] = {0x05, 0x01, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00};
	
	//app_printf("ReadDLT698Time recievebuf->");
	//dump_buf(revbuf,revlen);
	if(revlen > 17)
		GetInfoForm698Frame(revbuf,revlen, Addr, &Addrlen, NULL, &ControlCode, &len, &FECount, userData);
	//app_printf("FECount=%d,ControlCode=%02x\r\n",FECount,ControlCode);
	if(0 != memcmp(Addr , DevicePib_t.DevMacAddr , 6))
	{
		app_printf("addr error\r\n");
	}
	if(0xC3 == ControlCode&&0 == memcmp(&userData[3]  , &readtimelogo[3] , 4))
	{
		SysDate_Now.Year = (((userData[9]<<8)+ userData[10])%0x64);
		SysDate_Now.Mon = userData[11];//RTC时钟范围0-11
		SysDate_Now.Day = userData[12];
		SysDate_Now.Hour = userData[13];
		SysDate_Now.Min = userData[14];
		SysDate_Now.Sec = userData[15];
		ReadTimeRes = 1;
		app_printf("readtime 698\r\n");
	}
	if(FALSE == FunJudgeTime(SysDate_Now))
	{
		zc_free_mem(userData);
	 	return;
	}
    
	sta_cali_time_diff(&meterDate, &Temp, &meterSeconds);
	if(ReadTimeRes == 1)
	{
		ReadTimeRes = 0;
		g_ReadMeterResFlag = e_READ_METER_TIME_SUCCESS;
	}
	app_printf("698 meterSeconds = %d\n",meterSeconds);
	if(TimeCalFlag)
	{
		//if(Temp < 60*60 && Temp > -60*60)
		{
			DevicePib_t.TimeDeviation = Temp;
			app_printf("DevicePib_t.TimeDeviation = %d\n",DevicePib_t.TimeDeviation);
			staflag = TRUE;
		}
	}
	zc_free_mem(userData);
	return ;
}

void put_read_meter_to_serial_list(U8 *pTimeData, U16 dataLen, U8 CjqFlag, U32 baud,U8 lastFlag,U8 protocol)
{
    add_serialcache_msg_t serialmsg;
	memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));

    app_printf("lastFlag = %d\n", lastFlag);

    serialmsg.StaReadMeterInfo.LastFlag     = lastFlag;

    serialmsg.baud          = baud;
	
	serialmsg.Uartcfg = (baud == 0)?NULL:meter_serial_cfg;
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
    //extern U8 StaReadMeterMatch(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);
    serialmsg.Match  = protocol== e_other?NULL : sta_read_meter_match;
    serialmsg.Timeout = read_meter_time_timeout_handle;
    serialmsg.RevDel = (protocol == DLT645_2007)?read_clock_recive_645pro: read_clock_recive_698pro;

    serialmsg.Protocoltype = protocol;
	serialmsg.DeviceTimeout = 100;
    serialmsg.IntervalTime = 10;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = dataLen;
	serialmsg.FrameSeq = ++SerialSeq;
	serialmsg.lid = e_Serial_Broadcast;  //e_Serial_Trans;修改读表时间和精准校时保持一致，并低于抄表优先级；
	serialmsg.ReTransmitCnt = 0;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;

	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pTimeData,TRUE);

}

void ReadDLT698Time(U8 prtcl)
{

	U8  ctrCode_698 = 0x43;
	U8  sendbuf[50] = {0};
	U16 len=0;
	U32 Baud =0;
	U8  readtimelogo[] = {0x05, 0x01, 0x00, 0x40, 0x00, 0x02, 0x00, 0x00};
	
	
	Packet698Frame(sendbuf, &len, ctrCode_698, 0, 6, DevicePib_t.DevMacAddr, readtimelogo, sizeof(readtimelogo),0);

	put_read_meter_to_serial_list(sendbuf,len,FALSE,Baud,TRUE,prtcl);
	
}
/*
void get_time_from_645(U8 *buf, U16 len)
{
	U8 Time_buf[16] = {0} ;
    U8 ii=0;
                            
    for(ii=0;ii<6;ii++)
    {
            Time_buf[ii] = buf[ii+10] - 0x33;
    }
    dump_buf(Time_buf , 6);
    ii=0;
    SysDate.Sec = Time_buf[ii++] ;
    SysDate.Min = Time_buf[ii++] ;
    SysDate.Hour = Time_buf[ii++] ;
    SysDate.Day = Time_buf[ii++] ;
    SysDate.Mon = Time_buf[ii++] ;
    SysDate.Year = Time_buf[ii++] ;
	
	extern void bcd_to_bin(U8 *src, U8 *dest, U8 len);
    bcd_to_bin((U8 *)&SysDate, (U8 *)&SysDate, sizeof(SysDate_t));
    app_printf("[ %d-%d-%d  %d:%d:%d ]------------TimeCalibrateIndication\n",SysDate.Year,SysDate.Mon,SysDate.Day,SysDate.Hour,SysDate.Min,SysDate.Sec);
}
*/

/*
//读取电能表时钟，并计算电能表时钟和模块时钟的差值
void node_clock_os_pro(U8 *buf, U16 len, U8 Cjqflag, U8 baud)
{
	get_time_from_645(buf, len);
	if(((DLT645_2007 == DevicePib_t.Prtcl) || (DLT698 == DevicePib_t.Prtcl)) && (0 != SysDate.Mon) && (0 != SysDate.Day) && (StaEventReportStatus != e_3762EVENT_REPORT_FORBIDEN))
	{
		memset((U8*)&SysDate_Now,0x00,sizeof(SysDate_t));
		if(0 == SysDate_Now.tm_mon || 0 == SysDate_Now.tm_mday)
		{
			if(DLT645_2007 == DevicePib_t.Prtcl)
				ReadDLT645Time(baud);
			else if(DLT698 == DevicePib_t.Prtcl)
				ReadDLT698Time(baud);
		}
	}
}
*/


#endif

void get_time_from_645(U8 *buf, U16 len, SysDate_t *SysDate)
{
	U8 Time_buf[16] = {0} ;
	U8 ii=0;
	U8 FENum = 0;
	
	for(ii=0; ii<13; ii++)
	{
		if(*(buf + ii) == 0x68)
		{
			FENum = ii;
			break;
		}
	}
	
	for(ii=0;ii<6;ii++)
	{
		Time_buf[ii] = buf[ii+FENum+10] - 0x33;
	}
	dump_buf(Time_buf , 6);
	ii=0;
	SysDate->Sec = Time_buf[ii++] ;
	SysDate->Min = Time_buf[ii++] ;
	SysDate->Hour = Time_buf[ii++] ;
	SysDate->Day = Time_buf[ii++] ;
	SysDate->Mon = Time_buf[ii++] ;
	SysDate->Year = Time_buf[ii++] ;
	
	//bcd_to_bin((U8 *)&SysDate, (U8 *)&SysDate, sizeof(SysDate_t));
	//app_printf("[ %d-%d-%d  %d:%d:%d ]------------TimeCalibrateIndication\n",SysDate.Year,SysDate.Mon,SysDate.Day,SysDate.Hour,SysDate.Min,SysDate.Sec);
}
/****************************************************************************
* 函数名: IsLeapYear
* 功  能: 判断时期时间是否有效
* 输  入: Year ：年.
* 输  出: 无.
* 返  回: TRUE表示闰年.
****************************************************************************/
U8 IsRTCLeapYear (U16 Year)
{
  if (((!(Year%4)) && (Year%100)) || !(Year%400))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}

U8 FunJudgeTime(SysDate_t Date)
{
  app_printf("%d-%d-%d %d:%d:%d\n", Date.Year, Date.Mon, Date.Day, Date.Hour, Date.Min, Date.Sec);
  if (Date.Sec < 0 || Date.Sec >= 60)
	return FALSE;

  if (Date.Min < 0 || Date.Min >= 60)
	return FALSE;

  if (Date.Hour < 0 || Date.Hour >= 24)
	return FALSE;

  /*if(Date.Year < 20)
	return FALSE;*/

  if (Date.Mon < 1 || Date.Mon > 12)
	return FALSE;

  U16 year;

  switch (Date.Mon)
  {
  case 1:
  case 3:
  case 5:
  case 7:
  case 8:
  case 10:
  case 12:
	if (Date.Day < 1 || Date.Day > 31)
			return FALSE;
	break;
  case 4:
  case 6:
  case 9:
  case 11:
	if (Date.Day < 1 || Date.Day > 30)
			return FALSE;
	break;
  case 2:

	year = (Date.Year & 0x00ff) + 2000;
	if (TRUE == IsRTCLeapYear(year))
	{
			if (Date.Day < 1 || Date.Day > 29)
				return FALSE;
	}
	else
	{
			if (Date.Day < 1 || Date.Day > 28)
				return FALSE;
	}
	break;
  default:
	return FALSE;
  }

  return TRUE;
}
//#endif


