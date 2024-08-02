/*
 * Copyright: (c) 2016-2020, 2016 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        main.c
 * Purpose:     The entry for the whole system.
 * History:
 */

#include "trios.h"
#include "exc-handler.h"
#include "uart.h"
#include "meter.h"
#include "sys.h"
#include "flash.h"
//#include "ge.h"
#include "wdt.h"
#include "mem_zc.h"
#include "gpio.h"
#include "wphy_isr.h"
#include "phy_plc_isr.h"
#include "spi.h"
#include "softdog.h"
#include <string.h>
#ifdef MIRACL
#include "miracl.h"
#endif
#include "plc_io.h"
#include "DatalinkTask.h"
#include "ZCsystem.h"
#include "app.h"
#include "SchTask.h"

U32  SystenRunTime = 0;
void set_mac()
{
	char mac[MACF_LEN];
	ETH_ADDR[0] = 0x00;
	ETH_ADDR[1] = 0x03;
	ETH_ADDR[2] = 0x2f;
	ETH_ADDR[3] = rand() & 0xff;
	ETH_ADDR[4] = rand() & 0xff;
	ETH_ADDR[5] = rand() & 0xff;
	printf_s("set mac to %s\n", macf(mac, ETH_ADDR));

	return;
}

void print_exc(tty_t *term, uint32_t flag_mask)
{
#if defined(D233L)
	int32_t i;
	uint32_t *exc_info = (uint32_t *)EXC_INFO_ADDR;
 
	switch (*exc_info) {
	case EXCEPTION_FLAG:
	case ~EXCEPTION_FLAG:
		break;
	default:
		return;
	}

	switch (*exc_info & flag_mask) {
	case EXCEPTION_FLAG:
	case ~EXCEPTION_FLAG:
		break;
	default:
		return;
	}

	++exc_info;
	term->op->tputs(term, "Regs:");
	for (i = 0; i < 16; i++) {
		if (0 == (i & 3)) {
			term->op->tputs(term, "\n");
		}
		term->op->tprintf(term, "a%-2d = 0x%08x  ", i, exc_info[i]);
	}

	term->op->tputs(term, "\n\nSRegs:");
	term->op->tprintf(term, "\nPS       = 0x%08x    ", exc_info[PS_ADDR >> 2]);
	term->op->tprintf(term, "DEPC     = 0x%08x    ", exc_info[DEPC_ADDR >> 2]);
	term->op->tprintf(term, "EPC_1     = 0x%08x    ", exc_info[EPC_1_ADDR >> 2]);
	term->op->tprintf(term, "\nEXCCAUSE = 0x%08x    ", exc_info[EXCCAUSE_ADDR >> 2]);
	term->op->tprintf(term, "EXCVADDR = 0x%08x    ", exc_info[EXCVADDR_ADDR]);
	term->op->tprintf(term, "EXCSAVE_1 = 0x%08x    \n", exc_info[EXCSAVE_1_ADDR >> 2]);

	term->op->tputs(term, "\n\nTrace:");
	exc_info += STACK_INFO_ADDR >> 2;
	for (i = 0; i < STACK_INFO_LEN; i += 4) {
		if (0 == (i & 15)) {
			term->op->tputs(term, "\n");
		}
		term->op->tprintf(term, "[0x%08x]  ", exc_info[i >> 2] & 0x3fffffff);
	}
	term->op->tputs(term, "\n\n");

	if (EXCEPTION_FLAG == *(uint32_t *)EXC_INFO_ADDR) {
		term->op->tputs(term, "Because of exception reset system again!\n");
		*(uint32_t *)EXC_INFO_ADDR = ~EXCEPTION_FLAG; /* clear the flag of exception */
		dhit_write_back(EXC_INFO_ADDR, 4);
		sys_delayms(240);
		sys_reboot();
	}
#endif
}

//DEFINE_BUF_MEM(demo, MIN_BUF_SZ);
extern void ge_demo_init();
extern void app_sys_init();
extern void phy_demo_init();

#if defined(STD_DUAL)
extern void wphy_debug_init(void);
#endif

extern void irq_count_show(tty_t *term);
__isr__ void do_softdog_timeout(void)
{
	print_s("softdog is triggered!\n");

	irq_count_show(&tty);

	sys_delayms(240);
	sys_reboot();
}
U32 RebootReason = 0;

int32_t main(void)
{
//	wdt_close();
//	printf_s("closed wdt\n");
	printf_s("System reset by %x\n", get_sys_reset_status());
    RebootReason = get_sys_reset_status();
    
    clr_sys_reset_status();

	board_cfg();
    check_reboot_info(RebootReason);


    wdt_open(10000,WDT_GLB_RST|WDT_GPIO_RST|WDT_POR_RST);

    debug_level = 0;
    open_debug_level(DEBUG_MDL);
    open_debug_level(DEBUG_PHY);
    open_debug_level(DEBUG_WPHY);
    open_debug_level(DEBUG_APP);

    //extern void phy_demo_init();
    //phy_demo_init();

    SchTaskInit();

    mac_pma_init();
    app_init();


    DatalinkTaskInit();
#ifdef MIRACL
	mr_init();
#endif
	softdog_init();
#if defined(D233L)
	softdog_start(16, do_softdog_timeout);
#endif

    //延迟开启物理层调度。
    extern void phy_plc_init();
    phy_plc_init();
#if defined(STD_DUAL)
    wphy_isr_init();
#endif

	return OK;
}
