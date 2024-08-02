/*
 * Copyright: (c) 2006-2010, 2011 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	mdio.h
 * Purpose:	mdio interface
 * History:
 *	Nov 13, 2014	tgni    Create
 */
#ifndef __MDIO_H__
#define __MDIO_H__

#include "types.h"

extern void mdio_init(void);
extern int32_t mdio_read(uint16_t phy, uint16_t reg, uint16_t *val);
extern int32_t mdio_write(uint16_t phy, uint16_t reg, uint16_t val);

#endif /* __MDIO_H__ */
