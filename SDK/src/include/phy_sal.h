/*
 * Copyright: (c) 2014-2020, 2014 Triductor Technology, Inc.
 * All Rights Reserved.
 *
 * File:        phy_sal.h
 * Purpose:     PHY layer related function interface for China Grid Standard Q/GDW 1376.3
 * History:     11/03/2014, created by ypzhou
 */

#ifndef __PHY_SAL_H__
#define __PHY_SAL_H__

#include "types.h"
#include "framectrl.h"
#include "phy.h"
#include "vsh.h"

#define TMI_STD_ROBO_AV		0
#define TMI_HS_ROBO_AV		1
#define TMI_MINI_ROBO_AV	2

#define TMI_ROBO_520_0		0
#define TMI_ROBO_520_1		1
#define TMI_ROBO_136_2		2
#define TMI_ROBO_136_3		3
#define TMI_ROBO_136_4		4
#define TMI_ROBO_136_5		5
#define TMI_ROBO_136_6		6
#define TMI_ROBO_520_7		7
#define TMI_ROBO_520_8		8
#define TMI_ROBO_520_9		9
#define TMI_ROBO_520_10		10

#define TMI_ROBO_264_11		11
#define TMI_ROBO_264_12		12
#define TMI_ROBO_72_13		13
#define TMI_ROBO_72_14		14

#define TMI_EXT_520_1		17
#define TMI_EXT_520_2		18
#define TMI_EXT_520_3		19
#define TMI_EXT_520_4		20
#define TMI_EXT_520_5		21
#define TMI_EXT_520_6		22

#define TMI_EXT_136_10		26
#define TMI_EXT_136_11		27
#define TMI_EXT_136_12		28
#define TMI_EXT_136_13		29
#define TMI_EXT_136_14		30

#define NR_TONEMAP_MAX		32

#if (DDR_MEM_SIZE >= 8)
#define NR_RT_TONEMAP_MAX	64
#else
#define NR_RT_TONEMAP_MAX	32
#endif

typedef struct {
	const char *name;
	uint8_t tmi;
	uint8_t pbsz		:2;
	uint8_t fec_rate	:2;
	uint8_t copy		:4;
	uint16_t kp		:16;

	uint8_t mm		:3;		/* modulation method */
	uint8_t cpf		:1;		/* 0: cfp, 1: cp */
	uint8_t def		:1;		/* 0: def tonemap, 1: rt tonemap */
	uint8_t valid		:1;		/* 0: invalid, 1: valid */
	uint8_t			:2;

	uint32_t pbc_min	:4;
	uint32_t pbc_max	:4;
	uint32_t fec_sz		:24;		/* account to copy number, unit: bit */

	void *robo;
	void *bat;
} tonemap_t;

extern const char *tonemap_name[];
#define fill_sys_tonemap(tm, _tmi, _pbsz, _pbc_min, _pbc_max, _fec_rate, _copy, _mm) do { \
	(tm).name     = tonemap_name[_tmi];			\
	(tm).valid    = TRUE;					\
	(tm).tmi      = _tmi;					\
	(tm).pbsz     = _pbsz;					\
	(tm).pbc_min  = _pbc_min;				\
	(tm).pbc_max  = _pbc_max;				\
	(tm).fec_rate = _fec_rate;				\
	(tm).copy     = _copy;					\
	(tm).mm       = _mm;					\
} while(0)

extern void config_sys_tonemap(tonemap_t *tm);
extern tonemap_t *get_sys_tonemap(uint32_t tmi);
extern void update_tonemap();
extern void phy_tonemap_show(xsh_t *xsh);

#if defined(STD_2012) || defined(STD_GD)
	#define NR_BAND_MAX		(2)
#elif defined(STD_SPG)
	#define NR_BAND_MAX		(3)
#elif defined(STD_2016) || defined(STD_DUAL)
	#define NR_BAND_MAX		(4)
#else
	#error "Unknown std version!"
#endif

typedef struct {
	uint16_t start;
	uint16_t end;
	uint8_t	 idx;
	uint8_t  gain;
	uint8_t  symn		:5;	/* head symbol */
	uint8_t  mm		:2;
	uint8_t  size		:1;
} phy_band_info_t;

extern phy_band_info_t *CURR_B;
extern phy_band_info_t BAND[];
extern phy_band_info_t * phy_band_config(int32_t idx);
extern phy_scmask_t * phy_scmask_config(int32_t idx);
extern void phy_get_band_info(int32_t idx, phy_band_info_t *band);
extern void phy_set_band_info(int32_t idx, phy_band_info_t *band);
extern void phy_gain_config(int8_t ana,int8_t dig);


extern uint16_t OFDM_SYMBOL_DUR;

extern uint16_t PREAMBLE_DUR;		/* 13 * 40.96us, unit: 40ns */
extern uint16_t PMB_1SYM_DUR;
extern uint16_t HDR_1SYM_DUR;
extern uint16_t PAYLOAD_1SYM_DUR;
extern uint16_t PAYLOAD_2SYMS_DUR;
extern uint16_t PAYLOAD_SYM3_DUR;
extern uint16_t PD_EOF_DUR;
extern uint32_t __SCRM[2][SC_SCRM_LEN];

#define FRAME_CTRL_DUR(hdr_symn)	(HDR_1SYM_DUR * (hdr_symn))
/* 10ms * maxfl , one phy frame max duration without ifs */
/* longest ppdu, pbsz:520 fec:1/2 mm:BPSK copy:11 pbc:4 ==> 3028 syms */
#define PHY_FRAME_MAX			PHY_US2TICK(PHY_CIFS + 156670UL)
#define FL_DUR_MAX			(((1UL << FL_BIT_MAX) - 1) * FL_UNIT)
#define RFFL_DUR_MAX			(((1UL << FL_BIT_MAX) - 1) * RFFL_UNIT)

#define NR_FEC_RATE        3
#define NR_MM              5
#define NR_COPY            12

extern const char *fecr_str[];
static __inline__ const char *fecr_getstr(uint8_t rate)
{
	return fecr_str[rate];
}

extern uint16_t pbsz_val[];
static __inline__ uint32_t pb_bufsize(uint8_t pbsz)
{
	return pbsz_val[pbsz] + sizeof(pb_desc_t);
}

extern const char *pbsz_str[];
static __inline__ const char *pbsz_getstr(uint8_t pbsz)
{
	return pbsz_str[pbsz];
}

extern const char *wpbsz_str[];

extern const uint16_t pbsz_fec[NR_PB_SIZE][NR_FEC_RATE];
static __inline__ uint32_t pb_fecsize(uint8_t pbsz, uint8_t fec_rate)
{
	return pbsz_fec[pbsz][fec_rate];
}

extern const char *mm_str[];
static __inline__ const char *mm_getstr(uint8_t mm)
{
	return mm_str[mm];
}

extern const char *phase_str[];
static __inline__ const char * phase2str(uint8_t phase)
{
	return phase_str[phase];
}

static __inline__ uint8_t str2phase(const char *str)
{
	uint32_t i;

	for (i = 0; i < NR_PHASE_ORDER+1; ++i) {
		if (__strcmp(phase_str[i], str) == 0)
			break;
	}

	return i;
}

static __inline__ uint32_t time_to_pld_sym(uint32_t dur)
{
	if (dur >= PAYLOAD_2SYMS_DUR) return 2 + (dur - PAYLOAD_2SYMS_DUR) / PAYLOAD_SYM3_DUR;
	if (dur >= PAYLOAD_1SYM_DUR) return 1;

	return 0;
}

static __inline__ uint32_t pld_sym_to_time(uint32_t symn)
{
	if (symn > 2) return PAYLOAD_2SYMS_DUR + (symn - 2) * PAYLOAD_SYM3_DUR;
	if (symn > 1) return PAYLOAD_2SYMS_DUR;
	if (symn > 0) return PAYLOAD_1SYM_DUR;

	return 0;
}

extern uint32_t PHY_BIFS;           //400  us  突发帧间隔
extern uint32_t PHY_CIFS;           //400  us  竞争帧间隔
extern uint32_t PHY_RIFS;           //400  us  回应帧间隔
extern uint32_t PHY_EIFS;           //20   ms  扩展帧间隔
extern uint32_t PHY_RIFS_MAX;       //2300 us
#define MIN_FRAME_DUR(hdr_symn)  (PREAMBLE_DUR + FRAME_CTRL_DUR(hdr_symn) + PHY_CIFS)
#define SACK_FRAME_DUR(hdr_symn) MIN_FRAME_DUR(hdr_symn)

extern uint32_t WPHY_BIFS;          //800  us
extern uint32_t WPHY_CIFS;          //800  us
extern uint32_t WPHY_RIFS;          //800  us
extern uint32_t WPHY_RIFS_MAX;      //2300 us
extern uint32_t WPHY_EIFS;          //70   ms

static __inline__ uint32_t get_beacon_symn(beacon_ctrl_t *beacon)
{
	return beacon->symn;
}

static __inline__ uint32_t get_sof_pbc(sof_ctrl_t *sof)
{
	return sof->pbc;
}

static __inline__ uint32_t get_sof_tmi(sof_ctrl_t *sof);

static __inline__ uint32_t get_sof_symn(sof_ctrl_t *sof)
{
	tonemap_t *tm;

	if (!(tm = get_sys_tonemap(get_sof_tmi(sof))))
		return 0;

	return robo_pb_to_pld_sym(get_sof_pbc(sof), tm->robo);
}

static __inline__ uint32_t get_sound_symn(sound_ctrl_t *sound)
{
	return sound->symn;
}

static __inline__ uint32_t get_frame_symn(frame_ctrl_t *fc)
{
	switch (fc->dt) {
	case DT_BEACON:
		return get_beacon_symn((beacon_ctrl_t *)fc);
	case DT_SOF:
		return get_sof_symn((sof_ctrl_t *)fc);
	case DT_SOUND:
		return get_sound_symn((sound_ctrl_t *)fc);
	}

	return 0;
}

static __inline__ void set_beacon_symn(beacon_ctrl_t *beacon, uint32_t symn)
{
	beacon->symn = symn;
}

static __inline__ void set_sof_symn(sof_ctrl_t *sof, uint32_t symn)
{
	sof->symn = symn;
}

static __inline__ void set_sound_symn(sound_ctrl_t *sound, uint32_t symn)
{
	sound->symn = symn;
}

static __inline__ void set_frame_symn(frame_ctrl_t *fc, uint32_t symn)
{
	switch (fc->dt) {
	case DT_BEACON:
		set_beacon_symn((beacon_ctrl_t *)fc, symn);
		break;
	case DT_SOF:
		set_sof_symn((sof_ctrl_t *)fc, symn);
		break;
	case DT_SOUND:
		set_sound_symn((sound_ctrl_t *)fc, symn);
		break;
	}
}


static __inline__ uint32_t get_sof_tmi(sof_ctrl_t *sof)
{
#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG) 
	return (sof->tmi != 0xD) ? sof->tmi : (16 | sof->tmiext);
#elif defined(STD_2016) || defined(STD_DUAL)
	return (sof->tmi != 0xF) ? sof->tmi : (16 | sof->tmiext);
#endif
}

static __inline__ uint32_t get_beacon_tmi(beacon_ctrl_t *beacon)
{
	return beacon->tmi;
}

static __inline__ uint32_t get_frame_tmi(frame_ctrl_t *fc)
{
	switch (fc->dt) {
	case DT_BEACON:
		return get_beacon_tmi((beacon_ctrl_t *)fc);
	case DT_SOF:
		return get_sof_tmi((sof_ctrl_t *)fc);
	}

	return (uint32_t)-1;
}


static __inline__ void set_sof_tmi(sof_ctrl_t *sc, uint32_t tmi)
{
	if (tmi < 16) {
		sc->tmi    = tmi;
		sc->tmiext = 0;
	} else {
#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG) 
		sc->tmi    = 0xD;
#elif defined(STD_2016) || defined(STD_DUAL)
		sc->tmi    = 0xF;
#endif
		sc->tmiext = tmi & 0xF;
	}
}

static __inline__ void set_frame_tmi(frame_ctrl_t *fc, uint32_t tmi)
{
	switch (fc->dt) {
	case DT_BEACON:
		((beacon_ctrl_t *)fc)->tmi = tmi;
		break;
	case DT_SOF:
		set_sof_tmi((sof_ctrl_t *)fc, tmi);
		break;
	}
}

static __inline__ uint32_t get_beacon_pbc(beacon_ctrl_t *beacon)
{
	return get_beacon_symn(beacon) > 0 ? 1 : 0;
}

static __inline__ uint32_t get_frame_pbc(frame_ctrl_t *fc)
{
	switch (fc->dt) {
	case DT_BEACON:
		return get_beacon_pbc((beacon_ctrl_t *)fc);
	case DT_SOF:
		return get_sof_pbc((sof_ctrl_t *)fc);
	}

	return 0;
}

static uint32_t get_sof_dtei(sof_ctrl_t *sof)
{
	return sof->dtei;
}

static uint32_t get_sack_dtei(sack_ctrl_t *sack)
{
	return sack->dtei;
}

static __inline__ uint32_t get_frame_dtei(frame_ctrl_t *fc)
{
	switch (fc->dt) {
	case DT_SOF:
		return get_sof_dtei((sof_ctrl_t *)fc);
	case DT_SACK:
		return get_sack_dtei((sack_ctrl_t *)fc);
	}
	
	return -1;
}


static __inline__ uint16_t get_frame_stei(frame_ctrl_t *fc)
{
	return (fc->dt == DT_BEACON) ? ((beacon_ctrl_t *)fc)->stei :
        (fc->dt == DT_SOF) ? ((sof_ctrl_t *)fc)->stei :
         (fc->dt == DT_SACK) ? ((sack_ctrl_t *)fc)->stei :                    -1;
}


static uint32_t get_sof_bcsti(sof_ctrl_t *sof)
{
#if defined(STD_GD) || defined(STD_SPG) 
	return 0;
#else
	return sof->bcsti;
#endif
}

static uint32_t get_sack_bcsti(sack_ctrl_t *sack)
{
	return 0;
}

static __inline__ uint32_t get_frame_bcsti(frame_ctrl_t *fc)
{
	switch (fc->dt) {
	case DT_SOF:
		return get_sof_bcsti((sof_ctrl_t *)fc);
	case DT_SACK:
		return get_sack_bcsti((sack_ctrl_t *)fc);
	}
	
	return 0;
}


static __inline__ uint32_t get_frame_mnbc(frame_ctrl_t *fc)
{
	return 0;
}

static __inline__ void set_sof_fl(sof_ctrl_t *sof, uint32_t fl)
{
	sof->fl = mod_ceiling(fl, FL_UNIT);
}

#if defined(STD_DUAL)
static __inline__ void set_rf_sof_fl(rf_sof_ctrl_t *sof, uint32_t fl)
{
	sof->fl = mod_ceiling(fl, RFFL_UNIT);
}

static __inline__ uint32_t get_rf_sof_fl(rf_sof_ctrl_t *sof)
{
	return sof->fl * RFFL_UNIT;
}
#endif

static __inline__ uint32_t get_sof_fl(sof_ctrl_t *sof)
{
	return sof->fl * FL_UNIT;
}

static __inline__ uint32_t get_frame_dur(phy_frame_info_t *fi)
{
	return PREAMBLE_DUR + FRAME_CTRL_DUR(fi->hi.symn) + pld_sym_to_time(fi->pi.symn);
}

static __inline__ uint32_t get_rf_frame_dur(phy_frame_info_t *fi)
{
	return wphy_get_frame_dur(wphy_get_option(), fi->whi.phr_mcs, fi->wpi.pld_mcs, wphy_get_pbsz(fi->wpi.blkz), fi->wpi.pbc);
}

/*
 * |<----------------- 136/520-Byte -------------->|
 *
 * SOF PB format for ChinaGrid & HomeplugAV
 * +---------------------- ~ ----------------------+
 * |4-Byte|           128/512-Byte          |4-byte|
 * | pbh  |               body              | pbcs |
 * +---------------------- ~ ----------------------+
 *                                   |      |
 * Beacon PB format for ChinaGrid    |      |
 * +--------------- ~ -----------------------------+
 * |           128/512-byte          |4-byte|4-byte|
 * |               pld               |pld cs| pbcs |
 * +--------------- ~ -----------------------------+
 *                                   |      |
 * Beacon PB formwat for HomeplugAV  |      |
 * +------------------ ~ --------------------------+
 * |              132/516-byte              |4-byte|
 * |                  pld                   | pbcs |
 * +------------------ ~ --------------------------+
 *                                   |      |
 *                                   |      |
 *
 *                                   1      2
 *
 * a) Setting bit `crc' in either `pbh_t' or `bph_t' will generate a crc32
 *    at position `1', covering the first 128/512 bytes.
 * b) Setting field `crc' in `phy_pld_info_t' will generate a crc24/crc32
 *    at position `2', covering the first 132/516 bytes.
 */



extern const uint16_t body_size[];

#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
#define PB136_BODY_SIZE		128
#define PB520_BODY_SIZE		512
#elif defined(STD_2016) || defined(STD_DUAL)
#if defined(STD_DUAL)
#define PB16_BODY_SIZE		12
#define PB40_BODY_SIZE		36
#endif
#define PB72_BODY_SIZE		68
#define PB136_BODY_SIZE		132
#define PB264_BODY_SIZE		260
#define PB520_BODY_SIZE		516

#if defined(STD_DUAL)
#define PB16_BODY_BLKZ   0
#define PB40_BODY_BLKZ   1
#define PB72_BODY_BLKZ   2
#define PB136_BODY_BLKZ  3
#define PB264_BODY_BLKZ  4
#define PB520_BODY_BLKZ  5
#endif


#define PB136_BODY_SIZE_NO_CRC_BEACON	 129
#define PB520_BODY_SIZE_NO_CRC_BEACON    513
#endif

/* pbh_t - Physical Block Header, the most significant 32 bits are for
 * hardware usage, that is to say, the most significant 32 bits doesn't exist in SOF's PB.
 */
typedef struct {
#if defined(LIGHTELF2M) || defined(LIGHTELF8M)
	uint32_t som	:1;	/* the first pb in a mpdu */
	uint32_t eom	:1;	/* the last pb in a mpdu */
	uint32_t pbsz	:1;	/* PB size, 1: 520 byte, 0: 136 byte */
	uint32_t crc	:1;	/* Tx: if need to caculate crc and put it in offset 128/512 bytes, 
                                   Rx: if crc is correct in offset 132/516, 0 -> ok, 1 -> error */
	uint32_t crc32	:1;     /* Rx: if 32 bits crc is correct in offset 128/512 bytes, 
				       0 -> ok, 1 -> error */
	uint32_t	:27;
#elif defined(UNICORN2M) || defined(UNICORN8M) || defined(ROLAND1M) || defined(ROLAND1_1M) || defined(MIZAR1M) || defined(ROLAND9_1M) || defined(MIZAR9M)
	uint32_t som	:1;	/* the first pb in a mpdu */
	uint32_t eom	:1;	/* the last pb in a mpdu */
	uint32_t pbsz	:2;	/* PB size, 0: 72; 1: 136; 2: 264; 3: 520, (bytes) */
	uint32_t crc	:1;	/* Tx: if need to caculate crc and put it in offset 64/128/256/512 bytes, 
                                   Rx: if crc is correct in offset 68/132/260/516, 0 -> ok, 1 -> error */
	uint32_t crc32	:1;     /* Rx: if 32 bits crc is correct in offset 64/128/256/512 bytes, 
				       0 -> ok, 1 -> error */
	uint32_t	:26;
#endif
#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
	uint16_t  ssn;		/* segment sequence number */
	uint16_t  mfsf  :1;	/* Mac frame start block flag */
	uint16_t  mfef  :1;	/* Mac frame end block flag */
	uint16_t        :14;
#elif defined(STD_2016) || defined(STD_DUAL)
	uint8_t  ssn	:6;	/* segment sequence number */
	uint8_t  mfsf   :1;	/* Mac frame start block flag */
	uint8_t  mfef   :1;	/* Mac frame end block flag */
#endif
} __PACKED pbh_t;

typedef struct wpbh_s {
	uint8_t  ssn	:6;	/* segment sequence number */
	uint8_t  mfsf   :1;	/* Mac frame start block flag */
	uint8_t  mfef   :1;	/* Mac frame end block flag */
} __PACKED wpbh_t;

/* pbcs_t - Physical Block Check Sum, crc24 for ChinaGrid and crc32 for
 * HomePlugAV respectively.
 */
typedef struct {
#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
	uint32_t	:8;
	uint32_t pbcs	:24;
#elif defined(STD_2016) || defined(STD_DUAL)
	uint8_t  pbcs[3];
#endif
} __PACKED pbcs_t;

typedef pb_desc_t bph_t;

static __inline__ uint8_t get_bph_sz(uint8_t rf)
{
#if defined(MIZAR9M) || defined(MIZAR1M)
	return rf ? 0 : sizeof(bph_t); 
#else
	return sizeof(bph_t);
#endif
}

/* bpcs_t - Beacon Payload Check Sum, crc32
 */
typedef struct {
	uint32_t bpcs;
} bpcs_t;


#if defined(STD_2012) || defined(STD_GD) || defined(STD_SPG)
#define BP520_BODY_SIZE		512
#define BP136_BODY_SIZE		128
#elif defined(STD_2016) || defined(STD_DUAL)
#define BP520_BODY_SIZE		513 
#define BP136_BODY_SIZE		129
#endif

typedef struct {
	bph_t   h;
	uint8_t b[BP520_BODY_SIZE];
	bpcs_t  c;
	pbcs_t  t;
} __PACKED bp_520_t;

typedef struct {
	bph_t   h;
	uint8_t b[BP136_BODY_SIZE];
	bpcs_t  c;
	pbcs_t  t;
} __PACKED bp_136_t;


#endif	/* end of __PHY_SAL_H__ */
