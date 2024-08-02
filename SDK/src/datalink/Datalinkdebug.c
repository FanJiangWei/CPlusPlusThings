#include "datalinkdebug.h"
#include "DatalinkTask.h"
#include "DatalinkGlobal.h"
#include "app.h"
#include "app_dltpro.h"
#include "net_cmd_info.h"

mem_record_list_t *pDealNow;
mem_record_list_t *pAppDealNow;

#if DATALINKDEBUG_SW
RECORDMETER readmeter;

U32	RecordNumber = 0;

MMSG_RECORD_t MmsgReacord[MMSGREACORDNUM];
AREA_NOTIFY_BUFF AreaNotifyBuff;

#if defined(STATIC_MASTER)
#define MaxRecord     1000

MMS_DEEPTH_t  MmsgDeepth[MaxRecord];
#endif


U8   flow_num=0;
#endif // end DATALINKDEBUG_SW
U8 mg_update_type(U8 type, U8 index)
{
#if DATALINKDEBUG_SW
    record_mg_flow[index].type = type;
#if defined(STATIC_NODE)

	if(nniberr.errflag || checkNNIB())
	{
	  return FALSE;
	}	
	
	if(nnib.MacAddrType != e_UNKONWN && nniberr.errflag==0)//nnib出错时记录消息类型和任务类型
	//if( TMR_STOPPED == zc_timer_query(sta_bind_timer) && nniberr.errflag==0)//nnib出错时记录消息类型和任务类型
	{
		nniberr.ramErr =2;	
		nniberr.errflag =1;
		nniberr.fmtSeq = nnib.FormationSeqNumber;
		extern S32 PidDatalinkTask;
		if(task_query() == PidDatalinkTask)
		{
		 nniberr.taskid=1;	
		}
		else
	    {
	     nniberr.taskid=2;
	    }
		nniberr.msgtype = type;	
		nniberr.bcn =NwkBeaconPeriodCount;
		return TRUE;
	}

	#endif
#endif
    return FALSE;

}
void mg_process_record_init(work_t *work)
{

#if DATALINKDEBUG_SW
    work->record_index = flow_num;
    flow_num = (flow_num+1)%FLOWNUM;


    uint32_t tick = get_phy_tick_time();
//    work->posttick = get_phy_tick_time();

    record_mg_flow[work->record_index].type = work->msgtype;
    record_mg_flow[work->record_index].posttick = tick;
    record_mg_flow[work->record_index].recvtick = tick;
    record_mg_flow[work->record_index].dealtick = tick;

#endif
}
void mg_process_record_in(work_t *work,U8 taskid, U8 type)
{    
#if DATALINKDEBUG_SW
#if defined(STATIC_NODE)

	if(checkNNIB())
	{
	  return ;
	}
	//if(TMR_STOPPED == zc_timer_query(sta_bind_timer) && nniberr.errflag==0)//nnib出错时记录消息类型和任务类型
	if(nnib.MacAddrType != e_UNKONWN && nniberr.errflag==0)
	 {
		nniberr.errflag =1;
		nniberr.fmtSeq = nnib.FormationSeqNumber;
		nniberr.taskid =taskid;
		nniberr.msgtype = type;  
		nniberr.bcn =NwkBeaconPeriodCount;
		nniberr.ramErr =1;	
	 }
#endif
    record_mg_flow[work->record_index].taskid = taskid;
    record_mg_flow[work->record_index].type = type;
//    record_mg_flow[work->record_index].posttick = work->posttick;
    record_mg_flow[work->record_index].recvtick = get_phy_tick_time();
//    record_mg_flow[work->record_index].dealtick = work->recvtick;
#endif
}
void mg_process_record(work_t *work,U8 taskid)
{
#if DATALINKDEBUG_SW
#if defined(STATIC_NODE)

	if(nniberr.errflag || checkNNIB())
	{
	  return ;
	}	
	//if( TMR_STOPPED == zc_timer_query(sta_bind_timer) && nniberr.errflag==0)//nnib出错时记录消息类型和任务类型
	if(nnib.MacAddrType != e_UNKONWN && nniberr.errflag==0)//nnib出错时记录消息类型和任务类型
	{
		nniberr.ramErr =3;	
		nniberr.errflag =1;
		nniberr.fmtSeq = nnib.FormationSeqNumber;
		nniberr.taskid =taskid;
		nniberr.msgtype = record_mg_flow[work->record_index].type;	
		nniberr.bcn =NwkBeaconPeriodCount;
	}
#endif

    record_mg_flow[work->record_index].taskid = taskid;
//    record_mg_flow[work->record_index].type = type;
	//record_mg_flow[flow_num].s = s;	
//    record_mg_flow[work->record_index].posttick = work->posttick;
	//	record_mg_flow[flow_num].recvtick = work->recvtick;	
    record_mg_flow[work->record_index].dealtick = get_phy_tick_time();
//	flow_num = (flow_num+1)%FLOWNUM;
#endif
}


void show_mg_record(tty_t *term)
{
#if DATALINKDEBUG_SW

  U8 temp_flow_num = flow_num;
  U8 i;
  term->op->tprintf(term,"taskid\tname\t\t\tposttick\trecvtick\tdealtick\tdealtime\t\n");
  for(i=0;i<FLOWNUM;i++)
  {
    if(!((record_mg_flow[temp_flow_num].posttick == record_mg_flow[temp_flow_num].dealtick)
        &&(record_mg_flow[temp_flow_num].posttick != record_mg_flow[temp_flow_num].recvtick)))
    {
        continue;
    }
    //temp_flow_num++;
    if(record_mg_flow[temp_flow_num].taskid !=0)
     term->op->tprintf(term,"%s\t%12s\t\t%08x\t%08x\t%08x\t%d\n"
            ,record_mg_flow[temp_flow_num].taskid ==1 ?"net":
             record_mg_flow[temp_flow_num].taskid ==2 ?"app":"uk"
            , (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==MSDU_REQ    )?"MSDU_REQ":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==UPDATE_CB   )?"UP_CB":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==CLEAN_SOLT  )?"CL_SOLT":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==MAC_CFM     )?"MAC_CFM":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==CHAN_BAND   )?"CN_BND":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETKS       )?"NETKS":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==GATHER_IND  )?"GTHR_IND":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==CENBEACON   )?"CNBCN":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==PHASE_REQ   )?"PHS_REQ":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==LEAVEL_REQ  )?"LVL_REQ":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==SEND_DIS    )?"SND_DIS":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==PROC_BEACON )?"PRC_BCN":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==PROC_SOF    )?"PRC_SOF":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==PROC_COORD  )?"PRC_CRD":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==PROC_SACK   )?"PRC_SACK":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==ERR_SOF     )?"ERR_SF":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NOME_SOF    )?"OTHR_SF":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==REAPEAT_SOF )?"RPT_SOF":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==APS_SOF     )?"APS_SOF":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 0  )?"PAscReq":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 1  )?"PAscCf":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 2  )?"PGthrInd":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 3  )?"PPyReq":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 4  )?"PPyCf":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 5  )?"PPyBtCf":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 6  )?"PLvInd":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 7  )?"PHeart":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 8  )?"PDiscvy":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 9  )?"PScsRate":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 10 )?"PNetCft":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 11 )?"PZrInd":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==NETMMG + 12 )?"PZrRpt":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==60          )?"NXTBPTS":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==61          )?"MPDUIND":

              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_1)?"STEP_1":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_2)?"STEP_2":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_3)?"STEP_3":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_4)?"STEP_4":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_5)?"STEP_5":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_6)?"STEP_6":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_7)?"STEP_7":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_8)?"STEP_8":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_9)?"STEP_9":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_10)?"STEP_10":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_11)?"STEP_11":

              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_12)?"STEP_12":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_13)?"STEP_13":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_14)?"STEP_14":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_15)?"STEP_15":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_16)?"STEP_16":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_17)?"STEP_17":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_18)?"STEP_18":
              (record_mg_flow[temp_flow_num].taskid ==1 &&record_mg_flow[temp_flow_num].type==STEP_19)?"STEP_19":





              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==PRE_PRINT )?"PRE_PRT":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==SENDTIME_REQ )?"SNDTM_RQ":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==CCO_EVTSEND )?"CCOEVTSD":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==CCO_CLBIT )?"CCOCLBT":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==EVT_PROC )?"EVT_PRC":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==STA_CLBITPROC )?"STACLBT":

              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==EX_VER_PROC )?"EX_VER":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==JUDGE_PROC )?"JDG_PROC":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==SB_TASK_START )?"SB_START":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==SB_TASK_COLLECT )?"SB_CLT":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==SB_TASK_GET )?"SB_GET":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==SB_TASK_INFORM )?"SB_INFO":

              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==MID_REQ )?"MID_RQ":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==IN_MULTI_LIST )?"ENTY_LIST":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==UP_MULTI_LIST )?"UP_LIST":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==PHASE_RUN )?"PHS_RUN":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==POWEROFF_SEND )?"PWEVT_SND":
			  (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==POWERON_SEND )?"PWEVT_SND":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==POWERON_REPORT )?"PWON_RPT":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==POWEROFF_REPORT )?"PWOFF_RPT":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==ROUTER_RM )?"RTR_RM":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==STA_REG )?"STA_REG":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==UPGRDCTL )?"UPGRDCTL":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==DALKCFM )?"DALKCFM":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==MSDUIND )?"MSDUIND":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==TRIG2APP )?"TRIG2APP":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==PHASECMF )?"PHASECMF":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==VTCOL )?"VTCOL":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==FRTCOL )?"FRTCOL":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==ZAGATH )?"ZAGATH":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==SREGCFM )?"SREGCFM":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==CJQ2UP )?"CJQ2UP":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==ENREADM )?"ENREADM":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==UPREADM )?"UPREADM":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==ADDLF )?"ADDLF":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==DLELF )?"DLELF":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==UPSCIF )?"UPSCIF":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==RPTWAIT_SEND )?"PWT_SND":
              (record_mg_flow[temp_flow_num].taskid ==2 &&record_mg_flow[temp_flow_num].type==CYCLE_DEL )?"CYCLE_DEL":"UK"
            ,record_mg_flow[temp_flow_num].posttick
            ,record_mg_flow[temp_flow_num].recvtick
            ,record_mg_flow[temp_flow_num].dealtick
            ,PHY_TICK2US(record_mg_flow[temp_flow_num].dealtick-record_mg_flow[temp_flow_num].recvtick));
    temp_flow_num = (temp_flow_num+1)%FLOWNUM;

  }
#endif

}



void recordArrayInit()
{
#if DATALINKDEBUG_SW

    memset(MmsgReacord,0,sizeof(MmsgReacord));
    memset((U8 *)&AreaNotifyBuff,0x00,sizeof(AreaNotifyBuff));
#if defined(STATIC_MASTER)
    memset(MmsgDeepth,0xff,sizeof(MmsgDeepth));
#endif
#endif



}

void reacordmeter(U16 MsduSeq,U8 starAODV ,U8 mode,U16 DstTei,U16 NextTei)
{
#if DATALINKDEBUG_SW

    static S32 readmeterIndex=0;
    if(readmeter.MsduSeq[0]==0xffff)
    {
        readmeterIndex=0;
    }
    readmeter.MsduSeq[readmeterIndex] = MsduSeq;
    readmeter.starAODV[readmeterIndex]=starAODV;
    readmeter.mode[readmeterIndex]=mode;
    readmeter.DstTei[readmeterIndex]=DstTei;
    readmeter.NextTei[readmeterIndex]=NextTei;
    readmeterIndex=(readmeterIndex+1>=MAXNUMMEM)?0:readmeterIndex+1;
#endif
}










void showdealtime(tty_t *term)
{
#if DATALINKDEBUG_SW
    U8 i;
    if(term ==NULL)
    {
        net_printf("type maxtime lasttime\n");
    }
    else
    {term->op->tprintf(term,"type dealtime\n");}
	
    for(i=0;i<MAXDEALNUM;i++)
    {
        if(recordDealtime[i].type !=0)
        {
            if(term ==NULL)
            {
                net_printf("%02x,%d,%d\n",recordDealtime[i].type,recordDealtime[i].maxdealtime,recordDealtime[i].lastdealtime);
            }
            else
            {
                term->op->tprintf(term,"%02x,%d,%d\n",recordDealtime[i].type,recordDealtime[i].maxdealtime,recordDealtime[i].lastdealtime);
            }
			
        }
		
    }

#endif
}

void showrecordmeter(tty_t *term)
{
#if DATALINKDEBUG_SW

	U16 ii=0;
	term->op->tprintf(term,"idx---msduseq--adov-routemode--destei--next\n");
	do
	{
		if(readmeter.MsduSeq[ii]==INVALID)
		{
			term->op->tprintf(term,"totol num = %d !\n",ii);
			break;
		}
		term->op->tprintf(term,"%-2d ||%04x	%d	%d	 0x%04x 0x%04x\n",
			ii,readmeter.MsduSeq[ii],readmeter.starAODV[ii],readmeter.mode[ii],readmeter.DstTei[ii],readmeter.NextTei[ii]);
			 
		ii++;
		
	}while(ii<MAXNUMMEM);
#endif
	
}
void ReacordMmsgData(U8 Cmd,U16 SrcAddr ,U16 DstAddr , U16 NextAddr ,U8 Type)
{
#if DATALINKDEBUG_SW

    U16 i,j;
    U16 Num=0;
    for(i=0;i<MMSGREACORDNUM;i++)
    {
        if(MmsgReacord[i].Cmd ==0)
        {
            MmsgReacord[i].PeriodCount = NwkBeaconPeriodCount;
            MmsgReacord[i].Cmd = 	Cmd;
            MmsgReacord[i].SrcAddr = 	SrcAddr;
            MmsgReacord[i].DstAddr = 	DstAddr;
            MmsgReacord[i].NextAddr = 	NextAddr;
            MmsgReacord[i].Type = 	Type;
            break;
        }
    }

    if(i >= MMSGREACORDNUM)
        i = MMSGREACORDNUM -1;

    for(j=0; j<NWK_MAX_ROUTING_TABLE_ENTRYS; j++)
    {
        if(NwkRoutingTable[j].NextHopTEI != 0xfff && NwkRoutingTable[j].RouteState != INACTIVE_ROUTE)
        {
            if(Num>=27)break;
            MmsgReacord[i].RoutingTable[Num].DestTEI = j+1;
            MmsgReacord[i].RoutingTable[Num].NextHopTEI = NwkRoutingTable[j].NextHopTEI;
            MmsgReacord[i].RoutingTable[Num].RouteError = NwkRoutingTable[j].RouteError;
            MmsgReacord[i].RoutingTable[Num].RouteState = NwkRoutingTable[j].RouteState;
            Num++;
        }
    }
#endif
}
void showMmsgData(tty_t *term)
{
#if DATALINKDEBUG_SW

        U8 j,i=0,routenum;
        for(i=0;i<MMSGREACORDNUM;i++)
        {
            if(MmsgReacord[i].Cmd !=0)
            {

                term->op->tprintf(term,"PeriodCount=0x%08x,Cmd=0x%02x,SrcAddr=0x%04x,NewProxy=0x%04x,NextAddr%04x,Tpye=%02x----\n"
                ,MmsgReacord[i].PeriodCount
                ,MmsgReacord[i].Cmd
                ,MmsgReacord[i].SrcAddr
                ,MmsgReacord[i].DstAddr
                ,MmsgReacord[i].NextAddr
                ,MmsgReacord[i].Type);
                term->op->tprintf(term,"DestTEI----NextHopTEI----RouteState-----RouteError\n");
                for(routenum=0;routenum<27;routenum++)
                {
                    if(MmsgReacord[i].RoutingTable[routenum].DestTEI!=0)
                    {
                        term->op->tprintf(term,"%04x	  %04x			 %x 	   %x\n",
                        MmsgReacord[i].RoutingTable[routenum].DestTEI,
                        MmsgReacord[i].RoutingTable[routenum].NextHopTEI,
                        MmsgReacord[i].RoutingTable[routenum].RouteState,
                       MmsgReacord[i].RoutingTable[routenum].RouteError);

                   }
               }
               j++;

           }

       }

#endif
}

void RecordStaStstus(U8 Active, U8 Reason, U16 BeterSNID, U8 Sameration, U8 Result)
{

}


void RecordAreaNotifyBuff(U8 Active,U8  Reason,U16	BeterSNID,U8  Sameration,U8	Result)
{
#if DATALINKDEBUG_SW

    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].NWkSeq = nnib.FormationSeqNumber;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].SNID = GetSNID();
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].PeriodCount = NwkBeaconPeriodCount;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].State = nnib.WorkState;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].Active = Active;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].Reason = Reason;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].BeterSNID = BeterSNID;
//    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].SrnCCOPoint = nnib.SrnCCOPoint;
//    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].NtbCCOPoint = nnib.NtbCCOPoint;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].Sameration = Sameration;
    AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].Result = Result;

    AreaNotifyBuff.CurrentEntry = (AreaNotifyBuff.CurrentEntry+1)%AREA_NOTIFY_COUNT;
#endif

}

void showAreaNotifyBuff(tty_t *term)
{
#if DATALINKDEBUG_SW
	  U16 offset ,i,j=0;
	  offset =AreaNotifyBuff.CurrentEntry;
	  term->op->tprintf(term,"AreaNotifyBuff: \num\tnNWkSeq\tSNID\tPdCt\tState\tActv\tReason\tBeterSNID\tSrnPoint\tNtbPoint\tSamerat\tResult\n");
	  for(i=0;i<AREA_NOTIFY_COUNT;i++)
	  {
		  if(AreaNotifyBuff.AreaNotifyReacod[offset].NWkSeq !=0)
		  {
			  term->op->tprintf(term,"%02x\t%02x\t%02x\t%02x\t%-4s\t%-7s\t%d\t%02x\t\t%d\t%d\t\t%d\t%-7s\t\n",
			  j++,
			  AreaNotifyBuff.AreaNotifyReacod[offset].NWkSeq,
			  AreaNotifyBuff.AreaNotifyReacod[offset].SNID,
			  AreaNotifyBuff.AreaNotifyReacod[offset].PeriodCount,
			  //AreaNotifyBuff.AreaNotifyReacod[offset].State  == e_EVALUATE_FINISH ?"unlk":
			  //AreaNotifyBuff.AreaNotifyReacod[offset].State  == e_BIGWINDOW_FINISH ?"unlk1":
			  AreaNotifyBuff.AreaNotifyReacod[offset].State  == 0 ?"lock":
			  "UNKNOWN",
			  AreaNotifyBuff.AreaNotifyReacod[offset].Active == e_SnrCacl ?"SnrCacl":
			  AreaNotifyBuff.AreaNotifyReacod[offset].Active == e_NtbCacl ?"NtbCacl":
			  AreaNotifyBuff.AreaNotifyReacod[offset].Active == e_Clnnib ?"Clennib":
			  "UNKNOWN",
			  AreaNotifyBuff.AreaNotifyReacod[offset].Reason,
			  AreaNotifyBuff.AreaNotifyReacod[offset].BeterSNID,
			  AreaNotifyBuff.AreaNotifyReacod[offset].SrnCCOPoint,
			  AreaNotifyBuff.AreaNotifyReacod[offset].NtbCCOPoint,
			  AreaNotifyBuff.AreaNotifyReacod[offset].Sameration,
			  AreaNotifyBuff.AreaNotifyReacod[offset].Result == e_NODE_ON_LINE ?"online":
			  AreaNotifyBuff.AreaNotifyReacod[offset].Result == e_NODE_OFF_LINE ?"offline":
			  "UNKNOWN");
		  }

		  offset = (offset+1)%AREA_NOTIFY_COUNT;
	  }
#endif
}
#if defined(STATIC_MASTER)
void ReacordDeepth(U16 Cmd, U16 SrcTei, U16 OldProxy, U16 NewProxy, U8 Newdepth, U8 NewType, U8 Reason)
{
#if DATALINKDEBUG_SW

    U16 i,j,OneLineNum=0,OfflineNum=0;

    RecordNumber++;

    for(i=0;i<MaxRecord;i++)
    {
        if(MmsgDeepth[i].Cmd ==0xff)
        {
            MmsgDeepth[i].PeriodCount = NwkBeaconPeriodCount;
            MmsgDeepth[i].Cmd = 	Cmd;
            MmsgDeepth[i].SrcAddr = SrcTei;
            MmsgDeepth[i].OldProxy = OldProxy;


            MmsgDeepth[i].NewProxy = NewProxy;

            MmsgDeepth[i].Newdepth = Newdepth;
            MmsgDeepth[i].NewType = NewType;
            MmsgDeepth[i].Reason = Reason;

            if(MmsgDeepth[i].OldProxy == CCO_TEI)
            {
                MmsgDeepth[i].OldProxyType = e_CCO_NODETYPE;
            }
            else
            {
                MmsgDeepth[i].OldProxyType = DeviceTEIList[MmsgDeepth[i].OldProxy-2].NodeType;
            }
            if(MmsgDeepth[i].NewProxy == CCO_TEI)
            {
                MmsgDeepth[i].NewProxyType = e_CCO_NODETYPE;
            }
            else
            {
                MmsgDeepth[i].NewProxyType = DeviceTEIList[MmsgDeepth[i].NewProxy-2].NodeType;
            }

            for(j=0;j<MAX_WHITELIST_NUM;j++)//统计在线和离线节点总数
            {
                if(DeviceTEIList[j].NodeState == e_NODE_OFF_LINE)
                {
                    OfflineNum++;
                }
                else if(DeviceTEIList[j].NodeState == e_NODE_ON_LINE)
                {
                    OneLineNum++;
                }
            }
            MmsgDeepth[i].OnlineNum = OneLineNum;
            MmsgDeepth[i].OfflineNum = OfflineNum;
			MmsgDeepth[i].allnum = DeviceListHeader.nr;
            break;
        }
    }
    if(i>=MaxRecord)//如果缓存存满
    {
        //memset(MmsgDeepth,0xff,sizeof(MmsgDeepth));
    }

    if(CheckErrtopology(SrcTei) !=1) //如果缓存存满
    {
        app_printf("----CheckErrtopology----STATEI=%04X---\n",SrcTei);
        //打印后清空数据
        printNetDeviceAndRecord();


    }
//	mg_update_type(STEP_5);
    return ;
#endif

}
void showDeepthrecord(tty_t *term)
{
#if DATALINKDEBUG_SW


	U16 i;
	term->op->tprintf(term,"PeriodCount------------Cmd---------SrcAddr----OldProxy--OldPTy--NewProxy--NewPTy--Newdepth-NodeType-OnLineNum-OfflineNum\n");
	for(i=0;i<MaxRecord;i++)
	{
		if(MmsgDeepth[i].Cmd !=0xff)
		{
			term->op->tprintf(term,"0x%08x----%s---%d---0x%04x--------0x%04x----%s----%04x---%s---%02x---%s---%d---%d----%d\n"
			,MmsgDeepth[i].PeriodCount
			,MmsgDeepth[i].Cmd ==  e_MMeAssocGatherInd ?"In_GatherInd":
			MmsgDeepth[i].Cmd == e_MMeAssocCfm ?"In_AssocCfm":
			MmsgDeepth[i].Cmd == e_MMeChangeProxyCnf ?"In_ProxyCnf":
			MmsgDeepth[i].Cmd == 0xfc ?"TimeOut_offline":
			MmsgDeepth[i].Cmd == 0xfe ?"TimeOut_outnet":
			"UNKNOWN"
		    ,MmsgDeepth[i].Reason
			,MmsgDeepth[i].SrcAddr
			,MmsgDeepth[i].OldProxy
			,MmsgDeepth[i].OldProxyType== e_STA_NODETYPE ?"STA":
			MmsgDeepth[i].OldProxyType== e_PCO_NODETYPE ?"PCO":
			MmsgDeepth[i].OldProxyType== e_CCO_NODETYPE ?"CCO":
			"UNKNOWN"
			,MmsgDeepth[i].NewProxy
			,MmsgDeepth[i].NewProxyType== e_STA_NODETYPE ?"STA":
			MmsgDeepth[i].NewProxyType== e_PCO_NODETYPE ?"PCO":
			MmsgDeepth[i].NewProxyType== e_CCO_NODETYPE ?"CCO":
			"UNKNOWN"
			,MmsgDeepth[i].Newdepth,
			MmsgDeepth[i].NewType== e_STA_NODETYPE ?"STA":
			MmsgDeepth[i].NewType== e_PCO_NODETYPE ?"PCO":
			MmsgDeepth[i].NewType== e_CCO_NODETYPE ?"CCO":
			"UNKNOWN"
			,MmsgDeepth[i].OnlineNum
			,MmsgDeepth[i].OfflineNum
			,MmsgDeepth[i].allnum);
			memset(&MmsgDeepth[i],0xff,sizeof(MMS_DEEPTH_t));

		}

	}
#endif
}
void printNetDeviceAndRecord()
{
#if DATALINKDEBUG_SW

        U16 i,ii,jj;
        U8 MaxDepth=0,Depth;
        U8 macaddr[6]={0xff,0xff,0xff,0xff,0xff,0xff};

        app_printf("PeriodCount-Cmd-Reason-SrcAddr-OldProxy-OldPTy-NewProxy-NewPTy-Newdepth-NodeType-OnLineNum-OfflineNum-RecordNumber=%d",RecordNumber);
        app_printf("\n");
        for(i=0;i<MaxRecord;i++)
        {
           if(MmsgDeepth[i].Cmd !=0xff)
          {
                   app_printf("0x%08x----%s---%d---0x%04x----0x%04x----%s----%04x---%s---%02x---%s---%d---%d---%d---%c%c\n"
                   ,MmsgDeepth[i].PeriodCount
                   ,MmsgDeepth[i].Cmd == e_MMeAssocGatherInd ?"In_GatherInd":
                    MmsgDeepth[i].Cmd == e_MMeAssocCfm ?"In_AssocCfm":
                    MmsgDeepth[i].Cmd == e_MMeChangeProxyCnf ?"In_ProxyCnf":
                    MmsgDeepth[i].Cmd == 0xfc ?"TimeOut_offline":
                    MmsgDeepth[i].Cmd == 0xfe ?"TimeOut_outnet":
                        "UNKNOWN"
                    ,MmsgDeepth[i].Reason
                   ,MmsgDeepth[i].SrcAddr
                   ,MmsgDeepth[i].OldProxy
                    ,MmsgDeepth[i].OldProxyType== e_STA_NODETYPE ?"STA":
                    MmsgDeepth[i].OldProxyType== e_PCO_NODETYPE ?"PCO":
                    MmsgDeepth[i].OldProxyType== e_CCO_NODETYPE ?"CCO":
                    "UNKNOWN"
                   ,MmsgDeepth[i].NewProxy
                   ,MmsgDeepth[i].NewProxyType== e_STA_NODETYPE ?"STA":
                    MmsgDeepth[i].NewProxyType== e_PCO_NODETYPE ?"PCO":
                    MmsgDeepth[i].NewProxyType== e_CCO_NODETYPE ?"CCO":
                    "UNKNOWN"
                   ,MmsgDeepth[i].Newdepth,
                    MmsgDeepth[i].NewType== e_STA_NODETYPE ?"STA":
                    MmsgDeepth[i].NewType== e_PCO_NODETYPE ?"PCO":
                    MmsgDeepth[i].NewType== e_CCO_NODETYPE ?"CCO":
                    "UNKNOWN"
                    ,MmsgDeepth[i].OnlineNum
                    ,MmsgDeepth[i].OfflineNum
                    ,MmsgDeepth[i].allnum
                    ,DeviceTEIList[MmsgDeepth[i].SrcAddr-2].ManufactorCode[0]
                    ,DeviceTEIList[MmsgDeepth[i].SrcAddr-2].ManufactorCode[1]);
                    memset(&MmsgDeepth[i],0xff,sizeof(MMS_DEEPTH_t));
          
          }


        }
        //打印设备列表
        app_printf("DeviceTEIList: PCONum=%d,STANum=%d,nr=%d\n",nnib.PCOCount,nnib.discoverSTACount,DeviceListHeader.nr);
        app_printf("Mac          Lv TEI PTEI State   Dur  NdTyPe LkTp Phase SfVer VndCode route");
        app_printf("\n");
        jj=0;

          for(ii=0;ii<MAX_WHITELIST_NUM;ii++)//最大层级数
          {
            if(DeviceTEIList[ii].NodeTEI != BROADCAST_TEI)
            {
                MaxDepth = DeviceTEIList[ii].NodeDepth>MaxDepth? DeviceTEIList[ii].NodeDepth:MaxDepth;
                if(MaxDepth ==0x0f)break;
            }
          }
          for(Depth=0;Depth<MaxDepth;Depth++)
          {
              for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
              {
                if(0==memcmp(DeviceTEIList[ii].MacAddr,macaddr,6) || DeviceTEIList[ii].NodeDepth !=Depth+1 )continue;
                app_printf("%02x%02x%02x%02x%02x%02x %02d %03x %03x  %-7s %-4d %-6s %-4s %-5s %02x%02x  %c%c      %02x %s\n",
                        DeviceTEIList[ii].MacAddr[0],
                        DeviceTEIList[ii].MacAddr[1],
                        DeviceTEIList[ii].MacAddr[2],
                        DeviceTEIList[ii].MacAddr[3],
                        DeviceTEIList[ii].MacAddr[4],
                        DeviceTEIList[ii].MacAddr[5],
                        DeviceTEIList[ii].NodeDepth,
                        DeviceTEIList[ii].NodeTEI,
                        DeviceTEIList[ii].ParentTEI,
                        DeviceTEIList[ii].NodeState== e_NODE_ON_LINE ?"Online":
                        DeviceTEIList[ii].NodeState== e_NODE_OFF_LINE ?"Offline":
                        "UNKNOWN",
                        DeviceTEIList[ii].DurationTime,
                        DeviceTEIList[ii].NodeType== e_STA_NODETYPE ?"STA":
                        DeviceTEIList[ii].NodeType== e_PCO_NODETYPE ?"PCO":
                        DeviceTEIList[ii].NodeType== e_CCO_NODETYPE ?"CCO":
                        "UNKNOWN",
                        DeviceTEIList[ii].LinkType== e_HPLC_Link ?"HPLC":
                        DeviceTEIList[ii].LinkType== e_RF_Link   ?"HRF":
                        "UNKNOWN",
                        DeviceTEIList[ii].Phase== e_A_PHASE ?"A":
                        DeviceTEIList[ii].Phase== e_B_PHASE ?"B":
                        DeviceTEIList[ii].Phase== e_C_PHASE ?"C":
                        "U",
//                        DeviceTEIList[ii].NodeMachine== e_HasSendBeacon ?"e_HasSendBeacon":
//                        DeviceTEIList[ii].NodeMachine== e_IsSendBeacon ?"e_IsSendBeacon":
//                        DeviceTEIList[ii].NodeMachine== e_NewNode ?"e_NewNode":
//                        "UNKNOWN",
                        DeviceTEIList[ii].SoftVerNum[1],
                        DeviceTEIList[ii].SoftVerNum[0],
                        DeviceTEIList[ii].ManufactorCode[1],
                        DeviceTEIList[ii].ManufactorCode[0],

                        NwkRoutingTable[DeviceTEIList[ii].NodeTEI-1].NextHopTEI,
                        NwkRoutingTable[DeviceTEIList[ii].NodeTEI-1].LinkType == e_HPLC_Link ? "HPLC":
                        NwkRoutingTable[DeviceTEIList[ii].NodeTEI-1].LinkType == e_RF_Link   ? "HRF":"UKW"

                        );
                jj++;
              }
          }

          app_printf("DeviceTEIList num : %d\n",jj);
          memset(MmsgDeepth,0xff,sizeof(MmsgDeepth));
#endif
}

#endif  // static_master

void printNeighborDiscoveryTableEntry()
{
    U8 Nullmac[6]={0};
    U16 i,nr;
    list_head_t *pos;
    ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    g_vsh.term->op->tprintf(g_vsh.term,"SNID\t\tMacAddr\t\tTEI\tType\tDepth\tRltn\tPhase\tBpRtFlg\tGAIN\tLULSR\tLDLSR\tULSR\tDLSR\tMYRCV\tPCOSED\tThisRCV\tPCORCV\tRLT\n");

      //mutex_pend(&task_work_mutex, 0);
      nr = NeighborDiscoveryHeader.nr;
      pos= NeighborDiscoveryHeader.link.next;
      //mutex_post(&task_work_mutex);
      for(i=0;i<nr;i++)
      {
        //mutex_pend(&task_work_mutex, 0);
        if(NULL == pos)
        {
            g_vsh.term->op->tprintf(g_vsh.term,"link err\n");
            break;
        }
        if(pos == &NeighborDiscoveryHeader.link)
            break;

        DiscvryTable = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
        pos = pos->next;
        //mutex_post(&task_work_mutex);
        if(memcmp(DiscvryTable->MacAddr,Nullmac,6) ==0)
        {
            // continue;
        }
        g_vsh.term->op->tprintf(g_vsh.term,"%08x\t%02x%02x%02x%02x%02x%02x\t%04x\t%02x\t%02x\t%02x\t%02x\t%02x\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%04x\n",

        DiscvryTable->SNID,
        DiscvryTable->MacAddr[0],
        DiscvryTable->MacAddr[1],
        DiscvryTable->MacAddr[2],
        DiscvryTable->MacAddr[3],
        DiscvryTable->MacAddr[4],
        DiscvryTable->MacAddr[5],
        DiscvryTable->NodeTEI,
        DiscvryTable->NodeType,
        DiscvryTable->NodeDepth,
        DiscvryTable->Relation,
        DiscvryTable->Phase,

        DiscvryTable->BKRouteFg,
        DiscvryTable->HplcInfo.GAIN/DiscvryTable->HplcInfo.RecvCount,
        DiscvryTable->HplcInfo.LastUplinkSuccRatio,
        DiscvryTable->HplcInfo.LastDownlinkSuccRatio,

        DiscvryTable->HplcInfo.UplinkSuccRatio,
        DiscvryTable->HplcInfo.DownlinkSuccRatio,
//				  DiscvryTable->LMR,
        DiscvryTable->HplcInfo.My_REV,
        DiscvryTable->HplcInfo.PCO_SED,
        DiscvryTable->HplcInfo.ThisPeriod_REV,
        DiscvryTable->HplcInfo.PCO_REV,
//				  DiscvryTable->PerRoutePeriodRemainTime,
        DiscvryTable->RemainLifeTime);


      }
     g_vsh.term->op->tprintf(g_vsh.term,"NeibNum : %d\n",NeighborDiscoveryHeader.nr);


     //无线邻居表打印
     ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *rfNode;
     g_vsh.term->op->tprintf(g_vsh.term,"SNID\t\tMacAddr\t\tTEI\tType\tDepth\tRltn\tPhase\tBpRtFlg\tRSSI\tSNR\tLUPRCV\tLDRCV\tUPRCV\tDRCV\tRLT\n");


     nr = RfNeighborDiscoveryHeader.nr;
     pos= RfNeighborDiscoveryHeader.link.next;
     for(i=0;i<nr;i++)
     {
         //mutex_pend(&task_work_mutex, 0);
         if(pos == &RfNeighborDiscoveryHeader.link)
             break;
         rfNode = list_entry(pos, ALL_NEIGHBOR_DISCOVERY_TABLE_ENTRY_t, link);
         pos = pos->next;
         //mutex_post(&task_work_mutex);
         if(memcmp(rfNode->MacAddr,Nullmac,6) ==0)
         {
            //  continue;
         }
         g_vsh.term->op->tprintf(g_vsh.term,"%08x\t%02x%02x%02x%02x%02x%02x\t%04x\t%s\t%02x\t%02x\t%02x\t%02x\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                                 rfNode->SNID,
                                 rfNode->MacAddr[0],
                                 rfNode->MacAddr[1],
                                 rfNode->MacAddr[2],
                                 rfNode->MacAddr[3],
                                 rfNode->MacAddr[4],
                                 rfNode->MacAddr[5],
                                 rfNode->NodeTEI,
                                 rfNode->NodeType == e_CCO_NODETYPE ? "CCO" :
                                 rfNode->NodeType == e_STA_NODETYPE ? "STA" :
                                 rfNode->NodeType == e_PCO_NODETYPE ? "PCO" : "UNW",
                                 rfNode->NodeDepth,
                                 rfNode->Relation,
                                 rfNode->Phase,
                                 rfNode->BKRouteFg,
                                 rfNode->HrfInfo.DownRssi,
                                 rfNode->HrfInfo.DownSnr,
                                 rfNode->HrfInfo.LUpRcvRate,
                                 rfNode->HrfInfo.LDownRcvRate,
                                 rfNode->HrfInfo.UpRcvRate,
                                 rfNode->HrfInfo.DownRcvRate,
                                //  nnib.LProxyNodeUplinkRatio,//rfNode->UpRcvRate,
                                //  nnib.LProxyNodeDownlinkRatio,//rfNode->DownRcvRate,
                                //  nnib.ProxyNodeUplinkRatio,//rfNode->UpRcvRate,
                                //  nnib.ProxyNodeDownlinkRatio,//rfNode->DownRcvRate,
                                 rfNode->RemainLifeTime);
     }
    #if defined(STATIC_NODE)
    {
        g_vsh.term->op->tprintf(g_vsh.term,"ProxyRatio:%d\t%d\t%d\t%d\n",
                                nnib.LProxyNodeUplinkRatio,//rfNode->UpRcvRate,
                                nnib.LProxyNodeDownlinkRatio,//rfNode->DownRcvRate,
                                nnib.ProxyNodeUplinkRatio,//rfNode->UpRcvRate,
                                nnib.ProxyNodeDownlinkRatio//rfNode->DownRcvRate,
                                );
    }
    #endif
     
      g_vsh.term->op->tprintf(g_vsh.term,"RfNeibNum : %d\n",RfNeighborDiscoveryHeader.nr);


}



void printNetRouteInfo()
{
    U16 jj = 0;
    U16 i;
    tty_t *term = g_vsh.term;

    term->op->tprintf(term,"DestTEI\tNxHpTEI\tRtSt\tRtErr\tLink\tActiveLk\n");

    for(i=0; i<NWK_MAX_ROUTING_TABLE_ENTRYS; i++)
    {
        if(NwkRoutingTable[i].NextHopTEI != BROADCAST_TEI)
        {
            term->op->tprintf(term,"%04x\t%04x\t%x\t%x\t%s\t%02x\n",
                              i+1,
                              NwkRoutingTable[i].NextHopTEI,
                              NwkRoutingTable[i].RouteState,
                              NwkRoutingTable[i].RouteError,
                              NwkRoutingTable[i].LinkType == e_HPLC_Link ? "HPLC":
                              NwkRoutingTable[i].LinkType == e_RF_Link   ? "HRF":"UKW",
                              NwkRoutingTable[i].ActiveLk);
            jj++;
        }
    }
    term->op->tprintf(term,"NwkRt num: %d,last_My_SED=%d\n",jj,nnib.last_My_SED);
}


void printNetInfo()
{
    #if defined(STATIC_MASTER)
    uint16_t i;
    uint16_t DevListNum = 0;
    uint16_t OnlineNum = 0;
    uint16_t OfflineNum = 0;
    uint8_t  maxdepth = 0;
    uint16_t pcoNum = 0;
    uint16_t staNum = 0;
    uint16_t HplcNum = 0;
    uint16_t HrfNum = 0;
    tty_t *term = g_vsh.term;

    //统计版本信息
    VERSION_t version[DISPLAYNUM];
    memset((U8 *)&version,0,DISPLAYNUM*sizeof(VERSION_t));


    uint16_t  depthNum[15] = {0};
    
    for(i=0; i < MAX_WHITELIST_NUM; i++)
    {
        if(0==memcmp(DeviceTEIList[i].MacAddr,BroadcastAddress,MAC_ADDR_LEN))
            continue;

        //记录入网节点数量
        DevListNum++;

        //统计最大层级
        maxdepth = DeviceTEIList[i].NodeDepth > maxdepth ? DeviceTEIList[i].NodeDepth : maxdepth;
        //统计每个层级节点数量
        depthNum[DeviceTEIList[i].NodeDepth-1] += 1;

        //统计pco、STA 数量
        if(DeviceTEIList[i].NodeType == e_PCO_NODETYPE)
        {
            pcoNum++;
        }
        else if(DeviceTEIList[i].NodeType == e_STA_NODETYPE)
        {
           staNum++;
        }

        //统计online 、 pffline 节点数量
        if(DeviceTEIList[i].NodeState == e_NODE_ON_LINE)
        {
            OnlineNum++;
        }
        else if(DeviceTEIList[i].NodeState == e_NODE_OFF_LINE)
        {
            OfflineNum++;
        }

        //统计无线、载波入网节点数量
        if(DeviceTEIList[i].LinkType == e_RF_Link)
        {
            HrfNum++;
        }
        else if(DeviceTEIList[i].LinkType == e_HPLC_Link)
        {
            HplcNum++;
        }

        check_version(DeviceTEIList[i].ManufactorCode,DeviceTEIList[i].SoftVerNum,DeviceTEIList[i].ManufactorInfo.InnerVer,version,DISPLAYNUM);
    }

    term->op->tprintf(term, "Doc Num  : %d\n"
                            "DevNum   : %d\n"
                            "HPLC     : %d\n"
                            "HRF      : %d\n"
                            "ONLINE   : %d\n"
                            "OFFLINE  : %d\n"
                            "PCO NUM  : %d\n"
                            "STA NUM  : %d\n"
                            "MAX Lvl  : %d\n"
                            , FlashInfo_t.usJZQRecordNum
                            , DevListNum
                            , HplcNum
                            , HrfNum
                            , OnlineNum
                            , OfflineNum
                            , pcoNum
                            , staNum
                            , maxdepth);

    //打印各层级节点数量
    for(i=0; i < maxdepth; i++)
    {
        term->op->tprintf(term,"Lvl  %d   : %d\n",i+1, depthNum[i]);
    }
    term->op->tprintf(term, "\n");
    //打印节点版本信息
    for(i=0;i<DISPLAYNUM;i++)
    {
        if(0 != version[i].cnt)
        {
            term->op->tprintf(term,"version: %02x%02x cnt: %d\n",version[i].version[1],version[i].version[0],version[i].cnt);
        }
    }

    #endif
}


void printDevListInfo()
{
#if defined(STATIC_MASTER)
    U16 jj=0;
    U16 ii = 0;
    U8 macaddr[6]={0xff,0xff,0xff,0xff,0xff,0xff};
    tty_t *term = g_vsh.term;

    term->op->tprintf(term,"DvcTEILst: \nMacAddr\t\tLvl-TEI-PTEI NdSt\tDurTim\tNODETYP\tLINKTYPE\tPhase\tChMnYeMoDa-SfVr InnYMD-SfVr up/dwRa\tdevty\tmodeid\tD012\t567\tedge\tModType\n");
    jj=0;
    U8 MaxDepth=0,Depth = 0;
    VERSION_t version[DISPLAYNUM];
    memset((U8 *)&version,0,DISPLAYNUM*sizeof(VERSION_t));
    for(ii=0;ii<MAX_WHITELIST_NUM;ii++)//最大层级数
    {
        if(DeviceTEIList[ii].NodeTEI != 0xfff)
        {
            MaxDepth = DeviceTEIList[ii].NodeDepth>MaxDepth? DeviceTEIList[ii].NodeDepth:MaxDepth;
            if(MaxDepth ==0x0f)
                break;
        }
    }
    for(Depth=0; Depth < MaxDepth; Depth++)
    {
        for(ii=0;ii<MAX_WHITELIST_NUM;ii++)
        {
            if(0==memcmp(DeviceTEIList[ii].MacAddr,macaddr,6) || DeviceTEIList[ii].NodeDepth !=Depth+1 )
                continue;
            term->op->tprintf(term,"%02x%02x%02x%02x%02x%02x\t%-4d%-4x%-5x%-7s\t%-3d\t%-3s\t%-4s\t\t%-1s %-1s\t%c%c%02d%02d%02d %02x%02x   %02d%02d%02d %02x%02x %03d %03d\t%-1s\t%02x\t%d%d%d\t%-1s\t%-1s\t%02x\n",
            DeviceTEIList[ii].MacAddr[0],
            DeviceTEIList[ii].MacAddr[1],
            DeviceTEIList[ii].MacAddr[2],
            DeviceTEIList[ii].MacAddr[3],
            DeviceTEIList[ii].MacAddr[4],
            DeviceTEIList[ii].MacAddr[5],
            DeviceTEIList[ii].NodeDepth,
            DeviceTEIList[ii].NodeTEI,
            DeviceTEIList[ii].ParentTEI,
            DeviceTEIList[ii].NodeState== e_NODE_ON_LINE ?"Online":
            DeviceTEIList[ii].NodeState== e_NODE_OFF_LINE ?"Offline":
            "UNKNOWN",
            DeviceTEIList[ii].DurationTime,
            DeviceTEIList[ii].NodeType== e_STA_NODETYPE ?"STA":
            DeviceTEIList[ii].NodeType== e_PCO_NODETYPE ?"PCO":
            DeviceTEIList[ii].NodeType== e_CCO_NODETYPE ?"CCO":
            "UNK",
            DeviceTEIList[ii].LinkType== e_HPLC_Link ?"HPLC":
            DeviceTEIList[ii].LinkType== e_RF_Link   ?"HRF":
            "UNK",
            DeviceTEIList[ii].Phase== e_A_PHASE ?"A":
            DeviceTEIList[ii].Phase== e_B_PHASE ?"B":
            DeviceTEIList[ii].Phase== e_C_PHASE ?"C":
            "U",
            DeviceTEIList[ii].LogicPhase== e_A_PHASE ?"A":
            DeviceTEIList[ii].LogicPhase== e_B_PHASE ?"B":
            DeviceTEIList[ii].LogicPhase== e_C_PHASE ?"C":
            "U",


            DeviceTEIList[ii].ManufactorCode[1],
            DeviceTEIList[ii].ManufactorCode[0],
            //DeviceTEIList[ii].ChipVerNum[1],
            //DeviceTEIList[ii].ChipVerNum[0],
            (DeviceTEIList[ii].BuildTime )&0x7F,
            (DeviceTEIList[ii].BuildTime >> 7)&0x0F,
            (DeviceTEIList[ii].BuildTime >> 11)&0x1F,
            DeviceTEIList[ii].SoftVerNum[1],
            DeviceTEIList[ii].SoftVerNum[0],
            (DeviceTEIList[ii].ManufactorInfo.InnerBTime )&0x7F,
            (DeviceTEIList[ii].ManufactorInfo.InnerBTime >> 7)&0x0F,
            (DeviceTEIList[ii].ManufactorInfo.InnerBTime >> 11)&0x1F,
            DeviceTEIList[ii].ManufactorInfo.InnerVer[1],
            DeviceTEIList[ii].ManufactorInfo.InnerVer[0],
            DeviceTEIList[ii].UplinkSuccRatio,
            DeviceTEIList[ii].DownlinkSuccRatio,

            DeviceTEIList[ii].DeviceType == e_CKQ ?"e_CKQ":
            DeviceTEIList[ii].DeviceType == e_METER ?"e_METER":
            DeviceTEIList[ii].DeviceType == e_RELAY ?"e_RELAY":
            DeviceTEIList[ii].DeviceType == e_CJQ_1 ?"CJQ1":
            DeviceTEIList[ii].DeviceType == e_CJQ_2 ?"CJQ2":
            DeviceTEIList[ii].DeviceType == e_3PMETER ?"3XB":
            "UNKOWN",
            DeviceTEIList[ii].ModeNeed,
            DeviceTEIList[ii].F31_D0,
            DeviceTEIList[ii].F31_D1,
            DeviceTEIList[ii].F31_D2,

            DeviceTEIList[ii].DeviceType == e_3PMETER?
            ((DeviceTEIList[ii].F31_D5 == 0 && DeviceTEIList[ii].F31_D6 ==0 && DeviceTEIList[ii].F31_D7 ==0)?"ABC":
            (DeviceTEIList[ii].F31_D5 == 1 && DeviceTEIList[ii].F31_D6 ==0 && DeviceTEIList[ii].F31_D7 ==0)?"ACB":
            (DeviceTEIList[ii].F31_D5 == 0 && DeviceTEIList[ii].F31_D6 ==1 && DeviceTEIList[ii].F31_D7 ==0)?"BAC":
            (DeviceTEIList[ii].F31_D5 == 1 && DeviceTEIList[ii].F31_D6 ==1 && DeviceTEIList[ii].F31_D7 ==0)?"BCA":
            (DeviceTEIList[ii].F31_D5 == 0 && DeviceTEIList[ii].F31_D6 ==0 && DeviceTEIList[ii].F31_D7 ==1)?"CAB":
            (DeviceTEIList[ii].F31_D5 == 1 && DeviceTEIList[ii].F31_D6 ==0 && DeviceTEIList[ii].F31_D7 ==1)?"CBA":
            (DeviceTEIList[ii].F31_D5 == 0 && DeviceTEIList[ii].F31_D6 ==1 && DeviceTEIList[ii].F31_D7 ==1)?"N-L":
            "NOR"):(DeviceTEIList[ii].LNerr == 1?"ERR":DeviceTEIList[ii].LNerr == 0?"OK ":"UKW"),

            DeviceTEIList[ii].Edgeinfo == 0 ?"DOWN":
            DeviceTEIList[ii].Edgeinfo == 1 ?"UP":
            "UKW",
            DeviceTEIList[ii].ModuleType
            );
            //S8 check_version(U8  *ManufactorCode, U8 *check_ver,U8 * check_innerver , VERSION_t *version , U8 cnt);
            check_version(DeviceTEIList[ii].ManufactorCode,DeviceTEIList[ii].SoftVerNum,DeviceTEIList[ii].ManufactorInfo.InnerVer,version,DISPLAYNUM);
            jj++;
        }

    }
    U8 buf[2]={0,0};
    for(ii=0;ii<DISPLAYNUM;ii++)
    {
        if(0!=memcmp(buf,version[ii].version,2))
        {
            term->op->tprintf(term,"version : %02x%02x   cnt : %d\n",version[ii].version[1],version[ii].version[0],version[ii].cnt);
        }
    }

    term->op->tprintf(term,"DevList num: %d, nr:%d\n",jj, DeviceListHeader.nr);
#endif
}

void printNetAQ(tty_t *term)
{
    #if DATALINKDEBUG_SW

    //list_head_t *pos,*node;
    //NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *DiscvryTable;

    printNetRouteInfo();
    //printNeighborDiscoveryTableEntry();

    #if defined(STATIC_MASTER)

//            term->op->tprintf(term,"SysNetworkInfo: \nPCOCountNum=%d,discoverSTACount=%d\n",SysNetworkInfo.PCOCount,SysNetworkInfo.discoverSTACount);
    printDevListInfo();
    #endif
    //showdealtime();
    #endif
}

void periodprinfo(work_t *work)
{
	
#if defined(STATIC_MASTER)
	printNetDeviceAndRecord();
#else
	//printNeighborDiscoveryTableEntry();
#endif
}
ostimer_t print_timer;

void periodprint_timerCB( struct ostimer_s *ostimer, void *arg)
{
	
	
    work_t *work = zc_malloc_mem(sizeof(work_t),"PTCB",MEM_TIME_OUT);
	work->work = periodprinfo;
	work->msgtype = PRE_PRINT;
	//post_datalink_task_work(work);
	post_app_work(work); //datalink too busy so^^
    timer_start(&print_timer);
}



void printSwitch(U8 active)
{
    if(print_timer.fn == NULL)
	{
        timer_init(&print_timer,
                   10*60*1000,
//                   20*1000,
                   10,
                   TMR_TRIGGER,//TMR_TRIGGER
                   periodprint_timerCB,
                   NULL,
                   "periodprint_timerCB"
                   );
	}
	if(active)
    timer_start(&print_timer);
	else
    timer_stop(&print_timer,TMR_NULL);
		
	
	
}



