/*
 * Copyright: (c) 2021-2030, 2021 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        sm2.h
 * Purpose:     sm2 implementation for GMT 0003.[1-5]-2012
 * History:
 *	Dec 2, 2021	tgni	Create
 */

#ifndef __SM2_H__
#define __SM2_H__

#include "types.h"
#include <string.h>
#include "types.h"


#define SM2_NUMBYTES	32
#define SM2_NUMBITS	(SM2_NUMBYTES * 8)

extern int32_t sm2_init(void);
extern void sm2_uninit(void);
extern int32_t sm2_generate_key(uint8_t *priv_key, uint8_t *pub_key_x, uint8_t *pub_key_y);
extern int32_t sm2_generate_key2(uint8_t *priv_key, uint8_t *pub_key_x, uint8_t *pub_key_y);
extern int32_t sm2_sign(uint8_t *msg, int32_t len, uint8_t *ZA, uint8_t *rand, uint8_t *priv_key, uint8_t *_r, uint8_t *_s);
extern int32_t sm2_verify(uint8_t *msg, int32_t len, uint8_t *ZA, uint8_t *pub_key_x, uint8_t *pub_key_y, uint8_t *_r, uint8_t *_s);
extern void sm3_hash_za(uint8_t *pkeyx, uint8_t *pkeyy, uint8_t *ZA);
extern int32_t sm2_encryt(uint8_t *randk, uint8_t *pbkx, uint8_t *pbky, uint8_t M[], int32_t klen, uint8_t C[]);
extern int32_t sm2_decryt(uint8_t *prik, uint8_t C[], int32_t Clen, uint8_t M[]);
extern int32_t sm2_get_share_key(uint8_t privk[], uint8_t pbkx[], uint8_t pbky[], uint8_t sharekey[], int32_t klen);

#endif
