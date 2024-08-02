/*
 * Copyright: (c) 20014-2020, 2015 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	softdog.h
 * Purpose:	Bring out CPU from busy-waiting via an exception.
 * History:
 *	Apr 14, 2015	ypzhou	Create
 */

#ifndef __SOFTDOG_H__
#define __SOFTDOG_H__

#include "types.h"

typedef void (*softdog_callback_fp)(void);

extern void softdog_init(void);
/* softdog_start - Start the softdog which will be feed automatically as to prevent it being triggered.
 * @timeout: in unit of second
 * @return: 0 if succeed
 *          -EINVAL if argument error
 *          -ENODEV if fail
 */
extern int32_t softdog_start(uint32_t timeout, softdog_callback_fp callback);
/* softdog_stop - Stop the softdog.
 */
extern void softdog_stop(void);


#endif	/* end of __SOFTDOG_H__ */
