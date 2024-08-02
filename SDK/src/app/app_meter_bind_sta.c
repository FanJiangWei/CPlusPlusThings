/*
 * Corp:	2018-2026 Beijing Zhongchen Microelectronics Co., LTD.
 * File:	app_led.c
 * Date:	Nov 2, 2022
 * Author:	LuoPengwei
 */


#include "Datalinkdebug.h"
#include "app_meter_common.h"
#include "app_698p45.h"
#include "app_698client.h"
#include "app_meter_verify.h"
#include "app_exstate_verify.h"
#include "app_meter_bind_sta.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "meter.h"
#include "schtask.h"
#include "dev_manager.h"
#include "app_base_serial_cache_queue.h"
#include "app_clock_sync_sta.h"
#include "app_power_onoff.h"
#include "app_read_cjq_list_sta.h"

#if defined(ZC3750STA)

static void sta_bind_ctrl(void);
static void sta_bind_timercb(struct ostimer_s *ostimer, void *arg);
static void sta_bind_put_to_list(METER_COMU_INFO_t *pMeterInfo);
static void sta_bind_success(void);

extern METER_COMU_INFO_t sta_bind_normal_t;			//�������
extern METER_COMU_INFO_t sta_bind_check698_t;		//645���698���
extern METER_COMU_INFO_t sta_bind_check645_t;       //698���645���
extern METER_COMU_INFO_t sta_bind_consult20_t;		//20�沨����Э��
extern METER_COMU_INFO_t sta_bind_check_t;			//���

U8 high_baud = FALSE;


//97��Լ�������
U8 sta_bind_frame_97[14] = {0X68, 0X99, 0X99, 0X99, 0X99, 0X99, 0X99, 0X68, 0X01, 0X02, 0X43, 0XC3, 0X6F, 0X16};
//07��Լ�������
U8 sta_bind_frame_07[12] = {0X68, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0X68, 0X13, 0X00, 0XDF, 0X16};
//698��Լ�������
U8 sta_bind_frame_698[25] = {0x68,0x17,0x00,0x43,0x45,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x00,0x5B,0x4F,0x05,0x01,0x00,0x40,0x01,0x02,0x00,0x00,0xED,0x03,0x16};

ostimer_t *sta_bind_timer = NULL;

U32  sta_bind_consult_baud = 0;			//Э�̲�����

DEVICE_METER_t sta_bind_info_t = 
{
    .MeterComuState = MeterCheckBaud_e,
    .BaudRate = 0,
    .PrtclOop20 = 0,
    .Has20Event = 0,
};


static U8 sta_bind_check_frame(U8 protocol ,U8 *data , U8 datalen ,U8 *addr)
{
	U8  FEnum = 0;
    
	if(protocol == DLT645_1997)
	{
		if(Check645Frame(data,datalen,&FEnum,addr,NULL)== FALSE)
        {
            app_printf("check645 97 err!\n");
            return ERROR;
        }     
        if((*(data + 8 + FEnum) != 0x81) || (*(data + 9 + FEnum) != 0x06))
        {
            return ERROR;
        }
        if((0X43 != *(data + 10 + FEnum)) || (0XC3 != *(data + 11 + FEnum)))
        {
            return ERROR;
        }
        __memcpy(addr , &data[1 + FEnum] , 6);
        return OK;
	}
	else if(protocol == DLT645_2007)
	{
	    if(Check645Frame(data,datalen,&FEnum,addr,NULL)== FALSE)
        {
            app_printf("check645 07 err!\n");
            return ERROR;
        }   
        if(((*(data + 8 + FEnum)&0x9f) != 0x93) && (*(data + 9 + FEnum) != 0x06))
        {
            app_printf("*(data+8) = 0x%02x!\n", *(data + 8 + FEnum));
            return ERROR;
        }
        if((*(data + 1 + FEnum) != (U8)(*(data + 10 + FEnum) - 0x33))
                || (*(data + 2 + FEnum) != (U8)(*(data + 11 + FEnum) - 0x33))
                || (*(data + 3 + FEnum) != (U8)(*(data + 12 + FEnum) - 0x33))
                || (*(data + 4 + FEnum) != (U8)(*(data + 13 + FEnum) - 0x33))
                || (*(data + 5 + FEnum) != (U8)(*(data + 14 + FEnum) - 0x33))
                || (*(data + 6 + FEnum) != (U8)(*(data + 15 + FEnum) - 0x33))
          )
        {
            app_printf("*(data+1) = 0x%02x!\n", *(data + 1 + FEnum));
            app_printf("*(data+10) -33 = 0x%02x!\n", (U8)(*(data + 10 + FEnum) - 0x33));
            return ERROR;
        }
        __memcpy(addr , &data[1 + FEnum] , 6);
        return OK;
	}
    else if(protocol == DLT698)
    {
        if(Check698Frame(data,datalen,&FEnum,addr, NULL) == FALSE)
        {
            app_printf("check698 err!\n");
            return ERROR;
        }
        if((*(data + 5 + FEnum) != *(data + 29 + FEnum))
                || (*(data + 6 + FEnum) != *(data + 28 + FEnum))
                || (*(data + 7 + FEnum) != *(data + 27 + FEnum))
                || (*(data + 8 + FEnum) != *(data + 26 + FEnum))
                || (*(data + 9 + FEnum) != *(data + 25 + FEnum))
                || (*(data + 10 + FEnum) != *(data + 24 + FEnum))
          )
        {
            app_printf("*(data+5) = 0x%02x!\n", *(data + 1 + FEnum));
            app_printf("*(data+29) = 0x%02x!\n", *(data + 10 + FEnum));
            return ERROR;
        }
        __memcpy(addr , &data[5 + FEnum] , 6);
        return OK;
    }
    else
    {
        return ERROR;
    }
}


/*************************************************************************
 * sta_bind_check_meter - 20����Э��
 * @argument:	��
 * @return��		��
 *************************************************************************/
void sta_bind_check_meter(U8* Addr, U8 Protocol, U32 BaudRate, U8 state)
{
    U8  *sendbuf = zc_malloc_mem(100, "SndBuf", MEM_TIME_OUT);
	U16 sendlen = 0;

    
    if(Protocol != DLT698)
    {
        Dlt645Encode(Addr, Protocol,sendbuf,(U8 *)&sendlen);
    }
    else
    {
        Dlt698Encode(Addr, Protocol,sendbuf,&sendlen);
    }
    
    sta_bind_check_t.protocol = Protocol ==  DLT698?DLT698:DLT645_2007;
    sta_bind_check_t.timeout = BAUDMETER_TIMEOUT;
    sta_bind_check_t.frameLen = sendlen;
    sta_bind_check_t.frameSeq = DevicePib_t.BandMeterCnt;
    sta_bind_check_t.pPayload = sendbuf;
    sta_bind_check_t.baudrate = BaudRate;;
    sta_bind_check_t.comustate = state;
    
    if(zc_timer_query(sta_bind_timer) == TMR_RUNNING)
    {
    	app_printf("sta_bind_timer is Running,not Checkmeter\n");
		zc_free_mem(sendbuf);
		return;
    }
    sta_bind_put_to_list(&sta_bind_check_t);

    zc_free_mem(sendbuf);
}

static U8 sta_bind_check645_match(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }

    return result;
}

static void sta_bind_check645_timeout(void *pMsgNode)
{
	if(SinglePrtcl_e != DevicePib_t.MeterPrtcltype)
	{
		DevicePib_t.MeterPrtcltype = SinglePrtcl_e;			//��Э���
		staflag = TRUE;
	}
	
    app_printf("sta_bind_check645Timeout BandMeter 698 success!\n");
    if (1 == app_ext_info.func_switch.oop20BaudSWC && sta_bind_info_t.BaudRate == BAUDRATE_9600)
        sta_bind_consult20_start();
    else
        sta_bind_success();
}

static void sta_bind_check645_receive(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    app_printf("sta_bind_check645RecvDeal\n");
	U8 UpCode;  
	U8 Addr[6];
    if(Check645Frame(revbuf,revlen,NULL,Addr,&UpCode)==TRUE)            
    {                 
        if(UpCode == 0xD1 || 0 != memcmp(Addr, DevicePib_t.DevMacAddr, 6))          // ����0xD1��645 ������      
        {     
            app_printf("UpCode 0xD1!\n");
			if(SinglePrtcl_e != DevicePib_t.MeterPrtcltype)
			{
				DevicePib_t.MeterPrtcltype = SinglePrtcl_e;			//��Э���
				staflag = TRUE;
			}
        }  
		else
		{
			if(SinglePrtcl_e == DevicePib_t.MeterPrtcltype)
			{
				DevicePib_t.MeterPrtcltype = DoublePrtcl_e;			//˫Э���
				staflag = TRUE;
			}
		}
    }
	else
	{
		if(SinglePrtcl_e != DevicePib_t.MeterPrtcltype)
		{
			DevicePib_t.MeterPrtcltype = SinglePrtcl_e;			//��Э���
			staflag = TRUE;
		}
	}
	//׼������20�汾�����֤
    if(1 == app_ext_info.func_switch.oop20BaudSWC && sta_bind_info_t.BaudRate == BAUDRATE_9600)
        sta_bind_consult20_start();
    else
        sta_bind_success();
}

void sta_bind_consult20_act_req(U8 *Addr, U8 *pSendbuff, U16 *pSendLen)
{
    
    OMD_s   Omd = {0xF209, 0x80, 0x0};
    OAD_s   Oad = {0xF209, 0x02, 0x00, 0xFD};
    CMMDCB_s Comdcb = {e_19200BPS, e_EVEN, e_DATA_8_BIT, e_STOP_1_BIT, e_NULL_FLUID};

    SendActionRequestNormal(Addr, &Omd, &Oad, &Comdcb, pSendbuff, pSendLen);
}


/*************************************************************************
 * sta_bind_consult20_start - 20����Э��
 * @argument:	��
 * @return��		��
 *************************************************************************/
void sta_bind_consult20_start(void)
{	
    //׼������20�汾�����֤
    U8  *sendbuf = zc_malloc_mem(100, "sta_bind_consult20_t", 40);
	U16 sendlen = 0;
    
	sta_bind_consult20_act_req(DevicePib_t.DevMacAddr,sendbuf,&sendlen);
    sta_bind_consult20_t.protocol = DLT698;
    sta_bind_consult20_t.timeout = BAUDMETER_TIMEOUT;
    sta_bind_consult20_t.frameLen = sendlen;
    sta_bind_consult20_t.frameSeq = 0;
    sta_bind_consult20_t.pPayload = sendbuf;
    sta_bind_consult20_t.baudrate = sta_bind_info_t.BaudRate;
    
    sta_bind_put_to_list(&sta_bind_consult20_t);
    zc_free_mem(sendbuf);

    sta_bind_info_t.MeterComuState = MeterCheck20Ed_e;
}

static U8 sta_bind_normal_match(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }
    if(sendprtcl == DLT645_1997&&rcvprtcl == DLT645_2007)
    {
        result = TRUE;
    }

    return result;
}
static void sta_bind_normal_timeout(void *pMsgNode)
{
    if(zc_timer_query(sta_bind_timer)==TMR_STOPPED)
    {
        sta_bind_ctrl();
    }
}

static void sta_bind_normal_recvive(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    app_printf("StaBaudMeterRecvDeal!\n");
    U8   MeterAddr[6] = {0};
    
    U8   Prtcltemp = 0;
    
    
    
    if(OK == sta_bind_check_frame(protocoltype,revbuf,revlen,MeterAddr))
    {
        Prtcltemp = protocoltype;
    }
    else
    {
        app_printf("sta_bind_check_frame error!\n");//�쳣;
        return;
    }
    DevicePib_t.Prtcl = Prtcltemp;
    DevicePib_t.DevType = e_METER_APP;

    app_printf("timer_stop sta_bind_timer success!!\n");


	if(0 != timer_stop(sta_bind_timer, TMR_NULL))
	{
		app_printf("timer_stop sta_bind_timer fail!\n");
	}

    if(0 != memcmp(MeterAddr, DevicePib_t.ltu_addr, 6)) //��ַ�ı�
    {
        // STA�����ɼ�¼���ڵ��ַ
        record_ltu_address(MeterAddr, protocoltype);

        if(DevicePib_t.PowerOffFlag == e_power_on_now)
        {
            app_printf("the meter is change,clean PowerOffFlag!\n");
        }
        dump_buf(MeterAddr,6);
        dump_buf(DevicePib_t.ltu_addr, 6);
        app_printf("meter change\n");
		sta_bind_info_t.Has20Event = 0;	//������ȷ���Ƿ���20����;
        DevicePib_t.PowerOffFlag = e_power_is_ok;
        DevicePib_t.MeterPrtcltype = SinglePrtcl_e;
        event_report_save.Oop20flag = 0;
		event_report_save.outlflag = 0;
		sta_clear_time_manager_flash();  //���ʱ����ص�flash��Ϣ
        //��ֹ�����ʱ��ģ���Ѿ������˵ȴ������ϱ�������Ϣʱ�Ű��ɹ���
		//��ʱ�������״̬����ɻ����ϱ�������Ϣ
		if(TMR_STOPPED != zc_timer_query(sta_power_on_report_timer))
		{
			timer_stop(sta_power_on_report_timer,TMR_NULL);
		}
    }

    __memcpy(DevicePib_t.DevMacAddr, MeterAddr, 6);
    dump_buf( MeterAddr, 6);
    dump_buf(DevicePib_t.DevMacAddr, 6);
    if(sta_bind_info_t.BaudRate > BAUDRATE_9600)
    {
        if(PROVINCE_HENAN == app_ext_info.province_code)
        {
            sta_bind_info_t.PrtclOop20 = 0;
            high_baud =TRUE;
        }
        else
        {
            sta_bind_info_t.PrtclOop20 = 1;
        }
    }
    else
    {
        sta_bind_info_t.PrtclOop20 = 0;
    }
    if(DevicePib_t.Prtcl == DLT645_2007 )
    {
        app_printf("get protocol : %d\n",DevicePib_t.Prtcl);

        sta_bind_check698_t.protocol = DLT698;
        sta_bind_check698_t.timeout = BAUDMETER_TIMEOUT;
        sta_bind_check698_t.frameLen = sizeof(sta_bind_frame_698);
        sta_bind_check698_t.frameSeq = 0;
        sta_bind_check698_t.pPayload = sta_bind_frame_698;
        sta_bind_check698_t.baudrate = sta_bind_info_t.BaudRate;
        sta_bind_put_to_list(&sta_bind_check698_t);
        
        
        sta_bind_info_t.MeterComuState = MeterCheck698_e;
    }
    else if(DevicePib_t.Prtcl == DLT698)
    {
        if(PROVINCE_HENAN == app_ext_info.province_code)
        {
            U8 *sendbuf = zc_malloc_mem(100, "sta_bind_check645.pPayload", MEM_TIME_OUT);
            U16 sendlen = 0;

            Dlt645Encode(DevicePib_t.DevMacAddr, DLT645_2007, sendbuf, (U8 *)&sendlen);

            // ��֤�Ƿ���˫Э���
            sta_bind_check645_t.protocol = DLT645_2007;
            sta_bind_check645_t.timeout = BAUDMETER_TIMEOUT;
            sta_bind_check645_t.frameLen = sendlen;
            sta_bind_check645_t.frameSeq = 0;
            sta_bind_check645_t.pPayload = sendbuf;
            sta_bind_check645_t.baudrate = sta_bind_info_t.BaudRate;
            sta_bind_put_to_list(&sta_bind_check645_t);

            sta_bind_info_t.MeterComuState = MeterCheck645_e;

            zc_free_mem(sendbuf);
            return;
        }
        if(1 == app_ext_info.func_switch.oop20BaudSWC && sta_bind_info_t.BaudRate == BAUDRATE_9600)
            sta_bind_consult20_start();
        else
            sta_bind_success();
    }
    else
    {
        //Set 
        if(FactoryTestFlag)
		{
			if(0 != timer_stop(sta_bind_timer, TMR_NULL))
        	{
        		app_printf("FactoryTestFlag timer_stop sta_bind_timer fail!\n");
        	}
		}
		else
		{
            app_printf("BandMeter 97 success!\n");
            sta_bind_success();
        }
    }
    
    

}
static U8 sta_bind_check698_match(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }

    return result;
}

static void sta_bind_check698_timeout(void *pMsgNode)
{
    app_printf("MeterCheck698Timeout BandMeter 07 success!\n");
    if (PROVINCE_HENAN == app_ext_info.province_code)
    {
        if (SinglePrtcl_e != DevicePib_t.MeterPrtcltype)
        {
            DevicePib_t.MeterPrtcltype = SinglePrtcl_e; // ��Э���
            staflag = TRUE;
        }
    }
    sta_bind_success();
}

static void sta_bind_check698_receive(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    app_printf("MeterCheck698RecvDeal\n");
	U8 Addr[6] = {0};
    if(Check698Frame(revbuf,revlen,NULL,Addr, NULL) == FALSE || memcmp(Addr,DevicePib_t.DevMacAddr,MACADRDR_LENGTH) != 0)
    {
        DevicePib_t.Prtcl = DLT645_2007;
        app_printf("BandMeter 07 success!\n");
        if(PROVINCE_HENAN == app_ext_info.province_code)
        {
            if (SinglePrtcl_e != DevicePib_t.MeterPrtcltype)
            {
                DevicePib_t.MeterPrtcltype = SinglePrtcl_e; // ��Э���
                staflag = TRUE;
            }
        }
        sta_bind_success();
    }
	else
	{
        DevicePib_t.Prtcl = DLT698;
        //˫Э���
        if(PROVINCE_HENAN == app_ext_info.province_code)
        {
            if (SinglePrtcl_e == DevicePib_t.MeterPrtcltype)
            {
                DevicePib_t.MeterPrtcltype = DoublePrtcl_e; // ˫Э���
                staflag = TRUE;
            }
        }
        //׼������20�汾�����֤
        if(1 == app_ext_info.func_switch.oop20BaudSWC && sta_bind_info_t.BaudRate == BAUDRATE_9600)
		{
    		sta_bind_consult20_start(); 
	    }  
		else
		{
    		sta_bind_success();
		}
    }
}



static U8 sta_bind_consult20_match(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }

    return result;
}

static void sta_bind_consult20_timeout(void *pMsgNode)
{

    sta_bind_info_t.PrtclOop20 = 0;
    sta_bind_success();

	//����������Э�̱������
    StartMeterCheckManage(VerifyFirst_e);
}

static void sta_bind_consult20_receive(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    U16 action_len = (revlen > sizeof(DL698_action_type_s)) ? revlen : sizeof(DL698_action_type_s);
    DL698_action_type_s *action = zc_malloc_mem(action_len,"BIND",MEM_TIME_OUT);
    sta_bind_info_t.PrtclOop20 = 0;
    
    if(NULL != action && TRUE == Client698FrameProc(revbuf, revlen, (void *)action))
    {
        app_printf("action.OI = %x action.MethodId = %x,\n",action->OI,action->MethodId);
        if(action->OI == 0xF200 && action->MethodId == 0x80 && action->RESULT == 1)
        {
            //׼��һ����л�������
            app_printf("change_baud_by_698 OK\n");
            sta_bind_consult_baud = BAUDRATE_19200;
            sta_bind_info_t.PrtclOop20 = 1;
            
    
            Modify_ChangeBaudtimer(&sta_bind_consult_baud);
            //��ֹ�״����û��19200��ʼ
            sta_bind_info_t.BaudRate = BAUDRATE_19200;
            //�����20��ֱ�ӿ�����һ�����
            //StartMeterCheckManage(VerifyFirst_e);
            
        }
    }

	app_printf("sta_bind_consult20_t  BandMeter success!\n");
    sta_bind_success();
	//�����20��ֱ�ӿ�����һ�����,��ˢ���������ʱ��
    StartMeterCheckManage(VerifyFirst_e);

    if(NULL != action)
    {
        zc_free_mem(action);
    }
}

static U8 sta_bind_check_match(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }
    if(sendprtcl == DLT645_1997&&rcvprtcl == DLT645_2007)
    {
        result = TRUE;
    }

    return result;
}

static void sta_bind_check_timeout(void *pMsgNode)
{
    add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;
    
    MeterCheckFail(pSerialMsg->StaReadMeterInfo.ReadMode);
}

static void sta_bind_check_receive(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    U8   Addr[6] = {0};
    U8   Prtcltemp = 0;
    add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;
    
    Prtcltemp = protocoltype==DLT698?DLT698:DLT645_2007;
    if(GetAddrByProtocol(Prtcltemp,revbuf,revlen,Addr) == OK)
    {
        if(memcmp(Addr,DevicePib_t.ltu_addr,6) == 0)
        {
            MeterCheckSucc(pSerialMsg->StaReadMeterInfo.ReadMode);
            return;
        }
    }
    MeterCheckFail(pSerialMsg->StaReadMeterInfo.ReadMode);
}

METER_COMU_INFO_t sta_bind_normal_t = {
	.name    	= "sta_bind_normal_t",
	.Match   	= sta_bind_normal_match,
	.Timeout    = sta_bind_normal_timeout,
	.RevDel     = sta_bind_normal_recvive,
};


METER_COMU_INFO_t sta_bind_check698_t = {
	.name    	= "sta_bind_check698_t",
	.Match   	= sta_bind_check698_match,
	.Timeout    = sta_bind_check698_timeout,
	.RevDel     = sta_bind_check698_receive,
};


METER_COMU_INFO_t sta_bind_check645_t = {
	.name    = "sta_bind_check645_t",
	.Match   = sta_bind_check645_match,
	.Timeout = sta_bind_check645_timeout,
	.RevDel  = sta_bind_check645_receive,
};


METER_COMU_INFO_t sta_bind_consult20_t = {
	.name    	= "sta_bind_consult20_t",
	.Match   	= sta_bind_consult20_match,
	.Timeout    = sta_bind_consult20_timeout,
	.RevDel     = sta_bind_consult20_receive,
};


METER_COMU_INFO_t sta_bind_check_t = {
	.name    	= "sta_bind_check_t",
	.Match   	= sta_bind_check_match,
	.Timeout    = sta_bind_check_timeout,
	.RevDel     = sta_bind_check_receive,
};


static void sta_bind_put_to_list(METER_COMU_INFO_t *pMeterInfo)
{
    add_serialcache_msg_t serialmsg;
    
	memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));
    serialmsg.StaReadMeterInfo.ReadMode = pMeterInfo->comustate;

	serialmsg.Uartcfg = meter_serial_cfg;
    serialmsg.baud = pMeterInfo->baudrate;
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
    
    serialmsg.Match  = pMeterInfo->Match;
    serialmsg.Timeout = pMeterInfo->Timeout;
    serialmsg.RevDel = pMeterInfo->RevDel;


    serialmsg.Protocoltype = pMeterInfo->protocol;
	serialmsg.DeviceTimeout = pMeterInfo->timeout;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = pMeterInfo->frameLen;
	serialmsg.FrameSeq = pMeterInfo->frameSeq;
	serialmsg.lid = e_Serial_AA;
	serialmsg.ReTransmitCnt = 0;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;


	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pMeterInfo->pPayload,TRUE);
    return;
}



static void sta_bind_success(void)
{
#ifdef TH2CJQ
    app_printf("DevicePib_t.Modularmode = %d\n", DevicePib_t.Modularmode);//D0��ģ��ģʽ��0���ɼ���ģʽ��1�����ģʽ��
    app_printf("app_ext_info.func_switch.AddrShiftSWC =  %d\n", app_ext_info.func_switch.AddrShiftSWC);

    if(e_IOT_C_MODE_STA == DevicePib_t.Modularmode)	//D0��ģ��ģʽ��0���ɼ���ģʽ��1�����ģʽ��
    {
    	SetMacAddrRequest(DevicePib_t.DevMacAddr, e_METER, e_MAC_METER_ADDR); // DevicePib_t.DevType
    }
	else
	{
		SetMacAddrRequest(DevicePib_t.DevMacAddr, e_CJQ_2, e_MAC_METER_ADDR); // DevicePib_t.DevType
	}

    Modify_ReadTH2Metertimer(5);
#else
    app_printf("not TH2CJQ\n");
    SetMacAddrRequest(DevicePib_t.DevMacAddr,e_METER,e_MAC_METER_ADDR); // DevicePib_t.DevType
#endif
    staflag = TRUE;
    mutex_post(&mutexSch_t);	//��schtask����Ϣ
    //���е��Ĭ��׼���������
    //sta_bind_info_t.MeterComuState = MeterCheckCycle_e;
    if(sta_bind_info_t.PrtclOop20 == 1 || high_baud == TRUE)
    {
        ComuVerify.State = VerifyCycle_e;
        Modify_MeterVerifytimer(METER_COMU_JUDGE_PERIC);
    }
}



void sta_bind_timer_modify_start(U32 arg_time_ms)
{
	if(sta_bind_timer == NULL)
	{
		return;
	}
	
    timer_modify(sta_bind_timer,
					arg_time_ms,
					0,
					TMR_TRIGGER ,//TMR_TRIGGER
					sta_bind_timercb,
					NULL,
					"sta_bind_timercb",
					TRUE);

    timer_start(sta_bind_timer);
}

/*************************************************************************
 * sta_bind_packet_send_info - ��֡���֡
 * @arg_index: 		������
 * @arg_send_buf: 	���֡
 * @arg_send_buf_len: 	���֡����
 * @return��				
 	TRUE�����֡��֡�ɹ�
 	FALSE�����֡��֡ʧ��
 *************************************************************************/
U8 sta_bind_packet_send_info(U8 arg_index , U8 *arg_send_buf , U8 *arg_send_buf_len)
{
        
    U8 tp_pro = DLT645_UN_KNOWN;
    tp_pro = ProtBaud[arg_index].Protocol;
    app_printf("index: %d Protocol :%s Baud :%d\n",arg_index,tp_pro==DLT645_1997?"645-97":
                                                                                tp_pro==DLT645_2007?"645-07":
                                                                                tp_pro==DLT698?"698":"DLT645_UN_KNOWN"
        ,baud_enum_to_int(ProtBaud[arg_index].Baud));

    sta_bind_info_t.BaudRate = baud_enum_to_int(ProtBaud[arg_index].Baud);
	
    if(tp_pro == DLT645_1997)
    {
        __memcpy(arg_send_buf ,sta_bind_frame_97 , sizeof(sta_bind_frame_97));
        *arg_send_buf_len = sizeof(sta_bind_frame_97);
    }
    else if(tp_pro == DLT645_2007)
    {
        __memcpy(arg_send_buf ,sta_bind_frame_07 , sizeof(sta_bind_frame_07));
        *arg_send_buf_len = sizeof(sta_bind_frame_07);
    }
    else if(tp_pro == DLT698)
    {
        __memcpy(arg_send_buf ,sta_bind_frame_698 , sizeof(sta_bind_frame_698));
        *arg_send_buf_len = sizeof(sta_bind_frame_698);
    }
    else
    {
        return ERROR;
    }

    return OK;

}


//���ʱ���ص����ƺ���
static void sta_bind_ctrl(void)
{
	static U8 st_round = 0;
    U8 *tp_sendbuf = zc_malloc_mem(100, "sendbuf", MEM_TIME_OUT);
    U8  tp_sendbuflen = 0;
	U8  tp_fe_num;
	
	//���������ڰ��ʱ��
    sta_bind_timer_modify_start(BIND_ROUND_TIME);

	//����ģʽ�����
	if(FactoryTestFlag == 1 || getHplcTestMode() != NORM)
	{
		timer_stop(sta_bind_timer,TMR_NULL);
        zc_free_mem(tp_sendbuf);
		return;
	}

	//���ʱ�������Ҫģ���ַ������ģ���ַ����
	if(st_round >= BIND_TIME && DevicePib_t.DevType == e_SET_ADDR_APP)
	{
		st_round = 0;
		SetMacAddrRequest(DevicePib_t.DevMacAddr,e_METER,e_MAC_METER_ADDR);//TODO����ַ�����ǵ��ܱ���ģ��
		timer_stop(sta_bind_timer,TMR_NULL);
        zc_free_mem(tp_sendbuf);
		return;
	}
	
    //�ִο���
    /*
	������ʱ����ѭ20�κ��ٴν��а��
    */
    if(st_round >= BIND_TIME)
	{
	    st_round++;
		//���ʱ��20��
	    if(st_round >= (BIND_TIME + ROUND_INT_TIME))
        {   
            //10�ְ�����������40�룬���¿�ʼ���
		    st_round = 0;
            DevicePib_t.BandMeterCnt = 0;
        }
        else//������û����20�Σ�������ѭ
        {
            sta_bind_timer_modify_start(BIND_ROUND_TIME);
            zc_free_mem(tp_sendbuf);
            return;
        }
	}
	
	//��ֹ��ת
    if(st_round > 0)
    {
        DevicePib_t.BandMeterCnt++;
    }
    st_round++;

	//�ж�һ�ֽ���������PB_07_9600��ʼ
    (DevicePib_t.BandMeterCnt >= BAUD_ARR) ? DevicePib_t.BandMeterCnt = 0 : DevicePib_t.BandMeterCnt;
	
    if(sta_bind_packet_send_info(DevicePib_t.BandMeterCnt,tp_sendbuf, &tp_sendbuflen) != OK)
    {
    	//��֡ʧ��;
        zc_free_mem(tp_sendbuf);
        return;
    }
	
    /*�����ִ�������8��FE��ż���ִη���4��FE*/
	tp_fe_num = ((((st_round - 1)/BAUD_ARR)%2) != 0 ? 4 : 0);

	__memmove(tp_sendbuf+tp_fe_num,tp_sendbuf,tp_sendbuflen);
    tp_sendbuflen += tp_fe_num;
	memset(tp_sendbuf,0XFE,tp_fe_num);

	//���ð����Ϣ
    sta_bind_normal_t.protocol = ProtBaud[DevicePib_t.BandMeterCnt].Protocol;

    sta_bind_normal_t.timeout = BAUDMETER_TIMEOUT;
    sta_bind_normal_t.frameLen = tp_sendbuflen;
    sta_bind_normal_t.frameSeq = DevicePib_t.BandMeterCnt;
    sta_bind_normal_t.pPayload = tp_sendbuf;
	sta_bind_normal_t.baudrate = sta_bind_info_t.BaudRate;
    
    sta_bind_put_to_list(&sta_bind_normal_t);
	
    zc_free_mem(tp_sendbuf);
}


//���ʱ���ص�
static void sta_bind_timercb(struct ostimer_s *ostimer, void *arg)
{
	sta_bind_ctrl();
}



/*************************************************************************
 * sta_bind_init - ����ܳ�ʼ��
 * @argument:	��
 * @return��		��
 *************************************************************************/
void sta_bind_init(void)
{
    //������ģʽ����
    if(getHplcTestMode() != NORM)
	{
		return;
	}
	//��ʼ�����״̬Ϊ���
	sta_bind_info_t.MeterComuState = MeterCheckBaud_e;
    sta_bind_info_t.PrtclOop20 = 0;
	//��ʼ�����ʱ��
	if(sta_bind_timer == NULL)
	{
		sta_bind_timer = timer_create(2*TIMER_UNITS,
						 0,
						 TMR_TRIGGER,//TMR_TRIGGER
						 sta_bind_timercb,
						 NULL,
						 "sta_bind_timercb"
						);
	}

	if(sta_bind_timer == NULL)
	{
		sys_panic("<sta_bind_timer fail!> %s: %d\n");
		return;
	}
	
	sta_bind_timer_modify_start(0.6*TIMER_UNITS);
}

void sta_meter_prtcltype_init(void)
{
    if(PROVINCE_HENAN != app_ext_info.province_code)
    {
        DevicePib_t.MeterPrtcltype = SinglePrtcl_e;
    }
    else
    {
        app_printf("henan meter keep prtcltype\n");
    }
}

#endif

