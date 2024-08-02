/*
 * Copyright: (c) 2014-2020, 2014 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:		ProtocolProc.h
 * Purpose:	ProtocolProc interface
 * History:
 *	june 24, 2017	panyu    Create
 */
#ifndef __ZCSYSTEM_H__
#define __ZCSYSTEM_H__
#include <stdlib.h>
#include "trios.h"
#include "sys.h"
#include "vsh.h"
#include "que.h"
#include "mbuf.h"
#include "types.h"
#include "list.h"

typedef unsigned int		U32;
typedef unsigned short		U16;
typedef unsigned char		U8;

typedef int			S32;
typedef short			S16;
typedef signed char		S8;

#define    MACADRDR_LENGTH             6

extern U32  SystenRunTime;


typedef struct
{
    U8   *pMallocHeader;
    U8   *pPayload;
    U16  PayloadLen;
} __PACKED MDB_t;

typedef struct
{
    U16  PayloadLen;
    U8   Payload[0];
} __PACKED DATA_t;


typedef struct
{
    U8       SrcTask;
    U8       DesTask;
    U8  	 Protocol;
    U8	     MsgType;
    U8       *pMsgData;
    list_head_t	link;
} __PACKED  MSG_t;

#define phy_debug_printf(fmt, arg...) debug_printf(&dty, DEBUG_PHY, fmt, ##arg)


#endif /* __ZCSYSTEM_H__ */

