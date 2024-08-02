/*
 * Copyright: (c) 2006-2020, 2016 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	meter.c
 * Purpose:	Meter read and write
 * History:
 *	Oct 26, 2016	xxu   Create
 */

#include "meter.h"
#include "sys.h"
#include "vsh.h"
#include <string.h>
#include "ZCsystem.h"
#include "gpio.h"
#include "plc_io.h"
#include "uart.h"
#include "app_rdctrl.h"
#include "app_base_serial_cache_queue.h"
#include "ProtocolProc.h"
#include "app_gw3762.h"
#include "app_event_report_cco.h"
#include "app_event_report_sta.h"

#include "dl_mgm_msg.h"
#include "app_dltpro.h"
#include "printf_zc.h"

#ifndef REG32    
	#define REG32(x)  (*(volatile unsigned int *)(x))
#endif


extern gpio_dev_t meter_gpio;
ostimer_t *clear_rx_timer = NULL;

meter_dev_t meter_devices = {
	.gpio = &meter_gpio,
	.uart = UART1,
	.irq  = IRQ_UART1,
	.mode = 1,

	.baudrate = UART1_BAUDRATE,
	
	.set_mode             = meter_set_mode,
	.master_read          = meter_uart_read,
	.master_write         = meter_uart_write, 

	.rx_fifo_rwm          = 220, /*224 bytes*/
	.tx_fifo_twm          = 100,
};
meter_dev_t *g_meter = &meter_devices;

#if defined(ZC3750CJQ2)
extern meter_dev_t infrared_devices;
#endif


static void dump_debug_clear()
{
    g_meter->debug_tx = 0;
    g_meter->debug_rx = 0;
    g_meter->debug_tx_empty = 0;
    g_meter->debug_rx_timeout = 0;
    g_meter->debug_tx_twm = 0;
    g_meter->debug_rx_err = 0;
    g_meter->debug_tx_full = 0;
    g_meter->debug_rx_full = 0;

    g_meter->send = 0;
    g_meter->recv = 0;
    g_meter->write = 0;
    g_meter->read = 0;
}

void meter_uart_init(gpio_dev_t *dev)
{
    uint32_t lcr_val;
	
#if defined(HIGH_FREQUENCY)
    uart_init(g_meter->uart, g_meter->baudrate, PARITY_NONE, STOPBITS_1, DATABITS_8);
#else
	uart_init(g_meter->uart, g_meter->baudrate, PARITY_EVEN, STOPBITS_1, DATABITS_8);
#endif
    lcr_val = uart_get_line_ctrl(g_meter->uart);

#if defined(HIGH_FREQUENCY)
    lcr_val &= ~LCR_PARITY_ENABLE;
#else
	lcr_val |= LCR_EVEN_PARITY | LCR_PARITY_ENABLE;
#endif


    uart_set_line_ctrl(g_meter->uart, lcr_val);

    meter_task_init();
}

int32_t meter_uart_read(meter_dev_t *dev, uint8_t *buf, uint32_t len)
{
    uint16_t off = 0;
    int32_t ret = 0;

    while (len--)
    {
        ret = uart_getc(dev->uart, 100);
        if (-1 == ret)
            break;
        buf[off++] = ret;

        if (off >= len)
            return -EAGAIN;
    }

    return off;
}

int32_t meter_uart_write(meter_dev_t *dev, uint8_t *buf, uint32_t len)
{
    uint32_t off = 0;

    while (len--)
        uart_putc(g_meter->uart, buf[off++]);

    return 0;
}

int32_t meter_uart_read_timeout(meter_dev_t *dev, uint8_t *buf, uint32_t len, uint32_t timeout)
{
    uint32_t ret = 0;
    uint32_t base = cpu_get_cycle() + timeout;

    while (time_after(base, cpu_get_cycle()))
    {
        if (uart_rx_ready(dev->uart))
        {
            buf[ret++] = uart_get_rxbuf(dev->uart);
            if (ret == len)
                return len;
        }
    }

    return ret;
}
uint32_t meter_read_rx(meter_dev_t *dev, uint8_t *buf)
{
    uint32_t cpu_sr;
    uint32_t num;
    uint32_t ret = 0;

    cpu_sr = OS_ENTER_CRITICAL();
    if (dev->recv > dev->read)
    {
        num = dev->recv - dev->read;
        __memcpy(buf, dev->recv_buf + dev->read, num);
        dev->read = (dev->read + num) % METER_BUF_SIZE ;
        ret = num;
    }
    else if (dev->recv < dev->read)
    {
        num = METER_BUF_SIZE - dev->read;
        __memcpy(buf, dev->recv_buf + dev->read, num);
        dev->read = (dev->read + num) % METER_BUF_SIZE ;
        ret += num;
        __memcpy(buf + num, dev->recv_buf, dev->recv);
        dev->read = (dev->read + dev->recv) % METER_BUF_SIZE;
        ret += dev->recv;
    }
    else
    {
        ret = 0;
    }
    OS_EXIT_CRITICAL(cpu_sr);
    return ret;
}
__isr__ void meter_uart_rx(meter_dev_t *dev)
{
    uint32_t cpu_sr;
    uint32_t cnt = uart_get_rf_cnt(dev->uart);
    uint32_t free;

    if (cnt == 0)
        return;

    cpu_sr = OS_ENTER_CRITICAL();

    if (dev->recv > dev->read)
        free = METER_BUF_SIZE - dev->recv + dev->read - 1;
    else if (dev->recv < dev->read)
        free = dev->read - dev->recv - 1;
    else
        free = METER_BUF_SIZE;

    if (cnt > free)
    {
        dev->debug_rx_full++;
        cnt = free;
    }

    dev->rx_total += cnt;
    while (cnt--)
        dev->recv_buf[(dev->recv++) % METER_BUF_SIZE] = uart_get_rxbuf(dev->uart);
    dev->recv %= METER_BUF_SIZE;

    OS_EXIT_CRITICAL(cpu_sr);

    //sem_post(dev->uart_event);
}

static uint32_t first_num = 0;
static uint32_t cur_num = 0;
static uint32_t meter_tx_done = 0;
static uint32_t infrared_tx_done = 0;

uint32_t meter_write_tx(meter_dev_t *dev, uint8_t *buf, uint32_t len)
{
	uint32_t cpu_sr;
	uint32_t free;
	uint32_t ret = 0;
	uint32_t state;
	//uint32_t cnt = uart_get_tf_cnt(dev->uart);
	cpu_sr = OS_ENTER_CRITICAL();
	if (dev->write < dev->send)
		free = dev->send - dev->write;
	else if (dev->write > dev->send)
		free = METER_BUF_SIZE - dev->write + dev->send;
	else {
		free = METER_BUF_SIZE;
		/*if (cnt == 0)
			free = METER_BUF_SIZE;
		else {
			debug_printf(&dty, DEBUG_METER, "meter think buffer is full\n");
			OS_EXIT_CRITICAL(cpu_sr);
			return 0;
		}*/
	}

	
	if (free > len) {
		free    = len;
	if(dev==&meter_devices)
		{meter_tx_done = METER_TX_SET;}
		else{infrared_tx_done = METER_TX_SET;}	}
	
	ret = free-1;
	while (free--)
		dev->send_buf[(dev->write++) % METER_BUF_SIZE] = *(buf++);
	dev->write %= METER_BUF_SIZE;

    OS_EXIT_CRITICAL(cpu_sr);
	uart_set_tx_fifo_threshold(dev->uart, dev->tx_fifo_twm);
	state = uart_get_int_enable(dev->uart);
	uart_set_int_enable(dev->uart, state | IER_TWM | IER_THRE);

    return ret;
}


uint8_t PTCL_Decode(uint8_t sendflag,uint8_t *buf, uint32_t *len, uint8_t Protocol ,uint8_t port)
{
    uint32_t framelen = 0; //len = framelen
    uint8_t  framelenpos;
	uint8_t  RealProtocol = 0;
	
    if(e_DLT645_MSG == Protocol)
    {
        framelenpos = 9;
    }
    else if(e_DLT698_MSG == Protocol || e_GW13762_MSG == Protocol || e_GD2016_MSG == Protocol)
    {
        framelenpos = 1;
    }
    else
    {
        return e_Decode_Fail;
    }
    uint32_t pos = 0;
    DATA_t *data_t =NULL;
    if(*len < 12)
        return e_Decode_Fail;
    do
    {
        switch (*(buf + pos))
        {
        case 0x68:
        {
			framelen = (e_DLT645_MSG == Protocol)?*(buf + pos + framelenpos) + 12:
				(e_GW13762_MSG == Protocol||e_GD2016_MSG == Protocol)?*(buf + pos + framelenpos) | (*(buf + pos + framelenpos + 1) << 8):
				(e_DLT698_MSG == Protocol)?(*(buf + pos + framelenpos) | (*(buf + pos + framelenpos + 1) << 8)) + 2:0;
            if((e_DLT645_MSG == Protocol && framelen > 255) || 
                (e_DLT698_MSG == Protocol && framelen > 2048) ||
                (e_GW13762_MSG == Protocol && framelen > 2048))
            {
                debug_printf(&dty, DEBUG_METER, "framelen: 0x%04x > 2096\n",framelen);
                pos++;
                break;
            }
            //debug_printf(&dty, DEBUG_METER, "framelen : 0x%04x!\n",framelen);
            if((*len-pos) < framelen) //帧不完整
            {
                debug_printf(&dty, DEBUG_METER, "(*len-pos) < framelen) : 0x%04x!\n",framelen);
                pos++;
                break;
            }
            
			 //解决68扰码
            if(*(buf + pos + framelen - 1) != 0x16)
            {
                debug_printf(&dty, DEBUG_METER, "*(buf + pos + framelen - 1) : 0x%04x!\n",*(buf + pos + framelen - 1));
                pos++;
                break;
            }
			//645协议判定第二个68
            if(e_DLT645_MSG == Protocol)
            {
                if(*(buf + pos + 7)!=0x68)
                {
                    debug_printf(&dty, DEBUG_METER, "Protocol = %d \n" , Protocol);
                    //return e_Decode_Fail;
                    pos++;
                    break;
                }
            }
            if(*(buf + pos + framelen - 1) == 0x16)
            {
                //预解析不判断校验
                if(sendflag==1)
                {
    				//判定1376.2校验和，GD2016校验和
                    if(e_GW13762_MSG == Protocol||e_GD2016_MSG == Protocol)
                    {
                        if(*(buf + pos + framelen - 2) !=check_sum((buf + pos +3) , framelen - 3 - 2))
                        {
                            debug_printf(&dty, DEBUG_METER, "*(buf + pos + framelen - 2) = %04x,check_sum((buf + pos +3) , framelen-3-2) = %04x!\n", *(buf + pos + framelen - 2) , check_sum((buf + pos) , framelen-2));
                            //return e_Decode_Fail;
                            pos++;
                            break;
                        }
                    }
                    //判定645校验和
                    if(e_DLT645_MSG == Protocol)
                    {
                        if(*(buf + pos + framelen - 2) !=check_sum((buf + pos) , framelen - 2))
                        {
                            debug_printf(&dty, DEBUG_METER, "*(buf + pos + framelen - 2) = %04x,check_sum((buf + pos) , framelen-2) = %04x!\n", *(buf + pos + framelen - 2) , check_sum((buf + pos) , framelen-2));
                            //return e_Decode_Fail;
                            pos++;
                            break;
                        }
                    }
                    //判定698 CRC16校验
                    if(e_DLT698_MSG == Protocol)
                    {
                        U16  cs16;
                        U16  CRC16Cal;
                        cs16 = (*(buf + pos + framelen -3) | (*(buf + pos + framelen -2) << 8));
                        CRC16Cal = ResetCRC16();
                        if(cs16 != CalCRC16(CRC16Cal,(buf + pos +1) , 0 , framelen-4))
                        {
                            debug_printf(&dty, DEBUG_METER, "cs16 = %04x , CRC16Cal = %04x!\n", cs16 , CRC16Cal);
                            pos++;
                            break;
                        }
                    }
                }
            	 if(sendflag==1)
				 {
	                //dump_buf(buf+pos,framelen);
	                //post message
					data_t=zc_malloc_mem(sizeof(DATA_t)+framelen,"PTCL_Decode",MEM_TIME_OUT);

                    //app_printf("data_t p = %08x\n", data_t);
                    
	                data_t->PayloadLen = framelen;
	                __memcpy(data_t->Payload, buf + pos, framelen);
                    #if defined(STATIC_MASTER)
                        if(getHplcTestMode() ==  RD_CTRL)
						{
						
						}
						else
						{
						    if(e_GW13762_MSG == Protocol)
						    {
						        AppGw3762CtrlField_t   *pCtrlField = (AppGw3762CtrlField_t*)&data_t->Payload[APP_GW3762_HEAD_LEN+APP_GW3762_LEN_LEN];
                                AppGw3762DownInfoField_t *pDowInfoField = (AppGw3762DownInfoField_t*)&data_t->Payload[APP_GW3762_HEAD_LEN+APP_GW3762_LEN_LEN
                                                                                                                         +APP_GW3762_CTRL_LEN];
						        recive_msg_t   SerialMsg;

								debug_printf(&dty, DEBUG_METER,"port : %d Protocol : %d!\n",port,Protocol);
                                                                
								SerialMsg.Protocoltype = GW3762;
                                SerialMsg.FrameSeq = pDowInfoField->FrameNum;
								SerialMsg.FrameLen = data_t->PayloadLen;
								SerialMsg.process = proc_uart_2_gw13762_data;
								SerialMsg.Direction = pCtrlField->StartFlag;
                                SerialMsg.MsgType = METER_RX_OK;
								SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
								                           
                                serial_cache_del_list(SerialMsg, data_t->Payload);
								zc_free_mem(data_t);                  
                            }
                            else
							{
		                        if(e_DLT645_MSG == Protocol || e_DLT698_MSG == Protocol)
                                {
                                    U8 MeterAddr[6] = {0};
                                    U8 ControlCode = 0;
        			                U8 FECount     = 0;
        			                //U8 DataLen     = 0;

                                    recive_msg_t   SerialMsg;
                                    
                                    if(e_DLT645_MSG == Protocol)
                                    {
                                     
                                        Check645Frame(data_t->Payload, data_t->PayloadLen, &FECount,MeterAddr, &ControlCode);

                                        //RealProtocol = Check645TypeByCtrlCode(ControlCode);
                                        RealProtocol = DLT645_2007;
                                        SerialMsg.Direction = ((ControlCode & 0x80) >> 7 == 1)?0:1;
                                        if(SerialMsg.Direction == 1)
                                        {
                                            SerialMsg.process = ProDLT645Extend;   // Extend self 645 protocol 
                                        }
                                        else
                                        {
                                            SerialMsg.process = NULL;   // Handling active 645 protocol 
                                        }
                                    }
                                    else if(e_DLT698_MSG == Protocol)
                                    {
                                        Check698Frame(data_t->Payload, data_t->PayloadLen,&FECount, MeterAddr, NULL);
                                        RealProtocol = DLT698;
                                        SerialMsg.process = NULL;   // Handling active 645 protocol 
                                        SerialMsg.Direction = 0; 
                                    }
                                    
                                    SerialMsg.Protocoltype = RealProtocol;
                                    SerialMsg.FrameSeq = 0;
                                    SerialMsg.FrameLen = data_t->PayloadLen;
                                    SerialMsg.MatchLen = 6;
                                    SerialMsg.MsgType = METER_RX_OK;
                                    __memcpy(SerialMsg.MatchInfo, MeterAddr, 6);
                                    
                                    
                                    SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
        								                           
                                    serial_cache_del_list(SerialMsg, data_t->Payload);
                                    debug_printf(&dty, DEBUG_METER,"SerialMsg!SerialMsg.FrameLen %d\n",SerialMsg.FrameLen);
                                    zc_free_mem(data_t);
                                }
							}
						}
					#elif defined(ZC3750STA) 
                    {
                    	if(getHplcTestMode() ==  RD_CTRL)
                    	{
	                        if(e_DLT645_MSG == Protocol)
	                        {
	                            U8 MeterAddr[6] = {0};
	                            U8 ControlCode = 0;
				                U8 FECount     = 0;
				                //U8 DataLen     = 0;

	                            recive_msg_t   SerialMsg;
	                            
	                            if(e_DLT645_MSG == Protocol)
	                            {
	                             
	                                Check645Frame(data_t->Payload, data_t->PayloadLen, &FECount,MeterAddr, &ControlCode);

	                                //RealProtocol = Check645TypeByCtrlCode(ControlCode);
	                                RealProtocol = DLT645_2007;
	                                SerialMsg.Direction = ((ControlCode & 0x80) >> 7 == 1)?0:1;
	                                if(SerialMsg.Direction == 1)
	                                {
	                        			SerialMsg.process = rd_ctrl_pro_dlt645_ex; 
	                                }
	                                else
	                                {
	                                    SerialMsg.process = NULL;   // Handling active 645 protocol 
	                                }
	                            }
	                            
	                            SerialMsg.Protocoltype = RealProtocol;
	                            SerialMsg.FrameSeq = 0;
	                            SerialMsg.FrameLen = data_t->PayloadLen;
	                            SerialMsg.MatchLen = 6;
	                            __memcpy(SerialMsg.MatchInfo, MeterAddr, 6);
	                            
	                            
	                            SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
									                           
	                            serial_cache_del_list(SerialMsg, data_t->Payload);
	                            debug_printf(&dty, DEBUG_METER,"SerialMsg!SerialMsg.FrameLen %d\n",SerialMsg.FrameLen);
	                            zc_free_mem(data_t);
	                        }
						}
						else
						{
							if(e_DLT645_MSG == Protocol || e_DLT698_MSG == Protocol)
						   {
							   U8 MeterAddr[6] = {0};
							   U8 ControlCode = 0;
							   U8 FECount	  = 0;
							   //U8 DataLen 	= 0;
						
							   recive_msg_t   SerialMsg;
							   
							   if(e_DLT645_MSG == Protocol)
							   {
								
								   Check645Frame(data_t->Payload, data_t->PayloadLen, &FECount,MeterAddr, &ControlCode);
						
								   //RealProtocol = Check645TypeByCtrlCode(ControlCode);
								   RealProtocol = DLT645_2007;
								   SerialMsg.Direction = ((ControlCode & 0x80) >> 7 == 1)?0:1;
								   if(SerialMsg.Direction == 1)
								   {
									   SerialMsg.process = ProDLT645Extend;   // Extend self 645 protocol 
								   }
								   else
								   {
									   SerialMsg.process = NULL;   // Handling active 645 protocol 
								   }
							   }
							   else if(e_DLT698_MSG == Protocol)
							   {
									/*
								   Check698Frame(data_t->Payload, data_t->PayloadLen,&FECount, MeterAddr, NULL);
								   extern U8 Check698EventReport(U8* pDatabuf, U16 dataLen);
								   if(e_DLT698_MSG == Protocol && Check698EventReport(buf + pos, framelen) == TRUE)
								   {
									   SerialMsg.process = Sta20MeterRecvDeal;	 // Handling active 645 protocol 
									   SerialMsg.Direction = 1;
								   }
								   else
								   {
									   SerialMsg.process = NULL;   // Handling active 645 protocol 
									   SerialMsg.Direction = 0;
								   }
									
								   RealProtocol = DLT698;
								   */
								   /*
								   Check698Frame(data_t->Payload, data_t->PayloadLen,&FECount, MeterAddr, NULL);
                                   dl698frame_header_s *pFrameHeader = NULL;
                                   U8 *pApdu = NULL;
                                   U16 ApduLen = 0;
                                   ParseCheck698Frame(data_t->Payload, data_t->PayloadLen, &pFrameHeader, &pApdu, MeterAddr, &ApduLen);
                                   SerialMsg.Prmbit = pFrameHeader->CtrlField.DirBit;
                                   SerialMsg.Direction = pFrameHeader->CtrlField.PrmBit;
								   printf_s("dirbit : %d, prmbit : %d\n",SerialMsg.Direction,SerialMsg.Prmbit);
								   
                                   if (SerialMsg.Direction == 1 && SerialMsg.Prmbit == 0)
                                   {
                                       SerialMsg.process = process_698_extend; // Extend self 645 protocol
                                   }
                                   else if(SerialMsg.Direction == 1 && SerialMsg.Prmbit == 1)
                                   {
									   extern U8 Check698EventReport(U8 * pDatabuf, U16 dataLen);
                                       if (Check698EventReport(buf + pos, framelen) == TRUE)
                                       {
                                           SerialMsg.process = Sta20MeterRecvDeal;
									   }
								   }
								   else
                                   {
                                       SerialMsg.process = NULL; // Handling active 645 protocol
                                   }
                                   */
                                   dl698frame_header_s *pFrameHeader = NULL;
                                   U8 *pApdu = NULL;
                                   U16 ApduLen = 0;
                                   ParseCheck698Frame(data_t->Payload, data_t->PayloadLen, &pFrameHeader, &pApdu, MeterAddr, &ApduLen);
                                   Check698Frame(data_t->Payload, data_t->PayloadLen,&FECount, MeterAddr, NULL);
								   extern U8 Check698EventReport(U8* pDatabuf, U16 dataLen);
									SerialMsg.Prmbit = pFrameHeader->CtrlField.DirBit;
									SerialMsg.Direction = pFrameHeader->CtrlField.PrmBit;
									app_printf("dir : %d\n",SerialMsg.Direction);
									app_printf("prm : %d\n",SerialMsg.Prmbit);
									if(SerialMsg.Prmbit == 0 && SerialMsg.Direction == 1)
                                    {
                                       if (TRUE == Check_zc_698_extend(data_t->Payload, data_t->PayloadLen))
                                       {
                                           SerialMsg.process = process_698_extend; // Extend self 645 protocol
                                       }
                                       else
                                       {
                                           SerialMsg.process = NULL; // Extend self 645 protocol
                                       }

                                       SerialMsg.Direction = 1;
                                    }
                                    else
									{
										if(e_DLT698_MSG == Protocol && Check698EventReport(buf + pos, framelen) == TRUE)
										{
											SerialMsg.process = Sta20MeterRecvDeal;   // Handling active 645 protocol 
											SerialMsg.Direction = 1;
										}
										else
										{
											SerialMsg.process = NULL;	// Handling active 645 protocol 
											SerialMsg.Direction = 0;
										}
									}
									
                                   RealProtocol = DLT698;
                               }
							   
							   SerialMsg.Protocoltype = RealProtocol;
							   SerialMsg.FrameSeq = 0;
							   SerialMsg.FrameLen = data_t->PayloadLen;
							   SerialMsg.MatchLen = 6;
							   SerialMsg.MsgType = METER_RX_OK;
							   __memcpy(SerialMsg.MatchInfo, MeterAddr, 6);
							   
							   SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
															  
							   serial_cache_del_list(SerialMsg, data_t->Payload);
							   debug_printf(&dty, DEBUG_METER,"SerialMsg!SerialMsg.FrameLen %d,RealProtocol = %d\n",SerialMsg.FrameLen,SerialMsg.Protocoltype);
							   zc_free_mem(data_t);
						    }
   
                        }
                        
					}

					#elif defined(ZC3750CJQ2)
					if(e_DLT645_MSG == Protocol || e_DLT698_MSG == Protocol)
                    {
                        U8 MeterAddr[6] = {0};
                        U8 ControlCode = 0;
		                U8 FECount     = 0;
		                //U8 DataLen     = 0;

                        recive_msg_t   SerialMsg;
                        
                        if(e_DLT645_MSG == Protocol)
                        {
                         
                            Check645Frame(data_t->Payload, data_t->PayloadLen, &FECount,MeterAddr, &ControlCode);

                            //RealProtocol = Check645TypeByCtrlCode(ControlCode);
                            RealProtocol = DLT645_2007;
                            SerialMsg.Direction = ((ControlCode & 0x80) >> 7 == 1)?0:1;
                            if(SerialMsg.Direction == 1)
                            {
                                SerialMsg.process = ProDLT645Extend;   // Extend self 645 protocol 
                            }
                            else
                            {
                                SerialMsg.process = NULL;   // Handling active 645 protocol 
                            }
                        }
                        else if(e_DLT698_MSG == Protocol)
                        {
                            /* Check698Frame(data_t->Payload, data_t->PayloadLen,&FECount, MeterAddr, NULL);
                            // extern U8 Check698EventReport(U8* pDatabuf, U16 dataLen);
                            if(e_DLT698_MSG == Protocol && Check698EventReport(buf + pos, framelen) == TRUE)
                            {
                                SerialMsg.process = Sta20MeterRecvDeal;   // Handling active 645 protocol 
                            }
                            else
                            {
                                SerialMsg.process = NULL;   // Handling active 645 protocol 
                            }
							SerialMsg.Direction =((data_t->Payload[FECount+3] & 0x80) >> 7 == 1)?0:1;
							
                            RealProtocol = DLT698; */
                            dl698frame_header_s *pFrameHeader = NULL;
                            U8 *pApdu = NULL;
                            U16 ApduLen = 0;
                            //    extern U8 Check698EventReport(U8 * pDatabuf, U16 dataLen);
                            ParseCheck698Frame(data_t->Payload, data_t->PayloadLen, &pFrameHeader, &pApdu, MeterAddr, &ApduLen);
                            SerialMsg.Direction = pFrameHeader->CtrlField.DirBit;
                            if (SerialMsg.Direction == 0)
                            {
                                SerialMsg.process = process_698_extend; // Extend self 645 protocol
                            }
                            else
                            {
                                if (Check698EventReport(buf + pos, framelen) == TRUE)
                                {
                                    SerialMsg.process = Sta20MeterRecvDeal;
                                    SerialMsg.Direction = 1;
                                }
                                else
                                {
                                    SerialMsg.process = NULL; // Handling active 645 protocol
                                    SerialMsg.Direction = 0;
                                }
                            }
                            RealProtocol = DLT698;
                        }
                        
                        SerialMsg.Protocoltype = RealProtocol;
                        SerialMsg.FrameSeq = 0;
                        SerialMsg.FrameLen = data_t->PayloadLen;
                        SerialMsg.MatchLen = 6;
                        SerialMsg.MsgType = METER_RX_OK;
                        __memcpy(SerialMsg.MatchInfo, MeterAddr, 6);
                        
                        if(port == e_UART1_MSG)
                        {
                            SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
							                           
                            serial_cache_del_list(SerialMsg, data_t->Payload);
                            debug_printf(&dty, DEBUG_METER,"SerialMsg!SerialMsg.FrameLen %d,RealProtocol = %d\n",SerialMsg.FrameLen,SerialMsg.Protocoltype);
                        }
                        else if(port == e_UART2_MSG)
                        {
                            if(e_DLT698_MSG == Protocol)
                            {
                                dl698frame_header_s *pFrameHeader = NULL;
                                U8 *pApdu = NULL;
                                U16 ApduLen = 0;
                                ParseCheck698Frame(data_t->Payload, data_t->PayloadLen, &pFrameHeader, &pApdu, MeterAddr, &ApduLen);
                                SerialMsg.Direction = pFrameHeader->CtrlField.DirBit;
                                if(1 == SerialMsg.Direction)
                                {
                                    SerialMsg.Direction = 0;
                                }
                                else
                                {
                                    SerialMsg.Direction = 1;
                                }
                            }
                            extern void put_serial_infrared_msg(recive_msg_t msg,uint8_t *payload);
                            put_serial_infrared_msg(SerialMsg, data_t->Payload);
                            debug_printf(&dty, DEBUG_METER,"put_serial_infrared_msg FrameLen %d,RealProtocol = %d\n",SerialMsg.FrameLen,SerialMsg.Protocoltype);
                        }
                        zc_free_mem(data_t);
                    }
					#endif
				 }
					 
                if((pos + framelen) < *len) //如果后续还有数据
                {
                    debug_printf(&dty, DEBUG_METER, "len : %04x\n pos+framelen : %04x!\n", *len , pos + framelen);
                    *len = *len - (pos + framelen);
                    __memmove(buf, buf + pos + framelen, *len); //将未处理数据左对齐
                    //dump_buf(buf,*len);
                    pos = 0;
                }
                else
                {
                    *len = 0;
                }
                return e_Decode_Success;
            }
            
            break;
        }
        default:
            pos++;
            break;
        }
    }
    while(pos < *len);

    return e_Decode_Fail;
}

void read_rx_clear(meter_dev_t *dev)
{
    uint32_t cpu_sr;
    cpu_sr = OS_ENTER_CRITICAL();
    dev->recv = dev->read = 0;
    memset(dev->recv_buf,0x00,METER_BUF_SIZE);
	 OS_EXIT_CRITICAL(cpu_sr);
	 app_printf("APP_T_PROTOTYPE clear rxbuf\n");
     return;
}

void clear_rx_timerCB(struct ostimer_s *ostimer, void *arg)
{
    g_meter->uart_event->type = METER_RX_TOGETHER;
    sem_post(g_meter->uart_event);
}

void modify_clear_rx_timer(U32 ms)
{

	if (clear_rx_timer == NULL)
	{
		clear_rx_timer = timer_create(ms,
										 0,
										 TMR_TRIGGER,
										 clear_rx_timerCB,
										 NULL,
										 "clear_rx_timer");
	}
	else
	{
		timer_modify(clear_rx_timer,
					 ms,
					 0,
					 TMR_TRIGGER,
					 clear_rx_timerCB,
					 NULL,
					 "clear_rx_timer",
					 TRUE);
	}
	timer_start(clear_rx_timer);
}

uint32_t read_rx_num(meter_dev_t *dev)
{
    uint32_t ret = 0;
    uint32_t cpu_sr = 0;
    uint32_t num = 0;
    cpu_sr = OS_ENTER_CRITICAL();
    if (dev->recv > dev->read)
    {
        ret = dev->recv - dev->read;
    }
    else if (dev->recv < dev->read)
    {
        num = METER_BUF_SIZE - dev->read;
        ret += num;
		ret += dev->recv;
    }
    else
    {
        ret = 0;
    }
	 OS_EXIT_CRITICAL(cpu_sr);
     return ret;
}

uint8_t read_rx_expect(meter_dev_t *dev)
{
    uint32_t cpu_sr;
    uint32_t num;
    uint32_t ret = 0;
	uint8_t state;
//    uint8_t buf[METER_BUF_SIZE];
    uint8_t *buf = zc_malloc_mem(2096, "expt", MEM_TIME_OUT);
    cpu_sr = OS_ENTER_CRITICAL();
    if (dev->recv > dev->read)
    {
        num = dev->recv - dev->read;
        __memcpy(buf, dev->recv_buf + dev->read, num);
        ret = num;
    }
    else if (dev->recv < dev->read)
    {
        num = METER_BUF_SIZE - dev->read;
        __memcpy(buf, dev->recv_buf + dev->read, num);
        ret += num;
        __memcpy(buf + num, dev->recv_buf, dev->recv);
		ret += dev->recv;
    }
    else
    {
        ret = 0;
    }
	 OS_EXIT_CRITICAL(cpu_sr);
#if defined(STATIC_MASTER)
    do
    {
        state = PTCL_Decode(0,buf , &ret , e_GW13762_MSG,dev==g_meter?e_UART1_MSG:dev==g_infrared?e_UART2_MSG:e_UART2_MSG);
        if(state == e_Decode_Success)//&&ret==0)
		{
			debug_printf(&dty, DEBUG_METER, "e_GW13762_MSG success fifo right!\n");
            zc_free_mem(buf);
			return e_Decode_Success;
		}
    }
    while(state == e_Decode_Success && ret >= 0);
#endif
    do
    {
        state = PTCL_Decode(0,buf , &ret, e_DLT645_MSG ,dev==g_meter?e_UART1_MSG:dev==g_infrared?e_UART2_MSG:e_UART2_MSG);
        if(state == e_Decode_Success)//&&ret==0)
        {
           debug_printf(&dty, DEBUG_METER, "e_DLT645_MSG success fifo right!\n");
           zc_free_mem(buf);
			return e_Decode_Success;
        }
    }
    while(state == e_Decode_Success && ret != 0);
    do
    {
        state = PTCL_Decode(0,buf , &ret , e_DLT698_MSG ,dev==g_meter?e_UART1_MSG:dev==g_infrared?e_UART2_MSG:e_UART2_MSG);
        if(state == e_Decode_Success)//&&ret==0)
        {
			debug_printf(&dty, DEBUG_METER, "e_DLT698_MSG success fifo right!\n");
            zc_free_mem(buf);
			return e_Decode_Success;
        }
            
    }
    while(state == e_Decode_Success && ret != 0);
    zc_free_mem(buf);
	
	return e_Decode_Fail;
}
__isr__ void meter_uart_tx(meter_dev_t *dev)
{
    uint32_t cpu_sr;
    uint32_t cnt = UART_TX_FIFO_MAX - uart_get_tf_cnt(dev->uart);
    uint32_t data = 0;
    uint32_t state;
	debug_printf(&dty, DEBUG_METER, "tx_tcnt=%d,w=%d,s=%d",cnt,dev->write,dev->send);

	cpu_sr = OS_ENTER_CRITICAL();
	uart_set_tx_watermark_int(dev->uart, FALSE);
	if (dev->write > dev->send)
		data = dev->write - dev->send;
	else if (dev->write < dev->send)
		data = METER_BUF_SIZE + dev->write - dev->send;
	else {
		//extern meter_dev_t infrared_devices;
		if(dev==&meter_devices)
		{
			if (meter_tx_done == METER_TX_SET) {
				meter_tx_done = METER_TX_FINISH;
                    data = 0;
				state = uart_get_int_enable(dev->uart);
				uart_set_int_enable(dev->uart, state & (~(IER_TWM | IER_THRE)));
				OS_EXIT_CRITICAL(cpu_sr);
				
				#if defined(ZC3750CJQ2)
                    debug_printf(&dty, DEBUG_METER, "tx tick%d\n",get_phy_tick_time());
					en485_OFF();
					LED_485_OFF();
                    debug_printf(&dty, DEBUG_METER, "en485_OFF-----------1\n");
				#endif
                {
                    meter_tx_done = METER_TX_IDLE;
                    if(SERIAL_CACHE_LIST.nr > 0)
                    {
                        dev->uart_event->type = METER_TX_EMPTY;
                        sem_post(dev->uart_event);
                        debug_printf(&dty, DEBUG_METER,"me tx_o\n");
                    }
                }
				return;
			}
			else
            {
				//data = METER_BUF_SIZE;
				state = uart_get_int_enable(dev->uart);
				uart_set_int_enable(dev->uart, state & (~(IER_TWM | IER_THRE)));
				OS_EXIT_CRITICAL(cpu_sr);
            	#if defined(ZC3750CJQ2)
				uart_set_int_enable(dev->uart, state | IER_THRE);
			    #endif
                debug_printf(&dty, DEBUG_METER, "me tx_null\n");
                return;
            }
		}
        #if defined(ZC3750CJQ2)
		else if(dev==&infrared_devices)
		{
			if (infrared_tx_done == METER_TX_SET) {
				infrared_tx_done = METER_TX_FINISH;
                    data = 0;
				state = uart_get_int_enable(dev->uart);
				uart_set_int_enable(dev->uart, state & (~(IER_TWM | IER_THRE)));
				OS_EXIT_CRITICAL(cpu_sr);
				
				
					extern ostimer_t *infraredtimer;

					if (infraredtimer)
			        {
			          timer_start(infraredtimer);  
			        }
			        else
			            sys_panic("<infraredtimer fail!> %s: %d\n");	
				
                
                infrared_tx_done = METER_TX_IDLE;
                infrared_devices.status = METER_RX_STATUS;
                debug_printf(&dty, DEBUG_METER,"if tx_e\n");
                
				return;
			}
            
			else
				data = METER_BUF_SIZE;
		}
        #endif
		
	}
    //if(dev==&meter_devices)
    {

        if (cnt >= data)
            cnt  = data;

        dev->tx_total += cnt;
        debug_printf(&dty, DEBUG_METER, "tx cnt:%d\n",cnt);
        while (cnt--)
            uart_set_txbuf(dev->uart, dev->send_buf[(dev->send++) % METER_BUF_SIZE]);
        dev->send %= METER_BUF_SIZE;

    	uart_set_tx_watermark_int(dev->uart, TRUE);
    }
    OS_EXIT_CRITICAL(cpu_sr);
    
}
__isr__ void meter_uart_tx_empty(meter_dev_t *dev)
{
    uint32_t cpu_sr;
    uint32_t cnt = UART_TX_FIFO_MAX - uart_get_tf_cnt(dev->uart);
    uint32_t data;
    uint32_t state;
    cpu_sr = OS_ENTER_CRITICAL();
	debug_printf(&dty, DEBUG_METER, "tx_ecnt=%d,w =%d,s=%d",cnt,dev->write,dev->send);
    if (dev->write > dev->send)
        data = dev->write - dev->send;
    else if (dev->write < dev->send)
        data = METER_BUF_SIZE + dev->write - dev->send;
    else
    {
        data = 0;
        #if defined(ZC3750CJQ2)
        if(dev==&meter_devices)
        {
            //debug_printf(&dty, DEBUG_METER, "meter_uart_tx tick%d\n",get_phy_tick_time());
    		en485_OFF();
    		LED_485_OFF();
            debug_printf(&dty, DEBUG_METER, "en485_OFF-----------2\n");
        }
    	#endif
    }
	
	debug_printf(&dty, DEBUG_METER, "data = %d\n",data);
	if (cnt >= data) {
		cnt     = data;
		
		if(dev==&meter_devices){
		    if(meter_tx_done == METER_TX_SET){
                meter_tx_done = METER_TX_SET;
            }
        }
		else{
            if(infrared_tx_done == METER_TX_SET){
                infrared_tx_done = METER_TX_SET;
            }
        }

		state = uart_get_int_enable(dev->uart);
        
        #if defined(ZC3750CJQ2)
        if(dev==&meter_devices)
        {
            uart_set_int_enable(dev->uart, state | IER_THRE); 
        }
        else
        {
            uart_set_int_enable(dev->uart, state & (~(IER_THRE)));
        }
		#else
        uart_set_int_enable(dev->uart, state & (~(IER_THRE)));//IER_TWM | 
        #endif
	}
	
    dev->tx_total += cnt;
    debug_printf(&dty, DEBUG_METER, "tx_e cnt : %d\n",cnt);
    while (cnt--)
        uart_set_txbuf(dev->uart, dev->send_buf[(dev->send++) % METER_BUF_SIZE]);
    dev->send %= METER_BUF_SIZE;
    debug_printf(&dty, DEBUG_METER, "tx_e tick%d\n",get_phy_tick_time());

	
    OS_EXIT_CRITICAL(cpu_sr);
    #if defined(ZC3750CJQ2)
    if(dev==&meter_devices && dev->write == dev->send)
    #else
    if(dev==&meter_devices && dev->write == dev->send)
    #endif
    {
        meter_tx_done = METER_TX_IDLE;
        if(SERIAL_CACHE_LIST.nr > 0)
        {
            dev->uart_event->type = METER_TX_EMPTY;
            sem_post(dev->uart_event);
            debug_printf(&dty, DEBUG_METER,"me tx_e\n");
        }
    }
    #if defined(ZC3750CJQ2)
    else if(dev==&infrared_devices && infrared_tx_done == METER_TX_FINISH)
    {
        infrared_tx_done = METER_TX_IDLE;
        infrared_devices.status = METER_RX_STATUS;
        debug_printf(&dty, DEBUG_METER,"empty tx_e\n");
    }
    #endif
}
void resetmeter(void)
{
    printf_s("\n\n\nresetmeter!!!\n\n\n");
    U32 cnt = uart_get_rf_cnt(g_meter->uart);
    while(cnt--)
    {
        uart_get_rxbuf(g_meter->uart);
    }
    
    U32 reg = 0;
    reg = REG32(0xf0000010);
    reg |= 1 << 14;
    REG32(0xf0000010) = reg;
    reg &= ~(1<< 14);
    REG32(0xf0000010) = reg;
    
    dump_debug_clear();
    
    MeterUartInit(g_meter,g_meter->baudrate);
    
    extern void uart1testmode(U32 Baud);
    if(getHplcTestMode() != NORM)
    {           
        uart1testmode( g_meter->baudrate );     
    }
    
    
    return;

}
int32_t task_all_err = 0;

__LOCALTEXT __isr__ void isr_meter(void *data, void *context)
{
    meter_dev_t *dev = (meter_dev_t *)data;
    uint32_t iifc, lsr, ier;

    iifc   = uart_get_int_info(dev->uart);
    ier    = uart_get_int_enable(dev->uart);
    lsr    = uart_get_line_status(dev->uart);

	//printf_s("^\n");
	if(task_all_err >= 10)
    {
        printf_s("fail iifc %08x ier %08x lsr %08x.\n",iifc,ier,lsr);
    }   
    /*sys boot up, hardware is not stable*/
    if (((iifc & 0x1) == 1) && (ier & IER_RTO) && (lsr == (LSR_TFE | LSR_TE)))
    {
        uart_set_rx_fifo_non_empty_int(dev->uart, 0);
        uart_set_rx_fifo_non_empty_int(dev->uart, 1);
        uart_clear_mintr(dev->uart);
        dev->debug_patch++;
		
		printf_s("\n\n\npatch!!!\n\n\n");
		U32 cnt = uart_get_rf_cnt(dev->uart);
		while(cnt--)
		{
			uart_get_rxbuf(dev->uart);
		}

		U32 reg = 0;
		reg = REG32(0xf0000010);
		reg |= 1 << 14;
		REG32(0xf0000010) = reg;
		reg &= ~(1<< 14);
		REG32(0xf0000010) = reg;
		
        dump_debug_clear();

		MeterUartInit(g_meter,g_meter->baudrate);
		
		extern void uart1testmode(U32 Baud);
        if(getHplcTestMode() != NORM)
		{			
			uart1testmode( g_meter->baudrate ); 	
		}

		
        return;
    }
    //debug_printf(&dty, DEBUG_METER, "iifc %08x ier %08x lsr %08x.\n",iifc,ier,lsr);

    /* transmitter holding register empty interrupt type */
    if (meter_uart_check_thre_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "iti\n");
        dev->debug_tx_empty++;
        dev->status = METER_TX_STATUS;
        meter_uart_tx_empty(dev);
    }

    /* tx watermark interrupt */
    if (meter_uart_check_twm_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "twm\n");
        dev->debug_tx_twm++;
        meter_uart_tx(dev);
    }

    /* receive data available interrupt type */
    if (meter_uart_check_rda_int(ier, iifc, lsr))
    {
        debug_printf(&dty, DEBUG_METER, "iri\n");
        dev->debug_rx++;
        meter_uart_rx(dev);
		uint32_t num = 0;
		if (dev->recv > dev->read)
	    {
	        num = dev->recv - dev->read;
	    }
	    else if (dev->recv < dev->read)
	    {
       	 	num = METER_BUF_SIZE - dev->read + dev->recv;
	    }
		
		debug_printf(&dty, DEBUG_METER, "rx:%d num:%d tyep%d\n",dev->debug_rx,num,dev->uart_event->type);
		
		if(0x16==*(dev->recv_buf + dev->recv - 1))
		{
            dev->uart_event->type = METER_RX_16;
            sem_post(dev->uart_event);
		}
        
        //解帧的触发条件
		if(num > 1910)
		{
		    if(0x16 == *(dev->recv_buf + dev->recv - 1))
    		{
    			 //dev->uart_event->type = METER_RX_16;
    			 dev->uart_event->type |= METER_RX_16;
    		}
		     dev->uart_event->type |= METER_RX_OVER_FIFO;
			 sem_post(dev->uart_event);
		}
        //debug_printf(&dty, DEBUG_METER, "dev->recv_buf last value = %02x\n  dev->recv = %d\n", *(dev->recv_buf + dev->recv - 1), dev->recv);
    }

    /* receive line status interrupt type */
    if (meter_uart_check_rls_int(ier, iifc, lsr))
    {
        dev->debug_rx_err++;
        /* parity error indicator */
        if (lsr & LSR_PE)
        {
            //debug_printf(&dty, DEBUG_METER, "rls int by parity error.\n");
            debug_printf(&dty, DEBUG_METER, "LSR_PE\n");
        }
        /* overrun error indicator */
        if (lsr & LSR_OE)
        {
            //debug_printf(&dty, DEBUG_METER, "rls int by overrun error.\n");
            debug_printf(&dty, DEBUG_METER, "LSR_OE\n");
        }
        /* frame error indicator, the stop bit not detected high level */
        if (lsr & LSR_FE)
        {
            //debug_printf(&dty, DEBUG_METER, "rls int by frame error.\n");
            debug_printf(&dty, DEBUG_METER, "LSR_FE\n");
        }
        /* break interrupt indicator */
        if (lsr & LSR_BI)
        {
            //debug_printf(&dty, DEBUG_METER, "rls int by break interrupt.\n");
            debug_printf(&dty, DEBUG_METER, "LSR_BI\n");
        }
        /* read err data until rx fifo is empty */
		while (uart_rx_ready(dev->uart)) {
			uart_get_rxbuf(dev->uart); 
		}
    }

    /* Receive fifo non-empty timeout interrupt */
    if (meter_uart_check_rto_int(ier, iifc, lsr))
    {
        //debug_printf(&dty, DEBUG_METER, "rx fifo non-empty timeout int.\n");
        if(0x16 == *(dev->recv_buf + dev->recv - 1))
		{
			 dev->uart_event->type |= METER_RX_16;
		}
        dev->debug_rx_timeout++;
        meter_uart_rx(dev);
        //dev->uart_event->type = METER_RX_TIMEOUT;
        /*uint32_t num = 0;
        if (dev->recv > dev->read)
	    {
	        num = dev->recv - dev->read;
	    }
	    else if (dev->recv < dev->read)
	    {
       	 	num = METER_BUF_SIZE - dev->read + dev->recv;
	    }
        if(num > 11)*/
        {
            dev->uart_event->type |= METER_RX_TIMEOUT;
            //sem_post(dev->uart_event);
            uint32_t sem_ret;
            sem_ret = sem_post(dev->uart_event);
            if(sem_ret != 0)
            {
                printf_s("sem_ret4 %d %d\n",sem_ret,dev->uart_event->count);
            }
        }
    }
    if((iifc & 0xf) == 1 && (lsr == 0x61)) {
        //读清rx fifo;
        while (uart_rx_ready(dev->uart)) {
    			uart_get_rxbuf(dev->uart); 
    		}
    }
    uart_clear_mintr(dev->uart);
    debug_printf(&dty, DEBUG_METER, "imeout\n");

    return;
}

void meter_set_intr_mode(meter_dev_t *dev)
{
    uint8_t uart = dev->uart;
    uint8_t irq  = dev->irq;

    uart_set_int_enable(uart, 0); //disable write and read intr
    request_irq(5, irq, isr_meter, dev);
    uart_set_rx_fifo_threshold(uart, dev->rx_fifo_rwm);
    uart_set_tx_fifo_threshold(uart, dev->tx_fifo_twm);
    uart_set_rx_fifo_non_empty_time(uart, 22000000 / meter_devices.baudrate); //2 * 33bit /* 2048 * 3us */
    //debug_printf(&dty, DEBUG_PHY, "88000000/meter_devices.baudrate : %d\n",22000000/meter_devices.baudrate);
	U32 cnt = uart_get_rf_cnt(dev->uart);
	while(cnt--)
	{
		uart_get_rxbuf(dev->uart);
	}
	uart_set_int_enable(uart, IER_RDA | IER_RLS | IER_RTO);
}
/*
void dump_buf(uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        if (i && (i % 100) == 0)
            debug_printf(&dty, DEBUG_APP, "\n");
        debug_printf(&dty, DEBUG_APP, "%02x ", *(buf + i));
    }
    debug_printf(&dty, DEBUG_APP, "\n");
}
void dump_level_buf(uint32_t level,uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        if (i && (i % 100) == 0)
            debug_printf(&dty, level, "\n");
        debug_printf(&dty, level, "%02x ", *(buf + i));
    }
    debug_printf(&dty, level, "\n");
}
*/
#if defined(ZC3750CJQ2)
void chaosframe(void)
{
/*
	int32_t errorcode=0;
	DATA_t *data_t=NULL;
	data_t=zc_malloc_mem(sizeof(DATA_t)+2,"chaosframe1",MEM_TIME_OUT);

	data_t->PayloadLen = 2;
	data_t->Payload[0]  = 0xaa;
	data_t->Payload[1]  = 0xbb;
	
	errorcode=os_q_post(pos_q_uart1,data_t);

	if(errorcode != 0)
	{
		//zc_free_mdb("chaosframe2",mdb_t->pMallocHeader);
		zc_free_mem(data_t);
	}
	*/
}
#endif
static void uart_recv_deal(uint8_t *buf, uint32_t len)
{
    uint8_t state = 0xff;
    //debug_printf(&dty, DEBUG_METER, "len : %d!\n", len);
#if defined(STATIC_MASTER)
    do
    {
        state = PTCL_Decode(1,buf , &len , FlashInfo_t.scJZQProtocol,e_UART1_MSG);
        if(state == e_Decode_Success)
            debug_printf(&dty, DEBUG_METER, "%s success!\n",e_GW13762_MSG == FlashInfo_t.scJZQProtocol?"e_GW13762_MSG":"e_GD2016_MSG");
    }
    while(state == e_Decode_Success && len != 0);
	
	do
    {
        state = PTCL_Decode(1,buf , &len , e_DLT645_MSG,e_UART1_MSG);
        if(state == e_Decode_Success)
            debug_printf(&dty, DEBUG_METER, "e_DLT645_MSG success!\n");
    }
    while(state == e_Decode_Success && len != 0);
#elif defined(STATIC_NODE)
    if(getHplcTestMode() ==  RD_CTRL)
	{
		do
	    {
	        state = PTCL_Decode(1,buf , &len , e_GW13762_MSG,e_UART1_MSG);
	        if(state == e_Decode_Success)
	            debug_printf(&dty, DEBUG_METER, "%s success!\n","e_GW13762_MSG");
	    }
	    while(state == e_Decode_Success && len != 0);
	}
	
	extern U8	SericalTransState;

	if(SericalTransState == TRUE) //透传直接Post 回去
	{
		SericalTransState = FALSE;
		recive_msg_t   SerialMsg;
        DATA_t *data_t =NULL;
        data_t=zc_malloc_mem(sizeof(DATA_t)+len,"PTCL_Decode",MEM_TIME_OUT);

        data_t->PayloadLen = len;
        __memcpy(data_t->Payload, buf, len);
        SerialMsg.process = NULL;   // Handling active 645 protocol 
        SerialMsg.Direction = 0; 

        
        SerialMsg.Protocoltype = APP_T_PROTOTYPE;
        SerialMsg.FrameSeq = 0;
        SerialMsg.FrameLen = len;
        //SerialMsg.MatchLen = 6;
        //__memcpy(SerialMsg.MatchInfo, MeterAddr, 6);
        
        
        SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
			                           
        serial_cache_del_list(SerialMsg, data_t->Payload);
        debug_printf(&dty, DEBUG_METER,"SerialMsg!SerialMsg.FrameLen %d\n",SerialMsg.FrameLen);
        zc_free_mem(data_t);
        
		return;
	}
	

    do
    {
        state = PTCL_Decode(1,buf , &len , e_DLT645_MSG,e_UART1_MSG);
        if(state == e_Decode_Success)
            debug_printf(&dty, DEBUG_METER, "e_DLT645_MSG success!\n");
    }
    while(state == e_Decode_Success && len != 0);
    do
    {
        state = PTCL_Decode(1,buf , &len , e_DLT698_MSG,e_UART1_MSG);
        if(state == e_Decode_Success)
            debug_printf(&dty, DEBUG_METER, "e_DLT698_MSG success!\n");
    }
    while(state == e_Decode_Success && len != 0);
	
#if defined(ZC3750CJQ2)
	if(state == e_Decode_Fail&&len != 0)
	{
		//chaosframe();
	}
#endif
#endif
}
uart_recv_fn  uart1_recv;

void register_uart1_recv_proc(uart_recv_fn fn)
{
    uart1_recv = fn;

    return ;
}
void uart_send(uint8_t *buffer, uint16_t len)
{
#if defined(ZC3750CJQ2)

	en485_ON();
	LED_485_ON();

	uint8_t *array=zc_malloc_mem(len+1+4,"add 0xff", MEM_TIME_OUT);
    uint8_t cnt = 4;//4
	__memcpy(array+cnt , buffer , len);
    memset(array,0Xfe,cnt);
    len += cnt;
	array[len++] = 0xFF;
	uart1_send(array,len);
	
	zc_free_mem(array);
	return;
#elif defined(ZC3750STA)

    if(getHplcTestMode() == NORM)
    {
    	uint8_t *array=zc_malloc_mem(len+4,"add 0xfe", MEM_TIME_OUT);
    	uint8_t cnt = 4;//4
    	__memcpy(array+cnt,buffer,len);
    	memset(array,0Xfe,cnt);


    if(len > 0)
    {
	    uart1_send(array,len+cnt);
    }
    else
    {
        app_printf("don't have data :");
    }

	zc_free_mem(array);
}
else
{
	//dump_buf(buffer,len);
    uart1_send(buffer, len);
}

#elif defined(STATIC_MASTER)

	//extern ostimer_t *CcoRcvUART1timeoutimer;
    if(getHplcTestMode() != RD_CTRL)//(TMR_STOPPED == zc_timer_query(CcoRcvUART1timeoutimer) && getHplcTestMode() != RD_CTRL)
	{
		//app_printf("uart1 tx :");
		//dump_buf(buffer,len);
	    uart1_send(buffer, len);
	}
	else
	{
		app_printf("PLC tx :");
		//RdCtrlGW13762(buffer,len,0,rdaddr,FlashInfo_t.ucJZQMacAddr,ctrl_tei);
	}
#endif

}

void uart1_send(uint8_t *buf, uint32_t len)
{
    meter_dev_t *dev = g_meter;
    /* dev->master_write(dev, buf, len); */
    #if defined(ZC3750CJQ2)
	//app_printf("CJQ2 tx :");
	//en485_ON();
	//LED_485_ON();
    #endif
    meter_write_tx(dev, buf, len);

    if (g_printf_state == TRUE)
    {
        app_printf("uxt<%d>:", len);
        dump_buf(buf, len);
    }

    return;
}

int32_t read_meter_addr(void)
{
    uint8_t meter_addr_req_buf[] = {0x68, 0xaa, 0xaa, 0xaa, 0xaa,
                                    0xaa, 0xaa, 0x68, 0x13, 0x00,
                                    0xdf, 0x16
                                   };
    uint8_t meter_addr_cnf_buf[256];
    char mac[MACF_LEN];
    int32_t len, i, j;

    memset(mac,0x00,MACF_LEN);
    
    g_meter->set_mode(g_meter, 0);
    g_meter->master_write(g_meter, meter_addr_req_buf, sizeof(meter_addr_req_buf));

    sys_delayms(500);

    if ((len = g_meter->master_read(g_meter, meter_addr_cnf_buf, 256)) != -EAGAIN)
    {
        for (i = 0; meter_addr_cnf_buf[i] == 0xfe; ++i)
            ;

        if (meter_addr_cnf_buf[i] == 0x68)
        {
            for (i = i + 1, j = 0; j < ETH_ALEN; ++i, ++j)
            {
                mac[ETH_ALEN - j - 1] = meter_addr_cnf_buf[i];
            }

            __memcpy(ETH_ADDR, mac, ETH_ALEN);
            //printf_s("get meter addr: %s\n", macf(mac, ETH_ADDR));
            g_meter->set_mode(g_meter, 1);
            return 0;
        }
    }

    g_meter->set_mode(g_meter, 1);
    return -1;
}
ostimer_t *RxLedtimer = NULL;
extern gpio_dev_t rxled;
/*static*/ void RxLedtimerCB(struct ostimer_s *ostimer, void *arg)
{
	#if defined(ZC3750STA)
	extern ostimer_t *sta_bind_timer;

    if(TMR_RUNNING == zc_timer_query(sta_bind_timer))
	{
		gpio_pins_on(&rxled, RXLED_CTRL);
		return;
	}
	#endif
	
	if(GetNodeState() != e_NODE_ON_LINE)
	{
		if(RXLED_CTRL&gpio_input(NULL,RXLED_CTRL))
		{
			gpio_pins_off(&rxled, RXLED_CTRL);
		}
		else
		{
			gpio_pins_on(&rxled, RXLED_CTRL);
		}
	}
	else
	{
		gpio_pins_off(&rxled, RXLED_CTRL);
	}
}

void meter_test_tx_task()
{
    uint32_t i;
    uint32_t len;
    int32_t  off;
    uint32_t ret = 0;
//    static uint8_t buf[2 * 1000];
    uint8_t *buf = zc_malloc_mem(2000, "buf", MEM_TIME_OUT);

    for (i = 0; i < 2 * 1000; i++)
        buf[i] = i;

    while (1)
    {
        os_sleep(500);

        len = 20;
        off = 0;
        while (len > 0)
        {
        	#if defined(ZC3750CJQ2)
				en485_ON();
				LED_485_ON();
			#endif
			debug_printf(&dty, DEBUG_METER, "ret prev = %d!\n",ret);
            ret = meter_write_tx(g_meter, buf + off, len);
			debug_printf(&dty, DEBUG_METER, "ret after = %d!\n",ret);
            /*
            if (ret == 0)
            	os_sleep(1);
            //printf_s("send len: %d real send len: %d\n", len, ret);
            */
            len -= ret;
            off += ret;
        }
    }
    zc_free_mem(buf);
}

void meter_task()
{
    uint32_t len;
//    static uint8_t buf[METER_BUF_SIZE];
    U8 *buf = NULL;
//    printf_s("111\n");
//    RxLedtimer = timer_create(20,
//                              3000,
//                              TMR_PERIODIC ,//TMR_TRIGGER
//                              RxLedtimerCB,
//                              NULL,
//                              "RxLedtimer"
//                             );
//	#if defined(STA_BOARD_3_0_02)

//	#else
//	timer_start(RxLedtimer);
//	#endif
    register_uart1_recv_proc(uart_recv_deal);
    os_sleep(100);
    while (1)
    {
        sem_pend(g_meter->uart_event, 0);

		debug_printf(&dty, DEBUG_METER, "u type = %d %d!\n",g_meter->uart_event->type,g_meter->uart_event->count);
        #if defined(STATIC_MASTER)
         if(getHplcTestMode() != NORM && getHplcTestMode() != RD_CTRL && getHplcTestMode() != PHYCB && 
            getHplcTestMode() != WPHYCB && getHplcTestMode() != RF_PLCPHYCB)//测试模式uart1不接收数据,抄控器模块下接收数据
		{
			continue;
		}
		#endif
        /*
		if(g_meter->uart_event->type == METER_RX_16)
		{
			g_meter->uart_event->type = METER_IDLE_STATUS;
			if(e_Decode_Fail==read_rx_expect(g_meter))
			{
				debug_printf(&dty, DEBUG_METER, "read_rx_expect e_Decode_Fail!\n");
				continue;
			}
		}
		*/
        if(g_meter->uart_event->type & METER_TX_EMPTY)
        {
            g_meter->uart_event->type &= ~METER_TX_EMPTY;
            if(SERIAL_CACHE_LIST.nr > 0)
            {
                U8 errbuf[2] = {0xcc,0xdd};
                recive_msg_t   SerialMsg;
                                                                         
                SerialMsg.Protocoltype = 0;
                SerialMsg.FrameSeq = 0;
                SerialMsg.FrameLen = sizeof(errbuf);
                SerialMsg.MatchLen = 0;
                SerialMsg.pSerialheader = &SERIAL_CACHE_LIST;
            	SerialMsg.Direction = 0;//从动
            	SerialMsg.MsgType = METER_TX_OK;
                serial_cache_del_list(SerialMsg, errbuf);
                //printf_s("m_tx_ok!\n");
                debug_printf(&dty, DEBUG_METER,"m_tx_d!\n");
            }
            continue;
            
        }
        if(g_meter->uart_event->type & METER_RX_OVER_FIFO || 
           g_meter->uart_event->type & METER_RX_TIMEOUT   || 
           g_meter->uart_event->type & METER_RX_16         )
        {
            if(SericalTransState == FALSE && e_Decode_Fail==read_rx_expect(g_meter))
            {
                debug_printf(&dty, DEBUG_METER, "read_rx_expect e_Decode_Fail!\n");
                //app_printf("wait next!\n");
                modify_clear_rx_timer(500);
                first_num = read_rx_num(g_meter);
                continue;
            }
            
            if(g_meter->uart_event->type & METER_RX_OVER_FIFO)
            {
                g_meter->uart_event->type &= ~METER_RX_OVER_FIFO;
            }
            if(g_meter->uart_event->type & METER_RX_TIMEOUT)
            {
                g_meter->uart_event->type &= ~METER_RX_TIMEOUT;
            }
            if(g_meter->uart_event->type & METER_RX_16)
            {
                g_meter->uart_event->type &= ~METER_RX_16;
            }

            buf = zc_malloc_mem(2096, "rx", MEM_TIME_OUT);
            extern msgq_t app_wq;
            if(buf != NULL && app_wq.nr < (app_wq.thd-1))
            {
                len = meter_read_rx(g_meter, buf);
                if (uart1_recv)
                {                   
                    if (g_printf_state == TRUE)
                    {
                        app_printf("uxr<%d>:",len);
                        dump_buf(buf, len);
                    }
                    uart1_recv(buf, len);
                }
                else
                {
                    app_printf("read_rx_len = %d\n", len);
                }
            }
            else
            {
                if(NULL == buf)
                {
                    app_printf("malloc error!\n");
                }
                app_printf("app_wq.nr=%d, thd=%d\n", app_wq.nr, app_wq.thd);
            }

            if(buf)
            {
                zc_free_mem(buf);
            }
        }
        else if(g_meter->uart_event->type & METER_RX_TOGETHER)
        {
            g_meter->uart_event->type &= ~METER_RX_TOGETHER;
            cur_num = read_rx_num(g_meter);
            if(cur_num == first_num)
            {
                read_rx_clear(g_meter);
            }
        }
		
    }
}

#define TASK_TX_STK_SZ  512
#define TASK_PRIO_METER 49
uint32_t meter_task_work[TASK_TX_STK_SZ];
int32_t pid_meter_task = 0;
event_t gUart_evert;
void meter_task_init()
{
//    if (!(g_meter->uart_event = sem_create(0, "g_meter->uart_event")))
//        sys_panic("<meter_task_init event panic> %s: %d\n", __func__, __LINE__);
    sem_init(&gUart_evert, 0, "uart_event");
    g_meter->uart_event = &gUart_evert;
		
    meter_set_intr_mode(g_meter);

	if ((pid_meter_task = task_create(meter_task,
                                NULL,
                                &meter_task_work[TASK_TX_STK_SZ - 1],
                                TASK_PRIO_METER,
                                "meter_task",
                                &meter_task_work[0],
                                TASK_TX_STK_SZ)) < 0)
        sys_panic("<meter_task_init panic> %s: %d\n", __func__, __LINE__);
}
//#if defined(ZC3750STA) || defined(ZC3750CJQ2)
void MeterUartInit(meter_dev_t *dev, U32 Baud)
{
	uint32_t lcr_val;
	uint8_t uart = dev->uart;

	g_meter->baudrate = Baud;
	uart_set_int_enable(uart, 0); //disable write and read intr
	#if defined(HIGH_FREQUENCY)
    uart_init(uart, Baud,PARITY_NONE,STOPBITS_1,DATABITS_8);
	#else
    uart_init(uart, Baud,PARITY_EVEN,STOPBITS_1,DATABITS_8);
	#endif

	lcr_val = uart_get_line_ctrl(uart);
	
	#if defined(HIGH_FREQUENCY)
    lcr_val &= ~LCR_PARITY_ENABLE;
	#else
	lcr_val |= LCR_EVEN_PARITY | LCR_PARITY_ENABLE;
	#endif
	
	uart_set_line_ctrl(uart, lcr_val);

    uart_set_rx_fifo_threshold(dev->uart, dev->rx_fifo_rwm);
    uart_set_tx_fifo_threshold(dev->uart, dev->tx_fifo_twm);
    uart_set_rx_fifo_non_empty_time(dev->uart, 22000000 / dev->baudrate); //2 * 33bit /* 2048 * 3us */
    //debug_printf(&dty, DEBUG_PHY, "88000000/dev->baudrate : %d\n",22000000/dev->baudrate);
    U32 cnt = uart_get_rf_cnt(dev->uart);
	while(cnt--)
	{
		uart_get_rxbuf(dev->uart);
	}
    uart_set_int_enable(dev->uart, IER_RDA | IER_RLS | IER_RTO);//
	return ;
   
}
//#endif
void meter_set_mode(meter_dev_t *dev, uint8_t mode)
{
    uint8_t uart = dev->uart;
    uint8_t irq  = dev->irq;

    if (dev->mode == mode)
    {
        debug_printf(&dty, DEBUG_METER, "meter current mode: %d\n", mode);
        return;
    }

    dev->mode = mode;
    debug_printf(&dty, DEBUG_METER, "set meter mode: %d irq:%d\n", mode, irq);

    if (mode)
    {
        meter_set_intr_mode(dev);
    }
    else
    {
        uart_set_int_enable(uart, 0);
        release_irq(irq);
    }
}

int convert_buf(uint8_t *buf, uint8_t *cmd, int len)
{
    uint8_t high, low;
    uint8_t flag = 1;
    int ret = 0;

    while (len--)
    {
        if (*cmd == '\"' || *cmd == ' ')
        {
            cmd++;
            continue;
        }

        if (flag)
        {
            high = (*cmd >= 'a') ? (*cmd++ - 0x57) :
                   (*cmd >= 'A') ? (*cmd++ - 0x37) : (*cmd++ - '0');
            flag = 0;
        }
        else
        {
            low = (*cmd >= 'a') ? (*cmd++ - 0x57) :
                  (*cmd >= 'A') ? (*cmd++ - 0x37) : (*cmd++ - '0');
            *buf++ = (high << 4 | low);
            flag = 1;
            ret++;
        }
    }

    return ret;
}



static void dump_debug_display(tty_t *term)
{
    term->op->tprintf(term, "baudrate:    %d\n\n", g_meter->baudrate);

    term->op->tprintf(term, "tx:          %10d      rx:         %10d\n", g_meter->debug_tx, g_meter->debug_rx);
    term->op->tprintf(term, "tx_empty:    %10d      rx_timeout: %10d\n", g_meter->debug_tx_empty, g_meter->debug_rx_timeout);
    term->op->tprintf(term, "tx fifo td:  %10d      rx_err:     %10d\n", g_meter->debug_tx_twm, g_meter->debug_rx_err);
    term->op->tprintf(term, "tx full      %10d      rx_full:    %10d\n", g_meter->debug_tx_full, g_meter->debug_rx_full);
    term->op->tprintf(term, "patch        %10d                      \n", g_meter->debug_patch);
    term->op->tprintf(term, "\n");

    term->op->tprintf(term, "tx total     %10d      rx total:   %10d\n", g_meter->tx_total, g_meter->rx_total);
    term->op->tprintf(term, "tx buf send  %10d      rx buf recv:%10d\n", g_meter->send, g_meter->recv);
    term->op->tprintf(term, "tx buf write %10d      rx buf read:%10d\n", g_meter->write, g_meter->read);
}

param_t cmd_meter_param_tab[] =
{
    {
        PARAM_ARG | PARAM_TOGGLE | PARAM_OPTIONAL, "", "baud"
    },
    
	{	
		PARAM_ARG | PARAM_INTEGER | PARAM_SUB, "baud", "", "baud",
	},
    PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_meter_desc,
               "Meter App Command", cmd_meter_param_tab
               );





 

void docmd_meter(command_t *cmd, xsh_t *xsh)
{
    tty_t *term = xsh->term;


    if (xsh->argc == 1)
    {
        dump_debug_display(xsh->term);
        return;
    }
	else if (__strcmp(xsh->argv[1], "baud") == 0)
	{
		U32 baud = BAUDRATE_9600;
		baud=__atoi(xsh->argv[2]);
        MeterUartInit(g_meter,baud);
		term->op->tprintf(term, "baud: %d\n", baud);
	}
    return;
}
