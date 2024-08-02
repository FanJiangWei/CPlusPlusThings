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
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
#include "ZCsystem.h"
#include "app_read_router_cco.h"
#include "app_read_jzq_cco.h"
#include "datalinkdebug.h"
#include "app_common.h"
#include "app_gw3762.h"
#include "app_dltpro.h"
#include "SchTask.h"
#include "app_event_report_cco.h"


#if defined(STATIC_MASTER)


ostimer_t *route_rd_timer = NULL;

ROUTER_ACTIVE_INFO_t RouterActiveInfo_t =
{
    .State = EVT_ROUTER_IDLE,
    .CrnRMRound	=  0,
    .CrnMeterIndex =  0,
    .Timeout = 0,
    .IdleFlag = 0,
    .Res = 0,
};

extern U8  DataAddrFilterStatus;

static void route_read_modify_timer(uint32_t TimeMs);
static void router_read_afn_14f1_read_meter_timeout(void* pTaskPrmt);
static void router_read_router_act_read_meter_proc(void* pTaskPrmt, void* pUplinkData);

/*************************************************************************
 * 函数名称	: 	router_read_meter
 * 函数说明	: 	cco数据抄读并发请求
 * 参数说明	:
 *              U8 localFrameSeq    - 本地报文序号
                U8 *pMeterAddr      - 表地址
                U8 *pCnmAddr        - 网络地址
                U8 protoType        - 协议类型
                U16 frameLen        - 抄读帧长度
                U8 *pFrameUnit      - 抄读帧地址
                MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/


static int32_t router_read_afn14f1_entrylist_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    app_printf("list->nr = %d, list->thr = %d\n",list->nr, list->thr);       
    if(list->nr >= list->thr)
    {
        return -1;
    }
    return 0;
}


void router_read_meter(U8 localFrameSeq, U8 *pMeterAddr, U8 *pCnmAddr, U8 protoType, U16 frameLen, U8 *pFrameUnit, MESG_PORT_e port)
{

    multi_trans_t Afn14F1Readmeter;

    memset((void*)&Afn14F1Readmeter, 0x00, sizeof(multi_trans_t));
	
	Afn14F1Readmeter.lid = READ_METER_LID;
	Afn14F1Readmeter.SrcTEI = 0;
    Afn14F1Readmeter.DeviceTimeout = DEVTIMEOUT;
    Afn14F1Readmeter.Framenum = localFrameSeq;   //pGw3762Data_t->DownInfoField.FrameNum;
    __memcpy(Afn14F1Readmeter.Addr, pMeterAddr, MAC_ADDR_LEN);	
    __memcpy(Afn14F1Readmeter.CnmAddr, pCnmAddr, MAC_ADDR_LEN);

    Afn14F1Readmeter.MsgPort = port;

	Afn14F1Readmeter.State = UNEXECUTED;
	Afn14F1Readmeter.SendType = AFN14F1_READMETER;
    Afn14F1Readmeter.DatagramSeq = ++ApsSendPacketSeq;
	Afn14F1Readmeter.DealySecond = F14F1_TRANS_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    Afn14F1Readmeter.ReTransmitCnt = RETRANMITMAXNUM;
	
    Afn14F1Readmeter.DltNum = 1;
	Afn14F1Readmeter.ProtoType = protoType;
	Afn14F1Readmeter.FrameLen  = frameLen;
    
    Afn14F1Readmeter.EntryListCheck = router_read_afn14f1_entrylist_check;
	Afn14F1Readmeter.EntryListfail = NULL;  
	Afn14F1Readmeter.TaskPro = jzq_read_app_read_meter_req; 
    Afn14F1Readmeter.TaskUpPro = router_read_router_act_read_meter_proc;
	Afn14F1Readmeter.TimeoutPro = router_read_afn_14f1_read_meter_timeout;

    Afn14F1Readmeter.pMultiTransHeader = &F14F1_TRANS_LIST;
	Afn14F1Readmeter.CRT_timer = f14f1_trans_timer;
    multi_trans_put_list(Afn14F1Readmeter, pFrameUnit);

    //osh_event_send(&FlowEvent, EVT_APS_IND, 100);

    return;
}

static U8 router_read_state_req(void)
{
    U8  DefaultAddr[6] = {0};
    U16 ii = 0;
    U8  FinishFlag = FALSE;
	
    for(ii = RouterActiveInfo_t.CrnMeterIndex; ii < MAX_WHITELIST_NUM; ii++)
    {
        //record_addr_num++;
        if(e_FINISH == WhiteMacAddrList[ii].Result)
        {
            continue;
        }
        if(0 == memcmp(WhiteMacAddrList[ii].MacAddr, DefaultAddr, 6))
        {
            //record_addr_num--;
            continue;
        }              
        else
        {
            if(0 == memcmp(WhiteMacAddrList[ii].CnmAddr, DefaultAddr, 6) && nnib.Networkflag == FALSE)
			{
                FinishFlag = TRUE; 
                continue;
			}
		}
              
        if(e_UNFINISH == WhiteMacAddrList[ii].Result && 
			(0 != memcmp(WhiteMacAddrList[ii].MacAddr, DefaultAddr, 6)))
        {
            RouterActiveInfo_t.CrnMeterIndex = ii;
            FinishFlag = TRUE;
            break;
        }
    }
	
    if(ii >= MAX_WHITELIST_NUM)
    {
        if(FALSE == FinishFlag) 
        {
            for(ii = 0; ii < RouterActiveInfo_t.CrnMeterIndex; ii++)
		    {
		        if(e_FINISH == WhiteMacAddrList[ii].Result)
		        {
		            continue;
		        }
				if(0 == memcmp(WhiteMacAddrList[ii].MacAddr, DefaultAddr, 6))
				{
					continue;
				}
				if(0 == memcmp(WhiteMacAddrList[ii].CnmAddr, DefaultAddr, 6))
				{
		            FinishFlag = TRUE; 
		            break;
				}
		        if(e_UNFINISH == WhiteMacAddrList[ii].Result && 
					(0 != memcmp(WhiteMacAddrList[ii].MacAddr, DefaultAddr, 6)))
		        {
		            FinishFlag = TRUE; 
		            break;
		        }
		   	 }
		}
        else
        {
        }

		RouterActiveInfo_t.CrnMeterIndex=0;
        RouterActiveInfo_t.CrnRMRound++;
        if(RouterActiveInfo_t.CrnRMRound >= MAX_ROUND)
        {
        	RouterActiveInfo_t.CrnRMRound = MAX_ROUND;
			app_printf("MAX_ROUND : %d! FinishFlag : %s\n",MAX_ROUND,0==FinishFlag?"finish":"unfinish");
            return (0==FinishFlag) ? MAX_ROUND_FINISH : MAX_ROUND_UNFINISH;
        }
		else
		{
			app_printf("ROUND : %d! FinishFlag : %s\n",RouterActiveInfo_t.CrnRMRound,0==FinishFlag?"finish":"unfinish");
			return (0==FinishFlag) ? FREE_ROUND_FINISH : FREE_ROUND_UNFINISH;
		}
    }

    return READING_STATUS;
}

/*************************************************************************
 * 函数名称	: 	router_read_router_act_read_meter_proc
 * 函数说明	: 	路由主动载波抄表上行数据处理
 * 参数说明	: 	void *pTaskPrmt         - 任务参数
 *                void *pUplinkData       - 抄读数据响应原语
 * 返回值	: 	无
 *************************************************************************/
void router_read_router_act_read_meter_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    READ_METER_IND_t  *pReadMeterInd = pUplinkData;

    U8  ScrMeterAddr[MAC_ADDR_LEN]={0,0,0,0,0,0};
       
    if(TRUE==Check645Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength, NULL, ScrMeterAddr,NULL))
    {
        app_printf("645 frame \n");
    }
	else if(TRUE==Check698Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength,NULL, ScrMeterAddr, NULL))
	{
        app_printf("698 frame \n");
    }
    else
    {
        app_printf("other frame \n");
    }
	
    app_printf("RtActRM  Addr : ");
    dump_buf(pReadTask->Addr,6);
	app_printf("pReadTask->CnmAddr : ");
	dump_buf(pReadTask->CnmAddr,6);

    if(RouterActiveInfo_t.State == EVT_ROUTER_IDLE )
    {
        app_printf("wait for next req\n");
        return;
    }
    //地址过滤打开时，只判定地址过滤；关闭时，使用报文序号过滤
    if((DataAddrFilterStatus == TRUE&&(memcmp(pReadMeterInd->SrcMacAddr, pReadTask->CnmAddr, MAC_ADDR_LEN) == 0 ||
       	memcmp(ScrMeterAddr, pReadTask->Addr, MAC_ADDR_LEN) == 0))
        ||(DataAddrFilterStatus == FALSE&&(pReadTask->DatagramSeq == pReadMeterInd->ApsSeq)))
	{
		AppGw3762Afn14F1State.ValidFlag = FALSE;
       app_printf("Gw3762 up Afn06F2 report.\n");
       app_gw3762_up_afn06_f2_report_read_data(pReadTask->Addr, RouterActiveInfo_t.CrnMeterIndex, pReadTask->ProtoType,
                             1, pReadMeterInd->Asdu, pReadMeterInd->AsduLength, pReadTask->MsgPort);

       router_read_change_route_active_state(EVT_ROUTER_REQ, 100);

       if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
       {
           up_cnm_addr_by_read_meter(pReadMeterInd, ScrMeterAddr);
       }
	}

    return;
}

static void router_read_send_afn_14f1_up_frame(void)
{
    AppGw3762Afn14F1Up_t   AppGw3762Afn14F1UpFrame;
        
    app_printf("RouterActiveInfo_t.CrnRMRound : --%d-- CrnMeterIndex :  --%d--\n", RouterActiveInfo_t.CrnRMRound, RouterActiveInfo_t.CrnMeterIndex);

    AppGw3762Afn14F1UpFrame.CmnPhase       =    WhiteMacAddrList[RouterActiveInfo_t.CrnMeterIndex].MeterInfo_t.ucPhase;
    __memcpy(AppGw3762Afn14F1UpFrame.Addr, WhiteMacAddrList[RouterActiveInfo_t.CrnMeterIndex].MacAddr, 6);
    AppGw3762Afn14F1UpFrame.MeterIndex     =    RouterActiveInfo_t.CrnMeterIndex;
    AppGw3762Afn14F1UpFrame.ReTransmitCnt  =    RETRANMITMAXNUM;

    app_gw3762_up_afn14_f1_up_frame(AppGw3762Afn14F1UpFrame);
}

void router_evt_router_reset(void)
{
    U16 ii = 0;
    RouterActiveInfo_t.CrnRMRound	  =  0;
    //RouterActiveInfo_t.CrnMeterIndex =  0;

    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        WHLPTST //????????
        WhiteMacAddrList[ii].Result = e_UNFINISH;
        WHLPTED//??????????
    }

    AppGw3762Afn10F4State.ReadedNodeCount = 0;
    AppGw3762Afn10F4State.RelayReadedCount = 0;
    AppGw3762Afn10F4State.RunStatusWorking = 0 ;
	AppGw3762Afn10F4State.RunStatusRouter =  0;
	AppGw3762Afn10F4State.WorkSwitchRegister =  0;
	//AppGw3762Afn10F4State.WorkSwitchEvent =  0;
	//AppGw3762Afn10F4State.WorkSwitchZone =  0;
	AppGw3762Afn10F4State.WorkSwitchStates =  OTHER_SWITCH_FLAG;
}

static void router_read_meter_reset(void)
{
    U16 ii = 0;
    RouterActiveInfo_t.CrnRMRound	  =  0;
    RouterActiveInfo_t.CrnMeterIndex =  0;

    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        WHLPTST //????????
        WhiteMacAddrList[ii].Result = e_UNFINISH;
        WHLPTED//??????????
    }

    AppGw3762Afn10F4State.ReadedNodeCount = 0;
    AppGw3762Afn10F4State.RelayReadedCount = 0;
    AppGw3762Afn10F4State.RunStatusWorking = TRUE ;
}                                

static void router_read_active_run(work_t *work)
{
    app_printf("RouterActiveInfo_t.State : %-20s\n",
										EVT_ROUTER_RESET==RouterActiveInfo_t.State?"EVT_ROUTER_RESET":
										EVT_ROUTER_RESTART==RouterActiveInfo_t.State?"EVT_ROUTER_RESTART":
										EVT_ROUTER_STOP==RouterActiveInfo_t.State?"EVT_ROUTER_STOP":
										EVT_ROUTER_REQ==RouterActiveInfo_t.State?"EVT_ROUTER_REQ":
										EVT_ROUTER_RESP==RouterActiveInfo_t.State?"EVT_ROUTER_RESP":
										EVT_APS_IND==RouterActiveInfo_t.State?"EVT_APS_IND":
                                        EVT_ROUTER_IDLE==RouterActiveInfo_t.State?"EVT_ROUTER_IDLE":
                                                                "error state");
    if(RouterActiveInfo_t.State == EVT_ROUTER_RESET )
    {
        router_read_meter_reset();
		//app_gw3762_up_afn14_f2_router_req_clock(e_UART1_MSG);
		RouterActiveInfo_t.State = EVT_ROUTER_REQ;
        RouterActiveInfo_t.Timeout = 2*TIMER_UNITS;
    }
    else if(RouterActiveInfo_t.State == EVT_ROUTER_RESTART )
    {
        RouterActiveInfo_t.State = EVT_ROUTER_REQ;
        RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
    }
	else if(RouterActiveInfo_t.State == EVT_ROUTER_STOP )
    {
        RouterActiveInfo_t.State = EVT_ROUTER_IDLE;
		RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
    }
    else if(RouterActiveInfo_t.State == EVT_ROUTER_REQ )
    {
    	U16 state;
        U8 router_ilde_flag = 0;        
            
		state = router_read_state_req();

        if(RouterActiveInfo_t.IdleFlag == TRUE)
        {
            router_ilde_flag = TRUE;
        }
	   
		app_printf("EvtRouterReq state : %-20s||router idle flag : %s\n",
										MAX_ROUND_FINISH==state?"max round finish":
										MAX_ROUND_UNFINISH==state?"max round unfinish":
										FREE_ROUND_FINISH==state?"free round finish":
										FREE_ROUND_UNFINISH==state?"free round unfinish":
										READING_STATUS==state?"reading":"error state",
										    0==RouterActiveInfo_t.IdleFlag?"routing":"idle");
                                        
        RouterActiveInfo_t.IdleFlag = FALSE;
        
        if(MAX_ROUND_FINISH==state)
		{
			if(router_ilde_flag == TRUE)
			{
                router_read_send_afn_14f1_up_frame();
				RouterActiveInfo_t.State = EVT_ROUTER_RESP;
                RouterActiveInfo_t.Timeout = 10*TIMER_UNITS;
			}
			else//
			{			    
                RouterActiveInfo_t.State = EVT_ROUTER_IDLE;
                RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
			}
		}
		else if(MAX_ROUND_UNFINISH==state)//1:
		{
			RouterActiveInfo_t.CrnRMRound = MAX_ROUND-2;
			RouterActiveInfo_t.State = EVT_ROUTER_IDLE;	
            RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
        }
		else if(FREE_ROUND_FINISH==state)//2:
		{
			RouterActiveInfo_t.CrnRMRound -= 1;
			if(router_ilde_flag == TRUE)//
			{
                router_read_send_afn_14f1_up_frame();
                RouterActiveInfo_t.State = EVT_ROUTER_RESP; 
                RouterActiveInfo_t.Timeout = 10*TIMER_UNITS;
			}
			else//
			{
                RouterActiveInfo_t.State = EVT_ROUTER_IDLE;
                RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
			}
		}
		else if(FREE_ROUND_UNFINISH==state)//3:
		{
			RouterActiveInfo_t.CrnMeterIndex = 0;
			RouterActiveInfo_t.State = EVT_ROUTER_REQ;
            RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
		}
		else if(READING_STATUS==state) //
		{
            router_read_send_afn_14f1_up_frame();
			RouterActiveInfo_t.State = EVT_ROUTER_RESP;
            RouterActiveInfo_t.Timeout = 10*TIMER_UNITS;
		}
    }
    else if(RouterActiveInfo_t.State == EVT_ROUTER_RESP )
    {
        RouterActiveInfo_t.CrnMeterIndex++;
        RouterActiveInfo_t.State = EVT_ROUTER_REQ;
        RouterActiveInfo_t.Timeout = 0.1*TIMER_UNITS;
    }
    else if(RouterActiveInfo_t.State == EVT_APS_IND )
    {
        //RouterActiveInfo_t.CrnMeterIndex++;
        RouterActiveInfo_t.State = EVT_ROUTER_REQ;
        RouterActiveInfo_t.Timeout = 30*TIMER_UNITS;
    }
	else if (RouterActiveInfo_t.State == EVT_ROUTER_IDLE )
    {
        RouterActiveInfo_t.IdleFlag = TRUE;
        RouterActiveInfo_t.CrnMeterIndex++;
        RouterActiveInfo_t.State = EVT_ROUTER_REQ;
        RouterActiveInfo_t.Timeout = 30*60*TIMER_UNITS;
        
		AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;
		AppGw3762Afn10F4State.RunStatusWorking = FALSE;
        /*
		do
		{
	        if(e_FINISH == WhiteMacAddrList[RouterActiveInfo_t.CrnMeterIndex].Result)
	        {
	        	WhiteMacAddrList[RouterActiveInfo_t.CrnMeterIndex].Result = e_UNFINISH;
	            break;
	        }
		}while(++RouterActiveInfo_t.CrnMeterIndex<MAX_WHITELIST_NUM);
		*/
        //app_printf("EVT_ROUTER_IDLE!\n");
    }
    else
    {

    }

    route_read_modify_timer(RouterActiveInfo_t.Timeout);
}

static void router_read_meter_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	work_t *work = zc_malloc_mem(sizeof(work_t),"Route",MEM_TIME_OUT);
	work->work = router_read_active_run;
	work->msgtype =ROUTER_RM;
	post_app_work(work);	
}

static void router_read_afn_14f1_read_meter_timeout(void *pTaskPrmt)
{
	app_printf("router read time out\n");
	AppGw3762Afn14F1State.ValidFlag = FALSE;
    RouterActiveInfo_t.CrnMeterIndex++;
    router_read_change_route_active_state(EVT_ROUTER_REQ, 100);

    return;
}

void router_read_change_route_active_state(uint8_t state, uint32_t timeout)
{               
	if(EVT_ROUTER_STOP == RouterActiveInfo_t.State)
	{
		if((EVT_ROUTER_STOP == state || EVT_ROUTER_RESET == state
		|| EVT_ROUTER_RESTART == state))
		{
			RouterActiveInfo_t.State = state;
	    	RouterActiveInfo_t.Timeout = timeout;
	    	route_read_modify_timer(RouterActiveInfo_t.Timeout);
		}
		else
		{
			app_printf("ROUTER STOP!\n");
		}
	}
	else
	{
		RouterActiveInfo_t.State = state;
    	RouterActiveInfo_t.Timeout = timeout;
    	route_read_modify_timer(RouterActiveInfo_t.Timeout);
	}

    return;
}

int8_t  router_read_meter_init(void)
{
    if(route_rd_timer == NULL)
    {
        route_rd_timer = timer_create(100,
                                100,
	                            TMR_TRIGGER,//TMR_TRIGGER
                                router_read_meter_timer_cb,
	                            NULL,
	                            "ReadMeterTimerCB"
	                           );
    }

	//timer_start(route_rd_timer);

	return TRUE;
}

static void route_read_modify_timer(uint32_t TimeMs)
{
	if(NULL != route_rd_timer)
	{
		timer_modify(route_rd_timer,
                TimeMs,
				100,
				TMR_TRIGGER ,//TMR_TRIGGER
                router_read_meter_timer_cb,
				NULL,
                "ReadMeterTimerCB",
                TRUE);
	}
	else
	{
		sys_panic("route_rd_timer is null\n");
	}
	timer_start(route_rd_timer);
}

void router_read_refresh_timer(void)
{
    if(zc_timer_query(route_rd_timer) == TMR_RUNNING)
    {
        if(timer_remain(route_rd_timer) < 30*TIMER_UNITS)
        {
            app_printf("router_read_refresh_timer 30 S\n");
            route_read_modify_timer(30*TIMER_UNITS);
        }
    }
}

void router_read_suspend_timer(U32 time1S)
{
    if(zc_timer_query(route_rd_timer) == TMR_RUNNING)
    {
        router_read_change_route_active_state(EVT_ROUTER_STOP, time1S*TIMER_UNITS);
        app_printf("router_read_suspend_timer %d time1S\n",time1S);
    }
}

void router_read_stop_timer(void)
{
    if(zc_timer_query(route_rd_timer) == TMR_RUNNING)
    {
        RouterActiveInfo_t.State = EVT_ROUTER_IDLE;
        RouterActiveInfo_t.Timeout = 30*60*TIMER_UNITS;
        timer_stop(route_rd_timer,TMR_NULL);
        app_printf("router_read_stop_timer\n");
    }
}

#endif


