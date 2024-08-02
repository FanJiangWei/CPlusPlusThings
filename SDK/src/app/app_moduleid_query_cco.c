#include "list.h"
#include "types.h"
#include "Trios.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
#include "app_moduleid_query.h"
#include "datalinkdebug.h"
#include "aps_layer.h"
#include "aps_interface.h"
#include "app_base_multitrans.h"
#include "app_gw3762.h"
#include "netnnib.h"
#include "SchTask.h"



#if defined(STATIC_MASTER)

ostimer_t *module_id_query_timer = NULL;

U16   ApsQueryIDSendPacketSeq = 0;
U16   ApsQueryIDRecvPacketSeq = 0xffff;


void cco_app_module_id_query_cfm_proc(QUERY_IDINFO_CFM_t  * pQueryIdInfoCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;

    
    MultiTaskUp.pListHeader =  &MODULE_ID_QUERY_LIST;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(QUERY_IDINFO_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pQueryIdInfoCfm->SrcMacAddr);
    
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pQueryIdInfoCfm);
    

    return;
}


static void module_id_req(void *pTaskPrmt)
{
    multi_trans_t  *pModuleIdReqTask = (multi_trans_t*)pTaskPrmt;
    QUERY_IDINFO_REQ_RESP_t	 QueryIdInfoReq;
	
    QueryIdInfoReq.direct = e_DOWNLINK;
    QueryIdInfoReq.IdType = pModuleIdReqTask->FrameUnit[0];   //APP_GW3762_MODULE_ID;
    app_printf("IdType %d \n",QueryIdInfoReq.IdType);
    QueryIdInfoReq.destei = 0;

    QueryIdInfoReq.AsduLength = 0;
    QueryIdInfoReq.PacketSeq = pModuleIdReqTask->DatagramSeq;

    __memcpy(QueryIdInfoReq.DstMacAddr, pModuleIdReqTask->CnmAddr, 6);
    __memcpy(QueryIdInfoReq.SrcMacAddr, GetNnibMacAddr(), 6);
    
    ApsQueryIdInfoReqResp(&QueryIdInfoReq);

    return;
}


static int32_t module_id_req_entry_list_check(multi_trans_header_t *list, multi_trans_t *new_list)
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
        dump_buf(mbuf_n->CnmAddr, 6);
        if(memcmp(mbuf_n->CnmAddr, new_list->CnmAddr, 6) == 0)
        {
            return -2;
        }
    }

    return 0;
}

static void module_req_entry_list_fail(int32_t err_code, void *pTaskPrmt)
{
    app_printf("Entry ModuleReq fail, err = %d\n", err_code);
    
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

static void module_id_req_timeout(void *pTaskPrmt)
{
    
}
static U8 check_module_req_full(void)
{
    //if(MODULE_ID_QUERY_LIST.thr >= MODULE_ID_QUERY_LIST.nr)
    if(MODULE_ID_QUERY_LIST.nr == 0)
    {
        return FALSE;
    }
    return TRUE;
}

static void module_id_info_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    QUERY_IDINFO_CFM_t  *pQueryIdInfoCfm = (QUERY_IDINFO_CFM_t*)pUplinkData;
    U8  idInfoLen;
    //U8  deviceType;
    U8  IdInfo[50];
    U8  byLen = 0;
    if(multi_trans_add_con_result(&CTRL_MODULE_ID_QUERY_Info, pReadTask->StaIndex) == TRUE)
    {
        if(pQueryIdInfoCfm->Status == e_SUCCESS)
        {
            idInfoLen = pQueryIdInfoCfm->Asdu[byLen++];
            app_printf("idInfoLen=%d;\n",idInfoLen);
            dump_buf(&pQueryIdInfoCfm->Asdu[byLen], 6);
            __memcpy(IdInfo, &pQueryIdInfoCfm->Asdu[byLen], idInfoLen);
            byLen += idInfoLen;
            byLen++;
            //deviceType = pQueryIdInfoCfm->Asdu[byLen++];
            //deviceType = deviceType;

            
            U8 state =0;
            U8 macaddr[6]={0};
            //ChangeMacAddrOrder(pQueryIdInfoCfm->SrcMacAddr);
    		app_printf("Con daddr : ");
    		dump_buf(pQueryIdInfoCfm->DstMacAddr, 6);
    		
    		app_printf("Con saddr : ");
    		dump_buf(pQueryIdInfoCfm->SrcMacAddr, 6);
    		
    		state = SaveModeId(pQueryIdInfoCfm->SrcMacAddr,IdInfo);
    		app_printf("state=%d;\n",state);
            __memcpy(macaddr,pQueryIdInfoCfm->SrcMacAddr,6);
            ChangeMacAddrOrder(macaddr);
            if(state == e_HasGet||state == e_NeedReport)
            {
                save_mode_id_by_addr(macaddr,IdInfo, idInfoLen);
            }
            
        }
    }
    return;
}



void add_id_info_query_req(void)
{
    //app_printf("AddIdInfoQueryReq\n");
    
	if(multi_trans_add_task_form_poll(&CTRL_MODULE_ID_QUERY_Info) )
	{  
        multi_trans_t IdInfoQueryReq;
        U8 idType = APP_GW3762_MODULE_ID;
        app_printf("AddIdInfoQ = %d\n", CTRL_MODULE_ID_QUERY_Info.CurrentIndex);
      
        dump_buf(CTRL_MODULE_ID_QUERY_Info.CrntNodeAdd,6);
    	
    	IdInfoQueryReq.lid = UPGRADE_LID;
    	IdInfoQueryReq.SrcTEI = 0;
        IdInfoQueryReq.DeviceTimeout = DEVTIMEOUT;
    	
        __memcpy(IdInfoQueryReq.CnmAddr, CTRL_MODULE_ID_QUERY_Info.CrntNodeAdd, MAC_ADDR_LEN);

    	IdInfoQueryReq.State = UNEXECUTED;
        
        IdInfoQueryReq.SendType = MODULE_ID_QUERY;
        IdInfoQueryReq.StaIndex = CTRL_MODULE_ID_QUERY_Info.CurrentIndex;     
        IdInfoQueryReq.DatagramSeq = ++ApsQueryIDSendPacketSeq;
    	IdInfoQueryReq.DealySecond = MODULE_ID_QUERY_LIST.nr == 0?0:1;
        IdInfoQueryReq.ReTransmitCnt = CTRL_MODULE_ID_QUERY_Info.ReTransmitCnt;
    	
        IdInfoQueryReq.DltNum = 1;
    	IdInfoQueryReq.ProtoType = 0;
    	IdInfoQueryReq.FrameLen  = 1;
        IdInfoQueryReq.EntryListCheck = module_id_req_entry_list_check;
    	IdInfoQueryReq.EntryListfail = module_req_entry_list_fail;
    	IdInfoQueryReq.TaskPro = module_id_req; 
        IdInfoQueryReq.TaskUpPro = module_id_info_proc;
        IdInfoQueryReq.TimeoutPro = module_id_req_timeout;

        IdInfoQueryReq.pMultiTransHeader = &MODULE_ID_QUERY_LIST;
    	IdInfoQueryReq.CRT_timer = moduleid_query_timer;
        multi_trans_put_list(IdInfoQueryReq, &idType);
        //entry_multi_trans_list;//如果链表刷新过程中需要入队任务，是否可以直接使用入队接口，无需post
    }
    else
    {
        //app_printf("MODULE_ID_QUERY_LIST.TaskRoundPro\n");
        //MODULE_ID_QUERY_LIST.TaskRoundPro();
    }
}


void id_info_query(U16 moduleIndex, U8 *pMacAddr, U8 idType, MESG_PORT_e port, TaskUpProc pUpProcFunc)
{
    multi_trans_t ModuleIdQueryReq;

    ModuleIdQueryReq.lid = UPGRADE_LID;
    ModuleIdQueryReq.SrcTEI = 0;
    ModuleIdQueryReq.DeviceTimeout = DEVTIMEOUT;
    ModuleIdQueryReq.MsgPort = port;
    	
    __memcpy(ModuleIdQueryReq.CnmAddr, pMacAddr, MAC_ADDR_LEN);

    ModuleIdQueryReq.State = UNEXECUTED;
        
    ModuleIdQueryReq.SendType = MODULE_ID_QUERY;
    ModuleIdQueryReq.StaIndex = moduleIndex;     
    ModuleIdQueryReq.DatagramSeq = ++ApsQueryIDSendPacketSeq;
	ModuleIdQueryReq.DealySecond = MODULE_ID_QUERY_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    ModuleIdQueryReq.ReTransmitCnt = 1;
    	
    ModuleIdQueryReq.DltNum = 1;
    ModuleIdQueryReq.ProtoType = 0;
    ModuleIdQueryReq.FrameLen  = 1;  
    //ModuleIdQueryReq.FrameUnit[0] = idType;  // APP_GW3762_MODULE_ID;
            
    ModuleIdQueryReq.EntryListCheck = module_id_req_entry_list_check;
    ModuleIdQueryReq.EntryListfail = module_req_entry_list_fail;
    ModuleIdQueryReq.TaskPro = module_id_req; 

    ModuleIdQueryReq.TaskUpPro = (TaskUpProc)pUpProcFunc;    //module_id_info_proc;
    ModuleIdQueryReq.TimeoutPro = module_id_req_timeout;

    ModuleIdQueryReq.pMultiTransHeader = &MODULE_ID_QUERY_LIST;
    ModuleIdQueryReq.CRT_timer = moduleid_query_timer;
    multi_trans_put_list(ModuleIdQueryReq, &idType);


    return;
}
/*
static void AddModuleIdQueryReq(void)
{
    U16 i;
	static U16 modeIDTeiIndex=0;
    
	DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    
    if(app_ext_info.func_switch.ModuleIDGetSWC != 1)
        return;

    //app_printf("Add module id query Req\n");

    for(i = modeIDTeiIndex; i < MAX_WHITELIST_NUM; i++)
    {
        DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&i, &GetDeviceListTemp_t);//Get_DeviceList_All(i,&GetDeviceListTemp_t);

        if(GetDeviceListTemp_t.NodeTEI != 0xFFF && (GetDeviceListTemp_t.ModeNeed ==e_NeedGet || GetDeviceListTemp_t.ModeNeed ==e_InitState))
        {
            id_info_query(modeIDTeiIndex, GetDeviceListTemp_t.MacAddr, APP_GW3762_MODULE_ID, e_UART1_MSG, module_id_info_proc);
            
            modeIDTeiIndex ++;
            
            return;    
        }
    }
    if(i == MAX_WHITELIST_NUM)
    {
        modeIDTeiIndex = 0;
    }

    return;
}
*/
uint8_t id_query_match(DEVICE_TEI_LIST_t GetDeviceListTemp_t)
{
    if(GetDeviceListTemp_t.NodeTEI != 0xFFF && (GetDeviceListTemp_t.ModeNeed ==e_NeedGet || GetDeviceListTemp_t.ModeNeed ==e_InitState))
    {
        return TRUE;
    }
    return FALSE;
}

void get_idinfo_from_net_set_poll(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    U16 ii ;
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    app_printf("seq\ttei\tresult\n");
    for(ii = 0;ii < MAX_WHITELIST_NUM;ii++)
    {
        if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp_t)==TRUE&&id_query_match(GetDeviceListTemp_t)==TRUE)
        {

            pConInfo->taskPolling[ii].Valid = TRUE;
            pConInfo->taskPolling[ii].Send = 0;
            pConInfo->taskPolling[ii].Result = 0;
        }
        else
        {
            memset(&pConInfo->taskPolling[ii] , 0x00 ,sizeof(CCO_TASK_TEI_RECORD));
        }
    }
    pConInfo->CurrentIndex = 0;

}


static void app_module_id_query_start(work_t *work)
{
    if(app_ext_info.func_switch.ModuleIDGetSWC != 1)
        return;
    //if()
    MODULE_ID_QUERY_LIST.TaskAddOne = add_id_info_query_req;
    MODULE_ID_QUERY_LIST.TaskRoundPro = NULL;
    if(check_module_req_full() == FALSE)
    {
        get_idinfo_from_net_set_poll(&CTRL_MODULE_ID_QUERY_Info);
        add_id_info_query_req();
    }
}


static void module_id_query_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    if(HPLC.testmode != NORM)
    {
        return;
    }
    if(check_module_req_full() == FALSE)
    {
    	work_t *work = zc_malloc_mem(sizeof(work_t),"URMI",MEM_TIME_OUT);
    	work->work = app_module_id_query_start;
    	work->msgtype = MID_REQ;
    	post_app_work(work);	
    }
}


int8_t  moduleid_query_timer_init(void)
{

    if(module_id_query_timer == NULL)
    {
        module_id_query_timer = timer_create(180*1000,
                                5*60*1000,
	                            TMR_PERIODIC,//TMR_TRIGGER
	                            module_id_query_timer_cb,
	                            NULL,
	                            "ModuleIdQueryTimerCB"
	                           );
    }

	timer_start(module_id_query_timer);

	return TRUE;
}






#endif























