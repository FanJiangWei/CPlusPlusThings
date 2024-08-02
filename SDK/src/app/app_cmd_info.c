#include <stdio.h>
#include "phy_config.h"
#include "phy_debug.h"
#include "que.h"
#include "list.h"
#include "cpu.h"
#include "mem_zc.h"
#include "cmd.h"
#include "dev_manager.h"
#include "SchTask.h"
#include "app_base_serial_cache_queue.h"
#include "app.h"
#include "app_dltpro.h"
#include "app_read_jzq_cco.h"
#include "app_slave_register_cco.h"
#include "app_slave_register_sta.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_exstate_verify.h"
#include "app_gw3762.h"
#include "app_time_calibrate.h"
#include "app_read_router_cco.h"
#include "app_commu_test.h"
#include "app_moduleid_query.h"
#include "app_area_indentification_cco.h"
#include "app_phase_position_cco.h"
#include "app_power_onoff.h"
#include "app_upgrade_cco.h"
#include "app_upgrade_sta.h"
#include "app_rdctrl.h"
#include "Datalinkdebug.h"
#include "app_clock_sync_comm.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "app_sysdate.h"
#include "power_manager.h"
#include "efuse.h"
#include "netnnib.h"
#include "app_cnf.h"
#include "flash.h"
#include "meter.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "app_area_indentification_cco.h"
#include "app_off_grid.h"
#include "app_area_change.h"
#include "app_cco_clock_overproof_event.h"
#include "app_clock_sta_timeover_report.h"
#include "app_data_freeze_cco.h"
#include "app_cmd_info.h"
#include "DataLinkGlobal.h"
#include "app_read_cjq_list_sta.h"


extern board_cfg_t BoardCfg_t;

char HWVersionStr[50];
U32 g_Irsenddelay = 0;

U8 debugbuf[200];
U32 debuglen = 0;
extern uint32_t  DstImageBaseAddr;
extern uint32_t  ImageFileSize;
extern char* get_inner_version();
extern char* get_outer_version();

param_t cmd_app_param_tab[] =
{
    {PARAM_ARG | PARAM_TOGGLE | PARAM_OPTIONAL, "", "idntfctn|phpos|setband|nodepower|clearflash|cleardef|clearext|\
indentifypma|infoquery|localprotocol|reupgrade|singleupgrade|runupgrade|stopupgrade|showstainfo\
|clearstaflash|setmanuf|writenodeid|showwhitelist|cleanwhitelist|cjq|clnF1F1flag|showf1f1result|showupgrade|showRegist\
|showpower|showblackreport|showid|wsfo|whard|reboot|wdef|funcitionSwc|parameterCFG|setchipid|changebaud|addmeter|lowpower\
|showreboot|setRfband|offwrite|efusewrite|efuseread|showecc|showsm2|setecc_1|setecc_2|setsm2_1|setsm2_2|searchDulType" },
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","setband"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "second", "","setband"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "time", "","reboot"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","Ccopower"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "char", "","setmanuf"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "1/2", "","reupgrade"},	
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "gw13762|gd2016|show", "localprotocol" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "GW1376.2!", "", "gw13762" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "NW 2016!", "", "gd2016" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "show protocol", "", "show" },

    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,				  "dig", "", "nodepower" },//2?êy1
	{PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,				  "ana", "", "nodepower" },//2?êy1

	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "22 num", "command buf","writenodeid"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "48 num", "command buf","setchipid"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "48 num", "command buf","efusewrite"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "64 num", "command buf","setecc_1"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "97 num", "command buf","setecc_2"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "64 num", "command buf","setsm2_1"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "97 num", "command buf","setsm2_2"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "dlt cmd", "def buf","wdef"},
    { PARAM_ARG | PARAM_STRING | PARAM_SUB,				  "TEI", "", "singleupgrade" },//填写tei
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "mac", "","infoquery"},	

	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "show|useMode|debugeMode|noiseDetectSWC|whitelistSWC|upgradeMode|AODVSWC|eventReportSWC\
|moduleIDGetSWC|phaseSWC|indentifySWC|dataAddrFilterSWC|NetSenseSWC|SignalQualitySWC|SetBandSWC|SwtichPhase|oop20EvetSWC|oop20BaudSWC|JZQBaudSWC|MinCollectSWC|IDInfoGetModeSWC|transModeSWC|AddrShiftSWC", "funcitionSwc" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "show|useMode|concurrencySize|reTranmitMaxNum|reTranmitMaxTime|broadcastConRMNum|allbd", "parameterCFG" },
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","concurrencySize"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","reTranmitMaxNum"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","reTranmitMaxTime"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","broadcastConRMNum"},
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","allbd"},
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "init|stop|begin|set|show|print|readmeter|setccoaddr", "cjq" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "searchinit!", "", "init" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "searchbegin!", "", "begin" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "searchstop!", "", "stop" },
	
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "irdelay|timeout|interval", "set" },
	{PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:ms!", "", "irdelay" },
	{PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:s!", "", "timeout" },
	{PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unti:min!", "", "interval" },
		
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "meterinfo|reportwaterinfo", "show" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "searchresult!", "", "meterinfo" },
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "addr", "6byte","setccoaddr" },
	
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB, "", "on|off", "print" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "printopen!", "", "on" },
	{PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "printclose!", "", "off" },
	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "meterAddr", "", "readmeter" },
		
	{PARAM_ARG | PARAM_STRING | PARAM_SUB  , "dlt cmd", "",  "indentifypma"},
	{PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "window", "", "indentifypma" },
	{PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "cnt",    "", "indentifypma" },
	{PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "timer_s", "", "indentifypma" },

	{PARAM_ARG | PARAM_STRING   | PARAM_SUB, "meterAddr", "", "readmeter" },

	{PARAM_ARG | PARAM_TOGGLE   | PARAM_SUB, "", "bflist|start", "phpos" },
	{PARAM_ARG | PARAM_TOGGLE   | PARAM_SUB, "", "start|show", "idntfctn" },
	{PARAM_ARG | PARAM_STRING   | PARAM_SUB, "115200/460800", "","changebaud"},	
	{PARAM_ARG | PARAM_STRING   | PARAM_SUB, "addr", "", "addmeter" },
	{PARAM_ARG | PARAM_TOGGLE   | PARAM_SUB, "",    "start|stop|show",       "lowpower" },

    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB, "option", "1|2|3", "setRfband" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "channel","",      "setRfband" },
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "second", "","setRfband"},
    PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_app_desc, "", cmd_app_param_tab);

void docmd_app(command_t *cmd, xsh_t *xsh)
{
    if(xsh->argc == 1)
    {
        return;
    }
	if (__strcmp(xsh->argv[1], "reboot") == 0)
	{
		uint32_t time;

		time = __atoi(xsh->argv[2]);
		app_reboot(time);
	}
	else if (__strcmp(xsh->argv[1], "setband") == 0)
	{
		set_band(xsh);
	}
	else if (__strcmp(xsh->argv[1], "changebaud") == 0)
	{
		change_baud_rate(xsh);
	}
	else if (__strcmp(xsh->argv[1], "nodepower") == 0)
	{
		set_node_power(xsh);
	}
	else if (__strcmp(xsh->argv[1], "clearflash") == 0)
	{
		clear_flash(xsh->term);
	}
	else if (__strcmp(xsh->argv[1], "cleardef") == 0)
	{
		clear_DEF_flash(xsh->term);
	}
	else if (__strcmp(xsh->argv[1], "clearext") == 0)
	{
		clear_ext_info_flash(xsh->term);
	}
	else if (__strcmp(xsh->argv[1], "showstainfo") == 0)
	{
        showstainfo(xsh->term);
    }
	else if(__strcmp(xsh->argv[1], "funcitionSwc") == 0)
	{
		functionSwc_info(xsh);
	}
	else if(__strcmp(xsh->argv[1], "parameterCFG") == 0)
	{
		parameterCFG_info(xsh);
	}
	else if(__strcmp(xsh->argv[1], "setmanuf") == 0)
	{
		set_manuf(xsh);
	}
    else if (__strcmp(xsh->argv[1], "writenodeid") == 0)
    {
        set_nodeid(xsh);
    }
    else if (__strcmp(xsh->argv[1], "setchipid") == 0)
    {
        set_chipid(xsh);
    }
    else if (__strcmp(xsh->argv[1], "efusewrite") == 0)
	{
        write_efuse(xsh);
	}
    else if (__strcmp(xsh->argv[1], "efuseread") == 0)
    {
        read_efuse(xsh);
	}
    else if (__strcmp(xsh->argv[1], "wdef") == 0)
    {
        def_write(xsh);
    }
    else if(__strcmp(xsh->argv[1], "showreboot") == 0)
	{
        docmd_app_show_rebootinfo(xsh);
    }
	else if(__strcmp(xsh->argv[1], "idntfctn") == 0)
	{
		indentification(xsh);
	}
    else if(__strcmp(xsh->argv[1], "lowpower") == 0)
	{
	    docmd_app_lowpower_set(xsh);
    }
	else if(__strcmp(xsh->argv[1], "setRfband") == 0)
    {
        set_Rfband(xsh);
    }
    else if(__strcmp(xsh->argv[1], "showecc") == 0)
	{
		showecc(xsh);
    }
    else if(__strcmp(xsh->argv[1], "showsm2") == 0)
	{
		showsm2(xsh);
    }
     else if(__strcmp(xsh->argv[1], "setecc_1") == 0)
	{
		setecc_1(xsh);
    }
    else if(__strcmp(xsh->argv[1], "setecc_2") == 0)
	{
		setecc_2(xsh);
    }
    else if(__strcmp(xsh->argv[1], "setsm2_1") == 0)
	{
		setsm2_1(xsh);
    }
    else if(__strcmp(xsh->argv[1], "setsm2_2") == 0)
	{
		setsm2_2(xsh);
    }
    else if(strcmp(xsh->argv[1], "searchDulType") == 0)
	{
		app_printf("type:%d \n", DevicePib_t.MeterPrtcltype);
	}
#if defined(STATIC_MASTER)
    else if(__strcmp(xsh->argv[1], "showupgrade") == 0 )
    {
    	showupgrade(xsh->term);
    }
    else if(__strcmp(xsh->argv[1], "showRegist") == 0 )
    {
    	showRegist(xsh->term);
    }
    else if(__strcmp(xsh->argv[1], "showpower") == 0 )
    {
    	showpower(xsh->term);
    }
    else if(__strcmp(xsh->argv[1], "showblackreport") == 0 )
    {
        area_change_show_report_info(xsh->term);
    }
    else if(__strcmp(xsh->argv[1], "showid") == 0 )
    {
        ShowID(xsh);
    }
	else if(__strcmp(xsh->argv[1], "localprotocol") == 0)
	{
        localprotocol(xsh);
	}
    else  if(__strcmp(xsh->argv[1], "singleupgrade") == 0)
    {
		singleupgrade(xsh);
    }
	else if(__strcmp(xsh->argv[1], "infoquery") == 0)
	 {
	 	infoquery(xsh);
	 }
    else  if(__strcmp(xsh->argv[1], "reupgrade") == 0)
    {
        reupgrade(xsh);
    }
    else  if(__strcmp(xsh->argv[1], "stopupgrade") == 0)
    {
        stopupgrade();
    }
    else  if(__strcmp(xsh->argv[1], "runupgrade") == 0)
    {
		runupgrade();
    }
    else  if(__strcmp(xsh->argv[1], "clnF1F1flag")==0)
    {
        clnF1F1flag(xsh->term);  	
    }
    else  if(__strcmp(xsh->argv[1], "showf1f1result")==0)
    {
        showf1f1result(xsh->term);
    }
	else if (__strcmp(xsh->argv[1], "indentifypma") == 0)
    {
		indentifypma(xsh);
	}
    else if (__strcmp(xsh->argv[1], "showwhitelist") == 0)
    {
		show_whitelist(xsh->term);
    }
    else if (__strcmp(xsh->argv[1], "cleanwhitelist") == 0)
    {
        cleanwhitelist();
    }
	else if(__strcmp(xsh->argv[1], "phpos") == 0)
	{
		phaseposition(xsh);
	}
	else if(__strcmp(xsh->argv[1], "addmeter") == 0)
	{
		add_meter(xsh);
    }
#endif  
     
#if defined(STATIC_NODE)
    else if(__strcmp(xsh->argv[1], "wsfo") == 0)
    {
       sfo_write();
    }
	else if(__strcmp(xsh->argv[1], "whard") == 0)
	{
			DefSetInfo.Hardwarefeature_t.edgetype = 1; //1下降沿，2上升沿
			DefSetInfo.Hardwarefeature_t.Devicetype = e_3PMETER; //
            DefwriteFg.HWFeatrue  = TRUE;
			SetDefaultInfo();
	}
	else if(__strcmp(xsh->argv[1], "clearstaflash") == 0)
    {
        clearstaflash();      
    }
	else  if(__strcmp(xsh->argv[1], "offwrite") == 0)
    {
        off_write();
    }
#endif
    
#if defined(ZC3750CJQ2)
	else if(__strcmp(xsh->argv[1], "cjq") == 0)
	{
		cjq_info(xsh);
	}
#endif	
    
}

#if defined (STATIC_MASTER)

F1F1_INFO_t          f1f1infolist[MAX_WHITELIST_NUM] ;

U8 WIN_SIZE = 5;
U16  UpgradeTEI = 0xFFFF;
extern U8  datalink_deal_flag;


void clear_flash(tty_t *term)
{
    if(FLASH_OK==zc_flash_erase(FLASH,CCO_SYSINFO,CCO_SYSINFOLEN))
	{
		term->op->tprintf(term,"es CCO_SYSINFO ok\n");
	}
	else
	{
		term->op->tprintf(term,"es CCO_SYSINFO err\n");
	}
}


void showupgrade(tty_t *term)
{
	U16 seq=0, ii=0, SuccessNum = 0;
    CCO_UPGRADE_POLL poll_info;

    memset((INT8U *)&poll_info, 0, sizeof(CCO_UPGRADE_POLL));

 	term->op->tprintf(term,"seq\ttei\tStartR\tQbitmR\tQinfoR\tPerfR\tStopR\tRate\n");
    
	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	{
        upgrade_cco_get_poll_info(ii, &poll_info);
		if(poll_info.Valid == 0x01 )
		{
		    term->op->tprintf(term,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                seq++,
                ii+2,
                poll_info.StartResult,
                poll_info.QueryResult,
                poll_info.InfoQueryResult, 
                poll_info.PerformResult,
                poll_info.StopResult,
                poll_info.UpgradeRate);
            if(poll_info.QueryResult == 1 || poll_info.UpgradeRate == 100)
            {
                SuccessNum ++;
            }
        }
	}
    term->op->tprintf(term,"StartNum = %d,SuccessNum = %d\n",seq+1,SuccessNum);
}
void showRegist(tty_t *term)
{
    U16 seq=0,ii=0;
 	term->op->tprintf(term,"seq\ttei\tQuery\tLock\n");

    extern CcoRegstRecord_t CcoRegPolling[MAX_WHITELIST_NUM];
    
	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	{
		if( CcoRegPolling[ii].Valid == 0x01 )
		{
			term->op->tprintf(term,"%d\t%d\t%d\t%d\n"
				,seq++,ii+2,CcoRegPolling[ii].QueryState,CcoRegPolling[ii].LockState);
		}
	}
}
void showpower(tty_t *term)
{
    U16 ii=0 , cnt = 0;
    U8  buf[6] = {0};
    
    term->op->tprintf(term,"tei\tpoof\treof\trema\tpoon\treon\trema\t\n");
    for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
    {
    	cnt = power_off_bitmap_list[ii].PowerFlag+power_off_bitmap_list[ii].PowerReportFlag+power_off_bitmap_list[ii].RemainTime
    			+power_on_bitmap_list[ii].PowerFlag+power_on_bitmap_list[ii].PowerReportFlag+power_on_bitmap_list[ii].RemainTime;
    	if(cnt)
    	{
    		term->op->tprintf(term,"%d\t%d\t%d\t%d\t%d\t%d\t%d\t\n"
    			,ii+2,power_off_bitmap_list[ii].PowerFlag,power_off_bitmap_list[ii].PowerReportFlag,power_off_bitmap_list[ii].RemainTime
    			,power_on_bitmap_list[ii].PowerFlag,power_on_bitmap_list[ii].PowerReportFlag,power_on_bitmap_list[ii].RemainTime);
    	}
    }
    term->op->tprintf(term,"poof\treof\trema\tpoon\treon\trema\tAddr\t\n");
    for(ii=0; ii<POWERNUM; ii++)
    {
        if(memcmp(power_addr_list[ii].MacAddr,buf,6)!=0)
        {

            term->op->tprintf(term,"%d\t%d\t%d\t%d\t%d\t%d\t",
            power_off_addr_list[ii].PowerFlag,power_off_addr_list[ii].PowerReportFlag,power_off_addr_list[ii].RemainTime
    			,power_on_addr_list[ii].PowerFlag,power_on_addr_list[ii].PowerReportFlag,power_on_addr_list[ii].RemainTime);

            dump_tprintf(term,power_addr_list[ii].MacAddr ,6);
        }
    }
      
}


void showf1f1result(tty_t *term)
{
    U16 ii=0,resultnum ;
    U8 buf[6]={0,0,0,0,0,0};
    U8 fbuf[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    term->op->tprintf(term,"idx macaddr--------F1F1Index---------resultflag---\n");
    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        if(0!=memcmp(WhiteMacAddrList[ii].MacAddr,buf,6)&&0!=memcmp(WhiteMacAddrList[ii].MacAddr,fbuf,6))
        {
            term->op->tprintf(term,"%-3d %02x%02x%02x%02x%02x%02x   ",
                                                ii,
                                                WhiteMacAddrList[ii].MacAddr[0],
                                                WhiteMacAddrList[ii].MacAddr[1],
                                                WhiteMacAddrList[ii].MacAddr[2],
                                                WhiteMacAddrList[ii].MacAddr[3],
                                                WhiteMacAddrList[ii].MacAddr[4],
                                                WhiteMacAddrList[ii].MacAddr[5]);

            term->op->tprintf(term,"%-3d        %-3d\n",ii,f1f1infolist[ii].Resultflag);
            
        }
    }
    resultnum = 0;
    for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
    {
        if ( f1f1infolist[ii].Resultflag>0)
        {
            resultnum++;
        }
    }
    term->op->tprintf(term,"resultnum    =   %d\n",resultnum );
}

void show_whitelist(tty_t *term)
{
	U16 ii=0;
	U8 buf[6]={0,0,0,0,0,0};
	U8 fbuf[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8 null_ID[11]={0};
	term->op->tprintf(term,"idx\tmacaddr\t\tcmnaddr\t\tcjqaddr\t\tRelLvl\tSigQual\tPhase\tLNerr\tGUIYUE\tresult\trfth\tstrslt\tmoduleID\tIDState\n");
    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        if(0!=memcmp(WhiteMacAddrList[ii].MacAddr,buf,6)&&0!=memcmp(WhiteMacAddrList[ii].MacAddr,fbuf,6))
        {
			  term->op->tprintf(term,"%-4d\t%02x%02x%02x%02x%02x%02x %02x\t%02x%02x%02x%02x%02x%02x\t%02x%02x%02x%02x%02x%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x\t%02x",
										ii,
									WhiteMacAddrList[ii].MacAddr[0],
									WhiteMacAddrList[ii].MacAddr[1],
									WhiteMacAddrList[ii].MacAddr[2],
									WhiteMacAddrList[ii].MacAddr[3],
									WhiteMacAddrList[ii].MacAddr[4],
									WhiteMacAddrList[ii].MacAddr[5],
									WhiteMacAddrList[ii].WaterAddrH7,
									
									
									
									WhiteMacAddrList[ii].CnmAddr[0],
									WhiteMacAddrList[ii].CnmAddr[1],
									WhiteMacAddrList[ii].CnmAddr[2],
									WhiteMacAddrList[ii].CnmAddr[3],
									WhiteMacAddrList[ii].CnmAddr[4],
									WhiteMacAddrList[ii].CnmAddr[5],

			  						WhiteMaCJQMapList[ii].CJQAddr[0],
									WhiteMaCJQMapList[ii].CJQAddr[1],
									WhiteMaCJQMapList[ii].CJQAddr[2],
									WhiteMaCJQMapList[ii].CJQAddr[3],
									WhiteMaCJQMapList[ii].CJQAddr[4],
									WhiteMaCJQMapList[ii].CJQAddr[5],
									
									WhiteMacAddrList[ii].MeterInfo_t.ucRelLevel,
									WhiteMacAddrList[ii].MeterInfo_t.ucSigQual,
									WhiteMacAddrList[ii].MeterInfo_t.ucPhase,
									WhiteMacAddrList[ii].MeterInfo_t.LNerr,
									WhiteMacAddrList[ii].MeterInfo_t.ucGUIYUE,
									WhiteMacAddrList[ii].Result, 
									WhiteMacAddrList[ii].waterRfTh, 
									WhiteMacAddrList[ii].SetResult,
									(memcmp(WhiteMacAddrList[ii].ModuleID,null_ID,11)==0?0xFF:WhiteMacAddrList[ii].ModuleID[0]),
									WhiteMacAddrList[ii].IDState);
			 
			 if(0==memcmp(WhiteMacAddrList[ii].CnmAddr,buf,6))
			 {
			 	//term->op->tprintf(term," \t\tNOT IN NET");
			 }
			 else
			 {
				 term->op->tprintf(term," \tIN NET");
			 }
			 term->op->tprintf(term,"\n");
		}
    }
    term->op->tprintf(term,"JZQRecordNum = %d\n",FlashInfo_t.usJZQRecordNum );
    
}

void cleanwhitelist(void)
{
    app_printf("--------------------clean white list----------------------\n");
    LEAVE_IND_t *pleaveInd = zc_malloc_mem(sizeof(LEAVE_IND_t), "LEAVE", MEM_TIME_OUT);
        
	WHLPTST  
	
	FlashInfo_t.usJZQRecordNum =  0;	
	U16 i=0;
	do
	{
		memset(&WhiteMacAddrList[i],0,sizeof(WHITE_MAC_LIST_t));
	}while(++i<MAX_WHITELIST_NUM);
    
	WHLPTED 
        
	sysflag=TRUE;
	whitelistflag = TRUE;

    //清空设备列表
    if(Renetworkingtimer)
    {
        timer_start(Renetworkingtimer);
    }

    pleaveInd->StaNum = 0xaa;
    pleaveInd->DelType = e_LEAVE_AND_DEL_DEVICELIST;
    ApsSlaveLeaveIndRequst(pleaveInd);
    zc_free_mem(pleaveInd);
}


void term_buf(xsh_t *xsh,uint8_t *buf, uint32_t len)
{
    tty_t *term = xsh->term;
    uint32_t i;

    for (i = len; i > 0; i--)
    {
        term->op->tprintf(term, "%02x", *(buf + i-1));
    }
}

void ShowID(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U16 ii=0;
    DEVICE_TEI_LIST_t DeviceListTemp;
	U8 buf[6]={0,0,0,0,0,0};
    U8 meterbuf[6]={0,0,0,0,0,0};
	U8 fbuf[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    
    __memcpy(meterbuf,FlashInfo_t.ucJZQMacAddr,6);
    term->op->tprintf(term,"Macaddr\t\tChipID\t\t\t\t\t\t\tmoduleID\t\tCmnaddr\t\tidx\tIDState\tProto\tPhase\tLNerr\tNdMchnYeMoDa-SfVr InnYMD-SfVr \t\n");
    term_buf(xsh,FlashInfo_t.ucJZQMacAddr,6);
    term->op->tprintf(term,"\t");
    term_buf(xsh,ManageID,24);
    term->op->tprintf(term,"\t");
    term_buf(xsh,ModuleID,11);
    term->op->tprintf(term,"\t");
    term_buf(xsh,meterbuf,6);
    term->op->tprintf(term,"\t");
    term->op->tprintf(term,"%-4d\t%02x\t%02x\t%02x\t%02x\t%c%c %c%c %02d%02d%02d %02x%02x %02d%02d%02d %02x%02x \t\n",
                                0,
                                1,
                                FlashInfo_t.scJZQProtocol,
                                7,
                                1,
                                DefSetInfo.OutMFVersion.ucVendorCode[1],
                                DefSetInfo.OutMFVersion.ucVendorCode[0],
                                DefSetInfo.OutMFVersion.ucChipCode[1],
                                DefSetInfo.OutMFVersion.ucChipCode[0],
                                U8TOBCD(DefSetInfo.OutMFVersion.ucYear),
                                U8TOBCD(DefSetInfo.OutMFVersion.ucMonth),
                                U8TOBCD(DefSetInfo.OutMFVersion.ucDay),
                                U8TOBCD(DefSetInfo.OutMFVersion.ucVersionNum[1]),
                                U8TOBCD(DefSetInfo.OutMFVersion.ucVersionNum[0]),
                                InnerDate_Y,
                                InnerDate_M,
                                InnerDate_D,
                                ZC3951CCO_Innerver1,
                                ZC3951CCO_Innerver2
                                );

    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        if(0!=memcmp(WhiteMacAddrList[ii].MacAddr,buf,6)&&0!=memcmp(WhiteMacAddrList[ii].MacAddr,fbuf,6))
        {
            if(Get_DeviceList_All_ByAdd(WhiteMacAddrList[ii].MacAddr ,  &DeviceListTemp)==TRUE)
            {
            
            }
            else
            {
                memset((U8 *)&DeviceListTemp,0x00,sizeof(DeviceListTemp));
            }
            term_buf(xsh,WhiteMacAddrList[ii].MacAddr,6);
            term->op->tprintf(term,"\t");
            term_buf(xsh,DeviceListTemp.ManageID,24);
            term->op->tprintf(term,"\t");
            term_buf(xsh,WhiteMacAddrList[ii].ModuleID,11);
            term->op->tprintf(term,"\t");
            term_buf(xsh,WhiteMacAddrList[ii].CnmAddr,6);
            term->op->tprintf(term,"\t");
			  term->op->tprintf(term,"%-4d\t%02x\t%02x\t%02x\t%02x\t%c%c %c%c %02d%02d%02d %02x%02x %02d%02d%02d %02x%02x \t",
										ii,
										WhiteMacAddrList[ii].IDState,
										WhiteMacAddrList[ii].MeterInfo_t.ucGUIYUE,
										WhiteMacAddrList[ii].MeterInfo_t.ucPhase,
										WhiteMacAddrList[ii].MeterInfo_t.LNerr,
										DeviceListTemp.ManufactorCode[1],
                                        DeviceListTemp.ManufactorCode[0],
                                        DeviceListTemp.ChipVerNum[1],
                                        DeviceListTemp.ChipVerNum[0],
                                        (DeviceListTemp.BuildTime )&0x7F,
                                        (DeviceListTemp.BuildTime >> 7)&0x0F,
                                        (DeviceListTemp.BuildTime >> 11)&0x1F,
                                        DeviceListTemp.SoftVerNum[1],
                                        DeviceListTemp.SoftVerNum[0],
                                        (DeviceListTemp.ManufactorInfo.InnerBTime )&0x7F,
                                        (DeviceListTemp.ManufactorInfo.InnerBTime >> 7)&0x0F,
                                        (DeviceListTemp.ManufactorInfo.InnerBTime >> 11)&0x1F,
                                        DeviceListTemp.ManufactorInfo.InnerVer[1],
                                        DeviceListTemp.ManufactorInfo.InnerVer[0]
										);
				 
				 if(0==memcmp(WhiteMacAddrList[ii].CnmAddr,buf,6))
				 {
				 	//term->op->tprintf(term," \t\tNOT IN NET");
				 }
				 else
				 {
					term->op->tprintf(term," \tIN NET");
				 }
				 term->op->tprintf(term,"\n");
			}
        }
         
}

__LOCALTEXT __isr__ void cco_low_event(void *data)
{
    static U32  rtcnt = 0;
    rtcnt ++;
    if(datalink_deal_flag)
    {
        printf_s("rt %d\n",rtcnt);
    }
    //app_printf("rt2 %d\n",rtcnt);
    return;
}
void localprotocol(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if (__strcmp(xsh->argv[2], "gw13762") == 0)
    {
        FlashInfo_t.scJZQProtocol = e_GW13762_MSG;
        sysflag = TRUE;
    }
    else if (__strcmp(xsh->argv[2], "gd2016") == 0)
    {
        FlashInfo_t.scJZQProtocol = e_GD2016_MSG;
        sysflag = TRUE;
    }
    else if (__strcmp(xsh->argv[2], "show") == 0)
    {
        term->op->tprintf(term, "--------%s---------\n", (FlashInfo_t.scJZQProtocol == e_GD2016_MSG) ? "GD2016" : (FlashInfo_t.scJZQProtocol == e_GW13762_MSG) ? "GW13762"
                                                                                                                                                               : "UNKONW");
    }
}
void singleupgrade(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    term->op->tprintf(term, "this is CCO CMD !\n");

    // U16 tei,ii;
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;

    if (convert_string((U8 *)&UpgradeTEI, xsh->argv[2], 4))
    {
        term->op->tprintf(term, "singleupgrade-0x%x  TEI-------------\n", UpgradeTEI);
        UpgradeTEI -= 2;
        Get_DeviceList_All(UpgradeTEI - 2, &GetDeviceListTemp_t);
        if (GetDeviceListTemp_t.NodeTEI == UpgradeTEI)
        {
                 U8 BinType = 0;

                 upgrade_cco_read_upgrade_info(&BinType);

                 IsCjqfile = e_SELF_ONE_TASK;
                 term->op->tprintf(term, "BinType is %s\n", BinType);
                 term->op->tprintf(term, "Software version is--- %02x%02x ---\n\n",
                                   CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[1],
                                   CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[0]);

                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex = 0;
                 memset(CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrSlaveAddr, 0xFF, 6);
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID = 1; //++CurrUpgradeID;

                 // 自动开始升级
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_UPGRADE_RESET;
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT * 6;

                 upgrade_cco_reset_evt();
        }
    }
}
void infoquery(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 MacAddr[6] = {0};
    if (convert_string(MacAddr, xsh->argv[2], 12))
    {
        term->op->tprintf(term, "macaddr%02x%02x%02x%02x%02x%02x!\n", MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]);

        multi_trans_t ReadTask;
        __memcpy(ReadTask.CnmAddr, MacAddr, 6);
        upgrade_cco_station_info_request(&ReadTask);
    }
}
void reupgrade(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if (FlashInfo_t.scJZQProtocol == e_GD2016_MSG)
    {
    }
    else if (FlashInfo_t.scJZQProtocol == e_GW13762_MSG)
    {
        U8 IsCjqflag = 1;
        IsCjqflag = __atoi(xsh->argv[2]);
        if (IsCjqflag == 1)
        {
                 IsCjqfile = e_SELF_STA_TASK;
                 term->op->tprintf(term, "this is STA Upgrade !deviceType must be %c\n", ZC3750STA_type);
        }
        else if (IsCjqflag == 2)
        {
                 IsCjqfile = e_SELF_CJQ_TASK;
                 term->op->tprintf(term, "this is IICJQ Upgrade !deviceType must be %c\n", ZC3750CJQ2_type);
        }

        U8 BinType = 0;

        upgrade_cco_read_upgrade_info(&BinType);

        term->op->tprintf(term, "BinType is %c\n", BinType);
        term->op->tprintf(term, "Software version is--- %02x%02x ---\n\n",
                          CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[1],
                          CcoUpgrdCtrlInfo.UpgrdFileInfo.FileSoftVer[0]);
        if (((BinType == ZC3750STA_type || BinType == ZC3750STA_UpDate_type) && (IsCjqfile == e_SELF_STA_TASK)) || ((BinType == ZC3750CJQ2_type || BinType == ZC3750CJQ2_UpDate_type) && (IsCjqfile == e_SELF_CJQ_TASK)))
        {
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrBlockIndex = 0;
                 memset(CcoUpgrdCtrlInfo.UpgrdStatusInfo.CurrSlaveAddr, 0xFF, 6);
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradeID = 1; //++CurrUpgradeID;

                 // 自动开始升级
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_UPGRADE_RESET;
                 CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT * 6;

                 upgrade_cco_reset_evt();
        }
    }
}
void stopupgrade()
{
    if (FlashInfo_t.scJZQProtocol == e_GW13762_MSG)
    {
        AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG; // 释放升级状态
        AppGw3762Afn10F4State.RunStatusWorking = 0;
        upgrade_cco_stop();
    }
}
void runupgrade()
{
    if (FlashInfo_t.scJZQProtocol == e_GW13762_MSG)
    {
        upgrade_cco_test_run_req_aps();
    }
}
void clnF1F1flag(tty_t *term)
{
    memset(f1f1infolist, 0x00, sizeof(f1f1infolist));
    term->op->tprintf(term, " clnF1F1flag is OK! \n");
}
void setafn0f7pma(U16 window,U8 cnt,U8 ItemNum,U8 *Item,U16 time_s)
{

}
void indentifypma(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U16 window = 600;
    U8 cnt = 5;
    U8 ItemNum = 0;
    U8 Item[40] = {0};
    U16 time_s = 100;
    char *s;
    U8 i = 0;
    U8 len = 0;

    len = __strlen(xsh->argv[2]);

    term->op->tprintf(term, "len : %d\n", len);

    if (len % 8)
    {
        term->op->tprintf(term, "cmd len != 8*n, please check. \n");
        return;
    }

    len = convert_buf(Item, (uint8_t *)xsh->argv[2], len);

    term->op->tprintf(term, "len : %d\n", len);
    dump_tprintf(term, Item, len);
    ItemNum = len / 4;

    for (i = 3; i < xsh->argc; ++i)
    {
        if ((s = xsh_getopt_val(xsh->argv[i], "-window=")))
        {
                 window = __strtoul(s, NULL, 0);
        }
        else if ((s = xsh_getopt_val(xsh->argv[i], "-cnt=")))
        {
                 cnt = __strtoul(s, NULL, 0);
        }
        else if ((s = xsh_getopt_val(xsh->argv[i], "-timer_s=")))
        {
                 time_s = __strtoul(s, NULL, 0);
        }
    }

    term->op->tprintf(term, "window : %d cnt : %d timer_s : %d ItemNum : %d \n", window, cnt, time_s, ItemNum);
    setafn0f7pma(window, cnt, ItemNum, Item, time_s);

    term->op->tprintf(term, "set indentify\n");
}

void phaseposition(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if (__strcmp(xsh->argv[2], "start") == 0)
    {
        cco_phase_position_start(e_CENTRALIZATION, period_bit);
    }
    else if (__strcmp(xsh->argv[2], "bflist") == 0)
    {
        multi_trans_header_t *listheader = &PHASE_POSITION_BF_LIST;
        if (listheader->nr <= 0)
        {
                 term->op->tprintf(term, "phasebflist is empty!\n");
                 return;
        }
        multi_trans_t *pTask;
        list_head_t *pos, *node;

        list_for_each_safe(pos, node, &listheader->link)
        {
                 pTask = list_entry(pos, multi_trans_t, link);
                 if (pTask->State == EXECUTING)
                 {
                    term->op->tprintf(term, "tei       cnmaddr        State     ReTransmitCnt       DatagramSeq\n");
                    term->op->tprintf(term, "%02x     %02x%02x%02x%02x%02x%02x    %s    %d    %d\n",
                                      pTask->StaIndex + 2,
                                      *(pTask->CnmAddr), *(pTask->CnmAddr + 1), *(pTask->CnmAddr + 2),
                                      *(pTask->CnmAddr + 3), *(pTask->CnmAddr + 4), *(pTask->CnmAddr + 5),
                                      pTask->State == UNEXECUTED ? "UNEXECUTED" : pTask->State == EXECUTING ? "EXECUTING"
                                                                                                            : "NONRESP",
                                      pTask->ReTransmitCnt,
                                      pTask->DatagramSeq);
                 }
        }
    }
}

void add_meter(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 meterAddr[6] = {0};
    U8 debugbuff[12] = {0};
    U8 debugLen;
    U16 j;
    U16 startIndex = FlashInfo_t.usJZQRecordNum;
    debugLen = __strlen(xsh->argv[2]);
    if (debugLen == 0 || debugLen > 12)
    {
        term->op->tprintf(term, "debuglen = %d\n", debugLen);
        return;
    }
    __memcpy(debugbuff, (uint8_t *)xsh->argv[2], debugLen);

    DecodeDebugMeterAddr(debugbuff, meterAddr);
    term->op->tprintf(term, "meteraddr -> %02x %02x %02x %02x %02x %02x\n",
                      meterAddr[0], meterAddr[1], meterAddr[2], meterAddr[3], meterAddr[4], meterAddr[5]);

    //  判断BCD码
    for (j = 0; j < 6; j++)
    {
        if ((meterAddr[j] & 0x0F) > 0x09 || ((meterAddr[j] & 0xF0) >> 4) > 0x09)
        {
                 term->op->tprintf(term, "ERR :addr not BCD!!!\n", debugLen);
                 return;
        }
    }

    //  判断要下载的档案是否与已下载的重复
    for (j = 0; j < startIndex; j++)
    {
        if (memcmp(WhiteMacAddrList[j].MacAddr, meterAddr, 6) == 0)
        {
                 term->op->tprintf(term, "ERR :addr repeat!!!\n", debugLen);
                 return;
        }
    }

    __memcpy(WhiteMacAddrList[startIndex].MacAddr, meterAddr, 6);
    WhiteMacAddrList[startIndex].MeterInfo_t.ucGUIYUE = DLT645_UN_KNOWN;
    // 如果添加表档案已经入网，则只需将入网的通信地址写入白名单中（适用于从节点主动注册后,先入网之后再添加白名单）
    for (j = 0; j < MAX_WHITELIST_NUM; j++)
    {
        DEVICE_TEI_LIST_t GetDeviceListTemp_t;
        Get_DeviceList_All(j, &GetDeviceListTemp_t);

        ChangeMacAddrOrder(GetDeviceListTemp_t.MacAddr);
        if (memcmp(GetDeviceListTemp_t.MacAddr, meterAddr, 6) == 0)
        {
                 ChangeMacAddrOrder(GetDeviceListTemp_t.MacAddr);
                 __memcpy(WhiteMacAddrList[startIndex].CnmAddr, GetDeviceListTemp_t.MacAddr, 6);
                 WhiteMacAddrList[startIndex].MeterInfo_t.ucPhase = GetDeviceListTemp_t.Phase;
                 break;
        }
        ChangeMacAddrOrder(GetDeviceListTemp_t.MacAddr);
    }
    FlashInfo_t.usJZQRecordNum += 1;
    extern void flash_timer_start(void);
    flash_timer_start();
}


#endif

#if defined(STATIC_NODE)

void clear_flash(tty_t *term)
{
    if(FLASH_OK==zc_flash_erase(FLASH,STA_INFO,BOARD_CFG_LEN))
	{
		term->op->tprintf(term,"es BOARD_CFG 16k ok\n");
	}
	else
	{
		term->op->tprintf(term,"es BOARD_CFG 16k err\n");
	}
}
void sfo_write()
{
    DefSetInfo.PhySFOInfo.PhySFOFattributes = TRUE;

    U8 *p;
    PHY_SFO_DEF = phy_get_sfo(SFO_DIR_TX);
    app_printf("PHY_SFO_DEF = %lf", PHY_SFO_DEF);
    p = (U8 *)&PHY_SFO_DEF;
    app_printf("PHY_SFO_DEF = %lf ,hex=%02X %02X %02X %02X %02X %02X %02X %02X\n", PHY_SFO_DEF, p[7], p[6], p[5], p[4], p[3], p[2], p[1], p[0]);
    if ((PHY_SFO_DEF > -100 && PHY_SFO_DEF < -2) || (PHY_SFO_DEF > 2 && PHY_SFO_DEF < 100) || PHY_SFO_DEF == 0)

    {
        DefwriteFg.PhySFO = TRUE;
        SetDefaultInfo();
    }
    else
    {
        app_printf("PHY_SFO_DEF is too big \n");
    }
}
void clearstaflash()
{
    SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
    SaveUpgradeStatusInfo();
}
void off_write()
{
    DevicePib_t.PowerOffFlag = 1; // e_power_off_now;         // 停电状态合法
#if defined(ZC3750STA)
    WriteMeterInfo();
#elif defined(ZC3750CJQ2)
    CollectInfo_st.PowerOFFflag = DevicePib_t.PowerOffFlag;
    WriteCJQ2Info();
#endif
}

#if defined(ZC3750CJQ2)

void show_meterinfo(tty_t *term)
{
    term->op->tprintf(term,"CJQ Addr: %02x%02x%02x%02x%02x%02x\n",
										CollectInfo_st.CollectAddr[5],
										CollectInfo_st.CollectAddr[4],
										CollectInfo_st.CollectAddr[3],
										CollectInfo_st.CollectAddr[2],
										CollectInfo_st.CollectAddr[1],
										CollectInfo_st.CollectAddr[0]);
	term->op->tprintf(term,"DeviceBaseAddr: %02x%02x%02x%02x%02x%02x\n",
										DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[5],
										DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[4],
										DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[3],
										DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[2],
										DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[1],
										DefSetInfo.DeviceAddrInfo.DeviceBaseAddr[0]);
    term->op->tprintf(term,"SetCCOAddr : %02x%02x%02x%02x%02x%02x\n",
										CollectInfo_st.SetCCOAddr[5],
										CollectInfo_st.SetCCOAddr[4],
										CollectInfo_st.SetCCOAddr[3],
										CollectInfo_st.SetCCOAddr[2],
										CollectInfo_st.SetCCOAddr[1],
										CollectInfo_st.SetCCOAddr[0]);
    term->op->tprintf(term,"nnib.CCOMacAddr: %02x%02x%02x%02x%02x%02x\n",
										nnib.CCOMacAddr[5],
										nnib.CCOMacAddr[4],
										nnib.CCOMacAddr[3],
										nnib.CCOMacAddr[2],
										nnib.CCOMacAddr[1],
										nnib.CCOMacAddr[0]);
	U16 ii=0;
	
	term->op->tprintf(term,"Addr		 Protocol	 Baud		 TotalNum = %d\n",CollectInfo_st.MeterNum);
	
	for(ii = 0; ii < CollectInfo_st.MeterNum; ii++)
	{
		term->op->tprintf(term,"%02x%02x%02x%02x%02x%02x  %04d		  %04d\n",
					CollectInfo_st.MeterList[ii].MeterAddr[5],
					CollectInfo_st.MeterList[ii].MeterAddr[4],
					CollectInfo_st.MeterList[ii].MeterAddr[3],
					CollectInfo_st.MeterList[ii].MeterAddr[2],
					CollectInfo_st.MeterList[ii].MeterAddr[1],
					CollectInfo_st.MeterList[ii].MeterAddr[0],
					(DLT645_2007 == CollectInfo_st.MeterList[ii].Protocol) ? 2007 : 
					(DLT698 == CollectInfo_st.MeterList[ii].Protocol ? 698 : 
                        1997),
					baud_enum_to_int(CollectInfo_st.MeterList[ii].BaudRate));
	}
    
}

void cjq_info(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if (__strcmp(xsh->argv[2], "init") == 0)
    {
        CJQ_Extend_ParaInit();
        CJQ_Extend_ShowInfo();
    }
    else if (__strcmp(xsh->argv[2], "stop") == 0)
    {
        cjq2_search_meter_info_t.Ctrl.SearchState = SEARCH_STATE_STOP;
    }
    else if (__strcmp(xsh->argv[2], "begin") == 0)
    {
        CJQ_Check_Begin(&cjq2_search_meter_info_t.Ctrl);
        CJQ_Extend_ShowInfo();
    }
    else if (__strcmp(xsh->argv[2], "set") == 0)
    {
        if (__strcmp(xsh->argv[3], "irdelay") == 0)
        {
                 g_Irsenddelay = __atoi(xsh->argv[4]);
                 term->op->tprintf(term, "irdelay time : %d ms\n", g_Irsenddelay);
        }
        else if (__strcmp(xsh->argv[3], "timeout") == 0)
        {
                 CollectInfo_st.Timeout = __atoi(xsh->argv[4]);
                 term->op->tprintf(term, "timeout : %d s\n", __atoi(xsh->argv[4]));
        }
        else if (__strcmp(xsh->argv[3], "interval") == 0)
        {
                 CollectInfo_st.Interval = __atoi(xsh->argv[4]);
                 term->op->tprintf(term, "Interval : %d min\n", CollectInfo_st.Interval);
        }
    }
    else if (__strcmp(xsh->argv[2], "show") == 0)
    {
        if (__strcmp(xsh->argv[3], "meterinfo") == 0)
        {
                 show_meterinfo(xsh->term);
        }
    }
    else if (__strcmp(xsh->argv[2], "setccoaddr") == 0)
    {
        U32 len = 0;
        U8 buf[6] = {0};
        len = __strlen(xsh->argv[3]);
        if (len != 12)
        {
                 term->op->tprintf(term, " please check lenth !\n");
                 return;
        }
        len = convert_buf(buf, (uint8_t *)xsh->argv[3], len);
        term->op->tprintf(term, "CJQ2 setccoaddr!\n");
        ChangeMacAddrOrder(buf);
        __memcpy(CollectInfo_st.SetCCOAddr, buf, 6);
        dump_buf(CollectInfo_st.SetCCOAddr, 6);
    }
    else if (__strcmp(xsh->argv[2], "print") == 0)
    {
        if (__strcmp(xsh->argv[3], "on") == 0)
        {
                 CJQ_PRINT_ON();
        }
        else if (__strcmp(xsh->argv[3], "off") == 0)
        {
                 CJQ_PRINT_OFF();
        }
    }
}

#endif

#endif

void clear_DEF_flash(tty_t *term)
{
    if(FLASH_OK==zc_flash_erase(FLASH,DEFAULTSETTING,BOARD_CFG_LEN - EXT_INFO_LEN))
	{
		term->op->tprintf(term,"es DEFAULTSETTING 12k ok\n");
	}
	else
	{
		term->op->tprintf(term,"es DEFAULTSETTING 12k err\n");
	}
}

void clear_ext_info_flash(tty_t *term)
{
    if(FLASH_OK == zc_flash_erase(FLASH, EXT_INFO_ADDR, EXT_INFO_LEN))
    {
        term->op->tprintf(term, "[EI]es ext_info 4k ok\n");
    }
    else
    {
        term->op->tprintf(term, "[EI]es ext_info 4k err\n");
	}
}

void showstainfo(tty_t *term)
{
#if defined(STATIC_NODE)

#ifdef TH2CJQ
    TH2CJQ_Extend_ShowInfo();
#endif

    uint32_t    ImageBaseAddr;
    imghdr_t   imageHdr;

    if(BoardCfg_t.active == 1)   // Get current run image addr
    {
        ImageBaseAddr = IMAGE0_ADDR;
    }
    else if(BoardCfg_t.active == 2)
    {
        ImageBaseAddr = IMAGE1_ADDR;
    }
    else
    {
        ImageBaseAddr = IMAGE0_ADDR;
    }
    term->op->tprintf(term,"imageHdr : 0x%08x\n",&imageHdr);
    FLASH_OK == zc_flash_read(FLASH, ImageBaseAddr, (uint32_t *)&imageHdr,  sizeof(imghdr_t))
    ?  term->op->tprintf(term,"UPGRADE_INFO zc_flash_read OK!\n") :  term->op->tprintf(term,"zc_flash_read ERROR!\n");

    term->op->tprintf(term,"BoardCfg_t.active = %d\n", BoardCfg_t.active);

    term->op->tprintf(term,"SlaveUpgradeFileInfo.StaUpgradeStatus = %s\n", e_UPGRADE_IDLE==SlaveUpgradeFileInfo.StaUpgradeStatus?"e_UPGRADE_IDLE":
    																	  e_FILE_DATA_RECEIVING==SlaveUpgradeFileInfo.StaUpgradeStatus?"e_FILE_DATA_RECEIVING":
    																	  e_FILE_RCEV_FINISHED==SlaveUpgradeFileInfo.StaUpgradeStatus?"e_FILE_RCEV_FINISHED":
    																	  e_UPGRADE_PERFORMING==SlaveUpgradeFileInfo.StaUpgradeStatus?"e_UPGRADE_PERFORMING":
    																	  e_UPGRADE_TEST_RUN==SlaveUpgradeFileInfo.StaUpgradeStatus?"e_UPGRADE_TEST_RUN":"UNKOWN");

    term->op->tprintf(term,"Factory Num is %c%c, Version is %02x%02x, File CRC is %08x, File Size is %d\n", 
                                                Vender1,Vender2,ZC3750STA_ver1,ZC3750STA_ver2,imageHdr.crc,imageHdr.sz_file);
#elif defined(STATIC_MASTER)
    extern EventReportStatus_e    g_EventReportStatus;
    term->op->tprintf(term,"EventReportStatus == %d!\n",g_EventReportStatus);

#endif
    term->op->tprintf(term,"ModuleID -> ");
    dump_tprintf(term,ModuleID , sizeof(ModuleID));
    term->op->tprintf(term,"PHY_SFO_DEF = %lf \n",PHY_SFO_DEF);
    term->op->tprintf(term,"HWDefInfo -> ");
    extern HARDWARE_FEATURE_t   HWDefInfo;
    U8 tempbuf[14] = {0};
    dump_tprintf(term,(U8 *)&HWDefInfo , sizeof(HWDefInfo));
    __memcpy(tempbuf,(U8 *)&HWDefInfo,sizeof(tempbuf));
    Add0x33ForData(tempbuf,sizeof(tempbuf));
    dump_tprintf(term,tempbuf,sizeof(tempbuf));

}
void docmd_app_lowpower_set(xsh_t *xsh)
{
#if defined(STATIC_NODE)

#if defined(POWER_MANAGER)
    tty_t *term = xsh->term;

	if (__strcmp(xsh->argv[2], "start") == 0) 
    {           
	    lowpower_start();
		term->op->tprintf(term, "lowpower start\n");
	} else if (__strcmp(xsh->argv[2], "stop") == 0) 
    {
		lowpower_stop();
        term->op->tprintf(term, "lowpower stop\n");
	} else if (__strcmp(xsh->argv[2], "show") == 0) 
    {
		
        term->op->tprintf(term, "lowpower %d RX %d last %d time %d s\n",
            LWPR_INFO.LowPowerStates,LWPR_INFO.LowPowerRxStates,LWPR_INFO.LastRxStates,LWPR_INFO.RunTime);
	}
#endif

#endif

	return;
}

void docmd_app_show_rebootinfo(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    term->op->tprintf(term, "show_rebootinfo\n");
    U8 ii = 0;
    
    for(ii = 0;ii < REBOOT_INFO_MAX;ii++)
    {
        term->op->tprintf(term, "cnt %d ,Reason %08x\n",RebootInfo.Cnt[ii],RebootInfo.Reason[ii]);
    }

	return;
}
char *get_flash_hw_version()
{
    char   *Str = HWVersionStr;
    __snprintf(Str,sizeof(HWVersionStr), "%c%c%c%c%c-%c%c%c-V%01x.%01x.%02x     ProdtTime: 20%02d-%02d-%02d", 
        DefSetInfo.Hardwarefeature_t.Vender[1] , 
        DefSetInfo.Hardwarefeature_t.Vender[0] , 
        DefSetInfo.Hardwarefeature_t.ProductFunc , 
        DefSetInfo.Hardwarefeature_t.ChipCode ,
        DefSetInfo.Hardwarefeature_t.ProductType , 
        DefSetInfo.Hardwarefeature_t.Protocol ,
        DefSetInfo.Hardwarefeature_t.Category[1] ,
        DefSetInfo.Hardwarefeature_t.Category[0] , 
        (DefSetInfo.Hardwarefeature_t.HardwareVer[1]>>4)&0x0F ,
        DefSetInfo.Hardwarefeature_t.HardwareVer[1]&0x0F ,
        DefSetInfo.Hardwarefeature_t.HardwareVer[0] ,
        (DefSetInfo.Hardwarefeature_t.ProductTime)&0x7F ,
        (DefSetInfo.Hardwarefeature_t.ProductTime >> 7)&0x0F ,
        (DefSetInfo.Hardwarefeature_t.ProductTime >> 11)&0x1F );
    
    return Str; 
}

void show_version()
{
    app_printf("\r\n");
    app_printf("province      : %02X\n", app_ext_info.province_code);
    app_printf("SDKversion    : %s\n", get_sw_version());
    app_printf("Software      : %04x\n", get_zc_sw_version());
	app_printf("Build time    : %s\n", get_build_time());
	app_printf("Chip Hardware : %s\n", get_hw_version());
	app_printf("Inner version : %s\n", get_inner_version());
    app_printf("Outer version : %s\n", get_outer_version());
    app_printf("Def Hardware  : %s\n", get_flash_hw_version());
    app_printf("ChipID from %s : ",DefSetInfo.ChipIdFlash_t.Used ==Vender1?"F":"C");
	dump_buf(ManageID,sizeof(ManageID));
    app_printf("ModuleID      : ");
    dump_buf(ModuleID,sizeof(ModuleID));
	app_printf("Edgetype      : %s\n",DefSetInfo.Hardwarefeature_t.edgetype ==1?"DN":
								DefSetInfo.Hardwarefeature_t.edgetype ==2?"UP":
								DefSetInfo.Hardwarefeature_t.edgetype ==3?"DOUBLE":
									"UK");
	app_printf("Devicetype    : %s\n",
						DefSetInfo.Hardwarefeature_t.Devicetype == e_CKQ ?"e_CKQ":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_JZQ ?"e_JZQ":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_METER ?"e_METER":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_RELAY ?"e_RELAY":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_CJQ_1 ?"CJQ1":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_CJQ_2 ?"CJQ2":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_3PMETER ?"3XB":"Uk");
    app_printf("PHY_SFO_DEF   : %lf\n", PHY_SFO_DEF);
    app_printf("FreqBand      : %d\n", DefSetInfo.FreqInfo.FreqBand);
    app_printf("dig   ana     : %d    %d\n", DefSetInfo.FreqInfo.tgaindig,DefSetInfo.FreqInfo.tgainana);
    app_printf("\r\n");
	return;
}

void showfunctionSwc(tty_t *term)
{
    term->op->tprintf(term,"FuncitionSwc\t\tState\n");
    term->op->tprintf(term,"UseMode\t\t\t%d\n",app_ext_info.func_switch.UseMode);
    term->op->tprintf(term,"DebugeMode\t\t%d\n",app_ext_info.func_switch.DebugeMode);
    term->op->tprintf(term,"NoiseDetectSWC\t\t%d\n",app_ext_info.func_switch.NoiseDetectSWC);
    term->op->tprintf(term,"WhitelistSWC\t\t%d\n",app_ext_info.func_switch.WhitelistSWC);
    term->op->tprintf(term,"UpgradeMode\t\t%d\n",app_ext_info.func_switch.UpgradeMode);
    term->op->tprintf(term,"AODVSWC\t\t\t%d\n",app_ext_info.func_switch.AODVSWC);
    term->op->tprintf(term,"EventReportSWC\t\t%d\n",app_ext_info.func_switch.EventReportSWC);
    term->op->tprintf(term,"ModuleIDGetSWC\t\t%d\n",app_ext_info.func_switch.ModuleIDGetSWC);
    term->op->tprintf(term,"PhaseSWC\t\t%d\n",app_ext_info.func_switch.PhaseSWC);
    term->op->tprintf(term,"IndentifySWC\t\t%d\n",app_ext_info.func_switch.IndentifySWC);
    term->op->tprintf(term,"DataAddrFSWC\t\t%d\n",app_ext_info.func_switch.DataAddrFilterSWC);
    term->op->tprintf(term,"NetSenseSWC\t\t%d\n",app_ext_info.func_switch.NetSenseSWC);
    term->op->tprintf(term,"SignalQualitySWC\t%d\n",app_ext_info.func_switch.SignalQualitySWC);
    term->op->tprintf(term,"SetBandSWC\t\t%d\n",app_ext_info.func_switch.SetBandSWC);
    term->op->tprintf(term,"SwtichPhase\t\t%d\n",app_ext_info.func_switch.SwPhase);
    term->op->tprintf(term,"oop20EvetSWC\t\t%d\n",app_ext_info.func_switch.oop20EvetSWC);
    term->op->tprintf(term,"oop20BaudSWC\t\t%d\n",app_ext_info.func_switch.oop20BaudSWC);
	term->op->tprintf(term,"JZQBaudSWC\t\t%d\n",app_ext_info.func_switch.JZQBaudSWC);
	term->op->tprintf(term,"MinCollectSWC\t\t%d\n",app_ext_info.func_switch.MinCollectSWC);
	term->op->tprintf(term,"IDInfoGetModeSWC\t%d\n",app_ext_info.func_switch.IDInfoGetModeSWC);
    term->op->tprintf(term,"TransModeSWC\t\t%d\n",app_ext_info.func_switch.TransModeSWC);
    term->op->tprintf(term,"AddrShiftSWC\t\t%d\n",app_ext_info.func_switch.AddrShiftSWC);
}
void show_parameterCFG(tty_t *term)
{
    term->op->tprintf(term,"parameterCFG\tState\n");
    term->op->tprintf(term,"UseMode\t\t%d\n",app_ext_info.param_cfg.UseMode);
    term->op->tprintf(term,"ConSize\t\t%d\n",app_ext_info.param_cfg.ConcurrencySize);
    term->op->tprintf(term,"ReTMaxNum\t%d\n",app_ext_info.param_cfg.ReTranmitMaxNum);
    term->op->tprintf(term,"ReTMaxTime\t%d\n",app_ext_info.param_cfg.ReTranmitMaxTime);
    term->op->tprintf(term,"BcRMNum\t\t%d\n",app_ext_info.param_cfg.BroadcastConRMNum);
    term->op->tprintf(term,"AllBdType\t%d\n",app_ext_info.param_cfg.AllBroadcastType);
}
void DecodeDebugMeterAddr(U8 *buff, U8 *pAddr)//调整调试口输入的表地址
{
	U8 ii,record;
	for(ii=0; ii<12; ii++)
	{
		if(buff[ii] == 0)
			break;
	}
	record = ii;
	
	for(ii=0; ii<(record/2+record%2); ii++)
	{	
		if(record%2)
		{
			if(ii == record/2)
			{
				pAddr[ii] = buff[0]&0x0f;
				break;
			}
		}
		pAddr[ii] = ((buff[record-(2*(ii+1))]<<4)&0xf0) + (buff[record-(2*ii+1)]&0x0f);
	}
	return;

} 

void set_band(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 band;
    band = __atoi(xsh->argv[2]);
    if (band <= 3)
    {
#if defined(STATIC_MASTER)
        U16 RemianTime;
        RemianTime = __atoi(xsh->argv[3]);
        nnib.BandChangeState = TRUE;
        nnib.BandRemianTime = RemianTime; // 100s
#else
        net_nnib_ioctl(NET_SET_BAND, &band);
#endif

        DefSetInfo.FreqInfo.FreqBand = band;
        DefwriteFg.FREQ = TRUE;
        DefaultSettingFlag = TRUE;
        term->op->tprintf(term, "Set DefSetInfo.FreqInfo.FreqBand=%d", DefSetInfo.FreqInfo.FreqBand);
    }
    else
    {
        term->op->tprintf(term, "error band.\n");
    }
    return;
}

void change_baud_rate(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    board_cfg_t boardcfg;

    __memcpy(&boardcfg, BOARD_CFG, sizeof(board_cfg_t));
    if (!xsh->argv[2])
        boardcfg.baud = BAUDRATE_115200;
    else
        boardcfg.baud = __strtoul(xsh->argv[2], 0, 0);

    term->op->tprintf(term, "\nbaudrate: %8d\n", boardcfg.baud);
    uart_init(UART0, boardcfg.baud, PARITY_EVEN, STOPBITS_1, DATABITS_8);
    uart_set_int_enable(UART0, IER_RDA | IER_RLS | IER_RTO);

    return;
}
void set_node_power(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 bandix;
    S16 dig = 0, ana = 0;
    // int32_t tgain=0;
    dig = __atoi(xsh->argv[2]);
    ana = __atoi(xsh->argv[3]);

    dig = (dig < 0) ? 0 : (dig > 8 ? 8 : dig);
    ana = (ana < -20) ? -20 : (ana > 18 ? 18 : ana);

    DefSetInfo.FreqInfo.tgaindig = dig;
    DefSetInfo.FreqInfo.tgainana = ana;

    DefwriteFg.FREQ = TRUE;
    DefaultSettingFlag = TRUE;
    net_nnib_ioctl(NET_GET_BAND, &bandix);

    net_nnib_ioctl(NET_SET_BAND, &bandix);
    term->op->tprintf(term, "--nodepower dig=%d,ana=%d,%d---\n", dig, ana, bandix);

    mutex_post(&mutexSch_t);
}
void functionSwc_info(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if (__strcmp(xsh->argv[2], "show") == 0)
    {
        showfunctionSwc(term);
        return;
    }
    if (__strcmp(xsh->argv[2], "useMode") == 0)
    {
        app_ext_info.func_switch.UseMode = ~app_ext_info.func_switch.UseMode;
        term->op->tprintf(term, "UseMode\t\t%d\n", app_ext_info.func_switch.UseMode);
    }
    else if (__strcmp(xsh->argv[2], "debugeMode") == 0)
    {
        app_ext_info.func_switch.DebugeMode = ~app_ext_info.func_switch.DebugeMode;
        term->op->tprintf(term, "DebugeMode\t\t%d\n", app_ext_info.func_switch.DebugeMode);
    }
    else if (__strcmp(xsh->argv[2], "noiseDetectSWC") == 0)
    {
        app_ext_info.func_switch.NoiseDetectSWC = ~app_ext_info.func_switch.NoiseDetectSWC;
        term->op->tprintf(term, "NoiseDetectSWC\t\t%d\n", app_ext_info.func_switch.NoiseDetectSWC);
    }
    else if (__strcmp(xsh->argv[2], "whitelistSWC") == 0)
    {
        app_ext_info.func_switch.WhitelistSWC = ~app_ext_info.func_switch.WhitelistSWC;
        term->op->tprintf(term, "WhitelistSWC\t\t%d\n", app_ext_info.func_switch.WhitelistSWC);
    }
    else if (__strcmp(xsh->argv[2], "upgradeMode") == 0)
    {
        app_ext_info.func_switch.UpgradeMode = ~app_ext_info.func_switch.UpgradeMode;
        term->op->tprintf(term, "UpgradeMode\t\t%d\n", app_ext_info.func_switch.UpgradeMode);
    }
    else if (__strcmp(xsh->argv[2], "AODVSWC") == 0)
    {
        app_ext_info.func_switch.AODVSWC = ~app_ext_info.func_switch.AODVSWC;
        term->op->tprintf(term, "AODVSWC\t\t%d\t\n", app_ext_info.func_switch.AODVSWC);
        nnib.AODVRouteUse = app_ext_info.func_switch.AODVSWC;
        if (nnib.AODVRouteUse == TRUE)
        {
            term->op->tprintf(term, "--AODVRouteUse ON---\n");
        }
        else
        {
            term->op->tprintf(term, "--AODVRouteUse OFF---\n");
        }
    }
    else if (__strcmp(xsh->argv[2], "eventReportSWC") == 0)
    {
        app_ext_info.func_switch.EventReportSWC = ~app_ext_info.func_switch.EventReportSWC;
        term->op->tprintf(term, "EventReportSWC\t\t%d\n", app_ext_info.func_switch.EventReportSWC);
    }
    else if (__strcmp(xsh->argv[2], "moduleIDGetSWC") == 0)
    {
        app_ext_info.func_switch.ModuleIDGetSWC = ~app_ext_info.func_switch.ModuleIDGetSWC;
        term->op->tprintf(term, "ModuleIDGetSWC\t\t%d\n", app_ext_info.func_switch.ModuleIDGetSWC);
    }
    else if (__strcmp(xsh->argv[2], "phaseSWC") == 0)
    {
        app_ext_info.func_switch.PhaseSWC = ~app_ext_info.func_switch.PhaseSWC;
        term->op->tprintf(term, "PhaseSWC\t\t%d\t\n", app_ext_info.func_switch.PhaseSWC);
    }
    else if (__strcmp(xsh->argv[2], "indentifySWC") == 0)
    {
        app_ext_info.func_switch.IndentifySWC = ~app_ext_info.func_switch.IndentifySWC;
        term->op->tprintf(term, "IndentifySWC\t\t%d\n", app_ext_info.func_switch.IndentifySWC);
    }
    else if (__strcmp(xsh->argv[2], "dataAddrFilterSWC") == 0)
    {
        app_ext_info.func_switch.DataAddrFilterSWC = ~app_ext_info.func_switch.DataAddrFilterSWC;
        term->op->tprintf(term, "DataAddrFilterSWC\t\t%d\n", app_ext_info.func_switch.DataAddrFilterSWC);
    }
    else if (__strcmp(xsh->argv[2], "NetSenseSWC") == 0)
    {
        app_ext_info.func_switch.NetSenseSWC = ~app_ext_info.func_switch.NetSenseSWC;
        term->op->tprintf(term, "NetSenseSWC\t\t%d\n", app_ext_info.func_switch.NetSenseSWC);
    }
    else if (__strcmp(xsh->argv[2], "SignalQualitySWC") == 0)
    {
        app_ext_info.func_switch.SignalQualitySWC = ~app_ext_info.func_switch.SignalQualitySWC;
        term->op->tprintf(term, "SignalQualitySWC\t\t%d\n", app_ext_info.func_switch.SignalQualitySWC);
    }
    else if (__strcmp(xsh->argv[2], "SetBandSWC") == 0)
    {
        app_ext_info.func_switch.SetBandSWC = ~app_ext_info.func_switch.SetBandSWC;
        term->op->tprintf(term, "SetBandSWC\t\t%d\n", app_ext_info.func_switch.SetBandSWC);
    }
    else if (__strcmp(xsh->argv[2], "SwtichPhase") == 0)
    {
        app_ext_info.func_switch.SwPhase = ~app_ext_info.func_switch.SwPhase;
        setSwPhase(app_ext_info.func_switch.SwPhase);
        term->op->tprintf(term, "SwtichPhase\t\t%d\n", app_ext_info.func_switch.SwPhase);
        setSwPhase(app_ext_info.func_switch.SwPhase);
    }
    else if (__strcmp(xsh->argv[2], "oop20EvetSWC") == 0)
    {
        app_ext_info.func_switch.oop20EvetSWC = ~app_ext_info.func_switch.oop20EvetSWC;
        term->op->tprintf(term, "oop20EvetSWC\t\t%d\n", app_ext_info.func_switch.oop20EvetSWC);
    }
    else if (__strcmp(xsh->argv[2], "oop20BaudSWC") == 0)
    {
        app_ext_info.func_switch.oop20BaudSWC = ~app_ext_info.func_switch.oop20BaudSWC;
        term->op->tprintf(term, "oop20BaudSWC\t\t%d\n", app_ext_info.func_switch.oop20BaudSWC);
    }
    else if (__strcmp(xsh->argv[2], "JZQBaudSWC") == 0)
    {
        app_ext_info.func_switch.JZQBaudSWC = ~app_ext_info.func_switch.JZQBaudSWC;
        term->op->tprintf(term, "JZQBaudSWC\t\t%d\n", app_ext_info.func_switch.JZQBaudSWC);
    }
    else if (__strcmp(xsh->argv[2], "MinCollectSWC") == 0)
    {
        app_ext_info.func_switch.MinCollectSWC = ~app_ext_info.func_switch.MinCollectSWC;
        term->op->tprintf(term, "MinCollectSWC\t\t%d\n", app_ext_info.func_switch.MinCollectSWC);
    }
    else if (__strcmp(xsh->argv[2], "IDInfoGetModeSWC") == 0)
    {
        app_ext_info.func_switch.IDInfoGetModeSWC = ~app_ext_info.func_switch.IDInfoGetModeSWC;
        term->op->tprintf(term, "IDInfoGetModeSWC\t\t%d\n", app_ext_info.func_switch.IDInfoGetModeSWC);
    }
    else if (__strcmp(xsh->argv[2], "transModeSWC") == 0)
    {
        app_ext_info.func_switch.TransModeSWC = ~app_ext_info.func_switch.TransModeSWC;
        term->op->tprintf(term, "TransModeSWC\t\t%d\n", app_ext_info.func_switch.TransModeSWC);
    }
    else if (__strcmp(xsh->argv[2], "AddrShiftSWC") == 0)
    {
        app_ext_info.func_switch.AddrShiftSWC = ~app_ext_info.func_switch.AddrShiftSWC;
        term->op->tprintf(term, "AddrShiftSWC\t\t%d\n", app_ext_info.func_switch.AddrShiftSWC);
    }
    DefwriteFg.FunSWC = TRUE;
    SetDefaultInfo();
}
void parameterCFG_info(xsh_t *xsh)
{
    U8 data = 0;
    tty_t *term = xsh->term;
    if (__strcmp(xsh->argv[2], "show") == 0)
    {
        show_parameterCFG(term);
        return;
    }
    if (__strcmp(xsh->argv[2], "useMode") == 0)
    {
        app_ext_info.param_cfg.UseMode = app_ext_info.param_cfg.UseMode > 0 ? 0 : 1;
        term->op->tprintf(term, "UseMode\t\t%d\n", app_ext_info.param_cfg.UseMode);
    }
    else if (__strcmp(xsh->argv[2], "concurrencySize") == 0)
    {
        data = __atoi(xsh->argv[3]);
        app_ext_info.param_cfg.ConcurrencySize =
#if defined(STATIC_MASTER)
            SET_PARAMETER(data, 5, MAX_WIN_SIZE, CONCURRENCYSIZE);
        extern multi_trans_header_t F1F1_TRANS_LIST;
        F1F1_TRANS_LIST.thr = app_ext_info.param_cfg.ConcurrencySize;
#else
            SET_PARAMETER(data, 0, 0x99, 0);
#endif
        term->op->tprintf(term, "concurrencySize\t\t%d\n", app_ext_info.param_cfg.ConcurrencySize);
    }
    else if (__strcmp(xsh->argv[2], "reTranmitMaxNum") == 0)
    {
        data = __atoi(xsh->argv[3]);
        app_ext_info.param_cfg.ReTranmitMaxNum =
#if defined(STATIC_MASTER)
            SET_PARAMETER(data, 1, 12, 6);
#else
            SET_PARAMETER(data, 0, 0x99, 0);
#endif
        term->op->tprintf(term, "ReTranmitMaxNum\t\t%d\n", app_ext_info.param_cfg.ReTranmitMaxNum);
    }
    else if (__strcmp(xsh->argv[2], "reTranmitMaxTime") == 0)
    {
        data = __atoi(xsh->argv[3]);
        app_ext_info.param_cfg.ReTranmitMaxTime =
#if defined(STATIC_MASTER)
            SET_PARAMETER(data, 10, 100, 40);
#else
            SET_PARAMETER(data, 0, 0x99, 0);
#endif
        term->op->tprintf(term, "ReTranmitMaxTime\t\t%d\n", app_ext_info.param_cfg.ReTranmitMaxTime);
    }
    else if (__strcmp(xsh->argv[2], "broadcastConRMNum") == 0)
    {
        data = __atoi(xsh->argv[3]);
        app_ext_info.param_cfg.BroadcastConRMNum =
#if defined(STATIC_MASTER)
            SET_PARAMETER(data, 0, 20, 1);
#else
            SET_PARAMETER(data, 0, 0x99, 0);
#endif
        term->op->tprintf(term, "broadcastConRMNum\t\t%d\n", app_ext_info.param_cfg.BroadcastConRMNum);
    }
    else if (__strcmp(xsh->argv[2], "allbd") == 0)
    {
        data = __atoi(xsh->argv[3]);
        app_ext_info.param_cfg.AllBroadcastType =
#if defined(STATIC_MASTER)
            SET_PARAMETER(data, 0, 20, 1);
#else
            SET_PARAMETER(data, 0, 0x99, 0);
#endif
        term->op->tprintf(term, "AllBroadcastType\t\t%d\n", app_ext_info.param_cfg.AllBroadcastType);
    }

    DefwriteFg.ParaCFG = TRUE;
    SetDefaultInfo();
}

void set_manuf(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    debuglen = __strlen(xsh->argv[2]);

    term->op->tprintf(term, "debuglen = %d\n", debuglen);
    __memcpy(debugbuf, (uint8_t *)xsh->argv[2], debuglen);
    U8 buf[5] = {0};
    U8 len = 10;
    len = convert_buf(buf, &debugbuf[4], len);
    if (debuglen != 14)
    {
        setoutversion(NULL, NULL, NULL, NULL);
    }
    else
    {
        setoutversion(debugbuf, debugbuf + 2, buf, &buf[3]);
    }
}
void set_nodeid(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    debuglen = __strlen(xsh->argv[2]);
    debuglen = convert_buf(debugbuf, (uint8_t *)xsh->argv[2], debuglen);
    debuglen = (debuglen > 11 ? 11 : debuglen);
    term->op->tprintf(term, "ModuleIDTem ->\n");
    dump_tprintf(term, debugbuf, debuglen);
    DefSetInfo.ModuleIDInfo.ModuleIDFattributes = TRUE;
    DefwriteFg.MID = TRUE;
    memset(ModuleID, 0x00, 11);
    __memcpy(ModuleID, debugbuf, debuglen);
    SetDefaultInfo();

    term->op->tprintf(term, "\n");
}
void set_chipid(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    debuglen = __strlen(xsh->argv[2]);
    debuglen = convert_buf(debugbuf, (uint8_t *)xsh->argv[2], debuglen);
    debuglen = (debuglen > 24 ? 24 : debuglen);
    term->op->tprintf(term, "ChipIDTem ->\n");
    dump_tprintf(term, debugbuf, debuglen);

    DefwriteFg.CID = TRUE;
    memset(ManageID, 0x00, 24);
    __memcpy(ManageID, debugbuf, debuglen);
    __memcpy(DefSetInfo.ChipIdFlash_t.ChipID, ManageID, 24);

    SetDefaultInfo();
    term->op->tprintf(term, "\n");
}
void write_efuse(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    uint32_t data[6] = {0};
    // cpu_freq_shift(4);
    debuglen = __strlen(xsh->argv[2]);
    debuglen = convert_buf(debugbuf, (uint8_t *)xsh->argv[2], debuglen);
    debuglen = (debuglen > 24 ? 24 : debuglen);
    term->op->tprintf(term, "ChipIDTem ->\n");
    dump_tprintf(term, debugbuf, debuglen);
    term->op->tprintf(term, "\n");
    __memcpy(data, debugbuf, debuglen);
    efuse_write(data, 6);
}
void read_efuse(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    uint32_t data[6] = {0};
    memset(ManageID, 0x00, 24);
    efuse_read(data, 6);
    __memcpy(ManageID, data, sizeof(data));
    dump_tprintf(term, ManageID, 24);
    term->op->tprintf(term, "\n");
}
void def_write(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    debuglen = __strlen(xsh->argv[2]);
    debuglen = convert_buf(debugbuf, (uint8_t *)xsh->argv[2], debuglen);

    term->op->tprintf(term, "e_WRITE_HARDWARE_DEF ->");
    dump_tprintf(term, debugbuf, debuglen);
    if (check_sum(debugbuf, debuglen - 1) == debugbuf[debuglen - 1])
    {
        if (CheckDefInfo(debugbuf, debuglen - 1) == TRUE)
        {
            SetDefaultInfo();
        }
        else
        {
            term->op->tprintf(term, "SetDefaultInfo err\n");
        }
        term->op->tprintf(term, "\n");
    }
    else
    {
        term->op->tprintf(term, "check_sum err\n");
    }
}
void showecc(xsh_t *xsh)
{
    U8 *ecc_buf = NULL;
    tty_t *term = xsh->term;

    ecc_buf = zc_malloc_mem(170, "Show ecc", MEM_TIME_OUT);
    if(ecc_buf == NULL)
    {
        term->op->tprintf(term,"ecc_buf is NULL\n");
        return;
    }
    memset(ecc_buf,0,170);

    read_ecc_sm2_info(ecc_buf,e_ECC_KEY);

    term->op->tprintf(term,"ecc string:\n");

    dump_tprintf(term, ecc_buf, sizeof(ECC_KEY_STRING_t)-3);
    zc_free_mem(ecc_buf);
}
void showsm2(xsh_t *xsh)
{
    U8 *sm2_buf = NULL;
    tty_t *term = xsh->term;

    sm2_buf = zc_malloc_mem(170, "Show sm2", MEM_TIME_OUT);
    if(sm2_buf == NULL)
    {
        term->op->tprintf(term,"sm2_buf is NULL\n");
        return;
    }
    memset(sm2_buf,0,170);
    
    read_ecc_sm2_info(sm2_buf,e_SM2_KEY);

    term->op->tprintf(term,"sm2 string:\n");

    dump_tprintf(term, sm2_buf, sizeof(SM2_KEY_STRING_t)-3);
    zc_free_mem(sm2_buf);
}
static U8 g_Key_write_protect[2] = {0};  //加密密钥写保护标志
void setecc_1(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    debuglen = __strlen(xsh->argv[2]);
    memset(debugbuf,0,200);
    debuglen = convert_buf(debugbuf, (uint8_t *)xsh->argv[2], debuglen);

    if(debuglen != 64)
    {
        term->op->tprintf(term,"ecc_1 len error,must len :64,debuglen:%d\n",debuglen);
         return;
    }
    g_Key_write_protect[e_ECC_KEY] = 1;
    term->op->tprintf(term, "ecc1: ->\n");
    dump_tprintf(term, debugbuf, debuglen);

    //write_ecc_sm2_info(debugbuf,e_SM2_KEY,debuglen);
    //term->op->tprintf(term, "\n");
}
void setecc_2(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if(g_Key_write_protect[e_ECC_KEY] ==1)
    {
        g_Key_write_protect[e_ECC_KEY] = 0;
    }
    else 
    {
        term->op->tprintf(term,"Please first input ecc_1\n");
        return;
    }
    debuglen = __strlen(xsh->argv[2]);
    debuglen = convert_buf(debugbuf+64, (uint8_t *)xsh->argv[2], debuglen);
    if(debuglen != 97)
    {
        term->op->tprintf(term,"ecc_2 len error,must len :97,debuglen:%d\n",debuglen);
         return;
    }
    
    term->op->tprintf(term, "ecc2: ->\n");
    dump_tprintf(term, debugbuf, 161);

    write_ecc_sm2_info(debugbuf,e_ECC_KEY,161+4);
    term->op->tprintf(term, "\n");
}
void setsm2_1(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    debuglen = __strlen(xsh->argv[2]);
    memset(debugbuf,0,200);
     debuglen = convert_buf(debugbuf, (uint8_t *)xsh->argv[2], debuglen);
    if(debuglen != 64)
    {
        term->op->tprintf(term,"sm2_1 len error,must len :64,debuglen:%d\n",debuglen);
         return;
    }
    g_Key_write_protect[e_SM2_KEY] = 1;
    term->op->tprintf(term, "sm2_1: ->\n");
    dump_tprintf(term, debugbuf, debuglen);
}
void setsm2_2(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if(g_Key_write_protect[e_SM2_KEY] == 1)
    {
        g_Key_write_protect[e_SM2_KEY] = 0;
    }
    else
    {
        term->op->tprintf(term,"Please first input sm2_1\n");
        return;
    }
    debuglen = __strlen(xsh->argv[2]);
    debuglen = convert_buf(debugbuf+64, (uint8_t *)xsh->argv[2], debuglen);
    if(debuglen != 97)
    {
        term->op->tprintf(term,"sm2_2 len error,must len :97,debuglen:%d\n",debuglen);
         return;
    }
    
    term->op->tprintf(term, "sm2_2: ->\n");
    dump_tprintf(term, debugbuf, 161);

    write_ecc_sm2_info(debugbuf,e_SM2_KEY,161+4);
    term->op->tprintf(term, "\n");
}
void indentification(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if (__strcmp(xsh->argv[2], "start") == 0)
    {
#if defined(STATIC_MASTER)
        cco_area_ind_task_cfg(e_DISTRIBUT, period_bit);
#endif
    }
    if (__strcmp(xsh->argv[2], "show") == 0)
    {
        U16 i = 0;
#if defined(STATIC_NODE)
        term->op->tprintf(term, "seq  SNID    Macaddr    Differtime-FinishFlag-SameRatio-seq-MaxNum-currNum\n");
        for (i = 0; i < COMMPAER_NUM; i++)
        {
            if (Final_result_t.PublicResult_t[i].SNID != 0)
                term->op->tprintf(term, "%d  0x%08x %02x%02x%02x%02x%02x%02x 0x%08x %d %d %d %d %d\n",
                                  i,
                                  Final_result_t.PublicResult_t[i].SNID,
                                  Final_result_t.PublicResult_t[i].Macaddr[0],
                                  Final_result_t.PublicResult_t[i].Macaddr[1],
                                  Final_result_t.PublicResult_t[i].Macaddr[2],
                                  Final_result_t.PublicResult_t[i].Macaddr[3],
                                  Final_result_t.PublicResult_t[i].Macaddr[4],
                                  Final_result_t.PublicResult_t[i].Macaddr[5],
                                  Final_result_t.PublicResult_t[i].Differtime,
                                  Final_result_t.PublicResult_t[i].FinishFlag,
                                  Final_result_t.PublicResult_t[i].CalculateNum == 0 ? 0 : Final_result_t.PublicResult_t[i].FinishFlag == 1 ? Final_result_t.PublicResult_t[i].SameRatio
                                                                                                                                            : Final_result_t.PublicResult_t[i].SameRatio / Final_result_t.PublicResult_t[i].CalculateNum,
                                  Final_result_t.PublicResult_t[i].Collectseq,
                                  Final_result_t.PublicResult_t[i].CalculateMaxNum,
                                  Final_result_t.PublicResult_t[i].CalculateNum);
        }
        if (Final_result_t.FinishFlag == FINISH)
        {
            term->op->tprintf(term, "STA RESULT IS ok,it's belong to %02x%02x%02x%02x%02x%02x\n", Final_result_t.CCOMacaddr[0], Final_result_t.CCOMacaddr[1], Final_result_t.CCOMacaddr[2], Final_result_t.CCOMacaddr[3], Final_result_t.CCOMacaddr[4], Final_result_t.CCOMacaddr[5]);
        }
        else
        {
            term->op->tprintf(term, "please wait !!!\n");
            if (zc_timer_query(wait_finish_timer) == TMR_RUNNING)
                term->op->tprintf(term, "remaint : %d\n", (timer_remain(wait_finish_timer) / 1000));
        }
#else
        U16 num = 0;
        U8 macaddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
        term->op->tprintf(term, "Macaddr Chm Result\n");
        for (i = 0; i < MAX_WHITELIST_NUM; i++)
        {
            if (0 == memcmp(DeviceTEIList[i].MacAddr, macaddr, 6))
                continue;
            num++;
            term->op->tprintf(term, "%02x%02x%02x%02x%02x%02x %c%c %d\n",
                              DeviceTEIList[i].MacAddr[0],
                              DeviceTEIList[i].MacAddr[1],
                              DeviceTEIList[i].MacAddr[2],
                              DeviceTEIList[i].MacAddr[3],
                              DeviceTEIList[i].MacAddr[4],
                              DeviceTEIList[i].MacAddr[5],
                              DeviceTEIList[i].ManufactorCode[1],
                              DeviceTEIList[i].ManufactorCode[0],
                              DeviceTEIList[i].AREA_ERR);
        }
        term->op->tprintf(term, "count : %d\n", num);
#endif
    }
}
void set_Rfband(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    uint8_t option;
    uint32_t channel;
    option = __strtoul(xsh_getopt_val(xsh->argv[2], "-option="), NULL, 10);
    channel = __strtoul(xsh_getopt_val(xsh->argv[3], "-channel="), NULL, 10);
    term->op->tprintf(term, "set setRfband: <%d ,%d>\n", option, channel);

    if(checkRfChannel(option, channel) == FALSE)
    {
        app_printf("rfband err, op:%d,chl:%d\n", option, channel);
        return;
    }

#if defined(STATIC_MASTER)
    U16 RemianTime;
    RemianTime = __atoi(xsh->argv[4]);
    DefSetInfo.RfChannelInfo.option = option;
    DefSetInfo.RfChannelInfo.channel = channel;
    DefwriteFg.RfChannel = TRUE;
    DefaultSettingFlag = TRUE;

    nnib.RfChannelChangeState = TRUE;
    nnib.RfChgChannelRemainTime = RemianTime; // 100s
#else
    changRfChannel(option, channel);
#endif
}
#if defined(STATIC_MASTER)
void judge_transmode_swc()
{
	if(app_ext_info.func_switch.TransModeSWC == 1)
	{
		 APP_GW3762_TRANS_MODE = APP_GW3762_HPLC_HRF_CTRL_COMMMODE;
	}
    else
	{
		APP_GW3762_TRANS_MODE = APP_GW3762_HPLC_CTRL_COMMMODE;
	}
    app_printf("APP_GW3762_TRANS_MODE = %d\n",APP_GW3762_TRANS_MODE);
}
#endif