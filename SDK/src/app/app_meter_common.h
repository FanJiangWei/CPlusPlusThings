#ifndef __APP_METER_COMMON_H__
#define __APP_METER_COMMON_H__

#include "types.h"

#if defined(STATIC_NODE)

// //STA和CJQ绑表搜表流程
// typedef enum
// {

// #if defined(ZC3750STA)
// 	PB_079600,
// 	PB_6989600,
// 	PB_072400,
// 	PB_69819200,
// 	PB_6982400,
// 	PB_07115200,
// 	PB_698115200,
// 	PB_69838400,
// 	PB_69857600,
// 	PB_698230400,
// 	PB_698460800,
// 	PB_6984800,
// 	PB_074800,
// 	PB_071200,
// #endif
// #if defined(ZC3750CJQ2)
// 	PB_072400,
// 	PB_079600,
// 	PB_6989600,
// 	PB_69819200,
// 	PB_07115200,
// 	PB_698115200,
// 	PB_6982400,
// 	PB_971200,
// 	PB_972400,
// 	PB_6984800,
// 	PB_074800,
// 	PB_071200,
// 	PB_974800,
// 	PB_979600,
// #endif
// 	PB_END,
// } PROTOCOL_BAUD_E;

#if defined(ZHE_JIANG)
#if defined(ZC3750STA)
#define BAUD_ARR 14
#else
#define BAUD_ARR 2
#endif
#else
#define BAUD_ARR 14
#endif
//波特率枚举
typedef enum
{
	METER_BAUD_RES,
	METER_BAUD_1200,
	METER_BAUD_2400,
	METER_BAUD_4800,
	METER_BAUD_9600,
	METER_BAUD_19200,
	METER_BAUD_115200,
	METER_BAUD_460800,
	METER_BAUD_38400,
	METER_BAUD_57600,
	METER_BAUD_230400,
	METER_BAUD_END,
}METER_BAUD_E;

//协议枚举
typedef enum
{
	METER_PROTOCOL_RES,
	METER_PROTOCOL_97,
	METER_PROTOCOL_07,
	METER_PROTOCOL_698,
	METER_PROTOCOL_T188,
	METER_PROTOCOL_END,
}METER_PROTOCOL_E;


typedef struct
{
	U8 Protocol;
    U8 Baud;
	U16 Res;
} __PACKED PROT_BAUD_t;

extern PROT_BAUD_t ProtBaud[BAUD_ARR];
extern PROT_BAUD_t ProtBaud_shanghai[BAUD_ARR];

#if defined(ZC3750STA)

extern U8 sta_bind_frame_97[14];

extern U8 sta_bind_frame_07[12];

extern U8 sta_bind_frame_698[25];

#else

extern U8 cjq2_search_item_97[2];

extern U8 cjq2_search_item_07[4];

extern U8 cjq2_search_item_698[8];

#endif

U32 baud_enum_to_int(U8 arg_baud_e);


#endif

void Dlt645Encode(U8* MeterAddr, U8 Protocol,U8 *ReadMeter,U8 *ReadMeterLen);

void Dlt698Encode(U8* MeterAddr, U8 Protocol,U8 *ReadMeter,U16 *ReadMeterLen);

void Modify_ChangeBaudtimer(U32 *BaudRated);

U8 GetAddrByProtocol(U8 protocol ,U8 *data , U16 datalen ,U8 *addr);

void cco_serial_baud_init(void);


#endif


