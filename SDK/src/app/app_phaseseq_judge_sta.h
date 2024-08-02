#ifndef APP_PHASESEQ_JUDGE_STA_H
#define APP_PHASESEQ_JUDGE_STA_H

#include "ZCsystem.h"

extern U16 Meter_3_EventSendPacketSeq;

#if defined(STATIC_NODE)
extern ostimer_t *phaseseqJudgetimer;
void sta_phaseseq_judgetimer_init();
void sta_phaseseq_create_event_timer(U32 MStimes);
#endif  //#if defined(STATIC_NODE)

#endif // APP_PHASESEQ_JUDGE_STA_H
