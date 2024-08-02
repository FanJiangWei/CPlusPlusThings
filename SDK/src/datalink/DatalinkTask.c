
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
#include "ScanRfCHmanage.h"
#include "app_dltpro.h"
#include "meter.h"
#include "net_cmd_info.h"

#if 1

///////////////////////////////////数据链路层任务建立/////////////////////////////
#endif


event_t task_work_mutex;


//DatalinkTask Config
#define TASK_DATALINK_STK_SZ  1024
#define TASK_PRIO_DATALINK 59
U32 DatalinkTaskWork[TASK_DATALINK_STK_SZ];
S32 PidDatalinkTask;







//创建数据链路层数据队列
msgq_t Msgqdatalink_t;
#define NET_MSG_ENTRY_NR 30
static void MsgqDatalinkCreate()
{
    if(msgq_init(&Msgqdatalink_t, "Msgqdatalink_t" , offset_of(work_t, link), NET_MSG_ENTRY_NR))
    {
    	net_printf("MsgqDatalinkCreate fail!!\n");
    }

}

#if defined(STATIC_MASTER)

//关联汇总使用
#define FirstBcnTim 200 //200ms

U8 StartSendCenterBeaconFlag = FALSE;

void GatherIndCB(struct ostimer_s *ostimer, void *arg)
{

    work_t *work = zc_malloc_mem(sizeof(work_t)+1,"NetTaskGatherInd",MEM_TIME_OUT);
    work->work = NetTaskGatherInd;
    work->buf[0] = e_HPLC_Link;
	work->msgtype = GATHER_IND;
    post_datalink_task_work(work);

}

void RfGatherIndCB(struct ostimer_s *ostimer, void *arg)
{

    work_t *work = zc_malloc_mem(sizeof(work_t) + 1,"NetTaskGatherInd",MEM_TIME_OUT);
    work->work = NetTaskGatherInd;
    work->buf[0] = e_RF_Link;
    work->msgtype = GATHER_IND;
    post_datalink_task_work(work);

}
static void SendCenterBeacon(work_t *work)
{
    //重新规划时隙，释放现有时隙 <<<
    net_printf("SendBeaconCB...\n");
    // hplc_drain_beacon(TRUE);
    Clean_All_time_slot(TRUE);
//    HPLC.next_beacon_start = 0;
    HPLC.next_beacon_start = get_phy_tick_time() + PHY_US2TICK(1500);
    packet_beacon_period(NULL);
}
//static void IncreaseFromatSeq2App(work_t *work)
//{
//    IncreaseFromatSeq();
//}
///*static*/ void NetAddFormationSeqReq()
//{
//    static U8 firstflag = TRUE;

//    if(!firstflag)
//    {
//        return;
//    }

//    firstflag = FALSE;

//    work_t *work = zc_malloc_mem(sizeof(work_t), "FMSEQ", MEM_TIME_OUT);
//    work->work = IncreaseFromatSeq2App;
//    post_app_work(work);
//}

static void SendCenterBeaconTimerCB(struct ostimer_s *ostimer, void *arg)
{

    //启动信标发送时，修改组网序列号。
//    NetAddFormationSeqReq();

    if(getHplcTestMode() != NORM)
        return;
    if(StartSendCenterBeaconFlag == FALSE)
    {
        StartSendCenterBeaconFlag = TRUE;
    }
    work_t *work = zc_malloc_mem(sizeof(work_t),"SendCenternBeacon",MEM_TIME_OUT);
    work->work = SendCenterBeacon;
	work->msgtype = CENBEACON;
    post_datalink_task_work(work);

    //防止任务丢失
    timer_start(&g_SendCenterBeaconTimer);
}

void modify_sendcenterbeacon_timer(uint32_t first)
{
    if(g_SendCenterBeaconTimer.fn != NULL)
    {
        timer_modify(&g_SendCenterBeaconTimer,
                     first,//first /10,
                     first,//first /10,
                     TMR_TRIGGER,
                     SendCenterBeaconTimerCB,
                     NULL,
                     "g_SendCenterBeaconTimer",
                     TRUE);
    }
    else
	{
		sys_panic("g_SendCenterBeaconTimer is null\n");
	}
    timer_start(&g_SendCenterBeaconTimer);

}


static void StartFormationNet(void)
{
    if(g_SendCenterBeaconTimer.fn == NULL)
    {
        timer_init(&g_SendCenterBeaconTimer,
        30*1000,
        0,//BEACONPERIODLENGTH/10,
        TMR_TRIGGER,
        SendCenterBeaconTimerCB, //SendCenternBeacon,
        NULL,
        "g_SendCenterBeaconTimer");
    }

    timer_start(&g_SendCenterBeaconTimer);

    //关联汇总定时器
    {
        timer_init(&g_AssociGatherTimer,
        FirstBcnTim,
        0,
        TMR_TRIGGER,
        GatherIndCB,
        NULL,
        "g_AssociGatherTimer"
        );
    }
    //关联汇总定时器
    {
        timer_init(&g_RfAssociGatherTimer,
        FirstBcnTim,
        0,
        TMR_TRIGGER,
        RfGatherIndCB,
        NULL,
        "g_AssociGatherTimer"
        );
    }

    //重新组网定时器
    {
        Renetworkingtimer = timer_create(8*1000,
        0,
        TMR_TRIGGER ,//TMR_TRIGGER
        RenetworkingtimerCB,
        NULL,
        "RenetworkingtimerCB"
        );

    }

	if(FactoryTestFlag == FALSE)
    {   
        Trigger2app(TO_NET_INIT_OK);
    }
}

#endif

nniberr_t nniberr={0};
extern int32_t task_all_err;

U8  datalink_deal_flag = FALSE;
extern msgq_t app_wq;
static void DatalinkTaskProcess()
{
    work_t *work;
	//U8 id;
    //初始化数据链路层属性库和table
    //extern int32_t sync_clk_freq(phy_evt_param_t *para);
	os_sleep(50);
    changeband( DefSetInfo.FreqInfo.FreqBand);
    changRfChannel(DefSetInfo.RfChannelInfo.option,DefSetInfo.RfChannelInfo.channel);
    setRfChlHaveNetFlag(DefSetInfo.RfChannelInfo.option, DefSetInfo.RfChannelInfo.channel);
//    changeband(2);
#if defined(STATIC_NODE)
    change_band_timer_init();
#else
    StartFormationNet();
#endif

    ClearINIT();
    ClearNNIB();
	
    //record Init
    recordArrayInit();

    //启动列表维护
    StartDatalinkGlobalUpdata();

    //注册载波数据处理函数
    register_sync_func(CleanTimeSlotReq);
    register_plc_deal_Func(DT_BEACON, link_layer_beacon_deal);
    register_plc_deal_Func(DT_SOF,    link_layer_sof_deal);
    register_plc_deal_Func(DT_COORD,  link_layer_coord_deal);
    register_plc_deal_Func(DT_SACK,   link_layer_sack_deal);
	//register_plc_msg_func(&post_datalink_task_work);
    printSwitch(FALSE);

    APP_UPLINK_HANDLE(pSendBeaconCnt, UpdataNbSendCntForBeacon);

    #if DATALINKDEBUG_SW
	memset(recordDealtime,0,sizeof(recordDealtime));
    #endif

uint32_t state;
    register_plc_msg_func(&post_datalink_task_work);
    while(1)
    {
    	work = (work_t *)msgq_deq(&Msgqdatalink_t);
        if(NULL == work)
            continue;
        
        if(work->work != NULL)
        {
            mg_process_record_in(work,1,work->msgtype);
            state = mutex_pend(&task_work_mutex, 0);
            pDealNow = (void *)work - sizeof(mem_record_list_t);
            if(state == 1)
            {
                printf_s("penddlink nr\n",Msgqdatalink_t.nr);
                if(pDealNow != NULL)
                {
                    printf_s("da= %p dn= %s\n",pDealNow,pDealNow->s);
                }
                printf_s("2app nr\n",app_wq.nr);
                if(pAppDealNow != NULL)
                {
                    printf_s("da= %p dn= %s\n",pAppDealNow,pAppDealNow->s);
                }
            }
            work->work(work);
            mutex_post(&task_work_mutex);
            pDealNow = NULL;
            extern U8 Wdt_flag ;
            Wdt_flag = TRUE;

            mg_process_record(work,1);
            if(datalink_deal_flag)
            {
                mem_record_list_t *pTest = (void *)work - sizeof(mem_record_list_t);
                printf_s("datalink deal ok addr = %p,name = %s\n",pTest,pTest->s);
                datalink_deal_flag = FALSE;
                task_all_err = 0;
            }
        }



        zc_free_mem(work);
		
    }

}



extern int32_t pid_meter_task;



//其它层用次函数给数据链路层消息队列发送消息
void post_datalink_task_work(work_t *work)
{
    void *p;
    static uint32_t  tempcnt = 0;
    if(NULL == work)
    {
        printf_s("err nl\n");
        return;
    }
    mg_process_record_init(work);
    if(1)//(PidDatalinkTask)
    {
        p = msgq_enq(&Msgqdatalink_t, work);
    }
    else
    {
        zc_free_mem(work);
        return;
    }
    if(Msgqdatalink_t.nr > 5)
    {
        printf_s("lr%d\n",Msgqdatalink_t.nr);
        printf_s("ecn%d\n",g_meter->uart_event->count);
        if(Msgqdatalink_t.nr >= 10 && g_meter->uart_event->count != -1)
        {
            //resetmeter();
        }
        if(Msgqdatalink_t.nr == 10)
		{
		    //resetmeter();
			//gpio_disable_irq(ZCB_CTRL);
            if(0)//(Msgqdatalink_t.nr == 16)
            {
                task_dump(PidDatalinkTask,&tty);
                extern void print_stack_info(task_t *task, tty_t *term);

                print_stack_info(task_get(PidDatalinkTask), &tty);

                // extern xsh_t g_vsh;
                //printf_s("fail postinfo dlink nr%d:addr = %p,name = %s\n",Msgqdatalink_t.nr,pTest,pTest->s);
                //sys_panic("<msg full > %s: %d\n", __func__, __LINE__);
                //show_mg_record(g_vsh.term);
                extern xsh_t g_vsh;
                extern void docmd_os(command_t *cmd, xsh_t *xsh);
                g_vsh.argv[0] = "os";
                g_vsh.argv[1] = "top";
                command_t cmd;
                docmd_os(&cmd,&g_vsh);
            }
            
		}
    }
    if(p)
    {
        work_t *work = (work_t *)p;
//        if(work->work == packet_beacon_period)  //如果自动规划时隙消息丢失，重新post   可以使用定时器，延时post防止顶出其他任务。
//        {
//            modify_post_delay_timer(20, work);
//            printf_s("start delay post\n");
//        }
//        else
        {
            mem_record_list_t *pTest = (void *)work - sizeof(mem_record_list_t);
            mem_record_list_t *pfirst = (void *)msgq_first(&Msgqdatalink_t) - sizeof(mem_record_list_t);
            //resetmeter();
            if(1)
            {
                //printf_s("fdlink nr%d da= %p dn= %s:a= %p n= %s:fa= %p fn= %s\n",Msgqdatalink_t.nr,pDealNow,pDealNow->s,pfirst,pfirst->s,pTest,pTest->s);
                //printf_s("app nr%d da= %p dn= %s\n",app_wq.nr,pAppDealNow,pAppDealNow->s);
                printf_s("fdlink nr%d:a= %p n= %s:fa= %p fn= %s\n",Msgqdatalink_t.nr,pfirst,pfirst->s,pTest,pTest->s);
                task_dump(pid_meter_task,&tty);

                if(pDealNow != NULL)
                {
                    printf_s("da= %p dn= %s\n",pDealNow,pDealNow->s);
                }
                printf_s("app nr%d\n",app_wq.nr);
                if(pAppDealNow != NULL)
                {
                    printf_s("da= %p dn= %s\n",pAppDealNow,pAppDealNow->s);
                }
                tempcnt ++;
                if(tempcnt == 1|| !(tempcnt % 20))
                {
                    
                    printf_s("ecn%d\n",g_meter->uart_event->count);
                    
                    extern void print_stack_info(task_t *task, tty_t *term);
                    
                    print_stack_info(task_get(PidDatalinkTask), &tty);
                    
                    extern xsh_t g_vsh;
                    //printf_s("fail postinfo dlink nr%d:addr = %p,name = %s\n",Msgqdatalink_t.nr,pTest,pTest->s);
                    //sys_panic("<msg full > %s: %d\n", __func__, __LINE__);
                    //show_mg_record(g_vsh.term);
                    extern xsh_t g_vsh;
                    extern void docmd_os(command_t *cmd, xsh_t *xsh);
                    g_vsh.argv[0] = "os";
                    g_vsh.argv[1] = "top";
                    command_t cmd;
                    docmd_os(&cmd,&g_vsh);
                }
            }
            datalink_deal_flag = TRUE;
            zc_free_mem(p);
        }

    }
    else
    {
        tempcnt = 0;
    }
	return;
}


void DatalinkTaskInit()
{

    MsgqDatalinkCreate();


//    mutex_init(&task_work_mutex, "net mutex");

    if ((PidDatalinkTask = task_create(DatalinkTaskProcess,
                                  NULL,
                                  &DatalinkTaskWork[TASK_DATALINK_STK_SZ - 1],
                                  TASK_PRIO_DATALINK,
                                  "DatalinkTaskProcess",
                                  &DatalinkTaskWork[0],
                                  TASK_DATALINK_STK_SZ)) < 0)
        sys_panic("<NetTaskInit panic> %s: %d\n", __func__, __LINE__);

    
}




