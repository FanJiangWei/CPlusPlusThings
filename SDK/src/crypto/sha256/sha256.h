/*
 * Copyright: (c) 2021-2030, 2021 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        sha256.h
 * Purpose:     Abstract sha256 interface based upon miracl
 * History:
 *	Dec 1, 2021	tgni	Create
 */

#ifndef __SHA256_H__
#define __SHA256_H__

#include "types.h"

extern int32_t sha_256(uint8_t data[], int32_t len, uint8_t hash[]);

#endif
