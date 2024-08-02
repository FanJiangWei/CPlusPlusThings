/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        RouterTask.c
 * Purpose:     procedure for using
 * History:
 */
#include <stdlib.h>
#include "trios.h"
#include "sys.h"
#include "vsh.h"
#include "SchTask.h"
//#include "ApsTask.h"
#include "ProtocolProc.h"
#include "ZCsystem.h"
#include "phy_sal.h"
#include "app_exstate_verify.h"
#include "aps_layer.h"
#include "sys.h"

#include "plc_io.h"
#include "app_commu_test.h"
#include "wlc.h"
#include "app_rdctrl.h"
#include "test.h"
#include "SyncEntrynet.h"
#include "Scanbandmange.h"
#include "Beacon.h"
#include "netnnib.h"
#include "app_common.h"
#include "meter.h"
#include "app_gw3762_ex.h"

////U8  scanFlag =TRUE;
//U32 testmode = 0;

//extern U32 NwkBeaconPeriodCount;//网络层记录的信标周期

//frame_ctrl_t phycb_header;
//extern int32_t frame_construct(frame_ctrl_t *head, uint8_t dt, uint8_t pbc, uint16_t dtei, uint16_t symn,
//			uint16_t hdr_symn, tonemap_t *tm);
////extern INT8S phy_tx_start_arg(uint8_t dt, uint32_t bpc,uint8_t tmi,uint8_t phase,uint16_t stei, uint16_t dtei, uint32_t stime,
////	void *pld, uint16_t pldlen, uint8_t state,uint8_t lid,uint8_t repeat,uint32_t neighbour,uint16_t dur ,uint16_t bso);
//INT8S phy_tx_start_arg(uint8_t dt, uint32_t bpc,uint8_t tmi,uint8_t phase,uint16_t stei, uint16_t dtei, uint32_t stime,
//    void *pld, uint16_t pldlen, uint8_t state,uint8_t lid,uint8_t repeat,uint32_t neighbour,uint16_t dur ,uint16_t bso)
//{
////    extern phy_demo_t PHY_DEMO;
//    extern uint8_t PHY_HDR_SYMN_DEF;
//    tonemap_t *tm;
//    phy_pld_info_t *pi;
//    phy_frame_info_t *pf;
//    bph_t *bph;
//    pbh_t *pbh;
//    int32_t ret;
//    uint32_t cpu_sr;
//	int8_t  i;
//    uint16_t  len;
//    evt_notifier_t ntf;
	
//    pf = &PHY_DEMO.pf;
//    pi = &PHY_DEMO.pf.pi;
	
//    //PHY_DEMO.delay = 0;
//    //PHY_DEMO.bp    = 0;
//    pf->hi.symn    = PHY_HDR_SYMN_DEF;
//    pf->hi.mm      = MM_QPSK;
//    pf->hi.size    = HDR_SIZE_16;
//    pi->bat        = NULL;
		
//	pf->head    = PHY_DEMO.hdr;
//	pf->payload = pld ;
	
//	//phy_debug_printf("pf->payload  :  0x%08x\n", pf->payload);
//    if (dt == DT_BEACON)
//    {
//        pf->hi.pts = TRUE;	// if pts is true, then stime is decided by phy layer,
//        pf->hi.pts_off = offset_of(beacon_ctrl_t, bts);
//    }
//    else if(dt == DT_SACK && lid == 2)
//    {
//		pf->hi.pts = TRUE;	// if pts is true, then stime is decided by phy layer,
//        pf->hi.pts_off = offset_of(sack_syh_ctrl_t, ts);
//	}
//	else
//    {
//        pf->hi.pts  = FALSE;
//    }
		
//    if(dt == DT_SOF || dt == DT_BEACON || dt == DT_SOUND)
//    {
//        if (!(tm = get_sys_tonemap(tmi)))
//        {
//            phy_debug_printf("Tone map %u doesn't exist!\n", tmi);
//            return ERROR;
//        }
//        pi->pbsz     = tm->pbsz;
//        pi->fec_rate = tm->fec_rate;
//        pi->copy     = tm->copy;
//        pi->mm       = tm->mm;
//        pi->pbc      = 0;
//        if(pld != NULL && pldlen != 0)
//        {
//            switch (pi->pbsz)
//            {
//            case PBSZ_72:
//                len = PB72_BODY_SIZE;
//                break;
//            case PBSZ_136:
//                len = PB136_BODY_SIZE;
//                break;
//            case PBSZ_264:
//                len = PB264_BODY_SIZE;
//                break;
//            case PBSZ_520:
//                len = PB520_BODY_SIZE;
//                break;
//            default:
//                break;
//            }
//            pi->pbc = (pldlen % len != 0) ? 1 : 0;
//            pi->pbc = pldlen / len + pi->pbc;
//            //phy_debug_printf("pi->pbc = %d , pldlen = %d\n",pi->pbc,pldlen);
//        }

//        pi->robo = tm->robo;

//        if (pi->pbc && (pi->pbc < tm->pbc_min || pi->pbc > tm->pbc_max))
//        {
//            phy_debug_printf("Tone map %u's PB count can not be %u([%u, %u])!\n",
//                             tmi, pi->pbc, tm->pbc_min, tm->pbc_max);
//            return ERROR;
//        }

//        if (tm->mm)
//        {
//            pi->symn = robo_pb_to_pld_sym(pi->pbc, tm->robo);
//        }
//        else
//        {
//            pi->symn = rtbat_pb_to_pld_sym(pi->pbc, tm->robo);
//            pi->bat  = tm->bat;
//        }

//        pf->length  = pi->pbc * pb_bufsize(pi->pbsz);
       
//        if (pf->payload)
//        {
//            if (dt == DT_BEACON)
//            {
//                bph        = (bph_t *)pf->payload;
//                bph->som   = 1;
//                bph->eom   = 1;
//                bph->pbsz  = pi->pbsz;
//                bph->crc   = TRUE;
//                bph->crc32 = 0;
//                if(pldlen > len)
//                {
//                    phy_debug_printf("error pldlen %d > pbsz %d \n", pldlen, len);
//                }
//                __memcpy(pf->payload + 4, pld, pldlen);
//				//dump_buf(pf->payload + 4,pldlen);
//            }
//            else
//            {
//                pbh = (pbh_t *)pf->payload;
//                for (i = 0; i < pi->pbc; ++i)
//                {
//                    pbh->som  = i == 0;
//                    pbh->eom  = i == pi->pbc - 1;
//                    pbh->pbsz = pi->pbsz;
//                    pbh->crc  = FALSE;
//                    pbh->ssn  = i;
                    
//					#if defined(STD_2012) || defined(STD_GD)
//						pbh->resv = 0;
//					#elif defined(STD_2016)
//						pbh->mfsf = i == 0;
//	                    pbh->mfef = i == pi->pbc - 1;
//					#endif
					
//					if(((test_pbh.ssn!=0)||(test_pbh.mfsf!=0)||(test_pbh.mfef!=0))&&pi->pbc==1)
//					{
//						pbh->ssn = test_pbh.ssn;
//						#if defined(STD_2016)
//						pbh->mfsf = test_pbh.mfsf;
//						pbh->mfef = test_pbh.mfef;
//						test_pbh.ssn=test_pbh.mfsf=test_pbh.mfef=0;
//						#endif
//					}
//					//p__memcpysrc = (unsigned char *)pbh + sizeof(pbh_t);
//					//p__memcpydst =  pld + i * len;
//					//memcplen  = len;
//                    __memcpy((unsigned char *)pbh + sizeof(pbh_t), pld + i * len, len);
//                    //dump_buf((unsigned char *)pbh,sizeof(pbh_t));
//                    pbh = (void *)pbh + pb_bufsize(pi->pbsz);
//                }

				
//            }
//            dhit_write_back((uint32_t)pf->payload, pf->length);
//			//phy_debug_printf("pf->length : %d\n",pf->length);
//        }
//    }
	
//	if(testmode&PHYCB||testmode&UART_PHY)
//	{
//		if(testmode&UART_PHY)
//		{
//			testmode &= ~UART_PHY;
//		}
//		__memcpy(pf->head,&phycb_header,sizeof(frame_ctrl_t));
//	}
//	else
//	{
//		if(testmode&EASY_UART_PHY)
//		{
//			testmode &= ~EASY_UART_PHY;
//		}
//		frame_construct(pf->head,dt,pi->pbc ,dtei,pf->hi.symn,pi->symn,
//		      tm );
//		phase = 1<<phase;

//		//app_printf("sack -> ");
//		//dump_buf((U8 *)(pf->head),sizeof(sack_ctrl_t));
//	}
//    //PHY_DEMO.state    = PHY_DEMO_TX;
//    if(dt != DT_BEACON && dt != DT_SACK)
//    {
//		if (stime && !(testmode&PHYCB)) stime += get_phy_tick_time();
//	}

	
//	//hplc_hrf_phy_reset();

//    //fill_evt_notifier(&ntf, PHY_EVT_TX_END, csma_tx_end, NULL);

//	cpu_sr = OS_ENTER_CRITICAL();
	
//    //ld_set_tx_phase(dt==DT_BEACON?phase:PHY_PHASE_ALL);	//
//    //ld_set_tx_phase(PHY_PHASE_ALL);
//    if(testmode&PHYCB)//回传模式下，A相发送，优化TMI遍历测试
//    {
//        ld_set_tx_phase(PHY_PHASE_A);
//    }
//    else
//    {
//        //ld_set_tx_phase(PHY_PHASE_ALL);
//        ld_set_tx_phase_zc(PHY_PHASE_A);
//    }
//    if(stime)
//    {
//		if( (S32)(stime - get_phy_tick_time()) <= 25*200 )
//		{
//			phy_debug_printf("tick = 0x%08x \n",get_phy_tick_time());
//			stime = 0;
//		}
//	}
//	/*if(dt==DT_BEACON)
//	{
//	}*/
//	phy_debug_printf("stime-tick = 0x%08x \n",stime-get_phy_tick_time());
//	if ((ret = phy_start_tx(&PHY_DEMO.pf,  (stime) ? &stime : NULL, &ntf, 1)) != OK)
//	{

//        phy_debug_printf("tick_time = 0x%08x tx failed,ret = %d\n",get_phy_tick_time(), ret);
//		if ((ret = phy_start_tx(&PHY_DEMO.pf, NULL, &ntf, 1)) != OK)
//			 phy_debug_printf("tx failed, ret = %d\n",ret);
//	}
//    OS_EXIT_CRITICAL(cpu_sr);

//	//debug_printf(&dty, DEBUG_PHY,"TDMA time : 0x%08d\n",stp-Mpib_t.BeaconTimeManager_t.slice[e_BEACON_SLICE].StartStamp);
    
//    return OK;
//}

//extern sfo_handle_t HSFO;

//U32 datalen =0 ;
////U8 databuf[2500];
//wpbh_t test_pbh ={
//            .ssn=0,
//            .mfsf=0,
//            .mfef=0,
//};

U8 rdaddr[6]={0xdd,0xdd,0xdd,0xdd,0xdd,0xdd};
U16 RD_dtei;

extern U8 TestCmdFlag;
extern ostimer_t *testtimer;

param_t cmd_test_param_tab[] =
{
    {PARAM_ARG | PARAM_TOGGLE | PARAM_OPTIONAL ,"", "phasectr|readteststate|phytpt|phycb|mactpt|band|wphytpt|wphycb|rfplccb|rfchannel\
|rdctrl|uart2phy|easyuart2phy|spy|cleartest|sendmode|rfband|networktype|RdMeter" },

    {PARAM_ARG | PARAM_TOGGLE | PARAM_SUB | PARAM_OPTIONAL, "","Aen|Ben|Cen|Adis|Bdis|Cdis", "phasectr" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "phytpt" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "phycb" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "mactpt" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "wphytpt" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "wphycb" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "rfplccb" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "0|1|2|3", "", "band" },
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "option", "","rfchannel"},
    {PARAM_ARG | PARAM_STRING | PARAM_SUB, "channel", "","rfchannel"},
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "send beacon 0|1", "", "rdctrl" },
    {PARAM_ARG | PARAM_STRING  | PARAM_SUB , "state/end flag", "","uart2phy"},
    {PARAM_ARG | PARAM_STRING  | PARAM_SUB , "PLC2016 fch+mpdu", "","uart2phy"},
    {PARAM_ARG | PARAM_STRING  | PARAM_SUB , "PLC2016 fch+machader", "","easyuart2phy"},
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "tmi", "Tone Map Index", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "pbc", "PB Number", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "msdulen", "length", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "sendcnt", "num", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "period", "/10ms", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "ssn", "ssn", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "mfsf", "startflag", "easyuart2phy" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "mfef", "endflag", "easyuart2phy" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "baudrate", "", "spy" },
    {PARAM_ARG | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "unit:min!", "", "spy" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL,      "mode",	"PLC|RF|DUAL",	"sendmode" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL,      "mode",	"PLC|RF|DUAL",	"networktype" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL, "option", "1|2|3", "rfBand" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "channel","",      "rfBand" },

    PARAM_EMPTY
};

void docmd_test(command_t *cmd, xsh_t *xsh)
{

    if (xsh->argc == 1)
    {
        return;
    }
    //    //PHY_DEMO.snr_esti = FALSE;
    if (__strcmp(xsh->argv[1], "readteststate") == 0)
    {
        test_readteststate(xsh->term);
    }
#if defined(ZC3750STA) && (defined(ROLAND1M)) // || defined(MIZAR1M) || defined(VENUS2M))

    else if (__strcmp(xsh->argv[1], "phasectr") == 0)
    {
        if (__strcmp(xsh->argv[2], "Aen") == 0)
        {
            gpio_pins_on(&sta_phase_ctr, VA_EN);
        }
        else if (__strcmp(xsh->argv[2], "Ben") == 0)
        {
            gpio_pins_on(&sta_phase_ctr, VB_EN);
        }
        else if (__strcmp(xsh->argv[2], "Cen") == 0)
        {
            gpio_pins_on(&sta_phase_ctr, VC_EN);
        }
        else if (__strcmp(xsh->argv[2], "Adis") == 0)
        {
            gpio_pins_off(&sta_phase_ctr, VA_EN);
        }
        else if (__strcmp(xsh->argv[2], "Bdis") == 0)
        {
            gpio_pins_off(&sta_phase_ctr, VB_EN);
        }
        else if (__strcmp(xsh->argv[2], "Cdis") == 0)
        {
            gpio_pins_off(&sta_phase_ctr, VC_EN);
        }
        return;
    }
#endif

    else if (__strcmp(xsh->argv[1], "cleartest") == 0)
    {
        test_cleartest();
    }
    else if (__strcmp(xsh->argv[1], "phytpt") == 0)
    {
        test_phytpt(xsh);
    }
    else if (__strcmp(xsh->argv[1], "phycb") == 0)
    {
        test_phycb(xsh);
    }
    else if (__strcmp(xsh->argv[1], "wphytpt") == 0)
    {
        test_wphytpt(xsh);
    }
    else if (__strcmp(xsh->argv[1], "wphycb") == 0)
    {
        test_wphycb(xsh);
    }
    else if (__strcmp(xsh->argv[1], "mactpt") == 0)
    {
        test_mactpt(xsh);
    }
    else if (__strcmp(xsh->argv[1], "rfplccb") == 0)
    {
        test_rfplccb(xsh);
    }
    else if (__strcmp(xsh->argv[1], "rfchannel") == 0)
    {
        test_rfchannel(xsh);
    }
    else if (__strcmp(xsh->argv[1], "band") == 0)
    {
        test_band(xsh);
    }
    else if (__strcmp(xsh->argv[1], "rfband") == 0)
    {
        test_rfband(xsh);
    }
    else if (__strcmp(xsh->argv[1], "rdctrl") == 0)
    {
        test_rdctrl();
    }
    else if (__strcmp(xsh->argv[1], "spy") == 0)
    {
        test_spy(xsh);
    }
    else if (__strcmp(xsh->argv[1], "sendmode") == 0)
    {
        test_sendmode(xsh);
    }
#if defined(STATIC_MASTER)
    else if (__strcmp(xsh->argv[1], "networktype") == 0)
    {
        test_networktype(xsh);
    }
#endif
#if defined(STATIC_NODE)
    else if (__strcmp(xsh->argv[1], "RdMeter") == 0)
    {
        U8 BC07_ReadMatter[12] = {0X68, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0X68, 0X13, 0X00, 0XDF, 0X16};
        uart1_send(BC07_ReadMatter, sizeof(BC07_ReadMatter));
    }
#endif
}


#if defined(STATIC_MASTER)
void test_networktype(xsh_t *xsh)
{
    char *s;
    if ((s = xsh_getopt_val(xsh->argv[2], "-mode=")))
    {
        if (__strcmp(s, "PLC") == 0)
        {
            nnib.NetworkType = e_SINGLE_HPLC;
        }
        else if (__strcmp(s, "RF") == 0)
        {
            nnib.NetworkType = e_SINGLE_HRF;
        }
        else if (__strcmp(s, "DUAL") == 0)
        {
            nnib.NetworkType = e_DOUBLE_HPLC_HRF;
        }
    }
}
#endif

void test_sendmode(xsh_t *xsh)
{

    char *s;
    if ((s = xsh_getopt_val(xsh->argv[2], "-mode=")))
    {
        if (__strcmp(s, "PLC") == 0)
        {
            HPLC.worklinkmode = e_link_mode_plc;
        }
        else if (__strcmp(s, "RF") == 0)
        {
            HPLC.worklinkmode = e_link_mode_rf;
        }
        else if (__strcmp(s, "DUAL") == 0)
        {
            HPLC.worklinkmode = e_link_mode_dual;
        }
    }
}

void test_readteststate(tty_t *term)
{
        U32 testmode;
        U32 time = 0;
        testmode = getHplcTestMode();
        term->op->tprintf(term, "Current test mode : %s\n", testmode == PHYTPT ? "PHY_TPT" : testmode == PHYCB      ? "PHY_CB"
                                                                                         : testmode == MACTPT       ? "MAC_TPT"
                                                                                         : testmode == WPHYTPT      ? "WPHYTPT"
                                                                                         : testmode == WPHYCB       ? "WPHYCB"
                                                                                         : testmode == RF_PLCPHYCB  ? "RF_PLCPHYCB"
                                                                                         : testmode == PLC_TO_RF_CB ? "PLC_TO_RF_CB"
                                                                                         : testmode == SAFETEST     ? "SAFETEST"
                                                                                         : testmode == RD_CTRL      ? "RD_CTRL"
                                                                                         : testmode == MONITOR      ? "MONITOR"
                                                                                         : testmode == TEST_MODE    ? "TEST_MODE"
                                                                                                                    : "Normal mode");
        term->op->tprintf(term, "Currentlinkmode<%d>:%s\n", HPLC.worklinkmode,
                          HPLC.worklinkmode == e_link_mode_plc ? "HPLC" : HPLC.worklinkmode == e_link_mode_rf ? "HRF"
                                                                      : HPLC.worklinkmode == e_link_mode_dual ? "DUAL"
                                                                                                              : "UKW");

        if (testtimer)
        {
            time = timer_remain(testtimer);
            term->op->tprintf(term, "remain time = %d\n", time);
        }
        else
        {
            term->op->tprintf(term, "The timer is not open!\n");
        }
        return;
}

void test_cleartest()
{
        U8 mode;
        mode = NORM;
        net_nnib_ioctl(NET_SET_TSTMODE, &mode);

        // setHplcTestMode(NORM);
        if (TMR_RUNNING == zc_timer_query(testtimer))
        {
            timer_stop(testtimer, TMR_NULL);
        }
        return;
}

void test_phytpt(xsh_t *xsh)
{
        U32 time = 0;
       tty_t *term = xsh->term;
        /*
            U8 mode;

            U32 perfmode = PHY_PERF_MODE_A;
            if (phy_ioctl(PHY_CMD_SET_PERF_MODE, &perfmode) != OK )
            {
                app_printf("phy ioctl set perform failed\n");
                return;
            }

            uart1testmode(BAUDRATE_115200);

            //setHplcTestMode(PHYTPT);
            mode=PHYTPT;
            net_nnib_ioctl(NET_SET_TSTMODE,&mode);

            time=__atoi(xsh->argv[2]);
            phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
            HPLC.sfo_is_done=0;
            HPLC.snid = 0;
            #if defined(STATIC_NODE)
            ScanBandManage(e_TEST);
            #else
            HPLC.scanFlag = FALSE;
            #endif
            */


        time = __atoi(xsh->argv[2]);
        if (time == 0)
        {
            time = 60;
            TestCmdFlag = TRUE;
            term->op->tprintf(term, "ALL\n");
        }
        else
        {
            term->op->tprintf(term, "A\n");
        }
        phy_tpt(time);
}

void test_phycb(xsh_t *xsh)
{
       U32 time = 0;
       tty_t *term = xsh->term;
        /*
            U8 mode;

            U32 perfmode = PHY_PERF_MODE_A;
            if (phy_ioctl(PHY_CMD_SET_PERF_MODE, &perfmode) != OK )
            {
                app_printf("phy ioctl set perform failed\n");
                return;
            }
   //        uart1testmode(BAUDRATE_115200);
           //setHplcTestMode(PHYCB);
           mode=PHYCB;
           net_nnib_ioctl(NET_SET_TSTMODE,&mode);

           time=__atoi(xsh->argv[2]);
           phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
           HPLC.sfo_is_done=0;
           HPLC.snid = 0;
           #if defined(STATIC_NODE)
           ScanBandManage(e_TEST);
           #else
           HPLC.scanFlag = FALSE;
           #endif
           */
        time = __atoi(xsh->argv[2]);
        if (time == 0)
        {
            time = 60;
            TestCmdFlag = TRUE;
            term->op->tprintf(term, "ALL\n");
        }
        else
        {
            term->op->tprintf(term, "A\n");
        }
        phy_cb(PHYCB, time);
}
void test_wphytpt(xsh_t *xsh)
{
        U32 time = 0;
        tty_t *term = xsh->term;
        /*
           U8 mode;

           U32 perfmode = PHY_PERF_MODE_A;
           if (phy_ioctl(PHY_CMD_SET_PERF_MODE, &perfmode) != OK )
           {
               app_printf("phy ioctl set perform failed\n");
               return;
           }

           uart1testmode(BAUDRATE_115200);

           //setHplcTestMode(PHYTPT);
           mode=PHYTPT;
           net_nnib_ioctl(NET_SET_TSTMODE,&mode);

           time=__atoi(xsh->argv[2]);
           phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
           HPLC.sfo_is_done=0;
           HPLC.snid = 0;
           #if defined(STATIC_NODE)
           ScanBandManage(e_TEST);
           #else
           HPLC.scanFlag = FALSE;
           #endif
           */
        time = __atoi(xsh->argv[2]);
        if (time == 0)
        {
            time = 60;
            TestCmdFlag = TRUE;
            term->op->tprintf(term, "ALL\n");
        }
        else
        {
            term->op->tprintf(term, "A\n");
        }
        wphy_tpt(time);
}

void test_wphycb(xsh_t *xsh)
{
        U32 time = 0;
        tty_t *term = xsh->term;
        /*
            U8 mode;

            U32 perfmode = PHY_PERF_MODE_A;
            if (phy_ioctl(PHY_CMD_SET_PERF_MODE, &perfmode) != OK )
            {
                app_printf("phy ioctl set perform failed\n");
                return;
            }
   //        uart1testmode(BAUDRATE_115200);
           //setHplcTestMode(PHYCB);
           mode=PHYCB;
           net_nnib_ioctl(NET_SET_TSTMODE,&mode);

           time=__atoi(xsh->argv[2]);
           phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
           HPLC.sfo_is_done=0;
           HPLC.snid = 0;
           #if defined(STATIC_NODE)
           ScanBandManage(e_TEST);
           #else
           HPLC.scanFlag = FALSE;
           #endif
           */
        time = __atoi(xsh->argv[2]);
        if (time == 0)
        {
            time = 60;
            TestCmdFlag = TRUE;
            term->op->tprintf(term, "ALL\n");
        }
        else
        {
            term->op->tprintf(term, "A\n");
        }
        wphy_cb(time);
}
void test_mactpt(xsh_t *xsh)
{
        U32 time = 0;
        tty_t *term = xsh->term;
        /*
            U8 mode;

            U32 perfmode = PHY_PERF_MODE_A;
            if (phy_ioctl(PHY_CMD_SET_PERF_MODE, &perfmode) != OK )
            {
                app_printf("phy ioctl set perform failed\n");
                return;
            }

            uart1testmode(BAUDRATE_115200);
            //setHplcTestMode(MACTPT);
            mode=MACTPT;
            net_nnib_ioctl(NET_SET_TSTMODE,&mode);

            time=__atoi(xsh->argv[2]);
            phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);

    //        extern void tonemask_cfg(U8 idx);
    //        tonemask_cfg(2);
            */
        time = __atoi(xsh->argv[2]);
        if (time == 0)
        {
            time = 60;
            TestCmdFlag = TRUE;
            term->op->tprintf(term, "ALL\n");
        }
        else
        {
            term->op->tprintf(term, "ALL\n");
        }
        mac_tpt(time);
}
void test_rfplccb(xsh_t *xsh)
{
        U32 time = 0;
        tty_t *term = xsh->term;
        time = __atoi(xsh->argv[2]);
        if (time == 0)
        {
            time = 60;
            TestCmdFlag = TRUE;
            term->op->tprintf(term, "ALL\n");
        }
        else
        {
            term->op->tprintf(term, "A\n");
        }
        phy_cb(RF_PLCPHYCB, time);
}

void test_rfchannel(xsh_t *xsh)
{
        tty_t *term = xsh->term;
        U8 option = __atoi(xsh->argv[2]);
        U8 channel = __atoi(xsh->argv[3]);

        if(checkRfChannel(option, channel) == FALSE)
        {
            term->op->tprintf(term, "test_rfchannel err, op:%d,chl:%d\n", option, channel);
            return;
        }

        if (option > 4 || option < 1)
        {
            term->op->tprintf(term, "optin:%d is err\n", option);
            return;
        }
        wphy_channel_set(option, channel);
}

void test_band(xsh_t *xsh)
{
        /*
            //changeband(__atoi(xsh->argv[2]));
            U8 bandidx=__atoi(xsh->argv[2]) ;
            net_nnib_ioctl(NET_SET_BAND,&bandidx);
    #if defined(STATIC_NODE)
            ScanBandManage(e_TEST);
    #else
            HPLC.scanFlag = FALSE;
    #endif
    */
        U8 bandidx = __atoi(xsh->argv[2]);
        phy_change_band(bandidx);
}
void test_rfband(xsh_t *xsh)
{
        tty_t *term = xsh->term;
        uint8_t option;
        uint32_t channel;

        if (xsh->argc != 4)
        {
            term->op->tprintf(term, "argc: %d err!\n", xsh->argc);
            return;
        }

        option = __strtoul(xsh_getopt_val(xsh->argv[2], "-option="), NULL, 10);
        //        term->op->tprintf(term, "set option:  %d\n", option);
        channel = __strtoul(xsh_getopt_val(xsh->argv[3], "-channel="), NULL, 10);
        //        term->op->tprintf(term, "set channel: %d\n", channel);

        wphy_channel_set(option, channel);
}

void test_rdctrl()
{
        U8 mode;
        uart1testmode(BAUDRATE_115200);
        HPLC.tei = CTRL_TEI_GW;

        //        testmode |= RD_CTRL; <<<
        // setHplcTestMode(RD_CTRL);
        mode = RD_CTRL;
        net_nnib_ioctl(NET_SET_TSTMODE, &mode);

        //		RD_ctrl_beacon_flag=__atoi(xsh->argv[2]);

        return;
}
void test_spy(xsh_t *xsh)
{
        U32 time = 0;
        tty_t *term = xsh->term;
        U8 mode;
        if (xsh->argc > 2)
        {
            time = __atoi(xsh->argv[2]);
        }
        time = time == BAUDRATE_115200 ? BAUDRATE_115200 : BAUDRATE_460800;
        term->op->tprintf(term, "time = %d\n", time);
        uart1testmode(time);
        //		testmode =  0;
        //		testmode |= SPY;
        mode = MONITOR;
        net_nnib_ioctl(NET_SET_TSTMODE, &mode);
        phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
        HPLC.sfo_is_done = 0;

        if (xsh->argc > 3)
        {
            time = __atoi(xsh->argv[3]);
        }
        term->op->tprintf(term, "time = %d\n", time);
        close_debug_level(DEBUG_PHY);
        close_debug_level(DEBUG_METER);

#if defined(STATIC_NODE)
        ScanBandManage(e_TEST, 0);
        StopThisEntryNet();
        ClearNNIB();
        nnib.AODVRouteUse = TRUE;
#else
        HPLC.scanFlag = FALSE;
        nnib.RoutePeriodTime = RoutePeriodTime_Test;
#endif

        if (phy_get_status() == PHY_STATUS_IDLE)
        {
            hplc_do_next();
        }

        if (wphy_get_status() == WPHY_STATUS_IDLE)
        {
            wl_do_next();
        }

        return;
}

void testtimerCB(struct ostimer_s *ostimer, void *arg)
{
    printf_s("testtimerCB reboot\n");

    app_reboot(500);
}
void Modifytesttimer(U32 min)
{
    extern ostimer_t *testtimer;
    if(testtimer == NULL)
    {
        testtimer = timer_create(min*60*1000,
                                 0,
                                 TMR_TRIGGER ,//TMR_TRIGGER
                                 testtimerCB,
                                 NULL,
                                 "testtimerCB"
                                );
    }
    else
    {
        if(TMR_RUNNING == zc_timer_query(testtimer))
            timer_stop(testtimer, TMR_NULL);

        timer_modify(testtimer
                     ,min*60*1000,
                     0,
                     TMR_TRIGGER ,//TMR_TRIGGER
                     testtimerCB,
                     NULL,
                     "testtimerCB",
                     TRUE);
    }

    if(testtimer)
        timer_start(testtimer);
    else
        sys_panic("<testtimer fail!> %s: %d\n");
}

/**
 * @brief spy_send
 * @param buf
 * @param len
 * @param snr
 * @param crc
 */
U16 Ms=0;
void spy_send(mbuf_t *buf, uint8_t *payload, uint16_t pdlen, uint8_t crc)
{
    uint16_t msgLen = sizeof(sof_ctrl_t) + pdlen;
    SPY_t *Spy_Msg_t = (SPY_t *)zc_malloc_mem(sizeof(SPY_t) + 2 + msgLen, "spy", MEM_TIME_OUT);

    Spy_Msg_t->header    = 0x68;
    Spy_Msg_t->CRC_state = crc;
    Spy_Msg_t->time      = Ms;
    Spy_Msg_t->LinkType  = buf->LinkType;
    Spy_Msg_t->snr       = buf->snr;
    Spy_Msg_t->len       = msgLen;

    //拷贝fch
    __memcpy(Spy_Msg_t->pMsg, buf->fi.head, sizeof(sof_ctrl_t));
    //拷贝载荷
    __memcpy(Spy_Msg_t->pMsg + sizeof(sof_ctrl_t), payload, pdlen);

    Spy_Msg_t->pMsg[Spy_Msg_t->len]=0xBB;//check_sum((U8 *)&Spy_Msg_t,len+7);
    Spy_Msg_t->pMsg[Spy_Msg_t->len + 1]=0x16;

    if(msgLen>2300)
    {
        zc_free_mem(Spy_Msg_t);
        return;
    }

    uart1_send((uint8_t *)Spy_Msg_t, sizeof(SPY_t) + Spy_Msg_t->len + 2);

    zc_free_mem(Spy_Msg_t);
}

void RdCtrlAfn13F1ReadMeterReq(U8 *data,U16 len,U8 protocol,U8 *cmnaddr)
{
	HPLC.tei=CTRL_TEI_GW;
	READ_METER_REQ_t *pReadMeterRequest_t=NULL;
	pReadMeterRequest_t= (READ_METER_REQ_t*)zc_malloc_mem(sizeof(READ_METER_REQ_t)+len,"13F1ReadMeter",MEM_TIME_OUT);

	pReadMeterRequest_t->ReadMode = e_CONCENTRATOR_ACTIVE_RM;
	
	pReadMeterRequest_t->Direction			=	e_DOWNLINK;
	pReadMeterRequest_t->Timeout			=	DEVTIMEOUT;
	pReadMeterRequest_t->PacketIner 		=	PACKET_INTER;
	pReadMeterRequest_t->ResponseState		=	0;
	pReadMeterRequest_t->OptionWord 		=	0;
	pReadMeterRequest_t->ProtocolType		=	protocol;
	pReadMeterRequest_t->DatagramSeq		=	++ApsSendPacketSeq;
	rd_ctrl_info.readseq = pReadMeterRequest_t->DatagramSeq;
	if(getHplcTestMode() == RD_CTRL)
	{
		pReadMeterRequest_t->dtei = RD_dtei;
	}
	__memcpy(pReadMeterRequest_t->DstMacAddr, cmnaddr, 6);
   
	__memcpy(pReadMeterRequest_t->SrcMacAddr, rdaddr, 6);//抄控器地址
	//pReadMeterRequest_t->Handle			=	AppPIB_t.AppHandle++;

	__memcpy(pReadMeterRequest_t->Asdu, data, len);
	pReadMeterRequest_t->AsduLength = len;
	if(pReadMeterRequest_t->dtei == 0)
	{
		pReadMeterRequest_t->SendType = e_LOCAL_BROADCAST_FREAM_NOACK;
	}
	else
	{
		pReadMeterRequest_t->SendType = e_UNICAST_FREAM;
	}

	ApsReadMeterRequest(pReadMeterRequest_t);
	
	zc_free_mem(pReadMeterRequest_t);

	//SendPoolMsg(e_APP_MSG, e_APS_MSG, e_PLC_2016, e_READ_METER_REQUEST, (U8 *)pReadMeterRequest_t);
}

CMD_DESC_ENTRY(cmd_test_desc, "", cmd_test_param_tab);
//extern void uart_send(uint8_t *buffer, uint16_t len);
void uart1testmode(U32 Baud)
{
    uint32_t lcr_val;
    uint8_t uart = g_meter->uart;

    g_meter->baudrate = Baud;
    uart_set_int_enable(uart, 0); //disable write and read intr
    uart_init(uart, Baud,PARITY_NONE,STOPBITS_1,DATABITS_8);

    lcr_val = uart_get_line_ctrl(uart);
    lcr_val &= ~LCR_PARITY_ENABLE;
    uart_set_line_ctrl(uart, lcr_val);

    uart_set_rx_fifo_threshold(g_meter->uart, g_meter->rx_fifo_rwm);
    uart_set_tx_fifo_threshold(g_meter->uart, g_meter->tx_fifo_twm);
    uart_set_rx_fifo_non_empty_time(g_meter->uart, 22000000 / g_meter->baudrate); //2 * 33bit /* 2048 * 3us */
    //debug_printf(&dty, DEBUG_PHY, "88000000/g_meter->baudrate : %d\n",22000000/g_meter->baudrate);
    U32 cnt = uart_get_rf_cnt(g_meter->uart);
    while(cnt--)
    {
        uart_get_rxbuf(g_meter->uart);
    }
    uart_set_int_enable(g_meter->uart, IER_RDA | IER_RLS | IER_RTO);//

    return ;
    
}

