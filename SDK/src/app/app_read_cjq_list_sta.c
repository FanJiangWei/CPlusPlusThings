/*
 * Corp:	2023-2026 Beijing Zhongchen Microelectronics Co., LTD.
 * File:	app_read_cjq_list_sta.c
 * Date:	May 10, 2023
 * Author:	gaozhengzhou
 */

#include "app_read_cjq_list_sta.h"
#include "app_dltpro.h"
#include "app_meter_verify.h"
#include "printf_zc.h"
#include "app_base_serial_cache_queue.h"
#include "dev_manager.h"
#include "flash.h"
#include "app_exstate_verify.h"
#include "dl_mgm_msg.h"
#include "app_cnf.h"
#include "netnnib.h"

#if defined(ZC3750STA)

extern board_cfg_t BoardCfg_t;


TH2_METER_INFO_ST  TH2CJQ_MeterList = 
{
    .MeterNum = 0,
};

ostimer_t *ReadTH2Metertimer = NULL;
U8         ReadTH2MeterCnt = 0;
U8 change_address_flag = FALSE;


void ResetReadTH2MeterCnt(void)
{
    ReadTH2MeterCnt = 0;
}

void ReadTH2MeterCntSucc(void)
{
    ResetReadTH2MeterCnt();
}


void ReadTH2MeterCntFail(void)
{
    ReadTH2MeterCnt ++;
    if(ReadTH2MeterCnt > PLCRTTIMEOUTMAXCNT)
    {
        ResetReadTH2MeterCnt();
    }
    else
    {
        ReadTH2MeterListPut(DevicePib_t.ltu_addr, DLT645_2007, 1);
    }
}

U8 TH2CJQ_CheckMeterExistByAddr(U8* Addr, U8* Protocol)
{
	U8 ii = 0;
	
	for(ii = 0; ii < TH2CJQ_MeterList.MeterNum; ii++)
	{
		if(0 == memcmp(Addr, TH2CJQ_MeterList.MeterInfo[ii].MeterAddr, MACADRDR_LENGTH))
		{
            if(NULL != Protocol)
            {
                *Protocol = TH2CJQ_MeterList.MeterInfo[ii].Protocol;
            }

			return TRUE;//存在
		}
	}

    if(0 == memcmp(Addr, DevicePib_t.ltu_addr, MACADRDR_LENGTH))
    {
        if(NULL != Protocol)
        {
            *Protocol = DevicePib_t.ltu_protocol;
        }
        return TRUE;//存在
    }

	return FALSE;//不存在
}
void TH2CJQ_Extend_ShowInfo()
{
	U16 ii=0;
	app_printf("\r\nTH2CJQ Addr: %02x%02x%02x%02x%02x%02x\n",
				DevicePib_t.DevMacAddr[5],
				DevicePib_t.DevMacAddr[4],
				DevicePib_t.DevMacAddr[3],
				DevicePib_t.DevMacAddr[2],
				DevicePib_t.DevMacAddr[1],
				DevicePib_t.DevMacAddr[0]);
	
	
	app_printf("\r\nAddr         Protocol    TotalNum = %d\n",TH2CJQ_MeterList.MeterNum);
	
	for(ii = 0; ii < TH2CJQ_MeterList.MeterNum; ii++)
	{
		app_printf("%02x%02x%02x%02x%02x%02x  %04d        \n",
					TH2CJQ_MeterList.MeterInfo[ii].MeterAddr[5],
					TH2CJQ_MeterList.MeterInfo[ii].MeterAddr[4],
					TH2CJQ_MeterList.MeterInfo[ii].MeterAddr[3],
					TH2CJQ_MeterList.MeterInfo[ii].MeterAddr[2],
					TH2CJQ_MeterList.MeterInfo[ii].MeterAddr[1],
					TH2CJQ_MeterList.MeterInfo[ii].MeterAddr[0],
					TH2CJQ_MeterList.MeterInfo[ii].Protocol);
	}
    app_printf("DevicePib_t.Modularmode = %d\n", DevicePib_t.Modularmode);

}

static U8 ReadTH2MeterListMatch(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
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

static void ReadTH2MeterListTimeout(void *pMsgNode)
{
    ReadTH2MeterCntFail();
}

static void ReadTH2MeterListRecvDeal(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
    U8  Addr[6] = {0};
    U8  Item07Read[4] = {0x33, 0x33, 0xB4, 0x35};//07使用读取量测单元搜表列表
    U8  ValueBufLen = 0;
    U8  *ValueBuf = zc_malloc_mem(300, "ValueBuf", MEM_TIME_OUT);
    U8  i;
    U8  byLen = 0;
    U8  AllMeterNum = 0;
	U8	Protocol = 0;
    
    if(Check645Frame(revbuf,revlen,NULL,Addr,NULL) == TRUE)
    {
        if(GetValueBy645Frame(Item07Read,revbuf,revlen,ValueBuf,&ValueBufLen) == TRUE)
        {
            AllMeterNum = ValueBuf[byLen ++];
			// if(FALSE == FuncJudgeBCD(&AllMeterNum, sizeof(U8)))
			// {
			// 	printf_s("AllMeterNum %x NBCD\n",AllMeterNum);
            //     ReadTH2MeterCntFail();
            //     zc_free_mem(ValueBuf);
            //     return;
			// }

			// bcd_to_bin(&AllMeterNum, &AllMeterNum, sizeof(U8));
            if(AllMeterNum > TH2CJQ_METER_MAX)
            {
                printf_s("TH2CJQ_Num %d err\n",AllMeterNum);
                ReadTH2MeterCntFail();
                zc_free_mem(ValueBuf);
                return;
            }

            if(ValueBufLen < (AllMeterNum*7+1))
            {
                app_printf("ValueBufLen %d num %d err\n",ValueBufLen,AllMeterNum);
                ReadTH2MeterCntFail();
                zc_free_mem(ValueBuf);
                return;
            }
            //TH2CJQ_MeterList.MeterNum = AllMeterNum;
            app_printf("TH2CJQ_MeterNum %d Addr\n",AllMeterNum);
			//帧合格后先清除上次的列表，再赋值
			memset((U8*)&TH2CJQ_MeterList, 0x00, sizeof(TH2_METER_INFO_ST));
            for(i = 0;i < AllMeterNum; i ++)
            {
            	Protocol = ValueBuf[1+i*7+MACADRDR_LENGTH];
            	if(TRUE == FuncJudgeBCD(&ValueBuf[1+i*7], MACADRDR_LENGTH) && (Protocol >= DLT645_UN_KNOWN && Protocol <= DLT698))
	            {
	                memcpy(TH2CJQ_MeterList.MeterInfo[TH2CJQ_MeterList.MeterNum].MeterAddr,&ValueBuf[1+i*7],MACADRDR_LENGTH);
	                TH2CJQ_MeterList.MeterInfo[TH2CJQ_MeterList.MeterNum].Protocol = ValueBuf[1+i*7+MACADRDR_LENGTH];
	                dump_buf(TH2CJQ_MeterList.MeterInfo[TH2CJQ_MeterList.MeterNum].MeterAddr,7);
					TH2CJQ_MeterList.MeterNum++;
	            }
            }

            //设备类型判断
            TH2MeterChang();
            // 更新入网地址为LTU下挂电表地址
            change_network_access_address();

            ReadTH2MeterCntSucc();
            zc_free_mem(ValueBuf);
            return;
        }
    }
    ReadTH2MeterCntFail();
    zc_free_mem(ValueBuf);
}



void record_ltu_address(U8 *meter_address, U8 protocol)
{
    app_printf("record_ltu_address_and_Protocol\n");

    memcpy(DevicePib_t.ltu_addr, meter_address, 6);
    DevicePib_t.ltu_protocol = protocol;
}

void change_network_access_address()
{
    U8 MacType = e_UNKONWN;

    // 有下挂电表，已经更换入网地址
    if(0 != TH2CJQ_MeterList.MeterNum && TRUE == change_address_flag)
    {
        if(0 == memcmp(DevicePib_t.DevMacAddr, TH2CJQ_MeterList.MeterInfo[0].MeterAddr, 6))
        {
            app_printf("no need to change address\n");
            return;
        }
        else
        {
            change_address_flag = FALSE;
        }
    }

    // 无下挂电表，先前已经更换入网地址，需恢复原先绑表地址
    if(0 == TH2CJQ_MeterList.MeterNum && TRUE == change_address_flag)
    {
        app_printf("address recovery\n");

        memcpy(DevicePib_t.DevMacAddr, DevicePib_t.ltu_addr, 6);
        DevicePib_t.Prtcl = DevicePib_t.ltu_protocol;
        change_address_flag = FALSE;

        if(AppNodeState == e_NODE_ON_LINE)
        {
            //需要离线
            net_nnib_ioctl(NET_SET_OFFLINE,NULL);
            net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
        }
        else
        {
            net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
        }

        SetMacAddrRequest(DevicePib_t.ltu_addr, e_METER,e_MAC_METER_ADDR);
    }

    // 有下挂电表，但是未更换入网地址
    if(0 != TH2CJQ_MeterList.MeterNum && FALSE == change_address_flag)
    {
        app_printf("change_network_access_address\n");

        memcpy(DevicePib_t.DevMacAddr, TH2CJQ_MeterList.MeterInfo[0].MeterAddr, 6);
        DevicePib_t.Prtcl =TH2CJQ_MeterList.MeterInfo[0].Protocol;

        change_address_flag = TRUE;

        if(AppNodeState == e_NODE_ON_LINE)
        {
            //需要离线
            net_nnib_ioctl(NET_SET_OFFLINE,NULL);
            net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
        }
        else
        {
            net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
        }

        SetMacAddrRequest(TH2CJQ_MeterList.MeterInfo[0].MeterAddr, e_CJQ_2, e_MAC_METER_ADDR);
    }

    return;
}

void TH2MeterChang(void)
{
    if ((e_IOT_C_MODE_STA == DevicePib_t.Modularmode) && (TH2CJQ_MeterList.MeterNum > 0))
    {
        DevicePib_t.Modularmode = e_IOT_C_MODE_CJQ;
        DefwriteFg.ParaCFG = TRUE;
        SetDefaultInfo();
        app_printf("DevicePib_t.Modularmode = 0\n");
        WriteMeterInfo();
        app_reboot(500);
    }
    else if ((e_IOT_C_MODE_CJQ == DevicePib_t.Modularmode) && (TH2CJQ_MeterList.MeterNum <= 0))
    {
        DevicePib_t.Modularmode = e_IOT_C_MODE_STA;
        DefwriteFg.ParaCFG = TRUE;
        SetDefaultInfo();
        app_printf("DevicePib_t.Modularmode = 1\n");
        WriteMeterInfo();
        app_reboot(500);
    }
    else
    {
        ;
    }
}

METER_COMU_INFO_t ReadTH2MeterInfo = {
	.name    = "MeterCheckComu",
	.Match   = ReadTH2MeterListMatch,
	.Timeout    = ReadTH2MeterListTimeout,
	.RevDel     = ReadTH2MeterListRecvDeal,
};

void StaBaudMeterPutList(METER_COMU_INFO_t *pMeterInfo)
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

void ReadTH2MeterListPut(U8* Addr, U8 Protocol, U8 Retry)
{
    U8  *sendbuf = zc_malloc_mem(100, "SndBuf", MEM_TIME_OUT);
	U8  sendlen = 0;
    static U8   Seq = 0;
    U8 Item07Read[4] = {0x33, 0x33, 0xB4, 0x35};//07使用读取量测单元搜表列表
    if(Protocol == DLT645_2007)
    {
        Packet645Frame(sendbuf,&sendlen,Addr,0x11,Item07Read,sizeof(Item07Read));
    }
    else
    {
    	zc_free_mem(sendbuf);
        return;
    }

    ReadTH2MeterInfo.protocol = (Protocol ==  DLT698?DLT698:DLT645_2007);
    ReadTH2MeterInfo.timeout = BAUDMETER_TIMEOUT;
    ReadTH2MeterInfo.frameLen = sendlen;
    ReadTH2MeterInfo.frameSeq = ++Seq;
    ReadTH2MeterInfo.pPayload = sendbuf;
	ReadTH2MeterInfo.baudrate = sta_bind_info_t.BaudRate;

    StaBaudMeterPutList(&ReadTH2MeterInfo);

    zc_free_mem(sendbuf);
}

void ReadTH2MeterCB(struct ostimer_s *ostimer, void *arg)
{
    app_printf("ReadTH2MeterCB!\n");
    static U8  readcnt = 0;
    if(readcnt < 5)
    {
        Modify_ReadTH2Metertimer(3*60);
        readcnt ++;
    }
    else
    {
        Modify_ReadTH2Metertimer(15*60);
    }
    ReadTH2MeterListPut(DevicePib_t.ltu_addr, DLT645_2007, 1);
}
void Modify_ReadTH2Metertimer(U32 Sec)
{
    if(1 == FactoryTestFlag)
	{
        return;
    }
    if(ReadTH2Metertimer == NULL)
    {
        ReadTH2Metertimer = timer_create(Sec*1000,
	                             0,
	                             TMR_TRIGGER ,
	                             ReadTH2MeterCB,
	                             NULL,
	                             "ReadTH2MeterCB"
	                            );
    }
    else
    {
        timer_modify(ReadTH2Metertimer,
               Sec*1000,
               0,
               TMR_TRIGGER ,
               ReadTH2MeterCB,
               NULL,
               "ReadTH2MeterCB",
               TRUE
               );
    }
    timer_start(ReadTH2Metertimer);
}


#endif