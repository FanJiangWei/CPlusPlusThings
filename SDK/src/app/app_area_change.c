
#include "app.h"
#include "app_common.h"
#include "datalinkdebug.h"
#include "app_area_change.h"
#include "app_event_report_cco.h"
#include "SchTask.h"
#include "app_dltpro.h"


#if defined(STATIC_MASTER)
static void area_change_report_wait_timercb(struct ostimer_s *ostimer, void *arg);

//̨������

static U8 gl_area_change_flag = FALSE;
ostimer_t *area_change_report_ctrl_timer  = NULL; //�����û��ϱ�Ƶ�ʿ��Ƽ�ʱ��
ostimer_t *area_change_report_wait_timer = NULL;

REPORT_NEW_BLACK_t area_change_report_list[MAX_NEW_REPORT_BLACK_SIZE]; //��ʼ�������û��ϱ�������


void area_change_set_report_flag(U8 arg_state)
{
	gl_area_change_flag = arg_state;
}

void area_change_show_report_info(tty_t *term)
{

    U16 jj=0;
    U8 buf[8] = {0};
    term->op->tprintf(term,"Report\tDeType\tAddr\t\n");
    for(jj=0; jj<POWERNUM; jj++)
    {
        if(memcmp(area_change_report_list[jj].MacAddr,buf,8)!=0)
        {

            term->op->tprintf(term,"%d\t%d\t",
            area_change_report_list[jj].ReportFlag,
            area_change_report_list[jj].DeviceType);
            dump_tprintf(term,area_change_report_list[jj].MacAddr ,6);
        }
    }
}

static void area_change_report_wait_timer_modify(U32 MStimes)
{
	timer_modify(area_change_report_wait_timer,
               MStimes,
               0,
               TMR_TRIGGER ,
               area_change_report_wait_timercb,
               NULL,
               "area_change_report_wait_timercb",
               TRUE
               );
	timer_start(area_change_report_wait_timer);
}



void area_change_report_wait_timer_refresh(void)
{
    if(gl_area_change_flag==TRUE && zc_timer_query(area_change_report_wait_timer) != TMR_STOPPED)
    {
        area_change_report_wait_timer_modify(60*TIMER_UNITS);
    }
}



void area_change_report(U8 *mac_addr,U8 device_type)
{
    U8  null_addr[6] = {0,0,0,0,0,0};
    U8  firstFindFlag = TRUE;
    U16  firstFindIndex = 0;
    U16  i;
    U16  reportnum = 0;
    U8   needreport = FALSE;

    if(gl_area_change_flag == TRUE)
    {
        //����ȫ00�ڵ�
        if(memcmp(null_addr, mac_addr, 6) == 0)
        {
            return ;
        }
		
        ChangeMacAddrOrder(mac_addr);
		
        for(i = 0; i < MAX_NEW_REPORT_BLACK_SIZE; i++)
        {
            if(0 == memcmp(mac_addr, area_change_report_list[i].MacAddr, 6))
            {
                app_printf("In Balck%d: ",i);
                dump_buf(area_change_report_list[i].MacAddr, 6);
                break;                                                         //����������ֱ��break;
            }
            else if(0 == memcmp(null_addr, area_change_report_list[i].MacAddr, 6 ) && firstFindFlag) //�ҵ���һ����λ��
            {
                firstFindFlag = FALSE;
                firstFindIndex = i;
                //app_printf("firstFindIndex = %d\n", firstFindIndex);
            }
            if(area_change_report_list[i].ReportFlag)
      		{
          		reportnum ++;
                
            }
        }
        if(i == MAX_NEW_REPORT_BLACK_SIZE&&firstFindFlag == FALSE) //�ҵ���λ�ã����򻺴�����
        {
            
            __memcpy(area_change_report_list[firstFindIndex].MacAddr, mac_addr, 6);
            area_change_report_list[firstFindIndex].DeviceType = device_type; //����6Сʱ
            area_change_report_list[firstFindIndex].ReportFlag = TRUE;
            needreport = TRUE;//��ǰ�ڵ���Ҫ�ϱ�
            reportnum ++;
        }
        if(needreport == TRUE)
        {	
        	app_printf("reportnum = %d\n",reportnum);
            if(reportnum >= 32)
      		{
        		app_printf(">32\n");
                if(timer_remain(area_change_report_wait_timer) > (2*TIMER_UNITS))
                {
            		area_change_report_wait_timer_modify(2*TIMER_UNITS);
                }
      		}
            else
            {
                area_change_report_wait_timer_modify(60*TIMER_UNITS);
            }
        }
        //�״�ˢ��
        if(zc_timer_query(area_change_report_ctrl_timer) == TMR_STOPPED)
        {
            timer_start(area_change_report_ctrl_timer);
        }
    }
    else
    {
        app_printf("Rp 0!\n");
    }
}



static void area_change_report_wait_timer_fun(work_t *work)			//̨�����з��Ͷ�ʱ���ص�����
{    
	if(gl_area_change_flag==TRUE)
	{
	    U16 byLen = 0,ii;
    	U8	reportnum = 0;//255		//U16	num;
    	U8  null_addr[6] = {0};
        U8  *Gw3762TopUpReportData = NULL;
		U8  meteraddr[6];
        U8 device_type = APP_GW3762_M_SLAVETYPE;
        U8 max_rpt = 0U;
        
		byLen ++;	//���ֽ���Ϊ�ϱ�����
		
		reportnum = 0;

        if(PROVINCE_HEBEI == app_ext_info.province_code)
        {
            max_rpt = 1U;
        }
        else
        {
            max_rpt = 32U;
        }

        Gw3762TopUpReportData = zc_malloc_mem(300,"ReportWaitBuf",MEM_TIME_OUT);

        if(NULL == Gw3762TopUpReportData)
        {
            return;
        }
		
		for(ii=0; ii<MAX_NEW_REPORT_BLACK_SIZE; ii++)
		{
			if(area_change_report_list[ii].ReportFlag)
			{
			    if(memcmp(null_addr,area_change_report_list[ii].MacAddr,6)==0)
				{
					continue;
				}
			    if(check_whitelist_by_addr(area_change_report_list[ii].MacAddr)==TRUE)
				{
					memset(area_change_report_list[ii].MacAddr,0x00,8);
					//app_printf("repert>%d \n",ii);
					continue;
				}
				__memcpy(meteraddr,area_change_report_list[ii].MacAddr,6);

                if(PROVINCE_HEBEI == app_ext_info.province_code)
                {
                    //�ӱ�̨������
                    U8  dlt645frame[20];
                    U8  buff[7] = { 0x04, 0, 0, 0, 0, 0, 0 };//�ӱ���׼Ϊ04��
                    Add0x33ForData(buff, sizeof(buff));
                    byLen = 0;
                    Packet645Frame(dlt645frame, (U8 *)&byLen, meteraddr, 0x9E, buff, sizeof(buff));
                    app_printf("AreachangeReport:   ");
                    dump_buf(dlt645frame, byLen);
                    __memcpy(&Gw3762TopUpReportData[0], dlt645frame, byLen);

                    if((area_change_report_list[ii].DeviceType == e_CJQ_2) || (area_change_report_list[ii].DeviceType == e_CJQ_1))
                    {
                        device_type = APP_GW3762_C_SLAVETYPE;
                    }
                }
                else
                {
				    __memcpy(&Gw3762TopUpReportData[byLen], meteraddr, 6);
				    byLen+= 6;
				    Gw3762TopUpReportData[byLen++] = area_change_report_list[ii].DeviceType ;
                }
				
                area_change_report_list[ii].ReportFlag = FALSE;
				reportnum++;
			}
			if(reportnum >= max_rpt)
			{
				app_printf("some many need to report !\n");
				area_change_report_wait_timer_modify(2*1000);
				break;
			}
		}
	
		if(reportnum > 0)
		{
            if(PROVINCE_HEBEI == app_ext_info.province_code)
            {
                app_gw3762_up_afn06_f5_report_slave_event( FALSE,
                                                    device_type,
                                                    TRANS_PROTO_07, //�ӱ�����������ʵ��Э�������ϱ�
                                                    meteraddr,
                                                    byLen,
                                                    Gw3762TopUpReportData);
            }
            else
            {
			    Gw3762TopUpReportData[0] = reportnum;//��֡��������
			    app_gw3762_up_afn06_f5_report_slave_event( 	FALSE,
														    APP_GW3762_M_SLAVETYPE ,
														    APP_BLACK_REPORT,
														    0,
														    byLen,
														    Gw3762TopUpReportData);
            }

			//app_printf("area_change_report_list Data\n");
			//dump_buf(Gw3762TopUpReportData,byLen);
		}
		
        zc_free_mem(Gw3762TopUpReportData);
	}
}



static void area_change_report_wait_timercb(struct ostimer_s *ostimer, void *arg)
{
    work_t *work = zc_malloc_mem(sizeof(work_t),"area_change_report_wait_timer_fun",MEM_TIME_OUT);
    if(NULL == work)
    {
        return;
    }
	work->work = area_change_report_wait_timer_fun;
	work->msgtype = RPTWAIT_SEND;
	post_app_work(work);
}


void area_change_report_ctrl_timercb(struct ostimer_s *ostimer, void *arg)
{
	//����6Сʱ
	app_printf("move all balckList!\n");
	memset(area_change_report_list, 0x00, sizeof(area_change_report_list));
}

uint8_t area_change_report_info_init(void)
{

    memset(area_change_report_list, 0x00, sizeof(area_change_report_list));
    
    area_change_report_ctrl_timer  = timer_create(6*60*60*TIMER_UNITS,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            area_change_report_ctrl_timercb,
                            NULL,
                            "area_change_report_ctrl_timercb"
                           );
    //̨�����л����ʱ�ϱ���ʱ��
    area_change_report_wait_timer = timer_create(60*1000,
                0,
                TMR_TRIGGER ,//TMR_TRIGGER
                area_change_report_wait_timercb,
                NULL,
                "area_change_report_wait_timercb"
               );
    return 0;
}




#endif

