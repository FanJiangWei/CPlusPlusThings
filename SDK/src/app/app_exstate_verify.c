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


ostimer_t *RebootTimer = NULL;// ������ʱ��


#if defined(STATIC_NODE)

ostimer_t *ZeroLostCheckTimer = NULL;   // ���㶪ʧ��ⶨʱ��

S8 ZeroLostOrGetCnt = 0;


//U8	g_downprotocol = FALSE;


//U8        PlugFlag =	e_plug_in;      //ģ����λ��־
//U8        NeedjugePower =	e_reset_disable;



U8 AddrJudgeFlag = FALSE;         // ���ģ���������ɹ�����˱�־ΪTRUE,������flash���°��

U8 checkflag;

extern ostimer_t *ExStateVerifyTimer;     // �ⲿ״̬��鶨ʱ��





//#if defined(ZC3750STA)

ostimer_t *ResetCheckTimer = NULL;      // ��λ��鶨ʱ��
ostimer_t *PlugCheckTimer = NULL;      // ģ��γ�״̬��鶨ʱ��
ostimer_t *STA_IOCtrltimer = NULL;     // STAģ���STA��״̬������ƶ�ʱ��
ostimer_t *AddrJudgeTimer = NULL;      //�ж��Ƿ���Ҫ���°��

//ostimer_t *g_CheckACDCTimer=NULL;	 //ǿ������



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

__isr__ void Restprocess(void *data)     // ��λIO�жϴ���
{
    app_printf("Restprocess\n");
	//�����˲���ʱ��
	if(TMR_STOPPED == zc_timer_query(ResetCheckTimer) )
	{
		timer_start(ResetCheckTimer);
	}

}


#endif



#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)

__isr__ void poweroffdetect(void *data)   // �������ж�
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
        && DevicePib_t.PowerOffFlag != e_power_off_now)//ͣ����̵Ĺ���
	{
		module_state.vol_detect_state = e_vol_loss;   // ��⵽��Դ����
		
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
static void PowerDetectHigh(void)  // �ڹ��㶪ʧ��鶨ʱ���ص��е��ô˺���
{
    static S16 PowerCtrlhighNum = 0;
    U8 ioinfo = 0;
    ioinfo = ShakeFilter(READ_POWEROFF_CTRL , (IO_CHECK_TIME/2) , &PowerCtrlhighNum);  // ���IO״̬��ȥ��
    if(ioinfo == e_IO_HIGH)
	{
	    if(module_state.vol_detect_state == e_vol_normal
               && DevicePib_t.PowerOffFlag != e_power_off_now)//ͣ����̵Ĺ���
    	{
    		module_state.vol_detect_state = e_vol_loss;   // ��⵽��Դ����
//    		printf_s("*************************2 poweroffdetect\n");
    		
    	}
    }
    else
    {
        module_state.vol_detect_state = e_vol_normal;   // δ��⵽��Դ����
        app_printf("Pl!\n");
    }
    
}
#endif

#if defined(ZC3750STA)
static void PlugCheckTimerProc(work_t *work)
{
    static S16 PlugCnt = 0;
    static U8  PlugCheck = 0;

    if(READ_PLUG_PIN)   //�γ�״̬
    {		
        PlugCnt = PlugCnt>0?0:PlugCnt; // ״̬Ǩ��

        PlugCnt = PlugCnt>-6?PlugCnt-1:PlugCnt; // ״̬����

		
        if(PlugCnt<=-6 && e_plug_in == module_state.plug_state)
		{	
		    //�״��ϵ��ж��ǲ���PLUGģʽ;����Ǵ�ͣ��ģ�飬�γ�״̬�����������
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
                
    			module_state.plug_state = e_plug_out;//����������������ʱ������⵽�γ�״̬
    			
    			//�����ܼ�鶨ʱ����δ��������������
    			if(zc_timer_query(exstate_verify_timer) == TMR_STOPPED)
                {
                    timer_start(exstate_verify_timer);
                }
            }
		}
	}
	else//����״̬
	{
		PlugCnt = PlugCnt<0?0:PlugCnt; // ״̬Ǩ��
		
		PlugCnt = PlugCnt<6?PlugCnt+1:PlugCnt;//״̬����
		//�״��ϵ��ж��ǲ���״̬���������ټ��
        if(PlugCnt>=6 && PlugCheck == 0)
        {
            PlugCheck = 2;
            module_state.power_type = e_with_power;
            module_state.plug_state = e_plug_in;
        }
		if(PlugCnt>=6&&e_plug_out==module_state.plug_state)
		{		
			app_printf("Sta is inserted!\n");
            //����Ǵ�ͣ��ģ�飬����״̬����´�ͣ��ģʽ
            if(PlugCheck == 1)
            {
                module_state.power_type = e_with_power;
            }
			module_state.plug_state = e_plug_in;//����������������ʱ������⵽����״̬
			//��Ҫ�������
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
    //200ms�˲����Ե͵�ƽ��RSTΪ��
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


void zero_get_deal(void)  // �ڹ����ж��б����ã��������ж�
{
    ZeroLostOrGetCnt = ZeroLostOrGetCnt>0?0: ZeroLostOrGetCnt ;//״̬Ǩ��

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
			//����״̬���޹��㵽�й���
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

    // ��ÿһ�ι����ж���ˢ�����������жϼ�鶨ʱ����ʹ�˶�ʱ�����ܳ�ʱ�ص�    
    if(ZeroLostCheckTimer)
    {
        timer_start(ZeroLostCheckTimer);
    }
    

}


static void ZeroLostCheckTimerProc(work_t *work)   // ���㶪ʧ�ж�
{  
    
    ZeroLostOrGetCnt = ZeroLostOrGetCnt<0?0: ZeroLostOrGetCnt;

	ZeroLostOrGetCnt = (ZeroLostOrGetCnt >= ZERO_LOST_NUM)?ZeroLostOrGetCnt: ZeroLostOrGetCnt+1;
    
    if(ZeroLostOrGetCnt >= ZERO_LOST_NUM)
    {
        //������
        if(module_state.zero_cross_state == e_zc_get)
		{
			module_state.zero_cross_state = e_zc_lost;  // ���㶪ʧ

            app_printf("------------------------lost zero cross----------------------\n");

			//�й��㵽�޹���
			timer_stop(g_CheckACDCTimer,TMR_NULL);//���㶪ʧ�󣬹ر�ǿ����
			if(zc_timer_query(exstate_verify_timer) == TMR_STOPPED)
            {
                timer_start(exstate_verify_timer);  // �����ⲿ״̬���
            }
		}
        else
        {
#if defined(OLD_STA_BOARD)
//�ɰ汾����߼��£���⵽5�����㶪ʧ��������det�ߵ�ƽ������ʵ�ּ��
            PowerDetectHigh();  // ��5�������м���Դ������IO�ߵ�ƽ
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
        if(AddrJudgeFlag == FALSE)//��ʶ��Ҫ���°���������ʱ��
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
        if(sta_bind_packet_send_info(DevicePib_t.BandMeterCnt,sendbuf, &buflen) == OK)  // ���
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














