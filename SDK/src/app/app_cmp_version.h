#ifndef _APP_CMP_VERSION_H_
#define _APP_CMP_VERSION_H_
#include "sys.h"
#include "types.h"
#include "SHA1.h"
#include "flash.h"
#include "app.h"

#ifdef CMP_VERSION

#define MCUTYPELEN 10
#define READ_IMAGE0_ADDR       (IMAGE0_ADDR-FLASH_BASE_ADDR)
#define READ_IMAGE1_ADDR       (IMAGE1_ADDR-FLASH_BASE_ADDR)

extern SHA1Context SHA1ContextTemp;

typedef struct{
    U8    CPUSeq;           //CPU编号
	U8	  VendorCode[2];    //厂商代码
	U8	  Res1  :3;
    U8	  Day   :5;         //日
    U16	  Month :4;         //月
    U16	  Year  :12;        //年
    U8	  Res2[2];   		//硬件版本
    U8	  SoftVersion[2];   //软件版本
	U8    MCUTypeLen;       //MCU型号长度
	U8    MCUType[MCUTYPELEN];      //MCU型号
}__PACKED QUERY_SOFTVERSION_t;

typedef struct{
    U8    CPUSeq;           //CPU编号
	U32	  StartAddr;        //厂商代码
	U16	  DataLen;
}__PACKED QUERY_CODE_SHA1_t;

void soft_version_packet(QUERY_SOFTVERSION_t *SoftVer);
U8 query_code_sha1(QUERY_CODE_SHA1_t *QCodeSHA1,SHA1Context *context,uint8_t Message_Digest[SHA1HashSize]);

#if defined(STATIC_NODE)
void pro_version_cmp(U8 Ctrl, U8* Addr, U8* Dataunit, U8 Dataunitlen, U8* Updata, U8 Updatalen, U8* Sendbuf, U8* Sendbuflen);
#endif

#endif
#endif

