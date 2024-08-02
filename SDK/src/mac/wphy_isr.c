#include "wphy_isr.h"
#include "phy_sal.h"
#include "framectrl.h"
#include "wlc.h"
#include "printf_zc.h"
#include "flash.h"
#include "app_dltpro.h"

#if defined(STD_DUAL)

static __LOCALTEXT int32_t wbeacon_hdr_proc(phy_frame_info_t *fi, void *context)
{
    rf_beacon_ctrl_t *bc = (rf_beacon_ctrl_t *)fi->head;

    if (bc->mcs >= WPHY_PLD_MCS_MAX)
        return -1;
    if (bc->blkz >= WPHY_BLKZ_MAX)
        return -2;

    fi->wpi.pld_mcs   = bc->mcs;
    fi->wpi.blkz      = bc->blkz;
    fi->wpi.pbc       = 1;
    fi->wpi.crc32_en  = 1;
    fi->wpi.crc32_ofs = 7;
    fi->length        = fi->wpi.pbc * wphy_get_pbsz(bc->blkz);

    return 0;
}

static __LOCALTEXT int32_t wsof_hdr_proc(phy_frame_info_t *fi, void *context)
{
    rf_sof_ctrl_t *sof = (rf_sof_ctrl_t *)fi->head;

    if (sof->mcs >= WPHY_PLD_MCS_MAX)
        return -1;
    if (sof->blkz >= WPHY_BLKZ_MAX)
        return -2;

    fi->wpi.pld_mcs  = sof->mcs;
    fi->wpi.blkz     = sof->blkz;
    fi->wpi.pbc      = 1;
    fi->wpi.crc32_en = 0;
    fi->length       = fi->wpi.pbc * wphy_get_pbsz(sof->blkz);

    return 0;
}

static __LOCALTEXT int32_t wsack_hdr_proc(phy_frame_info_t *fi, void *context)
{
    fi->wpi.crc32_en = 0;
    fi->wpi.pbc      = 0;
    fi->length       = 0;

    return 0;
}
static __LOCALTEXT int32_t wcoord_hdr_proc(phy_frame_info_t *fi, void *context)
{
    fi->wpi.crc32_en = 0;
    fi->wpi.pbc      = 0;
    fi->length       = 0;

    return 0;
}

typedef int32_t (*wphy_hdr_proc)(phy_frame_info_t *fi, void *context);
__LOCALDATA static wphy_hdr_proc WHDR_PROC[] = {
    wbeacon_hdr_proc,	/* 0, DT_BEACON */
    wsof_hdr_proc,		/* 1, DT_SOF */
    wsack_hdr_proc,		/* 2, DT_SACK */
    wcoord_hdr_proc,	/* 3, DT_COORD */
    NULL,			/* 4 */
    NULL,			/* 5 */
    NULL,			/* 6 */
    NULL,			/* 7, DT_SILENT */
};

__LOCALTEXT __isr__ void wphy_hdr_isr_rx(phy_frame_info_t *fi, void *context)
{
    rf_frame_ctrl_t *head = (rf_frame_ctrl_t *)fi->head;

        if (!wheader_is_valid(fi))
            goto out;
        if(HPLC.testmode != WPHYTPT){
        #if defined(STD_2016) || defined(STD_DUAL)
            if (head->access) {
        #else
            if (!head->access) {
        #endif
                fi->whi.valid = HDR_INVALID_PROC;
                goto out;
            }
        }
        if (!WHDR_PROC[head->dt] || WHDR_PROC[head->dt](fi, context) != OK)
            fi->whi.valid = HDR_INVALID_PROC;
    out:
        wphy_frame_rx(fi, context);
        return;
}



int32_t wphy_isr_init()
{
#if defined(STD_DUAL)
    wl_pma_init();
    int32_t rf_type = RF_V1_INSIDE;
//#if defined(VENUS_V3)
//	rf_type = RF_V2;
//#else    
#if defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
    if ((rf_type = wphy_detect_rf_version()) < 0)
    {
        printf_s("detect rf version failed\n");
#if defined(V233L_3780)
        #if defined(VENUS2M)
        rf_type = RF_V2;
        #else
        rf_type = RF_V1_OUTSIDE;
        #endif
#endif
    }
        
#endif

    if (rf_type != RF_V2 && rf_type != RF_V1_WANA) {
#	if defined(VENUS2M) || defined(MIZAR1M)					/* rf v1 inside */
        rf_type = RF_V1_INSIDE;
#	elif defined(FPGA_VERSION) || defined(VENUS8M) || defined(MIZAR9M)	/* rf v1 outsid */
        rf_type = RF_V1_OUTSIDE;
#	endif
    }
//#endif
    U8 i;
    for (i = 0; i < 10; i++)
    {
        if (wphy_init(WPHY_OPTION_1, 490000000, rf_type) < 0)
        {
            continue;
        }
        else
        {
            break;
        }
    }
    if(i >= 10)
    {
		printf_s("wphy_init err \n");
        return -EPERM;
    }

    printf_s("%d wphy init ok with rf type: %s !\n",i, rf_type == RF_V2 ? "v2" : 
			(rf_type == RF_V1_WANA) ? "wana"  : "v1");

// #if !defined(MIZAR1M) && !defined(MIZAR9M)
#if  defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
	if ((rf_type == RF_V1_OUTSIDE) || (rf_type == RF_V1_INSIDE)) {
		extern int32_t wphy_pa_reset();
		wphy_pa_reset();
	}
#endif

    wphy_clibr_data_t data;

    data.rf_config0 = BOARD_CFG->rf_config0;
    data.rf_config1 = BOARD_CFG->rf_config1;
    data.rf_config2 = BOARD_CFG->rf_config2;
    data.rf_config3 = BOARD_CFG->rf_config3;
    app_printf("rf_config 0 %04x  1 %04x  2 %04x  3 %04x \n",data.rf_config0
                         , data.rf_config1, data.rf_config2, data.rf_config3);

    if (wphy_set_clibr_data(data) < 0)
    {
         return -EPERM;
    }


#if defined(STATIC_MASTER)
    //降低CCO无线发射功率。
    //wphy_set_pa(-6);
#endif
    wphy_set_pa(15);


    wphy_set_option(HPLC.option);
    wphy_set_channel(HPLC.rfchannel);

    register_wphy_hdr_isr(wphy_hdr_isr_rx);
	hplc_do_next();
    wl_do_next();
#endif
//    printf_s("wphy optin:%d, channel:%d, %d\n", wphy_get_option(), wphy_get_channel(), WPHY_CHANNEL);
    return OK;
}

/* RF CALIBERATION BASED SNR */
#if defined(STD_DUAL)
typedef struct rf_snr_data_s {
	int16_t snr_total;
	uint16_t nr;
} __PACKED rf_snr_data_t;

typedef struct msg_rf_data_s {
	uint32_t msg_id		:8;
	uint32_t		:24;
	uint32_t idx;
	int8_t	 snr;
	uint8_t  rsv[4];
	uint8_t  fccs[3];
} __PACKED msg_rf_data_t;

typedef struct rf_cal_ctrl_s {
	uint8_t state; 
	uint8_t cnt;
	uint8_t role;	/* 0: master, 1: slave */
	uint8_t mode;	/* 0: cal all, including dcoc&amp&phase; 1: amp&phase cal only */
	int32_t idx;
	uint32_t time;
	uint8_t cnt_max;
	uint8_t dc_max;
	uint8_t dc_step		:4;
	uint8_t img_step	:4;
	int8_t  best_snr;
	int32_t best_idx;
	phy_frame_info_t fi;
} rf_cal_ctrl_t;
rf_cal_ctrl_t RF_CAL = {
	.state    = RF_CAL_IDLE,
	.cnt      = 0,
	.cnt_max  = 1,
	.dc_max   = 32,
	.dc_step  = 4,
	.img_step = 4,
	.best_snr = 0,
	.best_idx = 0,
};
static char *rf_cal_state_str[] = {
	"rf cal idle",
	"rf cal running",
	"rf dcoc start",
	"rf dcoc data",
	"rf dcoc query",
	"rf dcoc reply",
	"rf amp start",
	"rf amp data",
	"rf amp query",
	"rf amp reply",
	"rf phase start",
	"rf phase data",
	"rf phase query",
	"rf phase reply",
};

extern void (*wphy_hdr_isr)(phy_frame_info_t *, void *context);
extern clksync_notifier_t clock_sync_notifier;

static __LOCALBSS void (*wphy_hdr_isr_save)(phy_frame_info_t *, void *context) = NULL;
static clksync_notify_fn  clksync_fn_save = NULL;

int32_t rf_recv_msg(uint32_t timeout);
int32_t send_msg_dcoc_start();
int32_t send_msg_dcoc_query();
int32_t send_msg_dcoc_data();
int32_t send_msg_amp_start();
int32_t send_msg_amp_query();
int32_t send_msg_amp_data();
int32_t send_msg_phase_start();
int32_t send_msg_phase_query();
int32_t send_msg_phase_data();
int32_t recv_msg_dcoc_start(msg_rf_data_t *msg);
int32_t recv_msg_dcoc_data(msg_rf_data_t *msg);
int32_t recv_msg_dcoc_query(msg_rf_data_t *msg);
int32_t recv_msg_dcoc_reply(msg_rf_data_t *msg);
int32_t recv_msg_amp_start(msg_rf_data_t *msg);
int32_t recv_msg_amp_data(msg_rf_data_t *msg);
int32_t recv_msg_amp_query(msg_rf_data_t *_msg);
int32_t recv_msg_amp_reply(msg_rf_data_t *msg);
int32_t recv_msg_phase_start(msg_rf_data_t *msg);
int32_t recv_msg_phase_data(msg_rf_data_t *msg);
int32_t recv_msg_phase_query(msg_rf_data_t *_msg);
int32_t recv_msg_phase_reply(msg_rf_data_t *msg);
__isr__ void slave_tx_end(phy_evt_param_t *para, void *arg);
__isr__ void slave_rx_end(phy_evt_param_t *para, void *arg);

int32_t increase_dcoc_idx(int32_t idx)
{
	uint32_t i_step, q_step, i_polar, q_polar;

	q_step = idx & 0x7F;
	i_step = (idx >> 7) & 0x7F;
	q_polar = (idx >> 14) & 0x1;
	i_polar = (idx >> 15) & 0x1;

	if ((q_step += RF_CAL.dc_step) >= RF_CAL.dc_max) {
		q_step = 0;
		if ((i_step += RF_CAL.dc_step) >= RF_CAL.dc_max) {
			i_step = 0;
			q_step = 0;
			if (++q_polar > 1) {
				q_polar = 0;
				++i_polar;
			}
		}
	}

	return (i_polar << 15) | (q_polar << 14) | (i_step << 7) | q_step;
}

int32_t reg_dcoc_to_idx(uint16_t rf_config0)
{
	return rf_config0;
}

int32_t reg_idx_to_dcoc(int32_t idx, uint16_t *rf_config0)
{
    if (idx > 65535)
    {
        return -1;
    }

	*rf_config0 = idx;

	return 0;
}

int32_t reg_amp_to_idx(uint16_t rf_config1, uint32_t rf_config2)
{
	return ((rf_config2 >> 12) << 4) | ((rf_config1 >> 2) & 0xf);
}

int32_t reg_idx_to_amp(int32_t idx, uint16_t *rf_config1, uint16_t *rf_config2)
{
	if (idx >= 256)
		return -1;

	*rf_config1 = (uint16_t)((idx & 0xf) << 2);
	*rf_config2 = (uint16_t)((idx >> 4) << 12);

	return 0;
}

int32_t reg_phase_to_idx(uint16_t rf_config3)
{
	return (rf_config3 & 0x3ff);
}

int32_t reg_idx_to_phase(int32_t idx, uint16_t *rf_config3)
{
	if (idx >= 1024)
		return -1;

	*rf_config3 = idx;
	return 0;
}

__isr__ void master_rx_end(phy_evt_param_t *para, void *arg)
{
	phy_frame_info_t *fi = para->fi;
	msg_rf_data_t *msg = (msg_rf_data_t *)fi->head;
	static uint32_t err_cnt = 0;

	if (para->event == WPHY_EVT_SIG_TMOT) {
		switch (RF_CAL.state) {
		case RF_DCOC_QUERY:
			if ((RF_CAL.idx = increase_dcoc_idx(RF_CAL.idx)) < 65536) {
				goto resend;
			} else {
				goto exit;
			}
			break;
		case RF_AMP_QUERY:
			if ((RF_CAL.idx += RF_CAL.img_step) < 256) {
				goto resend;
			} else {
				goto exit;
			}
			break;
		case RF_PHASE_QUERY:
			if ((RF_CAL.idx += RF_CAL.img_step) < 1024) {
				goto resend;
			} else {
				goto exit;
			}
		default:
			break;
		}
		return;
	}

	if (para->fail) {
		debug_printf(&dty, DEBUG_WPHY, "%s para fail\n", __func__);
		if (++err_cnt > 20) {
			err_cnt = 0;
			goto exit;
		}
		rf_recv_msg(10);
		return;
	}

	debug_printf(&dty, DEBUG_WPHY, "Recv msg %s idx %d snr %d\n", msg->msg_id <= RF_PHASE_REPLY ? 
		     rf_cal_state_str[msg->msg_id] : "id N/A", msg->idx, msg->snr);

	switch (RF_CAL.state) {
	case RF_DCOC_QUERY:
		if (msg->msg_id != RF_DCOC_REPLY)
			goto resend;
		recv_msg_dcoc_reply(msg);
		break;
	case RF_AMP_QUERY:
		if (msg->msg_id != RF_AMP_REPLY)
			goto resend;
		recv_msg_amp_reply(msg);
		break;
	case RF_PHASE_QUERY:
		if (msg->msg_id != RF_PHASE_REPLY)
			goto resend;
		recv_msg_phase_reply(msg);
		break;
	default:
		break;
	}
	return;

resend:
	debug_printf(&dty, DEBUG_WPHY, "Re");
	if (RF_CAL.state == RF_DCOC_QUERY)
		send_msg_dcoc_query();
	else if (RF_CAL.state == RF_AMP_QUERY)
		send_msg_amp_query();
	else if (RF_CAL.state == RF_PHASE_QUERY)
		send_msg_phase_query();
	return;
exit:
	debug_printf(&dty, DEBUG_WPHY, "Slave no reply! state %s\n", rf_cal_state_str[RF_CAL.state]);
	RF_CAL.state = RF_CAL_IDLE;
	RF_CAL.best_snr   = 0;
	return;
}

__isr__ void master_tx_end(phy_evt_param_t *para, void *arg)
{
	switch (RF_CAL.state) {
	case RF_DCOC_START:
		break;
	case RF_DCOC_DATA:
		if (++RF_CAL.cnt >= RF_CAL.cnt_max) {
			RF_CAL.cnt = 0;
			if ((RF_CAL.idx = increase_dcoc_idx(RF_CAL.idx)) < 65536) {
				send_msg_dcoc_data();
			} else {
				RF_CAL.idx = 0;
				send_msg_dcoc_query();
			}
		} else {
			send_msg_dcoc_data();
		}
		break;
	case RF_DCOC_QUERY:
		rf_recv_msg(10);
		break;
	case RF_DCOC_REPLY:
		break;
	case RF_AMP_START:
		break;
	case RF_AMP_DATA:
		if (++RF_CAL.cnt >= RF_CAL.cnt_max) {
			RF_CAL.cnt = 0;
			if ((RF_CAL.idx += RF_CAL.img_step) < 256) {
				send_msg_amp_data();
			} else {
				RF_CAL.idx = 0;
				send_msg_amp_query();
			}
		} else {
			send_msg_amp_data();
		}
		break;
	case RF_AMP_QUERY:
		rf_recv_msg(10);
		break;
	case RF_AMP_REPLY:
		break;
	case RF_PHASE_START:
		break;
	case RF_PHASE_DATA:
		if (++RF_CAL.cnt >= RF_CAL.cnt_max) {
			RF_CAL.cnt = 0;
			if ((RF_CAL.idx += RF_CAL.img_step) < 1024) {
				send_msg_phase_data();
			} else {
				RF_CAL.idx = 0;
				send_msg_phase_query();
			}
		} else {
			send_msg_phase_data();
		}
		break;
	case RF_PHASE_QUERY:
		rf_recv_msg(10);
		break;
	case RF_PHASE_REPLY:
		break;
	}

	return;
}

int32_t rf_send_msg(msg_rf_data_t *msg)
{
	phy_frame_info_t *fi = &RF_CAL.fi;
	wphy_hdr_info_t *whi = &fi->whi;
	evt_notifier_t ntf;
	uint32_t cpu_sr, stime;
	int32_t ret = 0;

	whi->phr_mcs = 6;
	fi->head     = msg;

	if (!RF_CAL.role) {
		fill_evt_notifier(&ntf, WPHY_EVT_EOTX, master_tx_end, NULL);
	} else {
		fill_evt_notifier(&ntf, WPHY_EVT_EOTX, slave_tx_end, NULL);
	}

	cpu_sr = OS_ENTER_CRITICAL();
	stime = get_phy_tick_time() + WPHY_CIFS;
	if ((ret = wphy_start_tx(fi, &stime, &ntf, 1)) < 0) {
		printf_s("start wtx frame failed, ret = %d\n", ret);
	}
	OS_EXIT_CRITICAL(cpu_sr);

	debug_printf(&dty, DEBUG_WPHY, "Send msg %s idx %d\n", msg->msg_id <= RF_PHASE_REPLY ? 
		     rf_cal_state_str[msg->msg_id] : "id N/A", msg->idx);

	return ret;
}

int32_t rf_recv_msg(uint32_t timeout)
{
	evt_notifier_t ntf[2];
	int32_t ret;

	if (timeout) {
		if (!RF_CAL.role) {
			fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, master_rx_end, NULL);
			fill_evt_notifier(&ntf[1], WPHY_EVT_EORX, master_rx_end, NULL);
		} else {
			fill_evt_notifier(&ntf[0], WPHY_EVT_SIG_TMOT, slave_rx_end, NULL);
			fill_evt_notifier(&ntf[1], WPHY_EVT_EORX, slave_rx_end, NULL);
		}

		timeout = PHY_MS2TICK(timeout) + get_phy_tick_time();
		if ((ret = wphy_start_rx(NULL, &timeout, ntf, 2)) < 0) {
			printf_s("start wrx frame failed, ret = %d\n", ret);
			return -1;
		}

	} else {
		if (!RF_CAL.role) {
			fill_evt_notifier(&ntf[0], WPHY_EVT_EORX, master_rx_end, NULL);
		} else {
			fill_evt_notifier(&ntf[0], WPHY_EVT_EORX, slave_rx_end, NULL);
		}
		if ((ret = wphy_start_rx(NULL, NULL, &ntf[0], 1)) < 0) {
			printf_s("start wrx frame failed, ret = %d\n", ret);
			return -1;
		}
	}

	return 0;
}

int32_t send_msg_dcoc_start()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	RF_CAL.idx = 0;
	msg.msg_id = RF_DCOC_START;
	msg.idx    = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_dcoc(msg.idx, &data.rf_config0);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_DCOC_DATA;

	return rf_send_msg(&msg);
}

int32_t send_msg_dcoc_data()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	msg.msg_id = RF_DCOC_DATA;
	msg.idx    = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_dcoc(msg.idx, &data.rf_config0);
	wphy_set_clibr_data(data);

	return rf_send_msg(&msg);
}

int32_t send_msg_dcoc_query()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	msg.msg_id = RF_DCOC_QUERY;
	msg.idx    = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_dcoc(msg.idx, &data.rf_config0);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_DCOC_QUERY;

	return rf_send_msg(&msg);
}

int32_t recv_msg_dcoc_reply(msg_rf_data_t *msg)
{
	wphy_clibr_data_t data;

	wphy_get_clibr_data(&data);
	reg_idx_to_dcoc(msg->idx, &data.rf_config0);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_AMP_START;
	send_msg_amp_start();

	return 0;
}

int32_t send_msg_amp_start()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	RF_CAL.idx = 0;
	msg.msg_id = RF_AMP_START;
	msg.idx    = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_amp(msg.idx, &data.rf_config1, &data.rf_config2);
	wphy_set_clibr_data(data);


	RF_CAL.state = RF_AMP_DATA;
	return rf_send_msg(&msg);	
}

int32_t send_msg_amp_data()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	msg.msg_id = RF_AMP_DATA;
	msg.idx = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_amp(msg.idx, &data.rf_config1, &data.rf_config2);
	wphy_set_clibr_data(data);

	return rf_send_msg(&msg);
}

int32_t send_msg_amp_query()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	msg.msg_id = RF_AMP_QUERY;
	msg.idx = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_amp(msg.idx, &data.rf_config1, &data.rf_config2);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_AMP_QUERY;
	return rf_send_msg(&msg);
}

int32_t recv_msg_amp_reply(msg_rf_data_t *msg)
{
	wphy_clibr_data_t data;

	wphy_get_clibr_data(&data);
	reg_idx_to_amp(msg->idx, &data.rf_config1, &data.rf_config2);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_PHASE_START;
	send_msg_phase_start();

	return 0;
}

int32_t send_msg_phase_start()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	RF_CAL.idx = 0;
	msg.msg_id = RF_PHASE_START;
	msg.idx = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_phase(msg.idx, &data.rf_config3);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_PHASE_DATA;

	return rf_send_msg(&msg);
}

int32_t send_msg_phase_data()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	msg.msg_id = RF_PHASE_DATA;
	msg.idx = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_phase(msg.idx, &data.rf_config3);
	wphy_set_clibr_data(data);

	return rf_send_msg(&msg);
}

int32_t send_msg_phase_query()
{
	msg_rf_data_t msg;
	wphy_clibr_data_t data;

	msg.msg_id = RF_PHASE_QUERY;
	msg.idx = RF_CAL.idx;

	wphy_get_clibr_data(&data);
	reg_idx_to_phase(msg.idx, &data.rf_config3);
	wphy_set_clibr_data(data);

	RF_CAL.state = RF_PHASE_QUERY;
	return rf_send_msg(&msg);
}

extern board_cfg_t BoardCfg_t;
int32_t recv_msg_phase_reply(msg_rf_data_t *msg)
{
	wphy_clibr_data_t data;

	wphy_get_clibr_data(&data);
	reg_idx_to_phase(msg->idx, &data.rf_config3);
	wphy_set_clibr_data(data);

	RF_CAL.best_idx = msg->idx;
	RF_CAL.best_snr = msg->snr;
	RF_CAL.time = time_diff(cpu_get_cycle(), RF_CAL.time) / CPU_CYCLE_PER_MS;
	RF_CAL.state = RF_CAL_IDLE;

	printf_d("Rf cal base snr done! Best snr %d Cost %dms.\n", msg->snr, RF_CAL.time);
	printf_d("Result: conf0 %08x conf1 %08x conf2 %08x conf3 %08x.\n", 
		     data.rf_config0, data.rf_config1, data.rf_config2, data.rf_config3);
    if(FLASH_OK != zc_flash_read(FLASH ,(uint32_t)BOARD_CFG ,(uint32_t *)&BoardCfg_t ,sizeof(board_cfg_t))){
        debug_printf(&dty, DEBUG_WPHY, "%s %d read flash error\n", __func__, __LINE__);
        return -1;
    }

	__memcpy(&BoardCfg_t, BOARD_CFG, sizeof(board_cfg_t));
    //__memcpy(&bcfg, BOARD_CFG, sizeof(board_cfg_t));
    BoardCfg_t.rf_config0 = data.rf_config0;
    BoardCfg_t.rf_config1 = data.rf_config1;
    BoardCfg_t.rf_config2 = data.rf_config2;
    BoardCfg_t.rf_config3 = data.rf_config3;
    os_sleep(10);
#if  defined(V233L_3780) || defined(RISCV) || defined(VENUS_V3)
	wphy_clibr_dc_data_t dc_diff;
	memset(&dc_diff, 0, sizeof(dc_diff));
	wphy_get_clibr_master_data(&dc_diff);
	BoardCfg_t.diff0	= dc_diff.diff0;
	BoardCfg_t.diff1	= dc_diff.diff1;
	BoardCfg_t.snr	    = msg->snr;
	BoardCfg_t.clibr_mode	= RF_CAL.mode == RF_CAL_MODE_AP ? 3 : 2;
	#endif
    if(FLASH_OK != zc_flash_write(FLASH, (uint32_t)BOARD_CFG_ADDR  ,(uint32_t)&BoardCfg_t ,sizeof(board_cfg_t))){
        debug_printf(&dty, DEBUG_WPHY, "%s %d write flash error\n", __func__, __LINE__);
        return -1;
    }
    
    if (zc_timer_query(calibration_report_timer) == TMR_RUNNING)
    {
        timer_stop(calibration_report_timer, TMR_CALLBACK);
    }

    return 0;
}

/* slave */
__isr__ void slave_tx_end(phy_evt_param_t *para, void *arg)
{
	if (RF_CAL.state == RF_PHASE_REPLY)
		rf_recv_msg(500);
	else
		rf_recv_msg(0);
	return;
}

__isr__ void slave_rx_end(phy_evt_param_t *para, void *arg)
{
	phy_frame_info_t *fi = para->fi;
	msg_rf_data_t *msg = (msg_rf_data_t *)fi->head;

	if (para->event == WPHY_EVT_SIG_TMOT) {
		switch (RF_CAL.state) {
		case RF_PHASE_REPLY:
			RF_CAL.state = RF_CAL_IDLE;
			break;
		default:
			break;
		}
		goto error;
	}

	if (para->fail) {
		goto error;
	}

	msg->snr = para->rf_snr;
	debug_printf(&dty, DEBUG_WPHY, "Recv msg %s idx %d snr %d\n", msg->msg_id <= RF_PHASE_REPLY ? 
		     rf_cal_state_str[msg->msg_id] : "id N/A", msg->idx, msg->snr);

	switch (msg->msg_id) {
	case RF_DCOC_START:
		recv_msg_dcoc_start(msg);
		break;
	case RF_DCOC_DATA:
		recv_msg_dcoc_data(msg);
		break;
	case RF_DCOC_QUERY:
		recv_msg_dcoc_query(msg);
		return;
	case RF_AMP_START:
		recv_msg_amp_start(msg);
		break;
	case RF_AMP_DATA:
		recv_msg_amp_data(msg);
		break;
	case RF_AMP_QUERY:
		recv_msg_amp_query(msg);
		return;
	case RF_PHASE_START:
		recv_msg_phase_start(msg);
		break;
	case RF_PHASE_DATA:
		recv_msg_phase_data(msg);
		break;
	case RF_PHASE_QUERY:
		recv_msg_phase_query(msg);
		return;
	default:
		break;
	}
error:
	rf_recv_msg(0);
	return;
}

int32_t recv_msg_dcoc_start(msg_rf_data_t *msg)
{
	if (msg->idx > 65535) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid msg idx %d\n", msg->idx);
		return -1;
	}

	RF_CAL.best_snr = 0;
	RF_CAL.best_idx = 0;

	RF_CAL.state = RF_DCOC_START;

	return 0;
}

int32_t recv_msg_dcoc_data(msg_rf_data_t *msg)
{
	if (RF_CAL.state ==  RF_DCOC_START) {
		RF_CAL.state = RF_DCOC_DATA;
	} else if (RF_CAL.state != RF_DCOC_DATA) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid slave state %d\n", RF_CAL.state);
		RF_CAL.state = RF_DCOC_DATA;
		RF_CAL.best_snr = 0;
		RF_CAL.best_idx = 0;
		return -1;
	}

	if (msg->idx > 65535) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid msg idx %d\n", msg->idx);
		return -2;
	}

	if (RF_CAL.best_snr < msg->snr) {
		RF_CAL.best_snr = msg->snr;
		RF_CAL.best_idx = msg->idx;
	}

	return 0;
}

int32_t recv_msg_dcoc_query(msg_rf_data_t *_msg)
{
	msg_rf_data_t msg;

	msg.msg_id  = RF_DCOC_REPLY;
	msg.idx = RF_CAL.best_idx;
	msg.snr = RF_CAL.best_snr;

	RF_CAL.state = RF_DCOC_REPLY;

	return rf_send_msg(&msg);
}

int32_t recv_msg_amp_start(msg_rf_data_t *msg)
{
	if (msg->idx > 255) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid msg idx %d\n", msg->idx);
		return -1;
	}

	RF_CAL.best_snr = 0;
	RF_CAL.best_idx = 0;

	RF_CAL.state = RF_AMP_START;

	return 0;
}

int32_t recv_msg_amp_data(msg_rf_data_t *msg)
{
	if (RF_CAL.state == RF_AMP_START) {
		RF_CAL.state = RF_AMP_DATA;
	} else if (RF_CAL.state != RF_AMP_DATA) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid slave state %d\n", RF_CAL.state);
		RF_CAL.state = RF_AMP_DATA;
		RF_CAL.best_snr = 0;
		RF_CAL.best_idx = 0;
		return -1;
	}

	if (msg->idx > 255) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid msg idx %d\n", msg->idx);
		return -2;
	}

	if (RF_CAL.best_snr < msg->snr) {
		RF_CAL.best_snr = msg->snr;
		RF_CAL.best_idx = msg->idx;
	}

	return 0;
}

int32_t recv_msg_amp_query(msg_rf_data_t *_msg)
{
	msg_rf_data_t msg;

	msg.msg_id = RF_AMP_REPLY;
	msg.idx = RF_CAL.best_idx;
	msg.snr = RF_CAL.best_snr;

	RF_CAL.state = RF_AMP_REPLY;
	return rf_send_msg(&msg);
}

int32_t recv_msg_phase_start(msg_rf_data_t *msg)
{
	if (msg->idx > 1023) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid msg idx %d\n", msg->idx);
		return -1;
	}

	RF_CAL.best_snr = 0;
	RF_CAL.best_idx = 0;

	RF_CAL.state = RF_PHASE_START;

	return 0;

}

int32_t recv_msg_phase_data(msg_rf_data_t *msg)
{
	if (RF_CAL.state == RF_PHASE_START) {
		RF_CAL.state = RF_PHASE_DATA;
	} else if (RF_CAL.state != RF_PHASE_DATA) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid slave state %d\n", RF_CAL.state);
		RF_CAL.state = RF_PHASE_DATA;
		RF_CAL.best_snr = 0;
		RF_CAL.best_idx = 0;
		return -1;
	}

	if (msg->idx > 1024) {
		debug_printf(&dty, DEBUG_WPHY, "Invalid msg idx %d\n", msg->idx);
		return -1;
	}

	if (RF_CAL.best_snr < msg->snr) {
		RF_CAL.best_snr = msg->snr;
		RF_CAL.best_idx = msg->idx;
	}

	return 0;
}

int32_t recv_msg_phase_query(msg_rf_data_t *_msg)
{
	msg_rf_data_t msg;

	msg.msg_id = RF_PHASE_REPLY;
	msg.idx = RF_CAL.best_idx;
	msg.snr = RF_CAL.best_snr;

	RF_CAL.state = RF_PHASE_REPLY;
	return rf_send_msg(&msg);
}

__LOCALTEXT __isr__ void rf_cal_hdr_isr(phy_frame_info_t *fi, void *context)
{
	wphy_frame_rx(fi, context);
	return;
}

void rf_cal_master_start()
{
    modify_calibration_report_timer();
	if (RF_CAL.state != RF_CAL_IDLE) {
		debug_printf(&dty, DEBUG_WPHY, "rf cal master busy\n");
		return;
	}

	RF_CAL.state    = RF_CAL_RUNNING;
	RF_CAL.cnt      = 0;
	RF_CAL.role     = 0;
	RF_CAL.idx      = 0;
	RF_CAL.time     = cpu_get_cycle();

	wphy_hdr_isr_save = wphy_hdr_isr;
	wphy_hdr_isr = rf_cal_hdr_isr;
	clksync_fn_save = clock_sync_notifier.fn;
	clock_sync_notifier.fn = NULL;

	while (wphy_reset() != OK)
		sys_delayus(100);

	if (RF_CAL.mode == RF_CAL_MODE_ALL)
		send_msg_dcoc_start();
	else if (RF_CAL.mode == RF_CAL_MODE_AP)
		send_msg_amp_start();

	return;
}

void rf_cal_master_stop()
{
	while (wphy_reset() != OK)
		sys_delayus(100);

	wphy_hdr_isr = wphy_hdr_isr_save;
	clock_sync_notifier.fn = clksync_fn_save;

	return;
}

void rf_cal_slave_start()
{
	memset(&RF_CAL, 0, sizeof(RF_CAL));

	RF_CAL.state = RF_CAL_RUNNING;
	RF_CAL.role  = 1;

	wphy_hdr_isr_save = wphy_hdr_isr;
	wphy_hdr_isr = rf_cal_hdr_isr;
	clksync_fn_save = clock_sync_notifier.fn;
	clock_sync_notifier.fn = NULL;

	while (wphy_reset() != OK)
		sys_delayus(100);

	rf_recv_msg(0);

	return;
}

void rf_cal_slave_stop()
{
	while (wphy_reset() != OK)
		sys_delayus(100);

	wphy_hdr_isr = wphy_hdr_isr_save;
	clock_sync_notifier.fn = clksync_fn_save;
	
	return;
}

void rf_cal_set_cnt_max(uint32_t cnt_max)
{
	RF_CAL.cnt_max = cnt_max;
}

void rf_cal_set_dc_max(uint32_t dc_max)
{
	if (dc_max > 128)
		return;
	RF_CAL.dc_max = dc_max;
}

void rf_cal_set_dc_step(uint32_t dc_step)
{
	RF_CAL.dc_step = dc_step;
}

void rf_cal_set_img_step(uint32_t img_step)
{
	RF_CAL.img_step = img_step;
}

void rf_cal_set_mode(uint8_t mode)
{
	if (mode == RF_CAL_MODE_ALL) {
		RF_CAL.mode = mode;
		RF_CAL.img_step = 4;
	} else if (mode == RF_CAL_MODE_AP) {
		RF_CAL.mode = mode;
		RF_CAL.img_step = 1;
	}
}
uint32_t rf_cal_get_time()
{
    return RF_CAL.time;
}

uint8_t rf_cal_get_state()
{
	return RF_CAL.state;
}

int32_t rf_cal_get_snr(void)
{
	return RF_CAL.best_snr;
}

param_t cmd_cal_param_tab[] = {
	{PARAM_ARG|PARAM_TOGGLE, "", "master|slave|state"
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "start|stop|cnt_max|dc_max|dc_step|img_step|mode", "master",
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "", "", "cnt_max",
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "", "", "dc_max",
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "", "", "dc_step",
	},
	{PARAM_ARG|PARAM_INTEGER|PARAM_SUB, "", "", "img_step",
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "all|ap", "mode",
	},
	{PARAM_ARG|PARAM_TOGGLE|PARAM_SUB, "", "start|stop", "slave",
	},
	PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_cal_desc,
	       "caliberation based snr", cmd_cal_param_tab);
__SLOWTEXT void docmd_cal(command_t *cmd, xsh_t *xsh)
{
	tty_t *term = xsh->term;

	if (__strcmp(xsh->argv[1], "master") == 0) {
		if (__strcmp(xsh->argv[2], "start") == 0) {
			rf_cal_master_start();
		} else if (__strcmp(xsh->argv[2], "stop") == 0) {
			rf_cal_master_stop();
		} else if (__strcmp(xsh->argv[2], "cnt_max") == 0) {
			RF_CAL.cnt_max = __strtoul(xsh->argv[3], NULL, 0);
		} else if (__strcmp(xsh->argv[2], "dc_max") == 0) {
			uint32_t dc_max;
			if ((dc_max = __strtoul(xsh->argv[3], NULL, 0)) > 128) {
				term->op->tprintf(term, "dc_max should <= 128.\n");
				return;
			}
			RF_CAL.dc_max = dc_max;
		} else if (__strcmp(xsh->argv[2], "dc_step") == 0) {
			RF_CAL.dc_step = __strtoul(xsh->argv[3], NULL, 0);
		} else if (__strcmp(xsh->argv[2], "img_step") == 0) {
			RF_CAL.img_step = __strtoul(xsh->argv[3], NULL, 0);
		} else if (__strcmp(xsh->argv[2], "mode") == 0) {
			if (__strcmp(xsh->argv[3], "all") == 0) {
				RF_CAL.mode = RF_CAL_MODE_ALL;
			} else if (__strcmp(xsh->argv[3], "ap") == 0) {
				RF_CAL.mode = RF_CAL_MODE_AP;
			}
		}
	} else if (__strcmp(xsh->argv[1], "slave") == 0) {
		if (__strcmp(xsh->argv[2], "start") == 0)
			rf_cal_slave_start();
		else if (__strcmp(xsh->argv[2], "stop") == 0)
			rf_cal_slave_stop();
	} else if (__strcmp(xsh->argv[1], "state") == 0) {
		uint8_t state = rf_cal_get_state();
		term->op->tprintf(term, "rf cal state %s\n", rf_cal_state_str[state]);	
	}
	return;
}
#endif
#endif
