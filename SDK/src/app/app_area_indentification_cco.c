#include "app_common.h"
#include "app_base_multitrans.h"
#include "app_phase_position_cco.h"
#include "datalinkdebug.h"
#include "app_base_serial_cache_queue.h"
#include "aps_layer.h"
#include "app_area_indentification_cco.h"
#include "app_area_indentification_sta.h"
#include "app.h"
#include "netnnib.h"
#include "app_dltpro.h"
#include "app_gw3762.h"
#include "app_event_report_cco.h"


#if defined(STATIC_MASTER)

static void cco_area_ind_task_start(work_t *work);
static void cco_area_ind_task_collect(work_t *work);
static void cco_area_ind_task_inform(work_t *work);
static void cco_area_ind_ctrl_timer_modify(uint32_t TimeMs);
static void cco_area_ind_task_timer_modify();			//台区识别任务定时器超时回调

U8 gl_period_edge = DEF_IND_EDGE;

CONCURRENCY_INFO_t gl_cco_area_ind_info_t;

static ostimer_t *cco_area_ind_ctrl_timer = NULL;
static ostimer_t *cco_area_ind_task_timer = NULL;		//台区识别任务超时定时器
static ostimer_t *cco_area_ind_result_timer = NULL;		//台区识别结果查询超时定时器

static INDENTIFICATION_CTRL_t cco_area_ind_flush_ctrl_t;

U8 cco_area_ind_round = 0;
VALUE_INFO_t cco_area_ind_value_info_t[MAX_WHITELIST_NUM];


static FEATURE_COLLECT_INFO_t cco_area_ind_voltage_collect_info_t={
 		.CrntNodeIndex = 0,
		.gettophandle = e_Voltage_TopoQueryFlag,
};
		
static FEATURE_COLLECT_INFO_t cco_area_ind_frequncy_collect_info_t={
 		.CrntNodeIndex = 0,
		.gettophandle = e_Frequncy_TopoQueryFlag,
};
		
static FEATURE_COLLECT_INFO_t cco_area_ind_period_collect_info_t={
 		.CrntNodeIndex = 0,
		.gettophandle = e_Period_TopoQueryFlag,
};


static void cco_area_ind_collect_info_set(U8 mode,U8 plan)
{	 
	cco_area_ind_flush_ctrl_t.mode = mode;    //e_DISTRIBUT
	cco_area_ind_flush_ctrl_t.plan_set = plan;//voltage_bit|frequncy_bit|period_bit;
	
	cco_area_ind_flush_ctrl_t.state = cco_area_ind_flush_ctrl_t.plan_set & voltage_bit ? e_START_COLLECT_VOLTAGE:
						cco_area_ind_flush_ctrl_t.plan_set & frequncy_bit ? e_START_COLLECT_FREQUNCY:
						cco_area_ind_flush_ctrl_t.plan_set & period_bit ? e_START_COLLECT_PERIOD:0;
						
	cco_area_ind_flush_ctrl_t.plan_programing = 0;
	cco_area_ind_flush_ctrl_t.plan_collecting = 0;
	cco_area_ind_flush_ctrl_t.plan_complete = 0;
}


static U8 cco_area_ind_mode_get(void)
{
	return cco_area_ind_flush_ctrl_t.mode;
}


/*
U8 indentification_state_get(void)
{
	return cco_area_ind_flush_ctrl_t.state;
}

FEATURE_COLLECT_START_DATA_t feature_collect_start_pma(U8 feature)
{
	FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t;
	static U8 seq = 0;
	switch  (feature)
	{
		case e_VOLTAGE:
				FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
				FeatureCollectStart_Data_t.CollectCycle = COLLECT_VOLTAGE_CYCLE;
				FeatureCollectStart_Data_t.CollectNum = 10;
			break;
		case e_FREQUNCY:
				FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
				FeatureCollectStart_Data_t.CollectCycle = COLLECT_FREQUNCY_CYCLE;
				FeatureCollectStart_Data_t.CollectNum = 10;
			break;
		case e_PERIOD:
				FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
				FeatureCollectStart_Data_t.CollectCycle = 0; //no meaning
				FeatureCollectStart_Data_t.CollectNum = 30;
			break;
		default:
			break;	
	}
	
	FeatureCollectStart_Data_t.CollectSeq = ++seq;
	FeatureCollectStart_Data_t.res = 0;

	area_ind_set_featrue_and_start_timer(feature,&FeatureCollectStart_Data_t);
	app_printf("feature : %d start ntb : 0x%08x  seq : 0x%x\n FEATURE_COLLECT_START_DATA_t-> ",feature,FeatureCollectStart_Data_t.Startntb,FeatureCollectStart_Data_t.CollectSeq);
	dump_buf((U8 *)&FeatureCollectStart_Data_t,sizeof(FEATURE_COLLECT_START_DATA_t));
	return FeatureCollectStart_Data_t;
}
*/


void cco_area_ind_flush_ctrl_timer_stop(void)
{
	if(indentification_multi_timer)
	{
		timer_stop(indentification_multi_timer,TMR_NULL);
	}
	if(voltage_info_collect_timer)
	{
		timer_stop(voltage_info_collect_timer,TMR_NULL);
	}
	if(freq_info_collect_timer)
	{
		timer_stop(freq_info_collect_timer,TMR_NULL);
	}	
	if(period_info_collect_timer)
	{
		timer_stop(period_info_collect_timer,TMR_NULL);
	}
	memset((U8 *)&cco_area_ind_flush_ctrl_t,0,sizeof(INDENTIFICATION_CTRL_t));
}


//-------------------------------------------------------------------------------------------------
// 函数名：void cco_area_ind_task_cfg(uint8_t mode, uint8_t plan)
// 
// 功能描述：
//       本函数用于台区识别任务的配置和启动，本函数在376p2的05F6命令中被调用，用于启动台区识别
// 参数：
//
//     uint8_t mode   台区识别模式，分布式或集中式，缺省为分布式 e_DISTRIBUT    为1，e_CENTRALIZATION为0
//     uint8_t plan   台区识别方案，分别为voltage_bit,frequncy_bit,period_bit，可以与
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------

void cco_area_ind_task_cfg(uint8_t mode, uint8_t plan)
{
    Final_result_t.PublicResult_t[0].CalculateMaxNum = MAX_CACULATE;	//设置CCO周期采集次数
	Final_result_t.PublicResult_t[0].CalculateNum = 0;					//清除CCO当前采集次数
	
	cco_area_ind_collect_info_set(mode, plan);  						//设置采集模式，默认分布式，工频周期
	
	app_printf("-----cco_area_ind_task_cfg-----\n");
    
	work_t *post_work = zc_malloc_mem(sizeof(work_t), "cco_area_ind_task_cfg", MEM_TIME_OUT);
	if(NULL == post_work)
	{
		return;
	}
	post_work->work = cco_area_ind_task_start;
    post_work->msgtype = SB_TASK_START;
	post_app_work(post_work);
	cco_area_ind_task_timer_modify();				//启动台区识别后启动任务定时器
	AppGw3762Afn10F4State.WorkSwitchZone = TRUE;	//同时将路由状态台区识别标志置TRUE,这部分是给app的命令做支持
}


void cco_area_ind_start(void)
{
	cco_area_ind_task_cfg(e_DISTRIBUT,period_bit);
	DeviceList_ioctl(DEV_RESET_AREA,NULL, NULL);	//清除所有模块台区信息
	cco_area_ind_task_timer_modify();
}


void cco_area_ind_stop(void)
{
	cco_area_ind_flush_ctrl_timer_stop();
	//台区识别停止时，需要停止查询结果超时定时器和台区识别任务超时定时器
	if(TMR_RUNNING == zc_timer_query(cco_area_ind_task_timer))
		timer_stop(cco_area_ind_task_timer,TMR_NULL);		//停止台区识别任务超时定时器
	if(TMR_RUNNING == zc_timer_query(cco_area_ind_result_timer))
		timer_stop(cco_area_ind_result_timer,TMR_NULL);;	//停止台区识别结果查询超时定时器
}

//
/*-----------------------------------上报台区识别结果----------------------------------*/

//ReprotCCO存储上报的CCO集合，启动台识别需要清空，CleanReprotCCO()
//CCO在集中式识别完成每个STA的识别，或者CCO分布式获得STA上报结果时调用AreaindentificationReport函数即可



//true 为本台区，false 为其他台区
//Mac地址对应为相应台区的CCO,未识别到归属CCO时，mac地址为NULL

void cco_area_ind_report_result(U16 TEI,U8 Result,U8 *BelongCCOMac)
{
	U8  AREA_ERR;
    U8  len=0;
    U8  dlt645frame[20];
	//U8  BCAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    U8  buff[7] = {0x03, 0, 0, 0, 0, 0, 0};
	U8  Meteraddr[6];
	U16 seq;
	DEVICE_TEI_LIST_t DeviceListTemp;

	seq = TEI-2;
	if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&seq, &DeviceListTemp))//Get_DeviceList_All(TEI-2, &DeviceListTemp);
    {   
        if(Result ==e_INDENTIFICATION_RESULT_BELONG )
        {
            AREA_ERR = TRUE;
            DeviceList_ioctl(DEV_SET_AREA,&TEI,&AREA_ERR);//Set_DeviceList_AREA(TEI , DeviceListTemp.AREA_ERR); //写入结果
            app_printf("cco_area_ind_report_result:TEI=%04x,it's good boy!\n",TEI);
            return;
        }
        else if(Result == e_INDENTIFICATION_RESULT_UNKOWN)
        {
            app_printf("Areaindentification is going on!\n",TEI);
            return;
        }
        else if(Result == e_INDENTIFICATION_RESULT_NOTBELONG)
        {
            if(DeviceListTemp.AREA_ERR != FALSE)
            {
                AREA_ERR = FALSE;
                DeviceList_ioctl(DEV_SET_AREA,&TEI,&AREA_ERR);//Set_DeviceList_AREA(TEI , DeviceListTemp.AREA_ERR); //写入结果
            }
            else
            {
                //去重
                return;
            }
        }
    }
	//识别结果不在本台区的节点，需要上报给集中器
	__memcpy(&buff[1],BelongCCOMac,6);
	Add0x33ForData(buff, sizeof(buff));
	__memcpy(Meteraddr,DeviceListTemp.MacAddr,6);
	ChangeMacAddrOrder(Meteraddr);
	Packet645Frame(dlt645frame, &len, Meteraddr, 0x9E,buff, sizeof(buff));
	
    app_gw3762_up_afn06_f5_report_slave_event(FALSE,
    									APP_GW3762_M_SLAVETYPE ,
										get_protocol_by_addr(Meteraddr),
                                        Meteraddr,
										len,
										dlt645frame);

}

















//-------------------------------------------------------------------------------------------------
// 函数名：uint8_t cco_area_ind_get_feature_by_state(uint8_t state)
// 
// 功能描述：
//       本函数用于从台区识别的起始状态，获取到识别方案枚举值，如e_PERIOD
// 参数：
//     uint8_t state  台区识别状态值
// 返回值：
//     uint8_t 类型，为台区识别方案枚举值，正常为1、2、3，0为异常
//-------------------------------------------------------------------------------------------------

uint8_t cco_area_ind_get_feature_by_state(uint8_t state)
{
	return state == e_START_COLLECT_VOLTAGE ? e_VOLTAGE :
			 state == e_START_COLLECT_FREQUNCY ? e_FREQUNCY :
			 	state == e_START_COLLECT_PERIOD ? e_PERIOD : 0;
}

//获取并发列表
//参数：并发任务类型
static void cco_area_ind_get_bf_info_from_net(CONCURRENCY_INFO_t *pConInfo,CON_TASK_TYPE_e tasktype,U16 maxCycle)
 {

	U16 ii;
	U8 result = 3;
	app_printf("seq\ttei\tresult\n");
	for (ii = 0; ii < MAX_WHITELIST_NUM; ii++)
	{
		//判断模块是否在网，不在网继续遍历下一个
		if (!DeviceList_ioctl(DEV_GET_VAILD, &ii, NULL))
		{
			memset(&pConInfo->taskPolling[ii], 0x00, sizeof(CCO_TASK_RECORD));
			continue;
		}
		//查询当前模块的台区识别结果
		if(!DeviceList_ioctl(DEV_GET_AREA, &ii, &result))
		{
			continue;
		}
		//台区识别结果不是未知，则继续下一个
		if(result != 3)
		{
			continue;
		}
		pConInfo->taskPolling[ii].Tasktype = pConInfo->CycleInfo.Tasktype = tasktype;
		pConInfo->taskPolling[ii].Valid = TRUE;
		pConInfo->taskPolling[ii].result = 0;
		pConInfo->CycleInfo.CycleNum = maxCycle;
		pConInfo->CycleInfo.CurrentIndex = 0;
	}
	pConInfo->CycleInfo.CurrentIndex = 0;
	// memset(pConInfo->BF_Window_List,0,sizeof(pConInfo->BF_Window_List));
}

 //检查并发列表中的任务是否全部完成
static U8 cco_area_ind_check_task_poll_finish(CONCURRENCY_INFO_t *pConInfo)
 {
    U16 ii ;
    
 	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
 	{
 		if(pConInfo->taskPolling[ii].Valid == FALSE || pConInfo->taskPolling[ii].result != 0)
 		{
			continue;
		}
		if(FALSE == DeviceList_ioctl(DEV_GET_VAILD,&ii,NULL))
		{
			pConInfo->taskPolling[ii].Valid = FALSE;
			continue;
		}
 	}
	return TRUE;
 }
/*
 U8 AddConResult(CONCURRENCY_INFO_t *pConInfo,multi_trans_header_t *listheader, U8  *Addr, U16 seq)
 {
	 if(listheader->nr<=0)
	{
		return FALSE;
	}
	multi_trans_t  *pTask;
    list_head_t *pos,*node;

    list_for_each_safe(pos, node,&listheader->link)
    {
        pTask = list_entry(pos , multi_trans_t , link);
        if(pTask->DatagramSeq == seq && 0 == memcmp(pTask->Addr, Addr, 6))
        {
        	pConInfo->taskPolling[pTask->StaIndex].result = TRUE;
			return TRUE;
        }
    }
	return FALSE;
 }
*/
 static uint8_t cco_area_ind_check_bf_list_repeat(multi_trans_header_t *listheader, uint16_t index, uint8_t sendtype)
 {
	 if(listheader->nr<=0)
	 {
		 return FALSE;
	 }
	 multi_trans_t	*pTask;
	 list_head_t *pos,*node;
 
	 list_for_each_safe(pos, node,&listheader->link)
	 {
		 pTask = list_entry(pos , multi_trans_t , link);
		 if(pTask->StaIndex == index && pTask->SendType == sendtype)
		 {
			 return TRUE;
		 }
	 }
	 return FALSE;
 }


 //从任务池中查找一个添加到发送窗口中
static uint8_t cco_area_ind_get_ind_info(CONCURRENCY_INFO_t *pConInfo, multi_trans_header_t *listheader, uint8_t sendtype)
{
	uint16_t ii ;
	DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    
	for(ii= pConInfo->CycleInfo.CurrentIndex ;ii<MAX_WHITELIST_NUM;ii++)
	{
	    if(pConInfo->taskPolling[ii].Valid != 0 )
	    {
			if(cco_area_ind_check_bf_list_repeat(listheader, ii, sendtype))
	        {
	            //app_printf("list repeat\n");
	            continue;
	        }
			if( pConInfo->taskPolling[ii].result == 0)
			{
				pConInfo->CycleInfo.CurrentIndex = ii;
				DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&ii, &GetDeviceListTemp_t);//Get_DeviceList_All(ii,&GetDeviceListTemp_t);
				__memcpy(pConInfo->CycleInfo.CrntNodeAdd,GetDeviceListTemp_t.MacAddr, 6);
				return TRUE;
			}
		}
	}
	if(ii == MAX_WHITELIST_NUM )
	{
		pConInfo->CycleInfo.CurrentIndex= MAX_WHITELIST_NUM;
	}
	 return FALSE;
}

//台区识别完成进行主动上报
void cco_area_ind_finish(work_t *work)
{
	if(FALSE == AppGw3762Afn10F4State.WorkSwitchZone)
	{
		return;
	}
	AppGw3762Afn10F4State.WorkSwitchZone = FALSE;
	app_gw3762_up_afn06_f3_report_router_change(APP_GW3762_AREA_TASKCHANGE, e_UART1_MSG);
	cco_area_ind_stop();									//停止台区识别控制定时器

}



static void cco_area_ind_task_timercb(struct ostimer_s *ostimer, void *arg)
{
	work_t *post_work = zc_malloc_mem(sizeof(work_t), "cco_area_ind_ctrl_timercb", MEM_TIME_OUT);
	if(NULL == post_work)
	{
		return;
	}
	app_printf("task tout\n");
	post_work->work = cco_area_ind_finish;
    post_work->msgtype = AREA_TASK_TIMEOUT;
	post_app_work(post_work);
}

static void cco_area_ind_result_timercb(struct ostimer_s *ostimer, void *arg)
{
	work_t *post_work = zc_malloc_mem(sizeof(work_t), "cco_area_ind_result_timercb", MEM_TIME_OUT);
	if(NULL == post_work)
	{
		return;
	}
	app_printf("result tout\n");
	post_work->work = cco_area_ind_finish;
    post_work->msgtype = AREA_RESULT_TIMEOUT;
	post_app_work(post_work);
}

/*************************************************************************
 * 函数名称 :	void cco_area_ind_task_timer_modify(uint32_t first)
 * 函数说明 :	
 *      本函数用于修改台区识别流程控制定时器，首次未创建时则创建，定时器为触发模式
 * 参数说明 :	
 *     uint32_t first  定时器触发模式定时时间值，10ms的单位
 * 返回值   :   
 *     无
 *************************************************************************/
static void cco_area_ind_task_timer_modify()
{
	if(NULL != cco_area_ind_task_timer)
	{
		timer_modify(cco_area_ind_task_timer,
                AREA_IND_TASK_TIME_OUT,
                0,
				TMR_TRIGGER,
				cco_area_ind_task_timercb,
				NULL,
				"cco_area_ind_task_timer"
                ,TRUE);
	}
	else
	{
        cco_area_ind_task_timer = timer_create(AREA_IND_TASK_TIME_OUT,
                          0,
						  TMR_TRIGGER,
						  cco_area_ind_task_timercb,
						  NULL,
						  "cco_area_ind_task_timer");
	}
	timer_start(cco_area_ind_task_timer);
}



/*************************************************************************
 * 函数名称 :	void cco_area_ind_result_timer_modify()
 * 函数说明 :	
 *      本函数用于修改台区识别流程控制定时器，首次未创建时则创建，定时器为触发模式
 * 参数说明 :	
 *     uint32_t first  定时器触发模式定时时间值，10ms的单位
 * 返回值   :   
 *     无
 *************************************************************************/
static void cco_area_ind_result_timer_modify()
{
	if(NULL != cco_area_ind_result_timer)
	{
		timer_modify(cco_area_ind_result_timer,
                AREA_IND_RESULT_TIME_OUT,
                0,
				TMR_TRIGGER,
				cco_area_ind_result_timercb,
				NULL,
				"cco_area_ind_result_timer"
                ,TRUE);
	}
	else
	{
        cco_area_ind_result_timer = timer_create(AREA_IND_RESULT_TIME_OUT,
                          0,
						  TMR_TRIGGER,
						  cco_area_ind_result_timercb,
						  NULL,
						  "cco_area_ind_result_timer");
	}
	timer_start(cco_area_ind_result_timer);
}


//-------------------------------------------------------------------------------------------------
// 函数名：static void cco_area_ind_task_start(work_t *work)
// 
// 功能描述：
//       本函数驱动发送台区特征采集启动
// 参数：
//     work_t *work 应用层消息传递参数结构指针；
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------
static void cco_area_ind_task_start(work_t *work)
{
    uint8_t  feature;
    uint8_t  addr_broadcast[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	
    feature = cco_area_ind_get_feature_by_state(cco_area_ind_flush_ctrl_t.state);		//获取特征类型，电压，频率，工频周期
    
    if(!feature)
        return;
	
    FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t;
	
	static U8 seq = 0;

	//配置采集信息，startntb，采集数量，采集序号，采集周期(对采集数量的间隔时间)
	switch(feature)
	{
		case e_VOLTAGE:
				FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
				FeatureCollectStart_Data_t.CollectCycle = COLLECT_VOLTAGE_CYCLE;
				FeatureCollectStart_Data_t.CollectNum = 10;
			break;
		case e_FREQUNCY:
				FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
				FeatureCollectStart_Data_t.CollectCycle = COLLECT_FREQUNCY_CYCLE;
				FeatureCollectStart_Data_t.CollectNum = 10;
			break;
		case e_PERIOD:
				FeatureCollectStart_Data_t.Startntb = get_phy_tick_time()+START_COLLECT_TIME*PHY_TICK_FREQ;
				FeatureCollectStart_Data_t.CollectCycle = 0;
				FeatureCollectStart_Data_t.CollectNum = 36;
			break;
		default:
			break;	
	}
	
	FeatureCollectStart_Data_t.CollectSeq = ++seq;
	FeatureCollectStart_Data_t.res = 0;

    // 广播发送台区特征采集启动命令
	FeatureCollectStart(FeatureCollectStart_Data_t,addr_broadcast,e_DEFAULT_PHASE,feature);

    //发送完台区特征采集启动命令后，CCO转而去自身同时进行特征信息采集，启动命令数据内容作为参数传递到下一状态
	app_printf("-----cco_area_ind_task_start-----\n");
	work_t *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(FEATURE_COLLECT_START_DATA_t), "cco_area_ind_task_start", MEM_TIME_OUT);
	if(NULL == post_work)
	{
		return;
	}
	__memcpy(post_work->buf, &FeatureCollectStart_Data_t, sizeof(FEATURE_COLLECT_START_DATA_t));
	post_work->work = cco_area_ind_task_collect;
    post_work->msgtype = SB_TASK_COLLECT;
	post_app_work(post_work);
}


/*************************************************************************
 * 函数名称 :	static void cco_area_ind_task_collect(work_t *work)
 * 函数说明 :	
 *     本函数依据当前状态，启动自身进行特征信息采集
 * 参数说明 :	
 *     work_t *work  应用层消息传递参数结构指针；
 * 返回值   :   
 *     void  无返回值
 *************************************************************************/
static void cco_area_ind_task_collect(work_t *work)
{
    uint8_t  feature;
	FEATURE_COLLECT_START_DATA_t *FeatureCollectStart_Data_t = (FEATURE_COLLECT_START_DATA_t *)work->buf;

	feature = cco_area_ind_get_feature_by_state(cco_area_ind_flush_ctrl_t.state);		//获取特征信息
    
	if(!feature)
		return;

	//CCO设置特征采集开始，等待定时器到
	
	area_ind_set_featrue_and_start_timer(feature,FeatureCollectStart_Data_t);
	app_printf("-----cco_area_ind_task_collect-----\n");

}

static void cco_area_ind_task_get(work_t *work)
{
	;
}


/*************************************************************************
 * 函数名称 :	void cco_area_ind_featrue_collecting(uint8_t plan_collecting, uint8_t state)
 * 函数说明 :	
 *     本函数在CCO采集完成自身特征信息采集后调用，准备发送特征信息告知
 * 参数说明 :	
 *     uint8_t plan_collecting   采集方案，如 period_bit
 *     uint8_t state             CCO特征信息采集状态是否正常，TRUE为正常
 * 返回值   :   
 *     void  无返回值
 *************************************************************************/
 
void cco_area_ind_featrue_collecting(uint8_t plan_collecting, uint8_t arg_check_flag)
{
	//采集完成，准备告知
	cco_area_ind_flush_ctrl_t.plan_collecting |= plan_collecting;
	
	FEATURE_COLLECT_INFO_t *feature_collect_info_t =
		cco_area_ind_flush_ctrl_t.plan_collecting & voltage_bit?(&cco_area_ind_voltage_collect_info_t):
		cco_area_ind_flush_ctrl_t.plan_collecting & frequncy_bit?(&cco_area_ind_frequncy_collect_info_t):
		cco_area_ind_flush_ctrl_t.plan_collecting & period_bit?(&cco_area_ind_period_collect_info_t):NULL;

	if(feature_collect_info_t)
	{
		feature_collect_info_t->CrntNodeIndex = 2;
	}
	else
	{
		;
	}
	app_printf("-----cco_area_ind_featrue_collecting-----\n");
    
	work_t *post_work = zc_malloc_mem(sizeof(work_t), "cco_area_ind_task_collect", MEM_TIME_OUT);	
	if(NULL == post_work)
	{
		return;
	}
	if(cco_area_ind_mode_get() == e_CENTRALIZATION)
	{
		post_work->work = cco_area_ind_task_get; // 集中器时开始进行特征信息收集
        post_work->msgtype = SB_TASK_GET;
	}
	else if(cco_area_ind_mode_get() == e_DISTRIBUT)
	{
		if(arg_check_flag == FALSE)
		{
		    //分布式并且特征采集未完成或不正常时，重新进行特征信息启动
			app_printf("-----next cco_area_ind_task_start-----\n");
			post_work->work = cco_area_ind_task_start;
            post_work->msgtype = SB_TASK_START;
		}
		else
		{
		    //分布式并且CCO特征采集完成时，启动进行特征信息告知
			app_printf("-----next cco_area_ind_task_inform-----\n");
			post_work->work = cco_area_ind_task_inform;
            post_work->msgtype = SB_TASK_INFORM;
		}
	}
	post_app_work(post_work);
}



void area_resend_edge_timercb(struct ostimer_s *ostimer, void *arg)
{
    cco_area_ind_featrue_collecting(period_bit, cco_area_ind_check_zero());	
}

ostimer_t *area_resend_edge_timer = NULL;	//工频周期控制定时器


void cco_area_ind_edge_resend_timer_modify(U32 first)
{
	if(NULL != area_resend_edge_timer)
	{
		timer_modify(area_resend_edge_timer,
				first,
				0,
				TMR_TRIGGER ,
				area_resend_edge_timercb,
				NULL,
				"area_resend_edge_timercb",
				TRUE);
	}
	else
	{
		area_resend_edge_timer = timer_create(first,
				0,
				TMR_TRIGGER ,
				area_resend_edge_timercb,
				NULL,
				"area_resend_edge_timercb");
	}
	timer_start(area_resend_edge_timer);
}



/*************************************************************************
 * 函数名称 :	static void cco_area_ind_task_inform(work_t *work)
 * 函数说明 :	
 *     本函数被CCO用于发送特征信息告知命令
 * 参数说明 :	
 *     work_t *work   应用层消息传递参数结构指针；
 * 返回值   :    
 *     void  无返回值
 *************************************************************************/
static void cco_area_ind_task_inform(work_t *work)
{
	U8 addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    
	if(!(cco_area_ind_flush_ctrl_t.plan_complete & voltage_bit)&&(cco_area_ind_flush_ctrl_t.plan_collecting & voltage_bit))
	{
		packet_featureinfo(e_VOLTAGE,addr,e_COLLECT_NEGEDGE);
	}
	if(!(cco_area_ind_flush_ctrl_t.plan_complete & frequncy_bit)&&cco_area_ind_flush_ctrl_t.plan_collecting & frequncy_bit)
	{
		packet_featureinfo(e_FREQUNCY,addr,e_COLLECT_NEGEDGE);
	}
	if(!(cco_area_ind_flush_ctrl_t.plan_complete & period_bit)&&cco_area_ind_flush_ctrl_t.plan_collecting & period_bit)
	{
		app_printf("packet_featureinfo\n");
		//packet_featureinfo(e_PERIOD,addr,e_COLLECT_DBLEDGE);
		packet_featureinfo(e_PERIOD,addr,gl_period_edge);
	}
    
	//发送次数达到以后，开始采集结果
	//采集定时器超时，开始发送特征告知命令；
	app_printf("CalculateNum =%d,CalculateMaxNum=%d\n",Final_result_t.PublicResult_t[0].CalculateNum,Final_result_t.PublicResult_t[0].CalculateMaxNum);
	if(Final_result_t.PublicResult_t[0].CalculateNum <Final_result_t.PublicResult_t[0].CalculateMaxNum)
	{
		// 最大轮次未满时，重复启动、采集、告知流程
		if((gl_period_edge & e_COLLECT_POSEDGE) == e_COLLECT_POSEDGE)
		{
			cco_area_ind_ctrl_timer_modify(49*TIMER_UNITS);
			Final_result_t.PublicResult_t[0].CalculateNum++;
			cco_area_ind_flush_ctrl_t.state = e_START_COLLECT_PERIOD;
			gl_period_edge = DEF_IND_EDGE;
		}
		else
		{
			cco_area_ind_edge_resend_timer_modify(1*TIMER_UNITS);
			gl_period_edge = e_COLLECT_POSEDGE;
		}
	}
	else
	{
		cco_area_ind_flush_ctrl_t.state=e_RESULT_QUERY;
		memset((U8*)&gl_cco_area_ind_info_t,0,sizeof(gl_cco_area_ind_info_t) );
		cco_area_ind_get_bf_info_from_net(&gl_cco_area_ind_info_t,e_AREA_RESULT,2);
		cco_area_ind_add_task_list();
	}
}




static void cco_area_ind_ctrl_timercb(struct ostimer_s *ostimer, void *arg)
{
	work_t *post_work = zc_malloc_mem(sizeof(work_t), "cco_area_ind_ctrl_timercb", MEM_TIME_OUT);
	if(NULL == post_work)
	{
		return;
	}
	post_work->work = cco_area_ind_task_start;
    post_work->msgtype = SB_TASK_START;
	post_app_work(post_work);
}

/*************************************************************************
 * 函数名称 :	void cco_area_ind_ctrl_timer_modify(uint32_t first)
 * 函数说明 :	
 *      本函数用于修改台区识别流程控制定时器，首次未创建时则创建，定时器为触发模式
 * 参数说明 :	
 *     uint32_t first  定时器触发模式定时时间值，10ms的单位
 * 返回值   :   
 *     无
 *************************************************************************/
static void cco_area_ind_ctrl_timer_modify(uint32_t TimeMs)
{
	if(NULL != cco_area_ind_ctrl_timer)
	{
		timer_modify(cco_area_ind_ctrl_timer,
                TimeMs,
                1000,
				TMR_TRIGGER,
				cco_area_ind_ctrl_timercb,
				NULL,
				"cco_area_ind_ctrl_timercb"
                ,TRUE);
	}
	else
	{
        cco_area_ind_ctrl_timer = timer_create(TimeMs,
                          100,
						  TMR_TRIGGER,
						  cco_area_ind_ctrl_timercb,
						  NULL,
						  "cco_area_ind_ctrl_timercb");
	}
	timer_start(cco_area_ind_ctrl_timer);
}



/*******************************以下为并发相关*********************************/
static int32_t cco_area_ind_bf_entry_check(multi_trans_header_t *list, multi_trans_t *new_list)
{
    list_head_t *pos,*node;
    multi_trans_t  *mbuf_n;
    
	if(list->nr >= list->thr)
	{
		app_printf("-----cco_area_ind_bf_entry_check fail-----\n");
		return -1;
	}
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
	app_printf("-----cco_area_ind_bf_entry_check ok-----\n");
	return 0;
}

static void cco_area_ind_bf_entry_fail(int32_t err, void *pTaskPrmt)
{
	if(err==-1)//list full
	{
		app_printf("indtf list full.");
	}
}

static void cco_area_ind_bf_task_proc(void  *pTaskPrmt)
{
	multi_trans_t *pTask = (multi_trans_t *)pTaskPrmt;
	//发送收集请求；
	uint8_t feature ;
	feature = cco_area_ind_flush_ctrl_t.plan_collecting & voltage_bit ?e_VOLTAGE:
			  cco_area_ind_flush_ctrl_t.plan_collecting & frequncy_bit?e_FREQUNCY:
		      cco_area_ind_flush_ctrl_t.plan_collecting & period_bit?e_PERIOD:0;
    app_printf("CollectInfoGather feature=%d\n",feature);
	if(feature)	
	{
		app_printf("sendType %d pTask->CnmAddr ->", pTask->SendType);
		dump_buf(pTask->CnmAddr, 6);
		DistinguishResultQuery (e_DOWNLINK,//U8   Directbit,
						APS_MASTER_PRM, //U8   Startbit,
						FlashInfo_t.ucJZQMacAddr,
						pTask->CnmAddr,//U8	 *DesMacaddr, //接收方 地址
						pTask->DatagramSeq,
						e_FIRST_PHASE,//U8	 Phase,    
						feature);//U8	Featuretypes)
	}
}





//判断台区识别结果是否都要到
U8 cco_area_ind_check_result()
{
	U16 ii;
	U8  result = 0xff;
	for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
	{

		if(!DeviceList_ioctl(DEV_GET_VAILD,&ii,NULL))
		{
			continue;
		}
		result = 0xff;

		if(DeviceList_ioctl(DEV_GET_AREA,&ii, &result) && result == 3)
		{
			//如果返回的结果是3表示未结束
			return FALSE;
		}
	}
	//全部有结果返回TRUE
	return TRUE;
}


static void cco_area_ind_bf_task_up_proc(void *pTaskPrmt, void *pUplinkData)
{
	multi_trans_t  *pIndenBFTask = (multi_trans_t  *)pTaskPrmt;
	INDENTIFICATION_IND_t *pIndentificationInd = (INDENTIFICATION_IND_t *)pUplinkData;
	INDENTIFICATION_RESULT_DATA_t *indentification_result_data_t =
					(INDENTIFICATION_RESULT_DATA_t *)pIndentificationInd->payload;
	app_printf("-----indentification_bf_task_up_pro-----\n");
	if(pIndentificationInd->Collectiontype == e_DISTINGUISH_RESULT_INFO)
	{
		U8 addr[6] = {0};
		CONCURRENCY_INFO_t *pConInfo = &gl_cco_area_ind_info_t;
		cco_area_ind_result_timer_modify();	//有结果则重新启动结果请求超时定时器
		app_printf("pIndenBFTask->CnmAddr->");
		dump_buf(pIndenBFTask->CnmAddr, 6);
		app_printf("pIndentificationInd->SrcMacAddr->");
		dump_buf(pIndentificationInd->SrcMacAddr, 6);
		if(0 == memcmp(pIndenBFTask->CnmAddr, pIndentificationInd->SrcMacAddr, 6))
		{		
			pConInfo->taskPolling[pIndenBFTask->StaIndex].result = TRUE;
			if(indentification_result_data_t->finishflag == 0)//存在未完成的节点，需要再本轮查询结束后再次启动告知
			{
				app_printf("indentification_result_data_t->finishflag = 0\n");
				return;
			}
			__memcpy(addr,indentification_result_data_t->CCOMacaddr,6);
			ChangeMacAddrOrder(addr);
			
			//if(TMR_STOPPED == query_get_line_state_timer())
			{
				cco_area_ind_report_result(indentification_result_data_t->TEI,indentification_result_data_t->result,addr);
			}
			//判断是否还有未识别完成的
			if(FALSE == AppGw3762Afn10F4State.WorkSwitchZone)
			{
				app_printf("WorkSwitchZone is false\n");
				return;
			}
			if(TRUE == cco_area_ind_check_result())
			{
				cco_area_ind_finish(NULL);
			}
		}
	}
}

void cco_area_ind_add_task_list(void)
{
	CONCURRENCY_INFO_t *pConInfo = &gl_cco_area_ind_info_t;
	if(cco_area_ind_get_ind_info(pConInfo, &INDENTIFICATION_COLLECT_BF_LIST, INDENTIFICATION_QUERY))
	{	
		multi_trans_t indentification_result_query;
		
		indentification_result_query.lid           = INDENTIFICATION_LID;
		indentification_result_query.SrcTEI        = 0;
	    indentification_result_query.DeviceTimeout = DEVTIMEOUT;
	    indentification_result_query.Framenum      = 0;
	    __memcpy(indentification_result_query.Addr, pConInfo->CycleInfo.CrntNodeAdd, MAC_ADDR_LEN);	
	    __memcpy(indentification_result_query.CnmAddr, pConInfo->CycleInfo.CrntNodeAdd, MAC_ADDR_LEN);

		indentification_result_query.State         = UNEXECUTED;
		indentification_result_query.SendType      = INDENTIFICATION_QUERY;
		indentification_result_query.StaIndex      = pConInfo->CycleInfo.CurrentIndex;
	    indentification_result_query.DatagramSeq   = ++ApsSendPacketSeq;
		indentification_result_query.DealySecond   = INDENTIFICATION_COLLECT_BF_LIST.nr == 0?0:MULTITRANS_INTERVAL;
	    indentification_result_query.ReTransmitCnt = BFMAXNUM;
		
	    indentification_result_query.DltNum         = 1;
		indentification_result_query.ProtoType      = 0;
		indentification_result_query.FrameLen       = 0;
	    indentification_result_query.EntryListCheck = cco_area_ind_bf_entry_check;
		indentification_result_query.EntryListfail  = cco_area_ind_bf_entry_fail; 
		indentification_result_query.TaskPro        = cco_area_ind_bf_task_proc; 
	    indentification_result_query.TaskUpPro      = cco_area_ind_bf_task_up_proc;
		indentification_result_query.TimeoutPro     = NULL;

	    indentification_result_query.pMultiTransHeader = &INDENTIFICATION_COLLECT_BF_LIST;
		indentification_result_query.CRT_timer = indentification_multi_timer;
		multi_trans_put_list(indentification_result_query, NULL);
	}
    else
    {
        /*if(INDENTIFICATION_COLLECT_BF_LIST.nr == 0)
        {
            app_printf("INDENTIFICATION_COLLECT_BF_LIST.TaskRoundPro\n");
            INDENTIFICATION_COLLECT_BF_LIST.TaskRoundPro();
        }*/
    }
}

void cco_area_ind_ctrl_task_round(void)
{
	CONCURRENCY_INFO_t *pConInfo = &gl_cco_area_ind_info_t;

	if(gl_cco_area_ind_info_t.CycleInfo.CurrentIndex >= MAX_WHITELIST_NUM )
	{
		if(INDENTIFICATION_COLLECT_BF_LIST.nr == 0)
		{
			if(cco_area_ind_check_task_poll_finish(pConInfo) || pConInfo->CycleInfo.CycleNum ==0)
			{
				U16 ii,seq=0;
			    app_printf("cco_area_ind_check_task_poll_finish is TRUE\n");
				app_printf("seq\ttei\tresult\n");
				for(ii=0; ii<MAX_WHITELIST_NUM;ii++)
				{
					if(pConInfo->taskPolling[ii].Valid != 0)
					{
						app_printf("%d\t%d\t%d\n"
						,seq++,ii+2,pConInfo->taskPolling[ii].result);
					}
				}
				cco_area_ind_flush_ctrl_t.state=e_START_COLLECT_PERIOD;
				Final_result_t.PublicResult_t[0].CalculateMaxNum = MAX_CACULATE/2;
				Final_result_t.PublicResult_t[0].CalculateNum = 0;
				cco_area_ind_period_collect_info_t.CrntNodeIndex =0;	

				app_printf("------------next period again-------------\n");
                cco_area_ind_ctrl_timer_modify(5*TIMER_UNITS);
			}
			else
			{
				//开始新的一轮
				pConInfo->CycleInfo.CycleNum--;
				pConInfo->CycleInfo.CurrentIndex = 0;
				app_printf("next cycle,CycleNum=%d\n",pConInfo->CycleInfo.CycleNum);
				cco_area_ind_add_task_list();
			}
		}
	}

}

void cco_area_ind_query_result_proc(INDENTIFICATION_IND_t *pIndentificationInd)
{
    MULTI_TASK_UP_t MultiTaskUp;
    
    MultiTaskUp.pListHeader =  &INDENTIFICATION_COLLECT_BF_LIST;
                                  
    MultiTaskUp.Downoffset = (U8)offset_of(multi_trans_t , CnmAddr);
    MultiTaskUp.Upoffset = (U8)offset_of(INDENTIFICATION_IND_t , SrcMacAddr);
    MultiTaskUp.Cmplen = sizeof(pIndentificationInd->SrcMacAddr);
    
	multi_trans_find_plc_task_up_info(&MultiTaskUp, (void*)pIndentificationInd);    
    return;
}

/*************************************************************************
 * 函数名称 :	U8 cco_area_ind_check_zero()
 * 函数说明 :	
 *      本函数检查CCO过零NTB工频特征信息是否正常，正常则返回TRUE,否则返回FALSE
 * 参数说明 :	
 *     无
 * 返回值   :   
 *     U8  正常则返回TRUE,否则返回FALSE
 *************************************************************************/
U8 cco_area_ind_check_zero()
{
	if(((Period_info_t[1].startntb - Period_info_t[0].startntb) >500000 && nnib.PhasebitA == TRUE && nnib.PhasebitB == TRUE)|| 
		((Period_info_t[2].startntb - Period_info_t[0].startntb) >500000 && nnib.PhasebitA == TRUE && nnib.PhasebitC == TRUE)|| 
		((Period_info_t[4].startntb - Period_info_t[3].startntb) >500000 && nnib.PhasebitA == TRUE && nnib.PhasebitB == TRUE)|| 
		((Period_info_t[5].startntb - Period_info_t[3].startntb) >500000&& nnib.PhasebitA == TRUE && nnib.PhasebitC == TRUE))
	{
		app_printf("CCO zero is False\n");
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	
}


/*
void find_indentification_bf_task_info(work_t *work)
{
	multi_trans_header_t       *pIndentificationBFlistheader;
    INDENTIFICATION_IND_t      *pIndentificationInd = (INDENTIFICATION_IND_t*)work->buf;

    multi_trans_t  *pIndenBFTask;
    list_head_t *pos,*node;


	pIndentificationBFlistheader = &INDENTIFICATION_COLLECT_BF_LIST;

    if(pIndentificationBFlistheader->nr <= 0)
	{
		return;
	}
    
    list_for_each_safe(pos, node,&pIndentificationBFlistheader->link)
    {
        pIndenBFTask = list_entry(pos , multi_trans_t , link);
        if(pIndenBFTask->State == EXECUTING && pIndenBFTask->DatagramSeq == pIndentificationInd->DatagramSeq)
        {
            if(pIndenBFTask->TaskUpPro != NULL)
            {
                pIndenBFTask->TaskUpPro(pIndenBFTask, pIndentificationInd);
            }
			pIndenBFTask->State = NONRESP;
        }
    }
	return;
}
*/


#endif




