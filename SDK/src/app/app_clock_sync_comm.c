#include "flash.h"
#include "app_clock_os_ex.h"
#include "aps_layer.h"
#include "app_gw3762.h"
#include "app_base_multitrans.h"
#include "app_sysdate.h"
#include "Datalinkdebug.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"

void ApsExactTimeCalibrateRequest(TIMER_CALIBRATE_REQ_t *TimerCalBrateReq)
{
	U32 CCONTB = 0;


	MSDU_DATA_REQ_t         *pMsduDataReq 		  = NULL;
    APDU_HEADER_t           *pApsHeader           = NULL;
	TIMER_CALIBRATE_HEADER_t  *Timer_CalibrateHeader = NULL;
	U16   msdu_length = 0;

	msdu_length = sizeof(APDU_HEADER_t) + sizeof(TIMER_CALIBRATE_HEADER_t) + TimerCalBrateReq->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

	pMsduDataReq	= (MSDU_DATA_REQ_t*)post_work->buf;
	pApsHeader	  = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	Timer_CalibrateHeader = (TIMER_CALIBRATE_HEADER_t*)pApsHeader->Apdu;
	CCONTB = get_phy_tick_time();

	
	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_EXACT_CALIBRATE_TIME;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    //填设置校时报文头
    Timer_CalibrateHeader->ProtocolVer  = PROTOCOL_VERSION_NUM;
    Timer_CalibrateHeader->HeaderLen	= sizeof(TIMER_CALIBRATE_HEADER_t);
    Timer_CalibrateHeader->DataRelayLen = TimerCalBrateReq->AsduLength;
    Timer_CalibrateHeader->PacketSeq    = ++ApsSendPacketSeq;
    Timer_CalibrateHeader->NTB    	    = CCONTB;

    //Timer_CalibrateHeader->AsduLength = TimerCalBrateReq->AsduLength;
    __memcpy(Timer_CalibrateHeader->Asdu,TimerCalBrateReq->Asdu,TimerCalBrateReq->AsduLength);

    ApsPostPacketReq(post_work, msdu_length, 0, TimerCalBrateReq->DstMacAddr, 
                         TimerCalBrateReq->SrcMacAddr, TimerCalBrateReq->SendType, (pApsHeader->PacketID << 16) + (++ApsHandle), TIME_CALIBRATE_LID);
}

void ApsQueryClkSwAndOverValueRequest(QUERY_CLKSWOROVER_VALUE_REQ_t *pQuery_CLKSwOROverValueReq)
{
	 MSDU_DATA_REQ_t		 *pMsduDataReq		   = NULL;
	 APDU_HEADER_t			 *pApsHeader		   = NULL;
	 QUERY_CLKSWOROVER_VALUE_HEADER_t *Query_ClkOR_ValueHeader = NULL;
	 U16   msdu_length = 0;
	 
	 msdu_length = sizeof(APDU_HEADER_t) + sizeof(QUERY_CLKSWOROVER_VALUE_HEADER_t) + pQuery_CLKSwOROverValueReq->AsduLength;
	 work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
	 
	 pMsduDataReq	 = (MSDU_DATA_REQ_t*)post_work->buf;
	 pApsHeader    = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	 Query_ClkOR_ValueHeader = (QUERY_CLKSWOROVER_VALUE_HEADER_t*)pApsHeader->Apdu;
 
	 //__memcpy(Query_ClkOR_ValueHeader->Asdu,pQuery_CLKSwOROverValueReq->Asdu,pQuery_CLKSwOROverValueReq->AsduLength);
	 //Query_ClkOR_ValueHeader->AsduLength = pQuery_CLKSwOROverValueReq->AsduLength;
	 
	 // 填APS通用帧头
	 pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
	 pApsHeader->PacketID	= e_TIME_OVER_QYERY;
	 pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;
 
	 //填设置开关报文头
	 Query_ClkOR_ValueHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
	 Query_ClkOR_ValueHeader->HeaderLen  = sizeof(QUERY_CLKSWOROVER_VALUE_HEADER_t);
	 
	 __memcpy(Query_ClkOR_ValueHeader->Macaddr,pQuery_CLKSwOROverValueReq->Macaddr,6);//下行帧头中带的表地址
 	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
		ChangeMacAddrOrder(Query_ClkOR_ValueHeader->Macaddr);
	}
	 ApsPostPacketReq(post_work, msdu_length, 0, pQuery_CLKSwOROverValueReq->DstMacAddr, 
						  pQuery_CLKSwOROverValueReq->SrcMacAddr, pQuery_CLKSwOROverValueReq->SendType, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);
}



void ApsSetCLKSWRequest(CLK_OVER_MANAGE_REQ_t      *pSetClkSwReq)
{
	MSDU_DATA_REQ_t         *pMsduDataReq 		  = NULL;
    APDU_HEADER_t           *pApsHeader           = NULL;
	CLOCK_MAINTAIN_HEADER_t *Clk_OverManageHeader = NULL;
	U16   msdu_length = 0;
	
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(CLOCK_MAINTAIN_HEADER_t) + pSetClkSwReq->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
	pMsduDataReq	= (MSDU_DATA_REQ_t*)post_work->buf;
	pApsHeader	  = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	Clk_OverManageHeader = (CLOCK_MAINTAIN_HEADER_t*)pApsHeader->Apdu;

	__memcpy(Clk_OverManageHeader->Asdu,pSetClkSwReq->Asdu,pSetClkSwReq->AsduLength);
	
	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_TIME_OVER_MANAGE;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    //填设置开关报文头
    Clk_OverManageHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    Clk_OverManageHeader->HeaderLen	= sizeof(CLOCK_MAINTAIN_HEADER_t);
    Clk_OverManageHeader->Type  = e_AUTO_TIMING_SWITCH;
    //Clk_OverManageHeader->DataLen 	= 0x01; 
    Clk_OverManageHeader->AsduLength = pSetClkSwReq->AsduLength;
    __memcpy(Clk_OverManageHeader->Macaddr,pSetClkSwReq->DstMacAddr,6);
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
		ChangeMacAddrOrder(Clk_OverManageHeader->Macaddr);
	}
    ApsPostPacketReq(post_work, msdu_length, 0, pSetClkSwReq->DstMacAddr, 
                         pSetClkSwReq->SrcMacAddr, pSetClkSwReq->SendType, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);
}

void ApsQueryClkSwAndOverValueResp(QUERY_CLKSWOROVER_VALUE_RESPONSE_t *pQuerySwORValueRsp)
{
	MSDU_DATA_REQ_t         *pMsduDataReq 		  = NULL;
    APDU_HEADER_t           *pApsHeader           = NULL;
	QUERY_CLKSWOROVER_VALUE_UP_HEADER_t *Query_ClkOR_ValueHeader = NULL;
	U16   msdu_length = 0;
	
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(QUERY_CLKSWOROVER_VALUE_UP_HEADER_t) + pQuerySwORValueRsp->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
	pMsduDataReq	= (MSDU_DATA_REQ_t*)post_work->buf;
	pApsHeader	  = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	Query_ClkOR_ValueHeader = (QUERY_CLKSWOROVER_VALUE_UP_HEADER_t*)pApsHeader->Apdu;

	//__memcpy(Query_ClkOR_ValueHeader->Asdu,pQuerySwORValueRsp->Asdu,pQuerySwORValueRsp->AsduLength);
	//Query_ClkOR_ValueHeader->AsduLength = pQuerySwORValueRsp->AsduLength;
	
	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_TIME_OVER_QYERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    //填设置开关报文头
    Query_ClkOR_ValueHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    Query_ClkOR_ValueHeader->HeaderLen	= sizeof(QUERY_CLKSWOROVER_VALUE_UP_HEADER_t);
    
    __memcpy(Query_ClkOR_ValueHeader->Macaddr,pQuerySwORValueRsp->Macaddr,6);
	Query_ClkOR_ValueHeader->AutoSwState = pQuerySwORValueRsp->AutoSwState;
	Query_ClkOR_ValueHeader->Res1 = 0;
	Query_ClkOR_ValueHeader->Time_Over_Value = pQuerySwORValueRsp->Time_Over_Value;
	
    ApsPostPacketReq(post_work, msdu_length, 0, pQuerySwORValueRsp->DstMacAddr, 
                         pQuerySwORValueRsp->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);
}

void ApsSetClkSwAndOverValueResp(CLOCK_MAINTAIN_RSP_t *pSetSwAndOverValueResp)
{
	app_printf("ApsSetClkSwAndOverValueRespn\n");
  	MSDU_DATA_REQ_t         *pMsduDataReq 		  = NULL;
    APDU_HEADER_t           *pApsHeader           = NULL;
	CLOCK_MAINTAIN_HEADER_t *Set_ClkOR_ValueHeader = NULL;
	U16   msdu_length = 0;
	
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(Set_ClkOR_ValueHeader) + pSetSwAndOverValueResp->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
	pMsduDataReq	= (MSDU_DATA_REQ_t*)post_work->buf;
	pApsHeader	  = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	Set_ClkOR_ValueHeader = (CLOCK_MAINTAIN_HEADER_t*)pApsHeader->Apdu;
	
	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_TIME_OVER_MANAGE;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    //填设置开关报文头
    Set_ClkOR_ValueHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    Set_ClkOR_ValueHeader->HeaderLen	= sizeof(CLOCK_MAINTAIN_HEADER_t);
    
	
    ApsPostPacketReq(post_work, msdu_length, 0, pSetSwAndOverValueResp->DstMacAddr, 
                         pSetSwAndOverValueResp->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);
}

void ApsClockMaintainRequest(CLOCK_MAINTAIN_REQ_t *pClockMaintainReq_t)
{
	MSDU_DATA_REQ_t         *pMsduDataReq 		  = NULL;
    APDU_HEADER_t           *pApsHeader           = NULL;
	CLOCK_MAINTAIN_HEADER_t *ClockMaintainHeader = NULL;
	U16   msdu_length = 0;
	
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(CLOCK_MAINTAIN_HEADER_t) + pClockMaintainReq_t->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
	pMsduDataReq	= (MSDU_DATA_REQ_t*)post_work->buf;
	pApsHeader	  = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	ClockMaintainHeader = (CLOCK_MAINTAIN_HEADER_t*)pApsHeader->Apdu;

	__memcpy(ClockMaintainHeader->Asdu,pClockMaintainReq_t->Asdu,pClockMaintainReq_t->AsduLength);
	ClockMaintainHeader->AsduLength = pClockMaintainReq_t->AsduLength;

	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_TIME_OVER_MANAGE;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;
    
	// 填报文头信息
	ClockMaintainHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
	ClockMaintainHeader->HeaderLen  = sizeof(CLOCK_MAINTAIN_HEADER_t);
	__memcpy(ClockMaintainHeader->Macaddr, pClockMaintainReq_t->DstMacAddr, 6);
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
		ChangeMacAddrOrder(ClockMaintainHeader->Macaddr);
	}
	ClockMaintainHeader->Type 		 = pClockMaintainReq_t->Type;
	ClockMaintainHeader->AsduLength		  = pClockMaintainReq_t->AsduLength;
	app_printf("aps_addr:\n");
	dump_buf(pClockMaintainReq_t->DstMacAddr,6);
	ApsPostPacketReq(post_work, msdu_length, 0, pClockMaintainReq_t->DstMacAddr, 
                         pClockMaintainReq_t->SrcMacAddr, pClockMaintainReq_t->SendType, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);

}
//陕西，湖南使用
void ApsModuleTimeSyncRequest(MODULE_TIMESYNC_REQ_t *pModuleTimeSyncReq_t)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;
    MODULE_TIMESYNC_HEADER_t  *pModTimeSyncHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(MODULE_TIMESYNC_HEADER_t) + pModuleTimeSyncReq_t->DateLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pModTimeSyncHeader = (MODULE_TIMESYNC_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_MODULE_TIME_SYNC;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

	pModTimeSyncHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
	pModTimeSyncHeader->HeaderLen   = sizeof(MODULE_TIMESYNC_HEADER_t);
    pModTimeSyncHeader->Reserve1 = 0;
    pModTimeSyncHeader->Reserve2 = 0;
    pModTimeSyncHeader->DataLength = pModuleTimeSyncReq_t->DateLen;
    __memcpy(pModTimeSyncHeader->Data, pModuleTimeSyncReq_t->Date, pModTimeSyncHeader->DataLength);

	ApsPostPacketReq(post_work, msdu_length, 0, pModuleTimeSyncReq_t->DstMacAddr, 
                     pModuleTimeSyncReq_t->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    

    return;
}




















