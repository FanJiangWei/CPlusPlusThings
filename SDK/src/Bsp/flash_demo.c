/*
 * Copyright: (c) 2016-2020, 2016 Triductor Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        flash_demo.c
 * Purpose:     spi flash demo operator
 * History:
 */
#include "gpio.h"
#include "spi.h"
#include "flash.h"

#ifndef REG32
#define REG32(x) (*(volatile unsigned int *)(x))
#endif

static uint32_t FLASH_SIZE = 0x200000;

#if 0
#define FLASH_PAGE_SHIFT	8UL
#define FLASH_PAGE_SIZE		256UL			/* 256 in every page */
#define FLASH_PAGE_MASK		(FLASH_PAGE_SIZE - 1)	/* 255 */
#define FLASH_SECTOR_SHIFT	16UL
#define FLASH_SECTOR_SIZE	(4 * 1024UL)	//(64 * 1024UL)		/* 64K in every sector */
#define FLASH_SECTOR_MASK	(FLASH_SECTOR_SIZE - 1)	/* 0xffff */
#define FLASH_MAX_SECTOR_NUM	31			/* M25P16 has 32 sectors: 0 ~ 31 */

/* Flash opcodes. */
#define	OPCODE_WREN		0x06	/* Write enable */
#define	OPCODE_WRDI	 	0x04	/* Write disable */
#define	OPCODE_RDSR		0x05	/* Read status register */
#define	OPCODE_WRSR		0x01	/* Write status register */
#define	OPCODE_READ		0x03	/* Read data bytes */
#define	OPCODE_PP		0x02	/* Page program */
#define	OPCODE_SE		0x20    /* Sector erase */
#define	OPCODE_BE		0xD8	/* Block erase */
#define	OPCODE_BR		0xc7	/* Buck erase */
#define	OPCODE_RES		0xab	/* Read Electronic Signature */
#define OPCODE_RDID		0x9f	/* Read JEDEC ID */

#define OPCODE_RDSR1		0x35	/* Read status register 1 */
#define OPCODE_ESR		0x44	/* Erase security register */
#define OPCODE_PSR		0x42	/* Program security register */
#define OPCODE_RSR		0x48	/* Read security register */
#endif

/* status Register */
#define SR_WIP			0x01	/* Write In Progress Bit */

/* status register 1 */
#define SR_LB			0x04	/* Lock bit(non-volatile one time program) */

#define flash_addr2sector(addr)		\
	(((addr) > FLASH_END_ADDR || (addr) < FLASH_BASE_ADDR) ? FLASH_PARA_ERR : ((addr) >> FLASH_SECTOR_SHIFT))
#define flash_sector2addr(sector)	\
	(FLASH_BASE_ADDR + ((sector) << FLASH_SECTOR_SHIFT))

/* flash_erase_sector - Erase flash sector starting from `@addr'.
 * @addr: sector-aligned address in flash area
 * @return:
 */
static int32_t flash_erase_sector(spi_dev_t *dev, uint32_t addr);
/* flash_write_page - Write data into a flash's page, that's, `@dst' + `@len'
 * shall not cross a page boundary.
 * @dst: 4-byte-aligned destination address in flash area
 * @src: 4-byte-aligned source address in non-flash area
 * @len: 4-byte-aligned amount of bytes
 * @return:
 */
static int32_t flash_write_page(spi_dev_t *dev, uint32_t dst, uint32_t src, int32_t len);
/* flash_copy - Copy flash content into `@buf' with `@len' bytes.
 * @dev: instance of `spi_dev_t'
 * @addr: 4-byte-aligned address in flash area
 * @buf: buffer
 * @len: buffer length
 * @return: FLASH_OK if succeed, FLASH_ERROR if fail
 */
static int32_t flash_copy(spi_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len);

struct {
	uint32_t start;
	int32_t len;
} FIRST, LAST;
static uint32_t FIRST_buffer[FLASH_SECTOR_SIZE >> 2];
static uint32_t LAST_buffer[FLASH_SECTOR_SIZE >> 2];

static uint32_t flash_demo_read_status(spi_dev_t *dev)
{
	spi_busy_wait(dev);
	spi_set_css_mode(dev, CSS_AUTO, 1 << dev->css);
	spi_set_css_auto_stop(dev, ENABLE);
	spi_set_data(dev, 3, OPCODE_RDSR);
	spi_busy_wait(dev);

	return spi_get_data_easy(dev);
}

static int32_t flash_demo_is_busy(spi_dev_t *dev)
{
	uint32_t count = 5000000 * CPU_CYCLE_PER_US;
	uint32_t start, current, diff, status;

	start = cpu_get_cycle();
	
	while(1) {
		status = flash_demo_read_status(dev);
		if (!((status >> 24) & SR_WIP))
			return 0;
        
		current = cpu_get_cycle();
		if(current < start)
			diff = ~(start - current);
		else
			diff = current - start;

		if(diff >= count)
			break;
	}

	printf_s("[%s:%d]WAIT ERROR\n", __func__, __LINE__);
	return 1;
}
uint32_t flash_read_capacity(spi_dev_t *dev)
{
	spi_busy_wait(dev);
	spi_set_css_mode(dev,CSS_AUTO, 1 << dev->css);
	spi_set_css_auto_stop(dev,ENABLE);
	spi_set_data(dev,4, OPCODE_RDID);
	spi_busy_wait(dev);

	return spi_get_data_easy(dev);
}

#if 0
static int32_t flash_write_status(spi_dev_t *dev, uint32_t status, uint32_t status1)
{
	uint32_t cpu_sr = 0;
	   
	if (flash_demo_is_busy(dev))
		return FLASH_ERROR;

	cpu_sr = OS_ENTER_CRITICAL();

	spi_set_css_mode(dev, CSS_AUTO, 1 << dev->css);
	spi_set_css_auto_stop(dev, ENABLE);

	/* write enable */
	spi_set_data(dev, 1, OPCODE_WREN);

	/* write status */
	spi_set_css_auto_stop(dev, DISABLE);
	spi_set_data(dev, 1, OPCODE_WRSR);

	spi_set_data(dev, 1, status);
	spi_set_data(dev, 1, status1);

	spi_set_css_auto_stop(dev, ENABLE);

	sys_delayus(10);

	OS_EXIT_CRITICAL(cpu_sr);

	if (flash_demo_is_busy(dev))
		return FLASH_ERROR;

	return FLASH_OK;
}
#endif

static int32_t __flash_write(spi_dev_t *dev, uint32_t dst, uint32_t src, int32_t len)
{
	int32_t i;
	uint32_t tmp, _dst, _src, _len;

	_dst = dst;
	_src = src;
	_len = len;
	i    = 0;

	printf_s(".");

	if ((tmp = _dst & FLASH_PAGE_MASK)) {
		tmp = FLASH_PAGE_SIZE - tmp;
		tmp = (tmp > _len) ? _len : tmp;

		if (flash_write_page(dev, _dst, _src, tmp) == FLASH_ERROR) 
			return FLASH_ERROR;

		_len -= tmp;
		_dst += tmp;
		_src += tmp;
		++i;
	}
	while (_len >= FLASH_PAGE_SIZE) {
		if (flash_write_page(dev, _dst, _src, FLASH_PAGE_SIZE) == FLASH_ERROR) 
			return FLASH_ERROR;

		_len -= FLASH_PAGE_SIZE;
		_dst += FLASH_PAGE_SIZE;
		_src += FLASH_PAGE_SIZE;

		if (!(++i & 255))
			printf_s(".");
	}
	if (_len) {
		if (flash_write_page(dev, _dst, _src, _len) == FLASH_ERROR) 
			return FLASH_ERROR;
	}

	if (flash_demo_is_busy(dev)) 
		return FLASH_ERROR;

	_dst = dst;
	_src = src;
	for (i = 0; i < len; i+=4, _dst+=4, _src+=4) {
		if ((tmp = REG32(_dst)) != REG32(_src))	{
			printf_s("write %08x with %08x error: %08x\n", _dst, REG32(_src), tmp);
			return FLASH_ERROR;
		}
	}

	return FLASH_OK;
}

static int32_t flash_erase_sector(spi_dev_t *dev, uint32_t addr)
{
	uint32_t cpu_sr;

	if (flash_demo_is_busy(dev))
		return FLASH_ERROR;

	cpu_sr = OS_ENTER_CRITICAL();

	spi_set_css_mode(dev, CSS_AUTO, 1 << dev->css);
	spi_set_css_auto_stop(dev, ENABLE);

	/* flash write enable */
	spi_set_data(dev, 1, OPCODE_WREN);

	/* erase sector */
	spi_set_wdata_msb(dev, 1);
	spi_set_data(dev, 4, (OPCODE_SE << 24) | (addr & 0x00ffffff));

	/* recover data msb */
	spi_set_wdata_msb(dev, 0);

	OS_EXIT_CRITICAL(cpu_sr);

	if (flash_demo_is_busy(dev))
		return FLASH_ERROR;

	return FLASH_OK;
}

static int32_t flash_write_page(spi_dev_t *dev, uint32_t dst, uint32_t src, int32_t len)
{
	int32_t i, ret = FLASH_OK;
	uint32_t cpu_sr = 0;

	if (flash_demo_is_busy(dev))
		return FLASH_ERROR;

	cpu_sr = OS_ENTER_CRITICAL();

	spi_set_css_mode(dev, CSS_AUTO, 1 << dev->css);
	spi_set_css_auto_stop(dev, ENABLE);

	/* write enable */
	spi_set_data(dev, 1, OPCODE_WREN);

	/* page program */
	spi_set_css_auto_stop(dev, DISABLE);
	spi_set_data(dev, 1, OPCODE_PP);

	/* set 24-bit address */
	spi_set_wdata_msb(dev, 1);
	spi_set_data(dev, 3, dst & 0xffffff);

	/* fill data */
	spi_set_wdata_msb(dev, 0);

	len -= 4;
	for (i = 0; i < len; ) {
		spi_set_data(dev, 4, REG32(src));
		i   += 4;
		src += 4;
	}

	/* fill last 4-byte */
	spi_busy_wait(dev);
	spi_set_css_auto_stop(dev, ENABLE);
	spi_set_data(dev, 4, REG32(src));

	sys_delayus(10);
	if (flash_demo_is_busy(dev))
		ret = FLASH_ERROR;

	OS_EXIT_CRITICAL(cpu_sr);

	return ret;
}

static int32_t flash_copy(spi_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len)
{
	int32_t i;

	if (flash_demo_is_busy(dev))
		return FLASH_ERROR;

	if (buf) {
		for (i = 0; i < len>>2; ++i, addr+=4) 
			buf[i] = REG32(addr);
	}
	return FLASH_OK;
}

/*static*/ int32_t flash_erase_demo(spi_dev_t *dev, uint32_t addr, int32_t len)
{
	int32_t i;
    
	if (len & 3)
		len = (len + 3) & ~3UL;

	if ((addr & 3) || (!len) || (addr < FLASH_BASE_ADDR) || ((addr + len) > (FLASH_BASE_ADDR + FLASH_SIZE))) {
		printf_s("ARGUEMENT ERROR\n");
		return FLASH_PARA_ERR;
	}
    
	FIRST.start = addr & ~FLASH_SECTOR_MASK;
	if ((FIRST.len = addr & FLASH_SECTOR_MASK)) {
		flash_copy(dev, FIRST.start, FIRST_buffer, FIRST.len);
	}

	LAST.start = addr + len;
	if ((LAST.len = LAST.start & FLASH_SECTOR_MASK)) {
		LAST.len  = FLASH_SECTOR_SIZE - LAST.len;
		flash_copy(dev, LAST.start, LAST_buffer, LAST.len);

		if (flash_erase_sector(dev, LAST.start) != FLASH_OK) {
			printf_s("ERASE ERROR\n");
			return FLASH_ERROR;
		}
	}

	for (i = FIRST.start; i < (LAST.start & ~FLASH_SECTOR_MASK); i += FLASH_SECTOR_SIZE) {
		if (flash_erase_sector(dev, i) != FLASH_OK) {
			printf_s("ERASE ERROR\n");
			return FLASH_ERROR;
		}
	}

	if (FIRST.len) 
		__flash_write(dev, FIRST.start, (uint32_t)FIRST_buffer, FIRST.len);

	if (LAST.len)
		__flash_write(dev, LAST.start, (uint32_t)LAST_buffer, LAST.len);
    
	return FLASH_OK;
}

/*static*/ int32_t flash_write_demo(spi_dev_t *dev, uint32_t dst, uint32_t src, int32_t len)
{
	int32_t i;
    uint32_t _dst;

	if (len & 3) 
		len = (len + 3) & ~3UL;

	if ((dst & 3) || (src & 3) || (!len) ||
			(dst < FLASH_BASE_ADDR) ||
			((dst + len) > (FLASH_BASE_ADDR + FLASH_SIZE))) {
		printf_s("ARGUEMENT ERROR\n");
		return FLASH_PARA_ERR;
	}
	if (! (((src + len) <= FLASH_BASE_ADDR) || (src > FLASH_END_ADDR))) {
		printf_s("source address in flash.\n");
		return FLASH_PARA_ERR;
	}

	spi_busy_wait(dev);
	spi_set_css_mode(dev, CSS_AUTO, 1 << dev->css);
	
	_dst = dst;
	
	for (i = 0; i < len; i+=4, _dst+=4) {
		if (REG32(_dst) != 0xffffffff) {
			if (flash_erase_demo(dev, dst, len) != FLASH_OK)
				return FLASH_ERROR;
			break;
		}
	}

	return __flash_write(dev, dst, src, len);
}

/*static*/ int32_t flash_read_demo(spi_dev_t *dev, uint32_t addr, uint32_t *buf, int32_t len)
{
	int32_t i;

	if (len & 3)
		len = (len + 3) & ~3UL;

	spi_busy_wait(dev);
	spi_set_css_mode(dev, CSS_AUTO, 1 << dev->css);

	for (i = 0; i < len/4; ++i, addr += 4) {
		buf[i] = REG32(addr);
	}

	return FLASH_OK;
}
#define   READ_PIN(pin)  ((pin&gpio_input(NULL,pin))?1:0)

uint32_t get_flash_size(tty_t *term)
{
	uint32_t pJEDEC;
	uint8_t  pCapacity;

	pJEDEC = flash_read_capacity(&FLASH->spi);
    
    term->op->tprintf(term,"pJEDEC = 0x%08x\n",pJEDEC);
    pCapacity = (pJEDEC&0xFF000000)>>24;
    if(pCapacity == FLASH_4M)
    {
		term->op->tprintf(term,"FLASH Size is 4M\n");
    }
    else
    {
		term->op->tprintf(term,"FLASH Size is 2M\n");
    }
    return 0;
}

param_t cmd_flash_demo_param_tab[] = {
	{PARAM_ARG|PARAM_TOGGLE, "", "read|write|set|ctrl|id"
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "source", "source address",
	 "read"
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "dest", "target address",
	 "read"
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "len", "",
	 "read"
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "source", "source address",
	 "write"
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "dest", "target address",
	 "write"
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "len", "",
	 "write"
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "0|1|2|3|4|5|6|7|8|9|10|jtdo|jtdi|jtck|jtms|jtrst",
	 "set"
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "0|1|2",
	 "set"
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "0|1|2|3|4|5|6|7|8|9|10|jtdo|jtdi|jtck|jtms|jtrst",
	 "ctrl"
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "0|1|2",
	 "ctrl"
	},
	PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_flash_demo_desc,
	"flash test", cmd_flash_demo_param_tab
	);
#if defined(STD_DUAL)
	extern gpio_dev_t wphy_ld;
#endif

extern board_cfg_t BoardCfg_t;

void set(tty_t *term, uint32_t  states, uint32_t  pin, char *name)
{
	if(states == 0)
     {
         gpio_pins_off(NULL,pin);
        //gpio_set_pull_down(pin);
        term->op->tprintf(term, "set %s 0\n", name);
    }
    else if(states == 1)
    {
        gpio_pins_on(NULL,pin);
        //gpio_set_pull_up(pin);
        term->op->tprintf(term, "set %s 1\n", name);
    }
    else if(states == 2)
    {
         //
    }
}

void ctrl(tty_t *term, uint32_t  states, uint32_t  pin, char *name)
{
	gpio_set_dir(NULL, pin, GPIO_OUT);
    if(states == 0)
	{
        gpio_set_dir(NULL, pin, GPIO_IN);
        term->op->tprintf(term, "ctrl %s IN\n", name);
    }
    else if(states == 1)
    {
        gpio_set_dir(NULL, pin, GPIO_OUT);
        term->op->tprintf(term, "ctrl %s OUT\n", name);
    }
    else if(states == 2)
    {
                
    }
}

void docmd_flash_demo(command_t *cmd, xsh_t *xsh)
{
        tty_t *term = xsh->term;
        uint32_t sa = 0, da = 0, len = 0;
	uint32_t time_start, time_end;

	if (xsh->argc >= 5) {
		sa = __strtoul(xsh->argv[2], 0, 0);
		if (sa & 3) {
			term->op->tputs(term, "illegal src address.\n");
			return;
		}

		da = __strtoul(xsh->argv[3], 0, 0);
		if (da & 3) {
			term->op->tputs(term, "illegal dest address.\n");
			return;
		}
		len = __strtoul(xsh->argv[4], 0, 0);
		term->op->tprintf(term, "sa: 0x%x da: 0x%x len: 0x%x\n", sa, da, len);
	}

	time_start = cpu_get_cycle();
	if (__strcmp(xsh->argv[1], "read") == 0) {
		flash_read_demo(&FLASH->spi, sa, (uint32_t *)da, len);
	} else if (__strcmp(xsh->argv[1], "write") == 0) {
		flash_write_demo(&FLASH->spi, da, sa, len);
	} else if (__strcmp(xsh->argv[1], "id") == 0) {
		get_flash_size(term);
	}
    #if defined(STD_DUAL)
    uint32_t  pin = 0;
    char *name = "n/a";
    uint32_t  states = 0;
    if (__strcmp(xsh->argv[1], "set") == 0 || __strcmp(xsh->argv[1], "ctrl") == 0) {
		if (__strcmp(xsh->argv[2], "0") == 0) {
            //if(gpio_find_dev(GPIO_09_MUXID) == &wphy_ld)
            {
                //assert_s( gpio_remove_dev(&wphy_ld) == 0);
            }
            //gpio_set_dir(NULL, GPIO_01, GPIO_OUT);
		    //gpio_pins_off(NULL,GPIO_01);
            //term->op->tprintf(term, "GPIO_09 set 0\n");
            pin = GPIO_00;
            name = "GPIO_00";
	    } 
        else if (__strcmp(xsh->argv[2], "1") == 0){
            //if(gpio_find_dev(GPIO_09_MUXID) == &wphy_ld)
            {
                //assert_s( gpio_remove_dev(&wphy_ld) == 0);
            }
            //gpio_set_dir(NULL, GPIO_01, GPIO_OUT);
            //gpio_pins_on(NULL,GPIO_01);
            //term->op->tprintf(term, "GPIO_09 set 1\n");
            pin = GPIO_01;
            name = "GPIO_01";
        }
        else if (__strcmp(xsh->argv[2], "2") == 0){
            pin = GPIO_02;
            name = "GPIO_02";
        }
        else if (__strcmp(xsh->argv[2], "3") == 0){
            pin = GPIO_03;
            name = "GPIO_03";
        }
        else if (__strcmp(xsh->argv[2], "4") == 0){
            pin = GPIO_04;
            name = "GPIO_04";
        }
        else if (__strcmp(xsh->argv[2], "5") == 0){
            pin = GPIO_05;
            name = "GPIO_05";
        }
        else if (__strcmp(xsh->argv[2], "6") == 0){
            pin = GPIO_06;
            name = "GPIO_06";
        }
        else if (__strcmp(xsh->argv[2], "7") == 0){
            pin = GPIO_07;
            name = "GPIO_07";
        }
        else if (__strcmp(xsh->argv[2], "8") == 0){
            pin = GPIO_08;
            name = "GPIO_08";
        }
        else if (__strcmp(xsh->argv[2], "9") == 0){
            pin = GPIO_09;
            name = "GPIO_09";
        }
        else if (__strcmp(xsh->argv[2], "10") == 0){
            pin = GPIO_10;
            name = "GPIO_10";
        }
        else if (__strcmp(xsh->argv[2], "jtdo") == 0){
            pin = JTDO;
            name = "JTDO";
        }
        else if (__strcmp(xsh->argv[2], "jtdi") == 0){
            pin = JTDI;
            name = "JTDI";
        }
        else if (__strcmp(xsh->argv[2], "jtck") == 0){
            pin = JTCK;
            name = "JTCK";
        }
        else if (__strcmp(xsh->argv[2], "jtms") == 0){
            pin = JTMS;
            name = "JTMS";
        }
        else if (__strcmp(xsh->argv[2], "jtrst") == 0){
            pin = JTRST;
            name = "JTRST";
        }
        if (__strcmp(xsh->argv[3], "0") == 0){
            states = 0;
        }
        else if (__strcmp(xsh->argv[3], "1") == 0){
            states = 1;
        }
        else if (__strcmp(xsh->argv[3], "2") == 0){
            states = 2;
        }
        if(__strcmp(xsh->argv[1], "set") == 0)
        {
            set(xsh->term,states,pin,name);//��װset
        }
        else if(__strcmp(xsh->argv[1], "ctrl") == 0)
        {
			ctrl(xsh->term,states,pin,name);//��װctrl
        }
        os_sleep(10);
        term->op->tprintf(term, "read %s : %d\n", name, READ_PIN(pin));
	} 
    #endif
	time_end = cpu_get_cycle();
	term->op->tprintf(term, "total time: %d(us)\n", (time_end - time_start) / CPU_CYCLE_PER_US);

	return;
}
