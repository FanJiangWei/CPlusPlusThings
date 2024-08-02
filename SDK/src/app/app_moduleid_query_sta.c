#include "list.h"
#include "types.h"
#include "Trios.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
#include "app_moduleid_query.h"
#include "datalinkdebug.h"
#include "aps_interface.h"
#include "dl_mgm_msg.h"
#include "SchTask.h"
#include "app_dltpro.h"
#include "app_rdctrl.h"


#if defined(STATIC_NODE)

void rd_ctrl_pack_read_id_info(U8 *pdata, U8* pdatalen, U8 *addr, U8 *id, U8 idlen)
{
	U8 dataunit[30] = {0};
	U8 dataunitlen = 0;
	dataunitlen++;
	__memcpy(dataunit+dataunitlen, id, idlen);
	dataunitlen += idlen;

	//Add0x33ForData(dataunit, dataunitlen);
	dataunit[0] = g_ReadIdType;
	
	Packet645Frame(pdata, pdatalen, addr, 0x9f, dataunit, dataunitlen);
}

void rd_query_id_info_cfm(QUERY_IDINFO_CFM_t  * pQueryIdInfoCfm)
{
    QUERY_IDINFO_CFM_t  *rd_query_id_info_cfm = pQueryIdInfoCfm;
	U8 addr[6] = {0};
	U8 idlen = 0;
	U8 *rd_send_frame = zc_malloc_mem(100,"rd_send_frame",MEM_TIME_OUT);
	U8  rd_send_frame_len = 0;
	
    app_printf("rd_query_id_info_cfm\n");
	__memcpy(addr, rd_query_id_info_cfm->SrcMacAddr, MACADRDR_LENGTH);
	
	idlen = rd_query_id_info_cfm->Asdu[0];
	ChangeMacAddrOrder(addr);
	//app_printf("ID(%d): ",idlen);
	//dump_buf(rd_query_id_info_cfm->Asdu + 1,idlen);
	
	if(getHplcTestMode() == RD_CTRL)
	{
		dump_buf(addr,MACADRDR_LENGTH);
		dump_buf(rd_ctrl_info.mac_addr,MACADRDR_LENGTH);
		;
		if(memcmp(addr,rd_ctrl_info.mac_addr,MACADRDR_LENGTH) == 0 && rd_ctrl_info.recupflag == FALSE)
		{
			app_printf("read id is ok\r");
			timer_stop(g_RdReadIdTimer, TMR_NULL);
			rd_ctrl_info.recupflag = TRUE;
			g_RdReadIdTimes = 0;
			rd_ctrl_pack_read_id_info(rd_send_frame, &rd_send_frame_len ,rd_ctrl_info.mac_addr,rd_query_id_info_cfm->Asdu + 1,idlen);
			uart_send(rd_send_frame, rd_send_frame_len);
		}
		else
		{
			app_printf("read id addr is err\r");		
		}
	}
	zc_free_mem(rd_send_frame);
}



void query_id_info_indication(QUERY_IDINFO_IND_t *pQueryIdInfoInd)
{
    QUERY_IDINFO_REQ_RESP_t  *pQueryIdInfoRsp = NULL;
    U8   byLen = 0;

    app_printf("QueryIdInfoIndication\n");


	pQueryIdInfoRsp = (QUERY_IDINFO_REQ_RESP_t*)zc_malloc_mem(sizeof(QUERY_IDINFO_REQ_RESP_t) + 30, "QueryIdInfoRsp",MEM_TIME_OUT); 
	
	if(getHplcTestMode() == RD_CTRL)
	{
		
		if(memcmp(pQueryIdInfoRsp->SrcMacAddr,rd_ctrl_info.mac_addr,MACADRDR_LENGTH))
		{
			app_printf("read id is ok\r");
			timer_stop(g_RdReadIdTimer, TMR_NULL);
			g_RdReadIdTimes = 0;
		}
		else
		{
			app_printf("read id addr is err\r");
		}
		app_printf("ID(%d): ",pQueryIdInfoRsp->AsduLength);
		dump_buf(pQueryIdInfoRsp->Asdu,pQueryIdInfoRsp->AsduLength);
		zc_free_mem(pQueryIdInfoRsp);
		return;
	}

    app_printf("pQueryIdInfoRsp 0x%x\n",pQueryIdInfoRsp);
    app_printf("IdType = %d\n", pQueryIdInfoInd->IdType);
    
    if(pQueryIdInfoInd->IdType == APP_GW3762_CHIP_ID)
    {
        pQueryIdInfoRsp->Asdu[byLen++] = 24;
        __memcpy(&pQueryIdInfoRsp->Asdu[byLen], ManageID, 24);
        byLen += 24;
    }
    else if(pQueryIdInfoInd->IdType == APP_GW3762_MODULE_ID || pQueryIdInfoInd->IdType == APP_GW3762_MODULE_ID_OLD)
    {
        pQueryIdInfoRsp->Asdu[byLen++] = 11;
        __memcpy(&pQueryIdInfoRsp->Asdu[byLen], ModuleID, 11);
        byLen += 11;
    }

    pQueryIdInfoRsp->Asdu[byLen++] = nnib.DeviceType;
    
    pQueryIdInfoRsp->AsduLength = byLen;
    pQueryIdInfoRsp->PacketSeq = pQueryIdInfoInd->PacketSeq;
    pQueryIdInfoRsp->IdType = pQueryIdInfoInd->IdType;
    pQueryIdInfoRsp->direct = e_UPLINK;
    pQueryIdInfoRsp->destei = pQueryIdInfoInd->Srctei;

    __memcpy(pQueryIdInfoRsp->DstMacAddr, pQueryIdInfoInd->SrcMacAddr , 6);
//    __memcpy(pQueryIdInfoRsp->SrcMacAddr, pQueryIdInfoInd->DstMacAddr, 6);
    __memcpy(pQueryIdInfoRsp->SrcMacAddr, nnib.MacAddr, 6);
    dump_buf(pQueryIdInfoRsp->DstMacAddr,6);
    dump_buf(pQueryIdInfoRsp->SrcMacAddr,6);
    

    ApsQueryIdInfoReqResp(pQueryIdInfoRsp);

    zc_free_mem(pQueryIdInfoRsp);
    
    return;
}



#endif

