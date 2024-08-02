/*******************************************************************************
  * @file        power_manager.c
  * @version     v1.0.0
  * @copyright   ZC Technology, Inc; 2022 
  * @author      wwji
  * @date        2022-7-23
  * @brief
  * @attention
  * Modification History
  * DATE         DESCRIPTION
  * ------------------------
  * - 2022-7-23  wwji Created
*******************************************************************************/
#include "power_manager.h"
#include "phy.h"
#include "gpio.h"
#include "printf_zc.h"
#include "plc_io.h"
#include "wlc.h"
#include "app_cnf.h"
#include "Datalinkdebug.h"
#include "app.h"

#if defined(STATIC_NODE)

#if defined(CHARGE_MANAGER)

ostimer_t	*check_recharge_timer; //检测定时器
ostimer_t	*charge_ctrl_timer; //充电控制定时器
U8  VoltValue = e_IO_PROCESS;
extern gpio_dev_t power_charge;

CHRAGE_CTRL_t ChargeCtrlInfo = {
    .ChargeState = CHARGE_IDLE,
    .ReChargeTime = 0,
    .ExReChargeTime = 0,
};
void recharge_start()
{
    app_printf("recharge_start\n");
    gpio_pins_off(&power_charge, CHARGE_CTRL);
}
void recharge_stop()
{
    app_printf("recharge_stop\n");
    gpio_pins_on(&power_charge, CHARGE_CTRL);
}

void check_recharge_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    static S16 VoltCnt = 0;

    U8 ioinfo = 0;
    static U16 checknum = 0;
    checknum ++;
    
    ioinfo = ShakeFilter(READ_VOLT_PIN , RECHARGE_CHECK_CNT , &VoltCnt);
    VoltValue |= ioinfo;

    if(checknum > (RECHARGE_CHECK_CNT*2))
    {
        checknum = 0;
        VoltCnt = 0;
        timer_stop(check_recharge_timer,TMR_NULL);
        app_printf("VoltValue = %d\n",VoltValue);
    }
    
}
void  check_recharge_timer_init()
{
    if(check_recharge_timer == NULL)
    {
    	check_recharge_timer = timer_create(1,
                                1,
                                TMR_PERIODIC ,//TMR_TRIGGER
                                check_recharge_timer_CB,
                                NULL,
                                "check_recharge_timer_CB"
                               );
    }

}
/*
充电管理
充电开始阶段 上电满电，充电延长固定时间
             上电无电，开始充电
充电过程     超过充电总超时，切换状态，不延长固定时间
         不超过充电总超时，切换状态，延长固定时间
停电过程 12V无电压时，停止充电，过零恢复后，12V恢复后，重新支持充电
*/
void  charge_ctlr_proc()
{
    app_printf("%d\n",ChargeCtrlInfo.ChargeState);
    switch (ChargeCtrlInfo.ChargeState)
    {
        case CHARGE_IDLE:
            VoltValue = e_IO_PROCESS;
            timer_start(check_recharge_timer);
            ChargeCtrlInfo.ChargeState = CHARGE_CHECK;
            break;
            
        case CHARGE_CHECK:
            if(VoltValue == e_IO_HIGH)//充电满电
            {
                ChargeCtrlInfo.ChargeState = CHARGE_EX;
                ChargeCtrlInfo.ReChargeTime = 0;
                ChargeCtrlInfo.ExReChargeTime = 0;
                recharge_start();
            }
            else if(VoltValue == e_IO_LOW)
            {
                ChargeCtrlInfo.ChargeState = CHARGE_ING;
                ChargeCtrlInfo.ReChargeTime = 0;
                ChargeCtrlInfo.ExReChargeTime = 0;
                recharge_start();
            }
            else
            {
                VoltValue = e_IO_PROCESS;
                timer_start(check_recharge_timer);
            }
            break;

        case CHARGE_ING:
            ChargeCtrlInfo.ReChargeTime ++;
            if(VoltValue == e_IO_HIGH)//充电满电
            {
                ChargeCtrlInfo.ChargeState = CHARGE_EX;
                ChargeCtrlInfo.ExReChargeTime = 0;
            }
            else if(VoltValue == e_IO_LOW)
            {
                if(ChargeCtrlInfo.ReChargeTime > RECHARGE_MAX_TIME)//连续充电300秒
                {
                    ChargeCtrlInfo.ChargeState = CHARGE_EX;
                    ChargeCtrlInfo.ExReChargeTime = 0;
                }
                
            }
            VoltValue = e_IO_PROCESS;
            timer_start(check_recharge_timer);
            break;
            
        case CHARGE_EX:
            ChargeCtrlInfo.ExReChargeTime ++;
            if(ChargeCtrlInfo.ExReChargeTime > EX_RECHARGE_MAX_TIME)//饱和充电30秒
            {
                ChargeCtrlInfo.ChargeState = CHARGE_OK;
                recharge_stop();
                VoltValue = e_IO_PROCESS;
                timer_start(check_recharge_timer);
            }
            
            break;
            
        case CHARGE_OK:
            if(VoltValue == e_IO_HIGH)//充电满电
            {
                ChargeCtrlInfo.ChargeState = CHARGE_OK;
            }
            else if(VoltValue == e_IO_LOW)//电流不足
            {
                ChargeCtrlInfo.ChargeState = CHARGE_CHECK;
                
            }
            VoltValue = e_IO_PROCESS;
            timer_start(check_recharge_timer);
            break;
            
        default :
            break;
    }
}
void charge_ctrl_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    charge_ctlr_proc();
}

void charge_ctrl_timer_init()
{
    if(charge_ctrl_timer == NULL)
    {
        charge_ctrl_timer = timer_create(100,
                                100,
                                TMR_PERIODIC,
                                charge_ctrl_timer_CB,
                                NULL,
                                "charge_ctrl_timer_CB"
                               );
    }
    else
    {
        timer_modify(charge_ctrl_timer,100,
				  100,
				  TMR_PERIODIC,
				  charge_ctrl_timer_CB,
				  NULL,
				  "charge_ctrl_timer_CB",
				 TRUE
				 );
    }
    timer_start(charge_ctrl_timer);
}

void    charge_ctrl_init()
{
    charge_ctrl_timer_init();
    check_recharge_timer_init();
    
}

#endif

#endif

#if defined(POWER_MANAGER)

ostimer_t	*lowpower_start_timer = NULL; //低功耗启动定时器
LOW_POWER_CTRL_t LWPR_INFO = 
{   
    .LowPowerStates = FALSE,
    .LowPowerRxStates = LWPR_RX_DUAL,
    .LastRxStates = LWPR_RX_DUAL,
    .RunTime = 0,
};

#if defined(STATIC_NODE)


ostimer_t	*lowpower_ctrl_timer = NULL; //低功耗控制定时器


extern int32_t config_sysclk_lpwr(uint8_t mode);
extern int32_t rf_standby_state(void);

/*******************************************************************************
  * @FunctionName: lowpower_set
  * @Author:       wwji
  * @DateTime:     2022年7月23日T11:33:14+0800
  * @Purpose:      
*******************************************************************************/
int32_t lowpower_set(uint8_t mode)
{
    uint32_t cpu_sr;
    phy_ana_lpwr_t  ana_lpwr;
    phy_bb_lpwr_t   bb_lpwr;

    cpu_sr = OS_ENTER_CRITICAL();

    bb_lpwr = mode;
    phy_ioctl(PHY_CMD_SET_BB_LPWR, &bb_lpwr);

	if (0 == mode) 
    {           
		config_sysclk_lpwr(0);
        ana_lpwr = PHY_ANA_LPWR_LEVEL1;
        phy_ioctl(PHY_CMD_SET_ANA_LPWR, &ana_lpwr);
#if defined(STD_DUAL)
        wphy_set_lpwr(0);
#endif
        //if(IOMAP.chrg_en <= GPIO_MAX_MUXID)
            //gpio_pins_on(NULL, 1 << IOMAP.chrg_en);
	}
    else
    {
        config_sysclk_lpwr(1);
		ana_lpwr = PHY_ANA_LPWR_LEVEL2;	
        phy_ioctl(PHY_CMD_SET_ANA_LPWR, &ana_lpwr);
#if defined(STD_DUAL)
            wphy_set_lpwr(2);
#endif
        //if(IOMAP.chrg_en <= GPIO_MAX_MUXID)
            //gpio_pins_on(NULL, 1 << IOMAP.chrg_en);
	}
    OS_EXIT_CRITICAL(cpu_sr);
    if (0 == mode) 
    {
        app_printf("exit low power mode\n");
    }
    else
    {
        app_printf("enter low power mode\n");
    }

	return 0;
}

/*******************************************************************************
  * @FunctionName: lowpower_ctrl_timer_proc
  * @Author:       wwji
  * @DateTime:     2022年8月18日T16:52:25+0800
  * @Purpose:      
*******************************************************************************/
static void lowpower_ctrl_timer_proc(work_t *work)   
{
    if(TRUE == LWPR_INFO.LowPowerStates)
    {
        if(LWPR_INFO.LowPowerRxStates != LWPR_RX_HRF && LWPR_INFO.LowPowerRxStates != LWPR_RX_HPLC)
        {
            LWPR_INFO.LowPowerRxStates = LWPR_RX_HRF;
            LWPR_INFO.RunTime = 0;
            app_printf("start lowpower\n");
            // wphy_reset();
            // phy_reset();
            hplc_hrf_wphy_reset();
            hplc_hrf_phy_reset();
            //if(phy_get_status() != PHY_STATUS_IDLE)
            {
                //hplc_hrf_phy_reset();
            }
            if(wphy_get_status() == WPHY_STATUS_IDLE)
            {
                wl_do_next();
            }
        }
        LWPR_INFO.RunTime ++;
        if(LWPR_INFO.LowPowerRxStates == LWPR_RX_HRF)
        {
            if(LWPR_INFO.RunTime > LOWPOWER_HRF_RX_TIME)
            {
                LWPR_INFO.LowPowerRxStates = LWPR_RX_HPLC;
                LWPR_INFO.LastRxStates = LWPR_RX_HRF;
                LWPR_INFO.RunTime = 0;
                app_printf("c plc\n");
                rf_standby_state();
                hplc_hrf_wphy_reset();
                // wphy_reset();
                if(phy_get_status() == PHY_STATUS_IDLE)
                {
                    hplc_do_next();
                }
                
                // if(wphy_get_status() != WPHY_STATUS_IDLE)
                // {
                //     wphy_reset();
                // }
            }
            else
            {
                
            }
        }
        else
        {
            if(LWPR_INFO.RunTime > LOWPOWER_HPLC_RX_TIME)
            {
                LWPR_INFO.LowPowerRxStates = LWPR_RX_HRF;
                LWPR_INFO.LastRxStates = LWPR_RX_HPLC;
                LWPR_INFO.RunTime = 0;
                app_printf("c hrf\n");
                if(phy_get_status() != PHY_STATUS_IDLE)
                {
                    hplc_hrf_phy_reset();
                    // phy_reset();
                }
                hplc_hrf_wphy_reset();
                // wphy_reset();
                if(wphy_get_status() == WPHY_STATUS_IDLE)
                {
                    wl_do_next();
                }
            }
            else
            {
                
            }
        }
    }
    else
    {
        LWPR_INFO.LowPowerRxStates = LWPR_RX_DUAL;
        LWPR_INFO.LastRxStates = LWPR_RX_DUAL;
        LWPR_INFO.RunTime = 0;
        hplc_hrf_phy_reset();
        hplc_hrf_wphy_reset();
        // phy_reset();
        // wphy_reset();
        if(phy_get_status() == PHY_STATUS_IDLE)
        {
            hplc_do_next();
        }
        if(wphy_get_status() == WPHY_STATUS_IDLE)
        {
            wl_do_next();
        }
        app_printf("start RX_DUAL\n");
    }
}

/*******************************************************************************
  * @FunctionName: lowpower_ctrl_timer_CB
  * @Author:       wwji
  * @DateTime:     2022年7月30日T11:19:55+0800
  * @Purpose:      修改调度，载波接收和无线接收进行分时控制
*******************************************************************************/
void lowpower_ctrl_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"lowpower_ctrl_timer_proc",MEM_TIME_OUT);
	work->work = lowpower_ctrl_timer_proc;
	work->msgtype = LOWPOWER_PROC;
	post_app_work(work);
}

void lowpower_ctrl_timer_init()
{
    if(lowpower_ctrl_timer == NULL)
    {
        lowpower_ctrl_timer = timer_create(1000,
                                1000,
                                TMR_PERIODIC,
                                lowpower_ctrl_timer_CB,
                                NULL,
                                "lowpower_ctrl_timer_CB"
                               );
    }
}

#endif

/*******************************************************************************
  * @FunctionName: check_lowpower_states_proc
  * @Author:       wwji
  * @DateTime:     2022年7月30日T14:57:59+0800
  * @Purpose:      
  * @param:                           
*******************************************************************************/
void check_lowpower_states_proc()
{
    if(TRUE == LWPR_INFO.LowPowerStates)
    {
        lowpower_stop();
    }
}

static void lowpower_start_timer_proc(work_t *work)   
{
#if defined(STATIC_NODE)
    if(AppNodeState != e_NODE_ON_LINE && HPLC.testmode == NORM)
#else
    if(APP_GETDEVNUM() == 0 && HPLC.testmode == NORM)
#endif
    {
        lowpower_start();
        app_printf("auto start lopower mode\n");
    }


}


void lowpower_start_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"lowpower_ctrl_timer_proc",MEM_TIME_OUT);
	work->work = lowpower_start_timer_proc;
	work->msgtype = LOWPOWER_START;
	post_app_work(work);
}

void lowpower_start_timer_init()
{
    if(lowpower_start_timer == NULL)
    {
        lowpower_start_timer = timer_create(10*60*1000,
                                0,
                                TMR_TRIGGER,
                                lowpower_start_timer_CB,
                                NULL,
                                "lowpower_start_timer_CB"
                               );
    }
}

/*******************************************************************************
  * @FunctionName: lowpower_start
  * @Author:       wwji
  * @DateTime:     2022年7月30日T10:53:43+0800
  * @Purpose:      
*******************************************************************************/
void lowpower_start(void)
{
    LWPR_INFO.LowPowerStates = TRUE;
    app_printf("start lopower mode\n");
#if defined(STATIC_NODE)    
    lowpower_set(1);
    //gpio_pins_off(NULL, LD0_CTRL);
    timer_start(lowpower_ctrl_timer);
#else

#endif
}
/*******************************************************************************
  * @FunctionName: lowpower_stop
  * @Author:       wwji
  * @DateTime:     2022年7月30日T10:53:34+0800
  * @Purpose:      低功耗模式下，入网成功，退出低功耗模式
*******************************************************************************/
void lowpower_stop(void)
{
    LWPR_INFO.LowPowerStates = FALSE;
    app_printf("stop lopower mode\n");
#if defined(STATIC_NODE)
    lowpower_set(0);
    //gpio_pins_on(NULL, LD0_CTRL);
    if(zc_timer_query(lowpower_ctrl_timer) == TMR_RUNNING)
    {
        timer_stop(lowpower_ctrl_timer, TMR_NULL);
    }
    lowpower_ctrl_timer_proc(NULL);
#else

#endif
}



/*******************************************************************************
  * @FunctionName: lowpower_init
  * @Author:       wwji
  * @DateTime:     2022年7月23日T11:59:09+0800
  * @Purpose:      
  * @Describe:     默认10分钟进低功耗模式， 也可以输入app lowpower start命令
                    stop退出低功耗，低功耗模式收到有效报文就退出低功耗模式
                    sta需要按10s,rf和plc交叉启动接收
*******************************************************************************/
void lowpower_init(void)
{
    lowpower_start_timer_init();
#if defined(STATIC_NODE)    
    lowpower_ctrl_timer_init();
#endif

    timer_start(lowpower_start_timer);
    //APP_UPLINK_HANDLE(check_lowpower_states_hook, check_lowpower_states_proc);
}

#endif

