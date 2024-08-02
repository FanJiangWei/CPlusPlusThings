//注意：宏定义后不能跟注释，否则会导致脚本运行异常
// #define ex_info_crc16_1 0x34 由脚本计算
// #define ex_info_crc16_2 0x1e

//暂时只扩展了49个字节(除crc16)剩余保留
#define ex_info_len_h 0x00
#define ex_info_len_l 0x31

#define water_mark_1 0xdd
#define water_mark_2 0xcc
#define water_mark_3 0xbb
#define water_mark_4 0xaa

//省份代码（BCD格式）
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
// #define date_month 0x12//脚本计算
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
// #define out_date_month 0x08//脚本计算
// #define out_date_year 0x22

//是否启用应用功能开关   
#define function_switch_usemode 0x01 
//调试模式开关          
#define DebugeMode          0x00
//噪声检测开关			 
#define NoiseDetectSWC      0x00
//白名单开关			 
#define WhitelistSWC        0x01
//升级模式开关			 
#define UpgradeMode         0x00
//AODV开关			 
#define AODVSWC             0x00
//事件上报开关			 
#define EventReportSWC      0x01
//模块ID获取开关			 
#define ModuleIDGetSWC      0x01
//相位识别开关			 
#define PhaseSWC            0x01
//北京台区识别默认开启开关			 
#define IndentifySWC        0x00
//抄表上行帧地址过滤开关			 
#define DataAddrFilterSWC   0x00
//离网感知开关			 
#define NetSenseSWC         0x00
//376.2上行信息域信号品质开关			 
#define SignalQualitySWC    0x00
//376.2设置频段开关			 
#define SetBandSWC          0x01
//切相开关			 
#define SwPhase             0x01
//20版电表事件上报开关，1表示以20版电表模式上报，0表示以读状态字模式上报			 
#define oop20EvetSWC		0x01
//20版电表波特率协商开关，1表示启用，0表示禁止			 
#define oop20BaudSWC		0x00

//集中器配置串口波特率开关，1是启用，从配置去读取，0是禁止，按照默认
#define JZQBaudSWC			0x00
//分钟采集开关 江苏打开|其他省份关闭
#define MinCollectSWC       0x00
//ID 信息获取开关，送检和量产获取的地方不同，默认为0表示量产，1表示送检
#define IDInfoGetModeSWC			0x00
//传输模式控制字开关,1为双模,0为单载波
#define TransModeSWC		0x01
// 地址自动切换切换开关：1开启，0关闭
#define AddrShiftSWC		0x01

//6字节保留由脚本添加
#define function_switch_used 0x5A		
// #define function_switch_cs 由脚本计算

//使用标志
#define  param_cfg_usemode 0x01
//并发抄表并发数              
#define  ConcurrencySize 0x17
//重抄次数		
#define  ReTranmitMaxNum 0x05
//重抄时间		
#define  ReTranmitMaxTime 0x50
//广播抄读条数     
#define  BroadcastConRMNum 0x10 
//广播抄读类型
#define  AllBroadcastType  0x03

#define  JZQ_baud_cfg 0x00

//13字节保留位由脚本添加
#define  param_cfg_used 0x5A
// #define param_cfg_cs 由脚本计算