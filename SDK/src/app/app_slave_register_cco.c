/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */
#include "mem_zc.h"
#include "app.h"
#include "ZCsystem.h"
#include "app_read_router_cco.h"
#include "datalinkdebug.h"
#include "app_slave_register_cco.h"
#include "app_gw3762.h"
#include "DatalinkTask.h"
#include "netnnib.h"
#include "SchTask.h"
#include "app_deviceinfo_report.h"

#if defined(STATIC_MASTER)

ostimer_t *slave_register_timer = NULL;
ostimer_t *register_over_timer = NULL;//注册最大时间管理
ostimer_t *register_jugeover_timer = NULL;//判断注册是否完成
ostimer_t *register_suspend_timer = NULL;//注册暂停管理
ostimer_t *g_CJQRegisterPeriodTimer = NULL;
ostimer_t *cycle_del_slave_timer = NULL;

RegisterInfo_t  register_info;
CcoRegstRecord_t CcoRegPolling[MAX_WHITELIST_NUM] ;

U8  g_LeaveCycleCnt = 0;//统计离线指示发送次数，不能一直发送，只发送1小时
U8  g_QueryCollectFuncFlag = FALSE;



U8 check_cjq_has()
{
    U16 j;
    DEVICE_TEI_LIST_t DeviceListTemp;
    U8  default_FF_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};

    app_printf("check_cjq_has!\n");
    for(j = 0 ; j < MAX_WHITELIST_NUM ; j++)
    {
        DeviceList_ioctl(DEV_GET_ALL_BYSEQ, &j, &DeviceListTemp);
        if(0==memcmp(DeviceListTemp.MacAddr,default_FF_addr,6))//无效地址
        {
            continue;
        }

        if((e_CJQ_2 == DeviceListTemp.DeviceType||e_CJQ_1 == DeviceListTemp.DeviceType))
        {
//            FindCJQFlag = TRUE;
            app_printf("Has CJQ ,need Register!\n");
            return TRUE;
        }
    }
//    FindCJQFlag = FALSE;
    return FALSE;
}
void cycle_del_slave_proc(struct work_s * pMsgData)
{
    U8	tempAddr[6]={0,0,0,0,0,0};
    static U16 tempindex = 0;
    U16 jj = 0;
    U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    
	LEAVE_IND_t *pLeaveInd = NULL;

    pLeaveInd = zc_malloc_mem(MAX_NET_MMDATA_LEN, "DLSL", MEM_TIME_OUT);
    pLeaveInd->delayTime = 0;
    pLeaveInd->StaNum = 0;
	pLeaveInd->DelType = 0;
	
    for(jj = tempindex;jj<MAX_WHITELIST_NUM;jj++)
    {
        DEVICE_TEI_LIST_t DeviceListTemp;
        Get_DeviceList_All(jj ,  &DeviceListTemp);
        
        if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
        {
            continue;
        }
        if(DeviceListTemp.DeviceType == e_RELAY)
        {
            continue;
        }
        if(DeviceListTemp.WhiteSeq == 0xFFF)
        {
            __memcpy(tempAddr,DeviceListTemp.MacAddr,6);
            __memcpy(&pLeaveInd->pMac[(pLeaveInd->StaNum)*MACADRDR_LENGTH],tempAddr,6);
			pLeaveInd->StaNum++;
            
            app_printf("LNum:=%d\n",pLeaveInd->StaNum);
            if(pLeaveInd->StaNum >= MAXLEVELNODENUM)
            {
                tempindex = ++ jj;
                app_printf("nextidex=%d\n",tempindex);
                modify_cycle_del_slave_timer(10);
                break;
            }
        }
    }
    if(g_LeaveCycleCnt == (MAXLEVELCNT)-1) //最后一轮
    {
        pLeaveInd->DelType = e_LEAVE_AND_DEL_DEVICELIST;
    }
    else
    {
        pLeaveInd->DelType = e_LEAVE_WAIT_SELF_OUTLINE;
    }
    
    if(jj == MAX_WHITELIST_NUM)
    {
        app_printf("clidex = %d\n",tempindex);
        tempindex = 0;
        g_LeaveCycleCnt ++;
        app_printf("LvCyN = %d\n",g_LeaveCycleCnt);
        if(g_LeaveCycleCnt < MAXLEVELCNT)
        {
            modify_cycle_del_slave_timer(5*60);
        }
        else
        {
            app_printf("stop Del\n");
            g_LeaveCycleCnt = 0;
            timer_stop(cycle_del_slave_timer,TMR_NULL);
        }
        
    }
	

	app_printf("---DelSlave=%d,idx=%d\n",pLeaveInd->StaNum,tempindex);
	if(pLeaveInd->StaNum > 0)
	{
		ApsSlaveLeaveIndRequst(pLeaveInd);
	}
	
	zc_free_mem(pLeaveInd);
}


void cycle_del_slave_timerCB(struct ostimer_s *ostimer, void *arg)
{
	//app_printf("CycleDelSlavetimerCB\n");
    if(AppGw3762Afn10F4State.WorkSwitchStates == REGISTER_RUN_FLAG)
    {
        g_LeaveCycleCnt = 0;
        timer_stop(cycle_del_slave_timer,TMR_NULL);
    }
    else
    {
        work_t *work = zc_malloc_mem(sizeof(work_t),"cycle_del_slave_proc",MEM_TIME_OUT);
    	work->work = cycle_del_slave_proc;
    	work->msgtype = CYCLE_DEL;
		post_app_work(work);	
    }
}

void modify_cycle_del_slave_timer(U32 Sec)
{
    if(Sec == 0)
        return;
    if(cycle_del_slave_timer == NULL)
    {
        cycle_del_slave_timer = timer_create(Sec*1000,
	                    0,
	                    TMR_TRIGGER ,//TMR_TRIGGER
	                    cycle_del_slave_timerCB,
	                    NULL,
	                    "cycle_del_slave_timerCB"
	                   );
    }
    else
    {
        timer_modify(cycle_del_slave_timer,
			Sec*1000, 
			0,
			TMR_TRIGGER ,//TMR_TRIGGER
			cycle_del_slave_timerCB,
			NULL,
            "cycle_del_slave_timerCB",
            TRUE);
    }
    timer_start(cycle_del_slave_timer);
}

void register_cco_query_cfm_proc(REGISTER_QUERY_CFM_t   *pRegisterQueryCfm)
{
    MULTI_TASK_UP_t MultiTaskUp;

    
    MultiTaskUp.pListHeader =  &REGISTER_QUER_LIST;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(REGISTER_QUERY_CFM_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pRegisterQueryCfm->SrcMacAddr);
    
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pRegisterQueryCfm);
    

    return;
}


 void clean_registe_list(void)
{
	memset(CcoRegPolling,0x00,sizeof(CcoRegPolling));
}

                                                  
static U16 regsiter_info_query_from_net(void)
{
    U16 ii ,TotalNum=0;
  
    app_printf("seq\ttei\tQuery\tLock\n");
    DEVICE_TEI_LIST_t DevTeiTemp;
    mutex_pend(&task_work_mutex, 0);
    for(ii=0; ii<MAX_WHITELIST_NUM; ii++)
    {
        if(DeviceList_ioctl_notsafe(DEV_GET_ALL_BYSEQ,&ii,&DevTeiTemp))
        {
            if(PROVINCE_JIANGSU == app_ext_info.province_code )
            {
                if(register_info.RegisterType == e_GENERAL_REGISTER || register_info.RegisterType == e_CJQSELF_REGISTER )
                {
                    if(AppGw3762Afn10F4State.WorkSwitchRegister != TRUE&&(DevTeiTemp.DeviceType != e_CJQ_2 && DevTeiTemp.DeviceType != e_CJQ_1))
                         continue;
                }
                TotalNum++;
                CcoRegPolling[ii].Valid = 0x01;
                CcoRegPolling[ii].CollectValid = 0x01;
            }
            else
            {
                if(AppGw3762Afn10F4State.WorkSwitchRegister != TRUE&&(DevTeiTemp.DeviceType != e_CJQ_2 && DevTeiTemp.DeviceType != e_CJQ_1))
                    continue;
                TotalNum++;
                CcoRegPolling[ii].Valid = 0x01;
            }
        }
        else
        {
            memset(&CcoRegPolling[ii] , 0x00 ,sizeof(CcoRegstRecord_t));
        }
    }
    mutex_post(&task_work_mutex);
    app_printf("-------regsiter_info_query_from_net,TotalNum=%d\n",TotalNum);
    return TotalNum;
}


static  U8 reqister_list_index_query_from_Poll(void)
{
    U16 ii;
    U8  FindFlag = FALSE;

    if(PROVINCE_JIANGSU == app_ext_info.province_code)//江苏独立的注册功能
    {
        if(register_info.RegistState != e_REGISTER_COLLECT)
        {	
        	 for(ii= register_info.CrnSlaveIndex;ii<MAX_WHITELIST_NUM;ii++)
    	    {
    	        if(CcoRegPolling[ii].Valid == 0)
    	        {
    	            continue;
    	        }

    	        if((register_info.RegistState == e_REGISTER_REQ) && ((CcoRegPolling[ii].QueryState == e_UNFINISH) ))
    	        { 
    	            register_info.CrnSlaveIndex = ii;            
    				FindFlag = TRUE;
    	            break;
    	        }
    	        else if((register_info.RegistState == e_REGISTER_LOCK)&& (CcoRegPolling[ii].LockState == e_UNFINISH))
    	        {
    	            register_info.CrnSlaveIndex = ii;
    	            FindFlag = TRUE;
    	            break;
    	        }
    	      
    	    }
        }
        else
    	{
    		//app_printf("RegisterInfo_t.CrnCollectIndex = %d\n",RegisterInfo_t.CrnCollectIndex);
    		for(ii= register_info.CrnCollectIndex;ii<MAX_WHITELIST_NUM;ii++)
    	    {
    	    	 if(CcoRegPolling[ii].CollectValid == 0)
    	        {
    	            continue;
    	        }
    			if((register_info.RegistState == e_REGISTER_COLLECT) && (CcoRegPolling[ii].CollectState == e_UNFINISH))
    	        {
    	  			register_info.CrnCollectIndex = ii;	  			
    	            FindFlag = TRUE;
    	            break;
    	        }
    	    }
    	}
    }
	else
	{
         for(ii= register_info.CrnSlaveIndex;ii<MAX_WHITELIST_NUM;ii++)
        {
            if(CcoRegPolling[ii].Valid == 0)
            {
                continue;
            }

            if((register_info.RegistState == e_REGISTER_REQ) && (CcoRegPolling[ii].QueryState == e_UNFINISH))
            { 
                register_info.CrnSlaveIndex = ii;            
    			FindFlag = TRUE;
                break;
            }
            else if((register_info.RegistState == e_REGISTER_LOCK)&& (CcoRegPolling[ii].LockState == e_UNFINISH))
            {
                register_info.CrnSlaveIndex = ii;
                FindFlag = TRUE;
                break;
            }
        }
	}
   

    return FindFlag;
 }

static void register_start_request(void)
{
    SLAVE_REGISTER_START_REQ_t *pSlaveRegStartReq_t = NULL;
    
    pSlaveRegStartReq_t = (SLAVE_REGISTER_START_REQ_t*)zc_malloc_mem(sizeof(SLAVE_REGISTER_START_REQ_t), "StartRegisterReq",MEM_TIME_OUT);

    pSlaveRegStartReq_t->PacketSeq = ++ApsSendRegMeterSeq;
    memset(pSlaveRegStartReq_t->DstMacAddr, 0xFF, 6);
    __memcpy(pSlaveRegStartReq_t->SrcMacAddr, GetNnibMacAddr(), 6);

    ApsSlaveRegStartRequest(pSlaveRegStartReq_t);
	
    zc_free_mem(pSlaveRegStartReq_t);
}
static void register_stop_request(void)
{
    SLAVE_REGISTER_STOP_REQ_t *pSlaveRegStopReq_t = NULL;
    
    pSlaveRegStopReq_t = (SLAVE_REGISTER_STOP_REQ_t*)zc_malloc_mem(sizeof(SLAVE_REGISTER_STOP_REQ_t), "pSlaveRegStopReq_t",MEM_TIME_OUT);

    memset(pSlaveRegStopReq_t->DstMacAddr, 0xFF, 6);
    __memcpy(pSlaveRegStopReq_t->SrcMacAddr, GetNnibMacAddr(), 6);

    ApsSlaveRegStopRequest(pSlaveRegStopReq_t);
	
    zc_free_mem(pSlaveRegStopReq_t);
}

static void register_query_request(void *pTaskPrmt)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    REGISTER_QUERY_REQ_t *pRegist_query_request_t = NULL;
    
    pRegist_query_request_t = (REGISTER_QUERY_REQ_t*)zc_malloc_mem(sizeof(REGISTER_QUERY_REQ_t), "RegisterQueryReq",MEM_TIME_OUT);

    if(pReadTask->SendType == SLAVEREGISTER_QUERY)
        pRegist_query_request_t->RegisterPrmt = e_QUERY_REGISTER_RESULT;
    else if(pReadTask->SendType == SLAVEREGISTER_LOCK)
        pRegist_query_request_t->RegisterPrmt = e_LOCK_CMD;
    else if(pReadTask->SendType == COLLECTDATA_QUERY)
    	pRegist_query_request_t->RegisterPrmt = e_QUERY_COLLECTDATA;
        
    pRegist_query_request_t->ForcedResFlag = 0 ;  //查询从节点注册结果下行，强制标志位1强制
    pRegist_query_request_t->PacketSeq = pReadTask->DatagramSeq;
    __memcpy(pRegist_query_request_t->SrcMacAddr,GetNnibMacAddr(), 6);
    //ChangeMacAddrOrder(pRegist_query_request_t->SrcMacAddr); // 源MAC地址需要反向
	__memcpy(pRegist_query_request_t->DstMacAddr,pReadTask->CnmAddr,6);
   

    ApsSlaveRegQueryRequest(pRegist_query_request_t);
	
    zc_free_mem(pRegist_query_request_t);
}

static int32_t register_query_entrylist_check(multi_trans_header_t *list, multi_trans_t *new_list)
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
        if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO:江苏独立
        {
            if(memcmp(mbuf_n->CnmAddr, new_list->CnmAddr, 6) == 0 && g_QueryCollectFuncFlag == FALSE)
            {
                return -2;
            }
        }
        else
        {
            if(memcmp(mbuf_n->CnmAddr, new_list->CnmAddr, 6) == 0)
            {
                return -2;
            }
        }

    }

    return 0;
}

static void register_query_entrylist_fail(int32_t err_code, void *pTaskPrmt)
{
    app_printf("Entry RegisterQuery fail, err = %d\n", err_code);
    
    //RegisterInfo_t.CrnSlaveIndex++;
    if(err_code == -2)
    {
        register_info.Timeout = 10*1000;
        modify_slave_register_timer(register_info.Timeout);
    }
    else if(err_code == -1)
    {
        modify_slave_register_timer(10);
    }
	
    return;
}
                 
static void register_query_timeout(void *pTaskPrmt)
{
    app_printf("RegisterQueryTimeout\n");
}


//返回结果:TRUE表示本次记录有效，FALSE表示本次结果记录无效(包括，已经记录的和未在发送列表的)
 static U8 register_result_add(U16 StaIndex, U8 type)
 {
	U8	 result =FALSE;
                           
    if(type == SLAVEREGISTER_QUERY)
    {	
        if(CcoRegPolling[StaIndex].QueryState == e_FINISH)//已经上报
        {
            result = FALSE;
        }
        else 
        {
            CcoRegPolling[StaIndex].QueryState = e_FINISH;
            result = TRUE;
        }
    }
    else if(type == SLAVEREGISTER_LOCK)
    {
        CcoRegPolling[StaIndex].LockState = 0x01;
        result = TRUE;
    }
    else if(type == e_REGISTER_COLLECT)
    {
         if(CcoRegPolling[StaIndex].CollectState == e_FINISH)//已经上报
        {
            result = FALSE;
        }
        else 
        {
            CcoRegPolling[StaIndex].CollectState = e_FINISH;
            result = TRUE;
        }
    }

    app_printf("CcoRegPolling[%d] is TRUE\n",StaIndex);

    return result; 
 }



static void register_query_result_proc(void *pTaskPrmt, void *pUplinkData)
{
    multi_trans_t  *pReadTask = pTaskPrmt;
    REGISTER_QUERY_CFM_t  *pRegisterQueryCfm = pUplinkData;
                                            
#if defined(STATIC_MASTER)
    U16 ii;
    S16 Sqe;
    U8 nulladdr[6] ={0};
    U8  ChangeFlag = FALSE;
    QUERY_SLAVE_REG_DATA_t *pdata;
    AppGw3762SubInfo_t   *meterInfo = zc_malloc_mem(sizeof(AppGw3762SubInfo_t)*REPORT_METER_MAX, "meterInfo", MEM_TIME_OUT); 
    static U16 SlaveRegQueryConfirmnum = 0 ;
                                                   
    //app_printf("SlaveRegQueryConfirm RegisterParameter : %d!\n",pRegisterQueryCfm->RegisterParameter);
    //dump_buf(pRegisterQueryCfm->MeterInfo.pPayload, pRegisterQueryCfm->MeterInfo.PayloadLen);
        
    if(e_QUERY_REGISTER_RESULT == pRegisterQueryCfm->RegisterParameter)
    {
        if(AppGw3762Afn10F4State.WorkSwitchRegister == TRUE)
        {
            if(pRegisterQueryCfm->Status == e_SUCCESS && pRegisterQueryCfm->PacketSeq == pReadTask->DatagramSeq
                && FlashInfo_t.scJZQProtocol == e_GW13762_MSG && register_result_add(pReadTask->StaIndex, pReadTask->SendType))
            {
                SlaveRegQueryConfirmnum ++ ;
                app_printf("SlaveRegCFM= %d ,MacAddr = %02x%02x%02x%02x%02x%02x\n"
                ,SlaveRegQueryConfirmnum
                ,pRegisterQueryCfm->SrcMacAddr[0]
                ,pRegisterQueryCfm->SrcMacAddr[1]
                ,pRegisterQueryCfm->SrcMacAddr[2]
                ,pRegisterQueryCfm->SrcMacAddr[3]
                ,pRegisterQueryCfm->SrcMacAddr[4]
                ,pRegisterQueryCfm->SrcMacAddr[5]);
                U16 ii = 0;
                    
                for(ii = 0; ii<pRegisterQueryCfm->MeterCount; ii++)
                {
                    if(ii >= REPORT_METER_MAX)
                    {
                        app_printf("%d >= 32,Cfm->MeterCount %d \n",ii,pRegisterQueryCfm->MeterCount);
                        pRegisterQueryCfm->MeterCount = REPORT_METER_MAX;
                        break;
                    }
                    pdata = (QUERY_SLAVE_REG_DATA_t*)&pRegisterQueryCfm->MeterInfo[ii*sizeof(QUERY_SLAVE_REG_DATA_t)];
    
                    __memcpy(meterInfo[ii].Addr, pdata->Addr, 6);
                    meterInfo[ii].ProtoType = pdata->Protocol;

                    if(0 == ((ii + 1) % 8) || (ii + 1) == pRegisterQueryCfm->MeterCount)
                    {
                        if(TMR_STOPPED !=zc_timer_query(register_jugeover_timer))
                        {
                            timer_start(register_jugeover_timer);
                        }
                        if(0 == ii)
                        {
                            register_info.ReporttotalNum++;
                        }
                        app_gw3762_up_afn06_f4_report_dev_type(1,pRegisterQueryCfm->DeviceAddr, meterInfo[0].ProtoType,
                                                register_info.ReporttotalNum, pRegisterQueryCfm->DeviceType, pRegisterQueryCfm->MeterCount, 
                                                            (0 == ((ii + 1) % 8))?8:((ii + 1)%8) , &meterInfo[((ii/8)*8)], pReadTask->MsgPort);
                    }
                }
            }
            else
            {
                app_printf("pRegisterQueryCfm->Status != e_SUCCESS\n");
                zc_free_mem(meterInfo);
                return;
            }
    
    
            //app_printf("SlaRegInfo_t.CrntNodeIndex : %d SlaRegInfo_t.NodetotalNum : %d!\n",SlaRegInfo_t.CurrentSlaveIndex,SlaRegInfo_t.ReporttotalNum);
    
    
        }
        else
        {
            if(pRegisterQueryCfm->MeterInfoLen != pRegisterQueryCfm->MeterCount*sizeof(QUERY_SLAVE_REG_DATA_t))
            {
                app_printf("================len is incorrect!\n");
                zc_free_mem(meterInfo);
                return;
            }
            if(pRegisterQueryCfm->MeterCount == 0)
            {
                U8  MacAddr[6] = {0};
                app_printf("MeterCount NULL!  MeterCount =%d ,",pRegisterQueryCfm->MeterCount);
                __memcpy(MacAddr,pRegisterQueryCfm->SrcMacAddr,6);
                ChangeMacAddrOrder(MacAddr);
                dump_buf(MacAddr,6);

                WHLPTST //共享存储内容保护
                Sqe=seach_seq_by_mac_addr(MacAddr);
                WHLPTED
                if(-1 != Sqe)
                {
                    if(memcmp(WhiteMaCJQMapList[Sqe].CJQAddr,pRegisterQueryCfm->DeviceAddr,6) != 0)
                    {
                        __memcpy(WhiteMaCJQMapList[Sqe].CJQAddr,pRegisterQueryCfm->DeviceAddr,6);
                    }
                    app_printf("Sqe = %d mac and cnm addr,CJQAddr->",Sqe);
                    dump_buf(WhiteMacAddrList[Sqe].MacAddr , 6);
                    dump_buf(WhiteMacAddrList[Sqe].CnmAddr , 6);
                    dump_buf(WhiteMaCJQMapList[Sqe].CJQAddr , 6);
                }
            }
            else
            {
                if(register_result_add(pReadTask->StaIndex, pReadTask->SendType))
                {
                    if(TMR_STOPPED !=zc_timer_query(register_jugeover_timer))
                    {
                        timer_start(register_jugeover_timer);
                    }
                }
            }
            for(ii=0;ii<pRegisterQueryCfm->MeterCount;ii++)
            {
                if(ii >= REPORT_METER_MAX)
                {
                    app_printf("%d >= 32,Cfm->MeterCount %d \n",ii,pRegisterQueryCfm->MeterCount);
                    pRegisterQueryCfm->MeterCount = REPORT_METER_MAX;
                    break;
                }
                app_printf("SlaveRegQueryConfirm!  MeterCount =%d \n",ii);
                //dump_buf(pRegisterQueryCfm->MeterInfo.pPayload,sizeof(QUERY_SLAVE_REG_DATA_t));
                pdata=(QUERY_SLAVE_REG_DATA_t *)(pRegisterQueryCfm->MeterInfo+ii*sizeof(QUERY_SLAVE_REG_DATA_t));
                app_printf("pdata->Addr = %02x %02x %02x %02x %02x %02x \r\n" , pdata->Addr[0] , pdata->Addr[1] , pdata->Addr[2],
                                pdata->Addr[3] , pdata->Addr[4] , pdata->Addr[5]);
                                
                if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO:江苏独立
                {
                     __memcpy(meterInfo[ii].Addr, pdata->Addr, 6);
                    meterInfo[ii].ProtoType = pdata->Protocol;
                }
                //__memcpy(meterInfo[ii].Addr, pdata->Addr, 6);
                //meterInfo[ii].ProtoType = pdata->Protocol;
                WHLPTST //共享存储内容保护
                Sqe=seach_seq_by_mac_addr(pdata->Addr);
                    (-1==Sqe) ? NULL : __memcpy(WhiteMacAddrList[Sqe].CnmAddr,pRegisterQueryCfm->SrcMacAddr,6);
                WHLPTED
    
                if(-1 != Sqe)
                {
                    if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO:江苏独立
                    {
                        if(register_info.RegisterType == e_NETWORK_REGISTER)
                        {
                            if(WhiteMaCJQMapList[Sqe].ReportFlag != e_has_report ||
                                memcmp(WhiteMaCJQMapList[Sqe].CJQAddr,nulladdr,6) == 0||
                                memcmp(WhiteMaCJQMapList[Sqe].CJQAddr,pRegisterQueryCfm->DeviceAddr,6) != 0)
                            {
                                ChangeFlag = TRUE;
                            }
                            WhiteMaCJQMapList[Sqe].ReportFlag = e_has_report;
                        }
                        #if defined(ZC3750CJQ2)
    	  				ChangeMacAddrOrder(pRegisterQueryCfm->DeviceAddr);
    	  				#endif
                    }
                    __memcpy(WhiteMaCJQMapList[Sqe].CJQAddr,pRegisterQueryCfm->DeviceAddr,6);
                    app_printf("Sqe = %d mac and cnm addr,CJQAddr->",Sqe);
                    dump_buf(WhiteMacAddrList[Sqe].MacAddr , 6);
                    dump_buf(WhiteMacAddrList[Sqe].CnmAddr , 6);
                    dump_buf(WhiteMaCJQMapList[Sqe].CJQAddr , 6);
                }
                if(ChangeFlag == TRUE && (0 == ((ii + 1) % 8) || (ii + 1) == pRegisterQueryCfm->MeterCount))
                {
                    if((ii + 1) == pRegisterQueryCfm->MeterCount)
                    {
                        ChangeFlag = FALSE;
                    }
                    if(0 == ii)
                    {
                        register_info.ReporttotalNum++;
                    }
                    app_gw3762_up_afn06_f4_report_dev_type(1,pRegisterQueryCfm->DeviceAddr, meterInfo[0].ProtoType,
                                                register_info.ReporttotalNum, pRegisterQueryCfm->DeviceType, pRegisterQueryCfm->MeterCount, 
                                                        (0 == ((ii + 1) % 8))?8:((ii + 1)%8) , &meterInfo[((ii/8)*8)], pReadTask->MsgPort);
                }
            }   
        }
    }
    else if(e_LOCK_CMD == pRegisterQueryCfm->RegisterParameter  )
    {
        
        register_result_add(pReadTask->StaIndex, pReadTask->SendType);
        
        if(TMR_STOPPED !=zc_timer_query(register_jugeover_timer))
        {
		    timer_start(register_jugeover_timer);
        }
                                          
    }
    else if(e_QUERY_COLLECTDATA == pRegisterQueryCfm->RegisterParameter )
	{
		
        if(register_result_add(pReadTask->StaIndex, pReadTask->SendType))
        {
            U8  state;
            state = pRegisterQueryCfm->MeterInfo[0];
            app_printf("e_CollectSupport state = %d \n",state);
            save_collect_state(pRegisterQueryCfm->SrcMacAddr,state);
        }
	}
	zc_free_mem(meterInfo);
#endif
    
}


/*
void find_register_query_task_info(work_t *work)
{
	multi_trans_header_t *pReadListHeader = &REGISTER_QUER_LIST;
    REGISTER_QUERY_CFM_t  *pRegisterQueryCfm = (REGISTER_QUERY_CFM_t*)work->buf;

    multi_trans_t  *pReadTask;
    list_head_t *pos,*node;
	
    if(pReadListHeader->nr <= 0)
	{
		return;
	}
    
    list_for_each_safe(pos, node,&pReadListHeader->link)
    {
        pReadTask = list_entry(pos , multi_trans_t , link);
        if(pReadTask->State == EXECUTING && memcmp(pReadTask->CnmAddr, pRegisterQueryCfm->SrcMacAddr, 6) == 0)
        {
            app_printf("find matching task node.\n");
            if(pReadTask->TaskUpPro != NULL)
            {
                pReadTask->TaskUpPro(pReadTask, pRegisterQueryCfm);
            }
			pReadTask->State = NONRESP;
        }
    }
	
	return;
}
*/

static void register_send_query_req(void)
{
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    
    multi_trans_t RegisterQueryReq;

    //app_printf("Send Register Query Req.\n");
    if(register_info.RegistState == e_REGISTER_REQ)
    {
    	app_printf("RegisterInfo_t.CrnSlaveIndex = %d\n", register_info.CrnSlaveIndex);
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&register_info.CrnSlaveIndex, &GetDeviceListTemp_t);//Get_DeviceList_All(RegisterInfo_t.CrnSlaveIndex,&GetDeviceListTemp_t);
    }
    else if(register_info.RegistState == e_REGISTER_COLLECT)
    {
    	app_printf("RegisterInfo_t.CrnCollectIndex = %d\n", register_info.CrnCollectIndex);
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&register_info.CrnCollectIndex, &GetDeviceListTemp_t);
    	
    }
	dump_buf(GetDeviceListTemp_t.MacAddr,6);
	
	RegisterQueryReq.lid = SLAVE_REGISTER_LID;
	RegisterQueryReq.SrcTEI = 0;
    RegisterQueryReq.DeviceTimeout = DEVTIMEOUT;
    RegisterQueryReq.Framenum = 0;
    //__memcpy(RegisterQueryReq.Addr, AppGw3762Afn14F1State.Addr, MAC_ADDR_LEN);	
    __memcpy(RegisterQueryReq.CnmAddr, GetDeviceListTemp_t.MacAddr, MAC_ADDR_LEN);

	RegisterQueryReq.State = UNEXECUTED;
    extern U8 multi_trans_register_list_full();
    if(register_info.RegistState == e_REGISTER_REQ)
    {
		if(multi_trans_register_list_full())  //需要串口回复的载波下行入队时，要考虑串口发送队列的状态
		{
		    RegisterQueryReq.SendType = SLAVEREGISTER_QUERY;
		}
		else
		{
			return;
		}
			

		
    }    
    else if(register_info.RegistState == e_REGISTER_COLLECT)
	{
		if(multi_trans_register_list_full())
		{	
			g_QueryCollectFuncFlag = TRUE;
			RegisterQueryReq.SendType = COLLECTDATA_QUERY;
		}
		else
		{
			return;
		}
	}
    else if(register_info.RegistState == e_REGISTER_LOCK)
    {
    
		RegisterQueryReq.SendType = SLAVEREGISTER_LOCK;
    }
        
    RegisterQueryReq.StaIndex = (RegisterQueryReq.SendType == SLAVEREGISTER_QUERY ?register_info.CrnSlaveIndex:register_info.CrnCollectIndex);     
    RegisterQueryReq.DatagramSeq = ApsSendRegMeterSeq;
	RegisterQueryReq.DealySecond = REGISTER_QUER_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    RegisterQueryReq.ReTransmitCnt = RETRANMITMAXNUM;

	RegisterQueryReq.MsgPort = register_info.portID ;
    RegisterQueryReq.DltNum = 1;
	RegisterQueryReq.ProtoType = 0;
	RegisterQueryReq.FrameLen  = 0;
    RegisterQueryReq.EntryListCheck = register_query_entrylist_check;
	RegisterQueryReq.EntryListfail = register_query_entrylist_fail;  //AppGw3762UpAnf13F1UpAnf00Deny;
	RegisterQueryReq.TaskPro = register_query_request; 
    RegisterQueryReq.TaskUpPro = register_query_result_proc;
	RegisterQueryReq.TimeoutPro = register_query_timeout;

    RegisterQueryReq.pMultiTransHeader = &REGISTER_QUER_LIST;
	RegisterQueryReq.CRT_timer = register_quer_timer;
    multi_trans_put_list(RegisterQueryReq, NULL);
}


static void register_state_run(work_t *work)
{
    if(HPLC.testmode != NORM)
    {
        return;
    }
    static U8  FindFlag = FALSE;
    app_printf("SlaveRegisterInfo_t.State : %-20s\n",
										e_REGISTER_UN==register_info.RegistState?"e_REGISTER_UN":
										e_REGISTER_START==register_info.RegistState?"e_REGISTER_START":
										e_REGISTER_GET==register_info.RegistState?"e_REGISTER_GET":
										e_REGISTER_REQ==register_info.RegistState?"e_REGISTER_REQ":
										e_REGISTER_LOCK==register_info.RegistState?"e_REGISTER_LOCK":
                                        e_REGISTER_STOP==register_info.RegistState?"e_REGISTER_STOP":
                                        e_REGISTER_JZQ_STOP==register_info.RegistState?"e_REGISTER_JZQ_STOP":
                                        e_REGISTER_COLLECT==register_info.RegistState?"e_REGISTER_COLLECT":
                                                                "error state");

    if(e_REGISTER_START == register_info.RegistState)  //广播发送开始注册
    {
        register_start_request();
        clean_registe_list();
        timer_stop(g_CJQRegisterPeriodTimer,TMR_NULL);

        register_info.SlaveRegFlag = TRUE;
        register_info.Timeout = (AppGw3762Afn10F4State.WorkSwitchRegister == TRUE) ?10*1000 :300*1000;   //自有注册5分钟后开始查询
        register_info.RegistState = e_REGISTER_GET;
        register_info.CrnRegRound = 0;
        register_info.CrnSlaveIndex = 0;
        register_info.ReporttotalNum =0;
    }
    else if(e_REGISTER_GET == register_info.RegistState) //更新要注册的列表，启动注册定时器
    {
        if(regsiter_info_query_from_net() > 0)
        {
            register_info.RegistState = e_REGISTER_REQ;
            register_info.Timeout = 1000;
            
        
            FindFlag = reqister_list_index_query_from_Poll();
            if(FindFlag == FALSE)
            {
                register_info.Timeout = 60*1000;
            }
        }
        else
        {
            //FindFlag = FALSE;
            register_info.Timeout = 5*60*1000;
        }
    }
    else if(e_REGISTER_REQ==register_info.RegistState)  //注册结果查询中
    {
        FindFlag = reqister_list_index_query_from_Poll();
        if(FindFlag == TRUE)
        {
        	if(serial_cache_list_ide_num() > SERIAL_IDE_NUM)
        	{
				register_send_query_req();
				register_info.CrnSlaveIndex++;
				register_info.Timeout = 1000;
			}
			else
			{
				app_printf("serial_num3 %d\n",serial_cache_list_ide_num());
				register_info.Timeout = 1000;
			}
        }
        else
        {
            if(PROVINCE_JIANGSU == app_ext_info.province_code)//江苏独立的注册功能
            {
                register_info.Timeout = 2000;
                if(AppGw3762Afn10F4State.WorkSwitchRegister == TRUE)//11F5注册开启
                 {
    				register_info.RegistState = e_REGISTER_GET;
                 }
                 else
                 {
    				register_info.RegistState = e_REGISTER_COLLECT;
                 }
            }
            else
            {
                register_info.Timeout = (AppGw3762Afn10F4State.WorkSwitchRegister == TRUE) ? 2000 : (120*1000);
    		    register_info.RegistState = e_REGISTER_GET;//e_REGISTER_LOCK;//国网送检版本取消LOCK状态，直接进入下一轮搜表
            }
            register_info.CrnRegRound++;
    		register_info.CrnSlaveIndex = 0;
        }

    }
    else if(PROVINCE_JIANGSU == app_ext_info.province_code && e_REGISTER_COLLECT==register_info.RegistState)  // 采集支持查询
    {
        U8  FindFlag = FALSE;

        FindFlag = reqister_list_index_query_from_Poll();
        if(FindFlag == TRUE)
        {
            register_send_query_req();
            register_info.CrnCollectIndex++;
            register_info.Timeout = 1000;
        }
        else
        {
			register_info.Timeout = 2000*10;
			register_info.RegistState = e_REGISTER_GET;
	    	register_info.CrnCollectIndex = 0;
			register_info.CrnRegRound++;
        }
    }
    else if(e_REGISTER_LOCK==register_info.RegistState)  // 锁定中
    {
        U8  FindFlag = FALSE;

        FindFlag = reqister_list_index_query_from_Poll();
        if(FindFlag == TRUE)
        {
            register_send_query_req();
            register_info.CrnSlaveIndex++;
            register_info.Timeout = 1000;
        }
        else
        {
            register_info.Timeout = 20*1000;
		    register_info.RegistState = e_REGISTER_GET;//e_REGISTER_LOCK;//国网送检版本取消LOCK状态，直接进入下一轮搜表
		    register_info.CrnRegRound++;
		    register_info.CrnSlaveIndex = 0;
        }
    }
    else if (e_REGISTER_STOP == register_info.RegistState || e_REGISTER_JZQ_STOP == register_info.RegistState) // 停止
    {
        if(register_over_timer)
            timer_stop(register_over_timer,TMR_NULL);
        if(register_jugeover_timer)
            timer_stop(register_jugeover_timer,TMR_NULL);

        if(AppGw3762Afn10F4State.WorkSwitchRegister == TRUE)
        {
            AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;// // 停止从节点主动注册
            AppGw3762Afn10F4State.WorkSwitchRegister = FALSE; //注册完成需要置位FALSE

            AppGw3762Afn10F4State.RunStatusWorking = FALSE;//正在工作,注册完成需要将标志清零。

            if (e_REGISTER_STOP == register_info.RegistState)//主动停止注册不上报路由工况
            {
                app_gw3762_up_afn06_f3_report_router_change(APP_GW3762_SEARCH_TASKCHANGE, e_UART1_MSG);
                
            }
            register_info.RegistState = e_REGISTER_STOP;
            
            STARegisterFlag=FALSE;
            //暂时天津使用
            //if(Renetworkingtimer)
            {
                //timer_start(Renetworkingtimer);
            }
            AppGw3762Afn10F4State.WorkSwitchRegister = FALSE;
            //有序清除不在白名单的节点
            modify_cycle_del_slave_timer(60);
            //集中器注册结束后，CCO立即开启主动注册
//            StartRegSlave(3600*100, 0);
//            return;
        }

        //保证每次停止注册都能发出去。
        register_stop_request();

        //主动注册，CCO准备再次激活，60min后
        if(PROVINCE_JIANGSU == app_ext_info.province_code)//江苏独立的注册功能
        {
            register_info.RegisterType =  e_CJQSELF_REGISTER; 
        }

        if(PROVINCE_JIANGSU == app_ext_info.province_code && check_cjq_has() == FALSE)
        {
            register_info.RegistState = e_REGISTER_GET;
            register_info.Timeout = 5*60*1000;
            modify_slave_register_timer(register_info.Timeout);
        }
        else
        {
            app_printf("timer_start(g_CJQRegisterPeriodTimer);\n");
            timer_start(g_CJQRegisterPeriodTimer);
        }

		return;
    }

    modify_slave_register_timer(register_info.Timeout);
}




 static void slave_register_timerCB(struct ostimer_s *ostimer, void *arg)
 {
     work_t *work = zc_malloc_mem(sizeof(work_t),"Reg",MEM_TIME_OUT);
     work->work = register_state_run;
	 work->msgtype = STA_REG;
     post_app_work(work);    
 }


static void register_cjq_period_func(struct ostimer_s *ostimer, void *arg)
{
    if(register_info.RegistState != e_REGISTER_STOP)
        return;


    app_printf("register_cjq_period_func!\n");
    if(PROVINCE_JIANGSU == app_ext_info.province_code)
    {
        if(register_info.RegisterType != e_GENERAL_REGISTER)
        {   
            register_info.RegisterType = e_NETWORK_REGISTER;
            clean_registe_list();
            register_slave_start(3600*1000, 0);
        }
    }
    else
    {
        if(check_cjq_has() == FALSE)
        {
            timer_start(g_CJQRegisterPeriodTimer);
            return;
        }
        register_slave_start(3600*1000, 0);
    }
}


void change_register_run_state(uint8_t state, uint16_t Ms)
{
                                   
    register_info.RegistState = state;
    register_info.Timeout = Ms;
    
    modify_slave_register_timer(register_info.Timeout);

    return;
}


 void register_over_timerCB(struct ostimer_s *ostimer, void *arg)
 {
	app_printf("register_over_timerCB,e_REGISTER_STOP\n");
    change_register_run_state(e_REGISTER_STOP, 10);

//	timer_stop(RegisterOvertimer,TMR_NULL);
//	timer_stop(RegisterJugeOvertimer,TMR_NULL);

    return;
 }

void register_slave_start(U32 Ms, MESG_PORT_e port)
{
    if(Ms == 0)
	{
        Ms =3600*1000;  // 1小时
	}
    app_printf("RetransTime_s=%d\n",Ms);
    register_info.portID =port;
    
    if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO：江苏独立
    {
        if(register_info.RegisterType ==  e_GENERAL_REGISTER)
        {
    		 change_register_run_state(e_REGISTER_START, 10);
        }
        else
        {
    		 change_register_run_state(e_REGISTER_GET, 10);
        }
    }
    else
    {
       change_register_run_state(e_REGISTER_START, 10);
    }

     
    //开启注册超时定时器，最大注册时间由集中器给出
    if(register_over_timer ==NULL)
    {
        register_over_timer = timer_create(Ms,
                                         Ms,
                                         TMR_TRIGGER ,//TMR_TRIGGER
                                         register_over_timerCB,
                                         NULL,
                                         "register_over_timerCB"
                                         );

    }
	else
    {
        timer_modify(register_over_timer,
                     Ms,
                     Ms,
                     TMR_TRIGGER ,//TMR_TRIGGER
                     register_over_timerCB,
                     NULL,
                     "register_over_timerCB",
                     TRUE);
    }

    timer_start(register_over_timer);
    
	 //开启注册完成判断定时器，注册过程中连续N时间段内没有新的表注册，则认为注册完成
	if(register_jugeover_timer == NULL)
	{
	   register_jugeover_timer=timer_create(15*60*1000, 
         15*60*1000,
		 TMR_TRIGGER ,//TMR_TRIGGER
		 register_over_timerCB,
		 NULL,
		 "register_over_timerCB"
		 );
	}
	else
    {
        timer_modify(register_jugeover_timer,
                     15*60*1000,
                     0,
                     TMR_TRIGGER ,//TMR_TRIGGER
                     register_over_timerCB,
                     NULL,
                     "register_over_timerCB",
                     TRUE);
    }

    timer_start(register_jugeover_timer);
}

 int8_t slave_register_timer_init(void)
 {

     APP_UPLINK_HANDLE(cco_register_query_cfm_hook, register_cco_query_cfm_proc);
     if(PROVINCE_JIANGSU == app_ext_info.province_code)//TODO：江苏独立
         register_info.RegisterType = e_NETWORK_REGISTER;
         
     if(slave_register_timer == NULL)
     {
         slave_register_timer = timer_create(100,
                                 100,
                                 TMR_TRIGGER,//TMR_TRIGGER
                                 slave_register_timerCB,
                                 NULL,
                                 "slave_register_timerCB"
                                );
     }
 
     //timer_start(slave_register_timer);
     //默认创建延时任务定时器
     register_suspend_timer_init();

     if(g_CJQRegisterPeriodTimer == NULL)
     {
         g_CJQRegisterPeriodTimer = timer_create(3600*1000,
                                                 0,
                                                 TMR_TRIGGER,
                                                 register_cjq_period_func,
                                                 NULL,
                                                 "g_CJQRegisterPeriodTimer"
                                                 );
     }

     return TRUE;
 }
 
 
 void modify_slave_register_timer(uint32_t Ms)
 {
     if(Ms == 0)
        return;
     
     if(NULL != slave_register_timer)
     {
         timer_modify(slave_register_timer,
                 Ms,
                 100,
                 TMR_TRIGGER ,//TMR_TRIGGER
                 slave_register_timerCB,
                 NULL,
                 "slave_register_timerCB",
                 TRUE);
     }
     else
     {
         sys_panic("slave_register_timer is null\n");
     }
     timer_start(slave_register_timer);

     return;
 }
static void register_suspend_timerCB(struct ostimer_s *ostimer, void *arg)
{			
    app_printf("register_suspend_timerCB Time out ,need resume!\n");
    
}

void modify_register_suspend_timer(uint32_t Sec)
{
    if(register_suspend_timer == NULL)
	{
	    register_suspend_timer = timer_create(Sec*1000,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
	                            register_suspend_timerCB,
	                            NULL,
	                            "register_suspend_timerCB"
	                           );
		
	}
	else
	{
        timer_modify(register_suspend_timer,
				Sec*1000,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				register_suspend_timerCB,
				NULL,
                "register_suspend_timerCB",
                TRUE);
	}
    if(Sec != 0)
    {
        timer_start(register_suspend_timer);
    }

}
void register_suspend_timer_init(void)
{
    register_suspend_timer = timer_create(30*1000,
                                    0,
                                    TMR_TRIGGER ,//TMR_TRIGGER
                                    register_suspend_timerCB,
                                    NULL,
                                    "register_suspend_timerCB"
                                   );

}
#endif



