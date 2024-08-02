#ifndef __APP_UPGRADE_STA_H__
#define __APP_UPGRADE_STA_H__
#include "list.h"
#include "types.h"
#include "app_upgrade_comm.h"
#include "aps_interface.h"

#if defined(STATIC_NODE)

#define  MIN_UPGRADE_BYTE_SIZE    416       //计算位图大小使用，image0reserve大小不超过416K，需要限制大小为416K
#define  UPGRADE_STA_DATA_SIZE    4
#define  STA_BITMAP_FILE_SIZE   (1024*1024) //其他厂有超过512的文件，位图需要支持1M

typedef struct{
    U32     UpgradeID;
    U16     UpgradeTimeout;
    U16     BlockSize;
    U32     FileSize;
    U32     FileCrc;
    U32     TotolBlockNum;
    U32     CurrValidBlockNum;
    U32     DstFlashBaseAddr;
    U32     UpgradeTestRunTime;
    U8      StaUpgradeStatus;
	U8		OnlineStatus;
    U8      Res[2];
}SLAVE_UPGRADE_FILEINFO_t;

typedef struct
{
    S16  LastBlockSeq;
    U16  TemBlockNum;
    S16  FirstBlockSeq;
    S16  Block4KSeq;
    U8   imageblock0[400];
}T_UpgradeStaWriteFlashVar;

extern U8 FileUpgradeData[UPGRADE_STA_DATA_SIZE * 1024];
extern SLAVE_UPGRADE_FILEINFO_t SlaveUpgradeFileInfo;

void upgrade_sta_start_proc(UPGRADE_START_IND_t *pUpgradeStartInd);
void upgrade_sta_status_query_proc(UPGRADE_STATUS_QUERY_IND_t *pUpgradeQueryInd);
void upgrade_sta_station_info_proc(STATION_INFO_QUERY_IND_t *pUpgradeStationInfoInd);
void upgrade_sta_file_trans_proc(FILE_TRANS_IND_t *pFileTransInd);
void upgrade_sta_stop_proc(uint32_t UpgradeID);
void upgrade_sta_perform_proc(UPGRADE_PERFORM_IND_t *pUpgradePerformInd);
void upgrade_sta_check_first_init(void);
void upgrade_sta_chage_run_image(void);
void upgrade_sta_clean_ram_data_temp(void);
void upgrade_sta_clean_image_reserve(void);
void upgrade_sta_flash_to_ram(S16 Seq, U16 BlockSize, U8* Data, U16 DataLen);
void upgrade_sta_write_last_data(U16 BlockSize);
void upgrade_sta_reset_run_image(void);
#endif


#endif 

