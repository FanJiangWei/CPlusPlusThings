#include "phytest_demo.h"
#include "phy_sal.h"
#include "plc_cnst.h"
#include "app_crypt_test.h"
#include "wl_cnst.h"
#include "Scanbandmange.h"
#include "dl_mgm_msg.h"
#include "crc.h"

/* 台体测试指令发送步骤：
 * 1、发送切换频段指令               //每个频段发送20次
 * 2、发送切换无线信道指令           //连续发送10次
 * 3、发送切换测试模式指令           //连续发送20次
 * 3、发送测试帧
 */
param_t cmd_dltest_param_tab[]=
{   {PARAM_ARG | PARAM_TOGGLE  | PARAM_OPTIONAL, "", "PlcBand|RfBand|TestMode|CryptTest"},
    {PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB,      "",	"0|1|2|3",	"PlcBand" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB, "option", "1|2|3", "RfBand" },
    {PARAM_OPT | PARAM_INTEGER | PARAM_SUB | PARAM_OPTIONAL, "channel","",      "RfBand" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL,      "mode",	"PHYTPT|PHYCB|MACTPT|WPHYTPT|WPHYCB|RF_PLCPHYCB|PHYTOWPHYCB",	"TestMode" },
    {PARAM_ARG | PARAM_TOGGLE  | PARAM_SUB ,      "",	"set|test",	"CryptTest" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL,      "mode",	"sha256|sm3|eccs|eccv|sm2s|sm2v|aes-cbc-e|aes-cbc-d|aes-gcm-e|aes-gcm-d|sm4-cbc-e|sm4-cbc-d",	"set" },
    {PARAM_OPT | PARAM_TOGGLE  | PARAM_SUB | PARAM_OPTIONAL,      "mode",	"sha256|sm3|eccs|eccv|sm2s|sm2v|aes-cbc-e|aes-cbc-d|aes-gcm-e|aes-gcm-d|sm4-cbc-e|sm4-cbc-d",	"test" },
    PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_dltest_desc, "", cmd_dltest_param_tab);


static void CreatMacHeader(MAC_PUBLIC_HEADER_t *pMacHeader, U16 MsduLen, U8 *DestMacAddr , U16 DestTEI, U8 SendType,
                        U8 MaxResendTime, U8 ProxyRouteFlag, U8 Radius, U8 RemainRadius, U8 bradDirc,U8 MacAddrUse, U8 MsduType)

{
    U8			*pData;
    U32  		Crc32Datatest = 0;

    pMacHeader->MacProtocolVersion = MAC_PROTOCOL_VER;
    pMacHeader->ScrTEI = GetTEI();
    pMacHeader->DestTEI =  DestTEI;
    pMacHeader->SendType = SendType;
    pMacHeader->MaxResendTime = MaxResendTime;
    pMacHeader->MsduSeq = ++g_MsduSequenceNumber;
    pMacHeader->MsduLength_l = MsduLen;
    pMacHeader->MsduLength_h = MsduLen >> 8;
    pMacHeader->RestTims = nnib.Resettimes;
    pMacHeader->ProxyRouteFlag = ProxyRouteFlag;
    pMacHeader->Radius = Radius;
    pMacHeader->RemainRadius = RemainRadius;
    pMacHeader->BroadcastDirection = bradDirc;  //e_CCO_STA;
    pMacHeader->RouteRepairFlag = e_NO_REPAIR;
    pMacHeader->MacAddrUseFlag = MacAddrUse;
    pMacHeader->FormationSeq = nnib.FormationSeqNumber;
    pMacHeader->MsduType = MsduType;
    pMacHeader->Reserve = 0;
    pMacHeader->Reserve1 = 0;
    pMacHeader->Reserve3 = 0;
    pMacHeader->Reserve4 = 0;

    pData = pMacHeader->Msdu;

    if(MacAddrUse)
    {
        if((MsduLen + sizeof(MAC_LONGDATA_HEADER)) >=512  || MsduLen==0 )//异常情况
        {
            //sys_panic("<CreatMMsgMacHeader MsduLen err:%d  \n> %s: %d\n",MsduLen, __func__, __LINE__);
            debug_printf(&dty,DEBUG_GE,"CreatMMsgMacHeader MsduLen err:%d,Msdutype=%02x %02x",MsduLen,pData[28],pData[29]);
        }
        __memcpy(pData, nnib.MacAddr, MACADRDR_LENGTH);
        pData += MACADRDR_LENGTH;
        __memcpy(pData, DestMacAddr, MACADRDR_LENGTH);
        pData += MACADRDR_LENGTH;

    }
    else
    {
        if(MsduLen + sizeof(MAC_PUBLIC_HEADER_t) >=512 || MsduLen==0 )
        {
            //sys_panic("<CreatMMsgMacHeader MsduLen err:%d \n> %s: %d\n",MsduLen, __func__, __LINE__);
            debug_printf(&dty,DEBUG_GE,"CreatMMsgMacHeader1 MsduLen err:%d,Msdutype=%02x %02x",MsduLen,pData[16],pData[17]);
        }
    }

    crc_digest(pData, MsduLen, (CRC_32 | CRC_SW), &Crc32Datatest);
    __memcpy(&pData[MsduLen], (U8 *)&Crc32Datatest, CRCLENGTH);

    return;
}

//mbuf_hdr_t *packedCommeTest(U8 TestMode, void *pData, U16 len)
//{
//    //发送载波测试帧，设置待测设备band
//    U16 msduLen = sizeof(APDU_HEADER_t)+sizeof(COMMU_TEST_HEADER_t);
//    U16 macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;
//    MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)zc_malloc_mem(sizeof(MAC_PUBLIC_HEADER_t)+ msduLen + CRCLENGTH, "TsPBd", MEM_TIME_OUT);
//    APDU_HEADER_t *pApsHead = (APDU_HEADER_t *)pMacHeader->Msdu;
//    COMMU_TEST_HEADER_t *head = (COMMU_TEST_HEADER_t *)pApsHead->Apdu;
//    //应用报文头
//    pApsHead->PacketPort      = e_READMETER_PACKET_PORT;
//    pApsHead->PacketID        = e_COMMU_TEST;
//    pApsHead->PacketCtrlWord  = APS_PACKET_CTRLWORD;

//    head->TestModeCfg = TestMode;
//    head->HeaderLen = sizeof(COMMU_TEST_HEADER_t);
//    head->DataLength = g_BandIndex;
//}

U8 g_TestModeCfg;
U8 g_Sendcnt;
U8 g_BandIndex;
__isr__ void phy_test_tx_evt(phy_evt_param_t *para, void *arg)
{
    tty_t *term = g_vsh.term;
    U8 bandidx  = 0;
//    phy_band_info_t band;
    uint32_t  time, curr, dur;
    int32_t ret;
    evt_notifier_t ntf;
    frame_ctrl_t *head;

    g_Sendcnt--;

    dur = PREAMBLE_DUR + FRAME_CTRL_DUR(para->fi->hi.symn) + pld_sym_to_time(para->fi->pi.symn);
    time = get_phy_tick_time() + dur+ PHY_BIFS + PHY_US2TICK(200) + PHY_MS2TICK(200);//para->stime + dur + PHY_BIFS;
//    time = para->stime + dur + 0 + PHY_BIFS;

    head = (frame_ctrl_t *)para->fi->head;

    if(g_TestModeCfg == e_CARRIER_BAND_CHANGE)
    {
        if(g_Sendcnt == 0)
        {
            phy_band_config(g_BandIndex);
        }
        else if((80 - g_Sendcnt) == 1)
        {
            bandidx = 0;
            phy_band_config(bandidx);
        }else
        {
            bandidx = (80 - g_Sendcnt)/20;
            phy_band_config(bandidx);
        }

        time = get_phy_tick_time() + dur+ PHY_BIFS + PHY_US2TICK(200) + PHY_MS2TICK(200);

    }

    if(g_Sendcnt == 0)
    {
        term->op->tprintf(term, "Sednok %d\n", g_BandIndex);
        if(arg)
        {
            zc_free_mem(arg);
        }
        return;
    }



    fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_test_tx_evt, arg);
    if ((ret = phy_start_tx(para->fi, &time, &ntf, 1)) != OK) {
        term->op->tprintf(term, "start tx frame failed, ret = %d\n", ret);
        if(arg)
        {
            zc_free_mem(arg);
            if(g_TestModeCfg == e_CARRIER_BAND_CHANGE)
            {
                phy_band_config(g_BandIndex);
            }
        }
        return;
    }

    curr = get_phy_tick_time();


    term->op->tprintf(term, "------------------------------------------------------------------------\n");
    term->op->tprintf(term, "idx         frame     hs   symn pbc tmi fl       len   next     curr\n");
    term->op->tprintf(term, "------------------------------------------------------------------------\n");

    term->op->tprintf(term, "%-8d %d tx %-9s %-4d %-4d %-3d %-3d %-8d %-5d %08x %08x\n",
             g_Sendcnt,bandidx, frame2str(head->dt), para->fi->hi.symn, get_frame_symn(head), get_frame_pbc(head),
             get_frame_tmi(head), get_frame_dur(para->fi), para->fi->length,
             g_Sendcnt > 0 ? time : 0, curr);


}


__isr__ void wphy_test_tx_evt(phy_evt_param_t *para, void *arg)
{
    tty_t *term = g_vsh.term;
    int32_t ret;
    evt_notifier_t ntf;
    rf_frame_ctrl_t *head;

    head = (rf_frame_ctrl_t *)para->fi->head;

    g_Sendcnt--;

    if(g_Sendcnt == 0)
    {
        term->op->tprintf(term, "wphy Sednok %d\n", g_BandIndex);
        if(arg)
        {
            zc_free_mem(arg);
        }
        return;
    }

    fill_evt_notifier(&ntf, WPHY_EVT_EOTX, wphy_test_tx_evt, arg);
    if ((ret = wphy_start_tx(para->fi, /*&time*/NULL, &ntf, 1)) != OK) {
        term->op->tprintf(term, "start tx frame failed, ret = %d\n", ret);
        if(arg)
        {
            zc_free_mem(arg);
        }
        return;
    }


    if ((g_Sendcnt & 0xf) == 1) {
        term->op->tprintf(term, "-------------------------------------------------------------------------------\n");
        term->op->tprintf(term, "idx         frame     gain nid hmcs pmcs pbsz len      stime    curr\n");
        term->op->tprintf(term, "-------------------------------------------------------------------------------\n");
    }

    term->op->tprintf(term, "%-8d tx %-9s %-4d %-3d %-3d  %-3d   %-3d %08d %08x %08x\n",
             g_Sendcnt, frame2str(head->dt), 0, head->snid, para->fi->whi.phr_mcs, para->fi->wpi.pld_mcs,
                      wphy_get_pbsz(para->fi->wpi.blkz), para->fi->length, para->stime, get_phy_tick_time());


}
void docmd_dltest(command_t *cmd, xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if(__strcmp(xsh->argv[1], "PlcBand") == 0)
    {
        g_BandIndex=__atoi(xsh->argv[2]) ;
        zc_phy_reset();

        g_Sendcnt = 80;
        evt_notifier_t ntf;

#if defined(STATIC_NODE)
        ScanBandManage(e_TEST,0);
#else
        HPLC.scanFlag = FALSE;
#endif
        HPLC.testmode = TEST_MODE;

        //发送载波测试帧，设置待测设备band
        U16 msduLen = sizeof(APDU_HEADER_t)+sizeof(COMMU_TEST_HEADER_t);
        U16 macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;
        MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)zc_malloc_mem(sizeof(MAC_PUBLIC_HEADER_t)+ msduLen + CRCLENGTH, "TsPBd", MEM_TIME_OUT);
        APDU_HEADER_t *pApsHead = (APDU_HEADER_t *)pMacHeader->Msdu;
        COMMU_TEST_HEADER_t *head = (COMMU_TEST_HEADER_t *)pApsHead->Apdu;
        //应用报文头
        pApsHead->PacketPort      = e_READMETER_PACKET_PORT;
        pApsHead->PacketID        = e_COMMU_TEST;
        pApsHead->PacketCtrlWord  = APS_PACKET_CTRLWORD;

        head->TestModeCfg = g_TestModeCfg = e_CARRIER_BAND_CHANGE;
        head->HeaderLen = sizeof(COMMU_TEST_HEADER_t);
        head->DataLength = g_BandIndex;
        //组建mac报文头
        CreatMacHeader(pMacHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                       e_LOCAL_BROADCAST_FREAM_NOACK, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 1, 1, e_TWO_WAY,FALSE, e_APS_FREAME);

        mbuf_hdr_t *p;
        p = packet_sof(1, BROADCAST_TEI, (U8 *)pMacHeader, macLen, pMacHeader->MaxResendTime, 0,e_UNKNOWN_PHASE,NULL);
        term->op->tprintf(term, "g_BandIndex=%d\n", g_BandIndex);
        if(!p)
        {
            term->op->tprintf(term, "packet_sof is NULL\n");
            zc_free_mem(pMacHeader);
            return ;
        }
        fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_test_tx_evt, p);
        if(phy_start_tx(&p->buf->fi, NULL ,&ntf, 1) != OK)
        {
            zc_free_mem(p);
            zc_phy_reset();
        }


        zc_free_mem(pMacHeader);

        //设置当前设备载波频段
//        phy_set_band_info(bandidx, &band);
    }
    else if(__strcmp(xsh->argv[1], "RfBand") == 0)
    {
        uint8_t option;
        uint32_t channel = 0;

        g_Sendcnt = 10;
        zc_phy_reset();

        option = __strtoul(xsh_getopt_val(xsh->argv[2], "-option="), NULL, 10);
        term->op->tprintf(term, "set option:  %d\n", option);

        if (xsh->argc == 4) {
            channel  = __strtoul(xsh_getopt_val(xsh->argv[3], "-channel="), NULL, 10);
            term->op->tprintf(term, "set channel: %d\n", channel);
        }


        //通过载波设置待测设备无线通信信道。
        U16 msduLen = sizeof(APDU_HEADER_t)+sizeof(COMMU_TEST_HEADER_t);
        U16 macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;
        MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)zc_malloc_mem(sizeof(MAC_PUBLIC_HEADER_t)+ msduLen + CRCLENGTH, "TsPBd", MEM_TIME_OUT);
        APDU_HEADER_t *pApsHead = (APDU_HEADER_t *)pMacHeader->Msdu;
        COMMU_TEST_HEADER_t *head = (COMMU_TEST_HEADER_t *)pApsHead->Apdu;
        //应用报文头
        pApsHead->PacketPort      = e_READMETER_PACKET_PORT;
        pApsHead->PacketID        = e_COMMU_TEST;
        pApsHead->PacketCtrlWord  = APS_PACKET_CTRLWORD;

        head->TestModeCfg = g_TestModeCfg = e_RFCHANNEL_CFG;
        head->HeaderLen = sizeof(COMMU_TEST_HEADER_t);
        head->DataLength = ((channel & 0xff) << 4) | (option & 0xf);

        //组建mac报文头
        CreatMacHeader(pMacHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                       e_LOCAL_BROADCAST_FREAM_NOACK, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 1, 1, e_TWO_WAY,FALSE, e_APS_FREAME);

        mbuf_hdr_t *p;
        evt_notifier_t ntf;
        p = packet_sof(1, BROADCAST_TEI, (U8 *)pMacHeader, macLen, pMacHeader->MaxResendTime, 0,e_UNKNOWN_PHASE,NULL);

        if(!p)
        {
            term->op->tprintf(term, "packet_sof is NULL\n");
            zc_free_mem(pMacHeader);
            return ;
        }
        fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_test_tx_evt, p);
        if(phy_start_tx(&p->buf->fi, NULL ,&ntf, 1) != OK)
        {
            zc_free_mem(p);
            zc_phy_reset();
        }

        //设置无线信道
         setHplcOptin(option);
         setHplcRfChannel(channel);

//         zc_free_mem(p);
         zc_free_mem(pMacHeader);
    }
    else if(__strcmp(xsh->argv[1], "TestMode") == 0)
    {
        g_Sendcnt = 10;
        uint8_t mode = e_RESERVE;
        char 	*s;
        if( (s = xsh_getopt_val(xsh->argv[2], "-mode=") ) )
        {
            if(__strcmp(s, "PHYTPT") == 0)
            {
                mode = e_PHY_TRANSPARENT_MODE;
            }
            else if(__strcmp(s, "PHYCB") == 0)
            {
                mode = e_PHY_RETURN_MODE;
            }
            else if(__strcmp(s, "MACTPT") == 0)
            {
                mode = e_MAC_TRANSPARENT_MODE;
            }
            else if(__strcmp(s, "WPHYTPT") == 0)
            {
                mode = e_WPHY_TRANSPARENT_MODE;
            }
            else if(__strcmp(s, "WPHYCB") == 0)
            {
                mode = e_WPHY_RETURN_MODE;
            }
            else if(__strcmp(s, "RF_PLCPHYCB") == 0)
            {
                mode = e_RF_HPLC_RETURN_MODE;
            }
            else if(__strcmp(s, "PHYTOWPHYCB") == 0)
            {
                mode = e_HPLC_TO_RF_RETURN_MODE;
            }
        }
        else
        {
            term->op->tprintf(term, "input format error !\n");
            return ;
        }

        if(mode == e_RESERVE)
        {
            term->op->tprintf(term, "mode<%d> err\n", mode);
            return;
        }

        //通过载波设置待测模块测试模式
        U16 msduLen = sizeof(APDU_HEADER_t)+sizeof(COMMU_TEST_HEADER_t);
        U16 macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;
        MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)zc_malloc_mem(sizeof(MAC_PUBLIC_HEADER_t)+ msduLen + CRCLENGTH, "TsPBd", MEM_TIME_OUT);
        APDU_HEADER_t *pApsHead = (APDU_HEADER_t *)pMacHeader->Msdu;
        COMMU_TEST_HEADER_t *head = (COMMU_TEST_HEADER_t *)pApsHead->Apdu;
        //应用报文头
        pApsHead->PacketPort      = e_READMETER_PACKET_PORT;
        pApsHead->PacketID        = e_COMMU_TEST;
        pApsHead->PacketCtrlWord  = APS_PACKET_CTRLWORD;

        head->TestModeCfg = g_TestModeCfg = mode;
        head->HeaderLen = sizeof(COMMU_TEST_HEADER_t);
        head->DataLength = 10;

        if(mode == e_HPLC_TO_RF_RETURN_MODE)
        {
             head->phr_mcs   = 3;
             head->psdu_mcs  = 4;
             head->pbsize    = PB264_BODY_BLKZ;
        }

        //组建mac报文头
        CreatMacHeader(pMacHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                       e_LOCAL_BROADCAST_FREAM_NOACK, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 1, 1, e_TWO_WAY,FALSE, e_APS_FREAME);

        mbuf_hdr_t *p;
        evt_notifier_t ntf;
        p = packet_sof(1, BROADCAST_TEI, (U8 *)pMacHeader, macLen, pMacHeader->MaxResendTime, 0,e_UNKNOWN_PHASE,NULL);

        if(!p)
        {
            term->op->tprintf(term, "packet_sof is NULL\n");
            zc_free_mem(pMacHeader);
            return ;
        }
        fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_test_tx_evt, p);
        if(phy_start_tx(&p->buf->fi, NULL ,&ntf, 1) != OK)
        {
            zc_free_mem(p);
            zc_phy_reset();
        }


//        zc_free_mem(p);
        zc_free_mem(pMacHeader);

    }
    else if(__strcmp(xsh->argv[1], "CryptTest") ==0 )
    {

        g_Sendcnt = 1;
        U8 sendLink = e_HPLC_Link;
        char 	*s;
        U8 CryptMode = 0;
        if(__strcmp(xsh->argv[2], "set") ==0  || __strcmp(xsh->argv[2], "test") ==0 )
        {
            if( (s = xsh_getopt_val(xsh->argv[3], "-mode=") ) )
            {
                term->op->tprintf(term, "Crypt set Mode: %s\n", s);
                if(__strcmp(s, "sha256") == 0)
                {
                    CryptMode = e_SHA256_ALGTEST;
                }
                else if(__strcmp(s, "sm3") == 0)
                {
                    CryptMode = e_SM3_ALGTEST;
                }
                else if(__strcmp(s, "eccs") == 0)
                {
                    CryptMode = e_ECC_SIGNTEST;
                }
                else if(__strcmp(s, "eccv") == 0)
                {
                    CryptMode = e_ECC_VERIFYTEST;
                }
                else if(__strcmp(s, "sm2s") == 0)
                {
                    CryptMode = e_SM2_SIGNTEST;
                }
                else if(__strcmp(s, "sm2v") == 0)
                {
                    CryptMode = e_SM2_VERIFYTEST;
                }
                else if(__strcmp(s, "aes-cbc-e") == 0)
                {
                    CryptMode = e_AES_CBC_ENCTEST;
                }
                else if(__strcmp(s, "aes-cbc-d") == 0)
                {
                    CryptMode = e_AES_CBC_DECTEST;
                }
                else if(__strcmp(s, "aes-gcm-e") == 0)
                {
                    CryptMode = e_AES_GCM_ENCTEST;
                }
                else if(__strcmp(s, "aes-gcm-d") == 0)
                {
                    CryptMode = e_AES_GCM_DECTEST;
                }
                else if(__strcmp(s, "sm4-cbc-e") == 0)
                {
                    CryptMode = e_SM4_CBC_ENCTEST;
                }
                else if(__strcmp(s, "sm4-cbc-d") == 0)
                {
                    CryptMode = e_SM4_CBC_DECTEST;
                }
                else
                {
                    term->op->tprintf(term, "Mode err!\n");
                    return ;
                }
            }
            else
            {
                return;
            }

            U16 msduLen;
            U16 macLen;
            MAC_PUBLIC_HEADER_t *pMacHeader = (MAC_PUBLIC_HEADER_t *)zc_malloc_mem(sizeof(MAC_PUBLIC_HEADER_t)+ 500 + CRCLENGTH, "TsPBd", MEM_TIME_OUT);
            APDU_HEADER_t *pApsHead = (APDU_HEADER_t *)pMacHeader->Msdu;
            //应用报文头
            pApsHead->PacketPort      = e_READMETER_PACKET_PORT;
            pApsHead->PacketID        = e_COMMU_TEST;
            pApsHead->PacketCtrlWord  = APS_PACKET_CTRLWORD;

            msduLen = sizeof(APDU_HEADER_t);

            if(__strcmp(xsh->argv[2], "set") ==0)               //设置安全测试模式
            {
                COMMU_TEST_HEADER_t *head = (COMMU_TEST_HEADER_t *)pApsHead->Apdu;

                head->TestModeCfg = g_TestModeCfg = e_SAFETEST_MODE;
                head->HeaderLen = sizeof(COMMU_TEST_HEADER_t);
                head->DataLength = 10;
                head->SafeTestMode = CryptMode;

                msduLen += sizeof(COMMU_TEST_HEADER_t);
                sendLink = e_HPLC_Link;
            }
            else if(__strcmp(xsh->argv[2], "test") ==0)         //发送安全测试用例
            {
                SAFE_TEST_HEADER_t *head =  (SAFE_TEST_HEADER_t *)pApsHead->Apdu;
                packCryptData(CryptMode, head);

                msduLen += (sizeof(SAFE_TEST_HEADER_t) + head->DataLength);
                sendLink = e_HPLC_Link;
            }

            macLen = sizeof(MAC_PUBLIC_HEADER_t) + msduLen + CRCLENGTH;
            //组建mac报文头
            CreatMacHeader(pMacHeader, msduLen, BroadcastAddress, BROADCAST_TEI,
                           e_LOCAL_BROADCAST_FREAM_NOACK, NWK_UNICAST_MAX_RETRIES, e_USE_PROXYROUTE, 1, 1, e_TWO_WAY,FALSE, e_APS_FREAME);

            mbuf_hdr_t *p;
            evt_notifier_t ntf;
            if(sendLink == e_HPLC_Link){
                p = packet_sof(1, BROADCAST_TEI, (U8 *)pMacHeader, macLen, pMacHeader->MaxResendTime, 0,e_UNKNOWN_PHASE,NULL);
                if(!p)
                {
                    term->op->tprintf(term, "packet_sof is NULL\n");
                    zc_free_mem(pMacHeader);
                    return ;
                }
                fill_evt_notifier(&ntf, PHY_EVT_TX_END, phy_test_tx_evt, p);
                if(phy_start_tx(&p->buf->fi, NULL ,&ntf, 1) != OK)
                {
                    zc_free_mem(p);
                    zc_phy_reset();
                }
            }
            else
            {
                p = packet_sof_wl(1, BROADCAST_TEI, (U8 *)pMacHeader, macLen, pMacHeader->MaxResendTime, 0,NULL);
                if(!p)
                {
                    term->op->tprintf(term, "packet_sof is NULL\n");
                    zc_free_mem(pMacHeader);
                    return ;
                }
                fill_evt_notifier(&ntf, WPHY_EVT_EOTX, wphy_test_tx_evt, p);
                if(wphy_start_tx(&p->buf->fi, NULL ,&ntf, 1) != OK)
                {
                    zc_free_mem(p);
                    zc_phy_reset();
                }
            }

            zc_free_mem(pMacHeader);

        }

        //发送安全加密测试报文
    }
}
