/*
 * Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        sch_demo.c
 * Purpose:     demo procedure for using
 * History:
 */
#include "ZCsystem.h"
#include "flash.h"
#include "wdt.h"
#include "app_exstate_verify.h"
#include "app.h"
#include "gpio.h"
#include "ProtocolProc.h"
#include "app_gw3762.h"
#include "DatalinkTask.h"
#include "SchTask.h"
#include "dl_mgm_msg.h"
#include "app_dltpro.h"
#include "printf_zc.h"
#include "Version.h"
#include "netnnib.h"
#include "app_clock_sync_cco.h"
#include "app_data_freeze_cco.h"
#include "app_led_sta.h"
U8  DeviceTypeDEF = 0;


U8 ModuleID[11];

//extern void phy_rx_start_arg(uint32_t snr_esti);
ostimer_t wdt_timer;

#if defined(STATIC_MASTER)
uint8_t sysflag=FALSE;
uint8_t whitelistflag = FALSE;

uint8_t CCOImageWriteFlag = FALSE;
uint8_t CcoUpgrdCtrlInfoWrFlag = FALSE;
U32 reboot_cnt = 0;

#endif
#if defined(STATIC_NODE)
uint8_t staflag = FALSE;


uint8_t UpgradeStaFlag = FALSE;
uint8_t check_upgrade_info = FALSE;
uint8_t reset_run_image = FALSE;
uint8_t ImageWriteFlag = FALSE;

#endif

DEFAULT_SETTING_INFO_t DefSetInfo; 
DEFAULT_WRITE_FLAG_t	DefwriteFg;

extern uint32_t  DstImageBaseAddr;
extern uint32_t  ImageFileSize;
board_cfg_t BoardCfg_t;
OUT_MF_VERSION_t OutVerFromBin;

AppExtInfo_t app_ext_info;

void execp_reoot()
{
	static U32 repair_cnt = 0;
	if( GetNodeState() != e_NODE_ON_LINE )
	{
		repair_cnt++;
	}
	else
	{
		repair_cnt=0;
	}
	
    if(repair_cnt >= 60*60 && getHplcTestMode() == NORM)
	{
		//sys_panic("------------repair_cnt : %d s--------reboot-------------------------\n",repair_cnt*2);
		repair_cnt = 0;
        if(getHplcTestMode() != RD_CTRL  || getHplcTestMode() != MONITOR)
        {
			//printf_s("reboot but no");
			printf_s("reboot\n");
            app_reboot(500);
		}
	}
}
U8 Wdt_flag = FALSE;
void wdt_timerCB(struct ostimer_s *ostimer, void *arg)
{
static U8 cnt;
#if defined(STATIC_NODE)
	//extern sfo_handle_t HSFO;


	if(phy_get_sfo(SFO_DIR_TX)>50||phy_get_sfo(SFO_DIR_TX)<-50
		||phy_get_sfo(SFO_DIR_RX)>50||phy_get_sfo(SFO_DIR_RX)<-50)
	{
        if( GetNodeState() != e_NODE_ON_LINE && getHplcTestMode() == NORM)
		{
			//zc_phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
			//HPLC.sfo_is_done=0;
			//HPLC.snid = 0;
			app_printf("waring:sfo =%d,%d\n",phy_get_sfo(SFO_DIR_TX),phy_get_sfo(SFO_DIR_RX));
		}
	}
#endif


//    app_printf("-----wdt_feed--cnt = %d---\n", cnt);
	if(Wdt_flag)
	{
		cnt = 0;
		Wdt_flag = FALSE;
		wdt_feed();
	}
	else
	{
		cnt++;
		if(cnt >= 3)
			{
			    extern void show_record_mem_list(xsh_t *xsh,uint32_t tick);
			    show_record_mem_list(&g_vsh, get_phy_tick_time());
                extern void show_mg_record(tty_t *term);
                show_mg_record(g_vsh.term);
			}
	}

}



void SysTem_mange_Init()
{

    mutex_init(&task_work_mutex, "net mutex");      //防止链路层任务未初始化完成，在APP中调用nnib_ctrl导致程序崩溃
    mutex_init(&work_mutex, "plc mutex");

	//init wtd 
    timer_init(&wdt_timer,
               2000,
               2000,
               TMR_PERIODIC,
               wdt_timerCB,
               NULL,
               "wdt_timer");
    if(wdt_timer.fn!=NULL)
       timer_start(&wdt_timer);
	else
	   sys_panic("<wdt_timer fail!> %s: %d\n");
}



void SchTaskProcess()
{
#if defined(ZC3750CJQ2)
	extern void infrared_task_init();
	infrared_task_init();
    os_sleep(30*100);
#endif
    //rand();
#if defined(ZC3750STA)
	os_sleep(2*60*100);
#endif

	while(1)
    {
        mutex_pend(&mutexSch_t, 60*100);
        if(getHplcTestMode() != NORM )
        {
            continue;
        }

#if defined(ZC3750STA)
        {

	
            if(DefaultSettingFlag == TRUE)
            {
                DefaultSettingFlag = FALSE;
                SetDefaultInfo();
            }

            if(staflag == TRUE)
            {
//                hplc_hrf_phy_reset();
                staflag = FALSE;
                WriteMeterInfo();
                /*if(FLASH_OK == WriteMeterInfo())
                {
                    staflag = FALSE;
                }*/
//                hplc_hrf_phy_reset();
            }

            if(ImageWriteFlag == TRUE)
            {
//                app_printf("start ImageWriteFlag\n");
//                hplc_hrf_phy_reset();
                ImageWriteFlag = FALSE;
//                WriteUpgradeImage();
//                hplc_hrf_phy_reset();
            }

            if(UpgradeStaFlag == TRUE)
            {
//                hplc_hrf_phy_reset();
                UpgradeStaFlag = FALSE;
                SaveUpgradeStatusInfo();
//                hplc_hrf_phy_reset();
            }
            if(check_upgrade_info == TRUE && ImageWriteFlag == FALSE)
            {
                check_upgrade_info = FALSE;
                upgrade_sta_chage_run_image();
            }
            if(reset_run_image == TRUE)
            {
                reset_run_image = FALSE;
                upgrade_sta_reset_run_image();
            }

        }
#elif defined(ZC3750CJQ2)
        {
            if(DefaultSettingFlag == TRUE)
            {
                DefaultSettingFlag = FALSE;
                SetDefaultInfo();
            }

            if(ImageWriteFlag == TRUE)
            {
                app_printf("start ImageWriteFlag\n");
//                hplc_hrf_phy_reset();
                ImageWriteFlag = FALSE;
                //WriteUpgradeImage();
//                hplc_hrf_phy_reset();
            }

            if(UpgradeStaFlag == TRUE)
            {
//                hplc_hrf_phy_reset();
                UpgradeStaFlag = FALSE;
                SaveUpgradeStatusInfo();
//                hplc_hrf_phy_reset();
            }
            if(check_upgrade_info == TRUE && ImageWriteFlag == FALSE)
            {
                check_upgrade_info = FALSE;
                upgrade_sta_chage_run_image();
            }
            if(reset_run_image == TRUE)
            {
                reset_run_image = FALSE;
                upgrade_sta_reset_run_image();
            }
            if(staflag == TRUE)
            {
                //mutex_pend(Mutex_CJQ_485,0);
//                hplc_hrf_phy_reset();
                staflag = FALSE;
                //dump_buf((U8*)&CollectInfo_st, sizeof(COLLECT_INFO_ST));
                //mutex_pend(&mutexSch_t, 100);
                CollectInfo_st.Cs = CJQ_Flash_Check((U8*)&CollectInfo_st, sizeof(COLLECT_INFO_ST) - 1);
                //mutex_pend(&mutexSch_t, 500);
                //dump_buf((U8*)&CollectInfo_st, sizeof(COLLECT_INFO_ST));
                //mutex_pend(mutexSch_t, 100);
                if(FLASH_OK == zc_flash_write(FLASH, STA_INFO, (U32)&CollectInfo_st, sizeof(COLLECT_INFO_ST)))
				{
					app_printf("CJQflash_write OK Cs = 0x%02x!\n", CollectInfo_st.Cs);
				}
                else
				{
					app_printf("CJQflash_write ERROR!\n");
				}
#if 0
                COLLECT_INFO_ST CollectInfo_st1;
                //mutex_pend(&mutexSch_t, 100);
                zc_flash_read(FLASH, STA_INFO, (U32*)&CollectInfo_st1, sizeof(COLLECT_INFO_ST));

                if(0 == memcmp(&CollectInfo_st1, &CollectInfo_st, sizeof(COLLECT_INFO_ST)))
                {
                    macs_printf("0CJQflash_write OK Cs = 0x%02x!\n",CollectInfo_st.Cs);
                    mutex_pend(&mutexSch_t, 100);
                    dump_buf((U8*)&CollectInfo_st, sizeof(COLLECT_INFO_ST));
                }
                else
                {
                    macs_printf("1CJQflash_write err CollectInfo_st.Cs = 0x%02x, CollectInfo_st1.Cs = 0x%02x!\n",CollectInfo_st.Cs,CollectInfo_st1.Cs);
                }
#endif

//                hplc_hrf_phy_reset();

                //mutex_post(Mutex_CJQ_485);
            }
        }
#elif defined(STATIC_MASTER)
        {

            if(DefaultSettingFlag == TRUE)
            {
                DefaultSettingFlag = FALSE;
                SetDefaultInfo();
            }

            SaveInfo(&sysflag,&whitelistflag);

            if(CCOImageWriteFlag == TRUE)
            {
//                hplc_hrf_phy_reset();
                CCOImageWriteFlag = FALSE;
                CcoWriteImage();
//                hplc_hrf_phy_reset();
            }

            if(CcoUpgrdCtrlInfoWrFlag == TRUE)
            {
//                hplc_hrf_phy_reset();
                CcoUpgrdCtrlInfoWrFlag = FALSE;
                CcoUpgrdCtrlInfoWrite();
//                hplc_hrf_phy_reset();
            }
			if(CCOCurveCfgFlag == TRUE)
            {
                CCOCurveCfgFlag = FALSE;
                cco_save_curve_cfg();
            }
        }
#endif

        os_sleep(100);
    }

}
void SchTaskInit()
{
    MutexSchCreate();
    if ((PidSchTask = task_create(SchTaskProcess,
                                  NULL,
                                  &SchTaskWork[TASK_SCH_STK_SZ - 1],
                                  TASK_PRIO_SCH,
                                  "Schtask",
                                  &SchTaskWork[0],
                                  TASK_SCH_STK_SZ)) < 0)
        sys_panic("<SchTaskInit panic> %s: %d\n", __func__, __LINE__);
}


void MutexSchCreate()
{
    mutex_init(&mutexSch_t, "mutexSch_t");
    debug_printf(&dty, DEBUG_PHY, "mutexSch_t-init: %d\n", mutex_pend(&mutexSch_t, 0));



    SysTem_mange_Init();
}

#if defined(STATIC_MASTER)
void SaveInfo(uint8_t *sysinfo_flag ,uint8_t *whitelist_flag)
{
    CCO_SYSINFO_t cco_info_t;
    U16  usMeterIdx;
    U16  len = 0;
    U16  cs16 = 0;

	if(*sysinfo_flag == TRUE)
    {
		*sysinfo_flag=FALSE;

		if(TRUE != ReadFreqSetInfo((void*)&cco_info_t))
		{
			return;
		}


		cco_info_t.formatseq         =   nnib.FormationSeqNumber;
        __memcpy(&cco_info_t.flashinfo_t, &FlashInfo_t, sizeof(FLASHINFO_t));

		if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
		{
			cco_info_t.clkMainCycle = clock_maintenance_cycle;	//时钟维护周期
		}

        cco_info_t.cs = check_sum((U8 *)&cco_info_t, sizeof(CCO_SYSINFO_t) - 1);
        app_printf("flash_w sysinfo ");

        if(FLASH_OK == zc_flash_write(FLASH, CCO_SYSINFO, (U32)&cco_info_t, sizeof(CCO_SYSINFO_t)))
		{
			app_printf("OK!cs=%02x,JZQProtocol=%02x\n", cco_info_t.cs,cco_info_t.flashinfo_t.scJZQProtocol);
		}
        else
		{
			app_printf("ERR!\n");
		}


    }

    if(*whitelist_flag == TRUE)
    {
        *whitelist_flag = FALSE;
        memset(&FlashWhiteList, 0x00, sizeof(FlashWhiteList));
        //recordtime("whitelist_flag == TRUE prve",get_phy_tick_time());
        WHLPTST
        for(usMeterIdx = 0; usMeterIdx < MAX_WHITELIST_NUM; usMeterIdx++)
        {
        	
            __memcpy(&FlashWhiteList.WhiteList[usMeterIdx].MacAddr,&WhiteMacAddrList[usMeterIdx].MacAddr,6);
            __memcpy(&FlashWhiteList.WhiteList[usMeterIdx].MeterInfo_t,&WhiteMacAddrList[usMeterIdx].MeterInfo_t,2);
            __memcpy(&FlashWhiteList.WhiteList[usMeterIdx].ModuleID,&WhiteMacAddrList[usMeterIdx].ModuleID,12);
        }
        WHLPTED
        
        FlashWhiteList.WhiteListNum = FlashInfo_t.usJZQRecordNum;
        cs16 = ResetCRC16();
        len = sizeof(FlashWhiteList)-2;
        FlashWhiteList.CS16 = CalCRC16(cs16,(U8 *)&FlashWhiteList, 0 , len);
        app_printf("flash_w whitelist ");
        if(FLASH_OK == zc_flash_write(FLASH, CCO_WHITE_LIST_ADDR, (U32)&FlashWhiteList, sizeof(FlashWhiteList)))
        {
            app_printf("OK!\n");
        }
        else
        {
            app_printf("ERR!\n");
        }
        //recordtime("whitelist_flag == TRUE prve",get_phy_tick_time());
    }
}
void setversion(U8 *vender,U8 *chipversion)
{
	__memcpy(&FlashInfo_t.ManuFactor_t,&DefSetInfo.OutMFVersion,sizeof(DCR_MANUFACTURER_t));
	
	sysflag=TRUE;
}

void Readsysinfo(void)
{
	CCO_SYSINFO_t cco_info_t;
    CCO_SYSINFO_t null_cco_info_t;
	U16 i=0;
	U8 null_addr[6]={0,0,0,0,0,0};
	memset((U8 *)&null_cco_info_t, 0,sizeof(CCO_SYSINFO_t));
    app_printf("flash_r CCO_SYSINFO ");
    if(FLASH_OK == zc_flash_read(FLASH, CCO_SYSINFO, (U32 *)&cco_info_t, sizeof(CCO_SYSINFO_t)))
	{
		app_printf("OK!cs=%02x,JZQProtocol=%02x\n", cco_info_t.cs, cco_info_t.flashinfo_t.scJZQProtocol);
	}
    else
	{
		app_printf("ERR!\n");
	}
//    dump_buf((U8 *)&cco_info_t,sizeof(CCO_SYSINFO_t));

//    app_printf("Readsysinfo : cco_info_t.cs = %02x, check_sum = %02x\n", cco_info_t.cs, check_sum((U8 *)&cco_info_t, sizeof(FLASHINFO_t) + 2));
	
    if(cco_info_t.cs == check_sum((U8 *)&cco_info_t, sizeof(CCO_SYSINFO_t) - 1)
                  /*&& cco_info_t.cs != 0x00*/ && cco_info_t.flashinfo_t.scJZQProtocol != 0x00)
	{
		//SystemState_e=cco_info_t.systemstate_t;
		//__memcpy(&RouterInfo_t,&cco_info_t.routinfo_t,sizeof(ROUTER_INFO_t));
        nnib.FormationSeqNumber = cco_info_t.formatseq;
		nnib.Resettimes = cco_info_t.resettimes;

		if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
		{
			clock_maintenance_cycle = cco_info_t.clkMainCycle;	//时钟维护周期
		}
		__memcpy(&FlashInfo_t,&cco_info_t.flashinfo_t,sizeof(FLASHINFO_t));

		if(FlashInfo_t.ManuFactor_t.ucVendorCode[0]<0x30||
			FlashInfo_t.ManuFactor_t.ucVendorCode[0]>0x7a||
			FlashInfo_t.ManuFactor_t.ucVendorCode[1]<0x30||
			FlashInfo_t.ManuFactor_t.ucVendorCode[1]>0x7a)
		{
			setversion(NULL,NULL);
			sysflag=TRUE;
            mutex_post(&mutexSch_t);
		}

		if(FlashInfo_t.scJZQProtocol!=e_GD2016_MSG&&FlashInfo_t.scJZQProtocol!=e_GW13762_MSG)
		{
#if defined(STD_GD)
		FlashInfo_t.scJZQProtocol = e_GD2016_MSG;
#elif defined(STD_2016)
		FlashInfo_t.scJZQProtocol = e_GW13762_MSG;
#endif
		}
        memset(&FlashWhiteList, 0x00, sizeof(FlashWhiteList));
        app_printf("flash_r CCO_SYSINFO ");
        if(FLASH_OK == zc_flash_read(FLASH, CCO_WHITE_LIST_ADDR, (U32 *)&FlashWhiteList, sizeof(FlashWhiteList)))
		{
			app_printf("OK!\n");
		}
		else
		{
			app_printf("ERR!\n");
		}
        U16  len = 0;
        U16  cs16 = 0;
        U16  usMeterIdx;
        
        memset(&WhiteMacAddrList, 0x00, sizeof(WhiteMacAddrList));
        cs16 = ResetCRC16();
        len = sizeof(FlashWhiteList) - 2;
        cs16 = CalCRC16(cs16,(U8 *)&FlashWhiteList, 0 , len);
        if(cs16 == FlashWhiteList.CS16)
        {
            WHLPTST
        	for(usMeterIdx = 0; usMeterIdx < MAX_WHITELIST_NUM; usMeterIdx++)
            {
            	if(0!=memcmp(FlashWhiteList.WhiteList[usMeterIdx].MacAddr,null_addr,6))
                {   
                    __memcpy(&WhiteMacAddrList[usMeterIdx].MacAddr,&FlashWhiteList.WhiteList[usMeterIdx].MacAddr,6);
                    __memcpy(&WhiteMacAddrList[usMeterIdx].MeterInfo_t,&FlashWhiteList.WhiteList[usMeterIdx].MeterInfo_t,2);
            		__memcpy(&WhiteMacAddrList[usMeterIdx].ModuleID,&FlashWhiteList.WhiteList[usMeterIdx].ModuleID,12);
                }
            }
            WHLPTED
            
            FlashInfo_t.usJZQRecordNum = FlashWhiteList.WhiteListNum;
            app_printf("JZQRecordNum = %d\n",FlashInfo_t.usJZQRecordNum);
            renew_all_mode_id_state();
        }
        else
        {
            FlashInfo_t.usJZQRecordNum = 0;
            goto cs_error;
        }
	}
	else
	{
		cs_error:
		app_printf("err flash_r cs=%02x!,JZQProtocol=%d\n",cco_info_t.cs,FlashInfo_t.scJZQProtocol);
		i=0;
		do
		{
			memset(&WhiteMacAddrList[i],0,sizeof(WHITE_MAC_LIST_t));
		}while(++i<MAX_WHITELIST_NUM);
		sysflag=TRUE;
		whitelistflag = TRUE;	
		FlashInfo_t.usJZQRecordNum = 0;
		//FlashInfo_t.FreqBand = 2;
        //FlashInfo_t.tgaindig = 0;
        //FlashInfo_t.tgainana = 0;
		setversion(NULL,NULL);
#if defined(STD_GD)
		FlashInfo_t.scJZQProtocol = e_GD2016_MSG;
#elif defined(STD_2016)
		FlashInfo_t.scJZQProtocol = e_GW13762_MSG;
#endif
        mutex_post(&mutexSch_t);
	}
	i=0;
	do
	{
		//app_printf("i : %d\n",i);
		memset(WhiteMacAddrList[i].CnmAddr,0,6);
		WhiteMacAddrList[i].waterRfTh = 0;
		WhiteMacAddrList[i].SetResult = 0;
	}while(++i<MAX_WHITELIST_NUM);

       memset((U8*)&AppGw3762Afn10F4State, 0x00, sizeof(AppGw3762Afn10F4State));
       AppGw3762Afn10F4State.SlaveNodeCount = FlashInfo_t.usJZQRecordNum;
       //AppGw3762Afn10F4State.CnmRate = 500*1024;  // 500 kbps
#if 1
	//FlashInfo_t.scJZQProtocol = e_GW13762_MSG;
	setversion(NULL, NULL);
#endif
	if(FlashInfo_t.ucSlaveReadTimeout<3)FlashInfo_t.ucSlaveReadTimeout=60;
    

}




void CcoWriteImage(void)
{
    uint32_t  state;
    if(CcoUpgrdCtrlInfoWrFlag == TRUE)
    {
        if(DstImageBaseAddr)//不存储非ZC的bin
        {
            state= zc_flash_write(FLASH, (U32)DstImageBaseAddr , (U32)FileUpgradeData, CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize);
            app_printf("flash_w sta image ");
            if(FLASH_OK==state)
			{
				app_printf("OK!\n");
			}
			else
			{
				app_printf("ERR!\n");
			}
        }
    }
    else
    { 
        state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
        if(state == FLASH_OK)
        {
            if(BoardCfg_t.active == 1)  // now runing is iamge0
            {
                DstImageBaseAddr = IMAGE1_ADDR;
            }
            else if(BoardCfg_t.active == 2) // now runing is iamge1
            {
                 DstImageBaseAddr = IMAGE0_ADDR;
            }

            state= zc_flash_write(FLASH, (U32)DstImageBaseAddr , (U32)FileUpgradeData, CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize);
            app_printf("flash_w cco image ");
            if(FLASH_OK==state)
			{
				app_printf("OK!\n");
			}
			else
			{
				app_printf("ERR!\n");
			}
            if(FLASH_OK==state)
            {
                if(BoardCfg_t.mode == 1)  // IMAGE_SINGLE_MODE
                 {
                     BoardCfg_t.mode = 2;   // IMAGE_DUAL_MODE
                 }
              
                 if(BoardCfg_t.active == 1)  // now runing is iamge0
                 {
                     BoardCfg_t.active = 2;
                 }
                 else if(BoardCfg_t.active == 2) // now runing is iamge1
                 {
                      BoardCfg_t.active = 1;
                 }
                 ReadBinOutVersion(FileUpgradeData);
                 os_sleep(100);
                 state=zc_flash_write(FLASH, (U32)BOARD_CFG  ,(U32)&BoardCfg_t ,sizeof(board_cfg_t));
                 
				 app_ext_info_write_after_upgrade();
                 os_sleep(10);                 
                 AppGw3762Afn10F4State.WorkSwitchStates =  OTHER_SWITCH_FLAG;//释放升级状态
                 AppGw3762Afn10F4State.RunStatusWorking = 0;
               	 app_reboot(500);
            }
        }
    }
}

void CcoLoadSlaveImage(void)
{
    DstImageBaseAddr = IMAGE1_ADDR+IMAGE_RESERVE;
    app_printf("flash_r image ");
    if(FLASH_OK == zc_flash_read(FLASH, DstImageBaseAddr, (U32 *)&FileUpgradeData, CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSize))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	}
}

void CcoLoadSlaveImage1(void)
	
{
    DstImageBaseAddr = IMAGE1_ADDR;//+IMAGE_RESERVE;
    app_printf("flash_r image1 ");
    if(FLASH_OK == zc_flash_read(FLASH, DstImageBaseAddr, (U32 *)&FileUpgradeData, sizeof(FileUpgradeData)))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	}
}


extern CCO_UPGRADE_CTRL_INFO_t   CcoUpgrdCtrlInfo;
void CcoUpgrdCtrlInfoWrite(void)
{
    app_printf("flash_w upgrade ctrl ");
    if(FLASH_OK == zc_flash_write(FLASH, CCO_UPGRDCTRL, (U32)&CcoUpgrdCtrlInfo, sizeof(CcoUpgrdCtrlInfo)))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	} 
}

void CcoUpgrdCtrlInfoRead(void)
{
    app_printf("flash_r upgrade ctrl ");
    if(FLASH_OK == zc_flash_read(FLASH, CCO_UPGRDCTRL, (U32 *)&CcoUpgrdCtrlInfo, sizeof(CcoUpgrdCtrlInfo)))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	}
}

#endif
U8 WriteDefSetInfo(DEFAULT_SETTING_INFO_t* pDefaultinfo)
{
	uint32_t  state;
    state= zc_flash_write(FLASH, (U32)DEFAULTSETTING , (U32)pDefaultinfo, sizeof(DEFAULT_SETTING_INFO_t));
    app_printf("flash_w DefSetInfo ");
	if(state == FLASH_OK)
    {
        app_printf("OK!\n") ;
		return TRUE;
    }
    else
    {
        app_printf("ERR!\n");
		return FALSE;
    }
}

U8 WriteFreqSetInfo(void* info)
{
	uint32_t  state;
	
#if defined(STATIC_MASTER)
	state= zc_flash_write(FLASH, (U32)CCO_SYSINFO , (U32)info, sizeof(CCO_SYSINFO_t));
#elif defined(ZC3750STA)
	state= zc_flash_write(FLASH, (U32)STA_INFO , (U32)info, sizeof(DEVICE_PIB_t));
#elif defined(ZC3750CJQ2)
	state= zc_flash_write(FLASH, (U32)STA_INFO , (U32)info, sizeof(COLLECT_INFO_ST));
#endif
    
    app_printf("flash_w FreqSetInfo ");
	if(state == FLASH_OK)
    {
        app_printf("OK!\n") ;
		return TRUE;
    }
    else
    {
        app_printf("ERR!\n");
		return FALSE;
    }
}

uint32_t ReadDefSetInfo(DEFAULT_SETTING_INFO_t* pDefaultinfo)
{
	uint32_t  state;
    
    app_printf("flash_r DefSetInfo ");
    state= zc_flash_read(FLASH, (U32)DEFAULTSETTING , (U32 *)pDefaultinfo, sizeof(DEFAULT_SETTING_INFO_t));
	if(state == FLASH_OK)
    {
        app_printf("OK!\n");
		return TRUE;
    }
    else
    {
        app_printf("ERR!\n");
		return FALSE;
    }
}

uint32_t ReadFreqSetInfo(void* info)
{
	uint32_t  state;
	
#if defined(STATIC_MASTER)
	state= zc_flash_read(FLASH, (U32)CCO_SYSINFO , (U32*)info, sizeof(CCO_SYSINFO_t));
#elif defined(ZC3750STA)
	state= zc_flash_read(FLASH, (U32)STA_INFO , (U32*)info, sizeof(DEVICE_PIB_t));
#elif defined(ZC3750CJQ2)
	state= zc_flash_read(FLASH, (U32)STA_INFO , (U32*)info, sizeof(COLLECT_INFO_ST));
#endif

	app_printf("flash_r FreqSetInfo ");
	if(state == FLASH_OK)
	{
		app_printf("OK!\n");
		return TRUE;
	}
	else
	{
		app_printf("ERR!\n");
		return FALSE;
	}
	return state;
}

void SetFunctionSwitchDef(U8 provice_code)
{
	app_printf("SetFunctionSwitchDef !\n");
	memset((INT8U *)&app_ext_info.func_switch, 0U, sizeof(app_ext_info.func_switch));
	app_ext_info.func_switch.UseMode			= 0;
	app_ext_info.func_switch.DebugeMode			= 0;
	app_ext_info.func_switch.NoiseDetectSWC		= 0;
	app_ext_info.func_switch.WhitelistSWC		= 1;
	app_ext_info.func_switch.UpgradeMode		= 0;
	app_ext_info.func_switch.AODVSWC			= 0;
	app_ext_info.func_switch.EventReportSWC		= 1;
	app_ext_info.func_switch.ModuleIDGetSWC		= 1;
	app_ext_info.func_switch.PhaseSWC			= 1;
	app_ext_info.func_switch.IndentifySWC		= 0;
	app_ext_info.func_switch.DataAddrFilterSWC	= 0;
	app_ext_info.func_switch.NetSenseSWC		= 0;
	app_ext_info.func_switch.SignalQualitySWC	= 0;
	app_ext_info.func_switch.SetBandSWC			= 1;
	app_ext_info.func_switch.SwPhase			= 1;
if(PROVINCE_HENAN == app_ext_info.province_code)
{	app_ext_info.func_switch.oop20EvetSWC		= 0;
	app_ext_info.func_switch.oop20BaudSWC		= 0;	
}
else
{
	app_ext_info.func_switch.oop20EvetSWC		= 1;
	app_ext_info.func_switch.oop20BaudSWC		= 1;
}
	app_ext_info.func_switch.JZQBaudSWC			= 0;
if(provice_code == PROVINCE_JIANGSU)
	app_ext_info.func_switch.MinCollectSWC		= 1;
else
    app_ext_info.func_switch.MinCollectSWC		= 0;
	app_ext_info.func_switch.IDInfoGetModeSWC	= 0;
	app_ext_info.func_switch.TransModeSWC 	    = 1;
    app_ext_info.func_switch.AddrShiftSWC 	    = 1;
}
void SetParameterCFG(U8 *data)
{
	PARAMETER_CFG_t *pParameterCFG = (PARAMETER_CFG_t *)data;
	app_printf("SetParameterCFG !\n");
    #if defined(STATIC_MASTER)
	if(data == NULL)
	{
		memset((U8 *)&app_ext_info.param_cfg, 0, sizeof(app_ext_info.param_cfg));

		app_ext_info.param_cfg.ConcurrencySize = CONCURRENCYSIZE;    //
		app_ext_info.param_cfg.ReTranmitMaxNum = 4;    //
		app_ext_info.param_cfg.ReTranmitMaxTime = 30;    //(40*100ms)
		app_ext_info.param_cfg.BroadcastConRMNum = 10;
		app_ext_info.param_cfg.AllBroadcastType = e_PROXY_BROADCAST_FREAM_NOACK;
		app_printf("SetParameterCFG NULL! ConcurrencySize %d;ReTranmitMaxNum %d;ReTranmitMaxTime %d;BroadcastConRMNum %d;BroadcastConRMNum %d!\n",
			app_ext_info.param_cfg.ConcurrencySize, app_ext_info.param_cfg.ReTranmitMaxNum
			, app_ext_info.param_cfg.ReTranmitMaxTime, app_ext_info.param_cfg.BroadcastConRMNum
			, app_ext_info.param_cfg.AllBroadcastType);
	}
	else
	{
		app_ext_info.param_cfg.ConcurrencySize = SET_PARAMETER(pParameterCFG->ConcurrencySize, 5, MAX_WIN_SIZE, 10);    //data，最小值，最大值，默认值
		app_ext_info.param_cfg.ReTranmitMaxNum = SET_PARAMETER(pParameterCFG->ReTranmitMaxNum, 1, 12, 6);    //6
		app_ext_info.param_cfg.ReTranmitMaxTime = SET_PARAMETER(pParameterCFG->ReTranmitMaxTime, 10, 100, 40);    //(40*100ms)
		app_ext_info.param_cfg.BroadcastConRMNum = SET_PARAMETER(pParameterCFG->BroadcastConRMNum, 0, 20, 1);
		app_ext_info.param_cfg.AllBroadcastType = SET_PARAMETER(pParameterCFG->AllBroadcastType, 0, 4, 1);
		app_printf("SetParameterCFG FLASH! ConcurrencySize %d;ReTranmitMaxNum %d;ReTranmitMaxTime %d;BroadcastConRMNum %d;BroadcastConRMNum %d!\n",
			app_ext_info.param_cfg.ConcurrencySize, app_ext_info.param_cfg.ReTranmitMaxNum
			, app_ext_info.param_cfg.ReTranmitMaxTime, app_ext_info.param_cfg.BroadcastConRMNum
			, app_ext_info.param_cfg.AllBroadcastType);
	}
    #else
    if(data == NULL)
	{
		memset((U8*)&app_ext_info.param_cfg,0,sizeof(app_ext_info.param_cfg));
		
		app_ext_info.param_cfg.ConcurrencySize = 0; //
		app_ext_info.param_cfg.ReTranmitMaxNum        = 0;    //
		app_ext_info.param_cfg.ReTranmitMaxTime       = 0;    //
		app_ext_info.param_cfg.BroadcastConRMNum      = 0;
		app_printf("SetParameterCFG NULL! ConcurrencySize %d;ReTranmitMaxNum %d;ReTranmitMaxTime %d;BroadcastConRMNum %d!\n",
			app_ext_info.param_cfg.ConcurrencySize, app_ext_info.param_cfg.ReTranmitMaxNum
			, app_ext_info.param_cfg.ReTranmitMaxTime, app_ext_info.param_cfg.BroadcastConRMNum);
	}
	else
	{
		app_ext_info.param_cfg.ConcurrencySize        = SET_PARAMETER(pParameterCFG->ConcurrencySize,0,0x99,0);    //data，最小值，最大值，默认值
		app_ext_info.param_cfg.ReTranmitMaxNum        = SET_PARAMETER(pParameterCFG->ReTranmitMaxNum,0,0x99,0);    //6
		app_ext_info.param_cfg.ReTranmitMaxTime       = SET_PARAMETER(pParameterCFG->ReTranmitMaxTime,0,0x99,0);;    //
		app_ext_info.param_cfg.BroadcastConRMNum      = SET_PARAMETER(pParameterCFG->BroadcastConRMNum,0,0x99,0);
		app_printf("SetParameterCFG FLASH! ConcurrencySize %d;ReTranmitMaxNum %d;ReTranmitMaxTime %d;BroadcastConRMNum %d!\n",
			app_ext_info.param_cfg.ConcurrencySize, app_ext_info.param_cfg.ReTranmitMaxNum
			, app_ext_info.param_cfg.ReTranmitMaxTime,app_ext_info.param_cfg.BroadcastConRMNum);
	}
    #endif
}
//修改RAM中的外部版本不存储
void USEoutversion(U8 *vender,U8 *chipversion,U8 *date,U8 *version)
{
	if(vender==NULL||chipversion==NULL)
	{
		DefSetInfo.OutMFVersion.ucVendorCode[0]=Vender2;
		DefSetInfo.OutMFVersion.ucVendorCode[1]=Vender1;
#if defined(STATIC_MASTER)
        DefSetInfo.OutMFVersion.ucChipCode[0]=ZC3951CCO_chip2;
	    DefSetInfo.OutMFVersion.ucChipCode[1]=ZC3951CCO_chip1;
#elif defined(ZC3750STA)
  		DefSetInfo.OutMFVersion.ucChipCode[0]=ZC3750STA_chip2;
	    DefSetInfo.OutMFVersion.ucChipCode[1]=ZC3750STA_chip1;
#elif defined(ZC3750CJQ2)
		DefSetInfo.OutMFVersion.ucChipCode[0]=ZC3750CJQ2_chip2;
	    DefSetInfo.OutMFVersion.ucChipCode[1]=ZC3750CJQ2_chip1;
#endif
		
	}
	else
	{
		DefSetInfo.OutMFVersion.ucVendorCode[0]=*vender;
		DefSetInfo.OutMFVersion.ucVendorCode[1]=*(vender+1);
		DefSetInfo.OutMFVersion.ucChipCode[0]=*chipversion;
		DefSetInfo.OutMFVersion.ucChipCode[1]=*(chipversion+1);
	}
	if(date == NULL||version == NULL)
	{
		DefSetInfo.OutMFVersion.ucDay=U8TOBCD(Date_D);//(Date_D/10<<4)|(Date_D%10);
		DefSetInfo.OutMFVersion.ucMonth=U8TOBCD(Date_M);//(Date_M/10<<4)|(Date_M%10);
		DefSetInfo.OutMFVersion.ucYear=U8TOBCD(Date_Y);//(Date_Y/10<<4)|(Date_Y%10);
#if defined(STATIC_MASTER)
        DefSetInfo.OutMFVersion.ucVersionNum[0]=ZC3951CCO_ver2;
	    DefSetInfo.OutMFVersion.ucVersionNum[1]=ZC3951CCO_ver1;
#elif defined(ZC3750STA)
  		DefSetInfo.OutMFVersion.ucVersionNum[0]=ZC3750STA_ver2;
	    DefSetInfo.OutMFVersion.ucVersionNum[1]=ZC3750STA_ver1;
#elif defined(ZC3750CJQ2)
		DefSetInfo.OutMFVersion.ucVersionNum[0]=ZC3750CJQ2_ver2;
	    DefSetInfo.OutMFVersion.ucVersionNum[1]=ZC3750CJQ2_ver1;
#endif
	}
	else
	{
		DefSetInfo.OutMFVersion.ucDay= *date;
		DefSetInfo.OutMFVersion.ucMonth=*(date+1);
		DefSetInfo.OutMFVersion.ucYear=*(date+2);
		DefSetInfo.OutMFVersion.ucVersionNum[0]=*version;
		DefSetInfo.OutMFVersion.ucVersionNum[1]=*(version+1);
	}
}
//修改外部版本并且存入FLASH
U8 setoutversion(U8 *vender,U8 *chipversion,U8 *date,U8 *version)
{
	if(vender==NULL||chipversion==NULL)
	{
		DefSetInfo.OutMFVersion.ucVendorCode[0]=Vender2;
		DefSetInfo.OutMFVersion.ucVendorCode[1]=Vender1;
#if defined(STATIC_MASTER)
        DefSetInfo.OutMFVersion.ucChipCode[0]=ZC3951CCO_chip2;
	    DefSetInfo.OutMFVersion.ucChipCode[1]=ZC3951CCO_chip1;
#elif defined(ZC3750STA)
  		DefSetInfo.OutMFVersion.ucChipCode[0]=ZC3750STA_chip2;
	    DefSetInfo.OutMFVersion.ucChipCode[1]=ZC3750STA_chip1;
#elif defined(ZC3750CJQ2)
		DefSetInfo.OutMFVersion.ucChipCode[0]=ZC3750CJQ2_chip2;
	    DefSetInfo.OutMFVersion.ucChipCode[1]=ZC3750CJQ2_chip1;
#endif
		
	}
	else
	{
		DefSetInfo.OutMFVersion.ucVendorCode[0]=*vender;
		DefSetInfo.OutMFVersion.ucVendorCode[1]=*(vender+1);
		DefSetInfo.OutMFVersion.ucChipCode[0]=*chipversion;
		DefSetInfo.OutMFVersion.ucChipCode[1]=*(chipversion+1);
	}
	if(date == NULL||version == NULL)
	{
		DefSetInfo.OutMFVersion.ucDay=U8TOBCD(Date_D);//(Date_D/10<<4)|(Date_D%10);
		DefSetInfo.OutMFVersion.ucMonth=U8TOBCD(Date_M);//(Date_M/10<<4)|(Date_M%10);
		DefSetInfo.OutMFVersion.ucYear=U8TOBCD(Date_Y);//(Date_Y/10<<4)|(Date_Y%10);
#if defined(STATIC_MASTER)
        DefSetInfo.OutMFVersion.ucVersionNum[0]=ZC3951CCO_ver2;
	    DefSetInfo.OutMFVersion.ucVersionNum[1]=ZC3951CCO_ver1;
#elif defined(ZC3750STA)
  		DefSetInfo.OutMFVersion.ucVersionNum[0]=ZC3750STA_ver2;
	    DefSetInfo.OutMFVersion.ucVersionNum[1]=ZC3750STA_ver1;
#elif defined(ZC3750CJQ2)
		DefSetInfo.OutMFVersion.ucVersionNum[0]=ZC3750CJQ2_ver2;
	    DefSetInfo.OutMFVersion.ucVersionNum[1]=ZC3750CJQ2_ver1;
#endif
	}
	else
	{
		DefSetInfo.OutMFVersion.ucDay= *date;
		DefSetInfo.OutMFVersion.ucMonth=*(date+1);
		DefSetInfo.OutMFVersion.ucYear=*(date+2);
		DefSetInfo.OutMFVersion.ucVersionNum[0]=*version;
		DefSetInfo.OutMFVersion.ucVersionNum[1]=*(version+1);
	}
	DefwriteFg.OMF = TRUE;
	return SetDefaultInfo();
}
void setVersionInfo()
{
#if defined(STD_2016)

	__memcpy(&VersionInfo.SoftVerNum,&DefSetInfo.OutMFVersion.ucVersionNum,2);
	__memcpy(&VersionInfo.ManufactorCode,&DefSetInfo.OutMFVersion.ucVendorCode,2);
	__memcpy(&VersionInfo.ChipVerNum,&DefSetInfo.OutMFVersion.ucChipCode,2);
	U8  Year = DefSetInfo.OutMFVersion.ucYear;
	U8  Month = DefSetInfo.OutMFVersion.ucMonth;
	U8  Day = DefSetInfo.OutMFVersion.ucDay;
	
	VersionInfo.BuildTime = (((BCDTOU8(Year))&0x7F)|(((BCDTOU8(Month))<<7)&0x780)|(((BCDTOU8(Day))<<11)&0xF800));
    VersionInfo.BootVersion = bootversion;
#endif

}

U8 CheckFreqNeedWrite(void)
{
	if(DefwriteFg.FREQ || DefwriteFg.RfChannel)
	{
		return TRUE;
	}
	return FALSE;
}

U8 CheckExtNeedWrite(void)
{
	if(DefwriteFg.FunSWC || DefwriteFg.ParaCFG)
	{
		return TRUE;
	}
	return FALSE;
}

U8 CheckDefNeedWrite(void)
{
	if(DefwriteFg.MID || DefwriteFg.PhySFO || DefwriteFg.CID || DefwriteFg.HWFeatrue 
		|| DefwriteFg.DevAddr || DefwriteFg.OMF)
	{
		return TRUE;
	}
	return FALSE;
}

U8 WriteDefInfo(void)
{
	DEFAULT_SETTING_INFO_t defalut_info_t;
	
	U8      NewSaveFlag = FALSE;

	if(TRUE != ReadDefSetInfo(&defalut_info_t))
	{
		app_printf("SetDefSetInfo fail,you can set again!!!\n");
		return FALSE;
	}

	//app_printf("defalut_info_t:");
	//dump_buf((U8*)&defalut_info_t, sizeof(defalut_info_t));
	if(DefwriteFg.MID)
	{
		app_printf("write:DefwriteFg.MID\n");
		DefwriteFg.MID = FALSE;
		__memcpy(defalut_info_t.ModuleIDInfo.ModuleID, ModuleID, sizeof(ModuleID));
		app_printf("ModuleID -> ");
		dump_buf(defalut_info_t.ModuleIDInfo.ModuleID, sizeof(defalut_info_t.ModuleIDInfo.ModuleID));
        defalut_info_t.ModuleIDInfo.cs=check_sum((U8 *)&defalut_info_t.ModuleIDInfo.ModuleID, sizeof(defalut_info_t.ModuleIDInfo.ModuleID));
        NewSaveFlag = TRUE;
	}
	if(DefwriteFg.PhySFO)
	{
		DefwriteFg.PhySFO = FALSE;	
		defalut_info_t.PhySFOInfo.PhySFODefault = PHY_SFO_DEF;
		app_printf("PHY_SFO_DEF = %lf \n",defalut_info_t.PhySFOInfo.PhySFODefault);
        defalut_info_t.PhySFOInfo.cs=check_sum((U8 *)&defalut_info_t.PhySFOInfo.PhySFODefault, sizeof(defalut_info_t.PhySFOInfo.PhySFODefault));
        NewSaveFlag = TRUE;
	}
	if(DefwriteFg.CID)
	{
		app_printf("write:DefwriteFg.CID\n");
		DefwriteFg.CID = FALSE;
		__memcpy(DefSetInfo.ChipIdFlash_t.ChipID, ManageID, sizeof(ManageID));
		DefSetInfo.ChipIdFlash_t.Used = Vender1;
		DefSetInfo.ChipIdFlash_t.CS = check_sum((U8 *)&DefSetInfo.ChipIdFlash_t,sizeof(DefSetInfo.ChipIdFlash_t)-1);
		__memcpy( (U8*)&defalut_info_t.ChipIdFlash_t, (U8*)&DefSetInfo.ChipIdFlash_t, sizeof(CHIP_ID_FLASH_t) );
        NewSaveFlag = TRUE;
	}
	if(DefwriteFg.HWFeatrue)
	{
		app_printf("write:DefwriteFg.HWFeatrue\n");
		DefwriteFg.HWFeatrue = FALSE;
		DefSetInfo.Hardwarefeature_t.Used = Vender1;
		DefSetInfo.Hardwarefeature_t.CS = check_sum((U8 *)&DefSetInfo.Hardwarefeature_t, sizeof(DefSetInfo.Hardwarefeature_t)-1) ;
		__memcpy( (U8*)&defalut_info_t.Hardwarefeature_t, (U8*)&DefSetInfo.Hardwarefeature_t, sizeof(HARDWARE_FEATURE_t));
        NewSaveFlag = TRUE;
		
	}
	if(DefwriteFg.DevAddr)
	{
		app_printf("write:DefwriteFg.DevAddr\n");
		DefwriteFg.DevAddr = FALSE;
		DefSetInfo.DeviceAddrInfo.Used = Vender1;
		DefSetInfo.DeviceAddrInfo.CS = check_sum((U8 *)&DefSetInfo.DeviceAddrInfo, sizeof(DefSetInfo.DeviceAddrInfo)-1);
		__memcpy( (U8*)&defalut_info_t.DeviceAddrInfo, (U8*)&DefSetInfo.DeviceAddrInfo, sizeof(DEVICE_ADDR_INFO_t));
        NewSaveFlag = TRUE;
		
	}

	if(DefwriteFg.OMF)
	{
		app_printf("write:DefwriteFg.OMF\n");
		DefwriteFg.OMF = FALSE;
		DefSetInfo.OutMFVersion.Used = Vender1;
		DefSetInfo.OutMFVersion.CS = check_sum((U8 *)&DefSetInfo.OutMFVersion, sizeof(DefSetInfo.OutMFVersion)-1);
		__memcpy( (U8*)&defalut_info_t.OutMFVersion, (U8*)&DefSetInfo.OutMFVersion, sizeof(OUT_MF_VERSION_t));
        NewSaveFlag = TRUE;
		
	}

    if(NewSaveFlag == TRUE)
    {
	    return WriteDefSetInfo(&defalut_info_t);
    }
    else
    {
        app_printf("SetDefSetInfo fail,nothing to save!\n");
		return FALSE;
    }
}
U8 WriteExtInfo()
{
	INT8U  ext_info_save_flag = FALSE;

	if(DefwriteFg.FunSWC)
	{
		app_printf("write:DefwriteFg.FunSWC\n");
		DefwriteFg.FunSWC = FALSE;
		app_ext_info.func_switch.Used = Vender1;
		app_ext_info.func_switch.CS = check_sum((U8 *)&app_ext_info.func_switch, sizeof(app_ext_info.func_switch)-1 ) ;
		//__memcpy( (U8*)&defalut_info_t.FunctionSwitch , (U8*)&app_ext_info.func_switch ,sizeof(FUNTION_SWITCH_t) );
		ext_info_save_flag = TRUE;
	}

	if(DefwriteFg.ParaCFG)
	{
		app_printf("write:DefwriteFg.ParaCFG\n");
		DefwriteFg.ParaCFG = FALSE;
		if(app_ext_info.param_cfg.ReTranmitMaxNum == 8)
		{
			app_ext_info.func_switch.AddrShiftSWC = ~app_ext_info.func_switch.AddrShiftSWC;
			DefwriteFg.FunSWC = TRUE;
			SetDefaultInfo();
			return TRUE;
		}
		app_ext_info.param_cfg.Used = Vender1;
		app_ext_info.param_cfg.CS = check_sum((U8 *)&app_ext_info.param_cfg, sizeof(app_ext_info.param_cfg)-1 ) ;
		//__memcpy( (U8*)&defalut_info_t.ParameterCFG , (U8*)&app_ext_info.param_cfg ,sizeof(PARAMETER_CFG_t) );
        ext_info_save_flag = TRUE;
	}
	
	if(TRUE == ext_info_save_flag)
	{
		return app_ext_info_write_param(app_ext_info.buf, APP_EXT_INFO_SIZE);
	}

	return FALSE;
}

U8 WriteFreqInfo()
{
	U8      NewSaveFlag = FALSE;	
	
#if defined(STATIC_MASTER)
	CCO_SYSINFO_t cco_sysinfo_t;
	if(TRUE != ReadFreqSetInfo((void*)&cco_sysinfo_t))
#elif defined(ZC3750STA)
	DEVICE_PIB_t device_pib_t;
	if(TRUE != ReadFreqSetInfo((void*)&device_pib_t))
#elif defined(ZC3750CJQ2)
	COLLECT_INFO_ST collect_info_st;
	if(TRUE != ReadFreqSetInfo((void*)&collect_info_st))
#endif	
	{
		app_printf("SetFreqSetInfo fail,you can set again!!!\n");
		return FALSE;
	}
	
	if(DefwriteFg.FREQ)
	{
		app_printf("write:DefwriteFg.FREQ\n");
		DefwriteFg.FREQ = FALSE;
		DefSetInfo.FreqInfo.Used = Vender1;
		DefSetInfo.FreqInfo.CS = check_sum((U8 *)&DefSetInfo.FreqInfo, sizeof(DefSetInfo.FreqInfo)-1) ;
		
	#if defined(STATIC_MASTER)
		__memcpy( (U8*)&cco_sysinfo_t.FreqInfo, (U8*)&DefSetInfo.FreqInfo, sizeof(FLASH_FREQ_INFO_t));
	#elif defined(ZC3750STA)
		__memcpy( (U8*)&device_pib_t.FreqInfo, (U8*)&DefSetInfo.FreqInfo, sizeof(FLASH_FREQ_INFO_t));
		__memcpy( (U8*)&DevicePib_t.FreqInfo, (U8*)&DefSetInfo.FreqInfo, sizeof(FLASH_FREQ_INFO_t));
	#elif defined(ZC3750CJQ2)
		__memcpy( (U8*)&collect_info_st.FreqInfo, (U8*)&DefSetInfo.FreqInfo, sizeof(FLASH_FREQ_INFO_t));
		__memcpy( (U8*)&CollectInfo_st.FreqInfo, (U8*)&DefSetInfo.FreqInfo, sizeof(FLASH_FREQ_INFO_t));
	#endif
        NewSaveFlag = TRUE;	
	}
	
	if( DefwriteFg.RfChannel == TRUE)
	{
		app_printf("write:DefwriteFg.RfChannel\n");
		DefwriteFg.RfChannel = FALSE;
		DefSetInfo.RfChannelInfo.Used = Vender1;
		DefSetInfo.RfChannelInfo.CS = check_sum((U8 *)&DefSetInfo.RfChannelInfo,sizeof(DefSetInfo.RfChannelInfo)-1) ;
		
	#if defined(STATIC_MASTER)
		__memcpy( (U8*)&cco_sysinfo_t.RfChannelInfo, (U8*)&DefSetInfo.RfChannelInfo, sizeof(Rf_CHANNEL_INFO_t));
	#elif defined(ZC3750STA)
		__memcpy( (U8*)&device_pib_t.RfChannelInfo, (U8*)&DefSetInfo.RfChannelInfo, sizeof(Rf_CHANNEL_INFO_t));
		__memcpy( (U8*)&DevicePib_t.RfChannelInfo, (U8*)&DefSetInfo.RfChannelInfo, sizeof(Rf_CHANNEL_INFO_t));
	#elif defined(ZC3750CJQ2)
		__memcpy( (U8*)&collect_info_st.RfChannelInfo, (U8*)&DefSetInfo.RfChannelInfo, sizeof(Rf_CHANNEL_INFO_t));
		__memcpy( (U8*)&CollectInfo_st.RfChannelInfo, (U8*)&DefSetInfo.RfChannelInfo, sizeof(Rf_CHANNEL_INFO_t));
	#endif
        NewSaveFlag = TRUE;
	}
	
	if(NewSaveFlag == TRUE)
    {
    #if defined(STATIC_MASTER)
		cco_sysinfo_t.cs = check_sum((U8 *)&cco_sysinfo_t, sizeof(CCO_SYSINFO_t) - 1);
		return WriteFreqSetInfo((void*)&cco_sysinfo_t);
	#elif defined(ZC3750STA)
		device_pib_t.Cs = check_sum((U8 *)&device_pib_t, sizeof(DEVICE_PIB_t) - 1);
		return WriteFreqSetInfo((void*)&device_pib_t);
	#elif defined(ZC3750CJQ2)
		collect_info_st.Cs = CJQ_Flash_Check((U8 *)&collect_info_st,sizeof(COLLECT_INFO_ST)-1);
		return WriteFreqSetInfo((void*)&collect_info_st);
	#endif
	    
    }
    else
    {
        app_printf("SetDefSetInfo fail,nothing to save!\n");
		return FALSE;
    }
}

__SLOWTEXT U8 SetDefaultInfo(void)
{
	U8 succ_flag = TRUE;
	if(CheckDefNeedWrite())
	{
		if(!WriteDefInfo())
		{
			succ_flag = FALSE;
		}
	}
	
	if(CheckExtNeedWrite())
	{
		if(!WriteExtInfo())
		{
			succ_flag = FALSE;
		}
	}

	if(CheckFreqNeedWrite())
	{
		if(!WriteFreqInfo())
		{
			succ_flag = FALSE;
		}
	}	
	
	return succ_flag;
}
__SLOWTEXT U8 ReadDefaultInfo(void)
{
	if(TRUE != ReadDefSetInfo(&DefSetInfo))
	{
		app_printf("ReadDefSetInfo fail,will be used default parm!!!\n");	
		return FALSE;
	}

	double    sfo_def = 0;
    if(DefSetInfo.PhySFOInfo.cs == check_sum((U8 *)&DefSetInfo.PhySFOInfo.PhySFODefault,sizeof(DefSetInfo.PhySFOInfo.PhySFODefault)))
    {
        sfo_def = DefSetInfo.PhySFOInfo.PhySFODefault ;
        if((sfo_def>-100&&sfo_def<-2)||(sfo_def>2&&sfo_def<100))
		{
            PHY_SFO_DEF = sfo_def ;
            app_printf("PHY_SFO_DEF = %lf \n",DefSetInfo.PhySFOInfo.PhySFODefault);
        }
        else
        {
            PHY_SFO_DEF = 0 ;
			app_printf("PHY_SFO_DEF = 0 !!!!\n");
        }
    }
	
	U8 moduleId_null[11] = {0};
    U8 moduleID_false[11]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    if(memcmp(moduleId_null,DefSetInfo.ModuleIDInfo.ModuleID,sizeof(ModuleID))&&memcmp(moduleID_false,DefSetInfo.ModuleIDInfo.ModuleID,sizeof(ModuleID)))
    {
        if(DefSetInfo.ModuleIDInfo.cs == check_sum((U8 *)&DefSetInfo.ModuleIDInfo.ModuleID,sizeof(DefSetInfo.ModuleIDInfo.ModuleID)))
        {
            __memcpy(ModuleID,DefSetInfo.ModuleIDInfo.ModuleID,sizeof(ModuleID));
            app_printf("ModuleID -> ");
            dump_buf(ModuleID,sizeof(ModuleID));
        }
    }
        
	if(DefSetInfo.ChipIdFlash_t.CS != check_sum((U8 *)&DefSetInfo.ChipIdFlash_t,sizeof(DefSetInfo.ChipIdFlash_t)-1) 
		|| DefSetInfo.ChipIdFlash_t.Used != Vender1)
	{
		memset((U8*)&DefSetInfo.ChipIdFlash_t,0,sizeof(DefSetInfo.ChipIdFlash_t));
	}
    else
    {
        /*#ifdef MODULE_INSPECT
        __memcpy(ManageID,DefSetInfo.ChipIdFlash_t.ChipID,sizeof(ManageID));
        app_printf("Flash ManageID -> ");
        dump_buf(ManageID,sizeof(ManageID));
        #endif*/
		if( INSPECT_PRODUCTION == app_ext_info.func_switch.IDInfoGetModeSWC)//送检
		{
			__memcpy(ManageID,DefSetInfo.ChipIdFlash_t.ChipID,sizeof(ManageID));
			app_printf("Flash ManageID -> ");
			dump_buf(ManageID,sizeof(ManageID));
		}
    }
        
	if( DefSetInfo.Hardwarefeature_t.CS != check_sum((U8 *)&DefSetInfo.Hardwarefeature_t,sizeof(DefSetInfo.Hardwarefeature_t)-1 ) 
										|| DefSetInfo.Hardwarefeature_t.Used != Vender1)
	{
        U8 type;
		memset((U8*)&DefSetInfo.Hardwarefeature_t,0,sizeof(DefSetInfo.Hardwarefeature_t));
        type=e_UNKW;
        net_nnib_ioctl(NET_SET_DVTYPE,&type);
#if defined(ZC3750STA)
        DeviceTypeDEF = e_NO_DEF;
#endif
	}
	else
	{
#if defined(ZC3750STA)
	    DeviceTypeDEF = ((DefSetInfo.Hardwarefeature_t.Devicetype==e_METER)?e_DEF_1PMETER:
            (DefSetInfo.Hardwarefeature_t.Devicetype==e_3PMETER?e_DEF_3PMETER: e_NO_DEF));
#endif
		
        net_nnib_ioctl(NET_SET_DVTYPE,&DefSetInfo.Hardwarefeature_t.Devicetype);

        net_nnib_ioctl(NET_SET_EDGE,&DefSetInfo.Hardwarefeature_t.edgetype);
	}
	
	if( DefSetInfo.DeviceAddrInfo.CS != check_sum((U8 *)&DefSetInfo.DeviceAddrInfo,sizeof(DefSetInfo.DeviceAddrInfo)-1 ) 
										|| DefSetInfo.DeviceAddrInfo.Used != Vender1)
	{
		memset((U8*)&DefSetInfo.DeviceAddrInfo,0,sizeof(DefSetInfo.DeviceAddrInfo));
	}
    else
    {
		app_printf("DeviceAddrInfo OK\n");
    }

	if( DefSetInfo.OutMFVersion.CS != check_sum((U8 *)&DefSetInfo.OutMFVersion,sizeof(DefSetInfo.OutMFVersion)-1 ) 
										|| DefSetInfo.OutMFVersion.Used != Vender1)
	{
		memset((U8 *)&DefSetInfo.OutMFVersion,0x00,sizeof(DefSetInfo.OutMFVersion));//清除数据，供做bin版本验证使用
		app_printf("OutMFVersion is err,wait!\n");
		
	}
    else
    {
		app_printf("OutMFVersion OK\n");
    }
	
	return TRUE;
}

__SLOWTEXT U8 ReadExtInfo(void)
{
	app_ext_info_read_and_check();
	
	if(app_ext_info.func_switch.CS != check_sum((U8 *)&app_ext_info.func_switch,sizeof(app_ext_info.func_switch)-1) 
										|| app_ext_info.func_switch.Used != Vender1)
	{
		//应用功能未配置，使用默认操作
		SetFunctionSwitchDef(app_ext_info.province_code);
	}
    else
    {
    	app_printf("FunctionSwitch OK\n");
    	if(app_ext_info.func_switch.UseMode == 1)
    	{
			app_printf("Use FunctionSwitch\n");
		}
		else
		{
			//应用功能开关关闭，使用默认操作
			SetFunctionSwitchDef(app_ext_info.province_code);
		}
    }

	if(app_ext_info.param_cfg.CS != check_sum((U8 *)&app_ext_info.param_cfg,sizeof(app_ext_info.param_cfg)-1) 
										|| app_ext_info.param_cfg.Used != Vender1)
	{
		//参数未配置，使用默认参数
		SetParameterCFG(NULL);
	}
    else
    {
    	app_printf("ParameterCFG OK\n");
    	if(app_ext_info.param_cfg.UseMode == 1)
    	{
    		SetParameterCFG((U8 *)&app_ext_info.param_cfg);
			app_printf("Use ParameterCFG\n");
		}
		else
		{
			//参数未启用，使用默认参数
			SetParameterCFG(NULL);
		}
    }
	return TRUE;
}

__SLOWTEXT U8 ReadFreqInfo(void)
{
#if defined(STATIC_MASTER)
	CCO_SYSINFO_t cco_sysinfo_t;
	if(TRUE != ReadFreqSetInfo((void*)&cco_sysinfo_t))
	{
		app_printf("ReadDefSetInfo fail,will be used default parm!!!\n");	
		return FALSE;
	}
	__memcpy((U8 *)&DefSetInfo.FreqInfo,(U8 *)&cco_sysinfo_t.FreqInfo,sizeof(cco_sysinfo_t.FreqInfo));
	__memcpy((U8 *)&DefSetInfo.RfChannelInfo,(U8 *)&cco_sysinfo_t.RfChannelInfo,sizeof(cco_sysinfo_t.RfChannelInfo));
	
#elif defined(ZC3750STA)
	DEVICE_PIB_t device_pib_t;
	if(TRUE != ReadFreqSetInfo((void*)&device_pib_t))
	{
		app_printf("ReadDefSetInfo fail,will be used default parm!!!\n");	
		return FALSE;
	}
	__memcpy((U8 *)&DefSetInfo.FreqInfo,(U8 *)&device_pib_t.FreqInfo,sizeof(device_pib_t.FreqInfo));
	__memcpy((U8 *)&DefSetInfo.RfChannelInfo,(U8 *)&device_pib_t.RfChannelInfo,sizeof(device_pib_t.RfChannelInfo));

#elif defined(ZC3750CJQ2)
	COLLECT_INFO_ST collect_info_st;
	if(TRUE != ReadFreqSetInfo((void*)&collect_info_st))
	{
		app_printf("ReadDefSetInfo fail,will be used default parm!!!\n");	
		return FALSE;
	}
	__memcpy((U8 *)&DefSetInfo.FreqInfo,(U8 *)&collect_info_st.FreqInfo,sizeof(collect_info_st.FreqInfo));
	__memcpy((U8 *)&DefSetInfo.RfChannelInfo,(U8 *)&collect_info_st.RfChannelInfo,sizeof(collect_info_st.RfChannelInfo));
#endif	


	if( DefSetInfo.FreqInfo.CS != check_sum((U8 *)&DefSetInfo.FreqInfo,sizeof(DefSetInfo.FreqInfo)-1 ) 
										|| DefSetInfo.FreqInfo.Used != Vender1)
	{
		DefSetInfo.FreqInfo.FreqBand = DEF_FREQBAND;
		DefSetInfo.FreqInfo.tgaindig = DEF_TGAIN_DIG;
		DefSetInfo.FreqInfo.tgainana = DEF_TGAIN_ANA;
	}

	if( DefSetInfo.RfChannelInfo.CS != check_sum((U8 *)&DefSetInfo.RfChannelInfo,sizeof(DefSetInfo.RfChannelInfo)-1 ) 
										|| DefSetInfo.RfChannelInfo.Used != Vender1)
	{
		DefSetInfo.RfChannelInfo.option = 2;
		DefSetInfo.RfChannelInfo.channel = 16;
	}

	app_printf("read rf chl <%d,%d>\n", DefSetInfo.RfChannelInfo.option, DefSetInfo.RfChannelInfo.channel); 

	return TRUE;

}

U8    ImghdrData[40] = {0};

void ReadOutVersion(void)
{
    uint32_t  state;
    uint32_t  DstImageAddr;
    OUT_MF_VERSION_t  OutMfVersion;
    memset((U8 *)&OutMfVersion,0x00,sizeof(OUT_MF_VERSION_t));
    
    //如果异常，读取BIN 中的文件
    if(memcmp((U8 *)&DefSetInfo.OutMFVersion,(U8 *)&OutMfVersion,sizeof(DefSetInfo.OutMFVersion)) == 0)
    {
        state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
        if(BoardCfg_t.active == 1) // now runing is iamge0
        {
            DstImageAddr = IMAGE0_ADDR;
        }
        else
        {
            DstImageAddr = IMAGE1_ADDR;
        }

        state = zc_flash_read(FLASH, DstImageAddr, (U32 *)&ImghdrData, sizeof(ImghdrData));

        if(state != FLASH_OK)
        {
            app_printf("Flash read upgrade image Fail.\n");
        }

        imghdr_t *pImghdr = (imghdr_t *)ImghdrData;

        if (!is_tri_image(pImghdr))
        {
            app_printf("IMAGE_UNSAFE!!!!\n");
        }
        U16 TempDate = (U16)pImghdr->outdate;
        OutVerFromBin.ucYear = U8TOBCD(((U8)(TempDate)&0x7F));
        OutVerFromBin.ucMonth = U8TOBCD(((U8)(TempDate>>7)&0x0F));
        OutVerFromBin.ucDay = U8TOBCD(((U8)(TempDate>>11)&0x1F));
        __memcpy(OutVerFromBin.ucVersionNum,pImghdr->outversion,2);
        
        app_printf("OutVerFromBin->%02X%02X 20%02X%02X%02x\n",OutVerFromBin.ucVersionNum[1],OutVerFromBin.ucVersionNum[0],OutVerFromBin.ucYear,OutVerFromBin.ucMonth,OutVerFromBin.ucDay);
        if(TempDate ==0)
        {
            app_printf("TempDate ==0!USE const!\n");
            //如果TempDate为空，会在USEoutversion中被重置
            USEoutversion(NULL,NULL,NULL,NULL);
        }
        else
        {
            app_printf("USE bin version and DEFINED MF!\n");
            USEoutversion(NULL,NULL,(U8 *)&OutVerFromBin.ucDay,OutVerFromBin.ucVersionNum);
        }
    }
    else
    {
        app_printf("FLASH version OK!\n");
    }
	//设置关联结构中的外部版本
    setVersionInfo();

}
void ReadBinOutVersion(U8 *Data)
{
    imghdr_t *pImghdr = (imghdr_t *)Data;
    
    if (!is_tri_image(pImghdr))
    {   
        app_printf("IMAGE_UNSAFE!!!!\n");
        return; 
    }
    U16 TempDate = (U16)pImghdr->outdate;
    //outdate[2] = U8TOBCD((U8)(TempDate)&0x7F);
    //outdate[1] = U8TOBCD((U8)(TempDate>>7)&0x0F);
    //outdate[0] = U8TOBCD((U8)(TempDate>>11)&0x1F);
    OutVerFromBin.ucYear = U8TOBCD(((U8)(TempDate)&0x7F));
    OutVerFromBin.ucMonth = U8TOBCD(((U8)(TempDate>>7)&0x0F));
    OutVerFromBin.ucDay = U8TOBCD(((U8)(TempDate>>11)&0x1F));
    __memcpy(OutVerFromBin.ucVersionNum,pImghdr->outversion,2);
    if(TempDate ==0)
    {
        app_printf("TempDate ==0!\n");
        return;
    }
    app_printf("OutVerFromUpgradeBin->%02X%02X 20%02X%02X%02x\n",OutVerFromBin.ucVersionNum[1],OutVerFromBin.ucVersionNum[0],OutVerFromBin.ucYear,OutVerFromBin.ucMonth,OutVerFromBin.ucDay);
    setoutversion(DefSetInfo.OutMFVersion.ucVendorCode,DefSetInfo.OutMFVersion.ucChipCode,(U8 *)&OutVerFromBin.ucDay,OutVerFromBin.ucVersionNum);

}

#if defined(STATIC_NODE)

 void SaveAccessInfo(void)
{
    app_printf("flash_w SaveAccessInfo");
    if(FLASH_OK == zc_flash_write(FLASH, ACCESS_INFO, (U32)&AccessControlListrow, sizeof(AccessControlListrow)))
    {
        app_printf("OK!\n");
    }
    else
    {
        app_printf("ERR!\n");
    }
}

void ReadAccessInfo(void)
{
    app_printf("flash_r SaveAccessInfo");
    if(FLASH_OK == zc_flash_read(FLASH, ACCESS_INFO, (U32 *)&AccessControlListrow, sizeof(AccessControlListrow)))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	}
}

void SaveUpgradeStatusInfo(void)
{
	//app_printf("(U32)&SlaveUpgradeFileInfo : 0x%08x\n",(U32)&SlaveUpgradeFileInfo);
	//dump_buf((U8 *)&SlaveUpgradeFileInfo,sizeof(SLAVE_UPGRADE_FILEINFO_t));
	app_printf("flash_w SaveUpgradeStatusInfo\n");
    if(FLASH_OK == zc_flash_write(FLASH, STA_UPGRADE_INFO, (U32)&SlaveUpgradeFileInfo, sizeof(SlaveUpgradeFileInfo)))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	}
}

void WriteUpgradeImage(void)
{
    uint32_t state;
	
	//app_printf("SlaveUpgradeFileInfo.DstFlashBaseAddr : 0x%08x\n",SlaveUpgradeFileInfo.DstFlashBaseAddr);
	//app_printf("(U32)FileUpgradeData : 0x%08x\n",(U32)FileUpgradeData);
	if((IMAGE0_ADDR ==SlaveUpgradeFileInfo.DstFlashBaseAddr)||(IMAGE1_ADDR ==SlaveUpgradeFileInfo.DstFlashBaseAddr))
	{
        state= zc_flash_write(FLASH, SlaveUpgradeFileInfo.DstFlashBaseAddr,(U32)FileUpgradeData, SlaveUpgradeFileInfo.FileSize);
        app_printf("flash_w WriteUpgradeImage ");
        if(FLASH_OK == state)
		{
			app_printf("OK!\n");
		}
		else
		{
			app_printf("ERR!\n");
		}
   }
}




void ReadStaUpgradeStatusInfo(void)
{
    

	 //    OK==state ? app_printf("zc_flash_read OK!\n") : app_printf("zc_flash_read ERROR!\n");
    app_printf("flash_r UPGRADE_INFO ");
    if(FLASH_OK == zc_flash_read(FLASH, STA_UPGRADE_INFO, (U32 *)&SlaveUpgradeFileInfo, sizeof(SLAVE_UPGRADE_FILEINFO_t)))
	{
		app_printf("OK!\n");
	}
	else
	{
		app_printf("ERR!\n");
	}

}

#endif


//extern board_cfg_t BoardCfg_t;

void ReadBoardCfgInfo(void)
{
    app_printf("flash_r BOARD_CFG");
    if(FLASH_OK == zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t)))
	{
		app_printf("OK!\n");
	}
    else
	{
		app_printf("ERR!\n");
	}
}

REBOOT_INFO_t  RebootInfo;

U8  SaveRebootInfo(void)
{
    REBOOT_INFO_t  RebootInfo_temp;
    int32_t state;
    __memcpy(&RebootInfo_temp,&RebootInfo,sizeof(REBOOT_INFO_t) - 4);
    RebootInfo_temp.Cs = check_sum((U8 *)&RebootInfo_temp, sizeof(REBOOT_INFO_t) - 4);
    RebootInfo_temp.Used = Vender1;
	printf_s("RebootInfo = 0x%02x!\n", RebootInfo_temp.Cs);

    state = zc_flash_write(FLASH, EBOOT_INFO, (U32)&RebootInfo_temp, sizeof(REBOOT_INFO_t));
    printf_s("flash_r RebootInfo ");
    if(FLASH_OK == state)
    {
        printf_s("OK Cs = 0x%02x!\n", RebootInfo_temp.Cs);
        return TRUE;
    }
    else
    {
        printf_s("ERR!\n");
    }
    return FALSE;
}

U8  ReadRebootInfo(void)
{
    printf_s("flash_r ReadRebootInfo ");
    if(FLASH_OK == zc_flash_read(FLASH, EBOOT_INFO, (U32 *)&RebootInfo, sizeof(REBOOT_INFO_t)))
	{
		
        if(RebootInfo.Cs != check_sum((U8 *)&RebootInfo, sizeof(REBOOT_INFO_t) - 4)
		|| RebootInfo.Used != Vender1)
        {
            printf_s("check_sum error Cs = 0x%02x\n", RebootInfo.Cs);

            memset((U8 *)&RebootInfo,0x00,sizeof(REBOOT_INFO_t));
            return FALSE;
        }
        printf_s("OK!\n");
        return TRUE;
	}
	else
	{
		printf_s("ERR!\n");
	}
    return FALSE;
    
}

void check_reboot_info(U32 RebooReason)
{
    U8 ii = 0;
    U8 num = 0;
    U32  MaxCnt = 0;
    
    if(ReadRebootInfo() == TRUE)
    {
        
        for(ii = 0;ii < REBOOT_INFO_MAX;ii++)
        {
            printf_s("cnt %d ,Reason %08x\n",RebootInfo.Cnt[ii],RebootInfo.Reason[ii]);
            if(MaxCnt < RebootInfo.Cnt[ii])
            {
                MaxCnt = RebootInfo.Cnt[ii];
                num = ii+1;
            }
            else
            {
                break;
            }
            
        }
        if(num == REBOOT_INFO_MAX)
        {
            __memmove((U8 *)&RebootInfo.Cnt[0],(U8 *)&RebootInfo.Cnt[1],4*(REBOOT_INFO_MAX-1));
            __memmove((U8 *)&RebootInfo.Reason[0],(U8 *)&RebootInfo.Reason[1],4*(REBOOT_INFO_MAX-1));
            RebootInfo.Cnt[REBOOT_INFO_MAX-1] = ++ MaxCnt;
            RebootInfo.Reason[REBOOT_INFO_MAX-1] = RebooReason;
            printf_s("new1 cnt %d ,Reason %08x\n",RebootInfo.Cnt[REBOOT_INFO_MAX-1],RebootInfo.Reason[REBOOT_INFO_MAX-1]);
        }
        else //if(num == 0)
        {
            RebootInfo.Cnt[num] = ++ MaxCnt;
            RebootInfo.Reason[num] = RebooReason;
            printf_s("new2 cnt %d ,Reason %08x\n",RebootInfo.Cnt[ii],RebootInfo.Reason[ii]);
        }
    }
    else
    {
        RebootInfo.Cnt[0] = 1;
        RebootInfo.Reason[0] = RebooReason;
        printf_s("new3 cnt %d ,Reason %08x\n",RebootInfo.Cnt[ii],RebootInfo.Reason[ii]);
    }

    if(SaveRebootInfo() == TRUE)
    {
        printf_s("SaveRebootInfo ok\n");
    }
    
}

int32_t zc_flash_write(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len)
{
	int32_t ret=0;
	U8 	*pTempBuf,*pScr;	
	U8		offsetFlag=FALSE;	
	pScr = (uint8_t*)src;
    if( src%4&&len < MEM_SIZE_H) //如果没有4字节对齐时，主要针对在栈中申请的临时变量。
    {
    	offsetFlag = TRUE;
        pTempBuf =zc_malloc_mem(len,"zc_flash_write",MEM_TIME_OUT);
		__memcpy(pTempBuf,pScr,len);
    }
	//mutex_pend(mutexSPI_t,0);

#if defined(RF_SPI_SOFT)
	int32_t value = 0;
	app_printf("$<\n");
	gpio_pins_on(&JTAG, NSEL_CTRL);
	//gpio_pins_on(NULL, SDN_CTRL);
	gpio_pins_off(NULL, SPI_MOSI);

	value = *(volatile unsigned int *)0xf2100050;
	value |= 1UL;
	*(volatile unsigned int *)0xf2100050 = value;

    assert_s(gpio_attach_dev(&FLASH->gpio) == 0);
#endif
	if(offsetFlag)
	{
		//app_printf("pTempBuf:");
		//dump_buf(pTempBuf,len);
		#if defined(VENUS_V3)
        ret=flash_write(dev,dst,(U32)pTempBuf,len);
        #else
        ret=flash_write_demo(&dev->spi,dst,(U32)pTempBuf,len);
        #endif
		zc_free_mem(pTempBuf);
	}
	else
	{
		//app_printf("src=%08x,dst=%08x,len=%d",src,dst,len);
		//dump_buf((U8*)src,len);
        #if defined(VENUS_V3)
        ret=flash_write(dev,dst,src,len);
        #else
        ret=flash_write_demo(&dev->spi,dst,src,len);
        #endif
	}

#if defined(RF_SPI_SOFT)

    assert_s(gpio_remove_dev(&FLASH->gpio) == 0);
	value = *(volatile unsigned int *)0xf2100050;
	value &= ~1UL;
	*(volatile unsigned int *)0xf2100050 = value;
	
    gpio_set_dir(&FLASH->gpio, SPI_MISO, GPIO_IN);
    gpio_set_dir(&FLASH->gpio, SPI_SCK|SPI_MOSI, GPIO_OUT);
	gpio_set_dir(&JTAG, NSEL_CTRL, GPIO_OUT);
	gpio_set_dir(NULL, SDN_CTRL, GPIO_OUT);
	gpio_set_dir(&JTAG, NIRQ_CTRL | CCA_CTRL, GPIO_IN);
	
	app_printf(">$\n");
#endif
	return ret;
}

int32_t zc_flash_read(flash_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len)
{
	int32_t ret=0;
	U32 	*pTempBuf;	
	U8		offsetFlag=FALSE;	
	U32    BUFADDR ;
	BUFADDR = (U32)buf;
    if( BUFADDR%4&&len < MEM_SIZE_H) //如果没有4字节对齐时，主要针对在栈中申请的临时变量。
    {
    	
    	offsetFlag = TRUE;
    	pTempBuf =zc_malloc_mem(len,"zc_flash_read",MEM_TIME_OUT);
		//__memcpy(pTempBuf,buf,len);
    }
	//mutex_pend(mutexSPI_t,0);
#if defined(RF_SPI_SOFT)
	int32_t value = 0;
	app_printf("#<");
	gpio_pins_on(&JTAG, NSEL_CTRL);
	//gpio_pins_on(NULL, SDN_CTRL);
	gpio_pins_off(NULL, SPI_MOSI);

	value = *(volatile unsigned int *)0xf2100050;
	value |= 1UL;
	*(volatile unsigned int *)0xf2100050 = value;
    assert_s(gpio_attach_dev(&FLASH->gpio) == 0);
	
#endif
	if(offsetFlag)
	{
        #if defined(VENUS_V3)
        ret=flash_read(dev,addr,pTempBuf,len);
        #else
        ret=flash_read_demo(&dev->spi,addr,pTempBuf,len);
        #endif
        __memcpy( (U8*)buf ,(U8*)pTempBuf,len );
		zc_free_mem(pTempBuf);
	}
	else
	{
        #if defined(VENUS_V3)
        ret=flash_read(dev,addr,buf,len);
        #else
        ret=flash_read_demo(&dev->spi,addr,buf,len);
        #endif
	}

#if defined(RF_SPI_SOFT)
    assert_s(gpio_remove_dev(&FLASH->gpio) == 0);

	value = *(volatile unsigned int *)0xf2100050;
	value &= ~1UL;
	*(volatile unsigned int *)0xf2100050 = value;
    gpio_set_dir(&FLASH->gpio, SPI_MISO, GPIO_IN);
    gpio_set_dir(&FLASH->gpio, SPI_SCK|SPI_MOSI, GPIO_OUT);
	gpio_set_dir(&JTAG, NSEL_CTRL, GPIO_OUT);
	gpio_set_dir(NULL, SDN_CTRL, GPIO_OUT);
	gpio_set_dir(&JTAG, NIRQ_CTRL | CCA_CTRL, GPIO_IN);
	app_printf(">#");
#endif
	//mutex_post(mutexSPI_t);

	return ret;
}
int32_t zc_flash_erase(flash_dev_t *dev, uint32_t addr, int32_t len )
	{
	int32_t ret=0;

    //mutex_pend(mutexSPI_t,0);   
#if defined(RF_SPI_SOFT)//&&defined(ZC3750STA)
		int32_t value = 0;
		app_printf("!<\n");

		gpio_pins_on(&JTAG, NSEL_CTRL);
		
		gpio_pins_off(NULL, SPI_MOSI);
	
		value = *(volatile unsigned int *)0xf2100050;
		value |= 1UL;
		*(volatile unsigned int *)0xf2100050 = value;
        assert_s(gpio_attach_dev(&FLASH->gpio) == 0);
		
#endif
        #if defined(VENUS_V3)
        ret=flash_erase(dev,addr,len);
        #else
        ret=flash_erase_demo(&dev->spi,addr,len);
        #endif

#if defined(RF_SPI_SOFT)
        assert_s(gpio_remove_dev(&FLASH->gpio) == 0);
	
		value = *(volatile unsigned int *)0xf2100050;
		value &= ~1UL;
		*(volatile unsigned int *)0xf2100050 = value;
        gpio_set_dir(&FLASH->gpio, SPI_MISO, GPIO_IN);
        gpio_set_dir(&FLASH->gpio, SPI_SCK|SPI_MOSI, GPIO_OUT);
		gpio_set_dir(&JTAG, NSEL_CTRL, GPIO_OUT);
		gpio_set_dir(NULL, SDN_CTRL, GPIO_OUT);

		gpio_set_dir(&JTAG, NIRQ_CTRL | CCA_CTRL, GPIO_IN);
		app_printf(">!\n");
#endif

		return ret;

	}

/**
* @brief	image是否携带ext_info检查
* @param	void 
* @return	TRUE-携带ext_info，FALSE-不携带ext_info
*******************************************************************************/
INT8U app_check_ext_info(void)
{
	int32_t  state = 0U;
	uint32_t  DstImageAddr = 0U;
	U8    ImageH[40U];

	memset(ImageH, 0U, 40U);
	state = zc_flash_read(FLASH, (U32)BOARD_CFG, (U32 *)&BoardCfg_t, sizeof(board_cfg_t));
	if(BoardCfg_t.active == 2) // now runing is iamge1
	{
		DstImageAddr = IMAGE1_ADDR;
	}
	else
	{
		DstImageAddr = IMAGE0_ADDR;
	}

	state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&ImageH, sizeof(ImageH));
	printf_s("[EI] DstImageAddr = %08x   \n", DstImageAddr);
	if(state != FLASH_OK)
	{
		print_s("[EI]Flash read upgrade image header Fail.\n");
		return FALSE;
	}

	imghdr_t *ih = (imghdr_t *)&ImageH;
	sys_dump(&tty, ImageH, sizeof(ImageH));
	if(FALSE == is_tri_image(ih))
	{
		print_s("[EI]Flash  image header isn't ZC.\n");
		return FALSE;
	}

	if(0x0U == ih->ext_info_flag)
	{
		printf_s("[EI]image no ext_info!\n");
		return FALSE;
	}

	return TRUE;
}

/**
* @brief	写扩展信息
* @param	void *p_data: 扩展信息
* @return	void
*******************************************************************************/
static INT8U write_ext_info(void *p_data)
{
	int32_t  state = FLASH_ERROR;
	AppExtInfo_t *p_app_ext_info = NULL;

	if(NULL == p_data)
	{
		return FALSE;
	}

	p_app_ext_info = (AppExtInfo_t *)p_data;
	//计算CS校验
	p_app_ext_info->func_switch.Used = 'Z';
	p_app_ext_info->func_switch.CS = check_sum((INT8U *)&p_app_ext_info->func_switch, APP_EXT_FUNC_SWITCH_LEN - 1U);

	p_app_ext_info->param_cfg.Used = 'Z';
	p_app_ext_info->param_cfg.CS = check_sum((INT8U *)&p_app_ext_info->param_cfg, APP_EXT_PARAM_CFG_LEN - 1U);

	//计算CRC16
	p_app_ext_info->ex_info_crc16 = CalCRC16(ResetCRC16(), &p_app_ext_info->buf[2], 0, p_app_ext_info->ex_info_len);


	//更新app_ext_info校验
	app_ext_info.func_switch.Used		= p_app_ext_info->func_switch.Used;
	app_ext_info.func_switch.CS			= p_app_ext_info->func_switch.CS;
	app_ext_info.param_cfg.Used			= p_app_ext_info->param_cfg.Used;
	app_ext_info.param_cfg.CS			= p_app_ext_info->param_cfg.CS;

	//更新CRC16校验
	app_ext_info.ex_info_crc16			= p_app_ext_info->ex_info_crc16;

	print_s("[EI]write flash info: \n");
	sys_dump(&tty, p_data, APP_EXT_INFO_SIZE);
	print_s("[EI]ram info: \n");
	sys_dump(&tty, app_ext_info.buf, APP_EXT_INFO_SIZE);
	state = zc_flash_write(FLASH, (U32)EXT_INFO_ADDR, (U32)p_data, APP_EXT_INFO_MAX_SIZE + 2U);
	print_s("[EI]flash_w ext_info ");
	if(state == FLASH_OK)
	{
		print_s("OK!\n");
		return TRUE;
	}
	else
	{
		print_s("ERR!\n");
		return FALSE;
	}
}

/**
* @brief	写参数
* @param	void *p_data: 待写数据
* @return	void
*******************************************************************************/
INT8U app_ext_info_write_param(INT8U *p_data, INT16U len)
{
	INT8U *p_e_data = NULL;
	INT8U result = FALSE;

	if(NULL == p_data)
	{
		return result;
	}

	p_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "ext_info", MEM_TIME_OUT);
	if(NULL == p_e_data)
	{
		return result;
	}

	if(len > (APP_EXT_INFO_MAX_SIZE + 2U))
	{
		zc_free_mem(p_e_data);
		printf_s("[EI]write param error, len=%d\n", len);
		return result;
	}

	__memcpy(p_e_data, p_data, len);
	result = write_ext_info(p_e_data);
	zc_free_mem(p_e_data);
	return result;
}

/**
* @brief	从bin文件末尾读取扩展信息
* @param	INT8U *p_data: 缓存
* @return	TRUE-成功，FALSE-失败
*******************************************************************************/
static INT8U app_ext_info_read_from_image_file(INT8U *p_data)
{
	int32_t  state = 0U;
	uint32_t  DstImageAddr = 0U;
	U8    ImageH[40U];

	if(NULL == p_data)
	{
		return FALSE;
	}

	memset(ImageH, 0U, 40U);
	state = zc_flash_read(FLASH, (U32)BOARD_CFG, (U32 *)&BoardCfg_t, sizeof(board_cfg_t));
	if(BoardCfg_t.active == 2) // now runing is iamge1
	{
		DstImageAddr = IMAGE1_ADDR;
	}
	else
	{
		DstImageAddr = IMAGE0_ADDR;
	}

	state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&ImageH, sizeof(ImageH));
	printf_s("[EI] DstImageAddr = %08x   \n", DstImageAddr);
	if(state != FLASH_OK)
	{
		print_s("[EI]Flash read upgrade image header Fail.\n");
		return FALSE;
	}

	imghdr_t *ih = (imghdr_t *)&ImageH;
	sys_dump(&tty, ImageH, sizeof(ImageH));
	if(FALSE == is_tri_image(ih))
	{
		print_s("[EI]Flash  image header isn't ZC.\n");
		return FALSE;
	}

	if(0x0U == ih->ext_info_flag)
	{
		printf_s("[EI]image no ext_info!\n");
		return FALSE;
	}

	//读取image文件末尾扩展信息
	state = zc_flash_read(FLASH, (DstImageAddr + ih->sz_file), (U32 *)p_data, APP_EXT_INFO_MAX_SIZE + 2U);
	printf_s("[EI]image file app ext_info: (%d)\n", APP_EXT_INFO_SIZE);
	sys_dump(&tty, p_data, APP_EXT_INFO_SIZE);
	return TRUE;
}

/**
* @brief	使用国网默认配置
* @param	void
* @return	void
*******************************************************************************/
static void app_ext_info_use_default_param(void)
{
	print_s("[EI]use default param!!\n");

	app_ext_info.province_code = 0U;
	SetFunctionSwitchDef(app_ext_info.province_code);
	SetParameterCFG(NULL);
}

/**
* @brief	使用image文件配置
* @param	void
* @return	void
*******************************************************************************/
static void app_ext_info_use_image_param(void)
{
	INT8U *p_image_e_data = NULL;
	AppExtInfo_t *image_app_ext_info = NULL;
	INT16U crc_16 = 0U;
	INT16U  crc16_cal = 0U;
	INT8U t_cs = 0U;

	p_image_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "image_ext_info", MEM_TIME_OUT);
	if(NULL == p_image_e_data)
	{
		return;
	}

	if(FALSE == app_ext_info_read_from_image_file(p_image_e_data))
	{
		app_ext_info_use_default_param();
		zc_free_mem(p_image_e_data);
		return;
	}
	image_app_ext_info = (AppExtInfo_t *)p_image_e_data;

	//水印判断
	if(0xAABBCCDD != image_app_ext_info->water_mark)
	{
		printf_s("[EI]image ext info water mark = %08X, error!\n", image_app_ext_info->water_mark);
		app_ext_info_use_default_param();
		zc_free_mem(p_image_e_data);
		return;
	}

	//长度判断
	if(image_app_ext_info->ex_info_len > APP_EXT_INFO_MAX_SIZE)
	{
		printf_s("[EI]image ext info len=%d, error!\n", image_app_ext_info->ex_info_len);
		app_ext_info_use_default_param();
		zc_free_mem(p_image_e_data);
		return;
	}

	//CRC16判断
	crc16_cal = ResetCRC16();
	crc_16 = CalCRC16(crc16_cal, &image_app_ext_info->buf[2], 0, image_app_ext_info->ex_info_len);
	if(crc_16 != image_app_ext_info->ex_info_crc16)
	{
		printf_s("[EI]image ext info crc error!, bin_crc16=%04X, cal_crc16=%04X\n", image_app_ext_info->ex_info_crc16, crc_16);
		app_ext_info_use_default_param();
		zc_free_mem(p_image_e_data);
		return;
	}

	//省份处理
	if(FALSE == FuncJudgeBCD(&image_app_ext_info->province_code, 1U))
	{
		printf_s("[EI]image province code not bcd = %02X\n", image_app_ext_info->province_code);
		app_ext_info_use_default_param();
		zc_free_mem(p_image_e_data);
		return;
	}

	if(image_app_ext_info->province_code >= PROVINCE_NULL)
	{
		//使用国网基础版本配置
		printf_s("[EI]image province code error = %02X\n", image_app_ext_info->province_code);
		app_ext_info_use_default_param();
		zc_free_mem(p_image_e_data);
		return;
	}

	printf_s("[EI]image province code = %02X\n", image_app_ext_info->province_code);
	__memcpy((INT8U *)&app_ext_info.buf, p_image_e_data, APP_EXT_INFO_SIZE);

	//应用功能开关处理
	t_cs = check_sum((INT8U *)&image_app_ext_info->func_switch, APP_EXT_FUNC_SWITCH_LEN - 1U);
	if(image_app_ext_info->func_switch.CS != t_cs)
	{
		printf_s("[EI]image func_switch cs error, use default, cs=%d, cs_cal=%d\n", image_app_ext_info->func_switch.CS, t_cs);
		SetFunctionSwitchDef(image_app_ext_info->province_code);
	}

	//参数配置处理
	t_cs = check_sum((INT8U *)&image_app_ext_info->param_cfg, APP_EXT_PARAM_CFG_LEN - 1U);
	if(image_app_ext_info->param_cfg.CS != t_cs)
	{
		printf_s("[EI]image param_cfg cs error, use default, cs=%d, cs_cal=%d\n", image_app_ext_info->param_cfg.CS, t_cs);
		SetParameterCFG(NULL);
	}

	zc_free_mem(p_image_e_data);
}

/**
* @brief	使用image文件配置
* @param	void
* @return	void
*******************************************************************************/
static void app_ext_info_use_image_param_and_fresh_flash(void)
{
	INT8U *p_flash_e_data = NULL;

	p_flash_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "flash_ext_info", MEM_TIME_OUT);
	if(NULL == p_flash_e_data)
	{
		return;
	}

	app_ext_info_use_image_param();
	__memcpy(p_flash_e_data, app_ext_info.buf, APP_EXT_INFO_SIZE);
	write_ext_info(p_flash_e_data);

	zc_free_mem(p_flash_e_data);
}

/**
* @brief	读取EXT_INFO区信息，并进行检查
* @param	void
* @return	void
*******************************************************************************/
void app_ext_info_read_and_check(void)
{
	int32_t  state = FLASH_ERROR;
	INT8U *p_flash_e_data = NULL;
	INT8U *p_image_e_data = NULL;
	AppExtInfo_t *flash_app_ext_info = NULL;
	AppExtInfo_t *image_app_ext_info = NULL;
	INT16U crc_16 = 0U;
	INT16U  crc16_cal = 0U;
	INT8U t_cs = 0U;

	if(FALSE == app_check_ext_info())
	{
		return;
	}

	p_flash_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "flash1_ext_info", MEM_TIME_OUT);
	if(NULL == p_flash_e_data)
	{
		return;
	}

	//处理扩展信息
	state = zc_flash_read(FLASH, (U32)EXT_INFO_ADDR, (U32*)p_flash_e_data, APP_EXT_INFO_MAX_SIZE + 2U);
	print_s("[EI]flash_r ext_info ");
	if(state == FLASH_OK)
	{
		print_s("OK!\n");
	}
	else
	{
		print_s("ERR!\n");
	}

	printf_s("[EI]flash app ext_info: (%d)\n", APP_EXT_INFO_SIZE);
	sys_dump(&tty, p_flash_e_data, APP_EXT_INFO_SIZE);
	flash_app_ext_info = (AppExtInfo_t *)p_flash_e_data;
	//水印判断
	if(0xAABBCCDD != flash_app_ext_info->water_mark)
	{
		printf_s("[EI]flash ext info water mark = %08X, error!\n", flash_app_ext_info->water_mark);
		app_ext_info_use_image_param_and_fresh_flash();
		zc_free_mem(p_flash_e_data);
		return;
	}

	//长度判断
	if(flash_app_ext_info->ex_info_len > APP_EXT_INFO_MAX_SIZE)
	{
		printf_s("[EI]flash ext info len=%d, error!\n", flash_app_ext_info->ex_info_len);
		app_ext_info_use_image_param_and_fresh_flash();
		zc_free_mem(p_flash_e_data);
		return;
	}

	//CRC16判断
	crc16_cal = ResetCRC16();
	crc_16 = CalCRC16(crc16_cal, &flash_app_ext_info->buf[2], 0, flash_app_ext_info->ex_info_len);
	if(crc_16 != flash_app_ext_info->ex_info_crc16)
	{
		printf_s("[EI]flash ext info crc error!, bin_crc16=%04X, cal_crc16=%04X\n", flash_app_ext_info->ex_info_crc16, crc_16);
		app_ext_info_use_image_param_and_fresh_flash();
		zc_free_mem(p_flash_e_data);
		return;
	}

	//省份处理
	if(FALSE == FuncJudgeBCD(&flash_app_ext_info->province_code, 1U))
		{
		printf_s("[EI]flash province code not bcd = %02X\n", flash_app_ext_info->province_code);
		app_ext_info_use_image_param_and_fresh_flash();
		zc_free_mem(p_flash_e_data);
		return;
		}

	if(flash_app_ext_info->province_code >= PROVINCE_NULL)
	{
		//使用国网基础版本配置
		printf_s("[EI]flash province code error = %02X\n", flash_app_ext_info->province_code);
		app_ext_info_use_image_param_and_fresh_flash();
		zc_free_mem(p_flash_e_data);
		return;
	}

	printf_s("[EI]flash province code = %02X\n", flash_app_ext_info->province_code);

	__memcpy((INT8U *)&app_ext_info.buf, p_flash_e_data, APP_EXT_INFO_SIZE);

	//应用功能开关处理
	t_cs = check_sum((INT8U *)&flash_app_ext_info->func_switch, APP_EXT_FUNC_SWITCH_LEN - 1U);
	if(flash_app_ext_info->func_switch.CS != t_cs)
	{
		printf_s("[EI]func_switch cs error, cs=%d, cs_cal=%d\n", flash_app_ext_info->func_switch.CS, t_cs);

		p_image_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "image1_ext_info", MEM_TIME_OUT);
		if(NULL == p_image_e_data)
		{
			return;
		}

		app_ext_info_read_from_image_file(p_image_e_data);
		image_app_ext_info = (AppExtInfo_t *)p_image_e_data;
		t_cs = check_sum((INT8U *)&image_app_ext_info->func_switch, APP_EXT_FUNC_SWITCH_LEN - 1U);
		if(image_app_ext_info->func_switch.CS != t_cs)
		{
			printf_s("[EI]image func_switch cs error, use default, cs=%d, cs_cal=%d\n", image_app_ext_info->func_switch.CS, t_cs);
			SetFunctionSwitchDef(image_app_ext_info->province_code);
	}
	else
	{
			print_s("[EI]use image func_switch\n");
			__memcpy((INT8U *)&app_ext_info.func_switch, p_image_e_data + APP_EXT_FUNC_SWITCH_OFFSET, APP_EXT_FUNC_SWITCH_LEN);
		}

		zc_free_mem(p_image_e_data);
	}

	//参数配置处理
	t_cs = check_sum((INT8U *)&flash_app_ext_info->param_cfg, APP_EXT_PARAM_CFG_LEN - 1U);
	if(flash_app_ext_info->param_cfg.CS != t_cs)
	{
		printf_s("[EI]param_cfg cs error, cs=%d, cs_cal=%d\n", flash_app_ext_info->param_cfg.CS, t_cs);

		p_image_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "image1_ext_info", MEM_TIME_OUT);
		if(NULL == p_image_e_data)
		{
			return;
		}

		app_ext_info_read_from_image_file(p_image_e_data);
		image_app_ext_info = (AppExtInfo_t *)p_image_e_data;
		t_cs = check_sum((INT8U *)&image_app_ext_info->param_cfg, APP_EXT_PARAM_CFG_LEN - 1U);
		if(image_app_ext_info->param_cfg.CS != t_cs)
	{
			printf_s("[EI]image param_cfg cs error, use default, cs=%d, cs_cal=%d\n", image_app_ext_info->param_cfg.CS, t_cs);
			SetParameterCFG(NULL);
		}
		else
		{
			print_s("[EI]use image param_cfg\n");
			__memcpy((INT8U *)&app_ext_info.param_cfg, p_image_e_data + APP_EXT_PARAM_CFG_OFFSET, APP_EXT_PARAM_CFG_LEN);
		}	

		zc_free_mem(p_image_e_data);
		}

	printf_s("[EI]ram app ext_info: (%d)\n", APP_EXT_INFO_SIZE);
	sys_dump(&tty, app_ext_info.buf, APP_EXT_INFO_SIZE);

	if(0 != memcmp(p_flash_e_data, app_ext_info.buf, APP_EXT_INFO_SIZE))
	{
		__memcpy(p_flash_e_data, app_ext_info.buf, APP_EXT_INFO_SIZE);
		write_ext_info(p_flash_e_data);
	}
	else
	{
		print_s("[EI]ram and flash ext_info same!\n");
	}

	zc_free_mem(p_flash_e_data);
}

/**
* @brief	升级完成后，写ext_info
* @param	void
* @return	void
*******************************************************************************/
void app_ext_info_write_after_upgrade(void)
{
	INT8U *p_e_data = NULL;
	int32_t  state = FLASH_ERROR;

	p_e_data = zc_malloc_mem(APP_EXT_INFO_MAX_SIZE + 2U, "upgrade_ext_info", MEM_TIME_OUT);
	if(NULL == p_e_data)
	{
		return;
	}

	if(FALSE == app_ext_info_read_from_image_file(p_e_data))
	{
		zc_free_mem(p_e_data);
		return;
	}

	//写扩展信息
	print_s("[EI]after upgrade, write flash info: \n");
	sys_dump(&tty, p_e_data, APP_EXT_INFO_SIZE);

	state = zc_flash_write(FLASH, (U32)EXT_INFO_ADDR, (U32)p_e_data, APP_EXT_INFO_MAX_SIZE + 2U);
	print_s("[EI]flash_w ext_info ");
	if(state == FLASH_OK)
	{
		print_s("OK!\n");
	}
	else
	{
		print_s("ERR!\n");
	}

	zc_free_mem(p_e_data);
}
/**
 * @brief 	保存无线信道信息
 * 
 * @param option 
 * @param channel 
 */
void save_Rf_channel(U8 option, U8 channel)
{
	#if defined(STATIC_NODE)
	extern ostimer_t g_ChangeChlTimer ;
	if(TMR_RUNNING == timer_query(&g_ChangeChlTimer))	//正在信道变更过程，不执行操作
	{
		return;
	}
	if(DefSetInfo.RfChannelInfo.option == option &&  DefSetInfo.RfChannelInfo.channel == channel)
	{
		return;
	}
	DefSetInfo.RfChannelInfo.option =   option;
	DefSetInfo.RfChannelInfo.channel =  channel ;
	DefwriteFg.RfChannel =TRUE;
	SetDefaultInfo();
	app_printf("flase save rf chl <%d,%d>\n", option, channel);
	#endif
}

