/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        crc.h
 * Purpose:     crc function
 * History:
 *	Jan 26, 2015	tgni    Create
 */
#ifndef __CRC_H__
#define __CRC_H__

#include "types.h"

#define CRC_24          (0)
#define CRC_32          (1)

#define CRC_HW          (0)
#define CRC_SW          (2)

#define CRC_LEN_MASK     1
#define CRC_CAC_MASK     2

extern uint32_t crc_update(uint32_t base, uint8_t *data, uint32_t size);
extern int32_t crc_digest(uint8_t *data, uint32_t size, uint32_t mode, uint32_t *out);
extern int32_t crc_init(void);

#endif /* __CRC_H__ */
