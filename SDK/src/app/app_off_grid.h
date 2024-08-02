#ifndef __APP_OFF_GRID_H__
#define __APP_OFF_GRID_H__

#if defined(STATIC_MASTER)

/*�������������������������������������������������������������������ⲿ���á�����������������������������������������������������������*/
extern U8  Gw3762TopUpReportData[512];


/*�������������������������������������������������������������������궨�������������������������������������������������������������*/


#define MAX_CHANGE_STATE_NUM              10//����ɽ��������֪
#define MAX_CHANGE_STATE_LIST_NUM         40//������


/*������������������������������������������������������������������ö�١�����������������������������������������������������������*/
typedef enum{
	e_OFF_GRID_REPORT = 0,
	e_OFF_GRID_NOREPROT = 1,
}REPORT_STATE_e;


typedef enum{
	e_OFF_2_ONLINE,
	e_ON_2_OFFILNE,
}STATE_CHANGE_e;


typedef enum{
	e_UNKOWN,
	e_POWER_OFF,
	e_CHANNEL_CHANGE,
	e_RESV,
}OFFLINE_REASON_e;


/*�������������������������������������������������������������������ṹ�������������������������������������������������������������*/

//������֪�ڵ���Ϣ
typedef struct NODE_STATE_CHANGE_INFO{
	U8  addr[6];			//�ڵ��ַ
	U8  changestate;		//�ڵ�������������
	U8  outlinereason;		//����ԭ��
	
	U32 outlinetime;		//����ʱ��

	U8  chip_id[24];
}__PACKED  NODE_STATE_CHANGE_INFO_t;


//������֪����ͷ
typedef struct OFF_GRID_INFO_HEAD{
	list_head_t link;
	
	uint16_t nr;
	uint16_t thr;
}__PACKED OFF_GRID_LIST_HEADER_t;


//������֪
typedef struct OFF_GRID_INFO{
	list_head_t link;
    NODE_STATE_CHANGE_INFO_t node_info_t;	//�ڵ���Ϣ
    U8 state;			//�ϱ�״̬
	U8 res[3];
}__PACKED OFF_GRID_LIST_t; 


/*�������������������������������������������������������������������ӿں���������������������������������������������������������������*/

void off_grid_list_init(void);

void off_grid_off_to_on(DEVICE_TEI_LIST_t *arg_dev_t);

void off_grid_on_to_off(DEVICE_TEI_LIST_t *arg_dev_t);

void off_grid_up_data(DEVICE_TEI_LIST_t *arg_dev_t);

void off_grid_set_reset_reason(U8 arg_power_on_off, U16 seq);

#endif

#endif

