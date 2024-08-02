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
#include "flash.h"
#include "mem_zc.h"
#include "app.h"
#include "app_base_multitrans.h"
#include "app_read_jzq_cco.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "SchTask.h"
#include "app_power_onoff.h"
#include "app_exstate_verify.h"
#include "Datalinkdebug.h"
#include "app_698p45.h"
#include "app_698client.h"
#include "app_dltpro.h"
#include "app_sysdate.h"
#include "app_meter_verify.h"
#include "app_clock_os_ex.h"
#include "netnnib.h"
#include "scanbandmange.h"
#include "app_base_serial_cache_queue.h"
#include "app_cnf.h"
#include "app_clock_sta_timeover_report.h"
#include "app_phaseseq_judge_sta.h"
#include "app_read_cjq_list_sta.h"


#if defined(STATIC_NODE)

EventReportStatus_e    g_StaEventReportStatus = e_3762EVENT_REPORT_ALLOWED;
ostimer_t *exstate_verify_timer = NULL;// �ⲿ״̬��鶨ʱ�� 
ostimer_t *exstate_reset_timer = NULL;// �ⲿ״̬�����λ��ʱ��
ostimer_t *exstate_event_timer = NULL;// �ⲿ״̬�¼��ϱ���ʱ��
ostimer_t *event_report_timer = NULL;

EventReportSave_t event_report_save=
{
    .check = 0,
	.Oop20flag = 0,
	.flag = 0,
	.len = 0,
	.outlflag = 0,
	.cnt = 0,
	.FilterRepeat = 0,
	.event_report_status = e_3762EVENT_REPORT_ALLOWED,
};                      // �¼��ϱ���־���ϱ����ݱ���


EventApsSeq_t aps_event_seq =
{
    .MeterEventSeq          = 0xFFFF,
    .PowerSTATransEventSeq  = 0xFFFF,
    .PowerOnEventSeq        = 0xFFFF,
    .SuspectPowerOffEventSeq= 0xFFFF,
    .ElecEventSeq           = 0xFFFF,
    .EventReportClockOverSeq= 0xFFFF,
};

StaReportCnt_t sta_report_cnt = 
{
    .PowerSTATransCnt   = 0,
    .PowerOnSTACnt      = 0,
    .PowerOffCnt        = 0,
};

ModuleState_t module_state =
{
    .plug_state = e_plug_in,
    .vol_detect_state = e_vol_normal,
    .zero_cross_state = e_zc_losting,
    .soft_reset_state = e_reset_disable,
};  


/*************************************************************************
 * �������� :	BitMapTrans(U8 *SrcBitMap , U8 *SrcBitMapLen ,U16 *StartIndex , U8 *RepeatBitMap , U8 *DstBitMap )
 * ����˵�� :	BitMapTrans����ת��
 * ����˵�� :	SrcBitMap - ԭλͼ��SrcBitMapLen-ԭλͼ���ȣ�StartIndex-ԭλͼ��ʼλ��
 *					      RepeatBitMap	- ����λͼ
 *					      DstBitMap - �ϲ���Ŀ��λͼ
 * ����ֵ		:	5 & 0 Ϊ�ط����������ͣ����������淢��
 *************************************************************************/
static U8  bit_map_trans(U8 *SrcBitMap , U16  SrcBitMapLen ,U16 StartIndex , U8 *RepeatBitMap , U8 *DstBitMap )
{
    U16 ii ;
    U16 reportcnt=0 ;
    
    for(ii=0; ii<(SrcBitMapLen)*8; ii++)
    {
        if(SrcBitMap[ii/8] & (0x01 << ii%8))
        {
            if(RepeatBitMap[(ii+StartIndex)/8]& (0x01 << ((ii+StartIndex)%8)))
            {
                        //ȥ��
            }
            else
            {
                DstBitMap[(ii+StartIndex)/8]|= (0x01 << ((ii+StartIndex)%8));
                reportcnt ++;
            }
        }
    }
    if(reportcnt>0)
    {
        return 6;
    }
    else
    {
        return 0;
    }

}

static void power_event_info_deal(EVENT_REPORT_IND_t *event_report_ind)
{
    U8 len , cnt=0;
	U8 powereventtype;
	U16 startTEI;
	powereventtype = event_report_ind->EventData[0];
	if(powereventtype == e_BITMAP_EVENT_POWER_OFF)  // λͼͣ���ϱ�
	{
        startTEI = (event_report_ind->EventData[1]) | event_report_ind->EventData[2]<<8;
	    app_printf("startTEI = %d map->",startTEI);
	    //dump_buf(PowerOffStaReportBitMapBuff,PowerOffBitMapLen);
	    
	    if(startTEI != 0 && event_report_ind->EventDataLen - 3 != 0)
	    {
	        len = event_report_ind->EventDataLen - 3;
            U8  *StaBitMapBuffTemp = zc_malloc_mem(PowerOffBitMapLen, "StaBitMapBuffTemp", MEM_TIME_OUT);//STAȥ��λͼ�������ϱ�λͼ��ȥ��λͼ
            if(StaBitMapBuffTemp)
            {
                //���ȥ��
				bit_map_trans(sta_power_off_report_bitmap_buff , PowerOffBitMapLen , 0 , StaBitMapBuffTemp , StaBitMapBuffTemp);
                bit_map_trans(sta_power_off_bitmap_buff , PowerOffBitMapLen , 0 , StaBitMapBuffTemp , StaBitMapBuffTemp);
		        cnt=bit_map_trans(&event_report_ind->EventData[3] , len , startTEI,StaBitMapBuffTemp, sta_power_off_report_bitmap_buff);
                printf_s("%s get a map :",__func__);
                dump_printfs(&event_report_ind->EventData[3],len);
		        zc_free_mem(StaBitMapBuffTemp);
				if(cnt >0)
				{
					aps_event_seq.PowerSTATransEventSeq = ++ApsEventSendPacketSeq;
				}
		        sta_report_cnt.PowerSTATransCnt = MAX(cnt,sta_report_cnt.PowerSTATransCnt);
		        app_printf("STATransCnt : %d,Seq = %d\n",sta_report_cnt.PowerSTATransCnt,ApsEventSendPacketSeq);
            }
		}
        if(sta_report_cnt.PowerSTATransCnt > 0 && TMR_RUNNING != zc_timer_query(clean_bitmap_timer))
        {
            //timer_start(clean_bitmap_timer);
            modify_sta_clean_bitmap_timer(STA_CLEAN_BITMAP*TIMER_UNITS);
        }
    }
	//CJQ2 Power off BitMap Trans
	else if(powereventtype == e_ADDR_EVENT_POWER_OFF)  // ��ַͣ���ϱ�
	{
		++ApsEventSendPacketSeq;
		if(ApsEventSendPacketSeq == 0)
		{
			++ApsEventSendPacketSeq;
		}

        if(check_power_off_info(ApsEventSendPacketSeq,event_report_ind->MeterAddr))
        {
            app_printf("send other cjq power off\n");
            power_off_event_report(e_CJQ2_ACT_CCO,e_ADDR_EVENT_POWER_OFF,e_UNICAST_FREAM,event_report_ind->EventData ,event_report_ind->EventDataLen,ApsEventSendPacketSeq);
        }


		//power_off_event_report(e_CJQ2_ACT_CCO,e_ADDR_EVENT_POWER_OFF,e_UNICAST_FREAM,event_report_ind->EventData ,event_report_ind->EventDataLen,++ApsEventSendPacketSeq);
	}
	return;
}


static void event_sta_event_report_ind(EVENT_REPORT_IND_t *pEventReportInd)
{

    app_printf("FunCode : %dDirect %d\n",pEventReportInd->FunCode,pEventReportInd->Direct);

    // ͣ�����¼�����
    if((e_STA_ACT_CCO == pEventReportInd->FunCode||e_CJQ2_ACT_CCO == pEventReportInd->FunCode ) && pEventReportInd->Direct == e_UPLINK)//&&pEventReportInd->EvenData.pPayload[0]==e_STA_EVENT_POWER_OFF)
    {
        U16  delayTime;
		
        //STA Power off BitMap Trans
        power_event_info_deal(pEventReportInd);
        
		//Power off report and clean Timer refresh
		#if defined(ZC3750STA)
		if(DevicePib_t.PowerOffFlag == e_power_off_now)  // 
		// ��ǰ�ڵ�Ҳͣ�磬�ϲ���λͼ��ˢ���ϱ���ʱ��
        {
            if(timer_remain(sta_power_off_send_timer) > 2*TIMER_UNITS)
            {
                delayTime = 2*TIMER_UNITS;
                modify_power_off_send_timer(delayTime);
            }
        }
        #endif
        if(TMR_STOPPED==zc_timer_query(sta_power_off_send_timer))  
		{
			if(sta_report_cnt.PowerSTATransCnt > 0)
			{
        	    g_PowerEventTranspondFlag=TRUE;   // �м�ڵ�ת����־
        	    //�㼯10�룬���5��
                delayTime = rand()%(5*TIMER_UNITS)+(10*TIMER_UNITS);
                
                app_printf("delayTime : %dms\n",delayTime);
                modify_power_off_send_timer(delayTime);
                
			}
        }  
    }
    else if(e_CCO_CNF_STA==pEventReportInd->FunCode)  // CCO��STA��ȷ��
    {
    	app_printf("FunCode = %d save_t.flag = %d  %d\n",pEventReportInd->FunCode,event_report_save.Oop20flag,DevicePib_t.PowerEventReported);
		dump_buf(nnib.MacAddr,6);
		dump_buf(pEventReportInd->DstMacAddr,6);
		dump_buf(pEventReportInd->SrcMacAddr,6);
        app_printf("STATrans = %d OnSeq = %d MeteSeq = %d\n",aps_event_seq.PowerSTATransEventSeq,aps_event_seq.PowerOnEventSeq,aps_event_seq.MeterEventSeq);

		if(aps_event_seq.PowerOnEventSeq == pEventReportInd->PacketSeq )
		{
		    app_printf("PowerOffFlag = %d\n",DevicePib_t.PowerOffFlag);
            
			if(DevicePib_t.PowerEventReported ==TRUE && DevicePib_t.PowerOffFlag == e_power_on_now)//����ʱ����ȷ��֡�Ĳ���
			{
	    		recive_power_on_cnf();
	            return;
			}
        }
        else if(aps_event_seq.PowerSTATransEventSeq == pEventReportInd->PacketSeq )
        {
			if(g_PowerEventTranspondFlag ==TRUE && DevicePib_t.PowerOffFlag != e_power_off_now)//ת��ͣ��ʱ���յ�ȷ��֡�Ĳ���
			{
				sta_report_cnt.PowerSTATransCnt = 0;
				//���ȥ��
				bit_map_trans(sta_power_off_report_bitmap_buff , PowerOffBitMapLen , 0 , sta_power_off_bitmap_buff , sta_power_off_bitmap_buff);
				app_printf("Recive Confirm PBMBuff : ");
				//upgrade_show_bit_map(sta_power_off_bitmap_buff,MAX_WHITELIST_NUM); 
		    }
	    }
#if defined(ZC3750STA)
        else if(aps_event_seq.EventReportClockOverSeq == pEventReportInd->PacketSeq )
        {
            timer_stop(EventReportClockOvertimer, TMR_CALLBACK);
            memset(event_report_clockover_save_t.buf, 0x00, sizeof(event_report_clockover_save_t.buf));
            event_report_clockover_save_t.len = 0;
            U32 NowTime = os_rtc_time()/(24*60*60);
            NowTime *= (24*60*60);
            DevicePib_t.TimeReportDay = NowTime;
            staflag = TRUE;
        }
#endif
		if(aps_event_seq.MeterEventSeq == pEventReportInd->PacketSeq)  // ���¼��ϱ�
		{
#if defined(ZC3750STA)
			event_report_save.event_report_status = e_3762EVENT_REPORT_ALLOWED;
			event_report_save.Oop20flag = 0;
			event_report_save.flag = 0;
			event_report_save.len = 0;
			timer_stop(event_report_timer,TMR_NULL);
            if(PROVINCE_HENAN == app_ext_info.province_code)
            {
                modify_event_report_timer(60*60*TIMER_UNITS);//�յ�ȷ�ϣ�120�������ϱ�ʱ�䣬��IO���ͣ�����30��̫�죬����Ҫ��1Сʱ
                //����ǿ��1Сʱֻ�ϱ�һ���¼�
                event_report_save.cnt = 0;
                event_report_save.FilterRepeat = 0;
            }
            else
            {
                modify_event_report_timer(120*TIMER_UNITS);//�յ�ȷ�ϣ�120�������ϱ�ʱ�䣬��IO���ͣ�����30��̫�죬���������ʵ���һЩ
            }
			event_report_save.outlflag = FALSE;
#endif
        }
#if defined(ZC3750STA)
        else if(Meter_3_EventSendPacketSeq == pEventReportInd->PacketSeq)
        {
            if(PROVINCE_HEBEI == app_ext_info.province_code)
            {

                app_printf("Receive MeterPhase Report Answer FromCCO seq = %d\n", Meter_3_EventSendPacketSeq);
                sta_phaseseq_create_event_timer(15 * 60 * TIMER_UNITS);

            }
            else
            {
                app_printf("Error pro!! seq = %d\n", Meter_3_EventSendPacketSeq);
            }
        }
#endif
		else
		{
#if defined(ZC3750CJQ2)
             SetPowerOffInfoFlag(pEventReportInd->PacketSeq);
#endif
		}
    }

#if defined(ZC3750STA)

    else if(e_CCO_ISSUE_PERMIT_STA_REPORT==pEventReportInd->FunCode)
    {
        event_report_save.event_report_status = e_3762EVENT_REPORT_ALLOWED;
		if(event_report_timer)
		{
			if(TMR_STOPPED==zc_timer_query(event_report_timer))
			{
				timer_start(event_report_timer);		
			}
		}
    }
    else if(e_CCO_ISSUE_FORBID_STA_REPORT==pEventReportInd->FunCode)
    {			
    	event_report_save.Oop20flag = 0;
		event_report_save.len = 0;
        event_report_save.flag =0;
        event_report_save.event_report_status = e_3762EVENT_REPORT_FORBIDEN;
		event_report_save.outlflag = FALSE;
		if(event_report_timer)
		{
			if(0 != timer_stop(event_report_timer, TMR_NULL))
            {
                app_printf("stop eventr f\n");
            }
		}
    }
    else if(e_CCO_ACK_BUF_FULL_STA==pEventReportInd->FunCode)
    {
        event_report_save.event_report_status = e_3762EVENT_REPORT_BUFF_FULL;
        if(event_report_save.Oop20flag == 0)
        {
		    event_report_save.flag = 1;
        }
        timer_stop(event_report_timer,TMR_NULL);
        modify_event_report_timer(30*TIMER_UNITS);

        if(PROVINCE_HEBEI == app_ext_info.province_code)
        {
            sta_phaseseq_create_event_timer(30 * TIMER_UNITS);
        }
	}	
#endif

    return;
}
/* ֱ������STAλͼ */
void sta_clean_bitmap(void)
{
    memset(sta_power_off_bitmap_buff, 0x00, PowerOffBitMapLen);
    memset(sta_power_off_report_bitmap_buff, 0x00, PowerOffBitMapLen);
    g_PowerEventTranspondFlag = FALSE;
    if ((sta_report_cnt.PowerOnSTACnt) == 0)
    {
        timer_stop(sta_power_off_send_timer, TMR_NULL);
    }
    else
    {
        app_printf("OnSTACnt %d\n", sta_report_cnt.PowerOnSTACnt);
    }
    app_printf("cleanbitmaptimerCB   ---------\n");
}

static void sta_clean_bitmap_timer_proc(work_t *work)
{
    if((sta_report_cnt.PowerSTATransCnt) > 0)  // ����ϱ�����δ������ˢ����λͼ��ʱ��
    {
        modify_sta_clean_bitmap_timer(sta_report_cnt.PowerSTATransCnt*3*1000);
    }
    else  // ������λͼ��תͣ��ת����־Ϊfalse��ͣ�ϱ����Ͷ�ʱ��
    {
		sta_clean_bitmap();
    }

    return;
}


static void exstate_verify_proc(work_t *work)
{
    U32 modifttimes = 0;
    static U32 timenum = 0;
    static U8  PowerOffState = 0;       //ͣ��󸴵磬��Ҫ�ָ����ʣ���ҪУ��20�������


    app_printf("Exstate verity proc.\n");

    //���粻���ڶ�����
    if((nnib.PowerType == TRUE) && module_state.zero_cross_state == e_zc_lost)//
    {
        if(module_state.vol_detect_state == e_vol_normal)
        {
            modifttimes = MAX(modifttimes,1500);    
        }

    }
    
    if(module_state.power_type == e_with_power && module_state.plug_state == e_plug_out)  // ģ��γ�
    {
    
#if defined(ZC3750STA)
        event_report_save.check = e_event_none;  // ���¼�
#endif
        module_state.vol_detect_state = e_vol_normal;
        module_state.soft_reset_state = e_reset_disable;
        module_state.zero_cross_state = e_zc_losting;
         
        return;
    }
    
    timenum += modifttimes;
    if(timenum >= 2000)
    {
        modifttimes = 0;
        timenum = 0;
    }
    
    if(modifttimes == 0)//������
    {
        app_printf("nnib.PowerType = %d\n", nnib.PowerType);
        app_printf("ModuleState.zero_cross_state = %d\n", module_state.zero_cross_state);
        app_printf("ModuleState.plug_state = %d\n", module_state.plug_state);
        app_printf("DevicePib_t.PowerOffFlag = %d\n", DevicePib_t.PowerOffFlag);
        // ͣ���ϱ����
        if(
        #if defined(ZC3750STA)
            module_state.vol_detect_state == e_vol_loss && 
        #endif

                module_state.zero_cross_state == e_zc_lost 
                 && module_state.plug_state == e_plug_in && DevicePib_t.PowerOffFlag != e_power_off_now)
        {
            if(module_state.power_type == e_without_power)
            {
                return;
            }
            //power off
            module_state.soft_reset_state = e_reset_disable;  // �����λȥʹ��
            //DevicePib_t.PowerOffFlag = e_power_off_now;         // ͣ��״̬�Ϸ�

            PowerOffState = 1;//�����ϵ磬����ͣ��

            timer_start(sta_power_off_report_timer);  //
	#if defined(ZC3750STA)
			//staflag = TRUE; //STA���ͣ���⣬�ɼ�����Ҫ�ڴ˺������ɲ���д
			//WriteMeterInfo();
			
	#endif
            module_state.zero_cross_state = e_zc_losting; 
            app_printf("ExStateVerify,start poweroffreporttimer!\n");
        }
        // �����ϱ�
        else if((nnib.PowerType == TRUE) && module_state.zero_cross_state == e_zc_get 
                     && module_state.plug_state == e_plug_in && DevicePib_t.PowerOffFlag == e_power_off_last)//
        {
            if(module_state.power_type == e_without_power)
            {
                return;
            }
            //power on
            //DevicePib_t.PowerOffFlag = e_power_on_now;   // ����״̬�Ϸ�
            DevicePib_t.PowerEventReported = TRUE;
#if defined(ZC3750STA)
            if(sta_bind_info_t.PrtclOop20 == 1)
            {
                
                //��ȷʱ��Ҫ�󣬵�һ�θ��粻����
                
                if(PowerOffState == 1)//20�汾���Э�̳ɹ����ٴλָ�
                {
                    //MeterUartInit(g_meter, BAUDRATE_9600);
                    //׼������20�汾�����֤
                    //StartMeterCheck20Ed();
                    StartMeterCheckManage(VerifyZero_e);
                    app_printf("power on and StartMeterCheck20Ed now\n");
                }
                
            }
#endif
            if(PowerOffState == 1)//ͣ��δ����磬����ָ�ԭ�ȹ���
    		{
    			PowerOffState = 0;
    			changepower(HPLC.band_idx);
    		}
            timer_start(sta_power_on_report_timer);

            app_printf("ExStateVerify,start poweronreporttimer!\n");
        }
        else if(zc_timer_query(sta_power_off_report_timer)== TMR_RUNNING && module_state.zero_cross_state == e_zc_get)//����ָ���ͣ���ж�ʧЧ
        {
            timer_stop(sta_power_off_report_timer,TMR_NULL);
            app_printf("else!\n");
            if (zc_timer_query(event_report_timer) != TMR_RUNNING)
            {
                app_printf("e_zc_get_start_event_timer\n");
                modify_event_report_timer(5 * TIMER_UNITS);
            }
        }

        if(module_state.zero_cross_state != e_zc_lost)//�ж�ʱ�䵽�����㻹δ��ʧ����������Ҫ��һ��12V������д���
        {
            module_state.vol_detect_state = e_vol_normal;//12V��������
        }
        
        return;
        
    }
    
    modify_exstate_verify_timer(modifttimes);

    return;
}

static void event_check(void)
{
#if defined(ZC3750STA)

    if(getHplcTestMode() != NORM || FactoryTestFlag == TRUE)
	{
		return;
	}
	
	if(sta_bind_timer)
	{
		if(TMR_RUNNING == zc_timer_query(sta_bind_timer))
		{
			return;
		}
	}
    #ifdef TH2CJQ
    //IOT_Cģ�鲻�ù���20�����¼���ֱ���ϱ�
    app_printf("IOT_C event_report\n");
    #else
    //��20�汾�������20�������¼��ϱ��رյģ��������¼��ϱ�����
    if(sta_bind_info_t.PrtclOop20 == 0 || (1 == sta_bind_info_t.PrtclOop20 && 1 != app_ext_info.func_switch.oop20EvetSWC))
    #endif
    {
        //event_report_save_t.Oop20flag = 0;	
        event_report_save.check = e_event_get;
        app_printf("*sta_event_report\n");
        /*if(TMR_STOPPED == zc_timer_query(exstate_verify_timer))
        {      
    	    timer_start(exstate_verify_timer);
        }*/
        if(TMR_STOPPED == zc_timer_query(exstate_event_timer))
        {      
    	    timer_start(exstate_event_timer);
        }
        
    }
#endif
}



__isr__ void sta_event_report(void *data)
{
    app_printf("event interrupt\n");
    event_check();
    
    return;
}


static void event_up_report_send(U8 resendFlag)
{
    EVENT_REPORT_REQ_t *pEventReportRequest_t = NULL;
	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);    
    pEventReportRequest_t = zc_malloc_mem(sizeof(EVENT_REPORT_REQ_t) + event_report_save.len,"EventReportRequest",MEM_TIME_OUT);

    pEventReportRequest_t->Direction = e_UPLINK;
    pEventReportRequest_t->PrmFlag   = 1;
    pEventReportRequest_t->FunCode   = e_METER_ACT_CCO;
	
	#if defined(ZC3750STA)
    __memcpy(pEventReportRequest_t->MeterAddr,DevicePib_t.DevMacAddr,6);
    __memcpy(pEventReportRequest_t->SrcMacAddr,DevicePib_t.DevMacAddr,6);
	#elif defined(ZC3750CJQ2)
    __memcpy(pEventReportRequest_t->MeterAddr,CollectInfo_st.CollectAddr,6);
    __memcpy(pEventReportRequest_t->SrcMacAddr,CollectInfo_st.CollectAddr,6);
	#endif
    __memcpy(pEventReportRequest_t->DstMacAddr, ccomac, 6);
         
    pEventReportRequest_t->PacketSeq =  resendFlag == TRUE ? ApsEventSendPacketSeq : ++ApsEventSendPacketSeq;//�ط�
    aps_event_seq.MeterEventSeq = pEventReportRequest_t->PacketSeq;

    pEventReportRequest_t->EvenDataLen = event_report_save.len;
    __memcpy(pEventReportRequest_t->EventData, event_report_save.buf, event_report_save.len);

    ApsEventReportRequest(pEventReportRequest_t);

    zc_free_mem(pEventReportRequest_t);
    
    return;
}

U8 Check698EventReport(U8* pDatabuf, U16 dataLen)
{
    dl698frame_header_s  *pFrameHeader = NULL;
    U8                   *pApdu = NULL;
    U16                   ApduLen = 0;

    U8  dstAddr[6] = {0};

    if(TRUE != ParseCheck698Frame(pDatabuf, dataLen, &pFrameHeader, &pApdu, dstAddr, &ApduLen))
    {
        return FALSE;
    }
    app_printf("Check698EventReport:%d %d %d\n",pFrameHeader->CtrlField.DirBit,
    pFrameHeader->CtrlField.PrmBit,pFrameHeader->CtrlField.FuncCode);
    if(pFrameHeader->CtrlField.DirBit == 1 && pFrameHeader->CtrlField.PrmBit == 0
        && pFrameHeader->CtrlField.FuncCode == e_APP_DATA)
    {
        DL698_event_type_s EventType = {0};
        if(TRUE == ServerReport698FrameProc(pDatabuf,dataLen,(U8 *)&EventType))
        {
            //��Ҫ��ȫ���������¼��ϱ�
            if(EventType.OI == 0x3011 && EventType.RESULT == 1)
            {
                //�����¼��ϱ���ֱ���ϱ����ر�ģ��ĸ����ϱ�
                app_printf("EventType is 0x3011\n");
                return TRUE;
            }
            else if(EventType.OI == 0x3320 && EventType.RESULT == 1)
            {
                //�¼��ϱ���ֱ���ϱ�����������
                app_printf("EventType is 0x3320\n");
                return TRUE;
            }
        }
    }

    return FALSE;
}


U8 StaReadEventMatch(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }

    return result;
}

 void StaReadEventTimeout(void *pMsgNode)
{
    U8 EventData[18]= {0x34,0x48,0x33,0x37,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0xDD,0XDD};

    event_report_save.len = 0;
    Packet645Frame(event_report_save.buf, &event_report_save.len, DevicePib_t.DevMacAddr,
                                             0x91,EventData,sizeof(EventData));

    event_up_report_send(FALSE);

    return;
}

 void StaReadEventRecvDeal(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    U8 UpCode;             

    if(Check645Frame(revbuf, revlen, NULL, NULL, &UpCode)==TRUE)             
    {                 
        if(UpCode == 0xD1)                 
        {      
            app_printf("eventout high but UpCode is 0xD1!\n");                 
            return;    
        }            
    } 

    event_report_save.len = revlen;
    __memcpy(event_report_save.buf, revbuf, revlen);

    event_up_report_send(FALSE);

    return;
}
void Sta20MeterRecvDeal(uint8_t *revbuf, uint16_t revlen)
{
#if defined(ZC3750STA)

	//�����¼������ж�
    if(Check698EventReport(revbuf, revlen) == TRUE)// && 1 == app_ext_info.func_switch.oop20EvetSWC)
    {
    	
        app_printf("event 20METER!\n");
    }
    else
    {
        app_printf("err event 20METER!\n");
        return;
    }

    event_report_save.len = revlen;
    __memcpy(event_report_save.buf, revbuf, revlen);
    event_report_save.cnt = EVENTREPORTCNT;
    event_report_save.Oop20flag = 1;	//ÿ���ϱ��������
    if(PROVINCE_HENAN == app_ext_info.province_code)
    {
        sta_bind_info_t.Has20Event = 0; //���ϲ�֧��20�����ϱ�
    }
    else
    {
	    sta_bind_info_t.Has20Event = 1;		//�ж��Ƿ�֧��20�¼��ϱ��������
    }
    /*20�汾�������ͬһ�¼�ֻ�ϱ����Σ�����ͨ���崥�����¼���һ����������һ�������ظ�����������*/
    timer_stop(event_report_timer,TMR_NULL);
    
    modify_event_report_timer(1*TIMER_UNITS); //1S��ֱ���ϱ�
    
    return;
#endif
}


static void sta_read_meter_event(void)
{
#if defined(ZC3750STA)
    app_printf("Event prtcl %d type %d\n",DevicePib_t.Prtcl,DevicePib_t.MeterPrtcltype);

    // ����Ƿ�Ϊ����ʡ
    const U8 is_henan_province = (PROVINCE_HENAN == app_ext_info.province_code);

        // �ж��Ƿ�Ϊ����ʡ����Э������Ϊ DLT698 ������ DoublePrtcl_698_e ���� DoublePrtcl_e
    if ((is_henan_province && ((DLT698 == DevicePib_t.Prtcl && SinglePrtcl_e == DevicePib_t.MeterPrtcltype) 
                        || (DoublePrtcl_698_e == DevicePib_t.MeterPrtcltype || DoublePrtcl_e == DevicePib_t.MeterPrtcltype)))
        || (!is_henan_province && (DLT698 == DevicePib_t.Prtcl)))
    {
        if(event_report_save.Oop20flag == 1)	//20�����¼�
        {
			if(1 == app_ext_info.func_switch.oop20EvetSWC) //20�����¼��ϱ����ؿ���
			{
	            event_up_report_send(FALSE);
			}
			else
			{
				//StaReadEventTimeout�����len��
				StaReadEventTimeout(NULL);
			}
		}
		else
		{
            #ifdef TH2CJQ
            app_printf("iot event report\n");
            #else
			if(sta_bind_info_t.PrtclOop20 == 1 || sta_bind_info_t.Has20Event == 1) //20�����¼��ϱ����ؿ���
			{
				app_printf("oop20EvetSWC=%d,Has20Event=%d\n",sta_bind_info_t.PrtclOop20,sta_bind_info_t.Has20Event);
			}
			else
            #endif
			{
				StaReadEventTimeout(NULL);
			}
		}
    }
    // �ж��Ƿ�Ϊ����ʡ����Э������Ϊ DLT645_2007 ����Ϊ SinglePrtcl_e
    else if ((is_henan_province && ((DLT645_2007 == DevicePib_t.Prtcl && SinglePrtcl_e == DevicePib_t.MeterPrtcltype)
                || (DoublePrtcl_645_e == DevicePib_t.MeterPrtcltype)))
            || (!is_henan_province && (DLT645_2007 == DevicePib_t.Prtcl)))
    {
        //�޸��¼��ϱ���������ʱ��645Э�飬ֱ���ϱ���һ�ε��¼�����
        if(event_report_save.flag == 1 && event_report_save.len > 0)
        {
            event_up_report_send(FALSE);
            return;
        }
        U8 *sendbuf = zc_malloc_mem(100, "sendbuf", MEM_TIME_OUT);
        U8 frameLen = 0;
        U8 EventDataId[4] = {0X34, 0X48, 0X33, 0X37};
        add_serialcache_msg_t serialmsg;
        memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));

        Packet645Frame(sendbuf, &frameLen, DevicePib_t.DevMacAddr, 0x11 ,EventDataId,sizeof(EventDataId));

        dump_buf(sendbuf, frameLen);

        serialmsg.Uartcfg = NULL;
        serialmsg.EntryFail = NULL;
        serialmsg.SendEvent = NULL;
        serialmsg.ReSendEvent = NULL;
        serialmsg.MsgErr = NULL;

        serialmsg.Match  = StaReadEventMatch;
        serialmsg.Timeout = StaReadEventTimeout;
        serialmsg.RevDel = StaReadEventRecvDeal;

        serialmsg.Protocoltype = DLT645_2007;
	    serialmsg.DeviceTimeout = 120;  // 1200ms
	    serialmsg.ack = TRUE;
	    serialmsg.FrameLen = frameLen;
	    serialmsg.FrameSeq = 0;
	    serialmsg.lid = e_Serial_Event;
	    serialmsg.ReTransmitCnt = 1;
        serialmsg.pSerialheader = &SERIAL_CACHE_LIST;

	    extern ostimer_t *serial_cache_timer;
        serialmsg.CRT_timer = serial_cache_timer;
                                        
	    serial_cache_add_list(serialmsg, sendbuf,TRUE);
        zc_free_mem(sendbuf);
    }

    return;
#endif
}


static void event_report_proc(work_t *work)
{    
    if(FactoryTestFlag == 1)
    {
    	//����ģʽ���ϱ�
        return;
    }
#if defined(ZC3750STA)
	if(JudgeMeterCheckStateNotPlug() == FALSE)
	{
		//�Ȳ�β��ϱ��¼�
		return;
	}
#endif
    //�����¼�Դ
    if(!READ_EVENT_PIN && event_report_save.flag != 1 && event_report_save.Oop20flag != 1 && event_report_save.outlflag == FALSE)
    {
        event_report_save.cnt = 0;
        event_report_save.FilterRepeat = 0;
        return;
    }

    //��ֹ״̬�²��ϱ��¼�
    if(event_report_save.event_report_status == e_3762EVENT_REPORT_FORBIDEN)
    {
        event_report_save.Oop20flag = 0;
        event_report_save.outlflag = FALSE;
        return;
    }
    
    //ģ��û������
    if(AppNodeState != e_NODE_ON_LINE)
    {
        event_report_save.outlflag = TRUE;
        modify_event_report_timer(5*TIMER_UNITS);
        return;
    }

    //ͣ������в��ϱ���ͨ�¼�
    if(event_report_save.Oop20flag == 0 && module_state.zero_cross_state != e_zc_get && nnib.PowerType == 1)
    {
        return;
    }

    event_report_save.outlflag = FALSE;
    app_printf("er.cnt= %d\n", event_report_save.cnt);

    if(event_report_save.cnt > 1)
    {
        sta_read_meter_event();
        modify_event_report_timer(75*TIMER_UNITS);
        event_report_save.cnt --;
    }
    else if(event_report_save.cnt == 1)
    {
        event_report_save.cnt --;
        event_report_save.flag = 0;
        event_report_save.Oop20flag = 0;
        if(PROVINCE_HENAN == app_ext_info.province_code)
        {
            modify_event_report_timer(1*TIMER_UNITS);
            event_report_save.FilterRepeat = 1;
        }
        else
        {
            modify_event_report_timer(5*TIMER_UNITS);
            event_report_save.FilterRepeat = (30*60/5);
        }
    }
    else
    {
        if(PROVINCE_CHONGQING == app_ext_info.province_code)
        {
            timer_stop(event_report_timer, TMR_NULL);
        }
        else
        {
            if(READ_EVENT_PIN)
            {
                if(event_report_save.FilterRepeat > 0)
                {
                    event_report_save.FilterRepeat --;
                    if(PROVINCE_HENAN == app_ext_info.province_code)
                    {
                        modify_event_report_timer(60*60*TIMER_UNITS);
                    }
                    else
                    {
                        modify_event_report_timer(5*TIMER_UNITS);
                    }
                }
                else
                {
                    event_check();
                }
            }
            else
            {
                event_report_save.FilterRepeat = 0;
            }
        }
    }
}


static void event_report_timerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"ExStateVerifyTimerCB",MEM_TIME_OUT);
	work->work = event_report_proc;
    work->msgtype = EVT_PROC;
	post_app_work(work);    
}


int8_t  event_report_timer_init(void)
{
#if defined(ZC3750STA)
    int32_t ret;
    ret=gpio_request_irq(GPIO_06_MUXID, GPIO_INTR_TRIGGER_MODE_POSEDGE, sta_event_report, NULL);
    app_printf("gpio_request_irq %s\n", ret == OK ? "OK" : "ERROR");
#endif

    APP_UPLINK_HANDLE(sta_event_report_hook, event_sta_event_report_ind);
    
    if(event_report_timer == NULL)
    {
        event_report_timer = timer_create(1200,
                                        0,
                                        TMR_TRIGGER ,//TMR_TRIGGER
                                        event_report_timerCB,
                                        NULL,
                                        "eventreporttimerCB"
                                        );
    }

    return 0;
}



void modify_event_report_timer(U32 MStimes)
{
    if(event_report_timer)
    {
        if(TMR_STOPPED==zc_timer_query(event_report_timer))
        {
            timer_modify(event_report_timer,
                            MStimes,
                            0,
                            TMR_TRIGGER, //TMR_TRIGGER
                            event_report_timerCB,
                            NULL,
                            "eventreporttimerCB",
                            TRUE);
            app_printf("Start event timer %d s.\n",MStimes/1000);
            timer_start(event_report_timer);
        }
    }
    else
        sys_panic("<eventreporttimer fail!> %s: %d\n");

    return;
}



static void sta_clean_bitmap_timerCB(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"ExStateVerifyTimerCB",MEM_TIME_OUT);
	work->work = sta_clean_bitmap_timer_proc;
    work->msgtype = STA_CLBITPROC;
	post_app_work(work);
}



U8 sta_clean_bitmap_timer_init(void)
{
    if(clean_bitmap_timer == NULL)
    {
        clean_bitmap_timer = timer_create(STA_CLEAN_BITMAP*TIMER_UNITS,
                                          0,
                                          TMR_TRIGGER ,//TMR_TRIGGER
                                          sta_clean_bitmap_timerCB,
                                          NULL,
                                          "sta_clean_bitmap_timerCB"
                                          );
    }
    
    return TRUE;
}


void modify_sta_clean_bitmap_timer(U32 MStimes)
{
    if(clean_bitmap_timer)
    {
        if(TMR_STOPPED==zc_timer_query(clean_bitmap_timer))
        {
            timer_modify(clean_bitmap_timer,
                            MStimes,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            sta_clean_bitmap_timerCB,
                            NULL,
                            "sta_clean_bitmap_timerCB",
                            TRUE);
            timer_start(clean_bitmap_timer);
        }
    }
    else
        sys_panic("<clean_bitmap_timer fail!> %s: %d\n");

}







static void exstate_verify_timerCB(struct ostimer_s *ostimer, void *arg)
{ 
	exstate_verify_proc(NULL);
}





int8_t  exstate_verify_timer_init(void)
{
#if defined(ZC3750STA)
    app_printf("exstate verify timer init.\n");
    #if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)
    //int32_t ret;
    //extern void poweroffdetect(void *data);
	//ret=gpio_request_irq(POWEROFF_MAP, GPIO_INTR_TRIGGER_MODE_NEGEDGE, poweroffdetect, NULL);
	//printf_s("poweroffdetect gpio_request_irq %s\n", ret == OK ? "OK" : "ERROR");
    #endif

#endif
    
    if(exstate_verify_timer == NULL)
    {
        exstate_verify_timer = timer_create(
                            1500	,
                            30,
							TMR_TRIGGER,
                            exstate_verify_timerCB,
                            NULL,
                            "exstate_verify_timerCB"
                           );

    }

    return 0;
}


void modify_exstate_verify_timer(U32 ms_time)
{
    if(ms_time > 0)
    {
        timer_modify(exstate_verify_timer,
			(ms_time),
            0,
            TMR_TRIGGER ,
            exstate_verify_timerCB,
            NULL,
            "exstate_verify_timerCB",
            TRUE);
    
	    timer_start(exstate_verify_timer);
    }
    else
    {
        timer_modify(exstate_verify_timer,
			1500,
            0,
            TMR_TRIGGER ,
            exstate_verify_timerCB,
            NULL,
            "exstate_verify_timerCB",
            TRUE);
    }
}

static void exstate_reset_timerCB(struct ostimer_s *ostimer, void *arg)
{
	app_printf("ModuleState.zero_cross_state = %d\n", module_state.zero_cross_state);
	app_printf("ModuleState.plug_state = %d\n", module_state.plug_state);
	app_printf("module_state.soft_reset_state = %d\n", module_state.soft_reset_state);
	app_printf("module_state.vol_detect_state = %d\n", module_state.vol_detect_state);

	if(module_state.plug_state == e_plug_out)  // ģ��γ�
	{
		module_state.soft_reset_state = e_reset_disable;
		return;
	}
    
    //ͣ������в���λ
	if(module_state.zero_cross_state != e_zc_get && nnib.PowerType == 1)
    {
        return;
    }

	// ��������λʹ�ܣ����Ҳ���ͣ��״̬����λ
    /* ��������ģʽ�²���Ӧ�����λ */
	if(module_state.soft_reset_state == e_reset_enable && module_state.vol_detect_state == e_vol_normal && FactoryTestFlag == 0)
	{
		if(zc_timer_query(sta_power_off_report_timer) != TMR_RUNNING)
		{
			//reboot is ok
			printf_s("ExStateVerify,start appreboot!\n");
			app_reboot(500);
		}
		
	}
	
	module_state.soft_reset_state = e_reset_disable;
	
	return;
}


int8_t  exstate_reset_timer_init(void)
{  
    if(exstate_reset_timer == NULL)
    {
        exstate_reset_timer = timer_create(
                            1500	,
                            30,
							TMR_TRIGGER,
                            exstate_reset_timerCB,
                            NULL,
                            "exstate_reset_timerCB"
                           );

    }

    return 0;
}

static void exstate_event_timerCB(struct ostimer_s *ostimer, void *arg)
{
	app_printf("exstate_event_timerCB.\n");
	
	if(module_state.plug_state == e_plug_out)  // ģ��γ�
	{
	
#if defined(ZC3750STA)
		event_report_save.check = e_event_none;  // ���¼�
#endif
		return;
	}
	
	app_printf("ModuleState.plug_state = %d\n", module_state.plug_state);
	app_printf("event.check : %s\n",event_report_save.check == e_event_get?"e_event_get":"un");

#if defined(ZC3750STA)
	if(event_report_save.check == e_event_get && module_state.plug_state == e_plug_in)	//ͣ�粻�ϱ���ͨ�¼�
	{
		//event is ok
		event_report_save.check = e_event_none;
		event_report_save.cnt = EVENTREPORTCNT;

		app_printf("ExStateVerify,start eventreporttimer!\n");
		modify_event_report_timer(100);
	   
	}
	event_report_save.check = e_event_none;
#endif
	return;
}



int8_t  exstate_event_timer_init(void)
{  
    if(exstate_event_timer == NULL)
    {
        exstate_event_timer = timer_create(
                            1500	,
                            30,
							TMR_TRIGGER,
                            exstate_event_timerCB,
                            NULL,
                            "exstate_event_timerCB"
                           );

    }

    return 0;
}

#endif
