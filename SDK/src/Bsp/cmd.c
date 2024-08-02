/*
 * Copyright: (c) 2006-2016, 2015 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        cmd.c
 * Purpose:     Command functions entry
 * History:     4/30/2007, created by jetmotor.
 * Usage:       You must put yourself command here as below!
 */

#include <string.h>
#include <stdio.h>
#include "vsh.h"
#include "version.h"
#include "types.h"
#include "unicorn_ver.h"
#include "cmd.h"
#include "ZCsystem.h"
#include "app_sysdate.h"
#include "trios.h"
#include "SchTask.h"
#include "dl_mgm_msg.h"
#include "dev_manager.h"
#include "printf_zc.h"

extern void docmd_reboot(command_t  *cmd, xsh_t *xsh);
extern void docmd_version(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_os_desc;
extern void docmd_os(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_level_desc;
extern void docmd_level(command_t *cmd, xsh_t *xsh);
#if defined(UNICORN8M) || defined(ROLAND9_1M)
#ifdef INET_LWIP
extern cmd_desc_t cmd_tftp_desc;
extern void docmd_tftp(command_t *cmd, xsh_t *xsh);
extern cmd_desc_t cmd_ping_desc;
extern void docmd_ping(command_t *cmd, xsh_t *xsh);
extern cmd_desc_t cmd_lwip_desc;
extern void docmd_lwip(command_t *cmd, xsh_t *xsh);
#endif
#endif


extern cmd_desc_t cmd_phy_demo_desc;
extern void docmd_phy_demo(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_meter_desc;
extern void docmd_meter(command_t *cmd, xsh_t *xsh);

//extern cmd_desc_t cmd_htmr_demo_desc;
//extern void docmd_htmr_demo(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_flash_demo_desc;
extern void docmd_flash_demo(command_t *cmd, xsh_t *xsh);


extern cmd_desc_t cmd_net_desc;
extern void docmd_net(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_macs_desc;
extern void docmd_macs(command_t *cmd, xsh_t *xsh);


extern cmd_desc_t cmd_app_desc;
extern void docmd_app(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_test_desc;
extern void docmd_test(command_t *cmd, xsh_t *xsh);

//extern cmd_desc_t cmd_si4438_desc;
//extern void docmd_si4438(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_read_desc;
extern void docmd_read(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_write_desc;
extern void docmd_write(command_t *cmd, xsh_t *xsh);


extern cmd_desc_t cmd_dltest_desc;
extern void docmd_dltest(command_t *cmd, xsh_t *xsh);

#if defined(STD_DUAL)
extern cmd_desc_t cmd_wphy_desc;
extern void docmd_wphy(command_t *cmd, xsh_t *xsh);

extern cmd_desc_t cmd_cal_desc;
extern void docmd_cal(command_t *cmd, xsh_t *xsh);

#endif

extern cmd_desc_t cmd_ecc_desc;
extern void docmd_ecc(command_t *cmd, xsh_t *xsh);
extern cmd_desc_t cmd_sm2_desc;
extern void docmd_sm2(command_t *cmd, xsh_t *xsh);
extern cmd_desc_t cmd_sha256_desc;
extern void docmd_sha256(command_t *cmd, xsh_t *xsh);


VIEW_TABLE_DEFINE_BEG()
#if defined(STATIC_MASTER)
	VIEWTAB_ENTRY(root, "@master>> "),
#else
	VIEWTAB_ENTRY(root, "@slave>> "),
#endif
VIEW_TABLE_DEFINE_END()

char *get_sw_version()
{
	return VERSION;
}

char * get_boot_version()
{
	return (char *)sys_info->boot_version;
}


const U16 get_zc_sw_version()
{
    #if defined (ZC3750STA)
	return ((ZC3750STA_ver1<<8)|ZC3750STA_ver2);
	#elif defined(ZC3750CJQ2)
	return ((ZC3750CJQ2_ver1<<8)|ZC3750CJQ2_ver2);
    #elif defined(STATIC_MASTER)
	return ((ZC3951CCO_ver1<<8)|ZC3951CCO_ver2);
	#endif
}

char *get_build_time()
{
	return __DATE__" "__TIME__;
}

char *get_hw_version()
{
#if defined(ZC3750STA) 
    #if defined(ROLAND1_1M) 
	    return "ZC3750ASTA "UNICORN_VER;
    #elif defined(VENUS2M) 
        #if defined(V233L_3780)
        return "ZC3780STA "UNICORN_VER;
        #elif defined(RISCV)
        return "ZC3780STA RISCV "UNICORN_VER;
        #elif defined(VENUS_V3)
        return "ZC3780USTA V3 "UNICORN_VER;
        #else
        return "ZC3780STA VENUS "UNICORN_VER;
        #endif
    #else
        return "ZC3750STA "UNICORN_VER;
    #endif
#elif  defined(ZC3750CJQ2) 
        
    #if defined(ROLAND1_1M) 
        return "ZC3750ACJQ2 "UNICORN_VER;
    #elif defined(VENUS2M) 
        #if defined(V233L_3780)
        return "ZC3780CJQ2 "UNICORN_VER;
        #elif defined(RISCV)
        return "ZC3780CJQ2 RISCV "UNICORN_VER;
        #elif defined(VENUS_V3)
        return "ZC3780UCJQ2 V3 "UNICORN_VER;
        #else
        return "ZC3780CJQ2 VENUS "UNICORN_VER;
        #endif
    #else
        return "ZC3750CJQ2 "UNICORN_VER;
    #endif

#elif defined(STATIC_MASTER)
	
	#if defined(ROLAND9_1M) 
         return "ZC3951ACCO "UNICORN_VER;
	#elif defined(MIZAR9M)
		return "MIZAR9M" UNICORN_VER;
	#elif defined(VENUS8M) 
        #if defined(V233L_3780)
        return "ZC3981CCO "UNICORN_VER;
        #elif defined(RISCV)
        return "ZC3981CCO RISCV "UNICORN_VER;
        #elif defined(VENUS_V3)
        return "ZC3981UCCO V3 "UNICORN_VER;
        #else
        return "ZC3981CCO VENUS "UNICORN_VER;
        #endif
    #else
         return "ZC3951CCO "UNICORN_VER;
    #endif

#else
#error  "Build Properties, Missing ASIC_VERSION or FPGA_VERSION ..."
#endif
}


char InnerVersionStr[50];
char OutVersionStr[50];


char *get_inner_version()
{
    char   *Str = InnerVersionStr;


    #if defined (ZC3750STA)
    
    __snprintf(Str, sizeof(InnerVersionStr), "%c%c%c%c%c-%02X%c%02x-R%02x%02x%c.%02x BulidTime: 20%02d-%02d-%02d", Vender1, Vender2, PRODUCT_func, CHIP_code,
                             ZC3750STA_type, ZC3750STA_prtcl, POWER_OFF, ZC3750STA_prdct, ZC3750STA_Innerver1, ZC3750STA_Innerver2,PROPERTY, app_ext_info.province_code,InnerDate_Y,InnerDate_M,InnerDate_D);

    #elif defined(ZC3750CJQ2)

    __snprintf(Str, sizeof(InnerVersionStr), "%c%c%c%c%c-%02X%c%02x-R%02x%02x%c.%02x BulidTime: 20%02d-%02d-%02d", Vender1, Vender2, PRODUCT_func, CHIP_code,
                             ZC3750CJQ2_type, ZC3750CJQ2_prtcl, POWER_OFF, ZC3750CJQ2_prdct, ZC3750CJQ2_Innerver1, ZC3750CJQ2_Innerver2,PROPERTY, app_ext_info.province_code,InnerDate_Y,InnerDate_M,InnerDate_D);

    #elif defined(STATIC_MASTER)

    __snprintf(Str, sizeof(InnerVersionStr), "%c%c%c%c%c-%02X%c%02x-R%02x%02x%c.%02x BulidTime: 20%02d-%02d-%02d", Vender1, Vender2, PRODUCT_func, CHIP_code,
                             ZC3951CCO_type, ZC3951CCO_prtcl, POWER_OFF, ZC3951CCO_prdct, ZC3951CCO_Innerver1, ZC3951CCO_Innerver2,PROPERTY, app_ext_info.province_code,InnerDate_Y,InnerDate_M,InnerDate_D);

    #endif
    

    return Str; 
}
char *get_outer_version()
{
    char   *Str = OutVersionStr;
	
	__snprintf(Str, sizeof(OutVersionStr), "%c%c%c%c-V%02x%02x            OuterTime: 20%02x-%02x-%02x", DefSetInfo.OutMFVersion.ucVendorCode[1], DefSetInfo.OutMFVersion.ucVendorCode[0],DefSetInfo.OutMFVersion.ucChipCode[1], DefSetInfo.OutMFVersion.ucChipCode[0],
		DefSetInfo.OutMFVersion.ucVersionNum[1], DefSetInfo.OutMFVersion.ucVersionNum[0],(DefSetInfo.OutMFVersion.ucYear),(DefSetInfo.OutMFVersion.ucMonth),(DefSetInfo.OutMFVersion.ucDay)); 

    return Str; 
}

int convert_string(uint8_t *buf, char *cmd, int len)
{
    uint8_t high, low;
    uint8_t flag = 1;
    int ret = 0;

    while (len--)
    {
        if (*cmd == '\"' || *cmd == ' ')
        {
            cmd++;
            continue;
        }

        if (flag)
        {
            high = (*cmd >= 'a') ? (*cmd++ - 0x57) :
                   (*cmd >= 'A') ? (*cmd++ - 0x37) : (*cmd++ - '0');
            flag = 0;
        }
        else
        {
            low = (*cmd >= 'a') ? (*cmd++ - 0x57) :
                  (*cmd >= 'A') ? (*cmd++ - 0x37) : (*cmd++ - '0');
            *buf++ = (high << 4 | low);
            flag = 1;
            ret++;
        }
    }

    return ret;
}

extern char *get_flash_hw_version();
void docmd_version(command_t *cmd, xsh_t *xsh)
{
	tty_t *term = xsh->term;


    //获取系统当前时间
    SysDate_t SysDate = {0};
    
	GetBinTime(&SysDate);

    term->op->tprintf(term, "province      : %02X\n", app_ext_info.province_code);
    term->op->tprintf(term, "SDKversion    : %s\n",   get_sw_version());
	term->op->tprintf(term, "Software      : %04x\n", get_zc_sw_version());
	term->op->tprintf(term, "Build time    : %s\n", get_build_time());
	term->op->tprintf(term, "Hardware      : %s\n", get_hw_version());
	term->op->tprintf(term, "Inner version : %s\n", get_inner_version());
    term->op->tprintf(term, "Outer version : %s\n", get_outer_version());
    term->op->tprintf(term, "Def Hardware  : %s\n", get_flash_hw_version());
    term->op->tprintf(term, "ManageID      : ");
	dump_tprintf(term,ManageID,sizeof(ManageID));
    term->op->tprintf(term, "ModuleID      : ");
    dump_tprintf(term,ModuleID,sizeof(ModuleID));
    term->op->tprintf(term, "Edgetype      : %s\n",DefSetInfo.Hardwarefeature_t.edgetype ==1?"DN":
								DefSetInfo.Hardwarefeature_t.edgetype ==2?"UP":
								DefSetInfo.Hardwarefeature_t.edgetype ==3?"DOUBLE":
									"UK");
	term->op->tprintf(term, "Devicetype    : %s\n",
						DefSetInfo.Hardwarefeature_t.Devicetype == e_CKQ ?"e_CKQ":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_JZQ ?"e_JZQ":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_METER ?"e_METER":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_RELAY ?"e_RELAY":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_CJQ_1 ?"CJQ1":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_CJQ_2 ?"CJQ2":
						DefSetInfo.Hardwarefeature_t.Devicetype == e_3PMETER ?"3XB":"Uk");
    term->op->tprintf(term, "PHY_SFO_DEF   : %lf\n", PHY_SFO_DEF);
    term->op->tprintf(term, "FreqBand      : %d\n", DefSetInfo.FreqInfo.FreqBand);
    term->op->tprintf(term, "RfBand        : %d-%d\n", DefSetInfo.RfChannelInfo.option, DefSetInfo.RfChannelInfo.channel);
    term->op->tprintf(term, "dig   ana     : %d    %d\n", DefSetInfo.FreqInfo.tgaindig,DefSetInfo.FreqInfo.tgainana);
	term->op->tprintf(term, "SystenRunTime : %d DAY %d HOUR %d MIN %d SEC;Clock now [ %d-%d-%d	%d:%d:%d ]\n",
		SystenRunTime/(24*3600),(SystenRunTime%(24*3600))/3600,((SystenRunTime%(24*3600))%3600)/60,(((SystenRunTime%(24*3600))%3600)%60)%60,
		SysDate.Year+2000,SysDate.Mon,SysDate.Day,SysDate.Hour,SysDate.Min,SysDate.Sec);
                       //pTm->tm_year + 1900,pTm->tm_mon + 1,pTm->tm_mday,pTm->tm_hour,pTm->tm_min,pTm->tm_sec);

    term->op->tprintf(term, "\r\n");

	return;
}

CMD_TABLE_DEFINE_BEG()
    CMDTAB_ENTRY(reboot,  docmd_reboot,    NULL),
    CMDTAB_ENTRY(version, docmd_version,   NULL),
    CMDTAB_ENTRY(os,      docmd_os,        &cmd_os_desc),
    CMDTAB_ENTRY(level,   docmd_level,	   &cmd_level_desc),
#if defined(STATIC_MASTER)&&defined(UNICORN8M)
#ifdef INET_LWIP
    CMDTAB_ENTRY(tftp, docmd_tftp, &cmd_tftp_desc),
    CMDTAB_ENTRY(ping, docmd_ping, &cmd_ping_desc),
    CMDTAB_ENTRY(lwip, docmd_lwip, &cmd_lwip_desc),
    CMDTAB_ENTRY(plc_monitor, docmd_plc_monitor, &cmd_plc_monitor_desc),
#endif
#endif
#if defined(UNICORN8M) || defined(ROLAND9_1M)
//	CMDTAB_ENTRY(ge,      root, root, docmd_ge, &cmd_ge_desc),
#endif
#if defined(STD_DUAL)
    CMDTAB_ENTRY(wphy,    docmd_wphy,	&cmd_wphy_desc),
    CMDTAB_ENTRY(cal,   docmd_cal, &cmd_cal_desc),
#endif
	//CMDTAB_ENTRY(gpio,    root, root, docmd_gpio_demo, &cmd_gpio_demo_desc),
    CMDTAB_ENTRY(phy,     docmd_phy_demo, &cmd_phy_demo_desc),
    CMDTAB_ENTRY(meter,   docmd_meter, &cmd_meter_desc),
//	CMDTAB_ENTRY(htmr,    docmd_htmr_demo, &cmd_htmr_demo_desc),
    CMDTAB_ENTRY(flash,   docmd_flash_demo, &cmd_flash_demo_desc),


#if defined(MIRACL)
    CMDTAB_ENTRY(ecc, 	  docmd_ecc,		&cmd_ecc_desc),
    CMDTAB_ENTRY(sm2, 	  docmd_sm2,		&cmd_sm2_desc),
    CMDTAB_ENTRY(sha256,  docmd_sha256,		&cmd_sha256_desc),
#endif
	
    CMDTAB_ENTRY(macs,   docmd_macs, &cmd_macs_desc),
    CMDTAB_ENTRY(net,    docmd_net, &cmd_net_desc),
    //CMDTAB_ENTRY(aps,   docmd_aps, &cmd_aps_desc),
    CMDTAB_ENTRY(app,    docmd_app, &cmd_app_desc),
    CMDTAB_ENTRY(test,   docmd_test, &cmd_test_desc),
    //CMDTAB_ENTRY(flash,   docmd_flash_demo, &cmd_flash_demo_desc),
    //CMDTAB_ENTRY(si4438, docmd_si4438, &cmd_si4438_desc),
    CMDTAB_ENTRY(read, docmd_read, &cmd_read_desc),
    CMDTAB_ENTRY(write, docmd_write, &cmd_write_desc),
    //测试用例
    CMDTAB_ENTRY(dltest, docmd_dltest, &cmd_dltest_desc),
    
CMD_TABLE_DEFINE_END()

