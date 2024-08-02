
//#include "ZCheader.h"
#include "app_area_indentification_common.h"
#include "app_phase_position_cco.h"
#include "app_area_indentification_cco.h"
#include "datalinkdebug.h"
#include "datalinktask.h"
#include "netnnib.h"
#include "schtask.h"
#include "app.h"
#include "app_event_report_cco.h"
#include "dev_manager.h"


#if defined(STATIC_MASTER)

static void cco_phase_position_timer_modify(uint32_t MsTime);

static ostimer_t *cco_phase_position_timer = NULL;

PHASE_POSITION_PARA_t cco_phase_position_t={
	.mode      = e_CENTRALIZATION,
	.plan      = period_bit,
	.state     = e_PHASE_POSITION_IDLE,
};


/*
static void feature_info_task_start(work_t *work)
{
	uint8_t  feature;
	uint8_t  addr_broadcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};

	FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t;
	static U8 seq = 0;
	feature = get_feature_from_state(cco_phase_position_t.plan);
	if(feature == e_PERIOD)
	{

		FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
		FeatureCollectStart_Data_t.CollectCycle = 0; //no meaning
		FeatureCollectStart_Data_t.CollectNum = 36;
	
	}
	
	FeatureCollectStart_Data_t.CollectSeq = ++seq;
	FeatureCollectStart_Data_t.res = 0;
	FeatureCollectStart(FeatureCollectStart_Data_t,addr_broadcast,e_DEFAULT_PHASE,feature);	
	app_printf("-----feature_info_task_start-----\n");
	
	work_t *postwork = zc_malloc_mem(sizeof(work_t), "feature_info_task_start", MEM_TIME_OUT);
	postwork->work = feature_info_task_collect;
	post_app_work(postwork);
}


static void feature_info_task_collect(work_t *work)
{
	FEATURE_COLLECT_INFO_t *pCollectInfo = &(cco_phase_position_t.feature_collect_info_t);
	pCollectInfo->CrntNodeIndex = 0;
	app_printf("-----feature_info_task_collect-----\n");
	//开始并发采集特征信息
    add_phase_position_task_list();

}
*/

//计算CCO的自身相序ABC或者ACB，只有两种

U8 gl_cco_phase_order = TRUE;

void cco_phase_calculate_order(void)
{
	U8 phaseA_index = 0;
	U16 ii;
	U32 phaseA_ntb = 0, diff, MiniDiff = 0xffffffff;
	
	if(nnib.PhasebitA == 0 || nnib.PhasebitB == 0 || nnib.PhasebitC == 0)
	{
		return;
	}

	phaseA_index = ZeroData[0].NewOffset - 10;

	app_printf("phaseA_index : %d\n");
	
	phaseA_ntb = ZeroData[0].NTB[phaseA_index];
	
	for(ii = 0; ii < MAXNTBCOUNT; ii++)
	{
		if(phaseA_ntb > ZeroData[e_B_PHASE-1].NTB[ii])
		{
			continue;
		}
		diff = ZeroData[e_B_PHASE-1].NTB[ii] - phaseA_ntb;
		if(MiniDiff > diff)
		{
			MiniDiff = diff;
		}
	}
	
	if(MiniDiff  > 500000) //最小大于20MS，数据无效
	{
		app_printf("---MiniDiff=%d----return----\n",MiniDiff);
		MiniDiff = MiniDiff % 500000;
	}
	//0    6666   13333   20000
	//A     B       C		  A
	
	if((MiniDiff/25)>(6666-1500) && (MiniDiff/25)<(6666+1500))
	{
		app_printf("cco phase ABC\n");
		gl_cco_phase_order = TRUE;
	}
	else if((MiniDiff/25)>(13333-1500) && (MiniDiff/25)<(13333+1500))
	{
		app_printf("cco phase ACB\n");
		gl_cco_phase_order = FALSE;
	}
}


void cco_phase_position_edge_proc(INDENTIFICATION_IND_t *pIndentificationInd)
{
    MULTI_TASK_UP_t MultiTaskUp;
    
    MultiTaskUp.pListHeader =  &PHASE_POSITION_BF_LIST;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(INDENTIFICATION_IND_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pIndentificationInd->SrcMacAddr);

	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pIndentificationInd);    

    return;
}

void cco_phase_position_phase_proc(work_t *work)
{
    PHASE_POSITION_CFM_t *pPhasePositionCfm = (PHASE_POSITION_CFM_t *)work->buf;
    MULTI_TASK_UP_t      MultiTaskUp;

    MultiTaskUp.pListHeader =  &PHASE_POSITION_BF_LIST;
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(PHASE_POSITION_CFM_t , addr);
    MultiTaskUp.Cmplen = sizeof(pPhasePositionCfm->addr);

    multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pPhasePositionCfm);    

    return;
}



static uint8_t cco_phase_position_get_un_edge(FEATURE_COLLECT_INFO_t *pCollectInfo)
{
	uint16_t ii;
	DEVICE_TEI_LIST_t GetDeviceListTemp;
    mutex_pend(&task_work_mutex, 0);

    for(ii=pCollectInfo->CrntNodeIndex; ii<MAX_WHITELIST_NUM;ii++)
	{
		//过滤已经识别和未入网的节点
	    if(DeviceList_ioctl_notsafe(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp) != TRUE || GetDeviceListTemp.NodeTEI == 0xFFF)
        {
            continue;
        }

		//如果在网或者
		if(GetDeviceListTemp.NodeState == e_NODE_ON_LINE  && GetDeviceListTemp.Edgeinfo !=0 
               && GetDeviceListTemp.Edgeinfo != 1)
		{
			pCollectInfo->CrntNodeIndex = ii;
			__memcpy(pCollectInfo->CrntNodeAdd,GetDeviceListTemp.MacAddr, 6);
            mutex_post(&task_work_mutex);
			return TRUE;
		}
	}
    mutex_post(&task_work_mutex);
	if(ii == MAX_WHITELIST_NUM )
	{
		pCollectInfo->CrntNodeIndex= 0;
	}
    
	return FALSE;
}


static int32_t cco_phase_position_bf_entry_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    multi_trans_t  *pTask;
    list_head_t *pos,*node;
    
	if(list->nr >= list->thr)
	{
		app_printf("phase_bf_entry FAIL\n");
		return -1;
	}

    list_for_each_safe(pos, node,&list->link)
    {
        pTask = list_entry(pos, multi_trans_t, link);

        app_printf("pTask->SendType=%d,State=%d,SendType=%d\n",
                              pTask->SendType, pTask->State, new_list->SendType);
        if(memcmp(pTask->CnmAddr, new_list->CnmAddr, 6) == 0 && pTask->State != NONRESP
                && pTask->SendType == new_list->SendType
                )
        {
            app_printf("entry list fail, err -2\n");
            return -2;
        }
    }
    
	app_printf("phase_bf_entry OK\n");
	return 0;
}
static void cco_phase_position_bf_entry_fail(int32_t err, void *pTaskPrmt)
{
	if(err == -1)//list full
	{
		app_printf("phase list full.\n");

        cco_phase_position_timer_modify(20*1000);
	}
    else if(err == -2)
    {
        app_printf("this sta is busy.\n");

        cco_phase_position_timer_modify(60*1000);
    }
}


static void cco_phase_position_edge_task_proc(void *pTaskPrmt)
{
	app_printf("feature_info_collect_task_pro\n");
	multi_trans_t  *pTask = (multi_trans_t *)pTaskPrmt;
    
	FeatureInfoGather(e_DOWNLINK,
					  APS_MASTER_PRM,
					  FlashInfo_t.ucJZQMacAddr,
					  pTask->CnmAddr,
					  pTask->DatagramSeq,
					  e_DEFAULT_PHASE,e_PERIOD);
}
static void cco_phase_position_edge_task_up_proc(void *pTaskPrmt, void *pUplinkData)
{
	U8 edge;
	U16 TEI;
	INDENTIFICATION_IND_t   *pIndentificationInd = (INDENTIFICATION_IND_t *)pUplinkData;
	//存储沿信息
	FEATURE_INFO_INFORMING_DATA_t *informing_data_t = (FEATURE_INFO_INFORMING_DATA_t *)pIndentificationInd->payload;

    app_printf("feature_info_collect_up_pro : %d\n",informing_data_t->CollectMode);
	TEI = informing_data_t->TEI;

	if(informing_data_t->CollectMode == e_COLLECT_NEGEDGE)//记录沿信息
	{
		edge=0;
	}
	else if(informing_data_t->CollectMode == e_COLLECT_POSEDGE)
	{
		edge=1;
		
	}
    DeviceList_ioctl(DEV_SET_EDGE,&TEI,&edge);
}


static U8 cco_phase_position_check_edge(PHASE_COLLECT_INFO_t *collectinfo)  // multi_trans_header_t *listheader, uint8_t sendtype
{
    U16 ii;
    DEVICE_TEI_LIST_t GetDeviceListTemp;
    mutex_pend(&task_work_mutex, 0);
    for(ii=collectinfo->currindex;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(DeviceList_ioctl_notsafe(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp) != TRUE || GetDeviceListTemp.NodeTEI == 0xFFF)
        {
            continue;
        }
        
        if(GetDeviceListTemp.NodeState == e_NODE_ON_LINE  && GetDeviceListTemp.Phase == e_UNKNOWN_PHASE 
			&& (GetDeviceListTemp.Edgeinfo ==0 || GetDeviceListTemp.Edgeinfo == 1))
        {
		    
            collectinfo->currindex = ii;
            //Get_DeviceList_All(ii,&GetDeviceListTemp);
            __memcpy(collectinfo->curraddr,GetDeviceListTemp.MacAddr, 6);
            mutex_post(&task_work_mutex);
            return TRUE;
        }
    }
    mutex_post(&task_work_mutex);
	if(ii == MAX_WHITELIST_NUM)
	{
		collectinfo->currindex = 0;
	}
	return FALSE;
}


static uint8_t cco_phase_position_check_phase(PHASE_COLLECT_INFO_t *collectinfo) // multi_trans_header_t *listheader, uint8_t sendtype
{
    U16 ii;
    DEVICE_TEI_LIST_t GetDeviceListTemp;
    mutex_pend(&task_work_mutex, 0);

    for(ii = collectinfo->currindex; ii < MAX_WHITELIST_NUM; ii++)
    {
        if(DeviceList_ioctl_notsafe(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp) != TRUE || GetDeviceListTemp.NodeTEI == 0xFFF)
        {
            continue;
        }
        
        if(GetDeviceListTemp.NodeState == e_NODE_ON_LINE && GetDeviceListTemp.Phase == e_UNKNOWN_PHASE)
        {
		    
            collectinfo->currindex = ii;
            //Get_DeviceList_All(ii,&GetDeviceListTemp);
            __memcpy(collectinfo->curraddr,GetDeviceListTemp.MacAddr, 6);
            mutex_post(&task_work_mutex);
            return TRUE;
        }
    }
    mutex_post(&task_work_mutex);
    if(ii == MAX_WHITELIST_NUM)
    {
        collectinfo->currindex = 0;
    }
    
    return FALSE;
}



static void cco_phase_position_phase_task_proc(void *pTaskPrmt)
{   
	multi_trans_t  *pTask = (multi_trans_t *)pTaskPrmt;
	
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(PHASE_POSITION_REQ_t), "PPBFP", MEM_TIME_OUT);
	PHASE_POSITION_REQ_t  *phase_position_req_t = (PHASE_POSITION_REQ_t *)work->buf;
    
	__memcpy(phase_position_req_t->addr, pTask->CnmAddr, 6);
    
	work->work = ProcPhaseDistinguishRequest;
	work->msgtype = PHASE_REQ;
	post_datalink_task_work(work);
}

static void cco_phase_position_phase_task_up_proc(void *pTaskPrmt, void *pUplinkData)
{
	//multi_trans_t  *pTask = (multi_trans_t *)pTaskPrmt;
	PHASE_POSITION_CFM_t   *pPhaseCfm = (PHASE_POSITION_CFM_t *)pUplinkData;
	app_printf("phase ok addr->");
	dump_buf(pPhaseCfm->addr, 6);
}

static U8 cco_phase_position_check_bf_list(void)
{
    if(PHASE_POSITION_BF_LIST.nr < PHASE_POSITION_BF_LIST.thr)
    {
        return FALSE;
    }
    return TRUE;
}


static void cco_phase_position_add_task_list(void)
{
    if(cco_phase_position_t.state == e_FEATURE_COLLECT)
    {
        FEATURE_COLLECT_INFO_t *pCollectInfo = &(cco_phase_position_t.feature_collect_info);
        
        multi_trans_t feature_info_req;
						
        feature_info_req.lid           = INDENTIFICATION_LID;
        feature_info_req.SrcTEI        = 0;
        feature_info_req.DeviceTimeout = DEVTIMEOUT;
        feature_info_req.Framenum      = 0;
        
        //__memcpy(addr, pCollectInfo->CrntNodeAdd, MAC_ADDR_LEN);
        //ChangeMacAddrOrder(addr);
        //__memcpy(feature_info_req.Addr, pCollectInfo->CrntNodeAdd, MAC_ADDR_LEN);	
        __memcpy(feature_info_req.CnmAddr, pCollectInfo->CrntNodeAdd, MAC_ADDR_LEN);

        feature_info_req.State         = UNEXECUTED;
        feature_info_req.SendType      = INDENTIFICATION_QUERY;
        feature_info_req.StaIndex      = pCollectInfo->CrntNodeIndex;
        feature_info_req.DatagramSeq   = ++ApsSendPacketSeq;
        feature_info_req.DealySecond   = PHASE_POSITION_BF_LIST.nr == 0?0:MULTITRANS_INTERVAL;
        feature_info_req.ReTransmitCnt = pCollectInfo->ReTransmitCnt > 0 ? pCollectInfo->ReTransmitCnt : 1;
			
        feature_info_req.DltNum         = 1;
        feature_info_req.ProtoType      = 0;
        feature_info_req.FrameLen       = 0;
        feature_info_req.EntryListCheck = cco_phase_position_bf_entry_check;
        feature_info_req.EntryListfail  = cco_phase_position_bf_entry_fail; 
        feature_info_req.TaskPro        = cco_phase_position_edge_task_proc; 
        feature_info_req.TaskUpPro      = cco_phase_position_edge_task_up_proc;
        feature_info_req.TimeoutPro     = NULL;

        feature_info_req.pMultiTransHeader = &PHASE_POSITION_BF_LIST;
        feature_info_req.CRT_timer = phaseposition_multi_timer;
            
        app_printf("get e_FEATURE_COLLECT TRUE\n");
        multi_trans_put_list(feature_info_req, NULL);

	}
	if(cco_phase_position_t.state == e_PHASE_COLLECT)
	{
		PHASE_COLLECT_INFO_t *pCollectInfo = &(cco_phase_position_t.phase_collect_info);

        multi_trans_t phase_position_req;
				
        phase_position_req.lid			 = INDENTIFICATION_LID;
        phase_position_req.SrcTEI		 = 0;
        phase_position_req.DeviceTimeout = DEVTIMEOUT;
        phase_position_req.Framenum 	 = 0;
        
        //__memcpy(addr, pCollectInfo->curraddr, MAC_ADDR_LEN);
        //ChangeMacAddrOrder(addr);
        //__memcpy(phase_position_req.Addr, addr, MAC_ADDR_LEN);	
        __memcpy(phase_position_req.CnmAddr, pCollectInfo->curraddr, MAC_ADDR_LEN);

        phase_position_req.State		 = UNEXECUTED;
        phase_position_req.SendType 	 = PHASEPOSITION_QUERY;
        phase_position_req.StaIndex      = pCollectInfo->currindex;
        phase_position_req.DatagramSeq	 = 0;//++ApsSendPacketSeq;
        phase_position_req.DealySecond	 = PHASE_POSITION_BF_LIST.nr == 0?0:MULTITRANS_INTERVAL;
        phase_position_req.ReTransmitCnt = BFMAXNUM;
			
        phase_position_req.DltNum		  = 1;
        phase_position_req.ProtoType	  = 0;
        phase_position_req.FrameLen 	  = 0;
        phase_position_req.EntryListCheck = cco_phase_position_bf_entry_check;
        phase_position_req.EntryListfail  = cco_phase_position_bf_entry_fail; 
        phase_position_req.TaskPro		  = cco_phase_position_phase_task_proc; 
        phase_position_req.TaskUpPro	  = cco_phase_position_phase_task_up_proc;
        phase_position_req.TimeoutPro	  = NULL;

        phase_position_req.pMultiTransHeader = &PHASE_POSITION_BF_LIST;
        phase_position_req.CRT_timer = phaseposition_multi_timer;
        
        app_printf("get e_PHASE_COLLECT TRUE\n");
        multi_trans_put_list(phase_position_req, NULL);
	}

    return;
}

/*

void ctrl_phase_position_task_round(void)
{
	if(cco_phase_position_t.state == e_FEATURE_COLLECT)
	{
		FEATURE_COLLECT_INFO_t *pCollectInfo = &(cco_phase_position_t.feature_collect_info_t);
		if(pCollectInfo->CrntNodeIndex == MAX_WHITELIST_NUM)
		{
			if(PHASE_POSITION_BF_LIST.nr == 0)//本次并发任务结束
			{
				app_printf("PHASE_POSITION_BF_LIST empty!!!\n");
				cco_phase_position_t.state = e_PHASE_COLLECT;
				cco_phase_position_add_task_list();
			}
		}
	}
	else if(cco_phase_position_t.state == e_PHASE_COLLECT)
	{
		PHASE_COLLECT_INFO_t *pCollectInfo = &(cco_phase_position_t.phase_collect_info_t);
		if(pCollectInfo->currindex == MAX_WHITELIST_NUM)
		{
			pCollectInfo->currindex = 0;
			pCollectInfo->currround++;
		}
	}
	else
	{
		;
	}
}


void find_feature_info_collect_task(work_t *work)
{
	multi_trans_header_t       *pPhaseBFlistheader;
    INDENTIFICATION_IND_t      *pIndentificationInd = (INDENTIFICATION_IND_t*)work->buf;

    multi_trans_t  *pEdgeBFTask;
    list_head_t *pos,*node;

	pPhaseBFlistheader = &PHASE_POSITION_BF_LIST;

    if(pPhaseBFlistheader->nr <= 0)
	{
		return;
	}
    
    list_for_each_safe(pos, node,&pPhaseBFlistheader->link)
    {
        pEdgeBFTask = list_entry(pos , multi_trans_t , link);
        if(pEdgeBFTask->State == EXECUTING && pEdgeBFTask->DatagramSeq == pIndentificationInd->DatagramSeq)
        {
            if(pEdgeBFTask->TaskUpPro != NULL)
            {
                pEdgeBFTask->TaskUpPro(pEdgeBFTask, pIndentificationInd);
            }
			//pEdgeBFTask->State = NONRESP;
			list_del(&pEdgeBFTask->link);
			pPhaseBFlistheader->nr--;
			zc_free_mem(pEdgeBFTask);
			app_printf("Pdel\n");

        }
    }
	return;
}
*/

/*
void find_phase_position_task_info(work_t *work)
{
	multi_trans_header_t       *pPhaseBFlistheader = &PHASE_POSITION_BF_LIST;
    PHASE_POSITION_IND_t	   *pPhaseInd_t = (PHASE_POSITION_IND_t *)work->buf;
	
    multi_trans_t  *pPhaseBFTask;
    list_head_t *pos,*node;

    if(pPhaseBFlistheader->nr <= 0)
	{
		return;
	}
    
    list_for_each_safe(pos, node,&pPhaseBFlistheader->link)
    {
        pPhaseBFTask = list_entry(pos , multi_trans_t , link);
        if(pPhaseBFTask->State == EXECUTING && 0==memcmp(pPhaseInd_t->addr, pPhaseBFTask->CnmAddr, 6))
        {
            if(pPhaseBFTask->TaskUpPro != NULL)
            {
                pPhaseBFTask->TaskUpPro(pPhaseBFTask, pPhaseInd_t);
            }
			pPhaseBFTask->State = NONRESP;
        }
    }
	return;
}
*/



static void cco_phase_position_run(work_t *work)
{
	U8	feature;
	U8	addr_broadcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	
	FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t;
	
	static U8 seq = 0;
	
	if(app_ext_info.func_switch.PhaseSWC != 1)
	{
		return;
	}
	
    app_printf("phase_t.state : %-20s\n",
										e_PHASE_POSITION_IDLE == cco_phase_position_t.state ? "e_PHASE_POSITION_IDLE":
										e_FEATURE_CLLCT_START == cco_phase_position_t.state ? "e_FEATURE_CLLCT_START":
										e_FEATURE_COLLECT == cco_phase_position_t.state ? "e_FEATURE_COLLECT":
										e_PHASE_COLLECT == cco_phase_position_t.state ? "e_PHASE_COLLECT":
                                                                "error state");
	
    switch(cco_phase_position_t.state)
    {
        case e_FEATURE_CLLCT_START:		//下发启动
        {
	        feature = cco_area_ind_get_feature_by_state(cco_phase_position_t.plan);
			
	        if(feature == e_PERIOD)
	        {
		        FeatureCollectStart_Data_t.Startntb = get_phy_tick_time() + START_COLLECT_TIME * PHY_TICK_FREQ;
		        FeatureCollectStart_Data_t.CollectCycle = 0;
		        FeatureCollectStart_Data_t.CollectNum = 36;	
	        }
			
	        FeatureCollectStart_Data_t.CollectSeq = ++seq;
	        FeatureCollectStart_Data_t.res = 0;

			//广播下发特征信息采集开始
	        FeatureCollectStart(FeatureCollectStart_Data_t,addr_broadcast, e_DEFAULT_PHASE, feature);	
            
	        cco_phase_position_t.state = e_FEATURE_COLLECT;
            cco_phase_position_t.timeout = (START_COLLECT_TIME+5)*TIMER_UNITS;
            
            cco_phase_position_t.feature_collect_info.CrntNodeIndex = 0;
            cco_phase_position_t.feature_collect_info.CrntRound = 0;

            cco_phase_position_t.phase_collect_info.currindex = 0;
            cco_phase_position_t.phase_collect_info.currround = 0;
			
            break;
        }
        case e_FEATURE_COLLECT:			//采集沿信息
        {
        	//并发列表是否满
            if(cco_phase_position_check_bf_list() == FALSE)
            {
                cco_phase_position_t.timeout = 5*TIMER_UNITS;
    	        //开始并发采集特征信息
    		    if(cco_phase_position_get_un_edge(&cco_phase_position_t.feature_collect_info) == TRUE)
                {
                    cco_phase_position_add_task_list();
                    cco_phase_position_t.feature_collect_info.CrntNodeIndex++;
                }
                else
                {
    			    cco_phase_position_t.state = e_PHASE_COLLECT;
                    cco_phase_position_t.feature_collect_info.CrntRound += 1;
                    cco_phase_position_t.timeout = 30*TIMER_UNITS;
                }
            }
            else
            {
                cco_phase_position_t.timeout = 5*TIMER_UNITS;
            }
            break;
        }
        case e_PHASE_COLLECT:			//相位采集
        {
            if(cco_phase_position_check_bf_list() == FALSE)
            {
                cco_phase_position_t.timeout = 5*TIMER_UNITS;
                //前三轮在要相位时判断是否有沿信息, 之后要相位时不再判断是否有沿信息，主要为了兼容之前没有增加台区识别命令的版本
                if((cco_phase_position_t.phase_collect_info.currround < 3 && cco_phase_position_check_edge(&cco_phase_position_t.phase_collect_info) ==  TRUE)
                  || (cco_phase_position_t.phase_collect_info.currround >= 3 && cco_phase_position_check_phase(&cco_phase_position_t.phase_collect_info) == TRUE))
                {
                    cco_phase_position_add_task_list();
                    cco_phase_position_t.phase_collect_info.currindex++;
                }
                else
                {
                    cco_phase_position_t.state = e_FEATURE_CLLCT_START;
                    cco_phase_position_t.phase_collect_info.currround += 1;
                    cco_phase_position_t.timeout = 30*TIMER_UNITS;
                }
            }
            else
            {
                cco_phase_position_t.timeout = 5*TIMER_UNITS;
            }
            
            break;
        }
        default:

            break;
    }

    cco_phase_position_timer_modify(cco_phase_position_t.timeout);
}



static void cco_phase_position_timercb(struct ostimer_s *ostimer, void *arg)
{
	
    if(getHplcTestMode() != NORM)
    {
        return;
    }
	work_t *work = zc_malloc_mem(sizeof(work_t),"Phase",MEM_TIME_OUT);
	work->work = cco_phase_position_run;
	work->msgtype = PHASE_RUN;
	post_app_work(work);	
    cco_phase_position_timer_modify(60*TIMER_UNITS);
}



int8_t cco_phase_position_timer_init(void)
{

    if(cco_phase_position_timer == NULL)
    {
        cco_phase_position_timer = timer_create(10*TIMER_UNITS,
                                10*TIMER_UNITS,
	                            TMR_TRIGGER,
	                            cco_phase_position_timercb,
	                            NULL,
	                            "cco_phase_position_timercb"
	                           );
    }
	return TRUE;
}

//相位识别定时器
static void cco_phase_position_timer_modify(uint32_t MsTime)
 {
     if(MsTime == 0)
        return;
     
     if(NULL != cco_phase_position_timer)
     {
         timer_modify(cco_phase_position_timer,
                 MsTime,
                 10,
                 TMR_TRIGGER,
                 cco_phase_position_timercb,
                 NULL,
                 "cco_phase_position_timercb",
                 TRUE
                 );
     }
     else
     {
         sys_panic("cco_phase_position_timer is null\n");
     }
     timer_start(cco_phase_position_timer);
}


/*
启动相位识别
*/
void cco_phase_position_start(uint8_t mode, uint8_t plan)
{
	cco_phase_position_t.mode = mode;
	cco_phase_position_t.plan = plan;
	cco_phase_position_t.state = e_FEATURE_CLLCT_START;
	memset(&cco_phase_position_t.feature_collect_info, 0x00, sizeof(FEATURE_COLLECT_INFO_t));
	memset(&cco_phase_position_t.phase_collect_info, 0x00, sizeof(PHASE_COLLECT_INFO_t));
	
	cco_phase_position_timer_modify(1*TIMER_UNITS);
}

#endif


