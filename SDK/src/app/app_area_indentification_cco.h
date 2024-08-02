#ifndef __APP_AREA_INDENTIFICATION_CCO_H__
#define __APP_AREA_INDENTIFICATION_CCO_H__

#include "app_common.h"
#include "list.h"
#include "types.h"
#include "app_base_multitrans.h"
#include "aps_layer.h"

#if defined(STATIC_MASTER)
/*������������������������������������������������������������������ȫ�֡���������������������������������������������������������*/
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


#define AREA_IND_RESULT_TIME_OUT	(2*60*60*1000)	//̨��ʶ���ѯ�����ʱ

#define AREA_IND_TASK_TIME_OUT		(8*60*60*1000)	//̨��ʶ�������ܳ�ʱ

U8 cco_area_ind_round;

/*������������������������������������������������������������������ö�١���������������������������������������������������������*/
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


/*�������������������������������������������������������������������ṹ�����������������������������������������������������������*/

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
	U8  mode;						//ģʽ��0����ʽ��1�ֲ�ʽ
	U8  state;						//״̬��01��ѹ�ɼ�״̬��02Ƶ�ʲɼ�״̬��04��Ƶ�ɼ�״̬
	U16 plan_set        : 3 ;		//�ƻ���01��ѹ��02Ƶ�ʣ�04��Ƶ
	U16 plan_programing : 3 ;		//û��ʹ��
	U16 plan_collecting : 3 ;		//�ɼ����ͣ���ѹ��Ƶ�ʣ���Ƶ
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
    U8    ProtoType;											/* ����D?D-��������D�� */

	U8    Framenum;
    U16   DownFrameLen;											/* ����??3��?�� */
    U8   *DownFrameUnit;				                        /* ����???����Y */
    U8    Addr[MAC_ADDR_LEN];
    U8    CnmAddr[MAC_ADDR_LEN];
	U8    DltNum;
	U16   DatagramSeq;

} __PACKED DOWN_INFO_t;

typedef struct
{
    U8    Valid;
 
    U16   UpFrameLen;											/* ����??3��?�� */
    U8   *UpFrameUnit;				                        /* ����???����Y */
    U8    Addr[MAC_ADDR_LEN];
    U8    CnmAddr[MAC_ADDR_LEN];
	U8    DltNum;
	U16   DatagramSeq;

} __PACKED UP_INFO_t;

typedef struct
{
    U8	  Valid    :  1;     
	U8    result   :  1;//?��1???1
	U8    result2  :  1;//?��1???1
	U8    res	   :  5;

} __PACKED Result_t;

typedef struct
{
    U8             priority;
    
    DOWN_INFO_t    down_info;
    UP_INFO_t      up_info;
    Result_t       result;
        
	U8             Valid;
	U8	           Tasktype     ;//2�顤�騨???����D��
	U16            SlaveIndex ;
    U8             MacAddr[MAC_ADDR_LEN];
    S8             DealySecond;
    S8             ReTransmitCnt;
	list_head_t	   con_list;
} __PACKED BF_LIST_t;


//�̡�?��2�顤����?��
typedef struct
{
	U8    Valid;
	U8	  Tasktype     ;//2�顤�騨???����D��
	U16   SlaveIndex ;
    U8    MacAddr[MAC_ADDR_LEN];
    S8    DealySecond;
    S8    ReTransmitCnt;
} __PACKED BF_WINDOW_t;


//BF_WINDOW_t CurveCfg_BF_Window_List[BF_WIN_SIZE];


//??��?????������1��?��??��11
typedef struct{
	U16		CurrentIndex;   //�̡�?��?��?��D��o?
	U8		CrntNodeAdd[6]; //�̡�?��?��?������D?��??��
	S8      ReTransmitCnt;	//���ꨮ��??��?��?��y
	U8 		CycleNum;		//��?�䨮??��?
	U8		Tasktype;		//2�顤�騨???����D��
}__PACKED  CYCLE_INFO_t;


CYCLE_INFO_t CurveCycleInfo;
//2�顤�騨???��????�騢��3��?��?o��???????
typedef struct{

    CCO_TASK_RECORD     taskPolling[MAX_WHITELIST_NUM]; //��??????TEI��DD��?��?��1??	BF_WINDOW_t         BF_Window_List[BF_WIN_SIZE];    //�̡�?�㨨????o��???
	CYCLE_INFO_t        CycleInfo;                      //�̡�?�㨨????????��
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

