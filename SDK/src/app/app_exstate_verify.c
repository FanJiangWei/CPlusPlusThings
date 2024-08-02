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
#include "flash.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
//#include "app_base_multitrans.h"
//#include "app_eventreport.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_power_onoff.h"
#include "app_exstate_verify.h"
//#include "RouterTask.h"
//#include "NetTable.h"
#include "app_gw3762.h"
#include "ZCsystem.h"
#include "Datalinkdebug.h"
#include "app_meter_verify.h"
#include "dev_manager.h"
#include "plc_io.h"


ostimer_t *RebootTimer = NULL;// 重启定时器


#if defined(STATIC_NODE)

ostimer_t *ZeroLostCheckTimer = NULL;   // 过零丢失检测定时器

S8 ZeroLostOrGetCnt = 0;


//U8	g_downprotocol = FALSE;


//U8        PlugFlag =	e_plug_in;      //模块在位标志
//U8        NeedjugePower =	e_reset_disable;



U8 AddrJudgeFlag = FALSE;         // 如果模块插入后验表成功，则此标志为TRUE,则不用清flash重新绑表

U8 checkflag;

extern ostimer_t *ExStateVerifyTimer;     // 外部状态检查定时器





//#if defined(ZC3750STA)

ostimer_t *ResetCheckTimer = NULL;      // 软复位检查定时器
ostimer_t *PlugCheckTimer = NULL;      // 模块拔出状态检查定时器
ostimer_t *STA_IOCtrltimer = NULL;     // STA模块的STA脚状态输出控制定时器
ostimer_t *AddrJudgeTimer = NULL;      //判断是否需要重新绑表

//ostimer_t *g_CheckACDCTimer=NULL;	 //强弱电检测



ostimer_t *PowerDetectChecktimer = NULL;
U8  POWERDETECTTYPE = e_V2100_PULSE;
U8  PowerDetectVlue = 0;








#if defined(ZC3750STA)
void STA_on(U16 Times)
{
    app_printf("STA_on Times is %d!\n",Times);

	if(NULL != STA_IOCtrltimer)
	{
		timer_start(STA_IOCtrltimer);
		gpio_pins_on(&stasta, STA_CTRL);
	}

        
}
void STA_off(void)
{
    gpio_pins_off(&stasta, STA_CTRL);
}


static void STA_IOCtrlTimerProc(work_t *work)
{
    gpio_pins_off(&stasta, STA_CTRL);  
    app_printf("STA_OFF\n");
}


#endif

void PowerDetectChecktimerCB(struct ostimer_s *ostimer, void *arg)
{
    static S16 DetectCheckCnt = 0;
    U8 ioinfo = 0;
    static U16 checknum = 0;
    
    checknum ++;
    
    ioinfo = ShakeFilter(READ_DETECT_PIN ,IO_CHECK_TIME, &DetectCheckCnt);
	
    if(checknum > (IO_CHECK_TIME+1))
    {
        checknum = 0;
        PowerDetectVlue = ioinfo;
        if(module_state.zero_cross_state == e_zc_lost&&PowerDetectVlue != e_IO_HIGH)
        {
            timer_start(PowerDetectChecktimer); 
        }
        else
        {
            timer_stop(PowerDetectChecktimer,TMR_NULL);
        }
        
        if(PowerDetectVlue == e_IO_HIGH)
        {
            module_state.vol_detect_state = e_vol_loss;
            if(TMR_STOPPED == zc_timer_query(exstate_verify_timer))
            {      
    		    timer_start(exstate_verify_timer);
    			app_printf("PowerDetectChecktimerCB,start(exstate_verify_timer)\n");
            }
        }
        else
        {
            module_state.vol_detect_state = e_vol_normal;
        }
        app_printf("PowerDetectVlue = %d!\n",PowerDetectVlue);
    }
    
}


#if defined(STA_BOARD_2_1_00)

__isr__ void Restprocess(void *data)     // 软复位IO中断处理
{
    app_printf("Restprocess\n");
	//启动滤波定时器
	if(TMR_STOPPED == zc_timer_query(ResetCheckTimer) )
	{
		timer_start(ResetCheckTimer);
	}

}


#endif



#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)

__isr__ void poweroffdetect(void *data)   // 掉电检测中断
{
//    printf_s("nnib.PowerType = %d\n",nnib.PowerType);
	
	if(nnib.PowerType ==FALSE)
	{
		return;
	}

	if(POWERDETECTTYPE  == e_V3200_STATE)
    {
//        printf_s("e_V3200_STATE err!\n");
        //start check IO 
        if(TMR_STOPPED == zc_timer_query(PowerDetectChecktimer))
        {      
		    timer_start(PowerDetectChecktimer);
			app_printf("poweroffdetect,start(PowerDetectChecktimer)\n");
        }
        return;
    }   

	if(module_state.vol_detect_state == e_vol_normal
        && DevicePib_t.PowerOffFlag != e_power_off_now)//停电过程的过滤
	{
		module_state.vol_detect_state = e_vol_loss;   // 检测到电源跌落
		
		if(TMR_STOPPED == zc_timer_query(exstate_verify_timer))
        {      
		    timer_start(exstate_verify_timer);
			app_printf("start(exstate_verify_timer)\n");
        }
	}
//	printf_s("*************************poweroffdetect\n");
}

#endif






#if defined(OLD_STA_BOARD)
static void PowerDetectHigh(void)  // 在过零丢失检查定时器回调中调用此函数
{
    static S16 PowerCtrlhighNum = 0;
    U8 ioinfo = 0;
    ioinfo = ShakeFilter(READ_POWEROFF_CTRL , (IO_CHECK_TIME/2) , &PowerCtrlhighNum);  // 检测IO状态并去抖
    if(ioinfo == e_IO_HIGH)
	{
	    if(module_state.vol_detect_state == e_vol_normal
               && DevicePib_t.PowerOffFlag != e_power_off_now)//停电过程的过滤
    	{
    		module_state.vol_detect_state = e_vol_loss;   // 检测到电源跌落
//    		printf_s("*************************2 poweroffdetect\n");
    		
    	}
    }
    else
    {
        module_state.vol_detect_state = e_vol_normal;   // 未检测到电源跌落
        app_printf("Pl!\n");
    }
    
}
#endif

#if defined(ZC3750STA)
static void PlugCheckTimerProc(work_t *work)
{
    static S16 PlugCnt = 0;
    static U8  PlugCheck = 0;

    if(READ_PLUG_PIN)   //拔出状态
    {		
        PlugCnt = PlugCnt>0?0:PlugCnt; // 状态迁移

        PlugCnt = PlugCnt>-6?PlugCnt-1:PlugCnt; // 状态防抖

		
        if(PlugCnt<=-6 && e_plug_in == module_state.plug_state)
		{	
		    //首次上电判定是不带PLUG模式;如果是带停电模块，拔出状态后续继续检测
		    if(PlugCheck == 0)
            {
                PlugCheck = 1;
                module_state.power_type = e_without_power;
                module_state.plug_state = e_plug_in;
                app_printf("Sta e_without_power!\n");
                return;
            }
            if(module_state.power_type != e_without_power)
            {
    			app_printf("Sta was pulled out!\n");
                
    			module_state.plug_state = e_plug_out;//防抖结束，启动定时器，检测到拔出状态
    			
    			//启动总检查定时器，未启动，则启动。
    			if(zc_timer_query(exstate_verify_timer) == TMR_STOPPED)
                {
                    timer_start(exstate_verify_timer);
                }
            }
		}
	}
	else//插入状态
	{
		PlugCnt = PlugCnt<0?0:PlugCnt; // 状态迁移
		
		PlugCnt = PlugCnt<6?PlugCnt+1:PlugCnt;//状态防抖
		//首次上电判定是插入状态，后续不再检测
        if(PlugCnt>=6 && PlugCheck == 0)
        {
            PlugCheck = 2;
            module_state.power_type = e_with_power;
            module_state.plug_state = e_plug_in;
        }
		if(PlugCnt>=6&&e_plug_out==module_state.plug_state)
		{		
			app_printf("Sta is inserted!\n");
            //如果是带停电模块，插入状态后更新带停电模式
            if(PlugCheck == 1)
            {
                module_state.power_type = e_with_power;
            }
			module_state.plug_state = e_plug_in;//防抖结束，启动定时器，检测到插入状态
			//需要重新验表
			StartMeterCheckManage(VerifyPlug_e);
			//Modify_MeterComuJudgetimer(2);
			/*
		    if(AddrJudgeTimer)
	        {
	            if(zc_timer_query(sta_bind_timer) == TMR_STOPPED&&(FactoryTestFlag != 1))
	            {
		            timer_start(AddrJudgeTimer);
	            }
	        }	*/	
			
		}
	}    
}



#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)
static void ResetCheckTimerProc(work_t *work)
{
    static U8 restCheckCnt = 0;

    if(SelfCheckFlag == TRUE)
    {
        app_printf("SelfCheckFlag! return\n");
        return;
    }
    if((PLCRST_CTRL&gpio_input(NULL,PLCRST_CTRL)) == 0) 
  	{
        
  	}
	else
	{
		app_printf("err reset check!!!!\n");
        restCheckCnt = 0;
		//if(zc_timer_query(exstate_verify_timer) == TMR_RUNNING 
		//		&& module_state.soft_reset_state == e_reset_enable) return ;

		if(zc_timer_query(exstate_reset_timer) == TMR_RUNNING 
				&& module_state.soft_reset_state == e_reset_enable) return ;
		
        
		module_state.soft_reset_state = e_reset_disable;
		return ;
	}
        
    restCheckCnt ++;
    //200ms滤波后仍低电平，RST为真
    if(restCheckCnt >= 4)//200ms
    {
        restCheckCnt = 0;
		
        module_state.soft_reset_state = e_reset_enable;

        /*if(zc_timer_query(exstate_verify_timer) == TMR_STOPPED)
        {
            timer_start(exstate_verify_timer);
        }*/
        if(zc_timer_query(exstate_reset_timer) == TMR_STOPPED)
        {
            timer_start(exstate_reset_timer);
        }
        return ;
    }
    
    timer_start(ResetCheckTimer);
    
}

#endif

#endif


void zero_get_deal(void)  // 在过零中断中被调用，过零获得判断
{
    ZeroLostOrGetCnt = ZeroLostOrGetCnt>0?0: ZeroLostOrGetCnt ;//状态迁移

	ZeroLostOrGetCnt = (ZeroLostOrGetCnt <= -ZERO_LOST_NUM)?ZeroLostOrGetCnt: ZeroLostOrGetCnt-1;

	if(ZeroLostOrGetCnt <= -ZERO_LOST_NUM)
	{
		//if(e_zc_lost == zero_cross_flag)
		if(e_zc_get != module_state.zero_cross_state)
		{
		    module_state.zero_cross_state = e_zc_get;
			app_printf("zero_cross_flag = e_zc_get\n");
        #if defined(ZC3750STA)
            StartMeterCheckManage(VerifyZero_e);
        #endif
			checkflag =TRUE;
            timer_modify(g_CheckACDCTimer,900,
				  900,
				  TMR_TRIGGER,
				  CheckACDCTimerCB,
				  NULL,
                  "g_CheckACDCTimer",
                  TRUE);
		    timer_start(g_CheckACDCTimer);
			timer_stop(g_CheckACDCTimer,TMR_CALLBACK);
            #if defined(ZC3750STA) 
                Check_MeterType_timer_start();
            #endif
			//带电状态下无过零到有过零
			if(DevicePib_t.PowerOffFlag == e_power_off_now)
            {
                DevicePib_t.PowerOffFlag = e_power_off_last;
            }	
            
			if(zc_timer_query(exstate_verify_timer) == TMR_STOPPED)
            {
                timer_start(exstate_verify_timer);
            }
		}
		
	}

    if(module_state.zero_cross_state == e_zc_get&&zc_timer_query(g_CheckACDCTimer) == TMR_STOPPED)
	{
		checkflag =TRUE;
		timer_modify(g_CheckACDCTimer,900,
				  900,
				  TMR_TRIGGER,
				  CheckACDCTimerCB,
				  NULL,
                  "g_CheckACDCTimer",
                  TRUE);
		timer_start(g_CheckACDCTimer);
	}

    // 在每一次过零中断中刷新启动过零中断检查定时器，使此定时器不能超时回调    
    if(ZeroLostCheckTimer)
    {
        timer_start(ZeroLostCheckTimer);
    }
    

}


static void ZeroLostCheckTimerProc(work_t *work)   // 过零丢失判断
{  
    
    ZeroLostOrGetCnt = ZeroLostOrGetCnt<0?0: ZeroLostOrGetCnt;

	ZeroLostOrGetCnt = (ZeroLostOrGetCnt >= ZERO_LOST_NUM)?ZeroLostOrGetCnt: ZeroLostOrGetCnt+1;
    
    if(ZeroLostOrGetCnt >= ZERO_LOST_NUM)
    {
        //丢过零
        if(module_state.zero_cross_state == e_zc_get)
		{
			module_state.zero_cross_state = e_zc_lost;  // 过零丢失

            app_printf("------------------------lost zero cross----------------------\n");

			//有过零到无过零
			timer_stop(g_CheckACDCTimer,TMR_NULL);//过零丢失后，关闭强电检测
			if(zc_timer_query(exstate_verify_timer) == TMR_STOPPED)
            {
                timer_start(exstate_verify_timer);  // 启动外部状态检测
            }
		}
        else
        {
#if defined(OLD_STA_BOARD)
//旧版本检测逻辑下，检测到5个过零丢失，立马检测det高电平，即可实现检测
            PowerDetectHigh();  // 在5个过零中检测电源跌落检测IO高电平
#endif
        }
    }


}






#if defined(ZC3750STA)



static void AddrJudgeTimerProc(work_t *work)
{
    U8 *sendbuf = zc_malloc_mem(100, "sendbuf", MEM_TIME_OUT);
    U8 buflen = 0;
    static U8 JudgeCnt= 0;
    
    JudgeCnt ++;
    if(JudgeCnt > 1)
    {
        JudgeCnt = 0;
        if(AddrJudgeFlag == FALSE)//标识需要重新绑表，启动绑表定时器
        {
            app_printf("Judge Meter Time Out!\n");
            app_printf("clean STA_INFO Flash!\n");
            if(FLASH_OK==zc_flash_erase(FLASH,STA_INFO , sizeof(DEVICE_PIB_t)))
            	{
            		app_printf("stacleanflag erase BOARD_CFG 64k ok!\n");
                    print_s("-----stacleanflag appreboot------\n");		
        		
        		    app_reboot(500);
            	}
            	else
            	{
            	    JudgeCnt++;
            		app_printf("stacleanflag erase BOARD_CFG 64k error!\n");
                    timer_start(AddrJudgeTimer);
            	}
                
        }
        zc_free_mem(sendbuf);
        return;
    }
    if(AddrJudgeTimer )
    {
        timer_start(AddrJudgeTimer);
    }
    else
        sys_panic("<AddrJudgetimer fail!> %s: %d\n");
    if(module_state.plug_state == e_plug_in && JudgeCnt == 1)
    {
        if(sta_bind_packet_send_info(DevicePib_t.BandMeterCnt,sendbuf, &buflen) == OK)  // 验表
        {
            app_printf("Need to Judge Meter Addr,PlugFlag =%s\n",module_state.plug_state == TRUE?"TRUE":"FALSE");
            uart_send(sendbuf, buflen);
        }
    }
    AddrJudgeFlag = FALSE;
    zc_free_mem(sendbuf);
    
}

static void STA_IOCtrlTimerCB(struct ostimer_s *ostimer, void *arg)
{
    //work_t *work = zc_malloc_mem(sizeof(work_t),"PlugcheckTimerProc",MEM_TIME_OUT);
	//work->work = STA_IOCtrlTimerProc;
	//post_app_work(work);
	STA_IOCtrlTimerProc(NULL);
}


static void AddrJudgeTimerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"PlugcheckTimerProc",MEM_TIME_OUT);
	work->work = AddrJudgeTimerProc;
    work->msgtype = JUDGE_PROC;
	post_app_work(work);
}


static void PlugCheckTimerCB(struct ostimer_s *ostimer, void *arg)
{
    //work_t *work = zc_malloc_mem(sizeof(work_t),"PlugcheckTimerProc",MEM_TIME_OUT);
	//work->work = PlugCheckTimerProc;
	//post_app_work(work);
	PlugCheckTimerProc(NULL);
}

static void ResetCheckTimerCB(struct ostimer_s *ostimer, void *arg)
{
    //work_t *work = zc_malloc_mem(sizeof(work_t),"ResetCheckTimerProc",MEM_TIME_OUT);
	//work->work = ResetCheckTimerProc;
	//post_app_work(work);
    ResetCheckTimerProc(NULL);
}





int8_t sta_io_ctrl_timer_init(void)
{
    if(STA_IOCtrltimer == NULL)
    {
        STA_IOCtrltimer = timer_create(500,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            STA_IOCtrlTimerCB,
                            NULL,
                            "STA_IOCtrlTimerCB"
                           );
    }

    return 0;
}



int8_t addr_judge_timer_init(void)
{
    if(AddrJudgeTimer == NULL)
    {
        AddrJudgeTimer = timer_create(2000,
                                      0,
                                      TMR_TRIGGER ,//TMR_TRIGGER
                                      AddrJudgeTimerCB,
                                      NULL,
                                      "AddrJudgeTimerCB"
                                      );

    }
    return 0;
}

int8_t plug_check_timer_init(void)
{
    if(PlugCheckTimer == NULL)
    {
        PlugCheckTimer = timer_create(100,
                                      100,
                                      TMR_PERIODIC ,//TMR_TRIGGER
                                      PlugCheckTimerCB,
                                      NULL,
                                      "PlugcheckTimerCB"
                                      );

        timer_start(PlugCheckTimer);
    }

    return 0;
}

int8_t  reset_check_timer_init(void)
{
    #if defined(STA_BOARD_2_1_00)
    int32_t ret;
    
	ret=gpio_request_irq(PLCRST_MAP, GPIO_INTR_TRIGGER_MODE_NEGEDGE, Restprocess, NULL);
	app_printf("Restprocess gpio_request_irq %s\n", ret == OK ? "OK" : "ERROR");
    #endif
    
    if(ResetCheckTimer == NULL)
    {
        ResetCheckTimer	= timer_create(40,
                                       0,
                                       TMR_TRIGGER ,//TMR_TRIGGER
                                       ResetCheckTimerCB,
                                       NULL,
                                       "ResetCheckTimerCB"
                                       );

    }

    return 0;
}

#endif

int8_t power_detect_init()
{
	int32_t ret;
    U8 i;
    U8 ioinfo = 0;
    static U16 checknum = 0;
    static S16 DetectCheckCnt = 0;
        
    for(i = 0;i < 5;i++)
    {
        checknum ++;
        ioinfo = ShakeFilter(READ_DETECT_PIN ,4, &DetectCheckCnt);
        os_sleep(1);
    }
    if(ioinfo == e_IO_LOW)
    {
        POWERDETECTTYPE = e_V3200_STATE;
        app_printf("e_V3200_STATE LOW \n");
    }
    else
    {
        POWERDETECTTYPE = e_V2100_PULSE;
        app_printf("e_V2100_PULSE HIGH %d\n",ioinfo);
    }
    
//os_sleep();
    if(POWERDETECTTYPE == e_V2100_PULSE)
    {
         ret=gpio_request_irq(POWEROFF_MAP,GPIO_INTR_TRIGGER_MODE_NEGEDGE , poweroffdetect, NULL);
    }
    else
    {
        ret=gpio_request_irq(POWEROFF_MAP,GPIO_INTR_TRIGGER_MODE_POSEDGE , poweroffdetect, NULL);
    }
	
	app_printf("poweroffdetect gpio_request_irq %s\n", ret == OK ? "OK" : "ERROR");
    PowerDetectChecktimer = timer_create(
                                 1000,
                                 10,
                                 TMR_PERIODIC,
                                 PowerDetectChecktimerCB,
                                 NULL,
                                 "PowerDetectChecktimerCB"
                                );
    return 0;
}


static void ZeroLostCheckTimerCB(struct ostimer_s *ostimer, void *arg)
{
    /*
    work_t *work = zc_malloc_mem(sizeof(work_t),"ZeroLostCheckTimerProc",MEM_TIME_OUT);
	work->work = ZeroLostCheckTimerProc;
	post_app_work(work);
	*/
	ZeroLostCheckTimerProc(NULL);
}


int8_t  zero_lost_check_timer_init(void)
{
    if(ZeroLostCheckTimer == NULL)
    {
        ZeroLostCheckTimer = timer_create(30,
                                          30,
							              TMR_PERIODIC,
                                          ZeroLostCheckTimerCB,
                                          NULL,
                                          "ZeroLostChecktimerCB"
                                          );
        timer_start(ZeroLostCheckTimer);
    }

    return 0;
}



#endif

static void RebootTimerCB(struct ostimer_s *ostimer, void *arg)
{
	app_printf("appreboot!!!!!!!!!!!!!!!!!!\n");
	app_reboot(500);
}



int8_t  reboot_timer_init(void)
{
    if(RebootTimer == NULL)
    {
        RebootTimer = timer_create(1*1000,
                                0,
                                TMR_TRIGGER ,//TMR_TRIGGER
                                RebootTimerCB,
                                NULL,
                                "RebootTimerCB"
                               );
        //timer_start(RebootTimer);
    }
    
    return 0;
}

int8_t  reboot_after_1s(void)
{
    if(RebootTimer != NULL)
    {
        timer_start(RebootTimer);
    }
    return 0;
}


void app_reboot(U32 Ms)
{
	uint32_t cpu_sr;

	cpu_sr = OS_ENTER_CRITICAL();
	sys_delayms(Ms);
	sys_reboot();
	OS_EXIT_CRITICAL(cpu_sr);
}














