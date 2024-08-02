/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	timeslot_equalization.c
 * Purpose:	
 * History:
 * Author : PanYu
 */

#include "List.h"
#include "Types.h"
#include "Trios.h"
#include "Mbuf.h"
#include "phy.h"
#include "phy_sal.h"
#include "plc.h"
#include "sys.h"
#include "vsh.h"
#include "mem_zc.h"
#include "timeslot_equalization.h"
#include <string.h>
#include "printf_zc.h"
#include "csma.h"
#include "list.h"
#include "plc_cnst.h"

#include "DataLinkInterface.h"
#include "power_manager.h"

#include "Beacon.h"
#include "dl_mgm_msg.h"
#if defined(STD_DUAL)
#include "wlc.h"
#include "wl_cnst.h"
#include "wl_csma.h"
#endif

/*���ʱ϶����ľ���λ��*/
static void mark_slot(uint8_t slot_num , uint16_t all_slot_num , uint8_t offset,uint16_t *pcnt)
{
	uint16_t ii;
	ii=0;
	if(!slot_num)
		return;
	do
	{
		*(pcnt + ii)= offset+ii*all_slot_num/slot_num;
		//phy_printf("cnt[%d] = %d\n",ii,*(pcnt+ii));
	}while(slot_num-1>ii++);
}
/*��a��b��c����ʱ϶�����մ�С����λ���ȼ�����ʱ϶С�ġ���λ��a>b>c���ȼ�˳�������ǰ��*/
static void sort_slot(uint32_t a_csma,uint32_t b_csma,uint32_t c_csma,SLOT_t *slot_t)
{
	SLOT_t change_slot_t;
	uint8_t i,j;
	slot_t[0].phase = PHASE_A;
	slot_t[0].slot = a_csma;
	slot_t[1].phase = PHASE_B;
	slot_t[1].slot = b_csma;
	slot_t[2].phase = PHASE_C;
	slot_t[2].slot = c_csma;
	
	for(i=0;i<3;i++)
	{
		for(j=i+1;j<3;j++)
		{
			if(slot_t[i].slot > slot_t[j].slot)
			{
				__memcpy(&change_slot_t,&slot_t[j],sizeof(SLOT_t));
				__memcpy(&slot_t[j],&slot_t[i],sizeof(SLOT_t));
				__memcpy(&slot_t[i],&change_slot_t,sizeof(SLOT_t));				
			}
			else if(slot_t[i].slot == slot_t[j].slot)
			{
				if(slot_t[i].phase > slot_t[j].phase)
				{
					__memcpy(&change_slot_t,&slot_t[j],sizeof(SLOT_t));
					__memcpy(&slot_t[j],&slot_t[i],sizeof(SLOT_t));
					__memcpy(&slot_t[i],&change_slot_t,sizeof(SLOT_t));
				}
			}
		}
	}
	
}
static uint32_t get_slot(uint32_t *remain_phase_csma,uint32_t slot)
{
	uint32_t get_slot_time;
	get_slot_time = 0;
	if(*remain_phase_csma == 0)
	{
		return get_slot_time;
	}
	
	if(*remain_phase_csma/slot<2)//��������ʱ϶������һ������
	{
		get_slot_time = *remain_phase_csma;
		*remain_phase_csma = 0;
	}
	else//���ڻ��ߵ�������ʱ϶�����ն��ʱ϶����
	{
		get_slot_time = slot;
		*remain_phase_csma -= get_slot_time;
	}
	
	return PHY_MS2TICK(get_slot_time);
}
U8 g_csma_scale_buf[2096];
/*a\b\c ����ʱ϶���ȣ�soltΪʱ϶��λ��solt_phaseΪ solt_all_numΪa+b+c����ʱ϶����*/
static SCALE_CSMA_t  *scale_csma_slot(uint32_t a_csma,
								uint32_t b_csma,
								uint32_t c_csma,
								uint32_t slot,
								uint16_t *slot_all_num,
								uint32_t time_stamp)
{
	SLOT_t slot_t[3];
	uint16_t ii,jj;
	uint8_t  current_phase;
	SCALE_CSMA_t  *scale_csma;
    SLOT_t *pslot_t;
    uint8_t *slot_phase;
	
	ii=jj=0;

    memset(g_csma_scale_buf, 0x00, sizeof(g_csma_scale_buf));

	pslot_t = (SLOT_t *)g_csma_scale_buf; //zc_malloc_mem(sizeof(SLOT_t)+MAX_SLOT_NUM*sizeof(uint16_t),"SLT0",MEM_TIME_OUT);

    if(!pslot_t)
    {
        return NULL;
    }
    //����
    sort_slot(a_csma,b_csma,c_csma,&slot_t[0]);

    slot_t[0].slot_num = 0==slot_t[0].slot?0:((0==slot_t[0].slot/slot)?1:slot_t[0].slot/slot);
    slot_t[1].slot_num = 0==slot_t[1].slot?0:((0==slot_t[1].slot/slot)?1:slot_t[1].slot/slot);
    slot_t[2].slot_num = 0==slot_t[2].slot?0:((0==slot_t[2].slot/slot)?1:slot_t[2].slot/slot);

    *slot_all_num = slot_t[0].slot_num + slot_t[1].slot_num + slot_t[2].slot_num;
    //phy_printf("%d + %d + %d = %d\n",slot_t[0].slot_num, slot_t[1].slot_num, slot_t[2].slot_num, *slot_all_num);

	slot_phase = zc_malloc_mem(*slot_all_num,"SCS",MEM_TIME_OUT);
    do//ȫ��ΪĬ�����(������)
    {
        slot_phase[ii]=slot_t[2].phase;

    }while(++ii<*slot_all_num);

    //�ֱ�Բ�ͬ������ʱ϶���з���
    __memcpy(pslot_t,&slot_t[0],sizeof(SLOT_t));
	//printf_s("0 pslot_t->slot_num : %d , *slot_all_num : %d , pslot_t->slot_cnt : %d\n",pslot_t->slot_num,*slot_all_num,*pslot_t->slot_cnt);
	
	mark_slot(pslot_t->slot_num,*slot_all_num,0,pslot_t->slot_cnt);
    for(jj=0;jj<pslot_t->slot_num;jj++)//���µ�һ��
    {
        if(pslot_t->slot_cnt[jj] >= *slot_all_num)
        {
            printf_s("ASLT err!\n");
            continue;
        }
        slot_phase[pslot_t->slot_cnt[jj]]=pslot_t->phase;
    }
    __memcpy(pslot_t,&slot_t[1],sizeof(SLOT_t));

	//printf_s("1 pslot_t->slot_num : %d , *slot_all_num : %d , pslot_t->slot_cnt : %d\n",pslot_t->slot_num,*slot_all_num,*pslot_t->slot_cnt);
    mark_slot(pslot_t->slot_num,*slot_all_num,1,pslot_t->slot_cnt);
    for(jj=0;jj<pslot_t->slot_num;jj++)//���µڶ���
    {
        uint16_t cur_cnt = pslot_t->slot_cnt[jj];
        while(slot_phase[cur_cnt] == slot_t[0].phase)
        {
            cur_cnt++;
            if(cur_cnt >= *slot_all_num)
            {
                printf_s("BSLT err!\n");
                break;;
            }
        }
        if(cur_cnt < *slot_all_num)
            slot_phase[cur_cnt]  = pslot_t->phase;
        //else
           // phy_printf("cur_cnt = %d, all_cnt_num = %d", cur_cnt, *slot_all_num);

    }

    // zc_free_mem(pslot_t);
    
    memset(g_csma_scale_buf, 0x00, sizeof(g_csma_scale_buf));

	ii = 0;
	jj = -1;

	scale_csma = NULL;
	current_phase = 0;

	if(*slot_all_num)
	{
		scale_csma = (SCALE_CSMA_t *)g_csma_scale_buf;// zc_malloc_mem((*slot_all_num)*sizeof(SCALE_CSMA_t),"ECST",MEM_TIME_OUT);
	}
	if(!scale_csma)
	{
		goto free;
	}
	do//����ʱ϶
	{
		if(slot_phase[ii] != current_phase)
		{
			jj++;
			if(jj == 0)
			{
				scale_csma[jj].start_timer = time_stamp;
			}
			else
			{
				scale_csma[jj].start_timer = scale_csma[jj-1].end_timer;
			}
			
			current_phase = slot_phase[ii];
			scale_csma[jj].phase = current_phase;
			
			scale_csma[jj].end_timer = ((PHASE_A == scale_csma[jj].phase)?get_slot(&a_csma,slot):
										 (PHASE_B == scale_csma[jj].phase)?get_slot(&b_csma,slot):
										 (PHASE_C == scale_csma[jj].phase)?get_slot(&c_csma,slot):0)
										 + scale_csma[jj].start_timer;
		}
		else
		{	
			if(jj >= 0)
			scale_csma[jj].end_timer += ((PHASE_A == scale_csma[jj].phase)?get_slot(&a_csma,slot):
										  (PHASE_B == scale_csma[jj].phase)?get_slot(&b_csma,slot):
										  (PHASE_C == scale_csma[jj].phase)?get_slot(&c_csma,slot):0);
		}
		//phy_printf("\na_csma : %d b_csma : %d c_csma : %d\n",a_csma,b_csma,c_csma);
		/*phy_printf("csma[%d]\t",jj);
		phy_printf("%s\t",scale_csma_t[jj].phase==PHASE_A?"A":scale_csma_t[jj].phase==PHASE_B?"B":scale_csma_t[jj].phase==PHASE_C?"C":"U");
		phy_printf("%08x\t",scale_csma_t[jj].start_timer);
		phy_printf("%08x\t",scale_csma_t[jj].end_timer);
		phy_printf("%d\n",scale_csma_t[jj].end_timer-scale_csma_t[jj].start_timer);
		os_sleep(10);*/
	}while((a_csma>0 || b_csma>0 || c_csma>0) && ++ii<*slot_all_num);
	*slot_all_num = jj+1;

	free:
	zc_free_mem(slot_phase);

	return scale_csma;

}

typedef struct
{
    SLOT_TYPE_e slotType;
    U8 RfBeaconType;
    U8 RfBeaconSlotCnt;
}__PACKED RF_BEACON_INFO_t;

static list_head_t *entry_beacon_slot(beacon_t *beacon,
                                      BEACON_PERIOD_t *beacon_info,
                                      RF_BEACON_INFO_t rfBeaconInfo)
{
	uint16_t i;
#if defined(STATIC_MASTER)
	uint8_t *p;
	uint16_t len;
	mbuf_hdr_t *mbuf_hdr;
#endif

    uint32_t time                     = beacon_info->beacon_start_time;
    uint16_t beacon_slot              = beacon_info->beacon_slot      ;
    uint16_t beacon_slot_nr           = beacon_info->beacon_slot_nr   ;
    beacon_slot_t  *beacon_slot_info  = beacon_info->beacon_slot_info ;
#if defined(STATIC_NODE)
    uint16_t sent_nr                  = beacon_info->beacon_sent_nr   ;
    if(rfBeaconInfo.RfBeaconType == e_BEACON_RF)
    {
        sent_nr = 0;
    }
#endif
	

	
	if(!beacon_slot_nr || !beacon_slot)
	{		
//        phy_printf("WARNING beacon slot is null!\n");
		return NULL;
    }

    for(i=0;i<beacon_slot_nr;i++)
    {
        bpts_t *bpts = zc_malloc_mem(sizeof(bpts_t),"bpts",MEM_TIME_OUT);

		bpts->beacon = beacon;
		bpts->start  = time + i*PHY_MS2TICK(beacon_slot);
		bpts->time  = bpts->start;
		bpts->end    = time + (i+1)*PHY_MS2TICK(beacon_slot);
		bpts->type   = beacon_slot_info->type;
		bpts->phase  = beacon_slot_info->phase;
		bpts->dir	 = DIR_RX;
		bpts->stei   = HPLC.tei;//beacon_slot_info->stei;
		bpts->sent   = NULL;
		bpts->life   = 0x3ff;
		bpts->post   = 0;
		INIT_LIST_HEAD(&bpts->pf_list);
        bpts->txflag = 1; //��¼

        // #if defined(STD_DUAL)
        // if(rfBeaconInfo.slotType == e_SLOT_TYPE_RF)
        // {
        //     bpts->op =  &wl_tdma_recv;
        // }else
        // #endif
        {
            bpts->op =  &tdma_recv;
        }

        #if defined STATIC_NODE
        if(sent_nr ==0) //δ����ʱ϶ʱ
		{
			i=beacon_slot_nr;
			bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
		}
		else if(sent_nr==i+1) //��ʱ϶ǰ��һ��ʱ϶
		{
			bpts->start  = time ;
            bpts->time  = bpts->start;
		}
        else if(sent_nr==i) //��ǰΪ�����ŵ�ʱ϶
        {

            // #if defined(STD_DUAL)
            // if(rfBeaconInfo.slotType == e_SLOT_TYPE_RF)
            // {
            //     bpts->op =  &wl_rbeacon_send;
            // }else
            // #endif
            {
                bpts->op = &rbeacon_send;
            }

            bpts->dir	 = DIR_TX;
            // bpts->time += PHY_US2TICK(200);
        }
        else if(i == (sent_nr + 1))         //��ǰʱ϶��һ��ʱ϶
        {
            i = beacon_slot_nr;             //��ǰ����ѭ��
            bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);

        }
        else
        {
            zc_free_mem(bpts);
            continue;
        }
        #elif defined STATIC_MASTER
        if(rfBeaconInfo.RfBeaconType == e_BEACON_RF)        //�����ŷ����ز��ű�
        {
            i += beacon_info->CCObcn_slot_nr;
            bpts->end    = time + beacon_info->CCObcn_slot_nr*PHY_MS2TICK(beacon_slot);
        }
        else if(i <= 2)
        {
            extern U32 NwkBeaconPeriodCount;
            p = zc_malloc_mem(520,"becn",MEM_TIME_OUT);


            // #if defined(STD_DUAL)
            // if(rfBeaconInfo.slotType == e_SLOT_TYPE_RF)
            // {
            //     len = packet_beacon_paload(p, i+1, rfBeaconInfo.RfBeaconType);
            //     mbuf_hdr = packet_beacon_wl(bpts->start,p,len,NwkBeaconPeriodCount)	;
            // }else
            // #endif
            {
                len = packet_beacon_paload(p, i+1, 0);
                mbuf_hdr = packet_beacon(bpts->start,i+1,p,len,NwkBeaconPeriodCount)	;
            }

            zc_free_mem(p);


            if(mbuf_hdr)
            {
                list_add_tail(&mbuf_hdr->link,&bpts->pf_list);

                // #if defined(STD_DUAL)
                // if(rfBeaconInfo.slotType == e_SLOT_TYPE_RF)
                // {
                //     bpts->op = &wl_beacon_send;
                // }else
                // #endif
                {
                    bpts->op = &beacon_send;
                }

                bpts->dir	 = DIR_TX;
                mbuf_hdr = NULL;
            }

        }
        else if(i == 2 && beacon_info->CCObcn_slot_nr > 3)
        {
            i += beacon_info->CCObcn_slot_nr - 3;
            bpts->end    = time + beacon_info->CCObcn_slot_nr*PHY_MS2TICK(beacon_slot);
        }

        #endif

		list_add_tail(&bpts->link,&beacon->bpts_list);


		beacon_slot_info++;

		/*phy_printf("[%-3d][%-3s]\t",i,bpts->type == TS_BEACON?"BCN":bpts->type == TS_CSMA?"CSM":bpts->type == TS_BEACON?"BCM":"UNK");
		phy_printf("%s\t",bpts->phase==PHASE_A?"A":bpts->phase==PHASE_B?"B":bpts->phase==PHASE_C?"C":"U");
		phy_printf("0x%08x\t",bpts->start);
		phy_printf("0x%08x\t",bpts->end);
		phy_printf("%d\n",PHY_TICK2MS(bpts->end-bpts->start));*/
	}
	
	return &beacon->bpts_list;

}
static list_head_t *entry_csma_slot(beacon_t *beacon,
								uint16_t csma_duration_a,
								uint16_t csma_duration_b,
								uint16_t csma_duration_c,
								uint16_t csma_slot,
								uint32_t time,
								uint8_t type,
								uint16_t *slot_all_num,
                                uint8_t coord_num,
                                SLOT_TYPE_e slot_type)
{
	*slot_all_num = 0;
	uint16_t i;
	
	if((!csma_duration_a&&!csma_duration_b&&!csma_duration_c) || !csma_slot)
	{
		//phy_printf("WARNING %s slot is null!\n",type==TS_CSMA?"csma":type==TS_BCSMA?"bcsma":"unk");
		return NULL;
	}
	
	SCALE_CSMA_t  *scale_csma ;
	
	scale_csma = scale_csma_slot(csma_duration_a,
									csma_duration_b,
									csma_duration_c,
									csma_slot,
									slot_all_num,
									time);

	if(!scale_csma) return NULL;
	
	#if defined STATIC_MASTER
	mbuf_hdr_t *mbuf_hdr;
	uint16_t cnt;
    uint16_t SendCnt = 0;
	cnt= 0;
	
	if(coord_num)
        cnt = *slot_all_num/coord_num;

   /* net_printf("<<< slot_all_num = %d, coord_num = %d, cnt = %d\n"
               , *slot_all_num, coord_num, cnt);*/
	
	#endif
	
	for(i=0;i<*slot_all_num;i++)
	{
		bpts_t *bpts = zc_malloc_mem(sizeof(bpts_t),"bpts",MEM_TIME_OUT);
		
	
		bpts->beacon = beacon;
		bpts->start  = scale_csma[i].start_timer;
		bpts->time  = bpts->start;
		bpts->end    = scale_csma[i].end_timer;
		bpts->type   = type;
		bpts->phase  = scale_csma[i].phase;
		bpts->dir    = DIR_TX;
        if(i == 0)
        {
            bpts->rxflag = 1;//(i == 0);
        }
        else
        {
            bpts->rxflag = 0;
        }

        // #if defined(STD_DUAL)
        // if(slot_type == e_SLOT_TYPE_RF)
        // {
        //     bpts->op     = wl_csma_send;
        // }
        // else
        // #endif
        {
            bpts->op     = csma_send;
        }

		bpts->sent   = NULL;
		bpts->life	 = 0x3ff;
		bpts->post	 = beacon->end == bpts->end?1:0;
		/*����Ҫ���͵��������*/
	
		INIT_LIST_HEAD(&bpts->pf_list);
#if defined STATIC_MASTER
        if(cnt>=1 && i%cnt==0 && nnib.NetworkType != e_SINGLE_HRF)      //������������ʽ������������Э��
        {
           if(SendCnt < coord_num)
           {
               //phy_printf("i = %d\n",i);
   #if defined(STD_2016)
               //                HPLC.bso = PHY_TICK2MS(beacon->end - get_phy_tick_time());
               HPLC.bso = PHY_TICK2MS(beacon->end - bpts->start);
   #elif defined(STD_GD)
               //                HPLC.bso = PHY_TICK2MS(beacon->end - get_phy_tick_time()) / 4;
               HPLC.bso = PHY_TICK2MS(beacon->end - bpts->end) / 4;
   #endif
               HPLC.neighbour = getNbForCoord();
            //     #if defined(STD_DUAL)
            //    if(slot_type == e_SLOT_TYPE_RF)
            //    {
            //        mbuf_hdr = packet_coord_wl();
            //    }
            //    else
            //    #endif
               {
                   mbuf_hdr = packet_coord(HPLC.dur,HPLC.bso,HPLC.neighbour,HPLC.bef,HPLC.beo);
               }
               if(mbuf_hdr)
               {
                   list_add_tail(&mbuf_hdr->link,&bpts->pf_list);
               }

               SendCnt++;
           }
        }
#endif
		
		list_add_tail(&bpts->link,&beacon->bpts_list);

		/*phy_printf("[%-3d][%-3s]\t",i,bpts->type == TS_BEACON?"BCN":bpts->type == TS_CSMA?"CSM":bpts->type == TS_BCSMA?"BCM":"UNK");
		phy_printf("%s\t",bpts->phase==PHASE_A?"A":bpts->phase==PHASE_B?"B":bpts->phase==PHASE_C?"C":"U");
		phy_printf("0x%08x\t",bpts->start);
		phy_printf("0x%08x\t",bpts->end);
		phy_printf("%d\n",PHY_TICK2MS(bpts->end-bpts->start));*/
	}
	//zc_free_mem(scale_csma);


	return &beacon->bpts_list;

}

void put_beacon_t(beacon_t *beacon)
{
	list_head_t *pos,*n;
	bpts_t  *bpts;
	if(!beacon)
		return;
	
	list_for_each_safe(pos, n, &beacon->bpts_list) {		
		bpts = list_entry(pos, bpts_t, link);		
		list_del(pos);			
		bpts_pushback_pf(bpts->sent);//just for node 
		bpts->sent = NULL;
		// zc_free_mem(bpts);
        put_bpts(bpts);
	}	
    if(beacon->next)
    {
        zc_free_mem(beacon->next);
    }
	zc_free_mem(beacon);
}
beacon_t *construct_beacon_t(BEACON_PERIOD_t *beacon_info, SLOT_TYPE_e slot_type)
{
	uint16_t csma_slot_nr;
	uint16_t bcsma_slot_nr;
	uint8_t coord_num;
	
	csma_slot_nr = 0;
	bcsma_slot_nr = 0;
	coord_num = 0;

    // char *str[2] = {"CBCT", "RFCBCT"};


	if(!beacon_info)
    {
//        phy_printf("beacon_info ==NULL\n");
		return NULL;
    }
	//���ű�ʱ϶����csmaʱ϶��Ϊ�ǷǷ�����
	/*if(!beacon_info->beacon_slot||
		!beacon_info->beacon_slot_nr||
		!beacon_info->csma_slot||
		(!beacon_info->csma_duration_a&&!beacon_info->csma_duration_b&&!beacon_info->csma_duration_c))
		return NULL;*/
	
	
        beacon_t *beacon = zc_malloc_mem(sizeof(beacon_t),"CBCT",MEM_TIME_OUT);

		//beacon->next = NULL;

        //�Ƿ�ֻ�����ڵ��Զ��滮ʱ϶��
//        #if defined STATIC_MASTER
//        if(slot_type == e_SLOT_TYPE_HPLC)
        beacon->next = put_next_beacon();
//        #endif

		beacon->start=beacon_info->beacon_start_time; 
		beacon->length = beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot)+
			PHY_MS2TICK(beacon_info->csma_duration_a+beacon_info->csma_duration_b+beacon_info->csma_duration_c)+
			PHY_MS2TICK(beacon_info->bcsma_duration_a+beacon_info->bcsma_duration_b+beacon_info->bcsma_duration_c); 
		beacon->end=beacon->start+beacon->length; 
		/*beacon->rx;
		beacon->bts; 
		beacon->bpc; 
		beacon-> 
		beacon->phase   :2; 
		beacon->bt      :3; 
		beacon->me_ts   :1; 
		beacon->stei; 
		beacon->networking; 
		beacon->coord; */
		beacon->ext_bpts = beacon_info->ext_bpts;
		
	INIT_LIST_HEAD(&beacon->bpts_list);


    RF_BEACON_INFO_t RfBcnInfo;
    RfBcnInfo.slotType = slot_type;
    RfBcnInfo.RfBeaconSlotCnt = beacon_info->RfBeaconSlotCnt;
    RfBcnInfo.RfBeaconType = beacon_info->RfBeaconType;
	
	entry_beacon_slot(beacon,
                        beacon_info, RfBcnInfo);
	
#if defined STATIC_MASTER
    //�͹���ģʽ������coord������2Ϊ�ű����ڳ���Ϊ10�������
    #if defined(POWER_MANAGER)
    if((TRUE == LWPR_INFO.LowPowerStates))
    {
        coord_num = 2;
    }
    else
    #endif
    {
        coord_num = PHY_TICK2MS(beacon->length)/1000+(PHY_TICK2MS(beacon->length)%1000?1:0);
    }

#if defined(STD_2016)
                HPLC.dur = beacon_info->beacon_slot * beacon_info->beacon_slot_nr;
#elif defined(STD_GD)
                if((beacon_info->beacon_slot * beacon_info->beacon_slot_nr) / 40 != 0)
                {
                    HPLC.dur = 1;
                }
                HPLC.dur += (beacon_info->beacon_slot * beacon_info->beacon_slot_nr)/40;
#endif

#endif
	
	entry_csma_slot(beacon,
						beacon_info->csma_duration_a,
						beacon_info->csma_duration_b,
						beacon_info->csma_duration_c,
						beacon_info->csma_slot,
						beacon_info->beacon_start_time+beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot),
						TS_CSMA,
						&csma_slot_nr,
                        coord_num,
                        slot_type);

	entry_csma_slot(beacon,
						beacon_info->bcsma_duration_a,
						beacon_info->bcsma_duration_b,
						beacon_info->bcsma_duration_c,
						beacon_info->bcsma_slot,
						beacon_info->beacon_start_time+beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot)+PHY_MS2TICK(beacon_info->csma_duration_a+beacon_info->csma_duration_b+beacon_info->csma_duration_c),
						TS_BCSMA,
						&bcsma_slot_nr,
                        0,
                        slot_type);
	
	beacon->curr_pos = beacon->bpts_list.next;

	/*printf_s("beacon_s->bpts_list : 0x%08x\n",&beacon_s->bpts_list);
	printf_s("beacon_s->bpts_list.next : 0x%08x\n",beacon_s->bpts_list.next);
	printf_s("beacon_s->bpts_list.prev : 0x%08x\n",beacon_s->bpts_list.prev);*/


	beacon->nr_ts[TS_BEACON] = beacon_info->beacon_slot_nr; 
	beacon->nr_ts[TS_TDMA]   = 0; 
	beacon->nr_ts[TS_CSMA]   = csma_slot_nr; 
	beacon->nr_ts[TS_BCSMA]  = bcsma_slot_nr; 
	
	beacon->nr_bpts = beacon->nr_ts[TS_BEACON]+beacon->nr_ts[TS_TDMA]+beacon->nr_ts[TS_CSMA]+beacon->nr_ts[TS_BCSMA]; 

    beacon->put = &put_beacon_t;

	return beacon;

}


#if defined(STD_DUAL)
ostimer_t  *InsertCoordTimer = NULL;

/**
 * @brief rf_entry_beacon_slot      �����ű�ʱ϶�滮ʵ��
 * @param beacon                    ʱ϶����ͷ
 * @param time                      �ű�ʱ϶��ʼʱ��
 * @param beacon_slot               �ű�ʱ϶Ƭ��С
 * @param beacon_slot_nr            �ű�ʱ϶Ƭ����
 * @param sent_nr                   �����ű�ʱ϶Ƭ���
 * @param rfBcnType                 ���ڵ���Ҫ���͵������ű����͡����㷢�������ű���Ҫռ�õ�tdmaʱ϶Ƭ����
 * @return
 */
static list_head_t *rf_entry_beacon_slot(beacon_t *beacon,
                                    uint32_t time,
                                    uint16_t beacon_slot,
                                    uint16_t beacon_slot_nr,
                                    uint16_t sent_nr,
                                    U8 rfBcnType,
                                    U8 RfBcnSlotCnt)
{
    uint16_t i;

    if(rfBcnType == e_BEACON_HPLC || rfBcnType == e_BEACON_HPLC_RF_SIMPLE_CSMA)
    {
        sent_nr = 0;        //û�а��������ű귢�Ͳ��滮�����ű�ʱ϶
    }


    #if defined(STATIC_MASTER)
    U8 rfsimpBcnFlag = e_STRAD_BEACON;

//    net_printf("rfBcnType : %d, sent_nr : %d, beacon_slot_nr:%d\n", rfBcnType, sent_nr, beacon_slot_nr);

    if(rfBcnType == e_BEACON_HPLC_RF_SIMPLE_CSMA || rfBcnType == e_BEACON_HPLC_RF_SIMPLE)
    {
        rfsimpBcnFlag = e_SIMP_BEACON;
    }
    else
    {
        rfsimpBcnFlag = e_STRAD_BEACON;
    }
    #endif

#if defined(STATIC_NODE)
//    U8 rfBcnSlot_nr = 0;
    if(rfBcnType == e_BEACON_RF)            //ֻ����ת�������ű�ʱ�������ز��ű�ʱ϶��
    {
        sent_nr -= 1;
    }
    else if(e_BEACON_HPLC_RF == rfBcnType || rfBcnType == e_BEACON_HPLC_RF_SIMPLE)      //̨�尲���ĸ��ű�ʱ϶��������Ҫ�����ز��ű�ʱ϶��
    {
        if(sent_nr == beacon_slot_nr && sent_nr >= 4)
        {
            sent_nr -= 1;
        }
    }
#endif


    if(!beacon_slot || !beacon_slot_nr)
    {
        return NULL;
    }

    for(i=0;i<beacon_slot_nr;i++)
    {
        bpts_t *bpts = zc_malloc_mem(sizeof(bpts_t),"rfbpts",MEM_TIME_OUT);


        bpts->beacon = beacon;
        bpts->start  = time + i*PHY_MS2TICK(beacon_slot);
        bpts->time  = bpts->start;
        bpts->end    = time + (i+1)*PHY_MS2TICK(beacon_slot);
        bpts->type   = TS_BEACON;
        bpts->dir	 = DIR_RX;
        bpts->stei   = HPLC.tei;//beacon_slot_info->stei;
        bpts->sent   = NULL;
        bpts->life   = 0x3ff;
        bpts->post   = 0;
        INIT_LIST_HEAD(&bpts->pf_list);
        bpts->op = &wl_tdma_recv;
        #if defined(STATIC_NODE)
        if(sent_nr == 0) //δ����ʱ϶ʱ
        {
            i=beacon_slot_nr;
            bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
        }
        else if(sent_nr==i+1) //��ʱ϶ǰ��һ��ʱ϶
        {
            bpts->start  = time ;
            bpts->time  = bpts->start;
        }
        else if(sent_nr==i) //��ǰΪ�����ŵ�ʱ϶
        {

            bpts->op = &wl_rbeacon_send;
            bpts->dir	 = DIR_TX;
            // bpts->time += PHY_US2TICK(200);
            if((i + RfBcnSlotCnt) >= beacon_slot_nr)
            {
                bpts->end = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
//                break;
            }
            else
            {
                bpts->end = bpts->start + RfBcnSlotCnt*PHY_MS2TICK(beacon_slot);
                i += RfBcnSlotCnt - 1;
            }


        }
        else if(i == (sent_nr + RfBcnSlotCnt))         //��ǰʱ϶��һ��ʱ϶
        {
            i = beacon_slot_nr;             //��ǰ����ѭ��
            bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
        }
        else
        {
            zc_free_mem(bpts);
            continue;
        }
        #else   //CCO �����ű�ʱ϶ ��ͷ��һ���ز��ű�ʱ϶��ʼ����
        if(rfBcnType == e_BEACON_HPLC)          //δ���ŷ��������ű�
        {
            i=beacon_slot_nr;
            bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
        }
        else if(sent_nr == 0) //δ����ʱ϶ʱ
        {
            i=beacon_slot_nr;
            bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
        }
        else if(i == 0)
        {

            if((i + RfBcnSlotCnt) >= beacon_slot_nr)
            {
                bpts->end = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
//                break;
            }
            else
            {
                bpts->end = bpts->start + RfBcnSlotCnt*PHY_MS2TICK(beacon_slot);
                i += RfBcnSlotCnt - 1;
            }


            bpts->op = &wl_beacon_send;
            bpts->dir	 = DIR_TX;
            extern U32 NwkBeaconPeriodCount;
            U8 *p;
            mbuf_hdr_t *mbuf_hdr;
            U16 len;
            p = zc_malloc_mem(520,"becn",MEM_TIME_OUT);

            len = packet_beacon_paload(p, 0, rfsimpBcnFlag);

            mbuf_hdr = packet_beacon_wl(bpts->start,p,len,NwkBeaconPeriodCount)	;
            zc_free_mem(p);


            if(mbuf_hdr)
            {
                list_add_tail(&mbuf_hdr->link,&bpts->pf_list);
            }

        }
        else if(i == RfBcnSlotCnt)         //��ǰʱ϶��һ��ʱ϶
        {
            i = beacon_slot_nr;             //��ǰ����ѭ��
            bpts->end    = time + beacon_slot_nr*PHY_MS2TICK(beacon_slot);
        }
        else
        {
            zc_free_mem(bpts);
            continue;
        }

        #endif


        list_add_tail(&bpts->link,&beacon->bpts_list);

        /*phy_printf("[%-3d][%-3s]\t",i,bpts->type == TS_BEACON?"BCN":bpts->type == TS_CSMA?"CSM":bpts->type == TS_BEACON?"BCM":"UNK");
        phy_printf("%s\t",bpts->phase==PHASE_A?"A":bpts->phase==PHASE_B?"B":bpts->phase==PHASE_C?"C":"U");
        phy_printf("0x%08x\t",bpts->start);
        phy_printf("0x%08x\t",bpts->end);
        phy_printf("%d\n",PHY_TICK2MS(bpts->end-bpts->start));*/
    }

    return &beacon->bpts_list;

}
//#if defined(STATIC_MASTER)
//static void InsertCoordTimerCB(struct ostimer_s *ostimer, void *arg)
//{
//   net_printf("tx coord sendcnt :%d\n",*(U8 *)arg);
//   if(*(U8 *)arg)
//   {
//	   (*(U8 *)arg) --;
//	   entry_tx_coord_data();
//   }
//   else
//   {
//       timer_stop(ostimer, TMR_NULL);
////       timer_delete(ostimer);
//   }
//}
//#endif
/**
 * @brief rf_entry_csma_slot        ����csmaʱ϶����ʵ�֡�
 * @param beacon                    ʱ϶����ͷ
 * @param csma_duration             csma����ʱ��
 * @param time                      csma��ʼʱ��
 * @param type                      ʱ϶���� csma/��csma
 * @return
 */
static list_head_t *rf_entry_csma_slot(beacon_t *beacon,
                                uint16_t csma_duration,
                                uint32_t time,
                                uint8_t type)
{
    if(!csma_duration)
        return NULL;

    bpts_t *bpts = zc_malloc_mem(sizeof(bpts_t),"rfbpts",MEM_TIME_OUT);

    bpts->beacon = beacon;
    bpts->start  = time;
    bpts->time   = bpts->start;
    bpts->end    = time + PHY_MS2TICK(csma_duration);
    bpts->type   = type;
    bpts->dir    = DIR_TX;
    bpts->op     = wl_csma_send;
    bpts->sent   = NULL;
    bpts->life	 = 0x3ff;
    bpts->post	 = beacon->end == bpts->end?1:0;

    bpts->rxflag = 0;

    INIT_LIST_HEAD(&bpts->pf_list);

//	#if defined(STATIC_MASTER)
//	uint32_t first;
//	static uint16_t sendcnt;

//	first =  PHY_TICK2MS(bpts->start - get_phy_tick_time());
//	sendcnt = PHY_TICK2MS(bpts->end - bpts->start) / 1000;
////	printf_s("tx coord first sendcnt :%d\n",sendcnt);
//    if(InsertCoordTimer == NULL)
//    {
//        InsertCoordTimer = timer_create(first,
//                                        1000,
//                                        TMR_PERIODIC,//TMR_TRIGGER
//                                        InsertCoordTimerCB,
//                                        &sendcnt,
//                                        "ICTCB"
//                                        );
//    }
//    timer_start(InsertCoordTimer);
//	#endif
    list_add_tail(&bpts->link,&beacon->bpts_list);

    return &beacon->bpts_list;

}
/**
 * @brief construct_beacon_t    ����ʱ϶�����㷨ʵ�ֺ���
 * @param beacon_info           ʱ϶��Ϣ
 * @return
 */
beacon_t *rf_construct_beacon_t(BEACON_PERIOD_t *beacon_info)
{
    if(!beacon_info)
    {
        return NULL;
    }

    beacon_t *beacon = zc_malloc_mem(sizeof(beacon_t),"RFCBCT",MEM_TIME_OUT);

    beacon->next = NULL;

    //�Ƿ�ֻ�����ڵ��Զ��滮ʱ϶��
    //        #if defined STATIC_MASTER
//    beacon->next = put_next_beacon();
    //        #endif

    beacon->start=beacon_info->beacon_start_time;
    beacon->length = beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot)+
            PHY_MS2TICK(beacon_info->csma_duration_a+beacon_info->csma_duration_b+beacon_info->csma_duration_c)+
            PHY_MS2TICK(beacon_info->bcsma_duration_a+beacon_info->bcsma_duration_b+beacon_info->bcsma_duration_c);
    beacon->end=beacon->start+beacon->length;
    beacon->ext_bpts = beacon_info->ext_bpts;

    INIT_LIST_HEAD(&beacon->bpts_list);



    rf_entry_beacon_slot(beacon,
                      beacon_info->beacon_start_time,
                      beacon_info->beacon_slot,
                      beacon_info->beacon_slot_nr,
                      beacon_info->beacon_sent_nr + 1,
                      beacon_info->RfBeaconType,
                      beacon_info->RfBeaconSlotCnt);     //�����ű���ز��ű��λ����

//#if defined STATIC_MASTER
//    coord_num = PHY_TICK2MS(beacon->length)/1000+(PHY_TICK2MS(beacon->length)%1000?1:0);
//#if defined(STD_2016)
//    HPLC.dur = beacon_info->beacon_slot * beacon_info->beacon_slot_nr;
//#elif defined(STD_GD)
//    if((beacon_info->beacon_slot * beacon_info->beacon_slot_nr) / 40 != 0)
//    {
//        HPLC.dur = 1;
//    }
//    HPLC.dur += (beacon_info->beacon_slot * beacon_info->beacon_slot_nr)/40;
//#endif

//#endif

    rf_entry_csma_slot(beacon,
                    beacon_info->csma_duration_a + beacon_info->csma_duration_b + beacon_info->csma_duration_c,
                    beacon_info->beacon_start_time+beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot),
                    TS_CSMA);

    rf_entry_csma_slot(beacon,
                    beacon_info->bcsma_duration_a + beacon_info->bcsma_duration_b + beacon_info->bcsma_duration_c,
                    beacon_info->beacon_start_time+beacon_info->beacon_slot_nr*PHY_MS2TICK(beacon_info->beacon_slot)+PHY_MS2TICK(beacon_info->csma_duration_a+beacon_info->csma_duration_b+beacon_info->csma_duration_c),
                    TS_BCSMA);

    beacon->curr_pos = beacon->bpts_list.next;

    /*printf_s("beacon_s->bpts_list : 0x%08x\n",&beacon_s->bpts_list);
    printf_s("beacon_s->bpts_list.next : 0x%08x\n",beacon_s->bpts_list.next);
    printf_s("beacon_s->bpts_list.prev : 0x%08x\n",beacon_s->bpts_list.prev);*/


    beacon->nr_ts[TS_BEACON] = beacon_info->beacon_slot_nr;
    beacon->nr_ts[TS_TDMA]   = 0;
    beacon->nr_ts[TS_CSMA]   = 1;
    beacon->nr_ts[TS_BCSMA]  = 1;

    beacon->nr_bpts = beacon->nr_ts[TS_BEACON]+beacon->nr_ts[TS_TDMA]+beacon->nr_ts[TS_CSMA]+beacon->nr_ts[TS_BCSMA];

    beacon->put = &put_beacon_t;

    return beacon;

}

#endif


void register_beacon_slot(BEACON_PERIOD_t *beacon_info)
{
    beacon_t *beacon;

//    if(getHplcTestMode() == MONITOR)    //͸��ģ�鸴λ
//    {
//        return;
//    }

    beacon = construct_beacon_t(beacon_info, e_SLOT_TYPE_HPLC);
	if(!beacon)
	{
        printf_s("ERROR beacon slot pma error!\n");
		return;
    }

//    phy_beacon_show(&beacon->bpts_list, g_vsh.term);

    //��¼��һ���ű�Ľ���ʱ�䣬���û�н��յ�����Э������Ҫʹ����һ��ʱ϶�Ľ���ʱ����Ϊ��һ��ʱ϶�Ŀ�ʼʱ��
#if defined(STATIC_MASTER)
    HPLC.next_beacon_start = beacon->end;
#endif
    hplc_push_beacon(beacon);


#if defined(STD_DUAL)
    beacon_t *rf_beacon;
//    rf_beacon = construct_beacon_t(beacon_info, e_SLOT_TYPE_RF);
    rf_beacon = rf_construct_beacon_t(beacon_info);

    if(!rf_beacon)
    {
        printf_s("ERROR rf_beacon slot pma error!\n");
        return;
    }
//    g_vsh.term->op->tprintf(g_vsh.term, "-----------------------------------------rf---------------\n");
//    phy_beacon_show(&rf_beacon->bpts_list, g_vsh.term);
    hplc_push_beacon_wl(rf_beacon);

#endif

}



void phy_beacon_show(list_head_t *link, tty_t *term)
{
	list_head_t *pos;
	bpts_t *bpts;
    uint16_t i = 0;

    uint32_t cpu_sr;

	if(!link)
        return;
    term->op->tprintf(term, "seq\ttype\tphase\tstart\t\tend\t\tperiod\top  nowtime:%08x\n", get_phy_tick_time());

    cpu_sr = OS_ENTER_CRITICAL();

	list_for_each(pos, link) {
		bpts = list_entry(pos, bpts_t, link);
        term->op->tprintf(term, "[%-3d]\t[%-3s]\t",i++,bpts->type == TS_BEACON?"BCN":bpts->type == TS_CSMA?"CSM":bpts->type == TS_BCSMA?"BCM":"UNK");
        term->op->tprintf(term, "%s\t",bpts->phase==PHASE_A?"A":bpts->phase==PHASE_B?"B":bpts->phase==PHASE_C?"C":"U");
        term->op->tprintf(term, "0x%08x\t",bpts->start);
        term->op->tprintf(term, "0x%08x\t",bpts->end);
        term->op->tprintf(term, "%d\t",PHY_TICK2MS(bpts->end-bpts->start));
        term->op->tprintf(term, "%s\n",bpts->op == csma_send ? "csma_send" :
                                       bpts->op == tdma_recv ? "tdma_recv" :
                                       bpts->op == rbeacon_send ? "rbeacon_send" :
                                       bpts->op == wl_csma_send ? "wl_csma_send" :
                                       bpts->op == wl_tdma_recv ? "wl_tdma_recv" :
                                       bpts->op == wl_rbeacon_send ? "wl_rbeacon_send" :
                                       #if defined(STATIC_MASTER)
                                       bpts->op == beacon_send ? "beacon_send" :
                                       bpts->op == wl_beacon_send ? "wl_beacon_send" :
                                       #endif
                                                                    "UKW");

//		os_sleep(1);
	}
    OS_EXIT_CRITICAL(cpu_sr);

}











 

