#include "app_phaseseq_judge_sta.h"
#include "app_power_onoff.h"
#include "dl_mgm_msg.h"
#include "app_dltpro.h"
#include "dev_manager.h"
#include "printf_zc.h"
#include "app_event_report_cco.h"


U16 Meter_3_EventSendPacketSeq =0;
#if defined(ZC3750STA)

ostimer_t *phaseseqJudgetimer = NULL;

static void sta_phaseseq_judge_proc(work_t *work)
{
    U8 event[10] = {0};
    U16 eventlen = 0;
    U8 phase = 0;
    U32 ntb = 0;
    U32 staNtb = 0;

    /*ZeroData[e_A_PHASE-1].NTB[ZeroData[e_A_PHASE-1].NewOffset]
    ZeroData[e_B_PHASE-1].NTB[ZeroData[e_B_PHASE-1].NewOffset]
    ZeroData[e_C_PHASE-1].NTB[ZeroData[e_C_PHASE-1].NewOffset]*/

    if(GetNodeState() != e_NODE_ON_LINE)
    {
        timer_start(phaseseqJudgetimer);
        return;
    }

    if(nnib.DeviceType != e_3PMETER)
    {
        app_printf("hebei, device_type err, d_t=%d\n", nnib.DeviceType);
        return;
    }

    event[eventlen++] = e_3_PHASE_METER_SEQ;
    __memcpy(event + eventlen, DevicePib_t.DevMacAddr, MACADRDR_LENGTH);
    eventlen += MACADRDR_LENGTH;

    if(nnib.PhasebitA == TRUE && nnib.PhasebitB == TRUE)//123 12X
    {
        ntb = ZeroData[e_A_PHASE - 1].NTB[ZeroData[e_A_PHASE - 1].NewOffset];
        staNtb = ZeroData[e_B_PHASE - 1].NTB[ZeroData[e_B_PHASE - 1].NewOffset];
        CalculateNTBDiff(ntb
                            , &staNtb
                            //CalculateNTBDiff(ZeroData[e_A_PHASE - 1].NTB[ZeroData[e_A_PHASE - 1].NewOffset]
                            //, &ZeroData[e_B_PHASE - 1].NTB[ZeroData[e_B_PHASE - 1].NewOffset]
                            , 1
                            , &phase
                            , NULL);
        event[eventlen++] = (phase != e_B_PHASE)?1:0;   //0：相序正常， 1：相序异常
    }
    else if(nnib.PhasebitA == FALSE && nnib.PhasebitB == TRUE && nnib.PhasebitC == TRUE) //X23
    {
        ntb = ZeroData[e_B_PHASE - 1].NTB[ZeroData[e_B_PHASE - 1].NewOffset];
        staNtb = ZeroData[e_C_PHASE - 1].NTB[ZeroData[e_C_PHASE - 1].NewOffset];
        CalculateNTBDiff(ntb
                            , &staNtb
                            , 1
                            , &phase
                            , NULL);
        event[eventlen++] = (phase != e_B_PHASE)?1:0;   //0：相序正常， 1：相序异常
    }
    else if(nnib.PhasebitA == TRUE && nnib.PhasebitB == FALSE && nnib.PhasebitC == TRUE)//1X3
    {
        ntb = ZeroData[e_A_PHASE - 1].NTB[ZeroData[e_A_PHASE - 1].NewOffset];
        staNtb = ZeroData[e_C_PHASE - 1].NTB[ZeroData[e_C_PHASE - 1].NewOffset];
        CalculateNTBDiff(ntb
                            , &staNtb
                            //CalculateNTBDiff(ZeroData[e_A_PHASE - 1].NTB[ZeroData[e_A_PHASE - 1].NewOffset]
                            //, &ZeroData[e_C_PHASE - 1].NTB[ZeroData[e_C_PHASE - 1].NewOffset]
                            , 1
                            , &phase
                            , NULL);
        event[eventlen++] = (phase != e_B_PHASE)?0:1;   //0：相序正常， 1：相序异常
    }
    else
    {
        event[eventlen++] = 0; //相序无异常
    }

    //TODO  report event
    Meter_3_EventSendPacketSeq++;

    app_printf("Report Phase Event!  Meter_3_EventSendPacketSeq = %d\n", Meter_3_EventSendPacketSeq);
    app_printf("A-%d,B-%d,C-%d, phase=%d, eventLen = %d, eventBuf = ", nnib.PhasebitA, nnib.PhasebitB, nnib.PhasebitC, phase, eventlen);
    dump_buf(event, eventlen);
    power_off_event_report(e_STA_ACT_CCO, e_3_PHASE_METER_SEQ, e_UNICAST_FREAM, event, eventlen,Meter_3_EventSendPacketSeq);
}

static void sta_phaseseq_judge_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t), "phaseseqJudge", MEM_TIME_OUT);
    work->work = sta_phaseseq_judge_proc;
    post_app_work(work);
}

void sta_phaseseq_judgetimer_init()
{
    phaseseqJudgetimer = timer_create(30* TIMER_UNITS,
                                      20* TIMER_UNITS,
                                      TMR_PERIODIC ,
                                      sta_phaseseq_judge_timer_cb,
                                      NULL,
                                      "phaseseqJudgetimerCB"
                                      );

    if(NULL != phaseseqJudgetimer)
    {
        timer_start(phaseseqJudgetimer);
    }
}


void sta_phaseseq_create_event_timer(U32 MStimes)
{
    if(NULL == phaseseqJudgetimer)
    {
        sys_panic("<eventreporttimer fail!> %s: %d\n");
        return;
    }
        
    if(TMR_RUNNING == timer_query(phaseseqJudgetimer))
    {
        app_printf("Timer phaseseqJudgetimer is Running, Stop it\n");
        timer_stop(phaseseqJudgetimer, TMR_NULL);
    }

    if(TMR_STOPPED == timer_query(phaseseqJudgetimer))
    {
        app_printf("Timer phaseseqJudgetimer is Stopped  And modify  interval To %d ms\n", MStimes);
        timer_modify(phaseseqJudgetimer,
                        MStimes/10,
                        20 * TIMER_UNITS,
                        TMR_PERIODIC ,//TMR_TRIGGER
                        sta_phaseseq_judge_timer_cb,
                        NULL,
                        "phaseseqJudgetimerCB",
                        TRUE);
        timer_start(phaseseqJudgetimer);
    }
    else
    {
        app_printf("phaseseqJudgetimer is not Stopped, Can not modify it !\n");
    }
}


#endif 


