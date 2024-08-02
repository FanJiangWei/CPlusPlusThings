#ifndef __NET_CMD_INFO_H__
#define __NET_CMD_INFO_H__

#include "ZCsystem.h"
#include "term.h"

#if defined (STATIC_MASTER)
void net_test_deldevtei(xsh_t *xsh);
void net_test_whitelistswitch(xsh_t *xsh);
void net_test_aodvstate(tty_t *term );
void net_test_stapower(xsh_t *xsh);
void net_test_sendmode(xsh_t *xsh);
void net_test_snidset(xsh_t *xsh);
void net_defineparent(tty_t *term);
#endif

#if defined(STATIC_NODE)
void net_test_rfcftrpt(xsh_t *xsh);
void net_test_netrpt(xsh_t *xsh);

#endif

S8 check_version(U8  *ManufactorCode, U8 *check_ver,U8 * check_innerver , VERSION_t *version , U8 cnt);
void net_appreq_smacaddr(xsh_t *xsh);
void net_appreq_saccessinfo(xsh_t *xsh);
void net_appreq_qaccessinfo(tty_t *term);
void net_queryinfo(tty_t *term);
void net_readnnib(tty_t *term);
void net_test_delroute(xsh_t *xsh);
void net_test_accesslistswitch(xsh_t *xsh);
void net_test_relationswitch(xsh_t *xsh);
void net_test_addnode(xsh_t *xsh);
void net_test_levelset(xsh_t *xsh);
void net_showrfbandmap(xsh_t *xsh);
#endif