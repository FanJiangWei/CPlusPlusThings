#ifndef _APP_DEVICEINFO_REPORT_H_
#define _APP_DEVICEINFO_REPORT_H_
#include "ZCsystem.h"
#include "app_gw3762.h"
#include "SchTask.h"
#include "app_698p45.h"
#include "meter.h"

#define JZQSETBAUD   115
#define SENDTIME06F4 1440
#define JS_BAUD115200 115200
#define JS_BAUD9600  9600
#define CCOBAUD_TIMEOUT 200
#define MAX_DATAITEM_COUNT 10

extern U8 Count;

typedef enum
{
    e_null = 0,
    e_need_report,
    e_has_report,
}REPORTSTATE;
	
#if defined(STATIC_MASTER)

typedef struct{
	U8 Result;
	U8 CurrentIndex;
	U8 Tasktype;
	U8 MacAddr[6];
}__PACKED QUERYREGANDCOLLECT;


typedef struct
{
    char		*name;
    U8          MeterComuState;
    U8          protocol;
    U16         timeout;
    U8          frameSeq;
    U16         frameLen;
    U8          *pPayload;
    U8          retryTimes;
    U8          active;
    U16         Res;
    uint8_t(*Match)(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);
	void   (*Timeout)(void *pMsgNode);					/*接收超时*/
	void   (*RevDel)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);/*接收到数据处理*/
    
} CCOCOMU_TEST_INFO_t;

//extern U32 CCOBaud;
//extern ostimer_t *SetBaudrateTimer;
//extern ostimer_t *CollectQueryBFcontroltimer;
//extern ostimer_t *wait_report_06f4_timer;
extern U8 ReadJZQclockSuccFlag;

extern void cco_alarm_clock_task(U16 CltPeriod);
void modify_wait_report_06f4_timer(uint32_t Sec);
U8 check_collect_state();
extern void save_collect_state(U8 *macaddr , U8 State);
extern void check_network_status();
#ifdef JIANG_SU
void NodeChange2app(U8 *addr,U8 addrlen);

extern void ProcOneStaNetworkStatusChangeIndication(work_t *work);
#endif
extern void (*pOneStaNetStatusChangeInd)(work_t *work); //一个节点在数据链路层的状态发生变化时，通知APP


#endif



#endif


