#ifndef __PRINTF_ZC_H__
#define __PRINTF_ZC_H__
#include "sys.h"
#include "vsh.h"



#define phy_printf(fmt, arg...)   debug_printf(&dty, DEBUG_PHY,  fmt, ##arg)
#define wphy_printf(fmt, arg...)  debug_printf(&dty, DEBUG_WPHY, fmt, ##arg)
#define net_printf(fmt, arg...)   debug_printf(&dty, DEBUG_MDL,  fmt, ##arg)
#define app_printf(fmt, arg...)   debug_printf(&dty, DEBUG_APP,  fmt, ##arg)
#define macs_printf(fmt, arg...)  debug_printf(&dty, DEBUG_MACS, fmt, ##arg)




void dump_zc(uint8_t delay,char *s,uint32_t debug,uint8_t * buf,uint32_t len);
void dump_buf(uint8_t *buf, uint32_t len);
void dump_level_buf(uint32_t level,uint8_t *buf, uint32_t len);

void dump_printfs(uint8_t *buf, uint32_t len);
void dump_tprintf(tty_t *term, uint8_t *buf, uint32_t len);



#endif 
