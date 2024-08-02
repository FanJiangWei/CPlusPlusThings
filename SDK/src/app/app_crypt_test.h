#ifndef APP_CRYPT_TEST_H
#define APP_CRYPT_TEST_H

#include "ZCsystem.h"
#include "aps_layer.h"


extern U8 g_SafeTestMode;

typedef enum{
    e_SHA256_ALGTEST = 1,       //SHA256算法测试模式
    e_SM3_ALGTEST,              //SM3算法测试模式
    e_ECC_SIGNTEST,             //ECC签名测试模式
    e_ECC_VERIFYTEST,           //ECC验签测试模式
    e_SM2_SIGNTEST,             //SM2签名测试模式
    e_SM2_VERIFYTEST,           //SM2验签测试模式
    e_AES_CBC_ENCTEST,          //AES-CBC加密测试模式
    e_AES_CBC_DECTEST,          //AES-CBC解密测试模式
    e_AES_GCM_ENCTEST,          //AES-GCM加密测试模式
    e_AES_GCM_DECTEST,          //AES-GCM解密测试模式
    e_SM4_CBC_ENCTEST,          //SM4-CBC加密测试模式
    e_SM4_CBC_DECTEST,          //SM4-CBC解密测试模式
}SAFE_TEST_MODE_e;

//验签结果定义
typedef enum{
    e_verify_ok = 0,
    e_verify_err = 1,
}VERIFY_RESULT_e;


typedef struct{
    uint8_t pkx_len;
    uint8_t pkx[32];
    uint8_t pky_len;
    uint8_t pky[32];
    uint8_t rlen;
    uint8_t r[32];
    uint8_t slen;
    uint8_t s[32];
}__PACKED SIGN_RESP_t;

typedef struct{
    uint8_t pkx_len;
    uint8_t pkx[32];
    uint8_t pky_len;
    uint8_t pky[32];
    uint8_t rlen;
    uint8_t r[32];
    uint8_t slen;
    uint8_t s[32];
    uint16_t datalen;
    uint8_t data[0];
}__PACKED VERIFY_REQ_t;


U8 do_hash_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);
U8 do_ecc_sign_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);
U8 do_ecc_verify_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);
U8 do_sm2_sign_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);
U8 do_comm_enc_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);
U8 do_comm_dec_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);





U8 do_safeTest(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp);



U16 packCryptData(U8 CryptMode, SAFE_TEST_HEADER_t *head);

#endif // APP_CRYPT_TEST_H
