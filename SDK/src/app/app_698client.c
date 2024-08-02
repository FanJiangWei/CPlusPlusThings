/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        app_698Taskmanage.c
 * Purpose:     procedure for using
 * History:
 */

#include "ZCsystem.h" 
#include "app_698p45.h"
#include "app_698client.h"



U8   BroadCastAddr[METER_ADDR_LEN] = {0x99,0x99,0x99,0x99,0x99,0x99};
U8   WildcardAddr[METER_ADDR_LEN]  = {0xAA,0xAA,0xAA,0xAA,0xAA,0xAA};
U8 BroadCastAddr_698 = 0xAA;
U8  Dl698SendApdu[100];

U8  FrameSeq = 0;


//OAD_s  MainOAD =
//{
//    0x5004, 0x02, 0x00, 0x00    //OAD
//};
    

OAD_s  Rsd_OAD = 
{
    0x2021, 0x02, 0x00, 0x00,     //OAD
};
    

date_time_s  RsdStartData = {2020, 9, 4, 0, 1, 1};
date_time_s  RsdEndData  = {2020, 9, 25, 0, 1, 1};
date_time_s  RsdStartMinData = {2020, 9, 4, 0, 1, 1};
date_time_s  RsdEndMinData  = {2020, 9, 25, 0, 1, 1};

U8  RsdTiData[3]    = {e_DAY, 0x00, 0x00};
U8  RsdTiMonData[3]    = {e_MONTH, 0x00, 0x01};
U8  RsdTiMinData[3]    = {e_MINUTE, 0x00, 0x0F};    


/*
RCSD_s  TestRCSD = 
{    
    13,
    {
        {0, {0x2021, 0x02, 0x00, 0x00}},   // 20 21 02 00    
        {0, {0x0010, 0x02, 0x00, 0x00}},   // 00 10 02 00 
        {0, {0x0020, 0x02, 0x00, 0x00}},   // 00 20 02 00 
        {0, {0x0030, 0x02, 0x00, 0x00}},   // 00 30 02 00 
        {0, {0x0040, 0x02, 0x00, 0x00}},   // 00 40 02 00 
        {0, {0x0050, 0x02, 0x00, 0x00}},   // 00 50 02 00 
        {0, {0x0060, 0x02, 0x00, 0x00}},   // 00 60 02 00 
        {0, {0x0070, 0x02, 0x00, 0x00}},   // 00 70 02 00 
        {0, {0x0080, 0x02, 0x00, 0x00}},   // 00 80 02 00 
        {0, {0x1010, 0x02, 0x00, 0x00}},   // 10 10 02 00 
        {0, {0x1020, 0x02, 0x00, 0x00}},   // 10 20 02 00 
        {0, {0x1030, 0x02, 0x00, 0x00}},   // 10 30 02 00 
        {0, {0x1040, 0x02, 0x00, 0x00}},   // 10 40 02 00 
    }
};

OAD_LIST_s  USED_OAD[24] =

{
    { e_VOLTAGE_NUM                             , { 0x2000 ,0x02 , 0x00 ,  0x00 } , } ,     //20000200  电压
    { e_ELECTRIC_CURRENT                        , { 0x2001 ,0x02 , 0x00 ,  0x00 } , } ,     //20010200  电流
    { e_ZERO_LINE_CURRENT                       , { 0x2001 ,0x04 , 0x00 ,  0x00 } , } ,     //20010400  零线（零序）电流
    { e_POWER_FACTOR                            , { 0x200A ,0x02 , 0x00 ,  0x00 } , } ,     //200A0200  功率因数
    { e_FREEZING_TIME                           , { 0x2021 ,0x02 , 0x00 ,  0x00 } , } ,     //20210200  冻结时标
                                                      	                             
    { e_POSITIVE_ACTIVE_ELECTRIC_ENERGY         , { 0x0010 ,0x02 , 0x00 ,  0x00 } , } ,     //00100200  正向有功电能
    { e_REVERSE_ACTIVE_ELECTRIC_ENERGY          , { 0x0020 ,0x02 , 0x00 ,  0x00 } , } ,     //00200200  反向有功电能
    { e_TOTAL_ELECTRIC_ENERGY_OF_POSITIVE       , { 0x0010 ,0x02 , 0x00 ,  0x01 } , } ,     //00100201  正向有功总电能
    { e_TOTAL_ELECTRIC_ENERGY_OF_REVERSE        , { 0x0020 ,0x02 , 0x00 ,  0x01 } , } ,     //00200201  反向有功总电能
                                                      	                             
    { e_COMBINED_REACTIVE_1_ELECTRIC_ENERGY     , { 0x0030 ,0x02 , 0x00 ,  0x00 } , } ,     //00300200  组合无功1电能
    { e_COMBINED_REACTIVE_2_ELECTRIC_ENERGY     , { 0x0040 ,0x02 , 0x00 ,  0x00 } , } ,     //00400200  组合无功2电能
                                                      	                             
    { e_FIRST_QUADRANT_REACTIVE_ENERGY          , { 0x0050 ,0x02 , 0x00 ,  0x00 } , } ,     //00500200  第一象限无功电能
    { e_SECOND_QUADRANT_REACTIVE_ENERGY         , { 0x0060 ,0x02 , 0x00 ,  0x00 } , } ,     //00600200  第二象限无功电能
    { e_THIRD_QUADRANT_REACTIVE_ENERGY          , { 0x0070 ,0x02 , 0x00 ,  0x00 } , } ,     //00700200  第三象限无功电能
    { e_FOURTH_QUADRANT_REACTIVE_ENERGY         , { 0x0080 ,0x02 , 0x00 ,  0x00 } , } ,     //00800200  第四象限无功电能
                                                      	                             
    { e_MAX_DEMAND_TIME_OF_POSITIVE_ACTIVE      , { 0x1010 ,0x02 , 0x00 ,  0x00 } , } ,     //10100200  正向有功最大需量及发生时间
    { e_MAX_DEMAND_TIME_OF_REVERSE_ACTIVE       , { 0x1020 ,0x02 , 0x00 ,  0x00 } , } ,     //10200200  反向有功最大需量及发生时间
    { e_MAX_DEMAND_TIME_OF_COMBINED_REACTIVE_I  , { 0x1030 ,0x02 , 0x00 ,  0x00 } , } ,     //10300200  组合无功I最大需量及发生时间
    { e_MAX_DEMAND_TIME_OF_COMBINED_REACTIVE_II , { 0x1040 ,0x02 , 0x00 ,  0x00 } , } ,     //10400200  组合无功II最大需量及发生时间
                                                      	                             
    { e_FIRST_QUADRANT_REACTIVE_TOTAL_ENERGY    , { 0x0050 ,0x02 , 0x00 ,  0x01 } , } ,     //00500201  第一象限无功总电能
    { e_SECOND_QUADRANT_REACTIVE_TOTAL_ENERGY   , { 0x0060 ,0x02 , 0x00 ,  0x01 } , } ,     //00600201  第二象限无功总电能
    { e_THIRD_QUADRANT_REACTIVE_TOTAL_ENERGY    , { 0x0070 ,0x02 , 0x00 ,  0x01 } , } ,     //00700201  第三象限无功总电能
    { e_FOURTH_QUADRANT_REACTIVE_TOTAL_ENERGY   , { 0x0080 ,0x02 , 0x00 ,  0x01 } , } ,     //00800201  第四象限无功总电能

}
;

*/



#if 0
OAD_s   TestRSD_OAD1[8] = 
{
    {0x2021, 0x02, 0x00, 0x00},   // 20 21 02 00    
    {0x0010, 0x02, 0x00, 0x00},   // 00 10 02 00 
    {0x0020, 0x02, 0x00, 0x00},   // 00 20 02 00 
    {0x0030, 0x02, 0x00, 0x00},   // 00 30 02 00 
    {0x0040, 0x02, 0x00, 0x00},   // 00 40 02 00 
    {0x0050, 0x02, 0x00, 0x00},   // 00 50 02 00 
    {0x0060, 0x02, 0x00, 0x00},   // 00 60 02 00 
    {0x0070, 0x02, 0x00, 0x00},   // 00 70 02 00 
};


OAD_s   TestRSD_OAD2[5] =  
{
    {0x0080, 0x02, 0x00, 0x00},   // 00 80 02 00 
    {0x1010, 0x02, 0x00, 0x00},   // 10 10 02 00 
    {0x1020, 0x02, 0x00, 0x00},   // 10 20 02 00 
    {0x1030, 0x02, 0x00, 0x00},   // 10 30 02 00 
    {0x1040, 0x02, 0x00, 0x00},   // 10 40 02 00 
};

    
OAD_s   TestOAD[1] = 
{
    {0xf101, 0x02, 0x00, 0x00},   // 20 00 02 00    
    //{0x2001, 0x02, 0x00, 0x00},   // 20 01 02 00 
    //{0x2001, 0x04, 0x00, 0x00},   // 20 01 04 00 
    //{0x200A, 0x02, 0x00, 0x00},   // 20 0A 02 00 
};


OAD_s   Minute15CurveOAD[6] = 
{
    {0x0010, 0x02, 0x00, 0x01},   // 00 10 02 01    
    {0x0020, 0x02, 0x00, 0x01},   // 00 20 02 01 
    {0x0050, 0x02, 0x00, 0x01},   // 00 50 02 01 
    {0x0060, 0x02, 0x00, 0x01},   // 00 60 02 01 
    {0x0070, 0x02, 0x00, 0x01},   // 00 70 02 01 
    {0x0080, 0x02, 0x00, 0x01},   // 00 80 02 01 
    
};
#endif


cjq_minute_cllct_s   CjqMinuteCllctData[MAX_MINUTE_CLLCT_POINT];


day_freeze_s   DayFreezeData[MAX_DAY_FREEZE_POINT][CJQ_MAX_METER];
day_freeze_s   MonthFreezeData[MAX_MONTH_FREEZE_POINT][CJQ_MAX_METER];

quarter_curve_s  QuarterCurve[MAX_QUARTER_CURVE_POINT][CJQ_MAX_METER];



Report_Ctrl_s ReportCtrlMin;

Report_Ctrl_s ReportCtrlDay;






void Packet698Header(U8 *pServerAddr, U8 *pSendbuff, U16 *pSendLen, U8 prm, U8 dir)
{
    U16                  sendLen = 0;
    dl698frame_header_s  *pSendFrameHeader = (dl698frame_header_s*)&pSendbuff[sendLen];

    // 帧头部分
    pSendFrameHeader->StartChar = 0x68;
    pSendFrameHeader->Res       = 0;

    pSendFrameHeader->CtrlField.PrmBit = prm;
    pSendFrameHeader->CtrlField.DirBit = dir;
            
    pSendFrameHeader->CtrlField.FuncCode = 0x03;
    pSendFrameHeader->CtrlField.Res = 0;
    pSendFrameHeader->CtrlField.FrgmntFlg = 0;
    
    // 广播地址的地址长度为1，逻辑地址为0，地址内容为AA
    pSendFrameHeader->SA.AddrLen = (*pServerAddr == BroadCastAddr_698) ? 0 : (METER_ADDR_LEN - 1);
    pSendFrameHeader->SA.LogicAddr = 0;

    pSendFrameHeader->SA.AddrType = (*pServerAddr == BroadCastAddr_698) ? 3 
                                    : (0 == memcmp(pServerAddr, WildcardAddr, METER_ADDR_LEN)) ? 1
                                    : 0;                                       
        
    sendLen += sizeof(dl698frame_header_s);
    if (*pServerAddr == BroadCastAddr_698)
    {
        __memcpy(pSendFrameHeader->SA.Addr, pServerAddr, pSendFrameHeader->SA.AddrLen + 1);
        sendLen += pSendFrameHeader->SA.AddrLen + 1;
    }
    else
    {
        __memcpy(pSendFrameHeader->SA.Addr, pServerAddr, METER_ADDR_LEN);
        sendLen += METER_ADDR_LEN;
    }

    pSendbuff[sendLen++] = CLIENT_ADDR;

    *pSendLen = sendLen;
}
    

//void SendGetReqeustNormal(U8 *pServerAddr, OAD_s Oad, U8 *pSendbuff, U16 *pSendLen)
//{
//    PIID_s   piid;
//    U16      apduLen = 0;

//    Packet698Header(pServerAddr, pSendbuff, pSendLen, 1, 0);

//    piid.serviceSeq = ++FrameSeq;
//    piid.res        = 0;
//    piid.servicePri = 0;

//    Dl698SendApdu[apduLen++] = (U8)(Oad.OI >> 8);
//    Dl698SendApdu[apduLen++] = (U8)(Oad.OI & 0xFF);
//    Dl698SendApdu[apduLen++] = Oad.AttriId | (Oad.AttriFeature << 5);
//    Dl698SendApdu[apduLen++] = Oad.AttriElementIdx;

//    Dl698SendApdu[apduLen++] = 0x00;   // 无时间标签
    
//    Dl698PacketFrame(GET_REQUEST, 0x01, piid, 0, TRUE, Dl698SendApdu, apduLen, pSendbuff, pSendLen);
        
//    return;
//}


//void SendGetRequestNormalList(U8 *pServerAddr, U8 listNum, OAD_s *pOad, U8 *pSendbuff, U16 *pSendLen)
//{
//    PIID_s   piid;
//    U16      apduLen = 0;
//    U8       ii;
    
//    Packet698Header(pServerAddr, pSendbuff, pSendLen, 1, 0);

//    piid.serviceSeq = ++FrameSeq;
//    piid.res        = 0;
//    piid.servicePri = 0;

//    for(ii=0; ii<listNum; ii++)
//    {
//        Dl698SendApdu[apduLen++] = (U8)(pOad[ii].OI >> 8);
//        Dl698SendApdu[apduLen++] = (U8)(pOad[ii].OI & 0xFF);
//        Dl698SendApdu[apduLen++] = pOad[ii].AttriId | (pOad[ii].AttriFeature << 5);
//        Dl698SendApdu[apduLen++] = pOad[ii].AttriElementIdx;
//    }

//    Dl698SendApdu[apduLen++] = 0x00;   // 无时间标签
    
//    Dl698PacketFrame(GET_REQUEST, 0x02, piid, listNum, TRUE, Dl698SendApdu, apduLen, pSendbuff, pSendLen);
        
//    return;
//}

//void SendGetRequestRecord(U8 *pServerAddr, OAD_s *pOad, RSD_s *pRsd, RCSD_s *pRcsd, U8 *pSendbuff, U16 *pSendLen)
//{
//    PIID_s   piid;
//    U16      apduLen = 0;
//    U8       ii;
    
//    Packet698Header(pServerAddr, pSendbuff, pSendLen, 1, 0);

//    piid.serviceSeq = ++FrameSeq;
//    piid.res        = 0;
//    piid.servicePri = 0;

//    // OAD
//    Dl698SendApdu[apduLen++] = (U8)(pOad->OI >> 8);
//    Dl698SendApdu[apduLen++] = (U8)(pOad->OI & 0xFF);
//    Dl698SendApdu[apduLen++] = pOad->AttriId | (pOad->AttriFeature << 5);
//    Dl698SendApdu[apduLen++] = pOad->AttriElementIdx;

//    // RSD
//    apduLen += DLT698ConstructData_RSD(e_NORMAL_RSD,&Dl698SendApdu[apduLen], pRsd);

//    // Rcsd
//    Dl698SendApdu[apduLen++] = pRcsd->num;
//    for(ii=0; ii<pRcsd->num; ii++)
//    {
//        Dl698SendApdu[apduLen++] = pRcsd->csd[ii].choice;
//        if(pRcsd->csd[ii].choice == 0)
//        {
//            // OAD
//            Dl698SendApdu[apduLen++] = (U8)(pRcsd->csd[ii].unioncsd.oad.OI >> 8);
//            Dl698SendApdu[apduLen++] = (U8)(pRcsd->csd[ii].unioncsd.oad.OI & 0xFF);
//            Dl698SendApdu[apduLen++] = pRcsd->csd[ii].unioncsd.oad.AttriId | (pRcsd->csd[ii].unioncsd.oad.AttriFeature << 5);
//            Dl698SendApdu[apduLen++] = pRcsd->csd[ii].unioncsd.oad.AttriElementIdx;
//            //apduLen += DLT698ConstructData(&Dl698SendApdu[apduLen], CSD, 0, (void*)(&pRcsd->csd[ii].unioncsd.oad ), 0);
//        }
//        else if(pRcsd->csd[ii].choice == 1)
//        {

//        }
//    }

//    Dl698SendApdu[apduLen++] = 0x00;   // 无时间标签

//    Dl698PacketFrame(GET_REQUEST, 0x03, piid, 0, TRUE, Dl698SendApdu, apduLen, pSendbuff, pSendLen);
    
//    return;
//}

//void SendGetRequestRecordList(U8 *pServerAddr, U8 listNum, OAD_s *pOad, RSD_s *pRsd, RCSD_s *pRcsd, U8 *pSendbuff, U16 *pSendLen)
//{
//    PIID_s   piid;
//    U16      apduLen = 0;
//    U8       ii;
    
//    Packet698Header(pServerAddr, pSendbuff, pSendLen, 1, 0);

//    piid.serviceSeq = ++FrameSeq;
//    piid.res        = 0;
//    piid.servicePri = 0;

//    for(ii=0; ii<listNum; ii++)
//    {
//        // OAD
//        Dl698SendApdu[apduLen++] = (U8)(pOad->OI >> 8);
//        Dl698SendApdu[apduLen++] = (U8)(pOad->OI & 0xFF);
//        Dl698SendApdu[apduLen++] = pOad->AttriId | (pOad->AttriFeature << 5);
//        Dl698SendApdu[apduLen++] = pOad->AttriElementIdx;
    
//        // RSD
//        apduLen += DLT698ConstructData_RSD(e_NORMAL_RSD,&Dl698SendApdu[apduLen], pRsd);
    
//        // Rcsd
//        Dl698SendApdu[apduLen++] = pRcsd->num;
//        for(ii=0; ii<pRcsd->num; ii++)
//        {
//            apduLen += DLT698ConstructData(&Dl698SendApdu[apduLen], CSD, 0, (void*)(&pRcsd->csd[ii]), 0);
//        }

//    }

//    Dl698SendApdu[apduLen++] = 0x00;   // 无时间标签

//    Dl698PacketFrame(GET_REQUEST, 0x04, piid, listNum, TRUE, Dl698SendApdu, apduLen, pSendbuff, pSendLen);
//}

void SendActionRequestNormal(U8 *pServerAddr,OMD_s *pOmd,OAD_s *pOad, CMMDCB_s *pComdcb, U8 *pSendbuff, U16 *pSendLen)
{
    PIID_s   piid;
    U16      apduLen = 0;

    Packet698Header(pServerAddr, pSendbuff, pSendLen, 1, 0);

    piid.serviceSeq = ++FrameSeq;
    piid.res        = 0;
    piid.servicePri = 0;
    // OMD
    Dl698SendApdu[apduLen++] = (U8)(pOmd->OI >> 8);
    Dl698SendApdu[apduLen++] = (U8)(pOmd->OI & 0xFF);
    Dl698SendApdu[apduLen++] = pOmd->MethodId;
    Dl698SendApdu[apduLen++] = pOmd->OperateMode;

    // structure
    apduLen += DLT698ConstructData(&Dl698SendApdu[apduLen], STRUCTURE, 1, 0, 2),
    //OAD
    apduLen += DLT698ConstructData(&Dl698SendApdu[apduLen], OAD, 0, (void*)(pOad), 0),
    //COMDCB
    apduLen += DLT698ConstructData(&Dl698SendApdu[apduLen], COMDCB, 0, (void*)(pComdcb), 0),
    
    Dl698SendApdu[apduLen++] = 0x00;   // 无时间标签
    
    Dl698PacketFrame(ACTION_REQUEST, 0x01, piid, 0, FALSE, Dl698SendApdu, apduLen, pSendbuff, pSendLen);

    return;
}


