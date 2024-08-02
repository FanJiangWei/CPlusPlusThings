#ifndef __SERIAL_CACHE_QUEUE_H__
#define __SERIAL_CACHE_QUEUE_H__
#include "list.h"
#include "types.h"
#include "trios.h"
#include "app_read_sta.h"




#if defined(ROLAND1_1M) || defined(UNICORN2M)
#define SERICAL_PER  50 //one=1ms
#else
#define SERICAL_PER  10 //one=1ms
#endif

#define GUARD_TIME  (10*1000) //one=1ms guard serial_cache_timer 6S

//�����������ȼ� Խ�����ȼ�Խ��
#if defined(STATIC_NODE)
typedef enum
{
    e_Serial_AA         = 0x0F,  //δ���ɹ�ʱ�����֡
    e_Serial_PowerEvent = 0x0E,  //ͣ��
    e_Serial_Trans      = 0x0D,  //�ز�͸������
    e_Serial_Rf         = 0x0C,  //��������
    e_GetClock			= 0x0B,  //��ȡ���ʱ��
    e_GetMeterType		= 0x0A,  //��ȡ�������
    e_Serial_Broadcast  = 0x09,  //�㲥������
    e_Serial_Event      = 0x08,  //���ڳ����¼�
    e_Serial_Freeze     = 0x07,    //��������
    e_Serial_Quarter    = 0x06,   //96��  ����
    e_Serial_PerMin     = 0x05,    //���Ӽ�����
} SERIAL_LID_e;
#endif

#if defined(STATIC_MASTER)

#define SERIAL_IDE_NUM  21				//��󲢷�����20+�㳭

typedef enum
{
    e_Serial_AA             = 0x0F,     //��ʼ��
    e_Serial_AFN13F1        = 0x0E,     //�㳭+͸��
    e_Serial_AFN00			= 0x0D,     //ȷ��
    e_Serial_AFNF1F1     	= 0x0C, 	//����
	e_Serial_AFN10XX		= 0x0B,     //��ѯ��AFN03/AFN10
    e_Serial_AFN06XX        = 0x0A,    	//�ϱ���AFN06
    e_Serial_OTHER          = 0x09,   	//����
} SERIAL_LID_e;
#endif

typedef enum
{
	UNACTIVE=1,			//����δ����
    WATIING,			//���������
	SENDOK,				//����ȴ�ACK
}serial_state_t;

typedef struct serial_cache_header_s {
	list_head_t link;

	uint16_t nr;
	uint16_t thr;  
}serial_cache_header_t;

typedef struct add_serialcache_msg_s {
	list_head_t link;
	
	uint8_t	    state;						    /*��ǰ״̬*/	
	uint8_t     ack;						    /*����ID��*/
	uint8_t     lid ;						    /* ���ȼ�,���ʱ���ݴ�ֵ��������*/
	uint8_t     cnt;						

    STA_READMETER_INFO_t     StaReadMeterInfo;  /*�ز�READMETER��Ϣ*/	
	
	uint16_t    DeviceTimeout;			        /* �豸��ʱʱ��,10MS��λ*/
    uint8_t     IntervalTime;                   /* ���ڼ��ʱ��,10MS��λ*/
    uint8_t     Protocoltype;                   /* Э������*/
	
	uint16_t    FrameSeq;                       /* ����֡���*/
    uint16_t    FrameLen;					    /* ���ĳ��� */
    //uint8_t    MatchInfo[8];                  /* ������ƥ����Ϣ */
    //uint8_t    MatchLen;

	uint32_t    baud;					        /*0��ʵ�ʲ�����*/
    
	uint8_t     verif	:2;					    /*0:��У�飬1��żУ�飬2����У��*/
    uint8_t     Res    :6;                      /* ���� */
    uint8_t     ReTransmitCnt;                  /* �ط�����*/
    uint8_t     Res1[2];                         /* ���� */

	//���ڲ���
	void   (*Uartcfg)(uint32_t baud, uint8_t verif);	/*���ڲ�������*/
	void   (*EntryFail)(void);                      /*���ʧ��*/
	void   (*SendEvent)(void);					    /*�������*/
	void   (*ReSendEvent)(void);					/*�ط������*/
	void   (*MsgErr)(void);					        /*��������ʱ��Ϣ�쳣*/
	uint8_t(*Match)(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);
	void   (*Timeout)(void *pMsgNode);				/*���ճ�ʱ*/
	void   (*RevDel)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);/*���յ����ݴ���*/

	serial_cache_header_t	 *pSerialheader;	    /*����ĿҪ���ص�����ͷ*/
	ostimer_t 				  *CRT_timer;		    /*״̬���ƶ�ʱ��*/
	uint8_t   FrameUnit[0];				            /* �������� */
} add_serialcache_msg_t;

typedef struct recive_msg_s{
	uint8_t   Protocoltype;                    /* ???��?��??*/
	uint8_t   FrameSeq;                    /* ???????��??*/		
	uint16_t  FrameLen;					/* ����???��?? */

    uint8_t   MatchInfo[8];
    
    uint8_t   MatchLen;
    
	uint8_t   Direction;			/*???��????0???��?? ??1??????*/
    uint8_t   MsgType;              //��Ϣ����
    uint8_t   Prmbit;
	
	void   (*process)(uint8_t *revbuf,uint16_t len);

	serial_cache_header_t	 *pSerialheader;	/*??����??*/
	uint8_t   FrameUnit[0];				/* ����?????? */
}recive_msg_t;

extern U8	SericalTransState;
extern serial_cache_header_t SERIAL_CACHE_LIST;


void serial_cache_add_list(add_serialcache_msg_t msg,uint8_t *payload,uint8_t post);
void serial_cache_del_list(recive_msg_t msg,uint8_t *payload);
int8_t serial_cache_timer_init(void);
uint8_t serial_cache_list_ide_num();

#endif 

