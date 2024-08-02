/*
 * Corp:	2018-2026 Beijing Zhongchen Microelectronics Co., LTD.
 * File:	app_led.h
 * Date:	Nov 2, 2022
 * Author:	LuoPengwei
 */


#ifndef __APP_LED_STA_H__
#define __APP_LED_STA_H__
#include "gpio.h"
#include "printf_zc.h"

#if defined(STATIC_NODE)

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓采集器和STA公用↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

#if 1
#define led_printf(fmt, arg...) ({app_printf("[LED]->");app_printf(fmt, ##arg);})
#else
#define led_printf(fmt, arg...)
#endif

#define S(X) (X*TIMER_UNITS)

#define M(X) (60*S(X))

#define H(X) (60*M(X))


typedef enum
{
    LED_TURN  = 0x00,
    LED_HIGH  = 0x01,
    LED_LOW  = 0x02,
}LED_S_e;


typedef struct
{
    U8		LedState;
    U16		LedOnTime;
    U16		LedOffTime;
} __PACKED	LED_ONE_CTRL_t;

typedef enum
{
    LED_GW            = 0x00,
    LED_CHONGQING     = 0X04,
    LED_HEILONGJIANG  = 0x09,
	LED_ZHEJIANG	  = 0x11,
	LED_ANHUI   	  = 0x12,
    LED_SHANDONG      = 0x15,
    LED_MENGDONG      = 0x28,
} LedLightType_e;



#if defined(ZC3750STA)

#if defined(STA_BOARD_3_0_02)
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓外部引用↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

extern gpio_dev_t txled;
extern gpio_dev_t rxled;

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓宏定义↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓枚举↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

typedef enum
{
    e_FIRST_POWER_ON = 0x01,
    e_BSP_METER_NOW  = 0x02,
    e_WAIT_ONLINE    = 0x04,
    e_STATE_ONLINE   = 0x08,
    e_EX_CTRL        = 0x10,
} STALedState_e;


typedef enum
{
	e_V2100_RXLED_CTRL = 0,
	e_V2200_TX_RXLED_CTRL,
} LED_TYPE_e;


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓结构体↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

typedef struct STA_LED_DEV{
	ostimer_t *rx_led_timer;
	ostimer_t *tx_led_timer;
	U8 module_state;
	U8 hardware_type;
	U8 city_index;
}__PACKED STA_LED_DEV_t;


typedef struct
{
    U8					LastRState;
    U8					LastTState;
    LED_ONE_CTRL_t		BspR;
    LED_ONE_CTRL_t		BspT;

    LED_ONE_CTRL_t		OffLineR;
    LED_ONE_CTRL_t		OffLineT;

    LED_ONE_CTRL_t		OnLineR;
    LED_ONE_CTRL_t		OnLineT;
    
} __PACKED	LED_CTRL_t;


typedef struct
{
    LED_ONE_CTRL_t		ExCtrlR;
    LED_ONE_CTRL_t		ExCtrlT;
} __PACKED	LED_EX_CTRL_t;


LED_EX_CTRL_t led_ctrl_ex;		//外部使用

void led_rx_ficker();			//rx灯闪烁

void led_tx_ficker();			//tx灯闪烁

void sta_led_init();			//led初始化

void led_set_module_state(U8 arg_state);	

U8 sta_led_get_cfg_type(void);						//配置参数

void sta_led_set_cfg_type(U8 arg_type);				//配置参数


#endif

#endif

U16 led_set_state(LED_ONE_CTRL_t * arg_led, U8 * arg_last_s, void (*arg_led_on)(), void (*arg_led_off)());

U8 led_ctrl_timer_init(ostimer_t **arg_timer, void(*arg_timercb)(struct ostimer_s *ostimer, void *arg),U16 argtime);

U8 led_ctrl_timer_modify(ostimer_t *arg_timer, void(*arg_timercb)(struct ostimer_s *ostimer, void *arg), U16 arg_time);
#endif

#endif


