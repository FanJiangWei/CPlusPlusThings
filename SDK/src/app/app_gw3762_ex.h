#ifndef _APP_GW3762_EX_H_
#define _APP_GW3762_EX_H_
#include "types.h"
#include "app_common.h"
#include "ProtocolProc.h"
#include "app_gw3762.h"
#include "app_dltpro.h"

#if defined(STATIC_MASTER)

#define MAX_LEVEL			15
#define MAX_NODE_NUM		32
#define MAX_SUB_NODE_NUM    50

extern U16 Topchangetimes;

typedef enum
{
    MINUTES = 1,					/* 表示分钟 */
    HOURS ,					        /* 表示小时 */
} BroadCastCyclyUnit_e;

typedef struct
{
    MDB_t    TimeData;
	
    U8       DstMacAddr[MACADRDR_LENGTH];
    U8       SrcMacAddr[MACADRDR_LENGTH];
} __PACKED TIME_CALIBRATE_REQUEST_t;

typedef struct{
    U8    Record : 1;
    U8    Phase1 : 1;
    U8    Phase2 : 1;
    U8    Phase3 : 1;
    U8    LNerr : 1;
    U8    Reserve1 : 3;
    U8    post3 : 2;
    U8    post2 : 2;
    U8    post1 : 2;
    U8    MeterType : 2;
}__PACKED APPGW3762AFN10F102_t;

typedef struct{
	U8  Enterlevel;
	U16 Levelnodenum;
}__PACKED LEVEL_INFO_t;

typedef struct{
	U16 Netnodetotalnum;
	U16 Onlinenodenum;
	U32 Startnettime;
	U16 Networktime;
	U8  Beaconperiod;
	U16 Routerperiod;
	U16 Topchangetimes;
	U8  TotalLevel;
	LEVEL_INFO_t Levelinfo_t[MAX_LEVEL];
}__PACKED AFNF0F100_DATA_UNIT_t;

typedef enum{
	e_F0F102_CKQ = 0x0001,
	e_F0F102_JZQ = 0x0002,
	e_F0F102_METER = 0x0004,
	e_F0F102_RELAY = 0x0008,
	e_F0F102_CJQ2 = 0x0010,
	e_F0F102_CJQ1 = 0x0020,
	e_F0F102_ZONEIDF = 0x0040,
	e_F0F102_DblMODE = 0x0080,
	e_F0F102_485 = 0x0100,
	e_F0F102_3PMETER = 0x0200,
}F0F102_DEVICE_TYPE;

typedef enum{
	e_AREA_UNKOWN,
	e_AREA_INSIDE,
	e_AREA_OUTSIDE,
	e_AREA_NOSUPPORT,
	e_AREA_ADDMETER,
}AREA_STATE;

typedef struct{
	U8  Slave2SlaveAddr[6];				//节点地址
	U8  Slave2SlaveProtocolType;		//通信协议
	U8  Slave2SlaveDeviceType;			//设备类型
}__PACKED SUBNODE_INFO_t;

typedef struct{
	U16 NodeTEI;					//节点TEI
	U16 ProxyTEI;					//父节点TEI
	U8  Nodelevel : 4;				//节点层级
	U8  Noderole : 4;				//节点角色
}__PACKED NODE_TOPO_INFO_t;		//节点top信息

typedef struct{
	U8  Nodeaddr[6];					//节点地址
	NODE_TOPO_INFO_t Nodetopinfo_t;		//节点TOP信息-5个字节

	U8 	Netstate : 1;			//在网状态标志位
	U8 	Beaconslot : 1;			//信标时隙是否已满
	U8 	Inwhitelist : 1;			//是否在表档案
	U8 	Rcvheart : 1;			//是否收到过心跳
	U8 	NoHeartRoutePeriod : 4;			//没有收到心跳的路由周期数

	U16 Devicetype;						//设备类型
	U8  Phaseinfo;						//相位信息
	U16 Proxytimes;						//代理变更次数
	U16 Nodeofflinetimes;				//站点离线次数
	U32 Nodeofflinetime;				//站点离线时长
	U32 Nodeofflinetime_max;			//站点离线最大时长
	U32 Upcomuntsucesrate;				//上行通信成功率
	U32 Dncomuntsucesrate;				//下行通信成功率
	U8  Masterversion[3];				//主版本号
	U8  Slaveversion[2];				//次版本号
	U16 Nexthopinfo;					//下一跳信息
	U16 Channeltype;					//信道类型
	U8  Protocoltype;					//规约类型
	U8  Zoneareastate;					//台区状态
	U8  Zoneareaaddr[6];				//台区号地址
	U8  Subnodenum;						//从节点下接从节点数量
	SUBNODE_INFO_t Subnode_info_t[MAX_SUB_NODE_NUM];	//下接节点信息
}__PACKED NODE_INFO_t;

typedef struct{
	U16 Netnodetotalnum;
	U16 Reportnodenum;
	U16 Startseq;
	NODE_INFO_t Nodeinfo_t[MAX_NODE_NUM];
}__PACKED AFNF0F102_DATA_UNIT_t;

void app_gw3762_ex_proc(U8 afn, U32 fn, APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port);
void app_gw3762_afn06_f10_report_node_state_change(U8 *data, U16 len, MESG_PORT_e port);
void app_gw3762_up_afnf1_f100_up_frame(U8 *Addr, AppProtoType_e proto, U8 *data, U16 len, MESG_PORT_e port,U8 localseq);
#ifdef SHAAN_XI
void send_broadcast_live_line_zero_line_enabled(void);
#endif
void app_send_live_zero_enable_wait_timer(void);
#endif
#endif


