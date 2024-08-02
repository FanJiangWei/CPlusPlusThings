/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	upgrade.c
 * Purpose:	
 * History:
 * Author : WWJ
 */
//#include <string.h>
//#include "app_upgrade.h"
#include "list.h"
#include "types.h"
#include "Trios.h"
#include "flash.h"
#include "Mbuf.h"
#include "mem_zc.h"
#include "app.h"
//#include "SchTask.h"
#include "ZCsystem.h"
#include "flash.h"
#include "app_exstate_verify.h"
//#include "printf_zc.h"
#include "datalinkdebug.h"
#include "app_upgrade_comm.h"
#include "app_upgrade_cco.h"
#include "app_dltpro.h"
#include "Version.h"
#include "crc.h"

extern board_cfg_t BoardCfg_t;
uint32_t Crc32Data;

U16  get_bit_state(U8  *pBitMapBuf, U16 BlockSeq)
{
	int j;
	j=(pBitMapBuf[BlockSeq/8] & (0x01 << (BlockSeq%8)))?1:0;
	return j;
}

void upgrade_show_bit_map(U8  *pBitMapBuf, U16 totolBlockNum)
{
	U16 i;
	U16 num=0;
	app_printf("\n-----------------------bit map--------totol:%d------------------\n",totolBlockNum);
	num=(totolBlockNum/8)+((totolBlockNum%8 != 0)?1:0);

	 for (i = 0; i < num; i++)
    {
        if (i && (i % 16) == 0)
            debug_printf(&dty, DEBUG_APP, "\n");
        debug_printf(&dty, DEBUG_APP, "%02x ", *(pBitMapBuf + i));
    }
    debug_printf(&dty, DEBUG_APP, "\n");
	//debug_printf(&dty, DEBUG_UPGRADE,"\n");
}

U16  CheckFileTransFinish(U8  *pBitMapBuf, U16 totolBlockNum)
{
    int i;
    int SucessNum;
    
    SucessNum = 0;
    for(i=0; i<totolBlockNum; i++)
    {
        if(!(pBitMapBuf[i/8] & (0x01 << (i%8))))
        {
            //return FALSE;
        }
        else
        {
            SucessNum ++;
        }
    }

    return SucessNum;
}

int32_t is_tri_image(imghdr_t *ih)
{
	char *__s = IMG_IDENT;
	char *__d = (char *)ih->ident;
    char deviceType = ih->devicetype;
	int32_t ret , retType;

	while ((ret = *__d++ - *__s) == 0 && *__s++)
		;
    
#if defined(STATIC_MASTER)
    retType = (deviceType == ZC3951CCO_UpDate_type ? 0 : 1);
#elif defined(ZC3750STA)
    retType = (deviceType == ZC3750STA_UpDate_type ? 0 : 1);
#elif defined(ZC3750CJQ2)
    retType = (deviceType == ZC3750CJQ2_UpDate_type ? 0 : 1);
#endif
    app_printf("ih->devicetype %x retType %x\n",deviceType, retType );
	return (ret == 0 && retType == 0) ? 1 : 0;   
}

int32_t is_test_image(imghdr_t *ih)
{
	char *__s = IMG_IDENT_TEST;
	char *__d = (char *)ih->ident;
	int32_t ret;

	while ((ret = *__d++ - *__s) == 0 && *__s++)
		;

	return ret == 0 ? 1 : 0;
}

int32_t check_image_block(uint32_t addr, uint32_t block_sz)
{
    uint32_t  state;
    uint32_t  DstImageAddr;
    U32   blockSeq = 0;
    U32   blockSeqMAX = 0;
    U8    ImageH[40] = {0};
    U32   checklen = 0;
    uint32_t Crc324K;
    //uint32_t Crc324Ktemp;
    int32_t  crcData;
    
    InitCRC32Table();
    if(addr == 0)
    {
        state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
        if(BoardCfg_t.active == 2) // now runing is iamge1
        {
            //dstActiveImg = 1;
            DstImageAddr = IMAGE0_ADDR;
        }
        else
        {
            //dstActiveImg = 2;
            DstImageAddr = IMAGE1_ADDR;
        }
    }
    else
    {
        DstImageAddr = addr;
    }
    state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&ImageH, sizeof(ImageH));
    //state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&FileUpgradeData, sizeof(FileUpgradeData));
    app_printf("DstImageAddr = %08x   \n",DstImageAddr);
    if(state != FLASH_OK)
    {
        app_printf("Flash read upgrade image header Fail.\n");
        return IMAGE_UNSAFE;
    }
    imghdr_t *ih = (imghdr_t *)&ImageH;
    
    dump_buf(ImageH,sizeof(ImageH));
	if (!is_tri_image(ih))
    {
        app_printf("Flash  image header isn't ZC.\n");
		return IMAGE_UNSAFE;
    }
    
    blockSeqMAX = ih->sz_file/block_sz + (ih->sz_file%block_sz >0 ?1:0);
    checklen = block_sz;

    Crc324K =0xFFFFFFFFUL;
    //Crc324Ktemp = 0xFFFFFFFFUL;
    app_printf("block_sz =%d,blockSeqMAX = %d,checklen = %d\n",block_sz,blockSeqMAX,checklen);
    
    app_printf("ih->sz_file is %d\n", ih->sz_file);
	app_printf("ih->sz_ih is %d, imghdr_t size is %d\n", ih->sz_ih, sizeof(imghdr_t));
	app_printf("ih->sz_sh is %d,  seghdr_t size is %d\n", ih->sz_sh, sizeof(seghdr_t));
       
    for(blockSeq = 0;blockSeq < blockSeqMAX;blockSeq++)
    {
        state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&FileUpgradeData, block_sz);
        DstImageAddr += block_sz;
        if(state != FLASH_OK)
        {
            app_printf("Flash read upgrade image blockSeq %d Fail.\n",blockSeq);
            return IMAGE_ERROR;
        }
        checklen = ((blockSeq == (blockSeqMAX-1))?(ih->sz_file%block_sz):block_sz);
        if(blockSeq == 0)
        {
            //crc_digest((&FileUpgradeData[ih->sz_ih]), (checklen- ih->sz_ih), (CRC_32 | CRC_SW), &Crc32Data);
            Crc324K = CalCRC32(Crc324K,(&FileUpgradeData[ih->sz_ih]),(checklen- ih->sz_ih));
            app_printf("b %08x, b^crc %08x,Crc32Data is %08x\n",Crc324K,Crc324K^0xFFFFFFFF,Crc32Data);
            //dump_buf(FileUpgradeData4K,64);
        }
        else
        {
            //crc_digest((&FileUpgradeData[ih->sz_ih]), ((blockSeq)*block_sz + checklen- ih->sz_ih), (CRC_32 | CRC_SW), &Crc32Data);
            //Crc324Ktemp = crc32(Crc324Ktemp,&FileUpgradeData[ih->sz_ih], (2*checklen- ih->sz_ih));
            
            //app_printf("b %d, checklen %d\n",blockSeq,checklen);
            Crc324K = CalCRC32(Crc324K,&FileUpgradeData[0],checklen);
            //app_printf("b %08x, b^crc %08x,Crc32Data is %08x,len %d\n",Crc324K,Crc324K^0xFFFFFFFF,Crc32Data,((blockSeq)*block_sz + checklen- ih->sz_ih));
            //dump_buf(FileUpgradeData,64);
            //dump_buf(FileUpgradeData,128);
        }
        
    }
	crcData = Crc324K^0xFFFFFFFF;
	app_printf("ih-crc   is  %08x, digest crc is %08x,^ %08x\n", ih->crc, crcData,Crc324K);	   

	if (/*ih->sz_file != sz || */
	    ih->sz_ih != sizeof(imghdr_t) ||
	    ih->sz_sh != sizeof(seghdr_t) ||
	    ih->crc != crcData)
       {
            return IMAGE_ERROR;
       }

	return IMAGE_OK;
}

int32_t check_image(uint32_t addr, uint32_t sz)
{
    int32_t  crcValue;
	imghdr_t *ih = (imghdr_t *)addr;

	if (!is_tri_image(ih))
		return IMAGE_UNSAFE;

    crcValue = crc_digest((unsigned char *)(addr + ih->sz_ih), ih->sz_file - ih->sz_ih, (CRC_32 | CRC_SW), &Crc32Data);

	app_printf("ih->sz_file is %d\n", ih->sz_file);
	app_printf("ih->sz_ih is %d, imghdr_t size is %d\n", ih->sz_ih, sizeof(imghdr_t));
	app_printf("ih->sz_sh is %d,  seghdr_t size is %d\n", ih->sz_sh, sizeof(seghdr_t));
	app_printf("ih-crc   is  %08x, digest crc is %08x, crcValue is %08x\n", ih->crc, Crc32Data, crcValue);	   

	if (/*ih->sz_file != sz || */
	    ih->sz_ih != sizeof(imghdr_t) ||
	    ih->sz_sh != sizeof(seghdr_t) ||
	    ih->crc != Crc32Data)
       {
			  return IMAGE_ERROR;
       }

	return IMAGE_OK;
}

uint8_t is_tri_image_type(imghdr_t *ih)
{
	char *__s = IMG_IDENT;
	char *__t = IMG_IDENT0;
 #if defined(STATIC_MASTER)   
    char *__v = IMG_IDENT1;
    char *__a = IMG_IDENT2;
    char *__b = IMG_IDENT3;
 #endif
	char *__d = (char *)ih->ident;
	char deviceType = ih->devicetype;
	uint8_t ret , retType;
		
	
	while ((ret = *__d++ - *__s) == 0 && *__s++)
		;
	if(ret != 0)
	{
		__d = (char *)ih->ident;
		while ((ret = *__d++ - *__t) == 0 && *__t++)
		;
	}
#if defined(STATIC_MASTER)   
        if(ret != 0)
    	{
    		__d = (char *)ih->ident;
    		while ((ret = *__d++ - *__v) == 0 && *__v++)
    		;
    	}
        if(ret != 0)
    	{
    		__d = (char *)ih->ident;
    		while ((ret = *__d++ - *__a) == 0 && *__a++)
    		;
    	}
        if(ret != 0)
    	{
    		__d = (char *)ih->ident;
    		while ((ret = *__d++ - *__b) == 0 && *__b++)
    		;
    	}
 #endif

    
#if defined(STATIC_MASTER)
    retType = (deviceType == ZC3951CCO_type  ? ZC3951CCO_type  :
               deviceType == ZC3750STA_type  ? ZC3750STA_type  :
               deviceType == ZC3750CJQ2_type ? ZC3750CJQ2_type :
               deviceType == ZC3951CCO_UpDate_type  ? ZC3951CCO_UpDate_type :
               deviceType == ZC3750STA_UpDate_type  ? ZC3750STA_UpDate_type :
               deviceType == ZC3750CJQ2_UpDate_type ? ZC3750CJQ2_UpDate_type: 0 );
               
    app_printf("ih->devicetype %x retType %x\n",deviceType,retType);
    
	return (ret == 0&&(retType != 0)) ? retType : 0;
    
#elif defined(STATIC_NODE)
    retType = (deviceType == ZC3750STA_type ? 0 : 1);
#elif defined(ZC3750CJQ2)
    retType = (deviceType == ZC3750CJQ2_type ? 0 : 1);
#endif
    app_printf("ih->devicetype %x retType %x\n",deviceType,retType);
	return (ret == 0&&retType == 0) ? 1 : 0;
}

uint8_t check_image_type(uint32_t addr, uint32_t sz)
{
    int32_t  crcValue;
	imghdr_t *ih = (imghdr_t *)addr;
    uint8_t  retType = 0;
    retType = is_tri_image_type(ih);
	if (!(retType ))
		return IMAGE_UNSAFE;

    crcValue = crc_digest((unsigned char *)(addr + ih->sz_ih), ih->sz_file - ih->sz_ih, (CRC_32 | CRC_SW), &Crc32Data);

	app_printf("ih->sz_file is %d\n", ih->sz_file);
	app_printf("ih->sz_ih is %d, imghdr_t size is %d\n", ih->sz_ih, sizeof(imghdr_t));
	app_printf("ih->sz_sh is %d,  seghdr_t size is %d\n", ih->sz_sh, sizeof(seghdr_t));
	app_printf("ih-crc   is  %08x, digest crc is %08x, crcValue is %08x\n", ih->crc, Crc32Data, crcValue);	   

	if (/*ih->sz_file != sz || */
	    ih->sz_ih != sizeof(imghdr_t) ||
	    ih->sz_sh != sizeof(seghdr_t) ||
	    ih->crc != Crc32Data)
       {
		    return IMAGE_ERROR;
       }
#if defined(STATIC_MASTER)
	return retType;
#else
    return IMAGE_OK;
#endif
}

int32_t write_image_block(uint32_t addr, uint32_t block_sz)
{
    uint32_t  state;
    uint32_t  DstImageAddr;
    U32   blockSeq = 0;
    U32   blockSeqMAX = 0;
    U8    ImageH[40];
    U32   checklen = 0;
    U32   filesize = 0;
    
    state = zc_flash_read(FLASH, (IMAGE0_RESERVE_ADDR), (U32 *)&ImageH, sizeof(ImageH));
    //state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&FileUpgradeData, sizeof(FileUpgradeData));
    
    if(state != FLASH_OK)
    {
        app_printf("Flash read upgrade image header Fail.\n");
        return IMAGE_UNSAFE;
    }

    imghdr_t *ih = (imghdr_t *)&ImageH;   
	if (!is_tri_image(ih))
		return IMAGE_UNSAFE;
    
    filesize = ih->sz_file + APP_EXT_INFO_MAX_SIZE + 2U;
    blockSeqMAX = filesize/block_sz + (filesize%block_sz >0 ?1:0);//ih->sz_file/block_sz + (ih->sz_file%block_sz >0 ?1:0);
    checklen = block_sz;

    app_printf("block_sz =%d,blockSeqMAX = %d,checklen = %d\n",block_sz,blockSeqMAX,checklen);
    
    app_printf("ih->sz_file is %d\n", ih->sz_file);
	app_printf("ih->sz_ih is %d, imghdr_t size is %d\n", ih->sz_ih, sizeof(imghdr_t));
	app_printf("ih->sz_sh is %d,  seghdr_t size is %d\n", ih->sz_sh, sizeof(seghdr_t));
    DstImageAddr = 0;
    
    zc_flash_erase(FLASH, (addr+DstImageAddr),IMAGE0_RESERVE_LEN);
    for(blockSeq = 0;blockSeq < blockSeqMAX;blockSeq++)
    {
        state = zc_flash_read(FLASH, (IMAGE0_RESERVE_ADDR+DstImageAddr), (U32 *)&FileUpgradeData, block_sz);
        
        if(state != FLASH_OK)
        {
            app_printf("Flash read upgrade image blockSeq %d Fail.\n",blockSeq);
            return IMAGE_ERROR;
        }
        checklen = ((blockSeq == (blockSeqMAX-1))?(filesize%block_sz):block_sz);
        if(blockSeq == 0)
        {
            
        }
        else
        {
            
        }
        state = zc_flash_write(FLASH, (addr+DstImageAddr), (U32 )&FileUpgradeData, block_sz);
        if(state != FLASH_OK)
        {
            app_printf("Flash write upgrade image blockSeq %d Fail.\n",blockSeq);
            return IMAGE_ERROR;
        }
        DstImageAddr += block_sz;  
    }
	
	return IMAGE_OK;
}