#ifndef APP_RDCTRL_H
#define APP_RDCTRL_H
#include "ZCsystem.h"
#include "app.h"
#include "aps_interface.h"

/***************************抄控器扩展使用****************************/
#define RD_SACK_PEND mutex_pend(&Mutex_RdCtrl_Search, 12)
#define RD_SACK_POST mutex_post(&Mutex_RdCtrl_Search)
extern U8 rdaddr[6];
extern U8 g_ReadIdType;
extern U8 g_RdReadIdTimes;
extern U16 g_RdApsIdSeq;
extern ostimer_t *g_RdReadIdTimer;

typedef struct
{
	U16 dtei : 12;

	U16 linktype : 2;
	U16 mac_use : 1;
	U16 recsyflag : 1; //接收到同步帧标志位;
	U16 recupflag : 1; //接收到载波上行标志位；
	U16 readseq : 8;   //接收到载波上行标志位；
	U16 res : 7;	   //接收到载波上行标志位；
	U8 mac_addr[6];
	U8 send_sack_cnt;
	U8 period_sendsyh_flag;
} RdCtrlLink_t;
extern RdCtrlLink_t rd_ctrl_info;

typedef struct
{

	U8 frame645len;
	U8 ctrlCode;
	U8 data[4];
	U8 addr[6];
	U8 datalen;
	U8 Pro;

	U8 frame645[0];
} __PACKED RdCtrl645Info_t;

typedef struct
{
	U8 ControlData;
	U8 DataArea[4];
	void (*Func)(RdCtrl645Info_t *Frame645info);
	char *name;
} Rd645Protocal_t;

extern ostimer_t *g_Sendsyhtimer;
void modify_send_syh_timer(U32 TimeMs);
extern ostimer_t *g_SendsyhtimeoutTimer;
void modify_send_syh_timeout_timer(U32 TimeMs);

void proc_rd_ctrl_indication(RD_CTRL_REQ_t *pRdCtrlData);

void proc_rd_ctrl_send_data(U8 *data, U16 len);
void proc_rd_ctrl_to_uart_indication(RD_CTRLT_TO_UART_REQ_t *p_rdctrl_to_uart_data);
// void proc_rd_ctrl_to_uart_send_data(U8 *data, U16 len);
void rd_ctrl_pro_dlt645_ex(U8 *pMsgData, U16 Msglength);
U8 is_rd_ctrl_tei(U16 tei);

#endif // APP_RDCTRL_H
