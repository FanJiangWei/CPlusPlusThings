/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */
#include "ZCsystem.h"
#include "mem_zc.h"
#include "app.h"

#include "app_read_router_cco.h"
#include "datalinkdebug.h"
#include "app_dltpro.h"
#include "SchTask.h"
#include "app_read_cjq_list_sta.h"


#if defined(STATIC_NODE)


static U16 packet_device_info(U8 *pdata)
{
    U16 len=0;
#if defined(ZC3750STA)
    //U16 ii=0;
    //U8 empty_addr[6]={0,0,0,0,0,0};
	QUERY_SLAVE_REG_DATA_t *pQuery_Slave_Reg_Data_t=(QUERY_SLAVE_REG_DATA_t *)pdata;
#ifdef TH2CJQ
	if(e_IOT_C_MODE_STA == DevicePib_t.Modularmode)
	{
		memcpy(pQuery_Slave_Reg_Data_t->Addr,DevicePib_t.DevMacAddr,6);
		pQuery_Slave_Reg_Data_t->Protocol = DevicePib_t.Prtcl;
		pQuery_Slave_Reg_Data_t->ModeType = DevicePib_t.DevType;
	    pQuery_Slave_Reg_Data_t->res = 0 ;
		len=sizeof(QUERY_SLAVE_REG_DATA_t);
	}
	else
	{
	    U16 ii=0;
	        
	    for(ii=0;ii<TH2CJQ_MeterList.MeterNum;ii++)
	    {
	        pQuery_Slave_Reg_Data_t = (QUERY_SLAVE_REG_DATA_t *)&pdata[len];
	        len += sizeof(QUERY_SLAVE_REG_DATA_t);
	        memcpy(pQuery_Slave_Reg_Data_t->Addr , TH2CJQ_MeterList.MeterInfo[ii].MeterAddr,6);
	        pQuery_Slave_Reg_Data_t->Protocol = (METER_PROTOCOL_07==TH2CJQ_MeterList.MeterInfo[ii].Protocol)?APP_07_PROTOTYPE:
	                                            (METER_PROTOCOL_97==TH2CJQ_MeterList.MeterInfo[ii].Protocol)?APP_97_PROTOTYPE:
	                                            (METER_PROTOCOL_698==TH2CJQ_MeterList.MeterInfo[ii].Protocol)?APP_698_PROTOTYPE:
	                                                APP_T_PROTOTYPE;
	        pQuery_Slave_Reg_Data_t->ModeType = e_CJQ2_APP;
	                   pQuery_Slave_Reg_Data_t->res = 0;
	    }
	}
#else
	__memcpy(pQuery_Slave_Reg_Data_t->Addr,DevicePib_t.DevMacAddr,6);
	pQuery_Slave_Reg_Data_t->Protocol = DevicePib_t.Prtcl;
	pQuery_Slave_Reg_Data_t->ModeType = DevicePib_t.DevType;
         pQuery_Slave_Reg_Data_t->res = 0 ;
	len=sizeof(QUERY_SLAVE_REG_DATA_t);
#endif
	
#elif defined(ZC3750CJQ2)
	QUERY_SLAVE_REG_DATA_t *pQuery_Slave_Reg_Data_t;  
	U16 ii=0;
	
	for(ii=0;ii<CollectInfo_st.MeterNum;ii++)
	{
		pQuery_Slave_Reg_Data_t = (QUERY_SLAVE_REG_DATA_t *)&pdata[len];
		len += sizeof(QUERY_SLAVE_REG_DATA_t);
		__memcpy(pQuery_Slave_Reg_Data_t->Addr , CollectInfo_st.MeterList[ii].MeterAddr,6);
		pQuery_Slave_Reg_Data_t->Protocol = (METER_PROTOCOL_07==CollectInfo_st.MeterList[ii].Protocol)?APP_07_PROTOTYPE:
											(METER_PROTOCOL_97==CollectInfo_st.MeterList[ii].Protocol)?APP_97_PROTOTYPE:
											(METER_PROTOCOL_698==CollectInfo_st.MeterList[ii].Protocol)?APP_698_PROTOTYPE:
												APP_T_PROTOTYPE;
		pQuery_Slave_Reg_Data_t->ModeType = e_CJQ2_APP;
                   pQuery_Slave_Reg_Data_t->res = 0;
	}
	
#endif	
	return len;
}


void SlaveRegQueryIndication(REGISTER_QUERY_IND_t *pRegsiterQueryInd_t) 
{
    REGISTER_QUERY_RSP_t* pRegisterQueryRsp_t = NULL;
    U8    MeterCount;
    U16   MeterInfoLen = 0;

    #ifdef TH2CJQ
	if(e_IOT_C_MODE_STA == DevicePib_t.Modularmode)
	{
		MeterCount = 1;
	}
	else
	{
    	MeterCount = TH2CJQ_MeterList.MeterNum;
	}
    #else
    MeterCount = 1;
    #endif

    #if defined(ZC3750CJQ2)
    MeterCount = CollectInfo_st.MeterNum;
    #endif

    MeterInfoLen = MeterCount*sizeof(QUERY_SLAVE_REG_DATA_t);
    
    pRegisterQueryRsp_t = (REGISTER_QUERY_RSP_t*)zc_malloc_mem(sizeof(REGISTER_QUERY_RSP_t)+MeterInfoLen,"RegQueryRsp",MEM_TIME_OUT);
    
   	if(pRegsiterQueryInd_t->RegisterParameter == e_QUERY_REGISTER_RESULT)
   	{
   	    #if defined(ZC3750STA)	
        pRegisterQueryRsp_t->Status         =   0;

        #ifdef TH2CJQ
		if(e_IOT_C_MODE_STA == DevicePib_t.Modularmode)
		{
			pRegisterQueryRsp_t->MeterCount     =   1;
        	pRegisterQueryRsp_t->DeviceType     =   e_METER_APP;
		}
		else
		{
	        pRegisterQueryRsp_t->MeterCount     =   TH2CJQ_MeterList.MeterNum;
	        pRegisterQueryRsp_t->DeviceType     =   e_CJQ2_APP;
		}
        #else
        pRegisterQueryRsp_t->MeterCount     =   1;
        pRegisterQueryRsp_t->DeviceType     =   e_METER_APP;
        #endif

        pRegisterQueryRsp_t->RegisterParameter = 0;
        __memcpy(pRegisterQueryRsp_t->DeviceAddr,DevicePib_t.DevMacAddr,6);
        
        // ChangeMacAddrOrder(pRegisterQueryRespones_t->DeviceAddr);
        __memcpy(pRegisterQueryRsp_t->DeviceId,DevicePib_t.DevMacAddr,6);
        __memcpy(pRegisterQueryRsp_t->SrcMacAddr,pRegsiterQueryInd_t->DstMacAddr,6);
        __memcpy(pRegisterQueryRsp_t->DstMacAddr,pRegsiterQueryInd_t->SrcMacAddr,6);
		
        #elif defined(ZC3750CJQ2)
	    
        //pRegisterQueryRespones_t->Status         =   (SEARCH_STATE_STOP==Cjq2SearchMeterInfo.Ctrl.SearchState)?0:1;
        pRegisterQueryRsp_t->Status         =   0;
        pRegisterQueryRsp_t->MeterCount     =   CollectInfo_st.MeterNum;
        pRegisterQueryRsp_t->DeviceType     =   e_CJQ2_APP;
        pRegisterQueryRsp_t->RegisterParameter = 0;
        //__memcpy(pRegisterQueryRespones_t->DeviceAddr,DefSetInfo.DeviceAddrInfo.DeviceBaseAddr,6);
        if(PROVINCE_ANHUI == app_ext_info.province_code) //todo: PROVINCE_ANHUI
        {
            //安徽使用二采地址进行上报
            __memcpy(pRegisterQueryRsp_t->DeviceAddr, DefSetInfo.DeviceAddrInfo.DeviceBaseAddr, 6);
        }
        else
        {
        __memcpy(pRegisterQueryRsp_t->DeviceAddr,CollectInfo_st.CollectAddr,6);
        ChangeMacAddrOrder(pRegisterQueryRsp_t->DeviceAddr);
        }

        __memcpy(pRegisterQueryRsp_t->DeviceId,CollectInfo_st.CollectAddr,6);
        __memcpy(pRegisterQueryRsp_t->SrcMacAddr, pRegsiterQueryInd_t->DstMacAddr,6);
        __memcpy(pRegisterQueryRsp_t->DstMacAddr, pRegsiterQueryInd_t->SrcMacAddr,6);
		#endif

		pRegisterQueryRsp_t->PacketSeq = pRegsiterQueryInd_t->PacketSeq;
		pRegisterQueryRsp_t->MeterInfoLen = packet_device_info(pRegisterQueryRsp_t->MeterInfo);

		ApsSlaveRegQueryResponse(pRegisterQueryRsp_t);
   	}
	else if(pRegsiterQueryInd_t->RegisterParameter == e_LOCK_CMD)
	{
		pRegisterQueryRsp_t->Status         =   0;
		pRegisterQueryRsp_t->MeterCount     =   1;
		pRegisterQueryRsp_t->DeviceType     =   e_METER_APP;
		pRegisterQueryRsp_t->RegisterParameter = 0;
		__memcpy(pRegisterQueryRsp_t->DeviceAddr,DevicePib_t.DevMacAddr,6);
	        // ChangeMacAddrOrder(pRegisterQueryRespones_t->DeviceAddr);
		__memcpy(pRegisterQueryRsp_t->DeviceId,DevicePib_t.DevMacAddr,6);
		__memcpy(pRegisterQueryRsp_t->SrcMacAddr, pRegsiterQueryInd_t->DstMacAddr,6);
		__memcpy(pRegisterQueryRsp_t->DstMacAddr, pRegsiterQueryInd_t->SrcMacAddr,6);
        pRegisterQueryRsp_t->PacketSeq = pRegsiterQueryInd_t->PacketSeq;
		
		pRegisterQueryRsp_t->MeterInfoLen = 0;//PacketDeviceInfo(pRegisterQueryRsp_t->MeterInfo);

		ApsSlaveRegQueryResponse(pRegisterQueryRsp_t);	
   	}


    zc_free_mem(pRegisterQueryRsp_t);

}





#endif 

