#ifndef _APP_CLOCK_SYNC_STA_H_
#define _APP_CLOCK_SYNC_STA_H_
#include "app_sysdate.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"


#define TimeCalIntarval 50
#define PIB_TIMEDEVIATION   0x3FFFFFFF


#if defined(STATIC_NODE) 


typedef enum
{
	e_GW_BASE = 1,
	e_SHANDONG = 2,
	e_HEILONGJIANG = 3,
	//....
}PROVINCE_TYPE_t;
	
typedef enum
{
	e_READ_METER_TIME_NONE,
	e_READ_METER_TIME_SUCCESS,
	e_READ_METER_TIME_TIMEOUT
}READ_METER_RES_t;
void read_meter_time_timeout_handle(void *pMsgNode);
extern READ_METER_RES_t g_ReadMeterResFlag;


#define TIME_OVER_CTRL_TIME_VALUE 3
#define READ_CLK_CTRL_TIME_VALUE  3
#define CLOCK_RECOVER_TIME_VALUE  1

typedef struct 
{
	//PROVINCE_TYPE_t ProvinceType;
	U8 AutoCaliState;
	U8 DeviationReport;
}__PACKED FUNCTION_USE_SWITCH_t;
extern FUNCTION_USE_SWITCH_t Function_Use_Switch;
extern U8	ReadClockState;
extern ostimer_t *readMeterClockTimer;

extern U8 ApsPacketSeq;
extern ostimer_t *AutoCalibrateTimer ;
extern ostimer_t *ApsLifeCycletimer;
extern ostimer_t *EventReportClockOvertimer;
extern U8	ReadClockWayState;

void date_time_to_sysdate(SysDate_t *sTime, date_time_s dTime);
U8 sta_calibration_clk(U8* buf, U16 len);
void sta_pkg_698_bc_cali_frame(U8 *pData,U16 *pDatalen,date_time_s pSysDateTmp);
void sta_auto_cali_time(void);

void sta_cali_clk_put_list(U8 *pTimeData, U16 dataLen, U8 Baud,U8 Protocl);
void sta_set_moudle_time(U8* data,U16 datalen,U16 OffSet);
void sta_clk_maintain_proc(work_t *work);
void query_clk_or_value_proc(work_t *work);
void sta_read_meter_clk();

extern void sta_query_clk_or_value_Indication(QUERY_CLKSWOROVER_VALUE_IND_t *QueryClkORValueInd);
extern void sta_set_clk_or_value_Indication(CLOCK_MAINTAIN_IND_t *SetClkORValueInd);

extern void aps_life_cycle_cb(struct ostimer_s *ostimer, void *arg);
extern void sta_clear_time_manager_flash();
U8  clock_recv_infoproc(U8 *buff,U16 bufflen);

#endif
void Module_Time_Sync_Ind(work_t *work);

#endif

