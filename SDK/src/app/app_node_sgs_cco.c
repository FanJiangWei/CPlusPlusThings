#include"app_node_sgs_ex.h"
#include"DataLinkInterface.h"
#include"aps_layer.h"
#include"mem_zc.h"
#include"app_common.h"


#if defined(STATIC_MASTER)

void cco_node_sgs_req(U8 *data)
{
    MSDU_DATA_REQ_t           *pMsduDataReq = NULL;
    APDU_HEADER_t             *pApsHeader = NULL;
    NODE_SGS_HEADER_t         *pNodeSGSHeader = NULL;
    U16   msdu_length = 0;
	
    msdu_length = sizeof(APDU_HEADER_t) + sizeof(NODE_SGS_HEADER_t) + 1;
    work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t) + msdu_length, "MsduDataRequest",MEM_TIME_OUT);
	
	U8  BCDstAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
   	
    pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pNodeSGSHeader  = (NODE_SGS_HEADER_t*)pApsHeader->Apdu;
		
    // 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_NODE_SGS;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;
    
    __memcpy(pNodeSGSHeader->Data, data, 1);
	
    // 填事件报文头
    pNodeSGSHeader->ProtocolVer  = PROTOCOL_VERSION_NUM;
    pNodeSGSHeader->HeaderLen    = sizeof(NODE_SGS_HEADER_t);
	pNodeSGSHeader->Res = 0;
	pNodeSGSHeader->PacketSeq = ++ApsEventSendPacketSeq;
	
	ApsPostPacketReq(post_work, msdu_length, 0, BCDstAddr,
                         FlashInfo_t.ucJZQMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), EVENT_REPORT_LID);

}



#endif



