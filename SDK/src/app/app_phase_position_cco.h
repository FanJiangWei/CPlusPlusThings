#ifndef __APP_PHASE_POSITION_CCO_H__
#define __APP_PHASE_POSITION_CCO_H__

#include "app_base_multitrans.h"
#include "app_area_indentification_common.h"
#include "app_area_indentification_cco.h"


#if defined(STATIC_MASTER)
U8 gl_cco_phase_order;

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓宏定义↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/


/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓枚举↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

typedef enum{
    e_PHASE_POSITION_IDLE,		//上电后为空闲态
    e_FEATURE_CLLCT_START,		//组网完成后切换为，特征信息采集开始
	e_FEATURE_COLLECT,			//特征信息收集;
	e_PHASE_COLLECT,			//相位识别收集
}PHASE_POSITION_COLLECT_STATE_e;

/*↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓结构体↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓*/

typedef struct{
    
	U8   curraddr[6];
    U16  currindex;
	U8   currround;
    U8   res[3];
}__PACKED  PHASE_COLLECT_INFO_t;





typedef struct{
	FEATURE_COLLECT_INFO_t  feature_collect_info;
	PHASE_COLLECT_INFO_t    phase_collect_info;
    U32  timeout;
    U8   mode;
	U8   plan;
	U8   state;
    U8   res;
}__PACKED  PHASE_POSITION_PARA_t;

typedef struct phase_position_header_s{
	list_head_t link;
	PHASE_POSITION_PARA_t    *pPhaseParaHeader;
}__PACKED  phase_position_header_t;


void cco_phase_position_start(uint8_t mode, uint8_t plan);

void cco_phase_position_edge_proc(INDENTIFICATION_IND_t *pIndentificationInd);

void cco_phase_position_phase_proc(work_t *work);

int8_t cco_phase_position_timer_init(void);

void cco_phase_calculate_order(void);

#endif

#endif


