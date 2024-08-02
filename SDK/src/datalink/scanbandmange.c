/////////////////////////////////////扫频管理//////////////////////////////



#include "Scanbandmange.h"
#include "sys.h"
#include "printf_zc.h"
#include "ZCsystem.h"
#include "app.h"
#include "SyncEntrynet.h"
#include "SchTask.h"
#include "dl_mgm_msg.h"
#include "Beacon.h"

#if defined(STD_DUAL)
#include "ScanRfCHmanage.h"
#endif


ostimer_t  ScanBand_timer;
U16			scantime=MAX_SCAN_TIME;


void WaitBandTimeOutFun(struct ostimer_s *ostimer, void *arg);

void modify_scanband_timer(uint32_t first)
{
    if(ScanBand_timer.fn == NULL)
	{
        timer_init(&ScanBand_timer,
                   first,
                   10,
                   TMR_TRIGGER,//TMR_TRIGGER
                   WaitBandTimeOutFun,
                   NULL,
                   "WaitBandTimeOutFun"
                   );
        timer_start(&ScanBand_timer);
	}

	else
	{
        timer_modify(&ScanBand_timer,
				first,
				10,
				TMR_TRIGGER ,//TMR_TRIGGER
				WaitBandTimeOutFun,
				NULL,
                "WaitBandTimeOutFun",
                TRUE);
        timer_start(&ScanBand_timer);
	}
	
}




void WaitBandTimeOutFun(struct ostimer_s *ostimer, void *arg)
{
    if((getHplcTestMode() != NORM && getHplcTestMode() != RD_CTRL) || !HPLC.scanFlag)
    {
        return;
    }
    HPLC.band_idx ++;
    HPLC.band_idx = HPLC.band_idx%4;

    changeband(HPLC.band_idx);
	
#if defined(STATIC_NODE)
    //入网阶段已经强行关闭载波扫频。在到这里就是入网失败。如果无线扫频被停止，需要重新启动。
    // if(getScanRfTimerState() == TMR_STOPPED)
    // {
    //     net_printf("rf go!\n");
    //     RfScanCallBackNow();
    // }

    //当前入网流程是载波入网，切换频段需要一下操作
//    if(GetSnidLkTp() == e_HPLC_Link && GetSNID() != 0)
    // if(GetWorkState() != e_ESTIMATE)        //如果是单无线侦听阶段，不重置同步状态。
    // {
    //     SetWorkState(e_LISEN);
    //     modify_network_timer(0);
    // }
    FreeBadlinkNid(HPLC.band_idx);
#endif
    net_printf("10s change band to %d\n",HPLC.band_idx);
    phy_reset_sfo(&HSFO, PHY_SFO_DEF, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
    //		PHY_DEMO.sfo_is_done=0;
    HPLC.sfo_is_done = 0;
    scantime = MAX_SCAN_TIME;
    modify_scanband_timer(scantime*1000);

}



void showtgain()
{
	 phy_tgain_t tgain;
    phy_ioctl(PHY_CMD_GET_TGAIN, &tgain);
    printf_s("\nshow ana = %d dig = %d \n",tgain.ana,tgain.dig);
}
void SetTgain(int16_t dig, int16_t ana)
{

    if(dig < 0)
    {
        dig = 0;
    }
    else if (dig > 8)
    {
        dig = 8;
    }

    if(ana < -20)
    {
       ana = -20;
    }
    else if (ana > 10)
    {
        ana = 10;
    }
	HPLC.dig = dig;
	HPLC.ana = ana;
    //phy_ioctl(PHY_CMD_SET_TGAIN, &tgain);
    //phy_ioctl(PHY_CMD_GET_TGAIN, &tgain);
    net_printf("\n-----------++++++++ana = %d dig = %d \n",HPLC.ana,HPLC.dig);
}

void changepower(U8 band)
{
	phy_tgain_t tgain;
	tgain.dig=0;
	tgain.ana=0;

	if(0==band)	 // 频段值
	{
       tgain.dig=DefSetInfo.FreqInfo.tgaindig;
	   tgain.ana=DefSetInfo.FreqInfo.tgainana;
	}
	else if(1==band)
	{
       tgain.dig=DefSetInfo.FreqInfo.tgaindig;
	   tgain.ana=DefSetInfo.FreqInfo.tgainana-4;
	}
	else if(2==band)
	{ 
       tgain.dig=DefSetInfo.FreqInfo.tgaindig;
	   tgain.ana=DefSetInfo.FreqInfo.tgainana-6;	   
	}
#if defined(STD_2016)
	else if(3==band)
	{
        tgain.dig=DefSetInfo.FreqInfo.tgaindig;
	    tgain.ana=DefSetInfo.FreqInfo.tgainana-8;
	}
#endif
	
	SetTgain(tgain.dig, tgain.ana);
}

void changeband(U8 band)
{

		phy_tgain_t tgain;//0x00080006;
		tgain.dig=0;
		tgain.ana=0;

		HPLC.band_idx = band;
        //phy_band_config(HPLC.band_idx);
		if(0==band)	 // 频段值
		{


           tgain.dig=DefSetInfo.FreqInfo.tgaindig;
		   tgain.ana=DefSetInfo.FreqInfo.tgainana;
		   g_BeaconSlotTime = 15;

		}
		else if(1==band)
		{
           tgain.dig=DefSetInfo.FreqInfo.tgaindig;
		   tgain.ana=DefSetInfo.FreqInfo.tgainana-4;
		   g_BeaconSlotTime = 20;

		}
		else if(2==band)
		{ 
	       tgain.dig=DefSetInfo.FreqInfo.tgaindig;
		   tgain.ana=DefSetInfo.FreqInfo.tgainana-6;	   
		   g_BeaconSlotTime = 25;


		}
		else if(3==band)
		{
            tgain.dig=DefSetInfo.FreqInfo.tgaindig;
		    tgain.ana=DefSetInfo.FreqInfo.tgainana-8;
            g_BeaconSlotTime = 25;


		}
		else
		{
	
            DefSetInfo.FreqInfo.FreqBand = DEF_FREQBAND;
            DefSetInfo.FreqInfo.tgaindig= DEF_TGAIN_DIG;
            DefSetInfo.FreqInfo.tgainana= DEF_TGAIN_ANA;
            //tgain.dig=DefSetInfo.FreqInfo.tgaindig-4;
            tgain.ana=DefSetInfo.FreqInfo.tgainana - 6;
            HPLC.band_idx = DefSetInfo.FreqInfo.FreqBand;
            g_BeaconSlotTime = 20;

			//return;
		}

        SetTgain(tgain.dig, tgain.ana);

}


/*函数名称：扫频管理
//启动条件：上电，离线
//停止条件：进入测试态，和抄控器建立连接，入网。
//重置：在扫频过程中接收到数据时
*/




#if defined(STATIC_NODE)

void ScanBandManage(uint8_t event, U32 snid)
{
    switch(event)
    {
    case e_INIT: //上电
    case e_LEAVE: //离线
        scantime = FIRST_SCAN_TIME; //MAX_SCAN_TIME;
        net_printf("start scanband,CurrentUseBand=%d, time:%d S\n",GetHplcBand(), scantime);
        modify_scanband_timer(scantime*1000);
        scantime = MAX_SCAN_TIME;
        break;
    case e_RECEIVE: //接收到数据
        if(HPLC.scanFlag == FALSE )
        {
            FreeBadlinkNid(HPLC.band_idx);
            return;
        }
        if(getHplcTestMode() != NORM && getHplcTestMode() != RD_CTRL)
            return;

        if(GetNodeState() == e_NODE_ON_LINE)
        {
            stopScanBandTimer();
            return;
        }
        if(scantime == MAX_SCAN_TIME )
        {
            if(GetSNID() == 0 || GetSNID() == snid)
            {
                scantime =MAX_STAY_TIME;
                if(zc_timer_query(network_timer) != TMR_RUNNING)
                {
                    //防止未听到信标在该频段停留600s
                    net_printf("scan stay\n");
                    modify_network_timer(0);
                }
            }

            modify_scanband_timer(scantime*1000);
        }
        break;
    case e_CNTCKQ:// 和抄控器建立连接
    case e_TEST://进入测试态
    case e_ONLINEEVENT://入网
        scanStopTimer(event);
        break;
    default:
        break;
    }
}
#endif

//返回值：FALSE表示未扫频 ，1表示扫频
U8 GetScanBandStatus()
{
    return timer_query(&ScanBand_timer);
}

void ScanBandCallBackNow()
{
    if(TMR_RUNNING == timer_query(&ScanBand_timer))
        timer_stop(&ScanBand_timer, TMR_CALLBACK);
}
/**
 * @brief stopScanBandTimer 停止载波扫频
 */
void stopScanBandTimer()
{
    if(TMR_RUNNING == timer_query(&ScanBand_timer))
        timer_stop(&ScanBand_timer, TMR_NULL);
}

void changeBandReq(struct work_s *work)
{
    changeband(work->buf[0]);
}

U32 GetRunTimerLeftTime(ostimer_t *timer)
{
    if(TMR_RUNNING == zc_timer_query(timer))
    {
        return timer_remain(timer);
    }
    else
    {
        return 0;
    }
}

U32 GetScanTiemrLeftTime()
{
    return GetRunTimerLeftTime(&ScanBand_timer);
}

