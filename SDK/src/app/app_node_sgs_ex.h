#ifndef __APP_NODE_SGS_EX_H__
#define __APP_NODE_SGS_EX_H__

#include "types.h"
#include "mbuf.h"

#define NODE_SGS

#define APPLICATION_ID0 'Y'
#define APPLICATION_ID1 'X'


typedef struct
{
    U16    ProtocolVer                : 6;
    U16    HeaderLen                  : 6;
    U16    Res                        : 4;
    U16    PacketSeq;
    U8     Data[0];
} __PACKED NODE_SGS_HEADER_t;

#ifdef NODE_SGS
//extern U8 ApplicationSWC;
void sta_sgs_ind(work_t *work);

#endif

#if defined(STATIC_MASTER)

void cco_node_sgs_req(U8 *data);

#endif


#endif

