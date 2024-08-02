/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     Apstask.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */

#include <stdlib.h>
#include "trios.h"
#include "sys.h"
#include "vsh.h"
#include "ZCsystem.h"
#include "dev_manager.h"
#include "meter.h"
#include "aps_interface.h"
#include "app.h"
#include "app_clock_sync_sta.h"
#include "app_dltpro.h"
#include "printf_zc.h"
#include "app_gw3762.h"
#include "SchTask.h"
#include "app_cnf.h"
#include "dl_mgm_msg.h"
#include "app_clock_os_ex.h"


#if defined(ZC3750STA)||defined(ZC3750CJQ2)


void sta_time_calibrate_put_list(U8 *pTimeData, U16 dataLen, U8 CjqFlag, U32 Baud,U8 IntervalTime)
{
    add_serialcache_msg_t serialmsg;
    memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));


	serialmsg.Uartcfg = (CjqFlag == TRUE) ? meter_serial_cfg : NULL;
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
    
    serialmsg.Match  = NULL;
    serialmsg.Timeout = NULL;
    serialmsg.RevDel = NULL;

    serialmsg.baud  = baud_enum_to_int(Baud);
    serialmsg.verif = 1;    /*0:��У�飬1��żУ�飬2����У��*/


    serialmsg.Protocoltype = 0;
    serialmsg.IntervalTime = IntervalTime;
	serialmsg.DeviceTimeout = 200;
	serialmsg.ack = TRUE;//�㲥Уʱ�ȱ���Ӧ
	serialmsg.FrameLen = dataLen;
	serialmsg.FrameSeq = Baud;
	serialmsg.lid = e_Serial_Broadcast;
	serialmsg.ReTransmitCnt = 0;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;


	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pTimeData,TRUE);

    return;

}




void time_calibrate_ind_proc(TIME_CALIBRATE_IND_t *pTimeCalibrateInd) 
{
    if(AppNodeState != e_NODE_ON_LINE || nnib.MacAddrType == e_UNKONWN)
	{
	    app_printf("exacttime module abnormal,don't execute indication\n");
		return;
	}
    U16 remainlen1=pTimeCalibrateInd->DataLength;
    U16 remainlen2=pTimeCalibrateInd->DataLength;
    U16 len=0;  
    U16 offset=0;
    static U8  SerialSeq = 0;
 
    app_printf("--TimeCalibrateIndication!\n");    
    if(e_Decode_Success!=ScaleDLT(DLT645_2007,pTimeCalibrateInd->TimeData,&offset,&remainlen1,&len) &&
         e_Decode_Success!=ScaleDLT(DLT698,pTimeCalibrateInd->TimeData,&offset,&remainlen2,&len))    
    {       
        app_printf("DLT decode failed!\n");             
        return;   
    }
    SerialSeq ++;
#if defined(ZC3750STA)||defined(ZC3750CJQ2)
    U8  CqjFlag = FALSE;
    U8  Baud = 0;
#if defined(ZC3750CJQ2)
    cjq2_search_suspend_timer_modify(20*TIMER_UNITS);
    for(Baud = METER_BAUD_1200; Baud < METER_BAUD_END; Baud++)
    {
        CqjFlag = TRUE;
#endif
        sta_time_calibrate_put_list(pTimeCalibrateInd->TimeData, pTimeCalibrateInd->DataLength, CqjFlag, SerialSeq<<8|Baud,50);
		#if defined(ZC3750STA)
		if(PROVINCE_SHANDONG==app_ext_info.province_code || PROVINCE_HEILONGJIANG==app_ext_info.province_code)  //������/ɽ��ʹ��
		{
			sta_set_moudle_time(pTimeCalibrateInd->TimeData, pTimeCalibrateInd->DataLength,offset);
		}
		#endif	
#if defined(ZC3750CJQ2)
        //os_sleep(20*(METER_BAUD_END-Baud+1));
    }

#endif
 
    //dump_buf(pTimeCalibrateInd->TimeData, pTimeCalibrateInd->DataLength);
 
#elif defined(STATIC_MASTER)

#endif
 
}

#endif
#if defined(STATIC_MASTER)

ostimer_t *Afn05F3StartBc_timer = NULL;
U8  Afn05F3StartBcTimeoutFlag = 0;

void time_calibrate_req_aps(U8 *TimeData,U8 DataLen)
{
	U8 fenum = 0;

    TIME_CALIBRATE_REQ_t *pTimeCalibrateReq_t = NULL;
    
    pTimeCalibrateReq_t = (TIME_CALIBRATE_REQ_t*)zc_malloc_mem(sizeof(TIME_CALIBRATE_REQ_t)+DataLen, "StartRegisterReq",MEM_TIME_OUT);

    pTimeCalibrateReq_t->DataLength = 6;
    memset(pTimeCalibrateReq_t->DstMacAddr, 0xFF, 6);
    __memcpy(pTimeCalibrateReq_t->SrcMacAddr, GetNnibMacAddr(), 6);
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code || PROVINCE_SHANDONG==app_ext_info.province_code)   //������/ɽ��ʹ��
	{
		if(Check698Frame(TimeData,DataLen,&fenum,NULL,NULL) == TRUE)
		{
	        __memcpy(pTimeCalibrateReq_t->TimeData,TimeData+fenum,DataLen-fenum);
		}
		else if(Check645Frame(TimeData,DataLen,&fenum,NULL,NULL) == TRUE)
		{
	        __memcpy(pTimeCalibrateReq_t->TimeData,TimeData+fenum,DataLen-fenum);
		}
		else
		{
	        app_printf("frame error\n");
	        zc_free_mem(pTimeCalibrateReq_t);
	        return;
		}
	}
    else
    {
        __memcpy(pTimeCalibrateReq_t->TimeData, TimeData, DataLen);
    }
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code || PROVINCE_SHANDONG==app_ext_info.province_code)//������/ɽ��ʹ��
    	pTimeCalibrateReq_t->DataLength = DataLen  - fenum;
	else
		pTimeCalibrateReq_t->DataLength = DataLen;

    ApsTimeCalibrateRequest(pTimeCalibrateReq_t);
	
    zc_free_mem(pTimeCalibrateReq_t);
}

void time_calibrate_cfm_pro(uint8_t Status)
{
    
	app_printf("TimeCalibrateCfmPro confirm : %d\n",Status);

    if(Status == e_SUCCESS)
    {
        Afn05F3StartBcTimeoutFlag = 1;
    }
    else
    {
        Afn05F3StartBcTimeoutFlag = 0;
    }
    timer_stop(Afn05F3StartBc_timer,TMR_CALLBACK);
}
/*************************************************************************
 * ��������	: 	void Afn05F3StartBc_timer_CB(struct ostimer_s * ostimer, void * arg)
 * ����˵��	: 	�ص�����
 * ����˵��	: 	��
 * ����ֵ		: 	��
 *************************************************************************/

void afn_05f3_send_sure_deny()
{
    app_printf("Afn05F3StartBcTimeoutFlag = %d\n", Afn05F3StartBcTimeoutFlag);
    if (Afn05F3StartBcTimeoutFlag == 0) // ��ʱ�ص�
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, e_UART1_MSG); // Ŀǰ��Ϊ���ڵ�æ
    }
    else // �յ�confirm��timer stop �ص�68 15 00 83 00 00 00 00 00 02 00 01 00 FF FF FF FF 02 00 84 16
    {
        app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, e_UART1_MSG);
    }
}

static void Afn05F3StartBc_timer_CB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"STARTBC",MEM_TIME_OUT);
    if(NULL == work)
	{
        printf_s("STARTBC is NULL\n");
		return;
	}
    work->work = afn_05f3_send_sure_deny;
    work->msgtype = SEND_05F3;
    post_app_work(work);
}
/*************************************************************************
 * ��������	: 	static void TimerCreate(U32 Ntenms)
 * ����˵��	: 	��ʱ����������
 * ����˵��	: 	��ʱʱ��
 * ����ֵ		: 	��
 *************************************************************************/
void TimerCreate(U32 Ntenms)
{
    if(Afn05F3StartBc_timer == NULL)
    {
        Afn05F3StartBc_timer = timer_create(Ntenms,
                                            0,
                                            TMR_TRIGGER,
                                            Afn05F3StartBc_timer_CB,
                                            NULL,
                                            "Afn05F3StartBc_timer"
                                            );
    }
    else
    {
        timer_modify(Afn05F3StartBc_timer
                     , Ntenms
                     , 0
                     , TMR_TRIGGER
                     , Afn05F3StartBc_timer_CB
                     , NULL
                     , "Afn05F3StartBc_timer"
                     ,TRUE);
    }

    if(Afn05F3StartBc_timer)
        timer_start(Afn05F3StartBc_timer);
    else
        sys_panic("<Afn05F3StartBc_timer fail!> %s: %d\n");
}

#endif

/**
 * У��㲥Уʱ֡�Ϸ��Բ���ȡBCDʱ��
 *
 * @param frame_buf ҪУ���֡������ָ��
 * @param frame_buf_len ֡����������
 * @param ret_bcd_time ���ڷ��غϷ��� BCD ʱ��
 * @return U8 ��ʾУ������TRUE ��ʾУ��ͨ����FALSE ��ʾУ��ʧ��
 */
U8 validate_and_extract_broadcast_time(U8 *frame_buf, U16 frame_buf_len, SysDate_t *ret_bcd_time)
{
    SysDate_t bcd_date = {0x00};
    SysDate_t bin_date = {0x00};
    date_time_s bin_698_date = {0x00};

    /* �ж�֡�Ϸ��� */
    const U8 is_645_frame = Check645Frame(frame_buf, frame_buf_len, NULL, NULL, NULL);
    const U8 is_698_frame = Check698Frame(frame_buf, frame_buf_len, NULL, NULL, NULL);

    if (is_645_frame)
    {
        get_time_from_645(frame_buf, frame_buf_len, &bcd_date);                  // ���� 645 ֡��ȡʱ��
        const U8 is_bcd_time = FuncJudgeBCD((U8 *)&bcd_date, sizeof(SysDate_t)); // �ж��Ƿ��� BCD ��

        if (!is_bcd_time)
        {
            app_printf("Time error , is_bcd_time:%s\n", is_bcd_time ? "TRUE" : "FALSE"); // ��ӡ������Ϣ
            return FALSE;
        }

        bcd_to_bin((U8 *)&bcd_date, (U8 *)&bin_date, sizeof(SysDate_t));
    }
    else if (is_698_frame)
    {
        U16 datalen = 0;
        Server698FrameProc(frame_buf, frame_buf_len, (U8 *)&bin_698_date, &datalen); // ���� 698 ֡��ȡʱ��

        DatetimesToSysDate(&bin_date, bin_698_date);

        bin_to_bcd((U8 *)&bin_date, (U8 *)&bcd_date, sizeof(SysDate_t)); // ��ʱ��ת��Ϊ BCD ��ʽ
    }
    else // ���� 645 Ҳ���� 698
    {
        app_printf("645 or 698 frame error !\n"); // ��ӡ������Ϣ
        return FALSE;                             // ����У��ʧ��
    }

    const U8 is_valid_time = FunJudgeTime(bin_date); // �ж�ʱ���Ƿ�Ϸ�

    if (!is_valid_time)
    {
        app_printf("Time error , is_valid_time:%s\n", is_valid_time ? "TRUE" : "FALSE"); // ��ӡ������Ϣ
        return FALSE;                                                                    // ����У��ʧ��
    }

    if (NULL != ret_bcd_time)
    {
        *ret_bcd_time = bcd_date; // ���غϷ��� BCD ʱ��
    }

    return TRUE; // ����У��ͨ��
}
