/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        sys.h
 * Purpose:     System data structure; each system consists of 4 ports.
 * History:     01/15/2007, created by andyzhou
 */

#ifndef _SYS_H
#define _SYS_H


#include "types.h"
#include "if.h"
#include "term.h"
#include "libc.h"


#define MACF_LEN	18
#define macf(s, mac)	\
({			\
	__snprintf(s, MACF_LEN, "%02x:%02x:%02x:%02x:%02x:%02x", (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5]); \
	s;		\
})
#define macf_simple(s, mac)	\
({			\
	__snprintf(s, MACF_LEN, "%02x%02x%02x%02x%02x%02x", (mac)[0], (mac)[1], (mac)[2], (mac)[3], (mac)[4], (mac)[5]); \
	s;		\
})
#define macf_invert(s, mac)	\
({			\
	__snprintf(s, MACF_LEN, "%02x%02x%02x%02x%02x%02x", (mac)[5], (mac)[4], (mac)[3], (mac)[2], (mac)[1], (mac)[0]); \
	s;		\
})

#define NELEMS(array)	(sizeof(array) / sizeof(array[0]))

#define TIMEF_LEN 9
#define timef(s, secs)   \
({      \
	__snprintf(s, TIMEF_LEN, "%02d:%02d:%02d", (uint8_t)((secs) % (24 * 3600) / 3600), (uint8_t)((secs) % 3600 / 60), (uint8_t)((secs) % 60));   \
	s;  \
})

extern uint8_t ETH_ADDR[];

typedef struct sys_info_s {
	uint8_t boot_version[16];
	uint8_t boot_build_date[32];

	uint32_t random_seed;

	uint8_t reserved[204];
} sys_info_t;
extern sys_info_t *sys_info;

extern uint8_t FIRM[];
extern uint8_t CHIP[];
extern uint32_t VER[];
extern uint32_t BOOTV[];
extern uint32_t YEAR, MONTH, DATE;
extern uint32_t HOUR, MINUTE, SECOND;
extern uint32_t RESET_CODE;

typedef struct sys_time_s {
	uint32_t year; 
	uint8_t  mon;
	uint8_t  day;
	
	uint8_t  hour;
	uint8_t  min;
	uint8_t  sec;
} sys_time_t;

extern void set_sys_time(sys_time_t *time);
extern void secs_to_time(sys_time_t *time, uint32_t sec);
extern ulong_t time_to_secs(uint32_t year0, uint32_t mon0, uint32_t day, uint32_t hour, uint32_t min, uint32_t sec);


#define bitmap_set_bit(bmp, idx)	(((uint8_t *)(bmp))[(idx) >> 3] |= (1 << ((idx) & 0x7)))
#define bitmap_clr_bit(bmp, idx)	(((uint8_t *)(bmp))[(idx) >> 3] &= ~(1 << ((idx) & 0x7)))
#define bitmap_in_bit(bmp, idx)		(((uint8_t *)(bmp))[(idx) >> 3] & (1 << ((idx) & 0x7)))

/* ffzb - Find First(least-significant) Zero Bit (in least-significant way).
 *        i.e., ffzb[255] is -1, ffzb[6] = 0. 
 */
extern int8_t const ffzb[];
static __inline__ int32_t bitmap_find_least_zero(uint8_t bmp[], uint32_t size)
{
	uint32_t i;
	int8_t bit;

	for (i = 0; i < size; ++i) {
		bit = ffzb[bmp[i]];
		if (bit < 0) continue;
		return (i << 3) + bit;
	}
	return -1;
}

/* ffsb - Find First(least-significant) Set Bit (in least-significant way).
 *        i.e., ffsb[0] is -1, ffsb[6] = 1.
 */
extern int8_t const ffsb[];
static __inline__ int32_t bitmap_find_least_one(uint8_t bmp[], uint32_t size)
{
	uint32_t i;
	int8_t bit;

	for (i = 0; i < size; ++i) {
		bit = ffsb[bmp[i]];
		if (bit < 0) continue;
		return (i << 3) + bit;
	}
	return -1;
}


/* fnsb - Find Number of Set Bit from a one-byte data.
 *        i.e., fnsb[6] is 2.
 */
extern int8_t const fnsb[];
static __inline__ uint32_t bitmap_get_one_nr(uint8_t bmp[], uint32_t size)
{
	uint32_t i, nr;

	for (i = 0, nr = 0; i < size; ++i) {
		nr += fnsb[bmp[i]];
	}

	return nr;
}


/*
 * printf_s - printf as synchronization
 */
extern void printf_s(const char *fmt, ...);
extern void print_s(const char *s);
#define assert_s(e)	\
	((e) ? (void)0 : printf_s("%s: %d: Assertion \"%s\" failed.\n", __func__, __LINE__, #e))

/*
 * printf_d - printf as a daemon
 */
extern void printf_d(const char *fmt, ...);
extern void print_d(const char *s);
#define assert_d(e)     \
        ((e) ? (void)0 : printf_d("%s: %d: Assertion \"%s\" failed.\n", __func__, __LINE__, #e))

#define CLK_GATE_AIF	(1UL << 0)
#define CLK_GATE_PMD	(1UL << 1)
#define CLK_GATE_PMA	(1UL << 2)
#define CLK_GATE_SPI	(1UL << 3)
#define CLK_GATE_WDT	(1UL << 4)
#define CLK_GATE_GPIO	(1UL << 5)
#define CLK_GATE_MDIO	(1UL << 6)
#define CLK_GATE_SPISLV	(1UL << 7)
#define CLK_GATE_UART0	(1UL << 8)
#define CLK_GATE_UART1	(1UL << 9)
#define CLK_GATE_UART2	(1UL << 10)
#define CLK_GATE_ACU	(1UL << 11)
#define CLK_GATE_TICK	(1UL << 12)
#define CLK_GATE_EFUSE	(1UL << 13)
#define CLK_GATE_I2C	(1UL << 14)
#define CLK_GATE_SPI1	(1UL << 15)
#define CLK_GATE_SPI2	(1UL << 16)
#define CLK_GATE_UART3	(1UL << 17)
#define CLK_GATE_UART4	(1UL << 18)
#define CLK_GATE_WAIF	(1UL << 19)
#define CLK_GATE_WPMD	(1UL << 20)
#define CLK_GATE_WPMA	(1UL << 21)



#define DEBUG_PHY	(1UL << 0)
#define DEBUG_DMA	(1UL << 1)
#define DEBUG_GE	(1UL << 2)
#define DEBUG_ACU	(1UL << 3)
#define DEBUG_BEACON	(1UL << 4)
#define DEBUG_BC	(1UL << 5)
#define DEBUG_SOF	(1UL << 6)
#define DEBUG_SACK	(1UL << 7)
#define DEBUG_COORD	(1UL << 8)
#define DEBUG_MM	(1UL << 9)
#define DEBUG_LWIP      (1UL << 10)
#define DEBUG_STA	(1UL << 11)
#define DEBUG_ROUTE	(1UL << 12)
#define DEBUG_APP	(1UL << 13)
#define DEBUG_METER     (1UL << 14)
#define DEBUG_UPGRADE	(1UL << 15)
#define DEBUG_MDL	(1UL << 16)
#define DEBUG_ZC	(1UL << 17)
#define DEBUG_MTAD	(1UL << 18)
#define DEBUG_TIME	(1UL << 19)
#define DEBUG_UART	(1UL << 20)
#define DEBUG_ID	(1UL << 21)
#define DEBUG_REG	(1UL << 22)
#define DEBUG_EVENT	(1UL << 23)
#define DEBUG_DIAGNOSE	(1UL << 24)
#define DEBUG_WPHY      (1UL << 25)
#define DEBUG_SWC       (1UL << 26)
#define DEBUG_MISC      (1UL << 27)

#define NR_DEBUG_LEVEL_MAX	32


#define DEBUG_NET   DEBUG_MDL
#define DEBUG_MACS  DEBUG_ACU

extern int32_t debug_level;

#define open_debug_level(levels)  debug_level|=levels
#define close_debug_level(levels)   debug_level&=~levels


extern tty_t dty;
extern void debug_dump(tty_t *term, int32_t level, uint8_t *p, uint32_t len);
/* debug_print(f) - Print a message with certain level in either synchronized way or daemon way.
 * @term: `tty' for synchrnized way or 'dty' for daemon way
 * @level: debug level proposed for this message
 */
extern void debug_print(tty_t *term, int32_t level, char *s);
extern void debug_printf(tty_t *term, int32_t level, char *fmt, ...);

/* dhit_invalidate - Invalidate those data cache lines that willl be referenced between `@rxaddr' and `@rxaddr +`@len'.
 *                   Note that this API shall be used BEFORE initializing a DMA transportation into `@rxaddr'.
 *                   Please note that memory between `@rxaddr' to `@rxaddr' + `@len' could be corrupted if this API was
 *                   used AFTER a DMA transportation complete suppose there're cache lines containing dirty data for
 *                   other memory addresses that would be written back after the DMA transportation.
 * @rxaddr: shall be cache-line aligned
 * @len: `@rxaddr' + `@len' shall be cache-line aligned
 */
extern void dhit_invalidate(uint32_t rxaddr, uint32_t len);
/* dhit_write_back - Write back dirty data in those data cache lines that will be referenced between `@txaddr' and
 *                  `@txaddr' + `@len' to memory.
 *                   Note that this API shall be used before initializing a DMA transportation from `@txaddr'
 *                   to somewhere else.
 * @txaddr: shall be cache-line aligned
 * @len: `@txaddr' + `@len' shall be cache-line aligned
 */
extern void dhit_write_back(uint32_t txaddr, uint32_t len);
/* dhit_write_back_invalidate - Write back dirty data in those data cache lines that will be referenced between `@txaddr'
 *                              and `@txaddr' + `@len' to memory, then have them invalidated.
 *                              Note that this API shall be used before initializing a DMA transportation from `@txaddr'
 *                              to somewhere else.
 * @txaddr: shall be cache-line aligned
 * @len: `@txaddr' + `@len' shall be cache-line aligned
 */
extern void dhit_write_back_invalidate(uint32_t txaddr, uint32_t len);

#define cond_delayus(us, cond)	do {					\
	uint32_t start;							\
	uint32_t current;						\
	uint32_t diff;							\
									\
	start = cpu_get_cycle();					\
									\
	while (cond) {							\
		current = cpu_get_cycle();				\
		if (current < start)					\
			diff = ~(start - current);			\
		else							\
			diff = current - start;				\
									\
		if(diff >= (us) * CPU_CYCLE_PER_US)			\
			break;						\
	}								\
} while(0)
extern void sys_delayus(uint32_t us);
/* sys_delayms - Busy wait for approximately `@ms' milli-seconds.
 */
extern void sys_delayms(uint32_t ms);
/* sys_panic - System hang up with a message printed first.
 */
extern void sys_panic(char *fmt, ...);
extern void inet_dump(tty_t *term, uint8_t *p, int32_t len);
extern void sys_dump(tty_t *term, uint8_t *p, uint32_t len);
extern void sys_dump16(tty_t *term, uint16_t *p, uint32_t len);
extern void sys_dump32(tty_t *term, uint32_t *p, uint32_t len);
extern uint32_t dump_mem(tty_t *term, uint32_t idx, uint32_t addr, uint32_t len, uint8_t refresh);
extern void sys_reboot(void);
extern void sys_reboot_write_flash(uint8_t reason, uint8_t sw_rst);

/* For internal usage ONLY.
 */
extern void board_init(void);
extern void printd_init(void);


#if !(defined(LIGHTELF2M) || defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(UNICORN2M))
extern int32_t check_integer(char *buf);
extern int32_t get_ipv6(char *arg, uint16_t ipv6[]);
extern int32_t get_ip(char *arg, uint8_t ip[]);
#endif
extern char * get_hw_version();
extern char * get_sw_version();
extern char * get_boot_version();
extern char * get_build_time();
extern int32_t strtomac(char *str, uint8_t mac[]);
extern const U16 get_zc_sw_version();
extern int32_t strtohex(tty_t *term, char *str, uint8_t *data, int32_t len);
extern void print_circular_data(tty_t *term, uint32_t saddr, uint32_t capacity, uint32_t addr, uint32_t len);
extern void clear_exception_info();

extern void ld_phase_off(void);
extern void ld_set_tx_phase(uint8_t phase);
extern void ld_set_rx_phase(uint8_t phase);
extern void ld_set_rx_phase_zc(uint8_t phase);
extern void ld_set_tx_phase_zc(uint8_t phase);

extern uint32_t get_sys_reset_status();
extern void clr_sys_reset_status();
extern void print_exc(tty_t *term, uint32_t flag_mask);
extern void sys_set_clk_off(uint32_t modules);
extern void sys_set_clk_on(uint32_t modules);

#endif	/* end of _SYS_H */
