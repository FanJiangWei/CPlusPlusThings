/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        Apsindentification.c
 * Purpose:     procedure for using
 * History:
 */
//#include "ZCheader.h"
#include "app_area_indentification_cco.h"
#include "app_area_indentification_sta.h"
#include "app_area_indentification_common.h"
#include "datalinkdebug.h"
#include "app_phase_position_cco.h"
#include "dev_manager.h"
#include "aps_interface.h"
#include "netnnib.h"
#include "app_cnf.h"
#include "app.h"
#include "app_gw3762.h"
#include "app_dltpro.h"
#include "app_event_report_cco.h"

INDENTIFICATION_FEATURE_INFO_t  area_ind_feature_info_t={
	.type = 0xff,
	.info_t = NULL,
};

static U32 gl_area_ind_ntb_buf[MAXNTBCOUNT];
U32 gl_diff[MAXNTBCOUNT];




ostimer_t *wait_finish_timer = NULL;

/*************************************************************************
 * 函数名称 :	indentification_msg_request
 * 函数说明 :	
 *    台区识别报文统一发送请求接口
 * 参数说明 :	
 *    uint8_t *payload   台区识别报文数据域
 *	  uint16_t len       台区识别报文数据域长度
 *    uint8_t Directbit   方向位，0为下行，1为上行
 *	  uint8_t Startbit    启动位，0为来自从动站，1为来自启动站
 *    uint8_t Phase       采集相位，0为默认相位
 *    uint16_t  seq       帧序号，具有递增性，上下行相关的报文，序号应该一致
 *    uint8_t Featuretypes  特征类型，e_VOLTAGE, e_FREQUNCY, e_PERIOD, 4为信噪比
 *    uint8_t Collectiontype 采集类型,标准中定义，用于区别台区识别报文不同的命令类型
 *    uint8_t sendtype   发送类型，单播或广播等
 *    uint8_t *desaddr   目标MAC地址
 *    uint8_t *srcaddr   源MAC地址
 * 返回值   :
 *************************************************************************/
void indentification_msg_request(uint8_t *payload, 
											  uint16_t len, 
											  uint8_t Directbit, 
											  uint8_t Startbit, 
											  uint8_t Phase,  
                                          	  uint16_t  seq,
                                          	  uint8_t Featuretypes, 
                                          	  uint8_t Collectiontype,
                                          	  uint8_t sendtype,
                                          	  uint8_t *desaddr, 
                                          	  uint8_t *srcaddr)
{
	INDENTIFICATION_REQ_t   *pIndentificationReq = NULL;
	pIndentificationReq = zc_malloc_mem(sizeof(INDENTIFICATION_REQ_t)+len, "indentification_msg_request", MEM_TIME_OUT);
	if(NULL == pIndentificationReq)
	{
		return;
	}
	//pIndentificationReq->protocol       = PROTOCOL_VERSION_NUM;
	//pIndentificationReq->HeaderLen      = sizeof(INDENTIFICATION_HEADER_t);
	pIndentificationReq->Direct         = Directbit;
	pIndentificationReq->StartBit       = Startbit;
	pIndentificationReq->Phase          = Phase;
	pIndentificationReq->DatagramSeq    = seq;
	pIndentificationReq->Featuretypes   = Featuretypes;
	pIndentificationReq->Collectiontype = Collectiontype;
	pIndentificationReq->sendtype       = sendtype;

	__memcpy(pIndentificationReq->DstMacAddr, desaddr, 6);
	__memcpy(pIndentificationReq->SrcMacAddr, srcaddr, 6);

	pIndentificationReq->payloadlen     = len;
	dump_buf(payload,pIndentificationReq->payloadlen);
	if(len)
		__memcpy(pIndentificationReq->payload, payload, pIndentificationReq->payloadlen);
	
	ApsIndentificationRequset(pIndentificationReq);
    
	zc_free_mem(pIndentificationReq);
}

                                              
/*************************************************************************
 * 函数名称 :	FeatureCollectStart
 * 函数说明 :	特征收集启动
 * 参数说明 :	FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t,  //特征收集启动载荷
				U8	 *DstMacaddr, //目标MAC地址，本命令为广播地址
				U8   Phase,       //采集相位
				U8   Featuretypes //特征类型
 * 返回值   :
 *************************************************************************/
void FeatureCollectStart (FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t,
							U8	 *DstMacaddr, //CCO addr
							U8   Phase,    
							U8   Featuretypes)
{
    app_printf("-----feature_info_task_start-----\n");
	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);
	
	indentification_msg_request((uint8_t *)&FeatureCollectStart_Data_t, 
							    sizeof(FEATURE_COLLECT_START_DATA_t),
								e_DOWNLINK,
								APS_MASTER_PRM,
								Phase,
								++ApsSendPacketSeq,
								Featuretypes,
								e_FEATURE_COLLECT_START,
								e_FULL_BROADCAST_FREAM_NOACK,
								DstMacaddr,
								ccomac);
	return;
}
/*************************************************************************
 * 函数名称 :	FeatureInfoGather
 * 函数说明 :	特征信息收集
 * 参数说明 :	U8   Directbit,  //方向位
				U8   Startbit,   //启动位
				U8   *Srcaddr,   //发送方 地址
				U8	 *DesMacaddr,//接收方 地址
				U8   Phase,      //采集相位
				U8   Featuretypes//特征类型
 * 返回值   :
 *************************************************************************/
void FeatureInfoGather (	U8      Directbit,
							U8   Startbit,
							U8   *Srcaddr, //发送方 地址
							U8	 *DesMacaddr, //接收方 地址
							U16  DateSeq,
							U8   Phase,    
							U8   Featuretypes)
{
	//MDB_t  mdb;
	U8 desaddr[6]; //更改为通讯地址
	U8 srcaddr[6]; //更改为通讯地址
	
	__memcpy(desaddr,DesMacaddr,6);
//	ChangeMacAddrOrder(desaddr);
	
	__memcpy(srcaddr,Srcaddr,6);
	ChangeMacAddrOrder(srcaddr);

	indentification_msg_request(NULL, 
						    0,
							Directbit,
							Startbit,
							Phase,
							DateSeq,
							Featuretypes,
							e_FEATURE_INFO_GATHER,
							e_UNICAST_FREAM,
							desaddr,
							srcaddr);
	return;
}
/*************************************************************************
 * 函数名称 :	FeatureInfoInforming
 * 函数说明 :	特征信息告知
 * 参数说明 :	U8   *payload,    //载荷指针
				U16  len,         //载荷长度
				U8   Directbit,   //方向位
				U8   Startbit,    //启动位
				U8   *Srcaddr,    //发送方 地址
				U8	 *DesMacaddr, //接收方 地址
				U8   Phase,       //采集相位
				U8   Featuretypes //特征类型
 * 返回值   :
 *************************************************************************/
void FeatureInfoInforming (	U8   *payload,
							U16  len,
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

	indentification_msg_request(payload, 
					    len,
						Directbit,
						Startbit,
						Phase,
                        #if defined(STATIC_MASTER)
						++ApsSendPacketSeq,
						#else
						sta_area_ind_recv_packet_seq,
						#endif
						Featuretypes,
						e_FEATURE_INFO_INFORMING,
                        #if defined(STATIC_MASTER)
                        e_FULL_BROADCAST_FREAM_NOACK,//CCO告知使用广播，STA告知使用单播
                        #else
                        e_UNICAST_FREAM,
                        #endif
						desaddr,
						srcaddr);
	return;
}
/*************************************************************************
 * 函数名称 :	DistinguishResultQuery
 * 函数说明 :	台区识别结果查询
 * 参数说明 :	U8   Directbit,  //方向位
				U8   Startbit,   //启动位
				U8   *Srcaddr,   //发送方 地址
				U8	 *DesMacaddr,//接收方 地址
				U8   Phase,      //采集相位
				U8   Featuretypes//特征类型
 * 返回值   :
 *************************************************************************/
void DistinguishResultQuery (	U8   Directbit,
								U8   Startbit,
								U8   *Srcaddr, //发送方 地址
								U8	 *DesMacaddr, //接收方 地址
								U16  DateSeq,
								U8   Phase,    
								U8   Featuretypes)
{
	//MDB_t  mdb;
	//U8 desaddr[6]; //更改为通讯地址
	U8 srcaddr[6]; //更改为通讯地址

	//__memcpy(desaddr,DesMacaddr,6);
	//ChangeMacAddrOrder(desaddr);
	
	__memcpy(srcaddr,Srcaddr,6);
	ChangeMacAddrOrder(srcaddr);

	indentification_msg_request(NULL, 
							    0,
								Directbit,
								Startbit,
								Phase,
								DateSeq,
								Featuretypes,
								e_DISTINGUISH_RESULT_QUERY,
								e_UNICAST_FREAM,
								DesMacaddr,
								srcaddr);
	return;
}





//-------------------------------------------------------------------------------------------------
// 函数名：U8 CalculateNTBDiff(U32 ntb ,U32 *staNtb,U8 num,U8 *Phase,U16 *zeroData)
// 
// 功能描述：
//     本函数用于计算某一出线的相线是否正确
// 参数：
//     U32 ntb      CCO记录的与上报基值最相近的过零点NTB值
//     U32 *staNtb  STA上报上来的某一相位的过零NTB值
//     U8 numa      某相线的告知数量
//     U8 *Phase    指针变量传递一个U8字节地址进去用于存放相位识别结果
//     U16 *zeroData  返回STA过零点与CCO的差值
// 返回值：
//     U8    TRUE or FALSE 以CCOA相下降沿为基准，识别上升沿TRUE，下降沿FALSE
//-------------------------------------------------------------------------------------------------
U8 CalculateNTBDiff(U32 ntb ,U32 *staNtb,U8 num,U8 *Phase, U16 *zeroData)
{
	U8 i;
	S32 differ;
	S32 averge = 0;
	U16 range = 1500;
    
	net_printf("ntb=0x%08x\n",ntb);
    
	num = 1;
	for(i = 0; i<num ;i++)
	{
		net_printf("ZeroNTB[%d]=0x%08x\n",i,staNtb[i]);
		differ = (abs(staNtb[i]-ntb) /25) % 20000;
		if(abs(staNtb[i] - ntb) > 0x0FFFFFFF)	//U32翻转
		{
			if(staNtb[i] > ntb)
			{
				differ = ((ntb + (0xFFFFFFFF - staNtb[i]))/25) % 20000;
			}
			else
			{
				differ = ((staNtb[i] + (0xFFFFFFFF - ntb))/25) % 20000;
			}
		}
		//#if defined(STATIC_NODE)
		if(staNtb[i] < ntb) //从模块采集的数据在CCO前 	 
		{
			differ = 20000-differ;
		}
		//#endif
		net_printf("differ=%d\n",differ);
		if(differ < range)
		{
			differ = 20000-differ;
		}

        averge += differ;

    }
    averge = averge/num;
    net_printf("------averge=%d----\n",averge);
//	 0--|--3333--|--6666--|--10000--|--13333--|--16666--|--20000
//	 A		C		  B		    A		  C		    B	      A

    if(averge <3333+range && averge >3333-range)  //NL-C
    {
        *Phase = e_C_PHASE;
        if(NULL != zeroData)
            *zeroData = (abs(averge - 3333));
        return TRUE;
    }
    else if(averge <6666+range && averge >6666-range)
    {
        *Phase = e_B_PHASE;
        if(NULL != zeroData)
            *zeroData = (abs(averge - 6666));
        return FALSE;
    }
    else if(averge <10000+range && averge >10000-range)//NL-A
    {
        *Phase = e_A_PHASE;
        if(NULL != zeroData)
            *zeroData = (abs(averge - 10000));
        return TRUE;
    }
    else if(averge <13333+range && averge >13333-range)
    {
        *Phase = e_C_PHASE;
        if(NULL != zeroData)
            *zeroData = (abs(averge - 13333));
        return FALSE;
    }
    else if(averge <16666+range && averge >16666-range)//NL-B
    {
        *Phase = e_B_PHASE;
        if(NULL != zeroData)
            *zeroData = (abs(averge - 16666));
        return TRUE;
    }
    else if(averge >=20000- range)
    {
        *Phase = e_A_PHASE;
        if(NULL != zeroData)
            *zeroData = (abs(averge - 20000));
        return FALSE;
    }
    
    *Phase =0;
    return FALSE;	 
}



//计算曲线相似度
U8	CalculateNTBcurve(S16 *mycurve,S16 *CCOcurve, U16 perNum)
{
	U16 samenum=0; 
	U16	diffnum=0;
	U16	 i;
	U8 pram =1;
	for(i=0;i<perNum;i++)
	{
		if(mycurve[i]>2500	|| CCOcurve[i]>2500)//存在不连续的过零点
		{
			app_printf("--err--myDiff=%4d,CCODiff=%4d---\n",mycurve[i],CCOcurve[i]);
			return 0; 
		}
		if( ((mycurve[i+1]-mycurve[i])/pram) >0)
		{
			mycurve[i] = 1;
		}
		else if( ((mycurve[i+1]-mycurve[i])/pram) ==0)
		{
			mycurve[i] = 0;
		}
		else if( ( (mycurve[i+1]-mycurve[i])/pram)<0)
		{
			mycurve[i] = -1;
		}
		
		if( ((CCOcurve[i+1]-CCOcurve[i])/pram)>0)
		{
			CCOcurve[i] = 1;
		}
		else if( ((CCOcurve[i+1]-CCOcurve[i])/pram)==0)
		{
			CCOcurve[i] = 0;
		}
		else if( ((CCOcurve[i+1]-CCOcurve[i])/pram)<0)
		{
			CCOcurve[i] = -1;
		}
	
	
		if( mycurve[i] == CCOcurve[i])
		{
			//app_printf("----Diff OK\n");
			samenum++;
		}
		else
		{
			//app_printf("----Diff no\n");
			diffnum++;
		}
		
	}
	if(perNum !=0)
	{
		app_printf("samenum=%d,diffnum=%d\n",samenum,diffnum);
		samenum = (samenum*100)/perNum;	
	}
	else
	{
		app_printf("CCO downfream err!\n");
	}
	return samenum;

}


/**************************************以上为台区识别发送数据接口************************************/

/**************************************以下为台区识别接收解析数据接口************************************/

/*************************************************************************
 * 函数名称 :	void area_ind_set_featrue_and_start_timer(U8 featuretypes,FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t)
 * 函数说明 :	
 *      本函数依据不同的特征类型，和特征采集启动命令数据，来设置特征信息采集启动定时器
 * 参数说明 :	
 *     U8 featuretypes   特征类型，如工频周期为e_PERIOD
 *     FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t   特征采集启动命令数据
 * 返回值   :   
 *     void  无返回值
 *************************************************************************/
void area_ind_set_featrue_and_start_timer(U8 featuretypes, FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t)
{
	U8 i=0 ,num =0;
	U32 nowtime = get_phy_tick_time();										//获取当前tick
	U32 time = (feature_collect_start_data_t->Startntb - nowtime)/250000;	//10ms 单位
	
	FEATURE_INFO_t *Feature_info_t = NULL;
	
    app_printf("starttime : 0x%08x nowtime : 0x%08x start collect time : 0x%08x\n",feature_collect_start_data_t->Startntb,nowtime,time);
    
	switch (featuretypes)
	{
		case e_VOLTAGE:
			voltage_info_collect_timer_init();
            modify_voltage_info_collect_timer(time,1000*feature_collect_start_data_t->CollectCycle);
			Feature_info_t = &Voltage_info_t[0];
			num = MAX_VOLTAGE;
			break;
		case e_FREQUNCY:
			freq_info_collect_timer_init();
            modify_freq_info_collect_timer(time,1000*feature_collect_start_data_t->CollectCycle);
			Feature_info_t = &Frequncy_info_t[0];
			num = MAX_FREQUNCY;
			break;
		case e_PERIOD:
			period_info_collect_timer_init();
            modify_period_info_collect_timer(time*10+20*(feature_collect_start_data_t->CollectNum+3));//20ms*num//1ms unit
			Feature_info_t = &Period_info_t[0];
			num = MAX_PERIOD;
			break;
		default:
			break;
	}

	//初始化缓存buf
	if(Feature_info_t)
	{
		memset(Feature_info_t, 0x00, sizeof(FEATURE_INFO_t));
	}
	
	Feature_info_t->total_num = feature_collect_start_data_t->CollectNum;		//采集数量
	Feature_info_t->cycle = feature_collect_start_data_t->CollectCycle;			//采集间隔，工频周期不起作用
	Feature_info_t->startntb = feature_collect_start_data_t->Startntb;			//起始NTB
	Feature_info_t->collect_seq = feature_collect_start_data_t->CollectSeq;		//采集序号
	
	for(i = 1; i < num; i++)
	{
		__memcpy(Feature_info_t+i,Feature_info_t,sizeof(FEATURE_INFO_t));
	}
	
    #if defined(STATIC_NODE)
	memset((U8 *)&sta_area_ind_dis_result_t[0],0,3*sizeof(INDENTIFICATION_RESULT_DATA_t));
	#endif
}

/*************************************************************************
 * 函数名称 :	DealFeatureCollectStart
 * 函数说明 :	特征信息收集启动
 * 参数说明 :	INDENTIFICATION_IND_t *pIndentificationInd // 台区识别header
 * 返回值   :   void
 *************************************************************************/
void DealFeatureCollectStart(INDENTIFICATION_IND_t *pIndentificationInd)
{
	FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t =
		                                 (FEATURE_COLLECT_START_DATA_t *)pIndentificationInd->payload;

	app_printf("pIndentificationInd 0x%08x -> ",pIndentificationInd);
	dump_buf((U8 *)pIndentificationInd,sizeof(INDENTIFICATION_IND_t)+pIndentificationInd->payloadlen);

	app_printf("feature start collect  0x%08x -> ",feature_collect_start_data_t);
	dump_buf((U8 *)(feature_collect_start_data_t),sizeof(FEATURE_COLLECT_START_DATA_t));
	//PublicResult_t.Collectseq = feature_collect_start_data_t->CollectSeq;
	//PublicResult_t.CalculateMaxNum =MAX_CACULATE-5;

	
	//STA接收到特征采集开始
	area_ind_set_featrue_and_start_timer(pIndentificationInd->Featuretypes,feature_collect_start_data_t);
}

void packet_pma(FEATURE_INFO_t *feature_info_t ,U8 *out_phase,U8 *FeatureInfoSeq,U16 *FeatureInfoSeqlen,U16 *infonum)
{
	U8 i = 0;
	U8 phase = 0;

	FEATURE_INFO_t *feature_info_payload_t = feature_info_t;
	*FeatureInfoSeqlen = 0;	
	//过零采集起始NTB
	//__memcpy(FeatureInfoSeq,(U8 *)&feature_info_t->startntb,4);
	(*FeatureInfoSeqlen) += 4;
	//保留
	(*FeatureInfoSeqlen) += 1;
	
	
	for(i=0; i<3; i++)
	{
		//报告数量
		//如果有有效数据，则改出线相位数量为采集的个数，但是里面可能有采集失败的无效数据
		//如果无有效数据，则改出线相位数量为0
		//以上只是对入参对应的相位报告数量赋值，其他的相位为报告数量为0
		*(FeatureInfoSeq+(*FeatureInfoSeqlen)++)=
				(feature_info_t->effective_cnt != 0)?feature_info_t->timer_cnt:0;

		if(0 != feature_info_t->effective_cnt)//该出线相位有有效数据,则该出线相位置位
		{
			if(phase == 0)//记录当前数据出线相位
			{
				__memcpy(FeatureInfoSeq,(U8 *)&feature_info_t->startntb,4);
				phase |= (1<<i); 
				*out_phase = i+1;
				app_printf("get ntb phase : %d,startntb=%08x\n",phase,feature_info_t->startntb);
			}
			else//如果有一个以上的数据序列，认为是默认出线相位
			{
				*out_phase = 0;
			}
			*infonum += feature_info_t->effective_cnt;
		}
		
		feature_info_t += 1;//每次偏移一个结构体
	}
	
	//edge = (feature == e_PERIOD)?edge:0;
	
	for(i=0;i<3;i++)
	{
		//电压，频率，周期值
		if(feature_info_payload_t->effective_cnt != 0)
		{
			__memcpy(FeatureInfoSeq+*FeatureInfoSeqlen,(U8 *)&feature_info_payload_t->collect_info[0],2*feature_info_payload_t->timer_cnt);
			(*FeatureInfoSeqlen) += 2*feature_info_payload_t->timer_cnt;
		}
		else
		{
			//如果无有效数据，则为空载荷
		}
		feature_info_payload_t += 1;//每次偏移一个结构体
	}

}
/*************************************************************************
 * 函数名称 :	PacketFeatureInfoInformingData
 * 函数说明 :	打包数据特征信息载荷，和特征信息告知函数（FeatureInfoInforming）配合使用
 * 参数说明 :	U8   Collectmode,        //采集方式
				U8   TEI,                //采集点tei
				U8   CollectSeq,         //采集序号
				U8   *FeatureInfoSeq1,   //特征序列1
				U16  FeatureInfoSeqlen1, //特征序列1长度
				U8   *FeatureInfoSeq2,   //特征序列2
				U16  FeatureInfoSeqlen2, //特征序列2长度
				U8   *payload,           //打完帧的指针
				U16  *len                //打完包的数据长度
 * 返回值   :
 *************************************************************************/
S8 PacketFeatureInfoInformingData(	U8   Collectmode,
									U16   TEI,
									U8   CollectSeq,
									U16	 CollectNum,
									U8   *FeatureInfoSeq1,
									U16  FeatureInfoSeqlen1,
									U8   *FeatureInfoSeq2,
									U16  FeatureInfoSeqlen2,
									U8   *payload,
									U16  *len)
{
	FEATURE_INFO_INFORMING_DATA_t *FeatureInfoInformingData_t = (FEATURE_INFO_INFORMING_DATA_t *)payload;
	*len = sizeof(FEATURE_INFO_INFORMING_DATA_t);
    
	if(FeatureInfoSeq1 == NULL || FeatureInfoSeqlen1 == 0)
	{
		app_printf("FeatureInfoSeq1 is null\n");
		return FALSE;
	}
	FeatureInfoInformingData_t->TEI  =  TEI;  
	FeatureInfoInformingData_t->CollectMode = Collectmode;
	FeatureInfoInformingData_t->res = 0;
	FeatureInfoInformingData_t->CollectSeq = CollectSeq;
	FeatureInfoInformingData_t->InformingNum = (FeatureInfoSeq2 == NULL || FeatureInfoSeqlen2 == 0)?1:2;
	
	__memcpy(payload+*len,FeatureInfoSeq1,FeatureInfoSeqlen1);
	*len += FeatureInfoSeqlen1;

	if(FeatureInfoInformingData_t->InformingNum == 2)
	{
		__memcpy(payload+*len,FeatureInfoSeq2,FeatureInfoSeqlen2);
		*len += FeatureInfoSeqlen2;
	}
	FeatureInfoInformingData_t->InformingNum  = CollectNum;
	//app_printf("PacketFeatureInfoInformingData -> ");
	//dump_buf(payload,*len);
	return OK;
}


/*************************************************************************
 * 函数名称 :   void packet_featureinfo(U8 feature, U8 *addr, U8 edge)
 * 函数说明 :   
 *     本函数被CCO用于打包特征信息告知命令数据，并且发送出去
 * 参数说明 :   
 *     U8 feature  特征类型
 *     U8 *addr    告知目的MAC地址，广播告知为全FF
 * 返回值   :    
 *     void  无返回值
*************************************************************************/
void packet_featureinfo(U8 feature, U8 *addr, U8 edge)
{
	FEATURE_INFO_t *feature_info_t = 
		feature == e_VOLTAGE?&Voltage_info_t[0]:
		feature == e_FREQUNCY?&Frequncy_info_t[0]:
        #if defined(STATIC_MASTER)
		(feature == e_PERIOD && (edge&e_COLLECT_NEGEDGE))?&Period_info_t[0]:   // 下降沿
		(feature == e_PERIOD && (edge&e_COLLECT_POSEDGE))?&Period_info_t[3]:NULL;  // 上升沿
        #elif defined(STATIC_NODE)
		(feature == e_PERIOD )?&Period_info_t[0]:NULL;
		#endif
    
	if(feature_info_t == NULL)
	{
		app_printf("pma error!!!\n");
		return;
	}
	
	U8 *payload = NULL;
	U16  len=0;
	U8 seq=0;
	U8 out_phase = 0;
	U8 *FeatureInfoSeq1 = NULL;
	U16 FeatureInfoSeqlen1 = 0;	

	U8 *FeatureInfoSeq2 = NULL;
	U16 FeatureInfoSeqlen2 = 0;	

	U16 Num1=0;
	U16 Num2=0;
	payload =  zc_malloc_mem(2*FEATURE_CNT,"payload",MEM_TIME_OUT);

	if(NULL == payload)
	{
		return;
	}
	//if(feature == e_VOLTAGE || feature == e_FREQUNCY || (feature == e_PERIOD && (edge&e_COLLECT_NEGEDGE)))
	#if defined(STATIC_MASTER)
	if(feature == e_VOLTAGE || feature == e_FREQUNCY ||( feature == e_PERIOD && (edge & e_COLLECT_NEGEDGE)))
	#else
	if(feature == e_VOLTAGE || feature == e_FREQUNCY ||( feature == e_PERIOD))
	#endif
	{
		seq = feature_info_t->collect_seq;
		FeatureInfoSeq1 = zc_malloc_mem(2*FEATURE_CNT,"FeatureInfoSeq1",MEM_TIME_OUT);
		packet_pma(feature_info_t,&out_phase,FeatureInfoSeq1,&FeatureInfoSeqlen1,&Num1);
	}
    #if defined(STATIC_MASTER)
	if(feature == e_PERIOD && (edge&e_COLLECT_POSEDGE))
	{
		feature_info_t = &Period_info_t[3];
		seq = feature_info_t->collect_seq;
		if(NULL==FeatureInfoSeq1)
		{
			FeatureInfoSeq1 = zc_malloc_mem(2*FEATURE_CNT,"FeatureInfoSeq2",MEM_TIME_OUT);
			packet_pma(feature_info_t,&out_phase,FeatureInfoSeq1,&FeatureInfoSeqlen1,&Num1);
		}
		else
		{
			FeatureInfoSeq2 = zc_malloc_mem(2*FEATURE_CNT,"FeatureInfoSeq2",MEM_TIME_OUT);
			packet_pma(feature_info_t,&out_phase,FeatureInfoSeq2,&FeatureInfoSeqlen2,&Num2);
		}
	}
	#endif
	
	PacketFeatureInfoInformingData(  edge,  //U8   Collectmode, 上升沿、下降沿或双沿
									 APP_GETTEI(),//	U8   TEI,
									seq, //	U8   CollectSeq,
									Num1+Num2,
									FeatureInfoSeq1,//	U8   *FeatureInfoSeq1,
									FeatureInfoSeqlen1, //	U16  FeatureInfoSeqlen1,
									FeatureInfoSeq2, //	U8   *FeatureInfoSeq2,
									FeatureInfoSeqlen2, //	U16  FeatureInfoSeqlen2,
									 payload,//	U8   *payload,
									 &len);//	U16  *len);
	if(FeatureInfoSeq1)
    {   
	    zc_free_mem(FeatureInfoSeq1);
    }
    if(FeatureInfoSeq2)
    {
	    zc_free_mem(FeatureInfoSeq2);
    }
	
	FeatureInfoInforming (	payload,		//U8   *payload,
							len,		//U16  len,
                            #if defined(STATIC_NODE)
							e_UPLINK,		//U8   Directbit,
							APS_SLAVE_PRM,		//U8   Startbit,
							DevicePib_t.DevMacAddr,	//U8   *Srcaddr, //发送方 地址
                            #elif defined(STATIC_MASTER)
							e_DOWNLINK,		//U8   Directbit,
							APS_MASTER_PRM,		//U8   Startbit,
							FlashInfo_t.ucJZQMacAddr,	//U8   *Srcaddr, //发送方 地址
							#endif
							addr,//U8	 *DesMacaddr, //接收方 地址
							out_phase,		//U8   Phase,    
							feature);		//U8   Featuretypes);

	zc_free_mem(payload);
}



void deal_voltage_or_frequncy_pma(FEATURE_INFO_SEQ_t *feature_info_seq_t,U8 Featuretypes)
{
	U8 i=0;
	U16 offset = 0;
	
	for(i=0;i<3;i++)
	{
		if(*(&feature_info_seq_t->phase1num+i) != 0)
		{
			
			app_printf("phase%dnum : %d\n",i+1,*(&feature_info_seq_t->phase1num+i));
			show_bcd_to_char((U8 *)feature_info_seq_t+sizeof(FEATURE_INFO_SEQ_t)+offset,2**(&feature_info_seq_t->phase1num+i));

			offset += *(&feature_info_seq_t->phase1num+i)*2;
			
			//处理电压，频率识别算法
		}
	}
}
U8 get_phase_by_startntb(U32 ccostartntb,U32 stastartntb,U8 *inverse)
{
	U8 phase = 0;
	*inverse = CalculateNTBDiff(ccostartntb,&stastartntb,1,&phase,NULL);

	//app_printf("ccostartntb 0x%08x ,stastartntb 0x%08x ,phase : %d\n",ccostartntb,stastartntb,phase);
	return phase;
}
U8 get_collect_seq(U8* CCOmacaddr,U8 *edgeType)
{
	U8 i;
	for(i=0;i<COMMPAER_NUM;i++)
	{
		if( memcmp(Final_result_t.PublicResult_t[i].Macaddr,CCOmacaddr,6) ==0 )
		{
			*edgeType = Final_result_t.PublicResult_t[i].EdgeType;
			return	Final_result_t.PublicResult_t[i].Collectseq ;



		}
	}
	return 0;

}
void record_collect_seq(U8 collectseq,U8* CCOmacaddr,U8 Edge )
{
	U8 i;
	for(i=0;i<COMMPAER_NUM;i++)
	{
		if( memcmp(Final_result_t.PublicResult_t[i].Macaddr,CCOmacaddr,6) ==0 )
		{

			Final_result_t.PublicResult_t[i].Collectseq = collectseq ;
			Final_result_t.PublicResult_t[i].EdgeType   = Edge;
			app_printf("CCOmacaddr %02x%02x%02x%02x%02x%02x,collectseq = %d,Edge = %d\n",CCOmacaddr[0],
																						 CCOmacaddr[1],
																						 CCOmacaddr[2],
																						 CCOmacaddr[3],
																						 CCOmacaddr[4],
																						 CCOmacaddr[5],
																						collectseq,Edge
																						);
			return;
		}
	}
}

void cacl_nbnet_difftime(uint32_t SNID,uint32_t       Sendbts,uint32_t Localbts)
{
  U8  i=0;
  for(i=1;i<COMMPAER_NUM;i++)
  {
  	 if(Final_result_t.PublicResult_t[i].SNID == SNID)
  	 {
  	 	Final_result_t.PublicResult_t[i].Differtime = Localbts-Sendbts;//更新定差
  	 	return;
  	 }
  }
  for(i=1;i<COMMPAER_NUM;i++)
  {
  	if(Final_result_t.PublicResult_t[i].SNID == 0)
  	{
  	  	Final_result_t.PublicResult_t[i].Differtime = Localbts-Sendbts;
		Final_result_t.PublicResult_t[i].SNID = SNID;
		app_printf("add SNID=%08x\n",SNID);
		return;
  	}
  }

}

U32 convert_startntb(U32 startntb,U8* CCOmacaddr)
{
	U8 i=0;
	//[0]存放本网络的采集信息
	for(i=1;i<COMMPAER_NUM;i++)
	{
		if( memcmp(Final_result_t.PublicResult_t[i].Macaddr,CCOmacaddr,6) ==0 )
		{
			return (startntb + Final_result_t.PublicResult_t[i].Differtime) ;
		}
	}
	return 0;
}

S8 deal_period_pma(FEATURE_INFO_SEQ_t *feature_info_seq_t , U8 edge , U8 CollectSeq ,U16 tei, U8 *result,U8 *value,U16 collectnum,U8* CCOmacaddr)
{
	//U8 i=0;
	//U16 offset = 0;
	U8  CollectType = 0;
    #if defined(STATIC_MASTER)
	U8 edgeoffset = (edge & e_COLLECT_NEGEDGE)?0:
					(edge & e_COLLECT_POSEDGE)?3:
					0xff;
	#endif

	
	if(get_collect_seq(CCOmacaddr,&CollectType) == CollectSeq && CollectType == edge)
	{
		*value =0;
		app_printf("err : CollectSeq is wrong : 0x%x!\n",Period_info_t[0].collect_seq);
		return FALSE;
	}
	record_collect_seq(CollectSeq,CCOmacaddr,edge);
	U8 phase = 0;
	S16 *ccodata = NULL,*stadata = NULL;
	U16 ccodatanum = 0,stadatanum = 0;
	U32 ccontb,stantb;
	U8  inverse = 0;
    #if defined(STATIC_MASTER)
		ccontb = Period_info_t[edgeoffset].startntb;
		stantb = feature_info_seq_t->startntb;

		phase = get_phase_by_startntb(ccontb,stantb,&inverse);
		if(inverse == TRUE)//如果逆序则按照逆序计算
		{
			app_printf("line error\n");
			edgeoffset = (edgeoffset == 3)?0:3;
			//set_LN_Devicelist(tei,1);
		}

		stadatanum =(feature_info_seq_t->phase1num != 0)?feature_info_seq_t->phase1num:
					(feature_info_seq_t->phase2num != 0)?feature_info_seq_t->phase2num:
					(feature_info_seq_t->phase3num != 0)?feature_info_seq_t->phase3num:0;
		stadata = (S16 *)((U8 *)feature_info_seq_t+sizeof(FEATURE_INFO_SEQ_t));	

		ccodata = (S16 *)(Period_info_t[phase-1+edgeoffset].collect_info);
		ccodatanum = Period_info_t[phase-1+edgeoffset].effective_cnt;
	
		if(stadatanum != ccodatanum)
        {
            return FALSE;
        }
    #elif defined(STATIC_NODE)
		U8 i = 0;
		U8 ccomac[6];
		net_nnib_ioctl(NET_GET_CCOADDR,ccomac);

		if(memcmp(CCOmacaddr, ccomac, 6)==0)
		{
			ccontb = feature_info_seq_t->startntb;
		}
		else
		{
			//根据维护的网络定差，计算出新的startntb
			ccontb =convert_startntb(feature_info_seq_t->startntb,CCOmacaddr);
		}
		app_printf("ccontb=0x%08xcollectnum=%d,->phase1num=%d,->phase2num=%d,->phase3num=%d\n",ccontb,collectnum,
							feature_info_seq_t->phase1num,feature_info_seq_t->phase2num,feature_info_seq_t->phase3num);
		
		sta_area_ind_get_start_ntb_from_zero_data(feature_info_seq_t->phase1num, ccontb,CollectSeq);
		
		for(i=0;i<3;i++)
		{
			if(Period_info_t[i].total_num == Period_info_t[i].effective_cnt)
			{
				//U8	*pheader;
				//pheader =(U8 *)feature_info_seq_t;
				//app_printf("feature_info_seq_t=%08x   ",feature_info_seq_t);
				stantb = Period_info_t[i].startntb;

				phase = get_phase_by_startntb(ccontb,stantb,&inverse);
				app_printf("inverse=%d\n",inverse);
				if(inverse == TRUE)
				{
					if(edge & e_COLLECT_POSEDGE && edge & e_COLLECT_NEGEDGE)
					{

						feature_info_seq_t=(FEATURE_INFO_SEQ_t *)(	((U8 *)feature_info_seq_t) + (sizeof(FEATURE_INFO_SEQ_t)+feature_info_seq_t->phase1num*2+feature_info_seq_t->phase2num*2+feature_info_seq_t->phase3num*2));					
						//app_printf("feature_info_seq_t=%08x\n",feature_info_seq_t);
						if(memcmp(CCOmacaddr, ccomac, 6)==0)
						{
							ccontb = feature_info_seq_t->startntb;
						}
						else
						{
							//根据维护的网络定差，计算出新的startntb
							ccontb =convert_startntb(feature_info_seq_t->startntb,CCOmacaddr);
						}
						app_printf("ccontb=0x%08x\n",ccontb);
						if(phase>1) ccontb = ccontb+(phase-1) * 25*6666;
						app_printf("phase=%d,new ccontb=0x%08x\n",phase,ccontb);
						sta_area_ind_get_start_ntb_from_zero_data(feature_info_seq_t->phase1num, ccontb,CollectSeq);
					}
					else//沿错误 
					{
						app_printf("edge is wrong! cco edge is %d\n",edge);
						return FALSE;
					}
				}
				else
				{	
					app_printf("ccontb=0x%08x\n",ccontb);
					if(phase>1) ccontb =ccontb+ (phase-1) * 25*6666;
					app_printf("phase=%d,new ccontb=0x%08x\n",phase,ccontb);
					sta_area_ind_get_start_ntb_from_zero_data(feature_info_seq_t->phase1num, ccontb,CollectSeq);
				}
				//feature_info_seq_t = (FEATURE_INFO_SEQ_t *)pheader;
				stadata = (S16 *)Period_info_t[i].collect_info;
				stadatanum = Period_info_t[i].effective_cnt;

				ccodatanum = feature_info_seq_t->phase1num;//对应相数量

				if(phase == e_A_PHASE)
				{
					ccodata = (S16 *)((U8 *)feature_info_seq_t+sizeof(FEATURE_INFO_SEQ_t));	
					ccodatanum = feature_info_seq_t->phase1num;
				}
				else if(phase == e_B_PHASE)
				{
					ccodata = (S16 *)((U8 *)feature_info_seq_t+sizeof(FEATURE_INFO_SEQ_t)+feature_info_seq_t->phase1num*2);	
					ccodatanum = feature_info_seq_t->phase2num;
				}
				else if(phase == e_C_PHASE)
				{
					ccodata = (S16 *)((U8 *)feature_info_seq_t+sizeof(FEATURE_INFO_SEQ_t)+feature_info_seq_t->phase1num*2+feature_info_seq_t->phase2num*2);	
					ccodatanum = feature_info_seq_t->phase3num;
				}
				else
				{
					return FALSE;
				}
				break;
			}
		}
		
	#endif
	
	

	app_printf("phase%dnum : %d\n",phase,ccodatanum);

	if(ccodatanum < 100)
	{
	    //dump_S16_buf("data cco",DEBUG_APP,ccodata,ccodatanum);
	    //dump_S16_buf("data sta",DEBUG_APP,stadata,stadatanum);
	    app_printf("data cco");
	    //dump_buf((U8 *)&ccodata,ccodatanum);
        app_printf("data sta");
	    //dump_buf((U8 *)&stadata,stadatanum);
	}

	if(TRUE==get_result_by_variance(ccodata,ccodatanum,stadata,stadatanum,value))
	{

			*result = e_INDENTIFICATION_RESULT_BELONG;

	}
	else
	{

			*result = e_INDENTIFICATION_RESULT_NOTBELONG;
	

	}
	
	
	return TRUE;
}

/*通过SNID填充MACADDR*/
void add_ccomacaddr(U32 SNID,U8* Macaddr)
{
	U8 i=0;
	app_printf("Macaddr:");
	dump_buf(Macaddr, 6);
	if(SNID == APP_GETSNID())
	{
		Final_result_t.PublicResult_t[i].SNID = SNID;
		__memcpy(Final_result_t.PublicResult_t[i].Macaddr,Macaddr,6);
		return;
	}
	for(i=1;i<COMMPAER_NUM;i++)
	{
		if(SNID == Final_result_t.PublicResult_t[i].SNID)	
		{
			__memcpy(Final_result_t.PublicResult_t[i].Macaddr,Macaddr,6);
			break;
		}
	}
}

void wait_finish_timerCB(struct ostimer_s *ostimer, void *arg)
{
	U8 NullAddr[6]={0};

	U8 i=0;


	if(Final_result_t.PublicResult_t[0].FinishFlag == FINISH)
	{
		;
	}
	else
	{
		timer_start(wait_finish_timer);
	}

	for(i=0;i<COMMPAER_NUM;i++)
	{
		if( memcmp(Final_result_t.PublicResult_t[i].Macaddr,NullAddr,6) !=0 
					&&Final_result_t.PublicResult_t[i].FinishFlag == UNFINISH)
		{

				Final_result_t.PublicResult_t[i].CalculateNum = Final_result_t.PublicResult_t[i].CalculateMaxNum;
				Final_result_t.PublicResult_t[i].FinishFlag = FINISH;
				Final_result_t.PublicResult_t[i].SameRatio = 0;


		}
	}
	Final_result_t.FinishFlag = FINISH;
}

U8	existNofinishedNet(U8* Macaddr)
{
	U8 NullAddr[6]={0};
	U8  NbSNID=0;
	U8 i=0;
	if(Final_result_t.PublicResult_t[0].FinishFlag == UNFINISH) //当前入网的网络未识别完成
	{
		return 0;
	}
	for(i=0;i<COMMPAER_NUM;i++)
	{
		if(memcmp(Final_result_t.PublicResult_t[i].Macaddr,Macaddr,6) !=0 
				&& memcmp(Final_result_t.PublicResult_t[i].Macaddr,NullAddr,6) !=0 )
		{
			if(Final_result_t.PublicResult_t[i].FinishFlag == UNFINISH)//统计正在识别且未完成的邻居网络数
			{
				NbSNID++;
			}
		}
	}
	//不在存在其他网络时，返回1，完成识别。
	return NbSNID == 0 ? 1:0;
}

void modify_wait_finish_timer(U32 ms)
{

	if (wait_finish_timer == NULL)
	{
		wait_finish_timer = timer_create(ms,
										 0,
										 TMR_TRIGGER,
										 wait_finish_timerCB,
										 NULL,
										 "wait_finish_timer");
	}
	else
	{
		timer_modify(wait_finish_timer,
					 ms,
					 0,
					 TMR_TRIGGER,
					 wait_finish_timerCB,
					 NULL,
					 "wait_finish_timer",
					 TRUE);
	}
	timer_start(wait_finish_timer);
}

//返回值：识别完成状态
//value：当前次，对比的结果
//Macaddr：当前次的CCO地址
void AddResult_after_DelFeature(U8 value,U8* Macaddr)
{
	U8	i=0;
    U8  null_addr[6] = {0};
    if(memcmp(null_addr,Macaddr,6) == 0)
    {
        app_printf("Macaddr is err =");
		dump_buf(Macaddr,6);
        return;
    }
    
	for(i=0;i<COMMPAER_NUM;i++)
	{
		if(memcmp(Final_result_t.PublicResult_t[i].Macaddr,Macaddr,6)==0 && 
					Final_result_t.PublicResult_t[i].FinishFlag == UNFINISH )
		{
			Final_result_t.PublicResult_t[i].CalculateNum++;
			Final_result_t.PublicResult_t[i].SameRatio += value;
			//本次接收到的CCO网络对比结束时，判断是否有别的网络正在进行。
			//如果有等待别的网络完成识别，没有别的网络时，直接完成
			if(Final_result_t.PublicResult_t[i].CalculateNum >= Final_result_t.PublicResult_t[i].CalculateMaxNum)
			{
			        U16  temp = 1;
                    if(Final_result_t.PublicResult_t[i].CalculateMaxNum != MAX_CACULATE-5)
                    {
 					    printf_s("i= %d,MaxNum=%d\n",i,Final_result_t.PublicResult_t[i].CalculateMaxNum);
                        temp = MAX_CACULATE-5;
                    }
                    else
                    {
                        temp = Final_result_t.PublicResult_t[i].CalculateMaxNum;
                    }
					Final_result_t.PublicResult_t[i].SameRatio=
						Final_result_t.PublicResult_t[i].SameRatio/temp;
					Final_result_t.PublicResult_t[i].FinishFlag = FINISH;
					Final_result_t.FinishFlag=existNofinishedNet(Macaddr);
					app_printf("SameRatio is =%d,",Final_result_t.PublicResult_t[i].SameRatio );
					app_printf("macaddr=");
					dump_buf(Final_result_t.PublicResult_t[i].Macaddr,6);
					if(Final_result_t.FinishFlag== UNFINISH) //存在未完成的识别网络时，启动刷新定时器，定时器结束后完成识别
					{
						modify_wait_finish_timer(60*60*TIMER_UNITS);
						app_printf("have unfinish net,wait onehour!\n");
					}
					else
					{
						app_printf("Final_result_t.FinishFlag chage to OK\n");
					}
			}
			else
			{

				app_printf("INDENTIFICATION time is =%d,",Final_result_t.PublicResult_t[i].CalculateNum );
				app_printf("macaddr=");
				dump_buf(Final_result_t.PublicResult_t[i].Macaddr,6);
				if(zc_timer_query(wait_finish_timer) == TMR_RUNNING)
				{
					app_printf("area defer onehour!\n");
					modify_wait_finish_timer(60*60*TIMER_UNITS);
				}
			}

			break;

		}
		else
		{;}
	}
	return ;
	
}


void CleanDistinguishResult(U8* pCCOMAC)
{
	U8 i=0;
	if(memcmp(Final_result_t.CCOMacaddr,pCCOMAC,6))//加入新的网络时，清识别状态
	{
		memset((U8*)&Final_result_t,0,sizeof(Final_result_t));
		__memcpy(Final_result_t.CCOMacaddr,pCCOMAC,6);
		for(i=0;i<COMMPAER_NUM;i++)
		{
			Final_result_t.PublicResult_t[i].CalculateMaxNum = MAX_CACULATE-5;
		}
		app_printf("CleanDistinguishResult,because CCO change!!!!\n");
	}
	
}
U8	JudgeFinalResult(U8* Macaddr)
{
	//判断原则：相似度最高的网络为归属网络，当归属网络为当前入网的网络时，结果为1，否则结果为2.
	U8 ratio=0;
	U8 i=0;
	U8 ccomac[6];
	for(i=0;i<COMMPAER_NUM;i++)
	{
		if(Final_result_t.PublicResult_t[i].FinishFlag == FINISH)
		{
			if(Final_result_t.PublicResult_t[i].SameRatio >ratio)
			{
				__memcpy(Macaddr,Final_result_t.PublicResult_t[i].Macaddr,6);
				ratio = Final_result_t.PublicResult_t[i].SameRatio;
				//app_printf("ratio=%d,i=%d",ratio,i);
				//dump_buf(Macaddr,6);
			}
		}
	}
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);
	if(memcmp(Macaddr,ccomac, 6) == 0)
	{
		return 1;
	}
	else 
	{
		return 2;
	}
}
/*************************************************************************
 * 函数名称 :	DealFeatureInfoInforming
 * 函数说明 :	特征信息告知
 * 参数说明 :	INDENTIFICATION_HEADER_t *pIndentificationHeader // 台区识别header
 * 返回值   :   void
 *************************************************************************/
void DealFeatureInfoInforming(INDENTIFICATION_IND_t *pIndentificationInd)
{
	U8 result = 0;
	U8 value = 0;
    
	//app_printf("PacketSeq : %d\n",pIndentificationHeader->PacketSeq);
	app_printf("Macaddr : ");
	dump_buf(pIndentificationInd->SrcMacAddr,6);
	/*app_printf("Featuretypes : %d\n",pIndentificationHeader->Featuretypes);
	app_printf("Collectiontype : %d\n",pIndentificationHeader->Collectiontype);*/

	FEATURE_INFO_INFORMING_DATA_t *informing_data_t = (FEATURE_INFO_INFORMING_DATA_t *)pIndentificationInd->payload;	
	/*app_printf("informing_data_t -> ");
	dump_buf((U8 *)informing_data_t,sizeof(FEATURE_INFO_INFORMING_DATA_t));*/
	
	FEATURE_INFO_SEQ_t *feature_info_seq_t = (FEATURE_INFO_SEQ_t *)((U8 *)informing_data_t+sizeof(FEATURE_INFO_INFORMING_DATA_t));

    /*#if defined(STATIC_MASTER)
	if(informing_data_t->CollectMode == e_COLLECT_NEGEDGE)//记录沿信息
	{
		SaveEdgeinfo(informing_data_t->TEI,0);
	}
	else if(informing_data_t->CollectMode == e_COLLECT_POSEDGE)
	{
		SaveEdgeinfo(informing_data_t->TEI,1);
	}
	#endif
	*/
	if(pIndentificationInd->Featuretypes == e_PERIOD && informing_data_t->InformingNum != 0)
	{
		if(TRUE== 
			deal_period_pma(feature_info_seq_t,informing_data_t->CollectMode,informing_data_t->CollectSeq,
			informing_data_t->TEI,&result,&value,informing_data_t->InformingNum, pIndentificationInd->SrcMacAddr)
			)
		{
			
		}
		else//参数错误
		{
			
		}

        #if defined(STATIC_MASTER)
			cco_area_ind_value_info_t[informing_data_t->TEI-2].feature |= e_START_COLLECT_PERIOD;
			cco_area_ind_value_info_t[informing_data_t->TEI-2].value[cco_area_ind_round] = value;
		#endif
	}
	else if(pIndentificationInd->Featuretypes != e_PERIOD && informing_data_t->InformingNum == 1)
	{
		deal_voltage_or_frequncy_pma(feature_info_seq_t,pIndentificationInd->Featuretypes);
	}
	
    #if defined(STATIC_MASTER)

	/*if(TMR_STOPPED == query_get_line_state_timer())
	{
		//cco_area_ind_report_result(informing_data_t->TEI,result,FlashInfo_t.ucJZQMacAddr);
	}	
	if(TRUE == set_collect_node_retry_cnt_0(pIndentificationHeader->Featuretypes,pIndentificationHeader->Macaddr))
	{
		timer_stop(cco_area_ind_ctrl_timer,TMR_CALLBACK);
	}*/
    #elif defined(STATIC_NODE)
	//异常数据
	if( value==0 )
	{
		
		app_printf("invaild info data!\n");
		return;
	}
	//识别完成
	if(  Final_result_t.FinishFlag == FINISH)
	{
		app_printf("INDENTIFICATION FINISHED !\n");
		return;
	}
	else//
	{
		AddResult_after_DelFeature(value, pIndentificationInd->SrcMacAddr);

	}

	
	#endif
}



/*************************************************************************
 * 函数名称 :	DealDistinguishResultQuery
 * 函数说明 :	台区判别结果查询
 * 参数说明 :	INDENTIFICATION_HEADER_t *pIndentificationHeader // 台区识别header
 * 返回值   :   void
 *************************************************************************/
void DealDistinguishResultQuery(INDENTIFICATION_IND_t *pIndentificationInd)
{
#if defined(STATIC_NODE)

    INDENTIFICATION_RESULT_DATA_t *IndentificationResultData_t = 
			             &sta_area_ind_dis_result_t[pIndentificationInd->Featuretypes-1];
	

    IndentificationResultData_t->TEI = APP_GETTEI();
    IndentificationResultData_t->finishflag = Final_result_t.FinishFlag ;
		
    if(Final_result_t.FinishFlag == UNFINISH)
    {
        IndentificationResultData_t->result = 0;
        memset(IndentificationResultData_t->CCOMacaddr,0,6);
    }
    else
    {
        IndentificationResultData_t->result = JudgeFinalResult(Final_result_t.CCOMacaddr);
		__memcpy(IndentificationResultData_t->CCOMacaddr,Final_result_t.CCOMacaddr,6);
    }
    app_printf("IndentificationResultData_t -> ");
    dump_buf((U8 *)IndentificationResultData_t,sizeof(INDENTIFICATION_RESULT_DATA_t));
		
    DistinguishResultInfo(IndentificationResultData_t,//INDENTIFICATION_RESULT_DATA_t IndentificationResultData_t,
                          e_UPLINK,//U8   Directbit,
                          APS_SLAVE_PRM,//U8   Startbit,
                          DevicePib_t.DevMacAddr,
                          pIndentificationInd->SrcMacAddr,//U8	 *DesMacaddr, //接收方 地址             
                          e_FIRST_PHASE,//U8   Phase,    
                          pIndentificationInd->Featuretypes);//U8   Featuretypes)
	#endif
}


/**************************************以上为台区识别流程控制定时器和状态机************************************/


/**************************************以下为台区识别采集定时器************************************/

void show_bcd_to_char(U8 *buf,U16 len)
{
	U16 i =0;
	U16 word=0;
	U8 *p = zc_malloc_mem(1024,"show_bcd_to_char",MEM_TIME_OUT);
	if(NULL == p)
	{
		return;
	}

	__memcpy(p,buf,len);

	app_printf("data  -> ");
	dump_buf(p,len);
	
	for(i=0;i<len;i++)
	{
		*(p+i) = BCDtoINT(*(p+i));
	}

	for(i=0;i<len;i+=2)
	{
		word=*(p+i)+100**(p+i+1);
		__memcpy(p+i,(U8 *)&word,2);
	}
	dump_S16_buf("show_bcd_to_char", DEBUG_APP, (S16 *)p,len/2);
	
	zc_free_mem(p);
	
}

//电压采集定时器
ostimer_t *voltage_info_collect_timer = NULL;
FEATURE_INFO_t Voltage_info_t[MAX_VOLTAGE];
void voltageinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
{
	U8 i = 0;
	for(i=0;i<MAX_VOLTAGE;i++)//打印结果
	{
		if(Voltage_info_t[i].timer_cnt >= Voltage_info_t[i].total_num)
		{
			show_bcd_to_char((U8 *)Voltage_info_t[i].collect_info,2*Voltage_info_t[i].timer_cnt);
			Voltage_info_t[i].state = e_INFO_COLLECT_END;
            #if defined(STATIC_MASTER)//cco需要暂停定时器，开始收集
				timer_stop(voltage_info_collect_timer,TMR_NULL);
				//feature_collecting(voltage_bit,TRUE);
				cco_area_ind_featrue_collecting(voltage_bit,TRUE);
			#endif	
		}
		else
		{
			Voltage_info_t[i].timer_cnt++;
		}
	}
	if(Voltage_info_t[0].state == e_INFO_COLLECT_END)
	{
		return;//若采集满，则返回不再采集
	}
	//发送采集命令
	/*INDENTIFICATION_645_INDICATION_t *pIndentification645Indication_t
		=zc_malloc_pmt("voltageinfocollecttimerCB",sizeof(INDENTIFICATION_645_INDICATION_t));

	pIndentification645Indication_t->collect_feature = e_VOLTAGE;
	pIndentification645Indication_t->mdb.pMallocHeader = NULL;
	
    SendPoolMsg(e_APS_MSG, e_APP_MSG, e_PLC_2016, e_INDENTIFICATION_INDICATION, (U8 *)pIndentification645Indication_t);
	*/
	work_t *work=zc_malloc_mem(sizeof(work_t), "Indentification_voltage_ind", MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}

	INDENTIFICATION_FEATURE_IND_t  *pIndentification_feature_ind_t=(INDENTIFICATION_FEATURE_IND_t *)work->buf;
	pIndentification_feature_ind_t->feature = e_VOLTAGE;
	work->work = indentification_feature_ind;
    work->msgtype = VTCOL;
	post_app_work(work);

}

S8 voltage_info_collect_timer_init(void)
{
	if(voltage_info_collect_timer == NULL)
	{
        voltage_info_collect_timer = timer_create(1000,
                                1000,
	                            TMR_TRIGGER ,//TMR_TRIGGER
	                            NULL,
	                            NULL,
	                            "voltageinfocollecttimerCB"
	                           );
	}
	return TRUE;
}
void modify_voltage_info_collect_timer(uint32_t first,uint32_t interval)
{
	if(NULL != voltage_info_collect_timer)
	{
		timer_modify(voltage_info_collect_timer,
				first,
				interval,
				TMR_PERIODIC ,//TMR_TRIGGER
				voltageinfocollecttimerCB,
				NULL,
                "voltageinfocollecttimerCB",
                TRUE);
	}
	else
	{
		sys_panic("voltage_info_collect_timer is null\n");
	}
	timer_start(voltage_info_collect_timer);
}
//频率采集定时器
ostimer_t *freq_info_collect_timer = NULL;
FEATURE_INFO_t Frequncy_info_t[MAX_FREQUNCY];
void freqinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
{
	U8 i = 0;
	for(i=0;i<MAX_FREQUNCY;i++)//打印结果
	{
		if(Frequncy_info_t[i].timer_cnt >= Frequncy_info_t[i].total_num)
		{
			show_bcd_to_char((U8 *)Frequncy_info_t[i].collect_info,2*Frequncy_info_t[i].timer_cnt);
			Frequncy_info_t[i].state = e_INFO_COLLECT_END;

            #if defined(STATIC_MASTER)
				timer_stop(freq_info_collect_timer,TMR_NULL);
				//feature_collecting(frequncy_bit,TRUE);
				cco_area_ind_featrue_collecting(frequncy_bit,TRUE);
			#endif
		}
		else
		{
			Frequncy_info_t[i].timer_cnt++;

		}
	}

	if(Frequncy_info_t[0].state == e_INFO_COLLECT_END)
	{
		return;
	}
	
	/*
	INDENTIFICATION_645_INDICATION_t *pIndentification645Indication_t
		=zc_malloc_pmt("freqinfocollecttimerCB",sizeof(INDENTIFICATION_645_INDICATION_t));

	pIndentification645Indication_t->collect_feature = e_FREQUNCY;
	pIndentification645Indication_t->mdb.pMallocHeader = NULL;
	
    SendPoolMsg(e_APS_MSG, e_APP_MSG, e_PLC_2016, e_INDENTIFICATION_INDICATION, (U8 *)pIndentification645Indication_t);
	*/
	work_t *work=zc_malloc_mem(sizeof(work_t)+sizeof(INDENTIFICATION_FEATURE_IND_t), "Indentification_frequncy_ind", MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}

	INDENTIFICATION_FEATURE_IND_t  *pIndentification_feature_ind_t=(INDENTIFICATION_FEATURE_IND_t *)work->buf;
	pIndentification_feature_ind_t->feature = e_FREQUNCY;
	work->work = indentification_feature_ind;
    work->msgtype = FRTCOL;
	post_app_work(work);
}

S8 freq_info_collect_timer_init(void)
{
	if(freq_info_collect_timer == NULL)
	{
        freq_info_collect_timer = timer_create(1000,
                                1000,
	                            TMR_TRIGGER ,//TMR_TRIGGER
	                            NULL,
	                            NULL,
	                            "freqinfocollecttimerCB"
	                           );
	}
	return TRUE;
}
void modify_freq_info_collect_timer(uint32_t first,uint32_t interval)
{
	if(NULL != freq_info_collect_timer)
	{
		timer_modify(freq_info_collect_timer,
				first,
				interval,
				TMR_PERIODIC ,//TMR_TRIGGER
				freqinfocollecttimerCB,
				NULL,
                "freqinfocollecttimerCB",
                TRUE);
	}
	else
	{
		sys_panic("modify_freq_info_collect_timer is null\n");
	}
	timer_start(freq_info_collect_timer);
}

//计算过零差值平均值
S16 cal_zero_mean(S16 *buf, U16 num)
{
	if(NULL == buf || 0 == num)
		return 0;
	
	U16 i;
	S32 sum = 0;
	S16 avg = 0;
	for(i = 0; i < num; i++)
	{
		sum += buf[i];
	}
	avg = sum/num;
	return avg;
}

//计算数据中和平均值的最大偏差
U16 cal_zero_max_diff(S16 *buf, U16 num, S16 avg)
{
	if(NULL == buf)
		return 0;
	
	U16 i;
	U16 max_diff = 0;
	for(i = 0; i < num; i++)
	{
		if(abs(avg - buf[i]) > max_diff)
		{
			max_diff = abs(avg - buf[i]);
		}
	}

	return max_diff;
}

//周期采集定时器
void dump_S16_buf(char *s,U32 printf_code,S16 *buf , U16 num)
{
	U16 i = 0;
	/*debug_printf(&dty, printf_code,"%s ->\n",s);
	for(i=0;i<num;i++)
		{
			if (i && (i % 20) == 0)
            	debug_printf(&dty, printf_code,"\n");
			
			debug_printf(&dty, printf_code,"|%-4d",buf[i]);
		}	
		debug_printf(&dty, printf_code,"\n");*/
	app_printf("%s->\n",s);
	for(i=0;i<num;i++)
	{
		if (i && (i % 20) == 0)
            	app_printf("\n");
			
			app_printf("|%-4d",buf[i]);
	}
	app_printf("\n");
}


ostimer_t *period_info_collect_timer = NULL;	//工频周期控制定时器
FEATURE_INFO_t Period_info_t[MAX_PERIOD];		//工频周期缓存buf

//回调函数被调用时，工频周期数据采集完成，收集这些数据放在FEATURE_INFO_t中

/*************************************************************************
 * 函数名称 :	void periodinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
 * 函数说明 :	
 *      本函数是工频周期信息采集定时器回调函数，在此回调函数中，CCO或者STA依照起始NTB值，
 *   从保存过零点NTB值的数据缓冲区，取指定数量的过零NTB数据到工频周期特征信息缓冲区
 *
 * 参数说明 :	
 *     struct ostimer_s *ostimer  回调的定时器指针
 *     void *arg   定时器回调函数参数
 * 返回值   :   
 *     无
 *************************************************************************/
void periodinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
{
	S8 result=0;
	
	U16 j=0,effective_num=0;
	U32 startntb = 0;
	
	FEATURE_INFO_t *info_t = &Period_info_t[0];		//工频周期信息存储区，
	ZERODATA_t *zero_data_t = &ZeroData[0];			//CCO过滤存储和STA过零存储
    
#if defined(STATIC_MASTER)
	U32 tp_start_ntb;
	tp_start_ntb = info_t->startntb;
	S32 sum = 0;
	U8 data_err = FALSE;
	U16 max_diff = 0;

	for(j=0 ;j < MAX_PERIOD ; j++)
	{
		sum = 0;
		//app_printf("Period_info_t [%d] -> ",j);
		//dump_buf((U8 *)&(Period_info_t[j].effective_cnt),sizeof(FEATURE_INFO_t)-2*FEATURE_CNT);
		effective_num = info_t->total_num;
		result = area_ind_get_ntb_data_by_start_ntb((S16*)info_t->collect_info,
											&effective_num,
											tp_start_ntb,
											zero_data_t,
											MAXNTBCOUNT,
											&startntb,
											0);
		if(info_t->total_num != effective_num)
		{
			//effective_num = 0;
		}
		//实现下降沿起始NTB为Period_info_t[0].startntb
		//A相下降沿使用Period_info_t[0].startntb查找
		//B相下降沿使用A相时间为基准
		//C相下降沿使用A相时间为基准
		//A相上升沿使用Period_info_t[0].startntb查找
		//B相上升沿使用A上升相时间为基准
		//C相上升沿使用A上升相时间为基准

		info_t->startntb = startntb;
		
		tp_start_ntb = j==2? Period_info_t[0].startntb:startntb;
		info_t->timer_cnt = info_t->effective_cnt = effective_num;
		
		app_printf("phase %d result : %d effective num : %d，startntb=%08x\n",j,result,effective_num,startntb);
		dump_S16_buf("period data ",DEBUG_APP,(S16*)info_t->collect_info,info_t->timer_cnt);

		sum = cal_zero_mean((S16*)info_t->collect_info,info_t->timer_cnt);
		max_diff = cal_zero_max_diff((S16*)info_t->collect_info,info_t->timer_cnt,sum);
		if(max_diff > NTB_OFFSET)
		{
			data_err = TRUE;
		}
		info_t += 1;
		zero_data_t += 1;
	}

	if(TRUE == data_err)
		cco_area_ind_featrue_collecting(period_bit, FALSE);	
	else
    	cco_area_ind_featrue_collecting(period_bit, cco_area_ind_check_zero());	
#else

	for(j=0 ;j<MAX_PERIOD ; j++)
	{
		//app_printf("Period_info_t [%d] -> ",j);
		//dump_buf((U8 *)&(Period_info_t[j].effective_cnt),sizeof(FEATURE_INFO_t)-2*FEATURE_CNT);
		effective_num = info_t->total_num;
		result = area_ind_get_ntb_data_by_start_ntb((S16*)info_t->collect_info,
											&effective_num,
											info_t->startntb,
											zero_data_t,
											MAXNTBCOUNT,
											&startntb,
											0);
		
		if(info_t->total_num != effective_num)
		{
			effective_num = 0;
		}
		
		info_t->startntb = startntb;

		info_t->timer_cnt = info_t->effective_cnt = effective_num;
		
		app_printf("phase %d result : %d effective num : %d\n",j,result,effective_num);
		dump_S16_buf("period data ",DEBUG_APP,(S16*)info_t->collect_info,info_t->timer_cnt);
		info_t += 1;
		zero_data_t += 1;
	}
#endif
}


/*************************************************************************
 * 函数名称 :	S8 period_info_collect_timer_init(void)
 * 函数说明 :	
 *      本函数为工频周期特征信息采集启动定时器的初始化函数
 * 参数说明 :	
 *     无
 * 返回值   :   
 *     S8  正常则返回TRUE
 *************************************************************************/
S8 period_info_collect_timer_init(void)
{
	if(period_info_collect_timer == NULL)
	{
        period_info_collect_timer = timer_create(1*TIMER_UNITS,
                                1*TIMER_UNITS,
	                            TMR_TRIGGER,//TMR_TRIGGER
	                            NULL,
	                            NULL,
	                            "periodinfocollecttimerCB"
	                           );
	}
	return TRUE;
}


/*************************************************************************
 * 函数名称 :	void modify_period_info_collect_timer(uint32_t first)
 * 函数说明 :	
 *      本函数用于修改工频周期特征信息采集驱动定时器的定时时间，定时器为触发模式
 * 参数说明 :	
 *     无
 * 返回值   :   
 *     无
 *************************************************************************/
void modify_period_info_collect_timer(uint32_t first)
{
	if(NULL != period_info_collect_timer)
	{
		timer_modify(period_info_collect_timer,
				first,
				100,
				TMR_TRIGGER ,//TMR_TRIGGER
				periodinfocollecttimerCB,
				NULL,
                "periodinfocollecttimerCB",
                TRUE);
	}
	else
	{
		sys_panic("modify_period_info_collect_timer is null\n");
	}
//	PHY_DEMO.noise_detect = 1 ;
	timer_start(period_info_collect_timer);
}

/**************************************以上为台区识别采集定时器************************************/

int sqr(int n)
{
    uint32_t i;
    for (i = 1; i*i <= n; i++)
    {
		;
	}
    return i - 1;
}



float calculateSD(U32 *data, U16 datalen,U32 *arg_u) 
{
    float sum = 0.0, mean,SD = 0.0;
	U32 tp_temp;
    int i;
    for (i = 0; i < datalen; ++i) {
        sum += data[i];
    }
    mean = sum / datalen;
	*arg_u = mean;
    for (i = 0; i < datalen; ++i)
    {
    	//printf_s("data[%d] : %d\n",i,data[i]);
    	if(data[i] > mean)
			tp_temp = data[i] - mean;
		else
			tp_temp = mean - data[i];
        SD += (tp_temp * tp_temp);
	}
    return sqr(SD / datalen);
}


/***ntb 取值***/
/**************************************************************************
 * 函数名称 :	area_ind_get_ntb_data_by_start_ntb
 * 函数说明 :	
 *      查找过零点NTB值，找到起始NTB值，然后按采集数量采集一定的过零NTB值到全局的Period_info_t结构中
 * 参数说明 :	
 *    S16 *periodbuf   Period_info_t中用于存放工频周期过零点数据的collect_info数组，U16类型，最大500个
 *    U16 *num     指针变量，用于存放有效的采集周期数量值返回
 *    U32 startntb 采集的起始NTB值
 *    void *data   保存过零NTB值的数据Buff
 *    U16 maxnum   采集最大数量,CCO宏定义为300
 *    U32 *start_collect_ntb  此指针用于存储实际的起始过零点NTB的值
 *    U8 collecttype  是1，则表示不是使用启动命令中的启动时间采集，0则相反
 * 返回值   :   
 *    S8   返回结果码，为0则正常采集正常，其它负整数为采集异常
 *************************************************************************/
S8 area_ind_get_ntb_data_by_start_ntb(S16 *periodbuf, U16 *num, U32 startntb, void *data, U16 maxnum,U32 *start_collect_ntb,U8 collecttype)
{
    U16 i=0;
    U16 get_pos=0xffff;
    U16 totalnum = 0;
    S8 result = 0;
    U16 bufindex = 0;
	U8 static send_time = 0;
    
    for(i = 0; i<MAXNTBCOUNT;i++)
	{
		gl_area_ind_ntb_buf[i] = 0;
	}
    ZERODATA_t *zerodata_t = (ZERODATA_t *)data;
		
	*num += 1;//取两个过零NTB差值，所以要多取一个值才能达到要求

	//app_printf("zerodata_t addr  : 0x%08x offset : %d \n",data,zerodata_t->NewOffset);
	
	if(startntb < zerodata_t->NTB[0] && startntb > zerodata_t->NTB[maxnum-1])//判断要找的点是否在尾和头之间
	{
		get_pos = 0;
		if(collecttype)// 1，表示不是使用启动命令中的启动时间采集
		{
			get_pos = abs(startntb - zerodata_t->NTB[0]) < abs(startntb - zerodata_t->NTB[maxnum-1]) ? 0:maxnum-1;//取靠近的一侧
			if(get_pos == maxnum-1)
			{
				*num = *num-1;
				__memcpy(&gl_area_ind_ntb_buf[bufindex],&zerodata_t->NTB[maxnum-1],sizeof(U32));		
				bufindex++;
			}
		}
		
		totalnum = zerodata_t->NewOffset + 1;
		
		if(totalnum < *num)
		{
			*num = totalnum;
			result = -2;
		}
		__memcpy(&gl_area_ind_ntb_buf[bufindex],zerodata_t->NTB+get_pos,*num*sizeof(U32));		

		goto getntb;
	}	

	for(i=0;i<=maxnum-1;i++)//判断从头到尾之间的数据
	{
		if(startntb > zerodata_t->NTB[i] && startntb < zerodata_t->NTB[i+1])
		{
			get_pos = i+1;
			if(collecttype)//1，表示不是使用启动命令中的启动时间采集
			{
				get_pos= abs(startntb - zerodata_t->NTB[i]) < abs(startntb - zerodata_t->NTB[i+1]) ? i:i+1;//取靠近的一侧
			}
			//app_printf("\nget_pos : %d ,start_ntb : 0x%08x zerodata_t->NewOffset : %d num : %d,0x%08x,0x%08x\n",get_pos,
			//	startntb,zerodata_t->NewOffset,*num,zerodata_t->NTB[i],zerodata_t->NTB[i+1]);
			
			if(get_pos <= zerodata_t->NewOffset)//需要取的数据在当前最新数据位置前
			{
				totalnum = zerodata_t->NewOffset - get_pos + 1;
				if(totalnum < *num)
				{
					*num = totalnum;
					result = -2;
				}
				//app_printf("totalnum : %d ,num : %d\n",totalnum,*num);
				__memcpy(&gl_area_ind_ntb_buf[bufindex],zerodata_t->NTB+get_pos,*num*sizeof(U32));
				goto getntb;
			}
			else//需要取的数据在当前最新数据位置后
			{
				totalnum = maxnum - get_pos + zerodata_t->NewOffset +1 ;
				
				if(maxnum - get_pos >= *num)
				{
					result = 0;
					__memcpy(&gl_area_ind_ntb_buf[bufindex],zerodata_t->NTB+get_pos,*num*sizeof(U32));
					goto getntb;

				}
				//先取后面的值
				result = 0;	
				__memcpy(&gl_area_ind_ntb_buf[bufindex],zerodata_t->NTB+get_pos,(maxnum - get_pos)*sizeof(U32));
				if(totalnum < *num)//如果数据不够
				{
					*num = totalnum;
					result = -2;	
				}
				//再取前面的值
				__memcpy(&gl_area_ind_ntb_buf[bufindex+maxnum] - get_pos,zerodata_t->NTB,(*num - (maxnum - get_pos))*sizeof(U32));

				goto getntb;
			}
		}
		else
		{
			//下一个循环再找
		}

	}

	result = -1;//未找到数据
	*num = 1;
	
	getntb :
		
	send_time ++;

#if 0
	for(i=0;i<*num-1;i++)
	{
		*(periodbuf+i) = (S16)(gl_area_ind_ntb_buf[i+1] - gl_area_ind_ntb_buf[i] - 20*25000)/8;
	}
#else
	//FALSE表示台体,TRUE正常工作模式或者发送次数大于10次以后恢复正常过零
	if(HPLC.scanFlag == TRUE || send_time > 10)
	{
		app_printf("NTB normal\n");
		for(i=0;i<(*num-1);i++)
		{
			*(periodbuf+i) = (S16)(gl_area_ind_ntb_buf[i+1] - gl_area_ind_ntb_buf[i] - 20*25000)/8;
		}
	}
	else
	{
		app_printf("NTB optimize\n");
		float tp_cal_sd;
		U32 tp_diff = 0, tp_diff_single;
		
		for(i=0;i<*num-1;i++)
		{
			gl_diff[i] = (gl_area_ind_ntb_buf[i+1] - gl_area_ind_ntb_buf[i]);
		}
		
		tp_cal_sd = calculateSD(gl_diff,(*num - 1),&tp_diff);
		printf_s("tp_cal_sd : %lf\n",tp_cal_sd);
		
		for(i = 0; i < *num - 1; i++)
		{
			tp_diff_single = gl_area_ind_ntb_buf[i+1] - gl_area_ind_ntb_buf[i];
			
			if(tp_diff_single > tp_diff)
			{
				tp_diff_single = tp_diff + ((tp_diff_single - tp_diff)/2);
			}
			else
			{
				tp_diff_single = tp_diff - ((tp_diff - tp_diff_single)/2);
			}
			gl_diff[i] = tp_diff_single;
			*(periodbuf+i) = (S16)(tp_diff_single - 20*25000)/8;
		}
		tp_cal_sd = calculateSD(gl_diff,(*num - 1),&tp_diff);
		printf_s("tp_cal_sd : %lf\n",tp_cal_sd);
	}
	
#endif

	*num -= 1;

	if(start_collect_ntb)
	{
		//app_printf("start_collect_ntb 0x%08x\n",start_collect_ntb);
		*start_collect_ntb = gl_area_ind_ntb_buf[bufindex];
	}

	return result;
}

/*********************************************以下台区识别算法*******************************************/

/**************************************period***********************************
 * 函数名称 :	cal_S16_variance
 * 函数说明 :	台区识别算法处理
 * 参数说明 :	S16 *p// 一组数据，cco和sta两组数据的方差
 				U16 cnt//数组数量
 * 返回值   :   float
 *************************************************************************/
float cal_S16_variance(S16 *p,U16 cnt)
{
	U32 i;
	float variance = 0;

	for(i=0;i<cnt;i++)
	{
		variance += (*(p+i) * *(p+i));
	}
	return variance/cnt;
}
/***************************************period**********************************
 * 函数名称 :	get_Dvalue_by_2buf
 * 函数说明 :	台区识别算法处理
 * 参数说明 :	S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2 ,S16 *getbuf , U16 *getnum
 * 返回值   :   S8
 *************************************************************************/
S8 get_Dvalue_by_2buf(S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2 ,S16 *getbuf , U16 *getnum)
{
	U16 i = 0;
	if(num1 != num2)
		return FALSE;

	for(i = 0;i < num1;i++)
	{
		getbuf[i] = buf1[i] - buf2[i];
	}
	*getnum = num1;
	return TRUE;
}
/************************************period*************************************
 * 函数名称 :	get_result_by_variance
 * 函数说明 :	台区识别算法处理
 * 参数说明 :	S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2 
 * 返回值   :   void
 *************************************************************************/
S8 get_result_by_variance(S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2,U8 *result)
{
	S16    *getbuf = NULL;
	U16     getnum = 0;
	float   value;

	S16 sum1 = 0;		//计算数据平均偏差
	S16 sum2 = 0;

	U16 max_diff1 = 0;	//最大偏差
	U16 max_diff2 = 0;	//最大偏差
	
	getbuf = zc_malloc_mem(1024,"get_result_by_variance",MEM_TIME_OUT);

	if(NULL == getbuf)
	{
		return FALSE;
	}

	sum1 = cal_zero_mean(buf1,num1);
	sum2 = cal_zero_mean(buf2,num1);

	max_diff1 = cal_zero_max_diff(buf1,num1,sum1);
	max_diff2 = cal_zero_max_diff(buf2,num1,sum2);

	if(max_diff1 > NTB_OFFSET || max_diff2 > NTB_OFFSET)
	{
		zc_free_mem(getbuf);
		return FALSE;
	}

	*result =CalculateNTBcurve(buf1,buf2,num1);
	get_Dvalue_by_2buf(buf1,num1,buf2,num2,getbuf,&getnum);

	//dump_S16_buf("~~~>get_result_by_variance",DEBUG_APP,getbuf,getnum);
	
	value =cal_S16_variance(getbuf,getnum);
//	printf_s("value : %.4f,result=%d\n",value,*result);
	if(value<15)
	{
		//*result = 100;
		
	}
	
	
	zc_free_mem(getbuf);
	
	return *result >= 60?TRUE:FALSE;
}

/************************************frequncy************************************
 * 函数名称 :	get_loss_matrix
 * 函数说明 :	台区识别算法处理，损失矩阵计算
 * 参数说明 :	U16 *crow_buf , U16 crow , U16 col_buf, U16 col
 * 返回值   :   void
 *************************************************************************/
/*S16 get_loss_matrix(S16 *crow_buf , U16 crow , S16 *col_buf, U16 col)
{
	S16 **a;
    U16 i, j;//,ii,jj;
    a = (S16**)__malloc(sizeof(S16*)*crow);//为二维数组分配3行
    for (i = 0; i < crow; ++i){//为每列分配4个大小空间
        a[i] = (S16*)__malloc(sizeof(S16)*col);
    }
    //初始化
    for (i = 0; i < crow; ++i){
        for (j = 0; j < col; ++j){
            a[i][j] = 0xffff;
        }
    }
	
	//欧式距离矩阵
	/for (i = 0; i < num; i++)
	{
		for (j = i; j < crow; j++)
		{
	       a[j][i] = abs(col_buf[j]-crow_buf[i]);
	    }
		
		for (j = i; j < col; j++)
		{
	       a[i][j] = abs(col_buf[i]-crow_buf[j]);
	    }
	}/
	//损失矩阵	
	for (i = 0; i < MAX(crow,col); i++)
	{
		for (j = i; j < col; j++)
		{
			a[i][j]=((i==0&&j==0)?0:MIN(MIN((i-1<0||j-1<0)?0xffff:a[i-1][j-1],i-1<0?0xffff:a[i-1][j]),j-1<0?0xffff:a[i][j-1]))+abs(col_buf[j]-crow_buf[i]);
		}
		
		for (j = i+1; j < crow; j++)
		{
			a[j][i]=((i==0&&j==0)?0:MIN(MIN((i-1<0||j-1<0)?0xffff:a[j-1][i-1],j-1<0?0xffff:a[j-1][i]),i-1<0?0xffff:a[j][i-1]))+abs(col_buf[i]-crow_buf[j]);
		}

		/os_sleep(50);
		//输出测试
	    for (ii = 0; ii < crow; ++ii)
		{
	        for (jj = 0; jj < col; ++jj){
	            app_printf("%-4x ", a[ii][jj]);
	        }
			//os_sleep(5);
	        app_printf("\n");
	    }/	
	}
	
    //输出测试
    for (i = 0; i < crow; ++i)
	{
        for (j = 0; j < col; ++j){
            app_printf("%-4x ",a[i][j]);
        }
		os_sleep(5);
        app_printf("\n");
    }
	
    //释放动态开辟的空间
    for (i = 0; i < crow; ++i){
        free(a[i]);
    }
    free(a);
	
	return 	a[crow-1][col-1];
}
*/






void indentification_feature_ind(work_t *work)
{
	INDENTIFICATION_FEATURE_IND_t  *pIndentification_feature_ind_t=(INDENTIFICATION_FEATURE_IND_t *)work->buf;
	

	U8 voltage_645[4]={0x33 , 0x32 , 0x34 , 0x35};
	U8 frequncy_645[4]={0x35 , 0x33 , 0xB3 , 0x35};	
	U8 *ID = NULL;
	
	if(e_VOLTAGE == pIndentification_feature_ind_t->feature)
	{
		ID = voltage_645;
		area_ind_feature_info_t.type = e_VOLTAGE;
		area_ind_feature_info_t.info_t = &Voltage_info_t[0];	
	}
	else if(e_FREQUNCY == pIndentification_feature_ind_t->feature)
	{
		ID = frequncy_645;
		area_ind_feature_info_t.type = e_FREQUNCY;
		area_ind_feature_info_t.info_t = &Frequncy_info_t[0];
	}

    #if defined(STATIC_MASTER)
	U8 type = e_ACSAMPING_TPEY_645;
	U8 phase = 0;		

	app_gw3762_afn14_f4_up_frame(type,ID+2,4,phase,0);
	
    #elif defined(STATIC_NODE)
	uint8_t  frame[50]={0};
	uint8_t  len_645=0;
	uint16_t len_698=0;
	if(DevicePib_t.Prtcl == DLT698)
	{
		sta_area_ind_packet_voltage_freq_698_frame(
						#if defined(ZC3750STA)
						DevicePib_t.DevMacAddr,
						#elif defined(ZC3750CJQ2)
						CollectInfo_st.CollectAddr,
						#endif
						frame,
						&len_698
						);
	}
	else
	{
		Packet645Frame(frame,&len_645,
						#if defined(ZC3750STA)
						DevicePib_t.DevMacAddr,
						#elif defined(ZC3750CJQ2)
						CollectInfo_st.CollectAddr,
						#endif
					    0x11,ID,4);
	}
	
	add_serialcache_msg_t  add_serialcache_msg;
    memset((U8 *)&add_serialcache_msg,0x00,sizeof(add_serialcache_msg_t));
        
	add_serialcache_msg.lid           = 3;
	add_serialcache_msg.ack           = TRUE;
	
	add_serialcache_msg.DeviceTimeout = 30;
	add_serialcache_msg.Protocoltype  = DevicePib_t.Prtcl;

	add_serialcache_msg.ReTransmitCnt = 1;
	add_serialcache_msg.FrameLen      = DevicePib_t.Prtcl==DLT698?len_698:len_645;
	
	add_serialcache_msg.Uartcfg       = NULL;
	add_serialcache_msg.EntryFail     = NULL;
	add_serialcache_msg.SendEvent     = NULL;
	add_serialcache_msg.ReSendEvent   = NULL;
	add_serialcache_msg.MsgErr        = NULL;
	add_serialcache_msg.Match         = sta_area_ind_voltage_or_frequncy_match;
	add_serialcache_msg.Timeout       = NULL;
	add_serialcache_msg.RevDel        = sta_area_ind_voltage_or_frequncy_rcv_deal;
	
	add_serialcache_msg.pSerialheader = &SERIAL_CACHE_LIST;
	extern ostimer_t *serial_cache_timer;
	add_serialcache_msg.CRT_timer     = serial_cache_timer;
	
	serial_cache_add_list(add_serialcache_msg, frame,TRUE);
	#endif
}


void AppIndentificationIndProc(INDENTIFICATION_IND_t *pIndentificationInd)
{
#if defined(STATIC_MASTER)
    if(pIndentificationInd->SNID != APP_GETSNID())
    {
        return;
    }

    if(pIndentificationInd->Collectiontype == e_FEATURE_INFO_INFORMING)       //相位识别特征信息告知
	{
        cco_phase_position_edge_proc(pIndentificationInd);
	}
	else if(pIndentificationInd->Collectiontype == e_DISTINGUISH_RESULT_INFO) //台区识别结果查询
	{
        cco_area_ind_query_result_proc(pIndentificationInd);
	}


#elif defined(STATIC_NODE)

	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);

    app_printf("type  :  %s\n",
	    pIndentificationInd->Collectiontype == e_FEATURE_COLLECT_START?"e_FEATURE_COLLECT_START":
	    pIndentificationInd->Collectiontype == e_FEATURE_INFO_GATHER?"e_FEATURE_INFO_GATHER":
	    pIndentificationInd->Collectiontype == e_FEATURE_INFO_INFORMING?"e_FEATURE_INFO_INFORMING":
	    pIndentificationInd->Collectiontype == e_DISTINGUISH_RESULT_QUERY?"e_DISTINGUISH_RESULT_QUERY":
	    pIndentificationInd->Collectiontype == e_DISTINGUISH_RESULT_INFO?"e_DISTINGUISH_RESULT_INFO":"UNKOWN");
        
	sta_area_ind_recv_packet_seq = pIndentificationInd->DatagramSeq;

    switch (pIndentificationInd->Collectiontype)
	{
		case e_FEATURE_COLLECT_START:
            if(pIndentificationInd->Featuretypes == e_PERIOD)
			{
				if(memcmp(pIndentificationInd->SrcMacAddr, ccomac,6) ==0)//过滤其他网络CCO发送的启动命令
                {            
					DealFeatureCollectStart(pIndentificationInd);
                }
			}
			break;
		case e_FEATURE_INFO_GATHER://集中式
			//__memcpy(pIndentificationInd->SrcMacAddr,ccomac,6);
			sta_area_ind_deal_feature_info_gather(pIndentificationInd);
			break;
		case e_FEATURE_INFO_INFORMING:
			add_ccomacaddr(pIndentificationInd->SNID, pIndentificationInd->SrcMacAddr);
			DealFeatureInfoInforming(pIndentificationInd);
			break;
		case e_DISTINGUISH_RESULT_QUERY:
			
			if(memcmp(pIndentificationInd->DstMacAddr, nnib.MacAddr, 6) ==0) 
            {   
            	//__memcpy(pIndentificationInd->SrcMacAddr,ccomac,6);
			    DealDistinguishResultQuery(pIndentificationInd);
            }
			break;

		default:
			break;
	}
#endif

}






















