/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	plc.c
 * Purpose:	PLC generic implementation suite
 * History:
 *
 */


#include "list.h"
#include "types.h"
#include "Trios.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
//#include "RouterTask.h"
//#include "NetTable.h"
#include "app_gw3762.h"
#include "ZCsystem.h"
#include "app_rdctrl.h"
#include "app_dltpro.h"
#include "app_meter_verify.h"
#include "dl_mgm_msg.h"
#include "dev_manager.h"
#include "printf_zc.h"
#include "meter.h"
#include "app_common.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_read_cjq_list_sta.h"


STA_READMETER_UPDATA_t  StaReadMeterUpData;
ostimer_t* MeterApsChecktimer = NULL;

void sta_read_get_fe_num(U8* pdata, U16 datalen, U8* FENum)
{
    U8 FECount = 0;

    if (NULL == pdata)
        return;

    while (pdata[FECount] == 0xFE)
    {
        FECount++;
    }
    if (FENum)
    {
        *FENum = FECount;
    }
    return;
}

#if defined(STATIC_NODE)
/*************************************************************************
 * 函数名称	: 	void sta_read_meter_req()
 * 函数说明	: 	抄表
 * 参数说明	: 	定时时间
 * 返回值		: 	无
 *************************************************************************/
static void sta_read_meter_req(U16 OptionWord, U8 ProtocolType, U8 ReadMode, U16 dtei, U8 *DesAddr, U16 ApsSeq, U8 *buf, U16 len)
{
    READ_METER_REQ_t *pReadMeterRequest_t=NULL;

    pReadMeterRequest_t = (READ_METER_REQ_t*)zc_malloc_mem(sizeof(READ_METER_REQ_t) + len,"F1F1ReadMeter",MEM_TIME_OUT);
    
    pReadMeterRequest_t->ReadMode       = ReadMode;
    pReadMeterRequest_t->dtei           = dtei;
    pReadMeterRequest_t->Direction		= e_UPLINK;
    pReadMeterRequest_t->Timeout        = DEVTIMEOUT;
    pReadMeterRequest_t->PacketIner     = 0;
    pReadMeterRequest_t->ResponseState  = 0;
    pReadMeterRequest_t->OptionWord		= OptionWord;
	pReadMeterRequest_t->DatagramSeq    = ApsSeq;
    
	/*if(TRANS_PROTO_RF != ProtocolType)
	{
	    pReadMeterRequest_t->ProtocolType		=	DevicePib_t.Prtcl;
	}
	else*/
	{
		pReadMeterRequest_t->ProtocolType		= ProtocolType;
	}
    
    __memcpy(pReadMeterRequest_t->DstMacAddr, DesAddr, 6);
    __memcpy(pReadMeterRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);
    //pReadMeterRequest_t->Handle			=	AppPIB_t.AppHandle++;

    //给copy载荷数据到pMDB的载荷位置
    __memcpy(pReadMeterRequest_t->Asdu, buf, len);
    pReadMeterRequest_t->AsduLength = len;

	pReadMeterRequest_t->SendType = GetNodeState()	==e_NODE_ON_LINE?e_UNICAST_FREAM: e_PROXY_BROADCAST_FREAM_NOACK;

    //抄控器直抄回复单播
    if(is_rd_ctrl_tei(dtei))
    {
       pReadMeterRequest_t->SendType = e_UNICAST_FREAM;
       app_printf("resp rdctrl[%06x], use e_UNICAST_FREAM\n", dtei);
    }

    ApsReadMeterRequest(pReadMeterRequest_t);
    
	zc_free_mem(pReadMeterRequest_t);
}

U8 sta_read_meter_match(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq)
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

static void sta_read_meter_timeout(void *pMsgNode)
{
    add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;
    
    if(pSerialMsg->StaReadMeterInfo.LastFlag == LAST_ACTIVE 
			//&& pSerialMsg.StaReadMeterInfo.ReadMode //并发
			&& StaReadMeterUpData.RecvLen > 0)
    {
        sta_read_meter_req(StaReadMeterUpData.OptionWord, pSerialMsg->StaReadMeterInfo.ProtocolType, pSerialMsg->StaReadMeterInfo.ReadMode,
                         pSerialMsg->StaReadMeterInfo.Dtei, pSerialMsg->StaReadMeterInfo.DestAddr, 
                         pSerialMsg->StaReadMeterInfo.ApsSeq, 
                         StaReadMeterUpData.RecvBuff, StaReadMeterUpData.RecvLen);
        StaReadMeterUpData.RecvLen = 0;
        StaReadMeterUpData.OptionWord = 0;
    }
    else
    {
        app_printf("read timeout!\n");
    #if defined(ZC3750STA)
        PlcReadMeterFail();
    #endif
    }
}

void sta_read_meter_recv_deal(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    U16  offset=0;
    U16  len = 0; 
    add_serialcache_msg_t  *pSerialMsg = (add_serialcache_msg_t*)pMsgNode;

    app_printf("ptcl=%d len=%d\n", protocoltype, revlen);
    //dump_buf(revbuf, revlen);
    
    if(protocoltype == APP_T_PROTOTYPE)
    {
        len = revlen;
    }
    else
    {
        if(e_Decode_Success != ScaleDLT(protocoltype, revbuf, &offset, &revlen,  &len)) 
        {
            app_printf("decode error!\n");//异常;
            return;
        }
    }
	//防御性编程，可保证全局不踩内存，异常进行提醒
	if((StaReadMeterUpData.RecvLen + len) <= MAX_INFO_UP_LEN)
	{
	    __memcpy(&StaReadMeterUpData.RecvBuff[StaReadMeterUpData.RecvLen], revbuf+offset, len);
    	StaReadMeterUpData.RecvLen += len;
    	StaReadMeterUpData.OptionWord |= (1<<((frameSeq>>8)-1));
    }
	else
	{
		app_printf("Rlen %d len %d error!\n",StaReadMeterUpData.RecvLen, len);//异常;
	}
    
    app_printf("LastF=%d Len=%d\n", pSerialMsg->StaReadMeterInfo.LastFlag,StaReadMeterUpData.RecvLen);
    if(pSerialMsg->StaReadMeterInfo.LastFlag == LAST_ACTIVE)
    {
        sta_read_meter_req(StaReadMeterUpData.OptionWord, pSerialMsg->StaReadMeterInfo.ProtocolType, pSerialMsg->StaReadMeterInfo.ReadMode,
                         pSerialMsg->StaReadMeterInfo.Dtei, pSerialMsg->StaReadMeterInfo.DestAddr, 
                         pSerialMsg->StaReadMeterInfo.ApsSeq, 
                         StaReadMeterUpData.RecvBuff, StaReadMeterUpData.RecvLen);
         //针对点抄，存储的抄表结果
        if(pSerialMsg->StaReadMeterInfo.ReadMode == e_CONCENTRATOR_ACTIVE_RM)
        {
            gRm13f1RecvDataInfo.RM13F1ApsRecvPacketSeq = pSerialMsg->StaReadMeterInfo.ApsSeq;
            gRm13f1RecvDataInfo.RM13F1recivelen = StaReadMeterUpData.RecvLen;
            __memcpy(gRm13f1RecvDataInfo.RM13F1recivebuf, StaReadMeterUpData.RecvBuff, StaReadMeterUpData.RecvLen);
            //超时控制
            timer_start(Clear13F1RMdataTimer);
        }
        StaReadMeterUpData.RecvLen = 0;
        StaReadMeterUpData.OptionWord = 0;
    }
    
#if defined(ZC3750STA)
    PlcReadMeterSucc();
#endif
    
    return;
}

void sta_read_meter_put_list(READ_METER_IND_t *pReadMeterInd, U8 protocol, U8 baud, U16 timeout,
                                              U8 intervaltime, U8 retry, U16 frameSeq, U16 frameLen, U8 *pPayload, U8 lastFlag,U8 Test)
{
    add_serialcache_msg_t serialmsg;
	memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));
    
    //app_printf("SendLocalFrame-------------------------\n");
    //app_printf("lastFlag = %d\n", lastFlag);

    serialmsg.StaReadMeterInfo.ApsSeq       = pReadMeterInd->ApsSeq;
    serialmsg.StaReadMeterInfo.Dtei         = pReadMeterInd->stei;
    serialmsg.StaReadMeterInfo.ProtocolType = pReadMeterInd->ProtocolType;
    serialmsg.StaReadMeterInfo.ReadMode     = pReadMeterInd->ReadMode;
    serialmsg.StaReadMeterInfo.LastFlag     = lastFlag;
    __memcpy(serialmsg.StaReadMeterInfo.DestAddr, pReadMeterInd->SrcMacAddr, 6);

    serialmsg.baud = baud_enum_to_int(baud);
	serialmsg.Uartcfg = (baud == 0)?NULL:meter_serial_cfg;
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
    
    serialmsg.Match  = protocol== e_other?NULL : sta_read_meter_match;
#if defined(ZC3750CJQ2)
	if(Test == 0)
	{
		serialmsg.Timeout = TestStaReadMeterTimeout;
		serialmsg.RevDel = TestStaReadMeterRecvDeal;
	}
	else
	{
    	serialmsg.Timeout = sta_read_meter_timeout;
		serialmsg.RevDel = sta_read_meter_recv_deal;
	}
#elif defined(ZC3750STA)
	serialmsg.Timeout = sta_read_meter_timeout;
	serialmsg.RevDel = sta_read_meter_recv_deal;
#endif

    serialmsg.Protocoltype = protocol;
	serialmsg.DeviceTimeout = timeout;
    serialmsg.IntervalTime = intervaltime;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = frameLen;
	serialmsg.FrameSeq = frameSeq;
	serialmsg.lid = e_Serial_Trans;
	serialmsg.ReTransmitCnt = retry;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;

	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pPayload,FALSE);

    return;
}

//收到抄表帧后，若是双协议表则根据抄表协议类型来进行事件上报
void sta_read_meter_judge_double_prtc(U8 ProtocolType)
{
	if(SinglePrtcl_e != DevicePib_t.MeterPrtcltype && (TRANS_PROTO_698 == ProtocolType
		|| TRANS_PROTO_97 == ProtocolType || TRANS_PROTO_07 == ProtocolType))
	{
		if(DoublePrtcl_e == DevicePib_t.MeterPrtcltype)	//首次抄表后若是高电平则1分钟后上报一次
		{
			timer_stop(event_report_timer,TMR_NULL);
			modify_event_report_timer(60*1000);		
		}
		
		if(TRANS_PROTO_698 == ProtocolType)
		{
			if(DoublePrtcl_698_e != DevicePib_t.MeterPrtcltype)
			{
				DevicePib_t.MeterPrtcltype = DoublePrtcl_698_e;
				staflag = TRUE;
			}
		}
		else if(TRANS_PROTO_97 == ProtocolType || TRANS_PROTO_07 == ProtocolType)
		{
			if(DoublePrtcl_645_e != DevicePib_t.MeterPrtcltype)
			{
				DevicePib_t.MeterPrtcltype = DoublePrtcl_645_e;
				staflag = TRUE;
			}
		}
	}
}

void sta_read_app_read_meter_ind_proc(READ_METER_IND_t *pReadMeterInd)
{
    U8  protocol;
    U16 remainlen = pReadMeterInd->AsduLength;
	U16 len = remainlen;
    U8  *pPayload = pReadMeterInd->Asdu;

    U8  cnt=0;
    U8  lastFlag = FALSE;
	U16 offset=0;
    // U16 timeout = 0;
    U8  intervaltime = 0;    
    U8  retry = 0;    
    U8  retryFromCCO = 0;
    U8  MeterAddr[6]    = {0};
    U8  RealAddr[6] = {0};
    U8  BaudCnt = 0;
    U8  readMeterNum = 0;

#if defined(ZC3750STA) 
    U8  BoradAddr_645[6]={0x99,0x99,0x99,0x99,0x99,0x99};
    U8  AA_645[6]={0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};

    if(zc_timer_query(sta_bind_timer) == TMR_RUNNING)
    {
        app_printf("sta_bind_timer is Running, not readmeter\n");
        return;
    }
#endif
    app_printf("protocol: %dSeq: %dlen:%d\n", pReadMeterInd->ProtocolType,pReadMeterInd->ApsSeq,pReadMeterInd->AsduLength);
    // dump_buf(pPayload,pReadMeterInd->AsduLength);
    if(getHplcTestMode() == RD_CTRL && rd_ctrl_info.recupflag == FALSE)
	{
		U8 addr[MACADRDR_LENGTH] = {0};
		U8 nulladdr[MACADRDR_LENGTH] = {0};
		__memcpy(addr,rd_ctrl_info.mac_addr,MACADRDR_LENGTH);
		ChangeMacAddrOrder(addr);
		app_printf("readmeterapsseq:%d  pReadMeterInd->ApsSeq:%d\n", rd_ctrl_info.readseq, pReadMeterInd->ApsSeq);
		dump_buf(addr,MACADRDR_LENGTH);
		dump_buf(pReadMeterInd->SrcMacAddr,MACADRDR_LENGTH);
		if(rd_ctrl_info.readseq == pReadMeterInd->ApsSeq && 
			(0 == memcmp(addr,pReadMeterInd->SrcMacAddr,MACADRDR_LENGTH)||
			0 == memcmp(nulladdr,pReadMeterInd->SrcMacAddr,MACADRDR_LENGTH)))
		{
			dump_buf(pReadMeterInd->Asdu,pReadMeterInd->AsduLength);
			uart_send(pReadMeterInd->Asdu,pReadMeterInd->AsduLength);
			rd_ctrl_info.recupflag = TRUE;
		}
		return;
	}
    //针对点抄，如果aps层序号不增加，默认使用本地存储的抄表结果
    if(pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_RM
            && pReadMeterInd->ApsSeq == gRm13f1RecvDataInfo.RM13F1ApsRecvPacketSeq
            && gRm13f1RecvDataInfo.RM13F1recivelen > 0)
    {
        sta_read_meter_req(1, pReadMeterInd->ProtocolType, pReadMeterInd->ReadMode,
                         pReadMeterInd->stei, pReadMeterInd->SrcMacAddr,
                         pReadMeterInd->ApsSeq, gRm13f1RecvDataInfo.RM13F1recivebuf, gRm13f1RecvDataInfo.RM13F1recivelen);
        app_printf("ReadMeterReq Use RM13F1recive!!!\n");
        return;
    }

    if(pReadMeterInd->ProtocolType == TRANS_PROTO_97 || pReadMeterInd->ProtocolType == TRANS_PROTO_07)
    {
        protocol = DLT645_2007;
    }
    else if(pReadMeterInd->ProtocolType == TRANS_PROTO_698)
    {
        protocol = DLT698;
    }
    else if(pReadMeterInd->ProtocolType == TRANS_PROTO_RF)
	{
		protocol = e_T188_MSG;
	}
	else //if(protocol == APP_T_PROTOTYPE)
	{
		protocol = APP_T_PROTOTYPE;
	}

    // timeout = (pReadMeterInd->Timeout < 1) ? 1:pReadMeterInd->Timeout;
    // timeout = (pReadMeterInd->Timeout > 100) ? 100:pReadMeterInd->Timeout;

    do
    {
        if(pReadMeterInd->ProtocolType == TRANS_PROTO_97 || pReadMeterInd->ProtocolType == TRANS_PROTO_07)
	    {
	        //第一帧判断本次收到的645帧数量，是否还能添加进串口队列
	        if(cnt == 0)
            {   
    	        readMeterNum = Check645FrameNum(pReadMeterInd->Asdu, pReadMeterInd->AsduLength);
                app_printf("readMeterNum %d\n",readMeterNum);
                if(readMeterNum > (SERIAL_CACHE_LIST.thr - SERIAL_CACHE_LIST.nr))
                {
                    printf_s("too many 645\n");
                    return;
                }
            }
	  	    len = 0;
            offset = 0;
            if(e_Decode_Success==ScaleDLT(pReadMeterInd->ProtocolType, pPayload, &offset, &remainlen,&len))
            {
			    U8  ControlCode = 0;
			    U8  FECount     = 0;
			    U8  DataLen     = 0;
                
                GetMacAddr(RealAddr);
                get_info_from_645_frame(&pPayload[offset], len, MeterAddr, &ControlCode, &DataLen, &FECount);

                if(memcmp(MeterAddr,RealAddr,6) == 0 && ControlCode == DL645EX_CTRCODE )
                {
                    U8  *Sendbuf = NULL;
                    Sendbuf = zc_malloc_mem(255, "ProRdDLT645Extend", MEM_TIME_OUT);
                    U8  SendBufLen = 0;
                    extern void ProRdDLT645Extend(U8 *pMsgData , U16 Msglength, U8 *Sendbuf, U8 *SendBufLen);
                    ProRdDLT645Extend(pPayload , len, Sendbuf, &SendBufLen);
                    app_printf("Process_Extend645 %d!\n",len);

                    sta_read_meter_req(1, pReadMeterInd->ProtocolType, pReadMeterInd->ReadMode,
                                     pReadMeterInd->stei, pReadMeterInd->SrcMacAddr,
                                     pReadMeterInd->ApsSeq, Sendbuf, SendBufLen);
                    zc_free_mem(Sendbuf);

                    return;
				}
                #if defined(ZC3750STA) 
                if(memcmp(MeterAddr,RealAddr,6) != 0 
                             && memcmp(MeterAddr,BoradAddr_645,6) != 0
                    #ifdef TH2CJQ
                        && (TH2CJQ_CheckMeterExistByAddr(MeterAddr, &protocol) == FALSE)
                    #endif
                    )
                {
				    pPayload += (offset+len);
                    continue;
                }
                //河南双协议表事件上报赋值
                if(PROVINCE_HENAN == app_ext_info.province_code)
                {
                    sta_read_meter_judge_double_prtc(pReadMeterInd->ProtocolType);
                }
                #elif defined(ZC3750CJQ2) 
                if(CJQ_CheckMeterExistByAddr(MeterAddr, &protocol, &BaudCnt) == TRUE)
            	{
            		app_printf("this meter baud =%d,%d\n",baud_enum_to_int(BaudCnt),protocol);
            	}
                else
                {
                    app_printf("other meter baud =%d,%d\n",BaudCnt,protocol);
                    return;
                }
			    #endif
            }
            else
            {
                len = 0;
			    remainlen = 0;
            }
            
            pPayload += offset;
        }
        else if(pReadMeterInd->ProtocolType == TRANS_PROTO_698)
        {
			if(TRUE == Check698Frame(pPayload,len,NULL,MeterAddr, NULL))
			{
			    readMeterNum = 1;//698只支持一条，和645保持一致，直接置last标志
			    #if defined(ZC3750STA) 
				if(memcmp(MeterAddr,DevicePib_t.DevMacAddr,6) != 0)
				{
					if(memcmp(MeterAddr,AA_645,6) != 0
                    #ifdef TH2CJQ
                        && (TH2CJQ_CheckMeterExistByAddr(MeterAddr, &protocol) == FALSE)
                    #endif
                    )
                    {
                        return;
                    }
				}
                //河南双协议表事件上报赋值
                if(PROVINCE_HENAN == app_ext_info.province_code)
                {
                    sta_read_meter_judge_double_prtc(pReadMeterInd->ProtocolType);
                }
                #elif defined(ZC3750CJQ2) 
                
	            
                if(CJQ_CheckMeterExistByAddr(MeterAddr, &protocol, &BaudCnt) == TRUE)
            	{
            		app_printf("this meter baud =%d,%d\n",baud_enum_to_int(BaudCnt),protocol);
            	}
			    #endif
			}
            else
            {
                pPayload ++;
			    remainlen --;
                len --;
                continue;
            }
            remainlen = 0;
        }
        else if(protocol == e_T188_MSG)
	    {
            readMeterNum = 1;
        }
        else if(protocol == APP_T_PROTOTYPE)//透传
	    {
            GetMacAddr(RealAddr);
#if defined(ZC3750STA)
            if(TRUE == Check698Frame(pPayload,len,NULL,MeterAddr, NULL) && (0 != memcmp(RealAddr, MeterAddr, 6))
            #ifdef TH2CJQ
            && (TH2CJQ_CheckMeterExistByAddr(MeterAddr, NULL) == FALSE)
            #endif
            )
#elif defined(ZC3750CJQ2)
            if(TRUE == Check698Frame(pPayload,len,NULL,MeterAddr, NULL) && CJQ_CheckMeterExistByAddr(MeterAddr, NULL, &BaudCnt) == FALSE)
#endif
            {

                app_printf("APP_T_PROTOTYPE 698 but MacAddr not \n");
                app_printf("MeterAddr->MeterAddr= ");
                dump_buf(MeterAddr, 6);
                app_printf("RealAddr= ");
                dump_buf(RealAddr, 6);
                return;
            }
#if defined(ZC3750STA)
            else if(TRUE == Check645Frame(pPayload, len, NULL, MeterAddr, NULL) && (0 != memcmp(RealAddr, MeterAddr, 6))
            #ifdef TH2CJQ
            && (TH2CJQ_CheckMeterExistByAddr(MeterAddr, NULL) == FALSE)
            #endif
            )
#else
            else if(TRUE == Check645Frame(pPayload, len, NULL, MeterAddr, NULL) && CJQ_CheckMeterExistByAddr(MeterAddr, NULL, &BaudCnt) == FALSE)
#endif
            {
                app_printf("APP_T_PROTOTYPE 07 but MacAddr not \n");
                app_printf("MeterAddr= ");
                dump_buf(MeterAddr, 6);
                app_printf("RealAddr= ");
                dump_buf(RealAddr, 6);
                return;
            }
            #if defined(ZC3750CJQ2)
                app_printf("this meter baud =%d,%d\n", baud_enum_to_int(BaudCnt), protocol);
            #endif
            readMeterNum = 1;
			remainlen=0;
	    }

        cnt ++;
        if(readMeterNum > 0 && cnt == readMeterNum)
        {
        	//StaReadMeterUpData.RecvLen	  = 0;
			//StaReadMeterUpData.OptionWord = 0;
			lastFlag = LAST_ACTIVE;
        }
        else
        {
            //if(cnt == 0)
            {
                //lastFlag = FIRST_ACTIVE;
            }
            lastFlag = MIDDLE_ACTIVE;
        }

        //app_printf("(cnt<<8|pReadMeterInd->ApsSeq&0xFF) == %04x\n",(cnt<<8|(pReadMeterInd->ApsSeq&0xFF)));
        if(pReadMeterInd->ReadMode == e_CONCENTRATOR_ACTIVE_CONCURRENT)
        {
            U8 cfgword = 0;
            cfgword = pReadMeterInd->CfgWord;
            //retry = ((cfgword&0x01) == 1)?(MIN(((cfgword>>2)&0x03), 3)):0;
            retry = 2;
            retryFromCCO = ((cfgword&0x01) == 1)?(MIN(((cfgword>>2)&0x03), 3)):0;
            //retry = 0;
            if(cnt > 1)
            {
                intervaltime = (pReadMeterInd->OptionWord/SERICAL_PER);//+((pReadMeterInd->OptionWord%10)?1:0);//超过100ms，按照除数来
            }
            //printf_s("cfgword = %d ,OptionWord %d,intervaltime = %d ,retry = %d\n", cfgword,pReadMeterInd->OptionWord,intervaltime, retry);
        }

        app_printf("Timeout=%dms, retryFromCCO=%d\n", (pReadMeterInd->Timeout * 100), retryFromCCO);
#if defined(ZC3750CJQ2) 
        cjq2_search_suspend_timer_modify(20 * TIMER_UNITS);
#endif
        //二采使用载波协议类型，可支持双协议
#if defined(ZC3750STA) 
        if(zc_timer_query(sta_bind_timer) == TMR_RUNNING)
	    {
	    	app_printf("sta_bind_timer is Running, not readmeter\n");
			return;
	    }
#endif
        sta_read_meter_put_list(pReadMeterInd, pReadMeterInd->ProtocolType,BaudCnt,
        100*10,intervaltime, retry,(cnt<<8|(pReadMeterInd->ApsSeq&0xFF)), len, pPayload, lastFlag,1);
        pPayload += len;
	}while(remainlen > 0 && remainlen < pReadMeterInd->AsduLength);

    return;
}

/***************重抄直接根据序号回复**********************/
static void sta_read_clear_13f1_rm_data_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    gRm13f1RecvDataInfo.RM13F1ApsRecvPacketSeq = 0xffff;
    gRm13f1RecvDataInfo.RM13F1recivelen        = 0;
    memset(gRm13f1RecvDataInfo.RM13F1recivebuf, 0, sizeof(gRm13f1RecvDataInfo.RM13F1recivebuf));
    app_printf("RM13F1recivelen clear!\n");
}

ostimer_t *Clear13F1RMdataTimer = NULL;
RM13F1RCVDATA_t gRm13f1RecvDataInfo;
void sta_read_clear_13f1_rm_data_timer_init(void)
{
    if(Clear13F1RMdataTimer == NULL)
    {
        Clear13F1RMdataTimer = timer_create(30*1000,
                                            0,
                                            TMR_TRIGGER ,//TMR_TRIGGER
                                            sta_read_clear_13f1_rm_data_timer_cb,
                                            NULL,
                                            "Clear13F1RMdataTimerCB"
                                            );
    }

    gRm13f1RecvDataInfo.RM13F1ApsRecvPacketSeq = 0xffff;
    gRm13f1RecvDataInfo.RM13F1recivelen        = 0;
    memset(gRm13f1RecvDataInfo.RM13F1recivebuf, 0, sizeof(gRm13f1RecvDataInfo.RM13F1recivebuf));
}

/*
void F1F1RecBufInit(void)
{
    U8 i;
	for(i=0;i<MaxRecordNum;i++)
	{
	    F1F1RecBuf[i].F1F1ApsSeq = 0xffff;
	    memset(F1F1RecBuf[i].Addr,0xff,6);
        F1F1RecBuf[i].F1F1recivelen = 0;
        memset(F1F1RecBuf[i].F1F1recivebuf,0x00,sizeof(F1F1RecBuf[i].F1F1recivebuf));
    	F1F1RecBuf[i].LifeTime = 0;
        F1F1RecBuf[i].OptionWord = 0;
	}
}
U8 CheckMeterSeq(U16 AsduSeq , U8 *Addr,U8 *Cnt)
{
	U8 i;

	for(i=0;i<MaxRecordNum;i++)
	{
		if(AsduSeq==F1F1RecBuf[i].F1F1ApsSeq && memcmp(F1F1RecBuf[i].Addr,Addr,6)==0&&F1F1RecBuf[i].LifeTime > 0)
		{
			app_printf("F1F1RecBuf %d exsit!\n",i);
			*Cnt = i;
			return FALSE;	
		}
	}

	return TRUE;
}

U8 AddMeterSeq(U16 AsduSeq ,U8 *Addr)
{
	U8 i = 0xff;
    U8 ff_Addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
    
    if(zc_timer_query(MeterApsChecktimer) == TMR_STOPPED)
    {
        Modify_MeterApsChecktimer(4);
    }
	for(i=0;i<MaxRecordNum;i++)
	{
		if(0xffff== F1F1RecBuf[i].F1F1ApsSeq && memcmp(F1F1RecBuf[i].Addr,ff_Addr,6)==0)
		{
            app_printf("F1F1RecBuf %d add!\n",i);
			break;
		}
	}
	//位置满,first in first out
	if(i == MaxRecordNum)
	{
	    i = MaxRecordNum-1;
		memmove((U8*)&F1F1RecBuf[0],(U8*)&F1F1RecBuf[1],(MaxRecordNum-1)*sizeof(RMF1F1RCVDATA_t));
		
        app_printf("F1F1RecBuf %d renew!\n",i);
	}
    F1F1RecBuf[i].F1F1ApsSeq = AsduSeq;
    F1F1RecBuf[i].F1F1recivelen = 0;
	__memcpy(F1F1RecBuf[i].Addr,Addr,6);
    memset(F1F1RecBuf[i].F1F1recivebuf,0x00,sizeof(F1F1RecBuf[i].F1F1recivebuf));
	F1F1RecBuf[i].LifeTime = 6;
    F1F1RecBuf[i].OptionWord = 0;
    
    return i;
}

void UpMeterSeq()
{
	U8 i ,j;
	U8 ff_Addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
	for(i=0;i<MaxRecordNum;i++)
	{
		if(0xffff != F1F1RecBuf[i].F1F1ApsSeq && memcmp(F1F1RecBuf[i].Addr,ff_Addr,6)!=0)
		{
			F1F1RecBuf[i].LifeTime--;
			if(F1F1RecBuf[i].LifeTime == 0)
			{
				memset((U8*)&F1F1RecBuf[i],0xff,sizeof(RMF1F1RCVDATA_t));
			}
		}

	}
	for(i=0;i<MaxRecordNum;i++)//将空的位置，剔除
	{
		if(0xffff != F1F1RecBuf[i].F1F1ApsSeq && memcmp(F1F1RecBuf[i].Addr,ff_Addr,6)!=0)
		{
			if(i== MaxRecordNum-1)return;//如果最后一个位置为空，更新完毕
			for(j=i+1;j<MaxRecordNum;j++)
			{
				if(0xffff != F1F1RecBuf[i].F1F1ApsSeq && memcmp(F1F1RecBuf[i].Addr,ff_Addr,6)!=0)
				{
					__memcpy((U8*)&F1F1RecBuf[i],(U8*)&F1F1RecBuf[j],sizeof(RMF1F1RCVDATA_t));
					memset((U8*)&F1F1RecBuf[j],0xff,sizeof(RMF1F1RCVDATA_t));
					break;
				}
			}
		}
	}
    Modify_MeterApsChecktimer(4);
}
ostimer_t *MeterApsChecktimer = NULL;

void MeterApsChecktimerCB(struct ostimer_s *ostimer, void *arg)
{
    //app_printf("MeterApsChecktimerCB!\n");
    UpMeterSeq();
}
void Modify_MeterApsChecktimer(U32 Sec)
{
    if(MeterApsChecktimer == NULL)
    {
        MeterApsChecktimer = timer_create(Sec*100,
	                             0,
	                             TMR_TRIGGER ,
	                             MeterApsChecktimerCB,
	                             NULL,
	                             "MeterApsChecktimerCB"
	                            );
        F1F1RecBufInit();
    }
    else
    {
        timer_modify(MeterApsChecktimer,
			   Sec*100, 
               0,
               TMR_TRIGGER ,
               MeterApsChecktimerCB,
               NULL,
               "MeterApsChecktimerCB"
               );
    }
    timer_start(MeterApsChecktimer);
}

*/
#endif