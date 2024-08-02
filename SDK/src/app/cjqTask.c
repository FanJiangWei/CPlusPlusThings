/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        RouterTask.c
 * Purpose:     procedure for using
 * History:
 */
#ifndef __CJQTASK_C__
#define __CJQTASK_C__

#include "flash.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_meter_search_cjq.h"
#include "cjqTask.h"
#include "printf_zc.h"
#include "app_meter_common.h"
#include "app_cnf.h"
#include "dl_mgm_msg.h"
#include "app_dltpro.h"
#include "netnnib.h"
#include "version.h"


#if defined(ZC3750CJQ2)

COLLECT_INFO_ST CollectInfo_st;

const U8 CJQ97Cmd[4] = {0X01, 0X02, 0X43, 0XC3};//97使用
const U8 CJQ07Cmd[6] = {0X11, 0X04, 0x33, 0x33, 0x34, 0x33};//07使用正向有功读取表号
static U8 ReadMeter[50] = {0};
static U16 FrameLen = 0;

U8 cjq_printfflag = TRUE;


U8 cjq_read_addr[6];

U8 g_irflag = 0;


/*static U16 pppfcs16(U16 fcs, unsigned char *cp, int len)
{
    while (len--)
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ *cp++) & 0xff];
    return (fcs);
}*/

#ifndef CJQ_PRINTFUNC
#define CJQ_PRINTFUNC

void CJQ_PRINT_ON()
{
	cjq_printfflag = TRUE;
}
void CJQ_PRINT_OFF()
{
	cjq_printfflag = FALSE;
}

void CJQ_dump(uint8_t * buf, uint32_t len)
{
	if(TRUE == cjq_printfflag)
	{
		dump_buf(buf, len);
	}
}


U8	cmp_cjq_meterlist(U8 *meter)
{
	U8 ii = 0;
	
	for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
	{
		if(0 == memcmp(meter, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH))
		{
			return TRUE;//存在
		}
	}

	return FALSE;//不存在	
}

#endif

#ifndef CJQ_BASEFUNC
#define	CJQ_BASEFUNC

static U8 CJQ_Func_Hex2Dec(U8 data)
{
	U8 temp = (data >> 4) * 10 + (data & 0x0F);
	return temp;
}

static U8 CJQ_Func_SetNextBcd(U8* data)
{
	U8 temp = *data;
	U8 high = 0;
	U8 low = 0;
	
	high = temp >> 4 & 0x0F;
	low = temp & 0x0F;
	
	low++;
	
	if(0x0A == low)
	{
		high++;
		low = 0;

		if(0x0A == high)
		{
			*data = 0xAA;
			return FALSE;
		}
	}

	*data = high << 4 | low;

	return TRUE; 
}
U8 dlt645_AA_addr=0;
//true 地址是BCD false 地址不是BCD
U8 CJQ_Func_JudgeBCD(U8 *buf, U8 len)
{
	U8 tempchar = 0;
	U8 templen = len;
	U8  NullAddr[6] = {0};
	//判定全0地址
	 if(0 == memcmp(NullAddr,buf,templen))
	{
		app_printf("Addr is NullAddr\n");
		return FALSE;
	}
	while(templen)
	{
		tempchar = *buf++;

		if(tempchar >> 4 >= 10 || (tempchar & 0xF) >= 10)
		{
			dlt645_AA_addr = 1;
			return FALSE;
		}

		templen--;
	}
	
	return TRUE;
}

static U8 CJQ_Func_JudgeProtocolBaud(METER_INFO_ST* pstMeterInfo)
{
	if(METER_BAUD_RES < pstMeterInfo->BaudRate && 
		pstMeterInfo->BaudRate < METER_BAUD_END && 
		METER_PROTOCOL_RES < pstMeterInfo->Protocol && 
		pstMeterInfo->Protocol < METER_PROTOCOL_END)
	{
		return TRUE;
	}
	
	return FALSE;
}

static void CJQ_Func_SetCJQBaseAddr(U8* Addr)
{
	U8 err_addr[6]={0x00};
	if(0 == memcmp(err_addr, Addr, 6))
	{
		app_printf("this Addr is err,Addr:%02x%02x%02x%02x%02x%02x\n",
			*Addr,*(Addr+1),*(Addr+2),*(Addr+3),*(Addr+4),*(Addr+5));
		if(CollectInfo_st.MeterNum >1)
		{
			__memcpy(err_addr, CollectInfo_st.MeterList[0].MeterAddr, 6);
			__memcpy(CollectInfo_st.MeterList[0].MeterAddr, CollectInfo_st.MeterList[1].MeterAddr, 6);
			__memcpy(CollectInfo_st.MeterList[1].MeterAddr, err_addr, 6);

			__memcpy(CollectInfo_st.CollectAddr, CollectInfo_st.MeterList[0].MeterAddr, MACADRDR_LENGTH);

		}
		else//目前就搜到一块为全0的表
			memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
	}
	else
	{
		__memcpy(CollectInfo_st.CollectAddr, Addr, MACADRDR_LENGTH);
	}
	return;
}
U8 ChangeNextSearchInfo(SEARCH_CONTROL_ST *pCtrl)
{
    pCtrl->ProtocolBaud ++;
    if(pCtrl->ProtocolBaud >= BAUD_ARR)
    {
        pCtrl->ProtocolBaud = 0;
        return FALSE;
    }
    return TRUE;
}

U8 CJQ_Func_GetProtocolBaud(SEARCH_CONTROL_ST *pCtrl,U8 *Protocol,U8 *Baud)
{
	if(pCtrl->ProtocolBaud >= 0 && pCtrl->ProtocolBaud < BAUD_ARR)
	{
		if(Protocol != NULL)
			*Protocol = ProtBaud[pCtrl->ProtocolBaud].Protocol;
		if(Baud != NULL)
			*Baud = ProtBaud[pCtrl->ProtocolBaud].Baud;
	   
	    return TRUE;
	}

	return FALSE;
}

U8 CJQ_Func_SetProtocolBaud(SEARCH_CONTROL_ST *pCtrl)
{
    app_printf("pCtrl->ProtocolBaud = %d\n",pCtrl->ProtocolBaud);
	if(pCtrl->ProtocolBaud >= 0 && pCtrl->ProtocolBaud < BAUD_ARR)
	{
		pCtrl->Protocol = ProtBaud[pCtrl->ProtocolBaud].Protocol;
		pCtrl->Baud = ProtBaud[pCtrl->ProtocolBaud].Baud;
		app_printf("Protocol = %d,Baud = %d\n",pCtrl->Protocol,baud_enum_to_int(pCtrl->Baud));
		
	    return TRUE;
	}

	return FALSE;
}

static U8 CJQ_Func_GetNoAAPos(U8* Addr,U8* Pos)
{
	U8 ii = 0;
	U8 data = 0;
	
	for(ii = MACADRDR_LENGTH - 1; ii >= 0; ii--)
	{
		if(0xAA == Addr[ii] && 0 == ii)
		{
			return FALSE;
		}
		
		data = CJQ_Func_Hex2Dec(Addr[ii]);
		
		if(data >= 0 && data < 100)
		{
			*Pos = ii;
			return TRUE;
		}
	}

	return FALSE;
}

static U8 CJQ_Func_GetAAPos(U8* Addr,U8* Pos)
{
	U8 ii = 0;
	
	for(ii = 0; ii < MACADRDR_LENGTH; ii++)
	{
		if(Addr[ii] == 0xAA)
		{
			*Pos = ii;
			return TRUE;
		}
	}

	return FALSE;
}
void MeterNumIsFull(SEARCH_CONTROL_ST *pCtrl)
{
    pCtrl->SearchState = SEARCH_STATE_FULL;
	app_printf("SEARCH_STATE_FULL\n");
	if(TMR_RUNNING == zc_timer_query(cjq2_search_meter_timer))
	{
		timer_stop(cjq2_search_meter_timer, TMR_NULL);
	}
	pCtrl->ProtocolBaud = 0;
	CJQ_Check_Begin(pCtrl);
	cjq2_search_meter_timer_modify(CJQ2_SEARCH_METER_STOP_TIME,TMR_TRIGGER);
	return;
}
void CJQ_NetSetOffline()
{
    if(GetNodeState() == e_NODE_ON_LINE && DevicePib_t.PowerOffFlag == e_power_is_ok)
    {
        //需要离线
        net_nnib_ioctl(NET_SET_OFFLINE,NULL);
        U8  MacType = e_UNKONWN;
        net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
    }
}
#endif



#ifndef	CJQ_METER_FUNC
#define CJQ_METER_FUNC

void CJQ_DelMeterInfoByIdx(U8 idx)
{
	U8 i = 0;
	if(CollectInfo_st.MeterNum > 0)
	{
		for(i = idx; i < CollectInfo_st.MeterNum - 1; i++)
		{
			CollectInfo_st.MeterList[i] = CollectInfo_st.MeterList[i + 1];
		}
		memset((U8 *)&CollectInfo_st.MeterList[CollectInfo_st.MeterNum - 1],0x00,sizeof(METER_INFO_ST));
		CollectInfo_st.MeterNum--;
	}
	
	if(CollectInfo_st.MeterNum == 0)
	{
		cjq2_search_meter_info_t.Ctrl.CheckMeterIdx = 0;
		memset(CollectInfo_st.MeterList , 0x00 , sizeof(METER_INFO_ST)*METERNUM_MAX);
	}
	
	if(COLLECT_ADDRMODE_NOADDR == CollectInfo_st.AddrMode)
	{
		if(0 == CollectInfo_st.MeterNum)
		{
			memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
			SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_UNKONWN);
			CJQ_NetSetOffline();//离线
			
		}
		/*
		else
		{
			if(0 != memcmp(CollectInfo_st.CollectAddr, CollectInfo_st.MeterList[0].MeterAddr, MACADRDR_LENGTH))
			{
				CJQ_Func_SetCJQBaseAddr(CollectInfo_st.MeterList[0].MeterAddr);
				SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
				CJQ_dump(CollectInfo_st.CollectAddr, MACADRDR_LENGTH);
				CJQ_NetSetOffline();
			}
		}
		*/
	}
	
	return;
}
void CJQ_698_Judge(U8* Addr, U8 *Protocol, U8 Baud)
{
    FrameLen = 0;
    Dlt698Encode(Addr, *Protocol,ReadMeter,&FrameLen);

	cjq2_search_meter_ctrl(ReadMeter,FrameLen,&cjq2_search_check698_t);
}

void CJQ_AddMeterInfo(U8* Addr, U8 Protocol, U8 Baud)
{
	app_printf("CJQ_AddMeterInfo\n");
	U8  AA_addr[6]= {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};
	U8  ii = 0;
	
	for(ii = 0; ii < CollectInfo_st.MeterNum;ii++)
	{
		if(memcmp(Addr,CollectInfo_st.MeterList[ii].MeterAddr,MACADRDR_LENGTH) == 0)
		{
			app_printf("Meter already Exist\n");
			return ;
		}
		
	}
	if(CollectInfo_st.MeterNum >= METERNUM_MAX)
	{
		return;
	}

	__memcpy(CollectInfo_st.MeterList[CollectInfo_st.MeterNum].MeterAddr,Addr, MACADRDR_LENGTH);
	CollectInfo_st.MeterList[CollectInfo_st.MeterNum].Protocol = Protocol;
	CollectInfo_st.MeterList[CollectInfo_st.MeterNum].BaudRate = Baud;
	CollectInfo_st.MeterNum++;
	
	if(TRUE == cjq_printfflag)
	{
		app_printf("CJQ totalnum = %d addr:", CollectInfo_st.MeterNum);
	}
	CJQ_dump(Addr, MACADRDR_LENGTH);
		
	if(COLLECT_ADDRMODE_NOADDR == CollectInfo_st.AddrMode)
	{
		if(0 != memcmp(CollectInfo_st.CollectAddr, CollectInfo_st.MeterList[0].MeterAddr, MACADRDR_LENGTH))
		{
			CJQ_Func_SetCJQBaseAddr(CollectInfo_st.MeterList[0].MeterAddr);
			if(TRUE == cjq_printfflag)
			{
				app_printf("CJQ_SET Add 1 MACADDR:\n");
			}
			CJQ_dump(CollectInfo_st.CollectAddr, MACADRDR_LENGTH);
			SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
		}
		else
		{
			if(1 == CollectInfo_st.MeterNum)
			{
				if(TRUE == cjq_printfflag)
				{
					app_printf("CJQ_SET Add 2 MACADDR:\n");
				}
				if(memcmp(CollectInfo_st.CollectAddr, AA_addr, 6))
				{
					;
				}
				CJQ_dump(CollectInfo_st.CollectAddr, MACADRDR_LENGTH);
				SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
			}
		}
	}
    CJQ_Flash_Save();
	return;
}

void  CJQ_UpdateMeterInfo(U8* Addr, U8 Protocol, U8 Baud)
{
	//app_printf("CJQ_UpdateMeterInfo\n");
	U8 ii = 0;
	
	for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
	{
		#if 0
		if(0 == memcmp(Addr, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH) &&
			Protocol == CollectInfo_st.MeterList[ii].Protocol &&
			Baud == CollectInfo_st.MeterList[ii].BaudRate)
		{
			return TRUE;//存在
		}
		
		#else
		if(0 == memcmp(Addr, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH))
		{
			//if((METER_PROTOCOL_07 == Protocol && METER_BAUD_2400 == Baud && CollectInfo_st.MeterList[ii].BaudRate != Baud) || 
				//(METER_PROTOCOL_97 == Protocol && METER_BAUD_1200 == Baud && CollectInfo_st.MeterList[ii].BaudRate != Baud)||
				//(METER_PROTOCOL_698 == Protocol && METER_BAUD_9600 == Baud && CollectInfo_st.MeterList[ii].BaudRate != Baud))
            //只针对07-645，698的双协议表更新为698协议
            //if(METER_PROTOCOL_07 == Protocol)
			{
			    if(METER_PROTOCOL_07 == Protocol && CollectInfo_st.MeterList[ii].Protocol == METER_PROTOCOL_698)
                {
                    Protocol = METER_PROTOCOL_698;
                }         
				CollectInfo_st.MeterList[ii].Protocol = Protocol;
				CollectInfo_st.MeterList[ii].BaudRate = Baud;
                app_printf("CJQ_UpdateMeterInfo Succ: Protocol:%d,Baud:%d\n",Protocol,Baud);
			}
		}
		#endif
	}
}

U8 CJQ_CheckMeterExist(U8* Addr, U8 Protocol, U8 Baud)
{
	U8 ii = 0;
	
	for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
	{
		#if 0
		if(0 == memcmp(Addr, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH) &&
			Protocol == CollectInfo_st.MeterList[ii].Protocol &&
			Baud == CollectInfo_st.MeterList[ii].BaudRate)
		{
			return TRUE;//存在
		}
		
		#else
		if(0 == memcmp(Addr, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH))
		{
			
			if(Protocol == DLT645_1997)
			{
                CollectInfo_st.MeterList[ii].Protocol = ((CollectInfo_st.MeterList[ii].Protocol == DLT698)?DLT698:
			                                         (CollectInfo_st.MeterList[ii].Protocol == DLT645_2007)?DLT645_2007:Protocol);
			}
			else
			{
                CollectInfo_st.MeterList[ii].Protocol = (CollectInfo_st.MeterList[ii].Protocol == DLT698)?DLT698:Protocol;
			}
			CollectInfo_st.MeterList[ii].BaudRate = Baud;
			CJQ_Flash_Save();
			return TRUE;//存在
		}
		#endif
	}

	return FALSE;//不存在
}

U8 CJQ_CheckMeterExistByAddr(U8* Addr, U8* Protocol,U8* Baud)
{
	U8 ii = 0;
	
	for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
	{
		#if 0
		if(0 == memcmp(Addr, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH) &&
			Protocol == CollectInfo_st.MeterList[ii].Protocol &&
			Baud == CollectInfo_st.MeterList[ii].BaudRate)
		{
			return TRUE;//存在
		}
		#else
		if(0 == memcmp(Addr, CollectInfo_st.MeterList[ii].MeterAddr, MACADRDR_LENGTH))
		{
			if (Protocol != NULL)
				*Protocol = CollectInfo_st.MeterList[ii].Protocol;
			if (Baud != NULL)
				*Baud = CollectInfo_st.MeterList[ii].BaudRate;
			return TRUE; // 存在
		}
		#endif
	}

	return FALSE;//不存在
}

#endif
void CJQ_Paket_Encode(SEARCH_CONTROL_ST *pCtrl,cjq_search_func_t *pCjqFunc)
{
    if(memcmp(pCjqFunc->name , "checkmeter",sizeof("checkmeter")))
    {
        CJQ_Func_SetProtocolBaud(pCtrl);
    }
    FrameLen = 0;
	if(pCtrl->Protocol == DLT698)//暂时不用698
    {
        Dlt698Encode(pCtrl->SearchAddr, pCtrl->Protocol,ReadMeter,&FrameLen);
    }
    else
    {
        Dlt645Encode(pCtrl->SearchAddr, pCtrl->Protocol,ReadMeter,(U8 *)&FrameLen);
    }

	cjq2_search_meter_ctrl(ReadMeter,FrameLen,pCjqFunc);
}

#ifndef CJQ_STATE_TRAVERSE
#define CJQ_STATE_TRAVERSE
static void CJQ_Traverse_Begin(SEARCH_CONTROL_ST *pCtrl)
{
	pCtrl->SearchState = SEARCH_STATE_TRAVERSE;
	pCtrl->CheckMeterIdx = 0;
    pCtrl->SearchTimeout = 0;
    memset(pCtrl->SearchAddr, 0xAA, MACADRDR_LENGTH);
	pCtrl->SearchAddr[0] = 0x00;
    memset(pCtrl->SearchResultAddr, 0xFF, MACADRDR_LENGTH);
    
	CJQ_Paket_Encode(pCtrl,&cjq2_search_traverse_t);
	return;
}


void CJQ_Traverse_Restart(SEARCH_CONTROL_ST *pCtrl)
{
	CJQ_Check_Begin(pCtrl);
}


void CJQ_Traverse_Next(SEARCH_CONTROL_ST *pCtrl)
{
	U8 Pos = 0;
	U8 data = 0xFF;
	//切换下一个表号，找到第一个不是AA的数，当前值大于等于0并且小于100 累加，如果大于等于99 当前值变成0xAA 前一个字节开始累加
	while(CJQ_Func_GetNoAAPos(pCtrl->SearchAddr, &Pos))
	{		
		data = CJQ_Func_Hex2Dec(pCtrl->SearchAddr[Pos]);
		
		if(data >= 0 && data <= 99)
		{
		    if(CJQ_Func_GetProtocolBaud(pCtrl,NULL,NULL))
		    {
                if(CJQ_Func_SetNextBcd(&pCtrl->SearchAddr[Pos]))
    			{
    			    CJQ_Paket_Encode(pCtrl,&cjq2_search_traverse_t);
    				return;
    			}
		    }
		    else
		    {
                break;
		    }
		}
	}
	//切换下一个波特率
	if(ChangeNextSearchInfo(pCtrl))
	{
		CJQ_Traverse_Begin(pCtrl);
	}
	else
	{
        app_printf("CJQ_Traverse_Next stop\n");
		pCtrl->SearchState = SEARCH_STATE_STOP;
		if(TMR_RUNNING == zc_timer_query(cjq2_search_meter_timer))
		{
			timer_stop(cjq2_search_meter_timer, TMR_NULL);
		}
		// pCtrl->ProtocolBaud = 0;
		// CJQ_Check_Begin(pCtrl);
		// g_restart_search_meter_fg = TRUE;
		cjq2_search_meter_timer_modify(CJQ2_SEARCH_METER_STOP_TIME,TMR_TRIGGER);
	}
	
	return;
}

void CJQ_Traverse_Succ(SEARCH_CONTROL_ST *pCtrl)
{
	U8 Protocol = 0;
	U8 Baud = 0;

	CJQ_Func_GetProtocolBaud(pCtrl,&Protocol, &Baud);
	
	//档案已存在
	if(CJQ_CheckMeterExist(pCtrl->SearchResultAddr, Protocol, Baud))
	{
        app_printf("Meter already Exist\n");
	}
	else//档案不存在
	{
	    if(Protocol == DLT645_2007)
        {
        	Protocol = DLT698;
            CJQ_698_Judge(cjq2_search_meter_info_t.Ctrl.SearchResultAddr, &Protocol, Baud);
            return;
        } 
		CJQ_AddMeterInfo(pCtrl->SearchResultAddr, Protocol, Baud);
		if(cjq2_search_meter_info_t.Ctrl.SearchState != SEARCH_STATE_CHECKMETER)
    	{
            if(CollectInfo_st.MeterNum >= METERNUM_MAX)
        	{
        		MeterNumIsFull(&cjq2_search_meter_info_t.Ctrl);
        		return;
        	}
    	}
	}
    CJQ_Traverse_Next(pCtrl);
	return;
}

void CJQ_Traverse_Errcode(SEARCH_CONTROL_ST *pCtrl)
{
	U8 Pos = 0;
    //找到第一个不为0的数，重新搜表
	if(CJQ_Func_GetAAPos(pCtrl->SearchAddr, &Pos))
	{
		pCtrl->SearchAddr[Pos] = 0;
		CJQ_Paket_Encode(pCtrl,&cjq2_search_traverse_t);
	}
    else
    {
    	CJQ_Traverse_Next(pCtrl);    
    }
    
    return;

}


#endif

#ifndef CJQ_STATE_AA
#define CJQ_STATE_AA


void CJQ_AA_Begin(SEARCH_CONTROL_ST *pCtrl)
{
	pCtrl->SearchState = SEARCH_STATE_AA;
	pCtrl->CheckMeterIdx = 0;
    pCtrl->SearchTimeout = 0;;
    memset(pCtrl->SearchAddr, 0xAA, MACADRDR_LENGTH);
    memset(pCtrl->SearchResultAddr, 0xFF, MACADRDR_LENGTH);
    
	CJQ_Paket_Encode(pCtrl,&cjq2_search_aa_t);
}

void CJQ_AA_Next(SEARCH_CONTROL_ST *pCtrl)
{
	if(ChangeNextSearchInfo(pCtrl))
	{
		CJQ_AA_Begin(pCtrl);
	}
	else
	{
	    pCtrl->ProtocolBaud = 0;
		CJQ_Traverse_Begin(pCtrl);
	}
	return;
}

void CJQ_AA_Succ(SEARCH_CONTROL_ST *pCtrl)
{
	U8  Protocol = 0;
	U8  Baud = 0;
	CJQ_Func_GetProtocolBaud(pCtrl,&Protocol, &Baud);

	if(CJQ_CheckMeterExist(pCtrl->SearchResultAddr, Protocol, Baud))
	{
        app_printf("Meter already Exist\n");
	}
	else
	{
		if(Protocol==DLT645_2007)
        {
            Protocol = DLT698;
            CJQ_698_Judge(pCtrl->SearchResultAddr, &Protocol, Baud);
            return;
        } 
        CJQ_AddMeterInfo(pCtrl->SearchResultAddr, Protocol, Baud);
        if(cjq2_search_meter_info_t.Ctrl.SearchState != SEARCH_STATE_CHECKMETER)
    	{
            if(CollectInfo_st.MeterNum >= METERNUM_MAX)
        	{
        		MeterNumIsFull(&cjq2_search_meter_info_t.Ctrl);
        		return;
        	}
    	}
	}
	CJQ_AA_Next(pCtrl);
	return;
}

void CJQ_AA_Errcode(SEARCH_CONTROL_ST *pCtrl)
{
	CJQ_AA_Next(pCtrl);
    
	return;
}


#endif



void CJQ_Check698_Next(SEARCH_CONTROL_ST *pCtrl)
{
	if(pCtrl->SearchState == SEARCH_STATE_AA)
    {
        CJQ_AA_Next(pCtrl);
    }
    else if(pCtrl->SearchState == SEARCH_STATE_TRAVERSE)
    {
        CJQ_Traverse_Next(pCtrl);
    }
    else if(pCtrl->SearchState == SEARCH_STATE_CHECKMETER)
    {
	    CJQ_Check_Next(pCtrl);
    }
	return;
}

void CJQ_Check698_Succ(SEARCH_CONTROL_ST *pCtrl)
{
	//更新698表档案
    U8  Protocol = 0;
	U8  Baud = 0;
	CJQ_Func_GetProtocolBaud(pCtrl,&Protocol, &Baud);
    if(Protocol==METER_PROTOCOL_07)
    {
        Protocol = METER_PROTOCOL_698;
        CJQ_AddMeterInfo(pCtrl->SearchResultAddr, Protocol, Baud);
        if(cjq2_search_meter_info_t.Ctrl.SearchState != SEARCH_STATE_CHECKMETER)
    	{
            if(CollectInfo_st.MeterNum >= METERNUM_MAX)
        	{
        		MeterNumIsFull(&cjq2_search_meter_info_t.Ctrl);
        		return;
        	}
    	}
    }
    CJQ_Check698_Next(pCtrl);
  
	return;
}
void CJQ_Check698_Errcode(SEARCH_CONTROL_ST *pCtrl)
{
	CJQ_Check698_Next(pCtrl);
    
	return;
}

#ifndef CJQ_STATE_CHECK
#define CJQ_STATE_CHECK
void CJQ_Check_Execute(SEARCH_CONTROL_ST *pCtrl)
{
	U8 ii=0;
	__memcpy(pCtrl->SearchAddr, CollectInfo_st.MeterList[pCtrl->CheckMeterIdx].MeterAddr, MACADRDR_LENGTH);
	pCtrl->SearchTimeout = 0;
	pCtrl->Protocol = CollectInfo_st.MeterList[pCtrl->CheckMeterIdx].Protocol;
	pCtrl->Baud = CollectInfo_st.MeterList[pCtrl->CheckMeterIdx].BaudRate;
	app_printf("pCtrl->ProtocolBaud1 = %d\n",pCtrl->ProtocolBaud);

	for(ii=0;ii<BAUD_ARR;ii++)
	{
		if((ProtBaud[ii].Protocol==pCtrl->Protocol)&&(ProtBaud[ii].Baud==pCtrl->Baud))
		{			
			pCtrl->ProtocolBaud = ii;
			app_printf("pCtrl->ProtocolBaud2 = %d\n",pCtrl->ProtocolBaud);
			break;
		}		
	}
	
    app_printf("CJQ_Check_Execute Protocol= %d,Baud= %d\n",pCtrl->Protocol,pCtrl->Baud);
    dump_buf(pCtrl->SearchAddr,MACADRDR_LENGTH);
	CJQ_Paket_Encode(pCtrl,&cjq2_search_check_t);
	
}

void CJQ_Check_Begin(SEARCH_CONTROL_ST *pCtrl)
{
	pCtrl->SearchState = SEARCH_STATE_CHECKMETER;
	pCtrl->ProtocolBaud = 0;
	pCtrl->CheckMeterIdx = 0;
    pCtrl->SearchTimeout = 0;
    memset(pCtrl->SearchResultAddr, 0xFF, MACADRDR_LENGTH);
    app_printf("\r\n CJQ_Check_Begin\n");
    CJQ_Check_Execute(pCtrl);
	return;
}


void CJQ_Check_Next(SEARCH_CONTROL_ST *pCtrl)
{

	if(pCtrl->CheckMeterIdx < CollectInfo_st.MeterNum)
	{
		//CJQ_DelMeterInfoByIdx(pCtrl->CheckMeterIdx);
        if(CollectInfo_st.MeterNum > 0)
		{
            CJQ_Check_Execute(pCtrl);
        }
		else
		{
			app_printf("CollectInfo_st.MeterNum = 0\n");
			app_printf("------CJQ CheckMeter has finished,will save flash!-------\n");
			CJQ_Flash_Save();
            pCtrl->ProtocolBaud = 0;
		    CJQ_AA_Begin(pCtrl);
		}
	}
	else
	{
		if(CollectInfo_st.MeterNum < METERNUM_MAX)
		{
			pCtrl->ProtocolBaud = 0;
		    CJQ_AA_Begin(pCtrl);
		}
		else
		{
			MeterNumIsFull(pCtrl);
		}
		app_printf("------CJQ_Check_Next,will save flash!-------\n");
		CJQ_Flash_Save();
	}

	return;
}

void CJQ_Check_Succ(SEARCH_CONTROL_ST *pCtrl)
{
	U8  AA_addr[6]= {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};
	if(0 == memcmp(pCtrl->SearchAddr, pCtrl->SearchResultAddr, MACADRDR_LENGTH))
	{
		if(0 != memcmp(CollectInfo_st.CollectAddr, CollectInfo_st.MeterList[0].MeterAddr, MACADRDR_LENGTH))
		{
			CJQ_Func_SetCJQBaseAddr(CollectInfo_st.MeterList[0].MeterAddr);
			SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
			//需要离线
			g_ReqReason = e_elffail;
			CJQ_NetSetOffline();
		}			
		
		if(0 == pCtrl->CheckMeterIdx)
		{
			if(TRUE == cjq_printfflag)
			{
				app_printf("CJQ_SET Check MACADDR:\n");
			}
			if(memcmp(CollectInfo_st.CollectAddr, AA_addr, 6))
			{
				;
			}
			CJQ_dump(CollectInfo_st.CollectAddr, MACADRDR_LENGTH);
			SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2, e_MAC_METER_ADDR);
		}
		CJQ_UpdateMeterInfo(pCtrl->SearchResultAddr,pCtrl->Protocol,pCtrl->Baud);
		app_printf("pCtrl->CheckMeterIdx = %d  CollectInfo_st.MeterNum = %d \r\n",pCtrl->CheckMeterIdx,CollectInfo_st.MeterNum);
		
	}
	pCtrl->CheckMeterIdx++;
    CJQ_Check_Next(pCtrl);
	return;
}

void CJQ_Check_Errocde(SEARCH_CONTROL_ST *pCtrl)
{
    app_printf("CJQ_Check_Errocde\n");
	CJQ_DelMeterInfoByIdx(cjq2_search_meter_info_t.Ctrl.CheckMeterIdx);
	CJQ_Check_Next(&cjq2_search_meter_info_t.Ctrl);
	return;
}


#endif



#ifndef CJQ_FLASH_FUNC
#define CJQ_FLASH_FUNC

void CJQ_Flash_Save()
{
	staflag = TRUE;
	if (GetNodeState() != e_NODE_ON_LINE)
		{
            mutex_post(&mutexSch_t);
		}
	return;
}
//计算长度大于255字节的累加和
U8 CJQ_Flash_Check(U8 *buf, U16 len)
{
	U16 templen = len;
	U8 ret = 0;

	while(templen)
	{
		ret += buf[templen - 1];
		templen--;
	}

	return ret;
}

void CJQ_Flash_Init(cjq_dev_t *dev)
{
	U8 i = 0;
	U8 Cs = 0;
	//COLLECT_INFO_ST Null_CollectInfo_st; 
	
	//memset((U8 *)&Null_CollectInfo_st, 0 ,sizeof(COLLECT_INFO_ST));
    zc_flash_read(FLASH, STA_INFO, (U32 *)&CollectInfo_st, sizeof(COLLECT_INFO_ST));
    
	if(0xA5 == CollectInfo_st.Flag[0] && 0x5A == CollectInfo_st.Flag[1])
	{
		app_printf("\r\nCJQ_Flash_Init Flag OK!\n");

		Cs = CJQ_Flash_Check((U8*)&CollectInfo_st, sizeof(COLLECT_INFO_ST) - 1);
		#if 0
		app_printf("\r\nbuff_len = %d\n", sizeof(COLLECT_INFO_ST));
		app_printf("\r\nCollectInfo_st.Cs = 0x%02x, Cs = 0x%02x\n", CollectInfo_st.Cs, Cs);
		CJQ_dump((U8*)&CollectInfo_st, sizeof(COLLECT_INFO_ST));
		#endif
		if(CollectInfo_st.Cs == Cs)
		{
			app_printf("\r\nCJQ_Flash_Init Cs OK! totalnum = %d\n", CollectInfo_st.MeterNum);
			
			for(i = 0; i < CollectInfo_st.MeterNum; i++)
			{			
				if(FALSE == CJQ_Func_JudgeBCD(CollectInfo_st.MeterList[i].MeterAddr, MACADRDR_LENGTH) || FALSE == CJQ_Func_JudgeProtocolBaud(CollectInfo_st.MeterList + i))
				{
					CJQ_DelMeterInfoByIdx(i);
					i--;
				}
			}
			
			//有地址模式
			if(COLLECT_ADDRMODE_NOADDR == CollectInfo_st.AddrMode)
			{
				if(0 == CollectInfo_st.MeterNum)
				{
					memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
				}
				else
				{
					if(0 != memcmp(CollectInfo_st.CollectAddr, CollectInfo_st.MeterList[0].MeterAddr, MACADRDR_LENGTH))
					{
						CJQ_Func_SetCJQBaseAddr(CollectInfo_st.MeterList[0].MeterAddr);
					}
				}
			}
			else
			{
				if(TRUE == cjq_printfflag)
				{
					app_printf("CJQ_SET init MACADDR:\n");
				}
				CJQ_dump(CollectInfo_st.CollectAddr, MACADRDR_LENGTH);
				SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
			}
		}
		else
		{
			app_printf("CJQ_Flash_Init Cs Err FlashInit!\n");
			//校验错误
#if defined(CJQ_SET_AS_RELAY)
                    CollectInfo_st.AddrMode = COLLECT_ADDRMODE_ADDR;
                    memset(CollectInfo_st.CollectAddr, 0xBB, MACADRDR_LENGTH);
                    //DefSetInfo.FreqInfo.FreqBand=DEF_FREQBAND;
                    //DefSetInfo.FreqInfo.tgaindig=6;
                    //DefSetInfo.FreqInfo.tgainana=8;
#else
                    CollectInfo_st.AddrMode = COLLECT_ADDRMODE_NOADDR;
                    memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
                    //DefSetInfo.FreqInfo.FreqBand=DEF_FREQBAND;
                    //DefSetInfo.FreqInfo.tgaindig=DEF_TGAIN_DIG;
                    //DefSetInfo.FreqInfo.tgainana=DEF_TGAIN_ANA;
                    
#endif

			memset(CollectInfo_st.SetCCOAddr, 0x00, MACADRDR_LENGTH);
			cjq2_search_meter_info_t.Ctrl.Protocol = METER_PROTOCOL_RES;
			cjq2_search_meter_info_t.Ctrl.Baud = METER_BAUD_RES;
			CollectInfo_st.Timeout = TIME_OUT;//延时1秒
			CollectInfo_st.Interval = INTERVAL;
			CollectInfo_st.Version.Vendorcode[0] = Vender2;
			CollectInfo_st.Version.Vendorcode[1] = Vender1;
			CollectInfo_st.Version.ProductType[0] = ZC3750CJQ2_chip2;
			CollectInfo_st.Version.ProductType[1] = ZC3750CJQ2_chip1;
			CollectInfo_st.Version.Day = Date_D/10*16+Date_D%10;
			CollectInfo_st.Version.Month = Date_M/10*16+Date_M%10;
			CollectInfo_st.Version.Year = Date_Y/10*16+Date_Y%10;
			CollectInfo_st.Version.Version2 = ZC3750CJQ2_ver1;
			CollectInfo_st.Version.Version1 = ZC3750CJQ2_ver2;
			CollectInfo_st.CrtUseListSeq = 0;


			CollectInfo_st.MeterNum = 0;
			memset((U8*)&(CollectInfo_st.MeterList[0]), 0x00, sizeof(METER_INFO_ST) * METERNUM_MAX);
			CollectInfo_st.PowerOFFflag = e_power_is_ok;
		}

		if(CollectInfo_st.MeterNum > 0)
		{
			CJQ_Check_Begin(&cjq2_search_meter_info_t.Ctrl);
		}
		else
		{
			cjq2_search_meter_info_t.Ctrl.ProtocolBaud = 0;
	        CJQ_AA_Begin(&cjq2_search_meter_info_t.Ctrl);
		}

		CollectInfo_st.ResetNum++;
	}
	else
	{
		app_printf("\r\nCJQ_Flash_Init Flag A55A!\n");
		memset(&CollectInfo_st, 0x00, sizeof(COLLECT_INFO_ST));
		CollectInfo_st.Flag[0] = 0xA5;
		CollectInfo_st.Flag[1] = 0x5A;
#if defined(CJQ_SET_AS_RELAY)
        CollectInfo_st.AddrMode = COLLECT_ADDRMODE_ADDR;
        memset(CollectInfo_st.CollectAddr, 0xBB, MACADRDR_LENGTH);
        //DefSetInfo.FreqInfo.FreqBand=2;
        //DefSetInfo.FreqInfo.tgaindig=6;
        //DefSetInfo.FreqInfo.tgainana=8;
#else
        CollectInfo_st.AddrMode = COLLECT_ADDRMODE_NOADDR;
        memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
        //DefSetInfo.FreqInfo.FreqBand=2;
        //DefSetInfo.FreqInfo.tgaindig=0;
        //DefSetInfo.FreqInfo.tgainana=0;
        
#endif

        memset(CollectInfo_st.SetCCOAddr, 0x00, MACADRDR_LENGTH);
		cjq2_search_meter_info_t.Ctrl.Protocol = METER_PROTOCOL_RES;
		cjq2_search_meter_info_t.Ctrl.Baud = METER_BAUD_RES;
		CollectInfo_st.Timeout = TIME_OUT;//延时1秒
		CollectInfo_st.Interval = INTERVAL;
		CollectInfo_st.Version.Vendorcode[0] = Vender2;
		CollectInfo_st.Version.Vendorcode[1] = Vender1;
		CollectInfo_st.Version.ProductType[0] = ZC3750CJQ2_chip2;
		CollectInfo_st.Version.ProductType[1] = ZC3750CJQ2_chip1;
		CollectInfo_st.Version.Day = Date_D/10*16+Date_D%10;
		CollectInfo_st.Version.Month = Date_M/10*16+Date_M%10;
		CollectInfo_st.Version.Year = Date_Y/10*16+Date_Y%10;
		CollectInfo_st.Version.Version2 = ZC3750CJQ2_ver1;
		CollectInfo_st.Version.Version1 = ZC3750CJQ2_ver2;
		CollectInfo_st.MeterNum = 0;		
		CollectInfo_st.CrtUseListSeq = 0;
		CollectInfo_st.PowerOFFflag = e_power_is_ok;
        
		cjq2_search_meter_info_t.Ctrl.ProtocolBaud = 0;
	    CJQ_AA_Begin(&cjq2_search_meter_info_t.Ctrl);

		CollectInfo_st.ResetNum = 0;
	}
    if(CollectInfo_st.PowerOFFflag !=e_power_off_now && CollectInfo_st.PowerOFFflag != e_power_is_ok)
    {
        CollectInfo_st.PowerOFFflag = e_power_is_ok;
    }
    DevicePib_t.PowerOffFlag = 	CollectInfo_st.PowerOFFflag;
	//DevicePib_t.FreqBand = CollectInfo_st.InitBand;
	app_printf("CollectInfo_st.PowerOFFflag=%d",CollectInfo_st.PowerOFFflag);
	CJQ_Flash_Save();
	
	return;
}

#endif

void SetDeviceaddrBymodeID(void)
{
	U8 i;
	U8 High = 0;
	U8 Low = 0;
	for(i = 0;i < 6;i++)
	{
		Low = ModuleID[i]>>4&0x0f;
		High = ModuleID[i+1]&0x0f;
		DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[i]=((High<<4)+Low);
	}
	app_printf("DeviceBaseAddr : ");
	dump_buf(DefSetInfo.DeviceAddrInfo.DeviceBaseAddr,6);
}

#ifndef CJQ_EXTEND645_FUNC
#define  CJQ_EXTEND645_FUNC

void CJQ_Extend_ShowVersion()
{
	app_printf("\r\nVendorcode:%c%c\nProductType:%c%c\nData:20%02x-%02x-%02x\nVersion:%d.%d.%02x\n",
				CollectInfo_st.Version.Vendorcode[1],
				CollectInfo_st.Version.Vendorcode[0],
				CollectInfo_st.Version.ProductType[1],
				CollectInfo_st.Version.ProductType[0],
				CollectInfo_st.Version.Year,
				CollectInfo_st.Version.Month,
				CollectInfo_st.Version.Day,
				CollectInfo_st.Version.Version2 >> 4,
				CollectInfo_st.Version.Version2 & 0xF,
				CollectInfo_st.Version.Version1);
}

VERSION_ST* CJQ_Extend_ReadVersion()
{
	CJQ_Extend_ShowVersion();
	return &(CollectInfo_st.Version);
}

void CJQ_Extend_ParaInit()
{
	if(COLLECT_ADDRMODE_NOADDR == CollectInfo_st.AddrMode)
	{
		memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
	}
	else
	{
		memset(CollectInfo_st.CollectAddr, 0xBB, MACADRDR_LENGTH);
	}
	SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
	if(AppNodeState == e_NODE_ON_LINE && DevicePib_t.PowerOffFlag == e_power_is_ok)
    {
        //需要离线
        net_nnib_ioctl(NET_SET_OFFLINE,NULL);
        U8  MacType = e_UNKONWN;
        net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
    }
	cjq2_search_meter_info_t.Ctrl.ProtocolBaud = 0;
	cjq2_search_meter_info_t.Ctrl.Protocol = METER_PROTOCOL_RES;
	cjq2_search_meter_info_t.Ctrl.Baud = METER_BAUD_RES;
	app_printf("CJQ_AA_Begin\n");
	CJQ_AA_Begin(&cjq2_search_meter_info_t.Ctrl);
	cjq2_search_meter_timer_modify(10,TMR_TRIGGER);
    
	CollectInfo_st.Timeout = TIME_OUT;
	CollectInfo_st.Interval = INTERVAL;
	CollectInfo_st.ResetNum = 0;
	CollectInfo_st.MeterNum = 0;
	memset((U8*)(&CollectInfo_st.MeterList[0]), 0x00, sizeof(METER_INFO_ST) * METERNUM_MAX);
	CJQ_Flash_Save();
	return;
}

U8 CJQ_Extend_MeterNum()
{
	return CollectInfo_st.MeterNum;
}

METER_INFO_ST* CJQ_Extend_MeterInfo()
{
	return CollectInfo_st.MeterList;
}

U8* CJQ_Extend_ReadBaseAddr()
{
	return DefSetInfo.DeviceAddrInfo.DeviceBaseAddr;
}

void CJQ_Extend_SetBaseAddr(U8* Addr)
{
	__memcpy(DefSetInfo.DeviceAddrInfo.DeviceBaseAddr, Addr, MACADRDR_LENGTH);
	DefaultSettingFlag = TRUE;
	DefwriteFg.DevAddr = TRUE;
	return;
}

U8 CJQ_Extend_ReadInterval()
{
	return CollectInfo_st.Interval;
}

void CJQ_Extend_SetInterval(U8 time)
{
	CollectInfo_st.Interval = time;
	CJQ_Flash_Save();
	return;
}

U8 CJQ_Extend_ReadTimeout()
{
	return CollectInfo_st.Timeout;
}

void CJQ_Extend_SetTimeout(U8 time)
{
	CollectInfo_st.Timeout = time;
	CJQ_Flash_Save();
	return;
}

U8 CJQ_Extend_ReadResetNum()
{
	return CollectInfo_st.ResetNum;
}

void CJQ_Extend_ShowInfo()
{
	U16 ii=0;
	app_printf("\r\nCJQ Addr: %02x%02x%02x%02x%02x%02x\n",
				CollectInfo_st.CollectAddr[5],
				CollectInfo_st.CollectAddr[4],
				CollectInfo_st.CollectAddr[3],
				CollectInfo_st.CollectAddr[2],
				CollectInfo_st.CollectAddr[1],
				CollectInfo_st.CollectAddr[0]);
	app_printf("\nDeviceBaseAddr: %02x%02x%02x%02x%02x%02x\n",
				DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[5],
				DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[4],
				DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[3],
				DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[2],
				DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[1],
				DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[0]);
	
	app_printf("\r\nAddr         Protocol    Baud        TotalNum = %d\n",CollectInfo_st.MeterNum);
	
	for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
	{
		app_printf("%02x%02x%02x%02x%02x%02x  %04d        %04d\n",
					CollectInfo_st.MeterList[ii].MeterAddr[5],
					CollectInfo_st.MeterList[ii].MeterAddr[4],
					CollectInfo_st.MeterList[ii].MeterAddr[3],
					CollectInfo_st.MeterList[ii].MeterAddr[2],
					CollectInfo_st.MeterList[ii].MeterAddr[1],
					CollectInfo_st.MeterList[ii].MeterAddr[0],
					(METER_PROTOCOL_07 == CollectInfo_st.MeterList[ii].Protocol) ? 2007 : 
					(METER_PROTOCOL_698 == CollectInfo_st.MeterList[ii].Protocol ? 698 : 
                        1997),
					baud_enum_to_int(CollectInfo_st.MeterList[ii].BaudRate));
	}

}
#endif
#ifndef CJQ_MACADDR_EXCHANGE
#define CJQ_MACADDR_EXCHANGE
void CJQ_ExchangeMacAddr()
{
	U8 transit_addr[6],transit_char;
	U16 ii=0;
	
	if(CollectInfo_st.MeterNum > 1)
	{
		if(CollectInfo_st.CrtUseListSeq+1 < CollectInfo_st.MeterNum)
		{
			CollectInfo_st.CrtUseListSeq++;
		}
		else
			CollectInfo_st.CrtUseListSeq = 1;

					
		app_printf("\r\nAddr		 Protocol	 Baud	   TotalNum = %d\n",CollectInfo_st.MeterNum);
		
		for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
		{
			app_printf("%02x%02x%02x%02x%02x%02x       %04d		%04d   \n",
						CollectInfo_st.MeterList[ii].MeterAddr[5],
						CollectInfo_st.MeterList[ii].MeterAddr[4],
						CollectInfo_st.MeterList[ii].MeterAddr[3],
						CollectInfo_st.MeterList[ii].MeterAddr[2],
						CollectInfo_st.MeterList[ii].MeterAddr[1],
						CollectInfo_st.MeterList[ii].MeterAddr[0],
						(METER_PROTOCOL_07 == CollectInfo_st.MeterList[ii].Protocol)?2007:(METER_PROTOCOL_698 == CollectInfo_st.MeterList[ii].Protocol ? 698:1997),
						baud_enum_to_int(CollectInfo_st.MeterList[ii].BaudRate));
		}

		app_printf("\r\nCJQ_ExchangeMacAddr:\n");
		app_printf("CollectInfo_st.CrtUseListSeq = %d    \n",CollectInfo_st.CrtUseListSeq);

		__memcpy(transit_addr, CollectInfo_st.MeterList[CollectInfo_st.CrtUseListSeq].MeterAddr, 6);
		__memcpy(CollectInfo_st.MeterList[CollectInfo_st.CrtUseListSeq].MeterAddr, CollectInfo_st.MeterList[0].MeterAddr, 6);
		__memcpy(CollectInfo_st.MeterList[0].MeterAddr, transit_addr, 6);
		transit_char = CollectInfo_st.MeterList[CollectInfo_st.CrtUseListSeq].Protocol;
		CollectInfo_st.MeterList[CollectInfo_st.CrtUseListSeq].Protocol = CollectInfo_st.MeterList[0].Protocol;
		CollectInfo_st.MeterList[0].Protocol = transit_char;
		transit_char = CollectInfo_st.MeterList[CollectInfo_st.CrtUseListSeq].BaudRate;
		CollectInfo_st.MeterList[CollectInfo_st.CrtUseListSeq].BaudRate = CollectInfo_st.MeterList[0].BaudRate;
		CollectInfo_st.MeterList[0].BaudRate = transit_char;
			

		__memcpy(CollectInfo_st.CollectAddr, CollectInfo_st.MeterList[0].MeterAddr,6);

		app_printf("the CJQmacAddr: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
			CollectInfo_st.CollectAddr[0],CollectInfo_st.CollectAddr[1],CollectInfo_st.CollectAddr[2],
			CollectInfo_st.CollectAddr[3],CollectInfo_st.CollectAddr[4],CollectInfo_st.CollectAddr[5]);

		SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
		CJQ_Flash_Save();
	}
	
	return;
}



#endif

#ifndef CJQ_TASK
#define CJQ_TASK

ostimer_t *setaddrbyidtimer = NULL;


void setaddrcjq2byidCB(struct ostimer_s *ostimer, void *arg)
{
	U8 broad_addr[6]={0};
	if(memcmp(DefSetInfo.DeviceAddrInfo.DeviceBaseAddr,broad_addr,6)==0)
	{
		__memcpy(DefSetInfo.DeviceAddrInfo.DeviceBaseAddr,ModuleID,5);
		DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[5]=0;
		app_printf("DeviceBaseAddr : ");
		dump_buf(DefSetInfo.DeviceAddrInfo.DeviceBaseAddr,6);
        if(FactoryTestFlag != 1)
        {
		    SetMacAddrRequest(DefSetInfo.DeviceAddrInfo.DeviceBaseAddr, e_CJQ_2, e_MAC_MODULE_ADDR);
        }
	}
}

#endif


#endif


#endif
