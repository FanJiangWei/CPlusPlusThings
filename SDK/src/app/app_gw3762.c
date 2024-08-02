#include "ZCsystem.h"
#include "flash.h"
#include "app_cnf.h"
#include "aps_interface.h"
#include "app_gw3762_ex.h"
#include "app_gw3762.h"
#include "app_slave_register_cco.h"
#include "app_exstate_verify.h"
#include "app_read_router_cco.h"
#include "app_read_jzq_cco.h"
#include "app_upgrade_cco.h"
#include "app_area_indentification_cco.h"
#include "app_moduleid_query.h"
#include "app_time_calibrate.h"
#include "app.h"
#include "app_rdctrl.h"
#include "stdarg.h"
#include "sys.h"
#include "app_clock_sync_cco.h"
#include "app_clock_sync_sta.h"
#include "app_sysdate.h"
#include "app_base_serial_cache_queue.h"
#include "app_deviceinfo_report.h"
#include "app_698p45.h"
#include "app_meter_common.h"
#include "app_area_change.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_cco_clock_overproof_event.h"
#include "app_clock_os_ex.h"
#include "dl_mgm_msg.h"
#include "dev_manager.h"
#include "app_dltpro.h"
#include "printf_zc.h"
#include "Version.h"
#include "Beacon.h"
#include "netnnib.h"
#include "meter.h"
#include "app_phase_position_cco.h"
#include "app_clock_cco_exact_time_cali.h"

SysDate_t SysDate					= {0};				/* 当前系统时钟时钟 */

#if defined(STATIC_MASTER)

extern ostimer_t *ReUpgradetimer ;
extern CCO_UPGRADE_CTRL_INFO_t   CcoUpgrdCtrlInfo;
extern void uart_send(uint8_t *buffer, uint16_t len);
extern EventReportStatus_e    g_EventReportStatus;
extern U8 rdaddr[6];
extern U8 DstCoordRfChannel;


APPGW3762DATA_t AppGw3762DownData;
APPGW3762DATA_t AppGw3762UpData;
AppGw3762Afn13F1State_t AppGw3762Afn13F1State;
U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] ;
U8 g_QueryLastRecordTimes = 0;
APPGW3762AFN10F4_t   BackupAppGw3762Afn10F4State;
ostimer_t *BcdTimeCaltimer = NULL;
U8		BcCusAddr = 0;
BAUD_INFO_t BaudInfo = {0};
U8  MASTERADDFLAG = FALSE;
U32 g_timekeep = 0;
APPGW3762AFN10F4_t   AppGw3762Afn10F4State;
ostimer_t *flashtimer = NULL;
APPGW3762AFN11F5_ST  stAFN11F5;
U8		STARegisterFlag = FALSE;
U8	NewMeterReport = FALSE;
AppGw3762Afn14F1State_t AppGw3762Afn14F1State;
U8 report_frame_seq = 0;
ostimer_t *Reprotwatertimer = NULL;
APPGW3762AFNF0F8_t   AppGw3762AfnF0F8State;
APPGW3762DATA_t   Gw3762updata;
AppGw3762Afn06F2State_t   AppGw3762Afn06F2State;
U8 JZQ_Set_Band = 2;
PIID_s Bcpiid = { 0, 0, 0 };

U8 APP_GW3762_TRANS_MODE = APP_GW3762_HPLC_HRF_CTRL_COMMMODE;

static void app_gw3762_up_afn10_fn_up_frame(U16 Fn, U8 TransMode, U16 StartIndex, U8 num, MESG_PORT_e port);
static U8 order_err(DEVICE_TEI_LIST_t GetDeviceListTemp_t);
static void app_gw3762_up_afn10_f40_query_id_info(U8 *nodeAddr, U8 deviceType, U8 IdType, U8 *data, U16 len, MESG_PORT_e port);
static void app_gw3762_up_afn14_f3_router_req_revise(U8 *addr, U16 time, U8 *data, U16 len, MESG_PORT_e port);
static U8  get_cnm_addr_by_mac_addr(U8 *MacAddr, U8 *CnmAddr);
static void app_gw3762_up_afn10_fn_by_whitelist_up_frame(U16 Fn, U8 TransMode, U16 StartIndex, U8 num, MESG_PORT_e port);


#if ((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))
/*************************************************************************
 * 函数名称	: 	static U16 app_gw3762_fn_bs_to_bin(U16 data)
 * 函数说明	: 	将Fn的BS转化成BIN格式
 * 参数说明	: 	data	- BS格式Fn
 * 返回值		: 	BIN格式Fn
 *************************************************************************/
U16 app_gw3762_fn_bs_to_bin(U16 data)
{
    U16 fn = 0;
    U8 i;
    U8 dt1 = data;

    for(i = 0 ; i < 8 ; i ++)
    {
        if((data >> i) & 0x01)
        {
            if(((dt1 >> i) | 0x01) == 0x01)
            {
                fn = (data >> 8) * 8 + (i + 1);
            }
            else
            {
                fn = 0xFFFF;
            }
            break;
        }
    }

    return fn;
}

/*************************************************************************
 * 函数名称	: 	static U16 app_gw3762_fn_bin_to_bs(AppGw3762Fn_e data)
 * 函数说明	: 	将Fn的BIN转化成BS格式
 * 参数说明	: 	data	- BIN格式Fn
 * 返回值		: 	BS格式Fn
 *************************************************************************/
U16 app_gw3762_fn_bin_to_bs(AppGw3762Fn_e data)
{
    U16 fn = 0;
    U8 fn1 = 0;
    U8 fn2 = 0;

    if(0 == (fn1 = data % 8))
    {
        fn1 = 7;
        fn2 = data / 8 - 1;
    }
    else
    {
        fn1 -= 1;
        fn2 = data / 8;
    }

    fn1 = 1 << fn1;

    fn = ((U16)fn2 << 8) | (U16)fn1;

    return fn;
}

static U8 cal_signal_quality(U8 DeviceInfoNullFlag,U8 SuccRatio)
{
    U8 SignalQuality;
    if(app_ext_info.func_switch.UseMode == 1&&app_ext_info.func_switch.SignalQualitySWC == 1&&DeviceInfoNullFlag == TRUE)
    {
        if((SuccRatio+6)<107)
        {
            SignalQuality = (SuccRatio+6)/7;
            return SignalQuality;
        }
    }
    return APP_GW3762_UPINFO_CMDQUALITY;
}

 /**
 * @brief	上行帧从设备信息域处理
 * @param	AppGw3762UpInfoField_t  *upinfofield		信息域指针
 * @param   U8                      mode                通信模块标识
 * @param   U8                      *Nodeaddr           节点地址
 * @param   U8                      localseq            序号
 * @return	void
 *******************************************************************************/
void app_gw3762_up_info_field_slave_pack(AppGw3762UpInfoField_t *upinfofield, U8 mode ,U8 *Nodeaddr,U8 localseq)
{
    U8 DeviceInfoNullFlag;
    
    DeviceInfoNullFlag = FALSE;
	DEVICE_TEI_LIST_t DeviceListTemp;

    memset(&DeviceListTemp,0x00,sizeof(DEVICE_TEI_LIST_t));

	if(Nodeaddr!=NULL)
    {   
	    if(DeviceList_ioctl(DEV_GET_ALL_BYMAC,Nodeaddr, &DeviceListTemp))
        {
            DeviceInfoNullFlag = TRUE;
        }
        else
        {
            U8	 addr[6] = {0};
            if(get_cnm_addr_by_mac_addr(Nodeaddr,addr) == TRUE)
            {
                ChangeMacAddrOrder(addr);
                if(DeviceList_ioctl(DEV_GET_ALL_BYMAC,addr, &DeviceListTemp))
                {
                    DeviceInfoNullFlag = TRUE;
                }
            }
        }
    }

    upinfofield->RouterFlag 				= APP_GW3762_UPINFO_ROUTERFLAG;
    upinfofield->ModuleFlag				= mode;
    upinfofield->RelayLevel				= APP_GW3762_UPINFO_RELAYLEVEL;
    upinfofield->ChanFlag				= APP_GW3762_UPINFO_CHANFLAG;
    upinfofield->PhaseFlag				= (DeviceInfoNullFlag==FALSE)?APP_GW3762_UPINFO_PHASEFLAG:DeviceListTemp.Phase;
    upinfofield->ChanFeat				= APP_GW3762_UPINFO_CHANFEAT;
    upinfofield->CmdQuality				= (DeviceInfoNullFlag==FALSE)?APP_GW3762_UPINFO_CMDQUALITY:
        (cal_signal_quality(DeviceInfoNullFlag,DeviceListTemp.DownlinkSuccRatio)&0x0F);//APP_GW3762_UPINFO_CMDQUALITY;
    upinfofield->ReplyQuality			= (DeviceInfoNullFlag==FALSE)?APP_GW3762_UPINFO_REPLYQUALITY:
        (cal_signal_quality(DeviceInfoNullFlag,DeviceListTemp.UplinkSuccRatio)&0x0F);//APP_GW3762_UPINFO_REPLYQUALITY;
    upinfofield->EventFlag				= APP_GW3762_UPINFO_EVENTFLAG;
    upinfofield->FrameNum				= localseq;
    upinfofield->Reserve1				= 0;
    upinfofield->Reserve2				= 0;
    upinfofield->Reserve3				= 0;
    //upinfofield->Reserve4				= 0;

	upinfofield->LineErr				= (DeviceInfoNullFlag==FALSE)?APP_GW3762_UPINFO_PHASEFLAG:(order_err(DeviceListTemp));
    
	upinfofield->AreaFlag				=(DeviceInfoNullFlag==FALSE)? APP_GW3762_UPINFO_PHASEFLAG:(DeviceListTemp.AREA_ERR==FALSE?1:0);
}

 /**
 * @brief	上行帧主设备信息域处理
 * @param	AppGw3762UpInfoField_t  *upinfofield		信息域指针
 * @param   U8                      mode                通信模块标识
 * @param   U8                      *Nodeaddr           节点地址
 * @return	void
 *******************************************************************************/
void app_gw3762_up_info_field_master_pack(AppGw3762UpInfoField_t *upinfofield,U8 *Nodeaddr)
{
	DEVICE_TEI_LIST_t DeviceListTemp;
	if(Nodeaddr!=NULL)
	//Get_DeviceList_All_ByAdd(Nodeaddr,&DeviceListTemp);
	DeviceList_ioctl(DEV_GET_ALL_BYMAC,Nodeaddr, &DeviceListTemp);

    upinfofield->RouterFlag 				= APP_GW3762_UPINFO_ROUTERFLAG;
    upinfofield->ModuleFlag				= APP_GW3762_UPINFO_MODULEFLAG;
    upinfofield->RelayLevel				= APP_GW3762_UPINFO_RELAYLEVEL;
    upinfofield->ChanFlag				= APP_GW3762_UPINFO_CHANFLAG;
    upinfofield->PhaseFlag				=  (Nodeaddr==NULL)?APP_GW3762_UPINFO_PHASEFLAG:DeviceListTemp.Phase;
    upinfofield->ChanFeat				= APP_GW3762_UPINFO_CHANFEAT;
    upinfofield->CmdQuality				= APP_GW3762_UPINFO_CMDQUALITY;
    upinfofield->ReplyQuality			= APP_GW3762_UPINFO_REPLYQUALITY;
    upinfofield->EventFlag				= APP_GW3762_UPINFO_EVENTFLAG;
    upinfofield->FrameNum				= ++AppPIB_t.AppGw3762UpInfoFrameNum;
    upinfofield->Reserve1				= 0;
    upinfofield->Reserve2				= 0;
    upinfofield->Reserve3				= 0;
    //upinfofield->Reserve4				= 0;
	
	upinfofield->LineErr				= (Nodeaddr==NULL)?APP_GW3762_UPINFO_PHASEFLAG:(order_err(DeviceListTemp));
	upinfofield->AreaFlag				=(Nodeaddr==NULL)? APP_GW3762_UPINFO_PHASEFLAG:(DeviceListTemp.AREA_ERR==FALSE?1:0);
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Reserve(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762协议错误处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_reserve(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Reserve!");
}

 /**
 * @brief	本地协议下行确认帧处理接口
 * @param	APPGW3762DATA_t         *pGw3762Data_t      下行数据
 * @param   MESG_PORT_e             port                消息端口
 * @return	void
 *******************************************************************************/
static void app_gw3762_afn00_f1_sure(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Afn00F1Sure!\n");
}

 /**
 * @brief	本地协议下行否认帧处理接口
 * @param	APPGW3762DATA_t         *pGw3762Data_t      下行数据
 * @param   MESG_PORT_e             port                消息端口
 * @return	void
 *******************************************************************************/
static void app_gw3762_afn00_f2_deny(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Afn00F2Deny!\n");
}

/*************************************************************************
 * 函数名称	: 	 static void AppGw3762Afn01F1HardInit(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN01F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn01_f1_hard_init(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("AppGw3762Afn01F1HardInit!");
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    reboot_after_1s();
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn01F2ParamInit(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN01F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn01_f2_param_init(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    LEAVE_IND_t *pleaveInd = zc_malloc_mem(sizeof(LEAVE_IND_t), "LEAVE", MEM_TIME_OUT);
    WHLPTST //共享存储内容保护
    RouterActiveInfo_t.CrnRMRound	 =  0;
    RouterActiveInfo_t.CrnMeterIndex =  0;
    FlashInfo_t.usJZQRecordNum =  0;	
    U16 i = 0;
    do
    {
        memset(&WhiteMacAddrList[i],0,sizeof(WHITE_MAC_LIST_t));
    }while(++i<MAX_WHITELIST_NUM);
    WHLPTED//释放共享存储内容保护

    if(PROVINCE_HEBEI == app_ext_info.province_code) //todo: PROVINCE_HEBEI
    {
        //河北，数据初始化，没设置主节点地址则进行清除主节点地址操作
        if(MASTERADDFLAG == FALSE)
        {
            memset(FlashInfo_t.ucJZQMacAddr, 0x00, 6);
            app_printf("need change master addr\n");
            CheckJZQMacAddr();
            SetMacAddrRequest(FlashInfo_t.ucJZQMacAddr, e_JZQ, 0);
        }
    }

    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

    app_printf("--------------------AppGw3762Afn01F2ParamInit----------------------\n");

    //sysflag=TRUE;
    //whitelistflag = TRUE;
    //mutex_post(&mutexSch_t);
    flash_timer_start();
    if(APP_GETDEVNUM() != 0)
    {
        pleaveInd->StaNum = 0xaa;
        pleaveInd->DelType = e_LEAVE_AND_DEL_DEVICELIST;
        ApsSlaveLeaveIndRequst(pleaveInd);
        
        timer_start(Renetworkingtimer);
        timer_stop(Renetworkingtimer,TMR_CALLBACK);
    }
    zc_free_mem(pleaveInd);
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn01F3DataInit(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN01F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn01_f3_data_init(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    router_evt_router_reset();
    if(PROVINCE_HEBEI != app_ext_info.province_code)//todo: != PROVINCE_HEBEI
    {
	    sysflag=TRUE;
        whitelistflag = TRUE;
    }
	app_printf("AppGw3762Afn01F3DataInit!");
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn02F1DataTrans(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN02F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn02_f1_data_trans(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U8  findMeterFlag = FALSE;
    U16   ii;
    
    //刷新传文件保护
   // timer_start(ReUpgradetimer);
    
    //AppPIB_t.AppGw3762UpInfoFrameNum = pGw3762Data_t->DownInfoField.FrameNum;

    /* 解析AFN13F1数据单元到Gw3762Afn13F1Param */
    AppGw3762Afn13F1State.ProtoType		= *gw3762downdataunit ++;
    AppGw3762Afn13F1State.FrameLen		= *gw3762downdataunit ++;

    if(AppGw3762Afn13F1State.FrameLen != 0)
    {
        __memcpy(AppGw3762Afn13F1State.FrameUnit, gw3762downdataunit, AppGw3762Afn13F1State.FrameLen);
    }
    
    //U16 len=0;
    if(pGw3762Data_t->DownInfoField.ModuleFlag == 1)
	{
		__memcpy(AppGw3762Afn13F1State.Addr,pGw3762Data_t->AddrField.DestAddr,6);	
	}
	else
	{
		if(TRUE==Check645Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL,AppGw3762Afn13F1State.Addr,NULL))
		{
			//AppGw3762Afn13F1State.ProtoType = TRANS_PROTO_07;
			//AppGw3762Afn13F1State.FrameLen=len;
		}
		else if(TRUE==Check698Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL,AppGw3762Afn13F1State.Addr, NULL))
		{
            app_printf("AppGw3762Afn13F1State.Addr : ");
            dump_buf(AppGw3762Afn13F1State.Addr, 6); 
			//AppGw3762Afn13F1State.FrameLen=len;
        }
		/*
		else if(TRUE==CheckT188Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,AppGw3762Afn13F1State.Addr))
		{
			AppGw3762Afn13F1State.ProtoType = TRANS_PROTO_RF;		
		}*/
		else
		{
			app_printf("error FrameUnit : ");
			dump_buf(AppGw3762Afn13F1State.FrameUnit, AppGw3762Afn13F1State.FrameLen);
            app_gw3762_up_afn00_f2_deny(APP_GW3762_FORMAT_ERRCODE, port);
        	return;
		}
	}
	app_printf("AppGw3762Afn13F1State.Addr : ");
	dump_buf(AppGw3762Afn13F1State.Addr, 6);
	
    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        //dump_buf(WhiteMacAddrList[ii].MacAddr, 6);
        if(memcmp(WhiteMacAddrList[ii].MacAddr, AppGw3762Afn13F1State.Addr, MAC_ADDR_LEN) == 0)
        {
			findMeterFlag = TRUE;
			
	        __memcpy(AppGw3762Afn13F1State.CnmAddr, WhiteMacAddrList[ii].CnmAddr, MAC_ADDR_LEN);
            app_printf("AppGw3762Afn13F1State.cnmaddr : ");
            dump_buf(WhiteMacAddrList[ii].CnmAddr, 6);
                
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
    AppGw3762Afn13F1State.ValidFlag = TRUE;
    AppGw3762Afn13F1State.Afn02Flag = TRUE;
    AppGw3762Afn14F1State.ValidFlag = FALSE;

    jzq_read_cntt_act_read_meter_req(pGw3762Data_t->DownInfoField.FrameNum, AppGw3762Afn13F1State.Addr, AppGw3762Afn13F1State.CnmAddr, 
                        AppGw3762Afn13F1State.ProtoType, AppGw3762Afn13F1State.FrameLen, AppGw3762Afn13F1State.FrameUnit, 
                        AppGw3762Afn13F1State.Afn02Flag, port);
}

void app_gw3762_up_afn02_f1_up_frame(U8 *Addr, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port,U8 localseq)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16  byLen = 0;
    U8 addr[6]={0};
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 1, Addr,localseq);

    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;
    Gw3762updata.Afn				= APP_GW3762_AFN02;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);

    __memcpy(Gw3762updata.AddrField.SrcAddr, Addr, MAC_ADDR_LEN);

	if(TRUE==CheckT188Frame(data,len,addr))
	{
	    __memcpy(Gw3762updata.AddrField.SrcAddr, addr, MAC_ADDR_LEN);
		proto = APP_DOUBLEMETER_PROTOTYPE;
	}
    __memcpy(Gw3762updata.AddrField.DestAddr, FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN);
	//辽宁要求回复空帧
    //if(len == 0) proto = 0;

	Gw3762updata.DataUnit[byLen++] = proto;
    Gw3762updata.DataUnit[byLen++] = len;
	
    if(len > 0)
    {
        __memcpy(&Gw3762updata.DataUnit[byLen], data, len);
        byLen += len;
    }
	
    Gw3762updata.DataUnitLen		= byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, localseq, FALSE, 0, port,e_Serial_AFN13F1);
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn03F1ManufCode(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f1_manuf_code(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;

    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 		= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 		= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 		= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);
    Gw3762updata.DataUnitLen				= sizeof(DCR_MANUFACTURER_t);

    __memcpy(Gw3762updata.DataUnit, (U8 *) & (FlashInfo_t.ManuFactor_t), sizeof(DCR_MANUFACTURER_t));
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	app_printf("Gw3762SendDataLen = %d\n",Gw3762SendDataLen);
	dump_buf(Gw3762SendData, Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE,0,port, e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn03F2Noise(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f2_noise(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;

    app_printf("AppGw3762Afn03F2Noise!");
    
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 		= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 		= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 		= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F2);
    Gw3762updata.DataUnitLen		       = 1;

    Gw3762updata.DataUnit[0] = 0x0F;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f3_slave_monitor(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f3_slave_monitor(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;

    app_printf("app_gw3762_afn03_f3_slave_monitor!");
    
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 		= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 		= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 		= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F3);
    Gw3762updata.DataUnitLen		       = 2;

    Gw3762updata.DataUnit[0] = 0x00;
    Gw3762updata.DataUnit[1] = 0x00;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f4_master_addr(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F4处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f4_master_addr(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;

    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN03;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F4);
    Gw3762updata.DataUnitLen			= 6;
    __memcpy(Gw3762updata.DataUnit, FlashInfo_t.ucJZQMacAddr, 6);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f5_master_state_speed(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F5处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f5_master_state_speed(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *gw3762updataunit					= Gw3762updata.DataUnit;
    APPGW3762AFN03F5_t stAFN03F5 = {0};
    stAFN03F5.ucSpeedNum					= APP_GW3762_SPEED_NUM;
    stAFN03F5.ucChanFeature					= APP_GW3762_ST_CHANFEAT;
    stAFN03F5.ucCycleReadMode				= APP_GW3762_D_PERIODMETER;
    stAFN03F5.ucChanNum						= APP_GW3762_CHAN_NUM;
    stAFN03F5.ucReserve						= 0;
    #if defined(STD_DUAL)
    stAFN03F5.usBaudSpeed					= 0;        //通信速率，双模建议为0
    stAFN03F5.usBaudUnit					= 0;        //速率单位标识，双模建议为0
    #else
    stAFN03F5.usBaudSpeed					= FlashInfo_t.WorkMode_t.usBaudSpeed;
    stAFN03F5.usBaudUnit					= FlashInfo_t.WorkMode_t.usBaudUnit;
    #endif
    Gw3762updata.CtrlField.TransDir 			= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 			= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 			= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 				= 0;
    Gw3762updata.Afn						= APP_GW3762_AFN03;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F5);
    Gw3762updata.DataUnitLen					= sizeof(APPGW3762AFN03F5_t);
    __memcpy(gw3762updataunit, (U8 *)&stAFN03F5, sizeof(APPGW3762AFN03F5_t));
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f6_master_disturb(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F6处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f6_master_disturb(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F6);
    Gw3762updata.DataUnitLen			= 1;
    *(Gw3762updata.DataUnit)			= APP_GW3762_N_MASTERDIST;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f7_slave_timeout(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F7处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f7_slave_timeout(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F7);
    Gw3762updata.DataUnitLen			= 1;
    *(Gw3762updata.DataUnit)			= FlashInfo_t.ucSlaveReadTimeout<3?60:FlashInfo_t.ucSlaveReadTimeout;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

U8 rf_channel_group = 0;
U8 rf_power = e_LOWEST_POWER;

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f8_query_rf_param(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F8处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f8_query_rf_param(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn03_f8_query_rf_param!");
    if(0)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
        return;
    }
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F8);
    Gw3762updata.DataUnitLen			= 2;
    Gw3762updata.DataUnit[0]			= rf_channel_group;
    Gw3762updata.DataUnit[0]			= rf_power;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);

}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f9_query_slave_timeout(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F9处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f9_query_slave_timeout(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F9);
    Gw3762updata.DataUnitLen			= pGw3762Data_t->DataUnitLen + 2;
    Gw3762updata.DataUnit[0]			= BC_TIMEOUT&0xff;
    Gw3762updata.DataUnit[1]			= (BC_TIMEOUT>>8)&0xff;
    __memcpy(Gw3762updata.DataUnit + 2, pGw3762Data_t->DataUnit, pGw3762Data_t->DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f10_master_mode(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F10处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f10_master_mode(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn03_f10_master_mode!");
    app_gw3762_up_afn03_f10_master_mode(TRUE, port);
}

/*************************************************************************
 * 函数名称	: 	void set_fn_bit_map(U8 *Map , U8 n, ... )
 * 函数说明	: 	自动按照FN填写03F11数据载荷
 * 参数说明	: 	U8 *Map数据载荷 , U8 n 使用的FN数量, ... 支持可变参数
 * 返回值		: 	无
 *************************************************************************/
static void set_fn_bit_map(U8 *Map , U8 n, ... )
{
    U8 i = 0;
    U8  FnTemp;
    va_list argptr;
    va_start( argptr, n );              // 初始化argptr
    for ( i = 0; i < n; ++i )           // 对每个可选参数，进行置位操作
    {
        FnTemp = 0;
        FnTemp = va_arg(argptr, int);
        bitmap_set_bit(Map,FnTemp-1);   // 按照FN置位
    }
    
    va_end( argptr );
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f11_master_index(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F11处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f11_master_index(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 tempbuf[32] = {0};
    memset(tempbuf, 0x00, sizeof(tempbuf));

    switch(*(pGw3762Data_t->DataUnit))
    {
    case APP_GW3762_AFN00:
        set_fn_bit_map(tempbuf ,2 ,APP_GW3762_F1 ,APP_GW3762_F2);
        break;
    case APP_GW3762_AFN01:  
        set_fn_bit_map(tempbuf ,3 ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3);
        break;
    case APP_GW3762_AFN02:
        set_fn_bit_map(tempbuf ,1 ,APP_GW3762_F1);
        break;
    case APP_GW3762_AFN03:
        if (PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
        {
            set_fn_bit_map(tempbuf, 22, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F8, APP_GW3762_F9
                , APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F21
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F102, APP_GW3762_F130, APP_GW3762_F131
                , APP_GW3762_F201);
        }
        else if (PROVINCE_SHANNXI == app_ext_info.province_code)//todo: PROVINCE_SHANNXI
        {
            set_fn_bit_map(tempbuf, 19, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F8, APP_GW3762_F9
                , APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F21
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F102);
        }
        else if(PROVINCE_SHANXI == app_ext_info.province_code)//todo: PROVINCE_SHANXI
        {
            set_fn_bit_map(tempbuf, 19, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F8, APP_GW3762_F9
                , APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F21
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F201);
        }
        else if(PROVINCE_JIANGSU == app_ext_info.province_code)//todo: PROVINCE_JIANGSU
        {
            set_fn_bit_map(tempbuf, 17, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7,  APP_GW3762_F9
                , APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F21
                , APP_GW3762_F100, APP_GW3762_F101);
        }
        else if(PROVINCE_ANHUI == app_ext_info.province_code)//todo: PROVINCE_ANHUI
        {
            set_fn_bit_map(tempbuf, 19, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F8, APP_GW3762_F9
                , APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F21
                , APP_GW3762_F56, APP_GW3762_F100, APP_GW3762_F101);
        }
        else 
        {
            set_fn_bit_map(tempbuf, 18, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F8, APP_GW3762_F9
                , APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F21
                , APP_GW3762_F100, APP_GW3762_F101);
        }
        break;
    case APP_GW3762_AFN04:
        set_fn_bit_map(tempbuf ,3 ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3);
        break;
    case APP_GW3762_AFN05:
        if (PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
        {
            set_fn_bit_map(tempbuf, 20, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4, APP_GW3762_F5
                , APP_GW3762_F6, APP_GW3762_F10, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F90, APP_GW3762_F92, APP_GW3762_F93, APP_GW3762_F94
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F102, APP_GW3762_F103, APP_GW3762_F130, APP_GW3762_F201);
        }
        else if (PROVINCE_SHANNXI == app_ext_info.province_code) //todo: PROVINCE_SHANNXI
        {
            set_fn_bit_map(tempbuf, 20, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4, APP_GW3762_F5
                , APP_GW3762_F6, APP_GW3762_F10, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F90, APP_GW3762_F92, APP_GW3762_F93, APP_GW3762_F94
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F102, APP_GW3762_F103, APP_GW3762_F104, APP_GW3762_F130);
        }
        else if(PROVINCE_HEBEI == app_ext_info.province_code)//todo:PROVINCE_HEBEI
        {
            set_fn_bit_map(tempbuf, 19, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4, APP_GW3762_F5
                , APP_GW3762_F6, APP_GW3762_F10, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F90, APP_GW3762_F92, APP_GW3762_F93, APP_GW3762_F94
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F130, APP_GW3762_F200, APP_GW3762_F201);
        }
        else if(PROVINCE_SHANXI == app_ext_info.province_code)//todo:PROVINCE_SHANXI
        {
            set_fn_bit_map(tempbuf, 17, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4, APP_GW3762_F5
                , APP_GW3762_F6, APP_GW3762_F10, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F90, APP_GW3762_F92, APP_GW3762_F93, APP_GW3762_F94
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F130, APP_GW3762_F201);
        }
        else if(PROVINCE_JIANGSU == app_ext_info.province_code)//todo: PROVINCE_JIANGSU
        {
            set_fn_bit_map(tempbuf, 16, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F6, APP_GW3762_F10, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F90, APP_GW3762_F92, APP_GW3762_F93, APP_GW3762_F94
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F130);
        }
        else
        {
            set_fn_bit_map(tempbuf, 17, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4, APP_GW3762_F5
                , APP_GW3762_F6, APP_GW3762_F10, APP_GW3762_F16, APP_GW3762_F17, APP_GW3762_F18, APP_GW3762_F90, APP_GW3762_F92, APP_GW3762_F93, APP_GW3762_F94
                , APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F130);
        }
        break;
    case APP_GW3762_AFN06:
        set_fn_bit_map(tempbuf ,6 ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3 ,APP_GW3762_F4 
                               ,APP_GW3762_F5 ,APP_GW3762_F10);
        break;
    case APP_GW3762_AFN10:
        if (PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
        {
            set_fn_bit_map(tempbuf, 22, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F9, APP_GW3762_F20
                , APP_GW3762_F21, APP_GW3762_F31, APP_GW3762_F32, APP_GW3762_F33, APP_GW3762_F40, APP_GW3762_F90, APP_GW3762_F93, APP_GW3762_F100
                , APP_GW3762_F101, APP_GW3762_F103, APP_GW3762_F104, APP_GW3762_F111, APP_GW3762_F112);
        }
        else if (PROVINCE_SHANNXI == app_ext_info.province_code) //todo: PROVINCE_SHANNXI
        {
            set_fn_bit_map(tempbuf, 24, APP_GW3762_F1, APP_GW3762_F2, APP_GW3762_F3, APP_GW3762_F4
                , APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F9, APP_GW3762_F20
                , APP_GW3762_F21, APP_GW3762_F31, APP_GW3762_F32, APP_GW3762_F33, APP_GW3762_F40, APP_GW3762_F90, APP_GW3762_F93, APP_GW3762_F100
                , APP_GW3762_F101, APP_GW3762_F102, APP_GW3762_F103, APP_GW3762_F104, APP_GW3762_F106, APP_GW3762_F111, APP_GW3762_F112);
        }
        else
        {
            set_fn_bit_map(tempbuf ,22 ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3 ,APP_GW3762_F4 
                ,APP_GW3762_F5 ,APP_GW3762_F6 ,APP_GW3762_F7 ,APP_GW3762_F9, APP_GW3762_F20
                ,APP_GW3762_F21 ,APP_GW3762_F31 , APP_GW3762_F32, APP_GW3762_F33, APP_GW3762_F40 ,APP_GW3762_F90,APP_GW3762_F93,APP_GW3762_F100
                , APP_GW3762_F101,APP_GW3762_F103 ,APP_GW3762_F104 ,APP_GW3762_F111,APP_GW3762_F112);
        }
        break;
    case APP_GW3762_AFN11:
        set_fn_bit_map(tempbuf ,10 ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3 ,APP_GW3762_F4 
                               ,APP_GW3762_F5 ,APP_GW3762_F6, APP_GW3762_F100, APP_GW3762_F101, APP_GW3762_F102, APP_GW3762_F225);
        break;
    case APP_GW3762_AFN12:
        set_fn_bit_map(tempbuf ,3 ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3);
        break;
    case APP_GW3762_AFN13:
        set_fn_bit_map(tempbuf ,1  ,APP_GW3762_F1);
        break;
    case APP_GW3762_AFN14:
        set_fn_bit_map(tempbuf ,4  ,APP_GW3762_F1 ,APP_GW3762_F2 ,APP_GW3762_F3 ,APP_GW3762_F4);
        break;
    case APP_GW3762_AFN15:
        set_fn_bit_map(tempbuf ,1  ,APP_GW3762_F1);
        break;       
    case APP_GW3762_AFNF0:
        if(PROVINCE_SHANXI == app_ext_info.province_code) //todo: PROVINCE_SHANXI
        {
            set_fn_bit_map(tempbuf, 14, APP_GW3762_F5, APP_GW3762_F6, APP_GW3762_F7, APP_GW3762_F8
                , APP_GW3762_F9, APP_GW3762_F10, APP_GW3762_F11, APP_GW3762_F12
                , APP_GW3762_F13, APP_GW3762_F14, APP_GW3762_F15, APP_GW3762_F100
                , APP_GW3762_F102, APP_GW3762_F103);
        }
        else
        {
            set_fn_bit_map(tempbuf ,11 ,APP_GW3762_F5 ,APP_GW3762_F6 ,APP_GW3762_F7 ,APP_GW3762_F8 
                                    ,APP_GW3762_F9 ,APP_GW3762_F10 ,APP_GW3762_F11 ,APP_GW3762_F12
                                    ,APP_GW3762_F13 ,APP_GW3762_F14 ,APP_GW3762_F15);
        }
        break;    
    case APP_GW3762_AFNF1:
        set_fn_bit_map(tempbuf ,2 ,APP_GW3762_F1 ,APP_GW3762_F100);
        break;
    default:
        break;
    }

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F11);
    Gw3762updata.DataUnitLen			= 33;
    *(Gw3762updata.DataUnit)			= *(pGw3762Data_t->DataUnit);
    memset((Gw3762updata.DataUnit + 1), 0x00, 32);
    __memcpy((Gw3762updata.DataUnit + 1), tempbuf, sizeof(tempbuf));
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afn03_f12_master_module_id_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{	 
	app_printf("app_gw3762_afn03_f12_master_module_id_info!");
	
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8  byLen = 0;
	U8  null_moduleID[11] = {0};

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN03;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F12);
	__memcpy(pGW3762updataunit,FlashInfo_t.ManuFactor_t.ucVendorCode,2);
    //pGW3762updataunit += 2 ;
	byLen += 2;
	if(memcmp(ModuleID,null_moduleID,11)==0)
	{
        pGW3762updataunit[byLen++] = 0x01;
		pGW3762updataunit[byLen++] = 0x02;
		pGW3762updataunit[byLen++] = 0xFF;//模块ID没有获得；0x00不支持模块ID读取
	}
	else
	{
        pGW3762updataunit[byLen++] = sizeof(ModuleID);
        if(PROVINCE_JIANGSU == app_ext_info.province_code)
		    pGW3762updataunit[byLen++] = BIN_TYPE;
		else
		    pGW3762updataunit[byLen++] = BCD_TYPE;
		    
        __memcpy(&pGW3762updataunit[byLen], ModuleID, sizeof(ModuleID));
        byLen += sizeof(ModuleID);
	}
    Gw3762updata.DataUnitLen = byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn03_f100_query_rssi_threshold(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F100处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn03_f100_query_rssi_threshold(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn03_f100_query_rssi_threshold!");
    app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}
#endif

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn04_f1_send_test(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN04F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn04_f1_send_test(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	U8 *pDataUnit = pGw3762Data_t->DataUnit;
	U8 timekeep = *pDataUnit;

	app_printf("app_gw3762_afn04_f1_send_test  keeptmie = %d \r\n",timekeep);

    /*
	if(NULL != Localprotocol04F1Tmier )
	{
		if(!timekeep) 
		{
			if(TMR_RUNNING == zc_timer_query(Localprotocol04F1Tmier))
			{
				app_printf("Localprotocol04F1Tmier close \r\n");
				timer_stop(Localprotocol04F1Tmier,TMR_NULL);
			}
		}
		else
		{
			if(	TMR_RUNNING == zc_timer_query(Localprotocol04F1Tmier)) //再次触发
			{
				app_printf("Localprotocol04F1Tmier close \r\n");
				timer_stop(Localprotocol04F1Tmier,TMR_NULL);
			}
			g_timekeep = timekeep * 1000; //转换 成毫秒
			timer_start(Localprotocol04F1Tmier);
		}
		
		app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	}
	else
	{
		app_printf("creat Localprotocol04F1Tmier fail\r\n");
		app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
	}
	*/
	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	return ;
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn04_f2_slave_roll(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN04F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn04_f2_slave_roll(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	if(0 == FlashInfo_t.usJZQRecordNum)
	{
		app_printf("app_gw3762_afn04_f2_slave_roll!");
    	app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
	}
	else
	{
	    /*
		timer_stop(Localprotocol04F1Tmier,TMR_NULL);
		g_timekeep = 500; //定时器执行一次
		timer_start(Localprotocol04F1Tmier);
		*/
		app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	}
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn04_f3_master_test(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN04F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn04_f3_master_test(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *pDataUnit = pGw3762Data_t->DataUnit;
    U8  protocolType;
    U8  dstAddr[6];
    U8  cnmAddr[6];
    U8  packetLen;
    U8  findAddrFlag = FALSE;
    U16  index;
	U8  BCDstAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    COMMU_TEST_REQ_t *pCommuTestRequest_t=NULL;    

    // testCommRate
    pDataUnit++;

    __memcpy(dstAddr, pDataUnit, 6);
    pDataUnit += 6;
    protocolType = *pDataUnit++;
    packetLen = *pDataUnit++;

    for(index = 0; index < MAX_WHITELIST_NUM; index++)
    {
        if(memcmp(dstAddr, WhiteMacAddrList[index].MacAddr, 6) == 0 || memcmp(dstAddr, BCDstAddr , 6) == 0)
        {
            findAddrFlag = TRUE;
            if(memcmp(dstAddr, BCDstAddr , 6) == 0)
            {
                __memcpy(cnmAddr, BCDstAddr, 6);
            }
            else
            {
            __memcpy(cnmAddr, WhiteMacAddrList[index].CnmAddr, 6);
            }
            break;
        }
    }

    if(findAddrFlag == FALSE)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
        return;
    }

    //申请原语空间
	pCommuTestRequest_t = (COMMU_TEST_REQ_t*)zc_malloc_mem(sizeof(COMMU_TEST_REQ_t) + packetLen, 
	                                                                                "CommuTestReq",MEM_TIME_OUT);
	if(packetLen == 2)
    {
        pCommuTestRequest_t->TestModeCfg = *pDataUnit++;
        pCommuTestRequest_t->TimeOrCfgValue = *pDataUnit++;

        pCommuTestRequest_t->AsduLength = 0;
    }
    else
    {
        pCommuTestRequest_t->TestModeCfg = 1;
        __memcpy(pCommuTestRequest_t->Asdu, pDataUnit, packetLen);
        pCommuTestRequest_t->AsduLength = packetLen;
    }        

    pCommuTestRequest_t->ProtocolType = protocolType;         
    __memcpy(pCommuTestRequest_t->DstMacAddr, cnmAddr, 6);
    __memcpy(pCommuTestRequest_t->SrcMacAddr, FlashInfo_t.ucJZQMacAddr, 6);   
  	
    ApsCommuTestRequest(pCommuTestRequest_t);
    zc_free_mem(pCommuTestRequest_t);
    
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);    
}
#endif

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f1_set_master_addr(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f1_set_master_addr(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //if(memcmp(FlashInfo_t.ucJZQMacAddr, pGw3762Data_t->DataUnit, MAC_ADDR_LEN))
    {
        __memcpy(FlashInfo_t.ucJZQMacAddr, pGw3762Data_t->DataUnit, MAC_ADDR_LEN);
        SetMacAddrRequest(FlashInfo_t.ucJZQMacAddr, e_JZQ,0);  // FlashInfo_t.ucJZQType

        /* if(TMR_RUNNING == zc_timer_query(g_CheckFromation))
        {
           StartFormationNet();
           timer_stop(g_CheckFromation,TMR_NULL);
        }*/

		sysflag=TRUE;
        //mutex_post(&mutexSch_t);
        if(PROVINCE_HEBEI == app_ext_info.province_code)//todo: PROVINCE_HEBEI
        {
            MASTERADDFLAG = TRUE;//记录主节点设置标志
        }
    }
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f2_set_slave_report(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f2_set_slave_report(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //FlashInfo_t.ucEventReportFlag = *(pGw3762Data_t->DataUnit);
    U8 BCAddr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8 ccomac[6];
	net_nnib_ioctl(NET_GET_CCOADDR,ccomac);

	if(*(pGw3762Data_t->DataUnit) == e_3762EVENT_REPORT_FORBIDEN
        ||*(pGw3762Data_t->DataUnit) == e_3762EVENT_REPORT_ALLOWED
        ||*(pGw3762Data_t->DataUnit) == e_3762EVENT_REPORT_BUFF_FULL)
	{
		g_EventReportStatus =  *(pGw3762Data_t->DataUnit);
	}
	else
	{		
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);		
		return;
	}
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
    if(g_EventReportStatus == e_3762EVENT_REPORT_BUFF_FULL)	//缓存区满不往载波发
	{
		return;
	}
    EVENT_REPORT_REQ_t *pEventReportRequest_t = NULL;
	pEventReportRequest_t= (EVENT_REPORT_REQ_t*)zc_malloc_mem(sizeof(EVENT_REPORT_REQ_t),"EventReportRequest",MEM_TIME_OUT);

    pEventReportRequest_t->Direction = e_DOWNLINK;
    pEventReportRequest_t->PrmFlag   = 1;
    
    if(g_EventReportStatus == e_3762EVENT_REPORT_FORBIDEN)
    {
        pEventReportRequest_t->FunCode   = e_CCO_ISSUE_FORBID_STA_REPORT;

        if (PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
        {
            //清空CCO给集中器进行超差事件上报的缓存列表且关闭缓存定时器
            clear_clock_overproof_event_list();
            clear_clock_overproof_event_ctrl_list();

            //关闭超差事件上报查重定时器，且清空查重列表
		if(TMR_STOPPED != zc_timer_query(clock_overproof_event_delduplitimer))
            {
                timer_stop(clock_overproof_event_delduplitimer, TMR_NULL);
            }
        }
    }
    else if(g_EventReportStatus == e_3762EVENT_REPORT_ALLOWED)
    {
        pEventReportRequest_t->FunCode   = e_CCO_ISSUE_PERMIT_STA_REPORT;
    }
    __memcpy(pEventReportRequest_t->MeterAddr,BCAddr,6);
    __memcpy(pEventReportRequest_t->DstMacAddr,BCAddr,6);
    __memcpy(pEventReportRequest_t->SrcMacAddr,ccomac,6);
    pEventReportRequest_t->PacketSeq = ++ApsEventSendPacketSeq;
    pEventReportRequest_t->sendType =  e_FULL_BROADCAST_FREAM_NOACK;
	pEventReportRequest_t->EvenDataLen = 0;   
	ApsEventReportRequest(pEventReportRequest_t);
    zc_free_mem(pEventReportRequest_t);
}

static void bcd_time_cal_timer_cb(struct ostimer_s * ostimer, void * arg)
{
	static U8	num = 0;
	U8 *sendbuf;
	U16 sendBuflen = 0;
	SysDate_t sysDateTemp = {0};

	sendbuf = zc_malloc_mem(52, "BcdTime", MEM_TIME_OUT);
	
	GetBinTime(&sysDateTemp);

	if(num < 3)
	{
		U8	bcAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};

		bin_to_bcd((U8*)&sysDateTemp, (U8*)&sysDateTemp, sizeof(SysDate_t));
		Add0x33ForData((U8*)&sysDateTemp, sizeof(SysDate_t));
		Packet645Frame(sendbuf,(U8*)&sendBuflen,bcAddr,0x08, (U8*)&sysDateTemp,sizeof(SysDate_t));

		num++;
		app_printf("645 sendbuf->");
	}
	else if(num >= 3 && num < 6)	//698广播校时
	{
		U8  pTxApdu[16] = {0x07, 0x01};
		U8	logo[5] = {0x40, 0x00, 0x7f, 0x00, 0x1c};
		U8	AAAddr[1] = {0xaa};

		__memcpy(&pTxApdu[2], (U8*)&Bcpiid, sizeof(U8));
		__memcpy(&pTxApdu[3], logo, sizeof(logo));

		pTxApdu[8] = (sysDateTemp.Year + 0x7d0)>>8;
		pTxApdu[9] = (sysDateTemp.Year + 0x7d0)&0x00FF;
		pTxApdu[10] = sysDateTemp.Mon;
		pTxApdu[11] = sysDateTemp.Day;
		pTxApdu[12] = sysDateTemp.Hour;
		pTxApdu[13] = sysDateTemp.Min;
		pTxApdu[14] = sysDateTemp.Sec;

		pTxApdu[15] = 0;	//无时间标签	

		Packet698Frame(sendbuf, &sendBuflen, 0x43, 0x03, 0x01, AAAddr, pTxApdu, sizeof(pTxApdu), BcCusAddr);
		
		app_printf("698 sendbuf->");
		
		num++;
	}

	dump_buf(sendbuf, sendBuflen);
	time_calibrate_req_aps(sendbuf, sendBuflen);
	timer_modify(BcdTimeCaltimer,
            	100,
             	0,
             	TMR_TRIGGER ,//TMR_TRIGGER
             	bcd_time_cal_timer_cb,
                NULL,
                "bcd_time_cal_timer_cb",
                TRUE
            );
	timer_start(BcdTimeCaltimer);
	
	if(num >= 6)
	{
		num = 0;
		BcCusAddr = 0;
		memset((U8*)&Bcpiid, 0x00, sizeof(U8));
		timer_stop(BcdTimeCaltimer, TMR_NULL);
	}
	zc_free_mem(sendbuf);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f3_start_bc(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f3_start_bc(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    /* 过滤非法广播校时帧 */
    U8 ctrl_word = pGw3762Data_t->DataUnit[0];

    if((ctrl_word == 1 || ctrl_word == 2) && Check645Frame(pGw3762Data_t->DataUnit + 2,pGw3762Data_t->DataUnitLen - 2,NULL,NULL,NULL) == FALSE)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }
    else if(ctrl_word == 3 && Check698Frame(pGw3762Data_t->DataUnit + 2,pGw3762Data_t->DataUnitLen - 2,NULL,NULL,NULL) == FALSE)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }


    if (PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
    {
        if(FALSE == validate_and_extract_broadcast_time(pGw3762Data_t->DataUnit + 2, pGw3762Data_t->DataUnitLen - 2, &SysDate))
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
        SysTemTimeSet(&SysDate);

	    if(NULL == BcdTimeCaltimer)
        {
            //广播校时启动定时器
            BcdTimeCaltimer = timer_create(1,
                0,
	                TMR_TRIGGER ,//TMR_TRIGGER
                bcd_time_cal_timer_cb,
                NULL,
                "bcd_time_cal_timer_cb"
            );
        }
	    if(TMR_STOPPED == zc_timer_query(BcdTimeCaltimer))
        {
            timer_start(BcdTimeCaltimer);

            //每次CCO开始下发广播校时05F3时关闭超差事件重复上报定时器,并清空列表
            app_printf("timer_stop(clock_overproof_event_delduplitimer, TMR_CALLBACK)!\n");
		
		    if(TMR_STOPPED != zc_timer_query(clock_overproof_event_delduplitimer))
            {
                timer_stop(clock_overproof_event_delduplitimer, TMR_NULL);
            }

            //每次CCO开始下发广播校时05F3时关闭CCO给集中器上报的超差事件定时器，并清空上报缓存区
            clear_clock_overproof_event_ctrl_list();
            clear_clock_overproof_event_list();
        }
	
        TimerCreate(200);//1s
    }
    else
    {
        time_calibrate_req_aps(pGw3762Data_t->DataUnit + 2, pGw3762Data_t->DataUnitLen - 2);
        TimerCreate(1000);//1s
    }
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f4_set_slave_max_timeout(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F4处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f4_set_slave_max_timeout(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    FlashInfo_t.ucSlaveReadTimeout = *(pGw3762Data_t->DataUnit);
    sysflag = TRUE;
    mutex_post(&mutexSch_t);
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f5_set_rf_param(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F5处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f5_set_rf_param(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    if (0)
    {
        app_printf("app_gw3762_afn05_f5_set_rf_param!");
        app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
        return;
    }

    if (pGw3762Data_t->DataUnitLen != 2)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port);
        return;
    }
    U8 cha_group = pGw3762Data_t->DataUnit[0];
    U8 tp_power = pGw3762Data_t->DataUnit[1];
    if (cha_group > 63 && cha_group != 0xfe && cha_group != 0xff)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }
    if (tp_power > e_LOWEST_POWER)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }
    if (cha_group <= 63)
    {
        rf_channel_group = cha_group;
    }
    rf_power = tp_power;
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 0, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f10_set_baudrate(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F10处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f10_set_baudrate(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn05_f10_set_baudrate!");

    if(pGw3762Data_t->DataUnit[0] == APP_GW3762_9600_BPS)
    {
        RunningInfo.BaudRate = BAUDRATE_9600;
    }
    else if(pGw3762Data_t->DataUnit[0] == APP_GW3762_19200_BPS)
    {
        RunningInfo.BaudRate = BAUDRATE_19200;
    }
    else if(pGw3762Data_t->DataUnit[0] == APP_GW3762_38400_BPS)
    {
        RunningInfo.BaudRate = BAUDRATE_38400;
    }
    else if(pGw3762Data_t->DataUnit[0] == APP_GW3762_57600_BPS)
    {
        RunningInfo.BaudRate = BAUDRATE_57600;
    }
    else if(pGw3762Data_t->DataUnit[0] == APP_GW3762_115200_BPS)
    {
        RunningInfo.BaudRate = BAUDRATE_115200;
    }
    else
    {
        RunningInfo.BaudRate = BAUDRATE_9600;
    }
    
    Modify_ChangeBaudtimer(&RunningInfo.BaudRate);
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);    
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f100_set_rssi_threshold(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F100处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f100_set_rssi_threshold(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn05_f100_set_rssi_threshold!");
    app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f10_set_master_time(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F101处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f101_set_master_time(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn05_f101_set_master_time!");

	SysDate_t date = {0};
    
	__memcpy((U8*)&date, pGw3762Data_t->DataUnit, sizeof(SysDate_t));
    SysTemTimeSet(&date);

    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}
#endif

/*************************************************************************
*																		*
*				         台区时钟精准治理									*
*																		*
*************************************************************************/
static int32_t query_sw_over_value_entry_list_check(multi_trans_header_t *list, multi_trans_t *new_list)
{       
	list_head_t *pos,*node;
	multi_trans_t  *mbuf_n;
    
    if(list->nr >= list->thr)
    {
		return -1;
	}
    app_printf("Entry check.\n");
    dump_buf(new_list->CnmAddr, 6);                    
    /*Check for duplicate meter address*/
    list_for_each_safe(pos, node,&list->link)
    {
        mbuf_n = list_entry(pos, multi_trans_t, link);
        
        if(memcmp(mbuf_n->Addr, new_list->Addr, 6) == 0)
        {
            return -2;
		}
    }

    return 0;
}

static void query_sw_over_value_entry_list_fail(int32_t err_code, void *pTaskPrmt)
{
    app_printf("Entry ModuleReq fail, err = %d\n", err_code);
    
    //RegisterInfo_t.CrnSlaveIndex++;
    if(err_code == -2)
    {
        //RegisterInfo_t.Timeout = 1000;
        //modify_slave_register_timer(RegisterInfo_t.Timeout);
    }
    else if(err_code == -1)
    {
        //modify_slave_register_timer(0);
    }
	
    return;
}

static void set_clk_sw_req(U16 moduleIndex,U8 FuncState,U8 *pMacAddr,MESG_PORT_e port)
{
	multi_trans_t SetClkSwORValue;

    SetClkSwORValue.lid = READ_METER_LID;
    SetClkSwORValue.SrcTEI = 0;
    SetClkSwORValue.DeviceTimeout = DEVTIMEOUT;
    SetClkSwORValue.MsgPort = port;
    	
    __memcpy(SetClkSwORValue.CnmAddr, pMacAddr, MAC_ADDR_LEN);

    SetClkSwORValue.State = UNEXECUTED;
        
    SetClkSwORValue.SendType = CLOCK_INFO_QUERY;
    SetClkSwORValue.StaIndex = moduleIndex;     
    SetClkSwORValue.DatagramSeq = ++QuerySwORValueSeq;
	SetClkSwORValue.DealySecond = SETSWORVALUE_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    SetClkSwORValue.ReTransmitCnt = 1;
    	
    SetClkSwORValue.DltNum = 1;
    SetClkSwORValue.ProtoType = 0;
    SetClkSwORValue.FrameLen  = 1;  
            
    SetClkSwORValue.EntryListCheck = query_sw_over_value_entry_list_check;
    SetClkSwORValue.EntryListfail = query_sw_over_value_entry_list_fail;
    SetClkSwORValue.TaskPro = cco_set_clk_sw; 

    SetClkSwORValue.TaskUpPro = NULL;    //module_id_info_proc;
    SetClkSwORValue.TimeoutPro = cco_query_sw_or_value_timeout;

    SetClkSwORValue.pMultiTransHeader = &SETSWORVALUE_LIST;
    SetClkSwORValue.CRT_timer = set_sworvalue_timer;
    multi_trans_put_list(SetClkSwORValue,&FuncState);
    
    return;
}

static void set_clk_over_value_req(U16 moduleIndex,U8 pValue,U8 *pMacAddr,MESG_PORT_e port)
{
	multi_trans_t SetClkSwORValue;

    SetClkSwORValue.lid = READ_METER_LID;
    SetClkSwORValue.SrcTEI = 0;
    SetClkSwORValue.DeviceTimeout = DEVTIMEOUT;
    SetClkSwORValue.MsgPort = port;
    	
    __memcpy(SetClkSwORValue.CnmAddr, pMacAddr, MAC_ADDR_LEN);

    SetClkSwORValue.State = UNEXECUTED;
        
    SetClkSwORValue.SendType = CLOCK_INFO_QUERY;
    SetClkSwORValue.StaIndex = moduleIndex;     
    SetClkSwORValue.DatagramSeq = ++QuerySwORValueSeq;
	SetClkSwORValue.DealySecond = SETSWORVALUE_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    SetClkSwORValue.ReTransmitCnt = 1;
    	
    SetClkSwORValue.DltNum = 1;
    SetClkSwORValue.ProtoType = 0;
    SetClkSwORValue.FrameLen  = 1;  
            
    SetClkSwORValue.EntryListCheck = query_sw_over_value_entry_list_check;
    SetClkSwORValue.EntryListfail = query_sw_over_value_entry_list_fail;
    SetClkSwORValue.TaskPro = cco_set_over_time_report_value; 

    SetClkSwORValue.TaskUpPro = NULL;    //ModuleIdInfoProc;
    SetClkSwORValue.TimeoutPro = cco_query_sw_or_value_timeout;

    SetClkSwORValue.pMultiTransHeader = &SETSWORVALUE_LIST;
    SetClkSwORValue.CRT_timer = set_sworvalue_timer;
    multi_trans_put_list(SetClkSwORValue,&pValue);

    return;
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn05_f90_set_clk_sw(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN05F90设置校时开关的处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn05_f90_set_clk_sw(APPGW3762DATA_t *pGw3762Data_t,MESG_PORT_e port)
{
	app_printf("----app_gw3762_afn05_f90_set_clk_sw---\n");
	dump_buf(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);
	
	U16	ii;
	U8 index = 0 ;
	U8 state = 0;
    U8 stateflag = 0;
	U8 NUllAddr[6] = {0};
	U8 MacAddr[6]  = {0};
	U8 BcdAddr[6] = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99};
    U8 FfAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	U8 *data = pGw3762Data_t->DataUnit;

	__memcpy(MacAddr,data+index,MACADRDR_LENGTH);
	index += MACADRDR_LENGTH;
    stateflag = *(data+index);
	index++;
	state = *(data+index);	
	ChangeMacAddrOrder(MacAddr);

	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
        if(0x01 != stateflag)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }
    if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
        if((0 == memcmp(BcdAddr, pGw3762Data_t->DataUnit, MACADRDR_LENGTH)) || (0 == memcmp(FfAddr, pGw3762Data_t->DataUnit, MACADRDR_LENGTH)))
        {
            set_clk_sw_req(0,state,MacAddr,port);
            app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
        }
        else
        {
            for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
            {
                if(memcmp(WhiteMacAddrList[ii].MacAddr, pGw3762Data_t->DataUnit, MAC_ADDR_LEN) == 0)
                {
                    if(memcmp(WhiteMacAddrList[ii].CnmAddr, NUllAddr, MAC_ADDR_LEN) == 0)
                    {
                        app_gw3762_up_afn00_f2_deny(APP_GW3762_NONWK_ERRCODE, port);
                        return;
                    }

                    break;
                }
            }

            if(ii == MAX_WHITELIST_NUM)
            {
                app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
                return;
            }
            set_clk_sw_req(0,state,MacAddr,port);
            app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
        }
    }
    else
    {
        if(0 == memcmp(BcdAddr, pGw3762Data_t->DataUnit, MACADRDR_LENGTH))
        {
            set_clk_sw_req(0,state,MacAddr,port);
            app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
        }
        else
        {
            for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
            {
                if(memcmp(WhiteMacAddrList[ii].MacAddr, pGw3762Data_t->DataUnit, MAC_ADDR_LEN) == 0)
                {
                    if(memcmp(WhiteMacAddrList[ii].CnmAddr, NUllAddr, MAC_ADDR_LEN) == 0)
                    {
                        app_gw3762_up_afn00_f2_deny(APP_GW3762_NONWK_ERRCODE, port);
                        return;
                    }

                    break;
                }
            }

            if(ii == MAX_WHITELIST_NUM)
            {
                app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
                return;
            }
            set_clk_sw_req(0,state,MacAddr,port);
            app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
        }
    }
}

static void app_gw3762_afn05_f92_exact_time_calibrate(APPGW3762DATA_t *pGw3762Data_t,MESG_PORT_e port)
{
	app_printf("----AppGw3762Afn05F92TimeCalibrate---\n");
	dump_buf(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);

	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    exact_timer_calibrate_req(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);
	
}

static void app_gw3762_afn05_f93_set_clock_maintain(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{	
    U16 cycle =0;
    cycle  = split_read_two(pGw3762Data_t->DataUnit,0);
	// clock_maintenance_cycle = pGw3762Data_t->DataUnit[0] + (pGw3762Data_t->DataUnit[1] << 8);
	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    if(cycle == 0)
    {
        app_printf("clock_maintenance_cycle is 0\n");
        return;
    }
    clock_maintenance_cycle = cycle;
	sysflag=TRUE;
	
	//下发时钟维护的代理广播
    if(PROVINCE_HUNAN!=app_ext_info.province_code && PROVINCE_SHANNXI!=app_ext_info.province_code)
	{
        timer_start(clockMaintaintimer);
        timer_stop(clockMaintaintimer, TMR_CALLBACK);
    }
}

static void app_gw3762_afn05_f94_set_over_time_report_value(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("Afn05F94SetOverTimeReportValue---------\n");
	U8	meterAddr[6];
	U8	NullAddr[6];
	U8	BcdAddr[6];
    U8  FfAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	U8	index, value, findMeterFlag;
	U16	ii;
	U8*	dataUnit = (U8*)&pGw3762Data_t->DataUnit;

	index = value = ii = findMeterFlag = 0;
	memset(meterAddr, 0x00, MAC_ADDR_LEN);
	memset(NullAddr, 0x00, MAC_ADDR_LEN);
	memset(BcdAddr, 0x99, MAC_ADDR_LEN);
	
	__memcpy(meterAddr, dataUnit, sizeof(meterAddr));
	index += sizeof(meterAddr);
	//ChangeMacAddrOrder(meterAddr);
	for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        if(memcmp(WhiteMacAddrList[ii].MacAddr, meterAddr, MAC_ADDR_LEN) == 0)
        {
            __memcpy(meterAddr, WhiteMacAddrList[ii].CnmAddr, MAC_ADDR_LEN);
			app_printf("meterAddr-> ");
			dump_buf(meterAddr, MAC_ADDR_LEN);
            findMeterFlag = 1;
			break;
        }
    }
	if(PROVINCE_HEILONGJIANG==app_ext_info.province_code)
	{
        if((0 == memcmp(meterAddr, BcdAddr, MAC_ADDR_LEN)) || (0 == memcmp(meterAddr, FfAddr, MAC_ADDR_LEN)))
        {
            findMeterFlag = 1;
        }
    }
    else
    {
        if(0 == memcmp(meterAddr, BcdAddr, MAC_ADDR_LEN))
        {
            findMeterFlag = 1;
        }
    }
	if(0 == findMeterFlag)
	{
		app_printf("APP_GW3762_ADDRNONE_ERRCODE!\n");
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
        return;
	}

	if(0 == memcmp(meterAddr, NullAddr, MAC_ADDR_LEN))
	{
		app_printf("APP_GW3762_NONWK_ERRCODE!\n");
		app_gw3762_up_afn00_f2_deny(APP_GW3762_NONWK_ERRCODE, port);
        return;
	}

	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	
	value = *(dataUnit+index);
	set_clk_over_value_req(0,value,meterAddr,port);
}

static void app_gw3762_up_afn10_f90_query_clk_over_value(U8* Addr,U8 addrlen,U8 *data,U8 len,MESG_PORT_e port)
{
	//app_printf("AppGw3762Afn10F90QueryClkSwAndOverValue!\n");
	U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
  
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN10;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F90);
    Gw3762updata.DataUnitLen			= len;
    dump_buf(data,len);
	__memcpy(Gw3762updata.DataUnit,data,len);
	
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,AppGw3762DownData.DownInfoField.FrameNum,FALSE,0,port,e_Serial_AFN10XX);
}

static void app_up_query_clk_over_value_proc(void *pTaskPrmt, void *pUplinkData)
{
	QUERY_CLKSWOROVER_VALUE_CFM_t *pQueryClkORValueCfm = (QUERY_CLKSWOROVER_VALUE_CFM_t *)pUplinkData;
	multi_trans_t *pTask = pTaskPrmt;
	
	U8 Res = 0;
	U8  byLen = 0;
	U8 UpData[9] = {0};
	app_printf("QueryClkORValueCfm->AutoSwState = %d\n",pQueryClkORValueCfm->AutoSwState);
	app_printf("QueryClkORValueCfm->Time_Over_Value = %d\n",pQueryClkORValueCfm->Time_Over_Value);
	__memcpy(UpData,pQueryClkORValueCfm->Macaddr,MACADRDR_LENGTH);
	byLen = MACADRDR_LENGTH;
	UpData[byLen++] = pQueryClkORValueCfm->AutoSwState;
	UpData[byLen++] = Res;
	UpData[byLen++] = pQueryClkORValueCfm->Time_Over_Value;

	app_gw3762_up_afn10_f90_query_clk_over_value(pQueryClkORValueCfm->Macaddr,6,UpData,sizeof(UpData),pTask->MsgPort);
}

static void query_clk_sw_and_over_value_req(U16 moduleIndex, U8 *pMacAddr,MESG_PORT_e port)
{
    multi_trans_t QueryClkSwORValue;

    QueryClkSwORValue.lid = READ_METER_LID;
    QueryClkSwORValue.SrcTEI = 0;
    QueryClkSwORValue.DeviceTimeout = DEVTIMEOUT;
    QueryClkSwORValue.MsgPort = port;
    	
    __memcpy(QueryClkSwORValue.CnmAddr, pMacAddr, MAC_ADDR_LEN);

    QueryClkSwORValue.State = UNEXECUTED;
        
    QueryClkSwORValue.SendType = CLOCK_INFO_QUERY;
    //QueryClkSwORValue.StaIndex = moduleIndex;     
    QueryClkSwORValue.DatagramSeq = ++QuerySwORValueSeq;
	QueryClkSwORValue.DealySecond = QUERYSWORVALUE_LIST.nr == 0?0:MULTITRANS_INTERVAL;
    QueryClkSwORValue.ReTransmitCnt = RETRANMITMAXNUM;
    	
    QueryClkSwORValue.DltNum = 1;
    QueryClkSwORValue.ProtoType = 0;
    QueryClkSwORValue.FrameLen  = 6;  
            
    QueryClkSwORValue.EntryListCheck = query_sw_over_value_entry_list_check;
    QueryClkSwORValue.EntryListfail = query_sw_over_value_entry_list_fail;
    QueryClkSwORValue.TaskPro = cco_query_clk_Sw_o_over_value; 

    QueryClkSwORValue.TaskUpPro = app_up_query_clk_over_value_proc;    //ModuleIdInfoProc;
    QueryClkSwORValue.TimeoutPro = cco_query_sw_or_value_timeout;

    QueryClkSwORValue.pMultiTransHeader = &QUERYSWORVALUE_LIST;
    QueryClkSwORValue.CRT_timer = query_sworvalue_timer;
    multi_trans_put_list(QueryClkSwORValue,pMacAddr);
    return;
}

static void app_gw3762_afn10_f90_query_clk_sw_and_over_valude(APPGW3762DATA_t *pGw3762Data_t,MESG_PORT_e port)
{
	U8 macAddr[6] = {0};
	app_printf("----app_gw3762_afn10_f90_query_clk_sw_and_over_valude---\n");
	dump_buf(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);
	__memcpy(macAddr,pGw3762Data_t->DataUnit,6);

	ChangeMacAddrOrder(macAddr);
	query_clk_sw_and_over_value_req(0,macAddr,port);
}

static void app_gw3762_afn10_f93_query_clokc_maintain_cycle(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	U16 Gw3762SendDataLen = 0;

    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		    = 0;
    Gw3762updata.Afn				    = APP_GW3762_AFN10;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F93);
    Gw3762updata.DataUnitLen			= 2;
	Gw3762updata.DataUnit[0] = (U8)(clock_maintenance_cycle & 0xff);
	Gw3762updata.DataUnit[1] = (U8)((clock_maintenance_cycle >> 8) & 0xff);
    
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,AppGw3762DownData.DownInfoField.FrameNum,FALSE,0,port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f1_query_slave_num(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f1_query_slave_num(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN10;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);
    Gw3762updata.DataUnitLen			= 4;
//    *((U16 *)(Gw3762updata.DataUnit))	= FlashInfo_t.usJZQRecordNum;
//    *((U16 *)(Gw3762updata.DataUnit + 2)) = (U16)APP_GW3762_METER_MAX;
	Gw3762updata.DataUnit[0] = (U8)(FlashInfo_t.usJZQRecordNum & 0xFF);    
	Gw3762updata.DataUnit[1] = (U8)(FlashInfo_t.usJZQRecordNum >> 8);	
	Gw3762updata.DataUnit[2] = (U8)(APP_GW3762_METER_MAX & 0xFF);	 
	Gw3762updata.DataUnit[3] = (U8)(APP_GW3762_METER_MAX >> 8);

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f2_query_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f2_query_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  default_addr[6]={0,0,0,0,0,0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    U16  jj = 0;
	DEVICE_TEI_LIST_t DeviceListTemp;
	static U8  start_1_flag=1;
	U8  *pnum=NULL;
	METER_INFO_t MeterInfo_t;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);
	area_change_report_wait_timer_refresh();
    //CreatreportWaittimer(60*1000);

	if(ucReadNum<0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
    
	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	
	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F2);
    Gw3762updata.DataUnitLen			= 2 + 1;
    pGW3762updataunit[0]	     = (U8)(FlashInfo_t.usJZQRecordNum & 0xFF);
    pGW3762updataunit[1]          = (U8)(FlashInfo_t.usJZQRecordNum >> 8);
    pGW3762updataunit[2]  	      = 0;//ucReadNum;
    pnum = &pGW3762updataunit[2];
    pGW3762updataunit             += 3;
	if(ucReadNum == 0)//查询数量为0
	{
		goto send;
	}
    WHLPTST //共享存储内容保护
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		if(0==memcmp(WhiteMacAddrList[jj].MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		
		if(ii == usStartIdx)//当前为读取的起始位置
		{
			U8 Phase;
			__memcpy(pGW3762updataunit, WhiteMacAddrList[jj].MacAddr, 6); //6字节地址+两字节信息
			//相位转换2bit转换为3bit
			__memcpy(&MeterInfo_t,&WhiteMacAddrList[jj].MeterInfo_t,sizeof(METER_INFO_t));
			MeterInfo_t.ucPhase = 0;
			if(DeviceList_ioctl(DEV_GET_ALL_BYMAC,WhiteMacAddrList[jj].MacAddr, &DeviceListTemp))
			{
				//app_printf("[%d]",DeviceListTemp.DeviceType);
				//dump_buf(WhiteMacAddrList[jj].MacAddr,MACADRDR_LENGTH);
				
				if(DeviceListTemp.DeviceType == e_3PMETER)
				{
					MeterInfo_t.ucPhase = (DeviceListTemp.F31_D0 | (DeviceListTemp.F31_D1 << 1) | (DeviceListTemp.F31_D2 << 2));
                    app_printf("ucphase : %d\n",MeterInfo_t.ucPhase);
				}
				else
				{
					Phase = DeviceListTemp.Phase;
					MeterInfo_t.ucPhase = 1<<(Phase-1);
				}
			}

			
			//MeterInfo_t.LNerr	= DeviceListTemp.LNerr;

			__memcpy(pGW3762updataunit+6, &MeterInfo_t, 2);
	        pGW3762updataunit += 8;
	        Gw3762updata.DataUnitLen	+= 8;	
			*pnum += 1;

			
				
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
    WHLPTED//释放共享存储内容保护
    
	if(*pnum == 0)
	{
		g_QueryLastRecordTimes++;
		if(g_QueryLastRecordTimes >= 2)
		{
			g_QueryLastRecordTimes = 0;
			goto send;
		}
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
	send:
	app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	U16 get_father_node_info(U8 *pMacAddr,U8 *pNodeDepth)
 * 函数说明	: 	get_father_node_info处理函数
 * 参数说明	: 	U8 *pMacAddr,U8 *pNodeDepth
 * 返回值		: 	档案位置
 *************************************************************************/
static U16 get_father_node_info(U8 *pMacAddr, U8 *pNodeDepth)
{
    U16 ii = 0;
    U16  jj = 0;
    U8 MacAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
    
        if(0 != memcmp(DeviceTEIList[ii].MacAddr, pMacAddr, 6))
        {
            continue;
        }
        if(DeviceTEIList[ii].ParentTEI > MAX_WHITELIST_NUM || DeviceTEIList[ii].ParentTEI < 2)
        {
            return MAX_WHITELIST_NUM;//不合法
        }
        if(0 == memcmp(DeviceTEIList[DeviceTEIList[ii].ParentTEI - 2].MacAddr, MacAddr, 6))
        {
            return MAX_WHITELIST_NUM;//不可用
        }
        for(jj = 0; jj < MAX_WHITELIST_NUM; jj++)
        {
            if(0 != memcmp(DeviceTEIList[DeviceTEIList[ii].ParentTEI - 2].MacAddr, WhiteMacAddrList[jj].MacAddr, 6))
            {
                continue;
            }
            *pNodeDepth = DeviceTEIList[DeviceTEIList[ii].ParentTEI - 2].NodeDepth;
            return jj;
        }
        return MAX_WHITELIST_NUM;//在档案中未查到
    }
    return MAX_WHITELIST_NUM;//在白名单中未查到
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f3_query_slave_relay(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f3_query_slave_relay(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pDataUnit = Gw3762updata.DataUnit;
    U16 pos = 0;
    U8  NodeDepth = 0xf;
    U8  ii = 0;
	METER_INFO_t MeterInfo_t;
	U8 phase = 0;
	DEVICE_TEI_LIST_t DeviceListTemp;
	
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F3);
    Gw3762updata.DataUnitLen			= 1;
    *pDataUnit++           			= 0;

    WHLPTST //共享存储内容保护
    do
    {
        ii++;
        pos = get_father_node_info(pGw3762Data_t->DataUnit, &NodeDepth);
        if(MAX_WHITELIST_NUM == pos)
        {
            break;
        }
        __memcpy(pDataUnit, WhiteMacAddrList[pos].MacAddr, 6);
		//phase=WhiteMacAddrList[pos].MeterInfo_t.ucPhase;

		//相位转换2bit转换为3bit
		__memcpy(&MeterInfo_t,&WhiteMacAddrList[pos].MeterInfo_t,sizeof(METER_INFO_t));
		DeviceList_ioctl(DEV_GET_ALL_BYMAC,WhiteMacAddrList[pos].MacAddr, &DeviceListTemp);
		phase = DeviceListTemp.Phase;
		if(phase)
		{
			MeterInfo_t.ucPhase = 1<<(phase-1);
		}
		else
		{
			MeterInfo_t.ucPhase =0;
		}
		
		//MeterInfo_t.LNerr	= DeviceListTemp.LNerr;
		__memcpy(pDataUnit+6, &MeterInfo_t, 2);
		
        pDataUnit += 8;
        Gw3762updata.DataUnitLen	+= 8;
        Gw3762updata.DataUnit[0]++;

    }
    while(NodeDepth > 1 && MAX_WHITELIST_NUM != pos && ii < 0xf);
    WHLPTED//释放共享存储内容保护
    
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f4_router_state(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F4处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f4_router_state(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);

    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN10;
    Gw3762updata.Fn                                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F4);
	if(nnib.Networkflag == TRUE)
	{
		AppGw3762Afn10F4State.RunStatusRouter =  TRUE;
	}
	else
	{
		AppGw3762Afn10F4State.RunStatusRouter = FALSE;
	}
	Gw3762updata.DataUnitLen			= sizeof(AppGw3762Afn10F4State);
	AppGw3762Afn10F4State.WorkStep[0]=8;	
	AppGw3762Afn10F4State.WorkStep[1]=8;	
	AppGw3762Afn10F4State.WorkStep[2]=8;
	AppGw3762Afn10F4State.SlaveNodeCount = FlashInfo_t.usJZQRecordNum;

    __memcpy(Gw3762updata.DataUnit, (U8*)&AppGw3762Afn10F4State, Gw3762updata.DataUnitLen);

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f5_fail_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F5处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f5_fail_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
	memset(&Gw3762updata , 0x00 , sizeof(APPGW3762DATA_t));
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	//U8  default_addr[6]={0,0,0,0,0,0};
    U16	usStartIdx = 0;
    //U16  ii = 0;
    //U16  jj = 0;
	static U8  start_1_flag=1;
	//U8  *pnum=NULL;
	
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	
	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F5);
    Gw3762updata.DataUnitLen			= 2 + 1;
    pGW3762updataunit[0]	     = (U8)(FlashInfo_t.usJZQRecordNum & 0xFF);
    pGW3762updataunit[1]          = (U8)(FlashInfo_t.usJZQRecordNum >> 8);
    pGW3762updataunit[2]  	      = 0;//ucReadNum;
    //pnum = &pGW3762updataunit[2];
    pGW3762updataunit             += 3;

    /*	
    WHLPTST //共享存储内容保护
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		if(0==memcmp(WhiteMacAddrList[jj].MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		
		if(ii == usStartIdx)//当前为读取的起始位置
		{
			if((WhiteMacAddrList[jj].Result == 0) && (jj != 0))
			{
				__memcpy(pGW3762updataunit, WhiteMacAddrList[jj].MacAddr, 8); //6字节地址+两字节信息
		        pGW3762updataunit += 8;
		        Gw3762updata.DataUnitLen	+= 8;	
				*pnum += 1;
				app_printf("report num : %d\n",*pnum);
				if(*pnum == ucReadNum)
				{
					WHLPTED//释放共享存储内容保护
					goto send;
				}
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
    WHLPTED//释放共享存储内容保护
    
	if(*pnum == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}

    
send:
    */
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
    //app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f6_reg_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F6处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f6_reg_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    memset(&Gw3762updata , 0x00 , sizeof(APPGW3762DATA_t));
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
    U8  device_addr[6]={0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    //U16  jj = 0;
    static U8  start_1_flag=1;
    U8  *pnum=NULL;
    METER_INFO_t   mterInfo;
	U16 seq;
    DEVICE_TEI_LIST_t DeviceListTemp;
	
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	
	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F6);
    Gw3762updata.DataUnitLen			= 2 + 1;
    pGW3762updataunit[0]	     = (U8)((APP_GETDEVNUM()) & 0xFF);
    pGW3762updataunit[1]          = (U8)((APP_GETDEVNUM()) >> 8);
    pGW3762updataunit[2]  	      = 0;//ucReadNum;
    pnum = &pGW3762updataunit[2];
    pGW3762updataunit             += 3;

    for(ii = 0; ii < ucReadNum; ii++)
    {
		seq = usStartIdx + ii;
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&seq, &DeviceListTemp);//Get_DeviceList_All( (usStartIdx + ii) ,  &DeviceListTemp);
        if((usStartIdx + ii) >= APP_GETDEVNUM())
        {
            break;
        }
        __memcpy(device_addr,DeviceListTemp.MacAddr,6);	
		ChangeMacAddrOrder(device_addr);
		__memcpy(pGW3762updataunit, device_addr, 6); //6字节地址+两字节信息
        
        pGW3762updataunit += 6;
        memset(&mterInfo, 0x00, sizeof(mterInfo));
        mterInfo.ucRelLevel = DeviceListTemp.NodeDepth; 
        mterInfo.ucPhase = DeviceListTemp.Phase;
        mterInfo.ucGUIYUE = 0x02;
        __memcpy(pGW3762updataunit, &mterInfo, sizeof(mterInfo));
        pGW3762updataunit += sizeof(mterInfo);
        Gw3762updata.DataUnitLen	+= 8;

        *pnum += 1;
    }

    /*	
    WHLPTST //共享存储内容保护
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		if(0==memcmp(WhiteMacAddrList[jj].MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		
		if(ii == usStartIdx)//当前为读取的起始位置
		{
			if(-1 != SeachSeqBySeq(jj)) //此处保存有所上报节点的下标
			{
				__memcpy(pGW3762updataunit, WhiteMacAddrList[jj].MacAddr, 8); //6字节地址+两字节信息
		        pGW3762updataunit += 8;
		        Gw3762updata.DataUnitLen	+= 8;	
				*pnum += 1;
				app_printf("report num : %d\n",*pnum);
				if(*pnum == ucReadNum)
				{
					WHLPTED//释放共享存储内容保护
					goto send;
				}
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
    WHLPTED//释放共享存储内容保护
    
	if(*pnum == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
	send:
    */
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f100_query_net_scale(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F100处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f100_query_net_scale(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
      //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN10;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F100);
    Gw3762updata.DataUnitLen			= 2;
//    *((U16 *)(Gw3762updata.DataUnit))	= FlashInfo_t.usJZQRecordNum;
//    *((U16 *)(Gw3762updata.DataUnit + 2)) = (U16)APP_GW3762_METER_MAX;
	Gw3762updata.DataUnit[0] = 0;    
	Gw3762updata.DataUnit[1] = 2;	

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f101_query_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F101处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f101_query_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f101_query_slave_info!\n");
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8  device_addr[6]={0};
    U16	usStartIdx = 0 , staridex=0 ;
    U16  jj = 0;
	U8   pnum=0;
    DEVICE_TEI_LIST_t DeviceListTemp;
	
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	
	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F101);
    Gw3762updata.DataUnitLen			= 2 + 1;
    pGW3762updataunit[0]	     = (U8)((APP_GETDEVNUM()+1) & 0xFF);
    pGW3762updataunit[1]          = (U8)((APP_GETDEVNUM()+1) >> 8);
    pGW3762updataunit[2]  	      = 0;//ucReadNum;
    pGW3762updataunit             += 3;

	//起始序号为0，读集中器地址//北京 1
	if(usStartIdx==1)
	{
		__memcpy(pGW3762updataunit, FlashInfo_t.ucJZQMacAddr, 6); //6字节地址+三字节信息
		memset(pGW3762updataunit+6,0,2);
		pGW3762updataunit[7] = bootversion;
		__memcpy(pGW3762updataunit+9,FlashInfo_t.ManuFactor_t.ucVersionNum,2);

		pGW3762updataunit += 11;
        Gw3762updata.DataUnitLen	+= 11;
		pnum += 1;
		if(pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
		{
			goto send;
		}
		
	}
	/*
    WHLPTST //共享存储内容保护
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		if(0==memcmp(WhiteMacAddrList[jj].MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		
		if(ii == usStartIdx)//当前为读取的起始位置
		{
			__memcpy(pGW3762updataunit, WhiteMacAddrList[jj].MacAddr, 6); //6字节地址+三字节信息
			memset(pGW3762updataunit+6,0,2);
			//__memcpy(pGW3762updataunit+8,version,3);

			pGW3762updataunit += 11;
	        Gw3762updata.DataUnitLen	+= 11;
			*pnum += 1;

			app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
				
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
    WHLPTED//释放共享存储内容保护
	*/
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
    {   	
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
    	if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		else
		{
		if(usStartIdx > 0)
			{
				if(staridex == usStartIdx-1)
				{
					break;
				}				
			}
		else
			{
				staridex = usStartIdx;
				break;
			}
		staridex++;
		}
    }
	if(jj>=MAX_WHITELIST_NUM)
	{
		if(usStartIdx != 1 )
			{
			app_printf("staridex is over\n");
			//app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
			pnum = 0 ;
			goto send;
			}
	}
	else
	{
		for(jj=staridex;jj<MAX_WHITELIST_NUM;jj++)
		{		 	
			DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
			if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
			{
				continue;
			}
			__memcpy(device_addr,DeviceListTemp.MacAddr,6);	
			ChangeMacAddrOrder(device_addr);
			__memcpy(pGW3762updataunit, device_addr, 6); //6字节地址+两字节信息
			memset(pGW3762updataunit+6,0,2);
			pGW3762updataunit[8] = DeviceListTemp.BootVersion;
			__memcpy(pGW3762updataunit+9,DeviceListTemp.SoftVerNum,2);
			pGW3762updataunit += 11;
            Gw3762updata.DataUnitLen	+= 11;
			pnum++;
			if(pnum == ucReadNum|| Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{				
				goto send;
			}
		}
		if(pnum<ucReadNum)
			{
				//Gw3762updata.DataUnit[2]=pnum;
				goto send;
			}
	}
	Gw3762updata.DataUnit[2]=pnum;
	if(pnum == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
	send:
		Gw3762updata.DataUnit[2]=pnum;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn10_f104_query_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F104处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn10_f104_query_slave_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f104_query_slave_info!\n");
    U8	ucReadNum = 0;
    U16	usStartIdx = 0 ;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

    if(ucReadNum <= 0U)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
    if(ucReadNum > 15U)
    {
        ucReadNum = 15U;
    }
    
    if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
    {
        app_gw3762_up_afn10_fn_by_whitelist_up_frame(APP_GW3762_F104, (U8)pGw3762Data_t->CtrlField.TransMode, usStartIdx, ucReadNum, port);
    }
    else
    {
        app_gw3762_up_afn10_fn_up_frame(APP_GW3762_F104, (U8)pGw3762Data_t->CtrlField.TransMode, usStartIdx, ucReadNum, port);
    }
}
#endif

/*************************************************************************
 * 函数名称	: 	U8 app_gw3762_afn11_f1_add_slave_proc(U8* pucData, U8 ucDataLen, APP_JZQ_TYPE_E eJzqType,MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static U8 app_gw3762_afn11_f1_add_slave_proc(U8 *pucData, U8 ucDataLen, APP_JZQ_TYPE_E eJzqType,MESG_PORT_e port)
{
    U8	*pucAddr = pucData;
    U8	*pucGuiYue = NULL;
    U16	usIdx = 0;
    U8   DefaultAddr[6] = {0};
	U16 current_idx=0;
    DEVICE_TEI_LIST_t GetDeviceListTemp_t;
    
	for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
	{
	    if(0!=memcmp(DefaultAddr, WhiteMacAddrList[usIdx].MacAddr, 6))
	    {
	    	current_idx++;			
	    }
		if(FlashInfo_t.usJZQRecordNum == 0)
		{
			current_idx=0;
			break;
		}
		else
		{
	    	if(current_idx==FlashInfo_t.usJZQRecordNum)
			{
				current_idx = usIdx+1;
				//app_printf("-1-usIdx : %d\n",usIdx);
				break;
			}
		}
	}
    while(ucDataLen)
    {
        if(APP_JZQ_TYPE_09 == eJzqType)
        {
            pucGuiYue = pucAddr + 6 + 2;
        }
        else
        {
            pucGuiYue = pucAddr + 6;
        }

		if(!memcmp(pucAddr, DefaultAddr, 6))
		{
            goto next;
		}
		
        for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
        {
            if(!memcmp(pucAddr, WhiteMacAddrList[usIdx].MacAddr, 6))
                    //&& (WhiteMacAddrList[usIdx].MeterInfo_t.ucGUIYUE  == *pucGuiYue))
            {
                app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRREPEAT_ERRCODE, port);
                return FALSE;
                //goto next;
            }
        }

        if(FlashInfo_t.usJZQRecordNum >= MAX_WHITELIST_NUM||current_idx>=MAX_WHITELIST_NUM)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return FALSE;
        }
       // app_printf("Add Addr : current_idx : %d >  ", current_idx);
       // dump_buf(pucAddr, 6);
        WHLPTST
        __memcpy(WhiteMacAddrList[current_idx].MacAddr, pucAddr, 6);
        WhiteMacAddrList[current_idx].MeterInfo_t.ucGUIYUE = *pucGuiYue;
		memset(WhiteMacAddrList[current_idx].ModuleID, 0X00, sizeof(WhiteMacAddrList[current_idx].ModuleID));
        WhiteMacAddrList[current_idx].IDState = MOUDLTID_NO_UP;
		memset(WhiteMaCJQMapList[current_idx].CJQAddr,0x00,6);
        
        ChangeMacAddrOrder(pucAddr);        
        for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
        {            
			DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&usIdx, &GetDeviceListTemp_t);//Get_DeviceList_All(usIdx,&GetDeviceListTemp_t);
			
			if(memcmp(GetDeviceListTemp_t.MacAddr, pucAddr, 6) == 0)
            {
                __memcpy(WhiteMacAddrList[current_idx].CnmAddr, GetDeviceListTemp_t.MacAddr, 6);
				WhiteMacAddrList[current_idx].MeterInfo_t.ucPhase = GetDeviceListTemp_t.Phase;
				DeviceList_ioctl(DEV_APP_SET_WHITESEQ, &usIdx, &current_idx);
                break;
            }
        }
        //current_idx++;
        for(usIdx = current_idx+1; usIdx < MAX_WHITELIST_NUM; usIdx++)
    	{
    	    if(0==memcmp(DefaultAddr, WhiteMacAddrList[usIdx].MacAddr, 6))
    	    {
    	        //app_printf("-1-current_idx : %d\n",current_idx);
                current_idx = usIdx;
    	    	break;
    	    }
            else
            {
                //current_idx++;
            }
        }
        FlashInfo_t.usJZQRecordNum ++;
        WHLPTED
        next :
        if(APP_JZQ_TYPE_09 == eJzqType)
        {
            pucAddr = pucAddr + (6 + 2 + 1);
            ucDataLen  -= (6 + 2 + 1);
        }

        else
        {
            pucAddr = pucAddr + (6 + 1);
            ucDataLen  -= (6 + 1);
        }
    }
    app_printf("FlashInfo_t.usJZQRecordNum = %d current_idx : %d\n",FlashInfo_t.usJZQRecordNum,current_idx);
	
    return TRUE;
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f1_add_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void flash_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	sysflag=TRUE;
    whitelistflag = TRUE;
    app_printf("flash_timer_cb: 	sysflag=TRUE, whitelistflag = TRUE\n");
}

void flash_timer_start(void)
{
	if(flashtimer == NULL)
	{
        flashtimer = timer_create(8*1000,
	                0,
	                TMR_TRIGGER ,//TMR_TRIGGER
	                flash_timer_cb,
	                NULL,
	                "flash_timer_cb"
	               );
	}
	timer_start(flashtimer);	
}
static void app_gw3762_afn11_f1_add_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{

    U8 ucNum = 0;
	
    U16 usMeterIdx = 0;
	
    ucNum = (*(pGw3762Data_t->DataUnit));
	
	//whitelistflag = TRUE;
	//sysflag = TRUE;
    //档案同步过程，不进行台区改切上报
    //CreatreportWaittimer(60*1000);
    area_change_report_wait_timer_refresh();
    //拉长路由抄表流程
    router_read_refresh_timer();


    if(pGw3762Data_t->DataUnitLen == (1 + ucNum * (6 + 1)))
    {
        if(APP_JZQ_TYPE_09 == FlashInfo_t.ucJZQType)
        {
            FlashInfo_t.ucJZQType = APP_JZQ_TYPE_13;
            WHLPTST;
            for(usMeterIdx = 0; usMeterIdx < MAX_WHITELIST_NUM; usMeterIdx++)
            {           	
                memset(&WhiteMacAddrList[usMeterIdx], 0, sizeof(WHITE_MAC_LIST_t));
				FlashInfo_t.usJZQRecordNum=0;
				
				memset(&WhiteMaCJQMapList[usMeterIdx],0x00,6);
            }
            WHLPTED;
        }

        if(FALSE == app_gw3762_afn11_f1_add_slave_proc(pGw3762Data_t->DataUnit + 1, pGw3762Data_t->DataUnitLen - 1, APP_JZQ_TYPE_13,port))
        {
            return;
        }

		flash_timer_start();
		
		app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

        return;
    }
    else if(pGw3762Data_t->DataUnitLen == 1 + ucNum * (6 + 2 + 1))
    {
        if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
        {
            FlashInfo_t.ucJZQType = APP_JZQ_TYPE_09;
            WHLPTST;
            for(usMeterIdx = 0; usMeterIdx < MAX_WHITELIST_NUM; usMeterIdx++)
            {
                memset(&WhiteMacAddrList[usMeterIdx], 0, sizeof(WHITE_MAC_LIST_t));
				FlashInfo_t.usJZQRecordNum=0;
				memset(&WhiteMaCJQMapList[usMeterIdx],0x00,6);
            }
            WHLPTED;
        }
        if(FALSE == app_gw3762_afn11_f1_add_slave_proc(pGw3762Data_t->DataUnit + 1, pGw3762Data_t->DataUnitLen - 1, APP_JZQ_TYPE_09,port))
        {
            return;
        }
		
		flash_timer_start();
        app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5 , port);
        return;
    }
    else
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port);
        return;
    }
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f2_del_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f2_del_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8	*pucData  = NULL;
    U8	ucDataLen = 0;
    U8	ucDelNum = 0;
    U8	*pucAddr = NULL;
    U8   bResult = 0;
    U16  index = 0;
    U8 addr_0[6]={0,0,0,0,0,0};
    pucData = pGw3762Data_t->DataUnit;
    ucDelNum = pucData[0];

    if(pGw3762Data_t->DataUnitLen != 1 + 6 * (ucDelNum))
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port);
        return;
    }
    //拉长路由抄表流程
    router_read_refresh_timer();

    LEAVE_IND_t *pLeaveInd = NULL;
    pLeaveInd = zc_malloc_mem(MAX_NET_MMDATA_LEN, "DLSL", MEM_TIME_OUT);
    pLeaveInd->delayTime = 0;
    pLeaveInd->StaNum = 0;
    
    pucData++;
    ucDataLen = pGw3762Data_t->DataUnitLen - 1;
	//app_printf("ucDataLen : %d\n",ucDataLen);
	//dump_buf(pucData,ucDataLen);
    while(ucDataLen)
    {
        pucAddr = pucData;
        for(index = 0; index < MAX_WHITELIST_NUM; index++)
        {
			if(!memcmp(pucAddr, addr_0, 6))
			{
				app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
        		zc_free_mem(pLeaveInd);
                return;
			}
            if(!memcmp(pucAddr, WhiteMacAddrList[index].MacAddr, 6))
            {
                WHLPTST
                __memcpy(&pLeaveInd->pMac[6*pLeaveInd->StaNum], WhiteMacAddrList[index].MacAddr,6);
                ChangeMacAddrOrder(&pLeaveInd->pMac[6*pLeaveInd->StaNum]);
                pLeaveInd->StaNum++;

				dump_buf(WhiteMacAddrList[index].MacAddr,6);
                memset(WhiteMacAddrList[index].MacAddr, 0, sizeof(WHITE_MAC_LIST_t));
				//sysflag=TRUE;
                //whitelistflag = TRUE;
				if(FlashInfo_t.usJZQRecordNum>0)
				{
					FlashInfo_t.usJZQRecordNum -= 1;
				}
                bResult = 1;
				WHLPTED
				memset(WhiteMaCJQMapList[index].CJQAddr,0x00,6);
                break;
            }
        }
		//app_printf("bResult : %d\n",bResult);

        if(bResult)
        {
            pucData += 6;
            ucDataLen -= 6;
        }
        else
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port);
            zc_free_mem(pLeaveInd);
            return;
        }
    }   
	
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    app_printf("------------app_gw3762_afn11_f2_del_slave:=%d------------\n",pLeaveInd->StaNum);
    if(pLeaveInd->StaNum>0)
	{
	    if(Renetworkingtimer)
        {
            timer_start(Renetworkingtimer);
        }
        //app_printf("------------MACADDR------------\n");
        //dump_buf(pLeaveInd->pMac, 6 * pLeaveInd->StaNum);
        pLeaveInd->DelType = e_LEAVE_AND_DEL_DEVICELIST;
        ApsSlaveLeaveIndRequst(pLeaveInd);
    }

    zc_free_mem(pLeaveInd);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f3_set_slave_relay(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f3_set_slave_relay(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn11_f3_set_slave_relay!");
	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    //app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f4_set_work_mode(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F4处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f4_set_work_mode(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	//WHLPTST
 //   FlashInfo_t.WorkMode_t = *(DCR_WORK_MODE_t *)(pGw3762Data_t->DataUnit);
	//WHLPTED
    DCR_WORK_MODE_t WorkMode_t;

    memset((INT8U*)&WorkMode_t, 0U, sizeof(DCR_MANUFACTURER_t));
    __memcpy((INT8U*)&WorkMode_t, pGw3762Data_t->DataUnit, sizeof(DCR_WORK_MODE_t));
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

    if (JZQSETBAUD == WorkMode_t.usBaudSpeed)
    {
        RunningInfo.BaudRate = BAUDRATE_115200;
        BaudInfo.usBaudSpeed = JZQSETBAUD;
        BaudInfo.usBaudUnit = 1;
    }
    else
    {
        RunningInfo.BaudRate = BAUDRATE_9600;
        BaudInfo.usBaudSpeed = 0;
        BaudInfo.usBaudUnit = 0;
    }

    Modify_ChangeBaudtimer(&RunningInfo.BaudRate);
    //app_cco_baudrate_set(WorkMode_t.usBaudSpeed);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f5_start_reg_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F5处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f5_start_reg_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 ucStartTime[6] = {0};
	U32 DurationTime =0;
    
    if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG 
                || AppGw3762Afn10F4State.WorkSwitchRegister == TRUE)  //升级状态或不允许注册
    {
        //台体互操作测试搜表需要注释
        // app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, port);
        // return;
    }
    
    __memcpy((U8 *)&stAFN11F5, pGw3762Data_t->DataUnit, sizeof(APPGW3762AFN11F5_ST));

    /* 过滤非法时间 */
    const U8 is_bcd_time = FuncJudgeBCD((U8 *)&(stAFN11F5.ucStartTime), sizeof(stAFN11F5.ucStartTime)); // 判断是否是 BCD 码

    bcd_to_bin(stAFN11F5.ucStartTime, ucStartTime, 6);
    __memcpy(stAFN11F5.ucStartTime, ucStartTime, 6);

    SysDate_t bin_time = {0x00};
    __memcpy(&bin_time, &ucStartTime, sizeof(SysDate_t));
    const U8 is_valid_time = FunJudgeTime(bin_time); // 判断时间是否合法

    if (!is_bcd_time || !is_valid_time)
    {
        app_printf("StartTime error , is_valid_time:%s is_bcd_time:%s\n", is_valid_time ? "TRUE" : "FALSE", is_bcd_time ? "TRUE" : "FALSE");
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }

    STARegisterFlag =TRUE;
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	//os_sleep(50);
	//memset(RegSlaveMacAddrList , 0x00 , sizeof(RegSlaveMacAddrList)); //初始化列表
    AppGw3762Afn10F4State.WorkSwitchStates = REGISTER_RUN_FLAG;
    if(PROVINCE_JIANGSU == app_ext_info.province_code) //todo: PROVINCE_JIANGSU
    {
        register_info.RegisterType = e_GENERAL_REGISTER;
    }
	AppGw3762Afn10F4State.WorkSwitchRegister = TRUE; //注册完成需要置位FALSE
	AppGw3762Afn10F4State.RunStatusWorking = TRUE;   //正在工作,注册完成需要将标志清零。

	DurationTime=(stAFN11F5.ucDurationTime[0]+stAFN11F5.ucDurationTime[1]*255) *60 *1000;
	register_slave_start(DurationTime,port);
}

//甘肃增加部分，即装即采
static void app_gw3762_afn11_f225_start_reg_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 ucStartTime[6] = {0};
	//U32 DurationTime =0;
	                        
    if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)//升级状态，不响应抄表帧
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, port);
        return;
    }
    
    __memcpy((U8 *)&stAFN11F5, pGw3762Data_t->DataUnit, sizeof(APPGW3762AFN11F5_ST));
    bcd_to_bin(stAFN11F5.ucStartTime, ucStartTime, 6);
    __memcpy(stAFN11F5.ucStartTime, ucStartTime, 6);
	STARegisterFlag =TRUE;
	NewMeterReport = TRUE;
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	os_sleep(50);
	//memset(RegSlaveMacAddrList , 0x00 , sizeof(RegSlaveMacAddrList)); //初始化列表
    AppGw3762Afn10F4State.WorkSwitchStates = REGISTER_RUN_FLAG;
	AppGw3762Afn10F4State.WorkSwitchRegister = TRUE; //注册完成需要置位FALSE
	AppGw3762Afn10F4State.RunStatusWorking = TRUE;//正在工作,注册完成需要将标志清零。    
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f6_stop_reg_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F6处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f6_stop_reg_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

    AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;
//	AppGw3762Afn10F4State.WorkSwitchRegister = FALSE;
	AppGw3762Afn10F4State.RunStatusWorking = FALSE;//注册完成需要将标志清零。
    STARegisterFlag=FALSE;
    if(PROVINCE_JIANGSU == app_ext_info.province_code) //todo: PROVINCE_JIANGSU
    {
        register_info.RegisterType = e_CJQSELF_REGISTER;//正常的注册流程
    }
    change_register_run_state(e_REGISTER_JZQ_STOP, 10);
    modify_cycle_del_slave_timer(60);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f100_set_net_scale(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F100处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f100_set_net_scale(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn11_f100_set_net_scale!");
    app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f101_start_net_maintain(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F101处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f101_start_net_maintain(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn11_f101_start_net_maintain!");
    app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn11_f102_start_net_work(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN11F102处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn11_f102_start_net_work(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn11_f102_start_net_work!");
    app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
}
#endif

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn12_f1_restart(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN12F1处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn12_f1_restart(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    if(AppGw3762Afn10F4State.WorkSwitchStates == REGISTER_RUN_FLAG||AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)//升级状态，不响应抄表帧
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, port);
        return;
    }
    AppGw3762Afn10F4State.WorkSwitchStates = CONCURRENT_SWTICH_FLAG;
	AppGw3762Afn10F4State.RunStatusWorking = TRUE;

    router_read_change_route_active_state(EVT_ROUTER_RESET, 100);
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);   
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn12F2Suspend(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN12F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn12_f2_suspend(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    if(AppGw3762Afn10F4State.WorkSwitchStates != OTHER_SWITCH_FLAG )//升级状态，不响应抄表帧
    {
    	if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)
    	{
	        //app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
			if(TMR_RUNNING== upgrade_cco_get_ctrl_timer_state() && CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus != EVT_UPGRADE_IDLE)
	    	{
                upgrade_cco_ctrl_timer_modify_and_start(30 * 1000);
	        }
	        //return;
    	}
		else if(AppGw3762Afn10F4State.WorkSwitchStates == REGISTER_RUN_FLAG)
		{
			

		}
		else
		{
			__memcpy((U8*)&BackupAppGw3762Afn10F4State,(U8*)&AppGw3762Afn10F4State,sizeof(APPGW3762AFN10F4_t));
			AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;
			AppGw3762Afn10F4State.RunStatusWorking = FALSE;

		}
    }
    //如果开启了路由抄表，暂停30分钟
    router_read_suspend_timer(30*60);
    
 #if defined(STATIC_MASTER)
	 memset(f1f1infolist , 0x00 , sizeof(f1f1infolist));
 #endif    
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn12_f3_recover(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN12F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn12_f3_recover(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{    
	if(BackupAppGw3762Afn10F4State.WorkSwitchStates == CONCURRENT_SWTICH_FLAG)
	{
		__memcpy((U8*)&AppGw3762Afn10F4State,(U8*)&BackupAppGw3762Afn10F4State,sizeof(APPGW3762AFN10F4_t));
		BackupAppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;

        router_read_change_route_active_state(EVT_ROUTER_RESTART, 100);
	}
 	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
}

/*************************************************************************
 * 函数名称	: 	U8 app_dlt645_frame_stand(U8 *data, U8 *len)
 * 函数说明	: 	DLT645帧标准化
 * 参数说明	: 	data			- 数据缓存
 *					len			- 数据长度
 * 返回值		: 	无
 *************************************************************************/
//static U8 app_dlt645_frame_stand(U8 *data, U8 *len)
//{
//    U8 *pdata = data;
//    U8 tmplen = *len;
//    U8 datalen;
//
//    while(tmplen)
//    {
//        while((*pdata != APP_DLT645_HEAD_VALUE) && (tmplen > 0))
//        {
//            pdata ++;
//            tmplen --;
//        }
//
//        if(tmplen < APP_DLT645_BASE_LEN)
//        {
//            return FALSE;
//        }
//
//        if((*(pdata + APP_DLT645_HEAD2_POS) != APP_DLT645_HEAD_VALUE)  && (tmplen > 0))
//        {
//            pdata ++;
//            tmplen --;
//            continue;
//        }
//
//        datalen = *(pdata + APP_DLT645_LEN_POS);
//        if(tmplen < APP_DLT645_BASE_LEN + datalen)
//        {
//            pdata ++;
//            tmplen --;
//            continue;
//        }
//
//        if(*(pdata + (APP_DLT645_BASE_LEN + datalen - 1)) != APP_DLT645_END_VALUE)
//        {
//            pdata ++;
//            tmplen --;
//            continue;
//        }
//
//        if(*(pdata + (APP_DLT645_BASE_LEN + datalen - 2))
//                != check_sum(pdata, (APP_DLT645_BASE_LEN + datalen - 2)))
//        {
//            pdata ++;
//            tmplen --;
//            continue;
//        }
//
//        //__memcpy(data, pdata, (APP_DLT645_BASE_LEN + datalen));
//        __memmove(data, pdata, (APP_DLT645_BASE_LEN + datalen));
//        *len = APP_DLT645_BASE_LEN + datalen;
//
//        return TRUE;
//    }
//
//    return FALSE;
//}

/*************************************************************************
 * 函数名称	: 	app_gw3762_afn13_f1_monitor_slave
 * 函数说明	: 	监控从节点处理接口
 * 参数说明	: 	
 *				APPGW3762DATA_t *pGw3762Data_t	- 解析GW3762帧的数据
 *              MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/
static void app_gw3762_afn13_f1_monitor_slave(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U8  findMeterFlag = FALSE;
	U8  hasBuffFlag = FALSE;
    U16   ii=0;
    U16   byLen = 0;
    //刷新传文件保护
    //timer_start(ReUpgradetimer);
    
	 //集中器没有发暂停
    if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)//
    {
    	//升级过程增加支持点抄，此时拉上流程控制时间
        upgrade_cco_modify_upgrade_suspend_timer(30);
    }
	else if(AppGw3762Afn10F4State.WorkSwitchStates == REGISTER_RUN_FLAG)
	{
        modify_register_suspend_timer(30);
	                                             
	}
    //AppPIB_t.AppGw3762UpInfoFrameNum = pGw3762Data_t->DownInfoField.FrameNum;

	dump_buf(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);
    if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
    {
        /* 基本部分(4Bytes) = 通信协议类型+通信延时相关性标志+从节点附属节点数量+报文长度 */
        if(pGw3762Data_t->DataUnitLen < 4)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }
    else
    {
        if(pGw3762Data_t->DataUnitLen < 3)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }

    /* 解析AFN13F1数据单元到Gw3762Afn13F1Param */
    AppGw3762Afn13F1State.FrameSeq  = pGw3762Data_t->DownInfoField.FrameNum;
    AppGw3762Afn13F1State.ProtoType	= *gw3762downdataunit;
	gw3762downdataunit++;
    byLen ++;

#if (GW376213_PROTO_EN > 0)
    if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
    {
        AppGw3762Afn13F1State.DelayFlag		= *gw3762downdataunit;
		gw3762downdataunit++;
        byLen ++;

    }
#endif

    AppGw3762Afn13F1State.SlaveSubNum	= *gw3762downdataunit;
	gw3762downdataunit++;
    byLen ++;


    if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
    {
        if(pGw3762Data_t->DataUnitLen < 4 + AppGw3762Afn13F1State.SlaveSubNum * MAC_ADDR_LEN)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }
    else
    {
        if(pGw3762Data_t->DataUnitLen < 3 + AppGw3762Afn13F1State.SlaveSubNum * MAC_ADDR_LEN)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }

    if(AppGw3762Afn13F1State.SlaveSubNum > APP_GW3762_SLAVE_SUB_MAX)
    {
        __memcpy(AppGw3762Afn13F1State.SlaveSubAddr,
               gw3762downdataunit,
               APP_GW3762_SLAVE_SUB_MAX * MAC_ADDR_LEN);
    }
    else
    {
        if(AppGw3762Afn13F1State.SlaveSubNum != 0)
        {
            __memcpy(AppGw3762Afn13F1State.SlaveSubAddr,
                   gw3762downdataunit,
                   AppGw3762Afn13F1State.SlaveSubNum * MAC_ADDR_LEN);
        }
    }
    gw3762downdataunit += AppGw3762Afn13F1State.SlaveSubNum * MAC_ADDR_LEN;
    byLen += AppGw3762Afn13F1State.SlaveSubNum * MAC_ADDR_LEN;
    AppGw3762Afn13F1State.FrameLen		= *gw3762downdataunit;

	gw3762downdataunit ++;
    byLen ++;

 	if(AppGw3762Afn13F1State.ProtoType == APP_97_PROTOTYPE||
    AppGw3762Afn13F1State.ProtoType ==  APP_07_PROTOTYPE||
   	AppGw3762Afn13F1State.ProtoType ==  APP_698_PROTOTYPE)
 	{
		while(AppGw3762Afn13F1State.FrameLen > 12)
		{
			if(*(gw3762downdataunit) != 0x68)
			{
				gw3762downdataunit++;
                byLen++;
				AppGw3762Afn13F1State.FrameLen--;
			}
			else
			{
				break;
			}
		}
        //防止数据长度不符合要求
        if( 1 )
        {
            AppGw3762Afn13F1State.FrameLen = pGw3762Data_t->DataUnitLen - byLen;
	        app_printf("byLen = %d\n",byLen);
        }
 	}   
    
    if(AppGw3762Afn13F1State.FrameLen != 0)
    {
        __memcpy(AppGw3762Afn13F1State.FrameUnit, gw3762downdataunit, AppGw3762Afn13F1State.FrameLen);
    }
    else
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }
    
	if(pGw3762Data_t->DownInfoField.ModuleFlag == 1)
	{
		__memcpy(AppGw3762Afn13F1State.Addr,pGw3762Data_t->AddrField.DestAddr,6);	
	}
	else
	{
		if(TRUE==Check645Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL,AppGw3762Afn13F1State.Addr,NULL))
		{
			app_printf("645 frame\n");

		}
		else  if(TRUE==Check698Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL, AppGw3762Afn13F1State.Addr, NULL))
	    {
			app_printf("698 frame\n");
		}		
		else
		{
			app_printf("other frame \n");
		}
	}
	app_printf("AppGw3762Afn13F1State.Addr : ");
	dump_buf(AppGw3762Afn13F1State.Addr, 6);
	app_printf("AppGw3762Afn13F1State.FrameUnit : ");
	dump_buf(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen);
	
    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
    {
        if(memcmp(WhiteMacAddrList[ii].MacAddr, AppGw3762Afn13F1State.Addr, MAC_ADDR_LEN) == 0)
        {
            __memcpy(AppGw3762Afn13F1State.CnmAddr, WhiteMacAddrList[ii].CnmAddr, MAC_ADDR_LEN);
			app_printf("AppGw3762Afn13F1State.cnmaddr : ");
			dump_buf(WhiteMacAddrList[ii].CnmAddr, 6);
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
    AppGw3762Afn13F1State.ValidFlag = TRUE;
    AppGw3762Afn13F1State.Afn02Flag = FALSE;
    AppGw3762Afn14F1State.ValidFlag = FALSE;

    router_read_suspend_timer(100);

	hasBuffFlag = TRUE;
	
	if(hasBuffFlag == TRUE)
    {
        //app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
        
#if (GW376213_PROTO_EN > 0)
        
        if(AppGw3762Afn13F1State.DelayFlag == TRUE)
        {
            app_gw3762_up_afn14_f3_router_req_revise(AppGw3762Afn13F1State.Addr,  0, 
                    AppGw3762Afn13F1State.FrameUnit, AppGw3762Afn13F1State.FrameLen, port);
    
            return;
        }
#endif
        
    }
    else
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, port);
        app_printf("376.2   APP_GW3762_MASTERBUSY_ERRCODE \n");
        return;
    }
    U8   nullAddr[6] = {0,0,0,0,0,0};
    if(memcmp(AppGw3762Afn13F1State.CnmAddr,nullAddr,6)==0)
    {
    	memset(AppGw3762Afn13F1State.CnmAddr,0xFF,6);
        app_printf("AllBroadcast\n");
    }
    jzq_read_cntt_act_read_meter_req(AppGw3762Afn13F1State.FrameSeq, AppGw3762Afn13F1State.Addr, AppGw3762Afn13F1State.CnmAddr, 
                 AppGw3762Afn13F1State.ProtoType, AppGw3762Afn13F1State.FrameLen, AppGw3762Afn13F1State.FrameUnit, 
                 AppGw3762Afn13F1State.Afn02Flag, port);
}

//static U8 get_phase_by_addr(U8 *addr)
//{
//	U16 usIdx=0;
//	for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
//	{
//		WHLPTST
//	    if(0==memcmp(addr, WhiteMacAddrList[usIdx].MacAddr, 6))
//	    {
//			WHLPTED
//				app_printf("report phase : %s\n",WhiteMacAddrList[usIdx].MeterInfo_t.ucPhase==1?"A":
//										WhiteMacAddrList[usIdx].MeterInfo_t.ucPhase==2?"B":
//										WhiteMacAddrList[usIdx].MeterInfo_t.ucPhase==3?"C":"U");
//			return WhiteMacAddrList[usIdx].MeterInfo_t.ucPhase;
//	    }
//		WHLPTED
//	}
//	return 0;
//}

U8 check_whitelist_by_addr(U8 *addr)
{
	U16 usIdx=0;
	for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
	{
		WHLPTST
	    if(0==memcmp(addr, WhiteMacAddrList[usIdx].MacAddr, MAC_ADDR_LEN))
	    {
			WHLPTED
				app_printf("It's in Whitelist ! \n");
			return TRUE;
	    }
		WHLPTED
	}
	app_printf("It's not in Whitelist ! \n");
	return FALSE;
}

S16 seach_seq_by_mac_addr(U8 *addr)
{
	U16 ii=0;
	do
	{
		if(0==memcmp(addr,WhiteMacAddrList[ii].MacAddr,6))
		{
			//__memcpy(WhiteMacAddrList[ii].MacAddr,addr,7);
			return ii;		
		}
	}while(ii++ < MAX_WHITELIST_NUM);

	return -1;
}

static U8 get_cnm_addr_by_mac_addr(U8 *MacAddr,U8 *CnmAddr)
{
	U16 ii;
	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	{
		if(0==memcmp(WhiteMacAddrList[ii].MacAddr,MacAddr,6))
        {
            __memcpy(CnmAddr,WhiteMacAddrList[ii].CnmAddr,6);
            return TRUE;
        }      
	}
    return FALSE;
}

U8 get_protocol_by_addr(U8 *addr)
{
	U16 ii;
	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
	{
		if(memcmp(addr, WhiteMacAddrList[ii].MacAddr, MAC_ADDR_LEN) == 0)
		{
			return WhiteMacAddrList[ii].MeterInfo_t.ucGUIYUE;
		}
	}
	return 0;
}

//U8 check_whitelist_by_addr(U8 *addr)
//{
//	U16 ii;
//	for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
//	{
//		if(memcmp(addr, WhiteMacAddrList[ii].MacAddr, MAC_ADDR_LEN) == 0)
//		{
//			return TRUE;
//		}
//	}
//	return FALSE;
//}

//static U8 get_ln_err_by_addr(U8 *addr)
//{
//	U16 usIdx=0;
//    WHLPTST
//	for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
//	{
//		
//	    if(0==memcmp(addr, WhiteMacAddrList[usIdx].MacAddr, 6))
//	    {
//			WHLPTED
//				app_printf("report LNerr : %s\n",WhiteMacAddrList[usIdx].MeterInfo_t.LNerr==1?"LN is err":"LN is Cure");
//			return WhiteMacAddrList[usIdx].MeterInfo_t.LNerr;
//	    }
//		
//	}
//    WHLPTED
//	return 0;
//}

void save_mode_id_by_addr(U8 *macaddr , U8* info ,U8 infolen)
{
	U16 usIdx=0;
	WHLPTST
    for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
	{	
	    if(0==memcmp(macaddr, WhiteMacAddrList[usIdx].MacAddr, 6))
	    {
			WhiteMacAddrList[usIdx].IDState =MOUDLTID_RENEW;
			__memcpy( WhiteMacAddrList[usIdx].ModuleID ,info,sizeof(WhiteMacAddrList[usIdx].ModuleID));
            app_printf("MID_RENEW :");
            dump_buf(WhiteMacAddrList[usIdx].MacAddr,6);
            break;
	    }
    }
	WHLPTED
	if(usIdx < MAX_WHITELIST_NUM)
	{
		flash_timer_start();
	}
    return;
}

void renew_all_mode_id_state()
{
	U16 usIdx=0;
    U8 nulladdr[6]={0};
	WHLPTST
    for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
	{	
	    if(0!=memcmp(nulladdr, WhiteMacAddrList[usIdx].MacAddr, 6))
	    {
			
			WhiteMacAddrList[usIdx].IDState =MOUDLTID_NO_UP;
	    }
		else
		{
			WhiteMacAddrList[usIdx].IDState =MOUDLTID_NO_UP;
			memset(WhiteMacAddrList[usIdx].ModuleID,0x00,sizeof(WhiteMacAddrList[usIdx].ModuleID));
		}
    }
	WHLPTED;
    return;
}

static U8 get_mode_id_by_addr(U8 *macaddr , U8* info ,U8 *IDstate)
{
    U8  state = FALSE;
	U16 usIdx=0;
	U8  null_ID[11] = {0};
	//U8  defult_ID[11] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	WHLPTST
    for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
	{	
	    if(0==memcmp(macaddr, WhiteMacAddrList[usIdx].MacAddr, 6))
	    {
			
			//if(memcmp(WhiteMacAddrList[usIdx].ModuleID ,null_ID,11)!= 0)
			{
				__memcpy(info, WhiteMacAddrList[usIdx].ModuleID ,sizeof(WhiteMacAddrList[usIdx].ModuleID));
			
				*IDstate = WhiteMacAddrList[usIdx].IDState;
                state = TRUE;
			}
            /*
			else
			{
				__memcpy(info, null_ID ,sizeof(null_ID));
				*IDstate = MOUDLTID_NO_UP;
			}*/
			break;
	    }
    }
	WHLPTED
	
	if(usIdx >= MAX_WHITELIST_NUM)
	{
		__memcpy(info, null_ID ,sizeof(null_ID));
        *IDstate = MOUDLTID_NO_UP;
	}
    return state;
}

//static U16  get_online_white_addr_num()
//{
//	U16 usIdx=0;
//    U8 nulladdr[6]={0};
//    U16 num = 0;
//	WHLPTST
//    for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
//	{	
//	    if(0!=memcmp(nulladdr, WhiteMacAddrList[usIdx].CnmAddr, 6))
//	    {
//			num++;
//	    }
//		else
//		{
//			
//		}
//    }
//	WHLPTED
//    return num;
//}

/*************************************************************************
 * 函数名称	: 	app_gw3762_up_afn13_f1_up_frame
 * 函数说明	: 	监控从节点上行报文处理接口
 * 参数说明	: 	U8 *Addr                    - 表地址
                AppProtoType_e proto        - 协议类型
                U16 time                    - 当前报文本地通信上行时长
                U8 *data                    - 上行数据
                U16 len                     - 上行数据长度
                MESG_PORT_e port            - 消息类型
                U8 localseq                 - 本地报文序号
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn13_f1_up_frame(U8 *Addr, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port,U8 localseq)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16  byLen = 0;    

    Gw3762updata.CtrlField.TransDir  = APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 1,Addr,localseq);

    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;
    Gw3762updata.Afn				= APP_GW3762_AFN13;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);

    __memcpy(Gw3762updata.AddrField.SrcAddr, Addr, MAC_ADDR_LEN);
    __memcpy(Gw3762updata.AddrField.DestAddr, FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN);

    //if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)        
   
	if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
	{
    	Gw3762updata.DataUnit[byLen++] = (U8)(time & 0xFF);
    	Gw3762updata.DataUnit[byLen++] = (U8)(time >> 8);
	}
    //辽宁要求，回复实际协议类型
	//if(len == 0) proto = 0;

    Gw3762updata.DataUnit[byLen++] = proto;

    Gw3762updata.DataUnit[byLen++] = len;
	
	if(len > 0)
    {
        __memcpy(&Gw3762updata.DataUnit[byLen], data, len);
        byLen += len;
    }
    Gw3762updata.DataUnitLen		= byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,localseq, FALSE, 0, port,e_Serial_AFN13F1);
}

/*************************************************************************
 * 函数名称	: 	app_gw3762_afn14_f1_route_req_read
 * 函数说明	: 	路由主动请求响应处理接口
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 *              MESG_PORT_e port - 消息类型
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn14_f1_route_req_read(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
	U8  nullAddr[6] = {0,0,0,0,0,0};

    if(AppGw3762Afn10F4State.WorkSwitchStates == REGISTER_RUN_FLAG||AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)//不是轮抄状态，不响应抄表帧
    {
        app_printf("It is Registering or Upgrading.\n");
        app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, port);
        return;
    }
    

    if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
    {
        /* 基本部分(4Bytes) = 通信协议类型+通信延时相关性标志+从节点附属节点数量+报文长度 */
        if(pGw3762Data_t->DataUnitLen < 4)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }
    else
    {
        if(pGw3762Data_t->DataUnitLen < 3)
        {
            app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
            return;
        }
    }

    /* 解析AFN13F1数据单元到Gw3762Afn14F1Param */
    AppGw3762Afn14F1State.FrameSeq  = pGw3762Data_t->DownInfoField.FrameNum;
    AppGw3762Afn14F1State.ReadState = *gw3762downdataunit ++;

    app_printf("AppGw3762Afn14F1State.ReadState is %s\n", 0==AppGw3762Afn14F1State.ReadState?"fail to apply":
															1==AppGw3762Afn14F1State.ReadState?"read finish":
															2==AppGw3762Afn14F1State.ReadState?"success to apply":"unknow");
    if(RouterActiveInfo_t.State != EVT_ROUTER_RESP)
    {
        app_printf("RouterActiveInfo_t.State = %d\n", RouterActiveInfo_t.State);
        app_gw3762_up_afn00_f2_deny(APP_GW3762_MASTERBUSY_ERRCODE, port);
        return;
    }
	
    switch (AppGw3762Afn14F1State.ReadState)
    {
    case 0://申请失败
    {
        WhiteMacAddrList[RouterActiveInfo_t.CrnMeterIndex].Result = e_UNFINISH;
        RouterActiveInfo_t.CrnMeterIndex++;//下一块
        router_read_change_route_active_state(EVT_ROUTER_REQ, 100);
        return;
    }
    case 1://抄读完成
    {
        WhiteMacAddrList[RouterActiveInfo_t.CrnMeterIndex].Result = e_FINISH;
        RouterActiveInfo_t.CrnMeterIndex++;//下一块
        router_read_change_route_active_state(EVT_ROUTER_REQ, 100);
        return;
    }

    case 2://可以抄读
    {
        break;
    }
    default :
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }
	
	if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
        AppGw3762Afn14F1State.CmnRlyFlag = *(gw3762downdataunit ++);
    AppGw3762Afn14F1State.FrameLen	= *(gw3762downdataunit ++);
    #if  1  
	while( AppGw3762Afn14F1State.FrameLen > 12)
	{
		if(*(gw3762downdataunit) != 0x68)
		{
			gw3762downdataunit++;
			AppGw3762Afn14F1State.FrameLen--;
		}
		else
		{
			break;
		}
	}
	#endif

    __memcpy(AppGw3762Afn14F1State.FrameUnit, gw3762downdataunit, AppGw3762Afn14F1State.FrameLen);
    app_printf("14F1 send to plc");
    dump_buf(AppGw3762Afn14F1State.FrameUnit,  AppGw3762Afn14F1State.FrameLen);

    AppGw3762Afn14F1State.SlaveSubNum = *(gw3762downdataunit + AppGw3762Afn14F1State.FrameLen);


    __memcpy(AppGw3762Afn14F1State.SlaveSubAddr, gw3762downdataunit + AppGw3762Afn14F1State.FrameLen + 1, MAC_ADDR_LEN * AppGw3762Afn14F1State.SlaveSubNum);

    if (pGw3762Data_t->DownInfoField.ModuleFlag)
    {
		__memcpy(AppGw3762Afn14F1State.Addr, pGw3762Data_t->AddrField.DestAddr, MAC_ADDR_LEN);
    }
	else
	{
		if(TRUE==Check645Frame(AppGw3762Afn14F1State.FrameUnit, AppGw3762Afn14F1State.FrameLen, NULL,AppGw3762Afn14F1State.Addr,NULL))
		{
			app_printf("645 frame\n");
		}
		else  if(TRUE==Check698Frame(AppGw3762Afn14F1State.FrameUnit,AppGw3762Afn14F1State.FrameLen, NULL,AppGw3762Afn14F1State.Addr, NULL))
	    {
			app_printf("698 frame\n");
		}		
		else
		{
			app_printf("other frame \n");
		}
	}

    get_cnm_addr_by_mac_addr(AppGw3762Afn14F1State.Addr,AppGw3762Afn14F1State.CnmAddr);
    AppGw3762Afn14F1State.ProtoType = get_protocol_by_addr(AppGw3762Afn14F1State.Addr);
	app_printf("AppGw3762Afn14F1State.Addr : ");
	dump_buf(AppGw3762Afn14F1State.Addr, MAC_ADDR_LEN);
    app_printf("AppGw3762Afn14F1State.cnmaddr : ");
    dump_buf(AppGw3762Afn14F1State.CnmAddr, MAC_ADDR_LEN);
    if(memcmp(AppGw3762Afn14F1State.CnmAddr,nullAddr,6)==0)
    {
    	memset(AppGw3762Afn14F1State.CnmAddr,0xFF,6);
        app_printf("AllBroadcast\n");
    }
    //AppGw3762Afn14F1State.ReTransmitCnt  = RETRANMITMAXNUM;//RETRANMITMAXNUM;
    AppGw3762Afn13F1State.ValidFlag = FALSE;
    AppGw3762Afn13F1State.Afn02Flag = FALSE;
    AppGw3762Afn14F1State.ValidFlag = TRUE;
	
#if (GW376213_PROTO_EN > 0)
    if(AppGw3762Afn14F1State.CmnRlyFlag == TRUE)
    {
        app_gw3762_up_afn14_f3_router_req_revise(AppGw3762Afn14F1State.Addr,  0, 
                AppGw3762Afn14F1State.FrameUnit, AppGw3762Afn14F1State.FrameLen, port);

        return;
    }
#endif

    router_read_meter(AppGw3762Afn14F1State.FrameSeq, AppGw3762Afn14F1State.Addr, AppGw3762Afn14F1State.CnmAddr, 
                             AppGw3762Afn14F1State.ProtoType, AppGw3762Afn14F1State.FrameLen, AppGw3762Afn14F1State.FrameUnit, port);
}

/*************************************************************************
 * 函数名称	: 	app_gw3762_up_afn14_f1_up_frame
 * 函数说明	: 	路由主动抄读请求接口
 * 参数说明	: 	AppGw3762Afn14F1Up_t AppGw3762Afn14F1Up	- 上行主动请求原语
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn14_f1_up_frame(AppGw3762Afn14F1Up_t AppGw3762Afn14F1Up)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir  = APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag = APP_GW3762_MASTER_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;
	//AppGw3762Afn14F1Up.MeterIndex;
	//U8 phase = get_phase_Devicelist(AppGw3762Afn14F1Up.Addr);
    app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField),AppGw3762Afn14F1Up.Addr);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN14;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);

    if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
    {
        Gw3762updata.DataUnitLen		= 9;
        __memcpy(Gw3762updata.DataUnit, &AppGw3762Afn14F1Up, Gw3762updata.DataUnitLen);
    }
    else
    {
        Gw3762updata.DataUnitLen		= 9;
        __memcpy(Gw3762updata.DataUnit, &AppGw3762Afn14F1Up, Gw3762updata.DataUnitLen);
    }

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);

    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, 0, e_UART1_MSG,e_Serial_OTHER);
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afn14_f2_router_req_clock(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN14F2处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn14_f2_router_req_clock(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	extern U8 ReadJZQclockSuccFlag;
    SysDate_t date = {0};
    
	__memcpy((U8*)&date, pGw3762Data_t->DataUnit, sizeof(SysDate_t));

    /* 过滤非法时间 */
    dump_buf((U8 *)&date, sizeof(SysDate_t));
    const U8 is_bcd_time = FuncJudgeBCD((U8 *)&date, sizeof(SysDate_t)); // 判断是否是 BCD 码
    SysDate_t bin_date = {0};
    bcd_to_bin((U8 *)&date, (U8 *)&bin_date, sizeof(SysDate_t));
    const U8 is_valid_time = FunJudgeTime(bin_date); // 判断时间是否合法

    if (!is_bcd_time || !is_valid_time)
    {
        app_printf("Time error , is_valid_time:%s is_bcd_time:%s\n", is_valid_time ? "TRUE" : "FALSE", is_bcd_time ? "TRUE" : "FALSE");
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
        return;
    }

    SysTemTimeSet(&date);
	ReadJZQclockSuccFlag = TRUE;
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

    if(PROVINCE_HUNAN==app_ext_info.province_code || PROVINCE_SHANNXI==app_ext_info.province_code)//todo: PROVINCE_HUNAN
    {
        cco_check_and_run_module_time_syns(pGw3762Data_t->DataUnit);
    }
    else
    {
        //时钟维护，获取到集中器时钟后发代理广播给从模块
	    cco_clock_maintain();
    }
	return ;
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	app_gw3762_afn14_f3_router_req_revise
 * 函数说明	: 	GW3762 AFN14F3处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn14_f3_router_req_revise(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8  length = pGw3762Data_t->DataUnit[0];

    app_printf("app_gw3762_afn14_f3_router_req_revise!");
    //app_gw3762_up_afn00_f2_deny(APP_GW3762_NOSUPPORT_ERRCODE, port);
    
    if(length > 0)
    {
        if(AppGw3762Afn13F1State.ValidFlag == TRUE)
        {
            jzq_read_cntt_act_read_meter_req(AppGw3762Afn13F1State.FrameSeq, AppGw3762Afn13F1State.Addr, AppGw3762Afn13F1State.CnmAddr, 
                 AppGw3762Afn13F1State.ProtoType, AppGw3762Afn13F1State.FrameLen, AppGw3762Afn13F1State.FrameUnit, 
                 AppGw3762Afn13F1State.Afn02Flag, port);
        }
        else if(AppGw3762Afn14F1State.ValidFlag == TRUE)
        {
        	router_read_meter(AppGw3762Afn14F1State.FrameSeq, AppGw3762Afn14F1State.Addr, AppGw3762Afn14F1State.CnmAddr, 
                             AppGw3762Afn14F1State.ProtoType, AppGw3762Afn14F1State.FrameLen, AppGw3762Afn14F1State.FrameUnit, port);
        }
    }
}
#endif

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn14F4RouterReqRevise(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN14F4处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_afn14_f4_request_ac_sampling(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn14_f4_request_ac_sampling!");

    U8  type  = pGw3762Data_t->DataUnit[0];
    //U8  *ID   = &pGw3762Data_t->DataUnit[1];
	//U8  *data = &pGw3762Data_t->DataUnit[5]; 
	if(e_ACSAMPING_TPEY_645 == type)
	{
		//解析645		
	}
	else if(e_ACSAMPING_TPEY_698 == type)
	{
		//解析698
	}
}

/*************************************************************************
 * 函数名称	: 	static void AppGw3762Afn14F4RouterReqRevise(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN14F4上行
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_afn14_f4_up_frame(U8 type,U8 *ID, U16 len,U8 phase,MESG_PORT_e port)
{
    app_printf("app_gw3762_afn14_f4_up_frame!\n");
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_MASTER_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField),NULL);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN14;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F4);

	Gw3762updata.DataUnitLen = 0;
	Gw3762updata.DataUnit[0] = type;
	Gw3762updata.DataUnitLen += 1;
    __memcpy(Gw3762updata.DataUnit+1, ID, len);
	Gw3762updata.DataUnitLen += len;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_OTHER);
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn15_f1_up_frame( U32  blockID)
 * 函数说明	: 	GW3762 AFN15F1上行处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_up_afn15_f1_up_frame(U32  blockID, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);//从设备回复
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN15;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);
    
    Gw3762updata.DataUnitLen		= 4;
    __memcpy(Gw3762updata.DataUnit, &blockID, 4);

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_OTHER);
    //app_printf("blockID is %d\n", blockID);
    //app_printf("Gw3762SendDataLen = %d!\n", Gw3762SendDataLen);
    //dump_buf(Gw3762SendData,Gw3762SendDataLen);
}

//static void change_version_2_hex(U8  *asciiVer, U8  *hexVer)
//{
//    U8  value = 0;
//
//    value = ((asciiVer[2] - 0x30) << 4);
//    value += ((asciiVer[3] - 0x30));
//
//    hexVer[0] = value;
//    
//    value = 0;
//    value = ((asciiVer[0] - 0x30) << 4);
//    value += ((asciiVer[1] - 0x30));
//    hexVer[1] = value;
//
//    return;
//}

/*************************************************************************
 * 函数名称	: 	app_gw3762_afn15_f1_file_trans
 * 函数说明	: 	文件传输处理接口
 * 参数说明	: 	
 *				APPGW3762DATA_t *pGw3762Data_t	- 解析GW3762帧的数据
 *              MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/
static void app_gw3762_afn15_f1_file_trans(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U16 rcvLen = 0;
    U8  fileTransId;
    U32 blockID;

    fileTransId  = gw3762downdataunit[rcvLen++];

    app_printf("fileTransId is %d\n", fileTransId);

	//工作状态，工作开关设置正在升级
	if(1)//(AppGw3762Afn10F4State.WorkSwitchStates == OTHER_SWITCH_FLAG|| AppGw3762Afn10F4State.WorkSwitchStates==CARRIER_UPGRADE_FLAG)
	{
		AppGw3762Afn10F4State.WorkSwitchStates = CARRIER_UPGRADE_FLAG;//升级状态
		AppGw3762Afn10F4State.RunStatusWorking = TRUE;//正在工作
	}
	else
	{
        app_gw3762_up_afn15_f1_up_frame(0xFFFFFFFF, port);
		return;
	}
    
	app_printf("app_gw3762_afn15_f1_file_trans!");
    blockID = upgrade_cco_check_upgrade_data(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);
    app_gw3762_up_afn15_f1_up_frame(blockID, port);
    
	if(fileTransId == APP_GW3762_CLEAR_DOWNLOAD_FILE)
	{
	   //强制停止升级使用
       //app_gw3762_up_afn15_f1_up_frame(0x00, port);
       if(AppGw3762Afn10F4State.WorkSwitchStates == CARRIER_UPGRADE_FLAG)
       {
           upgrade_cco_ctrl_timer_modify(100,upgrade_cco_stop);
       }
       AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;//释放升级状态
       AppGw3762Afn10F4State.RunStatusWorking = 0  ;
       timer_stop(ReUpgradetimer,TMR_NULL);

	   
	   memset(FileUpgradeData,0x00,sizeof(FileUpgradeData));
	}
    else if(fileTransId == APP_GW3762_LOCAL_MODULE_UPDATE)  // local module update
    {
        
    }      
	else if(fileTransId == APP_GW3762_LOCAL_REMOTE_UPDATE)
	{
        
	}	
	else if(fileTransId == APP_GW3762_SLAVE_MODULE_UPDATE || fileTransId == APP_GW3762_SLAVE_CJQ_UPGRADE_START)
    {

    }   
   else if(fileTransId == APP_GW3762_SLAVE_UPGRADE_STOP)
   {
       //强制停止升级使用
       //app_gw3762_up_afn15_f1_up_frame(0x00, port);

       AppGw3762Afn10F4State.WorkSwitchStates = OTHER_SWITCH_FLAG;//释放升级状态
       AppGw3762Afn10F4State.RunStatusWorking = 0 ;
       timer_stop(ReUpgradetimer,TMR_NULL);
       CcoUpgrdCtrlInfo.UpgrdStatusInfo.CcoUpgradeStatus = EVT_STOP_UPGRADE ;
	   CcoUpgrdCtrlInfo.UpgrdStatusInfo.ReTransmitCnt = RETRANMITMAXNUM;
	   CcoUpgrdCtrlInfo.UpgrdStatusInfo.UpgradePLCTimeout = UPGRADE_PLC_TIME_OUT;
       upgrade_cco_ctrl_timer_modify(100,upgrade_cco_stop);
   }
   else if(fileTransId == APP_GW3762_SLAVE_UPGRADE_START)
   {
  
   }
   else
   {
   }		
}

static void app_gw3762_afn20_f1_start_rwm(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn20_f1_start_rwm\n");
	report_frame_seq = 0;
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

	app_printf("cco start collect singele water\n");
}

static void app_gw3762_afn20_f2_stop_rwm(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn20_f2_stop_rwm\n");

    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	if(Reprotwatertimer)
	{
		timer_stop(Reprotwatertimer,TMR_NULL);
	}
}

static void app_gw3762_afn20_f3_report_wm(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn20_f3_report_wm  not surport down frame\n");
}

static void app_gw3762_afn20_f4_report_end(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn20_f4_report_end not surport down frame\n");
}

/*************************************************************************
 * 函数名称	: 	static void app_gw3762_afnf0_f5_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN240F5处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static U16 encode_f0_f5_data_unit(U8 msgType, U8 *pData, DEVICE_TEI_LIST_t *devInfo)
{
    if(msgType == 1)
    {
        APPGW3762AFNF005UP_UNIT1_t *pUnit = (APPGW3762AFNF005UP_UNIT1_t *)pData;

        __memcpy(pUnit->MacAddr, devInfo->MacAddr, 6);

        pUnit->NodeTEI            = devInfo->NodeTEI;
        pUnit->DeviceType         = devInfo->DeviceType;
        pUnit->Phase              = devInfo->Phase;
        pUnit->LNerr              = devInfo->LNerr;
        pUnit->Edgeinfo           = devInfo->Edgeinfo;
        pUnit->NodeMachine        = devInfo->NodeMachine;
        pUnit->NodeType           = devInfo->NodeType;
        pUnit->NodeState          = devInfo->NodeState;
        pUnit->NodeDepth          = devInfo->NodeDepth;
        pUnit->Reset              = devInfo->Reset;
        pUnit->DurationTime       = devInfo->DurationTime;
        pUnit->ParentTEI          = devInfo->ParentTEI;
        pUnit->UplinkSuccRatio    = devInfo->UplinkSuccRatio;
        pUnit->DownlinkSuccRatio  = devInfo->DownlinkSuccRatio;

        __memcpy(pUnit->SoftVerNum, devInfo->SoftVerNum, sizeof(devInfo->SoftVerNum));
        pUnit->BuildTime          = devInfo->BuildTime;
        pUnit->BootVersion        = devInfo->BootVersion;

        __memcpy(pUnit->ManufactorCode, devInfo->ManufactorCode, sizeof(devInfo->ManufactorCode));
        __memcpy(pUnit->ChipVerNum, devInfo->ChipVerNum, sizeof(devInfo->ChipVerNum));
        pUnit->F31_D0             = devInfo->F31_D0;
        pUnit->F31_D1             = devInfo->F31_D1;
        pUnit->F31_D2             = devInfo->F31_D2;
        pUnit->F31_D5             = devInfo->F31_D5;
        pUnit->F31_D6             = devInfo->F31_D6;
        pUnit->F31_D7             = devInfo->F31_D7;

        pUnit->AREA_ERR           = devInfo->AREA_ERR;
        pUnit->CCOMACSEQ          = 0;
        pUnit->ModeNeed           = devInfo->ModeNeed;

        __memcpy(&pUnit->ManufactorInfo, &devInfo->ManufactorInfo, sizeof(devInfo->ManufactorInfo));

        return sizeof(APPGW3762AFNF005UP_UNIT1_t);
    }
    else if(msgType == 2)
    {
        APPGW3762AFNF005UP_UNIT2_t *pUnit = (APPGW3762AFNF005UP_UNIT2_t *)pData;

        __memcpy(pUnit->MacAddr, devInfo->MacAddr, 6);
        pUnit->NodeType           = devInfo->NodeType;
        pUnit->NodeState          = devInfo->NodeState;
        pUnit->NodeDepth          = devInfo->NodeDepth;
        pUnit->Reset              = devInfo->Reset;
        pUnit->DurationTime       = devInfo->DurationTime;
        pUnit->ParentTEI          = devInfo->ParentTEI;
        pUnit->UplinkSuccRatio    = devInfo->UplinkSuccRatio;
        pUnit->DownlinkSuccRatio  = devInfo->DownlinkSuccRatio;
        pUnit->F31_D0             = devInfo->F31_D0;
        pUnit->F31_D1             = devInfo->F31_D1;
        pUnit->F31_D2             = devInfo->F31_D2;
        pUnit->F31_D5             = devInfo->F31_D5;
        pUnit->F31_D6             = devInfo->F31_D6;
        pUnit->F31_D7             = devInfo->F31_D7;

        pUnit->AREA_ERR           = devInfo->AREA_ERR;

        pUnit->CCOMACSEQ          = 0;
        pUnit->ModeNeed           = devInfo->ModeNeed;

        return sizeof(APPGW3762AFNF005UP_UNIT2_t);
    }

    return 0;
}

static void app_gw3762_afnf0_f5_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f5_debug-----\n");
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
	U16 Gw3762SendDataLen = 0;
	APPGW3762DATA_t Gw3762updata;
	U8 *pGW3762updataunit = Gw3762updata.DataUnit;
	U8	ucReadNum = 0;
	//U8	default_addr[6]={0,0,0,0,0,0};
	//U8	FF_addr[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	U16 usStartIdx = 0;
	U16  ii = 0;
	U16  jj = 0;
	U8   MesgType = 0;
	
	static U8  start_1_flag=1;
	U8	*pnum=NULL;
    DEVICE_TEI_LIST_t DeviceListTemp;
	
	MesgType = pGw3762Data_t->DataUnit[0];
	usStartIdx = (U16)pGw3762Data_t->DataUnit[1];
	usStartIdx += (U16)pGw3762Data_t->DataUnit[2] << 8;
	ucReadNum = pGw3762Data_t->DataUnit[3];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}	
	
	app_printf("usStartIdx = %d\n",usStartIdx);

	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode	= pGw3762Data_t->CtrlField.TransMode;
	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum		= 0;
	Gw3762updata.Afn				= APP_GW3762_AFNF0;
	Gw3762updata.Fn 			= app_gw3762_fn_bin_to_bs(APP_GW3762_F5);
	Gw3762updata.DataUnitLen			= 1 + 2 + 2 + 1;
	pGW3762updataunit[0]		  = MesgType;
	pGW3762updataunit[1]		  = (U8)(APP_GETDEVNUM() & 0xFF);
	pGW3762updataunit[2]		  = (U8)(APP_GETDEVNUM() >> 8);
	pGW3762updataunit[3]		  = (U8)(usStartIdx & 0xFF);
	pGW3762updataunit[4]		  = (U8)(usStartIdx >> 8);
	pGW3762updataunit[5]		  = 0;//ucReadNum;
	pnum = &pGW3762updataunit[5];
	pGW3762updataunit			  += 6;
	if(ucReadNum == 0)//查询数量为0
	{
		goto send;
	}
	
	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	
	//U8 DeviceListTemp[sizeof(DEVICE_TEI_LIST_t)];
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		
		//DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj , (DEVICE_TEI_LIST_t *)&DeviceListTemp);
		if(DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp) == FALSE)//无效地址
		{
			continue;
		}
		
		if(ii == usStartIdx)//当前为读取的起始位置
        {
            U16 UnitLen =  encode_f0_f5_data_unit(MesgType, pGW3762updataunit, &DeviceListTemp);
            Gw3762updata.DataUnitLen += UnitLen;
            pGW3762updataunit += UnitLen;
			
            *pnum += 1;

			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				//WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}		
	}
	
	//WHLPTED//释放共享存储内容保护
	
	if(*pnum == 0)
	{
		g_QueryLastRecordTimes++;
		if(g_QueryLastRecordTimes >= 2)
		{
			g_QueryLastRecordTimes = 0;
			goto send;
		}
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读 
		return; 
	}
	send:
	app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afnf0_f6_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
  
}

static void app_gw3762_afnf0_f7_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{

}

static void app_gw3762_afnf0_f8_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U16   rcvLen = 0;
	
    __memcpy(AppGw3762AfnF0F8State.DstAddr,  &gw3762downdataunit[rcvLen], 6);
    rcvLen += 6;
	app_printf("-app_gw3762_afnf0_f8_debug-----\n");
	dump_buf(AppGw3762AfnF0F8State.DstAddr,6);
    AppGw3762AfnF0F8State.GatherType = gw3762downdataunit[rcvLen++];


    ZONEAREA_INFO_REQ_t   *pZoneAreaInfoReq = NULL;

    //申请原语空间
    pZoneAreaInfoReq = (ZONEAREA_INFO_REQ_t*)zc_malloc_mem(sizeof(ZONEAREA_INFO_REQ_t)+6,"ZoneAreaInfoReq",MEM_TIME_OUT);

	pZoneAreaInfoReq->AsduLength =6;
	ChangeMacAddrOrder(AppGw3762AfnF0F8State.DstAddr);
	__memcpy(pZoneAreaInfoReq->Asdu,AppGw3762AfnF0F8State.DstAddr,6);
    pZoneAreaInfoReq->Direct = e_DOWNLINK;
    pZoneAreaInfoReq->GatherType = AppGw3762AfnF0F8State.GatherType;

    __memcpy(pZoneAreaInfoReq->DstMacAddr, AppGw3762AfnF0F8State.DstAddr, 6);
    __memcpy(pZoneAreaInfoReq->SrcMacAddr, FlashInfo_t.ucJZQMacAddr, 6);

    ApsZoneAreaInfoReqeust(pZoneAreaInfoReq);

    zc_free_mem(pZoneAreaInfoReq);
}

static void app_gw3762_afnf0_f9_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f9_debug-----\n");
    dump_buf(pGw3762Data_t->DataUnit,pGw3762Data_t->DataUnitLen);
    if(pGw3762Data_t->DataUnit[0]==1)
    {
        int32_t tgain=0;
        int16_t ana=0;
        int16_t dig=0;
        dig = DefSetInfo.FreqInfo.tgaindig= pGw3762Data_t->DataUnit[2]<<8|pGw3762Data_t->DataUnit[1];
        ana = DefSetInfo.FreqInfo.tgainana=pGw3762Data_t->DataUnit[4]<<8|pGw3762Data_t->DataUnit[3];
        DefwriteFg.FREQ = TRUE;
        DefaultSettingFlag = TRUE;
        tgain=dig|(ana<<16);
        app_printf( "tgain %d  tgain_dig: %d tgain_ana %d\n",tgain,dig,ana);

	 	net_nnib_ioctl(NET_SET_BAND,&DefSetInfo.FreqInfo.FreqBand);  
		nnib.TGAINDIG = DefSetInfo.FreqInfo.tgaindig;
		nnib.TGAINANA = DefSetInfo.FreqInfo.tgainana;
        app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);            
    }
    else if(pGw3762Data_t->DataUnit[0]==2)
    {
/************************注意信标中开启功率配置条目，clearNIB会关闭多余条目*******************************/
        nnib.FormationSeqNumber++;
        nnib.powerlevelSet =TRUE;
        nnib.TGAINDIG = pGw3762Data_t->DataUnit[2]<<8|pGw3762Data_t->DataUnit[1];
        nnib.TGAINANA=  pGw3762Data_t->DataUnit[4]<<8|pGw3762Data_t->DataUnit[3];
        app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);       
    }
    else
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port);
    }
    app_printf("SET TGAIN : %s", pGw3762Data_t->DataUnit[0] ==	1?"CCO":
				pGw3762Data_t->DataUnit[0] ==	2?"STA":
                                                            "UNKOWN");
                                                            app_printf("nnib.TGAINDIG = %d ,nnib.TGAINANA = %d\n",nnib.TGAINDIG,nnib.TGAINANA);
}

static void app_gw3762_afnf0_f10_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f10_debug-----\n");
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  default_addr[6]={0,0,0,0,0,0};
	U8  device_addr[6]={0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    U16  jj = 0;
	U16  Tei;
	DEVICE_TEI_LIST_t DeviceListTemp;
	static U8  start_1_flag=1;
	U8  *pnum=NULL;
	
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}	
	
	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFNF0;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F10);
    Gw3762updata.DataUnitLen			= 2 + 2 + 1;
    pGW3762updataunit[0]	     = (U8)(FlashInfo_t.usJZQRecordNum & 0xFF);
    pGW3762updataunit[1]          = (U8)(FlashInfo_t.usJZQRecordNum >> 8);
	pGW3762updataunit[2]	     = (U8)(usStartIdx & 0xFF);
    pGW3762updataunit[3]          = (U8)(usStartIdx >> 8);
    pGW3762updataunit[4]  	      = 0;//ucReadNum;
    pnum = &pGW3762updataunit[4];
    pGW3762updataunit             += 5;
	if(ucReadNum == 0)//查询数量为0
	{
		goto send;
	}
	
	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	
    WHLPTST //共享存储内容保护
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		if(0==memcmp(WhiteMacAddrList[jj].MacAddr,default_addr,6))//无效地址
		{
			continue;
		}
		
		if(ii == usStartIdx)//当前为读取的起始位置
		{
			
			__memcpy(pGW3762updataunit, WhiteMacAddrList[jj].MacAddr, 6); //6字节表地址
			pGW3762updataunit += 6;
			__memcpy(pGW3762updataunit, WhiteMacAddrList[jj].CnmAddr, 6); //6字节通信地址
			pGW3762updataunit += 6;
			__memcpy(pGW3762updataunit, (U8 *)&f1f1infolist[jj], 1); //1字节
			pGW3762updataunit += 1;
			__memcpy(pGW3762updataunit, WhiteMaCJQMapList[jj].CJQAddr, 6); //6字节地址
			pGW3762updataunit += 6;
			__memcpy(device_addr,WhiteMacAddrList[jj].CnmAddr, 6);
			ChangeMacAddrOrder(device_addr);
			if(DeviceList_ioctl(DEV_GET_ALL_BYMAC,device_addr, &DeviceListTemp) )
            {         
			    Tei = DeviceListTemp.NodeTEI;
            }
            else
            {
                Tei = 0;
            }
			__memcpy(pGW3762updataunit, (U8 *)&Tei, 2); //6字节地址
			pGW3762updataunit += 2;
			
	        Gw3762updata.DataUnitLen	+= 21;	
			*pnum += 1;

			
				
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
    WHLPTED//释放共享存储内容保护
    
	if(*pnum == 0)
	{
		g_QueryLastRecordTimes++;
		if(g_QueryLastRecordTimes >= 2)
		{
			g_QueryLastRecordTimes = 0;
			goto send;
		}
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
	send:
	app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afnf0_f11_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f11_debug-----\n");
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  MesgType = 0;
	U8  default_addr[6]={0,0,0,0,0,0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    U16  jj = 0;
	DEVICE_TEI_LIST_t DeviceListTemp;
	static U8  start_1_flag=1;
	U8  *pnum=NULL;
	U16 NodeTEI;
	MesgType = pGw3762Data_t->DataUnit[0];
    usStartIdx = (U16)pGw3762Data_t->DataUnit[1];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[2] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[3];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}    	
	
	app_printf("MesgType = %d\n",MesgType);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFNF0;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F11);
    Gw3762updata.DataUnitLen			= 1 + 2 + 2 + 1;
	pGW3762updataunit[0]	     = MesgType;
    pGW3762updataunit[1]	     = (U8)(APP_GETDEVNUM() & 0xFF);
    pGW3762updataunit[2]         = (U8)(APP_GETDEVNUM() >> 8);
	pGW3762updataunit[3]	     = (U8)(usStartIdx & 0xFF);
    pGW3762updataunit[4]         = (U8)(usStartIdx >> 8);
    pGW3762updataunit[5]  	     = 0;//ucReadNum;
    pnum = &pGW3762updataunit[5];
    pGW3762updataunit             += 6;
	if(ucReadNum == 0)//查询数量为0
	{
		goto send;
	}
	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	
    WHLPTST //共享存储内容保护
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj,&DeviceListTemp);
		
        if(((IS_BROADCAST_TEI(DeviceListTemp.NodeTEI))&&(MesgType == UOGRADE_TASK||MesgType == REGISTER_TASK||MesgType == BITMAP_OFF_TASK))||
			((MesgType == ADDR_OFF_TASK)&&(memcmp(power_addr_list[jj].MacAddr,default_addr,6)==0)&&(jj<POWERNUM)))//无效TEI
		{
			continue;
		}
		//app_printf("DeviceListTemp.NodeTEI = %d\n",DeviceListTemp.NodeTEI);
		if(ii == usStartIdx)//当前为读取的起始位置
		{
			NodeTEI = DeviceListTemp.NodeTEI;
			if(MesgType == UOGRADE_TASK)
			{
				
				__memcpy(pGW3762updataunit, &NodeTEI, 2); //2字节TEI
				pGW3762updataunit += 2;
				//__memcpy(pGW3762updataunit ,(U8 *)&CcoUpgradPoll[jj],2); //
                upgrade_cco_get_poll_info(jj, (CCO_UPGRADE_POLL *)pGW3762updataunit);
				pGW3762updataunit += 2;
				Gw3762updata.DataUnitLen	+= 4;	
			}
			else if(MesgType == REGISTER_TASK)
			{
				__memcpy(pGW3762updataunit, &NodeTEI, 2); //2字节TEI
				pGW3762updataunit += 2;
				__memcpy(pGW3762updataunit , (U8 *)&CcoRegPolling[jj],1); //
				pGW3762updataunit += 2;
				Gw3762updata.DataUnitLen	+= 4;	
			}
			else if(MesgType == BITMAP_OFF_TASK)
			{
				__memcpy(pGW3762updataunit, &NodeTEI, 2); //2字节TEI
				pGW3762updataunit += 2;
				__memcpy(pGW3762updataunit, (U8 *)&power_off_bitmap_list[jj], 1); //
				pGW3762updataunit += 1;
				__memcpy(pGW3762updataunit, (U8 *)&power_on_bitmap_list[jj], 1);
				pGW3762updataunit += 1;
				Gw3762updata.DataUnitLen	+= 4;	
			}
			else if(MesgType == ADDR_OFF_TASK)
			{
				if(jj>=POWERNUM)
				{
					app_printf("jj>=POWERNUM,jj=%d  \n",jj);
					WHLPTED//释放共享存储内容保护
					goto send;
				}
				
				__memcpy(pGW3762updataunit, (U8 *)&power_addr_list[jj], 6); //
				pGW3762updataunit += 6;
                __memcpy(pGW3762updataunit++, (U8 *)&power_off_addr_list[jj], 1); //
                __memcpy(pGW3762updataunit++, (U8 *)&power_on_addr_list[jj], 1); //
				Gw3762updata.DataUnitLen	+= 8;	//地址模式8字节
			}
							        
			*pnum += 1;
							
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}		
	}
	
    WHLPTED//释放共享存储内容保护
    
	if(*pnum == 0)
	{
		g_QueryLastRecordTimes++;
		if(g_QueryLastRecordTimes >= 2)
		{
			g_QueryLastRecordTimes = 0;
			goto send;
		}
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
	send:
	app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afnf0_f12_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f12_debug-----\n");
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
	U16 ii = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    //获取系统当前时间
    SysDate_t SysDate = {0};
    
	GetBinTime(&SysDate);
    bin_to_bcd((U8 *)&SysDate,(U8 *)&SysDate,sizeof(SysDate_t));
	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFNF0;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F12);
	Gw3762updata.DataUnitLen			= 0;

    __memcpy(pGW3762updataunit, (U8 *)&SysDate, sizeof(SysDate_t)); //
    pGW3762updataunit += 6;
	__memcpy(pGW3762updataunit, (U8 *)&SystenRunTime, 4); //
	pGW3762updataunit += 4;
	
	app_printf("SystenRunTime: %d DAY %d HOUR %d MIN %d SEC;Clock now [ %d-%d-%d	%d:%d:%d ]\n",
		SystenRunTime/(24*3600),(SystenRunTime%(24*3600))/3600,((SystenRunTime%(24*3600))%3600)/60,(((SystenRunTime%(24*3600))%3600)%60)%60,
		SysDate.Year+2000,SysDate.Mon,SysDate.Day,SysDate.Hour,SysDate.Min,SysDate.Sec);
	for(ii = 0;ii < 30;ii ++)
	{
		*pGW3762updataunit = memcmp(record_time_t.s[ii],"ZC-a-down",9)==0?0:
			memcmp(record_time_t.s[ii],"ZC-b-down",9)==0?1:
				memcmp(record_time_t.s[ii],"ZC-c-down",9)==0?2:
					memcmp(record_time_t.s[ii],"a",9)==0?3:
						memcmp(record_time_t.s[ii],"b",9)==0?4:5;
		pGW3762updataunit += 1;
	}
	Gw3762updata.DataUnitLen	+= 40;	//地址模式8字节，需要补齐4字节

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afnf0_f13_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f13_debug-----\n");
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
	U8 DataLen = 0;
	
	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFNF0;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F13);
	Gw3762updata.DataUnitLen			= 0;
	
	
    get_NNIB_info(pGW3762updataunit,&DataLen);
    dump_buf(pGW3762updataunit,DataLen);
	Gw3762updata.DataUnitLen	+= DataLen;	//

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static U16 encode_f0_f14_data_unit(U8 *pData, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *nodeInfo)
{
    APPGW3762AFNF014UP_UNIT_t *pUnit = (APPGW3762AFNF014UP_UNIT_t *)pData;

    __memcpy(pUnit->MacAddr, nodeInfo->MacAddr, 6);
    pUnit->SNID                       = nodeInfo->SNID;
    pUnit->NodeTEI                    = nodeInfo->NodeTEI;
    pUnit->NodeType                   = nodeInfo->NodeType;
    pUnit->NodeDepth                  = nodeInfo->NodeDepth;
    pUnit->Relation                   = nodeInfo->Relation;
    pUnit->Phase                      = nodeInfo->Phase;
    pUnit->BKRouteFg	              = nodeInfo->BKRouteFg;
    pUnit->rgain                      = (S8)(nodeInfo->HplcInfo.GAIN/nodeInfo->HplcInfo.RecvCount);
    pUnit->UplinkSuccRatio            = nodeInfo->HplcInfo.UplinkSuccRatio;
    pUnit->DownlinkSuccRatio          = nodeInfo->HplcInfo.DownlinkSuccRatio;
    pUnit->My_REV                     = nodeInfo->HplcInfo.My_REV;
    pUnit->PCO_SED                    = nodeInfo->HplcInfo.PCO_SED;
    pUnit->PCO_REV                    = nodeInfo->HplcInfo.PCO_REV;
    pUnit->ThisPeriod_REV             = nodeInfo->HplcInfo.ThisPeriod_REV;
    pUnit->PerRoutePeriodRemainTime   = 0;
    pUnit->RemainLifeTime             = nodeInfo->RemainLifeTime;
    pUnit->ResetTimes	              = nodeInfo->ResetTimes;
    pUnit->LastRst	                  = nodeInfo->LastRst;

    return 0;
}

static void app_gw3762_afnf0_f14_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f14_debug-----\n");
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
	U16 Gw3762SendDataLen = 0;
	APPGW3762DATA_t Gw3762updata;
	U8 *pGW3762updataunit = Gw3762updata.DataUnit;
	U8	ucReadNum = 0;
    //U8	default_addr[6]={0,0,0,0,0,0};
	U16 usStartIdx = 0;
    U16  ii = 0;
	U16  all_num = 0;
	
	static U8  start_1_flag=1;
	U8	*pnum=NULL;
	
	usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
	usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
	ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	
	app_printf("usStartIdx = %d\n",usStartIdx);

	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode	= pGw3762Data_t->CtrlField.TransMode;
	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum		= 0;
	Gw3762updata.Afn				= APP_GW3762_AFNF0;
	Gw3762updata.Fn 			= app_gw3762_fn_bin_to_bs(APP_GW3762_F14);
	Gw3762updata.DataUnitLen			= 2 + 2 + 1;
	
	all_num = GetNeighborCount();
	pGW3762updataunit[0]		  = (U8)(all_num & 0xFF);
	pGW3762updataunit[1]		  = (U8)(all_num >> 8);
	pGW3762updataunit[2]		  = (U8)(usStartIdx & 0xFF);
	pGW3762updataunit[3]		  = (U8)(usStartIdx >> 8);
	pGW3762updataunit[4]		  = 0;//ucReadNum;
	pnum = &pGW3762updataunit[4];
	pGW3762updataunit			  += 5;

	if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	if(ucReadNum == 0)//查询数量为0
	{
		goto send;
	}

    U8 Nullmac[6]={0};
    U16 i,nr;
    list_head_t *pos;

    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;
    nr = NeighborDiscoveryHeader.nr;
    pos= NeighborDiscoveryHeader.link.next;

    for(i=0;i<nr;i++)
    {
        if(pos == &NeighborDiscoveryHeader.link)break;
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        pos = pos->next;
        if(memcmp(DiscvryTable->MacAddr,Nullmac,6) ==0)
        {
            continue;
        }

        if(ii == usStartIdx)//当前为读取的起始位置
        {
            encode_f0_f14_data_unit(pGW3762updataunit, DiscvryTable);
            pGW3762updataunit += 32;
            Gw3762updata.DataUnitLen	+= 32;
            *pnum += 1;
            if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
            {
                goto send;
            }
        }
        else if(ii < usStartIdx)//已读过的地址
        {
            ii++;//查找当前读取到的地址位置
        }
    }

	
	if(*pnum == 0)
	{
		g_QueryLastRecordTimes++;
		if(g_QueryLastRecordTimes >= 2)
		{
			g_QueryLastRecordTimes = 0;
			goto send;
		}
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读 
		return; 
	}
	send:
	app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
	app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afnf0_f15_debug(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("-app_gw3762_afnf0_f15_debug-----\n");
    {
	    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
	    U16 Gw3762SendDataLen = 0;

	    APPGW3762DATA_t Gw3762updata;
	    Gw3762updata.CtrlField.TransDir 		= APP_GW3762_UP_DIR;
	    Gw3762updata.CtrlField.StartFlag 		= APP_GW3762_SLAVE_PRM;
	    Gw3762updata.CtrlField.TransMode 		= AppGw3762DownData.CtrlField.TransMode;
	    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
	    Gw3762updata.AddrFieldNum 			= 0;
	    Gw3762updata.Afn					= APP_GW3762_AFNF0;
	    Gw3762updata.Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F15);
	    Gw3762updata.DataUnitLen				= sizeof(DCR_MANUFACTURER_t);

	    //__memcpy(Gw3762updata.DataUnit, (U8 *) & (FlashInfo_t.ManuFactor_t), sizeof(DCR_MANUFACTURER_t));
	    Gw3762updata.DataUnit[0] = Vender2;
	    Gw3762updata.DataUnit[1] = Vender1;
	    Gw3762updata.DataUnit[2] = ZC3951CCO_chip2;
	    Gw3762updata.DataUnit[3] = ZC3951CCO_chip1;
	    Gw3762updata.DataUnit[4] = U8TOBCD(InnerDate_D);
	    Gw3762updata.DataUnit[5] = U8TOBCD(InnerDate_M);
	    Gw3762updata.DataUnit[6] = U8TOBCD(InnerDate_Y);
	    Gw3762updata.DataUnit[7] = ZC3951CCO_Innerver2;
	    Gw3762updata.DataUnit[8] = ZC3951CCO_Innerver1;

	    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
		app_printf("Gw3762SendDataLen = %d\n",Gw3762SendDataLen);
		dump_buf(Gw3762SendData, Gw3762SendDataLen);
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
	}
}

static void app_gw3762_afn05_f6_set_zone_status(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("pGw3762Data_t->DataUnit=%d\n",*(pGw3762Data_t->DataUnit));
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);

	if( *(pGw3762Data_t->DataUnit) == e_3762ZONE_DISTINGUISH_STOP)//关闭台区识别
	{
		AppGw3762Afn10F4State.WorkSwitchZone = FALSE;
		//start_get_line_state_timer();
		cco_area_ind_stop();
	}
	else //开启台区识别
	{
		AppGw3762Afn10F4State.WorkSwitchZone = TRUE;
		//Final_result_t.PublicResult_t[0].CalculateMaxNum = MAX_CACULATE;
		//Final_result_t.PublicResult_t[0].CalculateNum = 0;
		//stop_get_line_state_timer();
		//extern void indentification_task_cfg(uint8_t mode, uint8_t plan);
		app_printf("app_gw3762_afn05_f6_set_zone_status start\n");
        cco_area_ind_start();
		//indentification_task_cfg(e_DISTRIBUT,period_bit);
        //DeviceList_ioctl(DEV_RESET_AERE,NULL, NULL);
        return;
	}
}
    
//使能新增用户上报接口，0：禁止  1：允许
static void app_gw3762_afn05_f200_set_report_new_reg(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("Afn05F200 : pGw3762Data_t->DataUnit=%d\n",*(pGw3762Data_t->DataUnit));
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    if( *(pGw3762Data_t->DataUnit) == 1)//开启新增用户事件上报
    {
        area_change_set_report_flag(TRUE);
    }
    else
    {
        area_change_set_report_flag(FALSE);
    }
}

ZONE_DISTINGUISH_STATUS_e    ZoneDistinguishStatus =e_3762ZONE_DISTINGUISH_STOP;//e_3762ZONE_DISTINGUISH_STARTUP0203H;
static void app_gw3762_afn05_f130_set_zone_status(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	//U16 i=0;
	*(pGw3762Data_t->DataUnit)  = ( *(pGw3762Data_t->DataUnit) <= e_3762ZONE_DISTINGUISH_STARTUP010203H) ? *(pGw3762Data_t->DataUnit) : e_3762ZONE_DISTINGUISH_STOP;
	
	app_printf("pGw3762Data_t->DataUnit=%d\n",*(pGw3762Data_t->DataUnit));
    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
	//return;//关闭台区识别三圣测试

	if( *(pGw3762Data_t->DataUnit) == e_3762ZONE_DISTINGUISH_STOP)
	{
	    return;
	    //ReportMeterIndex = 0;
        //SystemState_e = e_EVENT_REPORT;
		app_printf("send EVT_ZONE_DISTINGUSH_02TYPE--\n");
		//FlowEvent.event  =0;
        //osh_event_send(&FlowEvent, EVT_ZONE_DISTINGUSH_02TYPE, 50); 
		//timer_stop(g_ZoneAreaOverTimer,TMR_NULL);
	}
	else 
	{
		nnib.FormationSeqNumber++;
		sysflag=TRUE;
        mutex_post(&mutexSch_t);
		/*do
		{
	    	memset(&DeviceTEIList[i], 0xff, sizeof(DEVICE_TEI_LIST_t));
		}while(++i<MAX_WHITELIST_NUM);
		
		nnib.PCOCount =0;
		nnib.discoverSTACount=0;*/
		//timer_start(g_ZoneAreaOverTimer);
	}
	ZoneDistinguishStatus =  *(pGw3762Data_t->DataUnit);
	
	//EventReportStatus = e_3762EVENT_REPORT_FORBIDEN;
    AppGw3762Afn10F4State.BJZoneFinish = TRUE; //    
}
#endif

#if ((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))

 /**
 * @brief	本地协议上行确认帧
 * @param	AppGw3762CmdState_e     cmdstate			命令状态
 * @param	AppGw3762ChanState_e    chanstate			信道状态
 * @param	U16                     waittime			等待时间
 * @param   MESG_PORT_e             port                消息端口
 * @return	void
 *******************************************************************************/
void app_gw3762_up_afn00_f1_sure(AppGw3762CmdState_e cmdstate, AppGw3762ChanState_e chanstate, U16 waittime, MESG_PORT_e port)
{
    U32 state = 0;
    U16 Gw3762SendDataLen = 0;
    U8  byLen = 0;
	waittime = 2;

    if(APP_GW3762_IDLE_CHANSTATE == chanstate)
    {
        state = 0xFFFFFFFF;
    }

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN00;
    Gw3762updata.Fn						= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);

    state = state << 1;
    state |= (cmdstate & 0x01);

    if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
    {
        Gw3762updata.DataUnitLen		= 6;

        __memcpy(&Gw3762updata.DataUnit[byLen], &state, 4);
        byLen += 4;
        __memcpy(&Gw3762updata.DataUnit[byLen], &waittime, 2);
        byLen += 2;
        //*(U32*)(Gw3762updata.DataUnit)	= (U32)(((U32)state << 1) | ((U8)cmdstate & 0x01));
        //*(U16*)(Gw3762updata.DataUnit + 4)	= waittime;
    }
    else
    {
        Gw3762updata.DataUnitLen		= 4;

        __memcpy(&Gw3762updata.DataUnit[byLen], &state, 2);
        byLen += 2;
        __memcpy(&Gw3762updata.DataUnit[byLen], &waittime, 2);
        byLen += 2;
        //*(U32*)(Gw3762updata.DataUnit)	= (U16)(((U16)state << 1) | ((U8)cmdstate & 0x01));
        //*(U16*)(Gw3762updata.DataUnit + 2)	= waittime;
    }

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);

	//printf_s("app_gw3762_up_afn00_f1_sure\n");
	
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN00);
}

 /**
 * @brief	本地协议上行否认帧
 * @param	AppGw3762ErrCode_e      errcode			    错误代码
 * @param   MESG_PORT_e             port                消息端口
 * @return	void
 *******************************************************************************/
void app_gw3762_up_afn00_f2_deny(AppGw3762ErrCode_e errcode, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;

    APPGW3762DATA_t gw3762updata;

    gw3762updata.CtrlField.TransDir 		= APP_GW3762_UP_DIR;
    gw3762updata.CtrlField.StartFlag 		= APP_GW3762_SLAVE_PRM;
    gw3762updata.CtrlField.TransMode 		= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    gw3762updata.AddrFieldNum 			= 0;
    gw3762updata.Afn					= APP_GW3762_AFN00;
    gw3762updata.Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F2);

    gw3762updata.DataUnitLen			= 1;
    *(gw3762updata.DataUnit)			= errcode;

    app_gw3762_up_frame_encode(&gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN00);
    return;
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn00_f2_deny_by_seq(AppGw3762ErrCode_e errcode,  U8 Seq,MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN00F2上行处理
 * 参数说明	: 	errcode	- 错误代码
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn00_f2_deny_by_seq(AppGw3762ErrCode_e errcode, U8 Seq, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;

    APPGW3762DATA_t gw3762updata;

    gw3762updata.CtrlField.TransDir 		= APP_GW3762_UP_DIR;
    gw3762updata.CtrlField.StartFlag 		= APP_GW3762_SLAVE_PRM;
    gw3762updata.CtrlField.TransMode 		= AppGw3762DownData.CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(gw3762updata.UpInfoField), 0,NULL,Seq);
    gw3762updata.AddrFieldNum 			= 0;
    gw3762updata.Afn					= APP_GW3762_AFN00;
    gw3762updata.Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F2);

    gw3762updata.DataUnitLen			= 1;
    *(gw3762updata.DataUnit)			= errcode;

    app_gw3762_up_frame_encode(&gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, Seq, FALSE, 0, port,e_Serial_AFN00);
    return;
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn03_f4_master_addr(U8 *addr, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN03F4上行处理函数
 * 参数说明	: 	addr	- 主节点地址
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn03_f4_master_addr(U8 *addr, MESG_PORT_e port)
//{
//}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn03_f10_master_mode(U8 mode)
 * 函数说明	: 	GW3762 AFN03F10上行处理
 * 参数说明	: 	无
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn03_f10_master_mode(U8 mode, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *gw3762updataunit				= Gw3762updata.DataUnit;
    APPGW3762AFN03F10_t stAFN03F10		= {0};
    Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    U8  active;
    U8  seqNum;

    if(FALSE == mode)
    {
        Gw3762updata.CtrlField.StartFlag = APP_GW3762_MASTER_PRM;
        app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField),NULL);

        active = TRUE;
        seqNum = AppPIB_t.AppGw3762UpInfoFrameNum;
    }

    else
    {
        Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
        app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);

        active = FALSE;
        seqNum = AppGw3762DownData.DownInfoField.FrameNum;
    }

    Gw3762updata.AddrFieldNum = 0;
    Gw3762updata.Afn = APP_GW3762_AFN03;
    Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(APP_GW3762_F10);
    if(APP_GW3762_TRANS_MODE == APP_GW3762_HPLC_HRF_CTRL_COMMMODE)
    {
        stAFN03F10.ucCommMode = APP_GW3762_HPLC_HRF_COMMMODE;
    }
    else
    {
        stAFN03F10.ucCommMode = APP_GW3762_BC_COMMMODE;
    }
    stAFN03F10.ucRouterManage = APP_GW3762_Y_ROUTERMANAGE;
    stAFN03F10.ucSlaveInfo = APP_GW3762_Y_SLAVEINFO;
    stAFN03F10.ucCycleMeter = APP_GW3762_D_CYCLEMETER;//  APP_GW3762_D_CYCLEMETER
    stAFN03F10.ucRouterTransDelay = APP_GW3762_Y_TRANSDELAY;
    stAFN03F10.ucSlaveTransDelay = APP_GW3762_Y_TRANSDELAY;
    stAFN03F10.ucBroadTransDelay = APP_GW3762_Y_TRANSDELAY;
    stAFN03F10.ucFailChange = APP_GW3762_R_FAILCHANGE;
    stAFN03F10.ucBroadConfirm = APP_GW3762_BEFORE_BCCONFIRM;
    stAFN03F10.ucBroadExecute = APP_GW3762_N_BCEXECUTE;
    stAFN03F10.ucChannelNum = 1;
    stAFN03F10.ucALowVoltage = APP_GW3762_N_LOWVOL;
    stAFN03F10.ucBLowVoltage = APP_GW3762_N_LOWVOL;
    stAFN03F10.ucCLowVoltage = APP_GW3762_N_LOWVOL;
    stAFN03F10.ucRateNum = APP_GW3762_SPEED_NUM;
    stAFN03F10.ucMinuteCollect = app_ext_info.func_switch.MinCollectSWC;
    stAFN03F10.ucReserve1 = 0;
    stAFN03F10.ucReserve2 = 0;
    stAFN03F10.ucReserve3 = 0;
    stAFN03F10.ucSlaveReadTimeout	= FlashInfo_t.ucSlaveReadTimeout<3?60:FlashInfo_t.ucSlaveReadTimeout;
    stAFN03F10.usBroadCastTimeout = APP_GW3762_BROADCAST_TIMEOUT;
    stAFN03F10.usMsgMaxLen = APP_GW3762_DATA_UNIT_LEN;
    stAFN03F10.usForwardMsgMaxLen = APP_GW3762_FILETRANS_PACK_MAX;
    stAFN03F10.ucUpdateTime = APP_GW3762_UPDATE_WAITTIME;

    __memcpy(stAFN03F10.ucMainAddr, FlashInfo_t.ucJZQMacAddr, 6);
    stAFN03F10.usNodeMaxNum = APP_GW3762_METER_MAX;

    stAFN03F10.usCurNodeNum = FlashInfo_t.usJZQRecordNum;
    stAFN03F10.ucProtoDate[0] = 0x13;
    stAFN03F10.ucProtoDate[1] = 0x03;
    stAFN03F10.ucProtoDate[2] = 0x21;
    stAFN03F10.ucProtoRecordDate[0] = 0x13;
    stAFN03F10.ucProtoRecordDate[1] = 0x03;
    stAFN03F10.ucProtoRecordDate[2] = 0x21;
    __memcpy(&stAFN03F10.ManuFactor_t, &FlashInfo_t.ManuFactor_t, sizeof(DCR_MANUFACTURER_t));
    if (PROVINCE_JIANGSU == app_ext_info.province_code) //todo: PROVINCE_JIANGSU
    {
        stAFN03F10.BaudSpeed            = BaudInfo.usBaudSpeed;
        stAFN03F10.BaudUnit             = BaudInfo.usBaudUnit;
    }
    else
    {
#if defined(STD_DUAL)
        stAFN03F10.BaudSpeed = 0;        //通信速率，双模建议为0
        stAFN03F10.BaudUnit = 0;        //速率单位标识，双模建议为0
#else
        stAFN03F10.BaudSpeed			= FlashInfo_t.WorkMode_t.usBaudSpeed;
        stAFN03F10.BaudUnit				= FlashInfo_t.WorkMode_t.usBaudUnit;
#endif
    }
    
    Gw3762updata.DataUnitLen	= sizeof(APPGW3762AFN03F10_t);
    __memcpy(gw3762updataunit, (U8 *)(&stAFN03F10), sizeof(APPGW3762AFN03F10_t));
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
	app_printf("app_gw3762_up_afn03_f10_master_mode! len : %d\n",Gw3762SendDataLen);

    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, seqNum, active, 0, port,e_Serial_AFN10XX);
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn06_f1_report_slave_info(U8 num, AppGw3762SlaveInfo_t *slaveinfo, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN06F1上行处理
 * 参数说明	: 	num		-上报从节点数量
 *					slaveinfo	-从节点信息指针
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn06_f1_report_slave_info(U8 num, AppGw3762SlaveInfo_t *slaveinfo, MESG_PORT_e port)
//{
//
//}

/*************************************************************************
 * 函数名称	: 	app_gw3762_up_afn06_f2_report_read_data
 * 函数说明	: 	GW3762 AFN06F2上行处理
 * 参数说明	: 	index	- 从节点序号
 *					proto	- 通信协议类型
 *					time		- 当前报文本地通信上行时长
 *					len		- 报文长度
 *					data		- 报文内容
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn06_f2_report_read_data(U8 *Addr, U16 meterIndex, AppProtoType_e proto, U16 time, U8 *data, U16 len, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    U16  byLen = 0;
    APPGW3762DATA_t Gw3762updata;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_MASTER_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;
	//U8 phase = get_phase_Devicelist(pAppGw3762Afn06F2Up->Addr);
    app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField),Addr);
    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;

    __memcpy(Gw3762updata.AddrField.SrcAddr, Addr, MAC_ADDR_LEN);
    __memcpy(Gw3762updata.AddrField.DestAddr, FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN);

    Gw3762updata.Afn				= APP_GW3762_AFN06;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F2);

    //if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
   
    Gw3762updata.DataUnit[byLen++] = (U8)(meterIndex & 0xFF);
    Gw3762updata.DataUnit[byLen++] = (U8)(meterIndex >> 8);
    Gw3762updata.DataUnit[byLen++] = proto;
	if(APP_JZQ_TYPE_13 == FlashInfo_t.ucJZQType)
	{
    	Gw3762updata.DataUnit[byLen++] = (U8)(time & 0xFF);
    	Gw3762updata.DataUnit[byLen++] = (U8)(time >> 8);
	}
    Gw3762updata.DataUnit[byLen++] = len;
    __memcpy(&Gw3762updata.DataUnit[byLen], data, len);
    byLen += len;
    Gw3762updata.DataUnitLen		= byLen;
   
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
   
    if(PROVINCE_CHONGQING == app_ext_info.province_code)
    {
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, RETRY_TIMES, port, e_Serial_AFN06XX);
    }
    else
    {
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, 0, port, e_Serial_AFN06XX);
    }
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn06_f3_report_router_change(AppGw3762TaskChange_e taskchange, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN06F3上行处理
 * 参数说明	: 	taskchange	-路由工作任务变动类型
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn06_f3_report_router_change(AppGw3762TaskChange_e taskchange, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    U16  byLen = 0;
    APPGW3762DATA_t Gw3762updata;
    //SlaRegInfo_t.SlaveRegFlag = FALSE;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_MASTER_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField),NULL);
    Gw3762updata.AddrFieldNum 		= 0;

    Gw3762updata.Afn				= APP_GW3762_AFN06;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F3);

    Gw3762updata.DataUnit[byLen++] = taskchange;    

    Gw3762updata.DataUnitLen = byLen;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    if(PROVINCE_CHONGQING == app_ext_info.province_code)
    {
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, RETRY_TIMES, port, e_Serial_AFN06XX);
    }
    else
    {
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, 0, port, e_Serial_AFN06XX);
    }
}


/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn06_f4_report_dev_type(U8 num, U8 *addr, AppProtoType_e proto,
 *	U16 index, AppGw3762SlaveType_e devtype, U8 totalnum, U8 curnum, AppGw3762SubInfo_t *subinfo, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN06F4上行处理
 * 参数说明	: 	num		-上报从节点数量
 *					addr		-从节点1通信地址
 *					proto	-从节点1通信协议类型
 *					index	-从节点1序号
 *					devtype	-从节点1设备类型
 *					totalnum	-从节点1下接从节点数量
 *					curnum	-本报文传输的节点数量
 *					subinfo	-下接从节点信息
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn06_f4_report_dev_type(U8 num, U8 *addr, AppProtoType_e proto,
                                     U16 index, AppGw3762SlaveType_e devtype, U8 totalnum, U8 curnum, AppGw3762SubInfo_t *subinfo, MESG_PORT_e port)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    U16  byLen = 0;
    U8 retry_times = 0;
    APPGW3762DATA_t Gw3762updata;
   
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_MASTER_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;
	//U8 phase = get_phase_Devicelist(addr);
    app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField),addr);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.UpInfoField.ModuleFlag = 0;
    /*
    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;

    __memcpy(Gw3762updata.AddrField.SrcAddr, addr, MAC_ADDR_LEN);
    __memcpy(Gw3762updata.AddrField.DestAddr, FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN);
    */

    Gw3762updata.Afn				= APP_GW3762_AFN06;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F4);

    Gw3762updata.DataUnit[byLen++] = num;
    __memcpy(&Gw3762updata.DataUnit[byLen], addr, 6);
    byLen += 6;

    Gw3762updata.DataUnit[byLen++] = proto;
    Gw3762updata.DataUnit[byLen++] = (U8)(index & 0xFF);
    Gw3762updata.DataUnit[byLen++] = (U8)(index >> 8);
	if(devtype  == 0) //aps层定义和APP冲突
	{
		devtype = APP_GW3762_M_SLAVETYPE;
		Gw3762updata.DataUnit[byLen++] = devtype;
		Gw3762updata.DataUnit[byLen++] = 0;
    	Gw3762updata.DataUnit[byLen++] = 0;
	}
	else if(devtype  == 1 || devtype  == 2)
	{
		devtype = APP_GW3762_C_SLAVETYPE;
		Gw3762updata.DataUnit[byLen++] = devtype;
		Gw3762updata.DataUnit[byLen++] = totalnum;
    	Gw3762updata.DataUnit[byLen++] = curnum;
		__memcpy(&Gw3762updata.DataUnit[byLen], (U8*)subinfo, curnum*sizeof(AppGw3762SubInfo_t));
    	byLen += curnum*sizeof(AppGw3762SubInfo_t); 
	}


	//甘肃即装即采使用
	if(NewMeterReport == TRUE)
	{
		if( check_whitelist_by_addr(addr) )
		{
			return;
		}
	}

    
   // __memcpy(&Gw3762updata.DataUnit[byLen], (U8*)subinfo, curnum*sizeof(AppGw3762SubInfo_t));
   // byLen += curnum*sizeof(AppGw3762SubInfo_t);    

    Gw3762updata.DataUnitLen = byLen;
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);

    retry_times = ((PROVINCE_CHONGQING == app_ext_info.province_code)
                    || (PROVINCE_HENAN == app_ext_info.province_code))?RETRY_TIMES:0;

    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, retry_times, port,e_Serial_AFN06XX);
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn06_f5_report_slave_event(AppGw3762SlaveType_e devtype, AppProtoType_e proto, U8 len, U8 *data)
 * 函数说明	: 	GW3762 AFN06F5上行处理
 * 参数说明	: 	devtype	-从节点设备类型
 *					proto	-通信协议类型
 *					len		-报文长度
 *					data		-报文内容
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn06_f5_report_slave_event(U8 LenNull_flag,AppGw3762SlaveType_e devtype, AppProtoType_e proto, U8 *meterAddr,U16 len, U8 *data)
{
	U8	*sendbuf = zc_malloc_mem(100, "sendbuf", MEM_TIME_OUT);
	U8 nonoeventlogo[18]={0x34,0x48,0x33,0x37,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0xDD,0XDD};

	APPGW3762DATA_t *gw3762updata 		= &AppGw3762UpData;
	U8 *gw3762updataunit				= gw3762updata->DataUnit;
	//U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
       U16 Gw3762SendDataLen = 0;
    U8 retry = FALSE;

	gw3762updata->CtrlField.TransDir 		= APP_GW3762_UP_DIR;
	gw3762updata->CtrlField.StartFlag 		= APP_GW3762_MASTER_PRM;
	gw3762updata->CtrlField.TransMode = APP_GW3762_TRANS_MODE;

	app_gw3762_up_info_field_master_pack(&(gw3762updata->UpInfoField),meterAddr);

	gw3762updata->AddrFieldNum 			= 0;
	gw3762updata->Afn					= APP_GW3762_AFN06;
	gw3762updata->Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F5);

	gw3762updata->DataUnitLen			= 3 + len;
	*gw3762updataunit++					= (U8)devtype;

	if(len != 0)
	{
		*gw3762updataunit++					= (U8)proto;
		*gw3762updataunit++			= len;
		__memcpy(gw3762updataunit, data, len);
	}
	else if(len == 0 && LenNull_flag == TRUE)//如果上报事件的长度为0默认为698协议
	{	
		Packet645Frame(sendbuf,(U8*)&len,meterAddr,0x91,nonoeventlogo,sizeof(nonoeventlogo));
		*gw3762updataunit++					= 3;//协议类型
		*gw3762updataunit++	= len;
		gw3762updata->DataUnitLen += len;
		__memcpy(gw3762updataunit, sendbuf, len);
	}	
	else if(len == 0 && LenNull_flag == FALSE)//其他方式为异常
	{
	    zc_free_mem(sendbuf);
		return;
	}
	app_gw3762_up_frame_encode(gw3762updata, Gw3762SendData, &Gw3762SendDataLen);

    if(zc_timer_query(g_SendsyhtimeoutTimer) == TMR_RUNNING)
    {
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, RETRY_TIMES, e_APP_MSG,e_Serial_AFN06XX);
    }
    else
    {
        if (proto == APP_POWER_EVENT && PROVINCE_SHANGHAI == app_ext_info.province_code)
        {
            retry = TRUE;
        }
        else if (PROVINCE_CHONGQING == app_ext_info.province_code || PROVINCE_HENAN == app_ext_info.province_code)
        {
            retry = TRUE;
        }
        send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, retry, RETRY_TIMES, e_UART1_MSG,e_Serial_AFN06XX);
    }
    zc_free_mem(sendbuf);

    return;
}
#endif

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn10_f1_query_slave_num(U16 num, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F1上行处理函数
 * 参数说明	: 	num	- 从节点数量
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn10_f1_query_slave_num(U16 num, MESG_PORT_e port)
//{
//
//}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn10_f2_query_slave_info(U16 totalnum, U8 curnum, U8 *data)
 * 函数说明	: 	GW3762 AFN10F2处理函数
 * 参数说明	: 	无
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn10_f2_query_slave_info(U16 totalnum, U8 curnum, U8 *data, MESG_PORT_e port)
//{
//
//}

/*************************************************************************
 * 函数名称	: 	void AppGw3763UpAfn10F2QuerySlaveFather(U8 num, U8 *data, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F3处理函数
 * 参数说明	: 	无
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn10_f3_query_slave_father(U8 num, U8 *data, MESG_PORT_e port)
//{
//
//}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn10_f5_fail_slave_info(U8 num, U8 *data, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F5处理函数
 * 参数说明	: 	无
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn10_f5_fail_slave_info(U16 totalnum, U8 num, U8 *data, MESG_PORT_e port)
//{
//
//}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn10_f6_reg_slave_info(U8 num, U8 *data, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN10F5处理函数
 * 参数说明	: 	无
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn10_f6_reg_slave_info(U16 totalnum, U8 num, U8 *data, MESG_PORT_e port)
//{
//	
//}

static void pakect_cco_report_info(U16 Fn,U8 *Data,U16 *DataLen)
{
    U16  ByLen;

    ByLen = 0;
    __memcpy(Data, FlashInfo_t.ucJZQMacAddr, 6); //6字节地址+三字节信息
    ByLen +=6;
        
    if(Fn == APP_GW3762_F7)
    {
        //NULL
    }

    else if(Fn == APP_GW3762_F20)
    {
        APPGW3762AFN10F20_t Up10F20info;
        
        memset((U8 *)&Up10F20info,0x00,sizeof(APPGW3762AFN10F20_t));
        Up10F20info.NodeTEI = 1;
        Up10F20info.ModuleType = 1;
        Up10F20info.ParentTEI = 0;
        Up10F20info.NodeDepth = 0;
        Up10F20info.NodeType = e_CCO_NODETYPE;
        Up10F20info.LinkType = 0;
        __memcpy(&Data[ByLen],(U8 *)&Up10F20info,sizeof(APPGW3762AFN10F20_t));
        ByLen += sizeof(APPGW3762AFN10F20_t);
    }
    
    else if(Fn == APP_GW3762_F21)
    {
        APPGW3762AFN10F21_t Up10F21info;
        
        memset((U8 *)&Up10F21info,0x00,sizeof(APPGW3762AFN10F21_t));
        Up10F21info.NodeTEI = 1;
        Up10F21info.ParentTEI = 0;
        Up10F21info.NodeDepth = 0;
        Up10F21info.NodeType = e_CCO_NODETYPE;
        __memcpy(&Data[ByLen],(U8 *)&Up10F21info,sizeof(APPGW3762AFN10F21_t));
        ByLen += sizeof(APPGW3762AFN10F21_t);
    }
    
    else if(Fn == APP_GW3762_F31)
    {
        APPGW3762AFN10F31_t Up10F31info;
        
        memset((U8 *)&Up10F31info,0x00,sizeof(APPGW3762AFN10F31_t));
		Up10F31info.Phase1 = 1;
		Up10F31info.Phase2 = 1;
		Up10F31info.Phase3 = 1;
		if(gl_cco_phase_order == TRUE)
        {
			Up10F31info.order1 = 0;
            Up10F31info.LNerr = 0;
        }
		else
        {
			Up10F31info.order1 = 1;
            Up10F31info.LNerr = 1;
        }
		Up10F31info.order2 = 0;
		Up10F31info.order3 = 0;
			
        __memcpy(&Data[ByLen],(U8 *)&Up10F31info,sizeof(APPGW3762AFN10F31_t));
        ByLen += sizeof(APPGW3762AFN10F31_t);
    }
    
    else if(Fn == APP_GW3762_F104)
    {
        APPGW3762AFN10F104_t Up10F104info;
        
        memset((U8 *)&Up10F104info,0x00,sizeof(APPGW3762AFN10F104_t));
        __memcpy(Up10F104info.VersionNum,FlashInfo_t.ManuFactor_t.ucVersionNum,2);//版本信息
        Up10F104info.Day =  FlashInfo_t.ManuFactor_t.ucDay; //日
        Up10F104info.Month =  FlashInfo_t.ManuFactor_t.ucMonth;//月
        Up10F104info.Year =  FlashInfo_t.ManuFactor_t.ucYear;//年
        __memcpy(Up10F104info.VendorCode,FlashInfo_t.ManuFactor_t.ucVendorCode,2);//厂商代码
        __memcpy(Up10F104info.ChipCode,FlashInfo_t.ManuFactor_t.ucChipCode,2);//芯片代码
        __memcpy(&Data[ByLen],(U8 *)&Up10F104info,sizeof(APPGW3762AFN10F104_t));
        ByLen += sizeof(APPGW3762AFN10F104_t);
    }
    else if(Fn == APP_GW3762_F112)
    {
        APPGW3762AFN10F112_t Up10F112info;
        
        memset((U8 *)&Up10F112info,0x00,sizeof(APPGW3762AFN10F112_t));
        Up10F112info.DevType = e_JZQ;//
        if(PROVINCE_ANHUI == app_ext_info.province_code)
        {
            U8  chipID_flase[24];
            U8  chipID_null[24] = { 0 };
            memset(chipID_flase, 0xff, sizeof(chipID_flase));
            if(memcmp(ManageID,chipID_null,24) == 0)
            {
                __memcpy(Up10F112info.ChipID,chipID_flase,24);//芯片ID
            }
        }
        else
        {
            __memcpy(Up10F112info.ChipID,ManageID,24);//芯片ID
        }
        __memcpy(Up10F112info.VersionNum,FlashInfo_t.ManuFactor_t.ucVersionNum,2);//版本信息
        __memcpy(&Data[ByLen],(U8 *)&Up10F112info,sizeof(APPGW3762AFN10F112_t));
        ByLen += sizeof(APPGW3762AFN10F112_t);
    }
    (*DataLen) += ByLen;
}

static void pakect_sta_report_info(U16 Fn , DEVICE_TEI_LIST_t *pDeviceListTemp , U8 *Data , U16 *DataLen)
{
    U16  ByLen;
    U8  device_addr[6]={0};

    ByLen = 0;
    __memcpy(device_addr,pDeviceListTemp->MacAddr,6);
    ChangeMacAddrOrder(device_addr);
    __memcpy(&Data[ByLen] , device_addr , 6);
    ByLen += 6;
    
    if(Fn == APP_GW3762_F7)
    {
        U8  moduleID_false[11]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        U8  moduleId_null[11] = {0};
	    U8  moduleID_temp[11] = {0};
        U8  IDState = 0;
        APPGW3762AFN10F7_t Up10F7info;
        
        memset((U8 *)&Up10F7info,0x00,sizeof(APPGW3762AFN10F7_t));
       
		Up10F7info.DeviceType= (pDeviceListTemp->DeviceType==e_METER||pDeviceListTemp->DeviceType==e_3PMETER)?0:
			(pDeviceListTemp->DeviceType==e_CJQ_2||pDeviceListTemp->DeviceType==e_CJQ_1)?1:15;
		//__memcpy(&Data[byLen++] , (U8 *)&ReportType ,1);

        if (PROVINCE_JIANGSU == app_ext_info.province_code)//todo: PROVINCE_JIANGSU
        {
            app_printf("pDeviceListTemp->CollectNeed = %d\n", pDeviceListTemp->CollectNeed);
            if (pDeviceListTemp->CollectNeed != 1)
            {
                pDeviceListTemp->CollectNeed = 0;
            }
            Up10F7info.Res1 = pDeviceListTemp->CollectNeed;
        }

		__memcpy((U8 *)&Up10F7info.VendorCode,pDeviceListTemp->ManufactorCode,2);
        //这个判定方法必须是依据在白名单，不在白名单直接使用设备列表结果进行上报
        if(get_mode_id_by_addr(device_addr,moduleID_temp,&IDState) == FALSE)
        {
            __memcpy(moduleID_temp,pDeviceListTemp->ModeID,ModuleIDLen);
            if(pDeviceListTemp->ModeNeed == e_InitState||pDeviceListTemp->ModeNeed == e_NeedGet)
            {
                IDState = MOUDLTID_NO_UP;
            }
            else
            {
                IDState = MOUDLTID_RENEW;
            }
        }
        
		if(memcmp(moduleID_temp, moduleId_null,ModuleIDLen) == 0||memcmp(moduleID_temp, moduleID_false,ModuleIDLen) == 0)
    	{
    	    Up10F7info.Renewal = IDState;//确认STA的更新标识为实际标识
            Up10F7info.ModeIDLen = 0x01;
		    Up10F7info.ModeIDType = BIN_TYPE;
            if(IDState == MOUDLTID_RENEW)
            {
                memset(Up10F7info.ModeID , 0x00,1);//确认STA不支持模块ID
            }
            else
            {
                memset(Up10F7info.ModeID , 0xFF,1);//模块ID获取到为FF，未获得
            }
		}
		else
		{
			Up10F7info.Renewal = IDState;   //确认STA的更新标识为实际标识
			Up10F7info.ModeIDLen = ModuleIDLen;
			if(PROVINCE_JIANGSU == app_ext_info.province_code)
    		    Up10F7info.ModeIDType = BIN_TYPE;//江苏要求查询ID信息时，格式为BIN
    		else
    		    Up10F7info.ModeIDType = BCD_TYPE;
			__memcpy(Up10F7info.ModeID,moduleID_temp,ModuleIDLen);
		}
        
        __memcpy(&Data[ByLen],(U8 *)&Up10F7info,5);
        ByLen += 5;
        __memcpy(&Data[ByLen],(U8 *)&Up10F7info.ModeID,Up10F7info.ModeIDLen);
        ByLen += Up10F7info.ModeIDLen;
    }
    
    else if(Fn == APP_GW3762_F20)
    {
        APPGW3762AFN10F20_t Up10F20info;
        
        memset((U8 *)&Up10F20info,0x00,sizeof(APPGW3762AFN10F20_t));
        Up10F20info.NodeTEI = pDeviceListTemp->NodeTEI;
        Up10F20info.ModuleType = pDeviceListTemp->ModuleType;
        Up10F20info.ParentTEI = pDeviceListTemp->ParentTEI;
        Up10F20info.NodeDepth = pDeviceListTemp->NodeDepth;
        Up10F20info.NodeType = pDeviceListTemp->NodeType;
        Up10F20info.LinkType = pDeviceListTemp->LinkType;
        __memcpy(&Data[ByLen],(U8 *)&Up10F20info,sizeof(APPGW3762AFN10F20_t));
        ByLen += sizeof(APPGW3762AFN10F20_t);
    }
    
    else if(Fn == APP_GW3762_F21)
    {
        APPGW3762AFN10F21_t Up10F21info;
        
        memset((U8 *)&Up10F21info,0x00,sizeof(APPGW3762AFN10F21_t));
        Up10F21info.NodeTEI = pDeviceListTemp->NodeTEI;
        Up10F21info.ParentTEI = pDeviceListTemp->ParentTEI;
        Up10F21info.NodeDepth = pDeviceListTemp->NodeDepth;
        Up10F21info.NodeType = pDeviceListTemp->NodeType;
        __memcpy(&Data[ByLen],(U8 *)&Up10F21info,sizeof(APPGW3762AFN10F21_t));
        ByLen += sizeof(APPGW3762AFN10F21_t);
    }
    
    else if(Fn == APP_GW3762_F31)
    {
        APPGW3762AFN10F31_t Up10F31info;
        
        memset((U8 *)&Up10F31info,0x00,sizeof(APPGW3762AFN10F31_t));
        Up10F31info.MeterType = pDeviceListTemp->DeviceType==e_3PMETER? 1:0;
		if(Up10F31info.MeterType ==1)/* 三相表 */
		{
			Up10F31info.Phase1 = pDeviceListTemp->F31_D0;
			Up10F31info.Phase2 = pDeviceListTemp->F31_D1;
			Up10F31info.Phase3 = pDeviceListTemp->F31_D2;
			Up10F31info.LNerr = pDeviceListTemp->LNerr&0x01;
			Up10F31info.order1 = pDeviceListTemp->F31_D5;
			Up10F31info.order2 = pDeviceListTemp->F31_D6;
			Up10F31info.order3 = pDeviceListTemp->F31_D7;

            if(PROVINCE_HENAN == app_ext_info.province_code)//todo: PROVINCE_HENAN
            {
                if((((Up10F31info.order1 == 0 && Up10F31info.order2 == 0 && Up10F31info.order3 == 0) || (Up10F31info.order1 == 1 && Up10F31info.order2 == 1 && Up10F31info.order3 == 0)		//ABC BCA
                    || (Up10F31info.order1 == 0 && Up10F31info.order2 == 0 && Up10F31info.order3 == 1)) && (Up10F31info.Phase1 + Up10F31info.Phase2 + Up10F31info.Phase3 == 3)) 	//CAB 河南协议里写明ABC、BCA、CAB均是正相序
                    || (Up10F31info.order1 == 1 && Up10F31info.order2 == 1 && Up10F31info.order3 == 1))
                {
                    ; //三相表接线正常
                }
                else
                {
                    Up10F31info.LNerr = 1; //接线异常
                }
            }
            else
            {
			    if( ((Up10F31info.order1 == 0 && Up10F31info.order2 == 0 && Up10F31info.order3 == 0)&&(Up10F31info.Phase1+Up10F31info.Phase2+Up10F31info.Phase3 == 3)) 
				    || (Up10F31info.order1 == 1 && Up10F31info.order2 == 1 && Up10F31info.order3 == 1) )
			    {
				    ; //三相表接线正常
			    }
			    else
			    {
				     Up10F31info.LNerr = 1; //接线异常
			    }
            }

            if (PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
            {
                if(1 == (Up10F31info.Phase1 + Up10F31info.Phase2 + Up10F31info.Phase3))
                {
                    /* 如三相四线断任意两线（两个火线断或者一火一零断）等不能判断相序的情况，接入相位应判断正确，“节点相位信息”中相序信息D5~D7报“111” */
                    Up10F31info.order1 = 1;
                    Up10F31info.order2 = 1;
                    Up10F31info.order3 = 1;
                }
            }
		}
		else/* 单相表 */
		{
			if(pDeviceListTemp->Phase == e_A_PHASE)
			{
				Up10F31info.Phase1 = TRUE;
			}
			else if(pDeviceListTemp->Phase == e_B_PHASE)
			{
				Up10F31info.Phase2 = TRUE;
			}
			else if(pDeviceListTemp->Phase == e_C_PHASE)
			{
				Up10F31info.Phase3 = TRUE;
			}
			Up10F31info.LNerr = pDeviceListTemp->LNerr&0x01;
			Up10F31info.order1 = pDeviceListTemp->F31_D5;
			Up10F31info.order2 = pDeviceListTemp->F31_D6;
			Up10F31info.order3 = pDeviceListTemp->F31_D7;

            if (PROVINCE_HUNAN == app_ext_info.province_code)//todo: PROVINCE_HUNAN
            {
                /* 单相表相序仅报“ABC”和“零火反接” */
                if(1 == Up10F31info.LNerr)
                {
                    Up10F31info.order1 = 0;
                    Up10F31info.order2 = 1;
                    Up10F31info.order3 = 1;
                }
            }
		}
        __memcpy(&Data[ByLen],(U8 *)&Up10F31info,sizeof(APPGW3762AFN10F31_t));
        ByLen += sizeof(APPGW3762AFN10F31_t);
    }
    
    else if(Fn == APP_GW3762_F104)
    {
        APPGW3762AFN10F104_t Up10F104info;
        U8  reDay=0 , reMon=0,reYear=0;
        
        memset((U8 *)&Up10F104info,0x00,sizeof(APPGW3762AFN10F104_t));
        __memcpy(Up10F104info.VersionNum,pDeviceListTemp->SoftVerNum,2);//版本信息
        reDay = (pDeviceListTemp->BuildTime >> 11)&0x1F ;
        reMon = (pDeviceListTemp->BuildTime >> 7)&0x0F;
        reYear  = (pDeviceListTemp->BuildTime )&0x7F;
        //bin转换至BCD
        Up10F104info.Day =  U8TOBCD(reDay); //日
        Up10F104info.Month =  U8TOBCD(reMon);//月
        Up10F104info.Year =  U8TOBCD(reYear);//年
        __memcpy(Up10F104info.VendorCode,pDeviceListTemp->ManufactorCode,2);//厂商代码
        __memcpy(Up10F104info.ChipCode,pDeviceListTemp->ChipVerNum,2);//芯片代码
        __memcpy(&Data[ByLen],(U8 *)&Up10F104info,sizeof(APPGW3762AFN10F104_t));
        ByLen += sizeof(APPGW3762AFN10F104_t);
    }
    else if(Fn == APP_GW3762_F112)
    {
        APPGW3762AFN10F112_t Up10F112info;
        
        memset((U8 *)&Up10F112info,0x00,sizeof(APPGW3762AFN10F112_t));
        Up10F112info.DevType = pDeviceListTemp->DeviceType;//

        if(PROVINCE_ANHUI == app_ext_info.province_code) //todo: PROVINCE_ANHUI
        {
            U8  chipID_null[24] = { 0 };
            U8  chipID_flase[24];
            memset(chipID_flase, 0xff, sizeof(chipID_flase));
            if(memcmp(pDeviceListTemp->ManageID, chipID_null, 24) == 0)
            {
                __memcpy(Up10F112info.ChipID, chipID_flase, 24);
            }
            else
            {
                __memcpy(Up10F112info.ChipID, pDeviceListTemp->ManageID, 24);//芯片ID
            }
        }
        else
        {
        	__memcpy(Up10F112info.ChipID,pDeviceListTemp->ManageID,24);//芯片ID
        }
        
        __memcpy(Up10F112info.VersionNum,pDeviceListTemp->SoftVerNum,2);//版本信息
        __memcpy(&Data[ByLen],(U8 *)&Up10F112info,sizeof(APPGW3762AFN10F112_t));
        ByLen += sizeof(APPGW3762AFN10F112_t);
    }
    (*DataLen ) += ByLen;
    //app_printf("*DataLen = %d \n",*DataLen);
}

U8 devicelist_ban_type(DEVICE_TEI_LIST_t DeviceListTemp)
{
	if(DeviceListTemp.DeviceType == e_RELAY || (DeviceListTemp.DeviceType==e_CJQ_2 && DeviceListTemp.MacType!=e_MAC_METER_ADDR) \
			||	(DeviceListTemp.DeviceType==e_CJQ_1 && DeviceListTemp.MacType!=e_MAC_METER_ADDR ))
	{
		return TRUE;
	}
	return FALSE;
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn10_fn_up_frame(U8 AFN, U16 Fn, U8 TransMode, QUERY_TYPE_e port, U16 StartIndex, U8 num)
 * 函数说明	: 	GW3762 AFN14F1上行处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_up_afn10_fn_up_frame(U16 Fn, U8 TransMode, U16 StartIndex, U8 num, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    //APPGW3762DATA_t Gw3762updata;
    U16  ByLen = 0;
    U16  QueryIndex = 0;
    U16  TotalNum = 0;
    U16  ReportCCOInfoFlag = FALSE; //cco上报标志
    U16  jj = 0;
    U8   *pGW3762updataunit = Gw3762updata.DataUnit;
    U16   NumCount =0;   //上报数量偏移
    U16   ReadCount =0;  //实际有效数量,超过255
    U8   ReadNum = 0;   //本次上报数量
    U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8  device_addr[6]={0};
    DEVICE_TEI_LIST_t DeviceListTemp;
    S16 Sqe = 0;
    U8  null_addr[6] = { 0 };
    U16 LastTemp = 0;
    
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(Fn);
    //确认查询从模块起始序号及总数量
    if(Fn == (APP_GW3762_F7))
    {
    //1为STA，从1开始
        QueryIndex = (StartIndex>0?StartIndex:1);
        TotalNum = APP_GETDEVNUM();
        
    }
    else if(Fn == (APP_GW3762_F104))
    {
    //0为CCO，从0开始
        QueryIndex = (StartIndex>0?StartIndex:1);
        TotalNum = APP_GETDEVNUM() + 1;
        if(StartIndex == 0)
            ReportCCOInfoFlag = TRUE;
    }
    else if(Fn == APP_GW3762_F20||Fn == APP_GW3762_F21||Fn == APP_GW3762_F31||Fn == APP_GW3762_F112)
    {
    //1为CCO，从1开始
        QueryIndex = (StartIndex>1?StartIndex-1:1);
        TotalNum = APP_GETDEVNUM() + 1;
        if(StartIndex == 1)
            ReportCCOInfoFlag = TRUE;
    }
    //组帧上报总数量、起始序号、上报数量
	pGW3762updataunit[ByLen++]	     = (U8)(TotalNum & 0xFF);
    pGW3762updataunit[ByLen++]          = (U8)(TotalNum >> 8);
    if(Fn == APP_GW3762_F20||Fn == APP_GW3762_F21||Fn == APP_GW3762_F31||Fn == APP_GW3762_F112)
    {
        pGW3762updataunit[ByLen++]	     = (U8)(StartIndex & 0xFF);
        pGW3762updataunit[ByLen++]          = (U8)(StartIndex >> 8);
        if(StartIndex == 1)
            ReportCCOInfoFlag = TRUE;
    }
    NumCount = ByLen;
	pGW3762updataunit[ByLen++]  	      = 0;//ucReadNum;
	if(num == 0)
    {
        goto send;
    }   
    if(ReportCCOInfoFlag == TRUE)
    {
        pakect_cco_report_info(Fn , &pGW3762updataunit[ByLen],&ByLen);
        //ReadCount ++;
        ReadNum ++;
        if(num <= ReadNum || ByLen>=APP_GW3762_DATA_UNIT_LEN-8)
        {
            goto send;
        }
    }
    
    for(jj = 0;jj<MAX_WHITELIST_NUM;jj++)
    {
        memset((U8 *)&DeviceListTemp,0xff,sizeof(DEVICE_TEI_LIST_t));
        DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
        if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
        {
            continue;
        }//安徽
        if(devicelist_ban_type(DeviceListTemp))
        {
            continue;
        }
        ReadCount++;
        if(ReadCount >= QueryIndex)
        {
            //app_printf("jj = %d\n",jj);
            if(PROVINCE_ANHUI == app_ext_info.province_code) //todo: PROVINCE_ANHUI
            {
                __memcpy(device_addr, DeviceListTemp.MacAddr, 6);
                ChangeMacAddrOrder(device_addr);

                if(Fn == APP_GW3762_F112 || Fn == APP_GW3762_F7 || Fn == APP_GW3762_F104)//安徽要求
                {
                    if((DeviceListTemp.DeviceType == e_CJQ_2 || DeviceListTemp.DeviceType == e_CJQ_1) && DeviceListTemp.WhiteSeq < MAX_WHITELIST_NUM)
                    {
                        Sqe = DeviceListTemp.WhiteSeq;
                        //                    Sqe=SeachSeqByMacAddr(device_addr);
                        if(DeviceListTemp.WhiteSeq >= MAX_WHITELIST_NUM || 0 != memcmp(device_addr, WhiteMacAddrList[DeviceListTemp.WhiteSeq].MacAddr, MACADRDR_LENGTH))
                        {
                            continue;
                        }
                        else
                        {
                            if(Sqe != -1 && memcmp(WhiteMaCJQMapList[Sqe].CJQAddr, null_addr, 6) != 0)
                            {
                                app_printf("Sqe=%d,CJQAddr ", Sqe);
                                __memcpy(device_addr, WhiteMaCJQMapList[Sqe].CJQAddr, 6);
                                dump_buf(device_addr, 6);
                            }
                            else
                            {
                                app_printf("Sqe=%d,CJQAddr is NULL", Sqe);
                                dump_buf(WhiteMaCJQMapList[Sqe].CJQAddr, 6);
                                continue;
                            }
                        }
                    }
                    else
                    {
                        app_printf("Sqe=meter Addr ");
                        dump_buf(device_addr, 6);
                    }
                }

                app_printf("jj = %d\n", jj);
                LastTemp = ByLen;
                pakect_sta_report_info(Fn, &DeviceListTemp, &pGW3762updataunit[ByLen], &ByLen);
                __memcpy(&pGW3762updataunit[LastTemp], device_addr, 6);//按照采集器地址上报，重新填写采集器地址。
            }
            else
            {
            	pakect_sta_report_info(Fn,&DeviceListTemp,&pGW3762updataunit[ByLen],&ByLen);
            }
            
            ReadNum ++;
        }
        if(num <= ReadNum || ByLen>=APP_GW3762_DATA_UNIT_LEN-8)
        {
            goto send;
        }   
    }
send:   
    pGW3762updataunit[NumCount] = ReadNum;
    Gw3762updata.DataUnitLen    = ByLen;
    if(num == 0)
    {
        //不要回复否认
    }
    app_printf("report num : %d\n",num);
    
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afn10_f7_module_id(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn10_f7_module_id\n");
    U8	ucReadNum = 0;
    U16	usStartIdx = 0 ;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

    if(ucReadNum <= 0U)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
	/*
    if(ucReadNum > 30U)
    {
        ucReadNum = 30U;
    }
    */
    if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
    {
        app_gw3762_up_afn10_fn_by_whitelist_up_frame(APP_GW3762_F7, (U8)pGw3762Data_t->CtrlField.TransMode, usStartIdx, ucReadNum, port);
    }
    else
    {
        app_gw3762_up_afn10_fn_up_frame(APP_GW3762_F7, (U8)pGw3762Data_t->CtrlField.TransMode, usStartIdx, ucReadNum, port);
    }
}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn14_f2_router_req_clock(MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN14F2上行处理
 * 参数说明	: 	无
 * 返回值		: 	无
 *************************************************************************/
void app_gw3762_up_afn14_f2_router_req_clock(MESG_PORT_e port)
{
	APPGW3762DATA_t *gw3762updata 		= &AppGw3762UpData;
    U16 Gw3762SendDataLen = 0;

	gw3762updata->CtrlField.TransDir 		= APP_GW3762_UP_DIR;
	gw3762updata->CtrlField.StartFlag 		= APP_GW3762_MASTER_PRM;
	gw3762updata->CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_master_pack(&(gw3762updata->UpInfoField),NULL);

	gw3762updata->AddrFieldNum 			= 0;
	gw3762updata->Afn					= APP_GW3762_AFN14;
	gw3762updata->Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F2);

	gw3762updata->DataUnitLen			= 0;

    app_gw3762_up_frame_encode(gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, TRUE, 0, port,e_Serial_AFN10XX);
}

#if (GW376213_PROTO_EN > 0)
/*************************************************************************
 * 函数名称	: 	app_gw3762_up_afn14_f3_router_req_revise
 * 函数说明	: 	GW3762 AFN14F3上行处理
 * 参数说明	: 	addr	-从节点地址
 *					time	-预计延迟时间
 *					len	-抄读信息长度
 *					data	-抄读数据内容
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_up_afn14_f3_router_req_revise(U8 *addr, U16 time, U8 *data, U16 len, MESG_PORT_e port)
{
    APPGW3762DATA_t *gw3762updata 		= &AppGw3762UpData;
    U16 Gw3762SendDataLen = 0;
    U16  byLen = 0;

    gw3762updata->CtrlField.TransDir 		= APP_GW3762_UP_DIR;
    gw3762updata->CtrlField.StartFlag 		= APP_GW3762_MASTER_PRM;
	gw3762updata->CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_master_pack(&(gw3762updata->UpInfoField), NULL);

    gw3762updata->AddrFieldNum 			= 0;
    gw3762updata->Afn					= APP_GW3762_AFN14;
    gw3762updata->Fn					= app_gw3762_fn_bin_to_bs(APP_GW3762_F3);

    __memcpy(gw3762updata->DataUnit, addr, 6);
    byLen += 6;
    gw3762updata->DataUnit[byLen++] = (U8)(time & 0xFF);
    gw3762updata->DataUnit[byLen++] = (U8)(time >> 8);
    gw3762updata->DataUnit[byLen++] = len;

    if(len > 0)
    {
        __memcpy(&gw3762updata->DataUnit[byLen], data, len);
        byLen += len;
    }

    gw3762updata->DataUnitLen = byLen;

    app_gw3762_up_frame_encode(gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppPIB_t.AppGw3762UpInfoFrameNum, FALSE, 0, port ,e_Serial_OTHER);
}
#endif

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afn15_f1_file_trans(U32 idx, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFN15F1上行处理
 * 参数说明	: 	idx	-收到当前段标识
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afn15_f1_file_trans(U32 idx, MESG_PORT_e port)
//{
//
//}

/*
U8 app_gw3762_up_afn10_f3_up_frame( U8 seq)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16  byLen = 0;
    U8 addr[6]={0,0,0,0,0,0};
	U16 ii = 0;
    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_MASTER_PRM;
    Gw3762updata.CtrlField.TransMode 	= APP_GW3762_MW_COMMMODE;
    app_gw3762_up_info_field_master_pack(&(Gw3762updata.UpInfoField), NULL);

    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;
    Gw3762updata.Afn				= APP_GW3762_AFN20;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F3);

	for( ii = seq ;ii<WATER_NUM ; ii++ )
	{
		if(0!=memcmp(report_water_t[ii].addr,addr,6))
		{
			 __memcpy(Gw3762updata.AddrField.SrcAddr, report_water_t[ii].addr, MAC_ADDR_LEN);
			 break;
		}	
	}

	if(ii == WATER_NUM)
	{
		return FALSE;
	}
		
    __memcpy(Gw3762updata.AddrField.DestAddr, FlashInfo_t.ucJZQMacAddr, MAC_ADDR_LEN);

    Gw3762updata.DataUnit[byLen++] = seq;

    Gw3762updata.DataUnit[byLen++] = 1;

    __memcpy(&Gw3762updata.DataUnit[byLen], report_water_t[ii].addr, sizeof(WATER_REPORT_t));
	
	byLen += sizeof(WATER_REPORT_t);
	
    Gw3762updata.DataUnitLen		= byLen;

    

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    uart_send(Gw3762SendData, Gw3762SendDataLen);
	return TRUE;

}
*/
//static U8 app_gw3762_up_afn20_f4_up_frame( void )
//{
//    U16 Gw3762SendDataLen = 0;
//    APPGW3762DATA_t Gw3762updata;
//    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
//    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
//    Gw3762updata.CtrlField.TransMode 	= 10;                   //微功率无线
//    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 1, NULL,AppGw3762DownData.DownInfoField.FrameNum);
//
//    Gw3762updata.AddrFieldNum 		= 0;
//    Gw3762updata.UpInfoField.ModuleFlag = 0;
//    Gw3762updata.Afn				= APP_GW3762_AFN20;
//    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F4);
//	
//	Gw3762updata.DataUnitLen = 0;
//	
//    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
//    uart_send(Gw3762SendData, Gw3762SendDataLen);
//	return TRUE;
//
//}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afnf0_f5_debug(U8 *data, U8 len, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFNF0F5上行处理
 * 参数说明	: 	len		-报文长度
 *					data		-报文内容
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afnf0_f5_debug(U8 externedFlag, U8 *data, U16 len, MESG_PORT_e port)
//{
//    APPGW3762DATA_t *gw3762updata    = &AppGw3762UpData;
//    U16 Gw3762SendDataLen = 0;
//
//    gw3762updata->CtrlField.TransDir     = APP_GW3762_UP_DIR;
//    gw3762updata->CtrlField.StartFlag     = APP_GW3762_SLAVE_PRM;
//    gw3762updata->CtrlField.TransMode  = APP_GW3762_HPLC_HRF_CTRL_COMMMODE;
//
//    app_gw3762_up_info_field_slave_pack(&(gw3762updata->UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
//
//    gw3762updata->AddrFieldNum = 0;
//
//    if(externedFlag == TRUE)
//    {
//        gw3762updata->Afn                = APP_GW3762_AFNF0;
//        gw3762updata->Fn                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F5);
//    }
//    else if(externedFlag == 2)
//    {
//        gw3762updata->Afn                = APP_GW3762_AFN10;
//        gw3762updata->Fn                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F21);
//    }
//    else if(externedFlag == 3)
//    {
//        gw3762updata->Afn                = APP_GW3762_AFN10;
//        gw3762updata->Fn                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F102);
//    }
//
//    gw3762updata->DataUnitLen = len;
//
//    __memcpy(gw3762updata->DataUnit, data, len);
//
//    app_gw3762_up_frame_encode(gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
//    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
//}

//static void app_gw3762_up_afnf0_f6_debug(U8 *data, U16 len, MESG_PORT_e port)
//{
//    APPGW3762DATA_t *gw3762updata    = &AppGw3762UpData;
//    U16 Gw3762SendDataLen = 0;
//
//    gw3762updata->CtrlField.TransDir     = APP_GW3762_UP_DIR;
//    gw3762updata->CtrlField.StartFlag     = APP_GW3762_SLAVE_PRM;
//    gw3762updata->CtrlField.TransMode  = APP_GW3762_HPLC_HRF_CTRL_COMMMODE;
//
//    app_gw3762_up_info_field_slave_pack(&(gw3762updata->UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
//
//    gw3762updata->AddrFieldNum = 0;
//
//    gw3762updata->Afn                = APP_GW3762_AFNF0;
//    gw3762updata->Fn                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F6);
//
//    gw3762updata->DataUnitLen = len;
//
//    __memcpy(gw3762updata->DataUnit, data, len);
//
//    app_gw3762_up_frame_encode(gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
//    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
//}

/*************************************************************************
 * 函数名称	: 	void app_gw3762_up_afnf0_f7_debug(U8 *data, U16 len, MESG_PORT_e port, MESG_PORT_e port)
 * 函数说明	: 	GW3762 AFNF0F7上行处理
 * 参数说明	: 	len		-报文长度
 *					data		-报文内容
 * 返回值		: 	无
 *************************************************************************/
//static void app_gw3762_up_afnf0_f7_debug(U8 *data, U16 len, MESG_PORT_e port)
//{
//    
//}

//static void app_gw3762_up_afnf0_f8_debug(U8 *addr, U8 gatherType, U8 *data, U16 len, MESG_PORT_e port)
//{
//    APPGW3762DATA_t *gw3762updata    = &AppGw3762UpData;
//    U16 Gw3762SendDataLen = 0;
//    U16  byLen = 0;
//    
//
//    gw3762updata->CtrlField.TransDir     = APP_GW3762_UP_DIR;
//    gw3762updata->CtrlField.StartFlag     = APP_GW3762_SLAVE_PRM;
//    gw3762updata->CtrlField.TransMode  = APP_GW3762_HPLC_HRF_CTRL_COMMMODE;
//
//    app_gw3762_up_info_field_slave_pack(&(gw3762updata->UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
//
//    gw3762updata->AddrFieldNum = 0;
//
//    gw3762updata->Afn                = APP_GW3762_AFNF0;
//    gw3762updata->Fn                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F8);
//
//    __memcpy(&gw3762updata->DataUnit[byLen], addr, MAC_ADDR_LEN);
//    byLen += MAC_ADDR_LEN;
//    
//    //gw3762updata->DataUnit[byLen++] = gatherType;
//
//    gw3762updata->DataUnitLen = len + byLen;
//    __memcpy(&gw3762updata->DataUnit[byLen], data, len);
//
//    app_gw3762_up_frame_encode(gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
//    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
//}

/*************************************************************************
 * 函数名称	: 	app_gw3762_afnf1_f1_concurrent
 * 函数说明	: 	f1f1处理接口
 * 参数说明	:
 *				APPGW3762DATA_t *pGw3762Data_t	- 解析GW3762帧的数据
 *              MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/
static void app_gw3762_afnf1_f1_concurrent(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8  *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U16 byLen = 0;
    U8  findMeterFlag = FALSE;//不在白名单
    U8  BroadcastFlag = Generally;//广播抄读标志
    U8  readMeterNum;//一帧携带645帧数量
    U16 ii = 0;
    U8  nullAddr[6] = {0,0,0,0,0,0};
    
    readMeterNum = 0;

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

    if (AppGw3762Afn13F1State.FrameLen == 0 || (AppGw3762Afn13F1State.FrameLen + byLen) != pGw3762Data_t->DataUnitLen)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port); 
        return;
    }
    else
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
	    else  if(TRUE==Check698Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen,NULL, AppGw3762Afn13F1State.Addr, NULL))
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

    switch (AppGw3762Afn13F1State.ProtoType)
	{
		case APP_T_PROTOTYPE:
	            app_printf("APP_T_PROTOTYPE \n");
				readMeterNum = 1;
			break;
		case APP_97_PROTOTYPE:
		case APP_07_PROTOTYPE:
			
		    readMeterNum = Check645FrameNum(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen);
			break;
		case APP_698_PROTOTYPE:
			if(TRUE==Check698Frame(AppGw3762Afn13F1State.FrameUnit,AppGw3762Afn13F1State.FrameLen, NULL,NULL, NULL))
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

    if(PROVINCE_JIANGSU == app_ext_info.province_code)//todo: PROVINCE_JIANGSU
    {
        for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
        {
            if((memcmp(WhiteMacAddrList[ii].MacAddr, AppGw3762Afn13F1State.Addr, MAC_ADDR_LEN) == 0) ||
                memcmp(WhiteMaCJQMapList[ii].CJQAddr, AppGw3762Afn13F1State.Addr, MAC_ADDR_LEN) == 0)
            {
                __memcpy(AppGw3762Afn13F1State.CnmAddr, WhiteMacAddrList[ii].CnmAddr, MAC_ADDR_LEN);
                findMeterFlag = TRUE;
                break;
            }
        }
    }
    else
    {
	    for(ii = 0; ii < MAX_WHITELIST_NUM; ii++)
        {
            if((memcmp(WhiteMacAddrList[ii].MacAddr, AppGw3762Afn13F1State.Addr, MAC_ADDR_LEN) == 0))
            {
                __memcpy(AppGw3762Afn13F1State.CnmAddr, WhiteMacAddrList[ii].CnmAddr, MAC_ADDR_LEN);
                findMeterFlag = TRUE;
                break;
            }
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
    	BroadcastFlag = AllBroadcast;
        app_printf("AllBroadcast\n");
    }
    
    app_printf("F1F1.Addr : ");
    dump_buf(AppGw3762Afn13F1State.Addr, 6);
	app_printf("F1F1.CnmAddr : ");
    dump_buf(AppGw3762Afn13F1State.CnmAddr, 6);

	jzq_read_cntt_act_cncrnt_read_meter_req(pGw3762Data_t->DownInfoField.FrameNum, readMeterNum, AppGw3762Afn13F1State.Addr, 
                                AppGw3762Afn13F1State.CnmAddr, AppGw3762Afn13F1State.ProtoType, AppGw3762Afn13F1State.FrameLen, 
                                 AppGw3762Afn13F1State.FrameUnit,BroadcastFlag, port);
	return;
}

/*************************************************************************
 * 函数名称	: 	app_gw3762_up_afnf1_f1_up_frame
 * 函数说明	: 	并发抄读上行报文处理接口
 * 参数说明	: 	U8 *Addr                    - 表地址
                AppProtoType_e proto        - 协议类型
                U8 *data                    - 上行数据
                U16 len                     - 上行数据长度
                MESG_PORT_e port            - 消息类型
                U8 localseq                 - 本地报文序号
 * 返回值	: 	无
 *************************************************************************/
void app_gw3762_up_afnf1_f1_up_frame(U8 *Addr, AppProtoType_e proto, U8 *data, U16 len, MESG_PORT_e port,U8 localseq)
{
    //U8 Gw3762SendData[APP_GW3762_FRAME_MAX_LEN] = {0};
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16 byLen = 0;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    Gw3762updata.AddrFieldNum 		= 2;
    Gw3762updata.UpInfoField.ModuleFlag = 1;
    Gw3762updata.Afn				= APP_GW3762_AFNF1;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F1);

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
    
    Gw3762updata.DataUnitLen = byLen;
    
    //抄表失败，打印节点信息
    DEVICE_TEI_LIST_t DeviceListTemp;
    app_printf("readup->%02x%02x%02x%02x%02x%02x:",Addr[5],Addr[4],Addr[3],Addr[2],Addr[1],Addr[0]);

    if(0 == len)
    {   
        app_printf("tout");
        if(Get_DeviceList_All_ByAdd(Addr ,  &DeviceListTemp)==TRUE)
        {
            app_printf("%d\t%04x\t%04x\t%d\t%d\t%d\n",DeviceListTemp.NodeDepth,DeviceListTemp.NodeTEI,DeviceListTemp.ParentTEI, 
            DeviceListTemp.LinkType,DeviceListTemp.UplinkSuccRatio,DeviceListTemp.DownlinkSuccRatio);
        }
        else
        {
            app_printf("offline\n");
        }
    }
    else
    {
        app_printf("recv\n");
    }

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen,localseq, FALSE, 0, port,e_Serial_AFNF1F1);
}

static void app_gw3762_afn10_f20_read_rf_topo(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f20_read_rf_topo!\n");
    
    U8	ucReadNum = 0;
    U16	usStartIdx = 0 ;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

    if(ucReadNum <= 0U)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
    if(ucReadNum > 64U)
    {
        ucReadNum = 64U;
    }
    
    app_gw3762_up_afn10_fn_up_frame(APP_GW3762_F20 , (U8)pGw3762Data_t->CtrlField.TransMode , usStartIdx , ucReadNum, port);
}

static void app_gw3762_afn10_f21_read_topo(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f21_read_topo!\n");
    
    U8	ucReadNum = 0;
    U16	usStartIdx = 0 ;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

    if(ucReadNum <= 0U)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
	/*
    if(ucReadNum > 64U)
    {
        ucReadNum = 64U;
    }*/
    
    app_gw3762_up_afn10_fn_up_frame(APP_GW3762_F21 , (U8)pGw3762Data_t->CtrlField.TransMode , usStartIdx , ucReadNum, port);
}

static void app_gw3762_afn10_f103_read_topo(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8 *gw3762downdataunit = pGw3762Data_t->DataUnit;
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U16   rcvLen = 0,jj,ii,byLen;
    U8	ucReadNum = 0;
    //U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    U8  device_addr[6]={0};
    DEVICE_TEI_LIST_t DeviceListTemp;
    
    app_printf("app_gw3762_afn10_f103_read_topo!\n");

    //startIndex = (U16)gw3762downdataunit[rcvLen++];
    //startIndex += ((U16)gw3762downdataunit[rcvLen++] << 8);

    ucReadNum = (U16)gw3762downdataunit[rcvLen++];
    
    
	if(ucReadNum<=0)
	{
	    app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
	    return;
	}
	if(ucReadNum>64)
	{
        ucReadNum = 64;
	}
	Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
	Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
	app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
	Gw3762updata.AddrFieldNum 		= 0;
	Gw3762updata.Afn				= APP_GW3762_AFN10;
	Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F103);
	Gw3762updata.DataUnitLen            = 2 + 2 +1;
	byLen = 0 ;
	pGW3762updataunit[byLen++]  = (U8)(ucReadNum & 0xFF);
	pGW3762updataunit[byLen++]  =  (U8)(ucReadNum >>8);

    for(ii = 0 ; ii<ucReadNum   ; ii++)
    {
        __memcpy(device_addr,&gw3762downdataunit[rcvLen],6);
		ChangeMacAddrOrder(device_addr);
        
        for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
            {
                memset((U8 *)&DeviceListTemp,0xff,sizeof(DEVICE_TEI_LIST_t));
				DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
                if(0==memcmp(DeviceListTemp.MacAddr,device_addr,6))//无效地址
                {
	                //__memcpy(device_addr,DeviceTEIList[jj].MacAddr,6);
	                ChangeMacAddrOrder(device_addr);
	                __memcpy(&pGW3762updataunit[byLen], device_addr, 6); //6字节地址+两字节信息
	                byLen += 6 ;

		       		//__memcpy(pGW3762updataunit+6,(U8 *)&DeviceTEIList[jj].UplinkSuccRatio,2);
	                pGW3762updataunit[byLen++] = (U8)(DeviceListTemp.NodeTEI & 0xFF);
	                pGW3762updataunit[byLen++] = (U8)(DeviceListTemp.NodeTEI >> 8);

	                pGW3762updataunit[byLen++] = (U8)(DeviceListTemp.ParentTEI & 0xFF);
	                pGW3762updataunit[byLen++] = (U8)(DeviceListTemp.ParentTEI >> 8);

	                pGW3762updataunit[byLen++] = (DeviceListTemp.NodeDepth & 0x0F) + ((DeviceListTemp.NodeType & 0x0F) << 4);
	                pGW3762updataunit[byLen++] =(U8)(DeviceListTemp.DeviceType);
	                __memcpy(&pGW3762updataunit[byLen], DeviceListTemp.ManufactorCode, 2);                
					byLen += 2;
	                pGW3762updataunit[byLen++] =(U8)(DeviceListTemp.UplinkSuccRatio);
	                pGW3762updataunit[byLen++] =(U8)(DeviceListTemp.DownlinkSuccRatio);
	                pGW3762updataunit[byLen++] =(U8)(DeviceListTemp.BootVersion);
	                __memcpy(&pGW3762updataunit[byLen], DeviceListTemp.ManufactorCode, 2);                
					byLen += 2;
					memset(&pGW3762updataunit[byLen], 0x00, 5);                //保留位5字节
					byLen += 5;
					//pGW3762updataunit += 28;
					Gw3762updata.DataUnitLen+= 24;
	               
	                break;
            	}
            else if(jj ==MAX_WHITELIST_NUM-1 )
            {
            	app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
        		return;
            }
        }
         rcvLen+=6;
    }


       app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
    return;
}

static void app_gw3762_afn10_f103_proc(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    extern void app_gw3762_afn10_f103_get_slave_curve_gather(APPGW3762DATA_t * pGw3762Data_t, MESG_PORT_e port);
    if((PROVINCE_SHANNXI == app_ext_info.province_code) || (PROVINCE_HUNAN == app_ext_info.province_code)) //todo: PROVINCE_SHANNXI PROVINCE_HUNAN
    {
        app_gw3762_afn10_f103_get_slave_curve_gather(pGw3762Data_t, port);
    }
    else
    {
        app_gw3762_afn10_f103_read_topo(pGw3762Data_t, port);
    }
}

static void app_gw3762_afn10_f111_neigh_net(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16  byLen = 0,i;
	U16  SNIDNum = 0;
    app_printf("app_gw3762_afn10_f111_neigh_net!\n");

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);

    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.UpInfoField.ModuleFlag = 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F111);

    Gw3762updata.DataUnit[byLen++] = 0;

    Gw3762updata.DataUnit[byLen++] = (U8)(APP_GETSNID() & 0xFF);
    Gw3762updata.DataUnit[byLen++] = (U8)(APP_GETSNID() >> 8);
    Gw3762updata.DataUnit[byLen++] = (U8)(APP_GETSNID() >> 16);

    __memcpy(&Gw3762updata.DataUnit[byLen], FlashInfo_t.ucJZQMacAddr , 6);
	byLen += 6;
	for(i=0;i<MaxSNIDnum;i++)
		{
		#if defined(STD_2016)
			if(NeighborNet[i].SendSNID !=0xffffffff)
			{				
				Gw3762updata.DataUnit[byLen++] = (U8)(NeighborNet[i].SendSNID  & 0xFF); 
				Gw3762updata.DataUnit[byLen++] = (U8)(NeighborNet[i].SendSNID  >> 8); 
				Gw3762updata.DataUnit[byLen++] = (U8)(NeighborNet[i].SendSNID  >> 16); 
				SNIDNum++;
			}
		#endif
		}

	Gw3762updata.DataUnit[0] = SNIDNum;
    Gw3762updata.DataUnitLen		= byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);

    return;
}

//static void app_gw3762_afn10_f111_neigh_net_up(U8 NbNum,U8 *pData, U16 DataLen,U32 MySNID, U8 *pMacAddr)
//{
//	
//	   U16 Gw3762SendDataLen = 0;
//	   U16  byLen = 0;
//	   APPGW3762DATA_t Gw3762updata;
//	
//	   app_printf("app_gw3762_afn10_f111_neigh_net!\n");
//	
//	   
//	
//	   Gw3762updata.CtrlField.TransDir	   = APP_GW3762_UP_DIR;
//	   Gw3762updata.CtrlField.StartFlag    = APP_GW3762_SLAVE_PRM;
//	   Gw3762updata.CtrlField.TransMode    = APP_GW3762_HPLC_HRF_CTRL_COMMMODE;
//	   app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
//	
//	   Gw3762updata.AddrFieldNum	   = 0;
//	   Gw3762updata.UpInfoField.ModuleFlag = 0;
//	   Gw3762updata.Afn 			   = APP_GW3762_AFN10;
//	   Gw3762updata.Fn			   = app_gw3762_fn_bin_to_bs(APP_GW3762_F111);
//
//	
//	   Gw3762updata.DataUnit[byLen++] = NbNum;
//		Gw3762updata.DataUnit[byLen++] = (U8)(MySNID & 0xFF);
//		Gw3762updata.DataUnit[byLen++] = (U8)(MySNID >> 8);
//		Gw3762updata.DataUnit[byLen++] = (U8)(MySNID >> 16);
//	   
//
//		__memcpy(&Gw3762updata.DataUnit[byLen], FlashInfo_t.ucJZQMacAddr, 6);
//		byLen +=6;
//	    __memcpy(&Gw3762updata.DataUnit[byLen], pData,DataLen);
//		byLen +=DataLen;
//		Gw3762updata.DataUnitLen 	   = byLen;
//	  
//	   app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
//       send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, e_UART1_MSG,e_Serial_AFN10XX);
//}

static void app_gw3762_afn03_f16_get_band(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U8  bandVal = 2;
    if(app_ext_info.func_switch.SetBandSWC == 1)
    {
        bandVal= DefSetInfo.FreqInfo.FreqBand;
    }
    else
    {
        bandVal = JZQ_Set_Band;
    }
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn                                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F16);
    Gw3762updata.DataUnitLen			= 1;

    Gw3762updata.DataUnit[0] = bandVal;    

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);    
}

static void app_gw3762_afn03_f17_get_hrf_band(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    APPGW3762AFN03F17_t Gw03F17updata;
    Gw03F17updata.ucRfOption = getHplcOptin();
    Gw03F17updata.ucRfChannel = getHplcRfChannel();
    Gw03F17updata.ucRfConsultEn = GetRfConsultEn();

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0,NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 			= 0;
    Gw3762updata.Afn					= APP_GW3762_AFN03;
    Gw3762updata.Fn                                  = app_gw3762_fn_bin_to_bs(APP_GW3762_F17);
    Gw3762updata.DataUnitLen			= 3;

    __memcpy(&Gw3762updata.DataUnit, (U8 *)&Gw03F17updata, 3);    

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);    
}

static void app_gw3762_afn05_f16_set_band(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
  
    //辽宁无法切换，强制注释载波切换
	//JZQ_Set_Band = pGw3762Data_t->DataUnit[0];

   //合法值检查
	if(pGw3762Data_t->DataUnit[0] >= 0&&pGw3762Data_t->DataUnit[0] <= 3)
    {   
        if(app_ext_info.func_switch.SetBandSWC == 1)
        {   
            nnib.BandChangeState = TRUE;
	        nnib.BandRemianTime = 150; //100s切换频段
    	    DefSetInfo.FreqInfo.FreqBand = pGw3762Data_t->DataUnit[0];
            app_printf("Set DefSetInfo.FreqInfo.FreqBand=%d",DefSetInfo.FreqInfo.FreqBand);
            DefwriteFg.FREQ = TRUE;
            DefaultSettingFlag = TRUE;
        }
        else
        {
            //辽宁无法切换，强制注释载波切换
	        JZQ_Set_Band = pGw3762Data_t->DataUnit[0];
        }
    }
	else
    {
        app_printf("err data pGw3762Data_t->DataUnit[0]=%d",pGw3762Data_t->DataUnit[0]);
    }
    
	app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    return;
}

static void app_gw3762_afn05_f17_set_hrf_band(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    APPGW3762AFN03F17_t Gw05F17data;

    __memcpy((U8 *)&Gw05F17data, pGw3762Data_t->DataUnit, 3);
    
    //设置禁止协商标志
    SetRfConsultEn(Gw05F17data.ucRfConsultEn);


    if(checkRfChannel(Gw05F17data.ucRfOption, Gw05F17data.ucRfChannel) == FALSE) 
    {   
        app_printf("err op:%d,chl:%d\n", Gw05F17data.ucRfOption, Gw05F17data.ucRfChannel);
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }

   //合法值检查
    // if(Gw05F17data.ucRfConsultEn == 1)
    {
        if(Gw05F17data.ucRfOption == getHplcOptin() && Gw05F17data.ucRfChannel == getHplcRfChannel())
        {
            app_printf("Rf Option channel <%d,%d> is same\n",Gw05F17data.ucRfOption, Gw05F17data.ucRfChannel);
        }
        else
        {
            nnib.RfChannelChangeState = 1;
            nnib.RfChgChannelRemainTime = 300;
            DefSetInfo.RfChannelInfo.option = Gw05F17data.ucRfOption ;
            DstCoordRfChannel = DefSetInfo.RfChannelInfo.channel = Gw05F17data.ucRfChannel;
            app_printf("set channel change to be <%d,%d>\n",DefSetInfo.RfChannelInfo.option, DefSetInfo.RfChannelInfo.channel);
        }
    }

    app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    return;
}

static void app_gw3762_afn03_f101_query_con_err(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn03_f101_query_con_err!\n");
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8  device_addr[6]={0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    U16  jj = 0;
	U16 phase = 0;
	APPGW3762AFN03F101_t  AData;
    DEVICE_TEI_LIST_t DeviceListTemp;
	U8  *pnum=NULL;

    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	if(ucReadNum>15)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port); //无信息可读
		return;
	}
	
	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN03;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F101);
    Gw3762updata.DataUnitLen			= 2 + 2 + 1;
    pGW3762updataunit[0]	     = (U8)(APP_GETDEVNUM() & 0xFF);
    pGW3762updataunit[1]          = (U8)(APP_GETDEVNUM() >> 8);
	__memcpy(&pGW3762updataunit[2],(U8 *)&usStartIdx,2);
	pGW3762updataunit[4]  	      = 0;//ucReadNum;
    pnum = &pGW3762updataunit[4];
    pGW3762updataunit             += 5;

	//其实序号为0，读集中器地址
	if(((U16)pGw3762Data_t->DataUnit[0]|((U16)pGw3762Data_t->DataUnit[1] << 8))==1)
	{
		__memcpy(pGW3762updataunit, FlashInfo_t.ucJZQMacAddr, 6); //6字节地址+1字节信息
		//phase = e_A_PHASE;
		AData.RecordLoc= 1;
	    AData.CourtsState= 1;
	    AData.Phase= 0b111 ;
	    AData.LNerr= 0; 
	    AData.reserve= 0 ;        
		__memcpy(pGW3762updataunit+6,&AData,1);
		__memcpy(pGW3762updataunit, FlashInfo_t.ucJZQMacAddr, 6); //6字节集中器地址

		pGW3762updataunit += 15;
              Gw3762updata.DataUnitLen	+= 15;
		*pnum += 1;
		if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
		{
			goto send;
		}
              usStartIdx+=1;
	}
	
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
		if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
		{
			continue;
		}

		__memcpy(device_addr,DeviceListTemp.MacAddr,6);
		
		ChangeMacAddrOrder(device_addr);
		
		if(ii == usStartIdx-2)//当前为读取的起始位置
		{
			__memcpy(pGW3762updataunit, device_addr, 6); //6字节地址+两字节信息
			phase = DeviceListTemp.Phase;
			AData.RecordLoc= 1;
		    AData.CourtsState= 1;
		    AData.Phase= 1<<(phase-1);
		    AData.LNerr= (DeviceListTemp.LNerr&0x01); 
		    AData.reserve= 0 ;        
			__memcpy(pGW3762updataunit+6,&AData,1);
			__memcpy(pGW3762updataunit, FlashInfo_t.ucJZQMacAddr, 6); //6字节集中器地址

			pGW3762updataunit += 15;
              Gw3762updata.DataUnitLen	+= 15;

				
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				//WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx-2)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
       /*
	if(*pnum == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
       */
	send:
	
    app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afn10_f9_net_work_size(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U16  byLen = 0;

    app_printf("app_gw3762_afn10_f9_net_work_size!\n");

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);

    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.UpInfoField.ModuleFlag = 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F9);
	
	Gw3762updata.DataUnit[byLen++] = (U8)((APP_GETDEVNUM()+1) & 0xFF);
	Gw3762updata.DataUnit[byLen++] = (U8)((APP_GETDEVNUM()+1) >> 8);
    Gw3762updata.DataUnitLen		= byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);	
}

static U8 order_err(DEVICE_TEI_LIST_t GetDeviceListTemp_t)
{
    U8 result = 0;
    U8 LNerr;
    U8 DeviceType;
    U8 order1;
    U8 order2;
    U8 order3;
    U8 Phase1;
    U8 Phase2;
    U8 Phase3;
    
    Phase1 = GetDeviceListTemp_t.F31_D0;
	Phase2 = GetDeviceListTemp_t.F31_D1;
	Phase3 = GetDeviceListTemp_t.F31_D2;
    LNerr = GetDeviceListTemp_t.LNerr&0x01;
	order1 = GetDeviceListTemp_t.F31_D5;
	order2 = GetDeviceListTemp_t.F31_D6;
	order3 = GetDeviceListTemp_t.F31_D7;
    DeviceType = GetDeviceListTemp_t.DeviceType;
    if(DeviceType == e_3PMETER)
    {
        if(PROVINCE_HENAN == app_ext_info.province_code)//todo: PROVINCE_HENAN
        {
            if((((order1 == 0 && order2 == 0 && order3 == 0) || (order1 == 1 && order2 == 1 && order3 == 0)
                || (order1 == 0 && order2 == 0 && order3 == 1)) && (Phase1 + Phase2 + Phase3 == 3))
                || (order1 == 1 && order2 == 1 && order3 == 1))
            {
                //result = 0; //三相表接线正常
            }
            else
            {
                result = 1; //接线异常
                app_printf("result = 1;\n");
            }
        }
        else
        {
            if( ((order1 == 0 && order2 == 0 && order3 == 0)&&(Phase1+Phase2+Phase3 == 3)) 
            || (order1 == 1 && order2 == 1 && order3 == 1) )
            {
                //result = 0; //三相表接线正常
            }
            else
            {
                result = 1; //接线异常
                app_printf("result = 1;\n");
            }
        }
    }
    //app_printf("(result |= LNerr)=%d  \n",(result |= LNerr));
    return (result |= LNerr)&0x01;
}

U16  get_online_whiteaddr_num(void)
{
    U16 usIdx = 0;
    U8 nulladdr[6] = { 0 };
    U16 num = 0;

    WHLPTST
        for(usIdx = 0; usIdx < MAX_WHITELIST_NUM; usIdx++)
        {
            if(0 != memcmp(nulladdr, WhiteMacAddrList[usIdx].CnmAddr, 6))
            {
                num++;
            }
            else
            {

            }
        }
    WHLPTED;
        return num;
}

/*************************************************************************
 * 函数名称	: 	void AppGw3762UpAfn10FnByWhitelistUpFrame(U8 AFN, U16 Fn, U8 TransMode, QUERY_TYPE_e port, U16 StartIndex, U8 num)
 * 函数说明	: 	GW3762 AFN14F1上行处理函数
 * 参数说明	: 	pGw3762Data_t	- 解析GW3762帧的数据
 * 返回值		: 	无
 *************************************************************************/
static void app_gw3762_up_afn10_fn_by_whitelist_up_frame(U16 Fn, U8 TransMode, U16 StartIndex, U8 num, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    //APPGW3762DATA_t Gw3762updata;
    U16  ByLen = 0;
    U16  QueryIndex = 0;
    U16  TotalNum = 0;
    U16  ReportCCOInfoFlag = FALSE; //cco上报标志
    U16  jj = 0;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U16   NumCount = 0;   //上报数量偏移
    U16   ReadCount = 0;  //实际有效数量,超过255
    U8   ReadNum = 0;   //本次上报数量
    U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    //U8  device_addr[6]={0};
    U8  null_addr[6] = { 0 };
    DEVICE_TEI_LIST_t DeviceListTemp;
    U8  TempCnmAddr[6] = { 0 };
    U16 LastTemp = 0;

    Gw3762updata.CtrlField.TransDir = APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag = APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode = TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL, AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum = 0;
    Gw3762updata.Afn = APP_GW3762_AFN10;
    Gw3762updata.Fn = app_gw3762_fn_bin_to_bs(Fn);
    //确认查询从模块起始序号及总数量
    if(Fn == (APP_GW3762_F7))
    {
        //1为STA，从1开始
        QueryIndex = (StartIndex > 0 ? StartIndex : 1);
        TotalNum = get_online_whiteaddr_num();

    }
    else if(Fn == (APP_GW3762_F104))
    {
        //0为CCO，从0开始
        QueryIndex = (StartIndex > 0 ? StartIndex : 1);
        TotalNum = get_online_whiteaddr_num() + 1;
        if(StartIndex == 0)
            ReportCCOInfoFlag = TRUE;
    }
    else if(Fn == APP_GW3762_F31 || Fn == APP_GW3762_F112)
    {
        //1为CCO，从1开始
        QueryIndex = (StartIndex > 1 ? StartIndex - 1 : 1);
        TotalNum = get_online_whiteaddr_num() + 1;
        if(StartIndex == 1)
            ReportCCOInfoFlag = TRUE;
    }
    //组帧上报总数量、起始序号、上报数量
    pGW3762updataunit[ByLen++] = (U8)(TotalNum & 0xFF);
    pGW3762updataunit[ByLen++] = (U8)(TotalNum >> 8);
    if(Fn == APP_GW3762_F31 || Fn == APP_GW3762_F112)
    {
        pGW3762updataunit[ByLen++] = (U8)(StartIndex & 0xFF);
        pGW3762updataunit[ByLen++] = (U8)(StartIndex >> 8);
        if(StartIndex == 1)
            ReportCCOInfoFlag = TRUE;
    }
    NumCount = ByLen;
    pGW3762updataunit[ByLen++] = 0;//ucReadNum;
    if(num == 0)
    {
        goto send;
    }
    if(ReportCCOInfoFlag == TRUE)
    {
        pakect_cco_report_info(Fn, &pGW3762updataunit[ByLen], &ByLen); 
        //ReadCount ++;
        ReadNum++;
        if(num <= ReadNum || ByLen >= APP_GW3762_DATA_UNIT_LEN - 8)
        {
            goto send;
        }
    }

    if((PROVINCE_ANHUI == app_ext_info.province_code) || (PROVINCE_SHANGHAI == app_ext_info.province_code))
    {
    for(jj = 0; jj < MAX_WHITELIST_NUM; jj++)
    {
        if(0 == memcmp(WhiteMacAddrList[jj].MacAddr, null_addr, 6))//无效地址
        {
            continue;
        }
        if(memcmp(null_addr, WhiteMacAddrList[jj].CnmAddr, 6) == 0)
        {
            //通讯地址为空，跳过
            continue;
        }
        ReadCount++;
        if(ReadCount >= QueryIndex)
        {
            app_printf("jj = %d\n", jj);
            __memcpy(TempCnmAddr, WhiteMacAddrList[jj].CnmAddr, 6);
            ChangeMacAddrOrder(TempCnmAddr);
            if(Get_DeviceList_All_ByAdd(TempCnmAddr, &DeviceListTemp) == TRUE)
            {

                LastTemp = ByLen; 
                pakect_sta_report_info(Fn, &DeviceListTemp, &pGW3762updataunit[ByLen], &ByLen);
                __memcpy(&pGW3762updataunit[LastTemp], WhiteMacAddrList[jj].MacAddr, 6);//按照白名单上报时，需要重新处理
                ReadNum++;
            }
            else
            {
                continue;
            }
        }
        if(num <= ReadNum || ByLen >= APP_GW3762_DATA_UNIT_LEN - 8)
        {
            goto send;
        }
    }
    }
    else
    {
        for(jj = 0; jj < MAX_WHITELIST_NUM; jj++)
        {
            memset((U8 *)&DeviceListTemp, 0xff, sizeof(DEVICE_TEI_LIST_t));
            DeviceList_ioctl(DEV_GET_ALL_BYSEQ, &jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
            if(0 == memcmp(DeviceListTemp.MacAddr, default_addr, 6))//无效地址
            {
                continue;
            }//安徽
            if(devicelist_ban_type(DeviceListTemp))
            {
                continue;
            }
            ReadCount++;
            if(ReadCount >= QueryIndex)
            {
                //app_printf("jj = %d\n", jj);
                pakect_sta_report_info(Fn, &DeviceListTemp, &pGW3762updataunit[ByLen], &ByLen);
                ReadNum++;
            }
            if(num <= ReadNum || ByLen >= APP_GW3762_DATA_UNIT_LEN - 8)
            {
                goto send;
            }
        }
    }

send:
    pGW3762updataunit[NumCount] = ReadNum;
    Gw3762updata.DataUnitLen = ByLen;
    if(num == 0)
    {
        //不要回复否认
    }
    app_printf("report num : %d\n", num);

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port, e_Serial_AFN10XX);
}

static void app_gw3762_afn10_f31_query_phase(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn10_f31_query_phase!\n");

    U8	ucReadNum = 0;
    U16	usStartIdx = 0 ;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

    if(ucReadNum <= 0U)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
    if(ucReadNum > 64U)
    {
        ucReadNum = 64U;
    }
    
    if((PROVINCE_ANHUI == app_ext_info.province_code) || (PROVINCE_SHANGHAI == app_ext_info.province_code)) //todo: PROVINCE_ANHUI  PROVINCE_SHANGHAI
    {
        app_gw3762_up_afn10_fn_by_whitelist_up_frame(APP_GW3762_F31, (U8)pGw3762Data_t->CtrlField.TransMode, usStartIdx, ucReadNum, port);
    }
    else
    {
    	app_gw3762_up_afn10_fn_up_frame(APP_GW3762_F31 , (U8)pGw3762Data_t->CtrlField.TransMode , usStartIdx , ucReadNum, port); 
    }   
}

static void app_gw3762_afn10_f32_query_plc_channel_quality(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f32_query_plc_channel_quality!\n");
	U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8  device_addr[6]={0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    U16  jj = 0;
	U16 rate = 0;
    DEVICE_TEI_LIST_t DeviceListTemp;
	U8  *pnum=NULL;

    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	if(ucReadNum>64)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port); //无信息可读
		return;
	}
	
    app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F32);
    Gw3762updata.DataUnitLen			= 2 + 2 + 1;
    pGW3762updataunit[0]	     = (U8)((APP_GETDEVNUM() + 1) & 0xFF);
    pGW3762updataunit[1]          = (U8)((APP_GETDEVNUM() + 1) >> 8);
	__memcpy(&pGW3762updataunit[2],(U8 *)&usStartIdx,2);
	pGW3762updataunit[4]  	      = 0;//ucReadNum;
    pnum = &pGW3762updataunit[4];
    pGW3762updataunit             += 5;

	//其实序号为0，读集中器地址
	if(((U16)pGw3762Data_t->DataUnit[0]|((U16)pGw3762Data_t->DataUnit[1] << 8))==1)
	{
		__memcpy(pGW3762updataunit, FlashInfo_t.ucJZQMacAddr, 6); //6字节地址+两字节信息
		rate = (100<<8)|100;
		__memcpy(pGW3762updataunit+6,&rate,2);

		pGW3762updataunit += 8;
             Gw3762updata.DataUnitLen	+= 8;
		*pnum += 1;
		if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
		{
			goto send;
		}
              usStartIdx+=1;
	}
	
	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{		
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);//Get_DeviceList_All(jj ,  &DeviceListTemp);
		if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
		{
			continue;
		}

		__memcpy(device_addr,DeviceListTemp.MacAddr,6);
		
		ChangeMacAddrOrder(device_addr);
		
		if(ii == usStartIdx-2)//当前为读取的起始位置
		{
			__memcpy(pGW3762updataunit, device_addr, 6); //6字节地址+两字节信息

			__memcpy(pGW3762updataunit+6,(U8 *)&DeviceListTemp.UplinkSuccRatio,2);

			pGW3762updataunit += 8;
        	Gw3762updata.DataUnitLen	+= 8;
			*pnum += 1;

				
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				WHLPTED//释放共享存储内容保护
				goto send;
			}
		}
		else if(ii < usStartIdx-2)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
		
	}
	
       /*
	if(*pnum == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读	
		return;	
	}
       */
	send:
	
    app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_afn10_f33_query_id_info(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    app_printf("app_gw3762_afn10_f33_query_id_info!\n");
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8	ucReadNum = 0;
	U8  default_addr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
	U8  device_addr[6]={0};
    U16	usStartIdx = 0;
    U16  ii = 0;
    U16  jj = 0;
    DEVICE_TEI_LIST_t DeviceListTemp;
	U8  *pnum=NULL;

    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

	if(ucReadNum<=0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	if(ucReadNum>64)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_DATALEN_ERRCODE, port); //无信息可读
		return;
	}

	app_printf("usStartIdx = %d\n",usStartIdx);

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
    Gw3762updata.CtrlField.TransMode 	= pGw3762Data_t->CtrlField.TransMode;
    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		    = 0;
    Gw3762updata.Afn				    = APP_GW3762_AFN10;
    Gw3762updata.Fn				        = app_gw3762_fn_bin_to_bs(APP_GW3762_F33);
    Gw3762updata.DataUnitLen			= 2 + 2 +1;
    pGW3762updataunit[0]	            = (U8)(APP_GETDEVNUM() & 0xFF);
    pGW3762updataunit[1]                = (U8)(APP_GETDEVNUM() >> 8);
	__memcpy(&pGW3762updataunit[2],(U8 *)&usStartIdx,2);
	pGW3762updataunit[4]  	            = 0;//ucReadNum;
    pnum = &pGW3762updataunit[4];
    pGW3762updataunit += 5;

	//起始序号为0，读集中器地址
	if(((U16)pGw3762Data_t->DataUnit[0]|((U16)pGw3762Data_t->DataUnit[1] << 8))==0)
	{
		__memcpy(device_addr,FlashInfo_t.ucJZQMacAddr,6);

		ChangeMacAddrOrder(device_addr);

		__memcpy(pGW3762updataunit, device_addr, 6); //6字节地址+两字节信息
		memset(pGW3762updataunit+6,0,18);
		__memcpy(pGW3762updataunit+24,device_addr,6);
		pGW3762updataunit += 30;
        Gw3762updata.DataUnitLen	+= 30;
		*pnum += 1;
		if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
		{
			goto send;
		}
	}

	for(jj=0;jj<MAX_WHITELIST_NUM;jj++)
	{
		DeviceList_ioctl(DEV_GET_ALL_BYSEQ,&jj, &DeviceListTemp);

		if(0==memcmp(DeviceListTemp.MacAddr,default_addr,6))//无效地址
		{
			continue;
		}

		__memcpy(device_addr,DeviceListTemp.MacAddr,6);

		if(ii == usStartIdx-1)//当前为读取的起始位置
		{
            __memcpy(pGW3762updataunit, device_addr, 6); //6字节地址+两字节信息
			memset(pGW3762updataunit+6,0,18);
			__memcpy(pGW3762updataunit+24,device_addr,6);
			pGW3762updataunit += 30;
            Gw3762updata.DataUnitLen	+= 30;
            *pnum += 1;
			if(*pnum == ucReadNum || Gw3762updata.DataUnitLen>=APP_GW3762_DATA_UNIT_LEN-8)
			{
				goto send;
			}
		}
		else if(ii < usStartIdx-1)//已读过的地址
		{
			ii++;//查找当前读取到的地址位置
		}
	}

	if(*pnum == 0)
	{
		app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		return;
	}
	send:

    app_printf("report num : %d len : %d\n",*pnum,Gw3762updata.DataUnitLen);
    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_up_afn10_f40_query_id_info(U8 *nodeAddr, U8 deviceType, U8 IdType, U8 *data, U16 len, MESG_PORT_e port)
{
    U16 Gw3762SendDataLen = 0;
    APPGW3762DATA_t Gw3762updata;
    U8 *pGW3762updataunit = Gw3762updata.DataUnit;
    U8  byLen = 0;

    Gw3762updata.CtrlField.TransDir 	= APP_GW3762_UP_DIR;
    Gw3762updata.CtrlField.StartFlag 	= APP_GW3762_SLAVE_PRM;
	Gw3762updata.CtrlField.TransMode = APP_GW3762_TRANS_MODE;

    app_gw3762_up_info_field_slave_pack(&(Gw3762updata.UpInfoField), 0, NULL,AppGw3762DownData.DownInfoField.FrameNum);
    Gw3762updata.AddrFieldNum 		= 0;
    Gw3762updata.Afn				= APP_GW3762_AFN10;
    Gw3762updata.Fn				= app_gw3762_fn_bin_to_bs(APP_GW3762_F40);

    pGW3762updataunit[byLen++] = deviceType;
    __memcpy(&pGW3762updataunit[byLen], nodeAddr, 6);
    byLen += 6;
    pGW3762updataunit[byLen++] = IdType;
    
    pGW3762updataunit[byLen++] = len;
    
    if(len > 0)
    {
        __memcpy(&pGW3762updataunit[byLen], data, len);
        byLen += len;
    }

    Gw3762updata.DataUnitLen = byLen;

    app_gw3762_up_frame_encode(&Gw3762updata, Gw3762SendData, &Gw3762SendDataLen);
    send_gw3762_frame(Gw3762SendData, Gw3762SendDataLen, AppGw3762DownData.DownInfoField.FrameNum, FALSE, 0, port,e_Serial_AFN10XX);
}

static void app_gw3762_up_afn10_f40_query_id_proc(void *pTaskPrmt, void *pUplinkData)
{
    QUERY_IDINFO_CFM_t  *pQueryIdInfoCfm = (QUERY_IDINFO_CFM_t*)pUplinkData;
    multi_trans_t *pTask = pTaskPrmt;
    U8  idInfoLen;
    U8  deviceType;
    U8  *IdInfo = zc_malloc_mem(100, "IdInfo", MEM_TIME_OUT);
    U8  byLen = 0;
    
    if(pQueryIdInfoCfm->Status == e_SUCCESS)
    {
        idInfoLen = pQueryIdInfoCfm->Asdu[byLen++];
        __memcpy(IdInfo, &pQueryIdInfoCfm->Asdu[byLen], idInfoLen);
        if(pQueryIdInfoCfm->IdType == APP_GW3762_CHIP_ID)
        {
            if(PROVINCE_ANHUI == app_ext_info.province_code)
            {
                U8  chipID_flase[24];
                U8  chipID_null[24] = { 0 };
                memset(chipID_flase, 0xff, sizeof(chipID_flase));
                if(memcmp(IdInfo,chipID_null,24) == 0)
                {
                    __memcpy(IdInfo,chipID_flase,24);
                }
            }
        }
        byLen += idInfoLen;

        deviceType = pQueryIdInfoCfm->Asdu[byLen++];


        
        //U8 state =0;
        //U8 macaddr[6]={0};
        ChangeMacAddrOrder(pQueryIdInfoCfm->SrcMacAddr);
		app_printf("Con daddr : ");
		dump_buf(pQueryIdInfoCfm->DstMacAddr, 6);
		
		app_printf("Con saddr : ");
		dump_buf(pQueryIdInfoCfm->SrcMacAddr, 6);
		
		
        app_gw3762_up_afn10_f40_query_id_info(pQueryIdInfoCfm->SrcMacAddr, deviceType, pQueryIdInfoCfm->IdType, IdInfo, idInfoLen, pTask->MsgPort);
    }
    
    zc_free_mem(IdInfo);
    return;
}

static void app_gw3762_afn10_f40_query_module_id(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{    
	app_printf("app_gw3762_afn10_f40_query_module_id!\n");
    U8   deviceType;
    U8   nodeAddr[6];
    U8   IdType;
    U8   byLen = 0;
	U8   fidemeterflag=FALSE;

    deviceType = pGw3762Data_t->DataUnit[byLen++];
    __memcpy(nodeAddr, &pGw3762Data_t->DataUnit[byLen], 6);
    byLen += 6;
    IdType = pGw3762Data_t->DataUnit[byLen++];

    app_printf("deviceType = %d, IdType = %d\n", deviceType, IdType);

    if(deviceType == APP_GW3762_RD_CTRL || deviceType == APP_GW3762_CCO_MODULE)
    {
        if(IdType == APP_GW3762_CHIP_ID)
        {
            if(PROVINCE_ANHUI == app_ext_info.province_code)
            {
                U8  chipID_flase[24];
                U8  chipID_null[24] = { 0 };
                memset(chipID_flase, 0xff, sizeof(chipID_flase));
                if(memcmp(ManageID,chipID_null,24) == 0)
                {
                    __memcpy(ManageID,chipID_flase,24);
                }
            }
            app_gw3762_up_afn10_f40_query_id_info(nodeAddr, deviceType, IdType, ManageID, sizeof(ManageID), port);
        }
        else if(IdType == APP_GW3762_MODULE_ID)
        {
            app_gw3762_up_afn10_f40_query_id_info(nodeAddr, deviceType, IdType, ModuleID, sizeof(ModuleID), port);
        }
        else
        {
            ;
        }
    }
    else
    {
        fidemeterflag = check_whitelist_by_addr(nodeAddr);
		
		if(fidemeterflag == FALSE)
		{
			app_gw3762_up_afn00_f2_deny(APP_GW3762_ADDRNONE_ERRCODE, port); //无信息可读
		    return;
		}
		ChangeMacAddrOrder(nodeAddr);
        
        id_info_query(0, nodeAddr, IdType, port,  app_gw3762_up_afn10_f40_query_id_proc);

    }   

    return;
}

static void app_gw3762_afn10_f112_chip_id(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
	app_printf("app_gw3762_afn10_f112_chip_id!\n");
    U8	ucReadNum = 0;
    U16	usStartIdx = 0 ;
    
    usStartIdx = (U16)pGw3762Data_t->DataUnit[0];
    usStartIdx += (U16)pGw3762Data_t->DataUnit[1] << 8;
    ucReadNum = pGw3762Data_t->DataUnit[2];
	app_printf("usStartIdx = %d\n",usStartIdx);

    if(ucReadNum <= 0U)
    {
        app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
        return;
    }
	/*
    if(ucReadNum > 32U)
    {
        ucReadNum = 32U;
    }
    */
    
    if(PROVINCE_SHANGHAI == app_ext_info.province_code) //todo: PROVINCE_SHANGHAI
    {
        app_gw3762_up_afn10_fn_by_whitelist_up_frame(APP_GW3762_F112, (U8)pGw3762Data_t->CtrlField.TransMode, usStartIdx, ucReadNum, port);
    }
    else
    {
    	app_gw3762_up_afn10_fn_up_frame(APP_GW3762_F112 , (U8)pGw3762Data_t->CtrlField.TransMode , usStartIdx , ucReadNum, port);
    }
}

static void app_gw3762_afn03_f21_query_concurrent_status(APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
{
    //app_gw3762_up_afn00_f1_sure(APP_GW3762_Y_CMDSTATE, APP_GW3762_IDLE_CHANSTATE, 5, port);
    app_gw3762_up_afn00_f2_deny(APP_GW3762_DATAUNIT_ERRCODE, port); //无信息可读
}

/*************************************************************************
 * 函数名称	: 	app_gw3762_down_frame_decode
 * 函数说明	: 	GW3762帧解码
 * 参数说明	: 	U8 *data			            - 数据缓存
 *				APPGW3762DATA_t *pGw3762Data_t	- 解析gw3762data帧的数据
 * 返回值	：  TRUE-解析正确；FALSE-解析有错误
 *************************************************************************/
static U8 app_gw3762_down_frame_decode(U8 *data, APPGW3762DATA_t *pGw3762Data_t)
{
    U8 i;
    U8 *pdata = data;
    U16 gw3762len = 0;

    pdata += APP_GW3762_HEAD_LEN;

    gw3762len = ((U16)(*pdata)) | (((U16)(*(pdata + 1))) << 8);
    if(gw3762len > APP_GW3762_DATA_UNIT_LEN)
    {
        return FALSE;
    }
    pdata += APP_GW3762_LEN_LEN;

    pGw3762Data_t->CtrlFieldAll = *pdata;
    if(APP_GW3762_DOWN_DIR != pGw3762Data_t->CtrlField.TransDir)
    {
        return FALSE;
    }
    pdata += APP_GW3762_CTRL_LEN;

    __memcpy(pGw3762Data_t->InfoFieldAll, pdata, APP_GW3762_INFO_LEN);
    pdata += APP_GW3762_INFO_LEN;

    if(pGw3762Data_t->DownInfoField.ModuleFlag)
    {
        if(gw3762len < APP_GW3762_BASE_LEN + APP_GW3762_ADDR_LEN * 2)
        {
            return FALSE;
        }
        __memcpy(pGw3762Data_t->AddrField.SrcAddr, pdata, MAC_ADDR_LEN);
        pdata += APP_GW3762_ADDR_LEN;

        for(i = 0 ; i < pGw3762Data_t->DownInfoField.RelayLevel ; i ++)
        {
            if(gw3762len < APP_GW3762_BASE_LEN
                    + APP_GW3762_ADDR_LEN * (2 + pGw3762Data_t->DownInfoField.RelayLevel))
            {
                return FALSE;
            }
            __memcpy(pGw3762Data_t->AddrField.RelayAddr[i], pdata, MAC_ADDR_LEN);
            pdata += APP_GW3762_ADDR_LEN;
        }

        __memcpy(pGw3762Data_t->AddrField.DestAddr, pdata, MAC_ADDR_LEN);
        pdata += APP_GW3762_ADDR_LEN;

        pGw3762Data_t->AddrFieldNum = pGw3762Data_t->DownInfoField.RelayLevel + 2;
    }
    else
    {
        pGw3762Data_t->AddrFieldNum = 0;
    }

    pGw3762Data_t->Afn = *pdata;
    pdata += APP_GW3762_AFN_LEN;

    pGw3762Data_t->Fn = ((U16)(*pdata)) | (((U16)(*(pdata + 1))) << 8);
    pdata += APP_GW3762_FN_LEN;

    pGw3762Data_t->DataUnitLen = gw3762len - (pdata - data) - 2;

    //app_printf("AFN: %02x , FN: F%02x\n", pGw3762Data_t->Afn,  pGw3762Data_t->Fn);

    //app_printf("Data unit len :%d\n", pGw3762Data_t->DataUnitLen);

    if(gw3762len < APP_GW3762_BASE_LEN
            + APP_GW3762_ADDR_LEN
            * ((pGw3762Data_t->DownInfoField.ModuleFlag) * 2 + pGw3762Data_t->DownInfoField.RelayLevel))
    {
        return FALSE;
    }

    if(pGw3762Data_t->DataUnitLen != 0)
    {
        __memcpy(pGw3762Data_t->DataUnit, pdata, pGw3762Data_t->DataUnitLen);
    }

    return TRUE;
}

 /**
 * @brief	GW3762帧编码
 * @param	APPGW3762DATA_t         *pGw3762Data_t      下行数据
 * @param   U8                      *data               数据缓存
 * @param   U16                     *len                数据长度
 * @return	void
 *******************************************************************************/
U8 app_gw3762_up_frame_encode(APPGW3762DATA_t *pGw3762Data_t, U8 *data, U16 *len)
{
    U8 *pdata = data;
    U16 tmplen;
    *pdata++ = APP_GW3762_HEAD_VALUE;
    pdata += APP_GW3762_LEN_LEN;
    *pdata++ = pGw3762Data_t->CtrlFieldAll;
    __memcpy(pdata, pGw3762Data_t->InfoFieldAll, APP_GW3762_INFO_LEN);
    pdata += APP_GW3762_INFO_LEN;

    if(pGw3762Data_t->AddrFieldNum >= 2)
    {
        __memcpy(pdata, pGw3762Data_t->AddrField.SrcAddr, APP_GW3762_ADDR_LEN);
        pdata += APP_GW3762_ADDR_LEN;

        if(pGw3762Data_t->AddrFieldNum != 2)
        {
            __memcpy(pdata, pGw3762Data_t->AddrField.RelayAddr, (pGw3762Data_t->AddrFieldNum - 2) * APP_GW3762_ADDR_LEN);
        }

        pdata += (pGw3762Data_t->AddrFieldNum - 2) * APP_GW3762_ADDR_LEN;
        __memcpy(pdata, pGw3762Data_t->AddrField.DestAddr, APP_GW3762_ADDR_LEN);
        pdata += APP_GW3762_ADDR_LEN;
    }

    *pdata++ = pGw3762Data_t->Afn;
    *pdata++ = (U8)pGw3762Data_t->Fn;
    *pdata++ = (U8)(pGw3762Data_t->Fn >> 8);

    if(pGw3762Data_t->DataUnitLen != 0)
    {
        __memcpy(pdata, pGw3762Data_t->DataUnit, pGw3762Data_t->DataUnitLen);
    }

    pdata += pGw3762Data_t->DataUnitLen;
    tmplen = pdata - data;
    *pdata++ = check_sum(data + 3, tmplen - 3);
    *pdata++ = APP_GW3762_END_VALUE;
    //*(U16*)(data + APP_GW3762_HEAD_LEN) = *len = pdata - data;
    *len = pdata - data;
    data[APP_GW3762_HEAD_LEN] = (U8)((*len) & 0xFF);
    data[APP_GW3762_HEAD_LEN + 1] = (U8)((*len) >> 8);

    return TRUE;
}

#if ((GW376213_PROTO_EN > 0) || (GW376209_PROTO_EN > 0))
AppGw3762ProcFunc AppGw3762ProcArray[] =
{
    /* 00 */{0x00000000, app_gw3762_reserve							},

    /* 01 */{0x00000001, app_gw3762_afn00_f1_sure						},
    /* 02 */{0x00000002, app_gw3762_afn00_f2_deny						},

    /* 03 */{0x00010001, app_gw3762_afn01_f1_hard_init					},
    /* 04 */{0x00010002, app_gw3762_afn01_f2_param_init					},
    /* 05 */{0x00010003, app_gw3762_afn01_f3_data_init					},

    /* 06 */{0x00020001, app_gw3762_afn02_f1_data_trans					},

    /* 07 */{0x00030001, app_gw3762_afn03_f1_manuf_code				},
    /* 08 */{0x00030002, app_gw3762_afn03_f2_noise						},
    /* 09 */{0x00030003, app_gw3762_afn03_f3_slave_monitor				},
    /* 10 */{0x00030004, app_gw3762_afn03_f4_master_addr				},
    /* 11 */{0x00030005, app_gw3762_afn03_f5_master_state_speed			},
    /* 12 */{0x00030006, app_gw3762_afn03_f6_master_disturb				},
#if (GW376213_PROTO_EN > 0)
    /* 13 */{0x00030007, app_gw3762_afn03_f7_slave_timeout				},	/* 13 */
    /* 14 */{0x00030008, app_gw3762_afn03_f8_query_rf_param				},	/* 13 */
    /* 15 */{0x00030009, app_gw3762_afn03_f9_query_slave_timeout			},	/* 13 */
    /* 16 */{0x0003000A, app_gw3762_afn03_f10_master_mode				},	/* 13 */
    /* 17 */{0x0003000B, app_gw3762_afn03_f11_master_index				},	/* 13 */
    /* 17 */{0x0003000C, app_gw3762_afn03_f12_master_module_id_info				},	/* 13 */
    /* 60 */{0x00030010, app_gw3762_afn03_f16_get_band                  },
    /* 60 */{0x00030011, app_gw3762_afn03_f17_get_hrf_band              },
    /* 18 */{0x00030064, app_gw3762_afn03_f100_query_rssi_threshold		},	/* 13 */
#endif
    /* 66 */{0x00030065, app_gw3762_afn03_f101_query_con_err 		    },
    /* 66 */{0x00030015, app_gw3762_afn03_f21_query_concurrent_status   },

    /* 19 */{0x00040001, app_gw3762_afn04_f1_send_test					},
    /* 20 */{0x00040002, app_gw3762_afn04_f2_slave_roll					},
#if (GW376213_PROTO_EN > 0)
    /* 21 */{0x00040003, app_gw3762_afn04_f3_master_test				},	/* 13 */
#endif

    /* 22 */{0x00050001, app_gw3762_afn05_f1_set_master_addr				},
    /* 23 */{0x00050002, app_gw3762_afn05_f2_set_slave_report			},
    /* 24 */{0x00050003, app_gw3762_afn05_f3_start_bc					},
#if (GW376213_PROTO_EN > 0)
    /* 25 */{0x00050004, app_gw3762_afn05_f4_set_slave_max_timeout		},	/* 13 */
    /* 26 */{0x00050005, app_gw3762_afn05_f5_set_rf_param				},	/* 13 */
    /* 26 */{0x0005000A, app_gw3762_afn05_f10_set_baudrate				},	/* 13 */
    /* 27 */{0x00050064, app_gw3762_afn05_f100_set_rssi_threshold			},	/* 13 */
    /* 28 */{0x00050065, app_gw3762_afn05_f101_set_master_time			},	/* 13 */
#endif
    /* 59 */{0x00050010, app_gw3762_afn05_f16_set_band					},
    /* 59 */{0x00050011, app_gw3762_afn05_f17_set_hrf_band              },
    /* 65 */{0x00050006, app_gw3762_afn05_f6_set_zone_status			},
    /* 65 */{0x00050082, app_gw3762_afn05_f130_set_zone_status          },
    /* 67 */{0x000500C8, app_gw3762_afn05_f200_set_report_new_reg       }, //河北：允许/ 禁止新增用户事件上报
    //精准校时相关
    /* 68 */{0x0005005A, app_gw3762_afn05_f90_set_clk_sw                },
    /* 69 */{0x0005005C, app_gw3762_afn05_f92_exact_time_calibrate      },
    /* 70 */{0x0005005D, app_gw3762_afn05_f93_set_clock_maintain        },
    /* 71 */{0x0005005E, app_gw3762_afn05_f94_set_over_time_report_value},


    /* 29 */{0x00100001, app_gw3762_afn10_f1_query_slave_num			},
    /* 30 */{0x00100002, app_gw3762_afn10_f2_query_slave_info			},
    /* 31 */{0x00100003, app_gw3762_afn10_f3_query_slave_relay			},
    /* 32 */{0x00100004, app_gw3762_afn10_f4_router_state				},
    /* 33 */{0x00100005, app_gw3762_afn10_f5_fail_slave_info				},
    /* 34 */{0x00100006, app_gw3762_afn10_f6_reg_slave_info				},
    /* 34 */{0x00100007, app_gw3762_afn10_f7_module_id			},
    /* 61 */{0x00100009, app_gw3762_afn10_f9_net_work_size				},
#if (GW376213_PROTO_EN > 0)
    /* 35 */{0x00100064, app_gw3762_afn10_f100_query_net_scale			},	/* 13 */
    /* 36 */{0x00100065, app_gw3762_afn10_f101_query_slave_info			},	/* 13 */
    /* 36 */{0x00100068, app_gw3762_afn10_f104_query_slave_info		},	/* 13 */
    /* 58 */{0x0010006f, app_gw3762_afn10_f111_neigh_net						},
#endif
    /* 57 */{0x00100014, app_gw3762_afn10_f20_read_rf_topo				},
    /* 57 */{0x00100015, app_gw3762_afn10_f21_read_topo					},
    /* 61 */{0x0010001F, app_gw3762_afn10_f31_query_phase				},
    /* 61 */{0x00100020, app_gw3762_afn10_f32_query_plc_channel_quality	},
    /* 61 */{0x00100021, app_gw3762_afn10_f33_query_id_info				},
    /* 57 */{0x00100067, app_gw3762_afn10_f103_proc						},
    /* 61 */{0x00100028, app_gw3762_afn10_f40_query_module_id			},
    /* 61 */{0x00100070, app_gw3762_afn10_f112_chip_id                  },
    //精准校时相关
    /* 72 */{0x0010005A, app_gw3762_afn10_f90_query_clk_sw_and_over_valude},
    /* 73 */{0x0010005D, app_gw3762_afn10_f93_query_clokc_maintain_cycle},

    /* 37 */{0x00110001, app_gw3762_afn11_f1_add_slave					},
    /* 38 */{0x00110002, app_gw3762_afn11_f2_del_slave					},
    /* 39 */{0x00110003, app_gw3762_afn11_f3_set_slave_relay				},
    /* 40 */{0x00110004, app_gw3762_afn11_f4_set_work_mode				},
    /* 41 */{0x00110005, app_gw3762_afn11_f5_start_reg_slave				},
    /* 41 */{0x001100e1, app_gw3762_afn11_f225_start_reg_slave				},
#if (GW376213_PROTO_EN > 0)
    /* 42 */{0x00110006, app_gw3762_afn11_f6_stop_reg_slave				},	/* 13 */
    /* 43 */{0x00110064, app_gw3762_afn11_f100_set_net_scale				},	/* 13 */
    /* 44 */{0x00110065, app_gw3762_afn11_f101_start_net_maintain			},	/* 13 */
    /* 45 */{0x00110066, app_gw3762_afn11_f102_start_net_work			},	/* 13 */
#endif

    /* 46 */{0x00120001, app_gw3762_afn12_f1_restart					},
    /* 47 */{0x00120002, app_gw3762_afn12_f2_suspend					},
    /* 48 */{0x00120003, app_gw3762_afn12_f3_recover					},

    /* 49 */{0x00130001, app_gw3762_afn13_f1_monitor_slave				},

    /* 50 */{0x00140001, app_gw3762_afn14_f1_route_req_read				},
    /* 51 */{0x00140002, app_gw3762_afn14_f2_router_req_clock			},	/* 13 */
#if (GW376213_PROTO_EN > 0)
    /* 52 */{0x00140003, app_gw3762_afn14_f3_router_req_revise			},	/* 13 */
#endif
    /* 53 */{0x00140004, app_gw3762_afn14_f4_request_ac_sampling			},

    /* 53 */{0x00150001, app_gw3762_afn15_f1_file_trans					},

    /* 53 */{0x00200001, app_gw3762_afn20_f1_start_rwm					},
    /* 53 */{0x00200002, app_gw3762_afn20_f2_stop_rwm					},
    /* 53 */{0x00200003, app_gw3762_afn20_f3_report_wm					},
    /* 53 */{0x00200004, app_gw3762_afn20_f4_report_end					},

    /* 56 */{0x00F10001, app_gw3762_afnf1_f1_concurrent						},

    /* 54 */{0x00F00005, app_gw3762_afnf0_f5_debug                      },
    /* 62 */{0x00F00006, app_gw3762_afnf0_f6_debug },
    /* 63 */{0x00F00007, app_gw3762_afnf0_f7_debug },
    /* 64 */{0x00F00008, app_gw3762_afnf0_f8_debug },
    /* 64 */{0x00F00009, app_gw3762_afnf0_f9_debug },
    /* 64 */{0x00F0000A, app_gw3762_afnf0_f10_debug },
    /* 64 */{0x00F0000B, app_gw3762_afnf0_f11_debug },
    /* 64 */{0x00F0000C, app_gw3762_afnf0_f12_debug },
    /* 64 */{0x00F0000D, app_gw3762_afnf0_f13_debug },
    /* 64 */{0x00F0000E, app_gw3762_afnf0_f14_debug },
    /* 64 */{0x00F0000F, app_gw3762_afnf0_f15_debug },
};
#endif

/*************************************************************************
 * 函数名称	: 	app_gw3762_proc
 * 函数说明	: 	Gw3762协议处理函数
 * 参数说明	: 	U8 afn			- afn
 *				U32 fn			- fn
 *				APPGW3762DATA_t *pGw3762Data_t	- 解析GW3762帧的数据
 *              MESG_PORT_e port    - 消息类型
 * 返回值	: 	无
 *************************************************************************/
static void app_gw3762_proc(U8 afn, U32 fn, APPGW3762DATA_t *pGw3762Data_t, MESG_PORT_e port)
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
    count = sizeof(AppGw3762ProcArray) / (2 * sizeof(U32));

    for(i = 0 ; i < count ; i++)
    {
        if(index == (U32)(AppGw3762ProcArray[i].Index))
        {
            AppGw3762ProcArray[i].Func(pGw3762Data_t, port);
            return;
        }
    }

    app_gw3762_ex_proc(afn, fn, pGw3762Data_t, port);
}
#endif
/**************************************************************************/

/*************************************************************************
 * 函数名称	: 	void proc_uart_2_gw13762_data
 * 函数说明	: 	串口处理本地协议报文回调接口
 * 参数说明	: 	uint8_t *revbuf			- 原始报文
 *				uint16_t len			- 报文长度
 * 返回值		: 无
 *************************************************************************/
void proc_uart_2_gw13762_data(uint8_t *revbuf,uint16_t len)
{
#if defined(STATIC_MASTER)
    // 拆包解析集中器下行帧
    if(!app_gw3762_down_frame_decode(revbuf, &AppGw3762DownData))
    {
        //if error ， how to deal?
        return;
    }
	
    // 根据AFN 及FN 找到对应376.2协议指令
    app_printf("AFN: %02x , FN: F%04x\n", AppGw3762DownData.Afn, AppGw3762DownData.Fn);
    app_gw3762_proc(AppGw3762DownData.Afn, AppGw3762DownData.Fn, &AppGw3762DownData, e_UART1_MSG);	
#endif
}

void proc_app_gw13762_data(uint8_t *revbuf,uint16_t len)
{
#if defined(STATIC_MASTER)

    if((TMR_RUNNING == zc_timer_query(g_SendsyhtimeoutTimer)) && getHplcTestMode() !=RD_CTRL)
    {
        app_printf("Rcv PLC_GW3762\n");
        timer_start(g_SendsyhtimeoutTimer);
    }

    // 拆包解析集中器下行帧
    if(!app_gw3762_down_frame_decode(revbuf, &AppGw3762DownData))
    {
        //if error ， how to deal?
        return;
    }

    // 根据AFN 及FN 找到对应376.2协议指令
    app_printf("AFN: %02x , FN: F%04x\n", AppGw3762DownData.Afn, AppGw3762DownData.Fn);
    app_gw3762_proc(AppGw3762DownData.Afn, AppGw3762DownData.Fn, &AppGw3762DownData, e_APP_MSG);
#endif
}

static U8 gw3762_frame_match(U8 sendprtcl, U8 rcvprtcl, U8 sendFrameSeq, U8 recvFrameSeq)
{
    if(sendprtcl == rcvprtcl && sendFrameSeq == recvFrameSeq)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

uint16_t get_DeviceTimeout(U8 retryTimes)
{
    uint16_t DeviceTimeout = 0;
    
    if (retryTimes != 0)
    {
        // SHANGHAI 上海停复电重复上报间隔10s, 重庆事件间隔10s,河南事件间隔5s
        if (PROVINCE_SHANGHAI == app_ext_info.province_code || PROVINCE_CHONGQING == app_ext_info.province_code)
        {
            DeviceTimeout = 10 * 100; // 10s
        }
        else if (PROVINCE_HENAN == app_ext_info.province_code)
        {
            DeviceTimeout = 5 * 100; // 5s
        }
        else
        {
            DeviceTimeout = 2 * 100; // 其他省份默认2s
        }
    }
    else
    {
        DeviceTimeout = 2 * 100; // 默认2s
    }

    return DeviceTimeout;
}

/**
* @brief	本地协议帧发送接口
* @param	U8              *buffer            帧数据
* @param    U16             len                数据长度
* @param    U8              seqNumber          帧序号
* @param    U8              active             ack标志
* @param    U8              retryTimes         重发次数
* @param    MESG_PORT_e     port               消息端口
* @param    SERIAL_LID_e    lid                优先级
* @return	void
*******************************************************************************/
void send_gw3762_frame(U8 *buffer, U16 len, U8 seqNumber, U8 active, U8 retryTimes, MESG_PORT_e port, SERIAL_LID_e lid)
{
    if(port == e_APP_MSG)       //回复抄控器
    {
        if(zc_timer_query(g_SendsyhtimeoutTimer) == TMR_RUNNING)
        {
            proc_rd_ctrl_send_data(buffer, len);
        }
    }
    else
    {
        add_serialcache_msg_t serialmsg;
        memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));
        //printf_s("SendLocalFrame-------------------------\n");
        serialmsg.RevDel = NULL; //PrintSendStatus;
        serialmsg.Uartcfg =NULL;
        serialmsg.SendEvent = NULL;
        serialmsg.ReSendEvent = NULL;
        serialmsg.MsgErr = NULL;
        serialmsg.Timeout = NULL;
        serialmsg.RevDel = NULL;
        serialmsg.Match = gw3762_frame_match;
        serialmsg.IntervalTime = SERIAL_CACHE_LIST.nr >0 ?1:0;

        serialmsg.state = UNACTIVE;
        serialmsg.Protocoltype = GW3762;
        serialmsg.DeviceTimeout = get_DeviceTimeout(retryTimes);
        serialmsg.ack = active==TRUE?1:0;
        serialmsg.FrameLen = len;
        serialmsg.FrameSeq = seqNumber;
        serialmsg.lid = lid;
        serialmsg.ReTransmitCnt = retryTimes;
        serialmsg.pSerialheader = &SERIAL_CACHE_LIST;

        extern ostimer_t *serial_cache_timer;
        serialmsg.CRT_timer = serial_cache_timer;

        serial_cache_add_list(serialmsg, buffer,TRUE);
    }
}

#endif
