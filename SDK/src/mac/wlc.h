#ifndef WLC_H
#define WLC_H

#include "plc.h"
#include "mac_common.h"

#if defined(STD_DUAL)

extern list_head_t WL_BEACON_FRAME_LIST;

extern int32_t zc_wphy_reset(void);
extern int32_t hplc_hrf_wphy_reset();

__LOCALTEXT __isr__ void wl_do_next(void);
__LOCALTEXT __isr__ int32_t wl_beacon_send(bpts_t *bpts);
__LOCALTEXT __isr__ int32_t wl_rbeacon_send(bpts_t *bpts);
int32_t wl_tdma_recv(bpts_t *bpts);
void hplc_push_beacon_wl(beacon_t *beacon);
void wl_pma_init();
void wl_link_layer_deal(work_t *mbuf);
uint32_t getWphyBifsByPostFlag(bpts_t *bpts);
#endif //#if defined(STD_DUAL)

#endif
