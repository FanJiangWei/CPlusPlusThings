#ifndef __DEV_MANAGER_H__
#define __DEV_MANAGER_H__
#include "types.h"
#include "phy.h"
#include "app_common.h"
#include "SchTask.h"


#if defined(STATIC_MASTER)
#define MAXNTBCOUNT		300
#define PHASENUM						6

#elif defined(STATIC_NODE)
#define MAXNTBCOUNT		1000
#if defined(ZC3750STA)
#define PHASENUM						3
#elif defined(ZC3750CJQ2)
#define PHASENUM						1
#endif


#endif


#define    DIFF_RANGE             1500
#define    SELF_DIFF_RANGE        300

#define MAXNUMMEM  80

#if defined(ZC3750STA)
#define MAXBCCHECKNUM		100
#define BCCHECKDIFFNUM		60

#endif

typedef struct
{
	U32  time[MAXNUMMEM];
	U32  dur[MAXNUMMEM];
	char *s[MAXNUMMEM];
}__PACKED TIME_t;

extern TIME_t record_time_t;
void recordtime(char *s,U32 p);



#define   ZERO_A_PIN ((ZCA_CTRL&gpio_input(NULL,ZCA_CTRL))?1:0)
#define   ZERO_B_PIN ((ZCB_CTRL&gpio_input(NULL,ZCB_CTRL))?1:0)
#define   ZERO_C_PIN ((ZCC_CTRL&gpio_input(NULL,ZCC_CTRL))?1:0)



#if defined(STA_BOARD_3_0_02)
/*
typedef enum
{
    LED_KEEP  = 0x00,
    LED_OFF = 0x01,
    LED_ON  = 0x02,
    
} LedState_e;
*/




#endif

extern U8 ManageID[24];

extern U8 FlashCheck(void);
extern U8 IoCheck(void);
extern U8  SelfCheckFlag;
extern U8 ChipID[8];
//ostimer_t    *g_CheckACDCTimer; //?¡§?2aAC or DC
extern void Check_ACDC_timer_init();
extern void Check_ACDC_timer_start();
extern U16  PhaseACONT; 
extern U16  PhaseBCONT; 
extern U16  PhaseCCONT; 


#if defined(STATIC_MASTER)
extern void CheckJZQMacAddr(void);
extern void UseParameterCFG(void);
extern void UseFunctionSwitchDef(void);
#endif

#if defined(ZC3750STA)
//extern void LedCtrl(U8 STAState , U8 Txstate , U8 Rxstate );

//extern U8 gl_led_state;
extern U8 CheckLedCtrlData(U8 *databuf,U8 buflen,U8 *revbuf,U8 *revlen);

#endif
#if defined(ZC3750CJQ2)

#endif

typedef struct
{

    U16		NewOffset;    //new NTB station
    U32     record_time;  //<<<
    U32		NTB[MAXNTBCOUNT]; //10-20s 
	
} __PACKED	ZERODATA_t;

ZERODATA_t ZeroData[PHASENUM];


#if defined(STATIC_NODE)

U8  IO_check_timer_Init();


#endif

#if defined(ZC3750STA) 
void Check_MeterType_timer_start();
#endif
void meter_serial_cfg(uint32_t baudrate, uint8_t verif);

U8	CreatZeroDifflist(U8 Num, U8 *StartSeq, U8 *pList,U32 firstNTB,U8 offest);
U8	CreatZeroDataDifflist(U8 Num, U8 *pList,U8 Phase);
U32 GetDataFormZeroDifflist(U8 SeqNum, U8 *pList);
S16 GetZeroDataFormZeroDifflist(U8 SeqNum, U8 *pList);



U8 CalculateNTBDiff(U32 ntb ,U32 *staNtb,U8 num,U8 *Phase,U16 *zeroData);
U8	CalculateNTBcurve(S16 *mycurve,S16 *CCOcurve, U16 perNum);




__isr__	void ZeroCrossISR (phy_evt_param_t *param, void *arg);
__isr__	void ZeroCrossISRA (phy_evt_param_t *param, void *arg);


void ReadManageID();

U8 read_ecc_sm2_info(INT8U *pBuf,ECC_SM2_KEY_t type);
U8 write_ecc_sm2_info(INT8U *pBuf,ECC_SM2_KEY_t type,U8 validlen);




#endif




