/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     __APSINDENTIFICATIONH__.h
 * Purpose:	 Aps interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __APP_AREA_INDENTIFICATION_COMMON_H__
#define __APP_AREA_INDENTIFICATION_COMMON_H__



//#include "ZCheader.h"
#include "aps_layer.h"



#define INTtoBCD(a) ((a)/10)*16+((a)%10)
#define BCDtoINT(i) ((((i & 0xf0) >> 4) * 10) + (i & 0x0f))

#define    START_COLLECT_TIME     10 //5S?????
#define    COLLECT_VOLTAGE_CYCLE  10 //10S ????
#define    COLLECT_FREQUNCY_CYCLE  5 //10S ????

#define    FEATURE_CNT         500
#define    PERIOD_PMA         10

#define	   MAX_CACULATE		  30

#define		NTB_OFFSET		200


typedef enum
{
    e_DEFAULT_PHASE  = 0,
    e_FIRST_PHASE    = 1,
    e_SECOND_PHASE   = 2,
    e_THIRD_PHASE    = 3,
} COLLECTPHASE_e;

typedef enum
{
    APS_SLAVE_PRM    = 0,
    APS_MASTER_PRM   = 1,
} APSPRM_e;
	
typedef enum
{
    e_VOLTAGE  = 1,
    e_FREQUNCY,
    e_PERIOD,
} FEATURETYPE_e;

typedef enum
{
    e_RESET  = 0,
	e_COLLECT_NEGEDGE,//下降沿
    e_COLLECT_POSEDGE,//上升沿
    e_COLLECT_DBLEDGE,//双沿
} COLLECTMODE_e;

typedef enum
{
    e_FEATURE_COLLECT_START  = 1,
    e_FEATURE_INFO_GATHER,
    e_FEATURE_INFO_INFORMING,
    e_DISTINGUISH_RESULT_QUERY,
    e_DISTINGUISH_RESULT_INFO,
} COLLECTTYPE_e;

typedef enum
{
    e_INDENTIFICATION_FINISHFLAG_UNDERWAY  = 0,
    e_INDENTIFICATION_FINISHFLAG_OVER,    
} INDENTIFICATION_ENDFLAG_e;

typedef enum
{
    e_INDENTIFICATION_RESULT_UNKOWN  = 0,
    e_INDENTIFICATION_RESULT_BELONG,
    e_INDENTIFICATION_RESULT_NOTBELONG,
    
} INDENTIFICATION_RESULT_e;

typedef enum
{
	e_gw3762F0F5TopologyQueryFlag = 0,
	e_nw2016AfnF0F1TopologyQueryFlag,
	e_gw3762Afn10F21TopologyQueryFlag,
	e_gw3762Afn10F102TopologyQueryFlag,
	e_FlieTransTopologyQueryFlag,
	e_STARegTopologyQueryFlag,
	e_Voltage_TopoQueryFlag,
	e_Frequncy_TopoQueryFlag,
	e_Period_TopoQueryFlag,
}__PACKED QUERYNETTOPO_e;

typedef struct
{
    U16    ProtocolVer     : 6;
    U16    HeaderLen       : 6;
    U16    Directbit       : 1;
    U16    Startbit        : 1;
    U16    Phase           : 2;
    U16    PacketSeq;
	U8     Macaddr[6];
	U8     Featuretypes;
	U8     Collectiontype;
} __PACKED INDENTIFICATION_HEADER_t;

#define COMMPAER_NUM 16
typedef struct
{
	U8		Collectseq;				//网络采集序号
	U8		CalculateNum;			//网络当前采集次数
	U16 	CalculateMaxNum;		//网络最大采集次数
	
	U16 	SameRatio;				//网络相似度
	U8		Macaddr[6];				//网络CCO地址
	
	U32     Differtime;				//网络tick差
	U32     SNID;					//网络snid
	U32		EdgeType;				//网络沿信息
	U8		FinishFlag;				//网络识别完成标志
} __PACKED PUBLIC_RESULT_t;


typedef enum
{
	UNFINISH,
	FINISH
} INID_STATUS_e;



typedef struct
{
	U8		CCOMacaddr[6];		//sta使用归属CCO地址
	U8		FinishFlag;			//sta使用台区识别标志位
	PUBLIC_RESULT_t PublicResult_t[COMMPAER_NUM];
} __PACKED FINAL_RESULT;

FINAL_RESULT Final_result_t;


// 台区特征采集启动命令数据格式
typedef struct
{
    U32    Startntb;        // 起始NTB，全网开始采集时刻的NTB
	U8     CollectCycle;    // 采集周期，对于工频周期采集无意义；其它时表示多少秒采集一次特征
	U8     CollectNum;      // 连续采集特征信息的数量
	U8     CollectSeq;      // 采集序列号，CCO每启动一次，就累加1次
	U8     res;

} __PACKED FEATURE_COLLECT_START_DATA_t;

typedef struct
{
    U8   res;
	U8   FirstReportNum;
	U8   SecondReportNum;
	U8   ThirdReportNum;
	
} __PACKED FEATURE_SEQ_t;

typedef struct
{
    U16    TEI         : 12;
	U16    CollectMode : 2 ;
	U16    res         : 2 ;
	U8     CollectSeq;
	U8     InformingNum;
} __PACKED FEATURE_INFO_INFORMING_DATA_t;

typedef struct
{
	U32   startntb;
    U8    res;
	U8    phase1num;
	U8    phase2num;
	U8    phase3num;
} __PACKED FEATURE_INFO_SEQ_t;

typedef struct
{
    U16    TEI;
	U8     finishflag;
	U8     result; // = (SameRatio/CalculateMaxNum)>60 ? TRUE:FALSE;
	U8     CCOMacaddr[6];
} __PACKED INDENTIFICATION_RESULT_DATA_t;



ostimer_t *wait_finish_timer;

#if defined(STATIC_MASTER)

#define MAX_PERIOD   6
#define MAX_FREQUNCY 3
#define MAX_VOLTAGE  3

#elif defined(STATIC_NODE)

#define MAX_PERIOD   3
#define MAX_FREQUNCY 3
#define MAX_VOLTAGE  3

#elif defined(ZC3750CJQ2)

#define MAX_PERIOD   1
#define MAX_FREQUNCY 1
#define MAX_VOLTAGE  1

#endif

typedef enum
{
	e_INFO_COLLECTING = 0,
    e_INFO_COLLECT_END,
}INFO_E;

typedef struct
{
    U16    collect_info[FEATURE_CNT];
	U16    effective_cnt;
	U16    timer_cnt;
	U16    total_num;
	U16    cycle;
	U32    startntb;
	U8     collect_seq;
	U8     state;

} __PACKED FEATURE_INFO_t;

typedef struct{
	uint8_t          type;
	FEATURE_INFO_t   *info_t;
}INDENTIFICATION_FEATURE_INFO_t;

extern INDENTIFICATION_FEATURE_INFO_t  area_ind_feature_info_t;


extern FEATURE_INFO_t Voltage_info_t[MAX_VOLTAGE];
extern FEATURE_INFO_t Frequncy_info_t[MAX_FREQUNCY];
extern FEATURE_INFO_t Period_info_t[MAX_PERIOD];

extern ostimer_t *voltage_info_collect_timer;
extern ostimer_t *freq_info_collect_timer;
extern ostimer_t *period_info_collect_timer;


void packet_featureinfo(U8 feature,U8 *addr,U8 edge);

S8 voltage_info_collect_timer_init(void);
S8 freq_info_collect_timer_init(void);
S8 period_info_collect_timer_init(void);
void modify_voltage_info_collect_timer(uint32_t first,uint32_t interval);
void modify_freq_info_collect_timer(uint32_t first,uint32_t interval);
void modify_period_info_collect_timer(uint32_t first);
void area_ind_set_featrue_and_start_timer(U8 featuretypes,FEATURE_COLLECT_START_DATA_t *feature_collect_start_data_t);


void dump_S16_buf(char *s,U32 printf_code,S16 *buf , U16 num);
S8 get_Dvalue_by_2buf(S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2 ,S16 *getbuf , U16 *getnum);
float cal_S16_variance(S16 *p,U16 cnt);
S8 get_result_by_variance(S16 *buf1 , U16 num1 , S16 *buf2 , U16 num2,U8 *result);
void show_bcd_to_char(U8 *buf,U16 len);
void CleanDistinguishResult(U8* pCCOMAC);

void DealFeatureCollectStart(INDENTIFICATION_IND_t *pIndentificationInd);
void DealFeatureInfoInforming(INDENTIFICATION_IND_t *pIndentificationInd);
void DealDistinguishResultQuery(INDENTIFICATION_IND_t *pIndentificationInd);
void add_ccomacaddr(U32 SNID,U8* Macaddr);
void cacl_nbnet_difftime(uint32_t SNID, uint32_t Sendbts, uint32_t Localbts);


void FeatureCollectStart (FEATURE_COLLECT_START_DATA_t FeatureCollectStart_Data_t,
							U8	 *SrcMacaddr, //CCO addr
							U8   Phase,    
							U8   Featuretypes);
void FeatureInfoGather (	U8   Directbit,
							U8   Startbit,
							U8   *Srcaddr, //??? ??
							U8	 *DesMacaddr, //??? ??
							U16  DateSeq,
							U8   Phase,    
							U8   Featuretypes);
void FeatureInfoInforming (	U8   *payload,
							U16  len,
							U8   Directbit,
							U8   Startbit,
							U8   *Srcaddr, //??? ??
							U8	 *DesMacaddr, //??? ??
							U8   Phase,    
							U8   Featuretypes);
void DistinguishResultQuery (	U8   Directbit,
								U8   Startbit,
								U8   *Srcaddr, //??? ??
								U8	 *DesMacaddr, //??? ??
								U16  DateSeq,
								U8   Phase,    
								U8   Featuretypes);

S8 PacketFeatureInfoInformingData(	U8   Collectmode,
									U16   TEI,
									U8   CollectSeq,
									U16	 CollectNum,
									U8   *FeatureInfoSeq1,
									U16  FeatureInfoSeqlen1,
									U8   *FeatureInfoSeq2,
									U16  FeatureInfoSeqlen2,
									U8   *payload,
									U16  *len);
//void DealIndentification(void *p,U32 SNID);

void AppIndentificationIndProc(INDENTIFICATION_IND_t *pIndentificationInd);

void indentification_feature_ind(work_t *work);

S8 area_ind_get_ntb_data_by_start_ntb(S16 *periodbuf, U16 *num, U32 startntb, void *data, U16 maxnum,U32 *start_collect_ntb,U8 collecttype);

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
                                          	  uint8_t *srcaddr);

#endif /* __APSTASK_H__ */


