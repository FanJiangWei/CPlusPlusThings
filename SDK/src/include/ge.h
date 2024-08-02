/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:	ge.h
 * Purpose:	ge interface 
 * History:
 *	Nov 13, 2014	tgni    Create
 */
#ifndef __GE_H__
#define __GE_H__

#include "types.h"
#include "mbuf.h"

#ifndef O_NONBLOCK
#define O_NONBLOCK		04000U
#endif

enum GE_CMD_E {
	GE_RX_EN,
	GE_TX_EN,
	GE_RX_DIS,
	GE_TX_DIS,
	GE_SET_RST,
	GE_GET_STATUS,
	GE_SET_MODE,
	GE_GET_LOOPBACK,
	GE_SET_LOOPBACK,
	GE_GET_HW_STATS,
	GE_GET_SW_STATS,
	GE_CLR_SW_STATS,
	GE_GET_PHY_ID,
	GE_REGISTER_PHY,
	GE_GET_PHASE,
	GE_SET_PHASE,
	GE_CMD_INVALID
};

#define GE_LINK_DOWN		0
#define GE_LINK_UP		1

#define GE_FORCE		0
#define GE_AUTO			1

#define GE_SPEED_10M		0
#define GE_SPEED_100M		1
#define GE_SPEED_1000M		2

#define GE_DUPLEX_HALF		0
#define GE_DUPLEX_FULL		1

#define GE_MODE_MII		0
#define GE_MODE_RMII		1
#define GE_MODE_RGMII		2

typedef struct ge_status_s {
	uint8_t link;
	uint8_t auto_ng;
	uint8_t speed;
	uint8_t duplex;
	uint8_t mode;
} ge_status_t;

typedef struct ge_mode_s {
	uint8_t auto_ng;
	uint8_t speed;
	uint8_t duplex;
} ge_mode_t;

#define GE_LOOP_DIS		0
#define GE_LOOP_EN		1

typedef struct ge_loopback_s {
	uint8_t in;
	uint8_t out;
} ge_loopback_t;

typedef struct ge_hw_statis_s {
	uint32_t rx_pkt;
	uint32_t rx_uc;
	uint32_t rx_mc;
	uint32_t rx_bc;
	uint32_t rx_pause;
	uint32_t rx_unk_ctl;
	uint32_t rx_under_min;
	uint32_t rx_minto63;
	uint32_t rx_64;
	uint32_t rx_65to127;
	uint32_t rx_128to255;
	uint32_t rx_256to511;
	uint32_t rx_512to1023;
	uint32_t rx_1024to1518;
	uint32_t rx_1519tomax;
	uint32_t rx_over_max;
	uint32_t rx_undersize;
	uint32_t rx_fragement;
	uint32_t rx_over_size;
	uint32_t rx_jabber;
	uint32_t rx_fcs_err;
	uint32_t rx_align_err;
	uint32_t rx_fifo_overrun;
	uint32_t rx_ch_tag_err;
	uint32_t rx_byte_ok;
	uint32_t rx_byte_err;

	uint32_t tx_pkt;
	uint32_t tx_uc;
	uint32_t tx_mc;
	uint32_t tx_bc;
	uint32_t tx_pause;
	uint32_t tx_minto63;
	uint32_t tx_64;
	uint32_t tx_65to127;
	uint32_t tx_128to255;
	uint32_t tx_256to511;
	uint32_t tx_512to1023;
	uint32_t tx_1024to1518;
	uint32_t tx_1519tomax;
	uint32_t tx_over_max;
	uint32_t tx_crc_err;
	uint32_t tx_single_col;
	uint32_t tx_multi_col;
	uint32_t tx_excess_col;
	uint32_t tx_late_col;
	uint32_t tx_deferral_trans;
	uint32_t tx_crs_lost;
	uint32_t tx_abort;
	uint32_t tx_fifo_underrun;
	uint32_t tx_byte_ok;
	uint32_t tx_byte_err;
} ge_hw_statis_t;

typedef struct ge_sw_statis_s {
	uint32_t rx_pkt;
	uint32_t rx_bytes;
	uint32_t rx_drop;
	uint32_t rx_int_err;
	uint32_t rx_dma_err;

	uint32_t tx_pkt;
	uint32_t tx_bytes;
	uint32_t tx_drop;
	uint32_t tx_full_cnt;
	uint32_t tx_dma_err;

	uint32_t inq_cnt;
	uint32_t outq_cnt;
} ge_sw_statis_t;


#define MAX_PHY_ID		7
typedef uint8_t		eth_phy_id_t;

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

typedef struct ge_eth_phy_s {
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
} ge_eth_phy_t;

typedef struct ge_phase_s {
	uint8_t rxp;
	uint8_t txp;
} ge_phase_t;

typedef struct gei_seg_s {
	int32_t offset;
	int32_t len;
} gei_seg_t;

#define MIN_BUF_SZ			(1 << 15)
#define ge_buf_name(name)		ge_##name##_buf
#define DEFINE_BUF_MEM(name, size)	__CACHEALIGNED uint8_t ge_buf_name(name)[size]

extern int32_t ge_rdy(int32_t flags);
extern int32_t ge_recv(void *buf, int32_t nbytes, int32_t flags);
extern int32_t ge_send(const void *buf, int32_t nbytes);
extern int32_t ge_ioctl(int32_t cmd, void *cfg);
extern void ge_init(void);

static __inline__ void ge_open(void)
{
	ge_ioctl(GE_RX_EN, NULL);
	ge_ioctl(GE_TX_EN, NULL);
}

static __inline__ void ge_close(void)
{
	ge_ioctl(GE_RX_DIS, NULL);
	ge_ioctl(GE_TX_DIS, NULL);
}

typedef void (*packet_handler)(mbuf_hdr_t *hdr);
extern void register_ge_recv_hook(packet_handler fn);
extern void post_msdu_to_ge(mbuf_hdr_t *buf);

#endif /* __GE_H__ */
