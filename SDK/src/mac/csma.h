/*
 * Copyright: (c) 2006-2010, 2009 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	csma.h
 * Purpose:	
 * History:
 *	Nov 19, 2019  py Create
 */

#ifndef _CSMA_H_
#define _CSMA_H_

#include "mbuf.h"
#include "plc.h"
#include "vsh.h"
#include "framectrl.h"
#include "mac_common.h"

#define BCOLT_STOP_EARLY 3

typedef uint8_t (* check_CB_FacTestMode)(uint8_t      *pData ,U16 DataLen);
extern check_CB_FacTestMode  check_CB_FacTestMode_hook;


__LOCALTEXT __isr__ void csma_pd_tmot(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void csma_rx_end(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void tdma_tx_end(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void swc_rx_frame(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void csma_tx_end(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ void csma_rx_hd(phy_evt_param_t *para, void *arg);
__LOCALTEXT __isr__ int32_t csma_send(bpts_t *bpts);
__LOCALTEXT __isr__ void hplc_do_next(void);
__LOCALTEXT __isr__ uint8_t check_pb_crc(phy_frame_info_t *pf, uint8_t *CRC32 , uint32_t *pld , uint16_t *pld_len, uint8_t *pkg);

void phy_msg_show(xsh_t *xsh,tx_link_t *tx_link);


static __inline__ uint8_t set_sof_pbc(sof_ctrl_t * fc,uint8_t pbc)
{
	return fc->pbc = pbc;
}
static __inline__ uint8_t get_frame_dt(frame_ctrl_t * fc)
{
	return fc->dt;
}
static __inline__ void set_sof_repeat(sof_ctrl_t * fc)
{
	fc->repeat = 1;
}
static __inline__ uint16_t get_sof_stei(sof_ctrl_t * fc)
{
	return fc->stei;
}
static __inline__ uint32_t get_sof_nid(sof_ctrl_t * fc)
{
	return fc->snid;
}
static __inline__ uint8_t expected_sack(sack_ctrl_t *fc , uint16_t stei , uint16_t dtei , uint32_t nid)
{
	#if defined(STD_2016) || defined(STD_DUAL)
	return fc->dtei == stei && fc->stei == dtei && fc->snid == nid && fc->result == 0;
	#elif defined(STD_SPG)
	return fc->dtei == stei && fc->snid == nid && fc->result == 0;
	#endif
}

static __inline__ uint8_t filter_invalid(frame_ctrl_t *fc , uint16_t tei , uint32_t nid)
{
	if(HPLC.testmode != 0)return TRUE;
	if(fc->dt == DT_SACK)
	{
		sack_ctrl_t* tempfc = (sack_ctrl_t*)fc;
		if(tempfc->ext_dt ==1 || tempfc->ext_dt==2)
		{
			return TRUE;
		}
		else
		{	
			return tempfc->dtei == tei  && tempfc->snid == nid ;
		}

	}
	else if(fc->dt == DT_COORD)
	{
		#if defined(STATIC_MASTER)
		return TRUE;
		#else
		return FALSE;
		#endif
	}
	else if(fc->dt == DT_SOF)
	{ 
		sof_ctrl_t* tempfc = (sof_ctrl_t*)fc;
		if(( tempfc->dtei == tei || tempfc->dtei==0xfff)  )//&& tempfc->snid == nid )
		{
			return TRUE;
		}
		//扩展命令格式(nid为0，源TEI规定为0；目的TEI规定为FFF)
		else if(tempfc->stei==0 && tempfc->snid == 0 && tempfc->dtei==0xfff)
		{
			return TRUE;
		}
		else
		{
			 return FALSE;
		}
	}
	else
	{
		return TRUE;
	}

}

static __inline__ uint8_t expected_sof(sof_ctrl_t *fc , uint16_t tei , uint32_t nid)
{
	return fc->dtei == tei && fc->snid == nid && (nid != 0 || fc->stei == 0xffe || fc->stei == 0xffd);
}














#endif



















































