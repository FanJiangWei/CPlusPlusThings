
#include "trios.h"
#include "datalinkglobal.h"
#include "datalinkdebug.h"
#include "app_gw3762_ex.h"
#include "app.h"
#include "schtask.h"
#include "app_event_report_sta.h"
#include "netnnib.h"
#include "app_off_grid.h"
#include "app_event_report_cco.h"


#if defined(STATIC_MASTER)

static void off_grid_ctrl_timer_fun(work_t *work);

static OFF_GRID_LIST_HEADER_t off_grid_list_t;

static ostimer_t *off_grid_ctrl_timer = NULL;

static void  off_grid_ctrl_timercb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"off_grid_ctrl_timercb",MEM_TIME_OUT);
	work->work = off_grid_ctrl_timer_fun;
    work->msgtype = OFF_GRID;
	post_app_work(work);
}


static void off_grid_ctrl_timer_modify(void)
{
	if(off_grid_ctrl_timer == NULL)
	{
		off_grid_ctrl_timer = timer_create(2*TIMER_UNITS,
							0,
							TMR_TRIGGER,
							off_grid_ctrl_timercb,
							NULL,
							"off_grid_ctrl_timercb"
						   );
	}
	else
	{
		timer_modify(off_grid_ctrl_timer,
							2*TIMER_UNITS, 
							0, 
							TMR_TRIGGER, 
							off_grid_ctrl_timercb, 
							NULL, 
							"off_grid_ctrl_timercb",
							TRUE);
	}
	timer_start(off_grid_ctrl_timer);
}


void off_grid_list_init()
{
	INIT_LIST_HEAD(&off_grid_list_t.link);
	off_grid_list_t.nr = 0;
	off_grid_list_t.thr = MAX_CHANGE_STATE_LIST_NUM;	
}


static U16 off_grid_get_num_by_state(U8 state)
{
    list_head_t *pos,*node;
	OFF_GRID_LIST_t *pReadTask;
	U16 num = 0;
	
	list_for_each_safe(pos, node,&off_grid_list_t.link)
	{
		pReadTask = list_entry(pos, OFF_GRID_LIST_t, link);
		if(off_grid_list_t.nr > 0 && pReadTask->state == state)
		{
			num++;
		}
	}
	return num;
}


static OFF_GRID_LIST_t *off_grid_list_by_state(U16 state)
{
    list_head_t *pos,*node;
	OFF_GRID_LIST_t *pReadTask;
	
	list_for_each_safe(pos, node,&off_grid_list_t.link)
	{
		pReadTask = list_entry(pos, OFF_GRID_LIST_t, link);
		if(off_grid_list_t.nr > 0 && pReadTask->state == state)
		{
			return pReadTask;
		}
	}

	return NULL;
}


static U16 off_grid_del_all_list_by_state(U16 state)
{
	OFF_GRID_LIST_t *del_ptr=NULL;
	U16 num=0;
	do{
		del_ptr = off_grid_list_by_state(state);
		if(del_ptr != NULL)
		{
			num++;
			list_del(&del_ptr->link);
			zc_free_mem(del_ptr);
		}
	}while(del_ptr!= NULL);
	return num;
}


static void off_grid_ctrl_timer_fun(work_t *work)
{
    U16 byLen=0;
    U16 list_num=0;
    U8  index = 0;
    list_head_t *pos,*node;
	OFF_GRID_LIST_t *pReadTask;

	if(serial_cache_list_ide_num() <= SERIAL_IDE_NUM)
	{
		off_grid_ctrl_timer_modify();
		return;
	}
	
    list_num = off_grid_get_num_by_state(UNEXECUTED);    //通过状态来获取链表中未上报的节点数量
    app_printf("net sense list num : %d\n",list_num);
	
	if(list_num == 0)
	{
		app_printf("net sense report list null\n");
        return;
	}
	
    if(list_num > MAX_CHANGE_STATE_NUM)
    {
        list_num = MAX_CHANGE_STATE_NUM;
		off_grid_ctrl_timer_modify();
    }
	
	Gw3762TopUpReportData[byLen++] = list_num&0x00FF;//上报总数量
	Gw3762TopUpReportData[byLen++] = (list_num>>8)&0x00FF;
	Gw3762TopUpReportData[byLen++] = list_num&0x00FF;//本次上报数量
	Gw3762TopUpReportData[byLen++] = 0;
	Gw3762TopUpReportData[byLen++] = 0;
	
	list_for_each_safe(pos, node, &off_grid_list_t.link)
	{
		pReadTask = list_entry(pos, OFF_GRID_LIST_t, link);
		if(off_grid_list_t.nr > 0 && pReadTask->state == UNEXECUTED)
		{
			if(index >= list_num)
			{
				break;
			}
			__memcpy(&Gw3762TopUpReportData[byLen],pReadTask->node_info_t.addr, 6);
			ChangeMacAddrOrder(&Gw3762TopUpReportData[byLen]);
			byLen += 6;
			Gw3762TopUpReportData[byLen++] = pReadTask->node_info_t.changestate;
			__memcpy(&Gw3762TopUpReportData[byLen], &pReadTask->node_info_t.outlinetime, sizeof(U32));
			byLen += sizeof(U32);
			Gw3762TopUpReportData[byLen++] = pReadTask->node_info_t.outlinereason;
			if(PROVINCE_SHANNXI == app_ext_info.province_code)
			{
				__memcpy(&Gw3762TopUpReportData[byLen], &pReadTask->node_info_t.chip_id, sizeof(pReadTask->node_info_t.chip_id));
				byLen += sizeof(pReadTask->node_info_t.chip_id);
			}
			pReadTask->state = NONRESP;
			index++;
		}
	}
	
    off_grid_list_t.nr = off_grid_list_t.nr - off_grid_del_all_list_by_state(NONRESP);
	
	app_gw3762_afn06_f10_report_node_state_change(Gw3762TopUpReportData, byLen, e_UART1_MSG);
}


static void off_grid_add_list(struct list_head* new, struct list_head *header)
{
    list_add(new , header->prev);
}

static OFF_GRID_LIST_t *off_grid_list_create(NODE_STATE_CHANGE_INFO_t *pdata)
{
	NODE_STATE_CHANGE_INFO_t *node_state_change_info_t  = pdata;
	OFF_GRID_LIST_t *net_sense_task_t = zc_malloc_mem(sizeof(OFF_GRID_LIST_t),"OFF_GRID",MEM_TIME_OUT);
	__memcpy(&(net_sense_task_t->node_info_t),node_state_change_info_t,sizeof(NODE_STATE_CHANGE_INFO_t));
	net_sense_task_t->link.next = net_sense_task_t->link.prev = NULL;
	net_sense_task_t->state = UNEXECUTED;

	return net_sense_task_t;
}

static void off_grid_add_list_fun(NODE_STATE_CHANGE_INFO_t *pData)
{
    NODE_STATE_CHANGE_INFO_t *node_info;
    node_info = pData;
    OFF_GRID_LIST_t *new_list = off_grid_list_create(node_info);
	off_grid_add_list(&new_list->link, &off_grid_list_t.link);
	off_grid_ctrl_timer_modify();
}





static void off_grid_pack_list(U8 *nodeaddr,  STATE_CHANGE_e state, U32 outlinetime, OFFLINE_REASON_e reason, U8 *arg_chip_id)
{
    NODE_STATE_CHANGE_INFO_t  NodeInfo;
	if(nodeaddr)
		__memcpy(NodeInfo.addr, nodeaddr, 6);
    NodeInfo.changestate = state;
    NodeInfo.outlinetime = outlinetime;
    NodeInfo.outlinereason = reason;
	if(arg_chip_id)
    	__memcpy(NodeInfo.chip_id,arg_chip_id,24);
	off_grid_add_list_fun((NODE_STATE_CHANGE_INFO_t *)&NodeInfo);
}


void off_grid_up_data(DEVICE_TEI_LIST_t *arg_dev_t)
{
	if(app_ext_info.func_switch.NetSenseSWC == 0)
	{
		if(arg_dev_t->OntoOff == e_OFF_GRID_REPORT)
			arg_dev_t->OntoOff = e_OFF_GRID_NOREPROT;
		if(arg_dev_t->OfftoOn == e_OFF_GRID_REPORT)
			arg_dev_t->OfftoOn = e_OFF_GRID_NOREPROT;
		return;
	}
	
	if(off_grid_list_t.nr < off_grid_list_t.thr)
	{
		if(arg_dev_t->OntoOff == e_OFF_GRID_REPORT)
		{
			//河北掉线，如果是停电，则不进行上报
			if(PROVINCE_HEBEI == app_ext_info.province_code && arg_dev_t->power_off_fg == TRUE)
			{
				arg_dev_t->power_off_fg = FALSE;
				app_printf("HeBei, power off, net sense no report!!!!\n");
			}
			else
			{
				off_grid_pack_list(arg_dev_t->MacAddr, e_ON_2_OFFILNE, 0xEEEEEEEE, e_UNKOWN,arg_dev_t->ManageID);
				off_grid_list_t.nr++;
			}
			arg_dev_t->OntoOff = e_OFF_GRID_NOREPROT;
			
		}
		else if(arg_dev_t->OfftoOn == e_OFF_GRID_REPORT)
		{
			//河北掉线变成在线不上报
			if(PROVINCE_HEBEI != app_ext_info.province_code)
			{
				off_grid_pack_list(arg_dev_t->MacAddr, e_OFF_2_ONLINE, arg_dev_t->Offtime, arg_dev_t->power_off_fg == TRUE ? e_POWER_OFF : e_CHANNEL_CHANGE,arg_dev_t->ManageID);
				off_grid_list_t.nr++;
			}
			else
			{
				app_printf("HeBei, off to on, net sense no report!!!!\n");
			}
			arg_dev_t->power_off_fg = FALSE;
			arg_dev_t->OfftoOn = e_OFF_GRID_NOREPROT;
			
		}
	}
}



void off_grid_off_to_on(DEVICE_TEI_LIST_t *arg_dev_t)
{
	if(arg_dev_t->NodeState == e_NODE_OFF_LINE)//离网在入网
	{
		arg_dev_t->OfftoOn = e_OFF_GRID_REPORT;
		arg_dev_t->Offtime = nnib.RoutePeriodTime*10 - arg_dev_t->DurationTime;
	}
	else if(arg_dev_t->NodeState == e_NODE_OUT_NET)
	{
		arg_dev_t->OfftoOn = e_OFF_GRID_NOREPROT;
		arg_dev_t->OntoOff = e_OFF_GRID_NOREPROT;
		arg_dev_t->Proxytimes = 0;
		arg_dev_t->Offtimes = 0;
		arg_dev_t->Offtime = 0;
	}
}


void off_grid_on_to_off(DEVICE_TEI_LIST_t *arg_dev_t)
{
	arg_dev_t->OntoOff = e_OFF_GRID_REPORT;
	arg_dev_t->Offtimes ++;
}

void off_grid_set_reset_reason(U8 arg_power_on_off, U16 seq)
{
	if(arg_power_on_off == e_BITMAP_EVENT_POWER_OFF)
	{
		DeviceList_ioctl(DEV_SET_POWER_OFF,&seq,NULL);
	}
	else if(arg_power_on_off == e_BITMAP_EVENT_POWER_ON)
	{
		DeviceList_ioctl(DEV_RESET_POWER_OFF,&seq,NULL);
	}
}



#endif

