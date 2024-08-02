/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     app_time_calibrate.h
 * Purpose:	 app time calibrate function
 * History:
 * june 10, 2020	dhc    Create
 */
#ifndef __APP_TIME_CALIBRATE_H__
#define __APP_TIME_CALIBRATE_H__



//void app_time_calibrate_init(void);

void time_calibrate_ind_proc(TIME_CALIBRATE_IND_t *pTimeCalibrateInd);

#if defined(ZC3750STA)||defined(ZC3750CJQ2)
void sta_time_calibrate_put_list(U8 *pTimeData, U16 dataLen, U8 CjqFlag, U32 Baud,U8 IntervalTime);
#endif

#if defined(STATIC_MASTER)

void time_calibrate_req_aps(U8 *TimeData,U8 DataLen);
void time_calibrate_cfm_pro(uint8_t Status);
void TimerCreate(U32 Ntenms);

#endif
U8 validate_and_extract_broadcast_time(U8 *frame_buf, U16 frame_buf_len, SysDate_t *ret_bcd_time);
#endif

