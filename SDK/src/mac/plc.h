#ifndef __PLC_H__
#define __PLC_H__


#include "list.h"
#include "mbuf.h"
#include "framectrl.h"
//#include "phy_sal.h"
#include "mac_common.h"

#define MEM_PMA (1)//统计内存池使用最大数量

#define DIR_TX 1
#define DIR_RX 2

extern	list_head_t BEACON_FRAME_LIST;
extern  list_head_t BEACON_LIST;

typedef void (*PLCDEALFUN_t)(mbuf_t *buf,void *pld,uint16_t len, uint8_t crc32,work_t *parm);
typedef void (*plc_post_func)(plc_work_t *work);

uint8_t (*pSendBeaconCnt)(void);

extern PLCDEALFUN_t pDealBeaconFunc;
extern PLCDEALFUN_t pDealSofFunc;
extern PLCDEALFUN_t pDealCoordFunc;
extern PLCDEALFUN_t pDealSackFunc;
extern sync_func sync_func_deal;
extern plc_post_func pPlcPostMsg;

void register_plc_deal_Func(uint8_t type, PLCDEALFUN_t func);

typedef void (*check_lowpower_states_fn)(void);
extern check_lowpower_states_fn  check_lowpower_states_hook;

typedef struct pbh_cmb_s{
	uint8_t  ssn	:6; /* segment sequence number */
	uint8_t  mfsf	:1; /* Mac frame start block flag */
	uint8_t  mfef	:1; /* Mac frame end block flag */

	uint8_t  xcmb   :1;
	uint8_t  copy   :1;
	uint8_t  pbsz   :6;	    /* PB size, 0: 72; 1: 136; 2: 264; 3: 520, (bytes) */
	
	uint16_t len;
	
	uint8_t data[0];
}pbh_cmb_t;

//错误代码
typedef enum
{
    e_SUCCESS = 0,
    e_NO_ACK  = 0xE1,
    e_ENQUEUE_FAIL,          //入队失败
    e_ROUTE_ERR,
    e_ROUTE_REPAIR,
    e_SOLT_ERR,
} CFM_ERRSTATE_e;


typedef enum
{
    e_link_mode_plc = 0x01,        //单载波工作模式
    e_link_mode_rf  = 0x02,        //单无线工作模式
    e_link_mode_dual= 0x03,        //双模工作模式
}LINK_SEND_MODE_e;
typedef struct
{
    U16 NextTei   :12;
    U16 LinkType  :4;
    // U16 res       :3;
}__PACKED BACK_ROUTE_t;
//数据请求确认
typedef struct
{
    U8     status;            //
    U8	   ReSendTimes; //重发次数
    U16    ScrTei;           //

    U16    NextTei;
    U16    DstTei;

    U32    MsduHandle;        //

    U8     MsduType;        //
    U8     RouterMode;
    U8	   ResendFlag;
    U8	   SendType;	//SEND_TYPE_e;

    U16    DelayTime;
    U8     SliceSeq;
    U8 	   BroadcastFlag :4;
    U8 	   MacVersion    :1;
    U8     sendlinkType  :3;

    BACK_ROUTE_t    BackRout[2];

    mbuf_hdr_t *MpduData;

    U8	   LinkId;
    U16    dataLen;
    U8     data[0];
} __PACKED MACDATA_CONFIRM_t;


extern pbh_cmb_t *pbh_cmb;

typedef struct bpts_s{
		list_head_t link;
		struct beacon_s *beacon;
		
		/* op - Hardware operation during a bpts.
		 * @return: -1,too late to recv/send or timeout or error happens,try to fetch next bpts
		 *           0,nothing to send try to fetch next bpts	
		 *           1,configure HW or recv or send or timeout successfully,remain in current bpts
		 */
		 __isr__ int32_t(*op)(struct bpts_s *); /*inital csma_send or beacon_send*/
		 
		 uint32_t start;        /* time slot start time */
		 uint32_t time;         /* next op time predicted within the time slot */
		 uint32_t end;          /* time slot end time */
		 
		 uint32_t type :4;      /*time slot type*/

		 uint32_t  dir    : 2 ;
		 uint32_t  phase  : 2 ;
		 uint32_t  lid    : 8 ;
		 uint32_t  async  : 1 ;
		 uint32_t  bc     : 4 ;
		 uint32_t  post   : 1 ;   
		 uint32_t  life   : 10 ;
		 
		 uint16_t  stei;
		 uint16_t  dtei;
         uint32_t  txflag :1; //记录TDMA 最后一个时隙序号
         uint32_t  rxflag :1; //记录CSMA 第一个时隙序号
         uint32_t         :30;
		 
		 list_head_t  pf_list;/*mbuf_hdr_t list*/
		
		 mbuf_hdr_t  *sent;
}bpts_t;





typedef struct STATICS_s{

		uint32_t nr_fatal_error;
		uint32_t nr_phy_reset_fail;
		uint32_t nr_late_csma_send;
		uint32_t nr_fail_csma_send;
		
		uint32_t nr_late[4];
		uint32_t nr_ts_late[4];
		uint32_t nr_fail[4];
		uint32_t nr_ts_frame[4];
		
}STATICS_t;

typedef struct MAC_PAMA_s{

		uint32_t zombie   :1;
		uint32_t res       :31;
		
		STATICS_t statis;


}MAC_PAMA_t;




typedef struct HPLC_s{

		phy_hdr_info_t hi;
		wphy_hdr_info_t whi;
#if MEM_PMA
        uint16_t size_a_min;
        uint16_t size_b_min;
        uint16_t size_c_min;
        uint16_t size_d_min;
        uint16_t size_e_min;
        uint16_t size_f_min;
        uint16_t size_g_min;
        uint16_t size_h_min;


        uint16_t size_a_max;
        uint16_t size_b_max;
        uint16_t size_c_max;
        uint16_t size_d_max;
        uint16_t size_e_max;
        uint16_t size_f_max;
        uint16_t size_g_max;
        uint16_t size_h_max;

        uint16_t a_num;
        uint16_t b_num;
        uint16_t c_num;
        uint16_t d_num;
        uint16_t e_num;
        uint16_t f_num;
        uint16_t g_num;
        uint16_t h_num;


        uint16_t a_max_num;
        uint16_t b_max_num;
        uint16_t c_max_num;
        uint16_t d_max_num;
        uint16_t e_max_num;
        uint16_t f_max_num;
        uint16_t g_max_num;
        uint16_t h_max_num;

#endif

#if defined STATIC_MASTER
		uint16_t    dur;
		uint16_t    bso;
		
		uint32_t    neighbour  :24;
		uint32_t                :8;

        uint32_t    bef   :1;
        uint32_t    beo   :31;
        uint32_t    next_beacon_start;

#endif		
		uint32_t   band_idx           :3;
		uint32_t   beacon_dur        :29;

		MAC_PAMA_t tx;
		MAC_PAMA_t rx;
        
        MAC_PAMA_t rf_tx;
        MAC_PAMA_t rf_rx;

		uint32_t sfo_not_first  :1;
		uint32_t phase           :2;
		uint32_t scanFlag       :1;
		uint32_t noise_proc     :4;
		
		uint32_t snid            :24;

        uint32_t sfotei			 :12;
        uint32_t sfo_is_done     :1;
        uint32_t sw_phase        :1;
        uint32_t testmode       :18;
		
		uint32_t tei			 :12;
        uint32_t plc_bpts_cnt       :10;//statis
        uint32_t rf_bpts_cnt       :10;//statis
		
		uint32_t sfo_bts;
		
        int8_t  ana;
        int8_t  dig;
        uint8_t worklinkmode :2;         //链路模式选择，bit0载波，bit1无线，0:无效，1:有效
        uint8_t hdr_mcs :3;
        uint8_t pld_mcs :3;

		#if defined(STD_DUAL)
        uint8_t option        ;
        uint32_t rfchannel     ;
		#endif
		
		phy_frame_info_t fi;
		
		uint8_t hdr[32];
		uint8_t pld[524*4] __CACHEALIGNED;
}__PACKED HPLC_t;
typedef struct phy_rxtx_cnt_s {
	uint32_t rx_cnt;
	uint32_t rx_err_cnt;
    
	uint32_t tx_data_cnt;
    uint32_t tx_data_full;
    uint32_t tx_mng_cnt;
    uint32_t tx_mng_full;
	
}__PACKED phy_rxtx_cnt_t;


HPLC_t HPLC;

#define PLC_PANIC()		sys_panic("[%s:%d]<PLC panic> Error!!!\r\n", __func__, __LINE__)


//typedef mbuf_hdr_t work_t;

typedef struct beacon_pld_s{

		uint32_t	  bpc;



}beacon_pld_t;

extern int32_t zc_phy_reset(void);
extern int32_t hplc_hrf_phy_reset();
__LOCALTEXT __isr__ void hplc_do_next(void);
__LOCALTEXT __isr__ int32_t beacon_send(bpts_t *bpts);

void hplc_push_beacon(beacon_t *beacon);

__LOCALTEXT __isr__ int32_t rbeacon_send(bpts_t *bpts);
int32_t tdma_recv(bpts_t *bpts);

//

void link_layer_deal(work_t *mbuf);
void put_bpts(bpts_t *curr_bpts);


static __inline__ uint8_t get_sack_ext_dt(sack_search_ctrl_t * sack)
{
	return sack->ext_dt;
}

void mac_pma_init(void);

#endif 
