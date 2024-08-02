/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     app_time_calibrate.h
 * Purpose:	 app time calibrate function
 * History:
 * june 10, 2020	dhc    Create
 */


#ifndef __APP_COMMU_TEST_H__
#define __APP_COMMU_TEST_H__
#include "ZCsystem.h"
#include "aps_interface.h"
#include "aps_layer.h"

extern uint8_t g_phr_msc ;
extern uint8_t g_psdu_mcs;
extern uint8_t g_pbsize  ;


void CommuTestIndication(COMMU_TEST_IND_t *pCommuTestInd);

void phy_tpt(U16 time);
void phy_cb(U8 testmode, U16 time);
void mac_tpt(U16 time);
void phy_change_band(U8 idx);
void tonemask_cfg(U8 idx);

void wphy_tpt(U16 time);
void wphy_cb(U16 time);

void safeTest_cfg(U8 TestMode, U16 time);
void wphy_channel_set(U8 option, U8 channel);

#endif


