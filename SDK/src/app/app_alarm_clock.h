#ifndef __APP_ALARM_CLOCK_H__
#define __APP_ALARM_CLOCK_H__

//#include <time.h>

//#if defined(STATIC_NODE)
#define ALARM_MINTIMEOUT	40
#define ALARM_FREEZETIMEOUT	5*60


typedef enum{
	e_ALARM_STATE_EXTING,
	e_ALARM_STATE_DELETE,
}ALARM_STATE_e;


typedef struct{
    list_head_t link;

    uint16_t nr;    // 当前链表任务数量
    uint16_t thr;   // 管理任务数量最高门限值，标准定义为100
}alarm_clock_list_header_t;

typedef enum{
    e_CLOCK_1MIN     = 0x01,
    e_CLOCK_15MIN    = 0x02,
    e_CLOCK_1DAY     = 0x04,
    e_CLOCK_1MON     = 0x08,
    e_CCO_ClOCK      = 0x10,
    e_CLOCK_STATUS   = 0x12,
}ALARM_CLOCK_TYPE_e;
	
typedef struct alarm_clock_s{
	list_head_t link;
	
	sys_time_t Tm;        //闹钟时间
	
	int32_t (*enq_check)(alarm_clock_list_header_t *list, struct alarm_clock_s *new_list);
	void (*exe_func)(struct alarm_clock_s *pAlarmClock);

	U8      type;
	U8		state;
	U8		res[2];

	uint8_t  payload[0];
}alarm_clock_t;


typedef struct{
    alarm_clock_list_header_t  *list_header;
    ostimer_t              *ostimer;
}alarm_clock_to_app_t;

extern ostimer_t *alarm_clock_timer;

uint32_t alarm_clock_init(uint32_t CurrSec, uint32_t interval_min,U8 clocktype);
void  alarm_clock_reload(uint32_t CurrSec, uint32_t interval_sec,sys_time_t *Tm);

uint16_t get_alarm_clock_list_type_num(U8 type);
uint16_t get_alarm_clock_list_nr(void);
void del_alarm_clock_list_by_type(U8 type);
void del_alarm_clock_list_by_payload(U8 *payload, U8 len);
void del_alarm_clock_list(work_t *work);
void query_alarm_clock_list(work_t *work);
void add_alarm_clock_list(sys_time_t *pNextTime, U8 type, void *exefunc, uint8_t *payload, uint16_t len);
void modify_alarm_clock_time(uint32_t first);
int8_t alarm_clock_timer_init(void);
uint16_t get_alarm_clock_list_type_Info(U8 type,sys_time_t *pTm);


//#endif
#endif

