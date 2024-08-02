#include "ZCsystem.h"
//#include <time.h>
#include "app_alarm_clock.h"
#include "app_clock_os_ex.h"
#include "printf_zc.h"
//#include "app_datafreeze.h"
#include "app.h"

//#if defined(STATIC_NODE)

ostimer_t *alarm_clock_timer = NULL;
alarm_clock_list_header_t  AlarmClockList;

/*****************************************************************
*CurrSec      :当前系统秒(System seconds since 19700101)
*interval_min :采集间隔(单位:min)
*
******************************************************************/
uint32_t alarm_clock_init(uint32_t CurrSec, uint32_t interval_min,U8 clocktype)
{
	uint32_t NewSec;
	uint16_t  in;
	int8_t i;
	sys_time_t Tm;

	NewSec = 0;
	//__memcpy(&Tm, localtime((time_t*)&CurrSec), sizeof(Tm));
	secs_to_time(&Tm, CurrSec);
	//app_printf("[ %d-%d-%d  %d:%02d:%02d ]------------current clock,interval:%d\n",
		//			Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec,interval_min);
	//24*60 = 1440min
	if(interval_min > 60)
	{
		in = interval_min/60;
		if(in > 24)//月冻结采集任务,每月的1号采集月冻结且在0点10分采集月冻结
		{	
			if(Tm.day  == 1 && Tm.hour == 0 && Tm.min < 10)
			{
				;
			}
			else
			{
				Tm.mon  = Tm.mon+1;
			}
			Tm.day = 1;
			Tm.hour = 0;
			Tm.min  = 10;
			Tm.sec  = 0;
			NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
		}
#if defined(STATIC_MASTER)
		if(in == 24)
		{
			Tm.hour = 22;
			Tm.min  = 0;
			Tm.sec  = 0;
			NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
		}
#endif
		else//日冻结采集任务 每日的0点过5分开始采集日冻结 电表状态字是每日的 0点15分开始采集
		{
			if(clocktype == e_CLOCK_STATUS)
			{	
				if(Tm.min < 15 && Tm.hour == 0)
				{
					;
				}
				else
				{
					Tm.day = Tm.day+1;
				}
				Tm.hour = 0;
				Tm.min  = 15;
				Tm.sec  = 0;
				NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
			}
			else if(clocktype == e_CLOCK_1DAY)
			{
				if(Tm.min < 5 && Tm.hour == 0)
				{
					;
				}
				else
				{
					Tm.day = Tm.day+1;
				}
				Tm.hour = 0;
				Tm.min  = 5;
				Tm.sec  = 0;
				NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
			}

		}
		
		return NewSec;		
	}
	else
	{
		in = 60/interval_min;
	}
	
	for(i=1; i<in+1; i++)
	{
		if(Tm.min < interval_min*i)
		{
			break;
		}
	}
	if(i > in)
	{
		app_printf("alarm_clock_init err!\n");
		return 0;
	}
	else if(i == in)//hour ++
	{
		uint32_t remain_sec;
		//计算距离下一个小时还有多少秒
		remain_sec = 60 * 60 - (Tm.min*60+Tm.sec);
		if(clocktype == e_CLOCK_15MIN)
		{
		    if(Tm.min < interval_min*(i-1)+7  )
		    {
                Tm.min = interval_min*(i-1)+7; 
                Tm.sec = 0;
    		    NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
		    }
		    else
		    {
                NewSec = CurrSec + remain_sec+7*60;
		    }
            //NewSec = CurrSec + remain_sec+7*60;
		}
		else
		{
            NewSec = CurrSec + remain_sec;
		}

	}
	else
	{
	    if(clocktype == e_CLOCK_15MIN)
	    {
	        //此处+7 是因为15分钟任务在整点基础上往后延迟了七分钟，需要兼容实际处于整点和往后延迟7分之间的情况 
	        //保证闹钟任务触发时能够按照设计去实现
	        if(Tm.min < interval_min*i+7 &&  Tm.min >= (interval_min*(i-1))+7 )
	        {
	            Tm.min = interval_min*i+7; 
	        }
	        else
	        {
	            Tm.min = (interval_min*(i-1))+7;
	        }
            //Tm.tm_min = interval_min*i+7; 
    		Tm.sec = 0;
    		NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
	    }
	    else
	    {
        	Tm.min = interval_min*i;
    		Tm.sec = 0;
    		NewSec = time_to_secs(Tm.year,Tm.mon,Tm.day,Tm.hour,Tm.min,Tm.sec);
	    }

	}
	
	return NewSec;
}
void  alarm_clock_reload(uint32_t CurrSec, uint32_t interval_sec,sys_time_t *Tm)

{
	uint32_t NewSec;
	
	NewSec = CurrSec + interval_sec;
	secs_to_time(Tm, NewSec);
	return ;
}

uint16_t get_alarm_clock_list_type_Info(U8 type,sys_time_t *pTm)
{
	alarm_clock_list_header_t *list_header = &AlarmClockList;
	if(list_header->nr <= 0)
    {
        return 0;
    }
	uint16_t  num;
	list_head_t *pos,*node;
	alarm_clock_t *pAlarmClock;

	num = 0;
	list_for_each_safe(pos, node,&list_header->link)
	{
		pAlarmClock = list_entry(pos, alarm_clock_t, link);
		if(pAlarmClock->type == type)
		{
        	num ++;
        	*pTm = pAlarmClock->Tm;
		}

	}
	return num;
}

uint16_t get_alarm_clock_list_type_num(U8 type)
{
	alarm_clock_list_header_t *list_header = &AlarmClockList;
	if(list_header->nr <= 0)
    {
        return 0;
    }
	uint16_t  num;
	list_head_t *pos,*node;
	alarm_clock_t *pAlarmClock;

	num = 0;
	list_for_each_safe(pos, node,&list_header->link)
	{
		pAlarmClock = list_entry(pos, alarm_clock_t, link);
		if(pAlarmClock->type == type)
        	num ++;	

	}
	return num;
}
void del_alarm_clock_list_by_type(U8 type)
{
	list_head_t *pos,*node;
	alarm_clock_t *pAlarmClock;
	alarm_clock_list_header_t *list_header = &AlarmClockList;
	if(list_header->nr <= 0)
    {
        return;
    }
	list_for_each_safe(pos, node,&list_header->link)
	{
		pAlarmClock = list_entry(pos, alarm_clock_t, link);
		if(pAlarmClock->type == type)
		{
			list_del(&pAlarmClock->link);
			zc_free_mem(pAlarmClock);
			if(list_header->nr)
				list_header->nr--;
		}
	}
}


void del_alarm_clock_list(work_t *work)
{
	list_head_t *pos,*node;
	alarm_clock_t *pAlarmClock;
	alarm_clock_list_header_t *list_header = &AlarmClockList;
	if(list_header->nr <= 0)
    {
        return;
    }
	list_for_each_safe(pos, node,&list_header->link)
	{
		pAlarmClock = list_entry(pos, alarm_clock_t, link);
		list_del(&pAlarmClock->link);
		zc_free_mem(pAlarmClock);
	}
	INIT_LIST_HEAD(&list_header->link);
    list_header->nr = 0;
}
void query_alarm_clock_list(work_t *work)
{
	U8 i=0;
	SysDate_t  SysData;
	list_head_t *pos,*node;
	alarm_clock_t *pAlarmClock;
	alarm_clock_list_header_t *list_header = &AlarmClockList;
	if(list_header->nr <= 0)
    {
		app_printf("alarm_clock_list empty\n");
        return;
    }
	app_printf("idx\talarm clock\n");
	list_for_each_safe(pos, node,&list_header->link)
	{
		pAlarmClock = list_entry(pos, alarm_clock_t, link);
		ostm2OutClock(&SysData, &pAlarmClock->Tm);
		app_printf("%d\t[ %d-%d-%d  %d:%02d:%02d ]--------\n",i++,
									SysData.Year,SysData.Mon,SysData.Day,SysData.Hour,SysData.Min,SysData.Sec);
	}
}
static int32_t insert_alarm_clock_list(alarm_clock_list_header_t *list_header, 
														alarm_clock_t *new_alarm_clock) 
{
    list_head_t *p;    
	int32_t  ret;

	if(new_alarm_clock->enq_check != NULL)
    {
        if((ret = new_alarm_clock->enq_check(list_header, new_alarm_clock)) != 0)
            return ret;
    }

	p=&list_header->link;
   	
	list_add_tail(&new_alarm_clock->link, p);
	++list_header->nr;
	return 0;
}
static int32_t check_alarm_clock_enq(alarm_clock_list_header_t *list_header, 
														alarm_clock_t *new_alarm_clock)
{
	if(list_header->nr >= list_header->thr)
	{
		app_printf("alarm nr:%d,thr:%d\n",list_header->nr, list_header->thr);
		return -1;
	}
	return 0;
}

void add_alarm_clock_list(sys_time_t *pNextTime, U8 type, void *exefunc, uint8_t *payload, uint16_t len)
{	
	int32_t  ret;
	alarm_clock_t *new_alarm_clock = zc_malloc_mem(sizeof(alarm_clock_t)+len,"PRML",MEM_TIME_OUT);
	if(NULL == new_alarm_clock)
	{
		return;
	}

	__memcpy(&new_alarm_clock->Tm, pNextTime, sizeof(new_alarm_clock->Tm));	
	new_alarm_clock->type      = type;
	new_alarm_clock->enq_check = check_alarm_clock_enq;
	new_alarm_clock->exe_func  = exefunc;
	if(payload && len)
		__memcpy(new_alarm_clock->payload, payload, len);
	
	
	if((ret = insert_alarm_clock_list(&AlarmClockList, new_alarm_clock) != 0))
	{
		zc_free_mem(new_alarm_clock);   
	}
	else
    {
	    if(TMR_STOPPED==zc_timer_query(alarm_clock_timer))
	    {
		    modify_alarm_clock_time(1);
	    }
    }
	return;
}

static void update_alarm_clock_list(work_t *work)
{
	list_head_t *pos,*node;
    alarm_clock_to_app_t  *msg_info = (alarm_clock_to_app_t*)work->buf;
	alarm_clock_t *pAlarmClock;
	if(msg_info->list_header->nr <= 0)
    {
        return;
    }
	ulong_t sysTime;
	sysTime = os_rtc_time();
	U16 timeout = 0;
	list_for_each_safe(pos, node,&msg_info->list_header->link)
    {
        pAlarmClock = list_entry(pos, alarm_clock_t, link);
		if(NULL != pAlarmClock)
		{
			ulong_t alarmTime = time_to_secs(pAlarmClock->Tm.year,pAlarmClock->Tm.mon,pAlarmClock->Tm.day,
			                            pAlarmClock->Tm.hour,pAlarmClock->Tm.min,pAlarmClock->Tm.sec);
			if(sysTime < alarmTime)
				continue;
			if(pAlarmClock->type == e_CLOCK_1MIN || pAlarmClock->type == e_CLOCK_15MIN)
			{
				timeout = ALARM_MINTIMEOUT;
			}
			else if(pAlarmClock->type == e_CLOCK_1MON || pAlarmClock->type == e_CLOCK_1DAY)
			{
				timeout = ALARM_FREEZETIMEOUT;
			}
			if((alarmTime+timeout) >= sysTime)
			{
				pAlarmClock->state = e_ALARM_STATE_EXTING;
			}
			else
			{
				pAlarmClock->state = e_ALARM_STATE_DELETE;
			}
			if(pAlarmClock->exe_func != NULL)
			{
				pAlarmClock->exe_func(pAlarmClock);
			}	
		
			list_del(&pAlarmClock->link);
			if(msg_info->list_header->nr)
				msg_info->list_header->nr--;
			zc_free_mem(pAlarmClock);
		}
	}
	modify_alarm_clock_time(100);
}

static void AlarmClockTimerCB(struct ostimer_s *ostimer, void *arg)
{
	if(!((alarm_clock_list_header_t *)arg)->nr)
		return;
	
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(alarm_clock_to_app_t),"AlarmClockTimer",MEM_TIME_OUT);    
	if(NULL == work)
	{
		return;
	}
    alarm_clock_to_app_t  *msg_info = (alarm_clock_to_app_t*)work->buf;
    
    msg_info->list_header = (alarm_clock_list_header_t*)arg;
    msg_info->ostimer     = ostimer;

    work->work = update_alarm_clock_list;
    post_app_work(work);
	
    modify_alarm_clock_time(30*100);//防止丢消息保护
    return;
}
void modify_alarm_clock_time(uint32_t first)
{
	if(first == 0)
	{
		return;
	}
	if(alarm_clock_timer == NULL)
	{
		alarm_clock_timer = timer_create(first*10,
                              0,
                              TMR_TRIGGER,
                              AlarmClockTimerCB,
                              &AlarmClockList,
                              "AlarmClockTimerCB"
                             );
	}
	else
	{
		timer_stop(alarm_clock_timer,TMR_NULL);
		timer_modify(alarm_clock_timer,
						first*10, 
						0,
						TMR_TRIGGER,
						AlarmClockTimerCB,
						&AlarmClockList,
						"AlarmClockTimerCB",
						TRUE);
	}
	timer_start(alarm_clock_timer);
}
int8_t alarm_clock_timer_init(void)
{
    INIT_LIST_HEAD(&AlarmClockList.link);
    AlarmClockList.nr = 0;
    AlarmClockList.thr = 20;

    if(alarm_clock_timer == NULL)
    {
        alarm_clock_timer = timer_create(100,
                              1000,
                              TMR_TRIGGER,
                              AlarmClockTimerCB,
                              &AlarmClockList,
                              "AlarmClockTimerCB"
                             );
    }
    
    return TRUE;
}
//#endif


