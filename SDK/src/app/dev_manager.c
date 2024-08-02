/*
 * Copyright: (c) 2009-2020, 2020 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	dev_manager.c
 * Purpose:	
 * History:
 * Author : WWJ
 */
#include "dev_manager.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "SchTask.h"
#include "ZCsystem.h"
#include "app_area_indentification_cco.h"
#include "app_area_indentification_sta.h"
#include "app_area_indentification_common.h"
#include "app_exstate_verify.h"
#include "ProtocolProc.h"
#include "efuse.h"
#include "app_led_sta.h"
#include "app_led_cjq.h"
#include "app_upgrade_cco.h"
#include "app.h"
#include "app_common.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"
#include "app_dltpro.h"
#include "Version.h"
#include "plc_io.h"
#include "flash.h"
#include "meter.h"
#include "app_phase_position_cco.h"
#include "SchTask.h"
#include "app_dltpro.h"
#include "app_read_cjq_list_sta.h"


extern U8 WIN_SIZE;
    
#if defined(STATIC_NODE)
    
        HARDWARE_FEATURE_t   HWDefInfo =
        {
            {Vender2,Vender1},
            PRODUCT_func,
            CHIP_code,
            ZC3750STA_type,
            'T',
            {'1' , POWER_OFF},
            {0x00 , 0x21},
            (18&0x7F)|((10<<7)&0x780)|( (16<<11)&0xF800),
            e_COLLECT_NEGEDGE,
            e_METER,
        };
    
#elif defined(ZC3750CJQ2)
    
        HARDWARE_FEATURE_t   HWDefInfo =
        {
            {Vender2,Vender1},
            PRODUCT_func,
            CHIP_code,
            ZC3750CJQ2_type,
            'T',
            {'1' , POWER_OFF},
            {0x00 , 0x21},
            (19&0x7F)|((02<<7)&0x780)|( (20<<11)&0xF800),
            e_COLLECT_NEGEDGE,
            e_METER,
        };
        
#elif defined(STATIC_MASTER)
    
        HARDWARE_FEATURE_t   HWDefInfo =
        {
            {Vender2,Vender1},
            PRODUCT_func,
            CHIP_code,
            ZC3951CCO_type,
            'T',
            {'1' , POWER_OFF},
            {0x01,0x20},
            (18&0x7F)|((12<<7)&0x780)|( (25<<11)&0xF800),
            e_COLLECT_DBLEDGE,
            e_JZQ,
        };
        
#endif


ostimer_t	*g_CheckACDCTimer; //检测AC or DC

U16     PhaseACONT =0; 
U16     PhaseBCONT =0; 
U16     PhaseCCONT =0; 
U8 checkflag =TRUE;


//U8 FactoryTestFlag = 0;
//U8 SendBUf[50] = {0};
U8 ChipID[8] = {0};

#if defined(ZC3750STA) || defined(ZC3750CJQ2)
DEVICE_PIB_t Tmp_DevicePib_t;

#elif defined(STATIC_MASTER)
CCO_SYSINFO_t Tmp_cco_info_t;
U8  DataAddrFilterStatus = FALSE;
U8  NetSenseFlag = FALSE;//离网感知标志

#endif


TIME_t record_time_t;


void recordtime(char *s,U32 p)
{
    static S32 k=0;
	if(record_time_t.s[0]==NULL&&record_time_t.time[0]==0)
	{
		k=0;
	}
	record_time_t.time[k]=p;
	record_time_t.dur[k]=k-1<0?0:p-record_time_t.time[k-1];
	record_time_t.s[k]=s;
    k=(k+1>=MAXNUMMEM)?0:k+1;
    
}



U8	CreatZeroDifflist(U8 Num, U8 *StartSeq, U8 *pList,U32 firstNTB,U8 offest)
{
	U16  enterdata , i, byte;
	U8   ByteSeq = 0;
	U8   ByteType = e_OneByte;
    U32  temp;
    //Bitlen = 0;
    //outdata = 0;
    byte = 0;
    for(i = 0; i < Num; i++)
    {
		if(i==0)
		{
			if(Zero_Result.ColloctResult[offest+i]>firstNTB)
			enterdata = (Zero_Result.ColloctResult[offest+i] -firstNTB) >> 8; //NTB差值为12位
			else
            {         
    			temp = ( firstNTB -Zero_Result.ColloctResult[offest+i])%500000;
				enterdata = ( 500000- temp)>> 8; //NTB差值为12位
            }
		}
		else
		{
			enterdata = (abs(Zero_Result.ColloctResult[offest+i] - Zero_Result.ColloctResult[offest+i-1])) >> 8; //NTB差值为12位
		}
		app_printf("enterdata = %d!\n",enterdata);
		//enterdata = (Zero_Result.ColloctResult[i] - Zero_Result.ColloctResult[i-1]) >> 8; //NTB差值为12位
		//根据NTB数量的序号计算字节序号，整字节及半字节处理
		ByteSeq = (*StartSeq *12)/8;
		
		if((*StartSeq *12)%8)
		{
			ByteType = e_HalfByte;
		}
		else
		{
			ByteType = e_OneByte;
		}
		//整字节直接计算填一个+半个；半字节先填半个+一个
		if(ByteType == e_OneByte)
		{
			pList[ByteSeq] = enterdata&0xFF;
			pList[ByteSeq+1] = (enterdata>>8)&0x0F;
		}
		else
		{
			pList[ByteSeq] |= (enterdata<<4)&0xF0;
			pList[ByteSeq+1] = (enterdata>>4)&0xFF;
		}
		
		*StartSeq += 1;
		/*
        outdata = (enterdata << Bitlen) + outdata;
        Bitlen += 12;
        while(Bitlen >= 8)
        {
            *pList = outdata;
            outdata = outdata >> 8;
            Bitlen -= 8;
            pList++;
            byte++;
        }

		*/
    }
    byte = ((*StartSeq *12)%8)?((*StartSeq *12)/8+1):(*StartSeq *12)/8;
    return byte;


}



#if defined(ZC3750STA)|| defined(ZC3750CJQ2)

/*************************************************************************
 * 函数名称	: 	ReadMeterInfo(U8 *buf, U16 len)
 * 函数说明	: 	校验和计算函数
 * 参数说明	: 	buf	- 要计算的数据的起始地址
 * 					len	- 要计算的数据的长度
 * 返回值		: 	校验和
 *************************************************************************/
 extern int32_t zc_flash_read(flash_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len);
void ReadMeterInfo()
{
	DEVICE_PIB_t ramDevPib;
	memset((U8 *)&ramDevPib, 0,sizeof(DEVICE_PIB_t));
    zc_flash_read(FLASH, STA_INFO, (U32 *)&DevicePib_t, sizeof(DEVICE_PIB_t));//DevicePib_t
    dump_buf((U8 *)&DevicePib_t, sizeof(DEVICE_PIB_t));
    app_printf("read check_sum  = 0x%02x!\n",check_sum((U8 *)&DevicePib_t,sizeof(DEVICE_PIB_t)-1));
    
#ifndef DEBUGMODE
    if(DevicePib_t.Cs != check_sum((U8 *)&DevicePib_t, sizeof(DEVICE_PIB_t) - 1)
		|| memcmp( (U8 *)&DevicePib_t, (U8 *)&ramDevPib, sizeof(DEVICE_PIB_t) ) ==0)
    {
        app_printf("check_sum error Cs = 0x%02x\n", DevicePib_t.Cs);

        DevicePib_t.BandMeterCnt = 0;
        DevicePib_t.Prtcl	=	DLT645_UN_KNOWN;
        DevicePib_t.DevType	=	e_UNKW;
        DevicePib_t.DevMacAddr[0] = 0;
        DevicePib_t.DevMacAddr[1] = 0;
        DevicePib_t.DevMacAddr[2] = 0;
        DevicePib_t.DevMacAddr[3] = 0;
        DevicePib_t.DevMacAddr[4] = 0;
        DevicePib_t.DevMacAddr[5] = 0;

        DevicePib_t.PowerOffFlag = e_power_is_ok;
        DevicePib_t.PowerEventReported = 0;
		DevicePib_t.HNOnLineFlg = FALSE;		//湖南入网认证
        DevicePib_t.MeterPrtcltype = SinglePrtcl_e;
        DevicePib_t.Modularmode = 1;   //模块模式，（默认电表模式）
        DevicePib_t.Cs = 0x0;
    }
#endif
#ifdef TH2CJQ
    if(e_IOT_C_MODE_CJQ != DevicePib_t.Modularmode && e_IOT_C_MODE_STA != DevicePib_t.Modularmode)
    {
        DevicePib_t.Modularmode = 1;
    }
#endif
    if(DevicePib_t.PowerOffFlag == e_power_on_now)// && DevicePib_t.PowerOffFlag != e_power_is_ok)
    {
        DevicePib_t.PowerOffFlag = e_power_off_last;
        
    }
	
    if(DevicePib_t.PowerOffFlag == e_power_off_now)
    {
        DevicePib_t.PowerOffFlag = e_power_off_last;
    }
	if(DevicePib_t.MeterPrtcltype < SinglePrtcl_e || DevicePib_t.MeterPrtcltype > DoublePrtcl_645_e)
	{
		DevicePib_t.MeterPrtcltype = SinglePrtcl_e;
	}
    DevicePib_t.BandMeterCnt = 0;
    app_printf("DevicePib_t.PowerOffFlag=%d,BandMeterCnt=%d!\n",DevicePib_t.PowerOffFlag,DevicePib_t.BandMeterCnt);
    DevicePib_t.PowerEventReported = FALSE;
}

U8 WriteMeterInfo(void)
{
    DEVICE_PIB_t Device_pib_temp;
    U32 state;
    __memcpy(&Device_pib_temp,&DevicePib_t,sizeof(DEVICE_PIB_t) - 1);
    Device_pib_temp.Cs = check_sum((U8 *)&Device_pib_temp, sizeof(DEVICE_PIB_t) - 1);
	app_printf("CheckSum_Write = 0x%02x!\n", Device_pib_temp.Cs);

    state = zc_flash_write(FLASH, STA_INFO, (U32)&Device_pib_temp, sizeof(DEVICE_PIB_t));
    if(state == FLASH_OK)
    {
        app_printf("staflag wirte OK Cs = 0x%02x!\n", Device_pib_temp.Cs);
    }
    else
    {
        app_printf("zc_flash_write ERROR!\n");
    }
    return state;
}
#endif

#if defined(ZC3750CJQ2)

U8 WriteCJQ2Info(void)

{
	COLLECT_INFO_ST CollectInfo_st_temp;
	U32 state;
	__memcpy(&CollectInfo_st_temp,&CollectInfo_st,sizeof(CollectInfo_st_temp) - 1);
 	CollectInfo_st_temp.Cs = CJQ_Flash_Check((U8*)&CollectInfo_st_temp, sizeof(COLLECT_INFO_ST) - 1);
    state = zc_flash_write(FLASH, STA_INFO, (U32)&CollectInfo_st_temp, sizeof(COLLECT_INFO_ST));
    if(state == FLASH_OK)
    {
        app_printf("CJQflash_write OK Cs = 0x%02x!\n", CollectInfo_st_temp.Cs) ;
    }
    else
    {
        app_printf("CJQflash_write ERROR!\n");
    }
	return state;
}



#endif

U8 FlashCheck(void)
{

	U8 ii;
#if defined(STATIC_NODE)
    
        memset((U8 *)&Tmp_DevicePib_t, 0x55, sizeof(DEVICE_PIB_t));
    
        //app_printf("flash write:");
        //dump_buf((U8 *)&Tmp_DevicePib_t, sizeof(DEVICE_PIB_t));
        if(FLASH_OK != zc_flash_write(FLASH, STA_INFO, (U32)&Tmp_DevicePib_t, sizeof(DEVICE_PIB_t)))
        {
            app_printf("fwERR\n");
            return FALSE;
        }
        
        memset((U8 *)&Tmp_DevicePib_t, 0x00, sizeof(DEVICE_PIB_t));
        os_sleep(10);
        
        if(FLASH_OK != zc_flash_read(FLASH, STA_INFO, (U32 *)&Tmp_DevicePib_t, sizeof(DEVICE_PIB_t)))
        {
            app_printf("frERR\n");
            return FALSE;
        }
        //app_printf("flash read:");
        //dump_buf((U8 *)&Tmp_DevicePib_t, sizeof(DEVICE_PIB_t));
        
        for(ii=0;ii<sizeof(DEVICE_PIB_t);ii++)
        {
            if(*(&Tmp_DevicePib_t.ResetTime+ii) != 0x55)
            {
                app_printf("FCerr\n");
                return FALSE;
            }   
            //app_printf("Tmp_DevicePib_t[%d] :%02x\n",ii,*(&Tmp_DevicePib_t.ResetTime+ii));
        }
#elif defined(STATIC_MASTER)
    
        memset((U8 *)&Tmp_cco_info_t, 0x55, sizeof(CCO_SYSINFO_t));
        if(FLASH_OK != zc_flash_write(FLASH, CCO_SYSINFO, (U32)&Tmp_cco_info_t, sizeof(CCO_SYSINFO_t)))
        {
            app_printf("sfwERR\n");
            return FALSE;
        }
        memset((U8 *)&Tmp_cco_info_t, 0x00, sizeof(CCO_SYSINFO_t));
        os_sleep(10);
        if(FLASH_OK != zc_flash_read(FLASH, CCO_SYSINFO, (U32 *)&Tmp_cco_info_t, sizeof(CCO_SYSINFO_t)))
        {
            app_printf("sfrERR!\n");
            return FALSE;
        }
        for(ii=0;ii<sizeof(CCO_SYSINFO_t);ii++)
        {
            if(*((U8 *)&Tmp_cco_info_t+ii) != 0x55)
            {
                app_printf("fcERR\n");
                return FALSE;
            }
        }
#endif    

	app_printf("FlashCheck ok\r\n");
    return TRUE;
}
void UseParameterCFG(void)
{
	app_printf("UseParameterCFG !\n");
	#if defined(STATIC_MASTER)
	WIN_SIZE = app_ext_info.param_cfg.ConcurrencySize ;
	app_printf("WIN_SIZE\t%d\t\n",WIN_SIZE);
    #else
	#if defined(ZC3750STA)
    sta_led_set_cfg_type(app_ext_info.province_code) ;
	app_printf("gl_led_cnf_type\t%02x\t\n",sta_led_get_cfg_type());
	#elif defined(ZC3750CJQ2)
    cjq_led_set_cfg_type(app_ext_info.province_code) ;
	app_printf("gl_led_cnf_type\t%02x\t\n",cjq_led_get_cfg_type());
	#endif
    #endif
}

#if defined(STATIC_MASTER)

void CheckJZQMacAddr(void)
{
    U8 null_addr[6]={0,0,0,0,0,0};
    if(memcmp(FlashInfo_t.ucJZQMacAddr,null_addr,6)==0)
    {
    	__memcpy(FlashInfo_t.ucJZQMacAddr,&ModuleID[0],6);
		app_printf("need to set JZQMacAddr by ModuleID last 6 bytes->");
		dump_buf(FlashInfo_t.ucJZQMacAddr,6);
	}
}


void UseFunctionSwitchDef(void)
{
	app_printf("UseFunctionSwitchDef !\n");
    extern EventReportStatus_e    g_EventReportStatus;
	g_EventReportStatus = app_ext_info.func_switch.EventReportSWC ;
	DataAddrFilterStatus = app_ext_info.func_switch.DataAddrFilterSWC ;
#if defined(ZC3951CCO)
    //ReUpgradeBySelf = app_ext_info.func_switch.UpgradeMode;
    NetSenseFlag = app_ext_info.func_switch.NetSenseSWC;

#endif
}


#endif

/****************************************************************************************
 * 函数名称 :	U8 ShakeFilter(U8 state , U8 totalnum,S16 lastcnt)
 * 函数说明 :	IO滤波，连续累加totalnum，认为稳定
 * 参数说明 :	U8  state 电平状态  0低 1高
 *				U8  totalnum  统计次数，计算时按照检测时间过滤，(timers*totalnum )ms
 *				S16 lastcnt 此前状态 
 * 返回值	:	e_IO_HIGH持续高电平，e_IO_LOW持续低电平，e_IO_PROCESS检测中
 ****************************************************************************************/


U8 ShakeFilter(U8 state , U8 totalnum,S16 *cnt)
{
    S16 lastcnt = *cnt;
    if(state)//高电平检测
	{
		lastcnt = lastcnt<0?0: lastcnt; // 状态迁移
		lastcnt = lastcnt<totalnum ?lastcnt+1: lastcnt; // 状态防抖
	}
	else//低电平检测成功
	{
		lastcnt = lastcnt>0?0: lastcnt; // 状态迁移
		lastcnt = lastcnt>-totalnum?lastcnt-1: lastcnt;//状态防抖
	} 
    *cnt = lastcnt;
    if(lastcnt>=totalnum)
	{
		//高电平检测成功
		return e_IO_HIGH;
	}
	if(lastcnt<=-totalnum)
	{
	    //低电平检测成功
		return e_IO_LOW;
	}
    return e_IO_PROCESS;
}
/****************************************************************************************
 * 函数名称 :	U8 TotalCounter(U8 state , U16 totalnum,U16 DiffValue,U16 *HighNum,U16 *LowNum)
 * 函数说明 :	IO检测，连续累加totalnum，认为稳定
 * 参数说明 :	U8  state 电平状态  0低 1高
 *				U8  totalnum  统计次数，计算时按照检测时间过滤，(timers*totalnum )ms
 *				U16 DiffValue  阈值
 *              U16 *HighNum  高电平次数
 *              U16 *LowNum   低电平次数
 * 返回值	:	e_IO_HIGH持续高电平，e_IO_LOW持续低电平，e_IO_PROCESS检测中
 ****************************************************************************************/


U8 TotalCounter(U8 state , U16 totalnum,U16 DiffValue,U16 *HighNum,U16 *LowNum)
{
    
   
    if(totalnum > (*HighNum+*LowNum))
    {
        if(state)//高电平检测
    	{
    		*HighNum=(*HighNum)+1;
    	}
    	else//低电平检测成功
    	{
    		*LowNum=(*LowNum)+1;
    	} 
    }
    else
    {
        if(abs(*HighNum-*LowNum)>=DiffValue)
        {
            
            if(*HighNum > *LowNum)
            {
                return e_IO_HIGH;
            }
            else
            {
                return e_IO_LOW;
            }
        }
    }
    return e_IO_PROCESS;
}

#if defined(ZC3750STA)

U8 IoCheck(void)
{
	//step1
	SelfCheckFlag = TRUE;
	app_printf("IoCheck begin\r\n");
	gpio_pins_off(&stasta, STA_CTRL);
    
	os_sleep(5);//等待信号稳定50ms
	
	timer_start(IOChecktimer);	//准备开始IO检测
	
	//step2
	
	os_sleep(15);//等待检测完成
	app_printf("IoCheck1 on \r\n");
	gpio_pins_on(&stasta, STA_CTRL);//STA切换高电平
    
	os_sleep(5);//等待信号稳定50ms
	
    timer_start(IOChecktimer);//准备开始IO检测
    
	os_sleep(15);//等待检测完成
		
    SelfCheckFlag = FALSE;
    
    app_printf("IoCheck2 off \r\n");
	return TRUE;
}


U8 PlugValue = 0;
U8 ReSetValue = 0;
U8 SETValue = 0;
U8 EVEValue = 0;


#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)

void IOCheckCB(struct ostimer_s *ostimer, void *arg)
{
    static S16 PlugCnt = 0;
    static S16 PlcRstCnt = 0;
    static S16 SETCnt = 0;
    static S16 EVECnt = 0;
    U8 ioinfo = 0;
    static U16 checknum = 0;
    //static S16 SetCnt = 0;
    
    checknum ++;
    
    ioinfo = ShakeFilter(READ_PLUG_PIN , IO_CHECK_TIME , &PlugCnt);
    PlugValue |= ioinfo;
	
	ioinfo = ShakeFilter(READ_PLCRST_PIN , IO_CHECK_TIME , &PlcRstCnt);
    ReSetValue |= ioinfo;

    ioinfo = ShakeFilter(READ_SET_PIN , IO_CHECK_TIME , &SETCnt);
    SETValue |= ioinfo;

    ioinfo = ShakeFilter(READ_EVE_PIN , IO_CHECK_TIME , &EVECnt);
    EVEValue |= ioinfo;
    /*
    if(checknum == 50)
        {gpio_pins_off(&stasta, STA_CTRL);}
    */
    if(checknum > 5)
    {
        checknum = 0;
        PlugCnt = 0;
        PlcRstCnt = 0;
        SETCnt = 0;
        EVECnt = 0;
        timer_stop(IOChecktimer,TMR_NULL);
        app_printf("PlugValue = %d,ReSetValue = %d,SETValue = %d,EVEValue = %d\n",PlugValue,ReSetValue,SETValue,EVEValue);
    }
    
}
#endif
#endif


#if defined(STA_BOARD_3_0_02)

U8 CheckLedCtrlData(U8 *databuf,U8 buflen,U8 *revbuf,U8 *revlen)
{
    if(FactoryTestFlag == 1)
    {
#if defined(ZC3750STA)
        U8  state = 0;
        *revlen = 0;
        if(databuf[0] == 0 && databuf[1] == 0)
        {
            revbuf[(*revlen)++] = 1;
            revbuf[(*revlen)++] = 10;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlR.LedState;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlR.LedOnTime>>8&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlR.LedOnTime&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlR.LedOffTime>>8&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlR.LedOffTime&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlT.LedState;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlT.LedOnTime>>8&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlT.LedOnTime&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlT.LedOffTime>>8&0xFF;
            revbuf[(*revlen)++] = led_ctrl_ex.ExCtrlT.LedOffTime&0xFF;
            
            state = 1;
        }
        else if(databuf[0] == 1 && databuf[1] == 10)
        {
            //gl_led_state = e_EX_CTRL;
            led_set_module_state(e_EX_CTRL);
            led_ctrl_ex.ExCtrlR.LedState      = databuf[2];
            led_ctrl_ex.ExCtrlR.LedOnTime     = databuf[3]<<8|databuf[4];
            led_ctrl_ex.ExCtrlR.LedOffTime   = databuf[5]<<8|databuf[6];
            led_ctrl_ex.ExCtrlT.LedState      = databuf[7];
            led_ctrl_ex.ExCtrlT.LedOnTime     = databuf[8]<<8|databuf[9];
            led_ctrl_ex.ExCtrlT.LedOffTime   = databuf[10]<<8|databuf[11];
            
            revbuf[(*revlen)++] = 1;
            revbuf[(*revlen)++] = 0;
            state = 1;
        }
        app_printf("ExCtrl %d R %d Ontm %d Offtm  %d T %d Ontm %d Offtm  %d\n",state
        ,led_ctrl_ex.ExCtrlR.LedState
        ,led_ctrl_ex.ExCtrlR.LedOnTime
        ,led_ctrl_ex.ExCtrlR.LedOffTime
        ,led_ctrl_ex.ExCtrlT.LedState
        ,led_ctrl_ex.ExCtrlT.LedOnTime
        ,led_ctrl_ex.ExCtrlT.LedOffTime);
        return state;
#else
        return 0;
#endif
    }
    else
    {
        return 0;
    }
}

#endif

#if defined(ZC3750STA)

U8  IO_check_timer_Init()
{
#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)

    if(IOChecktimer == NULL)
    {
        IOChecktimer = timer_create(10,
                                10,
                                TMR_PERIODIC ,//TMR_TRIGGER
                                IOCheckCB,
                                NULL,
                                "IOCheckCB"
                               );
    }
#else

#endif
    return 0;
}
   
#endif

#if defined(ZC3750STA)  


U8 checkzeroBCState(U8* IOerr)
{
    static U8  startflag=FALSE;	
    static U16 B_H_count=0;
    static U16 B_L_count=0;
    static U16 C_H_count=0;
    static U16 C_L_count=0;
    static U16 checknum = 0;
    U8  B_info = 0;
    U8  C_info = 0;
    
    if(startflag == FALSE)
	{
		startflag =TRUE;
        checknum = 0;
        B_H_count=0;
        B_L_count=0;
        C_H_count=0;
        C_L_count=0;
    }
    checknum ++;
    if(checknum > (MAXBCCHECKNUM+5))
    {
        checknum = 0;
        startflag=FALSE;
        *IOerr = 0;
        app_printf("B or C ,check over!!!!!!!!\n");
        app_printf("B_H_count=%d,B_L_count=%d,C_H_count=%d,C_L_count=%d,DiffValue=%d \n",B_H_count,B_L_count,C_H_count,C_L_count,BCCHECKDIFFNUM);
        return TRUE;
    }
    B_info = TotalCounter(ZERO_B_PIN,MAXBCCHECKNUM,BCCHECKDIFFNUM,&B_H_count,&B_L_count);
    C_info = TotalCounter(ZERO_C_PIN,MAXBCCHECKNUM,BCCHECKDIFFNUM,&C_H_count,&C_L_count);
    if(B_info == e_IO_HIGH||C_info == e_IO_HIGH)
    {
        *IOerr = B_info | C_info<<1;
        app_printf("e_IO_HIGH,IOerr=%d\n",*IOerr);
        app_printf("B_H_count=%d,B_L_count=%d,C_H_count=%d,C_L_count=%d,DiffValue=%d \n",B_H_count,B_L_count,C_H_count,C_L_count,BCCHECKDIFFNUM);
        startflag=FALSE;
    }
	else
    {
        return FALSE;
    }   
	if(*IOerr >= 4)
	{
		//gpio_set_dir(NULL, ZeroB, GPIO_OUT);
		//gpio_set_dir(NULL, ZeroC, GPIO_OUT);
		//gpio_pins_off(NULL, ZeroB);
		//gpio_pins_off(NULL, ZeroC);
		app_printf("B or C zeorIO err,set's e_BC_CHECK_1PMETER!!!!!!!!\n");
	}


	return TRUE;


}
ostimer_t	*CheckMeterTypeTimer;   //检测单相或三相
U8	checkBCzeroflag = FALSE;             //检测BC的标志
U8  DeviceTypeBCcheck = 0;          //自动检测单三相的结果

void CheckMeterTypeTimerCB(struct ostimer_s *ostimer, void *arg)
{
    if( checkBCzeroflag ==TRUE)
	{
		U8 ioerr = 0;
		if(checkzeroBCState(&ioerr))//校验完成
		{
			checkBCzeroflag =FALSE;
			if(ioerr >= 4)
            {
                DeviceTypeBCcheck = e_BC_CHECK_1PMETER;
            }
            else
            {
                DeviceTypeBCcheck = e_BC_CHECK_3PMETER;
                
            }
            app_printf("DeviceType BCcheck %d\n",DeviceTypeBCcheck);
            timer_stop(CheckMeterTypeTimer,TMR_NULL);
		}
        
	}
}

void Check_MeterType_timer_init()
{
    if(CheckMeterTypeTimer == NULL)
    {
        CheckMeterTypeTimer = timer_create(10,
                                10,
                                TMR_PERIODIC,
                                CheckMeterTypeTimerCB,
                                NULL,
                                "CheckMeterTypeTimerCB"
                               );
		//timer_start(CheckMeterTypeTimer);
    }
    else
    {
        timer_modify(CheckMeterTypeTimer,10,
                  10,
				  TMR_PERIODIC,
				  CheckMeterTypeTimerCB,
				  NULL,
                  "CheckMeterTypeTimerCB",
                  TRUE);
    }
}


void Check_MeterType_timer_start()
{
    Check_MeterType_timer_init();
    timer_stop(CheckMeterTypeTimer,TMR_NULL);
    checkBCzeroflag = TRUE;
    timer_start(CheckMeterTypeTimer);
}

#endif	

//间隔5分钟检测一次，一次检测时间为20*50
#if defined(STATIC_NODE)
extern S8 ZeroLostOrGetCnt;
#endif
void CheckACDCTimerCB(struct ostimer_s *ostimer, void *arg)
{
    if(HPLC.testmode != NORM)
    {
        return;
    }
	#if defined(STATIC_MASTER)
    static U8 check_fg = FALSE;
    #endif
	static U8 repeat=0;
	if(checkflag== TRUE)
	{
		checkflag= FALSE;
		PhaseACONT =0; 
		PhaseBCONT =0; 
		PhaseCCONT =0; 
		nnib.PhaseJuge = 0; //过零中断启动检测, 
		// 如果等待0，则PhaseACONT等会在相应的过零中断中被加1
		timer_modify(g_CheckACDCTimer,900,   // 500ms
				  900,
				  TMR_TRIGGER,
				  CheckACDCTimerCB,
				  NULL,
                  "g_CheckACDCTimer",
                  TRUE);
		 timer_start(g_CheckACDCTimer);
	}
	else
	{
		checkflag=  TRUE;
		nnib.PhaseJuge = 1; //关闭判断过零检测
		net_printf("PhaseCONTA=%d,B=%d,C=%d\n",PhaseACONT,PhaseBCONT,PhaseCCONT);
		nnib.PhasebitA =PhaseACONT>25? TRUE:FALSE;
		nnib.PhasebitB =PhaseBCONT>25? TRUE:FALSE;
		nnib.PhasebitC =PhaseCCONT>25? TRUE:FALSE;
		PhaseACONT=PhaseBCONT =PhaseCCONT=0;
#if defined(STATIC_MASTER)
    if(FALSE == check_fg)
    {
		cco_phase_calculate_order();
        check_fg = TRUE;
    }
#endif
        #if defined(STATIC_NODE)
        extern S8 ZeroLostOrGetCnt;
        if(ZeroLostOrGetCnt >= 10)
        {
            // 正在停电过程中，延后判定强电检测
    		timer_modify(g_CheckACDCTimer,900,   // 500ms
    				  900,
    				  TMR_TRIGGER,
    				  CheckACDCTimerCB,
    				  NULL,
                      "g_CheckACDCTimer",
                      TRUE);
    		timer_start(g_CheckACDCTimer);
		}
		else
	#endif
		{
			if((nnib.PhasebitA+nnib.PhasebitB+nnib.PhasebitC)>=1)
			{
				
				nnib.PowerType =TRUE;
#if defined(ZC3750STA)
				if(nnib.DeviceType == e_UNKW) //falsh中如果没有存储型号，使用过零判断一次
				{
					if((nnib.PhasebitB+nnib.PhasebitC) >= 1)nnib.DeviceType = e_3PMETER;
				}
#endif
			}
			else
			{
				if(repeat==0)
				{
					repeat =1;
					checkflag= FALSE;
					nnib.PhaseJuge = 0; //过零中断启动检测
					timer_start(g_CheckACDCTimer);
					
				}
				nnib.PowerType =FALSE;
			}

			timer_modify(g_CheckACDCTimer,30*1000,
					  30*1000,
					  TMR_TRIGGER,
					  CheckACDCTimerCB,
					  NULL,
                      "g_CheckACDCTimer",
                      TRUE);
			timer_start(g_CheckACDCTimer);
			app_printf("PhasebitA=%d,B=%d,C=%d,PwTp=%d\n",nnib.PhasebitA,nnib.PhasebitB,nnib.PhasebitC,nnib.PowerType);
		}
	}
}
void Check_ACDC_timer_init()
{
    if(g_CheckACDCTimer == NULL)
    {
        g_CheckACDCTimer = timer_create(240,
                                30*1000,
                                TMR_TRIGGER,
                                CheckACDCTimerCB,
                                NULL,
                                "g_CheckACDCTimer"
                               );
		timer_start(g_CheckACDCTimer);
    }
    else
    {
        timer_modify(g_CheckACDCTimer,240,
				  30*1000,
				  TMR_TRIGGER,
				  CheckACDCTimerCB,
				  NULL,
                  "g_CheckACDCTimer",
                  TRUE);
    }
}

void Check_ACDC_timer_start()
{
    if(TMR_RUNNING == zc_timer_query(g_CheckACDCTimer))
    {
        return;
    }
    Check_ACDC_timer_init();
    timer_stop(g_CheckACDCTimer,TMR_NULL);
    timer_start(g_CheckACDCTimer);
}



#if defined(STATIC_MASTER)


U32 GetDataFormZeroDifflist(U8 SeqNum, U8 *pList)
{
    U16		bitLen, byteNum;
    U32		DiffTime;
    
    bitLen = SeqNum * 12;
    byteNum = bitLen / 8;

    if(bitLen % 8)
    {
        // 被8模不为0的，取起始字节高4位为低4位，下1字节为高8位
        DiffTime = (pList[byteNum] >> 4) + (pList[byteNum + 1] << 4);
    }
    else
    {
        // 被8模为0的，取起始字节为低8位，下1字节的低4位为高4位值
        DiffTime = (pList[byteNum]) + ((pList[byteNum + 1] & 0x0f) << 8);
    }
    DiffTime = DiffTime << 8;
    
    return DiffTime;
}


//-------------------------------------------------------------------------------------------------
//  A down    B down    C down
//    0         1         2
//  A  up     B  up     C  up
//    3         4         5
//-------------------------------------------------------------------------------------------------

static U8 get_edge_by_time(U32 last_tick, U32 now_tick, U8 zc_edge)
{
	U8 edge=0xff;
    
	U32 period = (now_tick-last_tick)/25;

	if(zc_edge >= 3)
	{
		return edge;
	}
	if(period >= 3333 - DIFF_RANGE && period <= 3333 + DIFF_RANGE)
	{
        // A下降沿后为C上升沿，B下降沿后为A上升沿，C下降沿后为B上升沿
		edge = (zc_edge == 0)?5:(zc_edge + 2);		
	}
	else if(period >= 9999 - DIFF_RANGE && period <= 9999 + DIFF_RANGE)
	{
		// 每相下降沿半周期后，就是本相的上升沿
		edge = zc_edge + 3;
	}
	else
	{
		edge = 0xff;
	}

	return edge;
	
}
#endif


U8 zc_edge=0xff;

static U8 state_filter(U32 event)
{    gpio_dev_t *dev  = NULL;    
	switch(event)    
	{    
		case EVT_ZERO_CROSS0:       //A    
		#if defined(UNICORN8M)
             dev = &zc;
        #else
			 dev = &zca;
        #endif
			 break;
    #if defined(ROLAND1_1M) || defined(ROLAND9_1M) || defined(MIZAR1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
		case EVT_ZERO_CROSS1:       //B        
			 dev = &zcb;        
		     break;    
		case EVT_ZERO_CROSS2:       //C        
			 dev = &zcc;        
			 break;
        #if (defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS8M)) && defined(ZC3951CCO)
		case EVT_ZERO_CROSS3:       //0        
             dev = &zc;
			 break;
		#endif
	#endif    
		default:        break;    
	}
    if(dev == NULL)        
	return TRUE;

	
    if((gpio_input(dev, dev->pins) & dev->pins) == 0)        
	return TRUE;
	
    return FALSE;
}




void meter_serial_cfg(uint32_t baudrate, uint8_t verif)
{
	app_printf("verif = %d,baudrate:%d\n",verif,baudrate);
    if(baudrate == 0||baudrate > BAUDRATE_460800)
    {
        app_printf("baudrate too big\n");
        return;
    }
	MeterUartInit(g_meter, baudrate);

}

#if defined(ROLAND1_1M) || defined(ROLAND9_1M) || defined(MIZAR1M)|| defined(MIZAR9M) || defined(VENUS2M)|| defined(VENUS8M)

__isr__	void ZeroCrossISR (phy_evt_param_t *param, void *arg)
{
	#if defined(STATIC_MASTER)
	if(zc_edge == 0 || zc_edge == 1 || zc_edge == 2)  // CCO为6路过零检测
    {
        U8 edge = 0xff;
        
        edge = get_edge_by_time(ZeroData[zc_edge].NTB[ZeroData[zc_edge].NewOffset], param->stime, zc_edge);
        
        recordtime(edge == 3?"a":edge == 4?"b":edge == 5?"c":"U",param->stime);
        if(edge == 0xff || edge > 5)
        {
            return;
        }
        zc_edge = edge;
        ZeroData[zc_edge].NewOffset = (ZeroData[zc_edge].NewOffset+1) % MAXNTBCOUNT;
        ZeroData[zc_edge].NTB[ZeroData[zc_edge].NewOffset] = param->stime;
    }
	#endif
}


__isr__	void ZeroCrossISRA (phy_evt_param_t *param, void *arg)
{
    ZERODATA_t *zeroData = (ZERODATA_t*)arg;
    #if defined(STATIC_MASTER)
    if(gl_cco_phase_order == FALSE)
    {
        if(zeroData == &ZeroData[e_B_PHASE-1])
        {
            zeroData = &ZeroData[e_C_PHASE-1];
        }
        else if(zeroData == &ZeroData[e_C_PHASE-1])
        {
            zeroData = &ZeroData[e_B_PHASE-1];
        }
    }
#endif
    U32 timer = PHY_TICK2US(param->stime-zeroData->record_time);
//    app_printf("DN>%d,event=%d\n",PHY_TICK2US(param->stime),param->event);


    if(zeroData->record_time == 0)
    {
        zeroData->record_time = param->stime ;
        return;
    }
	if(timer>25000)
    {
        net_printf("timer:%d\n",timer);
    }
  	if(timer < 2000)
  	{
  		//printf_s("err time =%d",timer);
		return;
  	}

	if(state_filter(param->event))		return;

    zeroData->record_time = param->stime ;

//    app_printf("timer: %d\n", timer);



    zeroData->NewOffset = (zeroData->NewOffset+1) % MAXNTBCOUNT;
    zeroData->NTB[zeroData->NewOffset] = param->stime;


    if(param->event == EVT_ZERO_CROSS0)             //PhaseA
    {
    	zc_edge =0;
        recordtime("ZC-a-down",param->stime);
        if(nnib.PhaseJuge == 0)
        {
            PhaseACONT++;
        }
    }
#if defined(STATIC_MASTER)
    else if ((param->event == EVT_ZERO_CROSS1 && TRUE == gl_cco_phase_order) || (param->event == EVT_ZERO_CROSS2 && FALSE == gl_cco_phase_order)) // PhaseB
#else
    else if (param->event == EVT_ZERO_CROSS1)
#endif
    {
    	zc_edge =1;
        recordtime("ZC-b-down",param->stime);
        if(nnib.PhaseJuge == 0)
        {
            PhaseBCONT++;
        }
    }
#if defined(STATIC_MASTER)
    else if((param->event == EVT_ZERO_CROSS2 && TRUE == gl_cco_phase_order)||(param->event == EVT_ZERO_CROSS1 && FALSE == gl_cco_phase_order))        //PhaseC
#else
    else if (param->event == EVT_ZERO_CROSS2)
#endif
    {
    	zc_edge =2;
        recordtime("ZC-c-down",param->stime);
        if(nnib.PhaseJuge == 0)
        {
            PhaseCCONT++;
        }
    }


#if defined(ZC3750CJQ2)
    if(param->event == EVT_ZERO_CROSS0)
        zero_get_deal();
#elif defined(ZC3750STA)
    	zero_get_deal();
#endif



    return;
}
#else

extern U8  DeviceTypeDEF;
//extern U8  DeviceTypeBCcheck;


__isr__	void ZeroCrossISRA (phy_evt_param_t *param, void *arg)
{    
    static U32 time_record = 0;
    
    U32 timer = PHY_TICK2US(param->stime-time_record);//(param->stime-time_record) / 25;
    if(timer>21000)
    {
        //app_printf("------------------------lost zero cross----------------------------->%d\n",timer);
    }

    if(time_record == 0)
    {
        time_record = param->stime ;
        return;
    }

  	if(timer < 1000)
    {
        return;
    }
	if(state_filter(param->event))
    {
        return;
    }
    time_record = param->stime ;
#if defined(STATIC_MASTER)

	#if defined(NEW_CCO_BOARD) || defined(CCO_BOARD_2_0_01)
    //app_printf("%d %d %d zc_edge %d\n",ZERO_A_PIN,ZERO_B_PIN,ZERO_C_PIN,zc_edge);
    if(ZERO_B_PIN)
    {
        recordtime("ZC-b-down",param->stime);
    	
        zc_edge = 1;
        
        //net_printf("zero B phase come\n");
        ZeroData[e_B_PHASE-1].NewOffset = (ZeroData[e_B_PHASE-1].NewOffset+1)%MAXNTBCOUNT;
        ZeroData[e_B_PHASE-1].NTB[ZeroData[e_B_PHASE-1].NewOffset] = param->stime;

        if(nnib.PhaseJuge==0)  // AC/DC判断为打开
        {    
            PhaseBCONT++;
        }
    }
    else if(ZERO_C_PIN)
    {
        recordtime("ZC-c-down",param->stime);
    
        zc_edge = 2;
        
        //net_printf("zero C phase come\n");
        ZeroData[e_C_PHASE-1].NewOffset = (ZeroData[e_C_PHASE-1].NewOffset+1)%MAXNTBCOUNT;
        ZeroData[e_C_PHASE-1].NTB[ZeroData[e_C_PHASE-1].NewOffset] = param->stime;
            
        if(nnib.PhaseJuge == 0)  // AC/DC判断为打开
        {
            PhaseCCONT++;
        }    
    }
    else if(ZERO_A_PIN)
	#endif
    
#elif defined(STATIC_NODE)

    //app_printf("Node is STA\n");

	#if defined(OLD_STA_BOARD)
    
    if(ZCA_CTRL&gpio_input(NULL,ZCA_CTRL))
        
	#elif defined(STA_BOARD_2_0_01)
    
    //app_printf("defined STA_BOARD_2_0_01\n");
	#if defined(ZC3750STA)//采集器无BC模式
    //BC出现高电平时，要求至少判定配置信息不为单相或者电平常态检测不为单相时
    //app_printf("DeviceTypeDEF = %d\n", DeviceTypeDEF);
    //app_printf("DeviceTypeBCcheck = %d\n", DeviceTypeBCcheck);
    if(ZERO_B_PIN && DeviceTypeDEF == e_DEF_3PMETER)   //DeviceTypeBCcheck == e_BC_CHECK_3PMETER
    {
	#if defined(ZC3750STA)
       	#if defined(STD_2016)
        zero_get_deal();
		#endif
	#endif
        recordtime("ZC-b-down",param->stime);
        //app_printf("zero B phase come\n");
                
        ZeroData[e_B_PHASE-1].NewOffset = (ZeroData[e_B_PHASE-1].NewOffset+1)%MAXNTBCOUNT;
        ZeroData[e_B_PHASE-1].NTB[ZeroData[e_B_PHASE-1].NewOffset] =    param->stime;
        if(nnib.PhaseJuge == 0)
        {
            PhaseBCONT++;
        }    
    }
    else if(ZERO_C_PIN && DeviceTypeDEF == e_DEF_3PMETER)  // DeviceTypeBCcheck == e_BC_CHECK_3PMETER
    {
	#if defined(ZC3750STA)
       	#if defined(STD_2016)
        zero_get_deal();
		#endif
	#endif
        recordtime("ZC-c-down",param->stime);
        //app_printf("zero C phase come\n");
                
        ZeroData[e_C_PHASE-1].NewOffset = (ZeroData[e_C_PHASE-1].NewOffset+1)%MAXNTBCOUNT;
        ZeroData[e_C_PHASE-1].NTB[ZeroData[e_C_PHASE-1].NewOffset] =    param->stime;
        if(nnib.PhaseJuge == 0)
        {
            PhaseCCONT++;
        }
    }
    else            
	#endif
	#endif
#endif
    {
        //recordtime("ZC-a-down",param->stime);
	#if defined(NEW_CCO_BOARD)||defined(CCO_BOARD_2_0_01) 
        zc_edge = 0;
	#endif
        //app_printf("zero a phase come\n");
        ZeroData[e_A_PHASE-1].NewOffset = (ZeroData[e_A_PHASE-1].NewOffset+1) % MAXNTBCOUNT;
        ZeroData[e_A_PHASE-1].NTB[ZeroData[e_A_PHASE-1].NewOffset] = param->stime;
        
        if(nnib.PhaseJuge==0)  // AC/DC判断为打开
        {
            PhaseACONT++;
        }
        
        recordtime("ZC-a-down",param->stime);
        
	#if defined(ZC3750STA) || defined(ZC3750CJQ2)
       	#if defined(STD_2016)
        zero_get_deal();
		#endif
	#endif    
    }
#if defined(STATIC_MASTER) && defined(CCO_BOARD_2_0_01)
    else if(zc_edge == 0 || zc_edge == 1 || zc_edge == 2)  // CCO为6路过零检测
    {
        U8 edge = 0xff;
        
        edge = get_edge_by_time(ZeroData[zc_edge].NTB[ZeroData[zc_edge].NewOffset], param->stime, zc_edge);
        
        recordtime(edge == 3?"a":edge == 4?"b":edge == 5?"c":"U",param->stime);
        if(edge == 0xff || edge > 5)
        {
            return;
        }
        zc_edge = edge;
        ZeroData[zc_edge].NewOffset = (ZeroData[zc_edge].NewOffset+1) % MAXNTBCOUNT;
        ZeroData[zc_edge].NTB[ZeroData[zc_edge].NewOffset] = param->stime;
    }
    
#endif
}

#endif


void ReadManageID()
{
#if defined(UNICORN8M) || defined(UNICORN2M)
        
    *(uint32_t *)0xf2800000  = 0x1;
    os_sleep(1);//等读取芯片ID

    __memcpy(ManageID,(uint8_t *)0xf2800200,sizeof(ManageID));
    app_printf("3750  ManageID -> ");
    dump_buf(ManageID,sizeof(ManageID));

#elif defined(ROLAND1_1M) || defined(ROLAND9_1M) 

    *(uint32_t *)0xF2A00000  = 0x1110;
    *(uint32_t *)0xF2A00004  = 0x1;

    os_sleep(1);//等读取芯片ID
    //__memcpy(ManageID,(uint8_t *)0xF2A00200,sizeof(ManageID));
    __memcpy(ManageID,(uint8_t *)0xF2A00100,sizeof(ManageID));
    app_printf("3750A ManageID -> ");
    dump_buf(ManageID,sizeof(ManageID));
#elif defined(MIZAR1M)|| defined(MIZAR9M) || defined(VENUS2M)|| defined(VENUS8M)
    uint32_t data[6] = {0};
	memset(ManageID,0x00,24);
	efuse_read(data, 6);
	__memcpy(ManageID,data,sizeof(data));
    app_printf("3780  ManageID -> ");
    dump_buf(ManageID,sizeof(ManageID));
#endif

}
//返回0成功，1为失败
U8 read_ecc_sm2_info(U8 *pBuf,ECC_SM2_KEY_t type)
{
    U8 state = 0;
    U8 ecc_cs = 0;
    U8 sm2_cs = 0;
    ECC_SM2_KEY_STRING_t *temp_ecc_sm2_key = NULL;

    temp_ecc_sm2_key = zc_malloc_mem(sizeof(ECC_SM2_KEY_STRING_t), "flash1_ecc_sm2_info", MEM_TIME_OUT);

    if(temp_ecc_sm2_key == NULL)
    {
        app_printf("temp_ecc_sm2_key is NULL\n");
        return 1;
    }
    state = zc_flash_read(FLASH,ECC_SM2_KEY_ADDR, (uint32_t*)temp_ecc_sm2_key, sizeof(ECC_SM2_KEY_STRING_t));
    if(state != FLASH_OK)
    {
        app_printf("Read falsh res error\n");
        if(e_ECC_KEY==type)
        {
            memset(pBuf,0xff,sizeof(ECC_KEY_STRING_t));
        }
        if(e_SM2_KEY==type)
        {
            memset(pBuf,0xff,sizeof(SM2_KEY_STRING_t));
        }
        zc_free_mem(temp_ecc_sm2_key);
        return 1;
    }

    if(type == e_ECC_KEY)
    {
        dump_buf((U8*)&temp_ecc_sm2_key->ecc_key_string.buf[0],sizeof(ECC_KEY_STRING_t));
         ecc_cs = check_sum((U8*)&temp_ecc_sm2_key->ecc_key_string.buf[0],sizeof(ECC_KEY_STRING_t)-1);
        if(ecc_cs==temp_ecc_sm2_key->ecc_key_string.value.cs)
        {
            app_printf("Read falsh ecc cs ok,ecc_cs:0x%02x\n",ecc_cs);
            __memcpy(pBuf,(U8*)&temp_ecc_sm2_key->ecc_key_string.buf[0],sizeof(ECC_KEY_STRING_t)-3);
        }
        else
        {
            app_printf("Read falsh ecc cs error,ecc_cs:0x%02x,cal_cs:0x%02x\n",temp_ecc_sm2_key->ecc_key_string.value.cs,ecc_cs);
            
            memset(pBuf,0xff,sizeof(ECC_KEY_STRING_t));
            zc_free_mem(temp_ecc_sm2_key);
            return 1;
        }
    }
    if(type == e_SM2_KEY)
    {
        dump_buf((U8*)&temp_ecc_sm2_key->sm2_key_string.buf[0],sizeof(ECC_KEY_STRING_t));
        sm2_cs = check_sum((U8*)&temp_ecc_sm2_key->sm2_key_string.buf[0],sizeof(SM2_KEY_STRING_t)-1);
        if(sm2_cs==temp_ecc_sm2_key->sm2_key_string.value.cs)
        {
            app_printf("Read falsh sm2 cs ok,sm2_cs:0x%02x\n",sm2_cs);
            __memcpy(pBuf,(U8*)&temp_ecc_sm2_key->sm2_key_string.buf[0],sizeof(SM2_KEY_STRING_t)-3);
        }
        else
        {
            app_printf("Read falsh sm2 cs ok,sm2_cs:0x%02x,cal_cs:0x%02x\n",temp_ecc_sm2_key->sm2_key_string.value.cs,sm2_cs);
            memset(pBuf,0xff,sizeof(SM2_KEY_STRING_t));
            zc_free_mem(temp_ecc_sm2_key);
            return 1;
        }
    }
    zc_free_mem(temp_ecc_sm2_key);
    return 0;
}
//返回0成功，1为失败
U8 write_ecc_sm2_info(U8 *pBuf,ECC_SM2_KEY_t type,U8 validlen)
{
    U8 state = 0;
    U8 ecc_cs = 0;
    U8 sm2_cs = 0;
    ECC_SM2_KEY_STRING_t *temp_ecc_sm2_key = NULL;

    if((validlen -4) != (ECC_KEY_STRING_LEN-3))
    {
        app_printf("key len error validlen:%d\n",validlen);
        return 1;
    }
    temp_ecc_sm2_key = zc_malloc_mem(sizeof(ECC_SM2_KEY_STRING_t), "flash1_ecc_sm2_info", MEM_TIME_OUT);

    if(temp_ecc_sm2_key == NULL)
    {
        app_printf("temp_ecc_sm2_key is NULL\n");
        return 1;
    }
    state = zc_flash_read(FLASH,ECC_SM2_KEY_ADDR, (uint32_t*)temp_ecc_sm2_key, sizeof(ECC_SM2_KEY_STRING_t));
    if(state != FLASH_OK)
    {
        app_printf("Read flash error\n");
        return 1;
    }
    if(e_ECC_KEY == type)
    {
        __memcpy((INT8U*)&temp_ecc_sm2_key->ecc_key_string.buf[0],pBuf,validlen -4);
        temp_ecc_sm2_key->ecc_key_string.value.used = 'Z';
        temp_ecc_sm2_key->ecc_key_string.value.resv = 0;
        ecc_cs = check_sum((U8*)&temp_ecc_sm2_key->ecc_key_string.buf[0],sizeof(ECC_KEY_STRING_t)-1);
        temp_ecc_sm2_key->ecc_key_string.value.cs = ecc_cs;
        app_printf("New ecc check_sum  = 0x%02x!\n",ecc_cs);
    }
    else if(e_SM2_KEY == type)
    {
        
        __memcpy((INT8U*)&temp_ecc_sm2_key->sm2_key_string.buf[0],pBuf,validlen -4);
        temp_ecc_sm2_key->sm2_key_string.value.used = 'Z';
        temp_ecc_sm2_key->sm2_key_string.value.resv = 0;
        sm2_cs = check_sum((U8*)&temp_ecc_sm2_key->sm2_key_string.buf[0],sizeof(SM2_KEY_STRING_t)-1);
        temp_ecc_sm2_key->sm2_key_string.value.cs = sm2_cs;
        app_printf("New sm2 check_sum  = 0x%02x!\n",sm2_cs);
    }
    state = zc_flash_write(FLASH, ECC_SM2_KEY_ADDR, (U32)temp_ecc_sm2_key, sizeof(ECC_SM2_KEY_STRING_t));
    if(state == FLASH_OK)
    {
        app_printf("Wirte falsh ecc sm2 OK\n");
    }
    else
    {
        app_printf("Wirte falsh ecc sm2 ERROR!\n");
    }
    zc_free_mem(temp_ecc_sm2_key);
    return state;
}


