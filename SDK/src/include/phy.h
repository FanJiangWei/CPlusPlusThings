/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        phy.h
 * Purpose:     PHY layer Programming interfaces
 * History:     12/03/2014, created by ypzhou
 */

#ifndef __PHY_H__
#define __PHY_H__

#include "types.h"
#include "list.h"
#include "tick.h"

extern double    PHY_SFO_DEF;
typedef struct {
	uint32_t symn		:5;
	uint32_t mm		:2;
	uint32_t size		:1;

	uint32_t valid		:3;
	uint32_t pts		:1;	/* if fill tx timestamp in header while tx immediately */
	uint32_t pts_off	:5;	/* offset from header start where the timestmap is filled (unit: byte) */
	uint32_t		:15;
} phy_hdr_info_t;

typedef struct {
	uint32_t pbsz		:2;
	uint32_t pbc		:3;	/* pb counts */
	uint32_t mm		:3;
	uint32_t copy		:4;
	uint32_t fec_rate	:2;
	uint32_t symn		:12;
	uint32_t tmi		:5;	/* tonemap index */
	uint32_t		:1;

	uint32_t crc24		:4;	/* PBs' crc24 identifier 0: ok, 1: err, bits0-3 for pbs0-3 */
	uint32_t crc32		:1;	/* Payload's crc32 identifier 0: ok, 1: err. Only valid if crc32_en is 1 */
	uint32_t crc32_ofs	:4;	/* The byte offset from the last byte. Only valid if crc32_en is 1 */
	uint32_t crc32_en	:1;	/* Enable crc32 check */
	uint32_t		:22;

	uint8_t  *bat;
	void	*robo;
    void	*link;
} phy_pld_info_t;

typedef struct {
	uint32_t phr_mcs	:3;
	uint32_t valid		:3;
	uint32_t pts		:1;
	uint32_t pts_off	:5;
	uint32_t		:20;
} wphy_hdr_info_t;

typedef struct {
	uint32_t pld_mcs	:3;
	uint32_t blkz		:3;
	uint32_t pbc		:3;
	uint32_t crc24		:1;	/* Payload's crc24 identifier 0: ok, 1: err */
	uint32_t crc32		:1;	/* Payload's crc32 identifier 0: ok, 1: err.  Only valid if crc32_en is 1 */
	uint32_t crc32_ofs	:4;	/* The byte offset from the last byte. Only valid if crc32_en is 1 */
	uint32_t crc32_en	:1;	/* Enable crc32 check */
	uint32_t		:16;
} wphy_pld_info_t;

typedef struct {
	void *head;

	void *payload;
	uint32_t length;
	union {
		struct {
			phy_hdr_info_t hi;
			phy_pld_info_t pi;
		};
		struct {
			wphy_hdr_info_t whi;
			wphy_pld_info_t wpi;
		};
	};
} phy_frame_info_t;

enum {
	PHY_EVT_PD_TMOT = 0,
	PHY_EVT_PD_END,
	PHY_EVT_RX_HDR,
	PHY_EVT_RX_END,
	PHY_EVT_RX_FIN,
	PHY_EVT_TX_END,

	WPHY_EVT_SIG_TMOT,
	WPHY_EVT_SIG,
	WPHY_EVT_PHR,
	WPHY_EVT_EORX,
	WPHY_EVT_EFIN,
	WPHY_EVT_EOTX,
	NR_PHY_EVT,
};

typedef struct {
	uint32_t event		:8;
	uint32_t fail		:1;
	uint32_t pdfail		:1;
	uint32_t pulse		:1;
    uint32_t baud		:2;
    uint32_t rfoption	:2;
    uint32_t rfchannel  :8;
    uint32_t		    :9;

	uint32_t stime;
	uint32_t dur;
	int32_t  rgain;

#define RF_SNR_INVAL	(-127)
#define RF_RSSI_INVAL	(-127)
	int32_t  rf_snr;
	int32_t  rf_rssi;
#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
    int32_t  rf_cfohz;
	uint32_t notch_freq;
#endif
	phy_frame_info_t *fi;
} phy_evt_param_t;

#if defined(FPGA_VERSION) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M) || defined(VENUS2M) || defined(VENUS8M)
#define EVT_ZERO_CROSS0		0
#define EVT_ZERO_CROSS1		1
#define EVT_ZERO_CROSS2		2	
#define EVT_ZERO_CROSS3		3
#define EVT_ZERO_CROSS4		4
#define EVT_ZERO_CROSS5		5
#define NR_EVT_ZERO_CROSS	6
#else
#define EVT_ZERO_CROSS0		0
#define NR_EVT_ZERO_CROSS	1
#endif


typedef void (*evt_notify_fn)(phy_evt_param_t *param, void *arg);

extern void register_zero_cross_hook(uint32_t event, evt_notify_fn hook, void *arg);
extern void unregister_zero_cross_hook(uint32_t event);

typedef struct evt_notifier_s {
	uint32_t	event;
	evt_notify_fn	fn;
	void		*arg;
} evt_notifier_t;

static __inline__ void fill_evt_notifier(evt_notifier_t *ntf, uint32_t event, evt_notify_fn fn, void *arg)
{
	ntf->event = event;
	ntf->fn    = fn;
	ntf->arg   = arg;
}

typedef int32_t (*clksync_notify_fn)(phy_evt_param_t *param, void *arg);

typedef struct clksync_notifier_s {
	clksync_notify_fn fn;
	void *arg;
} clksync_notifier_t;

extern int32_t phy_start_tx(phy_frame_info_t *fi, uint32_t *time, evt_notifier_t ntf[], uint32_t nr_ntf);
extern int32_t phy_start_rx(phy_hdr_info_t *hi, uint32_t *time, uint32_t *timeout, evt_notifier_t ntf[], uint32_t nr_ntf);
extern int32_t phy_start_rx_signal(int8_t gain);
extern int32_t phy_start_rx_nbni(phy_hdr_info_t *hi, evt_notify_fn fn);

/* wphy_init -
 * @option: default option
 * @fchz: default frequency 
 * @type: 1 - The rf chip and main chip are packaged together
 *	  2 - The rf chip and main chip are packaged separately
 */
extern int32_t wphy_init(uint8_t option, uint32_t fchz, uint8_t type);
extern int32_t wphy_start_tx(phy_frame_info_t *fi, uint32_t *time, evt_notifier_t ntf[], uint32_t nr_ntf);
extern int32_t wphy_start_rx(uint32_t *time, uint32_t *timeout, evt_notifier_t ntf[], uint32_t nr_ntf);

extern uint8_t wphy_get_blkz(uint32_t pbsz);
extern uint32_t wphy_get_pbsz(uint8_t blkz);
extern uint8_t wphy_get_option(void);
extern int32_t wphy_set_option(uint8_t option);
extern int32_t wphy_set_fchz(uint32_t fchz);
extern uint32_t wphy_get_fchz();
extern uint32_t wphy_get_fckhz();
extern int16_t  wphy_get_pa(void);
extern int32_t  wphy_set_pa(int16_t dbm);
extern int32_t  wphy_set_channel(uint32_t channel);
extern int32_t  wphy_get_channel();
extern uint32_t wphy_preamble_dur(uint8_t option);
extern uint32_t wphy_sig_dur(uint8_t option);
extern uint32_t wphy_phr_dur(uint8_t option, uint8_t phr_mcs);
extern uint32_t wphy_pld_dur(uint8_t option, uint8_t pld_mcs, uint32_t pbsz, uint8_t pbc);
extern uint32_t wphy_get_frame_dur(uint8_t option, uint8_t phr_mcs, uint8_t pld_mcs, uint32_t pbsz, uint8_t pbc);
extern uint32_t wphy_fch_frame_dur(uint8_t option, uint8_t phr_mcs);
extern uint32_t wphy_ch_to_hz(uint8_t option, uint32_t ch);
extern uint32_t wphy_hz_to_ch(uint8_t option, uint32_t hz);
#define WPHY_TEST_NORMAL   0
#define WPHY_TEST_SPURIOUS 1
#define WPHY_TEST_MASK     2
#define WPHY_TEST_EVM      3
extern void wphy_set_mode(uint8_t mode);
extern void wphy_reset_sfo();
extern void wphy_set_sfo(double ppm, uint8_t est);
extern double wphy_get_sfo();
extern double wphy_cfo_hz2ppm(int32_t hz);

/* wphy_set_lpwr -
 * @level 0: normal, 1: low-power, 2: deep low-power
 * @return 0: ok, otherwise: err
 */
extern int32_t wphy_set_lpwr(uint8_t level);
extern int32_t wphy_standby_enable();
/* wphy_detect_rf_version - detect rf version @ system bootup
 * @return - 5: RF_V2, 2: RF_V1
 */
extern int32_t wphy_detect_rf_version();
#define RF_V1_OUTSIDE	1
#define RF_V1_INSIDE	2
#define RF_V2		5
#define RF_V1_WANA	6
/* wphy_detect_rf_version - get rf version after initialization
 * @return - 5: RF_V2, 2: RF_V1 inside, 1: RF_V1 outside
 */
extern int32_t wphy_get_rf_version();
#define wphy_mini_frame_dur(option, phr_mcs)	\
	(wphy_preamble_dur(option) + wphy_sig_dur(option) + wphy_phr_dur(option, phr_mcs) + WPHY_CIFS)
#define wphy_sack_frame_dur(option, phr_mcs)	wphy_mini_frame_dur(option, phr_mcs)


#define WPHY_CHANNEL  (wphy_get_channel())//(wphy_hz_to_ch(wphy_get_option(), wphy_get_fckhz() * 1000))

#define HDR_SIZE_16		0
#define HDR_SIZE_32		1

#define PB72_SIZE		72 
#define PB136_SIZE		136
#define PB264_SIZE		264 
#define PB520_SIZE		520

#define MM_BAT			0
#define MM_BPSK			1
#define MM_QPSK			2
#define MM_8QAM			3
#define MM_16QAM		4

#define FEC_RATE_1_2		0
#define FEC_RATE_16_21		1
#define FEC_RATE_16_18		2

#define PHY_SCM_START_DEF	100
#define PHY_SCM_END_DEF		230
extern int32_t phy_init(void);

#define NR_TONE_MAX		512

#define WPHY_OPTION_1           1
#define WPHY_OPTION_2           2
#define WPHY_OPTION_3           3
#define WPHY_OPTION_MIN         1
#define WPHY_OPTION_MAX         3

#define WPHY_PHR_MCS_MAX    	7
#define WPHY_PLD_MCS_MAX    	7
#define WPHY_BLKZ_MAX      	6
#define WPHY_PBC_MAX      	4

#define WPHY_PBSZ_16 		0
#define WPHY_PBSZ_40 		1
#define WPHY_PBSZ_72 		2
#define WPHY_PBSZ_136		3
#define WPHY_PBSZ_264 		4
#define WPHY_PBSZ_520 		5

#define WPHY_HDR_SIZE		16


#if defined(LIGHTELF2M) || defined(LIGHTELF8M)

typedef struct {
	uint32_t som	:1;	/* the first pb in a mpdu */
	uint32_t eom	:1;	/* the last pb in a mpdu */
#define PBSZ_136          0
#define PBSZ_520          1
#define NR_PB_SIZE        2
	uint32_t pbsz	:1;	/* PB size, 1: 520 byte, 0: 136 byte */
	uint32_t crc	:1;	/* Tx: if need to caculate crc and put it in offset 128/512 bytes, 
                                   Rx: if crc is correct in offset 132/516, 0 -> ok, 1 -> error */
	uint32_t crc32	:1;     /* Rx: if 32 bits crc is correct in offset 128/512 bytes, 
				       0 -> ok, 1 -> error */
	uint32_t	:27;
} pb_desc_t;

#elif defined(UNICORN2M) || defined(UNICORN8M) || defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M)

typedef struct {
	uint32_t som	:1;	/* the first pb in a mpdu */
	uint32_t eom	:1;	/* the last pb in a mpdu */
#define PBSZ_72		  0
#define PBSZ_136          1
#define PBSZ_264	  2
#define PBSZ_520          3
#define NR_PB_SIZE        4
	uint32_t pbsz	:2;	/* PB size, 0: 72; 1: 136; 2: 264; 3: 520, (bytes) */
	uint32_t crc	:1;	/* Tx: if need to caculate crc and put it in offset 64/128/256/512 bytes, 
                                   Rx: if crc is correct in offset 68/132/260/516, 0 -> ok, 1 -> error */
	uint32_t crc32	:1;     /* Rx: if 32 bits crc is correct in offset 64/128/256/512 bytes, 
				       0 -> ok, 1 -> error */
	uint32_t	:26;
} pb_desc_t;

#elif defined(FPGA_VERSION) || defined(VENUS2M) || defined(VENUS8M)

#define PBSZ_72		  0
#define PBSZ_136          1
#define PBSZ_264	  2
#define PBSZ_520          3
#define NR_PB_SIZE        4
typedef struct {
	//pb descriptor deprecated
} pb_desc_t;

#endif

/* fill_pb_desc - fill pb descriptor infomation, and return the offset after pb descriptor.
 */
#if !defined(FPGA_VERSION) && !defined(VENUS2M) && !defined(VENUS8M)
#define fill_pb_desc(_data, _som, _eom, _pbsz, _crc, _crc32)	\
({								\
	pb_desc_t *_desc = (pb_desc_t *)(_data);		\
	_desc->som   = (_som);					\
	_desc->eom   = (_eom);					\
	_desc->pbsz  = (_pbsz);					\
	_desc->crc   = (_crc);					\
	_desc->crc32 = (_crc32);				\
	(void *)((void *)_data + sizeof(pb_desc_t));		\
})
#else
#define fill_pb_desc(_data, _som, _eom, _pbsz, _crc, _crc32)	\
({								\
	(void *)(_data);					\
})
#endif

extern int32_t phy_reset(void);
extern int32_t wphy_reset(void);

extern int32_t phy_force_reset(void);
extern int32_t wphy_force_reset(void);

// 南网：测试项内�?�就�?台体发信标帧，从频偏100ppm开始，等待同�?�时间后开始抄表。�?�果100ppm抄表成功就往上涨（下次就120ppm），如果没抄表成功就�?0ppm开始，依�?��?�长
// 湖南：测试项内�?�就�?台体发信标帧，从频偏5ppm开始，等待同�?�时间后开始抄�?；按5pp，依次�?�长，进行抄�?
#define NR_SFO_EVAL_DEF		(3)
#define NR_SFO_FINE_DEF		(0)

#define NR_SFO_FINE_DEF_TEST (0)
typedef struct sfo_handle_s {
	uint8_t  first		:1;
	uint8_t  adjust		:1;	/* whether or not to adjust tick */
	uint8_t  fine		:4;	/* fine adjustment degree of iteration */
	uint8_t			:2;
	uint8_t	 threshold;
	uint8_t  iterator;

	int32_t  sfo_int_acc;
	INT64S	 sfo_int;
	uint32_t bts;			/* last beacon timestamp */
} sfo_handle_t;

/* phy_reset_sfo - Reset PHY sfo components before starting to evaluating sfo.
 * @hsfo: if NULL the user should initialize it otherwise SDK initialize it.
 * @ppm: sfo initial value.
 * @adjust: whether or not to adjust tick(use only in hsfo not NULL).
 * @threshold: threshold of sfo evaluate(use only in hsfo not NULL).
 * @fine: fine adjustment degree of iteration(use only in hsfo not NULL).
 */
extern void phy_reset_sfo(sfo_handle_t *hsfo, double ppm, uint8_t adjust, uint8_t threshold, uint8_t fine);
extern int32_t phy_iterate_sfo(int32_t *sfo, uint32_t bts, uint32_t brs, sfo_handle_t *hsfo);
#define SFO_DIR_TX	(1 << 0)
#define SFO_DIR_RX	(1 << 1)
#define SFO_CONST_FACTOR 4294.967296
extern void phy_set_sfo(uint32_t dir, int32_t sfo_int, int32_t adjust);
extern double phy_get_sfo(uint32_t dir);
extern void phy_evaluate_sfo(int32_t* sfo_int);
extern void phy_set_sfo1(int32_t sfo_int);

enum {
	PHY_TX_HDR_CNT = 0,
	PHY_TX_PLD_CNT,
	PHY_TX_PB72_CNT,
	PHY_TX_PB136_CNT,
	PHY_TX_PB264_CNT,
	PHY_TX_PB520_CNT,
	PHY_RX_HDR_CNT,
	PHY_RX_PLD_CNT,
	PHY_RX_PB72_CNT,
	PHY_RX_PB136_CNT,
	PHY_RX_PB264_CNT,
	PHY_RX_PB520_CNT,
	PHY_RX_HDR_ERR,
	PHY_RX_PB72_ERR,
	PHY_RX_PB136_ERR,
	PHY_RX_PB264_ERR,
	PHY_RX_PB520_ERR,
	PHY_CNT_CLEAR,
	PHY_RX_PB16_ERR,
	PHY_RX_PB32_ERR,
	PHY_RX_PB16_CNT,
	PHY_RX_PB32_CNT,
	PHY_TX_PB16_CNT,
	PHY_TX_PB32_CNT,
};

extern int32_t phy_state_get(uint32_t idx, uint32_t *val);

extern int32_t phy_smp_mem_dump(void *mem, uint32_t len);
extern uint32_t phy_smp_mem_size();
#define SMP_MEM_SIZE phy_smp_mem_size()


extern void register_clock_sync_hook(clksync_notify_fn hook, void *arg);

extern void phy_frame_rx(phy_frame_info_t *fi, void *context);
extern void wphy_frame_rx(phy_frame_info_t *fi, void *context);

extern void register_phy_hdr_isr(void (*isr)(phy_frame_info_t *, void *));
extern void register_wphy_hdr_isr(void (*isr)(phy_frame_info_t *, void *));

#define HDR_INVALID_ERROR	1	/* crc error */
#define HDR_INVALID_RX_FAIL	2	/* rx fail   */
#define HDR_INVALID_SPURIOUS	3	/* all zero  */
#define HDR_INVALID_PROC	4	/* missing header proc or error proc */

static __inline__ uint32_t header_is_valid(phy_frame_info_t *fi)
{
	return fi->hi.valid == OK;
}
static __inline__ uint32_t wheader_is_valid(phy_frame_info_t *fi)
{
	return fi->whi.valid == OK;
}
extern void * calc_robo(uint8_t tmi, uint8_t pbsz, uint8_t fec_rate, uint8_t copy, uint8_t mm);

extern uint32_t robo_pb_to_pld_sym(uint32_t pbc, void *robo);
extern uint32_t robo_pld_sym_to_pb(uint32_t symn, void *robo);
extern uint32_t rtbat_pb_to_pld_sym(uint32_t pbc, void *robo);
extern uint32_t rtbat_pld_sym_to_pb(uint32_t symn, void *robo);

enum {
	PHY_CMD_SET_SC_SCRM = 0,
	PHY_CMD_SET_GI,
	PHY_CMD_GET_GI,
	PHY_CMD_SET_SCRM_MODE,
	PHY_CMD_GET_SCRM_MODE,
	PHY_CMD_SET_PB_CRC_MODE,
	PHY_CMD_GET_PB_CRC_MODE,
	PHY_CMD_SET_PLD_CRC_MODE,
	PHY_CMD_GET_PLD_CRC_MODE,
	PHY_CMD_SET_SYNCP,
	PHY_CMD_GET_SYNCP,
	PHY_CMD_SET_SCG,
	PHY_CMD_GET_SCG,
	PHY_CMD_SET_RGAIN_EXT,
	PHY_CMD_GET_RGAIN_EXT,
	PHY_CMD_SET_SCMASK,
	PHY_CMD_GET_SCMASK,
	PHY_CMD_SET_PUNC_SEL,
	PHY_CMD_GET_PUNC_SEL,
	PHY_CMD_SET_SC_PAD_EN,
	PHY_CMD_GET_SC_PAD_EN,
	PHY_CMD_SET_PERF_MODE,
	PHY_CMD_GET_PERF_MODE,
	PHY_CMD_SET_PD_THRD,
	PHY_CMD_GET_PD_THRD,
	PHY_CMD_SET_NOTCH_FREQ,
	PHY_CMD_GET_NOTCH_FREQ,
	PHY_CMD_SET_TGAIN,
	PHY_CMD_GET_TGAIN,
	PHY_CMD_SET_RGAIN,
	PHY_CMD_GET_RGAIN,
	PHY_CMD_CLEAR_CNT,
	PHY_CMD_SHOW_TRACE,
	PHY_CMD_SHOW_DEBUG_INFO,
	PHY_CMD_SET_ANA_LPWR,
	PHY_CMD_GET_DCDC_CTR,
	PHY_CMD_SET_DCDC_CTR,
	PHY_CMD_GET_PHASE_TBL,
	PHY_CMD_SET_PHASE_TBL,
	PHY_CMD_GET_HDR_SYM_OFS,
	PHY_CMD_SET_HDR_SYM_OFS,
	PHY_CMD_GET_INTLEAVER_EN,
	PHY_CMD_SET_INTLEAVER_EN,
	PHY_CMD_GET_INTLEAVER_MODE,
	PHY_CMD_SET_INTLEAVER_MODE,
	PHY_CMD_GET_PB_FMT,
	PHY_CMD_SET_PB_FMT,
	PHY_CMD_GET_ROBO_MODE,
	PHY_CMD_SET_ROBO_MODE,
	PHY_CMD_SET_AGC2_STDET,
	PHY_CMD_CLR_AGC2_STDET,
	PHY_CMD_SET_AGC2_REFP_DBMV,
	PHY_CMD_GET_AGC2_GAIN_CFG,
	PHY_CMD_SET_AGC2_GAIN_CFG,
	PHY_CMD_GET_AGC2_STDET_PAPR,
	PHY_CMD_SET_AGC2_STDET_PAPR,
	PHY_CMD_GET_AGC2_REFP_ST_DBMV,
	PHY_CMD_SET_AGC2_REFP_ST_DBMV,
	PHY_CMD_GET_ANA_RX_ICTR,
	PHY_CMD_SET_ANA_RX_ICTR,
	PHY_CMD_GET_ANA_CTR_C,
	PHY_CMD_SET_ANA_CTR_C,
	PHY_CMD_GET_ANA_RX_BPF,
	PHY_CMD_SET_ANA_RX_BPF,
	PHY_CMD_GET_DEC_CLK,
	PHY_CMD_SET_DEC_CLK,
	PHY_CMD_GET_ITR_MAX,
	PHY_CMD_SET_ITR_MAX,
	PHY_CMD_GET_CSI_ERA,
	PHY_CMD_SET_CSI_ERA,
	PHY_CMD_SET_TM_VER,
	PHY_CMD_SET_BB_LPWR,
	PHY_CMD_SET_AUTO_NOTCH_FREQ,
	PHY_CMD_GET_AUTO_NOTCH_FREQ,
    PHY_CMD_SET_IMDET_THRD,
    PHY_CMD_GET_IMDET_THRD,
	PHY_CMD_SET_IMDET_EN,
	PHY_CMD_GET_IMDET_EN,
	PHY_CMD_SET_SYMB_DUR,
};

extern int32_t phy_ioctl(uint32_t cmd, void *arg);
typedef uint32_t phy_dec_clk_t;
typedef uint32_t phy_itr_max_t;
typedef uint16_t phy_csi_era_t;

typedef struct phy_ana_ctr_c_s {
	uint32_t value;
} phy_ana_ctr_c_t;

typedef struct phy_ana_rx_ictr_s {
	uint32_t value;
} phy_ana_rx_ictr_t;

typedef struct phy_ana_rx_bpf_s {
	uint32_t hpf_bw;
	uint16_t lpf_bw;
} phy_ana_rx_bpf_t;

typedef struct phy_agc2_refp_dbmv_s {
	uint8_t value;
} phy_agc2_refp_dbmv_t;

typedef struct phy_agc2_refp_st_dbmv_s {
	uint8_t value;
} phy_agc2_refp_st_dbmv_t;

typedef struct phy_agc2_stdet_papr_s {
	uint16_t value	:10;
	uint16_t	:6;
} phy_agc2_stdet_papr_t;

typedef struct phy_sc_scrm_s {
#define SC_SCRM_LEN	(128)
	uint32_t sc_scrm[SC_SCRM_LEN];
} phy_sc_scrm_t;

typedef struct phy_gi_s {
	uint16_t hdr;			/* gi of hdr */
	uint16_t pld;			/* gi of third and following symbol of payload */
} phy_gi_t;

typedef struct phy_scrm_mode_s {
	uint32_t scrm_mode	:1;	/* 0: scramble before encode and after decode
					   1: scramble after encode and before decode */
	uint32_t scrm_hdr_en	:1;	/* 0: disable hdr scramble, 1: enable hdr scramble */
#define SCRM_POLY_MODE0    (0)
#define SCRM_POLY_MODE1    (1)
	uint32_t poly_mode	:1;	/* 0: mode0, 1: mode1 */
	uint32_t		:29;
} phy_scrm_mode_t;

typedef struct phy_pb_crc_mode_s {
	uint32_t en		:1;	/* 0: disable tx crc; 1: enable tx crc */
#define CRC_MODE_24	(0)
#define CRC_MODE_32	(1)
	uint32_t mode		:1;	/* 0: crc24 , 1: crc32 */
	uint32_t flip		:1;	/* 0: no flip bits, 1: flip bits */
	uint32_t init		:1;	/* crc init value, 0: all zero, 1: all one */
	uint32_t		:28;
} phy_pb_crc_mode_t;

typedef struct phy_pld_crc_mode_s {
	uint32_t flip		:1;	/* 0: no flip bits, 1: flip bits */
	uint32_t init		:1;	/* crc init value, 0: all zero, 1: all one */
	uint32_t		:30;
} phy_pld_crc_mode_t;

typedef struct phy_syncp_s {
	uint32_t len		:5;    /* length of syncp of Preamble, unit: 0.5 symbol, 
					  Odd length should be set, or else, len=len+1. */
	uint32_t		:27;
} phy_syncp_t;

typedef struct phy_scg_s {
	uint16_t start;
	uint16_t end;
	uint32_t gain;
} phy_scg_t;

typedef int32_t phy_rgain_ext_t;

typedef struct phy_notch_band_s {
	uint16_t start;
	uint16_t end;
} phy_notch_band_t;

typedef struct phy_scmask_s {
	uint16_t start;
	uint16_t end;
	uint16_t nr_band;
	phy_notch_band_t *band;
} phy_scmask_t;

#define PHY_PUNC_SEL_HPAV	(0)
#define PHY_PUNC_SEL_GRID	(1)
typedef uint32_t phy_punc_sel_t;

#define PHY_SC_PAD_DISABLE	(0)
#define PHY_SC_PAD_ENABLE	(1)
typedef uint32_t phy_sc_pad_en_t;

#define PHY_PERF_MODE_A		0
#define	PHY_PERF_MODE_B		1
#define	PHY_PERF_MODE_C		2
#define	PHY_PERF_MODE_D		3
typedef uint32_t phy_perf_mode_t;


#define PHY_ANA_LPWR_DISABLE	0
#define PHY_ANA_LPWR_LEVEL1	1	/* default value */
#define PHY_ANA_LPWR_LEVEL2	2	/* deep low-power */
typedef uint32_t phy_ana_lpwr_t;

typedef uint32_t phy_bb_lpwr_t;

typedef struct phy_dcdc_ctr_s {
	uint32_t vctr	:6;
	uint32_t sfreq	:2;	/* dcdc switch frequency */
	uint32_t	:24;
} phy_dcdc_ctr_t;

typedef struct phy_pd_thrd_s {
	uint32_t thrd	:24;		/* pd threshold, [0, 0xffffff] */
	uint32_t	:8;
} phy_pd_thrd_t;

#define PHY_NOTCH_FREQ_MAX	3
typedef struct phy_notch_freq_s {
	uint32_t freq;
	uint32_t idx;
} phy_notch_freq_t;

typedef struct phy_tgain_t {
	int16_t  dig;
	int16_t  ana;
} phy_tgain_t;

typedef int32_t phy_rgain_t;

typedef struct phy_agc2_gain_cfg_s {
	uint8_t init_value;
	uint8_t max_value;
} phy_agc2_gain_cfg_t;

#define PHASE_TBL_2012		(0)
#define PHASE_TBL_2016		(1)
typedef int32_t phy_phase_tbl_t;

#define HDR_SYM_OFS_2012	(0)
#define HDR_SYM_OFS_2016	(1)
typedef int32_t phy_hdr_sym_ofs_t;

#define INTLEAVER_DISABLE	(0)
#define INTLEAVER_ENABLE	(1)
typedef int32_t phy_intl_en_t;

#define INTLEAVER_BPLC		(0)
#define INTLEAVER_HPAV		(1)
typedef int32_t phy_intl_mode_t;

#define PB_FORMAT_0		(0)
#define PB_FORMAT_1		(1)
typedef int32_t phy_pb_fmt_t;

#define ROBO_MODE_0		(0)
#define ROBO_MODE_1		(1)
typedef int32_t phy_robo_mode_t;

typedef struct phy_agc2_stdet_s {
	int32_t band;
} phy_agc2_stdet_t;

#define TM_VER_0		(0)
#define TM_VER_1		(1)
typedef int32_t phy_tm_ver_t;

#define NR_AUTO_NOTCH_FREQ	(8)
typedef struct phy_auto_notch_freq_s {
	uint32_t freq[NR_AUTO_NOTCH_FREQ];
} phy_auto_notch_freq_t;

typedef struct {
	uint32_t mm	:3;
	uint32_t fecr	:2;
	uint32_t copy	:4;
	uint32_t level	:3;		/* snr level */
	uint32_t	:20;

	double  snr_ave;
	int8_t *snr_tone;		/* memory to store each of tone's snr */
} phy_tonemap_para_t;
extern int32_t phy_pld_snr_esti(phy_frame_info_t *fi, phy_tonemap_para_t *tm);

typedef struct {
	int32_t freq;
	double sir;
} phy_nbni_info_t;


static __inline__ void bat_set_bn(uint8_t *bat, uint8_t bn, int32_t idx)
{
	bat[idx >> 1] = (idx & 1) ? (bat[idx >> 1] | (bn << 4)) : bn;
}

static __inline__ uint8_t bat_get_bn(uint8_t *bat, int32_t idx)
{
	return (idx & 1) ? (bat[idx >> 1] >> 4) : (bat[idx >> 1] & 0xf);
}

typedef struct {
	uint8_t  bat[NR_TONE_MAX >> 1];
	uint32_t pbsz	 :2;
	uint32_t fec_rate:2;
	uint32_t valid	 :1;
	uint32_t	 :27;
} ce_result_t;

extern void register_tick_ctimer_isr(void (*isr)(uint32_t idx));

#define PHASE_NONE		0
#define PHASE_A			1
#define PHASE_B			2
#define PHASE_C			3
#define PHASE_N			4
#define NR_PHASE		3
#define NR_PHASE_ORDER		4

#define PHY_PHASE_ALL		((1 << PHASE_A) | (1 << PHASE_B) | (1 << PHASE_C))
#define PHY_PHASE_A		(1 << PHASE_A)
#define PHY_PHASE_B		(1 << PHASE_B)
#define PHY_PHASE_C		(1 << PHASE_C)


#define TO_PHY_PHASE(x) (((x) == PHASE_NONE) ? PHY_PHASE_ALL : (1 << (x)))

#if defined (FPGA_VERSION) 
#define AGC2_MAX_GAIN		(54)
#elif defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M)  || defined(VENUS2M) || defined(VENUS8M)
#define AGC2_MAX_GAIN		(60)
#endif

extern uint32_t get_phy_tick_time(void);
extern void set_phy_tick_time(uint32_t time);
extern int16_t get_phy_tick_adj(void);
extern void set_phy_tick_adj(int16_t adj);

extern int32_t phy_sal_init(uint8_t band_index);

enum {
	PHY_STATUS_IDLE = 0,
	PHY_STATUS_TX,
	PHY_STATUS_WAIT_PD,
	PHY_STATUS_PD,
	PHY_STATUS_RX_HDR,
	PHY_STATUS_RX_PLD,
};

extern int32_t phy_get_status(void);

enum {
	WPHY_STATUS_IDLE = 0,
	WPHY_STATUS_TX,
	WPHY_STATUS_WAIT_SIG,
	WPHY_STATUS_SIG,
	WPHY_STATUS_RX_HDR,
	WPHY_STATUS_RX_PLD,
};

extern int32_t wphy_get_status(void);

/**
 * phy_start_tx_sin - tx sin wave
 * @tone: 80-490
 * @adj:  adjusting tx gain. adj should be less than or equal to 0 (unit: dB).
 * @return:
 *	0: OK, -1: ERROR
 */
extern int32_t phy_start_tx_sin(uint16_t tone, int8_t adj);

/* phy_get_pb_crc24_err - get rx pb crc error
 * @return: bitmap of pbs crc24 checksum, bit0 for pb0, bit1 for pb1, ... etc.
 * @Note: Only the least significant 4 bits are valid.
 */
extern int32_t phy_get_pb_crc24_err(void);

/* phy_get_payload_crc32_err - get rx payload crc error
 * @return: the result of payload crc32 checksum.
 * @Note: Only when the phy_pld_info_t.crc32_en bit is set.
 */
extern int32_t phy_get_payload_crc32_err(void);

/* caliberation register definition
 */
typedef struct wphy_clibr_data_s {
	uint16_t rf_config0;
	uint16_t rf_config1;
	uint16_t rf_config2;
	uint16_t rf_config3;
} wphy_clibr_data_t;
extern int32_t wphy_get_clibr_data(wphy_clibr_data_t *data);
extern int32_t wphy_set_clibr_data(wphy_clibr_data_t data);

/* caliberation based on snr interface
 */
enum {
	RF_CAL_IDLE = 0,
	RF_CAL_RUNNING,
	RF_DCOC_START,
	RF_DCOC_DATA,
	RF_DCOC_QUERY,
	RF_DCOC_REPLY,
	RF_AMP_START,
	RF_AMP_DATA,
	RF_AMP_QUERY,
	RF_AMP_REPLY,
	RF_PHASE_START,
	RF_PHASE_DATA,
	RF_PHASE_QUERY,
	RF_PHASE_REPLY,
};
extern uint32_t rf_cal_get_time(void);
extern uint8_t rf_cal_get_state(void);
extern int32_t rf_cal_get_snr(void);
extern void rf_cal_master_start(void);
extern void rf_cal_master_stop(void);
extern void rf_cal_slave_start(void);
extern void rf_cal_slave_stop(void);
extern void rf_cal_set_dc_max(uint32_t dc_max);
#define RF_CAL_MODE_ALL		0
#define RF_CAL_MODE_AP		1
extern void rf_cal_set_mode(uint8_t mode);

/* caliberation based on fft interface */
/* master */
extern void wphy_clibr_init(void);
extern void wphy_clibr_uninit(void);
extern void wphy_clibr_start(void); 
extern void wphy_clibr_stop(void);
extern int32_t wphy_get_clibr_data(wphy_clibr_data_t *data);
extern int32_t wphy_set_clibr_data(wphy_clibr_data_t data);
extern int32_t wphy_set_clibr_dc_max(uint8_t dc_max);
extern int32_t wphy_set_clibr_img_step(uint8_t step);
extern int32_t wphy_set_clibr_dc_step(uint8_t step);
extern int32_t wphy_get_clibr_reg_data(wphy_clibr_data_t *data);

enum {
	CLIBR_STATE_IDLE = 0,
	CLIBR_STATE_RUNNING,
	CLIBR_STATE_FIN,
	CLIBR_STATE_ERR = 0xFF,	
};
extern int32_t wphy_get_clibr_state(void);

/* slave */
extern void wphy_clibr_slave_init(void);
extern void wphy_clibr_slave_uninit(void);
extern void wphy_clibr_slave_start(void);
extern void wphy_clibr_slave_stop(void);
extern int32_t wphy_get_clibr_slave_state(void);
extern void wphy_clibr_tx_dcoc(int32_t delay, int32_t step); 

typedef struct wphy_clibr_dc_data_s {
	uint32_t diff0;
	uint32_t diff1;
} wphy_clibr_dc_data_t;
extern int32_t wphy_get_clibr_master_data(wphy_clibr_dc_data_t *data);
extern int32_t wphy_get_clibr_slave_data(wphy_clibr_dc_data_t *data);

extern double wphy_get_noise_rssi(void);

#endif	/* end of __PHY_H__ */
