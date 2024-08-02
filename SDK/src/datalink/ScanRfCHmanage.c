/**
*�����ŵ�ɨƵ����
*/

#include "ScanRfCHmanage.h"
#include "Scanbandmange.h"
#include "ZCsystem.h"
#include "SyncEntrynet.h"
#include "dl_mgm_msg.h"
#include "printf_zc.h"
#include "schtask.h"

//��������Ƶ�㼯��
RF_OPTIN_CH_t scanRfChArry[e_op_end]=
{
    
    //option 2 G1
    {2, 8},
    {2, 16},
    {2, 24},
    {2, 32},
    {2, 36},
    {2, 40},
    {2, 44},
    {2, 48},
    {2, 56},
    {2, 64},
    //option 2 G2
    {2, 4},
    {2, 20},
    {2, 28},
    {2, 38},
    {2, 42},
    {2, 46},
    {2, 50},
    {2, 52},
    {2, 54},
    {2, 69},
    //option 3  G1,
    {3, 41},
    {3, 61},
    {3, 81},
    {3, 91},
    {3, 97},
    {3, 101},
    {3, 107},
    {3, 121},
    {3, 141},
    {3, 161},
    //option 3  G2
    {3, 30},
    {3, 50},
    {3, 70},
    {3, 86},
    {3, 112},
    {3, 117},
    {3, 128},
    {3, 135},
    {3, 147},
    {3, 180},
    //option 1
//    {1, 1},
//    {1, 9},
//    {1, 16},
//    {1, 27},
//    {1, 25},
//    {1, 33},
//    {1, 40},
    {0,0}       //Ԥ��ʹ�ã���¼�ϴ������������ŵ���Ϣ��
};

#if defined(STATIC_NODE)


#define     RF_MAX_SCAN_TIME  20        //10
#define     RF_MAX_STAY_TIME  200       //60
ostimer_t  *Rf_ScanBand_timer = NULL;
U16		   rfscantime=RF_MAX_SCAN_TIME;


U16 RfChannelArryCnt = 0;
U16 g_ScanArryIndex = 0;


#define SCAN_NO_NET_CHAL_CNT  1  //ɨ����Щ������������Ƶ������¿�ʼɨ���¼��������Ƶ�㡣
U8 g_ScanNohaveNetChlCnt;        //��¼ɨ��������Ƶ����ŵ�������
U8 g_NohaveNetChlIndex;          //��¼��ǰɨ���������Ƶ����±�
U8 g_ScanHaveNetChlFlag;         //��¼��ǰɨ���������Ƶ����±�

static U8 find_next_havenet_chl()
{
    static U8 index = 0;
    U8 i;

//    net_printf("index1:%d", index);
    for(i = 0;i < e_op_end; i++)
    {
        index = index%e_op_end;

        if(scanRfChArry[index].option == 0 || 
           scanRfChArry[index].channel == 0 || 
           scanRfChArry[index].HaveNet == FALSE || 
           index == g_ScanArryIndex)
        {
            index++;
            continue;
        }
        return index++;
    }
    return e_op_end;
}
static U8 find_next_nohavenet_chl()
{
    static U8 index = 0;
    U8 i;
//    net_printf("index2:%d", index);
    for(i = 0;i < e_op_end; i++)
    {
        index = index%e_op_end;

        if( scanRfChArry[index].option == 0 || 
            scanRfChArry[index].channel == 0 || 
            scanRfChArry[index].HaveNet == TRUE || 
            index == g_ScanArryIndex)
        {
            index++;
            continue;
        }
        
        return index++;
    }
    
    return e_op_end;
}

void WaitRfScanTimeOutFun(struct ostimer_s *ostimer, void *arg);

void modify_rfscan_timer(uint32_t ms)
{
    if(Rf_ScanBand_timer == NULL)
    {
        Rf_ScanBand_timer = timer_create(
                   ms,
                   10,
                   TMR_TRIGGER,//TMR_TRIGGER
                   WaitRfScanTimeOutFun,
                   NULL,
                   "WaitRfScanTimeOutFun"
                   );
        timer_start(Rf_ScanBand_timer);
    }

    else
    {
        timer_modify(Rf_ScanBand_timer,
                ms,
                10,
                TMR_TRIGGER ,//TMR_TRIGGER
                WaitRfScanTimeOutFun,
                NULL,
                "WaitRfScanTimeOutFun",
                TRUE);
        timer_start(Rf_ScanBand_timer);
    }
}

U8 get_Next_Chl_scan()
{
    U8 tmpindex = e_op_end;

    #if 0
    
    if(g_ScanNohaveNetChlCnt >= SCAN_NO_NET_CHAL_CNT)       //��ʼɨ���������¼��Ƶ�㡣
    {
        tmpindex = find_next_havenet_chl();
//        net_printf("tmpindex1:%d", tmpindex);

        if(tmpindex == e_op_end)     //û�л�����ɨ��һ���������¼��Ƶ��󣬿�ʼɨ���������¼��Ƶ�㡣
          g_ScanNohaveNetChlCnt = 0;
    }

    if(g_ScanNohaveNetChlCnt < SCAN_NO_NET_CHAL_CNT)        //��ʼɨ���������¼������Ƶ�㡣
    {
        tmpindex = find_next_nohavenet_chl();
//        net_printf("tmpindex2:%d", tmpindex);
        g_ScanNohaveNetChlCnt++;        //��¼��ǰɨ���˼����������¼��Ƶ�㡣
    }
    #else
    
    if(g_ScanHaveNetChlFlag == TRUE)    //������һ���������¼���ŵ�
    {
        tmpindex = find_next_havenet_chl();

        if(tmpindex == e_op_end || (++g_ScanNohaveNetChlCnt >= SCAN_NO_NET_CHAL_CNT)) 
        {
            g_ScanNohaveNetChlCnt = 0;
            g_ScanHaveNetChlFlag = FALSE;
        }
    }
    else                                //������һ��û�������¼���ŵ�
    {
        tmpindex = find_next_nohavenet_chl();

        if(tmpindex == e_op_end || (++g_ScanNohaveNetChlCnt >= SCAN_NO_NET_CHAL_CNT)) 
        {
            g_ScanNohaveNetChlCnt = 0;
            g_ScanHaveNetChlFlag = TRUE;
        }
    }
    #endif

    return tmpindex;
} 

void WaitRfScanTimeOutFun(struct ostimer_s *ostimer, void *arg)
{
    if((getHplcTestMode() != NORM && getHplcTestMode() != RD_CTRL) || !HPLC.scanFlag)
    {
        return;
    }

    
    U8 tmpindex = e_op_end;
//    g_ScanArryIndex++;
//    g_ScanArryIndex = g_ScanArryIndex % RfChannelArryCnt;

    tmpindex = get_Next_Chl_scan();

    if(tmpindex == e_op_end)
    {
        net_printf("err:no chl\n");
        g_ScanArryIndex++;
        g_ScanArryIndex = g_ScanArryIndex % RfChannelArryCnt;
    }else
    {
        g_ScanArryIndex = tmpindex;
    }

    changRfChannel(scanRfChArry[g_ScanArryIndex].option, scanRfChArry[g_ScanArryIndex].channel);

//    if(GetSnidLkTp() == e_RF_Link && GetSNID() != 0 && GetNodeState() != e_NODE_ON_LINE)
//    {
//        SetWorkState(e_LISEN);
//        modify_network_timer(0);
//    }

    //�ͷŵ�ǰ�ŵ������ڵ�����
    FreeHRfBadLinkNid();
    net_printf("%ds change Rfband to <%d,%d>\n",rfscantime, HPLC.option, HPLC.rfchannel);
//    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
//    //		PHY_DEMO.sfo_is_done=0;
    HPLC.sfo_is_done = 0;
    // rfscantime = RF_MAX_SCAN_TIME;
    modify_rfscan_timer(rfscantime*1000);

}

void ScanRfInit()
{
    RfChannelArryCnt = NELEMS(scanRfChArry) - 1;
    
    g_ScanNohaveNetChlCnt = 0;
    if(HPLC.scanFlag == FALSE)
    {
        return;
    }

    //����ɨ���������¼��Ƶ��
    g_ScanArryIndex = find_next_havenet_chl();
    g_ScanHaveNetChlFlag = FALSE;

    if(g_ScanArryIndex == e_op_end)
    {
        g_ScanArryIndex = find_next_nohavenet_chl();
        g_ScanHaveNetChlFlag = TRUE;
    }

    changRfChannel(scanRfChArry[g_ScanArryIndex].option, scanRfChArry[g_ScanArryIndex].channel);
}

void ScanRfChMange(uint8_t event, U32 snid)
{
    switch (event) {
    case e_INIT:
    case e_LEAVE:
        ScanRfInit();
        rfscantime = RF_MAX_SCAN_TIME;
        net_printf("start Rf Scan option: %d, channel:%d\n"
                   , HPLC.option, HPLC.rfchannel);
        modify_rfscan_timer(rfscantime * 1000);
        break;
    case e_RECEIVE:

        if(HPLC.scanFlag == FALSE)
        {
            FreeHRfBadLinkNid();
        }

        if(getHplcTestMode() != NORM && getHplcTestMode() != RD_CTRL)
            return;

//        net_printf("snid:%08x, optin:%d, channel:%d\n", snid, getHplcOptin(), getHplcRfChannel());
        //updateNeiNetRfParam(snid, getHplcOptin(), getHplcRfChannel());

        if(GetNodeState() == e_NODE_ON_LINE)
        {
            StopScanRfTimer();
            return;
        }

//         if(rfscantime == RF_MAX_SCAN_TIME)
//         {
//             if(GetSNID() == 0 || GetSNID() == snid)//�����û��ѡ������ͬ�������ǵ�ǰ����ѡ��ͬ������������ŵ��Ÿ���ǰ�����ŵ��Ų�ͬ�����ڵ�ǰ�ŵ���ͣ��
//             {
//                 rfscantime =RF_MAX_STAY_TIME;
// //                if(zc_timer_query(network_timer) != TMR_RUNNING)
// //                {
// //                    //��ֹδ�����ű��ڸ�Ƶ��ͣ��600s
// //                    modify_network_timer(0);
// //                }
//             }
//             else
//             {
//                 ; //����ɨ�������ŵ�
//             }

//             modify_rfscan_timer(rfscantime*1000);
//         }

//            case e_CNTCKQ:// �ͳ�������������
//            case e_TEST://�������̬
//            case e_ONLINEEVENT://����
//                if(event == e_TEST)
//                {
//                    HPLC.scanFlag = FALSE;
//                }
//                if(TMR_RUNNING == timer_query(rfscantime))
//                {
//                    timer_stop(rfscantime, TMR_NULL);
//                }

//                break;
    default:
        break;
    }
}


U8 getScanRfTimerState()
{
    return zc_timer_query(Rf_ScanBand_timer);
}
/**
 * @brief StopScanRfTimer       ֹͣ����Ƶ��ɨ�趨ʱ��
 */
void StopScanRfTimer()
{
    if(Rf_ScanBand_timer)
        timer_stop(Rf_ScanBand_timer, TMR_NULL);
}


void rfScanStayCurCh()
{
    rfscantime =RF_MAX_STAY_TIME;
    modify_rfscan_timer(rfscantime*1000);
}

void RfScanCallBackNow()
{
    //��ֹ�ŵ������л�����֤ɨƵʱ�����
    if(TMR_STOPPED == zc_timer_query(Rf_ScanBand_timer))
    {
        timer_start(Rf_ScanBand_timer);
        timer_stop(Rf_ScanBand_timer, TMR_CALLBACK);
    }
}


void ChangRfChlReq(work_t *work)
{
    RF_OPTIN_CH_t *ChlInfo = (RF_OPTIN_CH_t *)work->buf;
    changRfChannel(ChlInfo->option, ChlInfo->channel);
    DefwriteFg.RfChannel = TRUE;
    DefaultSettingFlag = TRUE;
}

/**
 * @brief Set the rfscan channel object     �����ڵ㣬��������ʱ����Ҫ�ָ������ŵ���ɨƵ�ŵ�
 * 
 */
void set_rfscan_channel()
{
    changRfChannel(scanRfChArry[g_ScanArryIndex].option, scanRfChArry[g_ScanArryIndex].channel);
}


#endif //#if defined(STATIC_NODE)

#if defined(STATIC_MASTER)
/**
 * @brief updateChlOccLeftTme   ˢ���ŵ�ռ��ʣ��ʱ�䡣
 */
void updateChlHaveNetLeftTime()
{
    U8 i ;
    for(i = 0; i < e_op_end; i++)
    {
        if(scanRfChArry[i].HaveNet)
        {
            if(scanRfChArry[i].LifeTime > 0)
            {
                scanRfChArry[i].LifeTime--;
            }
            else
            {
                scanRfChArry[i].HaveNet = FALSE;
            }
        }
    }
}

#endif //#if defined(STATIC_MASTER)

/**
 * @brief setRfChlHaveNetFlag       �����ŵ�ռ��״̬
 * @param option
 * @param channel
 */
void setRfChlHaveNetFlag(U8 option, U16 channel)
{
    U8 i;
    for(i = 0; i < e_op_end; i++)
    {
        if(scanRfChArry[i].option == option && scanRfChArry[i].channel == channel)
        {
            scanRfChArry[i].HaveNet = TRUE;
            scanRfChArry[i].LifeTime = 60*60;      //�����ŵ�ռ���ϻ�ʱ��Ϊһ��Сʱ
            return;
        }
        else if (i == (e_op_end - 1))
        {
            scanRfChArry[i].option = option;
            scanRfChArry[i].channel = channel;
            scanRfChArry[i].HaveNet = TRUE;
            scanRfChArry[i].LifeTime = 60*60;      //�����ŵ�ռ���ϻ�ʱ��Ϊһ��Сʱ
        }
    }

    // net_printf("Not in Map <%d,%d>\n", option, channel);
}

/**
 * @brief changRfChannel        �޸������ŵ�����
 * @param option                �����ŵ�ʹ��option
 * @param rfchannel             �����ŵ�ʹ��Ƶ�㣨�����ŵ��ţ�
 */
void changRfChannel(U8 option, U16 rfchannel)
{
    if(checkRfChannel(option, rfchannel) == FALSE)
    {
        app_printf("changRfChannel err, op:%d,chl:%d\n", option, rfchannel);
        return;
    }

    HPLC.option    = option;
    HPLC.rfchannel = rfchannel;

    net_printf("chg rf chl: <%d, %d>\n", HPLC.option, HPLC.rfchannel);
}

