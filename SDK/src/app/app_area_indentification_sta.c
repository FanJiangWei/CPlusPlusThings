#include "app_phase_position_cco.h"
#include "app_area_indentification_sta.h"
#include "datalinkdebug.h"
#include "app_base_serial_cache_queue.h"
#include "app_area_indentification_cco.h"
#include "app_dltpro.h"
#include "protocolproc.h"
#include "netnnib.h"
#include "dev_manager.h"


#if defined(STATIC_NODE)

INDENTIFICATION_RESULT_DATA_t sta_area_ind_dis_result_t[3];
U16 sta_area_ind_recv_packet_seq = 0xFFFF;


static void sta_area_ind_get_voltage_frequncy_by_698_frame(U8 *buf ,U16 Len,U8 *value_voltage,U16 *value_voltage_len,U8 *value_freq,U16 *value_freq_len)
{
	U8 voltage_698[4]={0x20,0x00,0x02,0x00};
	U8 frequncy_698[4]={0x20,0x0f,0x02,0x00};
	U8 i = 0;
	U8 offset = 0;
	if(value_voltage_len != NULL)
		*value_voltage_len = 0;
	if(value_freq_len != NULL)
		*value_freq_len = 0;
	while(*buf != 0x68)
	{
		buf++;
		Len--;
	}
	if(Len<10)
	{
		return ;
	}

	offset += 21;

	app_printf("voltage : ");
	dump_buf(buf+offset,4);
	
	if(0==memcmp(buf+offset ,voltage_698 ,sizeof(voltage_698)))
	{
		offset += 6;
		dump_buf(buf+offset , 10);
		U8 num_voltage = *(buf+offset);
		app_printf("voltage num - >%d\n",num_voltage);

		for(i = 0;i < num_voltage;i++)
		{
			if(value_voltage)
			{
				*(value_voltage + i*2) = *(buf+offset+3);
				*(value_voltage + 1 +i*2) = *(buf+offset+2);
				(*value_voltage_len) += 2; 
			}
			offset += 3;
		}
	}
	offset ++;
	
	app_printf("freq : ");
	dump_buf(buf+offset,4);	

	if(0==memcmp(buf+offset ,frequncy_698 ,sizeof(frequncy_698)))
	{
		offset += 4;
		U8 num_freq = *(buf+offset);
		app_printf("freq num - >%d\n",num_freq);

		for(i = 0;i<num_freq;i++)
		{
			if(value_freq)
			{
				*(value_freq + i*2) = *(buf+offset+3);
				*(value_freq + 1 +i*2) = *(buf+offset+2);
				(*value_freq_len) += 2; 
			}
			offset += 3;
		}
	}

}
  
static U8 *sta_area_ind_get_voltage_frequncy_by_645_frame(U8 *id_log,U8 *buf ,U16 Len,U16 *value_len)
{
	U16 id_len = 0;
	U16 i = 0;
	U8 *id_buf = NULL;
	while(*buf != 0x68)
	{
		buf++;
		Len--;
	}
	if(Len<10)
	{
		return 0;
	}
			 //0x68
			 
	buf += 1;//μ??·
	
	buf += 6;//0x68

	buf += 1;//????×?
	
	buf += 1;//3¤?è
	id_len = *buf;
	buf += 1;//3-?á??
	if(0 != memcmp(id_log,buf,4))
	{
		app_printf("need id_log-> ");
		dump_buf(id_log,4);
		*value_len = 0;
		return NULL;
	}
	buf += 4;//êy?Y

	id_buf = buf;
	for(i=0;i<(id_len-4);i++)
	{
 	 *(buf+i) -= 0x33;
	 (*value_len) += 1;
	}
	
	return 	id_buf;
}

void sta_area_ind_packet_voltage_freq_698_frame(U8 *addr , U8 *buf,U16 *len)
{
	 uint8_t   meter_addr_req_buf[51] = {
									  	/*
									  	0x68,0x31,0x00,0x43,0x05,0x32,0x00,0x00,0x00,0x00,
									  	0x00,0x00,0x7e,0x8f,0x10,0x00,0x0d,0x05,0x02,0x00,
									  	0x02,0x20,0x00,0x02,0x00,0x20,0x0f,0x02,0x00,0x00,
									  	0x01,0x10,0xb7,0xa2,0x94,0x13,0xe6,0xda,0x4a,0x58,
									  	0x93,0xbe,0xf6,0x8e,0x1c,0xe1,0xc5,0xe6,0x54,0x2d,0x16
									  	*/
										0x68,0x31,0x00,0x43,0x05,0x35,0x00,0x00,0x00,0x00,
									  0x00,0x00,0x66,0x48,0x10,0x00,0x0D,0x05,0x02,0x00,
									  0x02,0x20,0x00,0x02,0x00,0x20,0x0F,0x02,0x00,0x00,
									  0x01,0x10,0x6C,0x04,0x5D,0x00,0xA9,0x99,0xAD,0x21,
									  0xE8,0xE1,0x1B,0x98,0xB7,0xAB,0xB1,0x87,0xD3,0x1B,0x16
                                     };
 		U16 cs16 = 0 ;
		*len = 0;
		__memcpy(meter_addr_req_buf+5,addr,6);
		
		//extern void Dlt698CrcReset();
		//extern U16 Crccheck_698(U8 *data ,int StartPos, int len);		

		*len = (*(meter_addr_req_buf+1)|((*(meter_addr_req_buf+2))<<8))-2;

		cs16 = ResetCRC16();
		
	    cs16 = CalCRC16(cs16,meter_addr_req_buf + 1, 0 , 11);

		__memcpy(meter_addr_req_buf+12,(U8 *)&cs16,2);
		
		cs16 = ResetCRC16();
		
	    cs16 = CalCRC16(cs16,meter_addr_req_buf + 1, 0 , *len);
		
		__memcpy(meter_addr_req_buf+*len+1,(U8 *)&cs16,2);

		app_printf("0x%04x,%d\n",cs16,*len);

		*len = sizeof(meter_addr_req_buf);
		dump_buf(meter_addr_req_buf ,*len);
		
		__memcpy(buf,meter_addr_req_buf,*len);
		
}



uint8_t sta_area_ind_voltage_or_frequncy_match(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq)
{
	uint8_t result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }

    return result;
}
void sta_area_ind_voltage_or_frequncy_rcv_deal(void *plistinfo, uint8_t *rcvbuf,uint16_t rcvlen,uint8_t protocoltype, uint16_t frameSeq)
{
	uint16_t i = 0;
	uint16_t word = 0;
	uint16_t id_len=0;
	uint8_t  flag_over = 0;
	uint8_t  *id_buf = NULL;
	uint8_t  type = area_ind_feature_info_t.type;
	FEATURE_INFO_t *info_t = area_ind_feature_info_t.info_t;
	add_serialcache_msg_t  *plist_msg = (add_serialcache_msg_t *)plistinfo;
	
	if(protocoltype == e_DLT698_MSG)
	{
		id_buf = zc_malloc_mem(50,"id_buf",MEM_TIME_OUT);
		if(NULL == id_buf)
		{
			return;
		}
		if(e_VOLTAGE == type)			
		{
			sta_area_ind_get_voltage_frequncy_by_698_frame(rcvbuf,rcvlen,id_buf,&id_len,NULL,NULL);
		}
		else if(e_FREQUNCY == type)
		{
			sta_area_ind_get_voltage_frequncy_by_698_frame(rcvbuf,rcvlen,NULL,NULL,id_buf,&id_len);
		}
		zc_free_mem(id_buf);
	}
	else
	{
		uint8_t voltage_645[4]={0x33 , 0x32 , 0x34 , 0x35};
		uint8_t frequncy_645[4]={0x35 , 0x33 , 0xB3 , 0x35};
		uint8_t *id=NULL;
		
		DLT645_GENERAL_STRUCTURE_t *pDlt645 = (DLT645_GENERAL_STRUCTURE_t *)plist_msg->FrameUnit;

		id = type==e_VOLTAGE?voltage_645:
				type==e_FREQUNCY?frequncy_645:NULL;
		if(id == NULL || 0 != memcmp(id, pDlt645->data, 4))
		{
			return;
		}
		id_buf = sta_area_ind_get_voltage_frequncy_by_645_frame(id,rcvbuf,rcvlen,&id_len);
	}
	if(info_t != NULL && id_buf != NULL && rcvlen != 0 && id_len != 0)
	{
		for(i = 0;i<id_len/2;i++)
		{				
			word = GetWord(id_buf+2*i);
			
			if(DevicePib_t.Prtcl == DLT698)
			{
				U8 byte_H = word/100;
				U8 byte_L = word%100;
				byte_H=INTtoBCD(byte_H);
				byte_L=INTtoBCD(byte_L);
				
				word = (byte_H<<8|byte_L);
			}
			
			if(word != 0xffff)
			{
				info_t->collect_info[info_t->effective_cnt++] = word;
				//app_printf("word : 0x%04x\n",word);
			}
			if(info_t->timer_cnt >= info_t->total_num)
			{
				flag_over = 1;
			}
			info_t += 1;
		}
	}
	else
	{
		;
	}	
	if(flag_over == 1)
	{
		if(e_VOLTAGE == type)
		{
			timer_stop(voltage_info_collect_timer,TMR_CALLBACK);
		}
		else if(e_FREQUNCY == type)
		{
			timer_stop(freq_info_collect_timer,TMR_CALLBACK);
		}
	}
	return;
}



/*************************************************************************
 * 函数名称 :	sta_area_ind_deal_feature_info_gather
 * 函数说明 :	特征信息收集
 * 参数说明 :	INDENTIFICATION_HEADER_t *pIndentificationHeader // 台区识别header
 * 返回值   :   void
 *************************************************************************/
 
void sta_area_ind_deal_feature_info_gather(INDENTIFICATION_IND_t *pIndentificationInd)
{
	U8 edge;
	net_nnib_ioctl(NET_GET_EDGE,&edge);
    U8 type;
    net_nnib_ioctl(NET_GET_DVTYPE, &type);

#if defined(ZC3750STA)
	
	//有配置沿信息使用沿信息，如果没有配置，根据表类型判断，如果不是三相表就使用上升沿
	packet_featureinfo(pIndentificationInd->Featuretypes, pIndentificationInd->SrcMacAddr,
									( edge != e_RESET) ? edge:
                                    ( (type == e_3PMETER) ? e_COLLECT_NEGEDGE: e_COLLECT_NEGEDGE));

#elif defined(ZC3750CJQ2)

	//二采如果配置了就用配置的信息，如果没有配置默认使用下降沿
	packet_featureinfo(pIndentificationInd->Featuretypes,pIndentificationInd->SrcMacAddr,
						( edge !=e_RESET)?edge:e_COLLECT_NEGEDGE);
#endif

}

void sta_area_ind_get_start_ntb_from_zero_data(U16 total_num, U32 CCOstartntb,U8 collectseq)
{
	S8 result=0;
	U16 j=0,effective_num=0;
	U32 startntb = 0;
	FEATURE_INFO_t *info_t = &Period_info_t[0];
	ZERODATA_t *zero_data_t = &ZeroData[0];
	
	
	for(j=0 ;j<MAX_PERIOD ; j++)
	{
		//app_printf("Period_info_t [%d] -> ",j);
		//dump_buf((U8 *)&(Period_info_t[j].effective_cnt),sizeof(FEATURE_INFO_t)-2*FEATURE_CNT);
		info_t->total_num    = total_num;
		info_t->startntb     = CCOstartntb;
		info_t->collect_seq  = collectseq;

		effective_num = info_t->total_num;
		result = area_ind_get_ntb_data_by_start_ntb((S16*)info_t->collect_info,
											&effective_num,
											info_t->startntb,
											zero_data_t,
											MAXNTBCOUNT,
											&startntb,
											1);


		if(info_t->total_num != effective_num)
		{
			effective_num = 0;
		}
		
		info_t->startntb = startntb;

		info_t->timer_cnt = info_t->effective_cnt = effective_num;
		
		app_printf("phase %d result : %d effective num : %d\n",j,result,effective_num);
		//dump_S16_buf("period data ",DEBUG_APP,(S16*)info_t->collect_info,info_t->timer_cnt);
		info_t += 1;
		zero_data_t += 1;
	}
}




/*************************************************************************
 * 函数名称 :	DistinguishResultInfo
 * 函数说明 :	台区判别结果
 * 参数说明 :	INDENTIFICATION_RESULT_DATA_t IndentificationResultData_t,台区判别结果载荷
				U8   Directbit,  //方向位
				U8   Startbit,   //启动位
				U8   *Srcaddr,   //源地址
				U8	 *DesMacaddr,//目的地址
				U8   Phase,      //采集相位
				U8   Featuretypes//特征类型
 * 返回值   :
 *************************************************************************/
void DistinguishResultInfo (	INDENTIFICATION_RESULT_DATA_t *IndentificationResultData_t,
								U8   Directbit,
								U8   Startbit,
								U8   *Srcaddr, //发送方 地址
								U8	 *DesMacaddr, //接收方 地址
								U8   Phase,    
								U8   Featuretypes)
{
	//MDB_t  mdb;
	U8 desaddr[6]; //更改为通讯地址
	U8 srcaddr[6]; //更改为通讯地址
	
	__memcpy(desaddr,DesMacaddr,6);
	//ChangeMacAddrOrder(desaddr);
	
	__memcpy(srcaddr,Srcaddr,6);
	ChangeMacAddrOrder(srcaddr);

	indentification_msg_request((uint8_t *)IndentificationResultData_t, 
							    sizeof(INDENTIFICATION_RESULT_DATA_t),
								Directbit,
								Startbit,
								Phase,
								sta_area_ind_recv_packet_seq,
								Featuretypes,
								e_DISTINGUISH_RESULT_INFO,
								e_UNICAST_FREAM,
								desaddr,
								srcaddr);
	return;
}



#endif


