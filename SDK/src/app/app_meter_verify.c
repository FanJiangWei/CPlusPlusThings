/*
* Copyright: (c) 2009-2022, 2022 ZC Technology, Inc.
* All Rights Reserved.
*
* File:	app_communication_verify.c
* Purpose:	
* History:
* Author : WWJ
*/
#include "softdog.h"
#include "wdt.h"
#include "ZCsystem.h"
#include "app_base_serial_cache_queue.h"
#include "aps_layer.h"
#include "app_exstate_verify.h"
#include "app_698p45.h"
#include "app_698client.h"
#include "app_power_onoff.h"
#include "app_dltpro.h"
#include "app_meter_common.h"
#include "app_meter_verify.h"
#include "app_cnf.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "netnnib.h"
#include "meter.h"
#include "app_read_cjq_list_sta.h"


#if defined(ZC3750STA)

COMU_VERIFY_t ComuVerify = 
{
    .timer = NULL,
    .State = VerifyCycle_e,
    .Cnt = 0,
    .PlcVerifyCnt = 0,
    .NextBaudRate = 0,
};




void ResetRecordComuCheck(void)
{
    ComuVerify.Cnt = 0;
}

void RecordComuCheck(void)
{
	U8  MacType;
    ComuVerify.Cnt ++;
    if(ComuVerify.Cnt > METERCOMUMAXCNT)
    {
        ResetRecordComuCheck();
        //判断是否需要切换波特率验表
        if(ComuVerify.NextBaudRate == 0)
        {
            MacType = e_UNKONWN;
            if(AppNodeState == e_NODE_ON_LINE)
            {
                //需要离线
                net_nnib_ioctl(NET_SET_OFFLINE,NULL);
                net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
            }
            else
            {
				net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
            }
            if(ComuVerify.State < VerifyFirst_e || ComuVerify.State > VerifyCoErr_e)
		    {
		        app_printf("MeterCheck err %d\n",ComuVerify.State);
		        return;
		    }
            MeterVerifyProc[ComuVerify.State].FailProc();
        }
        else
        {
            sta_bind_info_t.BaudRate = ComuVerify.NextBaudRate;
            ComuVerify.NextBaudRate = 0;
            //切换波特率，并且清除之前波特率
            //MeterUartInit(g_meter, sta_bind_info_t.BaudRate);
            if(sta_bind_info_t.BaudRate <= 9600 && ComuVerify.State != VerifyZero_e)
            {
                sta_bind_info_t.PrtclOop20 = 0;
                high_baud = FALSE;
            }
            app_printf("BaudRate = %d\n" , sta_bind_info_t.BaudRate);
            Modify_MeterVerifytimer(1);
            
        }
        
    }
    else
    {
        Modify_MeterVerifytimer(1);
    }

}

void MeterVerifyCB(struct ostimer_s *ostimer, void *arg)
{
    if(check_power_state() == FALSE&&getHplcTestMode()==NORM)
    {
        app_printf("Need MeterComuJudge!\n");
        if(FALSE == change_address_flag)
        {
            sta_bind_check_meter(DevicePib_t.DevMacAddr,DevicePib_t.Prtcl,sta_bind_info_t.BaudRate,ComuVerify.State);
        }
        else if(TRUE == change_address_flag)
        {
            sta_bind_check_meter(DevicePib_t.ltu_addr,DevicePib_t.ltu_protocol,sta_bind_info_t.BaudRate,ComuVerify.State);
        }
        else
        {
            app_printf("parameter error!!  change_address_flag = %d\n", change_address_flag);
        }
    }
    else
    {
        Modify_MeterVerifytimer(10);
    }
}
void Modify_MeterVerifytimer(U32 Sec)
{
	app_printf("Modify_MeterVerifytimer %d S\n",Sec);
    if(ComuVerify.timer == NULL)
    {
        ComuVerify.timer = timer_create(Sec*1000,
	                             0,
	                             TMR_TRIGGER ,
	                             MeterVerifyCB,
	                             NULL,
	                             "MeterVerifyCB"
	                            );
    }
    else
    {
        timer_modify(ComuVerify.timer,
               Sec*1000,
               0,
               TMR_TRIGGER ,
               MeterVerifyCB,
               NULL,
               "MeterVerifyCB",
               TRUE
               );
    }
    //针对需要重新15分钟验表的情况，清空计次
    if(METER_COMU_JUDGE_PERIC == Sec)
    {
        ResetRecordComuCheck();
    }
    timer_start(ComuVerify.timer);
}
U8  MeterCheck(void)
{
    if(sta_bind_info_t.PrtclOop20 == 1 || high_baud == TRUE)
    {
        return TRUE;
    }

    return FALSE;
}
void MC_FirstSucc(void)
{
    if(MeterCheck() == TRUE)
    {
        Modify_MeterVerifytimer(METER_COMU_JUDGE_PERIC);
    }
}
void MC_FirstFail(void)
{
    sta_bind_init();
}
void MC_CycleSucc(void)
{
    if(MeterCheck() == TRUE)
    {
        Modify_MeterVerifytimer(METER_COMU_JUDGE_PERIC);
    }
}
void MC_CycleFail(void)
{
    sta_bind_init();
}
void MC_PlugSucc(void)
{
    if(MeterCheck() == TRUE)
    {
        Modify_MeterVerifytimer(METER_COMU_JUDGE_PERIC);
    }
}
void MC_PlugFail(void)
{
    app_reboot(500);
}
void MC_ZeroSucc(void)
{
    //
    if(MeterCheck() == TRUE)
    {
        //如果是20版本电表，并且波特率是9600，准备协商//如果是高波特率绑表，无需协商，因为上电环节有验证20版本电表
        if(sta_bind_info_t.PrtclOop20 == 1 && sta_bind_info_t.BaudRate == BAUDRATE_9600)
        {
			if(1 == app_ext_info.func_switch.oop20BaudSWC)
            	sta_bind_consult20_start(); 
			else
				app_printf("oop20BaudSWC=0\n");
        }
        else
        {
            Modify_MeterVerifytimer(METER_COMU_JUDGE_PERIC);
        }
    }
}

void MC_ZeroFail(void)
{
    sta_bind_init();
}

void MC_CoErrSucc(void)
{
    if(MeterCheck() == TRUE)
    {
        Modify_MeterVerifytimer(METER_COMU_JUDGE_PERIC);
    }
}

void MC_CoErrFail(void)
{
    sta_bind_init();
}



METER_VERIFY_PROC MeterVerifyProc[] = 
{
    { MC_FirstSucc  , MC_FirstFail  ,   2 , VerifyFirst_e , },
    { MC_CycleSucc  , MC_CycleFail  ,   2 , VerifyCycle_e , },
    { MC_PlugSucc   , MC_PlugFail   ,   5 , VerifyPlug_e  , },
    { MC_ZeroSucc   , MC_ZeroFail   ,   5 , VerifyZero_e  , },
    { MC_CoErrSucc  , MC_CoErrFail  ,   10, VerifyCoErr_e , },
};
void MeterCheckSucc(U8 laststate)
{
    if(laststate != ComuVerify.State)
    {
        app_printf("ComuVerify.State %d err != %d\n", ComuVerify.State,laststate);
        return;
    }
    if(ComuVerify.State < VerifyFirst_e || ComuVerify.State > VerifyCoErr_e)
    {
        app_printf("MeterCheck err %d\n", ComuVerify.State);
        return;
    }
    MeterVerifyProc[ComuVerify.State].SuccProc();
    ComuVerify.State = VerifyCycle_e;
}

void MeterCheckFail(U8 laststate)
{
    if(laststate != ComuVerify.State)
    {
        app_printf("ComuVerify.State %d err != %d\n", ComuVerify.State,laststate);
        return;
    }
    RecordComuCheck();
}

void PlcResetReadMeterCnt(void)
{
    ComuVerify.PlcVerifyCnt = 0;
}

void PlcReadMeterSucc(void)
{
    if(MeterCheck() == TRUE)
    {
        if(ComuVerify.State == VerifyCycle_e)
        {
            MeterCheckSucc(VerifyCycle_e);
            PlcResetReadMeterCnt();
        }
    }
}


void PlcReadMeterFail(void)
{
    if(MeterCheck() == TRUE)
    {
        ComuVerify.PlcVerifyCnt ++;
        if(ComuVerify.PlcVerifyCnt > PLCRTTIMEOUTMAXCNT)
        {
            if(ComuVerify.State == VerifyCycle_e)
            {
                StartMeterCheckManage(VerifyCycle_e);
            }
            PlcResetReadMeterCnt();
        }
    }
}


void StartMeterCheckManage(VERIFY_STATE_e state)
{
	if(FactoryTestFlag == 1)
    {
        return;
    }

    if(state < VerifyFirst_e || state > VerifyCoErr_e)
    {
        app_printf("MeterCheck err %d\n", state);
        return;
    }
	
    if(zc_timer_query(sta_bind_timer) == TMR_RUNNING)
    {
    	app_printf("first power,not check meter\n");
		return;
    }
	
	if(VerifyPlug_e == ComuVerify.State)
	{
		app_printf("ComuVerify.State VerifyPlug_e!\n");
		return;
	}
	
    ComuVerify.NextBaudRate = 0;
	
    if(MeterCheck() == TRUE)
    {
        if(sta_bind_info_t.BaudRate == BAUDRATE_19200)
        {
            ComuVerify.NextBaudRate = BAUDRATE_9600;
        }
        if(sta_bind_info_t.BaudRate == BAUDRATE_9600)
        {
            ComuVerify.NextBaudRate = BAUDRATE_19200;
        }
        //MeterUartInit(g_meter, sta_bind_info_t.BaudRate);
        app_printf("BaudRate = %d\n" , sta_bind_info_t.BaudRate);
    }
        
    
    ComuVerify.State = state;
    ResetRecordComuCheck();//清空计次
	Modify_MeterVerifytimer(MeterVerifyProc[ComuVerify.State].Time);
    app_printf("StartMeterCheckManage %d ,after %d s\n", state, MeterVerifyProc[ComuVerify.State].Time);

}

U8 JudgeMeterCheckStateNotPlug()
{
	return (ComuVerify.State==VerifyPlug_e)?FALSE:TRUE;
}


#endif

