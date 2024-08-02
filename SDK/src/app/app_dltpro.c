/*
 * Copyright: (c) 2009-2020, 2020 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	app_dltpro.c
 * Purpose:	
 * History:
 * Author : WWJ
 */
#include "softdog.h"
#include "wdt.h"
#include "ZCsystem.h"
#include "app_base_serial_cache_queue.h"
#include "aps_layer.h"
#include "app_exstate_verify.h"
#include "app_698p45.h"
#include "app_698client.h"
#include "app_power_onoff.h"
#include "app_meter_verify.h"
#include "app_rdctrl.h"
#include "app_meter_common.h"
#include "app_meter_bind_sta.h"
#include "Scanbandmange.h"
#include "SchTask.h"
#include "dl_mgm_msg.h"
#include "dev_manager.h"
#include "app_dltpro.h"
#include "printf_zc.h"
#include "Version.h"
#include "netnnib.h"
#include "meter.h"
#include "wlc.h"
#include "ProtocolProc.h"
#include "app_clock_sync_sta.h"
#include "Datalinkdebug.h"
#include "SchTask.h"
#include "app_read_cjq_list_sta.h"

U8 FactoryTestFlag = 0;
ostimer_t *hrf_cal_ctrltimer = NULL;
ostimer_t *calibration_report_timer = NULL;
/*************************************************************************
 * 函数名称 :	void bcd_to_bin(U8 *src, OUT U8 *dest, U8 len)
 * 函数说明 :	BCD转化成BIN
 * 参数说明 :	src - BCD码
 *					dest	- BIN码
 *					len - 长度
 * 返回值		:	
 *************************************************************************/

void bcd_to_bin(U8 *src, U8 *dest, U8 len)
{
    U8 i;

    for(i = 0 ; i < len ; i++)
    {
        *(dest + i) = (*(src + i) >> 4) * 10 + (*(src + i) & 0x0F);
    }
}
/*************************************************************************
 * 函数名称 :	void bin_to_bcd(U8 *src, OUT U8 *dest, U8 len)
 * 函数说明 :	BIN转化成BCD
 * 参数说明 :	src - BIN码
 *					dest	- BCD码
 *					len - 长度
 * 返回值		:	
 *************************************************************************/

void bin_to_bcd(U8 *src, U8 *dest, U8 len)
{
    U8 i;

    for(i = 0 ; i < len ; i++)
    {
        *(dest + i) = (((*(src + i) ) /10)<<4) + (*(src + i) %10);
    }
}



/*************************************************************************
 * 函数名称	: 	check_sum(U8 *buf, U16 len)
 * 函数说明	: 	校验和计算函数
 * 参数说明	: 	buf	- 要计算的数据的起始地址
 * 					len	- 要计算的数据的长度
 * 返回值		: 	校验和
 *************************************************************************/
U8 check_sum(U8 *buf, U16 len)
{
    U16 i;
    U8 ret = 0;

    for(i = 0 ; i < len ; i++)
    {
        ret += buf[i];
    }
    
    return ret;
}


static const U16 fcstab[] = {
0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
};


U16 ResetCRC16(void)
{
    return 0xffff ;
}
		
/*************************************************************************
 * 函数名称	: 	CalCRC16(U16 CRC16Value,U8 *data ,int StartPos, int len)
 * 函数说明	: 	CRC16计算函数(CRC16_X25,初始值0xFFFF,x16+x12+x5+1)
 * 参数说明	: 	CRC16Value      - 要计算CRC16的初始值
 * 			   *data        - 计算CRC16的入参
 *             StartPos     - 偏移长度
 *             len          - 实际计算长度
 * 返回值		:  计算结果
 *************************************************************************/
U16 CalCRC16(U16 CRC16Value,U8 *data ,int StartPos, int len)
{
	int i = 0;
    
    for (i = StartPos; i < (len+StartPos); i++)
    {
        CRC16Value = (CRC16Value >> 8) ^ fcstab[(CRC16Value ^ data[i]) & 0xff];
    }
    
    return ~CRC16Value & 0xffff;
}

 unsigned int crc_table[256];
 
/*************************************************************************
 * 函数名称	: 	InitCRC32Table(void)
 * 函数说明	: 	初始化CRC32表
 * 参数说明	: 	无
 * 返回值		:  无
 *************************************************************************/
void InitCRC32Table(void)
{
	unsigned int c;
	unsigned int i, j;
	
	for (i = 0; i < 256; i++) 
    {
		c = (unsigned int)i;
		for (j = 0; j < 8; j++) 
        {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[i] = c;
	}
}
 
/*************************************************************************
 * 函数名称	: 	CalCRC32(unsigned int crc,unsigned char *buffer, unsigned int size)
 * 函数说明	: 	校验和计算函数
 * 参数说明	: 	crc             - 要计算CRC32的初始值
 * 			   *data        - 计算CRC32的入参
 *             len          - 实际计算长度
 * 返回值		:  CRC32计算结果
 *************************************************************************/
unsigned int CalCRC32(unsigned int crc,unsigned char *data, unsigned int len)
{
	unsigned int i;
    
	for (i = 0; i < len; i++) 
    {
		crc = crc_table[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
	}
    
	return crc ;
}
 


/*************************************************************************
 * 函数名称	: 	Check645Frame(unsigned char *buff, U16 len,U8 *FEnum,U8 *addr, U8 *ctrl)
 * 函数说明	: 	校验645协议
 * 参数说明	: 	*buff           - 校验数据
 *             len          - 数据长度
 * 			   *FEnum       - FE数量
 *             *addr        - 解析后表地址
 *             *ctrl        - 控制码
 * 返回值		:  解析结果
 *************************************************************************/
unsigned char Check645Frame(unsigned char *buff, U16 len,U8 *FEnum,U8 *addr, U8 *ctrl)
{
    unsigned char   i;
    volatile unsigned char   BaseIndex;
    volatile unsigned char   dataLen;
    unsigned char   sumIndex;
    unsigned char   rcvSum;
    unsigned char   Sum;

    BaseIndex = 0;

    for(i=0; i<13; i++)
    {
        if(*(buff + i) == 0x68)
        {
            BaseIndex = i;
            break;
        }
    }
    if(FEnum)
    {
        *FEnum = BaseIndex;
    }
    if((len-BaseIndex) < 12)
    {
        app_printf("BaseIndex = %d!\n",BaseIndex);
        return FALSE;
    }
	
    if((i > 12) || 
       (*(buff + BaseIndex)!= 0x68) || 
       (*(buff + BaseIndex + 7) != 0x68))   //   ||  ((*(buff + BaseIndex + 8)&0x80) != 0x80)
    {
        return FALSE;
    }

	if(ctrl)
	{
		*ctrl = *(buff + BaseIndex + 8);
	}
	
    dataLen = *(buff + BaseIndex + 9);
    
    if(buff[BaseIndex+10+dataLen+1] != 0x16)
    {
        return FALSE;
    }
    
    
    //Sum = 0;
    Sum = check_sum(&buff[BaseIndex],dataLen+10);
    //for(i=0; i<dataLen+10; i++)
    //{
       // Sum += *(buff + BaseIndex + i);
    //}
    sumIndex = BaseIndex+dataLen+10;
    
    //rcvSum = buff[BaseIndex+DataLen+10];
    rcvSum = buff[sumIndex];

    if(Sum != rcvSum)
    {
        
        app_printf("Sum %d ,rcvSum %d err!\n",Sum,rcvSum);
        return FALSE;
    }
    
    //__memcpy(DstMeterAddr, &buff[BaseIndex+1], 6);
    if(addr)
	{
		__memcpy(addr , buff + BaseIndex + 1,6);
	}
    
    return TRUE;
}

U8 Check645TypeByCtrlCode(U8 CtrlCode)
{
    Dlt645CtrlField_t Code;
    __memcpy(&Code, (Dlt645CtrlField_t *)&CtrlCode, 1);
    U8  Protocol = 0;
    switch(Code.CtrlCode)
    {
        //07-645 D4～D0 功能码

        case 0b01000 : //广播校时 广播校时，暂时固定按照07-645，因为STA不会从串口收到
        case 0b10001 : //读数据
        case 0b10010 : //读后续数据
        case 0b10011 : //读通信地址
        case 0b10100 : //写数据
        case 0b10101 : //写通信地址
        case 0b10110 : //冻结命令
        case 0b10111 : //更改通信速率
        case 0b11000 : //修改密码
        case 0b11001 : //最大需量清零
        case 0b11010 : //电表清零
        case 0b11011 : //事件清零
        	Protocol = DLT645_2007; 
        	break;
        
        //97-645 D4～D0 : 请求及应答功能码

        case 0b00001 : //读数据
        case 0b00010 : //读后续数据
        case 0b00011 : //重读数据
        case 0b00100 : //写数据
        //case 0b01000 : //广播校时
        case 0b01010 : //写设备地址
        case 0b01100 : //更改通信速率
        case 0b01111 : //修改密码
        case 0b10000 : //最大需量清零
            Protocol = DLT645_1997; 
            break;
        default:
            Protocol = 0; 
            break;
    }
    return Protocol;
}

/*************************************************************************
 * 函数名称	: 	Check698Frame(unsigned char *buff, U16 len,U8 *addr)
 * 函数说明	: 	校验698协议
 * 参数说明	: 	*buff           - 校验数据
 *             len          - 数据长度
 * 			   *FEnum       - FE数量
 *             *addr        - 解析后表地址
 * 返回值		:  解析结果
 *************************************************************************/
unsigned char Check698Frame(unsigned char *buff, U16 len,U8 *FEnum,U8 *addr, U8* logicaddr)
{
    U16  i=0;
    U16  BaseIndex=0;
    U16  dataLen=0;
    U16  cs16=0;
    U16  CRC16Cal = 0;
    U8	 Addrpos = 0;
    U8	 BoardAddrlen_698 = 1;
    dl698frame_header_s frame_698_head_t;
	
    for(i=0; i<len; i++)
    {
        if(*(buff + i) == 0x68)
        {
            BaseIndex = i;
            break;
        }
    }
    
	if(FEnum)
    {
        *FEnum = BaseIndex;
    }
    
	if(len-BaseIndex < 12)
    {
        return FALSE;
    }
	
    dataLen = (*(buff + BaseIndex + 1) | (*(buff + BaseIndex + 1 + 1) << 8)) + 2;

	//app_printf("repect : 0x%x  datalen : %d\n",buff[BaseIndex+dataLen-1],dataLen);

    if(buff[BaseIndex+dataLen-1] != 0x16)
    {
        return FALSE;
    }

    cs16 = (*(buff + BaseIndex + dataLen - 2)<<8) | (*(buff + BaseIndex + dataLen - 3));
	//app_printf("cs16 : 0x%04x\n",cs16);

	
	CRC16Cal = ResetCRC16();
    if(cs16 != CalCRC16(CRC16Cal,buff + BaseIndex + 1, 0 , dataLen - 4))
    {
		app_printf("cs16 error\n");
        return FALSE;
    }

    //addr_info_t = buff[BaseIndex + 4];
    __memcpy(&frame_698_head_t, buff + BaseIndex, sizeof(frame_698_head_t));
    //Addrlen = (buff[BaseIndex + 4]&0x0f) + 1;
    //app_printf("Addrlen =%d\n",Addrlen);
    //app_printf("AddrLen	%d\n",frame_698_head_t.SA.AddrLen);
    //app_printf("LogicAddr %d\n",frame_698_head_t.SA.LogicAddr);
    //app_printf("AddrType	%d\n",frame_698_head_t.SA.AddrType);

    Addrpos = ((frame_698_head_t.SA.AddrLen + 1) == BoardAddrlen_698 ? FRAME_698_ADD_POS : frame_698_head_t.SA.AddrLen);

    if(logicaddr)
    {
        if((frame_698_head_t.SA.LogicAddr & 0x02) == 0x02)
        {
            *logicaddr = buff[BaseIndex + FRAME_698_ADD_POS];
        }
        else
        {
            *logicaddr = frame_698_head_t.SA.LogicAddr & 0x01;
        }
    }

    if(addr)
	{
        __memcpy(addr, buff + BaseIndex + Addrpos,
            frame_698_head_t.SA.AddrLen + 1 > MACADRDR_LENGTH ? MACADRDR_LENGTH : frame_698_head_t.SA.AddrLen + 1);
	}
	
    return TRUE;
}
/*************************************************************************
 * 函数名称	: 	Check3762Frame(unsigned char *buff, U16 len)
 * 函数说明	: 	校验645协议
 * 参数说明	: 	*buff           - 校验数据
 *             len          - 数据长度
 * 			   
 * 返回值		:  解析结果
 *************************************************************************/
unsigned char Check3762Frame(unsigned char *buff, U16 len)
{
    U16   dataLen;
    U16   rcvSum;
    U16   Sum;


    if(buff[0] != 0x68)
    {
        app_printf("head = %d!\n",*buff);
        return FALSE;
    }

    dataLen = buff[1] | (buff[2] << 8);
    
    if(buff[1 + 2 + dataLen + 1] != 0x16)
    {
        return FALSE;
    }
    
    Sum = check_sum(&buff[3],dataLen);
    rcvSum = buff[1 + 2 + dataLen];

    if(Sum != rcvSum)
    {
        app_printf("Sum %d ,rcvSum %d err!\n",Sum,rcvSum);
        return FALSE;
    }
    
    return TRUE;
}


U8 ScaleDLT(U8 Protocol,U8 *buf , U16 *offset,U16 *remainlen , U16 *len)
{
    U16 framelen; 
    U8  framelenpos;
	U16 pos = 0;

    framelen = 0;
    framelenpos = 0;
    pos = 0;
    
    if(DLT645_2007 == Protocol || DLT645_1997 == Protocol)
    {
        framelenpos = 9;
    }
    else if(DLT698 == Protocol || GW3762 == Protocol || GD2016 == Protocol)
    {
        framelenpos = 1;
    }
    else
    {
        *offset = 0;
        *len = 0;
        return e_Decode_Fail;
    }
    

    if(*remainlen < 12)
    {
        *offset = 0;
        *len = 0;
        return e_Decode_Fail;
    }
        
    do
    {
        switch (*(buf + pos))
        {
        case 0x68:
        {
			framelen = (DLT645_2007 == Protocol || DLT645_1997 == Protocol)?*(buf + pos + framelenpos) + 12:
				(GW3762 == Protocol||GD2016 == Protocol)?*(buf + pos + framelenpos) | (*(buf + pos + framelenpos + 1) << 8):
				(DLT698 == Protocol)?(*(buf + pos + framelenpos) | (*(buf + pos + framelenpos + 1) << 8)) + 2:0;
            if(((DLT645_2007 == Protocol || DLT645_1997 == Protocol) && framelen > 255) || 
                (DLT698 == Protocol && framelen > 2048) ||
                ((GW3762 == Protocol || GD2016 == Protocol) && framelen > 2048))
            {
                //app_printf("buflen %d > 2096\n",framelen);
                pos++;
                break;
            }
            if((*remainlen - pos) < framelen) //帧不完整
            {
                //app_printf("buflen %d err\n",framelen);
                pos++;
                break;
            }
            
			//解决68扰码
            if(*(buf + pos + framelen - 1) != 0x16)
            {
                //app_printf("bufend %d err\n",*(buf + pos + framelen - 1));
                pos++;
                break;
            }
			//645协议判定第二个68
            if(DLT645_2007 == Protocol || DLT645_1997 == Protocol)
            {
                if(*(buf + pos + 7)!=0x68)
                {
                    //app_printf("Protocol = %d \n" , Protocol);
                    pos++;
                    break;
                }
            }
            //判断帧尾
            if(*(buf + pos + framelen - 1) == 0x16)
            {
                //判断校验
				//判定1376.2校验和，GD2016校验和
                if(GW3762 == Protocol || GD2016 == Protocol)
                {
                    if(*(buf + pos + framelen - 2) != check_sum((buf + pos +3) , framelen - 3 - 2))
                    {
                        //app_printf("sum = %02x,check_sum = %02x err\n", *(buf + pos + framelen - 2) , check_sum((buf + pos) , framelen-2));
                        pos++;
                        break;
                    }
                }
                //判定645校验和
                if(DLT645_2007 == Protocol || DLT645_1997 == Protocol)
                {
                    if(*(buf + pos + framelen - 2) !=check_sum((buf + pos) , framelen - 2))
                    {
                        //app_printf("sum = %02x,check_sum = %02x err\n", *(buf + pos + framelen - 2) , check_sum((buf + pos) , framelen-2));
                        pos++;
                        break;
                    }
                }
                //判定698 CRC16校验
                if(DLT698 == Protocol)
                {
                    U16  cs16;
                    U16  CRC16Cal;
                    cs16 = (*(buf + pos + framelen -3) | (*(buf + pos + framelen -2) << 8));
                    CRC16Cal = ResetCRC16();
                    if(cs16 != CalCRC16(CRC16Cal,(buf + pos +1) , 0 , framelen-4))
                    {
                        //app_printf("cs16 = %04x ,CRC16Cal = %04x err\n", cs16 , CRC16Cal);
                        pos++;
                        break;
                    }
                }
                
				 
				*offset = pos;
                *len = framelen;
                if((pos + framelen) < *remainlen) //如果后续还有数据
                {
                    //app_printf("len : %d\n pos+framelen : %d\n", *remainlen , pos + framelen);
                    *remainlen = *remainlen - (pos + framelen);
                    pos = 0;
                }
                else
                {
                    *remainlen = 0;
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
    while(pos < *remainlen);

    *offset = pos;
    *len = 0;
    *remainlen = 0;

    return e_Decode_Fail;
    
}

U8  Check645FrameNum(U8 *buf, U16 len)
{
    U8  readMeterNum;
    U16 offset;
    U16 onelen;
    U16 remainlen;
    U8 *pdata;

    pdata = buf;
    offset = 0;
    readMeterNum = 0;
    onelen = 0;
    remainlen = len;
#if defined(STATIC_NODE)
    U8 addr_temp[6] = {0};
#endif
#if defined(ZC3750STA)
    U8 sta_mac_addr[6] = {0};
    GetMacAddr(sta_mac_addr);
#endif

    do
    {
        if(e_Decode_Success ==ScaleDLT(DLT645_2007,pdata,&offset, &remainlen,&onelen))
        {
#if defined(ZC3750STA)
            if ((TRUE == Check645Frame(pdata + offset, onelen, NULL, addr_temp, NULL)) && 
#ifdef TH2CJQ
                ((0 == memcmp(sta_mac_addr, addr_temp, 6)) || (TRUE == TH2CJQ_CheckMeterExistByAddr(addr_temp, NULL)))
#else
                (0 == memcmp(sta_mac_addr, addr_temp, 6))
#endif
            )
#elif defined(ZC3750CJQ2)
            if ((TRUE == Check645Frame(pdata + offset, onelen, NULL, addr_temp, NULL)) && TRUE == CJQ_CheckMeterExistByAddr(addr_temp,NULL,NULL))
#endif
            {
                readMeterNum++;
            }
            pdata += offset;    //加偏移
            pdata += onelen;    //加帧长

        }
        else 
    	{
    	    if(remainlen > 0)
            {   
        		pdata += 1;
                remainlen --;
            }
    	}
    }while(0 != remainlen);

    return readMeterNum;
}

/*************************************************************************
 * 函数名称	: 	CheckT188Frame(unsigned char *buff, U16 len,U8 *addr)
 * 函数说明	: 	校验T188协议
 * 参数说明	: 	*buff           - 校验数据
 *             len          - 数据长度
 *             *addr        - 解析后表地址
 * 返回值		:  解析结果
 *************************************************************************/
unsigned char CheckT188Frame(unsigned char *buff, U16 len,U8 *addr)
{
    unsigned char   i = 0;
    volatile unsigned char   BaseIndex = 0;
    volatile unsigned char   dataLen = 0;
    unsigned char   sumIndex = 0;
    unsigned char   rcvSum;

    unsigned char   Sum;
	//app_printf("check188frame : ");
    //dump_buf(buff,len);
    //此处用13 不用len，若用len的话会死循环
    for(i=0; i<13; i++)
    {
        if(*(buff + i) == 0x68)
        {
            BaseIndex = i;
            break;
        }
    }

    if(len-BaseIndex < 12)
    {
		app_printf(" len-BaseIndex : %d\n",len-BaseIndex);
        return FALSE;
    }
	
    dataLen =*(buff + BaseIndex + 10)+10+3;
	
    if( (*(buff + BaseIndex	+ dataLen - 1) != 0x16) )
    {
		app_printf("0x16 respect : 0x%02x \n",*(buff + BaseIndex	+ dataLen - 1));
        return FALSE;
    }
    
    //Sum = 0;
    //for(i=0; i<dataLen-2; i++)
    //{
		//app_printf("*(buff + BaseIndex + %d) : %x\n",i,*(buff + BaseIndex + i));
        //Sum += *(buff + BaseIndex + i);
    //}
    Sum = check_sum((buff + BaseIndex),dataLen-2);
    sumIndex = BaseIndex+dataLen-2;
    
    rcvSum = buff[sumIndex];

    if(Sum != rcvSum)
    {
		app_printf("Sum : 0x%02x  rcvSum : 0x%02x \n",Sum,rcvSum);
        return FALSE;
    }

	if(addr)
	{
		__memcpy(addr , buff + BaseIndex + 2,7);
		//app_printf("T188 addr :");
		//dump_buf(addr,7);
	}
    return TRUE;
}

/*************************************************************************
 * 函数名称	: 	Add0x33ForData(U8 *buf,U16 buflen)
 * 函数说明	: 	数据添加0x33
 * 参数说明	: 	*buf            - 校验数据
 *             buflen       - 数据长度
 * 返回值		:  无
 *************************************************************************/
void Add0x33ForData(U8 *buf,U16 buflen)
{
    U16 ii = 0;
    for(ii = 0;ii < buflen;ii++)
    {
        buf[ii] += 0x33;
    }
}

/*************************************************************************
 * 函数名称	: 	Sub0x33ForData(U8 *buf,U16 buflen)
 * 函数说明	: 	数据减去0x33
 * 参数说明	: 	*buf            - 校验数据
 *             buflen       - 数据长度
 * 返回值		:  无
 *************************************************************************/
void Sub0x33ForData(U8 *buf,U16 buflen)
{
    U16 ii = 0;
    for(ii = 0;ii < buflen;ii++)
    {
        buf[ii] -= 0x33;
    }
}
/*************************************************************************
 * 函数名称	: 	Sub0x33ForData(U8 *buf,U16 buflen)
 * 函数说明	: 	true 地址是BCD false 地址不是BCD
 * 参数说明	: 	*buf            - 校验数据
 *             len          - 数据长度
 * 返回值		:  无
 *************************************************************************/
U8 FuncJudgeBCD(U8 *buf, U8 len)
{
	U8 tempchar = 0;
	U8 templen = len;
	
	while(templen)
	{
		tempchar = *buf++;
		if(tempchar >> 4 >= 10 || (tempchar & 0xF) >= 10)
		{
			return FALSE;
		}
		templen--;
	}
	return TRUE;
}

/*************************************************************************
 * 函数名称	: 	PacketDLT645Frame(U8 *pData,U8 *len,U8 *addr,U8 *logo ,U8 logolen)
 * 函数说明	: 	645协议组帧
 * 参数说明	: 	*pData          - 合并数据
 *             len          - 合并数据长度
 *             *addr        - 表地址
 *             ctrl         - 控制码
 *             *logo        - 数据内容
 *             *logolen     - 数据内容长度
 * 返回值		:  无
 *************************************************************************/
void Packet645Frame(U8 *pData,U8 *len,U8 *addr,U8 ctrl,U8 *logo ,U8 logolen)
{    
	U8 pos=0; 
    
	*pData = 0x68; //起始字符
	pos += 1;
	*len += 1;
	__memcpy(pData+pos,addr,6);
	pos += 6;
	*len += 6;
	*(pData+pos)=0x68;//分界符
	pos += 1;
	*len += 1;
    *(pData+pos)=ctrl;//控制码
    pos += 1;
	*len += 1;
    *(pData+pos)=logolen;//数据长度
    pos += 1;
	*len += 1;
	__memcpy(pData+pos,logo,logolen);//数据标识内容
	pos += logolen;
	*len += logolen;
	*(pData+pos)=check_sum(pData,*len);
	pos += 1;
	*len += 1;
	*(pData+pos)=0x16;
	pos += 1;
	*len += 1;
}
//校验698数据帧
void Verify698Frame(U8 *pData,	U16 FrameLen,U8 feCnt)
{
	 U16 crc16 = 0;
	 U8 i =0;
	 crc16 = ResetCRC16();
	 crc16 = CalCRC16(crc16,&pData[feCnt],1,FrameLen-2);

	 pData[feCnt+FrameLen-1] = (U8)(crc16 & 0x00FF);
	 pData[feCnt+FrameLen] = (crc16 & 0xFF00) >> 8;
	 for(i=0;i<FrameLen+2;i++)
	 	pData[i] = pData[feCnt+i];
}
/*************************************************************************
 * 函数名称	: 	PacketDLT698Frame(U8 *pData,U8 *len,U8 *addr,U8 *logo ,U8 logolen)
 * 函数说明	: 	645协议组帧
 * 参数说明	: 	*pData          - 合并数据
 *             len          - 合并数据长度
 *             *addr        - 表地址
 *             ctrl         - 控制码
 *             *logo        - 数据内容
 *             *logolen     - 数据内容长度
 * 返回值		:  无
 *************************************************************************/
void Packet698Frame(U8 *pData,U16 *pDatalen,U8 ctrl,U8 addrtype,U8 addrlen,U8 *addr,U8 *logo ,U16 logolen, U8 CusAddr)
{    
	if(NULL == pData)
		return;
	
	U8  pos=0; 
    U16 len = 0;
    U16 crc16;
    U16 HeadCRCLen = 0;
    //U16 TailCRCPos = 0;

    len = 2+1+1+addrlen+1+2+logolen+2;
	if(pDatalen)
    	*pDatalen = len;
	*pData = 0x68; //起始字符
	pos += 1;
    
    //长度域L
	*(pData+pos) = (len)&0xFF; 
	pos += 1;
    *(pData+pos) = (len>>8)&0xFF; 
	pos += 1;
    
    *(pData+pos) = ctrl; //控制域C
	pos += 1;
    
    *(pData+pos) = ((addrlen-1)&0x07)|((addrtype<<6)&0xC0); //地址标识AF
	pos += 1;

	if(addr)
		__memcpy(pData+pos,addr,addrlen);
	
	pos += addrlen;
    
    *(pData+pos) = CusAddr; //客户机地址CA=0，不关注
	pos += 1;

    crc16 = ResetCRC16();
    HeadCRCLen = (2+1+1+addrlen+1);
    crc16 = CalCRC16(crc16,pData,1,HeadCRCLen);
	*(pData+pos) = crc16 & 0x00FF;
    pos += 1;
    *(pData+pos) = (crc16 & 0xFF00) >> 8;    
	pos += 1;

	if(logo)
		__memcpy(pData+pos,logo,logolen);//数据标识内容
	pos += logolen;
    
	crc16 = ResetCRC16();
    HeadCRCLen = logolen+HeadCRCLen+2;
    crc16 = CalCRC16(crc16,pData,1,HeadCRCLen);
	*(pData+pos) = crc16 & 0x00FF;
    pos += 1;
    *(pData+pos) = (crc16 & 0xFF00) >> 8;    
	pos += 1;

	*(pData+pos)=0x16;
	pos += 1;
    *pDatalen = pos;
}

/*************************************************************************
 * 函数名称	: 	packetT188frame(U8 T,U8 *addr,U8 addr_len,U8 ctrl_0,U8 datalen,U8 *data,U8 *frame,U16 *framelen)
 * 函数说明	: 	T188组帧函数
 * 参数说明	: 	T              - 协议 
 *             *addr      - 地址
 *             addr_len   - 地址长度
 *             ctrl_0     - 控制码
 *             datalen    - 数据长度
 *             *data      - 数据
 *             *frame	   - 合并后的数据的起始地址
 * 			   *framelen  - 合并后的数据的长度
 * 返回值		: 	结果码
 *************************************************************************/
U8  PacketT188Frame(U8 T,U8 *addr,U8 addr_len,U8 ctrl_0,U8 datalen,U8 *data,U8 *frame,U16 *framelen)
{
	if(frame==NULL||addr==NULL)
	{
		return FALSE;
	}
	if(addr_len<6)return FALSE;

	*framelen=0;
		
	//H
	frame[*framelen]=0x68;
	(*framelen) += 1;
	//T
	frame[*framelen]=T;
	(*framelen) += 1;
	//A
	__memcpy(&frame[*framelen],addr,addr_len);
	(*framelen) += addr_len;
	
	if(addr_len==6)
	{
		frame[*framelen]=0;
		(*framelen) += 1;
	}
	//C
	frame[*framelen]=ctrl_0;
	(*framelen) += 1;
	//L
	frame[*framelen]=datalen;
	(*framelen) += 1;
	//DTATA
	__memcpy(&frame[*framelen],data,datalen);
	(*framelen) += datalen;
	//CS
	frame[*framelen]=check_sum(frame, *framelen);
	(*framelen) += 1;
	//Tail
	frame[*framelen]=0x16;
	(*framelen) += 1;

	return OK;
}
/*
void  GetInfoFrom645Frame(U8 *pdata , U16 datalen ,U8 *addr  ,U8 *controlcode , U8 *len , U8 *FENum)
{    
    U8 FECount = 0;
    
    while(pdata[FECount] == 0xFE)
    {
        FECount++;
    }

    __memcpy(addr , pdata+FECount+1 , 6);
    *len         = pdata[FECount + 9];
    *controlcode = pdata[FECount + 8];
    *FENum      = FECount;
    return;
}
*/

/*************************************************************************
 * 函数名称	: 	GetValueBy645Frame(U8 *id_log,U8 *buf ,U16 Len,U8 *ValueBufLen)
 * 函数说明	: 	校验645协议
 * 参数说明	: 	*id_log         - 数据标识
 *             *buf         - 上行数据
 *             *len         - 上行数据长度
 *             *ValueBufLen - 有效数据长度
 *             *logo        - 数据内容
 *             *logolen     - 数据内容长度
 * 返回值		:  解析正常/异常
 *************************************************************************/
U8 GetValueBy645Frame(U8 *id_log,U8 *buf ,U16 Len,U8 *ValueBuf,U8 *ValueBufLen)
{
	U16 id_len = 0;

    //FE
	while(*buf != 0x68)
	{
		buf++;
		Len--;
	}
    //len legal
	if(Len<10)
	{
		return FALSE;
	}
			 
	buf += 1;//0x68
	
	buf += 6;//addr

	buf += 1;//0x68
	
	buf += 1;//ctrlcode
	id_len = *buf;//item+datalen
	buf += 1;//item start
	
	if(0 != memcmp(id_log,buf,4))
	{
		app_printf("id_log-> ");
		dump_buf(id_log,4);
		
		return FALSE;
	}
    
	buf += 4;//data start
	if(ValueBuf)
    {   
	    __memcpy(ValueBuf,buf,id_len-4);//copy data to sub 0x33
    }
    else
    {
        return FALSE;
    }
    if(ValueBufLen)
    {
        *ValueBufLen = id_len-4;
    }
    else
    {
        return FALSE;
    }
    
    Sub0x33ForData(ValueBuf, *ValueBufLen);
    
	//dump_buf(ValueBuf,*ValueBufLen);
	return 	TRUE;
}


/*
int8_t TestStateCheckFunc(U8 *pBuff, U16 BuffLen)
{
	U8 testAddr[6]     = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
	U8 TestMode[4]     = {0x87,0x76,0x8D,0xA5};
	U8 SendBuf[50];
	U8 TxLen;
    U8 addr[6]={0};
    
    if(Check645Frame(pBuff,BuffLen,NULL,addr,NULL)== FALSE)
    {
        app_printf("check645 err!\n");
        return ERROR;
    }   
	if(*(pBuff+8) != 0x1F || *(pBuff+9) != sizeof(TestMode))
	{
		return ERROR;
	}
	if(memcmp(pBuff+10, TestMode, 4))
	{
		return ERROR;

	}
//检验是07-645下的工厂模式测试帧
#if defined(STATIC_MASTER)	
	 __memcpy(FlashInfo_t.ucJZQMacAddr ,addr,6);
#elif defined(ZC3750STA)
	 __memcpy(DevicePib_t.DevMacAddr ,addr, 6);
#elif defined(ZC3750CJQ2)
     ChangeMacAddrOrder(addr);
	 __memcpy(nnib.MacAddr, addr,6);
	 cjq2_search_suspend_timer_modify(600 * TIMER_UNITS);
#endif

	memset(SendBuf , 0x00 , sizeof(SendBuf));
    TxLen = 0;
    
    SendBuf[TxLen++] = 0x68;
    __memcpy(SendBuf+TxLen , testAddr, 6);
    TxLen += 6;
    SendBuf[TxLen++] = 0x68;
    SendBuf[TxLen++] = 0x9F;
    SendBuf[TxLen++] = 0x04;
    __memcpy(SendBuf+TxLen , TestMode , 4);
    TxLen += 4;
    SendBuf[TxLen]=check_sum(SendBuf,TxLen);
    TxLen++;
    SendBuf[TxLen++] = 0x16;

	FactoryTestFlag = 1;
#if defined(ZC3750STA)
#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)
    IO_check_timer_Init();
    //timer_start(IOChecktimer);
#endif
#endif
    //UartSend(0 ,SendBuf,TxLen);
    Extend645PutList(DLT645_1997,BAUDMETER_TIMEOUT,0,TxLen,SendBuf);
	return OK;
}
*/









#if defined(STATIC_NODE)
//97规约绑表命令
extern U8 sta_bind_frame_97[14];
//07规约绑表命令
extern U8 sta_bind_frame_07[12];
//698规约绑表命令
extern U8 sta_bind_frame_698[25];





#endif

U32 SetDefInfo(U8 item , U8 *data , U8 datalen)
{
    U32 result = 0;
    switch(item)
    {
        case HARDWARE_DEF: //硬件信息
            if(datalen == (sizeof(DefSetInfo.Hardwarefeature_t)-2))
            {
                __memcpy(&DefSetInfo.Hardwarefeature_t,data,(sizeof(DefSetInfo.Hardwarefeature_t)-2));
                app_printf("DefSetInfo.Hardwarefeature_t->");
                dump_buf(data,datalen);
                result = HARDWARE_DEF;
                DefwriteFg.HWFeatrue = TRUE;
            }
        break;
        case FLASH_FREQ_DEF: //功率信息
            if(datalen == (sizeof(DefSetInfo.FreqInfo)-3))
            {
                __memcpy(&DefSetInfo.FreqInfo,data,(sizeof(DefSetInfo.FreqInfo)-3));
               if(DefSetInfo.FreqInfo.tgaindig<0)
    		   {
    			   DefSetInfo.FreqInfo.tgaindig =0;
    		   }
    		   else if(DefSetInfo.FreqInfo.tgaindig >8)
    		   {
    			   DefSetInfo.FreqInfo.tgaindig =8; 
    		   }
    		   if(DefSetInfo.FreqInfo.tgainana<-20)
    		   {
    			   DefSetInfo.FreqInfo.tgainana =-20;
    		   }
    		   else if(DefSetInfo.FreqInfo.tgainana >18)
    		   {
    			   DefSetInfo.FreqInfo.tgainana =18;    
    		   }
                app_printf("DefSetInfo.FreqInfo->");
                dump_buf(data,datalen);
                result = FLASH_FREQ_DEF;
                DefwriteFg.FREQ = TRUE;
            }
        break;
		case DEVICE_ADDR_DEF: //出厂地址信息
            if(datalen == (sizeof(DefSetInfo.DeviceAddrInfo)-2))
            {
                __memcpy(&DefSetInfo.DeviceAddrInfo,data,(sizeof(DefSetInfo.DeviceAddrInfo)-2));
                app_printf("DefSetInfo.DeviceAddrInfo\n");
                result = DEVICE_ADDR_DEF;
				DefwriteFg.DevAddr = TRUE;
            }
        break;
        case FUNC_SWITCH_DEF: //应用功能开关信息
            if(datalen == (sizeof(app_ext_info.func_switch)-2))
            {
                __memcpy(&app_ext_info.func_switch,data,(sizeof(app_ext_info.func_switch)-2));
                app_printf("app_ext_info.func_switch\n");
                result = FUNC_SWITCH_DEF;
				DefwriteFg.FunSWC = TRUE;
            }
        break;
		case PARAMETER_CFG: //应用功能开关信息
            if(datalen == (sizeof(app_ext_info.param_cfg)-2))
            {
                __memcpy(&app_ext_info.param_cfg,data,(sizeof(app_ext_info.param_cfg)-2));
                app_printf("app_ext_info.param_cfg\n");
                result = PARAMETER_CFG;
				DefwriteFg.ParaCFG = TRUE;
            }
        break;
        default :
            result = DEF_ERR;
            break;
        

    }
    return result;
}
U32 ReadDefInfo(U8 item , U8 *data , U8 *datalen)
{
    U32 result = 0;
    switch(item)
    {
        case HARDWARE_DEF: //硬件信息
            {
                __memcpy(data,&DefSetInfo.Hardwarefeature_t,(sizeof(DefSetInfo.Hardwarefeature_t)-2));
				*datalen = (sizeof(DefSetInfo.Hardwarefeature_t)-2);
                app_printf("DefSetInfo.Hardwarefeature_t->");
                dump_buf(data,*datalen);
                result = HARDWARE_DEF;
            }
        	break;
		
        case FLASH_FREQ_DEF: //功率信息 
            {
                __memcpy(data,&DefSetInfo.FreqInfo,(sizeof(DefSetInfo.FreqInfo)-3));
				*datalen = (sizeof(DefSetInfo.FreqInfo)-3);
                app_printf("DefSetInfo.FreqInfo->");
                dump_buf(data,*datalen);
                result = FLASH_FREQ_DEF;
                
            }
        	break;
		
        case DEVICE_ADDR_DEF: //出厂设备地址
            {
                __memcpy(data,&DefSetInfo.DeviceAddrInfo,(sizeof(DefSetInfo.DeviceAddrInfo)-2));
				*datalen = (sizeof(DefSetInfo.DeviceAddrInfo)-2);
                app_printf("DefSetInfo.DeviceAddrInfo->");
				dump_buf(data,*datalen);
                result = FUNC_SWITCH_DEF;
				
            }
        	break;
		
		case FUNC_SWITCH_DEF: //应用功能开关信息
            {
                __memcpy(data,&app_ext_info.func_switch,(sizeof(app_ext_info.func_switch)-2));
				*datalen = (sizeof(app_ext_info.func_switch)-2);
                app_printf("app_ext_info.func_switch->");
				dump_buf(data,*datalen);
                result = FUNC_SWITCH_DEF;
				
            }
        	break;
		
		case PARAMETER_CFG: //应用功能开关信息 
            {
                __memcpy(data,&app_ext_info.param_cfg,(sizeof(app_ext_info.param_cfg)-2));
				*datalen = (sizeof(app_ext_info.param_cfg)-2);
                app_printf("app_ext_info.param_cfg->");
				dump_buf(data,*datalen);
                result = PARAMETER_CFG;
				
            }
        	break;
		
        default :
            result = DEF_ERR;
            break;
        

    }
    return result;
}


U8 CheckDefInfo(U8 * pdata ,U16 pdatalen)
{
    U8 datanum = 0,datalen = 0,byLen = 0;
    U8 TempDataNum = 0,TempDataLen = 0;
    U8 ItemCnt =0;
    //SET_DEF_BY_645_t DefInfo[10];
    U8 Item = 0,DefInfolen = 0;
    U32 result = 0;
    
    datanum = pdata[byLen++];
    datalen = pdata[byLen++];
    for(ItemCnt = 0 ; ItemCnt < datanum ; ItemCnt++)
    {
        Item = pdata[byLen++];
        DefInfolen = pdata[byLen++];
        
        result = SetDefInfo(Item , &pdata[byLen] , DefInfolen);
        byLen += DefInfolen;
        if(result == 0)
        {
            app_printf("result == 0!\n");
            return FALSE;
        }
        else
        {
            TempDataNum ++;
            TempDataLen += DefInfolen;
        }
    }
    app_printf("TempDataNum=%d,datanum=%d,TempDataLen=%d,datalen=%d,pdatalen=%d,byLen=%d!\n",TempDataNum,datanum,TempDataLen,datalen,pdatalen,byLen);
    if(TempDataNum == datanum&&TempDataLen == datalen&&pdatalen == (byLen))
    {
        app_printf("DefSetInfo is OK!\n");
        return TRUE;
    }
    return FALSE;
    
    
}
void get_NNIB_info(U8 *data , U8 *byLen)
{

    *byLen = 0;
    __memcpy(&data[*byLen], (U8 *)&nnib.Networkflag, sizeof(nnib.Networkflag)); // 1
    *byLen += sizeof(nnib.Networkflag);
    U32 snid = GetSNID();
    __memcpy(&data[*byLen], (U8 *)&snid, sizeof(snid)); // 4
    *byLen += sizeof(snid);
    U16 StaTei = GetTEI();
    __memcpy(&data[*byLen], (U8 *)&StaTei, sizeof(StaTei)); // 2
    *byLen += sizeof(StaTei);
    __memcpy(&data[*byLen], (U8 *)&nnib.NodeType, sizeof(nnib.NodeType)); // 1
    *byLen += sizeof(nnib.NodeType);
    __memcpy(&data[*byLen], (U8 *)&nnib.NodeState, sizeof(nnib.NodeState)); // 1
    *byLen += sizeof(nnib.NodeState);
    __memcpy(&data[*byLen], (U8 *)&nnib.StaOfflinetime, sizeof(nnib.StaOfflinetime)); // 2
    *byLen += sizeof(nnib.StaOfflinetime);
    __memcpy(&data[*byLen], (U8 *)&nnib.NodeLevel, sizeof(nnib.NodeLevel)); // 1
    *byLen += sizeof(nnib.NodeLevel);
    U16 ParentTei = GetProxy();
    __memcpy(&data[*byLen], (U8 *)&ParentTei, sizeof(ParentTei)); // 2
    *byLen += sizeof(ParentTei);
    __memcpy(&data[*byLen], (U8 *)&nnib.SendDiscoverListTime, sizeof(nnib.SendDiscoverListTime)); // 4
    *byLen += sizeof(nnib.SendDiscoverListTime);
    __memcpy(&data[*byLen], (U8 *)&nnib.SuccessRateReportTime, sizeof(nnib.SuccessRateReportTime)); // 4
    *byLen += sizeof(nnib.SuccessRateReportTime);
    __memcpy(&data[*byLen], (U8 *)&nnib.HeartBeatTime, sizeof(nnib.HeartBeatTime)); // 4
    *byLen += sizeof(nnib.HeartBeatTime);
    __memcpy(&data[*byLen], (U8 *)&nnib.RecvDisListTime, sizeof(nnib.RecvDisListTime)); // 2
    *byLen += sizeof(nnib.RecvDisListTime);
    __memcpy(&data[*byLen], (U8 *)&nnib.WorkState, sizeof(nnib.WorkState)); // 1
    *byLen += sizeof(nnib.WorkState);
    __memcpy(&data[*byLen], (U8 *)&nnib.DeviceType, sizeof(nnib.DeviceType)); // 1
    *byLen += sizeof(nnib.DeviceType);

}

 U8 CheckNeighborInfo(U8 * pdata ,U16 pdatalen , U8 *updata , U8 *updatalen)
 {
 /*
	 U8 byLen = 0;

	 U16 usStartIdx = 0;
	 U8	 ucReadNum = 0;
	 U8	 pnum = 0;
	 U8  MesgType = 0;
	 U8  start_1_flag = 1;
	 U16 ii = 0,jj = 0;
	 U8  FF_addr[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	 U16 all_num = 0;
	 
	 MesgType = pdata[0];
	 usStartIdx = (U16)pdata[1];
	 usStartIdx += (U16)pdata[2] << 8;
	 ucReadNum = pdata[3];
	 if(ucReadNum<=0)
	 {
		//无信息可读
		return FALSE;
	 }
	 if(ucReadNum>64)
	 {
		//无信息可读
		return FALSE;
	 }
	 app_printf("usStartIdx = %d\n",usStartIdx);
	 updata[byLen++] = MesgType;
	 if(0==usStartIdx)
	{
		start_1_flag=0;
	}
	
	if(start_1_flag==1)
	{
		usStartIdx -= 1;
	}
	if(MesgType == 1)
	{
		get_NNIB_info(&updata[byLen],updatalen);
		*updatalen += byLen;
		return TRUE;
	}
	else if(MesgType == 2)
	{	
		all_num = GetNeighborCount();
		updata[byLen++] = (U8)(all_num & 0xFF);
		updata[byLen++] = (U8)(all_num >> 8);
		updata[byLen++] = (U8)(usStartIdx & 0xFF);
		updata[byLen++] = (U8)(usStartIdx >> 8);
		updata[byLen] = 0;//ucReadNum;
		
		byLen ++;
		
		for(jj=0;jj<MAX_NEIGHBOR_NUMBER;jj++)
		{
			U8 NeighborListTemp[sizeof(NEIGHBOR_DISCOVERY_TABLE_ENTRY_t)];
			Get_Neighbor_All(jj , (NEIGHBOR_DISCOVERY_TABLE_ENTRY_t *)&NeighborListTemp);
			if(0==memcmp(&NeighborListTemp[4],FF_addr,6))//无效地址
			{
				continue;
			}
			
			if(ii == usStartIdx)//当前为读取的起始位置
			{
				
				__memcpy(&updata[byLen], &NeighborListTemp[0], 4+6+2+1+1+1+1); //16字节表地址
				byLen += 16;
				__memcpy(&updata[byLen], &NeighborListTemp[16+2], 1+1+2+2+2+2+2+2+1+1); //
				byLen += 16;

				pnum += 1;

				if(pnum == ucReadNum || byLen>=200)
				{
					//WHLPTED//释放共享存储内容保护
					goto send;
				}
			}
			else if(ii < usStartIdx)//已读过的地址
			{
				ii++;//查找当前读取到的地址位置
			}
			
		}

	}
#if defined(ZC3750STA) || defined(ZC3750CJQ2)

	else if(MesgType == 3)
	{
		all_num = GetNeighborCount();
		updata[byLen++] = (U8)(all_num & 0xFF);
		updata[byLen++] = (U8)(all_num >> 8);
		updata[byLen++] = (U8)(usStartIdx & 0xFF);
		updata[byLen++] = (U8)(usStartIdx >> 8);
		updata[byLen] = 0;//ucReadNum;
		byLen ++;
		
		for(jj=0;jj<MAX_NEIGHBOR_NUMBER;jj++)
		{
			
			if(SnrReacord[jj].SNID ==0XFFFFFFFF)
			{
				continue;
			}
			
			if(ii == usStartIdx)//当前为读取的起始位置
			{
				S32 gain=0;
				__memcpy(&updata[byLen], &SnrReacord[jj].TEI, 2); //2字节表地址
				byLen +=2;
				SearchMacAddressInNeighborDiscoveryTableEntrybyTEI_SNID(SnrReacord[jj].TEI ,SnrReacord[jj].SNID ,&updata[byLen]);
				byLen +=6;
				gain = SnrReacord[jj].GAIN/SnrReacord[jj].RecvCount;
				__memcpy(&updata[byLen], &gain, sizeof(S32)); //4字节表地址
				byLen +=4;

				pnum += 1;

				if(pnum == ucReadNum || byLen>=230)
				{
					//WHLPTED//释放共享存储内容保护
					goto send;
				}
			}
			else if(ii < usStartIdx)//已读过的地址
			{
				ii++;//查找当前读取到的地址位置
			}
			
		}

		
	}
#endif
	send:
	updata[5] = pnum;	
	*updatalen += byLen;	
	app_printf("all_num=%d,pnum=%d,ucReadNum=%dbyLen=%d!\n",all_num,pnum,ucReadNum,byLen);
	*/ 

	return TRUE;
	 
	 

 }


U8  QueryBaud()
{
    U8  BaudValue = 0;
	U32 tp_baud = 0;
#if defined(ZC3750STA)
	tp_baud = sta_bind_info_t.BaudRate;
#elif defined(ZC3750CJQ2)
	tp_baud = BaudNum;
#endif
    (tp_baud == BAUDRATE_2400)? (BaudValue = BAUD_2400):
        (tp_baud == BAUDRATE_4800)? BaudValue = BAUD_4800:
            (tp_baud == BAUDRATE_9600)? BaudValue = BAUD_9600:
                (tp_baud == BAUDRATE_38400)? BaudValue = BAUD_38400:
                    (tp_baud == BAUDRATE_57600)? BaudValue = BAUD_57600:
                        (tp_baud == BAUDRATE_115200)? BaudValue = BAUD_115200:
                            (tp_baud == BAUDRATE_460800)? BaudValue = BAUD_460800:
								(tp_baud == BAUDRATE_921600)? BaudValue = BAUD_921600:
                                    (tp_baud == BAUDRATE_19200)? BaudValue = BAUD_19200:
                                        (tp_baud == BAUDRATE_230400)? BaudValue = BAUD_230400:
                                 	        (BaudValue = 0);
    return  BaudValue;
}


U8 CheckBaudValue(U8 *buf,U8 buflen)
{
	U32 tp_baud = 0;

    if(buflen != 1)
    {
        //return FALSE;
    }
    switch(*buf)
    {
        case BAUD_NULL: // 
            app_printf("Uart1 Baud = %d!!!\n",tp_baud);
            return TRUE;
            break;
        case BAUD_2400: // 
            tp_baud = BAUDRATE_2400;
            break;
    
        case BAUD_4800: //
            tp_baud = BAUDRATE_4800;
            break;
        case BAUD_9600: //
            tp_baud = BAUDRATE_9600;
            break;
        case BAUD_38400: //
            tp_baud = BAUDRATE_38400;
            break;
        case BAUD_57600: //
            tp_baud = BAUDRATE_57600;
            break;
        case BAUD_115200: //
            tp_baud = BAUDRATE_115200;
            break;
        case BAUD_460800: //
            tp_baud = BAUDRATE_460800;
            break;
        case BAUD_921600: //
            tp_baud = BAUDRATE_921600;
            break;
        case BAUD_19200: //
            tp_baud = BAUDRATE_19200;
            break;
        case BAUD_230400: //
            tp_baud = BAUDRATE_230400;
            break;
        default : //
            return FALSE;
            break;
    }
	#if defined(ZC3750STA)
	sta_bind_info_t.BaudRate = tp_baud;
	#elif defined(ZC3750CJQ2)
	BaudNum = tp_baud;
	#endif
    app_printf("Change Uart1 Baud = %d!!!\n",tp_baud);
    MeterUartInit(g_meter, tp_baud);
    return TRUE;
}
U32  LblockID = 0xFFFFFFFF;

U32 checkUpgradedata(U8 *databuf,U16 buflen)
{
#if defined(ZC3750STA) || defined(ZC3750CJQ2)

    U16 rcvLen = 0;
	U8  fileTransId, fileAttribute, fileCmd;
	U16 totolBlockNum;
	U16 blockID;
	U32 offsetAddr;
	U16 blockLen = 0;
	static U16 BlockSize = 0;
	
	//U8 pBlockData[400] = {0};
    U8 *pBlockData = NULL;

    pBlockData = zc_malloc_mem(buflen,"pBlockData",MEM_TIME_OUT);
    
	fileTransId  = databuf[rcvLen++];
	fileAttribute = databuf[rcvLen++];
	fileCmd       = databuf[rcvLen++];

	totolBlockNum = (U16)(databuf[rcvLen]|(databuf[rcvLen+1]<<8));//*(U16*)&databuf[rcvLen];
	rcvLen += 2;
	blockID = (U32)(databuf[rcvLen]|(databuf[rcvLen+1]<<8)|(databuf[rcvLen+2]<<16)|(databuf[rcvLen+3]<<24));//*(U32*)&databuf[rcvLen];
	rcvLen += 4;
	blockLen     = (U16)(databuf[rcvLen]|(databuf[rcvLen+1]<<8));//*(U16*)&databuf[rcvLen];
	rcvLen += 2;
    app_printf("totolBlockNum =%d,blockID =%d,blockLen =%d\n",totolBlockNum,blockID,blockLen);
    if(fileTransId == 0)
    {
        LblockID = 0;
    }
    
    if(fileAttribute == 0x00 && blockID == 0x00)
    {
        upgrade_sta_clean_image_reserve();
        upgrade_sta_clean_ram_data_temp();
        BlockSize = blockLen;
        //如果不是自己image帧头，不接收
        __memcpy(pBlockData, (databuf+rcvLen), blockLen);
        imghdr_t *ih = (imghdr_t *)pBlockData;
        if (!is_tri_image(ih))
        {
            zc_free_mem(pBlockData);
            return 0xFFFFFFFF;
        }
    }
    
    offsetAddr = blockID*BlockSize;
    if(offsetAddr > (MAX_UPGRADE_FILE_SIZE-BlockSize))
    {
        zc_free_mem(pBlockData);
        return 0xFFFFFFFF;
    }

    if(fileCmd != 0x00)
    {
           ;
    }
    
    
    if(blockID == 0 || LblockID == (blockID-1))//包序号合法，更改记录
    {
        LblockID = blockID;
    }
    else//包序号不合法
    {
        zc_free_mem(pBlockData);
        return LblockID;
    }
    __memcpy(pBlockData, (databuf+rcvLen), blockLen);
    app_printf("Block ID  is %d\n", blockID);
    upgrade_sta_flash_to_ram(blockID,BlockSize,pBlockData,blockLen);
    zc_free_mem(pBlockData);
    
    //最后一帧数据接收
    if(fileAttribute == 0x01 && blockID == (totolBlockNum-1))
    {
        LblockID = 0xFFFFFFFF;

         upgrade_sta_write_last_data(BlockSize);
         app_printf("check_image == IMAGE_OK!!!!!!!!!!!!!\n");
         

         #if defined(ZC3750STA)||defined(ZC3750CJQ2)

         SlaveUpgradeFileInfo.FileSize = offsetAddr+blockLen;

         os_sleep(1);

         os_sleep(100);
         wdt_close();
         softdog_stop();
         uint32_t cpu_sr;
         cpu_sr = OS_ENTER_CRITICAL();
         upgrade_sta_chage_run_image();
         OS_EXIT_CRITICAL(cpu_sr);
         #endif
             
    }
    return blockID;
#else
    return 0xFFFFFFFF;
#endif
}


/*645扩展错误码格式*/
U8  CODE_ERR[4] = {0x54 , 0x43 , 0x5A , 0xCA};
U8  DATA_ERR[4] = {0x54 , 0x43 , 0x5A , 0xCB};
U8  MODE_ERR[4] = {0x54 , 0x43 , 0x5A , 0xCC};

void ReadRelayZone(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    //app_printf("ReadRelayZone ->\n");
#if defined(CJQ_SET_AS_RELAY) && defined(ZC3750CJQ2)
    U8  ByLen;
    ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    ByLen += 2;//网络编号
    __memcpy(&UpData[ByLen],CollectInfo_st.SetCCOAddr , 6);//归属地址
    ByLen += 6;
    *UpDataLen = ByLen;
    
#endif
}
void SetRelayZone(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    //app_printf("SetRelayZone ->\n");
#if defined(CJQ_SET_AS_RELAY) && defined(ZC3750CJQ2)
    U8  ByLen;
    ByLen = 0;
    
    __memcpy(CollectInfo_st.SetCCOAddr, &ValidData[PREAMBLE_CHAR_COUNT] , 6);//设置归属地址
    //staflag = TRUE;
    //mutex_post(mutexSch_t);	
    
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    ByLen += 2;//网络编号
    __memcpy(CollectInfo_st.SetCCOAddr,&UpData[ByLen] , 6);//归属地址
    ByLen += 6;
    *UpDataLen = ByLen;
    
#endif
}
typedef struct
{
    U8	      ucVendorCode[2];
    U8	      ucChipCode[2];
    U8	      ucDay;
    U8	      ucMonth;
    U8	      ucYear;
    U8	      ucVersionNum[2];
} __PACKED MF_VERSION_t;

void ReadOuterVersion(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;
    MF_VERSION_t version;

	if(!ReadDefaultInfo())
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;	
	}
	
    ReadOutVersion();
	
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    
    version.ucVendorCode[0] = DefSetInfo.OutMFVersion.ucVendorCode[0];
    version.ucVendorCode[1] = DefSetInfo.OutMFVersion.ucVendorCode[1];
    version.ucChipCode[0] = DefSetInfo.OutMFVersion.ucChipCode[0];
    version.ucChipCode[1] = DefSetInfo.OutMFVersion.ucChipCode[1];
    version.ucDay = DefSetInfo.OutMFVersion.ucDay;
    version.ucMonth = DefSetInfo.OutMFVersion.ucMonth;
    version.ucYear = DefSetInfo.OutMFVersion.ucYear;
    version.ucVersionNum[0] = DefSetInfo.OutMFVersion.ucVersionNum[0];
    version.ucVersionNum[1] = DefSetInfo.OutMFVersion.ucVersionNum[1];
    
    __memcpy(&UpData[ByLen],(U8 *)&version,9);
    ByLen += 9;
    *UpDataLen = ByLen;
}

void ReadinnerVersion(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    UpData[ByLen ++] = 0x43;
    UpData[ByLen ++] = 0x5A;
#if defined(STATIC_MASTER)
    UpData[ByLen ++] = ZC3951CCO_chip2;//
    UpData[ByLen ++] = ZC3951CCO_chip1;
#elif defined(ZC3750STA)
    UpData[ByLen ++] = ZC3750STA_chip2;
    UpData[ByLen ++] = ZC3750STA_chip1;
#elif defined(ZC3750CJQ2)
    UpData[ByLen ++] = ZC3750CJQ2_chip2;
    UpData[ByLen ++] = ZC3750CJQ2_chip1;
#endif

    UpData[ByLen ++] = U8TOBCD(InnerDate_D);
    UpData[ByLen ++] = U8TOBCD(InnerDate_M);
    UpData[ByLen ++] = U8TOBCD(InnerDate_Y);

#if defined(STATIC_MASTER)			 
    UpData[ByLen ++] = ZC3951CCO_Innerver2;
    UpData[ByLen ++] = ZC3951CCO_Innerver1;
#elif defined(ZC3750STA)
    UpData[ByLen ++] = ZC3750STA_Innerver2;
    UpData[ByLen ++] = ZC3750STA_Innerver1;
#elif defined(ZC3750CJQ2)
    UpData[ByLen ++] = ZC3750CJQ2_Innerver2;
    UpData[ByLen ++] = ZC3750CJQ2_Innerver1;
#endif

	UpData[ByLen ++] = app_ext_info.province_code;
	UpData[ByLen ++] = PRODUCT_func; //功能
	UpData[ByLen ++] = CHIP_code; //芯片类型

#if defined(STATIC_MASTER)       
	UpData[ByLen ++] = ZC3951CCO_type;//产品类型
	UpData[ByLen ++] = ZC3951CCO_prtcl; //通信规约
#elif defined(ZC3750STA)
	UpData[ByLen ++] = ZC3750STA_type;
	UpData[ByLen ++] = ZC3750STA_prtcl; //通信规约
#elif defined(ZC3750CJQ2)
	UpData[ByLen ++] = ZC3750CJQ2_type;
	UpData[ByLen ++] = ZC3750CJQ2_prtcl; //通信规约
#endif

	UpData[ByLen ++] = POWER_OFF;

#if defined(STATIC_MASTER)       
	UpData[ByLen ++] = ZC3951CCO_prdct; //具体产品
#elif defined(ZC3750STA)
	UpData[ByLen ++] = ZC3750STA_prdct; //具体产品
#elif defined(ZC3750CJQ2)
	UpData[ByLen ++] = ZC3750CJQ2_prdct; //具体产品
#endif
	UpData[ByLen ++] = PROPERTY; //软件属性

	UpData[ByLen ++] = 0x00; //保留
	UpData[ByLen ++] = 0x00; //保留
	UpData[ByLen ++] = 0x00; //保留
	UpData[ByLen ++] = 0x00; //保留
	*UpDataLen = ByLen;
}
U8  SelfCheckFlag = FALSE;
void SelfCheckTest(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;

    UpData[ByLen] = 0;
#if defined(ZC3750STA)
    if(TRUE == IoCheck())
    {
        if(SETValue == 0x06 && EVEValue == 0x06)
        {
            UpData[ByLen] |= 0x01; 
        }
    }
    if(ReSetValue == 0x06)
    {
        UpData[ByLen] |= 0x20;
    }
    if(PlugValue == 0x06)
    {
        UpData[ByLen] |= 0x40;
    }
#endif
    if(TRUE == FlashCheck())
    {
        UpData[ByLen] |= 0x02;
    }

    if(nnib.PhasebitA)
    {
        UpData[ByLen] |= 0x04;
    }

    if(nnib.PhasebitB)
    {
        UpData[ByLen] |= 0x08;
    }

    if(nnib.PhasebitC)
    {
        UpData[ByLen] |= 0x10;
    }
    ByLen +=1;

    __memcpy(&UpData[ByLen] , ChipID , 8);
    ByLen += 8;
    *UpDataLen = ByLen;
}
U8 TestSof[]={0x10 ,0x00 ,0xff ,0x0f ,0x00 ,0xe6 ,0x02 ,0x30 ,0x08 ,0xa8 ,0x11 ,0x01 ,0x02 ,0x41 ,0x00 ,0x00 ,0x11 ,0x06 ,0x00 ,0x00 ,
0x01 ,0x41 ,0x00 ,0x06 ,0x41 ,0xfe ,0x64 ,0x96};
void test_tx_end(phy_evt_param_t *para, void *arg)
{
    dump_zc(0,"$ test tx : ",DEBUG_PHY,para->fi->head,sizeof(frame_ctrl_t));
}

void test_tx()
{
    // hplc_hrf_phy_reset();
    phy_reset();
    //phy_tx_start_arg(DT_SOF,NwkBeaconPeriodCount, 3,e_A_PHASE,0,0xfff, PHY_US2TICK(1000*2)+get_phy_tick_time(), beacon,0,0,1,0,0,0,0);
    //extern phy_demo_t PHY_DEMO;
    //PHY_DEMO.pf.payload = TestSof;
    //PHY_DEMO.pf.length = sizeof(TestSof);
    evt_notifier_t ntf[1];
    int32_t ret;
    
    mbuf_hdr_t *mbuf_hdr = NULL;
    extern mbuf_hdr_t * packet_sof(uint8_t priority,uint16_t dtei,uint8_t *payload,uint16_t len,uint8_t retrans ,uint8_t tf, uint8_t phase,plc_work_t *cnfrm);
    mbuf_hdr = packet_sof(2, 1 ,TestSof, sizeof(TestSof),1,0,PHASE_A,NULL);

    if (mbuf_hdr==NULL)
    {
        return;
    }
    
    
    fill_evt_notifier(&ntf[0], PHY_EVT_TX_END, test_tx_end, NULL); 

    uint32_t time = PHY_US2TICK(1000*2)+get_phy_tick_time();
    ld_set_tx_phase_zc(PHY_PHASE_A);
    if((ret = phy_start_tx(&mbuf_hdr->buf->fi, &time, &ntf[0] , 1)) != OK)//&ntf[0], 1);
    {
        phy_debug_printf("tick_time = 0x%08x tx failed,ret = %d\n",get_phy_tick_time(), ret);
    }
    zc_free_mem(mbuf_hdr);

}
void DynamiacCheckTest(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;
    U16  SendCount;

    ByLen = 0;
    SendCount = 100;
    
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    //zc_phy_reset();
    os_sleep(1);
    while(SendCount--)
    {
        test_tx();
        os_sleep(1);
    }
    hplc_do_next();
    *UpDataLen = ByLen;
}
/*

void MemoryCheckTest(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
    memset( Send645Buf , 0x00 , sizeof(Send645Buf));
    __memcpy(Send645Buf , data + FECount + 14 ,sizeof(s_SelfCheckMemory));
    U8 ii              = 0;
    for(ii = 0 ; ii < sizeof(s_SelfCheckMemory) ; ii++)
    {
        Send645Buf[ii] -= 0x33;
    }
    
    memset(&AddFlashMemory  , 0x00 , sizeof(s_SelfCheckMemory)) ;
    __memcpy(&AddFlashMemory , Send645Buf ,sizeof(s_SelfCheckMemory));

    app_printf("AddFlashMemory_addr = %08x\r\n",&AddFlashMemory);
    //AddFlashMemory.Count ++;
    if(FLASH_OK != zc_flash_write(FLASH, (U32)CHECKMEMORY  ,(U32)&AddFlashMemory ,sizeof(s_SelfCheckMemory)))
    {
        app_printf("zc_flash_write CHECKMEMORY error\r\n");
        return;
    }
    else
    {
        app_printf("zc_flash_write CHECKMEMORY ok\r\n");
        //TestCount++;
        memset( Send645Buf , 0x00 , sizeof(SendBuf));
        
        SendBuf[TxLen++] = e_MEMORY_CHECK_TEST;
        
    }
    
}

void MemoryReadTest(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;

    *UpDataLen = ByLen;
    memset(&AddFlashMemory , 0x00 , sizeof(s_SelfCheckMemory));
    if(FLASH_OK != zc_flash_read(FLASH, (U32)CHECKMEMORY  ,(U32 *)&AddFlashMemory ,sizeof(s_SelfCheckMemory)))
    {
        app_printf("zc_flash_read CHECKMEMORY error\r\n");
        return;
    }
    else
    {
        app_printf("zc_flash_read CHECKMEMORY ok\r\n");
        
        app_printf("AddFlashMemory.SelfCheckMessage = %04x\r\n",AddFlashMemory.SelfCheckMessage);
        app_printf("AddFlashMemory.ChipId = %02x %02x %02x %02x %02x %02x %02x %02x\r\n",AddFlashMemory.ChipId[0],AddFlashMemory.ChipId[1],
                    AddFlashMemory.ChipId[2] , AddFlashMemory.ChipId[3] , AddFlashMemory.ChipId[4] , AddFlashMemory.ChipId[5],
                    AddFlashMemory.ChipId[6] , AddFlashMemory.ChipId[7]);
        app_printf("AddFlashMemory.TesterName = %02x %02x %02x \r\n",AddFlashMemory.TesterName[0] , AddFlashMemory.TesterName[1],
                    AddFlashMemory.TesterName[2]);
        app_printf("AddFlashMemory.Date = %02x %02x %02x \r\n",AddFlashMemory.Date[0] , AddFlashMemory.Date[1] , AddFlashMemory.Date[2]);
        app_printf("AddFlashMemory.Count = %02x\r\n",AddFlashMemory.Count);

		SendBuf[TxLen++] = e_MEMORY_READ_TEST;
        U8 tmp_buf[17] = {0};
        __memcpy(tmp_buf , &AddFlashMemory , sizeof(s_SelfCheckMemory));
       
        __memcpy(SendBuf+TxLen , tmp_buf , sizeof(s_SelfCheckMemory));
        TxLen += sizeof(s_SelfCheckMemory);

		
    }
}
*/


void SetBand(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
#if defined(ZC3750STA) || defined(ZC3750CJQ2)
    //StandThisBand();
#endif		
    U8 bandnum = ValidData[4];
    //changeband(bandnum);
	net_nnib_ioctl(NET_SET_BAND,&bandnum);
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    UpData[ByLen++] = bandnum;
    *UpDataLen = ByLen;
}
void SetPhyCBMode(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    
    U8 setmode ;
    setmode= ValidData[PREAMBLE_CHAR_COUNT];
    app_printf("setting mode :%s\n",e_PHY_TPT == setmode ? "PHY_TPT" : 
                                    e_PHY_CB == setmode ? "PHY_CB":
                                    e_MAC_TPT == setmode ? "MAC_TPT":
                                    e_WPHY_CB == setmode ? "WPHY_CB":
                                    e_RF_PLCPHYCB == setmode ? "RF_PLCPHYCB":
                                    e_PLC_TO_RF_CB == setmode ? "PLC_TO_RF_CB":
                                    "NORM");

#if defined(STATIC_MASTER)
    //uart1testmode(BAUDRATE_9600);//UartInit(g_meter,BAUDRATE_9600);
#elif defined(ZC3750STA) || defined(ZC3750CJQ2)
    //MeterUartInit(g_meter,2400);

    //phy_reset_sfo(&HSFO, 0, 1, NR_SFO_EVAL_DEF, NR_SFO_FINE_DEF_TEST);
#endif
    UpData[ByLen++] = setmode;

	setmode = (e_PHY_TPT == setmode ? PHYTPT : 
                e_PHY_CB == setmode ? PHYCB :
                    e_WPHY_CB == setmode ? WPHYCB :
                        e_RF_PLCPHYCB == setmode ? RF_PLCPHYCB :
                            e_PLC_TO_RF_CB == setmode ? PLC_TO_RF_CB :
							    NORM);
	net_nnib_ioctl(NET_SET_TSTMODE,&setmode);	
    *UpDataLen = ByLen;
}
void PlcSendFrame(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;

}
void SetOuterVendorcodeVersion(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;

	if((ValidDataLen-4) != (sizeof(OUT_MF_VERSION_t)-3))
	{
        __memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;	
		*UpDataLen = ByLen;
		return;
	}

	if(FALSE == setoutversion(&ValidData[0+4],&ValidData[2+4],&ValidData[4+4],&ValidData[7+4]))
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
	}

	__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT; 
	*UpDataLen = ByLen;
}
void RebootDevice(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
    reboot_after_1s();
}

void WriterChipID(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;
    if((ValidDataLen-4) != sizeof(ManageID))
    {
    	__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
    }

	__memcpy(ManageID, &ValidData[4] , ValidDataLen-4);
    DefwriteFg.CID  = TRUE;
	
	if(FALSE == SetDefaultInfo())
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
	}          

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
	*UpDataLen = ByLen;

}


void WriteModuleID(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;
    if((ValidDataLen-4) != sizeof(ModuleID))
    {
    	__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
    }
	
	__memcpy(ModuleID, &ValidData[4] , ValidDataLen);
    DefwriteFg.MID  = TRUE;
	
	if(FALSE == SetDefaultInfo())
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
	}
	
	__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
	*UpDataLen = ByLen;

}

void ReadModuleID(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;
	
	if(!ReadDefaultInfo())
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;	
	}

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;

    __memcpy(&UpData[ByLen], DefSetInfo.ModuleIDInfo.ModuleID, sizeof(DefSetInfo.ModuleIDInfo.ModuleID));
    ByLen += sizeof(DefSetInfo.ModuleIDInfo.ModuleID);
    *UpDataLen = ByLen;

}


void ReadChipID(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
	U8 ByLen = 0;
	if( VOLUME_PRODUCTION == app_ext_info.func_switch.IDInfoGetModeSWC)//量产
	{
		ReadManageID();
	}
	else
	{
		if(!ReadDefaultInfo())
		{
			__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
	        ByLen += PREAMBLE_CHAR_COUNT;
			*UpDataLen = ByLen;
			return;	
		}
	}

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    
    __memcpy(&UpData[ByLen], ManageID, sizeof(ManageID));
    ByLen += sizeof(ManageID);
	
	*UpDataLen = ByLen;
}


void WrietFrequeryOffset(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;

	if(FactoryTestFlag != TRUE)
    {
    	__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
    }
	
	U8 *p ;
	PHY_SFO_DEF = phy_get_sfo(SFO_DIR_TX);
	app_printf("PHY_SFO_DEF = %lf" ,PHY_SFO_DEF);
	p =(U8 *)&PHY_SFO_DEF;
	app_printf("PHY_SFO_DEF = %lf ,hex=%02X %02X %02X %02X %02X %02X %02X %02X\n",PHY_SFO_DEF,p[7],p[6],p[5],p[4],p[3],p[2],p[1],p[0]);

	DefSetInfo.PhySFOInfo.PhySFOFattributes = TRUE;
	
	if((PHY_SFO_DEF>-50&&PHY_SFO_DEF<-2)||(PHY_SFO_DEF>2&&PHY_SFO_DEF<50)||PHY_SFO_DEF == 0)
	{
	    DefwriteFg.PhySFO  = TRUE;
		if(FALSE == SetDefaultInfo())
		{
			__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
	    	ByLen += PREAMBLE_CHAR_COUNT;
			*UpDataLen = ByLen;
			return;
		}
	}
	else
	{
		app_printf("PHY_SFO_DEF is too big \n");
	}
	
	__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
	__memcpy(&UpData[ByLen], p , 8);
	ByLen += sizeof(PHY_SFO_DEF);
	*UpDataLen = ByLen;

}

void WriteHardwareDef(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;
    if(CheckDefInfo(&ValidData[4],ValidDataLen-4) == FALSE)
    {
    	os_sleep(60);
        __memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
    }
	
	if(FALSE == SetDefaultInfo())
	{
		os_sleep(60);
        __memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;
	}
	__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
	*UpDataLen = ByLen;

}


void ReadHardwareDef(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen = 0;
	U8 read_flag = TRUE;
	
    if(FALSE == ReadDefaultInfo())
    {
    	read_flag = FALSE;
    }
	if(FALSE == ReadExtInfo())
    {
    	read_flag = FALSE;
    }
	if(FALSE == ReadFreqInfo())
    {
    	read_flag = FALSE;
    }

	if(FALSE == read_flag)
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
		*UpDataLen = ByLen;
		return;	
	}			

	__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    UpData[ByLen++] = 5;
    UpData[ByLen++] = (sizeof(HARDWARE_FEATURE_t)-2)+(sizeof(FLASH_FREQ_INFO_t)-3)+(sizeof(DEVICE_ADDR_INFO_t)-2)
					+(sizeof(FUNTION_SWITCH_t)-2)+(sizeof(PARAMETER_CFG_t)-2);

    DEFAULT_ITEM_e item = DEF_ERR;
	for(item = HARDWARE_DEF; item < MODULEID_DEF; item++)
	{
	    U8  TemuData[50];
        U8  TempDataLen = 0;
		ReadDefInfo(item,TemuData,&TempDataLen);
		UpData[ByLen++] = item;
		UpData[ByLen++] = TempDataLen;
		__memcpy(&UpData[ByLen],TemuData,TempDataLen);
		ByLen += TempDataLen;
	}	
    *UpDataLen = ByLen;

}

void TestMode(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    open_debug_level(DEBUG_PHY);
    open_debug_level(DEBUG_WPHY);

    U8 ByLen;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    FactoryTestFlag = 1;
    
#if defined(ZC3750STA)
    //指示灯闪亮
    PlugValue = 0;
    ReSetValue = 0;
#if defined(STA_BOARD_2_0_01) || defined(STA_BOARD_2_1_00)
    IO_check_timer_Init();
    //timer_start(IOChecktimer);
#endif
    Check_ACDC_timer_start();
#endif

#if defined(ZC3750STA) || defined(ZC3750CJQ2)
    ScanBandManage(e_TEST, 0);
#endif

#if defined(STD_DUAL)

    HPLC.option = 2;
    HPLC.rfchannel = 2;

    wphy_set_option(HPLC.option);
    wphy_set_channel(HPLC.rfchannel);
#endif
#if  defined(ZC3750CJQ2)
	BaudNum = g_meter->baudrate;
	cjq2_search_suspend_timer_modify(10*60*TIMER_UNITS);
#endif
    *UpDataLen = ByLen;
}

/*
void SetDeviceTimeout(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
}

void ReadDeviceTimeout(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
}


void SetSendPowerPara(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
}


void ReadSendPowerPara(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
}
*/


void AreanotifuBuff(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    
    /*
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;

    U16 offset ,i;
	U16 Readnum = 0 ;
	Readnum = ValidData[4] ;//small then 40
	if(Readnum<1 || Readnum>40)
	{
		return;
	}
	if(AreaNotifyBuff.AreaNotifyReacod[AreaNotifyBuff.CurrentEntry].NWkSeq!= 0)
	{
		offset =(AreaNotifyBuff.CurrentEntry+(Readnum-1)*10)%AREA_NOTIFY_COUNT;
	}
	else
	{
		if(AreaNotifyBuff.CurrentEntry >(Readnum-1)*10)
		{
			offset =(Readnum-1)*10 ;
		}
		else
		{
			UpData[ByLen] = 0;
			return;
		}
		
	}

	//offset =(AreaNotifyBuff.CurrentEntry+Readnum*12)%AREA_NOTIFY_COUNT;
	UpData[ByLen] = Readnum ;
	for(i=0;i<10;i++)
	{
		__memcpy (&UpData[ByLen] , &AreaNotifyBuff.AreaNotifyReacod[offset] , 14 );
		ByLen += 14;
		offset = (offset+1)%AREA_NOTIFY_COUNT;
	}*/
}

void LocalUpgrade(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U32  blockreturn = 0xFFFFFFFF;
    U8 ByLen;

    ByLen = 0;
    blockreturn = checkUpgradedata(&ValidData[4],ValidDataLen-4);

    if (blockreturn == 0xFFFFFFFF)
    {
        g_printf_state = TRUE;
    }
    else
    {
        g_printf_state = FALSE;
    }

    if(1)
    {            
		__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
        UpData[ByLen++] = (U8)(blockreturn);
        UpData[ByLen++] = (U8)((blockreturn>>8));
        UpData[ByLen++] = (U8)((blockreturn>>16));
        UpData[ByLen++] = (U8)((blockreturn>>24));
        app_printf("*UpDataLen ->%d\n",*UpDataLen);
	}
	else
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
	}
    *UpDataLen = ByLen;

}

void BaudOpt(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    if(CheckBaudValue(&ValidData[4],ValidDataLen-4) == TRUE)
	{			 
		__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
        UpData[ByLen++] = QueryBaud();
	}
	else
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
	}
    *UpDataLen = ByLen;
}

void NeighbrorList(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    U8  TemuData[200];
    U8  TempDataLen;
    
    ByLen = 0;
	
    if(CheckNeighborInfo(&ValidData[4],ValidDataLen,TemuData,&TempDataLen) == TRUE)
	{			 
		//SetDefaultInfo();
		__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
        __memcpy (&UpData[ByLen] , TemuData , TempDataLen );
        ByLen += TempDataLen;
	}
	else
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
	}
    *UpDataLen = ByLen;
}

void RfPowerSet(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;
    int8_t dbm;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    dbm = ValidData[PREAMBLE_CHAR_COUNT];
    
    wphy_set_pa(dbm);
    app_printf("Set wphy tgain %d!\n", wphy_get_pa());
    UpData[ByLen++] = (U8)(dbm);
    *UpDataLen = ByLen;
}


void RfSendFrame(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
}


void SetLedState(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;

    ByLen = 0;
    
#if defined(ZC3750STA)
    U8  revLen = 0;
    U8  Update[12] = {0};

    if(CheckLedCtrlData(&ValidData[4],ValidDataLen-4,Update,&revLen))
    {            
		__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
        __memcpy(&UpData[ByLen],Update,revLen);
        *UpDataLen += revLen;
        app_printf("*UpDataLen ->%d\n",*UpDataLen);
	}
	else
#endif
	{
		__memcpy(UpData,DATA_ERR,PREAMBLE_CHAR_COUNT);
        ByLen += PREAMBLE_CHAR_COUNT;
	}
    *UpDataLen = ByLen;
}

void SetHRFOP_CHL(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;
    uint8_t option;
    uint8_t channel;

    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    option = ValidData[PREAMBLE_CHAR_COUNT];
    channel = ValidData[PREAMBLE_CHAR_COUNT+1];;
    wphy_set_option(option);
    HPLC.option = option;
    wphy_set_channel(channel);
    HPLC.rfchannel = channel;
    app_printf("option %d channel %d\n", option, channel);

    UpData[ByLen++] = (U8)(option);
    UpData[ByLen++] = (U8)(channel);
    *UpDataLen = ByLen;
}
extern void wphy_hdr_isr_rx(phy_frame_info_t *fi, void *context);
void SetHRF_CAL(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;
    U8 mode_type;
    U8 op_value;
    uint8_t state;
    
    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    mode_type = ValidData[PREAMBLE_CHAR_COUNT];
    op_value = ValidData[PREAMBLE_CHAR_COUNT+1];
    state = op_value;
    app_printf("mode_type %d op_value %d\n", mode_type, op_value);
    if(0 == mode_type)//slave
    {
        if(0 == op_value)//query
        {
            state = rf_cal_get_state();
        }
        else if(1 == op_value)//start
        {
            rf_cal_slave_start();
        }
        else if(2 == op_value)//stop
        {
            rf_cal_slave_stop();
            register_wphy_hdr_isr(wphy_hdr_isr_rx);
            wl_do_next();
        }
        //
    }
    else if(1 == mode_type)//master
    {
        if(0 == op_value)//query
        {
            state = rf_cal_get_state();
        }
        else if(1 == op_value)//start
        {
            phy_reset();
            wphy_reset();
            rf_cal_master_start();
            *UpDataLen = 0;
            return;
        }
        else if(2 == op_value)//stop
        {
            rf_cal_master_stop();
            register_wphy_hdr_isr(wphy_hdr_isr_rx);
            wl_do_next();
        }
        
    }
    UpData[ByLen++] = (U8)(mode_type);
    UpData[ByLen++] = (U8)(state);
    *UpDataLen = ByLen;
}
void ReadHRF_CAL(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
    U8 ByLen;
    U8 best_snr;
    uint32_t time;
    ByLen = 0;
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    best_snr = rf_cal_get_snr();
    time = rf_cal_get_time();
    UpData[ByLen++] = best_snr;
    UpData[ByLen++] = (U8)(time);
    UpData[ByLen++] = (U8)((time>>8));
    UpData[ByLen++] = (U8)((time>>16));
    UpData[ByLen++] = (U8)((time>>24));
    app_printf("Best snr %d Cost %dms.\n", best_snr, time);
    register_wphy_hdr_isr(wphy_hdr_isr_rx);
    wl_do_next();
    *UpDataLen = ByLen;
}

void report_calibration_results(work_t *work)
{
    // timer_stop(calibration_report_timer,TMR_NULL);
    // 68 01 00 00 00 00 00 68 9F 09 87 76 8D BC 51 04 38 33 33 B2 16
    // 68 01 00 00 00 00 00 68 9F 09 87 76 8D BC 49 04 38 33 33 AA 16
    U8 up_buf[21] = {0};
    U8 up_buf_len = 0;
    U8 ByLen = 0;
    U8 best_snr;
    U32 time;
    U8 up_date[9] = {0};

    up_date[ByLen++] = 0x54;
    up_date[ByLen++] = 0x43;
    up_date[ByLen++] = 0x5a;
    up_date[ByLen++] = 0x89;

    best_snr = rf_cal_get_snr();
    time = rf_cal_get_time();

    if (best_snr == 0)
    {
        memset(up_date + ByLen, 0x00, 5);
        ByLen += 5;
    }
    else
    {
        up_date[ByLen++] = best_snr;
        up_date[ByLen++] = (U8)(time);
        up_date[ByLen++] = (U8)((time >> 8));
        up_date[ByLen++] = (U8)((time >> 16));
        up_date[ByLen++] = (U8)((time >> 24));
    }

    Add0x33ForData(up_date,ByLen);

    app_printf("Best snr %d Cost %dms.\n", best_snr, time);
    hplc_hrf_wphy_reset();
    hplc_hrf_phy_reset();
    register_wphy_hdr_isr(wphy_hdr_isr_rx);
    wl_do_next();
    hplc_do_next();

    Packet645Frame(up_buf, &up_buf_len, DevicePib_t.DevMacAddr, DL645EX_UP_CTRCODE, up_date, ByLen);
    if (up_buf_len > 0)
    {
        Extend645PutList(DLT645_1997, BAUDMETER_TIMEOUT, 0, up_buf_len, up_buf);
    }
}

static void calibration_report_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    // report_calibration_results(FALSE);
    // app_printf("calibration timeout\n");
    work_t *post_work = zc_malloc_mem(sizeof(work_t), "calibration_report_timer_cb", MEM_TIME_OUT);
    if (NULL == post_work)
    {
        return;
    }
    post_work->work = report_calibration_results;
    post_work->msgtype = CALIBRATION_REPORT;
    post_app_work(post_work);
}

void modify_calibration_report_timer()
{

	if(calibration_report_timer == NULL)
	{
		calibration_report_timer = timer_create(2.5*TIMER_UNITS,
                              0,
                              TMR_TRIGGER,
                              calibration_report_timer_cb,
                              NULL,
                              "calibration_report_timer_cb"
                             );
	}
	else
	{
		timer_stop(calibration_report_timer,TMR_NULL);
		timer_modify(calibration_report_timer,
						2.5*TIMER_UNITS, 
						0,
						TMR_TRIGGER,
						calibration_report_timer_cb,
						NULL,
						"calibration_report_timer_cb",
						TRUE);
	}
	timer_start(calibration_report_timer);
}

void GWReadChipID(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
	U8 ByLen = 0;

	memset(ManageID,0x00,sizeof(ManageID));
	
	if( VOLUME_PRODUCTION == app_ext_info.func_switch.IDInfoGetModeSWC)//量产
	{
		ReadManageID();
	}
	else
	{
		ReadDefaultInfo();
	}

    __memcpy(UpData,ValidData,ValidDataLen);
    ByLen += ValidDataLen;
	
    UpData[ByLen] = sizeof(ManageID);
    ByLen += 1;
	
    __memcpy(&UpData[ByLen], ManageID, sizeof(ManageID));
    ByLen += sizeof(ManageID);
	
	*UpDataLen = ByLen;
}

void GWReadModuleID(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{ 
	U8 ByLen = 0;
	
	memset(ModuleID,0x00,sizeof(ModuleID));
	
	ReadDefaultInfo();

    __memcpy(UpData,ValidData,ValidDataLen);
    ByLen += ValidDataLen;

    UpData[ByLen] = sizeof(ModuleID);
    ByLen += 1;

    __memcpy(&UpData[ByLen], ModuleID, sizeof(ModuleID));
    ByLen += sizeof(ModuleID);
    *UpDataLen = ByLen;
}

U8 SetMacAddr(U8 *Addr)
{
    U8  NullAddr[6] = {0};
    if(memcmp(NullAddr,Addr,6))
    {
#if defined(STATIC_MASTER)	
        __memcpy(FlashInfo_t.ucJZQMacAddr ,Addr,6);
#elif defined(ZC3750STA)
        __memcpy(DevicePib_t.DevMacAddr ,Addr, 6);
#elif defined(ZC3750CJQ2)
        __memcpy(CollectInfo_st.CollectAddr, Addr,6);
#endif
    return TRUE;
    }

    return FALSE;
}

U8 GetMacAddr(U8 *Addr)
{
    U8  NullAddr[6] = {0};
    
#if defined(STATIC_MASTER)	
    if(memcmp(NullAddr,FlashInfo_t.ucJZQMacAddr,6))
    {
        __memcpy(Addr,FlashInfo_t.ucJZQMacAddr ,6);
        return TRUE;
    }
#elif defined(ZC3750STA)
    if(memcmp(NullAddr,DevicePib_t.DevMacAddr,6))
    {
        __memcpy(Addr,DevicePib_t.DevMacAddr, 6);
        return TRUE;
    }
#elif defined(ZC3750CJQ2)
    if(memcmp(NullAddr,CollectInfo_st.CollectAddr,6))
    {
        __memcpy(Addr,CollectInfo_st.CollectAddr, 6);
        return TRUE;
    }
#endif

    return FALSE;
}
//密钥存储及读取
void set_ecc_key(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{
    U8 res = 0;
    U8 ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    
    res = write_ecc_sm2_info(ValidData+4,e_ECC_KEY,ValidDataLen);
    UpData[ByLen] = res;
    ByLen += 1;
    *UpDataLen = ByLen;
}
void read_ecc_key(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{
    U8 ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    
    read_ecc_sm2_info(&UpData[ByLen],e_ECC_KEY);
    
    ByLen += (sizeof(ECC_KEY_STRING_t)-3);
    *UpDataLen = ByLen;
}
void set_sm2_key(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{
    U8 res = 0;
    U8 ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;

    res = write_ecc_sm2_info(ValidData+4,e_SM2_KEY,ValidDataLen);
    UpData[ByLen] = res;
    ByLen += 1;
    *UpDataLen = ByLen;
}
void read_sm2_key(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{
    U8 ByLen = 0;

    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    
    read_ecc_sm2_info(&UpData[ByLen],e_SM2_KEY);

    ByLen += (sizeof(SM2_KEY_STRING_t)-3);
    *UpDataLen = ByLen;
}
U8 Check_CB_FacTestMode_Proc(uint8_t      *pData ,U16 DataLen)

{
    if(FactoryTestFlag == 1)
    {
        U8  addr[6] = {0};
        U8  AAaddr[6] = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
        U8  deviceaddr[6] = {0};
        U8  FEnum = 0;

        //dump_printfs(pData,DataLen);
        if(TRUE==Check645Frame(pData,DataLen,&FEnum, addr,NULL))
        {
        	U8 buf1[4] = {0x87,0x76,0x8d,0x62};
            if(memcmp(buf1 ,&pData[10+FEnum] , 4 )==0)
        	{
                if(GetMacAddr(deviceaddr) == TRUE)
                {
        		    if(memcmp(addr , deviceaddr , 6 )!=0&&memcmp(addr , AAaddr , 6 )!=0)
                    {         
        			    printf_s("other macaddr \n");
                        dump_printfs(addr,6);
                        //符合工厂测试帧，地址不匹配
                        return FALSE;
                    }
                }
                //app_printf("cb test\n");
        	}
        }
        else
        {
            app_printf("Check645Frame err\n");
        }
    }
    //printf_s("cb test\n");
    //其他情况均进行回传发送
    return TRUE;
}

//集中器初始化串口波特率；



void SetJZQBaud(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{
#if defined(STATIC_MASTER)
	U8 ByLen = 0;
	U8 Baud = 0;
	//U32 Baudnum;

	Baud = ValidData[PREAMBLE_CHAR_COUNT];
    __memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
    ByLen += PREAMBLE_CHAR_COUNT;
    *UpDataLen = ByLen;
	
	app_printf("JZQ baud : %d\n",Baud);
	
	if(Baud < APP_GW3762_9600_BPS || Baud > APP_GW3762_115200_BPS)
	{
		app_printf("baud err\n");
		return;
	}
	
	app_ext_info.param_cfg.JZQBaudCfg = Baud;
	DefwriteFg.ParaCFG = TRUE;
	DefaultSettingFlag = TRUE;
	mutex_post(&mutexSch_t);
	//TODO进行存储;
	//打开功能开关usemode和对应功能开关标志位
	//DefSetInfo.FunctionSwitch.UseMode = TRUE;
	//DefSetInfo.FunctionSwitch.JZQBaudSWC = TRUE;
	//flash写标志位
	
	
#endif
}


void ReadJZQBaud(U8 *ValidData ,U8 ValidDataLen,U8 *UpData,U8 *UpDataLen)
{
#if defined(STATIC_MASTER)
	U8 ByLen = 0;
	U8 baud	= 0;
	__memcpy(UpData,ValidData,PREAMBLE_CHAR_COUNT);
	ByLen += PREAMBLE_CHAR_COUNT;
	
	//读取flash进行确认；
	app_ext_info_read_and_check();
	baud = app_ext_info.param_cfg.JZQBaudCfg;
	
	UpData[ByLen++] = baud;
    *UpDataLen = ByLen;
#endif
}


DLT_07_PROTOCAL Ex645_Protocal[] = 
{
    
    {0x1F , {0x54 , 0x43 , 0x5A , 0x03}  , ReadRelayZone    ,"ReadRelayZone"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x04}  , SetRelayZone     ,"SetRelayZone"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x07}  , ReadOuterVersion     ,"ReadOuterVersion"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x16}  , ReadinnerVersion     ,"ReadinnerVersion"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x22}  , SelfCheckTest     ,"SelfCheckTest"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x23}  , DynamiacCheckTest     ,"DynamiacCheckTest"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x24}  , MemoryCheckTest     ,"MemoryCheckTest"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x25}  , MemoryReadTest     ,"MemoryReadTest"},
    
    {0x1F , {0x54 , 0x43 , 0x5A , 0x2D}  , SetBand     ,"SetBand"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x2E}  , SetPhyCBMode     ,"SetPhyCBMode"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x2F}  , PlcSendFrame     ,"PlcSendFrame"},
    
    {0x1F , {0x54 , 0x43 , 0x5A , 0x3A}  , SetOuterVendorcodeVersion     ,"SetOuterVendorcodeVersion"},
    
    {0x1F , {0x54 , 0x43 , 0x5A , 0x44}  , RebootDevice     ,"RebootDevice"},
    
    {0x1F , {0x54 , 0x43 , 0x5A , 0x5B}  , WriterChipID     ,"WriterChipID"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x5C}  , WriteModuleID     ,"WriteModuleID"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x5D}  , ReadModuleID   ,"ReadModuleID"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x5E}  , ReadChipID     ,"ReadChipID"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x5F}  , WrietFrequeryOffset     ,"WrietFrequeryOffset"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x60}  , WriteHardwareDef     ,"WriteHardwareDef"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x61}  , ReadHardwareDef     ,"ReadHardwareDef"},

    {0x1F , {0x54 , 0x43 , 0x5A , 0x72}  , TestMode     ,"TestMode"},
    
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x7A}  , SetDeviceTimeout     ,"SetDeviceTimeout"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x7B}  , ReadDeviceTimeout     ,"ReadDeviceTimeout"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x7C}  , SetSendPowerPara     ,"SetSendPowerPara"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x7D}  , ReadSendPowerPara     ,"ReadSendPowerPara"},
    
    {0x1F , {0x54 , 0x43 , 0x5A , 0x80}  , AreanotifuBuff     ,"AreanotifuBuff"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x81}  , LocalUpgrade     ,"LocalUpgrade"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x82}  , BaudOpt     ,"BaudOpt"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x83}  , NeighbrorList     ,"NeighbrorList"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x84}  , RfPowerSet     ,"RfPowerSet"},
    //{0x1F , {0x54 , 0x43 , 0x5A , 0x85}  , RfSendFrame     ,"RfSendFrame"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x86}  , SetLedState     ,"SetLedState"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x87}  , SetHRFOP_CHL     ,"SetHRFOP_CHL"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x88}  , SetHRF_CAL     ,"SetHRF_CAL"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x89}  , ReadHRF_CAL     ,"ReadHRF_CAL"},
	{0x1F , {0x54 , 0x43 , 0x5A , 0x8A}  , SetJZQBaud	   ,"SetJZQBaud"},
	{0x1F , {0x54 , 0x43 , 0x5A , 0x8B}  , ReadJZQBaud	   ,"ReadJZQBaud"},
    //面向流水线
    {0x1F , {0xCE , 0x00 , 0x00 , 0x00}  , GWReadChipID     ,"GWReadChipID"},
    {0x1F , {0xCF , 0x00 , 0x00 , 0x00}  , GWReadModuleID     ,"GWReadModuleID"},
    //加密密钥写入方案
    {0x1F , {0x54 , 0x43 , 0x5A , 0x98}  , set_ecc_key     ,"set_ecc_key"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x99}  , read_ecc_key     ,"read_ecc_key"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x9A}  , set_sm2_key     ,"set_sm2_key"},
    {0x1F , {0x54 , 0x43 , 0x5A , 0x9B}  , read_sm2_key     ,"read_sm2_key"},
};

void Extend645PutList(U8 protocol, U16 timeout, U8 frameSeq, U16 frameLen, U8 *pPayload)

{
    add_serialcache_msg_t serialmsg;

    memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));
    #if defined(ZC3750CJQ2)
    serialmsg.baud = BaudNum;
    serialmsg.Uartcfg = meter_serial_cfg;
    #else
    serialmsg.Uartcfg = NULL;
    #endif
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
    
    serialmsg.Match  = NULL;
    serialmsg.Timeout = NULL;
    serialmsg.RevDel = NULL;


    serialmsg.Protocoltype = protocol;
	serialmsg.DeviceTimeout = timeout;
	serialmsg.ack = FALSE;
	serialmsg.FrameLen = frameLen;
	serialmsg.FrameSeq = frameSeq;
	serialmsg.lid =e_Serial_AA;
	serialmsg.ReTransmitCnt = 0;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;


	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pPayload,TRUE);

    return;
}

void ProRdDLT645Extend(U8 *pMsgData , U16 Msglength, U8 *Sendbuf, U8 *SendBufLen)
{
    //不用入参判断 ,645帧格式调用前已经判断过了

    U8  Data[4] = {0};
    U8  FEnum = 0;
    U8  Addr[6] = {0};
    U8  ii = 0;
    U8  Ctrl = 0;
    U8  UpData[256] = {0};
    U8  UpDataLen = 0;
    U8  ValidDataLen = 0;
    U8  ValidData[250] = {0};

    if(Check645Frame(pMsgData,Msglength,&FEnum,Addr,&Ctrl)==TRUE)
    {
        __memcpy(Data , pMsgData + 10 + FEnum , 4);
        Sub0x33ForData(Data , 4);
        app_printf("Ctrl=%02x,VDLen=%d->", Ctrl, pMsgData[9+FEnum]);
        //dump_buf( &pMsgData[10+FEnum], pMsgData[9+FEnum]);
        for( ii = 0 ; ii < (sizeof(Ex645_Protocal)/sizeof(DLT_07_PROTOCAL)) ; ii++)
        {

            ValidDataLen = pMsgData[9+FEnum];//645数据长度
            __memcpy(ValidData,&pMsgData[10+FEnum] , ValidDataLen);
            Sub0x33ForData(ValidData , ValidDataLen);
            dump_buf(ValidData , ValidDataLen);
            //支持97，支持国网扩展流水线长度0x01
            if((Ctrl == DL645EX_CTRCODE)
                && 0 == memcmp(Data , Ex645_Protocal[ii].DataArea , (ValidDataLen>4?4: ValidDataLen)))
            {
                printf_s("Ex645_Pro:%s!\n",Ex645_Protocal[ii].name);

                Ex645_Protocal[ii].Func(ValidData,ValidDataLen,UpData,&UpDataLen);
                //__memmove(UpData,UpData,PREAMBLE_CHAR_COUNT);
                //UpDataLen += PREAMBLE_CHAR_COUNT;
                //__memcpy(UpData,Ex645_Protocal[ii].DataArea,PREAMBLE_CHAR_COUNT);
                if(Ex645_Protocal[ii].Func == TestMode)
                {
                    //检验是07-645下的工厂模式测试帧
                    SetMacAddr(Addr);
                    dump_buf(Addr,6);
                }
                //流水线ID 管理数据域不需要加33 操作。
                if(Ex645_Protocal[ii].Func != GWReadChipID && Ex645_Protocal[ii].Func != GWReadModuleID)
                {
                    Add0x33ForData(UpData,UpDataLen);
                }
                else    //上面将数据标识字段做了减33处理，这里要加回去
                {
                    Add0x33ForData(UpData,1);
                }
                GetMacAddr(Addr);
                dump_buf(Addr,6);
                Packet645Frame(Sendbuf, SendBufLen, Addr, DL645EX_UP_CTRCODE, UpData, UpDataLen);
                //send
                break;
            }
        }
        if(ii >= (sizeof(Ex645_Protocal)/sizeof(DLT_07_PROTOCAL)))
        {
            //回复0xDF
            //SendDFCtrlCode();
            app_printf("645 Err Code!\n");
        }
        //send To rdCtrl
    }
}


void ProDLT645Extend(U8 *pMsgData , U16 Msglength)
{
    //不用入参判断 ,645帧格式调用前已经判断过了

    U8  Data[4] = {0};
    U8  FEnum = 0;
    U8  Addr[6] = {0};
    U8  ii = 0;
    U8  Ctrl = 0;
    U8  *UpData = zc_malloc_mem(256, "UpData", MEM_TIME_OUT);
    U8  UpDataLen = 0;
    U8  *Sendbuf = zc_malloc_mem(256, "Sendbuf", MEM_TIME_OUT);
    U8  SendBufLen = 0;
    U8  ValidDataLen = 0;
    U8  *ValidData = zc_malloc_mem(256, "ValidData", MEM_TIME_OUT);
	
    if(Check645Frame(pMsgData,Msglength,&FEnum,Addr,&Ctrl)==TRUE)
    {
        __memcpy(Data , pMsgData + 10 + FEnum , 4);
        Sub0x33ForData(Data , 4);
        app_printf("Ctrl = %02x, ValidDataLen = %d --> ", Ctrl, pMsgData[9+FEnum]);
        //dump_buf( &pMsgData[10+FEnum], pMsgData[9+FEnum]);
        for( ii = 0 ; ii < (sizeof(Ex645_Protocal)/sizeof(DLT_07_PROTOCAL)) ; ii++)
        {
            ValidDataLen = pMsgData[9+FEnum];//645数据长度
            __memcpy(ValidData,&pMsgData[10+FEnum] , ValidDataLen);
            Sub0x33ForData(ValidData , ValidDataLen);
            //dump_buf(ValidData , ValidDataLen);
            //支持97，支持国网扩展流水线长度0x01
            if((Ctrl == DL645EX_CTRCODE)
                && 0 == memcmp(Data , Ex645_Protocal[ii].DataArea , (ValidDataLen>4?4: ValidDataLen)))
            {
                app_printf("Ex645_Protocal   :  %s!\n",Ex645_Protocal[ii].name);

                Ex645_Protocal[ii].Func(ValidData,ValidDataLen,UpData,&UpDataLen);
                //__memmove(UpData,UpData,PREAMBLE_CHAR_COUNT);
                //UpDataLen += PREAMBLE_CHAR_COUNT;
                //__memcpy(UpData,Ex645_Protocal[ii].DataArea,PREAMBLE_CHAR_COUNT);
                if(Ex645_Protocal[ii].Func == TestMode)
                {
                    //检验是07-645下的工厂模式测试帧
                    SetMacAddr(Addr);
                    dump_buf(Addr,6);
                }
                //流水线ID 管理数据域不需要加33 操作。
                if(Ex645_Protocal[ii].Func != GWReadChipID && Ex645_Protocal[ii].Func != GWReadModuleID)
                {
                    Add0x33ForData(UpData,UpDataLen);
                }
                else    //上面将数据标识字段做了减33处理，这里要加回去
                {
                    Add0x33ForData(UpData,1);
                }
                GetMacAddr(Addr);
                dump_buf(Addr,6);
                if (UpDataLen>0)
                {
                    Packet645Frame(Sendbuf, &SendBufLen, Addr, DL645EX_UP_CTRCODE, UpData, UpDataLen);
                }               
                
                //send
                break;
            }
        }
        if(ii >= (sizeof(Ex645_Protocal)/sizeof(DLT_07_PROTOCAL)))
        {
        	//回复0xDF
        	//SendDFCtrlCode();
        	app_printf("645 Err Code!\n");
        }
        //send by serial uart
        if(SendBufLen > 0)
        {
            Extend645PutList(DLT645_1997,BAUDMETER_TIMEOUT,0,SendBufLen, Sendbuf);
        }
    }
	
    zc_free_mem(UpData);
    zc_free_mem(Sendbuf);
    zc_free_mem(ValidData);
}
void  deal_set_hrf_cal()
{
    U8 Addr[6] = {0};
    U8 set_hrf_cal_logo[4] = {0x54,0x43,0x5a,0x89};
    U8 ByLen;
    U8 best_snr;
    uint32_t time;
    ByLen = 0;
    U8  *hrf_sendbuf = zc_malloc_mem(10, "hrf_sendbuf", MEM_TIME_OUT);
    U16  hrf_sendbuflen =0;
    U8  *sendbuf = zc_malloc_mem(50, "sendbuf", MEM_TIME_OUT);
    U8  sendbuflen = 0;
    __memcpy(hrf_sendbuf,set_hrf_cal_logo,sizeof(set_hrf_cal_logo));
    ByLen += PREAMBLE_CHAR_COUNT;
    best_snr = rf_cal_get_snr();
    time = rf_cal_get_time();
    
    hrf_sendbuf[ByLen++] = best_snr;
    hrf_sendbuf[ByLen++] = (U8)(time);
    hrf_sendbuf[ByLen++] = (U8)((time>>8));
    hrf_sendbuf[ByLen++] = (U8)((time>>16));
    hrf_sendbuf[ByLen++] = (U8)((time>>24));
    app_printf("Best snr %d Cost %dms.\n", best_snr, time);
    
    hrf_sendbuflen = ByLen;
    GetMacAddr(Addr);
    dump_buf(Addr,6);
    Add0x33ForData(hrf_sendbuf,hrf_sendbuflen);
    Packet645Frame(sendbuf, &sendbuflen, Addr, DL645EX_UP_CTRCODE, hrf_sendbuf, hrf_sendbuflen);

    if(sendbuflen > 0)
    {
        Extend645PutList(DLT645_2007,BAUDMETER_TIMEOUT,0, sendbuflen,sendbuf);
    }

    register_wphy_hdr_isr(wphy_hdr_isr_rx);
    wl_do_next();
    zc_free_mem(hrf_sendbuf);
    zc_free_mem(sendbuf);
}
static void hrf_cal_ctrltimerCB(struct ostimer_s *ostimer, void *arg)
{
    //fe fe fe fe 68 01 00 00 00 00 00 68 1f 06 87 76 8d bb 34 34 a3 16 
   if(!rf_cal_get_snr())
   {
        app_printf("best_snr null\n");
   }
   deal_set_hrf_cal();//超时直接回复
   rf_cal_master_stop();
   register_wphy_hdr_isr(wphy_hdr_isr_rx);
   wl_do_next(); 
}

void modify_hrf_cal_ctrl_timer(uint32_t MS)
{

	if(NULL == hrf_cal_ctrltimer)
	{
	    hrf_cal_ctrltimer = timer_create(MS,
								  0,
								  TMR_TRIGGER,
								  hrf_cal_ctrltimerCB,
								  NULL,
								  "hrf_cal_ctrltimerCB");
	}
	else
	{
		if(MS == 0)
	    {
	        MS = 1;
        }
		timer_modify(hrf_cal_ctrltimer,
				MS,
				0,
				TMR_TRIGGER ,//TMR_TRIGGER
				hrf_cal_ctrltimerCB,
				NULL,
				"hrf_cal_ctrltimerCB",
				TRUE);
	}
	timer_start(hrf_cal_ctrltimer);
	
}

#if defined(ZC3750CJQ2)
static void InfraredReadMeterPutList(U8 protocol,U8 baud, U16 timeout, U8 intervaltime,
										U8 retry,U16 frameSeq, U16 frameLen, U8 *pPayload, U8 lastFlag,U8 IN_MeterList);
/*
U8 CheckMeterProtocol(U8 ControlData , U8 FrameDataLength)
{
    if((ControlData >= 0x11 && ControlData <= 0x1D )
        ||(ControlData == 0x03 && FrameDataLength != 0))
    {
        return METER_PROTOCOL_07;
    }
    else if(ControlData == 0x01 || ControlData == 0x02 ||
            (ControlData == 0x03 && FrameDataLength == 0) ||
            ControlData == 0x04 || ControlData == 0x0A ||
            ControlData == 0x0C ||ControlData == 0x0F ||
            ControlData == 0x10)
    {
        return METER_PROTOCOL_97;
    }
    else
    {
        return METER_PROTOCOL_RES;
    }
}
*/

typedef struct
{
    U8 ControlData ;
    U8 DataArea[4] ;
    void (* Func) (U8 *pMsgData);
}ST_PrivateProForRed;


ST_PrivateProForRed Red_Protocal[14] = 
{
    {0x1F , {0x01+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , ReadSoftVersion},
    {0x1F , {0x02+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , ReadSearchTableInterval},
    {0x1F , {0x03+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , SetSearchTableInterval},
    {0x1F , {0x04+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , ParameterInit},
    {0x1F , {0x05+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , DataInit},
    {0x1F , {0x06+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , ParameterAndDataInit},
    {0x1F , {0x07+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , ReadMeterNum},
    {0x1F , {0x08+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , ReadMeterAddr},
    {0x1F , {0x01+0x33 , 0x04+0x33 , 0x00+0x33 , 0x04+0x33}  , ReadCollectorAddr},
    {0x1F , {0x02+0x33 , 0x04+0x33 , 0x00+0x33 , 0x04+0x33}  , SetCollectorAddr},
    {0x1F , {0x03+0x33 , 0x01+0x33 , 0x90+0x33 , 0x04+0x33}  , Read485Overtime},
    {0x1F , {0x04+0x33 , 0x01+0x33 , 0x90+0x33 , 0x04+0x33}  , Set485Overtime},
    {0x1F , {0x00+0x33 , 0x00+0x33 , 0x14+0x33 , 0x03+0x33}  , ReadCollectorRebootCount},    
    {0x1F , {0x0d+0x33 , 0x00+0x33 , 0x80+0x33 , 0x04+0x33}  , Start_StopSearchMeter}, 
};

U8 SendBuf[255] = {0};

//#define U8TOBCD(data) ((((data/ 10)<<4) & 0xF0 ) | ((data % 10) & 0x0F))

void ReadSoftVersion(U8 *pMsgData)
{
	VERSION_ST* pstversion = CJQ_Extend_ReadVersion();
    memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    //memset(SendBuf+1 , 0xAA , 6);
    __memcpy(SendBuf+1,CollectInfo_st.CollectAddr,MACADRDR_LENGTH);
    //SendBuf[7] = 0x68

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x0D;
    SendBuf[10] = 0x34;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;
    SendBuf[14] = pstversion->Vendorcode[0]+0x33;
    SendBuf[15] = pstversion->Vendorcode[1]+0x33;
    
    SendBuf[16] = ZC3750CJQ2_chip2+ 0x33;//芯片型号
    SendBuf[17] = ZC3750CJQ2_chip1 + 0x33;;
    SendBuf[18] = U8TOBCD(Date_D) + 0x33;
    SendBuf[19] = U8TOBCD(Date_M) + 0x33;
    SendBuf[20] = U8TOBCD(Date_Y) + 0x33;
    SendBuf[21] = ZC3750CJQ2_ver2 + 0x33;
    SendBuf[22] = ZC3750CJQ2_ver1 + 0x33;

    
    U8 Sum = 0;
    U8 i = 0 ;
    for(i=0; i<23; i++)
    {
        Sum += *(SendBuf + i);
    }
    SendBuf[23] = Sum ;
    SendBuf[24] = 0x16;

    infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}
void ReadSearchTableInterval(U8 *pMsgData)
{
	memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;
    SendBuf[10] = 0x35;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;
    SendBuf[14] = CJQ_Extend_ReadInterval()+ 0x33;

    
    U8 Sum = 0;
    U8 i = 0 ;
    for(i=0; i<15; i++)
    {
        Sum += *(SendBuf + i);
    }
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;

    infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}
void SetSearchTableInterval(U8 *pMsgData)
{

	memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;
    SendBuf[10] = 0x36;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;
	
	if(0 >= (pMsgData[14] - 0x33))
	{
		SendBuf[14] = 0x01 + 0x33;
	}
	else
	{
		CJQ_Extend_SetInterval(pMsgData[14] - 0x33);
    	SendBuf[14] = 0x00 + 0x33;
	}
	
    U8 Sum = 0;
    U8 i = 0 ;
    for(i=0; i<15; i++)
    {
        Sum += *(SendBuf + i);
    }
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;

    infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}


void ParameterInit(U8 *pMsgData)
{
    memset(SendBuf , 0x00 , sizeof(SendBuf));
	
    CollectInfo_st.Interval = INTERVAL;
	CollectInfo_st.Timeout = TIME_OUT;
    //CJQ_Extend_ParaInit();
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;

    SendBuf[10] = 0x37;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;

    SendBuf[14] = 0x00 + 0x33;

    U8 Sum = 0;
    U8 i = 0 ;
    for(i=0; i<15; i++)
    {
        Sum += *(SendBuf + i);
    }
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;

	infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}
//采集器参数初始化命令是用来清除采集器冻结数据的命令
void DataInit(U8 *pMsgData)
{
	U8	MacType = e_UNKONWN;
    memset(SendBuf , 0x00 , sizeof(SendBuf));
	
    //CJQ_Extend_ParaInit();

	if(COLLECT_ADDRMODE_NOADDR == CollectInfo_st.AddrMode)
	{
		memset(CollectInfo_st.CollectAddr, 0xAA, MACADRDR_LENGTH);
	}
	else
	{
		memset(CollectInfo_st.CollectAddr, 0xBB, MACADRDR_LENGTH);
	}

    SetMacAddrRequest(CollectInfo_st.CollectAddr, e_CJQ_2,e_MAC_METER_ADDR);
	if(GetNodeState() == e_NODE_ON_LINE && DevicePib_t.PowerOffFlag == e_power_is_ok)
    {
        //需要离线
        net_nnib_ioctl(NET_SET_OFFLINE,NULL);
        net_nnib_ioctl(NET_SET_MACTYPE,&MacType);
    }

	
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;

    SendBuf[10] = 0x38;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;

    SendBuf[14] = 0x00 + 0x33;

    U8 Sum = 0;
    U8 i = 0 ;
    for(i=0; i<15; i++)
    {
        Sum += *(SendBuf + i);
    }
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;

	infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);

	//上行回复完之后 开始搜表
	CollectInfo_st.MeterNum = 0;
	memset((U8*)(&CollectInfo_st.MeterList[0]), 0x00, sizeof(METER_INFO_ST) * METERNUM_MAX);
	app_printf("CJQ_AA_Begin\n");
	cjq2_search_meter_info_t.Ctrl.ProtocolBaud = 0;
	cjq2_search_meter_info_t.Ctrl.Protocol = METER_PROTOCOL_RES;
	cjq2_search_meter_info_t.Ctrl.Baud = METER_BAUD_RES;
	CJQ_AA_Begin(&cjq2_search_meter_info_t.Ctrl);
	cjq2_search_meter_timer_modify(0.1*TIMER_UNITS,TMR_TRIGGER);

    return ;

}
//采集器参数及数据初始化命令是用来清除采集器冻结数据、清除采集器内部电表档案和参数恢复出厂默认值命令
void ParameterAndDataInit(U8 *pMsgData)
{
    memset(SendBuf , 0x00 , sizeof(SendBuf));
	

   
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;

    SendBuf[10] = 0x39;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;

    SendBuf[14] = 0x00 + 0x33;

    U8 Sum = 0;
    U8 i = 0 ;
    for(i=0; i<15; i++)
    {
        Sum += *(SendBuf + i);
    }
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;

	infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
	CJQ_Extend_ParaInit();
    return ;

}
void ReadMeterNum(U8 *pMsgData)
{
    memset(SendBuf , 0x00 , sizeof(SendBuf));
    U8 Sum = 0;
    U8 i = 0 ;
    U8 meternum = 0;
	
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;

    SendBuf[10] = 0x3A;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;
	meternum = CJQ_Extend_MeterNum();
	meternum = ((((meternum / 10)<<4) & 0xF0 ) | ((meternum % 10) & 0x0F)) ;
	//app_printf("[ReadMeterNum] meternum = %d \r\n" , meternum);
	
    SendBuf[14] = meternum  + 0x33;
    
    for(i=0; i<15; i++)
    {
       Sum += *(SendBuf + i);
    }    
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;
	
	infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}

void ReadMeterAddr(U8 *pMsgData)
{
    U8 Sum = 0;
    U8 i = 0 ;
	U8 j = 0;
    U8 MeterNumTmp = 0 ;
    U8 * pData = NULL;
    U8 rx_count = 0;
	METER_INFO_ST* pArrmeter = CJQ_Extend_MeterInfo();
    MeterNumTmp = CJQ_Extend_MeterNum();

    while(MeterNumTmp - METERNUM_MAX > 0)
    {
    	Sum = 0;
        memset(SendBuf , 0x00 , sizeof(SendBuf));

        SendBuf[0] = 0x68;
        SendBuf[7] = 0x68;
        memset(SendBuf+1 , 0xAA , 6);

        SendBuf[8] = 0x9F;
        SendBuf[9] = 0xE5;
        SendBuf[10] = 0x3B;
        SendBuf[11] = 0x33;
        SendBuf[12] = 0xB3;
        SendBuf[13] = 0x37;
        SendBuf[14] = 0x53;
        
        pData = &SendBuf[15];

        for( i = 0 ; i < METERNUM_MAX ; i++)
        {
            __memcpy(pData , pArrmeter + (rx_count*METERNUM_MAX + i) , sizeof(METER_INFO_ST));
			for( j = 0 ;  j < 7 ;j++)
			{
				pData[j] = pData[j] + 0x33;
			}
			
            pData += 7;//sizeof(METER_INFO_ST);
        }
        
        for(i=0; i<239; i++) // 239 = 7*32 + 5 + 10
        {
            Sum += *(SendBuf + i);
        }    
        SendBuf[239] = Sum ;
        SendBuf[240] = 0x16;
        infrared_send(SendBuf ,SendBuf[9]+12);
		dump_buf(SendBuf , SendBuf[9]+12);
        rx_count++;    
        MeterNumTmp -= METERNUM_MAX;      
		os_sleep(200);
    }
    
    memset(SendBuf , 0x00 , sizeof(SendBuf));
    Sum = 0;
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);
    
    SendBuf[8] = 0x9F;
    SendBuf[9] = MeterNumTmp * 7 + 5;
    SendBuf[10] = 0x3B;
    SendBuf[11] = 0x33;
    SendBuf[12] = 0xB3;
    SendBuf[13] = 0x37;
    SendBuf[14] = MeterNumTmp + 0x33;
    
    pData = &SendBuf[15];
    
    for( i = 0 ; i < MeterNumTmp ; i++)
    {
        __memcpy(pData , pArrmeter + (rx_count*METERNUM_MAX + i) , sizeof(METER_INFO_ST));
		for( j = 0 ;  j < 7 ;j++)
		{
			pData[j] = pData[j] + 0x33;
		}
        pData += 7;//sizeof(METER_INFO_ST);
    }
    
    for(i=0; i < MeterNumTmp * 7 + 5 + 10; i++)
    {
        Sum += *(SendBuf + i);
    }    
    SendBuf[MeterNumTmp * 7 + 5 + 10 ] = Sum ;
    SendBuf[MeterNumTmp * 7 + 5 + 10 +1 ] = 0x16;
    infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}

void ReadCollectorAddr(U8 *pMsgData)
{
    U8 Sum = 0;
    U8 i = 0 ;  
	U8 Addr[6] = {0};
	U8* cjqaddr = CJQ_Extend_ReadBaseAddr();
	
    memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    __memcpy(SendBuf+1 , cjqaddr , 6);

    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x0A;

    SendBuf[10] = 0x34;
    SendBuf[11] = 0x37;
    SendBuf[12] = 0x33;
    SendBuf[13] = 0x37;

    for( i = 0 ; i < 6 ; i++)
	{
		Addr[i] = cjqaddr[i] + 0x33;
	}
	
    __memcpy(SendBuf + 14 , Addr , 6);

    for(i=0; i<20; i++)
    {
       Sum += *(SendBuf + i);
    }  
	
    SendBuf[20] = Sum ;
    SendBuf[21] = 0x16;
	infrared_send(SendBuf ,SendBuf[9]+12);
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}
void SetCollectorAddr(U8 *pMsgData)
{
    U8 Sum = 0;
    U8 i = 0 ;  
	U8 flag=0;
    
    memset(SendBuf , 0x00 , sizeof(SendBuf));
	SendBuf[0] = 0x68;
	SendBuf[7] = 0x68;
	memset(SendBuf+1 , 0xAA , 6);
	
    U8 Addr[6] = {0};
    __memcpy(Addr , pMsgData + 14 , 6);
	
	for(i = 0 ; i < 6 ; i++)
	{
		Addr[i]-= 0x33;
		if(Addr[i] >>4 >=10 || (Addr[i] & 0xf)>= 10)
		{
			app_printf("Not BCD format,2c fail\n");
			flag=1;//非BCD格式
			break;
		}
	}
	if(!flag)
	CJQ_Extend_SetBaseAddr(Addr);

	SendBuf[8] = 0x9F;
	SendBuf[9] = 0x05;

	SendBuf[10] = 0x35;
	SendBuf[11] = 0x37;
	SendBuf[12] = 0x33;
	SendBuf[13] = 0x37;
	if(flag)
		SendBuf[14] = 0x01 + 0x33;//set fail
	else
		SendBuf[14] = 0x00 + 0x33;//set success

	for(i=0; i<15; i++)
	{
	   Sum += *(SendBuf + i);
	}    
	SendBuf[15] = Sum ;
	SendBuf[16] = 0x16;
	infrared_send(SendBuf ,SendBuf[9]+12);		
	dump_buf(SendBuf , SendBuf[9]+12);		
	return ;  
}
void Read485Overtime(U8 *pMsgData)
{
    
    U8 Sum = 0;
    U8 i = 0 ;  

    memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);
    
    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;

    SendBuf[10] = 0x36;
    SendBuf[11] = 0x34;
    SendBuf[12] = 0xC3;
    SendBuf[13] = 0x37;

	U8 Set_timeout = CJQ_Extend_ReadTimeout();
    SendBuf[14] = Set_timeout + 0x33;

    
    for(i=0; i<15; i++)
    {
       Sum += *(SendBuf + i);
    }    
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;
    
	infrared_send(SendBuf ,SendBuf[9]+12);	
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}


void Set485Overtime(U8 *pMsgData)
{
    
    U8 Sum = 0;
    U8 i = 0 ;  
    
    memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);
    
    SendBuf[8] = 0x9F;
    SendBuf[9] = 0x05;

    SendBuf[10] = 0x37;
    SendBuf[11] = 0x34;
    SendBuf[12] = 0xC3;
    SendBuf[13] = 0x37;
	if(0 >= (*(pMsgData+ 14) - 0x33) || 5 < (*(pMsgData+ 14) - 0x33))//可设置有效范围0~5秒(不包含0)
	{
		app_printf("the time is not between 0~5 seconds\n");
		SendBuf[14] = 0x34; //01 fail
	}
	else
	{
		CJQ_Extend_SetTimeout(*(pMsgData+ 14) - 0x33);
		SendBuf[14] = 0x33; //00 success
    }
	
    for(i=0; i<15; i++)
    {
       Sum += *(SendBuf + i);
    }    
    SendBuf[15] = Sum ;
    SendBuf[16] = 0x16;
	
	infrared_send(SendBuf ,SendBuf[9]+12);	
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}


void ReadCollectorRebootCount(U8 *pMsgData)
{
    U8 Sum = 0;
    U8 i = 0 ;  
	U32 resetnum = CJQ_Extend_ReadResetNum();
    
    memset(SendBuf , 0x00 , sizeof(SendBuf));
	
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);	
	
	SendBuf[8] = 0x9F;
	SendBuf[9] = 0x08;
	
	SendBuf[10] = 0x33;
	SendBuf[11] = 0x33;
	SendBuf[12] = 0x47;
	SendBuf[13] = 0x36;
	
	SendBuf[14] = (resetnum & 0xFF) + 0x33;
	SendBuf[15] = ((resetnum >> 8 )& 0xFF) + 0x33;
	SendBuf[16] = ((resetnum >> 16 )& 0xFF) +  0x33;
	SendBuf[17] = ((resetnum >> 24 )& 0xFF) +  0x33;
	
	for(i=0; i<18; i++)
    {
       Sum += *(SendBuf + i);
    }    
    SendBuf[18] = Sum ;
    SendBuf[19] = 0x16;

	infrared_send(SendBuf ,SendBuf[9]+12);	
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}


void SendDFCtrlCode()
{
    
    U8 Sum = 0;
    U8 i = 0 ;  

    memset(SendBuf , 0x00 , sizeof(SendBuf));
    
    SendBuf[0] = 0x68;
    SendBuf[7] = 0x68;
    memset(SendBuf+1 , 0xAA , 6);
    
    SendBuf[8] = 0xDF;
    SendBuf[9] = 0x00;

    
    for(i=0; i<15; i++)
    {
       Sum += *(SendBuf + i);
    }    
    SendBuf[10] = Sum ;
    SendBuf[11] = 0x16;
    
	infrared_send(SendBuf ,SendBuf[9]+12);	
	dump_buf(SendBuf , SendBuf[9]+12);
    return ;
}


void Start_StopSearchMeter(U8 *pMsgData)
{
	if(*(pMsgData+ 14) - 0x33)
	{
		if(cjq2_search_meter_info_t.Ctrl.SearchState != SEARCH_STATE_STOP)
		{
			app_printf("CJQ is running search meter!\n");
			return;
		}
		else
		{
			app_printf("CJQ search meter is waiting\n");
		}
	}
	else
	{
		app_printf("CJQ stop search meter!\n");
		cjq2_search_meter_info_t.Ctrl.SearchState = SEARCH_STATE_STOP;
	}
	return;
}



 void ProcPrivateProForRed(U8 *pMsgData , U8 Msglength)
{
    //不用入参判断 ,645帧格式调用前已经判断过了

    U8 Data[4] = {0};
    __memcpy(Data , pMsgData + 10 , 4);
    U8 ii = 0;

	static U8 FEnum = 0;
    static U8 FreamAddr[6] = {0};
	app_printf(" Infrared CJQ \n");
	if(TRUE == Check645Frame(pMsgData,Msglength , &FEnum, FreamAddr,NULL))
	{
		for( ii = 0 ; ii < 14 ; ii++)
		{
			if(0 == memcmp(Data , Red_Protocal[ii].DataArea , 4))
			{
				
				app_printf("ProcPrivateProForRed   :  %s!\n",ii==0?"ReadSoftVersion":
															ii==1?"ReadSearchTableInterval":
															ii==2?"SetSearchTableInterval":
															ii==3?"ParameterInit":
															ii==4?"DataInit":
															ii==5?"ParameterAndDataInit":
															ii==6?"ReadMeterNum":
															ii==7?"ReadMeterAddr":
															ii==8?"ReadCollectorAddr":
															ii==9?"SetCollectorAddr":
															ii==10?"Read485Overtime":
															ii==11?"Set485Overtime":
															ii==12?"ReadCollectorRebootCount":
															ii==13?"start/stop search meter":
																	"UNKOWN");
				
				Red_Protocal[ii].Func(pMsgData);
		
				break;
			}
		}
		if(ii >= 14)
		{
			ReadMeterForInfrared(DLT645_2007,FreamAddr,pMsgData,Msglength);
		}
	}
	else if(Check698Frame(pMsgData,Msglength , &FEnum, FreamAddr,NULL) == TRUE)
	{
		ReadMeterForInfrared(DLT698,FreamAddr,pMsgData,Msglength);
	}
	else
	{
		app_printf("CJQ Err Code!\n");
		SendDFCtrlCode();
	}
}


 READ_METER_IND_t * pTestpReadMeterInd =NULL ;
 READ_METER_IND_t TestpReadMeterInd ; 


 //645试抄列表
 U8 Baud_645[]={
	 METER_BAUD_2400,
	 METER_BAUD_9600,
	 METER_BAUD_4800,
	 METER_BAUD_1200,
	 METER_BAUD_19200
 };
 
 //698试抄列表
 U8 Baud_698[]={
	 METER_BAUD_9600,
	 METER_BAUD_2400,
	 METER_BAUD_4800,
	 METER_BAUD_1200,
	 METER_BAUD_19200
 };

 static void get_send_band(U8 Protocoltype,U8 * pbaud )
 {
	 U8 baud =*pbaud;
	 U8 i = 0;
	 if(Protocoltype == DLT698 )
	 {
		 //698切下一波特率
		 for(i=0;i<sizeof(Baud_698);i++)
		 {
			 if(baud == Baud_698[i])
			 {
				 baud = Baud_698[i+1];
				 break;
			 }
		 }
	 }
	 else
	 {
		 //645切下一波特率
		 for(i=0;i<sizeof(Baud_645);i++)
		 {
			 if(baud==Baud_645[i])
			 {
				 baud = Baud_645[i+1];
				 break;
			 }
		 }
	 }
	 *pbaud = baud;
 }


 static U8 test_read_meter_check_baud(U8 arg_protocol, U8 arg_baud)
 {
	 if(arg_protocol == DLT698)
	 {
		 if(arg_baud == Baud_698[sizeof(Baud_698) - 1])
		 {
			 return FALSE;
		 }
	 }
	 else
	 {
		 if(arg_baud == Baud_645[sizeof(Baud_645) - 1])
		 {
			 return FALSE;
		 }
	 }
	 return TRUE;
 }
 
 static void InfraredReadMeterTimeout_noin_list(void *pMsgNode)
 {
	 app_printf("Infrared time out!\n");
 }


static U8 InfraredReadMeterMatch(U8 sendprtcl, U8 rcvprtcl,            U8 sendFrameSeq, U8 recvFrameSeq)
{
    U8 result = FALSE;

    if(sendprtcl == rcvprtcl)
    {
        result = TRUE;
    }
    if(sendprtcl == DLT645_1997 && rcvprtcl == DLT645_2007)
    {
        result = TRUE;
    }
    return result;
}

U8 GetBaudcntbyBaudrate(U32 Baud)
{
     U8  BaudValue;
     (Baud == 1200)? (BaudValue = METER_BAUD_1200):
        (Baud == 2400)? (BaudValue = METER_BAUD_2400):
            (Baud == 4800)? BaudValue = METER_BAUD_4800:
                (Baud == 9600)? BaudValue = METER_BAUD_9600:
                    (Baud == 19200)? BaudValue = METER_BAUD_19200:
                        (Baud == 115200)? BaudValue = METER_BAUD_115200:
                            (Baud == 460800)? BaudValue = METER_BAUD_460800:
                                (Baud == 38400)? BaudValue = METER_BAUD_38400:
                                    (Baud == 57600)? BaudValue = METER_BAUD_57600:
                                        (Baud == 230400)? BaudValue = METER_BAUD_230400:
                                            (BaudValue = 0);
    return  BaudValue;
}

static void InfraredReadMeterTimeout(void *pMsgNode)
{
	add_serialcache_msg_t * msg = pMsgNode;
	U8 baud = GetBaudcntbyBaudrate(msg->baud);
	U8 Seq = msg->FrameSeq;
	//U8 i=0;
	//更换下一个波特率
	if(test_read_meter_check_baud(msg->Protocoltype, baud) == FALSE)
	{
		pTestpReadMeterInd = NULL;
		return;
	}
	
	get_send_band(msg->Protocoltype,&baud);
	app_printf("Infrared Next Band!\n");
	Seq++;
	cjq2_search_suspend_timer_modify(20*TIMER_UNITS);
   	InfraredReadMeterPutList( msg->Protocoltype,baud,1*TIMER_UNITS,0,0,Seq,msg->FrameLen,msg->FrameUnit,1,0);

}

static void InfraredReadMeterRecvDeal(void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
	static U8 FEnum = 0;
    static U8 FreamAddr[6] = {0};
	add_serialcache_msg_t * msg = pMsgNode;
	//U8 band= 0;
	//U8 port =0;
	pTestpReadMeterInd = NULL;
	//获取addr//搜表列表是否已存在
	if(((TRUE == Check645Frame(revbuf,revlen , &FEnum, FreamAddr,NULL))||(TRUE == Check698Frame(revbuf,revlen , &FEnum, FreamAddr,NULL)))
		&&(FALSE == CJQ_CheckMeterExistByAddr(FreamAddr,NULL,NULL)))
	{
		//添加至搜表列表
		CJQ_AddMeterInfo(FreamAddr,msg->Protocoltype, msg->baud);
	}

	infrared_send(revbuf ,revlen);	
	dump_buf(revbuf , revlen);
    return ;
}


void TestStaReadMeterRecvDeal (void *pMsgNode, uint8_t *revbuf, uint16_t revlen, uint8_t protocoltype, uint16_t frameSeq)
{
	static U8 FEnum = 0;
    static U8 FreamAddr[6] = {0};
	add_serialcache_msg_t * msg = pMsgNode;
	pTestpReadMeterInd = NULL; 
	//获取addr//搜表列表是否已存在
	if(((TRUE == Check645Frame(revbuf,revlen , &FEnum, FreamAddr,NULL))||(TRUE == Check698Frame(revbuf,revlen , &FEnum, FreamAddr,NULL)))
		&&(FALSE == CJQ_CheckMeterExistByAddr(FreamAddr,NULL,NULL)))
	{
		//添加至搜表列表
		CJQ_AddMeterInfo(FreamAddr,msg->Protocoltype, msg->baud);
	}
	sta_read_meter_recv_deal(pMsgNode,revbuf,revlen,protocoltype,frameSeq);
}


void TestStaReadMeterTimeout (void *pMsgNode)
{
	add_serialcache_msg_t * msg = pMsgNode;
	
	U8 baud = GetBaudcntbyBaudrate(msg->baud);
	U8 Seq = msg->FrameSeq;
		
	if(test_read_meter_check_baud(msg->Protocoltype, baud) == FALSE)
	{
		pTestpReadMeterInd = NULL; 
		return;
	}

	get_send_band(msg->Protocoltype,&baud);
	app_printf("Test Next Band %d!\n",baud);
	Seq++;
	cjq2_search_suspend_timer_modify(20*TIMER_UNITS);
   	//StaReadMeterPutList(msg->StaReadMeterInfo ,msg->Protocoltype,baud,1000,0,0,Seq,msg->FrameLen,msg->FrameUnit,1,0);
   	sta_read_meter_put_list(pTestpReadMeterInd,msg->Protocoltype,baud,msg->DeviceTimeout,msg->IntervalTime, msg->ReTransmitCnt ,Seq,msg->FrameLen,msg->FrameUnit,msg->StaReadMeterInfo.LastFlag,0);
}


void Test_Read_Meter (U8 InfraredOrCCO,void *pReadMeterInd, U8 protocol, U16 timeout, 
                                              U8 intervaltime,U8 retry,U16 frameSeq, U16 frameLen, U8 *pPayload, U8 lastFlag )
{
	U8 Baud = 0;
	U8 NULL_DstMacAddr[MACADRDR_LENGTH]={0XFF,0XFF,0XFF,0XFF,0XFF,0XFF};
	if(pTestpReadMeterInd!=NULL)
		return;
	pTestpReadMeterInd = &TestpReadMeterInd;
	if(pReadMeterInd!=NULL)
	{
		READ_METER_IND_t * ReadMeterInd = (READ_METER_IND_t *) pReadMeterInd;
		if(0!=memcmp(NULL_DstMacAddr,ReadMeterInd ->DstMacAddr,MACADRDR_LENGTH))	
			__memcpy(pTestpReadMeterInd,ReadMeterInd,sizeof(READ_METER_IND_t));
		else
		{
			app_printf("DstMacAddr is 0xFF\n");
			pTestpReadMeterInd = NULL;
			return;
		}
	}
	app_printf("Test_Read_Meter go\n");
	/*
	if(protocol == DLT698)
	{
		Baud = METER_BAUD_9600;
	}
	else
	{
		Baud = METER_BAUD_2400;
	}
	*/
	if(protocol == DLT698)
	{
		Baud = Baud_698[0];
	}
	else
	{
		Baud = Baud_645[0];
	}
	
	if(InfraredOrCCO == 0)//载波抄表
	{
 		sta_read_meter_put_list(pReadMeterInd,protocol,Baud,timeout,intervaltime,retry,frameSeq,frameLen,pPayload,lastFlag,0);
	}
	else
	{
		InfraredReadMeterPutList(protocol,Baud, timeout, intervaltime,retry,frameSeq, frameLen, pPayload, lastFlag,0);
	}
}

//static void InfraredReadMeterPutList(READ_METER_IND_t *pReadMeterInd, U8 protocol,U8 baud, U16 timeout, 
//                                              U8 intervaltime,U8 retry,U16 frameSeq, U16 frameLen, U8 *pPayload, U8 lastFlag)
static void InfraredReadMeterPutList(U8 protocol,U8 baud, U16 timeout, U8 intervaltime,
										U8 retry,U16 frameSeq, U16 frameLen, U8 *pPayload, U8 lastFlag,U8 IN_MeterList)

{
    add_serialcache_msg_t serialmsg;
	memset((U8 *)&serialmsg,0x00,sizeof(add_serialcache_msg_t));
    
    //app_printf("SendLocalFrame-------------------------\n");
    app_printf("Infrared lastFlag = %d\n", lastFlag);

    //serialmsg.StaReadMeterInfo.ApsSeq       = pReadMeterInd->ApsSeq;
    //serialmsg.StaReadMeterInfo.Dtei         = pReadMeterInd->stei;
    //serialmsg.StaReadMeterInfo.ProtocolType = pReadMeterInd->ProtocolType;
    //serialmsg.StaReadMeterInfo.ReadMode     = pReadMeterInd->ReadMode;
    //serialmsg.StaReadMeterInfo.LastFlag     = lastFlag;
    //__memcpy(serialmsg.StaReadMeterInfo.DestAddr, pReadMeterInd->SrcMacAddr, 6);

    serialmsg.baud = baud_enum_to_int(baud);
	serialmsg.Uartcfg = meter_serial_cfg;
    serialmsg.EntryFail = NULL;
    serialmsg.SendEvent = NULL;
    serialmsg.ReSendEvent = NULL;
    serialmsg.MsgErr = NULL;
	
	serialmsg.Match  = protocol== e_other?NULL : InfraredReadMeterMatch;
	serialmsg.RevDel = InfraredReadMeterRecvDeal;	
	
    if(0==IN_MeterList)
    {
    	serialmsg.Timeout = InfraredReadMeterTimeout;
    }
	else
	{
    	serialmsg.Timeout = InfraredReadMeterTimeout_noin_list;
	}

    serialmsg.Protocoltype = protocol;
	serialmsg.DeviceTimeout = timeout;
    serialmsg.IntervalTime = intervaltime;
	serialmsg.ack = TRUE;
	serialmsg.FrameLen = frameLen;
	serialmsg.FrameSeq = frameSeq;
	serialmsg.lid = e_Serial_Trans;
	serialmsg.ReTransmitCnt = retry;
    serialmsg.pSerialheader = &SERIAL_CACHE_LIST;


	extern ostimer_t *serial_cache_timer;
    serialmsg.CRT_timer = serial_cache_timer;
                                        
	serial_cache_add_list(serialmsg, pPayload,TRUE);

    return;
}


void ReadMeterForInfrared(U8 Protocol , U8 *FreamAddr , U8* Freamdata , U16 Freamlength)
{
    U8   ii               = 0;//,jj;
    U8   *recivebuf = zc_malloc_mem(256, "recivebuf", MEM_TIME_OUT);
    U16  reciveLen      = 0;
	U8   Baud			=METER_BAUD_RES;
    static U8 MeterAddr[6] = {0};
	U8   addr_aa[6] = {0xaa,0xaa,0xaa,0xaa,0xaa,0xaa};

	if(0 != memcmp(MeterAddr , FreamAddr , 6))
	{
		__memcpy(MeterAddr , FreamAddr , 6);
		//ReadCount = 0;
	}
	__memcpy(cjq_read_addr, FreamAddr, 6);
	reciveLen= Freamlength;
	__memcpy(recivebuf , Freamdata , Freamlength);
    for(ii = 0 ; ii < CollectInfo_st.MeterNum ; ii++)
    {
        if( 0 == memcmp(CollectInfo_st.MeterList[ii].MeterAddr , FreamAddr , 6))
        {
            if((1  > CollectInfo_st.MeterList[ii].BaudRate) ||
                (4  < CollectInfo_st.MeterList[ii].BaudRate ))
            {
               // flag = 0 ;
				CJQ_DelMeterInfoByIdx(ii);
                break;
            }
			//Protocol = CollectInfo_st.MeterList[ii].Protocol;
			Baud = CollectInfo_st.MeterList[ii].BaudRate;
            cjq2_search_suspend_timer_modify(20*TIMER_UNITS);
        	InfraredReadMeterPutList(Protocol,Baud,1000,0,0,0,reciveLen,recivebuf,1,TRUE);
        	break;
            
        }  
    }
	
	if(ii == CollectInfo_st.MeterNum && memcmp(FreamAddr,addr_aa,MACADRDR_LENGTH)!=0)
	{
		app_printf("CJQ Err Code!\n");
		cjq2_search_suspend_timer_modify(20*TIMER_UNITS);
		Test_Read_Meter(1,NULL,Protocol,1000,0,0,0,reciveLen,recivebuf,1);
	}
	zc_free_mem(recivebuf);

    return;
}
#endif


DEVICE_PIB_t DevicePib_t =
{
	.ResetTime    =0,			
    .BandMeterCnt = 0,
    .Prtcl	=	DLT645_UN_KNOWN,
    .DevType	=	e_UNKOWN_APP,
    .DevMacAddr[0] = 0,
    .DevMacAddr[1] = 0,
    .DevMacAddr[2] = 0,
    .DevMacAddr[3] = 0,
    .DevMacAddr[4] = 0,
    .DevMacAddr[5] = 0,
    .AutoCalibrateSw = 0,
    .Over_Value = 5,
    .TimeReportDay = 0,
	.TimeDeviation = PIB_TIMEDEVIATION,
    .HNOnLineFlg   = FALSE,		//湖南入网认证
    .MeterPrtcltype = SinglePrtcl_e,
    .Modularmode = 1,
    .Cs = 0xf,
};



#if defined(STATIC_NODE)


//07扩展读中宸采集器附属节点地址指令
U8 BC07_ReadCJQ[12] = {0X68, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0X68, 0X1E, 0X00, 0XEA, 0X16};
//07扩展读中宸采集器附后续属节点地址指令
U8 BC07_ReadCJQ_3E[12] = {0X68, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0XAA, 0X68, 0X3E, 0X00, 0X0A, 0X16};
//extern gpio_dev_t rxled;



#if defined(STATIC_NODE)



ostimer_t *IOChecktimer = NULL;
#endif


#endif
static U8 get_upgrade_data(U8 *raw_data, U8 *upgrade_data)
{
    U8 upgrade_data_type = 0;
    U8 raw_data_bylen = 0;
    U8 upgrade_data_len_type = 0;
    U16 upgrade_data_len = 0;
    U8 upgrade_data_len_cnt = 0;
    U8 upgrade_data_bylen = 3;
    U8 temp[2] = {0x00, 0x00};

    upgrade_data_type = raw_data[raw_data_bylen++];

    if (upgrade_data_type != OCTET_STRING)
    {
        app_printf("upgrade_data_type error:");
        return FALSE;
    }
    else
    {
        upgrade_data_len_type = raw_data[raw_data_bylen++];

        if (upgrade_data_len_type < 0x80)
        {
            upgrade_data_len = upgrade_data_len_type;
        }
        else
        {
            upgrade_data_len_cnt = upgrade_data_len_type - 0x80;

            if (upgrade_data_len_cnt == 1)
            {
                upgrade_data_len = raw_data[raw_data_bylen++];
            }
            else if (upgrade_data_len_cnt == 2)
            {
                upgrade_data_len = raw_data[raw_data_bylen++] << 8;
                upgrade_data_len += raw_data[raw_data_bylen++];
            }
            else
            {
                app_printf("upgrade_data_len too long ! \n");
                return FALSE;
            }
        }

        upgrade_data_len += 4;//兼容645补两个空字节和2个字节长度

        memset(upgrade_data,0xff,upgrade_data_len);

        __memcpy(upgrade_data, raw_data + raw_data_bylen, upgrade_data_bylen);

        //总段数改变大小端，兼容645
        raw_data_bylen += (upgrade_data_bylen + 1);
        upgrade_data[upgrade_data_bylen++] = raw_data[raw_data_bylen--];
        upgrade_data[upgrade_data_bylen++] = raw_data[raw_data_bylen];

        //第i段改变大小端，兼容645
        raw_data_bylen += 3;
        upgrade_data[upgrade_data_bylen++] = raw_data[raw_data_bylen--];
        upgrade_data[upgrade_data_bylen++] = raw_data[raw_data_bylen];
        
        //第i段补两个空字节，兼容645
        __memcpy(upgrade_data + upgrade_data_bylen, temp, sizeof(temp));
        raw_data_bylen += 2;
        upgrade_data_bylen += sizeof(temp);

        //补两个字节实际长度，兼容645
        upgrade_data[upgrade_data_bylen++] = (upgrade_data_len - 11) & 0xff;
        upgrade_data[upgrade_data_bylen++] = (upgrade_data_len - 11) >> 8;

        //copy剩余数据
        __memcpy(upgrade_data + upgrade_data_bylen, raw_data + raw_data_bylen, upgrade_data_len - 11);

        return TRUE;
    }
}
U8 g_printf_state = TRUE ;
static void process_local_upgrade(U8* data, U16 data_len , U8 *UpData, U16 *UpDataLen)
{
    app_printf("process_698_local_upgrade ! \n");

    U16 upgrade_data_len = data_len + 4;
    U8 *upgrade_data = zc_malloc_mem(upgrade_data_len, "upgrade_data", MEM_TIME_OUT);
    U32 blockreturn = 0xFFFFFFFF;
    U8 bylen = 0;

    if (FALSE == get_upgrade_data(data,upgrade_data))
    {
        app_printf("get_upgrade_data fail ! \n");
    }
    else
    {
        blockreturn = checkUpgradedata(upgrade_data, upgrade_data_len);
    }
    
    if (blockreturn == 0xFFFFFFFF)
    {
        UpData[bylen++] = upgrade_fail;
        g_printf_state = TRUE;
    }
    else
    {
        UpData[bylen++] = upgrade_success;
        g_printf_state = FALSE;
    }

    UpData[bylen++] = with_data;
    UpData[bylen++] = OCTET_STRING;
    UpData[bylen++] = sizeof(U16);
    UpData[bylen++] = (blockreturn & 0x00ff00) >> 8;
    UpData[bylen++] = blockreturn & 0x0000ff;

    *UpDataLen = bylen;
    zc_free_mem(upgrade_data);
}

void process_uart_forward(U8* data, U16 data_len , U8 *UpData, U16 *UpDataLen)
{
    app_printf("process_uart_forward ! \n");
    *UpDataLen = 0;
}

Extend698_t extend_698_array[] =
{

    {local_upgrade, process_local_upgrade},
    {uart_forward, process_uart_forward},

};

void process_698_extend(U8 *pMsgData, U16 Msglength)
{
    dl698frame_header_s *down_frame_header = NULL;
    U8 *pApdu = NULL;
    U16 ApduLen = 0;
    U8 dstAddr[MAC_ADDR_LEN];
    U8 byLen = 0;
    U32 extend_code = 0;
    U8 count = 0;
    U8 i = 0;
    // 回码
    U8 apdu_type = 0;
    U8 object_num = 0;
    PIID_s piid = {0};
    OMD_s Omd;
    U8 up_apdu_type = 0;
    U8 up_ctrl = 0;
    U8 up_sever_addr_type = 0x01; // 通配地址
    U8 up_client_addr = 0x00;     // 不关注客户机地址
	
    U8 *up_data = zc_malloc_mem(50, "up_data", 40);
    U16 up_data_len = 0;
    U8 *up_apdu = zc_malloc_mem(50, "up_apdu", 40);
    U16 up_apdu_len = 0;
    U8 *up_frame = zc_malloc_mem(50, "up_frame", 40);
    U16 up_frame_len = 0;

    if (FALSE == ParseCheck698Frame(pMsgData, Msglength, &down_frame_header, &pApdu, dstAddr, &ApduLen))
    {
        app_printf("error 698 !\n");
    }
    else
    {
        apdu_type = pApdu[byLen++];
        object_num = pApdu[byLen++];
        __memcpy(&piid, &pApdu[byLen++], sizeof(PIID_s));
        __memcpy(&Omd, &pApdu[byLen], sizeof(OMD_s));
        byLen += sizeof(OMD_s);

        extend_code = Omd.OperateMode;
        count = sizeof(extend_698_array) / (sizeof(Extend698_t));
        for (i = 0; i < count; i++)
        {
            if (extend_code == (extend_698_array[i].extend_code))
            {
                extend_698_array[i].Func(pApdu + byLen, ApduLen - byLen, up_data, &up_data_len);
            }
        }
		
        if (up_data == 0)
        {
            app_printf("up_apdu_len = 0 , uart not reply ! \n");
        }
        else
        {
            byLen = 0;
            up_apdu_type = apdu_type | 0x80; // 符号位取反
            up_apdu[byLen++] = up_apdu_type;
            up_apdu[byLen++] = object_num;
            __memcpy(&(up_apdu[byLen++]), &piid, sizeof(PIID_s));
            __memcpy(&(up_apdu[byLen]), &Omd, sizeof(OMD_s));
            byLen += sizeof(OMD_s);
            __memcpy(&(up_apdu[byLen]), up_data, up_data_len);
            byLen += up_data_len;
            up_apdu[byLen++] = 0; // 无跟随上报
            up_apdu[byLen++] = 0; // 无时间标签
            up_apdu_len = byLen;
			
            down_frame_header->CtrlField.DirBit = ~(down_frame_header->CtrlField.DirBit);//方向位取反
            __memcpy(&up_ctrl, &(down_frame_header->CtrlField), sizeof(ctrl_field_s));
			
            Packet698Frame(up_frame, &up_frame_len, up_ctrl, up_sever_addr_type, MAC_ADDR_LEN, dstAddr, up_apdu, up_apdu_len, up_client_addr);
            Extend645PutList(DLT698,BAUDMETER_TIMEOUT,0,up_frame_len,up_frame);
        }
    }
    zc_free_mem(up_data);
    zc_free_mem(up_apdu);
    zc_free_mem(up_frame);
}

U8 Check_zc_698_extend(U8 *pDatabuf, U16 dataLen)
{
    dl698frame_header_s *pFrameHeader = NULL;
    U8 *pApdu = NULL;
    U16 ApduLen = 0;

    U8 dstAddr[6] = {0};
    U8 byLen = 3;
    OMD_s Omd;
    U8 zc_extend_omd[3] = {0x54, 0x43, 0x5a};

    if (TRUE != ParseCheck698Frame(pDatabuf, dataLen, &pFrameHeader, &pApdu, dstAddr, &ApduLen))
    {
        return FALSE;
    }
    else
    {
        __memcpy(&Omd, &pApdu[byLen], sizeof(OMD_s));
    }
    if (0 == memcmp(&zc_extend_omd, (U8 *)&Omd, sizeof(zc_extend_omd)))
        return TRUE;
    else
        return FALSE;
}
