#ifndef __APP_CURRENT_ABNORMAL_EVENT_CCO_H__
#define __APP_CURRENT_ABNORMAL_EVENT_CCO_H__

#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "app.h"
#include "app_dltpro.h"
#include "app_event_report_cco.h"

#if defined(STATIC_MASTER)

typedef struct{
    U8       MacAddr[6];
	U8		 LiveLine[3];
	U8		 ZeroLine[3];
	U8		 Difference_Value[3];
	U8       Time_Clock[6];
}__PACKED LiveLineZeroLine_UP_Data_t;
typedef struct
{
	U16  	ProtocolVer                : 6;
    U16    	HeaderLen                  : 6;
	U16    Direction  					:1;   // 0�����з���ָCCO���͸�STA�ı��ģ�1�����з���ָSTA�ϱ�CCO�ı��ġ�
    U16    PrmFlag    					:1;   // 0�����ԴӶ�վ��1����������վ��
    U16    FunCode    					:2;   // ����
	U16    PacketSeq;       // �������
	U8    SrcMacAddr[MACADRDR_LENGTH];//ԴMAC��ַ
	U8 	  CommandType;//��������
	U8 	  EvenDataLen;  	//���ݳ���
	U8 	  EventData[0];    // ��������
}__PACKED Zero_Fire_Line_Line_Enabled_t;
extern LiveLineZeroLine_UP_Data_t LiveLineZeroLine_UP_Data[POWERNUM];
extern BitMapPowerInfo_t LiveLineZeroLineAddrList[POWERNUM];

extern U8  g_LiveLineZeroLineReportCnt;
extern U8 g_LiveLineZeroLineReportFlag;
#if defined(SHAAN_XI)
extern ostimer_t *liveline_zeroline_cco_event_send_timer;
extern ostimer_t *LiveLineZeroLine_clean_bitmap_timer;
#endif
//U8 LiveLineZeroLine_UP_Ok = 0;
U8 zero_live_line_event_cal(U8 FunCode, U8 *pEvent,U16  pEventLen);
U8 zero_live_line_map_trans(U8 *SrcBitMap , U16  SrcBitMapLen ,U16 StartIndex );
//void zero_live_line_cco_power_event_send(work_t *work);
void  aps_zero_fire_line_enabled_request(LiveLineZeroLineEnabled_t      *pEventReportReq);
void zero_fire_line_cco_clean_bitmap_timer_cb(struct ostimer_s *ostimer, void *arg);
void modify_zero_live_line_cco_power_event_send_timer(uint32_t first);
void zero_live_line_cco_power_event_send_timer_cb(struct ostimer_s *ostimer, void *arg);

#endif
#endif

