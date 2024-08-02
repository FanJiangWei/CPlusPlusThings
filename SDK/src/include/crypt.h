/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        acu.h
 * Purpose:     acu module function
 * History:
 *	Jan 21, 2015	tgni    Create
 */
#ifndef __CRYPT_H__
#define __CRYPT_H__

#include "types.h"

#if defined(UNICORN2M) || defined(UNICORN8M) || defined(LIGHTELF2M) || defined(LIGHTELF8M)
#define __localtext
#define __localbss
#define __localdata
#else
#define __localtext __LOCALTEXT
#define __localbss  __LOCALBSS
#define __localdata __LOCALDATA
#endif
#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3) || defined(VENUS_V4) || defined(VENUS_V5)
#define DECRYPT                 (1)
#define ENCRYPT                 (0)
#else
#define DECRYPT                 (0)
#define ENCRYPT                 (1)
#endif

#define CRYPT_ALG_AES           (0)
#define CRYPT_ALG_SMS4          (1)

#define CRYPT_MODE_ECB          (0)
#define CRYPT_MODE_CBC          (1)
#define CRYPT_MODE_CTR		(2)
#define CRYPT_MODE_GCM		(3)

#define ORDER_NONE              (0)
#define ORDER_BIT               (1)
#define ORDER_BYTE              (2)
#define ORDER_WORD              (4)

typedef struct crypt_cfg_s {
	uint32_t enc		:8;	/* 0: ENC; 1: DEC */
	uint32_t alg		:4;	/* 0: AES 1: SMS4 2-15:Resv */
	uint32_t mode		:4;	/* 0: ECB 1: CBC 2: CTR 3: GCM */
	uint32_t in_order	:3;	/* 0b000: none; 0b001: bit; 0b010: byte; 0b100: word */
	uint32_t out_order	:3;	/* same as in_order */
	uint32_t auto_iv	:1;	/* auto iv++, 0: no, 1: yes; aes-gcm encrypt only */
	uint32_t		:9;

	uint8_t *key;
	int32_t  key_len;		/* key len: 128, 192 or 256 */

	uint8_t *iv;
	int32_t iv_len;
} crypt_cfg_t;


#define FILL_CRYPT_CFG(cfg, _enc, _alg, _mode, _in_order, _out_order, _key, _key_len, _iv, _iv_len) \
do {								\
	(cfg)->enc       = _enc;				\
	(cfg)->alg       = _alg;				\
	(cfg)->mode      = _mode;				\
	(cfg)->in_order  = _in_order;				\
	(cfg)->out_order = _out_order;				\
	(cfg)->key       = _key;				\
	(cfg)->key_len   = _key_len;				\
	(cfg)->iv        = _iv;					\
	(cfg)->iv_len    = _iv_len;				\
	(cfg)->auto_iv   = 0;					\
} while(0)

#define FILL_CRYPT_CFG2(cfg, _enc, _alg, _mode, _in_order, _out_order, _key, _key_len, _iv, _iv_len) \
do {								\
	(cfg)->enc       = _enc;				\
	(cfg)->alg       = _alg;				\
	(cfg)->mode      = _mode;				\
	(cfg)->in_order  = _in_order;				\
	(cfg)->out_order = _out_order;				\
	(cfg)->key       = _key;				\
	(cfg)->key_len   = _key_len;				\
	(cfg)->iv        = _iv;					\
	(cfg)->iv_len    = _iv_len;				\
	(cfg)->auto_iv   = 1;					\
} while(0)

extern int32_t crypt_action(crypt_cfg_t *cfg, void *add, int32_t alen, void *in, void *out, uint32_t dlen, uint8_t *tag);
extern int32_t crypt_action2(crypt_cfg_t *cfg, void *add, int32_t alen, void *in, void *out, uint32_t dlen, uint8_t *tag, int32_t times);
#ifndef FPGA_VERSION
extern void get_decrypt_key(crypt_cfg_t *cfg, uint8_t *outkey);
#endif
extern void crypt_init(void);
extern uint32_t get_random(void);

#endif /* __CRYPT_H__ */
