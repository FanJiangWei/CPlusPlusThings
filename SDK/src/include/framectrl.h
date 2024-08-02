/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        framectrl.h
 * Purpose:     MPDU frame control information for China Grid Standard Q/GDW 1376.3
 * History:     11/03/2014, created by ypzhou
 */

#ifndef __FRAME_CTRL_H__
#define __FRAME_CTRL_H__

#include "types.h"
#include "sys.h"
#include <string.h>

#if defined(STD_2016) || defined(STD_DUAL)
typedef struct frame_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;	/* std_2016: 0, std_2012: 1, std_gd: 1 */
	uint32_t snid		:24;

	uint8_t	vf[8];			/* variable field */
	uint8_t 		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED frame_ctrl_t;

#define NR_SYMN_PER_FRAME_MAX	511
typedef struct beacon_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t bts;

	uint32_t stei		:12;
	uint32_t tmi		:4;
	uint32_t symn		:9;
	uint32_t phase		:2;
	uint32_t		:5;

	uint8_t 		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED beacon_ctrl_t;

typedef struct sof_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t stei		:12;
	uint32_t dtei		:12;
	uint32_t lid		:8;

	uint32_t fl		:12;	/* unit: 10us */
	uint32_t pbc		:4;
	uint32_t symn		:9;
	uint32_t bcsti		:1;
	uint32_t repeat		:1;
	uint32_t enc		:1;
	uint32_t tmi		:4;

	uint8_t tmiext		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED sof_ctrl_t;

typedef struct sack_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t result		:4;
	uint32_t state		:4;
	uint32_t stei		:12;
	uint32_t dtei		:12;

	uint32_t pbc		:3;
	uint32_t		:5;
	uint32_t snr		:8;
	uint32_t load		:8;
	uint32_t		:8;

	uint8_t	ext_dt		:4;	/* 0 is sack otherwise reserver */
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED sack_ctrl_t;

#define COORD_TICK2DUR(tick)		PHY_TICK2MS(tick)	/* 1ms */
#define COORD_TICK2BSO(tick)		PHY_TICK2MS(tick)	/* 1ms */
#define COORD_BSO2MS(bso)		(bso)
typedef struct sack_search_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint8_t addr[6];

	uint16_t stei :12;
	uint16_t res  :4;

	uint8_t	ext_dt		:4;	/* 0 is sack 1 is search 2 is syh otherwise reserver */
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED sack_search_ctrl_t;
typedef struct sack_syh_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t ts;

	uint32_t stei :12;
	uint32_t res  :20;
	
	uint8_t	ext_dt		:4;	/* 0 is sack 1 is search 2 is syh otherwise reserver */
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED sack_syh_ctrl_t;

typedef struct coord_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t dur		:16;
	uint32_t bso		:16;

	uint32_t neighbour	:24;
#if defined(STD_DUAL)
	uint32_t rfchan_id	:8;
    uint32_t rfoption     :2;
    uint32_t res        :2;
#else
    uint32_t res		:12;
#endif

    uint8_t std         :4;

	uint8_t fccs[3];
} __PACKED coord_ctrl_t;

#if defined(STD_DUAL)
typedef frame_ctrl_t rf_frame_ctrl_t;

typedef struct rf_beacon_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t bts;

	uint32_t stei		:12;
	uint32_t mcs		:4;
	uint32_t blkz		:4;
	uint32_t		:12;

	uint8_t 		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED rf_beacon_ctrl_t;

typedef struct rf_sof_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t stei		:12;
	uint32_t dtei		:12;
	uint32_t lid		:8;

	uint32_t fl		:12;
	uint32_t blkz		:4;
	uint32_t 		:9;
	uint32_t bcsti		:1;
	uint32_t repeat		:1;
	uint32_t enc		:1;
	uint32_t mcs 		:4;

	uint8_t 		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED rf_sof_ctrl_t;

typedef struct rf_sack_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint32_t result		:4;
	uint32_t		:4;
	uint32_t stei		:12;
	uint32_t dtei		:12;

	uint32_t 		:8;
	uint32_t rssi		:8;
	uint32_t load		:8;
	uint32_t		:8;

	uint8_t	ext_dt		:4;	/* 0 is sack otherwise reserver */
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED rf_sack_ctrl_t;

typedef struct rf_coord_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:5;
	uint32_t snid		:24;

	uint8_t  mac[6];
	uint8_t  rfchan_id;		/* neighbor's rf channel id */
	uint8_t  rsv;

	uint8_t			:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED rf_coord_ctrl_t;
#endif

#elif defined(STD_2012)
typedef struct frame_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;	/* Must be 1 at least now */
	uint8_t snid		:4;	/* [1, 15] */

	uint8_t	vf[12];			/* variable field */

	uint8_t fccs[3];
} __PACKED frame_ctrl_t;

#define NR_SYMN_PER_FRAME_MAX	511
typedef struct beacon_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;
	uint8_t snid		:4;

	uint32_t bts;
	uint32_t bpc;			/* beacon period count */

	uint32_t stei		:12;
	uint32_t tmi		:4;
	uint32_t symn		:9;
	uint32_t		:1;
	uint32_t phase		:2;
	uint32_t 		:4;

	uint8_t fccs[3];
} __PACKED beacon_ctrl_t;

typedef struct sof_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:1;
	uint32_t snid		:4;
	uint32_t stei		:12;
	uint32_t dtei		:12;

	uint32_t lid		:8;
	uint32_t 		:16;
	uint32_t pbc		:4;
	uint32_t tmi		:4;

	uint32_t fl		:12;	/* unit: 10us */
	uint32_t		:9;
	uint32_t bcsti		:1;
	uint32_t repeat		:1;
	uint32_t symn		:9;

	uint8_t			:4;
	uint8_t tmiext		:4;

	uint8_t fccs[3];
} __PACKED sof_ctrl_t;

typedef struct sack_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;
	uint8_t snid		:4;

	uint8_t result		:4;
	uint8_t state		:4;

	uint16_t dtei		:12;
	uint16_t pbc		:4;

	uint8_t rsvd[9];

	uint8_t fccs[3];
} __PACKED sack_ctrl_t;

#define COORD_TICK2DUR(tick)	(mod_ceiling(PHY_TICK2US(tick), 25))	/* 25us */
#define COORD_TICK2BSO(tick)	(PHY_TICK2US(tick) / 250)		/* 250us */
#define COORD_TICK2BEO(tick)	(PHY_TICK2US(tick) / 250)		/* 250us */
#define COORD_DUR2MS(beo)	((beo) * 25 / 1000)
#define COORD_BEO2MS(beo)	((beo) * 250 / 1000)
#define COORD_BSO2MS(beo)	((beo) * 250 / 1000)
typedef struct coord_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:1;
	uint32_t snid		:4;
	uint32_t		:1;
	uint32_t snid_bmp_high	:9;
	uint32_t		:14;

	uint32_t		:10;
	uint32_t dur		:14;
	uint32_t cof		:1;
	uint32_t bef		:1;
	uint32_t snid_bmp_low	:6;

	uint16_t beo;
	uint16_t bso;

	uint8_t	rsv		:8;

	uint8_t fccs[3];
} __PACKED coord_ctrl_t;

#elif defined(STD_GD) || defined(STD_SPG) 
typedef struct frame_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;	/* Must be 1 at least now */
	uint8_t snid		:4;	/* [1, 15] */

	uint8_t	vf[11];			/* variable field */

	uint8_t 		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED frame_ctrl_t;

#define NR_SYMN_PER_FRAME_MAX	511
typedef struct beacon_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;
	uint8_t snid		:4;

	uint32_t bts;
	uint32_t bpc;			/* beacon period count */

	uint16_t stei		:12;
	uint16_t tmi		:4;

	uint16_t symn		:9;
	uint16_t		:1;
	uint16_t phase		:2;
	uint16_t std		:4;

	uint8_t fccs[3];
} __PACKED beacon_ctrl_t;

typedef struct sof_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:1;
	uint32_t snid		:4;
	uint32_t stei		:12;
	uint32_t dtei		:12;

	uint32_t lid		:8;
	uint32_t 		:16;
	uint32_t pbc		:4;
	uint32_t tmi		:4;

	uint32_t fl		:12;	/* unit: 10us */
	uint32_t		:9;
	uint32_t tf		:1;	/* is phy check tei, fix to 1 at this std */
	uint32_t repeat		:1;
	uint32_t symn		:9;

	uint8_t tmiext		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED sof_ctrl_t;

typedef struct sack_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;
	uint8_t snid		:4;

	uint8_t result		:4;
	uint8_t state		:4;

	uint16_t dtei		:12;
	uint16_t pbc		:4;

	uint8_t rsvd[8];

	uint8_t ext_dt		:4;
	uint8_t std		:4;

	uint8_t fccs[3];
} __PACKED sack_ctrl_t;

#define COORD_TICK2DUR(tick)	(mod_ceiling(PHY_TICK2MS(tick), 40))	/* 40ms */
#define COORD_TICK2BSO(tick)	(PHY_TICK2MS(tick) / 4)			/* 4ms */
#define COORD_TICK2BEO(tick)	(PHY_TICK2MS(tick) / 4)			/* 4ms */
#define COORD_DUR2MS(beo)	((beo) * 40)
#define COORD_BEO2MS(beo)	((beo) * 4)
#define COORD_BSO2MS(beo)	((beo) * 4)

typedef struct sack_search_ctrl_s {
	uint32_t dt			:3;	
	uint32_t access		:1;	
	uint32_t snid		:4;	
	
	uint32_t result     :4;	
	uint32_t state      :4;	
	
	uint32_t dtei       :12;	
	uint32_t pbc        :4;	
	
	uint8_t addr[6];	
	
	uint16_t stei 		:12;	
	uint16_t res  		:4;	
	
	uint8_t	ext_dt		:4;	/* 0 is sack 1 is search 2 is syh otherwise reserver */	
	uint8_t std			:4;	
	
	uint8_t fccs[3];
}__PACKED sack_search_ctrl_t;
typedef struct sack_syh_ctrl_s {	
	uint32_t dt			:3;	
	uint32_t access		:1;	
	uint32_t snid		:4;	
	
	uint32_t result     :4;	
	uint32_t state      :4;	
	
	uint32_t dtei       :12;	
	uint32_t pbc        :4;	
	
	uint32_t ts;	
	
	uint32_t stei      :12;	
	uint32_t res       :20;		
	
	uint8_t	ext_dt		:4;	/* 0 is sack 1 is search 2 is syh otherwise reserver */	
	uint8_t std			:4;	
	
	uint8_t fccs[3];
} __PACKED sack_syh_ctrl_t;

typedef struct coord_ctrl_s {
	uint8_t dt		:3;
	uint8_t access		:1;
	uint8_t snid		:4;

	uint16_t neighbour;

	uint32_t		:18;
	uint32_t dur		:14;

	uint8_t			:1;
	uint8_t bef		:1;
	uint8_t			:6;

	uint16_t beo;
	uint16_t bso;

	uint8_t			:4;
	uint8_t	std		:4;

	uint8_t fccs[3];
} __PACKED coord_ctrl_t;
#else
#error "Unknown std version!"
#endif

typedef struct sound_ctrl_s {
	uint32_t dt		:3;
	uint32_t access		:1;
	uint32_t snid		:4;
	uint32_t stei		:12;
	uint32_t dtei		:12;

	uint32_t		:28;
	uint32_t tmi		:4;

	uint32_t fl		:12;	/* unit: 10us */
	uint32_t		:11;
	uint32_t symn		:9;

	uint8_t			:4;
	uint8_t tmiext		:4;

	uint8_t fccs[3];
} __PACKED sound_ctrl_t;

enum {
	DT_BEACON	= 0,
	DT_SOF		= 1,
	DT_SACK		= 2,
	DT_COORD	= 3,
	DT_SOUND	= 4,
	DT_SILENT	= 7,
	NR_DT		= 8
};

#define BCAST_TEI	4095
#define FL_UNIT		(10 * PHY_TICKS_PER_US)
#define RFFL_UNIT	(100 * PHY_TICKS_PER_US)
#define FL_BIT_MAX	12


#define ACK_RX_OK		0
#define ACK_RX_CRC_FAIL		1
#define ACK_RX_DECRY_FAIL	2

#define NR_SOF_SYMN_MAX		512


extern const char *m_frame_str[];
static __inline__ const char * frame2str(uint8_t dt)
{
	if (dt < NR_DT)	return m_frame_str[dt];
	if (dt > NR_DT) return "n/a";

	return "error";
}


static __inline__ uint8_t str2frame(const char *str)
{
	uint32_t i;

	for (i = 0; i < NR_DT; ++i) {
		if (__strcmp(m_frame_str[i], str) == 0)
			break;
	}

	return i;
}


#endif	/* end of __FRAME_CTRL_H__ */
