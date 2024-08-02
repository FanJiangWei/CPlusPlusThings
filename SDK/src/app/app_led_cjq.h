#ifndef __APP_LED_CJQ_H__
#define __APP_LED_CJQ_H__


#if defined(ZC3750CJQ2)
/*�������������������������������������������������������������������������ⲿ���á���������������������������������������������������������������������*/

extern gpio_dev_t rxled;


/*�������������������������������������������������������������������������궨�����������������������������������������������������������������������*/

//#define LED_RUN_ON()     gpio_pins_on(&JTAG, RUN_CTRL)		//���е�
//#define LED_RUN_OFF()    gpio_pins_off(&JTAG, RUN_CTRL)		
#define LED_485_ON()     gpio_pins_on(&JTAG, EN485_CTRL)	//״̬�ƺ�ɫ
#define LED_485_OFF()    gpio_pins_off(&JTAG, EN485_CTRL)
//#define LED_RX_ON()		 gpio_pins_on(&rxled, RXLED_CTRL)
//#define LED_RX_OFF()	 gpio_pins_off(&rxled, RXLED_CTRL)


/*������������������������������������������������������������������������ö�١���������������������������������������������������������������������*/


/*�������������������������������������������������������������������������ṹ�����������������������������������������������������������������������*/

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
    U8					RunLastState;		//���е����״̬;
    U8					RxLastState;		//״̬��Rx���״̬;
    LED_ONE_CTRL_t		RunSeaching;		//���е��ѱ�״̬;
    LED_ONE_CTRL_t		RunSeachOne;		//���е��ѵ�һ���״̬;
    LED_ONE_CTRL_t		RunSeachSucc;		//���е��ѱ�ɹ���
    LED_ONE_CTRL_t		RunSeachFail;		//���е��ѱ������
	LED_ONE_CTRL_t		RxOnline;			//״̬��������
    LED_ONE_CTRL_t		RxOffline;			//״̬��δ������
} __PACKED	CJQ_LED_CTRL_t;


void cjq_led_init();

void cjq_led_rx_ficker();

U8 cjq_led_get_cfg_type(void);

void cjq_led_set_cfg_type(U8 arg_type);

#endif


#endif

