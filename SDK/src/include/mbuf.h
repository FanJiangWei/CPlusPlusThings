/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        mbuf.h
 * Purpose:     message buffer used in soft switch
 * History:     03/02/2015, created by ypzhou
 *              
 */

#ifndef __MBUF_H__
#define __MBUF_H__

#include "types.h"
#include "list.h"
#include "cpu.h"
#include "tri_malloc.h"
#include "phy.h"
#include "sys.h"

typedef struct mpdu_desc_s {
	uint32_t snid		:24;
	uint32_t retrans	:8;
	uint32_t stime;
	int16_t  snr;
	int16_t  rssi;
	int16_t  rgain;
	uint16_t snr_level	:3;
	uint16_t acki		:1;
	uint16_t backoff	:5;
	uint16_t ufs		:1;	/* update sof frame when request ack && send_type is broadcast */
	uint16_t if_type	:1;	/* interface type, 0: plc, 1: wireless */
	uint16_t		:5;
	float    cfo;

	phy_frame_info_t fi;

	void *from;
} mpdu_desc_t;

typedef struct usr_hdr_s {
	uint16_t orig;			/* original source */
	uint16_t stei;
	uint16_t dest;			/* final destination */
	uint16_t relay;

	uint32_t bcast_dir	:4;
	uint32_t pkt		:4;
	uint32_t send_type	:3;
	uint32_t sport		:2;
	uint32_t hop_count	:4;
	uint32_t ttl		:8;
	uint32_t if_type	:1;
	uint32_t		:6;

	int16_t  rgain;
	int16_t  rssi;
	int16_t  snr;
	int16_t  rsv;

	float    cfo;

	uint32_t snid		:24;
	uint32_t snr_level	:3;
	uint32_t		:5;
} usr_hdr_t;

typedef struct pb_info_s {
	int16_t  rgain;
	int16_t  rssi;
	int16_t  snr;
	float    cfo;
	uint32_t snid		:24;
	uint32_t snr_level	:3;
	uint32_t if_type	:1;
	uint32_t		:4;
} pb_info_t;

typedef struct work_s{
	list_head_t link;
//	int32_t posttick;
//	int32_t recvtick;

	uint8_t  msgtype;
    uint8_t  record_index;
    uint8_t res[2];

	void (*work)(struct work_s *);

	uint8_t buf[0];
}__PACKED work_t;

typedef work_t plc_work_t;

typedef struct {
	// atomic_t ref;
	// mem_allocator_t *allocator;
	// void *mem;


	uint32_t snid		:24;
	uint32_t lid	    :8;
	uint32_t stime;
	
    int8_t  rgain;
    int8_t  bandidx;
	int16_t  snr		:8;    //信噪比
	int16_t  rssi		:8;    //信号强度

	uint16_t snr_level	:3;
	uint16_t acki		:1;
    uint16_t backoff	:5;     /*backoff turns 0-8*/
    uint16_t ufs		:1;     /* update sof frame when request ack && send_type is broadcast */
    uint16_t priority   :2;
    uint16_t retrans    :4;
    uint16_t LinkType   :1;     //记录数据来源  0:HPLC  1:RF
    uint16_t rfoption   :2;
    uint16_t rfchannel  :8;
	uint16_t phase		:2;
    uint16_t Res        :3;
	uint32_t bct;   
	phy_frame_info_t fi;

    plc_work_t *cnfrm;

	union {
		uint8_t     data[0];
		uint16_t    data16[0];
	  	uint32_t    data32[0];
		mpdu_desc_t dp[0];
		usr_hdr_t   uhdr[0];
		pb_info_t   pbi[0];
	};
} mbuf_t;

#define data_to_mbuf(data, off)	((mbuf_t *)((void *)(data) - (off) - sizeof(mbuf_t)))

typedef struct mbuf_hdr_s {
	list_head_t link;
	mbuf_t *buf;

	void (*work)(struct mbuf_hdr_s *);

    uint32_t nif	  :4;
    uint32_t sendflag :1;  //记录当前数据是否被发送成功
    uint32_t	:2;
    uint32_t cloned	:1;
    uint32_t phase  :2;
    uint32_t len	:22;

	uint32_t timestamp;

	uint8_t *head;
	uint8_t *beg;
	uint8_t *end;
	uint8_t *tail;
} mbuf_hdr_t;



/*

mbuf diagram:

  mem +-----------------+	<-- mbuf get
      |   mbuf_hdr_t    |
      |-----------------|
      |     mbuf_t      |
      |-----------------|
      |			|
      |			|
      |	     data	|
      |                 |
      |			|
      |-----------------|

      s     ......	s

      |-----------------|
      |   mbuf_hdr_t    |	<-- mbuf yahdr
      |-----------------|

      s     ......	s

      |-----------------|
      |   mbuf_hdr_t    |       <-- mbuf yahdr
      |-----------------|
      |                 |
      |                 |
      +-----------------+

*/

#define MEM_ALIGNMENT_SIZE	32
#define BUF_MEM_ALIGN(size)	(((size) + MEM_ALIGNMENT_SIZE - 1) & (~(MEM_ALIGNMENT_SIZE - 1)))
#define BUF_PANIC()		sys_panic("[%s:%d]<mbuf panic> Error!!!\r\n", __func__, __LINE__)


extern mbuf_hdr_t * mbuf_get(mem_allocator_t *allocator, uint32_t size);
extern void mbuf_put(mbuf_hdr_t *hdr);

/* mbuf_yahdr - yet another header
 * @buf:
 */
extern mbuf_hdr_t * mbuf_yahdr(mbuf_hdr_t *hdr);

/**
 * mbuf_add - add data to the end of a buffer
 * @hdr:
 * @len:
 */
static __inline__ uint8_t * mbuf_add(mbuf_hdr_t *hdr, uint32_t len)
{
	uint8_t *tmp = hdr->end;

	hdr->end += len;
	hdr->len += len;
	return tmp;
}

/**
 * mbuf_remove - remove data from the end of a buffer
 * @hdr:
 * @len:
 */
static __inline__ uint8_t * mbuf_remove(mbuf_hdr_t *hdr, uint32_t len)
{
	uint8_t *tmp = hdr->end;

	hdr->end -= len;
	hdr->len -= len;
	return tmp;
}

/**
 * mbuf_push - add data to the start of a buffer
 * @hdr:
 * @len:
 */
static __inline__ uint8_t * mbuf_push(mbuf_hdr_t *hdr, uint32_t len)
{
	hdr->beg -= len;
	hdr->len += len;
	return hdr->beg;
}

/**
 * mbuf_pull - remove data from the start of a buffer
 * @hdr:
 * @len:
 */
static __inline__ uint8_t * mbuf_pull(mbuf_hdr_t *hdr, uint32_t len)
{
	hdr->len -= len;
	hdr->beg += len;
	return hdr->beg;
}

/**
 * mbuf_reserve - adjust headroom
 * @hdr:
 * @len:
 */
static __inline__ void mbuf_reserve(mbuf_hdr_t *hdr, uint32_t len)
{
	hdr->beg += len;
	hdr->end += len;
}


#endif	/* end of __MBUF_H__ */
