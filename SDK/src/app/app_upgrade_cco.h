#ifndef __APP_UPGRADE_CCO_H__
#define __APP_UPGRADE_CCO_H__
#include "list.h"
#include "types.h"
#include "ZCsystem.h"
#include "app_upgrade_comm.h"
#include "aps_interface.h"

#if defined(STATIC_MASTER)

#define  UPGRADE_PERFORM_TIME           6       // minitue
#define  UPGRADEMAXTIMES                30
#define  MIN_UPGRADE_BYTE_SIZE          1024    //512有其他厂家的STA程序超过了512字节，所以需要进行扩充到1024

typedef enum{
    EVT_UPGRADE_RESET,
    EVT_UPGRADE_START,
    EVT_FILETRANS_SEND,
    EVT_UPGRADE_QUERY,
    EVT_STA_INFO_QUERY,
    EVT_UPGRADE_TESTRUN,
    EVT_STA_INFO_QUE_FIRST,
    EVT_GET_STA_ADDR,
    EVT_STOP_UPGRADE,
    EVT_FILETRANS_BRADCST,
    EVT_UPGRADE_ALL_START,
    EVT_UPGRADE_IDLE,
}UPGRADE_STATE_e;

typedef enum
{ 
    e_IDLE,
    e_UPGRADE_START_TASK,
    e_UPGRADE_QUERY_TASK,
    e_UPGRADE_PERFORM_TASK,
    e_UPGRADE_INFOQUERY_TASK,
    e_UPGRADE_STOP_TASK,
} UPGRADE_TASK_TYPE_e;

typedef enum
{ 
    e_JZQ_TASK,
    e_SELF_STA_TASK,
    e_SELF_CJQ_TASK,
    e_SELF_ONE_TASK,
} UPGRADE_SRC_TYPE_e;

typedef enum
{
	AUTO_UPGRADE_IDLE = 0,
    AUTO_UPGRADE_LOAD_BIN_OK ,
    AUTO_UPGRADE_NEED_START,
    AUTO_UPGRADE_NEED_RUNNING,
    AUTO_UPGRADE_FINISH,
    AUTO_UPGRADE_CHECK_OK,
}UPGRADE_BY_SELF_STATE_e;

typedef struct{
    U32     FileSize;
    U32     FileCrc;
    U32     BlockSize;
    U32     TotolBlockNum;
    U32     BlockBitMapLen;
    U32     ImageBaseAddr;
    U8      FileSoftVer[2];
}UPGRADE_FILE_INFO_t;

typedef struct{
    U32     UpgradeID;
	U32     CcoUpgradeStatus;
	U16     UpgradePLCTimeout;
    U16     UpgradeTimeout;
    U16     CurrNodeTotalNum;
    U16     CurrentSlaveIndex;
	U16     CurrentParent;
    U8      CurrSlaveAddr[6];
    U8      CurrSlaveLevel;
    U8      CurrNodeType;
	S8      UpgradeTotalNum;
	S8		UpgradeStartNum;
    S8		UpgradeQueryNum;
    U16     NeedSendNum;
    U16     CurrBlockIndex;
    S8      ReTransmitCnt;
    U8      DealySecond;
    U8      QueryRecvStatusNum;
}UPGRADE_STATUS_INFO_t;

typedef struct{
    UPGRADE_FILE_INFO_t          UpgrdFileInfo;
    UPGRADE_STATUS_INFO_t     UpgrdStatusInfo;
}CCO_UPGRADE_CTRL_INFO_t;

typedef struct{
    U8     Valid            :  1;
	U8     StartResult	    :  1;    //upgrade start      0 fail  1 success
	U8     QueryResult      :  1;    //upgrade bitquery   0 fail  1 success
	U8     InfoQueryResult  :  1;    //upgrade infoquery  0 fail  1 success
	U8     PerformResult    :  1;    //upgrade perform    0 fail  1 success
	U8     StopResult       :  1;    //upgrade stop       0 fail  1 success
	U8     res              :  2;    
	U8     UpgradeRate;              //升级进度百分比0~100
}__PACKED CCO_UPGRADE_POLL;


extern U8    FileUpgradeData[MIN_UPGRADE_BYTE_SIZE * 1024];
extern CCO_UPGRADE_CTRL_INFO_t   CcoUpgrdCtrlInfo;
extern ostimer_t *UpgradeSuspendtimer;


void upgrade_cco_reset_evt(void);
void upgrade_cco_stop(void);
void upgrade_cco_test_run_req_aps(void);
void upgrade_cco_station_info_request(void* pTaskPrmt);
void upgrade_cco_file_trans_cfm_pro(uint8_t Status);
int8_t  upgrade_cco_init(void);
void upgrade_cco_app_upgrade_start_cfm_proc(UPGRADE_START_CFM_t* pUpgradeStartCfm);
void upgrade_cco_app_upgrade_status_query_cfm_proc(UPGRADE_STATUS_QUERY_CFM_t* pUpgradeQueryCfm);
void upgrade_cco_upgrade_station_info_cfm_proc(STATION_INFO_QUERY_CFM_t* pStationInfoQueryCfm);
U32 upgrade_cco_check_upgrade_data(U8* databuf, U16 buflen);
void upgrade_cco_modify_upgrade_suspend_timer(uint32_t Sec);
void upgrade_cco_start_upgrade_byself(void);
void upgrade_cco_read_upgrade_info(U8* BinType);
void upgrade_cco_ctrl_timer_modify(uint32_t Ms, void* arg);

INT32U upgrade_cco_get_ctrl_timer_state(void);
void upgrade_cco_ctrl_timer_modify_and_start(INT32U first_time_ms);
void upgrade_cco_get_poll_info(INT16U idx, CCO_UPGRADE_POLL *p_data);
#endif

#endif 

