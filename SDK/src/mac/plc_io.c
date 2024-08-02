#include "plc_io.h"
#include "spi.h"
#include "flash.h"
#include "meter.h"
#include "phy.h"
#include "spi.h"
#include "phy_plc_isr.h"


gpio_dev_t UART = {
    .init    = NULL,
    .name    = "uart0",
    .type    = GPIO_TYPE_UART0,
    .pins    = UTX | URX,
    .dirs    = 0,
};
gpio_dev_t JTAG = {
    .init    = NULL,
    .name    = "jtag",
    .type    = GPIO_TYPE_JTAG,
    .pins    = JTDO | JTDI | JTCK | JTMS| JTRST,
    .dirs    = 0,
};

gpio_dev_t meter_gpio = {
    .init    = meter_uart_init,
    .name    = "meter uart",
    .type    = GPIO_TYPE_UART1,
    #if defined(ROLAND1_1M)&&defined(ZC3750CJQ2)
    .pins    = GPIO_06 | GPIO_05,
    #elif defined(STD_DUAL)&&defined(ZC3750CJQ2)
    .pins    = GPIO_06 | GPIO_08,
    #else
    .pins    = GPIO_04 | GPIO_05,
    #endif
    .dirs    = 0,
    .mux={
        {
        #if defined(ROLAND1_1M)&&defined(ZC3750CJQ2)
            .id  = GPIO_06_MUXID,
		#elif defined(STD_DUAL)&&defined(ZC3750CJQ2)
			.id  = GPIO_08_MUXID,
        #else
            .id  = GPIO_04_MUXID,
        #endif
            .dir = GPIO_OUT,
        },
        {
        #if defined(ROLAND1_1M)&&defined(ZC3750CJQ2)
            .id  = GPIO_05_MUXID,
		#elif defined(STD_DUAL)&&defined(ZC3750CJQ2)
			.id  = GPIO_06_MUXID,
        #else
            .id  = GPIO_05_MUXID,
        #endif
            .dir = GPIO_IN,
        },
    },
};
#if defined(UNICORN8M)
gpio_dev_t MDIO = {
    .init    = NULL,
    .name    = "mdio",
    .type    = GPIO_TYPE_MDIO,
    .pins    = MDIO_MDC | MDIO_MDIO,
    .dirs    = 0,
};
gpio_dev_t RMII  = {
    .init    = NULL,
    .name    = "rmii",
    .type    = GPIO_TYPE_GE_RMII,
    .pins    = RMII_TXD0 | RMII_TXD1 | RMII_TXEN | RMII_CLKREF | RMII_RXD0 | RMII_RXD1 | RMII_RXER,
    .dirs    = 0,
};
#endif



static void ld_init(gpio_dev_t *dev)
{
    gpio_set_dir(dev, LD0_CTRL, GPIO_OUT);
}

//static void zc_init(gpio_dev_t *dev)
//{
//    gpio_set_dir(dev, ZCA_CTRL, GPIO_IN);
//#if defined(ROLAND1_1M) || defined(ROLAND9_1M)
//    gpio_set_smit_en(dev->pins, ENABLE);
//#endif
//}

gpio_dev_t ld = {
    .init    = ld_init,
    .name    = "line driver",
    .type    = GPIO_TYPE_LD0,
    .pins    = LD0_CTRL,
    .dirs    = 0,
    .mux     = {{LD0_MAP, GPIO_OUT}},
};
static void rxled_init(gpio_dev_t *dev)
{
     gpio_set_dir(dev, RXLED_CTRL, GPIO_OUT);
     gpio_pins_off(dev, RXLED_CTRL);
}
gpio_dev_t rxled =
{
     .init	  = rxled_init,
     .name	  = "rxled",
     .type	  = GPIO_TYPE_USR,
     .pins	  = RXLED_CTRL,
     .dirs	  = 0,
     .mux	  = {{RXLED_MAP, GPIO_OUT},
     },
};


#if defined(ZC3750STA) && (defined(ROLAND1_1M) )//|| defined(MIZAR1M) || defined(VENUS2M))       //三相模块使用

static void STAphasectr_init(gpio_dev_t *dev)
{
    gpio_set_dir(dev, VA_EN | VB_EN | VC_EN, GPIO_OUT);
    gpio_pins_on(dev, VA_EN | VB_EN | VC_EN);
}

gpio_dev_t sta_phase_ctr  = {
    .init	 = STAphasectr_init,
    .name	 = "Staphasectr",
    .type	 = GPIO_TYPE_USR,
    .pins	 = VA_EN | VB_EN | VC_EN,
    .dirs	 = 0,
};

#endif

static void zc_init(gpio_dev_t *dev)
{
#if defined(ROLAND1_1M) || defined(ROLAND9_1M)
    gpio_set_smit_en(dev->pins, ENABLE);
//    printf_d("init %s. type : %d ,  pin : %08x\n",dev->name, dev->type, dev->pins);
#endif
    gpio_set_dir(NULL, dev->pins, GPIO_IN);

}


gpio_dev_t zca =
{
    .init	 = zc_init,
    .name	 = "zca",
    .type	 = GPIO_TYPE_ZC0,
    .pins	 = ZCA_CTRL,
    .dirs	 = 0,
    .mux	 = {{ZCA_MAP, GPIO_IN},
    },
};
#if  defined(UNICORN8M)
gpio_dev_t zc =
{
    .init	 = zc_init,
    .name	 = "zc",
    .type	 = GPIO_TYPE_ZC0,
    .pins	 = ZC0_CTRL,
    .dirs	 = 0,
    .mux	 = {{ZC0_MAP, GPIO_IN},
    },
};
#endif
#if defined(ROLAND1_1M) || defined(ROLAND9_1M)  || defined(MIZAR9M) || defined(MIZAR1M) || defined(VENUS2M) || defined(VENUS8M)
gpio_dev_t zcb =
{
    .init	 = zc_init,
    .name	 = "zcb",
    .type	 = GPIO_TYPE_ZC1,
    .pins	 = ZCB_CTRL,
    .dirs	 = 0,
    .mux	 = {{ZCB_MAP, GPIO_IN},
    },
};
gpio_dev_t zcc =
{
    .init	 = zc_init,
    .name	 = "zcc",
    .type	 = GPIO_TYPE_ZC2,
    .pins	 = ZCC_CTRL,
    .dirs	 = 0,
    .mux	 = {{ZCC_MAP, GPIO_IN},
    },
};
#if  defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS8M)
gpio_dev_t zc =
{
    .init	 = zc_init,
    .name	 = "zc",
    .type	 = GPIO_TYPE_ZC3,
    .pins	 = ZC0_CTRL,
    .dirs	 = 0,
    .mux	 = {{ZC0_MAP, GPIO_IN},
    },
};
#endif

#endif

#if defined(STD_DUAL)
__SLOWTEXT void wphy_ld_init(gpio_dev_t *dev)
{
    gpio_set_dir(dev, dev->pins, GPIO_OUT);
}
gpio_dev_t wphy_ld = {
    .init    = wphy_ld_init,
    .name    = "wphy ld switch",
    .type    = GPIO_TYPE_USR,
    .pins    = LD1_CTRL,
    .dirs    = 0,
    .mux     = {{LD1_MAP, GPIO_OUT}},
};
#endif

#if defined(UNICORN8M) || defined(ROLAND9_1M) || defined(MIZAR9M)|| defined(VENUS8M)
static void gpio_phase_init(gpio_dev_t *dev)
{
    gpio_set_dir(dev, dev->pins, GPIO_OUT);
    gpio_pins_on(dev, dev->pins);

    phase_mux.txa = dev->mux[0].id;
    phase_mux.txb = dev->mux[1].id;
    phase_mux.txc = dev->mux[2].id;
    phase_mux.rxa = dev->mux[3].id;
    phase_mux.rxb = dev->mux[4].id;
    phase_mux.rxc = dev->mux[5].id;
}

gpio_dev_t phase_dev = {
    .init	= gpio_phase_init,
    .name   = "phase ctrl",
    .type   = GPIO_TYPE_USR,
    .pins	= TXA | TXB | TXC | RXA | RXB | RXC,
    .dirs   = 0,
    .mux    = {
        {TXA_MUXID, GPIO_OUT},
        {TXB_MUXID, GPIO_OUT},
        {TXC_MUXID, GPIO_OUT},
        {RXA_MUXID, GPIO_OUT},
        {RXB_MUXID, GPIO_OUT},
        {RXC_MUXID, GPIO_OUT},
    },
};
#endif

//#elif defined(ZC3750STA)
#if defined(ZC3750STA)
static void set_init(gpio_dev_t *dev)
{
    gpio_set_dir(dev, SET_CTRL, GPIO_IN);
}

 gpio_dev_t staset =
 {
     .init	  = set_init,
     .name	  = "staset",
     .type	  = GPIO_TYPE_USR,
     .pins	  = SET_CTRL,
     .dirs	  = 0,
     .mux	  = {{SET_MAP, GPIO_IN},
     },
 };

 static void sta_init(gpio_dev_t *dev)
 {
     gpio_set_dir(dev, STA_CTRL, GPIO_OUT);
     gpio_pins_off(dev, STA_CTRL);
 }

 gpio_dev_t stasta =
 {
     .init	  = sta_init,
     .name	  = "stasta",
     .type	  = GPIO_TYPE_USR,
     .pins	  = STA_CTRL,
     .dirs	  = 0,
     .mux	  = {{STA_MAP, GPIO_OUT},
     },
 };
 #if defined(STA_BOARD_3_0_02)
 static void txled_init(gpio_dev_t *dev)
 {
      gpio_set_dir(dev, TXLED_CTRL, GPIO_OUT);
      gpio_pins_off(dev, TXLED_CTRL);
 }

 gpio_dev_t txled =
 {
     .init	  = txled_init,
     .name	  = "txled",
     .type	  = GPIO_TYPE_USR,
     .pins	  = TXLED_CTRL,
     .dirs	  = 0,
     .mux	  = {{TXLED_MAP, GPIO_OUT},
     },
 };

#endif
 static void poweroff_init(gpio_dev_t *dev)
 {
     gpio_set_dir(dev, POWEROFF_CTRL, GPIO_IN);
     gpio_pins_on(dev, POWEROFF_CTRL);
 }

 gpio_dev_t poweroff =
 {
     .init	  = poweroff_init,
     .name	  = "poweroff",
     .type	  = GPIO_TYPE_USR,
     .pins	  = POWEROFF_CTRL,
     .dirs	  = 0,
     .mux	  = {{POWEROFF_MAP, GPIO_IN},
     },
 };

 static void reset_init(gpio_dev_t *dev)
 {
     gpio_set_dir(dev, PLCRST_CTRL, GPIO_IN);
     gpio_pins_on(dev, PLCRST_CTRL);
 }


 gpio_dev_t zc_reset =
 {
     .init	  = reset_init,
     .name	  = "reset",
     .type	  = GPIO_TYPE_USR,
     .pins	  = PLCRST_CTRL,
     .dirs	  = 0,
     .mux	  = {{PLCRST_MAP, GPIO_IN},
     },
 };

 static void plug_init(gpio_dev_t *dev)
 {
    gpio_set_dir(dev, PLUG_CTRL, GPIO_IN);
    gpio_pins_on(dev, PLUG_CTRL);
 }


 gpio_dev_t plug =
 {
     .init	 = plug_init,
     .name	 = "plug",
     .type	 = GPIO_TYPE_USR,
     .pins	 = PLUG_CTRL,
     .dirs	 = 0,
     .mux	 = {{PLUG_MAP, GPIO_IN},
     },
 };


 #elif defined(ZC3750CJQ2)
 static void en485_init(gpio_dev_t *dev)
 {
  gpio_set_dir(dev, EN485_CTRL, GPIO_OUT);
  gpio_pins_off(dev, EN485_CTRL);
 }


  gpio_dev_t en485 =
  {
      .init	  = en485_init,
      .name	  = "en485",
      .type	  = GPIO_TYPE_USR,
      .pins	  = EN485_CTRL,
      .dirs	  = 0,
      .mux	  = {{EN485_MAP, GPIO_OUT},
      },
  };

 static void run_init(gpio_dev_t *dev)
 {
   gpio_set_dir(dev, RUN_CTRL, GPIO_OUT);
   gpio_pins_off(dev, RUN_CTRL);
 }


 gpio_dev_t run =
 {
      .init	  = run_init,
      .name	  = "en485",
      .type	  = GPIO_TYPE_USR,
      .pins	  = RUN_CTRL,
      .dirs	  = 0,
      .mux	  = {{RUN_MAP, GPIO_OUT},
      },
 };

 static void poweroff_init(gpio_dev_t *dev)
 {
     gpio_set_dir(dev, POWEROFF_CTRL, GPIO_IN);
     gpio_pins_on(dev, POWEROFF_CTRL);
 }

 gpio_dev_t poweroff =
 {
     .init	  = poweroff_init,
     .name	  = "poweroff",
     .type	  = GPIO_TYPE_USR,
     .pins	  = POWEROFF_CTRL,
     .dirs	  = 0,
     .mux	  = {{POWEROFF_MAP, GPIO_IN},
     },
 };



#endif

#if 0
spi_dev_t spi_devices[] = {
    {
        {
            .init    = NULL,
            .name    = "boot flash",
            .type    = GPIO_TYPE_SPI,
            .pins    = SPI_MOSI | SPI_CS | SPI_MISO | SPI_SCK,
            .dirs    = 0,
            .mux     = {
                /* sequence: css, miso, mosi, sck; */
                /* don't care mux.dir */
                {SPI_CS_MUXID, 0},
                {SPI_MISO_MUXID,0},
                {SPI_MOSI_MUXID,0},
                {SPI_SCK_MUXID, 0},
            },
        },
        .css   = BIT_CSS0,
        .master_read  = flash_read,
        .master_write = flash_write
    },
    {
        {
            .init    = NULL,
            .name    = "external flash",
            .type    = GPIO_TYPE_SS1,
            .pins    = SPI_MOSI | GPIO_02 | SPI_MISO | SPI_SCK,
            .dirs    = 0,
            .mux     = {
                /* sequence: css, miso, mosi, sck; */
                /* don't care mux.dir */
                {GPIO_02_MUXID, 0},
                {SPI_MISO_MUXID,    0},
                {SPI_MOSI_MUXID,    0},
                {SPI_SCK_MUXID, 0},
            },
        },
        .css   = BIT_CSS1,
        .master_read  = flash_read,
        .master_write = flash_write
    }
};
#if defined(ROLAND9_1M) || defined(MIZAR9M)
spi_dev_t *FLASH  = &spi_devices[1];
#else
spi_dev_t *FLASH  = &spi_devices[0];
#endif
#endif

flash_dev_t flash_device = {
    .name = "flash device",

#if defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS8M)
    .spi = {
        .gpio = {
            .init    = NULL,
            .name    = "external flash",
             #if defined(VENUS_V3) && defined(VENUS8M)
            .type    = GPIO_TYPE_SPI,
            .pins    = SPI_MOSI | GPIO_02 | SPI_MISO | SPI_SCK,
            #else
            .type    = GPIO_TYPE_SS1,
            .pins    = SPI_MOSI | GPIO_02 | SPI_MISO | SPI_SCK,
            #endif
            .dirs    = 0,
            #if defined(VENUS_V3)
            .mux     = {
                /* sequence: sck, css, mosi, miso, io2, io3; */
                {GPIO_02_MUXID, 0},
                {SPI_MISO_MUXID,    0},
                {SPI_MOSI_MUXID,    0},
                {SPI_SCK_MUXID, 0},
            /*
                {SPI_SCK_MUXID, 0},
                {SPI_CS_MUXID, 0},
                {SPI_MOSI_MUXID,    0},
                {SPI_MISO_MUXID,    0},
                {GPIO_MAX_MUXID,    0},
                {GPIO_MAX_MUXID,    0},
                */
            }
                #else
            .mux     = {
                /* sequence: css, miso, mosi, sck; */
                /* don't care mux.dir */
                
                {GPIO_02_MUXID, 0},
                {SPI_MISO_MUXID,    0},
                {SPI_MOSI_MUXID,    0},
                {SPI_SCK_MUXID, 0},
                
            }
            #endif
        },
        
        #if defined(VENUS_V3)
        .idx   = 0,
        .css   = BIT_CSS0,
        #else
        .idx   = 0,
        .css   = BIT_CSS1,
        #endif
    },
#else
    .spi = {
        .gpio = {
            .init    = NULL,
            .name    = "boot flash",
            .type    = GPIO_TYPE_SPI,
            .pins    = SPI_MOSI | SPI_CS | SPI_MISO | SPI_SCK,
            .dirs    = 0,
            .mux     = {
                /* sequence: css, miso, mosi, sck; */
                {SPI_CS_MUXID,   0},
                {SPI_MISO_MUXID, 0},
                {SPI_MOSI_MUXID, 0},
                {SPI_SCK_MUXID,  0},
            }
        },
        .idx   = 0,
        .css   = BIT_CSS0,
    },
#endif

#if defined(IDLING_TEXT_IN_ROM)
    .lock_level = FLASH_LOCK_ISR,	/* spi for XIP(Execute in Place) only */
#else
    .lock_level = FLASH_LOCK_MUTEX,
#endif
    .opcode_wren    = OPCODE_WREN,
    .opcode_wrdi	= OPCODE_WRDI,
    .opcode_rdsr	= OPCODE_RDSR,
    .opcode_wrsr	= OPCODE_WRSR,
    .opcode_wrenv	= OPCODE_WRENV,
    .opcode_read	= OPCODE_READ,
    .opcode_dread	= OPCODE_DREAD,
    .opcode_qread	= OPCODE_QREAD,
    .opcode_pp	= OPCODE_PP,
    .opcode_qpp	= OPCODE_QPP,
    .opcode_se	= OPCODE_SE,
    .opcode_be	= OPCODE_BE,
    .opcode_br	= OPCODE_BR,
    .opcode_res	= OPCODE_RES,
    .opcode_rdid	= OPCODE_RDID,
    .opcode_rdsr1	= OPCODE_RDSR1,
    .opcode_esr	= OPCODE_ESR,
    .opcode_psr	= OPCODE_PSR,
    .opcode_rsr	= OPCODE_RSR,
    .page_size      = FLASH_PAGE_SIZE,
    .sect_size      = FLASH_SECTOR_SIZE,
    .block_size     = FLASH_BLOCK_SIZE,
    .flash_base     = FLASH_BASE_ADDR,
    .flash_size     = 0x200000,

    .master_read  = flash_read,
    .master_write = flash_write,
};
flash_dev_t *FLASH = &flash_device;

void board_cfg()
{
    gpio_attach_dev(&UART);
    gpio_attach_dev(&JTAG);
    gpio_remove_dev(&JTAG);

    assert_s(gpio_attach_dev(&ld) == 0);
    assert_s(gpio_attach_dev(&zca) == 0);
    #if defined(UNICORN8M)
    assert_s(gpio_attach_dev(&zc) == 0);
    #endif
    zc_set_edge(EVT_ZERO_CROSS0, GPIO_RISING_EDGE);

#if defined(ROLAND1_1M) || defined(ROLAND9_1M)|| defined(MIZAR1M)|| defined(MIZAR9M)|| defined(VENUS2M)|| defined(VENUS8M)
    assert_s(gpio_attach_dev(&zcb) == 0);
    assert_s(gpio_attach_dev(&zcc) == 0);

    zc_set_edge(EVT_ZERO_CROSS1, GPIO_RISING_EDGE);
    zc_set_edge(EVT_ZERO_CROSS2, GPIO_RISING_EDGE);
#endif

#if defined(ROLAND9_1M)|| defined(MIZAR9M) || defined(VENUS8M)
    assert_s(gpio_attach_dev(&zc) == 0);
    zc_set_edge(EVT_ZERO_CROSS3, GPIO_RISING_EDGE);
#endif

#if defined(UNICORN8M)
    gpio_attach_dev(&MDIO);
    gpio_attach_dev(&RMII);
    assert_s(gpio_attach_dev(&phase_dev) == 0);
#endif

#if defined(ROLAND9_1M) || defined(MIZAR9M) || defined(UNICORN8M) || defined(VENUS8M)
    assert_s(gpio_attach_dev(&phase_dev) == 0);
//    assert_s(gpio_attach_dev(&zc) == 0);
#endif

#if defined(STD_DUAL)
    assert_s(gpio_attach_dev(&wphy_ld) == 0);
#endif
#if defined(STD_DUAL)

#else
    assert_s(gpio_attach_dev(&rxled) == 0);
#endif
#if !defined(VENUS_V3)
    assert_s(gpio_attach_dev(&FLASH->spi.gpio) == 0);
#else
#if defined(VENUS8M)
    gpio_disable_2nd_boot(1);   /* extern flash */
#else
    gpio_disable_2nd_boot(0);   /* local flash */
#endif
#endif
#if defined(VENUS_V3) && defined(VENUS8M)
    //assert_s(gpio_attach_dev(&FLASH->spi.gpio) == 0);
#endif
    flash_init(FLASH);

    assert_s(gpio_attach_dev(g_meter->gpio) == 0);

#if defined(ZC3750CJQ2)
    assert_s(gpio_attach_dev(&run) == 0);
    assert_s(gpio_attach_dev(&en485) == 0);
    assert_s(gpio_attach_dev(&poweroff) == 0);
    //extern meter_dev_t *g_infrared;
//    ir_enable(g_infrared->uart);
    assert_s(gpio_attach_dev(g_infrared->gpio) == 0);
#elif defined(ZC3750STA)
    assert_s(gpio_attach_dev(&staset) == 0);
    assert_s(gpio_attach_dev(&stasta) == 0);
    #if defined(STA_BOARD_3_0_02)
        assert_s(gpio_attach_dev(&txled) == 0);
    #endif
    assert_s(gpio_attach_dev(&poweroff) == 0);
    assert_s(gpio_attach_dev(&zc_reset) == 0);
    assert_s(gpio_attach_dev(&plug) == 0);
    #if defined(ZC3750STA) && (defined(ROLAND1M))// || defined(MIZAR1M) || defined(VENUS2M))
    assert_s(gpio_attach_dev(&sta_phase_ctr) == 0);
    #endif
#endif

    return;
}


extern U32  getHplcTestMode();

U8 TestCmdFlag = FALSE;

void ld_set_rx_phase_zc(uint8_t phase)
{
#if defined(ZC3951CCO)
    gpio_pins_off(&phase_dev, phase_dev.pins);
#if 1
    //gpio_pins_off(NULL, RXA | RXB | RXC);

    uint32_t pins = 0;
	if(HPLC.sw_phase == 0)
	{
		pins = RXA | RXB | RXC;
	}
	else
	{
		if(getHplcTestMode() != NORM)
    	{
            if(TestCmdFlag == FALSE && getHplcTestMode() == PHYTPT)
            {   
    		    pins |= RXA;
            }
            else if(TestCmdFlag == FALSE && getHplcTestMode() == PHYCB)
            {   
    		    pins |= RXA;
            }
            else
            {
                pins = RXA | RXB | RXC;
            }
    	}
		else
		{
			if(phase & PHY_PHASE_A)
		    {
		        pins |= RXA;
		    }
		    if(phase & PHY_PHASE_B)
		    {
		        pins |= RXB;
		    }
		    if(phase & PHY_PHASE_C)
		    {
		        pins |= RXC;
		    }
		}
	}

    if(pins)
        gpio_pins_on(NULL, pins);
#else
    ld_set_rx_phase(phase);
#endif
#else
    ld_set_rx_phase(PHY_PHASE_ALL);
#endif
}

void ld_set_tx_phase_zc(uint8_t phase)
{
#if defined(ZC3951CCO)
     gpio_pins_off(&phase_dev, phase_dev.pins);
#if 1
    //gpio_pins_off(NULL, TXA | TXB | TXC);
    uint32_t pins = 0;
    if(HPLC.sw_phase == 0)
    {
        pins = TXA | TXB | TXC;
    }
    else
    {
        if(getHplcTestMode() != NORM)
    	{
    	    if(TestCmdFlag == FALSE && getHplcTestMode() == PHYCB)
            {   
    		    pins |= TXA;
            }
            else
            {
                pins = TXA | TXB | TXC;
            }
    	}
        else
        {
            static uint8_t  currPhase=PHY_PHASE_A;
            if(phase == PHY_PHASE_ALL)
            {
                phase = currPhase = currPhase == PHY_PHASE_A?PHY_PHASE_B:
                                    currPhase == PHY_PHASE_B?PHY_PHASE_C:
                                    currPhase == PHY_PHASE_C?PHY_PHASE_A:PHY_PHASE_A;
            }
            if(phase & PHY_PHASE_A)
            {
                pins |= TXA;
            }
            if(phase & PHY_PHASE_B)
            {
                pins |= TXB;
            }
            if(phase & PHY_PHASE_C)
            {
                pins |= TXC;
            }
        }
    }
    if(pins)
        gpio_pins_on(NULL, pins);
#else
    ld_set_tx_phase(phase);
#endif
#else
    ld_set_tx_phase(PHY_PHASE_ALL);
#endif
}


void ld_set_3p_meter_phase_zc(uint8_t phase)
{
#if defined(ZC3750STA) && (defined(ROLAND1M))// || defined(MIZAR1M) || defined(VENUS2M))
     //gpio_pins_off(&sta_phase_ctr, sta_phase_ctr.pins);

    gpio_pins_off(&sta_phase_ctr, VA_EN | VB_EN | VC_EN);
    uint32_t pins = 0;

        static uint8_t  currPhase=PHY_PHASE_A;
        if(phase == PHY_PHASE_ALL)
        {
            phase = currPhase = currPhase == PHY_PHASE_A?PHY_PHASE_B:
                                currPhase == PHY_PHASE_B?PHY_PHASE_C:
                                currPhase == PHY_PHASE_C?PHY_PHASE_A:PHY_PHASE_A;
        }
        if(phase & PHY_PHASE_A)
        {
            pins |= VA_EN;
        }
        if(phase & PHY_PHASE_B)
        {
            pins |= VB_EN;
        }
        if(phase & PHY_PHASE_C)
        {
            pins |= VC_EN;
        }

    if(pins)
        gpio_pins_on(&sta_phase_ctr, pins);

#endif
}


ostimer_t *timer_create(uint32_t first, uint32_t interval, uint32_t opt, void (*fn)(struct ostimer_s *, void *), void *arg, char *name)
{
    ostimer_t *pTimer = zc_malloc_mem(sizeof(ostimer_t),"timer", 50);
    timer_init(pTimer, first, interval, opt, fn, arg, name);
    return pTimer;
}

void timer_delete(ostimer_t *tmr)
{
    zc_free_mem(tmr);
}
