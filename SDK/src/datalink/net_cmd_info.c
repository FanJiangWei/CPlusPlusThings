#include "DatalinkTask.h"
#include "cmd.h"
#include "que.h"
#include "Beacon.h"

#include "plc_cnst.h"
#include "SyncEntrynet.h"
#include "phy.h"
#include "DataLinkGlobal.h"
#include "mem_zc.h"
#include "dl_mgm_msg.h"
#include "SchTask.h"
#include "plc.h"
#include "Datalinkdebug.h"

#include "printf_zc.h"
#include "ProtocolProc.h"
#include "app_gw3762.h"
#include "Scanbandmange.h"
#include "app_dltpro.h"
#include "meter.h"
#include "net_cmd_info.h"

param_t cmd_net_param_tab[] =
{
    { PARAM_ARG | PARAM_TOGGLE, "", "show|Appreq|NbNet|DeepthRecord|DefineParent|Readnnib|MmsgReacord|MemCheck|Test|showRfbandMap" },
    { PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB, "", "Qtopology|Smacaddr|Qmacaddr|Saccessinfo|Qaccessinfo", "Appreq" },

    { PARAM_ARG | PARAM_TOGGLE	| PARAM_SUB, "", "neigb|Route|DevList|mgrcd|NetInfo", "show" },

    { PARAM_ARG | PARAM_TOGGLE	| PARAM_SUB, "", "AreaNotifyBuff|RelationSwitch|WhitelistSwitch|AccessListSwitch\
|Delroute|AodvState|SeqNumAdd|Stapower|Levelset|edgecln|RfcftRpt|Sendmode|NetRpt\
|StopNetMnt|DelDevTei|SnidSet|sendproxy", "Test" },
//	{ PARAM_OPT | PARAM_TOGGLE	| PARAM_SUB | PARAM_OPTIONAL, "SendType", "1|2|3|4", "ChangeType" },//参数2
//	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "switch", "on|off", "UseMacAddr" },
	
    { PARAM_ARG | PARAM_MAC | PARAM_SUB,				  "Macaddr", "", "Smacaddr" },//参数1
    { PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "Devicetype", "ckq|jzq|meter|relay|icjq|iicjq", "Smacaddr" },//参数2

	{ PARAM_ARG | PARAM_MAC | PARAM_SUB,				  "Macaddr1", "", "Saccessinfo" },//参数1
	{ PARAM_ARG | PARAM_MAC | PARAM_SUB,				  "Macaddr2", "", "Saccessinfo" },//参数2
	{ PARAM_ARG | PARAM_MAC | PARAM_SUB,				  "Macaddr3", "", "Saccessinfo" },//参数2

	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,				  "dig", "", "Stapower" },//参数1
	{ PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,				  "ana", "", "Stapower" },//参数1
	
    { PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,                 "LinkMode", "", "Sendmode" },//参数1
    { PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,                 "ChangeTime", "", "Sendmode" },//参数2
    { PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL,                 "DurationTime", "", "Sendmode" },//参数3

	
	{ PARAM_ARG | PARAM_STRING | PARAM_SUB,				  "DelTEI", "", "Delroute" },//参数1
    { PARAM_ARG | PARAM_STRING | PARAM_SUB,				  "DelTEI", "", "DelDevTei" },//参数1
    { PARAM_ARG | PARAM_STRING | PARAM_SUB,				  "DelTEI", "", "SnidSet" },//参数1

//	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "status", "on|off", "SwitchOnDiscovery" },
		{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "status", "on|off", "WhitelistSwitch" },

	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "status", "on|off", "AccessListSwitch" },
	{ PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "status", "on|off", "RelationSwitch" },


	{PARAM_ARG | PARAM_STRING | PARAM_SUB, "num", "","Levelset"},
    {PARAM_ARG | PARAM_MAC | PARAM_SUB,				  "Macaddr", "", "RfcftRpt" },//参数1
    {PARAM_ARG | PARAM_STRING | PARAM_SUB,            "option", "","RfcftRpt"},   //参数2
    {PARAM_ARG | PARAM_STRING | PARAM_SUB,            "channel", "","RfcftRpt"},  //参数3
    {PARAM_ARG | PARAM_MAC | PARAM_SUB,				  "Macaddr", "", "NetRpt" },//参数1   
    {PARAM_ARG | PARAM_STRING | PARAM_SUB,				  "Reason", "", "sendproxy" },//参数1   

    PARAM_EMPTY
};

CMD_DESC_ENTRY(cmd_net_desc, "", cmd_net_param_tab);

void docmd_net(command_t *cmd, xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if(xsh->argc == 1)
    {
        return;
    }

    if (__strcmp(xsh->argv[1], "Appreq") == 0) //--------------------------------APP消息接口测试
    {
        if(__strcmp(xsh->argv[2], "Smacaddr") == 0)//设置MAC地址
        {
           net_appreq_smacaddr(xsh);
        }
        else if(__strcmp(xsh->argv[2], "Saccessinfo") == 0)
        {
            net_appreq_saccessinfo(xsh);
        }
        else if(__strcmp(xsh->argv[2], "Qaccessinfo") == 0)//查询访问列表
        {
            net_appreq_qaccessinfo(xsh->term);
        }
        else if(__strcmp(xsh->argv[2], "Qmacaddr") == 0)//查询MAC地址和设备类型
        {
            QueryMacAddrRequest();
        }
        else if(__strcmp(xsh->argv[2], "Qtopology") == 0)//拓扑查询
        {
   			extern void printNetAQ(tty_t *term);
			printNetAQ(term);
        }
    }
	else if(__strcmp(xsh->argv[1], "show") == 0)//查询消息处理耗时
	{
        if(__strcmp(xsh->argv[2], "neigb") == 0)
        {
            printNeighborDiscoveryTableEntry();
        }
        else if(__strcmp(xsh->argv[2], "Route") == 0)
        {
            printNetRouteInfo();
        }
        else if(__strcmp(xsh->argv[2], "DevList") == 0)
        {
            printDevListInfo();
        }
        else if(__strcmp(xsh->argv[2], "mgrcd") == 0)
        {
		    show_mg_record(term);
        }
        else if(__strcmp(xsh->argv[2], "NetInfo") == 0)
        {
            #if defined(STATIC_MASTER)
            //打印网络统计信息，档案数、入网节点数、最大层级、个层级数量 等。
            printNetInfo();
            #endif
        }
	}
    else if(__strcmp(xsh->argv[1], "NbNet") == 0)//查询邻居网络信息
    {
        net_queryinfo(xsh->term);
    }
    else if (__strcmp(xsh->argv[1], "Readnnib") == 0) //----------------------查询网络属性库
    {
       net_readnnib(xsh->term);
    }
    else if (__strcmp(xsh->argv[1], "MmsgReacord") == 0)
    {
		extern void showMmsgData(tty_t *term);
		showMmsgData( term);
    }
#if defined(STATIC_MASTER)
    else if (__strcmp(xsh->argv[1], "DefineParent") == 0)
    {
        net_defineparent(xsh->term);
    }
    else if (__strcmp(xsh->argv[1], "DeepthRecord") == 0)
    {
        extern void showDeepthrecord(tty_t * term);
        showDeepthrecord(term);
    }
#endif
    else if(__strcmp(xsh->argv[1], "Test") == 0)
    {

        if(__strcmp(xsh->argv[2], "Delroute") == 0)
        {
           net_test_delroute(xsh);
        }
        else if(__strcmp(xsh->argv[2], "AccessListSwitch") == 0)
        {
            net_test_accesslistswitch(xsh);
        }
        else if(__strcmp(xsh->argv[2], "AreaNotifyBuff") == 0)
        {
			extern void showAreaNotifyBuff(tty_t *term);
				showAreaNotifyBuff(term);
        }
        else if(__strcmp(xsh->argv[2], "RelationSwitch") == 0)
        {
            net_test_relationswitch(xsh);
        }
        else if(__strcmp(xsh->argv[2], "Levelset") == 0)
        {
            net_test_levelset(xsh);
        }
        else if (__strcmp(xsh->argv[2], "StopNetMnt") == 0) // 停止发送网络维护帧
        {
            term->op->tprintf(term, "StopNetworkMainten...\n");
            StopNetworkMainten();
        }
#if defined(STATIC_MASTER)
        else if (__strcmp(xsh->argv[2], "WhitelistSwitch") == 0)
        {
           net_test_whitelistswitch(xsh);
        }
        else if (__strcmp(xsh->argv[2], "DelDevTei") == 0) // 删除入网节点信息
        {
           net_test_deldevtei(xsh);
        }
        else if (__strcmp(xsh->argv[2], "AodvState") == 0)
        {
           net_test_aodvstate(xsh->term);
        }
        else if (__strcmp(xsh->argv[2], "SeqNumAdd") == 0)
        {
           IncreaseFromatSeq();
        }

        else if (__strcmp(xsh->argv[2], "edgecln") == 0)
        {
           net_printf("clean tei2 edge");
           DeviceTEIList[0].Edgeinfo = 2;
        }
        else if (__strcmp(xsh->argv[2], "Stapower") == 0)
        {
           net_test_stapower(xsh);
        }
        else if(__strcmp(xsh->argv[2], "Sendmode") == 0 )
        {

            net_test_sendmode(xsh);
        }
        else if (__strcmp(xsh->argv[2], "SnidSet") == 0)
        {
            net_test_snidset(xsh);
        }
#endif        
#if defined(STATIC_NODE)
        else if (__strcmp(xsh->argv[2], "RfcftRpt") == 0)
        {
            net_test_rfcftrpt(xsh);
        }
        else if (__strcmp(xsh->argv[2], "NetRpt") == 0)
        {
            net_test_netrpt(xsh);
        }
        else if(__strcmp(xsh->argv[2], "sendproxy") == 0)
        {
            U8 reason = __atoi(xsh->argv[3]);
            sendproxychangeDelay(reason);
        }
#endif   
    }
    else if((__strcmp(xsh->argv[1], "showRfbandMap") == 0))
    {
        net_showrfbandmap(xsh);
    }

}

#if defined (STATIC_MASTER)

void net_test_deldevtei(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U16 tei;
    if (convert_string((U8 *)&tei, xsh->argv[3], 4))
    {
        term->op->tprintf(term, "DelDevTei--%04x-----\n", tei);
        if (tei < 1 || tei > MAX_WHITELIST_NUM - 1)
            return;

        DelDeviceListEach(tei);
    }
}

void net_test_whitelistswitch(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    char *s;
    if ((s = xsh_getopt_val(xsh->argv[3], "-status=")))
    {
        if (__strcmp(s, "on") == 0)
        {
            app_ext_info.func_switch.WhitelistSWC = 1;
            term->op->tprintf(term, "WhitelistSwitch is on\n");
        }
        else
        {
            app_ext_info.func_switch.WhitelistSWC = 0;
            term->op->tprintf(term, "WhitelistSwitch is off\n");
        }
    }
}
void net_test_aodvstate(tty_t *term )
{
    if (nnib.AODVRouteUse == TRUE)
    {
        nnib.AODVRouteUse = FALSE;
        term->op->tprintf(term, "--AODVRouteUse OFF---\n");
    }
    else
    {
        nnib.AODVRouteUse = TRUE;
        term->op->tprintf(term, "--AODVRouteUse ON---\n");
    }
}
void net_test_stapower(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    S16 dig = 0, ana = 0;
    dig = __atoi(xsh->argv[3]);
    ana = __atoi(xsh->argv[4]);
    if (dig < 0)
    {
        dig = 0;
    }
    else if (dig > 8)
    {
        dig = 8;
    }
    if (ana < -20)
    {
        ana = -20;
    }
    else if (ana > 10)
    {
        ana = 10;
    }

    term->op->tprintf(term, "--Stapower dig=%d,ana=%d---\n", dig, ana);
    nnib.FormationSeqNumber++;
    nnib.powerlevelSet = TRUE;
    //            extern uint8_t sysflag;
    sysflag = TRUE;
    SaveInfo(&sysflag, &whitelistflag);
    nnib.TGAINDIG = dig;
    nnib.TGAINANA = ana;
}
void net_test_sendmode(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 LinkMode;      // 0x00：默认0，不进行设置；0x01：单载波工作模式；0x02：单无线工作模式；0x03：双模工作模式。其他：保留
    U16 ChangeTime;   // unit:s
    U16 DurationTime; // unit:s

    LinkMode = 0;
    ChangeTime = 0;
    DurationTime = 0;
    LinkMode = __atoi(xsh->argv[3]);
    ChangeTime = __atoi(xsh->argv[4]);
    DurationTime = __atoi(xsh->argv[5]);
    if (LinkMode > 0 && LinkMode <= e_link_mode_dual)
    {
        term->op->tprintf(term, "Slm= %d,Ctm=%ds,Dtm=%ds\n", LinkMode, ChangeTime, DurationTime);

        nnib.LinkModeStart = TRUE;
        nnib.LinkMode = LinkMode;
        nnib.ChangeTime = ChangeTime;
        nnib.DurationTime = DurationTime;
    }
}
void net_test_snidset(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    uint32_t snid;
    snid = 0;

    if (convert_string((U8 *)&snid, xsh->argv[3], 6))
    {
        SetPib(snid, CCO_TEI);
        term->op->tprintf(term, "--SetSNID=%x---\n", GetSNID());
    }
}
void net_defineparent(tty_t *term)
{
    DeviceTEIList[0].ParentTEI = 0x0003;
    term->op->tprintf(term, "-------------------%04x-------------------\n", DeviceTEIList[0].ParentTEI);
}

#endif

#if defined(STATIC_NODE)

void net_test_rfcftrpt(xsh_t *xsh)
{
    tty_t *term = xsh->term;

    U8 option   = getHplcOptin();
    U16 channel = getHplcRfChannel();

    if(xsh->argc > 3)       //option
    {
        option = __atoi(xsh->argv[4]);
    }

    if(xsh->argc > 4)       //channel
    {
        channel = __atoi(xsh->argv[5]);
    }

    U8 MacAddr[6] = {0};
    if (convert_string(MacAddr, xsh->argv[3], 12))
    {
        term->op->tprintf(term, "RfcftRpt: %02x %02x %02x %02x %02x %02x, <%d, %d>\n"
                                , MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]
                                , option, channel);

        SendMMeRFChannelConflictReport(MacAddr, 0, TRUE, option, channel);
    }
}
void net_test_netrpt(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 MacAddr[6] = {0};
    if (convert_string(MacAddr, xsh->argv[3], 12))
    {
        term->op->tprintf(term, "NetRpt: %02x %02x %02x %02x %02x %02x\n", MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]);

        SendNetworkConflictReport(MacAddr, 0, TRUE);
    }
}
#endif


S8 check_version(U8  *ManufactorCode, U8 *check_ver,U8 * check_innerver , VERSION_t *version , U8 cnt)
{
    U16 ii=0;
    U16 null_seq;
    U8 buf[2]={0,0};
    U8 check_version[2]={0};

    null_seq = 0xFFFF;
    if(ManufactorCode[0]==Vender2&&ManufactorCode[1]==Vender1)
    {
        __memcpy(check_version,check_innerver,2);
    }
    else
    {
        __memcpy(check_version,check_ver,2);
    }
    for(ii=0;ii<cnt;ii++)
    {
        if(0 == memcmp(check_version,version[ii].version,2))
        {
            version[ii].cnt++;
            return OK;
        }
        if(0xFFFF == null_seq && 0 == memcmp(buf,version[ii].version,2))
        {
            null_seq = ii;
        }
    }
    if(cnt == ii && 0xFFFF != null_seq)
    {
        version[null_seq].cnt=1;
        __memcpy(version[null_seq].version,check_version,2);
        return OK;
    }
    return ERROR;
}

void net_appreq_smacaddr(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 MacAddr[6];
    U8 typenum = 0;
    char *s;
    if (convert_string(MacAddr, xsh->argv[3], 12))
    {
        if ((s = xsh_getopt_val(xsh->argv[4], "-Devicetype=")))
        {
            if (__strcmp(s, "ckq") == 0)
            {
                typenum = e_CKQ;
            }
            else if (__strcmp(s, "jzq") == 0)
            {
                typenum = e_JZQ;
            }
            else if (__strcmp(s, "meter") == 0)
            {
                typenum = e_METER;
            }
            else if (__strcmp(s, "relay") == 0)
            {
                typenum = e_RELAY;
            }
            else if (__strcmp(s, "icjq") == 0)
            {
                typenum = e_CJQ_1;
            }
            else if (__strcmp(s, "iicjq") == 0)
            {
                typenum = e_CJQ_2;
            }
        }

        SetMacAddrRequest(MacAddr, typenum, e_MAC_METER_ADDR);
#if defined(STATIC_MASTER)
        __memcpy(FlashInfo_t.ucJZQMacAddr, MacAddr, 6);
        sysflag = TRUE;
        mutex_post(&mutexSch_t);
#endif
    }
    else
    {
        term->op->tprintf(term, "input format error !\n");
    }
}

void net_appreq_saccessinfo(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 MacAddr[6] = {0}, MacAddrCmp[6] = {0}, Number = 0;
    if (convert_string(MacAddr, xsh->argv[3], 12))
    {
        term->op->tprintf(term, "MacAddr1: %02x %02x %02x %02x %02x %02x\n", MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]);
        if (memcmp(MacAddr, MacAddrCmp, 6) != 0)
        {
            Number++;
            __memcpy(AccessControlListrow.AccessControlList[0].MacAddr, MacAddr, 6);
            AccessControlListrow.AccessControlList[0].PermitOrDeny = TRUE;
        }
        if (convert_string(MacAddr, xsh->argv[4], 12))
        {
            if (memcmp(MacAddr, MacAddrCmp, 6) != 0)
            {
                Number++;
                __memcpy(AccessControlListrow.AccessControlList[1].MacAddr, MacAddr, 6);
                AccessControlListrow.AccessControlList[1].PermitOrDeny = TRUE;
            }

            term->op->tprintf(term, "MacAddr2: %02x %02x %02x %02x %02x %02x\n", MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]);
            if (convert_string(MacAddr, xsh->argv[5], 12))
            {
                term->op->tprintf(term, "MacAddr3: %02x %02x %02x %02x %02x %02x\n", MacAddr[0], MacAddr[1], MacAddr[2], MacAddr[3], MacAddr[4], MacAddr[5]);
                if (memcmp(MacAddr, MacAddrCmp, 6) != 0)
                {
                    Number++;
                    __memcpy(AccessControlListrow.AccessControlList[2].MacAddr, MacAddr, 6);
                    AccessControlListrow.AccessControlList[2].PermitOrDeny = TRUE;
                }
                if (Number == 0)
                {
                    return;
                }
#if defined(STATIC_NODE)
                SaveAccessInfo();
#endif
            }
            else
            {
                term->op->tprintf(term, "input MacAddr3 error !\n");
            }
        }
        else
        {
            term->op->tprintf(term, "input MacAddr2 error !\n");
        }
    }
    else
    {
        term->op->tprintf(term, "input MacAddr1 error !\n");
    }
}

void net_appreq_qaccessinfo(tty_t *term)
{
    term->op->tprintf(term, "AccessListSwitch  :  %s\n", AccessListSwitch ? "ON" : "OFF");
    U8 i;
    for (i = 0; i < 3; i++)
    {
        if (AccessControlListrow.AccessControlList[i].PermitOrDeny == TRUE)
        {
            term->op->tprintf(term, "MacAddr:%02x %02x %02x %02x %02x %02x\n",
                              AccessControlListrow.AccessControlList[i].MacAddr[0],
                              AccessControlListrow.AccessControlList[i].MacAddr[1],
                              AccessControlListrow.AccessControlList[i].MacAddr[2],
                              AccessControlListrow.AccessControlList[i].MacAddr[3],
                              AccessControlListrow.AccessControlList[i].MacAddr[4],
                              AccessControlListrow.AccessControlList[i].MacAddr[5]);
        }
    }
}
void net_queryinfo(tty_t *term)
{
#if defined(STATIC_MASTER)
    U8 i, jj = 0;
    term->op->tprintf(term, "NIBSNID\t\tDuration\tSendStartOffset\tMyStartOffset\tLifeTime\n");
#if defined(STD_2016)

    for (i = 0; i < MaxSNIDnum; i++)
    {
        if (NeighborNet[i].SendSNID != 0xffffffff)
        {

            term->op->tprintf(term, "0x%08x\t%d\t%d\t\t%d\t\t%d\n",
                              NeighborNet[i].SendSNID,
                              NeighborNet[i].Duration,
                              NeighborNet[i].SendStartOffset,
                              NeighborNet[i].MyStartOffset,
                              NeighborNet[i].LifeTime);
            jj++;
        }
    }

#elif defined(STD_GD)
    for (i = 0; i < MaxSNIDnum; i++)
    {
        if (NeighborNet[i].LifeTime != 0xff)
        {

            term->op->tprintf(term, "0x%08x      %d          %d        %d\n",
                              i + 1,
                              0,
                              0,
                              0);
            jj++;
        }
    }
#endif
    term->op->tprintf(term, "NeighborNet num : %d\n", jj);
#else
    Sta_Show_NbNet();
#endif
}

void net_readnnib(tty_t *term)
{
    term->op->tprintf(term,
                      "Nwkflg              = %2x\n"
                      "SNID                = 0x%08x\n"
                      "StaTEI              = 0x%04x\n"
                      "NodeType            = %2x\n"
                      "NodeState           = %2x\n"
                      "StaOfflinetime      = %d\n"
                      "NodeLevel           = %d\n"
                      "ParentTEI           = %04x\n"
                      "DeviceType          = %2x\n",
                      nnib.Networkflag, GetSNID(), GetTEI(), nnib.NodeType, nnib.NodeState, nnib.StaOfflinetime, nnib.NodeLevel, GetProxy(), nnib.DeviceType);

    term->op->tprintf(term,
                      "RoutePeriod         = %d\n"
                      "SndDisLstTim        = %d\n"
                      "SucRateRptTim       = %d\n"
                      "HeartBeatTime       = %d\n"
                      "RecvDisListTime     = %d\n"
                      "RfRecvDisListTime   = %d\n"
                      "WorkState           = %d\n"
                      "mac                 = %02x%02x%02x%02x%02x%02x\n",
                      nnib.RoutePeriodTime, nnib.SendDiscoverListTime, nnib.SuccessRateReportTime, nnib.HeartBeatTime, nnib.RecvDisListTime, nnib.RfRecvDisListTime, nnib.WorkState, nnib.MacAddr[0], nnib.MacAddr[1], nnib.MacAddr[2], nnib.MacAddr[3], nnib.MacAddr[4], nnib.MacAddr[5]);

    term->op->tprintf(term,
                      "LinkType            = %d\n"
                      "RfCount             = %d\n"
                      "RfDiscoverPeriod    = %d\n"
                      "RfRcvRateOldPeriod  = %d\n"
                      "SuccessRateZerocnt  = %d\n",
                      nnib.LinkType, nnib.RfCount, nnib.RfDiscoverPeriod, nnib.RfRcvRateOldPeriod, nnib.SuccessRateZerocnt);
#if defined(STATIC_MASTER)
    term->op->tprintf(term,
                      "RfConsultEn         = %d\n"
                      ,nnib.RfConsultEn);
#endif

    term->op->tprintf(term,
                      "NetworkType         = %d\n"
                      ,nnib.NetworkType);

    if (nniberr.errflag == 1)
    {
        term->op->tprintf(term, "ramErr=%d,fmtSeq=%02x,bcn=%08x,taskid=%d,msgtype=%d\n", nniberr.ramErr, nniberr.fmtSeq, nniberr.bcn, nniberr.taskid, nniberr.msgtype);
        // dump_buf(bugrg,bugrglen);
    }
    else
    {
        term->op->tprintf(term, "nnib is ok\n");
    }
}

void net_test_delroute(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U16 tei, ii;
    if (convert_string((U8 *)&tei, xsh->argv[3], 4))
    {
        term->op->tprintf(term, "-------------------%04x-------------------\n", tei);
        if (tei < 1 || tei > NWK_MAX_ROUTING_TABLE_ENTRYS)
            return;

        memset(&NwkRoutingTable[tei - 1], 0xff, sizeof(ROUTE_TABLE_ENTRY_t));

        for (ii = 0; ii < NWK_AODV_ROUTE_TABLE_ENTRYS; ii++)
        {
            if (NwkAodvRouteTable[ii].DestTEI == tei)
            {
                term->op->tprintf(term, "@AODV DestTEI---%04x,NextHopTE---%04x,RouteState---%x,RouteError---%x\n",
                                  NwkAodvRouteTable[ii].DestTEI,
                                  NwkAodvRouteTable[ii].NextHopTEI,
                                  NwkAodvRouteTable[ii].RouteState,
                                  NwkAodvRouteTable[ii].RouteError);
                memset(&NwkAodvRouteTable[ii], 0XFF, sizeof(ROUTE_TABLE_RECD_t));
                break;
            }
        }
    }
}

void net_test_accesslistswitch(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    char *s;
    if ((s = xsh_getopt_val(xsh->argv[3], "-status=")))
    {
        if (__strcmp(s, "on") == 0)
        {
            AccessListSwitch = TRUE;
            term->op->tprintf(term, "AccessListSwitch is on");
        }
        else
        {
            AccessListSwitch = FALSE;
            term->op->tprintf(term, "AccessListSwitch is off");
        }
    }
}
void net_test_relationswitch(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    char *s;
    if ((s = xsh_getopt_val(xsh->argv[3], "-status=")))
    {
        if (__strcmp(s, "on") == 0)
        {
            g_RelationSwitch = TRUE;
            term->op->tprintf(term, "RelationSwitch is on");
        }
        else
        {
            g_RelationSwitch = FALSE;
            term->op->tprintf(term, "RelationSwitch is off");
        }
    }
}
void net_test_levelset(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 level;

    level = __atoi(xsh->argv[3]);
    term->op->tprintf(term, "Set level=%d", level);
    nnib.NodeLevel = level;
}

void net_showrfbandmap(xsh_t *xsh)
{
    tty_t *term = xsh->term;
    U8 i;
    term->op->tprintf(term, "option\tchannel\tHaveNet\tdur\n", GetSNID());
    for (i = 0; i < e_op_end; i++)
    {
        if(scanRfChArry[i].option == 0 || scanRfChArry[i].channel == 0)
                continue;
        term->op->tprintf(term, "%d\t%d\t%d\t%d\n", scanRfChArry[i].option, scanRfChArry[i].channel, scanRfChArry[i].HaveNet, scanRfChArry[i].LifeTime);
    }
}
