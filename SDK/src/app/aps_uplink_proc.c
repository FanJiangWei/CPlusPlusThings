/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */
#include <stdlib.h>
#include "ZCsystem.h"
#include "ZCsystem.h"
#include "phy_config.h"
#include "phy_debug.h"
#include "que.h"
#include "list.h"
#include "cpu.h"
#include "aps_layer.h"
#include "aps_interface.h"
#include "app_phase_position_cco.h"  
#include "app_area_indentification_cco.h"
#include "app_area_indentification_sta.h"
#include "app_time_calibrate.h"
#include "app_rdctrl.h"
#include "datalinkdebug.h"
#include "app_commu_test.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "app_crypt_test.h"
#include "app_upgrade_sta.h"
#include "app_upgrade_cco.h"
#include "DatalinkTask.h"
#include "dev_manager.h"
#include "meter.h"
#include "app_clock_sync_sta.h"
#include "app_clock_sta_exact_time_cali.h"
#include "app_data_freeze_cco.h"


unsigned char    BroadcastExternAddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF};



static void ReadMeterIndicationProc(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;
	
	
#if defined(STATIC_MASTER)
    READMETER_UPLINK_HEADER_t  *pReadMeterUpHeader = (READMETER_UPLINK_HEADER_t *)pApsHeader->Apdu;
    //MULTI_TASK_UP_t           *pMultiTaskUp;
    READ_METER_IND_t          *pReadMeterInd;
	
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(READMETER_UPLINK_HEADER_t);

	//work_t *postWork = zc_malloc_mem(sizeof(work_t)+sizeof(MULTI_TASK_UP_t)+sizeof(READ_METER_IND_t)+PayloadLen,"F1F1ReadMeter",MEM_TIME_OUT);
    pReadMeterInd = (READ_METER_IND_t*)zc_malloc_mem(sizeof(READ_METER_IND_t)+PayloadLen,"F1F1ReadMeter", MEM_TIME_OUT);

	 //app_printf("%d\n",PayloadLen);
	//app_printf("pReadMeterUpHeader:\n", pReadMeterUpHeader->DataLength);	
	//dump_buf(pMsduDataInd->Msdu, pMsduDataInd->MsduLength);
    
	pReadMeterInd->stei   = pMsduDataInd->Stei;
    pReadMeterInd->ApsSeq = pReadMeterUpHeader->PacketSeq;
    pReadMeterInd->ReadMode = pApsHeader->PacketID;
    pReadMeterInd->OptionWord = pReadMeterUpHeader->OptionWord;
    //pReadMeterInd->ResponseState = pReadMeterUpHeader->ResponseState;
    __memcpy(pReadMeterInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pReadMeterInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
	
    pReadMeterInd->ProtocolType = pReadMeterUpHeader->DataProType;
	pReadMeterInd->AsduLength   = PayloadLen;
    __memcpy(pReadMeterInd->Asdu, pReadMeterUpHeader->Payload, pReadMeterInd->AsduLength);


    /*
    pMultiTaskUp->pListHeader =   pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_RM ? &HIGHEST_PRIORITY_LIST:
            					  pReadMeterInd->ReadMode == e_ROUTER_ACTIVE_RM ? &F14F1_TRANS_LIST:
            					  pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_RM ? &F1F1_TRANS_LIST:
            					  NULL;
    pMultiTaskUp->Downoffset = (U8)offset_of(multi_trans_t , DatagramSeq);
    pMultiTaskUp->Upoffset = (U8)offset_of(READ_METER_IND_t , ApsSeq);
    pMultiTaskUp->Cmplen = sizeof(pReadMeterInd->ApsSeq);
    
    pMultiTaskUp->AsduLength   = sizeof(READ_METER_IND_t)+PayloadLen;
    __memcpy(pMultiTaskUp->Asdu, (U8 *)pReadMeterInd, pMultiTaskUp->AsduLength);
    
	multi_trans_find_plc_task_up_info(pMultiTaskUp);
	*/
    app_printf("read-up, stei=%d, seq=%04x, r_mode=%04x, mac=", pMsduDataInd->Stei, pReadMeterUpHeader->PacketSeq, pReadMeterInd->ReadMode);
    dump_buf(pMsduDataInd->SrcAddress, 6);
	if(cco_read_meter_ind_hook != NULL)
    {   
        cco_read_meter_ind_hook(pReadMeterInd);
    }

    zc_free_mem(pReadMeterInd);


#elif defined(STATIC_NODE)

    if(GetNodeState() != e_NODE_ON_LINE && !is_rd_ctrl_tei(pMsduDataInd->Stei))
    {   
        app_printf("out line\n");
        return;
    }

    READMETER_DOWNLINK_HEADER_t  *pReadMeterDwnHeader = (READMETER_DOWNLINK_HEADER_t *)pApsHeader->Apdu;
    READ_METER_IND_t           *pReadMeterInd = NULL;
    
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(READMETER_DOWNLINK_HEADER_t);

	pReadMeterInd = (READ_METER_IND_t*)zc_malloc_mem(sizeof(READ_METER_IND_t)+PayloadLen,"ReadMeterInd",MEM_TIME_OUT);

	app_printf("%d",pReadMeterDwnHeader->HeaderLen);
    
    pReadMeterInd->ProtocolType = pReadMeterDwnHeader->DataProType;
    pReadMeterInd->Timeout        = pReadMeterDwnHeader->DeviceTimeout;
    pReadMeterInd->CfgWord       = pReadMeterDwnHeader->CfgWord;
    pReadMeterInd->OptionWord   = pReadMeterDwnHeader->OptionWord;
        
    pReadMeterInd->stei   = pMsduDataInd->Stei;
    pReadMeterInd->ApsSeq = pReadMeterDwnHeader->PacketSeq;
    pReadMeterInd->ReadMode = pApsHeader->PacketID;
    pReadMeterInd->OptionWord = pReadMeterDwnHeader->OptionWord;
    
	__memcpy(pReadMeterInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pReadMeterInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

    pReadMeterInd->AsduLength   = PayloadLen;
    __memcpy(pReadMeterInd->Asdu, pReadMeterDwnHeader->Payload, pReadMeterInd->AsduLength);

    
    //AppReadMeterIndProc(pReadMeterInd);
    if(sta_read_meter_ind_hook != NULL)
    {
        sta_read_meter_ind_hook(pReadMeterInd);
    }

    zc_free_mem(pReadMeterInd);

#endif

    if(TMR_RUNNING == zc_timer_query(g_SendsyhtimeoutTimer) && is_rd_ctrl_tei(pMsduDataInd->Stei))
    {
        timer_start(g_SendsyhtimeoutTimer);
    }

}





static void BroadcastTimeCalibrateInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;

    TIME_CALIBRATE_HEADER_t  *pTimeCalibrateHeader = (TIME_CALIBRATE_HEADER_t *)pApsHeader->Apdu;
    TIME_CALIBRATE_IND_t     *pTimeCalibrateInd = NULL;

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(TIME_CALIBRATE_HEADER_t);
    pTimeCalibrateInd = (TIME_CALIBRATE_IND_t*)zc_malloc_mem(sizeof(TIME_CALIBRATE_IND_t)+PayloadLen,"TimeCalibrate",MEM_TIME_OUT);
    
    __memcpy(pTimeCalibrateInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pTimeCalibrateInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    
    pTimeCalibrateInd->DataLength = PayloadLen;
    __memcpy(&pTimeCalibrateInd->TimeData, pTimeCalibrateHeader->TimeData, PayloadLen);
	app_printf("BroadcastTimeCalibrateInd:");
	dump_buf(pTimeCalibrateHeader->TimeData, PayloadLen);
    //TimeCalibrateIndication(pTimeCalibrateInd);
    if(time_calibrate_hook != NULL)
    {
        time_calibrate_hook(pTimeCalibrateInd);
    }

    zc_free_mem(pTimeCalibrateInd);

}



/**
 * @brief SafeComTestInd 平台通过通信测试报文格式，在RF信道，将需要加密的随机报文，发送给DUT，
 * DUT解析报文中的数据域，并进行加密处理，处理后采用MAC层透传方式，将加密后的数据同应用层报文头一同经过串口发送到测试平台
 * @param work
 * 测试报文中的数据域格式，定义如下：
 * 域                                            字节数
 * 密钥长度/公钥长度/哈希结果长度/验签结果长度/         1
 * 密钥/公钥/哈希结果/验签结果/                       N1
 * IV/签名长度                                      1
 * IV/签名                                         N2
 * 明文/密文长度                                    2
 * 明文/密文                                       N3
 * 说明：SHA256/SM3算法和ECC/SM2签名测试，平台只下发随机数，数据域只填充随机数长度和随机数。
 * 验签结果定义：0：失败；1：成功。
 */
__SLOWTEXT static void SafeComTestInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    SAFE_TEST_HEADER_t  *pSafeTestHeader = (SAFE_TEST_HEADER_t *)pApsHeader->Apdu;

    U8 *Sendbuf = zc_malloc_mem(520, "safetest", MEM_TIME_OUT);
#if 1

    APDU_HEADER_t     *pApsResp = (APDU_HEADER_t *)Sendbuf;
    SAFE_TEST_HEADER_t  *pSafeTestResp = (SAFE_TEST_HEADER_t *)pApsResp->Apdu;
    __memcpy(Sendbuf, (U8 *)pApsHeader, sizeof(APDU_HEADER_t) + sizeof(SAFE_TEST_HEADER_t));
    U16 ApduLen = sizeof(APDU_HEADER_t) + sizeof(SAFE_TEST_HEADER_t);
#else
    SAFE_TEST_HEADER_t  *pSafeTestResp = (SAFE_TEST_HEADER_t *)Sendbuf;
    __memcpy(pSafeTestResp, pSafeTestHeader, sizeof(SAFE_TEST_HEADER_t));
    U16 ApduLen = sizeof(SAFE_TEST_HEADER_t);
#endif


    if(do_safeTest(pSafeTestHeader, pSafeTestResp))
    {
        ApduLen += pSafeTestResp->DataLength;
        app_printf("ApduLen:%d\n", ApduLen);
        uart1_send(Sendbuf, ApduLen);
    }
    zc_free_mem(Sendbuf);

}


//extern void CommuTestIndication(COMMU_TEST_IND_t *pCommuTestInd);
static  __SLOWTEXT void CommunicationTestInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;
    U8*                pPayload = NULL;

    COMMU_TEST_HEADER_t  *pCommTestHeader = (COMMU_TEST_HEADER_t *)pApsHeader->Apdu;
    COMMU_TEST_IND_t  *pCommuTestInd = NULL;

    if(getHplcTestMode() == SAFETEST)
    {
        SafeComTestInd(work);
        return;
    }

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t) - pCommTestHeader->HeaderLen;  //按照最大payload长度申请内存，误差两个字节
    app_printf("PayloadLen %d \n",PayloadLen);
    if(PayloadLen >500)
    {
        PayloadLen = 500;
    }
    pCommuTestInd = (COMMU_TEST_IND_t*)zc_malloc_mem(sizeof(COMMU_TEST_IND_t)+PayloadLen,"CommuTestInd",MEM_TIME_OUT);

    pCommuTestInd->TestModeCfg = pCommTestHeader->TestModeCfg;
    pCommuTestInd->ProtocolType = pCommTestHeader->ProtocolType;
    if(pCommuTestInd->TestModeCfg != e_APP_PACKET_FORWARD_UART && 
                                pCommuTestInd->TestModeCfg != e_APP_PACKET_FORWARD_CARRIER)
    {
       pCommuTestInd->TimeOrCfgValue = pCommTestHeader->DataLength;
       pCommuTestInd->SafeTestMode = pCommTestHeader->SafeTestMode;
       //plc到rf回传模式参数，只有测试模式为12时生效其他测试模式为保留位。
       pCommuTestInd->phr_mcs  = pCommTestHeader->phr_mcs ;
       pCommuTestInd->psdu_mcs = pCommTestHeader->psdu_mcs;
       pCommuTestInd->pbsize   = pCommTestHeader->pbsize  ;

       PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t) - pCommTestHeader->HeaderLen;  //计算实际的payload长度
       pPayload   = (U8*)pApsHeader + pCommTestHeader->HeaderLen;
    }
    else    //应用层测试帧处理
    {
        APS_TESET_HEADER_t *pApsTestHeader = (APS_TESET_HEADER_t *)pApsHeader->Apdu;

        pPayload   = pApsTestHeader->Asdu;
        PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t) - sizeof(APS_TESET_HEADER_t); //计算实际的payload长度
    }

    __memcpy(pCommuTestInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pCommuTestInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

    app_printf("PayloadLen %d \n",PayloadLen);
    if(PayloadLen >500)
    {
        PayloadLen = 500;
    }

    //应用层测试帧处理
    if(PayloadLen)
    {
        pCommuTestInd->AsduLength = PayloadLen;
        __memcpy(&pCommuTestInd->Asdu, pPayload, PayloadLen);
    }

    //CommuTestIndication(pCommuTestInd);
    if(commu_test_ind_hook != NULL)
    {
        commu_test_ind_hook(pCommuTestInd);
    }

    zc_free_mem(pCommuTestInd);
}



static void StaEventReportInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;
    
    if(CheckSTAOnlineSate() == FALSE)
    {
        return;
    }

    EVENT_REPORT_HEADER_t  *pEventReportHeader = (EVENT_REPORT_HEADER_t *)pApsHeader->Apdu;
    EVENT_REPORT_IND_t     *pEventReportInd = NULL;

    
    
#if defined(STATIC_MASTER)
    if(pEventReportHeader->Dir == e_DOWNLINK)
    {
        app_printf("event downlink----\n");
        return;
    }
    
    //app_printf("event PacketSeq=%d,pMsduDataInd->stei=%d\n",pEventReportHeader->PacketSeq,pMsduDataInd->stei);
   /* extern U8 CheckApsSeq(U16 AsduSeq , U16 SrcTei);
    if(FALSE == CheckApsSeq(pEventReportHeader->PacketSeq,pMsduDataInd->Stei))
    {
        return;
    }

    extern void AddAsduSeq(U16 MsduSeq , U16 SrcTei);
    AddAsduSeq(pEventReportHeader->PacketSeq,pMsduDataInd->Stei);*/
#endif
    
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(EVENT_REPORT_HEADER_t);
	pEventReportInd = (EVENT_REPORT_IND_t*)zc_malloc_mem(sizeof(EVENT_REPORT_IND_t)+PayloadLen,"EventReportInd",MEM_TIME_OUT);

    pEventReportInd->SendType = pMsduDataInd->SendType;
    
    pEventReportInd->EventDataLen = PayloadLen;
    __memcpy(pEventReportInd->EventData, pEventReportHeader->EventData, PayloadLen);
    
    pEventReportInd->PacketSeq = pEventReportHeader->PacketSeq;
    
    pEventReportInd->FunCode = pEventReportHeader->FunCode;
    pEventReportInd->Direct     = pEventReportHeader->Dir;
    __memcpy(pEventReportInd->MeterAddr, pEventReportHeader->MeterAddr, 6);
    __memcpy(pEventReportInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pEventReportInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    
    ApsEventRecvPacketSeq = pEventReportHeader->PacketSeq;

#if defined(STATIC_MASTER)    
    //CcoProcEventReportInd(pEventReportInd);
    if(cco_event_report_hook != NULL && StartSendCenterBeaconFlag == TRUE)
    {
        cco_event_report_hook(pEventReportInd);
    }
#else
    //StaProcEventReportInd(pEventReportInd);
    if(sta_event_report_hook != NULL)
    {
        sta_event_report_hook(pEventReportInd);
    }
#endif
    zc_free_mem(pEventReportInd);
}


static void ZoneAreaInfoGatherInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    U16                PayloadLen = 0;

    app_printf("---ZONEAREA_INFO_INDICATION_t----\n");

    ZONEAREA_INFO_HEADER_t  *pZoneAreaInfoHeader = (ZONEAREA_INFO_HEADER_t *)pApsHeader->Apdu;
    ZONEAREA_INFO_IND_t  *pZoneAreaInfoInd = NULL;

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(ZONEAREA_INFO_HEADER_t);
    work_t *postWork = zc_malloc_mem(sizeof(work_t)+sizeof(ZONEAREA_INFO_IND_t)+PayloadLen,"EventReportInd",MEM_TIME_OUT);
    
    pZoneAreaInfoInd->AsduLength = PayloadLen;
    __memcpy(pZoneAreaInfoInd->Asdu, pZoneAreaInfoHeader->Asdu, PayloadLen);
    
    
    pZoneAreaInfoInd->Direct         = pZoneAreaInfoHeader->Dir;
    pZoneAreaInfoInd->GatherType = pZoneAreaInfoHeader->GaterhType;
    __memcpy(pZoneAreaInfoInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pZoneAreaInfoInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    
    // post
    postWork->work = NULL;
    postWork->msgtype = ZAGATH;
    post_app_work(postWork);
}

static void SlaveRegisterQueryProc(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    U16                PayloadLen = 0;
    
#if defined(STATIC_MASTER)    
    
    REGISTER_QUERY_UP_HEADER_t  *pRegisterQueryUpHeader = (REGISTER_QUERY_UP_HEADER_t *)pApsHeader->Apdu;
    REGISTER_QUERY_CFM_t  *pRegisterQueryCfm = NULL;
    //MULTI_TASK_UP_t           *pMultiTaskUp;

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(REGISTER_QUERY_UP_HEADER_t);
    //app_printf("%d\n",PayloadLen);

    pRegisterQueryCfm = (REGISTER_QUERY_CFM_t*)zc_malloc_mem(sizeof(REGISTER_QUERY_CFM_t)+PayloadLen,"RegQueryCfm",MEM_TIME_OUT);
    //pMultiTaskUp = (MULTI_TASK_UP_t*)pTempUpInfo;
    
    //pRegisterQueryCfm = (REGISTER_QUERY_CFM_t *)pMultiTaskUp->Asdu;

    pRegisterQueryCfm->Status = e_SUCCESS;
    pRegisterQueryCfm->SearchMeterStatus  = pRegisterQueryUpHeader->Status;
    pRegisterQueryCfm->MeterCount = pRegisterQueryUpHeader->MeterCount;
    pRegisterQueryCfm->DeviceType = pRegisterQueryUpHeader->DeviceType;
    pRegisterQueryCfm->RegisterParameter = pRegisterQueryUpHeader->RegisterParameter;

    pRegisterQueryCfm->PacketSeq = pRegisterQueryUpHeader->PacketSeq;
    __memcpy(pRegisterQueryCfm->DeviceAddr, pRegisterQueryUpHeader->DeviceAddr, 6);
    __memcpy(pRegisterQueryCfm->DeviceId, pRegisterQueryUpHeader->DeviceId, 6);

    __memcpy(pRegisterQueryCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    __memcpy(pRegisterQueryCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);

    pRegisterQueryCfm->MeterInfoLen = PayloadLen;
    __memcpy(&pRegisterQueryCfm->MeterInfo, pRegisterQueryUpHeader->MeterInfo, PayloadLen);
/*
    app_printf("pRegisterQueryUpHeader->RegisterParameter : %s\n",
                         pRegisterQueryUpHeader->RegisterParameter  == e_QUERY_REGISTER_RESULT?"e_QUERY_REGISTER_RESULT":
                         pRegisterQueryUpHeader->RegisterParameter  == e_START_SLAVE_REGISTER?"e_START_SLAVE_REGISTER":
                         pRegisterQueryUpHeader->RegisterParameter  == e_LOCK_CMD?"e_LOCK_CMD":
                         pRegisterQueryUpHeader->RegisterParameter  == e_QUERY_WATER_CMD?"e_QUERY_WATER_CMD":"unkown");
*/
    
	if(cco_register_query_cfm_hook != NULL)
    {
        cco_register_query_cfm_hook(pRegisterQueryCfm);
    }

    zc_free_mem(pRegisterQueryCfm);

#elif defined(STATIC_NODE)

    REGISTER_QUERY_DWN_HEADER_t  *pRegisterQueryDwnHeader = (REGISTER_QUERY_DWN_HEADER_t *)pApsHeader->Apdu;
    REGISTER_QUERY_IND_t *pRegisterQueryInd = NULL;

    app_printf("ind daddr : ");
    dump_buf(pMsduDataInd->DstAddress, 6);
    app_printf("frame daddr : ");
    dump_buf(pRegisterQueryDwnHeader->DstMacAddr, 6);
    app_printf("ind saddr : ");
    dump_buf(pMsduDataInd->SrcAddress, 6);
    app_printf("frame saddr : ");
    dump_buf(pRegisterQueryDwnHeader->SrcMacAddr, 6);

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(REGISTER_QUERY_DWN_HEADER_t);
    app_printf("%d %d\n",pMsduDataInd->MsduLength,PayloadLen);
		
    pRegisterQueryInd = zc_malloc_mem(sizeof(REGISTER_QUERY_IND_t)+PayloadLen,"RegQueryInd",MEM_TIME_OUT);

	pRegisterQueryInd->ForcedResFlag = pRegisterQueryDwnHeader->ForcedResFlag;
	__memcpy(pRegisterQueryInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
	__memcpy(pRegisterQueryInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

	//ApsRecvRegMeterSeq = pRegisterQueryDwnHeader->PacketSeq;
	pRegisterQueryInd->PacketSeq = pRegisterQueryDwnHeader->PacketSeq;
		
    pRegisterQueryInd->RegisterParameter = pRegisterQueryDwnHeader->RegisterParameter;

    pRegisterQueryInd->AsduLength = PayloadLen;
    if(PayloadLen > 0)
        __memcpy(pRegisterQueryInd->Asdu, pRegisterQueryDwnHeader->Payload, PayloadLen);

    app_printf("pRegisterQueryDwnHeader->RegisterParameter : %s\n",
			pRegisterQueryDwnHeader->RegisterParameter  == e_QUERY_REGISTER_RESULT?"e_QUERY_REGISTER_RESULT":
			pRegisterQueryDwnHeader->RegisterParameter  == e_START_SLAVE_REGISTER?"e_START_SLAVE_REGISTER":
			pRegisterQueryDwnHeader->RegisterParameter  == e_LOCK_CMD?"e_LOCK_CMD":
			pRegisterQueryDwnHeader->RegisterParameter  == e_QUERY_WATER_CMD?"e_QUERY_WATER_CMD":"unkown");

    
    if(memcmp(pMsduDataInd->DstAddress , pRegisterQueryDwnHeader->DstMacAddr , 6)==0 &&
				 memcmp(pMsduDataInd->SrcAddress, pRegisterQueryDwnHeader->SrcMacAddr, 6) == 0)
    {
        if(sta_register_query_ind_hook != NULL)
        {
            sta_register_query_ind_hook(pRegisterQueryInd);
        }
    }
    else
    {
        app_printf(" addr err\n");
    }
    zc_free_mem(pRegisterQueryInd);
#endif

    return;
}
static void SlaveRegisterStartInd(work_t *work)
{
    /*
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;

    REGISTER_START_HEADER_t  *pRegisterStartHeader = (REGISTER_START_HEADER_t *)pApsHeader->Apdu;

    
    SLAVE_REGISTER_START_IND_t  RegisterStartInd;

    __memcpy(RegisterStartInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(RegisterStartInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    */

    return;
}

static void SlaveRegisterStopInd(work_t *work)
{
    return;
}

static void AckOrNakInd(work_t *work)
{
    return;;
}

static void DataAggregateReadMeterInd(work_t *work)
{
    return;
}

static void UpgradeStartInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	//U16                PayloadLen = 0;
    
    if(CheckSTAOnlineSate() == FALSE)
    {
        app_printf("Free UpgradeStartInd !\n");
        return;
    }
#if defined(STATIC_MASTER)
    UPGRADE_START_UP_HEADER_t  *pUpgradeStartUpHeader = (UPGRADE_START_UP_HEADER_t *)pApsHeader->Apdu;
    //MULTI_TASK_UP_t           *pMultiTaskUp;
    UPGRADE_START_CFM_t       *pUpgradeStartCfm;

    //PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(UPGRADE_START_UP_HEADER_t);

    //work_t *postWork = zc_malloc_mem(sizeof(work_t)+sizeof(MULTI_TASK_UP_t)+sizeof(READ_METER_IND_t)+PayloadLen,"F1F1ReadMeter",MEM_TIME_OUT);
    pUpgradeStartCfm = (UPGRADE_START_CFM_t*)zc_malloc_mem(sizeof(UPGRADE_START_CFM_t), "UpgradeStart", MEM_TIME_OUT);

    //app_printf("%d\n",PayloadLen);
    app_printf("pUpgradeStartCfm:%d\n", pUpgradeStartUpHeader->StartResultCode);    
    dump_buf(pMsduDataInd->Msdu, pMsduDataInd->MsduLength);

    //pMultiTaskUp = (MULTI_TASK_UP_t*)pTempUpInfo;
    //pUpgradeStartCfm = (UPGRADE_START_CFM_t *)pMultiTaskUp->Asdu;

    pUpgradeStartCfm->Status = e_SUCCESS;
    pUpgradeStartCfm->StartResultCode = pUpgradeStartUpHeader->StartResultCode;
    pUpgradeStartCfm->UpgradeID = pUpgradeStartUpHeader->UpgradeID;
    __memcpy(pUpgradeStartCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pUpgradeStartCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

    /*
    pMultiTaskUp->pListHeader = &UPGRADE_START_LIST;
    pMultiTaskUp->Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    pMultiTaskUp->Upoffset = (U8)offset_of(UPGRADE_START_CFM_t , SrcMacAddr);
    pMultiTaskUp->Cmplen = sizeof(pUpgradeStartCfm->SrcMacAddr);
    
    pMultiTaskUp->AsduLength   = sizeof(UPGRADE_START_CFM_t);
    //__memcpy(pMultiTaskUp->Asdu, (U8 *)pUpgradeStartCfm, pMultiTaskUp->AsduLength);
    */
    
    //multi_trans_find_plc_task_up_info(pMultiTaskUp);
    if(upgrade_start_cfm_hook != NULL)
    {
        upgrade_start_cfm_hook(pUpgradeStartCfm);
    }

    zc_free_mem(pUpgradeStartCfm);
    

#elif defined(STATIC_NODE)
    UPGRADE_START_DWN_HEADER_t  *pUpgradeStartDwnHeader = (UPGRADE_START_DWN_HEADER_t *)pApsHeader->Apdu;
    UPGRADE_START_IND_t       UpgradeStartInd;//*pUpgradeStartInd = zc_malloc_mem(sizeof(UPGRADE_START_IND_t),"UpgradeStartInd",MEM_TIME_OUT);;

    
    UpgradeStartInd.UpgradeID         = pUpgradeStartDwnHeader->UpgradeID;
    UpgradeStartInd.UpgradeTimeout = pUpgradeStartDwnHeader->UpgradeTimeout;
    UpgradeStartInd.BlockSize           = pUpgradeStartDwnHeader->BlockSize;
    UpgradeStartInd.FileSize              = pUpgradeStartDwnHeader->FileSize;
    UpgradeStartInd.FileCrc               = pUpgradeStartDwnHeader->FileCrc;
    __memcpy(UpgradeStartInd.DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(UpgradeStartInd.SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    //app_printf("%d",pUpgradeStartDwnHeader->FileSize);
    app_printf("%d",UpgradeStartInd.FileSize);
    
    //UpgradeStartPro(&pUpgradeStartInd);
    if(upgrade_start_ind_hook != NULL)
    {
        upgrade_start_ind_hook(&UpgradeStartInd);
    }
#endif

}
static void UpgradeStopInd(work_t *work)
{

#if defined(STATIC_NODE)

    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    UPGRADE_STOP_HEADER_t  *pUpgradeStopDwnHeader = (UPGRADE_STOP_HEADER_t *)pApsHeader->Apdu;
    
    app_printf("\n UpgradeStopIndication,SlaveUpgradeFileInfo.OnlineStatus = %d\n\n",SlaveUpgradeFileInfo.OnlineStatus);
    //UpgradeStopPro(pUpgradeStopDwnHeader->UpgradeID);
    
    if(upgrade_stop_ind_hook != NULL)
    {
        upgrade_stop_ind_hook(pUpgradeStopDwnHeader->UpgradeID);
    }

#endif
}

static void UpgradeFileTransInd(work_t *work)
{
    
#if defined(STATIC_NODE)
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    
    if(CheckSTAOnlineSate() == FALSE)
	{
		app_printf("Free e_FILE_TRANS mdb!\n");
        return;
	}
    FILE_TRANS_HEADER_t  *pFileTransHeader = (FILE_TRANS_HEADER_t *)pApsHeader->Apdu;
    FILE_TRANS_IND_t     *pFileTransInd = zc_malloc_mem(sizeof(FILE_TRANS_REQ_t)+pFileTransHeader->BlockSize,"UpgradeFiletrans",MEM_TIME_OUT);
    if(NULL == pFileTransInd)
    {
        return;
    }
	pFileTransInd->IsCjq     = pFileTransHeader->Reserve;
	pFileTransInd->UpgradeID = pFileTransHeader->UpgradeID;
	pFileTransInd->BlockSize = pFileTransHeader->BlockSize;
	pFileTransInd->BlockSeq = pFileTransHeader->BlockSeq;
	
	if(pApsHeader->PacketID == e_FILE_TRANS)
	{
		pFileTransInd->TransMode = e_FILE_TRANS_UNICAST;
	}
	else if(pApsHeader->PacketID == e_FILE_TRANS_LOCAL_BROADCAST)
	{
		pFileTransInd->TransMode = e_UNICAST_TO_LOCAL_BROADCAST;
	}
	else
	{;
	}
	
	__memcpy(&pFileTransInd->DataBlock, pFileTransHeader->DataBlock, pFileTransHeader->BlockSize);
    
    //UpgradeFileTransPro(pFileTransInd);
    if(upgrade_filetrans_ind_hook != NULL)
    {
        upgrade_filetrans_ind_hook(pFileTransInd);
    }
    
    zc_free_mem(pFileTransInd);
#endif
}
/*
static void UpgradeFileTransBroadcastInd(work_t *work)
{

}
*/
static void UpgradeStatus_QueryInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	
#if defined(STATIC_MASTER)
    UPGRADE_STATUS_UP_HEADER_t  *pUpgradeSttusUpHeader = (UPGRADE_STATUS_UP_HEADER_t *)pApsHeader->Apdu;
    //MULTI_TASK_UP_t             *pMultiTaskUp;
    UPGRADE_STATUS_QUERY_CFM_t  *pUpgradeQueryCfm;
    U16                         PayloadLen = 0;
    
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(UPGRADE_STATUS_UP_HEADER_t);

    pUpgradeQueryCfm = (UPGRADE_STATUS_QUERY_CFM_t*)zc_malloc_mem(sizeof(UPGRADE_STATUS_QUERY_CFM_t)+PayloadLen,"UpgradeQuery",MEM_TIME_OUT);

    app_printf("pUpgradeQueryRsp %d:\n",PayloadLen);    
    dump_buf(pMsduDataInd->Msdu, pMsduDataInd->MsduLength);

    pUpgradeQueryCfm->UpgradeStatus = pUpgradeSttusUpHeader->UpgradeStatus;
    pUpgradeQueryCfm->ValidBlockCount = pUpgradeSttusUpHeader->ValidBlockCount;
    pUpgradeQueryCfm->StartBlockIndex = pUpgradeSttusUpHeader->StartBlockIndex;
    pUpgradeQueryCfm->UpgradeID = pUpgradeSttusUpHeader->UpgradeID;
    pUpgradeQueryCfm->BitMapLen = PayloadLen;
    __memcpy(pUpgradeQueryCfm->BlockInfoBitMap, pUpgradeSttusUpHeader->BlockInfoBitMap, pUpgradeQueryCfm->BitMapLen);
    
    __memcpy(pUpgradeQueryCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pUpgradeQueryCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

    if(upgrade_status_query_cfm_hook != NULL)
    {
        upgrade_status_query_cfm_hook(pUpgradeQueryCfm);
    }
    zc_free_mem(pUpgradeQueryCfm);
    

#elif defined(STATIC_NODE)
    if(CheckSTAOnlineSate() == FALSE)
    {
        app_printf("Free UpgradeStatus_QueryInd!\n");
        return;
    }
    UPGRADE_STATUS_DWN_HEADER_t  *pUpgradeStatusDwnHeader = (UPGRADE_STATUS_DWN_HEADER_t *)pApsHeader->Apdu;
    UPGRADE_STATUS_QUERY_IND_t    UpggradeStatusQueryInd;// = zc_malloc_mem(sizeof(UPGRADE_START_IND_t),"UpgradeStartInd",MEM_TIME_OUT);;

    
    UpggradeStatusQueryInd.QueryBlockCount = pUpgradeStatusDwnHeader->QueryBlockCount;
    UpggradeStatusQueryInd.StartBockIndex    = pUpgradeStatusDwnHeader->StartBockIndex;
    UpggradeStatusQueryInd.UpgradeID = pUpgradeStatusDwnHeader->UpgradeID;
    __memcpy(UpggradeStatusQueryInd.DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(UpggradeStatusQueryInd.SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    
    //UpgradeStatusQueryPro(&pUpggradeStatusQueryInd);
    if(upgrade_status_query_ind_hook != NULL)
    {
        upgrade_status_query_ind_hook(&UpggradeStatusQueryInd);
    }

#endif
}
static void UpgradeStaInfoQueryInd(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;
    
    
#if defined(STATIC_MASTER)
    STATION_INFO_UP_HEADER_t  *pUpgradeInfoUpHeader = (STATION_INFO_UP_HEADER_t *)pApsHeader->Apdu;
    //MULTI_TASK_UP_t           *pMultiTaskUp;
    STATION_INFO_QUERY_CFM_t       *pStationInfoCfm;

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(STATION_INFO_UP_HEADER_t);

    //work_t *postWork = zc_malloc_mem(sizeof(work_t)+sizeof(MULTI_TASK_UP_t)+sizeof(READ_METER_IND_t)+PayloadLen,"F1F1ReadMeter",MEM_TIME_OUT);
    pStationInfoCfm = (STATION_INFO_QUERY_CFM_t*)zc_malloc_mem(sizeof(STATION_INFO_QUERY_CFM_t)+PayloadLen, "StationInfoCfm", MEM_TIME_OUT);

    //app_printf("%d\n",PayloadLen);  
    dump_buf(pMsduDataInd->Msdu, pMsduDataInd->MsduLength);

    //pMultiTaskUp = (MULTI_TASK_UP_t*)pTempUpInfo;
    //pUpgradeStartCfm = (UPGRADE_START_CFM_t *)pMultiTaskUp->Asdu;


    pStationInfoCfm->InfoListNum = pUpgradeInfoUpHeader->InfoListNum;
    pStationInfoCfm->InfoListLen = PayloadLen;
    __memcpy(pStationInfoCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pStationInfoCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    __memcpy(pStationInfoCfm->InfoList , pUpgradeInfoUpHeader->InfoData,pStationInfoCfm->InfoListLen);

    
    if(upgrade_stainfo_query_cfm_hook != NULL)
    {
        upgrade_stainfo_query_cfm_hook(pStationInfoCfm);
    }

    zc_free_mem(pStationInfoCfm);
    

#elif defined(STATIC_NODE)
    STATION_INFO_DWN_HEADER_t  *pStationInfoDwnHeader = (STATION_INFO_DWN_HEADER_t *)pApsHeader->Apdu;
    STATION_INFO_QUERY_IND_t   *pStationIfoInd;
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(STATION_INFO_DWN_HEADER_t);
    pStationIfoInd = zc_malloc_mem(sizeof(STATION_INFO_QUERY_IND_t)+PayloadLen,"UpgradeStartInd",MEM_TIME_OUT);
    
    
    
    pStationIfoInd->InfoListNum = pStationInfoDwnHeader->InfoListNum;
    pStationIfoInd->InfoListLen = PayloadLen;    
    __memcpy(pStationIfoInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pStationIfoInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    __memcpy(pStationIfoInd->InfoList , pStationInfoDwnHeader->InfoList,pStationIfoInd->InfoListLen);
    app_printf("StationIfoInd->InfoListLen %d\n",pStationIfoInd->InfoListLen);
    dump_buf(pStationIfoInd->InfoList , 6);
    dump_buf(pStationInfoDwnHeader->InfoList,6);
    if(upgrade_stainfo_query_ind_hook != NULL)
    {
        upgrade_stainfo_query_ind_hook(pStationIfoInd);
    }
#endif

}


static void UpgradePerformInd(work_t *work)
{
#if defined(ZC3750STA) || defined(ZC3750CJQ2)
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    
    if(CheckSTAOnlineSate() == FALSE)
    {
        app_printf("Free UpgradeStatus_QueryInd!\n");
        return;
    }
    UPGRADE_PERFORM_HEADER_t *pUpgradePerformHeader = (UPGRADE_PERFORM_HEADER_t *)pApsHeader->Apdu;
    UPGRADE_PERFORM_IND_t     UpgradePerformInd;// = (UPGRADE_PERFORM_INDICATION_t *)pApsHeader->Apdu;

    UpgradePerformInd.WaitResetTime = pUpgradePerformHeader->WaitResetTime;
    UpgradePerformInd.UpgradeID      =  pUpgradePerformHeader->UpgradeID;
    UpgradePerformInd.TestRunTime   = pUpgradePerformHeader->TestRunTime;
    __memcpy(UpgradePerformInd.DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(UpgradePerformInd.SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    
    //UpgradePerformPro(&pUpgradePerformInd);
    if(upgrade_perform_ind_hook != NULL)
    {
        upgrade_perform_ind_hook(&UpgradePerformInd);
    }
  
#endif
}



static void RdCtrlCcoInd(work_t *work)
{
    if(getHplcTestMode() == RD_CTRL)
    {

    }
    else
    {
        MSDU_DATA_IND_t   *pMsduDataInd  = (MSDU_DATA_IND_t *)work->buf;
        APDU_HEADER_t     *pApsHeader    = (APDU_HEADER_t *)pMsduDataInd->Msdu;
        RDCTRL_HEADER_t   *pRdCtrlHeader = (RDCTRL_HEADER_t*)pApsHeader->Apdu;
        U16                PayloadLen = 0;

        RD_CTRL_REQ_t     *pRdCtrlInd = NULL;
        PayloadLen = pMsduDataInd->MsduLength - sizeof(APDU_HEADER_t) - sizeof(RDCTRL_HEADER_t);

        pRdCtrlInd = zc_malloc_mem(sizeof(RD_CTRL_REQ_t) + PayloadLen, "rd", MEM_TIME_OUT);

        pRdCtrlInd->protocol = pRdCtrlHeader->protocol;
        pRdCtrlInd->len = PayloadLen;
        pRdCtrlInd->dtei = pMsduDataInd->Stei;
//        pRdCtrlInd->MacUsed = pMsduDataInd->MacUsed;

        __memcpy(pRdCtrlInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
        __memcpy(pRdCtrlInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
        __memcpy(pRdCtrlInd->Asdu, pRdCtrlHeader->Asdu, pRdCtrlInd->len);

        if(rdctrl_ind_hook)
        {
            rdctrl_ind_hook(pRdCtrlInd);
        }

        zc_free_mem(pRdCtrlInd);
    }
}
static void RdCtrlToUartInd(work_t *work)
{
	MSDU_DATA_IND_t   *pMsduDataInd  = (MSDU_DATA_IND_t *)work->buf;
	APDU_HEADER_t	  *pApsHeader	 = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	RDCTRL_TO_UART_HEADER_t   *pRdCtrlTouartHeader = (RDCTRL_TO_UART_HEADER_t*)pApsHeader->Apdu;
	U16 			   PayloadLen = 0;
	
	RD_CTRLT_TO_UART_REQ_t	   *pRdCtrlToUartInd = NULL;

    PayloadLen = pMsduDataInd->MsduLength - sizeof(APDU_HEADER_t) - sizeof(RDCTRL_TO_UART_HEADER_t);
    pRdCtrlToUartInd = zc_malloc_mem(sizeof(RD_CTRLT_TO_UART_REQ_t) + PayloadLen, "rdtouart", MEM_TIME_OUT);
    pRdCtrlToUartInd->protocol = pRdCtrlTouartHeader->Protocol;
    pRdCtrlToUartInd->dirflag = pRdCtrlTouartHeader->Dir;
    pRdCtrlToUartInd->len = PayloadLen;
    pRdCtrlToUartInd->dtei = pMsduDataInd->Stei;
    pRdCtrlToUartInd->baud = pRdCtrlTouartHeader->Baud;

    __memcpy(pRdCtrlToUartInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pRdCtrlToUartInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    __memcpy(pRdCtrlToUartInd->Asdu, pRdCtrlTouartHeader->Asdu, pRdCtrlToUartInd->len);

    if(rdctrl_to_uart_ind_hook)
    {
        rdctrl_to_uart_ind_hook(pRdCtrlToUartInd);
    }
	zc_free_mem(pRdCtrlToUartInd);
}

static void AuthenticationSecurityInd(work_t *work){}


/*
static void IndentificationAreaInd(work_t *work)
{
    
	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
	APDU_HEADER_t	  *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	
	INDENTIFICATION_NEW_HEADER_t  *pIndentificationNewHeader = (INDENTIFICATION_NEW_HEADER_t *)pApsHeader->Apdu;
	
#if defined(STATIC_MASTER)
	U16 			   PayloadLen = 0;
	INDENTIFICATION_IND_t      *pIndentificationInd=NULL;
	
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(INDENTIFICATION_NEW_HEADER_t);

	work_t *postWork = zc_malloc_mem(sizeof(work_t)+sizeof(INDENTIFICATION_IND_t)+PayloadLen,"F1F1ReadMeter",MEM_TIME_OUT);

	pIndentificationInd = (INDENTIFICATION_IND_t*)postWork->buf;

	pIndentificationInd->stei           = pMsduDataInd->Stei;
    pIndentificationInd->DatagramSeq    = pIndentificationNewHeader->PacketSeq;
	pIndentificationInd->Featuretypes   = pIndentificationNewHeader->Featuretypes;
	pIndentificationInd->Collectiontype = pIndentificationNewHeader->Collectiontype;
    __memcpy(pIndentificationInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pIndentificationInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
	
	pIndentificationInd->payloadlen     = PayloadLen;
    __memcpy(pIndentificationInd->payload, pIndentificationNewHeader->Payload, pIndentificationInd->payloadlen);
	if(pIndentificationNewHeader->Collectiontype == e_FEATURE_INFO_INFORMING)//相位识别特征信息告知
	{
		postWork->work = find_feature_info_collect_task;
	}
	else if(pIndentificationNewHeader->Collectiontype == e_DISTINGUISH_RESULT_INFO)//台区识别结果查询
	{
    	postWork->work = find_indentification_bf_task_info;
	}
    post_app_work(postWork);
	
#elif defined(STATIC_NODE)
	
	app_printf("type  :  %s\n",
	pIndentificationNewHeader->Collectiontype == e_FEATURE_COLLECT_START?"e_FEATURE_COLLECT_START":
	pIndentificationNewHeader->Collectiontype == e_FEATURE_INFO_GATHER?"e_FEATURE_INFO_GATHER":
	pIndentificationNewHeader->Collectiontype == e_FEATURE_INFO_INFORMING?"e_FEATURE_INFO_INFORMING":
	pIndentificationNewHeader->Collectiontype == e_DISTINGUISH_RESULT_QUERY?"e_DISTINGUISH_RESULT_QUERY":
	pIndentificationNewHeader->Collectiontype == e_DISTINGUISH_RESULT_INFO?"e_DISTINGUISH_RESULT_INFO":"UNKOWN");
	IndenApsRecvPacketSeq = pIndentificationNewHeader->PacketSeq;

	switch (pIndentificationNewHeader->Collectiontype)
	{
		case e_FEATURE_COLLECT_START:
			if(pIndentificationNewHeader->Featuretypes != e_PERIOD)
			{
				//if(memcmp(pIndentificationNewHeader->Macaddr, GetCCOAddr(), 6) ==0)//过滤其他网络CCO发送的启动命令
					//DealFeatureCollectStart(pIndentificationNewHeader);
			}
			break;
		case e_FEATURE_INFO_GATHER://集中式
			//DealFeatureInfoGather(pIndentificationNewHeader);
			break;
		case e_FEATURE_INFO_INFORMING:
			//add_ccomacaddr(pMsduDataInd->SNID,pIndentificationNewHeader->Macaddr);
			//DealFeatureInfoInforming(pIndentificationNewHeader);
			break;
		case e_DISTINGUISH_RESULT_QUERY:
			//if(memcmp(pIndentificationNewHeader->Macaddr,nnib.MacAddr,6) ==0) //过滤其他网络CCO要结果
			//DealDistinguishResultQuery(pIndentificationNewHeader);
			break;

		default:
			break;
	}
	
#endif

}
*/

static void IndentificationAreaInd(work_t *work)
{
    
	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
	APDU_HEADER_t	  *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	
	INDENTIFICATION_NEW_HEADER_t  *pIndentificationNewHeader = (INDENTIFICATION_NEW_HEADER_t *)pApsHeader->Apdu;
	
	U16 			   PayloadLen = 0;
	INDENTIFICATION_IND_t      *pIndentificationInd = NULL;
	
    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(INDENTIFICATION_NEW_HEADER_t);

	pIndentificationInd = 
	                 (INDENTIFICATION_IND_t*)zc_malloc_mem(sizeof(INDENTIFICATION_IND_t) + PayloadLen, "Indentification", MEM_TIME_OUT);

	pIndentificationInd->stei           = pMsduDataInd->Stei;
    pIndentificationInd->DatagramSeq    = pIndentificationNewHeader->PacketSeq;
    pIndentificationInd->SNID           = pMsduDataInd->SNID;
    
	pIndentificationInd->Featuretypes   = pIndentificationNewHeader->Featuretypes;
	pIndentificationInd->Collectiontype = pIndentificationNewHeader->Collectiontype;
    __memcpy(pIndentificationInd->DstMacAddr, pMsduDataInd->DstAddress, 6);
    if(pMsduDataInd->SendType == e_UNICAST_FREAM)
    {
        __memcpy(pIndentificationInd->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
    }
    else
    {
        __memcpy(pIndentificationInd->SrcMacAddr, pIndentificationNewHeader->Macaddr, 6);
    }
	pIndentificationInd->payloadlen     = PayloadLen;
    __memcpy(pIndentificationInd->payload, pIndentificationNewHeader->Payload, pIndentificationInd->payloadlen);

    if(indentification_ind_hook != NULL)
    {
        indentification_ind_hook(pIndentificationInd);
    }
    
	zc_free_mem(pIndentificationInd);

    return;
}


static void QueryIdInfoProc(work_t *work)
{
    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;    
    QUERY_IDINFO_HEADER_t  *pQueryIdInfoHeader = (QUERY_IDINFO_HEADER_t*)pApsHeader->Apdu;
    
            
    //ApsQueryIDRecvPacketSeq = pQueryIdInfoHeader->PacketSeq;
            
    app_printf("ds: ");
    dump_buf(pMsduDataInd->DstAddress, 6);
            
    app_printf("sa: ");
    dump_buf(pMsduDataInd->SrcAddress, 6);
    if(memcmp(pMsduDataInd->SrcAddress,BroadcastExternAddr,6 ) ==0 )
    {
        return;
    }

#if defined(STATIC_MASTER)
    //MULTI_TASK_UP_t     *pMultiTaskUp;
    QUERY_IDINFO_CFM_t  *pQueryIdInfoCfm = NULL;
    U16      PayloadLen = 0;

    PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(QUERY_IDINFO_HEADER_t);

    pQueryIdInfoCfm = (QUERY_IDINFO_CFM_t*)zc_malloc_mem(sizeof(QUERY_IDINFO_CFM_t) + PayloadLen,
                                                                 "ModuleQuery",MEM_TIME_OUT);

    //pQueryIdInfoCfm = (QUERY_IDINFO_CFM_t*)pMultiTaskUp->Asdu;

    pQueryIdInfoCfm->Status     = e_SUCCESS;    
    pQueryIdInfoCfm->IdType     = pQueryIdInfoHeader->IdType;
    pQueryIdInfoCfm->AsduLength = PayloadLen;
    __memcpy(pQueryIdInfoCfm->Asdu, pQueryIdInfoHeader->Asdu, pQueryIdInfoCfm->AsduLength);
    
    __memcpy(pQueryIdInfoCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(pQueryIdInfoCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);
            
    //pMultiTaskUp->pListHeader = &MODULE_ID_QUERY_LIST;
    //pMultiTaskUp->Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    //pMultiTaskUp->Upoffset = (U8)offset_of(QUERY_IDINFO_CFM_t , SrcMacAddr);
    //pMultiTaskUp->Cmplen = sizeof(pQueryIdInfoCfm->SrcMacAddr);
    
    //pMultiTaskUp->AsduLength   = sizeof(QUERY_IDINFO_CFM_t) + pQueryIdInfoCfm->AsduLength;

    //multi_trans_find_plc_task_up_info(pMultiTaskUp);
    if(cco_moduleid_query_cfm_hook != NULL)
    {
        cco_moduleid_query_cfm_hook(pQueryIdInfoCfm);
    }
    
    zc_free_mem(pQueryIdInfoCfm);
    
#elif defined(STATIC_NODE)
	if(getHplcTestMode() == RD_CTRL)
	{
		QUERY_IDINFO_CFM_t	*pQueryIdInfoCfm = NULL;
		U16 	 PayloadLen = 0;
		
		PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(QUERY_IDINFO_HEADER_t);
		
		pQueryIdInfoCfm = (QUERY_IDINFO_CFM_t*)zc_malloc_mem(sizeof(QUERY_IDINFO_CFM_t) + PayloadLen,
																	 "ModuleQuery",MEM_TIME_OUT);
				
		pQueryIdInfoCfm->Status 	= e_SUCCESS;	
		pQueryIdInfoCfm->IdType 	= pQueryIdInfoHeader->IdType;
		pQueryIdInfoCfm->AsduLength = PayloadLen;
		__memcpy(pQueryIdInfoCfm->Asdu, pQueryIdInfoHeader->Asdu, pQueryIdInfoCfm->AsduLength);
		
		__memcpy(pQueryIdInfoCfm->DstMacAddr, pMsduDataInd->DstAddress, 6);
		__memcpy(pQueryIdInfoCfm->SrcMacAddr, pMsduDataInd->SrcAddress, 6);

		if(rdctrl_moduleid_query_cfm_hook != NULL)
		{
			rdctrl_moduleid_query_cfm_hook(pQueryIdInfoCfm);
		}
		return;
	}
    else
    {
        if(is_rd_ctrl_tei(pMsduDataInd->Stei) && GetNodeState() != e_NODE_ON_LINE)
        {
            extern void StopThisEntryNet();
            StopThisEntryNet();
            extern void modify_scanband_timer(uint32_t first);
            modify_scanband_timer(60*1000);
            app_printf("is_rd_ctrl_tei,Stay\n");
        }
    }
    QUERY_IDINFO_IND_t   QueryIdInfoInd;
    
    QueryIdInfoInd.IdType = pQueryIdInfoHeader->IdType;
    QueryIdInfoInd.PacketSeq = pQueryIdInfoHeader->PacketSeq;
    QueryIdInfoInd.Srctei = pMsduDataInd->Stei;
    __memcpy(QueryIdInfoInd.DstMacAddr, pMsduDataInd->DstAddress, 6);
    __memcpy(QueryIdInfoInd.SrcMacAddr, pMsduDataInd->SrcAddress, 6);

    //extern void query_id_info_indication(QUERY_IDINFO_IND_t *pQueryIdInfoInd);
    //query_id_info_indication(&QueryIdInfoInd);
    if(memcmp(QueryIdInfoInd.DstMacAddr,GetNnibMacAddr(),6) != 0 && !is_rd_ctrl_tei(pMsduDataInd->Stei))
    {
        
        return;
    }

    if(sta_moduleid_query_ind_hook != NULL)
    {
        sta_moduleid_query_ind_hook(&QueryIdInfoInd);
    }
#endif

}





ApsPacketIdFunc PacketIdIndicationFuncArray[] = 
{
    {e_CONCENTRATOR_ACTIVE_RM,          ReadMeterIndicationProc},
    {e_ROUTER_ACTIVE_RM,                ReadMeterIndicationProc},
    {e_CONCENTRATOR_ACTIVE_CONCURRENT,  ReadMeterIndicationProc},
    
    {e_TIME_CALIBRATE,                  BroadcastTimeCalibrateInd},
    {e_COMMU_TEST,                      CommunicationTestInd},
    {e_EVENT_REPORT,                    StaEventReportInd},
    {e_ZONEAREA_INFO_GATHER,            ZoneAreaInfoGatherInd},
    {e_SLAVE_REGISTER_QUERY,            SlaveRegisterQueryProc},
    {e_SLAVE_REGISTER_START,            SlaveRegisterStartInd},
    {e_SLAVE_REGISTER_STOP,             SlaveRegisterStopInd},
    {e_ACK_NAK,                         AckOrNakInd},
    {e_DATA_AGGREGATE_RM,               DataAggregateReadMeterInd},
    {e_TIME_OVER_MANAGE,				sta_clk_maintain_proc},
    {e_TIME_OVER_QYERY,					query_clk_or_value_proc},
    {e_UPGRADE_START,                   UpgradeStartInd},
    {e_UPGRADE_STOP,                    UpgradeStopInd},
    {e_FILE_TRANS,                      UpgradeFileTransInd},
    {e_FILE_TRANS_LOCAL_BROADCAST,      UpgradeFileTransInd},
    {e_UPGRADE_STATUS_QUERY,            UpgradeStatus_QueryInd},
    {e_UPGRADE_PERFORM,                 UpgradePerformInd},
    {e_STATION_INFO_QUERY,              UpgradeStaInfoQueryInd},
    {e_RDCTRL_CCO,                      RdCtrlCcoInd},
    {e_RDCTRL_TO_UART,                  RdCtrlToUartInd},
    {e_AUTHENTICATION_SECURITY,         AuthenticationSecurityInd},
    {e_INDENTIFICATION_AREA,            IndentificationAreaInd},
    {e_QUERY_ID_INFO,                   QueryIdInfoProc},
    {e_EXACT_CALIBRATE_TIME,            sta_exact_timer_calibrate_ind},
	{e_MODULE_TIME_SYNC,                Module_Time_Sync_Ind},
	{e_CURVE_PROFILE_CFG,               CurveProfileCfgProc},
    {e_CURVE_DATA_CONCRRNT_READ,        ReadMeterIndicationProc},
#ifdef NODE_SGS
		{e_NODE_SGS,					sta_sgs_ind},
#endif
};



void ProcMsduDataIndication(work_t *work)
{
    MSDU_DATA_IND_t          *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t             *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;

	U8  count;
	U8  ii;
	

    
	aps_printf("ApsID %s ! Value=0X%0X\n", pApsHeader->PacketID == e_CONCENTRATOR_ACTIVE_RM ?"e_CONCENTRATOR_ACTIVE_RM":
   										   			 pApsHeader->PacketID == e_ROUTER_ACTIVE_RM?"e_ROUTER_ACTIVE_RM":
													 pApsHeader->PacketID == e_CONCENTRATOR_ACTIVE_CONCURRENT?"e_CONCENTRATOR_ACTIVE_CONCURRENT":
													 pApsHeader->PacketID == e_TIME_CALIBRATE?"e_TIME_CALIBRATE":					   
													 pApsHeader->PacketID == e_COMMU_TEST?"e_COMMU_TEST":		   
													 pApsHeader->PacketID == e_EVENT_REPORT?"e_EVENT_REPORT":				   
													 pApsHeader->PacketID == e_SLAVE_REGISTER_QUERY?"e_SLAVE_REGISTER_QUERY":					   
													 pApsHeader->PacketID == e_SLAVE_REGISTER_START?"e_SLAVE_REGISTER_START":					   
													 pApsHeader->PacketID == e_SLAVE_REGISTER_STOP?"e_SLAVE_REGISTER_STOP":				   
													 pApsHeader->PacketID == e_ACK_NAK?"e_ACK_NAK":					   
													 pApsHeader->PacketID == e_DATA_AGGREGATE_RM?"e_DATA_AGGREGATE_RM":		
													 pApsHeader->PacketID == e_TIME_OVER_MANAGE?"e_TIME_OVER_MANAGE":	
                                                     pApsHeader->PacketID == e_TIME_OVER_QYERY?"e_TIME_OVER_QYERY":
													 pApsHeader->PacketID == e_UPGRADE_START?"e_UPGRADE_START": 					   
													 pApsHeader->PacketID == e_UPGRADE_STOP?"e_UPGRADE_STOP": 					   
													 pApsHeader->PacketID == e_FILE_TRANS?"e_FILE_TRANS":					   
													 pApsHeader->PacketID == e_FILE_TRANS_LOCAL_BROADCAST?"e_FILE_TRANS_LOCAL_BROADCAST":					   
													 pApsHeader->PacketID == e_UPGRADE_STATUS_QUERY?"e_UPGRADE_STATUS_QUERY":				   
													 pApsHeader->PacketID == e_UPGRADE_PERFORM?"e_UPGRADE_PERFORM":				   
													 pApsHeader->PacketID == e_STATION_INFO_QUERY?"e_STATION_INFO_QUERY":	
													 pApsHeader->PacketID == e_RDCTRL_CCO?"e_RDCTRL_CCO":
                                                     pApsHeader->PacketID == e_RDCTRL_TO_UART?"e_RDCTRL_TO_UART":
													 pApsHeader->PacketID == e_AUTHENTICATION_SECURITY?"e_AUTHENTICATION_SECURITY":	
                                                     pApsHeader->PacketID == e_ZONEAREA_INFO_GATHER?"e_ZONEAREA_INFO_GATHER":
													 pApsHeader->PacketID == e_INDENTIFICATION_AREA?"e_INDENTIFICATION_AREA":
                                                     pApsHeader->PacketID == e_QUERY_ID_INFO?"e_QUERY_ID_INFO":
                                                     pApsHeader->PacketID == e_EXACT_CALIBRATE_TIME ? "e_EXACT_CALIBRATE_TIME" :
													 pApsHeader->PacketID == e_MODULE_TIME_SYNC?"e_MODULE_TIME_SYNC":
													 pApsHeader->PacketID == e_CURVE_PROFILE_CFG?"e_CURVE_PROFILE_CFG":
													 pApsHeader->PacketID == e_CURVE_DATA_CONCRRNT_READ?"e_CURVE_DATA_CONCRRNT_READ":
													 pApsHeader->PacketID == e_NODE_SGS?"e_NODE_SGS":
													 pApsHeader->PacketID == e_SYS_IDLE?"e_SYS_IDLE":
													 	"UNKOWN" ,pApsHeader->PacketID);

    
	
	//LED灯的控制
#if defined(ZC3750STA)
		led_rx_ficker();
#elif defined(ZC3750CJQ2)
		cjq_led_rx_ficker();
#endif


	count = (sizeof(PacketIdIndicationFuncArray) / sizeof(ApsPacketIdFunc));
    for(ii = 0; ii < count; ii++)
    {
        if(PacketIdIndicationFuncArray[ii].PacketId ==pApsHeader->PacketID)
        {
            PacketIdIndicationFuncArray[ii].Func(work);
        }
    }

}




static void ReadMeterConfirmProc(work_t *work)
{

#if defined(STATIC_MASTER)
    MSDU_DATA_CFM_t   *pMsduDataCfm = (MSDU_DATA_CFM_t *)work->buf;
    

    READ_METER_CFM_t  *pReadMterCfm = NULL;
    pReadMterCfm = zc_malloc_mem(sizeof(READ_METER_CFM_t) ,"pReadMterCfm",MEM_TIME_OUT);
    pReadMterCfm->Status = pMsduDataCfm->Status;
    pReadMterCfm->PacketID = (pMsduDataCfm->MsduHandle >> 16) & 0xFFFF;
    pReadMterCfm->ApsSeq = pMsduDataCfm->MsduHandle & 0xFFFF;
    app_printf("Status %d PacketID %02x ApsSeq %02x\n",pReadMterCfm->Status,pReadMterCfm->PacketID,pReadMterCfm->ApsSeq);
    //if(pReadMterCfm->Status >= e_ENQUEUE_FAIL && pReadMterCfm->Status <=e_SOLT_ERR)
    if(pReadMterCfm->Status == e_ENQUEUE_FAIL)
    {
        if(cco_read_meter_cfm_hook != NULL)
        {   
            cco_read_meter_cfm_hook(pReadMterCfm);
        }
    }
    zc_free_mem(pReadMterCfm);
#endif

}


static void BroadcastTimeCalibrateCfm(work_t *work)
{
#if defined(STATIC_MASTER)
    MSDU_DATA_CFM_t   *pMsduDataCfm = (MSDU_DATA_CFM_t *)work->buf;
    TIME_CALIBRATE_CFM_t  *pTimeCalibrateCfm =NULL;
    pTimeCalibrateCfm = zc_malloc_mem(sizeof(FILE_TRANS_CFM_t) ,"pTimeCalibrateCfm",MEM_TIME_OUT);
    pTimeCalibrateCfm->Status = pMsduDataCfm->Status;
    time_calibrate_cfm_pro(pTimeCalibrateCfm->Status);
    zc_free_mem(pTimeCalibrateCfm);
#endif
}
static void CommunicationTestCfm(work_t *work){}
static void StaEventReportCfm(work_t *work){}
static void ZoneAreaInfoGatherCfm(work_t *work){}
static void SlaveRegisterQueryCfm(work_t *work){}
static void SlaveRegisterStartCfm(work_t *work)
{
    MSDU_DATA_CFM_t          *pMsduDataCfm = (MSDU_DATA_CFM_t *)work->buf;    
    SLAVE_REGISTER_START_CFM_t   *pSlaveRegisterStartCfm = NULL;

    

    work_t *postWork = zc_malloc_mem(sizeof(work_t)+sizeof(SLAVE_REGISTER_START_CFM_t),"SlaveRegStartCfm",MEM_TIME_OUT);
    pSlaveRegisterStartCfm = (SLAVE_REGISTER_START_CFM_t*)postWork->buf;
    
    pSlaveRegisterStartCfm->Status = pMsduDataCfm->Status;
    
    postWork->work = NULL;
    
    // post
    postWork->msgtype = SREGCFM;
    post_app_work(postWork);

}
static void SlaveRegisterStopCfm(work_t *work){}
static void AckOrNakCfm(work_t *work){}
static void DataAggregateReadMeterCfm(work_t *work){}
static void UpgradeStartCfm(work_t *work){}
static void UpgradeStopCfm(work_t *work){}
static void UpgradeFileTransCfm(work_t *work)
{

#if defined(STATIC_MASTER)
    MSDU_DATA_CFM_t   *pMsduDataCfm = (MSDU_DATA_CFM_t *)work->buf;
    

    FILE_TRANS_CFM_t  *pFileTransCfm =NULL;
    pFileTransCfm = zc_malloc_mem(sizeof(FILE_TRANS_CFM_t) ,"pFileTransCfm",MEM_TIME_OUT);
    pFileTransCfm->Status = pMsduDataCfm->Status;
    upgrade_cco_file_trans_cfm_pro(pFileTransCfm->Status);
    zc_free_mem(pFileTransCfm);
#endif

}
/*static void UpgradeFileTransBroadcastCfm(work_t *work){}*/
static void UpgradeStatus_QueryCfm(work_t *work){}
static void UpgradePerformCfm(work_t *work){}
static void UpgradeStaInfoQueryCfm(work_t *work){}
static void RdCtrlCfm(work_t *work){}
static void AuthenticationSecurityCfm(work_t *work){}
static void IndentificationAreaCfm(work_t *work){}
static void QueryIdInfoCfm(work_t *work){}




ApsPacketIdFunc PacketIdConfirmFuncArray[] = 
{
    {e_CONCENTRATOR_ACTIVE_RM,          ReadMeterConfirmProc},
    {e_ROUTER_ACTIVE_RM,                NULL},
    {e_CONCENTRATOR_ACTIVE_CONCURRENT,  ReadMeterConfirmProc},
    
    {e_TIME_CALIBRATE,                  BroadcastTimeCalibrateCfm},
    {e_COMMU_TEST,                      CommunicationTestCfm},
    {e_EVENT_REPORT,                    StaEventReportCfm},
    {e_ZONEAREA_INFO_GATHER,            ZoneAreaInfoGatherCfm},
    {e_SLAVE_REGISTER_QUERY,            SlaveRegisterQueryCfm},
    {e_SLAVE_REGISTER_START,            SlaveRegisterStartCfm},
    {e_SLAVE_REGISTER_STOP,             SlaveRegisterStopCfm},
    {e_ACK_NAK,                         AckOrNakCfm},
    {e_DATA_AGGREGATE_RM,               DataAggregateReadMeterCfm},
    {e_UPGRADE_START,                   UpgradeStartCfm},
    {e_UPGRADE_STOP,                    UpgradeStopCfm},
    {e_FILE_TRANS,                      UpgradeFileTransCfm},
    {e_FILE_TRANS_LOCAL_BROADCAST,      UpgradeFileTransCfm},
    {e_UPGRADE_STATUS_QUERY,            UpgradeStatus_QueryCfm},
    {e_UPGRADE_PERFORM,                 UpgradePerformCfm},
    {e_STATION_INFO_QUERY,              UpgradeStaInfoQueryCfm},
    {e_RDCTRL_CCO,                      RdCtrlCfm},
    {e_AUTHENTICATION_SECURITY,         AuthenticationSecurityCfm},
    {e_INDENTIFICATION_AREA,            IndentificationAreaCfm},
    {e_QUERY_ID_INFO,                   QueryIdInfoCfm},

};



void ProcMsduDataConfirm(work_t *work)
{
    MSDU_DATA_CFM_t          *pMsduDataCfm = (MSDU_DATA_CFM_t *)work->buf;
	U8 packetId;
    //U8 handle;
    U8  count;
    U8  ii;

    //handle    = (U8)(pMsduDataCfm->MsduHandle & 0xFF);
    packetId = (U8)(pMsduDataCfm->MsduHandle >> 16);

	//aps_printf("pMsduDataCfm->MsduHandle :%04x ,Status=%d\n", pMsduDataCfm->MsduHandle,pMsduDataCfm->Status);



    count = (sizeof(PacketIdConfirmFuncArray) / sizeof(ApsPacketIdFunc));
    for(ii = 0; ii < count; ii++)
    {
        if(PacketIdConfirmFuncArray[ii].PacketId == packetId)
        {
        	if(PacketIdConfirmFuncArray[ii].Func != NULL)
            	PacketIdConfirmFuncArray[ii].Func(work);
        }
    }
}





read_meter_ind_fn  sta_read_meter_ind_hook = NULL;
read_meter_ind_fn  cco_read_meter_ind_hook = NULL;
read_meter_cfm_fn  cco_read_meter_cfm_hook = NULL;

time_calibrate_ind_fn  time_calibrate_hook = NULL;
commu_test_ind_fn commu_test_ind_hook = NULL;

event_report_ind_fn sta_event_report_hook = NULL;
event_report_ind_fn cco_event_report_hook = NULL;

register_query_cfm_fn cco_register_query_cfm_hook = NULL;
register_query_ind_fn  sta_register_query_ind_hook = NULL;

upgrade_start_cfm_fn upgrade_start_cfm_hook = NULL;
upgrade_start_ind_fn upgrade_start_ind_hook = NULL;
upgrade_stop_ind_fn upgrade_stop_ind_hook = NULL;
upgrade_file_trans_ind_fn  upgrade_filetrans_ind_hook = NULL;
upgrade_status_query_cfm_fn upgrade_status_query_cfm_hook = NULL;
upgrade_status_query_ind_fn upgrade_status_query_ind_hook = NULL;
upgrade_stainfo_query_ind_fn  upgrade_stainfo_query_ind_hook = NULL;
upgrade_stainfo_query_cfm_fn  upgrade_stainfo_query_cfm_hook = NULL;
upgrade_perform_ind_fn upgrade_perform_ind_hook = NULL;

moduleid_query_cfm_fn  cco_moduleid_query_cfm_hook = NULL;
moduleid_query_cfm_fn rdctrl_moduleid_query_cfm_hook = NULL;

moduleid_query_ind_fn  sta_moduleid_query_ind_hook = NULL;

indentification_ind_fn  indentification_ind_hook = NULL;

rdctrl_ind_fn  rdctrl_ind_hook = NULL;
rdctrl_to_uart_ind_fn  rdctrl_to_uart_ind_hook = NULL;

check_CB_FacTestMode  check_CB_FacTestMode_hook = NULL;


Query_SwORValue_ind_fn  sta_QuerySwORValue_ind_hook = NULL;
Query_SwORValue_cfm_fn  cco_QuerySwORValue_cfm_hook = NULL;

Set_SwORValue_ind_fn  sta_SetSwORValue_ind_hook = NULL;
Set_SwORValue_cfm_fn  cco_SetSwORValue_cfm_hook = NULL;







/*
void sta_read_meter_ind(read_meter_ind_fn fn)
{
    sta_read_meter_ind_hook = fn;
    return;
}
*/



                                                                                                  
