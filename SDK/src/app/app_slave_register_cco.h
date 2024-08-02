/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_slaveregister.h
 * Purpose:	
 * History:
 * Author : 
 */


#ifndef __APP_SLAVE_REGISTER_CCO_H__
#define __APP_SLAVE_REGISTER_CCO_H__
#include "list.h"
#include "types.h"
#include "aps_interface.h"
#include "ProtocolProc.h"
#define     REPORT_METER_MAX    32

//�Զ�ɾ��ʱ����������ָʾ������������ݾ�STA�޷�����һ���쳣�������⣬��ֵΪ1
#define		MAXLEVELNODENUM	    50 

typedef enum
{
	e_REGISTER_UN,
    e_REGISTER_START,
	e_REGISTER_GET,
	e_REGISTER_REQ,
	e_REGISTER_LOCK,
    e_REGISTER_STOP,
    e_REGISTER_JZQ_STOP,
    e_REGISTER_COLLECT,
} EvtRegister_e;

typedef enum
{
	e_LEAVE_AND_DEL_DEVICELIST,		//��������ָʾ����ɾ���豸�б�
    e_LEAVE_WAIT_SELF_OUTLINE,      //��������ָʾ���ҵȴ��Զ�����
  	
}DelWhilthType_e;

typedef enum
{
  e_GENERAL_REGISTER,
  e_NETWORK_REGISTER,
  e_CJQSELF_REGISTER,
} RegisterState_e;

typedef struct{
    U8		RegistState;
    U8      CrnRegRound;
    U16		CrnSlaveIndex; 
    
    U8      SlaveRegFlag;
    U8      CrntNodeType;
	U16		res;
	
    U32		Timeout;

	
    U8		CrntNodeAdd[6];
    U16		ReporttotalNum;	

	U16     CrnCollectIndex;
	U8 		portID;
	U8 	    RegisterType;

}__PACKED RegisterInfo_t;


typedef struct{
    U8	  Valid             :  1;
	U8    QueryState		:  1;   //1��ʾ��ѯ���
	U8 	  LockState         :  1;   //1��ʾ�������
	U8 	  CollectState      :  1;   //1��ʾ�ɼ�����֧��
	U8    CollectValid      :  1;
	U8    res               :  3;   //����
}__PACKED CcoRegstRecord_t;

extern ostimer_t *register_suspend_timer;
extern ostimer_t *register_over_timer;//ע�����ʱ�����
extern ostimer_t *g_CJQRegisterPeriodTimer;
extern RegisterInfo_t  register_info;
extern CcoRegstRecord_t CcoRegPolling[MAX_WHITELIST_NUM] ;

void clean_registe_list(void);
void modify_cycle_del_slave_timer(U32 Sec);
void register_cco_query_cfm_proc(REGISTER_QUERY_CFM_t    *pRegisterQueryCfm);
//void find_register_query_task_info(work_t *work);

void change_register_run_state(uint8_t state, uint16_t Ms);
int8_t slave_register_timer_init(void);
void register_suspend_timer_init(void);
void modify_slave_register_timer(uint32_t Ms);
void modify_register_suspend_timer(uint32_t Sec);
void register_slave_start(U32 Ms, MESG_PORT_e port);

#endif



    
