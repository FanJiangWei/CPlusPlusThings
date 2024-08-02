#include "app_deviceinfo_report.h"
#include "app_clock_os_ex.h"
#include "app_slave_register_cco.h"
#include "app_area_indentification_cco.h"
#include "app_area_indentification_sta.h"
#include "app_base_multitrans.h"
#include "DataLinkGlobal.h"
#include "Datalinkdebug.h"
#include "app_alarm_clock.h"
#include "meter.h"
#include "netnnib.h"
#include "app.h"
#include "app_clock_sync_cco.h"
#if defined(STATIC_MASTER)
ostimer_t *wait_report_06f4_timer = NULL;

U8 CheckCollectCnt = 0;
U8  NeedkCheckReport06F4Num = 0;
U8 newnodenetwork = 0;

U8 check_collect_state()
{
	if(CheckCollectCnt >2)
	{
		CheckCollectCnt = 0;
		return TRUE;
	}
	app_printf("CheckCollectState\n");
	//检查采集功能的支持情况
	U16 ii;
	DEVICE_TEI_LIST_t GetDeviceListTemp_t;
	
	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)//存在采集器时，需要确保采集器的对应关系全部采集完成
	{
		if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii,&GetDeviceListTemp_t) == TRUE)
		{
			if(GetDeviceListTemp_t.NodeTEI != 0xfff)
			{
				if(GetDeviceListTemp_t.CollectNeed == 1 )
				{
					continue;
				}
				else//在列表中找到不支持分钟采集的点
				{
					CheckCollectCnt ++;
					return FALSE;
				}
			}
			else
			{
				continue;
			}
		}
	}
	return TRUE;
}

void save_collect_state(U8 *macaddr , U8 State)
{
	app_printf("SaveCollect\n");
	U16 usIdx=0;
	DEVICE_TEI_LIST_t GetDeviceListTemp_t;
	for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
	{
		if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&usIdx,&GetDeviceListTemp_t) == TRUE)
		{
			if(0==memcmp(macaddr, GetDeviceListTemp_t.MacAddr, 6))
			{	
				if(GetDeviceListTemp_t.NodeTEI == 0xfff)
				{
					return;
				}
				if(DeviceList_ioctl(DEV_SET_COLLECT,&usIdx,&State) == TRUE)
				{
					//app_printf("GetDeviceListTemp_t[%d].CollectNeed = %d\n",usIdx,GetDeviceListTemp_t.CollectNeed);
					return;
				}

			}
		}	
	}	
	return;
}


void wait_report_06f4_Proc(work_t* work)
{
    app_printf("WaitReport06F4Proc\n");
    //进行查询及06F4上报
    if(register_info.RegisterType != e_GENERAL_REGISTER)
    {
        register_info.RegisterType = e_NETWORK_REGISTER;
    	register_slave_start(3600*1000,0);
    	clean_registe_list();

    }

}

static void wait_report_06f4_timerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"WaitReport06F4TimerCB",MEM_TIME_OUT);
	work->work = wait_report_06f4_Proc;
    work->msgtype =WAIT_06F4;
	post_app_work(work);  
}

void modify_wait_report_06f4_timer(uint32_t Sec)
{
	if(NULL == wait_report_06f4_timer)
	{
		
		wait_report_06f4_timer = timer_create(Sec*1000,
							0,
							TMR_TRIGGER ,
							wait_report_06f4_timerCB,
							NULL,
							"wait_report_06f4_timerCB"
						   );
	}	
	else
	{
		timer_modify(wait_report_06f4_timer,
				Sec*1000,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				wait_report_06f4_timerCB,
				NULL,
				"wait_report_06f4_timerCB",
				TRUE);
	}
	app_printf("wait_report_06f4_timerCB %d Sec\n",Sec);
	timer_start(wait_report_06f4_timer);
}

void period_send_devicelist(alarm_clock_t *pAlarmClock)
{
	U16 ii=0;
	ulong_t alarmsec =0;
	sys_time_t tmbuf;
	sys_time_t roladtime;
	SysDate_t  SysDatenext;
	U8 *Index = (U8 *)pAlarmClock->payload;
	
	for(ii=0; ii< MAX_WHITELIST_NUM; ii++)
	{
		WhiteMaCJQMapList[ii].ReportFlag  = e_need_report;
	}
	
	alarmsec = time_to_secs(pAlarmClock->Tm.year,pAlarmClock->Tm.mon,pAlarmClock->Tm.day,pAlarmClock->Tm.hour,pAlarmClock->Tm.min,pAlarmClock->Tm.sec);
	alarm_clock_reload(alarmsec, SENDTIME06F4*60,&roladtime);
	__memcpy(&tmbuf, (void *)&roladtime,sizeof(tmbuf));

	ostm2OutClock(&SysDatenext, &tmbuf);
	app_printf("[  %02d-%02d-%02d  %02d:%02d:%02d ]------------next_alarm_clock\n",
						SysDatenext.Year+2000,SysDatenext.Mon,SysDatenext.Day,SysDatenext.Hour,SysDatenext.Min,SysDatenext.Sec);
	add_alarm_clock_list(&tmbuf, pAlarmClock->type, period_send_devicelist,Index, sizeof(U8));

	//进行查询及06F4上报	
    if(register_info.RegisterType != e_GENERAL_REGISTER)
    {
       register_info.RegisterType = e_NETWORK_REGISTER;
       register_slave_start(3600*1000,0);
       clean_registe_list();
    
    }

}
void check_network_status()
{	
	//static U16  NetWorkNum = 0;
	app_printf("appnnib->Networkflag %d\n",nnib.Networkflag);
	if(nnib.Networkflag == TRUE ) //组网完成
	{
		//进行查询及06F4上报
		if(register_info.RegisterType != e_GENERAL_REGISTER)
		{
		    clean_registe_list();
            register_info.RegisterType = e_NETWORK_REGISTER;
		    register_slave_start(3600*1000,0);
		}
		
		//组网完成后启动一次全网时钟同步
		if(zc_timer_query(RTCTimeSyncSendTimer) == TMR_STOPPED)
		{
			cco_modify_rtc_time_sync_send_timer(60);//组网完成60S之后进行RTC时钟同步
		}
		
	}
}

void cco_alarm_clock_task(U16 CltPeriod)
{
	uint8_t i;
	U8 CurrIndex = 0;
	sys_time_t tmbuf;	
	uint32_t NewSec;
	SysDate_t SysDateTmp;
	//date_time_s DateTemp = {0};
	ALARM_CLOCK_TYPE_e alarm_clock_type;
	
	alarm_clock_type = e_CCO_ClOCK;
	if(get_alarm_clock_list_type_num(alarm_clock_type)>0)
	{
		app_printf("current have curve task executing!\n");
		return;
	}
	for(i=0; i<MAX_DATAITEM_COUNT; i++)
	{
		
		if((NewSec = alarm_clock_init(os_rtc_time(),CltPeriod,alarm_clock_type)))
		{	CurrIndex = i ;
			//__memcpy(&tmbuf, localtime((time_t*)&NewSec), sizeof(tmbuf));
			
			secs_to_time(&tmbuf,NewSec);
			//app_printf("Time:%d-%d-%d %d:%d:%d\n",tmbuf.year,tmbuf.mon, tmbuf.day, tmbuf.hour,tmbuf.min, tmbuf.sec);
			ostm2OutClock(&SysDateTmp, &tmbuf);
			//SysDateToDatetimes(SysDateTmp,&DateTemp);
			app_printf("[ %02d-%02d-%02d  %d:%02d:%02d ]------------alarm_clock\n",
				SysDateTmp.Year+2000,SysDateTmp.Mon,SysDateTmp.Day,SysDateTmp.Hour,SysDateTmp.Min,SysDateTmp.Sec);
			add_alarm_clock_list(&tmbuf, alarm_clock_type, period_send_devicelist, &CurrIndex, sizeof(U16));
			break;
		}
		
	}
}
#ifdef JIANG_SU
void check_report_06f4()
{
	app_printf("RegisterInfo_t.RegisterType = %d\n",register_info.RegisterType);
    if(register_info.RegisterType == e_CJQSELF_REGISTER)
    {
        NeedkCheckReport06F4Num ++;
        if(NeedkCheckReport06F4Num >= 8)
        {
            modify_wait_report_06f4_timer(60);
            timer_stop(wait_report_06f4_timer,TMR_CALLBACK);
        }
        if(zc_timer_query(wait_report_06f4_timer) == TMR_STOPPED)
        {
            modify_wait_report_06f4_timer(15*60);
        }
    }
    else
    {
		NeedkCheckReport06F4Num = 0;
    }
}


void ProcOneStaNetworkStatusChangeIndication(work_t *work)
{
	app_printf("staNetworkStatusChange\n");
	newnodenetwork = TRUE;
    if(nnib.Networkflag == TRUE)
	{
        check_report_06f4();
    }
}

void (*pOneStaNetStatusChangeInd)(work_t *work) = NULL; //一个节点在数据链路层的状态发生变化时，通知APP

void NodeChange2app(U8 *addr,U8 addrlen)
{
	app_printf("NodeChange2app addr:");
	dump_buf(addr,addrlen);
    work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(NETWORK_ACTION_t)+addrlen,"NodeChange2app",MEM_TIME_OUT);
    work->work = pOneStaNetStatusChangeInd;
    __memcpy(work->buf,addr,addrlen);
	work->msgtype = NODECHANGE2APP;
    post_app_work(work);
}
#endif //JIANG_SU
#endif


