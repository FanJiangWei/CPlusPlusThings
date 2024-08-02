#ifndef __APP_AREA_INDENTIFICATION_CCO_H__
#define __APP_AREA_INDENTIFICATION_CCO_H__

#include "app_common.h"
#include "list.h"
#include "types.h"
#include "app_base_multitrans.h"
#include "aps_layer.h"

#if defined(STATIC_MASTER)
/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓全局↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
#define BF_TIM   		500   //100MS
#define BF_WIN_SIZE   	5

#define DELAY_INTERVAL 	4000/BF_TIM
#define BFMAXNUM       	5
#define TASK_TYPE_NUM 	3
#define MAX_FEATURE_COLLECTION_CNT  5
#define INDENTIFICATION_MAX_CNT  10

#define voltage_bit   0x01 
#define frequncy_bit  0x02
#define period_bit    0x04 

#define    DEF_IND_EDGE  e_COLLECT_NEGEDGE


#define AREA_IND_RESULT_TIME_OUT	(2*60*60*1000)	//台区识别查询结果超时

#define AREA_IND_TASK_TIME_OUT		(8*60*60*1000)	//台区识别任务总超时

U8 cco_area_ind_round;

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓枚举↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/
typedef enum
{
	e_TIMEOUT,
	e_COMPLETE,
}CON_TASK_DEL_LIST_TYPE_E;

enum
{
    e_CENTRALIZATION  = 0,
    e_DISTRIBUT    = 1,
}MODE_E;

enum
{
	e_START_COLLECT_VOLTAGE = voltage_bit,
	e_START_COLLECT_FREQUNCY = frequncy_bit,
	e_START_COLLECT_PERIOD = period_bit,	
    e_COLLECT_INFO,
    e_INFOMING_INFO,
    e_RESULT_QUERY,
} ;

typedef enum
{
	e_REGISTER=0,
    e_REGISTERLOCK,    
    e_UPGRADESTART,
	e_AREA_COLLECT,
	e_AREA_RESULT,
	e_CURVECFG,
} CON_TASK_TYPE_e;


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓结构体↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

typedef struct{
	uint8_t  mode;
	uint8_t  plan;
}INDENTIFICATION_TASK_CFG_t;

typedef struct indentification_bf_query_s{
	list_head_t link;
	multi_trans_header_t  *plisthead;
	ostimer_t   *ostimer;
}indentification_bf_query_t;

typedef struct
{
	U8  mode;						//模式，0集中式，1分布式
	U8  state;						//状态，01电压采集状态，02频率采集状态，04工频采集状态
	U16 plan_set        : 3 ;		//计划，01电压，02频率，04工频
	U16 plan_programing : 3 ;		//没有使用
	U16 plan_collecting : 3 ;		//采集类型，电压，频率，工频
	U16 plan_complete   : 3 ;
	U16                 : 4 ;
} __PACKED INDENTIFICATION_CTRL_t;


typedef struct{
    
    U8      CrntNodeAdd[6]; 
    U16     NodetotalNum;
    U16     CrntNodeIndex;
    U8      CrntRound;
    S8      ReTransmitCnt;
	U8      gettophandle;
	U8      Edgeinfo;
    U8      res[2];
}__PACKED  FEATURE_COLLECT_INFO_t;



typedef struct{
    U8      feature;
	float   value[INDENTIFICATION_MAX_CNT];
}__PACKED  VALUE_INFO_t;

extern VALUE_INFO_t cco_area_ind_value_info_t[MAX_WHITELIST_NUM];



typedef struct{
	
    U8	  Valid    :  1;     
	U8    result   :  1;
	U8    result2  :  1;
	U8    res	   :  5;
	U8	  Tasktype     ;
}__PACKED CCO_TASK_RECORD;
  
CCO_TASK_RECORD  ccotaskPolling[MAX_WHITELIST_NUM];

typedef struct
{
    U8    Valid;
    U8    ProtoType;											/* í¨D?D-òéààDí */

	U8    Framenum;
    U16   DownFrameLen;											/* ±¨??3¤?è */
    U8   *DownFrameUnit;				                        /* ±¨???úèY */
    U8    Addr[MAC_ADDR_LEN];
    U8    CnmAddr[MAC_ADDR_LEN];
	U8    DltNum;
	U16   DatagramSeq;

} __PACKED DOWN_INFO_t;

typedef struct
{
    U8    Valid;
 
    U16   UpFrameLen;											/* ±¨??3¤?è */
    U8   *UpFrameUnit;				                        /* ±¨???úèY */
    U8    Addr[MAC_ADDR_LEN];
    U8    CnmAddr[MAC_ADDR_LEN];
	U8    DltNum;
	U16   DatagramSeq;

} __PACKED UP_INFO_t;

typedef struct
{
    U8	  Valid    :  1;     
	U8    result   :  1;//?á1???1
	U8    result2  :  1;//?á1???1
	U8    res	   :  5;

} __PACKED Result_t;

typedef struct
{
    U8             priority;
    
    DOWN_INFO_t    down_info;
    UP_INFO_t      up_info;
    Result_t       result;
        
	U8             Valid;
	U8	           Tasktype     ;//2￠·￠è???ààDí
	U16            SlaveIndex ;
    U8             MacAddr[MAC_ADDR_LEN];
    S8             DealySecond;
    S8             ReTransmitCnt;
	list_head_t	   con_list;
} __PACKED BF_LIST_t;


//μ±?°2￠·￠′°?ú
typedef struct
{
	U8    Valid;
	U8	  Tasktype     ;//2￠·￠è???ààDí
	U16   SlaveIndex ;
    U8    MacAddr[MAC_ADDR_LEN];
    S8    DealySecond;
    S8    ReTransmitCnt;
} __PACKED BF_WINDOW_t;


//BF_WINDOW_t CurveCfg_BF_Window_List[BF_WIN_SIZE];


//??′?????ê±ê1ó?μ??á11
typedef struct{
	U16		CurrentIndex;   //μ±?°?￡?éDòo?
	U8		CrntNodeAdd[6]; //μ±?°?￡?éí¨D?μ??·
	S8      ReTransmitCnt;	//ê￡óà??ê?′?êy
	U8 		CycleNum;		//×?′ó??′?
	U8		Tasktype;		//2￠·￠è???ààDí
}__PACKED  CYCLE_INFO_t;


CYCLE_INFO_t CurveCycleInfo;
//2￠·￠è???μ????￠á÷3ì?￠?o′???????
typedef struct{

    CCO_TASK_RECORD     taskPolling[MAX_WHITELIST_NUM]; //è??????TEIóDD§?￠?á1??	BF_WINDOW_t         BF_Window_List[BF_WIN_SIZE];    //μ±?°è????o′???
	CYCLE_INFO_t        CycleInfo;                      //μ±?°è????????÷
	//BF_WINDOW_t			BF_Window_List[BF_WIN_SIZE];
}__PACKED  CONCURRENCY_INFO_t;

extern CONCURRENCY_INFO_t CurveConInfo;


uint8_t cco_area_ind_get_feature_by_state(uint8_t state);

void cco_area_ind_add_task_list(void);

void cco_area_ind_ctrl_task_round(void);

void find_indentification_bf_task_info(work_t *work);

void cco_area_ind_featrue_collecting(uint8_t plan_collecting, uint8_t state);

U8 cco_area_ind_check_zero(void);

void cco_area_ind_task_cfg(uint8_t mode, uint8_t plan);

void cco_area_ind_start(void);

void cco_area_ind_stop(void);

void cco_area_ind_report_result(U16 TEI,U8 Result,U8 *BelongCCOMac);

void cco_area_ind_query_result_proc(INDENTIFICATION_IND_t *pIndentificationInd);


#endif



#endif

