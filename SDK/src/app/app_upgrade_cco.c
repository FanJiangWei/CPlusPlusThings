/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	upgrade.c
 * Purpose:	
 * History:
 * Author : WWJ
 */
//#include <string.h>
//#include "app_upgrade.h"
#include "list.h"
#include "types.h"
#include "Trios.h"
#include "flash.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
//#include "SchTask.h"
#include "ZCsystem.h"
#include "flash.h"
#include "app_exstate_verify.h"
//#include "printf_zc.h"
#include "datalinkdebug.h"
#include "app_upgrade_cco.h"
#include "app_cnf.h"
#include "app_common.h"
#include "app_gw3762.h"
#include "Version.h"
#include "netnnib.h"
#include "crc.h"


#if defined(STATIC_MASTER)

ostimer_t *ReUpgradetimer = NULL;
ostimer_t *UpgradeCtrltimer = NULL;
ostimer_t *UpgradeTimeOuttimer = NULL;
ostimer_t *AutoUpgradetimer = NULL;//自动升级使用
ostimer_t *UpgradeSuspendtimer = NULL;//升级暂停
CCO_UPGRADE_POLL CcoUpgradPoll[MAX_WHITELIST_NUM] ;
CCO_UPGRADE_CTRL_INFO_t   CcoUpgrdCtrlInfo;
CURR_SLA_STA_INFO_t       CurrSlaveStaInfo;                             // 当前从节点站点信息
U8    FileUpgradeData[MIN_UPGRADE_BYTE_SIZE * 1024];                      //CCO升级缓存区
U8    FileTransBitMap[MAX_UPGRADE_FILE_SIZE / FILE_TRANS_BLOCK_SIZE / 8];   //CCO传文件位图
U8    FileTransBitMapCal[MAX_UPGRADE_FILE_SIZE / FILE_TRANS_BLOCK_SIZE / 8];//CCO统计位图
U8    Upgrade_filetrans_bradcst = 1 ;
U32   CurrUpgradeID = 0;
U8    IsCjqfile = e_JZQ_TASK;
uint32_t  DstImageBaseAddr = 0;
uint32_t  ImageFileSize = 0;
U32  LastblockID = 0xFFFFFFFF;
U8 UpgradeBySelfState = AUTO_UPGRADE_IDLE;//自动升级切换状态

//extern U32  LastblockID;
extern U32  LblockID;
extern U16  UpgradeTEI;

static void upgrade_cco_query_evt(work_t* work);
static void upgrade_cco_bradcst_evt(work_t *work);
static void upgrade_cco_perform_evt(work_t *work);
static void upgrade_cco_start_renew_evt(work_t *work);
static void upgrade_cco_stop_req_aps(void);
static void upgrade_cco_set_upgrade_bitmap(void);
static int8_t upgrade_cco_reupgrade_timer_init(void);
static int8_t upgrade_cco_ctrl_timer_init(void);
static void upgrade_cco_time_out_modify(uint32_t minute);

static U16 upgrade_cco_get_bradcst_interval(void)
{
#if defined(STD_DUAL)
    return (nnib.PCOCount)*40 + 100;
#else
    return (nnib.PCOCount)*20 + 50;
#endif
}

void upgrade_cco_app_upgrade_start_cfm_proc(UPGRADE_START_CFM_t *pUpgradeStartCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;
    
    MultiTaskUp.pListHeader =  &UPGRADE_START_LIST;
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(UPGRADE_START_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pUpgradeStartCfm->SrcMacAddr);
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pUpgradeStartCfm);
    return;
}

void upgrade_cco_app_upgrade_status_query_cfm_proc(UPGRADE_STATUS_QUERY_CFM_t *pUpgradeQueryCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;
    
    MultiTaskUp.pListHeader =  &UPGRADE_QUERY_LIST;                   
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(UPGRADE_STATUS_QUERY_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pUpgradeQueryCfm->SrcMacAddr);

	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pUpgradeQueryCfm);
    return;
}

void upgrade_cco_upgrade_station_info_cfm_proc(STATION_INFO_QUERY_CFM_t  *pStationInfoQueryCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;
    
    MultiTaskUp.pListHeader =  &UPGRADE_STATION_INFO_QUERY_LIST;                            
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t, CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(STATION_INFO_QUERY_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pStationInfoQueryCfm->SrcMacAddr);
 
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pStationInfoQueryCfm);
    return;
}

/*************************************************************************
 * 函数名称	: 	upgrade_cco_start_request
 * 函数说明	: 	cco发送开始升级
 * 参数说明	: 	void *pTaskPrmt         - 任务参数
 * 返回值	: 	无
 *************************************************************************/
static void upgrade_cco_start_request(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    UPGRADE_START_REQ_t *pUpgradeStartRequest_t = NULL;
    
    pUpgradeStartRequest_t = (UPGRADE_START_REQ_t*)zc_malloc_mem(sizeof(UPGRADE_START_REQ_t), "UpgradeStartReq",MEM_TIME_OUT); 
    pUpgradeStartRequest_t->UpgradeID = CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID;
    pUpgradeStartRequest_t->UpgradeTimeout = CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTimeout;
    pUpgradeStartRequest_t->BlockSize = CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize;
    pUpgradeStartRequest_t->FileSize  = CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize;
    pUpgradeStartRequest_t->FileCrc   = CcoUpgrdCtrlInfo.UpgrdFileInfo.FileCrc;
	
	//BrdcstFlag==TRUE?memset(pUpgradeStartRequest_t->DstMacAddr, 0xff, 6):
    __memcpy(pUpgradeStartRequest_t->DstMacAddr, pReadTask->CnmAddr, 6);
    __memcpy(pUpgradeStartRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);
  
    ApsUpgradeStartRequest(pUpgradeStartRequest_t);
    zc_free_mem(pUpgradeStartRequest_t);
}

static void upgrade_cco_file_trans_request(uint8_t pTaskPrmt)
{
    FILE_TRANS_REQ_t *pFileTransRequest_t = NULL;
    
    pFileTransRequest_t = (FILE_TRANS_REQ_t*)zc_malloc_mem(sizeof(UPGRADE_STATUS_QUERY_REQ_t)+CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize, 
                                                              "UpgradeQueryReq",MEM_TIME_OUT);

    //app_printf("FileTransReqAps.\n");

    pFileTransRequest_t->UpgradeID = CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID;
    //最后一包被400整除
    if((CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex == (CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum - 1))
        &&(0 != (CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize % CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize)))  // Last Block
    {
        pFileTransRequest_t->BlockSize = (CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize % CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize);
    }
    else
    {
        pFileTransRequest_t->BlockSize   = CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize;
    }
    pFileTransRequest_t->BlockSeq    = CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex;
    pFileTransRequest_t->TransMode  = pTaskPrmt;
    pTaskPrmt==e_FULL_NET_BROADCAST?memset(pFileTransRequest_t->DstMacAddr, 0xff, 6):
    __memcpy(pFileTransRequest_t->DstMacAddr, CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrSlaveAddr, 6);
    __memcpy(pFileTransRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);
    
	
    //给copy载荷数据到pMDB的载荷位置
    __memcpy(pFileTransRequest_t->DataBlock, &FileUpgradeData[CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize*pFileTransRequest_t->BlockSeq],
                                    pFileTransRequest_t->BlockSize); 
    
	
	//IsCjqfile == e_SELF_CJQ_TASK ? (pFileTransRequest_t->IsCjq = 1) : (pFileTransRequest_t->IsCjq = 0);
    app_printf("send file size : %d\n",pFileTransRequest_t->BlockSize);
    ApsFileTransRequest(pFileTransRequest_t);
	
    zc_free_mem(pFileTransRequest_t);
}

static void upgrade_cco_query_request(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    UPGRADE_STATUS_QUERY_REQ_t *pUpgradeQueryRequest_t = NULL;
    
    pUpgradeQueryRequest_t = (UPGRADE_STATUS_QUERY_REQ_t*)zc_malloc_mem(sizeof(UPGRADE_STATUS_QUERY_REQ_t), "UpgradeQueryReq",MEM_TIME_OUT);
    pUpgradeQueryRequest_t->UpgradeID = CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID;
    pUpgradeQueryRequest_t->StartBockIndex = 0;
    pUpgradeQueryRequest_t->QueryBlockCount = 0xFFFF;
    __memcpy(pUpgradeQueryRequest_t->DstMacAddr, pReadTask->CnmAddr, 6);
	
    __memcpy(pUpgradeQueryRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);
   
    ApsUpgradeStatusQueryRequest(pUpgradeQueryRequest_t);
	
    zc_free_mem(pUpgradeQueryRequest_t);
}

void upgrade_cco_test_run_req_aps(void)
{
    UPGRADE_PERFORM_REQ_t  *pUpgradePerformReq_t=NULL;

    //申请原语空间
	pUpgradePerformReq_t=(UPGRADE_PERFORM_REQ_t*)zc_malloc_mem(sizeof(UPGRADE_PERFORM_REQ_t), 
	                                                                                "UpgradePerformReq",MEM_TIME_OUT);

    app_printf("Now is UPGRADE_PERFORM.\n");
        
    pUpgradePerformReq_t->WaitResetTime = WAIT_RESET_TIME;
    pUpgradePerformReq_t->UpgradeID     = CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID;
    pUpgradePerformReq_t->TestRunTime   = TEST_RUN_TIME;
    //__memcpy(pUpgradePerformReq_t->DstMacAddr, CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrSlaveAddr, 6);
    memset(pUpgradePerformReq_t->DstMacAddr, 0xFF, 6);
    __memcpy(pUpgradePerformReq_t->SrcMacAddr, GetNnibMacAddr(), 6);

    ApsUpgradePerformRequest(pUpgradePerformReq_t);
    
    zc_free_mem(pUpgradePerformReq_t);
    return;
}

/*************************************************************************
 * 函数名称	: 	upgrade_cco_stop_req_aps
 * 函数说明	: 	cco发送停止升级
 * 参数说明	: 	无
 * 返回值	: 	无
 *************************************************************************/
static void upgrade_cco_stop_req_aps(void)
{
    UPGRADE_STOP_REQ_t  *pUpgradeStopRequest_t = NULL;

    //申请原语空间
	pUpgradeStopRequest_t = (UPGRADE_STOP_REQ_t*)zc_malloc_mem(sizeof(UPGRADE_STOP_REQ_t) +100, 
	                                                                                "UpgradeStopReq",MEM_TIME_OUT);
	app_printf("Now is UPGRADE_STOP.\n");

    pUpgradeStopRequest_t->UpgradeID =  0;       //CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID;

    memset(pUpgradeStopRequest_t->DstMacAddr, 0xFF, 6);
    __memcpy(pUpgradeStopRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);


    ApsUpgradeStopRequest(pUpgradeStopRequest_t);

    zc_free_mem(pUpgradeStopRequest_t);
    return;
}

void upgrade_cco_station_info_request(void *pTaskPrmt)
{
    STATION_INFO_QUERY_REQ_t  *pStaInfoQueryReq_t=NULL;
    multi_trans_t  *pReadTask = pTaskPrmt;
    int i;

    //申请原语空间
	pStaInfoQueryReq_t = (STATION_INFO_QUERY_REQ_t*)zc_malloc_mem(sizeof(STATION_INFO_QUERY_REQ_t)+STA_INFO_LIST_MAXNUM, 
	                                                                                "StaInfoQueryReq",MEM_TIME_OUT);

    pStaInfoQueryReq_t->InfoListNum = STA_INFO_LIST_MAXNUM;
    __memcpy(pStaInfoQueryReq_t->DstMacAddr, pReadTask->CnmAddr, 6);
    __memcpy(pStaInfoQueryReq_t->SrcMacAddr, FlashInfo_t.ucJZQMacAddr, 6);


    for(i=0; i<STA_INFO_LIST_MAXNUM; i++)
    {
        pStaInfoQueryReq_t->InfoList[i] = i;
    }
    pStaInfoQueryReq_t->InfoListLen = STA_INFO_LIST_MAXNUM;

    ApsStationInfoQueryRequest(pStaInfoQueryReq_t);

    zc_free_mem(pStaInfoQueryReq_t);

    return;
}

static U8 upgrade_cco_set_upgrade_down_bitmap(CONCURRENCY_CTRL_INFO_t *pConInfo, U8 tasktype)
{
    U16  ii;
    U8   State = FALSE;
    app_printf("ST seq\ttei\tUP_S\tResult\n");
    for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(CcoUpgradPoll[ii].Valid == 1&&(((*(U8*)&CcoUpgradPoll[ii])>>(tasktype))&0x01)==0)
        {
            pConInfo->taskPolling[ii].Valid = 1;
            pConInfo->taskPolling[ii].Send = 0;
            pConInfo->taskPolling[ii].Result = 0;
            State = TRUE;
            app_printf("%d\t%d\t%d\t%d\t%d\n",
                ii,
                ii+2,
                (*(U8*)&CcoUpgradPoll[ii]),
                (((*(U8*)&CcoUpgradPoll[ii])>>(tasktype))&0x01),
                pConInfo->taskPolling[ii].Result);
        }
        else
        {
            pConInfo->taskPolling[ii].Valid = 0;
        }
            
    }
    if(State == TRUE);
    return State;
    
}
/*

void reset_upgrade_start_ctrl_bitmap(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    
}
U8 set_upgrade_Qbitmap_ctrl_bitmap(CONCURRENCY_CTRL_INFO_t *pConInfo  )
{
    U16  ii;
    U8   State = FALSE;
    app_printf("BM seq\ttei\tSend\tResult\n");
    for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(CcoUpgradPoll[ii].Valid == 1&&CcoUpgradPoll[ii].QueryResult == 0)
        {
            pConInfo->taskPolling[ii].Valid = 1;
            pConInfo->taskPolling[ii].Send = 0;
            pConInfo->taskPolling[ii].Result = 0;
            State = TRUE;
            app_printf("%d\t%d\t%d\t%d\t\n"
				,ii,ii+2,pConInfo->taskPolling[ii].Send,pConInfo->taskPolling[ii].Result);
        }
        else
        {
            pConInfo->taskPolling[ii].Valid = 0;
        }
            
    }
    if(State == TRUE);
    return State;
    
}

U8 renew_upgrade_start_ctrl_bitmap(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    U16  ii;
    U8   State = FALSE;
    for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(CcoUpgradPoll[ii].Valid == 1&&pConInfo->taskPolling[ii].Valid == 1)
        {
            CcoUpgradPoll[ii].UpgradeStartSend = pConInfo->taskPolling[ii].Send;
            CcoUpgradPoll[ii].UpgradeStartResult = pConInfo->taskPolling[ii].Result;
            //pConInfo->taskPolling[ii].Send = 0;
            //pConInfo->taskPolling[ii].Result = 0;
            State = TRUE;
        }            
    }
    if(State == TRUE);
    return State;
}
*/

/*
U8 renew_upgrade_Qbitmap_ctrl_bitmap(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    U16  ii;
    U8   State = FALSE;
    for(ii = 0;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(CcoUpgradPoll[ii].Valid == 1&&pConInfo->taskPolling[ii].Valid == 1)
        {
            CcoUpgradPoll[ii].UpgradeBitQuerySend = pConInfo->taskPolling[ii].Send;
            CcoUpgradPoll[ii].UpgradeBitQueryResult = pConInfo->taskPolling[ii].Result;
            //pConInfo->taskPolling[ii].Send = 0;
            //pConInfo->taskPolling[ii].Result = 0;
            State = TRUE;
        }            
    }
    if(State == TRUE);
    return State;
}*/
static U8 upgrade_cco_add_upgrade_start_sucess(U16 Index)
{
    U8   State = FALSE;
    if(Index < MAX_WHITELIST_NUM)
    {
        if(CcoUpgradPoll[Index].Valid == 1)
        {
            CcoUpgradPoll[Index].StartResult = 1;

            State = TRUE;
            app_printf("upgrade_cco_add_upgrade_start_sucess %d \n",Index);
        }            
    }

    return State;
}

static U8 upgrade_cco_sub_upgrade_start_sucess(U16 Index)
{
    U8   State = FALSE;
    if(Index < MAX_WHITELIST_NUM)
    {
        if(CcoUpgradPoll[Index].Valid == 1)
        {
            CcoUpgradPoll[Index].StartResult = 0;

            State = TRUE;
            app_printf("upgrade_cco_sub_upgrade_start_sucess %d \n",Index);
        }            
    }

    return State;
}

static U8 upgrade_cco_add_upgrade_qbitmap_sucess(U16 Index)
{
    U8   State = FALSE;
    if(Index < MAX_WHITELIST_NUM)
    {
        if(CcoUpgradPoll[Index].Valid == 1)
        {
            CcoUpgradPoll[Index].QueryResult = 1;

            State = TRUE;
            app_printf("upgrade_cco_add_upgrade_qbitmap_sucess %d\n",Index);
        }            
    }

    return State;
}

//static U8 add_upgrade_stationinfo_sucess(U16 Index)
//{
//    U8   State = FALSE;
//    if(Index < MAX_WHITELIST_NUM)
//    {
//        if(CcoUpgradPoll[Index].Valid == 1)
//        {
//            CcoUpgradPoll[Index].InfoQueryResult = 1;
//
//            State = TRUE;
//            app_printf("add_upgrade_stationinfo_sucess %d \n",Index);
//        }            
//    }
//
//    return State;
//}

static void upgrade_cco_add_upgrade_rate(U16 Index, U8 persent)
{
	CcoUpgradPoll[Index].UpgradeRate = persent ;
	app_printf("CcoUpgradPoll[%d].UpgradeRate is %d",Index,persent);
}

static U8 upgrade_cco_query_upgrade_start_result(void)
{
     U16 ii ;

     app_printf("upgrade_cco_query_upgrade_start_result");
	 for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	 {
	    if( CcoUpgradPoll[ii].Valid == 1&&CcoUpgradPoll[ii].StartResult != 1)
		{
		    app_printf(" not OK!\n");
		    return FALSE;
        }
     }
     app_printf(" OK!\n");
     return TRUE;
}

static U8 upgrade_cco_query_upgrade_result(void)
{
     U16 ii ;

     app_printf("QueryUpgradeResult");
	 for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	 {
	    if( CcoUpgradPoll[ii].Valid == 1&&CcoUpgradPoll[ii].QueryResult != 1)
		{
		    app_printf(" not OK!\n");
		    return FALSE;
        }
     }
     app_printf(" OK!\n");
     return TRUE;
}

void upgrade_cco_file_trans_cfm_pro(uint8_t Status)
{
	//app_printf("normal file confirm : %d\n",Status);
	U32 NowTime = 0;
    U32 NextTime = 0;
	U16  BradcstInterval;
    if(zc_timer_query(UpgradeSuspendtimer) == TMR_RUNNING)
    {
        NowTime=timer_remain(UpgradeSuspendtimer);
        //升级流程需要拉长
        if(NowTime >= UPGRADE_PLC_TIME_OUT)
    	{
    	    app_printf("remUp: %d\n",NowTime);
    		upgrade_cco_ctrl_timer_modify(NowTime, upgrade_cco_bradcst_evt);
            return;
    	}
    }
    if(zc_timer_query(UpgradeCtrltimer) == TMR_RUNNING)
    {
        NowTime=timer_remain(UpgradeCtrltimer);
    }
    //extern U16 upgrade_cco_get_bradcst_interval();
	BradcstInterval = upgrade_cco_get_bradcst_interval();
    //BradcstInterval = BradcstInterval;
	app_printf("remUp: %d,ival = %d\n",NowTime,BradcstInterval);
    //升级流程需要等待
    if(NowTime >= UPGRADE_PLC_TIME_OUT)
	{
		NextTime = NowTime;
	}
    else
    {
        if(Status == e_SUCCESS||Status == e_NO_ACK)
        {
            if(500 < NowTime && NowTime < UPGRADE_PLC_TIME_OUT)
    		{
    			NextTime = BradcstInterval;
    		}																																																																																																	
    		else
    		{
    		    if(zc_timer_query(UpgradeCtrltimer) == TMR_RUNNING)
                {
    			    timer_stop(UpgradeCtrltimer,TMR_CALLBACK);
                    return;
                }
                else
                {
                    NextTime = BradcstInterval;
                }
                
    		}
        }
        else
        {
            NextTime = 1*1000;
    		app_printf("e_ENQUEUE_FAIL to 1s timer_start(UpgradeCtrltimer)!\n");
        }
    }
    if(Status == e_SUCCESS)
    {
        // 刷新重传次数
    	CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANMITMAXNUM;
    }
    else
    {
        //减少重传次数
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt--;
    }
    //app_printf("NextTime: %d \n",NextTime);
    upgrade_cco_ctrl_timer_modify(NextTime, upgrade_cco_bradcst_evt);
}

static int32_t upgrade_cco_start_entry_list_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    list_head_t *pos,*node;
    multi_trans_t  *mbuf_n;
        
    if(list->nr >= list->thr)
    {
        return -1;
    }

    //app_printf("Entry check.\n");
    //dump_buf(new_list->CnmAddr, 6);

    /*Check for duplicate meter address*/
    list_for_each_safe(pos, node,&list->link)
    {
        mbuf_n = list_entry(pos, multi_trans_t, link);
        //dump_buf(mbuf_n->CnmAddr, 6);
        if(memcmp(mbuf_n->CnmAddr, new_list->CnmAddr, 6) == 0)
        {
            return -2;
        }
    }

    return 0;
}

static void upgrade_cco_start_entry_list_fail(int32_t err_code, void *pTaskPrmt)
{
    app_printf("Entry UpgradeStart fail, err = %d\n", err_code);
    
    //RegisterInfo_t.CrnSlaveIndex++;
    if(err_code == -2)
    {
        //RegisterInfo_t.Timeout = 1000;
        //modify_slave_register_timer(RegisterInfo_t.Timeout);
    }
    else if(err_code == -1)
    {
        //modify_slave_register_timer(0);
    }
	
    return;
}
                 
static void upgrade_cco_start_timeout(void *pTaskPrmt)
{
    
}

static void upgrade_cco_start_result_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    UPGRADE_START_CFM_t  *pUpgradeStartCfm = pUplinkData;
    U8  BrocastMac[6] = { 0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF };
#if defined(STATIC_MASTER)
     
    if(memcmp(pUpgradeStartCfm->SrcMacAddr, BrocastMac , 6) != 0)
    {
       
        app_printf("upgrade_cco_start_result_proc,ID=%d,Src = %02x%02x%02x%02x%02x%02x\n"
            ,pUpgradeStartCfm->UpgradeID
        ,pUpgradeStartCfm->SrcMacAddr[0]
        ,pUpgradeStartCfm->SrcMacAddr[1]
        ,pUpgradeStartCfm->SrcMacAddr[2]
        ,pUpgradeStartCfm->SrcMacAddr[3]
        ,pUpgradeStartCfm->SrcMacAddr[4]
        ,pUpgradeStartCfm->SrcMacAddr[5]);
        
        if(pUpgradeStartCfm->StartResultCode == e_UPGRADESTART_SUCCESS)
        {
            if(multi_trans_add_con_result(&CTRL_UPGRADE_START_Info, pReadTask->StaIndex))
            {

                upgrade_cco_add_upgrade_start_sucess(pReadTask->StaIndex);
            }
        }
        else
        {
            multi_trans_add_con_result(&CTRL_UPGRADE_START_Info, pReadTask->StaIndex);
            app_printf("e_UPGRADE_BUSY\n");
        }
            
                
    }

#endif
    
}

static void upgrade_cco_add_upgrade_start_req(void)
{    
    app_printf("upgrade_cco_add_upgrade_start_req\n");
    
	if(multi_trans_add_task_form_poll(&CTRL_UPGRADE_START_Info))
	{  
        multi_trans_t UpgradeStartReq;

        app_printf("CrnSlaveIndex = %d\n", CTRL_UPGRADE_START_Info.CurrentIndex);
        
        //Get_DeviceList_All(CTRL_UPGRADE_START_Info.CrnSlaveIndex,&GetDeviceListTemp_t);
        dump_buf(CTRL_UPGRADE_START_Info.CrntNodeAdd,6);
    	
    	UpgradeStartReq.lid = UPGRADE_LID;
    	UpgradeStartReq.SrcTEI = 0;
        UpgradeStartReq.DeviceTimeout = 60;//DEVTIMEOUT
        UpgradeStartReq.Framenum = 0;
        //__memcpy(UpgradeStartReq.Addr, AppGw3762Afn14F1State.Addr, MAC_ADDR_LEN);	
        __memcpy(UpgradeStartReq.CnmAddr, CTRL_UPGRADE_START_Info.CrntNodeAdd, MAC_ADDR_LEN);

    	UpgradeStartReq.State = UNEXECUTED;
        
        UpgradeStartReq.SendType = SLAVEREGISTER_LOCK;
        UpgradeStartReq.StaIndex = CTRL_UPGRADE_START_Info.CurrentIndex;     
        UpgradeStartReq.DatagramSeq = 0;
    	UpgradeStartReq.DealySecond = UPGRADE_START_LIST.nr == 0?0:1;
        UpgradeStartReq.ReTransmitCnt = CTRL_UPGRADE_START_Info.ReTransmitCnt;
    	
        UpgradeStartReq.DltNum = 1;
    	UpgradeStartReq.ProtoType = 0;
    	UpgradeStartReq.FrameLen  = 0;
        UpgradeStartReq.EntryListCheck = upgrade_cco_start_entry_list_check;
    	UpgradeStartReq.EntryListfail = upgrade_cco_start_entry_list_fail;
    	UpgradeStartReq.TaskPro = upgrade_cco_start_request;
        UpgradeStartReq.TaskUpPro = upgrade_cco_start_result_proc;
        UpgradeStartReq.TimeoutPro = upgrade_cco_start_timeout;

        UpgradeStartReq.pMultiTransHeader = &UPGRADE_START_LIST;
    	UpgradeStartReq.CRT_timer = upgrade_start_timer;
        multi_trans_put_list(UpgradeStartReq, NULL);
    }
    else
    {
        if(UPGRADE_START_LIST.nr == 0)
        {
            app_printf("UPGRADE_START_LIST.TaskRoundPro\n");
            UPGRADE_START_LIST.TaskRoundPro();
        }
    }
}

static void upgrade_cco_ctrl_upgrade_start_round(void)
{
    multi_trans_ctrl_plc_task_round(&UPGRADE_START_LIST,&CTRL_UPGRADE_START_Info);
}

static void upgrade_cco_query_result_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    UPGRADE_STATUS_QUERY_CFM_t  *pUpgradeQueryCfm = pUplinkData;
                                            
#if defined(STATIC_MASTER)
    if(multi_trans_add_con_result(&CTRL_UPGRADE_QUERY_Info, pReadTask->StaIndex) == TRUE)//(pUpgradeQueryCfm->ValidBlockCount > 0)
    {
        app_printf("upgrade_cco_query_result_proc,ID=%d ,Cfm->SrcMacAddr = ",pUpgradeQueryCfm->UpgradeID);
        dump_buf(pUpgradeQueryCfm->SrcMacAddr,6);
        
        if(pUpgradeQueryCfm->UpgradeID == CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID)
        {

            if(pUpgradeQueryCfm->UpgradeStatus == e_FILE_RCEV_FINISHED || pUpgradeQueryCfm->UpgradeStatus == e_UPGRADE_TEST_RUN)
            {	
                //记录传包全满或者接收完成
                upgrade_cco_add_upgrade_qbitmap_sucess(pReadTask->StaIndex);
                upgrade_cco_add_upgrade_rate(pReadTask->StaIndex,100);
                return;
            }

            if(pUpgradeQueryCfm->ValidBlockCount > (sizeof(FileTransBitMapCal) * 8U))
            {
                app_printf("query res validblockcount=0x%04X error!\n", pUpgradeQueryCfm->ValidBlockCount);
                return;
            }

            memset(FileTransBitMapCal, 0x00, MAX_UPGRADE_FILE_SIZE/FILE_TRANS_BLOCK_SIZE/8);
            
            //合并位图,合法包不操作，非1包需要重传
            uint16_t i = 0;

            for(i=0; i<pUpgradeQueryCfm->ValidBlockCount; i++)
            {
                if(pUpgradeQueryCfm->BlockInfoBitMap[i/8] & (0x01 << (i % 8)))
                {
                    //统计当前节点成功数
                    bitmap_set_bit(FileTransBitMapCal,i);
                }
                else
                {
                    bitmap_clr_bit(FileTransBitMapCal,i);
                }
            }
            //app_printf("FileTransBitMapCal");
            //dump_buf(FileTransBitMapCal, MAX_UPGRADE_FILE_SIZE/FILE_TRANS_BLOCK_SIZE/8);
            U32  SuccessNum;
            U8   SuccessPersent;
            SuccessNum = 0;
            SuccessPersent = 0;
            SuccessNum = bitmap_get_one_nr(FileTransBitMapCal, sizeof(FileTransBitMapCal));
            SuccessPersent = ((SuccessNum == 0)?0:(SuccessNum*100/CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum));
			app_printf("total : %d, successnum : %d\n",CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum,SuccessNum);
			app_printf("UpgradeStatus = %d,ValidBlockCount = %d,SuccessPersent = %d\n"
                ,pUpgradeQueryCfm->UpgradeStatus,pUpgradeQueryCfm->ValidBlockCount,SuccessPersent);
            U8 MacAddr[6];
    	    DEVICE_TEI_LIST_t DeviceListTemp;
            __memcpy(MacAddr,pUpgradeQueryCfm->SrcMacAddr,6);
            ChangeMacAddrOrder(MacAddr);
            if(DeviceList_ioctl(DEV_GET_ALL_BYMAC, MacAddr, &DeviceListTemp)== TRUE)
            {
                if(SuccessPersent>0)
                {
                    if((IsCjqfile == e_JZQ_TASK)||(IsCjqfile == e_SELF_ONE_TASK)||
                        ((IsCjqfile == e_SELF_STA_TASK||IsCjqfile == e_SELF_CJQ_TASK)&&
                        ((CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum == UPGRADEMAXTIMES-1&&SuccessPersent>30)
                        ||(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum == UPGRADEMAXTIMES-2&&SuccessPersent>20)
                        ||(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum <= UPGRADEMAXTIMES-3&&SuccessPersent>10))))
                    {
                        upgrade_cco_add_upgrade_rate(DeviceListTemp.NodeTEI-2,SuccessPersent);
                        //仅统计失败表号
                        for(i=0; i<pUpgradeQueryCfm->ValidBlockCount; i++)
                        {
                            if(pUpgradeQueryCfm->BlockInfoBitMap[i/8] & (0x01 << (i % 8)))
                            {
                            	
                            }
                            else
                            {
                                //失败包序号做取并集处理
                                bitmap_clr_bit(FileTransBitMap,i);
                            }
                        }
                        //app_printf("FileTransBitMap");
                        //dump_buf(FileTransBitMap, MAX_UPGRADE_FILE_SIZE/FILE_TRANS_BLOCK_SIZE/8);
                        if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus == EVT_UPGRADE_QUERY)
                        {
                            U16 MaxNum;
                            //最大白名单数量除以20，作为未收全触发节点
                            MaxNum = (FlashInfo_t.usJZQRecordNum==0?20:FlashInfo_t.usJZQRecordNum/20);
                            MaxNum = MAX(MaxNum , 20);
                            if(SuccessPersent!=100)
                            {
                                CcoUpgrdCtrlInfo.UpgrdStatusInfo.NeedSendNum ++;
                            }
                            if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.NeedSendNum > MaxNum)
                            {
                                //如果连续发现20个节点需要补包，先补包
                                CcoUpgrdCtrlInfo.UpgrdStatusInfo.NeedSendNum = 0;
                                CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_FILETRANS_SEND;
                                multi_trans_stop_plc_task((void *)&UPGRADE_QUERY_LIST);

                                app_printf("need send num =%d ,send bin now!\n",CcoUpgrdCtrlInfo.UpgrdStatusInfo.NeedSendNum);
                            }
                        }
                        
                    }
                }
                
                if(pUpgradeQueryCfm->UpgradeStatus == e_UPGRADE_IDLE&&SuccessNum ==0)
                {
                    upgrade_cco_sub_upgrade_start_sucess(DeviceListTemp.NodeTEI-2);
                }
                if(pUpgradeQueryCfm->UpgradeStatus == e_FILE_RCEV_FINISHED ||
    		    pUpgradeQueryCfm->UpgradeStatus == e_UPGRADE_TEST_RUN ||
    		    CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum == SuccessNum)
                {	
                    //记录传包全满或者接收完成
                    upgrade_cco_add_upgrade_qbitmap_sucess(pReadTask->StaIndex);
                }
            }
        }
    }
    else
    {
        //UpgradeStartReult is not OK!
    }
#endif
}

static void upgrade_cco_add_upgrade_query_req(void)
{
    app_printf("upgrade_cco_add_upgrade_query_req\n");
    
	if(multi_trans_add_task_form_poll(&CTRL_UPGRADE_QUERY_Info) )
	{  
        multi_trans_t UpgradeQueryReq;

        app_printf("upgrade_cco_add_upgrade_query_req,CrnSlaveIndex = %d\n", CTRL_UPGRADE_QUERY_Info.CurrentIndex);
      
        dump_buf(CTRL_UPGRADE_QUERY_Info.CrntNodeAdd,6);
    	
    	UpgradeQueryReq.lid = UPGRADE_LID;
    	UpgradeQueryReq.SrcTEI = 0;
        UpgradeQueryReq.DeviceTimeout = DEVTIMEOUT;
    	
        __memcpy(UpgradeQueryReq.CnmAddr, CTRL_UPGRADE_QUERY_Info.CrntNodeAdd, MAC_ADDR_LEN);

    	UpgradeQueryReq.State = UNEXECUTED;
        
        UpgradeQueryReq.SendType = SLAVEREGISTER_LOCK;
        UpgradeQueryReq.StaIndex = CTRL_UPGRADE_QUERY_Info.CurrentIndex;     
        UpgradeQueryReq.DatagramSeq = 0;
    	UpgradeQueryReq.DealySecond = UPGRADE_QUERY_LIST.nr == 0?0:1;
        UpgradeQueryReq.ReTransmitCnt = CTRL_UPGRADE_QUERY_Info.ReTransmitCnt;
    	
        UpgradeQueryReq.DltNum = 1;
    	UpgradeQueryReq.ProtoType = 0;
    	UpgradeQueryReq.FrameLen  = 0;
        UpgradeQueryReq.EntryListCheck = upgrade_cco_start_entry_list_check;
    	UpgradeQueryReq.EntryListfail = upgrade_cco_start_entry_list_fail;
    	UpgradeQueryReq.TaskPro = upgrade_cco_query_request;
        UpgradeQueryReq.TaskUpPro = upgrade_cco_query_result_proc;
        UpgradeQueryReq.TimeoutPro = upgrade_cco_start_timeout;

        UpgradeQueryReq.pMultiTransHeader = &UPGRADE_QUERY_LIST;
    	UpgradeQueryReq.CRT_timer = upgrade_query_timer;
        multi_trans_put_list(UpgradeQueryReq, NULL);
        //multi_trans_entry_list;//如果链表刷新过程中需要入队任务，是否可以直接使用入队接口，无需post
    }
    else
    {
        if(UPGRADE_QUERY_LIST.nr == 0)
        {
            app_printf("UPGRADE_QUERY_LIST.TaskRoundPro\n");
            UPGRADE_QUERY_LIST.TaskRoundPro();
        }
    }
}

static void upgrade_cco_ctrl_upgrade_query_round(void)
{
    if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus == EVT_FILETRANS_SEND)
    {
        //停止查询
        app_printf("stop UPGRADE_QUERY!\n");
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeQueryNum =-1;

        app_printf("early EVT_UPGRADE_QUERY>>>EVT_FILETRANS_SEND\n");
        //其他轮次载波补包
        upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_bradcst_evt);
    }
    multi_trans_ctrl_plc_task_round(&UPGRADE_QUERY_LIST,&CTRL_UPGRADE_QUERY_Info);
}

//static void UpgradeStationInfoResultProc(void *pTaskPrmt, void *pUplinkData)
//{
//    multi_trans_t  *pReadTask = pTaskPrmt;
//    STATION_INFO_QUERY_CFM_t  *pUpgradeStationInfoCfm = pUplinkData;
//    U8  BrocastMac[6] = { 0xFF , 0xFF , 0xFF , 0xFF , 0xFF , 0xFF };
//#if defined(STATIC_MASTER)
//     
//    if(memcmp(pUpgradeStationInfoCfm->SrcMacAddr, BrocastMac , 6) != 0)
//    {
//       
//        app_printf("UpgradeStationInfoResultProc,pUpgradeStartCfm->SrcMacAddr = %02x%02x%02x%02x%02x%02x\n"
//        ,pUpgradeStationInfoCfm->SrcMacAddr[0]
//        ,pUpgradeStationInfoCfm->SrcMacAddr[1]
//        ,pUpgradeStationInfoCfm->SrcMacAddr[2]
//        ,pUpgradeStationInfoCfm->SrcMacAddr[3]
//        ,pUpgradeStationInfoCfm->SrcMacAddr[4]
//        ,pUpgradeStationInfoCfm->SrcMacAddr[5]);
//
//        if(multi_trans_add_con_result(&CTRL_UPGRADE_STATION_INFO_QUERY_Info, pReadTask->StaIndex))
//        {
//
//            add_upgrade_stationinfo_sucess(pReadTask->StaIndex);
//        }
//        
//    }
//#endif
//}

//static void AddUpgradeStationInfoReq(void)
//{    
//    app_printf("AddUpgradeStationInfoReq\n");
//    
//	if(multi_trans_add_task_form_poll(&CTRL_UPGRADE_STATION_INFO_QUERY_Info))
//	{  
//        multi_trans_t UpgradeStationInfoReq;
//
//        app_printf("CrnSlaveIndex = %d\n", CTRL_UPGRADE_STATION_INFO_QUERY_Info.CurrentIndex);
//        
//        //Get_DeviceList_All(CTRL_UPGRADE_START_Info.CrnSlaveIndex,&GetDeviceListTemp_t);
//        dump_buf(CTRL_UPGRADE_STATION_INFO_QUERY_Info.CrntNodeAdd,6);
//    	
//    	UpgradeStationInfoReq.lid = UPGRADE_LID;
//    	UpgradeStationInfoReq.SrcTEI = 0;
//        UpgradeStationInfoReq.DeviceTimeout = DEVTIMEOUT;
//        UpgradeStationInfoReq.Framenum = 0;
//        //__memcpy(UpgradeStartReq.Addr, AppGw3762Afn14F1State.Addr, MAC_ADDR_LEN);	
//        __memcpy(UpgradeStationInfoReq.CnmAddr, CTRL_UPGRADE_STATION_INFO_QUERY_Info.CrntNodeAdd, MAC_ADDR_LEN);
//
//    	UpgradeStationInfoReq.State = UNEXECUTED;
//        
//        UpgradeStationInfoReq.SendType = SLAVEREGISTER_LOCK;
//        UpgradeStationInfoReq.StaIndex = CTRL_UPGRADE_STATION_INFO_QUERY_Info.CurrentIndex;     
//        UpgradeStationInfoReq.DatagramSeq = 0;
//    	UpgradeStationInfoReq.DealySecond = UPGRADE_STATION_INFO_QUERY_LIST.nr == 0?0:1;
//        UpgradeStationInfoReq.ReTransmitCnt = CTRL_UPGRADE_QUERY_Info.ReTransmitCnt;
//    	
//        UpgradeStationInfoReq.DltNum = 1;
//    	UpgradeStationInfoReq.ProtoType = 0;
//    	UpgradeStationInfoReq.FrameLen  = 0;
//        UpgradeStationInfoReq.EntryListCheck = upgrade_cco_start_entry_list_check;
//    	UpgradeStationInfoReq.EntryListfail = upgrade_cco_start_entry_list_fail;
//    	UpgradeStationInfoReq.TaskPro = upgrade_cco_station_info_request; 
//        UpgradeStationInfoReq.TaskUpPro = UpgradeStationInfoResultProc;
//        UpgradeStationInfoReq.TimeoutPro = upgrade_cco_start_timeout;
//
//        UpgradeStationInfoReq.pMultiTransHeader = &UPGRADE_STATION_INFO_QUERY_LIST;
//    	UpgradeStationInfoReq.CRT_timer = upgrade_station_info_query_timer;
//        multi_trans_put_list(UpgradeStationInfoReq, NULL);
//    }
//    else
//    {
//        app_printf("UPGRADE_STATION_INFO_QUERY_LIST.TaskRoundPro\n");
//        UPGRADE_STATION_INFO_QUERY_LIST.TaskRoundPro();
//    }
//}

//static void CtrlUpgradeStationInfoRound(void)
//{
//    multi_trans_ctrl_plc_task_round(&UPGRADE_STATION_INFO_QUERY_LIST,&CTRL_UPGRADE_STATION_INFO_QUERY_Info);
//}

static void upgrade_cco_set_upgrade_bitmap(void)
{
   memset(FileTransBitMap,0x00,sizeof(FileTransBitMap));
}

//升级使用
static U16 upgrade_cco_query_upgrade_info_from_net(void)
{
	U16 ii ;
    U8  ManufactorCode[2] = {'C','Z'};
    DEVICE_TEI_LIST_t tempdevicelist;

    __memcpy(ManufactorCode,FlashInfo_t.ManuFactor_t.ucVendorCode,2);
	CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrNodeTotalNum = 0;
	app_printf("seq\ttei\tStartR\tQbitmR\tQinfoR\tPerfR\tStopR\tRate\n");
	for(ii = (IsCjqfile==e_SELF_ONE_TASK?UpgradeTEI:0);ii<MAX_WHITELIST_NUM;ii++)
	{
		if( DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &tempdevicelist) &&(tempdevicelist.NodeTEI != 0xFFF))
		{
		    if(IsCjqfile == e_JZQ_TASK)
            {
                //标准升级流程
            }
            else if((IsCjqfile == e_SELF_STA_TASK&&(tempdevicelist.DeviceType==e_METER||tempdevicelist.DeviceType==e_3PMETER))
                ||(IsCjqfile == e_SELF_CJQ_TASK&&tempdevicelist.DeviceType==e_CJQ_2)
                ||(IsCjqfile == e_SELF_ONE_TASK))
            {
                if((memcmp(&tempdevicelist.ManufactorCode,ManufactorCode,2)==0)
                    &&(memcmp(CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer,tempdevicelist.ManufactorInfo.InnerVer,2 )!=0))
                {
                    //内部升级,仅支持区分类型,单点升级
                }
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
			CcoUpgradPoll[ii].Valid = 0x01;
			CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrNodeTotalNum++;
		}
		else
		{
			memset(&CcoUpgradPoll[ii] , 0x00 ,sizeof(CCO_UPGRADE_POLL));
		}
	}
	app_printf("-------upgrade_cco_query_upgrade_info_from_net,CurrNodeTotalNum=%d\n",CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrNodeTotalNum);
    return CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrNodeTotalNum;
}

static void upgrade_cco_start_upgrade_query_evt(void)
{
    upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_query_evt);
}

static void upgrade_cco_start_upgrade_start_evt(void)
{
    upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_start_renew_evt);
}

static void upgrade_cco_start_all_evt(void)
{
    app_printf("upgrade_cco_start_all_evt \n");
   
    upgrade_cco_set_upgrade_down_bitmap(&CTRL_UPGRADE_START_Info,e_UPGRADE_START_TASK);
    UPGRADE_START_LIST.TaskAddOne = upgrade_cco_add_upgrade_start_req;
    UPGRADE_START_LIST.TaskRoundPro = upgrade_cco_ctrl_upgrade_start_round;
    
    CTRL_UPGRADE_START_Info.lid = UPGRADE_LID;
    CTRL_UPGRADE_START_Info.CurrentIndex = 0;
    CTRL_UPGRADE_START_Info.ReTransmitCnt = 2;
    CTRL_UPGRADE_START_Info.DealySecond = 20;
    CTRL_UPGRADE_START_Info.CycleNum = 1;

    CTRL_UPGRADE_START_Info.TaskNext = upgrade_cco_start_upgrade_start_evt;
    
    upgrade_cco_add_upgrade_start_req();//添加第一条并发任务

    //ctrl_plc_multi_task_list_all_add(&UPGRADE_START_MULTI_TASK_LIST);
    //modify_plc_upgrade_start_timer(300);
    
}

static void upgrade_cco_query_all_evt(void)
{
    app_printf("upgrade_cco_query_all_evt \n");
    //首次更新查询位图
    //if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeQueryNum == RETRANUPGRADEMAXNUM-1)
    {
        //set_upgrade_Qbitmap_ctrl_bitmap(&CTRL_UPGRADE_QUERY_Info);
        upgrade_cco_set_upgrade_down_bitmap(&CTRL_UPGRADE_QUERY_Info,e_UPGRADE_QUERY_TASK);
    }
    
    UPGRADE_QUERY_LIST.TaskAddOne = upgrade_cco_add_upgrade_query_req;
    UPGRADE_QUERY_LIST.TaskRoundPro = upgrade_cco_ctrl_upgrade_query_round;
    
    CTRL_UPGRADE_QUERY_Info.lid = UPGRADE_LID;
    CTRL_UPGRADE_QUERY_Info.CurrentIndex = 0;
    CTRL_UPGRADE_QUERY_Info.ReTransmitCnt = 1;
    CTRL_UPGRADE_QUERY_Info.DealySecond = 20;
    CTRL_UPGRADE_QUERY_Info.CycleNum = 1;

    CTRL_UPGRADE_QUERY_Info.TaskNext = upgrade_cco_start_upgrade_query_evt;
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_UPGRADE_QUERY;
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.NeedSendNum = 0;

    upgrade_cco_add_upgrade_query_req();
    //ctrl_plc_multi_task_list_all_add(&UPGRADE_QUERY_MULTI_TASK_LIST);
    //modify_plc_upgrade_query_timer(300);
}

static void upgrade_cco_query_evt(work_t *work)
{
    app_printf("upgrade_cco_query_evt \n");

    CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeQueryNum --;
	if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeQueryNum < 0)
    {
        app_printf("EVT_UPGRADE_QUERY>>>EVT_FILETRANS_SEND\n");
        //其他轮次载波补包
        upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_bradcst_evt);
    }
    else
    {
        upgrade_cco_query_all_evt();
    }
}

//static void upgrade_STAinfo_evt(work_t *work)
//{
//    
//    
//}

static void upgrade_cco_stop_evt(work_t *work)
{
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt --;
	if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt < 0)
    {
		AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;//释放升级状态
        AppGw3762Afn10F4State.RunStatusWorking  = 0 ;
		CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_UPGRADE_IDLE ;
        timer_stop(ReUpgradetimer,TMR_NULL);
	    timer_stop(UpgradeCtrltimer,TMR_NULL);
        IsCjqfile = e_JZQ_TASK;
		return ;
	}
	else
	{
        upgrade_cco_stop_req_aps();
        //超时重发，需要3秒间隔
        upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_stop_evt);

	}
    
}

static void upgrade_cco_perform_evt(work_t *work)
{
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt --;
	if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt < 0)
    {
		AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;//释放升级状态
        AppGw3762Afn10F4State.RunStatusWorking  = 0 ;
		CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_UPGRADE_IDLE ;
        timer_stop(ReUpgradetimer,TMR_NULL);
	    timer_stop(UpgradeCtrltimer,TMR_NULL);
        IsCjqfile = e_JZQ_TASK;
        upgrade_cco_time_out_modify(UPGRADE_PERFORM_TIME);
		return ;
	}
	else
	{
    	upgrade_cco_test_run_req_aps();
        //超时重发，需要3秒间隔
        upgrade_cco_ctrl_timer_modify(3000, upgrade_cco_perform_evt);
        
	}
    
}

static void upgrade_cco_bradcst_evt(work_t *work)
{
    app_printf("upffevt ");
    
    S16 Index = -1;  
    U32  size = 0;
    
    size = (CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum/8);			
    if(CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum % 8)
    {
    	size += 1;
    }
    //app_printf("%d-%d-%d-%d-%d-%d  \n",YEAR,MONTH,DATE,HOUR,MINUTE,SECOND);
    Index = bitmap_find_least_zero(FileTransBitMap,size);
    app_printf("size= %d Index= %d(%d)\n",size,Index, CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum);
    if((Index == -1) ||(Index >= CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum))
    {
        app_printf("EVT_FILETRANS_SEND>>>EVT_UPGRADE_START\n");
        //升级总轮次--
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum--;
        
        //全部发完，切换至发送开始升级及位图查询,然后继续补包
        upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_start_renew_evt);
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeStartNum = RETRANUPGRADEMAXNUM;
		CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeQueryNum = 1;
       
    }
    else
    {
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex = Index;
	    CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANMITMAXNUM;
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_FILETRANS_BRADCST ;
		bitmap_set_bit(&FileTransBitMap,Index);
		
		if(Upgrade_filetrans_bradcst == 1)
		{
			CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT;
			
		}
		else
		{
			CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = 20;
			
		}
        CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT;
        upgrade_cco_file_trans_request(e_FULL_NET_BROADCAST);  //全网广播

        upgrade_cco_ctrl_timer_modify(2000, upgrade_cco_bradcst_evt);//启动广播发送超时控制
    }
}

static void upgrade_cco_start_renew_evt(work_t *work)
{
    app_printf("upgrade_cco_start_renew_evt \n");
    U8  MaxUpgradeStartnum;   
    if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum == UPGRADEMAXTIMES)
    {
        MaxUpgradeStartnum = 0;
    }
    else
    {
        MaxUpgradeStartnum = RETRANUPGRADEMAXNUM-1;
    }
    if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeStartNum <= MaxUpgradeStartnum||
        (CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeStartNum < RETRANUPGRADEMAXNUM && upgrade_cco_query_upgrade_start_result() == TRUE))
    {
        
        if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum == UPGRADEMAXTIMES)
        {
            //首轮发全部升级文件
            upgrade_cco_ctrl_timer_modify(5000, upgrade_cco_bradcst_evt);
        }
        else
        {
            //其他轮次直接查询补包
            upgrade_cco_ctrl_timer_modify(6000, upgrade_cco_query_evt);
        }
    }
    else
    {
        upgrade_cco_query_upgrade_info_from_net();
        if(CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum <= 0||(upgrade_cco_query_upgrade_result() == TRUE))
        {
            //最后一轮执行升级
            upgrade_cco_ctrl_timer_modify(6000, upgrade_cco_perform_evt);
        }
        else
        {
            upgrade_cco_start_all_evt();
        }
    }
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeStartNum -- ;
    //__memcpy(worknext->buf , work->buf,sizeof(plc_multi_task_header_t));//错误示范	
}

void upgrade_cco_reset_evt(void)
{
    app_printf("upgrade_cco_reset_evt \n");

    CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTimeout = UPGRADE_TIMEOUT;
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrentSlaveIndex = 0;
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANUPGRADEMAXNUM;
    CcoUpgrdCtrlInfo.UpgrdStatusInfo.DealySecond = 20;
    
	CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeStartNum = RETRANUPGRADEMAXNUM;//并发模式查询开始升级5次
	CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeQueryNum = RETRANUPGRADEMAXNUM;//并发模式查询位图5次
	CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeTotalNum = UPGRADEMAXTIMES;//升级总轮次5次
	
    upgrade_cco_set_upgrade_bitmap();//首次上电需要初始化发送位图;
    upgrade_cco_ctrl_timer_init();
    memset(CcoUpgradPoll,0x00,sizeof(CcoUpgradPoll));
    memset((U8 *)&CTRL_UPGRADE_START_Info,0x00,sizeof(CTRL_UPGRADE_START_Info));
    memset((U8 *)&CTRL_UPGRADE_QUERY_Info,0x00,sizeof(CTRL_UPGRADE_QUERY_Info));
    upgrade_cco_ctrl_timer_modify(2000, upgrade_cco_start_renew_evt);
    upgrade_cco_time_out_modify(1000);
    timer_stop(UpgradeTimeOuttimer,TMR_NULL);
}

void upgrade_cco_stop(void)
{
    app_printf("upgrade_cco_stop \n");

    CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANUPGRADEMAXNUM;
    multi_trans_stop_plc_task((void *)&UPGRADE_START_LIST);
    multi_trans_stop_plc_task((void *)&UPGRADE_QUERY_LIST);
    timer_stop(UpgradeCtrltimer,TMR_NULL);
    upgrade_cco_ctrl_timer_modify(2000, upgrade_cco_stop_evt);//停止升级广播发送超时控制
}

static void upgrade_cco_ctrl_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    //post;
    work_t *worknext = zc_malloc_mem(sizeof(work_t),"UPGRDCTL",MEM_TIME_OUT);
    worknext->work = arg;
	worknext->msgtype = UPGRDCTL;
    post_app_work(worknext);

    //upgrade_cco_ctrl_timer_modify(6*GUARD_MULTI_TIME,arg);
}

void upgrade_cco_ctrl_timer_modify(uint32_t Ms, void *arg)
{
    if(zc_timer_query(UpgradeSuspendtimer) == TMR_RUNNING)
    {
        U32  times = 0;
        //最大拉长30秒，可根据效率调整
        times = MIN(timer_remain(UpgradeSuspendtimer),30*1000);
        Ms += times;
        app_printf("times = %d ,Ms = %d ms\n",times,Ms);
    }
    timer_stop(UpgradeCtrltimer,TMR_NULL);
	timer_modify(UpgradeCtrltimer,
                    Ms,
					0,
					TMR_TRIGGER ,//TMR_TRIGGER
					upgrade_cco_ctrl_timer_cb,
					arg,
                    "upgrade_cco_ctrl_timer_cb",
                    TRUE);
	timer_start(UpgradeCtrltimer);
}

static void upgrade_cco_repgrade_timer_cb(struct ostimer_s *ostimer, void *arg)
{			
    if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG )
    {
        
        app_printf("upgrade_cco_repgrade_timer_cb Time out ,need clear WorkSwitchStates!\n");
        AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;//释放CCO升级状态
        AppGw3762Afn10F4State.RunStatusWorking  = 0 ;
        LastblockID = 0xFFFFFFFF;;
		return ;
    }
    else
    {
        app_printf("upgrade_cco_repgrade_timer_cb Time out,but nothing!\n");
    }
}

static U8  upgrade_cco_net_woking_percent(void)
{
    U16 OnlineNum = 0;
    
    OnlineNum = APP_GETDEVNUM();
    if(OnlineNum == 0||FlashInfo_t.usJZQRecordNum == 0)
    {
        return 0;
    }
    else
    {
        return (OnlineNum*100/FlashInfo_t.usJZQRecordNum);
    }
}

static void upgrade_cco_auto_upgrade_proc(work_t *work)
{
    U8  NetwokingPercentnum = 0;
	U16  UpgradeNum = 0;
    static U16  TestRunCnt = 0;
    
    if(UpgradeBySelfState == AUTO_UPGRADE_LOAD_BIN_OK )
    {
        app_printf("UpgradeBySelfState=%s\n",UpgradeBySelfState == AUTO_UPGRADE_IDLE?"AUTO_UPGRADE_IDLE":
                                         UpgradeBySelfState == AUTO_UPGRADE_LOAD_BIN_OK?"AUTO_UPGRADE_LOAD_BIN_OK":
                                         UpgradeBySelfState == AUTO_UPGRADE_NEED_START?"AUTO_UPGRADE_NEED_START":
                                         UpgradeBySelfState == AUTO_UPGRADE_NEED_RUNNING?"AUTO_UPGRADE_NEED_RUNNING":
                                         UpgradeBySelfState == AUTO_UPGRADE_FINISH?"AUTO_UPGRADE_FINISH":
                                         UpgradeBySelfState == AUTO_UPGRADE_CHECK_OK?"AUTO_UPGRADE_CHECK_OK":"unknow");
        NetwokingPercentnum = upgrade_cco_net_woking_percent();
        UpgradeNum = upgrade_cco_query_upgrade_info_from_net();
        app_printf("NetwokingPercentnum = %d , UpgradeNum = %d###\n",NetwokingPercentnum,UpgradeNum);
        //白天空闲时间，并且组网完成度超过95%，或者组网时间超过60分钟，优先开始
        //if(SysDate.Year > 19&&SysDate.Hour > 8)
        {
            if((NetwokingPercentnum >95 || SystenRunTime > (60*60))&&(UpgradeNum > 0)&&(zc_timer_query(UpgradeCtrltimer) == TMR_STOPPED))
            {
                UpgradeBySelfState = AUTO_UPGRADE_NEED_START;
                
                upgrade_cco_stop_req_aps(); //强制清除升级状态
                
                CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex = 0;

                //memset(FileTransBitMap, 0x00, CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockBitMapLen);
                memset(CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrSlaveAddr, 0xFF, 6);
                CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID = 1 ;//++CurrUpgradeID;
                CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANMITMAXNUM;
                //AppGw3762Afn10F4State.WorkSwitchStates = CARRIER_UPGRADE_FLAG;  //
                CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex = 0;

                //自动开始升级
                CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_UPGRADE_RESET ;
                CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT*6;
                upgrade_cco_reset_evt();
            }
        } 
    }
    else
    {
        app_printf("upgrade_cco_repgrade_timer_cb Time out,but nothing!\n");
    }
    if(UpgradeBySelfState == AUTO_UPGRADE_FINISH )
    {
        TestRunCnt ++;
        if(TestRunCnt > 6)
        {
            UpgradeNum = upgrade_cco_query_upgrade_info_from_net();
            TestRunCnt = 0;
            if(UpgradeNum > 0)
            {
                UpgradeBySelfState = AUTO_UPGRADE_LOAD_BIN_OK;
            }
            else
            {
                UpgradeBySelfState = AUTO_UPGRADE_CHECK_OK;
                app_printf("The UpgradeBySelfState is AUTO_UPGRADE_CHECK_OK!\n");
            }
        }
    }
    timer_start(AutoUpgradetimer);
}

static void upgrade_cco_auto_upgrade_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    work_t *worknext = zc_malloc_mem(sizeof(work_t),"AUTOUP",MEM_TIME_OUT);
    worknext->work = upgrade_cco_auto_upgrade_proc;
    worknext->msgtype = UPGRDCTL;
    post_app_work(worknext);
}

static void upgrade_cco_auto_upgrade_timer_modify(uint32_t Ms)
{
    if(AutoUpgradetimer == NULL)
    {
        AutoUpgradetimer = timer_create(Ms,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            upgrade_cco_auto_upgrade_timer_cb,
                            NULL,
                            "upgrade_cco_auto_upgrade_timer_cb"
                           );
    }
    else
    {
	    timer_modify(AutoUpgradetimer,
                    Ms,
					0,
					TMR_TRIGGER ,//TMR_TRIGGER
                    upgrade_cco_auto_upgrade_timer_cb,
					NULL,
                    "upgrade_cco_auto_upgrade_timer_cb",
                    TRUE);
    }
	timer_start(AutoUpgradetimer);
}

void upgrade_cco_read_upgrade_info(U8* BinType)
{
    U32   crcValue;
    U32   Crc32Data;
    
    CcoLoadSlaveImage1();
    imghdr_t   *pImghdr = (imghdr_t*)&FileUpgradeData;
    CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize = pImghdr->sz_file;
    CcoUpgrdCtrlInfo.UpgrdFileInfo.FileCrc=pImghdr->crc;
    DstImageBaseAddr = IMAGE1_ADDR;//+IMAGE_RESERVE;
    ImageFileSize = CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize;
    crcValue = crc_digest((unsigned char *)FileUpgradeData, ImageFileSize, (CRC_32 | CRC_SW), &Crc32Data);
    app_printf("crcValue = %08x, Crc32Data = %08x\n", crcValue, Crc32Data);

    CcoUpgrdCtrlInfo.UpgrdFileInfo.FileCrc  = Crc32Data;
    CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize = FILE_TRANS_BLOCK_SIZE;
    CcoUpgrdCtrlInfo.UpgrdFileInfo.ImageBaseAddr = DstImageBaseAddr;
    CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum = (CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize/FILE_TRANS_BLOCK_SIZE);
    if(CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize % FILE_TRANS_BLOCK_SIZE)
    {
        CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum += 1;
    }
    	   
    CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockBitMapLen = (CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum/8);
    if(CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum % 8)
    {
         CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockBitMapLen += 1;
    }	
    
    *BinType = check_image_type((U32)FileUpgradeData, CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize);
    CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[0] = pImghdr->version[1];
    CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[1] = pImghdr->version[0];
    
    app_printf("Software version is--- %02x%02x ---\n\n", pImghdr->version[0], pImghdr->version[1]);
    app_printf("bin is %c\n", *BinType);
}

void upgrade_cco_start_upgrade_byself(void)
{
	CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID = 1;

    if(TRUE == app_ext_info.func_switch.UpgradeMode)
	{
	    U8    BinType = 0;
        
        upgrade_cco_read_upgrade_info(&BinType);
        if(BinType == ZC3750STA_type || 
			BinType == ZC3750CJQ2_type || 
			BinType == ZC3750STA_UpDate_type || 
			BinType == ZC3750CJQ2_UpDate_type)
        {
            app_printf("Crc check is OK.\n");
            
            app_printf("Software version is %02x%02x\n", 
                CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[1], 
                CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[0]);
            
            IsCjqfile = ((BinType == ZC3750STA_type || BinType == ZC3750STA_UpDate_type)?e_SELF_STA_TASK:
                (BinType == ZC3750CJQ2_type || BinType == ZC3750CJQ2_UpDate_type)?e_SELF_CJQ_TASK:e_JZQ_TASK);
                
            UpgradeBySelfState = AUTO_UPGRADE_LOAD_BIN_OK;
            upgrade_cco_auto_upgrade_timer_modify(300*1000);
        }
        else
        {
            UpgradeBySelfState = AUTO_UPGRADE_IDLE;
        }
	}
    else
    {
        app_printf("app_ext_info.func_switch.UpgradeMode == FALSE\n");
    }
}

U32 upgrade_cco_check_upgrade_data(U8 *databuf, U16 buflen)
{
    U16 rcvLen = 0;
	U8  fileTransId, fileAttribute, fileCmd;
	U16  totolBlockNum;
	U16  blockID;
	U32  offsetAddr;
	U16   blockLen = 0;
	static U16 BlockSize = 0;

	U8 *pBlockData = NULL;

	fileTransId  = databuf[rcvLen++];
	fileAttribute = databuf[rcvLen++];
	fileCmd       = databuf[rcvLen++];

	totolBlockNum = (U16)(databuf[rcvLen]|(databuf[rcvLen+1]<<8));//*(U16*)&databuf[rcvLen];
	rcvLen += 2;
	blockID = (U32)(databuf[rcvLen]|(databuf[rcvLen+1]<<8)|(databuf[rcvLen+2]<<16)|(databuf[rcvLen+3]<<24));//*(U32*)&databuf[rcvLen];
	rcvLen += 4;
	blockLen     = (U16)(databuf[rcvLen]|(databuf[rcvLen+1]<<8));//*(U16*)&databuf[rcvLen];
	rcvLen += 2;
    app_printf("totolBlockNum =%d,blockID =%d,blockLen =%d\n",totolBlockNum,blockID,blockLen);
    if(fileTransId == 0)
    {
        LblockID = 0;
    }
    //刷新传文件保护
    timer_start(ReUpgradetimer);
    
    if(fileAttribute == 0x00 && blockID == 0x00)
    {
        //辽宁台体，接收到15F1，停止CCO请求集中器时钟定时器及不上报03F10
        //68 1a 00 43 00 00 00 00 00 00 15 01 00 00 00 00 00 00 00 00 00 00 00 00 59 16 
        if((0U == totolBlockNum) && (0U == blockLen))
        {
            extern U8  send_03f10_flag;
            send_03f10_flag = 1U;
            if(NULL != clockMaintaintimer)
            {
                timer_stop(clockMaintaintimer, TMR_STOPPED);
                app_printf("recv test, 15f1, stop req jzq timer and not send 03f10!\n");
            }
        }

        BlockSize = blockLen;
        
        __memcpy(FileUpgradeData, (databuf+rcvLen), blockLen);
        imghdr_t *ih = (imghdr_t *)FileUpgradeData;
        
        if(fileTransId == APP_GW3762_LOCAL_MODULE_UPDATE)
        {
            //如果不是自己image帧头，不接收
            if (!is_tri_image(ih))
            {
                
                return 0xFFFFFFFF;
            }
        }
        else if(fileTransId == APP_GW3762_SLAVE_MODULE_UPDATE || fileTransId == APP_GW3762_SLAVE_CJQ_UPGRADE_START)
        {

        }
        LastblockID = 0;
    }
    
    offsetAddr = blockID*BlockSize;
    if(offsetAddr > (MAX_UPGRADE_FILE_SIZE-BlockSize))
    {
        
        return 0xFFFFFFFF;
    }

    if(fileCmd != 0x00)
    {
           ;
    }
    
    
    if(blockID == 0 || LblockID == (blockID-1))//包序号合法，更改记录
    {
        LblockID = blockID;
    }
    else//包序号不合法
    {
        
        return LblockID;
    }
    pBlockData=zc_malloc_mem(blockLen,"AppGw3762Afn15F1FileTrans",MEM_TIME_OUT);
	if(pBlockData == NULL)
	{
		return 0xFFFFFFFF;
	}
    __memcpy(pBlockData, (databuf+rcvLen), blockLen);
    app_printf("Block ID  is %d\n", blockID);
    __memcpy(&FileUpgradeData[offsetAddr], pBlockData, blockLen);
    zc_free_mem(pBlockData);
    
    //最后一帧数据接收
    if(fileAttribute == 0x01 && blockID == (totolBlockNum-1))
    {
        if(fileTransId == APP_GW3762_LOCAL_MODULE_UPDATE)
        {
            if(check_image((U32)FileUpgradeData, offsetAddr+blockLen) == IMAGE_OK)
            {
                app_printf("check_image == IMAGE_OK!!!!!!!!!!!!!\n");
                CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize = offsetAddr+blockLen;
                CCOImageWriteFlag = TRUE;

                //CcoWriteImage();
            
            }

            else
            {
                app_printf("check_image == IMAGE_ERR!!!!!!!!!!!!!\n");
                return 0;
            }
            
        }
        else if(fileTransId == APP_GW3762_SLAVE_MODULE_UPDATE || fileTransId == APP_GW3762_SLAVE_CJQ_UPGRADE_START)
		{
            uint32_t   crcValue;
            uint32_t   Crc32Data;
            uint32_t   state;
            ImageFileSize = offsetAddr+blockLen;
            state = check_image_type((U32)FileUpgradeData, ImageFileSize);
            app_printf("Crc check state is %d\n", state);
            crcValue = crc_digest((unsigned char *)FileUpgradeData, ImageFileSize, (CRC_32 | CRC_SW), &Crc32Data);
            app_printf("crcValue = %08x, Crc32Data = %08x\n", crcValue, Crc32Data);

            if(state == IMAGE_OK)
            {
                app_printf("Crc check is OK.\n");
                app_printf("Software version is %02x%02x\n", CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[1], 
                                           CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[0]);
                DstImageBaseAddr = IMAGE1_ADDR+IMAGE_RESERVE;
                CCOImageWriteFlag = TRUE;
            }
            else if(state == IMAGE_UNSAFE)
            {
                DstImageBaseAddr = 0;
                app_printf("It's not ZC bin.\n");
            }
            else
            {

            }
			
            CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize = ImageFileSize;
            CcoUpgrdCtrlInfo.UpgrdFileInfo.FileCrc  = Crc32Data;
            CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockSize = FILE_TRANS_BLOCK_SIZE;
            CcoUpgrdCtrlInfo.UpgrdFileInfo.ImageBaseAddr = DstImageBaseAddr;
            CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum = (CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize/FILE_TRANS_BLOCK_SIZE);

            if(CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize % FILE_TRANS_BLOCK_SIZE)
            {
                CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum += 1;
            }

            CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockBitMapLen = (CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum/8);

            if(CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum % 8)
            {
                CcoUpgrdCtrlInfo.UpgrdFileInfo.BlockBitMapLen += 1;
            }

            app_printf("ImageFileSize : %d, TotolBlockNum: %d\n", ImageFileSize, CcoUpgrdCtrlInfo.UpgrdFileInfo.TotolBlockNum);
            CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex = 0;

            memset(CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrSlaveAddr, 0xFF, 6);
            CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID = ++CurrUpgradeID;
            CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANMITMAXNUM;

            //自动开始升级

            CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT;
            IsCjqfile = e_JZQ_TASK;
            upgrade_cco_reset_evt();
        }
        //结束传文件保护
        timer_stop(ReUpgradetimer , TMR_NULL);
    }
    return blockID;
}

static int8_t  upgrade_cco_reupgrade_timer_init(void)
{
    if(ReUpgradetimer == NULL)
    {
        ReUpgradetimer = timer_create(10*60*1000,
	                    0,
	                    TMR_TRIGGER ,//TMR_TRIGGER
                        upgrade_cco_repgrade_timer_cb,
	                    NULL,
	                    "upgrade_cco_repgrade_timer_cb"
	                    );
    }
    return 0;
}

static int8_t  upgrade_cco_ctrl_timer_init(void)
{
    if(UpgradeCtrltimer == NULL)
    {	
	    UpgradeCtrltimer = timer_create(2*1000,
	                    0,
	                    TMR_TRIGGER ,//TMR_TRIGGER
	                    upgrade_cco_ctrl_timer_cb,
	                    NULL,
	                    "upgrade_cco_ctrl_timer_cb"
	                   );
    }
    return 0;
}

static void upgrade_cco_timeout_timer_cb(struct ostimer_s *ostimer, void *arg)
{			
    app_printf("upgrade_cco_timeout_timer_cb Time out ,need clear WorkSwitchStates!\n");
    if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)
    {
        AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;//释放CCO升级状态
        AppGw3762Afn10F4State.RunStatusWorking  = 0 ;
    }
    LastblockID = 0xFFFFFFFF;
    timer_stop(UpgradeCtrltimer,TMR_NULL);
}

static void upgrade_cco_time_out_modify(uint32_t minute)
{
    if(UpgradeTimeOuttimer == NULL)
	{
        UpgradeTimeOuttimer = timer_create(minute*60*1000,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
                                upgrade_cco_timeout_timer_cb,
	                            NULL,
	                            "upgrade_cco_timeout_timer_cb"
	                           );
		
	}
	else
	{
        timer_modify(UpgradeTimeOuttimer,
                minute*60*1000,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
                upgrade_cco_timeout_timer_cb,
				NULL,
                "upgrade_cco_timeout_timer_cb",
                TRUE);
	}
    timer_start(UpgradeTimeOuttimer);
}

static void upgrade_cco_suspend_timer_cb(struct ostimer_s *ostimer, void *arg)
{			
    app_printf("upgrade_cco_suspend_timer_cb Time out ,need resume!\n");
}

static void upgrade_cco_suspend_timer_init(void)
{
    UpgradeSuspendtimer = timer_create(30*1000,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
                                upgrade_cco_suspend_timer_cb,
	                            NULL,
	                            "upgrade_cco_suspend_timer_cb"
	                           );
}

void upgrade_cco_modify_upgrade_suspend_timer(uint32_t Sec)
{
    if(UpgradeSuspendtimer == NULL)
	{
	    UpgradeSuspendtimer = timer_create(Sec*1000,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
                                upgrade_cco_suspend_timer_cb,
	                            NULL,
	                            "upgrade_cco_suspend_timer_cb"
	                           );
		
	}
	else
	{
        timer_modify(UpgradeSuspendtimer,
				Sec*1000,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
                upgrade_cco_suspend_timer_cb,
				NULL,
                "upgrade_cco_suspend_timer_cb",
                TRUE);
	}
    if(Sec != 0)
    {
        timer_start(UpgradeSuspendtimer);
    }
}

int8_t  upgrade_cco_init(void)
{
    upgrade_cco_reupgrade_timer_init();
    upgrade_cco_ctrl_timer_init();
    //默认创建延时任务定时器
    upgrade_cco_suspend_timer_init();
    return 0;
}

INT32U upgrade_cco_get_ctrl_timer_state(void)
{
    return zc_timer_query(UpgradeCtrltimer);
}

void upgrade_cco_ctrl_timer_modify_and_start(INT32U first_time_ms)
{
    timer_modify(UpgradeCtrltimer,
        first_time_ms,
        0,
        TMR_TRIGGER,//TMR_TRIGGER
        upgrade_cco_ctrl_timer_cb,
        NULL,
        "upgrade_cco_ctrl_timer_cb",
        TRUE);
    timer_start(UpgradeCtrltimer);
}

void upgrade_cco_get_poll_info(INT16U idx, CCO_UPGRADE_POLL *p_data)
{
    if(idx >= MAX_WHITELIST_NUM)
    {
        app_printf("idx error = %d\n", idx);
        return;
    }

    __memcpy((INT8U *)p_data, (INT8U *)&CcoUpgradPoll[idx], sizeof(CCO_UPGRADE_POLL));
}
#endif