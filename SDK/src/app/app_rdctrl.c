#include "app_rdctrl.h"
#include "printf_zc.h"
#include "Scanbandmange.h"
#include "DataLinkInterface.h"
#include "DataLinkGlobal.h"
#include "dl_mgm_msg.h"
#include "app_base_serial_cache_queue.h"
#include "test.h"
#include "app_gw3762.h"
#include "dev_manager.h"
#include "app_dltpro.h"
#include "netnnib.h"
#include "app_meter_bind_sta.h"
#include "meter.h"
#include "app_rdctrl.h"

/***************************抄控器扩展使用****************************/
RdCtrlLink_t rd_ctrl_info =
{
    .linktype = e_RF_Link,
    .dtei = 0,
    .recsyflag = 0,
    .recupflag = 0,
    .send_sack_cnt = 0,
    .period_sendsyh_flag = FALSE,
    .readseq = 0,
};

U8 g_ReadIdType;
U16 g_RdApsIdSeq = 0;
ostimer_t *g_RdReadIdTimer = NULL;
U8 g_RdReadIdTimes = 0;
ostimer_t *g_Sendsyhtimer = NULL;
ostimer_t *g_SendsyhtimeoutTimer = NULL;
static void send_syh_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	if (FALSE == rd_ctrl_info.period_sendsyh_flag)
	{
		app_printf("period send syh frame!!!!!flase!!!!!!!!!\n");
		rd_ctrl_info.send_sack_cnt++;
		if (rd_ctrl_info.send_sack_cnt >= 5)
		{
			rd_ctrl_info.send_sack_cnt = 0;
			rd_ctrl_info.period_sendsyh_flag = TRUE;
			timer_modify(g_Sendsyhtimer,
						 30 * 1000,
						 30 * 1000,
						 TMR_PERIODIC, // TMR_TRIGGER
						 send_syh_timer_cb,
						 NULL,
						 "send_syh_timer_cb",
						 TRUE);
		}
		else
		{
			timer_modify(g_Sendsyhtimer,
						 100,
						 100,
						 TMR_TRIGGER, // TMR_TRIGGER
						 send_syh_timer_cb,
						 NULL,
						 "send_syh_timer_cb",
						 TRUE);
		}
		timer_start(g_Sendsyhtimer);
	}
	else
	{
		app_printf("period send syh frame!!!!!!!true!!!!!!!!!\n");
	}
	ProcSackCtrlRequest(2, NULL, rd_ctrl_info.linktype);
}

void modify_send_syh_timer(U32 TimeMs)
{
	if (TimeMs == 0)
		TimeMs = 100;

	if (g_Sendsyhtimer == NULL)
	{
		g_Sendsyhtimer = timer_create(TimeMs, 
		                            TimeMs,
									TMR_TRIGGER, 
									send_syh_timer_cb, 
									NULL, 
									"send_syh_timer_cb");
	}
	else
	{
		timer_modify(g_Sendsyhtimer,
					 TimeMs,
					 TimeMs,
					 TMR_TRIGGER, // TMR_TRIGGER
					 send_syh_timer_cb,
					 NULL,
					 "send_syh_timer_cb",
					 TRUE);
	}

	if (g_Sendsyhtimer)
	{
		timer_start(g_Sendsyhtimer);
	}
}

static void send_syh_timeout_timer_cb(struct ostimer_s *ostimer , void *arg)
{
    net_printf("send_syh_timeout_timer_cb\n");
    if(TMR_RUNNING == zc_timer_query(g_Sendsyhtimer))
    {
        rd_ctrl_info.send_sack_cnt = 0;
        rd_ctrl_info.period_sendsyh_flag = FALSE;

        timer_stop(g_Sendsyhtimer,TMR_NULL);

    }
#if defined(STATIC_NODE)
        if(GetNodeState() == e_NODE_OUT_NET)
        {
            ScanBandManage(e_INIT, 0);
            //断开抄控器连接，启动扫频
            ScanRfChMange(e_INIT, 0);
        }
#endif

}

void modify_send_syh_timeout_timer(U32 TimeMs)
{
	if (TimeMs == 0)
		TimeMs = 3 * 60 * 1000;

	if (g_SendsyhtimeoutTimer == NULL)
	{
		g_SendsyhtimeoutTimer = timer_create(TimeMs, 
		                                   TimeMs, 
										   TMR_TRIGGER, 
										   send_syh_timeout_timer_cb, 
										   NULL, 
										   "send_syh_timeout_timer_cb");
	}
	else
	{
		timer_modify(g_SendsyhtimeoutTimer,
					 TimeMs,
					 TimeMs,
					 TMR_TRIGGER, // TMR_TRIGGER
					 send_syh_timeout_timer_cb,
					 NULL,
					 "send_syh_timer_cb",
					 TRUE);
	}

	if (g_SendsyhtimeoutTimer)
		timer_start(g_SendsyhtimeoutTimer);
}

void proc_rd_ctrl_indication(RD_CTRL_REQ_t *pRdCtrlData)
{
	app_printf("proc_rd_ctrl_indication\n");
	app_printf("protocol: %d, tei:%06x, len: %d >> ", pRdCtrlData->protocol, pRdCtrlData->dtei, pRdCtrlData->len);
	dump_level_buf(DEBUG_APP, pRdCtrlData->Asdu, pRdCtrlData->len);

	//记录抄控器地址信息
	rd_ctrl_info.dtei = pRdCtrlData->dtei;
	//    rd_ctrl_info.macUse = pRdCtrlData->MacUsed;
	__memcpy(rd_ctrl_info.mac_addr, pRdCtrlData->SrcMacAddr, MAC_ADDR_LEN);

#if defined(STATIC_MASTER)
	proc_app_gw13762_data(pRdCtrlData->Asdu, pRdCtrlData->len);
#endif
}

void proc_rd_ctrl_send_data(U8 *data, U16 len)
{
	RD_CTRL_REQ_t *pRdCtrlReq = zc_malloc_mem(sizeof(RD_CTRL_REQ_t) + len, "rdreq", MEM_TIME_OUT);

	pRdCtrlReq->protocol = 0; //默认为 0 表示 Q/GDW 1376.2 协议
	pRdCtrlReq->len = len;
	pRdCtrlReq->dtei = rd_ctrl_info.dtei;
	//    pRdCtrlReq->MacUsed = rd_ctrl_info.macUse;

	__memcpy(pRdCtrlReq->DstMacAddr, rd_ctrl_info.mac_addr, MAC_ADDR_LEN);
#if defined(STATIC_MASTER)
	__memcpy(pRdCtrlReq->SrcMacAddr, GetNnibMacAddr(), MAC_ADDR_LEN);
#endif
	__memcpy(pRdCtrlReq->Asdu, data, pRdCtrlReq->len);

	ApsRdCtrlRequest(pRdCtrlReq);

	zc_free_mem(pRdCtrlReq);
}

static void rd_ctrl_to_uart_timeout(void *pMsgNode)
{
	add_serialcache_msg_t *pSerialMsg = (add_serialcache_msg_t *)pMsgNode;
	if (pSerialMsg->baud != 0)
	{
		#if defined(STATIC_MASTER)
		meter_serial_cfg(g_meter->baudrate, 0);
		#elif defined(ZC3750STA)
		meter_serial_cfg(sta_bind_info_t.BaudRate, 0);
		#endif
	}
}

static void proc_rd_ctrl_to_uart_send_data(U8 *data, U16 len)
{
	RD_CTRLT_TO_UART_REQ_t *p_rd_ctrl_to_uart_req = zc_malloc_mem(sizeof(RD_CTRLT_TO_UART_REQ_t) + len, "rduartreq", MEM_TIME_OUT);
	U8 ckq_addr[MAC_ADDR_LEN] = {0xdd,0xdd,0xdd,0xdd,0xdd,0xdd};

	p_rd_ctrl_to_uart_req->protocol = 0; //默认为 0 表示 Q/GDW 1376.2 协议
	p_rd_ctrl_to_uart_req->len = len;
	p_rd_ctrl_to_uart_req->dtei = CTRL_TEI_GW;
	p_rd_ctrl_to_uart_req->dirflag = 0;
	p_rd_ctrl_to_uart_req->baud = 0;

	__memcpy(p_rd_ctrl_to_uart_req->DstMacAddr, ckq_addr, MAC_ADDR_LEN);
// #if defined(STATIC_MASTER)
	__memcpy(p_rd_ctrl_to_uart_req->SrcMacAddr, GetNnibMacAddr(), MAC_ADDR_LEN);
// #endif
	__memcpy(p_rd_ctrl_to_uart_req->Asdu, data, p_rd_ctrl_to_uart_req->len);

	ApsRdCtrlToUartRequest(p_rd_ctrl_to_uart_req);

	zc_free_mem(p_rd_ctrl_to_uart_req);
}

static void rd_ctrl_to_uart_recv_deal(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
	add_serialcache_msg_t *pSerialMsg = (add_serialcache_msg_t *)pMsgNode;

	app_printf("ptcl=%d len=%d\n", protocoltype, revlen);
	if (revlen > 0)
	{
		proc_rd_ctrl_to_uart_send_data(revbuf, revlen);
	}
	if (pSerialMsg->baud != 0)
	{
		#if defined(STATIC_MASTER)
		meter_serial_cfg(g_meter->baudrate, 0);
		#elif defined(ZC3750STA)
		meter_serial_cfg(sta_bind_info_t.BaudRate, 0);
		#endif
	}
	return;
}

static void rd_ctrl_send_to_uart_put_list(U16 stei, U8 *SrcMacAddr, U32 baud, U16 timeout,
										  U16 frameSeq, U16 frameLen, U8 *pPayload)
{
	add_serialcache_msg_t serialmsg;
	memset((U8 *)&serialmsg, 0x00, sizeof(add_serialcache_msg_t));

	// app_printf("SendLocalFrame-------------------------\n");
	// app_printf("lastFlag = %d\n", lastFlag);

	serialmsg.StaReadMeterInfo.ApsSeq = 0;
	serialmsg.StaReadMeterInfo.Dtei = stei;
	serialmsg.StaReadMeterInfo.ProtocolType = 0;
	serialmsg.StaReadMeterInfo.ReadMode = 0;
	serialmsg.StaReadMeterInfo.LastFlag = 0;
	__memcpy(serialmsg.StaReadMeterInfo.DestAddr, SrcMacAddr, 6);

	serialmsg.baud = baud;
	serialmsg.Uartcfg = (baud == 0) ? NULL : meter_serial_cfg;
	serialmsg.EntryFail = NULL;
	serialmsg.SendEvent = NULL;
	serialmsg.ReSendEvent = NULL;
	serialmsg.MsgErr = NULL;

	serialmsg.Match = NULL;
	serialmsg.Timeout = NULL;
	serialmsg.RevDel = NULL;

	serialmsg.Protocoltype = 0;
	serialmsg.DeviceTimeout = timeout;
	serialmsg.IntervalTime = 0;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = frameLen;
	serialmsg.FrameSeq = frameSeq;
#if defined(STATIC_MASTER)
	serialmsg.lid = e_Serial_AFN13F1;
#else
	serialmsg.lid = e_Serial_Trans;
#endif
	serialmsg.ReTransmitCnt = 0;
	serialmsg.pSerialheader = &SERIAL_CACHE_LIST;

	extern ostimer_t *serial_cache_timer;
	serialmsg.CRT_timer = serial_cache_timer;

	serial_cache_add_list(serialmsg, pPayload, FALSE);

	return;
}

static void rd_ctrl_to_uart_put_list(U16 stei, U8 *SrcMacAddr, U32 baud, U16 timeout,
									 U16 frameSeq, U16 frameLen, U8 *pPayload)
{
	add_serialcache_msg_t serialmsg;
	memset((U8 *)&serialmsg, 0x00, sizeof(add_serialcache_msg_t));

	// app_printf("SendLocalFrame-------------------------\n");
	// app_printf("lastFlag = %d\n", lastFlag);

	serialmsg.StaReadMeterInfo.ApsSeq = 0;
	serialmsg.StaReadMeterInfo.Dtei = stei;
	serialmsg.StaReadMeterInfo.ProtocolType = 0;
	serialmsg.StaReadMeterInfo.ReadMode = 0;
	serialmsg.StaReadMeterInfo.LastFlag = 0;
	__memcpy(serialmsg.StaReadMeterInfo.DestAddr, SrcMacAddr, MAC_ADDR_LEN);

	serialmsg.baud = baud;
	serialmsg.Uartcfg = (baud == 0) ? NULL : meter_serial_cfg;
	serialmsg.EntryFail = NULL;
	serialmsg.SendEvent = NULL;
	serialmsg.ReSendEvent = NULL;
	serialmsg.MsgErr = NULL;

	serialmsg.Match = NULL;
	serialmsg.Timeout = rd_ctrl_to_uart_timeout;
	serialmsg.RevDel = rd_ctrl_to_uart_recv_deal;

	serialmsg.Protocoltype = 0;
	serialmsg.DeviceTimeout = timeout;
	serialmsg.IntervalTime = 0;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = frameLen;
	serialmsg.FrameSeq = frameSeq;
#if defined(STATIC_MASTER)
	serialmsg.lid = e_Serial_AFN13F1;
#else
	serialmsg.lid = e_Serial_Trans;
#endif
	serialmsg.ReTransmitCnt = 0;
	serialmsg.pSerialheader = &SERIAL_CACHE_LIST;

	extern ostimer_t *serial_cache_timer;
	serialmsg.CRT_timer = serial_cache_timer;

	serial_cache_add_list(serialmsg, pPayload, FALSE);

	return;
}

//抄控器接收载波上行处理
void proc_rd_ctrl_to_uart_indication(RD_CTRLT_TO_UART_REQ_t *p_rdctrl_to_uart_data)
{
	static U16 Seq = 0;
	app_printf("proc_rd_ctrl_to_uart_indication ! \n");
	app_printf("protocol: %d,tei:%06x,dir: %d,baud %d,len: %d >> ", p_rdctrl_to_uart_data->protocol,
			   p_rdctrl_to_uart_data->dtei, p_rdctrl_to_uart_data->dirflag, p_rdctrl_to_uart_data->baud, p_rdctrl_to_uart_data->len);
	dump_level_buf(DEBUG_APP, p_rdctrl_to_uart_data->Asdu, p_rdctrl_to_uart_data->len);

	if (getHplcTestMode() == RD_CTRL)
	{
		//修改波特率进行串口转发
		if (p_rdctrl_to_uart_data->dirflag == 0 && rd_ctrl_info.recupflag == FALSE)
		{
			rd_ctrl_info.recupflag = TRUE;
			rd_ctrl_send_to_uart_put_list(p_rdctrl_to_uart_data->dtei, p_rdctrl_to_uart_data->DstMacAddr, p_rdctrl_to_uart_data->baud, 200,
										  ++Seq, p_rdctrl_to_uart_data->len, p_rdctrl_to_uart_data->Asdu);
		}
	}
	else
	{


		//修改波特率进行串口转发
		if (p_rdctrl_to_uart_data->dirflag == 1)
		{
			if (p_rdctrl_to_uart_data->baud != 0)
			{
				//修改波特率，传输结束，修改回原波特率
				app_printf("need back baud\n");
			}
			//转发
			rd_ctrl_to_uart_put_list(p_rdctrl_to_uart_data->dtei, p_rdctrl_to_uart_data->DstMacAddr, p_rdctrl_to_uart_data->baud, 200,
									 ++Seq, p_rdctrl_to_uart_data->len, p_rdctrl_to_uart_data->Asdu);
		}
	}
}

U8 is_rd_ctrl_tei(U16 tei)
{
	return (GET_TEI_VALID_BIT(tei) == CTRL_TEI_BJ || GET_TEI_VALID_BIT(tei) == CTRL_TEI_GW);
}

void RdCtrlSendDataTonode(U8 *data, U16 len)
{
	RD_CTRLT_TO_UART_REQ_t *pRdCtrlToUartReq = zc_malloc_mem(sizeof(RD_CTRLT_TO_UART_REQ_t) + len, "rduartreq", MEM_TIME_OUT);

	pRdCtrlToUartReq->protocol = 0;
	pRdCtrlToUartReq->len = len;
	pRdCtrlToUartReq->dtei = rd_ctrl_info.dtei;
	pRdCtrlToUartReq->dirflag = 1;
	pRdCtrlToUartReq->baud = 0;

	__memcpy(pRdCtrlToUartReq->DstMacAddr, rd_ctrl_info.mac_addr, 6);
#if defined(ZC3750STA)
	__memcpy(pRdCtrlToUartReq->SrcMacAddr, DevicePib_t.DevMacAddr, 6);
#elif defined(STATIC_MASTER)
	__memcpy(pRdCtrlToUartReq->SrcMacAddr, FlashInfo_t.ucJZQMacAddr, 6);
#endif
	__memcpy(pRdCtrlToUartReq->Asdu, data, pRdCtrlToUartReq->len);

	rd_ctrl_info.linktype = e_HPLC_Link;//默认载波发送

	ApsRdCtrlToUartRequest(pRdCtrlToUartReq);
	zc_free_mem(pRdCtrlToUartReq);
}

static U8 rd_ctrl_send_search(U8 link_flag, U8 *link_addr)
{
	static U8 band = 0;
	U8 i, k;

	for (i = 0; i < 5; i++)
	{
		if (FALSE == link_flag)
		{
			band = (i % 4);
			// changeband(band);
			net_nnib_ioctl(NET_SET_BAND, &band);
			setHplcBand(band);
			printf_s("CKQ send search band is %d!\n", band);
		}
		for (k = 0; k < 3; k++)
		{
			ProcSackCtrlRequest(1, link_addr, rd_ctrl_info.linktype);
			printf_s("search frame send cnt is %d\n", k + 1);
			if (0 == RD_SACK_PEND)
			{
				rd_ctrl_info.recsyflag = 1;
				printf_s("RdCtrl Search success!!!");
				nnib.AODVRouteUse = TRUE;
				modify_send_syh_timeout_timer(0);
				// timer_start(g_SendsyhtimeoutTimer);
				return TRUE;
			}
			else
			{
				rd_ctrl_info.recsyflag = 0;
			}
		}
	}

	return FALSE;
}

static void rd_read_meter_req(RdCtrl645Info_t *frame_info)
{
	printf_s("rd_read_meter_req->\n");
// #if defined(STATIC_NODE)
// 	ScanBandManage(e_CNTCKQ, 0);
// 	ScanRfChMange(e_CNTCKQ, 0);
// #endif
	rd_ctrl_info.recsyflag = 0;
	rd_ctrl_info.recupflag = FALSE;
	__memcpy(rd_ctrl_info.mac_addr, frame_info->addr, MAC_ADDR_LEN);
	
	if (FALSE == rd_ctrl_send_search(FALSE, rd_ctrl_info.mac_addr))
	{
		rd_ctrl_info.recsyflag = FALSE;		
		printf_s("connect  fail ! \n");
		return;
	}
	else
	{
		rd_ctrl_info.recsyflag = TRUE;		
		printf_s("connect  success , start send to plc ! \n");
		// RdCtrlAfn13F1ReadMeterReq(frame_info->frame645, frame_info->frame645len, frame_info->Pro, frame_info->addr);
		RdCtrlSendDataTonode(frame_info->frame645, frame_info->frame645len);
	}
}

static void rd_module_id_req(U8 query_id_type)
{
	U8 addr[MAC_ADDR_LEN] = {0};
	QUERY_IDINFO_REQ_RESP_t query_id_info_req;

	query_id_info_req.direct = e_DOWNLINK;
	query_id_info_req.IdType = query_id_type; // APP_GW3762_MODULE_ID;
	query_id_info_req.destei = rd_ctrl_info.dtei;

	query_id_info_req.AsduLength = 0;
	query_id_info_req.PacketSeq = ++g_RdApsIdSeq;

	__memcpy(addr, rd_ctrl_info.mac_addr, MAC_ADDR_LEN);
	ChangeMacAddrOrder(addr);
	__memcpy(query_id_info_req.DstMacAddr, addr, MAC_ADDR_LEN);
	__memcpy(query_id_info_req.SrcMacAddr, rdaddr, MAC_ADDR_LEN);

	ApsQueryIdInfoReqResp(&query_id_info_req);

	return;
}

void g_RdReadIdTimercb(struct ostimer_s *ostimer, void *arg)
{
	static U8 cnt = 4;
	static U8 band = 2;
	if (0)
	{
		if (g_RdReadIdTimes < 15)
		{
			g_RdReadIdTimes++;
			band = (cnt++) % 4;
			app_printf("\n-----------changeband for %d-----------\n\n", band);
			// changeband(band);
			net_nnib_ioctl(NET_SET_BAND, &band);
			rd_module_id_req(g_ReadIdType);
		}
		else
		{
			g_RdReadIdTimes = 0;
			cnt = 4;
			timer_stop(g_RdReadIdTimer, TMR_NULL);
		}
	}
	if (1) // RF
	{
		if (g_RdReadIdTimes < 15)
		{
			g_RdReadIdTimes++;
			rd_module_id_req(g_ReadIdType);
			timer_start(g_RdReadIdTimer);
		}
		else
		{
			g_RdReadIdTimes = 0;
			rd_ctrl_info.recupflag = TRUE;
		}
	}
}

void modify_read_modeid_timer()
{
	if (g_RdReadIdTimer == NULL)
	{
		g_RdReadIdTimer = timer_create(3000,
										3000,
										TMR_TRIGGER,
										g_RdReadIdTimercb,
										NULL,
										"g_RdReadIdTimercb");
	}
	else
	{
		timer_modify(g_RdReadIdTimer,
					 3000,
					 3000,
					 TMR_TRIGGER, // TMR_TRIGGER
					 g_RdReadIdTimercb,
					 NULL,
					 "g_RdReadIdTimercb",
					 TRUE);
	}
}

void rd_read_meter_id(RdCtrl645Info_t *frame_info)
{
	printf_s("rd_read_meter_id->\n");

#if defined(ZC3750STA)
	ScanBandManage(e_CNTCKQ, 0);
	ScanRfChMange(e_CNTCKQ, 0);
	rd_ctrl_info.recsyflag = 0;
	rd_ctrl_info.recupflag = FALSE;
	__memcpy(rd_ctrl_info.mac_addr, frame_info->addr, MAC_ADDR_LEN);
	if (FALSE)
	{
		if (!rd_ctrl_send_search(FALSE, rd_ctrl_info.mac_addr))
		{
			rd_ctrl_info.recsyflag = FALSE;
			return;
		}
		rd_ctrl_info.recsyflag = TRUE;
		RdCtrlAfn13F1ReadMeterReq(frame_info->frame645, frame_info->frame645len, frame_info->Pro, frame_info->addr);
	}
#endif
	g_ReadIdType = frame_info->frame645[10];
	app_printf("g_ReadIdType : %d\n", g_ReadIdType);
	rd_ctrl_info.dtei = 0x0fff;
	g_RdReadIdTimes = 0;
	modify_read_modeid_timer();
	timer_start(g_RdReadIdTimer);
	timer_stop(g_RdReadIdTimer, TMR_CALLBACK);
}

Rd645Protocal_t ex_645_rd_protocal[] =
{
		{0x11, {0x33, 0x33, 0x33, 0x33}, rd_read_meter_req, "rd_read_meter_req"}, //载波抄表
		{0x1F, {0x01, 0x00, 0x00, 0x00}, rd_read_meter_id, "rd_read_meter_id"},	  //载波抄模块ID
};

static void rd_check_645frame(U8 *data, U8 data_len, U8 *ctrl_code, U8 *data_filed, U8 *addr, U8 *data_filed_len, U8 *fe_num)
{
	static U8 base_index;
	U8 i;

	for (i = 0; i < 13; i++)
	{
		if (*(data + i) == 0x68)
		{
			base_index = i;
			break;
		}
	}
	if (fe_num)
	{
		*fe_num = base_index;
	}
	if (addr)
	{
		__memcpy(addr, data + base_index + 1, MAC_ADDR_LEN);
	}
	if (ctrl_code)
	{
		*ctrl_code = *(data + base_index + 8);
	}
	if (data_filed)
	{
		__memcpy(data_filed, data + base_index + 10, 4);
	}
	*data_filed_len = *(data + base_index + 9);
}

void rd_ctrl_pro_dlt645_ex(U8 *pMsgData , U16 Msglength)
{
	static U8 addr_aa[MAC_ADDR_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
	static U8 fe_num = 0;
	U8 num = 0;
	RdCtrl645Info_t *Frameinfo = zc_malloc_mem(sizeof(RdCtrl645Info_t) + Msglength, "ProRdDLT645Extend", MEM_TIME_OUT);

	rd_check_645frame(pMsgData,Msglength,&Frameinfo->ctrlCode,Frameinfo->data,Frameinfo->addr,&Frameinfo->datalen,&fe_num);

	Frameinfo->Pro = DLT645_2007;
	__memcpy(Frameinfo->frame645, pMsgData, Msglength);
	Frameinfo->frame645len = Msglength;

	if ((Frameinfo->ctrlCode == 0x11) || (Frameinfo->ctrlCode == 0x13))
	{
		memset(Frameinfo->data, 0x33, sizeof(Frameinfo->data));
		if (memcmp(addr_aa, Frameinfo->addr, MAC_ADDR_LEN) == 0)
		{
			Frameinfo->ctrlCode = 0x13;
		}
	}
	else if (Frameinfo->ctrlCode == 0x1f && pMsgData[fe_num + 9] == 0x01)
	{
		memset(Frameinfo->data, 0x00, sizeof(Frameinfo->data));
		Frameinfo->data[0] = 0x01;
	}

	app_printf("ctrl : %02x, data : ", Frameinfo->ctrlCode);
	dump_buf(Frameinfo->data, 4);

	for (num = 0; num < sizeof(ex_645_rd_protocal) / sizeof(DLT_07_PROTOCAL); num++)
	{
		if ((ex_645_rd_protocal[num].ControlData == Frameinfo->ctrlCode) && (memcmp(Frameinfo->data, ex_645_rd_protocal[num].DataArea, 4) == 0))
		{
			ex_645_rd_protocal[num].Func(Frameinfo);
			app_printf("Find 645 data:%d\n", num);
			break;
		}
	}

	if (num == sizeof(ex_645_rd_protocal) / sizeof(DLT_07_PROTOCAL))
	{
		ex_645_rd_protocal[0].Func(Frameinfo);
	}

	zc_free_mem(Frameinfo);
	
}


