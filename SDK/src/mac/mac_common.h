#ifndef MAC_COMMON_H
#define MAC_COMMON_H


#include "list.h"
#include "mbuf.h"



typedef void (*sync_func)(void);
extern sync_func sync_func_deal;

enum
{
	NORM=0,
	PHYTPT,
	PHYCB,
	MACTPT,
    WPHYTPT,
    WPHYCB,
    RF_PLCPHYCB,
    PLC_TO_RF_CB,
    SAFETEST,
	RD_CTRL,
    MONITOR,
    TEST_MODE, //作为测试模块，模拟台体发送测试报文
};

#define 	ENTERY_OK   0       //入队成功
#define 	LIST_FULL   1       //队列满
#define 	DATA_NULL   2       //数据空


#define  TS_BEACON   0
#define  TS_TDMA     1
#define  TS_CSMA     2
#define  TS_BCSMA    3


#define NR_TS_TYPE 4

/* beacon period time slot entry*/
typedef struct beacon_s{
		list_head_t   link;
		//mbuf_hdr_t	  *curr;            /*current beacon data*/
		//mbuf_hdr_t    *central;		 /*central beacon data to be sent for the current beacon period*/    
		plc_work_t    *next;            /*next beacon data*/
		list_head_t   *curr_pos;		/*solt pma bpts_t*/
		void(*put)(struct beacon_s *); /*del curr_beacon,release mem*/
		// void(*put)(struct beacon_s *, list_head_t *); /*del curr_beacon,release mem*/
		
		uint32_t      rx;
		uint32_t	  bts;
		uint32_t	  start;
		uint32_t	  length;
		uint32_t	  end;
		uint32_t	  bpc;
		
		uint16_t      phase   :2;
		uint16_t      bt      :3;
		uint16_t      me_ts   :1;
		uint16_t	  ext_bpts :1;
		uint16_t               :9;
		
		uint16_t      stei;
		uint8_t       networking;
		uint8_t       coord;
		
		uint32_t      nr_ts[NR_TS_TYPE];
		uint32_t      nr_bpts;
		list_head_t   bpts_list;    	/*solt pma bpts_t  list_header*/
}beacon_t;
extern list_head_t BEACON_LIST;
extern beacon_t *CURR_BEACON;

extern list_head_t WL_BEACON_LIST;
extern beacon_t *WL_CURR_BEACON;



beacon_t * hplc_pop_beacon(list_head_t *pBeaconList);
void hplc_drain_beacon(U8 CurFlag);

void wl_drain_beacon(U8 CurFlag);

/**
 * @brief 同时清空载波和无线时隙信息。
 * 
 */
void Clean_All_time_slot(U8 CurFlag);

typedef struct tx_link_s{
	list_head_t link;
	
	uint16_t nr;
	uint16_t thr;
}tx_link_t;


extern tx_link_t TX_MNG_LINK;
extern tx_link_t TX_DATA_LINK;
extern tx_link_t TX_BCSMA_LINK;

extern tx_link_t WL_TX_MNG_LINK;
extern tx_link_t WL_TX_DATA_LINK;
extern tx_link_t WL_TX_BCSMA_LINK;
typedef enum
{
   add_head=1,
   add_lid,
   del_lid,
   cln_list,
}msg_op_type_e;

void tx_link_clean(tx_link_t *tx_link);
int32_t tx_link_enq_head(list_head_t *link,tx_link_t *tx_list);
int32_t tx_link_enq_by_lid(mbuf_hdr_t *mbuf,tx_link_t *tx_list);
mbuf_hdr_t *tx_link_deq(tx_link_t *tx_link, U8 phase);
void phy_tx_link_init(tx_link_t *tx_list,uint16_t thr);
__LOCALTEXT __isr__ void bpts_pushback_pf(mbuf_hdr_t *mpdu);
__LOCALTEXT  uint32_t backoff_mask(uint8_t backoff,uint8_t priority);




typedef void (*maintain_nettick_func)(uint32_t SNID,uint32_t           Sendbts,uint32_t Localbts);
extern maintain_nettick_func maintain_net_tick;



typedef void (*plc_post_func)(plc_work_t *work);
extern plc_post_func pPlcPostMsg;
void register_plc_msg_func(plc_post_func func);
void post_plc_work(plc_work_t *work);
void register_sync_func(sync_func func);
void register_nettick_func(maintain_nettick_func func);

uint8_t check_bpts_op(void *opAddr);



#endif
