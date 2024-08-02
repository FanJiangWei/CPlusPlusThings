/*
 * Copyright: (c) 2006-2007, 2008 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	flash.h
 * Purpose:	
 * History:
 *	Jan 5, 2016	jetmotor	rewrite
 */

#ifndef _FLASH_H_
#define _FLASH_H_

#include "types.h"
#include "gpio.h"
#include "if.h"
#include "app_common.h"
#include "spi.h"
#include "trios.h"
#include "SchTask.h"


/* Flash Map
 *
 *           offset
 *        0x00_0000 +-----------------------+
 *                  |                       |
 *                  ~  b o o t l o a d e r  ~ 192KB
 *                  |                       |
 *        0x03_0000 |- - - - - - - - - - - -|
 *                  |    b o a r d  c f g   |  64KB
 *        0x04_0000 |- - - - - - - - - - - -|
 *                  |     s y s   c f g     |  16KB
 *        0x04_4000 |- - - - - - - - - - - -|
 *                  |   w h i t e  l i s t  |  16KB
 *        0x04_8000 |- - - - - - - - - - - -|
 *                  |    l w i p   c f g    | 512 B
 *        0x04_8200 |- - - - - - - - - - - -|
 *                  |    r e s e r v e d    |31.5KB
 *        0x05_0000 |- - - - - - - - - - - -|
 *                  |b o a r d  c f g backup|  48KB
 *        0x05_c000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  |  s y s t e m  l o g   |  16KB
 *                  |                       |
 *        0x06_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  |                       |
 *                  ~     i m a g e 0       ~ 704KB
 *                  |                       |
 *        0x11_0000 |- - - - - - - - - - - -|
 *                  |    r e s e r v e d    | 128KB
 *        0x13_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  |                       |
 *                  ~     i m a g e 1       ~ 640KB
 *                  |                       |
 *                  |                       |
 *        0x1D_0000 |- - - - - - - - - - - -|
 *                  |   m e t e r  d a t a  | 192KB
 *        0x20_0000 +-----------------------+
 *
 */

/* Flash Map (IDLING_TEXT_IN_ROM)
 *
 *           offset
 *        0x00_0000 +-----------------------+
 *                  |                       |
 *                  ~  b o o t l o a d e r  ~ 192KB
 *                  |                       |
 *        0x03_0000 |- - - - - - - - - - - -|
 *                  |    b o a r d  c f g   |  64KB
 *        0x04_0000 |- - - - - - - - - - - -|
 *                  |     s y s   c f g     |  16KB
 *        0x04_4000 |- - - - - - - - - - - -|
 *                  |   w h i t e  l i s t  |  16KB
 *        0x04_8000 |- - - - - - - - - - - -|
 *                  |    l w i p   c f g    | 512 B
 *        0x04_8200 |- - - - - - - - - - - -|
 *                  |    r e s e r v e d    |31.5KB
 *        0x05_0000 |- - - - - - - - - - - -|
 *                  |b o a r d  c f g backup|  48KB
 *        0x05_c000 |- - - - - - - - - - - -|
 *                  |  s y s t e m  l o g   |  12KB
 *					|                       |
 *		  0x05_f000 |-----------------------|  
 *					|       ext_info		|	4KB
 *        0x06_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~     i m a g e 0       ~ 384KB
 *                  |                       |
 *        0x0C_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~    r e s e r v e d    ~ 448KB
 *                  |                       |
 *        0x13_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~     i m a g e 1       ~ 384KB
 *                  |                       |
 *        0x19_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~   s r o m . t e x t   ~ 448KB
 *                  |                       |
 *        0x20_0000 +-----------------------+
 *
 */

/* Flash Map (STATIC_NODE)
 *
 *           offset
 *        0x00_0000 +-----------------------+
 *                  |                       |
 *                  ~  b o o t l o a d e r  ~ 192KB
 *                  |                       |
 *        0x03_0000 |- - - - - - - - - - - -|
 *                  |    b o a r d  c f g   |  64KB
 *        0x04_0000 |- - - - - - - - - - - -|
 *                  |     s y s   c f g     |  16KB
 *        0x04_4000 |- - - - - - - - - - - -|
 *                  |   	sta_info   	    |  1KB
 *  	  0x04_4400 |- - - - - - - - - - - -|
 *                  |     access_info  	    |  1KB
 *  	  0x04_4800 |- - - - - - - - - - - -|
 *                  |   sta_upgrade_info    |  1KB
 * 		  0x04_4c00 |- - - - - - - - - - - -|
 *                  |      checkmemroy      |  1KB
 *    	  0x04_5000 |- - - - - - - - - - - -|
 *                  |      eboot_info       |  84B
 *        0x04_5054 |- - - - - - - - - - - -|
 *                  |    r e s e r v e d    |  91.9KB
 *        0x05_c000 |- - - - - - - - - - - -|
 *                  |  s y s t e m  l o g   |  8KB
 * 		  0x05_e000 |- - - - - - - - - - - -|
 *                  |   ecc_sm2_key_addr    |  4KB
 *		  0x05_f000 |-----------------------|  
 *					|       ext_info		|  4KB
 *        0x06_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~     i m a g e 0       ~ 384KB
 *                  |                       |
 *        0x0C_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~    r e s e r v e d    ~ 448KB
 *                  |                       |
 *        0x13_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~     i m a g e 1       ~ 384KB
 *                  |                       |
 *        0x19_0000 |- - - - - - - - - - - -|
 *                  |                       |
 *                  ~   s r o m . t e x t   ~ 448KB
 *                  |                       |
 *        0x20_0000 +-----------------------+
 *
 */

#if defined(ARMCM33)
#define FLASH_BASE_ADDR		0x0
#else
#define FLASH_BASE_ADDR		0xfe000000
#endif
#define FLASH_END_ADDR		(FLASH_BASE_ADDR + flash_get_size(FLASH) - 1)

//#define FLASH_SIZE		(flash_get_size(FLASH))

#define FLASH_2M            0x15
#define FLASH_4M   			0x16

#define FLASH_OK		0
#define FLASH_ERROR		-1
#define FLASH_PARA_ERR		-2
#define FLASH_DMA_ERR		-3

#define BOOTLOADER_START	FLASH_BASE_ADDR
#define BOOTLOADER_LEN		(128 * 1024)

#define BOOTINFO_ADDR		(FLASH_BASE_ADDR + 0x20000)
#define BOOTINFO_LEN		(60 * 1024)
#define EXCINFO_ADDR		(BOOTINFO_ADDR + BOOTINFO_LEN)
#define EXCINFO_LEN		0x400
#define EXC_NUM_MAX		4

#define FLASH_PAGE_SHIFT        8UL
#define FLASH_PAGE_SIZE         256UL			/* 256 in every page */
#define FLASH_PAGE_MASK         (FLASH_PAGE_SIZE - 1)	/* 255 */
#define FLASH_SECTOR_SHIFT      12UL
#define FLASH_SECTOR_SIZE       (4 * 1024UL)		/* 4K in every sector */
#define FLASH_SECTOR_MASK       (FLASH_SECTOR_SIZE - 1)	/* 0xfff */
#define FLASH_BLOCK_SIZE        (64 * 1024UL)		/* 64K in every block*/
#define FLASH_BLOCK_MASK        (FLASH_BLOCK_SIZE - 1)	/* 0xffff */
#define FLASH_MAX_SECTOR_NUM    31			/* M25P16 has 32 sectors: 0 ~ 31 */

/* Flash opcodes. */
#define	OPCODE_WREN		0x06	/* Write enable */
#define	OPCODE_WRDI	 	0x04	/* Write disable */
#define	OPCODE_RDSR		0x05	/* Read status register */
#define	OPCODE_WRSR		0x01	/* Write status register */
#define OPCODE_WRENV		0x50	/* Write enable for volatile status register */
#define	OPCODE_READ		0x03	/* Read data bytes */
#define OPCODE_DREAD		0x3b	/* Dual Output Fast Read */
#define OPCODE_QREAD		0x6b	/* Qual Output Fast Read */
#define	OPCODE_PP		0x02	/* Page program */
#define OPCODE_QPP		0x32	/* Qual Page program */
#define	OPCODE_SE		0x20	/* Sector erase */
#define	OPCODE_BE		0xd8	/* Block  erase */
#define	OPCODE_BR		0xc7	/* Buck erase */
#define	OPCODE_RES		0xab	/* Read Electronic Signature */
#define OPCODE_RDID		0x9f	/* Read JEDEC ID */
#define OPCODE_RDSR1		0x35	/* Read status register 1 */
#define OPCODE_ESR		0x44	/* Erase security register */
#define OPCODE_PSR		0x42	/* Program security register */
#define OPCODE_RSR		0x48	/* Read security register */
#define OPCODE_UID		0x4B	/* Read Unique ID */

#define FLASH_LOCK_MUTEX	0
#define FLASH_LOCK_ISR		1

#define FLASH_UID_LEN		16

struct flash_dev_s;
typedef struct flash_dev_s {
	char *name;

	spi_dev_t spi;

	uint8_t mode;		/* spi mode(0: normal, 1: dual, 2: qual) */
	uint8_t lock_level;	/* 0: mutex, 1: isr, otherwise: no lock */
	event_t mutex;

	int32_t (*setcmd)(struct flash_dev_s *dev);
	int32_t (*master_read)(struct flash_dev_s *dev, uint32_t addr, uint32_t *buf, int32_t len);
	int32_t (*master_write)(struct flash_dev_s *dev, uint32_t addr, uint32_t src, int32_t len);

	uint8_t opcode_wren;        /* Write enable */
	uint8_t opcode_wrdi;	     /* Write disable */
	uint8_t opcode_rdsr;	     /* Read status register */
	uint8_t opcode_wrsr;	     /* Write status register */
	uint8_t opcode_wrenv;	     /* Write enable for volatile status register */
	uint8_t opcode_read;	     /* Read data bytes */
	uint8_t opcode_dread;	     /* Dual Output Fast Read */
	uint8_t opcode_qread;	     /* Qual Output Fast Read */
	uint8_t opcode_pp;	     /* Page program */
	uint8_t opcode_qpp;	     /* Qual Page program */
	uint8_t opcode_se;	     /* Sector erase */
	uint8_t opcode_be;	     /* Block  erase */
	uint8_t opcode_br;	     /* Buck erase */
	uint8_t opcode_res;	     /* Read Electronic Signature */
	uint8_t opcode_rdid;	     /* Read JEDEC ID */
	uint8_t opcode_rdsr1;	     /* Read status register 1 */
	uint8_t opcode_esr;	     /* Erase security register */
	uint8_t opcode_psr;	     /* Program security register */
	uint8_t opcode_rsr;	     /* Read security register */
    uint8_t opcode_uid;		/* Read Unique ID */

	uint32_t page_size;
	uint32_t sect_size;
	uint32_t block_size;
	uint32_t flash_size;
	uint32_t flash_base;

	uint32_t sect_buf[FLASH_SECTOR_SIZE >> 2];
	uint32_t page_buf[FLASH_PAGE_SIZE >> 2];

    uint8_t  uid[FLASH_UID_LEN];
} flash_dev_t;

#if 0
typedef struct tftp_debug_s {
	uint32_t flag;
	uint32_t server_ip;
	uint32_t filename[8]; 
} tftp_debug_t;

#define IMAGE0_ACT	1
#define IMAGE1_ACT	2

#define IMAGE_SINGLE_MODE 1
#define IMAGE_DUAL_MODE 2

typedef struct {
	uint32_t mode;
	uint32_t active;
	uint32_t bootDly;
	uint32_t baud;
	uint32_t interface;
	uint8_t  mac[6];
	uint8_t  rsv[2];
	tftp_debug_t tftp;
	
	int32_t  diff0;
	int32_t  diff1;
	int8_t   snr;
	int8_t   clibr_mode;
	uint8_t  rsv1[14];

	uint32_t facility;
	
	uint16_t rf_config0;
	uint16_t rf_config1;
	uint16_t rf_config2;
	uint16_t rf_config3;
} board_cfg_t;

// write only one on produce
typedef struct {
	uint8_t  chip_id[24];
	uint8_t  mdl_id[64];
	uint32_t mdl_id_len;

	uint8_t  mac[6];
	uint8_t  rsv[2];

	uint8_t  firm[2];	// eg. TR, firm[0] = 'T', firm[1] = 'R'
	uint8_t  chip[2];	// eg, TR, chip[0] = 'T', chip[1] = 'R'
	
	uint32_t test_ok;	// 1: pass, other: fail

	uint32_t random;
} orig_cfg_t;

typedef struct {
	uint16_t networking;
	uint16_t reboot;
#define SHAKEDOWN_ID	0x72599527
	uint32_t shakedown_id;
	uint32_t shakedown_time;
	uint16_t band_idx;
	uint16_t rbt_reason;	// reboot reason
#define POWER_CUT_MAGIC	0xA5A5E0AC
	uint32_t powercut_magic;
	uint16_t clt_period;
	uint16_t clt_wp;
	uint16_t hw_rst;	// hardware reset count
	uint16_t sw_rst;	// software reset count
} sys_cfg_t;

#define SYSLOG_MAGIC		0x3c9a
#define SYSLOG_TYPE_EXCEPTION	1
#define SYSLOG_TYPE_SOFTDOG	2
#define SYSLOG_TYPE_OTHER	3

#define NR_SYSLOG_ENT_MAX	15
typedef struct syslog_ent_hdr_s {
	uint32_t valid  :1;
	uint32_t type	:3;
	uint32_t size	:12;
	uint32_t count	:16;
} syslog_ent_hdr_t;

typedef struct syslog_hdr_s {
	uint32_t magic	:16;
	uint32_t last	:16;
	syslog_ent_hdr_t ent[15];
} syslog_hdr_t;

#if !defined(VENUS_V2) && !defined(VENUS_V3) && !defined(FPGA_VERSION)
#define BOARD_CFG_ADDR		(FLASH_BASE_ADDR + 0x30000)
#define BOARD_CFG_LEN		(60 * 1024)
#define BOARD_CFG		((board_cfg_t *)BOARD_CFG_ADDR)

#define ORIG_CFG_ADDR		(BOARD_CFG_ADDR + BOARD_CFG_LEN)
#define ORIG_CFG_LEN		(4 * 1024)
#define ORIG_CFG		((orig_cfg_t *)ORIG_CFG_ADDR)

#define SYS_CFG_ADDR		(FLASH_BASE_ADDR + 0x40000)
#define SYS_CFG_LEN		(16 * 1024)
#define SYS_CFG		        ((sys_cfg_t *)SYS_CFG_ADDR)

#define WHITE_LIST_ADDR		(FLASH_BASE_ADDR + 0x44000)
#define WHITE_LIST_LEN		(16 * 1024)

#define LWIP_CFG_ADDR		(FLASH_BASE_ADDR + 0x48000)
#define LWIP_CFG_LEN		(1 * 512)

#define SYSLOG_ADDR		(FLASH_BASE_ADDR + 0x5c000)
#define SYSLOG_SIZE		(0x4000)
#define SYSLOG_ENT_SIZE		(1024)
#define SYSLOG_ENT_BASE_ADDR	(SYSLOG_ADDR + SYSLOG_ENT_SIZE)

#define IMAGE0_ADDR		(FLASH_BASE_ADDR + 0x60000)
#define IMAGE1_ADDR		(FLASH_BASE_ADDR + 0x130000)
#if defined(IDLING_TEXT_IN_ROM)
#define IMAGE_LEN		(384 * 1024)
#define IMAGE0_CFG_ADDR		(FLASH_BASE_ADDR + 0xC0000)
#define IMAGE0_CFG_LEN		(448 * 1024)
#define IMAGE1_CFG_ADDR		(FLASH_BASE_ADDR + 0x190000)
#define IMAGE1_CFG_LEN		(IMAGE0_CFG_LEN)
#else
#define IMAGE_LEN		(704 * 1024)
#endif

#else  /*VENUS_V2*/

#define BOARD_CFG_ADDR		(FLASH_BASE_ADDR + 0x30000)
#define BOARD_CFG_LEN		(4 * 1024)
#define BOARD_CFG		((board_cfg_t *)BOARD_CFG_ADDR)

#define ORIG_CFG_ADDR		(BOARD_CFG_ADDR + BOARD_CFG_LEN)
#define ORIG_CFG_LEN		(4 * 1024)
#define ORIG_CFG		((orig_cfg_t *)ORIG_CFG_ADDR)

#define SYS_CFG_ADDR		(FLASH_BASE_ADDR + 0x32000)
#define SYS_CFG_LEN		(4 * 1024)
#define SYS_CFG		        ((sys_cfg_t *)SYS_CFG_ADDR)

#define WHITE_LIST_ADDR		(FLASH_BASE_ADDR + 0x33000)
#define WHITE_LIST_LEN		(28 * 1024)

#define LWIP_CFG_ADDR		(FLASH_BASE_ADDR + 0x3A000)
#define LWIP_CFG_LEN		(4 * 1024)

#define SYSLOG_ADDR		(FLASH_BASE_ADDR + 0x3C000)
#define SYSLOG_SIZE		(4 * 1014)
#define SYSLOG_ENT_SIZE		(1024)
#define SYSLOG_ENT_BASE_ADDR	(SYSLOG_ADDR + SYSLOG_ENT_SIZE)

#define IMAGE0_ADDR		(FLASH_BASE_ADDR + 0x40000)
#define IMAGE1_ADDR		(FLASH_BASE_ADDR + 0xC0000)
#if defined(IDLING_TEXT_IN_ROM)
#define IMAGE_LEN		(512 * 1024)
#define IMAGE0_CFG_ADDR		(IMAGE0_ADDR + IMAGE_LEN - 4096)
#define IMAGE0_CFG_LEN		(4 * 1024)
#define IMAGE1_CFG_ADDR		(IMAGE1_ADDR + IMAGE_LEN - 4096)
#define IMAGE1_CFG_LEN		(IMAGE0_CFG_LEN)
#else
#define IMAGE_LEN		(512 * 1024)
#endif
#endif


/* Security register define */
#define NR_OTP_REG			4
#define OTP_REG_SIZE		256	/* bytes */

#define OTP_REG_TOTOL_SIZE	(NR_OTP_REG * OTP_REG_SIZE)

#define OTP_REG_ADDR_BASE	0x000000
#define OTP_REG_ADDR_END	(OTP_REG_TOTOL_SIZE - 1)

//#define IMAGE0_ADDR		(FLASH_BASE_ADDR + 0x60000)
//#define IMAGE_LEN		(832 * 1024)
#define IMAGE0_LEN		(448 * 1024)
#define IMAGE0_RESERVE_ADDR		(IMAGE0_ADDR + IMAGE0_LEN)
#define IMAGE0_RESERVE_LEN		(384 * 1024)
#define IMAGE_RESERVE   (400*1024)

#if defined(UNICORN2M) || defined(UNICORN8M)
#define DEFAULTSETTING   	(FLASH_BASE_ADDR + 0x50000)
#elif defined(ROLAND1_1M) || defined(ROLAND9_1M) || defined(STD_DUAL)
#define DEFAULTSETTING   	PROFILE_ADDR
#endif
#define DEFAULTSETTINGLEN	sizeof(DEFAULT_SETTING_INFO_t)

//#if defined(STATIC_MASTER)
//typedef struct
//{
//    uint8_t       formatseq;
//    uint8_t       resettimes;
//    FLASHINFO_t   flashinfo_t;
//    uint8_t       cs;
//}__PACKED4 CCO_SYSINFO_t;

//#define CCO_SYSINFO		 (FLASH_BASE_ADDR + 0x4E000)
//#define CCO_SYSINFOLEN   sizeof(CCO_SYSINFO_t)
//#endif
#else

//#define FLASH_BASE_ADDR		0x0

#define BOARD_CFG_ADDR		(FLASH_BASE_ADDR + 0x30000)
#define BOARD_CFG_LEN		(16 * 1024)


typedef struct tftp_debug_s {
    uint32_t flag;
    uint32_t server_ip;
    uint32_t filename[8];
} tftp_debug_t;

#define IMAGE0_ACT	1
#define IMAGE1_ACT	2

#define IMAGE_SINGLE_MODE 1
#define IMAGE_DUAL_MODE 2

typedef struct {
	uint32_t mode;
	uint32_t active;
	uint32_t bootDly;
	uint32_t baud;
	uint32_t interface;
	uint8_t  mac[6];
	uint8_t  rsv[2];
	tftp_debug_t tftp;
	
	int32_t  diff0;
	int32_t  diff1;
	int8_t   snr;
	int8_t   clibr_mode;
	uint8_t  rsv1[14];

	uint32_t facility;
	
	uint16_t rf_config0;
	uint16_t rf_config1;
	uint16_t rf_config2;
	uint16_t rf_config3;
} board_cfg_t;





#pragma pack(push)
#pragma pack(1)
typedef struct
{
    uint16_t SelfCheckMessage ;

    uint8_t ChipId[8];

    uint8_t TesterName[3];

    uint8_t Date[3];

    uint8_t Count;

}s_SelfCheckMemory;
#pragma pack(pop)

#if  defined(STATIC_NODE)
    extern gpio_dev_t stasta;
#endif
#define BOARD_CFG		((board_cfg_t *)BOARD_CFG_ADDR)

#define SYS_CFG_ADDR		(FLASH_BASE_ADDR + 0x40000)
#define SYS_CFG_LEN		(16 * 1024)


#if defined(STATIC_NODE)
#define STA_INFO		(uint32_t)(SYS_CFG_ADDR+BOARD_CFG_LEN)
#define  STA_CJQ_RESERVE      1024
#define ACCESS_INFO        (uint32_t)(STA_INFO + STA_CJQ_RESERVE)
#define  STA_UPGRADE_INFO   (uint32_t)(ACCESS_INFO+STA_CJQ_RESERVE)
#define CHECKMEMORY		(uint32_t)(STA_UPGRADE_INFO+STA_CJQ_RESERVE)
#define EBOOT_INFO          (FLASH_BASE_ADDR + 0x45000)	//此信息最好单独放一个完整4K块、防止掉电时，模块在掉电前一两秒复位，导致进行flash存储复位信息，存储失败时，会导致整个4K块数据丢失
#define EBOOT_INFO_LEN      (4*1024)//(8*10+4)

// #define WATER_INFO          (uint32_t)(SYS_CFG_ADDR+BOARD_CFG_LEN + 3072 +sizeof(s_SelfCheckMemory)+3)
// #define WATER_INFO_LEN      sizeof(SINGLE_WATER_INFO_ST)*WATER_MAX_NUM
#elif defined(STATIC_MASTER)

#define CCO_WHITE_LIST_BACKUP_ADDR		(FLASH_BASE_ADDR + 0x34000)
#define CCO_WHITE_LIST_BACKUP_LEN		(40 * 1024)

#define CCO_UPGRDCTRL      (FLASH_BASE_ADDR + 0x3E000)
#define CCO_UPGRDCTRLlEN   sizeof(CcoUpgrdCtrlInfo)
#define EBOOT_INFO          (uint32_t)(CCO_UPGRDCTRL + CCO_UPGRDCTRLlEN)
#define EBOOT_INFO_LEN      (8*10+4)

#endif



//#define IMAGE_CFG_ADDR		(FLASH_BASE_ADDR + 0x40000)
//#define IMAGE_CFG_LEN		(16 * 1024)


#define CCO_WHITE_LIST_ADDR		(FLASH_BASE_ADDR + 0x44000)
#define CCO_WHITE_LIST_LEN		(40 * 1024)

#if defined(STATIC_MASTER)
typedef struct
{
    uint8_t       		formatseq;
    uint8_t       		resettimes;
	uint16_t 	  		clkMainCycle;
	
    FLASHINFO_t   		flashinfo_t;
	
	FLASH_FREQ_INFO_t   FreqInfo;
	Rf_CHANNEL_INFO_t   RfChannelInfo;
    uint8_t       		cs;
}__PACKED4 CCO_SYSINFO_t;

#define CCO_SYSINFO		 (FLASH_BASE_ADDR + 0x4E000)
#define CCO_SYSINFOLEN   sizeof(CCO_SYSINFO_t)
#endif



//#define PROFILE_ADDR		(FLASH_BASE_ADDR + 0x54000)
#define SYS_LOG_ADDR		(FLASH_BASE_ADDR + 0x5c000)
#if defined(UNICORN2M) || defined(UNICORN8M)

#define DEFAULTSETTING   	(FLASH_BASE_ADDR + 0x5c000)
#elif defined(ROLAND1_1M) || defined(ROLAND9_1M) || defined(STD_DUAL)
#define DEFAULTSETTING   	SYS_LOG_ADDR
#endif
#define DEFAULTSETTINGLEN	sizeof(DEFAULT_SETTING_INFO_t)

#define ECC_SM2_AREA_LEN		(4*1024)
#define ECC_SM2_KEY_ADDR		(SYS_LOG_ADDR + 0x00002000)

#define EXT_INFO_ADDR	(SYS_LOG_ADDR + 0x00003000)
#define EXT_INFO_LEN	(0x00001000)					//4KB

#define IMAGE0_ADDR		(FLASH_BASE_ADDR + 0x60000)
#define IMAGE_LEN		(832 * 1024)
#define IMAGE0_LEN		(416 * 1024)
#define IMAGE0_RESERVE_ADDR		(IMAGE0_ADDR + IMAGE0_LEN)
#define IMAGE0_RESERVE_LEN		(416 * 1024)
#define IMAGE1_ADDR		(IMAGE0_ADDR + IMAGE_LEN)
#define IMAGE_RESERVE   (400*1024)

//#ifdef CURVE_GATHER
#define CURVE_CFG       (IMAGE1_ADDR + 704*1024)
#if defined(STATIC_MASTER)
#define CURVE_CFG_LEN   sizeof(CCOCurveGatherInfo_t)
#elif defined(STATIC_NODE)
#define CURVE_CFG_LEN   160
#endif


//#define CURVE0_DATA      (CURVE_CFG + CURVE_CFG_LEN)
//#define CURVE_DATA_LEN  (64*1024 - CURVE_CFG_LEN)
//#define CURVE1_DATA      (CURVE0_DATA + CURVE_DATA_LEN )

//#endif

typedef struct clt_task_ent_s {
	uint32_t time;
	uint8_t  cnt;	// data_id cnt
	uint8_t  rsv[27];
	
	struct {
		uint32_t data_id;	// dlt645
		uint8_t  data_len;	// max 27byte
		uint8_t  data[27];
	} data[31];
} clt_task_ent_t;	// 1K

#define CLT_DATA_ADDR		(FLASH_BASE_ADDR + 0x1D0000)
#define CLT_DATA_SIZE		(192 * 1024)
#define CLT_DATA_ENT_CNT	(192)
#define CLT_DATA_ENT_SIZE	(1024)

/* Security register define */
#define NR_OTP_REG			4
#define OTP_REG_SIZE		256	/* bytes */

#define OTP_REG_TOTOL_SIZE	(NR_OTP_REG * OTP_REG_SIZE)

#define OTP_REG_ADDR_BASE	0x000000
#define OTP_REG_ADDR_END	(OTP_REG_TOTOL_SIZE - 1)

#endif
extern flash_dev_t *FLASH;

/* flash_erase - Erase flash starting from `@addr' with `@len' bytes.
 * @dev: instance of `flash_dev_t'
 * @addr: 4-byte-aligned address in flash area
 * @len: amount of bytes to erase
 * @return: FLASH_PARA_ERR if argument invalid, FLASH_OK if succeed, FLASH_ERROR if fail
 */
extern int32_t flash_erase(flash_dev_t *dev, uint32_t addr, int32_t len);
extern int32_t flash_erase_all(flash_dev_t *dev);
extern int32_t flash_reset(flash_dev_t *dev);
/* flash_write - Write data starting from `@src' with `@len' bytes into flash.
 * @dev: instance of `flash_dev_t'
 * @dst: 4-byte-aligned destination address in flash area
 * @src: 4-byte-aligned source address in non-flash area
 * @len: amount of bytes
 * @return: FLASH_PARA_ERR if argument invalid, FLASH_OK if succeed, FLASH_ERROR if fail
 */
extern int32_t flash_write(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);
extern int32_t flash_write_nonblocking(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);

/* flash_write_easy - write flash without erase
 * @dev: instance of `flash_dev_t'
 * @dst: 4-byte-aligned destination address in flash area
 * @src: 4-byte-aligned source address in non-flash area
 * @len: amount of bytes
 * @return: FLASH_PARA_ERR if argument invalid, FLASH_OK if succeed, FLASH_ERROR if fail
 */
extern int32_t flash_write_easy(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);

/* flash_read - Read flash content starting from `@addr' with `@len' bytes into `@buf'.
 * @dev: instance of `flash_dev_t'
 * @addr: 4-byte-aligned address in flash area
 * @buf: buffer
 * @len: buffer length
 * @return: always FLASH_OK, or hang up.
 */
extern int32_t flash_read(flash_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len);

/* flash_burst_read - Read flash content starting from `@addr' with `@len' bytes into `@buf'.
 * @dev: instance of `flash_dev_t'
 * @addr: 4-byte-aligned address in flash area
 * @buf: buffer
 * @len: buffer length, len > 0
 * @return: always FLASH_OK, or hang up.
 */
extern int32_t flash_burst_read(flash_dev_t *dev, uint32_t addr, uint8_t *buf, int32_t len);

/* flash_dma_read - Read flash content starting from `@addr' with `@len' bytes into `@buf'.
 * @dev: instance of `flash_dev_t'
 * @addr: 4-byte-aligned address in flash area
 * @buf: buffer
 * @len: buffer length, len > 0
 * @return: always FLASH_OK, or hang up.
 * @return: FLASH_PARA_ERR if argument invalid, FLASH_OK if succeed,
 *          FLASH_DMA_ERR if get dma fail.
 */
extern int32_t flash_dma_read(flash_dev_t *dev, uint32_t src, uint8_t *buf, int32_t len);

/* flash_dma_write - Write data starting from `@src' with `@len' bytes into flash.
 * @dev: instance of `flash_dev_t'
 * @dst: 4-byte-aligned destination address in flash area
 * @src: 4-byte-aligned source address in non-flash area
 * @len: amount of bytes
 * @return: FLASH_PARA_ERR if argument invalid, FLASH_OK if succeed, FLASH_ERROR if fail
 *          FLASH_DMA_ERR if get dma fail.
 */
extern int32_t flash_dma_write(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);

/* flash_init - create flash_mutex.
 * @return: FLASH_OK if ok
 * @return: FLASH_ERROR if fail
 */
extern int32_t flash_init(flash_dev_t *dev);

/* flash_set_size - 
 * @sz: default as 0x200000
 */
extern void flash_set_size(flash_dev_t *dev, uint32_t sz);

/* flash_get_size -
 * @return: flash size, default as 0x200000
 */
extern uint32_t flash_get_size(flash_dev_t *dev);

/* flash_get_base -
 * @return: flash base address
 */
extern uint32_t flash_get_base(flash_dev_t *dev);

/* flash_get_end -
 * @return: flash end address
 */
extern uint32_t flash_get_end(flash_dev_t *dev);

/* flash_set_mode - set spi's transfer mode
 * @mode: 
 */
extern void flash_set_mode(flash_dev_t *dev, uint32_t mode);

/* flash_is_busy - 
 */
extern int32_t flash_is_busy(flash_dev_t *dev);

extern flash_dev_t *flash_dev;
extern int32_t flash_write_demo(spi_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);
extern int32_t flash_read_demo(spi_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len);
extern int32_t flash_erase_demo(spi_dev_t *dev, uint32_t addr, int32_t len);

int32_t zc_flash_write(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);
int32_t zc_flash_read(flash_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len);
int32_t zc_flash_erase(flash_dev_t *dev, uint32_t addr, int32_t len);

int32_t flash_erase_sector_demo(spi_dev_t *dev, uint32_t addr, int32_t len);

extern int32_t otp_set_lock(flash_dev_t *dev, int32_t flag);
extern int32_t otp_erase(flash_dev_t *dev, uint32_t blocking, uint32_t addr);
extern int32_t otp_write(flash_dev_t *dev, uint32_t blocking, uint32_t addr, uint8_t *buf, uint32_t len);
extern int32_t otp_read(flash_dev_t *dev, uint32_t blocking, uint32_t addr, uint8_t *buf, uint32_t len);
extern int32_t flash_spi_write(flash_dev_t *dev, void *data, uint32_t len, uint32_t flags);
extern int32_t flash_spi_read(flash_dev_t *dev, void *data, int32_t len, uint32_t flags);
extern uint8_t flash_read_status(flash_dev_t *dev, uint8_t cmd);
extern int32_t flash_write_qe(flash_dev_t *dev, uint8_t qe);
extern int32_t flash_write_status(flash_dev_t *dev, uint8_t status, uint8_t status1);
extern int32_t flash_read_uid(flash_dev_t *dev);	/* opcode = 0x4B */
extern uint32_t flash_read_id(flash_dev_t *dev);	/* opcode = 0x9f */	
#if defined(VENUS_V5) || defined(FPGA_VERSION) 
extern int32_t fmc_write_status(flash_dev_t *dev, uint8_t status, uint8_t status1);
extern int32_t fmc_write_qe(flash_dev_t *dev, uint8_t qe);
extern int32_t fmc_write_page(flash_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);
extern int32_t fmc_erase_sector(flash_dev_t *dev, uint32_t addr);
#if defined(ARMCM33)
extern void fmc_cache_disable(void);
extern void fmc_cache_enable(void);
#endif
#endif
#endif	/* end of _FLASH_H_ */

