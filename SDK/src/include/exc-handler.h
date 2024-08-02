#ifndef EXC_HANDLER_H
#define EXC_HANDLER_H

#if defined(D233L)
#include <xtensa/config/core.h>

#if defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(FPGA_VERSION) || defined(ROLAND9_1M) || defined(MIZAR9M)  || defined(VENUS2M) || defined(VENUS8M)
#if defined(IDLING_TEXT_IN_ROM)
#define EXC_REBOOT_ADDR		0x20000200
#define EXC_INFO_ADDR		0x20000800
#else
#define EXC_REBOOT_ADDR		0x20000200
#define EXC_INFO_ADDR		0x20000800
#endif
#else
#define EXC_INFO_ADDR		(XCHAL_WINDOW_VECTORS_VADDR + 0xa00)
#endif
#define EXCEPTION_FLAG		0x16158643	/* exception flag */

/*Loop*/
#define LBEG_ADDR		64
#define LEND_ADDR		(LBEG_ADDR + 4)//68
#define LCOUNT_ADDR		(LBEG_ADDR + 2*4)//72
#define SAR_ADDR		(LBEG_ADDR + 3*4)//76
#define PS_ADDR			(LBEG_ADDR + 4*4)//80

/*High Priority interrupt*/
#define EPS_2_ADDR		(LBEG_ADDR + 5*4)//84
#define EPS_3_ADDR		(LBEG_ADDR + 6*4)//88
#define EPS_4_ADDR		(LBEG_ADDR + 7*4)//
#define EPS_5_ADDR		(LBEG_ADDR + 8*4)//
#define EPS_6_ADDR		(LBEG_ADDR + 9*4)//
#define EPS_7_ADDR		(LBEG_ADDR + 10*4)//
#define EPC_2_ADDR		(LBEG_ADDR + 11*4)//
#define EPC_3_ADDR		(LBEG_ADDR + 12*4)//
#define EPC_4_ADDR		(LBEG_ADDR + 13*4)//
#define EPC_5_ADDR		(LBEG_ADDR + 14*4)//
#define EPC_6_ADDR		(LBEG_ADDR + 15*4)//
#define EPC_7_ADDR		(LBEG_ADDR + 16*4)//
#define EXCSAVE_2_ADDR		(LBEG_ADDR + 17*4)//
#define EXCSAVE_3_ADDR		(LBEG_ADDR + 18*4)//
#define EXCSAVE_4_ADDR		(LBEG_ADDR + 19*4)//
#define EXCSAVE_5_ADDR		(LBEG_ADDR + 20*4)//
#define EXCSAVE_6_ADDR		(LBEG_ADDR + 21*4)//
#define EXCSAVE_7_ADDR		(LBEG_ADDR + 22*4)//
/*Exceptions*/
#define DEPC_ADDR		(LBEG_ADDR + 23*4)//
#define EPC_1_ADDR		(LBEG_ADDR + 24*4)//
#define EXCCAUSE_ADDR		(LBEG_ADDR + 25*4)//
#define EXCVADDR_ADDR		(LBEG_ADDR + 26*4)//
#define EXCSAVE_1_ADDR		(LBEG_ADDR + 27*4)//
/*timer interrupt*/
#define CCOMPARE_0_ADDR		(LBEG_ADDR + 28*4)//
#define CCOMPARE_1_ADDR		(LBEG_ADDR + 29*4)//
#define CCOMPARE_2_ADDR		(LBEG_ADDR + 30*4)//
#define CCOUNT_ADDR		(LBEG_ADDR + 31*4)//
#define WINDOWBASE_ADDR		(LBEG_ADDR + 32*4)//
#define WINDOWSTART_ADDR	(LBEG_ADDR + 33*4)//
/*memory management*/
#define PTEVADDR_ADDR		(LBEG_ADDR + 34*4)//
#define RASID_ADDR		(LBEG_ADDR + 35*4)//
#define ITLBCFG_ADDR		(LBEG_ADDR + 36*4)//
#define DTLBCFG_ADDR		(LBEG_ADDR + 37*4)//
/*debug*/
#define DDR_ADDR		(LBEG_ADDR + 38*4)//
#define IBREAKA_0_ADDR		(LBEG_ADDR + 39*4)//
#define IBREAKA_1_ADDR		(LBEG_ADDR + 40*4)//
#define DBREAKA_0_ADDR		(LBEG_ADDR + 41*4)//
#define DBREAKA_1_ADDR		(LBEG_ADDR + 42*4)//
#define DBREAKC_0_ADDR		(LBEG_ADDR + 43*4)//
#define DBREAKC_1_ADDR		(LBEG_ADDR + 44*4)//
#define DEBUGCAUSE_ADDR		(LBEG_ADDR + 45*4)//
#define ICOUNT_ADDR		(LBEG_ADDR + 46*4)//
#define ICOUNTLEVEL_ADDR	(LBEG_ADDR + 47*4)//

/*interrupt*/
#define INTERRUPT_ADDR		(LBEG_ADDR + 48*4)
#define INTREAD_ADDR		(LBEG_ADDR + 49*4)	/* alternate name for backward compatibility */
#define INTENABLE_ADDR		(LBEG_ADDR + 50*4)

/*used for save the stack infomation*/
#define STACK_INFO_ADDR		(LBEG_ADDR + 51*4)
#define STACK_INFO_LEN		0x80

/*other*/
#define BR_ADDR			4
#define LITBASE_ADDR		5
#define SCOMPARE1_ADDR		12
#define ACCLO_ADDR		16
#define ACCHI_ADDR		17
#define MR_0_ADDR		32
#define MR_1_ADDR		33
#define MR_2_ADDR		34
#define MR_3_ADDR		35
#define IBREAKENABLE_ADDR	96
#define CACHEATTR_ADDR		98
#define CPENABLE_ADDR		224
#define PRID_ADDR		235
#define MISC_REG_0_ADDR		244
#define MISC_REG_1_ADDR		245
#define MISC_REG_2_ADDR		246
#define MISC_REG_3_ADDR		247
#endif

#endif
