/*
 * @Author: wangwenji wangwenji@zc-power.com
 * @Date: 2022-12-21 15:42:36
 * @LastEditors: wangwenji wangwenji@zc-power.com
 * @LastEditTime: 2023-01-09 15:33:43
 * @FilePath: \SDK\src\app\test.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:	     test.h
 * Purpose:	 test interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __TEST_H__
#define __TEST_H__
#include "phy_plc_isr.h"
#include "ZCsystem.h"


#define MAX_SNR_NUM  100
#define MAX_CCO_NUM  20



//extern wpbh_t test_pbh;

//extern U16 hi;
//extern U16 th;
//extern U16 lo;
////extern 	U32 testmode;
//extern  U8 RD_ctrl_beacon_flag;
//extern void testtimerCB(struct ostimer_s *ostimer, void *arg);
void Modifytesttimer(U32 min);
//extern void spy_send(uint8_t *buf, uint32_t len ,uint8_t snr,uint8_t crc);

//extern U8 rdaddr[6];
#if defined(STATIC_NODE)

	#if defined(STD_2016)

	#define   TEST_DELAY    30//second
	#elif defined(STD_GD)
	#define   TEST_DELAY    10//second
	#endif
#elif defined(STATIC_MASTER)
	#if defined(STD_2016)
	#define   TEST_DELAY    25//second

	#elif defined(STD_GD)
	#define   TEST_DELAY    10//second

	#endif

#endif
////#define   PHY_TPT  		0X00000001
////#define   PHY_CB   		0x00000002
////#define   MAC_TPT  		0x00000004
//#define   UART_PHY 		0x00000008
//#define   EASY_UART_PHY 0x00000010

////#define   RD_CTRL  0x40000000
////#define   SPY      0x80000000
//#define   SPY      0x00008000
////#define   ctrl_tei 0xffe

////extern U8 databuf[2500];

//extern U16 ZeroCrossMs ;

//extern frame_ctrl_t phycb_header;
////extern U8  scanFlag;



typedef struct
{
    U8 header;
    U8 CRC_state :7;
    U8 LinkType  :1;
    U16 time;
    U8 snr;
    U16 len;
    U8 pMsg[0];
//    U8 cs;
//    U8 tail;
}__PACKED SPY_t;
//float cal_variance(S8 *p,U8 cnt);
//U32 statistics_snr_variance(U32 bandstart , U32 bandend , U32 hi , U32 th , U32 lo , S8 *p);
//extern void RdCtrlAfn13F1ReadMeterReq(U8 *data,U16 len,U8 protocol,U8 *cmnaddr);

//extern INT8S phy_tx_start_arg(uint8_t dt, uint32_t bpc,uint8_t tmi,uint8_t phase,uint16_t stei, uint16_t dtei, uint32_t stime,
//    void *pld, uint16_t pldlen, uint8_t state,uint8_t lid,uint8_t repeat,uint32_t neighbour,uint16_t dur ,uint16_t bso);
void RdCtrlAfn13F1ReadMeterReq(U8 *data,U16 len,U8 protocol,U8 *cmnaddr);

void spy_send(mbuf_t *buf, uint8_t *payload, uint16_t pdlen, uint8_t crc);
void uart1testmode(U32 Baud);
void test_readteststate(tty_t* term);
void test_cleartest();
void test_phytpt(xsh_t *xsh);
void test_phycb(xsh_t *xsh);
void test_wphytpt(xsh_t *xsh);
void test_wphycb(xsh_t *xsh);
void test_mactpt(xsh_t *xsh);
void test_rfplccb(xsh_t *xsh);
void test_rfchannel(xsh_t *xsh);
void test_band(xsh_t *xsh);
void test_rfband(xsh_t *xsh);
void test_rdctrl();
void test_spy(xsh_t *xsh);
void test_sendmode(xsh_t *xsh);
#if defined(STATIC_MASTER)
void test_networktype(xsh_t *xsh);
#endif

#endif /* __TEST_H__ */

