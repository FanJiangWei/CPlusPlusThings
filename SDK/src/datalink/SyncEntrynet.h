#ifndef __SYNCENTRYNET_H__
#define __SYNCENTRYNET_H__
#include "phy.h"
#include "sys.h"



typedef enum
{
    e_LISEN,//�ϵ磬����
    e_ESTIMATE,//ͬ��״̬
	e_WAITBEACON,//30��Ȳ����ű�����
	e_BEACONSYNC,//�ȵ��ű�֮������ͬ��
	e_ENTER,//��ʼ�����׶�
    //e_WAITENTERCFM, //�ȴ������ظ�
} ModleWorkState_e;


extern ostimer_t *network_timer;
void modify_network_timer(uint32_t ms);

void StartsyncTEI(U32 SNID);

void StartSTAnetwork(U32 delaytime,U8 formationseq);
void StartSTABcnSync(U32 snid);
void StopThisEntryNet();

void Change_next_net_option();

#endif
