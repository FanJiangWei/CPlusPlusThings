
#ifndef __DATALINK_LAYER_H__
#define __DATALINK_LAYER_H__
#include "sys.h"
#include "mbuf.h"
#define NET_PANIC()		sys_panic("[%s:%d]<NET panic> Error!!!\r\n", __func__, __LINE__)

#define DISPLAYNUM	20

U8 StartSendCenterBeaconFlag;


typedef struct
{
   U16  cnt;
   U8   version[2];
}__PACKED VERSION_t;


typedef struct
{
  U8 errflag:4;
  U8 ramErr:4;
  U8 fmtSeq;
  U8 taskid;
  U8 msgtype;

  U32 bcn;

  
}nniberr_t;
extern nniberr_t nniberr;


extern event_t task_work_mutex;

void DatalinkTaskInit();
void post_datalink_task_work(work_t *work);

void modify_sendcenterbeacon_timer(uint32_t first);



extern S8 check_version(U8  *ManufactorCode, U8 *check_ver,U8 * check_innerver , VERSION_t *version , U8 cnt);



#endif

