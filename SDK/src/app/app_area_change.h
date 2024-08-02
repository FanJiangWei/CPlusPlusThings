#ifndef __APP_AREA_CHANGE_H__
#define __APP_AREA_CHANGE_H__
#include "ZCsystem.h"




#if defined(STATIC_MASTER)
extern U8 check_whitelist_by_addr(U8 *addr);

#define MAX_NEW_REPORT_BLACK_SIZE 1024

typedef struct{
    U8      MacAddr[6];
	U8      DeviceType;
	U8      ReportFlag;
}REPORT_NEW_BLACK_t;

void area_change_report(U8 *mac_addr,U8 device_type);

void area_change_show_report_info(tty_t *term);

void area_change_set_report_flag(U8 arg_state);

void area_change_report_wait_timer_refresh(void);

uint8_t area_change_report_info_init(void);

#endif

#endif


