/*
 * Corp:	2018-2026 Beijing Zhongchen Microelectronics Co., LTD.
 * File:	app_led.c
 * Date:	Nov 2, 2022
 * Author:	LuoPengwei
 */


#include "types.h"
#include "gpio.h"
#include "plc_io.h"
#include "app_dltpro.h"
#include "app_exstate_verify.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "app_event_report_cco.h"
#include "SchTask.h"

#if defined(STATIC_NODE)

//static U8  gl_led_cfg_type = 0;		//从flash获取灯语配置



#if defined(ZC3750STA)

#if defined(STA_BOARD_3_0_02)

static void sta_led_rx_ctrl(U8 arg_state);
static void sta_led_tx_ctrl(U8 arg_state);

static STA_LED_DEV_t gl_sta_led_t = {
	.rx_led_timer = NULL,
	.tx_led_timer = NULL,
	.module_state = e_FIRST_POWER_ON,
	.hardware_type = e_V2100_RXLED_CTRL,
	.city_index = 0,
};

/*
U8 gl_led_hard_type;					//模块硬件版本

U8 gl_led_state = e_FIRST_POWER_ON;		//模块状态

ostimer_t *sta_led_rx_ctrl_timer = NULL;	//模块TX灯语控制定时器
ostimer_t *sta_led_tx_ctrl_timer = NULL;	//模块RX灯语控制定时器
*/


static LED_CTRL_t gl_led_ctrl_def = 
{
    .BspR.LedState = LED_TURN,
    .BspR.LedOnTime = 0.5*TIMER_UNITS,
    .BspR.LedOffTime = 0.5*TIMER_UNITS,
    .BspT.LedState = LED_LOW,
    .BspT.LedOnTime = 0,
    .BspT.LedOffTime = 0,

    .OffLineR.LedState = LED_TURN,
    .OffLineR.LedOnTime = 3*TIMER_UNITS,
    .OffLineR.LedOffTime = 1*TIMER_UNITS,
    .OffLineT.LedState = LED_LOW,
    .OffLineT.LedOnTime = 0.5*TIMER_UNITS,
    .OffLineT.LedOffTime = 0.5*TIMER_UNITS,
    
    .OnLineR.LedState = LED_LOW,
    .OnLineR.LedOnTime = 0.02*TIMER_UNITS,
    .OnLineR.LedOffTime = 0,
    .OnLineT.LedState = LED_LOW,
    .OnLineT.LedOnTime = 0.01*TIMER_UNITS,
    .OnLineT.LedOffTime = 0,
};

static LED_CTRL_t sta_led_meng_dong = 
{
	.BspR.LedState = LED_TURN,
	.BspR.LedOnTime = 0.5*TIMER_UNITS,
	.BspR.LedOffTime = 0.5*TIMER_UNITS,
	.BspT.LedState = LED_LOW,
	.BspT.LedOnTime = 0,
	.BspT.LedOffTime = 0,

	.OffLineR.LedState = LED_LOW,
	.OffLineR.LedOnTime = 0,
	.OffLineR.LedOffTime = 0.02*TIMER_UNITS,
	.OffLineT.LedState = LED_LOW,
	.OffLineT.LedOnTime = 0,
	.OffLineT.LedOffTime = 0.01*TIMER_UNITS,
	
	.OnLineR.LedState = LED_HIGH,
	.OnLineR.LedOnTime = 0.02*TIMER_UNITS,
	.OnLineR.LedOffTime = 0,
	.OnLineT.LedState = LED_HIGH,
	.OnLineT.LedOnTime = 0.01*TIMER_UNITS,
	.OnLineT.LedOffTime = 0,
};

static LED_CTRL_t sta_led_shan_dong = 
{
    .BspR.LedState = LED_TURN,
    .BspR.LedOnTime = 0.5*TIMER_UNITS,
    .BspR.LedOffTime = 0.5*TIMER_UNITS,
    .BspT.LedState = LED_LOW,
    .BspT.LedOnTime = 0,
    .BspT.LedOffTime = 0,

    .OffLineR.LedState = LED_LOW,
    .OffLineR.LedOnTime = 0,
    .OffLineR.LedOffTime = 0,
    .OffLineT.LedState = LED_TURN,
    .OffLineT.LedOnTime = 1*TIMER_UNITS,
    .OffLineT.LedOffTime = 1*TIMER_UNITS,
    
    .OnLineR.LedState = LED_LOW,
    .OnLineR.LedOnTime = 0.02*TIMER_UNITS,
    .OnLineR.LedOffTime = 0,
    .OnLineT.LedState = LED_LOW,
    .OnLineT.LedOnTime = 0.01*TIMER_UNITS,
    .OnLineT.LedOffTime = 0,
};

static LED_CTRL_t sta_led_hei_long_jiang = 
{
    .BspR.LedState = LED_TURN,
    .BspR.LedOnTime = 1*TIMER_UNITS,
    .BspR.LedOffTime = 1*TIMER_UNITS,
    .BspT.LedState = LED_TURN,
    .BspT.LedOnTime = 1*TIMER_UNITS,
    .BspT.LedOffTime = 1*TIMER_UNITS,

    .OffLineR.LedState = LED_LOW,
    .OffLineR.LedOnTime = 0,
    .OffLineR.LedOffTime = 0,
    .OffLineT.LedState = LED_TURN,
    .OffLineT.LedOnTime = 1*TIMER_UNITS,
    .OffLineT.LedOffTime = 1*TIMER_UNITS,
    
    .OnLineR.LedState = LED_LOW,
    .OnLineR.LedOnTime = 0.02*TIMER_UNITS,
    .OnLineR.LedOffTime = 0,
    .OnLineT.LedState = LED_LOW,
    .OnLineT.LedOnTime = 0.01*TIMER_UNITS,
    .OnLineT.LedOffTime = 0,
};

static LED_CTRL_t sta_led_zhe_jiang = 
{
    .BspR.LedState = LED_TURN,
    .BspR.LedOnTime = 1*TIMER_UNITS,
    .BspR.LedOffTime = 1*TIMER_UNITS,
    .BspT.LedState = LED_TURN,
    .BspT.LedOnTime = 0.1*TIMER_UNITS,
    .BspT.LedOffTime = 0.1*TIMER_UNITS,
    
    .OffLineR.LedState = LED_TURN,
    .OffLineR.LedOnTime = 1*TIMER_UNITS,
    .OffLineR.LedOffTime = 1*TIMER_UNITS,
    .OffLineT.LedState = LED_HIGH,
    .OffLineT.LedOnTime = 0,
    .OffLineT.LedOffTime = 0,
    
    .OnLineR.LedState = LED_HIGH,
    .OnLineR.LedOnTime = 0,
    .OnLineR.LedOffTime = 0,
    .OnLineT.LedState = LED_HIGH,
    .OnLineT.LedOnTime = 0,
    .OnLineT.LedOffTime = 0,
};

static LED_CTRL_t sta_led_an_hui = 
{
    .BspR.LedState = LED_LOW,
    .BspR.LedOnTime = 2*TIMER_UNITS,
    .BspR.LedOffTime = 2*TIMER_UNITS,
    .BspT.LedState = LED_TURN,
    .BspT.LedOnTime = 1*TIMER_UNITS,
    .BspT.LedOffTime = 1*TIMER_UNITS,
    
    .OffLineR.LedState = LED_TURN,
    .OffLineR.LedOnTime = 1*TIMER_UNITS,
    .OffLineR.LedOffTime = 1*TIMER_UNITS,
    .OffLineT.LedState = LED_LOW,
    .OffLineT.LedOnTime = 1*TIMER_UNITS,
    .OffLineT.LedOffTime = 1*TIMER_UNITS,
    
    .OnLineR.LedState = LED_LOW,
    .OnLineR.LedOnTime = 0,
    .OnLineR.LedOffTime = 0,
    .OnLineT.LedState = LED_LOW,
    .OnLineT.LedOnTime = 0,
    .OnLineT.LedOffTime = 0,
};

LED_EX_CTRL_t led_ctrl_ex = 
{
    .ExCtrlR.LedState = LED_TURN,
    .ExCtrlR.LedOnTime = 0.1*TIMER_UNITS,
    .ExCtrlR.LedOffTime = 0.1*TIMER_UNITS,
    .ExCtrlT.LedState = LED_TURN,
    .ExCtrlT.LedOnTime = 0.1*TIMER_UNITS,
    .ExCtrlT.LedOffTime = 0.1*TIMER_UNITS,
};



static void led_rx_on(void)
{
    //if(gl_led_hard_type == e_V2200_TX_RXLED_CTRL)
	if(gl_sta_led_t.hardware_type == e_V2200_TX_RXLED_CTRL)
    {
	    gpio_pins_off(&rxled, RXLED_CTRL);
    }
    else
    {
        gpio_pins_on(&rxled, RXLED_CTRL);
    }
}

static void led_rx_off(void)
{
    //if(gl_led_hard_type == e_V2200_TX_RXLED_CTRL)
	if(gl_sta_led_t.hardware_type == e_V2200_TX_RXLED_CTRL)
    {
        gpio_pins_on(&rxled, RXLED_CTRL);
    }
    else
    {
        gpio_pins_off(&rxled, RXLED_CTRL);
    }
}

static void led_tx_on(void)
{
    //if(gl_led_hard_type == e_V2200_TX_RXLED_CTRL)
    if(gl_sta_led_t.hardware_type == e_V2200_TX_RXLED_CTRL)
    {
        gpio_pins_on(&txled, TXLED_CTRL);
    }
    else
    {
        //2.1.00不可控
    }
}

static void led_tx_off(void)
{
	//if(gl_led_hard_type == e_V2200_TX_RXLED_CTRL)
	if(gl_sta_led_t.hardware_type == e_V2200_TX_RXLED_CTRL)
    {
        gpio_pins_off(&txled, TXLED_CTRL);
    }
    else
    {
        //2.1.00不可控
    }
}


static void sta_led_rx_ctrl_timercb(struct ostimer_s *ostimer, void *arg)
{
	//sta_led_rx_ctrl(gl_led_state);
	sta_led_rx_ctrl(gl_sta_led_t.module_state);
}

static void sta_led_tx_ctrl_timercb(struct ostimer_s *ostimer, void *arg)
{
	//sta_led_tx_ctrl(gl_led_state);
	sta_led_tx_ctrl(gl_sta_led_t.module_state);
}







static void sta_led_tx_ctrl(U8 arg_state)
{
	U32 tp_time = 1*TIMER_UNITS;

	//if(sta_led_tx_ctrl_timer == NULL)
	if(gl_sta_led_t.tx_led_timer == NULL)
	{
		return ;
	}
	
	//if(TMR_RUNNING == zc_timer_query(sta_led_tx_ctrl_timer))
	if(TMR_RUNNING == zc_timer_query(gl_sta_led_t.tx_led_timer))
	{
		return;
	}

	if(arg_state == e_FIRST_POWER_ON)
	{
		led_tx_on();
		gl_led_ctrl_def.LastTState = LED_HIGH;
		if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)//黑龙江灯语要求
		{
			tp_time = 3*TIMER_UNITS;
		}
		gl_sta_led_t.module_state = e_BSP_METER_NOW;
		//gl_led_state = e_BSP_METER_NOW;
	}
	else if(arg_state == e_BSP_METER_NOW)
	{
		tp_time = led_set_state(&gl_led_ctrl_def.BspT, &gl_led_ctrl_def.LastTState, led_tx_on, led_tx_off);
		if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
		{
			gl_led_ctrl_def.LastRState = gl_led_ctrl_def.LastTState == LED_HIGH?LED_LOW:LED_HIGH;
			//(gl_sta_led_t.rx_led_timer,sta_led_rx_ctrl_timercb,tp_time);
		}
		if(TMR_RUNNING == zc_timer_query(sta_bind_timer))
		{
			//gl_led_state = e_BSP_METER_NOW;
			gl_sta_led_t.module_state = e_BSP_METER_NOW;
		}
		else
		{
			//gl_led_state = e_WAIT_ONLINE;
			gl_sta_led_t.module_state = e_WAIT_ONLINE;
		}
	}
	else if(arg_state == e_WAIT_ONLINE)
	{
		tp_time = led_set_state(&gl_led_ctrl_def.OffLineT, &gl_led_ctrl_def.LastTState, led_tx_on, led_tx_off);
		if(CheckSTAOnlineSate() == TRUE)
		{
			//gl_led_state = e_STATE_ONLINE;
			gl_sta_led_t.module_state = e_STATE_ONLINE;
		}
		else
		{
			//gl_led_state = e_WAIT_ONLINE;
			gl_sta_led_t.module_state = e_WAIT_ONLINE;
		}
	}
	else if(arg_state == e_STATE_ONLINE)
	{
		tp_time = led_set_state(&gl_led_ctrl_def.OnLineT, &gl_led_ctrl_def.LastTState, led_tx_on, led_tx_off);
		if(CheckSTAOnlineSate() == TRUE)
		{
			//gl_led_state = e_STATE_ONLINE;
			gl_sta_led_t.module_state = e_STATE_ONLINE;
		}
		else
		{
			//gl_led_state = e_WAIT_ONLINE;
			gl_sta_led_t.module_state = e_WAIT_ONLINE;
		}
	}
    else if(arg_state == e_EX_CTRL)
	{
		tp_time = led_set_state(&led_ctrl_ex.ExCtrlT, &gl_led_ctrl_def.LastTState, led_tx_on, led_tx_off);
	}
	else 
	{
		
	}
	
	if(tp_time > 0)
	{
		led_ctrl_timer_modify(gl_sta_led_t.tx_led_timer,sta_led_tx_ctrl_timercb,tp_time);
		/*
		timer_modify(sta_led_tx_ctrl_timer,
					tp_time, 
				    0,
				    TMR_TRIGGER,
				    sta_led_tx_ctrl_timercb,
				    NULL,
                    "sta_led_tx_ctrl_timer",
                    TRUE);
	    timer_start(sta_led_tx_ctrl_timer);
	    */
	}
}



static void sta_led_rx_ctrl(U8 arg_state)
{
	U32 tp_time = 0;

	//if(sta_led_rx_ctrl_timer == NULL)
	if(gl_sta_led_t.rx_led_timer == NULL)
	{
		return ;
	}
	
	//if(TMR_RUNNING == zc_timer_query(sta_led_rx_ctrl_timer))
	if(TMR_RUNNING == zc_timer_query(gl_sta_led_t.rx_led_timer))
	{
		return;
	}

	if(arg_state == e_FIRST_POWER_ON)
	{
		led_rx_on();
		gl_led_ctrl_def.LastRState = LED_HIGH;
		if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)//黑龙江灯语要求
		{
        	tp_time = 3*TIMER_UNITS;
		}
		else
		{
			tp_time = 1*TIMER_UNITS;
		}
	}
	else if(arg_state == e_BSP_METER_NOW)
	{
		tp_time = led_set_state(&gl_led_ctrl_def.BspR, &gl_led_ctrl_def.LastRState, led_rx_on, led_rx_off);
	}
	else if(arg_state == e_WAIT_ONLINE)
	{
		tp_time = led_set_state(&gl_led_ctrl_def.OffLineR, &gl_led_ctrl_def.LastRState, led_rx_on, led_rx_off);
	}
	else if(arg_state == e_STATE_ONLINE)
	{
		tp_time = led_set_state(&gl_led_ctrl_def.OnLineR, &gl_led_ctrl_def.LastRState, led_rx_on, led_rx_off);
	}
    else if(arg_state == e_EX_CTRL)
	{
		tp_time = led_set_state(&led_ctrl_ex.ExCtrlR, &gl_led_ctrl_def.LastRState, led_rx_on, led_rx_off);
	}
	else 
	{
	}
	
	if(tp_time > 0)
	{
		led_ctrl_timer_modify(gl_sta_led_t.rx_led_timer,sta_led_rx_ctrl_timercb,tp_time);
		/*
		timer_modify(sta_led_rx_ctrl_timer,
					tp_time, 
				    0,
				    TMR_TRIGGER,
				    sta_led_rx_ctrl_timercb,
				    NULL,
                    "sta_led_rx_ctrl_timer",
                    TRUE);
	    timer_start(sta_led_rx_ctrl_timer);
	    */
	}
}


void led_rx_ficker(void)
{	
	if(PROVINCE_ANHUI == app_ext_info.province_code)
	{
		return;
	}
	if(PROVINCE_HEILONGJIANG == app_ext_info.province_code && e_BSP_METER_NOW == gl_sta_led_t.module_state)//黑龙江绑表状态收发载波帧灯不亮
	{
		return;
	}
	//timer_stop(sta_led_rx_ctrl_timer,TMR_NULL);
	timer_stop(gl_sta_led_t.rx_led_timer,TMR_NULL);
	sta_led_rx_ctrl(e_EX_CTRL);
}


void led_tx_ficker(void)
{
	if(PROVINCE_ANHUI == app_ext_info.province_code)
	{
		return;
	}
	if(PROVINCE_HEILONGJIANG == app_ext_info.province_code && e_BSP_METER_NOW == gl_sta_led_t.module_state)//黑龙江绑表状态收发载波帧灯不亮
	{
		return;
	}
	//timer_stop(sta_led_tx_ctrl_timer,TMR_NULL);
	timer_stop(gl_sta_led_t.tx_led_timer,TMR_NULL);
	sta_led_tx_ctrl(e_EX_CTRL);
}

void led_set_module_state(U8 arg_state)
{
	gl_sta_led_t.module_state = arg_state;
}


/*
static U8 led_ctrl_timer_init()
{
	if(sta_led_rx_ctrl_timer == NULL)
	{
		sta_led_rx_ctrl_timer = timer_create(1*TIMER_UNITS,
						 0,
						 TMR_TRIGGER,
						 sta_led_rx_ctrl_timercb,
						 NULL,
						 "led_r_ctrl_timercb"
						);
	}
	
	if(sta_led_tx_ctrl_timer == NULL)
	{
		sta_led_tx_ctrl_timer = timer_create(1*TIMER_UNITS,
						 0,
						 TMR_TRIGGER,
						 sta_led_tx_ctrl_timercb,
						 NULL,
						 "led_t_ctrl_timercb"
						);
	}
	timer_start(sta_led_tx_ctrl_timer);
	timer_start(sta_led_rx_ctrl_timer);
	sta_led_tx_ctrl(e_FIRST_POWER_ON);
	return 0;
}
*/
static void sta_led_cnf_type(void)
{
	if(gl_sta_led_t.city_index == LED_HEILONGJIANG)
	{
		__memcpy((U8 *)&gl_led_ctrl_def,(U8 *)&sta_led_hei_long_jiang,sizeof(LED_CTRL_t));
		led_printf("LED_HEILONGJIANG\n");
	}
	else if(gl_sta_led_t.city_index == LED_SHANDONG || gl_sta_led_t.city_index == LED_CHONGQING)
	{
		__memcpy((U8 *)&gl_led_ctrl_def,(U8 *)&sta_led_shan_dong,sizeof(LED_CTRL_t));
		led_printf("LED_SHANDONG\n");
	}
	else if(gl_sta_led_t.city_index == LED_MENGDONG)
	{
		__memcpy((U8 *)&gl_led_ctrl_def,(U8 *)&sta_led_meng_dong,sizeof(LED_CTRL_t));
		led_printf("LED_MENGDONG\n");
	}	
	else if(gl_sta_led_t.city_index == LED_ZHEJIANG)
	{
		__memcpy((U8 *)&gl_led_ctrl_def,(U8 *)&sta_led_zhe_jiang,sizeof(LED_CTRL_t));
		led_printf("LED_ZHEJIANG\n");
	}
	else if(gl_sta_led_t.city_index == LED_ANHUI)
	{
		__memcpy((U8 *)&gl_led_ctrl_def,(U8 *)&sta_led_an_hui,sizeof(LED_CTRL_t));
		led_printf("LED_ANHUI\n");
	}
	else
	{
		led_printf("LED_GW\n");
	}

}


void sta_led_init(void)
{
    gpio_set_dir(&rxled, RXLED_CTRL, GPIO_OUT);
    gpio_pins_off(&rxled, RXLED_CTRL);
    sys_delayms(100);

    gpio_set_dir(&rxled, RXLED_CTRL, GPIO_IN);
    sys_delayms(100);
    U8 i;
    U8 rx_ioinfo = 0;
    static U16 checknum = 0;
    static S16 rx_DetectCheckCnt = 0;
        
    for(i = 0;i < 5;i++)
    {
        checknum ++;
        rx_ioinfo = ShakeFilter(READ_RXLED_PIN ,4, &rx_DetectCheckCnt);
        os_sleep(1);
    }
	
    if(rx_ioinfo == e_IO_LOW)
    {
        //gl_led_hard_type = e_V2100_RXLED_CTRL;
        gl_sta_led_t.hardware_type = e_V2100_RXLED_CTRL;
        led_printf("RXLED_CTRL LOW \n");
    }
    else
    {
        //gl_led_hard_type = e_V2200_TX_RXLED_CTRL;
        gl_sta_led_t.hardware_type = e_V2200_TX_RXLED_CTRL;
        led_printf("RXLED_CTRL HIGH \n");
    }
	/*
    if(tx_ioinfo == e_IO_LOW)
    {
        led_printf("TXLED_CTRL LOW \n");
    }
    else
    {
        led_printf("TXLED_CTRL HIGH \n");
    }
	*/
    gpio_set_dir(&rxled, RXLED_CTRL, GPIO_OUT);
    gpio_set_dir(&txled, TXLED_CTRL, GPIO_OUT);

	gl_sta_led_t.module_state = e_FIRST_POWER_ON;
    led_ctrl_timer_init(&gl_sta_led_t.rx_led_timer,sta_led_rx_ctrl_timercb,10);
	led_ctrl_timer_init(&gl_sta_led_t.tx_led_timer,sta_led_tx_ctrl_timercb,20);
	
	sta_led_cnf_type();
}

U8 sta_led_get_cfg_type(void)
{
	return gl_sta_led_t.city_index;
}

void sta_led_set_cfg_type(U8 arg_type)
{
	gl_sta_led_t.city_index = arg_type;
}




#endif

#endif






/*
 * led_set_state - 控制灯的点亮、熄灭
 * @arg_led: 		灯状态，点亮时间、熄灭时间、以及是点亮还是熄灭还是翻转
 * @arg_last_s: 	灯最后状态
 * @arg_led_on: 	灯点亮函数
 * @arg_led_off：	灯熄灭函数
 * @return：			当前状态灯时长
 */

U16 led_set_state(LED_ONE_CTRL_t * arg_led, U8 * arg_last_s, void (*arg_led_on)(), void (*arg_led_off)())
{
	U16 tp_time;
	
	if((arg_led->LedState == LED_TURN && *arg_last_s == LED_LOW)||arg_led->LedState == LED_HIGH)
	{
		*arg_last_s = LED_HIGH;
        arg_led_on();
		tp_time = arg_led->LedOnTime;
	}
	else
	{
		*arg_last_s = LED_LOW;
        arg_led_off();
		tp_time = arg_led->LedOffTime;
	}

	tp_time = tp_time == 0 ? (1*TIMER_UNITS) : tp_time;
	
	return tp_time;
}


U8 led_ctrl_timer_init(ostimer_t **arg_timer, void(*arg_timercb)(struct ostimer_s *ostimer, void *arg),U16 argtime)
{
	*arg_timer = timer_create(argtime,
					 0,
					 TMR_TRIGGER,
					 arg_timercb,
					 NULL,
					 "sta_led_timer");

	timer_start(*arg_timer);
	return 0;
}

U8 led_ctrl_timer_modify(ostimer_t *arg_timer, void(*arg_timercb)(struct ostimer_s *ostimer, void *arg), U16 arg_time)
{
	timer_modify(arg_timer,
					arg_time,
					0,
					TMR_TRIGGER,
					arg_timercb,
					NULL,
					"sta_led_timer",
					TRUE);
	timer_start(arg_timer);
	return 0;
}


#endif


