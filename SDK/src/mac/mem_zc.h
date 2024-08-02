/*
 * Copyright: (c) 2019, 2019 ZC Technology, Inc.
 * All Rights Reserved.
 * 
 * File:        MEM_zc.H
 * Purpose:     demo procedure for using 
 * History:
 */
 
#ifndef __MEM_ZC_H__
#define __MEM_ZC_H__

#include "trios.h"
#include "sys.h"
#include "LIST.h"
#include "Mbuf.h"
#include "plc.h"
#include "framectrl.h"

typedef struct mem_record_list_s {
	list_head_t	link;
	
	uint8_t    *addr;
	const char  *s;
	uint32_t     time   :24;
	uint32_t     type   :8;
	uint32_t     tick;
	U8 			 runState;
}mem_record_list_t;



#define MEMNUM    8


#if defined(ZC3951CCO)
#define	MEM_NR_A 2000//2000 //USE DEVICE_LIST_EACH
#else
#define	MEM_NR_A 20 //short message
#endif

#if defined(ZC3951CCO)
#define	MEM_NR_B 650 //信标最大安排225
#else
#define	MEM_NR_B 100 //信标最大安排225
#endif
#if defined(ZC3951CCO)
#define	MEM_NR_C 1800//use NEIGHBOR_DISCOVERY_TABLE_ENTRY_t
#elif defined(ZC3750CJQ2)
#define	MEM_NR_C 250//use NEIGHBOR_DISCOVERY_TABLE_ENTRY_t
#else

#if defined(RISCV) || defined(VENUS_V3)
#define	MEM_NR_C 300//800//use NEIGHBOR_DISCOVERY_TABLE_ENTRY_t
#else
#define	MEM_NR_C 1000//800//use NEIGHBOR_DISCOVERY_TABLE_ENTRY_t
#endif

#endif
#define	MEM_NR_D 20
#define	MEM_NR_E 30//20
#define	MEM_NR_F 30//20
#if defined(ZC3951CCO)
#define	MEM_NR_G 30
#define	MEM_NR_H 30
#else
#if defined(RISCV) || defined(VENUS_V3)
#define	MEM_NR_G 10
#define	MEM_NR_H 10

#else
#define	MEM_NR_G 20
#define	MEM_NR_H 20

#endif


#endif

#define MEM_RECORD_SIZE sizeof(mem_record_list_t)
#define MEM_MBUF_HDR_SIZE (sizeof(mbuf_hdr_t)+sizeof(mbuf_t)+sizeof(frame_ctrl_t))




#define	MEM_SIZE_A (MEM_RECORD_SIZE + 20) // 20bytes  
#define	MEM_SIZE_B (MEM_RECORD_SIZE + sizeof(bpts_t)) // 48bytes
#define	MEM_SIZE_C (MEM_RECORD_SIZE + 72+ MEM_MBUF_HDR_SIZE) // 48bytes
#define	MEM_SIZE_D (MEM_RECORD_SIZE + 140 + MEM_MBUF_HDR_SIZE)  //for 68pbsz 132pbsz 
#define	MEM_SIZE_E (MEM_RECORD_SIZE + 268 + MEM_MBUF_HDR_SIZE)  //for 260pbsz
#define	MEM_SIZE_F (MEM_RECORD_SIZE + 524 + MEM_MBUF_HDR_SIZE)  //for 516pbsz
#define	MEM_SIZE_G (MEM_RECORD_SIZE + 1048 + MEM_MBUF_HDR_SIZE) //for 516*2pbsz
#define	MEM_SIZE_H (MEM_RECORD_SIZE + 2096 + MEM_MBUF_HDR_SIZE) //for 516*4pbsz

enum {
	MEM_A ,
	MEM_B ,
	MEM_C ,
	MEM_D ,
	MEM_E ,
	MEM_F ,
	MEM_G ,
	MEM_H ,
	MEM_UK,
};


/**********************MEM A**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_A];
	list_head_t	link;
} MEMA_t;

typedef struct
{
    pool_t Po_t;
    MEMA_t MEM_A_t[MEM_NR_A];
} MEMAPOOL_t;

MEMAPOOL_t MEM_A_Pool_t;
/**********************MEM A**********************/

/**********************MEM B**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_B];
	list_head_t	link;
} MEMB_t;

typedef struct
{
    pool_t Po_t;
    MEMB_t MEM_B_t[MEM_NR_B];
} MEMBPOOL_t;

MEMBPOOL_t MEM_B_Pool_t;

/**********************MEM B**********************/




/**********************MEM C**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_C];
	list_head_t	link;
} MEMC_t;

typedef struct
{
    pool_t Po_t;
    MEMC_t MEM_C_t[MEM_NR_C];
} MEMCPOOL_t;

MEMCPOOL_t MEM_C_Pool_t;

/**********************MEM C**********************/


/**********************MEM D**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_D];
	list_head_t	link;
} MEMD_t;

typedef struct
{
    pool_t Po_t;
    MEMD_t MEM_D_t[MEM_NR_D];
} MEMDPOOL_t;

MEMDPOOL_t MEM_D_Pool_t;

/**********************MEM D**********************/

/**********************MEM E**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_E];
	list_head_t	link;
} MEME_t;

typedef struct
{
    pool_t Po_t;
    MEME_t MEM_E_t[MEM_NR_E];
} MEMEPOOL_t;

MEMEPOOL_t MEM_E_Pool_t;

/**********************MEM E**********************/

/**********************MEM F**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_F];
	list_head_t	link;
} MEMF_t;

typedef struct
{
    pool_t Po_t;
    MEMF_t MEM_F_t[MEM_NR_F];
} MEMFPOOL_t;

MEMFPOOL_t MEM_F_Pool_t;

/**********************MEM F**********************/


/**********************MEM G**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_G];
	list_head_t	link;
} MEMG_t;

typedef struct
{
    pool_t Po_t;
    MEMG_t MEM_G_t[MEM_NR_G];
} MEMGPOOL_t;

MEMGPOOL_t MEM_G_Pool_t;

/**********************MEM G**********************/
/**********************MEM H**********************/

typedef struct
{
    uint8_t buf[MEM_SIZE_H];
	list_head_t	link;
} MEMH_t;

typedef struct
{
    pool_t Po_t;
    MEMH_t MEM_H_t[MEM_NR_H];
} MEMHPOOL_t;

MEMHPOOL_t MEM_H_Pool_t;

/**********************MEM H**********************/



#define MEM_LIST_MAX_NUM (MEM_NR_A+MEM_NR_B+MEM_NR_C+MEM_NR_D+MEM_NR_E+MEM_NR_F+MEM_NR_G+MEM_NR_H)
#define MEM_TIME_OUT 50

#define	MEM_USED_MAX 70     //cal mem used
    
typedef struct
{
    const char  s[8];
    U32  all_num;
    U16  type_num[MEM_UK];
}__PACKED MEM_NAME_TYPE_t;

#define MEM_INFO_LEN sizeof(MEM_NAME_TYPE_t)

list_head_t MEM_RECORD_LIST;

#define data_to_mem(data, off)	((mem_record_list_t *)((uint8_t *)(data) - (off) - sizeof(mem_record_list_t)))


void *zc_malloc_mem(uint16_t size,char *s,uint16_t time);
uint8_t zc_free_mem(void *p);


void mem_pool_init(void);




























 
#endif 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
