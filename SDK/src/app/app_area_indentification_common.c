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
 * �������� :	indentification_msg_request
 * ����˵�� :	
 *    ̨��ʶ����ͳһ��������ӿ�
 * ����˵�� :	
 *    uint8_t *payload   ̨��ʶ����������
 *	  uint16_t len       ̨��ʶ���������򳤶�
 *    uint8_t Directbit   ����λ��0Ϊ���У�1Ϊ����
 *	  uint8_t Startbit    ����λ��0Ϊ���ԴӶ�վ��1Ϊ��������վ
 *    uint8_t Phase       �ɼ���λ��0ΪĬ����λ
 *    uint16_t  seq       ֡��ţ����е����ԣ���������صı��ģ����Ӧ��һ��
 *    uint8_t Featuretypes  �������ͣ�e_VOLTAGE, e_FREQUNCY, e_PERIOD, 4Ϊ�����
 *    uint8_t Collectiontype �ɼ�����,��׼�ж��壬��������̨��ʶ���Ĳ�ͬ����������
 *    uint8_t sendtype   �������ͣ�������㲥��
 *    uint8_t *desaddr   Ŀ��MAC��ַ
 *    uint8_t *srcaddr   ԴMAC��ַ
 * ����ֵ   :
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
 * �������� :	FeatureCollectStart
 * ����˵�� :	�����ռ�����
 * ����˵�� :	FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t,  //�����ռ������غ�
				U8	 *DstMacaddr, //Ŀ��MAC��ַ��������Ϊ�㲥��ַ
				U8   Phase,       //�ɼ���λ
				U8   Featuretypes //��������
 * ����ֵ   :
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
 * �������� :	FeatureInfoGather
 * ����˵�� :	������Ϣ�ռ�
 * ����˵�� :	U8   Directbit,  //����λ
				U8   Startbit,   //����λ
				U8   *Srcaddr,   //���ͷ� ��ַ
				U8	 *DesMacaddr,//���շ� ��ַ
				U8   Phase,      //�ɼ���λ
				U8   Featuretypes//��������
 * ����ֵ   :
 *************************************************************************/
void FeatureInfoGather (	U8      Directbit,
							U8   Startbit,
							U8   *Srcaddr, //���ͷ� ��ַ
							U8	 *DesMacaddr, //���շ� ��ַ
							U16  DateSeq,
							U8   Phase,    
							U8   Featuretypes)
{
	//MDB_t  mdb;
	U8 desaddr[6]; //����ΪͨѶ��ַ
	U8 srcaddr[6]; //����ΪͨѶ��ַ
	
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
 * �������� :	FeatureInfoInforming
 * ����˵�� :	������Ϣ��֪
 * ����˵�� :	U8   *payload,    //�غ�ָ��
				U16  len,         //�غɳ���
				U8   Directbit,   //����λ
				U8   Startbit,    //����λ
				U8   *Srcaddr,    //���ͷ� ��ַ
				U8	 *DesMacaddr, //���շ� ��ַ
				U8   Phase,       //�ɼ���λ
				U8   Featuretypes //��������
 * ����ֵ   :
 *************************************************************************/
void FeatureInfoInforming (	U8   *payload,
							U16  len,
							U8   Directbit,
							U8   Startbit,
							U8   *Srcaddr, //���ͷ� ��ַ
							U8	 *DesMacaddr, //���շ� ��ַ
							U8   Phase,    
							U8   Featuretypes)
{
	//MDB_t  mdb;
	U8 desaddr[6]; //����ΪͨѶ��ַ
	U8 srcaddr[6]; //����ΪͨѶ��ַ
	
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
                        e_FULL_BROADCAST_FREAM_NOACK,//CCO��֪ʹ�ù㲥��STA��֪ʹ�õ���
                        #else
                        e_UNICAST_FREAM,
                        #endif
						desaddr,
						srcaddr);
	return;
}
/*************************************************************************
 * �������� :	DistinguishResultQuery
 * ����˵�� :	̨��ʶ������ѯ
 * ����˵�� :	U8   Directbit,  //����λ
				U8   Startbit,   //����λ
				U8   *Srcaddr,   //���ͷ� ��ַ
				U8	 *DesMacaddr,//���շ� ��ַ
				U8   Phase,      //�ɼ���λ
				U8   Featuretypes//��������
 * ����ֵ   :
 *************************************************************************/
void DistinguishResultQuery (	U8   Directbit,
								U8   Startbit,
								U8   *Srcaddr, //���ͷ� ��ַ
								U8	 *DesMacaddr, //���շ� ��ַ
								U16  DateSeq,
								U8   Phase,    
								U8   Featuretypes)
{
	//MDB_t  mdb;
	//U8 desaddr[6]; //����ΪͨѶ��ַ
	U8 srcaddr[6]; //����ΪͨѶ��ַ

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
// ��������U8 CalculateNTBDiff(U32 ntb ,U32 *staNtb,U8 num,U8 *Phase,U16 *zeroData)
// 
// ����������
//     ���������ڼ���ĳһ���ߵ������Ƿ���ȷ
// ������
//     U32 ntb      CCO��¼�����ϱ���ֵ������Ĺ����NTBֵ
//     U32 *staNtb  STA�ϱ�������ĳһ��λ�Ĺ���NTBֵ
//     U8 numa      ĳ���ߵĸ�֪����
//     U8 *Phase    ָ���������һ��U8�ֽڵ�ַ��ȥ���ڴ����λʶ����
//     U16 *zeroData  ����STA�������CCO�Ĳ�ֵ
// ����ֵ��
//     U8    TRUE or FALSE ��CCOA���½���Ϊ��׼��ʶ��������TRUE���½���FALSE
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
		if(abs(staNtb[i] - ntb) > 0x0FFFFFFF)	//U32��ת
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
		if(staNtb[i] < ntb) //��ģ��ɼ���������CCOǰ 	 
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



//�����������ƶ�
U8	CalculateNTBcurve(S16 *mycurve,S16 *CCOcurve, U16 perNum)
{
	U16 samenum=0; 
	U16	diffnum=0;
	U16	 i;
	U8 pram =1;
	for(i=0;i<perNum;i++)
	{
		if(mycurve[i]>2500	|| CCOcurve[i]>2500)//���ڲ������Ĺ����
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


/**************************************����Ϊ̨��ʶ�������ݽӿ�************************************/

/**************************************����Ϊ̨��ʶ����ս������ݽӿ�************************************/

/*************************************************************************
 * �������� :	void area_ind_set_featrue_and_start_timer(U8 featuretypes,FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t)
 * ����˵�� :	
 *      ���������ݲ�ͬ���������ͣ��������ɼ������������ݣ�������������Ϣ�ɼ�������ʱ��
 * ����˵�� :	
 *     U8 featuretypes   �������ͣ��繤Ƶ����Ϊe_PERIOD
 *     FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t   �����ɼ�������������
 * ����ֵ   :   
 *     void  �޷���ֵ
 *************************************************************************/
void area_ind_set_featrue_and_start_timer(U8 featuretypes, FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t)
{
	U8 i=0 ,num =0;
	U32 nowtime = get_phy_tick_time();										//��ȡ��ǰtick
	U32 time = (feature_collect_start_data_t->Startntb - nowtime)/250000;	//10ms ��λ
	
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

	//��ʼ������buf
	if(Feature_info_t)
	{
		memset(Feature_info_t, 0x00, sizeof(FEATURE_INFO_t));
	}
	
	Feature_info_t->total_num = feature_collect_start_data_t->CollectNum;		//�ɼ�����
	Feature_info_t->cycle = feature_collect_start_data_t->CollectCycle;			//�ɼ��������Ƶ���ڲ�������
	Feature_info_t->startntb = feature_collect_start_data_t->Startntb;			//��ʼNTB
	Feature_info_t->collect_seq = feature_collect_start_data_t->CollectSeq;		//�ɼ����
	
	for(i = 1; i < num; i++)
	{
		__memcpy(Feature_info_t+i,Feature_info_t,sizeof(FEATURE_INFO_t));
	}
	
    #if defined(STATIC_NODE)
	memset((U8 *)&sta_area_ind_dis_result_t[0],0,3*sizeof(INDENTIFICATION_RESULT_DATA_t));
	#endif
}

/*************************************************************************
 * �������� :	DealFeatureCollectStart
 * ����˵�� :	������Ϣ�ռ�����
 * ����˵�� :	INDENTIFICATION_IND_t *pIndentificationInd // ̨��ʶ��header
 * ����ֵ   :   void
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

	
	//STA���յ������ɼ���ʼ
	area_ind_set_featrue_and_start_timer(pIndentificationInd->Featuretypes,feature_collect_start_data_t);
}

void packet_pma(FEATURE_INFO_t *feature_info_t ,U8 *out_phase,U8 *FeatureInfoSeq,U16 *FeatureInfoSeqlen,U16 *infonum)
{
	U8 i = 0;
	U8 phase = 0;

	FEATURE_INFO_t *feature_info_payload_t = feature_info_t;
	*FeatureInfoSeqlen = 0;	
	//����ɼ���ʼNTB
	//__memcpy(FeatureInfoSeq,(U8 *)&feature_info_t->startntb,4);
	(*FeatureInfoSeqlen) += 4;
	//����
	(*FeatureInfoSeqlen) += 1;
	
	
	for(i=0; i<3; i++)
	{
		//��������
		//�������Ч���ݣ���ĳ�����λ����Ϊ�ɼ��ĸ�����������������вɼ�ʧ�ܵ���Ч����
		//�������Ч���ݣ���ĳ�����λ����Ϊ0
		//����ֻ�Ƕ���ζ�Ӧ����λ����������ֵ����������λΪ��������Ϊ0
		*(FeatureInfoSeq+(*FeatureInfoSeqlen)++)=
				(feature_info_t->effective_cnt != 0)?feature_info_t->timer_cnt:0;

		if(0 != feature_info_t->effective_cnt)//�ó�����λ����Ч����,��ó�����λ��λ
		{
			if(phase == 0)//��¼��ǰ���ݳ�����λ
			{
				__memcpy(FeatureInfoSeq,(U8 *)&feature_info_t->startntb,4);
				phase |= (1<<i); 
				*out_phase = i+1;
				app_printf("get ntb phase : %d,startntb=%08x\n",phase,feature_info_t->startntb);
			}
			else//�����һ�����ϵ��������У���Ϊ��Ĭ�ϳ�����λ
			{
				*out_phase = 0;
			}
			*infonum += feature_info_t->effective_cnt;
		}
		
		feature_info_t += 1;//ÿ��ƫ��һ���ṹ��
	}
	
	//edge = (feature == e_PERIOD)?edge:0;
	
	for(i=0;i<3;i++)
	{
		//��ѹ��Ƶ�ʣ�����ֵ
		if(feature_info_payload_t->effective_cnt != 0)
		{
			__memcpy(FeatureInfoSeq+*FeatureInfoSeqlen,(U8 *)&feature_info_payload_t->collect_info[0],2*feature_info_payload_t->timer_cnt);
			(*FeatureInfoSeqlen) += 2*feature_info_payload_t->timer_cnt;
		}
		else
		{
			//�������Ч���ݣ���Ϊ���غ�
		}
		feature_info_payload_t += 1;//ÿ��ƫ��һ���ṹ��
	}

}
/*************************************************************************
 * �������� :	PacketFeatureInfoInformingData
 * ����˵�� :	�������������Ϣ�غɣ���������Ϣ��֪������FeatureInfoInforming�����ʹ��
 * ����˵�� :	U8   Collectmode,        //�ɼ���ʽ
				U8   TEI,                //�ɼ���tei
				U8   CollectSeq,         //�ɼ����
				U8   *FeatureInfoSeq1,   //��������1
				U16  FeatureInfoSeqlen1, //��������1����
				U8   *FeatureInfoSeq2,   //��������2
				U16  FeatureInfoSeqlen2, //��������2����
				U8   *payload,           //����֡��ָ��
				U16  *len                //����������ݳ���
 * ����ֵ   :
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
 * �������� :   void packet_featureinfo(U8 feature, U8 *addr, U8 edge)
 * ����˵�� :   
 *     ��������CCO���ڴ��������Ϣ��֪�������ݣ����ҷ��ͳ�ȥ
 * ����˵�� :   
 *     U8 feature  ��������
 *     U8 *addr    ��֪Ŀ��MAC��ַ���㲥��֪ΪȫFF
 * ����ֵ   :    
 *     void  �޷���ֵ
*************************************************************************/
void packet_featureinfo(U8 feature, U8 *addr, U8 edge)
{
	FEATURE_INFO_t *feature_info_t = 
		feature == e_VOLTAGE?&Voltage_info_t[0]:
		feature == e_FREQUNCY?&Frequncy_info_t[0]:
        #if defined(STATIC_MASTER)
		(feature == e_PERIOD && (edge&e_COLLECT_NEGEDGE))?&Period_info_t[0]:   // �½���
		(feature == e_PERIOD && (edge&e_COLLECT_POSEDGE))?&Period_info_t[3]:NULL;  // ������
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
	
	PacketFeatureInfoInformingData(  edge,  //U8   Collectmode, �����ء��½��ػ�˫��
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
							DevicePib_t.DevMacAddr,	//U8   *Srcaddr, //���ͷ� ��ַ
                            #elif defined(STATIC_MASTER)
							e_DOWNLINK,		//U8   Directbit,
							APS_MASTER_PRM,		//U8   Startbit,
							FlashInfo_t.ucJZQMacAddr,	//U8   *Srcaddr, //���ͷ� ��ַ
							#endif
							addr,//U8	 *DesMacaddr, //���շ� ��ַ
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
			
			//�����ѹ��Ƶ��ʶ���㷨
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
  	 	Final_result_t.PublicResult_t[i].Differtime = Localbts-Sendbts;//���¶���
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
	//[0]��ű�����Ĳɼ���Ϣ
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
		if(inverse == TRUE)//������������������
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
			//����ά�������綨�������µ�startntb
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
							//����ά�������綨�������µ�startntb
							ccontb =convert_startntb(feature_info_seq_t->startntb,CCOmacaddr);
						}
						app_printf("ccontb=0x%08x\n",ccontb);
						if(phase>1) ccontb = ccontb+(phase-1) * 25*6666;
						app_printf("phase=%d,new ccontb=0x%08x\n",phase,ccontb);
						sta_area_ind_get_start_ntb_from_zero_data(feature_info_seq_t->phase1num, ccontb,CollectSeq);
					}
					else//�ش��� 
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

				ccodatanum = feature_info_seq_t->phase1num;//��Ӧ������

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

/*ͨ��SNID���MACADDR*/
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
	if(Final_result_t.PublicResult_t[0].FinishFlag == UNFINISH) //��ǰ����������δʶ�����
	{
		return 0;
	}
	for(i=0;i<COMMPAER_NUM;i++)
	{
		if(memcmp(Final_result_t.PublicResult_t[i].Macaddr,Macaddr,6) !=0 
				&& memcmp(Final_result_t.PublicResult_t[i].Macaddr,NullAddr,6) !=0 )
		{
			if(Final_result_t.PublicResult_t[i].FinishFlag == UNFINISH)//ͳ������ʶ����δ��ɵ��ھ�������
			{
				NbSNID++;
			}
		}
	}
	//���ڴ�����������ʱ������1�����ʶ��
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

//����ֵ��ʶ�����״̬
//value����ǰ�Σ��ԱȵĽ��
//Macaddr����ǰ�ε�CCO��ַ
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
			//���ν��յ���CCO����ԱȽ���ʱ���ж��Ƿ��б���������ڽ��С�
			//����еȴ�����������ʶ��û�б������ʱ��ֱ�����
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
					if(Final_result_t.FinishFlag== UNFINISH) //����δ��ɵ�ʶ������ʱ������ˢ�¶�ʱ������ʱ�����������ʶ��
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
	if(memcmp(Final_result_t.CCOMacaddr,pCCOMAC,6))//�����µ�����ʱ����ʶ��״̬
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
	//�ж�ԭ�����ƶ���ߵ�����Ϊ�������磬����������Ϊ��ǰ����������ʱ�����Ϊ1��������Ϊ2.
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
 * �������� :	DealFeatureInfoInforming
 * ����˵�� :	������Ϣ��֪
 * ����˵�� :	INDENTIFICATION_HEADER_t *pIndentificationHeader // ̨��ʶ��header
 * ����ֵ   :   void
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
	if(informing_data_t->CollectMode == e_COLLECT_NEGEDGE)//��¼����Ϣ
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
		else//��������
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
	//�쳣����
	if( value==0 )
	{
		
		app_printf("invaild info data!\n");
		return;
	}
	//ʶ�����
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
 * �������� :	DealDistinguishResultQuery
 * ����˵�� :	̨���б�����ѯ
 * ����˵�� :	INDENTIFICATION_HEADER_t *pIndentificationHeader // ̨��ʶ��header
 * ����ֵ   :   void
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
                          pIndentificationInd->SrcMacAddr,//U8	 *DesMacaddr, //���շ� ��ַ             
                          e_FIRST_PHASE,//U8   Phase,    
                          pIndentificationInd->Featuretypes);//U8   Featuretypes)
	#endif
}


/**************************************����Ϊ̨��ʶ�����̿��ƶ�ʱ����״̬��************************************/


/**************************************����Ϊ̨��ʶ��ɼ���ʱ��************************************/

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

//��ѹ�ɼ���ʱ��
ostimer_t *voltage_info_collect_timer = NULL;
FEATURE_INFO_t Voltage_info_t[MAX_VOLTAGE];
void voltageinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
{
	U8 i = 0;
	for(i=0;i<MAX_VOLTAGE;i++)//��ӡ���
	{
		if(Voltage_info_t[i].timer_cnt >= Voltage_info_t[i].total_num)
		{
			show_bcd_to_char((U8 *)Voltage_info_t[i].collect_info,2*Voltage_info_t[i].timer_cnt);
			Voltage_info_t[i].state = e_INFO_COLLECT_END;
            #if defined(STATIC_MASTER)//cco��Ҫ��ͣ��ʱ������ʼ�ռ�
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
		return;//���ɼ������򷵻ز��ٲɼ�
	}
	//���Ͳɼ�����
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
//Ƶ�ʲɼ���ʱ��
ostimer_t *freq_info_collect_timer = NULL;
FEATURE_INFO_t Frequncy_info_t[MAX_FREQUNCY];
void freqinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
{
	U8 i = 0;
	for(i=0;i<MAX_FREQUNCY;i++)//��ӡ���
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

//��������ֵƽ��ֵ
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

//���������к�ƽ��ֵ�����ƫ��
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

//���ڲɼ���ʱ��
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


ostimer_t *period_info_collect_timer = NULL;	//��Ƶ���ڿ��ƶ�ʱ��
FEATURE_INFO_t Period_info_t[MAX_PERIOD];		//��Ƶ���ڻ���buf

//�ص�����������ʱ����Ƶ�������ݲɼ���ɣ��ռ���Щ���ݷ���FEATURE_INFO_t��

/*************************************************************************
 * �������� :	void periodinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
 * ����˵�� :	
 *      �������ǹ�Ƶ������Ϣ�ɼ���ʱ���ص��������ڴ˻ص������У�CCO����STA������ʼNTBֵ��
 *   �ӱ�������NTBֵ�����ݻ�������ȡָ�������Ĺ���NTB���ݵ���Ƶ����������Ϣ������
 *
 * ����˵�� :	
 *     struct ostimer_s *ostimer  �ص��Ķ�ʱ��ָ��
 *     void *arg   ��ʱ���ص���������
 * ����ֵ   :   
 *     ��
 *************************************************************************/
void periodinfocollecttimerCB(struct ostimer_s *ostimer, void *arg)
{
	S8 result=0;
	
	U16 j=0,effective_num=0;
	U32 startntb = 0;
	
	FEATURE_INFO_t *info_t = &Period_info_t[0];		//��Ƶ������Ϣ�洢����
	ZERODATA_t *zero_data_t = &ZeroData[0];			//CCO���˴洢��STA����洢
    
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
		//ʵ���½�����ʼNTBΪPeriod_info_t[0].startntb
		//A���½���ʹ��Period_info_t[0].startntb����
		//B���½���ʹ��A��ʱ��Ϊ��׼
		//C���½���ʹ��A��ʱ��Ϊ��׼
		//A��������ʹ��Period_info_t[0].startntb����
		//B��������ʹ��A������ʱ��Ϊ��׼
		//C��������ʹ��A������ʱ��Ϊ��׼

		info_t->startntb = startntb;
		
		tp_start_ntb = j==2? Period_info_t[0].startntb:startntb;
		info_t->timer_cnt = info_t->effective_cnt = effective_num;
		
		app_printf("phase %d result : %d effective num : %d��startntb=%08x\n",j,result,effective_num,startntb);
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
 * �������� :	S8 period_info_collect_timer_init(void)
 * ����˵�� :	
 *      ������Ϊ��Ƶ����������Ϣ�ɼ�������ʱ���ĳ�ʼ������
 * ����˵�� :	
 *     ��
 * ����ֵ   :   
 *     S8  �����򷵻�TRUE
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
 * �������� :	void modify_period_info_collect_timer(uint32_t first)
 * ����˵�� :	
 *      �����������޸Ĺ�Ƶ����������Ϣ�ɼ�������ʱ���Ķ�ʱʱ�䣬��ʱ��Ϊ����ģʽ
 * ����˵�� :	
 *     ��
 * ����ֵ   :   
 *     ��
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

/**************************************����Ϊ̨��ʶ��ɼ���ʱ��************************************/

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


/***ntb ȡֵ***/
/**************************************************************************
 * �������� :	area_ind_get_ntb_data_by_start_ntb
 * ����˵�� :	
 *      ���ҹ����NTBֵ���ҵ���ʼNTBֵ��Ȼ�󰴲ɼ������ɼ�һ���Ĺ���NTBֵ��ȫ�ֵ�Period_info_t�ṹ��
 * ����˵�� :	
 *    S16 *periodbuf   Period_info_t�����ڴ�Ź�Ƶ���ڹ�������ݵ�collect_info���飬U16���ͣ����500��
 *    U16 *num     ָ����������ڴ����Ч�Ĳɼ���������ֵ����
 *    U32 startntb �ɼ�����ʼNTBֵ
 *    void *data   �������NTBֵ������Buff
 *    U16 maxnum   �ɼ��������,CCO�궨��Ϊ300
 *    U32 *start_collect_ntb  ��ָ�����ڴ洢ʵ�ʵ���ʼ�����NTB��ֵ
 *    U8 collecttype  ��1�����ʾ����ʹ�����������е�����ʱ��ɼ���0���෴
 * ����ֵ   :   
 *    S8   ���ؽ���룬Ϊ0�������ɼ�����������������Ϊ�ɼ��쳣
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
		
	*num += 1;//ȡ��������NTB��ֵ������Ҫ��ȡһ��ֵ���ܴﵽҪ��

	//app_printf("zerodata_t addr  : 0x%08x offset : %d \n",data,zerodata_t->NewOffset);
	
	if(startntb < zerodata_t->NTB[0] && startntb > zerodata_t->NTB[maxnum-1])//�ж�Ҫ�ҵĵ��Ƿ���β��ͷ֮��
	{
		get_pos = 0;
		if(collecttype)// 1����ʾ����ʹ�����������е�����ʱ��ɼ�
		{
			get_pos = abs(startntb - zerodata_t->NTB[0]) < abs(startntb - zerodata_t->NTB[maxnum-1]) ? 0:maxnum-1;//ȡ������һ��
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

	for(i=0;i<=maxnum-1;i++)//�жϴ�ͷ��β֮�������
	{
		if(startntb > zerodata_t->NTB[i] && startntb < zerodata_t->NTB[i+1])
		{
			get_pos = i+1;
			if(collecttype)//1����ʾ����ʹ�����������е�����ʱ��ɼ�
			{
				get_pos= abs(startntb - zerodata_t->NTB[i]) < abs(startntb - zerodata_t->NTB[i+1]) ? i:i+1;//ȡ������һ��
			}
			//app_printf("\nget_pos : %d ,start_ntb : 0x%08x zerodata_t->NewOffset : %d num : %d,0x%08x,0x%08x\n",get_pos,
			//	startntb,zerodata_t->NewOffset,*num,zerodata_t->NTB[i],zerodata_t->NTB[i+1]);
			
			if(get_pos <= zerodata_t->NewOffset)//��Ҫȡ�������ڵ�ǰ��������λ��ǰ
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
			else//��Ҫȡ�������ڵ�ǰ��������λ�ú�
			{
				totalnum = maxnum - get_pos + zerodata_t->NewOffset +1 ;
				
				if(maxnum - get_pos >= *num)
				{
					result = 0;
					__memcpy(&gl_area_ind_ntb_buf[bufindex],zerodata_t->NTB+get_pos,*num*sizeof(U32));
					goto getntb;

				}
				//��ȡ�����ֵ
				result = 0;	
				__memcpy(&gl_area_ind_ntb_buf[bufindex],zerodata_t->NTB+get_pos,(maxnum - get_pos)*sizeof(U32));
				if(totalnum < *num)//������ݲ���
				{
					*num = totalnum;
					result = -2;	
				}
				//��ȡǰ���ֵ
				__memcpy(&gl_area_ind_ntb_buf[bufindex+maxnum] - get_pos,zerodata_t->NTB,(*num - (maxnum - get_pos))*sizeof(U32));

				goto getntb;
			}
		}
		else
		{
			//��һ��ѭ������
		}

	}

	result = -1;//δ�ҵ�����
	*num = 1;
	
	getntb :
		
	send_time ++;

#if 0
	for(i=0;i<*num-1;i++)
	{
		*(periodbuf+i) = (S16)(gl_area_ind_ntb_buf[i+1] - gl_area_ind_ntb_buf[i] - 20*25000)/8;
	}
#else
	//FALSE��ʾ̨��,TRUE��������ģʽ���߷��ʹ�������10���Ժ�ָ���������
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

/*********************************************����̨��ʶ���㷨*******************************************/

/**************************************period***********************************
 * �������� :	cal_S16_variance
 * ����˵�� :	̨��ʶ���㷨����
 * ����˵�� :	S16 *p// һ�����ݣ�cco��sta�������ݵķ���
 				U16 cnt//��������
 * ����ֵ   :   float
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
 * �������� :	get_Dvalue_by_2buf
 * ����˵�� :	̨��ʶ���㷨����
 * ����˵�� :	S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2 ,S16 *getbuf , U16 *getnum
 * ����ֵ   :   S8
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
 * �������� :	get_result_by_variance
 * ����˵�� :	̨��ʶ���㷨����
 * ����˵�� :	S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2 
 * ����ֵ   :   void
 *************************************************************************/
S8 get_result_by_variance(S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2,U8 *result)
{
	S16    *getbuf = NULL;
	U16     getnum = 0;
	float   value;

	S16 sum1 = 0;		//��������ƽ��ƫ��
	S16 sum2 = 0;

	U16 max_diff1 = 0;	//���ƫ��
	U16 max_diff2 = 0;	//���ƫ��
	
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
 * �������� :	get_loss_matrix
 * ����˵�� :	̨��ʶ���㷨������ʧ�������
 * ����˵�� :	U16 *crow_buf , U16 crow , U16 col_buf, U16 col
 * ����ֵ   :   void
 *************************************************************************/
/*S16 get_loss_matrix(S16 *crow_buf , U16 crow , S16 *col_buf, U16 col)
{
	S16 **a;
    U16 i, j;//,ii,jj;
    a = (S16**)__malloc(sizeof(S16*)*crow);//Ϊ��ά�������3��
    for (i = 0; i < crow; ++i){//Ϊÿ�з���4����С�ռ�
        a[i] = (S16*)__malloc(sizeof(S16)*col);
    }
    //��ʼ��
    for (i = 0; i < crow; ++i){
        for (j = 0; j < col; ++j){
            a[i][j] = 0xffff;
        }
    }
	
	//ŷʽ�������
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
	//��ʧ����	
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
		//�������
	    for (ii = 0; ii < crow; ++ii)
		{
	        for (jj = 0; jj < col; ++jj){
	            app_printf("%-4x ", a[ii][jj]);
	        }
			//os_sleep(5);
	        app_printf("\n");
	    }/	
	}
	
    //�������
    for (i = 0; i < crow; ++i)
	{
        for (j = 0; j < col; ++j){
            app_printf("%-4x ",a[i][j]);
        }
		os_sleep(5);
        app_printf("\n");
    }
	
    //�ͷŶ�̬���ٵĿռ�
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

    if(pIndentificationInd->Collectiontype == e_FEATURE_INFO_INFORMING)       //��λʶ��������Ϣ��֪
	{
        cco_phase_position_edge_proc(pIndentificationInd);
	}
	else if(pIndentificationInd->Collectiontype == e_DISTINGUISH_RESULT_INFO) //̨��ʶ������ѯ
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
				if(memcmp(pIndentificationInd->SrcMacAddr, ccomac,6) ==0)//������������CCO���͵���������
                {            
					DealFeatureCollectStart(pIndentificationInd);
                }
			}
			break;
		case e_FEATURE_INFO_GATHER://����ʽ
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






















