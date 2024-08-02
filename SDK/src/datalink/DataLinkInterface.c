
#include "DataLinkInterface.h"
#include "phy_sal.h"
#include "DataLinkGlobal.h"
#include "printf_zc.h"
#include "app.h"
#include "dl_mgm_msg.h"
#include "DatalinkTask.h"
#include "DataLinkGlobal.h"
#include "Beacon.h"
#include "crc.h"
#include "test.h"
#include "plc.h"
#include "plc_cnst.h"
#include "Datalinkdebug.h"
#include "app_rdctrl.h"
#include "csma.h"
#include "wl_csma.h"
#include "wl_cnst.h"
#include "ZCsystem.h"
#include "Scanbandmange.h"
#include "app_dltpro.h"
#include "meter.h"
#include "app_event_report_sta.h"

#if defined(STD_DUAL)
#include "wlc.h"
#include "wphy_isr.h"
#endif

//////////////////////////////////////////////////APP�ӿ�,MSDU���/////////////////////////////////////



//datalink��������Ϣ��appʱ����ҪAPP�ڳ�ʼ����ʱ��ע�����º�����
void (*pProcessMsduDataInd)(work_t *work) = NULL;
void (*pProcessMsduCfm)(work_t *work) = NULL;

void (*pNetStatusChangeInd)(work_t *work) = NULL; //�ڵ���������·���״̬�����仯ʱ��֪ͨAPP

void (*pPhasePositionCfmProc)(work_t *work) = NULL;   // ������·����λʶ�����ȷ��


//COMFIRM,mac��ֱ�Ӻ�������
static void  MsduConfirmToApp(U16 ScrTEI,U32 Handle, U8 Status)
{
    MSDU_DATA_CFM_t  *pMsduCfm_t=NULL;

    if(ScrTEI == GetTEI())//�����������ݷ����߲Ż���APS��
    {
        work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_CFM_t),"NetDataCfm",MEM_TIME_OUT);
        pMsduCfm_t = (MSDU_DATA_CFM_t*)work->buf;
		
        pMsduCfm_t->MsduHandle = Handle;
        pMsduCfm_t->Status = Status;

        work->work = pProcessMsduCfm;
		work->msgtype =DALKCFM;
        post_app_work(work);
    }

    return;

}


#if defined(STATIC_MASTER)

//REQUEST  

void ProcPhaseDistinguishRequest(work_t *work)
{
    PHASE_POSITION_REQ_t  *pPhasePositionReq = (PHASE_POSITION_REQ_t*)work->buf;
    U16  dstTei = INVALID;

    dstTei = SearchTEIDeviceTEIList(pPhasePositionReq->addr);
    
    if(dstTei != INVALID)
    {
        SendMMeZeroCrossNTBCollectInd(dstTei, 0, TRUE);
    }
	
}

#endif
extern ostimer_t 	g_WaitReplyTimer;
/**
 * @brief ProcSigMsduDataRequest        ����֡Ӧ�ñ���
 * @param work
 */
void ProcSigMsduDataRequest(work_t *work)
{
    MSDU_DATA_REQ_t	  *pMsduRequest_t = (MSDU_DATA_REQ_t*)work->buf;
    U32 			   Crc32Data = 0;
    U16 			   macLen;
    MAC_RF_HEADER_t    *pMacHeader = NULL;
    U8				   *pData;

    net_printf("DstMac: ");
    dump_level_buf(DEBUG_MDL, pMsduRequest_t->DstAddress, MACADRDR_LENGTH);

    macLen = sizeof(MAC_RF_HEADER_t) + pMsduRequest_t->MsduLength + 4;
    pMacHeader = (MAC_RF_HEADER_t *)zc_malloc_mem(macLen, "SigMac", MEM_TIME_OUT);
    pData = pMacHeader->Msdu;

    __memcpy(pData, pMsduRequest_t->Msdu, pMsduRequest_t->MsduLength);
    pMacHeader->MsduLen = pMsduRequest_t->MsduLength;

    if(pMsduRequest_t->MsduLength ==0 || pMsduRequest_t->MsduLength>517)
    {
        net_printf("ProcMsduDataRequest MsduLen err:%d,Handle=%04x",pMsduRequest_t->MsduLength,pMsduRequest_t->Handle);
        return;
    }
    crc_digest(pData, pMsduRequest_t->MsduLength, (CRC_32 | CRC_SW), &Crc32Data);
    __memcpy(&pData[pMsduRequest_t->MsduLength], (U8 *)&Crc32Data, CRCLENGTH);
    net_printf("SigMac Handle: 0x%04x\n", pMsduRequest_t->Handle);

    //�齨����֡mac֡ͷ
    CreatMMsgMacHeaderForSig(pMacHeader, e_RF_APS_FREAME, pMacHeader->MsduLen);


    MacDataRequstRf((MAC_PUBLIC_HEADER_t *)pMacHeader, macLen, pMsduRequest_t->dtei,
                  pMsduRequest_t->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_RF_APS_FREAME, 0, 0,e_MainRoute);

}


void ProcMsduDataRequest(work_t *work)
{
    MSDU_DATA_REQ_t	  *pMsduRequest_t = (MSDU_DATA_REQ_t*)work->buf;
    U8				   RouteMode = e_MainRoute;
    U32 			   Crc32Data = 0;
    U16 			   macLen;
    U16 			   nextTEI;
    MAC_PUBLIC_HEADER_t 	 *pMacPublicHeader = NULL;
    U8						 *pData;
    U8                      LinkType = 0;

	
    //net_printf("DstMac:");
    //dump_buf(pMsduRequest_t->DstAddress, MACADRDR_LENGTH);
    if(pMsduRequest_t->MacAddrUseFlag == e_USE_MACADDR)
    {
        macLen = sizeof(MAC_LONGDATA_HEADER) + pMsduRequest_t->MsduLength + 4;
        pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(macLen, "MsduDataReq", MEM_TIME_OUT);
        pData = pMacPublicHeader->Msdu;
	
        __memcpy(pData, pMsduRequest_t->SrcAddress, MACADRDR_LENGTH);
        pData += MACADRDR_LENGTH;
        __memcpy(pData, pMsduRequest_t->DstAddress, MACADRDR_LENGTH);
        pData += MACADRDR_LENGTH;
        dump_buf(pMsduRequest_t->DstAddress, MACADRDR_LENGTH);
        dump_buf(pMsduRequest_t->SrcAddress, MACADRDR_LENGTH);
    }
    else
    {
        macLen = sizeof(MAC_PUBLIC_HEADER_t) + pMsduRequest_t->MsduLength + 4;
        pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(macLen, "MsduDataReq", MEM_TIME_OUT);
	
        pData = pMacPublicHeader->Msdu;
    }
	
    __memcpy(pData, pMsduRequest_t->Msdu, pMsduRequest_t->MsduLength);
	
    if(pMsduRequest_t->MsduLength ==0 || pMsduRequest_t->MsduLength>517)
    {
        debug_printf(&dty,DEBUG_MDL,"ProcMsduDR Len err:%d,Hdl=%04x",pMsduRequest_t->MsduLength,pMsduRequest_t->Handle);
    }
	 mg_update_type(STEP_1, work->record_index); 
    crc_digest(pData, pMsduRequest_t->MsduLength, (CRC_32 | CRC_SW), &Crc32Data);
    __memcpy(&pData[pMsduRequest_t->MsduLength], (U8 *)&Crc32Data, CRCLENGTH);
	//pMsduRequest_t->MsduLength += CRCLENGTH;

    //net_printf("pMReHd : 0x%04x\n",pMsduRequest_t->Handle);
	
    //�齨MAC֡ͷ
    pMacPublicHeader->MacProtocolVersion = MAC_PROTOCOL_VER;
    pMacPublicHeader->ScrTEI = GetTEI();
    pMacPublicHeader->SendType = pMsduRequest_t->SendType;
		

	pMacPublicHeader->MaxResendTime = 
		READ_METER_LID == pMsduRequest_t->LID ?  READMETER_UNICAST_MAX_RETRIES: NWK_UNICAST_MAX_RETRIES;
		
    pMacPublicHeader->MsduSeq = ++g_MsduSequenceNumber;
    /*pMacPublicHeader->MsduLength_l = pMsduRequest_t->MsduLength-CRCLENGTH;
    pMacPublicHeader->MsduLength_h = (pMsduRequest_t->MsduLength-CRCLENGTH) >> 8;*/
    pMacPublicHeader->MsduLength_l = pMsduRequest_t->MsduLength;
    pMacPublicHeader->MsduLength_h = (pMsduRequest_t->MsduLength >> 8) & 0x07;
    pMacPublicHeader->RestTims = nnib.Resettimes;
    pMacPublicHeader->ProxyRouteFlag = e_UNUSE_PROXYROUTE;//e_USE_PROXYROUTE;
    pMacPublicHeader->Radius = NET_RELAY_LEVEL_MAX;
    pMacPublicHeader->RemainRadius = NET_RELAY_LEVEL_MAX;
		
    // pMacPublicHeader->BroadcastDirection = e_CCO_STA;
    pMacPublicHeader->RouteRepairFlag = e_NO_REPAIR;
    pMacPublicHeader->MacAddrUseFlag = pMsduRequest_t->MacAddrUseFlag;
    pMacPublicHeader->FormationSeq = nnib.FormationSeqNumber;
    pMacPublicHeader->MsduType = e_APS_FREAME;
    pMacPublicHeader->Reserve =  0;
    pMacPublicHeader->Reserve1 = 0;
    pMacPublicHeader->Reserve2 = 0;
    pMacPublicHeader->Reserve3 = 0;
    pMacPublicHeader->Reserve4 = 0;
	
	
    if(pMsduRequest_t->SendType != e_UNICAST_FREAM)
    {
        pMacPublicHeader->DestTEI = BROADCAST_TEI;
        nextTEI = BROADCAST_TEI;
        RouteMode = e_Broadcast;
        //pMacPublicHeader->BroadcastDirection = e_CCO_STA;
        #if defined(STATIC_MASTER)
		pMacPublicHeader->BroadcastDirection = e_CCO_STA;
        #else
        pMacPublicHeader->BroadcastDirection = e_STA_CCO;
        #endif
        //����ʵ�ʹ㲥���ͽ��з���
		//pMacPublicHeader->SendType = e_FULL_BROADCAST_FREAM_NOACK;
    }
    else
    {
    	if(getHplcTestMode() == RD_CTRL)
    	{

			if(rd_ctrl_info.dtei == 0 || rd_ctrl_info.dtei == BROADCAST_TEI)
			{    		
				pMacPublicHeader->DestTEI = rd_ctrl_info.dtei==0?BROADCAST_TEI:rd_ctrl_info.dtei ;
				nextTEI =  rd_ctrl_info.dtei == 0? BROADCAST_TEI:rd_ctrl_info.dtei ;
	            net_printf("set Broadcast sendtype\n");
				pMacPublicHeader->SendType = e_FULL_BROADCAST_FREAM_NOACK;
			}
			else
			{
				pMacPublicHeader->DestTEI = rd_ctrl_info.dtei;
				nextTEI = rd_ctrl_info.dtei;
				pMacPublicHeader->SendType = e_UNICAST_FREAM;
			}
    	}
		else
		{
#if defined(STATIC_MASTER)
	        pMacPublicHeader->DestTEI = pMsduRequest_t->dtei==CTRL_TEI_GW?CTRL_TEI_GW:
	                                    pMsduRequest_t->dtei==CTRL_TEI_BJ?CTRL_TEI_BJ:SearchTEIDeviceTEIList(pMsduRequest_t->DstAddress);
	        if(pMacPublicHeader->DestTEI == BROADCAST_TEI)
	        {
	            net_printf("send fail,not found STA int net,you can try Broadcast sendtype\n");
	            zc_free_mem(pMacPublicHeader);
	            return;
	        }
#else
        	pMacPublicHeader->DestTEI = pMsduRequest_t->dtei==CTRL_TEI_GW? CTRL_TEI_GW:
                                    pMsduRequest_t->dtei==CTRL_TEI_BJ? CTRL_TEI_BJ:CCO_TEI;
#endif
            if(is_rd_ctrl_tei(pMacPublicHeader->DestTEI))
            {
                nextTEI = pMacPublicHeader->DestTEI;
            }
#if defined(STATIC_NODE)
            else if(pMacPublicHeader->DestTEI == CCO_TEI && GetNodeState() == e_NODE_ON_LINE)
            {
                nextTEI = GetProxy();
            }
#endif
            else
            {
                nextTEI = SearchNextHopAddrFromRouteTable(pMacPublicHeader->DestTEI, ACTIVE_ROUTE);
            }

            RouteMode = e_MainRoute;
            mg_update_type(STEP_2, work->record_index);
        }
        #if defined(STATIC_MASTER)
        if(nextTEI == INVALID) //CCO,Ŀǰ��·��ʱ����㲥����
        {
            pMacPublicHeader->DestTEI = BROADCAST_TEI;
            nextTEI = BROADCAST_TEI;
            pMacPublicHeader->SendType = e_FULL_BROADCAST_FREAM_NOACK;
            pMacPublicHeader->BroadcastDirection = e_CCO_STA;
            pMacPublicHeader->MaxResendTime = 1;
            RouteMode = e_Broadcast;
        }
        #elif defined(STATIC_NODE)
        if(nextTEI == INVALID)//����,���ұ�������·��
        {
            RouteMode = e_FristBackupRoute;
            nextTEI = SearchBestBackupProxyNode(0);
        }

        //app_printf("nextTEI -> 0x%04x\n",nextTEI);
        if(nextTEI == INVALID) //�ޱ���·��ʱ
        {
            if(IsInNeighborDiscoveryTableEntry(pMacPublicHeader->DestTEI))//���Ŀ��ڵ����ھӱ���,ֱ�ӷ���
            {
                nextTEI = pMacPublicHeader->DestTEI;
                RouteMode = e_FristBackupRoute;
            }
            else
            {
                if(getHplcTestMode() ==  RD_CTRL)
                {

                }
                else
                {
                    if(nnib.AODVRouteUse == FALSE) //������ʹ��AODVʱ
                    {
                        MsduConfirmToApp(pMacPublicHeader->ScrTEI, pMsduRequest_t->Handle, e_ROUTE_ERR);
                        net_printf("WARING:e_ROUTE_ERR\n");
                        zc_free_mem(pMacPublicHeader);
                        return;
                    }

                    RouteMode = e_AODVRoute;
                    nextTEI = SearchNextHopFromAodvRouteTable(pMacPublicHeader->DestTEI, ACTIVE_ROUTE);
                    if(nextTEI ==INVALID) //AODV��·�ɱ�����Ȼδ�ҵ�
                    {

                        if(TMR_STOPPED == timer_query(&g_WaitReplyTimer))
                        {
                            net_printf("--Start AODV --\n");

                            g_AodvMsduLID = pMsduRequest_t->LID;

                            //reacordmeter(pMacPublicHeader->MsduSeq,1,0,pMacPublicHeader->DestTEI,0);
                            SendMMeRouteReq(pMacPublicHeader->DestTEI, GetTEI(), pMsduRequest_t->Handle);
                            zc_free_mem(pMacPublicHeader);
                            return ;
                        }
                        else
                        {
                            MsduConfirmToApp(pMacPublicHeader->ScrTEI, pMsduRequest_t->Handle, e_ROUTE_REPAIR);

                            zc_free_mem(pMacPublicHeader);
                            return;
                        }
                    }
                }
            }
        }
	
#endif

    }
	if(getHplcTestMode() ==  RD_CTRL)
	{
		pMacPublicHeader->DestTEI = rd_ctrl_info.dtei;
		if(rd_ctrl_info.linktype == e_HPLC_Link)
		MacDataRequst(pMacPublicHeader, macLen, rd_ctrl_info.dtei,
					  pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode,0);
		else
		MacDataRequstRf(pMacPublicHeader, macLen, rd_ctrl_info.dtei,
						  pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode);
	}
	else
	{
	    if(pMacPublicHeader->DestTEI == CTRL_TEI_GW || pMsduRequest_t->dtei == CTRL_TEI_BJ)
	    {
	        LinkType = rd_ctrl_info.linktype;
	    }
	    else
	    {
//            LinkType = NwkRoutingTable[nextTEI-1].LinkType;
            LinkType = getRouteTableLinkType(nextTEI);
	    }
	    net_printf("nextTei : 0x%04x,linktp : %d\n",nextTEI,LinkType);
		mg_update_type(STEP_3, work->record_index);
#if 0
        if((ifHasRoteLkTp(e_HPLC_Link) && IS_BROADCAST_TEI(nextTEI)) || LinkType == e_HPLC_Link)
	    {
	        net_printf("apsdata send by hplc\n");
	        MacDataRequst(pMacPublicHeader, macLen, nextTEI,
	                      pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode,0);
	    }

        if((ifHasRoteLkTp(e_RF_Link) && IS_BROADCAST_TEI(nextTEI)) || LinkType == e_RF_Link)
	    {
	        net_printf("apsdata send by hrf\n");
	        MacDataRequstRf(pMacPublicHeader, macLen, nextTEI,
	                      pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode);
	    }
#else
        if(IS_BROADCAST_TEI(nextTEI))
        {
            if(pMacPublicHeader->SendType == e_LOCAL_BROADCAST_FREAM_NOACK)
            {
                pMacPublicHeader->MaxResendTime = 3;
            }
            if(ifHasRoteLkTp(e_HPLC_Link))
            {
                MacDataRequst(pMacPublicHeader, macLen, nextTEI,
                              pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode,0);
            }

            if(ifHasRoteLkTp(e_RF_Link))
            {
                MacDataRequstRf(pMacPublicHeader, macLen, nextTEI,
                              pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode);
            }
        }
        else
        {
            if(LinkType == e_HPLC_Link)
            {
                if(TX_DATA_LINK.nr >= TX_DATA_LINK.thr && (NwkRoutingTable[nextTEI-1].ActiveLk & (1 << e_RF_Link)))  //���������������ھ�
                {
                    MacDataRequstRf(pMacPublicHeader, macLen, nextTEI,
                                  pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode);
                }
                else
                {
                    MacDataRequst(pMacPublicHeader, macLen, nextTEI,
                                  pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode,0);
                }

            }
            else if(LinkType == e_RF_Link)
            {
                if(WL_TX_DATA_LINK.nr >= WL_TX_DATA_LINK.thr && (NwkRoutingTable[nextTEI-1].ActiveLk & (1 << e_HPLC_Link)))  //���������������ھ�
                {
                    MacDataRequst(pMacPublicHeader, macLen, nextTEI,
                                  pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode,0);
                }
                else
                {
                    MacDataRequstRf(pMacPublicHeader, macLen, nextTEI,
                                  pMacPublicHeader->SendType, pMsduRequest_t->Handle, pMsduRequest_t->LID, e_APS_FREAME, 0, 0,RouteMode);
                }
            }
        }
#endif
	}


    net_printf("Dst=%04x next=%04x type = %d ActiveLk = %d\n", pMacPublicHeader->DestTEI,nextTEI,LinkType,NwkRoutingTable[nextTEI-1].ActiveLk);
	//						   ,RouteMode==e_MainRoute?"MainRoute":
	//							RouteMode==e_AODVRoute?"AODVRoute":
	//							RouteMode==e_FristBackupRoute?"FristBK":
	//							RouteMode==e_SecondBproute?"SecondBK":
	//							RouteMode==e_Broadcast?"Broadcast":"UK");
    mg_update_type(STEP_4, work->record_index); 
    zc_free_mem(pMacPublicHeader);
	mg_update_type(STEP_5, work->record_index); 

}





//�Ƿ�Ӧ�÷��ڴ���MPDU�ĵط��ж��Ƿ�ת��
//ת��������1��ȫ���㲥������㲥��2Ŀ���ַ�����Լ�
//����APP������Ŀ���ַΪFFF���������TEI
#if defined(STATIC_NODE)

static void RelayMacData(U8	*pMacIndication, U16 len, mbuf_t *buf)
{
    MAC_PUBLIC_HEADER_t	*pMacHeader  = NULL;

    U16        DelayTime = 0;
    U16        NextTei;
    U8         RouteMode;

    
    if(nnib.NodeState != e_NODE_ON_LINE )
    {
        return;
    }
    if(buf->snid !=GetSNID()) //���˷Ǳ������ת������
	{
		return ;
	}

//    pMacHeader = (MAC_PUBLIC_HEADER_t *)pMacIndication->Mpdu;
    pMacHeader = (MAC_PUBLIC_HEADER_t *)pMacIndication;


    net_printf("------RemainRadius= %d---\n",pMacHeader->RemainRadius);
    if( pMacHeader->SendType== e_PROXY_BROADCAST_FREAM_NOACK
        || pMacHeader->SendType== e_FULL_BROADCAST_FREAM_NOACK) //���Ŀ��Ϊ�㲥

    {
        U16 randomNum = 0;
        randomNum = 0;

//        if(pMacHeader->ScrTEI == 0 && pMacHeader->SendType== e_FULL_BROADCAST_FREAM_NOACK)
//        {
//            return;
//        }
        randomNum = GetNeighborCount();
        randomNum = rand()%((randomNum+1)*8);
        net_printf("randomNum=%d,nowtime=%d\n",randomNum,PHY_TICK2MS(get_phy_tick_time()) );
        
        DelayTime = randomNum;
        NextTei = BROADCAST_TEI;
        RouteMode = e_Broadcast;
    }
    else
    {
        RouteMode = e_MainRoute;
        DelayTime = 0;
        if(pMacHeader->DestTEI == CCO_TEI)
        {
            NextTei = GetProxy();
        }
        else
        {
            NextTei =  SearchNextHopAddrFromRouteTable(pMacHeader->DestTEI, ACTIVE_ROUTE);
        }
        
        if(NextTei == INVALID && pMacHeader->DestTEI == CCO_TEI) //��·��ʧ��ʱ����������У��������б�������Ѱ��
        {
            RouteMode = e_FristBackupRoute;
            NextTei = SearchBestBackupProxyNode(0);  // �����ҵ�����·�ɺ�����IF�Ͳ��ڽ���
        }

        if(NextTei ==INVALID) //��·����Ϊ�ҵ�
        {
            if(IsInNeighborDiscoveryTableEntry(pMacHeader->DestTEI))//���Ŀ��ڵ����ھӱ���,ֱ�ӷ���
            {
                NextTei = pMacHeader->DestTEI;
                RouteMode = e_FristBackupRoute;
            }
            else
            {
                if(nnib.AODVRouteUse ==FALSE) //������ʹ��AODVʱ
                {
                    return;
                }
                
                RouteMode = e_AODVRoute;
                NextTei = SearchNextHopFromAodvRouteTable(pMacHeader->DestTEI, ACTIVE_ROUTE);
                if(NextTei == INVALID) //AODV��·�ɱ�����Ȼδ�ҵ�
                {
                    //����AODV
                    if(TMR_STOPPED == timer_query(&g_WaitReplyTimer))
                    {
                        //����Ҫ���͵�����
                        __memcpy(AodvBuff.payload, pMacIndication, len);
                        AodvBuff.payloadLen = len;
                        g_AodvMsduLID = buf->lid;
							 
                        //reacordmeter(pMacHeader->MsduSeq,1,1,pMacHeader->DestTEI,0);
                        SendMMeRouteReq(pMacHeader->DestTEI, pMacHeader->ScrTEI, 0);
                    }

                    return;
                }
            }
        }
    }

    U8 linkType = getRouteTableLinkType(NextTei);

    net_printf("------RelayNwkData Sucess---nextTei<%03x>,RouteMode = %d, linkType:%d\n",NextTei, RouteMode,linkType);

    //if(buf->LinkType == e_HPLC_Link)


#if 0
    if((ifHasRoteLkTp(e_HPLC_Link) && IS_BROADCAST_TEI(NextTei)) || linkType == e_HPLC_Link)
    {
        net_printf("ReTrans:hplc\n");
        MacDataRequst(pMacHeader, len, NextTei,
                      pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode,0);
    }
    if((ifHasRoteLkTp(e_RF_Link) && IS_BROADCAST_TEI(NextTei)) || linkType == e_RF_Link)
    {
        net_printf("ReTrans:hrf\n");
        MacDataRequstRf(pMacHeader, len, NextTei,
                        pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode);
    }
#else
    if(IS_BROADCAST_TEI(NextTei))
    {
        if(ifHasRoteLkTp(e_HPLC_Link))
        {
            MacDataRequst(pMacHeader, len, NextTei,
                          pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode,0);
        }
        if(ifHasRoteLkTp(e_RF_Link))
        {
            MacDataRequstRf(pMacHeader, len, NextTei,
                            pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode);
        }
    }
    else
    {
        if(linkType == e_HPLC_Link)
        {
            if(TX_DATA_LINK.nr >= TX_DATA_LINK.thr && (NwkRoutingTable[NextTei-1].ActiveLk & (1 << e_RF_Link)))  //���������������ھ�
            {
                MacDataRequstRf(pMacHeader, len, NextTei,
                                pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode);
            }
            else
            {
                MacDataRequst(pMacHeader, len, NextTei,
                              pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode,0);
            }

        }
        else if(linkType == e_RF_Link)
        {
            if(WL_TX_DATA_LINK.nr >= WL_TX_DATA_LINK.thr && (NwkRoutingTable[NextTei-1].ActiveLk & (1 << e_HPLC_Link)))  //���������������ھ�
            {
                MacDataRequst(pMacHeader, len, NextTei,
                              pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode,0);
            }
            else
            {
                MacDataRequstRf(pMacHeader, len, NextTei,
                                pMacHeader->SendType, 0, buf->lid, e_APS_FREAME, 0, DelayTime, RouteMode);
            }
        }

    }
#endif

    return;
}
#endif
//INDCATION ,��mac��ֱ�Ӻ�������
/**
 * @brief SimMacMsduDataIndToApp        ����֡Ӧ�ò����ݴ���
 * @param buf
 * @param pMacIndication
 * @param len
 */
static void SimMacMsduDataIndToApp(mbuf_t *buf, U8 *pMacIndication, U16 len)
{
    MAC_RF_HEADER_t *pMacHeader = (MAC_RF_HEADER_t *)pMacIndication;
    rf_sof_ctrl_t *pFramCtrl = (rf_sof_ctrl_t*)(buf->fi.head);

    if(pFramCtrl->dtei != GetTEI() && pFramCtrl->dtei != BROADCAST_TEI)
    {
        net_printf("simMac app dtei :%03x\n", pFramCtrl->dtei);
        return;
    }

    U16   PayloadLen = pMacHeader->MsduLen;
    work_t *post_work = zc_malloc_mem(sizeof(work_t) + sizeof(MSDU_DATA_IND_t) + PayloadLen,"MSDU",MEM_TIME_OUT);
    MSDU_DATA_IND_t  *pMsduInd_t = (MSDU_DATA_IND_t*)post_work->buf;

    pMsduInd_t->SNID = pFramCtrl->snid;

    if(pFramCtrl->dtei == GetTEI())
    {
        __memcpy(pMsduInd_t->DstAddress, nnib.MacAddr, MACADRDR_LENGTH);
    }
    else
    {
        memset(pMsduInd_t->DstAddress, 0xFF, MACADRDR_LENGTH);
    }

#if defined(STATIC_MASTER)
    if(is_rd_ctrl_tei(pFramCtrl->stei))
    {
        memset(pMsduInd_t->SrcAddress,0xDD,MACADRDR_LENGTH);
    }
    else
    {
        __memcpy(pMsduInd_t->SrcAddress, DeviceTEIList[pFramCtrl->stei-2].MacAddr, MACADRDR_LENGTH);
    }
#else
    if(is_rd_ctrl_tei(pFramCtrl->stei))
        memset(pMsduInd_t->SrcAddress,0xDD,MACADRDR_LENGTH);
    else
        __memcpy(pMsduInd_t->SrcAddress, nnib.CCOMacAddr, MACADRDR_LENGTH);
#endif


    __memcpy(pMsduInd_t->Msdu, pMacHeader->Msdu, PayloadLen);
    pMsduInd_t->MsduLength = PayloadLen;
    pMsduInd_t->Stei = pFramCtrl->stei;
//    pMsduInd_t->SendType = pMacHeader->SendType;

    post_work->work = pProcessMsduDataInd;
    post_work->msgtype = MSDUIND;
    post_app_work(post_work);

}


static void MsduDataIndToApp(mbuf_t *buf, U8 *pMacIndication, U16 len)
{
   MAC_PUBLIC_HEADER_t		 *pMacHeader = NULL;
    pMacHeader = (MAC_PUBLIC_HEADER_t *)pMacIndication;

    net_printf("pMacHdDes=%4x, Scr=%04x\n",pMacHeader->DestTEI,pMacHeader->ScrTEI);

    if((pMacHeader->SendType == e_UNICAST_FREAM )&&
           (pMacHeader->DestTEI != GetTEI())
           )//�����Ҳ���Ŀǰ��ַ����ǰת��
    {        

#if defined(STATIC_NODE)
        if(nnib.NodeState == e_NODE_ON_LINE )
        {
            net_printf("RND\n");
            
            //ת�����ݣ�post��Ϣ��MacSTask();
            RelayMacData(pMacIndication, len, buf);//ת��������
        }
#endif
    }
    else   //��APS�㷢����Ϣ
    {
        //pMacHeader = (MAC_PUBLIC_HEADER_t *)pMacIndication;

		
        U16   PayloadLen = pMacHeader->MsduLength_l + (pMacHeader->MsduLength_h << 8);


        work_t *post_work = zc_malloc_mem(sizeof(work_t) + sizeof(MSDU_DATA_IND_t) + PayloadLen,"MSDU",MEM_TIME_OUT);
        MSDU_DATA_IND_t  *pMsduInd_t = (MSDU_DATA_IND_t*)post_work->buf;

        pMsduInd_t->SNID = buf->snid;

        if(pMacHeader->DestTEI == GetTEI())
        {
        	if(getHplcTestMode() == RD_CTRL)
        	{
				memset(pMsduInd_t->DstAddress, 0xdd, MACADRDR_LENGTH);
			}
			else
			{
				__memcpy(pMsduInd_t->DstAddress, nnib.MacAddr, MACADRDR_LENGTH);
			}
        }
        else
        {
            memset(pMsduInd_t->DstAddress, 0xFF, MACADRDR_LENGTH);
        }

#if defined(STATIC_MASTER)

        if(is_rd_ctrl_tei(pMacHeader->ScrTEI))
        {
            if(pMacHeader->MacAddrUseFlag == TRUE)
            {
                __memcpy(pMsduInd_t->SrcAddress, &pMacIndication[sizeof(MAC_PUBLIC_HEADER_t)], MACADRDR_LENGTH);
            }
            else
            {
               memset(pMsduInd_t->SrcAddress,0xDD,MACADRDR_LENGTH);
            }
        }
        else
        {
            __memcpy(pMsduInd_t->SrcAddress, DeviceTEIList[pMacHeader->ScrTEI-2].MacAddr, MACADRDR_LENGTH);
        }
#else
        if(pMacHeader->MacAddrUseFlag == TRUE)
        {
            __memcpy(pMsduInd_t->SrcAddress, &pMacIndication[sizeof(MAC_PUBLIC_HEADER_t)], MACADRDR_LENGTH);
        }
        else
        {
            if(is_rd_ctrl_tei(pMacHeader->ScrTEI))
                memset(pMsduInd_t->SrcAddress,0xDD,MACADRDR_LENGTH);
            else
                __memcpy(pMsduInd_t->SrcAddress, nnib.CCOMacAddr, MACADRDR_LENGTH);
        }
#endif

        if(pMacHeader->MacAddrUseFlag)
        {
            __memcpy(pMsduInd_t->Msdu, &pMacIndication[sizeof(MAC_LONGDATA_HEADER)], PayloadLen);
        }
        else
        {
            __memcpy(pMsduInd_t->Msdu, &pMacIndication[sizeof(MAC_PUBLIC_HEADER_t)], PayloadLen);
        }
//        pMsduInd_t->MacUsed = pMacHeader->MacAddrUseFlag;
        if(is_rd_ctrl_tei(pMacHeader->ScrTEI))  //��¼���������ݷ����Ƿ�ʹ�õ��ǳ�mac
        {
            rd_ctrl_info.mac_use = pMacHeader->MacAddrUseFlag;
            rd_ctrl_info.linktype = buf->LinkType;
        }
        pMsduInd_t->MsduLength = PayloadLen;
		
        pMsduInd_t->Stei = pMacHeader->ScrTEI;
        pMsduInd_t->SendType = pMacHeader->SendType;
        

        post_work->work = pProcessMsduDataInd;
		post_work->msgtype = MSDUIND;
        post_app_work(post_work);
    }
#if defined(STATIC_NODE)
    net_printf("pMacHd-sType=%d,Dst=0x%02x,Type=%d\n",pMacHeader->SendType,pMacHeader->DestTEI,nnib.NodeType);

	if(pMacHeader->RemainRadius == 0 && pMacHeader->SendType !=e_UNICAST_FREAM)//��������
	{
		app_printf("=0, pMacHd-sType:%d\n",pMacHeader->SendType);
		return;
	}
    if(pMacHeader->SendType == e_FULL_BROADCAST_FREAM_NOACK )//�ж��Ƿ�Ҫת��
    {
        if(pMacHeader->DestTEI != GetTEI())
        {
            //ת�����ݣ�post��Ϣ��MacSTask();
            RelayMacData(pMacIndication, len, buf); //ת�㲥����
        }
    }
    else if(pMacHeader->SendType == e_PROXY_BROADCAST_FREAM_NOACK)
    {
        if(pMacHeader->DestTEI != GetTEI() && nnib.NodeType == e_PCO_NODETYPE)
        {
            //ת�����ݣ�post��Ϣ��MacSTask();
            RelayMacData(pMacIndication, len, buf);//ת�㲥����
        }
    }
    else
    {
        //do nothing
    }

	
#endif

    return;

}








void Trigger2app(U8 state)
{
    work_t *work = zc_malloc_mem(sizeof(work_t) + sizeof(NETWORK_ACTION_t) + 100,"Trigger2app",MEM_TIME_OUT);
    work->work = pNetStatusChangeInd;
    work->buf[0] = state;
    work->msgtype = TRIG2APP;
    post_app_work(work);
}

//////////////////////////////////////////////////PHY�ӿ�,mac���/////////////////////////////////////
//1)mpdu����ACK��cts��beacon����
//2)comfirm


//U8  cmb_buf[2500]={0};
//U16 cmb_len=0;
ostimer_t updataMsduTimer;


MSDU_DATA_SEQ_t MsduDataSeq[MAXFILTER_NUM];

static void updataMsduSeq(work_t *work);

static void updataMsduTimerCB( struct ostimer_s *ostimer, void *arg)
{
//    work_t *work = zc_malloc_mem(sizeof(work_t),"MSDU",MEM_TIME_OUT);
//    work->work = updataMsduSeq;
//    post_datalink_task_work(work);

    updataMsduSeq(NULL);
}

static void modify_updataMsduTimer(uint32_t first)
{
    if(updataMsduTimer.fn == NULL)
    {
        timer_init(&updataMsduTimer,
                   first,
                   first,
                   TMR_PERIODIC,//TMR_TRIGGER
                   updataMsduTimerCB,
                   NULL,
                   "network_timerCB"
                   );
    }
    else
    {
        timer_modify(&updataMsduTimer,
                first,
                first,
                TMR_PERIODIC ,//TMR_TRIGGER
                updataMsduTimerCB,
                NULL,
                "network_timerCB",
                TRUE);
    }

    timer_start(&updataMsduTimer);
	
}



/*����ԭ����е���
1.�㲥ʹ��SNID,MsduSeq,RestTims����
�㲥�Ĺ���ԭ��
�Զ�ʱ��Ľ��յ��Ĺ㲥֡(ȫ���㲥�ʹ���㲥)�洢�����ô洢��֡���½��ܵ�֡�����ظ����ˡ�
2.���������ع㲥����ʹ��SNID,SrcTei,MsduSeq,RestTims���ˣ�ʹ��֮ǰһ������Ϣ�������˵�ǰ��Ϣ��
*/

static void AddMsduSeq(U16 MsduSeq, U8 RestTims, U16 srcTei)
{
    U8 i;
    for(i=0;i<MAXFILTER_NUM;i++)
    {
        if(INVALID== MsduDataSeq[i].DataMsduSeq && 0xff == MsduDataSeq[i].Resettimes)
        {

            MsduDataSeq[i].DataMsduSeq = MsduSeq;
            MsduDataSeq[i].Resettimes  = RestTims;
            MsduDataSeq[i].LifeTime = 0;
            MsduDataSeq[i].tei = srcTei;
            break;

        }
    }
    //λ����,first in first out
    if(i==MAXFILTER_NUM)
    {
        __memmove((U8*)&MsduDataSeq[0],(U8*)&MsduDataSeq[1],(MAXFILTER_NUM-1)*sizeof(MSDU_DATA_SEQ_t));
        MsduDataSeq[MAXFILTER_NUM-1].DataMsduSeq = MsduSeq;
        MsduDataSeq[MAXFILTER_NUM-1].Resettimes  = RestTims;
        MsduDataSeq[MAXFILTER_NUM-1].LifeTime = 0;
        MsduDataSeq[MAXFILTER_NUM-1].tei = srcTei;
    }
    if(timer_query(&updataMsduTimer)==TMR_STOPPED )
    {
        modify_updataMsduTimer(1000);
    }
    return;
}

static U8 CheckMsduSeq(U16 MsduSeq, U8 RestTims, U16 srcTei)
{
    U8 i;
	for(i=0;i<MAXFILTER_NUM;i++)
    {
        if(MsduSeq==MsduDataSeq[i].DataMsduSeq && RestTims == MsduDataSeq[i].Resettimes && MsduDataSeq[i].tei == srcTei)
		{
			MsduDataSeq[i].LifeTime = 0;
			return FALSE;	
		}
	}  
    return TRUE;
}



static void updataMsduSeq(work_t *work)
{
    U8 i ,j;
    for(i=0;i<MAXFILTER_NUM;i++)
    {
        if(INVALID!= MsduDataSeq[i].DataMsduSeq)
        {
            MsduDataSeq[i].LifeTime++;
            if(MsduDataSeq[i].LifeTime >=MASKTIME)
            {
                memset((U8*)&MsduDataSeq[i],0xff,sizeof(MSDU_DATA_SEQ_t));
            }
        }

    }
    for(i=0;i<MAXFILTER_NUM;i++)//���յ�λ�ã��޳�
    {
        if(INVALID == MsduDataSeq[i].DataMsduSeq)
        {

            for(j=i+1;j<MAXFILTER_NUM;j++)
            {
                if(INVALID != MsduDataSeq[j].DataMsduSeq)
                {
                    __memcpy((U8*)&MsduDataSeq[i],(U8*)&MsduDataSeq[j],sizeof(MSDU_DATA_SEQ_t));
                    memset((U8*)&MsduDataSeq[j],0xff,sizeof(MSDU_DATA_SEQ_t));
                    break;
                }
            }
        }
    }
    if(INVALID == MsduDataSeq[0].DataMsduSeq)
    {
        net_printf("MsduDataSeq is empty, Stop Update \n");
        timer_stop(&updataMsduTimer, TMR_NULL);
    }

    return;
}

//�����ھӱ��¼��MSDU��ţ������ع㲥�Ĺ����Լ��������� 
U8 CheckUNICASTMsduSeq(U16 TEI , U16 MsduSeq , U8 RestTims)
{
	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

	if(TEI == 0)
	{
		return TRUE; //����δ����TEI�Ľڵ���ˣ���������
	}
    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
	{
        DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

		if(DiscvryTable->NodeTEI == TEI)
        {
    		if(MsduSeq==DiscvryTable->MsduSeq && RestTims == DiscvryTable->ResetTimes)
    		{
                
    			return FALSE;	
    		}
        }
	}

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *rfNodeInfo;

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        rfNodeInfo= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(rfNodeInfo->NodeTEI == TEI)
        {
            if(MsduSeq==rfNodeInfo->MsduSeq && RestTims == rfNodeInfo->ResetTimes)
            {

                return FALSE;
            }
        }
    }

	return TRUE;
}

void AddUNICASTMsduSeq(U16 TEI , U16 MsduSeq , U8 RestTims)
{
	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

	if(TEI == 0)return ;
    
	list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
	{
        DiscvryTable= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

		if(DiscvryTable->NodeTEI == TEI)
        {
    		DiscvryTable->MsduSeq = MsduSeq;
			DiscvryTable->ResetTimes = RestTims;
            
            break; ;
        }
	}

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *rfNodeInfo;

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        rfNodeInfo= list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(rfNodeInfo->NodeTEI == TEI)
        {
            rfNodeInfo->MsduSeq = MsduSeq;
            rfNodeInfo->ResetTimes = RestTims;
            return ;
        }
    }
    
	
}

//static void usebackup_route(MACDATA_CONFIRM_t  *pMacCfm)
//{
//    if(pMacCfm->RouterMode == e_MainRoute  || pMacCfm->RouterMode == e_FristBackupRoute) //��·�ɳ���ʧ��.��һ����·��ʧ��
//    {
//        U16	 nextTEI;
//        if(pMacCfm->DstTei == CCO_TEI) //˵�����������ݣ������л�����������·��
//        {
			
//            nextTEI = SearchBestBackupProxyNode(pMacCfm->NextTei);
//            if(nextTEI !=INVALID)
//            {
//                MAC_PUBLIC_HEADER_t *pMacData;
//                pMacData = (MAC_PUBLIC_HEADER_t*)pMacCfm->data;
//                MacDataRequst(pMacData,pMacCfm->dataLen,
//                        nextTEI,pMacCfm->SendType,pMacCfm->MsduHandle,pMacCfm->LinkId,pMacCfm->MsduType,
//                        pMacCfm->ResendFlag,0, pMacCfm->RouterMode == e_MainRoute?e_FristBackupRoute:e_SecondBproute,0);//
//                return;
//            }


//        }
//        MsduConfirmToApp(pMacCfm->ScrTei,pMacCfm->MsduHandle, pMacCfm->status);
//    }
//	else if(pMacCfm->RouterMode == e_SecondBproute)//���б���·�ɳ���ʧ��
//	{
//		 MsduConfirmToApp(pMacCfm->ScrTei,pMacCfm->MsduHandle, pMacCfm->status);
//	}
//}


U16 GetApsPacketID(void *pld)
{
	U16 len=0;
	MAC_PUBLIC_HEADER_t 	*pMacHeader = NULL;
	L_APS_HEADER_t *pApsHeader;

	pMacHeader = (MAC_PUBLIC_HEADER_t *)pld;
    if(pMacHeader->MacProtocolVersion == e_MAC_VER_STANDARD)
    {
        len = pMacHeader->MacAddrUseFlag?28:16; //mac֡ͷ
    }
    else
    {
        len = sizeof(MAC_RF_HEADER_t); //mac֡ͷ
    }

	pApsHeader = (L_APS_HEADER_t *)(pld+len);
	return pApsHeader->PacketID;
}

//MAC֡ ����֡����ӿ�
void ProcSigleHopMacIndication(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc, work_t *work)
{
    U32 					MSDUCrc32Data = 0;
    U8                      MacHeadLen = 0;
    U32                     MsduLen = 0;
    MAC_RF_HEADER_t         *pMacHeader = NULL;

    rf_sof_ctrl_t *pFrame = (rf_sof_ctrl_t *)buf->fi.head;
    pMacHeader = (MAC_RF_HEADER_t *)pld;

#if defined(STATIC_NODE)
    //<<< TODO
    //���������ھ�����
    AddNeighborNet(buf, DT_SOF);//�����ھ�����
    //���������ŵ�ɨƵ
    ScanRfChMange(e_RECEIVE, buf->snid);
#endif

    //���������ھӱ���Ϣ��
    //TODO ���������ھӱ���Ϣ
    UpdateRfNbInfo(buf->snid, NULL, pFrame->stei, 0, 0, 0, 0, buf->rssi, buf->snr,0 ,0 , DT_SOF);

    if(crc != 0)
    {
        return;
    }

    MacHeadLen = sizeof(MAC_RF_HEADER_t);
    MsduLen = pMacHeader->MsduLen;


    if(MsduLen == 0 || MsduLen > 508)
    {
        debug_printf(&dty,DEBUG_MDL,"ProcMacDataIndication MsduLen err:%d\n",len);
        debug_printf(&dty,DEBUG_MDL,"MsduLen=%d\n", MsduLen);
        return;
    }


    crc_digest(pld+ MacHeadLen, MsduLen, (CRC_32 | CRC_SW), &MSDUCrc32Data);

    if(0==memcmp((void *)&MSDUCrc32Data, pld + MacHeadLen + MsduLen, CRCLENGTH))
    {
        if(HPLC.testmode == MACTPT)
        {
            uart1_send(pld + MacHeadLen, MsduLen);
            return;
        }
    }
    else
    {
        net_printf("ProcMacDataIndication : crc_digest err!\n");
        return;
    }

//    if(buf->snid != GetSNID())
//    {
//        return;
//    }



    //����MAC֡��������Ӧ�Ĵ���
    if(pMacHeader->MsduType == e_RF_APS_FREAME )//APS������
    {
        if(GetSNID() !=0 && GetSNID()!= buf->snid) //���Ǳ�̨������Ҫ��̨��ʶ���ͨ�Ų��������͵�APS
        {
            U16 PacketID=0;
            PacketID = GetApsPacketID(pld);
            if(PacketID != e_INDENTIFICATION_AREA &&PacketID != e_COMMU_TEST)//����̨��ʶ�������ͷ�
            {
                net_printf("free = 0x%04x\n",PacketID);
                return;
            }
            net_printf("GetApsPacketID len = %d\n",len);
        }
//		*mgType = APS_SOF;
//        mg_update_type(APS_SOF, work->record_index);
        SimMacMsduDataIndToApp(buf, pld, MsduLen + MacHeadLen + CRCLENGTH);
    }
    else if(pMacHeader->MsduType == e_RF_DISCOVER_LIST)     //���߷����б�
    {
//        if(MONITOR == HPLC.testmode)
//        {
//            U8 *pHead = zc_malloc_mem(len + sizeof(sof_ctrl_t), "spy", MEM_TIME_OUT);
//            U8 *payload = pHead + sizeof(sof_ctrl_t);

//            __memcpy(pHead, buf->fi.head, sizeof(sof_ctrl_t));
//            __memcpy(payload, pld, len);

//            uart1_send((uint8_t *)pHead, len + sizeof(sof_ctrl_t));

//            zc_free_mem(pHead);
//            return;
//        }

        //���߷����б���
        if(buf->snid != GetSNID())
            return;

        net_printf("e_RF_DISCOVER_LIST\n");
        ProcessMMeRFDiscoverNodeList(buf, pld, len, work);
        return;

    }
    else//δ֪����
    {
        return;
    }

    return;

}
//MAC֡ ��׼֡����ӿ�
void ProcMPDUDataIndication(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc, work_t *work)
{
    /*
    //����ͳ��
    //���ݽ�����MAC����֡����msdu��CRC��֤,phy crc�������
    //����
    //SNR���� void CollectAllStaSNR(frame_ctrl_t*fch,double Snr,int32_t GAIN);
    */
//    U32						protime,protime1,protime2;
//    U16 					byCmdID = 0;
    U32 					MSDUCrc32Data = 0;
    U8                      MacHeadLen = 0;
    U32                     MsduLen = 0;
    MAC_PUBLIC_HEADER_t 	*pMacHeader = NULL;
	sof_ctrl_t *pFrame = (sof_ctrl_t *)buf->fi.head;
    if(pFrame->dt != DT_SOF)
    {
        net_printf("pFrame->dt != DT_SOF\n");
        return;
    }


    pMacHeader =   (MAC_PUBLIC_HEADER_t *)pld;



	
	#if defined(STATIC_NODE)


    //CCO������������ģ�黹δ���ߣ�����������
    if(pMacHeader->FormationSeq != nnib.FormationSeqNumber && GetSNID() == buf->snid && GetNodeState()==e_NODE_ON_LINE)
    {
        if(!is_rd_ctrl_tei(get_sof_stei((sof_ctrl_t *)buf->fi.head)))
        {
            return;
        }
    }

    AddNeighborNet(buf, DT_SOF);//�����ھ�����
    //������Դ�ز���ִ���ز���ز���
    if(buf->LinkType == e_HPLC_Link)
    {
        ScanBandManage(e_RECEIVE, buf->snid);
    }
    else if(buf->LinkType == e_RF_Link)
    {
        //TODO �ŵ�ɨ�� recv
        ScanRfChMange(e_RECEIVE, buf->snid);
    }
	#endif
    if(buf->LinkType == e_HPLC_Link)
    {
        UpdateNbInfo(buf->snid, NULL, pFrame->stei,0,0,0,0,buf->rgain,0,0,DT_SOF);
    }
    else if(buf->LinkType == e_RF_Link)
    {
        //TODO ���������ھӱ���Ϣ
        UpdateRfNbInfo(buf->snid, NULL, pFrame->stei, 0, 0, 0, 0, buf->rssi, buf->snr,0 ,0 , DT_SOF);
    }
    
    //crc У��
    if(crc != 0)
    {
        //net_printf("pb err,sfo: %.3fppm\n", phy_get_sfo(SFO_DIR_TX));
        return;
    }

    MacHeadLen = pMacHeader->MacAddrUseFlag ? sizeof(MAC_LONGDATA_HEADER):sizeof(MAC_PUBLIC_HEADER_t);
    MsduLen    = pMacHeader->MsduLength_h<<8 | pMacHeader->MsduLength_l;

    if(pMacHeader->RemainRadius > 0)
    {
        pMacHeader->RemainRadius -= 1;
    }
	
    if(MsduLen == 0 || MsduLen > 512*4)
    {
        debug_printf(&dty,DEBUG_MDL,"ProcMacDataIndication MsduLen err:%d,srctei=%04x\n",len, pMacHeader->ScrTEI);
        debug_printf(&dty,DEBUG_MDL,"MsduLen=%d\n", MsduLen);
//        sys_panic("<ProcMacDataIndication MsduLen err:%d \n> %s: %d\n",len, __func__, __LINE__);
        return;
    }


    crc_digest(pld+ MacHeadLen, MsduLen, (CRC_32 | CRC_SW), &MSDUCrc32Data);

    if(0==memcmp((void *)&MSDUCrc32Data, pld + MacHeadLen + MsduLen, CRCLENGTH))
	{
		if(HPLC.testmode == MACTPT)
        {
            uart1_send(pld + MacHeadLen, MsduLen);
			return;
		}
	}
	else
	{
        net_printf("ProcMacDataIndication : crc_digest err!\n");
		return;
	}

	if((pMacHeader->SendType == e_UNICAST_FREAM && pMacHeader->DestTEI	== BROADCAST_TEI)
            || (pMacHeader->DestTEI != 0xfff  && pMacHeader->SendType != e_UNICAST_FREAM)) //�Ƿ�֡����
	{
		net_printf("it's err sendtype\n");
//		*mgType = ERR_SOF;
        mg_update_type(ERR_SOF, work->record_index);
		return;
		
	}


    //MSDU���˷�Ϊ(����㲥��ȫ���㲥)��(���������ع㲥)��������
    if ( (pMacHeader->SendType ==e_PROXY_BROADCAST_FREAM_NOACK ||  pMacHeader->SendType ==e_FULL_BROADCAST_FREAM_NOACK) )
    {
        #if defined(STATIC_MASTER)
        if(GetSNID() == buf->snid && pMacHeader->ScrTEI == GetTEI())    //cco ���յ����ڵ㷢�͵Ĺ㲥���ģ�ֱ�ӹ��ˡ�
        {
            return;
        }
        #endif

//        net_printf("seq:%d, restTime:%d\n", pMacHeader->MsduSeq, pMacHeader->RestTims);
		if ( !CheckMsduSeq(pMacHeader->MsduSeq,pMacHeader->RestTims, pMacHeader->ScrTEI) )//�㲥ֻ֡��Ҫ��Ź���
		{

			net_printf("-b\n");
//			*mgType = REAPEAT_SOF;
            mg_update_type(REAPEAT_SOF, work->record_index);
			return;
		}
		else
		{
			AddMsduSeq(pMacHeader->MsduSeq , pMacHeader->RestTims, pMacHeader->ScrTEI);
		}
    }
	else //���ع㲥&����
	{

        if(get_sof_dtei((sof_ctrl_t *)buf->fi.head) != GetTEI() && pMacHeader->SendType == e_UNICAST_FREAM)
        {
//        	*mgType = NOME_SOF;
            mg_update_type(NOME_SOF, work->record_index);
//            net_printf("dtei:%03x, sndTp:%d\n",get_sof_dtei((sof_ctrl_t *)buf->fi.head),  pMacHeader->SendType);
            return;
        }
        if ( !CheckUNICASTMsduSeq(get_frame_stei((frame_ctrl_t *)buf->fi.head) , pMacHeader->MsduSeq,pMacHeader->RestTims)  )//���˱��ع㲥�͵���
		{
			//app_printf("MsduType : %d\n",pMacPayload_t->MsduType);
//			*mgType = REAPEAT_SOF;
            mg_update_type(REAPEAT_SOF, work->record_index);
//            net_printf("--ScrTEI=%02x,MsduSeq=%02x,filter Unicast cast----\n",pMacHeader->ScrTEI,pMacHeader->MsduSeq);

//            if(pMacHeader->SendType == e_LOCAL_BROADCAST_FREAM_NOACK)       //����̨���������
                return;
		}
		else
		{
			AddUNICASTMsduSeq(get_frame_stei((frame_ctrl_t *)buf->fi.head) , pMacHeader->MsduSeq , pMacHeader->RestTims); //�����ھӽڵ��MSDU
		}
		
	}

	
    //����MAC֡��������Ӧ�Ĵ���
    if(pMacHeader->MsduType == e_APS_FREAME )//APS������
    {
        if(GetSNID() !=0 && GetSNID()!= buf->snid) //���Ǳ�̨������Ҫ��̨��ʶ���ͨ�Ų��������͵�APS
        {
            U16 PacketID=0;
            PacketID = GetApsPacketID(pld);
            if(0 == buf->snid && PacketID == e_QUERY_ID_INFO)
            {
                net_printf("GetApsPacketID %d len = %d\n",e_QUERY_ID_INFO,len);
            }
            else if(PacketID != e_INDENTIFICATION_AREA && PacketID != e_COMMU_TEST)//����̨��ʶ�������ͷ�
            {
                net_printf("free = 0x%04x\n",PacketID);
                return;
            }
            net_printf("GetApsPacketID len = %d\n",len);
        }
//		*mgType = APS_SOF;
        mg_update_type(APS_SOF, work->record_index);
        MsduDataIndToApp(buf, pld, MsduLen + MacHeadLen + CRCLENGTH);
    }
    else if(pMacHeader->MsduType == e_NETMMSG)//���������Ϣ
    {
    	if( GetSNID()== buf->snid)
    	{
             ProcessNetMgmMsg(buf, pld, len,work);
    	}
       
        return;
	
    }
    else//δ֪����
    {
        return;
    }
	
    return;
	
}



///*
//������ά���������˴����������ֹ�����������Ϣ������ģ��
//*/
void ProcSackCtrlRequest(U8 dt, U8 *addr,U8 linktype)
{
    if(linktype == e_HPLC_Link)
    {
        mbuf_hdr_t *p = packet_sack_ctl(dt, addr, HPLC.tei, HPLC.snid);//packet sack
        if(p)
        {
            if(rd_ctrl_info.send_sack_cnt < 5)
            {
                phy_reset();
                ld_set_tx_phase_zc(PHY_PHASE_ALL);
                evt_notifier_t ntf[1];
                fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, csma_tx_end, NULL);
                if ((phy_start_tx(&p->buf->fi, NULL, &ntf[0], 1)) < 0) {
                    printf_d("WARNING ProcSackCtrlRequest plc\n");
                }
                //            hplc_do_next();
                zc_free_mem(p);
            }
            else
            {
                void msg_trace_record_func(U8 op ,tx_link_t *tx_link,U8 func);
                msg_trace_record_func(1,&TX_MNG_LINK,3);
                if((tx_link_enq_by_lid(p, &TX_MNG_LINK)) < ENTERY_OK)
                {
                    zc_free_mem(p);
                }
            }
        }
    }
    else if(linktype == e_RF_Link)
    {
		
/* 	    HPLC.option = 2;
	    HPLC.rfchannel = 16;
	    wphy_set_option(HPLC.option);
	    wphy_set_channel(HPLC.rfchannel); */
        mbuf_hdr_t *p_rf = packet_sack_ctl_rf(dt, addr, HPLC.tei, HPLC.snid);//packet sack
        if(p_rf)
        {
            if(rd_ctrl_info.send_sack_cnt < 5)
            {
                wphy_reset();
                evt_notifier_t ntf[1];
                fill_evt_notifier(&ntf[0], WPHY_EVT_EOTX, wl_csma_tx_end, NULL);
                if ((wphy_start_tx(&p_rf->buf->fi, NULL, &ntf[0], 1)) < 0) {
                    printf_d("WARNING ProcSackCtrlRequest rf\n");
                }
                //            hplc_do_next();
                zc_free_mem(p_rf);
            }
            else
            {
                void msg_trace_record_func(U8 op ,tx_link_t *tx_link,U8 func);
                msg_trace_record_func(1,&WL_TX_MNG_LINK,3);
                if((tx_link_enq_by_lid(p_rf, &WL_TX_MNG_LINK)) < ENTERY_OK)
                {
                    zc_free_mem(p_rf);
                }
            }
        }
    }
}
void ProcSackDataIndication(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc)
{
   sack_ctrl_t *pFrameCtrl = (sack_ctrl_t *)buf->fi.head;

   //printf_s("buf->LinkType:%d\n",buf->LinkType);

   if(pFrameCtrl->dt != DT_SACK)
   {
       net_printf("ProcSackDataIndication : pFrameCtrl->dt  = %d \n", pFrameCtrl->dt);
       return;
   }
   if(pFrameCtrl->ext_dt == 0)         //sack
   {

   }
   else if(pFrameCtrl->ext_dt == 1)    //��������չ����֡
   {
        if(check_lowpower_states_hook != NULL)
        {   
            check_lowpower_states_hook();
        }
   		if(getHplcTestMode() == RD_CTRL)
   		{
			return;
		}
       rd_ctrl_info.linktype = buf->LinkType;
       sack_search_ctrl_t *pFrame = (sack_search_ctrl_t *)buf->fi.head;
#if defined(ZC3750STA)
       if(0==memcmp(pFrame->addr,DevicePib_t.DevMacAddr,6) )
#elif  defined(ZC3750CJQ2)
       //if(0==memcmp(pFrame->addr,CollectInfo_st.CollectAddr,6) )
       if(	cmp_cjq_meterlist(pFrame->addr) ==TRUE)
#elif defined(ZC3951CCO)
       if(0==memcmp(pFrame->addr,FlashInfo_t.ucJZQMacAddr,6) )
#endif
       {
           if(TRUE == rd_ctrl_info.period_sendsyh_flag)//���ڷ�ͬ��֡ʱ���յ�����֡���������־λ����������)
           {
               net_printf("rcv search frame,clear period_sendsyh_flag,Reconnect!!!!!!!!!!!!\n");
               rd_ctrl_info.send_sack_cnt = 0;
               rd_ctrl_info.period_sendsyh_flag = FALSE;

               modify_send_syh_timer(100);
               modify_send_syh_timeout_timer(3*60*1000);
#if defined(STATIC_NODE)    //��յ�ǰʱ϶�������Զ�����ʱ϶ʱ������TDMA
               if(nnib.NodeState != e_NODE_ON_LINE )
               {
                //    hplc_drain_beacon(TRUE);
                    Clean_All_time_slot(TRUE);
                    extern void packet_csma_bpts(plc_work_t *mbuf_hdr);
                    packet_csma_bpts(NULL);
               }
#endif
               return;

           }

           modify_send_syh_timer(100);
           modify_send_syh_timeout_timer(3*60*1000);
#if defined(STATIC_NODE)
           if(nnib.NodeState != e_NODE_ON_LINE )//&& nnib.SNID == 0)
           {
//                    Mpib_t.BeaconTimeManager_t.Available = FALSE;
               net_printf("----stop  switch band and wait g_SendsyhtimeoutTimer-----\n");
               ScanBandManage(e_CNTCKQ, buf->snid);
           }
#endif
       }
       else
       {
           if(TMR_RUNNING == zc_timer_query(g_Sendsyhtimer))
               timer_stop(g_Sendsyhtimer,TMR_NULL);
       }
   }
   else if(pFrameCtrl->ext_dt == 2)    //��������չͬ��֡
   {
#if defined(ZC3750STA) //|| defined(ZC3951CCO)
		sack_syh_ctrl_t *pFrame = (sack_syh_ctrl_t *)buf->fi.head;
		if(getHplcTestMode() == RD_CTRL && rd_ctrl_info.recsyflag==0)
		{
			rd_ctrl_info.recsyflag = 1;
			net_printf("rdctrl connect succ!!!!!!!!!\n");
			HPLC.snid = pFrameCtrl->snid;
			SetPib(pFrame->snid,CTRL_TEI_GW);
			rd_ctrl_info.dtei = pFrame->stei;
			net_printf("snid : 0x%08x  RD_dtei : %02x\n",GetSNID(),rd_ctrl_info.dtei);
			
			if(rd_ctrl_info.dtei == 0 && GetSNID()==0)
			{
				net_printf("packet_csma_bpts!!!!!!!!!\n");
				// hplc_drain_beacon(TRUE);
                Clean_All_time_slot(TRUE);

				extern void packet_csma_bpts(plc_work_t *mbuf_hdr);
				packet_csma_bpts(NULL);
			}
			RD_SACK_POST;
		}
#endif
   }
   else                                //other
   {
       net_printf("Sack : pFrameCtrl->ext_dt = %d \n", pFrameCtrl->ext_dt);
   }

   return;
}


///*
// ����Э���Ĵ��� ���˴����������ֹ�����������Ϣ������ģ��
//*/
#if defined(STATIC_MASTER)

U8 comcmpMac(U8 *pMac1, U8 *pMac2)
{
    U8 i;
#if 0
    for(i = 0; i < 6; i++)
    {
        if(pMac1[5 - i] > pMac2[5 - i])
        {
            return TRUE;
        }
        else if(pMac1[5 - i] < pMac2[5 - i])
        {
            return FALSE;
        }
    }
#else
    for(i = 0; i < 6; i++)
    {
        if(pMac1[i] > pMac2[i])
        {
            return TRUE;
        }
        else if(pMac1[i] < pMac2[i])
        {
            return FALSE;
        }
    }
#endif
    return FALSE;
}

//����������Э���˱�ָ������
U32 GetBeaconOffset()
{
    U8      i;
    U32     backoffNum = 0;
    U16     SendDuration   ;
    U16     SendStartOffset;
    U16     MyDuration     ;
    U16     MyStartOffset  ;

    MyDuration = HPLC.dur;

    U8			changeFlag=FALSE;
    U16			Ia,Ib,Yc,Yd;

    U8 level = 0;               //ֵԽ�����ȼ�Խ��
    U8 tmplevel = 0;

    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(NeighborNet[i].SendSNID == 0xFFFFFFFF || NeighborNet[i].Duration == 0 || NeighborNet[i].Duration == 0xffff)
            continue;

        MyStartOffset   = NeighborNet[i].MyStartOffset;
        SendDuration    = NeighborNet[i].Duration;
        SendStartOffset = NeighborNet[i].SendStartOffset;

        net_printf("Bcon snid = %06x\n", NeighborNet[i].SendSNID);

        net_printf("Mysft=0x%04x Nbsft=0x%04x NbDp=0x%04x MD=0x%04x\n", MyStartOffset,SendStartOffset,SendDuration,MyDuration);

        Ia = MyStartOffset;
        Ib = MyDuration + Ia; //ʱ϶����ʱ��
        Yc = SendStartOffset;
        Yd = SendDuration + Yc;
        /*-------------------------------//snidδ��ͻ������ʱ϶��ͻ���˱�------------------------------------*/

        if(abs(Ia-Yc) < 10)  //ͬʱ����
        {
            if(GetSNID() > NeighborNet[i].SendSNID )     //SNIDС����
            {
                tmplevel = 2;
                changeFlag =TRUE;
                net_printf("---OV TG,bf\n");
            }
            else if(!NeighborNetCanHearMe(NeighborNet[i].SendSNID, NeighborNet[i].NbSNID[0])) //ͬʱ�������Է��������Լ�
            {
                tmplevel = 2;
                changeFlag =TRUE;
                net_printf("---OV TG,NO H,bf\n");
            }
            else
            {
                net_printf("---OV TG,NO bf---\n");
            }
        }
        else if(Ia < Yc)                        //���Ƚ���   �Ƚ�������ԭ��
        {
            if(Yc < Ib)              //�ű�ʱ϶���ص�
            {
                if(!NeighborNetCanHearMe(NeighborNet[i].SendSNID, NeighborNet[i].NbSNID[0]))
                {
                    tmplevel = 1;
                    changeFlag = TRUE;
                    net_printf("---OV FT,NO H,bf\n");
                }
            }
            else
            {
                net_printf("---OV FT,NO bf\n");
            }

        }
        else if(Ia > Yc)                       //�Է��Ƚ���
        {
            if(Ia < Yd)
            {
                tmplevel = 3;
                net_printf("---OV LR,bf\n");
                changeFlag = TRUE;
            }
            else
            {
                net_printf("---OV LR,NO bf\n");
            }
        }

        if(changeFlag == TRUE)
        {
            changeFlag = FALSE;
            if(level == 0 || tmplevel > level)
            {
                backoffNum = Yd - Ia + 10;
                level = tmplevel;
            }
            else if(level == tmplevel)
            {
                backoffNum = Yd - Ia + 10 > backoffNum ? Yd - Ia + 10 : backoffNum;
            }

            net_printf("Bcon bf NbSeq = %d,snid = %08x,backoffNum = %d\n"
                       ,i, NeighborNet[i].SendSNID, backoffNum);
        }

        NeighborNet[i].Duration = NeighborNet[i].SendStartOffset = NeighborNet[i].MyStartOffset = 0;
    }
    if(backoffNum != 0)
    {
        backoffNum += rand() % 30;
    }

    return backoffNum ;
}

void	ProcPhyCoordIndication(mbuf_t *buf,void *pld, uint16_t len, uint8_t crc)
{
    if(CURR_BEACON == NULL)
        return;

    coord_ctrl_t *pFrameCtrl = (coord_ctrl_t *)buf->fi.head;
    if(pFrameCtrl->dt != DT_COORD)
    {
        net_printf("ProcCoordInd Err dt = %d\n", pFrameCtrl->dt);
        return;
    }

    U16     SendDuration    = pFrameCtrl->dur;                      //�ھ��ŵ�����ʱ��
    U16     SendStartOffset = pFrameCtrl->bso;                      //����ʼƫ��  //
    //U16     MyDuration      = (HPLC.dur);		//���ŵ�����ʱ��  <<< ��Ӧ������һ���ű����ڵĳ���ʱ�䣩
    U16     MyStartOffset   = PHY_TICK2MS(CURR_BEACON->end - get_phy_tick_time());                                    //����ʼƫ��  //


    //net_printf("Coord My Dur : %d ms\n", MyDuration);

#if defined(STD_2016) || defined(STD_DUAL)
    U8			j,i;
#endif
#if defined(NEW_CCORD_ALG)

    if(pFrameCtrl->snid == GetSNID())//����Э������SNID��ͻ���ı��Լ���SNID,��������
    {
        net_printf("---Recv Coord and Clean\n");
        ClearNNIB();
//        HPLC.snid = rand() & 0x00FFFFFF;//�������
        SetPib(rand() & 0x00FFFFFF, CCO_TEI);
        return;
    }

    if(SendStartOffset == 0)
    {
        //������յ�offsetΪ0������Э�����ж��Լ��Ƿ��Ѿ��滮��ʱ϶������Ѿ��滮��ʱ϶���˱����¹滮��
        //if (SendDuration > PHY_TICK2MS(get_phy_tick_time() - beacon.start))
        if(!list_empty(&BEACON_LIST))
        {
            // hplc_drain_beacon(FALSE);
            Clean_All_time_slot(FALSE);
            BeaconStopOffset = SendDuration + ((rand() % 5) * 10);
            net_printf("NB TDMA is begining, BACK off %d\n", BeaconStopOffset);
            packet_beacon_period(NULL);
        }


        return;

    }
#else
    U8			changeFlag=FALSE;
    U16			Ia,Ib,Yc,Yd;
//    COORD_INDICATION_t *pCoord_Ind;
//    pCoord_Ind = (COORD_INDICATION_t *) pMsgData;
    net_printf("---ProcPhyCoordIndication\n");
    if(1) //<<<  nnib.WorkState == e_WORK
    {
        if(pFrameCtrl->snid == GetSNID())//����Э������SNID��ͻ���ı��Լ���SNID,��������
        {

            net_printf("---Recv Coord and Clean\n");
            ClearNNIB();
#if defined(STD_2016)
            HPLC.snid = rand() & 0x00FFFFFF;//�������
#elif defined(STD_GD)
            NeighborNet[buf->snid - 1].LifeTime = SNID_LIFE_TIME; //��ӻ��߸��·�������Э���ߵ�����ʱ��
            /*for(i = 0; i < MaxSNIDnum-1; i++)
            {
                if( 1<< i &  pFrameCtrl->neighbour)
                {
                    NeighborNet[i].LifeTime = SNID_LIFE_TIME;
                }

            }*/
            SetSNIDFormation();
#endif
//            SetPibRequest(nnib.SNID,CCO_TEI);
            SetPib(HPLC.snid, CCO_TEI);

            //���¹滮ʱ϶
//            modify_sendcenterbeacon_timer(BEACONPERIODLENGTH/10);

            return;
        }

        Ia = MyStartOffset;
        Ib = HPLC.dur + Ia; //ʱ϶����ʱ��
        Yc = SendStartOffset;
        Yd = SendDuration + Yc;
        /*-------------------------------//snidδ��ͻ������ʱ϶��ͻ���˱�------------------------------------*/

        if(abs(Ib-Yd)<10)//ͬʱ�������Ƚ�NID
        {
            if(GetSNID() > pFrameCtrl->snid )
            {
                changeFlag =TRUE;
                net_printf("---OV TG,bf");
            }
            else
            {
                net_printf("---OV TG,no bf");
            }
        }

        else if(Ia>Yc && Ia<Yd)
        {
            if(Ib < Yd) //c--------a-------b-----d
            {
                if(!NeighborNetCanHearMe(pFrameCtrl->snid, pFrameCtrl->neighbour))//�Ƚ��������Է��������Լ�
                {
                    changeFlag =TRUE;
                    net_printf("---NO H,bf\n");
                }
            }
            else if (Ib > Yd)//c--------a-------d-----b
            {
                changeFlag =TRUE;
                net_printf("---OV L1,bf");
            }

        }
        else if(Ib>Yc && Ib<Yd) //   c-------b------d
        {
            if(!NeighborNetCanHearMe(pFrameCtrl->snid, pFrameCtrl->neighbour))//�Ƚ��������Է��������Լ�
            {
                changeFlag =TRUE;
                net_printf("---NO H,bf\n");
            }
        }
        else if ( Ia<Yc && Ib>Yd) //a-----c----d----b
        {
            changeFlag =TRUE;
            net_printf("---OV L2,bf\n");
        }

        if(changeFlag == TRUE)
        {

            /*net_printf("----------------------MyStartOffset=0x%04x----------------------\n", MyStartOffset);
            net_printf("----------------------NbStartOffset=0x%04x----------------------\n",SendStartOffset);
            net_printf("----------------------NbDuratpoyion=0x%04x----------------------\n",SendDuration);
            net_printf("----------------------MyDuration=0x%04x----------------------\n",MyDuration);*/

            //���¿�ʼ�滮ʱ϶
//            modify_sendcenterbeacon_timer(Yd/10 +20);
            //ʱ϶�˱�
            BeaconStopOffset = PHY_MS2TICK(Yd-Ia+10);

//            if(!list_empty(&BEACON_LIST))
//            {
//                hplc_drain_beacon(FALSE);
//                packet_beacon_period(NULL);
//            }

            net_printf("---back time=0x%04x,for %08x-Offset=%08x\n"
                       ,(int)(Yd-Ia+200),pFrameCtrl->snid, BeaconStopOffset);
        }


        /*-------------------------------------------------------------------*/
    }
#endif    //#if defined(NEW_CCORD_ALG)
#if defined(STD_2016) || defined(STD_DUAL)
    U8 findSendIndex = 0xff;
    U8 findNbIndex = 0xff;
    U8 CurrChnl;
    CurrChnl = getHplcRfChannel();//WPHY_CHANNEL;//wphy_get_channel()+1;

    //�����ŵ�ռ�����.
    setRfChlHaveNetFlag(pFrameCtrl->rfoption, pFrameCtrl->rfchan_id);

    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(pFrameCtrl->snid == NeighborNet[i].SendSNID)//����Ѿ����ڣ����³�Ա
        {
            //�����ھӵ��ھ������
            for(j = 0; j < MaxSNIDnum; j++)
            {
                if(NeighborNet[i].NbSNID[j]	==	pFrameCtrl->neighbour)
                {
                    findNbIndex = 0xff;
                    break;
                }
                else if(NeighborNet[i].NbSNID[j] == 0xffffffff && findNbIndex == 0xff)
                {
                    findNbIndex = j;
                }
            }
            if(findNbIndex != 0xff)
            {
                NeighborNet[i].NbSNID[findNbIndex]	=	pFrameCtrl->neighbour;
            }
            NeighborNet[i].Duration = SendDuration;
            NeighborNet[i].SendStartOffset	= SendStartOffset;
            NeighborNet[i].MyStartOffset = MyStartOffset;
            NeighborNet[i].LifeTime = SNID_LIFE_TIME;
            if(pFrameCtrl->rfchan_id != NeighborNet[i].SendChannl || pFrameCtrl->rfoption != NeighborNet[i].SendOption)//�ھ�����׼���л��ŵ�
			{
				if(zc_timer_query(g_ChangeRfChannelTimer) == TMR_RUNNING)//Э������
				{
//					printf_s("neighour change channel, stop timer\n");
					timer_stop(g_ChangeRfChannelTimer, TMR_NULL);
					timer_delete(g_ChangeRfChannelTimer);
					g_ChangeRfChannelTimer = NULL;
				}
                NeighborNet[i].SendChannl = pFrameCtrl->rfchan_id;
                NeighborNet[i].SendOption = pFrameCtrl->rfoption;
            }
            else
            {
                //�Ƚ�����MAC��ַ
                if(0 == memcmp(BroadcastAddress, NeighborNet[i].CCOMacAddr, MACADRDR_LENGTH))
                {
                    net_printf("Nb mac eer!\n");
                    return ;
                }

                if(comcmpMac(nnib.MacAddr, NeighborNet[i].CCOMacAddr) == TRUE) //���ڵ�mac��ַ�ϴ�
                {
                    return;
                }

                if(getHplcOptin() == pFrameCtrl->rfoption && getHplcRfChannel() == pFrameCtrl->rfchan_id)
                {
                    StartRfChlChgeForCCO();
                }

            }
            return;
        }
        else if(NeighborNet[i].SendSNID == 0xffffffff && findSendIndex == 0xff)
        {
            findSendIndex = i;
        }
    }
    if(findSendIndex != 0xff)//
    {
        NeighborNet[findSendIndex].SendSNID = pFrameCtrl->snid;
        NeighborNet[findSendIndex].NbSNID[0] = pFrameCtrl->neighbour;
        NeighborNet[findSendIndex].Duration = SendDuration;
        NeighborNet[findSendIndex].SendStartOffset	= SendStartOffset;
        NeighborNet[findSendIndex].MyStartOffset = MyStartOffset;
        NeighborNet[findSendIndex].LifeTime = SNID_LIFE_TIME;
        NeighborNet[findSendIndex].SendChannl = pFrameCtrl->rfchan_id;
        NeighborNet[findSendIndex].SendOption = pFrameCtrl->rfoption;
        //nnib.NbSNIDnum++;

        if(pFrameCtrl->rfchan_id == CurrChnl && pFrameCtrl->rfoption == getHplcOptin())
		{
//            if(g_ChangeRfChannelTimer == NULL)
//            {
////				printf_s("rf channel confilct, start timer 15s\n");
//                g_ChangeRfChannelTimer = timer_create(15*1000,
//                                                      0,
//                                                      TMR_TRIGGER,
//                                                      ChangeChannelPlcTimerCB,
//                                                      NULL,
//                                                      "ChangeChannelPlcTimerCB");
////				timer_start(g_ChangeRfChannelTimer);
//                DstCoordRfChannel = 0xff;
//            }

//            CurrChnl = getHplcRfChannel();//WPHY_CHANNEL;//wphy_get_channel()+1;
//            StartRfChlChgeForCCO();


//            DstCoordRfChannel = i;


            //		timer_modify(g_ChangeRfChannelTimer,
            //					15*1000,
            //					0,
            //					TMR_TRIGGER,
            //					ChangeChannelPlcTimerCB,
            //					NULL,
            //					"ChangeChannelPlcTimerCB",
            //					TRUE);
            //		timer_start(g_ChangeRfChannelTimer);
        }
    }
#elif defined(STD_GD)

    NeighborNet[pFrameCtrl->snid-1].LifeTime = SNID_LIFE_TIME; //��ӻ��߸��·�������Э���ߵ�����ʱ��
    /*for(i = 0; i < MaxSNIDnum-1; i++)
    {
        if( 1<< i &  pFrameCtrl->neighbour)
        {
            NeighborNet[i].LifeTime = SNID_LIFE_TIME;
        }

    }*/
//    if(nnib.WorkState != e_WORK)
//        net_printf("----listen SNID =%d\n------",pCoord_Ind->SNID);
#endif



}

void ProcWlCoordIndication(mbuf_t *buf,void *pld, uint16_t len, uint8_t crc)
{
	rf_coord_ctrl_t *pFrameCtrl = (rf_coord_ctrl_t *)buf->fi.head;

	if(pFrameCtrl->dt != DT_COORD)
    {
        net_printf("ProcWlCoordIndication  dt Error! dt = %d\n", pFrameCtrl->dt);
        return;
    }
	U8 CurrChnl;

    CurrChnl = getHplcRfChannel();//WPHY_CHANNEL;//wphy_get_channel()+1;
	if(0)//NbrChannlΪ�����ھ��ŵ����
	{
		uint8_t i,j;
		U8 findSendIndex = 0xff;
	    U8 findNbIndex = 0xff;
		
        for(i = 0; i < MaxChannelNum; i++)
	    {
            if(CurrChnl==RfNbrNet[i].SendChannl
                    && 0==memcmp(pFrameCtrl->mac, RfNbrNet[i].CCOaddr, 6))//�����ھ��ŵ�
			{
                for(j = 0; j < MaxChannelNum; j++)
	        	{
                    if(pFrameCtrl->rfchan_id == RfNbrNet[i].NbrChannl[j])
	                {
	                    break;
	                }
                    else if(RfNbrNet[i].NbrChannl[j]==0xff && findNbIndex==0xff)
					{
						findNbIndex = j;
					}
	            }
                if(j == MaxChannelNum && findNbIndex != 0xff)
	            {
                    RfNbrNet[i].NbrChannl[findNbIndex]	=	pFrameCtrl->rfchan_id;
	            }
                RfNbrNet[i].LifeTime = SNID_LIFE_TIME;
	            return;
	        }
            else if(RfNbrNet[i].SendChannl == 0xff && findSendIndex == 0xff)
	        {
	            findSendIndex = i;
	        }
	    }
	    if(findSendIndex != 0xff)//
        {
            RfNbrNet[findSendIndex].SendChannl = CurrChnl;
            RfNbrNet[findSendIndex].NbrChannl[0] = pFrameCtrl->rfchan_id;
            __memcpy(RfNbrNet[findSendIndex].CCOaddr, pFrameCtrl->mac, 6);
            RfNbrNet[findSendIndex].LifeTime = SNID_LIFE_TIME;
	        nnib.NbSNIDnum++;
	        return;
	    }
	}
	else//NbrChannlΪ�����ŵ�,��������
	{
		uint8_t i;
	    uint8_t findNbIndex = 0xff;
//		printf_s("ProcWlCoordIndication\n");
        for(i = 0; i < MaxChannelNum; i++)
	    {
            if(0==memcmp(RfNbrNet[i].CCOaddr, pFrameCtrl->mac, 6))
	        {
                if(pFrameCtrl->rfchan_id == RfNbrNet[i].SendChannl)
				{
				}
				else//�ھ�����׼���л��ŵ�
				{
					if(zc_timer_query(g_ChangeRfChannelTimer) == TMR_RUNNING)//Э������
					{
                        RfNbrNet[i].SendChannl = pFrameCtrl->rfchan_id;
						timer_stop(g_ChangeRfChannelTimer, TMR_NULL);
						timer_delete(g_ChangeRfChannelTimer);
						g_ChangeRfChannelTimer = NULL;
					}
				}
				return;
	        }
            else if(RfNbrNet[i].SendChannl==0xff && findNbIndex==0xff)
			{
//				printf_s("--------findNbIndex\n");
				findNbIndex = i;
			}
	    }
//		printf_s("ProcWlCoordIndication i:%d\n",i);
        if(i==MaxChannelNum && findNbIndex!=0xff)
        {

            RfNbrNet[findNbIndex].SendChannl = CurrChnl;
            RfNbrNet[findNbIndex].SendOption = getHplcOptin();
            __memcpy(RfNbrNet[findNbIndex].CCOaddr, pFrameCtrl->mac, 6);
//            RfNbrNet[findNbIndex].NbrChannl[0] = pFrameCtrl->rfchan_id;
            RfNbrNet[findNbIndex].LifeTime = SNID_LIFE_TIME;
	        nnib.NbSNIDnum++;

            uint32_t  first;
            if(0 == memcmp(nnib.CCOMacAddr, pFrameCtrl->mac,6) || comcmpMac(nnib.CCOMacAddr, pFrameCtrl->mac) == 0)//������Mac��ַС
            {
                first = (rand() & 9) + 1;
            }
            else
            {
                first = 30*60;
            }
            printf_s("change rf channel first:%d\n",first);
            if(g_ChangeRfChannelTimer == NULL)
            {
                g_ChangeRfChannelTimer = timer_create(first*1000,
                                                      0,
                                                      TMR_TRIGGER,
                                                      ChangeRfChannelTimerCB,
                                                      NULL,
                                                      "ChangeRfChannelTimerCB");

                timer_start(g_ChangeRfChannelTimer);
            }
		}
	}
}

#endif

U8 SendDataUseBack(work_t *work)
{

    
    MACDATA_CONFIRM_t *cfm = (MACDATA_CONFIRM_t *)work->buf;

    if(NULL == cfm)
    {
        return FALSE;
    }

    if(NULL == cfm->MpduData)
    {
        return FALSE;
    }

    mbuf_hdr_t *pf = cfm->MpduData;

    tx_link_t *pListHeader = NULL;


    work_t *pCfm_work = zc_malloc_mem(sizeof(work_t) + sizeof(MACDATA_CONFIRM_t), "CFBT", MEM_TIME_OUT);
    __memcpy(pCfm_work, work, sizeof(MACDATA_CONFIRM_t)+sizeof(work_t));

    pf->buf->cnfrm = pCfm_work;
    MACDATA_CONFIRM_t *pCfm = (MACDATA_CONFIRM_t *)pCfm_work->buf;
    pCfm->dataLen = cfm->dataLen;
    pCfm->MpduData = NULL;
    // __memcpy(pCfm->data, cfm->data, cfm->dataLen);
    
    // pCfm->MpduData = NULL;

    // net_printf("pCfm->status:%d,dst:%03x\n", pCfm->status, pCfm->DstTei);

    // #if defined(STATIC_NODE)
    // if(pCfm->DstTei == CCO_TEI && pCfm->status == e_NO_ACK)   //��������
    // #else
    // if(pCfm->status == e_NO_ACK)   //CCO���г���ʹ�ò�ͬ��·����
    // #endif
    if(pCfm->status == e_NO_ACK)   //CCO���м�ڵ����г���ʹ�ñ���·�ɷ���
    {
        BACK_ROUTE_t NextRoute;
        U8  findindex = 0xff;
        U8 i;
        // if(pCfm->RouterMode == 0)
        // {
        //     NextRoute = &pCfm->BackRout[0];
        // }
        // else if(pCfm->RouterMode == 2)
        // {
        //     NextRoute = &pCfm->BackRout[1];
        // }
        // else
        // {
        //     goto free;
        // }

        memset(&NextRoute, 0xff, sizeof(BACK_ROUTE_t));

        for(i = 0; i < NELEMS(pCfm->BackRout); i++)
        {
            if(pCfm->BackRout[i].NextTei != BROADCAST_TEI)
            {
                __memcpy(&NextRoute, &pCfm->BackRout[i], sizeof(BACK_ROUTE_t));
                findindex = i;
                
                if(pCfm->BackRout[i].LinkType == pCfm->sendlinkType)    //����ʹ����·��ͬ�ı���·�ɡ�
                {
                    break;
                }
            }

        }

        if(findindex != 0xff)   //�Ѿ����Եı���·�ɣ���Ҫ��ա�
        {
            memset(&pCfm->BackRout[findindex], 0xff, sizeof(BACK_ROUTE_t));
        }

        if(NextRoute.NextTei != BROADCAST_TEI && findindex != 0xff)
        {
            pCfm->RouterMode = pCfm->RouterMode == 0?2:3;

            if(NextRoute.LinkType == pCfm->sendlinkType)   //ʹ�ñ���·�ɣ���·����û�䣬ֱ����ӡ�
            {
                pf->buf->retrans = pCfm->ReSendTimes ? pCfm->ReSendTimes : 3;           //�����ط�����
                ((sof_ctrl_t *)pf->buf->fi.head)->repeat = 0;                           //����ط�֡��־
                ((sof_ctrl_t *)pf->buf->fi.head)->dtei = NextRoute.NextTei;            //ʹ�ñ���·��

                if(tx_link_enq_by_lid(pf, pf->buf->fi.pi.link) < ENTERY_OK)
                {
                    net_printf("cfm tx error, link:%d!!!\n", NextRoute.LinkType);
                    zc_free_mem(pCfm_work);
                    return FALSE; 
                }
            }
            else                                            // ʹ�ñ���·�ɣ�������·���ͱ������Ҫ������֡��ӡ�
            {
                MAC_PUBLIC_HEADER_t *pMacdata  = NULL; 

                if(pCfm->sendlinkType == e_RF_Link)         // �л����ز���·���͡�
                {
                    pListHeader = &TX_DATA_LINK;
                    pMacdata = (MAC_PUBLIC_HEADER_t *)(pf->buf->fi.payload + sizeof(wpbh_t));
                    entry_tx_mng_list_data(pCfm->LinkId, pCfm->dataLen, (U8 *)pMacdata, NextRoute.NextTei, 
                    getTeiPhase(NextRoute.NextTei), pListHeader, pCfm_work);
                    
                }
                else                                        // �л���������·���ͣ����pb�� > 1 ��ʹ�����߱���·�ɷ��͡�
                {
                    pListHeader = &WL_TX_DATA_LINK;

                    sof_ctrl_t *fc = (sof_ctrl_t *)pf->buf->fi.head;

                    if(fc->pbc > 1)
                    {
                        net_printf("cfm tx error, pbc:%d!!!\n", fc->pbc);
                        zc_free_mem(pCfm_work);
                        return FALSE; 
                    }

                    pMacdata = (MAC_PUBLIC_HEADER_t *)(pf->buf->fi.payload + sizeof(pbh_t));

                    entry_rf_msg_list_data(pCfm->LinkId, pCfm->dataLen, (U8 *)pMacdata, NextRoute.NextTei,
                    pCfm->ReSendTimes, pListHeader, pCfm_work);

                }

                pCfm->sendlinkType = NextRoute.LinkType;
                zc_free_mem(pf);
            }

            // //����Ҫ�ж��Ƿ���ӳɹ���������ʧ�ܻ��ͷ�pCmf;
            // if(NextRoute->LinkType == e_RF_Link)     //����·��ʹ������ת��
            // {
            //     if(pCfm->MsduType == e_NETMMSG || pCfm->MsduType == e_RF_DISCOVER_LIST) 
            //     {
            //         pListHeader = &WL_TX_MNG_LINK;
            //     }
            //     else if(pCfm->MsduType == e_APS_FREAME || pCfm->MsduType == e_RF_APS_FREAME)
            //     {
            //         pListHeader = &WL_TX_DATA_LINK;
            //     }
            //     else
            //     {
            //         goto free;
            //     }

            //     entry_rf_msg_list_data(pCfm->LinkId, pCfm->dataLen, (U8 *)pCfm->data,NextRoute->NextTei,
            //     pCfm->ReSendTimes, pListHeader, pCfm_work);
            // }
            // else                                    //����·��ʹ���ز�ת��
            // {
            //     if(pCfm->MsduType == e_NETMMSG)
            //     {
            //         pListHeader = &TX_MNG_LINK;
            //     }
            //     else if(pCfm->MsduType == e_APS_FREAME)
            //     {
            //         pListHeader = &TX_DATA_LINK;
            //     }
            //     else
            //     {
            //         goto free;
            //     }
                
            //     entry_tx_mng_list_data(pCfm->LinkId, pCfm->dataLen, (U8 *)pCfm->data, NextRoute->NextTei, 
            //     getTeiPhase(NextRoute->NextTei), pListHeader, pCfm_work);
            // }
            
            net_printf("BackRt>>>next: %03x, lk:%d RtMd: %d\n", NextRoute.NextTei, NextRoute.LinkType, pCfm->RouterMode);

            return TRUE;
        }

    }
    zc_free_mem(pCfm_work);
    return FALSE;
}

///*
// CFM������㷢������жϡ�����ʱ����
//*/
void ProcMpduDataConfirm(work_t *work)
{

    MACDATA_CONFIRM_t  *pMacCfm;
    pMacCfm = (MACDATA_CONFIRM_t *)work->buf;

//    net_printf("%s : cmf addr = %p State = %02x\n",__func__, work, pMacCfm->status);

    /*net_printf("ProcMpduDataConfirm MsduHandle = %08x, MsduType= %d, Status = %d\n",
               pMacCfm->MsduHandle, pMacCfm->MsduType, pMacCfm->status);*/


    if(pMacCfm->MacVersion == e_MAC_VER_SINGLE)       //����֡
    {
        goto free;
        // return;
    }
//    else if(pMacCfm->MacVersion == e_MAC_VER_STANDARD) //��׼֡
//    {

//    }

    switch(pMacCfm->MsduType)//�ж�MSDU����
    {
    case e_NETMMSG://����������Ϣ
    {
		if(pMacCfm->MsduHandle == e_MMeDiscoverNodeList)
		{
			nnib.SendDiscoverListCount++;//���·��ʹ���
//            net_printf(">>>     Discover SendCnt %d\n", nnib.SendDiscoverListCount);
		}
        break;
    }
    case e_EXPAND://������·����չ
    {
        break;
    }
    case e_APS_FREAME://aps����
    {
        if(SendDataUseBack(work))
        {
            //�ط����ظ�confirm
            return;
        }

        //            pMacCfm->status == e_NO_ACK? usebackup_route(pMacCfm):
        //                                         MsduConfirmToApp(pMacCfm->ScrTei,pMacCfm->MsduHandle, pMacCfm->status);
        MsduConfirmToApp(pMacCfm->ScrTei,pMacCfm->MsduHandle, pMacCfm->status);

        break;
    }
    case e_IP_FREAME://��̫������
    {
        break;
    }

    }

    free:

    if(pMacCfm->MpduData)
        zc_free_mem(pMacCfm->MpduData);
}

//void ProBeaconConfirm(work_t *work)
//{
//	BEACON_CONFIRM_t *pMacBeaconConfirm_t = (BEACON_CONFIRM_t*)work->buf;
// 	net_printf("BeaconHandle %d send status is %d\n", pMacBeaconConfirm_t->BeaconHandle, pMacBeaconConfirm_t->status);


//}
/**
 * @brief               ѡ�񱸷�·�ɡ�
 * 
 * @param backRout      ����·������
 * @param lasttei       �Ѿ�ѡ���·��tei
 * @param linkAct       ������·����ѡ���Ӧ��·��·��
 * @return U16          ���ص�ǰѡ���·��tei
 */
U16 SearchBestBackRouteNode(BACK_ROUTE_t *backRout, U16 LASTTEI, U8 linkAct)
{
    U8	nodeDeep;
    list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    U8 null_addr[6] = {0};

    U16	UPLvsuccRatio=0;
    BACK_ROUTE_t UPLvRt;
    UPLvRt.NextTei = BROADCAST_TEI;

    U16	SLvsuccRatio=0;
    BACK_ROUTE_t SLvRt;
    SLvRt.NextTei = BROADCAST_TEI;

    U16	DepsuccRatio=0;
    BACK_ROUTE_t DeepRt;
    DeepRt.NextTei = BROADCAST_TEI;

    nodeDeep =  nnib.NodeLevel;

    if(backRout == NULL || IS_BROADCAST_TEI(LASTTEI))
        return BROADCAST_TEI;

    if(linkAct == e_HPLC_Link || linkAct == e_Dual_Link)
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->SNID != GetSNID() || 0 == memcmp(null_addr, DiscvryTable->MacAddr, 6))
                continue;

            if(DiscvryTable->HplcInfo.UplinkSuccRatio <= BACKUP_SUCC_RATIO)
                continue;

            if(GetProxy() != CCO_TEI && LASTTEI != CCO_TEI )
            {
                if(DiscvryTable->NodeTEI == CCO_TEI )
                {
                    backRout->NextTei  = DiscvryTable->NodeTEI;
                    backRout->LinkType = e_HPLC_Link;
                    return DiscvryTable->NodeTEI;
                }
            }

            //����͵ȼ���PCO��STA
            if(LASTTEI !=DiscvryTable->NodeTEI
                &&DiscvryTable->Relation == e_UPPERLEVEL)
            {
                if( DiscvryTable->NodeDepth < nodeDeep || 
                   (DiscvryTable->NodeDepth == nodeDeep && DiscvryTable->HplcInfo.UplinkSuccRatio > DepsuccRatio))
                {
                    nodeDeep = DiscvryTable->NodeDepth;
                    DeepRt.NextTei  = DiscvryTable->NodeTEI;
                    DeepRt.LinkType = e_HPLC_Link;
                    DepsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;
                }
            }

            //����һ��PCO��ͨ�ųɹ�����ߵ�
            if(LASTTEI !=DiscvryTable->NodeTEI
                    &&DiscvryTable->Relation == e_UPLEVEL)
            {
                if(	DiscvryTable->HplcInfo.UplinkSuccRatio> UPLvsuccRatio)
                {
                    UPLvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;
                    UPLvRt.NextTei  = DiscvryTable->NodeTEI;
                    UPLvRt.LinkType = e_HPLC_Link;
                }
            }

            //��ͬ��PCO��STA
            if(LASTTEI !=DiscvryTable->NodeTEI
                    && DiscvryTable->Relation == e_SAMELEVEL)
            {
                if(	DiscvryTable->HplcInfo.UplinkSuccRatio > SLvsuccRatio)
                {
                    SLvsuccRatio = DiscvryTable->HplcInfo.UplinkSuccRatio;
                    SLvRt.NextTei  = DiscvryTable->NodeTEI;
                    SLvRt.LinkType = e_HPLC_Link;
                }
            }
        }
    }

    if(linkAct == e_RF_Link || linkAct == e_Dual_Link)
    {
        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if(DiscvryTable->SNID != GetSNID() || 0 == memcmp(null_addr, DiscvryTable->MacAddr, 6))
                continue;

            if(DiscvryTable->HrfInfo.UpRcvRate <= BACKUP_SUCC_RATIO)
                continue;

            if(GetProxy() != CCO_TEI && LASTTEI != CCO_TEI )
            {
                if(DiscvryTable->NodeTEI == CCO_TEI)
                {
                    backRout->NextTei  = DiscvryTable->NodeTEI;
                    backRout->LinkType = e_RF_Link;
                    return DiscvryTable->NodeTEI;
                }
            }

            //����͵ȼ���PCO��STA
            if(LASTTEI !=DiscvryTable->NodeTEI
                &&DiscvryTable->Relation == e_UPPERLEVEL)
            {
                if( DiscvryTable->NodeDepth < nodeDeep || 
                  ( !IS_BROADCAST_TEI(DeepRt.NextTei)   && 
                    DeepRt.LinkType == e_RF_Link        &&
                    DiscvryTable->NodeDepth == nodeDeep && 
                    DiscvryTable->HrfInfo.UpRcvRate > DepsuccRatio))
                {
                    nodeDeep = DiscvryTable->NodeDepth;
                    DeepRt.NextTei  = DiscvryTable->NodeTEI;
                    DeepRt.LinkType = e_RF_Link;
                    DepsuccRatio = DiscvryTable->HrfInfo.UpRcvRate;
                }
            }

            //����һ��PCO��ͨ�ųɹ�����ߵ�
            if(LASTTEI !=DiscvryTable->NodeTEI
                    &&DiscvryTable->Relation == e_UPLEVEL)
            {
                if(	DiscvryTable->HrfInfo.UpRcvRate> UPLvsuccRatio)
                {
                    UPLvsuccRatio = DiscvryTable->HrfInfo.UpRcvRate;
                    UPLvRt.NextTei  = DiscvryTable->NodeTEI;
                    UPLvRt.LinkType = e_RF_Link;
                }
            }

            //��ͬ��PCO��STA
            if(LASTTEI !=DiscvryTable->NodeTEI
                    && DiscvryTable->Relation == e_SAMELEVEL)
            {
                if(	DiscvryTable->HrfInfo.UpRcvRate > SLvsuccRatio)
                {
                    SLvsuccRatio = DiscvryTable->HrfInfo.UpRcvRate;
                    SLvRt.NextTei  = DiscvryTable->NodeTEI;
                    SLvRt.LinkType = e_RF_Link;
                }
            }
        }
    }

    if(!IS_BROADCAST_TEI(DeepRt.NextTei))
    {
        backRout->NextTei  = DeepRt.NextTei;
        backRout->LinkType = DeepRt.LinkType;
        return DeepRt.NextTei;
    }
    if(!IS_BROADCAST_TEI(UPLvRt.NextTei))
    {
        backRout->NextTei  = UPLvRt.NextTei;
        backRout->LinkType = UPLvRt.LinkType;
        return UPLvRt.NextTei;
    }
    if(!IS_BROADCAST_TEI(SLvRt.NextTei))
    {
        backRout->NextTei  = SLvRt.NextTei;
        backRout->LinkType = SLvRt.LinkType;
        return SLvRt.NextTei;
    }



    return backRout->NextTei;
}

/**
 * @brief               �����һ���ڵ���·ͨ�ųɹ����Ƿ�����Ҫ��
 * 
 * @param nextTei       ��һ���ڵ�TEI
 * @param link          ��һ���ڵ���·���͡�
 * @return U8           ���÷���TRUE�� ��෵��FALSE
 */
static U8 check_route_link_uprate(U16 nextTei, U8 link)
{
    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    if(link == e_HPLC_Link)
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(DiscvryTable->NodeTEI == nextTei && DiscvryTable->HplcInfo.UplinkSuccRatio >= BACKUP_SUCC_RATIO)
                return TRUE;
        }
    }
    else
    {
        list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
            if(DiscvryTable->NodeTEI == nextTei && DiscvryTable->HrfInfo.UpRcvRate >= BACKUP_SUCC_RATIO)
                return TRUE;
        }
    }

    return FALSE;
}

void Search2BackRout(U16 destTei, U16 nextTei, U8 LinkType, BACK_ROUTE_t *backRout, U8 BackNum, U16 macLen)
{
    U8 index = 0;
    BACK_ROUTE_t backRoutTmp[2];
    memset(backRoutTmp, 0xff, sizeof(backRoutTmp));

    //net_printf("next:%03x, link:%d, macLen:%d\n", nextTei, LinkType, macLen);

    #if 0
    if(destTei == CCO_TEI)            //����������䱸��·��
    {
        //net_printf("BackRouteNum = %d\n", BackNum);
        for(i = 0; i < BackNum; i++)
        {
            if(tei == INVALID)
                break;

            backRout[i] = SearchBestBackupProxyNode(tei);
            net_printf("Sc2BR>tei=%03x,i=%d,bR[i]=%03x\n",tei, i, backRout[i]);
            tei = backRout[i];
        }
        dump_level_buf(DEBUG_MDL ,(U8 *)backRout, 4);
    }
    #else

    if(IS_BROADCAST_TEI(nextTei))     //�㲥�����ñ���·��
        return;

    #if defined(STATIC_NODE)
    
    U8 i;

    // if(destTei != CCO_TEI)          //Ŀ�Ĳ���cco�����ñ���·��
    //     return;
    if(destTei == CCO_TEI)             //ֻ������ѡ�񱸷�·�ɣ�����ֻ���Բ�ͬ��·
    {
        U16 tei = nextTei;
        //����������ʽѡ�񱸷�·����·���͡�
        U8 RouteLink =  nnib.NetworkType == e_SINGLE_HRF ? e_RF_Link : 
                        nnib.NetworkType == e_SINGLE_HPLC ? e_HPLC_Link :
                        nnib.NetworkType == e_DOUBLE_HPLC_HRF ? e_Dual_Link : e_Dual_Link;

        if(macLen > 512)
        {
            RouteLink = e_HPLC_Link;
        }

        //�Ȼ�ȡ��������·�ɡ�
        for(i = 0; i < BackNum; i++)
        {
            if(IS_BROADCAST_TEI(tei))
                break;
            tei = SearchBestBackRouteNode(&backRoutTmp[i], tei, RouteLink);
        }
    }
    #endif

    if(nnib.NetworkType == e_DOUBLE_HPLC_HRF)       //��������£������л���·
    {
        if(LinkType == e_RF_Link)
        {
            // if(NwkRoutingTable[nextTei -1].ActiveLk & (1 << e_HPLC_Link))   //����ʧ�ܣ�����ʹ���ز����͡�
            if(check_route_link_uprate(nextTei, e_HPLC_Link))        //ͨ�ųɹ�������Ҫ��
            {
                backRout[index].NextTei = nextTei;
                backRout[index++].LinkType = e_HPLC_Link;
            }
        }
        else if(LinkType == e_HPLC_Link && macLen <= 512)        //���Ȳ�����512���Գ���ʹ������·�ɷ���
        {
            // if(NwkRoutingTable[nextTei -1].ActiveLk & (1 << e_RF_Link))
            if(check_route_link_uprate(nextTei, e_RF_Link))         //ͨ�ųɹ�������Ҫ��
            {
                backRout[index].NextTei = nextTei;
                backRout[index++].LinkType = e_RF_Link;
            }
        }
    }

    //net_printf("back 1:%03x,%d\n", backRout[0].NextTei, backRout[0].LinkType);
    
    #if defined(STATIC_NODE)
    for(i = 0; i < BackNum; i++)
    {
        if(index >= BackNum)
            break;

        if(!IS_BROADCAST_TEI(backRoutTmp[i].NextTei))
        {
            __memcpy(&backRout[index++], &backRoutTmp[i], sizeof(BACK_ROUTE_t));
        }
    }
    // __memcpy(backRout, backRoutTmp, sizeof(backRoutTmp));
    #endif

    net_printf("bckRt:");
    dump_level_buf(DEBUG_MDL ,(U8 *)backRout, BackNum * sizeof(BACK_ROUTE_t));
    
    #endif

    return;
}

//void ProCoordConfirm(work_t *work)
//{
//    COORD_CONFIRM_t *pMacCoordConfirm_t=(COORD_CONFIRM_t*)work->buf;
//	net_printf("Coord send status is %d\n", pMacCoordConfirm_t->status);
//}

ostimer_t DelaysendTimer;
ostimer_t DelayRfsendTimer;

void DelaySendCB( struct ostimer_s *ostimer, void *arg)
{
	work_t *pwork;
	pwork = (work_t*)arg;
	MACDATA_CONFIRM_t *pCmf = (MACDATA_CONFIRM_t *)pwork->buf;
	net_printf("^^^Dlystm=%d\n",PHY_TICK2MS(get_phy_tick_time()) );
        
	MacDataRequst((MAC_PUBLIC_HEADER_t *)pCmf->data, pCmf->dataLen, pCmf->NextTei,
                        pCmf->SendType ,pCmf->MsduHandle, pCmf->LinkId, pCmf->MsduType, pCmf->ResendFlag,0,pCmf->RouterMode,0);
	zc_free_mem(arg);
}
U8 DelaySend( work_t *Cmf)
{
	MACDATA_CONFIRM_t *pCmf = (MACDATA_CONFIRM_t *)Cmf->buf;
	if(pCmf->DelayTime<10) //������ʱֱ�ӷ��͵�����
	{
		return FALSE;
	}
    if(DelaysendTimer.fn == NULL)
	{
		
        timer_init(&DelaysendTimer,
                   pCmf->DelayTime,
                   pCmf->DelayTime,
                   TMR_TRIGGER ,//TMR_TRIGGER
                   DelaySendCB,
                   Cmf,
                   "DelaySendCB"
                   );
        timer_start(&DelaysendTimer);
		return TRUE;
	}
	else
	{
        if(timer_query(&DelaysendTimer) == TMR_RUNNING) //��ʱ����ͬʱ��֧��һ��
		{
			return FALSE;
		}
		else
		{
            timer_modify(&DelaysendTimer,
            pCmf->DelayTime,
            pCmf->DelayTime,
			TMR_TRIGGER ,//TMR_TRIGGER
			DelaySendCB,
			Cmf,
            "DelaySendCB",
            TRUE);
            timer_start(&DelaysendTimer);
			return TRUE;
		}
	}
}

void DelayRfSendCB( struct ostimer_s *ostimer, void *arg)
{
	work_t *pwork;
	pwork = (work_t*)arg;
	MACDATA_CONFIRM_t *pCmf = (MACDATA_CONFIRM_t *)pwork->buf;
	net_printf("^RfDlystm=%d\n",PHY_TICK2MS(get_phy_tick_time()) );
        
	MacDataRequstRf((MAC_PUBLIC_HEADER_t *)pCmf->data, pCmf->dataLen, pCmf->NextTei,
                        pCmf->SendType ,pCmf->MsduHandle, pCmf->LinkId, pCmf->MsduType, pCmf->ResendFlag,0,pCmf->RouterMode);
	zc_free_mem(arg);
}
U8 DelayRfSend( work_t *Cmf)
{
	MACDATA_CONFIRM_t *pCmf = (MACDATA_CONFIRM_t *)Cmf->buf;
	if(pCmf->DelayTime<10) //������ʱֱ�ӷ��͵�����
	{
		return FALSE;
	}
    if(DelayRfsendTimer.fn == NULL)
	{
		
        timer_init(&DelayRfsendTimer,
                   pCmf->DelayTime,
                   pCmf->DelayTime,
                   TMR_TRIGGER ,//TMR_TRIGGER
                   DelayRfSendCB,
                   Cmf,
                   "DelayRfSendCB"
                   );
        timer_start(&DelayRfsendTimer);
		return TRUE;
	}
	else
	{
        if(timer_query(&DelayRfsendTimer) == TMR_RUNNING) //��ʱ����ͬʱ��֧��һ��
		{
			return FALSE;
		}
		else
		{
            timer_modify(&DelayRfsendTimer,
            pCmf->DelayTime,
            pCmf->DelayTime,
			TMR_TRIGGER ,//TMR_TRIGGER
			DelayRfSendCB,
			Cmf,
            "DelayRfSendCB",
            TRUE);
            timer_start(&DelayRfsendTimer);
			return TRUE;
		}
	}
}

void MacDataRequstRf(MAC_PUBLIC_HEADER_t *pMacData, U16 MacLen, U16 Nextei,
                             U8 SendType, U32 Handle, U8 LinkId, U8 MSDUType, U8 ResendFlag, U16 DelayTime,U8 RouterMode)
{
    work_t *cmf = zc_malloc_mem(sizeof(work_t) + sizeof(MACDATA_CONFIRM_t) +  (DelayTime==0?0:MacLen), "RCFMT", MEM_TIME_OUT);

    MACDATA_CONFIRM_t *pCmf = (MACDATA_CONFIRM_t *)cmf->buf;
    U8 RetransTimes = pMacData->MacProtocolVersion == e_MAC_VER_SINGLE? 0: pMacData->MaxResendTime;
    cmf->work = ProcMpduDataConfirm;
    pCmf->status            = 0;                    //mac���ṩ
    pCmf->ScrTei            = pMacData->MacProtocolVersion == e_MAC_VER_SINGLE? GetTEI(): pMacData->ScrTEI;
    pCmf->NextTei           = Nextei;
    pCmf->DstTei            = pMacData->MacProtocolVersion == e_MAC_VER_SINGLE? Nextei : pMacData->DestTEI;
    pCmf->MsduHandle        = Handle;
    pCmf->MsduType          = MSDUType;
    pCmf->RouterMode        = RouterMode;
    pCmf->ResendFlag        = ResendFlag;
    pCmf->SendType          = SendType;
    pCmf->ReSendTimes       = RetransTimes;
    pCmf->DelayTime         = DelayTime;
    pCmf->SliceSeq          = e_CSMA_SLICE;
    pCmf->BroadcastFlag 	= SendType;				  //��sof֡��������
    pCmf->LinkId            = LinkId;
    pCmf->dataLen           = 0;
    pCmf->MpduData          = NULL;
    pCmf->MacVersion      = pMacData->MacProtocolVersion;
    pCmf->sendlinkType      = e_RF_Link;
    memset(pCmf->BackRout, 0xff, sizeof(pCmf->BackRout));   //��ʼ������·��

    //���ݿ���,ʹ�ñ���·�ɷ���ʱʹ��
    pCmf->dataLen = MacLen;
   if(DelayTime)
    {
        // pCmf->dataLen = MacLen;
        __memcpy(pCmf->data, (U8 *)pMacData, MacLen);
    }
    if(DelayRfSend(cmf)) //��Ҫ��ʱʱ����ʱ����������
    {
        net_printf("Rf waiting send\n");
        return;
    }
    if(SendType == e_PROXY_BROADCAST_FREAM_NOACK || SendType == e_FULL_BROADCAST_FREAM_NOACK)
    {
        if(pMacData->MacProtocolVersion == e_MAC_VER_STANDARD)
            AddMsduSeq(pMacData->MsduSeq , pMacData->RestTims, GetTEI());
    }
    pCmf->BroadcastFlag 	= SendType == e_UNICAST_FREAM ? 0 : 1;					  //��sof֡��������

//    net_printf("%s : cmf addr = %p\n",__func__, cmf);


    if(MSDUType == e_NETMMSG || MSDUType == e_RF_DISCOVER_LIST)  //��Ҫ���͵Ĺ�����Ϣ������ӵ�������Ϣ������
    {
        #if defined(STATIC_NODE)
        if(DevicePib_t.PowerOffFlag != e_power_off_now) 
        #endif
            entry_rf_msg_list_data(LinkId, MacLen, (U8 *)pMacData,Nextei,RetransTimes, &WL_TX_MNG_LINK, cmf);
    }
    else if(MSDUType == e_APS_FREAME || MSDUType == e_RF_APS_FREAME)  //e_APS_FREAME
    {

        if(pMacData->MacProtocolVersion == e_MAC_VER_STANDARD)
        {
            if(GetApsPacketID((void*)pMacData)== 0x03)
            {
                //if( (Handle>>16) &0x03)
                reacordmeter(pMacData->MsduSeq,0,RouterMode,pMacData->DestTEI,Nextei);
            }
        }
        //��䱸��·��
        //<<< ��Ҫ������Ʊ���·�ɣ��������߲�ͨ��ʱ��ʹ���ز����͡��������ز���ͨ��ʱ��ʹ�����߷���
        Search2BackRout(pCmf->DstTei, pCmf->NextTei, e_RF_Link, pCmf->BackRout, NELEMS(pCmf->BackRout), MacLen);
        entry_rf_msg_list_data(LinkId, MacLen, (U8 *)pMacData, Nextei,RetransTimes,  &WL_TX_DATA_LINK, cmf);
    }

    return;
}

void MacDataRequst(MAC_PUBLIC_HEADER_t *pMacData, U16 MacLen, U16 Nextei,
                        U8 SendType, U32 Handle, U8 LinkId, U8 MSDUType, U8 ResendFlag, U16 DelayTime,U8 RouterMode,U8 DatalinkPhase)
{
    work_t *cmf = zc_malloc_mem(sizeof(work_t) + sizeof(MACDATA_CONFIRM_t) +  (DelayTime==0?0:MacLen), "CFMT", MEM_TIME_OUT);
	
    MACDATA_CONFIRM_t *pCmf = (MACDATA_CONFIRM_t *)cmf->buf;
    cmf->work = ProcMpduDataConfirm;
    pCmf->status            = 0;                    //mac���ṩ
    pCmf->ScrTei            = pMacData->ScrTEI;
    pCmf->NextTei           = Nextei;
    pCmf->DstTei            = pMacData->DestTEI;
    pCmf->MsduHandle        = Handle;
    pCmf->MsduType          = MSDUType;
    pCmf->RouterMode        = RouterMode;
    pCmf->ResendFlag        = ResendFlag;
    pCmf->SendType          = SendType;
    pCmf->ReSendTimes       = pMacData->MaxResendTime;
    pCmf->DelayTime         = DelayTime;
    pCmf->SliceSeq          = e_CSMA_SLICE;
	pCmf->BroadcastFlag 	= SendType;				  //��sof֡��������
    pCmf->LinkId            = LinkId;
    pCmf->dataLen           = 0;
    pCmf->MpduData          = NULL;
    pCmf->MacVersion      = pMacData->MacProtocolVersion;
    pCmf->sendlinkType      = e_HPLC_Link;
    memset(pCmf->BackRout, 0xff, sizeof(pCmf->BackRout));   //��ʼ������·��


    //���ݿ���,ʹ�ñ���·�ɷ���ʱʹ��
	pCmf->dataLen = MacLen;
	if(DelayTime)
	{
    	__memcpy(pCmf->data, (U8 *)pMacData, MacLen);	
	}
	if(DelaySend(cmf)) //��Ҫ��ʱʱ����ʱ����������
	{
		net_printf("waiting send\n");
		return;
	}
	if(SendType == e_PROXY_BROADCAST_FREAM_NOACK || SendType == e_FULL_BROADCAST_FREAM_NOACK)
	{
		AddMsduSeq(pMacData->MsduSeq , pMacData->RestTims, GetTEI());	
	}
	pCmf->BroadcastFlag 	= SendType == e_UNICAST_FREAM ? 0 : 1;					  //��sof֡��������

//    net_printf("%s : cmf addr = %p\n",__func__, cmf);


    if(MSDUType == e_NETMMSG)  //��Ҫ���͵Ĺ�����Ϣ������ӵ�������Ϣ������
    {
        #if defined(STATIC_NODE)
        if(DevicePib_t.PowerOffFlag != e_power_off_now) 
        #endif
            entry_tx_mng_list_data(LinkId, MacLen, (U8 *)pMacData,Nextei, DatalinkPhase!=0?DatalinkPhase:getTeiPhase(Nextei), &TX_MNG_LINK, cmf);
    }
    else if(MSDUType == e_APS_FREAME)  //e_APS_FREAME
    {
    	//if(GetApsPacketID((void*)pMacData)== 0x03)
		//if( (Handle>>16) &0x03)
    	//reacordmeter(pMacData->MsduSeq,0,RouterMode,pMacData->DestTEI,Nextei);
        //��䱸��·��
        Search2BackRout(pCmf->DstTei, pCmf->NextTei, e_HPLC_Link, pCmf->BackRout, NELEMS(pCmf->BackRout), MacLen);

        entry_tx_mng_list_data(LinkId, MacLen, (U8 *)pMacData, Nextei, DatalinkPhase!=0?DatalinkPhase:getTeiPhase(Nextei), &TX_DATA_LINK, cmf);
    }

    return;
}

void ProcLeaveIndRequset(work_t *work)
{
#if defined(STATIC_MASTER)
    LEAVE_IND_t *pLeaveInd = (LEAVE_IND_t *)work->buf;

    SendMMeDelayLeaveOfNum(pLeaveInd->pMac, pLeaveInd->StaNum, pLeaveInd->delayTime,pLeaveInd->DelType);
#endif
}

#if defined(STATIC_MASTER)
U32 getNbForCoord()
{
    static U8 LastNbSNID = 0;
    U32 NbNet = 0;
    U8 i;

#if defined(STD_2016) || defined(STD_DUAL)
    U8				nbSeq = 0;
    LastNbSNID = (LastNbSNID + 1 >= nnib.NbSNIDnum)?0:LastNbSNID + 1;
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(NeighborNet[i].SendSNID != 0xffffffff)
        {

            if(LastNbSNID == nbSeq)
            {
                NbNet = NeighborNet[i].SendSNID;
                break;
            }
            nbSeq++;
        }
    }
#elif defined(STD_GD)
    U16 SendNBNet =0;
    for(i=0;i<MaxSNIDnum-1;i++)
    {
        if(NeighborNet[i].LifeTime!=0xff)
        {
            SendNBNet +=	1<<i;
        }
    }
    NbNet = SendNBNet;

#endif

    return NbNet;
}
#endif

void CleanTimeSlotInd(work_t *work)
{
    // hplc_drain_beacon(TRUE);
    Clean_All_time_slot(TRUE);
	
#if STATIC_NODE
	extern void packet_csma_bpts(plc_work_t *mbuf_hdr);
    packet_csma_bpts(NULL);
#endif

}

void CleanTimeSlotReq()
{
    work_t *work= zc_malloc_mem(sizeof(work_t), "CLBN", MEM_TIME_OUT);
    work->work = CleanTimeSlotInd;
	work->msgtype = CLEAN_SOLT;
    post_datalink_task_work(work);
}



//mac to net msg
void link_layer_beacon_deal(mbuf_t *buf, void *pld, uint16_t len, uint8_t crc32, work_t *work)
{
    //�����ű�֡
    ProcBeaconIndication(buf, pld, len, crc32);
}
void link_layer_sof_deal(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc, work_t *work)
{
    MAC_RF_HEADER_t *pHeader = (MAC_RF_HEADER_t *)pld;
    if(pHeader->MacProtocolVersion == e_MAC_VER_SINGLE)             //����֡   ������֧��
    {
        ProcSigleHopMacIndication(buf, pld, len, crc,work);
    }
    else if(pHeader->MacProtocolVersion == e_MAC_VER_STANDARD)      //��׼֡   HPLC+RF
    {
        ProcMPDUDataIndication(buf, pld, len, crc,work);
    }
}
void link_layer_coord_deal(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc, work_t *work)
{	
#if defined(STATIC_MASTER)
	if(buf->LinkType == e_RF_Link)
    	ProcWlCoordIndication(buf, pld, len, crc);
	else if(buf->LinkType == e_HPLC_Link)
		ProcPhyCoordIndication(buf, pld, len, crc);
#elif defined(STATIC_NODE)
    if(buf->LinkType == e_HPLC_Link)
        ScanBandManage(e_RECEIVE, buf->snid);
    else
        ScanRfChMange(e_RECEIVE, buf->snid);

#endif
}
void link_layer_sack_deal(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc, work_t *work)
{
#if defined(STATIC_NODE)
    if(buf->LinkType == e_HPLC_Link)
        ScanBandManage(e_RECEIVE, buf->snid);
    else
        ScanRfChMange(e_RECEIVE, buf->snid);
#endif

        ProcSackDataIndication(buf, pld, len, crc);
}

