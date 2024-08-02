#ifndef __APP_READ_CJQ_LIST_STA_H__
#define __APP_READ_CJQ_LIST_STA_H__

#include "trios.h"
#include "types.h"
#include "app_meter_bind_sta.h"

#if defined(ZC3750STA)
#define TH2CJQ
#ifdef TH2CJQ

//�����������������������������������������������궨���������������������������������������������


#define  TH2CJQ_METER_MAX           32




//����������������������������������������������ö�١�������������������������������������������

typedef enum{
	e_IOT_C_MODE_CJQ,		//�ɼ���ģʽ
	e_IOT_C_MODE_STA,		//���ģʽ
}__PACKED iot_c_ModeType_e;

//�����������������������������������������������ṹ���������������������������������������������

typedef struct
{
	U8 MeterAddr[6];	//����ַ
	U8 Protocol;		//��Լ 0ȱʡ 1:97 2:07 3 698
	
}__PACKED  TH2_METER_INFO;

typedef struct
{
	TH2_METER_INFO  MeterInfo[TH2CJQ_METER_MAX];	//����ַ
	U8                 MeterNum;
	

}__PACKED  TH2_METER_INFO_ST;


//����������������������������������������������������������������������������������������������

U8 TH2CJQ_CheckMeterExistByAddr(U8* Addr, U8* Protocol);
void TH2CJQ_Extend_ShowInfo();
void ReadTH2MeterListPut(U8* Addr, U8 Protocol, U8 Retry);
void TH2MeterChang(void);
void change_network_access_address();
void record_ltu_address(U8 *meter_address, U8 protocol);
void Modify_ReadTH2Metertimer(U32 Sec);
extern TH2_METER_INFO_ST  TH2CJQ_MeterList;
extern METER_COMU_INFO_t ReadTH2MeterInfo;
extern U8 change_address_flag;

#endif

#endif
#endif