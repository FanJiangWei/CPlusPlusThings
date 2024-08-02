/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_exstate_verify.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_EXSTATE_VERIFY_H__
#define __APP_EXSTATE_VERIFY_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"

#if defined(OLD_STA_BOARD)
#define   READ_POWEROFF_CTRL (POWEROFF_CTRL&gpio_input(NULL,POWEROFF_CTRL)?1:0)
#endif

#if defined(ZC3750STA)
#define   READ_PLUG_PIN  ((PLUG_CTRL&gpio_input(NULL,PLUG_CTRL))?1:0)
#define   READ_PLCRST_PIN  ((PLCRST_CTRL&gpio_input(NULL,PLCRST_CTRL))?1:0)
#define   READ_SET_PIN  ((SET_CTRL&gpio_input(NULL,SET_CTRL))?1:0)
#define   READ_STA_PIN  ((STA_CTRL&gpio_input(NULL,STA_CTRL))?1:0)
#define   READ_EVE_PIN  ((GPIO_06&gpio_input(NULL,GPIO_06))?1:0)
#define   READ_RXLED_PIN  ((RXLED_CTRL&gpio_input(NULL,RXLED_CTRL))?1:0)



#endif

#define   READ_EVENT_PIN  ((GPIO_06&gpio_input(NULL,GPIO_06))?1:0)
#define   READ_DETECT_PIN  ((POWEROFF_CTRL&gpio_input(NULL,POWEROFF_CTRL))?1:0)
#define   READ_IO_PIN(pin)  ((pin&gpio_input(NULL,pin))?1:0)

typedef enum
{
	e_V2100_PULSE = 0,
	e_V3200_STATE,
} POWER_DETECT_TYPE_e;


#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)
#define    ZERO_LOST_NUM 20
//#elif defined(OLD_STA_BOARD) //<<<
#else
#define    ZERO_LOST_NUM 5          //旧版本检测逻辑下，检测到5个过零丢失，立马检测det高电平，即可实现检测
#endif

extern ostimer_t *AddrJudgeTimer;
extern ostimer_t *RebootTimer;

extern ostimer_t *g_CheckACDCTimer;
extern void CheckACDCTimerCB(struct ostimer_s *ostimer, void *arg);
#if defined(ZC3750STA)
void STA_on(U16 Times);

#endif
void PowerDetectChecktimerCB(struct ostimer_s *ostimer, void *arg);

void app_reboot(U32 Ms);
int8_t sta_io_ctrl_timer_init(void);

int8_t addr_judge_timer_init(void);

int8_t plug_check_timer_init(void);

int8_t  reset_check_timer_init(void);

void  zero_get_deal(void);

int8_t  reboot_timer_init(void);

int8_t  reboot_after_1s(void);

int8_t  zero_lost_check_timer_init(void);

int8_t  power_detect_init();


#endif

