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
#include "trios.h"
#include "sys.h"
#include "vsh.h"
#include "SchTask.h"
//#include "ApsTask.h"
#include "ProtocolProc.h"
#include "ZCsystem.h"
#include "aps_layer.h"
#include "app_rdctrl.h"
#include "Datalinkdebug.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "app_upgrade_comm.h"
#include "app_cnf.h"
#include "DatalinkTask.h"
#include "dev_manager.h"
#include "app_area_indentification_common.h"
#include "app_clock_os_ex.h"
#include "app_data_freeze_cco.h"


typedef struct
{
	U8	LifeTime;
	U16	AsduSeq;
	U16	SrcTEI;
} __PACKED ASDU_SEQ;
ASDU_SEQ ASDUSeq[20];


U32   ApsSendPacketSeq = 0;
U32   ApsRecvPacketSeq = 0xffff;
U16   ApsEventSendPacketSeq = 0;
U16   ApsEventRecvPacketSeq = 0xffff;


U32	  ApsSendRegMeterSeq = 0;
//U32	  ApsRecvRegMeterSeq = 0;



U8   ApsHandle = 0;








U8 CheckApsSeq(U16 Seq , U16 SrcTei)
{
	U8 i;

	for(i=0;i<20;i++)
	{
		if(Seq==ASDUSeq[i].AsduSeq && SrcTei == ASDUSeq[i].SrcTEI)
		{
			ASDUSeq[i].LifeTime = 0;
			//app_printf("ASDUSeq  same exsit!\n");
			return FALSE;	
		}
	}

	return TRUE;
}

void AddAsduSeq(U16 MsduSeq , U16 SrcTei)
{
	U8 i;
	for(i=0;i<20;i++)
	{
		if(0xffff== ASDUSeq[i].AsduSeq && 0xffff == ASDUSeq[i].SrcTEI)
		{

			ASDUSeq[i].AsduSeq = MsduSeq;
			ASDUSeq[i].SrcTEI = SrcTei;
			ASDUSeq[i].LifeTime = 0;
			break;

		}
	}
	//位置满,first in first out
	if(i==20)
	{
		__memmove((U8*)&ASDUSeq[0],(U8*)&ASDUSeq[1],19*sizeof(ASDU_SEQ));
		ASDUSeq[19].AsduSeq = MsduSeq;
		ASDUSeq[19].SrcTEI = SrcTei;
		ASDUSeq[19].LifeTime = 0;
	}

}


void UpAsduSeq()
{
	U8 i ,j;
	for(i=0;i<20;i++)
	{
		if(0xffff!= ASDUSeq[i].SrcTEI)
		{
			ASDUSeq[i].LifeTime++;
			if(ASDUSeq[i].LifeTime >=5)
			{
				memset((U8*)&ASDUSeq[i],0xff,sizeof(ASDU_SEQ));
			}
		}

	}
	for(i=0;i<20;i++)//将空的位置，剔除
	{
		if(0xffff == ASDUSeq[i].AsduSeq)
		{
			if(i==19)return;//如果最后一个位置为空，更新完毕
			for(j=i+1;j<20;j++)
			{
				if(0xffff != ASDUSeq[j].AsduSeq)
				{
					__memcpy((U8*)&ASDUSeq[i],(U8*)&ASDUSeq[j],sizeof(ASDU_SEQ));
					memset((U8*)&ASDUSeq[j],0xff,sizeof(ASDU_SEQ));
					break;
				}
			}
		}
	}
}



U8 CheckSTAOnlineSate()
{
#if defined(STATIC_MASTER)
	return TRUE;
#elif defined(STATIC_NODE)
	if( GetNodeState() == e_NODE_ON_LINE)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}	
#endif
}


/***********************************************************************************************/


//-------------------------------------------------------------------------------------------------
// 函数名：static void ApsPostPacketReq(work_t *work, U16 msduLen, U16 dtei, U8 *pDstMacAddr, U8 *pSrcMacAddr, U8 SendType, U32 handle, U8 lid)
// 
// 功能描述：
//     本函数用于应用层组好各种类型帧格式后，统一向下一层（数据链路层）发送消息，传递msdu数据发送请求
// 参数：
//     work_t   *work             APDU消息结构指针
//     U16      msduLen           APDU，也就是MSDU数据长度
//     U16      dtei              目标TEI地址，抄控器数据回传时使用
//     U8       *pDstMacAddr      数据发送目标MAC地址（表地址的反序），广播用全FF
//     U8       *pSrcMacAddr      数据发送源MAC地址
//     U8       SendType          发送类型，e_UNICAST_FREAM或者广播等类型
//     U32      handle            数据发送句柄，高16位为应用层PacketID，低16位为报文序号
//     U8       lid               Link ID如READ_METER_LID定义为3等
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
void ApsPostPacketReq(work_t *work, U16 msduLen, U16 dtei, U8 *pDstMacAddr, U8 *pSrcMacAddr, U8 SendType, U32 handle, U8 lid)
{
    MSDU_DATA_REQ_t     *pMsduDataReq = (MSDU_DATA_REQ_t*)work->buf;

    pMsduDataReq->MsduLength = msduLen;
   // dump_buf(pMsduDataReq->Msdu, msduLen);

    __memcpy(pMsduDataReq->DstAddress, pDstMacAddr, MACADRDR_LENGTH);
    __memcpy(pMsduDataReq->SrcAddress, pSrcMacAddr, MACADRDR_LENGTH);

    //app_printf("pMsduDR-DstAddr: ");
	//dump_buf(pMsduDataReq->DstAddress, 6);
    
    pMsduDataReq->SendType = SendType;
    pMsduDataReq->Handle = handle;       // (pApsHeader->PacketID << 16) + (++ApsHandle);
    pMsduDataReq->LID = lid;             // SLAVE_REGISTER_LID;
	
    pMsduDataReq->dtei = dtei;
    pMsduDataReq->MacAddrUseFlag = e_UNUSE_MACADDR;

    if(e_QUERY_ID_INFO == ((handle >> 16) & 0xff))        //台体测试ID查询增加。
    {
        pMsduDataReq->MacAddrUseFlag = e_USE_MACADDR;
    }
    else if(e_RDCTRL_CCO == ((handle >> 16) & 0xff) || is_rd_ctrl_tei(dtei))
    {
        pMsduDataReq->MacAddrUseFlag = rd_ctrl_info.mac_use;
    }

    // post
    work->work = ProcMsduDataRequest;    //ProcMsduDataRequest;	
    work->msgtype = MSDU_REQ;
    post_datalink_task_work(work);
    #if defined(ZC3750STA)
	/*
	extern ostimer_t *led_t_ctrl_timer;
	timer_stop(led_t_ctrl_timer,TMR_NULL);
	led_t_ctrl(e_EX_CTRL);*/
	led_tx_ficker();
	#endif
}


//-------------------------------------------------------------------------------------------------
// 函数名：void ApsReadMeterRequest(READ_METER_REQ_t *pReadMeterReq)
// 
// 功能描述：
//     本函数用于发送应用层抄表命令报文，上下行都使用，本函数中按参数组织下行或上行
//       的抄表命令的报文帧头，然后再调用ApsPostPacketReq函数通过下层发送出去。
// 参数：
//
//     READ_METER_REQ_t   *pReadMeterReq  抄表数据发送请求数据结构指针，参见结构设计
//
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
void ApsReadMeterRequest(READ_METER_REQ_t *pReadMeterReq)
{
    work_t              *post_work = NULL;
    APDU_HEADER_t       *pApsHeader = NULL;
    MSDU_DATA_REQ_t     *pMsduDataReq = NULL;
    
    U16   msdu_length = 0;

    // app_printf("pReadMeterReq->Asdu.pPayload :0x%08x\n", pReadMeterReq->Asdu.pPayload);
    // dump_buf(pReadMeterReq->Asdu.pPayload, pReadMeterReq->Asdu.PayloadLen);

    
    if(pReadMeterReq->Direction == e_DOWNLINK)  // read meter downlink packet
    {
        READMETER_DOWNLINK_HEADER_t   *pDownlinkHeader = NULL;
        
        msdu_length = sizeof(APDU_HEADER_t)+sizeof(READMETER_DOWNLINK_HEADER_t)+pReadMeterReq->AsduLength;

        post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
        pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
        pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
        pDownlinkHeader = (READMETER_DOWNLINK_HEADER_t*)pApsHeader->Apdu;

        // 填抄表下行帧头
        pDownlinkHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
        pDownlinkHeader->HeaderLen  = sizeof(READMETER_DOWNLINK_HEADER_t);

        pDownlinkHeader->DataProType = pReadMeterReq->ProtocolType;
        pDownlinkHeader->DataLength = pReadMeterReq->AsduLength;
        pDownlinkHeader->PacketSeq = pReadMeterReq->DatagramSeq;//psSendPacketSeq;
        
		//app_printf("pDownlinkHeader->PacketSeq : %d\n",pDownlinkHeader->PacketSeq);
        pDownlinkHeader->DeviceTimeout = pReadMeterReq->Timeout;

        if(pReadMeterReq->ReadMode == e_CONCENTRATOR_ACTIVE_RM || pReadMeterReq->ReadMode == e_ROUTER_ACTIVE_RM)
        {
            pDownlinkHeader->CfgWord = 0;
            pDownlinkHeader->OptionWord = pReadMeterReq->Direction & 0x01;
        }
        else if(pReadMeterReq->ReadMode == e_CONCENTRATOR_ACTIVE_CONCURRENT || pReadMeterReq->ReadMode == e_DATA_AGGREGATE_RM
			|| pReadMeterReq->ReadMode == e_CURVE_DATA_CONCRRNT_READ)
        {
            pDownlinkHeader->CfgWord = pReadMeterReq->CfgWord;
            pDownlinkHeader->OptionWord = pReadMeterReq->PacketIner;
        }

		__memcpy(pDownlinkHeader->Payload, pReadMeterReq->Asdu, pReadMeterReq->AsduLength);

    }
    else if(pReadMeterReq->Direction == e_UPLINK)  // read meter uplink packet
    {
        READMETER_UPLINK_HEADER_t   *pUplinkHeader = NULL;
        
        msdu_length = sizeof(APDU_HEADER_t)+sizeof(READMETER_UPLINK_HEADER_t)+pReadMeterReq->AsduLength;

        post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
        pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
        pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
        pUplinkHeader = (READMETER_UPLINK_HEADER_t*)pApsHeader->Apdu;
        

        // 填抄表上行帧帧头
        pUplinkHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
        pUplinkHeader->HeaderLen = sizeof(READMETER_UPLINK_HEADER_t);
        pUplinkHeader->ResponseState = pReadMeterReq->ResponseState;
        pUplinkHeader->DataProType = pReadMeterReq->ProtocolType;
        pUplinkHeader->DataLength = pReadMeterReq->AsduLength;
        pUplinkHeader->PacketSeq = pReadMeterReq->DatagramSeq;   //ApsRecvPacketSeq;

        if(pReadMeterReq->ReadMode == e_CONCENTRATOR_ACTIVE_RM || pReadMeterReq->ReadMode == e_ROUTER_ACTIVE_RM)
        {
            pUplinkHeader->OptionWord = (pReadMeterReq->Direction & 0x01) << 8;
        }
        else if(pReadMeterReq->ReadMode == e_CONCENTRATOR_ACTIVE_CONCURRENT || pReadMeterReq->ReadMode == e_DATA_AGGREGATE_RM
			|| pReadMeterReq->ReadMode == e_CURVE_DATA_CONCRRNT_READ)
        {
            pUplinkHeader->OptionWord = pReadMeterReq->OptionWord;
        }

		__memcpy(pUplinkHeader->Payload, pReadMeterReq->Asdu, pReadMeterReq->AsduLength);
		
    }

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = pReadMeterReq->ReadMode;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

	app_printf("Requst handle: %d\n",(pApsHeader->PacketID << 16) + pReadMeterReq->DatagramSeq);
    ApsPostPacketReq(post_work, msdu_length, pReadMeterReq->dtei, pReadMeterReq->DstMacAddr, 
    pReadMeterReq->SrcMacAddr, pReadMeterReq->SendType, (pApsHeader->PacketID << 16) + pReadMeterReq->DatagramSeq, READ_METER_LID);
        
    return;
}
void ApsModuleTimesynsRequest(RTC_TIMESYNC_REQUEST_t *pRTCTimeSyncRequestReq)
{
	U32 CCONTB = 0;
	MSDU_DATA_REQ_t         *pMsduDataReq 		  = NULL;
    APDU_HEADER_t           *pApsHeader           = NULL;
	RTC_TIMESYNC_HEADER_t   *RTC_TimerSysncHeader = NULL;
	U16   msdu_length = 0;

	msdu_length = sizeof(APDU_HEADER_t) + sizeof(RTC_TIMESYNC_HEADER_t) ;
	work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

	
	pMsduDataReq	= (MSDU_DATA_REQ_t*)post_work->buf;
	pApsHeader	  = (APDU_HEADER_t*)pMsduDataReq->Msdu;
	RTC_TimerSysncHeader = (RTC_TIMESYNC_HEADER_t*)pApsHeader->Apdu;
	
	//__memcpy(RTC_TimerSysncHeader->Asdu,pRTCTimeSyncRequestReq->Asdu,pRTCTimeSyncRequestReq->AsduLength);
	//RTC_TimerSysncHeader->AsduLength = pRTCTimeSyncRequestReq->AsduLength;
	CCONTB = get_phy_tick_time();

	// 填APS通用帧头
    pApsHeader->PacketPort 		= e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   		= e_RTC_TIME_SYNC;
    pApsHeader->PacketCtrlWord  = APS_PACKET_CTRLWORD;

     //填设置校时报文头
    RTC_TimerSysncHeader->ProtocolVer  = PROTOCOL_VERSION_NUM;
    RTC_TimerSysncHeader->HeaderLen	   = sizeof(RTC_TIMESYNC_HEADER_t);
    RTC_TimerSysncHeader->Reserve1 = 0;

    __memcpy(RTC_TimerSysncHeader->RTCClock,pRTCTimeSyncRequestReq->RTCClock,6);
    RTC_TimerSysncHeader->NTBtime   	   = CCONTB;
    printf_s("RTC_TimerSysncHeader->RTCClock\n");
	dump_buf(RTC_TimerSysncHeader->RTCClock,6);
	//dump_printfs((U8*)RTC_TimerSysncHeader,sizeof(RTC_TIMESYNC_HEADER_t));
    ApsPostPacketReq(post_work, msdu_length, 0, pRTCTimeSyncRequestReq->DstMacAddr, 
                         pRTCTimeSyncRequestReq->SrcMacAddr, pRTCTimeSyncRequestReq->SendType, (pApsHeader->PacketID << 16) + (++ApsHandle), TIME_CALIBRATE_LID);
	
}

//-------------------------------------------------------------------------------------------------
// 函数名：void ApsSlaveRegStartRequest(SLAVE_REGISTER_START_REQ_t *pSlaveRegStartReq)
// 
// 功能描述：
//     本函数由CCO用于发送从节点主动注册开始报文，此报文为广播报文
// 参数：
//
//     SLAVE_REGISTER_START_REQ_t *pSlaveRegStartReq   从节点主动注册开始注册请求数据结构指针，参见参数结构设计
// 
//
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
void ApsSlaveRegStartRequest(SLAVE_REGISTER_START_REQ_t *pSlaveRegStartReq)
{
    MSDU_DATA_REQ_t            *pMsduDataReq = NULL;
    APDU_HEADER_t              *pApsHeader = NULL;
    REGISTER_START_HEADER_t    *pRegisterHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t)+sizeof(REGISTER_START_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
	pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRegisterHeader = (REGISTER_START_HEADER_t*)pApsHeader->Apdu;
    
	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_SLAVE_REGISTER_START;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;
    

    // 填启动从节点注册报文头
    pRegisterHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pRegisterHeader->HeaderLen  = sizeof(REGISTER_START_HEADER_t);
    pRegisterHeader->ForcedResFlag = 0;
    pRegisterHeader->RegisterParameter = e_START_SLAVE_REGISTER;
    pRegisterHeader->Reserve = 0;
    pRegisterHeader->PacketSeq = pSlaveRegStartReq->PacketSeq;


    ApsPostPacketReq(post_work, msdu_length, 0, pSlaveRegStartReq->DstMacAddr, 
         pSlaveRegStartReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), SLAVE_REGISTER_LID);

    return;
}




//-------------------------------------------------------------------------------------------------
// 函数名：void ApsSlaveRegQueryRequest(REGISTER_QUERY_REQ_t      *pRegQueryReq)
// 
// 功能描述：
//     本函数由CCO用于发送注册查询请求报文
// 参数：
//
//     REGISTER_QUERY_REQ_t   *pRegQueryReq   从节点注册查询请求参数结构指针，参见数据结构定义
// 
//
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
void ApsSlaveRegQueryRequest(REGISTER_QUERY_REQ_t              *pRegQueryReq)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    REGISTER_QUERY_DWN_HEADER_t   *pRegQueryHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t)+sizeof(REGISTER_QUERY_DWN_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRegQueryHeader = (REGISTER_QUERY_DWN_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_SLAVE_REGISTER_QUERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    
    // 填查询从节点注册结果下行报文
    pRegQueryHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pRegQueryHeader->HeaderLen = sizeof(REGISTER_QUERY_DWN_HEADER_t);
    pRegQueryHeader->ForcedResFlag = pRegQueryReq->ForcedResFlag;
    pRegQueryHeader->RegisterParameter = pRegQueryReq->RegisterPrmt;
    pRegQueryHeader->Reserve = 0;
    pRegQueryHeader->PacketSeq = pRegQueryReq->PacketSeq;
    __memcpy(pRegQueryHeader->SrcMacAddr, pRegQueryReq->SrcMacAddr, MACADRDR_LENGTH);
    __memcpy(pRegQueryHeader->DstMacAddr, pRegQueryReq->DstMacAddr, MACADRDR_LENGTH);


    ApsPostPacketReq(post_work, msdu_length, 0, pRegQueryReq->DstMacAddr, 
         pRegQueryReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), SLAVE_REGISTER_LID);

    return;
}


//-------------------------------------------------------------------------------------------------
// 函数名：void ApsSlaveRegQueryResponse(REGISTER_QUERY_RSP_t *pRegQueryResp)
// 
// 功能描述：
//     本函数由STA用于发送从节点注册查询响应
// 参数：
//
//     REGISTER_QUERY_RSP_t *pRegQueryResp   从节点注册查询响应参数数据结构指针，参见数据结构定义
//
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
void ApsSlaveRegQueryResponse(REGISTER_QUERY_RSP_t *pRegQueryResp)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    REGISTER_QUERY_UP_HEADER_t    *pRegQueryHeader = NULL;
    U16   msdu_length = 0;
    
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(REGISTER_QUERY_UP_HEADER_t) + pRegQueryResp->MeterInfoLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRegQueryHeader = (REGISTER_QUERY_UP_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_SLAVE_REGISTER_QUERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;


    // 填查询从节点注册结果上行报文
    pRegQueryHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pRegQueryHeader->HeaderLen  = sizeof(REGISTER_QUERY_UP_HEADER_t);
    pRegQueryHeader->Status = pRegQueryResp->Status;
    pRegQueryHeader->RegisterParameter = pRegQueryResp->RegisterParameter;
    pRegQueryHeader->MeterCount = pRegQueryResp->MeterCount;
    pRegQueryHeader->DeviceType = pRegQueryResp->DeviceType;
    __memcpy(pRegQueryHeader->DeviceAddr, pRegQueryResp->DeviceAddr, 6);
    __memcpy(pRegQueryHeader->DeviceId, pRegQueryResp->DeviceId, 6);
    //pRegQueryHeader->PacketSeq = ApsRecvRegMeterSeq;
    pRegQueryHeader->PacketSeq = pRegQueryResp->PacketSeq;
    
    pRegQueryHeader->Reserve = 0;
    __memcpy(pRegQueryHeader->SrcMacAddr, pRegQueryResp->SrcMacAddr, 6);
    __memcpy(pRegQueryHeader->DstMacAddr, pRegQueryResp->DstMacAddr, 6);

    __memcpy(pRegQueryHeader->MeterInfo, pRegQueryResp->MeterInfo, pRegQueryResp->MeterInfoLen);

    ApsPostPacketReq(post_work, msdu_length, 0, pRegQueryResp->DstMacAddr, 
         pRegQueryResp->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), SLAVE_REGISTER_LID);

    return;
}


//-------------------------------------------------------------------------------------------------
// 函数名：void ApsSlaveRegStopRequest(SLAVE_REGISTER_STOP_REQ_t *pRegStopReq)
// 
// 功能描述：
//     本函数由CCO用于发送停止从节点主动注册报文，此报文为广播报文
// 参数：
//
//     SLAVE_REGISTER_STOP_REQ_t *pRegStopReq  从节点参数查询响应参数数据结构指针，参见数据结构定义
//
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
void ApsSlaveRegStopRequest(SLAVE_REGISTER_STOP_REQ_t *pRegStopReq)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    REGISTER_STOP_HEADER_t        *pRegStopHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(REGISTER_STOP_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRegStopHeader = (REGISTER_STOP_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_SLAVE_REGISTER_STOP;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;


    // 填停止从节点注册下行报文帧头
    pRegStopHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pRegStopHeader->HeaderLen  = sizeof(REGISTER_STOP_HEADER_t);
    pRegStopHeader->Reserve1    = 0;
    pRegStopHeader->Reserve2    = 0;
    pRegStopHeader->PacketSeq  = ApsSendRegMeterSeq;

    ApsPostPacketReq(post_work, msdu_length, 0, pRegStopReq->DstMacAddr, 
         pRegStopReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), SLAVE_REGISTER_LID);

    return;
}


void ApsTimeCalibrateRequest(TIME_CALIBRATE_REQ_t *pTimeCalibrateReq)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    TIME_CALIBRATE_HEADER_t        *pTimeCalHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(TIME_CALIBRATE_HEADER_t) + pTimeCalibrateReq->DataLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pTimeCalHeader = (TIME_CALIBRATE_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_TIME_CALIBRATE;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填校时下行报文头
    pTimeCalHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pTimeCalHeader->HeaderLen  = sizeof(TIME_CALIBRATE_HEADER_t);
    pTimeCalHeader->Reserve1   = 0;
    pTimeCalHeader->Reserve2   = 0;
    pTimeCalHeader->DataLength = pTimeCalibrateReq->DataLength;

    __memcpy(pTimeCalHeader->TimeData, pTimeCalibrateReq->TimeData, pTimeCalibrateReq->DataLength);
    
    ApsPostPacketReq(post_work, msdu_length, 0, pTimeCalibrateReq->DstMacAddr, 
         pTimeCalibrateReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), TIME_CALIBRATE_LID);

    return;
}


void ApsCommuTestRequest(COMMU_TEST_REQ_t *pCommuTestReq)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    APS_TESET_HEADER_t           *pCommuTestHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(APS_TESET_HEADER_t) + pCommuTestReq->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pCommuTestHeader = (APS_TESET_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_COMMU_TEST;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;


    // 填通讯测试命令下行报文头
    pCommuTestHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pCommuTestHeader->HeaderLen  = sizeof(APS_TESET_HEADER_t);
    pCommuTestHeader->TestModeCfg = pCommuTestReq->TestModeCfg;
    if(pCommuTestHeader->TestModeCfg == e_APP_PACKET_FORWARD_UART ||
                 pCommuTestHeader->TestModeCfg == e_APP_PACKET_FORWARD_CARRIER)
    {
        pCommuTestHeader->ProtocolType = pCommuTestReq->ProtocolType;
        pCommuTestHeader->DataLength = pCommuTestReq->AsduLength;
    }
    else
    {
        pCommuTestHeader->ProtocolType = 0;
        pCommuTestHeader->DataLength = pCommuTestReq->TimeOrCfgValue;
    }
    __memcpy(pCommuTestHeader->Asdu, pCommuTestReq->Asdu, pCommuTestReq->AsduLength);
	
    ApsPostPacketReq(post_work, msdu_length, 0, pCommuTestReq->DstMacAddr, 
         pCommuTestReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), COMMU_TEST_LID);

    return;
}


void ApsUpgradeStartRequest(UPGRADE_START_REQ_t   *pUpgradeStartReq)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    UPGRADE_START_DWN_HEADER_t           *pUpgradeStartHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(UPGRADE_START_DWN_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pUpgradeStartHeader = (UPGRADE_START_DWN_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_UPGRADE_START;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;


    // 填开始升级下行报文头
    pUpgradeStartHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pUpgradeStartHeader->HeaderLen  = sizeof(UPGRADE_START_DWN_HEADER_t);
    pUpgradeStartHeader->Reserve      = 0;
    pUpgradeStartHeader->UpgradeID  = pUpgradeStartReq->UpgradeID;
    pUpgradeStartHeader->UpgradeTimeout = pUpgradeStartReq->UpgradeTimeout;
    pUpgradeStartHeader->BlockSize  = pUpgradeStartReq->BlockSize;
    pUpgradeStartHeader->FileSize     = pUpgradeStartReq->FileSize;
    pUpgradeStartHeader->FileCrc      = pUpgradeStartReq->FileCrc;

    
    ApsPostPacketReq(post_work, msdu_length, 0, pUpgradeStartReq->DstMacAddr, 
             pUpgradeStartReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);

    return;
}


void ApsUpgradeStartResponse(UPGRADE_START_RSP_t *pUpgradeStartResp)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    UPGRADE_START_UP_HEADER_t           *pUpgradeStartHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(UPGRADE_START_UP_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pUpgradeStartHeader = (UPGRADE_START_UP_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_UPGRADE_START;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填开始升级上行报文头
    pUpgradeStartHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pUpgradeStartHeader->HeaderLen  = sizeof(UPGRADE_START_UP_HEADER_t);
    pUpgradeStartHeader->Reserve     = 0;
    pUpgradeStartHeader->StartResultCode = pUpgradeStartResp->StartResultCode;
    pUpgradeStartHeader->UpgradeID = pUpgradeStartResp->UpgradeID;
    
    ApsPostPacketReq(post_work, msdu_length, 0, pUpgradeStartResp->DstMacAddr, 
                 pUpgradeStartResp->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    
    return;
}


void ApsUpgradeStopRequest(UPGRADE_STOP_REQ_t *pUpgradeStopReq)
{
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    UPGRADE_STOP_HEADER_t     *pUpgradeStopHeader = NULL;
    U16   msdu_length = 0;
    
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(UPGRADE_STOP_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pUpgradeStopHeader = (UPGRADE_STOP_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_UPGRADE_STOP;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填停止升级下行报文格式
    pUpgradeStopHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pUpgradeStopHeader->HeaderLen  = sizeof(UPGRADE_STOP_HEADER_t);
    pUpgradeStopHeader->Reserve     = 0;
    pUpgradeStopHeader->UpgradeID = pUpgradeStopReq->UpgradeID;

    ApsPostPacketReq(post_work, msdu_length, 0, pUpgradeStopReq->DstMacAddr, 
                 pUpgradeStopReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);

    return;
}


void ApsFileTransRequest(FILE_TRANS_REQ_t *pFileTransReq)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;
    FILE_TRANS_HEADER_t     *pFileTransHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(FILE_TRANS_HEADER_t) + pFileTransReq->BlockSize;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pFileTransHeader = (FILE_TRANS_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_FILE_TRANS;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填文件传输下行报文头
    pFileTransHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pFileTransHeader->HeaderLen  = sizeof(FILE_TRANS_HEADER_t);
	
#if defined(STATIC_MASTER)
	if(pFileTransReq->IsCjq)
	{
    	pFileTransHeader->Reserve      = 0x01;
	}
	else
	{
		pFileTransHeader->Reserve      = 0x00;
	}
	//app_printf("IsCjqfile = %d \r\n" , IsCjqfile);
	//app_printf("pFileTransHeader->Reserve = %d \r\n" , pFileTransHeader->Reserve);
#elif defined(STATIC_NODE)
	pFileTransHeader->Reserve = pFileTransReq->IsCjq;
#endif

    pFileTransHeader->BlockSize    = pFileTransReq->BlockSize;
    pFileTransHeader->UpgradeID  = pFileTransReq->UpgradeID;
    pFileTransHeader->BlockSeq    = pFileTransReq->BlockSeq;

    __memcpy(pFileTransHeader->DataBlock, pFileTransReq->DataBlock, pFileTransReq->BlockSize);


    if(pFileTransReq->TransMode == e_FILE_TRANS_UNICAST)
    {
        ApsPostPacketReq(post_work, msdu_length, 0, pFileTransReq->DstMacAddr, 
                 pFileTransReq->SrcMacAddr, e_LOCAL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    }
    else if(pFileTransReq->TransMode == e_FULL_NET_BROADCAST)
    {
    	#if defined(STD_2016)
        ApsPostPacketReq(post_work, msdu_length, 0, pFileTransReq->DstMacAddr, 
                 pFileTransReq->SrcMacAddr, e_PROXY_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
		#elif defined(STD_GD)
        ApsPostPacketReq(post_work, msdu_length, 0, pFileTransReq->DstMacAddr, 
                 pFileTransReq->SrcMacAddr, e_LOCAL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
		#endif
    }
    else if(pFileTransReq->TransMode == e_UNICAST_TO_LOCAL_BROADCAST)
    {
        ApsPostPacketReq(post_work, msdu_length, 0, pFileTransReq->DstMacAddr, 
                 pFileTransReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    }

    return;
}


void ApsUpgradeStatusQueryRequest(UPGRADE_STATUS_QUERY_REQ_t *pUpgradeStatusReq)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;
    UPGRADE_STATUS_DWN_HEADER_t     *pUpgradeStatusHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(UPGRADE_STATUS_DWN_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pUpgradeStatusHeader = (UPGRADE_STATUS_DWN_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_UPGRADE_STATUS_QUERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;
    

    // 填查询站点升级状态下行报文头
    pUpgradeStatusHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pUpgradeStatusHeader->HeaderLen  = sizeof(UPGRADE_STATUS_DWN_HEADER_t);
    pUpgradeStatusHeader->Reserve     = 0;
    pUpgradeStatusHeader->QueryBlockCount = pUpgradeStatusReq->QueryBlockCount;
    pUpgradeStatusHeader->StartBockIndex   = pUpgradeStatusReq->StartBockIndex;
    pUpgradeStatusHeader->UpgradeID          = pUpgradeStatusReq->UpgradeID;

    ApsPostPacketReq(post_work, msdu_length, 0, pUpgradeStatusReq->DstMacAddr, 
                 pUpgradeStatusReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    
    return;
}


void ApsUpgradeStatusQueryResponse(UPGRADE_STATUS_QUERY_RSP_t *pUpgradeStatusResp)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;
    UPGRADE_STATUS_UP_HEADER_t     *pUpgradeStatusHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(UPGRADE_STATUS_UP_HEADER_t) + pUpgradeStatusResp->BitMapLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pUpgradeStatusHeader = (UPGRADE_STATUS_UP_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_UPGRADE_STATUS_QUERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填查询升级状态上行报文头
    pUpgradeStatusHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pUpgradeStatusHeader->HeaderLen  = sizeof(UPGRADE_STATUS_UP_HEADER_t);
    pUpgradeStatusHeader->UpgradeStatus = pUpgradeStatusResp->UpgradeStatus;
    pUpgradeStatusHeader->ValidBlockCount = pUpgradeStatusResp->ValidBlockCount;
    pUpgradeStatusHeader->StartBlockIndex = pUpgradeStatusResp->StartBlockIndex;
    pUpgradeStatusHeader->UpgradeID        = pUpgradeStatusResp->UpgradeID;

    __memcpy(pUpgradeStatusHeader->BlockInfoBitMap, pUpgradeStatusResp->BlockInfoBitMap, pUpgradeStatusResp->BitMapLen);
    app_printf(" pUpgradeStatusHeader->ValidBlockCount = %d\n", pUpgradeStatusHeader->ValidBlockCount);

    ApsPostPacketReq(post_work, msdu_length, 0, pUpgradeStatusResp->DstMacAddr, 
                 pUpgradeStatusResp->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);

    return;
}


void ApsUpgradePerformRequest(UPGRADE_PERFORM_REQ_t *pUpgradePerformReq)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;
    UPGRADE_PERFORM_HEADER_t     *pUpgradePerformHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(UPGRADE_PERFORM_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pUpgradePerformHeader = (UPGRADE_PERFORM_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_UPGRADE_PERFORM;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填执行升级下行报文头
    pUpgradePerformHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pUpgradePerformHeader->HeaderLen  = sizeof(UPGRADE_PERFORM_HEADER_t);
    pUpgradePerformHeader->Reserve      = 0;
    pUpgradePerformHeader->WaitResetTime = pUpgradePerformReq->WaitResetTime;
    pUpgradePerformHeader->UpgradeID       = pUpgradePerformReq->UpgradeID;
    pUpgradePerformHeader->TestRunTime    = pUpgradePerformReq->TestRunTime;

    
    ApsPostPacketReq(post_work, msdu_length, 0, pUpgradePerformReq->DstMacAddr, 
                     pUpgradePerformReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    
    return;
}


void ApsStationInfoQueryRequest(STATION_INFO_QUERY_REQ_t *pStationInfoReq)
{
    MSDU_DATA_REQ_t         *pMsduDataReq = NULL;
    APDU_HEADER_t           *pApsHeader = NULL;
    STATION_INFO_DWN_HEADER_t     *pStationInfoHeader = NULL;
    U16   msdu_length = 0;
    
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(STATION_INFO_DWN_HEADER_t) + pStationInfoReq->InfoListLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pStationInfoHeader = (STATION_INFO_DWN_HEADER_t*)pApsHeader->Apdu;

    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_STATION_INFO_QUERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填站点信息查询下行报文头
    pStationInfoHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pStationInfoHeader->HeaderLen  = sizeof(STATION_INFO_DWN_HEADER_t);
    pStationInfoHeader->Reserve      = 0;
    pStationInfoHeader->Res              = 0;
    pStationInfoHeader->InfoListNum = pStationInfoReq->InfoListNum;

    __memcpy(pStationInfoHeader->InfoList, pStationInfoReq->InfoList, pStationInfoReq->InfoListLen);

    ApsPostPacketReq(post_work, msdu_length, 0, pStationInfoReq->DstMacAddr, 
                     pStationInfoReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);
    
    return;
}


void ApsStationInfoQueryResponse(STATION_INFO_QUERY_RSP_t *pStationInfoResp)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    STATION_INFO_UP_HEADER_t     *pStationInfoHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(STATION_INFO_UP_HEADER_t) + pStationInfoResp->InfoDataLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
    
    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pStationInfoHeader = (STATION_INFO_UP_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_STATION_INFO_QUERY;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填站点信息查询上行报文头
    pStationInfoHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pStationInfoHeader->HeaderLen   = sizeof(STATION_INFO_UP_HEADER_t);
    pStationInfoHeader->Reserve     = 0;
    pStationInfoHeader->InfoListNum = pStationInfoResp->InfoListNum;
    pStationInfoHeader->UpgradeID   = pStationInfoResp->UpgradeID;

    __memcpy(pStationInfoHeader->InfoData, pStationInfoResp->InfoData, pStationInfoResp->InfoDataLen);

    ApsPostPacketReq(post_work, msdu_length, 0, pStationInfoResp->DstMacAddr, 
                     pStationInfoResp->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), UPGRADE_LID);

    return;
}


void ApsEventReportRequest(EVENT_REPORT_REQ_t      *pEventReportReq)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    EVENT_REPORT_HEADER_t     *pEventReportHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(EVENT_REPORT_HEADER_t) + pEventReportReq->EvenDataLen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pEventReportHeader = (EVENT_REPORT_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_EVENT_REPORT;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;
    
    __memcpy(pEventReportHeader->EventData, pEventReportReq->EventData, pEventReportReq->EvenDataLen);

    // 填事件报文头
    pEventReportHeader->ProtocolVer  = PROTOCOL_VERSION_NUM;
    pEventReportHeader->HeaderLen    = sizeof(EVENT_REPORT_HEADER_t);
    pEventReportHeader->Dir          = pEventReportReq->Direction;
    pEventReportHeader->Prm          = pEventReportReq->PrmFlag;
    pEventReportHeader->FunCode      = pEventReportReq->FunCode;
    pEventReportHeader->EventDataLen = pEventReportReq->EvenDataLen;
    if(pEventReportReq->PrmFlag == 1) // 启动站
    {
        pEventReportHeader->PacketSeq = pEventReportReq->PacketSeq; //++ApsEventSendPacketSeq;
    }
    else
    {
        pEventReportHeader->PacketSeq = pEventReportReq->PacketSeq; //ApsEventRecvPacketSeq;
    }
    __memcpy(pEventReportHeader->MeterAddr, pEventReportReq->MeterAddr, 6);
	
    
    ApsPostPacketReq(post_work, msdu_length, 0, pEventReportReq->DstMacAddr, 
                         pEventReportReq->SrcMacAddr, pEventReportReq->sendType, (pApsHeader->PacketID << 16) + (++ApsHandle), EVENT_REPORT_LID);

    return;
}


void ApsAckNakRequest(ACK_NAK_REQ_t *pAckNakReq)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    ACK_NAK_HEADER_t          *pAckNakHeader = NULL;
    U16   msdu_length = 0;
    
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(ACK_NAK_HEADER_t);
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pAckNakHeader   = (ACK_NAK_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_UPGRADE_PACKET_PORT;
    pApsHeader->PacketID   = e_ACK_NAK;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填确认否认报文头
    pAckNakHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pAckNakHeader->HeaderLen  = sizeof(ACK_NAK_HEADER_t);
    pAckNakHeader->Dir             = pAckNakReq->Direct;
    pAckNakHeader->AckOrNak   = pAckNakReq->AckOrNak;
    pAckNakHeader->Reserve     = 0;
    pAckNakHeader->PacketSeq  = ApsRecvPacketSeq;
    pAckNakHeader->OptionWord = pAckNakReq->ReportSeq;
    
    ApsPostPacketReq(post_work, msdu_length, 0, pAckNakReq->DstMacAddr, 
                             pAckNakReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), EVENT_REPORT_LID);

    return;
}


void ApsZoneAreaInfoReqeust(ZONEAREA_INFO_REQ_t *pZoneAreaInfoReq)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    ZONEAREA_INFO_HEADER_t          *pZoneAreaInfoHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(ZONEAREA_INFO_HEADER_t) + pZoneAreaInfoReq->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pZoneAreaInfoHeader   = (ZONEAREA_INFO_HEADER_t*)pApsHeader->Apdu;
    
    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_ZONEAREA_INFO_GATHER;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    __memcpy(pZoneAreaInfoHeader->Asdu, pZoneAreaInfoReq->Asdu, pZoneAreaInfoReq->AsduLength);

    // 填台区信息报文头
    pZoneAreaInfoHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pZoneAreaInfoHeader->HeaderLen  = sizeof(ZONEAREA_INFO_HEADER_t);
    pZoneAreaInfoHeader->Dir             = pZoneAreaInfoReq->Direct;
    pZoneAreaInfoHeader->GaterhType = pZoneAreaInfoReq->GatherType; 
    pZoneAreaInfoHeader->DataLength = pZoneAreaInfoReq->AsduLength;

	
    ApsPostPacketReq(post_work, msdu_length, 0, pZoneAreaInfoReq->DstMacAddr, 
                                 pZoneAreaInfoReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);

    return;
}


void ApsRdCtrlRequest(RD_CTRL_REQ_t *pRdCtrlReq)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    RDCTRL_HEADER_t           *pRdCtrlHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(RDCTRL_HEADER_t) + pRdCtrlReq->len;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRdCtrlHeader   = (RDCTRL_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_RDCTRL_CCO;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    __memcpy(pRdCtrlHeader->Asdu, pRdCtrlReq->Asdu, pRdCtrlReq->len);
    
    // 填台区信息报文头
    pRdCtrlHeader->protocol = pRdCtrlReq->protocol;
    pRdCtrlHeader->len = pRdCtrlReq->len;
    
    ApsPostPacketReq(post_work, msdu_length, pRdCtrlReq->dtei, pRdCtrlReq->DstMacAddr, 
                                     pRdCtrlReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), RDCTRL_CCO_LID);

    return;
}

void ApsRdCtrlToUartRequest(RD_CTRLT_TO_UART_REQ_t *pRdCtrlReq)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    RDCTRL_TO_UART_HEADER_t           *pRdCtrlToUartHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(RDCTRL_TO_UART_HEADER_t) + pRdCtrlReq->len;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRdCtrlToUartHeader   = (RDCTRL_TO_UART_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_RDCTRL_TO_UART;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    __memcpy(pRdCtrlToUartHeader->Asdu, pRdCtrlReq->Asdu, pRdCtrlReq->len);
    
    // 填台区信息报文头
    pRdCtrlToUartHeader->Protocol = pRdCtrlReq->protocol;
    pRdCtrlToUartHeader->Dir = pRdCtrlReq->dirflag;
    pRdCtrlToUartHeader->Baud = pRdCtrlReq->baud;
    pRdCtrlToUartHeader->Len = pRdCtrlReq->len;
    
    ApsPostPacketReq(post_work, msdu_length, pRdCtrlReq->dtei, pRdCtrlReq->DstMacAddr, 
                                     pRdCtrlReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), RDCTRL_TO_UART_LID);

    return;
}

void ApsQueryIdInfoReqResp(QUERY_IDINFO_REQ_RESP_t *pQueryIdInfoReq)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    QUERY_IDINFO_HEADER_t     *pQueryIdInfoHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(QUERY_IDINFO_HEADER_t) + pQueryIdInfoReq->AsduLength;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "ApsQueryIdInfoReqResp",MEM_TIME_OUT);

    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pQueryIdInfoHeader   = (QUERY_IDINFO_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_QUERY_ID_INFO;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    __memcpy(pQueryIdInfoHeader->Asdu, pQueryIdInfoReq->Asdu, pQueryIdInfoReq->AsduLength);

    // 填报文头信息
    pQueryIdInfoHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
    pQueryIdInfoHeader->HeaderLen  = sizeof(QUERY_IDINFO_HEADER_t);
    pQueryIdInfoHeader->Dir            = pQueryIdInfoReq->direct;
    pQueryIdInfoHeader->IdType      = pQueryIdInfoReq->IdType;

	
    pQueryIdInfoHeader->PacketSeq = pQueryIdInfoReq->PacketSeq;

    
    ApsPostPacketReq(post_work, msdu_length, pQueryIdInfoReq->destei, pQueryIdInfoReq->DstMacAddr, 
                                         pQueryIdInfoReq->SrcMacAddr, e_UNICAST_FREAM, (pApsHeader->PacketID << 16) + (++ApsHandle), READ_METER_LID);

    return;
}
/**************************
台区特征相关命令和信息联的 Macaddr MAC 地址， 是 48 比特的字段
台区特征采集类型 报文发送方向 报文类型  MAC地址内容
台区特征采集启动 CCO →STA             全网广播      CCO主节点地址
台区特征信息收集 CCO →STA             单播        STA MAC地址
台区特征信息告知 CCO →STA             全网广播      CCO主节点地址
台区特征信息告知 STA →CCO             单播        STA MAC地址
台区判别结果查询 CCO →STA             单播        STA MAC地址
台区判别结果信息 STA →CCO             单播        STA MAC地址
CMAC地址字节传输 均按照 “大端”序列 
***************************/
void ApsIndentificationRequset(INDENTIFICATION_REQ_t             *pIndentificationReq)
{
	//INDENTIFICATION_REQ_t         *pIndentificationReq = (INDENTIFICATION_REQ_t *)work->buf;
    MSDU_DATA_REQ_t               *pMsduDataReq = NULL;
    APDU_HEADER_t                 *pApsHeader = NULL;
    INDENTIFICATION_NEW_HEADER_t  *pIndentificationHeader = NULL;
    U16   msdu_length = 0;

    msdu_length = sizeof(APDU_HEADER_t) + sizeof(INDENTIFICATION_NEW_HEADER_t) + pIndentificationReq->payloadlen;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);

    pMsduDataReq             = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader               = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pIndentificationHeader   = (INDENTIFICATION_NEW_HEADER_t*)pApsHeader->Apdu;

    // 填APS通用帧头
    pApsHeader->PacketPort     = e_INDENTIFICATION_AREA_PORT;
    pApsHeader->PacketID       = e_INDENTIFICATION_AREA;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

    // 填台区信息报文头
    pIndentificationHeader->ProtocolVer    = PROTOCOL_VERSION_NUM;
	pIndentificationHeader->HeaderLen      = sizeof(INDENTIFICATION_HEADER_t);
	pIndentificationHeader->Directbit      = pIndentificationReq->Direct;
	pIndentificationHeader->Startbit       = pIndentificationReq->StartBit;
	pIndentificationHeader->Phase          = pIndentificationReq->Phase;
	pIndentificationHeader->PacketSeq      = pIndentificationReq->DatagramSeq;
	pIndentificationHeader->Featuretypes   = pIndentificationReq->Featuretypes;
	pIndentificationHeader->Collectiontype = pIndentificationReq->Collectiontype;
    
	#if defined(STATIC_MASTER)
    if(pIndentificationReq->sendtype == e_UNICAST_FREAM)
        __memcpy(pIndentificationHeader->Macaddr, pIndentificationReq->DstMacAddr, 6);
    else
        __memcpy(pIndentificationHeader->Macaddr, pIndentificationReq->SrcMacAddr, 6);
    #else
    __memcpy(pIndentificationHeader->Macaddr, pIndentificationReq->SrcMacAddr, 6);
    #endif
    

	__memcpy(pIndentificationHeader->Payload, pIndentificationReq->payload, pIndentificationReq->payloadlen);
    ApsPostPacketReq(post_work, msdu_length, 0, pIndentificationReq->DstMacAddr,  pIndentificationReq->SrcMacAddr, 
		                      pIndentificationReq->sendtype, (pApsHeader->PacketID << 16) + (++ApsHandle), INDENTIFICATION_LID);
}

void ApsSlaveLeaveIndRequst(LEAVE_IND_t *pData)
{
#if defined(STATIC_MASTER)
    work_t *work = zc_malloc_mem(sizeof(work_t) + sizeof(LEAVE_IND_t) + MAX_NET_MMDATA_LEN, "LEAVE", MEM_TIME_OUT);
    LEAVE_IND_t *pLeaveInd = (LEAVE_IND_t *)work->buf;
    work->work = ProcLeaveIndRequset;
	work->msgtype = LEAVEL_REQ;
    app_printf("pData->StaNum = %02x\n", pData->StaNum);

    if(pData->StaNum == 0xaa)   //所有节点离线
    {
       U16 leaveNum = 0;
       U16 i;
       pLeaveInd->StaNum = ((APP_GETDEVNUM() > MAXLEVELNODE )? MAXLEVELNODE : APP_GETDEVNUM());

       if(pLeaveInd->StaNum > 0)
       {
           for(i=0;i<MAX_WHITELIST_NUM;i++)
           {
               if(DeviceTEIList[i].NodeTEI !=0xfff)
               {

                   __memcpy(&pLeaveInd->pMac[leaveNum*6],DeviceTEIList[i].MacAddr, 6);
                   leaveNum++;
                   if(leaveNum == pLeaveInd->StaNum)
                   {
                       break;
                   }
               }
           }
       }

    }
    else
    {
        if(pData->StaNum > MAXLEVELNODE)
        {
            pLeaveInd->StaNum = MAXLEVELNODE;
        }
        else
        {
            pLeaveInd->StaNum = pData->StaNum;
        }

        __memcpy(pLeaveInd->pMac, pData->pMac, pLeaveInd->StaNum * MACADRDR_LENGTH);
        pLeaveInd->DelType = pData->DelType;
    }

    if(pLeaveInd->StaNum  > 0)
    {
        //app_printf("leaveInd StaNum = %d\n", pLeaveInd->StaNum);
        //dump_buf(pLeaveInd->pMac, pLeaveInd->StaNum * MACADRDR_LENGTH);
        pLeaveInd->delayTime = DELAY_LEAVE_TIME;
		
        post_datalink_task_work(work);
    }
    else
    {
        app_printf("pLeaveInd->StaNum = %d, freem Data \n", pLeaveInd->StaNum);
        zc_free_mem(work);
    }
#endif
}




