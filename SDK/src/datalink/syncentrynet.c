
#include "SyncEntrynet.h"
#include "framectrl.h"
#include "phy_plc_isr.h"
#include "DataLinkGlobal.h"
#include "printf_zc.h"
#include "dl_mgm_msg.h"
#include "DatalinkTask.h"
#include "Scanbandmange.h"
#include "datalinkdebug.h"

#include "ScanRfCHmanage.h"


#if defined(STATIC_NODE)

#define ESTIMATE_TIME								(5*1000)   //����ʱ��
#define WAITCFM_TIME								4000//500//5   //�ȴ������ظ���ʱʱ��
#define ENTER_MAXTIME                               (30*1000)//����������������ʱ϶ʧ�����ʱʱ��
#define MAX_ENTER_MAXTIME                           (75*1000)//����������������ʱ϶ʧ�����ʱʱ��





ostimer_t *network_timer=NULL;

extern U8 CheckBlackNegibor(U32 SNID);







void StartSTAnetwork(U32 delaytime,U8 formationseq)
{
    net_printf("StartSTAnetwork : workState = %d,delaytime=%d\n"
               , GetWorkState(),delaytime);
    if(GetWorkState() == e_BEACONSYNC )
	{
		nnib.FormationSeqNumber = formationseq;
		SetWorkState(e_ENTER);
        modify_network_timer(delaytime);
	}

}
/**
 * @brief           �յ��ű������Ϊͬ��״̬
 * 
 * @param snid      ���յ��ű�������
 */
void StartSTABcnSync(U32 snid)
{
    // net_printf("StartSTABcnSync : workState = %d,snid:%08x\n"
    //            , GetWorkState(),snid);
    if(GetWorkState() == e_WAITBEACON && snid == GetSNID())
	{
		SetWorkState(e_BEACONSYNC);
        modify_network_timer(MAX_ENTER_MAXTIME);
	}

}







void StartsyncTEI(U32 SNID)
{
    // net_printf("StartsyncTEI : workState = %d,testmode:%d\n"
    //            , GetWorkState(),getHplcTestMode());
    if(GetWorkState() == e_LISEN && getHplcTestMode() == NORM)
	{
		if(CheckBlackNegibor(SNID)==FALSE) //�����������粻����ͬ��
		{
            net_printf("net in black\n"); //
			return;
		}
        if(zc_timer_query(network_timer) != TMR_RUNNING) //����ʱ�Ƿ�����
		{
           net_printf("Trigger SNID=0x%08x\n",SNID);
		   modify_network_timer(0);
		}
		
	}
}


void StopThisEntryNet()
{
    if(zc_timer_query(network_timer) == TMR_RUNNING)
        timer_stop(network_timer, TMR_NULL);
}

static U32 SelectBestNetAssoc()
{

    S8  gain =RF_RSSI_INVAL;
	U32 snid=0xffffffff;

	list_head_t *pos,*node;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    Sta_Show_NbNet();

    list_for_each_safe(pos, node,&RfNeighborDiscoveryHeader.link)
    {
        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

        if( (!CheckBlackNegibor(DiscvryTable->SNID)) || DiscvryTable->NodeDepth >= 15)
        {
            continue;
        }

        if(!CheckNbNetHRfBaud(DiscvryTable->SNID))
        {
            continue;
        }

        if(DiscvryTable->HrfInfo.DownRssi > gain)
        {
            gain = DiscvryTable->HrfInfo.DownRssi;
            snid = DiscvryTable->SNID;

            SetSnidLkTp(e_RF_Link);

        }
        /*net_printf("SelectBestNetAssoc,NodeTEI=%02x,gain=%d,%d,%d,snid=%08x\n",NeighborDiscoveryTableEntry[i].NodeTEI,
                                                                        gain,NeighborDiscoveryTableEntry[i].GAIN,
                                                                        NeighborDiscoveryTableEntry[i].RecvCount,snid);*/
    }

    net_printf("gain:%d\n", gain);
    gain = abs(gain);
    net_printf("abs(gain):%d\n", gain);
    if(snid == 0xffffffff)
    {
        list_for_each_safe(pos, node,&NeighborDiscoveryHeader.link)
        {
            DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);

            if( (!CheckBlackNegibor(DiscvryTable->SNID)) || DiscvryTable->NodeDepth >= 15)
            {
                continue;
            }

            if(!CheckNbNetHPLCBaud(DiscvryTable->SNID))
            {
                continue;
            }

            if(gain > (DiscvryTable->HplcInfo.GAIN/DiscvryTable->HplcInfo.RecvCount))
            {
                gain = (DiscvryTable->HplcInfo.GAIN/DiscvryTable->HplcInfo.RecvCount);
                snid = DiscvryTable->SNID;
                SetSnidLkTp(e_HPLC_Link);

            }
            /*net_printf("SelectBestNetAssoc,NodeTEI=%02x,gain=%d,%d,%d,snid=%08x\n",NeighborDiscoveryTableEntry[i].NodeTEI,
                                                                            gain,NeighborDiscoveryTableEntry[i].GAIN,
                                                                            NeighborDiscoveryTableEntry[i].RecvCount,snid);*/
        }

    }


    
	net_printf("best snid =%08x\n",snid);
	return snid;
		
}


static U8 getNeiNetInfo(U32 snid, STA_NEIGHBOR_NET_t *pNetInfo)
{
    U16 i ;
    for(i = 0; i < MaxSNIDnum; i++)
    {
        if(StaNeigborNet_t[i].NbSNID == snid)
        {
            if(pNetInfo)
            {
               __memcpy(pNetInfo, &StaNeigborNet_t[i], sizeof(STA_NEIGHBOR_NET_t));
                return TRUE;
            }
            break;
        }
    }

    return FALSE;
}




//static 
void update_network_state(work_t *work)
{
	U32 bestsnid;
    net_printf("update_network_state : GetWorkState = %s \n", GetWorkState()==e_LISEN?"e_LISEN":
                                                              GetWorkState()==e_ESTIMATE?"e_ESTIMATE":
                                                              GetWorkState()==e_WAITBEACON?"e_WAITBEACONFAIL":
                                                              GetWorkState()==e_BEACONSYNC?"e_BEACONSYNC":
                                                              GetWorkState()==e_ENTER?"e_ENTER":"UK");

    STA_NEIGHBOR_NET_t staNeiNetInfo;

    switch (GetWorkState())
    {

    case e_LISEN://�����׶���������ѡ���ĸ��������ͬ��
        //����SNID
        SetWorkState(e_ESTIMATE);
        modify_network_timer(ESTIMATE_TIME);//�ȴ���ʱ
        //�����׶���������
        memset(&gBcnRgainInfo, 0, sizeof(BCN_RGAIN_ASSESS_t));
        break;
    case e_ESTIMATE: //�������,��ʼͬ��,��ʱ��Ҫѡ���ź�������ǿ������
        //����SNID
        
        SetWorkState(e_WAITBEACON);
        modify_network_timer(ENTER_MAXTIME);//�ȴ���ʱ������ʱ��ʾ�����������û���յ����������ű�

        bestsnid = SelectBestNetAssoc();
        SetPib(bestsnid,0);
        SetProxy(0xfff); //����ͬ���׶�TEI����FFF
        InitHistoryPlanRecord();

        //�ȴ�ͬ��
        HPLC.sfo_not_first = 0;

        //�ز������������׶�ǿ��ֹͣ�ز�ɨƵ��ͣ��600��������������
        modify_scanband_timer(MAX_STAY_TIME*1000);

        //��ǰƵ��û�п������磬ֱ���л�Ƶ��
        if(bestsnid == 0xffffffff)
        {
            ScanBandCallBackNow();
            RfScanCallBackNow();
            SetWorkState(e_LISEN);
            StopThisEntryNet();
            SetPib(0,0);
            return;
        }

        net_printf("SnidLkTp:%d\n", GetSnidLkTp());
        if(getNeiNetInfo(bestsnid, &staNeiNetInfo) && getHplcTestMode() == NORM && HPLC.scanFlag)  //��ȡ��ǰѡ���ھ�������Ϣ���л������ŵ�
        {
            net_printf("HRF:cur=<%d,%d>, net=<%d,%d>, HPLC:cur=%d,net=%d\n"
                       , getHplcOptin(), getHplcRfChannel()
                       , staNeiNetInfo.option, staNeiNetInfo.RfChannel
                       , GetHplcBand(), staNeiNetInfo.bandidx);
//            if(GetSnidLkTp() == e_HPLC_Link)
            {
                if((staNeiNetInfo.RfChannel != 0xfff) && (staNeiNetInfo.option != 0x0f))
                {
                    //���������ŵ���Ŀ������, �˴��л������ŵ���Ϊ���ܻ�ȡĿ�����������ھӽڵ���Ϣ��
                    changRfChannel(staNeiNetInfo.option, staNeiNetInfo.RfChannel);
                    //ѡ��Ŀ�������ֹͣ����ɨƵ���ز�ɨƵ����������������Ƿ�ֹͣ�� ������ʧ�ܺ���������ɨƵ
//                    rfScanStayCurCh();
                    StopScanRfTimer();
                }

                //�ز�Ƶ����Ҫ�л��������ӦƵ�Ρ�Ӧ�ò��������������
                if(staNeiNetInfo.bandidx != 0xff)
                {
                    changeband(staNeiNetInfo.bandidx);
                }
            }
//            else
//            { //ͨ��������·ѡ�����·�������ǰƵ�κ�����Ƶ�β�һ�¡����ڵ�ǰƵ��ͣ��
//                if(staNeiNetInfo.bandidx != GetHplcBand())
//                    ScanBandCallBackNow();
//            }
        }

        break;
    case e_WAITBEACON:
    case e_BEACONSYNC:
        //��Ӻ�����������
        AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink);
        break;
    case e_ENTER: //���������ڵ�����

        if(GetNodeState() == e_NODE_ON_LINE) //������ֹͣ������
        {
        	net_printf("stop updata_network timer\n");
            return;
        }
        if(GetScanTiemrLeftTime() > WAITCFM_TIME || HPLC.scanFlag ==FALSE)   //��Ҫ�ж��л�Ƶ�ε�ʣ��ʱ���Ƿ��㹻
        {
            if(SendAssociateReq(0,g_ReqReason))
            {
                modify_network_timer(WAITCFM_TIME); //�ȴ������ظ�
            }
            else
            {
//                GetPLanAinfo()==0 ?
//                AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink)://�Ӻ���������һ���������������û�к��ʵ��ھ�
//                modify_network_timer(WAITCFM_TIME); //�ȴ������ظ�

                AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink);
            }
        }
        else
        {
            //����Ǿ�����һ���л�band��ʱ�䲻��һ�����������Ƿ���Ҫ��������뵽�������У�
            net_printf("%s: ChangBand left time is Not enough\n", __func__);
            AddBlackNeigbor(GetSNID(),BADLINKTIME,e_BadLink);
            // SetWorkState(e_LISEN);
            // ScanBandCallBackNow();
            // RfScanCallBackNow();
        }
        break;
    }
	
}

void network_timerCB( struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"NTWK",MEM_TIME_OUT);
	work->work = update_network_state;
	work->msgtype=NETKS;
	post_datalink_task_work(work);	
}

void modify_network_timer(uint32_t ms)
{
    if(network_timer == NULL)
	{
        network_timer = timer_create(
                   100,
                   100,
                   TMR_TRIGGER,//TMR_TRIGGER
                   network_timerCB,
                   NULL,
                   "network_timerCB"
                   );
	}
    if(ms == 0) //0ֱ�ӽ���ص�
	{
       if(zc_timer_query(network_timer) ==TMR_STOPPED)
       {
           timer_start(network_timer);
       }
       timer_stop(network_timer, TMR_CALLBACK);
	   
	}
	else
	{
        timer_modify(network_timer,
                ms,
                ms,
				TMR_TRIGGER ,//TMR_TRIGGER
				network_timerCB,
				NULL,
                "network_timerCB",
                TRUE);
        timer_start(network_timer);
	}
	
}
/**
 * @brief       ׼���л���һ�����磬����ͬ��״̬
 * 
 */
void Change_next_net_option()
{
    SetPib(0,0); //׼���л���һ������			
    SetProxy(0xfff);
    SetWorkState(e_LISEN);
    StopThisEntryNet();
    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF);
    HPLC.sfo_is_done=0;

    //���ڵ�ǰ����ʱ��ֹͣ�ŵ������ʱ����
    if(timer_query(&g_ChangeChlTimer) == TMR_RUNNING)
    {
        timer_stop(&g_ChangeChlTimer, TMR_NULL);
    }
    
    //�л�����ʱ����Ҫ�ָ�ɨƵ�ŵ�
    if(HPLC.scanFlag)
    {
        set_rfscan_channel();
    }
}

#endif




