#include "ZCsystem.h"
#include "app_cmp_version.h"
#include "SchTask.h"
#include "app_dltpro.h"
#include "printf_zc.h"
#include "app.h"

#ifdef CMP_VERSION
U8 MCUTYPE[MCUTYPELEN] = {'2','5','Q','1','6','0','0','0','0','0'};
SHA1Context SHA1ContextTemp;
extern board_cfg_t BoardCfg_t;

void soft_version_packet(QUERY_SOFTVERSION_t *SoftVer)
{
    SoftVer->CPUSeq = 0;
    __memcpy(SoftVer->VendorCode, DefSetInfo.OutMFVersion.ucVendorCode, 2);
    SoftVer->Day = BCDTOU8(DefSetInfo.OutMFVersion.ucDay);
    SoftVer->Month = BCDTOU8(DefSetInfo.OutMFVersion.ucMonth);

    if(PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
    {
        SoftVer->Year = 2000 + BCDTOU8(DefSetInfo.OutMFVersion.ucYear);
        __memcpy(SoftVer->Res2, DefSetInfo.Hardwarefeature_t.HardwareVer, 2);
    }
    else
    {
        SoftVer->Year = BCDTOU8(DefSetInfo.OutMFVersion.ucYear);
        memset(SoftVer->Res2, 0x00, 2);
    }

    __memcpy(SoftVer->SoftVersion, DefSetInfo.OutMFVersion.ucVersionNum, 2);
    SoftVer->MCUTypeLen = MCUTYPELEN;
    __memcpy(SoftVer->MCUType, MCUTYPE, MCUTYPELEN);
}

U8 query_code_sha1(QUERY_CODE_SHA1_t *QCodeSHA1, SHA1Context *context, uint8_t Message_Digest[SHA1HashSize])
{
    uint32_t  state;
    uint32_t  DstImageAddr;
    uint32_t  ReadImageAddr;//读取数据做偏移
    U32       err;
    U8        RemainAddr;
    U16       OffLen;
    uint32_t  Offaddr;

    printf_s("CPUSeq    = %d\n", QCodeSHA1->CPUSeq);
    printf_s("StartAddr = %p\n", QCodeSHA1->StartAddr);
    printf_s("DataLen   = %d\n", QCodeSHA1->DataLen);
    printf_s("IMAGE0_ADDR = %p\n", READ_IMAGE0_ADDR);
    QCodeSHA1->StartAddr += 0x060000;

    RemainAddr = (QCodeSHA1->StartAddr % 4);
    OffLen = QCodeSHA1->DataLen + 4;
    Offaddr = QCodeSHA1->StartAddr - RemainAddr;
    printf_s("RemainAddr = %d\n", RemainAddr);
    printf_s("OffLen     = %d\n", OffLen);
    printf_s("Offaddr    = %p\n", Offaddr);

    if(QCodeSHA1->CPUSeq != 0)
    {
        memset(FileUpgradeData, 0xff, QCodeSHA1->DataLen);
    }
    if(QCodeSHA1->StartAddr < READ_IMAGE0_ADDR)//||QCodeSHA1.StartAddr < BOARD_CFG_ADDR)
    {
        memset(FileUpgradeData, 0xff, QCodeSHA1->DataLen);
    }
    else if(QCodeSHA1->StartAddr >= READ_IMAGE0_ADDR)
    {
        if(QCodeSHA1->StartAddr >= READ_IMAGE0_ADDR && QCodeSHA1->StartAddr < READ_IMAGE1_ADDR)
        {
            //ReadImageAddr = QCodeSHA1->StartAddr - READ_IMAGE0_ADDR;
            ReadImageAddr = Offaddr - READ_IMAGE0_ADDR;
            state = zc_flash_read(FLASH, (U32)BOARD_CFG, (U32*)&BoardCfg_t, sizeof(board_cfg_t));
            if(BoardCfg_t.active == 2)
            {
                DstImageAddr = IMAGE1_ADDR;
            }
            else
            {
                DstImageAddr = IMAGE0_ADDR;
            }
            ReadImageAddr = ReadImageAddr + DstImageAddr;
            //state = zc_flash_read(flash, ReadImageAddr, (U32 *)&FileUpgradeData, QCodeSHA1->DataLen);
            state = zc_flash_read(FLASH, ReadImageAddr, (U32*)&FileUpgradeData, OffLen);
            app_printf("state %d,StartAddr 0x%08x,ReadImageAddr 0x%08x,DataLen %d \n", state, QCodeSHA1->StartAddr, ReadImageAddr, QCodeSHA1->DataLen);
        }
        else
        {
            memset(FileUpgradeData, 0xff, QCodeSHA1->DataLen);
        }
    }

    SHA1Reset(context);
    SHA1Input(context, FileUpgradeData + RemainAddr, QCodeSHA1->DataLen);
    err = SHA1Result(context, Message_Digest);
    app_printf("SHA1Result:%d\n", err);
    return TRUE;
}

#if defined(STATIC_NODE)
static void read_soft_version_packet(U8 *ValidData, U8 ValidDataLen, U8 *UpData, U8 *UpDataLen)//ReadSoftVersionPacket
{
    app_printf("ReadSoftVersionPacket ->\n");
    dump_buf(ValidData, ValidDataLen);
    __memcpy(UpData, ValidData, PREAMBLE_CHAR_COUNT);
    (*UpDataLen) += PREAMBLE_CHAR_COUNT;

    QUERY_SOFTVERSION_t SoftVer;
    soft_version_packet(&SoftVer);
    __memcpy(UpData + *UpDataLen, (U8*)&SoftVer, sizeof(QUERY_SOFTVERSION_t));
    *UpDataLen += sizeof(QUERY_SOFTVERSION_t);
}

static void read_query_code_sha1(U8 *ValidData, U8 ValidDataLen, U8 *UpData, U8 *UpDataLen)//ReadQueryCodeSHA1
{
    app_printf("ReadQueryCodeSHA1 ->\n");
    dump_buf(ValidData, ValidDataLen);
    __memcpy(UpData, ValidData, PREAMBLE_CHAR_COUNT);
    (*UpDataLen) += PREAMBLE_CHAR_COUNT;
    QUERY_CODE_SHA1_t QCodeSHA1;
    uint8_t Message_Digest[SHA1HashSize];
    __memcpy((U8*)&QCodeSHA1, (U8*)&ValidData[4], sizeof(QUERY_CODE_SHA1_t));
    query_code_sha1(&QCodeSHA1, &SHA1ContextTemp, Message_Digest);
    __memcpy(UpData + *UpDataLen, Message_Digest, sizeof(Message_Digest));
    *UpDataLen += sizeof(Message_Digest);
}

const DLT_07_PROTOCAL CmpVersionEx645_HN[] =
{
    {0x11 , {0x01, 0x00, 0x90, 0x04}  , read_soft_version_packet     ,"read_soft_version_packet"},
    {0x11 , {0x02, 0x00, 0x90, 0x04}  , read_query_code_sha1         ,"read_query_code_sha1"},
};

const DLT_07_PROTOCAL CmpVersionEx645_BJ[] =
{
    {0x11 , {0x01, 0x05, 0x90, 0x04}  , read_soft_version_packet     ,"read_soft_version_packet"},
    {0x11 , {0x02, 0x05, 0x90, 0x04}  , read_query_code_sha1         ,"read_query_code_sha1"},
};

void pro_version_cmp(U8 Ctrl, U8 *Addr, U8 *Dataunit, U8 Dataunitlen, U8 *Updata, U8 Updatalen, U8 *Sendbuf, U8 *Sendbuflen)
{
    U8 i;
    DLT_07_PROTOCAL *CmpVersionEx645 = NULL;
    INT8U array_size = 0;

    if(PROVINCE_HUNAN == app_ext_info.province_code) //todo: PROVINCE_HUNAN
    {
        CmpVersionEx645 = (DLT_07_PROTOCAL *)CmpVersionEx645_HN;
        array_size = sizeof(CmpVersionEx645_HN);
    }
    else
    {
        CmpVersionEx645 = (DLT_07_PROTOCAL * )CmpVersionEx645_BJ;
        array_size = sizeof(CmpVersionEx645_BJ);
    }

    for(i = 0; i < (array_size / sizeof(DLT_07_PROTOCAL)); i++)
    {
        if(Ctrl == CmpVersionEx645[i].ControlData && 0 == memcmp(Dataunit, CmpVersionEx645[i].DataArea, (Dataunitlen > 4 ? 4 : Dataunitlen)))
        {
            CmpVersionEx645[i].Func(Dataunit, Dataunitlen, Updata, &Updatalen);
            break;
        }
    }
    if(i != array_size / sizeof(DLT_07_PROTOCAL))
    {
        Add0x33ForData(Updata, Updatalen);
        Packet645Frame(Sendbuf, Sendbuflen, Addr, 0x91, Updata, Updatalen);
    }
}

#endif

#endif

