/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     Apstask.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */



//#include "app_common.h"
#include "string.h"
#include "meter.h"
#include "app_gw3762.h"
#include "DataLinkGlobal.h"
#include "printf_zc.h"


APPPIB_t  AppPIB_t;
FLASHINFO_t FlashInfo_t;
RUNNING_INFO_t  RunningInfo = 
{
    .BaudRate = BAUDRATE_9600,
};

//调整MAC地址字节顺序
void ChangeMacAddrOrder(U8 *pMac)
{
	U8 buf[6];
    U8 i;
    
	for(i=0;i<6;i++)
	{
		buf[i]= pMac[5-i];
	}
	__memcpy(pMac,buf,6);
}
unsigned short GetWord(unsigned char *ptr)
{
    unsigned short  wValue;

    wValue  = (unsigned short)ptr[0];
    wValue += (unsigned short)ptr[1] << 8;

    return wValue;
}

void  get_info_from_645_frame(U8 *pdata, U16 datalen, U8 *addr, U8 *controlcode, U8 *len, U8 *FENum)
{
    U8 FECount = 0;

    if(NULL == pdata)
        return;

    while(pdata[FECount] == 0xFE)
    {
        FECount++;
    }
    if(addr)
    {
        __memcpy(addr, pdata + FECount + 1, 6);
    }
    if(len)
    {
        *len = pdata[FECount + 9];
    }
    if(controlcode)
    {
        *controlcode = pdata[FECount + 8];
    }
    if(FENum)
    {
        *FENum = FECount;
    }
    return;
}

#if defined(STATIC_MASTER)
void up_cnm_addr_by_read_meter(READ_METER_IND_t *pReadMeterInd, U8 *ScrMeterAddr)
{
    S16  Seq;

    Seq = seach_seq_by_mac_addr(ScrMeterAddr);
    if(Seq != -1)
    {
        WHLPTST //共享存储内容保护
            if(memcmp(WhiteMacAddrList[Seq].CnmAddr, pReadMeterInd->SrcMacAddr, 6) != 0)
            {
                __memcpy(WhiteMacAddrList[Seq].CnmAddr, pReadMeterInd->SrcMacAddr, 6);
                app_printf("record3\n");
            }
        WHLPTED//释放共享存储内容保护
    }
}
#endif
