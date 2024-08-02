#ifndef _APP_CLOCK_SYNC_CCO_H_
#define _APP_CLOCK_SYNC_CCO_H_
#include "app_sysdate.h"
#include "app_clock_sync_comm.h"

#define MAX_EVENT_REPORT_NUM			2048//过滤最大存储事件上报数

/*******************广播周期配置（05F102）******************/
typedef struct{
U8   BroadcastCycle;
U8   CycleUnit;
U8   Res[2];
} __PACKED BroadcastCycleCfg_t;
extern BroadcastCycleCfg_t BroadcastCycleCfg;

/*
typedef struct{
    U8 		MacAddr[6];
    U16		RemainTimeEvent;
}ADDR_EVENT_t;*/
extern	U8 ModuleTimeSyncFlag;

extern U16	clock_maintenance_cycle;
extern ostimer_t *clockMaintaintimer;
//extern ADDR_EVENT_t EventAddrList[MAX_EVENT_REPORT_NUM];
extern U8 QuerySwORValueSeq;
extern ostimer_t *RTCTimeSyncSendTimer;
extern void cco_clock_maintain();
extern void cco_query_clk_Sw_o_over_value(void *pTaskPrmt);
extern void cco_set_clk_sw(void *pTaskPrmt);
extern void cco_set_over_time_report_value(void *pTaskPrmt);

extern void cco_query_clk_or_value_cfm(QUERY_CLKSWOROVER_VALUE_CFM_t  * pQueryClkORValueCfm);
extern void cco_set_clk_or_value_cfm(CLOCK_MAINTAIN_CFM_t  * pSetClkORValueCfm);
extern void cco_clk_maintain_timer_cb(struct ostimer_s *ostimer, void *arg);
void cco_modify_rtc_time_sync_send_timer(uint32_t Sec);
void cco_rtc_time_sync_send_proc(work_t* work);
void cco_cco_clk_maintain_proc(work_t *work);
void query_clk_or_value_proc(work_t *work);
void cco_time_manager_init();
void sta_clk_maintain_proc(work_t *work);

//void RpsSet_clkorOvertimerCB(struct ostimer_s *ostimer, void *arg);
//void Modify_Set_CLKSwOROverValueTimer(U32 Mstimes);
void cco_modify_read_JZQ_clk_timer(uint32_t Sec);
//陕西，湖南
void cco_check_and_run_module_time_syns(U8 *DataUnit);
void cco_set_auto_curve_req(U8 BroadcastCycle,U8 CycleUnit);
void cco_modify_curve_req_clock_timer(U32 first,uint32_t interval,uint32_t opt);

//江苏
void cco_module_time_syns_request(SysDate_t SysDateNow,U8 *addr);
void cco_set_sta_time_sync_modify(uint32_t first,U8 *Addr);

void Module_Time_Sync_Ind(work_t *work);

#endif

