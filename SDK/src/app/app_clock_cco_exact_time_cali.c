#include "ZCsystem.h"
#include "app_clock_sync_comm.h"
#include "netnnib.h"
#include "types.h"
#include "app_common.h"
#include "app_clock_sync_comm.h"
#include "mem_zc.h"
#include "app_dltpro.h"
#include "printf_zc.h"

#if defined(STATIC_MASTER)
void exact_timer_calibrate_req(U8* pdata,U16 len)
{
    U8	*data = pdata;
	U8 ccomac[6];
	U16	index = 0;
	U8  fenum = 0;
	U8  BCAddr[6] = {0x99,0x99,0x99,0x99,0x99,0x99};
	U8  FfAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);
	
	
	TIMER_CALIBRATE_REQ_t *TimerCalibrateReq_t = NULL;
	U8 macAddr[MACADRDR_LENGTH] = {0};
	U16 RelayDatalen = 0;

	__memcpy(macAddr, data+index, MACADRDR_LENGTH);
	index += MACADRDR_LENGTH;
	//RelayDatalen = (*(data+index)) + (*(data+index+1)<<8);
	RelayDatalen =  split_read_two(data+index,0);
	index += sizeof(U16);
	app_printf("RelayDatalen = %d\n",RelayDatalen);
	if(RelayDatalen > len)
	{
        app_printf("RelayDatalen error\n");
        return ;
	}
	//申请原语空间
	TimerCalibrateReq_t = (TIMER_CALIBRATE_REQ_t*)zc_malloc_mem( sizeof(TIMER_CALIBRATE_REQ_t) + RelayDatalen,"TimerCalibrateReq_t",MEM_TIME_OUT);
    if(TimerCalibrateReq_t == NULL)
    {
        return;
    }
	ChangeMacAddrOrder(macAddr);
	__memcpy(TimerCalibrateReq_t->DstMacAddr,macAddr,MACADRDR_LENGTH);
	__memcpy(TimerCalibrateReq_t->SrcMacAddr,ccomac ,MACADRDR_LENGTH);
	
    if(Check698Frame(data+index,RelayDatalen,&fenum,NULL,NULL) == TRUE)
	{
        __memcpy(TimerCalibrateReq_t->Asdu,data+index+fenum,RelayDatalen-fenum);
	}
	else if(Check645Frame(data+index,RelayDatalen,&fenum,NULL,NULL) == TRUE)
	{
        __memcpy(TimerCalibrateReq_t->Asdu,data+index+fenum,RelayDatalen-fenum);
	}
	else
	{
        app_printf("frame error\n");
        zc_free_mem(TimerCalibrateReq_t);
        return;
	}
	dump_buf(TimerCalibrateReq_t->Asdu,RelayDatalen - fenum);
	TimerCalibrateReq_t->AsduLength = RelayDatalen - fenum;
	//校验帧合法性
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
		if((0 == memcmp(TimerCalibrateReq_t->DstMacAddr,BCAddr,6)) || (0 == memcmp(TimerCalibrateReq_t->DstMacAddr,FfAddr,6)))
		{
			TimerCalibrateReq_t->SendType = e_PROXY_BROADCAST_FREAM_NOACK;//e_FULL_BROADCAST_FREAM_NOACK;
		}
		else
		{
			TimerCalibrateReq_t->SendType = e_UNICAST_FREAM;
		}
	}
	else
	{
		if(0 == memcmp(TimerCalibrateReq_t->DstMacAddr,BCAddr,6))
		{
			TimerCalibrateReq_t->SendType = e_FULL_BROADCAST_FREAM_NOACK;
		}
		else
		{
			TimerCalibrateReq_t->SendType = e_UNICAST_FREAM;
		}
	}
	ApsExactTimeCalibrateRequest(TimerCalibrateReq_t);
	zc_free_mem(TimerCalibrateReq_t);
    
}
#endif
