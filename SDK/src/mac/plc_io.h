#ifndef PLC_IO_H
#define PLC_IO_H

#include "gpio.h"
#if defined(ZC3750STA)                            //STA

#if defined(UNICORN2M)
#define STA_CTRL		(GPIO_00)
#define STA_MAP		    (GPIO_00_MUXID)
#define LD0_CTRL		(GPIO_02)
#define LD0_MAP		    (GPIO_02_MUXID)
#elif defined(ROLAND1_1M)
#define STA_CTRL		(GPIO_08)
#define STA_MAP		    (GPIO_08_MUXID)
#define LD0_CTRL		(GPIO_09)
#define LD0_MAP		    (GPIO_09_MUXID)
#endif

#if defined(ROLAND1M) || defined(UNICORN2M)
#define SET_CTRL		(RMII_RXDV)
#define SET_MAP		    (RMII_RXDV_MUXID)
#define RXLED_CTRL		(GPIO_03)
#define RXLED_MAP		(GPIO_03_MUXID)
#define VA_EN		    (GPIO_10)
#define VB_EN		    (RMII_RXER)
#define VC_EN		    (GPIO_02)
#endif

#define TXLED_CTRL		(JTRST)
#define TXLED_MAP		(JTRST_MUXID)

#if defined(MIZAR1M) || defined(VENUS2M)
#define LD0_CTRL	(JTCK)
#define LD0_MAP		(JTCK_MUXID)
#define STA_CTRL	(JTDO)
#define STA_MAP		(JTDO_MUXID)
#define SET_CTRL	(GPIO_08)
#define SET_MAP		(GPIO_08_MUXID)
#define RXLED_CTRL	(GPIO_02)
#define RXLED_MAP	(GPIO_02_MUXID)
#define LD1_CTRL	(GPIO_09)
#define LD1_MAP		(GPIO_09_MUXID)
//#define VA_EN		    (GPIO_10)
//#define VB_EN		    (RMII_RXER)
//#define VC_EN		    (GPIO_02)

#endif
//#define ZC0_CTRL       (GPIO_01)
//#define ZC0_MAP 		(GPIO_01_MUXID)


#define ZCA_CTRL 		(GPIO_01)
#define ZCA_MAP 		(GPIO_01_MUXID)

#define ZCB_CTRL		(GPIO_10)
#define ZCB_MAP		    (GPIO_10_MUXID)

#define ZCC_CTRL		(GPIO_03)
#define ZCC_MAP		    (GPIO_03_MUXID)


#define PLCRST_CTRL	    (GPIO_07)
#define PLCRST_MAP		(GPIO_07_MUXID)
#define PLUG_CTRL		(JTMS)
#define PLUG_MAP		(JTMS_MUXID)
#define POWEROFF_CTRL   (JTDI)
#define POWEROFF_MAP    (JTDI_MUXID)

#elif defined(ZC3750CJQ2)                       //CJQ2

#if defined(UNICORN2M)
#define EN485_CTRL      (GPIO_03)
#define EN485_MAP		(GPIO_03_MUXID)

#define RXLED_CTRL		(JTRST)
#define RXLED_MAP		(JTRST_MUXID)
#define RUN_CTRL        (JTDO)
#define RUN_MAP         (JTDO_MUXID)

#define LD0_CTRL		(GPIO_02)
#define LD0_MAP		    (GPIO_02_MUXID)

#define POWEROFF_CTRL   (RMII_RXDV)
#define POWEROFF_MAP    (RMII_RXDV_MUXID)


#define ZC0_CTRL 		(GPIO_01)
#define ZC0_MAP 		(GPIO_01_MUXID)

#elif defined(ROLAND1_1M)
#define EN485_CTRL      (GPIO_07)
#define EN485_MAP		(GPIO_07_MUXID)

#define RXLED_CTRL		(GPIO_10)
#define RXLED_MAP		(GPIO_10_MUXID)
#define RUN_CTRL        (GPIO_04)
#define RUN_MAP         (GPIO_04_MUXID)

#define LD0_CTRL		(GPIO_09)
#define LD0_MAP		    (GPIO_09_MUXID)

#define POWEROFF_CTRL   (JTDO)
#define POWEROFF_MAP    (JTDO_MUXID)

#elif defined(MIZAR1M) || defined(VENUS2M)

#define SET_CTRL        (JTDO)
#define SET_MAP         (JTDO_MUXID)

#define EN485_CTRL      (GPIO_07)
#define EN485_MAP		(GPIO_07_MUXID)

//#define RXLED_CTRL		(GPIO_10)
//#define RXLED_MAP		(GPIO_10_MUXID)
#define RXLED_CTRL		(JTRST)
#define RXLED_MAP		(JTRST_MUXID)

//#define RUN_CTRL        (GPIO_04)
//#define RUN_MAP         (GPIO_04_MUXID)
#define RUN_CTRL        (GPIO_02)
#define RUN_MAP         (GPIO_02_MUXID)


#define POWEROFF_CTRL   (JTDI)
#define POWEROFF_MAP    (JTDI_MUXID)

#define LD0_CTRL	(JTCK)
#define LD0_MAP		(JTCK_MUXID)
#if defined(STD_DUAL)
#define LD1_CTRL	(GPIO_09)
#define LD1_MAP		(GPIO_09_MUXID)
//#define LD1_CTRL	(JTCK)
//#define LD1_MAP	(JTCK_MUXID)

#endif

#endif

/*
#define ZCA_CTRL 		(GPIO_01)
#define ZCA_MAP 		(GPIO_01_MUXID)
#define ZCB_CTRL		(JTCK)
#define ZCB_MAP		    (JTCK_MUXID)
#define ZCC_CTRL		(JTDI)
#define ZCC_MAP		    (JTDI_MUXID)
*/

#define ZCA_CTRL 		(GPIO_01)
#define ZCA_MAP 		(GPIO_01_MUXID)
#define ZCB_CTRL		(GPIO_10)
#define ZCB_MAP		    (GPIO_10_MUXID)
#define ZCC_CTRL		(GPIO_03)
#define ZCC_MAP		    (GPIO_03_MUXID)




#elif defined(ZC3951CCO)                            //CCO
#define RXLED_CTRL		(URX)
#define RXLED_MAP		(UART0_RX_MUXID)


#if defined(UNICORN8M)
#define LD0_CTRL	(GPIO_02)
#define LD0_MAP		(GPIO_02_MUXID)
#define ZCA_CTRL	(GPIO_01)
#define ZCA_MAP		(GPIO_01_MUXID)
#define TXA		(GPIO_06)
#define TXB		(GPIO_07)
#define TXC		(GPIO_08)
#define RXA		(GPIO_09)
#define RXB		(GPIO_10)
#define RXC		(GPIO_00)
#define TXA_MUXID	(GPIO_06_MUXID)
#define TXB_MUXID	(GPIO_07_MUXID)
#define TXC_MUXID	(GPIO_08_MUXID)
#define RXA_MUXID	(GPIO_09_MUXID)
#define RXB_MUXID	(GPIO_10_MUXID)
#define RXC_MUXID	(GPIO_00_MUXID)

#elif defined(ROLAND9_1M)

#define LD0_CTRL	(JTDO)
#define LD0_MAP		(JTDO_MUXID)
#define ZCA_CTRL	(JTCK)
#define ZCA_MAP		(JTCK_MUXID)
#define TXA		(GPIO_06)
#define TXB		(GPIO_07)
#define TXC		(GPIO_04)
#define RXA		(GPIO_09)
#define RXB		(GPIO_10)
#define RXC		(JTRST)
#define TXA_MUXID	(GPIO_06_MUXID)
#define TXB_MUXID	(GPIO_07_MUXID)
#define TXC_MUXID	(GPIO_04_MUXID)
#define RXA_MUXID	(GPIO_09_MUXID)
#define RXB_MUXID	(GPIO_10_MUXID)
#define RXC_MUXID	(JTRST_MUXID)

#elif defined(MIZAR9M) || defined(VENUS8M)
#define LD0_CTRL	(JTCK)
#define LD0_MAP		(JTCK_MUXID)
#define LD1_CTRL	(JTRST)
#define LD1_MAP		(JTRST_MUXID)
#define ZCA_CTRL	(GPIO_01)
#define ZCA_MAP		(GPIO_01_MUXID)
#define TXA		(GPIO_06)
#define TXB		(GPIO_07)
#define TXC		(GPIO_08)
#define RXA		(GPIO_09)
#define RXB		(GPIO_10)
#define RXC		(JTDO)
#define TXA_MUXID	(GPIO_06_MUXID)
#define TXB_MUXID	(GPIO_07_MUXID)
#define TXC_MUXID	(GPIO_08_MUXID)
#define RXA_MUXID	(GPIO_09_MUXID)
#define RXB_MUXID	(GPIO_10_MUXID)
#define RXC_MUXID	(JTDO_MUXID)

#endif
#define ZC0_CTRL        (GPIO_03)
#define ZC0_MAP 		(GPIO_03_MUXID)

#define ZCB_CTRL		(JTMS)
#define ZCB_MAP		    (JTMS_MUXID)
#define ZCC_CTRL		(JTDI)
#define ZCC_MAP		    (JTDI_MUXID)

#endif



extern gpio_dev_t zca;


#if defined(ZC3750STA) && (defined(ROLAND1M) )//|| defined(MIZAR1M) || defined(VENUS2M))

gpio_dev_t sta_phase_ctr;

#define   READ_EN_A_PIN ((VA_EN&gpio_input(NULL,VA_EN))?1:0)
#define   READ_EN_B_PIN ((VB_EN&gpio_input(NULL,VB_EN))?1:0)
#define   READ_EN_C_PIN ((VC_EN&gpio_input(NULL,VC_EN))?1:0)

#endif
/*#if defined(ZC3951CCO)
#define   ZERO_A_PIN ((ZCA_CTRL&gpio_input(NULL,ZCA_CTRL))?1:0)
#define   ZERO_B_PIN ((ZCB_CTRL&gpio_input(NULL,ZCB_CTRL))?1:0)
#define   ZERO_C_PIN ((ZCC_CTRL&gpio_input(NULL,ZCC_CTRL))?1:0)
#endif*/





extern void board_cfg();



extern void ld_set_3p_meter_phase_zc(uint8_t phase);


#endif // PLC_IO_H
