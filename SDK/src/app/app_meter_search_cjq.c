#include "types.h"
#include "app_dltpro.h"
#include "datalinkdebug.h"
#include "app_meter_search_cjq.h"
#include "cjqTask.h"
#include "app_base_serial_cache_queue.h"
#include "dev_manager.h"
#include "app.h"
#include "app_power_onoff.h"

#if defined(ZC3750CJQ2)

/**/
static uint8_t cjq2_search_match(uint8_t sendprtcl, uint8_t rcvprtcl, uint8_t sendseq, uint8_t rcvseq);
static uint8_t cjq2_search_traverse_match(uint8_t sendprtcl, uint8_t rcvprtcl, uint8_t sendseq, uint8_t rcvseq);
static void cjq2_search_check_timeout(void *pMsgNode);
static void cjq2_search_check_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);
static void cjq2_search_check698_timeout(void *pMsgNode);
static void cjq2_search_check698_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);
static void cjq2_search_aa_timeout(void *pMsgNode);
static void cjq2_search_aa_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);
static void cjq2_search_traverse_timeout(void *pMsgNode);
static void cjq2_search_traverse_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq);


U32 BaudNum = 0;


cjq2_searh_meter_t cjq2_search_meter_info_t;
ostimer_t *cjq2_search_suspend_timer = NULL;		//�ѱ���ͣ��ʱ��
ostimer_t *cjq2_search_meter_timer = NULL;			//�ѱ�ʱ��
ostimer_t *cjq2_search_meter_restart_timer = NULL;	//�����ѱ�ʱ��


U8 cjq2_search_item_97[2] = {0X43, 0XC3};//97ʹ��
U8 cjq2_search_item_07[4] = {0x33, 0x33, 0x34, 0x33};//07ʹ�������й���ȡ���
U8 cjq2_search_item_698[8] = {0x05,0x01,0x00,0x40,0x02,0x01,0x00,0x00 };//698��ȡ���-�߼���  ��΢�ѱ�ʹ��
//U8 Item698AA[8] = {0x05,0x01,0x00,0x40,0x01,0x02,0x00,0x00 };//698��ȡͨ�ŵ�ַ-���� ͨ���AA�ѱ�


cjq_dev_t cjq2 = {
	.init      = CJQ_Flash_Init,
	.name      = "cjq2 init",
	.showver   = CJQ_Extend_ShowVersion,
	.showinfo  = CJQ_Extend_ShowInfo,
};
	
cjq_search_func_t cjq2_search_check_t = {
	.name    = "cjq2_search_check_t",
	.match   = cjq2_search_match,
	.tmot    = cjq2_search_check_timeout,
	.rcv     = cjq2_search_check_receive,
};

cjq_search_func_t cjq2_search_check698_t = {
	.name    = "cjq2_search_check698_t",
	.match   = cjq2_search_match,
	.tmot    = cjq2_search_check698_timeout,
	.rcv     = cjq2_search_check698_receive,
};

cjq_search_func_t cjq2_search_aa_t = {
	.name    = "cjq2_search_aa_t",
	.match   = cjq2_search_match,
	.tmot    = cjq2_search_aa_timeout,
	.rcv     = cjq2_search_aa_receive,
};
cjq_search_func_t cjq2_search_traverse_t = {
	.name    = "cjq2_search_traverse_t",
	.match   = cjq2_search_traverse_match,
	.tmot    = cjq2_search_traverse_timeout,
	.rcv     = cjq2_search_traverse_receive,
};


static U8 cjq2_search_get_info_by_pro(U8 protocol, U8 *data, U16 datalen, U8 *pctrl, U8 *paddr, U8 *pFEnum, U16 *pdecodelen)
{
	if(protocol == DLT645_2007 || protocol == DLT645_1997)
	{
		if(Check645Frame(data,datalen,pFEnum,paddr,pctrl)== FALSE)
		{
			return ERROR;
		}
		return OK;
	}
	else if(protocol == DLT698)
	{
		if(Check698Frame(data,datalen,pFEnum,paddr,NULL) == FALSE)
        {
            return ERROR;
        }
		return OK;
	}
	else
    {
        return ERROR;
    }
}


static U8 cjq2_search_check_err_code_len(U8 protocol ,U8 *buf ,U16 datalen ,U8 *fenum ,U8 *errLen)
{
    uint32_t framelen = 0;
    uint8_t  framelenpos;
	
    if(DLT645_1997 == protocol || DLT645_2007 == protocol)
    {
        framelenpos = 9;
    }
    else if(DLT698 == protocol)
    {
        framelenpos = 1;
    }
    else
    {
        return ERROR;
    }
    uint32_t pos = 0;

    if(datalen < 12)
        return ERROR;
    do
    {
    	//app_printf("buf + pos  = %d\n",buf[pos]);
        switch (*(buf + pos))
        {
            case 0x68:
            {
    			framelen = (DLT645_1997 == protocol || DLT645_2007 == protocol)?*(buf + pos + framelenpos) + 12:
    				(DLT698 == protocol)?(*(buf + pos + framelenpos) | (*(buf + pos + framelenpos + 1) << 8)) + 2:0;
                if((DLT645_1997 == protocol && framelen > 255) || 
                    (DLT645_2007 == protocol && framelen > 255) ||
                    (DLT698 == protocol && framelen > 2048))
                {
                    app_printf("framelen: 0x%04x > 2096\n",framelen);
                    pos++;
                    break;
                }
                //debug_printf(&dty, DEBUG_METER, "framelen : 0x%04x!\n",framelen);
                if((datalen - pos) < framelen) //帧不完整
                {
                    app_printf("(*len-pos) < framelen) : 0x%04x!\n",framelen);
                    pos++;
                    break;
                }
                
    			 //解决68扰码
                if(*(buf + pos + framelen - 1) != 0x16)
                {
                    app_printf("*(buf + pos + framelen - 1) : 0x%04x!\n",*(buf + pos + framelen - 1));
                    pos++;
                    break;
                }
    			//645协�??判定�?二�??8
                if(DLT645_1997 == protocol || DLT645_2007 == protocol)
                {
                    if(*(buf + pos + 7)!=0x68)
                    {
                        app_printf("Protocol = %d \n" , protocol);
                        pos++;
                        break;
                    }
                }
                if(DLT645_1997 == protocol || DLT645_2007 == protocol)
                {
                   
                    if(*(buf + pos + framelen - 2) !=check_sum((buf + pos) , framelen - 2))
                    {
                        app_printf("Sum = %04x,check_sum = %04x!\n", *(buf + pos + framelen - 2) , check_sum((buf + pos) , framelen-2));
                        pos++;
                        break;
                    }
                }
				if(DLT698 == protocol)
                {
                    U16  cs16;
                    U16  CRC16Cal;
                    cs16 = (*(buf + pos + framelen -3) | (*(buf + pos + framelen -2) << 8));
                    CRC16Cal = ResetCRC16();
                    if(cs16 != CalCRC16(CRC16Cal,(buf + pos +1) , 0 , framelen-4))
                    {
                        app_printf("cs16 = %04x , CRC16Cal = %04x!\n", cs16 , CRC16Cal);
                        pos++;
                        break;
                    }
                }
                if((pos + framelen) < datalen) //如果后续还有数据
                {
                    app_printf("len : %04x\n pos+framelen : %04x!\n", datalen , pos + framelen);
                    *errLen += datalen - (pos + framelen);
                    
                    app_printf("OK *errLen : %d *fenum : %d\n", *errLen , *fenum);
                }
                else
                {
                    
                }
				app_printf("OK *errLen : %d *fenum : %d\n", *errLen , *fenum);
                return OK;
            }
            break;
        
            default:
            {
                
                if(*(buf + pos) == 0xFE)
                {
                    (*fenum) ++;
                }
                else
                {
                    (*errLen) ++;
                }
				pos++;
            }
            break;
        }
    }while(pos < datalen);
    app_printf("ERR *errLen : %d *fenum : %d\n", *errLen , *fenum);
    return ERROR;
}















	
static void cjq2_search_attach_dev(cjq_dev_t *dev)
{
	dev->init(dev);
	dev->showver();
	dev->showinfo();
}


static uint8_t cjq2_search_match(uint8_t sendprtcl, uint8_t rcvprtcl, uint8_t sendseq, uint8_t rcvseq)
{
	app_printf("cjq2_search_match sendprtcl:%d    rcvprtcl:%d\n",sendprtcl,rcvprtcl);
	if(rcvprtcl == DLT645_2007)
	{

	}
	else if(rcvprtcl == DLT698)
	{

	}
	else
	{
		;
	}
	return TRUE;
	
}

static uint8_t cjq2_search_traverse_match(uint8_t sendprtcl, uint8_t rcvprtcl, uint8_t sendseq, uint8_t rcvseq)
{
	app_printf("cjq2_search_match sendprtcl:%d    rcvprtcl:%d\n",sendprtcl,rcvprtcl);
	return TRUE;
}
static void cjq2_search_check_timeout(void *pMsgNode)
{
    app_printf("cjq2_search_check_timeout\n");
	if(FALSE == check_power_off_and_plug_out())
	{
    	CJQ_DelMeterInfoByIdx(cjq2_search_meter_info_t.Ctrl.CheckMeterIdx);
	}
	CJQ_Check_Next(&cjq2_search_meter_info_t.Ctrl);
}

static void cjq2_search_check_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
    app_printf("cjq2_search_check_receive\n");
	U8	Protocol = 0;
	U8	Baud = 0;
	//U8  Ctrl = 0;
	CJQ_Func_GetProtocolBaud(&cjq2_search_meter_info_t.Ctrl,&Protocol, &Baud);

	if(cjq2_search_get_info_by_pro(Protocol,revbuf,revlen,NULL,cjq2_search_meter_info_t.Ctrl.SearchResultAddr,NULL,NULL)== OK)
	{
		CJQ_Check_Succ(&cjq2_search_meter_info_t.Ctrl);
	}
	else
	{
		CJQ_Check_Errocde(&cjq2_search_meter_info_t.Ctrl);
	}
}
static void cjq2_search_check698_timeout(void *pMsgNode)
{
    app_printf("cjq2_search_check698_timeout\n");
    //ȷ����07����˫Э��
    U8  Protocol = 0;
	U8  Baud = 0;
    CJQ_Func_GetProtocolBaud(&cjq2_search_meter_info_t.Ctrl,&Protocol, &Baud);
    if(Protocol == DLT645_2007)
    {
    	CJQ_AddMeterInfo(cjq2_search_meter_info_t.Ctrl.SearchResultAddr, Protocol, Baud);
    	if(cjq2_search_meter_info_t.Ctrl.SearchState != SEARCH_STATE_CHECKMETER)
    	{
            if(CollectInfo_st.MeterNum >= METERNUM_MAX)
        	{
        		MeterNumIsFull(&cjq2_search_meter_info_t.Ctrl);
        		return;
        	}
    	}
    }
    //���յ�ǰ״̬���л�next�������ѱ�
    CJQ_Check698_Next(&cjq2_search_meter_info_t.Ctrl);
}
static void cjq2_search_check698_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
    //�ж��Ƿ���698��

	app_printf("cjq2_search_check698_receive\n");
	U8	Protocol = 0;
	U8	Baud = 0;
	//U8  Ctrl = 0;
	CJQ_Func_GetProtocolBaud(&cjq2_search_meter_info_t.Ctrl,&Protocol, &Baud);
	if(Protocol == DLT645_2007)
	{
		Protocol = DLT698;
	}
	//if(GetAddrByProtocol(protocoltype,revbuf,revlen,cjq2_search_meter_info_t.Ctrl.SearchResultAddr) == OK)
	//if((GetAddrByProtocol(Protocol,revbuf,revlen,cjq2_search_meter_info_t.Ctrl.SearchResultAddr) == OK))
	if(cjq2_search_get_info_by_pro(Protocol,revbuf,revlen,NULL,cjq2_search_meter_info_t.Ctrl.SearchResultAddr,NULL,NULL)== OK)
    {
        if(CJQ_Func_JudgeBCD(cjq2_search_meter_info_t.Ctrl.SearchResultAddr,6) == TRUE)
        {
            //����698����
            CJQ_Check698_Succ(&cjq2_search_meter_info_t.Ctrl);
            
            return;
        }
    }
	CJQ_Check698_Errcode(&cjq2_search_meter_info_t.Ctrl);
}

static U8 cjq2_search_check_ctrl(U8       Ctrl ,U8 protocol)
{
	//�жϷ���λ�Ƿ���ȷ
	if(protocol == DLT645_2007 || protocol == DLT645_1997)
	{
		return ((Ctrl&0x80)==0x80)?TRUE:FALSE; 
	}
	else
	{
		return TRUE;
	}
}



static void cjq2_search_aa_timeout(void *pMsgNode)
{
    app_printf("cjq2_search_aa_timeout\n");
	CJQ_AA_Next(&cjq2_search_meter_info_t.Ctrl);
}
static void cjq2_search_aa_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
    app_printf("cjq2_search_aa_receive protocoltype = %d\n",protocoltype);
	U8	Protocol = 0;
	U8	Baud = 0;
	//U8  Ctrl = 0;
	CJQ_Func_GetProtocolBaud(&cjq2_search_meter_info_t.Ctrl,&Protocol, &Baud);
    if(cjq2_search_get_info_by_pro(Protocol,revbuf,revlen,NULL,cjq2_search_meter_info_t.Ctrl.SearchResultAddr,NULL,NULL)== OK)
    {
        if(CJQ_Func_JudgeBCD(cjq2_search_meter_info_t.Ctrl.SearchResultAddr,6) == TRUE)
        {
            CJQ_AA_Succ(&cjq2_search_meter_info_t.Ctrl);
            return;
        }
    }

    CJQ_AA_Errcode(&cjq2_search_meter_info_t.Ctrl);//�쳣;
   

    return;	
}
static void cjq2_search_traverse_timeout(void *pMsgNode)
{
    app_printf("cjq2_search_traverse_timeout\n");
	CJQ_Traverse_Next(&cjq2_search_meter_info_t.Ctrl);
}
static void cjq2_search_traverse_receive(void *pMsgNode, uint8_t *revbuf,uint16_t revlen,uint8_t protocoltype, uint16_t frameSeq)
{
    app_printf("cjq2_search_traverse_receive\n");
	U8	Protocol = 0;
	U8	Baud = 0;
    U8  FeNum = 0;
    U8  ErrLen = 0;
	U8  Ctrl = 0;
	CJQ_Func_GetProtocolBaud(&cjq2_search_meter_info_t.Ctrl,&Protocol, &Baud);
    //�ж��쳣֡����֡ճ֡�����쳣���������Ҫ��Ϊ�˴�����ͬβ�ű��ѱ����⡣
    if(cjq2_search_check_err_code_len(Protocol,revbuf,revlen,&FeNum,&ErrLen) == OK)
    {
        //�Ϸ�֡�ж�
		if(ErrLen == 0 && cjq2_search_get_info_by_pro(Protocol,revbuf,revlen,&Ctrl,cjq2_search_meter_info_t.Ctrl.SearchResultAddr,NULL,NULL)== OK)
        {
            if(cjq2_search_check_ctrl(Ctrl,Protocol) == TRUE &&
				 CJQ_Func_JudgeBCD(cjq2_search_meter_info_t.Ctrl.SearchResultAddr,6) == TRUE)
            {
                CJQ_Traverse_Succ(&cjq2_search_meter_info_t.Ctrl);
                return;
            }
        }
        //CJQ_Traverse_Next(&cjq2_search_meter_info_t.Ctrl);
		CJQ_Traverse_Errcode(&cjq2_search_meter_info_t.Ctrl);
		return;
    }
	
    CJQ_Traverse_Errcode(&cjq2_search_meter_info_t.Ctrl);
    return ;
}


/*************************************************************************
 * cjq2_search_meter_ctrl - �ѱ������Ϣ����
 * @argument:
 	@pPayload:	�ѱ�֡
 	@func:		ִ�к�������ʱ�����գ��ȶ�
 * @return��		��
 *************************************************************************/
void cjq2_search_meter_ctrl(uint8_t *pPayload, uint8_t len, cjq_search_func_t  *func)//(cjq2_searh_meter_t *Cjq2SearchInfo,cjq_search_func_t  *func)
{
    cjq2_searh_meter_t *Cjq2SearchInfo = &cjq2_search_meter_info_t;
    
    Cjq2SearchInfo->SearchTimeout   = CJQ2_SEARCH_METER_TIMEOUT;//2s
	Cjq2SearchInfo->ReTransmitCnt   = CJQ2_SEARCH_METER_RTRSMT_CNT;
    Cjq2SearchInfo->match           = func->match;
	Cjq2SearchInfo->tmot_func       = func->tmot;
	Cjq2SearchInfo->rcv_func        = func->rcv;
    Cjq2SearchInfo->FrameLen        = len;
    Cjq2SearchInfo->priority        = e_Serial_AA;
	__memcpy(Cjq2SearchInfo->FrameUnit,pPayload,len);
    app_printf("CILF :");
    dump_buf(cjq2_search_meter_info_t.FrameUnit,cjq2_search_meter_info_t.FrameLen);
    
}
/*
static cjq2_searh_meter_t *get_search_meter_list(cjq2_search_meter_header_t *list)
{
	cjq2_searh_meter_t *mbuf_n=NULL;
	if(list->nr)
	{
		mbuf_n = list_entry(list->link.prev, cjq2_searh_meter_t, link);
		if(mbuf_n->RunState == e_UNEXECUTE)
		{
			mbuf_n->RunState = e_EXECUTING;
			return mbuf_n;
		}
	}
	return NULL;
}
*/
static void cjq2_search_entry_fail()
{
	cjq2_search_meter_timer_modify(CJQ2_SEARCH_METER_INTERVAL_TIME,TMR_TRIGGER);
}
static void cjq2_search_send_event()
{
    cjq2_search_meter_timer_modify(CJQ2_SEARCH_METER_INTERVAL_TIME,TMR_TRIGGER);
	app_printf("cjq2 uart1 send ok!!!\n");
}
static void cjq2_search_msg_err()
{
	app_printf("cjq2_search_msg_err\n");
}
static void cjq2_search_update_meter_info(work_t *work)
{
    static U16  SericalSeq = 0;
	cjq2_searh_meter_t  *mbuf_n;
    //U32 *p;
    //__memcpy(p , (U32 *)work->buf , sizeof(U32));
    //p = (U32*)work->buf;
	//msg = (cjq2_searh_meter_t *)work->buf;
	U32 *p = NULL;
	p = ((unsigned int*)work->buf);
	mbuf_n = (cjq2_searh_meter_t *)*p;
	//mbuf_n = &cjq2_search_meter_info_t;
    app_printf("mbuf_n->FrameLen %d  %p,mbuf_n %p\n",mbuf_n->FrameLen,&cjq2_search_meter_info_t,mbuf_n);
	if(mbuf_n->FrameLen != 0)//(mbuf_n != NULL)
	{	
		//if(mbuf_n->SearchState == SEARCH_STATE_STOP)//��ͣ���������¿�ʼ�ѱ�
		//{
            //cjq2_search_meter_timer_modify(100,TMR_TRIGGER);
		//}
		if(mbuf_n->Ctrl.SearchState == SEARCH_STATE_FULL)
		{
			//�ѱ���ʱ���Ƿ���Ҫ����������δ��ʱ�������ѱ�
			return;
		}
		add_serialcache_msg_t  cjq2_add_serialcache_msg;
		memset((U8 *)&cjq2_add_serialcache_msg,0x00,sizeof(add_serialcache_msg_t));
        
		cjq2_add_serialcache_msg.lid           = mbuf_n->priority;
		cjq2_add_serialcache_msg.ack           = 1;

        cjq2_add_serialcache_msg.DeviceTimeout = mbuf_n->SearchTimeout;
		cjq2_add_serialcache_msg.FrameSeq = ++SericalSeq;
        //��λ�ѱ����ڶ��а���͸������������񣬻ظ�����λ�������н���
        cjq2_add_serialcache_msg.Protocoltype  = ((mbuf_n->Ctrl.SearchState == SEARCH_STATE_TRAVERSE)? APP_T_PROTOTYPE: mbuf_n->Ctrl.Protocol);
		//cjq2_add_serialcache_msg.Protocoltype  = ((mbuf_n->SearchState == SEARCH_STATE_TRAVERSE)? APP_T_PROTOTYPE: mbuf_n->Ctrl.Protocol);
		//app_printf("SearchState = %d\n",mbuf_n->Ctrl.SearchState);
		//app_printf("Protocoltype = %d\n",cjq2_add_serialcache_msg.Protocoltype);

		cjq2_add_serialcache_msg.baud          = baud_enum_to_int(mbuf_n->Ctrl.Baud);
		cjq2_add_serialcache_msg.verif         = mbuf_n->verif;
		cjq2_add_serialcache_msg.ReTransmitCnt = mbuf_n->ReTransmitCnt;
		cjq2_add_serialcache_msg.FrameLen      = mbuf_n->FrameLen;
		
		cjq2_add_serialcache_msg.Uartcfg       = meter_serial_cfg;
		cjq2_add_serialcache_msg.EntryFail     = cjq2_search_entry_fail;
		cjq2_add_serialcache_msg.SendEvent     = cjq2_search_send_event;
		cjq2_add_serialcache_msg.ReSendEvent   = NULL;
		cjq2_add_serialcache_msg.MsgErr        = cjq2_search_msg_err;
		cjq2_add_serialcache_msg.Match         = mbuf_n->match;
		cjq2_add_serialcache_msg.Timeout       = mbuf_n->tmot_func;
		cjq2_add_serialcache_msg.RevDel        = mbuf_n->rcv_func;
		
		cjq2_add_serialcache_msg.pSerialheader = &SERIAL_CACHE_LIST;
		extern ostimer_t *serial_cache_timer;
		cjq2_add_serialcache_msg.CRT_timer     = serial_cache_timer;
		serial_cache_add_list(cjq2_add_serialcache_msg, mbuf_n->FrameUnit,TRUE);
	}
	else
	{
		return;
	}
}

void cjq2_search_suspend_timercb(struct ostimer_s *ostimer, void *arg)
{
    app_printf("cjq2_search_suspend_timercb wait OK\n");
}


/*************************************************************************
 * cjq2_search_suspend_timer_modify - �ѱ���ͣ��ʱ���ع�
 * @argument:
 	@pPayload:	��ʱʱ��
 * @return��		��
 *************************************************************************/
void cjq2_search_suspend_timer_modify(uint32_t Ms)
{
    if(Ms == 0)
    {
        Ms = 100;
    }
    app_printf("Ms = %d ms\n",Ms);
	if(NULL == cjq2_search_suspend_timer)
	{
        cjq2_search_suspend_timer = timer_create(Ms,
								  0,
								  TMR_TRIGGER,
								  cjq2_search_suspend_timercb,
								  &cjq2_search_meter_info_t,
								  "cjq2_search_suspend_timercb");
    }
    else
    {
		timer_modify(cjq2_search_suspend_timer,
                Ms,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				cjq2_search_suspend_timercb,
				&cjq2_search_meter_info_t,
                "cjq2_search_suspend_timercb",
                TRUE);
	}
	
	timer_start(cjq2_search_suspend_timer);
	
}

void cjq2_search_meter_restart(SEARCH_CONTROL_ST *ctrl)
{
	if (CollectInfo_st.MeterNum == 0)
	{
		cjq2_search_meter_info_t.Ctrl.ProtocolBaud = 0;
		cjq2_search_meter_info_t.Ctrl.Protocol = METER_PROTOCOL_RES;
		cjq2_search_meter_info_t.Ctrl.Baud = METER_BAUD_RES;
		CJQ_AA_Begin(ctrl);
	}
	else
	{
		cjq2_search_meter_info_t.Ctrl.ProtocolBaud = 0;
		CJQ_Check_Begin(ctrl);
	}
}

static void cjq2_search_meter_timercb(struct ostimer_s *ostimer, void *arg)
{
    
    cjq2_searh_meter_t *cjq2_search_msg = (cjq2_searh_meter_t *)arg;
    //if(zc_timer_query(cjq2_search_msg->Suspend_timer) == TMR_RUNNING)
	if(zc_timer_query(cjq2_search_suspend_timer) == TMR_RUNNING)
    {
		if (cjq2_search_meter_info_t.CheckMeterWaitFlag || cjq2_search_msg->Ctrl.SearchState != SEARCH_STATE_STOP)
		{
			const uint32_t suspend_time = timer_remain(cjq2_search_suspend_timer);
			cjq2_search_meter_timer_modify(suspend_time, TMR_TRIGGER);
		}

		return;
    }
	else
	{
		if(cjq2_search_msg->Ctrl.SearchState == SEARCH_STATE_STOP || 
		cjq2_search_msg->Ctrl.SearchState == SEARCH_STATE_FULL)
		{
			cjq2_search_meter_info_t.CheckMeterWaitFlag = FALSE;
			cjq2_search_meter_restart(&cjq2_search_msg->Ctrl);
		}
	}
	
    work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(cjq2_searh_meter_t), "CUSMI", MEM_TIME_OUT);
	U32 *p = (U32 *)work->buf;
    __memcpy(p , (U32 *)&arg , sizeof(U32));
	//cjq2_search_msg->pcjq2listheader = (cjq2_search_meter_header_t *)arg;
	//__memcpy(work->buf,cjq2_search_msg,sizeof(U32));
	//cjq2_search_msg = (cjq2_searh_meter_t *)arg;
	
	app_printf("cjq2_search_msg->FrameLen %d,p %p %08x\n",cjq2_search_msg->FrameLen ,p , *p);
	work->work = cjq2_search_update_meter_info;
    work->msgtype = CJQ2UP;
	post_app_work(work);
}



/*************************************************************************
 * cjq2_search_meter_timer_init - �ѱ�ʱ����ʼ��
 * @argument:	��
 * @return��	
 	TRUE��Ĭ��
 *************************************************************************/
int8_t cjq2_search_meter_timer_init(void)
{
	if(PROVINCE_SHANGHAI == app_ext_info.province_code)
    {
		__memcpy((U8*)&ProtBaud,(U8*)&ProtBaud_shanghai,sizeof(ProtBaud));
	}
	//��ӡ�ɼ�����Ϣ
	cjq2_search_attach_dev(&cjq2);
	
	cjq2_search_meter_info_t.Suspend_timer = cjq2_search_suspend_timer;
	
	if(cjq2_search_meter_timer == NULL)
	{
        cjq2_search_meter_timer = timer_create(CJQ2_SEARCH_METER_INTERVAL_TIME,
								  CJQ2_SEARCH_METER_INTERVAL_TIME,
								  TMR_TRIGGER,
								  cjq2_search_meter_timercb,
								  &cjq2_search_meter_info_t,
								  "cjq2_search_meter_timercb");
		timer_start(cjq2_search_meter_timer);
	}
	return TRUE;
}

/*************************************************************************
 * cjq2_search_suspend_timer_modify - �ѱ���ͣ��ʱ���ع�
 * @argument:
 	@Ms:	��ʱʱ�䣬��λ1ms
 	@opt��	��ʱ�����ͣ����ںʹ���ʽ
 * @return��		��
 *************************************************************************/
void cjq2_search_meter_timer_modify(uint32_t Ms,uint32_t opt)
{
	if (Ms == CJQ2_SEARCH_METER_STOP_TIME)
	{
		cjq2_search_meter_info_t.CheckMeterWaitFlag = TRUE;
	}

	Ms = (Ms == 0) ? 100 : Ms;

	if(NULL != cjq2_search_meter_timer)
	{
		timer_modify(cjq2_search_meter_timer,
                Ms,
				CJQ2_SEARCH_METER_INTERVAL_TIME,
				opt ,//TMR_TRIGGER
				cjq2_search_meter_timercb,
				&cjq2_search_meter_info_t,
                "cjq2_search_meter_timercb",
                TRUE);
	}
	else
	{
		sys_panic("cjq2_search_meter_timer is null\n");
	}
	timer_start(cjq2_search_meter_timer);
	
}

#endif

