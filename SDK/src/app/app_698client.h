/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        app_698p45.h
 * Purpose:     procedure for using
 * History:
 */


#ifndef __APP_698CLIENT_H__
#define __APP_698CLIENT_H__
#include "app_698p45.h"


#if defined(ZC3750CJQ2)
#define   CJQ_MAX_METER              1//12
#else
#define   CJQ_MAX_METER              1
#endif

#define   MAX_MINUTE_CLLCT_POINT     1//90
#define   MAX_QUARTER_CURVE_POINT    1//48

#define   MAX_DAY_FREEZE_POINT       1
#define   MAX_MONTH_FREEZE_POINT     1

#define   DAY_FREEZE_DATA_LENG       1//400
#define   CURVE_DATA_LENG            1//30


//extern OAD_s   MainOAD;
//extern OAD_s   TestOAD[1];

extern OAD_s   Rsd_OAD;
extern date_time_s  RsdStartData;
extern date_time_s  RsdEndData;
extern date_time_s  RsdStartMinData;
extern date_time_s  RsdEndMinData;

extern U8      RsdTiData[3];
extern U8      RsdTiMinData[3];
extern U8  RsdTiMonData[3];

//extern OAD_s   TestRSD_OAD1[8];
//extern OAD_s   TestRSD_OAD2[5];


//extern OAD_LIST_s  USED_OAD[24];



typedef struct{
    U16   Voltage[3];   // 6 bytes
    S32   Current[3];   // 12 bytes
    S32   ZeroCurrent;  // 4 bytes
    U16   Factor[4];    // 8 bytes
}__PACKED minute_cllct_data_s;

typedef struct{
    date_time_s         CllctTime;
    U8                  CjqMinuteData[CJQ_MAX_METER][30];
}__PACKED cjq_minute_cllct_s;

typedef struct{
    S32           MaxDemand;
    date_time_s   HappenTime;
}__PACKED demand_time_s;


typedef struct{
    date_time_s    FreezeTime;

    U8   FreezeData[DAY_FREEZE_DATA_LENG];

}__PACKED day_freeze_s;



typedef struct{
    date_time_s    FreezeTime;

    U8   CurveData[CURVE_DATA_LENG];

}__PACKED quarter_curve_s;

typedef struct{
    //queue_ctrl_s    *QueueReport;
    U8              PointNum;
    U8              ReportFlag;
}__PACKED Report_Ctrl_s;

extern Report_Ctrl_s ReportCtrlMin;
extern Report_Ctrl_s ReportCtrlDay;
extern U8  FrameSeq;
extern U8  Dl698SendApdu[100];

void Packet698Header(U8 *pServerAddr, U8 *pSendbuff, U16 *pSendLen, U8 prm, U8 dir);

extern cjq_minute_cllct_s   CjqMinuteCllctData[MAX_MINUTE_CLLCT_POINT];





//void SendGetReqeustNormal(U8 *pServerAddr, OAD_s Oad, U8 *pSendbuff, U16 *pSendLen);

//void SendGetRequestNormalList(U8 *pServerAddr, U8 listNum, OAD_s *pOad, U8 *pSendbuff, U16 *pSendLen);

//void SendGetRequestRecord(U8 *pServerAddr, OAD_s *pOad, RSD_s *pRsd, RCSD_s *pRcsd, U8 *pSendbuff, U16 *pSendLen);

//void SendGetRequestRecordList(U8 *pServerAddr, U8 listNum, OAD_s *pOad, RSD_s *pRsd, RCSD_s *pRcsd,
//                                                                                        U8 *pSendbuff, U16 *pSendLen);

void SendActionRequestNormal(U8 *pServerAddr,OMD_s *pOmd,OAD_s *pOad, CMMDCB_s *pComdcb, U8 *pSendbuff, U16 *pSendLen);






#endif
