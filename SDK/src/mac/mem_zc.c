/*
 * Copyright: (c) 2016-2020, 2016 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        mem_zc.c
 * Purpose:     demo procedure for using libphy.a
 * History:
 */
#include "mem_zc.h"
#include "trios.h"
#include "sys.h"
#include "list.h"
#include "plc.h"
#include <string.h>
#include <vsh.h>
#include "printf_zc.h"


/*************************mem pool uint****************************/
void mem_pool_init(void)
{
	create_pool(&MEM_A_Pool_t.Po_t,
				MEM_A_Pool_t.MEM_A_t,
				MEM_NR_A,
				sizeof(MEMA_t),
				offset_of(MEMA_t, link),
				"A");
	create_pool(&MEM_B_Pool_t.Po_t,
				MEM_B_Pool_t.MEM_B_t,
				MEM_NR_B,
				sizeof(MEMB_t),
				offset_of(MEMB_t, link),
				"B");
	create_pool(&MEM_C_Pool_t.Po_t,
				MEM_C_Pool_t.MEM_C_t,
				MEM_NR_C,
				sizeof(MEMC_t),
				offset_of(MEMC_t, link),
				"C");
	create_pool(&MEM_D_Pool_t.Po_t,
				MEM_D_Pool_t.MEM_D_t,
				MEM_NR_D,
				sizeof(MEMD_t),
				offset_of(MEMD_t, link),
				"D");
	create_pool(&MEM_E_Pool_t.Po_t,
				MEM_E_Pool_t.MEM_E_t,
				MEM_NR_E,
				sizeof(MEME_t),
				offset_of(MEME_t, link),
				"E");
	create_pool(&MEM_F_Pool_t.Po_t,
				MEM_F_Pool_t.MEM_F_t,
				MEM_NR_F,
				sizeof(MEMF_t),
				offset_of(MEMF_t, link),
				"F");
	create_pool(&MEM_G_Pool_t.Po_t,
				MEM_G_Pool_t.MEM_G_t,
				MEM_NR_G,
				sizeof(MEMG_t),
				offset_of(MEMG_t, link),
				"G");
	create_pool(&MEM_H_Pool_t.Po_t,
				MEM_H_Pool_t.MEM_H_t,
				MEM_NR_H,
				sizeof(MEMH_t),
				offset_of(MEMH_t, link),
				"H");

	INIT_LIST_HEAD(&MEM_RECORD_LIST);	
    
}
/*************************mem pool uint****************************/


static __inline__ void record_mem_list(mem_record_list_t record , mem_record_list_t *p)
{
	__memcpy(p,(uint8_t *)&record,sizeof(mem_record_list_t));
	list_add_tail(&p->link, &MEM_RECORD_LIST);
#if MEM_PMA
	if(p->type == MEM_A)
	{	
		HPLC.a_num++;
		if(HPLC.a_num>HPLC.a_max_num)
			HPLC.a_max_num=HPLC.a_num;
	}
	else if(p->type == MEM_B)
	{	
		HPLC.b_num++;
		if(HPLC.b_num>HPLC.b_max_num)
			HPLC.b_max_num=HPLC.b_num;
	}
	else if(p->type == MEM_C)
	{
		HPLC.c_num++;
		if(HPLC.c_num>HPLC.c_max_num)
			HPLC.c_max_num=HPLC.c_num;
	}
	else if(p->type == MEM_D)
	{
		HPLC.d_num++;
		if(HPLC.d_num>HPLC.d_max_num)
			HPLC.d_max_num=HPLC.d_num;
	}
	else if(p->type == MEM_E)
	{
		HPLC.e_num++;
		if(HPLC.e_num>HPLC.e_max_num)
			HPLC.e_max_num=HPLC.e_num;
	}
	else if(p->type == MEM_F)
	{
		HPLC.f_num++;
		if(HPLC.f_num>HPLC.f_max_num)
			HPLC.f_max_num=HPLC.f_num;
	}
	else if(p->type == MEM_G)
	{
		HPLC.g_num++;
		if(HPLC.g_num>HPLC.g_max_num)
			HPLC.g_max_num=HPLC.g_num;
	}
	else if(p->type == MEM_H)
	{
		HPLC.h_num++;
		if(HPLC.h_num>HPLC.h_max_num)
			HPLC.h_max_num=HPLC.h_num;
	}

#endif
	//printf_s("malc type :%d p : 0x%08x prev : 0x%08x next : 0x%08x p->addr : 0x%08x\n",p->type,p,p->link.prev,p->link.next,p->addr);
}
static __inline__ uint8_t delete_mem_list(mem_record_list_t *p)
{
	if(p->addr!=NULL && p->addr==(uint8_t*)p)
	{
	    /*list_head_t *next,*prev;
        next = p->link.next;
        prev = p->link.prev;
	    if((void*)p == next->prev && (void*)p == prev->next)//(list_empty_one(&p->link))//
	    */
        {   
    		list_del(&p->link);	
            p->addr = NULL;
#if MEM_PMA

			if(p->type == MEM_A)HPLC.a_num--;
			else if(p->type == MEM_B)HPLC.b_num--;
			else if(p->type == MEM_C)HPLC.c_num--;
			else if(p->type == MEM_D)HPLC.d_num--;
			else if(p->type == MEM_E)HPLC.e_num--;
			else if(p->type == MEM_F)HPLC.f_num--;
			else if(p->type == MEM_G)HPLC.g_num--;
			else if(p->type == MEM_H)HPLC.h_num--;

#endif
			//printf_s("free type :%d p : 0x%08x p->addr : 0x%08x\n",p->type,p,p->addr);
			return TRUE;
	        }
        }

	printf_s("ERROR p : 0x%08x != p->addr : 0x%08x\n",p,p->addr);

	
	return FALSE;
}



/*************************check mem name num****************************/

U8 check_mem_name_num(mem_record_list_t *mem_list, MEM_NAME_TYPE_t *mem_name_types)
{
    U16 ii;
    U16  null_seq;
    MEM_NAME_TYPE_t *p_mem_name_types = NULL;
    
    null_seq = 0xFFFF;
    for(ii = 0;ii < MEM_USED_MAX;ii ++)
    {
        if(0 == __strlen(mem_list->s))
        {
            return ERROR;
        }
        p_mem_name_types = &mem_name_types[ii];
        if(0xFFFF == null_seq && 0 == p_mem_name_types->all_num)
        {
            null_seq = ii;
        }
        if(0 == __strncmp(mem_list->s,p_mem_name_types->s,__strlen(mem_list->s)))
        {
            p_mem_name_types->all_num ++;
            p_mem_name_types->type_num[mem_list->type] ++;
            return OK;
        }
    }
    if(MEM_USED_MAX == ii && 0xFFFF != null_seq)
    {
        p_mem_name_types = &mem_name_types[null_seq];
        //printf_s("__strlen%d \n",__strlen(mem_list->s));
        __strncpy((char *)&p_mem_name_types->s, mem_list->s, __strlen(mem_list->s));
        p_mem_name_types->all_num ++;
        p_mem_name_types->type_num[mem_list->type] ++;
        return OK;
    }
    
    return ERROR;
}
U8    MEM_INFO[MEM_INFO_LEN*MEM_USED_MAX];
void show_record_mem_list(xsh_t *xsh,uint32_t tick)
{
    tty_t *term = NULL;
	list_head_t *pos;
    mem_record_list_t *mem_list;
    uint32_t cpu_sr;
	uint32_t i  = 0 ;
	uint32_t i_A  = 0 ;
	uint32_t i_B  = 0 ;
	uint32_t i_C  = 0 ;
	uint32_t i_D  = 0 ;
	uint32_t i_E  = 0 ;
	uint32_t i_F  = 0 ;
	uint32_t i_G  = 0 ;
	uint32_t i_H  = 0 ;	
        term = xsh->term;
        term->op->tputs(term, "seq\t addr\t  type\t time\t name \t\t runTiem \t runState \t\t\t\n");
    MEM_NAME_TYPE_t *p_mem_name_info = (MEM_NAME_TYPE_t *)MEM_INFO;//zc_malloc_mem((sizeof(MEM_NAME_TYPE_t)*MEM_USED_MAX), "MEMTY", MEM_TIME_OUT);
    
    cpu_sr = OS_ENTER_CRITICAL();
	
		list_for_each(pos, &MEM_RECORD_LIST) 
		{
			char *s="UK";
			mem_list = list_entry(pos, mem_record_list_t, link);
            if(p_mem_name_info)
            {
                check_mem_name_num(mem_list,p_mem_name_info);
            }
			if(mem_list->type==MEM_A)
			{
				s="A";
				i_A++;
                continue;
			}
			else if(mem_list->type==MEM_B)
			{
				s="B";
				i_B++;
                continue;
			}
			else if(mem_list->type==MEM_C)
			{	
				s="C";
				i_C++;
                continue;
			}
			else if(mem_list->type==MEM_D)
			{
				s="D";
				i_D++;
			}
			else if(mem_list->type==MEM_E)
			{
				s="E";
				i_E++;
			}
			else if(mem_list->type==MEM_F)
			{			
				s="F";
				i_F++;
			}
			else if(mem_list->type==MEM_G)
			{
				s="G";
				i_G++;
			}
			else if(mem_list->type==MEM_H)
			{			
				s="H";
				i_H++;
			}

			term->op->tprintf(term, "%d\t %-08x %s\t %-ld\t %s\t\t %d \t %d \t\t\n",
						++i,
						mem_list->addr,
						s,
						mem_list->time,mem_list->s,PHY_TICK2US(tick-mem_list->tick), mem_list->runState );

		}
		
		
		
		
	

    OS_EXIT_CRITICAL(cpu_sr);
    term->op->tprintf(term, "A- %5dBytes: %d/%d\nB- %5dBytes: %d/%d\nC- %5dBytes: %d/%d\nD- %5dBytes: %d/%d\nE- %5dBytes: %d/%d\nF- %5dBytes: %d/%d\nG- %5dBytes: %d/%d\nH- %5dBytes: %d/%d\n",
                                MEM_SIZE_A,i_A,MEM_NR_A,MEM_SIZE_B,i_B,MEM_NR_B,MEM_SIZE_C,i_C,MEM_NR_C,MEM_SIZE_D,i_D,MEM_NR_D,MEM_SIZE_E,i_E,MEM_NR_E,MEM_SIZE_F,i_F,MEM_NR_F,MEM_SIZE_G,i_G,MEM_NR_G,MEM_SIZE_H,i_H,MEM_NR_H);
    if(p_mem_name_info)
    {
        term->op->tprintf(term, "idx name\t\t all A   B   C   D   E   F   G   H   \n");
        for(i = 0;i < MEM_USED_MAX;i ++)
        {
            if(0 == __strlen(p_mem_name_info[i].s))
            {
                continue;
            }
            term->op->tprintf(term, "%-3d %s\t\t %-3d %-3d %-3d %-3d %-3d %-3d %-3d %-3d %-3d \n",
    						i,
    						p_mem_name_info[i].s,
    						p_mem_name_info[i].all_num,
    						p_mem_name_info[i].type_num[MEM_A],
    						p_mem_name_info[i].type_num[MEM_B],
    						p_mem_name_info[i].type_num[MEM_C],
    						p_mem_name_info[i].type_num[MEM_D],
    						p_mem_name_info[i].type_num[MEM_E],
    						p_mem_name_info[i].type_num[MEM_F],
    						p_mem_name_info[i].type_num[MEM_G],
    						p_mem_name_info[i].type_num[MEM_H]
    						);
        }
    //zc_free_mem(p_mem_name_info);
    }

    #if MEM_PMA
        term->op->tputs(term, "seq\t mins\t maxs\t nnum\t mnum\t\n");
        term->op->tprintf(term,"A %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_A,HPLC.size_a_min,HPLC.size_a_max,HPLC.a_num,HPLC.a_max_num);
        term->op->tprintf(term,"B %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_B,HPLC.size_b_min,HPLC.size_b_max,HPLC.b_num,HPLC.b_max_num);
        term->op->tprintf(term,"C %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_C,HPLC.size_c_min,HPLC.size_c_max,HPLC.c_num,HPLC.c_max_num);
        term->op->tprintf(term,"D %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_D,HPLC.size_d_min,HPLC.size_d_max,HPLC.d_num,HPLC.d_max_num);
        term->op->tprintf(term,"E %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_E,HPLC.size_e_min,HPLC.size_e_max,HPLC.e_num,HPLC.e_max_num);
        term->op->tprintf(term,"F %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_F,HPLC.size_f_min,HPLC.size_f_max,HPLC.f_num,HPLC.f_max_num);
		term->op->tprintf(term,"G %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_G,HPLC.size_g_min,HPLC.size_g_max,HPLC.g_num,HPLC.g_max_num);
        term->op->tprintf(term,"H %-5d\t %-5d\t %-3d\t %-3d\t %-3d\t\n",MEM_SIZE_H,HPLC.size_h_min,HPLC.size_h_max,HPLC.h_num,HPLC.h_max_num);
    #endif
}
void *zc_malloc_mem(uint16_t size,char *s,uint16_t time)
{
	int32_t cpu_sr;
	void *p;
	uint16_t orig_size,per_size,mem_num;
	void *pool,*pool_start;
	int32_t off;
	mem_record_list_t record;
    static uint32_t err_cnt = 0;
    static uint32_t last_tick = 0;

	p=NULL;
	orig_size=0;
	record.s = s;
	record.time = time == 0 ? 0xffff:time ;
	record.tick = get_phy_tick_time(); //debug use
	
	size += sizeof(mem_record_list_t);

	//if(size == 13520)
		//printf_s("mem %d\t%s\t\n",size,s);
#if MEM_PMA

	if(size <= MEM_SIZE_A)
	{
		if(time_after(size,HPLC.size_a_max))
		{
			HPLC.size_a_max = size;
		}
		if(time_after(HPLC.size_a_min,size))
		{
			HPLC.size_a_min = size;
		}
	}
	else if(size <= MEM_SIZE_B)
	{
		if(time_after(size,HPLC.size_b_max))
		{
			HPLC.size_b_max = size;
		}
		if(time_after(HPLC.size_b_min,size))
		{
			HPLC.size_b_min = size;
		}
	}
	else if(size <= MEM_SIZE_C)
	{
		if(time_after(size,HPLC.size_c_max))
		{
			HPLC.size_c_max = size;
		}
		if(time_after(HPLC.size_c_min,size))
		{
			HPLC.size_c_min = size;
		}
	}
	else if(size <= MEM_SIZE_D)
	{
		if(time_after(size,HPLC.size_d_max))
		{
			HPLC.size_d_max = size;
		}
		if(time_after(HPLC.size_d_min,size))
		{
			HPLC.size_d_min = size;
		}
	}
	else if(size <= MEM_SIZE_E)
	{
		if(time_after(size,HPLC.size_e_max))
		{
			HPLC.size_e_max = size;
		}
		if(time_after(HPLC.size_e_min,size))
		{
			HPLC.size_e_min = size;
		}
	}
	else if(size <= MEM_SIZE_F)
	{
		if(time_after(size,HPLC.size_f_max))
		{
			HPLC.size_f_max = size;
		}
		if(time_after(HPLC.size_f_min,size))
		{
			HPLC.size_f_min = size;
		}
	}
	else if(size <= MEM_SIZE_G)
	{
		if(time_after(size,HPLC.size_g_max))
		{
			HPLC.size_g_max = size;
		}
		if(time_after(HPLC.size_g_min,size))
		{
			HPLC.size_g_min = size;
		}
	}
	else if(size <= MEM_SIZE_H)
	{
		if(time_after(size,HPLC.size_h_max))
		{
			HPLC.size_h_max = size;
		}
		if(time_after(HPLC.size_h_min,size))
		{
			HPLC.size_h_min = size;
		}
	}


#endif
	do{
		orig_size = size;
		if(size <= MEM_SIZE_A)
		{
			pool = &MEM_A_Pool_t.Po_t;
            pool_start = &MEM_A_Pool_t.MEM_A_t[0];
			off  = offset_of(MEMA_t, link);	
			record.type = MEM_A;
			size = MEM_SIZE_B;
            per_size = sizeof(MEMA_t);
            mem_num = MEM_NR_A;
		}
		else if(size > MEM_SIZE_A&&size <= MEM_SIZE_B)
		{
			pool = &MEM_B_Pool_t.Po_t;
            pool_start = &MEM_B_Pool_t.MEM_B_t[0];
			off  = offset_of(MEMB_t, link);
			record.type = MEM_B;	
			size = MEM_SIZE_C;
            per_size = sizeof(MEMB_t);
            mem_num = MEM_NR_B;
		}
		else if(size > MEM_SIZE_B&&size <= MEM_SIZE_C)
		{
			pool = &MEM_C_Pool_t.Po_t;
            pool_start = &MEM_C_Pool_t.MEM_C_t[0];
			off  = offset_of(MEMC_t, link);
			record.type = MEM_C;
			size = MEM_SIZE_D;
            per_size = sizeof(MEMC_t);
            mem_num = MEM_NR_C;
		}
		else if(size > MEM_SIZE_C&&size <= MEM_SIZE_D)
		{		
			pool = &MEM_D_Pool_t.Po_t;
            pool_start = &MEM_D_Pool_t.MEM_D_t[0];
			off  = offset_of(MEMD_t, link);
			record.type = MEM_D;
			size = MEM_SIZE_E;
            per_size = sizeof(MEMD_t);
            mem_num = MEM_NR_D;
		}
		else if(size > MEM_SIZE_D&&size <= MEM_SIZE_E)
		{
			pool = &MEM_E_Pool_t.Po_t;
            pool_start = &MEM_E_Pool_t.MEM_E_t[0];
			off  = offset_of(MEME_t, link);
			record.type = MEM_E;
			size = MEM_SIZE_F;
            per_size = sizeof(MEME_t);
            mem_num = MEM_NR_E;
		}
		else if(size > MEM_SIZE_E&&size <= MEM_SIZE_F)
		{
			pool = &MEM_F_Pool_t.Po_t;
            pool_start = &MEM_F_Pool_t.MEM_F_t[0];
			off  = offset_of(MEMF_t, link);			
			record.type = MEM_F;
			size = MEM_SIZE_G;
            per_size = sizeof(MEMF_t);
            mem_num = MEM_NR_F;
		}
		else if(size > MEM_SIZE_F&&size <= MEM_SIZE_G)
		{
			pool = &MEM_G_Pool_t.Po_t;
            pool_start = &MEM_G_Pool_t.MEM_G_t[0];
			off  = offset_of(MEMG_t, link);			
			record.type = MEM_G;
			size = MEM_SIZE_H;
            per_size = sizeof(MEMG_t);
            mem_num = MEM_NR_G;
		}
		else if(size > MEM_SIZE_G&&size <= MEM_SIZE_H)
		{
			pool = &MEM_H_Pool_t.Po_t;
            pool_start = &MEM_H_Pool_t.MEM_H_t[0];
			off  = offset_of(MEMH_t, link);			
			record.type = MEM_H;
			size = MEM_SIZE_H+10;
            per_size = sizeof(MEMH_t);
            mem_num = MEM_NR_H;
		}
		else
		{
		    err_cnt ++ ;
            if(err_cnt == 1 || (PHY_TICK2MS(get_phy_tick_time() - last_tick)) > 3000)
            {
                last_tick = get_phy_tick_time();
			    show_record_mem_list(&g_vsh, get_phy_tick_time());
            }
            printf_s("large malloc %s,len %d,err %d\n",s, orig_size,err_cnt);
			return NULL;
		}
		
		cpu_sr = OS_ENTER_CRITICAL();
		p = get_from_pool((pool_t *)pool, off);
		//printf_s("malloc : 0x%08x\n",p+sizeof(mem_record_list_t));
		if(p)
		{
		    if(((p-pool_start)/per_size > mem_num) ||((p-pool_start)%per_size)!=0)
            {
                printf_s("get_from_pool %s (((0x%08x-0x%08x)/0x%08x = (0x%08x) > 0x%08x) ||((0x%08x-0x%08x) = (0x%08x) % 0x%08x)!=0)\n",
                    s,p,pool_start,per_size,(p-pool_start)/per_size , mem_num,p,pool_start,per_size,(p-pool_start)%per_size);
                sys_panic("<get_from_pool> %s: %d\n", __func__, __LINE__);
            }      
			record.addr = p;
			record.runState = FALSE;
			record_mem_list(record,p);			
			OS_EXIT_CRITICAL(cpu_sr);
			memset(p+sizeof(mem_record_list_t),0,orig_size-sizeof(mem_record_list_t));

			return p+sizeof(mem_record_list_t);
		}
		OS_EXIT_CRITICAL(cpu_sr);
		
	}while(p==NULL);

	return NULL;

}

uint8_t zc_free_mem(void *p)
{
	int32_t cpu_sr;
	
	void *pool,*pool_start;;
	int32_t off;
    uint16_t per_size,mem_num;


	if(!(int)p || (int)p%4)
	{
		printf_s("err f:%p\n",p);
		return FALSE;
	}


	cpu_sr = OS_ENTER_CRITICAL();

	mem_record_list_t *addr = data_to_mem(p,0);

	/*printf_s("p : 0x%08x\n",addr);
	printf_s("pre : 0x%08x\n",addr->link.prev);
	printf_s("nex : 0x%08x\n",addr->link.next);
	printf_s("addr : 0x%08x\n",addr->addr);
	printf_s("type : %d\n",addr->type);*/
	
	switch (addr->type)
	{
		case MEM_A:
			pool = &MEM_A_Pool_t.Po_t;
			off  = offset_of(MEMA_t, link); 
            pool_start = &MEM_A_Pool_t.MEM_A_t[0];
            per_size = sizeof(MEMA_t);
            mem_num = MEM_NR_A;
			break;
		case MEM_B:
			pool = &MEM_B_Pool_t;
			off  = offset_of(MEMB_t, link);
            pool_start = &MEM_B_Pool_t.MEM_B_t[0];
            per_size = sizeof(MEMB_t);
            mem_num = MEM_NR_B;
			break;
		case MEM_C:
			pool = &MEM_C_Pool_t;
			off  = offset_of(MEMC_t, link);
            pool_start = &MEM_C_Pool_t.MEM_C_t[0];
            per_size = sizeof(MEMC_t);
            mem_num = MEM_NR_C;
			break;
		case MEM_D:
			pool = &MEM_D_Pool_t;
			off  = offset_of(MEMD_t, link);
            pool_start = &MEM_D_Pool_t.MEM_D_t[0];
            per_size = sizeof(MEMD_t);
            mem_num = MEM_NR_D;
			break;
		case MEM_E:
			pool = &MEM_E_Pool_t;
			off  = offset_of(MEME_t, link);
            pool_start = &MEM_E_Pool_t.MEM_E_t[0];
            per_size = sizeof(MEME_t);
            mem_num = MEM_NR_E;
			break;
		case MEM_F:
			pool = &MEM_F_Pool_t;
			off  = offset_of(MEMF_t, link); 
            pool_start = &MEM_F_Pool_t.MEM_F_t[0];
            per_size = sizeof(MEMF_t);
            mem_num = MEM_NR_F;
			break;
		case MEM_G:
			pool = &MEM_G_Pool_t;
			off  = offset_of(MEMG_t, link); 
            pool_start = &MEM_G_Pool_t.MEM_G_t[0];
            per_size = sizeof(MEMG_t);
            mem_num = MEM_NR_G;
			break;
		case MEM_H:
			pool = &MEM_H_Pool_t;
			off  = offset_of(MEMH_t, link); 
            pool_start = &MEM_H_Pool_t.MEM_H_t[0];
            per_size = sizeof(MEMH_t);
            mem_num = MEM_NR_H;
		break;

		default :
            OS_EXIT_CRITICAL(cpu_sr);
	        printf_s("default fail\n");
			return FALSE;
	}
		
	
	if(TRUE== delete_mem_list(addr))
	{
	    if((((void*)addr-pool_start)/per_size > mem_num) ||(((void*)addr-pool_start)%per_size)!=0)
        {
            printf_s("delete_mem_list %s ((%p-%p)/0x%08x = (0x%08x) > %p) ",addr->s,(void*)addr,pool_start,per_size,((void*)addr-pool_start)/per_size , mem_num);
            printf_s("((%p-%p) = (0x%08x)) % 0x%08x)!=0)\n",(void*)addr,pool_start,per_size,((void*)addr-pool_start)%per_size);
            sys_panic("<get_from_pool> %s: %d\n", __func__, __LINE__);
        }
        list_head_t	*selflink;
        selflink = (void*)addr+(per_size-sizeof(list_head_t));
        if(((void*)selflink->next -pool_start)/per_size > mem_num||((void*)selflink->prev -pool_start)/per_size > mem_num)
        {
            printf_s("selflink %s ((%p-%p)/%p = (0x%08x) > 0x%08x) ",
                 addr->s,selflink->next,pool_start,per_size,((void*)selflink->next-pool_start)/per_size , mem_num);
            printf_s("((%p-%p)/0x%08x = (0x%08x) )\n",
                 selflink->prev,pool_start,per_size,((void*)selflink->prev-pool_start)/per_size);
            sys_panic("<get_from_pool> %s: %d\n", __func__, __LINE__);
        }
		put_into_pool((pool_t *)pool, addr, off);
		OS_EXIT_CRITICAL(cpu_sr);
		//printf_s("free success\n");
		return TRUE;
	}
    OS_EXIT_CRITICAL(cpu_sr);
	printf_s("free fail\n");
	return FALSE;

}





































