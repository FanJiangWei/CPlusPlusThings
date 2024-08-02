/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     Apstask.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */

#include <stdlib.h>
//#include "trios.h"
//#include "sys.h"
//#include "vsh.h"
//#include "ZCsystem.h"
//#include "ZCsystem.h"
////#include "meter.h"
////#include "ProtocolProc.h"
//#include "aps_interface.h"
//#include "app.h"
#include "phy_sal.h"
#include "app_commu_test.h"
#include "test.h"
#include "SyncEntrynet.h"
#include "Scanbandmange.h"
#include "printf_zc.h"
#include "Beacon.h"
#include "plc_io.h"
#include "meter.h"

ostimer_t *testtimer = NULL;


extern 	void uart1testmode(U32 Baud);

static phy_tgain_t get_cb_mode_tgain_by_idx(U8 idx)
{
	phy_tgain_t tgain;

	if(idx == 0)
	{
		tgain.dig = 0;
		tgain.ana = -6;
	}
	else if(idx == 1)
	{
		tgain.dig = 0;
		tgain.ana = -8;
	}
	else if(idx == 2)
	{
		tgain.dig = 0;
		tgain.ana = -8;
	}
	else if(idx == 3)
	{	
		tgain.dig = 0;
		tgain.ana = -12;
	}
	else
	{
		tgain.dig = 0;
		tgain.ana = 0;
	}


	return tgain;
}
static phy_tgain_t get_tgain_by_idx(U8 idx)
{
	phy_tgain_t tgain;

	if(idx == 0)
	{
		tgain.dig = 0;
		tgain.ana = 0;
	}
	else if(idx == 1)
	{
		tgain.dig = 0;
		tgain.ana = -2;
	}
	else if(idx == 2)
	{
		tgain.dig = 0;
		tgain.ana = -4;
	}
	else if(idx == 3)
	{	
		tgain.dig = 0;
		tgain.ana = -8;
	}
	else
	{
		tgain.dig = 0;
		tgain.ana = 0;
	}


	return tgain;
}

__SLOWTEXT void phy_tpt(U16 time)
{
    if(getHplcTestMode()!=NORM)
	{
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
		return;
	}
	app_printf("e_PHY_TRANSPARENT_MODE\n");
    uart1testmode(BAUDRATE_115200);
    setHplcTestMode(PHYTPT);
	phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
	HPLC.sfo_is_done=0;
	HPLC.snid = 0;
    //pCommuTestInd->TimeOrCfgValue       // 持续时间
    Modifytesttimer(time);
}
__SLOWTEXT void phy_cb(U8 testmode, U16 time)
{
	phy_tgain_t tgain;
	
    if(getHplcTestMode() != NORM)
	{
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
		return;
	}

    if(testmode != PHYCB && testmode != RF_PLCPHYCB)
    {
        app_printf("test cb Mode<%d> error!", testmode);
    }
    app_printf("%s\n", testmode == PHYCB       ? "e_PHY_RETURN_MODE" :
                       testmode == RF_PLCPHYCB ? "e_RF_PLC_RETURN_MODE" : "UKW");
    uart1testmode(BAUDRATE_115200);
    setHplcTestMode(testmode);
	phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
	HPLC.sfo_is_done=0;
    HPLC.snid = 0;

    Modifytesttimer(time);
    ld_set_3p_meter_phase_zc(PHY_PHASE_A);

	tgain = get_cb_mode_tgain_by_idx(CURR_B->idx);
    SetTgain(tgain.dig, tgain.ana);
}

__SLOWTEXT void wphy_tpt(U16 time)
{
    if(getHplcTestMode()!=NORM)
    {
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
        return;
    }
    app_printf("e_WPHY_TRANSPARENT_MODE\n");
    uart1testmode(BAUDRATE_115200);
    setHplcTestMode(WPHYTPT);
    HPLC.sfo_is_done=0;
    HPLC.snid = 0;
    //pCommuTestInd->TimeOrCfgValue       // 持续时间
    Modifytesttimer(time);
}
__SLOWTEXT void wphy_cb(U16 time)
{

    if(getHplcTestMode() != NORM)
    {
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
        return;
    }
    app_printf("e_WPHY_RETURN_MODE\n");
    uart1testmode(BAUDRATE_115200);
    setHplcTestMode(WPHYCB);
    HPLC.sfo_is_done=0;
    HPLC.snid = 0;

    Modifytesttimer(time);

}
__SLOWTEXT void mac_tpt(U16 time)
{
	
    if(getHplcTestMode() != NORM)
	{
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
		return;
	}
	app_printf("e_MAC_TRANSPARENT_MODE\n");
    uart1testmode(BAUDRATE_115200);
    setHplcTestMode(MACTPT);
	
	phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);

	ld_set_3p_meter_phase_zc(PHY_PHASE_A);
    //pCommuTestInd->TimeOrCfgValue       // 持续时间
    Modifytesttimer(time);
}
__SLOWTEXT void phy_change_band(U8 idx)
{
    phy_tgain_t tgain;
	
	tgain=get_tgain_by_idx(idx);
    HPLC.band_idx = idx;

    app_printf("e_CARRIER_BAND_CHANGE: change to %d \n", HPLC.band_idx);
	
    SetTgain(tgain.dig, tgain.ana);
	
	//scanFlag = FALSE;
    #if defined(STATIC_NODE)
	StopThisEntryNet();
	ClearNNIB();
    ScanBandManage(e_TEST, 0);
    nnib.AODVRouteUse  = TRUE;
    #else
    HPLC.scanFlag = FALSE;
    nnib.RoutePeriodTime = RoutePeriodTime_FormationNet;//RoutePeriodTime_Test;
	#endif
	
	phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);

}

__SLOWTEXT void tonemask_cfg(U8 idx)
{
	phy_scmask_config(idx); 
}

/**
 * @brief wphy_channel_set  设置无线信道参数
 * @param option            option值
 * @param channel           信道号
 */
__SLOWTEXT void wphy_channel_set(U8 option, U8 channel)
{

    HPLC.option = option;
    HPLC.rfchannel = channel;
    HPLC.hdr_mcs = 2;
    HPLC.pld_mcs = 2;

    app_printf("option:%d, channel:%d, scanflag:%d, mcs:%d-%d\n",option, channel
                , HPLC.scanFlag
                , HPLC.hdr_mcs
                , HPLC.pld_mcs );


    #if defined(STATIC_NODE)
    StopThisEntryNet();
    ClearNNIB();        //清除邻居节点信息。
    ScanBandManage(e_TEST, 0);
    #endif

    wphy_set_option(option);
    wphy_set_channel(channel);
}

uint8_t g_phr_msc  = 0;
uint8_t g_psdu_mcs = 0;
uint8_t g_pbsize   = 0;
__SLOWTEXT void phytowphy_cb(COMMU_TEST_IND_t *pCommuTestInd)
{

    if(getHplcTestMode() != NORM)
    {
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
        return;
    }
    app_printf("e_PHY_TO_WPHY_RETURN_MODE\n");
    setHplcTestMode(PLC_TO_RF_CB);
    uart1testmode(BAUDRATE_115200);
    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
    HPLC.sfo_is_done=0;
    HPLC.snid = 0;

    #if defined(STATIC_NODE)
    StopThisEntryNet(); // 停止入网流程。
    #endif
   
//    set_pa_ldovs(200);//最新的SDK mode1比原来SDK增大了功率，主要给送检的时候提高发送频谱模板用的，杂散差一点的模块可能过不了，向来
//    app_printf("Set wphy mode 1\n");

    Modifytesttimer(pCommuTestInd->TimeOrCfgValue);

    //更新无线回传参数。
    g_phr_msc  = pCommuTestInd->phr_mcs ;
    g_psdu_mcs = pCommuTestInd->psdu_mcs;
    g_pbsize   = pCommuTestInd->pbsize  ;
    
    if(g_phr_msc == 4 || g_phr_msc == 5 || g_phr_msc == 6 || 
        g_psdu_mcs == 4 || g_psdu_mcs == 5 || g_psdu_mcs == 6)
    {
        wphy_set_mode(0);
        //wphy_set_pa(0);
        
    }
    else
    {
        wphy_set_mode(0);
        #if  defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
        if(wphy_get_rf_version() == RF_V2)
        {
            wphy_set_pa(10);
        }
        else
        #endif
        {
            wphy_set_pa(6);
        }
    }
    app_printf("param:%d, %d, %d \n", g_phr_msc, g_psdu_mcs, g_pbsize);

}


U8 g_SafeTestMode  = 0;
/**
 * @brief safeTest_cfg      安全测试模式
 * @param TestMode          测试对应的安全测试模式
 */
__SLOWTEXT void safeTest_cfg(U8 TestMode, U16 time)
{
    if(getHplcTestMode() != NORM)
    {
//        app_printf("testmode is not idle ,testmode = %d\n",getHplcTestMode());
        return;
    }
    g_SafeTestMode = TestMode;
    app_printf("e_SAFETEST_MODE %d\n", g_SafeTestMode);
    uart1testmode(BAUDRATE_115200);
    setHplcTestMode(SAFETEST);

    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);

    ld_set_3p_meter_phase_zc(PHY_PHASE_A);
    //pCommuTestInd->TimeOrCfgValue       // 持续时间
    Modifytesttimer(time);
}


__SLOWTEXT void CommuTestIndication(COMMU_TEST_IND_t *pCommuTestInd)
{


    if(pCommuTestInd->TestModeCfg == e_APP_PACKET_FORWARD_UART)  // 应用层转发到串口
    {
    	app_printf("e_APP_PACKET_FORWARD_UART\n");
		extern void uart_send(uint8_t *buffer, uint16_t len);
        uart_send(pCommuTestInd->Asdu,pCommuTestInd->AsduLength);
    }
    else if(pCommuTestInd->TestModeCfg == e_APP_PACKET_FORWARD_CARRIER)  // 应用层转发到载波
    {
    	app_printf("e_APP_PACKET_FORWARD_CARRIER\n");
    }
    else if(pCommuTestInd->TestModeCfg == e_PHY_TRANSPARENT_MODE) // 物理层透传模式
    {
    	phy_tpt(pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_PHY_RETURN_MODE)  // 物理层回传模式
    {
        phy_cb(PHYCB, pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_MAC_TRANSPARENT_MODE)  // MAC层透传模式
    {
    	mac_tpt(pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_CARRIER_BAND_CHANGE)  // 载波频段切换
    {
		phy_change_band(pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_TONEMASK_CFG)  // TONEMASK  配置
    {
		tonemask_cfg(pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_RFCHANNEL_CFG)  // 无线信道  配置
    {
        wphy_channel_set(pCommuTestInd->TimeOrCfgValue & 0x0f, (pCommuTestInd->TimeOrCfgValue>>4)&0xff);
    }
    else if(pCommuTestInd->TestModeCfg == e_WPHY_RETURN_MODE)  // RF物理层回传
    {
        wphy_cb(pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_WPHY_TRANSPARENT_MODE)  // RF物理层透传
    {
        wphy_tpt(pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_RF_HPLC_RETURN_MODE)  // RF/HPLC物理层回传
    {
        phy_cb(RF_PLCPHYCB,pCommuTestInd->TimeOrCfgValue);
    }
    else if(pCommuTestInd->TestModeCfg == e_HPLC_TO_RF_RETURN_MODE)  // PLC TO RF 回传模式
    {
        phytowphy_cb(pCommuTestInd);
    }
    else if(pCommuTestInd->TestModeCfg == e_SAFETEST_MODE)  // 安全测试模式
    {
        safeTest_cfg(pCommuTestInd->SafeTestMode, pCommuTestInd->TimeOrCfgValue);
    }
    else
    {
        ;
    }

    return;
}


