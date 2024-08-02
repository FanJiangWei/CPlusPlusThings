#ifndef __APP_AREA_INDENTIFICATION_STA_H__
#define __APP_AREA_INDENTIFICATION_STA_H__

#include "app_area_indentification_common.h"


#if defined(STATIC_NODE)

extern U16 sta_area_ind_recv_packet_seq;

extern INDENTIFICATION_RESULT_DATA_t sta_area_ind_dis_result_t[3];

void sta_area_ind_packet_voltage_freq_698_frame(U8 *addr , U8 *buf,U16 *len);

uint8_t sta_area_ind_voltage_or_frequncy_match(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq);

void sta_area_ind_voltage_or_frequncy_rcv_deal(void *plistinfo, uint8_t *rcvbuf,uint16_t rcvlen,uint8_t protocoltype, uint16_t frameSeq);

void sta_area_ind_deal_feature_info_gather(INDENTIFICATION_IND_t *pIndentificationInd);

void sta_area_ind_get_start_ntb_from_zero_data(U16 total_num, U32 CCOstartntb,U8 collectseq);

void DistinguishResultInfo(INDENTIFICATION_RESULT_DATA_t *IndentificationResultData_t,
								U8   Directbit,
								U8   Startbit,
								U8   *Srcaddr, //发送方 地址
								U8	 *DesMacaddr, //接收方 地址
								U8   Phase,    
								U8   Featuretypes);


#endif

#endif

