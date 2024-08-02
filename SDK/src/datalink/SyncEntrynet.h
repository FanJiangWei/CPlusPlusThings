#ifndef __SYNCENTRYNET_H__
#define __SYNCENTRYNET_H__
#include "phy.h"
#include "sys.h"



typedef enum
{
    e_LISEN,//上电，离线
    e_ESTIMATE,//同步状态
	e_WAITBEACON,//30秒等不到信标离线
	e_BEACONSYNC,//等到信标之后启动同步
	e_ENTER,//开始入网阶段
    //e_WAITENTERCFM, //等待关联回复
} ModleWorkState_e;


extern ostimer_t *network_timer;
void modify_network_timer(uint32_t ms);

void StartsyncTEI(U32 SNID);

void StartSTAnetwork(U32 delaytime,U8 formationseq);
void StartSTABcnSync(U32 snid);
void StopThisEntryNet();

void Change_next_net_option();

#endif
