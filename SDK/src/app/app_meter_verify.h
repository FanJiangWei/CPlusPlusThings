#ifndef __APP_METER_VERIFY_H__
#define __APP_METER_VERIFY_H__

#include "app_dltpro.h"
#include "app_meter_bind_sta.h"
#include "app_meter_common.h"

#if defined(ZC3750STA)

#define MAXRETRYCNT         1   //
#define METERCOMUMAXCNT     2   //
#define PLCRTTIMEOUTMAXCNT  5   //




typedef enum
{
    VerifyFirst_e,		
    VerifyCycle_e,		
    VerifyPlug_e,		
    VerifyZero_e,		
    VerifyCoErr_e,		
} VERIFY_STATE_e;

typedef struct
{
    ostimer_t               *timer;
    VERIFY_STATE_e          State;
    U8                      Cnt;
    U8                      PlcVerifyCnt;
    U32                     NextBaudRate;

} __PACKED COMU_VERIFY_t;

extern COMU_VERIFY_t ComuVerify;

typedef struct
{
    
	void   (*SuccProc)(void );	    /*?¨¦?¡è????*/
	void   (*FailProc)(void );      /*?¨¦?¡è?¡ì¡ã?*/
    U32    Time;                    /*?????????¨¦¡À¨ª???¡À??*/
    METER_COMU_TYPE_e State;
} METER_VERIFY_PROC;

extern METER_VERIFY_PROC MeterVerifyProc[];

void Modify_MeterVerifytimer(U32 Sec);

void StartMeterCheckManage(VERIFY_STATE_e state);
void MeterCheckSucc(U8 laststate);
void MeterCheckFail(U8 laststate);
void PlcReadMeterSucc(void);
void PlcReadMeterFail(void);

void RecordComuCheck(void);

U8 JudgeMeterCheckStateNotPlug();

#endif

#endif

