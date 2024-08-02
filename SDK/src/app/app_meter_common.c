
#include "sys.h"
#include "printf_zc.h"
#include "meter.h"
#include "app_dltpro.h"
#include "app_meter_common.h"
#include "app_gw3762.h"
#include "schtask.h"


ostimer_t *ChangeBaudtimer = NULL;






#if defined(STATIC_NODE)


//STA和CJQ波特率转换，枚举转换数值
U32 baud_enum_to_int(U8 arg_baud_e)
{
	U32 baud = 0;
#if defined(STATIC_NODE)
	baud = arg_baud_e == METER_BAUD_1200?1200:
			arg_baud_e == METER_BAUD_2400?2400:
			arg_baud_e == METER_BAUD_4800?4800:
			arg_baud_e == METER_BAUD_9600?9600:
			arg_baud_e == METER_BAUD_19200?19200:
			arg_baud_e == METER_BAUD_115200?115200:
			arg_baud_e == METER_BAUD_460800?460800:
            arg_baud_e == METER_BAUD_38400?38400:
            arg_baud_e == METER_BAUD_57600?57600:
            arg_baud_e == METER_BAUD_230400?230400:0;
#endif
	return baud;
	
}


//STA和CJQ绑表与搜表查询表
PROT_BAUD_t ProtBaud[BAUD_ARR] =
{
	#if defined(ZC3750STA)
    {DLT645_2007     , METER_BAUD_9600 ,},
    {DLT698          , METER_BAUD_9600 ,},
    {DLT645_2007     , METER_BAUD_2400 ,},
    {DLT698          , METER_BAUD_19200 ,},
    {DLT698          , METER_BAUD_2400 ,},
    {DLT645_2007     , METER_BAUD_115200 ,},
	{DLT698		     , METER_BAUD_115200 ,},
	{DLT698			 , METER_BAUD_38400 ,},
	{DLT698			 , METER_BAUD_57600 ,},
	{DLT698			 , METER_BAUD_230400 ,},
	{DLT698			 , METER_BAUD_460800 ,},
    {DLT698          , METER_BAUD_4800 ,},
    {DLT645_2007     , METER_BAUD_4800 ,},
    {DLT645_2007     , METER_BAUD_1200 ,},
	#endif
    #if defined(ZC3750CJQ2)
    {DLT645_2007     , METER_BAUD_2400 ,},
    {DLT645_2007     , METER_BAUD_9600 ,},
    {DLT698          , METER_BAUD_9600 ,},
    {DLT698          , METER_BAUD_19200 ,},
    {DLT645_2007     , METER_BAUD_115200 ,},
    {DLT698          , METER_BAUD_115200 ,},
    {DLT698          , METER_BAUD_2400 ,},
    {DLT645_1997     , METER_BAUD_1200 ,},
    {DLT645_1997     , METER_BAUD_2400 ,},
    {DLT698          , METER_BAUD_4800 ,},
    {DLT645_2007     , METER_BAUD_4800 ,},
    {DLT645_2007     , METER_BAUD_1200 ,},
    {DLT645_1997     , METER_BAUD_4800 ,},
    {DLT645_1997     , METER_BAUD_9600 ,},
    #endif
};


PROT_BAUD_t ProtBaud_shanghai[BAUD_ARR] =
{
    {DLT698          , METER_BAUD_4800 ,},
    {DLT645_2007     , METER_BAUD_4800 ,},
    {DLT645_2007     , METER_BAUD_2400 ,},
    {DLT698          , METER_BAUD_9600 ,},
    {DLT698          , METER_BAUD_19200 ,},
    {DLT698          , METER_BAUD_2400 ,},
    {DLT645_2007     , METER_BAUD_9600 ,},
	#if defined(ZC3750STA)
    {DLT645_2007     , METER_BAUD_115200 ,},
	{DLT698		     , METER_BAUD_115200 ,},
	{DLT698			 , METER_BAUD_38400 ,},
	{DLT698			 , METER_BAUD_57600 ,},
	{DLT698			 , METER_BAUD_230400 ,},
	{DLT698			 , METER_BAUD_460800 ,},
	#endif
    #if defined(ZC3750CJQ2)
    {DLT645_1997     , METER_BAUD_1200 ,},
    {DLT645_1997     , METER_BAUD_2400 ,},
    #endif
    {DLT645_2007     , METER_BAUD_1200 ,},
    #if defined(ZC3750CJQ2)
    {DLT645_1997     , METER_BAUD_4800 ,},
    {DLT645_1997     , METER_BAUD_9600 ,},
    #endif
};



//根据645协议组帧绑表和搜表帧
void Dlt645Encode(U8* MeterAddr, U8 Protocol,U8 *ReadMeter,U8 *ReadMeterLen)
{
	if(DLT645_2007 == Protocol)
	{
		#if defined(ZC3750STA)
		__memcpy(ReadMeter ,sta_bind_frame_07 , sizeof(sta_bind_frame_07));
		*ReadMeterLen = sizeof(sta_bind_frame_07);
		#elif defined(ZC3750CJQ2)
		Packet645Frame(ReadMeter,ReadMeterLen,MeterAddr,0x11,cjq2_search_item_07,sizeof(cjq2_search_item_07));
		#endif
	}
	else
	{
		#if defined(ZC3750STA)
		__memcpy(ReadMeter ,sta_bind_frame_97 , sizeof(sta_bind_frame_97));
		*ReadMeterLen = sizeof(sta_bind_frame_97);
		#elif defined(ZC3750CJQ2)
		Packet645Frame(ReadMeter,ReadMeterLen,MeterAddr,0x01,cjq2_search_item_97,sizeof(cjq2_search_item_97));
		#endif
	}

	return;
}


void Dlt698Encode(U8* MeterAddr, U8 Protocol,U8 *ReadMeter,U16 *ReadMeterLen)
{
#if defined(ZC3750STA)
    __memcpy(ReadMeter ,sta_bind_frame_698 , sizeof(sta_bind_frame_698));
	*ReadMeterLen = sizeof(sta_bind_frame_698);
#elif defined(ZC3750CJQ2)
    U8 addrtype;

    if(Protocol != DLT698)
    {
        app_printf("not 698\n");
        return ;
    }
    if(FuncJudgeBCD(MeterAddr,6) == TRUE)
    {
	    addrtype = SingleAddr_e;
    }
    else
    {
        addrtype = WildcardAddr_e;
    }
    Packet698Frame(ReadMeter,ReadMeterLen,0x43,addrtype,6,MeterAddr,cjq2_search_item_698,sizeof(cjq2_search_item_698), 0);
#endif
	return;
}



U8 GetAddrByProtocol(U8 protocol ,U8 *data , U16 datalen ,U8 *addr)
{
	
	if(protocol == DLT645_1997)
	{
		if(Check645Frame(data,datalen,NULL,addr,NULL)== FALSE)
        {
            app_printf("check645 err!\n");
            return ERROR;
        }     
        return OK;
	}
	else if(protocol == DLT645_2007)
	{
	    if(Check645Frame(data,datalen,NULL,addr,NULL)== FALSE)
        {
            app_printf("check645 err!\n");
            return ERROR;
        }   
        return OK;
	}
    else if(protocol == DLT698)
    {
        if(Check698Frame(data,datalen,NULL,addr, NULL) == FALSE)
        {
            app_printf("check698 err!\n");
            return ERROR;
        }
        return OK;
    }
    else
    {
        return ERROR;
    }
	
}


#endif
#if defined(STATIC_MASTER)
void cco_serial_baud_init(void)
{
	U32 baud = 0;
	U8 baud_e;

	baud_e = app_ext_info.param_cfg.JZQBaudCfg;
	
	baud = baud_e == APP_GW3762_9600_BPS? BAUDRATE_9600:
			baud_e == APP_GW3762_19200_BPS ? BAUDRATE_19200:
			baud_e == APP_GW3762_38400_BPS ? BAUDRATE_38400:
			baud_e == APP_GW3762_57600_BPS ? BAUDRATE_57600:
			baud_e == APP_GW3762_115200_BPS ? BAUDRATE_115200:BAUDRATE_9600;
	
	if(baud_e > APP_GW3762_115200_BPS || //波特率超出配置范围
		baud_e < APP_GW3762_9600_BPS || 
		app_ext_info.func_switch.JZQBaudSWC == 0)//波特率配置开关未启用
	{
		return;
	}
	
	MeterUartInit(g_meter, baud);
}

#endif
void ChangeMeterBaudCB(struct ostimer_s *ostimer, void *arg)
{
    U32 BaudRate = 0;
    __memcpy(&BaudRate ,(U32 *)arg,sizeof(U32));
	app_printf("Uart1 BaudRate = %d!!!\n",BaudRate);
    if(BaudRate != 0)
    {
        #if defined(ZC3750STA)
        sta_bind_info_t.BaudRate = BaudRate;
        #else
        
        #endif
        MeterUartInit(g_meter, BaudRate);
    }
}



void Modify_ChangeBaudtimer(U32 *BaudRated)
{
    if(ChangeBaudtimer == NULL)
    {
        ChangeBaudtimer = timer_create(50,
	                             0,
	                             TMR_TRIGGER ,
	                             ChangeMeterBaudCB,
	                             BaudRated,
	                             "ChangeMeterBaudCB"
	                            );
    }
    else
    {
        timer_modify(ChangeBaudtimer,
               50,
               0,
               TMR_TRIGGER ,
               ChangeMeterBaudCB,
               BaudRated,
               "ChangeMeterBaudCB",
               TRUE);
    }
    timer_start(ChangeBaudtimer);
}


