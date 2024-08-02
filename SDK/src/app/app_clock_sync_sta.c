#include "flash.h"
#include "app_clock_os_ex.h"
#include "aps_layer.h"
#include "app_gw3762.h"
#include "app_base_multitrans.h"
#include "app_sysdate.h"
#include "Datalinkdebug.h"
#include "app_698client.h"
#include "app_clock_sta_timeover_report.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "app_time_calibrate.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_cnf.h"
#include "app_698client.h"
#include "app_dltpro.h"
#include "SchTask.h"



//������������������������GW������������������������������
#if defined(STATIC_NODE) 

READ_METER_RES_t g_ReadMeterResFlag = e_READ_METER_TIME_NONE;  //��������־

void read_meter_time_timeout_handle(void *pMsgNode)
{
	app_printf("read_clock_645TimeOut\n");

	g_ReadMeterResFlag = e_READ_METER_TIME_TIMEOUT;

	if(YMDFlag == TRUE)
    {
        YMDFlag = FALSE;
    }
}

FUNCTION_USE_SWITCH_t Function_Use_Switch = {0};

U8        TimeCalFlag = FALSE;

//ostimer_t *EventReportClockOvertimer = NULL;
ostimer_t *ApsLifeCycletimer = NULL;
ostimer_t *WaitOnlinetimer	 = NULL;
ostimer_t *clockRecoverTimer = NULL;
ostimer_t *readMeterClockTimer = NULL;
ostimer_t *ReadMeterCtrlTimer = NULL;

U8	ApsPacketSeq = 0xFF;
U8	ReadClockState = e_READMETER_CLOCK;
U8	ReadClockWayState = e_READCLOCK_NULL;
U32 meterSeconds = 0;

sys_time_t meterDate = {0};

static void bcd_time_to_tm(date_time_s *Time, U8* buf)
{
	Time->second = buf[0];
    Time->minute = buf[1];
    Time->hour= buf[2];
    Time->day = buf[3];
    Time->month  = buf[4];
    Time->year = buf[5];
}

void date_time_to_sysdate(SysDate_t *sTime, date_time_s dTime)
{
	sTime->Sec = dTime.second;
    sTime->Min = dTime.minute;
    sTime->Hour = dTime.hour;
    sTime->Day = dTime.day;
    sTime->Mon = dTime.month;
    sTime->Year = dTime.year%100;
}

U8 sta_calibration_clk(U8* buf, U16 len)
{
	U32 staNtbTime_now;		//��ǰ��ģ���NTBʱ��
	U32 ccoNtbTime;			//CCO�·�������CCO��NTBʱ��
	U32	systemDelayTime;	//staNtbTime_now��ccoNtbTime�Ĳ�ֵ
	date_time_s ccoTime;
	date_time_s	staTime;
	U8* data = buf;

	staNtbTime_now = ccoNtbTime = systemDelayTime = 0;

	TimeCalFlag = TRUE;
	app_printf("TimeCalFlag = TRUE\n");
	if(buf == NULL || len == 0)
	    return FALSE;
	if(len > 6)
	{
		bcd_time_to_tm(&ccoTime, &data[4]);	
		ccoNtbTime = (data[0])+(data[1]<<8)+(data[2]<<16)+(data[3]<<24);
		//app_printf("Cal Sta Now NTB!\n");
		staNtbTime_now = get_phy_tick_time();
		
		if(staNtbTime_now >= ccoNtbTime)
		{
			systemDelayTime 	= (staNtbTime_now - ccoNtbTime)*40/1000;
		}
		else 
		{
			systemDelayTime 	= (0xffffffff - (ccoNtbTime - staNtbTime_now) + 1)*40/1000;
		}
		
		if(systemDelayTime > 30*1000*1000)//30���ж�
		{
			return FALSE ;
		}
		//app_printf("ccoNtbTime = %d,\t staNtbTime_now = %d,\t systemDelayTime = %d\n", ccoNtbTime, staNtbTime_now, systemDelayTime);
	}
	else
	{
		bcd_time_to_tm(&ccoTime, data);
		//__memcpy((U8*)&ccoTime, data, sizeof(date_time_s));
	}
	
	bcd_to_bin((U8*)&ccoTime, (U8*)&ccoTime, sizeof(date_time_s));
	app_printf("ccoTime before [ %d-%d-%d  %d:%d:%d ]\n",
				ccoTime.year,ccoTime.month,ccoTime.day,ccoTime.hour,ccoTime.minute,ccoTime.second);

	get_other_date_time_s(ccoTime, (systemDelayTime / 1000 / 1000), &staTime);

	app_printf("SysDate Time Now[ %d-%d-%d  %d:%d:%d ]\n",staTime.year,staTime.month,staTime.day,staTime.hour,staTime.minute,staTime.second);

	SetBinTime(staTime);
	app_printf("ntbTimingRecord.systemDelayTime = %d, add_sec = %d\n", 
		systemDelayTime, (systemDelayTime / 1000 / 1000) );
    return TRUE;
}

void sta_clk_maintain_proc(work_t *work) 
{
	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
  
	
	U16    PayloadLen = 0;

	CLOCK_MAINTAIN_HEADER_t    *Clk_OverManageHeader = NULL;
	CLOCK_MAINTAIN_IND_t *pClockMaintainInd_t= NULL;
	 APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	Clk_OverManageHeader = (CLOCK_MAINTAIN_HEADER_t *)pApsHeader->Apdu;
	
	PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(CLOCK_MAINTAIN_HEADER_t);
	pClockMaintainInd_t = (CLOCK_MAINTAIN_IND_t*)zc_malloc_mem(sizeof(CLOCK_MAINTAIN_IND_t)+PayloadLen,"pClockMaintainInd_t",MEM_TIME_OUT);

	pClockMaintainInd_t->Type  =  Clk_OverManageHeader->Type;
	pClockMaintainInd_t->SendType = pMsduDataInd->SendType;
	
	pClockMaintainInd_t->AsduLength = Clk_OverManageHeader->AsduLength;
	__memcpy(pClockMaintainInd_t->Asdu, Clk_OverManageHeader->Asdu, pClockMaintainInd_t->AsduLength);
	
	__memcpy(pClockMaintainInd_t->DstMacAddr,pMsduDataInd->DstAddress,6);
	__memcpy(pClockMaintainInd_t->SrcMacAddr,pMsduDataInd->SrcAddress,6);
	
	if(sta_SetSwORValue_ind_hook != NULL)
    {
        sta_SetSwORValue_ind_hook(pClockMaintainInd_t);
    }
	zc_free_mem(pClockMaintainInd_t);

}
/*
void ByteRevOrder(U8 *pBuf,U8 len)
{
	U8 Tmpbuf[50] = {0};
	U8 i=0;
	for(i=0;i<len;i++)  			//�����ֽڵ���
		Tmpbuf[i] = pBuf[len-1-i];
	__memcpy(&pBuf[0], &Tmpbuf[0],len);
}

void _698GetTimeData(U8 *pData,U8 *Addrlen,U8 *pTimeSt)
{
	U8 addrlen = 0;
	U8 addrtype = 0;
	addrtype = (pData[4]>>6)&0xff; 
	switch(addrtype)
	{
		case 0:break;
		case 1:break;
		case 2:break;
		case 3:
			addrlen = 1;
			*Addrlen = addrlen;
			break;
		default:break;
	}
	__memcpy(pTimeSt,&pData[4+addrlen+1+2+_698_FRAME_TIME_POS],7);   //����7byte��ʱ������
}*/

void sta_pkg_698_bc_cali_frame(U8 *pData,U16 *pDatalen,date_time_s pSysDateTmp)
{    
	if(NULL == pData)
		return;

    U8  BcAddr_698 = 0xaa;
    PIID_s   piid;
    U16      apduLen = 0;
    OMD_s    Omd ={0x4000,0x7F,0x00};//����ʱ��㲥Уʱ
    //֡ͷ��֡
    Packet698Header(&BcAddr_698,pData,pDatalen,1,0);
 
    piid.serviceSeq = ++FrameSeq;
    piid.res        = 0;
    piid.servicePri = 0;

    Dl698SendApdu[apduLen++] = (U8)(Omd.OI >> 8);
    Dl698SendApdu[apduLen++] = (U8)(Omd.OI & 0xFF);
    Dl698SendApdu[apduLen++] = Omd.MethodId;
    Dl698SendApdu[apduLen++] = Omd.OperateMode;
    apduLen += DLT698ConstructData(&Dl698SendApdu[apduLen],DATE_TIME_S,0,(void *)&pSysDateTmp,0);
    
    Dl698SendApdu[apduLen++] = 0x00;   // ��ʱ���ǩ
    
    Dl698PacketFrame(ACTION_REQUEST, 0x01, piid, 0, FALSE, Dl698SendApdu, apduLen, pData, pDatalen);  
}

//��������������׼Уʱ������
void sta_set_moudle_time(U8* data,U16 datalen,U16 OffSet)
{
    SysDate_t JzqDate;
    date_time_s _date_time ;
    U16 Len =0;
    U8  pos =0;
    
    //�����ز���������645����698֡ ���ܽ�����ģ����������
    if(Check698Frame(data, datalen,NULL,NULL,NULL) == TRUE)
    {
        //дģ��ʱ�Ӳ����볬���¼��ϱ�����
		Server698FrameProc(data, datalen,(U8*)&_date_time,&Len);
		app_printf("len = %d\n",Len);
		date_time_to_sysdate(&JzqDate,_date_time);
		bin_to_bcd((U8*)&JzqDate,(U8*)&JzqDate,sizeof(SysDate_t));
    }			
	else if(Check645Frame(data, datalen,NULL,NULL,NULL) == TRUE)
	{
	    pos = 1+6+1+2;//68+addrlen+68+ctrl+datalen
	    __memcpy((U8*)&JzqDate, data+OffSet+pos, sizeof(SysDate_t));
        Sub0x33ForData((U8*)&JzqDate, sizeof(SysDate_t));
	}
	else
	{
	    app_printf("TimeCalibrate Frame error\n");
        return;
	}
	clock_recv_infoproc((U8*)&JzqDate, sizeof(SysDate_t));

	return;
}

void clean_clock_state()
{
    TimeOverReportState = e_TIMEOVER_NULL;
    ReadClockState = e_READMETER_CLOCK;
    ReadClockWayState = e_READCLOCK_NULL; 
    timer_stop(TimeOverControlTimer,TMR_NULL);
    timer_stop(readMeterClockTimer,TMR_NULL);
    timer_stop(TimeOverCtrlTimer,TMR_NULL);
    timer_stop(ReadMeterCtrlTimer,TMR_NULL);
    timer_stop(WaitOnlinetimer,TMR_NULL);
}

U8  clock_recv_infoproc(U8 *buff,U16 bufflen)
{
    if(sta_calibration_clk(buff,bufflen) == FALSE)
    {
        return FALSE;
    }
    else
    {
        clean_clock_state();
		TimeOverReportState = e_TIMEOVER_READMETER_TIME_FIRST;
		timer_start(TimeOverControlTimer);	
		app_printf("TimeOverReportState = %d\n", TimeOverReportState);
		return TRUE;
    }
}

void query_clk_or_value_proc(work_t *work)
{
	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;

	app_printf("ind dstaddr : ");
    dump_buf(pMsduDataInd->DstAddress, 6);
    app_printf("ind srcaddr : ");
    dump_buf(pMsduDataInd->SrcAddress, 6);

	QUERY_CLKSWOROVER_VALUE_HEADER_t	*Query_ClkSwOrValueHeader = NULL;//����֡ͷ
	Query_ClkSwOrValueHeader = (QUERY_CLKSWOROVER_VALUE_HEADER_t *)pApsHeader->Apdu;
	U8 Len = 0;
	
	QUERY_CLKSWOROVER_VALUE_IND_t *pQueryClkORValueInd_t = NULL;
	
	Len = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(QUERY_CLKSWOROVER_VALUE_HEADER_t);
	pQueryClkORValueInd_t = (QUERY_CLKSWOROVER_VALUE_IND_t*)zc_malloc_mem(sizeof(QUERY_CLKSWOROVER_VALUE_IND_t) +Len ,"pQueryClkORValueInd_t",MEM_TIME_OUT);
	
	__memcpy(pQueryClkORValueInd_t->Macaddr,Query_ClkSwOrValueHeader->Macaddr,6);
	//pQueryClkORValueInd_t->AutoSwState = DevicePib_t.AutoCalibrateSw;
	//pQueryClkORValueInd_t->Time_Over_Value = DevicePib_t.Over_Value;
	
	__memcpy(pQueryClkORValueInd_t->DstMacAddr,pMsduDataInd->DstAddress,6);
	__memcpy(pQueryClkORValueInd_t->SrcMacAddr,pMsduDataInd->SrcAddress,6);
	if(sta_QuerySwORValue_ind_hook != NULL)
    {
        sta_QuerySwORValue_ind_hook(pQueryClkORValueInd_t);
    }

    zc_free_mem(pQueryClkORValueInd_t);
}

//extern void bcd_to_bin(U8 *src, U8 *dest, U8 len);

void sta_clear_time_manager_flash()
{
	DevicePib_t.TimeDeviation = PIB_TIMEDEVIATION;//ʱ���
	DevicePib_t.AutoCalibrateSw = TimeCalOff;//�Զ�Уʱ����
	DevicePib_t.Over_Value = 5;	   //������ֵ
	DevicePib_t.TimeReportDay = 0;	//�����ϱ����գ�������ʱ������գ�
}

void aps_life_cycle_cb(struct ostimer_s *ostimer, void *arg)
{
	ApsPacketSeq = 0xFF;
}

static void sta_wait_online_timer_cb(struct ostimer_s *ostimer, void *arg)
{		
	app_printf("WaitOnlinetimerCB!!!   AppNodeState = %d\n", AppNodeState);
	if(AppNodeState == e_NODE_ON_LINE)
	{
		//����ʱ�ӳ�������
		if(TMR_STOPPED == zc_timer_query(TimeOverControlTimer))
		{
			TimeOverReportState = e_TIMEOVER_READMETER_TIME_FIRST;
			timer_start(TimeOverControlTimer);		//sta_time_over_control_timer_cb
		}
	}
	else
	{
		timer_start(WaitOnlinetimer);
		app_printf("wait Online into TimeOverControlTimer!!\n");
	}
}

static void sta_clk_recover_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	extern sys_time_t meterDate;

	if(getHplcTestMode() == RD_CTRL)
	{
		return;
	}
	
	if((nnib.MacAddrType == e_MAC_METER_ADDR) && FactoryTestFlag != 1)  //ɽ��Ϊ &&nnib.NodeState == e_NODE_ON_LINE
    {
    	app_printf("Runed,nnib.MacAddrType:%d\n", nnib.MacAddrType);
    	//�жϲ�ֵ��Ч��
		if(DevicePib_t.TimeDeviation != PIB_TIMEDEVIATION)  //ɽ��Ϊ0x3FFFFFFF
		{
	    	meterSeconds = 0;
			memset((U8*)&meterDate, 0x00, sizeof(sys_time_t));
			
	    	ReadClockWayState = e_STA_CLOCKRECOVER;
			ReadClockState = e_READMETER_CLOCK;
			timer_start(readMeterClockTimer);	//readMeterClockTimerCB
		}
		else	//��Ϊ��ʼֵ��˵����һ���ϵ磬���Բ����볬���¼��ϱ�����
		{
			app_printf("first power on, meter time Not TRUE!\n");
			return;
		}
	}
	else
	{
		timer_start(clockRecoverTimer);
	}
}

static void sta_clear_meter_date()
{	
	meterDate.year = 0;
	meterDate.mon = 0;
	meterDate.day = 0;
	meterDate.hour = 0;
	meterDate.min = 0;
	meterDate.sec= 0;
}
#if defined(ZC3750STA)

//��ȡ���ܱ�ʱ�ӣ���������ܱ�ʱ�Ӻ�ģ��ʱ�ӵĲ�ֵ
void sta_read_meter_clk()
{
	//�뻻����
	if(DLT698 == DevicePib_t.Prtcl)
	{
		ReadDLT698Time(DevicePib_t.Prtcl);
	}
	else
	{
		ReadDLT645Time(DevicePib_t.Prtcl);
	}

}
#endif
void sta_cali_time_diff(sys_time_t *MeterDate, S32 *TimeDiffence, U32 *MeterSeconds)
{
	extern SysDate_t  SysDate_Now;
	
	//static U8 FirstTimeAcqre = 0;   //�ϵ�ʱʱ���ȡ��־
	ulong_t staSeconds = 0;
	sys_time_t staDate = {0};
	U32 os_secs = os_rtc_time();
	
	secs_to_time(&staDate, os_secs);
	
	dump_buf((U8 *)&SysDate_Now,6);
	app_printf("TimeCalFlag = %d\n",TimeCalFlag);
	SysDateTotm(SysDate_Now,MeterDate);
	if((0 > MeterDate->year/*1900-2000*/) || 0 >= MeterDate->mon || 0 >= MeterDate->day)
	{
		sta_clear_meter_date();
		return;
	}

    app_printf("Read meterDate [ %02d-%02d-%d  %02d:%02d:%02d ]\n"
    	,MeterDate->year/*1900-2000*/,MeterDate->mon,MeterDate->day,MeterDate->hour,MeterDate->min,MeterDate->sec);
    app_printf("Module Now Time [ %02d-%02d-%02d  %02d:%02d:%02d ]\n"
		,staDate.year/*1900-2000*/,staDate.mon,staDate.day,staDate.hour,staDate.min,staDate.sec);

	*MeterSeconds = time_to_secs(MeterDate->year, MeterDate->mon, MeterDate->day, MeterDate->hour, MeterDate->min, MeterDate->sec);
	
	staSeconds = time_to_secs(staDate.year, staDate.mon, staDate.day, staDate.hour, staDate.min, staDate.sec);
	/*if(FirstTimeAcqre == 0)    //������ϵ������STAʱ��ͱ��ͬ��
	{
		FirstTimeAcqre = 1;
		os_set_time(*MeterSeconds);  
	}*/
	
	if(!TimeCalFlag)		//����ʱ�ӻָ������Խ��в�ֵ���㣬ֻ��Уʱ����
	{
		return;
	}
	
	if(PIB_TIMEDEVIATION < abs(staSeconds - *MeterSeconds))
	{
		if((staSeconds - *MeterSeconds) > 0)
		{
			*TimeDiffence = PIB_TIMEDEVIATION-1; //PIB_TIMEDEVIATIONΪĬ��ֵ���˴���Ҫ-1�ſ����ϱ�
		}
		else
		{
			*TimeDiffence = 0xffffffff-1;
		}	
	}
	else
	{
		*TimeDiffence = staSeconds - *MeterSeconds;
	}
	
	app_printf("TimeDeviation = %d\n", *TimeDiffence);

}

#if defined(ZC3750STA)
// ʱ�ӻָ�
static void sta_clk_recover()
{
	  //��ȡ���ʱ��
	app_printf("---------------StaClockRecover!!!-----------\n");
	U32 staSeconds;
	sys_time_t staDate = {0};

	staSeconds = 0;
	//DevicePib_t.TimeDeviation = 400;  //�����ϱ������ã�����ʱע�͵�
	app_printf("DevicePib_t.TimeDeviation = %d\n",DevicePib_t.TimeDeviation);

	app_printf("meterSeconds = %d\n",meterSeconds);
	
	if(DevicePib_t.TimeDeviation != PIB_TIMEDEVIATION)  //ɽ��Ϊ>0
	{
		if(DevicePib_t.TimeDeviation > 0)
		{
			staSeconds  = meterSeconds + abs(DevicePib_t.TimeDeviation);//�����������
		}
		else
		{
			staSeconds  = meterSeconds - abs(DevicePib_t.TimeDeviation);//�����������
		}
	}
	else
	{
	    staSeconds = meterSeconds;
	}

	app_printf("staSeconds = %d\n", staSeconds);

	//staDate = localtime(&staSeconds);
	secs_to_time(&staDate, staSeconds);
	
	app_printf("Sta Now Time [ %d-%d-%d  %d:%d:%d ]\n",
		staDate.year,staDate.mon,staDate.day,staDate.hour,staDate.min,staDate.sec);

	//time_t localTime;
    //localTime = mktime(staDate);
	ulong_t os_secs = 0;
	os_secs = time_to_secs(staDate.year, staDate.mon,staDate.day,staDate.hour,staDate.min,staDate.sec);
	
    os_set_time(os_secs);
	//��������ɽ�����ã�ɽ��ʹ��ʱҪ���˵�
	//DevicePib_t.TimeDeviation = 400;
	if(PROVINCE_SHANDONG==app_ext_info.province_code)   //ɽ��ʹ����Ҫ����
	{
		return;
	}
	app_printf("DevicePib_t.TimeDeviation = %d, DevicePib_t.Over_Value = %d~~\n", DevicePib_t.TimeDeviation, DevicePib_t.Over_Value);
	if(abs(DevicePib_t.TimeDeviation) <= (DevicePib_t.Over_Value*60))
	{
		app_printf("DevicePib_t.TimeDeviation <= DevicePib_t.Over_Value\n");
		return;
	}

	if(NULL == WaitOnlinetimer)
	{
		WaitOnlinetimer = timer_create(30*100,
							  0, 
							  TMR_TRIGGER, 
							  sta_wait_online_timer_cb, 
							  NULL, 
							  "WaitOnlinetimerCB"
							  );
	}
	timer_start(WaitOnlinetimer);
	
}
#endif

//�Զ�Уʱ
static void sta_auto_cali(U32 meterSeconds, S32 TimeDeviation)
{
	app_printf("AutoCalibrate  Start !!!!\n");
	U8 BcAddr[6] = {0x99,0x99,0x99,0x99,0x99,0x99};
	U8  CtrlCode = 0x08;
	U8	*buf = zc_malloc_mem(50, "AutoCalibrate", MEM_TIME_OUT);
	U16	buflen = 0;
	U8 Baud = 0;
	date_time_s _datetimes = {0};
	//time_t meterSeconds_now = 0;
	U32 meterSeconds_now = 0;
	SysDate_t   _sysdate   = {0};
	sys_time_t correctData = {0};
	U16 CalibrateTimeSec = 4*60+30;

	if(abs(TimeDeviation) > 5*60)
	{
		if(TimeDeviation > 0)
		{
			meterSeconds_now = meterSeconds + CalibrateTimeSec;
		}
		else
		{
			meterSeconds_now = meterSeconds - CalibrateTimeSec;
		}
			
		//correctData = localtime(&meterSeconds_now);
		secs_to_time(&correctData, meterSeconds_now);
	}
	else
	{
    	//time_t LocalTime = os_time();
		U32 os_secs = os_rtc_time();
    	//correctData = localtime(&LocalTime);		//�뻻����
    	secs_to_time(&correctData, os_secs);   		//�ó�ʱ�����ʵ�ʵ����� 
	}

	app_printf("correctData:%d %d %d %d %d %d \n", correctData.year, correctData.mon,
				correctData.day, correctData.hour, correctData.min, correctData.sec);

	if(DLT645_2007 == DevicePib_t.Prtcl)
	{
		correctData.year = (correctData.year == 0? 0 : (correctData.year%100));
		tmToSysDate(&_sysdate,correctData);
		app_printf("%d %d %d %d %d %d \n", _sysdate.Year, _sysdate.Mon,
				_sysdate.Day, _sysdate.Hour, _sysdate.Min, _sysdate.Sec);
        bin_to_bcd((U8*)&_sysdate, (U8*)&_sysdate, sizeof(SysDate_t));
    	Add0x33ForData((U8*)&_sysdate, sizeof(SysDate_t));
    	Packet645Frame(buf, (U8 *)&buflen, BcAddr,CtrlCode, (U8*)&_sysdate, sizeof(SysDate_t));
    	sta_time_calibrate_put_list(buf, buflen,0,Baud,50);
	}
	else if(DLT698 == DevicePib_t.Prtcl)
	{
		correctData.year = correctData.year - 1900;
		tmTodate_times(&_datetimes,correctData);
		app_printf("%d %d %d %d %d %d \n", _datetimes.year, _datetimes.month,
				   _datetimes.day, _datetimes.hour, _datetimes.minute, _datetimes.second);
		sta_pkg_698_bc_cali_frame(buf, &buflen,_datetimes);
        sta_time_calibrate_put_list(buf, buflen,0,Baud,50);
	}
	if(PROVINCE_SHANDONG==app_ext_info.province_code)  //ɽ��ʹ��
	{
		ulong_t NowTime = os_rtc_time()/(24*60*60);
		NowTime *= (24*60*60);
		DevicePib_t.TimeReportDay = NowTime;
		staflag = TRUE;
	}
	zc_free_mem(buf);
}

static void sta_read_meter_ctrl_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	extern sys_time_t meterDate;

	static U8 	readmeterfrq = 0;
	
	switch (ReadClockState)
	{
		case e_READMETER_CLOCK:
		{
			if(0 <= meterDate.year/*1900-2000*/ && 0 <= meterDate.mon && 0 <= meterDate.day && g_ReadMeterResFlag==e_READ_METER_TIME_SUCCESS)
			{
				ReadClockState = ReadClockWayState;
				app_printf("Read meter time success!\n");
			}
			else
			{
				ReadClockState = e_READMETER_CLOCK;
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				readmeterfrq++;
			}

			if(readmeterfrq > 2)
			{
				ReadClockState = e_READCLOCK_NULL;
				ReadClockWayState = e_READCLOCK_NULL;
				g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
				readmeterfrq = 0;
				app_printf("Read meter time timeout!\n");
			}
			
			break;
		}
		case e_CLOCKMAINTAIN_MANAGE:
		case e_STA_CLOCKRECOVER:
		default:
		{
			ReadClockState = e_READCLOCK_NULL;
			ReadClockWayState = e_READCLOCK_NULL;
			break;
		}
	}
	if(e_READCLOCK_NULL != ReadClockState)
	{
		timer_start(readMeterClockTimer);
	}

}

static void sta_read_meter_clk_SM(work_t *work)
{
#if defined(ZC3750STA)
	//S32 Temp = 0;
	app_printf("ReadClockReq->ReadClockType = %d\n", ReadClockState);
	switch (ReadClockState)
	{
		case e_READMETER_CLOCK:
		{
			app_printf("ClockRecoverStateMachine:READMETER\n");
			//sta_read_meter_clk(&meterDate, &DevicePib_t.TimeDeviation, &meterSeconds);
			g_ReadMeterResFlag = e_READ_METER_TIME_NONE;
			sta_read_meter_clk();
			/*if(0 <= meterDate.tm_year+ 1900-2000 && 0 < meterDate.tm_mon+1 && 0 < meterDate.tm_mday)
			{
				app_printf("ReadClockState = ReadClockWayState;\n");
				ReadClockState = ReadClockWayState;
			}*/
			break;
		}
		case e_CLOCKMAINTAIN_MANAGE:
		{
			app_printf("ClockRecoverStateMachine:AUTOCALI\n");
			

			if(TimeCalFlag == TRUE && meterSeconds != 0)
			{
				sta_auto_cali(meterSeconds, DevicePib_t.TimeDeviation);
			}

			break;
		}
		case e_STA_CLOCKRECOVER:
		{
			app_printf("ClockRecoverStateMachine:CLK_RECOVER\n");
			sta_clk_recover();
			break;
		}
	}
	if(e_READCLOCK_NULL != ReadClockState)
	{
		timer_start(ReadMeterCtrlTimer);
	}
#endif
}

static void sta_read_meter_clk_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	if(NORM != HPLC.testmode)
        return;
	if(e_READCLOCK_NULL != ReadClockState)
	{
	    work_t *work = zc_malloc_mem(sizeof(work_t),"AppReadMeterClock",MEM_TIME_OUT);
		work->work = sta_read_meter_clk_SM;
		work->msgtype = READ_METER_CLOCK;
		post_app_work(work); 
		//AppReadMeterClock();
	}


	app_printf("ReadClockState = %d, ReadClockWayState = %d\n", ReadClockState, ReadClockWayState);
	//dump_buf((U8*)&meterDate,sizeof(meterDate));
}
//STA�Զ�Уʱ������Уʱ֡
void sta_auto_cali_time(void)
{
	if(PIB_TIMEDEVIATION != DevicePib_t.TimeDeviation)
	{
		sta_auto_cali(meterSeconds, DevicePib_t.TimeDeviation);
	}
}

void sta_query_clk_or_value_Indication(QUERY_CLKSWOROVER_VALUE_IND_t *QueryClkORValueInd)
{
    if(AppNodeState != e_NODE_ON_LINE || nnib.MacAddrType == e_UNKONWN)
	{
	    app_printf("exacttime module abnormal,don't execute indication\n");
		return;
	}
	QUERY_CLKSWOROVER_VALUE_RESPONSE_t *QuerySwORValueRsp = NULL;
	QuerySwORValueRsp = (QUERY_CLKSWOROVER_VALUE_RESPONSE_t*)zc_malloc_mem(sizeof(QUERY_CLKSWOROVER_VALUE_RESPONSE_t) + 30, "QueryIdInfoRsp",MEM_TIME_OUT);
	
	__memcpy(QuerySwORValueRsp->Macaddr,QueryClkORValueInd->Macaddr,6);	
	QuerySwORValueRsp->AutoSwState = DevicePib_t.AutoCalibrateSw;
	QuerySwORValueRsp->Time_Over_Value = DevicePib_t.Over_Value;

	app_printf("QuerySwORValueRsp QueryClkORValueRsp->AutoSwState = %d, QuerySwORValueRsp->Time_Over_Value = %d\n", 
		QuerySwORValueRsp->AutoSwState, QuerySwORValueRsp->Time_Over_Value);
		
	__memcpy(QuerySwORValueRsp->DstMacAddr,QueryClkORValueInd->SrcMacAddr,6);
	__memcpy(QuerySwORValueRsp->SrcMacAddr,QueryClkORValueInd->DstMacAddr,6);	

	ApsQueryClkSwAndOverValueResp(QuerySwORValueRsp);
	zc_free_mem(QuerySwORValueRsp);
}

void sta_set_clk_or_value_Indication(CLOCK_MAINTAIN_IND_t *SetClkORValueInd)
{
	if (AppNodeState != e_NODE_ON_LINE || nnib.MacAddrType == e_UNKONWN)
	{
		app_printf("exacttime module abnormal,don't execute indication\n");
		return;
	}
	extern sys_time_t meterDate;
	U8 clock_maintain_type = 0;
	clock_maintain_type = SetClkORValueInd->Type;
	app_printf("pClockMaintainInd_t->Type = %d!\n", clock_maintain_type);
	if (e_AUTO_TIMING_SWITCH == clock_maintain_type)
	{
		U8 FuncState = 0;
		__memcpy(&FuncState, SetClkORValueInd->Asdu, sizeof(U8));

		if (FuncState == TimeCalOn) // �����Զ�Уʱ���ؿ���
		{
				app_printf("Auto  CalibrateSw  On\n");
				meterSeconds = 0;
				memset((U8 *)&meterDate, 0x00, sizeof(sys_time_t));
				ReadClockWayState = e_CLOCKMAINTAIN_MANAGE;
				ReadClockState = e_READMETER_CLOCK;
				timer_start(readMeterClockTimer); //  readMeterClockTimerCB
				if (DevicePib_t.AutoCalibrateSw != TimeCalOn)
				{
					DevicePib_t.AutoCalibrateSw = TimeCalOn;
					staflag = TRUE;
				}
		}
		else // �����Զ�Уʱ���عر�
		{
				app_printf("Auto  CalibrateSw  Off\n");

				if (DevicePib_t.AutoCalibrateSw != TimeCalOff)
				{
					DevicePib_t.AutoCalibrateSw = TimeCalOff;
					staflag = TRUE;
				}
		}
	}
	else if (e_OVER_TIME_VALUE_SET == clock_maintain_type)
	{

		DevicePib_t.Over_Value = *SetClkORValueInd->Asdu;
		app_printf("DevicePib_t.Over_Value = %d\n", DevicePib_t.Over_Value);

		staflag = TRUE;
		if (DevicePib_t.Over_Value == 0)
		{
				// �����¼��ϱ��ر�
				app_printf("TimeOver_Value = 0,OVER_TIME_VALUE turn off!!\n");
		}
	}
	else if (e_CLOCK_MAINTAIN_INFO == clock_maintain_type)
	{
		clock_recv_infoproc(SetClkORValueInd->Asdu, SetClkORValueInd->AsduLength);
	}

	return;
}

void sta_cali_clk_put_list(U8 *pTimeData, U16 dataLen, U8 Baud,U8 Protocl)
{
    add_serialcache_msg_t serialmsg;
    memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));

	//extern void sta_serial_cfg(int8_t baudcnt, int8_t verif);
	serialmsg.Uartcfg = NULL;
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
    
    serialmsg.Match  = NULL;
    serialmsg.Timeout = NULL;
    serialmsg.RevDel = NULL;

    serialmsg.baud  = Baud;
    serialmsg.verif = 1;    /*0:��У�飬1��żУ�飬2����У��*/


    serialmsg.Protocoltype = Protocl;
    serialmsg.IntervalTime = 10;
	serialmsg.DeviceTimeout = 200;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = dataLen;
	serialmsg.FrameSeq = Baud;
	serialmsg.lid = e_Serial_Trans;
	serialmsg.ReTransmitCnt = 0;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;


	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pTimeData,FALSE);

    return;
}

void sta_time_manager_init()
{
	//Function_Use_Switch.ProvinceType = e_GW_BASE;
	Function_Use_Switch.AutoCaliState = 1;  		//�Զ�Уʱ���أ�����ʡ��ѡ��Ĭ�Ͽ�
	
	ReadMeterCtrlTimer = timer_create(3*TIMER_UNITS,			//todo gcl  ɽ����ʱʱ��Ϊ2��
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
	                            sta_read_meter_ctrl_timer_cb,
	                            NULL,
	                            "ReadMeterCtrlTimerCB"
	                           );

	readMeterClockTimer = timer_create(1*TIMER_UNITS,                //todo gcl  ɽ����ʱʱ��Ϊ3��
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
	                            sta_read_meter_clk_timer_cb,
	                            NULL,
	                            "readMeterClockTimerCB"
	                           );

	clockRecoverTimer = timer_create(1*TIMER_UNITS,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
	                            sta_clk_recover_timer_cb,
	                            NULL,
	                            "clockRecoverTimerCB"
	                           );
	WaitOnlinetimer = timer_create(30*100,
							  0, 
							  TMR_TRIGGER, 
							  sta_wait_online_timer_cb, 
							  NULL, 
							  "WaitOnlinetimerCB"
							  );
	if(getHplcTestMode() == NORM && FactoryTestFlag != TRUE)
	{
		timer_start(clockRecoverTimer);
	}
}

void Module_Time_Sync_Ind(work_t *work)
{}

#endif
































