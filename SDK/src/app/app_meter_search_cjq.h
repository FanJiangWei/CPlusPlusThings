#ifndef __APP_METER_SEARCH_CJQ_H__
#define __APP_METER_SEARCH_CJQ_H__

#include "list.h"
#include "types.h"
#include "trios.h"



#if defined(ZC3750CJQ2)


U32 BaudNum;


#define CJQ2_SEARCH_METER_TIMEOUT           180 /* unit:10ms */
#define CJQ2_SEARCH_METER_INTERVAL_TIME     2000 /* unit:1ms */
#define CJQ2_SEARCH_METER_RTRSMT_CNT        0  /* �ѱ����ط�*/
#define MACADRDR_LENGTH                     6
#define METERNUM_MAX			            32
#define CJQ2_SEARCH_METER_STOP_TIME         (1*60*60*1000)


typedef struct cjq_dev_s {
	void (*init)(struct cjq_dev_s *dev);

	list_head_t	link;

	char		*name;		
	void (*showver)(void);
	void (*showinfo)(void);
} cjq_dev_t;

typedef struct cjq_search_func_s {
	list_head_t	link;

	char		*name;	
	uint8_t     (*match)(uint8_t sendprtcl, uint8_t rcvprtcl, uint8_t sendseq, uint8_t rcvseq); 
	void        (*tmot)(void *pMsgNode);
	void        (*rcv)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);
} cjq_search_func_t;
extern cjq_search_func_t cjq2_search_check_t;
extern cjq_search_func_t cjq2_search_check698_t;
extern cjq_search_func_t cjq2_search_aa_t;
extern cjq_search_func_t cjq2_search_traverse_t;

/*
typedef struct cjq2_search_meter_header_s{
	list_head_t link;
	
	uint16_t nr;
	uint16_t thr;
}cjq2_search_meter_header_t;

cjq2_search_meter_header_t CJQ_SEARCH_METER_LIST;

typedef struct{
	uint8_t  checkmeter  :2;
	uint8_t  AA_search   :2;
	uint8_t  traves      :2;
	uint8_t              :2;
}search_meter_state_t;
*/



typedef struct
{
	U8  SearchState;				//�ѱ�׶�: 0��� 1ȫAA�ѱ� 2�����ѱ� 3 ����07 ��698�� 
	U8  SearchResult;			//���
	U8  ProtocolBaud;			//�ѱ�ʱ��Լ�Ͳ�����
							    //0 07 2400
								//1 97 1200
								//2 07 9600 ���698����н���
								//3 07 1200
								//4 97 2400
								//5 07 4800
								//6 97 4800
								//7 97 9600
	U8  Protocol;
	U32 Baud;
	U8  CheckMeterIdx;			//��ǰ������
    U8  SearchTimeout;			//ȫAA�ѱ���� ���ֵ 3
    U8  SearchAddr[MACADRDR_LENGTH];			//Ĭ��Ϊ6��AA
    U8  SearchResultAddr[MACADRDR_LENGTH];
}SEARCH_CONTROL_ST;

typedef struct cjq2_search_meter_s{
	//list_head_t link;
	
	uint8_t  PortID        :2;
	uint8_t  priority      :6;
	uint8_t  baud          :4;  /*0:1200��    1��2400��2��4800��3��9600��4��115200��5��460800*/
	uint8_t  verif         :2;  /*0:��У�飬1��żУ�飬2����У��*/
	uint8_t  protocol      :2;  /*0:ȱʡ��      1:97��      2:07�� 3 6 98*/
	uint16_t  SearchTimeout;
	int8_t   ReTransmitCnt;
	
	uint8_t  (*match)(uint8_t sendprtcl, uint8_t rcvprtcl, uint8_t sendFrameSeq, uint8_t recvFrameSeq);
	void     (*tmot_func)(void *pMsgNode);
	void     (*rcv_func)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);
	U32     *pCjqSearchMeter;
    ostimer_t 	*Suspend_timer;		/*��ͣ�ѱ�ʱ��*/
    
	uint8_t  RunState;
	uint8_t  SearchState;
    SEARCH_CONTROL_ST  Ctrl;
	uint16_t FrameLen;
	uint8_t  FrameUnit[50];

	uint8_t CheckMeterWaitFlag; // �ȴ�����־λ
}cjq2_searh_meter_t;

extern cjq2_searh_meter_t cjq2_search_meter_info_t;

/*
enum{
	e_UNEXECUTE,
	e_EXECUTING,
}CJQ2_SEARCH_METER_LIST_STATE_e;
*/

extern ostimer_t *cjq2_search_meter_timer;

int8_t cjq2_search_meter_timer_init(void);

void cjq2_search_meter_ctrl(uint8_t *pPayload, uint8_t len, cjq_search_func_t  *func);

void cjq2_search_suspend_timer_modify(uint32_t Ms);

void cjq2_search_meter_timer_modify(uint32_t Ms, uint32_t opt);

#endif

#endif
