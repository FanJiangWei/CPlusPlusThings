#ifndef __APP_METER_VERIFY_STA_H__
#define __APP_METER_VERIFY_STA_H__


#if defined(ZC3750STA)

//↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓宏定义↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓


#define BIND_TIME 			(BAUD_ARR*10)		//绑表轮次
#define ROUND_INT_TIME 		20				//绑表超时后，间隔轮次后继续绑表
#define BIND_ROUND_TIME 	(2*TIMER_UNITS)	//绑表周期定时器时长


//↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓枚举↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓

//绑表状态
typedef enum
{
    MeterCheckBaud_e,       //绑表
    MeterCheck698_e,        //698验证
    MeterCheck20Ed_e,       //20表波特率协商
    MeterCheck645_e,        //双协议表中645验证
} METER_COMU_TYPE_e;


//↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓结构体↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓


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
	void   (*Timeout)(void *pMsgNode);					/*接收超时*/
	void   (*RevDel)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);/*接收到数据处理*/
}__PACKED METER_COMU_INFO_t;


typedef struct
{
    U32 BaudRate;					//模块绑表波特率
    U8  MeterComuState;				//当前绑表状态，绑表、645->698验表、20版波特率协商、698->645验表
    U8  PrtclOop20      : 1;		//20版电表波特率协商成功标志
    U8  Has20Event		: 1;		//串口接收串口主动事件上报标志
    U8  Res1			: 6;		//
}__PACKED DEVICE_METER_t;


//外部调用
extern U8 high_baud;
extern DEVICE_METER_t sta_bind_info_t;

U8 sta_bind_packet_send_info(U8 arg_index , U8 *arg_send_buf , U8 *arg_send_buf_len);

void sta_bind_init(void);

void sta_bind_consult20_start();

void sta_bind_check_meter(U8* Addr, U8 Protocol, U32 BaudRate, U8 state);

void sta_meter_prtcltype_init();

#endif

#endif
