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

//#include "RouterTask.h"
//#include "NetTable.h"
#include "ZCsystem.h"
#include "app_area_indentification_cco.h"
#include "datalinkdebug.h"
#include "app_slave_register_cco.h"
#include "app_common.h"
#include "SchTask.h"
#include "app_dltpro.h"
#include "netnnib.h"
#include "app_data_freeze_cco.h"


#if defined(STATIC_MASTER)
//并发支持的条目数，F1F1支持配置信息修改
int8_t HIGHEST_PRIORITY_LIST_NUM = 5;
int8_t F1F1_TRANS_LIST_NUM = MAX_WIN_SIZE;
int8_t F14F1_TRANS_LIST_NUM = 1;
int8_t REGISTER_QUER_LIST_NUM = 5;
int8_t UPGRADE_START_LIST_NUM = 10;
int8_t UPGRADE_QUERY_LIST_NUM = 5;
int8_t INDENTIFICATION_COLLECT_BF_LIST_NUM = 5;
int8_t PHASE_POSITION_BF_LIST_NUM = 2;
int8_t MODULE_ID_QUERY_LIST_NUM = 2;
int8_t CLOCK_MANAGE_LIST_NUM   = 5;


multi_trans_header_t  HIGHEST_PRIORITY_LIST;	ostimer_t *highest_priority_timer=NULL;
ostimer_t *highest_priority_guard_timer=NULL;

multi_trans_header_t  F1F1_TRANS_LIST;  	    ostimer_t *f1f1_trans_timer = NULL;
ostimer_t *f1f1_trans_guard_timer = NULL;

multi_trans_header_t  F14F1_TRANS_LIST; 		ostimer_t *f14f1_trans_timer = NULL;
ostimer_t *f14f1_trans_guard_timer = NULL;

multi_trans_header_t  REGISTER_QUER_LIST; 	 	ostimer_t *register_quer_timer =NULL;
ostimer_t *register_quer_guard_timer = NULL;



multi_trans_header_t  UPGRADE_START_LIST;       ostimer_t *upgrade_start_timer = NULL;
ostimer_t *upgrade_start_guard_timer = NULL;
CONCURRENCY_CTRL_INFO_t  CTRL_UPGRADE_START_Info;

multi_trans_header_t  UPGRADE_QUERY_LIST;       ostimer_t *upgrade_query_timer = NULL;
ostimer_t *upgrade_query_guard_timer = NULL;
CONCURRENCY_CTRL_INFO_t  CTRL_UPGRADE_QUERY_Info;

multi_trans_header_t  UPGRADE_STATION_INFO_QUERY_LIST;       ostimer_t *upgrade_station_info_query_timer = NULL;
ostimer_t *upgrade_station_info_query_guard_timer = NULL;

CONCURRENCY_CTRL_INFO_t  CTRL_UPGRADE_STATION_INFO_QUERY_Info;


multi_trans_header_t  INDENTIFICATION_COLLECT_BF_LIST;ostimer_t *indentification_multi_timer =NULL;
ostimer_t *indentification_multi_guard_timer = NULL;
multi_trans_header_t  PHASE_POSITION_BF_LIST;   ostimer_t *phaseposition_multi_timer=NULL;
ostimer_t *phaseposition_multi_guard_timer=NULL;


multi_trans_header_t  MODULE_ID_QUERY_LIST;     ostimer_t *moduleid_query_timer = NULL;
ostimer_t *moduleid_query_guard_timer = NULL;
CONCURRENCY_CTRL_INFO_t  CTRL_MODULE_ID_QUERY_Info;

multi_trans_header_t  QUERYSWORVALUE_LIST;      ostimer_t *query_sworvalue_timer = NULL;
ostimer_t *query_sworvalue_guard_timer=NULL;

multi_trans_header_t  SETSWORVALUE_LIST;		ostimer_t *set_sworvalue_timer = NULL;
ostimer_t *set_sworvalue_guard_timer=NULL;

manage_link_header_t manage_link_header;
manage_timer_t manage_timer_link_header;
multi_ctrl_timer_t multi_ctrl_timer_list[MAX_CTRL_TIMER_NUM];


//static void modify_multi_trans_timer(multi_trans_header_t	 *pMultiTransHeader,ostimer_t *multi_trans_timer,uint32_t first);
static void multi_trans_one_timer_init(ostimer_t **ptimer );
static void multi_trans_modify_guard_timer(uint32_t first,ostimer_t **ptimer,void *arg);
static void multi_trans_modify_one_timer(uint32_t first,ostimer_t **ptimer,void *arg);
static void *multi_trans_get_timer_from_list(void);
static int8_t multi_trans_put_timer_into_list(multi_ctrl_timer_t *pheader);


//static U8 get_translsit_borcast_num(multi_trans_header_t *p)
//{
//	list_head_t *pos,*node;
//	multi_trans_t  *mbuf_n;	
//	U8 broadcastaddr[6];
//	U8 num=0;
//	if(p->nr==0)return 0;
//	memset(broadcastaddr,0xff,6);
//	list_for_each_safe(pos, node,&p->link) 
//	{
//		mbuf_n = list_entry(pos, multi_trans_t, link);
//		if(memcmp(mbuf_n->CnmAddr,broadcastaddr,6) ==0)num++;
//		
//	}
//	net_printf("!!!!!!!borcast num=%d\n",num);
//	return num;
//}

int8_t manage_link_init(void)
{
	INIT_LIST_HEAD(&manage_link_header.vertical_link);
	manage_link_header.nr = 0;
	manage_link_header.thr = 10;	
	return TRUE;
}

static int8_t manage_link_add(multi_trans_header_t *pheader)
{
	/*over threshold , drop the msg*/
	if(manage_link_header.nr >= manage_link_header.thr)
	{
		return -1;
	}	 
	 list_add_tail(&pheader->vertical_link, &manage_link_header.vertical_link);
	 ++manage_link_header.nr;
	 return 0;

}

//static int8_t check_canbe_execute(int8_t lid)
//{
//	list_head_t *pos,*node;
//	multi_trans_header_t  *msgheader;
//	 list_for_each_safe(pos, node,&manage_link_header.vertical_link) 
//	 {
//	 	msgheader = list_entry(pos, multi_trans_header_t, vertical_link);
//		if( (msgheader->lid < lid ) &&  msgheader->nr)
//		{
//			return FALSE;
//		}
//	 }
//	 return TRUE;
//}


static int32_t multi_trans_insert_list(multi_trans_header_t *list, multi_trans_t *new_list)
{
	list_head_t *pos,*node,*p;
	multi_trans_t  *mbuf_n;
    int32_t  ret;

    //app_printf("-----------------insert list!----------------------------\n");
                                    
    if(new_list->EntryListCheck != NULL)
    {
        if((ret = new_list->EntryListCheck(list, new_list)) != 0)
            return ret;
    }
	
	/*insert list by lid*/
	list_for_each_safe(pos, node,&list->link)
	{
		mbuf_n = list_entry(pos, multi_trans_t, link);
		if(time_before_eq(new_list->lid,mbuf_n->lid))
		{
			p= &mbuf_n->link;
			goto ok;
		}
	}
	p=&list->link;

	//app_printf("insert list success.!\n");
	
   ok:
	list_add_tail(&new_list->link, p);
	++list->nr;
	return 0;
}

static void multi_trans_entry_list(work_t *work)
{
    int32_t ret;
    multi_ctrl_timer_t       *pTimerHeader;         /*单独控制定时器*/
	multi_trans_t *rm_info = (multi_trans_t *)work->buf;
	multi_trans_t *new_list = zc_malloc_mem(sizeof(multi_trans_t)+rm_info->FrameLen,"PRML",MEM_TIME_OUT);
	if(new_list == NULL)
	{
		app_printf("new_list NULL\n");
		return;
	}
	__memcpy((uint8_t *)new_list,work->buf,sizeof(multi_trans_t)+rm_info->FrameLen);

    //app_printf("new_list->pMultiTransHeader address is %08x\n", new_list->pMultiTransHeader);
    //multi_ctrl_timer_t * pheader;
    pTimerHeader = multi_trans_get_timer_from_list();
    if(pTimerHeader == NULL)
    {
        zc_free_mem(new_list);
        return;
    }
    new_list->pTimerHeader = pTimerHeader;
    //__memcpy(new_list->pTimerHeader , pTimerHeader , sizeof(multi_ctrl_timer_t));
    //printf_s("pTimerHeader = %p,seq = %d,timer = %p\n",new_list->pTimerHeader,new_list->pTimerHeader->seq,new_list->pTimerHeader->ctrl_timer);
	if((ret = multi_trans_insert_list(new_list->pMultiTransHeader,new_list))!=0)
	{
	    if(new_list->EntryListfail != NULL)
	    {
	    	app_printf("F1F1_TRANS_LIST.nr=%d", F1F1_TRANS_LIST.nr);  //抄表测试用
            new_list->EntryListfail(ret, new_list);
        }
        multi_trans_put_timer_into_list(new_list->pTimerHeader);
		zc_free_mem(new_list);
	}
	else
    {
        new_list->ProcCnt = 0;
	    if(TMR_STOPPED==zc_timer_query(new_list->pTimerHeader->ctrl_timer))//you can send rigth now
	    {
	        //printf_s("new_list = %p\n",new_list);
            U32 times;
            times = new_list->pMultiTransHeader->nr == 0?1:1;//1*new_list->pMultiTransHeader->nr
            //printf_s("times = %d\n",times);
		    multi_trans_modify_one_timer(times, &new_list->pTimerHeader->ctrl_timer,new_list);
	    }
    }
	return;
}

static void multi_trans_check_auto_add(multi_trans_header_t  *pReadListHeader)
{
    if(pReadListHeader->nr < pReadListHeader->thr)
	{
	    if(pReadListHeader->TaskAddOne != NULL)
	    {
            pReadListHeader->TaskAddOne();      //add_one;
        }           
    }
}

static uint8_t multi_trans_del_list(multi_trans_t  *pReadTask)
{
	if(pReadTask->pMultiTransHeader->nr >0)
    {
        list_del(&pReadTask->link);
		pReadTask->pMultiTransHeader->nr--;
        
        multi_trans_put_timer_into_list(pReadTask->pTimerHeader);
        multi_trans_check_auto_add(pReadTask->pMultiTransHeader);
        zc_free_mem(pReadTask);

		app_printf("mu del\n");
        return TRUE;
    }

	return FALSE;
}


static void multi_trans_update_info(work_t *work)
{
	//static U8 execuitflag=FALSE;
    //list_head_t *pos,*node;

    multi_trans_t  *pReadTask;//,*msgwork;
    U32 p;
    multi_trans_header_t  *pReadListHeader;
    //msgwork = (multi_trans_t *)work->buf;
	//__memcpy((void*)pReadTask,work->buf,sizeof(void*));
	//printf_s("buf = %p,p %02x%02x%02x%02x\n",work->buf,work->buf[0],work->buf[1],work->buf[2],work->buf[3]);
    __memcpy((U8 *)&p,work->buf,4);
	pReadTask = (multi_trans_t *)p;
    //printf_s("pReadTask = %p\n",pReadTask);
    
	pReadListHeader = pReadTask->pMultiTransHeader;
    //printf_s("pReadListHeader = %p\n",pReadListHeader);
    if(pReadTask->ProcCnt > 0)
    {
        //printf_s("ProcCnt = %d\n",pReadTask->ProcCnt);
        pReadTask->ProcCnt --;
    }
    //pReadTask->GuardFlag = 0;
    // add at first
    multi_trans_check_auto_add(pReadListHeader);
	
    /*
	if(pReadListHeader->nr == 0)
	{
	    if(zc_timer_query(pReadTask->pTimerHeader->ctrl_timer) == TMR_RUNNING)
    	{
			timer_stop(pReadTask->pTimerHeader->ctrl_timer, TMR_NULL);
    	}
		return; //结束任务
    }*/
    
	//pReadTask = msgwork;
	U32 time;
    time = 0;
    if(pReadTask->State == EXECUTING)
    { 
        if(pReadTask->ReTransmitCnt <=0)
        {
            // 超时处理删除、串口上报 
	        if(pReadTask->TimeoutPro != NULL)
            {
                pReadTask->TimeoutPro(pReadTask);
                app_printf("read task timeout, serial_num=%02x\n", pReadTask->Framenum);
            }               
		
            // 超时处理完后，切换下一条 
            if(pReadTask->ProcCnt == 0)
            {
			    multi_trans_del_list(pReadTask);
            }
        }
        else
        {
            // 执行载波任务、填写APS序号++、建立单条超时 
	        if(pReadTask->TaskPro != NULL)
	        {
        	    app_printf("pReadTask->TaskPro1\n");
        		pReadTask->TaskPro(pReadTask);
	        }
            pReadTask->ReTransmitCnt --;
            time = pReadTask->DeviceTimeout;
            multi_trans_modify_one_timer(time,&pReadTask->pTimerHeader->ctrl_timer,pReadTask);
        }
    }
    else if(pReadTask->State == UNEXECUTED)
    {
        if(pReadTask->TaskPro != NULL)
		{
            pReadTask->TaskPro(pReadTask);
            app_printf("pReadTask->TaskPro2\n");
		}
        pReadTask->State = EXECUTING;
        pReadTask->ReTransmitCnt --;
        time = pReadTask->DeviceTimeout;
        multi_trans_modify_one_timer(time,&pReadTask->pTimerHeader->ctrl_timer,pReadTask);
    }
    else if(pReadTask->State == NONRESP)
    {
        // 上行均处理完后，切换下一条 
        if(pReadTask->ProcCnt == 0)
        {
		    multi_trans_del_list(pReadTask);
        }
    }
	else
	{
		;
	}
    // check round at last
    if(pReadListHeader->nr == 0)
	{
	    // add at last ,if the pReadListHeader->thr is only 1
        multi_trans_check_auto_add(pReadListHeader);
        // must execute TaskRoundPro
        if(pReadListHeader->nr == 0)
        {
            if(pReadListHeader->TaskRoundPro != NULL)
            {
                pReadListHeader->TaskRoundPro();    //multi_trans_ctrl_plc_task_round(list);
            //return ;
            } 
        }
	}
}

static void multi_trans_guard_info(work_t *work)
{
	multi_trans_header_t *multi_header;
	multi_header = (multi_trans_header_t *)work->buf;
    multi_trans_t  *pReadTask;
    list_head_t *pos,*node;

    if(multi_header->nr <= 0)
    {
		return;
	}
    list_for_each_safe(pos, node,&multi_header->link)
    {
        pReadTask = list_entry(pos , multi_trans_t , link);
        if(pReadTask)
        {
            app_printf("guard mu %02x%02x%02x%02x%02x%02x\n",pReadTask->CnmAddr[0],pReadTask->CnmAddr[1],
                pReadTask->CnmAddr[2],pReadTask->CnmAddr[3],pReadTask->CnmAddr[4],pReadTask->CnmAddr[5]);
            multi_trans_del_list(pReadTask);
        }
    }
	
	return;
}

/*

void find_plc_multi_task_info(work_t *work)
{
	multi_trans_header_t *pReadListHeader;
    READ_METER_IND_t    *pReadMeterInd = (READ_METER_IND_t*)work->buf;

    multi_trans_t  *pReadTask;
    list_head_t *pos,*node;


	pReadListHeader = pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_RM ? &HIGHEST_PRIORITY_LIST:
					  pReadMeterInd->ReadMode == e_ROUTER_ACTIVE_RM ? &F14F1_TRANS_LIST:
					  pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_RM ? &F1F1_TRANS_LIST:
					  NULL;
	if(pReadListHeader ==NULL)
	{
		app_printf("pReadMeterInd->ReadMode is Unkown!\n");
	}
    if(pReadListHeader->nr <= 0)
	{
		return;
	}
    
    list_for_each_safe(pos, node,&pReadListHeader->link)
    {
        pReadTask = list_entry(pos , multi_trans_t , link);
        if(pReadTask->State == EXECUTING && pReadTask->DatagramSeq == pReadMeterInd->ApsSeq)
        {
            if(pReadTask->TaskUpPro != NULL)
            {
                pReadTask->TaskUpPro(pReadTask, pReadMeterInd);
            }
			pReadTask->State = NONRESP;
        }
    }
	
	return;
}
*/

/*************************************************************************
 * 函数名称	: 	multi_trans_find_plc_task_up_info
 * 函数说明	: 	并发任务上行处理接口
 * 参数说明	: 	MULTI_TASK_UP_t *pMultiTaskUp		- 任务上行服务原语
 *				void *pUplinkData		            - 业务上行服务原语
 * 返回值	: 	无
 *************************************************************************/
void multi_trans_find_plc_task_up_info(MULTI_TASK_UP_t *pMultiTaskUp, void *pUplinkData)
{
	multi_trans_header_t *pReadListHeader = pMultiTaskUp->pListHeader; //&MULTI_TRANS_LIST;
    U8  *pTemo = NULL;
    U8  *pUpData = (U8*)pUplinkData;
    multi_trans_t  *pReadTask;
    list_head_t *pos,*node;
    INT8U find_tsk_flag = FALSE;
	
    if(pReadListHeader->nr <= 0)
    {
        app_printf("nr <= 0\n");
		return;
	}
    
    list_for_each_safe(pos, node,&pReadListHeader->link)
    {
        pReadTask = list_entry(pos , multi_trans_t , link);
        pTemo = (U8 *)pReadTask;
        //判定条件使用固定偏移之间的对比上下行条件
        if(pReadTask->State == EXECUTING && 0 == memcmp(&pTemo[pMultiTaskUp->Downoffset], 
                                                                &pUpData[pMultiTaskUp->Upoffset], pMultiTaskUp->Cmplen))
        {
            if(pReadTask->TaskUpPro != NULL)
            {
                pReadTask->TaskUpPro(pReadTask, pUplinkData);
            }
            pReadTask->ReTransmitCnt = 0;
			pReadTask->State = NONRESP;
			//multi_trans_modify_one_timer(2,&pReadTask->pTimerHeader->ctrl_timer,pReadTask);
			//有效上行通过定时器发消息统一删除
			if(zc_timer_query(pReadTask->pTimerHeader->ctrl_timer) == TMR_RUNNING)
            {
                app_printf("CT");
                timer_stop(pReadTask->pTimerHeader->ctrl_timer,TMR_CALLBACK);
            }
			app_printf("recvup--, serial_num= %02x, plc_num=%04x\n", pReadTask->Framenum, pReadTask->DatagramSeq);
            find_tsk_flag = TRUE;
            break;
			//multi_trans_del_list(pReadTask);
            //multi_trans_check_auto_add(pReadListHeader);
			/*
			list_del(&pReadTask->link);
			pReadListHeader->nr--;
            multi_trans_put_timer_into_list(pReadTask->pTimerHeader);
            multi_trans_check_auto_add(pReadListHeader);
			zc_free_mem(pReadTask);
			app_printf("recvup--delete list node.\n");*/
        }
    }
    if(FALSE == find_tsk_flag)
    {
        app_printf("find_tsk_flag=%d, d_ot=%d, u_ot=%d, cmp_len=%d, u_seq=%02x%02x\n", find_tsk_flag, pMultiTaskUp->Downoffset, pMultiTaskUp->Upoffset, pMultiTaskUp->Cmplen,  pUpData[pMultiTaskUp->Upoffset],
            pUpData[pMultiTaskUp->Upoffset+1]);
        dump_buf(pUpData, 50);
    }
	
	return;
}

void multi_trans_find_plc_multi_task_send_fail(MULTI_TASK_UP_t *pMultiTaskUp, void *pUplinkData)
{
	multi_trans_header_t *pReadListHeader = pMultiTaskUp->pListHeader; //&MULTI_TRANS_LIST;
    U8  *pTemo = NULL;
    U8  *pUpData = (U8*)pUplinkData;
    multi_trans_t  *pReadTask;
    list_head_t *pos,*node;
	
    if(pReadListHeader->nr <= 0)
    {
		return;
	}
    
    list_for_each_safe(pos, node,&pReadListHeader->link)
    {
        pReadTask = list_entry(pos , multi_trans_t , link);
        pTemo = (U8 *)pReadTask;
        //判定条件使用固定偏移之间的对比上下行条件
        if(pReadTask->State == EXECUTING && 0 == memcmp(&pTemo[pMultiTaskUp->Downoffset], 
                                                                &pUpData[pMultiTaskUp->Upoffset], pMultiTaskUp->Cmplen))
        {
            //if(pReadTask->TaskSendFail != NULL)
            //{
            //    pReadTask->TaskSendFail(pReadTask, pUplinkData);
            //}
            //重新发给链路层，记录重试次数
            if(pReadTask->SendRetryCnt < CONTASKSENDMAX)
            {
                pReadTask->SendRetryCnt ++;
                pReadTask->ReTransmitCnt ++;
            
    			//超时通过定时器发消息统一重发
    			if(zc_timer_query(pReadTask->pTimerHeader->ctrl_timer) == TMR_RUNNING)
                {
                    app_printf("CA ");
                    //if(timer_remain(&pReadTask->pTimerHeader->ctrl_timer))
                    multi_trans_modify_one_timer(10,&pReadTask->pTimerHeader->ctrl_timer,pReadTask);
                }
    			app_printf("retrysendcnt-%d-%d-aps ",pReadTask->SendRetryCnt,pReadTask->ReTransmitCnt);
                dump_buf(&pUpData[pMultiTaskUp->Upoffset], pMultiTaskUp->Cmplen);
                break;
            }
        }
    }
	
	return;
}

/*************************************************************************
 * 函数名称	: 	multi_trans_check_broadcast_num
 * 函数说明	: 	并发任务广播任务数量
 * 参数说明	: 	multi_trans_t  *pOneTask		- 任务原语
 * 返回值	: 	广播数量
 *************************************************************************/
uint8_t  multi_trans_check_broadcast_num(multi_trans_t  *pOneTask)
{
    multi_trans_header_t *pReadListHeader = pOneTask->pMultiTransHeader; //&MULTI_TRANS_LIST;
    multi_trans_t  *pReadTask;
    list_head_t *pos,*node;
	uint8_t  BroadcastNum = 0;
    
    if(pReadListHeader->nr <= 0)
    {
		return 0;
	}
    
    list_for_each_safe(pos, node,&pReadListHeader->link)
    {
        pReadTask = list_entry(pos , multi_trans_t , link);
        //判定当前广播条数
        if(pReadTask->BroadcastNow == TRUE)
        {
            BroadcastNum ++;
        }
    }
	
    app_printf("BroadcastNum=%d!\n",BroadcastNum);
    return BroadcastNum;
}

static uint8_t multi_trans_check_task_poll_finish(CONCURRENCY_CTRL_INFO_t *pConInfo)
 {
    uint16_t ii ;
    
 	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
 	{
 		if(pConInfo->taskPolling[ii].Valid == 1 &&  pConInfo->taskPolling[ii].Result == 0)
        {
        return FALSE;
 	}
 	}
	return TRUE;
 }

/*************************************************************************
 * 函数名称	: 	multi_trans_ctrl_plc_task_round
 * 函数说明	: 	任务轮次控制接口
 * 参数说明	: 	multi_trans_header_t *list          - 任务原语
                CONCURRENCY_CTRL_INFO_t *pConInfo	- 并发任务控制原语
 * 返回值	: 	无
 *************************************************************************/
void multi_trans_ctrl_plc_task_round(multi_trans_header_t *list,CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    app_printf("ctrl : 0x%08x\n",list);
	
	if(list->nr <= 0)
	{
		if(pConInfo->CycleNum > 0) //
		{
			pConInfo->CycleNum --;
		}
        
		// 任务全部完成
		if(multi_trans_check_task_poll_finish(pConInfo)==TRUE)
		{
			// 切换至下一任务 
			pConInfo->TaskNext();
			app_printf("TaskNext\n");
			return;
		}

		if(pConInfo->CycleNum == 0)
		{
			// 结束任务 或切换下一任务
			pConInfo->TaskNext();
			app_printf("TaskNext\n");
		}
		else
		{
			pConInfo->CurrentIndex = 0;
			multi_trans_check_auto_add(list);
		}
	}

    return ;
}

//获取并发列表
//参数：并发任务类型
void get_info_from_net_set_poll(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
    U16 ii ;
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    app_printf("seq\ttei\tresult\n");
    for(ii = 0;ii < MAX_WHITELIST_NUM;ii++)
    {
        if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp_t)==TRUE)
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


//从任务池中查找一个添加到发送窗口中
/*************************************************************************
 * 函数名称	: 	multi_trans_add_task_form_poll
 * 函数说明	: 	从任务池中添加任务
 * 参数说明	: 	CONCURRENCY_CTRL_INFO_t *pConInfo		- 并发任务控制原语
 * 返回值	: 	TRUE-找到任务，FALSE-没找到任务
 *************************************************************************/
uint8_t multi_trans_add_task_form_poll(CONCURRENCY_CTRL_INFO_t *pConInfo)
{
	uint16_t ii ;
	DEVICE_TEI_LIST_t GetDeviceListTemp_t;

    //app_printf("AddTaskFormPoll CurrentIndex=%d\n",pConInfo->CurrentIndex);
	for(ii= pConInfo->CurrentIndex ;ii<MAX_WHITELIST_NUM;ii++)
    {
        if(pConInfo->taskPolling[ii].Valid != 0 )
        {
	        if( pConInfo->taskPolling[ii].Result == 0&&pConInfo->taskPolling[ii].Send == 0)
			{
				if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp_t)==TRUE)
                {            
				    __memcpy(pConInfo->CrntNodeAdd,GetDeviceListTemp_t.MacAddr, 6);
                    pConInfo->taskPolling[ii].Send = 1;
				    pConInfo->CurrentIndex = ii;
                    //app_printf("Ix %d \n",pConInfo->CurrentIndex);
                    return TRUE;
                }
			}
        }    
    }

	if(ii >= MAX_WHITELIST_NUM )
	{
		pConInfo->CurrentIndex= 0;
	
	}
    app_printf("pConInfo->CurrentIndex= MAX_WHITELIST_NUM;\n");
	return FALSE;
}

/*************************************************************************
 * 函数名称	: 	multi_trans_add_con_result
 * 函数说明	: 	任务处理结果更新接口
 * 参数说明	: 	CONCURRENCY_CTRL_INFO_t *pConInfo	- 并发任务控制原语
 *              uint16_t Index                      - 任务索引
 * 返回值	: 	TRUE-添加成功，FALSE-添加失败
 *************************************************************************/
uint8_t multi_trans_add_con_result(  CONCURRENCY_CTRL_INFO_t *pConInfo,uint16_t Index)
{
    if(pConInfo->taskPolling[Index].Valid == 1)
    {
        pConInfo->taskPolling[Index].Result = 1;
     	app_printf("ACT:%d\n",Index);
        return TRUE;  
    }
    else
    {
        app_printf("ACF:%d\n",Index);
    }
    return FALSE;
}

/*************************************************************************
 * 函数名称	: 	multi_trans_stop_plc_task
 * 函数说明	: 	并发任务停止接口
 * 参数说明	: 	void *buf    - 任务原语
 * 返回值	: 	无
 *************************************************************************/
void multi_trans_stop_plc_task(void *buf)
{
    list_head_t *pos,*node;
    multi_trans_t  *pReadTask;
    multi_trans_header_t  *pReadListHeader;
	
	pReadListHeader = (multi_trans_header_t *)buf;

    if(pReadListHeader->TaskAddOne != NULL)
    {
        pReadListHeader->TaskAddOne = NULL;      
    }           

    list_for_each_safe(pos, node,&pReadListHeader->link)
	{
		pReadTask = list_entry(pos, multi_trans_t, link);
		
        pReadTask->State = UNEXECUTED;
    }
}


/*任务调用*/
/*************************************************************************
 * 函数名称	: 	multi_trans_put_list
 * 函数说明	: 	任务入队接口
 * 参数说明	: 	multi_trans_t meter		- 任务原语
 *				uint8_t *payload		- 任务载荷
 * 返回值	: 	无
 *************************************************************************/
void multi_trans_put_list(multi_trans_t meter,uint8_t *payload)
{
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(multi_trans_t)+meter.FrameLen,"PMTL",MEM_TIME_OUT);
    if(NULL == work)
    {
        return;
    }
	multi_trans_t *rm_info = (multi_trans_t *)work->buf;
    
	__memcpy(work->buf,(uint8_t *)&meter,sizeof(multi_trans_t));
	__memcpy(rm_info->FrameUnit,payload,meter.FrameLen);
    
	work->work = multi_trans_entry_list;
	work->msgtype = IN_MULTI_LIST;
	post_app_work(work);
}


static void multi_trans_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    multi_trans_t *pMultiTransOne;// = (multi_trans_t*)arg;
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(void *)+sizeof(multi_trans_t),"Mult",MEM_TIME_OUT);
    if(NULL == work)
    {
        return;
    }

	__memcpy(work->buf,&arg,sizeof(void *));
    pMultiTransOne = (multi_trans_t *)arg;
    //防止APP丢消息，如果APP没处理消息，操作数重置，让multi_trans_update_info 直接按结果处理
    //if(pMultiTransOne->GuardFlag == 1)
    //{
    //    pMultiTransOne->ProcCnt = 0;
    //}
    pMultiTransOne->GuardFlag = 1;
    //当载波上行和定时器超时时间交叉触发处理条目时，按照ProcCnt条目判定是否可以删除
    pMultiTransOne->ProcCnt ++;
    //app_printf("mu_timer = %p,%d\n",pMultiTransOne->pTimerHeader->ctrl_timer,pMultiTransOne->ProcCnt);
	work->work = multi_trans_update_info;
    
	work->msgtype = UP_MULTI_LIST;
	post_app_work(work);	
    if(pMultiTransOne->pMultiTransHeader->Guard_timer)
    {
        multi_trans_modify_guard_timer(GUARD_MULTI_TIME,&pMultiTransOne->pMultiTransHeader->Guard_timer,pMultiTransOne->pMultiTransHeader);
    }
    //multi_trans_modify_one_timer(GUARD_MULTI_TIME,&pMultiTransOne->pTimerHeader->ctrl_timer,arg);
}
 
 static void multi_trans_guard_timer_cb(struct ostimer_s *ostimer, void *arg)
 {
    work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(multi_trans_header_t),"MultG",MEM_TIME_OUT);
    if(NULL == work)
    {
        return;
    }
    multi_trans_header_t *multi_header = (multi_trans_header_t *)work->buf;
    multi_header = (multi_trans_header_t*)arg;
    work->msgtype = MULTI_GUARD;

    //serial_cache_update_info(work);
    //zc_free_mem(work);
    work->work = multi_trans_guard_info;
    if(multi_header->nr > 0)
    {
        post_app_work(work);	
        multi_trans_modify_guard_timer(GUARD_MULTI_TIME,&multi_header->Guard_timer,arg);
    }
    else
    {
        zc_free_mem(work);
    }
 }
 
 static void multi_trans_modify_guard_timer(uint32_t first,ostimer_t **ptimer,void *arg)
 {
     //对first值做保护
     if(first == 0)
     {
         app_printf("multi_trans_modify_guard_timer = 0\n");
         return;
     }   

     if(NULL != *ptimer)
     {
//         if(first == 1)
//         {
//         }
//         else
//         {
//             first = first*10;
//         }
         //app_printf("one_timer = %d\n",first);
         timer_modify(*ptimer,
                 first,
                 0,
                 TMR_TRIGGER ,//TMR_TRIGGER
                 multi_trans_guard_timer_cb,
                 arg,
                 "multi_trans_guard_timer_cb"
                 , TRUE);
     }
     else
     {
         sys_panic("*ptimer is null\n");
     }
     timer_start(*ptimer);
 }

//static uint8_t multi_trans_task_init(multi_trans_header_t *pHeader, ostimer_t **ptimer ,
//                                        uint16_t nr, uint16_t thr, uint8_t lid, 
//                                        void* TaskAddOne,void* TaskRoundPro,ostimer_t 	*pSuspend_timer)
//{
//    INIT_LIST_HEAD(&pHeader->link);
//	pHeader->nr = nr;
//	pHeader->thr = thr;
//	pHeader->lid = lid; //SEVER TASK
//	pHeader->TaskAddOne = TaskAddOne;
//    pHeader->TaskRoundPro = TaskRoundPro;
//    pHeader->Suspend_timer = pSuspend_timer;
//
//	manage_link_add(pHeader);
//
//    if(*ptimer == NULL)
//	{
//        *ptimer = timer_create(100,
//                                100,
//	                            TMR_PERIODIC,//TMR_TRIGGER
//	                            multi_trans_timer_cb,
//	                            pHeader,
//	                            "multi_trans_timer_cb"
//	                           );
//	}
//    else
//    {
//        timer_modify(*ptimer,
//                                100,
//                                100,
//	                            TMR_PERIODIC,//TMR_TRIGGER
//	                            multi_trans_timer_cb,
//	                            pHeader,
//                                "multi_trans_timer_cb",
//                                TRUE);
//    }
//
//    return TRUE;
//}

uint8_t multi_trans_task_self_init(multi_trans_header_t *pHeader,
                                        uint16_t nr, uint16_t thr, uint8_t lid, 
                                        void* TaskAddOne,void* TaskRoundPro,ostimer_t 	*pSuspend_timer,
                                        ostimer_t 	**pGuard_timer)
{
    INIT_LIST_HEAD(&pHeader->link);
	pHeader->nr = nr;
	pHeader->thr = thr;
	pHeader->lid = lid; //SEVER TASK
	pHeader->TaskAddOne = TaskAddOne;
    pHeader->TaskRoundPro = TaskRoundPro;
    pHeader->Suspend_timer = pSuspend_timer;

    if(*pGuard_timer == NULL)
    {
        *pGuard_timer = timer_create(GUARD_MULTI_TIME,
                                0,
                                TMR_TRIGGER,//TMR_TRIGGER
                                multi_trans_guard_timer_cb,
                                pHeader,
                                "multi_trans_guard_timer_cb"
                               );
    }
    else
    {
        timer_modify(*pGuard_timer,
                                GUARD_MULTI_TIME,
                                0,
                                TMR_TRIGGER,//TMR_TRIGGER
                                multi_trans_guard_timer_cb,
                                pHeader,
                                "multi_trans_guard_timer_cb",
                                TRUE);
    }

    pHeader->Guard_timer = *pGuard_timer;

	manage_link_add(pHeader);
    return TRUE;
}

static int8_t multi_trans_manage_timer_link_add(multi_ctrl_timer_t *pheader)
{
	/*over threshold , drop the msg*/
	if(manage_timer_link_header.nr >= manage_timer_link_header.thr)
	{
		return -1;
	}	 
	 list_add_tail(&pheader->link, &manage_timer_link_header.link);
	 ++manage_timer_link_header.nr;
	 return 0;
}

static void multi_trans_manage_timer_link_init(void)
{
    int8_t ii;
    
	INIT_LIST_HEAD(&manage_timer_link_header.link);
    app_printf("timer_header %p\n",manage_timer_link_header.link);
	manage_timer_link_header.nr = 0;
	manage_timer_link_header.thr = (HIGHEST_PRIORITY_LIST_NUM +
                                    F1F1_TRANS_LIST_NUM +
                                    F14F1_TRANS_LIST_NUM +
                                    REGISTER_QUER_LIST_NUM +
                                    UPGRADE_START_LIST_NUM +
                                    UPGRADE_QUERY_LIST_NUM +
                                    INDENTIFICATION_COLLECT_BF_LIST_NUM +
                                    PHASE_POSITION_BF_LIST_NUM +
                                    MODULE_ID_QUERY_LIST_NUM +
                                    CLOCK_MANAGE_LIST_NUM +
                                    MODULE_CURVE_CFG_LIST_NUM +
									20);	

    if(manage_timer_link_header.thr > MAX_CTRL_TIMER_NUM)
    {
        app_printf("sub timer num\n");
        manage_timer_link_header.thr = MAX_CTRL_TIMER_NUM;
    }

    for(ii = 0;ii < manage_timer_link_header.thr;ii ++)
    {
        multi_trans_one_timer_init(&multi_ctrl_timer_list[ii].ctrl_timer);
        multi_ctrl_timer_list[ii].seq = ii;
        //app_printf("%02d,%p,%p\n",ii,multi_ctrl_timer_list[ii].ctrl_timer,&multi_ctrl_timer_list[ii]);
        if(multi_trans_manage_timer_link_add(&multi_ctrl_timer_list[ii]) != 0)
        {
            app_printf("mg_tl_a err\n");
        }
    }
    app_printf("mg_tl_h %p\n",manage_timer_link_header.link);
}

static void* multi_trans_get_timer_from_list(void)
{
    if(manage_timer_link_header.nr > 0)
    {
        list_head_t *pheader;

        pheader = (void *)manage_timer_link_header.link.next;

        //printf_s("get_timer = %p\n",pheader);
        //printf_s("1 = %p\n",manage_timer_link_header.link);
        list_del(pheader);
        //printf_s("2 = %p\n",manage_timer_link_header.link);
        --manage_timer_link_header.nr;
        return pheader;
    }
    else
    {
        return NULL;
    }
}

static int8_t multi_trans_put_timer_into_list(multi_ctrl_timer_t *pheader)
{
    if(zc_timer_query(pheader->ctrl_timer) == TMR_RUNNING)
    {
        timer_stop(pheader->ctrl_timer,TMR_NULL);
    }

    if(manage_timer_link_header.nr < manage_timer_link_header.thr)
    {
        //printf_s("put_timer = %p,seq = %d\n",pheader,pheader->seq);
        //list_del(&pheader->link);	
        //++manage_timer_link_header.nr;
        multi_trans_manage_timer_link_add(pheader);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

static void multi_trans_one_timer_init(ostimer_t **ptimer )
{
    if(*ptimer == NULL)
	{
        *ptimer = timer_create(100,
                                100,
	                            TMR_TRIGGER,//TMR_TRIGGER
	                            multi_trans_timer_cb,
	                            NULL,
	                            "multi_trans_timer_cb"
	                           );
	}
    else
    {
        timer_modify(*ptimer,
                                100,
                                100,
	                            TMR_TRIGGER,//TMR_TRIGGER
	                            multi_trans_timer_cb,
	                            NULL,
                                "multi_trans_timer_cb",
                                TRUE);
    }
}


static void multi_trans_modify_one_timer(uint32_t first,ostimer_t **ptimer,void *arg)
{
	//对first值做保护
	if(first == 0)
    {
        app_printf("multi_trans_modify_one_timer = 0\n");
        return;
    }   

	if(NULL != *ptimer)
	{
	    if(first == 1)
        {
        }
        else
        {
            first = first*100;
        }
	    //app_printf("one_timer = %d\n",first);
		timer_modify(*ptimer,
				first,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				multi_trans_timer_cb,
				arg,
                "multi_trans_timer_cb",
                TRUE);
	}
	else
	{
		sys_panic("*ptimer is null\n");
	}
	timer_start(*ptimer);
}

/*************************************************************************
 * 函数名称	: 	multi_trans_timer_init
 * 函数说明	: 	任务链表、定时器初始化
 * 参数说明	: 	无
 * 返回值	: 	TRUE
 *************************************************************************/
int8_t multi_trans_timer_init(void)
{
    F1F1_TRANS_LIST_NUM = (SET_PARAMETER(app_ext_info.param_cfg.ConcurrencySize,10,MAX_WIN_SIZE,10));
    multi_trans_task_self_init(&F1F1_TRANS_LIST, 0, F1F1_TRANS_LIST_NUM , 2, NULL, NULL, NULL,&f1f1_trans_guard_timer);
    app_printf("ConcurrencySize %d\n",F1F1_TRANS_LIST_NUM);
    multi_trans_task_self_init(&HIGHEST_PRIORITY_LIST, 0, HIGHEST_PRIORITY_LIST_NUM, 1, NULL, NULL, NULL, &highest_priority_guard_timer);
    
    multi_trans_task_self_init(&F14F1_TRANS_LIST, 0, F14F1_TRANS_LIST_NUM, 3, NULL, NULL, NULL, &f14f1_trans_guard_timer);
    
    multi_trans_task_self_init(&REGISTER_QUER_LIST, 0, REGISTER_QUER_LIST_NUM, 3, NULL, NULL, register_suspend_timer, &register_quer_guard_timer);
    
    multi_trans_task_self_init(&UPGRADE_START_LIST, 0, UPGRADE_START_LIST_NUM, 3, NULL, NULL, UpgradeSuspendtimer, &upgrade_start_guard_timer);
    
    multi_trans_task_self_init(&UPGRADE_QUERY_LIST, 0, UPGRADE_QUERY_LIST_NUM, 3, NULL, NULL, UpgradeSuspendtimer, &upgrade_query_guard_timer);
    
    multi_trans_task_self_init(&INDENTIFICATION_COLLECT_BF_LIST, 0, INDENTIFICATION_COLLECT_BF_LIST_NUM, 3, 
                                cco_area_ind_add_task_list, cco_area_ind_ctrl_task_round, NULL, &indentification_multi_guard_timer);
    
    multi_trans_task_self_init(&PHASE_POSITION_BF_LIST, 0, PHASE_POSITION_BF_LIST_NUM, 3, NULL, NULL, NULL, &phaseposition_multi_guard_timer);
    
    multi_trans_task_self_init(&MODULE_ID_QUERY_LIST, 0, MODULE_ID_QUERY_LIST_NUM, 3, NULL, NULL, NULL, &moduleid_query_guard_timer);
    
    multi_trans_task_self_init(&QUERYSWORVALUE_LIST, 0,CLOCK_MANAGE_LIST_NUM, 3, NULL, NULL, NULL,&query_sworvalue_guard_timer);
    
    multi_trans_task_self_init(&SETSWORVALUE_LIST,0,CLOCK_MANAGE_LIST_NUM, 3, NULL, NULL, NULL,&set_sworvalue_guard_timer);

	multi_trans_task_self_init(&CURVE_PROFILE_CFG_LIST, 0, MODULE_CURVE_CFG_LIST_NUM, 3, NULL, NULL, NULL, &curve_profile_cfg_guard_timer);
/*
    multi_trans_task_init(&HIGHEST_PRIORITY_LIST, &highest_priority_timer, 0, HIGHEST_PRIORITY_LIST_NUM, 1, NULL, NULL, NULL);
    //F1F1_TRANS_LIST_NUM = (SET_PARAMETER(app_ext_info.param_cfg.ConcurrencySize,10,MAX_WIN_SIZE,10));
    //multi_trans_task_init(&F1F1_TRANS_LIST, &f1f1_trans_timer, 0, F1F1_TRANS_LIST_NUM , 2, NULL, NULL, NULL);
    //app_printf("ConcurrencySize %d\n",(SET_PARAMETER(app_ext_info.param_cfg.ConcurrencySize,10,MAX_WIN_SIZE,10)));

    multi_trans_task_init(&F14F1_TRANS_LIST, &f14f1_trans_timer, 0, F14F1_TRANS_LIST_NUM, 3, NULL, NULL, NULL);
    
    multi_trans_task_init(&REGISTER_QUER_LIST, &register_quer_timer, 0, REGISTER_QUER_LIST_NUM, 3, NULL, NULL, RegisterSuspendtimer);
    
    multi_trans_task_init(&UPGRADE_START_LIST, &upgrade_start_timer, 0, UPGRADE_START_LIST_NUM, 3, NULL, NULL, UpgradeSuspendtimer);
    
    multi_trans_task_init(&UPGRADE_QUERY_LIST, &upgrade_query_timer, 0, UPGRADE_QUERY_LIST_NUM, 3, NULL, NULL, UpgradeSuspendtimer);
    
    multi_trans_task_init(&INDENTIFICATION_COLLECT_BF_LIST, &indentification_multi_timer, 0, INDENTIFICATION_COLLECT_BF_LIST_NUM, 3, 
                                add_indentification_task_list, ctrl_indentification_task_round, NULL);
    
    multi_trans_task_init(&PHASE_POSITION_BF_LIST, &phaseposition_multi_timer, 0, PHASE_POSITION_BF_LIST_NUM, 3, NULL, NULL, NULL);
    
    multi_trans_task_init(&MODULE_ID_QUERY_LIST, &moduleid_query_timer, 0, MODULE_ID_QUERY_LIST_NUM, 3, NULL, NULL, NULL);*/

    multi_trans_manage_timer_link_init();
    
	return TRUE;
}


U8 multi_trans_register_list_full(void)
{
	return  REGISTER_QUER_LIST.nr < REGISTER_QUER_LIST.thr ?TRUE:FALSE;
}

//static void modify_multi_trans_timer(multi_trans_header_t	 *pMultiTransHeader,ostimer_t *multi_trans_timer,uint32_t first)
//{
//	if(NULL != multi_trans_timer)
//	{
//		timer_modify(multi_trans_timer,
//				first*10,
//				100,
//				TMR_PERIODIC ,//TMR_TRIGGER
//				multi_trans_timer_cb,
//				pMultiTransHeader,
//                "multi_trans_timer_cb",
//                TRUE);
//	}
//	else
//	{
//		sys_panic("multi_trans_timer is null\n");
//	}
//	timer_start(multi_trans_timer);
//}

#endif

