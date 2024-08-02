#include "app_crypt_test.h"
#include "crypt.h"
#include "sha256/sha256.h"
#include "ecc/ecc.h"
#include "sm2/sm2.h"
#include "sm2/sm3.h"
#include "printf_zc.h"

/**
 * @brief do_hash_test
 * @param pHeadReq      input:  rand_len(2) + rand(400)
 * @param pHeadRsp      output: hash_len(1) + hash(32)
 * @return  成功：TRUE， 失败：FALSE
 */
U8 do_hash_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    uint8_t *data, hash_len = 32;
    data = pHeadRsp->Asdu;
    pHeadRsp->DataLength = hash_len + 1;
    *data = hash_len;
    data++;

    U16 dataLen = split_read_two(pHeadReq->Asdu, 0); //pHeadReq->Asdu[0] | pHeadReq->Asdu[1];

    if(e_SHA256_ALGTEST == g_SafeTestMode)
    {
        sha_256(pHeadReq->Asdu + 2, dataLen, data);
    }
    else if(e_SM3_ALGTEST == g_SafeTestMode)
    {
        sm3_256(pHeadReq->Asdu + 2, dataLen, data);
    }

    return TRUE;
}

/**
 * @brief do_ecc_sign_test
 * @param pHeadReq      input:  rand_len(2) + rand(400)
 * @param pHeadRsp      output: pkx_len(1) + pkx(32) + pky_len(1) + pky(32) + rlen(1) + r(32) + slen(1) + s(32)
 * @return 成功：TRUE， 失败：FALSE
 */
U8 do_ecc_sign_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    uint8_t priv_key[32];
    U16 dataLen = split_read_two(pHeadReq->Asdu, 0); //pHeadReq->Asdu[0] | pHeadReq->Asdu[1];
    SIGN_RESP_t *resp = (SIGN_RESP_t *)pHeadRsp->Asdu;
    pHeadRsp->DataLength = sizeof(SIGN_RESP_t);

    resp->pkx_len = 32;
    resp->pky_len = 32;
    resp->rlen    = 32;
    resp->slen    = 32;

    if (ecc_init() < 0)
    {
        printf_d("ecc init failed\n");
        return FALSE;
    }

    if (ecc_generate_key(priv_key, resp->pkx, resp->pky) < 0)
    {
        printf_d("ecc generate key failed\n");
        return FALSE;
    }

    if (ecc_sign(pHeadReq->Asdu + 2, dataLen, priv_key, resp->r, resp->s) < 0)
    {
        printf_d("ecc sign failed\n");
        return FALSE;
    }

    ecc_uninit();

    return TRUE;

}

/**
 * @brief do_ecc_verify_test
 * @param pHeadReq      pkx_len(1) + pkx(32) + pky_len(1) + pky(32) + rlen(1) + r(32) + slen(1) + s(32) +  rand_len(2) + rand(400)
 * @param pHeadRsp      output: result_len(1) + result(1)
 * @return  成功：TRUE， 失败：FALSE
 */
U8 do_ecc_verify_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    VERIFY_REQ_t *req = (VERIFY_REQ_t *)pHeadReq->Asdu;

    int32_t ret;

    if (ecc_init() < 0)
    {
        printf_d("ecc init failed\n");
        return FALSE;
    }

    ret = ecc_verify(req->data, req->datalen, req->pkx, req->pky, req->r, req->s);

    pHeadRsp->DataLength = 2;
    pHeadRsp->Asdu[0] = 1;                                          //签名结果长度
    pHeadRsp->Asdu[1] = ret == 0 ? e_verify_err : e_verify_ok;      //签名结果

   ecc_uninit();
   return TRUE;
}

/**
 * @brief do_sm2_sign_test
 * @param pHeadReq      input:  rand_len(2) + rand(400)
 * @param pHeadRsp      output: pkx_len(1) + pkx(32) + pky_len(1) + pky(32) + rlen(1) + r(32) + slen(1) + s(32)
 * @return  成功：TRUE， 失败：FALSE
 */
U8 do_sm2_sign_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    uint8_t priv_key[32], za[32], _rand[32];
    int32_t i;
    U16 dataLen = split_read_two(pHeadReq->Asdu, 0); //pHeadReq->Asdu[0] | pHeadReq->Asdu[1];

    SIGN_RESP_t *resp = (SIGN_RESP_t *)pHeadRsp->Asdu;
    pHeadRsp->DataLength = sizeof(SIGN_RESP_t);

    resp->pkx_len = 32;
    resp->pky_len = 32;
    resp->rlen    = 32;
    resp->slen    = 32;

    if (sm2_init() < 0)
    {
        printf_d("sm2 init failed\n");
        return FALSE;
    }

    if (sm2_generate_key(priv_key, resp->pkx, resp->pky) < 0)
    {
        printf_d("sm2 generate key failed\n");
        return FALSE;
    }


    sm3_hash_za(resp->pkx, resp->pky, za);

    for (i = 0; i < 32; ++i)
            _rand[i] = rand() & 0xff;

    if (sm2_sign(pHeadReq->Asdu + 2, dataLen, za, _rand, priv_key, resp->r, resp->s) < 0)
    {
        printf_d("ecc sign failed\n");
        return FALSE;
    }

    sm2_uninit();

    return TRUE;
}

/**
 * @brief do_sm2_verify_test
 * @param pHeadReq      pkx_len(1) + pkx(32) + pky_len(1) + pky(32) + rlen(1) + r(32) + slen(1) + s(32) +  rand_len(2) + rand(400)
 * @param pHeadRsp      result_len(1) + result(1)
 * @return              成功：TRUE， 失败：FALSE
 */
U8 do_sm2_verify_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    VERIFY_REQ_t *req = (VERIFY_REQ_t *)pHeadReq->Asdu;
    uint8_t za[32];
    int32_t ret;

    if (sm2_init() < 0)
    {
        printf_d("sm2 init failed\n");
        return FALSE;
    }

    sm3_hash_za(req->pkx, req->pky, za);

    ret = sm2_verify(req->data, req->datalen, za, req->pkx, req->pky, req->r, req->s);

    pHeadRsp->DataLength = 2;
    pHeadRsp->Asdu[0] = 1;                                          //签名结果长度
    pHeadRsp->Asdu[1] = ret == 0 ? e_verify_err : e_verify_ok;      //签名结果

   sm2_uninit();
   return TRUE;
}
/**
 * @brief do_comm_enc_test
 * @param pHeadReq      input:  key_len(1) + key(16) + rand_len(2) + rand(400)
 * @param pHeadRsp      output: [mac_len(1) + mac(16)] + iv_len(1) + iv(16|12) + ciper_len(2) + ciper(400)
 * @return              成功：TRUE， 失败：FALSE
 */
U8 do_comm_enc_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    int32_t i, rand_len, times = 100000, ret;
    uint8_t key_len, *key, *plain, *iv, *ciper, *tag, *data, tag_len, iv_len;
    uint8_t alg = 0, mode = 0, order = 0;
    crypt_cfg_t cfg;

    key_len = pHeadReq->Asdu[0];
//    if (msdu->len < key_len) {
//        printf_d("invalid msdu len %d, key_len %d\n", msdu->len, key_len);
//        return;
//    }

    key = pHeadReq->Asdu + 1;

    rand_len = split_read_two(pHeadReq->Asdu + 1 + key_len, 0);

//    if (msdu->len != rand_len) {
//        printf_d("invalid msdu len %d, rand_len %d\n", msdu->len, rand_len);
//        return;
//    }

    plain = pHeadReq->Asdu + 1 + key_len + 2;

    tag_len = (g_SafeTestMode == e_AES_GCM_ENCTEST) ? (1 + 16) : 0 ;
    iv_len  = (g_SafeTestMode == e_AES_GCM_ENCTEST) ? 12 : 16;


    pHeadRsp->DataLength = tag_len + 1 + iv_len + 2 + rand_len;

    data = pHeadRsp->Asdu;

    if (g_SafeTestMode == e_AES_GCM_ENCTEST) {
        *data++ = 16; //tag len
        tag = data;
        data += 16;
    } else {
        tag = NULL;
    }

    *data++ = iv_len;	/* iv len */
    iv = data;
    for (i = 0; i < iv_len; ++i)
        iv[i] = rand() & 0xff;
    data += iv_len;

    split_write_two(data, rand_len, 0);
    data += 2;
    ciper = data;
    data += rand_len;

    if (g_SafeTestMode == e_AES_CBC_ENCTEST) {
        alg = CRYPT_ALG_AES;
        mode = CRYPT_MODE_CBC;
        order = ORDER_NONE;
    } else if (g_SafeTestMode == e_AES_GCM_ENCTEST) {
        alg = CRYPT_ALG_AES;
        mode = CRYPT_MODE_GCM;
        order = ORDER_NONE;
    } else if (g_SafeTestMode == e_SM4_CBC_ENCTEST) {
        alg = CRYPT_ALG_SMS4;
        mode = CRYPT_MODE_CBC;
#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
        order = ORDER_BIT;
#else
        order = ORDER_BYTE | ORDER_WORD;
#endif
    }

    if (mode == CRYPT_MODE_GCM)
        FILL_CRYPT_CFG2(&cfg, ENCRYPT, alg, mode, order, order, key, key_len, iv, iv_len);
    else
        FILL_CRYPT_CFG(&cfg, ENCRYPT, alg, mode, order, order, key, key_len, iv, iv_len);
    if ((ret = crypt_action2(&cfg, NULL, 0, plain, ciper, rand_len, tag, times)) < 0) {
        printf_d("crypt action2 failed %d @%s\n", ret, __func__);
        return FALSE;
    }



    if (1) {
        dty.op->tprintf(&dty, "key:\n");
        sys_dump(&dty, key, key_len);
        dty.op->tprintf(&dty, "iv:\n");
        sys_dump(&dty, iv, iv_len);
        dty.op->tprintf(&dty, "plain:\n");
        sys_dump(&dty, plain, rand_len);
        dty.op->tprintf(&dty, "ciper:\n");
        sys_dump(&dty, ciper, rand_len);
        if (tag) {
            dty.op->tprintf(&dty, "tag:\n");
            sys_dump(&dty, tag, 16);
        }
    }

    printf_d("do comm enc ok\n");

    return TRUE;
}
/**
 * @brief do_comm_dec_test
 * @param pHeadReq      input:  key_len(1) + key(16) + iv_len(1) + iv(16) + ciper_len(2) + ciper(400)
 * @param pHeadRsp      output: plain_len(2) + plain(400)
 * @return               成功：TRUE， 失败：FALSE
 */
U8 do_comm_dec_test(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    int32_t ciper_len, ret;
    uint8_t key_len, *key, *plain, *ciper, iv_len, *iv, *data;
    uint8_t alg = 0, mode = 0, order = 0;
    crypt_cfg_t cfg;

    uint16_t offset = 0;

    key_len = pHeadReq->Asdu[offset++];

//        if (msdu->len < key_len) {
//            printf_d("invalid msdu len %d, key_len %d\n", msdu->len, key_len);
//            return;
//        }

    key = pHeadReq->Asdu + offset;
    offset += key_len;

    iv_len = pHeadReq->Asdu[offset++];

//        if (msdu->len < iv_len) {
//            printf_d("invalid msdu len %d, iv_len %d\n", msdu->len, iv_len);
//            return;
//        }

    iv = pHeadReq->Asdu + offset;
    offset += iv_len;

    ciper_len = split_read_two(pHeadReq->Asdu + offset, 0);
    offset += 2;

//        if (msdu->len != ciper_len) {
//            printf_d("invalid msdu len %d, ciper_len %d\n", msdu->len, ciper_len);
//            return;
//        }

    ciper = pHeadReq->Asdu + offset;
    offset += ciper_len;


    pHeadRsp->DataLength = 2 + ciper_len;

    data = pHeadRsp->Asdu;
    split_write_two(data, ciper_len, 0);
    data += 2;

    plain = data;

    if (g_SafeTestMode == e_AES_CBC_DECTEST) {
        alg = CRYPT_ALG_AES;
        mode = CRYPT_MODE_CBC;
        order = ORDER_NONE;
    } else if (g_SafeTestMode == e_AES_GCM_DECTEST) {
        alg = CRYPT_ALG_AES;
        mode = CRYPT_MODE_GCM;
        order = ORDER_NONE;
    } else if (g_SafeTestMode == e_SM4_CBC_DECTEST) {
        alg = CRYPT_ALG_SMS4;
        mode = CRYPT_MODE_CBC;
#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
        order = ORDER_BIT;
#else
        order = ORDER_BYTE | ORDER_WORD;
#endif
    }

    if (1) {
        dty.op->tprintf(&dty, "alg: %d mode: %d order: %d\n", alg, mode, order);

        dty.op->tprintf(&dty, "key:\n");
        sys_dump(&dty, key, key_len);
        dty.op->tprintf(&dty, "iv:\n");
        sys_dump(&dty, iv, iv_len);
    }

#if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
#else
    FILL_CRYPT_CFG(&cfg, ENCRYPT, alg, mode, order, order, key, key_len, iv, iv_len);
    get_decrypt_key(&cfg, key);
    if (1) {
        dty.op->tprintf(&dty, "outkey:\n");
        sys_dump(&dty, key, key_len);
    }
#endif

    FILL_CRYPT_CFG(&cfg, DECRYPT, alg, mode, order, order, key, key_len, iv, iv_len);

    if ((ret = crypt_action(&cfg, NULL, 0, ciper, plain, ciper_len, NULL)) < 0) {
        printf_d("crypt action failed %d @%s\n", ret, __func__);
        return FALSE;
    }


    if (1) {
        dty.op->tprintf(&dty, "ciper:\n");
        sys_dump(&dty, ciper, ciper_len);
        dty.op->tprintf(&dty, "plain:\n");
        sys_dump(&dty, plain, ciper_len);
    }


    printf_d("do comm dec ok\n");

    return TRUE;
}


/**
 * @brief do_safeTest       安全模式测试，根据测试模式计算对应的结果。将结果通过串口透传给台体。
 * @param pData             接收到的数据
 * @param len               数据长度
 */

//测试报文中的数据域格式，定义如下：
//域                                             字节数
//密钥/MAC(TAG)/哈希结果/验签结果长度                   1
//密钥/MAC(TAG)/哈希结果/验签结果/	                   N1
//IV/公钥 x 长度                                  	1
//IV/公钥 x                                        N2
//公钥 y 长度                                        	1
//公钥 y                                           N3
//签名 r 长度                                     	1
//签名 r                                           N4
//签名 s 长度                                     	1
//签名 s                                           N5
//随机数/明文/密文长度                                2
//随机数/明文/密文                                  N6
//说明：SHA256/SM3算法和ECC/SM2签名测试，平台只下发随机数，数据域只填充随机数长度和随机数。
//验签结果定义：0：失败；1：成功。

U8 do_safeTest(SAFE_TEST_HEADER_t *pHeadReq, SAFE_TEST_HEADER_t *pHeadRsp)
{
    printf_s("do safeTest testMode<%d>\n",g_SafeTestMode);

    U8 ret = FALSE;
    switch (g_SafeTestMode)
    {
    case e_SHA256_ALGTEST:          //SHA256算法测试
    case e_SM3_ALGTEST:             //SM3算法测试
        ret = do_hash_test(pHeadReq, pHeadRsp);
        break;
    case e_ECC_SIGNTEST   :         //ECC签名测试模式
        ret = do_ecc_sign_test(pHeadReq, pHeadRsp);
        break;
    case e_ECC_VERIFYTEST   :         //ECC验签测试模式
        ret = do_ecc_verify_test(pHeadReq, pHeadRsp);
        break;
    case e_SM2_SIGNTEST   :         //SM2签名测试模式
        ret = do_sm2_sign_test(pHeadReq, pHeadRsp);
        break;
    case e_SM2_VERIFYTEST   :         //SM2验签测试模式
        ret = do_sm2_verify_test(pHeadReq, pHeadRsp);
        break;
    case e_AES_CBC_ENCTEST:         //AES-CBC加密测试模式
    {
        ret = do_comm_enc_test(pHeadReq, pHeadRsp);
        break;
    }
    case e_AES_CBC_DECTEST:         //AES-CBC解密测试模式
    {
        ret = do_comm_dec_test(pHeadReq, pHeadRsp);
        break;
    }
    case e_AES_GCM_ENCTEST:         //AES-GCM加密测试模式
    {
        ret = do_comm_enc_test(pHeadReq, pHeadRsp);
        break;
    }
    case e_AES_GCM_DECTEST:         //AES-GCM解密测试模式
    {
        ret = do_comm_dec_test(pHeadReq, pHeadRsp);
        break;
    }
    case e_SM4_CBC_ENCTEST:         //SM4-CBC加密测试模式
    {
        ret = do_comm_enc_test(pHeadReq, pHeadRsp);
        break;
    }
    case e_SM4_CBC_DECTEST:         //SM4-CBC解密测试模式
    {
        ret = do_comm_dec_test(pHeadReq, pHeadRsp);
        break;
    }
    default:
        break;
    }


    return ret;

}





/*
 * 以下部分代码是模拟台体发送测试报文组帧代码
 */
static U16 pack_hash_test_data(U8 *pData, U8  CryptMode)
{
    U16 byteLen  = 0;
    U16 i;
    *((U16 *)pData) = 400;
    byteLen += 2;

    U8 hash[32];

    for(i = 0; i < 400; i++)
    {
        pData[byteLen++] = rand()%0xff;
    }

    if(e_SHA256_ALGTEST == CryptMode)
    {
        sha_256(pData + 2, 400, hash);
    }
    else if(e_SM3_ALGTEST == CryptMode)
    {
        sm3_256(pData + 2, 400, hash);
    }

    printf_s("hash: ");
    dump_buf(hash, 32);

    return byteLen;

}

static U16 pack_sign_test_data(U8 *pData, U8  CryptMode)
{
    U16 byteLen  = 0;
    U16 i;
    *((U16 *)pData) = 400;
    byteLen += 2;
    for(i = 0; i < 400; i++)
    {
        pData[byteLen++] = rand()%0xff;
    }
    return byteLen;
}

static U16 pack_ecc_verify_data(U8 *pData, U8  CryptMode)
{

    static U8 successflag = TRUE;


    uint8_t priv_key[ECC_NUMBYTES];
    U16 i;
    VERIFY_REQ_t *req = (VERIFY_REQ_t *)pData;
    req->datalen = 400;

    req->pkx_len = 32;
    req->pky_len = 32;
    req->rlen    = 32;
    req->slen    = 32;

    printf_s("successflag = %d\n", successflag);

    for(i = 0; i < req->datalen; i++)
    {
        req->data[i] = rand()%0xff;
    }

    if (ecc_init() < 0) {
        printf_s("ecc init failed\n");
        return 0;
    }

    if (ecc_generate_key(priv_key, req->pkx, req->pky) < 0) {
        printf_s("ecc genkey test failed\n");
        return 0;
    }

    if (ecc_sign(req->data, req->datalen, priv_key, req->r, req->s) < 0) {
        printf_s("ecc sign failed\n");
        return 0;
    }

    ecc_uninit();

    if(successflag == FALSE)
    {

        req->pkx[10] += 0x11;
    }

    successflag = !successflag;

    return (400+sizeof(VERIFY_REQ_t));
}


static U16 pack_sm2_verify_data(U8 *pData, U8  CryptMode)
{
    static U8 successflag = TRUE;

    uint8_t priv_key[32], za[32], _rand[32];
    int32_t i;
    VERIFY_REQ_t *req = (VERIFY_REQ_t *)pData;
    req->datalen = 400;
    req->pkx_len = 32;
    req->pky_len = 32;
    req->rlen    = 32;
    req->slen    = 32;

    if (sm2_init() < 0)
    {
        printf_d("sm2 init failed\n");
        return 0;
    }

    if (sm2_generate_key(priv_key, req->pkx, req->pky) < 0)
    {
        printf_d("sm2 generate key failed\n");
        return 0;
    }

    sm3_hash_za(req->pkx, req->pky, za);

    for (i = 0; i < 32; ++i)
            _rand[i] = rand() & 0xff;

    if (sm2_sign(req->data, req->datalen, za, _rand, priv_key, req->r, req->s) < 0)
    {
        printf_d("ecc sign failed\n");
        return 0;
    }

    sm2_uninit();


    printf_s("successflag = %d\n", successflag);

    if(successflag == FALSE)
    {
        req->pkx[10] += 0x11;
    }

    successflag = !successflag;

    return (req->datalen+sizeof(VERIFY_REQ_t));

}

/**
 * @brief pack_enc_test_data
 * @param pData             key_len(1) + key(16) + rand_len(2) + rand(400)
 * @param CryptMode
 */
static U16 pack_enc_test_data(U8 *pData, U8  CryptMode)
{
    uint8_t *key;
    uint8_t *data;
    int32_t i;

    int32_t offset = 0;
    U16 datalen = 400;


    //填写秘钥
    pData[offset++] = 16;
    key = pData + offset;
    for(i = 0; i < 16; i++)
    {
        pData[offset++] = rand()%0xff;
    }


    //填充随机数
    split_write_two(pData + offset, datalen, 0);
    offset += 2;

    data = pData + offset;

    for(i = 0; i < datalen; i++)
    {
        pData[offset++] = rand()%0xff;
    }

    {//计算加密结果
        uint8_t alg = 0, mode = 0, order = 0;
        crypt_cfg_t cfg;
        int32_t times = 100000, ret;

        uint8_t iv[16];
        uint8_t tag[16];
        uint8_t ciper[400];
        U16 tag_len = 0;
        U8  iv_len = 16;

        mode = 0;

        if (CryptMode == e_AES_CBC_ENCTEST) {
            alg = CRYPT_ALG_AES;
            mode = CRYPT_MODE_CBC;
            order = ORDER_NONE;
        } else if (CryptMode == e_AES_GCM_ENCTEST) {
            alg = CRYPT_ALG_AES;
            mode = CRYPT_MODE_GCM;
            order = ORDER_NONE;
            tag_len = 16;
            iv_len  = 12;
        } else if (CryptMode == e_SM4_CBC_ENCTEST) {
            alg = CRYPT_ALG_SMS4;
            mode = CRYPT_MODE_CBC;
    #if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
            order = ORDER_BIT;
    #else
            order = ORDER_BYTE | ORDER_WORD;
    #endif
        }


        for (i = 0; i < iv_len; ++i)
            iv[i] = i&0xff;//rand() & 0xff;

        if (mode == CRYPT_MODE_GCM)
            FILL_CRYPT_CFG2(&cfg, ENCRYPT, alg, mode, order, order, key, 16, iv, iv_len);
        else
            FILL_CRYPT_CFG(&cfg, ENCRYPT, alg, mode, order, order, key, 16, iv, iv_len);
        if ((ret = crypt_action2(&cfg, NULL, 0, data, ciper, datalen, tag, times)) < 0) {
            printf_d("crypt action2 failed %d @%s\n", ret, __func__);
            return 0;
        }



        if (1) {
            dty.op->tprintf(&dty, "key:\n");
            sys_dump(&dty, key, 16);
            dty.op->tprintf(&dty, "iv:\n");
            sys_dump(&dty, iv, iv_len);
            dty.op->tprintf(&dty, "plain:\n");
            sys_dump(&dty, data, datalen);
            dty.op->tprintf(&dty, "ciper:\n");
            sys_dump(&dty, ciper, datalen);
            if (tag_len) {
                dty.op->tprintf(&dty, "tag:\n");
                sys_dump(&dty, tag, tag_len);
            }
        }

        printf_d("do comm enc ok\n");

    }



    return (1+16+2+400);

}
/**
 * @brief pack_dec_test_data
 * @param pData         key_len(1) + key(16) + iv_len(1) + iv(16) + ciper_len(2) + ciper(400)
 * @param CryptMode
 * @return
 */
static U16 pack_dec_test_data(U8 *pData, U8  CryptMode)
{
    uint8_t *key;
    U8 key_len = 16;
    uint8_t *iv;
    U8 iv_len = 16;

    uint8_t plain[400];

    uint8_t *data;
    int32_t i;

    int32_t offset = 0;
    U16 datalen = 400;

    for(i = 0; i < 400; i++)
    {
        plain[i] = rand()%0xff;
    }


    if(CryptMode == e_AES_GCM_DECTEST)
        iv_len =12;

    pData[offset++] = key_len;
    key = pData + offset;
    offset += key_len;
    pData[offset++] = iv_len;
    iv = pData + offset;
    offset += iv_len;

    split_write_two(pData + offset, datalen, 0);
    offset+=2;
    data = pData + offset;

    {//数据加密
        uint8_t alg = 0, mode = 0, order = 0;
        crypt_cfg_t cfg;
        int32_t ret;
        uint8_t tag[16];
        U16 tag_len = 0;
        uint8_t *ptag = NULL;

        order = 0;

        if (CryptMode == e_AES_CBC_DECTEST) {
            alg = CRYPT_ALG_AES;
            mode = CRYPT_MODE_CBC;
            order = ORDER_NONE;
        } else if (CryptMode == e_AES_GCM_DECTEST) {
            alg = CRYPT_ALG_AES;
            mode = CRYPT_MODE_GCM;
            order = ORDER_NONE;
            tag_len = 16;
            iv_len  = 12;
            ptag = tag;
        } else if (CryptMode == e_SM4_CBC_DECTEST) {
            alg = CRYPT_ALG_SMS4;
            mode = CRYPT_MODE_CBC;
    #if defined(FPGA_VERSION) || defined(VENUS_V2) || defined(VENUS_V3)
            order = ORDER_BIT;
    #else
            order = ORDER_BYTE | ORDER_WORD;
    #endif
        }

        for (i = 0; i < iv_len; ++i)
            iv[i] = i&0xff;//rand() & 0xff;

        for (i = 0; i < key_len; ++i)
            key[i] = i&0xff;//rand() & 0xff;

//        if (mode == CRYPT_MODE_GCM)
//            FILL_CRYPT_CFG2(&cfg, ENCRYPT, alg, mode, order, order, key, 16, iv, iv_len);
//        else
        FILL_CRYPT_CFG(&cfg, ENCRYPT, alg, mode, order, order, key, 16, iv, iv_len);

        if ((ret = crypt_action(&cfg, NULL, 0, plain, data, datalen, ptag)) < 0) {
            printf_d("crypt action2 failed %d @%s\n", ret, __func__);
            return 0;
        }

        if (1) {
            dty.op->tprintf(&dty, "key:\n");
            sys_dump(&dty, key, 16);
            dty.op->tprintf(&dty, "iv:\n");
            sys_dump(&dty, iv, iv_len);
            dty.op->tprintf(&dty, "plain:\n");
            sys_dump(&dty, plain, datalen);
            dty.op->tprintf(&dty, "ciper:\n");
            sys_dump(&dty, data, datalen);
            if (tag_len) {
                dty.op->tprintf(&dty, "tag:\n");
                sys_dump(&dty, tag, tag_len);
            }
        }
    }

    return (1 + key_len + 1 + iv_len + 2 + datalen);
}


U16 packCryptData(U8 CryptMode, SAFE_TEST_HEADER_t *head)
{

    U16 dataLen = 0;

    head->ProtocolVer   = 0;
    head->HeaderLen     = sizeof(SAFE_TEST_HEADER_t);
    head->ProtocolType  = CryptMode;

    //TODO
    switch(CryptMode)
    {
    case e_SHA256_ALGTEST:          //SHA256算法测试
    case e_SM3_ALGTEST:             //SM3算法测试
        dataLen = pack_hash_test_data(head->Asdu, CryptMode);
        break;
    case e_ECC_SIGNTEST   :         //ECC签名测试模式
        dataLen = pack_sign_test_data(head->Asdu, CryptMode);
        break;
    case e_ECC_VERIFYTEST   :         //ECC验签测试模式
        dataLen = pack_ecc_verify_data(head->Asdu, CryptMode);
        break;
    case e_SM2_SIGNTEST   :         //SM2签名测试模式
        dataLen = pack_sign_test_data(head->Asdu, CryptMode);
        break;
    case e_SM2_VERIFYTEST   :         //SM2验签测试模式
        dataLen = pack_sm2_verify_data(head->Asdu, CryptMode);
        break;
    case e_AES_CBC_ENCTEST:         //AES-CBC加密测试模式
        dataLen = pack_enc_test_data(head->Asdu, CryptMode);
        break;
    case e_AES_CBC_DECTEST:         //AES-CBC解密测试模式
        dataLen = pack_dec_test_data(head->Asdu, CryptMode);
        break;
    case e_AES_GCM_ENCTEST:         //AES-GCM加密测试模式
        dataLen = pack_enc_test_data(head->Asdu, CryptMode);
        break;
    case e_AES_GCM_DECTEST:         //AES-GCM解密测试模式
        dataLen = pack_dec_test_data(head->Asdu, CryptMode);
        break;
    case e_SM4_CBC_ENCTEST:         //SM4-CBC加密测试模式
        dataLen = pack_enc_test_data(head->Asdu, CryptMode);
        break;
    case e_SM4_CBC_DECTEST:         //SM4-CBC解密测试模式
        dataLen = pack_dec_test_data(head->Asdu, CryptMode);
        break;
    default:
        break;
    }

    head->DataLength = dataLen;

    return dataLen;
}

