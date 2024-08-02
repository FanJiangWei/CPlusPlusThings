/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */


#include "dl_mgm_msg.h"
#include "version.h"

#include "mem_zc.h"
#include "printf_zc.h"

#include "plc_cnst.h"
#include "crc.h"

#include "SyncEntrynet.h"
#include "Beacon.h"
#include "DatalinkTask.h"
//#include "dev_manager.h"

//#include "app_common.h"

#include "ZCsystem.h"
#include "Datalinkdebug.h"
#include "app_slave_register_cco.h"
#include "app_off_grid.h"
#include "app_gw3762.h"
#include "app.h"
#include "Scanbandmange.h"
#include "app_area_change.h"
#include "SchTask.h"
#include "dev_manager.h"
#include "app_gw3762_ex.h"
#include "app_node_sgs_ex.h"
#include "app_deviceinfo_report.h"
#include "DataLinkGlobal.h"

#if defined(STD_DUAL)
#include "wl_cnst.h"
#include "wlc.h"
#include "wphy_isr.h"
#endif

VERSION_INFO_t   	VersionInfo ;

U16 		g_MsduSequenceNumber;

CHANGEReq_t ChangeReq_t;

ZERO_COLLECTIND_RESULT_t	Zero_Result;

U8				   BroadcastAddress[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
U8 				   ManageID[24];


U32 g_CnfAssociaRandomNum, g_CnfManageMsgSeqNum;

U8 netMsgRecordIndex = 0;


RF_DISC_NBINFOTYPELEN_t  g_RfNbInfoTypeLenGroup[RF_NBINFOTYPELEN_GROUP_COUNT] =
{
    {12, 8, 0, 0, 20},
    {12, 0, 4, 0, 16},
    {12, 0, 0, 8, 20},
    {12, 0, 0, 4, 16},
    {12, 8, 4, 0, 24},
    {12, 6, 0, 6, 24},
    {12, 0, 6, 6, 24},
    {12, 8, 6, 6, 32},
    {12, 0, 4, 4, 20},
    {12, 8, 0, 4, 24},
};

RF_DISC_NBINFOTYPELEN_MAP_t g_RfNbInfoTypeLenGroupMap[RF_NBINFOTYPELEN_MAP_GROUP_COUNT] =
{
    {8, 0, 0, 8},
    {0, 4, 0, 4},
    {0, 0, 8, 8},
    {0, 0, 4, 4},
    {8, 4, 0, 12},
    {8, 0, 8, 16},
    {0, 6, 6, 12},
    {8, 6, 6, 20},
    {8, 4, 4, 16},
    {0, 4, 4, 8},
    {8, 0, 4, 12},
};

extern phy_rxtx_cnt_t phy_rxtx_info;
extern phy_rxtx_cnt_t wphy_rxtx_info;

//����Ҫ���͵Ĺ�����Ϣ����TX_MNG_LINK������
void entry_tx_mng_list_data(U8 priority, U16 macLen, U8 *pMacLongHeader, U16 destTEI, U8 phase, tx_link_t *tx_list, work_t *cfm)
{
    mbuf_hdr_t *p;
    S32 ret;
	MAC_PUBLIC_HEADER_t *macheader=(MAC_PUBLIC_HEADER_t*)pMacLongHeader;

    if(phase > e_C_PHASE)  //��ֹ����Խ��0-all,1-a,2-b,3-c
        phase = e_UNKNOWN_PHASE;
	
    p = packet_sof(priority, destTEI, pMacLongHeader, macLen, macheader->MaxResendTime, 0,phase,cfm);

    if(p && !plc_can_not_send_message())
    {
//    	void msg_trace_record_func(U8 op ,tx_link_t *tx_link,U8 func);
//    	msg_trace_record_func(1,tx_list,5);
        if((ret = tx_link_enq_by_lid(p, tx_list)) < ENTERY_OK)
        {
            printf_s("t_mng %s ret:<%d>\n",(tx_list==&TX_DATA_LINK?"cd":"cm"), ret);
            
//            extern void phy_trace_show(tty_t *term);
//            phy_trace_show(g_vsh.term);

            if(cfm)
            {
                cfm->buf[0] = e_ENQUEUE_FAIL;   //���ض�������cfm
                cfm->msgtype = MAC_CFM;
                post_datalink_task_work(cfm);
            }
            zc_free_mem(p);
        }
        if(tx_list==&TX_DATA_LINK)
        {
            if(ret == -LIST_FULL)
            {
                phy_rxtx_info.tx_data_full ++;
            }
            else if(ret == ENTERY_OK)
            {
                phy_rxtx_info.tx_data_cnt ++;
            }
        }
        else
        {
            if(ret == -LIST_FULL)
            {
                phy_rxtx_info.tx_mng_full ++;
            }
            else if(ret == ENTERY_OK)
            {
                phy_rxtx_info.tx_mng_cnt ++;
            }
        }
    }
    else
    {
        if(NULL != cfm)
        {
            zc_free_mem(cfm);
        }
        if(NULL != p)
        {
            zc_free_mem(p);
        }
    }
}

void entry_tx_beacon_list_data(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, list_head_t *tx_list)
{

    mbuf_hdr_t *mbuf_hdr;

    mbuf_hdr = packet_beacon(stime, nnib.PossiblePhase, pMacLongHeader, macLen, bpc);

    //��������
    if(mbuf_hdr)
    {
        if(!list_empty(tx_list))
        {
            list_head_t *pos, *n;
            mbuf_hdr_t *node;
            list_for_each_safe(pos, n, tx_list)
            {
                if(pos != tx_list)
                {
                    node = list_entry(pos, mbuf_hdr_t, link);
                    list_del(pos);
                    zc_free_mem(node);
                }
            }
        }
        list_add_tail(&mbuf_hdr->link, tx_list);
    }
}
/**
 * @brief entry_rf_beacon_list_data
 * @param stime
 * @param macLen
 * @param pMacLongHeader
 * @param bpc
 * @param tx_list
 * ���߱�׼�űꡢ���߾����ű꣬���� WL_BEACON_FRAME_LIST
 * CSMAʱ϶���͵ľ����ű�֡������ WL_TX_MNG_LINK
 */
void entry_rf_beacon_list_data(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, list_head_t *tx_list)
{
    mbuf_hdr_t *mbuf_hdr;

    mbuf_hdr = packet_beacon_wl(stime, pMacLongHeader, macLen, bpc);

    if(mbuf_hdr)
    {
        if(tx_list == &WL_BEACON_FRAME_LIST)        //��ӵ��ű�����֤������ֻ��һ���������ű�
        {
            if(!list_empty(tx_list))
            {
                list_head_t *pos, *n;
                mbuf_hdr_t *node;
                list_for_each_safe(pos, n, tx_list)
                {
                    if(pos != tx_list)
                    {
                        node = list_entry(pos, mbuf_hdr_t, link);
                        list_del(pos);
                        zc_free_mem(node);
                    }
                }
            }
        }


        list_add_tail(&mbuf_hdr->link, tx_list);
    }
}
/**
 * @brief entry_rf_beacon_list_data_csma        ��csmaʱ϶���������ű�֡
 * @param stime
 * @param macLen
 * @param pMacLongHeader
 * @param bpc
 * @param tx_list
 */
void entry_rf_beacon_list_data_csma(U32 stime, U16 macLen, U8 *pMacLongHeader, U32 bpc, tx_link_t *tx_list)
{
    mbuf_hdr_t *mbuf_hdr;
    S32 ret;

    mbuf_hdr = packet_beacon_wl(stime, pMacLongHeader, macLen, bpc);

    if(mbuf_hdr)
    {
        if((ret = tx_link_enq_by_lid(mbuf_hdr, tx_list)) < ENTERY_OK)
        {
            printf_s("rf Bcn tx ret : <%d>\n", ret);
            zc_free_mem(mbuf_hdr);
        }
    }
}

void entry_rf_msg_list_data(U8 priority, U16 macLen, U8 *pMacLongHeader, U16 destTEI, U8 ResendTime,  tx_link_t *tx_list, work_t *cfm)
{
    mbuf_hdr_t *p;
    S32 ret;

    p = packet_sof_wl(priority, destTEI, pMacLongHeader, macLen, ResendTime, 0,cfm);

    if(p && !hrf_can_not_send_message())
    {
        if((ret = tx_link_enq_by_lid(p, tx_list)) < ENTERY_OK)
        {
            printf_s("t_mng %s ret:<%d>\n",(tx_list==&WL_TX_DATA_LINK?"rd":"rm"), ret);

            //void msg_trace_show(tty_t *term);
            //msg_trace_show(g_vsh.term);

            if(cfm)
            {
                cfm->buf[0] = e_ENQUEUE_FAIL;   //���ض�������cfm
                cfm->msgtype = MAC_CFM;
                post_datalink_task_work(cfm);
            }
            zc_free_mem(p);
        }
        if(tx_list==&WL_TX_DATA_LINK)
        {
            if(ret == -LIST_FULL)
            {
                wphy_rxtx_info.tx_data_full ++;
            }
            else if(ret == ENTERY_OK)
            {
                wphy_rxtx_info.tx_data_cnt ++;
            }
        }
        else
        {
            if(ret == -LIST_FULL)
            {
                wphy_rxtx_info.tx_mng_full ++;
            }
            else if(ret == ENTERY_OK)
            {
                wphy_rxtx_info.tx_mng_cnt ++;
            }
        }
    }
    else
    {
        if (NULL != cfm)
        {
            zc_free_mem(cfm);
        }

        if (NULL != p)
        {
            zc_free_mem(p);
        }
    }
}
#if defined(STATIC_MASTER)
void entry_tx_coord_data()
{
	mbuf_hdr_t *p;
	S32 ret;
	
    p = packet_coord_wl();

	if(p)
    {
        if((ret = tx_link_enq_by_lid(p, &WL_TX_MNG_LINK)) < ENTERY_OK)
        {
            printf_s("rf tx_coord ret : <%d>\n", ret);

            void msg_trace_show(tty_t *term);
            msg_trace_show(g_vsh.term);

            zc_free_mem(p);
        }
    }
}
#endif

/******************�����������ʹ��*****************/
ostimer_t updataAsscMsduTimer ;

void updataAsscMsdu()
{
    U8 i ,j;
    for(i=0;i<MAX_ASSCMSDU_NUM;i++)
    {
        if(INVALID!= AsscMsdu[i].msdu)
        {
            AsscMsdu[i].lifTime++;
            if(AsscMsdu[i].lifTime >=ASSC_MASKTIME)
            {
                memset((U8*)&AsscMsdu[i],0xff,sizeof(ZEROTEI_MSDU_t));
            }
        }

    }
    for(i=0;i<MAX_ASSCMSDU_NUM;i++)//���յ�λ�ã��޳�
    {
        if(INVALID == AsscMsdu[i].msdu)
        {

            for(j=i+1;j<MAX_ASSCMSDU_NUM;j++)
            {
                if(INVALID != AsscMsdu[j].msdu)
                {
                    __memcpy((U8*)&AsscMsdu[i],(U8*)&AsscMsdu[j],sizeof(ZEROTEI_MSDU_t));
                    memset((U8*)&AsscMsdu[j],0xff,sizeof(ZEROTEI_MSDU_t));
                    break;
                }
            }
        }
    }
    if(INVALID == AsscMsdu[0].msdu)
    {
        net_printf("AsscMsdu is empty, Stop Update \n");
        timer_stop(&updataAsscMsduTimer, TMR_NULL);
    }

    return;
}

static void updataAsscMsduTimerCB( struct ostimer_s *ostimer, void *arg)
{
    updataAsscMsdu();
}

static void modify_updataAsscMsduTimer(uint32_t first)
{
    if(updataAsscMsduTimer.fn == NULL)
    {
        timer_init(&updataAsscMsduTimer,
                   first,
                   first,
                   TMR_PERIODIC,//TMR_TRIGGER
                   updataAsscMsduTimerCB,
                   NULL,
                   "updataAsscMsduTimerCB"
                   );
    }
    else
    {
        timer_modify(&updataAsscMsduTimer,
                first,
                first,
                TMR_PERIODIC ,//TMR_TRIGGER
                updataAsscMsduTimerCB,
                NULL,
                "updataAsscMsduTimerCB",
                TRUE);
    }

    timer_start(&updataAsscMsduTimer);

}

void Add__ZEROTEI_msdu(U16 msdu, U8 *macAddr)
{
    U8 i;
    for(i=0;i<MAX_ASSCMSDU_NUM;i++)
    {
        if(INVALID== AsscMsdu[i].msdu && 0 == memcmp(BroadcastAddress, AsscMsdu[i].macAddr, MACADRDR_LENGTH))
        {

            AsscMsdu[i].msdu = msdu;
            __memcpy(AsscMsdu[i].macAddr, macAddr, MACADRDR_LENGTH);
            AsscMsdu[i].lifTime = 0;
            break;

        }
    }
    //λ����,first in first out
    if(i==MAX_ASSCMSDU_NUM)
    {
        __memmove((U8*)&AsscMsdu[0],(U8*)&AsscMsdu[1],(MAX_ASSCMSDU_NUM-1)*sizeof(ZEROTEI_MSDU_t));

        AsscMsdu[MAX_ASSCMSDU_NUM-1].msdu = msdu;
        __memcpy(AsscMsdu[MAX_ASSCMSDU_NUM-1].macAddr, macAddr, MACADRDR_LENGTH);
        AsscMsdu[MAX_ASSCMSDU_NUM-1].lifTime = 0;
    }
    if(timer_query(&updataAsscMsduTimer)==TMR_STOPPED )
    {
        modify_updataAsscMsduTimer(1000);
    }
    return;
}
/**
 * @brief check_ZEROTEI_msdu
 * @param msdu
 * @param macAddr
 * @return
 */
U8 check_ZEROTEI_msdu(U16 msdu, U8 *macAddr)
{
    U8 i;
    for(i = 0; i < MAX_ASSCMSDU_NUM; i++)
    {
        if(AsscMsdu[i].msdu == msdu && 0 == memcmp(AsscMsdu[i].macAddr, macAddr, MACADRDR_LENGTH))
        {
            AsscMsdu[i].lifTime = 0;
            return FALSE;
        }
    }

    return TRUE;
}

U8  filter_ZEROTEI_msdu(ZEROTEI_MSDU_t* pLocalmsud,U16 msdu,U16 Srctei)
{
	if(Srctei !=0)
	{
		return TRUE;
	}
	if(pLocalmsud->valid ==FALSE)
	{
		pLocalmsud->valid =TRUE;
        pLocalmsud->msdu = msdu;
		return TRUE;
	}
	else
	{
        if(pLocalmsud->msdu != msdu )
		{
            pLocalmsud->msdu = msdu;
			return TRUE;
		}
		net_printf("----filter_assreq_msdu----\n");
		return FALSE;
		
	}
}

/******************�����������ʹ��*****************/



#if defined(STATIC_NODE)
//�������ƣ����͹���������
/*
Reason :
*/
U8 SendAssociateReq(U16 DelayTime,U8 Reason)
{

    U16									msduLen, macLen;
    MAC_LONGDATA_HEADER    				*pMacLongHeader;
    ASSOCIATE_REQ_CMD_t  		        *pAssciateReq;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t 	DiscvryTable,*p;

    U8 MacAddr[6];


    pMacLongHeader = (MAC_LONGDATA_HEADER*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(ASSOCIATE_REQ_CMD_t)+ CRCLENGTH,"AssciateReq",MEM_TIME_OUT);
    pAssciateReq   = (ASSOCIATE_REQ_CMD_t*)pMacLongHeader->Msdu;

    pAssciateReq->MmsgHeader_t.MmsgType = e_MMeAssocReq;
    pAssciateReq->MmsgHeader_t.Reserve  = 0;

    if(Reason == e_NodeRouteLoss)
    {
        pAssciateReq->ParentTEI[0].ParatTEI = GetProxy();
        if( (p=GetNeighborByTEI(pAssciateReq->ParentTEI[0].ParatTEI, nnib.LinkType)) ==NULL)
        {
            zc_free_mem(pMacLongHeader);
            return FALSE;
        }
        else
        {
            __memcpy(&DiscvryTable,p,sizeof(ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t));
        }
    }
    else
    {
        if(ForAssociateReqSearchProxyNode((U16 *)pAssciateReq->ParentTEI,&DiscvryTable) ==FALSE)
        {
            net_printf("------ has't nice neighbor------\n");
            zc_free_mem(pMacLongHeader);
            return FALSE;
        }
    }

    __memcpy(pAssciateReq->ApplyNodeMacAddr, nnib.MacAddr, 6);

    net_printf("ParentTEI LinkeType:%d\n", pAssciateReq->ParentTEI[0].LinkType);

    if(pAssciateReq->ParentTEI[0].LinkType == e_HPLC_Link)
    {
        net_printf("NeighborDiscoveryTableEntry[neighborSeq].Phase=%d\n",DiscvryTable.Phase);
        if(DiscvryTable.Phase  !=e_A_PHASE &&
                DiscvryTable.Phase  !=e_B_PHASE &&
                DiscvryTable.Phase  !=e_C_PHASE)
        {
            net_printf("PossiblePhase err\n");
        }
        else
        {
            setPossiblePhase(DiscvryTable.Phase);
        }

    }
    __memcpy(MacAddr, DiscvryTable.MacAddr, 6);

    pAssciateReq->PhaseInfo1 = nnib.PossiblePhase;
    pAssciateReq->PhaseInfo2 = nnib.BackupPhase1;
    pAssciateReq->PhaseInfo3 = nnib.BackupPhase2;

    pAssciateReq->DeviceType = nnib.DeviceType;

    pAssciateReq->MacAddrType = nnib.MacAddrType;
    #ifdef HPLC_HRF_PLATFORM_TEST
    pAssciateReq->ModuleType =  nnib.ModuleType;
    #else
    pAssciateReq->ModuleType =  0;//nnib.ModuleType;
    #endif
    pAssciateReq->AssociaRandomNum = nnib.AssociaRandomNum;
    __memcpy((void *)&pAssciateReq->VersionInfo , (void *)&VersionInfo, sizeof(VERSION_INFO_t));
//#if defined(DATALINKDEBUG_SW)
    pAssciateReq->VersionInfo.ResetCause = 0;//Reason; //��¼ԭ��
//#endif
    pAssciateReq->HardwareResetCount = nnib.HardwareResetCount;
    pAssciateReq->SoftwareResetCount = nnib.SoftwareResetCount;
    pAssciateReq->ProxyType = e_DYNAMIC;

    pAssciateReq->ManageMsgSeqNum = nnib.ManageMsgSeqNum++;
    __memcpy(pAssciateReq->ManufactorInfo.HardwareVer ,DefSetInfo.Hardwarefeature_t.HardwareVer,sizeof(DefSetInfo.Hardwarefeature_t.HardwareVer));
#if defined(ZC3750STA)
    pAssciateReq->ManufactorInfo.InnerVer[0] = ZC3750STA_Innerver2;
    pAssciateReq->ManufactorInfo.InnerVer[1] = ZC3750STA_Innerver1;
#elif defined(ZC3750CJQ2)
    pAssciateReq->ManufactorInfo.InnerVer[0] = ZC3750CJQ2_Innerver2;
    pAssciateReq->ManufactorInfo.InnerVer[1] = ZC3750CJQ2_Innerver1;
#endif
    pAssciateReq->ManufactorInfo.InnerBTime = ((InnerDate_Y&0x7F)|((InnerDate_M<<7)&0x780)|( (InnerDate_D<<11)&0xF800));
    pAssciateReq->ManufactorInfo.edgetype = nnib.Edge;
    //��¼ģ�鸴λԭ��
    extern U32 RebootReason;
    pAssciateReq->ManufactorInfo.reboot_reason = RebootReason;
    __memcpy((void *)&pAssciateReq->ManageID , (void *)&ManageID, 24);

    msduLen = sizeof(ASSOCIATE_REQ_CMD_t);

    //�齨MAC֡ͷ
    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader
                       , msduLen
                       , MacAddr
                       , pAssciateReq->ParentTEI[0].ParatTEI
                       , e_UNICAST_FREAM
                       , NWK_MMEMSG_MAX_RETRIES, e_UNUSE_PROXYROUTE, 15, 15,e_TWO_WAY, TRUE);

    macLen = msduLen + sizeof(MAC_LONGDATA_HEADER) + CRCLENGTH;

//    net_printf("macLen :%d\n", macLen);
//    dump_level_buf(DEBUG_MDL, (U8 *)pMacLongHeader, macLen);

    if(pAssciateReq->ParentTEI[0].LinkType == e_HPLC_Link)
    {
        MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, pMacLongHeader->MacPublicHeader.DestTEI,
                      e_UNICAST_FREAM, e_MMeAssocReq, 1, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, pMacLongHeader->MacPublicHeader.DestTEI,
                        e_UNICAST_FREAM, e_MMeAssocReq, 1, e_NETMMSG, 0, 0, e_MainRoute);
    }

    zc_free_mem(pMacLongHeader);

    net_printf("------SendAssociateReq------MacAddr:0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n"
    ,MacAddr[0]
    ,MacAddr[1]
    ,MacAddr[2]
    ,MacAddr[3]
    ,MacAddr[4]
    ,MacAddr[5]);

    return TRUE;
}


void sendproxychangeDelay(U8 Reason)
{
	U16 TIME_S;

    if(g_WaitChageProxyCnf != NULL && TMR_RUNNING == zc_timer_query(g_WaitChageProxyCnf))
    {
        return;
    }
    TIME_S = rand()%nnib.RoutePeriodTime;
    modify_mgm_changesendtimer(TIME_S);
	net_printf("sendproxychangeDelay %d s\n",TIME_S);
	ChangeReq_t.RetryTime=0;
    ChangeReq_t.Reason = Reason;
}
//�������ƣ�����������
/*
Reason :ΪTRUE��ʾ�Ż���FASLE��ʾԭ�ȵĸ��ڵ��Ѿ�ʧЧ
*/
void SendProxyChangeReq(U8 Reason)
{
    U8                        linkid, DestMacAddr[6];
    U16                       msduLen, macLen;
    MAC_LONGDATA_HEADER       *pMacLongHeader;
    PROXYCHANGE_REQ_CMD_t     *pProxyChgeReq;

    U8                         OldProxyLevel,NewProxyLevel;



    pMacLongHeader = (MAC_LONGDATA_HEADER*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(PROXYCHANGE_REQ_CMD_t) + CRCLENGTH,"ProxyChgeReq",MEM_TIME_OUT);
    pProxyChgeReq   = (PROXYCHANGE_REQ_CMD_t*)pMacLongHeader->Msdu;

    pProxyChgeReq->MmsgHeader_t.MmsgType = e_MMeChangeProxyReq;
    pProxyChgeReq->MmsgHeader_t.Reserve  = 0;

    linkid = 1;
    net_printf("Reason=%d\n",Reason);

    /*for(i = 0; i < MAX_NEIGHBOR_NUMBER; i++)//��ӡ
    {
        if(NeighborDiscoveryTableEntry[i].NodeTEI !=INVALID)
            net_printf("%02x%02x%02x%02x%02x%02x\t%04x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\n",NeighborDiscoveryTableEntry[i].MacAddr[0],
                    NeighborDiscoveryTableEntry[i].MacAddr[1],
                    NeighborDiscoveryTableEntry[i].MacAddr[2],
                    NeighborDiscoveryTableEntry[i].MacAddr[3],
                    NeighborDiscoveryTableEntry[i].MacAddr[4],
                    NeighborDiscoveryTableEntry[i].MacAddr[5],
                    NeighborDiscoveryTableEntry[i].NodeTEI,
                    NeighborDiscoveryTableEntry[i].NodeType,
                    NeighborDiscoveryTableEntry[i].NodeDepth,
                    NeighborDiscoveryTableEntry[i].Relation,
                    NeighborDiscoveryTableEntry[i].UplinkSuccRatio,
                    NeighborDiscoveryTableEntry[i].DownlinkSuccRatio);
    }
	net_printf("NEIGHBOR_NUMBER=%d",i);*/
    if(ChangeReq_t.Reason == e_KeepSyncCCO || ChangeReq_t.Reason == e_RouteLoss)//�����ڱ��ʧ�ܣ�֪ͨCCO�Լ�����ԭ���ĸ��ڵ�����
    {
        pProxyChgeReq->NewProxyTei[0].ParatTEI = GetProxy();
        pProxyChgeReq->NewProxyTei[0].LinkType = nnib.LinkType;
    }
    else
    {
//recordtime("Req0",get_phy_tick_time());

        if(SearchFiveBackupProxyNodeAtEvaluate(pProxyChgeReq->NewProxyTei,Reason, sizeof(pProxyChgeReq->NewProxyTei)) == 0)
        {
            net_printf("NoBckPrxy,Rs:%d,Rst:%d\n", ChangeReq_t.Reason, ChangeReq_t.Result);
            if(ChangeReq_t.Result == FALSE && ChangeReq_t.Reason == e_FindBetterProxy)    //������ִ�����û��ѡ�񵽺��ʵĽڵ㣬�����ϸ�·�����ڱ��ʧ�ܣ�����һ��e_KeepSyncCCO
            {
                pProxyChgeReq->NewProxyTei[0].ParatTEI = GetProxy();
                pProxyChgeReq->NewProxyTei[0].LinkType = nnib.LinkType;
                ChangeReq_t.Reason = e_KeepSyncCCO;     //�ص��п��Ʋ��ط�
            }
            else
            {
                if(e_LevelToHigh == Reason )
                {
                    ClearNNIB();
                }
                else if(e_FialToTry == Reason )  //�޿��ñ�ѡ����������ǰ���������̣��ñ��ʧ�ܱ�־
                {
                    ChangeReq_t.Result =FALSE;
                }
                zc_free_mem(pMacLongHeader);
                return;
            }
            
        }

        //�븸�ڵ��л���·���ͣ�Ҳ��Ҫ���������          ����ѡ��ʱ�Ѿ����ˡ�
        // if(pProxyChgeReq->NewProxyTei[0].ParatTEI == GetProxy() && pProxyChgeReq->NewProxyTei[0].LinkType == nnib.LinkType)
		// {
        //     if(e_FialToTry == Reason )
        //     {
        //         ChangeReq_t.Result =FALSE;
        //     }
		// 	zc_free_mem(pMacLongHeader);
        //     return;
		// }
    }
//recordtime("Req1",get_phy_tick_time());

    //ɾ��֮ǰ�Ĵ���ڵ���ھӱ�
    OldProxyLevel = nnib.NodeLevel-1;//SearchDepthInNeighborDiscoveryTableEntry(GetProxy());
    NewProxyLevel =	SearchDepthInNeighborDiscoveryTableEntry(pProxyChgeReq->NewProxyTei[0].ParatTEI, pProxyChgeReq->NewProxyTei[0].LinkType);
    net_printf("lv=%d,OPT=%02x,OPL=%d,NPT=%03x,NPL=%d\n",nnib.NodeLevel,GetProxy(),OldProxyLevel,pProxyChgeReq->NewProxyTei[0].ParatTEI,NewProxyLevel);

    /*if(pProxyChgeReq->NewProxyTei[0] == GetProxy())
    {
        Reason =e_ElftoElf;
    }*/
//recordtime("Req2",get_phy_tick_time());

    pProxyChgeReq->StaTEI = GetTEI();
    pProxyChgeReq->OldProxyTei = GetProxy();
    pProxyChgeReq->ProxyType = e_DYNAMIC;

    pProxyChgeReq->PhaseInfo1 = nnib.PossiblePhase;
    pProxyChgeReq->PhaseInfo2 = nnib.BackupPhase1;
    pProxyChgeReq->PhaseInfo3 = nnib.BackupPhase2;

    pProxyChgeReq->ChangeCause = 0;//Reason;
    pProxyChgeReq->ManageMsgSeqNum = ++nnib.ManageMsgSeqNum;

    //�齨MAC֡ͷ
    msduLen = sizeof(PROXYCHANGE_REQ_CMD_t);

    SearchMacInNeighborDisEntrybyTEI_SNID(pProxyChgeReq->NewProxyTei[0].ParatTEI,GetSNID(), DestMacAddr);
//recordtime("Req3",get_phy_tick_time());

    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader, msduLen, DestMacAddr, pProxyChgeReq->NewProxyTei[0].ParatTEI,
            e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 15, 15,e_STA_CCO, TRUE);

    macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;


    if(pProxyChgeReq->NewProxyTei[0].LinkType == e_HPLC_Link)
    {
        MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, pMacLongHeader->MacPublicHeader.DestTEI,
                      e_UNICAST_FREAM, e_MMeChangeProxyReq, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, pMacLongHeader->MacPublicHeader.DestTEI,
                      e_UNICAST_FREAM, e_MMeChangeProxyReq, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*
        LongMacDataRequst(macLen, pMacData, pMacData->DestTEI, e_UNICAST_FREAM,
                          e_MMeChangeProxyReq, linkid, e_NETMMSG, 0, DelayTime);*/


    zc_free_mem(pMacLongHeader);

    modify_mgm_changesendtimer(5);
//recordtime("Req4",get_phy_tick_time());

}



/**********************************************************************
Function: 			����������ⱨ��
Description:		ֻ��PCO�����ӽڵ�ȫ��ΪSTA�Ľڵ㷢��������
Input:				Mac��Indictionָ��
Output:				��
Return:  			��
Other:
Flag: FlagΪTRUE��ʾ

***********************************************************************/

__SLOWTEXT void SendHeartBeatCheck(work_t * work)
{
    U8                         linkid;
    U16                        macLen, msduLen;
    MAC_LONGDATA_HEADER       *pMacLongHeader;
    HEART_BEAT_CHECK_CMD_t    *pHeartBeatCheckReq;


    pMacLongHeader = (MAC_LONGDATA_HEADER*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(HEART_BEAT_CHECK_CMD_t)+256+CRCLENGTH,
                                                                                      "HeartBeatCheckReq",MEM_TIME_OUT);
    pHeartBeatCheckReq   = (HEART_BEAT_CHECK_CMD_t*)pMacLongHeader->Msdu;
	

    pHeartBeatCheckReq->MmsgHeader_t.MmsgType = e_MMeHeartBeatCheck;
    pHeartBeatCheckReq->MmsgHeader_t.Reserve  = 0;
    linkid	= 1;

    pHeartBeatCheckReq->SrcTEI  = GetTEI();
    pHeartBeatCheckReq->MostTEI = GetTEI();
    pHeartBeatCheckReq->MaxiNum = GetNeighborBmp(pHeartBeatCheckReq->TEIBmp, &pHeartBeatCheckReq->BmpNumber);
//    pHeartBeatCheckReq->BmpNumber = GetNeighborBmpByteNum();

    msduLen = sizeof(HEART_BEAT_CHECK_CMD_t) + pHeartBeatCheckReq->BmpNumber;

    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader, msduLen, nnib.ParentMacAddr, GetProxy(),
                       e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE,
                       NET_RELAY_LEVEL_MAX, NET_RELAY_LEVEL_MAX,e_TWO_WAY, TRUE);
    macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;


    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeHeartBeatCheck, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeHeartBeatCheck, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }
    /*
    LongMacDataRequst(macLen, pMacData, GetProxy(), e_UNICAST_FREAM,
                      e_MMeHeartBeatCheck, linkid, e_NETMMSG, 0, 0);*/
	net_printf("%%%%%%%%SendHeartBeatCheck\n");
    zc_free_mem(pMacLongHeader);

 

}



__SLOWTEXT void SendSuccessRateReport(work_t * work)
{
    U8                          linkid;
    U16                         macLen, msduLen;
    MAC_LONGDATA_HEADER         *pMacLongHeader;
    SUCCESSRATE_REPORT_CMD_t  	*pSuccRateReport;
    SUCCESSRATE_INFO_t          SuccessInfo_t;

    list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    if(nnib.NodeType !=e_PCO_NODETYPE)
    {
    	net_printf("not pco\n");
        return;
    }

    net_printf("Send SucceRateReport\n");

    pMacLongHeader = (MAC_LONGDATA_HEADER*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(SUCCESSRATE_REPORT_CMD_t)+1024 + CRCLENGTH,
                                                                                      "SuccRateReport",MEM_TIME_OUT);
    pSuccRateReport = (SUCCESSRATE_REPORT_CMD_t*)pMacLongHeader->Msdu;

    pSuccRateReport->MmsgHeader_t.MmsgType = e_MMeSuccessRateReport;
    pSuccRateReport->MmsgHeader_t.Reserve  = 0;
    linkid	= 1;

    pSuccRateReport->NodeTEI = GetTEI();

    
    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)//����CCO
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(DiscvryTable->Relation == e_CHILD && NwkRoutingTable[DiscvryTable->NodeTEI-1].LinkType == e_HPLC_Link)
        {
            SuccessInfo_t.ChildTEI = DiscvryTable->NodeTEI;
            SuccessInfo_t.DownCommRate = DiscvryTable->childDownRatio;
            SuccessInfo_t.UpCommRate = DiscvryTable->childUpRatio;
            __memcpy(&pSuccRateReport->ChildInfo[pSuccRateReport->NodeCount * sizeof(SUCCESSRATE_INFO_t)],
                   &SuccessInfo_t, sizeof(SUCCESSRATE_INFO_t) );
            pSuccRateReport->NodeCount++;

            if(pSuccRateReport->NodeCount > 256)
            {
                break;
            }
        }
    }

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)//����CCO
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if(pSuccRateReport->NodeCount > 256)
        {
            break;
        }

        if(DiscvryTable->Relation == e_CHILD && NwkRoutingTable[DiscvryTable->NodeTEI-1].LinkType == e_RF_Link)
        {
            SuccessInfo_t.ChildTEI = DiscvryTable->NodeTEI;
            SuccessInfo_t.DownCommRate = DiscvryTable->childDownRatio;
            SuccessInfo_t.UpCommRate = DiscvryTable->childUpRatio;
            __memcpy(&pSuccRateReport->ChildInfo[pSuccRateReport->NodeCount * sizeof(SUCCESSRATE_INFO_t)],
                   &SuccessInfo_t, sizeof(SUCCESSRATE_INFO_t) );
            pSuccRateReport->NodeCount++;
        }
    }
    
    msduLen = sizeof(SUCCESSRATE_REPORT_CMD_t) + pSuccRateReport->NodeCount * sizeof(SUCCESSRATE_INFO_t);

    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader, msduLen, nnib.CCOMacAddr, CCO_TEI,
                       e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE,
                       NET_RELAY_LEVEL_MAX, NET_RELAY_LEVEL_MAX,e_TWO_WAY, TRUE);
    macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeSuccessRateReport, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeSuccessRateReport, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*LongMacDataRequst(macLen, pMacData, GetProxy(), e_UNICAST_FREAM,
                      e_MMeSuccessRateReport, linkid, e_NETMMSG, 0, 0);*/

    zc_free_mem(pMacLongHeader);

    return;
}

//����⵽��ͻʱ��ÿ���ϱ�һ���ھ��������Ϣ
__SLOWTEXT void SendNetworkConflictReport(U8 *pMacAddr, U16 DelayTime, U8 MacUse)
{
    U8							NeighborNetSNID[3];
    U8                          linkid;
    U8                          i;
    U16							macLen, msduLen;
    MAC_PUBLIC_HEADER_t    		*pMacPublicHeader;
    NETWORK_CONFLIC_REPORT_t	*pNetConflicReport;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(NETWORK_CONFLIC_REPORT_t)
                                                           +sizeof(NeighborNetSNID)*MaxSNIDnum + CRCLENGTH, "NetConflicReport",MEM_TIME_OUT);

    if(MacUse)
    {
        pNetConflicReport = (NETWORK_CONFLIC_REPORT_t *)((U8*)pMacPublicHeader+sizeof(MAC_LONGDATA_HEADER));
    }
    else
    {
        pNetConflicReport = (NETWORK_CONFLIC_REPORT_t *)pMacPublicHeader->Msdu;
    }

    linkid = 1;
    pNetConflicReport->MmsgHeader_t.MmsgType = e_MMeNetworkConflictReport;
    pNetConflicReport->MmsgHeader_t.Reserve  = 0;

    __memcpy(pNetConflicReport->CCOAddr, pMacAddr, 6);
    pNetConflicReport->NBnetNumber = 0;
    pNetConflicReport->NIDWidth = 3;

    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(StaNeigborNet_t[i].NbSNID != 0xffffffff)
        {
            NeighborNetSNID[0] = StaNeigborNet_t[i].NbSNID;
            NeighborNetSNID[1] = StaNeigborNet_t[i].NbSNID >> 8;
            NeighborNetSNID[2] = StaNeigborNet_t[i].NbSNID >> 16;
            __memcpy(&pNetConflicReport->NBnetList[3 * (pNetConflicReport->NBnetNumber++)], NeighborNetSNID, 3);
        }
    }

    msduLen = sizeof(NETWORK_CONFLIC_REPORT_t) + 3 * (pNetConflicReport->NBnetNumber);

    CreatMMsgMacHeader(pMacPublicHeader, msduLen, nnib.CCOMacAddr, GetProxy(),
                       e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, nnib.NodeLevel, nnib.NodeLevel,e_STA_CCO, MacUse);

    if(MacUse)
    {
        macLen = sizeof(MAC_LONGDATA_HEADER) + msduLen + CRCLENGTH;
    }
    else
    {
        macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;

    }

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeNetworkConflictReport, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(),
                        e_UNICAST_FREAM, e_MMeNetworkConflictReport, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*LongMacDataRequst(macLen, pMacData, nnib.ParentTEI , e_UNICAST_FREAM,
                      e_MMeNetworkConflictReport, linkid, e_NETMMSG, 0, DelayTime);*/

    zc_free_mem(pMacPublicHeader);

}

void SendMMeZeroCrossNTBReport(U8 MacUse)
{
    U8                          linkid;
    U8                          i;
    U8                          listLen=0;
    U8                          listSeq=0;
    U16                         macLen, msduLen;
    U16                         Afirstpoint;
#if defined(ZC3750STA)
    U16                         Bfirstpoint,Cfirstpoint;
#endif
    MAC_PUBLIC_HEADER_t 		*pMacPublicHeader;
    ZERO_CROSSNTB_REPORT_t		*pZeroReport;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(ZERO_CROSSNTB_REPORT_t)
                                                           + Zero_Result.ColloctNumber*4 + CRCLENGTH, "ZeroNTBReport",MEM_TIME_OUT);
    if(MacUse)
    {
        pZeroReport = (ZERO_CROSSNTB_REPORT_t*)((U8*)pMacPublicHeader+sizeof(MAC_LONGDATA_HEADER));
    }
    else
    {
        pZeroReport = (ZERO_CROSSNTB_REPORT_t*)pMacPublicHeader->Msdu;
    }

    Zero_Result.ColloctResult[0] =0;

    linkid = 1;
    pZeroReport->MmsgHeader_t.MmsgType = e_MMeZeroCrossNTBReport;
    pZeroReport->MmsgHeader_t.Reserve  = 0;

    pZeroReport->NodeTEI = GetTEI();
    pZeroReport->ColloctNumber = Zero_Result.ColloctNumber;

    app_printf("nnib.PowerType = %d\n",nnib.PowerType);

    if(nnib.PowerType == FALSE)//���㻷����Ҫ�ظ�
    {
        pZeroReport->FirstNTB = 0;
        pZeroReport->ColloctNumber =0;
        pZeroReport->Phase1Num = 0;
        pZeroReport->Phase2Num = 0;
        pZeroReport->Phase3Num = 0;
    }
    else
    {
        if(nnib.DeviceType != e_3PMETER)
        {
            pZeroReport->Phase1Num = Zero_Result.ColloctNumber;
            pZeroReport->Phase2Num = 0;
            pZeroReport->Phase3Num = 0;

            if(ZeroData[e_A_PHASE-1].NewOffset> (pZeroReport->Phase1Num+1))
            {
                Afirstpoint = ZeroData[e_A_PHASE-1].NewOffset - (pZeroReport->Phase1Num+1);
            }
            else
            {
                Afirstpoint = MAXNTBCOUNT- (pZeroReport->Phase1Num+1) + ZeroData[e_A_PHASE-1].NewOffset;
            }

            Zero_Result.ColloctResult[0] = ZeroData[e_A_PHASE-1].NTB[Afirstpoint++];
            pZeroReport->FirstNTB = Zero_Result.ColloctResult[0]>>8;
            for(i=0;i<pZeroReport->Phase1Num;)
            {
                if(Zero_Result.ColloctPeriod == e_HalfPeriod )
                {
                    Zero_Result.ColloctResult[i+1] = ZeroData[e_A_PHASE-1].NTB[Afirstpoint] -10*STAMPUNIT;
                    i++;
                }

                Zero_Result.ColloctResult[i+1] = ZeroData[e_A_PHASE-1].NTB[Afirstpoint];
                i++;
                Afirstpoint = (Afirstpoint+1)%MAXNTBCOUNT;
            }

            listLen = CreatZeroDifflist(Zero_Result.ColloctNumber, &listSeq, pZeroReport->NTBdiff,Zero_Result.ColloctResult[0],1);
        }
        else //�������
        {
            U8  bitNum = 0;
            U8  remainder = 0;
			if(Zero_Result.ColloctNumber>0 && (nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC)>0)
			{
				
			   bitNum = ((Zero_Result.ColloctNumber -1)/(nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC));
			   remainder = ((Zero_Result.ColloctNumber -1)%(nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC));
			}
 
            if(nnib.PhasebitA)
            {
                pZeroReport->Phase1Num = bitNum + (remainder>0?0:1);
                //remainder = (remainder>0?remainder-1:0);
            }
            if(nnib.PhasebitB)
            {
                pZeroReport->Phase2Num = bitNum + (remainder>0?1:0);
                remainder = (remainder>0?remainder-1:0);
            }
            if(nnib.PhasebitC)
            {
                pZeroReport->Phase3Num = bitNum + (remainder>0?1:0);
            }
            app_printf("1pZeroReport_t->Phase1Num=%d,2=%d,3=%d!\n",
                pZeroReport->Phase1Num,pZeroReport->Phase2Num,pZeroReport->Phase3Num);



            //��׼NTBȡ��Ч�Ľ�������
            if(nnib.PhasebitA)
            {

                if(ZeroData[e_A_PHASE-1].NewOffset> (pZeroReport->Phase1Num+1))
                {
                    Afirstpoint = ZeroData[e_A_PHASE-1].NewOffset - (pZeroReport->Phase1Num+1);
                }
                else
                {
                    Afirstpoint =MAXNTBCOUNT- (pZeroReport->Phase1Num+1) +ZeroData[e_A_PHASE-1].NewOffset;
                }
                Zero_Result.ColloctResult[0] = ZeroData[e_A_PHASE-1].NTB[Afirstpoint++];
                for(i=0;i<pZeroReport->Phase1Num;)
                {
                    if(Zero_Result.ColloctPeriod == e_HalfPeriod )
                    {
                        Zero_Result.ColloctResult[i +1] = ZeroData[e_A_PHASE-1].NTB[Afirstpoint]- 10*STAMPUNIT;
                        i++;
                    }
                    Zero_Result.ColloctResult[i +1] = ZeroData[e_A_PHASE-1].NTB[Afirstpoint];
                    i++;

                    Afirstpoint = (Afirstpoint+1)%MAXNTBCOUNT;
                }
                listLen= CreatZeroDifflist(pZeroReport->Phase1Num, &listSeq ,pZeroReport->NTBdiff,
                                            Zero_Result.ColloctResult[0],1);
                app_printf("1listSeq = %d,listLen = %d!\n",listSeq,listLen);
                //listSeq += Zero_Result.ColloctNumber;
                //listLen +=perlen;
            }
            else
            {
                pZeroReport->Phase1Num =0;
            }
#if defined(ZC3750STA)
            if(nnib.PhasebitB)
            {

                if(ZeroData[e_B_PHASE-1].NewOffset> (pZeroReport->Phase2Num+1))
                {
                    Bfirstpoint = ZeroData[e_B_PHASE-1].NewOffset - (pZeroReport->Phase2Num+1);
                }
                else
                {
                    Bfirstpoint =MAXNTBCOUNT- (pZeroReport->Phase2Num+1) +ZeroData[e_B_PHASE-1].NewOffset;
                }
                if(Zero_Result.ColloctResult[0] ==0)Zero_Result.ColloctResult[0] = ZeroData[e_B_PHASE-1].NTB[Bfirstpoint++];
                for(i=0;i<pZeroReport->Phase2Num;)
                {
                    if(Zero_Result.ColloctPeriod == e_HalfPeriod )
                    {
                        Zero_Result.ColloctResult[i +1 +pZeroReport->Phase1Num] = ZeroData[e_B_PHASE-1].NTB[Bfirstpoint]- 10*STAMPUNIT;
                        i++;
                    }
                    Zero_Result.ColloctResult[i +1 +pZeroReport->Phase1Num] = ZeroData[e_B_PHASE-1].NTB[Bfirstpoint];
                    i++;

                    Bfirstpoint = (Bfirstpoint+1)%MAXNTBCOUNT;
                }
                listLen= CreatZeroDifflist(pZeroReport->Phase2Num, &listSeq, pZeroReport->NTBdiff,
                                            Zero_Result.ColloctResult[0],1+pZeroReport->Phase1Num);
                app_printf("2listSeq = %d,listLen = %d!\n",listSeq,listLen);
                //listLen +=perlen;
            }
            else
#endif
            {
                pZeroReport->Phase2Num =0;
            }
#if defined(ZC3750STA)
            if(nnib.PhasebitC)
            {

                if(ZeroData[e_C_PHASE-1].NewOffset> (pZeroReport->Phase3Num+1))
                {
                    Cfirstpoint = ZeroData[e_C_PHASE-1].NewOffset - (pZeroReport->Phase3Num+1);
                }
                else
                {
                    Cfirstpoint =MAXNTBCOUNT- (pZeroReport->Phase3Num+1) +ZeroData[e_C_PHASE-1].NewOffset;
                }
                if(Zero_Result.ColloctResult[0] ==0)Zero_Result.ColloctResult[0] = ZeroData[e_C_PHASE-1].NTB[Cfirstpoint++];
                for(i=0;i<pZeroReport->Phase3Num;)
                {
                    if(Zero_Result.ColloctPeriod == e_HalfPeriod )
                    {
                        Zero_Result.ColloctResult[i + 1 + pZeroReport->Phase1Num + pZeroReport->Phase2Num] = ZeroData[e_C_PHASE-1].NTB[Cfirstpoint]- 10*STAMPUNIT;
                        i++;
                    }
                    Zero_Result.ColloctResult[i + 1 + pZeroReport->Phase1Num + pZeroReport->Phase2Num] = ZeroData[e_C_PHASE-1].NTB[Cfirstpoint];
                    i++;

                    Cfirstpoint = (Cfirstpoint+1)%MAXNTBCOUNT;
                }
                listLen= CreatZeroDifflist(pZeroReport->Phase3Num, &listSeq, pZeroReport->NTBdiff,
                                            Zero_Result.ColloctResult[0],1+pZeroReport->Phase1Num+pZeroReport->Phase2Num);
                app_printf("3listSeq = %d,listLen = %d!\n",listSeq,listLen);
            }
            else
#endif
            {
                pZeroReport->Phase3Num =0;
            }
            pZeroReport->ColloctNumber = pZeroReport->Phase3Num + pZeroReport->Phase2Num+pZeroReport->Phase1Num +1;
            pZeroReport->FirstNTB = Zero_Result.ColloctResult[0]>>8;

        }
    }

    for(i=0;i<pZeroReport->ColloctNumber+1;i++)
    {
        net_printf("-------i=%d-----ColloctResult=0x%08x-------------\n",i,Zero_Result.ColloctResult[i]);
    }
    dump_buf(pZeroReport->NTBdiff,listLen);
    dump_buf((U8 *)&pZeroReport,listLen+10);

    msduLen = sizeof(ZERO_CROSSNTB_REPORT_t) + listLen;
    CreatMMsgMacHeader(pMacPublicHeader, msduLen, nnib.CCOMacAddr, CCO_TEI,
                       e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_UNUSE_PROXYROUTE, 15,15,e_TWO_WAY, TRUE);

    if(MacUse)
    {
        macLen = sizeof(MAC_LONGDATA_HEADER) + msduLen + CRCLENGTH;
    }
    else
    {
        macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;

    }

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeZeroCrossNTBReport, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeZeroCrossNTBReport, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }


    /*LongMacDataRequst(macLen, pMacPublicHeader, nnib.ParentTEI , e_UNICAST_FREAM,
                      e_MMeZeroCrossNTBReport, linkid, e_NETMMSG, 0, 10);*/
    //timer_stop(g_WaitNTBTimer,TMR_NULL);

    zc_free_mem(pMacPublicHeader);

    return;
}


//���������ͻ�ϱ�
void SendMMeRFChannelConflictReport(U8 *pMacAddr, U16 DelayTime, U8 MacUse, U8 option, U16 channel)
{
    U8                          linkid;
//    U8                          i;
    U16							macLen, msduLen;
    MAC_PUBLIC_HEADER_t    		*pMacPublicHeader;
    RFCHANNEL_CONFLIC_REPORT_t	*pNetConflicReport;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(RFCHANNEL_CONFLIC_REPORT_t)
                                                           +MaxSNIDnum * 2 + CRCLENGTH, "NetConflicReport",MEM_TIME_OUT);

    if(MacUse)
    {
        pNetConflicReport = (RFCHANNEL_CONFLIC_REPORT_t *)((U8*)pMacPublicHeader+sizeof(MAC_LONGDATA_HEADER));
    }
    else
    {
        pNetConflicReport = (RFCHANNEL_CONFLIC_REPORT_t *)pMacPublicHeader->Msdu;
    }

    linkid = 1;
    pNetConflicReport->MmsgHeader_t.MmsgType = e_MMeRfChannelConfilictReport;
    pNetConflicReport->MmsgHeader_t.Reserve  = 0;

    __memcpy(pNetConflicReport->CCOAddr, pMacAddr, MACADRDR_LENGTH);
    pNetConflicReport->NBNetNum = 1;
    pNetConflicReport->NBnetList[0] = channel;//WPHY_CHANNEL;//wphy_hz_to_ch(wphy_get_option(), wphy_get_fckhz() * 1000);
    pNetConflicReport->NBnetList[1] = option & 0x03;

    net_printf("RFChannelConflictReport : %d, ", pNetConflicReport->NBnetList[0]);
    dump_level_buf(DEBUG_MDL, pNetConflicReport->CCOAddr, MACADRDR_LENGTH);

    //��д�ھ������ŵ����
//    for(i = 0; i < MaxSNIDnum; i++)
//    {
//        if(StaNeigborNet_t[i].NbSNID != 0xffffffff)
//        {
//            NeighborNetSNID[0] = StaNeigborNet_t[i].NbSNID;
//            NeighborNetSNID[1] = StaNeigborNet_t[i].NbSNID >> 8;
//            NeighborNetSNID[2] = StaNeigborNet_t[i].NbSNID >> 16;
//            __memcpy(&pNetConflicReport->NBnetList[3 * (pNetConflicReport->NBnetNumber++)], NeighborNetSNID, 3);
//        }
//    }

    msduLen = sizeof(NETWORK_CONFLIC_REPORT_t) + (pNetConflicReport->NBNetNum);

    CreatMMsgMacHeader(pMacPublicHeader, msduLen, nnib.CCOMacAddr, GetProxy(),
                       e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, nnib.NodeLevel, nnib.NodeLevel,e_STA_CCO, MacUse);

    if(MacUse)
    {
        macLen = sizeof(MAC_LONGDATA_HEADER) + msduLen + CRCLENGTH;
    }
    else
    {
        macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;

    }

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeRfChannelConfilictReport, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(),
                      e_UNICAST_FREAM, e_MMeRfChannelConfilictReport, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }


    /*LongMacDataRequst(macLen, pMacData, nnib.ParentTEI , e_UNICAST_FREAM,
                          e_MMeNetworkConflictReport, linkid, e_NETMMSG, 0, DelayTime);*/

    zc_free_mem(pMacPublicHeader);
}


#endif

#if defined (STATIC_MASTER)



/*---------------��������ʹ��--------------------*/
U8  GatherIndBuffer[512];//�齨����ָʾʱʹ��
U16 GatherIndBufferLen = 0; // mac length
U8  GatherFlag = FALSE; //�������ܱ�־

extern U32 g_CnfAssociaRandomNum, g_CnfManageMsgSeqNum;
U16 StationInfoLenght = 0; //վ����Ϣ����
ostimer_t g_AssociGatherTimer;

/*----------����������·��������-------------------*/
U8  RfGatherIndBuffer[512];//�齨����ָʾʱʹ��
U16 RfGatherIndBufferLen = 0; // mac length
U8  RfGatherFlag = FALSE; //�������ܱ�־
U16 RfStationInfoLenght = 0; //վ����Ϣ����
ostimer_t g_RfAssociGatherTimer;

/*���� �� 	CCO���͹����ظ���STA
  ����1	�����ظ����
  ����2	����Ŀ���ַ
  ����3	���������
  ����4	�������к�
  ����5	�����ڵ����Ĳ㼶��
  ����6	�����ڵ��TEI
  ����7	�����ڵ�ĸ��ڵ�TEI
  ����8	��ʱʱ��
  ����9	mac��ַʹ�ñ�־
*/
void SendAssociateCfm(U8 Status, U8 *AssocDstMacAdd, U32 AssocRandomNum,
                                         U32 MMsgSeqNum , U8 Level, U16 TEI, PROXY_INFO_t ProxyInfo, U16 DelayTime, U8 MacUse)
{

    mg_update_type(STEP_5, netMsgRecordIndex);
    U8                          linkid,DstMaddr[6];
        U16                         msduLen, macLen;
        U16                         i, j, nextTEI, dstTEI;
        U16                         offset = 0;
        U16                         PCOnum = 0;
        MAC_PUBLIC_HEADER_t         *pMacPublicHeader;
        ASSOCIATE_CFM_CMD_t         *pAssociateCfm;

        PROXY_ROUTE_RABLE_t          DirectConnectProxyInfo;
        U8 linkType = 0;

        U16 ProxyTEI = ProxyInfo.ParatTEI;
        
        linkType = ProxyInfo.LinkType;
        pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER) + 2048,"AssciateCfm",MEM_TIME_OUT);

        if(MacUse == TRUE)
        {
            pAssociateCfm = (ASSOCIATE_CFM_CMD_t *)((U8*)pMacPublicHeader+sizeof(MAC_LONGDATA_HEADER));
        }
        else
        {
            pAssociateCfm = (ASSOCIATE_CFM_CMD_t *)pMacPublicHeader->Msdu;
        }

        pAssociateCfm->MmsgHeader_t.MmsgType = e_MMeAssocCfm;
        pAssociateCfm->MmsgHeader_t.Reserve  = 0;

        linkid = 1;
        __memcpy(pAssociateCfm->DstMacAddr, AssocDstMacAdd, 6);
        __memcpy(pAssociateCfm->CCOMacAddr, nnib.CCOMacAddr, 6);


        pAssociateCfm->Result = Status;
        pAssociateCfm->AssociaRandomNum = AssocRandomNum;
        pAssociateCfm->ManageMsgSeqNum  = MMsgSeqNum;
        pAssociateCfm->RouteUpdateSeqNumber = nnib.RouteUpdateSeqNumber++;
        if(PROVINCE_HUNAN==app_ext_info.province_code)//����������֤��Ҫ��Ӧ�ñ�ʶ��ֵ
        {
            pAssociateCfm->ApplicationID[0] = APPLICATION_ID0;
            pAssociateCfm->ApplicationID[1] = APPLICATION_ID1;
        }
        pAssociateCfm->NodeDeep = Level;
        pAssociateCfm->NodeTEI = TEI;
        pAssociateCfm->LinkType = ProxyInfo.LinkType;	//���ز�̨����Ҫ�ظ�������·
        #ifdef HPLC_HRF_PLATFORM_TEST
		//��Ƶ�α�־����������ز���Ƶ�Σ���֧�֣�����ǰ�����ز�
		pAssociateCfm->HplcBand = GetHplcBand();
		if(HPLC.scanFlag == TRUE && DeviceTEIList[TEI-2].ModuleType == e_HPLC_Module)
        {      
            pAssociateCfm->HplcBand = 0;
        }
        #endif
        pAssociateCfm->ProxyTEI = ProxyTEI;
        pAssociateCfm->SubpackageNum = 1;
        pAssociateCfm->SubpackageSeq = 1;
        pAssociateCfm->RouteTableInfo.DirectConnectProxyNum = 0;
        pAssociateCfm->RouteTableInfo.DirectConnectNodeNum = 0;
        pAssociateCfm->RouteTableInfo.RouteTableSize = 0;
        
        net_printf("pAssociateCfm->LinkType = %d\n", pAssociateCfm->LinkType);
        switch(Status)
        {
        case ASSOC_SUCCESS:
        case RE_ASSOC_SUCCESS:
        {
            pAssociateCfm->ReassociaTime = 0;
            //���·�ɱ�
            pAssociateCfm->RouteTableInfo.DirectConnectNodeNum = SearchAllDirectSTA(TEI, pAssociateCfm->RouteTableInfo.RouteTable);
            offset = 2 * pAssociateCfm->RouteTableInfo.DirectConnectNodeNum ;

            //���㾭��������������е���վ��
            for(i = 0; i < NWK_MAX_ROUTING_TABLE_ENTRYS; i++)
            {
                if(TEI ==  DeviceTEIList[i].ParentTEI)
                {
                    if(DeviceTEIList[i].NodeType == e_PCO_NODETYPE)//�ҵ�TEI�ӽڵ��е�PCO
                    {
                        PCOnum++;

                        ChildCount = 0;
                        DT_SearchAllChildStation(DeviceTEIList[i].NodeTEI);

                        DirectConnectProxyInfo.NodeTei = DeviceTEIList[i].NodeTEI;
                        DirectConnectProxyInfo.LinkType = DeviceTEIList[i].LinkType;
                        DirectConnectProxyInfo.ChildCount = ChildCount;

                        __memcpy((U8 *)&pAssociateCfm->RouteTableInfo.RouteTable[offset],	&DirectConnectProxyInfo, sizeof(PROXY_ROUTE_RABLE_t));
                        offset += sizeof(PROXY_ROUTE_RABLE_t);

                        if(ChildCount >1000)
                        {

                            net_printf("routeList too long ,need more packet!-------ChildCount=%d",ChildCount);
                            ChildCount =1000;
                        }
                        for(j = 0; j < ChildCount; j++)
                        {
                            ChildStaIndex[j] += 2;		//�����ת��ΪTEI
                        }
                        __memcpy((U8 *)&pAssociateCfm->RouteTableInfo.RouteTable[offset] ,	ChildStaIndex, 2 * ChildCount);
                        offset += 2 * ChildCount;
                    }

                }
            }

            pAssociateCfm->RouteTableInfo.DirectConnectProxyNum = PCOnum;
            pAssociateCfm->RouteTableInfo.RouteTableSize = 	offset;

            msduLen = sizeof(ASSOCIATE_CFM_CMD_t) + pAssociateCfm->RouteTableInfo.RouteTableSize;

            break;
        }
        case NOTIN_WHITELIST:
        case NO_WHITELIST:
        case STA_FULL:
        case DEPTH_OVER:
        {
            if(Status == NO_WHITELIST)
            {
                pAssociateCfm->ReassociaTime = 60*1000;
            }
            else
            {
                pAssociateCfm->ReassociaTime = 200*1000;
            }
            msduLen = sizeof(ASSOCIATE_CFM_CMD_t);
            break;
        }
        default :
        {
            zc_free_mem(pMacPublicHeader);
            net_printf("----SendAserr\n");
            return;
        }

        }

        mg_update_type(STEP_6, netMsgRecordIndex);

        if(ProxyTEI==CCO_TEI)
        {
            nextTEI = 0xFFF;   //nextTEI = TEI;
            dstTEI  = nextTEI;
            //linkType = DeviceTEIList[TEI -2].LinkType;
            __memcpy(DstMaddr, AssocDstMacAdd, 6);
        }
        else
        {
            nextTEI = SearchNextHopAddrFromRouteTable(ProxyTEI, ACTIVE_ROUTE);
            dstTEI  = nextTEI;
            if(nextTEI ==INVALID)
            {
                zc_free_mem(pMacPublicHeader);
                return;
            }

            //linkType = DeviceTEIList[nextTEI -2].LinkType;
            __memcpy(DstMaddr,DeviceTEIList[nextTEI-2].MacAddr,6);
        }


        mg_update_type(STEP_7, netMsgRecordIndex);
        if(MacUse)
        {
            macLen = sizeof(MAC_LONGDATA_HEADER) + msduLen+CRCLENGTH;
        }
        else
        {
            macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen+CRCLENGTH;

        }

        //�齨MAC֡ͷ
        if(ProxyTEI == CCO_TEI)
        {
            net_printf("---AsLBLen=%d, linktype:%d--\n",msduLen, linkType);
            CreatMMsgMacHeader(pMacPublicHeader, msduLen, DstMaddr, dstTEI,
                           e_LOCAL_BROADCAST_FREAM_NOACK, NWK_MMEMSG_MAX_RETRIES/*NWK_UNICAST_MAX_RETRIES*/, e_USE_PROXYROUTE, 1, 1, e_TWO_WAY, MacUse);

            if(linkType == e_HPLC_Link)
            {
                MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacPublicHeader, macLen, nextTEI,
                              e_LOCAL_BROADCAST_FREAM_NOACK, e_MMeAssocCfm, linkid, e_NETMMSG, 0, 0, e_MainRoute,getTeiPhase(TEI));
            }
            else
            {
                MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacPublicHeader, macLen, nextTEI,
                                e_LOCAL_BROADCAST_FREAM_NOACK, e_MMeAssocCfm, linkid, e_NETMMSG, 0, 0, e_MainRoute);
            }
        }
        else
        {
            linkType = DeviceTEIList[nextTEI -2].LinkType;
            net_printf("---AsUNLen=%d, linktype:%d--\n",msduLen, linkType);
            CreatMMsgMacHeader(pMacPublicHeader, msduLen, DstMaddr, dstTEI,
                           e_UNICAST_FREAM, NWK_MMEMSG_MAX_RETRIES/*NWK_UNICAST_MAX_RETRIES*/, e_USE_PROXYROUTE, 15, 15,e_CCO_STA, MacUse);
            if(linkType == e_HPLC_Link)
            {
                MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacPublicHeader, macLen, nextTEI,
                                     e_UNICAST_FREAM, e_MMeAssocCfm, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
            }
            else
            {
                MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacPublicHeader, macLen, nextTEI,
                                e_UNICAST_FREAM, e_MMeAssocCfm, linkid, e_NETMMSG, 0, 0, e_MainRoute);
            }

        }

        mg_update_type(STEP_8, netMsgRecordIndex);


        zc_free_mem(pMacPublicHeader);

        return;
}
/*---------------��������ʹ��--------------------*/


//��������ָʾʹ�ô�MAC��ַ��MAC֡ͷ
void  SendAssociateGatherInd(U16 TEI)
{
    U16                                      msduLen;
    MAC_PUBLIC_HEADER_t                      *pMacPublicHeader;
    ASSOCIATE_GATHER_IND_CMD_t               *pAssociateGatherInd;

    U8 LinkType = DeviceTEIList[TEI -2].LinkType;

    net_printf("SendGatherInd:LkTp:%d\n", LinkType);

    U8 *GatherBuf;
    U16 *StaInfoLen;
    U16 *GatherBufLen;
    U8 *GathFlag;
    ostimer_t *GatherTimer;

    if(LinkType == e_HPLC_Link)
    {
        GatherBuf     = GatherIndBuffer;
        StaInfoLen    = &StationInfoLenght;
        GatherBufLen  = &GatherIndBufferLen;
        GathFlag      = &GatherFlag;
        GatherTimer   = &g_AssociGatherTimer;
    }
    else
    {
        GatherBuf     = RfGatherIndBuffer;
        StaInfoLen    = &RfStationInfoLenght;
        GatherBufLen  = &RfGatherIndBufferLen;
        GathFlag      = &RfGatherFlag;
        GatherTimer   = &g_RfAssociGatherTimer;
    }


    pMacPublicHeader    = (MAC_PUBLIC_HEADER_t *)GatherBuf;
    pAssociateGatherInd = (ASSOCIATE_GATHER_IND_CMD_t *)&GatherBuf[sizeof(MAC_LONGDATA_HEADER)];

    if(*GathFlag)//�ڻ���״̬ʱ���ڹ�������ָʾ�������TEI
    {

        //ʹ��TEI����
        {
            U8 i = 0;
            for(i = 0; i < pAssociateGatherInd->GatherNodeNum; i++)
            {
                if(GetWord(pAssociateGatherInd->StationInfo + 8*i + MACADRDR_LENGTH) == TEI)
                {
                    net_printf("TEI: %03x In GathInfo\n", TEI);
                    return;
                }
            }
        }


        timer_stop(GatherTimer,TMR_NULL);

        pAssociateGatherInd->GatherNodeNum++;
        __memcpy(&pAssociateGatherInd->StationInfo[*StaInfoLen] , DeviceTEIList[TEI - 2].MacAddr, MACADRDR_LENGTH);
        *StaInfoLen += MACADRDR_LENGTH;
        __memcpy(&pAssociateGatherInd->StationInfo[*StaInfoLen], &TEI, 2);
        *StaInfoLen += 2;

        msduLen = pMacPublicHeader->MsduLength_l + (pMacPublicHeader->MsduLength_h << 8);
        msduLen += 8;
        pMacPublicHeader->MsduLength_l = msduLen;
        pMacPublicHeader->MsduLength_h = msduLen >> 8;

        *GatherBufLen += 8;
        timer_start(GatherTimer);//��ʱ�����㿪ʼ��ʱ

        if(pAssociateGatherInd->GatherNodeNum >=50)
        {
            net_printf("pAssociateGatherInd->GatherNodeNum : %d\n",pAssociateGatherInd->GatherNodeNum);
            timer_stop(GatherTimer,TMR_CALLBACK);
        }
    }
    else//�齨��������ָʾ
    {
        *GathFlag = TRUE;
//        memset(GatherBuf, 0, sizeof(GatherBuf));
        memset(GatherBuf, 0, 512);

        *StaInfoLen = 0;
        *GatherBufLen = 0;
        pAssociateGatherInd->MmsgHeader_t.MmsgType = e_MMeAssocGatherInd;
        pAssociateGatherInd->MmsgHeader_t.Reserve  = 0;

        pAssociateGatherInd->Result = ASSOC_SUCCESS;
        pAssociateGatherInd->NodeDeep = 1;
        __memcpy(pAssociateGatherInd->CCOMacAddr, nnib.CCOMacAddr, MACADRDR_LENGTH);
        pAssociateGatherInd->ProxyTEI = CCO_TEI;
        pAssociateGatherInd->FormationSeqNumber = nnib.FormationSeqNumber;
        pAssociateGatherInd->GatherNodeNum = 1;
        //if(LinkType == e_RF_Link)
        {
            pAssociateGatherInd->HplcBand = GetHplcBand();
        }
        if( PROVINCE_HUNAN == app_ext_info.province_code )
        {
            pAssociateGatherInd->ApplicationID[0] = APPLICATION_ID0;
            pAssociateGatherInd->ApplicationID[1] = APPLICATION_ID1;
        }
        __memcpy(&pAssociateGatherInd->StationInfo[0], DeviceTEIList[TEI-2].MacAddr, MACADRDR_LENGTH);
        *StaInfoLen += MACADRDR_LENGTH;
        __memcpy(&pAssociateGatherInd->StationInfo[*StaInfoLen], &TEI, 2);
        *StaInfoLen += 2;
//        msduLen = sizeof(ASSOCIATE_GATHER_IND_CMD_t) + StationInfoLenght - 1;
        msduLen = sizeof(ASSOCIATE_GATHER_IND_CMD_t) + *StaInfoLen;
        *GatherBufLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;

        //�齨MAC֡ͷ
        CreatMMsgMacHeader(pMacPublicHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                           e_LOCAL_BROADCAST_FREAM_NOACK, 10, e_USE_PROXYROUTE, 1, 1,e_TWO_WAY, TRUE);

        timer_start(GatherTimer);//��ʱ�����㿪ʼ��ʱ
    }

}

//OriginateTEI �����ɷ�����
//ProxyTEI  �µĴ���
void SendProxyChangeCnf(U16 OriginateTEI, U16 ProxyTEI, U32 ManageMsgSeq , U16 DelayTime, U8 MacUse,U8 Reason)
{
    U8                        linkid;
    U16                       j;
    U16                       nextTEI, msduLen, macLen;
    MAC_PUBLIC_HEADER_t       *pMacPublicHeader;
    PROXYCHANGE_CFM_CMD_t     *pChangeProxycfm;
    net_printf("SendProxyChangeCnf<%03x> = %d,ChildCnt = %d\n",OriginateTEI, sizeof(MAC_LONGDATA_HEADER)+sizeof(PROXYCHANGE_CFM_CMD_t)+CRCLENGTH+ChildCount*2,ChildCount);
    pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(PROXYCHANGE_CFM_CMD_t)+CRCLENGTH+ChildCount*2,
                                                           "ChangeProxyCfm",MEM_TIME_OUT);

    if(MacUse)
    {
        pChangeProxycfm = (PROXYCHANGE_CFM_CMD_t *)((U8*)pMacPublicHeader+sizeof(MAC_LONGDATA_HEADER));
    }
    else
    {
        pChangeProxycfm = (PROXYCHANGE_CFM_CMD_t *)pMacPublicHeader->Msdu;
    }

    linkid = 1;
    pChangeProxycfm->MmsgHeader_t.MmsgType = e_MMeChangeProxyCnf;
    pChangeProxycfm->MmsgHeader_t.Reserve  = 0;

    pChangeProxycfm->Result = Reason;
    pChangeProxycfm->AllPacketcount  = 1;
    pChangeProxycfm->SubPacketSeqNum = 1;
    pChangeProxycfm->StaTEI = OriginateTEI;
    pChangeProxycfm->ProxyTei = ProxyTEI;
    pChangeProxycfm->LinkType = DeviceTEIList[pChangeProxycfm->StaTEI-2].LinkType;

    if(ProxyTEI == 0)
    {
        zc_free_mem(pMacPublicHeader);
        return;
    }
//	mg_update_type(STEP_6);

    if(ProxyTEI != CCO_TEI)
    {
        if(DeviceTEIList[ProxyTEI-2].NodeType == e_STA_NODETYPE)
        {
            DeviceTEIList[ProxyTEI-2].NodeType = e_PCO_NODETYPE;
            nnib.PCOCount++;
            if(nnib.discoverSTACount >0)
                nnib.discoverSTACount--;
        }
        pMacPublicHeader->Radius = DeviceTEIList[ProxyTEI - 2].NodeDepth + 1;
        nextTEI = SearchNextHopAddrFromRouteTable(ProxyTEI, ACTIVE_ROUTE);
        if(nextTEI == INVALID)
        {
            zc_free_mem(pMacPublicHeader);
            return;
        }
    }
    else
    {
        pMacPublicHeader->Radius = 1;
        nextTEI = OriginateTEI;
    }
    msduLen = sizeof(PROXYCHANGE_CFM_CMD_t);

    //�����������ʱ���Ѿ�����
    pChangeProxycfm->ChildStaCount = ChildCount;
    pChangeProxycfm->ManageMsgSeqNum = ManageMsgSeq;
    pChangeProxycfm->RouteUpdateSeqNum = nnib.RouteUpdateSeqNumber++;
//	mg_update_type(STEP_7);

    //net_printf("---CCO send ManageMsgSeqNum is %d----\n", ManageMsgSeq);
    for(j=0; j < ChildCount; j++)
    {
        ChildStaIndex[j] += 2;		//�����ת��ΪTEI
    }
    __memcpy(pChangeProxycfm->StationEntry, ChildStaIndex, ChildCount * 2);
    msduLen  += ChildCount * 2;

    //�齨MAC֡ͷ
    CreatMMsgMacHeader(pMacPublicHeader, msduLen, DeviceTEIList[nextTEI-2].MacAddr, nextTEI,
            e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 15, 15,e_CCO_STA, MacUse);
    if(MacUse)
    {
        macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;
    }
    else
    {
        macLen = msduLen + sizeof(MAC_PUBLIC_HEADER_t)+CRCLENGTH;
    }
//	mg_update_type(STEP_8);

    if(NwkRoutingTable[nextTEI-1].LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, macLen, nextTEI,
                      e_UNICAST_FREAM, e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, macLen, nextTEI,
                      e_UNICAST_FREAM, e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*LongMacDataRequst(macLen, pMacData, nextTEI, e_UNICAST_FREAM,
                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, DelayTime);*/

    zc_free_mem(pMacPublicHeader);
//    mg_update_type(STEP_9);


}
void SendProxyChangeBitMapCnf(U16 OriginateTEI, U16 ProxyTEI, U32 ManageMsgSeq, U16 DelayTime, U8 MacUse,U8 Reason)
{
//    U8 									linkid;
//    //U16									j;
//    U16 								nextTEI, msduLen, macLen;
//    MAC_PUBLIC_HEADER_t    				*pMacData;
//    PROXYCHANGE_BITMAP_CFM_CMD_t   	*pChangeProxyBitMapcfm_t;
//    U8									*pManageTxBuffer;
//    U8                                  StaBitMap[128];

//    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendProxyChangeBitMapCnf", MEM_TIME_OUT);

//    pMacData		= (MAC_PUBLIC_HEADER_t *)pManageTxBuffer;
//    if(MacUse)
//    {
//        pChangeProxyBitMapcfm_t = (PROXYCHANGE_BITMAP_CFM_CMD_t *)&pManageTxBuffer[sizeof(MAC_LONGDATA_HEADER)];
//    }
//    else
//    {
//        pChangeProxyBitMapcfm_t = (PROXYCHANGE_BITMAP_CFM_CMD_t *)&pManageTxBuffer[sizeof(MAC_PUBLIC_HEADER_t)];
//    }

//    linkid = 1;
//    pChangeProxyBitMapcfm_t->MmsgHeader_t.MmsgType = e_MMeChangeProxyBitMapCnf;

//    pChangeProxyBitMapcfm_t->Result = Reason;
//    pChangeProxyBitMapcfm_t->StaTEI = OriginateTEI;
//    pChangeProxyBitMapcfm_t->ProxyTei = ProxyTEI;
//    pChangeProxyBitMapcfm_t->LinkType = DeviceTEIList[pChangeProxyBitMapcfm_t->StaTEI-2].LinkType;

//    if( ProxyTEI == 0)
//    {
//        zc_free_mem(pManageTxBuffer);
//        return;
//    }
//    if( ProxyTEI != CCO_TEI)
//    {

//        if(DeviceTEIList[ProxyTEI - 2].NodeType == e_STA_NODETYPE)
//        {
//            DeviceTEIList[ProxyTEI - 2].NodeType = e_PCO_NODETYPE;
//            nnib.PCOCount++;
//            if(nnib.discoverSTACount >0)
//                nnib.discoverSTACount--;
//        }
//        pMacData->Radius = DeviceTEIList[ProxyTEI - 2].NodeDepth + 1;
//        nextTEI = SearchNextHopAddrFromRouteTable(ProxyTEI, ACTIVE_ROUTE);
//        if(nextTEI == INVALID)
//        {
//            zc_free_mem(pManageTxBuffer);
//            return;
//        }
//    }
//    else
//    {
//        pMacData->Radius = 1;
//        nextTEI = OriginateTEI;
//    }
//    msduLen = sizeof(PROXYCHANGE_BITMAP_CFM_CMD_t) - 1;

//    pChangeProxyBitMapcfm_t->ManageMsgSeqNum = ManageMsgSeq;
//    pChangeProxyBitMapcfm_t->RouteUpdateSeqNum = nnib.RouteUpdateSeqNumber++;
//    net_printf("---CCO send ManageMsgSeqNum is %d----\n", ManageMsgSeq);

//    memset(StaBitMap, 0x00, 128);
//    pChangeProxyBitMapcfm_t->BitMapSize = GetChildStaBmpByteNum(StaBitMap);
//    app_printf("pChangeProxyBitMapcfm_t->BitMapSize = %d\n",pChangeProxyBitMapcfm_t->BitMapSize);
//    app_printf("ProxyChangeStaBitMap: ");
//    dump_buf(StaBitMap, 128);
//    __memcpy( pChangeProxyBitMapcfm_t->StationBitMap, StaBitMap, pChangeProxyBitMapcfm_t->BitMapSize);
//    msduLen  += pChangeProxyBitMapcfm_t->BitMapSize;

//    //�齨MAC֡ͷ
//    CreatMMsgMacHeader(pMacData, msduLen, DeviceTEIList[nextTEI - 2].MacAddr, nextTEI,
//                       e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 15, 15,e_CCO_STA, MacUse);
//    if(MacUse)
//    {
//        macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;
//    }
//    else
//    {
//        macLen = msduLen + sizeof(MAC_PUBLIC_HEADER_t)+CRCLENGTH;
//    }
////    LongMacDataRequst(macLen, pMacData, nextTEI, e_UNICAST_FREAM,
////                      e_MMeChangeProxyBitMapCnf, linkid, e_NETMMSG, 0, DelayTime);
//    MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacData, macLen, nextTEI,
//                  e_UNICAST_FREAM, e_MMeChangeProxyBitMapCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);

//    zc_free_mem(pManageTxBuffer);
}

void SendMMeDelayLeaveOfNum(U8 *MacAddr, U8 StaNum, U16 DelayTime,U8 DelType)
{
    U8                          linkid;
    U16                         i;
    U16                         msduLen, macLen,TEI;
    MAC_LONGDATA_HEADER        *pMacLongHeader;
    DELAY_LEAVE_LINE_CMD_t     *pDelayLLInd;


    U8 *pMacData = zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER) + sizeof(DELAY_LEAVE_LINE_CMD_t) + StaNum*MAC_ADDR_LEN + CRCLENGTH, "SLVE", MEM_TIME_OUT);

    pMacLongHeader = (MAC_LONGDATA_HEADER*)pMacData;
    pDelayLLInd   = (DELAY_LEAVE_LINE_CMD_t*)pMacLongHeader->Msdu;

    pDelayLLInd->MmsgHeader_t.MmsgType = e_MMeLeaveInd;
    pDelayLLInd->MmsgHeader_t.Reserve  = 0;
    linkid	= 1;

    pDelayLLInd->Result = e_CCOdiver;
    pDelayLLInd->StaNum = StaNum;
    __memcpy(pDelayLLInd->MacAddress, MacAddr, StaNum * MAC_ADDR_LEN);

    net_printf("---LevelNum=%d\n", StaNum);
    msduLen = pDelayLLInd->StaNum * MAC_ADDR_LEN + sizeof(DELAY_LEAVE_LINE_CMD_t);

    for(i=0; i<StaNum; i++)
    {
        TEI = SearchTEIDeviceTEIList(MacAddr + i * MAC_ADDR_LEN);
        net_printf("---TEI=%d\n",TEI);

        if(TEI !=INVALID)
        {
            
            if(DelType == e_LEAVE_AND_DEL_DEVICELIST)
            {
                if(DeviceTEIList[TEI-2].NodeType ==	e_STA_NODETYPE )
                {
                    if(nnib.discoverSTACount > 0)
                        nnib.discoverSTACount--;

	                //net_printf("-----------nnib.discoverSTACount-----------\n");
	            }
	            else if(DeviceTEIList[TEI-2].NodeType == e_PCO_NODETYPE)
	            {
	            	if(nnib.PCOCount > 0)
	                	nnib.PCOCount--;
	                //net_printf("-----------nnib.PCOCount-----------\n");
	            }
                DelDeviceListEach(TEI);
            }
            //memset(&DeviceTEIList[TEI-2],0xff,sizeof(DEVICE_TEI_LIST_t));
        }
    }

    pDelayLLInd->DelayTime = DELAY_LEAVE_TIME;

    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                       e_PROXY_BROADCAST_FREAM_NOACK, NWK_FULLCAST_DEF_RETRIES, e_USE_PROXYROUTE, 15, 15,e_CCO_STA, TRUE);

    macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;

    MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, BROADCAST_TEI,
                  e_PROXY_BROADCAST_FREAM_NOACK, e_MMeLeaveInd, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, BROADCAST_TEI,
                  e_PROXY_BROADCAST_FREAM_NOACK, e_MMeLeaveInd, linkid, e_NETMMSG, 0, 0, e_MainRoute);

    /*LongMacDataRequst(macLen, pMacData, BROADCAST_TEI, e_FULL_BROADCAST_FREAM_NOACK,
                          e_MMeLeaveInd, linkid, e_NETMMSG, 0, DelayTime);*/

    zc_free_mem(pMacData);

}



void SendMMeZeroCrossNTBCollectInd(U16 NodeTEI, U16 DelayTime, U8 MacUse)
{
    U8                          linkid;
    U16                         macLen, msduLen, nextTEI;
    MAC_PUBLIC_HEADER_t         *pMacPublicHeader;
    ZERO_CROSSNTB_COLLECTIND_t	*pZeroInd;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(ZERO_CROSSNTB_COLLECTIND_t) + CRCLENGTH,
                                                                                      "ZeroNTBInd",MEM_TIME_OUT);

    if(MacUse)
    {
        pZeroInd = (ZERO_CROSSNTB_COLLECTIND_t *)((U8*)pMacPublicHeader+sizeof(MAC_LONGDATA_HEADER));
    }
    else
    {
        pZeroInd = (ZERO_CROSSNTB_COLLECTIND_t *)pMacPublicHeader->Msdu;
    }
    
    linkid = 1;
    pZeroInd->MmsgHeader_t.MmsgType = e_MMeZeroCrossNTBCollectInd;
    pZeroInd->MmsgHeader_t.Reserve  = 0;

    pZeroInd->NodeTEI = GET_TEI_VALID_BIT(NodeTEI);
    net_printf("pZeroInd->NodeTEI  is %04x  ", pZeroInd->NodeTEI);
    
    if(GET_TEI_VALID_BIT(NodeTEI) == BROADCAST_TEI)
    {
        pZeroInd->ColloctStaion = e_ColloctAll;   // ȫ��վ��ɼ�
    }
    else
    {
        pZeroInd->ColloctStaion = e_ColloctOne;   // ��վ��ɼ�
    }
    pZeroInd->ColloctPeriod = e_OnePeriod;        // ȫ���ڲɼ�
    pZeroInd->ColloctNumber =	ZERO_COLLOCT_NUM;  // �ɼ�����Ϊ13

    msduLen = sizeof(ZERO_CROSSNTB_COLLECTIND_t);
    
    if(CheckTheTEILegal(NodeTEI) != TRUE)
    {
        zc_free_mem(pMacPublicHeader);
        return;
    }

    CreatMMsgMacHeader(pMacPublicHeader, msduLen, DeviceTEIList[NodeTEI-2].MacAddr, NodeTEI,
                           e_UNICAST_FREAM, NWK_UNICAST_MAX_RETRIES, e_UNUSE_PROXYROUTE,
                           DeviceTEIList[NodeTEI-2].NodeDepth, DeviceTEIList[NodeTEI - 2].NodeDepth,e_CCO_STA, TRUE);

    if(MacUse)
    {
        macLen = sizeof(MAC_LONGDATA_HEADER) + msduLen+CRCLENGTH;
    }
    else
    {
        macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen+CRCLENGTH;
    }
    
        nextTEI = SearchNextHopAddrFromRouteTable(NodeTEI, ACTIVE_ROUTE);
    
        if(nextTEI !=INVALID)
        {
            //               LongMacDataRequst(macLen, pMacData, nextTEI , e_UNICAST_FREAM,
            //                          e_MMeZeroCrossNTBCollectInd, linkid, e_NETMMSG, 0, DelayTime);
            if(NwkRoutingTable[nextTEI - 1].LinkType == e_HPLC_Link)
            {
                MacDataRequst(pMacPublicHeader, macLen, nextTEI,
                              e_UNICAST_FREAM, e_MMeZeroCrossNTBCollectInd, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
            }
            else
            {
                MacDataRequstRf(pMacPublicHeader, macLen, nextTEI,
                              e_UNICAST_FREAM, e_MMeZeroCrossNTBCollectInd, linkid, e_NETMMSG, 0, 0, e_MainRoute);
            }



            //������ģ��Ĳ����������ʹ�ģ��һ��
            Zero_Result.ColloctType = pZeroInd->ColloctStaion;    // ��վ��ɼ���ȫ���ɼ�
            Zero_Result.ColloctPeriod = pZeroInd->ColloctPeriod;  // �����ڻ�ȫ����
            Zero_Result.ColloctTEI = NodeTEI;
            Zero_Result.ColloctNumber = pZeroInd->ColloctNumber;  // �ɼ���Ϊ13
            Zero_Result.ColloctSeq = 0;
            Zero_Result.ColloctFlag = TRUE;
            Zero_Result.ColloctResult[0] = ZeroData[e_A_PHASE-1].NTB[ZeroData[e_A_PHASE-1].NewOffset];
        }

    zc_free_mem(pMacPublicHeader);



}


void NetTaskGatherInd(work_t *work)
{
    U16    msduLen;
    U32    Crc32Datatest;

    U8 linkType = work->buf[0];
    U16 GatherBufLen;
    U8 *GatherBuf;
    net_printf("NetTaskGatherInd LkTp:%d\n", linkType);


    ASSOCIATE_GATHER_IND_CMD_t 		*pAssociateGatherInd_t;

    if(linkType == e_HPLC_Link)
    {
        GatherFlag = FALSE;
        GatherBufLen = GatherIndBufferLen;
        GatherBuf    = GatherIndBuffer;
    }
    else
    {
        RfGatherFlag = FALSE;
        GatherBufLen = RfGatherIndBufferLen;
        GatherBuf    = RfGatherIndBuffer;
    }

    pAssociateGatherInd_t = (ASSOCIATE_GATHER_IND_CMD_t*)&GatherBuf[sizeof(MAC_LONGDATA_HEADER)];


	net_printf("-----NetTaskGatherInd num =%d\n",pAssociateGatherInd_t->GatherNodeNum);
    if(pAssociateGatherInd_t->GatherNodeNum == 1)
    {
        U16 TEI;
        U8  macaddr[6];
        PROXY_INFO_t ProxyInfo;
        ProxyInfo.ParatTEI = CCO_TEI;

        __memcpy(macaddr,&GatherBuf[sizeof(MAC_LONGDATA_HEADER) + sizeof(ASSOCIATE_GATHER_IND_CMD_t)],6);
        __memcpy(&TEI,&GatherBuf[sizeof(MAC_LONGDATA_HEADER) + sizeof(ASSOCIATE_GATHER_IND_CMD_t)+6],2);


        ProxyInfo.LinkType = DeviceTEIList[TEI - 2].LinkType;

        SendAssociateCfm(ASSOC_SUCCESS, macaddr, g_CnfAssociaRandomNum, g_CnfManageMsgSeqNum,
                         1, TEI, ProxyInfo, 0, TRUE);
    }
    else
    {
        msduLen = GatherBufLen-CRCLENGTH-28;
        if(msduLen ==0 || msduLen>512)
        {
            //sys_panic("<GatherInd MsduLen err:%d \n> %s: %d\n",msduLen, __func__, __LINE__);
            debug_printf(&dty,DEBUG_GE,"NetTaskGatherInd MsduLen err: %d\n",msduLen);
        }

        crc_digest(&GatherBuf[28],msduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
        __memcpy(&GatherBuf[GatherBufLen-4], (U8 *)&Crc32Datatest, CRCLENGTH);


        //TODO ���Լ��ж�ֻ���ز� ���� ���ߡ������Ƕ���
        if(linkType == e_HPLC_Link)
        {
            MacDataRequst((MAC_PUBLIC_HEADER_t *)GatherBuf, GatherBufLen, BROADCAST_TEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                          e_MMeAssocGatherInd, 1, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {

            MacDataRequstRf((MAC_PUBLIC_HEADER_t *)GatherBuf, GatherBufLen, BROADCAST_TEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                            e_MMeAssocGatherInd, 1, e_NETMMSG, 0, 0, e_MainRoute);
        }

        /*LongMacDataRequst(GatherBufLen, (MAC_PUBLIC_HEADER_t *)GatherBuf, BROADCAST_TEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                              e_MMeAssocGatherInd, 1, e_NETMMSG, 0, 0);*/
    }

}

U8 SearchUplinkNodeTei(U16	staTei,U16 ProxyTei)
{
    U8 i;
    U16  seq;

    if(staTei==ProxyTei)
        return FALSE;

    if(ProxyTei==0 || ProxyTei>0x0fff) //����Ƿ�
    {
        return FALSE;
    }
    else if(ProxyTei == CCO_TEI) //ѡ��CCO
    {
        return TRUE;
    }
    else
    {
        seq= ProxyTei-2;

        for(i=0; i<=MaxDeepth; i++)
        {
            if(DeviceTEIList[seq].ParentTEI == CCO_TEI) //����CCO
            {
                return TRUE;
            }
            else if(DeviceTEIList[seq].ParentTEI == staTei) //���������������Լ�
            {
                return FALSE;
            }
            seq = DeviceTEIList[seq].ParentTEI-2;
            if(seq >= MAX_WHITELIST_NUM)
                return FALSE;
        }
    }

    return FALSE;
}

#endif

/**
 * @brief CreatMMsgMacHeaderForSig  ����֡ macͷ��װ��У��
 * @param pMacHeader                mac֡��ʼ��ַ
 * @param msduType                  ����֡msdu����
 * @param msduLen
 */
void CreatMMsgMacHeaderForSig(MAC_RF_HEADER_t *pMacHeader, U8 msduType, U16 msduLen)
{
    pMacHeader->MacProtocolVersion = e_MAC_VER_SINGLE;
    pMacHeader->MsduType = msduType;
    pMacHeader->MsduLen = msduLen;

    U8			*pData;
    U32  		Crc32Datatest = 0;

    pData = pMacHeader->Msdu;


    if(msduLen + sizeof(MAC_RF_HEADER_t) >=512 || msduLen==0 )
    {
        //sys_panic("<CreatMMsgMacHeader MsduLen err:%d \n> %s: %d\n",MsduLen, __func__, __LINE__);
        debug_printf(&dty,DEBUG_GE,"CreatMMsgMacHeaderForSig MsduLen err:%d,Msdutype=%02x",msduLen,pMacHeader->MsduType);
    }

    crc_digest(pData, msduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
    __memcpy(&pData[msduLen], (U8 *)&Crc32Datatest, CRCLENGTH);

}


void CreatMMsgMacHeader(MAC_PUBLIC_HEADER_t *pMacHeader, U16 MsduLen, U8 *DestMacAddr , U16 DestTEI, U8 SendType,
                        U8 MaxResendTime, U8 ProxyRouteFlag, U8 Radius, U8 RemainRadius, U8 bradDirc,U8 MacAddrUse)

{
    U8			*pData;
    U32  		Crc32Datatest = 0;

    pMacHeader->MacProtocolVersion = MAC_PROTOCOL_VER;
    pMacHeader->ScrTEI = GetTEI();
    pMacHeader->DestTEI =  DestTEI;
    pMacHeader->SendType = SendType;
    pMacHeader->MaxResendTime = MaxResendTime;
    pMacHeader->MsduSeq = ++g_MsduSequenceNumber;
    pMacHeader->MsduLength_l = MsduLen;
    pMacHeader->MsduLength_h = MsduLen >> 8;
    pMacHeader->RestTims = nnib.Resettimes;
    pMacHeader->ProxyRouteFlag = ProxyRouteFlag;
    pMacHeader->Radius = Radius;
    pMacHeader->RemainRadius = RemainRadius;
    pMacHeader->BroadcastDirection = bradDirc;  //e_CCO_STA;
    pMacHeader->RouteRepairFlag = e_NO_REPAIR;
    pMacHeader->MacAddrUseFlag = MacAddrUse;
    pMacHeader->FormationSeq = nnib.FormationSeqNumber;
    pMacHeader->MsduType = e_NETMMSG;
    pMacHeader->Reserve = 0;
    pMacHeader->Reserve1 = 0;
    pMacHeader->Reserve3 = 0;
    pMacHeader->Reserve4 = 0;

    pData = pMacHeader->Msdu;

    if(MacAddrUse)
    {
        if((MsduLen + sizeof(MAC_LONGDATA_HEADER)) >=512  || MsduLen==0 )//�쳣���
        {
            //sys_panic("<CreatMMsgMacHeader MsduLen err:%d  \n> %s: %d\n",MsduLen, __func__, __LINE__);
            debug_printf(&dty,DEBUG_GE,"CMMsgMH Len err:%d,type=%02x %02x",MsduLen,pData[28],pData[29]);
        }
        __memcpy(pData, nnib.MacAddr, MACADRDR_LENGTH);
        pData += MACADRDR_LENGTH;
        __memcpy(pData, DestMacAddr, MACADRDR_LENGTH);
        pData += MACADRDR_LENGTH;

    }
    else
    {
        if(MsduLen + sizeof(MAC_PUBLIC_HEADER_t) >=512 || MsduLen==0 )
        {
            //sys_panic("<CreatMMsgMacHeader MsduLen err:%d \n> %s: %d\n",MsduLen, __func__, __LINE__);
            debug_printf(&dty,DEBUG_GE,"CMMsgMH1 Len err:%d,type=%02x %02x",MsduLen,pData[16],pData[17]);
        }
    }

    crc_digest(pData, MsduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
    __memcpy(&pData[MsduLen], (U8 *)&Crc32Datatest, CRCLENGTH);

    return;
}

static U16 GetDisNeighborBmp(U8 *OnlineTEIbmp, U16 *ByteNum, U8 *pListInfo,U16* maxTei)
{
    U8 Byteoffset, Bitoffset;
    U16 NeighborNum = 0;
  
	list_head_t *pos,*node;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    *maxTei =0;
    list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->SNID == GetSNID()  &&  DiscvryTable->HplcInfo.My_REV !=0)
        {
            Byteoffset = DiscvryTable->NodeTEI / 8;
            Bitoffset = DiscvryTable->NodeTEI % 8;
            Bitoffset = 1 << Bitoffset;
            OnlineTEIbmp[Byteoffset] = OnlineTEIbmp[Byteoffset] | Bitoffset;

            //pListInfo[DiscvryTable->NodeTEI-1] = DiscvryTable->My_REV;
            pListInfo[NeighborNum] = DiscvryTable->HplcInfo.My_REV;
            NeighborNum++;

            if(DiscvryTable->NodeTEI > (*maxTei))
                *maxTei = DiscvryTable->NodeTEI;

        }
    }
    
    if(ByteNum != NULL)
    {
        if(*maxTei)
            *ByteNum = (*maxTei)/8 + 1;
        else
            *ByteNum = 0;
    }
    return NeighborNum;
}
void SendDiscoverNodeList(void)
{
    U8                              *pBMPheader;
    U8								*pListtail;
    U8                              *pListInfo;
    U8                              linkid;
	U16 							maxTei, i, index;

    U16								InfoLen = 0;
    U16								/*i, j,TEI,*/ msduLen, macLen;
    MAC_LONGDATA_HEADER             *pMacLongHeader;
    DISCOVER_NODELIST_CMD_t  	    *pNodeList;
	UpLinkRoute_t 					uplinkroute;

    pMacLongHeader =
    (MAC_LONGDATA_HEADER*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(DISCOVER_NODELIST_CMD_t)+2048,
                                                                                               "NodeListReq",MEM_TIME_OUT);
    if(NULL == pMacLongHeader)
    {
        net_printf("S D ph null\n");
        return;
    }

    pListInfo = zc_malloc_mem(2040, "LSIF", MEM_TIME_OUT);

    if(NULL == pListInfo)
    {
        net_printf("S D pl null\n");
        zc_free_mem(pMacLongHeader);
        return;
    }

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pNode = GetNeighborByTEI(GetProxy(), e_HPLC_Link);

    pNodeList      = (DISCOVER_NODELIST_CMD_t*)pMacLongHeader->Msdu;

    pNodeList->MmsgHeader_t.MmsgType = e_MMeDiscoverNodeList;
    pNodeList->MmsgHeader_t.Reserve  = 0;
    linkid	= 1;

    pNodeList->NodeTEI = GetTEI();
    pNodeList->ProxyTEI = GetProxy();
    pNodeList->NodeType = nnib.NodeType;
    pNodeList->NodeDeepth = nnib.NodeLevel;
    __memcpy(pNodeList->MacAddr , nnib.MacAddr, 6);
    __memcpy(pNodeList->CCOAddr , nnib.CCOMacAddr, 6);


    pNodeList->PhaseInfo1	= nnib.PossiblePhase;
    pNodeList->PhaseInfo2	= nnib.BackupPhase1;
    pNodeList->PhaseInfo3	= nnib.BackupPhase2;
    pNodeList->WithProxyLQI	= nnib.WithProxyLQI;
    if(pNode)
    {
        pNodeList->ProxyNodeUplinkRatio = pNode->HplcInfo.UplinkSuccRatio;
        pNodeList->ProxyNodeDownlinkRatio = pNode->HplcInfo.DownlinkSuccRatio;
    }
    else
    {
        pNodeList->ProxyNodeUplinkRatio	    = nnib.ProxyNodeUplinkRatio;
        pNodeList->ProxyNodeDownlinkRatio	= nnib.ProxyNodeDownlinkRatio;
    }
//    pNodeList->NeighborNum	= GetNeighborCount();
    pNodeList->SendDiscoListCount	= nnib.last_My_SED;
    #if defined(STATIC_MASTER)
    pNodeList->UpLinkRouteNum = 0;
    #else
    pNodeList->UpLinkRouteNum = 1;
    #endif
    pNodeList->NextRoutePeriodStartTime = nnib.NextRoutePeriodStartTime;
    pNodeList->LowestSuccRatio = nnib.LowestSuccRatio;

    //����·����Ϣ�ֶ�
    /*for(i = 0; i < pNodeList->UpLinkRouteNum; i++)
    {
        TEISeq = GetNeighborByTEI(GetProxy());
        if(TEISeq != 0xffff)
        {
            UpLinkRoute_t uplinkroute;

            uplinkroute.UpTEI = NeighborDiscoveryTableEntry[TEISeq].NodeTEI;
            uplinkroute.RouteType = NeighborDiscoveryTableEntry[TEISeq].Relation;
            __memcpy(&pNodeList->ListInfo[InfoLen], (U8 *)&uplinkroute, sizeof(UpLinkRoute_t));
            InfoLen += sizeof(UpLinkRoute_t);
        }
    }*/
    

    uplinkroute.UpTEI =GetProxy();
    uplinkroute.RouteType = e_PROXY;
    __memcpy(&pNodeList->ListInfo[InfoLen], (U8 *)&uplinkroute, sizeof(UpLinkRoute_t) * pNodeList->UpLinkRouteNum);
    InfoLen += sizeof(UpLinkRoute_t) * pNodeList->UpLinkRouteNum;

    pBMPheader = &pNodeList->ListInfo[InfoLen];
    pNodeList->NeighborNum = GetDisNeighborBmp(pBMPheader, &pNodeList->BmpNumber, pListInfo,&maxTei);

    //����վ��λͼ
#if defined(STATIC_MASTER)
    if(nnib.FristRoutePeriod !=TRUE)
    {

        pNodeList->BmpNumber =0;
    }
#endif
    InfoLen += pNodeList->BmpNumber;
    pListtail = &pNodeList->ListInfo[InfoLen];

    //���շ����б���Ϣ
    
    for(i = 0,index=0; i < maxTei; i++)
    {
        if(pListInfo[i] != 0)
            pListtail[index++] = pListInfo[i];
    }

    //__memcpy(pListtail, pListInfo, pNodeList->NeighborNum);

    //pNodeList->NeighborNum	= listnum; //�ھ�������ʵ�ʷ����ģ���ͳ�ƽ��մ���Ϊ0��
    InfoLen += pNodeList->NeighborNum;
    msduLen	= sizeof(DISCOVER_NODELIST_CMD_t) + InfoLen;


#if defined(STATIC_MASTER)
    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                       e_LOCAL_BROADCAST_FREAM_NOACK, 3, e_UNUSE_PROXYROUTE,
                       1, 1,e_TWO_WAY, TRUE);


#elif defined(STATIC_NODE)
    CreatMMsgMacHeader((MAC_PUBLIC_HEADER_t*)pMacLongHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                       e_LOCAL_BROADCAST_FREAM_NOACK, 1, e_UNUSE_PROXYROUTE,
                       1, 1,e_TWO_WAY, TRUE);
#endif
    macLen = msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;

    // if(macLen > sizeof(MAC_LONGDATA_HEADER)+sizeof(DISCOVER_NODELIST_CMD_t)+2048)
    // {
    //     printf_s("disc Len err:%d\n",macLen);
    // }

    MacDataRequst((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, BROADCAST_TEI,
                                 e_LOCAL_BROADCAST_FREAM_NOACK, e_MMeDiscoverNodeList, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);

    /*LongMacDataRequst(macLen, pMacData, BROADCAST_TEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                      e_MMeDiscoverNodeList, linkid, e_NETMMSG, 0, 0);*/

    //nnib.SendDiscoverListCount++;//���·��ʹ���

    zc_free_mem(pListInfo);
    zc_free_mem(pMacLongHeader);

    return;
}




#if defined(STATIC_NODE)



static void ProcessMMeAssocGatherInd(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8									macuse = FALSE;
    U8									*pMeterAddr, i;

    MAC_PUBLIC_HEADER_t 				*pMacPublicHeader = NULL;
    ASSOCIATE_GATHER_IND_CMD_t			*pAssocGatherInd  = NULL;

    PROXY_INFO_t                        *pNodeINfo;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;


    macuse = pMacPublicHeader->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pAssocGatherInd = (ASSOCIATE_GATHER_IND_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pAssocGatherInd = (ASSOCIATE_GATHER_IND_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }


    pMeterAddr = pAssocGatherInd->StationInfo;

    if(nnib.NodeState != e_NODE_OUT_NET)
    {
        net_printf("----------------nnib.NodeState != e_NODE_OUT_NET---------------\n");
        return;
    }

    for(i=0; i < pAssocGatherInd->GatherNodeNum; i++)
    {
        if(memcmp(pMeterAddr, nnib.MacAddr, 6) == 0)
        {
        	if(PROVINCE_HUNAN==app_ext_info.province_code)  //����������֤��Ҫ����
        	{
				U8  AppID[2] = {APPLICATION_ID0,APPLICATION_ID1};
				if((pAssocGatherInd->Result ==ASSOC_SUCCESS || pAssocGatherInd->Result ==RE_ASSOC_SUCCESS)&&
	                (DevicePib_t.HNOnLineFlg == TRUE&&memcmp(pAssocGatherInd->ApplicationID,AppID,2)!=0))
	            {
	                net_printf("pAssocGatherInd_t,it's not YX\n");
	                return;//����������֤���ؿ�����������֤ʧ�ܣ�������
	            }   
			}
            //���Ʊ��ڵ���Ϣ
            nnib.NodeState = e_NODE_ON_LINE;//����
            nnib.StaOfflinetime = nnib.RoutePeriodTime*2;
            nnib.RecvDisListTime = nnib.RoutePeriodTime*4;
            nnib.RfRecvDisListTime = nnib.RoutePeriodTime*4;
            nnib.NodeType = e_STA_NODETYPE;
            nnib.FormationSeqNumber = pMacPublicHeader->FormationSeq;
            __memcpy(nnib.CCOMacAddr, pAssocGatherInd->CCOMacAddr, 6);

            pNodeINfo = (PROXY_INFO_t *)(pMeterAddr + 6);

            SetTEI(pNodeINfo->ParatTEI);

//            SetTEI(GetWord((pMeterAddr + 6)));
            SetProxy(CCO_TEI) ;
            __memcpy(nnib.ParentMacAddr, nnib.CCOMacAddr, 6);
            nnib.NodeLevel = 1;



            //timer_start(g_NoBeaconTimer);//������ʱ
			nnib.LinkType = buf->LinkType;

            
            //����������·���ͣ�����Ĭ��������ʽ����������ݰ��ŵ��ű����͸��¡�
            if(nnib.LinkType == e_RF_Link)
            { 
                nnib.NetworkType = e_SINGLE_HRF;
            }
            else
            {
                nnib.NetworkType = e_SINGLE_HPLC;
            }


            //�����ͨ���������������ز�Ƶ��
            if(nnib.LinkType == e_RF_Link)
            {
                changeband(pAssocGatherInd->HplcBand);
            }


            if(DefSetInfo.FreqInfo.FreqBand != GetHplcBand()) //���������޸���Ƶ��ʱ��Ҫ����
            {
                DefSetInfo.FreqInfo.FreqBand = GetHplcBand();

                DefwriteFg.FREQ = TRUE;
                DefaultSettingFlag = TRUE;
            }

            //SavePowerlevel();
            Trigger2app(TO_ONLINE);
            ScanBandManage(e_ONLINEEVENT, buf->snid);





            // if(nnib.LinkType == e_HPLC_Link)
            // {
            //     UpdateNbInfo(GetSNID(),nnib.ParentMacAddr,CCO_TEI,0,e_CCO_NODETYPE, 0, e_PROXY,buf->rgain,PHASE_A,0,DT_SOF);
            // }
            // else
            // {
            //     UpdateRfNbInfo(GetSNID(), nnib.ParentMacAddr, CCO_TEI, 0, e_CCO_NODETYPE, 0, e_PROXY, buf->rssi, buf->snr, 0, 0, DT_SOF);
            // }

            //�����ھӱ�
            ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
            DiscvryTable = GetNeighborByTEI(GetProxy(), e_HPLC_Link);
            if(DiscvryTable)
            {
                __memcpy(nnib.ParentMacAddr , DiscvryTable->MacAddr, 6);
                DiscvryTable->NodeType = e_PCO_NODETYPE;
                DiscvryTable->Relation = e_PROXY;
                DiscvryTable->BKRouteFg = TRUE;
                DiscvryTable->HplcInfo.DownlinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->HplcInfo.UplinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->HplcInfo.LastDownlinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->HplcInfo.LastUplinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->ResetTimes = pMacPublicHeader->RestTims;
            }


            DiscvryTable = GetNeighborByTEI(GetProxy(), e_RF_Link);
            if(DiscvryTable)
            {
                __memcpy(nnib.ParentMacAddr , DiscvryTable->MacAddr, 6);
                DiscvryTable->NodeType = e_PCO_NODETYPE;
                DiscvryTable->Relation = e_PROXY;
                DiscvryTable->BKRouteFg = TRUE;
                DiscvryTable->ResetTimes = pMacPublicHeader->RestTims;
                DiscvryTable->HrfInfo.DownRcvRate = NICE_SUCC_RATIO;
                DiscvryTable->HrfInfo.UpRcvRate = NICE_SUCC_RATIO;
                DiscvryTable->HrfInfo.LDownRcvRate = NICE_SUCC_RATIO;
                DiscvryTable->HrfInfo.LUpRcvRate = NICE_SUCC_RATIO;
            }

            nnib.ProxyNodeDownlinkRatio = NICE_SUCC_RATIO;
            nnib.ProxyNodeUplinkRatio = NICE_SUCC_RATIO;

            //�������·��
            net_printf("------e_NODE_ON_LINE----------\n");
            net_printf("------StaOfflinetime =%d----------\n",nnib.StaOfflinetime);
            net_printf("------RecvDisListTime=%d----------\n",nnib.RecvDisListTime);
            net_printf("------RfRecvDisListTime=%d----------\n",nnib.RfRecvDisListTime);
            net_printf("------LinkType=%d----------\n",nnib.LinkType);
            if(!IS_BROADCAST_TEI(GetProxy()))
            {
                UpdateRoutingTableEntry(CCO_TEI, GetProxy(), 0, ACTIVE_ROUTE, /*pNodeINfo->LinkType*/buf->LinkType);
            }
            break;
        }

        pMeterAddr += 8;
    }

}


/**********************************************************************
Function:			����������ʱָʾ
Description:		���������е�����ȷ���Լ��Ƿ���Ҫ���ߣ���ú�����
Input:				Mac��Indictionָ��
Output: 			��
Return: 			��
Other:				�˺���Ϊ�²����ݴ�����յ㣬��Ҫ�ͷ��ڴ�
***********************************************************************/

static void ProcessMMeLeaveInd(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8						  *pMacAddr, linkid, i, macuse;
    U16 					   macLen, DelayTime;
    MAC_PUBLIC_HEADER_t 	  *pMacPublicHeader;
    DELAY_LEAVE_LINE_CMD_t	  *pDelayLLInd;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    net_printf("----------------ProcessMMeLeaveInd---------------\n");

    macuse = pMacPublicHeader->MacAddrUseFlag;

    if(pMacPublicHeader->FormationSeq !=nnib.FormationSeqNumber)//������Ų���Ȳ�ת��
    {
        return;
    }

    if(macuse == TRUE)
    {
        pDelayLLInd = (DELAY_LEAVE_LINE_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pDelayLLInd = (DELAY_LEAVE_LINE_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }

//    net_printf("pDelayLLInd->StaNum = %d\n", pDelayLLInd->StaNum);
//    dump_zc(0, "pDelayLLInd MacInfo : ", DEBUG_MDL, pDelayLLInd->MacAddress, pDelayLLInd->StaNum * 6);
//    dump_zc(0, "nnib.macaddr : ", DEBUG_MDL, nnib.MacAddr, 6);


    pMacAddr	= pDelayLLInd->MacAddress;
    for(i = 0; i < pDelayLLInd->StaNum; i++) //ѭ���Ա�������Я����MAC���Լ��Ƿ�һ��
    {
        if(memcmp(pMacAddr, nnib.MacAddr, 6) == 0)
        {
            DelayTime	= pDelayLLInd->DelayTime * 1000; //ϵͳ��ʱ����1MSΪ��λ
            if(DelayTime == 0)
            {
                DelayTime = 2000;
            }
            //��������ָʾ��ʱ��
            modify_LeaveIndTimer(DelayTime);
            break;
        }
        pMacAddr += 6;
    }

    linkid = 1;
    macLen = len;
    DelayTime = rand() % 1024;
    __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)],nnib.MacAddr,6);
    if(pMacPublicHeader->RemainRadius > 0)
    {
        if(buf->LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, BROADCAST_TEI, pMacPublicHeader->SendType,
                          e_MMeLeaveInd, linkid, e_NETMMSG, 0, DelayTime, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, BROADCAST_TEI, pMacPublicHeader->SendType,
                          e_MMeLeaveInd, linkid, e_NETMMSG, 0, DelayTime, e_MainRoute);
        }
        /*
        LongMacDataRequst(macLen, pMacPublicHeader, BROADCAST_TEI, pMacPublicHeader->SendType,
                      e_MMeLeaveInd, linkid, e_NETMMSG, 0, DelayTime);*/
    }

}


static void ProcessMMeZeroCrossNTBCollectInd(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8								macuse;
    U16 							nextTEI;
    MAC_PUBLIC_HEADER_t 			*pMacPublicHeader;
    ZERO_CROSSNTB_COLLECTIND_t		*pZeroInd_t;


    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    if(pMacPublicHeader->DestTEI == GetTEI())
    {
        macuse = pMacPublicHeader->MacAddrUseFlag;
        if(macuse == TRUE)
        {
            pZeroInd_t	= (ZERO_CROSSNTB_COLLECTIND_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
        }
        else
        {
            pZeroInd_t	= (ZERO_CROSSNTB_COLLECTIND_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
        }

        //���ڵ����
        Zero_Result.ColloctType = pZeroInd_t->ColloctStaion;
        Zero_Result.ColloctPeriod = pZeroInd_t->ColloctPeriod;
        Zero_Result.ColloctTEI = GetTEI();
        Zero_Result.ColloctNumber = pZeroInd_t->ColloctNumber;
        if(Zero_Result.ColloctNumber>MAXCOLLNUM)
        {
            Zero_Result.ColloctNumber=MAXCOLLNUM;
        }
        Zero_Result.ColloctSeq = 0;
        Zero_Result.ColloctFlag = TRUE;

        SendMMeZeroCrossNTBReport(TRUE);
    }
    else//ת��
    {
        nextTEI = SearchNextHopAddrFromRouteTable(pMacPublicHeader->DestTEI, ACTIVE_ROUTE);
        if(nextTEI !=INVALID)
        {

            if(NwkRoutingTable[nextTEI - 1].LinkType == e_HPLC_Link)
            {
                MacDataRequst(pMacPublicHeader, len, nextTEI, pMacPublicHeader->SendType,
                              e_MMeZeroCrossNTBCollectInd, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
            }
            else
            {
                MacDataRequstRf(pMacPublicHeader, len, nextTEI, pMacPublicHeader->SendType,
                              e_MMeZeroCrossNTBCollectInd, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
            }
            /*LongMacDataRequst(pMacIndication->MpduLength, pMacPublicHeader, nextTEI, pMacPublicHeader->SendType,
                          e_MMeZeroCrossNTBCollectInd, pMacIndication->LinkId, e_NETMMSG, 0, 0);*/
        }
    }


    return;
}

#endif


#if 0
#if defined(STATIC_MASTER)
/**
 * @brief update_devInfo_by_AsscReq     �����豸�б���Ϣ
 * @param pMacPublicHeader
 * @param pAssciateReq_t
 */
void update_devInfo_by_AsscReq(MAC_PUBLIC_HEADER_t *pMacPublicHeader, ASSOCIATE_REQ_CMD_t *pAssciateReq_t, U16 ListSeq)
{

    DeviceTEIList[ListSeq].ParentTEI = pAssciateReq_t->ParentTEI[0].ParatTEI;
    DeviceTEIList[ListSeq].NodeState = e_NODE_ON_LINE;
    DeviceTEIList[ListSeq].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
    DeviceTEIList[ListSeq].LogicPhase = pAssciateReq_t->PhaseInfo1;
    DeviceTEIList[ListSeq].UplinkSuccRatio = INITIAL_SUCC_RATIO;
    DeviceTEIList[ListSeq].DownlinkSuccRatio = INITIAL_SUCC_RATIO;
    DeviceTEIList[ListSeq].DeviceType = pAssciateReq_t->DeviceType;
    DeviceTEIList[ListSeq].Reset = pMacPublicHeader->RestTims;//���´�ģ����������
    DeviceTEIList[ListSeq].BootVersion =  pAssciateReq_t->VersionInfo.BootVersion;
    DeviceTEIList[ListSeq].SoftVerNum[0] = pAssciateReq_t->VersionInfo.SoftVerNum[0];
    DeviceTEIList[ListSeq].SoftVerNum[1] = pAssciateReq_t->VersionInfo.SoftVerNum[1];
    DeviceTEIList[ListSeq].BuildTime   = pAssciateReq_t->VersionInfo.BuildTime;
    DeviceTEIList[ListSeq].ManufactorCode[0] = pAssciateReq_t->VersionInfo.ManufactorCode[0];
    DeviceTEIList[ListSeq].ManufactorCode[1] = pAssciateReq_t->VersionInfo.ManufactorCode[1];
    DeviceTEIList[ListSeq].ChipVerNum[0] = pAssciateReq_t->VersionInfo.ChipVerNum[0];
    DeviceTEIList[ListSeq].ChipVerNum[1] = pAssciateReq_t->VersionInfo.ChipVerNum[1];
    DeviceTEIList[ListSeq].LinkType = pAssciateReq_t->ParentTEI[0].LinkType;
    DeviceTEIList[ListSeq].ModuleType = pAssciateReq_t->ModuleType;
    __memcpy(DeviceTEIList[ListSeq].ManageID,pAssciateReq_t->ManageID,24);
    __memcpy((U8 *)&DeviceTEIList[ListSeq].ManufactorInfo,(U8 *)&pAssciateReq_t->ManufactorInfo,sizeof(MANUFACTORINFO_t));
    DeviceTEIList[ListSeq].MacType = pAssciateReq_t->MacAddrType;
}
#endif

static void ProcessMMeAssocReq(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    MAC_PUBLIC_HEADER_t 				*pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)pMacInd;
    U8									 macuse = FALSE;
#if defined(STATIC_MASTER)
    U8								   AssociateDstMacAdd[6];
    U32 							   ReplyAssociaRandomNum, ReplyManageMsgSeqNum;
    U16 			   DestTEI, NextHopTEI;

    U8	  assocResult;
    U8	  Result;
    U16   ListSeq = 0;
    U16   WhiteSeq = 0xFFF;

    U16   OldPTei;
    U16   OldNodeType;
#else
    U16 								  macLen;
#endif
    macuse = pMacPublicHeader->MacAddrUseFlag;

    ASSOCIATE_REQ_CMD_t 			   *pAssciateReq_t;
    if(macuse == TRUE)
    {
        pAssciateReq_t = (ASSOCIATE_REQ_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pAssciateReq_t = (ASSOCIATE_REQ_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }
    if(pMacPublicHeader->ScrTEI == 0)
    {
        if(!check_ZEROTEI_msdu(pMacPublicHeader->MsduSeq, pAssciateReq_t->ApplyNodeMacAddr))
        {
//            net_printf("filter ASSOCReq msduSeq = %04x\n", pMacPublicHeader->MsduSeq);
            dump_zc(0, "filter ASSOCReq msduSeq : ", DEBUG_MDL, pAssciateReq_t->ApplyNodeMacAddr, MACADRDR_LENGTH);
            return;
        }
        else
        {
            Add__ZEROTEI_msdu(pMacPublicHeader->MsduSeq, pAssciateReq_t->ApplyNodeMacAddr);
        }
    }
    if(TX_MNG_LINK.nr>TX_MNG_LINK.thr/2) //������Ϣ̫��δ����ȥʱ����Ҫ�����������
    {
        return;
    }
//recordtime("Req2",get_phy_tick_time());


    if(pMacPublicHeader->DestTEI != GetTEI())
    {
#if defined(STATIC_MASTER)
        return ;
#else
        static ZEROTEI_MSDU_t asscmsdu={0,0};
        if(filter_ZEROTEI_msdu(&asscmsdu,pMacPublicHeader->MsduSeq,pMacPublicHeader->ScrTEI) == FALSE)
        {
            return;
        }
        if(pMacPublicHeader->DestTEI != CCO_TEI) //�����м�ת����������Ľڵ㣬��������е�Ŀ���ַ��ΪCCO����ת��
        {
            return;
        }
        macLen = len;
        //macLen = sizeof(ASSOCIATE_REQUEST_COMMAND_t) + sizeof(MAC_LONGDATA_HEADER);

        net_printf("mLkTp:%d\n", nnib.LinkType);
        if(nnib.LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                          e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                            e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
        }
        return;


#endif
    }

#if defined(STATIC_MASTER)

    mg_update_type(STEP_1, netMsgRecordIndex);

    __memcpy(AssociateDstMacAdd , pAssciateReq_t->ApplyNodeMacAddr, 6);
    ReplyAssociaRandomNum = pAssciateReq_t->AssociaRandomNum; //Ӧ������������
    ReplyManageMsgSeqNum =	pAssciateReq_t->ManageMsgSeqNum;  //Ӧ��Ķ˵������

    //�鿴�����ߵ�MAC��ַ�Ƿ��ڰ���������
    Result = CheckTheMacAddrLegal(pAssciateReq_t->ApplyNodeMacAddr, pAssciateReq_t->DeviceType, pAssciateReq_t->MacAddrType,&WhiteSeq);

    mg_update_type(STEP_2, netMsgRecordIndex);

    net_printf("Legal=%d,Reg=%d,DeTp=%d,lkTp=%d,MdTp=%d,", Result, STARegisterFlag,pAssciateReq_t->DeviceType, pAssciateReq_t->ParentTEI[0].LinkType,pAssciateReq_t->ModuleType);
    net_printf("NodeMac=");
    dump_level_buf(DEBUG_NET,pAssciateReq_t->ApplyNodeMacAddr, MACADRDR_LENGTH);
    dump_buf(pAssciateReq_t->ManageID,24);
    //recordtime("Req3",get_phy_tick_time());
    if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO�����ն���
    {
        if(Result == ASSOC_SUCCESS || Result == RE_ASSOC_SUCCESS)
        {
            //�������ڵ���в��ѱ�Ͳ��Ƿ�֧�ַ��Ӽ��ɼ����ܣ������������֮���¼���Ľڵ�ֱ�ӽ��е���RTCʱ��ͬ�������򲻽���RTCʱ��ͬ����
            NodeChange2app(pAssciateReq_t->ApplyNodeMacAddr,MACADRDR_LENGTH);
        }
    }
    if(Result !=ASSOC_SUCCESS)//���� " ���ڰ�������֪ͨ "
    {

        //mg_update_type(STEP_3, netMsgRecordIndex);
        if(Result == NOTIN_WHITELIST)  //2.7�����淶
        {
            area_change_report(pAssciateReq_t->ApplyNodeMacAddr,pAssciateReq_t->DeviceType);
        }
        net_printf("---N_IN_WL\n ");
        SendAssociateCfm(Result, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, macuse);

        //mg_update_type(STEP_4, netMsgRecordIndex);
        return ;
    }

    mg_update_type(STEP_9, netMsgRecordIndex);

    ListSeq = DT_AssignTEI(pAssciateReq_t->ApplyNodeMacAddr);
    //recordtime("Req4",get_phy_tick_time());

    //net_printf("ListSeq ==  %08x DeviceTEIList[ListSeq].NodeType = %02x\n", ListSeq, DeviceTEIList[ListSeq].NodeType);

    if(ListSeq == INVALID)
    {
        //�����ظ�//STA_FULL,�ظ�վ�������������ظ�����
        SendAssociateCfm(STA_FULL, AssociateDstMacAdd, ReplyAssociaRandomNum,
                         ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, macuse);
        return ;
    }


    mg_update_type(STEP_10, netMsgRecordIndex);
    //Դ��ַΪδ�����ڵ�

    off_grid_off_to_on(&DeviceTEIList[ListSeq]);

    //��¼�����ڵ�MAC��ַ�ڰ������е����
    DeviceTEIList[ListSeq].WhiteSeq = WhiteSeq;
    DeviceTEIList[ListSeq].NodeTEI = ListSeq + 2;
    DeviceTEIList[ListSeq].LinkType = pAssciateReq_t->ParentTEI[0].LinkType;

    //��¼�ڵ��Ƿ����״������ͽڵ�ľɴ���TEI��Ϣ��
    OldPTei = DeviceTEIList[ListSeq].ParentTEI;
    OldNodeType = DeviceTEIList[ListSeq].NodeType;

    //��д�������е�ͨ�ŵ�ַ
    AddWhiteListCnmAddrbyTEI(pAssciateReq_t->ApplyNodeMacAddr,WhiteSeq,ListSeq+2,pAssciateReq_t->DeviceType,pAssciateReq_t->MacAddrType);

    if(pAssciateReq_t->ParentTEI[0].ParatTEI == CCO_TEI)
    {
        //���ɵ����������ߵ�·��
        DestTEI = DeviceTEIList[ListSeq].NodeTEI;
        NextHopTEI = DeviceTEIList[ListSeq].NodeTEI;
        UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);

        //��Ϊ1���ڵ㣬����״μ��룬������ΪSTA�����������,ʹ�ù�������ָʾ
        if(DeviceTEIList[ListSeq].NodeType == e_STA_NODETYPE || DeviceTEIList[ListSeq].NodeType == e_UNKNOWN_NODETYPE)
        {
            DeviceTEIList[ListSeq].NodeType = e_STA_NODETYPE;
//            DeviceTEIList[ListSeq].ParentTEI = CCO_TEI;
            //�����豸�б���Ϣ
            update_devInfo_by_AsscReq(pMacPublicHeader, pAssciateReq_t, ListSeq);

            //�����ھӱ�
            if(pAssciateReq_t->ParentTEI[0].LinkType == e_HPLC_Link)
            {
                UpdateNbInfo(GetSNID(),pAssciateReq_t->ApplyNodeMacAddr,ListSeq + 2,1,e_STA_NODETYPE, GetTEI(), e_CHILD, buf->rgain,PHASE_A,0,DT_SOF);
            }
            else
            {
                UpdateRfNbInfo(GetSNID(),pAssciateReq_t->ApplyNodeMacAddr,ListSeq + 2,1,e_STA_NODETYPE, GetTEI(), e_CHILD, buf->rssi,buf->snr,0, nnib.RfCount + 1,DT_SOF);
            }


            //recordtime("ReacordDeepth before",get_phy_tick_time());
            ReacordDeepth(e_MMeAssocGatherInd,ListSeq + 2,0,pAssciateReq_t->ParentTEI[0].ParatTEI,1,DeviceTEIList[ListSeq].NodeType,pAssciateReq_t->VersionInfo.ResetCause);

            //recordtime("SendAssociateGatherInd before",get_phy_tick_time());
            SendAssociateGatherInd(ListSeq + 2);
            g_CnfAssociaRandomNum = ReplyAssociaRandomNum;
            g_CnfManageMsgSeqNum = ReplyManageMsgSeqNum;


        }
        else//�����״μ��룬���һ��Ǵ���ڵ�,ʹ�ù����ظ�����
        {
            //��������������ߵ�ԭ���ڵ���¸��ڵ㲻һ�µĻ�����Ҫ����CCO�������ߵĸ����ڵ��·�ɱ�
            if(pAssciateReq_t->ParentTEI[0].ParatTEI != DeviceTEIList[ListSeq].ParentTEI)
            {
                U16 i=0;

                ChildCount = 0;
                net_printf("--in Net angin CCO-\n");

                DT_SearchAllChildStation(DeviceTEIList[ListSeq].NodeTEI);

                for(i = 0; i < ChildCount; i++)
                {
                    DestTEI = ChildStaIndex[i] + 2;
                    //                        NextHopTEI = DeviceTEIList[ListSeq].NodeTEI;
                    if(!IS_BROADCAST_TEI(NextHopTEI))
                    {
                        UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
                    }
                    DeviceTEIList[ChildStaIndex[i]].NodeDepth -= (DeviceTEIList[ListSeq].NodeDepth - 1);
                    //net_printf("--upone--node:%04x,nodedepth:%d\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI,DeviceTEIList[ChildStaIndex[i]].NodeDepth);
                }
            }

//            DeviceTEIList[ListSeq].ParentTEI = CCO_TEI;
            //�����豸�б���Ϣ
            update_devInfo_by_AsscReq(pMacPublicHeader, pAssciateReq_t, ListSeq);

            ReacordDeepth(e_MMeAssocCfm,DeviceTEIList[ListSeq].NodeTEI,0XFF,pAssciateReq_t->ParentTEI[0].ParatTEI,1,DeviceTEIList[ListSeq].NodeType,pAssciateReq_t->VersionInfo.ResetCause);
            //recordtime("SendAssociateCfm before",get_phy_tick_time());
            SendAssociateCfm(RE_ASSOC_SUCCESS, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum,
                             1, DeviceTEIList[ListSeq].NodeTEI, pAssciateReq_t->ParentTEI[0], 0, macuse);

        }

        //����һ���ڵ�㼶�ʹ�����Ϣ��
        DeviceTEIList[ListSeq].NodeDepth = 1;

    }
    else//�����ڵ�
    {
        U16 proxySeq = 0;
        sof_ctrl_t *pFrame = (sof_ctrl_t *)buf->fi.head;

        if((pAssciateReq_t->ParentTEI[0].ParatTEI <= CCO_TEI) ||
                (pAssciateReq_t->ParentTEI[0].ParatTEI >= NWK_MAX_ROUTING_TABLE_ENTRYS) ) //��ֹ����Խ��
        {
            DelDeviceListEach(ListSeq + 2);	//todo
            return;
        }

        proxySeq = pAssciateReq_t->ParentTEI[0].ParatTEI - 2;

        if(DeviceTEIList[proxySeq].NodeDepth >= 15)//ģ�������ȼ� >15ʱ
        {
            net_printf("---Cfm :DEPTH_OVER,ParentTEI=%04X\n ",pAssciateReq_t->ParentTEI[0].ParatTEI);
            SendAssociateCfm(DEPTH_OVER, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, macuse);
            DelDeviceListEach(ListSeq + 2);//todo
            return;
        }

        if((DeviceTEIList[proxySeq].NodeType == e_STA_NODETYPE) && (nnib.PCOCount >= MAXPCONUM) ) //��PCO��������MAXPCONUMʱ,��������PCO
        {
            net_printf("---err,nnib.PCOCount >= 200\n");
            DelDeviceListEach(ListSeq + 2);//todo
            return;
        }

        if(SearchUplinkNodeTei(DeviceTEIList[ListSeq].NodeTEI,pAssciateReq_t->ParentTEI[0].ParatTEI) ==FALSE)
        {
            net_printf("---SearchUplinkNodeTei err\n");
            DelDeviceListEach(ListSeq + 2);//todo
            return;
        }

        //��ȡ��Ŀ�Ľڵ����һ��·��
        NextHopTEI = SearchNextHopAddrFromRouteTable(pAssciateReq_t->ParentTEI[0].ParatTEI, ACTIVE_ROUTE);

        if(DeviceTEIList[ListSeq].NodeType ==e_UNKNOWN_NODETYPE)//�״�����
        {
            //����������Ļ�ԭ�������ͱ��ֲ��䣬�״���������Ϊe_STA_NODETYPE
            DeviceTEIList[ListSeq].NodeType = e_STA_NODETYPE;
            assocResult = ASSOC_SUCCESS;
        }
        else//�ٴ������Ľڵ�
        {

            net_printf("---in Net angin\n");

            //��������������ߵ�ԭ���ڵ���¸��ڵ㲻һ�µĻ�����Ҫ����CCO�������ߵĸ����ڵ��·�ɱ�
            if(pAssciateReq_t->ParentTEI[0].ParatTEI != DeviceTEIList[ListSeq].ParentTEI)
            {
                U16 i=0;

                ChildCount = 0;
                DT_SearchAllChildStation(DeviceTEIList[ListSeq].NodeTEI);
                //recordtime("Req8",get_phy_tick_time());

                for(i = 0; i < ChildCount; i++)
                {
                    DestTEI = ChildStaIndex[i] + 2;
                    //                        NextHopTEI = SearchNextHopAddrFromRouteTable(pAssciateReq_t->ParentTEI[0].ParatTEI, ACTIVE_ROUTE);
                    if(!IS_BROADCAST_TEI(NextHopTEI))
                    {
                        UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
                    }

                    if(DeviceTEIList[ListSeq].NodeDepth > DeviceTEIList[proxySeq].NodeDepth + 1)//ԭ�ȼ������µȼ�����ģ������
                    {
                        //��ģ��ȼ�= ��ģ��ԭ�ȼ�-( ������ԭ�ȼ�-�������ֵȼ�);
                        DeviceTEIList[ChildStaIndex[i]].NodeDepth -=
                                (DeviceTEIList[ListSeq].NodeDepth-DeviceTEIList[proxySeq].NodeDepth - 1);
                        //net_printf("--up--node:%04x,nodedepth:%d\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI,DeviceTEIList[ChildStaIndex[i]].NodeDepth);
                    }
                    else if(DeviceTEIList[ListSeq].NodeDepth < DeviceTEIList[proxySeq].NodeDepth + 1)//����
                    {
                        //��ģ��ȼ�= ��ģ��ԭ�ȼ�-( �������ֵȼ�-������ԭ�ȼ�);
                        DeviceTEIList[ChildStaIndex[i]].NodeDepth +=
                                (DeviceTEIList[proxySeq].NodeDepth + 1 -DeviceTEIList[ListSeq].NodeDepth);
                        //net_printf("--down--node:%04x,nodedepth:%d\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI,DeviceTEIList[ChildStaIndex[i]].NodeDepth);
                    }
                }

                net_printf("---cco r TEI=%d AsReq,OldPTei=%d,NewPTei=%d,ChildCount=%d\n"
                           ,DeviceTEIList[ListSeq].NodeTEI
                           ,DeviceTEIList[ListSeq].ParentTEI
                           ,pAssciateReq_t->ParentTEI[0].ParatTEI
                           ,ChildCount);

            }
            assocResult = RE_ASSOC_SUCCESS;
        }
        DeviceTEIList[ListSeq].NodeDepth = DeviceTEIList[proxySeq].NodeDepth + 1;
//        DeviceTEIList[ListSeq].ParentTEI = pAssciateReq_t->ParentTEI[0].ParatTEI;
        //�����豸�б���Ϣ
        update_devInfo_by_AsscReq(pMacPublicHeader, pAssciateReq_t, ListSeq);

        //���ɵ����������ߵ�·��
        DestTEI = DeviceTEIList[ListSeq].NodeTEI;
        //            NextHopTEI = SearchNextHopAddrFromRouteTable(pAssciateReq_t->ParentTEI[0].ParatTEI, ACTIVE_ROUTE);
        if(!IS_BROADCAST_TEI(NextHopTEI))
        {
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
        }

        //��ת���߸���Ϊ�����
        if(DeviceTEIList[proxySeq].NodeType != e_PCO_NODETYPE)
        {
            DeviceTEIList[proxySeq].NodeType = e_PCO_NODETYPE;
            nnib.PCOCount++;
            if(nnib.discoverSTACount > 0)
                nnib.discoverSTACount--;

            //���������У����PCO���ڹ����������� "�����ŷ��͵�STA" ��Ϊ�����Ͳ��ڰ��ţ���Ϊ����ÿ�ζ�����
            if(DeviceTEIList[proxySeq].NodeMachine == e_IsSendBeacon)
            {
                DeviceTEIList[proxySeq].NodeMachine = e_HasSendBeacon;
                JoinCtrl_t.ThisLevelJoinNum--;
            }
        }


        ReacordDeepth(e_MMeAssocCfm,DeviceTEIList[ListSeq].NodeTEI,pFrame->stei,DeviceTEIList[ListSeq].ParentTEI,DeviceTEIList[ListSeq].NodeDepth,DeviceTEIList[ListSeq].NodeType,pAssciateReq_t->VersionInfo.ResetCause);
        //                recordtime("SendAssociateCfm 2  before",get_phy_tick_time());

        net_printf("N,assoc R = %d\n", assocResult);
        //recordtime("Req9",get_phy_tick_time());

        SendAssociateCfm(assocResult, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum,
                         DeviceTEIList[ListSeq].NodeDepth, DeviceTEIList[ListSeq].NodeTEI, pAssciateReq_t->ParentTEI[0], 0, macuse);
        //recordtime("Req10",get_phy_tick_time());

    }


    if(pAssciateReq_t->VersionInfo.ResetCause ==0)//�������������������λ��Ϣ
    {
        DeviceTEIList[ListSeq].Phase = 0;
    }

    //���������Ľڵ㣬�������ɹ���������������жϾɴ���ڵ�Ľڵ������Ƿ���Ҫ�����
//    printf_s("Oldp:%03x, Otype:%02x, newP:%03x", OldPTei, OldNodeType, pAssciateReq_t->ParentTEI[0].ParatTEI);
    if(OldNodeType != e_UNKNOWN_NODETYPE && OldPTei != pAssciateReq_t->ParentTEI[0].ParatTEI)
    {
        if(OldPTei > 1 && DeviceTEIList[OldPTei - 2].NodeType  == e_PCO_NODETYPE)
        {
            if(CountproxyNum(OldPTei) == FALSE)
            {
                DeviceTEIList[OldPTei-2].NodeType = e_STA_NODETYPE;
                nnib.discoverSTACount++;
                if(nnib.PCOCount >0)
                {
                    nnib.PCOCount--;
                }
                net_printf("PCO--\n");
            }
        }
    }

    mg_update_type(STEP_11, netMsgRecordIndex);

#else
        net_printf("NTp=:%s\n ",nnib.NodeType == e_PCO_NODETYPE?"PCO":
                                         nnib.NodeType == e_STA_NODETYPE?"STA":
                                            "UNKOWN");


        //��MSDU֡ͷ�е�Ŀ���ַ��ΪCCO��ַ,Դ��ַ����Ϊ�����MAC��ַ

        pMacPublicHeader->DestTEI = CCO_TEI;
        pMacPublicHeader->ScrTEI  = GetTEI();
        pMacPublicHeader->MsduSeq = ++g_MsduSequenceNumber;
        pMacPublicHeader->RemainRadius = MaxDeepth;
        if(macuse == TRUE)
        {
            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)], nnib.MacAddr , 6);

            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + 6] , nnib.CCOMacAddr , 6);
        }
        macLen = len;
        //macLen = sizeof(ASSOCIATE_REQUEST_COMMAND_t) + sizeof(MAC_LONGDATA_HEADER);

        net_printf("nnib.linkType = %d\n", nnib.LinkType);
        if(nnib.LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                          e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                              e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
        }


#endif


}
#else
static void ProcessMMeAssocReq(mbuf_t *buf, U8 *pMacInd, U16 len)
{
//    MACDATA_INDICATION_t				*pMacIndication = (MACDATA_INDICATION_t*)pMacInd;
//    MAC_PUBLIC_HEADER_t 				*pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)pMacIndication->Mpdu;
    MAC_PUBLIC_HEADER_t 				*pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)pMacInd;

    U8									 macuse = FALSE;
	
#if defined(STATIC_NODE)
    U16 								  macLen;
#else
	U8								   AssociateDstMacAdd[6];
    U32 							   ReplyAssociaRandomNum, ReplyManageMsgSeqNum;
    U16 			   DestTEI, NextHopTEI;

	U8	  assocResult;
	U8	  Result;
	U16   ListSeq = 0;
    U16   WhiteSeq = 0xFFF;

#endif
    macuse = pMacPublicHeader->MacAddrUseFlag;


    ASSOCIATE_REQ_CMD_t 			   *pAssciateReq_t;
    if(macuse == TRUE)
    {
        pAssciateReq_t = (ASSOCIATE_REQ_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pAssciateReq_t = (ASSOCIATE_REQ_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }
    if(pMacPublicHeader->ScrTEI == 0)
    {
        if(!check_ZEROTEI_msdu(pMacPublicHeader->MsduSeq, pAssciateReq_t->ApplyNodeMacAddr))
        {
//            net_printf("filter ASSOCReq msduSeq = %04x\n", pMacPublicHeader->MsduSeq);
            dump_zc(0, "filter ASSOCReq msduSeq : ", DEBUG_MDL, pAssciateReq_t->ApplyNodeMacAddr, MACADRDR_LENGTH);
            return;
        }
        else
        {
            Add__ZEROTEI_msdu(pMacPublicHeader->MsduSeq, pAssciateReq_t->ApplyNodeMacAddr);
        }
    }
	if(TX_MNG_LINK.nr>TX_MNG_LINK.thr/2) //������Ϣ̫��δ����ȥʱ����Ҫ�����������
	{
		return;
	}
//recordtime("Req2",get_phy_tick_time());


	if(pMacPublicHeader->DestTEI != GetTEI())
	{
#if defined(STATIC_MASTER)
	  	return ;
#else
		static ZEROTEI_MSDU_t asscmsdu={0,0};
		if(filter_ZEROTEI_msdu(&asscmsdu,pMacPublicHeader->MsduSeq,pMacPublicHeader->ScrTEI) == FALSE)
		{
			return;
		}
		if(pMacPublicHeader->DestTEI != CCO_TEI) //�����м�ת����������Ľڵ㣬��������е�Ŀ���ַ��ΪCCO����ת��
		{
			return;
		}
		macLen = len;
		//macLen = sizeof(ASSOCIATE_REQUEST_COMMAND_t) + sizeof(MAC_LONGDATA_HEADER);

        net_printf("nnibLinkType:%d\n", nnib.LinkType);
        if(nnib.LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                          e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                          e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
        }
		return;

	
#endif
	}

#if defined(STATIC_MASTER)

        //�������õ�������ʽ����ģ��������
        if((nnib.NetworkType == e_SINGLE_HPLC && pAssciateReq_t->ParentTEI[0].LinkType == e_RF_Link) || 
           (nnib.NetworkType == e_SINGLE_HRF  && pAssciateReq_t->ParentTEI[0].LinkType == e_HPLC_Link))
        {
            net_printf("err: Ntp:%d, Lk:%d\n", nnib.NetworkType, pAssciateReq_t->ParentTEI[0].LinkType);
            return;
        }

        mg_update_type(STEP_1, netMsgRecordIndex);

        __memcpy(AssociateDstMacAdd , pAssciateReq_t->ApplyNodeMacAddr, 6);
        ReplyAssociaRandomNum = pAssciateReq_t->AssociaRandomNum; //Ӧ������������
        ReplyManageMsgSeqNum =	pAssciateReq_t->ManageMsgSeqNum;  //Ӧ��Ķ˵������

        //�鿴�����ߵ�MAC��ַ�Ƿ��ڰ���������
        Result = CheckTheMacAddrLegal(pAssciateReq_t->ApplyNodeMacAddr, pAssciateReq_t->DeviceType, pAssciateReq_t->MacAddrType,&WhiteSeq);

        mg_update_type(STEP_2, netMsgRecordIndex);
        
        net_printf("Legal=%d,Reg=%d,DeTp=%d,lkTp=%d,MdTp=%d, RstR=%d, RbtR=%x, "
                    , Result
                    , STARegisterFlag
                    , pAssciateReq_t->DeviceType
                    , pAssciateReq_t->ParentTEI[0].LinkType
                    , pAssciateReq_t->ModuleType
                    , pAssciateReq_t->VersionInfo.ResetCause
                    , pAssciateReq_t->ManufactorInfo.reboot_reason);
        net_printf("NodeMac=%02x%02x%02x%02x%02x%02x\n"
                    , pAssciateReq_t->ApplyNodeMacAddr[0]
                    , pAssciateReq_t->ApplyNodeMacAddr[1]
                    , pAssciateReq_t->ApplyNodeMacAddr[2]
                    , pAssciateReq_t->ApplyNodeMacAddr[3]
                    , pAssciateReq_t->ApplyNodeMacAddr[4]
                    , pAssciateReq_t->ApplyNodeMacAddr[5] );
        // dump_level_buf(DEBUG_NET,pAssciateReq_t->ApplyNodeMacAddr, MACADRDR_LENGTH);

        dump_buf(pAssciateReq_t->ManageID,24);
//recordtime("Req3",get_phy_tick_time());
#ifdef JIANG_SU
        if (Result == ASSOC_SUCCESS || Result == RE_ASSOC_SUCCESS)
        {
            // �������ڵ���в��ѱ�Ͳ��Ƿ�֧�ַ��Ӽ��ɼ����ܣ������������֮���¼���Ľڵ�ֱ�ӽ��е���RTCʱ��ͬ�������򲻽���RTCʱ��ͬ����
            NodeChange2app(pAssciateReq_t->ApplyNodeMacAddr, MACADRDR_LENGTH);
        }
#endif
        if(Result !=ASSOC_SUCCESS)//���� " ���ڰ�������֪ͨ "
        {

            //mg_update_type(STEP_3, netMsgRecordIndex);
            if(Result == NOTIN_WHITELIST)  //2.7�����淶
            {
                area_change_report(pAssciateReq_t->ApplyNodeMacAddr,pAssciateReq_t->DeviceType);
            }
            net_printf("---N_IN_WL\n ");
            SendAssociateCfm(Result, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, macuse);

            //mg_update_type(STEP_4, netMsgRecordIndex);
            return ;
		}


        mg_update_type(STEP_9, netMsgRecordIndex);

        ListSeq = DT_AssignTEI(pAssciateReq_t->ApplyNodeMacAddr);
//recordtime("Req4",get_phy_tick_time());

        //net_printf("ListSeq ==  %08x DeviceTEIList[ListSeq].NodeType = %02x\n", ListSeq, DeviceTEIList[ListSeq].NodeType);

        if(ListSeq == INVALID)
        {

            //�����ظ�//STA_FULL,�ظ�վ�������������ظ�����
            SendAssociateCfm(STA_FULL, AssociateDstMacAdd, ReplyAssociaRandomNum,
                                     ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, macuse);

            return ;
        }


        mg_update_type(STEP_10, netMsgRecordIndex);
        //Դ��ַΪδ�����ڵ�

		off_grid_off_to_on(&DeviceTEIList[ListSeq]);
		
        if(pAssciateReq_t->ParentTEI[0].ParatTEI == CCO_TEI)
        {
            //��Ϊ1���ڵ㣬����״μ��룬������ΪSTA�����������,ʹ�ù�������ָʾ
            if(DeviceTEIList[ListSeq].NodeType == e_STA_NODETYPE || DeviceTEIList[ListSeq].NodeType == e_UNKNOWN_NODETYPE)
            {
                //��д�������е�ͨ�ŵ�ַ
                if(DeviceTEIList[ListSeq].NodeType == e_UNKNOWN_NODETYPE)
                    AddWhiteListCnmAddrbyTEI(pAssciateReq_t->ApplyNodeMacAddr,WhiteSeq,ListSeq+2,pAssciateReq_t->DeviceType,pAssciateReq_t->MacAddrType);
                    //AddWhiteListCnmAddr(pAssciateReq_t->ApplyNodeMacAddr,pAssciateReq_t->DeviceType,pAssciateReq_t->MacAddrType);
                DeviceTEIList[ListSeq].WhiteSeq = WhiteSeq;
                __memcpy(DeviceTEIList[ListSeq].ManageID,pAssciateReq_t->ManageID,24);
                //CCOΪ�½ڵ�����豸�б�
                DeviceTEIList[ListSeq].NodeTEI = ListSeq + 2;
                __memcpy(DeviceTEIList[ListSeq].MacAddr , pAssciateReq_t->ApplyNodeMacAddr, 6);
                DeviceTEIList[ListSeq].DeviceType = pAssciateReq_t->DeviceType;
                DeviceTEIList[ListSeq].LogicPhase = pAssciateReq_t->PhaseInfo1;
                DeviceTEIList[ListSeq].NodeType = e_STA_NODETYPE;
                DeviceTEIList[ListSeq].NodeState = e_NODE_ON_LINE;
                DeviceTEIList[ListSeq].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
                DeviceTEIList[ListSeq].NodeDepth = 1;
                DeviceTEIList[ListSeq].Reset = pMacPublicHeader->RestTims;
                DeviceTEIList[ListSeq].ParentTEI = 1;
                DeviceTEIList[ListSeq].UplinkSuccRatio = INITIAL_SUCC_RATIO;
                DeviceTEIList[ListSeq].DownlinkSuccRatio = INITIAL_SUCC_RATIO;
                DeviceTEIList[ListSeq].BootVersion =  pAssciateReq_t->VersionInfo.BootVersion;
                DeviceTEIList[ListSeq].SoftVerNum[0] = pAssciateReq_t->VersionInfo.SoftVerNum[0];
                DeviceTEIList[ListSeq].SoftVerNum[1] = pAssciateReq_t->VersionInfo.SoftVerNum[1];
                DeviceTEIList[ListSeq].BuildTime   = pAssciateReq_t->VersionInfo.BuildTime;
                DeviceTEIList[ListSeq].ManufactorCode[0] = pAssciateReq_t->VersionInfo.ManufactorCode[0];
                DeviceTEIList[ListSeq].ManufactorCode[1] = pAssciateReq_t->VersionInfo.ManufactorCode[1];
                DeviceTEIList[ListSeq].ChipVerNum[0] = pAssciateReq_t->VersionInfo.ChipVerNum[0];
                DeviceTEIList[ListSeq].ChipVerNum[1] = pAssciateReq_t->VersionInfo.ChipVerNum[1];
                DeviceTEIList[ListSeq].LinkType = pAssciateReq_t->ParentTEI[0].LinkType;
                DeviceTEIList[ListSeq].ModuleType = pAssciateReq_t->ModuleType;
                __memcpy((U8 *)&DeviceTEIList[ListSeq].ManufactorInfo,(U8 *)&pAssciateReq_t->ManufactorInfo,sizeof(MANUFACTORINFO_t));
                DeviceTEIList[ListSeq].MacType = pAssciateReq_t->MacAddrType;

                //���ɵ����������ߵ�·��



                //recordtime("UpdateRoutingTableEntry before",get_phy_tick_time());
                DestTEI = DeviceTEIList[ListSeq].NodeTEI;
                NextHopTEI = DeviceTEIList[ListSeq].NodeTEI;
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);

                //�����ھӱ�

                if(pAssciateReq_t->ParentTEI[0].LinkType == e_HPLC_Link)
                {
                    UpdateNbInfo(GetSNID(),pAssciateReq_t->ApplyNodeMacAddr,ListSeq + 2,1,e_STA_NODETYPE, GetTEI(), e_CHILD, buf->rgain,PHASE_A,0,DT_SOF);
                }
                else
                {
                    UpdateRfNbInfo(GetSNID(),pAssciateReq_t->ApplyNodeMacAddr,ListSeq + 2,1,e_STA_NODETYPE, GetTEI(), e_CHILD, buf->rssi,buf->snr,0, nnib.RfCount + 1,DT_SOF);
                }

                //net_printf("-----------------DeviceTEIList[ListSeq].NodeType=%d--------------------\n",DeviceTEIList[ListSeq].NodeType);
                //��д�������е�ͨ�ŵ�ַ

                //recordtime("ReacordDeepth before",get_phy_tick_time());
                ReacordDeepth(e_MMeAssocGatherInd,ListSeq + 2,0,pAssciateReq_t->ParentTEI[0].ParatTEI,1,DeviceTEIList[ListSeq].NodeType,pAssciateReq_t->VersionInfo.ResetCause);
                if(pAssciateReq_t->VersionInfo.ResetCause ==0)//�������������������λ��Ϣ
                {
                    DeviceTEIList[ListSeq].Phase = 0;
                }

                //recordtime("SendAssociateGatherInd before",get_phy_tick_time());
                SendAssociateGatherInd(ListSeq + 2);
                g_CnfAssociaRandomNum = ReplyAssociaRandomNum;
                g_CnfManageMsgSeqNum = ReplyManageMsgSeqNum;


            }
            else//�����״μ��룬���һ��Ǵ���ڵ�,ʹ�ù����ظ�����
            {
                DeviceTEIList[ListSeq].NodeState = e_NODE_ON_LINE;
                DeviceTEIList[ListSeq].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
                DeviceTEIList[ListSeq].LogicPhase = pAssciateReq_t->PhaseInfo1;
                DeviceTEIList[ListSeq].UplinkSuccRatio = INITIAL_SUCC_RATIO;
                DeviceTEIList[ListSeq].DownlinkSuccRatio = INITIAL_SUCC_RATIO;
                DeviceTEIList[ListSeq].DeviceType = pAssciateReq_t->DeviceType;
                DeviceTEIList[ListSeq].Reset = pMacPublicHeader->RestTims;//���´�ģ����������
                DeviceTEIList[ListSeq].BootVersion =  pAssciateReq_t->VersionInfo.BootVersion;
                DeviceTEIList[ListSeq].SoftVerNum[0] = pAssciateReq_t->VersionInfo.SoftVerNum[0];
                DeviceTEIList[ListSeq].SoftVerNum[1] = pAssciateReq_t->VersionInfo.SoftVerNum[1];
                DeviceTEIList[ListSeq].BuildTime   = pAssciateReq_t->VersionInfo.BuildTime;
                DeviceTEIList[ListSeq].ManufactorCode[0] = pAssciateReq_t->VersionInfo.ManufactorCode[0];
                DeviceTEIList[ListSeq].ManufactorCode[1] = pAssciateReq_t->VersionInfo.ManufactorCode[1];
                DeviceTEIList[ListSeq].ChipVerNum[0] = pAssciateReq_t->VersionInfo.ChipVerNum[0];
                DeviceTEIList[ListSeq].ChipVerNum[1] = pAssciateReq_t->VersionInfo.ChipVerNum[1];
                DeviceTEIList[ListSeq].LinkType = pAssciateReq_t->ParentTEI[0].LinkType;
                DeviceTEIList[ListSeq].ModuleType = pAssciateReq_t->ModuleType;
                __memcpy(DeviceTEIList[ListSeq].ManageID,pAssciateReq_t->ManageID,24);
                DeviceTEIList[ListSeq].WhiteSeq = WhiteSeq;
                __memcpy((U8 *)&DeviceTEIList[ListSeq].ManufactorInfo,(U8 *)&pAssciateReq_t->ManufactorInfo,sizeof(MANUFACTORINFO_t));
                DeviceTEIList[ListSeq].MacType = pAssciateReq_t->MacAddrType;

                //���ɵ����������ߵ�·��
                DestTEI = DeviceTEIList[ListSeq].NodeTEI;
                NextHopTEI = DeviceTEIList[ListSeq].NodeTEI;
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);

                //��������������ߵ�ԭ���ڵ���¸��ڵ㲻һ�µĻ�����Ҫ����CCO�������ߵĸ����ڵ��·�ɱ�
                if(pAssciateReq_t->ParentTEI[0].ParatTEI != DeviceTEIList[ListSeq].ParentTEI)
                {
                    U16 i=0;

                    ChildCount = 0;
                    net_printf("---------in Net angin CCO---\n");

                    DT_SearchAllChildStation(DeviceTEIList[ListSeq].NodeTEI);

                    for(i = 0; i < ChildCount; i++)
                    {
                        DestTEI = ChildStaIndex[i] + 2;
//                        NextHopTEI = DeviceTEIList[ListSeq].NodeTEI;
                        if(!IS_BROADCAST_TEI(NextHopTEI))
                        {
                            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
                        }
                        DeviceTEIList[ChildStaIndex[i]].NodeDepth -= (DeviceTEIList[ListSeq].NodeDepth - 1);
                        //net_printf("--upone--node:%04x,nodedepth:%d\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI,DeviceTEIList[ChildStaIndex[i]].NodeDepth);
                    }
                }
                DeviceTEIList[ListSeq].NodeDepth = 1;//���·����ߵĲ㼶
                DeviceTEIList[ListSeq].ParentTEI = 1;

                ReacordDeepth(e_MMeAssocCfm,DeviceTEIList[ListSeq].NodeTEI,0XFF,pAssciateReq_t->ParentTEI[0].ParatTEI,1,DeviceTEIList[ListSeq].NodeType,pAssciateReq_t->VersionInfo.ResetCause);
                if(pAssciateReq_t->VersionInfo.ResetCause ==0)//�������������������λ��Ϣ
                {
                    DeviceTEIList[ListSeq].Phase = 0;
                }
                //recordtime("SendAssociateCfm before",get_phy_tick_time());d
                SendAssociateCfm(RE_ASSOC_SUCCESS, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum,
                                 1, DeviceTEIList[ListSeq].NodeTEI, pAssciateReq_t->ParentTEI[0], 0, macuse);


            }
        }
        else//�����ڵ�
        {
            U16 proxySeq = 0;
			sof_ctrl_t *pFrame = (sof_ctrl_t *)buf->fi.head;
            DeviceTEIList[ListSeq].NodeTEI = ListSeq + 2;
            if((pAssciateReq_t->ParentTEI[0].ParatTEI <= CCO_TEI) ||
                    (pAssciateReq_t->ParentTEI[0].ParatTEI >= NWK_MAX_ROUTING_TABLE_ENTRYS) ) //��ֹ����Խ��
            {
                if(DeviceTEIList[ListSeq].NodeType ==e_UNKNOWN_NODETYPE)
                {
                    if(nnib.discoverSTACount>0)
                    {
						nnib.discoverSTACount--;
					}
					DelDeviceListEach(ListSeq +2);
                }
                return;
            }

            proxySeq = pAssciateReq_t->ParentTEI[0].ParatTEI - 2;
            if(DeviceTEIList[proxySeq].NodeDepth >= 15)//ģ�������ȼ� >15ʱ
            {
                net_printf("---Cfm :DEPTH_OVER,ParentTEI=%04X\n ",pAssciateReq_t->ParentTEI[0].ParatTEI);
                SendAssociateCfm(DEPTH_OVER, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, macuse);
            	if(DeviceTEIList[ListSeq].NodeType ==e_UNKNOWN_NODETYPE)
                {
                    if(nnib.discoverSTACount>0)
                    {
						nnib.discoverSTACount--;
					}
					DelDeviceListEach(ListSeq +2);
                }
                return;
            }

            if((DeviceTEIList[proxySeq].NodeType == e_STA_NODETYPE) && (nnib.PCOCount >= MAXPCONUM) ) //��PCO��������MAXPCONUMʱ,��������PCO
            {
                net_printf("---err,nnib.PCOCount >= 200\n");
            	if(DeviceTEIList[ListSeq].NodeType ==e_UNKNOWN_NODETYPE)
                {
                    if(nnib.discoverSTACount>0)
                    {
						nnib.discoverSTACount--;
					}
					DelDeviceListEach(ListSeq +2);
                }
                return;
            }

            DeviceTEIList[ListSeq].Reset = pMacPublicHeader->RestTims;//���´�ģ����������
            DeviceTEIList[ListSeq].NodeTEI = ListSeq + 2;
            __memcpy(DeviceTEIList[ListSeq].MacAddr , pAssciateReq_t->ApplyNodeMacAddr, 6);
            DeviceTEIList[ListSeq].DeviceType = pAssciateReq_t->DeviceType;
            //DeviceTEIList[ListSeq].Phase = pAssciateReq_t->PhaseInfo1;

            //��д�������е�ͨ�ŵ�ַ
            AddWhiteListCnmAddrbyTEI(pAssciateReq_t->ApplyNodeMacAddr,WhiteSeq,ListSeq+2,pAssciateReq_t->DeviceType,pAssciateReq_t->MacAddrType);
            __memcpy(DeviceTEIList[ListSeq].ManageID,pAssciateReq_t->ManageID,24);
            DeviceTEIList[ListSeq].WhiteSeq = WhiteSeq;

            //��ȡ��Ŀ�Ľڵ����һ��·��
            NextHopTEI = SearchNextHopAddrFromRouteTable(pAssciateReq_t->ParentTEI[0].ParatTEI, ACTIVE_ROUTE);


            if(DeviceTEIList[ListSeq].NodeType ==e_UNKNOWN_NODETYPE)//�״�����
            {
                 //����������Ļ�ԭ�������ͱ��ֲ��䣬�״���������Ϊe_STA_NODETYPE
                 DeviceTEIList[ListSeq].NodeType = e_STA_NODETYPE;

                 assocResult = ASSOC_SUCCESS;
            }
            else//�ٴ������Ľڵ�
            {
                if(SearchUplinkNodeTei(DeviceTEIList[ListSeq].NodeTEI,pAssciateReq_t->ParentTEI[0].ParatTEI) ==FALSE)
                {
                    net_printf("---SearchUplinkNodeTei err\n");
                    return;
                }
                net_printf("---in Net angin\n");

                //��������������ߵ�ԭ���ڵ���¸��ڵ㲻һ�µĻ�����Ҫ����CCO�������ߵĸ����ڵ��·�ɱ�
                if(pAssciateReq_t->ParentTEI[0].ParatTEI != DeviceTEIList[ListSeq].ParentTEI)
                {
                    U16 i=0;

                    ChildCount = 0;
					ChildMaxDeepth = 0;
					DT_SearchAllChildStation(DeviceTEIList[ListSeq].NodeTEI);
					if(DeviceTEIList[ListSeq].NodeDepth < DeviceTEIList[pAssciateReq_t->ParentTEI[0].ParatTEI-2].NodeDepth+1)
					{
						if((ChildMaxDeepth-DeviceTEIList[ListSeq].NodeDepth+DeviceTEIList[pAssciateReq_t->ParentTEI[0].ParatTEI-2].NodeDepth+1)>MaxDeepth)
						{
							net_printf("warning:deepthovershoot assoc,childmax:%d,sta:%d,newproxy:%d\n",
								ChildMaxDeepth,
								DeviceTEIList[ListSeq].NodeDepth,
								DeviceTEIList[pAssciateReq_t->ParentTEI[0].ParatTEI-2].NodeDepth);
							
							//RANL_add(pAssciateReq_t->ApplyNodeMacAddr);
							
							//SendAssociateCfm(DEPTH_OVER, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum, 0, 0, pAssciateReq_t->ParentTEI[0], 0, pAssciateReq_t->GD_manageMessageheader.OriginalSrcAddr);
							//����15���������ȫ���ӽڵ㷢����ָʾ
							U16 childnum;
							U8 *mac_list = zc_malloc_mem(ChildCount*MAC_ADDR_LEN, "leave_mac_list", MEM_TIME_OUT);
							for(childnum =0; childnum<ChildCount; childnum++)
							{
								__memcpy(mac_list+childnum*MAC_ADDR_LEN,DeviceTEIList[DeviceTEIList[ChildStaIndex[childnum]].NodeTEI-2].MacAddr,MAC_ADDR_LEN);
							}
							SendMMeDelayLeaveOfNum(mac_list,childnum,0,e_LEAVE_WAIT_SELF_OUTLINE);
							zc_free_mem(mac_list);
							return;
						}
					}

                    for(i = 0; i < ChildCount; i++)
                    {
                        DestTEI = ChildStaIndex[i] + 2;
//                        NextHopTEI = SearchNextHopAddrFromRouteTable(pAssciateReq_t->ParentTEI[0].ParatTEI, ACTIVE_ROUTE);
                        if(!IS_BROADCAST_TEI(NextHopTEI))
                        {
                            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
                        }

                        if(DeviceTEIList[ListSeq].NodeDepth > DeviceTEIList[proxySeq].NodeDepth + 1)//ԭ�ȼ������µȼ�����ģ������
                        {
                            //��ģ��ȼ�= ��ģ��ԭ�ȼ�-( ������ԭ�ȼ�-�������ֵȼ�);
                            DeviceTEIList[ChildStaIndex[i]].NodeDepth -=
                                    (DeviceTEIList[ListSeq].NodeDepth-DeviceTEIList[proxySeq].NodeDepth - 1);
                            //net_printf("--up--node:%04x,nodedepth:%d\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI,DeviceTEIList[ChildStaIndex[i]].NodeDepth);
                        }
                        else if(DeviceTEIList[ListSeq].NodeDepth < DeviceTEIList[proxySeq].NodeDepth + 1)//����
                        {
                            //��ģ��ȼ�= ��ģ��ԭ�ȼ�-( �������ֵȼ�-������ԭ�ȼ�);
                            DeviceTEIList[ChildStaIndex[i]].NodeDepth +=
                                    (DeviceTEIList[proxySeq].NodeDepth + 1 -DeviceTEIList[ListSeq].NodeDepth);
                            //net_printf("--down--node:%04x,nodedepth:%d\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI,DeviceTEIList[ChildStaIndex[i]].NodeDepth);
                        }
                    }

                    net_printf("---cco r TEI=%d AsReq,OldPTei=%d,NewPTei=%d,ChildCount=%d\n"
                        ,DeviceTEIList[ListSeq].NodeTEI
                        ,DeviceTEIList[ListSeq].ParentTEI
                        ,pAssciateReq_t->ParentTEI[0].ParatTEI

                        ,ChildCount);

                }


                assocResult = RE_ASSOC_SUCCESS;
            }

            DeviceTEIList[ListSeq].NodeState = e_NODE_ON_LINE;
            DeviceTEIList[ListSeq].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
            //������CCO��ѡ��
            DeviceTEIList[ListSeq].ParentTEI = pAssciateReq_t->ParentTEI[0].ParatTEI;

            proxySeq = DeviceTEIList[ListSeq].ParentTEI - 2;
            DeviceTEIList[ListSeq].NodeDepth = DeviceTEIList[proxySeq].NodeDepth + 1;
            DeviceTEIList[ListSeq].DeviceType = pAssciateReq_t->DeviceType;
            DeviceTEIList[ListSeq].LogicPhase = pAssciateReq_t->PhaseInfo1;
            DeviceTEIList[ListSeq].UplinkSuccRatio = INITIAL_SUCC_RATIO;
            DeviceTEIList[ListSeq].DownlinkSuccRatio = INITIAL_SUCC_RATIO;
            DeviceTEIList[ListSeq].BootVersion =  pAssciateReq_t->VersionInfo.BootVersion;
            __memcpy(DeviceTEIList[ListSeq].ManageID,pAssciateReq_t->ManageID,24);
            DeviceTEIList[ListSeq].WhiteSeq = WhiteSeq;
            DeviceTEIList[ListSeq].SoftVerNum[0] = pAssciateReq_t->VersionInfo.SoftVerNum[0];
            DeviceTEIList[ListSeq].SoftVerNum[1] = pAssciateReq_t->VersionInfo.SoftVerNum[1];
            DeviceTEIList[ListSeq].BuildTime   = pAssciateReq_t->VersionInfo.BuildTime;
            DeviceTEIList[ListSeq].ManufactorCode[0] = pAssciateReq_t->VersionInfo.ManufactorCode[0];
            DeviceTEIList[ListSeq].ManufactorCode[1] = pAssciateReq_t->VersionInfo.ManufactorCode[1];
            DeviceTEIList[ListSeq].ChipVerNum[0] = pAssciateReq_t->VersionInfo.ChipVerNum[0];
            DeviceTEIList[ListSeq].ChipVerNum[1] = pAssciateReq_t->VersionInfo.ChipVerNum[1];
            DeviceTEIList[ListSeq].LinkType = pAssciateReq_t->ParentTEI[0].LinkType;
            DeviceTEIList[ListSeq].ModuleType = pAssciateReq_t->ModuleType;
            __memcpy((U8 *)&DeviceTEIList[ListSeq].ManufactorInfo,(U8 *)&pAssciateReq_t->ManufactorInfo,sizeof(MANUFACTORINFO_t));

            //���ɵ����������ߵ�·��
            DestTEI = DeviceTEIList[ListSeq].NodeTEI;
//            NextHopTEI = SearchNextHopAddrFromRouteTable(pAssciateReq_t->ParentTEI[0].ParatTEI, ACTIVE_ROUTE);
            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
            }

            //��ת���߸���Ϊ�����
            if(DeviceTEIList[proxySeq].NodeType != e_PCO_NODETYPE)
            {
                DeviceTEIList[proxySeq].NodeType = e_PCO_NODETYPE;
                nnib.PCOCount++;
                if(nnib.discoverSTACount > 0)
                    nnib.discoverSTACount--;

                //���������У����PCO���ڹ����������� "�����ŷ��͵�STA" ��Ϊ�����Ͳ��ڰ��ţ���Ϊ����ÿ�ζ�����
                if(DeviceTEIList[proxySeq].NodeMachine == e_IsSendBeacon)
                {
                    DeviceTEIList[proxySeq].NodeMachine = e_HasSendBeacon;
                    JoinCtrl_t.ThisLevelJoinNum--;
                }
            }


            ReacordDeepth(e_MMeAssocCfm,DeviceTEIList[ListSeq].NodeTEI,pFrame->stei,DeviceTEIList[ListSeq].ParentTEI,DeviceTEIList[ListSeq].NodeDepth,DeviceTEIList[ListSeq].NodeType,pAssciateReq_t->VersionInfo.ResetCause);
            if(pAssciateReq_t->VersionInfo.ResetCause ==0)//�������������������λ��Ϣ
            {
                DeviceTEIList[ListSeq].Phase = 0;
            }
//          recordtime("SendAssociateCfm 2  before",get_phy_tick_time());

            net_printf("N,assoc R = %d\n", assocResult);
//recordtime("Req9",get_phy_tick_time());

            SendAssociateCfm(assocResult, AssociateDstMacAdd, ReplyAssociaRandomNum, ReplyManageMsgSeqNum,
                             DeviceTEIList[ListSeq].NodeDepth, DeviceTEIList[ListSeq].NodeTEI, pAssciateReq_t->ParentTEI[0], 0, macuse);
//recordtime("Req10",get_phy_tick_time());



        }

        net_printf("node_tei=%d \n", DeviceTEIList[ListSeq].NodeTEI);
        mg_update_type(STEP_11, netMsgRecordIndex);


#else
        net_printf("NTp=:%s\n ",nnib.NodeType == e_PCO_NODETYPE?"PCO":
                                         nnib.NodeType == e_STA_NODETYPE?"STA":
                                            "UNKOWN");


        //��MSDU֡ͷ�е�Ŀ���ַ��ΪCCO��ַ,Դ��ַ����Ϊ�����MAC��ַ

        pMacPublicHeader->DestTEI = CCO_TEI;
        pMacPublicHeader->ScrTEI  = GetTEI();
        pMacPublicHeader->MsduSeq = ++g_MsduSequenceNumber;
        pMacPublicHeader->RemainRadius = MaxDeepth;
        if(macuse == TRUE)
        {
            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)], nnib.MacAddr , 6);

            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + 6] , nnib.CCOMacAddr , 6);
        }
        macLen = len;
        //macLen = sizeof(ASSOCIATE_REQUEST_COMMAND_t) + sizeof(MAC_LONGDATA_HEADER);

        net_printf("nnib.linkType = %d\n", nnib.LinkType);
        if(nnib.LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                          e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                              e_MMeAssocReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
        }


#endif


}
#endif


void ProcessMMeAssocCnf(mbuf_t *buf, U8 *pMacInd, U16 len)
{
#if defined(STATIC_NODE)
    U8									 macuse = FALSE;
    U16 								 nextTEI = 0;
//    U16 								 macLen = 0;
    ASSOCIATE_CFM_CMD_t 				*pAssociateCfm;
    MAC_PUBLIC_HEADER_t 				*pMacPublicHeader;


    U16 DestTEI = 0;
    U16 NextHopTEI = 0;

    U16 offset = 0;
    U16 i, j;
    U16 staNum;



    //U8									RecordFlag=TRUE;
    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    macuse = pMacPublicHeader->MacAddrUseFlag;
    if(macuse == TRUE)
    {
        pAssociateCfm = (ASSOCIATE_CFM_CMD_t *)(pMacInd + sizeof(MAC_LONGDATA_HEADER));
    }
    else
    {
        pAssociateCfm = (ASSOCIATE_CFM_CMD_t *)(pMacInd + sizeof(MAC_PUBLIC_HEADER_t));
    }

//    net_printf("ProcessMMeAssocCnf: Dest macAddr : ");
//    dump_level_buf(DEBUG_NET, pAssociateCfm->DstMacAddr, MACADRDR_LENGTH);
    if(nnib.MacAddrType == e_UNKONWN)
    {
        return ;
    }
    if(((pMacPublicHeader->DestTEI == GetTEI()) && (nnib.NodeState == e_NODE_ON_LINE)) ||  //�����ڵ�ʹ��TEI�жϣ�Ϊ����ʹ��MACADDR
           (memcmp(pAssociateCfm->DstMacAddr, nnib.MacAddr, MACADRDR_LENGTH) == 0)) //�����е�MAC
    {

        if(memcmp(pAssociateCfm->DstMacAddr, nnib.MacAddr, MACADRDR_LENGTH ) == 0) //���ڵ�Ϊ�����ظ���Ŀ�Ľڵ�
        {

            ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable=NULL;
			//����������֤
			U8  AppID[2] = {APPLICATION_ID0,APPLICATION_ID1};
			U8  ApplicationCheck = TRUE;
            if(pAssociateCfm->Result == NO_WHITELIST) //CCOû�����ð�����
            {
                //if(TMR_RUNNING==zc_timer_query(g_WaitReqFail))
                //timer_stop(g_WaitReqFail,TMR_CALLBACK);
                net_printf("pAssocCfm,NO_W %d,ReTime=%d\n",pAssociateCfm->ReassociaTime/1000);
                //û�а������������������������̬���ú�����ʱ��
                //nbNetNum = GetNeighborNetNum();
                //nbNetNum =nbNetNum == 0 ? 1 : nbNetNum;
                //AddBlackNeigbor(GetSNID(), BLACKNEIGBORLIFE , e_NotInlist);
                //net_printf("pAssociateCfm->ReassociaTime = %d\n",pAssociateCfm->ReassociaTime/1000);
                if(pAssociateCfm->ReassociaTime != 0)
                {
                	if(pAssociateCfm->ReassociaTime < (BADLINKTIME*1000))
						AddBlackNeigbor(GetSNID() , pAssociateCfm->ReassociaTime/1000 , e_NotInlist);
                    else
						AddBlackNeigbor(GetSNID() , BADLINKTIME , e_NotInlist);
                }
                else
                {
                    AddBlackNeigbor(GetSNID() , BLACKNEIGBORLIFE , e_NotInlist);
                }
                return;
            }

			if(PROVINCE_HUNAN==app_ext_info.province_code)  		//����������֤��Ҫ����
			{
				if((pAssociateCfm->Result ==ASSOC_SUCCESS || pAssociateCfm->Result ==RE_ASSOC_SUCCESS)&&
                (DevicePib_t.HNOnLineFlg == TRUE&&memcmp(pAssociateCfm->ApplicationID,AppID,2)!=0))
	            {
	                ApplicationCheck = FALSE;//����������֤���ؿ�����������֤ʧ�ܣ�������
	                net_printf("pAssociateCfm_t,it's not YX\n");
	            }  
			}
            if((pAssociateCfm->Result != ASSOC_SUCCESS && pAssociateCfm->Result != RE_ASSOC_SUCCESS) || ApplicationCheck == FALSE)//�����ظ����Ϊ���ɹ�ʱ��Ӧ�÷���
            {
                if(GetSNID() == 0 || nnib.NodeState == e_NODE_ON_LINE)//
                {
                    app_printf("SNID clean,waring!!!!!!!!!!!!!!!!!!!!!\n");
                    return;
                }

                if(pAssociateCfm->ReassociaTime !=0 )
                {
                	if(pAssociateCfm->ReassociaTime < (BADLINKTIME*1000))
						AddBlackNeigbor(GetSNID() , pAssociateCfm->ReassociaTime/1000 , e_NotInlist);
                    else
						AddBlackNeigbor(GetSNID() , BADLINKTIME , e_NotInlist);
                }
                else
                {
                    AddBlackNeigbor(GetSNID(),BLACKNEIGBORLIFE,e_NotInlist);
                }

                net_printf("---Result !=OK\n");
                return;
            }

            //���Ʊ��ڵ���Ϣ
			nnib.LinkType = pAssociateCfm->LinkType;
            net_printf("nnib.LinkType : %d\n", nnib.LinkType);
            
            //����������·���ͣ�����Ĭ��������ʽ����������ݰ��ŵ��ű����͸��¡�
            if(nnib.LinkType == e_RF_Link)
            { 
                nnib.NetworkType = e_SINGLE_HRF;
            }
            else
            {
                nnib.NetworkType = e_SINGLE_HPLC;
            }
            //�����ͨ���������������ز�Ƶ��
            if(nnib.LinkType == e_RF_Link)
            {
                changeband(pAssociateCfm->HplcBand);
            }
//          timer_stop(g_WaitBandInNetTimeout, TMR_NULL);//��ֹ����ص�����,�л�Ƶ��
            if(DefSetInfo.FreqInfo.FreqBand != GetHplcBand()) //���������޸���Ƶ��ʱ��Ҫ����
            {
                DefSetInfo.FreqInfo.FreqBand = GetHplcBand();
                DefwriteFg.FREQ = TRUE;
                DefaultSettingFlag = TRUE;
            }

            //SavePowerlevel();
            __memcpy(nnib.CCOMacAddr , pAssociateCfm->CCOMacAddr, 6);
            SetTEI(pAssociateCfm->NodeTEI);
            nnib.NodeState = e_NODE_ON_LINE;
            nnib.StaOfflinetime = nnib.RoutePeriodTime*2;
            nnib.RecvDisListTime = nnib.RoutePeriodTime*4;
            nnib.RfRecvDisListTime = nnib.RoutePeriodTime*4;
            nnib.FormationSeqNumber = pMacPublicHeader->FormationSeq;

            if((pAssociateCfm->RouteTableInfo.DirectConnectNodeNum+
                pAssociateCfm->RouteTableInfo.DirectConnectProxyNum) > 0)
            {
                nnib.NodeType = e_PCO_NODETYPE;
            }
            else
            {
                nnib.NodeType = e_STA_NODETYPE;
            }
			
            SetProxy(pAssociateCfm->ProxyTEI) ;
            nnib.NodeLevel = pAssociateCfm->NodeDeep;
            net_printf("---PAsCnf,OK\n");
            net_printf("------StaOfflinetime =%d----------\n",nnib.StaOfflinetime);
            net_printf("------RecvDisListTime=%d----------\n",nnib.RecvDisListTime);
            net_printf("------RfRecvDisListTime=%d----------\n",nnib.RfRecvDisListTime);
            Trigger2app(TO_ONLINE);
            ScanBandManage(e_ONLINEEVENT, buf->snid);

            //�����ھӱ�
            DiscvryTable = GetNeighborByTEI(GetProxy(), e_HPLC_Link);
            if(DiscvryTable)
            {
                __memcpy(nnib.ParentMacAddr , DiscvryTable->MacAddr, 6);
                DiscvryTable->NodeType = e_PCO_NODETYPE;
                DiscvryTable->Relation = e_PROXY;
                DiscvryTable->BKRouteFg = TRUE;
                DiscvryTable->HplcInfo.DownlinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->HplcInfo.UplinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->HplcInfo.LastDownlinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->HplcInfo.LastUplinkSuccRatio = NICE_SUCC_RATIO;
                DiscvryTable->ResetTimes = pMacPublicHeader->RestTims;
            }


            ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pRfNodeInfo;
            pRfNodeInfo = GetNeighborByTEI(GetProxy(), e_RF_Link);
            if(pRfNodeInfo)
            {
                __memcpy(nnib.ParentMacAddr , pRfNodeInfo->MacAddr, 6);
                pRfNodeInfo->NodeType = e_PCO_NODETYPE;
                pRfNodeInfo->Relation = e_PROXY;
                pRfNodeInfo->BKRouteFg = TRUE;
                pRfNodeInfo->ResetTimes = pMacPublicHeader->RestTims;
                pRfNodeInfo->HrfInfo.DownRcvRate = NICE_SUCC_RATIO;
                pRfNodeInfo->HrfInfo.UpRcvRate = NICE_SUCC_RATIO;
                pRfNodeInfo->HrfInfo.LDownRcvRate = NICE_SUCC_RATIO;
                pRfNodeInfo->HrfInfo.LUpRcvRate = NICE_SUCC_RATIO;
            }
            
            nnib.ProxyNodeDownlinkRatio = NICE_SUCC_RATIO;
            nnib.ProxyNodeUplinkRatio = NICE_SUCC_RATIO;

            //�������·��
            DestTEI = CCO_TEI;
            NextHopTEI = GetProxy();
            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pAssociateCfm->LinkType);
                //���µ������·����Ϣ
                UpdateRoutingTableEntry(NextHopTEI, NextHopTEI, 0, ACTIVE_ROUTE, pAssociateCfm->LinkType);
            }
        }
        else
        {
            U16 macLen = 0;

            pMacPublicHeader->ScrTEI = GetTEI();
            if(pAssociateCfm->ProxyTEI == GetTEI())	 //�жϱ��ڵ��Ƿ�Ϊָ���Ĵ���
            {
                pMacPublicHeader->DestTEI = 0xFFF;
                pMacPublicHeader->SendType = e_LOCAL_BROADCAST_FREAM_NOACK;
                pMacPublicHeader->MaxResendTime = 3;
                pMacPublicHeader->Radius = 15;
                pMacPublicHeader->RemainRadius = 15;
                pMacPublicHeader->BroadcastDirection = e_TWO_WAY;
                if(pAssociateCfm->Result !=ASSOC_SUCCESS
                        && pAssociateCfm->Result !=RE_ASSOC_SUCCESS)
                {
                    macLen = len;
                    pMacPublicHeader->ScrTEI = GetTEI();
                    //pMacPublicHeader->DestTEI=SearchNextHopAddrFromRouteTable(pAssociateCfm->ProxyTEI, ACTIVE_ROUTE);

                    if(macuse == TRUE)
                    {
                        __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)] , nnib.MacAddr , MACADRDR_LENGTH);
                        __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)+6] , pAssociateCfm->DstMacAddr , MACADRDR_LENGTH);
                    }

                    if(pAssociateCfm->LinkType == e_HPLC_Link)
                    {
                        MacDataRequst(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                                      e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
                    }
                    else
                    {
                        MacDataRequstRf(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                                      e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
                    }

                    /*
                    LongMacDataRequst(macLen, pMacData_t, pMacData_t->DestTEI, e_UNICAST_FREAM,
                                  e_MMeAssocCfm, pMacIndication_t->LinkId, e_NETMMSG, pMacIndication_t->ResendFlg, Waittime);
                    */
                    return;

                }
                net_printf("--I am ProxyTEI, linktype:%d\n", pAssociateCfm->LinkType);

                if(nnib.NodeType == e_STA_NODETYPE )
                {
                    //���Ʊ��ڵ���Ϣ
                    nnib.NodeType = e_PCO_NODETYPE;
                }

                if(pAssociateCfm->LinkType == e_HPLC_Link)
                {
                    UpdateNbInfo(GetSNID(),pAssociateCfm->DstMacAddr,pAssociateCfm->NodeTEI,pAssociateCfm->NodeDeep,e_STA_NODETYPE, GetTEI(), e_CHILD, buf->rgain,PHASE_A,0,DT_SOF);
                }
                else
                {
                    UpdateRfNbInfo(GetSNID(),pAssociateCfm->DstMacAddr,pAssociateCfm->NodeTEI,pAssociateCfm->NodeDeep,e_STA_NODETYPE, GetTEI(), e_CHILD, buf->rssi,buf->snr,0, nnib.RfCount + 1,DT_SOF);
                }


                //����·�ɣ��������ظ�Ŀ��ڵ�
                //		if(pAssociateCfm_t->routeUpdateSeqNumber != nnib.routeUpdateSeqNumber)
                {
                    DestTEI = pAssociateCfm->NodeTEI;
                    NextHopTEI =  pAssociateCfm->NodeTEI;

                    if(!IS_BROADCAST_TEI(NextHopTEI))
                    {

                        UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pAssociateCfm->LinkType);
                    }
                    nextTEI = NextHopTEI;
                }

                ReacordMmsgData(e_MMeAssocCfm,pAssociateCfm->NodeTEI ,pAssociateCfm->ProxyTEI ,nextTEI ,1);
                if(macuse == TRUE)
                {
                    __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)] , nnib.MacAddr , MACADRDR_LENGTH);
//                    memset(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + 6], 0xff, 6);
                    __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)+6] , pAssociateCfm->DstMacAddr , MACADRDR_LENGTH);
                }
                macLen = len;


//                dump_zc(0, "assoc cfm: ", DEBUG_MDL, pMacInd, len);
                if(pAssociateCfm->LinkType == e_HPLC_Link)
                {
                    MacDataRequst(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                                  e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute, 0);
                }
                else
                {
                    MacDataRequstRf(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_LOCAL_BROADCAST_FREAM_NOACK,
                                  e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
                }


            }
            else//�м��
            {
                if(pAssociateCfm->Result !=ASSOC_SUCCESS
                        && pAssociateCfm->Result !=RE_ASSOC_SUCCESS)
                {
                    macLen = len;
                    pMacPublicHeader->ScrTEI = GetTEI();
                    pMacPublicHeader->DestTEI=SearchNextHopAddrFromRouteTable(pAssociateCfm->ProxyTEI, ACTIVE_ROUTE);
                    if(pMacPublicHeader->DestTEI == BROADCAST_TEI)
                    {
                        return;
                    }

                    if(macuse == TRUE)
                    {
                        __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)] , nnib.MacAddr , MACADRDR_LENGTH);
                        SearchMacInNeighborDisEntrybyTEI_SNID(pMacPublicHeader->DestTEI,	GetSNID(),
                                                              &pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + 6]);
                    }

                    if(NwkRoutingTable[pMacPublicHeader->DestTEI-1].LinkType == e_HPLC_Link)
                    {

                        MacDataRequst(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_UNICAST_FREAM,
                                      e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
                    }
                    else
                    {
                        MacDataRequstRf(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_UNICAST_FREAM,
                                      e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
                    }

                    /*
                    LongMacDataRequst(macLen, pMacData_t, pMacData_t->DestTEI, e_UNICAST_FREAM,
                                  e_MMeAssocCfm, pMacIndication_t->LinkId, e_NETMMSG, pMacIndication_t->ResendFlg, Waittime);
                    */
                    return;

                }

                net_printf("---I am Relay TEI\n");
                DestTEI = pAssociateCfm->NodeTEI;
                NextHopTEI = SearchNextHopAddrFromRouteTable(pAssociateCfm->ProxyTEI, ACTIVE_ROUTE);

                if(!IS_BROADCAST_TEI(NextHopTEI))//��ֹ���㲥
                {
                    UpdateRoutingTableEntry(DestTEI,  NextHopTEI, 0, ACTIVE_ROUTE, NwkRoutingTable[NextHopTEI - 1].LinkType);
                }
                nextTEI = NextHopTEI;
                ReacordMmsgData(e_MMeAssocCfm,pAssociateCfm->NodeTEI ,pAssociateCfm->ProxyTEI ,nextTEI ,2);
                if((GET_TEI_VALID_BIT(nextTEI)) != BROADCAST_TEI)//��ֹ���㲥
                {
                    pMacPublicHeader->DestTEI = nextTEI;
                    if(macuse == TRUE)
                    {
                        __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)] , nnib.MacAddr , MACADRDR_LENGTH);
                        SearchMacInNeighborDisEntrybyTEI_SNID(NextHopTEI,GetSNID(),
                                                              &pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + 6]);
                    }
                    macLen = len;
                    if(NwkRoutingTable[nextTEI - 1].LinkType == e_HPLC_Link)
                    {
                        MacDataRequst(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_UNICAST_FREAM,
                                      e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute, 0);
                    }
                    else
                    {
                        MacDataRequstRf(pMacPublicHeader, macLen, pMacPublicHeader->DestTEI, e_UNICAST_FREAM,
                                      e_MMeAssocCfm, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
                    }
                }
                else
                {
                    ;
                }

            }

        }

        //����·�ɱ�,�����ǵ������ظ��ڵ��ֱ��վ��

        U8 linkType;
        PROXY_INFO_t *pNodeInfo;

        for(i = 0; i < pAssociateCfm->RouteTableInfo.DirectConnectNodeNum; i++)
        {
            pNodeInfo = (PROXY_INFO_t *)&pAssociateCfm->RouteTableInfo.RouteTable[offset];// + pAssociateCfm->RouteTableInfo.RouteTable[offset + 1] * 256;
            DestTEI = pNodeInfo->ParatTEI;
            if(GetTEI() == pAssociateCfm->NodeTEI)          //�����ڵ�����ӽڵ�·����Ϣ
            {
                NextHopTEI = pNodeInfo->ParatTEI;
                linkType   = pNodeInfo->LinkType;
            }
            else                                            //�м�ڵ�ʹ���ڵ�����ӽڵ�·����Ϣ
            {
                NextHopTEI = nextTEI;
                linkType   = NwkRoutingTable[NextHopTEI -1].LinkType;
            }

            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, linkType);
            }
            offset += 2;
        }
        //����·�ɱ�,�����ǵ������ظ��ڵ㼶��վ��
        for(i = 0; i < pAssociateCfm->RouteTableInfo.DirectConnectProxyNum; i++)
        {
            pNodeInfo = (PROXY_INFO_t *)&pAssociateCfm->RouteTableInfo.RouteTable[offset];// + pAssociateCfm->RouteTableInfo.RouteTable[offset + 1] * 256;
            offset += 2;
            staNum = pAssociateCfm->RouteTableInfo.RouteTable[offset] + pAssociateCfm->RouteTableInfo.RouteTable[offset + 1] * 256;
            offset += 2;
            DestTEI = pNodeInfo->ParatTEI;
            //�ȱ��浽�����ֱ��·��
            if(GetTEI() == pAssociateCfm->NodeTEI)
            {
                NextHopTEI  = pNodeInfo->ParatTEI;
                linkType    = pNodeInfo->LinkType;
            }
            else
            {
                NextHopTEI = nextTEI;
                linkType   = NwkRoutingTable[NextHopTEI - 1].LinkType;
            }
            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, linkType);
            }

            //Ȼ�󱣴浽������ӽڵ��·��
            for(j = 0; j < staNum; j++)
            {
                pNodeInfo = (PROXY_INFO_t *)&pAssociateCfm->RouteTableInfo.RouteTable[offset];
                DestTEI = pNodeInfo->ParatTEI;
                offset += 2;
                if(!IS_BROADCAST_TEI(NextHopTEI))
                {
                    UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, linkType);
                }
                offset += 2;
            }

        }

    }
    else
    {
        //net_printf("Data is not to me!!!\n");
        //ת��
    }
#endif
}


/**********************************************************************
Function:			���������
Description:		�ж������Ƿ���Ҫ�����ת��
Input:				Mac��Indictionָ��
Output: 			��
Return: 			��
Other:				�˺���Ϊ�²����ݴ�����յ㣬��Ҫ�ͷ��ڴ�
***********************************************************************/
static void ProcessMMeChangeProxyReq(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8							macuse = FALSE;
//    MACDATA_INDICATION_t		*pMacIndication = NULL;
    U16 DestTEI, NextHopTEI;

    MAC_PUBLIC_HEADER_t 		*pMacPublicHeader;
    PROXYCHANGE_REQ_CMD_t	   *pProxyChgeReq;

    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    macuse = pMacPublicHeader->MacAddrUseFlag;
    if(macuse == TRUE)
    {
        pProxyChgeReq = (PROXYCHANGE_REQ_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pProxyChgeReq = (PROXYCHANGE_REQ_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }

#if defined(STATIC_MASTER)

    //recordtime("MMeChangeProxyReq befor",get_phy_tick_time());
    if(TX_MNG_LINK.nr>TX_MNG_LINK.thr/2) //������Ϣ̫��δ����ȥʱ����Ҫ�����������
    {
        return;
    }

    //CCO��������������ģ��ֻ����ת��
    if(GetNodeType() == e_CCO_NODETYPE)
    {
        U16 						i;
        U8							Depthdiffer = 0;
        U8							UpFlag = 0;

//        pMacIndication	 = (MACDATA_INDICATION_t *)pMacInd;
        pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

        macuse = pMacPublicHeader->MacAddrUseFlag;
        if(macuse == TRUE)
        {
            pProxyChgeReq = (PROXYCHANGE_REQ_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
        }
        else
        {
            pProxyChgeReq = (PROXYCHANGE_REQ_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
        }
        if(LEGAL_CHECK(pProxyChgeReq->StaTEI,CCO_TEI,MAX_WHITELIST_NUM) ==FALSE )
		{
			return;
		}
        if(LEGAL_CHECK(pProxyChgeReq->NewProxyTei[0].ParatTEI,0,MAX_WHITELIST_NUM) ==FALSE )
        {
			return;
		}
        if(LEGAL_CHECK(pProxyChgeReq->OldProxyTei,0,MAX_WHITELIST_NUM) ==FALSE )
        {
			return;
		}
		
		if(LEGAL_CHECK(DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeDepth,0,MaxDeepth+1) == FALSE)
		{
			net_printf("warning:stadeepth:%02x\n", DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeDepth);
			return;
		}
        if(SearchUplinkNodeTei(pProxyChgeReq->StaTEI, pProxyChgeReq->NewProxyTei[0].ParatTEI) == FALSE)
        {
            net_printf("SearchUplinkNodeTei err MMchange StaTEI=%03x,NewP=%03x\n"
                       ,pProxyChgeReq->StaTEI, pProxyChgeReq->NewProxyTei[0].ParatTEI);
            return;
        }
        if(pProxyChgeReq->NewProxyTei[0].ParatTEI != CCO_TEI) //�������µĴ���Ϊ15���ı������
        {
            if(DeviceTEIList[(pProxyChgeReq->NewProxyTei[0].ParatTEI - 2)].NodeDepth  ==0x0f)
            {
                net_printf("tei=0x%04x,newproxy >15\n",pProxyChgeReq->NewProxyTei[0].ParatTEI);
                return;
            }
        }
        if((DeviceTEIList[(pProxyChgeReq->NewProxyTei[0].ParatTEI - 2)].NodeType == e_STA_NODETYPE) && (nnib.PCOCount >= MAXPCONUM) ) //��PCO��������MAXPCONUMʱ,��������PCO
        {
            net_printf("err,proxyreq PCO >= 200\n");
            return;
        }
		
		ChildMaxDeepth = 0;
		ChildCount = 0;
		DT_SearchAllChildStation(pProxyChgeReq->StaTEI);
		if((DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeDepth < DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI-2].NodeDepth+1)
			&& (pProxyChgeReq->NewProxyTei[0].ParatTEI != CCO_TEI))
		{

				if((ChildMaxDeepth-DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeDepth+DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI-2].NodeDepth+1)>MaxDeepth)
				{
					net_printf("warning:deepthovershoot,childmax:%d,sta:%d,newproxy:%d\n",
						ChildMaxDeepth,
						DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeDepth,
						DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI-2].NodeDepth);
					return;
				}
		}
		
        /*
            ���������·���Գƣ�����ڴ������ظ��²�ȥ����������PCO��������
            ����ȼ��ɴ����Ƿ�Ϊ����CCO��¼�Ĵ������һ����˵���ϴδ������ظ��ɹ�����֮��ʧ��
            ʧ�ܵ�����£�CCOӦ���ȼ��ʧ���Ǵα��ΪPCO�Ĵ����Ƿ�Ҫ��ΪPCO
        */
        if(pProxyChgeReq->OldProxyTei != DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI)
        {
            //if(CountOfUseMyParentNodeAsParent(DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI) <= 1&& DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI !=CCO_TEI)
                if(CountproxyNum(DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI) == FALSE && DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI !=CCO_TEI)
            {
                net_printf("diff proxy=%04x,c to sta\n",DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI);
                DeviceTEIList[DeviceTEIList[pProxyChgeReq->StaTEI-2].ParentTEI-2].NodeType = e_STA_NODETYPE;
                nnib.discoverSTACount++;
                if(nnib.PCOCount >0)
                    nnib.PCOCount--;
            }
        }
        else
        {
            /*�жϴ�������STA�ľɴ���ڵ�������PCO��
            ����ǣ��жϾɴ���ڵ��±��Ƿ�ֻ��һ���ӽڵ㣿
            ����ǣ���ɴ���ڵ�����ͽ���ΪSTA*/
            if(pProxyChgeReq->OldProxyTei > CCO_TEI)
            {
                if(DeviceTEIList[pProxyChgeReq->OldProxyTei-2].NodeType  == e_PCO_NODETYPE)
                {
                    //if(CountOfUseMyParentNodeAsParent(pProxyChgeReq->OldProxyTei) == 1)
                    if(CountproxyNum(pProxyChgeReq->OldProxyTei) == FALSE)
                    {
                        net_printf("same proxy=%04x,c to sta--\n",pProxyChgeReq->OldProxyTei);
                        DeviceTEIList[pProxyChgeReq->OldProxyTei-2].NodeType = e_STA_NODETYPE;
                        nnib.discoverSTACount++;
                        if(nnib.PCOCount >0)
                            nnib.PCOCount--;
                    }
                }
            }
        }

        //���������б������������������ĸ��ڵ�,���ӽڵ�ѡ��Ϊ��
        if(pProxyChgeReq->NewProxyTei[0].ParatTEI == 0)
        {
            return;
        }

        // ����´���ĸ��ڵ�Ϊ�������ߣ��򷵻أ������
        /* UplinkNodeCount = 0;
        UplinkNodeTei[UplinkNodeCount++] = pProxyChgeReq_t->NewProxyTei[0];
        SearchUplinkNodeTei(pProxyChgeReq_t->NewProxyTei[0]);
        for(i=0; i<UplinkNodeCount; i++)
        {
             net_printf("uplink node tei %d\n", UplinkNodeTei[i]);
             if(UplinkNodeTei[i] ==  pProxyChgeReq_t->StaTEI)
                 return;
        }*/


        DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].ParentTEI = pProxyChgeReq->NewProxyTei[0].ParatTEI;
        DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].LogicPhase = pProxyChgeReq->PhaseInfo1;
        DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].LinkType = pProxyChgeReq->NewProxyTei[0].LinkType;

		//���������������վ�����վ��
        ChildCount = 0;
        DT_SearchAllChildStation(pProxyChgeReq->StaTEI);
        //		mg_update_type(STEP_3);
        if(pProxyChgeReq->NewProxyTei[0].ParatTEI == CCO_TEI) //cco��TEIΪ1������ʹ������-2�Ĳ���
        {
            Depthdiffer = DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth-1;
            UpFlag = e_UPLEVEL;//����
            DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth = 1;
        }
        //�Ƚϲ㼶��
        else if(DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth == DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI - 2].NodeDepth + 1)
        {
            Depthdiffer = 0;
            UpFlag = e_SAMELEVEL; //�㼶����
        }
        else if(DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth > DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI - 2].NodeDepth + 1)
        {
            Depthdiffer = DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth - DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI - 2].NodeDepth - 1;
            UpFlag = e_UPLEVEL;//����
            DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth -= Depthdiffer;
        }
        else
        {
            Depthdiffer = DeviceTEIList[pProxyChgeReq->NewProxyTei[0].ParatTEI - 2].NodeDepth - DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth + 1;
            UpFlag = e_CHILD;//����
             DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth += Depthdiffer;
        }

        //����·�ɱ�
        DestTEI = pProxyChgeReq->StaTEI;
        if(pProxyChgeReq->NewProxyTei[0].ParatTEI == CCO_TEI)
        {
            NextHopTEI = pProxyChgeReq->StaTEI;
        }
        else
        {
            NextHopTEI = SearchNextHopAddrFromRouteTable(pProxyChgeReq->NewProxyTei[0].ParatTEI, ACTIVE_ROUTE);
        }
        if(!IS_BROADCAST_TEI(NextHopTEI))
        {
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, DeviceTEIList[NextHopTEI - 2].LinkType);
        }

        net_printf("UpFlg:%d, Dpdiff:%d,%03x,lk%d\n", UpFlag, Depthdiffer,NextHopTEI,DeviceTEIList[NextHopTEI - 2].LinkType);
		
        for(i = 0; i < ChildCount; i++)
        {
            //ChildStaIndex[i] +=2;
            DestTEI = ChildStaIndex[i] + 2;
            net_printf(" %03x",DestTEI);
            //NextHopTEI = SearchNextHopAddrFromRouteTable(pProxyChgeReq->StaTEI, ACTIVE_ROUTE);
            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE,  DeviceTEIList[NextHopTEI - 2].LinkType);
            }
            //�����豸�б�
            if(UpFlag == e_UPLEVEL)
            {
                DeviceTEIList[ChildStaIndex[i]].NodeDepth -= Depthdiffer;
            }
            if(UpFlag == e_CHILD)
            {
                if((DeviceTEIList[ChildStaIndex[i]].NodeDepth + Depthdiffer) > MaxDeepth)
                {
                    net_printf("deepthover,DelWLAddr,TEI=%04X\n",DeviceTEIList[ChildStaIndex[i]].NodeTEI);
                    DeviceTEIList[ChildStaIndex[i]].NodeDepth = MaxDeepth;
                }
                else
                {
                    DeviceTEIList[ChildStaIndex[i]].NodeDepth += Depthdiffer;
                }
            }
        }
//		mg_update_type(STEP_4);
        net_printf(" \n");

        ReacordDeepth(e_MMeChangeProxyCnf,pProxyChgeReq->StaTEI
            ,pProxyChgeReq->OldProxyTei
            ,pProxyChgeReq->NewProxyTei[0].ParatTEI
            ,DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeDepth
            ,DeviceTEIList[pProxyChgeReq->StaTEI-2].NodeType
            ,pProxyChgeReq->ChangeCause);
        SendProxyChangeCnf(pProxyChgeReq->StaTEI, pProxyChgeReq->NewProxyTei[0].ParatTEI, pProxyChgeReq->ManageMsgSeqNum, 0, TRUE,ASSOC_SUCCESS); // macuse
        //net_printf("---CCO recv ManageMsgSeqNum is %d----\n", pProxyChgeReq->ManageMsgSeqNum);
        net_printf("recv TEI=%03x,lv:%d,OPT=%02x,NPT=%02x,lk=%d\n"
            ,pProxyChgeReq->StaTEI
            ,DeviceTEIList[(pProxyChgeReq->StaTEI - 2)].NodeDepth
            ,pProxyChgeReq->OldProxyTei
            ,pProxyChgeReq->NewProxyTei[0].ParatTEI
            ,pProxyChgeReq->NewProxyTei[0].LinkType);
//		mg_update_type(STEP_10);
		Topchangetimes++;											//�ܴ���������
		DeviceTEIList[pProxyChgeReq->StaTEI-2].Proxytimes++;		//�ڵ����������
        return;
    }
#else

    U16 					macLen;

    macLen = len;

    if(pMacPublicHeader->DestTEI == GetTEI()) //˵����ѡ��Ϊ����ڵ�
    {
        //�������е�������������·��
        DestTEI = pMacPublicHeader->ScrTEI;
        NextHopTEI = pMacPublicHeader->ScrTEI;
        if(!IS_BROADCAST_TEI(NextHopTEI))
        {
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pProxyChgeReq->NewProxyTei[0].LinkType);
        }
        pMacPublicHeader->DestTEI = CCO_TEI;
        pMacPublicHeader->ScrTEI  = GetTEI();
        if(macuse == TRUE)
        {
            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)], nnib.MacAddr , MACADRDR_LENGTH);
            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + 6], nnib.CCOMacAddr , 6);
        }

    }
    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                      e_MMeChangeProxyReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, macLen, GetProxy(), e_UNICAST_FREAM,
                      e_MMeChangeProxyReq, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*
    LongMacDataRequst(macLen, pMacPublicHeader, nnib.ParentTEI, e_UNICAST_FREAM,
                      e_MMeChangeProxyReq, pMacIndication->LinkId, e_NETMMSG, pMacIndication->ResendFlg, waittime);
    */
    //recordtime("MMeChangeProxyReq after",get_phy_tick_time());
#endif

}
#if defined(STATIC_NODE)
/**********************************************************************
Function:			���������ȷ��
Description:		�ж������Ƿ���Ҫ�����ת��
Input:				Mac��Indictionָ��
Output: 			��
Return: 			��
Other:				�˺���Ϊ�²����ݴ�����յ㣬��Ҫ�ͷ��ڴ�
***********************************************************************/

static void ProcessMMeChangeProxyCnf(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U16 					   i;
    U8						   *pChildEntry, macuse;
    MAC_PUBLIC_HEADER_t 	   *pMacPublicHeader;
//    MACDATA_INDICATION_t	   *pMacIndication;
    PROXYCHANGE_CFM_CMD_t	   *pChangeProxycfm;

    U16 	   DestTEI, NextHopTEI;


//    pMacIndication	 = (MACDATA_INDICATION_t *)pMacInd;
    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    if(pMacPublicHeader->DestTEI != GetTEI())
        return;

    macuse = pMacPublicHeader->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pChangeProxycfm = (PROXYCHANGE_CFM_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
        pChildEntry 	= &pMacInd[sizeof(MAC_LONGDATA_HEADER) + sizeof(PROXYCHANGE_CFM_CMD_t)];
    }
    else
    {
        pChangeProxycfm = (PROXYCHANGE_CFM_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
        pChildEntry 	= &pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + sizeof(PROXYCHANGE_CFM_CMD_t)];
    }

    if(pChangeProxycfm->StaTEI == GetTEI())//����Ǳ����������
    {

        ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
        //ɾ��֮ǰ�Ĵ���ڵ���ھӱ�
        if(pChangeProxycfm->ProxyTei != GetProxy() ) //�������´����֮ǰ��һ�£�ɾ��֮ǰ�Ĵ���
        {
            DelDiscoveryTableByTEI(GetProxy());
        }

		SetProxy(pChangeProxycfm->ProxyTei) ;
        nnib.LinkType = pChangeProxycfm->LinkType;

        SearchMacInNeighborDisEntrybyTEI_SNID(GetProxy(),GetSNID(), nnib.ParentMacAddr);
        nnib.NodeLevel = SearchDepthInNeighborDiscoveryTableEntry(pChangeProxycfm->ProxyTei, pChangeProxycfm->LinkType);
        nnib.NodeLevel += 1;

        DiscvryTable = GetNeighborByTEI(GetProxy(), pChangeProxycfm->LinkType);
        if(DiscvryTable)
        {
            //�����ھӱ�
            DiscvryTable->Relation = e_PROXY;
            DiscvryTable->ResetTimes =  pMacPublicHeader->RestTims;
            DiscvryTable->RemainLifeTime =  nnib.RoutePeriodTime;//nnib.RoutePeriodTime/2;//����ʱ������Ϊ·�ɵ�һ��

        }

        nnib.LinkType = pChangeProxycfm->LinkType;
        net_printf("New P:%03x,Lk:%d,Lv:%d\n", pChangeProxycfm->ProxyTei, pChangeProxycfm->LinkType, nnib.NodeLevel);
         //�������·��
        DestTEI = CCO_TEI;
        NextHopTEI = GetProxy();
        if(!IS_BROADCAST_TEI(NextHopTEI))
        {
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxycfm->LinkType);
            //���µ������·����Ϣ���ھ���·backup������Ҫ����
            UpdateRoutingTableEntry(NextHopTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxycfm->LinkType);
        }
        if(zc_timer_query(g_WaitChageProxyCnf) != TMR_STOPPED)
        {
            timer_stop(g_WaitChageProxyCnf,TMR_NULL);
        }
        ChangeReq_t.RetryTime =0;
        ChangeReq_t.Result =TRUE;

        ReacordMmsgData(e_MMeChangeProxyCnf,pChangeProxycfm->StaTEI ,pChangeProxycfm->ProxyTei ,0 ,0);
    }
    else
    {
        U8		linkid;
        U16 	macLen, nextTEI;

        if(pChangeProxycfm->ProxyTei == GetTEI())//������ڵ�Ϊ�µĴ���ڵ�
        {
            ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

            if(nnib.NodeType == e_STA_NODETYPE)
            {
                nnib.NodeType = e_PCO_NODETYPE;//����ΪPCO
            }

            //����·��,�������ߵ�·��
            DestTEI	 = pChangeProxycfm->StaTEI;
            nextTEI  = pChangeProxycfm->StaTEI;

            NextHopTEI =	nextTEI;
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxycfm->LinkType);

            //���µ������������ߵ������ӽڵ��·��
            for(i = 0; i < pChangeProxycfm->ChildStaCount; i++)
            {
                DestTEI = GetWord(pChildEntry);
                pChildEntry += 2;
//                NextHopTEI = SearchNextHopAddrFromRouteTable(nextTEI, ACTIVE_ROUTE);
                if(!IS_BROADCAST_TEI(NextHopTEI))
                {
                    UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxycfm->LinkType);
                }
            }
            DiscvryTable = GetNeighborByTEI(pChangeProxycfm->StaTEI, pChangeProxycfm->LinkType);
            if(DiscvryTable)
            {
                DiscvryTable->Relation = e_CHILD;
                DiscvryTable->NodeDepth = nnib.NodeLevel + 1;
                DiscvryTable->RemainLifeTime =  nnib.RoutePeriodTime;//nnib.RoutePeriodTime/2;//����ʱ������Ϊ·�ɵ�һ��
            }
        }
        else//���ΪCCO���´���֮��Ľڵ�
        {
            //����·��,�������ߵ�·��

            DestTEI = pChangeProxycfm->StaTEI;
            nextTEI = SearchNextHopAddrFromRouteTable(pChangeProxycfm->ProxyTei, ACTIVE_ROUTE);

            NextHopTEI =	nextTEI;

            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, NwkRoutingTable[NextHopTEI-1].LinkType);
            }
            else
            {
                return;
            }
             //���µ������������ߵ������ӽڵ��·��
            for(i = 0; i < pChangeProxycfm->ChildStaCount; i++)
            {
                DestTEI = GetWord(pChildEntry);
                pChildEntry += 2;
//                NextHopTEI = SearchNextHopAddrFromRouteTable(nextTEI, ACTIVE_ROUTE);
                if(!IS_BROADCAST_TEI(NextHopTEI))
                {
                    UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, NwkRoutingTable[NextHopTEI-1].LinkType);
                }
            }
            ReacordMmsgData(e_MMeChangeProxyCnf,pChangeProxycfm->StaTEI ,pChangeProxycfm->ProxyTei ,nextTEI ,2);
            net_printf("--I am Relay node, NextHop=0x%04x---\n",NextHopTEI);
        }

        linkid = 1;

        //ת��ʱ����ԭʼĿ��ֻ�Ǻ�ԭʼԴ��ַ
        pMacPublicHeader->DestTEI = nextTEI;
        pMacPublicHeader->ScrTEI = GetTEI();
        macLen = len;
        if(macuse == TRUE)
        {
            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)],nnib.MacAddr,6);
            SearchMacInNeighborDisEntrybyTEI_SNID(nextTEI,GetSNID() ,&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)+6]);
        }

        if(NwkRoutingTable[nextTEI-1].LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, nextTEI, e_UNICAST_FREAM,
                                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, nextTEI, e_UNICAST_FREAM,
                                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute);
        }

        /*LongMacDataRequst(macLen, pMacPublicHeader, nextTEI, e_UNICAST_FREAM,
                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, DelayTime);*/
    }

}


static void ProcessMMeChangeProxyBitMapCnf(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U16 						i;
    U8							*pChildEntry, macuse;
    MAC_PUBLIC_HEADER_t 		*pMacPublicHeader;
//    MACDATA_INDICATION_t		*pMacIndication;
    PROXYCHANGE_CFM_CMD_t		*pChangeProxyCfm;

    U16 		DestTEI, NextHopTEI;


//    pMacIndication	 = (MACDATA_INDICATION_t *) pMacInd;
    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    if(pMacPublicHeader->DestTEI !=GetTEI())
        return;

    macuse = pMacPublicHeader->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pChangeProxyCfm = (PROXYCHANGE_CFM_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
        pChildEntry 	= &pMacInd[sizeof(MAC_LONGDATA_HEADER) + sizeof(PROXYCHANGE_CFM_CMD_t)];
    }
    else
    {
        pChangeProxyCfm = (PROXYCHANGE_CFM_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
        pChildEntry 	= &pMacInd[sizeof(MAC_PUBLIC_HEADER_t) + sizeof(PROXYCHANGE_CFM_CMD_t)];
    }

    if(pChangeProxyCfm->StaTEI == GetTEI())//����Ǳ����������
    {
        ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

         //ɾ��֮ǰ�Ĵ���ڵ���ھӱ�
        if(pChangeProxyCfm->ProxyTei != GetProxy() ) //�������´����֮ǰ��һ�£�ɾ��֮ǰ�Ĵ���
        {
            DelDiscoveryTableByTEI(GetProxy());
        }
		SetProxy(pChangeProxyCfm->ProxyTei) ;
        nnib.LinkType = pChangeProxyCfm->LinkType;

        SearchMacInNeighborDisEntrybyTEI_SNID(GetProxy(), GetSNID(),nnib.ParentMacAddr);
        nnib.NodeLevel = SearchDepthInNeighborDiscoveryTableEntry(pChangeProxyCfm->ProxyTei, pChangeProxyCfm->LinkType);
        nnib.NodeLevel += 1;

        DiscvryTable = GetNeighborByTEI(GetProxy(), pChangeProxyCfm->LinkType);
        if(DiscvryTable)
        {
            //�����ھӱ�
            DiscvryTable->Relation = e_PROXY;
            DiscvryTable->ResetTimes =  pMacPublicHeader->RestTims;
            DiscvryTable->RemainLifeTime =  nnib.RoutePeriodTime;//nnib.RoutePeriodTime/2;//����ʱ������Ϊ·�ɵ�һ��

        }

        nnib.LinkType = pChangeProxyCfm->LinkType;

         //�������·��
        DestTEI = CCO_TEI;
        NextHopTEI = GetProxy();
        if(!IS_BROADCAST_TEI(NextHopTEI))
        {
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxyCfm->LinkType);
            //���µ������·����Ϣ���ھ���·backup������Ҫ����
            UpdateRoutingTableEntry(NextHopTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxyCfm->LinkType);
        }
        if(zc_timer_query(g_WaitChageProxyCnf) != TMR_STOPPED)
        {
            timer_stop(g_WaitChageProxyCnf,TMR_NULL);
        }
        ChangeReq_t.RetryTime =0;
        ChangeReq_t.Result =TRUE;
    }
    else
    {
        U8		linkid;
        U16 	macLen, nextTEI;

        if(pChangeProxyCfm->ProxyTei == GetTEI())//������ڵ�Ϊ�µĴ���ڵ�
        {

            ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

            if(nnib.NodeType == e_STA_NODETYPE)
            {
                nnib.NodeType = e_PCO_NODETYPE;//����ΪPCO
            }
            //����·��,�������ߵ�·��
            DestTEI = pChangeProxyCfm->StaTEI;
            nextTEI = pChangeProxyCfm->StaTEI;

            NextHopTEI =  nextTEI;
            UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxyCfm->LinkType);

             //���µ������������ߵ������ӽڵ��·��
            for(i = 0; i < pChangeProxyCfm->ChildStaCount; i++)
            {
                DestTEI = GetWord(pChildEntry);
                pChildEntry += 2;
//                NextHopTEI = SearchNextHopAddrFromRouteTable(nextTEI, ACTIVE_ROUTE);
                if(!IS_BROADCAST_TEI(NextHopTEI))
                {
                    UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, pChangeProxyCfm->LinkType);
                }
            }

            DiscvryTable = GetNeighborByTEI(pChangeProxyCfm->StaTEI, pChangeProxyCfm->LinkType);
            if(DiscvryTable)
            {
                DiscvryTable->Relation = e_CHILD;
                DiscvryTable->NodeDepth = nnib.NodeLevel + 1;
                DiscvryTable->RemainLifeTime =  nnib.RoutePeriodTime;//nnib.RoutePeriodTime/2;//����ʱ������Ϊ·�ɵ�һ��
            }
        }
        else//���ΪCCO���´���֮��Ľڵ�
        {
            //����·��,�������ߵ�·��

            DestTEI = pChangeProxyCfm->StaTEI;
            nextTEI = SearchNextHopAddrFromRouteTable(pChangeProxyCfm->ProxyTei, ACTIVE_ROUTE);
            NextHopTEI =  nextTEI;

            if(!IS_BROADCAST_TEI(NextHopTEI))
            {
                UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, NwkRoutingTable[NextHopTEI-1].LinkType);
            }
            else
            {
                return;
            }

            //���µ������������ߵ������ӽڵ��·��
            for(i = 0; i < pChangeProxyCfm->ChildStaCount; i++)
            {
                DestTEI = GetWord(pChildEntry);
                pChildEntry += 2;
//                NextHopTEI = SearchNextHopAddrFromRouteTable(nextTEI, ACTIVE_ROUTE);

                if(!IS_BROADCAST_TEI(NextHopTEI))
                {
                    UpdateRoutingTableEntry(DestTEI, NextHopTEI, 0, ACTIVE_ROUTE, NwkRoutingTable[NextHopTEI-1].LinkType);
                }
            }
            ReacordMmsgData(e_MMeChangeProxyCnf,pChangeProxyCfm->StaTEI ,pChangeProxyCfm->ProxyTei ,nextTEI ,2);
            net_printf("--I am Relay node, NextHop=0x%04x---\n",NextHopTEI);
        }

        linkid = 1;
        //ת��ʱ����ԭʼĿ��ֻ�Ǻ�ԭʼԴ��ַ
        pMacPublicHeader->DestTEI = nextTEI;
        pMacPublicHeader->ScrTEI = GetTEI();
        macLen = len;

        if(macuse == TRUE)
        {
            __memcpy(&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)],nnib.MacAddr,6);
            SearchMacInNeighborDisEntrybyTEI_SNID(nextTEI,GetSNID(), &pMacInd[sizeof(MAC_PUBLIC_HEADER_t)+6]);
        }

        if(NwkRoutingTable[nextTEI - 1].LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, macLen, nextTEI, e_UNICAST_FREAM,
                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, macLen, nextTEI, e_UNICAST_FREAM,
                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, 0, e_MainRoute);
        }

        /*LongMacDataRequst(macLen, pMacPublicHeader, nextTEI, e_UNICAST_FREAM,
                          e_MMeChangeProxyCnf, linkid, e_NETMMSG, 0, DelayTime);*/
    }

}
#endif //#if defined(STATIC_NODE)

static __SLOWTEXT void ProcessMMeHeartBeatCheck(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8						   macuse;
    MAC_PUBLIC_HEADER_t 	  *pMacPublicHeader;
//    MACDATA_INDICATION_t	  *pMacIndication;
    HEART_BEAT_CHECK_CMD_t	  *pHeartBeatCheck;

//    pMacIndication	 = (MACDATA_INDICATION_t *)pMacInd;
    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    macuse = pMacPublicHeader->MacAddrUseFlag;

#if defined(STATIC_NODE)   //��ģ��ת��
    U8						negiberNum;
    U16                     bmpNumber;
    U16 					macLen, msduLen;
    U32 					Crc32Datatest;
    MAC_PUBLIC_HEADER_t 	*pRelayMacHeader;

    U8						*pMsduData;

    pRelayMacHeader = (MAC_PUBLIC_HEADER_t*)zc_malloc_mem(sizeof(MAC_LONGDATA_HEADER)+sizeof(HEART_BEAT_CHECK_CMD_t)+256 + CRCLENGTH,
                                                                                      "HeartBeatCheckRelay",MEM_TIME_OUT);

    if(macuse == TRUE)
    {
        pHeartBeatCheck = (HEART_BEAT_CHECK_CMD_t *)(((MAC_LONGDATA_HEADER*)pRelayMacHeader)->Msdu); //<<<
        //pHeartBeatCheck = (HEART_BEAT_CHECK_CMD_t *)(pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
        __memcpy(pRelayMacHeader, pMacInd, len);
    }
    else
    {
        pHeartBeatCheck = (HEART_BEAT_CHECK_CMD_t *)pRelayMacHeader->Msdu;
        //pHeartBeatCheck = (HEART_BEAT_CHECK_CMD_t *)(pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));
        __memcpy(pRelayMacHeader, pMacInd, len);
    }

//    bmpNumber= GetNeighborBmpByteNum();

    GetNeighborBmp(pHeartBeatCheck->TEIBmp, &bmpNumber);

    if(bmpNumber > pHeartBeatCheck->BmpNumber)
    {
        pHeartBeatCheck->BmpNumber = bmpNumber;
    }
//    GetNeighborBmp(pHeartBeatCheck->TEIBmp, NULL);
    negiberNum = GetNeighborCount();
    if(negiberNum > pHeartBeatCheck->MaxiNum)
    {
        pHeartBeatCheck->MaxiNum = negiberNum;
    }
    msduLen = sizeof(HEART_BEAT_CHECK_CMD_t) + pHeartBeatCheck->BmpNumber;

    pRelayMacHeader->MsduLength_l = msduLen;
    pRelayMacHeader->MsduLength_h = msduLen>>8;
    pRelayMacHeader->ScrTEI = GetTEI();
    pRelayMacHeader->DestTEI = GetProxy();

    if(msduLen ==0 || msduLen>512)
    {
        //sys_panic("<ProcessMMeHeartBeatCheck MsduLen err:%d \n> %s: %d\n",msduLen, __func__, __LINE__);
        debug_printf(&dty,DEBUG_GE,"ProcessMMeHeartBeatCheck MsduLen err: :%d",msduLen);
    }

    pMsduData = pRelayMacHeader->Msdu;

    if(macuse == TRUE)
    {
        macLen =  msduLen + sizeof(MAC_LONGDATA_HEADER)+CRCLENGTH;

        __memcpy(pMsduData, nnib.MacAddr,6);
        pMsduData +=6;
        __memcpy(pMsduData, nnib.ParentMacAddr,6);
        pMsduData += 6;
    }
    else
    {
        macLen =  msduLen + sizeof(MAC_PUBLIC_HEADER_t) + CRCLENGTH;
    }

    crc_digest(pMsduData, msduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
    __memcpy(&pMsduData[msduLen], (U8 *)&Crc32Datatest, CRCLENGTH);
    net_printf("%%%%%%%%%%%relay SendHeartBeatCheck\n");
    dump_buf(pRelayMacHeader->Msdu, 12);
    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pRelayMacHeader, macLen, GetProxy(), pMacPublicHeader->SendType,
                      e_MMeHeartBeatCheck, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pRelayMacHeader, macLen, GetProxy(), pMacPublicHeader->SendType,
                      e_MMeHeartBeatCheck, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*LongMacDataRequst(macLen, pRelayMacHeader, nnib.ParentTEI, pMacPublicHeader->SendType,
                      e_MMeHeartBeatCheck, pMacIndication->LinkId, e_NETMMSG, 0, 0);*/


    zc_free_mem(pRelayMacHeader);

    //PCO��ת��������ˢ�¼���
    UpDataHeartBeat();
#endif

#if defined(STATIC_MASTER) //ͳ������TEI������TEI����ʱ�䡣
    U16 num=0;
    U16 i,j,Bitoffset;

    if(macuse == TRUE)
    {
       pHeartBeatCheck	= (HEART_BEAT_CHECK_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];

    }
    else
    {
       pHeartBeatCheck	= (HEART_BEAT_CHECK_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];

    }

    net_printf("HeartBeathe SrcTEI:=%04x\n",pHeartBeatCheck->SrcTEI);
    for(i=0;i<pHeartBeatCheck->BmpNumber;i++)
    {
        for(j=0;j<8;j++)
        {
            if(pHeartBeatCheck->TEIBmp[i] & (1<<j))
            {
                Bitoffset = i*8+j;
                if(Bitoffset > CCO_TEI)
                {
                    //app_printf("-0x%04x",DeviceTEIList[Bitoffset-2].NodeTEI);
                    if(Bitoffset >= (MAX_WHITELIST_NUM+1))
                    {
                        break;
                    }
                    if(DeviceTEIList[Bitoffset-2].NodeState != e_NODE_OUT_NET)
                    {
                        //net_printf("%04x,",DeviceTEIList[Bitoffset-2].NodeTEI);
                        num++;

                        if(num>32)
                        {
                            num=0;
                          //  net_printf("\n");
                        }

                        DeviceTEIList[Bitoffset-2].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
                        if(DeviceTEIList[Bitoffset-2].NodeState == e_NODE_OFF_LINE)
                            DeviceTEIList[Bitoffset-2].NodeState = e_NODE_ON_LINE;
                    }
                }
            }
        }
    }
   // net_printf("\n");

#endif


}


//�ӷ����б��л�ȡPCO���߱��ô��������Լ��Ĵ���
static U8 GetPcoRevmeCount(U16 BmpNumber ,U8* pBmp , U8* pList)
{
    U8	j,recvMecount;
    U16 i,Byteoffset,Bitoffset;
    U16  NumOffset=0;

    Byteoffset = GetTEI() / 8;
    Bitoffset  = GetTEI() % 8;

    for(i = 0; i < BmpNumber; i++) //�ӽ�λͼ����
    {
        for(j = 0; j < 8; j++) //ÿһ���ֽ�
        {
            if(pBmp[i] & (1 << j))
            {
                NumOffset++;
                if ( (i == Byteoffset) && (j == Bitoffset) )
                {
                    {
                        recvMecount = pList[NumOffset - 1];
                        return recvMecount;
                    }
                }
            }
        }
    }
    return 0;
}



/**********************************************************************
Function:			�������б�
Description:
Input:				Mac��Indictionָ��
Output: 			��
Return: 			��
Other:				�˺���Ϊ�²����ݴ�����յ㣬��Ҫ�ͷ��ڴ�
***********************************************************************/
/*����֮ǰ��˼·*/
//U8 TestInetOKflag = FALSE;

U8 CheckTeiInBitMap(U8 *map, U16 size, U16 tei)
{
    if(tei == 0)
        return FALSE;

    U16 byteIndex = tei/8;
    U8  bit = tei % 8;

    if (byteIndex + 1 > size)
        return FALSE;

    if(map[byteIndex] & 1 << bit)
        return TRUE;

    return FALSE;

}

static void ProcessMMeDiscoverNodeList(mbuf_t *buf, U8 *pMacInd, U16 len)
{

    U8									*pBmp, *pList;
    U8									 macuse;
    MAC_PUBLIC_HEADER_t 				*pMacPublicHeader;
    DISCOVER_NODELIST_CMD_t 			*pNodeList;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t 	 *pEntry;

//    pMacIndication	 = (MACDATA_INDICATION_t *)pMacInd;
    pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;


    macuse = pMacPublicHeader->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pNodeList	= (DISCOVER_NODELIST_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pNodeList	= (DISCOVER_NODELIST_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }
    if(nnib.NodeState == e_NODE_OUT_NET)
    {
        UpdateNbInfo(buf->snid, pNodeList->MacAddr,pNodeList->NodeTEI,pNodeList->NodeDeepth,pNodeList->NodeType, pNodeList->ProxyTEI, e_NEIGHBER,buf->rgain,pNodeList->PhaseInfo1,0,DT_SOF);
        return;
    }



    pBmp  = (U8*)pNodeList + sizeof(DISCOVER_NODELIST_CMD_t) + pNodeList->UpLinkRouteNum * 2;
    pList = pBmp + pNodeList->BmpNumber;

#if	defined(STATIC_MASTER)//��ģ�����������б��ֱ�Ӹ����豸�б����豸��ͨ�ųɹ���

    if(DeviceTEIList[pNodeList->NodeTEI-2].LinkType == e_HPLC_Link)
    {
        DeviceTEIList[pNodeList->NodeTEI-2].UplinkSuccRatio =  pNodeList->ProxyNodeUplinkRatio;
        DeviceTEIList[pNodeList->NodeTEI-2].DownlinkSuccRatio = pNodeList->ProxyNodeDownlinkRatio;
    }
    //DeviceTEIList[pNodeList->NodeTEI-2].LogicPhase = pNodeList->PhaseInfo1;             //���´ӽڵ����λ��Ϣ
    //net_printf("------Recv NB DiscoverNodeList TEI= 0x%04x----------\n",pNodeList->NodeTEI);

#elif defined(STATIC_NODE)

    //ÿ��·������ͬ��һ�θ��ڵ��·��ʣ��ʱ��

    if(GetProxy() == pNodeList->NodeTEI)
    {
        //TestInetOKflag = TRUE; //��ɾ��
		//if����ԭ����Ҫ����������
        if(CheckTeiInBitMap(pBmp, pNodeList->BmpNumber, GetTEI()))  //����Ǹ��ڵ㣬��Ҫ���Ҹ��ڵ��λͼ���Ƿ����Լ�
        {
            nnib.RecvDisListTime = nnib.RoutePeriodTime*4;
        }
        net_printf("Rcv Pxy Disc ,TEI= 0x%04x, Time=%d\n",GetProxy(),nnib.RecvDisListTime);
        nnib.NodeLevel = pNodeList->NodeDeepth+1; //���Լ��ĸ��ڵ�ͬ���㼶
    }
    else
    {
        //net_printf("------Recv NB DiscoverNodeList TEI= 0x%04x----------\n",pNodeList->NodeTEI);
    }

#endif

    //�����ھӱ�
    pEntry = GetNeighborByTEI(pNodeList->NodeTEI, e_HPLC_Link);
    if(pEntry)
    {
        pEntry->RemainLifeTime =  nnib.RoutePeriodTime;//nnib.RoutePeriodTime/2;//����ʱ������Ϊ·�ɵ�һ��
        pEntry->NodeType = pNodeList->NodeType;
        pEntry->NodeDepth = pNodeList->NodeDeepth;
        pEntry->HplcInfo.ThisPeriod_REV++;

        pEntry->HplcInfo.PCO_SED = pNodeList->SendDiscoListCount;

        pEntry->ResetTimes =  pMacPublicHeader->RestTims;
        pEntry->HplcInfo.PCO_REV = GetPcoRevmeCount(pNodeList->BmpNumber,pBmp,pList);

        UpDataNeighborRelation(pEntry, pNodeList->ProxyTEI, pNodeList->NodeDeepth,pNodeList->ProxyNodeUplinkRatio,pNodeList->ProxyNodeDownlinkRatio);

//        net_printf("ThisPeriod_REV : %d\n", pEntry->HplcInfo.ThisPeriod_REV);
    }
    else
    {
        // net_printf("ProcessDiscover Node : %03x is not in neighbor\n", pNodeList->NodeTEI);
    }

    RepairNextHopAddr(pNodeList->NodeTEI, e_HPLC_Link);

    return;
}


static void ProcessMMeSuccessRateReport(mbuf_t *buf, U8 *pMacInd, U16 len)
{
//    MACDATA_INDICATION_t	  *pMacIndication;

    MAC_PUBLIC_HEADER_t *pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

#if defined(STATIC_MASTER)
    U8							macuse;
    U16 						i, TEIseq;
    SUCCESSRATE_REPORT_CMD_t   *pSuccessRateList;
    SUCCESSRATE_INFO_t		   *pSuccesInfo;
#endif


//    pMacIndication	= (MACDATA_INDICATION_t *)pMacInd;

#if defined(STATIC_MASTER)	//���ϱ��ĳɹ��ʱ��浽�豸�б���


    macuse = pMacPublicHeader->MacAddrUseFlag;
    if(macuse == TRUE)
    {
        pSuccessRateList	= (SUCCESSRATE_REPORT_CMD_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
    }
    else
    {
        pSuccessRateList	= (SUCCESSRATE_REPORT_CMD_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
    }
    pSuccesInfo = (SUCCESSRATE_INFO_t *)&pSuccessRateList->ChildInfo[0];

    for(i = 0; i < pSuccessRateList->NodeCount ; i++)
    {
        TEIseq = pSuccesInfo->ChildTEI-2;
        if((TEIseq>=0) && (TEIseq<MAX_WHITELIST_NUM) && DeviceTEIList[TEIseq].ParentTEI == pSuccessRateList->NodeTEI)
        {
            DeviceTEIList[TEIseq].UplinkSuccRatio = pSuccesInfo->UpCommRate;
            DeviceTEIList[TEIseq].DownlinkSuccRatio = pSuccesInfo->DownCommRate;
            // pSuccesInfo = (SUCCESSRATE_INFO_t *)((U8*)pSuccesInfo+sizeof(SUCCESSRATE_INFO_t));
        }
        // else if(TEIseq < MAX_WHITELIST_NUM)
        // {
        //     net_printf("err tatio:%03x, %03x, %03x\n"
        //                         , pSuccessRateList->NodeTEI
        //                         , pSuccesInfo->ChildTEI
        //                         , DeviceTEIList[TEIseq].ParentTEI);
        // }
        pSuccesInfo = (SUCCESSRATE_INFO_t *)((U8*)pSuccesInfo+sizeof(SUCCESSRATE_INFO_t));
    }

#endif

#if defined(STATIC_NODE) //ת���ɹ����ϱ�

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                                      e_MMeSuccessRateReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                                          e_MMeSuccessRateReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*LongMacDataRequst(pMacIndication->MpduLength, pMacPublicHeader, nnib.ParentTEI, pMacPublicHeader->SendType,
                      e_MMeSuccessRateReport, pMacIndication->LinkId, e_NETMMSG, 0, 0);*/
#endif

}

static void ProcessMMeNetworkConflictReport(mbuf_t *buf, U8 *pMacInd, U16 len)
{
//    MACDATA_INDICATION_t		*pMacIndication;


    MAC_PUBLIC_HEADER_t *pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

#if defined(STATIC_MASTER)
    U8							macuse;
    NETWORK_CONFLIC_REPORT_t	*pNetConflicReport;
    MAC_LONGDATA_HEADER 		*pNetConflicLongmac;

    macuse = pMacPublicHeader->MacAddrUseFlag;
    if(macuse == TRUE)
    {
        pNetConflicReport	= (NETWORK_CONFLIC_REPORT_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
        pNetConflicLongmac	= (MAC_LONGDATA_HEADER*)pMacInd;
        if(memcmp(nnib.CCOMacAddr, pNetConflicLongmac->DestMacAddress, 6) != 0 )
        {
            return;
        }
    }
    else
    {
        pNetConflicReport = (NETWORK_CONFLIC_REPORT_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];

    }
    //�����µ��ھ�����
    //�жϳ�ͻ��CCO��ַ�Ƿ񲻱��ڵ��
    if(comcmpMac(nnib.CCOMacAddr, pNetConflicReport->CCOAddr) == 0) //�ȱ��ڵ�MAC��ַ��,��������
    {
        //����NID
        //
        net_printf("-----------------------ProcessMMeNetworkConflictReport and Clean-----------------------------\n");
        net_printf("----------------nnib.CCOMacAddr%02x %02x %02x %02x %02x%02x-------pNetConflicReport->CCOAddr%02x %02x %02x %02x %02x %02xSRCTEI %04x--\n",
                    nnib.CCOMacAddr[0],
                    nnib.CCOMacAddr[1],
                    nnib.CCOMacAddr[2],
                    nnib.CCOMacAddr[3],
                    nnib.CCOMacAddr[4],
                    nnib.CCOMacAddr[5],
                    pNetConflicReport->CCOAddr[0],
                    pNetConflicReport->CCOAddr[1],
                    pNetConflicReport->CCOAddr[2],
                    pNetConflicReport->CCOAddr[3],
                    pNetConflicReport->CCOAddr[4],
                    pNetConflicReport->CCOAddr[5],
                    pMacPublicHeader->ScrTEI);
        ClearNNIB();

        SetPib(rand() & 0x00FFFFFF,CCO_TEI);


    }
    else
    {
        if(timer_query(&g_ChangeSNIDTimer) == TMR_STOPPED)
        {
            timer_start(&g_ChangeSNIDTimer);
            net_printf("Start g_ChangeSNIDTimer\n");
        }
    }


#elif defined(STATIC_NODE) //ת����ͻ���

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                      e_MMeNetworkConflictReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                      e_MMeNetworkConflictReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
    }

    /*LongMacDataRequst(pMacIndication->MpduLength, pMacPublicHeader, nnib.ParentTEI, pMacPublicHeader->SendType,
                      e_MMeNetworkConflictReport, pMacIndication->LinkId, e_NETMMSG, 0, 0);*/
#endif

    return;
}

U8 GlRFChangeOCtimes = 0;

/**
 * @brief ProcessMMeRFChannelConflictReport
 * @param buf
 * @param pMacInd
 * @param len
 */
static void ProcessMMeRFChannelConflictReport(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    //    MACDATA_INDICATION_t		*pMacIndication;


        MAC_PUBLIC_HEADER_t *pMacPublicHeader = (MAC_PUBLIC_HEADER_t *)pMacInd;

    #if defined(STATIC_MASTER)
        U8							macuse;
        RFCHANNEL_CONFLIC_REPORT_t	*pNetConflicReport;
        MAC_LONGDATA_HEADER 		*pNetConflicLongmac;

        U8 Conflict_flag = FALSE;

        macuse = pMacPublicHeader->MacAddrUseFlag;
        if(macuse == TRUE)
        {
            pNetConflicReport	= (RFCHANNEL_CONFLIC_REPORT_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
            pNetConflicLongmac	= (MAC_LONGDATA_HEADER*)pMacInd;
            if(memcmp(nnib.CCOMacAddr, pNetConflicLongmac->DestMacAddress, 6) != 0 )
            {
                return;
            }
        }
        else
        {
            pNetConflicReport = (RFCHANNEL_CONFLIC_REPORT_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];

        }
        //�����µ��ھ�����
        uint8_t i;
	    uint8_t findNbIndex = 0xff;
		uint8_t CurrChnl;

        CurrChnl = getHplcRfChannel();//WPHY_CHANNEL;//wphy_get_channel()+1;

        //��¼�ŵ�ռ�������
        {
            U8 i;
            setRfChlHaveNetFlag(getHplcOptin(), getHplcRfChannel());

            U8 *PchlInfo = pNetConflicReport->NBnetList;
            U8 *pOptInfo = pNetConflicReport->NBnetList + pNetConflicReport->NBNetNum;
            // net_printf("NBNetNum:%d\n",pNetConflicReport->NBNetNum);
            for(i = 0; i < pNetConflicReport->NBNetNum; i++)
            {
                setRfChlHaveNetFlag(pOptInfo[i], PchlInfo[i]);
                if(pOptInfo[i] == getHplcOptin() && PchlInfo[i] == getHplcRfChannel())
                {
                    Conflict_flag = TRUE;
                }
            }
        }

        for(i = 0; i < MaxChannelNum; i++)
	    {
            if(0==memcmp(RfNbrNet[i].CCOaddr, pNetConflicReport->CCOAddr, 6))
	        {
                findNbIndex = i;
                break;
	        }
            else if(RfNbrNet[i].SendChannl==0xff && findNbIndex==0xff)
			{
				findNbIndex = i;
			}
	    }
        if(findNbIndex!=0xff)
		{
            RfNbrNet[findNbIndex].SendChannl = CurrChnl;
            RfNbrNet[findNbIndex].SendOption = getHplcOptin();
            __memcpy(RfNbrNet[findNbIndex].CCOaddr, pNetConflicReport->CCOAddr, 6);
			if(pNetConflicReport->NBNetNum)
            {
                //��¼�ϱ���ͻģ����Χ�ھ������ŵ����
                __memcpy(RfNbrNet[findNbIndex].NbrChannl, pNetConflicReport->NBnetList, MIN( pNetConflicReport->NBNetNum, MaxChannelNum));
                __memcpy(RfNbrNet[findNbIndex].NbrOption, pNetConflicReport->NBnetList + pNetConflicReport->NBNetNum, MIN( pNetConflicReport->NBNetNum, MaxChannelNum));
            }
            RfNbrNet[findNbIndex].LifeTime = SNID_LIFE_TIME;
	        nnib.NbSNIDnum++;
		}

        if(!GetRfConsultEn())
        {
            return;
        }

        //TODO ��������ŵ�
        if(nnib.RfChannelChangeState == 0 && Conflict_flag == TRUE &&  GlRFChangeOCtimes == 0)
        {
            if(comcmpMac(nnib.CCOMacAddr, pNetConflicReport->CCOAddr) == 0) //�ȱ��ڵ�MAC��ַ��,��������
            {       
                //����������MAC��ַ���򱣳����е������ŵ����䣬�����ͻ״̬����30���ӣ��� ���������������ŵ���
                net_printf("RF Conflict CCO:%02x%02x%02x%02x%02x%02x, chnl:%d\n",pNetConflicReport->CCOAddr[0],pNetConflicReport->CCOAddr[1],
                            pNetConflicReport->CCOAddr[2],pNetConflicReport->CCOAddr[3],pNetConflicReport->CCOAddr[4],pNetConflicReport->CCOAddr[5],CurrChnl);
                net_printf("CCOMac:%02x%02x%02x%02x%02x%02x\n"
                            , nnib.CCOMacAddr[0], nnib.CCOMacAddr[1], nnib.CCOMacAddr[2], nnib.CCOMacAddr[3], nnib.CCOMacAddr[4], nnib.CCOMacAddr[5]);

                RF_OPTIN_CH_t Rfparam;
                if(get_valid_channel(&Rfparam))
                {
                    nnib.RfChannelChangeState = 1;
                    nnib.RfChgChannelRemainTime = 300;
                    DefSetInfo.RfChannelInfo.option = Rfparam.option;
                    DstCoordRfChannel = DefSetInfo.RfChannelInfo.channel = Rfparam.channel;
                    net_printf("channel chnage to be <%d,%d>\n",DefSetInfo.RfChannelInfo.option, DefSetInfo.RfChannelInfo.channel);
                }
            }
        }

    #elif defined(STATIC_NODE) //ת����ͻ���

        if(nnib.LinkType == e_HPLC_Link)
        {
            MacDataRequst(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                          e_MMeRfChannelConfilictReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
        else
        {
            MacDataRequstRf(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                          e_MMeRfChannelConfilictReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
        }

    #endif

        return;
}

#if defined(STATIC_MASTER)
//#define    SELF_DIFF_RANGE        300

static U32 CheckTemp(U32 temp)
{
    U32 reTemp = 0;

    reTemp = (abs(temp) / 25) % 20000;

    if((reTemp < SELF_DIFF_RANGE || reTemp > (20000-SELF_DIFF_RANGE)) || (reTemp > (10000-SELF_DIFF_RANGE) && reTemp < (10000+SELF_DIFF_RANGE)))
    {
        return TRUE;
    }
    app_printf("temp = %08x ,reTemp = %d ",temp,reTemp);
    app_printf("CheckTemp False!\n");
    return FALSE;
}
#endif

void ProcessMMeZeroCrossNTBReport(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    MAC_PUBLIC_HEADER_t        *pMacPublicHeader = (MAC_PUBLIC_HEADER_t*)pMacInd;

#if defined(STATIC_MASTER)
	U8         macuse,phase;
    U16        i;                
	U16        NowOffset  = 0;
    U16        calcuCount1, calcuCount2, calcuCount3;
    U16        diffsize = 0;
	U32        diff, MiniDiff=0xffffffff;


	ZERO_CROSSNTB_REPORT_t	   *pZeroReport;

	U32 						StaNTB[ZERO_COLLOCT_NUM*3+1];
	U32 						temp;

	macuse = pMacPublicHeader->MacAddrUseFlag;
	if(macuse == TRUE)
	{
		pZeroReport = (ZERO_CROSSNTB_REPORT_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
	}
	else
	{
		pZeroReport = (ZERO_CROSSNTB_REPORT_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
	}

    app_printf("mpdu data len = %d:\n", len);
    dump_buf(pMacInd, len);


	if(pZeroReport->NodeTEI> MAX_WHITELIST_NUM || pZeroReport->NodeTEI<=CCO_TEI )
	{
		return;
	}

	if(Zero_Result.ColloctTEI != pZeroReport->NodeTEI) //�ռ��������ݲ�����CCO��ǰҪ��TEI�����ݣ�����
	{
		return;
	}

	StaNTB[0] = pZeroReport->FirstNTB<<8;
	calcuCount1 = pZeroReport->Phase1Num;
	calcuCount2 = pZeroReport->Phase2Num;
	calcuCount3 = pZeroReport->Phase3Num;
    
	if(pZeroReport->ColloctNumber > ZERO_COLLOCT_NUM*3) //�ϱ���������CCOҪ�����ݣ�Ϊ�쳣����
	{
		return;
	}

	//StaNTB,0�ֽڴ�Ż�׼NTB�������ǵ�һ����ֵ���ڶ�����ֵ����������ֵ
	app_printf("NodeTEI=%04x  ",pZeroReport->NodeTEI);
	app_printf("calcuCount1=%d, diffsize=%d ->", calcuCount1, diffsize);
	dump_buf((U8 *)pZeroReport,sizeof(ZERO_CROSSNTB_REPORT_t));
	dump_buf(pZeroReport->NTBdiff, (calcuCount1+calcuCount2+calcuCount3)*12/8);

	if(calcuCount1 > 0)
	{
		//dump_buf(pZeroReport_t->NTBdiff+diffsize,15);
		for(i = 0; i < calcuCount1; i++)
		{
			temp= GetDataFormZeroDifflist(i, pZeroReport->NTBdiff);  // ��ȡ��ֵ
			if((i > 0) && (i < (calcuCount1-1)))
			{
				if(FALSE == CheckTemp(temp))
				{
					return;
				}
			}
			StaNTB[i+1] = temp + StaNTB[i];
            net_printf("temp=%08x\n",temp);
		}
	}

    net_printf("calcuCount2=%d,diffsize=%d\n",calcuCount2,diffsize);
	if(calcuCount2 > 0)
	{
		//dump_buf(pZeroReport_t->NTBdiff+diffsize,15);
		for(i = calcuCount1; i < (calcuCount1+calcuCount2); i++)
		{
			temp = GetDataFormZeroDifflist(i, pZeroReport->NTBdiff);
			if(i == calcuCount1)
			{
				StaNTB[i+1] = temp + StaNTB[0]; // �ڶ���ĵ�һ��ֵ���ֵ���
			}
			else
			{
				if((i < (calcuCount1+calcuCount2-1)) && FALSE == CheckTemp(temp))
				{
					return;
				}
				StaNTB[i+1] = temp + StaNTB[i];
			}

			app_printf("temp=%08x\n",temp);
		}
	}

	app_printf("calcuCount3=%d,diffsize=%d\n",calcuCount3,diffsize);
    
	if(calcuCount3>0)
	{
		//dump_buf(pZeroReport_t->NTBdiff+diffsize,15);
		for(i = calcuCount1+calcuCount2; i < (calcuCount1+calcuCount2+calcuCount3); i++)
		{
			temp= GetDataFormZeroDifflist(i, pZeroReport->NTBdiff);
			if(i==calcuCount1+calcuCount2)
			{
				StaNTB[i+1] = temp + StaNTB[0];  // ������ĵ�һ��ֵ���ֵ���
			}
			else
			{
				if((i < (calcuCount1+calcuCount2+calcuCount3-1)) && FALSE == CheckTemp(temp))
				{
					return;
				}

				StaNTB[i+1] = temp + StaNTB[i];
			}

			app_printf("temp=%08x\n",temp);
		}
	}

	//�ҵ���STA��ӽ��Ĺ����
	for(i=0;i<MAXNTBCOUNT;i++)
	{
		diff = abs(StaNTB[0] - ZeroData[e_A_PHASE-1].NTB[i]);
		if(MiniDiff > diff)
		{
			MiniDiff = diff;
			NowOffset = i;
		}
	}
	if(MiniDiff  > 500000) //��С����20MS��������Ч
	{
		net_printf("---MiniDiff=%d----return----\n",MiniDiff);
		return;
	}

	Zero_Result.ColloctResult[0] = ZeroData[e_A_PHASE-1].NTB[NowOffset]; // ȡ���뱨���л�ֵ�����С��A������NTBֵ 
	
	CalculatePHASE(pZeroReport->NodeTEI,Zero_Result.ColloctResult[0] ,StaNTB,calcuCount1,calcuCount2,calcuCount3,&phase);
    
	DeviceTEIList[pZeroReport->NodeTEI-2].Phase = phase;

	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(PHASE_POSITION_CFM_t), "CUSMI", MEM_TIME_OUT);
	PHASE_POSITION_CFM_t *pPhasePositionCfm = (PHASE_POSITION_CFM_t *)work->buf;
	__memcpy(pPhasePositionCfm->addr, DeviceTEIList[pZeroReport->NodeTEI-2].MacAddr, 6);
    pPhasePositionCfm->phase = DeviceTEIList[pZeroReport->NodeTEI-2].Phase;
    pPhasePositionCfm->LNerr = DeviceTEIList[pZeroReport->NodeTEI-2].LNerr;
        
	work->work = pPhasePositionCfmProc;
	work->msgtype = PHASECMF;
	post_app_work(work);


#endif
#if defined(STATIC_NODE)

    if(nnib.LinkType == e_HPLC_Link)
    {
        MacDataRequst(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                      e_MMeZeroCrossNTBReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    else
    {
        MacDataRequstRf(pMacPublicHeader, len, GetProxy(), pMacPublicHeader->SendType,
                        e_MMeZeroCrossNTBReport, buf->lid, e_NETMMSG, 0, 0, e_MainRoute);
    }
    /*LongMacDataRequst(pMacIndication->MpduLength, pMacPublicHeader, nnib.ParentTEI, pMacPublicHeader->SendType,
                      e_MMeZeroCrossNTBReport, pMacIndication->LinkId, e_NETMMSG, 0, 0);*/

#endif

}


#if 1

U32 ReceiveRequestID;
//����AODVǰ��Ҫ��¼�Ĳ���
U8			 g_AodvMsduHandle = 0; //APS�㴫������handle��������㷢��AODVʱ����
U8 			 g_AodvMsduLID=0;//APS�㴫������LID
U8			 g_DstTEI = 0;    //���ݵ�Ŀ��TEI
U8			 g_OriginalTEI = 0;//����AODVǰ��¼���ݵķ�����
MDB_t   	        AodvMdb;//���Ҫ���͵����ݵĻ���

AODV_BUFF_t    AodvBuff;
ROUTE_TABLE_RECD_t 			   RouteTableEntryOne;

extern ostimer_t 	g_WaitReplyTimer; //�ȴ�·��Ӧ��ʱ
extern ostimer_t 	g_ReplyTimer;   //·��Ӧ���Ż���ʱʱ��

U8 AODVMacUseFlag = FALSE;

/*******************************************����ΪAODV����************************************************/
void	SendMMeRouteReq( U16 DstTEI, U16 ScrTei, U16 Handle)
{
    //U16								DelayTime = 0;
    U8								msduLen, macLen, linkid;
    MAC_PUBLIC_HEADER_t    			*pMacData;
    ROUTE_REQUEST_t 				*pRouteReq_t;
    ROUTING_DISCOVERY_TABLE_ENTRY_t RouteDiscoveryOne;
    U8								*pManageTxBuffer;


    if(nnib.NodeState != e_NODE_ON_LINE)return;
    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendMMeRouteReq", MEM_TIME_OUT);
    pMacData = (MAC_PUBLIC_HEADER_t *) pManageTxBuffer;
    if(AODVMacUseFlag)
        pRouteReq_t = (ROUTE_REQUEST_t *) (pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
    else
        pRouteReq_t = (ROUTE_REQUEST_t *) (pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));
    //memset(ManageTxBuffer, 0, sizeof(ManageTxBuffer));
    pRouteReq_t->MmsgHeader_t.MmsgType = e_MMeRouteRequest;
    linkid = 1;

    pRouteReq_t->RouteRepairVer = 0;
    pRouteReq_t->RouteRequestSeq = g_MsduSequenceNumber + (GetTEI() << 16);
    pRouteReq_t->RouteSelectFlg = 1;
    pRouteReq_t->PayloadType = e_WithOutPayload;
    pRouteReq_t->PayloadLen = 0;

    msduLen = sizeof(ROUTE_REQUEST_t);
    net_printf("---------g_MsduSequenceNumber=0x%04x,nnib.StaTEI=0x%04x,find=0x%04x-----\n",g_MsduSequenceNumber,GetTEI(),DstTEI);
    RouteDiscoveryOne.SrcTEI = GetTEI();
    RouteDiscoveryOne.UpTEI = INVALID;
    RouteDiscoveryOne.DstTEI = DstTEI;
    RouteDiscoveryOne.RouteRequestID = g_MsduSequenceNumber;
    RouteDiscoveryOne.RemainRadius = AODV_MAX_DEPTH;
    RouteDiscoveryOne.ExpirationTime = MAX_DISCOERY_EPIRATION_TIME;
    AddRoutingDiscoveryTableEntry(&RouteDiscoveryOne);


    CreatMMsgMacHeader(pMacData
                       , msduLen
                       , (AODVMacUseFlag ?  BroadcastAddress : NULL)
                       , DstTEI
                       , e_FULL_BROADCAST_FREAM_NOACK
                       , NWK_MMEMSG_MAX_RETRIES, e_USE_PROXYROUTE, AODV_MAX_DEPTH, AODV_MAX_DEPTH,e_TWO_WAY
                       , AODVMacUseFlag);
    macLen =  msduLen
            + (AODVMacUseFlag ? sizeof(MAC_LONGDATA_HEADER) : sizeof(MAC_PUBLIC_HEADER_t))
            + CRCLENGTH;

//    LongMacDataRequst(macLen, pMacData, BROADCAST_TEI, e_FULL_BROADCAST_FREAM_NOACK,
//                      e_MMeRouteRequest, linkid, e_NETMMSG, 0, 0);

    MacDataRequst(pMacData, macLen, BROADCAST_TEI, e_FULL_BROADCAST_FREAM_NOACK,
                  e_MMeRouteRequest, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);


    //DelayTime = 100;
    //������ʱ������ʱ��ʱ�䵽δ�ҵ�·����ScrTei����·�ɴ�������
    //g_BeInAodvStatus = TRUE;

    g_AodvMsduHandle = Handle;
    g_OriginalTEI = ScrTei;
    g_DstTEI = DstTEI;
    timer_start(&g_WaitReplyTimer);//��ʱ�����㿪ʼ��ʱs
    zc_free_mem(pManageTxBuffer);



}
void	ProcessMMeRouteRequest(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8								macuse,headerlen;
    U16								srcTEI,dstTEI, DelayTime,msduLen;
    U32								Crc32Datatest;
    MAC_PUBLIC_HEADER_t 			*pMacData_t;
//    MACDATA_INDICATION_t			*pMacIndication_t;
    ROUTE_REQUEST_t 				*pRouteReq_t;
    ROUTING_DISCOVERY_TABLE_ENTRY_t *pRouteDiscoveryOne;
    ROUTE_TABLE_RECD_t				*pRouteTableEntry_t;
//    pMacIndication_t	= (MACDATA_INDICATION_t *) MacInd;

    sof_ctrl_t *pctrl = (sof_ctrl_t *)(buf->fi.head);
    pMacData_t =   (MAC_PUBLIC_HEADER_t *)pMacInd;

    macuse = pMacData_t->MacAddrUseFlag;
    if(nnib.NodeState != e_NODE_ON_LINE)
        return;
//    return;//<<<
    if(macuse == TRUE)
    {
        pRouteReq_t	   = (ROUTE_REQUEST_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];
        headerlen = sizeof(MAC_LONGDATA_HEADER);

        //<<<
        MAC_LONGDATA_HEADER *pMacHeader = (MAC_LONGDATA_HEADER *)pMacInd;
        __memcpy(pMacHeader->SrcMacAddress, nnib.MacAddr, 6);

    }
    else
    {
        pRouteReq_t	   = (ROUTE_REQUEST_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];
        headerlen = sizeof(MAC_PUBLIC_HEADER_t);

    }
    ReceiveRequestID  = pRouteReq_t->RouteRequestSeq;
    srcTEI	= pRouteReq_t->RouteRequestSeq >> 16;
    if(srcTEI == GetTEI())
    {
        return;
    }
    dstTEI = pMacData_t->DestTEI;

    pRouteDiscoveryOne =&RouteDiscoveryEntryOne ;
    pRouteTableEntry_t = &RouteTableEntryOne;

    if(IsInRouteDiscoveryTable(ReceiveRequestID, dstTEI)) //Existing
    //if(IsInRouteDiscoveryTable(srcTEI, dstTEI)) //Existing
    {
        net_printf("RoutingDiscovery is Existing\n");
        if(IsLessRelayInRreqCmd(ReceiveRequestID, dstTEI, pMacData_t->RemainRadius) == -1) // if better,will  replace
        {
            pRouteDiscoveryOne->SrcTEI = pRouteReq_t->RouteRequestSeq >> 16;
             pRouteDiscoveryOne->UpTEI = pctrl->stei;
             pRouteDiscoveryOne->DstTEI = dstTEI;
             pRouteDiscoveryOne->RouteRequestID = ReceiveRequestID;
             pRouteDiscoveryOne->RemainRadius = pMacData_t->RemainRadius;
             pRouteDiscoveryOne->ExpirationTime = MAX_DISCOERY_EPIRATION_TIME;
            AddRoutingDiscoveryTableEntry(pRouteDiscoveryOne);

             //�������е�·�ɱ�
            pRouteTableEntry_t->DestTEI = pRouteDiscoveryOne->SrcTEI;
            pRouteTableEntry_t->NextHopTEI = pctrl->stei;
            pRouteTableEntry_t->RouteError = 0;
            pRouteTableEntry_t->RouteState = ACTIVE_ROUTE;
            if(!IS_BROADCAST_TEI(pRouteTableEntry_t->NextHopTEI))
           {

//                UpdateRoutingTableEntry(pRouteTableEntry_t);
                UpdateAodvRouteTableEntry(pRouteTableEntry_t, e_AODV_UP);
           }
        }
        else
        {
            return;
        }
    }
    else    //add RoutingDiscovery
    {
        net_printf("add RoutingDiscovery,srcTEI:0x%04x, UpTEI:%4x, DstTEI:%04x\n",srcTEI, pctrl->stei, dstTEI);
        pRouteDiscoveryOne->SrcTEI = pRouteReq_t->RouteRequestSeq >> 16;
        pRouteDiscoveryOne->UpTEI = pctrl->stei;
        pRouteDiscoveryOne->DstTEI = dstTEI;
        pRouteDiscoveryOne->RouteRequestID = ReceiveRequestID;
        pRouteDiscoveryOne->RemainRadius = pMacData_t->RemainRadius;
        pRouteDiscoveryOne->ExpirationTime = MAX_DISCOERY_EPIRATION_TIME;
        AddRoutingDiscoveryTableEntry(pRouteDiscoveryOne);

        //�������е�·�ɱ�
        pRouteTableEntry_t->DestTEI = pRouteDiscoveryOne->SrcTEI;
        pRouteTableEntry_t->NextHopTEI = pctrl->stei;
        pRouteTableEntry_t->RouteError = 0;
        pRouteTableEntry_t->RouteState = ACTIVE_ROUTE;
        if(!IS_BROADCAST_TEI(pRouteTableEntry_t->NextHopTEI))
        {

            //UpdateRoutingTableEntry(pRouteTableEntry_t);
            UpdateAodvRouteTableEntry(pRouteTableEntry_t, e_AODV_UP);
        }
    }
    if(dstTEI == GetTEI())//��·���޸���Ŀ��
    {
        net_printf("--------------timer_start----------\n");

        timer_start(&g_ReplyTimer);//��ʱ�����㿪ʼ��ʱ
    }
    else   //�м�ڵ�
    {
        net_printf("--------------pRouteReq_t->PayloadType=%d----------\n",e_WithPayload);
        if(AccessListSwitch)
        {
            pMacData_t->MsduSeq = ++g_MsduSequenceNumber;       //��ֹ���Թ��̱�����
        }
        //ת��·������
//        pMacData_t->RemainRadius--;
       /* if(pRouteReq_t->PayloadType == e_WithPayload)
        {
            U8 offSet;
            RELAY_LIST_t relayList;
            relayList.RelayTEI = pMacIndication_t->UpTEI;
            relayList.CommRate = 100;//?
            relayList.LQI = pMacIndication_t->RSSI;
            offSet = pRouteReq_t->PayloadLen;
            __memcpy(&pRouteReq_t->Payload[offSet], &relayList, sizeof(RELAY_LIST_t));
            pRouteReq_t->PayloadLen +=sizeof(RELAY_LIST_t);
            pMacIndication_t->Mdb.PayloadLen += sizeof(RELAY_LIST_t);
            pMacData_t->MsduLength_l +=sizeof(RELAY_LIST_t);
        }*/
        msduLen= len -headerlen-CRCLENGTH;
        net_printf("pMacData_t->MsduLength_l=%d,msduLen=%d\n",pMacData_t->MsduLength_l,msduLen);
        DelayTime = 0;//rand()%100;

        if(msduLen ==0 || msduLen>512)
        {
            //sys_panic("<ProcessMMeRouteRequest MsduLen err:%d \n> %s: %d\n",msduLen, __func__, __LINE__);
            debug_printf(&dty,DEBUG_GE,"ProcessMMeRouteRequest MsduLen err: %d",msduLen);
        }
        crc_digest(&pMacInd[headerlen],msduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
        __memcpy(&pMacInd[headerlen+msduLen], (U8 *)&Crc32Datatest, CRCLENGTH);
        //���������ʱʱ��
//        LongMacDataRequst(pMacIndication_t->Mdb.PayloadLen, pMacData_t, BROADCAST_TEI, e_FULL_BROADCAST_FREAM_NOACK,
//                          e_MMeRouteRequest, pMacIndication_t->LinkId, e_NETMMSG, 0, DelayTime);
        MacDataRequst(pMacData_t, len,BROADCAST_TEI, e_FULL_BROADCAST_FREAM_NOACK,
                      e_MMeRouteRequest, buf->lid, e_NETMMSG, 0, DelayTime, e_MainRoute,0);

    }

}

void SendLinkRequest( struct ostimer_s *ostimer, void *arg)
{
    U8								msduLen, macLen, linkid;
    U16 							TEI;
    MAC_PUBLIC_HEADER_t    			*pMacData;
    LINK_CONFIRM_REQUEST_t 		    *pLinkReq;
    ROUTING_DISCOVERY_TABLE_ENTRY_t *pRouteDiscovery;
    U8								*pManageTxBuffer;
    if(nnib.NodeState != e_NODE_ON_LINE)return;

    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendLinkRequest", MEM_TIME_OUT);
    pMacData = (MAC_PUBLIC_HEADER_t *) pManageTxBuffer;
    if(AODVMacUseFlag)
        pLinkReq = (LINK_CONFIRM_REQUEST_t *) (pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
    else
        pLinkReq = (LINK_CONFIRM_REQUEST_t *) (pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));
    //memset(ManageTxBuffer, 0, sizeof(ManageTxBuffer));
    pLinkReq->MmsgHeader_t.MmsgType = e_MMeLinkConfirmRequest;
    linkid = 1;

    pRouteDiscovery = GetRouteDiscoveryTableEntry(ReceiveRequestID);
    if(pRouteDiscovery == NULL)
    {
        zc_free_mem(pManageTxBuffer);
        return;
    }
    TEI = pRouteDiscovery->UpTEI;
    pLinkReq->RouteRepairVer =0;
    pLinkReq->RouteRequestSeq =ReceiveRequestID + (pRouteDiscovery->SrcTEI << 16);
    pLinkReq->ConfirmStaNum =1;
    __memcpy(pLinkReq->ConfirmStaList ,&TEI,2);


    net_printf("--------------SendLinkRequest-------DestTei = %03x---\n", TEI);

    msduLen = sizeof(LINK_CONFIRM_REQUEST_t) + pLinkReq->ConfirmStaNum * 2;

    CreatMMsgMacHeader(pMacData
                       , msduLen
                       , (AODVMacUseFlag ? BroadcastAddress : NULL)
                       , TEI
                       , e_UNICAST_FREAM
                       , NWK_UNICAST_MAX_RETRIES
                       , e_USE_PROXYROUTE
                       , AODV_MAX_DEPTH, AODV_MAX_DEPTH,e_TWO_WAY
                       , AODVMacUseFlag);

    macLen = msduLen
            + (AODVMacUseFlag ? sizeof(MAC_LONGDATA_HEADER) : sizeof(MAC_PUBLIC_HEADER_t))
            +CRCLENGTH;
//    LongMacDataRequst(macLen, pMacData, TEI, e_UNICAST_FREAM, e_MMeLinkConfirmRequest, linkid, e_NETMMSG, 0, 0);
    MacDataRequst(pMacData, macLen, TEI, e_UNICAST_FREAM,
                  e_MMeLinkConfirmRequest, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    dump_buf((U8*)pMacData,macLen);
    zc_free_mem(pManageTxBuffer);

}

void ProcessMMeLinkConfirmRequest(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8								macuse,i;
    MAC_PUBLIC_HEADER_t    			*pMacData_t;
//    MACDATA_INDICATION_t			*pMacIndication_t;
    LINK_CONFIRM_REQUEST_t 		    *pLinkReq;

    U16 tei;

    sof_ctrl_t *pCtrlFrame = (sof_ctrl_t *)buf->fi.head;

    if(nnib.NodeState != e_NODE_ON_LINE)
        return;

//    pMacIndication_t	= (MACDATA_INDICATION_t *) MacInd;
    pMacData_t =   (MAC_PUBLIC_HEADER_t *)pMacInd;
    macuse = pMacData_t->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pLinkReq	   = (LINK_CONFIRM_REQUEST_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];

        //<<<
        MAC_LONGDATA_HEADER *pMacHeader = (MAC_LONGDATA_HEADER *)pMacInd;
        __memcpy(pMacHeader->SrcMacAddress, nnib.MacAddr, 6);

    }
    else
    {
        pLinkReq	   = (LINK_CONFIRM_REQUEST_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];

    }
    tei = GetTEI();
    for(i=0;i<pLinkReq->ConfirmStaNum;i++)
    {
        if(memcmp(&pLinkReq->ConfirmStaList[2*i],&tei,2)==0)//�б��д����Լ������ܻظ���·ȷ�ϻظ�
        {
            SendLinkResponse(pLinkReq->RouteRequestSeq,pCtrlFrame->stei,buf->snr_level);
        }
    }

}

void SendLinkResponse(U32 RequestSeq, U8 DstTEI, U8 LIQ)
{
    U8								msduLen, macLen, linkid;
    U16								nextTEI;
    MAC_PUBLIC_HEADER_t    			*pMacData;
    LINK_CONFIRM_RESPONSE_t 	    *pLinkResponse;
    U8								*pManageTxBuffer;

    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendLinkResponse", MEM_TIME_OUT);

    pMacData = (MAC_PUBLIC_HEADER_t *) pManageTxBuffer;
    if(AODVMacUseFlag)
        pLinkResponse = (LINK_CONFIRM_RESPONSE_t *) (pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
    else
        pLinkResponse = (LINK_CONFIRM_RESPONSE_t *) (pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));
    //memset(ManageTxBuffer, 0, sizeof(ManageTxBuffer));
    pLinkResponse->MmsgHeader_t.MmsgType = e_MMeLinkConfirmResponse;
    linkid = 1;

    pLinkResponse->RouteRepairVer = 0;
    pLinkResponse->RouteRequestSeq = RequestSeq;
    pLinkResponse->Deepth = nnib.NodeLevel;
    pLinkResponse->LIQ = LIQ;
    pLinkResponse->RouteSelectFlg = 0;

    net_printf("--------------SendLinkResponse-------DestTei = %03x---\n", DstTEI);



    msduLen = sizeof(LINK_CONFIRM_RESPONSE_t);

    CreatMMsgMacHeader(pMacData
                       , msduLen
                       , (AODVMacUseFlag ? BroadcastAddress : NULL)
                       , DstTEI
                       , e_UNICAST_FREAM
                       , NWK_UNICAST_MAX_RETRIES
                       , e_USE_PROXYROUTE, 1, 1,e_TWO_WAY
                       , AODVMacUseFlag);

    macLen = msduLen
            + (AODVMacUseFlag ? sizeof(MAC_LONGDATA_HEADER) : sizeof(MAC_PUBLIC_HEADER_t))
            +CRCLENGTH;
    nextTEI = DstTEI;
//    LongMacDataRequst(macLen, pMacData, nextTEI, e_UNICAST_FREAM, e_MMeLinkConfirmResponse, linkid, e_NETMMSG, 0, 0);

    MacDataRequst(pMacData, macLen, nextTEI, e_UNICAST_FREAM,
                  e_MMeLinkConfirmResponse, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    zc_free_mem(pManageTxBuffer);

}

void ProcessMMeLinkConfirmResponse(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    SendMMeRouteReply();
}

void	SendMMeRouteReply( )  //��·�ɻظ���Ϊ�ص�����ʹ��
{

    U8								msduLen, macLen, linkid;
    MAC_PUBLIC_HEADER_t    			*pMacData;
    ROUTE_REPLY_t 				    *pRouteReply_t;
    ROUTING_DISCOVERY_TABLE_ENTRY_t *pRouteDiscovery;
    U8								*pManageTxBuffer;

    //timer_delete(g_ReplyTimer);
    if(nnib.NodeState != e_NODE_ON_LINE)return;


    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendMMeRouteReply", MEM_TIME_OUT);

    pMacData = (MAC_PUBLIC_HEADER_t *) pManageTxBuffer;

    if(AODVMacUseFlag)
        pRouteReply_t = (ROUTE_REPLY_t *) (pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
    else
        pRouteReply_t = (ROUTE_REPLY_t *) (pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));

    //memset(ManageTxBuffer, 0, sizeof(ManageTxBuffer));
    pRouteReply_t->MmsgHeader_t.MmsgType = e_MMeRouteReply;
    linkid = 1;

    pRouteDiscovery = GetRouteDiscoveryTableEntry(ReceiveRequestID);
    if(pRouteDiscovery == NULL)
    {
        zc_free_mem(pManageTxBuffer);
        return;
    }

    net_printf("--------------SendMMeRouteReply----------DestTei = %03x\n", pRouteDiscovery->SrcTEI);

    pRouteReply_t->RouteRepairVer = 0;
    pRouteReply_t->RouteRequestSeq = ReceiveRequestID + (pRouteDiscovery->SrcTEI << 16);
    pRouteReply_t->PayloadType = e_WithPayload;
    pRouteReply_t->PayloadLen	= 0;
    msduLen = sizeof(ROUTE_REPLY_t);

    CreatMMsgMacHeader(pMacData
                       , msduLen
                       , (AODVMacUseFlag ? BroadcastAddress : NULL)
                       , pRouteDiscovery->SrcTEI
                       , e_UNICAST_FREAM
                       , NWK_UNICAST_MAX_RETRIES
                       , e_USE_PROXYROUTE
                       , AODV_MAX_DEPTH, AODV_MAX_DEPTH,e_TWO_WAY
                       , AODVMacUseFlag);

    macLen = msduLen
            + (AODVMacUseFlag ? sizeof(MAC_LONGDATA_HEADER) : sizeof(MAC_PUBLIC_HEADER_t))
            +CRCLENGTH;
//    LongMacDataRequst(macLen, pMacData, pRouteDiscovery->UpTEI, e_UNICAST_FREAM, e_MMeRouteReply, linkid, e_NETMMSG, 0, 0);

    MacDataRequst(pMacData, macLen, pRouteDiscovery->UpTEI, e_UNICAST_FREAM,
                  e_MMeRouteReply, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);

    zc_free_mem(pManageTxBuffer);

}
void	ProcessMMeRouteReply(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8								 macuse;
    U16								RequestID;
    MAC_PUBLIC_HEADER_t    			*pMacData_t;
//    MACDATA_INDICATION_t			*pMacIndication_t;
    ROUTE_REPLY_t 				    *pRouteReply_t;
    ROUTING_DISCOVERY_TABLE_ENTRY_t *pRouteDiscovery;
    ROUTE_TABLE_RECD_t   			*pRouteTableEntry_t;

    sof_ctrl_t *pCtrlFrame = (sof_ctrl_t *)buf->fi.head;


    if(nnib.NodeState != e_NODE_ON_LINE)return;

//    pMacIndication_t	= (MACDATA_INDICATION_t *) MacInd;s
    pMacData_t =   (MAC_PUBLIC_HEADER_t *)pMacInd;

    macuse = pMacData_t->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pRouteReply_t 	= (ROUTE_REPLY_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];

        //<<<
        MAC_LONGDATA_HEADER *pMacHeader = (MAC_LONGDATA_HEADER *)pMacInd;
        __memcpy(pMacHeader->SrcMacAddress, nnib.MacAddr, 6);

    }
    else
    {
        pRouteReply_t 	= (ROUTE_REPLY_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];

    }
    pRouteTableEntry_t = &RouteTableEntryOne;

    RequestID = pRouteReply_t->RouteRequestSeq;
    pRouteDiscovery = GetRouteDiscoveryTableEntry(RequestID);


    if(pRouteDiscovery == NULL)
    {
        return;
    }
    net_printf("UpdateAodvRouteTableEntry,DestTEI=%03x,NextHopTEI=%03x\n",pRouteDiscovery->DstTEI, pCtrlFrame->stei);
    //�������µ�·�ɱ�
    pRouteTableEntry_t->DestTEI = pRouteDiscovery->DstTEI;
    pRouteTableEntry_t->NextHopTEI = pCtrlFrame->stei;
    pRouteTableEntry_t->RouteError = 0;
    pRouteTableEntry_t->RouteState = ACTIVE_ROUTE;
    if(!IS_BROADCAST_TEI(pRouteTableEntry_t->NextHopTEI))
    {

//        UpdateRoutingTableEntry(pRouteTableEntry_t);
        UpdateAodvRouteTableEntry(pRouteTableEntry_t, e_AODV_DOWN);
    }


    if(GetTEI() == pRouteDiscovery->SrcTEI)//�Ǳ��ڵ㷢���·������
    {
        //����·�ɻ�Ӧ����
        SendMMeRouteAck(pRouteReply_t->RouteRequestSeq, pRouteDiscovery->DstTEI);
    }
    else//�м�ڵ�
    {
        //����ת��·�ɻظ�����
//        pMacData_t->RemainRadius--;

//        LongMacDataRequst(pMacIndication_t->Mdb.PayloadLen, pMacData_t, pRouteDiscovery->UpTEI, e_UNICAST_FREAM,
//                          e_MMeRouteReply, pMacIndication_t->LinkId, e_NETMMSG, 0, 0);

        MacDataRequst(pMacData_t, len, pRouteDiscovery->UpTEI, e_UNICAST_FREAM,
                      e_MMeRouteReply, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }
    //ɾ��·�ɷ��ֱ�
    // DleteRouteDiscoveryTableEntry(RequestID);//���յ�·�ɻ�Ӧ�����ɾ��

}
void	SendMMeRouteAck(U32 RouteRequestSeq, U16 DstTEI)
{
    U8								msduLen, macLen, linkid;
    U16								nextTEI;
    MAC_PUBLIC_HEADER_t    			*pMacData;
    ROUTE_ACK_t 				    *pRouteAck_t;
    U8								*pManageTxBuffer;

    if(nnib.NodeState != e_NODE_ON_LINE)return;
    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendMMeRouteAck", MEM_TIME_OUT);
    pMacData = (MAC_PUBLIC_HEADER_t *) pManageTxBuffer;
    if(AODVMacUseFlag)
        pRouteAck_t = (ROUTE_ACK_t *) (pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
    else
        pRouteAck_t = (ROUTE_ACK_t *) (pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));
    //memset(ManageTxBuffer, 0, sizeof(ManageTxBuffer));
    pRouteAck_t->MmsgHeader_t.MmsgType = e_MMeRouteAck;
    linkid = 1;

    pRouteAck_t->RouteRepairVer = 0;
    pRouteAck_t->RouteRequestSeq = RouteRequestSeq;


    net_printf("--------------SendMMeRouteAck----------DestTei = %03x\n", DstTEI);


    msduLen = sizeof(ROUTE_ACK_t);

    CreatMMsgMacHeader(pMacData
                       , msduLen
                       , (AODVMacUseFlag ? BroadcastAddress : NULL)
                       , DstTEI
                       , e_UNICAST_FREAM
                       , NWK_UNICAST_MAX_RETRIES
                       , e_USE_PROXYROUTE
                       , AODV_MAX_DEPTH, AODV_MAX_DEPTH,e_TWO_WAY
                       , AODVMacUseFlag);

    macLen = msduLen
            + (AODVMacUseFlag ? sizeof(MAC_LONGDATA_HEADER) : sizeof(MAC_PUBLIC_HEADER_t))
            +CRCLENGTH;
    nextTEI = SearchNextHopFromAodvRouteTable(DstTEI, ACTIVE_ROUTE);
    net_printf("-----------SendMMeRouteAck nextTEI:%02x----------\n",nextTEI);

    //g_BeInAodvStatus = FALSE;
    timer_stop(&g_WaitReplyTimer,TMR_NULL);//�ȵ�·�ɻظ���ɾ����ʱ��
    if(GET_TEI_VALID_BIT(nextTEI) != BROADCAST_TEI)
    {
//        LongMacDataRequst(macLen, pMacData, nextTEI, e_UNICAST_FREAM, e_MMeRouteAck, linkid, e_NETMMSG, 0, 0);

        MacDataRequst(pMacData, macLen, nextTEI, e_UNICAST_FREAM,
                      e_MMeRouteAck, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
        net_printf( "------------LongMacDataRequst--------------\n");

//        os_sleep(10); //<<<
        //���ͻ���������
        MacDataRequst((MAC_PUBLIC_HEADER_t*)AodvBuff.payload, AodvBuff.payloadLen, nextTEI,
                           e_UNICAST_FREAM,g_AodvMsduHandle, g_AodvMsduLID, e_APS_FREAME, 0, 0,e_AODVRoute,0);
        net_printf( "------------ShortMacDataRequst--------------\n");
    }
    else
    {
        if(g_OriginalTEI == GetTEI())//����AODV�ͷ������ݵ���ͬһ�ڵ㣬�ϱ�AODV״̬
        {
            work_t *work = zc_malloc_mem(sizeof(work_t) + sizeof(MACDATA_CONFIRM_t), "WaitReplyCmdOut", MEM_TIME_OUT);
            MACDATA_CONFIRM_t  *pMsduCfm_t = (MACDATA_CONFIRM_t *)work->buf;
//            pMsduCfm_t->dataLen = 0;
            pMsduCfm_t->MsduHandle = g_AodvMsduHandle;
            pMsduCfm_t->MsduType = e_APS_FREAME;
    //        zc_free_mdb("WaitReplyCmdOut2",AodvMdb.pMallocHeader);

            pMsduCfm_t->status = e_ROUTE_ERR;
            work->work = ProcMpduDataConfirm;
			work->msgtype=MAC_CFM;

            post_datalink_task_work(work);
        }

    }
    zc_free_mem(pManageTxBuffer);
}

void	ProcessMMeRouteAck(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    U8								 macuse;
    U16							     nextTEI, RequestID;
    MAC_PUBLIC_HEADER_t    			*pMacData_t;
//    MACDATA_INDICATION_t			*pMacIndication_t;
    ROUTE_TABLE_RECD_t   			*pRouteTableEntry_t;
    ROUTE_ACK_t 				    *pRouteAck_t;
    ROUTING_DISCOVERY_TABLE_ENTRY_t *pRouteDiscovery;

    if(nnib.NodeState != e_NODE_ON_LINE)return;
//    pMacIndication_t	= (MACDATA_INDICATION_t *) MacInd;
    pMacData_t =   (MAC_PUBLIC_HEADER_t *)pMacInd;

    sof_ctrl_t *pCtrlFrame = (sof_ctrl_t *)buf->fi.head;

    macuse = pMacData_t->MacAddrUseFlag;

    if(macuse == TRUE)
    {
        pRouteAck_t 	= (ROUTE_ACK_t *)&pMacInd[sizeof(MAC_LONGDATA_HEADER)];

        //<<<
        MAC_LONGDATA_HEADER *pMacHeader = (MAC_LONGDATA_HEADER *)pMacInd;
        __memcpy(pMacHeader->SrcMacAddress, nnib.MacAddr, 6);

    }
    else
    {
        pRouteAck_t 	= (ROUTE_ACK_t *)&pMacInd[sizeof(MAC_PUBLIC_HEADER_t)];

    }

    RequestID  = pRouteAck_t->RouteRequestSeq;
    pRouteDiscovery = GetRouteDiscoveryTableEntry(RequestID);


    if(pRouteDiscovery == NULL)
    {
        return;
    }




    pRouteTableEntry_t = &RouteTableEntryOne;

    //�������ϵ�·�ɱ�
    pRouteTableEntry_t->DestTEI = pMacData_t->ScrTEI;
    pRouteTableEntry_t->NextHopTEI = pCtrlFrame->stei;
    pRouteTableEntry_t->RouteError = 0;
    pRouteTableEntry_t->RouteState = ACTIVE_ROUTE;
    if(!IS_BROADCAST_TEI(pRouteTableEntry_t->NextHopTEI))
    {

//        UpdateRoutingTableEntry(pRouteTableEntry_t);
        UpdateAodvRouteTableEntry(pRouteTableEntry_t, e_AODV_UP);
    }
    //�ж��Ƿ���Ҫ����ת��
    if(pMacData_t->DestTEI !=GetTEI())
    {
        nextTEI = SearchNextHopFromAodvRouteTable(pMacData_t->DestTEI, ACTIVE_ROUTE);
        if(GET_TEI_VALID_BIT(nextTEI) != BROADCAST_TEI)
        {
//            LongMacDataRequst(pMacIndication_t->Mdb.PayloadLen, pMacData_t,
//                              nextTEI, e_UNICAST_FREAM, e_MMeRouteAck, pMacIndication_t->LinkId, e_NETMMSG, 0, 0);

            MacDataRequst(pMacData_t, len, nextTEI, e_UNICAST_FREAM,
                          e_MMeRouteAck, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
    }
    DleteRouteDiscoveryTableEntry(RequestID);

}


void	SendMMeRouteError(U16 OriginalTEI,U16 DstTEI)
{
    U8								msduLen, macLen, linkid;
    U16								nextTEI,routeseq;
    MAC_PUBLIC_HEADER_t    			*pMacData;
    ROUTE_ERR_t 				    *pRouteErr_t;
    U8								*pManageTxBuffer;

    //timer_delete(g_ReplyTimer);
    if(nnib.NodeState != e_NODE_ON_LINE)return;
    pManageTxBuffer = zc_malloc_mem(MAX_NET_MMDATA_LEN, "SendMMeRouteError", MEM_TIME_OUT);
    pMacData = (MAC_PUBLIC_HEADER_t *) pManageTxBuffer;
    if(AODVMacUseFlag)
        pRouteErr_t = (ROUTE_ERR_t *) (pManageTxBuffer + sizeof(MAC_LONGDATA_HEADER));
    else
        pRouteErr_t = (ROUTE_ERR_t *) (pManageTxBuffer + sizeof(MAC_PUBLIC_HEADER_t));
    //memset(ManageTxBuffer, 0, sizeof(ManageTxBuffer));
    pRouteErr_t->MmsgHeader_t.MmsgType = e_MMeRouteError;
    linkid = 1;


    routeseq = GetRouteIDBySrcTEI(GetTEI());
    net_printf("--------------------routeseq=0x%02x-----OriginalTEI:%03x------\n",routeseq, OriginalTEI);
    if(routeseq ==INVALID ) //��ֹ·�ɷ��ֱ���ʧ
    {
        routeseq = g_MsduSequenceNumber-1;
    }

    pRouteErr_t->RouteRepairVer = 0;
    pRouteErr_t->RouteRequestSeq = routeseq + (GetTEI() << 16);
    pRouteErr_t->UnArriveNum = 1;
    __memcpy(pRouteErr_t->UnArriveList,&DstTEI ,2);


    msduLen = sizeof(ROUTE_ERR_t) + pRouteErr_t->UnArriveNum * 2;

    CreatMMsgMacHeader(pMacData
                       , msduLen
                       , (AODVMacUseFlag ? BroadcastAddress : NULL)
                       , OriginalTEI
                       , e_UNICAST_FREAM
                       , NWK_UNICAST_MAX_RETRIES
                       , e_USE_PROXYROUTE
                       , AODV_MAX_DEPTH, AODV_MAX_DEPTH,e_TWO_WAY
                       , AODVMacUseFlag);

    macLen = msduLen
            + (AODVMacUseFlag ? sizeof(MAC_LONGDATA_HEADER) : sizeof(MAC_PUBLIC_HEADER_t))
            +CRCLENGTH;
    nextTEI = SearchNextHopAddrFromRouteTable(OriginalTEI, ACTIVE_ROUTE);
    net_printf("---------------SendMMeRouteError nextTEI:%03x------\n",nextTEI);
    if(GET_TEI_VALID_BIT(nextTEI) != BROADCAST_TEI)
    {
//        LongMacDataRequst(macLen, pMacData, nextTEI, e_UNICAST_FREAM, e_MMeRouteError, linkid, e_NETMMSG, 0, 0);

        MacDataRequst(pMacData, macLen, nextTEI, e_UNICAST_FREAM,
                      e_MMeRouteError, linkid, e_NETMMSG, 0, 0, e_MainRoute,0);
    }

     zc_free_mem(pManageTxBuffer);
}

void	ProcessMMeRouteError(mbuf_t *buf, U8 *pMacInd, U16 len)
{
    //�յ�·�ɴ���Ľڵ�ɾ��·�ɱ� ?
    //	U8								 macuse;
    U16							     nextTEI;
    MAC_PUBLIC_HEADER_t    			*pMacData_t;
//    MACDATA_INDICATION_t			*pMacIndication_t;
    //	ROUTE_ERR_t 					 *pRouteErr_t;
    if(nnib.NodeState != e_NODE_ON_LINE)return;

//    pMacIndication_t	= (MACDATA_INDICATION_t *) MacInd;
    pMacData_t =   (MAC_PUBLIC_HEADER_t *)pMacInd;

    /*	 macuse = pMacData_t->MacAddrUseFlag;

        if(macuse == TRUE)
        {
            pRouteErr_t 	=(ROUTE_ERR_t *)&pMacIndication_t->Mdb.pPayload[sizeof(MAC_LONGDATA_HEADER)];

        }
        else
        {
            pRouteErr_t 	=(ROUTE_ERR_t *)&pMacIndication_t->Mdb.pPayload[sizeof(MAC_PUBLIC_HEADER_t)];

        }
    */

    if(pMacData_t->MacAddrUseFlag)
    {
        //<<<
        MAC_LONGDATA_HEADER *pMacHeader = (MAC_LONGDATA_HEADER *)pMacInd;
        __memcpy(pMacHeader->SrcMacAddress, nnib.MacAddr, 6);
    }

    //�ж��Ƿ���Ҫ����ת��
    if(pMacData_t->DestTEI != GetTEI())
    {
        nextTEI = SearchNextHopAddrFromRouteTable(pMacData_t->DestTEI, ACTIVE_ROUTE);
        if(GET_TEI_VALID_BIT(nextTEI) != BROADCAST_TEI)
        {
//            LongMacDataRequst(pMacIndication_t->Mdb.PayloadLen, pMacData_t,
//                              nextTEI, e_UNICAST_FREAM, e_MMeRouteError, pMacIndication_t->LinkId, e_NETMMSG, 0, 0);

            MacDataRequst(pMacData_t, len, nextTEI, e_UNICAST_FREAM,
                          e_MMeRouteError, buf->lid, e_NETMMSG, 0, 0, e_MainRoute,0);
        }
    }
}


//�ȴ�AODV��ʱ�Ļص�����
void	WaitReplyCmdOut(struct ostimer_s *ostimer, void *arg)
{
    //g_BeInAodvStatus = FALSE;
    net_printf("------------WaitReplyCmdOut,g_OriginalTEI=%04x-----\n",g_OriginalTEI);
    if(g_OriginalTEI == GetTEI())//����AODV�ͷ������ݵ���ͬһ�ڵ㣬�ϱ�AODV״̬
    {
        work_t *work = zc_malloc_mem(sizeof(work_t) + sizeof(MACDATA_CONFIRM_t), "WaitReplyCmdOut", MEM_TIME_OUT);
        MACDATA_CONFIRM_t  *pMsduCfm_t = (MACDATA_CONFIRM_t *)work->buf;
//        pMsduCfm_t->dataLen = 0;
        pMsduCfm_t->MsduHandle = g_AodvMsduHandle;
        pMsduCfm_t->MsduType = e_APS_FREAME;
//        zc_free_mdb("WaitReplyCmdOut2",AodvMdb.pMallocHeader);

        pMsduCfm_t->status = e_ROUTE_ERR;
        work->work = ProcMpduDataConfirm;
		work->msgtype=MAC_CFM;
        post_datalink_task_work(work);
    }
    else//�����ݷ����߷���·�ɴ���
    {
        SendMMeRouteError(g_OriginalTEI,g_DstTEI);
//        zc_free_mdb("WaitReplyCmdOut2",AodvMdb.pMallocHeader);
    }
}

#endif




static	NetMgmMsgFunc_t NetMgmMsgArray[] =
{
    {e_MMeAssocReq,						ProcessMMeAssocReq				 },
    {e_MMeAssocCfm,						ProcessMMeAssocCnf				 },

    {e_MMeChangeProxyReq,				ProcessMMeChangeProxyReq		 },

    {e_MMeHeartBeatCheck,				ProcessMMeHeartBeatCheck		 },
    {e_MMeDiscoverNodeList,				ProcessMMeDiscoverNodeList		 },
    {e_MMeSuccessRateReport,			ProcessMMeSuccessRateReport 	 },
    {e_MMeNetworkConflictReport,		ProcessMMeNetworkConflictReport  },

    {e_MMeZeroCrossNTBReport,			ProcessMMeZeroCrossNTBReport	 },
#if defined(STATIC_NODE)
    {e_MMeAssocGatherInd,				ProcessMMeAssocGatherInd		 },
    {e_MMeChangeProxyCnf,				ProcessMMeChangeProxyCnf		 },
    {e_MMeChangeProxyBitMapCnf,			ProcessMMeChangeProxyBitMapCnf	 },
    {e_MMeLeaveInd,						ProcessMMeLeaveInd				 },
    {e_MMeZeroCrossNTBCollectInd,		ProcessMMeZeroCrossNTBCollectInd },
#endif

    {e_MMeRouteRequest,		            ProcessMMeRouteRequest			 },
    {e_MMeRouteReply,		            ProcessMMeRouteReply			 },
    {e_MMeRouteError,		            ProcessMMeRouteError			 },
    {e_MMeRouteAck,		                ProcessMMeRouteAck				 },
    {e_MMeLinkConfirmRequest,		    ProcessMMeLinkConfirmRequest	 },
    {e_MMeLinkConfirmResponse,		    ProcessMMeLinkConfirmResponse	 },
	{e_MMeRfChannelConfilictReport,		ProcessMMeRFChannelConflictReport},
};


void ProcessNetMgmMsg(mbuf_t *buf,  U8 *pld, U16 len,work_t *work)
{
    U16 					byCmdID = 0;
    U8						*pMsdu;
    MAC_PUBLIC_HEADER_t 	*pMacHeader = NULL;



    pMacHeader =   (MAC_PUBLIC_HEADER_t *)pld;

//    if(getHplcTestMode() == RD_CTRL)
//    {
//        net_printf("-----testmode == RD_CTRL-----free----------\n");
//        return;
//    }




    if(pMacHeader->MacAddrUseFlag)
    {
        pMsdu =   pld + sizeof(MAC_LONGDATA_HEADER);
    }
    else
    {
        pMsdu =   pld + sizeof(MAC_PUBLIC_HEADER_t);
    }

    netMsgRecordIndex = work->record_index;

    byCmdID = pMsdu[0] + (pMsdu[1] << 8);
//	*mgType = byCmdID+NETMMG;
    mg_update_type(byCmdID+NETMMG, work->record_index);
//	mg_update_type(*mgType);
    //net_printf("byCmdID = %04x, GetNodeState = %02x\n", byCmdID, GetNodeState());
    if(GetNodeState() == e_NODE_OUT_NET ) //δ������δ����TEI��ֻ�������ָʾ�͹�������ָʾ
    {
        if((byCmdID != e_MMeAssocCfm) &&  (byCmdID != e_MMeAssocGatherInd) &&  (byCmdID != e_MMeDiscoverNodeList))
        {
            return;
        }
    }


#if defined(STATIC_NODE)//����
    if(byCmdID !=e_MMeAssocReq && byCmdID !=e_MMeChangeProxyReq)
    {
        if(pMacHeader->MacAddrUseFlag)
        {
            if(!checkAccessControlList(&pld[sizeof(MAC_PUBLIC_HEADER_t)], 1) )
            {
                if(AccessListSwitch ==TRUE)
                {
                    net_printf("------free4---%2x %2x %2x %2x %2x %2x-------\n",
                               pld[sizeof(MAC_PUBLIC_HEADER_t)],
                               pld[sizeof(MAC_PUBLIC_HEADER_t)+1],
                               pld[sizeof(MAC_PUBLIC_HEADER_t)+2],
                               pld[sizeof(MAC_PUBLIC_HEADER_t)+3],
                               pld[sizeof(MAC_PUBLIC_HEADER_t)+4],
                               pld[sizeof(MAC_PUBLIC_HEADER_t)+5]);
                    return;
                }
            }
        }
        else
        {
            if(!checkAccessControlListByTEI(get_sof_stei((sof_ctrl_t *)buf->fi.head),GetSNID(), 1) )
            {
                if(AccessListSwitch ==TRUE)
                {
                    net_printf("free TEI=%06x aodv cmd\n", get_sof_stei((sof_ctrl_t *)buf->fi.head));
                    return;
                }
            }
        }
    }
#endif

    //�����ж��յ���TEI״̬������״̬����
#if defined(STATIC_MASTER)

    if(pMacHeader->FormationSeq != nnib.FormationSeqNumber)
        return;

    if(pMacHeader->ScrTEI > CCO_TEI )//��ֹ����Խ��
    {
        if(DeviceTEIList[pMacHeader->ScrTEI-2].NodeState != e_NODE_OUT_NET)//���ڸ��� ʱ��
        {
            DeviceTEIList[pMacHeader->ScrTEI-2].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
            DeviceTEIList[pMacHeader->ScrTEI-2].NodeState = e_NODE_ON_LINE;

            //���ֹ���TEI�Ľڵ㣬����CCO��¼�Ľڵ�����
            if(pMacHeader->MacAddrUseFlag && 0!= memcmp(DeviceTEIList[pMacHeader->ScrTEI-2].MacAddr, pld + sizeof(MAC_PUBLIC_HEADER_t), 6))
            {
                SendMMeDelayLeaveOfNum(pld + sizeof(MAC_PUBLIC_HEADER_t), 1, 0, e_LEAVE_AND_DEL_DEVICELIST);
            }
        }
        else//���ⷢ������ָʾ��ģ������
        {
            if(byCmdID == e_MMeLeaveInd)
            {
                return;
            }
//            MMeDelayLeaveOfflineNode(&pMacIndication->Mpdu[sizeof(MAC_PUBLIC_HEADER_t)],0);
            if(pMacHeader->MacAddrUseFlag )
            {
                SendMMeDelayLeaveOfNum(pld + sizeof(MAC_PUBLIC_HEADER_t), 1, 0, e_LEAVE_AND_DEL_DEVICELIST);
            }
            return;

        }
    }
#endif


    U16 i, count;

    count = sizeof(NetMgmMsgArray) / sizeof(NetMgmMsgFunc_t);
    for(i = 0 ; i < count ; i++)
    {
        if(byCmdID == NetMgmMsgArray[i].MMType)
        {
            /*
            net_printf("net r: %s\n",byCmdID == e_MMeAssocReq?"e_MMeAssocReq":
                                                            byCmdID == e_MMeAssocCfm ?"e_MMeAssocCfm":
                                                            byCmdID == e_MMeAssocGatherInd ?"e_MMeAssocGatherInd":
                                                            byCmdID == e_MMeChangeProxyReq ?"e_MMeChangeProxyReq":
                                                            byCmdID == e_MMeChangeProxyCnf ?"e_MMeChangeProxyCnf":
                                                            byCmdID == e_MMeChangeProxyBitMapCnf?"e_MMeChangeProxyBitMapCnf":
                                                            byCmdID == e_MMeLeaveInd?"e_MMeLeaveInd":
                                                            byCmdID == e_MMeHeartBeatCheck?"e_MMeHeartBeatCheck":
                                                            byCmdID == e_MMeDiscoverNodeList?"e_MMeDiscoverNodeList":
                                                            byCmdID == e_MMeSuccessRateReport?"e_MMeSuccessRateReport":
                                                            byCmdID == e_MMeNetworkConflictReport?"e_MMeNetworkConflictReport":
                                                            byCmdID == e_MMeZeroCrossNTBCollectInd?"e_MMeZeroCrossNTBCollectInd":
                                                            byCmdID == e_MMeZeroCrossNTBReport?"e_MMeZeroCrossNTBReport":
                                                            byCmdID == e_MMeRouteRequest?"e_MMeRouteRequest":
                                                            byCmdID == e_MMeRouteReply?"e_MMeRouteReply":
                                                            byCmdID == e_MMeRouteError?"e_MMeRouteError":
                                                            byCmdID == e_MMeRouteAck?"e_MMeRouteAck":
                                                            byCmdID == e_MMeLinkConfirmRequest?"e_MMeLinkConfirmRequest":
                                                            byCmdID == e_MMeLinkConfirmResponse?"e_MMeLinkConfirmResponse":
                                                            byCmdID == e_MMeRfChannelConfilictReport?"e_MMeRfChannelConfilictReport":
                                                            "UNKOOWN");
                                                            */
            net_printf("net r:%04x\n",byCmdID);
            NetMgmMsgArray[i].Func(buf, pld, len);
        }
    }

    return;
}
/**
 * @brief SetBitsValue  ����input��bitNumλ���ݵ�output ��startbit����
 * @param output        Ŀ����������
 * @param startbit      ������ʼbit
 * @param bitNum        ���ݳ��ȣ� ��λ��bit
 * @param input         ԭʼ����
 * @return
 */
// U32 SetBitsValue(U32 *pOutput, U8 startbit, U8 bitNum, U32 input)
// {
//     printf_s("startbit:%d,bitNum:%d\n", startbit, bitNum);
//     U32 mask;
//     U32 dest = 0;
//     U8  pos=startbit+1-bitNum;
//     printf_s("pos:%d\n", pos);
//     mask=~(~0<<bitNum) << pos ;
//     printf_s("mask:%08x\n", mask);
//     dest=dest & ~mask;
//     printf_s("dest:%08x\n", dest);
//     input=input<<pos & mask;
//     printf_s("input:%08x\n", input);
//     dest=dest|input;
//     printf_s("dest:%08x\n", dest);

//     if(pOutput)
//     {
//         *pOutput |= dest;
//     }

//     return dest;
// }
/**
 * @brief setbits
 * @param x
 * @param p
 * @param n
 * @param y
 */
//unsigned setbits(unsigned x, int p, int n, unsigned y)
//{
//    return ~(~(~0 << n) << (p + 1 - n)) & x | ((~(~0 << n) & y) << (p + 1 - n));
//}


/***************�������߷����б���ӿ�************************/
U16 SetRfDiscStaInfo(U8 *PNodeInfo)
{

    RF_DISC_STAINFO_t *pStaInfo = (RF_DISC_STAINFO_t *)(PNodeInfo+2);

    PNodeInfo[0]              = e_RF_DISCV_STAINFO;
    PNodeInfo[1]              = sizeof(RF_DISC_STAINFO_t);

    __memcpy(pStaInfo->CCOMAC, GetCCOAddr(), 6);
    pStaInfo->ProxyTEI              = GetProxy();
    pStaInfo->NodeType              = GetNodeType();
    pStaInfo->NodeLevel             = nnib.NodeLevel;
    pStaInfo->RfCount               = nnib.RfCount;
    pStaInfo->ProxyUpRcvRate        = nnib.ProxyNodeUplinkRatio;
    pStaInfo->ProxyDownRcvRate      = nnib.ProxyNodeDownlinkRatio;
    pStaInfo->MixRcvRate            = 0;
    pStaInfo->DiscoverPeriod        = nnib.RfDiscoverPeriod;
    pStaInfo->RfRcvRateOldPeriod    = nnib.RfRcvRateOldPeriod;

    return (2+sizeof(RF_DISC_STAINFO_t));
}
typedef struct{
    U32 tei      :12;
    U32 rcvRate  :8;
    U32 snr      :6;
    U32 rssi     :6;
}__PACKED DISC_GROUP_INFO_NOMAP_7_t;

/**
 * @brief GetBitsValue  ����һ������bit������
 * @param input         ��������
 * @param startbit      ��ʼbitλ
 * @param bitlen        ���ݳ��ȡ���λ��bit
 * @return
 */
static U32 GetBitsValue(U32 input, U8 startbit, U8 bitlen)
{
    U32 k = 1;
    U32 sum  = 0;
    U8 i;
    for(i = 0; i < bitlen; i++)
    {
        sum += k;
        k *= 2;
    }

    return ((input >> startbit) & sum);
}
/**
 * @brief Set the Bits Value object
 * 
 * @param output 
 * @param startbit 
 * @param bitlen 
 * @param value 
 * @return U32 
 */
static U32 SetBitsValue(U32 *output, U8 startbit, U8 bitlen, U32 value)
{
    U32 k = 0;
    U8 i= 0;
    // printf_s("st:%d,len:%d,value:%d,output:%p, %08x\n", startbit,bitlen,value, output, *output);
    if(NULL == output || (startbit + bitlen) > 31)
    {
        printf_s("err\n");
        return 0;
    }    
    U32 data = *output;
    
    for(i = 0; i < bitlen; i++)
    {
        k |= 1<<i;
    }

    data  |= (value & k) << startbit;
    __memcpy(output, &data, 4);

    // printf_s("out:%08x\n", *output);

    return data;
}
/**
 * @brief SetRfDiscNebInfoNoMap
 * @param groupType                 �����б��͹̶�ʹ����Ϣ�������Ϊ7
 * @param pNbInfo
 * @return
 */
U32 SetRfDiscNebInfoNoMap(U8 groupType, U8 *pNbInfo)
{
//    RF_DISC_NBINFOTYPELEN_t mRfDisNbInfo = g_RfNbInfoTypeLenGroup[groupType];

    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable=NULL;


    //��Ϣ��Ԫ���ͣ� 0-6bit
    pNbInfo[0] = e_RF_DISCV_NBCHLINFO_NOBITMAP;
    //��Ϣ��Ԫ��������  bit7    �������Ͳ��������ֽ�
    pNbInfo[0] |= 1 << 7;
    //��Ϣ��Ԫ����
    U16 *pInfoLen = (U16 *)&pNbInfo[1];
    //��Ϣ�������
    pNbInfo[3] = groupType & 0x0F;      //�̶��ŵ��������Ϊ7
    *pInfoLen += 1;                     //һ���ֽ��ŵ��������

    U8 *pData =  NULL;
//    U8 startBit;




    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
//        startBit = 0;
        pData = pNbInfo + *pInfoLen + 3;
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->HrfInfo.DownRcvRate == 0 || DiscvryTable->SNID != GetSNID())
        {
            //�ھӽڵ������ͨ�ųɹ���Ϊ0�����ٷ������ھӽڵ��ͨ����
            continue;
        }

#if 0

        //���TEI��Ϣ
        if(mRfDisNbInfo.TEILen)
        {
            SetBitsValue((U32 *)pData, startBit, mRfDisNbInfo.TEILen, DiscvryTable->NodeTEI);
            startBit += mRfDisNbInfo.TEILen;
        }
        //��������Ϣ
        if(mRfDisNbInfo.RcvRateLen)
        {
            SetBitsValue((U32 *)pData, startBit, mRfDisNbInfo.RcvRateLen, DiscvryTable->HrfInfo.DownRcvRate);
            startBit += mRfDisNbInfo.RcvRateLen;
        }
        //ƽ�������
        if(mRfDisNbInfo.SNRAvgLen)
        {
            SetBitsValue((U32 *)pData, startBit, mRfDisNbInfo.SNRAvgLen, DiscvryTable->HrfInfo.DownSnr);
            startBit += mRfDisNbInfo.RcvRateLen;
        }
        //�ź�ǿ��
        if(mRfDisNbInfo.RSSIAvgLen)
        {
            SetBitsValue((U32 *)pData, startBit, mRfDisNbInfo.RSSIAvgLen, DiscvryTable->HrfInfo.DownRssi);
//            startBit += mRfDisNbInfo.RcvRateLen;
        }
#else
        DISC_GROUP_INFO_NOMAP_7_t *pInfo = (DISC_GROUP_INFO_NOMAP_7_t *)pData;
        pInfo->tei    = DiscvryTable->NodeTEI;
        pInfo->rcvRate= DiscvryTable->HrfInfo.DownRcvRate;
        pInfo->snr    = DiscvryTable->HrfInfo.DownSnr;
        pInfo->rssi   = DiscvryTable->HrfInfo.DownRssi;
#endif
        *pInfoLen += 4; //Ĭ��ʹ���ŵ��������7���̶���Ϣ����Ϊ�ĸ��ֽ�

        if(*pInfoLen >= 476)
            break;
    }

//    dump_level_buf(DEBUG_MDL, pNbInfo, 3 + *pInfoLen);

    if(*pInfoLen == 1)
        return 0;
    else
        return (3 + *pInfoLen);

}
/**
 * @brief Set the Rf Disc Neb Info Map object       ��дλͼ�汾�ھӽڵ���Ϣ
 * 
 * @param groupType                                 �ŵ���Ϣ�������
 * @param pNbInfo                                   ���ݽ��յ�ַ
 * @return U32 
 */
static U32 SetRfDiscNebInfoMap(U8 groupType, U8 *pNbInfo)
{
    list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable=NULL;
    U16 offset = 0;

    static U16 offsetIndex = 0;
    U16 Index = 0;

    if(groupType > 10 || NULL == pNbInfo)
    {
        return 0;
    }

    RF_DISC_NBINFOTYPELEN_MAP_t RfNbInfoGroup = g_RfNbInfoTypeLenGroupMap[groupType];

    U16 startTei = 0;
    U8 *pStartTei;
    U16 mapLen = 0;
    U16 NodeNum  = 0;

    U16 InfoLen = 0;

    
    U16 InfoBitLen = 0;
    U8 startbit = 0;
    U16 InfoByteOffset = 0;
    U32 *pData = NULL;

    U8  *pInfo = zc_malloc_mem(520, "rfinfo", MEM_TIME_OUT);

    U8 Byteoffset, Bitoffset;

    //��Ϣ��Ԫ����
    pNbInfo[offset] = e_RF_DISCV_NBCHLINFO_BITMAP;
    //��Ϣ��Ԫ��������  bit7    �������Ͳ��������ֽ�
    pNbInfo[offset++] |= 1 << 7;
    //��Ϣ��Ԫ����
    U16 *pInfoLen = (U16 *)&pNbInfo[offset];
    offset+=2;

    //�ŵ���Ϣ�������
    pNbInfo[offset++] = groupType & 0x0f;
    // *pInfoLen += 1;

     //λͼ��ʼTEI
    pStartTei = &pNbInfo[offset];
    offset += 2;
    // *pInfoLen += 2;
    //λͼ��С
    U8 *pMapLen  = (U8 *)&pNbInfo[offset++];
    // *pInfoLen += 1;
    //λͼ�� �������Ѿ���д
    // __memcpy(pNbInfo + offset, pMap, mapLen);
    
    U8  *pMap = pNbInfo + offset;

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        if(DiscvryTable->HrfInfo.DownRcvRate == 0 || DiscvryTable->SNID != GetSNID())
        {
            //�ھӽڵ������ͨ�ųɹ���Ϊ0�����ٷ������ھӽڵ��ͨ����
            continue;
        }

        Index++;
        if(Index <  offsetIndex)
        {
            continue;
        }
        offsetIndex = 0;
        
        NodeNum++;

        if(startTei == 0)
        {
            startTei = DiscvryTable->NodeTEI;
            __memcpy(pStartTei, &startTei, 2);
            // printf_s("startTei:%03x\n", startTei);
        }


        U16 tmpTEI = DiscvryTable->NodeTEI - startTei;
        Byteoffset = tmpTEI / 8;
        Bitoffset = tmpTEI % 8;
        Bitoffset = 1 << Bitoffset;
        pMap[Byteoffset] = pMap[Byteoffset] | Bitoffset;
        
        //��ȡλͼ��С
        mapLen = MAX(mapLen, Byteoffset+1);

        InfoByteOffset = InfoBitLen/8;

        if(InfoBitLen%8 == 0)
        {
            startbit = 0;
        }
        else
        {
            startbit = InfoBitLen%8;
        }

        pData = (U32 *)(pInfo + InfoByteOffset);
        U32 data = 0;
        __memcpy(&data, pData, 4);
        // pInfo[InfoByteOffset] = DiscvryTable->HrfInfo.DownRcvRate;

        //����������Ϣ
        if(RfNbInfoGroup.RcvRateLen)
        {
            SetBitsValue(&data, startbit, RfNbInfoGroup.RcvRateLen, DiscvryTable->HrfInfo.DownRcvRate);
            startbit += RfNbInfoGroup.RcvRateLen;
        }
        //���ƽ���������Ϣ
        if(RfNbInfoGroup.SNRAvgLen)
        {
            SetBitsValue(&data, startbit, RfNbInfoGroup.SNRAvgLen, DiscvryTable->HrfInfo.DownSnr);
            startbit += RfNbInfoGroup.SNRAvgLen; 
        }
        //����ź�ǿ����Ϣ
        if(RfNbInfoGroup.RSSIAvgLen)
        {
            SetBitsValue(&data, startbit, RfNbInfoGroup.RSSIAvgLen, DiscvryTable->HrfInfo.DownRssi);
            startbit += RfNbInfoGroup.RSSIAvgLen; 
        }

        __memcpy(pData, &data, 4);

        InfoBitLen = NodeNum * RfNbInfoGroup.InfoLen;
        //�����ھӽڵ��ŵ���Ϣ����
        if(InfoBitLen%8)
        {
            InfoLen  =  InfoBitLen/8 + 1;
        }
        else
        {
            InfoLen  =  InfoBitLen/8;
        }

        //��������
        if((mapLen + InfoLen + 4) >460)
        {
            offsetIndex = Index;
            break;
        }

    }


    net_printf("st:%d,NdNum:%d\n", offsetIndex ,NodeNum);

    *pMapLen = mapLen;

    // printf_s("map:%d\n", mapLen);

    offset += mapLen;
    // *pInfoLen += mapLen;
    //�ھӽڵ���Ϣ
    __memcpy(pNbInfo + offset, pInfo, InfoLen);
    offset += InfoLen;
    // printf_s("InfoLen:%d\n", InfoLen);
    // *pInfoLen += InfoLen;

    *pInfoLen  = offset - 3;;
    
    // printf_s("offset:%d\n", offset);

    zc_free_mem(pInfo);

    if(InfoLen)
    {
        return offset;
    }
    else
    {
        return 0;
    }

}


void SendMMeRFDiscoverNodeList(void)
{
    static U8 RfDisListSeq = 0;
    net_printf("SendMMeRFDiscoverNodeList\n");

//    U8                              *pBMPheader;
//    U8								*pListtail;
    U8                              linkid = 0;
//    U16 							maxTei, i, index;

//    U16								InfoLen = 0;
    U16								/*i, j,TEI,*/ msduLen;
    U16 macLen;
    MAC_RF_HEADER_t             *pMacLongHeader;
    RF_DISCOVER_LIST_t              *pNodeList;
    U8                            *pStaInfo;
    U8                            *pNbInfo;

    U16 ListInfoLen = 0;

    pMacLongHeader = (MAC_RF_HEADER_t*)zc_malloc_mem(520,"RfDisList",MEM_TIME_OUT);
    if(NULL == pMacLongHeader)
    {
        return;
    }
    pNodeList = (RF_DISCOVER_LIST_t *)pMacLongHeader->Msdu;

    pNodeList->DiscoverSeq = RfDisListSeq++;
    __memcpy(pNodeList->MacAddr, GetNnibMacAddr(), 6);

    //վ����Ϣ
    pStaInfo = &pNodeList->ListInfo[ListInfoLen];
    ListInfoLen += SetRfDiscStaInfo(pStaInfo);

    pNbInfo = &pNodeList->ListInfo[ListInfoLen];

    //�ھӽڵ���Ϣ��λͼ��
    // ListInfoLen += SetRfDiscNebInfoNoMap(7, pNbInfo);

    //�ھӽڵ���Ϣλͼ��
    ListInfoLen +=  SetRfDiscNebInfoMap(0, pNbInfo);


    msduLen = sizeof(RF_DISCOVER_LIST_t) + ListInfoLen;

    CreatMMsgMacHeaderForSig(pMacLongHeader,e_RF_DISCOVER_LIST, msduLen);



    macLen = msduLen + sizeof(MAC_RF_HEADER_t)+CRCLENGTH;

    if(macLen > 520)
    {
        printf_s("rf discLen err:%d\n", macLen);
    }

    MacDataRequstRf((MAC_PUBLIC_HEADER_t*)pMacLongHeader, macLen, BROADCAST_TEI,
                                 e_LOCAL_BROADCAST_FREAM_NOACK, e_MMeDiscoverNodeList, linkid, e_RF_DISCOVER_LIST, 0, 0, e_MainRoute);


    zc_free_mem(pMacLongHeader);

}
/**
 * @brief ProcessRfDiscoverRcvIndex  ����mac��ַ�����߷����б���š������ھӱ����б����λͼ���������н�����
 * @param MacAddr                    �������߷����б�ڵ�mac��ַ  û�и���mac��ַ�Ľڵ㲻���������
 * @param DiscoverSeq                ���߷����б�ͳ����š�
 * @return
 */
static void ProcessRfDiscoverRcvIndex(U32 snid, U8 *MacAddr, U8 DiscoverSeq, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry)
{
    U8 RcvIndex = 0;
    RcvIndex = DiscoverSeq % RF_RCVMAP;

    if(NULL == pEntry)
    {
        return;
    }

    //ˢ�����н������ϻ����ڼ���
    pEntry->HrfInfo.DicvPeriodCntDown = nnib.RfDiscoverPeriod;
    //ˢ�����гɹ����ϻ�����
    pEntry->HrfInfo.RcvRateOldPeriodCntDown = nnib.RfRcvRateOldPeriod;
    
    //net_printf("up OldCnt:%d\n", nnib.RfDiscoverPeriod);

    if(snid == pEntry->SNID && 0 == memcmp(MacAddr, pEntry->MacAddr, 6))
    {
        //������MAP��Ӧλ��1
        bitmap_set_bit(pEntry->HrfInfo.RcvMap, RcvIndex);

        //net_printf("RcvIndex:%d, pEntry->HrfInfo.UpdateIndex:%d\n", RcvIndex, pEntry->HrfInfo.UpdateIndex);
        //dump_buf(pEntry->HrfInfo.RcvMap, sizeof(pEntry->HrfInfo.RcvMap));

        if(RcvIndex == (pEntry->HrfInfo.UpdateIndex + 1) % RF_RCVMAP)   //��������
        {
            //net_printf("1\n");
            pEntry->HrfInfo.UpdateIndex = RcvIndex;
        }
        else if(RcvIndex > (pEntry->HrfInfo.UpdateIndex + 1) )           //���ճ��ֶ�ʧ, ��ֹ��ת
        {
            //net_printf("2\n");
            U8 i;
            for(i = pEntry->HrfInfo.UpdateIndex + 1; i < RcvIndex; i++)
            {
                bitmap_clr_bit(pEntry->HrfInfo.RcvMap, i);
            }
            pEntry->HrfInfo.UpdateIndex = RcvIndex;
        }
        //�������򣬻����ӳ٣���ǰ��¼λͼΪ0�������ƶ��������1 
        else if((RcvIndex <= pEntry->HrfInfo.UpdateIndex) && !(bitmap_in_bit(pEntry->HrfInfo.RcvMap, pEntry->HrfInfo.UpdateIndex)))
        {
            //net_printf("3\n");
            U8 i;
            for(i = pEntry->HrfInfo.UpdateIndex; i >= RcvIndex; i--)
            {
                if(bitmap_in_bit(pEntry->HrfInfo.RcvMap, i))
                {
                    pEntry->HrfInfo.UpdateIndex = i;
                    break;
                }
            }
        }


        //���������
        pEntry->HrfInfo.DownRcvRate = (bitmap_get_one_nr(pEntry->HrfInfo.RcvMap, sizeof(pEntry->HrfInfo.RcvMap)) * 100) / RF_RCVMAP;
        //net_printf("rf DownRcvRate : %d\n", pEntry->HrfInfo.DownRcvRate);
        //dump_buf(pEntry->HrfInfo.RcvMap, sizeof(pEntry->HrfInfo.RcvMap));
        if(pEntry->Relation == e_PROXY && nnib.LinkType == e_RF_Link) //���������ͨ�ųɹ���
        {
            nnib.ProxyNodeDownlinkRatio = pEntry->HrfInfo.DownRcvRate;
        }
    }
}

/**
 * @brief ProcessRfDiscoverRouteInfo  �������߷����б�վ��·����Ϣ
 * @param pData                       վ��·����Ϣ��������
 * @param pDataLen                    վ��·����Ϣ���ݳ���
 * @return
 */
static void ProcessRfDiscoverRouteInfo(U8 *pData, U16 pDataLen)
{

}

/**
 * @brief ProcessRfDiscoverNbChlInfo  �������н����ʣ��ź�ǿ�Ⱥ��������Ϣ���ϻ�����
 * @param pData                       �ھӽڵ��ŵ���Ϣ��λͼ����������
 * @param pDataLen                    �ھӽڵ��ŵ���Ϣ��λͼ�����ݳ���
 * @return
 */
typedef struct{
    U32 tei1      :12;
    U32 rcvRate1  :8;
    U32 tei2      :12;

    U8  rcvRate2  :8;
}__PACKED DISC_NBINFO_NOMAP_TYPE0_t;

typedef struct{
    U32 tei1      :12;
    U32 snr1      :4;
    U32 rssi1     :4;
    U32 tei2      :12;

    U8  snr2      :4;
    U8  rssi2     :4;
}__PACKED DISC_NBINFO_NOMAP_TYPE8_t;

typedef struct{
    union{
        DISC_NBINFO_NOMAP_TYPE0_t dataType0;
        DISC_NBINFO_NOMAP_TYPE0_t dataType8;
    };
}__PACKED DISC_GROUP_INFO_NOMAP_BITLEN20_t;


static void ProcessRfDiscoverNbChlInfo(U8 *pData, U16 pDataLen, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry)
{
//    dump_zc(0,"rfdiscover list info ", DEBUG_MDL, pData, pDataLen);
    U8 groupType = pData[0] & 0x0f;
    pData += 1;

    if(groupType > RF_NBINFOTYPELEN_GROUP_COUNT)
    {
        net_printf("groupType : %d Err!", groupType);
        return;
    }

//    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry = GetNeighborByTEI(NodeTei, e_RF_Link);
    if(!pEntry)
    {
        net_printf("is not in neighbor!\n");
        return;
    }

//    net_printf("groupType: %d, info len:%d\n", groupType, pDataLen);
//    dump_level_buf(DEBUG_MDL, pData, pDataLen-1);


    U16 offset = 0;
    U16 Nodetei = 0;
    U8 RcvRate = 0;
//    S8 snr;
//    S8 rssi;

    U8 StartBit = 0;

    U32 data;

    RF_DISC_NBINFOTYPELEN_t mRfDisNbInfo = g_RfNbInfoTypeLenGroup[groupType];
//    net_printf("len: %d,%d,%d,%d,%d\n",    mRfDisNbInfo.TEILen
//                                        ,mRfDisNbInfo.RcvRateLen
//                                        ,mRfDisNbInfo.SNRAvgLen
//                                        ,mRfDisNbInfo.RSSIAvgLen
//                                        ,mRfDisNbInfo.InfoLen);

    do{
        if(pDataLen <= 1 || (pDataLen-1) < (mRfDisNbInfo.InfoLen / 8))
        {
            net_printf("NbInfolen err %d\n", pDataLen);
            break;
        }

#if 1
        __memcpy(&data, pData + offset, MIN(4, pDataLen - offset - 1));

//        net_printf("offset:%d, startBit:%d, data:%08x\n", offset, StartBit, data);

        //��ȡ�ھӽڵ�TEI
        if(mRfDisNbInfo.TEILen)
        {
            Nodetei = GetBitsValue(data, StartBit, mRfDisNbInfo.TEILen);
            StartBit += mRfDisNbInfo.TEILen;
        }
//        net_printf("tei:%03x\n", Nodetei);
        if(Nodetei == GetTEI())
        {

            //��ȡ�ھӽڵ������
            if(mRfDisNbInfo.RcvRateLen)
            {
                RcvRate = GetBitsValue(data, StartBit, mRfDisNbInfo.RcvRateLen);
                StartBit += mRfDisNbInfo.RcvRateLen;

                if(mRfDisNbInfo.RcvRateLen == 6)        //6bit������Ϊʵ�ʽ����ʵ�1/2
                    RcvRate *= 2;


//                net_printf("RcvRate:%d\n", RcvRate);

                //�������н�����
                pEntry->HrfInfo.UpRcvRate = RcvRate;

                #if defined(STATIC_MASTER)
                if(pEntry->Relation == e_PROXY && nnib.LinkType == e_RF_Link)
                {
                    nnib.ProxyNodeUplinkRatio = pEntry->HrfInfo.UpRcvRate;
                    //net_printf("ProxyUpRatio:%d\n", nnib.ProxyNodeUplinkRatio);
                }
                #endif
                
                //ˢ�����гɹ����ϻ�����
                // pEntry->HrfInfo.RcvRateOldPeriodCntDown = pEntry->HrfInfo.RfRcvRateOldPeriod;

            }

            //��ȡ�ھ�SNR����ֵ
            if(mRfDisNbInfo.SNRAvgLen)
            {
                //                    snr = GetBitsValue(data, StartBit, mRfDisNbInfo.SNRAvgLen);
                StartBit += mRfDisNbInfo.SNRAvgLen;
            }

            //��ȡ�ź�ǿ������ֵ
            if(mRfDisNbInfo.RSSIAvgLen)
            {
                //                    rssi = GetBitsValue(data, StartBit, mRfDisNbInfo.RSSIAvgLen);
                StartBit += mRfDisNbInfo.RSSIAvgLen;
            }
        }
        else
        {
            StartBit += (mRfDisNbInfo.InfoLen - mRfDisNbInfo.TEILen); //Ϊ��������ƫ����׼��
        }

        if((mRfDisNbInfo.InfoLen % 8) == 0) //��Ϣ��ϳ����������ֽڣ�����ʵ���ֽڳ���ƫ��
        {
            offset += (mRfDisNbInfo.InfoLen / 8);
            StartBit = 0;
        }
        else
        {
            if((StartBit % 8) == 0)     //��һ�����ݴ��ֽ�0bit��ʼ
            {
                offset += ((mRfDisNbInfo.InfoLen / 8) + 1);
                StartBit = 0;
            }
            else
            {
                offset += ((mRfDisNbInfo.InfoLen / 8));
                StartBit = StartBit % 8;
            }
        }
        
#else

        StartBit = 0;
        if((mRfDisNbInfo.InfoLen % 8) == 0)       //��Ϣ��ϳ����������ֽڣ�����ʵ���ֽڳ��ȱ���
        {
//            U32 data = *((U32*)(pData + offset));

//            dump_level_buf(DEBUG_MDL, pData+offset, 4);
            __memcpy(&data, pData+offset, 4);
            //��ȡ�ھӽڵ�TEI
            if(mRfDisNbInfo.TEILen)
            {
                Nodetei = GetBitsValue(data, StartBit, mRfDisNbInfo.TEILen);
                StartBit += mRfDisNbInfo.TEILen;
            }
//            net_printf("tei:%03x\n", Nodetei);
            if(Nodetei == GetTEI())
            {

                //��ȡ�ھӽڵ������
                if(mRfDisNbInfo.RcvRateLen)
                {
                    RcvRate = GetBitsValue(data, StartBit, mRfDisNbInfo.RcvRateLen);
                    StartBit += mRfDisNbInfo.RcvRateLen;

                    if(mRfDisNbInfo.RcvRateLen == 6)        //6bit������Ϊʵ�ʽ����ʵ�1/2
                        RcvRate *= 2;


//                    net_printf("RcvRate:%d\n", RcvRate);

                    //�������н�����
                    pEntry->HrfInfo.UpRcvRate = RcvRate;
                    //ˢ�����гɹ����ϻ�����
                    pEntry->HrfInfo.RcvRateOldPeriodCntDown = pEntry->HrfInfo.RfRcvRateOldPeriod;

                }

                //��ȡ�ھ�SNR����ֵ
                if(mRfDisNbInfo.SNRAvgLen)
                {
//                    snr = GetBitsValue(data, StartBit, mRfDisNbInfo.SNRAvgLen);
                    StartBit += mRfDisNbInfo.SNRAvgLen;
                }

                //��ȡ�ź�ǿ������ֵ
                if(mRfDisNbInfo.RSSIAvgLen)
                {
//                    rssi = GetBitsValue(data, StartBit, mRfDisNbInfo.RSSIAvgLen);
                    StartBit += mRfDisNbInfo.RSSIAvgLen;
                }
            }

            offset += (mRfDisNbInfo.InfoLen / 8);
        }
        else                                    //��Ϣ��ϳ���Ϊ20bit�� һ��ƫ�������ڵ���Ϣ���ȡ�
        {


            DISC_GROUP_INFO_NOMAP_BITLEN20_t *pNodeData = (DISC_GROUP_INFO_NOMAP_BITLEN20_t *)(pData + offset);
            if(groupType == 0 || groupType == 2)
            {
                if(pNodeData->dataType0.tei1 == GetTEI())
                {
                    if(mRfDisNbInfo.RcvRateLen)     //TEI + ������
                    {
                        pEntry->HrfInfo.UpRcvRate = pNodeData->dataType0.rcvRate1;
                    }
                    else                            //TEI + �ź�ǿ��
                    {

                    }
                }

                if((offset + 5) > pDataLen - 1)     //��������ֻ��һ���ڵ㡣���ݳ��Ȳ��������ڵ���Ϣ
                    break;

                if(pNodeData->dataType0.tei2 == GetTEI())
                {
                    pEntry->HrfInfo.UpRcvRate = pNodeData->dataType0.rcvRate2;
                }

            }
            else if(groupType == 8)
            {
                if(pNodeData->dataType8.tei1 == GetTEI())
                {

                }

                if((offset + 5) > pDataLen - 1)
                    break;

                if(pNodeData->dataType8.tei2 == GetTEI())
                {

                }
            }

            offset += (mRfDisNbInfo.InfoLen * 2) / 8;

        }

        if(pEntry->Relation == e_PROXY && nnib.LinkType == e_RF_Link)
        {
            nnib.ProxyNodeUplinkRatio = pEntry->HrfInfo.UpRcvRate;
        }
#endif

        if(offset == 0)     //��ѭ���쳣��ӡ
            net_printf("offset:%d\n", offset);

    }while(offset <= (pDataLen -1 - (mRfDisNbInfo.InfoLen/8)));

}
/**
 * @brief getRfDiscNbInfoMap        �������߷����б��ھ���Ϣλͼ��
 * @param StartTei                  λͼ��ʼTEI
 * @param pData                     �����ھ��ŵ���Ϣ������ʼ��ַ
 * @param bitMapNum                 λͼ��С
 * @param mRfDisNbInfo              ��Ϣ�������
 * @param pBitMap                   TEIλͼ������ʼ��ַ
 * @param pEntry                    �ھ�����ڵ���Ϣ
 */
void getRfDiscNbInfoMap(U16 StartTei, U8 *pData, U8 bitMapNum, RF_DISC_NBINFOTYPELEN_MAP_t mRfDisNbInfo, U8 *pBitMap, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry, U16 DataLen)
{
    U16 i;
    U8 j;
    U16 NumOffset = 0;

    U8 RcvRate;
    U32 data;

    U8 StartBit = 0;
//    S8 snr;
//    S8 rssi;

    U16 InfoByteOffset = 0;

    for( i =0; i < bitMapNum; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(pBitMap[i] & (1<<j))
            {

//                net_printf("tei :%03x\n", StartTei + (i * 8 + j));

                if((StartTei + (i * 8 + j)) == GetTEI())
                {
                    if(mRfDisNbInfo.InfoLen == 0)
                    {
                        net_printf("mRfDisNbInfo2 %d,InfoLen=0 Err!\n",mRfDisNbInfo.InfoLen);
                        return;
                    }

                    InfoByteOffset = ((NumOffset) * mRfDisNbInfo.InfoLen)/8;

//                    net_printf("InfoByteOffset: %d\n", InfoByteOffset);
                    __memcpy(&data, pData + InfoByteOffset, MIN(4, DataLen - InfoByteOffset));

                    if((mRfDisNbInfo.InfoLen % 8) == 0 )
                    {
                        StartBit = 0;
                    }
                    else
                    {
                        StartBit = (((NumOffset) * mRfDisNbInfo.InfoLen) % 8);
                    }


//                    net_printf("data : %08x, StartBit:%d\n", data, StartBit);

                    if(mRfDisNbInfo.RcvRateLen)
                    {
                        RcvRate = GetBitsValue(data, StartBit, mRfDisNbInfo.RcvRateLen);
                        StartBit += mRfDisNbInfo.RcvRateLen;


//                        net_printf("RcvRateLen:%d, RcvRate:%d\n", mRfDisNbInfo.RcvRateLen, RcvRate);

                        pEntry->HrfInfo.UpRcvRate = RcvRate;
                        #if defined(STATIC_MASTER)
                        if(pEntry->Relation == e_PROXY && nnib.LinkType == e_RF_Link)
                        {
                            nnib.ProxyNodeUplinkRatio = pEntry->HrfInfo.UpRcvRate;
                            //net_printf("ProxyUpRatio from map:%d\n", nnib.ProxyNodeUplinkRatio);
                        }
                        #endif

                        //ˢ�����гɹ����ϻ�����
                        // pEntry->HrfInfo.RcvRateOldPeriodCntDown = pEntry->HrfInfo.RfRcvRateOldPeriod;

                    }
                    if(mRfDisNbInfo.SNRAvgLen)
                    {
                        //                            snr = GetBitsValue(data, StartBit, mRfDisNbInfo.RcvRateLen);
                        StartBit += mRfDisNbInfo.SNRAvgLen;
                    }
                    if(mRfDisNbInfo.RSSIAvgLen)
                    {
                        //                            rssi = GetBitsValue(data, StartBit, mRfDisNbInfo.RcvRateLen);
                    }
                }

                NumOffset++;        //�ڵ�����ƫ��

            }
        }
    }
}
/**
 * @brief ProcessRfDiscoverNbChlInfoMap   �������н����ʣ��ź�ǿ�Ⱥ��������Ϣ���ϻ�����
 * @param pData                           �ھӽڵ��ŵ���Ϣλͼ����������
 * @param pDataLen                        �ھӽڵ��ŵ���Ϣλͼ�����ݳ���
 * @return
 */

static void ProcessRfDiscoverNbChlInfoMap(U8 *pData, U16 pDataLen, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry)
{
    U8 groupType = pData[0] & 0x0f;
    pData += 1;

    if(groupType > RF_NBINFOTYPELEN_MAP_GROUP_COUNT)
    {
        net_printf("groupType : %d Err!", groupType);
        return;
    }

//    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry = GetNeighborByTEI(NodeTei, e_RF_Link);
    if(!pEntry)
    {
        net_printf("is not in neighbor!\n");
        return;
    }

    RF_DISC_NBINFOTYPELEN_MAP_t mRfDisNbInfo = g_RfNbInfoTypeLenGroupMap[groupType];


    U16 StartTei;
    U16 offset = 0;

    U16 bitMapNum = 0;
    U8 *pBitMap;
    U16 NodeNum = 0;
    U16 NbInfoLen = 0;

//    net_printf("groupType: %d, info len:%d\n", groupType, pDataLen);
//    dump_level_buf(DEBUG_MDL, pData, pDataLen-1);

    //����λͼ����������Ϣ
    do
    {
        //��ʼTEI
//        StartTei = (*(U16 *)pData) & BROADCAST_TEI;
        StartTei = GetWord(pData) & BROADCAST_TEI;
        offset += 2;

        //λͼ��С
        bitMapNum = pData[offset++];
        pBitMap = &pData[offset];
        offset += bitMapNum;
        //��ȡλͼ����Чtei����
        NodeNum = bitmap_get_one_nr(pBitMap, bitMapNum);
        if(mRfDisNbInfo.InfoLen == 0 || NodeNum == 0)
        {
            net_printf("mRfDisNbInfo InfoLen=%d,NodeNum=%d,Err!\n",mRfDisNbInfo.InfoLen,NodeNum);
            return;
        }
        //�ھӽڵ���Ϣ
        NbInfoLen = (NodeNum * mRfDisNbInfo.InfoLen) / 8;
        if((NodeNum * mRfDisNbInfo.InfoLen) % 8)
            NbInfoLen += 1;

//        net_printf("NbInfolen : %d; ", NbInfoLen);
//        dump_level_buf(DEBUG_MDL, pData + offset, (NodeNum * mRfDisNbInfo.InfoLen) / 8);

        getRfDiscNbInfoMap(StartTei, pData + offset, bitMapNum, mRfDisNbInfo, pBitMap, pEntry, NbInfoLen);

        offset += NbInfoLen;


    }while(offset < (pDataLen - 4));


}
/**
 * @brief ProcessMMeRFDiscoverNodeList      ���߷����б���ӿ�
 * @param buf
 * @param pld
 * @param len
 * @param work
 */
void ProcessMMeRFDiscoverNodeList(mbuf_t *buf, U8 *pld, U16 len, work_t *work)
{
    MAC_RF_HEADER_t *PMacHeader = (MAC_RF_HEADER_t *)pld;
    RF_DISCOVER_LIST_t *PData = (RF_DISCOVER_LIST_t *)(pld + sizeof(MAC_RF_HEADER_t));
    U16 PDataLen = PMacHeader->MsduLen;
    U8 *pListInfo = PData->ListInfo;
    U16 InfoLen = 0;
    U16 offset = 0;

    U8 InfoType;
    U8 InfoLenType;

    U16 NodeTei = get_rf_frame_stei(buf->fi.head);



    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *pEntry = GetNeighborByTEI(NodeTei, e_RF_Link);
    if(pEntry)
    {
        if(pEntry->HrfInfo.DiscoverSeq == PData->DiscoverSeq)
            return;

        pEntry->HrfInfo.DiscoverSeq = PData->DiscoverSeq;

    }

    RepairNextHopAddr(NodeTei, e_RF_Link);

    net_printf("seq:%d,tei:%03x, mac:", PData->DiscoverSeq, NodeTei);
    dump_level_buf(DEBUG_MDL, PData->MacAddr, 6);

#if defined(STATIC_NODE)

    if(AccessListSwitch ==TRUE)
   {
        if(!checkAccessControlList(PData->MacAddr, 1) )
       {
           net_printf("----------Rfdisc checkAccCtrlList!:%02x %02x %02x %02x %02x %02x\n",
                      PData->MacAddr[0],
                      PData->MacAddr[1],
                      PData->MacAddr[2],
                      PData->MacAddr[3],
                      PData->MacAddr[4],
                      PData->MacAddr[5]);
//           net_printf("----------beacon checkAccessControlList!:%02x %02x %02x %02x %02x %02x\n",
//               AccessControlListrow.AccessControlList[0].MacAddr[0],
//               AccessControlListrow.AccessControlList[0].MacAddr[1],
//               AccessControlListrow.AccessControlList[0].MacAddr[2],
//               AccessControlListrow.AccessControlList[0].MacAddr[3],
//               AccessControlListrow.AccessControlList[0].MacAddr[4],
//               AccessControlListrow.AccessControlList[0].MacAddr[5],
//               AccessControlListrow.AccessControlList[0].PermitOrDeny);
           return;
       }

   }
#endif

   if(NodeTei == GetProxy())
   {
       nnib.RfRecvDisListTime = nnib.RoutePeriodTime * 4;
       net_printf("Rcv Rf PrxyDisc,tei:%03x,time:%d\n", NodeTei, nnib.RfRecvDisListTime);
   }

    do{
        InfoType = pListInfo[offset] & 0x7f;
        InfoLenType = pListInfo[offset++] >> 7 & 0x01;     //0:�����ֶ�һ���ֽڣ�1�������ֶ�2���ֽ�
        if(InfoLenType == 0)
        {
            InfoLen = pListInfo[offset++];
        }else
        {
            InfoLen = pListInfo[offset] | (pListInfo[offset + 1] << 8);
            offset += 2;
        }
        if(InfoLen == 0)
        {
            net_printf("Rf Dis offset %d,InfoLen=0 Err!\n",offset);
            continue;
        }
        switch(InfoType)
        {
        case e_RF_DISCV_STAINFO:
        {
            // net_printf("Process e_RF_DISCV_STAINFO\n");
            RF_DISC_STAINFO_t *pInfo = (RF_DISC_STAINFO_t *)&pListInfo[offset];
            // net_printf("tei:%03x,lv:%d,ntype:%d\n", NodeTei, pInfo->NodeLevel, pInfo->NodeType);
            UpdateRfNbInfo(buf->snid,PData->MacAddr, NodeTei, pInfo->NodeLevel, pInfo->NodeType, pInfo->ProxyTEI, 0, buf->rssi, buf->snr, 0, pInfo->RfCount, DT_SOF);
            
            if(pEntry)
            {
                pEntry->HrfInfo.RfDiscoverPeriod = pInfo->DiscoverPeriod;
                pEntry->HrfInfo.RfRcvRateOldPeriod = pInfo->RfRcvRateOldPeriod;
                UpDataRfNeighborRelation(pEntry, pInfo->ProxyTEI, pInfo->NodeLevel, pInfo->ProxyUpRcvRate, pInfo->ProxyDownRcvRate);
            }
            
            
            #if defined(STATIC_MASTER)
            if(NodeTei > CCO_TEI)
            {
                net_printf("status:%d, Lk:%d up:%d, down:%d\n"
                                , DeviceTEIList[NodeTei-2].NodeState
                                , DeviceTEIList[NodeTei-2].LinkType
                                , pInfo->ProxyUpRcvRate
                                , pInfo->ProxyDownRcvRate);
                                
                if(DeviceTEIList[NodeTei-2].NodeState != e_NODE_OUT_NET)//���ڸ��� ʱ��
                {
                    DeviceTEIList[NodeTei-2].DurationTime = nnib.RoutePeriodTime*2;//ONLINETIME;
                    DeviceTEIList[NodeTei-2].NodeState = e_NODE_ON_LINE;
                    if(DeviceTEIList[NodeTei-2].LinkType == e_RF_Link)
                    {
                        DeviceTEIList[NodeTei-2].UplinkSuccRatio = pInfo->ProxyUpRcvRate;
                        DeviceTEIList[NodeTei-2].DownlinkSuccRatio = pInfo->ProxyDownRcvRate;
                    }
                }
                else
                {
                    SendMMeDelayLeaveOfNum(PData->MacAddr, 1, 0, e_LEAVE_WAIT_SELF_OUTLINE);
                }
            }
            #elif defined(STATIC_NODE)
            if(NodeTei == GetProxy())
            {
//                nnib.NodeLevel = pInfo->NodeLevel + 1;
            }
            #endif

//            ProcessRfDiscoverStaInfo(pListInfo[offset], InfoLen);
        }
            break;
        case e_RF_DISCV_ROUTEINFO:
            // net_printf("Process e_RF_DISCV_ROUTEINFO\n");
            ProcessRfDiscoverRouteInfo(&pListInfo[offset], InfoLen);
            break;
        case e_RF_DISCV_NBCHLINFO_NOBITMAP:
            // net_printf("Process e_RF_DISCV_NBCHLINFO_NOBITMAP\n");
            ProcessRfDiscoverNbChlInfo(&pListInfo[offset], InfoLen, pEntry);
            break;
        case e_RF_DISCV_NBCHLINFO_BITMAP:
            // net_printf("Process e_RF_DISCV_NBCHLINFO_BITMAP\n");
            ProcessRfDiscoverNbChlInfoMap(&pListInfo[offset], InfoLen, pEntry);
            break;
        default:
            net_printf("Rf Dis InfoType Err!\n");
            return;
        }

        offset += InfoLen;

    }while(offset < (PDataLen - sizeof(RF_DISCOVER_LIST_t)));


    // printf_s("1\n");
    //TODO ʹ��TEI����ż������н�����
    {
        ProcessRfDiscoverRcvIndex(buf->snid, PData->MacAddr, PData->DiscoverSeq, pEntry);
    }
    // printf_s("2\n");


}

