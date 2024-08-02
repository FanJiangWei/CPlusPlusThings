#ifndef __APP_CMD_INFO_H__
#define __APP_CMD_INFO_H__

#include "ZCsystem.h"
#include "term.h"


#if defined (STATIC_MASTER)

void clear_flash(tty_t *term);
void showupgrade(tty_t *term);
void showRegist(tty_t *term);
void showpower(tty_t *term);
void showf1f1result(tty_t *term);
void show_whitelist(tty_t *term);
void cleanwhitelist(void);
void term_buf(xsh_t *xsh,uint8_t *buf, uint32_t len);
void ShowID(xsh_t *xsh);
void localprotocol(xsh_t *xsh);
void singleupgrade(xsh_t *xsh);
void infoquery(xsh_t *xsh);
void reupgrade(xsh_t *xsh);
void stopupgrade();
void runupgrade();
void clnF1F1flag(tty_t *term);
void indentifypma(xsh_t *xsh);
void phaseposition(xsh_t *xsh);
void add_meter(xsh_t *xsh);
void judge_transmode_swc(void);

#endif

#if defined(STATIC_NODE)

void clear_flash(tty_t *term);
void sfo_write();
void clearstaflash();
void off_write();
#endif


#if defined(ZC3750CJQ2)

void show_meterinfo(tty_t *term);
void cjq_info(xsh_t *xsh);
#endif

void clear_DEF_flash(tty_t *term);
void clear_ext_info_flash(tty_t *term);
void showstainfo(tty_t *term);
void docmd_app_lowpower_set(xsh_t *xsh);
void docmd_app_show_rebootinfo(xsh_t *xsh);
char *get_flash_hw_version();
void show_version();
void showfunctionSwc(tty_t *term);
void show_parameterCFG(tty_t *term);
void DecodeDebugMeterAddr(U8 *buff, U8 *pAddr);
void set_band(xsh_t *xsh);
void change_baud_rate(xsh_t *xsh);
void set_node_power(xsh_t *xsh);
void functionSwc_info(xsh_t *xsh);
void parameterCFG_info(xsh_t *xsh);
void set_manuf(xsh_t *xsh);
void set_nodeid(xsh_t *xsh);
void set_chipid(xsh_t *xsh);
void write_efuse(xsh_t *xsh);
void read_efuse(xsh_t *xsh);
void def_write(xsh_t *xsh);
void indentification(xsh_t *xsh);
void set_Rfband(xsh_t *xsh);

void showecc(xsh_t *xsh);
void showsm2(xsh_t *xsh);
void setecc_1(xsh_t *xsh);
void setecc_2(xsh_t *xsh);
void setsm2_1(xsh_t *xsh);
void setsm2_2(xsh_t *xsh);




#endif