/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     app_moduleid_query.h
 * Purpose:	 app time calibrate function
 * History:
 * june 10, 2020	dhc    Create
 */


#ifndef __APP_MODULEID_ID_H__
#define __APP_MODULEID_ID_H__

#include "ProtocolProc.h"
#include "aps_layer.h"

typedef enum
{
	APP_GW3762_MODULE_ID_OLD = 0,
    APP_GW3762_CHIP_ID ,
    APP_GW3762_MODULE_ID,
    
}QUERY_IDINFO_TYPE_e;


#if defined(STATIC_MASTER)

typedef void (*TaskUpProc)(void *pTaskPrmt, void *pUplinkData);


int8_t  moduleid_query_timer_init(void);

void id_info_query(U16 moduleIndex, U8 *pMacAddr, U8 idType, MESG_PORT_e port, TaskUpProc pUpProcFunc);
void cco_app_module_id_query_cfm_proc(QUERY_IDINFO_CFM_t  * pQueryIdInfoCfm);



#elif defined(STATIC_NODE)

void query_id_info_indication(QUERY_IDINFO_IND_t *pQueryIdInfoInd);
void rd_query_id_info_cfm(QUERY_IDINFO_CFM_t  * pQueryIdInfoCfm);

#endif

#endif

