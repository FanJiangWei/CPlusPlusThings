/*
 * Copyright: (c) 2006-2020, 2017 zc-power Technology, Inc.
 * All Rights Reserved.
 *
 * File:	infared.c
 * Purpose:	infared read and write
 * History:
 *	sep 6, 2017	panyu   Create
 */

#include "meter.h"
#include "sys.h"
#include "vsh.h"
#include <string.h>
#include "datalinkdebug.h"
#include "app_dltpro.h"
#include "protocolproc.h"
#include "app_base_serial_cache_queue.h"
#include "app.h"


#if defined(ZC3750CJQ2)
void infrared_uart_init(gpio_dev_t *dev);

static void infrared_set_intr_mode(meter_dev_t *dev);


gpio_dev_t infrared_gpio =
{
    .init    = infrared_uart_init,
    .name    = "infrared uart",
    .type    = GPIO_TYPE_UART2,
    #if defined(ROLAND1_1M)
	.pins    = GPIO_03 | GPIO_02,
	#elif defined(STD_DUAL)
	.pins    = GPIO_04 | GPIO_05,
	#else
    .pins    = GPIO_00 | GPIO_06,
    #endif
    
    .dirs    = 0,
    .mux={
        {
        #if defined(ROLAND1_1M)
    	    .id  = GPIO_03_MUXID,
		#elif defined(STD_DUAL)
			.id  = GPIO_05_MUXID,
    	#else
            .id  = GPIO_00_MUXID,
        #endif
            .dir = GPIO_OUT,
        },
        {
        #if defined(ROLAND1_1M)
    	    .id  = GPIO_02_MUXID,
		#elif defined(STD_DUAL)
			.id  = GPIO_04_MUXID,
    	#else
            .id  = GPIO_06_MUXID,
        #endif
            .dir = GPIO_IN,
        },
    },
};

meter_dev_t infrared_devices =
{
    .gpio = &infrared_gpio,
    .uart = UART2,
    .irq  = IRQ_UART2,
    .mode = 1,

    .baudrate = UART2_BAUDRATE,

    .set_mode             = meter_set_mode,
    .master_read          = meter_uart_read,
    .master_write         = meter_uart_write,
    .master_read_timeout  = meter_uart_read_timeout,

    .rx_fifo_rwm          = 15, /*256 bytes*/  /*14 :224*/
    .tx_fifo_twm          = 0,

	.status               = METER_RX_STATUS,
};
meter_dev_t *g_infrared = &infrared_devices;
ostimer_t *infraredtimer = NULL;

static void infraredtimerCB(struct ostimer_s *ostimer, void *arg)
{
	g_infrared->status = METER_RX_STATUS;
	//uart_set_int_enable(infrared_devices.uart, IER_RDA | IER_RLS | IER_RTO);
}

void infrared_uart_init(gpio_dev_t *dev)
{
    uint32_t lcr_val;

    uart_init(g_infrared->uart, g_infrared->baudrate,PARITY_EVEN,STOPBITS_1,DATABITS_8);

    lcr_val = uart_get_line_ctrl(g_infrared->uart);
    lcr_val |= LCR_EVEN_PARITY | LCR_PARITY_ENABLE;
    uart_set_line_ctrl(g_infrared->uart, lcr_val);
}

__LOCALTEXT __isr__ void isr_infrared(void *data, void *context)
{
    meter_dev_t *dev = (meter_dev_t *)data;
    uint32_t iifc, lsr, ier;
	uint8_t i=0;

    iifc   = uart_get_int_info(dev->uart);
    ier    = uart_get_int_enable(dev->uart);
    lsr    = uart_get_line_status(dev->uart);

    /*sys boot up, hardware is not stable*/
    if (((iifc & 0x1) == 1) && (ier & IER_RTO) && (lsr == (LSR_TFE | LSR_TE)))
    {
        uart_set_rx_fifo_non_empty_int(dev->uart, 0);
        uart_set_rx_fifo_non_empty_int(dev->uart, 1);
        uart_clear_mintr(dev->uart);
        dev->debug_patch++;
        debug_printf(&dty, DEBUG_METER, "\n\n\n uart2 patch!!!\n\n\n");
        return;
    }

    /* transmitter holding register empty interrupt type */
    if (meter_uart_check_thre_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "uart2 iifc thre int.\n");
        dev->debug_tx_empty++;
        dev->status = METER_TX_STATUS;
        meter_uart_tx_empty(dev);
    }

    /* tx watermark interrupt */
    if (meter_uart_check_twm_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "-----uart2 iifc twm int.\n");
        dev->debug_tx_twm++;
        meter_uart_tx(dev);
    }

    /* receive data available interrupt type */
    if (meter_uart_check_rda_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "uart2 iifc rda int.\n");
        dev->debug_rx++;
        meter_uart_rx(dev);
	    //debug_printf(&dty, DEBUG_METER, "dev->recv_buf last value = %02x\n  dev->recv = %d\n", *(dev->recv_buf + dev->recv - 1), dev->recv);
    }

    /* receive line status interrupt type */
    if (meter_uart_check_rls_int(ier, iifc, lsr))
    {
        dev->debug_rx_err++;
        /* parity error indicator */
        if (lsr & LSR_PE)
        {
            debug_printf(&dty, DEBUG_METER, "uart2 rls int by parity error.\n");
        }
        /* overrun error indicator */
        if (lsr & LSR_OE)
        {
            debug_printf(&dty, DEBUG_METER, "uart2 rls int by overrun error.\n");
        }
        /* frame error indicator, the stop bit not detected high level */
        if (lsr & LSR_FE)
        {
            debug_printf(&dty, DEBUG_METER, "uart2 rls int by frame error.\n");
        }
        /* break interrupt indicator */
        if (lsr & LSR_BI)
        {
            debug_printf(&dty, DEBUG_METER, "uart2 rls int by break interrupt.\n");
        }
    }

    /* Receive fifo non-empty timeout interrupt */
    if (meter_uart_check_rto_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "uart2 rx fifo non-empty timeout int.\n");
		if(METER_RX_STATUS == dev->status)
		{
	        dev->debug_rx_timeout++;
	        meter_uart_rx(dev);
	        sem_post(dev->uart_event);
		}
		else
		{
			i=uart_get_rf_cnt(dev->uart);
			while(i--)
			{
				uart_get_rxbuf(dev->uart);
			}
			
		}
    }

    uart_clear_mintr(dev->uart);

    return;
}

static void infrared_set_intr_mode(meter_dev_t *dev)
{
    uint8_t uart = dev->uart;
    uint8_t irq  = dev->irq;

    uart_set_int_enable(uart, 0); //disable write and read intr
    request_irq(5, irq, isr_infrared, dev);

    uart_set_rx_fifo_threshold(uart, dev->rx_fifo_rwm);
    uart_set_tx_fifo_threshold(uart, dev->tx_fifo_twm);
    uart_set_rx_fifo_non_empty_time(uart, 22000000 / infrared_devices.baudrate); //2 * 33bit /* 2048 * 3us */
    //debug_printf(&dty, DEBUG_PHY, "22000000/meter_devices.baudrate : %d\n",22000000/meter_devices.baudrate);
    uart_set_int_enable(uart, IER_RDA | IER_RLS | IER_RTO);
}
static void infrared_recv_deal(uint8_t *buf, uint32_t len)
{
 	 uint8_t state = 0xff;
	 do
	 {
		 state = PTCL_Decode(1,buf , &len , e_DLT645_MSG,e_UART2_MSG);
		 if(state == e_Decode_Success)
			 debug_printf(&dty, DEBUG_METER, "e_DLT645_MSG success!\n");
	 }
	 while(state == e_Decode_Success && len != 0);
	 do
	 {
		 state = PTCL_Decode(1,buf , &len ,e_DLT698_MSG,e_UART2_MSG);
		 if(state == e_Decode_Success)
			 debug_printf(&dty, DEBUG_METER, "e_DLT698_MSG success!\n");
	 }
	 while(state == e_Decode_Success && len != 0);

}
uart_recv_fn uart2_recv;

void register_uart2_recv_proc(uart_recv_fn fn)
{
    uart2_recv = fn;

    return ;
}

void infrared_msg_proc(work_t *work)
{

	printf_ir("infrared_msg_proc.\n");

    recive_msg_t *msg_info = (recive_msg_t *)work->buf;
	
     if(msg_info->Direction ==1) //主动
    {
		//ReadMeterForInfrared(msg_info->Protocoltype,msg_info->MatchInfo,msg_info->FrameUnit,msg_info->FrameLen);
		//调用红外处理函数
		ProcPrivateProForRed(msg_info->FrameUnit,msg_info->FrameLen);
		/*
        if(msg_info->process != NULL)
            msg_info->process(msg_info->FrameUnit,msg_info->FrameLen);
            */
    }
    else //从动
    {
    	 if(msg_info->process != NULL)
            msg_info->process(msg_info->FrameUnit,msg_info->FrameLen);
    }
	//ProcPrivateProForRed
}

void put_serial_infrared_msg(recive_msg_t msg,uint8_t *payload)
{

	work_t *work = zc_malloc_mem(sizeof(work_t)+sizeof(recive_msg_t)+msg.FrameLen,"DESE",MEM_TIME_OUT);
	recive_msg_t *msg_info = (recive_msg_t *)work->buf;
	__memcpy(work->buf,(uint8_t *)&msg,sizeof(recive_msg_t));
	__memcpy(msg_info->FrameUnit,payload,msg.FrameLen);
	work->work = infrared_msg_proc;
    work->msgtype = IRSCIF;
	post_app_work(work);
		
}


void uart2_send(uint8_t *buf, uint32_t len)
{
    meter_dev_t *dev = g_infrared;
    meter_write_tx(dev, buf, len);
    return ;
}
extern U32 g_Irsenddelay; 

void infrared_send(uint8_t *buffer, uint16_t len)
{
	printf_ir("---------infrared_send---------");
	g_infrared->status = METER_TX_STATUS;
	//os_sleep(g_Irsenddelay/10);

    uart2_send(buffer, len);
}

void infrared_task()
{
	uint32_t len;
    static uint8_t buf[METER_BUF_SIZE];
	register_uart2_recv_proc(infrared_recv_deal);
	
	os_sleep(10);
	infraredtimer = timer_create(100,
								0, 
								TMR_TRIGGER, 
								infraredtimerCB, 
								NULL, 
								"infraredtimerCB"
								);
								
	
    while (1)
    {
        sem_pend(g_infrared->uart_event, 0);
		len = meter_read_rx(g_infrared, buf);
        debug_printf(&dty, DEBUG_METER, "infrared_read_rx len : %d\n", len);
        if (uart2_recv)
        {
            uart2_recv(buf, len);
        }
    }
}

#define TASK_INFRARED_STK_SZ  1024
#define TASK_PRIO_INFRARED 48
uint32_t infrared_task_work[TASK_INFRARED_STK_SZ];
event_t ginfrared_evert;
void infrared_task_init()
{
    int32_t pid_work;
	ir_cfg_t tp_cfg = {
		.enable = ENABLE,
		.freq   = 38000,
		.high   = 1,
		.low    = 1,
	};
	
    //if (!(g_infrared->uart_event = sem_create(0, "g_infrared->uart_event")))
        //sys_panic("<infrared_task_init event panic> %s: %d\n", __func__, __LINE__);
    sem_init(&ginfrared_evert, 0, "g_infrared uart_event");
    g_infrared->uart_event = &ginfrared_evert;
	
    if ((pid_work = task_create(infrared_task,
                                NULL,
                                &infrared_task_work[TASK_INFRARED_STK_SZ - 1],
                                TASK_PRIO_INFRARED,
                                "infrared_task",
                                &infrared_task_work[0],
                                TASK_INFRARED_STK_SZ)) < 0)
        sys_panic("<infrared_task_init panic> %s: %d\n", __func__, __LINE__);

    infrared_set_intr_mode(g_infrared);
	uart_ir_cfg(g_infrared->uart, &tp_cfg);
	//uart_ir_enable(g_infrared->uart, &tp_cfg)
}

#endif
