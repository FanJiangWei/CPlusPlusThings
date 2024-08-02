/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	gpio.h
 * Purpose:	GPIO manipulation
 * History:
 *	Jan 5, 2016	jetmotor	Create
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#include "types.h"
#include "list.h"
#include "trios.h"
#include "vsh.h"

enum {
	GPIO_TYPE_USR = 0,
	GPIO_TYPE_SPI,
	GPIO_TYPE_SS1,
	GPIO_TYPE_SS2,
	GPIO_TYPE_SS3,
	GPIO_TYPE_UART0,
	GPIO_TYPE_UART1,
	GPIO_TYPE_UART2,
#if defined(FPGA_VERSION) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) \
	|| defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
	GPIO_TYPE_UART3,
	GPIO_TYPE_UART4,
	GPIO_TYPE_I2C,
	GPIO_TYPE_SPI_SLAVE,
#elif defined(LIGHTELF2M) || defined(LIGHTELF8M)
	GPIO_TYPE_I2C,
#elif defined(ROLAND1M) || defined(UNICORN2M) || defined(UNICORN8M)
	GPIO_TYPE_SPI_SLAVE,
#endif
	GPIO_TYPE_LD0,
	GPIO_TYPE_LD1,
	GPIO_TYPE_LD2,
	GPIO_TYPE_LD3,

	GPIO_TYPE_TICK0,
	GPIO_TYPE_TICK1,
	GPIO_TYPE_TICK2,
	GPIO_TYPE_TICK3,
	GPIO_TYPE_GE_RMII,
	GPIO_TYPE_JTAG,
	GPIO_TYPE_MDIO,
	GPIO_TYPE_WDT,
	GPIO_TYPE_ZC0,
	GPIO_TYPE_ZC1,
#if defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) \
	|| defined(VENUS2M) || defined(VENUS8M)
	GPIO_TYPE_ZC2,
	GPIO_TYPE_ZC3,
	GPIO_TYPE_ZC4,
	GPIO_TYPE_ZC5,
	GPIO_TYPE_SPI1_CS0,
	GPIO_TYPE_SPI1_CS1,
#if defined(ROLAND1_1M) || defined(ROLAND9_1M)
	GPIO_TYPE_SPI2_CS0,
	GPIO_TYPE_SPI2_CS1,
#endif
#endif
#if defined(VENUS_V3) || defined(VENUS_V5) || defined(FPGA_VERSION)
	GPIO_TYPE_RF,
	GPIO_TYPE_PSRAM,
	GPIO_TYPE_SPI_SLAVE1,
#endif
};

/* GPIO direction */
#define GPIO_IN		1
#define GPIO_OUT	0

/* MUX_ID definition */
#define GPIO_00_MUXID		0
#define GPIO_01_MUXID		1
#define JTRST_MUXID		2
#define JTMS_MUXID		3
#define JTDI_MUXID		4
#define JTDO_MUXID		5
#define JTCK_MUXID		6
#define GPIO_02_MUXID		7
#define RMII_TXD1_MUXID		8
#define RMII_TXD0_MUXID		9
#define RMII_TXEN_MUXID		10
#define RMII_CLKREF_MUXID	11
#define GPIO_03_MUXID		12
#define RMII_RXDV_MUXID		13
#define RMII_RXD0_MUXID		14
#define RMII_RXD1_MUXID		15	
#define RMII_RXER_MUXID		16
#define GPIO_04_MUXID		17
#define GPIO_05_MUXID		18
#define GPIO_06_MUXID		19
#define UART0_TX_MUXID		20
#define UART0_RX_MUXID		21
#define	MDIO_MDC_MUXID		22
#define MDIO_MDIO_MUXID		23
#define SPI_MOSI_MUXID		24
#define SPI_CS_MUXID		25
#define SPI_MISO_MUXID		26
#define SPI_SCK_MUXID		27
#define GPIO_07_MUXID		28
#define GPIO_08_MUXID		29
#define GPIO_09_MUXID		30
#define GPIO_10_MUXID		31
#define GPIO_INVALID_MUXID	0xff
#if defined(VENUS_V3) || defined(VENUS_V5) || defined(FPGA_VERSION)
#define GPIO_32_MUXID		32
#define GPIO_33_MUXID		33
#define GPIO_34_MUXID		34
#define GPIO_35_MUXID		35
#define GPIO_36_MUXID		36
#define GPIO_37_MUXID		37
#define GPIO_38_MUXID		38
#define GPIO_39_MUXID		39
#define GPIO_40_MUXID		40
#define GPIO_41_MUXID		41
#define GPIO_42_MUXID		42
#define GPIO_43_MUXID		43
#define GPIO_44_MUXID		44
#define GPIO_45_MUXID		45
#define GPIO_46_MUXID		46
#define GPIO_47_MUXID		47
#define GPIO_48_MUXID		48
#define GPIO_49_MUXID		49
#define GPIO_50_MUXID		50
#define GPIO_51_MUXID		51
#define GPIO_52_MUXID		52
#define GPIO_53_MUXID		53
#define GPIO_54_MUXID		54
#define GPIO_55_MUXID		55
#define GPIO_56_MUXID		56
#define GPIO_57_MUXID		57
#define GPIO_58_MUXID		58
#define GPIO_59_MUXID		59
#define GPIO_60_MUXID		60
#define GPIO_61_MUXID		61
#define GPIO_62_MUXID		62
#define GPIO_63_MUXID		63
#define GPIO_11_MUXID		GPIO_32_MUXID
#define GPIO_12_MUXID		GPIO_33_MUXID
#define GPIO_13_MUXID		GPIO_34_MUXID
#define GPIO_14_MUXID		GPIO_35_MUXID
#define GPIO_15_MUXID		GPIO_36_MUXID
#define PSRAM_CS_MUXID		GPIO_37_MUXID
#define PSRAM_SCK_MUXID		GPIO_38_MUXID
#define PSRAM_IO0_MUXID		GPIO_39_MUXID
#define PSRAM_IO1_MUXID		GPIO_40_MUXID
#define PSRAM_IO2_MUXID		GPIO_41_MUXID
#define PSRAM_IO3_MUXID		GPIO_42_MUXID
#define RF_CS_MUXID		GPIO_43_MUXID
#define RF_SCK_MUXID		GPIO_44_MUXID
#define RF_MOSI_MUXID		GPIO_45_MUXID
#define RF_MISO_MUXID		GPIO_46_MUXID
#define RF_MCLK_MUXID		GPIO_47_MUXID
#define RF_FCLK_MUXID		GPIO_48_MUXID
#define RF_TXNRX_MUXID		GPIO_49_MUXID
#define RF_ENABLE_MUXID		GPIO_50_MUXID
#define RF_GAIN_MUXID		GPIO_51_MUXID
#define RF_DIQ0_MUXID		GPIO_52_MUXID
#define RF_DIQ1_MUXID		GPIO_53_MUXID
#define RF_DIQ2_MUXID		GPIO_54_MUXID
#define RF_DIQ3_MUXID		GPIO_55_MUXID
#define RF_DIQ4_MUXID		GPIO_56_MUXID
#define RF_DIQ5_MUXID		GPIO_57_MUXID
#define RF_DIQ6_MUXID		GPIO_58_MUXID
#define RF_DIQ7_MUXID		GPIO_59_MUXID
#define RF_DIQ8_MUXID		GPIO_60_MUXID
#define RF_DIQ9_MUXID		GPIO_61_MUXID
#define RF_DIQ10_MUXID		GPIO_62_MUXID
#define RF_DIQ11_MUXID		GPIO_63_MUXID
#define GPIO_MAX_MUXID		63
#else
#define GPIO_MAX_MUXID		31
#endif

/* PINS definition */
#define GPIO_00			(1UL << GPIO_00_MUXID)
#define GPIO_01			(1UL << GPIO_01_MUXID)
#define GPIO_02			(1UL << GPIO_02_MUXID)
#define GPIO_03			(1UL << GPIO_03_MUXID)
#define GPIO_04			(1UL << GPIO_04_MUXID)
#define GPIO_05			(1UL << GPIO_05_MUXID)
#define GPIO_06			(1UL << GPIO_06_MUXID)
#define GPIO_07			(1UL << GPIO_07_MUXID)
#define GPIO_08			(1UL << GPIO_08_MUXID)
#define GPIO_09			(1UL << GPIO_09_MUXID)
#define GPIO_10			(1UL << GPIO_10_MUXID)
#define JTDO			(1UL << JTDO_MUXID)
#define JTDI			(1UL << JTDI_MUXID)
#define JTCK			(1UL << JTCK_MUXID)
#define JTMS			(1UL << JTMS_MUXID)
#define JTRST			(1UL << JTRST_MUXID)
#define UTX			(1UL << UART0_TX_MUXID)
#define URX			(1UL << UART0_RX_MUXID)
#define SPI_MOSI		(1UL << SPI_MOSI_MUXID)
#define SPI_CS			(1UL << SPI_CS_MUXID)
#define SPI_MISO		(1UL << SPI_MISO_MUXID)
#define SPI_SCK			(1UL << SPI_SCK_MUXID)
#define RMII_RXDV		(1UL << RMII_RXDV_MUXID)
#define RMII_TXD0		(1UL << RMII_TXD0_MUXID)
#define RMII_TXD1		(1UL << RMII_TXD1_MUXID)
#define RMII_TXEN		(1UL << RMII_TXEN_MUXID)
#define RMII_CLKREF		(1UL << RMII_CLKREF_MUXID)
#define RMII_RXD0		(1UL << RMII_RXD0_MUXID)
#define RMII_RXD1		(1UL << RMII_RXD1_MUXID)
#define RMII_RXER		(1UL << RMII_RXER_MUXID)
#define	MDIO_MDC		(1UL << MDIO_MDC_MUXID)
#define MDIO_MDIO		(1UL << MDIO_MDIO_MUXID)
#define GPIO_INVALID_PINS	(0)
#if defined(VENUS_V3) || defined(VENUS_V5) || defined(FPGA_VERSION)
#define GPIO_32	       (1ULL << GPIO_32_MUXID)
#define GPIO_33	       (1ULL << GPIO_33_MUXID)
#define GPIO_34	       (1ULL << GPIO_34_MUXID)
#define GPIO_35	       (1ULL << GPIO_35_MUXID)
#define GPIO_36	       (1ULL << GPIO_36_MUXID)
#define GPIO_37	       (1ULL << GPIO_37_MUXID)
#define GPIO_38	       (1ULL << GPIO_38_MUXID)
#define GPIO_39	       (1ULL << GPIO_39_MUXID)
#define GPIO_40	       (1ULL << GPIO_40_MUXID)
#define GPIO_41	       (1ULL << GPIO_41_MUXID)
#define GPIO_42	       (1ULL << GPIO_42_MUXID)
#define GPIO_43	       (1ULL << GPIO_43_MUXID)
#define GPIO_44	       (1ULL << GPIO_44_MUXID)
#define GPIO_45	       (1ULL << GPIO_45_MUXID)
#define GPIO_46	       (1ULL << GPIO_46_MUXID)
#define GPIO_47	       (1ULL << GPIO_47_MUXID)
#define GPIO_48	       (1ULL << GPIO_48_MUXID)
#define GPIO_49	       (1ULL << GPIO_49_MUXID)
#define GPIO_50	       (1ULL << GPIO_50_MUXID)
#define GPIO_51	       (1ULL << GPIO_51_MUXID)
#define GPIO_52	       (1ULL << GPIO_52_MUXID)
#define GPIO_53	       (1ULL << GPIO_53_MUXID)
#define GPIO_54	       (1ULL << GPIO_54_MUXID)
#define GPIO_55	       (1ULL << GPIO_55_MUXID)
#define GPIO_56	       (1ULL << GPIO_56_MUXID)
#define GPIO_57	       (1ULL << GPIO_57_MUXID)
#define GPIO_58	       (1ULL << GPIO_58_MUXID)
#define GPIO_59	       (1ULL << GPIO_59_MUXID)
#define GPIO_60	       (1ULL << GPIO_60_MUXID)
#define GPIO_61	       (1ULL << GPIO_61_MUXID)
#define GPIO_62	       (1ULL << GPIO_62_MUXID)
#define GPIO_63	       (1ULL << GPIO_63_MUXID)
#define PSRAM_CS	GPIO_37
#define PSRAM_SCK	GPIO_38
#define PSRAM_IO0	GPIO_39
#define PSRAM_IO1	GPIO_40
#define PSRAM_IO2	GPIO_41
#define PSRAM_IO3	GPIO_42
#define RF_CS		GPIO_43
#define RF_SCK		GPIO_44
#define RF_MOSI		GPIO_45
#define RF_MISO		GPIO_46
#define RF_MCLK		GPIO_47
#define RF_FCLK		GPIO_48
#define RF_TXNRX	GPIO_49
#define RF_ENABLE	GPIO_50
#define RF_GAIN		GPIO_51
#define RF_DIQ0		GPIO_52
#define RF_DIQ1		GPIO_53
#define RF_DIQ2		GPIO_54
#define RF_DIQ3		GPIO_55
#define RF_DIQ4		GPIO_56
#define RF_DIQ5		GPIO_57
#define RF_DIQ6		GPIO_58
#define RF_DIQ7		GPIO_59
#define RF_DIQ8		GPIO_60
#define RF_DIQ9		GPIO_61
#define RF_DIQ10	GPIO_62
#define RF_DIQ11	GPIO_63
#endif

typedef struct phase_muxid_s {
	uint8_t txa;		/* tx phase A gpio muxid */
	uint8_t txb;		/* tx phase B gpio muxid */
	uint8_t txc;		/* tx phase C gpio muxid */
	uint8_t rxa;		/* rx phase A gpio muxid */
	uint8_t rxb;		/* rx phase B gpio muxid */
	uint8_t rxc;		/* rx phase C gpio muxid */
} phase_muxid_t;

extern phase_muxid_t phase_mux;

#define GPIO_INTR_TRIGGER_MODE_HIGHLEVEL	0
#define GPIO_INTR_TRIGGER_MODE_POSEDGE		1
#define GPIO_INTR_TRIGGER_MODE_LOWLEVEL		2
#define GPIO_INTR_TRIGGER_MODE_NEGEDGE		3

typedef union gpio_user_map_s {
	struct {
		uint8_t ld0;
		uint8_t ld1;
		uint8_t zca;
		uint8_t zcb;
		uint8_t zcc;
		uint8_t zcd;
		uint8_t zce;
		uint8_t zcf;
		uint8_t txa;
		uint8_t txb;
		uint8_t txc;
		uint8_t rxa;
		uint8_t rxb;
		uint8_t rxc;
		uint8_t plc_txd;	/* uart1 tx */
		uint8_t plc_rxd;	/* uart1 rx */
		uint8_t det_pwr;
		uint8_t det_plug;
		uint8_t io_reset;
		uint8_t event;
		uint8_t cs;
		uint8_t miso;
		uint8_t mosi;
		uint8_t sck;
		uint8_t ird_txd;
		uint8_t ird_rxd;
		uint8_t ird_38k;
		uint8_t chrg_en;
	};
	uint8_t io[32];
} gpio_user_map_t;

typedef struct gpio_drv_s {
	//event_t	mutex;

	list_head_t	dev_list;
	uint32_t	dev_nr;

	uint64_t	pins;		/* GPIO pins vacant or occupied */
	uint64_t	dirs;		/* occupied GPIO pins direction */
} gpio_drv_t;

extern gpio_drv_t GPIO;

typedef struct gpio_desc_s {
	uint8_t id;		/* MUXID of to-be-occupied single pin that dosen't belong to this GPIO device type by default */
	uint8_t dir :1;		/* GPIO_IN or GPIO_OUT */
	uint8_t trigger :2;	/* 0: high level; 1: pos edge; 2: low level; 3: neg edge */
	uint8_t :5;
} gpio_desc_t;

/* gpio_dev_t - Any devices that use at least one GPIO pin shall be described as a GPIO device, and
 * grouped into managment via `gpio_attach_dev'.
 */
typedef struct gpio_dev_s {
	void (*init)(struct gpio_dev_s *dev);

	list_head_t	link;

	char		*name;		/* device name */
	uint32_t	type;		/* GPIO device type */
	uint64_t	pins;		/* GPIO pins (bitmap of MUXIDs) the device used */
	uint64_t	dirs;		/* GPIO pins direction (bitmap of MUXIDs) */

	union {
		/* spi */
		struct {
			gpio_desc_t css;
			gpio_desc_t miso;
			gpio_desc_t mosi;
			gpio_desc_t sck;
		};

		/* uart */
		struct {
			gpio_desc_t txd;
			gpio_desc_t rxd;
		};

		/* i2c */
		struct {
			gpio_desc_t sda;
			gpio_desc_t scl;
		};

		/* phase */
		struct {
			gpio_desc_t txa;
			gpio_desc_t txb;
			gpio_desc_t txc;
			gpio_desc_t rxa;
			gpio_desc_t rxb;
			gpio_desc_t rxc;
		};

		gpio_desc_t mux[8];
	};
} gpio_dev_t;

extern gpio_dev_t UART;
extern gpio_dev_t JTAG;
#if defined(LIGHTELF8M) || defined(UNICORN8M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS8M)
extern gpio_dev_t MDIO;
extern gpio_dev_t RMII;
#endif

extern void gpio_init(void);
/* gpio_disable_2nd_boot - disable hardware automatic spi cs selection
 */
extern void gpio_disable_2nd_boot(uint8_t ext_flash);
/* gpio_attach_dev - Attach a new GPIO device into management.
 * @dev: GPIO device instance
 * @return: 0 if succeed, -EINVAL if fail (pins overlapped)
 */
extern int32_t gpio_attach_dev(gpio_dev_t *dev);
/* gpio_remove_dev - Remove a GPIO device from management.
 * @dev: GPIO device instance
 * @return: 0 if succeed, -EINVAL if `@dev' with suspicious pins
 */
extern int32_t gpio_remove_dev(gpio_dev_t *dev);
/* gpio_find_dev - Find certain GPIO device's handler according to pins.
 * @pins: pins (bitmap of MUXIDs)
 * @return: NULL or pointer to an instance of gpio_dev_t
 */
extern gpio_dev_t * gpio_find_dev(uint64_t pins);
/* gpio_input_safe - Read status of corresponding `GPIO_IN' GPIO pins.
 * @time: unit ms
 * @return: 0 --> low,  !0 --> high
 */
extern uint64_t gpio_input_safe(gpio_dev_t *dev, uint64_t pins, uint32_t time);
/* gpio_input - Read status of corresponding `GPIO_IN' GPIO pins.
 */
extern uint64_t gpio_input(gpio_dev_t *dev, uint64_t pins);
/* gpio_output - Read status of corresponding `GPIO_OUT' GPIO pins.
 */
extern uint64_t gpio_output(gpio_dev_t *dev, uint64_t pins);
/* gpio_set_dir - Set GPIO pins in group of either `GPIO_IN' or `GPIO_OUT'.
 */
extern void gpio_set_dir(gpio_dev_t *dev, uint64_t pins, uint8_t dir);
/* gpio_pins_on - Turn on (up) corresponding `GPIO_OUT' GPIO pins.
 */
extern void gpio_pins_on(gpio_dev_t *dev, uint64_t pins);
/* gpio_pins_off - Turn off (down) corresponding `GPIO_OUT' GPIO pins.
 */
extern void gpio_pins_off(gpio_dev_t *dev, uint64_t pins);
/* gpio_request_irq - request a interrupt for gpio pin
 * @pin: which gpio pin (0 ~ 31)
 * @mode: trigger mode (0: highlevel, 1: posedge, 2: lowlevel, 3 negedge)
 * @handler: user provided handler
 * @data: the argument for '@handler'
 * @return: OK if succeed, -EINVAL if fail (unknown pin, pin's interrupt is occupied, trigger mode error)
 */
extern int32_t gpio_request_irq(uint8_t pin, uint32_t mode, void (*handler)(void *data), void *data);
/* gpio_release_irq - release a interrrupt for gpio pin
 * @pin: which gpio pin
 * @return: OK if succeed, -EINVAL if fail (unknown pin)
 */
extern int32_t gpio_release_irq(uint8_t pin);

extern void gpio_enable_irq(uint8_t pin, void *context);
extern void gpio_disable_irq(uint8_t pin);
extern uint32_t gpio_get_enable_state(uint8_t pin);
extern __SLOWTEXT void gpio_show(tty_t *term);

#if defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
/* gpio_set_pull_down - pull down gpio pin
 */
extern void gpio_set_pull_down(uint32_t pins);
/* gpio_set_pull_up - pull up gpio pin
 */
extern void gpio_set_pull_up(uint32_t pins);

#define GPIO_PULL_NONE		(0)
#define GPIO_PULL_DOWN		(1)
#define GPIO_PULL_UP		(2)
/* gpio_get_pull_state - get gpio pull state 
 */
extern uint32_t gpio_get_pull_state(uint32_t pins);
/* gpio_set_smit_en - smooth gpio high/low state
 */
extern void gpio_set_smit_en(uint64_t pins, uint8_t en);
/* gpio_get_smit_en - get gpio smit state
 */
extern uint64_t gpio_get_smit_en(uint64_t pins);

/* drv 0:2.4mA 1:4.8mA 2:7.2mA 3:9.6mA
 */
extern void gpio_set_pad_drv(uint64_t pins, uint8_t drv);
#endif

#define GPIO_RISING_EDGE		(1)
#define GPIO_FALLING_EDGE		(2)
#define GPIO_DOUBLE_EDGE		(3)
/*idx: [0 - 5]*/
extern void zc_set_edge(uint8_t idx, int32_t mode);
extern int32_t zc_get_edge(uint8_t idx);
extern int32_t zc_get_level(uint8_t idx);

extern gpio_dev_t UART;
extern gpio_dev_t JTAG;
#if defined(LIGHTELF8M) || defined(UNICORN8M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS8M)
extern gpio_dev_t MDIO;
extern gpio_dev_t RMII;
extern gpio_dev_t phase_dev;
#endif
extern gpio_dev_t ld;
extern gpio_dev_t zc0;
extern gpio_dev_t zc1;
extern gpio_dev_t zc2;
extern gpio_dev_t zc3;

extern gpio_dev_t zc4;
extern gpio_dev_t zc5;

extern gpio_dev_t zca;
extern gpio_dev_t zcb;
extern gpio_dev_t zcc;
extern gpio_dev_t zc;
extern gpio_dev_t power_gpio;
extern gpio_dev_t ird_38k_gpio;

extern gpio_user_map_t IOMAP;

#endif	/* end of _GPIO_H_ */
