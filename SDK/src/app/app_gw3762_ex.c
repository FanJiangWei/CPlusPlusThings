

#include "app_cmp_version.h"
#include "ZCsystem.h"
#include "app_gw3762_ex.h"
#include "SHA1.h"
#include "app_data_freeze_cco.h"
#include "app_read_router_cco.h"
#include "app_upgrade_cco.h"
#include "app_node_sgs_ex.h"
#include "aps_interface.h"
#include "app_cnf.h"
#include "test.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "meter.h"
#include "app_clock_sync_cco.h"
#include "app_current_abnormal_event_cco.h"
#include "app.h"
#include "app_gw3762.h"



#if defined(STATIC_MASTER)

U8	staIdEntifyFlg = 0;
ostimer_t *BCOnLineFlgtimer = NULL;	//湖南-周期广播入网认证命令
extern U8  NetSenseFlag;//离网感知标志
//extern CCOCurveGatherInfo_t CcoCurveGatherInfo;
ostimer_t *Send_LiveLineZeroLinetimer = NULL;
U16 Topchangetimes = 0;
extern U8 check_whitelist_by_addr(U8 *addr);

static void app_gw3762_afn03_f130_query_soft_version(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
#if defined(CMP_VERSION)
	app_printf("AppGw3762Afn03F130QuerySoftVer!\n");
	
	U16 Gw3762SendDataLen = 0;
	APPGW3762DATA_t Gw3762updata;
	QUERY_SOFTVERSION_t SoftVer;

	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode	= pGw3762Data_t->CtrlField.TransMode;
	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,pGw3762Data_t->DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum		= 0;
	Gw3762updata.Afn				= APP_GW3762_AFN03;
	Gw3762updata.Fn 				= app_gw3762_fn_bin_to_bs(APP_GW3762_F130);
	Gw3762updata.DataUnitLen		= 0;
	
	soft_version_packet(&SoftVer);
    __memcpy(&Gw3762updata.DataUnit,(U8 *)&SoftVer,sizeof(QUERY_SOFTVERSION_t));
	Gw3762updata.DataUnitLen		= sizeof(QUERY_SOFTVERSION_t);
	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
#endif
}

static void app_gw3762_afn03_f131_query_code_sha1(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
#if defined(CMP_VERSION)
	app_printf("AppGw3762Afn03F131!\n");
	U16 Gw3762SendDataLen = 0;
	APPGW3762DATA_t Gw3762updata;
	QUERY_CODE_SHA1_t QCodeSHA1;
	uint8_t Message_Digest[SHA1HashSize];
	
	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode	= pGw3762Data_t->CtrlField.TransMode;
	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,pGw3762Data_t->DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum		= 0;
	Gw3762updata.Afn				= APP_GW3762_AFN03;
	Gw3762updata.Fn 				= app_gw3762_fn_bin_to_bs(APP_GW3762_F131);
	Gw3762updata.DataUnitLen		= 0;

    __memcpy((U8 *)&QCodeSHA1,(U8 *)&pGw3762Data_t->DataUnit,sizeof(QUERY_CODE_SHA1_t));
	query_code_sha1(&QCodeSHA1,&SHA1ContextTemp,Message_Digest);
	
    __memcpy(&Gw3762updata.DataUnit,(U8 *)&Message_Digest,sizeof(Message_Digest));
	Gw3762updata.DataUnitLen		= sizeof(Message_Digest);
	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
#endif
}

static void app_bc_online_flag_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	cco_node_sgs_req(&staIdEntifyFlg);
}
#ifdef SHAAN_XI
void send_broadcast_live_line_zero_line_enabled(void)
{
    LiveLineZeroLineEnabled_t *pEventReportRequest_t = NULL;
    pEventReportRequest_t = (LiveLineZeroLineEnabled_t *)zc_malloc_mem(sizeof(LiveLineZeroLineEnabled_t) + 1, "Send_Broadcast_LiveLineZeroLineEnabled", MEM_TIME_OUT);

    pEventReportRequest_t->Direction = e_DOWNLINK;//下行
    pEventReportRequest_t->PrmFlag = 1;//启动站

    pEventReportRequest_t->PacketSeq = ++ApsEventSendPacketSeq;//报文序号
    pEventReportRequest_t->sendType = e_FULL_BROADCAST_FREAM_NOACK;//发送类型广播
    pEventReportRequest_t->CommandType = 2;
    pEventReportRequest_t->EvenDataLen = 1;
    pEventReportRequest_t->EventData[0] = CcoCurveGatherInfo.LiveLineZeroLineEnabled;
    pEventReportRequest_t->PacketSeq = ++ApsSendRegMeterSeq;
    memset(pEventReportRequest_t->DstMacAddr, 0xFF, 6);
    __memcpy(pEventReportRequest_t->SrcMacAddr, GetNnibMacAddr(), 6);

	aps_zero_fire_line_enabled_request(pEventReportRequest_t);

    zc_free_mem(pEventReportRequest_t);
}

static void send_broadcast_live_line_zero_line_enabled_cb(struct ostimer_s *ostimer, void *arg)
{
    send_broadcast_live_line_zero_line_enabled();
}

void app_send_live_zero_enable_wait_timer(void)//Send_LiveZeroEnabledWaittimer
{
    if(Send_LiveLineZeroLinetimer == NULL)
    {
        Send_LiveLineZeroLinetimer = timer_create(10 * 60 * TIMER_UNITS,
            0,
            TMR_TRIGGER,//TMR_TRIGGER
            send_broadcast_live_line_zero_line_enabled_cb,
            NULL,
            "send_broadcast_live_line_zero_line_enabled_cb"
        );
    }
	if(NULL != Send_LiveLineZeroLinetimer && TMR_STOPPED == timer_query(Send_LiveLineZeroLinetimer))
	{
		timer_start(Send_LiveLineZeroLinetimer);
	}
}

static void app_gw3762_afn05_f104_set_live_line_zero_line_enabled(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 newLiveLineZeroLineEnabled;
    //获取下行使能标志
    newLiveLineZeroLineEnabled = pGw3762Data_t->DataUnit[0];
    if(newLiveLineZeroLineEnabled > 1)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }
    //如果设置目标与当前不同
    if(newLiveLineZeroLineEnabled != CcoCurveGatherInfo.LiveLineZeroLineEnabled)
    {
        //对使能标志进行保存
        CcoCurveGatherInfo.LiveLineZeroLineEnabled = newLiveLineZeroLineEnabled;
        cco_save_curve_cfg();
    }
    //进行广播发送
    send_broadcast_live_line_zero_line_enabled();
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}

static void app_gw3762_afn10_f106_get_live_line_zero_line_enabled(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode = pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum = 0;
    Gw3762updata.Afn = APP_GW3762_AFN10;
    Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F106);
    Gw3762updata.DataUnitLen = 1;
    Gw3762updata.DataUnit[0] = CcoCurveGatherInfo.LiveLineZeroLineEnabled;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
}
#endif
void app_gw3762_afn06_f10_report_node_state_change(U8 *data, U16 len, MESG_PORT_e port)//AppGw3762Afn06F10ReportNodeStateChange
{
    app_printf("AppGw3762Afn06F10ReportNodeStateChange\n");
    dump_buf(data,len);
    APPGW3762DATA_t *Gw3762updata = &AppGw3762UpData;
    U16 Gw3762SendDataLen = 0;

    Gw3762updata->CtrlField.TransDir  = APP_GW3762_UP_DIR;
    Gw3762updata->CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata->CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_master_pack(&(Gw3762updata->UpInfoField),NULL);

    Gw3762updata->AddrFieldNum = 0;

    Gw3762updata->Afn                = APP_GW3762_AFN06;
    Gw3762updata->Fn                 = app_gw3762_fn_bin_to_bs(APP_GW3762_F10);

    Gw3762updata->DataUnitLen = len;
    __memcpy(Gw3762updata->DataUnit, data, len);

    app_gw3762_up_frame_encode(Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
}

static U8 get_post_by_phase(U8  phase1, U8  phase2, U8  phase3, U8 *post1, U8 *post2, U8 *post3)
{
    U8 PhaseToPost[6][6] = {
                            { 0 ,  0 ,  0  , 1 ,  2 ,  3 ,  } ,//ABC
                            { 1 ,  0 ,  0  , 1 ,  3 ,  2 ,  } ,//ACB
                            { 0 ,  1 ,  0  , 2 ,  1 ,  3 ,  } ,//BAC
                            { 1 ,  1 ,  0  , 2 ,  3 ,  1 ,  } ,//BCA
                            { 0 ,  0 ,  1  , 3 ,  1 ,  2 ,  } ,//CAB
                            { 1 ,  0 ,  1  , 3 ,  2 ,  1 ,  } ,//CBA

    };

    U8 i;
    for(i = 0; i < 6; i++)
    {
        if(PhaseToPost[i][0] == phase1 && PhaseToPost[i][1] == phase2 && PhaseToPost[i][2] == phase3)
        {

            *post1 = PhaseToPost[i][3];
            *post2 = PhaseToPost[i][4];
            *post3 = PhaseToPost[i][5];
            app_printf("post1=%d,post2=%d,post3=%d\n", post1, post2, post3);
            return TRUE;
        }
    }

    return FALSE;
}

static void app_gw3762_afn10_f102_read_sta_phase(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Afn10F102ShanxixianQueryPhase!\n");
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    APPGW3762AFN10F102_t  QPDATA;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8  ucReadNum = 0;
    U16	bylen = 0;
    U8  indexNum = 0;	//本次上报个数下标
    U16  ReadCount = 0;  //实际有效数量,超过255
    U16  QueryIndex = 0; //开始序号
    U8  default_addr[6] = { 0xff,0xff,0xff,0xff,0xff,0xff };
    U8  device_addr[6] = { 0 };
    U16 usStartIdx = 0;
    U16  jj = 0;
    U8  pnum = 0;		//本次上报个数
    U8  post1 = 0, post2 = 0, post3 = 0;

    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
    app_printf("usStartIdx = %d\n", usStartIdx);

    if(ucReadNum <= 0 || usStartIdx < 1)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
    if(ucReadNum > 30)
    {
		ucReadNum = 30;
    }

    app_printf("usStartIdx = %d\n", usStartIdx);

    Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode = pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum = 0;
    Gw3762updata.Afn = APP_GW3762_AFN10;
    Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F102);
    pGW3762updataunit[bylen++] = (U8)((APP_GETDEVNUM() + 1) & 0xFF);
    pGW3762updataunit[bylen++] = (U8)((APP_GETDEVNUM() + 1) >> 8);
    indexNum = bylen & 0x00ff;
    pGW3762updataunit[bylen++] = 0;//ucReadNum;

    QueryIndex = (usStartIdx > 1 ? usStartIdx - 1 : 1);

    //起始序号为1，读集中器地址
    if(usStartIdx == 1)
    {
        __memcpy(&pGW3762updataunit[bylen], FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN); //6字节地址+两字节信息
        bylen += MAC_ADDR_LEN;
        memset(&QPDATA, 0x00, 2);
        QPDATA.Record = 0;//1;
        QPDATA.Phase1 = 1;
        QPDATA.Phase2 = 1;
        QPDATA.Phase3 = 1;
        QPDATA.LNerr = 0;
        QPDATA.post1 = 1;
        QPDATA.post2 = 2;
        QPDATA.post3 = 3;
        QPDATA.MeterType = 1;

        __memcpy(&pGW3762updataunit[bylen], &QPDATA, sizeof(APPGW3762AFN10F102_t));

        bylen += sizeof(APPGW3762AFN10F102_t);
        //Gw3762updata.DataUnitLen  += 8;
        pnum++;
        if(pnum == ucReadNum || bylen >= APP_GW3762_DATA_UNIT_LEN - 8)
        {
            goto send;
        }
    }

    for(jj = 0; jj < MAX_WHITELIST_NUM; jj++)
    {
        DEVICE_TEI_LIST_t GetDeviceListTemp_t;
        Get_DeviceList_All(jj, &GetDeviceListTemp_t);
        if(0 == memcmp(GetDeviceListTemp_t.MacAddr, default_addr, MAC_ADDR_LEN))//无效地址
        {
            continue;
        }
        ReadCount++;
        if(ReadCount >= QueryIndex)
        {
            __memcpy(device_addr, GetDeviceListTemp_t.MacAddr, MAC_ADDR_LEN);
            ChangeMacAddrOrder(device_addr);
            __memcpy(&pGW3762updataunit[bylen], device_addr, MAC_ADDR_LEN); //6字节地址+两字节信息
            bylen += MAC_ADDR_LEN;
            memset(&QPDATA, 0x00, sizeof(APPGW3762AFN10F102_t));
            QPDATA.MeterType = GetDeviceListTemp_t.DeviceType == e_3PMETER ? 1 : 0;
            if(check_whitelist_by_addr(device_addr))
            {
                QPDATA.Record = 1;
            }

            if(QPDATA.MeterType == 1)
            {
                QPDATA.Phase1 = 1;
                QPDATA.Phase2 = 1;
                QPDATA.Phase3 = 1;

                QPDATA.LNerr = 0;

                if(TRUE == get_post_by_phase(GetDeviceListTemp_t.F31_D5, GetDeviceListTemp_t.F31_D6, GetDeviceListTemp_t.F31_D7, &post1, &post2, &post3))
                {
                    QPDATA.post1 = (post1 & 0x03) * GetDeviceListTemp_t.F31_D0;
                    QPDATA.post2 = (post2 & 0x03) * GetDeviceListTemp_t.F31_D1;
                    QPDATA.post3 = (post3 & 0x03) * GetDeviceListTemp_t.F31_D2;
                    if(QPDATA.post1 == 1 && QPDATA.post2 == 0 && QPDATA.post3 == 0)
                    {
                        QPDATA.post1 = GetDeviceListTemp_t.Phase;
                    }
                    else if(QPDATA.post1 == 0 && QPDATA.post2 == 1 && QPDATA.post3 == 0)
                    {
                        QPDATA.post2 = GetDeviceListTemp_t.Phase;
                    }
                    else if(QPDATA.post1 == 0 && QPDATA.post2 == 0 && QPDATA.post3 == 1)
                    {
                        QPDATA.post3 = GetDeviceListTemp_t.Phase;
                    }
                }
            }
            else
            {
                if(GetDeviceListTemp_t.Phase == e_A_PHASE)
                {
                    QPDATA.Phase1 = TRUE;
                }
                else if(GetDeviceListTemp_t.Phase == e_B_PHASE)
                {
                    QPDATA.Phase2 = TRUE;
                }
                else if(GetDeviceListTemp_t.Phase == e_C_PHASE)
                {
                    QPDATA.Phase3 = TRUE;
                }
                QPDATA.LNerr = GetDeviceListTemp_t.LNerr;
                QPDATA.post1 = 0;
                QPDATA.post2 = 0;
                QPDATA.post3 = 0;
            }

            __memcpy(&pGW3762updataunit[bylen], &QPDATA, sizeof(APPGW3762AFN10F102_t));
            bylen += sizeof(APPGW3762AFN10F102_t);

            pnum++;
            if(pnum == ucReadNum || bylen >= APP_GW3762_DATA_UNIT_LEN - 8)
            {

                goto send;
            }
        }
    }

send:
    Gw3762updata.DataUnit[indexNum] = pnum;
    Gw3762updata.DataUnitLen = bylen;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
}

static void app_gw3762_afn03_f102_get_master_curve_gather(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Afn03F102GetMasterCurveGather\n");
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    //U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8 QueryPrtclType =0 ;
    U8 getFlag = 0;

    
    QueryPrtclType          = pGw3762Data_t->DataUnit[0];
    app_printf("QueryPrtclType = %d\n",QueryPrtclType);
    if((QueryPrtclType != DLT645_2007)&&(QueryPrtclType != DLT698))
    {
        
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //
        return;
    }
    Gw3762updata.CtrlField.TransDir     = APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag    = APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode    = pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,pGw3762Data_t->DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum       = 0;
    Gw3762updata.Afn                = APP_GW3762_AFN03;
    Gw3762updata.Fn             = app_gw3762_fn_bin_to_bs(APP_GW3762_F102);

    //确认上报类型
    //pGW3762updataunit[0] = QueryPrtclType ;
	getFlag = UART_Get_Sta_Curve_Profile_Cfg_By_Protocol(QueryPrtclType, &Gw3762updata.DataUnit[0], &Gw3762SendDataLen);

	if(getFlag == FALSE)
    {   
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
        
	Gw3762updata.DataUnitLen	= Gw3762SendDataLen;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afn05_f102_curve_req_clock(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Afn05F102CurveReqClock\n");
  
    if((pGw3762Data_t->DataUnit[0]!=0)&&(pGw3762Data_t->DataUnit[1] == MINUTES||pGw3762Data_t->DataUnit[1] == HOURS))
    {   
        app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
        cco_set_auto_curve_req(pGw3762Data_t->DataUnit[0],pGw3762Data_t->DataUnit[1]);
    }
    else if(pGw3762Data_t->DataUnit[0]==0)
    {
    	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
    }
	else
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);      
        return;
    }  
}

void clear_sta_curve_status(void)//clearStaCurveStatus
{
	U16 ii;
    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
    	if(DeviceTEIList[ii].NodeTEI != 0xFFF)
    		DeviceTEIList[ii].CurveNeed = e_NeedGet; //获取曲线冻结数据  
    }
}

static void app_gw3762_afn05_f103_set_slave_curve_gather(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("AppGw3762Afn05F103SetSlaveCurveGather\n");
	U8 byLen = 0;
	U8 QueryPrtclType =0 ;
	U8	recoverLen = 0; 		
	U8	saveIndex = 0;
	U8	countIndex = 0;
	U8	recoverLenIndex = 0;
	U8	saveCount = 0;
	U8	itemCount = 0;
	U8	i;
	
	CcoCurveGatherProfile.UseFlag		   = pGw3762Data_t->DataUnit[byLen++];
	CcoCurveGatherProfile.ReadmeterCycle   = pGw3762Data_t->DataUnit[byLen++];

	if(CcoCurveGatherProfile.UseFlag == FALSE)
	{
		CcoCurveGatherProfile.Prtcl645 = 0;
		CcoCurveGatherProfile.Item645SubLen = 0;
		memset(CcoCurveGatherProfile.Item645Sub,0x00,sizeof(CcoCurveGatherProfile.Item645Sub));
		CcoCurveGatherProfile.Prtcl698 = 0;
		CcoCurveGatherProfile.Item698SubLen = 0;
		memset(CcoCurveGatherProfile.Item698Sub,0x00,sizeof(CcoCurveGatherProfile.Item698Sub));

		CCOCurveCfgFlag = TRUE;

		//每次变更任务后清空模块配置状态
		clear_sta_curve_status();

		//dump_buf((U8*)&CcoCurveGatherProfile, sizeof(CcoCurveGatherProfile));
		CurveGatherState = e_CURVE_CFG_SET_ALL;
		SetSlaveCurveGatherTimerModify(2*1000,SetSlaveCurveGather);

		app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
		return;
	}
	
	QueryPrtclType = pGw3762Data_t->DataUnit[byLen++];
	if(QueryPrtclType == DLT645_2007)
	{
		CcoCurveGatherProfile.Prtcl645 = DLT645_2007;
		//单相表
		__memcpy(&CcoCurveGatherProfile.Item645Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],3);
		itemCount = pGw3762Data_t->DataUnit[byLen+1];
		countIndex = saveIndex + 1;
		recoverLenIndex = saveIndex + 2;
		saveIndex += 3;
		byLen += 3;
		if(itemCount > MAX_DATA_COUNT)
			saveCount = MAX_DATA_COUNT;
		else
			saveCount = itemCount;

		for(i = 0; i < saveCount; ++i)
		{
			__memcpy(&CcoCurveGatherProfile.Item645Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],5);
			recoverLen += pGw3762Data_t->DataUnit[byLen+4];
			saveIndex += 5;
			byLen += 5;
		}
		CcoCurveGatherProfile.Item645Sub[countIndex] = saveCount;
		CcoCurveGatherProfile.Item645Sub[recoverLenIndex] = recoverLen;

		//三相表
		recoverLen = 0;
		byLen = 6+(itemCount*5);		//找到下发帧的三相表部分
		__memcpy(&CcoCurveGatherProfile.Item645Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],3);
		itemCount = pGw3762Data_t->DataUnit[byLen+1];
		countIndex = saveIndex + 1;
		recoverLenIndex = saveIndex + 2;
		saveIndex += 3;
		byLen += 3;
		
		if(itemCount > MAX_DATA_COUNT)
			saveCount = MAX_DATA_COUNT;
		else
			saveCount = itemCount;

		for(i = 0; i < saveCount; ++i)
		{
			__memcpy(&CcoCurveGatherProfile.Item645Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],5);
			recoverLen += pGw3762Data_t->DataUnit[byLen+4];
			saveIndex += 5;
			byLen += 5;
		}
		CcoCurveGatherProfile.Item645Sub[countIndex] = saveCount;
		CcoCurveGatherProfile.Item645Sub[recoverLenIndex] = recoverLen;

		CcoCurveGatherProfile.Item645SubLen = saveIndex;
	}
	else if(QueryPrtclType == DLT698)
	{
		CcoCurveGatherProfile.Prtcl698 = DLT698;
		//单相表
		__memcpy(&CcoCurveGatherProfile.Item698Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],3);
		itemCount = pGw3762Data_t->DataUnit[byLen+1];
		countIndex = saveIndex + 1;
		recoverLenIndex = saveIndex + 2;
		saveIndex += 3;
		byLen += 3;
		if(itemCount > MAX_DATA_COUNT)
			saveCount = MAX_DATA_COUNT;
		else
			saveCount = itemCount;

		for(i = 0; i < saveCount; ++i)
		{
			__memcpy(&CcoCurveGatherProfile.Item698Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],5);
			recoverLen += pGw3762Data_t->DataUnit[byLen+4];
			saveIndex += 5;
			byLen += 5;
		}
		CcoCurveGatherProfile.Item698Sub[countIndex] = saveCount;
		CcoCurveGatherProfile.Item698Sub[recoverLenIndex] = recoverLen;

		//三相表
		recoverLen = 0;
		byLen = 6+(itemCount*5);		//找到下发帧的三相表部分
		__memcpy(&CcoCurveGatherProfile.Item698Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],3);
		itemCount = pGw3762Data_t->DataUnit[byLen+1];
		countIndex = saveIndex + 1;
		recoverLenIndex = saveIndex + 2;
		saveIndex += 3;
		byLen += 3;
		
		if(itemCount > MAX_DATA_COUNT)
			saveCount = MAX_DATA_COUNT;
		else
			saveCount = itemCount;

		for(i = 0; i < saveCount; ++i)
		{
			__memcpy(&CcoCurveGatherProfile.Item698Sub[saveIndex],&pGw3762Data_t->DataUnit[byLen],5);
			recoverLen += pGw3762Data_t->DataUnit[byLen+4];
			saveIndex += 5;
			byLen += 5;
		}
		CcoCurveGatherProfile.Item698Sub[countIndex] = saveCount;
		CcoCurveGatherProfile.Item698Sub[recoverLenIndex] = recoverLen;

		CcoCurveGatherProfile.Item698SubLen = saveIndex;
	}
	else
	{
		
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
		return;
	}
	
	CCOCurveCfgFlag = TRUE;

	//每次变更任务后清空模块配置状态
	clear_sta_curve_status();

	//dump_buf((U8*)&CcoCurveGatherProfile, sizeof(CcoCurveGatherProfile));
	CurveGatherState = e_CURVE_CFG_SET_ALL;
	SetSlaveCurveGatherTimerModify(2*1000,SetSlaveCurveGather);
	app_printf("CcoCurveGatherProfile.UseFlag = %d \n",CcoCurveGatherProfile.UseFlag);
	if(CcoCurveGatherProfile.UseFlag == FALSE)
	{

	}
	else
	{
		U32 retime = 0;
			
		retime = BroadcastCycleCfg.BroadcastCycle*60*(BroadcastCycleCfg.CycleUnit==1?1:60);
		cco_modify_curve_req_clock_timer(3,retime,TMR_PERIODIC);
	}
	
	
	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
	   
}

void app_gw3762_afn10_f103_get_slave_curve_gather(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f103_get_slave_curve_gather\n");

	U16 Gw3762SendDataLen = 0;
	APPGW3762DATA_t Gw3762updata;
	U8 *pGW3762updataunit = Gw3762updata.DataUnit;
	U8	ucReadNum = 0;
	U8	default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8	device_addr[6]={0};
	U16 usStartIdx = 0;
	U16  ii = 0;
	U16  jj = 0;
	U8	num=0;
	U16  byLen=0;
	
	usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
	usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
	ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port); //无信息可读
		return;
	}
	if(ucReadNum>30)
	{
		ucReadNum = 30;
	}
	if(((APP_GETDEVNUM() + 1) < usStartIdx) || usStartIdx <= 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port); //无信息可读
		return;
	}
	
	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode	= pGw3762Data_t->CtrlField.TransMode;
	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,pGw3762Data_t->DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum		= 0;
	Gw3762updata.Afn				= APP_GW3762_AFN10;
	Gw3762updata.Fn 			= app_gw3762_fn_bin_to_bs(APP_GW3762_F103);
	Gw3762updata.DataUnitLen			= 2 + 2 +1;
	pGW3762updataunit[byLen++]		 = (U8)(APP_GETDEVNUM() & 0xFF);
	pGW3762updataunit[byLen++]			= (U8)(APP_GETDEVNUM() >> 8);
	pGW3762updataunit[byLen++]			  = 0;//ucReadNum;

	ii = 1;
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		DEVICE_TEI_LIST_t DeviceListTemp;
		Get_DeviceList_All(jj ,  &DeviceListTemp);
		if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		ii++;
		if(ii >= usStartIdx)//当前为读取的起始位置
		{
			__memcpy(device_addr,DeviceListTemp.MacAddr,6);
			ChangeMacAddrOrder(device_addr);
			__memcpy(&pGW3762updataunit[byLen], device_addr, 6); //6字节地址+两字节信息
			byLen+= 6;
			
			pGW3762updataunit[byLen++] = GetSTACurveGatherCfgbySeq(jj);
			
			num += 1;
			
			if(num == ucReadNum || byLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				goto send;
			}
		}	
	}
	app_printf("ii = %d,jj = %d, usStartIdx = %d \n",ii,jj,usStartIdx);
send:	
	pGW3762updataunit[2] = num;
	Gw3762updata.DataUnitLen	= byLen;
	if(num == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读 
		return; 
	}
	app_printf("report num : %d\n",num);
	
	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afnf1_f100_concurrent_curve_gather(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U16  byLen = 0;
    U8  findMeterFlag = FALSE;//不在白名单
    U8  BroadcastFlag = Generally;//广播抄读标志
    U8 readMeterNum;//一帧携带645帧数量
    U16   ii=0;
    U8   nullAddr[6] = {0,0,0,0,0,0};
	
    //app_printf("AppGw3762AfnF1F1Concurrent!\n");

    if(AppGw3762Afn10F4State.WorkSwitchStates ==  CARRIER_UPGRADE_FLAG)
    {
    	//升级过程增加支持点抄，此时拉长流程控制时间
        upgrade_cco_modify_upgrade_suspend_timer(20);
    }
    //停止路由抄表流程，需要12F1开启
    router_read_stop_timer();

    if(pGw3762Data_t->DataUnitLen < 4)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }


    /* 解析AFNF1F1数据单元到Gw3762AfnF1F1Param */
    AppGw3762Afn13F1State.ProtoType		= gw3762downdataunit[byLen++]; // ProtoType

    byLen++;   // Reserve
    
    
    AppGw3762Afn13F1State.FrameLen = (U16)gw3762downdataunit[byLen] + ((U16)gw3762downdataunit[byLen+1] << 8);
    byLen += 2;

    if(AppGw3762Afn13F1State.FrameLen != 0)
    {
    
        __memcpy(AppGw3762Afn13F1State.FrameUnit, &gw3762downdataunit[byLen], AppGw3762Afn13F1State.FrameLen);
    }
    //dump_buf(&gw3762downdataunit[byLen],AppGw3762Afn13F1State.FrameLen);
    if(pGw3762Data_t->DownInfoField.ModuleFlag == 1)
	{
        __memcpy(AppGw3762Afn13F1State.Addr,pGw3762Data_t->AddrField.DestAddr,6);
	}
	else
	{
	    if(TRUE==Check645Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL,AppGw3762Afn13F1State.Addr,NULL))
	    {
	        app_printf("645 frame \n");
	    }
	    else  if(TRUE==Check698Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL, AppGw3762Afn13F1State.Addr,NULL))
	    {
			app_printf("698 frame\n");
		}
		else if(TRUE == CheckT188Frame(AppGw3762Afn13F1State.FrameUnit, AppGw3762Afn13F1State.FrameLen, AppGw3762Afn13F1State.Addr))
		{
			app_printf("T188 frame:");
			dump_buf(AppGw3762Afn13F1State.FrameUnit, AppGw3762Afn13F1State.FrameLen);
		}
		else
		{
			app_printf("other frame \n");
		}
	}
    U8 *payload=AppGw3762Afn13F1State.FrameUnit;
    U16 len=0;  
    U16 offset=0;
    readMeterNum = 0;
    U16 remainlen = AppGw3762Afn13F1State.FrameLen;

switch (AppGw3762Afn13F1State.ProtoType)
	{
		case APP_T_PROTOTYPE:
	            app_printf("APP_T_PROTOTYPE \n");
				readMeterNum = 1;
			break;
		case APP_97_PROTOTYPE:
		case APP_07_PROTOTYPE:
			do
		    {
		        if(e_Decode_Success ==ScaleDLT(e_DLT645_MSG,payload,&offset, &remainlen,&len))
		        {
		            readMeterNum++;
		            payload += len;
		        }
		        else 
				{
					break;
				}
		    }while(0!=remainlen);
			break;
		case APP_698_PROTOTYPE:
			if(TRUE==Check698Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen, NULL,NULL,NULL))
	        {
				
	            app_printf("APP_698_PROTOTYPE !\n");
				readMeterNum = 1;
	        }
			break;
		case APP_SINGLEMETER_PROTOTYPE:		/* 单向主动上报水表*/
    	case APP_DOUBLEMETER_PROTOTYPE:		/* 双向主动上报水表*/
    	case APP_GASMETER_PROTOTYPE:			/* 燃气表*/
    	case APP_HEATMEYET_PROTOTYPE:		/* 热表*/
			if(TRUE==CheckT188Frame(AppGw3762Afn13F1State.FrameUnit, AppGw3762Afn13F1State.FrameLen, AppGw3762Afn13F1State.Addr))
	        {
			
	            app_printf("188 frame \n");
				readMeterNum = 1;
	            break;
	        }
		default :
			break;
	}
	
    app_printf("readMeterNum is %d \n", readMeterNum);

    if(readMeterNum>METER_NUM_SIZE || AppGw3762Afn13F1State.FrameLen > 2000)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_OUT_MAX3762FRAMENUM_ERRCODE, port);//110超出376.2帧中最大电表协议报文条数
        app_printf("376.2   APP_GW3762_OUT_MAX3762FRAMENUM_ERRCODE \n");
        return;
    }
	
	for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        if((memcmp(WhiteMacAddrList[ii].MacAddr, AppGw3762Afn13F1State.Addr, MAC_ADDR_LEN) == 0))
        {
            __memcpy(AppGw3762Afn13F1State.CnmAddr, WhiteMacAddrList[ii].CnmAddr, MAC_ADDR_LEN);
            findMeterFlag = TRUE;
            break;
        }
    }
	
    if(findMeterFlag == FALSE)
    {
        app_printf("APP_GW3762_ADDRNONE_ERRCODE  ----->\n");
        dump_buf(AppGw3762Afn13F1State.Addr,6);
        app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
        return;
    }
	
    if(memcmp(AppGw3762Afn13F1State.CnmAddr,nullAddr,6)==0)
    {
    	//BroadcastFlag = Generally;
        app_printf("Generally\n");
		app_gw3762_up_afn00_f2_deny(APP_GW3762_METER_READING_ERRCODE, port);
		return;
    }
    
    app_printf("F1F1.Addr : ");
    dump_buf(AppGw3762Afn13F1State.Addr, 6);
	app_printf("F1F1.CnmAddr : ");
    dump_buf(AppGw3762Afn13F1State.CnmAddr, 6);
	
	CnttActCncrntF1F100ReadMeterReq(pGw3762Data_t->DownInfoField.FrameNum, readMeterNum, AppGw3762Afn13F1State.Addr, 
                                AppGw3762Afn13F1State.CnmAddr, ii ,AppGw3762Afn13F1State.ProtoType, AppGw3762Afn13F1State.FrameLen, 
                                 AppGw3762Afn13F1State.FrameUnit,BroadcastFlag, port);	
	return;
}

void app_gw3762_up_afnf1_f100_up_frame(U8 *Addr, AppProtoType_e proto, U8 *data, U16 len, MESG_PORT_e port,U8 localseq)//AppGw3762UpAfnF1F100UpFrame
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16  byLen = 0;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode    = APP_GW3762_TRANS_MODE;

    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;
    Gw3762updata.Afn				= APP_GW3762_AFNF1;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F100);

    __memcpy(Gw3762updata.AddrField.SrcAddr, Addr, MAC_ADDR_LEN);
    __memcpy(Gw3762updata.AddrField.DestAddr, FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN);

    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 1, Gw3762updata.AddrField.SrcAddr,localseq);

    Gw3762updata.DataUnit[byLen++] = proto;
    Gw3762updata.DataUnit[byLen++] = (U8)(len & 0xFF);
    Gw3762updata.DataUnit[byLen++] = (U8)(len >> 8);

    if(len > 0)
    {
        __memcpy(&Gw3762updata.DataUnit[byLen], data, len);
        byLen += len;
    }
    
    Gw3762updata.DataUnitLen		= byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,localseq, FALSE, 0, port,e_Serial_AFNF1F1);
}

static void app_gw3762_afnf0_f100_query_net_base_msg(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)//AppGw3762AfnF0F100QueryNetBaseMsg
{
	U16 ii; U8 MaxDepth = 0, jj;
	U16 Gw3762SendDataLen = 0;
	APPGW3762DATA_t Gw3762updata;
	AFNF0F100_DATA_UNIT_t  F0F100_data_unit_t;

	memset(&F0F100_data_unit_t.Levelinfo_t[0].Enterlevel, 0x00, sizeof(LEVEL_INFO_t) * MAX_LEVEL);

	Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum = 0;
	Gw3762updata.UpInfoField.ModuleFlag = 0;
	Gw3762updata.Afn = APP_GW3762_AFNF0;
	Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F100);
	F0F100_data_unit_t.Netnodetotalnum = FlashInfo_t.usJZQRecordNum;
	F0F100_data_unit_t.Onlinenodenum = nnib.discoverSTACount + nnib.PCOCount;
	F0F100_data_unit_t.Startnettime = TEST_DELAY + 1;

	if(FALSE == nnib.Networkflag)
	{
		F0F100_data_unit_t.Networktime = 0xEEEE;
	}	
	else
	{
		F0F100_data_unit_t.Networktime = SystenRunTime - TEST_DELAY - 1;
	}		

	F0F100_data_unit_t.Beaconperiod = nnib.BeaconPeriodLength / 1000;
	F0F100_data_unit_t.Routerperiod = nnib.RoutePeriodTime;
	F0F100_data_unit_t.Topchangetimes = Topchangetimes;

	for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
	{
		if(DeviceTEIList[ii].NodeTEI != 0x0fff)
		{
			MaxDepth = DeviceTEIList[ii].NodeDepth > MaxDepth ? DeviceTEIList[ii].NodeDepth : MaxDepth;
			if(MaxDepth == 0x0f)break;
		}
	}

	F0F100_data_unit_t.TotalLevel = MaxDepth;

	for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
	{
		if(DeviceTEIList[ii].NodeTEI != 0xffff)
		{
			for(jj = 1; jj <= MaxDepth; jj++)
			{
				if(jj == DeviceTEIList[ii].NodeDepth)
				{
					F0F100_data_unit_t.Levelinfo_t[jj - 1].Enterlevel = jj;
					F0F100_data_unit_t.Levelinfo_t[jj - 1].Levelnodenum++;
					break;
				}
			}
		}
	}

	Gw3762updata.DataUnitLen = 16 + MaxDepth * 3;
	__memcpy(Gw3762updata.DataUnit, &F0F100_data_unit_t.Netnodetotalnum, Gw3762updata.DataUnitLen);

	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
}

static void app_gw3762_afnf0_f102_query_net_node_msg(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)//AppGw3762AfnF0F102QueryNetNodeMsg
{
	U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
	U16 jj;
	U8	reportcnt = 0;
	U16 startIndex;
	U8	nodeNum, offset;
	U8	default_addr[6] = { 0xff,0xff,0xff,0xff,0xff,0xff };
	U8	nodeaddr[6] = { 0 };
	U16 Gw3762SendDataLen = 0, sendLen = 0;
	APPGW3762DATA_t Gw3762updata;
	//APPGW3762AFNF0F102_t AppGw3762AfnF0F102_t;
	NODE_INFO_t node_info_t;
	U16 NetNodeNum, RptNodeNum, StartSeq;

	startIndex = (U16)gw3762downdataunit[0];
	startIndex += ((U16)gw3762downdataunit[1] << 8);
	nodeNum = gw3762downdataunit[2];

	if(startIndex < 1)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
		return;
	}

	if(nodeNum <= 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
		return;
	}

	nodeNum = nodeNum > MAX_NODE_NUM ? MAX_NODE_NUM : nodeNum;

	Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);

	Gw3762updata.AddrFieldNum = 0;
	Gw3762updata.UpInfoField.ModuleFlag = 0;
	Gw3762updata.Afn = APP_GW3762_AFNF0;
	Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F102);

	NetNodeNum = nnib.discoverSTACount + nnib.PCOCount + 1;
	RptNodeNum = nodeNum;
	StartSeq = startIndex;
	sendLen += 6;

	if(startIndex == 1)
	{
		offset = 0;
		__memcpy(nodeaddr, FlashInfo_t.ucJZQMacAddr, 6);
		__memcpy(node_info_t.Nodeaddr, nodeaddr, 6);
		node_info_t.Nodetopinfo_t.NodeTEI = 1;
		node_info_t.Nodetopinfo_t.ProxyTEI = 0;
		node_info_t.Nodetopinfo_t.Nodelevel = 0;
		node_info_t.Nodetopinfo_t.Noderole = 4;

		node_info_t.Netstate = 1;
		node_info_t.Beaconslot = 1;
		node_info_t.Inwhitelist = 1;
		node_info_t.Rcvheart = 1;
		node_info_t.NoHeartRoutePeriod = 0;

		node_info_t.Devicetype = e_F0F102_JZQ;
		node_info_t.Phaseinfo = 0x0e;

		node_info_t.Proxytimes = 0;
		node_info_t.Nodeofflinetimes = 0;
		node_info_t.Nodeofflinetime = 0;
		node_info_t.Nodeofflinetime_max = 10 * nnib.RoutePeriodTime;
		node_info_t.Upcomuntsucesrate = 1;
		node_info_t.Dncomuntsucesrate = 1;
		node_info_t.Masterversion[0] = DefSetInfo.OutMFVersion.ucVersionNum[1];
		node_info_t.Masterversion[1] = DefSetInfo.OutMFVersion.ucVersionNum[0];
		node_info_t.Masterversion[2] = 1;
		node_info_t.Slaveversion[0] = 1;
		node_info_t.Slaveversion[1] = 0;
		node_info_t.Nexthopinfo = 0;
		node_info_t.Channeltype = 0;										// 每一bit代表1跳	0:电力线通信	1:无线通信
		node_info_t.Protocoltype = 4;

		node_info_t.Zoneareastate = e_AREA_INSIDE;
		__memcpy(node_info_t.Zoneareaaddr, FlashInfo_t.ucJZQMacAddr, 6);

		node_info_t.Subnodenum = 0;
		offset += 53;
		offset += (node_info_t.Subnodenum * sizeof(SUBNODE_INFO_t));
		__memcpy(&Gw3762updata.DataUnit[sendLen], node_info_t.Nodeaddr, offset);
		sendLen += offset;
		reportcnt++;
		nodeNum -= 1;
	}
	else
	{
		startIndex -= 1;
	}

	for(jj = startIndex - 1; jj < startIndex - 1 + nodeNum; jj++)
	{
		DEVICE_TEI_LIST_t GetDeviceListTemp_t;
		Get_DeviceList_All(jj, &GetDeviceListTemp_t);
		if(0 == memcmp(GetDeviceListTemp_t.MacAddr, default_addr, 6))//无效地址
		{
			continue;
		}
		else
		{
			memset((U8 *)&node_info_t, 0x00, sizeof(node_info_t));
			offset = 0;
			__memcpy(nodeaddr, GetDeviceListTemp_t.MacAddr, 6);
			ChangeMacAddrOrder(nodeaddr);
			__memcpy(node_info_t.Nodeaddr, nodeaddr, 6);
			node_info_t.Nodetopinfo_t.NodeTEI = GetDeviceListTemp_t.NodeTEI;
			node_info_t.Nodetopinfo_t.ProxyTEI = GetDeviceListTemp_t.ParentTEI;
			node_info_t.Nodetopinfo_t.Nodelevel = GetDeviceListTemp_t.NodeDepth;
			node_info_t.Nodetopinfo_t.Noderole = GetDeviceListTemp_t.NodeType;

			node_info_t.Netstate = GetDeviceListTemp_t.NodeState == e_NODE_ON_LINE ? 1 : 0;
			node_info_t.Beaconslot = 1;
			node_info_t.Inwhitelist = 1;
			node_info_t.Rcvheart = 1;
			node_info_t.NoHeartRoutePeriod = 0;
			/*
			app_printf("GetDeviceListTemp_t.DeviceType : %s\n",GetDeviceListTemp_t.DeviceType == e_CKQ?"e_CKQ":
															GetDeviceListTemp_t.DeviceType == e_JZQ?"e_JZQ":
															GetDeviceListTemp_t.DeviceType == e_METER?"e_METER":
															GetDeviceListTemp_t.DeviceType == e_RELAY?"e_RELAY":
															GetDeviceListTemp_t.DeviceType == e_CJQ_2?"e_CJQ_2":
															GetDeviceListTemp_t.DeviceType == e_CJQ_1?"e_CJQ_1":
															GetDeviceListTemp_t.DeviceType == e_3PMETER?"e_3PMETER":"UN");*/

			node_info_t.Devicetype = GetDeviceListTemp_t.DeviceType == e_CKQ ? e_F0F102_CKQ :
				GetDeviceListTemp_t.DeviceType == e_JZQ ? e_F0F102_JZQ :
				GetDeviceListTemp_t.DeviceType == e_METER ? e_F0F102_METER :
				GetDeviceListTemp_t.DeviceType == e_RELAY ? e_F0F102_RELAY :
				GetDeviceListTemp_t.DeviceType == e_CJQ_2 ? e_F0F102_CJQ2 :
				GetDeviceListTemp_t.DeviceType == e_CJQ_1 ? e_F0F102_CJQ1 :
				GetDeviceListTemp_t.DeviceType == e_3PMETER ? e_F0F102_3PMETER : e_F0F102_485;
			//app_printf("GetDeviceListTemp_t.Phase = %d\n",GetDeviceListTemp_t.Phase);
			node_info_t.Phaseinfo = GetDeviceListTemp_t.Phase == 0 ? 0x01 :
				GetDeviceListTemp_t.Phase == 1 ? 0x02 :
				GetDeviceListTemp_t.Phase == 2 ? 0x04 :
				GetDeviceListTemp_t.Phase == 3 ? 0x08 : 0x0e;

			node_info_t.Proxytimes = 0;
			node_info_t.Nodeofflinetimes = 0;
			node_info_t.Nodeofflinetime = 0;
			node_info_t.Nodeofflinetime_max = 10 * nnib.RoutePeriodTime;
			node_info_t.Upcomuntsucesrate = GetDeviceListTemp_t.UplinkSuccRatio;
			node_info_t.Dncomuntsucesrate = GetDeviceListTemp_t.DownlinkSuccRatio;
			__memcpy(node_info_t.Masterversion, GetDeviceListTemp_t.SoftVerNum, 2);
			node_info_t.Masterversion[2] = 1;
			node_info_t.Slaveversion[0] = 1;
			node_info_t.Slaveversion[1] = 0;
			node_info_t.Nexthopinfo = NwkRoutingTable[GetDeviceListTemp_t.NodeTEI - 1].NextHopTEI;

			node_info_t.Channeltype = 0;// 每一bit代表1跳	0:电力线通信	1:无线通信
			node_info_t.Protocoltype = get_protocol_by_addr(nodeaddr);

			node_info_t.Zoneareastate = GetDeviceListTemp_t.AREA_ERR == 0x03 ?
				0 : GetDeviceListTemp_t.AREA_ERR == FALSE ? 02 : 01;
			//todo:__memcpy(node_info_t.Zoneareaaddr, GetDeviceListTemp_t.AREA_ERR == FALSE ? ReprotCCO[GetDeviceListTemp_t.CCOMACSEQ].BeLongCCO : FlashInfo_t.ucJZQMacAddr, 6);
			node_info_t.Subnodenum = 0;
			offset += 53;
			offset += (node_info_t.Subnodenum * sizeof(SUBNODE_INFO_t));
			__memcpy(&Gw3762updata.DataUnit[sendLen], node_info_t.Nodeaddr, offset);
			sendLen += offset;
			reportcnt++;
		}
	}

	/*
	if (startIndex==1)
	{
		offset = 0;
		__memcpy(nodeaddr,FlashInfo_t.ucJZQMacAddr,6);
		__memcpy(AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeAddr, nodeaddr, 6);
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.nodeTEI = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.proxyTEI = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.nodelevel = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.noderole = 4;

		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NetState = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].BeaconSlot = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Inwhitelist = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Rcvheart = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NoHeartRoutePeriod = 0;

		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].DeviceType = e_F0F102_JZQ;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].phaseInfo = 0x0e;

		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].proxyTimes = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].STAOutlineTimes = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].STAOutlineTimeLen = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MaxSTAOutlineTimeLen = 10 * nnib.RoutePeriodTime;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].UpComuntSucesRate = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].DownComuntSucesRate= 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MainVersion[0] = ZC3951CCO_ver1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MainVersion[1] = ZC3951CCO_ver2;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MainVersion[2] = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].SecondVersion[0] = 1;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].SecondVersion[1] = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NextHopInfo = 0;
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ChannelType = 0;// 每一bit代表1跳  0:电力线通信   1:无线通信
		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ProtocolType = 4;

		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ZoneAreaState = e_AREA_INSIDE;
		__memcpy(AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ZoneAreaAddr, FlashInfo_t.ucJZQMacAddr, 6);

		AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Slave2SlaveNum = 0;
		offset	+= 53;
		//app_printf("Slave2SlaveInfoLen = %d\n",AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Slave2SlaveNum*sizeof(SLAVE2SLAVE_INFO_t));
		offset += (AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Slave2SlaveNum*sizeof(SLAVE2SLAVE_INFO_t));

		__memcpy(&Gw3762updata.DataUnit[6+sendLen],AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeAddr,offset);
		sendLen += offset;
		reportcnt++;
		nodeNum-=1;
	}
	else
	{
		startIndex-=1;
	}

	for(jj=startIndex-1;jj<startIndex-1+nodeNum;jj++)
	{
		DEVICE_TEI_LIST_t GetDeviceListTemp_t;
		Get_DeviceList_All(jj,&GetDeviceListTemp_t);
		if(0==memcmp(GetDeviceListTemp_t.MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		else
		{
			offset=0;
			__memcpy(nodeaddr,GetDeviceListTemp_t.MacAddr,6);
			ChangeMacAddrOrder(nodeaddr);
			__memcpy(AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeAddr, nodeaddr, 6);
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.nodeTEI = GetDeviceListTemp_t.NodeTEI;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.proxyTEI = GetDeviceListTemp_t.ParentTEI;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.nodelevel = GetDeviceListTemp_t.NodeDepth;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeTopoInfo_t.noderole = GetDeviceListTemp_t.NodeType;

			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NetState = GetDeviceListTemp_t.NodeState==e_NODE_ON_LINE?1:0;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].BeaconSlot = 1;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Inwhitelist = 1;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Rcvheart = 1;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NoHeartRoutePeriod = 0;

			app_printf("GetDeviceListTemp_t.DeviceType : %s\n",GetDeviceListTemp_t.DeviceType == e_CKQ?"e_CKQ":
															GetDeviceListTemp_t.DeviceType == e_JZQ?"e_JZQ":
															GetDeviceListTemp_t.DeviceType == e_METER?"e_METER":
															GetDeviceListTemp_t.DeviceType == e_RELAY?"e_RELAY":
															GetDeviceListTemp_t.DeviceType == e_CJQ_2?"e_CJQ_2":
															GetDeviceListTemp_t.DeviceType == e_CJQ_1?"e_CJQ_1":
															GetDeviceListTemp_t.DeviceType == e_3PMETER?"e_3PMETER":"UN"
			);

			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].DeviceType = GetDeviceListTemp_t.DeviceType == e_CKQ?e_F0F102_CKQ:
																	GetDeviceListTemp_t.DeviceType == e_JZQ?e_F0F102_JZQ:
																	GetDeviceListTemp_t.DeviceType == e_METER?e_F0F102_METER:
																	GetDeviceListTemp_t.DeviceType == e_RELAY?e_F0F102_RELAY:
																	GetDeviceListTemp_t.DeviceType == e_CJQ_2?e_F0F102_CJQ2:
																	GetDeviceListTemp_t.DeviceType == e_CJQ_1?e_F0F102_CJQ1:
																	GetDeviceListTemp_t.DeviceType == e_3PMETER?e_F0F102_3PMETER:e_F0F102_485;
			//app_printf("GetDeviceListTemp_t.Phase = %d\n",GetDeviceListTemp_t.Phase);
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].phaseInfo = GetDeviceListTemp_t.Phase == 0?0x01:
															GetDeviceListTemp_t.Phase == 1?0x02:
															GetDeviceListTemp_t.Phase == 2?0x04:
															GetDeviceListTemp_t.Phase == 3?0x08:0x0e;

			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].proxyTimes = 0;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].STAOutlineTimes = 0;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].STAOutlineTimeLen = 0;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MaxSTAOutlineTimeLen = 10*nnib.RoutePeriodTime;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].UpComuntSucesRate = GetDeviceListTemp_t.UplinkSuccRatio;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].DownComuntSucesRate= GetDeviceListTemp_t.DownlinkSuccRatio;
			__memcpy(AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MainVersion, GetDeviceListTemp_t.SoftVerNum, 2);
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].MainVersion[2] = 1;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].SecondVersion[0] = 1;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].SecondVersion[1] = 0;
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NextHopInfo = 0;

			for(ii=0;ii<NWK_MAX_ROUTING_TABLE_ENTRYS;ii++)
			{
				if(GetDeviceListTemp_t.NodeTEI == NwkRoutingTable[ii].DestTEI)
				{
					AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NextHopInfo = NwkRoutingTable[ii].NextHopTEI;
					break;
				}
			}

			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ChannelType = 0;// 每一bit代表1跳  0:电力线通信   1:无线通信
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ProtocolType = get_protocol_by_addr(nodeaddr);

			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ZoneAreaState = GetDeviceListTemp_t.AREA_ERR == 0xff?
				0:GetDeviceListTemp_t.AREA_ERR == FALSE?02:01;
			__memcpy(AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].ZoneAreaAddr,GetDeviceListTemp_t.AREA_ERR == 0xff?
				FlashInfo_t.ucJZQMacAddr:GetDeviceListTemp_t.AREA_ERR == TRUE?FlashInfo_t.ucJZQMacAddr:ReprotCCO[GetDeviceListTemp_t.CCOMACSEQ].BeLongCCO, 6);
			//app_printf("GetDeviceListTemp_t.CCOMACSEQ : %d\n",GetDeviceListTemp_t.CCOMACSEQ);
			AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Slave2SlaveNum = 0;
			offset	+= 53;
			//app_printf("Slave2SlaveInfoLen = %d\n",AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Slave2SlaveNum*sizeof(SLAVE2SLAVE_INFO_t));
			offset += (AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].Slave2SlaveNum*sizeof(SLAVE2SLAVE_INFO_t));

			__memcpy(&Gw3762updata.DataUnit[6+sendLen],AppGw3762AfnF0F102_t.NodeInfo_t[reportcnt].NodeAddr,offset);
			sendLen += offset;
			reportcnt++;
		}
	}
	*/
	RptNodeNum = reportcnt;
	__memcpy(Gw3762updata.DataUnit, (U8 *)&NetNodeNum, 2);
	__memcpy(Gw3762updata.DataUnit + 2, (U8 *)&RptNodeNum, 2);
	__memcpy(Gw3762updata.DataUnit + 4, (U8 *)&StartSeq, 2);
	//sendLen += 6;
	Gw3762updata.DataUnitLen = sendLen;
	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);

}

static void app_gw3762_afnf0_f103_query_app_pointed_node_msg(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)//AppGw3762AfnF0F103QueryAppointedNodeMsg
{
	U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
	U8	QueryNum, rcvLen = 0, index = 0, offset = 0;
	U16 Gw3762SendDataLen = 0;
	U8	Addr[6] = { 0x00,0x00,0x00,0x00,0x00,0x00 };
	U8	nodeaddr[6] = { 0 };

	APPGW3762DATA_t Gw3762updata;
	NODE_INFO_t NodeInfo_t;
	QueryNum = gw3762downdataunit[rcvLen++];

	if(QueryNum <= 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
		return;
	}

	QueryNum = QueryNum > MAX_NODE_NUM ? MAX_NODE_NUM : QueryNum;


	Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);

	Gw3762updata.AddrFieldNum = 0;
	Gw3762updata.UpInfoField.ModuleFlag = 0;
	Gw3762updata.Afn = APP_GW3762_AFNF0;
	Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F103);

	Gw3762updata.DataUnitLen = 2;//偏移本次上报节点数量
	app_printf("gw3762downdataunit[rcvLen]:");
	dump_buf(&gw3762downdataunit[rcvLen], 6);

	for(index = 0; index < QueryNum; index++)
	{
		memset((U8 *)&NodeInfo_t, 0x00, sizeof(NodeInfo_t));
		__memcpy(Addr, &gw3762downdataunit[rcvLen + index * 6], 6);
		if(0 == memcmp(Addr, FlashInfo_t.ucJZQMacAddr, 6))
		{
			app_printf("--------------------query master node info--------------------\n");
			offset = 0;
			__memcpy(NodeInfo_t.Nodeaddr, FlashInfo_t.ucJZQMacAddr, 6);
			NodeInfo_t.Nodetopinfo_t.NodeTEI = 1;
			NodeInfo_t.Nodetopinfo_t.ProxyTEI = 0;
			NodeInfo_t.Nodetopinfo_t.Nodelevel = 0;
			NodeInfo_t.Nodetopinfo_t.Noderole = 0x04;

			NodeInfo_t.Netstate = e_NODE_ON_LINE;
			NodeInfo_t.Beaconslot = 1;
			NodeInfo_t.Inwhitelist = 1;
			NodeInfo_t.Rcvheart = 1;
			NodeInfo_t.NoHeartRoutePeriod = 0;

			NodeInfo_t.Devicetype = 0x02;			// BIT 2
			NodeInfo_t.Phaseinfo = 0x0E;			// A B C  三相
			NodeInfo_t.Proxytimes = Topchangetimes;
			NodeInfo_t.Nodeofflinetimes = 0;
			NodeInfo_t.Nodeofflinetime = 0;
			NodeInfo_t.Nodeofflinetime_max = 10 * nnib.RoutePeriodTime;

			NodeInfo_t.Upcomuntsucesrate = 100;
			NodeInfo_t.Dncomuntsucesrate = 100;
			NodeInfo_t.Masterversion[0] = DefSetInfo.OutMFVersion.ucVersionNum[1];
			NodeInfo_t.Masterversion[1] = DefSetInfo.OutMFVersion.ucVersionNum[0];
			NodeInfo_t.Masterversion[2] = 1;
			NodeInfo_t.Slaveversion[0] = 01;
			NodeInfo_t.Slaveversion[1] = 00;

			NodeInfo_t.Nexthopinfo = 0;
			NodeInfo_t.Channeltype = 0;// 每一bit代表1跳  0:电力线通信   1:无线通信
			NodeInfo_t.Protocoltype = 4;

			NodeInfo_t.Zoneareastate = e_AREA_INSIDE;
			__memcpy(NodeInfo_t.Zoneareaaddr, FlashInfo_t.ucJZQMacAddr, 6);

			NodeInfo_t.Subnodenum = 0;
			offset += 53;
			offset += (NodeInfo_t.Subnodenum * sizeof(SUBNODE_INFO_t));

			__memcpy(&Gw3762updata.DataUnit[Gw3762updata.DataUnitLen], NodeInfo_t.Nodeaddr, offset);
			Gw3762updata.DataUnitLen += offset;
			if(QueryNum == 1)//只查询主节点信息
			{
				index = 1;
				break;
			}
		}
		else
		{
			__memcpy(nodeaddr, Addr, 6);
			DEVICE_TEI_LIST_t GetDeviceListTemp_t;
			if(Get_DeviceList_All_ByAdd(Addr, &GetDeviceListTemp_t))
			{
				offset = 0;
				__memcpy(NodeInfo_t.Nodeaddr, nodeaddr, 6);
				NodeInfo_t.Nodetopinfo_t.NodeTEI = GetDeviceListTemp_t.NodeTEI;
				NodeInfo_t.Nodetopinfo_t.ProxyTEI = GetDeviceListTemp_t.ParentTEI;
				NodeInfo_t.Nodetopinfo_t.Nodelevel = GetDeviceListTemp_t.NodeDepth;
				NodeInfo_t.Nodetopinfo_t.Noderole = GetDeviceListTemp_t.NodeType;

				NodeInfo_t.Netstate = GetDeviceListTemp_t.NodeState == e_NODE_ON_LINE ? 1 : 0;
				NodeInfo_t.Beaconslot = 1;
				NodeInfo_t.Inwhitelist = 1;
				NodeInfo_t.Rcvheart = 1;
				NodeInfo_t.NoHeartRoutePeriod = 0;

				NodeInfo_t.Devicetype = GetDeviceListTemp_t.DeviceType == e_CKQ ? e_F0F102_CKQ :
					GetDeviceListTemp_t.DeviceType == e_JZQ ? e_F0F102_JZQ :
					GetDeviceListTemp_t.DeviceType == e_METER ? e_F0F102_METER :
					GetDeviceListTemp_t.DeviceType == e_RELAY ? e_F0F102_RELAY :
					GetDeviceListTemp_t.DeviceType == e_CJQ_2 ? e_F0F102_CJQ2 :
					GetDeviceListTemp_t.DeviceType == e_CJQ_1 ? e_F0F102_CJQ1 :
					GetDeviceListTemp_t.DeviceType == e_3PMETER ? e_F0F102_3PMETER : e_F0F102_485;

				NodeInfo_t.Phaseinfo = GetDeviceListTemp_t.Phase == 0 ? 0x01 :
					GetDeviceListTemp_t.Phase == 1 ? 0x02 :
					GetDeviceListTemp_t.Phase == 2 ? 0x04 :
					GetDeviceListTemp_t.Phase == 3 ? 0x08 : 0x0e;

				NodeInfo_t.Proxytimes = 0;
				NodeInfo_t.Nodeofflinetimes = 0;
				NodeInfo_t.Nodeofflinetime = 0;
				NodeInfo_t.Nodeofflinetime_max = 10 * nnib.RoutePeriodTime;
				NodeInfo_t.Upcomuntsucesrate = GetDeviceListTemp_t.UplinkSuccRatio;
				NodeInfo_t.Dncomuntsucesrate = GetDeviceListTemp_t.DownlinkSuccRatio;
				__memcpy(NodeInfo_t.Masterversion, GetDeviceListTemp_t.SoftVerNum, 2);

				NodeInfo_t.Masterversion[2] = 1;
				NodeInfo_t.Slaveversion[0] = 1;
				NodeInfo_t.Slaveversion[1] = 0;
				NodeInfo_t.Nexthopinfo = NwkRoutingTable[GetDeviceListTemp_t.NodeTEI - 1].NextHopTEI;
				NodeInfo_t.Channeltype = 0;// 每一bit代表1跳  0:电力线通信   1:无线通信
				NodeInfo_t.Protocoltype = get_protocol_by_addr(nodeaddr);
				NodeInfo_t.Zoneareastate = GetDeviceListTemp_t.AREA_ERR == 0x03 ?
					0 : GetDeviceListTemp_t.AREA_ERR == FALSE ? 02 : 01;
				//todo:__memcpy(NodeInfo_t.Zoneareaaddr, GetDeviceListTemp_t.AREA_ERR == FALSE ? ReprotCCO[GetDeviceListTemp_t.CCOMACSEQ].BeLongCCO : FlashInfo_t.ucJZQMacAddr, 6);
				app_printf("GetDeviceListTemp_t.CCOMACSEQ : %d\n", GetDeviceListTemp_t.CCOMACSEQ);

				NodeInfo_t.Subnodenum = 0;
				offset += 53;
				offset += (NodeInfo_t.Subnodenum * sizeof(SUBNODE_INFO_t));

				__memcpy(&Gw3762updata.DataUnit[Gw3762updata.DataUnitLen], NodeInfo_t.Nodeaddr, offset);
				Gw3762updata.DataUnitLen += offset;
			}
			else
			{
				__memcpy(&Gw3762updata.DataUnit[Gw3762updata.DataUnitLen], nodeaddr, 6);
				memset(&Gw3762updata.DataUnit[Gw3762updata.DataUnitLen + 6], 0x00, 47);
				Gw3762updata.DataUnitLen += 53;
			}
		}
	}
	Gw3762updata.DataUnit[0] = index;
	Gw3762updata.DataUnit[1] = 0;

	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);

}

static void app_gw3762_afn05_f201_proc(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)//AppGw3762Afn05F201SlaveNodeRepSwtich
{
	if((PROVINCE_SHANXI == app_ext_info.province_code) || (PROVINCE_HEBEI == app_ext_info.province_code)) //todo: PROVINCE_SHANXI   PROVINCE_HEBEI
	{
		app_printf("Afn05F201 : pGw3762Data_t->DataUnit=%d\n", *(pGw3762Data_t->DataUnit));
		if(app_ext_info.func_switch.NetSenseSWC != pGw3762Data_t->DataUnit[0])
		{
			app_ext_info.func_switch.UseMode = 1;
			app_ext_info.func_switch.NetSenseSWC = pGw3762Data_t->DataUnit[0];
			DefwriteFg.FunSWC = TRUE;
			DefaultSettingFlag = TRUE;
		}

		//河北
		//if(*(pGw3762Data_t->DataUnit) == 1)//开启离网感知
		//{
		//	NetSenseFlag = TRUE;
		//}
		//else                                //关闭离网感知
		//{
		//	NetSenseFlag = FALSE;
		//}
	}
	else if(PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
	{
		//使能STA入网认证使能接口，0：禁止  1：允许
		app_printf("Afn05F201_hunan : pGw3762Data_t->DataUnit=%d\n", *(pGw3762Data_t->DataUnit));

		if(*(pGw3762Data_t->DataUnit) == 1 || *(pGw3762Data_t->DataUnit) == 0)//STA入网认证使能
		{
			staIdEntifyFlg = *(pGw3762Data_t->DataUnit);

			if(NULL == BCOnLineFlgtimer)
			{
				BCOnLineFlgtimer = timer_create(100,
					24 * 60 * 60 * TIMER_UNITS,
					TMR_PERIODIC,
					app_bc_online_flag_timer_cb,
					NULL,
					"app_bc_online_flag_timer_cb");
			}
			/* 这里的逻辑是收到05_f201后立即发广播，然后将定时器重置为24小时 */
			if(TMR_RUNNING == zc_timer_query(BCOnLineFlgtimer))//之所以先关闭再启动，是为了保证第一次启动时先100ms发送一次后再周期发送
			{
				timer_stop(BCOnLineFlgtimer, TMR_NULL);
			}
				
			timer_start(BCOnLineFlgtimer);
		}
		else
		{
			app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
		}
	}
	else
	{
		app_printf("-AppGw3762Afn05F201 error-----\n");
		return;
	}

	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	return;
}

static void app_gw3762_afn03_f201_proc(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)//AppGw3762Afn03F201SlaveNodeRepSwtichflgEn
{
	APPGW3762DATA_t *Gw3762updata = &AppGw3762UpData;
	U16 Gw3762SendDataLen = 0;

	Gw3762updata->CtrlField.TransDir = APP_GW3762_UP_DIR;
	Gw3762updata->CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata->CtrlField.TransMode = APP_GW3762_TRANS_MODE;

	app_gw3762_up_info_field_slave_pack(&(Gw3762updata->UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
	Gw3762updata->AddrFieldNum = 0;
	Gw3762updata->UpInfoField.ModuleFlag = 0;
	Gw3762updata->Afn = APP_GW3762_AFN03;
	Gw3762updata->Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F201);
	Gw3762updata->DataUnitLen = 1;

	if(PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
	{
		app_printf("-AppGw3762Afn03F201_hunan-----\n");
		Gw3762updata->DataUnit[0] = staIdEntifyFlg;
	}
	else if(PROVINCE_SHANXI == app_ext_info.province_code) //todo: PROVINCE_SHANXI
	{
		app_printf("-AppGw3762Afn03F201_shanxi-----\n");
		Gw3762updata->DataUnit[Gw3762SendDataLen++] = app_ext_info.func_switch.NetSenseSWC;
	}
	else
	{
		app_printf("-AppGw3762Afn03F201 error-----\n");
		return;
	}

	app_gw3762_up_frame_encode(Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
	return;
}


//河南流水线
//链路模式设置
// U8 network_type = e_DOUBLE_HPLC_HRF;
static void app_gw3762_afn05_f18_proc_set_network_type(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 tp_network_type = 0;
    //获取下行使能标志
    tp_network_type = pGw3762Data_t->DataUnit[0];

    if(tp_network_type > e_SINGLE_HRF)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }

    // network_type = tp_network_type;
	SetNetworkType(tp_network_type);

    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}

//组网模式查询
static void app_gw3762_afn03_f18_proc_query_network_type(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{

	APPGW3762DATA_t *Gw3762updata = &AppGw3762UpData;
	U16 Gw3762SendDataLen = 0;

	Gw3762updata->CtrlField.TransDir = APP_GW3762_UP_DIR;
	Gw3762updata->CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata->CtrlField.TransMode = APP_GW3762_TRANS_MODE;

	app_gw3762_up_info_field_slave_pack(&(Gw3762updata->UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
	Gw3762updata->AddrFieldNum = 0;
	Gw3762updata->UpInfoField.ModuleFlag = 0;
	Gw3762updata->Afn = APP_GW3762_AFN03;
	Gw3762updata->Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F18);
	Gw3762updata->DataUnitLen = 1;

	Gw3762updata->DataUnit[0] = GetNetworkType();

	app_gw3762_up_frame_encode(Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
	return;
}

//安徽串口速率转698枚举
U8 baud_to_698enum(U32 arg_baud_e)
{
	U8 baud_enum = 0;

	baud_enum = arg_baud_e == 300?e_300BPS:
                arg_baud_e == 600?e_600BPS:
                arg_baud_e == 1200?e_1200BPS:
                arg_baud_e == 2400?e_2400BPS:
                arg_baud_e == 4800?e_4800BPS:
                arg_baud_e == 7200?e_7200BPS:
                arg_baud_e == 9600?e_9600BPS:
                arg_baud_e == 19200?e_19200BPS:
                arg_baud_e == 38400?e_38400BPS:
                arg_baud_e == 57600?e_57600BPS:
                arg_baud_e == 115200?e_115200BPS:e_SELFADAPTION;

	return baud_enum;
}

//安徽串口速率查询
static void app_gw3762_afn03_f56_query_cco_baud(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn03_f56_query_cco_baud!\n");
    U8  byLen = 0;
    U16 Gw3762SendDataLen = 0;

    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum = 0;
    Gw3762updata.Afn		  = APP_GW3762_AFN03;
    Gw3762updata.Fn			  = app_gw3762_fn_bin_to_bs(APP_GW3762_F56);

    pGW3762updataunit[byLen++] = baud_to_698enum(RunningInfo.BaudRate);
    pGW3762updataunit[byLen++] = e_115200BPS;
    pGW3762updataunit[byLen++] = 0;
    pGW3762updataunit[byLen++] = 0;

    Gw3762updata.DataUnitLen = byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

#if ((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))
AppGw3762ProcFunc AppGw3762ExProcArray[] =
{
    //陕西、湖南拓展    
	/* 66 */{0x00030012, app_gw3762_afn03_f18_proc_query_network_type				},			//陕西/湖南：查询 CCO 模块组网方式
	/* 66 */{0x00050012, app_gw3762_afn05_f18_proc_set_network_type					},			//陕西/湖南：设置 CCO 模块组网方式
    /* 66 */{0x00030066, app_gw3762_afn03_f102_get_master_curve_gather				},			//陕西/湖南：查询 CCO 模块曲线存储数据项
    /* 66 */{0x00050066, app_gw3762_afn05_f102_curve_req_clock						},			//陕西/湖南：设置路由请求终端时钟及全网广播周期
    /* 66 */{0x00050067, app_gw3762_afn05_f103_set_slave_curve_gather				},			//陕西/湖南：设置STA模块曲线存储数据项
    /* 66 */{0x00100067, app_gw3762_afn10_f103_get_slave_curve_gather				},			//陕西/湖南：查询 STA 模块数据项配置状态
    /* 56 */{0x00F10064, app_gw3762_afnf1_f100_concurrent_curve_gather				},			//陕西/湖南：并发抄 STA 模块数据
    /* 88 */{0X00030082, app_gw3762_afn03_f130_query_soft_version 					},			//湖南：基本信息读取命令：查询模块程序版本信息数据单元
    /* 88 */{0X00030083, app_gw3762_afn03_f131_query_code_sha1						},			//湖南：读取程序段校验码：查询比对数据信息数据单元
    

    //陕西新增零火线上报功能
	#ifdef SHAAN_XI    
    /* 66 */{0x00050068, app_gw3762_afn05_f104_set_live_line_zero_line_enabled		},			//陕西：设置零火线异常使能标志
    /* 66 */{0x0010006A, app_gw3762_afn10_f106_get_live_line_zero_line_enabled		},			//陕西：查询零火线异常使能标志
	#endif
    /* 57 */{0x00100066, app_gw3762_afn10_f102_read_sta_phase						},			//陕西：查询节点相位信息

    //山西扩展
    /* 67 */{0x00F00064, app_gw3762_afnf0_f100_query_net_base_msg					},			//查询网络基本信息
    /* 68 */{0x00F00066, app_gw3762_afnf0_f102_query_net_node_msg					},			//查询网络节点信息
    /* 69 */{0x00F00067, app_gw3762_afnf0_f103_query_app_pointed_node_msg			},			//查询指定节点信息
	/* 56 */{0x000500C9, app_gw3762_afn05_f201_proc									},			//湖南：STA 认证使能开启/禁止
																								//河北：允许/ 禁止离网感知上报
																								//山西离网感知：禁止/允许从节点状态变化上报

    /* 73 */{0x000300c9, app_gw3762_afn03_f201_proc									},			//山西离网感知：查询从节点状态变化上报使能标志
																								//湖南：查询STA认证使能开关
	//安徽扩展
	/* 53 */{0x00030038, app_gw3762_afn03_f56_query_cco_baud                        },          //安徽串口速率查询

};
#endif

void app_gw3762_ex_proc(U8 afn, U32 fn, APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 i = 0;
    U32 index = 0;
    U16 gw3762fn = 0;
    U8 count = 0;

    /*//对从动站的帧序号过滤，如果是cco主动发起的帧，从动站(集中器)应答帧序号必须和主动站帧序号相同
    if(pGw3762Data_t->CtrlField.StartFlag == APP_GW3762_SLAVE_PRM
        && (AppPIB_t.AppGw3762UpInfoFrameNum-1) != pGw3762Data_t->DownInfoField.FrameNum)
    {
        app_printf("error pGw3762Data_t.DownInfoField.FrameNum : %d!\n",pGw3762Data_t->DownInfoField.FrameNum);
        return;
    }*/

    gw3762fn = app_gw3762_fn_bs_to_bin(fn);
    index = (afn << 16) | gw3762fn;
    //app_printf("AFN : %04x\n",index);
    count = sizeof(AppGw3762ExProcArray) / (2 * sizeof(U32));

    for(i = 0; i < count; i++)
    {
        if(index == (U32)(AppGw3762ExProcArray[i].Index))
        {
            AppGw3762ExProcArray[i].Func(pGw3762Data_t, port);
            return;
        }
    }

    app_printf("AppGw3762Reserve!\n");
}

#endif
 
