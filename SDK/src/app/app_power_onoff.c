/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */


#include "list.h"
#include "types.h"
#include "Trios.h"
#include "sys.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
#include "ZCsystem.h"
#include "app_base_multitrans.h"
//#include "app_eventreport.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_power_onoff.h"
#include "Datalinkdebug.h"
#include "app_cnf.h"
#include "app_dltpro.h"
#include "netnnib.h"
#include "plc_io.h"
#include "flash.h"
#include "dev_manager.h"
#include "dl_mgm_msg.h"
#include "DataLinkGlobal.h"



#if defined(STATIC_NODE)
static U8 add_power_off_info(U16 Seq,U8 *addr);
static void modify_UpdatePowerOffInfoTimer(uint32_t first);
static void power_off_send_timer_proc(work_t *work);   // 停电事件发送定时器回调函数
static void power_on_send_timer_proc(work_t *work);   // 复电事件发送定时器回调函数

ostimer_t *sta_power_off_report_timer = NULL;   // 停电上报定时器
ostimer_t *sta_power_on_report_timer = NULL;    // 复电上报定时器
ostimer_t *sta_power_off_send_timer = NULL;     // 停电发送定时器
ostimer_t *sta_power_on_send_timer = NULL;      // 复电发送定时器


U8  sta_power_off_bitmap_buff[PowerOffBitMapLen]={0};            //STA去重位图
U8  sta_power_off_report_bitmap_buff[PowerOffBitMapLen]={0};      //STA上报位图

U8   g_PowerEventTranspondFlag = FALSE;    // 停复电事件发送标志

//EventApsSeq_t aps_event_seq;   // 事件上报APS层序号

#if defined(ZC3750CJQ2)

U8  poweroncheckmeterflag = FALSE;
ostimer_t *cjq2_power_off_on_flow_protec_timer = NULL; //采集器停复电验表事件上报流程保护定时器
#endif
POWEROFFCHECK_INFO_t poweroffcheck_info[MAX_POWEROFFNUM];
ostimer_t *UpdatePowerOffInfoTimer=NULL;

//停电、拔出状态检测
//返回值：true-power off or plug out，FALSE-no power off and no plug out
U8 check_power_off_and_plug_out()
{
    if (DevicePib_t.PowerOffFlag == e_power_off_now || module_state.zero_cross_state != e_zc_get 
#if defined(ZC3750STA)
         || module_state.plug_state == e_plug_out
#endif      
        )
	{
		app_printf("Power off or plug out!!!\n");
		return TRUE;
	}

    return FALSE;
}

void power_off_event_report(U8 FunCode,U8 EventType ,U8 sendType,U8 *event ,U16 eventlen ,U16 ApsSeq)
{
    EVENT_REPORT_REQ_t *pEventReportRequest_t = NULL;
    U8   BCAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);


	if(AppNodeState != e_NODE_ON_LINE)
	{
		return;
	}

    pEventReportRequest_t = zc_malloc_mem(sizeof(EVENT_REPORT_REQ_t) + eventlen,"EventReportRequest",MEM_TIME_OUT);
    

    pEventReportRequest_t->Direction = e_UPLINK;
    pEventReportRequest_t->PrmFlag   = 1;
    pEventReportRequest_t->FunCode   = FunCode;
    pEventReportRequest_t->sendType =  sendType;   
	pEventReportRequest_t->PacketSeq =  ApsSeq;
    __memcpy(pEventReportRequest_t->MeterAddr,DevicePib_t.DevMacAddr,6);

    if(sendType == e_LOCAL_BROADCAST_FREAM_NOACK)
    {
        __memcpy(pEventReportRequest_t->DstMacAddr,BCAddress,6);
    }
    else
    {
        __memcpy(pEventReportRequest_t->DstMacAddr,ccomac,6);
    }
    
#if defined(ZC3750STA)
    __memcpy(pEventReportRequest_t->SrcMacAddr, DevicePib_t.DevMacAddr, 6);
    __memcpy(pEventReportRequest_t->MeterAddr,DevicePib_t.DevMacAddr,6);
#elif defined(ZC3750CJQ2)
    __memcpy(pEventReportRequest_t->SrcMacAddr,CollectInfo_st.CollectAddr,6);
    __memcpy(pEventReportRequest_t->MeterAddr,CollectInfo_st.CollectAddr,6);
#endif 
    
    __memcpy(pEventReportRequest_t->EventData, event, eventlen);
    pEventReportRequest_t->EvenDataLen = eventlen;
    
    ApsEventReportRequest(pEventReportRequest_t);

    zc_free_mem(pEventReportRequest_t);
}




typedef struct
{
    U8    EventType;
    U16   StartIdx;

    U8    EventData[0];
} __PACKED POWER_EVENT_DATA_t;


static void power_event_send_cal(U8 FunCode,U8 EventType ,U8 sendType,U16 ApsSeq)   // 停复电事件发送计算
{
    POWER_EVENT_DATA_t  *pPowerEventData = NULL;
    
    //U8   Event[PowerOffBitMapLen] = {0};
    //U16  EventLen = 0 ;
    U32  StartIndex=0;
    U16  ii;
    //U8   ReportStaBitMapBuff[PowerOffBitMapLen] = {0};
    U16  ReportBitMapLen = 0;
    
    //STA power event 
    if(FunCode==e_STA_ACT_CCO)  //停复电上报
    {	
        //bitmap power off
        if(EventType == e_BITMAP_EVENT_POWER_OFF)   // 停电事件位图上报
        {
            app_printf("run to this.\n");
            //StartIndex = bitmap_find_least_one(sta_power_off_report_bitmap_buff, MAX_WHITELIST_NUM);
            /**/
            for(ii=0; ii<MAX_WHITELIST_NUM; ii++)
            {
                if(sta_power_off_report_bitmap_buff[ii/8] & (0x01 << ii%8))
                {
	            	StartIndex = ii;    //获取起始序号，即起始TEI
	            	break;
                }
            }
            
            
            app_printf("StartIndex = %d\n",StartIndex);
            for(ii=MAX_WHITELIST_NUM; ii>1; ii--)
            {
                if(sta_power_off_report_bitmap_buff[ii/8] & 0x01 <<  ii%8)
                {
                	ReportBitMapLen = (ii-StartIndex+1)%8 ? (ii-StartIndex+1)/8+1:(ii-StartIndex+1)/8;  // 
                	// 获取发送位图长度
                	break;
                }
            }
            app_printf("ReportBitMapLen = %d\n",ReportBitMapLen);
            
            pPowerEventData = zc_malloc_mem(sizeof(POWER_EVENT_DATA_t) + ReportBitMapLen,"PowerEventData",MEM_TIME_OUT);
            
            for(ii=StartIndex; ii<=(StartIndex+ReportBitMapLen*8); ii++)
            {
                if(sta_power_off_report_bitmap_buff[ii/8] & 0x01 <<  ii%8)
                {
                    pPowerEventData->EventData[(ii-StartIndex )/8]|= 0x01 << (ii-StartIndex)%8;  // 取发送位图
                    app_printf("ii = %d\n",ii);
                }
            }
            if(StartIndex == 0)
            {
                app_printf("nothing to report\n");
                return ;
            }
			
        }
		//bietmap power on
        else if(EventType == e_BITMAP_EVENT_POWER_ON)    // 复电位图上报
        {
            //位图方式上报复电 ，复电是单播只报自己
            StartIndex = APP_GETTEI();            
            ReportBitMapLen = 1;

            pPowerEventData = zc_malloc_mem(sizeof(POWER_EVENT_DATA_t) + ReportBitMapLen,"PowerEventData",MEM_TIME_OUT);
            pPowerEventData->EventData[0]= 1 ;
        }
        //addr power on
        else if(EventType == e_ADDR_EVENT_POWER_ON)    // 复电地址上报
        {
            //地址方式上报复电
            StartIndex = 1;
            ReportBitMapLen = 7;

            pPowerEventData = zc_malloc_mem(sizeof(POWER_EVENT_DATA_t) + ReportBitMapLen,"PowerEventData",MEM_TIME_OUT);
            __memcpy(pPowerEventData->EventData, DevicePib_t.DevMacAddr,6);
            
            pPowerEventData->EventData[6] = 1;
        }
        else
        {

        }
		
    }
	//CJQ2 power event 
    else if (FunCode==e_CJQ2_ACT_CCO)  // 采集器停复电事件上报
    {
		U16 reportnum = 0 ;
		METER_INFO_ST *tp_power_meter_info_t = NULL;
        if(PROVINCE_ANHUI == app_ext_info.province_code) //todo: PROVINCE_ANHUI
        {
            reportnum = 1;  //使用采集器地址上报
        }
        else
        {
            
            if (EventType == e_ADDR_EVENT_POWER_OFF) // 地址停电上报
            {
                reportnum = MIN(6, CollectInfo_st.MeterNum);
                tp_power_meter_info_t = CollectInfo_st.MeterList;
            }
            else if (EventType == e_ADDR_EVENT_POWER_ON) // 地址复电上报
            {
                //reportnum = CollectInfo_st.MeterNum;
                reportnum = CollectInfo_st.power_on_meter_num;
                tp_power_meter_info_t = CollectInfo_st.power_on_meter_list;
            }
        }

        if(reportnum == 0 && EventType == e_ADDR_EVENT_POWER_ON)
		{
	        reportnum = MIN(6,CollectInfo_st.MeterNum);
			tp_power_meter_info_t = CollectInfo_st.MeterList;
		}

        ReportBitMapLen = reportnum*7;
        pPowerEventData = zc_malloc_mem(sizeof(POWER_EVENT_DATA_t) + ReportBitMapLen,"PowerEventData",MEM_TIME_OUT);
        
        if(PROVINCE_ANHUI == app_ext_info.province_code) //todo: PROVINCE_ANHUI
        {
            __memcpy(pPowerEventData->EventData, DefSetInfo.DeviceAddrInfo.DeviceBaseAddr, 6);
            pPowerEventData->EventData[6] = DevicePib_t.PowerOffFlag == e_power_off_now ? 0 : 1;
            StartIndex++;
            //        ReportBitMapLen += 7;
        }
        else
        {
		//组帧
            for(ii = 0; ii < reportnum; ii++)
        {
                __memcpy(&pPowerEventData->EventData[ii * 7], tp_power_meter_info_t[ii].MeterAddr, 6);
            if(EventType == e_ADDR_EVENT_POWER_OFF)
		    {
                    pPowerEventData->EventData[ii * 7 + 6] = tp_power_meter_info_t[ii].PowerFlag == 1 ? 0 : 1;
            }
            else if(EventType == e_ADDR_EVENT_POWER_ON)
		    {
                    pPowerEventData->EventData[ii * 7 + 6] = tp_power_meter_info_t[ii].PowerFlag ==1?0:1;//复电时，存在搜表列表的表号，即已复电
            }
			StartIndex++;
			//ReportBitMapLen += 7;
        }
        }
           
        //loop end ,initial CJQ2reportcont
        app_printf("reportnum = %d\n",reportnum);
		if(reportnum == 0)
		{
			//nothing to report
			app_printf("nothing CJQ2 to report\n");
			return ;
        }
        
	}

    pPowerEventData->EventType = EventType;
    pPowerEventData->StartIdx = StartIndex;
    /*
	Event[EventLen++] = EventType ;
	Event[EventLen++] = (U8)(StartIndex & 0xFF);
    Event[EventLen++] = (U8)(StartIndex >> 8);
	__memcpy(&Event[EventLen], ReportStaBitMapBuff, ReportBitMapLen);
	EventLen += ReportBitMapLen ;
	*/
    dump_buf((U8*)pPowerEventData , sizeof(POWER_EVENT_DATA_t) + ReportBitMapLen);
    
	power_off_event_report( FunCode, EventType , sendType, (U8*)pPowerEventData ,sizeof(POWER_EVENT_DATA_t) + ReportBitMapLen,ApsSeq);
    if(pPowerEventData)
    {
        zc_free_mem(pPowerEventData);
    }

    return;
}


U8 check_power_state(void)
{
    if(zc_timer_query(exstate_verify_timer) == TMR_RUNNING 
        || zc_timer_query(sta_power_off_report_timer) == TMR_RUNNING 
        || DevicePib_t.PowerOffFlag == e_power_off_now)
    {
        app_printf("check_power_state TRUE\n");
        return TRUE;
    }
    return FALSE;
}

static void wait_for_report_power_on(void)   // 随机延时等待启动发送复电上报
{
    if(DevicePib_t.PowerEventReported == TRUE)
    {
#if defined(ZC3750STA)
        U32 reporttimers = 0;
        reporttimers = 20*1000 + nnib.NodeLevel*100 + (rand() % (6*1000));                                                     
#elif defined(ZC3750CJQ2)
        // reporttimers = 20*1000 + (nnib.NodeLevel*100 + (rand() % (6*1000))+(30+CollectInfo_st.MeterNum*POWER_TIME_OUT)*100); 
#endif
        
        if(sta_power_on_send_timer)
        {
            if(PROVINCE_HENAN == app_ext_info.province_code)
            {
                sta_report_cnt.PowerOnSTACnt = 10;//河南复电上报次数10次
            }
            else
            {
                sta_report_cnt.PowerOnSTACnt = POWERON_REPORT_NUM;//复电上报次数
            }
        	
            ++ApsEventSendPacketSeq;
             
            if(TMR_STOPPED==zc_timer_query(sta_power_on_send_timer))
            {
                memset(sta_power_off_bitmap_buff, 0x00, PowerOffBitMapLen);
                memset(sta_power_off_report_bitmap_buff,0x00,PowerOffBitMapLen);
            }
        #if defined(ZC3750STA)
            modify_power_on_send_timer(reporttimers);
        #endif
        }
        else
            sys_panic("<powereventSendtimer fail!> %s: %d\n");
        
    }

}

void recover_power_on_report(void)
{
    if(DevicePib_t.PowerEventReported == TRUE && DevicePib_t.PowerOffFlag == e_power_on_now )
    {
        if(sta_report_cnt.PowerOnSTACnt > 0)
        {
            if(sta_report_cnt.PowerOffCnt > 0)
    	    { 
    	        sta_report_cnt.PowerOffCnt = 0;
    	    }
            if(sta_report_cnt.PowerSTATransCnt > 0)
    	    { 
    	        sta_report_cnt.PowerSTATransCnt = 0;
                g_PowerEventTranspondFlag = FALSE;
    	    }
            if(TMR_STOPPED==zc_timer_query(sta_power_on_send_timer))
            {
                U32 reporttimers = 0;
#if defined(ZC3750STA)
                reporttimers = nnib.NodeLevel*100 + (rand() % 2000);                         
#elif defined(ZC3750CJQ2)
                if(sta_report_cnt.PowerOnSTACnt == POWERON_REPORT_NUM)
                {
                    reporttimers = nnib.NodeLevel*100 + (rand() % 2000)+(60*1000); 
                }
                else
                {
                    reporttimers = nnib.NodeLevel*100 + (rand() % 2000);   
                } 
#endif
                app_printf("recover_power_on_report,%d cnt %dms\n",sta_report_cnt.PowerOnSTACnt,reporttimers); 
                modify_power_on_send_timer(reporttimers); 
            }
        }
        else
        {
            recive_power_on_cnf();
        }
    }
}

void recive_power_on_cnf(void)
{
    DevicePib_t.PowerEventReported = FALSE;
    DevicePib_t.PowerOffFlag = e_power_is_ok;
    staflag = TRUE;
#if defined(ZC3750CJQ2)

    CollectInfo_st.PowerOFFflag = DevicePib_t.PowerOffFlag;
#endif
    if(staflag == TRUE)
    {
#if defined(ZC3750STA) 
        if(WriteMeterInfo() == FLASH_OK)
#elif defined(ZC3750CJQ2)
		if(WriteCJQ2Info() == FLASH_OK)
#endif
        {
            staflag = FALSE;
            app_printf("power on success!\n");
        }
    }
    if(zc_timer_query(clean_bitmap_timer) != TMR_RUNNING)
    {
	    //timer_start(clean_bitmap_timer);
	    modify_sta_clean_bitmap_timer(STA_CLEAN_BITMAP*1000);
    }
	sta_report_cnt.PowerOnSTACnt = 0;

	app_printf("recive_power_on_cnf\n");
    if(zc_timer_query(event_report_timer) != TMR_RUNNING)
    {
        app_printf("modify_event_report_timer\n");
        modify_event_report_timer(5*TIMER_UNITS);
    }
}
static void power_on_report_timer_proc(work_t *work)
{
    app_printf("power_on_report_timer_CB  io  : 0x%08x\n",gpio_input(NULL,POWEROFF_CTRL));

    sta_report_cnt.PowerOffCnt = 0;
    if(AppNodeState == e_NODE_ON_LINE)
    {
        DevicePib_t.PowerEventReported = TRUE;
        DevicePib_t.PowerOffFlag = e_power_on_now;
        staflag = TRUE;
        app_printf("power on !\n");
#if defined(ZC3750CJQ2)
		//启动验表
		if(CollectInfo_st.MeterNum > 0)
		{
			poweroncheckmeterflag = TRUE;
			poweroffverify();
		}
#endif
        wait_for_report_power_on();
        timer_stop(sta_power_on_report_timer,TMR_NULL);
    }
    else 
    {
        timer_start(sta_power_on_report_timer);   // 等待模块上线
        app_printf("wait for online\n");
    }


}

#if defined(ZC3750CJQ2)
U8 send_buff[50]={0};
U16 send_buff_len =0;
U8  power_off_num =0;
U8  power_off_flag = FALSE ;//FALSE :电表状态获取完成 OR TRUE:电表带电状态正在获取
U8  power_off_seq=0xff;
U8 collect_meter_index = 0;
U8 collect_meter_max = 0;
U8 gl_power_check_meter_fg = FALSE;

void power_off_verify_read_timeout(void *pMsgNode);
void power_off_verify_read_recive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);

void power_off_verify_to_sreial(U8 protocol,U16 timeout, U8 frameSeq, U16 frameLen, U8 *frame, U8 lastFlag, U8 Baud)
{
	add_serialcache_msg_t serialmsg;
	memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));
	app_printf("lastFlag = %d\n", lastFlag);
	
	serialmsg.StaReadMeterInfo.LastFlag = lastFlag;
	serialmsg.baud = baud_enum_to_int(Baud);			//枚举转实际数值
	serialmsg.Uartcfg = meter_serial_cfg;
	serialmsg.EntryFail = NULL;
	serialmsg.SendEvent = NULL;
	serialmsg.ReSendEvent = NULL;
	serialmsg.MsgErr = NULL;
	
	serialmsg.Match  = NULL;
	serialmsg.Timeout = power_off_verify_read_timeout;
	serialmsg.RevDel = power_off_verify_read_recive;

	serialmsg.Protocoltype = protocol;
	serialmsg.DeviceTimeout = timeout;
	serialmsg.IntervalTime = 10;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = frameLen;
	serialmsg.FrameSeq = frameSeq;
	serialmsg.lid =e_Serial_PowerEvent;
	serialmsg.ReTransmitCnt = 0;
	serialmsg.pSerialheader = &SERIAL_CACHE_LIST;
	
	extern ostimer_t *serial_cache_timer;
	serialmsg.CRT_timer = serial_cache_timer;
	
	serial_cache_add_list(serialmsg, frame,TRUE);
	
	return;
}

void packet_check_meter_to_serial()
{
	U8 LastFlag = MIDDLE_ACTIVE;
	U8 Baud = 0;
	U8 Protocol = 0;
	METER_INFO_ST tp_check_meter_t;

	if(collect_meter_index >= collect_meter_max)
	{
		return;
	}

	if(collect_meter_index == collect_meter_max - 1)
	{
		LastFlag = LAST_ACTIVE;
	}

	app_printf("index:%d max:%d\n",collect_meter_index,collect_meter_max);

	if(gl_power_check_meter_fg == FALSE)
	{
		__memcpy(&tp_check_meter_t, &CollectInfo_st.MeterList[collect_meter_index], sizeof(METER_INFO_ST));
	}
	else
	{
		__memcpy(&tp_check_meter_t, &CollectInfo_st.power_on_meter_list[collect_meter_index], sizeof(METER_INFO_ST));
	}
    
	Baud = tp_check_meter_t.BaudRate;
	Protocol = tp_check_meter_t.Protocol;

	if(Protocol == DLT698)
	{
		Dlt698Encode(tp_check_meter_t.MeterAddr, Protocol,send_buff,&send_buff_len);
		power_off_verify_to_sreial(e_DLT698_MSG,POWER_TIME_OUT*100,++power_off_seq,send_buff_len,send_buff,LastFlag,Baud);
	}
	else
	{
		Dlt645Encode(tp_check_meter_t.MeterAddr, Protocol,send_buff,(U8 *)&send_buff_len);
		power_off_verify_to_sreial(e_DLT645_MSG,POWER_TIME_OUT*100,++power_off_seq,send_buff_len,send_buff,LastFlag,Baud);
	}

	send_buff_len = 0;
	collect_meter_index++;

    
    cjq2_power_off_on_flow_protec_timer_modify(5*TIMER_UNITS); //若太长，当停电时间较短时，有可能报不上去
}

void power_off_verify_read_timeout(void *pMsgNode)
{
	U16  delayTime;
	add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;
	if(gl_power_check_meter_fg == FALSE)
	{
		CollectInfo_st.MeterList[power_off_num++].PowerFlag=1;
	}
	else
	{
		CollectInfo_st.power_on_meter_list[power_off_num++].PowerFlag=1;
	}

    if(TMR_RUNNING == zc_timer_query(cjq2_power_off_on_flow_protec_timer))
    {
        timer_stop(cjq2_power_off_on_flow_protec_timer, TMR_NULL);
    }

	if(pSerialMsg->StaReadMeterInfo.LastFlag == LAST_ACTIVE )
	{
		power_off_flag = FALSE ;
		if(e_power_off_now == DevicePib_t.PowerOffFlag) //停电
        {
           if(timer_remain(sta_power_off_send_timer) > 2*TIMER_UNITS || TMR_STOPPED == zc_timer_query(sta_power_off_send_timer))
            {
                delayTime = (100 + (rand() % 200))*10;
                modify_power_off_send_timer(delayTime);
            }
        }
        else    //复电
        {
            delayTime = 2*TIMER_UNITS + (nnib.NodeLevel*100 + (rand() % (6*TIMER_UNITS)));
            modify_power_on_send_timer(delayTime);
        }
	}
    else
    {
        timer_start(cjq2_power_off_on_flow_protec_timer);
    }
	
	packet_check_meter_to_serial();
}

void power_off_verify_read_recive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
	//CollectInfo_st.MeterList[power_off_num++].PowerFlag=0;
	U16  delayTime;
	add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;
	if(gl_power_check_meter_fg == FALSE)
	{
		CollectInfo_st.MeterList[power_off_num++].PowerFlag=0;
	}
	else
	{
		CollectInfo_st.power_on_meter_list[power_off_num++].PowerFlag=0;
	}
    
    if(TMR_RUNNING == zc_timer_query(cjq2_power_off_on_flow_protec_timer))
    {
        timer_stop(cjq2_power_off_on_flow_protec_timer, TMR_NULL);
    }

	if(pSerialMsg->StaReadMeterInfo.LastFlag == LAST_ACTIVE )
	{
		power_off_flag = FALSE ;
		if(e_power_off_now == DevicePib_t.PowerOffFlag) //停电
        {
           if(timer_remain(sta_power_off_send_timer) > 2*TIMER_UNITS || TMR_STOPPED == zc_timer_query(sta_power_off_send_timer))
            {
                delayTime = (100 + (rand() % 200))*10;
                modify_power_off_send_timer(delayTime);
            }
        }
        else    //复电
        {
            delayTime = 2*TIMER_UNITS + (nnib.NodeLevel*100 + (rand() % (6*TIMER_UNITS)));
            modify_power_on_send_timer(delayTime);
        }
	}
    else
    {
        timer_start(cjq2_power_off_on_flow_protec_timer);
    }
	packet_check_meter_to_serial();
}

void cjq2_power_off_on_flow_protec_timercb(struct ostimer_s *ostimer, void *arg)
{
    static U8 run_num = 0;

    if(0 == run_num)
    {
        run_num++;
        collect_meter_index--;  //丢失后继续发当前丢失的地址
        packet_check_meter_to_serial();

        if(TMR_STOPPED == zc_timer_query(cjq2_power_off_on_flow_protec_timer))
        {
            timer_start(cjq2_power_off_on_flow_protec_timer);
        }
    }
    else
    {
        run_num = 0;

        if(e_power_off_now == DevicePib_t.PowerOffFlag) //停电
        {
            power_off_send_timer_proc(NULL);    //若发消息失败，才进此流程，所以这里不能再发消息，直接进入事件打包上报
        }
        else    //复电
        {
            power_on_send_timer_proc(NULL);
        }

        if(TMR_RUNNING == zc_timer_query(cjq2_power_off_on_flow_protec_timer))
        {
            timer_stop(cjq2_power_off_on_flow_protec_timer, TMR_NULL);
        }
    }
}

void cjq2_power_off_on_flow_protec_timer_modify(uint32_t Ms)
{
	if(NULL == cjq2_power_off_on_flow_protec_timer)
	{
        cjq2_power_off_on_flow_protec_timer = timer_create(Ms,
								  0,
								  TMR_TRIGGER,
								  cjq2_power_off_on_flow_protec_timercb,
								  NULL,
								  "cjq2_power_off_on_flow_protec_timercb");
    }
    else
    {
		timer_modify(cjq2_power_off_on_flow_protec_timer,
                Ms,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				cjq2_power_off_on_flow_protec_timercb,
				NULL,
                "cjq2_power_off_on_flow_protec_timercb",
                TRUE);
	}
	
	timer_start(cjq2_power_off_on_flow_protec_timer);
}

void poweroffverify()
{
	collect_meter_max = ((poweroncheckmeterflag == TRUE)?CollectInfo_st.power_on_meter_num :(MIN(6,CollectInfo_st.MeterNum)));
	gl_power_check_meter_fg = poweroncheckmeterflag;
	send_buff_len = 0;
	power_off_num = 0;
	power_off_flag = TRUE ;
	poweroncheckmeterflag = FALSE;
	cjq2_search_suspend_timer_modify(60 * TIMER_UNITS);
	collect_meter_index = 0;
	packet_check_meter_to_serial();
}














#endif

//返回TRUE可以转发，返回FALSE不进行转发
 U8 CheckPowerOffInfo(U16 Seq,U8 *addr)
{
    U8 i;
    U8 nulladdr[6]={0};

    if(memcmp(addr,nulladdr,MACADRDR_LENGTH) == 0)
    {
        app_printf(" nulladdr no trans \n");
        return FALSE;
    }

    app_printf("CheckPowerOff Seq:%d addr:",Seq);
    dump_buf(addr,6);

    for(i = 0; i < MAX_POWEROFFNUM; i++)
    {
        if(memcmp(addr, poweroffcheck_info[i].MeterAddr, MACADRDR_LENGTH) != 0)
        {
            continue;
        }

        //如果接收到确认帧则不进行上报，或者达到最大重传次数,或者发送标志位禁止
        if(poweroffcheck_info[i].CfmFlag == TRUE || 
            poweroffcheck_info[i].Times >= MAX_POWEROFF_RETURN ||
            poweroffcheck_info[i].SendFlag == FALSE)
        {
            return FALSE;
        }
        
        //未收到确认，有剩余转发次数，允许发送
        poweroffcheck_info[i].SendFlag = FALSE;
        poweroffcheck_info[i].Times++;
        return TRUE;
	}

    //新节点进行添加
    add_power_off_info(Seq,addr);//针对停电信息列表里没有的直接添加，可以直接转发
    return TRUE;
}

/*
停电信息列表更新，每1S进一次，判断当前列表中没有收到确认帧的节点，在40S内执行够3次。
当时间为0时代表此次转发结束，清空记录
*/
void updatePowerOffSeq(work_t *work)
{
    U8 i;
    U8 vaildflag = FALSE;
    U8 nulladdr[6]={0};
	
	for(i = 0; i < MAX_POWEROFFNUM; i++)
	{
		if(memcmp(nulladdr,poweroffcheck_info[i].MeterAddr,MACADRDR_LENGTH) == 0)
		{
			continue;
		}
		
		if(poweroffcheck_info[i].LifeTime > 0)
		{
			poweroffcheck_info[i].LifeTime--;
            vaildflag = TRUE;
		}
		else
		{
            memset((U8*)&poweroffcheck_info[i],0x00,sizeof(POWEROFFCHECK_INFO_t));
		}

        if(poweroffcheck_info[i].LifeTime != 0 && (poweroffcheck_info[i].LifeTime/POWER_OFF_INTER) == 0)
        {
            poweroffcheck_info[i].SendFlag = TRUE;
        }
	}

    if(vaildflag == TRUE)
    {
         if(UpdatePowerOffInfoTimer == NULL ||  zc_timer_query(UpdatePowerOffInfoTimer)==TMR_STOPPED )
        {
            modify_UpdatePowerOffInfoTimer(100);
        }
    }
    return;
}


void UpdatePowerOffInfoTimerCB( struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"PowerOffInfo",MEM_TIME_OUT);
    work->work = updatePowerOffSeq;
    work->msgtype = POWEROFF_ADDR_INFO;
    post_app_work(work);  
}

static void modify_UpdatePowerOffInfoTimer(uint32_t first)
{
    if(UpdatePowerOffInfoTimer == NULL)
    {
        UpdatePowerOffInfoTimer = timer_create(first,
                                first,
                                TMR_TRIGGER,//TMR_TRIGGER
                                UpdatePowerOffInfoTimerCB,
                                NULL,
                                "UpdatePowerOffInfoTimerCB"
                               );
    }
    else
    {
        timer_modify(UpdatePowerOffInfoTimer,
                first,
                first,
                TMR_TRIGGER ,//TMR_TRIGGER
                UpdatePowerOffInfoTimerCB,
                NULL,
                "UpdatePowerOffInfoTimerCB",
                TRUE);
    }

    timer_start(UpdatePowerOffInfoTimer);
	
}

static U8 add_power_off_info(U16 Seq,U8 *addr)
{
    app_printf("AddPowerOffInfo\n");
    U8 i;
    U8 nulladdr[6]={0};
    U8 nullseq = 0;
	
    for(i=0;i<MAX_POWEROFFNUM;i++)
    {
        if(memcmp(nulladdr,poweroffcheck_info[i].MeterAddr,MACADRDR_LENGTH) == 0)
        {
            nullseq = i;
            break;
        }
    }

    poweroffcheck_info[nullseq].ApsSeq = Seq;
    poweroffcheck_info[nullseq].LifeTime   = MAX_POWEROFFTIME;//给时间为59S,让少执行一次
    __memcpy(poweroffcheck_info[nullseq].MeterAddr,addr,MACADRDR_LENGTH);
    poweroffcheck_info[nullseq].SendFlag = FALSE;
    poweroffcheck_info[nullseq].CfmFlag = FALSE;
    poweroffcheck_info[nullseq].Times = 0;
    
    if(UpdatePowerOffInfoTimer == NULL ||  zc_timer_query(UpdatePowerOffInfoTimer)==TMR_STOPPED)
    {
        modify_UpdatePowerOffInfoTimer(100);
    }
    return nullseq;
}

static void power_off_report_timer_proc(work_t *work)
{
	if(FactoryTestFlag == 1)
    {
        return;
    }
	
    if(module_state.power_type != e_with_power)
	{
		app_printf("!= e_with_power\n");
		return;
	}
	app_printf("------------------------------------------------power off !\n");
    
    if(module_state.plug_state != e_plug_in||module_state.zero_cross_state == e_zc_get||nnib.NodeState != e_NODE_ON_LINE)
	{
        app_printf("plug_state is %d,zero_cross_state is %d NodeState = %d\n",module_state.plug_state,module_state.zero_cross_state,nnib.NodeState);
		return;
	}
    DevicePib_t.PowerOffFlag = e_power_off_now;         // 停电状态合法
    //staflag = TRUE;
    sta_report_cnt.PowerOffCnt = POWEROFF_REPORT_NUM;
#if defined(ZC3750STA) 
    U32 reporttime = 0;
    WriteMeterInfo();

    reporttime = (100+rand() % 200);
#elif defined(ZC3750CJQ2)
    //U8 Cjq2Num =0;

    //Cjq2Num= MIN(6,CollectInfo_st.MeterNum);
    //reporttime = (100 + (rand() % 200) + Cjq2Num * TIME_OUT * 100) * 10;
    //reporttime = ((100 + (rand() % 200) + Cjq2Num * POWER_TIME_OUT * 100 + 200) * 10);//POWER_TIME_OUT*100 后边加的这个200 是给串口超时留够的时间

    //RSPTIME = 100;
    //app_printf("Cjq2Num = %d,reporttime = %d \n",Cjq2Num,reporttime);
    CollectInfo_st.PowerOFFflag = DevicePib_t.PowerOffFlag;
	CollectInfo_st.power_on_meter_num = CollectInfo_st.MeterNum;
	__memcpy(CollectInfo_st.power_on_meter_list,CollectInfo_st.MeterList,sizeof(METER_INFO_ST) * METERNUM_MAX);
    WriteCJQ2Info();
#endif

    DevicePib_t.PowerEventReported = FALSE;

    if(sta_power_off_send_timer)
    {
        if(TMR_STOPPED==zc_timer_query(sta_power_off_send_timer))
    	{
            if(g_PowerEventTranspondFlag != TRUE)
            {
                memset(sta_power_off_bitmap_buff, 0x00, PowerOffBitMapLen);  //未转发位图，直接清除上报位图
                memset(sta_power_off_report_bitmap_buff,0x00,PowerOffBitMapLen);
            }
        }
        //	    		
    		 
        //if( poweroffverifyFlag != 1)     //
    	{

            // 停电上报时增加功率
           
            extern void SetPowerLevel(S16 dig,S16 ana);
            SetPowerLevel(0,8);
		
#if defined(ZC3750STA)
            //poweroffverifyFlag = 1;
            modify_power_off_send_timer(reporttime);
#elif defined(ZC3750CJQ2)
    			
            //poweroffverifyFlag = 0;
            //g_rdflag = 1;
#endif
            //timer_start( poweroffverifytimer); //USE 645 to verify CJQ2
				
#if defined(ZC3750CJQ2)
            poweroffverify();
#endif
            app_printf("timer_start------------------------------------powereventSendtimer !\n");

            timer_stop(sta_power_off_report_timer,TMR_NULL);
        }
    }
    else
        sys_panic("<powereventSendtimer fail!> %s: %d\n");
				 

}

static void power_on_send_timer_proc(work_t *work)   // 停复电事件发送定时器回调函数
{
    U32 delaytime = 5*1000;

    app_printf(" power_on_send_timer_CB report!,PowerOffFlag=%d,PowerEventReported=%d\n",DevicePib_t.PowerOffFlag,DevicePib_t.PowerEventReported);

    //如果复电上报次数达到，也可清理，注意联合判定条件的时序，需要确保PowerOffFlag 被置上e_power_on_now的同时
    //PowerOnSTACnt被填上次数
    if(sta_report_cnt.PowerOnSTACnt > 0)
    {
        //power on report
    #ifdef STA_POWER_ON
        if(DevicePib_t.PowerOffFlag == e_power_on_now) // 复电事件合法
        {
            app_printf("RepowerOn report\n");
        #if defined(ROLAND1_1M)&&defined(ZC3750STA)
            if(GetnnibDevicetype() == e_3PMETER)
            {      
                if(READ_EN_A_PIN == 0||READ_EN_B_PIN == 0||READ_EN_C_PIN == 0)
                {
                    app_printf("PHY_PHASE_ALL1\n");
                    ld_set_3p_meter_phase_zc(PHY_PHASE_ALL);
                }
            }
            else
            {
                
            }
        #endif
            aps_event_seq.PowerOnEventSeq = ++ApsEventSendPacketSeq;
            
    #if defined(ZC3750STA)
            power_event_send_cal(e_STA_ACT_CCO,e_ADDR_EVENT_POWER_ON,e_UNICAST_FREAM, aps_event_seq.PowerOnEventSeq);
    #elif defined(ZC3750CJQ2)
            //g_plcflag = 1;//采集器搜表挂起
            power_event_send_cal(e_CJQ2_ACT_CCO,e_ADDR_EVENT_POWER_ON,e_UNICAST_FREAM, aps_event_seq.PowerOnEventSeq);
    #endif
            delaytime = 3*1000; 
        }
#endif
        modify_power_on_send_timer(delaytime);
        sta_report_cnt.PowerOnSTACnt--; 
        if(sta_report_cnt.PowerOnSTACnt == 0) 
        {
            DevicePib_t.PowerEventReported = FALSE;
            DevicePib_t.PowerOffFlag = e_power_is_ok;
            staflag = TRUE;
#if defined(ZC3750CJQ2)
            CollectInfo_st.PowerOFFflag = DevicePib_t.PowerOffFlag;
#endif
            app_printf("power on timeout!\n");
        }
    }
    else 
    {
        
    }
    if(sta_report_cnt.PowerOnSTACnt == 0)
    {
        timer_stop( sta_power_on_send_timer , TMR_NULL);
    }
    app_printf("StaReport.PowerOnSTACnt %d\n",sta_report_cnt.PowerOnSTACnt);
}

static void power_off_send_timer_proc(work_t *work)   // 停电事件发送定时器回调函数
{
    U32 delaytime = 5*1000;

    app_printf(" power_off_send_timer_CB report!,PowerOffFlag=%d,PowerEventReported=%d\n",DevicePib_t.PowerOffFlag,DevicePib_t.PowerEventReported);

    //power off
    if(DevicePib_t.PowerOffFlag == e_power_off_now)  // 停电状态合法
    {
        if(sta_report_cnt.PowerOffCnt > 0)
		{   
    #if defined(ROLAND1_1M)&&defined(ZC3750STA)
		    if(GetnnibDevicetype() == e_3PMETER&&(nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC)>0)
            {      
                U8 phase;
                if(sta_report_cnt.PowerOffCnt%(nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC) == 0)
                {
                    if(nnib.PhasebitA)
                        phase = PHY_PHASE_A;
                    else if(nnib.PhasebitC)
                        phase = PHY_PHASE_C;
                    else if(nnib.PhasebitB)
                        phase = PHY_PHASE_B;
                }
                else if(sta_report_cnt.PowerOffCnt%(nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC) == 1)
                {
                    if(nnib.PhasebitB)
                        phase = PHY_PHASE_B;
                    else if(nnib.PhasebitC)
                        phase = PHY_PHASE_C;
                } 
                else if(sta_report_cnt.PowerOffCnt%(nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC) == 2)
                {
                    phase = PHY_PHASE_C;
                }
                app_printf("phase %d\n",phase);
                ld_set_3p_meter_phase_zc(phase);
            }
            else
            {
                
            }
   #endif
#if defined(ZC3750STA)
    		//汇聚自己位图
            app_printf("My TEI is %d\n", APP_GETTEI());
		    bitmap_set_bit(sta_power_off_report_bitmap_buff, APP_GETTEI());
            app_printf("PowerOffBitMapLen = %d!\n",PowerOffBitMapLen);
   
            power_event_send_cal(e_STA_ACT_CCO,e_BITMAP_EVENT_POWER_OFF,e_LOCAL_BROADCAST_FREAM_NOACK, ++ApsEventSendPacketSeq);
#elif defined(ZC3750CJQ2)
    		//g_plcflag = 1;//采集器搜表挂起
            power_event_send_cal(e_CJQ2_ACT_CCO,e_ADDR_EVENT_POWER_OFF,e_LOCAL_BROADCAST_FREAM_NOACK, ++ApsEventSendPacketSeq);
#endif
            
            app_printf("power off e_LOCAL_BROADCAST_FREAM_NOACK report!\n");
    		
    		delaytime = 2*1000;// 上报间隔为2秒
            
    	    app_printf("power off report!\n");
            if(staflag == TRUE)
            {
                mutex_post(&mutexSch_t);  // 停电状态保存flash
            }
			modify_power_off_send_timer(delaytime);
            sta_report_cnt.PowerOffCnt--;	
		}
        else 
		{
			timer_stop( sta_power_off_send_timer , TMR_NULL);
		}
 
		 app_printf("StaReport.PowerOffCnt = %d!\n",sta_report_cnt.PowerOffCnt);

         
    	++ApsEventSendPacketSeq ;


		
		delaytime = 2*1000;  // 上报间隔为2秒

	    app_printf("power off report!\n");
        
		if(staflag == TRUE)
        {
            mutex_post(&mutexSch_t);  // 停电状态保存flash
        }
        modify_power_off_send_timer(delaytime);
    }
    else
    { 
		if(sta_report_cnt.PowerSTATransCnt > 0)
		{
		    //it's not power off ,need to transpond
    		if(g_PowerEventTranspondFlag == TRUE)   // 未停电但需要发送
    		{
    			power_event_send_cal(e_STA_ACT_CCO,e_BITMAP_EVENT_POWER_OFF,e_UNICAST_FREAM, aps_event_seq.PowerSTATransEventSeq);  // 未停电节点单播上报；
    			app_printf("power off e_UNICAST_FREAM report!\n");
    			delaytime = 5*1000;
    		}
			modify_power_off_send_timer(delaytime);
            sta_report_cnt.PowerSTATransCnt--;	
            if(sta_report_cnt.PowerSTATransCnt == 0)
            {
                g_PowerEventTranspondFlag = FALSE;
                app_printf("Power Event Trans end!\n");
            }
		}
        else
        {
            
        }
        
        if(sta_report_cnt.PowerSTATransCnt == 0)
        {
            timer_stop( sta_power_off_send_timer , TMR_NULL);
        }
        app_printf("StaReport.PowerSTATransCnt = %d\n",sta_report_cnt.PowerSTATransCnt);
	}
}

static void power_off_send_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"PowerOffSendTimerProc",MEM_TIME_OUT);
	work->work = power_off_send_timer_proc;
	work->msgtype = POWEROFF_SEND;
	post_app_work(work);
}

static void power_on_send_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"PowerOnSendTimerProc",MEM_TIME_OUT);
	work->work = power_on_send_timer_proc;
	work->msgtype = POWERON_SEND;
	post_app_work(work);
}

static void power_on_report_timer_CB(struct ostimer_s * ostimer, void * arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"power_on_report_timer_proc",MEM_TIME_OUT);
	work->work = power_on_report_timer_proc;
	work->msgtype = POWERON_REPORT;
	post_app_work(work);

}



static void power_off_report_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"power_off_report_timer_proc",MEM_TIME_OUT);
	work->work = power_off_report_timer_proc;
	work->msgtype = POWEROFF_REPORT;
	post_app_work(work);

}




int8_t  powerevent_send_timer_init(void)
{
    if(sta_power_on_send_timer == NULL)
    {
        sta_power_on_send_timer = timer_create(30,
                                        30,
                                        TMR_TRIGGER ,//TMR_TRIGGER
                                        power_on_send_timer_CB,
                                        NULL,
                                        "power_on_send_timer_CB"
                                        );
    }

	if(sta_power_off_send_timer == NULL)
    {
        sta_power_off_send_timer = timer_create(30,
                                        30,
                                        TMR_TRIGGER ,//TMR_TRIGGER
                                        power_off_send_timer_CB,
                                        NULL,
                                        "power_off_send_timer_CB"
                                        );
    }

    return 0;


}

int8_t  poweron_report_timer_init(void)
{
    if(sta_power_on_report_timer == NULL)
    {
        sta_power_on_report_timer = timer_create(6000,
                                        1000,
                                        TMR_PERIODIC ,//TMR_TRIGGER
                                        power_on_report_timer_CB,
                                        NULL,
                                        "power_on_report_timer_CB"
                                        );
    }

    return 0;

}





int8_t  poweroff_report_timer_init(void)
{
    if(sta_power_off_report_timer == NULL)
    {
        sta_power_off_report_timer = timer_create(5*1000,
                                        0,
                                        TMR_TRIGGER ,//TMR_TRIGGER
                                        power_off_report_timer_CB,
                                        NULL,
                                        "power_off_report_timer_CB"
                                        );
    }

    return 0;
}


void modify_power_off_send_timer(U32 MStimes)
{
    timer_modify(sta_power_off_send_timer,
			   MStimes, 
               0,
               TMR_TRIGGER ,
               power_off_send_timer_CB,
               NULL,
               "power_off_send_timer_CB",
               TRUE);
	timer_start(sta_power_off_send_timer);
}

void modify_power_on_send_timer(U32 MStimes)
{
    timer_modify(sta_power_on_send_timer,
			   MStimes, 
               0,
               TMR_TRIGGER ,
               power_on_send_timer_CB,
               NULL,
               "power_on_send_timer_CB",
               TRUE);
	timer_start(sta_power_on_send_timer);
}



void UpdatePowerOffInfoTimerCB( struct ostimer_s *ostimer, void *arg);



U16 PowerOffMeterNum = 0;


void SetPowerOffInfoFlag(U16 Seq)
{
    U8 i;
    for(i=0;i<MAX_POWEROFFNUM;i++)
    {
        if(Seq == poweroffcheck_info[i].ApsSeq)
		{
			app_printf("CfmFlag is true:%d",Seq);
			poweroffcheck_info[i].CfmFlag = TRUE;//收到CCO的确认帧后，置TRUE，后续不再转发
			break;
		}
	}  
    return;
    
}



U8 check_power_off_info(U16 Seq,U8 *addr)
{
    U8 i;
    U8 nulladdr[6]={0};

    if(memcmp(addr,nulladdr,MACADRDR_LENGTH) == 0)
    {
        app_printf(" nulladdr no trans \n");
        return FALSE;
    }

    app_printf("CheckPowerOff Seq:%d addr:",Seq);
    dump_buf(addr,6);

    for(i = 0; i < MAX_POWEROFFNUM; i++)
    {
        if(memcmp(addr, poweroffcheck_info[i].MeterAddr, MACADRDR_LENGTH) != 0)
        {
            continue;
        }

        //如果接收到确认帧则不进行上报，或者达到最大重传次数,或者发送标志位禁止
        if(poweroffcheck_info[i].CfmFlag == TRUE || 
            poweroffcheck_info[i].Times >= MAX_POWEROFF_RETURN ||
            poweroffcheck_info[i].SendFlag == FALSE)
        {
            return FALSE;
        }
        
        //未收到确认，有剩余转发次数，允许发送
        poweroffcheck_info[i].SendFlag = FALSE;
        poweroffcheck_info[i].Times++;
        return TRUE;
	}

    //新节点进行添加
    add_power_off_info(Seq,addr);//针对停电信息列表里没有的直接添加，可以直接转发
    return TRUE;
}




#endif


