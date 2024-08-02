

#ifndef __SCANBANDMANAGE_H__
#define __SCANBANDMANAGE_H__
#include "types.h"
#include "mbuf.h"

typedef enum
{
    e_INIT,//�ϵ�
    e_LEAVE,//����
	e_RECEIVE,//�յ�����
	e_TEST,//�������̬
	e_CNTCKQ,//���ӵ�������
	e_ONLINEEVENT,//����״̬
} SCANEVENT_e;


#define     FIRST_SCAN_TIME  30
#define     MAX_SCAN_TIME  10
#define     MAX_STAY_TIME  600

void changepower(U8 band);
void changeband(U8 band);

void  ScanBandManage(uint8_t event, U32 snid);
U8 GetScanBandStatus();
void changeBandReq(struct work_s *work);
U32 GetRunTimerLeftTime(ostimer_t *timer);
U32 GetScanTiemrLeftTime();
void SetTgain(int16_t dig, int16_t ana);
//�����л�Ƶ��
void ScanBandCallBackNow();
void stopScanBandTimer();

ostimer_t  ScanBand_timer;
void modify_scanband_timer(uint32_t first);

#endif

