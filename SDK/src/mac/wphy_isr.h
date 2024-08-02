#ifndef WPHY_ISR_H
#define WPHY_ISR_H
#include "phy.h"
#include "framectrl.h"
#if defined(STD_DUAL)

static __inline__ uint8_t get_rf_frame_dt(rf_frame_ctrl_t * fc)
{
	return fc->dt;
}

static __inline__ uint32_t get_rf_sof_bcsti(rf_sof_ctrl_t *sof)
{
	return sof->bcsti;
}
static __inline__ uint8_t expected_rf_sof(rf_sof_ctrl_t *fc , uint16_t tei , uint32_t nid)
{
	return fc->dtei == tei && fc->snid == nid && (nid != 0 || fc->stei == 0xffe || fc->stei == 0xffd);
}

static __inline__ uint8_t expected_rf_sack(rf_sack_ctrl_t *fc , uint16_t stei , uint16_t dtei , uint32_t nid)
{
	return fc->dtei == stei && fc->stei == dtei && fc->snid == nid && fc->result == 0;
}


static __inline__ void set_rf_sof_repeat(rf_sof_ctrl_t * fc)
{
	fc->repeat = 1;
}
static __inline__ uint16_t get_rf_sof_stei(rf_sof_ctrl_t * fc)
{
	return fc->stei;
}
static __inline__ uint16_t get_rf_sof_dtei(rf_sof_ctrl_t * fc)
{
	return fc->dtei;
}
static __inline__ uint32_t get_rf_sof_nid(rf_sof_ctrl_t * fc)
{
	return fc->snid;
}
static __inline__ uint16_t get_rf_frame_stei(rf_frame_ctrl_t *fc)
{
    return (fc->dt == DT_BEACON) ? ((rf_beacon_ctrl_t *)fc)->stei :
        (fc->dt == DT_SOF) ? ((rf_sof_ctrl_t *)fc)->stei :
        (fc->dt == DT_SACK) ? ((rf_sack_ctrl_t *)fc)->stei : -1;
}

static __inline__ uint16_t get_rf_frame_dtei(rf_frame_ctrl_t *fc)
{
    return (fc->dt == DT_SOF) ? ((rf_sof_ctrl_t *)fc)->dtei :
        (fc->dt == DT_SACK) ? ((rf_sack_ctrl_t *)fc)->dtei : -1;
}

// __LOCALTEXT __isr__ void wphy_hdr_isr_rx(phy_frame_info_t *fi, void *context);

int32_t wphy_isr_init(void);
#endif

#endif // WPHY_ISR_H
