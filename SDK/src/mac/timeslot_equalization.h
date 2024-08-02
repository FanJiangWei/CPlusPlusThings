#ifndef __TIMESLOT_EQUALIZATION_H__
#define __TIMESLOT_EQUALIZATION_H__
#include "types.h"
#include "mbuf.h"

typedef enum {
    e_SLOT_TYPE_HPLC,
    e_SLOT_TYPE_RF
}SLOT_TYPE_e;


typedef struct
{	
	uint8_t   phase;
	uint32_t  start_timer;
	uint32_t  end_timer;
}__PACKED SCALE_CSMA_t;

#define MAX_SLOT_NUM 500 //ʹ����Ҫ�޸Ĳ���Ϊ 500
typedef struct
{	
	uint32_t   phase     :4;
	uint32_t   slot      :14;
	uint32_t   slot_num :14;
	
	uint16_t   slot_cnt[0];	
}SLOT_t;

typedef struct
{
	uint8_t type :4;//central = 1\pco = 2\sta = 3
	uint8_t phase :4;

	//uint16_t  stei;
}__PACKED beacon_slot_t;


typedef struct
{	
	uint32_t beacon_start_time;			//�ű�������ʼʱ��
	//uint32_t tdma_start_time;
	//uint32_t csma_start_time;
	//uint32_t bcsma_start_time;
	
	uint16_t beacon_slot;                  //ʱ϶Ƭ����
	uint16_t beacon_slot_nr;				//ʱ϶Ƭ����
	
	beacon_slot_t  beacon_slot_info[1000]; //ÿ��ʱ϶��Ϣ
	
	//uint16_t tdma_duration;
	
	uint16_t csma_duration_a;				//a��ʱ϶ʱ��
	uint16_t csma_duration_b;				//b��ʱ϶ʱ��
	uint16_t csma_duration_c;				//c��ʱ϶ʱ��
	uint16_t csma_slot;					//ʱ϶Ƭ����

	uint16_t bcsma_duration_a;            //a��ʱ϶ʱ��
	uint16_t bcsma_duration_b;			//b��ʱ϶ʱ��
	uint16_t bcsma_duration_c;			//c��ʱ϶ʱ��
	uint16_t bcsma_slot;					//ʱ϶Ƭ����
	
	uint32_t bcsma_lid       :8;					//��·��ʶ��
	uint32_t beacon_sent_nr :16;					
	uint32_t ext_bpts        :1;
    uint32_t RfBeaconType    :3;       //�����ű�����
    uint32_t RfBeaconSlotCnt :4;       //�����ű�ռ���ű�ʱ϶����

    uint32_t CCObcn_slot_nr  :8;       //�����ű�ʱ϶����
    uint32_t                 :24;
	
	mbuf_hdr_t	  *beacon_curr;
}BEACON_PERIOD_t;


void register_beacon_slot(BEACON_PERIOD_t *beacon_info);

void phy_beacon_show(list_head_t *link, tty_t *term);






















#endif 
