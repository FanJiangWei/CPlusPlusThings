#ifndef __APP_H__
#define __APP_H__
#include "sys.h"
#include "ZCsystem.h"
#include "DataLinkGlobal.h"
//#define MODULE_INSPECT


//#define  app_printf(fmt, arg...)  debug_printf(&dty, DEBUG_APP, fmt, ##arg)
#define  SET_PARAMETER(data,min,max,def)   (data<min?def: data>max?def: data)

#define DEF_FREQBAND  2
#define DEF_TGAIN_DIG  0
#define DEF_TGAIN_ANA  -2

#define CMP_VERSION 


event_t work_mutex;
event_t Mutex_RdCtrl_Search;


#define APP_PANIC()		sys_panic("[%s:%d]<APP panic> Error!!!\r\n", __func__, __LINE__)

#define app_printl(fmt, arg...) 	debug_printf(&dty, DEBUG_APP,"%s %d :" ,__func__,__LINE__ );\
						debug_printf(&dty, DEBUG_APP, fmt, ##arg);\
						debug_printf(&dty, DEBUG_APP,"\n")

void app_init(void);
void post_app_work(work_t *work);


#if defined(STATIC_MASTER)

typedef struct{
    U8       Resultflag;
}F1F1_INFO_t;

extern F1F1_INFO_t          f1f1infolist[MAX_WHITELIST_NUM] ;


#endif




















#endif 
