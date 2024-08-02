#ifndef SCANRFCHMANAGE_H
#define SCANRFCHMANAGE_H
#include "types.h"
#include "mbuf.h"

typedef enum{
    
    //optin 2 G1
    e_op2_ch8,
    e_op2_ch16,
    e_op2_ch24,
    e_op2_ch32,
    e_op2_ch36,
    e_op2_ch40,
    e_op2_ch44,
    e_op2_ch48,
    e_op2_ch56,
    e_op2_ch64,
    //optin 2 G2
    e_op2_ch4,
    e_op2_ch20,
    e_op2_ch28,
    e_op2_ch38,
    e_op2_ch42,
    e_op2_ch46,
    e_op2_ch50,
    e_op2_ch52,
    e_op2_ch54,
    e_op2_ch69,
    //optin 3 G1
    e_op3_ch41,
    e_op3_ch61,
    e_op3_ch81,
    e_op3_ch91,
    e_op3_ch97,
    e_op3_ch101,
    e_op3_ch107,
    e_op3_ch121,
    e_op3_ch141,
    e_op3_ch161,
    //optin 3 G2
    e_op3_ch30,
    e_op3_ch50,
    e_op3_ch70,
    e_op3_ch86,
    e_op3_ch112,
    e_op3_ch117,
    e_op3_ch128,
    e_op3_ch135,
    e_op3_ch147,
    e_op3_ch180,
    e_res_chl,
    e_op_end
}RF_OP_CH_e;

typedef struct
{
    U32  option   :2; //无线信道option
    U32  channel  :8;  //无线信道号
    U32  HaveNet  :1;  //当前频点是否存在网络
    U32  LifeTime :16; //信道占用持续时间，CCO信道协商参考。
    U32           :5;  //保留
}__PACKED RF_OPTIN_CH_t;

RF_OPTIN_CH_t scanRfChArry[e_op_end];

#if defined(STATIC_NODE)

void ScanRfInit();


U8 getScanRfTimerState();
void StopScanRfTimer();
void ScanRfChMange(uint8_t event, U32 snid);
void changRfChannel(U8 option, U16 rfchannel);
void rfScanStayCurCh();

void RfScanCallBackNow();

void ChangRfChlReq(work_t *work);

//RF_OPTIN_CH_t scanRfChArry[];

/**
 * @brief Set the rfscan channel object     入网节点，拉黑网络时，需要恢复无线信道到扫频信道
 * 
 */
void set_rfscan_channel();

#endif //#if defined(STATIC_NODE)

#if defined(STATIC_MASTER)
void updateChlHaveNetLeftTime();
#endif //#if defined(STATIC_MASTER)


void setRfChlHaveNetFlag(U8 option, U16 channel);
void changRfChannel(U8 option, U16 rfchannel);


#endif // SCANRFCHMANAGE_H
