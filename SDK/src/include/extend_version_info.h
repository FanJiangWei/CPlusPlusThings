//ע�⣺�궨����ܸ�ע�ͣ�����ᵼ�½ű������쳣
// #define ex_info_crc16_1 0x34 �ɽű�����
// #define ex_info_crc16_2 0x1e

//��ʱֻ��չ��49���ֽ�(��crc16)ʣ�ౣ��
#define ex_info_len_h 0x00
#define ex_info_len_l 0x31

#define water_mark_1 0xdd
#define water_mark_2 0xcc
#define water_mark_3 0xbb
#define water_mark_4 0xaa

//ʡ�ݴ��루BCD��ʽ��
#define province_code 0x00

#define sta_3750_version_l 0x30
#define sta_3750_version_h 0x40

#define sta_3750A_version_l 0x30
#define sta_3750A_version_h 0x50

#define sta_riscv_version_l 0x09
#define sta_riscv_version_h 0x00

#define sta_venus_version_l 0x09
#define sta_venus_version_h 0x10
				
#define sta_233l_version_l 0x11
#define sta_233l_version_h 0x10

#define cjq2_3750_version_l 0x25
#define cjq2_3750_version_h 0x40

#define cjq2_3750A_version_l 0x30
#define cjq2_3750A_version_h 0x50

#define cjq2_venus_version_l 0x09
#define cjq2_venus_version_h 0x10

#define cjq2_riscv_version_l 0x09
#define cjq2_riscv_version_h 0x00

#define cjq2_233l_version_l 0x11
#define cjq2_233l_version_h 0x10

#define cco_3951_version_l 0x25
#define cco_3951_version_h 0x40

#define cco_3951A_version_l 0x30
#define cco_3951A_version_h 0x50

#define cco_venus_version_l 0x09
#define cco_venus_version_h 0x10

#define cco_233l_version_l 0x11
#define cco_233l_version_h 0x10

#define cco_riscv_version_l 0x11
#define cco_riscv_version_h 0x10

// #define date_day 0x05
// #define date_month 0x12//�ű�����
// #define date_year 0x22

#define sta_3750_out_version_l 0x00
#define sta_3750_out_version_h 0x10

#define sta_3750A_out_version_l 0x00
#define sta_3750A_out_version_h 0x10

#define sta_riscv_out_version_l 0x00
#define sta_riscv_out_version_h 0x10

#define sta_venus_out_version_l 0x00
#define sta_venus_out_version_h 0x10
				
#define sta_233l_out_version_l 0x00
#define sta_233l_out_version_h 0x10

#define cjq2_3750_out_version_l 0x00
#define cjq2_3750_out_version_h 0x10

#define cjq2_3750A_out_version_l 0x00
#define cjq2_3750A_out_version_h 0x10

#define cjq2_venus_out_version_l 0x00
#define cjq2_venus_out_version_h 0x10

#define cjq2_riscv_out_version_l 0x00
#define cjq2_riscv_out_version_h 0x10

#define cjq2_233l_out_version_l 0x00
#define cjq2_233l_out_version_h 0x10

#define cco_3951_out_version_l 0x00
#define cco_3951_out_version_h 0x10

#define cco_3951A_out_version_l 0x00
#define cco_3951A_out_version_h 0x50

#define cco_venus_out_version_l 0x00
#define cco_venus_out_version_h 0x10

#define cco_233l_out_version_l 0x00
#define cco_233l_out_version_h 0x10

#define cco_riscv_out_version_l 0x11
#define cco_riscv_out_version_h 0x10

// #define out_date_day 14
// #define out_date_month 0x08//�ű�����
// #define out_date_year 0x22

//�Ƿ�����Ӧ�ù��ܿ���   
#define function_switch_usemode 0x01 
//����ģʽ����          
#define DebugeMode          0x00
//������⿪��			 
#define NoiseDetectSWC      0x00
//����������			 
#define WhitelistSWC        0x01
//����ģʽ����			 
#define UpgradeMode         0x00
//AODV����			 
#define AODVSWC             0x00
//�¼��ϱ�����			 
#define EventReportSWC      0x01
//ģ��ID��ȡ����			 
#define ModuleIDGetSWC      0x01
//��λʶ�𿪹�			 
#define PhaseSWC            0x01
//����̨��ʶ��Ĭ�Ͽ�������			 
#define IndentifySWC        0x00
//��������֡��ַ���˿���			 
#define DataAddrFilterSWC   0x00
//������֪����			 
#define NetSenseSWC         0x00
//376.2������Ϣ���ź�Ʒ�ʿ���			 
#define SignalQualitySWC    0x00
//376.2����Ƶ�ο���			 
#define SetBandSWC          0x01
//���࿪��			 
#define SwPhase             0x01
//20�����¼��ϱ����أ�1��ʾ��20����ģʽ�ϱ���0��ʾ�Զ�״̬��ģʽ�ϱ�			 
#define oop20EvetSWC		0x01
//20��������Э�̿��أ�1��ʾ���ã�0��ʾ��ֹ			 
#define oop20BaudSWC		0x00

//���������ô��ڲ����ʿ��أ�1�����ã�������ȥ��ȡ��0�ǽ�ֹ������Ĭ��
#define JZQBaudSWC			0x00
//���Ӳɼ����� ���մ�|����ʡ�ݹر�
#define MinCollectSWC       0x00
//ID ��Ϣ��ȡ���أ��ͼ��������ȡ�ĵط���ͬ��Ĭ��Ϊ0��ʾ������1��ʾ�ͼ�
#define IDInfoGetModeSWC			0x00
//����ģʽ�����ֿ���,1Ϊ˫ģ,0Ϊ���ز�
#define TransModeSWC		0x01
// ��ַ�Զ��л��л����أ�1������0�ر�
#define AddrShiftSWC		0x01

//6�ֽڱ����ɽű����
#define function_switch_used 0x5A		
// #define function_switch_cs �ɽű�����

//ʹ�ñ�־
#define  param_cfg_usemode 0x01
//������������              
#define  ConcurrencySize 0x17
//�س�����		
#define  ReTranmitMaxNum 0x05
//�س�ʱ��		
#define  ReTranmitMaxTime 0x50
//�㲥��������     
#define  BroadcastConRMNum 0x10 
//�㲥��������
#define  AllBroadcastType  0x03

#define  JZQ_baud_cfg 0x00

//13�ֽڱ���λ�ɽű����
#define  param_cfg_used 0x5A
// #define param_cfg_cs �ɽű�����