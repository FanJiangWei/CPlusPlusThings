/*
 * Copyright: (c) 2020-2030, 2022 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        crypto_demo.c
 * Purpose:     crypto demo
 * History:     
 */

#include <string.h>
#include "crypt.h"
#include "sys.h"
#include "vsh.h"

#if defined(MIRACL)
#include "ecc/ecc.h"
#include "sm2/sm2.h"
#include "sm2/sm3.h"
#include "sha256/sha256.h"

#define SM3_LEN			(256)
#define SHA256_BLOCK_SIZE	(32)

static uint8_t rand_msg[400];
static uint8_t priv_key[ECC_NUMBYTES];
static uint8_t pub_key_x[ECC_NUMBYTES];
static uint8_t pub_key_y[ECC_NUMBYTES];
static uint8_t rr[ECC_NUMBYTES];
static uint8_t ss[ECC_NUMBYTES];

void ecc_genkey_test(void)
{
	int32_t i;

	if (ecc_init() < 0) {
		printf_s("ecc init failed\n");
		return;
	}

	if (ecc_generate_key(priv_key, pub_key_x, pub_key_y) < 0) {
		printf_s("ecc genkey test failed\n");
		return;
	}

	printf_s("private key:\n");
	for (i = 0; i < ECC_NUMBYTES; ++i) {
		printf_s("%02x", priv_key[i]);
	}
	printf_s("\n");

	printf_s("public key:\n");
	for (i = 0; i < ECC_NUMBYTES; ++i) {
		printf_s("%02x", pub_key_x[i]);
	}
	printf_s("\n");
	for (i = 0; i < ECC_NUMBYTES; ++i) {
		printf_s("%02x", pub_key_y[i]);
	}
	printf_s("\n");

	ecc_uninit();

	return;
}

void ecc_sv_test(void)
{
	int32_t i;
	uint32_t time = cpu_get_cycle();

	if (ecc_init() < 0) {
		printf_s("ecc init failed\n");
		return;
	}

	if (ecc_generate_key(priv_key, pub_key_x, pub_key_y) < 0) {
		printf_s("ecc genkey test failed\n");
		return;
	}
	printf_d("gen key time : %dus\n", time_diff(cpu_get_cycle(), time) / CPU_CYCLE_PER_US);

	for (i = 0; i < 400; ++i) {
		rand_msg[i] = rand() & 0xff;
	}

	time = cpu_get_cycle();
	if (ecc_sign(rand_msg, 400, priv_key, rr, ss) < 0) {
		printf_s("ecc sign failed\n");
		return;
	}
	printf_d("sign time : %dus\n", time_diff(cpu_get_cycle(), time) / CPU_CYCLE_PER_US);

	time = cpu_get_cycle();
	if (ecc_verify(rand_msg, 400, pub_key_x, pub_key_y, rr, ss) < 0) {
		printf_s("ecc verification failed\n");
		return;
	}
	printf_d("verify time : %dus\n", time_diff(cpu_get_cycle(), time) / CPU_CYCLE_PER_US);

	printf_d("Signature is verified\n");

	ecc_uninit();

	return;
}

param_t cmd_ecc_param_tab[] = {
	{PARAM_ARG|PARAM_TOGGLE, "", "genkey|sv"},
	PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_ecc_desc,
	"ecc enc/dec test" , cmd_ecc_param_tab
	);

void docmd_ecc(command_t *cmd, xsh_t *xsh)
{
	if (__strcmp(xsh->argv[1], "genkey") == 0) {
		ecc_genkey_test();
	} else if (__strcmp(xsh->argv[1], "sv") == 0) {
		ecc_sv_test();
	}

	return;
}


//the private key
static uint8_t dA[32] = {
	0x39,0x45,0x20,0x8f,0x7b,0x21,0x44,0xb1,0x3f,0x36,0xe3,0x8a,0xc6,0xd3,0x9f,0x95,
	0x88,0x93,0x93,0x69,0x28,0x60,0xb5,0x1a,0x42,0xfb,0x81,0xef,0x4d,0xf7,0xc5,0xb8
};
static uint8_t _rand[32] = {
	0x59,0x27,0x6E,0x27,0xD5,0x06,0x86,0x1A,0x16,0x68,0x0F,0x3A,0xD9,0xC0,0x2D,0xCC,
	0xEF,0x3C,0xC1,0xFA,0x3C,0xDB,0xE4,0xCE,0x6D,0x54,0xB8,0x0D,0xEA,0xC1,0xBC,0x21
};
//the public key
static uint8_t xA[32] = {
	0x09,0xf9,0xdf,0x31,0x1e,0x54,0x21,0xa1,0x50,0xdd,0x7d,0x16,0x1e,0x4b,0xc5,0xc6,
	0x72,0x17,0x9f,0xad,0x18,0x33,0xfc,0x07,0x6b,0xb0,0x8f,0xf3,0x56,0xf3,0x50,0x20
};
static uint8_t yA[32] = {
	0xcc,0xea,0x49,0x0c,0xe2,0x67,0x75,0xa5,0x2d,0xc6,0xea,0x71,0x8c,0xc1,0xaa,0x60,
	0x0a,0xed,0x05,0xfb,0xf3,0x5e,0x08,0x4a,0x66,0x32,0xf6,0x07,0x2d,0xa9,0xad,0x13
};
int32_t sm2_sv_check(void)
{
	char *msg = "message digest";	//the message to be signed
	int32_t len = __strlen(msg);		//the length of message
	uint8_t ZA[SM3_LEN/8];			//ZA=Hash(ENTLA|| IDA|| a|| b|| Gx || Gy || xA|| yA)
	uint8_t r[32], s[32];			// Signature
	int32_t ret = 0;
	uint32_t time;

	time = cpu_get_cycle();

	if (sm2_init() < 0)
		return -1;

	if ((ret = sm2_generate_key2(dA, xA, yA)) < 0)
		return ret;

	printf_d("gen key time : %dus\n", time_diff(cpu_get_cycle(), time) / CPU_CYCLE_PER_US);

	time = cpu_get_cycle();

	sm3_hash_za(xA, yA, ZA);

	if ((ret = sm2_sign((uint8_t *)msg, len, ZA, _rand, dA, r, s)))
		return ret;

	printf_d("sign time : %dus\n", time_diff(cpu_get_cycle(), time) / CPU_CYCLE_PER_US);

	time = cpu_get_cycle();
	if ((ret = sm2_verify((uint8_t *)msg, len, ZA, xA, yA, r, s)) < 0) {
		printf_d("sm2 sv err\n");
		return ret;
	} 

	printf_d("verify time : %dus\n", time_diff(cpu_get_cycle(), time) / CPU_CYCLE_PER_US);
	printf_d("sm2 sign and verify ok\n");

	sm2_uninit();

	return ret;
}

param_t cmd_sm2_param_tab[] = {
	{PARAM_ARG|PARAM_TOGGLE, "", "sv"},
	PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_sm2_desc,
	"sm2 enc/dec test" , cmd_sm2_param_tab
	);

void docmd_sm2(command_t *cmd, xsh_t *xsh)
{
	if (__strcmp(xsh->argv[1], "sv") == 0) {
		sm2_sv_check();
	}
	return;
}

/* 
 * test program: should produce digest  
 * 248d6a61 d20638b8 e5c02693 0c3e6039 a33ce459 64ff2167 f6ecedd4 19db06c1
 */
int32_t sha256_test(void)
{
	char test[]="abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
	uint8_t hash[SHA256_BLOCK_SIZE];
	int32_t i;

	sha_256((uint8_t *)test, __strlen(test), hash);

	for (i = 0; i < 32; i++) 
		printf_s("%02x", hash[i]);
	printf_s("\n");

	return 0;
}

int32_t sha256_test2(void)
{
	char text1[] = {"abc"};
	char text2[] = {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"};
	char text3[] = {"aaaaaaaaaa"};
	uint8_t hash1[SHA256_BLOCK_SIZE] = {0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
		0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad};
	uint8_t hash2[SHA256_BLOCK_SIZE] = {0x24,0x8d,0x6a,0x61,0xd2,0x06,0x38,0xb8,0xe5,0xc0,0x26,0x93,0x0c,0x3e,0x60,0x39,
		0xa3,0x3c,0xe4,0x59,0x64,0xff,0x21,0x67,0xf6,0xec,0xed,0xd4,0x19,0xdb,0x06,0xc1};
	uint8_t hash3[SHA256_BLOCK_SIZE] = {0xcd,0xc7,0x6e,0x5c,0x99,0x14,0xfb,0x92,0x81,0xa1,0xc7,0xe2,0x84,0xd7,0x3e,0x67,
		0xf1,0x80,0x9a,0x48,0xa4,0x97,0x20,0x0e,0x04,0x6d,0x39,0xcc,0xc7,0x11,0x2c,0xd0};
	uint8_t buf[SHA256_BLOCK_SIZE];
	int32_t i, j;
	int32_t pass = 1;

	sha_256((uint8_t *)text1, __strlen(text1), buf);
	pass = pass && !memcmp(hash1, buf, SHA256_BLOCK_SIZE);

	sha_256((uint8_t *)text2, __strlen(text2), buf);
	pass = pass && !memcmp(hash2, buf, SHA256_BLOCK_SIZE);

	sha256 sh;
	shs256_init(&sh);
	for (i = 0; i < 100000; ++i) {
		for (j = 0; j < __strlen(text3); ++j)
			shs256_process(&sh, text3[j]);
	}
	shs256_hash(&sh, (char *)buf);
	pass = pass && !memcmp(hash3, buf, SHA256_BLOCK_SIZE);
	printf_s("sha-256 test : %s\n", pass ? "success" : "fail");

	return(pass);
}


param_t cmd_sha256_param_tab[] = {
	{PARAM_ARG|PARAM_TOGGLE, "", "test|test2" },
	PARAM_EMPTY
};

CMD_DESC_ENTRY(cmd_sha256_desc,
	"sha256 test" , cmd_sha256_param_tab
	);

void docmd_sha256(command_t *cmd, xsh_t *xsh)
{
	if (__strcmp(xsh->argv[1], "test") == 0) {
		sha256_test();
	} else if (__strcmp(xsh->argv[1], "test2") == 0) {
		sha256_test2();
	}

	return;
}

#endif

void sms4_ecb_test(void)
{
	static char keystr[] =  {"0123456789abcdeffedcba9876543210"};
	static char outstr[] =  {"681edf34d206965e86b3e94f536e4246"};
	static char outstr2[] = {"595298c7c6fd271f0402f804c33d3f66"};
	static uint8_t key[16], in[16], out[16], std[16];
	int32_t ret;
	crypt_cfg_t cfg;

	strtohex(&dty, keystr, in, 16);
    strtohex(&dty, keystr, key, 16);
#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_ECB, ORDER_BIT, 
		ORDER_BIT, key, 16, NULL, 0);
#else
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_ECB, ORDER_BYTE | ORDER_WORD, 
		ORDER_BYTE | ORDER_WORD, key, 16, NULL, 0);
#endif

	if ((ret = crypt_action(&cfg, NULL, 0, in, out, 16, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}
	strtohex(&dty, outstr, std, 16);

	printf_d("enc 1:\n");
	printf_d("key: ");
	sys_dump(&dty, key, 16);
	printf_d("in : ");
	sys_dump(&dty, in, 16);
	printf_d("out: ");
	sys_dump(&dty, out, 16);
	printf_d("std: ");
	sys_dump(&dty, std, 16);

	if (memcmp(out, std, 16) != 0) {
		printf_d("ecb/sms4 encryption once failed\n");
		return;
	}
	printf_d("ecb/sms4 encryption once success\n");

#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
	FILL_CRYPT_CFG(&cfg, DECRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_ECB, ORDER_BIT, 
		ORDER_BIT, key, 16, NULL, 0);
#else
	get_decrypt_key(&cfg, key);
	FILL_CRYPT_CFG(&cfg, DECRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_ECB, ORDER_BYTE | ORDER_WORD, 
		ORDER_BYTE | ORDER_WORD, key, 16, NULL, 0);
#endif
	if ((ret = crypt_action(&cfg, NULL, 0, out, out, 16, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}

	printf_d("dec 1:\n");
	printf_d("key: ");
	sys_dump(&dty, key, 16);
	printf_d("in : ");
	sys_dump(&dty, in, 16);
	printf_d("out: ");
	sys_dump(&dty, out, 16);

	if (memcmp(out, in, 16) != 0) {
		printf_d("ecb/sms4 decryption once failed\n");
		return;
	}
	printf_d("ecb/sms4 decryption once success\n");

#ifdef FPGA_VERSION
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_ECB, ORDER_BIT, 
		ORDER_BIT, key, 16, NULL, 0);
#else
	strtohex(&dty, keystr, key, 16);
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_ECB, ORDER_BYTE | ORDER_WORD, 
		ORDER_BYTE | ORDER_WORD, key, 16, NULL, 0);
#endif
	if ((ret = crypt_action2(&cfg, NULL, 0, in, out, 16, NULL, 1000000)) < 0) {
		printf_d("crypt action2 failed %d\n", ret);
		return;
	}

	strtohex(&dty, outstr2, std, 16);

	printf_d("enc 1000,000:\n");
	printf_d("key: ");
	sys_dump(&dty, key, 16);
	printf_d("in : ");
	sys_dump(&dty, in, 16);
	printf_d("out: ");
	sys_dump(&dty, out, 16);
	printf_d("std: ");
	sys_dump(&dty, std, 16);

	if (memcmp(out, std, 16) != 0) {
		printf_d("ecb/sms4 encryption 1000,000 times failed\n");
		return;
	}

	printf_d("ecb/sms4 encryption OK!\n");
	return;
}

void sms4_cbc_test1(void)
{
	static char instr[]  = {"AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDDEEEEEEEEFFFFFFFFAAAAAAAABBBBBBBB"};
	static char outstr[] = {"78EBB11CC40B0A48312AAEB2040244CB4CB7016951909226979B0D15DC6A8F6D"};
	static char keystr[] = {"0123456789abcdeffedcba9876543210"};
	static char ivstr[]  = {"000102030405060708090a0b0c0d0e0f"};
	

	static uint8_t key[16], iv[16], in[32], out[32], std[32];
	int32_t ret;
	crypt_cfg_t cfg;

	strtohex(&dty, instr, in, 32);
	strtohex(&dty, keystr, key, 16);
	strtohex(&dty, ivstr, iv, 16);
#ifdef FPGA_VERSION
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_CBC, ORDER_BIT, 
		ORDER_BIT, key, 16, iv, 16);
#else
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_CBC, ORDER_BYTE | ORDER_WORD, 
		ORDER_BYTE | ORDER_WORD, key, 16, iv, 16);
#endif

	if ((ret = crypt_action(&cfg, NULL, 0, in, out, 32, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}
	strtohex(&dty, outstr, std, 32);

	printf_d("enc: \n");
	printf_d("key: ");
	sys_dump(&dty, key, 16);
	printf_d("iv:  ");
	sys_dump(&dty, iv, 16);
	printf_d("in:  \n");
	sys_dump(&dty, in, 32);
	printf_d("out: \n");
	sys_dump(&dty, out, 32);
	printf_d("std: \n");
	sys_dump(&dty, std, 32);

	if (memcmp(out, std, 32) != 0) {
		printf_d("cbc/sms4 encryption case 1 failed\n");
		return;
	}
	printf_d("cbc/sms4 encryption case 1 success\n");

#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
	FILL_CRYPT_CFG(&cfg, DECRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_CBC, ORDER_BIT, 
		ORDER_BIT, key, 16, iv, 16);
#else
	get_decrypt_key(&cfg, key);
	FILL_CRYPT_CFG(&cfg, DECRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_CBC, ORDER_BYTE | ORDER_WORD, 
		ORDER_BYTE | ORDER_WORD, key, 16, iv, 16);
#endif
	if ((ret = crypt_action(&cfg, NULL, 0, out, out, 32, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}

	printf_d("dec:\n");
	printf_d("key: ");
	sys_dump(&dty, key, 16);
	printf_d("in : \n");
	sys_dump(&dty, in, 32);
	printf_d("out: \n");
	sys_dump(&dty, out, 32);

	if (memcmp(out, in, 32) != 0) {
		printf_d("cbc/sms4 decryption case 1 failed\n");
		return;
	}
	printf_d("cbc/sms4 decryption case 1 success\n");

	return;
}

void sms4_cbc_test2(void)
{
	static char instr[]  = {"AAAAAAAABBBBBBBBCCCCCCCCDDDDDDDDEEEEEEEEFFFFFFFFAAAAAAAABBBBBBBB"};
	static char outstr[] = {"0D3A6DDC2D21C698857215587B7BB59A91F2C147911A4144665E1FA1D40BAE38"};
	static char keystr[] = {"FEDCBA98765432100123456789ABCDEF"};
	static char ivstr[]  = {"000102030405060708090a0b0c0d0e0f"};
	

	static uint8_t key[16], iv[16], in[32], out[32], std[32];
	int32_t ret;
	crypt_cfg_t cfg;

	strtohex(&dty, instr, in, 32);
	strtohex(&dty, keystr, key, 16);
	strtohex(&dty, ivstr, iv, 16);
#ifdef FPGA_VERSION
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_CBC, ORDER_BIT, 
		ORDER_BIT, key, 16, iv, 16);
#else
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_SMS4, CRYPT_MODE_CBC, ORDER_BYTE | ORDER_WORD, 
		ORDER_BYTE | ORDER_WORD, key, 16, iv, 16);
#endif

	if ((ret = crypt_action(&cfg, NULL, 0, in, out, 32, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}
	strtohex(&dty, outstr, std, 32);

	printf_d("key: ");
	sys_dump(&dty, key, 16);
	printf_d("iv:  ");
	sys_dump(&dty, iv, 16);
	printf_d("in:  \n");
	sys_dump(&dty, in, 32);
	printf_d("out: \n");
	sys_dump(&dty, out, 32);
	printf_d("std: \n");
	sys_dump(&dty, std, 32);

	if (memcmp(out, std, 32) != 0) {
		printf_d("cbc/sms4 encryption case 2  failed\n");
		return;
	}
	printf_d("cbc/sms4 encryption case 2 success\n");

	return;
}

void aes_ecb_test(void)
{
	char keystr[] = {"2b7e151628aed2a6abf7158809cf4f3c"};
	char instr[] =  {"6bc1bee22e409f96e93d7e117393172a"};
	char outstr[] = {"3ad77bb40d7a3660a89ecaf32466ef97"};
	uint8_t key[16], in[16], out[16];
	int32_t ret;
	crypt_cfg_t cfg;

	strtohex(&dty, keystr, key, 16);
	strtohex(&dty, instr, in, 16);
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_AES, CRYPT_MODE_ECB, ORDER_NONE, 
		ORDER_NONE, key, 16, NULL, 0);
	if ((ret = crypt_action(&cfg, NULL, 0, in, out, 16, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}

	strtohex(&dty, outstr, in, 16);
	if (memcmp(in, out, 16) != 0) {
		printf_d("aes ecb test failed\n");
		return;
	}

	printf_d("aes ecb test success\n");
	return;
}

void aes_cbc_test(void)
{
	char keystr[] = {"2b7e151628aed2a6abf7158809cf4f3c"};
	char ivstr[]  = {"000102030405060708090a0b0c0d0e0f"};
	char instr[] =  {"6bc1bee22e409f96e93d7e117393172a"};
	char outstr[] = {"7649abac8119b246cee98e9b12e9197d"};
	uint8_t key[16], iv[16], in[16], out[16];
	int32_t ret;
	crypt_cfg_t cfg;

	strtohex(&dty, keystr, key, 16);
	strtohex(&dty, ivstr, iv, 16);
	strtohex(&dty, instr, in, 16);
	FILL_CRYPT_CFG(&cfg, ENCRYPT, CRYPT_ALG_AES, CRYPT_MODE_CBC, ORDER_NONE, 
		ORDER_NONE, key, 16, iv, 16);
	if ((ret = crypt_action(&cfg, NULL, 0, in, out, 16, NULL)) < 0) {
		printf_d("crypt action failed %d\n", ret);
		return;
	}

	strtohex(&dty, outstr, in, 16);
	if (memcmp(in, out, 16) != 0) {
		printf_d("aes cbc test failed\n");
		return;
	}

	printf_d("aes cbc test success\n");
	return;
}

void aes_gcm_test_special(void)
{
	crypt_cfg_t cfg;
	int32_t ret;
	static uint8_t key[] = {0x39, 0x60, 0x1b, 0x31, 0x72, 0x7d, 0x2e, 0x12, 0x1c, 0x0b, 0x04, 0x2c, 0x1b, 0x6e, 0x00, 0x71};
	static uint8_t in[400];
	static uint8_t out[400];
	static uint8_t _out[400];
	static uint8_t tag[16];
	static uint8_t _tag[16];
	static char instr[] = {
			"441c314c4f4b115a76557734107a0410"
			"12393c4b754d50684d556040021a1b0b"
			"060c58102b0a601e783e57691307304d"
			"4241126905422c613c2206266f411d08"
			"3f0a6e5677732c220a205a1453420521"
			"086d280b0f170c233b13190b3a032249"
			"6b7e5c7073730f75460133767b741456"
			"462965754361091142661663426e4c5f"
			"092a383f527e7e20501e7c67300f0342"
			"6425496b1d67042f02562b084f3d0e4f"
			"09360a28423c160714095f1477234477"
			"0d1d3e760b406a3450281e49787c0c06"
			"333a024710271f39363d707a7658727d"
			"702977744d0a6d28705c715d0a653f60"
			"177047767c693e6f027c005a482f0a40"
			"4d6410342c765b4c6d443162191c1515"
			"755a3f7b393260727e2925532b503700"
			"742037476b77771540300b3128164736"
			"3711691d123a373079603e01551a1879"
			"186a1d67193a736a0f0904476c6a4216"
			"5912305d3a4e0c3e4a19224018410511"
			"12216815103f5f7a3e7e0005254b343f"
			"6f2e5314540918097c7955791f35462d"
			"7b5c773f634e4454111f62011866304f"
			"0d7744320c07316c3a3b630a120a6821"
		};

	static uint8_t iv[] =  {0xbc, 0xaf, 0xd2, 0xd7, 0xad, 0x31, 0x97, 0x41, 0x9a, 0x75, 0x5b, 0xd3};
	static char outstr[] = {
			"D94C4983FC135AC8518D9417BD66F20C"
			"7722FA1E346E2B087DC52FFADAAC91D8"
			"D8EDDD51C4D7B3ADCF7492EB27779D1A"
			"6A41D2E3F19D9FF963E5707BAD7A6D3A"
			"EF6DF128433EE06BD62802201220EE77"
			"407C35B13E997FB94A1C3A3110091B26"
			"B889376CE1B5E9446457CA2DAB1285C7"
			"00C72A3F768102BA4761EEECA45B1E7E"
			"2F96159FF388B427625C2656C59427FF"
			"C626726E3E94D3EB3BB0B0C076CFBC55"
			"505E49DBC380D646D10DD0A8F6ABD254"
			"F1DFC77EA603A15B12DC5989A19EDB7C"
			"D8A2756CB1DC78FDF77C97BEBED0D792"
			"21399A4423F9B4B8810BD2FC7D3DD6BE"
			"57E35A1D17B510477113EB0BB5A521F3"
			"2875EC85D18A5BDBCCC3CD0511C353FD"
			"ABF43C081AB8A2B2DB21EB5BD7D3FF1D"
			"C2F8313B9995850179092CEA192678B1"
			"55F2148A522710E78ECF2D7AB6F5F6B8"
			"BB5703E7970656752A45EB7AA1A299AD"
			"7D362F8922060014E900EC6C8889BF1B"
			"0D5D0B4CA52027E6C9635C9948430FCA"
			"FDEB553F87A90DC8D750DD6812E10967"
			"846FE291638E6539328C79A179DF16A6"
			"865E4E16D3CE7AE409319EB2DB2144A3"
		};
	static char tagstr[] = {
			"56B3CF37C0A28CB8AC6041E8A0C4EC2D"
		};

	strtohex(&tty, instr, in, 400);
	strtohex(&tty, outstr, _out, 400);
	strtohex(&tty, tagstr, _tag, 16);

	FILL_CRYPT_CFG2(&cfg, ENCRYPT, CRYPT_ALG_AES, CRYPT_MODE_GCM, ORDER_NONE, 
		ORDER_NONE, key, 16, iv, 12);

	if ((ret = crypt_action2(&cfg, NULL, 0, in, out, 400, tag, 100000)) < 0) {
		printf_d("crypt action2 failed %d\n", ret);
		return;
	}

	printf_s("key: ");
	sys_dump(&tty, key, 16);
	printf_s("iv : ");
	sys_dump(&tty, iv, 12);

	printf_s("in : ");
	sys_dump(&tty, in, 400);
	printf_s("out: ");
	sys_dump(&tty, out, 400);
	printf_s("stdOut: ");
	sys_dump(&tty, _out, 400);

	printf_s("tag: ");
	sys_dump(&tty, tag, 16);
	printf_s("stdTag: ");
	sys_dump(&tty, _tag, 16);

	if (memcmp(_out, out, 400) != 0) {
		printf_d("out is different!\n");
		return;
	}

	if (memcmp(_tag, tag, 16) != 0) {
		printf_d("tag is different!\n");
		return;
	}

	printf_d("aes gcm test case special success\n");
}
