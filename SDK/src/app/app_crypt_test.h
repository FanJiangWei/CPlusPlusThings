#ifndef APP_CRYPT_TEST_H
#define APP_CRYPT_TEST_H

#include "ZCsystem.h"
#include "aps_layer.h"


extern U8 g_SafeTestMode;

typedef enum{
    e_SHA256_ALGTEST = 1,       //SHA256�㷨����ģʽ
    e_SM3_ALGTEST,              //SM3�㷨����ģʽ
    e_ECC_SIGNTEST,             //ECCǩ������ģʽ
    e_ECC_VERIFYTEST,           //ECC��ǩ����ģʽ
    e_SM2_SIGNTEST,             //SM2ǩ������ģʽ
    e_SM2_VERIFYTEST,           //SM2��ǩ����ģʽ
    e_AES_CBC_ENCTEST,          //AES-CBC���ܲ���ģʽ
    e_AES_CBC_DECTEST,          //AES-CBC���ܲ���ģʽ
    e_AES_GCM_ENCTEST,          //AES-GCM���ܲ���ģʽ
    e_AES_GCM_DECTEST,          //AES-GCM���ܲ���ģʽ
    e_SM4_CBC_ENCTEST,          //SM4-CBC���ܲ���ģʽ
    e_SM4_CBC_DECTEST,          //SM4-CBC���ܲ���ģʽ
}SAFE_TEST_MODE_e;

//��ǩ�������
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
