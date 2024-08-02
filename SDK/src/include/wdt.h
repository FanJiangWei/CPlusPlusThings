/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	wdt.h
 * Purpose:	SOC's internal watchdog timer interface
 * History:
 *	Jan 07, 2015	tgni    Create
 */
#ifndef __WDT_H__
#define __WDT_H__


#define WDT_GLB_RST		(1UL << 0)	/* reset the whole SOC */
#define WDT_GPIO_RST		(1UL << 1)	/* watchdog timer timeout signal output on GPIO when reset */
#define WDT_POR_RST		(1UL << 2)	/* reset analog power on */

#define WDT_CLK		(50000000)
/* wdt_is_timeout - Determine whether or not the latest reset is triggered by watchdog timer.
 * @return: TRUE if watchdog timer has been triggered
 *          FALSE if watchdog timer has not been triggered yet
 */
extern int32_t wdt_is_timeout(void);
/* wdt_feed - This API shall be invoked periodically to feed watchdog timer as to prevent it
 *            from being triggered.
 */
extern void wdt_feed(void);
/* wdt_open - Setup the watchdog timer functionality. Note that if `WDT_GPIO_RST' is toggled, a gpio device of
 *            type `GPIO_TYPE_WDT' shall be initialized in advanced as to map the watchdog timeout signal output
 *            to certain GPIO pin.
 * @msec: milliseconds for the watchdog timer to trigger
 * @mode: combination of `WDT_GLB_RST', `WDT_GPIO_RST' and `WDT_POR_RST'.
 */
extern void wdt_open(uint32_t msec, int32_t mode);
/* wdt_close - Close the watchdog timer functionality.
 */
extern void wdt_close(void);


#endif /* end of __WDT_H__ */
