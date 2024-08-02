#ifndef WL_CNST_H
#define WL_CNST_H

#include "phy.h"
#include "vsh.h"
#include "framectrl.h"
#include "mbuf.h"

#if defined(STD_DUAL)
uint8_t hrf_can_not_send_message();
phy_frame_info_t *packet_sack_wl(rf_sof_ctrl_t *head,uint8_t result,int8_t snr);
mbuf_hdr_t * packet_sack_ctl_rf(uint8_t ext_dt,uint8_t *addr ,uint16_t stei,uint32_t snid);

mbuf_hdr_t *packet_beacon_wl(uint32_t stime,uint8_t *payload,uint16_t len,uint32_t bpc);
mbuf_hdr_t * packet_sof_wl(uint8_t priority,uint16_t dtei,uint8_t *payload,uint16_t len,uint8_t retrans ,uint8_t tf,plc_work_t *cnfrm);
mbuf_hdr_t * packet_sof_wl_forplc(void *rfHead,uint8_t *payload,uint16_t len, uint8_t phr_mcs, uint8_t pld_mcs, uint8_t pbsize);
#if defined(STATIC_MASTER)
mbuf_hdr_t * packet_coord_wl();
#endif

#endif//#if defined(STD_DUAL)

#endif 
