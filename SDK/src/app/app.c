/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */
#include <stdio.h>
#include "cmd.h"
#include "dev_manager.h"
#include "app.h"
#include "app_dltpro.h"
#include "app_read_jzq_cco.h"
#include "app_slave_register_cco.h"
#include "app_slave_register_sta.h"


#include "app_exstate_verify.h"

#include "app_time_calibrate.h"
#include "app_read_router_cco.h"
#include "app_commu_test.h"
#include "app_moduleid_query.h"

#include "app_phase_position_cco.h"
#include "app_power_onoff.h"
#include "app_rdctrl.h"
#include "Datalinkdebug.h"
#include "app_clock_sync_sta.h"

#include "app_cnf.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "power_manager.h"
#include "app_off_grid.h"
#include "app_area_change.h"
#include "app_cco_clock_overproof_event.h"
#include "app_clock_sta_timeover_report.h"
#include "app_data_freeze_cco.h"
#include "app_cmd_info.h"
#include "power_manager.h"
#include "app_phaseseq_judge_sta.h"
#ifdef JIANG_SU
#include "app_deviceinfo_report.h"
#endif //JIANG_SU
//#if defined(MIZAR9M)//defined(ROLAND1_1M) || defined(UNICORN2M)
//#define TASK_APP_WORK_STK_SZ	1024
//#else
//#define TASK_APP_WORK_STK_SZ	4096
//#endif
//#define TASK_PRIO_APP_WORK	60
//#if defined(STATIC_NODE)//defined(ROLAND1_1M) || defined(UNICORN2M)
//#define APP_MSG_ENTRY_NR 20
//#else
//#define APP_MSG_ENTRY_NR 40
//#endif
#define TASK_APP_WORK_STK_SZ	1024
#define TASK_PRIO_APP_WORK	60
#define APP_MSG_ENTRY_NR 20



int32_t pid_app_work = 0;
msgq_t app_wq;
uint32_t stk_app_work[TASK_APP_WORK_STK_SZ];

event_t work_mutex;

U8  app_deal_flag = FALSE;

void post_app_work(work_t *work)
{
	void *p;

    mg_process_record_init(work);
    p=msgq_enq(&app_wq, work);
	if(p)
    {
	  mem_record_list_t *pTest = (void *)work - sizeof(mem_record_list_t);
      mem_record_list_t *pfirst = (void *)msgq_first(&app_wq) - sizeof(mem_record_list_t);
      
	  printf_s("fapp nr%d a= %p n= %s:fa= %p fn= %s\n",app_wq.nr,pfirst,pfirst->s,pTest,pTest->s);
      app_deal_flag = TRUE;
      zc_free_mem(p);
    }

	return;
}

void task_app_work()
{
	work_t *work;

	serial_cache_timer_init();

    app_ext_info_read_and_check();
    ReadBoardCfgInfo();
    ReadDefaultInfo();
	ReadExtInfo();
	ReadFreqInfo();
    ReadOutVersion();
    extern void UseParameterCFG(void);
    UseParameterCFG();

    //初始化系统时间
    SysTemTimeInit();
    
    APP_UPLINK_HANDLE(pNetStatusChangeInd, ProcNetworkStatusChangeIndication);
#if defined(STATIC_MASTER)
    APP_UPLINK_HANDLE(cco_read_meter_ind_hook,          jzq_read_cco_app_read_meter_ind_proc); 
    APP_UPLINK_HANDLE(cco_read_meter_cfm_hook,          jzq_read_cco_app_read_meter_cfm_proc);

    APP_UPLINK_HANDLE(upgrade_start_cfm_hook,           upgrade_cco_app_upgrade_start_cfm_proc);
    APP_UPLINK_HANDLE(upgrade_status_query_cfm_hook,    upgrade_cco_app_upgrade_status_query_cfm_proc);
    APP_UPLINK_HANDLE(upgrade_stainfo_query_cfm_hook,   upgrade_cco_upgrade_station_info_cfm_proc);
    APP_UPLINK_HANDLE(cco_moduleid_query_cfm_hook, cco_app_module_id_query_cfm_proc);
    APP_UPLINK_HANDLE(cco_QuerySwORValue_cfm_hook, cco_query_clk_or_value_cfm);
    APP_UPLINK_HANDLE(cco_SetSwORValue_cfm_hook, cco_set_clk_or_value_cfm);
	
	APP_UPLINK_HANDLE(app_assoc_renew_one_hook, app_assoc_renew_one_proc);    //陕西，湖南
	APP_UPLINK_HANDLE(cco_curve_profile_cfg_cfm_hook, CurveProfileCfgCfmProc);//陕西，湖南
	
	//读取配置波特率
	cco_serial_baud_init();
	
	off_grid_list_init();
    //判断传输模式
	judge_transmode_swc();
    
    //CCO 低功耗模式
	#if defined(POWER_MANAGER)
    APP_UPLINK_HANDLE(check_lowpower_states_hook, check_lowpower_states_proc);
    lowpower_init();
	#endif

    area_change_report_info_init();
    router_read_meter_init();
    slave_register_timer_init();
    moduleid_query_timer_init();
    cco_phase_position_timer_init();
    clock_overproof_event_timer_init();
    
    cco_eventreport_timer_init();
    cco_clean_bitmap_timer_init();

    upgrade_cco_init();
    
    manage_link_init();
    multi_trans_timer_init();

    pPhasePositionCfmProc = cco_phase_position_phase_proc;

    Readsysinfo();

    //上电直接修改组网序列号，防止CCO上电不足一分钟会发送离线指示
	IncreaseFromatSeq();


    extern void CheckJZQMacAddr(void);
    CheckJZQMacAddr();
	
    extern void UseFunctionSwitchDef(void);
    UseFunctionSwitchDef();

    os_sleep(200);

    SetMacAddrRequest(FlashInfo_t.ucJZQMacAddr, e_JZQ,0); 
    
    FlashInfo_t.scJZQProtocol = e_GW13762_MSG;
    FlashInfo_t.ucJZQType = APP_JZQ_TYPE_13;

	if(PROVINCE_HUNAN==app_ext_info.province_code || PROVINCE_SHANNXI==app_ext_info.province_code)
	{
		cco_curve_init();//陕西、湖南数据冻结初始化
	}
	
    //送检需要关闭
	cco_time_manager_init();//精准时钟初始化

#endif

#if defined(STATIC_NODE)
    //flash读取模块信息
	ReadMeterInfo();

	sta_time_manager_init();
	sta_time_over_init();

    //AODV开关使用配置文件
    nnib.AODVRouteUse = app_ext_info.func_switch.AODVSWC;
	register_nettick_func(cacl_nbnet_difftime);
#if defined(ZC3750STA)
    //os_sleep(100);
    sta_bind_init();
    sta_meter_prtcltype_init();
    #if defined(STA_BOARD_3_0_02)
    sta_led_init();
    #endif
    


#endif

#if defined(ZC3750CJQ2)
	cjq_led_init();
    cjq2_search_meter_timer_init();
#endif
    
    APP_UPLINK_HANDLE(sta_read_meter_ind_hook, sta_read_app_read_meter_ind_proc);
    APP_UPLINK_HANDLE(time_calibrate_hook, time_calibrate_ind_proc);
    APP_UPLINK_HANDLE(sta_register_query_ind_hook, SlaveRegQueryIndication);
    APP_UPLINK_HANDLE(sta_moduleid_query_ind_hook, query_id_info_indication);
    APP_UPLINK_HANDLE(rdctrl_moduleid_query_cfm_hook, rd_query_id_info_cfm);
    APP_UPLINK_HANDLE(upgrade_start_ind_hook, upgrade_sta_start_proc);
    APP_UPLINK_HANDLE(upgrade_stop_ind_hook, upgrade_sta_stop_proc);
    APP_UPLINK_HANDLE(upgrade_filetrans_ind_hook, upgrade_sta_file_trans_proc);    
    APP_UPLINK_HANDLE(upgrade_status_query_ind_hook, upgrade_sta_status_query_proc);
    APP_UPLINK_HANDLE(upgrade_stainfo_query_ind_hook, upgrade_sta_station_info_proc);
    APP_UPLINK_HANDLE(upgrade_perform_ind_hook, upgrade_sta_perform_proc);

    

    APP_UPLINK_HANDLE(sta_QuerySwORValue_ind_hook, sta_query_clk_or_value_Indication);
    APP_UPLINK_HANDLE(sta_SetSwORValue_ind_hook, sta_set_clk_or_value_Indication);
    event_report_timer_init();		//事件上报初始化
    power_detect_init();			//12V电压检测初始化
    exstate_verify_timer_init();	//复电检测定时器初始化
    exstate_reset_timer_init();		//外部软复位检测定时器初始化
    exstate_event_timer_init();		//外部事件引脚检测定时器初始化
    zero_lost_check_timer_init();	//过零检测定时器初始化
    #if defined(ZC3750STA)
	plug_check_timer_init();		//在位检测定时器
	reset_check_timer_init();
	sta_io_ctrl_timer_init();
	addr_judge_timer_init();
	//IO_check_timer_Init();
    if(PROVINCE_HEBEI == app_ext_info.province_code)
    {
        //三相相序识别
        sta_phaseseq_judgetimer_init();
    }

	#if defined(POWER_MANAGER)
	APP_UPLINK_HANDLE(check_lowpower_states_hook, check_lowpower_states_proc);
	lowpower_init();
	#endif
	
    #endif
    sta_read_clear_13f1_rm_data_timer_init();
    poweroff_report_timer_init();
    powerevent_send_timer_init();

    poweron_report_timer_init();
    sta_clean_bitmap_timer_init();
    upgrade_sta_check_first_init();
#endif
    reboot_timer_init();
    //通信测试处理
    APP_UPLINK_HANDLE(commu_test_ind_hook, CommuTestIndication);

    APP_UPLINK_HANDLE(indentification_ind_hook, AppIndentificationIndProc);

    /* 回传模式测试 */
    APP_UPLINK_HANDLE(check_CB_FacTestMode_hook, Check_CB_FacTestMode_Proc);

    //网络数据处理函数指针初始化
    APP_UPLINK_HANDLE(pProcessMsduCfm, ProcMsduDataConfirm);
    APP_UPLINK_HANDLE(pProcessMsduDataInd, ProcMsduDataIndication);
    //初始化网络状态变化处理函数
    //APP_UPLINK_HANDLE(pNetStatusChangeInd, ProcNetworkStatusChangeIndication);
    #if defined(STATIC_MASTER) && defined(JIANG_SU)
    //初始化一个节点网络状态变化处理函数
    APP_UPLINK_HANDLE(pOneStaNetStatusChangeInd, ProcOneStaNetworkStatusChangeIndication);
    #endif
    //抄控器数据处理函数
    APP_UPLINK_HANDLE(rdctrl_ind_hook, proc_rd_ctrl_indication);
    APP_UPLINK_HANDLE(rdctrl_to_uart_ind_hook, proc_rd_ctrl_to_uart_indication);
	
    //读取芯片ID，送检注释掉，量产需要支持
    /*#ifndef MODULE_INSPECT
    ReadManageID();
    #endif*/
	if( VOLUME_PRODUCTION == app_ext_info.func_switch.IDInfoGetModeSWC)//量产
	{
		ReadManageID();
	}

    //显示基本信息
    show_version();
    #if defined(STATIC_MASTER)
    //显示应用功能开关
    showfunctionSwc(g_vsh.term);
    //显示配置参数
    show_parameterCFG(g_vsh.term);
    //自动升级
    upgrade_cco_start_upgrade_byself();
    #endif
    while(1) {
        work = (work_t *)msgq_deq(&app_wq);
        mutex_pend(&work_mutex, 0);
        if(work->work != NULL)
        {

#if DATALINKDEBUG_SW
//        	work->recvtick = get_phy_tick_time();
            mg_process_record_in(work,2,work->msgtype);
            pAppDealNow = (void *)work - sizeof(mem_record_list_t);
#endif

			work->work(work);

#if DATALINKDEBUG_SW
			extern void mg_process_record(work_t *work,U8 taskid);
			mg_process_record(work,2);
            pAppDealNow = NULL;
            if(app_deal_flag)
            {
                mem_record_list_t *pTest = (void *)work - sizeof(mem_record_list_t);
                printf_s("app deal ok addr = %p,name = %s\n",pTest,pTest->s);
                app_deal_flag = FALSE;
            }
#endif
			
        }

        mutex_post(&work_mutex);

		zc_free_mem(work);
	}

	return;
}

void app_init(void)
{
    /*
    //U8 crcBuf[13] = {0xE7,0x51,0xCB,0x10,0xCA,0x8B,0xCE,0xC6,0xE6,0x2B,0x0F,0x30,0x36};
    U8   crcBuf[13] = {0xe7,0x8a,0xd3,0x08,0x53,0xd1,0x73,0x63,0x67,0xd4,0xf0,0x0c,0x6c};
    U32 crcValue;
    U32 Crc32Data;

    crcValue = crc_digest((unsigned char *)crcBuf, 13, (CRC_24 | CRC_SW), &Crc32Data);
    app_printf("crcValue = %08x, Crc32Data = %08x\n", crcValue, Crc32Data);
    */
//    register_zero_cross_hook(EVT_ZERO_CROSS0, ZeroCrossISR, NULL);
    
    msgq_init(&app_wq, "app wq", offset_of(work_t, link), APP_MSG_ENTRY_NR);
	
//    if (!(mutexNetShareProt_t = mutex_create("mutexNetShareProt")))
//    {
//        sys_panic("<mutexNetShareProt panic> %s: %d\n", __func__, __LINE__);
//    }
    mutex_init(&mutexNetShareProt_t, "mutexNetShareProt");
    mutex_init(&Mutex_RdCtrl_Search, "Mutex_RdCtrl_Search");
	RD_SACK_PEND;
	if ((pid_app_work = task_create(task_app_work,
				    NULL,
				    &stk_app_work[TASK_APP_WORK_STK_SZ - 1],
				    TASK_PRIO_APP_WORK,
				    "task_app_work",
				    &stk_app_work[0],
				    TASK_APP_WORK_STK_SZ)) < 0)

		APP_PANIC();

	return;
}



