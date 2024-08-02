#ifndef __APP_OFF_GRID_H__
#define __APP_OFF_GRID_H__

#if defined(STATIC_MASTER)

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓外部引用↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
extern U8  Gw3762TopUpReportData[512];


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓宏定义↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/


#define MAX_CHANGE_STATE_NUM              10//串口山西离网感知
#define MAX_CHANGE_STATE_LIST_NUM         40//链表数


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓枚举↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
typedef enum{
	e_OFF_GRID_REPORT = 0,
	e_OFF_GRID_NOREPROT = 1,
}REPORT_STATE_e;


typedef enum{
	e_OFF_2_ONLINE,
	e_ON_2_OFFILNE,
}STATE_CHANGE_e;


typedef enum{
	e_UNKOWN,
	e_POWER_OFF,
	e_CHANNEL_CHANGE,
	e_RESV,
}OFFLINE_REASON_e;


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓结构体↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

//离网感知节点信息
typedef struct NODE_STATE_CHANGE_INFO{
	U8  addr[6];			//节点地址
	U8  changestate;		//节点入网还是离网
	U8  outlinereason;		//离网原因
	
	U32 outlinetime;		//离网时长

	U8  chip_id[24];
}__PACKED  NODE_STATE_CHANGE_INFO_t;


//离网感知链表头
typedef struct OFF_GRID_INFO_HEAD{
	list_head_t link;
	
	uint16_t nr;
	uint16_t thr;
}__PACKED OFF_GRID_LIST_HEADER_t;


//离网感知
typedef struct OFF_GRID_INFO{
	list_head_t link;
    NODE_STATE_CHANGE_INFO_t node_info_t;	//节点信息
    U8 state;			//上报状态
	U8 res[3];
}__PACKED OFF_GRID_LIST_t; 


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓接口函数↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

void off_grid_list_init(void);

void off_grid_off_to_on(DEVICE_TEI_LIST_t *arg_dev_t);

void off_grid_on_to_off(DEVICE_TEI_LIST_t *arg_dev_t);

void off_grid_up_data(DEVICE_TEI_LIST_t *arg_dev_t);

void off_grid_set_reset_reason(U8 arg_power_on_off, U16 seq);

#endif

#endif

