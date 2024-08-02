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

//串口数据优先级 越大优先级越高
#if defined(STATIC_NODE)
typedef enum
{
    e_Serial_AA         = 0x0F,  //未绑表成功时，绑表帧
    e_Serial_PowerEvent = 0x0E,  //停电
    e_Serial_Trans      = 0x0D,  //载波透传数据
    e_Serial_Rf         = 0x0C,  //红外数据
    e_GetClock			= 0x0B,  //获取表计时钟
    e_GetMeterType		= 0x0A,  //获取表计类型
    e_Serial_Broadcast  = 0x09,  //广播类任务
    e_Serial_Event      = 0x08,  //周期抄读事件
    e_Serial_Freeze     = 0x07,    //冻结数据
    e_Serial_Quarter    = 0x06,   //96点  数据
    e_Serial_PerMin     = 0x05,    //分钟级数据
} SERIAL_LID_e;
#endif

#if defined(STATIC_MASTER)

#define SERIAL_IDE_NUM  21				//最大并发条数20+点抄

typedef enum
{
    e_Serial_AA             = 0x0F,     //初始化
    e_Serial_AFN13F1        = 0x0E,     //点抄+透传
    e_Serial_AFN00			= 0x0D,     //确认
    e_Serial_AFNF1F1     	= 0x0C, 	//并发
	e_Serial_AFN10XX		= 0x0B,     //查询类AFN03/AFN10
    e_Serial_AFN06XX        = 0x0A,    	//上报类AFN06
    e_Serial_OTHER          = 0x09,   	//其他
} SERIAL_LID_e;
#endif

typedef enum
{
	UNACTIVE=1,			//任务未启动
    WATIING,			//任务发送完成
	SENDOK,				//任务等待ACK
}serial_state_t;

typedef struct serial_cache_header_s {
	list_head_t link;

	uint16_t nr;
	uint16_t thr;  
}serial_cache_header_t;

typedef struct add_serialcache_msg_s {
	list_head_t link;
	
	uint8_t	    state;						    /*当前状态*/	
	uint8_t     ack;						    /*任务ID号*/
	uint8_t     lid ;						    /* 优先级,入队时根据此值进行排列*/
	uint8_t     cnt;						

    STA_READMETER_INFO_t     StaReadMeterInfo;  /*载波READMETER信息*/	
	
	uint16_t    DeviceTimeout;			        /* 设备超时时间,10MS单位*/
    uint8_t     IntervalTime;                   /* 串口间隔时间,10MS单位*/
    uint8_t     Protocoltype;                   /* 协议类型*/
	
	uint16_t    FrameSeq;                       /* 串口帧序号*/
    uint16_t    FrameLen;					    /* 报文长度 */
    //uint8_t    MatchInfo[8];                  /* 上下行匹配信息 */
    //uint8_t    MatchLen;

	uint32_t    baud;					        /*0：实际波特率*/
    
	uint8_t     verif	:2;					    /*0:无校验，1：偶校验，2：奇校验*/
    uint8_t     Res    :6;                      /* 保留 */
    uint8_t     ReTransmitCnt;                  /* 重发次数*/
    uint8_t     Res1[2];                         /* 保留 */

	//串口参数
	void   (*Uartcfg)(uint32_t baud, uint8_t verif);	/*串口参数设置*/
	void   (*EntryFail)(void);                      /*入队失败*/
	void   (*SendEvent)(void);					    /*发送完成*/
	void   (*ReSendEvent)(void);					/*重发送完成*/
	void   (*MsgErr)(void);					        /*处理链表时消息异常*/
	uint8_t(*Match)(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);
	void   (*Timeout)(void *pMsgNode);				/*接收超时*/
	void   (*RevDel)(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);/*接收到数据处理*/

	serial_cache_header_t	 *pSerialheader;	    /*此条目要挂载的链表头*/
	ostimer_t 				  *CRT_timer;		    /*状态控制定时器*/
	uint8_t   FrameUnit[0];				            /* 报文内容 */
} add_serialcache_msg_t;

typedef struct recive_msg_s{
	uint8_t   Protocoltype;                    /* ???é?à??*/
	uint8_t   FrameSeq;                    /* ???????ò??*/		
	uint16_t  FrameLen;					/* ±¨???¤?? */

    uint8_t   MatchInfo[8];
    
    uint8_t   MatchLen;
    
	uint8_t   Direction;			/*???ò????0???÷?? ??1??????*/
    uint8_t   MsgType;              //消息类型
    uint8_t   Prmbit;
	
	void   (*process)(uint8_t *revbuf,uint16_t len);

	serial_cache_header_t	 *pSerialheader;	/*??±í??*/
	uint8_t   FrameUnit[0];				/* ±¨?????? */
}recive_msg_t;

extern U8	SericalTransState;
extern serial_cache_header_t SERIAL_CACHE_LIST;


void serial_cache_add_list(add_serialcache_msg_t msg,uint8_t *payload,uint8_t post);
void serial_cache_del_list(recive_msg_t msg,uint8_t *payload);
int8_t serial_cache_timer_init(void);
uint8_t serial_cache_list_ide_num();

#endif 

