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

#define MAX_SLOT_NUM 500 //使用需要修改参数为 500
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
	uint32_t beacon_start_time;			//信标周期起始时间
	//uint32_t tdma_start_time;
	//uint32_t csma_start_time;
	//uint32_t bcsma_start_time;
	
	uint16_t beacon_slot;                  //时隙片长度
	uint16_t beacon_slot_nr;				//时隙片个数
	
	beacon_slot_t  beacon_slot_info[1000]; //每个时隙信息
	
	//uint16_t tdma_duration;
	
	uint16_t csma_duration_a;				//a相时隙时长
	uint16_t csma_duration_b;				//b相时隙时长
	uint16_t csma_duration_c;				//c相时隙时长
	uint16_t csma_slot;					//时隙片长度

	uint16_t bcsma_duration_a;            //a相时隙时长
	uint16_t bcsma_duration_b;			//b相时隙时长
	uint16_t bcsma_duration_c;			//c相时隙时长
	uint16_t bcsma_slot;					//时隙片长度
	
	uint32_t bcsma_lid       :8;					//链路标识符
	uint32_t beacon_sent_nr :16;					
	uint32_t ext_bpts        :1;
    uint32_t RfBeaconType    :3;       //无线信标类型
    uint32_t RfBeaconSlotCnt :4;       //无线信标占用信标时隙个数

    uint32_t CCObcn_slot_nr  :8;       //中央信标时隙数量
    uint32_t                 :24;
	
	mbuf_hdr_t	  *beacon_curr;
}BEACON_PERIOD_t;


void register_beacon_slot(BEACON_PERIOD_t *beacon_info);

void phy_beacon_show(list_head_t *link, tty_t *term);






















#endif 
