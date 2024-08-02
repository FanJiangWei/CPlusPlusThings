#ifndef __PLC_CNST_H__
#define __PLC_CNST_H__

#include "phy.h"
#include "vsh.h"
#include "framectrl.h"
#include "mbuf.h"

uint8_t plc_can_not_send_message();
phy_frame_info_t *packet_sack(sof_ctrl_t *head,uint8_t result,int8_t snr);
uint16_t packet_beacon_paload(uint8_t *payload, U8 phase, U8 SimpBeaconFlag);
mbuf_hdr_t *packet_beacon(uint32_t stime,uint8_t phase,uint8_t *payload,uint16_t len,uint32_t bpc);
mbuf_hdr_t * packet_sof(uint8_t priority,uint16_t dtei,uint8_t *payload,uint16_t len,uint8_t retrans ,uint8_t tf, uint8_t phase,plc_work_t *cnfrm);
mbuf_hdr_t * packet_sack_ctl(uint8_t ext_dt,uint8_t *addr ,uint16_t stei,uint32_t snid);
plc_work_t *put_next_beacon(void);
#if STATIC_MASTER
mbuf_hdr_t * packet_coord(uint16_t dur,uint16_t bso ,uint32_t neighbour,uint8_t bef,uint16_t beo);
#endif
void packet_beacon_period(work_t *work);


#if STATIC_NODE
void packet_csma_bpts(plc_work_t *mbuf_hdr);
#endif















#endif 
