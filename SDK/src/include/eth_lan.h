/*
 * Copyright: (c) 2010-2020, 2015 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	eth_lan.h
 * Purpose:	eth lan data structure and interface	
 * History:
 *	June 8, 2015	tgni	Create	
 */

#ifndef __ETH_LAN_H__
#define __ETH_LAN_H__

#include "types.h"
#include "list.h"

#define IP101G			0x243
#define IP1001			0x243
#define AR8035			0x4d
#define LAN8710A		0x7
#define KSZ8041			0x22

#define PHY_CTRL		0
#define PHY_STAT		1
#define PHY_VENDOR		2

#define NULL_VENDOR		0xFFFF
#define NR_PHY_ENT_MAX		8


#define MAX_PHY_ID		7
#define NULL_PHY_ID		0xFF

#define PHY_OP_OK		0
#define PHY_OP_ERR		-1

#define PHY_LINK_DOWN		0
#define PHY_LINK_UP		1

#define PHY_MII_MODE		0
#define PHY_RMII_MODE		1
#define PHY_RGMII_MODE		2

#define PHY_NORMAL		0
#define PHY_LOOPBACK		1

#define PHY_FORCE		0
#define PHY_AUTO		1

#define PHY_SPEED_10M		0
#define PHY_SPEED_100M		1
#define PHY_SPEED_1000M		2

#define PHY_DUPLEX_HALF		0
#define PHY_DUPLEX_FULL		1

typedef struct eth_phy_s {
	uint16_t	vendor;
	void 		(*eth_phy_init)();
	uint8_t 	(*get_phy_link)();
	uint8_t		(*get_phy_mode)();
	uint8_t 	(*get_phy_loopback)();
	int32_t		(*set_phy_loopback)(uint8_t);
	uint8_t		(*get_phy_auto)();
	int32_t		(*set_phy_auto)(uint8_t);
	uint8_t 	(*get_phy_speed)();
	int32_t		(*set_phy_speed)(uint8_t);
	uint8_t 	(*get_phy_duplex)();
	int32_t		(*set_phy_duplex)(uint8_t);
} eth_phy_t;

extern uint8_t common_get_phy_link();
extern uint8_t common_get_phy_loopback();
extern int32_t common_set_phy_loopback(uint8_t en);
extern uint8_t common_get_phy_auto();
extern int32_t common_set_phy_auto(uint8_t en);
extern int32_t common_set_phy_speed(uint8_t speed);
extern int32_t common_set_phy_duplex(uint8_t duplex);
extern void common_eth_phy_init(eth_phy_t *eth_phy);

extern uint8_t eth_phy_id(void);
extern int32_t register_phy_device(eth_phy_t *device);
extern void eth_phy_scan(void);
extern int32_t eth_lan_init(void);
extern eth_phy_t *eth_phy_get(void);

extern void register_ar8035();
extern void register_ip1001();
extern void register_lan8710a();
extern void register_ksz8041();
extern void register_ip101g();

#endif /* __ETH_LAN_H__ */
