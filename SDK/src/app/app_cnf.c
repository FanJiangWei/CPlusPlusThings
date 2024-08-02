#include "app_cnf.h"
#include <string.h>
#include "types.h"
#include "ZCsystem.h"
#include "ProtocolProc.h"
#include "sys.h"
//#include "app_eventreport.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"

#include "app_power_onoff.h"
#include "dev_manager.h"
#include "app_exstate_verify.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "app_upgrade_sta.h"
#include "app_upgrade_cco.h"
#include "app_phase_position_cco.h"
#include "app_slave_register_cco.h"
#include "SyncEntrynet.h"
#include "Scanbandmange.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "netnnib.h"
#include "meter.h"
#include "app_gw3762_ex.h"
#include "Datalinkdebug.h"

U8  NetFormationFinishedFlag;
ostimer_t   *send_03f10_timer = NULL;
U8  send_03f10_flag;

U8  Gw3762TopUpReportData[512];


U16 APP_GETTEI()
{
	U16 tei;
	net_nnib_ioctl(NET_GET_TEI,&tei);
	return tei;
}

U32 APP_GETSNID()
{
	U32 snid;
	net_nnib_ioctl(NET_GET_SNID,&snid);
	return snid;
}

#if defined(STATIC_MASTER)

U16 APP_GETDEVNUM()
{
	U16 num;
    DeviceList_ioctl(DEV_APP_GET_LISTNUM,NULL,&num);//安徽
	return num;
}

U16 APP_GETTEI_BYMAC(U8 *MAC)
{
	U16 num;
	DeviceList_ioctl(DEV_GET_TEI_BYMAC,MAC,&num);
	return num;
}

static void app_send_03f10(work_t *work)
{
	if(0U == send_03f10_flag)
	{
		extern void app_gw3762_up_afn03_f10_master_mode(U8 mode, MESG_PORT_e port);
		app_gw3762_up_afn03_f10_master_mode(TRUE,e_UART1_MSG);			
	}
}

static void app_send_03f10_timer_cb(struct ostimer_s *ostimer, void *arg)
{   
	app_printf("send_03f10 timer out!\n");
	work_t *work = zc_malloc_mem(sizeof(work_t),"app_send_03f10",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	work->work = app_send_03f10;
    work->msgtype = SEND_03F10;
	post_app_work(work);
}
#endif

#if defined(STATIC_NODE)

U8 AppNodeState = e_NODE_OUT_NET;

void CleanAppStatus(void)
{
	//可清理位图
	if(TMR_RUNNING == zc_timer_query(sta_power_off_send_timer))
	{
		timer_stop(sta_power_off_send_timer,TMR_NULL);
	    app_printf("out to net and stop sta_power_off_send_timer\n");
	}
	if(TMR_RUNNING == zc_timer_query(sta_power_on_send_timer))
	{
		timer_stop(sta_power_on_send_timer,TMR_NULL);
	    app_printf("out to net and stop sta_power_on_send_timer\n");
	}
	/* if(TMR_RUNNING == zc_timer_query(clean_bitmap_timer))
	{
		timer_stop(clean_bitmap_timer,TMR_CALLBACK);
		app_printf("out to net and cleanbitmap\n");
	} */
    /* 直接清理位图 */
	sta_clean_bitmap();
}
#endif

void ProcNetworkStatusChangeIndication(work_t *work)
{
	U8 status=work->buf[0];
    #if defined(STATIC_NODE)
	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);  

	if(status == TO_ONLINE) //入网
	{
	    //入网后退出低功耗模式
	    if(check_lowpower_states_hook != NULL)
        {   
            check_lowpower_states_hook();
        }
	    AppNodeState = e_NODE_ON_LINE;
		CleanAppStatus();
        
        recover_power_on_report();
        
		CleanDistinguishResult(ccomac);
		app_printf("goin to net\n");
		//二采不需要事件检测
		#if defined(ZC3750STA)
		if(READ_EVENT_PIN && event_report_save.check == e_event_none)
		{
			event_report_save.check = e_event_get;
			//timer_start(exstate_verify_timer);
			timer_start(exstate_event_timer);
		}
		#endif
        if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_UPGRADE_TEST_RUN)
    	{
    		SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE ;
    		SaveUpgradeStatusInfo();
    		SlaveUpgradeFileInfo.OnlineStatus = TRUE;
    		app_printf("change OnlineStatus\n");
    	}	
        #if defined(ROLAND1_1M)&&defined(ZC3750STA)
        if(GetnnibDevicetype() == e_3PMETER)
        {      
            if(READ_EN_A_PIN == 0||READ_EN_B_PIN == 0||READ_EN_C_PIN == 0)
            {
                app_printf("PHY_PHASE_ALL2\n");
                ld_set_3p_meter_phase_zc(PHY_PHASE_ALL);
            }
        }
        #endif

        #if defined(STATIC_NODE)        //<<< RF test
		//停止同步和扫频流程 实际上不需要
        // if(zc_timer_query(network_timer) == TMR_RUNNING)
        //     timer_stop(network_timer, TMR_NULL);
		// stopScanBandTimer();
		// StopScanRfTimer();

		//保存无线信道信息
		save_Rf_channel(getHplcOptin(), getHplcRfChannel());
        #endif
	}
	else if(status == TO_OFFLINE)//离线
	{
		AppNodeState = e_NODE_OUT_NET;
		CleanAppStatus();
		#if defined(ZC3750STA)
		if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
		{
			//黑龙江灯语要求
			//led_tx_ficker();
		}
		else
		{
			led_tx_ficker();
		}
		#endif
		if(g_ReqReason == e_CCOnidChage || g_ReqReason == e_FormationSeq || g_ReqReason == e_RecvLeaveInd)//由于CCO的SNID变化或者组网序号变化的离线，清除分布式台区识别的结果。
		{
			U8 i;
		
			memset((U8*)&Final_result_t,0,sizeof(Final_result_t));
			for(i=0;i<COMMPAER_NUM;i++)
			{
				Final_result_t.PublicResult_t[i].CalculateMaxNum = MAX_CACULATE-5;
			}
			if(zc_timer_query(wait_finish_timer) == TMR_RUNNING)
		    {
		        timer_stop(wait_finish_timer,TMR_NULL);
		    }
		}
		app_printf("goout to net\n");
	
	}
	#elif defined(STATIC_MASTER)
	if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO：江苏独立
	{
	    extern void check_network_status();
        check_network_status();
	}
	if(status == TO_WORKFINISH)
	{
		U8 pwrtype;
		net_nnib_ioctl(NET_GET_PWRTYPE,&pwrtype);
		if(pwrtype) //强电启动相位识别
		{
		  cco_phase_position_start(e_CENTRALIZATION,period_bit);
		}
		else
		{
			app_printf("it's don't phase idtifacation\n");
		}
        //组网完成启动CJQ搜表
        if(PROVINCE_JIANGSU != app_ext_info.province_code)
        {
            if(TMR_RUNNING != zc_timer_query(register_over_timer)&&TMR_STOPPED == zc_timer_query(g_CJQRegisterPeriodTimer))
            {
                register_slave_start(3600*1000, e_APP_MSG);
            }
        }
		#ifdef SHAAN_XI
        else if(PROVINCE_SHANNXI == app_ext_info.province_code)
        {
            send_broadcast_live_line_zero_line_enabled();
        }
		#endif
	}

	if(TO_NET_INIT_OK == status)
	{
		if(NULL == send_03f10_timer)
		{
			send_03f10_timer = timer_create(SEND_03F10_SEC * TIMER_UNITS, 
											0,
											TMR_TRIGGER ,//TMR_TRIGGER
											app_send_03f10_timer_cb,
											NULL,
											"app_send_03f10_timer_cb"
											);
    	}

		if(NULL != send_03f10_timer)
		{
			timer_start(send_03f10_timer);
			app_printf("send_03f10 timer start: %d\n", SEND_03F10_SEC);			
		}
		else
		{
			app_printf("send_03f10_timer is NULL!\n");
		}
	}
	#endif
}




#if defined(STATIC_MASTER)
static void ProcQuetyCJQ2InfoConfirm(U8 *pMsgData)
{
	app_printf("ProcQuetyCJQ2InfoConfirm(U8 *pMsgData)!\n");
	//更新采集器信息
/*	if(FlowEvent.event & EVT_REGISTER_RUNING && SystemState_e == e_SLAVE_REGISTER_START)
	{
		app_printf("osh_event_send EVT_REGISTER_QUERY!\n");
		FlowEvent.event  = 0;
		osh_event_send(&FlowEvent, EVT_REGISTER_QUERY, 0);
	}
	else
	{
		//异常流程
	}*/

	//释放mdb
}





static void	ProcSetSnidConfirm(U8 *pMsgData) {}
static void	ProcQueryNgbrListNumConfirm(U8 *pMsgData) {}


static void	ProcSetWhiteListConfirm(U8 *pMsgData) {}
static void	ProcQueryWhiteListConfirm(U8 *pMsgData) {}







#endif

DealFunc ProcNet2AppPLC2016MsgTypeFuncArray[] =
{
   // ProcSetMacAddrConfirm,
    //ProcQueryMacAddrConfirm,
    //ProcQueryRouterListNumConfirm,
    //ProcQueryRouterListConfirm,
  
    //ProcSetAcessListConfirm,
    //ProcQueryAcesslistConfirm,
    
	//ProcInstantFrozenIndtion,
	//ProcNetworkStatusChangeIndication,
#if defined(STATIC_MASTER)
	ProcQuetyCJQ2InfoConfirm,
    //ProcQueryToplogyConfirm,
    ProcSetSnidConfirm,
    ProcSetWhiteListConfirm,
    ProcQueryWhiteListConfirm,
    //ProcSetCJQRegisterConfirm,     
    ProcQueryNgbrListNumConfirm,
#endif
};


void ProcNet2AppPLC2016Msg(MSG_t *pMsg)//net indication confirm
{
    if(pMsg->MsgType >= (sizeof(ProcNet2AppPLC2016MsgTypeFuncArray) / sizeof(DealFunc) ))
    {
		app_printf("ProcNet2AppPLC2016Msg pMsg->MsgType = %d\n",pMsg->MsgType);
		return;//error
    }
    ProcNet2AppPLC2016MsgTypeFuncArray[pMsg->MsgType](pMsg->pMsgData);
}





