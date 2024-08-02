/*
 * Copyright: (c) 2021-2030, 2021 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        sm3.h
 * Purpose:     sm3 for GMT 0004-2012
 * History:
 *
 */
#ifndef __SM3_H__
#define __SM3_H__

#include "types.h"

typedef struct {
	uint32_t state[8];
	uint32_t length;
	uint32_t curlen;
	uint8_t buf[64];
} sm3_state_t;

extern void sm3_init(sm3_state_t *md);
extern void sm3_process(sm3_state_t * md, uint8_t buf[], int32_t len);
extern void sm3_done(sm3_state_t *md, uint8_t *hash);
extern int32_t sm3_256(uint8_t buf[], int32_t len, uint8_t hash[]);

#endif
