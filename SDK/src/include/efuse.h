/*
 * Copyright: (c) 2018-2020, 2018 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	efuse.h
 * Purpose:	efuse manipulation
 * History:
 *	Oct 16, 2018	tgni	Create
 */

#ifndef __EFUSE_H__
#define __EFUSE_H__

#include "types.h"

/* efuse_read - read efuse 128bits data
 * @data: efuse output data
 * @len:  efuse len, must be 4
 */
extern int32_t efuse_read(uint32_t data[], int32_t len);

/* efuse_write - write efuse 128bits data
 * @data: efuse input data
 * @len:  efuse len, must be 4
 * Note:
 *	write permanently one time, given 2.5V outside
 */
extern int32_t efuse_write(uint32_t data[], int32_t len);

#endif /* end of __EFUSE_H__ */

