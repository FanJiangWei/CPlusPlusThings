#include "types.h"
#include "gpio.h"
#include "plc_io.h"
#include "app_dltpro.h"
#include "app_exstate_verify.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "dl_mgm_msg.h"
#include "app_event_report_cco.h"

#if defined(ZC3750CJQ2)

static void cjq_led_run_ctrl(U8 arg_state);
static void cjq_led_rx_ctrl(U8 arg_state);

static CJQ_LED_DEV_t gl_cjq_led_t = {
	.run_led_timer = NULL,
	.rx_led_timer = NULL,
	.run_led_state = e_POWER_ON,
	.rx_led_state = e_POWER_ON,
	.city_index = 0,
};

CJQ_LED_CTRL_t cjq_led_def = 
{
	.RxLastState = LED_LOW,
	.RunLastState = LED_LOW,
	
	.RunSeaching.LedState = LED_TURN,
	.RunSeaching.LedOnTime = 1*TIMER_UNITS,
	.RunSeaching.LedOffTime = 1*TIMER_UNITS,

	.RunSeachOne.LedState = LED_TURN,
	.RunSeachOne.LedOnTime = 1*TIMER_UNITS,
	.RunSeachOne.LedOffTime = 1*TIMER_UNITS,
	
	.RunSeachSucc.LedState = LED_TURN,
	.RunSeachSucc.LedOnTime = 1*TIMER_UNITS,
	.RunSeachSucc.LedOffTime = 1*TIMER_UNITS,

	.RunSeachFail.LedState = LED_TURN,
	.RunSeachFail.LedOnTime = 1*TIMER_UNITS,
	.RunSeachFail.LedOffTime = 1*TIMER_UNITS,

	.RxOnline.LedState = LED_LOW,
	.RxOnline.LedOnTime = 1*TIMER_UNITS,
	.RxOnline.LedOffTime = 1*TIMER_UNITS,

	.RxOffline.LedState = LED_TURN,
	.RxOffline.LedOnTime = 1*TIMER_UNITS,
	.RxOffline.LedOffTime = 1*TIMER_UNITS,
};


CJQ_LED_CTRL_t cjq_led_zhe_jiang = 
{
	.RxLastState = LED_LOW,
	.RunLastState = LED_LOW,
	
	.RunSeaching.LedState = LED_TURN,
	.RunSeaching.LedOnTime = 0.1*TIMER_UNITS,
	.RunSeaching.LedOffTime = 0.1*TIMER_UNITS,

	.RunSeachOne.LedState = LED_HIGH,
	.RunSeachOne.LedOnTime = 20*TIMER_UNITS,
	.RunSeachOne.LedOffTime = 0,
	
	.RunSeachSucc.LedState = LED_HIGH,
	.RunSeachSucc.LedOnTime = 1*TIMER_UNITS,
	.RunSeachSucc.LedOffTime = 0,

	.RunSeachFail.LedState = LED_TURN,
	.RunSeachFail.LedOnTime = 1*TIMER_UNITS,
	.RunSeachFail.LedOffTime = 1*TIMER_UNITS,

	.RxOnline.LedState = LED_LOW,
	.RxOnline.LedOnTime = 1*TIMER_UNITS,
	.RxOnline.LedOffTime = 1*TIMER_UNITS,

	.RxOffline.LedState = LED_TURN,
	.RxOffline.LedOnTime = 1*TIMER_UNITS,
	.RxOffline.LedOffTime = 1*TIMER_UNITS,
};


LED_ONE_CTRL_t cjq_led_rx_ex = {
	.LedState = LED_TURN,
	.LedOnTime = 0.1*TIMER_UNITS,
	.LedOffTime = 0.1*TIMER_UNITS,
};


void cjq_led_rx_on(void)
{
	gpio_pins_on(&rxled, RXLED_CTRL);
}

void cjq_led_rx_off(void)
{
	gpio_pins_off(&rxled, RXLED_CTRL);
}

void cjq_led_run_on(void)
{
	gpio_pins_on(&JTAG, RUN_CTRL);
}

void cjq_led_run_off(void)
{
	gpio_pins_off(&JTAG, RUN_CTRL);
}

static void cjq_led_run_timercb(struct ostimer_s *ostimer, void *arg)
{
	cjq_led_run_ctrl(gl_cjq_led_t.run_led_state);
}


static void cjq_led_rx_timercb(struct ostimer_s *ostimer, void *arg)
{
	cjq_led_rx_ctrl(gl_cjq_led_t.rx_led_state);
}


static void cjq_led_rx_ctrl(U8 arg_state)
{
	U32 tp_time = 1*TIMER_UNITS;
	U8  tp_state = GetNodeState();
	
	if(zc_timer_query(gl_cjq_led_t.rx_led_timer) == TMR_RUNNING || gl_cjq_led_t.rx_led_timer == NULL)
	{
		return;
	}

	
	if(arg_state == e_POWER_ON)
	{
		tp_time = 1*TIMER_UNITS;
		gl_cjq_led_t.rx_led_state = e_OFFLINE;
	}
	else if(arg_state == e_ONLINE)
	{
		tp_time = led_set_state(&cjq_led_def.RxOnline, &cjq_led_def.RxLastState,cjq_led_rx_on,cjq_led_rx_off);
		if(tp_state != e_NODE_ON_LINE)
		{
			gl_cjq_led_t.rx_led_state = e_OFFLINE;
		}
	}
	else if(arg_state == e_OFFLINE)
	{
		tp_time = led_set_state(&cjq_led_def.RxOffline, &cjq_led_def.RxLastState,cjq_led_rx_on,cjq_led_rx_off);
		if(tp_state == e_NODE_ON_LINE)
		{
			gl_cjq_led_t.rx_led_state = e_ONLINE;
		}
	}
	else if(arg_state == e_EX_CTRL)
	{
		tp_time = led_set_state(&cjq_led_rx_ex, &cjq_led_def.RxLastState,cjq_led_rx_on,cjq_led_rx_off);
	}
	else
	{
		
	}
	
	led_ctrl_timer_modify(gl_cjq_led_t.rx_led_timer,cjq_led_rx_timercb,tp_time);
}


static void cjq_led_run_ctrl(U8 arg_state)
{
	U32 tp_time = 1*TIMER_UNITS;
	static U8 tp_meter_n = 0;
	U8  tp_seach_state = cjq2_search_meter_info_t.Ctrl.SearchState;
	U8  tp_cjq_meter = CollectInfo_st.MeterNum;
	
	if(zc_timer_query(gl_cjq_led_t.run_led_timer) == TMR_RUNNING || gl_cjq_led_t.run_led_timer == NULL)
	{
		return;
	}
	
	switch(arg_state)
	{
		case e_POWER_ON:
			tp_time = 1*TIMER_UNITS;
			gl_cjq_led_t.run_led_state = e_SEACHING;
			break;
		case e_SEACHING:
			tp_time = led_set_state(&cjq_led_def.RunSeaching, &cjq_led_def.RunLastState, cjq_led_run_on, cjq_led_run_off);
			//搜表结束，搜到表或者搜表满，为搜表成功
			if((tp_seach_state == SEARCH_STATE_STOP && tp_cjq_meter != 0) || tp_seach_state == SEARCH_STATE_FULL)
			{
				gl_cjq_led_t.run_led_state = e_SEACHSUCC;
			}
			else if(tp_seach_state == SEARCH_STATE_STOP && tp_cjq_meter == 0)	//搜表结束，未搜到表，为搜表失败
			{
				gl_cjq_led_t.run_led_state = e_SEACHFAIL;
			}
			else if(tp_meter_n < tp_cjq_meter && tp_seach_state == SEARCH_STATE_TRAVERSE)	//缩位搜表中，当前记录表数量少于搜表数量，
			{
				tp_meter_n = tp_cjq_meter;
				gl_cjq_led_t.run_led_state = e_SEACHONE;
			}
			break;
		case e_SEACHONE:
			tp_time = led_set_state(&cjq_led_def.RunSeachOne, &cjq_led_def.RunLastState, cjq_led_run_on, cjq_led_run_off);
			if(tp_seach_state == SEARCH_STATE_STOP || tp_seach_state == SEARCH_STATE_FULL)
			{
				gl_cjq_led_t.run_led_state = e_SEACHSUCC;
			}
			else
			{
				gl_cjq_led_t.run_led_state = e_SEACHING;
			}
			break;
		case e_SEACHSUCC:
			tp_meter_n = 0;
			tp_time = led_set_state(&cjq_led_def.RunSeachSucc, &cjq_led_def.RunLastState, cjq_led_run_on, cjq_led_run_off);
			if(tp_seach_state != SEARCH_STATE_STOP && tp_seach_state != SEARCH_STATE_FULL)
			{
				gl_cjq_led_t.run_led_state = e_SEACHING;
			}
			break;
		case e_SEACHFAIL:
			tp_meter_n = 0;
			tp_time = led_set_state(&cjq_led_def.RunSeachFail, &cjq_led_def.RunLastState, cjq_led_run_on, cjq_led_run_off);
			if(tp_seach_state != SEARCH_STATE_STOP)
			{
				gl_cjq_led_t.run_led_state = e_SEACHING;
			}
			break;
		default:
			break;
	}

	led_ctrl_timer_modify(gl_cjq_led_t.run_led_timer,cjq_led_run_timercb,tp_time);
}



void cjq_led_timer_init(void)
{
	
    led_ctrl_timer_init(&gl_cjq_led_t.run_led_timer,cjq_led_run_timercb,10);
	led_ctrl_timer_init(&gl_cjq_led_t.rx_led_timer,cjq_led_rx_timercb,10);
	
	cjq_led_run_ctrl(e_POWER_ON);
	cjq_led_rx_ctrl(e_POWER_ON);

	/*
	if(cjq_led_run_timer == NULL)
	{
		cjq_led_run_timer = timer_create(1*TIMER_UNITS,
								  0,
								  TMR_TRIGGER,
								  cjq_led_run_timercb,
								  NULL,
								  "cjq_led_run_timer");
		led_printf("start led run timer\n");
		timer_start(cjq_led_run_timer);
	}
	if(cjq_led_rx_timer == NULL)
	{
		cjq_led_rx_timer = timer_create(1*TIMER_UNITS,
								  0,
								  TMR_TRIGGER,
								  cjq_led_rx_timercb,
								  NULL,
								  "cjq_led_rx_timer");
		led_printf("start led run timer\n");
		timer_start(cjq_led_rx_timer);
	}
	*/
}

void cjq_led_rx_ficker(void)
{
	if(gl_cjq_led_t.rx_led_timer && GetNodeState() == e_NODE_ON_LINE)
	{
		timer_stop(gl_cjq_led_t.rx_led_timer,TMR_NULL);
		cjq_led_rx_ctrl(e_EX_CTRL);
	}
}


static void cjq_led_cnf_type(void)
{
    if(gl_cjq_led_t.city_index == LED_ZHEJIANG)
	{
        __memcpy((U8 *)&cjq_led_def,(U8 *)&cjq_led_zhe_jiang,sizeof(CJQ_LED_CTRL_t));
        led_printf("LED_ZHEJIANG\n");
	}
    else
    {
        led_printf("LED_GW\n");
    }
}

void cjq_led_init(void)
{
	cjq_led_cnf_type();
    cjq_led_timer_init();
}


/*
状态灯红色
LED_485_ON
LED_485_OFF
*/

U8 cjq_led_get_cfg_type(void)
{
	return gl_cjq_led_t.city_index;
}

void cjq_led_set_cfg_type(U8 arg_type)
{
	gl_cjq_led_t.city_index = arg_type;
}


#endif


