#ifndef _APP_CNF_H_
#define _APP_CNF_H_
#include "sys.h"
#include "types.h"
#include "SchTask.h"
#include "ZCsystem.h"


#define SEND_03F10_SEC  (1U)


void ProcSetMacAddrCfm(work_t *work);

void ProcSetCJQRegisterCfm(work_t *work);



void ProcNet2AppPLC2016Msg(MSG_t *pMsg);



extern U8  NetFormationFinishedFlag;
void	 ProcNetworkStatusChangeIndication(work_t *work);
U16 	 APP_GETTEI();
U32      APP_GETSNID();
U16 	 APP_GETDEVNUM();
U16 	 APP_GETTEI_BYMAC(U8 *MAC);

#if defined(STATIC_NODE)
extern U8 AppNodeState;
#endif

#endif



