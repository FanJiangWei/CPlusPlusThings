/*
 * File:	app_current_abnormal_event_cco.c
 * Purpose:	Zero Live wire current abnormal event reported
 * History:
 *
 */
#include "ZCsystem.h"
#include "app_base_multitrans.h"
#include "app_read_jzq_cco.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_power_onoff.h"
#include "app_exstate_verify.h"
#include "Datalinkdebug.h"
#include "app_698p45.h"
#include "app_common.h"
#include "app_gw3762.h"
#include "app_current_abnormal_event_cco.h"

/*零火线电流异常上报*/
#if defined(STATIC_MASTER) && defined(SHAAN_XI)
U8  g_LiveLineZeroLineReportCnt = 0;
U8  g_LiveLineZeroLineReportFlag = FALSE;
U8  g_LiveLineZeroLinePowerStaFlag =0;

ostimer_t *liveline_zeroline_cco_event_send_timer = NULL;
ostimer_t *LiveLineZeroLine_clean_bitmap_timer = NULL;
U16 g_LiveLineZeroLineSwitchBoard =0;
U16 g_LiveLineZeroLineStartchBoard =0;

LiveLineZeroLine_UP_Data_t LiveLineZeroLine_UP_Data[POWERNUM];
BitMapPowerInfo_t LiveLineZeroLineAddrList[POWERNUM];

extern U8  Gw3762TopUpReportData[512];


U8 zero_live_line_event_cal(U8 FunCode, U8 *pEvent,U16  pEventLen)
{
	U16   rcvLen = 0;
	//U16   ii =0 ;
	U16    len,cnt;
	U8    powereventtype;
	powereventtype = pEvent[rcvLen];
	U16  renum ;
	//app_printf("e_LIVE_ZERO_ELEC_EVENT\n");
	if (powereventtype == e_LIVE_ZERO_ELEC_EVENT)
	{
		renum = (pEvent[rcvLen+1])| pEvent[rcvLen+2]<<8;//报文内电表个数
		rcvLen += 3;
		len = pEventLen - 3;
		app_printf("///len=%d,renum=%d,type=%d",len,renum,powereventtype);
		cnt = zero_live_line_map_trans( &pEvent[rcvLen] ,len, renum);
		app_printf("LiveLineZeroLineProcnt=%d",cnt);


		
		g_LiveLineZeroLineReportCnt =  MAX(cnt,g_LiveLineZeroLineReportCnt);
	}
	else
	{
		return 0;
	}
	return g_LiveLineZeroLineReportCnt;
}

U8  zero_live_line_map_trans(U8 *SrcBitMap , U16  SrcBitMapLen,U16 num)
{
	U16 ii ,jj;
    U16 reportcnt=0 ;
    U16 FirstNullSeq = 0xFFFF;
    U16 FindMeterSeq = 0xFFFF;
    U8 buf[6]={0,0,0,0,0,0};
    BitMapPowerInfo_t *DstArray = NULL;
    LiveLineZeroLine_UP_Data_t * Data_Map =NULL;
    if(SrcBitMapLen!=0&&SrcBitMapLen/sizeof(LiveLineZeroLine_UP_Data_t) != num)
    {
        return 0;
    }
	app_printf("num  =%d\n",num);
	DstArray = ( BitMapPowerInfo_t *)&LiveLineZeroLineAddrList;
	Data_Map =(LiveLineZeroLine_UP_Data_t * )SrcBitMap;
    for(ii=0; ii<num; ii++)
    {
        FirstNullSeq = 0xFFFF;
        FindMeterSeq = 0xFFFF;
        for(jj=0; jj<POWERNUM; jj++)
        {
            //找到重复地址
            if(FindMeterSeq == 0xFFFF && memcmp(Data_Map[ii].MacAddr,LiveLineZeroLine_UP_Data[jj].MacAddr,6)==0)
            {
            	//memcpy(&(U8*)LiveLineZeroLine_UP_Data[ii],SrcBitMap+(ii*sizeof(LiveLineZeroLine_UP_Data_t)),sizeof(LiveLineZeroLine_UP_Data_t));
            	__memcpy((U8*)&LiveLineZeroLine_UP_Data[jj],(U8*)&Data_Map[ii],sizeof(LiveLineZeroLine_UP_Data_t));
                reportcnt += renew_power_falg(&DstArray[jj],1,1,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
                app_printf("reportcnt1 = %d \n",reportcnt);
                //直接跳出，无需找空
                FindMeterSeq = jj;
                break;
            }
            //找到空地址
            if(FirstNullSeq == 0xFFFF && memcmp(LiveLineZeroLine_UP_Data[jj].MacAddr,buf,6)==0)
            {
                FirstNullSeq = jj;
            }
            if(FindMeterSeq != 0xFFFF && FirstNullSeq != 0xFFFF)
            {
                break;
            }
        }
        //添加新的地址
        if(FindMeterSeq == 0xFFFF && FirstNullSeq != 0xFFFF)
        {
            //memcpy(PowerAddrList[FirstNullSeq].MacAddr, &SrcBitMap[ii*7], 6);
			__memcpy((U8*)&LiveLineZeroLine_UP_Data[FirstNullSeq],(U8*)&Data_Map[ii],sizeof(LiveLineZeroLine_UP_Data_t));
            reportcnt += renew_power_falg(&DstArray[FirstNullSeq],1,1,CCO_CLEAN_BITMAP_TIME/CCO_CLEAN_PERIOD_TIME);
			app_printf("reportcnt2 = %d \n",reportcnt);
        }
    }
	g_LiveLineZeroLineSwitchBoard+=reportcnt;
    if(reportcnt > 0)
    {
        return 6;
    }
    else
    {
        return 0;
    }
}
//-------------------------------------------------------------------------------------------------
// 函数名：void aps_zero_fire_line_enabled_request(LiveLineZeroLineEnabled_t      *pEventReportReq)
// 
// 功能描述：
//     本函数由CCO用于发送电流异常上报使能
// 返回值：
//     无
//-------------------------------------------------------------------------------------------------

void  aps_zero_fire_line_enabled_request(LiveLineZeroLineEnabled_t      *pEventReportReq)
{
	MSDU_DATA_REQ_t            *pMsduDataReq = NULL;
	APDU_HEADER_t              *pApsHeader = NULL;
	Zero_Fire_Line_Line_Enabled_t * pRegisterHeader = NULL;
	U16   msdu_length = 0;
	msdu_length = sizeof(APDU_HEADER_t)+sizeof(Zero_Fire_Line_Line_Enabled_t)+1;
	work_t  *post_work = zc_malloc_mem(sizeof(work_t)+sizeof(MSDU_DATA_REQ_t)+msdu_length, "MsduDataRequest",MEM_TIME_OUT);
	app_printf("aps_zero_fire_line_enabled_request  \n");
	pMsduDataReq    = (MSDU_DATA_REQ_t*)post_work->buf;
    pApsHeader      = (APDU_HEADER_t*)pMsduDataReq->Msdu;
    pRegisterHeader = (Zero_Fire_Line_Line_Enabled_t*)pApsHeader->Apdu;
	
	// 填APS通用帧头
    pApsHeader->PacketPort = e_READMETER_PACKET_PORT;
    pApsHeader->PacketID   = e_ZERO_FIRE_LINE_ENABLED;
    pApsHeader->PacketCtrlWord = APS_PACKET_CTRLWORD;

	//填写报文内容
	pRegisterHeader->ProtocolVer = PROTOCOL_VERSION_NUM;
	pRegisterHeader->HeaderLen  = sizeof(Zero_Fire_Line_Line_Enabled_t)-pEventReportReq->EvenDataLen;
	pRegisterHeader->Direction = e_DOWNLINK;//下行
    pRegisterHeader->PrmFlag   = 1;//启动站
    pRegisterHeader->FunCode   = 0;
	pRegisterHeader->PacketSeq = pEventReportReq->PacketSeq;
	__memcpy(pRegisterHeader->SrcMacAddr,pEventReportReq->SrcMacAddr,6);
	pRegisterHeader->CommandType = pEventReportReq ->CommandType;
	pRegisterHeader->EvenDataLen = pEventReportReq->EvenDataLen;
	__memcpy(pRegisterHeader->EventData,pEventReportReq->EventData,pEventReportReq->EvenDataLen);
	ApsPostPacketReq(post_work, msdu_length, 0, pEventReportReq->DstMacAddr, 
         pEventReportReq->SrcMacAddr, e_FULL_BROADCAST_FREAM_NOACK, (pApsHeader->PacketID << 16) + (++ApsHandle), SLAVE_REGISTER_LID);
    return;
}
static void zero_live_line_cco_power_event_send(work_t *work)
{
	U16   byLen = 0,ii;
	U8 offcountzz = 0;
	U8 null_addr[6] = {0};
	U16 num =0;
	byLen = 5;
	g_LiveLineZeroLinePowerStaFlag = 0;
	if(g_LiveLineZeroLineReportFlag == TRUE)
	{
		for(ii=0; ii<POWERNUM; ii++)
    	{
    		//if((memcmp(null_addr,LiveLineZeroLine_UP_Data[ii].MacAddr,6)==0)&&(0 == LiveLineZeroLine_UP_Data[ii].Up_ok))
    		if(memcmp(null_addr,LiveLineZeroLine_UP_Data[ii].MacAddr,6)==0)
       	 	{
       	 		//app_printf("LiveLineZeroLine_UP_Data[ii].MacAddr is null_addr \n");
        		continue;
			}
			//app_printf("LiveLineZeroLineAddrList[ii].PowerReportFlag = %d\n",LiveLineZeroLineAddrList[ii].PowerReportFlag);
			if((offcountzz<20)&&(1==LiveLineZeroLineAddrList[ii].PowerReportFlag))
			//if(offcountzz<20)
			{
				g_LiveLineZeroLinePowerStaFlag = e_LIVE_ZERO_ELEC_EVENT;
				dump_buf(LiveLineZeroLine_UP_Data[ii].MacAddr,6);
				__memcpy(Gw3762TopUpReportData+byLen,&LiveLineZeroLine_UP_Data[ii],sizeof(LiveLineZeroLine_UP_Data_t));
				LiveLineZeroLineAddrList[ii].PowerReportFlag = 0;
				byLen+=sizeof(LiveLineZeroLine_UP_Data_t);
				offcountzz++;
			}
			if(offcountzz>=20)
			{
				//continue;
				break;
			}
		}
		num = g_LiveLineZeroLineSwitchBoard;
		Gw3762TopUpReportData[4]= offcountzz;
		__memcpy(Gw3762TopUpReportData,&num,2);
		num = g_LiveLineZeroLineStartchBoard +1;
		g_LiveLineZeroLineStartchBoard += offcountzz;
		__memcpy(Gw3762TopUpReportData+2,&num,2);
		app_printf("offcountzz =%d\n",offcountzz );
	}
	if(g_LiveLineZeroLinePowerStaFlag == e_LIVE_ZERO_ELEC_EVENT)
    {
		
    	app_gw3762_up_afn06_f5_report_slave_event(		FALSE,
												APP_GW3762_HPLC_MODULE ,
												APP_LIVE_ZERO_ELEC_EVENT,
												0,
												byLen,
												Gw3762TopUpReportData);
        app_printf("app_gw3762_up_afn06_f5_report_slave_event\n");
   }
	if(g_LiveLineZeroLinePowerStaFlag)
   {      
        modify_zero_live_line_cco_power_event_send_timer(200);
        g_LiveLineZeroLineReportFlag = TRUE;//如果有上报，等待下一次确认是否需要上报
   }
}

static void zero_live_line_cco_clean_bitmap(work_t *work)
{
	U16 ii;
    U8 stopcleanflag = FALSE;
    //STA BitMap
    for(ii = 0;ii<POWERNUM;ii++)
    {        
        //ADDR off
         if(LiveLineZeroLineAddrList[ii].RemainTime == 0)
         {
           	memset(LiveLineZeroLine_UP_Data[ii].MacAddr,0x00,sizeof(LiveLineZeroLine_UP_Data_t));
         }
		 //app_printf("LiveLineZeroLineAddrList[ii].RemainTime =%d,ii=%d\n",LiveLineZeroLineAddrList[ii].RemainTime,ii);
         if(sub_power_remain_time(&LiveLineZeroLineAddrList[ii] ,NULL))
		 {
     	   stopcleanflag = TRUE;
		 }
    }
	if(stopcleanflag == FALSE)
    {
    	app_printf("stopcleanflag == FALSE\n");
        g_LiveLineZeroLinePowerStaFlag=FALSE;
		g_LiveLineZeroLineSwitchBoard =0;
		g_LiveLineZeroLineStartchBoard =0;
        timer_stop(liveline_zeroline_cco_event_send_timer,TMR_NULL);
        timer_stop(LiveLineZeroLine_clean_bitmap_timer,TMR_NULL);
    }

    return;
}

void zero_live_line_cco_power_event_send_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	work_t *work = zc_malloc_mem(sizeof(work_t),"CcoPowerEventSend",MEM_TIME_OUT);
    work->work = zero_live_line_cco_power_event_send;
    work->msgtype = CCO_EVTSEND;
	post_app_work(work);	
}
void zero_fire_line_cco_clean_bitmap_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"CcoCleanBitmap",MEM_TIME_OUT);
	work->work = zero_live_line_cco_clean_bitmap;
    work->msgtype = CCO_CLBIT;
	post_app_work(work);
}

void modify_zero_live_line_cco_power_event_send_timer(uint32_t first)
{
	if(NULL != liveline_zeroline_cco_event_send_timer)
	{
		timer_modify(liveline_zeroline_cco_event_send_timer,
				first,
				10,
				TMR_TRIGGER ,//TMR_TRIGGER
				zero_live_line_cco_power_event_send_timer_cb,
				NULL,
				"ccopowereventSendtimerCB",
				TRUE);
		timer_start(liveline_zeroline_cco_event_send_timer);
	}
	else
	{
		sys_panic("cco_event_send_timer is null\n");
	}
	
}

#endif

