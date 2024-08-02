/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */


#include "list.h"
#include "types.h"
#include "Trios.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
#include "ZCsystem.h"
#include "app_read_jzq_cco.h"
#include "app_gw3762.h"
#include "SchTask.h"
#include "app_dltpro.h"
#include "printf_zc.h"
#include "app_common.h"
#include "app_gw3762_ex.h"
#include "app_read_router_cco.h"


CJQ2_INFO_t Cjq2Info_t;
extern U8  DataAddrFilterStatus;

#if defined(STATIC_MASTER)
static int32_t jzq_read_afn_f1f1_entrylist_check(multi_trans_header_t* list, multi_trans_t* new_list);
static void jzq_read_app_gw3762_up_anff1f1_upf00_deny(int32_t err_code, void* pTaskPrmt);
static void jzq_read_afn_f1f1_read_meter_timeout(void* pTaskPrmt);
static void jzq_read_cntt_act_read_meter_proc(void* pTaskPrmt, void* pUplinkData);
static int32_t jzq_read_afn_13f1_entrylist_check(multi_trans_header_t* list, multi_trans_t* new_list);
static void jzq_read_app_gw3762_up_anf13f1_upf00_deny(int32_t err_code, void* pTaskPrmt);
static void jzq_read_afn_13f1_read_meter_timeout(void* pTaskPrmt);

/*************************************************************************
 * 函数名称	: 	jzq_read_cco_app_read_meter_ind_proc
 * 函数说明	: 	cco载波抄表上行数据回调接口
 * 参数说明	: 	READ_METER_IND_t *pReadMeterInd    - 抄读数据响应原语
 * 返回值	: 	无
 *************************************************************************/
void jzq_read_cco_app_read_meter_ind_proc(READ_METER_IND_t *pReadMeterInd)
{
    MULTI_TASK_UP_t MultiTaskUp;

    
    MultiTaskUp.pListHeader =   pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_RM ? &HIGHEST_PRIORITY_LIST:
            					pReadMeterInd->ReadMode == e_ROUTER_ACTIVE_RM ? &F14F1_TRANS_LIST:
            					pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_CONCURRENT ? &F1F1_TRANS_LIST:
								pReadMeterInd->ReadMode == e_CURVE_DATA_CONCRRNT_READ ? &F1F1_TRANS_LIST:
            					  NULL;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , DatagramSeq);
    MultiTaskUp.Upoffset = (U8)offset_of(READ_METER_IND_t , ApsSeq);
    MultiTaskUp.Cmplen = sizeof(pReadMeterInd->ApsSeq);
    
    //pMultiTaskUp->AsduLength   = sizeof(READ_METER_IND_t)+PayloadLen;
    //__memcpy(pMultiTaskUp->Asdu, (U8 *)pReadMeterInd, pMultiTaskUp->AsduLength);
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pReadMeterInd);
    
    return;
}

void jzq_read_cco_app_read_meter_cfm_proc(READ_METER_CFM_t *pReadMeterCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;

    
    MultiTaskUp.pListHeader =   pReadMeterCfm->PacketID == e_CONCENTRATOR_ACTIVE_RM ? &HIGHEST_PRIORITY_LIST:
            					pReadMeterCfm->PacketID == e_ROUTER_ACTIVE_RM ? &F14F1_TRANS_LIST:
            					pReadMeterCfm->PacketID == e_CONCENTRATOR_ACTIVE_CONCURRENT ? &F1F1_TRANS_LIST:
            					  NULL;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , DatagramSeq);
    MultiTaskUp.Upoffset = (U8)offset_of(READ_METER_CFM_t , ApsSeq);
    MultiTaskUp.Cmplen = sizeof(pReadMeterCfm->ApsSeq);
    
    multi_trans_find_plc_multi_task_send_fail(&MultiTaskUp, (void*)pReadMeterCfm);
    
    return;
}

/*************************************************************************
 * 函数名称	: 	jzq_read_app_read_meter_req
 * 函数说明	: 	cco发送抄读请求
 * 参数说明	: 	void *pTaskPrmt         - 任务参数
 * 返回值	: 	无
 *************************************************************************/
void jzq_read_app_read_meter_req(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    READ_METER_REQ_t *pReadMeterRequest_t = NULL;
	U8	boardaddr[6];
	memset(boardaddr,0xff,6);
    pReadMeterRequest_t = (READ_METER_REQ_t*)zc_malloc_mem(sizeof(READ_METER_REQ_t)+pReadTask->FrameLen,"F1F1ReadMeter",MEM_TIME_OUT);

    if(pReadTask->SendType == F1F1_READMETER)
    {
        pReadMeterRequest_t->ReadMode = e_CONCENTRATOR_ACTIVE_CONCURRENT;
		if((pReadTask->ReTransmitCnt%2) == 0)
		{
			pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT18;//pReadTask->DeviceTimeout;
			pReadMeterRequest_t->CfgWord            = (NO_REPLY_RETRY_FLAG) | (NACK_RETRY_FLAG << 1) | (MAX_RETRY_NUM << 2);
		}
		else
		{
			pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT35;//pReadTask->DeviceTimeout;
			pReadMeterRequest_t->CfgWord            = 0;
		}
		
		//广播最后一条必须3.5秒
		if(pReadTask->ReTransmitCnt == 1)
        {
			pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT35;
			pReadMeterRequest_t->CfgWord            = 0;
        }
    }
	else if(pReadTask->SendType == AFN13F1_READMETER || pReadTask->SendType == AFN02F1_READMETER)
	{
	    pReadMeterRequest_t->ReadMode = e_CONCENTRATOR_ACTIVE_RM;
		pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT35;//pReadTask->DeviceTimeout;
    }
    else if(pReadTask->SendType == AFN14F1_READMETER)
    {
        pReadMeterRequest_t->ReadMode = e_ROUTER_ACTIVE_RM;
		pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT35;//pReadTask->DeviceTimeout;
		router_read_change_route_active_state(EVT_APS_IND, 100);
    }
	else if(pReadTask->SendType == F1F100_READMETER)
    {
        pReadMeterRequest_t->ReadMode = e_CURVE_DATA_CONCRRNT_READ;
		if((pReadTask->ReTransmitCnt%2) == 0)
		{
			pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT18;//pReadTask->DeviceTimeout;
			pReadMeterRequest_t->CfgWord            = (NO_REPLY_RETRY_FLAG) | (NACK_RETRY_FLAG << 1) | (MAX_RETRY_NUM << 2);
		}
		else
		{
			pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT35;//pReadTask->DeviceTimeout;
			pReadMeterRequest_t->CfgWord            = 0;
		}
		
		//广播最后一条必须3.5秒
		if(pReadTask->ReTransmitCnt == 1)
        {
			pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT35;
			pReadMeterRequest_t->CfgWord            = 0;
        }
    }
    
    pReadMeterRequest_t->Direction			=    e_DOWNLINK;
    //pReadMeterRequest_t->Timeout			=	 DEVUARTTIMEOUT;//pReadTask->DeviceTimeout;
    if(pReadTask->SendType == F1F1_READMETER  || pReadTask->SendType == F1F100_READMETER)
    {
        pReadMeterRequest_t->PacketIner		    =	PACKET_INTER;
    }
    else
    {
        pReadMeterRequest_t->PacketIner		    =	0;
    }
    pReadMeterRequest_t->ResponseState		=	0;
    pReadMeterRequest_t->OptionWord		=	0;
    pReadMeterRequest_t->ProtocolType		=	pReadTask->ProtoType;
    if(pReadTask->BroadcastFlag == AllBroadcast)
    {
        if(multi_trans_check_broadcast_num(pReadTask) < app_ext_info.param_cfg.BroadcastConRMNum)//BROADCAST_CONRM_NUM
        {
            memset(pReadMeterRequest_t->DstMacAddr, 0xFF, 6);
            pReadTask->BroadcastNow = TRUE;
        }
        else
        {
            goto free;
            //__memcpy(pReadMeterRequest_t->DstMacAddr, pReadTask->CnmAddr, 6);
        }
    }
    else
    {
        if(pReadTask->ReTransmitCnt == 1 && multi_trans_check_broadcast_num(pReadTask) < app_ext_info.param_cfg.BroadcastConRMNum)//BROADCAST_CONRM_NUM
        {
            memset(pReadMeterRequest_t->DstMacAddr, 0xFF, 6);
            pReadTask->BroadcastNow = TRUE;
        }
        else
        {
            __memcpy(pReadMeterRequest_t->DstMacAddr, pReadTask->CnmAddr, 6);
        }
    }
    __memcpy(pReadMeterRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);
    //pReadMeterRequest_t->Handle      = AppPIB_t.AppHandle++;
	pReadMeterRequest_t->DatagramSeq = pReadTask->DatagramSeq;

    pReadMeterRequest_t->AsduLength = pReadTask->FrameLen;
    __memcpy(pReadMeterRequest_t->Asdu, pReadTask->FrameUnit, pReadTask->FrameLen);

	//dump_buf(pReadMeterRequest_t->Asdu, pReadMeterRequest_t->AsduLength);

    //app_printf("pReadMeterRequest_t->DstMacAddr : ");
	//dump_buf(pReadMeterRequest_t->DstMacAddr, 6);
	if(memcmp(pReadMeterRequest_t->DstMacAddr, boardaddr, 6)==0)
	{
	 	pReadMeterRequest_t->SendType = app_ext_info.param_cfg.AllBroadcastType == 0 ? e_PROXY_BROADCAST_FREAM_NOACK : app_ext_info.param_cfg.AllBroadcastType;
	}
	else
	{
		pReadMeterRequest_t->SendType = e_UNICAST_FREAM;
	}
    ApsReadMeterRequest(pReadMeterRequest_t);
    app_printf("aps req, timeout: %d, plc_num: %d\n", pReadTask->DeviceTimeout, pReadMeterRequest_t->DatagramSeq);

    free:
	zc_free_mem(pReadMeterRequest_t);
}

/*************************************************************************
 * 函数名称	: 	jzq_read_cntt_act_read_meter_req
 * 函数说明	: 	cco数据抄读并发请求
 * 参数说明	: 	
 *              U8 localFrameSeq    - 本地报文序号
                U8 *pMeterAddr      - 表地址
                U8 *pCnmAddr        - 网络地址
                U8 protoType        - 协议类型
                U16 frameLen        - 抄读帧长度
                U8 *pFrameUnit      - 抄读帧地址
                U8 afn02Flag        - afn02标志
                MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/
void jzq_read_cntt_act_read_meter_req(U8 localFrameSeq, U8 *pMeterAddr, U8 *pCnmAddr, U8 protoType, U16 frameLen, U8 *pFrameUnit,
                                                                         U8 afn02Flag, MESG_PORT_e port)
{
    multi_trans_t Afn13F1Readmeter;
    
	memset((void*)&Afn13F1Readmeter, 0x00, sizeof(multi_trans_t));
    
	Afn13F1Readmeter.lid = READ_METER_LID;
	Afn13F1Readmeter.SrcTEI = 0;
    Afn13F1Readmeter.DeviceTimeout = DEVTIMEOUT;
    Afn13F1Readmeter.Framenum = localFrameSeq;
    __memcpy(Afn13F1Readmeter.Addr, pMeterAddr, MAC_ADDR_LEN);	
    __memcpy(Afn13F1Readmeter.CnmAddr, pCnmAddr, MAC_ADDR_LEN);

    Afn13F1Readmeter.MsgPort = port;

	Afn13F1Readmeter.State = UNEXECUTED;
    
	Afn13F1Readmeter.SendType = afn02Flag == TRUE ? AFN02F1_READMETER : AFN13F1_READMETER;
    Afn13F1Readmeter.DatagramSeq = ++ApsSendPacketSeq;
	Afn13F1Readmeter.DealySecond = HIGHEST_PRIORITY_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    Afn13F1Readmeter.ReTransmitCnt = app_ext_info.param_cfg.ReTranmitMaxNum;  //RETRANMITMAXNUM
    Afn13F1Readmeter.SendRetryCnt = 0;
	
    Afn13F1Readmeter.DltNum = 1;
	Afn13F1Readmeter.ProtoType = protoType;
	Afn13F1Readmeter.FrameLen  = frameLen;
    
    Afn13F1Readmeter.EntryListCheck = jzq_read_afn_13f1_entrylist_check;
	Afn13F1Readmeter.EntryListfail = jzq_read_app_gw3762_up_anf13f1_upf00_deny;
	Afn13F1Readmeter.TaskPro = jzq_read_app_read_meter_req; 
    Afn13F1Readmeter.TaskUpPro = jzq_read_cntt_act_read_meter_proc;
	Afn13F1Readmeter.TimeoutPro = jzq_read_afn_13f1_read_meter_timeout;

	Afn13F1Readmeter.pMultiTransHeader = &HIGHEST_PRIORITY_LIST;
	Afn13F1Readmeter.CRT_timer = highest_priority_timer;
	
    multi_trans_put_list(Afn13F1Readmeter, pFrameUnit);

    return;
}

/*************************************************************************
 * 函数名称	: 	jzq_read_cntt_act_cncrnt_read_meter_req
 * 函数说明	: 	cco并发抄读并发请求
 * 参数说明	:
 *              U8 localFrameSeq    - 本地报文序号
 *              U8 readMeterNum     - 抄表帧数
                U8 *pMeterAddr      - 表地址
                U8 *pCnmAddr        - 网络地址
                U8 protoType        - 协议类型
                U16 frameLen        - 抄读帧长度
                U8 *pFrameUnit      - 抄读帧地址
                U8 BroadcastFlag    - 广播标志
                MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/
void jzq_read_cntt_act_cncrnt_read_meter_req(U8 localFrameSeq, U8 readMeterNum, U8 *pMeterAddr, U8 *pCnmAddr, U8 protoType,
                                                      U16 frameLen, U8 *pFrameUnit,U8 BroadcastFlag, MESG_PORT_e port)
{
    multi_trans_t f1f1Readmeter;

    memset((void*)&f1f1Readmeter, 0x00, sizeof(multi_trans_t));
	
	f1f1Readmeter.lid = READ_METER_LID;
	f1f1Readmeter.SrcTEI = 0;
	//13项串口超时需要3.5*13=45.5秒，载波超时（4+13*0.2）* 6 = 39.6
	//如果有其中一项回复，增3.5*12 = 42，还是大于载波超时，那抄读成功的一帧就回不上去，所以需要拉长
	//载波超时（4+13*0.4）*6 = 55.2
    f1f1Readmeter.DeviceTimeout = ((readMeterNum>1)?(app_ext_info.param_cfg.ReTranmitMaxTime+readMeterNum*4):app_ext_info.param_cfg.ReTranmitMaxTime);	//((readMeterNum>1)?(DEVTIMEOUT+readMeterNum*4):DEVTIMEOUT);	
    f1f1Readmeter.Framenum = localFrameSeq;
    __memcpy(f1f1Readmeter.Addr, pMeterAddr, MAC_ADDR_LEN);	
    __memcpy(f1f1Readmeter.CnmAddr, pCnmAddr, MAC_ADDR_LEN);
	
    f1f1Readmeter.MsgPort = port;
	f1f1Readmeter.State = UNEXECUTED;
	f1f1Readmeter.SendType = F1F1_READMETER;
    f1f1Readmeter.DatagramSeq = ++ApsSendPacketSeq;
	f1f1Readmeter.DealySecond = F1F1_TRANS_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    f1f1Readmeter.ReTransmitCnt = (BroadcastFlag == AllBroadcast?1:app_ext_info.param_cfg.ReTranmitMaxNum);//RETRANMITMAXNUM 未入网的节点支持并发抄表一次，但是仅支持BROADCAST_CONRM_NUM条
    f1f1Readmeter.BroadcastFlag = BroadcastFlag;
    f1f1Readmeter.SendRetryCnt = 0;
	
    if(BroadcastFlag == AllBroadcast)
    {
        f1f1Readmeter.DeviceTimeout = 2 * f1f1Readmeter.DeviceTimeout;
    }
	
    f1f1Readmeter.DltNum = readMeterNum;
	f1f1Readmeter.ProtoType = protoType;
	f1f1Readmeter.FrameLen  = frameLen;
    
    f1f1Readmeter.EntryListCheck = jzq_read_afn_f1f1_entrylist_check;
	f1f1Readmeter.EntryListfail = jzq_read_app_gw3762_up_anff1f1_upf00_deny;
	f1f1Readmeter.TaskPro = jzq_read_app_read_meter_req; 
    f1f1Readmeter.TaskUpPro = jzq_read_cntt_act_cncrnt_proc;
	f1f1Readmeter.TimeoutPro = jzq_read_afn_f1f1_read_meter_timeout;
	
    f1f1Readmeter.pMultiTransHeader = &F1F1_TRANS_LIST;
	f1f1Readmeter.CRT_timer = f1f1_trans_timer;  
    multi_trans_put_list(f1f1Readmeter, pFrameUnit);
	
    return;
}

/*************************************************************************
 * 函数名称	: 	jzq_read_cntt_act_cncrnt_proc
 * 函数说明	: 	cco载波并发抄表上行数据处理
 * 参数说明	: 	void *pTaskPrmt         - 任务参数
 *                void *pUplinkData       - 抄读数据响应原语
 * 返回值	: 	无
 *************************************************************************/
void jzq_read_cntt_act_cncrnt_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    READ_METER_IND_t  *pReadMeterInd = pUplinkData;
	
	U8  ScrMeterAddr[MAC_ADDR_LEN]={0,0,0,0,0,0};
	//S16 Seq = 0;
	//U16 resultnum;
	//int ii;

    if(e_CONCENTRATOR_ACTIVE_CONCURRENT != pReadMeterInd->ReadMode  && e_CURVE_DATA_CONCRRNT_READ != pReadMeterInd->ReadMode)
    {
        return;
    }
    app_printf("pRMIlen= %d\n",pReadMeterInd->AsduLength);

    __memcpy(ScrMeterAddr, pReadMeterInd->SrcMacAddr, 6);
    ChangeMacAddrOrder(ScrMeterAddr);
    app_printf("ScrMeterAddr = ");
    dump_buf(ScrMeterAddr, 6);
    
	if(TRUE==Check645Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength, NULL, ScrMeterAddr,NULL))
    {
        app_printf("645 frame \n");
    }
    else  if(TRUE==Check698Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength, NULL,ScrMeterAddr, NULL))

    {
		app_printf("698 frame \n");
    }
	else if(TRUE == CheckT188Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength, ScrMeterAddr))
	{
		app_printf("T188 frame \n");
	}
	else
	{
		app_printf("other frame \n");
	}

	if((DataAddrFilterStatus == TRUE&&(memcmp(pReadMeterInd->SrcMacAddr, pReadTask->CnmAddr, MAC_ADDR_LEN) == 0 ||
               	memcmp(ScrMeterAddr, pReadTask->Addr, MAC_ADDR_LEN) == 0))
            ||(DataAddrFilterStatus == FALSE&&(pReadTask->DatagramSeq == pReadMeterInd->ApsSeq)))
    {
        if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
        {
            up_cnm_addr_by_read_meter(pReadMeterInd, ScrMeterAddr);
        }
		if(e_CONCENTRATOR_ACTIVE_CONCURRENT == pReadMeterInd->ReadMode)
        {
        	app_gw3762_up_afnf1_f1_up_frame(ScrMeterAddr, pReadMeterInd->ProtocolType, pReadMeterInd->Asdu, pReadMeterInd->AsduLength, pReadTask->MsgPort, pReadTask->Framenum);
		}
		else if(e_CURVE_DATA_CONCRRNT_READ == pReadMeterInd->ReadMode)
		{
			app_gw3762_up_afnf1_f100_up_frame(ScrMeterAddr, pReadMeterInd->ProtocolType, pReadMeterInd->Asdu, pReadMeterInd->AsduLength, pReadTask->MsgPort, pReadTask->Framenum);
		}
/*
	                                         
		Seq = seach_seq_by_mac_addr(ScrMeterAddr);
	    if(Seq != -1)
	    {
	        f1f1infolist[Seq].Resultflag= f1f1infolist[Seq].Resultflag+  1 ;
	        WHLPTST //共享存储内容保护
	        WhiteMacAddrList[Seq].Result++;
	        if(WhiteMacAddrList[Seq].Result>=0xff)WhiteMacAddrList[Seq].Result=1;
	        WHLPTED//释放共享存储内容保护
	    }
					
	    resultnum = 0;
	    for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	    {
	        if(f1f1infolist[ii].Resultflag>0)
	        {
	            resultnum++;
	        }
	    }
	    app_printf("F1F1Up ok,Seq %d,all= %d\n",Seq,resultnum);
	*/    
    }    
    return;
}

static int32_t jzq_read_afn_f1f1_entrylist_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    list_head_t *pos,*node;
	multi_trans_t  *mbuf_n;
    U8 nullAddr[6] = { 0 };
    
    if(list->nr >= list->thr)
    {
		return -1;
	}
                             
    /*Check for duplicate meter address*/
    list_for_each_safe(pos, node,&list->link)
    {
        mbuf_n = list_entry(pos, multi_trans_t, link);
        
        if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
        {
            if(memcmp(mbuf_n->Addr, new_list->Addr, 6) == 0
                || (memcmp(mbuf_n->CnmAddr, new_list->CnmAddr, 6) == 0 && memcmp(new_list->CnmAddr, nullAddr, 6) != 0))//SHANGHAI 上海要求，跳过同一二采下的表计
            {
                return -2;
            }
        }
        else
        {
	        if(memcmp(mbuf_n->Addr, new_list->Addr, 6) == 0)
	        {
	            return -2;
			}
    	}
    }

    return 0;
}

static void jzq_read_app_gw3762_up_anff1f1_upf00_deny(int32_t err_code, void *pTaskPrmt)//,int8_t localseq)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    if(err_code == -1)
    {
        app_gw3762_up_afn00_f2_deny_by_seq(APP_GW3762_OUT_MAXCONCURRNUM_ERRCODE,pReadTask->Framenum, pReadTask->MsgPort);//109超过最大并发数
    }
	else if(err_code == -2)
	{
        app_gw3762_up_afn00_f2_deny_by_seq(APP_GW3762_METER_READING_ERRCODE,pReadTask->Framenum, pReadTask->MsgPort);//111此表正在抄读
	}
    else
    {
        
    }

	return;
}

static void jzq_read_afn_f1f1_read_meter_timeout(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    
    app_gw3762_up_afnf1_f1_up_frame(pReadTask->Addr, pReadTask->ProtoType, 0 , 0, pReadTask->MsgPort,pReadTask->Framenum);
    return;
}

/*************************************************************************
 * 函数名称	: 	jzq_read_cntt_act_read_meter_proc
 * 函数说明	: 	cco载波抄表上行数据处理
 * 参数说明	: 	void *pTaskPrmt         - 任务参数
 *              void *pUplinkData       - 抄读数据响应原语
 * 返回值	: 	无
 *************************************************************************/
static void jzq_read_cntt_act_read_meter_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    READ_METER_IND_t  *pReadMeterInd = pUplinkData;
    U8  ScrMeterAddr[MAC_ADDR_LEN];

    app_printf("pReadMeterInd->AsduLength = %d\n",pReadMeterInd->AsduLength);
    dump_buf(pReadMeterInd->Asdu, pReadMeterInd->AsduLength);

    dump_buf(pReadMeterInd->SrcMacAddr,MAC_ADDR_LEN);
    dump_buf(pReadTask->Addr,MAC_ADDR_LEN);
    __memcpy(ScrMeterAddr, pReadMeterInd->SrcMacAddr, MAC_ADDR_LEN);
    ChangeMacAddrOrder(ScrMeterAddr);
    app_printf("pReadMeterInd->ProtocolType = %d\n",pReadMeterInd->ProtocolType);

    /*
    if(TRANS_PROTO_RF == pReadMeterInd->ProtocolType)
    {
        if(0x05 == pReadMeterInd->Asdu[0] && 0x47 == pReadMeterInd->Asdu[1] && 0xcd == pReadMeterInd->Asdu[2])
        {
            app_printf("water cmd id %x\n", *(pReadMeterInd->Asdu+1+2+1+2+7+7));
            if(WATER_45 == *(pReadMeterInd->Asdu+1+2+1+2+7+7))
            {
                app_printf("CnttActRM set info rsp\n");
                g_set_double_info_control.status = SETINFO_STATUS_SUCCESS;
                timer_stop(g_set_double_info_Timer,TMR_CALLBACK);
            }
            else
            {
                app_printf("CnttActRM read water rsp\n");
                app_gw3762_up_afn02_f1_up_frame(pReadTask->ProtoType, 1, pReadMeterInd->Asdu+2+1+2+7+7 + 1+2, pReadMeterInd->AsduLength-(2+1+2+7+7+1+2), e_UART1_MSG);
            }
        }
    }
    else
    */
    {
        app_printf("pReadMeterInd->AsduLength = %d\n",pReadMeterInd->AsduLength);
		dump_buf(pReadMeterInd->SrcMacAddr,MAC_ADDR_LEN);
		dump_buf(pReadTask->Addr,MAC_ADDR_LEN);

		if(TRUE==Check645Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength, NULL, ScrMeterAddr,NULL))
		{
		    app_printf("645 frame \n");
		}
		else  if(TRUE==Check698Frame(pReadMeterInd->Asdu, pReadMeterInd->AsduLength, NULL, ScrMeterAddr, NULL))

		{
			app_printf("698 frame \n");
		}
		else
		{
			app_printf("other frame \n");
		}

		//地址过滤打开时，只判定地址过滤；关闭时，使用报文序号过滤
        if((DataAddrFilterStatus == TRUE && (memcmp(pReadMeterInd->SrcMacAddr, pReadTask->CnmAddr, MAC_ADDR_LEN) == 0 ||
           	memcmp(ScrMeterAddr, pReadTask->Addr, MAC_ADDR_LEN) == 0))
             ||(DataAddrFilterStatus == FALSE && (pReadTask->DatagramSeq == pReadMeterInd->ApsSeq)))
		{
            if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
            {
                up_cnm_addr_by_read_meter(pReadMeterInd, ScrMeterAddr);
            }

			if(pReadTask->SendType == AFN13F1_READMETER)
            {   
                AppGw3762Afn13F1State.ValidFlag = FALSE;
                app_gw3762_up_afn13_f1_up_frame(pReadTask->Addr, pReadTask->ProtoType, 1, pReadMeterInd->Asdu, pReadMeterInd->AsduLength, pReadTask->MsgPort,pReadTask->Framenum);
            }
            else if(pReadTask->SendType == AFN02F1_READMETER)
            {
                AppGw3762Afn13F1State.Afn02Flag = FALSE;
                app_gw3762_up_afn02_f1_up_frame(pReadTask->Addr, pReadTask->ProtoType, 1, pReadMeterInd->Asdu, pReadMeterInd->AsduLength, 
                                                                        pReadTask->MsgPort,pReadTask->Framenum);
            }
		}
	}
}

static int32_t jzq_read_afn_13f1_entrylist_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    list_head_t *pos,*node;
    multi_trans_t  *mbuf_n;

    app_printf("list->nr = %d, list->thr = %d\n",list->nr, list->thr);       
    if(list->nr >= list->thr)
    {
        return -1;
    }

    /*Check for duplicate meter address*/
    //printf_s("list->link.next = %08x, list->link.prev = %08x, list = %08x\n", list->link.next, list->link.prev, list);

    //extern void app_show_record_mem_list();
    //app_show_record_mem_list();
    
    list_for_each_safe(pos, node,&list->link)
    {
        mbuf_n = list_entry(pos, multi_trans_t, link);

        app_printf("mbuf_n->SendType = %d, mbuf_n->State = %d, new_list->SendType = %d\n",
                              mbuf_n->SendType, mbuf_n->State, new_list->SendType);
        if(mbuf_n->SendType == new_list->SendType && mbuf_n->State != NONRESP)
        {
            app_printf("entry list fail, error code is -2\n");
            // return -2;          //双模台体测试，需要发送8次抄表帧。 注释可以通过测试
        }
    }

    return 0;
}

static void jzq_read_app_gw3762_up_anf13f1_upf00_deny(int32_t err_code, void *pTaskPrmt)
{
     multi_trans_t  *pReadTask = pTaskPrmt;
    if(err_code == -2)
    {
        app_gw3762_up_afn00_f2_deny_by_seq(APP_GW3762_MASTERBUSY_ERRCODE, pReadTask->Framenum,pReadTask->MsgPort);//111此表正在抄读
    }
    else
    {
			
    }
	
    return;
}

static void jzq_read_afn_13f1_read_meter_timeout(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    if(pReadTask->SendType == AFN02F1_READMETER)
    {
        AppGw3762Afn13F1State.Afn02Flag = FALSE;
        app_gw3762_up_afn02_f1_up_frame(pReadTask->Addr, pReadTask->ProtoType, 1, NULL, 0, pReadTask->MsgPort,pReadTask->Framenum);
    }
    else
    {
        AppGw3762Afn13F1State.ValidFlag = FALSE;
        app_gw3762_up_afn13_f1_up_frame(pReadTask->Addr, pReadTask->ProtoType, 1, NULL, 0, pReadTask->MsgPort,pReadTask->Framenum);
    }
    return;
}

#endif


                
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
