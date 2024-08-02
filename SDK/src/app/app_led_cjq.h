#ifndef __APP_LED_CJQ_H__
#define __APP_LED_CJQ_H__


#if defined(ZC3750CJQ2)
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓外部引用↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

extern gpio_dev_t rxled;


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓宏定义↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

//#define LED_RUN_ON()     gpio_pins_on(&JTAG, RUN_CTRL)		//运行灯
//#define LED_RUN_OFF()    gpio_pins_off(&JTAG, RUN_CTRL)		
#define LED_485_ON()     gpio_pins_on(&JTAG, EN485_CTRL)	//状态灯红色
#define LED_485_OFF()    gpio_pins_off(&JTAG, EN485_CTRL)
//#define LED_RX_ON()		 gpio_pins_on(&rxled, RXLED_CTRL)
//#define LED_RX_OFF()	 gpio_pins_off(&rxled, RXLED_CTRL)


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓枚举↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓结构体↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

typedef struct CJQ_LED_DEV{
	ostimer_t *run_led_timer;
	ostimer_t *rx_led_timer;
	U8 run_led_state;
	U8 rx_led_state;
	U8 city_index;
}__PACKED CJQ_LED_DEV_t;



//ostimer_t *cjq_led_rx_timer;


typedef enum
{
    e_POWER_ON    = 0x01,
    e_SEACHING    = 0x02,
    e_SEACHONE    = 0x04,
    e_SEACHSUCC   = 0x08,
    e_SEACHFAIL   = 0x10,
} CJQ_LED_RUN_STATE_e;

typedef enum
{
    e_ONLINE      = 0x02,
    e_OFFLINE     = 0x04,
    e_EX_CTRL     = 0x08,
} CJQ_LED_RX_STATE_e;



typedef struct
{
    U8					RunLastState;		//运行灯最后状态;
    U8					RxLastState;		//状态灯Rx最后状态;
    LED_ONE_CTRL_t		RunSeaching;		//运行灯搜表状态;
    LED_ONE_CTRL_t		RunSeachOne;		//运行灯搜到一块表状态;
    LED_ONE_CTRL_t		RunSeachSucc;		//运行灯搜表成功；
    LED_ONE_CTRL_t		RunSeachFail;		//运行灯搜表结束；
	LED_ONE_CTRL_t		RxOnline;			//状态灯入网；
    LED_ONE_CTRL_t		RxOffline;			//状态灯未入网；
} __PACKED	CJQ_LED_CTRL_t;


void cjq_led_init();

void cjq_led_rx_ficker();

U8 cjq_led_get_cfg_type(void);

void cjq_led_set_cfg_type(U8 arg_type);

#endif


#endif

