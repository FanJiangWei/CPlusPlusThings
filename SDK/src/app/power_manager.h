#ifndef __POWER_MANAGER_H__
#define __POWER_MANAGER_H__
#include "types.h"
#include "mbuf.h"
#include "ZCsystem.h"
#include "version.h"

#if defined(STATIC_NODE)

#if defined(CHARGE_MANAGER)

#if defined(STA_BOARD_TJ2_5_0_00)
#define CHARGE_CTRL	    (GPIO_02)
#define CHARGE_MAP		(GPIO_02_MUXID)

#define VOLTDET_CTRL	    (GPIO_10)
#define VOLTDET_MAP		(GPIO_10_MUXID)

#endif

#define    RECHARGE_CHECK_CNT       40      //10ms
#define    RECHARGE_MAX_TIME        300     //1s
#define    EX_RECHARGE_MAX_TIME     30      //1s

#define   READ_VOLT_PIN  ((VOLTDET_CTRL&gpio_input(NULL,VOLTDET_CTRL))?1:0)


typedef enum
{
    CHARGE_IDLE     ,
    CHARGE_CHECK    ,
    CHARGE_ING      ,
    CHARGE_EX       ,
    CHARGE_OK       ,
    
} CHARGE_STATE_e;

typedef struct
{

    U8		ChargeState;        //
    U32     ReChargeTime;       //充电时间
    U32		ExReChargeTime;     //等待充电完成之后，延长等待时间
	
} __PACKED	CHRAGE_CTRL_t;

void    charge_ctrl_init();

#endif

#endif  //#if defined(STATIC_NODE)

#if defined(POWER_MANAGER)

#define LOWPOWER_HRF_RX_TIME	    (5)    //unit:1s
#define LOWPOWER_HPLC_RX_TIME	    (5)    //unit:1s

typedef enum
{
    LWPR_RX_IDLE   ,
    LWPR_RX_HRF    ,
    LWPR_RX_HPLC   ,
    LWPR_RX_DUAL   ,
    
} LWPR_RX_STATE_e;

typedef struct
{
    U8      LowPowerStates;         //低功耗状态
    U8		LowPowerRxStates;       //当前接收状态
    U8      LastRxStates;           //上一次接收状态
    U8		RunTime;                //当前接收状态持续时间
	
} __PACKED	LOW_POWER_CTRL_t;

extern LOW_POWER_CTRL_t LWPR_INFO;
extern void check_lowpower_states_proc();

extern int32_t lowpower_set(uint8_t mode);
void lowpower_start(void);
void lowpower_stop(void);
extern void lowpower_init(void);


#endif  //#if defined(POWER_MANAGER)

#endif /* __CHARGE_MANAGER_H__ */
