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
    MINUTES = 1,					/* ��ʾ���� */
    HOURS ,					        /* ��ʾСʱ */
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
	U8  Slave2SlaveAddr[6];				//�ڵ��ַ
	U8  Slave2SlaveProtocolType;		//ͨ��Э��
	U8  Slave2SlaveDeviceType;			//�豸����
}__PACKED SUBNODE_INFO_t;

typedef struct{
	U16 NodeTEI;					//�ڵ�TEI
	U16 ProxyTEI;					//���ڵ�TEI
	U8  Nodelevel : 4;				//�ڵ�㼶
	U8  Noderole : 4;				//�ڵ��ɫ
}__PACKED NODE_TOPO_INFO_t;		//�ڵ�top��Ϣ

typedef struct{
	U8  Nodeaddr[6];					//�ڵ��ַ
	NODE_TOPO_INFO_t Nodetopinfo_t;		//�ڵ�TOP��Ϣ-5���ֽ�

	U8 	Netstate : 1;			//����״̬��־λ
	U8 	Beaconslot : 1;			//�ű�ʱ϶�Ƿ�����
	U8 	Inwhitelist : 1;			//�Ƿ��ڱ���
	U8 	Rcvheart : 1;			//�Ƿ��յ�������
	U8 	NoHeartRoutePeriod : 4;			//û���յ�������·��������

	U16 Devicetype;						//�豸����
	U8  Phaseinfo;						//��λ��Ϣ
	U16 Proxytimes;						//����������
	U16 Nodeofflinetimes;				//վ�����ߴ���
	U32 Nodeofflinetime;				//վ������ʱ��
	U32 Nodeofflinetime_max;			//վ���������ʱ��
	U32 Upcomuntsucesrate;				//����ͨ�ųɹ���
	U32 Dncomuntsucesrate;				//����ͨ�ųɹ���
	U8  Masterversion[3];				//���汾��
	U8  Slaveversion[2];				//�ΰ汾��
	U16 Nexthopinfo;					//��һ����Ϣ
	U16 Channeltype;					//�ŵ�����
	U8  Protocoltype;					//��Լ����
	U8  Zoneareastate;					//̨��״̬
	U8  Zoneareaaddr[6];				//̨���ŵ�ַ
	U8  Subnodenum;						//�ӽڵ��½Ӵӽڵ�����
	SUBNODE_INFO_t Subnode_info_t[MAX_SUB_NODE_NUM];	//�½ӽڵ���Ϣ
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


