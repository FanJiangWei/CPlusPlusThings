#include "ZCsystem.h"
#include "app_base_serial_cache_queue.h"
#include "datalinkdebug.h"
#include "app.h"
#include "uart.h"
#include "meter.h"


U8	SericalTransState = FALSE;
serial_cache_header_t SERIAL_CACHE_LIST;
ostimer_t *serial_cache_timer = NULL;

static void serial_cache_modify_timer(uint32_t first);
static uint8_t serial_cache_delete_list(add_serialcache_msg_t *msg);
static int8_t serial_cache_list_is_full();
static void serial_cache_timer_cb(struct ostimer_s *ostimer, void *arg);

static int32_t serial_cache_insert_list(serial_cache_header_t *list,add_serialcache_msg_t *new_list)
{
	list_head_t *pos,*node;
	list_head_t *p=NULL;
	add_serialcache_msg_t  *mbuf_n;
	/*over threshold , drop the msg*/
	if(list->nr >= list->thr)
	{
        app_printf("new serial p = %08x fail\n", new_list);
		return -1;
	}
	/*insert list by lid*/
	list_for_each_safe(pos, node,&list->link) 
	{
		mbuf_n = list_entry(pos, add_serialcache_msg_t, link);

		if(new_list->FrameSeq == mbuf_n->FrameSeq && new_list->ack == mbuf_n->ack && new_list->lid == mbuf_n->lid)
		{
            app_printf("repeat,Seq=%04x!\n",mbuf_n->FrameSeq);
			return -2;//上行帧序号添加时不在进行过滤
		}
		if(!p)
		{
		    //mbuf_n->lid >= new_list->lid
			if(time_before_eq(new_list->lid,mbuf_n->lid) || mbuf_n->state == SENDOK)
			{
				p= &mbuf_n->link;
				//goto ok;
			}			
		}

	}
	
	if(!p)
	p=&list->link;
	
//   ok:
//   	printf_s("add serial ok\n");
	list_add_tail(&new_list->link, p);
	++list->nr;
	//printf_s("return\n");
	return 0;
}


//，串口只保证一个最高优先级的任务处于激活状态。
static add_serialcache_msg_t *serial_cache_get_task_state(serial_cache_header_t *list)
{
	list_head_t *pos,*node;
	add_serialcache_msg_t  *mbuf_n,*ptemp;
    U8  readnum = 0;

	if(list->nr)
	{
		mbuf_n = list_entry(list->link.prev, add_serialcache_msg_t, link); //取最高优先级执行
		
		list_for_each_safe(pos, node,&list->link)
		{
			ptemp = list_entry(pos, add_serialcache_msg_t, link);
			if(ptemp->state == SENDOK) //倒计时已被执行的任务
			{
				//if(ptemp->cnt>0)ptemp->cnt--;
				return ptemp;
			}
            readnum ++;
            if(readnum > list->nr || readnum > list->thr || readnum > 100)
            {
                printf_s("GetTaskStatefail,nr=%d,thr=%d,readnum=%d",list->nr,list->thr,readnum);
                return mbuf_n;
            }
		}
		return mbuf_n;
	}
	else
	{
		return NULL;
	}
}

static void serial_cache_send_data_buf(add_serialcache_msg_t *msg)
{
	if(msg->Uartcfg!=NULL)
	{	
		msg->Uartcfg(msg->baud,msg->verif);
	}
	uart_send(msg->FrameUnit, msg->FrameLen);
}

/*描述：串口队列的数据按照此状态机进行启动发送，回码等待，超时控制
pSerialheader   链表头
一次发送一条
执行超时控制
接收时自动匹配回码
*/
static void serial_cache_update_info(work_t *work)
{
	uint8_t state;
	add_serialcache_msg_t *msg,*msgwork;
	msgwork = (add_serialcache_msg_t *)work->buf;
	U32 time = 0;
    
	msg = serial_cache_get_task_state(msgwork->pSerialheader);
    if(msgwork->pSerialheader->nr > 1)
    {
        time = 1;//有后续帧
    }

	if( msg )
	{
		state = msg->state;
	}
	else
	{
	   //if(zc_timer_query(msgwork->CRT_timer) == TMR_RUNNING)
					//timer_stop(msgwork->CRT_timer, TMR_NULL);	
		return; //结束任务
	}
	
	switch( state )
	{
		case UNACTIVE://任务未启动时，启动发送
		{
		    //printf_s("msg->IntervalTime %d.\n",msg->IntervalTime);
		    if(msg->IntervalTime <= 0)
            {   
                if(msg->Protocoltype == APP_T_PROTOTYPE)
                {
                    SericalTransState = TRUE; // 透传标志
                }
    			serial_cache_send_data_buf(msg);
                //printf_s("Serial send finish.\n");
    			if(msg->SendEvent != NULL)
                {     
                    //printf_s("msg->SendEvent %p %d %d\n",msg->SendEvent,msg->FrameSeq,msg->FrameLen);
                    msg->SendEvent();
                }
    			if(msg->ack)
    			{
    			    //printf_s("SENDOK\n");
    				msg->state = SENDOK;
    				//msg->cnt = msg->DeviceTimeout/SERICAL_PER;
    				time = msg->DeviceTimeout;
    			}
    			else
    			{
    			    //printf_s("WATIING\n");
    			    SericalTransState = FALSE; // 透传标志
    			    msg->state = WATIING;
    				//msg->cnt = msg->DeviceTimeout/SERICAL_PER;
    				//time = 100;
    				time = ((msg->FrameLen+20)*100)/(g_meter->baudrate/10)+100;//发完等待后续帧
                    
    				//serial_cache_delete_list(msg);//删除此条目
    			}
            }
            else
            {
                time = msg->IntervalTime;
                msg->IntervalTime = 0;
            }
			break;
		}
        case WATIING://BUSY等待串口发送回应
		{
		    //time = 0;
		    printf_s("WATIING!\n");
			serial_cache_delete_list(msg);//删除此条目
		    break;
        }
		case SENDOK://BUSY等待串口发送回应
		{
			if(msg->ReTransmitCnt>0)
			{
			    msg->ReTransmitCnt--;
			    //msg->cnt = msg->DeviceTimeout/SERICAL_PER;
			    time = msg->DeviceTimeout;
			    msg->state = SENDOK;
                if(msg->Protocoltype == APP_T_PROTOTYPE)
                {
                    SericalTransState = TRUE; // 透传标志
                }
			    serial_cache_send_data_buf(msg);
				if(msg->ReSendEvent != NULL)
				{
			        msg->ReSendEvent();
				}    
			}
			else
			{
				if(msg->Timeout !=NULL)
                {            
				    msg->Timeout((void*)msg);	
                }

                if(msg->Protocoltype == APP_T_PROTOTYPE)
                {
                    SericalTransState = FALSE; // 清除透传标志
                }
				serial_cache_delete_list(msg);//删除此条目
			}

			break;
		}
		default:
			if(msg->MsgErr != NULL)
			{
				msg->MsgErr();
			}

		//删除此条目
			serial_cache_delete_list(msg);        
		break;
	}

    if(msgwork->pSerialheader->nr == 0)
    {
        time = 0;//无后续帧
    }
    serial_cache_modify_timer(time*10);
}

static void serial_cache_add_list_func(work_t *work)
{
    U32  ret;
	add_serialcache_msg_t *msg_info = (add_serialcache_msg_t *)work->buf;

	msg_info->state = UNACTIVE;
	add_serialcache_msg_t *new_list = zc_malloc_mem(sizeof(add_serialcache_msg_t) + msg_info->FrameLen,"ADSE2",MEM_TIME_OUT);
    if(new_list)
    {
        __memcpy((uint8_t *)new_list,work->buf,sizeof(add_serialcache_msg_t)+msg_info->FrameLen);
        if((ret = serial_cache_insert_list(new_list->pSerialheader,new_list))!= 0)
        {
            //new_list->EntryFail(); //entry fail
            //printf_s("add serial fail = %08x\n", new_list);
            if(ret == -1&&new_list->pSerialheader->nr > 0&& TMR_STOPPED==zc_timer_query(new_list->CRT_timer))//you can send rigth now
            {
                serial_cache_modify_timer(10);
                app_printf("CRT_timer fail\n");
            }
            app_printf("ret %d\n",ret);
            zc_free_mem(new_list);
            return;
        }

        if(new_list->pSerialheader->nr == 1)//&& TMR_STOPPED==zc_timer_query(new_list->CRT_timer))//you can send rigth now
        {
            //serial_cache_update_info(work);
            //timer_stop(new_list->CRT_timer,TMR_CALLBACK);
            serial_cache_modify_timer(SERICAL_PER);
            new_list->IntervalTime = 0;
            //serial_cache_timer_cb(new_list->CRT_timer,new_list->pSerialheader);
        }
    }
}

static uint8_t serial_cache_delete_list(add_serialcache_msg_t *msg)
{
	if(msg->pSerialheader->nr >0)
    {
        list_del(&msg->link);	
        msg->pSerialheader->nr--;
        zc_free_mem(msg);
        return TRUE;
    }

	return FALSE;
}

static add_serialcache_msg_t *serial_cache_find_msg(recive_msg_t *msg_info)
{
	list_head_t *pos,*node;
	add_serialcache_msg_t  *mbuf_n;

	/*over threshold , drop the msg*/
	/*insert list by lid*/
	list_for_each_safe(pos, node,&msg_info->pSerialheader->link) 
	{
		mbuf_n = list_entry(pos, add_serialcache_msg_t, link);
        
        if(msg_info->MsgType == METER_TX_OK && (mbuf_n->state == WATIING || mbuf_n->state == SENDOK)) 
		{
		    return mbuf_n; 
        }
        
		if(mbuf_n->ack && mbuf_n->state == SENDOK) 
		{
		    if(mbuf_n->Match != NULL)
            {
                if(mbuf_n->Match(mbuf_n->Protocoltype, msg_info->Protocoltype, mbuf_n->FrameSeq, msg_info->FrameSeq) == TRUE)
                {
                    //printf_s("find corresponding uplink frame\n");
				    return mbuf_n;
                }
                else
                {

                }
            }
			else //Not need to match,透传类数据
			{
				 return mbuf_n; 
			}
		}
	}
	return NULL;
}

static void serial_cache_del_list_func(work_t *work)
{
	recive_msg_t *msg_info = (recive_msg_t *)work->buf;
	//app_printf("msg_info->Direction = %d\n", msg_info->Direction);

	if(msg_info->Direction == 1) //主动
	{
	    if(msg_info->process != NULL)
		{ 
		    msg_info->process(msg_info->FrameUnit,msg_info->FrameLen);
		}    
	}
	else //从动
	{
		if(msg_info->pSerialheader->nr >0)
		{
			add_serialcache_msg_t  *mbuf_n =serial_cache_find_msg(msg_info);
			if(mbuf_n !=NULL) //find the Correct fream
			{
                //app_printf("free mbuf_n p = %08x,mbuf_n->ack = %d\n", mbuf_n,mbuf_n->ack);

                if(msg_info->MsgType == METER_TX_OK )
                {
                    if(mbuf_n->ack == 0)
                    {
                        //处理后续帧
                        if(msg_info->pSerialheader->nr > 1) //判断是否有后续任务
                        {
                            U32 time;
                            time = (mbuf_n->IntervalTime == 0?SERICAL_PER: mbuf_n->IntervalTime);
                            serial_cache_modify_timer(time*10);
                        }
                        else
                        {
                            if(zc_timer_query(serial_cache_timer) != TMR_STOPPED)
                            {
                                //timer_stop(serial_cache_timer,TMR_NULL);
                            }
                        }
                        //删除发送完成的内容
						serial_cache_delete_list(mbuf_n);
                    }
                    else if(mbuf_n->ack == 1)
                    {
                        //无动作
                    }
                }
                else
                {
                    if(mbuf_n->ack == 1)
                    {
                        //处理后续帧
                        if(msg_info->pSerialheader->nr > 1) //判断是否有后续任务
                        {
                            U32 time;
                            time = (mbuf_n->IntervalTime == 0?SERICAL_PER: mbuf_n->IntervalTime);
                            serial_cache_modify_timer(time*10);
                        }
                        else
                        {
                            if(zc_timer_query(serial_cache_timer) != TMR_STOPPED)
                            {
                                //timer_stop(serial_cache_timer,TMR_NULL);
                            }
                        }

                        //处理接收
                        if(mbuf_n->RevDel != NULL)
						{
				        mbuf_n->RevDel((void*)mbuf_n, msg_info->FrameUnit,msg_info->FrameLen, msg_info->Protocoltype, 
				                                       mbuf_n->FrameSeq);
						}

						serial_cache_delete_list(mbuf_n);
                    }
                }
			}
            else
			{
                app_printf("not found.\n");
			}
		}
        if(msg_info->process != NULL)
		{
			msg_info->process(msg_info->FrameUnit, msg_info->FrameLen);
		}
	}
}

/**
* @brief	各个任务和模块通过此接口将数据发到串口缓冲队列中。之后本模块按照参数发送
* @param	serial_cache_msg_t  msg				本次发送的参数
* @param	uint8_t * 			payload			数据载荷
* @param	U8					post			post标志，是否需要发送到work队列
* @return	void
*******************************************************************************/
void serial_cache_add_list(add_serialcache_msg_t msg,uint8_t *payload,U8 post)
{
    if(msg.FrameLen == 0)
    {
        return;
    }
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(add_serialcache_msg_t)+msg.FrameLen,"ADSE1",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	add_serialcache_msg_t *msg_info = (add_serialcache_msg_t *)work->buf;
	__memcpy(work->buf,(uint8_t *)&msg,sizeof(add_serialcache_msg_t));
	__memcpy(msg_info->FrameUnit,payload,msg.FrameLen);
	if(post)
	{
		app_printf("uart send to queue!\n");
		work->work = serial_cache_add_list_func;
        work->msgtype = ADDLF;
		post_app_work(work);	
	}
	else
	{
		serial_cache_add_list_func(work);
		zc_free_mem(work);
	}
}

/**
* @brief	串口收到数据时将与解析后的数据回传到发送队列，发送队列即可判断执行后续动作
* @param	recive_msg_t		msg				本次发送的参数
* @param	uint8_t * 			payload			数据载荷
* @return	void
*******************************************************************************/
void serial_cache_del_list(recive_msg_t msg,uint8_t *payload)
{
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(recive_msg_t)+msg.FrameLen,"DESE",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	recive_msg_t *msg_info = (recive_msg_t *)work->buf;
	__memcpy(work->buf,(uint8_t *)&msg,sizeof(recive_msg_t));
	__memcpy(msg_info->FrameUnit,payload,msg.FrameLen);
	work->work = serial_cache_del_list_func;
    work->msgtype = DLELF;
	post_app_work(work);
}
//U8  serial_cache_cnt = 0;

static void serial_cache_timer_cb(struct ostimer_s *ostimer, void *arg)
{
	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(add_serialcache_msg_t),"SRLCH",MEM_TIME_OUT);
	if(NULL == work)
	{
		return;
	}
	add_serialcache_msg_t *msg_info = (add_serialcache_msg_t *)work->buf;
	msg_info->pSerialheader = (serial_cache_header_t*)arg;
    msg_info->CRT_timer = ostimer;
    work->msgtype = UPSCIF;

	//serial_cache_update_info(work);
	//zc_free_mem(work);
	work->work = serial_cache_update_info;
    if(msg_info->pSerialheader->nr > 0)
    {
	    post_app_work(work);	
        serial_cache_modify_timer(GUARD_TIME);
    }
    else
    {
        zc_free_mem(work);
    }
}

/**
* @brief	串口任务初始化接口
* @param	无
* @return	TRUE- init ok
*******************************************************************************/
int8_t serial_cache_timer_init(void)
{
	INIT_LIST_HEAD(&SERIAL_CACHE_LIST.link);
	SERIAL_CACHE_LIST.nr = 0;
	#if defined (STATIC_MASTER)
	SERIAL_CACHE_LIST.thr = 50;
	#else
	SERIAL_CACHE_LIST.thr = 30;
	#endif

	if(serial_cache_timer == NULL)
	{
        serial_cache_timer = timer_create(10,
	                            SERICAL_PER,
	                            TMR_TRIGGER,//TMR_TRIGGER
	                            serial_cache_timer_cb,
	                            &SERIAL_CACHE_LIST,
	                            "serial_cache_timer_cb"
	                           );
	}
	return TRUE;
}

/**
* @brief	空闲空间获取接口
* @param	void
* @return	空闲条目数
*******************************************************************************/
uint8_t serial_cache_list_ide_num()
{
	if(serial_cache_list_is_full() == TRUE)
	{
		return (SERIAL_CACHE_LIST.thr - SERIAL_CACHE_LIST.nr);
	}
	else
	{
		return 0;
	}
}


static int8_t serial_cache_list_is_full()
{
  //app_printf("$$$$$$$$nr=%d,thr=%d\n",SERIAL_CACHE_LIST.nr,SERIAL_CACHE_LIST.thr);
  return  SERIAL_CACHE_LIST.nr < SERIAL_CACHE_LIST.thr ?TRUE:FALSE;
}

static void serial_cache_modify_timer(uint32_t first)
{
	//对first值做保护
	if(first == 0)
    {
        app_printf("serial_cache_modify_timer = 0\n");
        return;
    }  
    //app_printf("md se timer = %d\n",first);
	if(NULL != serial_cache_timer)
	{
		timer_modify(serial_cache_timer,
				first,
				
				SERICAL_PER,
				TMR_TRIGGER ,//TMR_TRIGGER
				serial_cache_timer_cb,
				&SERIAL_CACHE_LIST,
                "serial_cache_timer_cb",
                TRUE);
	}
	else
	{
		sys_panic("serial_cache_timer is null\n");
	}
    //serial_cache_cnt = 0;
	timer_start(serial_cache_timer);
}
