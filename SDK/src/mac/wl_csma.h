#ifndef WL_CSMA_H
#define WL_CSMA_H

#include "mbuf.h"
#include "plc.h"
#include "vsh.h"
#include "framectrl.h"
#include "mac_common.h"

#if defined(STD_DUAL)


/**************************************************************
 * 为无线部分增加接口
 * 
 * 
 *************************************************************/
__LOCALTEXT __isr__ void wl_csma_sig_tmot(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void wl_csma_rx_end(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void wl_tdma_tx_end(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void wl_swc_rx_frame(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void wl_csma_tx_end(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void wl_csma_rx_hd(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ int32_t wl_csma_send(bpts_t *bpts);
__LOCALTEXT __isr__ uint8_t check_Rf_pb_crc(phy_frame_info_t *pf, uint8_t *crc32, uint8_t *pld , uint16_t *len);
__LOCALTEXT __isr__ int32_t plc_to_rf_testmode_deal(uint8_t result, frame_ctrl_t *head, uint8_t *pld, uint16_t pld_len);

#endif //#if defined(STD_DUAL)


#endif

