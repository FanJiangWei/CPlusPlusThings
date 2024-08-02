#include "flash.h"
#include "app_clock_os_ex.h"
#include "aps_layer.h"
#include "app_gw3762.h"
#include "app_base_multitrans.h"
#include "app_sysdate.h"
#include "Datalinkdebug.h"
#include "app_698client.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "app_time_calibrate.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_cnf.h"
#include "app_698client.h"
#include "app_dltpro.h"
#include "app_time_calibrate.h"
#include "app_clock_sta_timeover_report.h"



#if defined(STATIC_NODE) 
static void sta_send_exact_time(U8* pdata,U16 pdatalen,U32 Ntb)
{
	//↓↓↓黑龙江部分校时(GW基础)↓↓↓
	U8  BuffLen			   	 = 0;
	U8  Addr[6]			   	 = {0x99,0x99,0x99,0x99,0x99,0x99};
	U8  Controlcode		     = 0x08;
	U16 Len				     = 0;
	U8  offset               = 0;
	U8  fenum                = 0;
	SysDate_t    SysDateTemp = {0};
	date_time_s  _datetimes  = {0};
	U8	*PacketBuff 		 = zc_malloc_mem(20, "PacketBuff", MEM_TIME_OUT);
	U8	*Sendbuf             = zc_malloc_mem(50, "Sendbuf", MEM_TIME_OUT);

	if(Check698Frame(pdata,pdatalen,&fenum,NULL,NULL) == TRUE)
	{
	    U16 len_698 =0;
        Server698FrameProc(pdata+fenum,pdatalen,(U8*)&_datetimes,&len_698);
        app_printf("len%d\n",len_698);
        date_time_to_sysdate(&SysDateTemp,_datetimes);
        bin_to_bcd((U8*)&SysDateTemp,(U8*)&SysDateTemp,sizeof(SysDate_t));
	}
	else if(Check645Frame(pdata,pdatalen,&fenum,NULL,NULL) == TRUE)
	{
        offset = fenum+1+6+1+2;//68+addrlen+68+ctrl+datalen
        Sub0x33ForData(&pdata[offset],sizeof(SysDate_t));
        __memcpy((U8*)&SysDateTemp,&pdata[offset],sizeof(SysDate_t));//找到645的日期时间
	}
	else
	{
        app_printf("fream error\n");

    	zc_free_mem(PacketBuff);
    	zc_free_mem(Sendbuf);
        return; 
	}
	
	dump_buf((U8*)&SysDateTemp,6);
	__memcpy(PacketBuff,&Ntb,sizeof(U32));//NTB4个字节
	BuffLen += sizeof(U32);
	__memcpy(&PacketBuff[BuffLen],(U8*)&SysDateTemp,sizeof(SysDate_t));//time 6字节
	BuffLen += sizeof(SysDate_t);
	
	//sta_calibration_clk(PacketBuff,BuffLen);//计算完NTB差值后设置为模块当前时间
	if(clock_recv_infoproc(PacketBuff, BuffLen) == FALSE)
    {
        zc_free_mem(PacketBuff);
	    zc_free_mem(Sendbuf);
        return ;
    }

	GetBinTime(&SysDateTemp);
	if(DevicePib_t.Prtcl == DLT698)
	{
        SysDateToDatetimes(SysDateTemp,&_datetimes);
        sta_pkg_698_bc_cali_frame(Sendbuf, &Len,_datetimes);
        sta_time_calibrate_put_list(Sendbuf,Len,0,0,TimeCalIntarval);
	}
	else
	{
        bin_to_bcd((U8*)&SysDateTemp, (U8*)&SysDateTemp, sizeof(SysDate_t));
    	Add0x33ForData((U8*)&SysDateTemp,sizeof(SysDate_t));
    	Packet645Frame(Sendbuf, (U8 *)&Len, Addr,Controlcode,(U8*)&SysDateTemp,sizeof(SysDate_t));
    	sta_time_calibrate_put_list(Sendbuf,Len,0,0,TimeCalIntarval);
	}
	

	zc_free_mem(PacketBuff);
	zc_free_mem(Sendbuf);
}
#endif

void sta_exact_timer_calibrate_ind(work_t *work)//精准校时
{
#if defined(STATIC_NODE) 

    if(AppNodeState != e_NODE_ON_LINE || nnib.MacAddrType == e_UNKONWN)
	{
	    app_printf("exacttime module abnormal,don't execute indication\n");
		return;
	}
	U8	*pPayload 			 = zc_malloc_mem(50, "pPayload", MEM_TIME_OUT);
    U16 pPayloadLen          = 0;
	TIMER_CALIBRATE_HEADER_t    *Timer_CalibrateHeader_t= NULL;

	MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
	U16                PayloadLen = 0;

	Timer_CalibrateHeader_t = (TIMER_CALIBRATE_HEADER_t *)pApsHeader->Apdu;
	TIMER_CALIBRATE_IND_t *pTimerCalibrateInd_t= NULL;
	
	PayloadLen = pMsduDataInd->MsduLength-sizeof(APDU_HEADER_t)-sizeof(TIMER_CALIBRATE_IND_t);
	pTimerCalibrateInd_t = (TIMER_CALIBRATE_IND_t*)zc_malloc_mem(sizeof(TIMER_CALIBRATE_IND_t)+PayloadLen,"pClockMaintainInd_t",MEM_TIME_OUT);

	pTimerCalibrateInd_t->NTB  =  Timer_CalibrateHeader_t->NTB;
	pTimerCalibrateInd_t->AsduLength = Timer_CalibrateHeader_t->DataRelayLen;
	__memcpy(pTimerCalibrateInd_t->Asdu, Timer_CalibrateHeader_t->Asdu, pTimerCalibrateInd_t->AsduLength);

	__memcpy(pPayload,pTimerCalibrateInd_t->Asdu,pTimerCalibrateInd_t->AsduLength);
	pPayloadLen = pTimerCalibrateInd_t->AsduLength;
	app_printf("pPayload is:");
	dump_buf(pPayload,pPayloadLen);
    sta_send_exact_time(pPayload,pPayloadLen,pTimerCalibrateInd_t->NTB);
    
	zc_free_mem(pTimerCalibrateInd_t);
	zc_free_mem(pPayload);
#endif
}






































