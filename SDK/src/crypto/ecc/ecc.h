 /*
 * Copyright: (c) 2021-2030, 2021 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        ecc.h
 * Purpose:     ECDSA implementation with curve brainpool256 based upon miracl
 * History:
 *	Dec 1, 2021	tgni	Create
 */

#ifndef __ECC_H__
#define __ECC_H__

#include "types.h"
#include "miracl.h"

#define ECC_NUMBYTES	32
#define ECC_NUMBITS	(ECC_NUMBYTES * 8)

extern int32_t ecc_init(void); 
extern void ecc_uninit(void);
extern int32_t ecc_generate_key(uint8_t *priv_key, uint8_t *pub_key_x, uint8_t *pub_key_y);
extern int32_t ecc_sign(uint8_t *msg, int32_t len, uint8_t *priv_key, uint8_t *_r, uint8_t *_s);
extern int32_t ecc_verify(uint8_t *msg, int32_t len, uint8_t *pub_key_x, uint8_t *pub_key_y, uint8_t *_r, uint8_t *_s);
extern int32_t ecc_get_share_key(uint8_t privk[], uint8_t pbkx[], uint8_t pbky[], uint8_t sharekey[], int32_t klen);

#endif
