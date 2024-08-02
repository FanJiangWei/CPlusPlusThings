/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        app_698p45.c
 * Purpose:     procedure for using
 * History:
 */

#include "ZCsystem.h" 
#include "app_698p45.h"
#include "ZCsystem.h"
#include "app_698client.h"
#include "app_dltpro.h"
#include "printf_zc.h"

U8  Dl698RetApdu[500];
extern const U16 fcstab[];
U8  RN_DATA[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

U8      upReportFlag;
U8      timeTag;

static U8 DLT698ParseData(U8 *pRxApdu, void *pData, DLT698_data_s *pParseData);
static U8 DLT698Set698data_CSD(U8* pRxApdu, CSD_s* pCsd, U16* pProcLen);
static U16 DLT698GetDataLen(U8* pRxApdu);
static void Swap(U8* str, int len);
static U16 DLT698ConstructSimplifyData(U8* pTxApdu, data_type_e datatype, U16 datalen, void* pElement, U8 dataNum);

static void ProcGetRequestNormal(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcGetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcGetRequestRecord(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcGetRequestRecordList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcGetRequestNext(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcGetRequestMD5(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcSetRequestNormal(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcSetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcSetThenGetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcActionRequest(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcActionRequestList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcActionThenGetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcReportResponseRecordList(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static void ProcReportResponseSimplifyRecord(U8 *pApdu, U16 ApduLen, U8 *recivebuf, U16 *reciveLen);
static U8 ProcGetResponseNormal(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcGetResponseNormalList(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcGetResponseRecord(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcGetResponseRecordList(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcGetResponseNext(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcGetResponseMD5(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcGetActionRequestNormal(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcReportNotificationList(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcReportNotificationRecordList(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcSecurityResponsePlainText(U8 *pApdu, U16 ApduLen, U8 *p698Data);
static U8 ProcSecurityResponseCipherText(U8 *pApdu, U16 ApduLen, U8 *p698Data);


DLT698_SERVICE_FUNC   ClientServiceFuncArray[] =
{
	{0x0501,  ProcGetRequestNormal},
	{0x0502,  ProcGetRequestNormalList},
	{0x0503,  ProcGetRequestRecord},
	{0x0504,  ProcGetRequestRecordList},
	{0x0505,  ProcGetRequestNext},
	{0x0506,  ProcGetRequestMD5},
	{0x0601,  ProcSetRequestNormal},
	{0x0602,  ProcSetRequestNormalList},
	{0x0603,  ProcSetThenGetRequestNormalList},
	{0x0701,  ProcActionRequest},
	{0x0702,  ProcActionRequestList},
	{0x0703,  ProcActionThenGetRequestNormalList},
	{0x0802,  ProcReportResponseRecordList},
	{0x0806,  ProcReportResponseSimplifyRecord},
};

DLT698_RESPONSE_PROC_FUNC  ResponseProcFuncArray[9] =
{
	{0x8501,  ProcGetResponseNormal},
	{0x8502,  ProcGetResponseNormalList},
	{0x8503,  ProcGetResponseRecord},
	{0x8504,  ProcGetResponseRecordList},
	{0x8505,  ProcGetResponseNext},
	{0x8506,  ProcGetResponseMD5},
	{0x8701,  ProcGetActionRequestNormal},
	{0x9000,  ProcSecurityResponsePlainText},
	{0x9001,  ProcSecurityResponseCipherText},
};

DLT698_RESPONSE_PROC_FUNC  ServiceRpeortProcFuncArray[] =
{
	{0x8801,  ProcReportNotificationList},
	{0x8802,  ProcReportNotificationRecordList},
};

DLT698_OI_GET_FUNC       ClientGetOIFuncArray[] =
{
	/*
		{0x6012,  GetTaskCfgInfo},         // 查询任务配置信息
		{0x6014,  GetCollectPlanCfg},      // 查询采集方案配置

	#if defined(ZC3750CJQ2)
		{0x6032, GetCollectStatus},        // 查询采集状态集
		{0x6034, GetCollectTaskState},     // 查询采集任务监控集
		{0x6043, GetCjqAttribute},         // 查询采集器属性
	#endif

		{0x310A, GetDeviceFailureRecord},  // 查询设备故障记录
		*/
};

/*
U16 ResetCRC16(void)
{
    return 0xffff ;
}


U16 CalCRC16(U16 CRC16Value,U8 *data ,int StartPos, int len)
{
	int i = 0;
    
    for (i = StartPos; i < (len+StartPos); i++)
    {
        CRC16Value = (CRC16Value >> 8) ^ fcstab[(CRC16Value ^ data[i]) & 0xff];
    }
    
    return ~CRC16Value & 0xffff;
}
*/

/*************************************************************************
 * 函数名称: 	Check698Frame(unsigned char *buff, U16 len,U8 *addr)
 * 函数说明: 	校验698协议
 * 参数说明: 	*buff           - 校验数据
 *             len          - 数据长度
 * 			   *FEnum       - FE数量
 *             *addr        - 解析后表地址
 * 返回值  ：  解析结果
 *************************************************************************/

 /*
unsigned char Check698Frame(unsigned char *buff, U16 len,U8 *FEnum,U8 *addr)
{
    U16  i = 0;
    U16  BaseIndex = 0;
    U16  dataLen = 0;
    U16  cs16 = 0;
    U16  CRC16Cal = 0;
	
    for(i=0; i<len; i++)
    {
        if(*(buff + i) == 0x68)
        {
            BaseIndex = i;
            break;
        }
    }
    
	if(FEnum)
    {
        *FEnum = BaseIndex;
    }
    
	if(len-BaseIndex < 12)
    {
        return FALSE;
    }
	
    dataLen = (*(buff + BaseIndex + 1) | (*(buff + BaseIndex + 1 + 1) << 8)) + 2;

	app_printf("repect : 0x%x  dataLen : %d\n",buff[BaseIndex+dataLen-1],dataLen);

    if(buff[BaseIndex+dataLen-1] != 0x16)
    {
        return FALSE;
    }

    cs16 = (*(buff + BaseIndex + dataLen - 2)<<8) | (*(buff + BaseIndex + dataLen - 3));
	app_printf("cs16 : 0x%04x\n",cs16);

	
	CRC16Cal = ResetCRC16();
    if(cs16 != CalCRC16(CRC16Cal,buff + BaseIndex + 1, 0 , dataLen - 4))
    {
		app_printf("cs16 error\n");
        return FALSE;
    }
	
    if(addr)
	{
		__memcpy(addr , buff + BaseIndex + 5,6);
	}
	
    return TRUE;
}
*/

void Dl698PacketFrame(APDU_TYPE_e apduType, U8 subType, PIID_s piid, U8 listNum, U8 securityFlag,
                                                        U8 *pTxApdu, U16 apduLen, U8 *recivebuf ,U16 *reciveLen)
{
    U16  retLen = *reciveLen;
    U8  *pHcs;
    U8  headerLen;
    dl698frame_header_s  *pFrameHeader = (dl698frame_header_s*)recivebuf;

    U16  cs16 = 0;
    U16  CRC16Cal = 0;

    //pFrameHeader->FrameLen = retLen + 2 + 4 + apduLen + 2 + 1 - 2;

    pHcs = &recivebuf[retLen];
    headerLen = retLen;
    
    retLen += 2;  // HCS

    if(securityFlag == TRUE)
    {
        recivebuf[retLen++] = SECURITY_REQUEST;
        recivebuf[retLen++] = 0x00;  // 明文应用数据单元
        
        // 明文应用数据长度
        recivebuf[retLen++] = (subType == 0x02) ? 4+apduLen :
                              (subType == 0x04) ? 4+apduLen : 3+apduLen;
    }
    recivebuf[retLen++] = apduType;
    recivebuf[retLen++] = subType;
    recivebuf[retLen++] = *((U8*)&piid);

    if(subType == 0x02 || subType == 0x04)
    {
        recivebuf[retLen++] = listNum;
    }

    __memcpy(&recivebuf[retLen], pTxApdu, apduLen);
    retLen += apduLen;

    if(securityFlag == TRUE)
    {
        recivebuf[retLen++] = 0x01;  // 随机数
        recivebuf[retLen++] = sizeof(RN_DATA);

        __memcpy(&recivebuf[retLen], RN_DATA, sizeof(RN_DATA));
        retLen += sizeof(RN_DATA);
    }

    pFrameHeader->FrameLen = (retLen+2+1)-2;

    CRC16Cal = ResetCRC16();
    cs16 = CalCRC16(CRC16Cal, &recivebuf[1], 0, headerLen-1);
    pHcs[0] = (U8)(cs16 & 0xFF);
    pHcs[1] = (U8)(cs16 >> 8);
    
    CRC16Cal = ResetCRC16();
    cs16 = CalCRC16(CRC16Cal, &recivebuf[1], 0, retLen-1);
    recivebuf[retLen++] = (U8)(cs16 & 0xFF);
    recivebuf[retLen++] = (U8)(cs16 >> 8);

    recivebuf[retLen++] = 0x16;
    *reciveLen = retLen;

    return;

}

static void SendGetResponseNormal(PIID_s piid, U8 *pTxApdu, U16 apduLen, U8 *recivebuf ,U16 *reciveLen)
{
    pTxApdu[apduLen++] = 0x00;   // 无跟随上报信息域
    pTxApdu[apduLen++] = 0x00;   // 无时间标签

    Dl698PacketFrame(GET_RESPONSE, 0x01, piid, 0, FALSE, pTxApdu, apduLen, recivebuf, reciveLen);
}

static void SendGetResponseNormalList(PIID_s piid, U8 listNum, U8 *pTxApdu, U16 apduLen, U8 *recivebuf ,U16 *reciveLen)
{
    pTxApdu[apduLen++] = 0x00;   // 无跟随上报信息域
    pTxApdu[apduLen++] = 0x00;   // 无时间标签

    Dl698PacketFrame(GET_RESPONSE, 0x02, piid, listNum, FALSE, pTxApdu, apduLen, recivebuf, reciveLen);
}

static void SendGetResponseRecord(PIID_s piid, U8 *pTxApdu, U16 apduLen, U8 *recivebuf ,U16 *reciveLen)
{
    pTxApdu[apduLen++] = 0x00;   // 无跟随上报信息域
    pTxApdu[apduLen++] = 0x00;   // 无时间标签

    Dl698PacketFrame(GET_RESPONSE, 0x03, piid, 0, FALSE, pTxApdu, apduLen, recivebuf, reciveLen);
}

/*
static void GetResponseRecord(){}
static void GetResponseRecordList(){}
static void GetResponseNext(){}
static void GetResponseMD5(){}

static void ActionResponseNormal(){}
*/

static void SendActionResponseNormalList(PIID_s piid, U8 listNum, U8 *pTxApdu, U16 apduLen, U8 *recivebuf ,U16 *reciveLen)
{
    pTxApdu[apduLen++] = 0x00;   // 无跟随上报信息域
    pTxApdu[apduLen++] = 0x00;   // 无时间标签
    
    Dl698PacketFrame(ACTION_RESPONSE, 2, piid, listNum, FALSE, pTxApdu, apduLen, recivebuf, reciveLen);

    return;
}

/*
extern U16 GetTaskCfgInfo(OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);
extern U16 GetCollectPlanCfg(OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);

#if defined(ZC3750CJQ2)
extern U16 GetCollectStatus(OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);
extern U16 GetCollectTaskState(OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);
extern U16 GetCjqAttribute(OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);
#endif

extern U16 GetDeviceFailureRecord(OAD_s Oad, U8 *pRxApdu, U8 *pTxApdu);
*/

U8 ParseOIDataDemandTime(U8 *pApdu, U16 *pProcLen, void *p698Data)
{
    U16 byLen = 0;
    DLT698_data_s  setdata = {0};

    if(FALSE == DLT698ParseData(&pApdu[byLen], NULL, &setdata) || STRUCTURE != setdata.dataType || 
                                              setdata.dataNum != 2)
    {
        return FALSE;
    }
    byLen += setdata.procLen;

    if(FALSE == DLT698ParseData(&pApdu[byLen], p698Data, &setdata)
                                         || DOUBLE_LONG_UNSIGNED != setdata.dataType)
    {
        return FALSE;
    }
    byLen += setdata.procLen;

    if(FALSE == DLT698ParseData(&pApdu[byLen], (void*)((U8*)p698Data + 4), &setdata)
                                         || DATE_TIME_S != setdata.dataType)
    {
        return FALSE;
    }
    byLen += setdata.procLen;

    *pProcLen += byLen;
    app_printf("byLen = %d,*pProcLen = %d\n",byLen,*pProcLen);
    return TRUE;
}


U8 ParseOIData(U8 *pApdu, U16 *pProcLen, U8 listNum, data_type_e dateType, U8 itemLen, void *p698Data)
{
    U8  ii;
    U16 byLen = 0;
    U8  ret = TRUE;
    DLT698_data_s  setdata = {0};
    

    if(listNum > 1)
    {
        if(FALSE == DLT698ParseData(&pApdu[byLen], NULL, &setdata) || ARRAY != setdata.dataType)
        {
            return FALSE;
        }
        byLen += setdata.procLen;
    }
    else
    {
        setdata.dataNum = 1;
    }
    app_printf("setdata.dataNum %d ,dataType %d\n",setdata.dataNum,setdata.dataType);

    for(ii=0; ii<setdata.dataNum; ii++)
    {
        app_printf("byLen %d,setdata.procLen = %d\n",byLen,setdata.procLen);
        dump_buf(&pApdu[byLen],30);
        if(FALSE == DLT698ParseData(&pApdu[byLen], (p698Data + ii*itemLen), &setdata)
                   || (dateType != setdata.dataType&&setdata.dataType !=e_NULL))//数据为空时，依旧需要偏移一个字节
        {
            ret = FALSE;
            goto ret1;
        }
        byLen += setdata.procLen;
    }
    
ret1:    
    *pProcLen += byLen;
    app_printf("byLen = %d,*pProcLen = %d\n",byLen,*pProcLen);
    return ret;
}

/*
U8 ParseVoltageData(U8 *pApdu, U16 *pProcLen, void *p698Data)
{
    U8  ii;
    U16 byLen = 0;
    U8  retStatus;
    U8  ret = TRUE;
    DLT698_data_s  setdata = {0};
    
    
    retStatus = pApdu[byLen++];

    if(retStatus != 0x01)  // 不等于数据时
    {
        byLen++;     // DAR值

        ret = FALSE;
        goto ret1;
    }

    if(FALSE == DLT698ParseData(&pApdu[byLen], NULL, &setdata) || ARRAY != setdata.dataType)
    {
        return FALSE;
    }
    byLen += setdata.procLen;
    
    for(ii=0; ii<setdata.dataNum; ii++)
    {
        if(FALSE == DLT698ParseData(&pApdu[byLen], (p698Data + ii*2), &setdata)
                   || LONG_UNSIGNED != setdata.dataType)
        {
            ret = FALSE;
            goto ret1;
        }
        byLen += setdata.procLen;
    }


ret1:    
    *pProcLen += byLen;
    return ret;
}

U8 ParseCurrentData(U8 *pApdu, U16 *pProcLen, void *p698Data)
{
    U8  ii;
    U16 byLen = 0;
    U8  retStatus;
    U8  ret = TRUE;
    DLT698_data_s  setdata = {0};
    
    
    retStatus = pApdu[byLen++];

    if(retStatus != 0x01)  // 不等于数据时
    {
        byLen++;     // DAR值

        ret = FALSE;
        goto ret1;
    }

    if(FALSE == DLT698ParseData(&pApdu[byLen], NULL, &setdata) || ARRAY != setdata.dataType)
    {
        return FALSE;
    }
    byLen += setdata.procLen;
    
    for(ii=0; ii<setdata.dataNum; ii++)
    {
        if(FALSE == DLT698ParseData(&pApdu[byLen], (p698Data + ii*4), &setdata)
                   || DOUBLE_LONG != setdata.dataType)
        {
            ret = FALSE;
            goto ret1;
        }
        byLen += setdata.procLen;
    }


ret1:    
    *pProcLen += byLen;
    return ret;    
}

U8 ParseZeroCurrentData(U8 *pApdu, U16 *pProcLen, void *p698Data)
{
    U16 byLen = 0;
    U8  retStatus;
    U8  ret = TRUE;
    DLT698_data_s  setdata = {0};
        
        
    retStatus = pApdu[byLen++];
    
    if(retStatus != 0x01)  // 不等于数据时
    {
        byLen++;     // DAR值
    
        ret = FALSE;
        goto ret1;
    }

    if(FALSE == DLT698ParseData(&pApdu[byLen], p698Data, &setdata)
                       || DOUBLE_LONG != setdata.dataType)
    {
        ret = FALSE;
        goto ret1;
    }
    byLen += setdata.procLen;
    
    
ret1:    
    *pProcLen += byLen;
    return ret; 
}

U8 ParseFactorData(U8 *pApdu, U16 *pProcLen, void *p698Data)
{
    U8  ii;
    U16 byLen = 0;
    U8  retStatus;
    U8  ret = TRUE;
    DLT698_data_s  setdata = {0};
    
    
    retStatus = pApdu[byLen++];

    if(retStatus != 0x01)  // 不等于数据时
    {
        byLen++;     // DAR值

        ret = FALSE;
        goto ret1;
    }

    if(FALSE == DLT698ParseData(&pApdu[byLen], NULL, &setdata) || ARRAY != setdata.dataType)
    {
        return FALSE;
    }
    byLen += setdata.procLen;
    
    for(ii=0; ii<setdata.dataNum; ii++)
    {
        if(FALSE == DLT698ParseData(&pApdu[byLen], (p698Data + ii*2), &setdata)
                   || DATA_LONG != setdata.dataType)
        {
            ret = FALSE;
            goto ret1;
        }
        byLen += setdata.procLen;
    }


ret1:    
    *pProcLen += byLen;
    return ret; 

}

DLT698_OI_PARSE_FUNC   ClientProcOIFuncArray[] = 
{
    {0x20000200, ParseVoltageData},
    {0x20010200, ParseCurrentData},
    {0x20010400, ParseZeroCurrentData},
    {0x200A0200, ParseFactorData},
};
*/

static void ProcGetRequestNormal(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
    U16     byLen = 0;
    PIID_s  piid;
    OAD_s   Oad;

    U8      ii;
    U8      count;
    U16     retLen = 0;
    //U8      retStatus;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
        
    Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Oad.AttriId      = (pApdu[byLen] & 0x1F);
    Oad.AttriFeature = (pApdu[byLen] >> 5);
    byLen += 1;
    Oad.AttriElementIdx = pApdu[byLen++];

    count = sizeof(ClientGetOIFuncArray)/sizeof(DLT698_OI_GET_FUNC);

    for(ii=0; ii<count; ii++)
    {
        if(ClientGetOIFuncArray[ii].OI == Oad.OI)
        {
            retLen += ClientGetOIFuncArray[ii].Func(Oad, &pApdu[byLen], Dl698RetApdu);
        }
    }

    // 应答报文APDU
    SendGetResponseNormal(piid, Dl698RetApdu, retLen, recivebuf, reciveLen);
}

static void ProcGetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
    U16     byLen = 0;
    PIID_s  piid = {0};
    U8      listNum = 0;
    U8      ii, jj;
    OAD_s   Oad;

    U8      count;
    U16     retLen = 0;
    //U8      retStatus;
    
    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
    
    listNum = pApdu[byLen++];

    for(ii=0; ii<listNum; ii++)
    {
        Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
        byLen += 2;
        Oad.AttriId      = (pApdu[byLen] & 0x1F);
        Oad.AttriFeature = (pApdu[byLen] >> 5);
        byLen += 1;
        Oad.AttriElementIdx = pApdu[byLen++];

        count = sizeof(ClientGetOIFuncArray)/sizeof(DLT698_OI_GET_FUNC);

        for(jj=0; jj<count; jj++)
        {
            if(ClientGetOIFuncArray[jj].OI == Oad.OI)
            {
                retLen += ClientGetOIFuncArray[jj].Func(Oad, &pApdu[byLen], Dl698RetApdu);
            }
        }
    }

    SendGetResponseNormalList(piid, listNum, Dl698RetApdu, retLen, recivebuf, reciveLen);
}

static void ProcGetRequestRecord(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
    U16     byLen = 0;
    PIID_s  piid;
    OAD_s   Oad;

    U8      ii;
    U8      count;
    U16     retLen = 0;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);

    Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Oad.AttriId      = (pApdu[byLen] & 0x1F);
    Oad.AttriFeature = (pApdu[byLen] >> 5);
    byLen += 1;
    Oad.AttriElementIdx = pApdu[byLen++];

    count = sizeof(ClientGetOIFuncArray)/sizeof(DLT698_OI_GET_FUNC);

    for(ii=0; ii<count; ii++)
    {
        if(ClientGetOIFuncArray[ii].OI == Oad.OI)
        {
            retLen += ClientGetOIFuncArray[ii].Func(Oad, &pApdu[byLen], Dl698RetApdu);
        }
    }

    SendGetResponseRecord(piid, Dl698RetApdu, retLen, recivebuf, reciveLen);
}

static void ProcGetRequestRecordList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcGetRequestNext(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcGetRequestMD5(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcSetRequestNormal(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcSetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcSetThenGetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcActionRequest(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
    U16     byLen = 0;
    PIID_s  piid = {0};
    OMD_s   Omd;

    //U16  retLen = 0;
    //U8   retStatus;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);

    Omd.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Omd.MethodId    = pApdu[byLen++];
    Omd.OperateMode = pApdu[byLen++];

    //retStatus = DLT698_SUCCESS;    // 返回状态先置为成功
    app_printf("Omd.OI %04x,MethodId %d\n",Omd.OI,Omd.MethodId);
    if(Omd.OI == 0x6043) // 任务配置表
    {            
        if(Omd.MethodId == 0x7F)   // 方法127
        {
            U8    prm;
            data_type_e  dataType = pApdu[byLen++];
            
            if(dataType != UNSIGNED)
                return;

            prm = pApdu[byLen++];
            #if defined(ZC3750STA) || defined(ZC3750CJQ2)
            if(prm == 0)   // 点名上报关闭
            {
                
            }
            else if(prm == 1)  // 点名上报开启
            {
            }
            #else
            if(prm == 0)   // 点名上报关闭
            {
                
            }
            else if(prm == 1)  // 点名上报开启
            {
                
            }
            #endif
        }
			//SendActionResponseNormalList(piid, 0, Dl698RetApdu, retLen, receivebuf, receiveLen);
       
        //*receiveLen = 0;
        //SendReportNotificationSimplifyRecord();
        //SendReportNotificationRecordList(U8 *pMeterAddr, U8 listNum, OAD_s *pTaskOad, U8 CsdNum, CSD_s *pCsd, U8 usrType,
        //                                 U8 recordNum, U8 *pRecordData, U8 *ReportBuf ,U16 *ReportLen);
    }
	else if (Omd.OI == 0x4000)
	{
		// 日期时间
		DLT698_data_s ParseData;
		if (Omd.MethodId == 0x7F)
		{
			DLT698ParseData(&pApdu[byLen], (void *)recivebuf, &ParseData);
			*reciveLen = sizeof(date_time_s);
			dump_buf(recivebuf, *reciveLen);
		}
	}
}

static void ProcActionRequestList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
    U16     byLen = 0;
    PIID_s  piid = {0};
    U8      listNum = 0;
    U8      ii;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
    
    listNum = pApdu[byLen++];

    //U8   retApdu[50];
    U16  retLen = 0;
    U8   retStatus;
    
    for(ii=0; ii<listNum; ii++)
    {
        OMD_s   Omd;
        
        Omd.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
        byLen += 2;
        Omd.MethodId    = pApdu[byLen++];
        Omd.OperateMode = pApdu[byLen++];

        retStatus = DLT698_SUCCESS;    // 返回状态先置为成功

        
        
        if(Omd.OI == 0x6012) // 任务配置表
        {            
            if(Omd.MethodId == 0x7F)   // 方法127
            {
                data_type_e  dataType = pApdu[byLen++];
                U8           arrayNum;
                U8           jj;
                
                if(dataType != ARRAY)
                    return;
                
                arrayNum = pApdu[byLen++];
                for(jj=0; jj<arrayNum; jj++)
                {
                    //if(FALSE == AddTask(&pApdu[byLen], ApduLen-byLen, &byLen))
                    {
                        retStatus = TYPE_MISMATCH;
                    }
                }

                
            }
            else if(Omd.MethodId == 0x80)  // 方法128
            {
                data_type_e  dataType = pApdu[byLen++];
                U8           arrayNum;
                U8           taskId;
                U8           jj;

                DLT698_data_s setdata = {0};
                
                if(dataType != ARRAY)
                    return;
                
                arrayNum = pApdu[byLen++];
                for(jj=0; jj<arrayNum; jj++)
                {
                    if(FALSE == DLT698ParseData(&pApdu[byLen], &(taskId), &setdata)
                                      || UNSIGNED != setdata.dataType)
                    {
                        return;
                    }
                    byLen += setdata.procLen;
                                      
                    //DeleteTask(taskId);
                }
            }
            else if(Omd.MethodId == 0x81)  // 方法129
            {
                //void ClearTask() 
                byLen++;  // 数据为空
            }
            else if(Omd.MethodId == 0x82)  // 方法130
            {
                //void UpdateTask()
            }
            else
            {
                ;
            }
        }
        else if(Omd.OI == 0x6014)  // 普通采集方案集
        {
            if(Omd.MethodId == 0x7F)   // 方法127
            {
                data_type_e  dataType = pApdu[byLen++];
                U8           arrayNum;
                U8           jj;
                
                if(dataType != ARRAY)
                    return;
                
                arrayNum = pApdu[byLen++];
                for(jj=0; jj<arrayNum; jj++)
                {
                    //AddCollectPlan(&pApdu[byLen], ApduLen-byLen, &byLen);
                }
            }
            else if(Omd.MethodId == 0x80)  // 方法128
            {

            }
            else if(Omd.MethodId == 0x81)  // 方法129
            {

            }
            else if(Omd.MethodId == 0x82)  // 方法130
            {

            }
        }
        
        // 应答报文APDU
        Dl698RetApdu[retLen++] = (U8)(Omd.OI >> 8);
        Dl698RetApdu[retLen++] = (U8)(Omd.OI & 0xFF);
        Dl698RetApdu[retLen++] = Omd.MethodId;
        Dl698RetApdu[retLen++] = Omd.OperateMode;

        Dl698RetApdu[retLen++] = retStatus;
        Dl698RetApdu[retLen++] = 0x00;
        
    }

    SendActionResponseNormalList(piid, listNum, Dl698RetApdu, retLen, recivebuf, reciveLen);

    return;        
}

static void ProcActionThenGetRequestNormalList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen){}

static void ProcReportResponseRecordList(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
/*
    app_printf("pApdu[0] %d\n",pApdu[0]);
    dump_buf(pApdu, ApduLen);
#if defined(ZC3750STA) || defined(ZC3750CJQ2)
    U8  ii;
    U16     byLen = 0;
    PIID_s  piid = {0};
    OAD_s   Oad;
    U8      OadNum = 0;

    //U16  retLen = 0;
    //U8   retStatus;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
    OadNum = pApdu[byLen++];
    Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Oad.AttriId      = (pApdu[byLen] & 0x1F);
    Oad.AttriFeature = (pApdu[byLen] >> 5);
    byLen += 1;
    Oad.AttriElementIdx = pApdu[byLen++];  

    
    app_printf("OadNum = %d Omd.OI %04x\n",OadNum,Oad.OI);
    if(Oad.OI == 0x6012) // 任务配置表-记录表   上报成功
    {

        queue_ctrl_s    QueueTemp;
        //__memcpy(&QueueTemp,ReportCtrlDay.QueueReport,sizeof(queue_ctrl_s));
        QueueTemp.ReadPtr = ReportCtrlDay.QueueReport->ReadPtr;
        QueueTemp.WritePtr = ReportCtrlDay.QueueReport->WritePtr;
        QueueTemp.PointCount = ReportCtrlDay.QueueReport->PointCount;
        QueueTemp.MaxQueueSize = ReportCtrlDay.QueueReport->MaxQueueSize;
        app_printf("ReportCtrlDay ReportCtrlDay.PointNum = %dPointCount = %d\n",ReportCtrlDay.PointNum,QueueTemp.PointCount);
        for(ii = 0;ii < ReportCtrlDay.PointNum;ii++)
        {
            OutQueue(ReportCtrlDay.QueueReport,QueueTemp.MaxQueueSize);
        }
        ReportCtrlDay.PointNum = 0;
        ReportCtrlDay.ReportFlag = FALSE;
        ModifyDlt698ReportTimer(200);
        ReportMeter.MeterCnt ++;

        if(Dlt698ReportFlag == NotificationSimplifyRecord_e)
        {
            PacketReportNotificationSimplifyRecord(receivebuf ,receiveLen);
        }
        else 
        {
            PacketReportNotificationRecordList(receivebuf ,receiveLen);
        }
    }
#endif    
*/
}

static void ProcReportResponseSimplifyRecord(U8 *pApdu, U16 ApduLen, U8 *recivebuf ,U16 *reciveLen)
{
/*
    app_printf("pApdu[0] %d\n",pApdu[0]);

    U8  ii;
    queue_ctrl_s    QueueTemp;
    __memcpy(&QueueTemp,ReportCtrlMin.QueueReport,sizeof(queue_ctrl_s));
    
    if(pApdu[0] == 0x02)  // 上报成功
    {
        ReportCtrlMin.ReportFlag = FALSE;
        //ReportCtrl.QueueReport;
        app_printf("ReportCtrlMin PointCount = %d\n",QueueTemp.PointCount);
        for(ii = 0;ii < QueueTemp.PointCount;ii++)
        {
            OutQueue(ReportCtrlMin.QueueReport,QueueTemp.MaxQueueSize);
        }
    }
*/
}

static U8 ProcGetResponseNormal(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U16     byLen = 0;
    PIID_s  piid;
    OAD_s   Oad;

    U8      retState = TRUE;    
    

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
        
    Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Oad.AttriId      = (pApdu[byLen] & 0x1F);
    Oad.AttriFeature = (pApdu[byLen] >> 5);
    byLen += 1;
    Oad.AttriElementIdx = pApdu[byLen++];  

    if(pApdu[byLen] != 0x01)  // 不等于数据时
    {
        byLen += 2;     // DAR值

        retState = FALSE;
        return retState;
    }

    switch(Oad.OI)
    {
        case 0x2000:  // 电压
            retState = ParseOIData(&pApdu[byLen], &byLen, 3, LONG_UNSIGNED, 2, p698Data);
            break;
        case 0x2001: // 电流
            if(Oad.AttriId == 0x02)
            {
                retState = ParseOIData(&pApdu[byLen], &byLen, 3, DOUBLE_LONG, 4, p698Data);
            }
            else if(Oad.AttriId == 0x04)
            {
                retState = ParseOIData(&pApdu[byLen], &byLen, 1, DOUBLE_LONG, 4, p698Data);
            }
            break;
        case 0x200A:  // 功率因数
            retState = ParseOIData(&pApdu[byLen], &byLen, 3, DATA_LONG, 2, p698Data);
            break;
        default:
            break;
    }

    upReportFlag = pApdu[byLen++];
    timeTag      = pApdu[byLen++];

    return retState;
}

static U8 ProcGetResponseNormalList(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U16     byLen = 0;
    U16     dataLen = 0;
    PIID_s  piid;
    OAD_s   Oad;

    U8      ii;
    U8      listNum;
    U8      retState = TRUE;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);

    listNum = pApdu[byLen++];

    //minute_cllct_data_s   *pCllctData = (minute_cllct_data_s*)p698Data;
    app_printf("listNum %d \n",listNum);

    for(ii=0; ii<listNum; ii++)
    {        
        Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
        byLen += 2;
        Oad.AttriId      = (pApdu[byLen] & 0x1F);
        Oad.AttriFeature = (pApdu[byLen] >> 5);
        byLen += 1;
        Oad.AttriElementIdx = pApdu[byLen++];

        if(pApdu[byLen] != 0x01)  // 不等于数据时
        {
            byLen += 2;     // DAR值

            continue;
        }
        else
        {
            byLen++;
        }

        switch(Oad.OI)
        {
            case 0x2000:  // 电压
                memset(&p698Data[dataLen], 0xFF, 3*2);
                retState = ParseOIData(&pApdu[byLen], &byLen, 3, LONG_UNSIGNED, 2, (void*)&p698Data[dataLen]);
                dataLen += 3*2;
                app_printf("0x2000 byLen %d \n",byLen);
                break;
            case 0x2001: // 电流
                if(Oad.AttriId == 0x02)
                {
                    memset(&p698Data[dataLen], 0xFF, 3*4);
                    retState = ParseOIData(&pApdu[byLen], &byLen, 3, DOUBLE_LONG, 4, (void*)&p698Data[dataLen]);
                    dataLen += 3*4;
                    app_printf("0x2001 0x02 byLen %d \n",byLen);
                }
                else if(Oad.AttriId == 0x04)
                {
                    memset(&p698Data[dataLen], 0xFF, 1*4);
                    retState = ParseOIData(&pApdu[byLen], &byLen, 1, DOUBLE_LONG, 4, (void*)&p698Data[dataLen]);
                    dataLen += 1*4;
                    app_printf("0x200A 0x04 byLen %d \n",byLen);
                }
                break;
            case 0x200A:  // 功率因数
                memset(&p698Data[dataLen], 0xFF, 4*2);
                retState = ParseOIData(&pApdu[byLen], &byLen, 4, DATA_LONG, 2, (void*)&p698Data[dataLen]);
                dataLen += 4*2;
                app_printf("0x200A byLen %d \n",byLen);
                break;
            default:
                break;
        }
    }

    upReportFlag = pApdu[byLen++];
    timeTag      = pApdu[byLen++];

    return retState;
}

static U8 ProcGetResponseRecord(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U8      ii, jj;
    U16     byLen = 0;
    PIID_s  piid;
    OAD_s   Oad;

    U8      retState = TRUE;

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);

    // OAD
    Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Oad.AttriId      = (pApdu[byLen] & 0x1F);
    Oad.AttriFeature = (pApdu[byLen] >> 5);
    byLen += 1;
    Oad.AttriElementIdx = pApdu[byLen++]; 

    RCSD_s  RespRCSD;
    U16     procLen;
    U8      recordNum;
    U8      itemNum = 0;
    U16     dataLen = 0;
    DLT698_data_s ParseData;
    U16 oi;
    

    RespRCSD.num = pApdu[byLen++];    // 一行记录N列属性描述符
    app_printf("RespRCSD.num %d \n",RespRCSD.num);

    for(ii=0; ii<RespRCSD.num; ii++)
    {
        procLen = 0;
        
        if(TRUE == DLT698Set698data_CSD(&pApdu[byLen], &RespRCSD.csd[ii], &procLen))
        {
            byLen += procLen;
        }            
    }

    if(pApdu[byLen] != 0x01)  // 不等于数据时
    {
        byLen += 2;     // DAR值

        retState = FALSE;
        return retState;
    }
    else
    {
        byLen++;
    }

    recordNum = pApdu[byLen++];
    app_printf("recordNum %d \n",recordNum);

    for(ii=0; ii<recordNum; ii++)
    {
        oi = Oad.OI;
        Swap((U8*)&(oi), 2);
        if(Oad.OI == 0x5004 || Oad.OI == 0x5006)  // 日冻结数据或者月冻结数据
        {        
            itemNum = 5;
        }
        else if(Oad.OI == 0x5002)  // 分钟冻结数据
        {
            itemNum = 1;
        }
        app_printf("itemNum %d \n",itemNum);

        for(jj=0; jj<RespRCSD.num; jj++)
        {
            app_printf("byLen %d,procLen = %d\n",byLen,ParseData.procLen);
            dump_buf(&pApdu[byLen],30);
            dump_buf((U8 *)&RespRCSD.csd[jj].unioncsd.oad.OI,2);
            oi = RespRCSD.csd[jj].unioncsd.oad.OI;
            Swap((U8*)&(oi), 2);
            switch(oi)
            {
                case 0x2021:
                    memset(&p698Data[dataLen], 0xFF, sizeof(date_time_s));
                    
                    procLen = 0;
                
                    retState = DLT698ParseData(&pApdu[byLen], (void*)&p698Data[dataLen], &ParseData);
                    //retState = DLT698Set698data_DATE_TIME_S(&pApdu[byLen], (void*)&p698Data[dataLen], &procLen);
                    byLen += ParseData.procLen;

                    app_printf("byLen %d,procLen = %d\n",byLen,ParseData.procLen);
                    dataLen += sizeof(date_time_s);
                    break;
                    
                case 0x0010:
                case 0x0020:
                case 0x0030:
                case 0x0040:
                case 0x0050:
                case 0x0060:
                case 0x0070:
                case 0x0080:
                    memset(&p698Data[dataLen], 0xFF, itemNum*4);
                    retState = ParseOIData(&pApdu[byLen], &byLen, itemNum, DOUBLE_LONG_UNSIGNED, 4, (void*)&p698Data[dataLen]);
                    dataLen += itemNum*4;
                    //byLen += itemNum*4;
                    break;
                
                case 0x1010:
                case 0x1020:
                case 0x1030:
                case 0x1040:
                {
                    U8 kk;

                    for(kk=0; kk<itemNum; kk++)
                    {
                        memset(&p698Data[dataLen], 0xFF, 11);
                        retState = ParseOIDataDemandTime(&pApdu[byLen], &byLen, (void*)&p698Data[dataLen]);
                        dataLen += 11;
                        //byLen += itemNum*4;
                    }
                    break;                
                }    
                default:
                    break;
            }


        }
    }

    upReportFlag = pApdu[byLen++];
    timeTag      = pApdu[byLen++];

    return retState;
}

static U8 ProcGetResponseRecordList(U8 *pApdu, U16 ApduLen, U8 *p698Data){return 0;}

static U8 ProcGetResponseNext(U8 *pApdu, U16 ApduLen, U8 *p698Data){return 0;}

static U8 ProcGetResponseMD5(U8 *pApdu, U16 ApduLen, U8 *p698Data){return 0;}

static U8 ProcGetActionRequestNormal(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U16     byLen = 0;
    PIID_s  piid;
    OMD_s   Omd;

    U8      retState = FALSE;    
    

    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
        
    Omd.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
    byLen += 2;
    Omd.MethodId    = pApdu[byLen++];
    Omd.OperateMode = pApdu[byLen++];
    app_printf("Omd.OI = %x Omd.MethodId = %x,\n",Omd.OI,Omd.MethodId);
    if(Omd.OI == 0xF209)
    {
        if(Omd.MethodId == 0x80)   // 方法128
        {
            if(pApdu[byLen] == 0)  // DAR值 成功
            {
                //返回OMD
                __memcpy(p698Data,(U8 *)&Omd,sizeof(OMD_s));
                p698Data[4] = 1;

                retState = TRUE;
                
            }
            else
            {
                app_printf("DAR err %d\n",pApdu[byLen]);
            }
        }
    }
    
    p698Data[0] = FALSE;
    return retState;
}

static U8 ProcReportNotificationList(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U16     byLen = 0;
    PIID_s  piid = {0};
    U8      listNum = 0;
    U8      ii;
    OAD_s   Oad;


    U8      retStatus = FALSE;
    
    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
    
    listNum = pApdu[byLen++];

    for(ii=0; ii<listNum; ii++)
    {
        Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
        byLen += 2;
        Oad.AttriId      = (pApdu[byLen] & 0x1F);
        Oad.AttriFeature = (pApdu[byLen] >> 5);
        byLen += 1;
        Oad.AttriElementIdx = pApdu[byLen++];

        if(Oad.OI == 0x3320)
        {
            //返回OAD
            __memcpy(p698Data,(U8 *)&Oad.OI,2);
            p698Data[2] = 1;
            
            retStatus = TRUE;
        }
    }
    return retStatus;

    
}

static U8 ProcReportNotificationRecordList(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U16     byLen = 0;
    PIID_s  piid = {0};
    U8      listNum = 0;
    U8      ii;
    OAD_s   Oad;


    U8      retStatus = FALSE;
    
    __memcpy(&piid, &pApdu[byLen], sizeof(piid));
    byLen += sizeof(piid);
    
    listNum = pApdu[byLen++];

    for(ii=0; ii<listNum; ii++)
    {
        Oad.OI = pApdu[byLen] <<8 | pApdu[byLen+1];
        byLen += 2;
        Oad.AttriId      = (pApdu[byLen] & 0x1F);
        Oad.AttriFeature = (pApdu[byLen] >> 5);
        byLen += 1;
        Oad.AttriElementIdx = pApdu[byLen++];

        if(Oad.OI == 0x3011)
        {
            //返回OAD
            __memcpy(p698Data,(U8 *)&Oad.OI,2);
            p698Data[2] = 1;

            retStatus = TRUE;
        }
    }
    return retStatus;

    
}

static U8 ProcSecurityResponsePlainText(U8 *pApdu, U16 ApduLen, U8 *p698Data)
{
    U16  byLen = 0;
    U16  dataUnitLen = pApdu[byLen++];

    U8   count;
    U8   ii;
    U16  serviceId;
    U8   retState = 0;

    serviceId = pApdu[byLen] << 8 | pApdu[byLen + 1];
    byLen += 2;

    count = sizeof(ResponseProcFuncArray)/sizeof(DLT698_RESPONSE_PROC_FUNC);
        
    for(ii = 0; ii < count; ii++)
    {
        if(ResponseProcFuncArray[ii].ServiceId == serviceId)
        {
            retState = ResponseProcFuncArray[ii].Func(&pApdu[byLen], dataUnitLen-2, p698Data);
        }
    }

    byLen += (dataUnitLen-2);

    // 其它信息处理

    return retState;
}

static U8 ProcSecurityResponseCipherText(U8 *pApdu, U16 ApduLen, U8 *p698Data){return 0;}

U8 ParseCheck698Frame(U8* pDatabuf, U16 dataLen, dl698frame_header_s  ** ppFrameHeader, U8 ** ppApdu, U8 *pAddr, U16 *pApduLen)
{
    U8   FeNum = 0;
    U16  cs16 = 0;
    U16  CRC16Cal = 0;
    U8   SecurityRequest = 0;//安全请求

    if(TRUE != Check698Frame(pDatabuf, dataLen, &FeNum, pAddr, NULL))
    {
        return FALSE;
    }

    *ppFrameHeader = (dl698frame_header_s*)&pDatabuf[FeNum];

    cs16 = (*ppFrameHeader)->SA.Addr[((*ppFrameHeader)->SA.AddrLen+1) + 1] | (*ppFrameHeader)->SA.Addr[((*ppFrameHeader)->SA.AddrLen+1) + 1 + 1] << 8;

    CRC16Cal = ResetCRC16();
    SecurityRequest = (*ppFrameHeader)->SA.Addr[((*ppFrameHeader)->SA.AddrLen+1)+1+2];
    //app_printf("SecurityRequest = %x\n",SecurityRequest);
    if(cs16 != CalCRC16(CRC16Cal, &pDatabuf[FeNum+1], 0, sizeof(dl698frame_header_s)+(*ppFrameHeader)->SA.AddrLen+1))
    {
        app_printf("Header cs16 error\n");
        return FALSE;
    }

    // 服务器地址为SA.AddrLen+1，客户地址1，HCS 2字节
    if(SecurityRequest == SECURITY_REQUEST)
    {
        *ppApdu = &((*ppFrameHeader)->SA.Addr[((*ppFrameHeader)->SA.AddrLen+1) + 1 + 2 + 1 + 2]);
    }
    else
    {
        *ppApdu = &((*ppFrameHeader)->SA.Addr[((*ppFrameHeader)->SA.AddrLen+1) + 1 + 2]);
    }



    *pApduLen = dataLen - FeNum - sizeof(dl698frame_header_s) - ((*ppFrameHeader)->SA.AddrLen+1) - 1 - 2 - 3;

    return TRUE;
}

U8 Server698FrameProc(U8* pDatabuf, U16 dataLen, U8 *pRetDataBuf ,U16 *pRetDataLen)
{
    dl698frame_header_s  *pFrameHeader = NULL;
    U8                   *pApdu;
    U16                   ApduLen;

    U8  dstAddr[6];

    if(TRUE != ParseCheck698Frame(pDatabuf, dataLen, &pFrameHeader, &pApdu, dstAddr, &ApduLen))
    {
        return FALSE;
    }

    if(pFrameHeader->CtrlField.DirBit == 0 && pFrameHeader->CtrlField.PrmBit == 1)
    {
        dl698frame_header_s  *pRetFrameHeader = (dl698frame_header_s*)pRetDataBuf;
        U16                   retLen = 0;

        // 客户机发起的请求            
        pRetFrameHeader->CtrlField.PrmBit = 1;
        pRetFrameHeader->CtrlField.DirBit = 1;
    
        pRetFrameHeader->StartChar = 0x68;
        pRetFrameHeader->Res       = 0;
        pRetFrameHeader->CtrlField.FuncCode = pFrameHeader->CtrlField.FuncCode;
        pRetFrameHeader->CtrlField.Res = 0;
        pRetFrameHeader->CtrlField.FrgmntFlg = 0;
        __memcpy(&pRetFrameHeader->SA, &pFrameHeader->SA, sizeof(SA_s));
        retLen += sizeof(dl698frame_header_s);
    
        __memcpy(&pRetDataBuf[retLen], pFrameHeader->SA.Addr, (pFrameHeader->SA.AddrLen+1)+1);
        retLen += (pFrameHeader->SA.AddrLen+1)+1;
        *pRetDataLen = retLen;
    }

    if(pFrameHeader->CtrlField.DirBit == 0 && pFrameHeader->CtrlField.FuncCode == e_APP_DATA)// 客户机发送，服务器处理
    {
        U8  count;
        U8  ii;
        U16 serviceId = pApdu[0] << 8 | pApdu[1];

        count = sizeof(ClientServiceFuncArray)/sizeof(DLT698_SERVICE_FUNC);
        
        for(ii = 0; ii < count; ii++)
        {
            if(ClientServiceFuncArray[ii].ServiceId == serviceId)
            {
                ClientServiceFuncArray[ii].Func(&pApdu[2], ApduLen-2, pRetDataBuf, pRetDataLen);
            }
        }
        
    }

    return TRUE;
}

U8 Client698FrameProc(U8* pDatabuf, U16 dataLen, void *p698Data)
{
    dl698frame_header_s  *pFrameHeader = NULL;
    U8                   *pApdu;
    U16                   ApduLen;
	U8   byLen = 0;
    U8   ByteCnt    = 0;//字节数
    U8   LengthArea = 0;//长度区

    U8  dstAddr[6];

    if(TRUE != ParseCheck698Frame(pDatabuf, dataLen, &pFrameHeader, &pApdu, dstAddr, &ApduLen))
    {
        return FALSE;
    }

    if(pFrameHeader->CtrlField.DirBit == 1 && pFrameHeader->CtrlField.PrmBit == 1) // 服务器发送，客户机处理
    {
        U8  count;
        U8  ii;
        U16 serviceId = pApdu[0] << 8 | pApdu[1];
        
        // 服务器对客户机请求的响应
        count = sizeof(ResponseProcFuncArray)/sizeof(DLT698_RESPONSE_PROC_FUNC);
        
		LengthArea = (pApdu[byLen+2]&0xF0)>> 4;
	    ByteCnt    = pApdu[byLen+2]&0x0F;
	    if(LengthArea == 8)
	    {
	    	byLen += ByteCnt+2;
	    }
	    else
	    {
			byLen += 2;
	    }

        for(ii = 0; ii < count; ii++)
        {
            if(ResponseProcFuncArray[ii].ServiceId == serviceId)
            {
                ResponseProcFuncArray[ii].Func(&pApdu[byLen], ApduLen-byLen, p698Data);
            }
        }
    }    

    return TRUE;

}

U8 ServerReport698FrameProc(U8* pDatabuf, U16 dataLen, void *p698Data)
{
    dl698frame_header_s  *pFrameHeader = NULL;
    U8                   *pApdu;
    U16                   ApduLen;

    U8  dstAddr[6];

    if(TRUE != ParseCheck698Frame(pDatabuf, dataLen, &pFrameHeader, &pApdu, dstAddr, &ApduLen))
    {
        return FALSE;
    }

    if(pFrameHeader->CtrlField.DirBit == 1 && pFrameHeader->CtrlField.PrmBit == 0
        && pFrameHeader->CtrlField.FuncCode == e_APP_DATA) // 服务器上报
    {
        U8  count;
        U8  ii;
        U16 serviceId = pApdu[0] << 8 | pApdu[1];
        
        // 判定服务器上报
        count = sizeof(ServiceRpeortProcFuncArray)/sizeof(DLT698_RESPONSE_PROC_FUNC);
        
        for(ii = 0; ii < count; ii++)
        {
            if(ServiceRpeortProcFuncArray[ii].ServiceId == serviceId)
            {
                ServiceRpeortProcFuncArray[ii].Func(&pApdu[2], ApduLen-2, p698Data);
            }
        }
    }    

    return TRUE;

}

/*
U8 Process698Frame(U8* pDatabuf, U16 dataLen, U8 *receivebuf ,U16 *receiveLen, void *p698Data)
{
    U8  FeNum = 0;
    U8  dstAddr[6];
    U16  cs16 = 0;
    U16  CRC16Cal = 0;
    
    dl698frame_header_s  *pFrameHeader = NULL;
    U8                   *pApdu;
    U16                   ApduLen;

    dl698frame_header_s  *pRetFrameHeader = (dl698frame_header_s*)receivebuf;
    U16                  retLen = 0;
    
    if(TRUE != Check698Frame(pDatabuf, dataLen, &FeNum, dstAddr))
    {
        return FALSE;
    }
    
    pFrameHeader = (dl698frame_header_s*)&pDatabuf[FeNum];
    
    cs16 = pFrameHeader->SA.Addr[(pFrameHeader->SA.AddrLen+1) + 1] | pFrameHeader->SA.Addr[(pFrameHeader->SA.AddrLen+1) + 1 + 1] << 8;
    
    CRC16Cal = ResetCRC16();
    if(cs16 != CalCRC16(CRC16Cal, &pDatabuf[FeNum+1], 0, sizeof(dl698frame_header_s)+pFrameHeader->SA.AddrLen+1))
    {
		app_printf("Header cs16 error\n");
        return FALSE;
    }

    pRetFrameHeader->StartChar = 0x68;
    pRetFrameHeader->Res       = 0;
    pRetFrameHeader->CtrlField.FuncCode = pFrameHeader->CtrlField.FuncCode;
    pRetFrameHeader->CtrlField.Res = 0;
    pRetFrameHeader->CtrlField.FrgmntFlg = 0;
    __memcpy(&pRetFrameHeader->SA, &pFrameHeader->SA, sizeof(SA_s));
    retLen += sizeof(dl698frame_header_s);

    __memcpy(&receivebuf[retLen], pFrameHeader->SA.Addr, (pFrameHeader->SA.AddrLen+1)+1);
    retLen += (pFrameHeader->SA.AddrLen+1)+1;
    *receiveLen = retLen;

    // 服务器地址为SA.AddrLen+1，客户地址1，HCS 2字节
    pApdu = &(pFrameHeader->SA.Addr[(pFrameHeader->SA.AddrLen+1) + 1 + 2]);
    
    
    ApduLen = dataLen - FeNum - sizeof(dl698frame_header_s) - (pFrameHeader->SA.AddrLen+1) - 1 - 2 - 3;
    
    
    if(pFrameHeader->CtrlField.FuncCode == e_LINK_MANAGE)
    {
        //Proc698LinkManage(pApdu, ApduLen);
    }
    else if(pFrameHeader->CtrlField.FuncCode == e_APP_DATA)
    {
        U8  count;
        U8  ii;
        U16 serviceId = pApdu[0] << 8 | pApdu[1];
        
        if(pFrameHeader->CtrlField.DirBit == 0 && pFrameHeader->CtrlField.PrmBit == 1)
        {
            // 客户机发起的请求            
            pRetFrameHeader->CtrlField.PrmBit = 1;
            pRetFrameHeader->CtrlField.DirBit = 1;
            
            count = sizeof(ClientServiceFuncArray)/sizeof(DLT698_SERVICE_FUNC);
        
            for(ii = 0; ii < count; ii++)
            {
                if(ClientServiceFuncArray[ii].ServiceId == serviceId)
                {
                    ClientServiceFuncArray[ii].Func(&pApdu[2], ApduLen-2, receivebuf, receiveLen);
                }
            }
        }
        else if(pFrameHeader->CtrlField.DirBit == 0 && pFrameHeader->CtrlField.PrmBit == 0)
        {
            // 客户机对服务器上报的响应
            
        }
        else if(pFrameHeader->CtrlField.DirBit == 1 && pFrameHeader->CtrlField.PrmBit == 1)
        {
            // 服务器对客户机请求的响应
            count = sizeof(ResponseProcFuncArray)/sizeof(DLT698_RESPONSE_PROC_FUNC);
        
            for(ii = 0; ii < count; ii++)
            {
                if(ResponseProcFuncArray[ii].ServiceId == serviceId)
                {
                    ResponseProcFuncArray[ii].Func(&pApdu[2], ApduLen-2, p698Data);
                }
            }
        }
        else if(pFrameHeader->CtrlField.DirBit == 1 && pFrameHeader->CtrlField.PrmBit == 0)
        {
            // 服务器发起的上报
        }
    }

    return 0;
}
*/

U8 DLT698Set698data_DATE_TIME_S(U8* pRxApdu, date_time_s* pDateTimes, U16* pProcLen)
{
	U16 proclen = 0;
    
	__memcpy((void*)pDateTimes, (void*)(pRxApdu + proclen), sizeof(date_time_s));
	proclen += sizeof(date_time_s);
    
	Swap((U8*)&(pDateTimes->year), 2);
	*pProcLen = proclen;
    
	return TRUE;
}

U8 DLT698Set698data_ROAD(U8* pRxApdu, ROAD_s* pRoad, U16* pProcLen)
{
	U16 proclen = 0;
	U8  i = 0;
	U8  fullFlag = FALSE;
	
	__memcpy((void*)&(pRoad->OAD), (void*)(pRxApdu + proclen), sizeof(ROAD_s));
	proclen += sizeof(ROAD_s);

	__memcpy((void*)&(pRoad->AssocOADNum), (void*)(pRxApdu + proclen), 1);
	proclen++;

	for(i = 0; i < pRoad->AssocOADNum; i++)
	{		
		if(i < MAX_ASSOC_OAD_NUM)
		{
			__memcpy((void*)(pRoad->AssocOAD + i), (void*)(pRxApdu + proclen), sizeof(ROAD_s));
			//Swap((U8*)&(proad->slaveOAD[i].oi), 2);
		}
		else
		{
			fullFlag = TRUE;
		}

		proclen += sizeof(ROAD_s);
	}

	if(TRUE == fullFlag)
	{
		app_printf("DLT698Set698data_ROAD ROAD_SLAVEOAD_NUM!!!");
		pRoad->AssocOADNum = MAX_ASSOC_OAD_NUM;
	}

	*pProcLen = proclen;
	
	return TRUE;
}

U8 DLT698Set698data_TI(U8* pRxApdu, TI_s* pTi, U16* pProcLen)
{
	U16 proclen = 0;
	
	if(pTi != NULL)
	{
		__memcpy((void*)pTi, (void*)(pRxApdu + proclen), sizeof(TI_s));
		Swap((U8*)&(pTi->InterValue), 2);
		//SysDebugStr_r(SOURCE, "TI: unit %d, interval %d", pti->unit,pti->interval);
	}
	
	proclen += sizeof(TI_s);
	*pProcLen = proclen;
	
	return TRUE;
}

U8 DLT698Set698data_MS(U8* pRxApdu, MS_s* pMs, U16* pProcLen)
{
    U16   proclen = 0;
    U8    i = 0;
    U8    fullFlag = FALSE;

	__memcpy((void*)&(pMs->MS_type), (void*)(pRxApdu + proclen), 1);
	proclen++;

	switch (pMs->MS_type)
	{
		case 2://一组用户类型
		{
			__memcpy((void*)&(pMs->Ms.userTypeUnit.userTypeNum), (void*)(pRxApdu + proclen), 1);
			proclen++;

			for(i = 0; i < pMs->Ms.userTypeUnit.userTypeNum; i++)
			{
				U8 sbtemp = 0;

				proclen++;
				
				if(i < MS_USERTYPE_NUM)
				{
					__memcpy((void*)pMs->Ms.userTypeUnit.userType + i, (void*)(pRxApdu + proclen), 1);
				}
				else
				{
					__memcpy((void*)&sbtemp, (void*)(pRxApdu + proclen), 1);
					fullFlag = TRUE;
				}
				
				proclen++;
			}

			if(TRUE == fullFlag)
			{
				app_printf("DLT698Set698data_MS MS_USERTYPE_NUM!!!");
				pMs->Ms.userTypeUnit.userTypeNum = MS_USERTYPE_NUM;
			}
			
			break;
		}
		case 3://一组用户地址
		{
			__memcpy((void*)&(pMs->Ms.userAddrUnit.userAddrNum), (void*)(pRxApdu + proclen), 1);
			proclen++;

			for(i = 0; i < pMs->Ms.userAddrUnit.userAddrNum; i++)
			{
				//类型应该是 data_TSA						
				if(i < MS_USERADDR_NUM)
				{
					//数据类型 data_TSA
					proclen++;
					//数据长度 data_len
					pMs->Ms.userAddrUnit.userAddr[i].saLen = pRxApdu[proclen++];
					__memcpy((void*)pMs->Ms.userAddrUnit.userAddr[i].saValue, (void*)(pRxApdu + proclen), pMs->Ms.userAddrUnit.userAddr[i].saLen);
				}
				else
				{
					TSA_s tsa = {0};
					//数据类型 data_TSA
					proclen++;
					//数据长度 data_len
					tsa.saLen = pRxApdu[proclen++];
					__memcpy((void*)tsa.saValue, (void*)(pRxApdu + proclen), pMs->Ms.userAddrUnit.userAddr[i].saLen);
					fullFlag = TRUE;
				}

				proclen += pMs->Ms.userAddrUnit.userAddr[i].saLen;
			}

			if(TRUE == fullFlag)
			{
				app_printf("DLT698Set698data_MS MS_USERADDR_NUM!!!\n");
				pMs->Ms.userAddrUnit.userAddrNum = MS_USERADDR_NUM;
			}
			break;
		}
		case 4://一组配置序号
		{
			__memcpy((void*)&(pMs->Ms.ConfigUnit.configNum), (void*)(pRxApdu + proclen), 1);
			proclen++;

			for(i = 0; i < pMs->Ms.ConfigUnit.configNum; i++)
			{
				U16 temp = 0;

				proclen++;
				
				if(i < MS_CONFIGIDX_NUM)
				{
					__memcpy((void*)(pMs->Ms.ConfigUnit.configIdx + i), (void*)(pRxApdu + proclen), 2);
					Swap((U8*)(pMs->Ms.ConfigUnit.configIdx + i), 2);
				}
				else
				{
					__memcpy((void*)&temp, (void*)(pRxApdu + proclen), 2);
					Swap((U8*)&temp, 2);
					fullFlag = TRUE;
				}
				
				proclen += 2;
			}

			if(TRUE == fullFlag)
			{
				app_printf("DLT698Set698data_MS MS_CONFIGIDX_NUM!!!\n");
				pMs->Ms.ConfigUnit.configNum = MS_CONFIGIDX_NUM;
			}
			break;
		}
		case 5:
		{
			break;
		}
		case 6:
		{
			break;
		}
		case 7:
		{
			break;
		}

		default:
		{
			break;
		}
	}

	*pProcLen = proclen;
	return TRUE;
}

U8 DLT698Set698data_RSD(U8* pRxApdu, RSD_s* pRsd, U16* pProcLen)
{
	U16     proclen = 0;
	U16     tmplen = 0;
	U8      i = 0;
	U8      fullFlag = FALSE;
	U16     oi = 0;

	__memcpy((void*)&(pRsd->choice), (void*)(pRxApdu + proclen), 1);
	proclen++;

	switch (pRsd->choice)
	{
		case 0:
		{
			break;
		}

		case 1:
		{
			//OAD
			__memcpy((void*)&(pRsd->rsdselector.select1.oad), (void*)(pRxApdu + proclen), sizeof(OAD_s));
			proclen += sizeof(OAD_s);
			oi = pRsd->rsdselector.select1.oad.OI;
			Swap((U8*)&(oi), 2);

			//DATA
			pRsd->rsdselector.select1.data.dataType = pRxApdu[proclen++];
			//SysDebugStr_r(SOURCE, "DLT698Set698data_RSD01 datatype: %d", pRsd->rsdselector.select1.data.dataType);

			tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);
			//SysDebugStr_r(SOURCE, "DLT698Set698data_RSD01 dataLen: %d", tmplen);

			if(tmplen > 0xFF)
			{
				proclen++;
			}

			//对数据长度进行处理
			pRsd->rsdselector.select1.data.dataLen = (tmplen > 255)? 255:tmplen;

			__memcpy((void*)pRsd->rsdselector.select1.data.data, (void*)(pRxApdu + proclen), 
                                                       pRsd->rsdselector.select1.data.dataLen);
			//SysDebugHex_r("DLT698Set698data_RSD01 data:", pRsd->rsdselector.select1.data.data, tmplen);
			proclen += tmplen;
			break;
		}

		case 2:
		{
			//OAD
			__memcpy((void*)&(pRsd->rsdselector.select2.oad), (void*)(pRxApdu + proclen), sizeof(OAD_s));
			proclen += sizeof(OAD_s);

			//DATA1
			pRsd->rsdselector.select2.dataStart.dataType = pRxApdu[proclen++];

			tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

			if(tmplen > 0xFF)
			{
				proclen++;
			}

			pRsd->rsdselector.select2.dataStart.dataLen = (tmplen > 255)? 255:pRsd->rsdselector.select2.dataStart.dataLen;

			__memcpy((void*)pRsd->rsdselector.select2.dataStart.data, (void*)(pRxApdu + proclen), 
                                              pRsd->rsdselector.select2.dataStart.dataLen);
			proclen += tmplen;

			//DATA2
			pRsd->rsdselector.select2.dataEnd.dataType = pRxApdu[proclen++];

			tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);
			
			if(tmplen > 0xFF)
			{
				proclen++;
			}

			pRsd->rsdselector.select2.dataEnd.dataLen = (tmplen > 255)? 255:pRsd->rsdselector.select2.dataEnd.dataLen;

			__memcpy((void*)pRsd->rsdselector.select2.dataEnd.data, (void*)(pRxApdu + proclen),
                                                             pRsd->rsdselector.select2.dataEnd.dataLen);
			proclen += tmplen;

			//DATA3
			pRsd->rsdselector.select2.dataInterval.dataType = pRxApdu[proclen++];

			tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

			if(tmplen > 0xFF)
			{
				proclen++;
			}

			pRsd->rsdselector.select2.dataInterval.dataLen = (tmplen > 255)? 
			                                                       255:pRsd->rsdselector.select2.dataInterval.dataLen;

			__memcpy((void*)pRsd->rsdselector.select2.dataInterval.data, (void*)(pRxApdu + proclen),
                                                            pRsd->rsdselector.select2.dataInterval.dataLen);
			proclen += tmplen;

			break;
		}

		case 3:
		{
			pRsd->rsdselector.select3.num = pRxApdu[proclen++];

			for(i = 0; i < pRsd->rsdselector.select3.num; i++)
			{
				if(i < RSD_SELECT3_NUM)
				{
					//OAD
					__memcpy((void*)&(pRsd->rsdselector.select3.rsdSelector2[i].oad), (void*)(pRxApdu + proclen),
					                                                                             sizeof(OAD_s));
					proclen += sizeof(OAD_s);

					//DATA1
					pRsd->rsdselector.select3.rsdSelector2[i].dataStart.dataType = pRxApdu[proclen++];

					tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

					pRsd->rsdselector.select3.rsdSelector2[i].dataStart.dataLen = 
                                 (tmplen > 255)? 255:pRsd->rsdselector.select3.rsdSelector2[i].dataStart.dataLen;

					__memcpy((void*)pRsd->rsdselector.select3.rsdSelector2[i].dataStart.data, (void*)(pRxApdu + proclen),
                                                      pRsd->rsdselector.select3.rsdSelector2[i].dataStart.dataLen);
					proclen += tmplen;

					//DATA2
					pRsd->rsdselector.select3.rsdSelector2[i].dataEnd.dataType = pRxApdu[proclen++];

					tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

					pRsd->rsdselector.select3.rsdSelector2[i].dataEnd.dataLen = 
                              (tmplen > 255)? 255:pRsd->rsdselector.select3.rsdSelector2[i].dataEnd.dataLen;

					__memcpy((void*)pRsd->rsdselector.select3.rsdSelector2[i].dataEnd.data, (void*)(pRxApdu + proclen),
                        pRsd->rsdselector.select3.rsdSelector2[i].dataEnd.dataLen);
					proclen += tmplen;

					//DATA2
					pRsd->rsdselector.select3.rsdSelector2[i].dataInterval.dataType = pRxApdu[proclen++];

					tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

					pRsd->rsdselector.select3.rsdSelector2[i].dataInterval.dataLen = 
                                     (tmplen > 255)?255:pRsd->rsdselector.select3.rsdSelector2[i].dataInterval.dataLen;

					__memcpy((void*)pRsd->rsdselector.select3.rsdSelector2[i].dataInterval.data, (void*)(pRxApdu + proclen), 
                                          pRsd->rsdselector.select3.rsdSelector2[i].dataInterval.dataLen);
					proclen += tmplen;
				}
				else
				{
					fullFlag = TRUE;
					RSD_Selector2_s select2;

					//OAD
					__memcpy((void*)&(select2.oad), (void*)(pRxApdu + proclen), sizeof(OAD_s));
					proclen += sizeof(OAD_s);

					//DATA1
					select2.dataStart.dataType = pRxApdu[proclen++];

					tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

					select2.dataStart.dataLen = (tmplen > 255)? 255:select2.dataStart.dataLen;

					__memcpy((void*)(select2.dataStart.data), (void*)(pRxApdu + proclen), select2.dataStart.dataLen);
					proclen += tmplen;

					//DATA2
					select2.dataEnd.dataType = pRxApdu[proclen++];

					tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

					select2.dataEnd.dataLen = (tmplen > 255)? 255:select2.dataEnd.dataLen;

					__memcpy((void*)select2.dataEnd.data, (void*)(pRxApdu + proclen), select2.dataEnd.dataLen);
					proclen += tmplen;

					//DATA2
					select2.dataInterval.dataType = pRxApdu[proclen++];

					tmplen = DLT698GetDataLen(pRxApdu + proclen - 1);

					select2.dataInterval.dataLen = (tmplen > 255)? 255:select2.dataInterval.dataLen;

					__memcpy((void*)select2.dataInterval.data, (void*)(pRxApdu + proclen), select2.dataInterval.dataLen);
					proclen += tmplen;
				}
			}

			if(TRUE == fullFlag)
			{
				app_printf("DLT698Set698data_RSD RSD_SELECT3_NUM!!!");
				pRsd->rsdselector.select3.num = RSD_SELECT3_NUM;
			}
			break;
		}
		
		case 4:
		{
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select4.collectStarttime), &tmplen);
			proclen += tmplen;
			DLT698Set698data_MS(pRxApdu + proclen, &(pRsd->rsdselector.select4.meterSet), &tmplen);
			proclen += tmplen;
			break;
		}

		case 5:
		{
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select5.collectSavetime), &tmplen);
			proclen += tmplen;
			DLT698Set698data_MS(pRxApdu + proclen, &(pRsd->rsdselector.select5.meterSet), &tmplen);
			proclen += tmplen;
			break;
		}

		case 6:
		{
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select6.collectStarttime), &tmplen);
			proclen += tmplen;
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select6.collectEndtime), &tmplen);
			proclen += tmplen;
			DLT698Set698data_TI(pRxApdu + proclen, &(pRsd->rsdselector.select6.timeInterval), &tmplen);
			proclen += tmplen;
			DLT698Set698data_MS(pRxApdu + proclen, &(pRsd->rsdselector.select6.meterSet), &tmplen);
			proclen += tmplen;
			break;
		}

		case 7:
		{
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select7.collectSave_s), &tmplen);
			proclen += tmplen;
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select7.collectSave_e), &tmplen);
			proclen += tmplen;
			DLT698Set698data_TI(pRxApdu + proclen, &(pRsd->rsdselector.select7.timeInterval), &tmplen);
			proclen += tmplen;
			DLT698Set698data_MS(pRxApdu + proclen, &(pRsd->rsdselector.select7.meterSet), &tmplen);
			proclen += tmplen;
			break;
		}

		case 8:
		{
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select8.collectSucc_s), &tmplen);
			proclen += tmplen;
			DLT698Set698data_DATE_TIME_S(pRxApdu + proclen, &(pRsd->rsdselector.select8.collectSucc_e), &tmplen);
			proclen += tmplen;
			DLT698Set698data_TI(pRxApdu + proclen, &(pRsd->rsdselector.select8.timeInterval), &tmplen);
			proclen += tmplen;
			DLT698Set698data_MS(pRxApdu + proclen, &(pRsd->rsdselector.select8.meterSet), &tmplen);
			proclen += tmplen;
			break;
		}

		case 9:
		{
			//__memcpy((void*)&(pRsd->rsdselector.select9.lastNum), (void*)(pRxApdu + proclen), 2);
			proclen++;
			pRsd->rsdselector.select9.lastNum = pRxApdu[proclen++];
			//SysDebugStr_r(SOURCE, "DLT698Set698data_RSD09 lastnum: %d", pRsd->rsdselector.select9.lastNum);
			break;
		}

		case 10:
		{
			//__memcpy((void*)&(pRsd->rsdselector.select10.lastNum), (void*)(pRxApdu + proclen), 2);
			proclen += 1;
			pRsd->rsdselector.select10.lastNum = pRxApdu[proclen++];
			DLT698Set698data_MS(pRxApdu + proclen, &(pRsd->rsdselector.select10.meterSet), &tmplen);
			proclen += tmplen;
			break;
		}

		default:
		{
			return FALSE;
		}
	}

	*pProcLen = proclen;
	return TRUE;
}

static U8 DLT698Set698data_CSD(U8* pRxApdu, CSD_s* pCsd, U16* pProcLen)
{
	U16     proclen = 0;
	U8      i = 0;
	U8      fullFlag = FALSE;
	U16	    oi = 0;
	
	//choice 0或者1
	__memcpy((void*)&(pCsd->choice), (void*)(pRxApdu + proclen), 1);
	proclen++;

	//OAD
	if(0 == pCsd->choice)
	{
		__memcpy((void*)&(pCsd->unioncsd.oad), (void*)(pRxApdu + proclen), sizeof(OAD_s));
		proclen += sizeof(OAD_s);
		oi = pCsd->unioncsd.oad.OI;
		Swap((U8*)&(oi), 2);
	}
	//ROAD
	else if(1 == pCsd->choice)
	{
		__memcpy((void*)&(pCsd->unioncsd.road.OAD), (void*)(pRxApdu + proclen), sizeof(OAD_s));
		proclen += sizeof(OAD_s);
		oi = pCsd->unioncsd.road.OAD.OI;
		Swap((U8*)&(oi), 2);

		__memcpy((void*)&(pCsd->unioncsd.road.AssocOADNum), (void*)(pRxApdu + proclen), 1);
		proclen++;

		for(i = 0; i < pCsd->unioncsd.road.AssocOADNum; i++)
		{			
			if(i < MAX_ASSOC_OAD_NUM)
			{
				__memcpy((void*)(pCsd->unioncsd.road.AssocOAD + i), (void*)(pRxApdu + proclen), sizeof(OAD_s));
				proclen += sizeof(OAD_s);
				oi = pCsd->unioncsd.road.AssocOAD[i].OI;
				Swap((U8*)&(oi), 2);
			}
			else
			{
				proclen += sizeof(OAD_s);
				fullFlag = TRUE;
			}
		}

		if(TRUE == fullFlag)
		{
			pCsd->unioncsd.road.AssocOADNum = MAX_ASSOC_OAD_NUM;
		}
	}
	else
	{
		return FALSE;
	}

	*pProcLen = proclen;
	
	return TRUE;
}

U8 DLT698Set698data_SID(U8* pRxApdu, SID_s* pSid, U16* pProcLen)
{
	U16 proclen = 0;
    
	__memcpy((void*)&(pSid->tag), (void*)(pRxApdu + proclen), sizeof(U16));
	proclen += sizeof(U16);
	
	pSid->len = pRxApdu[proclen++];
	__memcpy((void*)(pSid->data), (void*)(pRxApdu + proclen), pSid->len);
	proclen += pSid->len;
	
	*pProcLen = proclen;
	
	return TRUE;
}

U8 DLT698Set698data_SID_MAC(U8* pRxApdu, SID_MAC_s* pSidMac, U16* pProcLen)
{
	U16 proclen = 0;
    
	__memcpy((void*)&(pSidMac->sid.tag), (void*)(pRxApdu + proclen), sizeof(U16));
	proclen += sizeof(U16);
	
	pSidMac->sid.len = pRxApdu[proclen++];
	__memcpy((void*)(pSidMac->sid.data), (void*)(pRxApdu + proclen), pSidMac->sid.len);
	proclen += pSidMac->sid.len;

	pSidMac->mac.len = pRxApdu[proclen++];
	__memcpy((void*)(pSidMac->mac.data), (void*)(pRxApdu + proclen), pSidMac->mac.len);
	proclen += pSidMac->mac.len;
	
	*pProcLen = proclen;

	//SysDebugHex_r("SID_MAC tag: ", (void*)&(pSidMac->sid.tag), sizeof(U16));
	//SysDebugHex_r("SID_MAC attach data: ", (void*)(pSidMac->sid.data), pSidMac->sid.len);
	//SysDebugHex_r("SID_MAC mac: ", (void*)(pSidMac->mac.data), pSidMac->mac.len);
	
	return TRUE;
}

U8 DLT698Set698data_RCSD(U8* pRxApdu, RCSD_s* pRcsd, U16* pProcLen)
{
	U8	Flag = TRUE;
	U16	proclen = 0;
	U8	i = 0;
	CSD_s  csd = {0};
	U8 fullFlag = FALSE;
	U16 templen = 0;
	
	__memcpy((void*)&(pRcsd->num), (void*)(pRxApdu + proclen), 1);
	proclen++;

	//SysDebugStr_r(SOURCE, "DLT698Set698data_RCSD num: %d", prcsd->num);
	
	for(i = 0; i < pRcsd->num; i++)
	{
		//SysDebugStr_r(SOURCE, "DLT698Set698data_RCSD index: %d", i);
		
		if(i < RCSD_RSD_NUM)
		{
			Flag = DLT698Set698data_CSD(pRxApdu + proclen, pRcsd->csd + i, &templen);
		}
		else
		{
			Flag = DLT698Set698data_CSD(pRxApdu + proclen, &csd, &templen);
			fullFlag = TRUE;
		}

		proclen += templen;
	}

	if(TRUE == fullFlag)
	{
		//SysDebugStr_r(SOURCE, "DLT698Set698data_RCSD num(%d) > RCSD_RSD_NUM(%d)",prcsd->num,RCSD_RSD_NUM);
		pRcsd->num = RCSD_RSD_NUM;
	}

	*pProcLen = proclen;
	return Flag;
}

static U8 DLT698ParseData(U8 *pRxApdu, void *pData, DLT698_data_s *pParseData)
{
    U8  Flag = TRUE;
    U16 procLen = 0;  // 处理的缓冲区长度
    U16 tmpLen = 0;   // 接口函数处理报文长度
	
	pParseData->dataType = pRxApdu[procLen++];

	switch(pParseData->dataType)
	{
		case e_NULL:
		{
			break;
		}
		
		case ARRAY:
		{
			pParseData->dataNum = pRxApdu[procLen++];
			break;
		}
		
		case STRUCTURE:
		{
			pParseData->dataNum = pRxApdu[procLen++];
			break;
		}

		case e_BOOL:
		case INTEGER:
		case UNSIGNED:
		case ENUM:
		{
			if(!pData)
                return FALSE;
			pParseData->dataLen = 1;
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			break;
		}

		case BIT_STRING:  // 位串
		{
			if(!pData)  
                return FALSE;
			pParseData->dataLen = pRxApdu[procLen++];
			tmpLen = pParseData->dataLen;

			if(pParseData->dataLen % 8 > 0)
			{
				tmpLen = pParseData->dataLen / 8 + 1;
			}
			else
			{
				tmpLen = pParseData->dataLen / 8;
			}
			
			__memcpy(pData, (void*)(pRxApdu + procLen), tmpLen);
			procLen += tmpLen;
			break;
		}

		case DOUBLE_LONG:
		case DOUBLE_LONG_UNSIGNED:
		case FLOAT32:
		{
			if(!pData)
                return FALSE;
            
			pParseData->dataLen = 4;
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			Swap((U8*)pData, pParseData->dataLen);
			
			break;
		}

		case OCTET_STRING:
		case VISIBLE_STRING:
		case UTF8_STRING:
		case MAC:
		case RN:
		{
			if(!pData)
                return FALSE;
			pParseData->dataLen = pRxApdu[procLen++];
			
			U8 len = (U8)pParseData->dataLen;
			
			if(0x01 == ((len >> 7) & 0x01))
			{
				len = pRxApdu[procLen++] << 8;
				len += pRxApdu[procLen++];

				pParseData->dataLen = len;
			}
			
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			break;
		}

		case TSA:
		{
			if(!pData)
                return FALSE;
			pParseData->dataLen = pRxApdu[procLen++];
			
			pParseData->dataLen = pRxApdu[procLen++] + 1;
					
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;

			break;
		}

		case DATA_LONG:
		case LONG_UNSIGNED:
		{
			if(!pData)
                return FALSE;
			pParseData->dataLen = 2;
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			Swap((U8*)pData, pParseData->dataLen);
			break;
		}
		
		case LONG64:
		case LONG64_UNSIGNED:
		case FLOAT64:
		{
			if(!pData)
                return FALSE;
			pParseData->dataLen = 8;
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			Swap((U8*)pData, pParseData->dataLen);
			break;
		}

		case DATE_TIME:
		{
			if(!pData)
                return FALSE; 
            
			obj_date_time date_time = {0};
			pParseData->dataLen = sizeof(obj_date_time);
			__memcpy((void*)&date_time, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			Swap((U8*)&(date_time.year), 2);
			Swap((U8*)&(date_time.milliseconds), 2);
			__memcpy(pData, (void*)&date_time, pParseData->dataLen);
			break;
		}

		case DATE_Type:
		{
			if(!pData)
                return FALSE;
            
			date_s date = {0};
			pParseData->dataLen = sizeof(date_s);
			__memcpy((void*)&date, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			Swap((U8*)&(date.year), 2);
			__memcpy(pData, (void*)&date, pParseData->dataLen);
			break;
		}

		case TIME:
		{
			if(!pData)
                return FALSE;
            
			time_s time = {0};
			pParseData->dataLen = sizeof(time_s);
			__memcpy((void*)&time, (void*)(pRxApdu + procLen), pParseData->dataLen);
			__memcpy(pData, (void*)&time, pParseData->dataLen);
			procLen += pParseData->dataLen;
			break;
		}

		case DATE_TIME_S:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_DATE_TIME_S(pRxApdu + procLen, (date_time_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case OI:
		{
			if(!pData)
                return FALSE;
            
			pParseData->dataLen = 2;
			__memcpy((void *)pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			Swap((U8 *)pData, pParseData->dataLen);
			break;
		}
		
		case OAD:
		{
			if(!pData)
                return FALSE;
			pParseData->dataLen = sizeof(OAD_s);
			__memcpy(pData, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
			break;
		}

		case ROAD:
		{
			if(!pData)
                return FALSE;
            
			Flag = DLT698Set698data_ROAD(pRxApdu + procLen, (ROAD_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case TI:
		{
			if(!pData)
                return FALSE;
            
			Flag = DLT698Set698data_TI(pRxApdu + procLen, (TI_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case REGION:
		{
			break;
		}

		case SCALER_UNIT:
		{
			if(!pData)
                return FALSE;
            
			scaler_unit_s* pScaler = (scaler_unit_s*)pData;
			pParseData->dataLen = sizeof(scaler_unit_s);
			__memcpy((void*)pScaler, (void*)(pRxApdu + procLen), pParseData->dataLen);
			procLen += pParseData->dataLen;
            
			break;
		}

		case RSD:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_RSD(pRxApdu + procLen, (RSD_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case CSD:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_CSD(pRxApdu + procLen, (CSD_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case MS:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_MS(pRxApdu + procLen, (MS_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case SID:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_SID(pRxApdu + procLen, (SID_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case SID_MAC:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_SID_MAC(pRxApdu + procLen, (SID_MAC_s*)pData, &tmpLen);
			procLen += tmpLen;
			break;
		}

		case COMDCB:
		{
			break;
		}

		case RCSD:
		{
			if(!pData)
                return FALSE;
			Flag = DLT698Set698data_RCSD(pRxApdu + procLen, (RCSD_s*)pData, &tmpLen);
			procLen += tmpLen;
			
			break;
		}
		default:
			return FALSE;
	}

	pParseData->procLen = procLen;

	//SysDebugStr_r(SOURCE, "datatype %d, procLen %d", pST->datatype, pST->procLen);
	
	return Flag;
}

U16 DLT698ConstructData_RSD(rsd_type_e RsdType,U8* pTxApdu            , RSD_s *pRsd)
{
    U16  proclen = 0;//添加长度
    U8   ii;

    pTxApdu[proclen++] = pRsd->choice;

    switch(pRsd->choice)
    {
        case 0:
            break;
        case 1:            
            pTxApdu[proclen++] = (U8)(pRsd->rsdselector.select1.oad.OI >> 8);
            pTxApdu[proclen++] = (U8)(pRsd->rsdselector.select1.oad.OI & 0xFF);
            pTxApdu[proclen++] = pRsd->rsdselector.select1.oad.AttriId | (pRsd->rsdselector.select1.oad.AttriFeature << 5);
            pTxApdu[proclen++] = pRsd->rsdselector.select1.oad.AttriElementIdx;

            pTxApdu[proclen++] = pRsd->rsdselector.select1.data.dataType;
            __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select1.data.data, pRsd->rsdselector.select1.data.dataLen);
            proclen += pRsd->rsdselector.select1.data.dataLen;
            break;
        case 2:
            // OAD
            pTxApdu[proclen++] = (U8)(pRsd->rsdselector.select2.oad.OI >> 8);
            pTxApdu[proclen++] = (U8)(pRsd->rsdselector.select2.oad.OI & 0xFF);
            pTxApdu[proclen++] = pRsd->rsdselector.select2.oad.AttriId | (pRsd->rsdselector.select2.oad.AttriFeature << 5);
            pTxApdu[proclen++] = pRsd->rsdselector.select2.oad.AttriElementIdx;

            // 起始值
            pTxApdu[proclen++] = pRsd->rsdselector.select2.dataStart.dataType;
            __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select2.dataStart.data, pRsd->rsdselector.select2.dataStart.dataLen);
            proclen += pRsd->rsdselector.select2.dataStart.dataLen;

            // 结束值
            pTxApdu[proclen++] = pRsd->rsdselector.select2.dataEnd.dataType;
            __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select2.dataEnd.data, pRsd->rsdselector.select2.dataEnd.dataLen);
            proclen += pRsd->rsdselector.select2.dataEnd.dataLen;

            // 数据间隔            
            pTxApdu[proclen++] = pRsd->rsdselector.select2.dataInterval.dataType;
            __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select2.dataInterval.data, pRsd->rsdselector.select2.dataInterval.dataLen);
            proclen += pRsd->rsdselector.select2.dataInterval.dataLen;
            
            break;
        case 3:
            pTxApdu[proclen++] = pRsd->rsdselector.select3.num;

            for(ii=0; ii<pRsd->rsdselector.select3.num; ii++)
            {
                // OAD
                pTxApdu[proclen++] = (U8)(pRsd->rsdselector.select3.rsdSelector2[ii].oad.OI >> 8);
                pTxApdu[proclen++] = (U8)(pRsd->rsdselector.select3.rsdSelector2[ii].oad.OI & 0xFF);
                pTxApdu[proclen++] = pRsd->rsdselector.select3.rsdSelector2[ii].oad.AttriId | (pRsd->rsdselector.select3.rsdSelector2[ii].oad.AttriFeature << 5);
                pTxApdu[proclen++] = pRsd->rsdselector.select3.rsdSelector2[ii].oad.AttriElementIdx;

                // 起始值
                pTxApdu[proclen++] = pRsd->rsdselector.select3.rsdSelector2[ii].dataStart.dataType;
                __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select3.rsdSelector2[ii].dataStart.data, pRsd->rsdselector.select3.rsdSelector2[ii].dataStart.dataLen);
                proclen += pRsd->rsdselector.select3.rsdSelector2[ii].dataStart.dataLen;

                // 结束值
                pTxApdu[proclen++] = pRsd->rsdselector.select3.rsdSelector2[ii].dataEnd.dataType;
                __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select3.rsdSelector2[ii].dataEnd.data, pRsd->rsdselector.select3.rsdSelector2[ii].dataEnd.dataLen);
                proclen += pRsd->rsdselector.select3.rsdSelector2[ii].dataEnd.dataLen;

                // 数据间隔            
                pTxApdu[proclen++] = pRsd->rsdselector.select3.rsdSelector2[ii].dataInterval.dataType;
                __memcpy(&pTxApdu[proclen], pRsd->rsdselector.select3.rsdSelector2[ii].dataInterval.data, pRsd->rsdselector.select3.rsdSelector2[ii].dataInterval.dataLen);
                proclen += pRsd->rsdselector.select3.rsdSelector2[ii].dataInterval.dataLen;

            }
            break;
        case 4:
            proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select4.collectStarttime), 0);
            
            proclen += DLT698ConstructData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select4.meterSet), 0);
            break;
        case 5:
            proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select5.collectSavetime), 0);
            
            proclen += DLT698ConstructData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select5.meterSet), 0);
            break;
        case 6:
            proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select6.collectStarttime), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select6.collectEndtime), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], TI, 0, (void*)(&pRsd->rsdselector.select6.timeInterval), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select6.meterSet), 0);
            
            break;
        case 7:
            if(RsdType == e_NORMAL_RSD)
            {
                proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select7.collectSave_s), 0);
                proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select7.collectSave_e), 0);
                proclen += DLT698ConstructData(&pTxApdu[proclen], TI, 0, (void*)(&pRsd->rsdselector.select7.timeInterval), 0);
                proclen += DLT698ConstructData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select7.meterSet), 0);
            }
            else if(RsdType == e_REPORT_SIMPLIFY_RSD)
            {
                proclen += DLT698ConstructSimplifyData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select7.collectSave_s), 0);
                proclen += DLT698ConstructSimplifyData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select7.collectSave_e), 0);
                proclen += DLT698ConstructSimplifyData(&pTxApdu[proclen], TI, 0, (void*)(&pRsd->rsdselector.select7.timeInterval), 0);
                proclen += DLT698ConstructSimplifyData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select7.meterSet), 0);
            }
            break;
        case 8:
            proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select8.collectSucc_s), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], DATE_TIME_S, 0, (void*)(&pRsd->rsdselector.select8.collectSucc_e), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], TI, 0, (void*)(&pRsd->rsdselector.select8.timeInterval), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select8.meterSet), 0);
            
            break;
        case 9:
            proclen += DLT698ConstructData(&pTxApdu[proclen], UNSIGNED, 0, (void*)(&pRsd->rsdselector.select9.lastNum), 0);
            break;
        case 10:
            proclen += DLT698ConstructData(&pTxApdu[proclen], UNSIGNED, 0, (void*)(&pRsd->rsdselector.select10.lastNum), 0);
            proclen += DLT698ConstructData(&pTxApdu[proclen], MS, 0, (void*)(&pRsd->rsdselector.select10.meterSet), 0);
            break;
        default:
            break;
    }

    return proclen;
}

U16 DLT698ConstructData(U8* pTxApdu            , data_type_e datatype, U16 datalen, void* pElement, U8 dataNum)
{
	U16  proclen = 0;//添加长度

	pTxApdu[proclen++] = datatype;

	switch(datatype)
	{
		case e_NULL:
		{
			break;
		}
		
		case ARRAY:
		{
			pTxApdu[proclen++] = dataNum;
			break;
		}
		
		case STRUCTURE:
		{
			pTxApdu[proclen++] = dataNum;
			break;
		}

		case e_BOOL:
		case INTEGER:
		case UNSIGNED:
		case ENUM:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 1);
			proclen += 1;
			break;
		}

		case BIT_STRING:
		{
			S8 tmplen = 0;
			
			if (!pElement) return 0;
			if(datalen%8)
			{
				tmplen = datalen/8+1;
			}
			else
			{
				tmplen = datalen/8;
			}

			pTxApdu[proclen++] = datalen;
			__memcpy((void*)(pTxApdu + proclen), pElement, tmplen);
			proclen += tmplen;
			break;
		}

		case DOUBLE_LONG:
		case DOUBLE_LONG_UNSIGNED:
		case FLOAT32:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 4);
			Swap((void*)(pTxApdu + proclen), 4);
			proclen += 4;
			break;
		}

		case OCTET_STRING:
		case VISIBLE_STRING:
		case UTF8_STRING:
		case MAC:
		case RN:
		{
			if (!pElement) return 0;
			if(datalen <= 0xFF)
			{
				pTxApdu[proclen++] = datalen;
			}
			else
			{
				pTxApdu[proclen++] = 0x82;
				pTxApdu[proclen++] = datalen >> 8;
				pTxApdu[proclen++] = datalen % 256;
			}
			__memcpy((void*)(pTxApdu + proclen), pElement, datalen);
			proclen += datalen;
			break;
		}

		case TSA:
		{
			if (!pElement) return 0;
			pTxApdu[proclen++] = datalen + 1;
			pTxApdu[proclen++] = datalen - 1;
			__memcpy((void*)(pTxApdu + proclen), pElement, datalen);
			proclen += datalen;
			break;
		}
		
		case DATA_LONG:
		case LONG_UNSIGNED:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 2);
			Swap((void*)(pTxApdu + proclen), 2);
			proclen += 2;
			break;
		}
		
		case LONG64:
		case LONG64_UNSIGNED:
		case FLOAT64:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 8);
			Swap((void*)(pTxApdu + proclen), 8);
			proclen += 8;
			break;
		}

		case DATE_TIME:
		{
			if (!pElement) return 0;
			obj_date_time date_time = {0};
			__memcpy((void*)&date_time, pElement, sizeof(obj_date_time));
			Swap((U8*)&(date_time.year), 2);
			Swap((U8*)&(date_time.milliseconds), 2);
			__memcpy((void*)(pTxApdu + proclen), &date_time, sizeof(obj_date_time));
			proclen += sizeof(obj_date_time);
			break;
		}

		case DATE_Type:
		{
			if (!pElement) return 0;
			date_s date = {0};
			__memcpy(&date, pElement, sizeof(date_s));
			Swap((U8*)&(date.year), 2);
			__memcpy(pTxApdu, &date, sizeof(date_s));
			proclen += sizeof(date_s);
			break;
		}

		case TIME:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, sizeof(time_s));
			proclen += sizeof(time_s);
			break;
		}

		case DATE_TIME_S:
		{
			if (!pElement) return 0;
			date_time_s data_time_s = {0};
			__memcpy(&data_time_s, pElement, sizeof(date_time_s));
			Swap((U8*)&(data_time_s.year), 2);
			__memcpy((void*)(pTxApdu + proclen), &data_time_s, sizeof(date_time_s));
			proclen+=sizeof(date_time_s);
			break;
		}

        /*
		case data_atime:
		{
			if (!pElement) return 0;
			STIME STime;
			proclen --;
			pTxApdu[proclen++] = data_date_time_s;
			OsPackAtime2Stime_r((ATIME *)pElement, &STime);
			pTxApdu[proclen++] = (STime.Year>>8)&0xFF;
			pTxApdu[proclen++] = STime.Year&0xFF;
			pTxApdu[proclen++] = STime.Month&0xFF;
			pTxApdu[proclen++] = STime.Day&0xFF;
			pTxApdu[proclen++] = STime.Hour&0xFF;
			pTxApdu[proclen++] = STime.Minute&0xFF;
			pTxApdu[proclen++] = STime.Second&0xFF;
			
			break;
		}
		*/
		case OI:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 2);
			Swap((U8 *)(pTxApdu + proclen), 2);
			proclen+=2;
			break;
		}
		
		case OAD:
		{
			if (!pElement) return 0;
			OAD_s oad = {0};
			
			__memcpy(&oad, pElement, sizeof(OAD_s));
			Swap((U8*)&(oad.OI), 2);
			__memcpy((void*)(pTxApdu + proclen), &oad, sizeof(OAD_s));
			proclen += sizeof(OAD_s);
			break;
		}

		case ROAD:
		{
			if (!pElement) return 0;
			ROAD_s road;
			S8 i = 0;
			memset((void*) &road, 0xFF, sizeof(ROAD_s));
			
			__memcpy(&road, pElement, sizeof(ROAD_s));
			
			proclen+=DLT698ConstructData(pTxApdu + proclen, OAD, sizeof(OAD_s), (void *)&(road.OAD), 0);
			pTxApdu[proclen++] = road.AssocOADNum;

			for(i = 0; i < road.AssocOADNum; i++)
			{
				proclen+=DLT698ConstructData(pTxApdu + proclen, OAD, sizeof(OAD_s), (void *)&(road.AssocOAD[i]), 0);
			}
			
			break;
		}

		case TI:
		{
			if (!pElement) return 0;
			TI_s ti = {0};
			__memcpy(&ti, pElement, sizeof(TI_s));
			//SysDebugStr_r(SOURCE, "TI: unit %d, interval %d", ti.unit,ti.interval);
			Swap((U8*)&(ti.InterValue), 2);
			__memcpy((void*)(pTxApdu + proclen), &ti, sizeof(TI_s));
			proclen += sizeof(TI_s);
			break;
		}

		case REGION:
		{
			break;
		}

		case SCALER_UNIT:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, sizeof(scaler_unit_s));
			proclen += sizeof(scaler_unit_s);
			break;
		}

		case RSD:
		{
			break;
		}

		case CSD:
		{
			if (!pElement) return 0;
			CSD_s  csd = {0};
			__memcpy(&csd, pElement, sizeof(CSD_s));

			__memcpy((void*)(pTxApdu + proclen), (void*)&(csd.choice), 1);
			proclen += 1;

			if(0 == csd.choice)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(csd.unioncsd.oad), sizeof(OAD_s));
				proclen += sizeof(OAD_s);
			}
			else if(1 == csd.choice)
			{
				ROAD_s road;
				S8 i = 0;
                
				memset((void*) &road, 0xFF, sizeof(ROAD_s));
				
				__memcpy(&road, &(csd.unioncsd.road), sizeof(ROAD_s));
				
				__memcpy((void*)(pTxApdu + proclen), (void *)&(road.OAD), sizeof(OAD_s));
				proclen += sizeof(OAD_s);
				
				pTxApdu[proclen++] = road.AssocOADNum;

				for(i = 0; i < road.AssocOADNum; i++)
				{
					__memcpy((void*)(pTxApdu + proclen), (void *)&(road.AssocOAD[i]), sizeof(OAD_s));
					proclen += sizeof(OAD_s);
				}
			}
			else
			{
				app_printf("DLT698Get698data CSD CHOICE ERR!!!\n");		
			}
			break;
		}

		case MS:
		{
			if (!pElement) return 0;
			MS_s ms = {0};
			__memcpy(&ms, pElement, sizeof(MS_s));

			__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.MS_type), 1);
			proclen += 1;

			S8 i =0;

			if(1 == ms.MS_type)
			{
				break;
			}
			else if(2 == ms.MS_type)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.Ms.userTypeUnit.userTypeNum), 1);
				proclen += 1;

				for(i = 0; i < ms.Ms.userTypeUnit.userTypeNum; i++)
				{
					proclen += DLT698ConstructData(pTxApdu + proclen, UNSIGNED, 1, (void *)&(ms.Ms.userTypeUnit.userType[i]), 0);
				}
			}
			else if(3 == ms.MS_type)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.Ms.userAddrUnit.userAddrNum), 1);
				proclen += 1;

				for(i = 0; i < ms.Ms.userAddrUnit.userAddrNum; i++)
				{
					proclen += 
                      DLT698ConstructData(pTxApdu + proclen, TSA, ms.Ms.userAddrUnit.userAddr[i].saLen, (void *)(ms.Ms.userAddrUnit.userAddr[i].saValue), 0);
				}
			}
			else if(4 == ms.MS_type)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.Ms.ConfigUnit.configNum), 1);
				proclen += 1;

				for(i = 0; i < ms.Ms.ConfigUnit.configNum; i++)
				{
					proclen += DLT698ConstructData(pTxApdu + proclen, LONG_UNSIGNED, 2, (void *)&(ms.Ms.ConfigUnit.configIdx[i]), 0);
				}
			}
			else
			{
				app_printf("DLT698Get698data MS CHOICE unsupport!!!\n");
			}
			break;
		}

		case SID:
		{
			break;
		}

		case SID_MAC:
		{
			break;
		}

		case COMDCB:
		{
		    if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, sizeof(CMMDCB_s));
			proclen += sizeof(CMMDCB_s);
			break;
		}

		case RCSD:
		{
			break;
		}

		default:
			return FALSE;
	}
	
	return proclen;
}

static U16 DLT698ConstructSimplifyData(U8 *pTxApdu, data_type_e datatype, U16 datalen, void* pElement, U8 dataNum)
{
	U16  proclen = 0;//添加长度

	//pTxApdu[proclen++] = datatype;

	switch(datatype)
	{
		case e_NULL:
		{
			break;
		}
		
		case ARRAY:
		{
			pTxApdu[proclen++] = dataNum;
			break;
		}
		
		case STRUCTURE:
		{
			pTxApdu[proclen++] = dataNum;
			break;
		}

		case e_BOOL:
		case INTEGER:
		case UNSIGNED:
		case ENUM:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 1);
			proclen += 1;
			break;
		}

		case BIT_STRING:
		{
			S8 tmplen = 0;
			
			if (!pElement) return 0;
			if(datalen%8)
			{
				tmplen = datalen/8+1;
			}
			else
			{
				tmplen = datalen/8;
			}

			pTxApdu[proclen++] = datalen;
			__memcpy((void*)(pTxApdu + proclen), pElement, tmplen);
			proclen += tmplen;
			break;
		}

		case DOUBLE_LONG:
		case DOUBLE_LONG_UNSIGNED:
		case FLOAT32:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 4);
			Swap((void*)(pTxApdu + proclen), 4);
			proclen += 4;
			break;
		}

		case OCTET_STRING:
		case VISIBLE_STRING:
		case UTF8_STRING:
		case MAC:
		case RN:
		{
			if (!pElement) return 0;
			if(datalen <= 0xFF)
			{
				pTxApdu[proclen++] = datalen;
			}
			else
			{
				pTxApdu[proclen++] = 0x82;
				pTxApdu[proclen++] = datalen >> 8;
				pTxApdu[proclen++] = datalen % 256;
			}
			__memcpy((void*)(pTxApdu + proclen), pElement, datalen);
			proclen += datalen;
			break;
		}

		case TSA:
		{
			if (!pElement) return 0;
            app_printf("datalen = %d \n",datalen);
			pTxApdu[proclen++] = datalen + 1;
			pTxApdu[proclen++] = datalen - 1;
			__memcpy((void*)(pTxApdu + proclen), pElement, datalen);
			proclen += datalen;
			break;
		}
		
		case DATA_LONG:
		case LONG_UNSIGNED:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 2);
			Swap((void*)(pTxApdu + proclen), 2);
			proclen += 2;
			break;
		}
		
		case LONG64:
		case LONG64_UNSIGNED:
		case FLOAT64:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 8);
			Swap((void*)(pTxApdu + proclen), 8);
			proclen += 8;
			break;
		}

		case DATE_TIME:
		{
			if (!pElement) return 0;
			obj_date_time date_time = {0};
			__memcpy((void*)&date_time, pElement, sizeof(obj_date_time));
			Swap((U8*)&(date_time.year), 2);
			Swap((U8*)&(date_time.milliseconds), 2);
			__memcpy((void*)(pTxApdu + proclen), &date_time, sizeof(obj_date_time));
			proclen += sizeof(obj_date_time);
			break;
		}

		case DATE_Type:
		{
			if (!pElement) return 0;
			date_s date = {0};
			__memcpy(&date, pElement, sizeof(date_s));
			Swap((U8*)&(date.year), 2);
			__memcpy(pTxApdu, &date, sizeof(date_s));
			proclen += sizeof(date_s);
			break;
		}

		case TIME:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, sizeof(time_s));
			proclen += sizeof(time_s);
			break;
		}

		case DATE_TIME_S:
		{
			if (!pElement) return 0;
			date_time_s data_time_s = {0};
			__memcpy(&data_time_s, pElement, sizeof(date_time_s));
			Swap((U8*)&(data_time_s.year), 2);
			__memcpy((void*)(pTxApdu + proclen), &data_time_s, sizeof(date_time_s));
			proclen+=sizeof(date_time_s);
			break;
		}

        /*
		case data_atime:
		{
			if (!pElement) return 0;
			STIME STime;
			proclen --;
			pTxApdu[proclen++] = data_date_time_s;
			OsPackAtime2Stime_r((ATIME *)pElement, &STime);
			pTxApdu[proclen++] = (STime.Year>>8)&0xFF;
			pTxApdu[proclen++] = STime.Year&0xFF;
			pTxApdu[proclen++] = STime.Month&0xFF;
			pTxApdu[proclen++] = STime.Day&0xFF;
			pTxApdu[proclen++] = STime.Hour&0xFF;
			pTxApdu[proclen++] = STime.Minute&0xFF;
			pTxApdu[proclen++] = STime.Second&0xFF;
			
			break;
		}
		*/
		case OI:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, 2);
			Swap((U8 *)(pTxApdu + proclen), 2);
			proclen+=2;
			break;
		}
		
		case OAD:
		{
			if (!pElement) return 0;
			OAD_s oad = {0};
			
			__memcpy(&oad, pElement, sizeof(OAD_s));
			Swap((U8*)&(oad.OI), 2);
			__memcpy((void*)(pTxApdu + proclen), &oad, sizeof(OAD_s));
			proclen += sizeof(OAD_s);
			break;
		}

		case ROAD:
		{
			if (!pElement) return 0;
			ROAD_s road;
			S8 i = 0;
			memset((void*) &road, 0xFF, sizeof(ROAD_s));
			
			__memcpy(&road, pElement, sizeof(ROAD_s));
			
			proclen+=DLT698ConstructSimplifyData(pTxApdu + proclen, OAD, sizeof(OAD_s), (void *)&(road.OAD), 0);
			pTxApdu[proclen++] = road.AssocOADNum;

			for(i = 0; i < road.AssocOADNum; i++)
			{
				proclen+=DLT698ConstructSimplifyData(pTxApdu + proclen, OAD, sizeof(OAD_s), (void *)&(road.AssocOAD[i]), 0);
			}
			
			break;
		}

		case TI:
		{
			if (!pElement) return 0;
			TI_s ti = {0};
			__memcpy(&ti, pElement, sizeof(TI_s));
			//SysDebugStr_r(SOURCE, "TI: unit %d, interval %d", ti.unit,ti.interval);
			Swap((U8*)&(ti.InterValue), 2);
			__memcpy((void*)(pTxApdu + proclen), &ti, sizeof(TI_s));
			proclen += sizeof(TI_s);
			break;
		}

		case REGION:
		{
			break;
		}

		case SCALER_UNIT:
		{
			if (!pElement) return 0;
			__memcpy((void*)(pTxApdu + proclen), pElement, sizeof(scaler_unit_s));
			proclen += sizeof(scaler_unit_s);
			break;
		}

		case RSD:
		{
			break;
		}

		case CSD:
		{
			if (!pElement) return 0;
			CSD_s  csd = {0};
			__memcpy(&csd, pElement, sizeof(CSD_s));

			__memcpy((void*)(pTxApdu + proclen), (void*)&(csd.choice), 1);
			proclen += 1;

			if(0 == csd.choice)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(csd.unioncsd.oad), sizeof(OAD_s));
				proclen += sizeof(OAD_s);
			}
			else if(1 == csd.choice)
			{
				ROAD_s road;
				S8 i = 0;
                
				memset((void*) &road, 0xFF, sizeof(ROAD_s));
				
				__memcpy(&road, &(csd.unioncsd.road), sizeof(ROAD_s));
				
				__memcpy((void*)(pTxApdu + proclen), (void *)&(road.OAD), sizeof(OAD_s));
                Swap((U8*)(pTxApdu + proclen), 2);
				proclen += sizeof(OAD_s);
				
				pTxApdu[proclen++] = road.AssocOADNum;

				for(i = 0; i < road.AssocOADNum; i++)
				{
					__memcpy((void*)(pTxApdu + proclen), (void *)&(road.AssocOAD[i]), sizeof(OAD_s));
                    Swap((U8*)(pTxApdu + proclen), 2);
					proclen += sizeof(OAD_s);
				}
			}
			else
			{
				app_printf("DLT698Get698data CSD CHOICE ERR!!!\n");		
			}
			break;
		}

		case MS:
		{
			if (!pElement) return 0;
			MS_s ms = {0};
			__memcpy(&ms, pElement, sizeof(MS_s));

			__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.MS_type), 1);
			proclen += 1;

			S8 i =0;

			if(1 == ms.MS_type)
			{
				break;
			}
			else if(2 == ms.MS_type)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.Ms.userTypeUnit.userTypeNum), 1);
				proclen += 1;

				for(i = 0; i < ms.Ms.userTypeUnit.userTypeNum; i++)
				{
					proclen += DLT698ConstructData(pTxApdu + proclen, UNSIGNED, 1, (void *)&(ms.Ms.userTypeUnit.userType[i]), 0);
				}
			}
			else if(3 == ms.MS_type)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.Ms.userAddrUnit.userAddrNum), 1);
				proclen += 1;

				for(i = 0; i < ms.Ms.userAddrUnit.userAddrNum; i++)
				{
					proclen += 
                      DLT698ConstructSimplifyData(pTxApdu + proclen, TSA, ms.Ms.userAddrUnit.userAddr[i].saLen, (void *)(&ms.Ms.userAddrUnit.userAddr[i].saValue), 0);
				}
			}
			else if(4 == ms.MS_type)
			{
				__memcpy((void*)(pTxApdu + proclen), (void*)&(ms.Ms.ConfigUnit.configNum), 1);
				proclen += 1;

				for(i = 0; i < ms.Ms.ConfigUnit.configNum; i++)
				{
					proclen += DLT698ConstructData(pTxApdu + proclen, LONG_UNSIGNED, 2, (void *)&(ms.Ms.ConfigUnit.configIdx[i]), 0);
				}
			}
			else
			{
				app_printf("DLT698Get698data MS CHOICE unsupport!!!\n");
			}
			break;
		}

		case SID:
		{
			break;
		}

		case SID_MAC:
		{
			break;
		}

		case COMDCB:
		{
			break;
		}

		case RCSD:
		{
			break;
		}

		default:
			return FALSE;
	}
	
	return proclen;
}

static U16 DLT698GetDataLen(U8* pRxApdu)
{
    data_type_e  datatype = 0;
    U16          proclen = 0;
    U8    num = 0;
	U8    i = 0;
	U16   tmplen = 0;
	U8    choice = 0;
	datatype = pRxApdu[proclen++];
	
	switch (datatype)
	{
		case e_NULL:
			break;
		
		case ARRAY:
			num = pRxApdu[proclen++];
			
			for(i = 0; i < num; i++)
			{
				proclen += DLT698GetDataLen(pRxApdu + proclen);
			}

			break;
			
		case STRUCTURE:
			num = pRxApdu[proclen++];
			
			for(i = 0; i < num; i++)
			{
				proclen += DLT698GetDataLen(pRxApdu + proclen);
			}

			break;
			
		case e_BOOL:
		case INTEGER:
		case UNSIGNED:
		case ENUM:
			proclen++;
			break;
		
		case BIT_STRING:
			tmplen = pRxApdu[proclen++];
			proclen += (tmplen % 8 > 0) ? (tmplen / 8 + 1):(tmplen / 8); 
			break;

		case DOUBLE_LONG:
		case DOUBLE_LONG_UNSIGNED:
		case FLOAT32:
			proclen += 4;
			break;

		case OCTET_STRING:			
		case VISIBLE_STRING:
		case UTF8_STRING:
		case TSA:
		case MAC:
		case RN:
			tmplen = pRxApdu[proclen++];

			if(1 == tmplen >> 7)
			{
				U16 trueLen = 0;
				proclen++;
				__memcpy((void*)&trueLen, (void*)(pRxApdu + proclen), tmplen & 0xF);
				Swap((U8*)&trueLen, tmplen & 0xF);
				proclen += tmplen & 0xF;
			}
			
			proclen+= tmplen;
			break;
		
		case DATA_LONG:
		case LONG_UNSIGNED:
			proclen+= 2;
			break;

		case LONG64:
		case LONG64_UNSIGNED:
		case FLOAT64:
			proclen+= 8;
			break;

		case DATE_TIME:
			proclen+= sizeof(obj_date_time);
			break;

		case DATE_Type:
			proclen+= sizeof(date_s);
			break;

		case TIME:
			proclen+= sizeof(time_s);
			break;

		case DATE_TIME_S:
			proclen+= sizeof(date_time_s);
			break;

		case OI:
			proclen+= 2;
			break;

		case OAD:
		case OMD:
			proclen+= sizeof(OAD_s);
			break;

		case TI:
			proclen+= sizeof(TI_s);
			break;

		case SCALER_UNIT:
			proclen+= sizeof(scaler_unit_s);
			break;
			
		case ROAD:
			proclen+= sizeof(OAD_s);
			num = pRxApdu[proclen++];
			proclen+= num * sizeof(OAD_s);
			break;
			
		case REGION:
			proclen++;
			proclen += DLT698GetDataLen(pRxApdu+proclen);
			proclen += DLT698GetDataLen(pRxApdu+proclen);
			break;
			
		case RSD:
			choice = pRxApdu[proclen++];
			switch(choice)
			{
				case 0:
					break;

				case 1:
					proclen+=sizeof(OAD_s);
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
					break;

				case 2:
					
					proclen+=sizeof(OAD_s);
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
					break;
					
				case 3:
					num = pRxApdu[proclen++];

					for(i = 0; i < num; i++)
					{
						proclen+=sizeof(OAD_s);
						proclen+=DLT698GetDataLen(pRxApdu+proclen);
						proclen+=DLT698GetDataLen(pRxApdu+proclen);
						proclen+=DLT698GetDataLen(pRxApdu+proclen);
					}
					break;
					
				case 4:
				case 5:
					proclen+=sizeof(date_time_s);
					//针对MS进行处理
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
					break;
				case 6:
				case 7:
				case 8:
					proclen+=2*sizeof(date_time_s);
					proclen+=sizeof(TI_s);
					//针对MS进行处理
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
					break;
				case 9:
					proclen++;
				case 10:
					proclen++;
					//真对MS进行处理
					proclen+=DLT698GetDataLen(pRxApdu+proclen);
				default:
					break;
			}
			break;

		case CSD:
			choice = pRxApdu[proclen++];
			if(0 == choice)
			{
				proclen+=sizeof(OAD_s);
			}
			else if(1 == choice)
			{
				proclen+=sizeof(OAD_s);
				num = pRxApdu[proclen++];
				proclen+= num*sizeof(OAD_s);
			}
			else
			{
				//异常
			}
			break;

		case MS:
			choice = pRxApdu[proclen++];
			switch (choice)
			{
				case 0:
					break;
				case 1:
					break;
				case 2:
					num = pRxApdu[proclen++];
					proclen += num;
					break;
				case 3:
					num = pRxApdu[proclen++];
					for(i = 0; i < num; i++)
					{
						proclen += DLT698GetDataLen(pRxApdu+proclen);
					}
					break;
				case 4:
					num = pRxApdu[proclen++];
					proclen += num*2;
					break;
				case 5:
				case 6:
				case 7:
					num = pRxApdu[proclen++];
					for(i = 0; i < num; i++)
					{
						proclen++;
						proclen += DLT698GetDataLen(pRxApdu+proclen);
						proclen += DLT698GetDataLen(pRxApdu+proclen);
					}
					break;
				default:
					break;
			}
			break;

		case SID:
			proclen += 4;
			tmplen = pRxApdu[proclen++];
			proclen += tmplen;
			break;

		case SID_MAC:
			//SID
			proclen += 4;
			tmplen = pRxApdu[proclen++];
			proclen += tmplen;
			//MAC
			tmplen = pRxApdu[proclen++];
			proclen += tmplen;
			break;

		case COMDCB:
			proclen += 5;
			break;

		case RCSD:
			num = pRxApdu[proclen++];
			for(i = 0; i < num; i++)
			{
				choice = pRxApdu[proclen++];
				if(0 == choice)
				{
					proclen+=sizeof(OAD_s);
				}
				else if(1 == choice)
				{
					proclen+=sizeof(OAD_s);
					num = pRxApdu[proclen++];
					proclen+= num*sizeof(OAD_s);
				}
				else
				{
					//异常
				}
			}
			break;
		
		default:
			break;
	}

	return proclen - 1;
}

static void Swap(U8* str, int len)
{
	char c;
	int i;

	for(i=0; i<len/2; i++)
	{
		c =str[len-1-i];
		str[len-1-i] = str[i];
		str[i] = c;
	}
}
