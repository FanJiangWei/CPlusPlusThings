#ifndef __APP_METER_VERIFY_STA_H__
#define __APP_METER_VERIFY_STA_H__


#if defined(ZC3750STA)

//�����������������������������������������������궨���������������������������������������������


#define BIND_TIME 			(BAUD_ARR*10)		//����ִ�
#define ROUND_INT_TIME 		20				//���ʱ�󣬼���ִκ�������
#define BIND_ROUND_TIME 	(2*TIMER_UNITS)	//������ڶ�ʱ��ʱ��


//����������������������������������������������ö�١�������������������������������������������

//���״̬
typedef enum
{
    MeterCheckBaud_e,       //���
    MeterCheck698_e,        //698��֤
    MeterCheck20Ed_e,       //20������Э��
    MeterCheck645_e,        //˫Э�����645��֤
} METER_COMU_TYPE_e;


//�����������������������������������������������ṹ���������������������������������������������


typedef struct
{
    char		*name;
    
    U8          protocol;
    U8          frameSeq;
    U16         timeout;

    U32         baudrate;
    
    U16         frameLen;
    U8          comustate;
    U8          res[1];
    
    U8          *pPayload;
    uint8_t(*Match)(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);
	void   (*Timeout)(void *pMsgNode);					/*���ճ�ʱ*/
	void   (*RevDel)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);/*���յ����ݴ���*/
}__PACKED METER_COMU_INFO_t;


typedef struct
{
    U32 BaudRate;					//ģ��������
    U8  MeterComuState;				//��ǰ���״̬�����645->698���20�沨����Э�̡�698->645���
    U8  PrtclOop20      : 1;		//20��������Э�̳ɹ���־
    U8  Has20Event		: 1;		//���ڽ��մ��������¼��ϱ���־
    U8  Res1			: 6;		//
}__PACKED DEVICE_METER_t;


//�ⲿ����
extern U8 high_baud;
extern DEVICE_METER_t sta_bind_info_t;

U8 sta_bind_packet_send_info(U8 arg_index , U8 *arg_send_buf , U8 *arg_send_buf_len);

void sta_bind_init(void);

void sta_bind_consult20_start();

void sta_bind_check_meter(U8* Addr, U8 Protocol, U32 BaudRate, U8 state);

void sta_meter_prtcltype_init();

#endif

#endif
