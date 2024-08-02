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
#include "app_upgrade_sta.h"
#include "SchTask.h"
#include "app_dltpro.h"

#if defined(STATIC_NODE)

extern board_cfg_t BoardCfg_t;

U8    FileUpgradeData[UPGRADE_STA_DATA_SIZE * 1024];                      //STA升级缓存区
SLAVE_UPGRADE_FILEINFO_t    SlaveUpgradeFileInfo;
U8    SlaveFileTransBitMap[STA_BITMAP_FILE_SIZE / MIN_FILE_TRANS_BLOCK_SIZE / 8 + 1];
ostimer_t *UpgradeOutTimer = NULL;
ostimer_t *UpgradeWaitResetTimer = NULL;
ostimer_t *UpgradeTestRunTimer = NULL;
ostimer_t * WriteTimeOutTimer = NULL;

T_UpgradeStaWriteFlashVar upgrade_sta_write_flash_var = {
    .LastBlockSeq = -1,
    .TemBlockNum = 0,
    .FirstBlockSeq = -1,
    .Block4KSeq = -1,
};

static void upgrade_sta_out_timer_cb(struct ostimer_s *ostimer, void *arg);
static void upgrade_sta_wait_reset_timer_cb(struct ostimer_s *ostimer, void *arg);
static void upgrade_sta_test_run_timer_cb(struct ostimer_s *ostimer, void *arg);
static void upgrade_sta_clean_sta_upgrade_status(void);
static U8 upgrade_sta_write_image_reserve(U16 Seq, U16 UnitLen, U32 Data, U16 DataLen);

static void upgrade_sta_set_dst_flash_base_addr(void)
{
    U32  state;
    state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
    if(state == OK)
    {
        if(BoardCfg_t.active == 1)  // now runing is iamge0
        {
        SlaveUpgradeFileInfo.DstFlashBaseAddr = IMAGE1_ADDR;
        }
         else if(BoardCfg_t.active == 2) // now runing is iamge1
        {
          SlaveUpgradeFileInfo.DstFlashBaseAddr = IMAGE0_ADDR;
        }
        else
        {
          SlaveUpgradeFileInfo.DstFlashBaseAddr = IMAGE1_ADDR;
        }
    }
}

void upgrade_sta_start_proc(UPGRADE_START_IND_t *pUpgradeStartInd)
{
    UPGRADE_START_RSP_t *pUpgradeStartResponse_t = NULL;
	pUpgradeStartResponse_t = (UPGRADE_START_RSP_t*)zc_malloc_mem(sizeof(work_t)+sizeof(UPGRADE_START_RSP_t), "UpgradeQueryRsq",MEM_TIME_OUT);

    //debug_printf(&dty, DEBUG_APP,"----------SlaveUpgradeFileInfo.TotolBlockNum : %d\n",SlaveUpgradeFileInfo.TotolBlockNum);
 	if(pUpgradeStartInd->UpgradeID != SlaveUpgradeFileInfo.UpgradeID)
    {
        SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        
        SlaveUpgradeFileInfo.BlockSize           = pUpgradeStartInd->BlockSize;
        SlaveUpgradeFileInfo.OnlineStatus = FALSE;
        timer_stop(UpgradeTestRunTimer,TMR_NULL);
        upgrade_sta_clean_sta_upgrade_status();
        upgrade_sta_clean_ram_data_temp();
    }  
    if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_UPGRADE_IDLE)
    {
        SlaveUpgradeFileInfo.OnlineStatus = FALSE;
        timer_stop(UpgradeTestRunTimer,TMR_NULL);
        SlaveUpgradeFileInfo.UpgradeID         = pUpgradeStartInd->UpgradeID;
        SlaveUpgradeFileInfo.UpgradeTimeout = pUpgradeStartInd->UpgradeTimeout;
        SlaveUpgradeFileInfo.BlockSize           = pUpgradeStartInd->BlockSize;
        SlaveUpgradeFileInfo.FileSize              = pUpgradeStartInd->FileSize;
        SlaveUpgradeFileInfo.FileCrc               = pUpgradeStartInd->FileCrc;
		
		//app_printf("crcValue = %08x, Crc32Data = %08x\n", crcValue, Crc32Data);
		app_printf("upID%d\n",SlaveUpgradeFileInfo.UpgradeID);
        SlaveUpgradeFileInfo.TotolBlockNum = (SlaveUpgradeFileInfo.FileSize / SlaveUpgradeFileInfo.BlockSize);
        if(SlaveUpgradeFileInfo.FileSize % SlaveUpgradeFileInfo.BlockSize)
        {
            SlaveUpgradeFileInfo.TotolBlockNum += 1;
        }

        app_printf("---BlockSize : %d FileSize : %d TotalBlockNum: %d Crc32Data = %08x\n",SlaveUpgradeFileInfo.BlockSize,SlaveUpgradeFileInfo.FileSize, SlaveUpgradeFileInfo.TotolBlockNum,SlaveUpgradeFileInfo.FileCrc);
        upgrade_sta_clean_sta_upgrade_status();
        upgrade_sta_clean_image_reserve();
        upgrade_sta_clean_ram_data_temp();
        memset(upgrade_sta_write_flash_var.imageblock0, 0x00, sizeof(upgrade_sta_write_flash_var.imageblock0));
        //memset(SlaveFileTransBitMap, 0x00, MAX_UPGRADE_FILE_SIZE/FILE_TRANS_BLOCK_SIZE/8);  //自用免重置
    	
        // Start a Timer for upgrade timeout.
        if(UpgradeOutTimer)
	 	{
			//if(TMR_STOPPED==zc_timer_query(UpgradeOutTimer))
			{
				timer_modify(UpgradeOutTimer,
								SlaveUpgradeFileInfo.UpgradeTimeout * 60 * 1000,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
                                upgrade_sta_out_timer_cb,
	                            NULL,
                                "upgrade_sta_out_timer_cb",
                                TRUE);
	        	timer_start(UpgradeOutTimer);
			}
		}
       else
          sys_panic("<UpgradeOutTimer fail!> %s: %d\n");

        pUpgradeStartResponse_t->StartResultCode = e_UPGRADESTART_SUCCESS;
        pUpgradeStartResponse_t->UpgradeID = pUpgradeStartInd->UpgradeID;

        SlaveUpgradeFileInfo.StaUpgradeStatus = e_FILE_DATA_RECEIVING;
        //UpgradeStaFlag = TRUE;
    }
    else
    {
        if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_FILE_DATA_RECEIVING||SlaveUpgradeFileInfo.StaUpgradeStatus ==e_FILE_RCEV_FINISHED) 
			//&&pUpgradeStartInd->UpgradeID == SlaveUpgradeFileInfo.UpgradeID)  //现场版本不检查升级ID
        {
            pUpgradeStartResponse_t->StartResultCode = e_UPGRADESTART_SUCCESS;
        }
        else
        {            
            pUpgradeStartResponse_t->StartResultCode = e_UPGRADE_BUSY;
        }
        pUpgradeStartResponse_t->UpgradeID = SlaveUpgradeFileInfo.UpgradeID;
    }   
    
    __memcpy(pUpgradeStartResponse_t->DstMacAddr, pUpgradeStartInd->SrcMacAddr, 6);
    __memcpy(pUpgradeStartResponse_t->SrcMacAddr, pUpgradeStartInd->DstMacAddr, 6);

    ApsUpgradeStartResponse(pUpgradeStartResponse_t);
	
    zc_free_mem(pUpgradeStartResponse_t);
}

void upgrade_sta_status_query_proc(UPGRADE_STATUS_QUERY_IND_t *pUpgradeQueryInd)
{
    UPGRADE_STATUS_QUERY_RSP_t *pUpgradeQueryResponse_t = NULL;
    pUpgradeQueryResponse_t = zc_malloc_mem(sizeof(work_t)+sizeof(UPGRADE_STATUS_QUERY_RSP_t)+sizeof(SlaveFileTransBitMap), "UpgradeQueryRsq",MEM_TIME_OUT);
    //UPGRADE_STATUS_QUERY_RSP_t *pUpgradeQueryResponse_t = (UPGRADE_STATUS_QUERY_RSP_t*)work->buf;

    int  bitMapLen;
    int  i;
	SlaveUpgradeFileInfo.TotolBlockNum = MIN(SlaveUpgradeFileInfo.TotolBlockNum,STA_BITMAP_FILE_SIZE/MIN_FILE_TRANS_BLOCK_SIZE);

	app_printf("UpID:%d,Sa:%d\n",pUpgradeQueryInd->UpgradeID,SlaveUpgradeFileInfo.UpgradeID);

    //if(pUpgradeQueryInd->UpgradeID == SlaveUpgradeFileInfo.UpgradeID)
    {
        //??????
        pUpgradeQueryResponse_t->UpgradeStatus = SlaveUpgradeFileInfo.StaUpgradeStatus; //CheckFileTransFinish(SlaveFileTransBitMap, SlaveUpgradeFileInfo.TotolBlockNum);
		//防止试运行态回复错误
		if(SlaveUpgradeFileInfo.OnlineStatus == TRUE&&SlaveUpgradeFileInfo.StaUpgradeStatus == e_UPGRADE_IDLE)
		{
			pUpgradeQueryResponse_t->UpgradeStatus = e_UPGRADE_TEST_RUN ;
		}
        if(pUpgradeQueryInd->QueryBlockCount == 0xFFFF)
        {
            pUpgradeQueryResponse_t->ValidBlockCount = MIN(SlaveUpgradeFileInfo.TotolBlockNum,STA_BITMAP_FILE_SIZE/FILE_TRANS_BLOCK_SIZE);
        }
        else
        {
            if((pUpgradeQueryInd->StartBockIndex + pUpgradeQueryInd->QueryBlockCount)  < SlaveUpgradeFileInfo.TotolBlockNum)
            {
                pUpgradeQueryResponse_t->ValidBlockCount = pUpgradeQueryInd->QueryBlockCount;//
            }
            else
            {
                pUpgradeQueryResponse_t->ValidBlockCount = (SlaveUpgradeFileInfo.TotolBlockNum - pUpgradeQueryInd->StartBockIndex);
            }
        }
        pUpgradeQueryResponse_t->StartBlockIndex = pUpgradeQueryInd->StartBockIndex;
        pUpgradeQueryResponse_t->UpgradeID         = SlaveUpgradeFileInfo.UpgradeID;
        
        __memcpy(pUpgradeQueryResponse_t->DstMacAddr, pUpgradeQueryInd->SrcMacAddr, 6);
        __memcpy(pUpgradeQueryResponse_t->SrcMacAddr, pUpgradeQueryInd->DstMacAddr, 6);

        bitMapLen = pUpgradeQueryResponse_t->ValidBlockCount / 8;
        if(pUpgradeQueryResponse_t->ValidBlockCount % 8)
        {
            bitMapLen += 1;
        }

        memset(pUpgradeQueryResponse_t->BlockInfoBitMap, 0x00, bitMapLen);
        
        for(i=0; i<pUpgradeQueryResponse_t->ValidBlockCount; i++)
        {
           if(SlaveFileTransBitMap[(pUpgradeQueryInd->StartBockIndex+i) / 8] & (0x01 << ((pUpgradeQueryInd->StartBockIndex+i)  % 8)))
           {
               pUpgradeQueryResponse_t->BlockInfoBitMap[i/8] |= (0x01 << (i % 8));
           }
        }
       
		if(SlaveFileTransBitMap[0]&0x01)
		{
			imghdr_t *ih = (imghdr_t *)upgrade_sta_write_flash_var.imageblock0;
			if (!is_tri_image(ih))//非自己bin直接回复接收完成
			{
				pUpgradeQueryResponse_t->UpgradeStatus = e_FILE_RCEV_FINISHED;
				app_printf("pUpgradeQueryResponse_t->UpgradeStatus = %d\n",pUpgradeQueryResponse_t->UpgradeStatus);
			}
		}
        
        //__memcpy(pUpgradeQueryResponse_t->BlockInfoBitMap.pPayload, 
        //                         &SlaveFileTransBitMap[pUpgradeQueryInd->StartBockIndex / 8], bitMapLen);
        pUpgradeQueryResponse_t->BitMapLen = bitMapLen;
        app_printf("pUpgradeQueryResponse_t->UpgradeStatus = %d\n",pUpgradeQueryResponse_t->UpgradeStatus);
        app_printf("bitMapLen is %08x\n", bitMapLen);
    	dump_buf( pUpgradeQueryResponse_t->BlockInfoBitMap,bitMapLen);
        //work->work = ApsUpgradeStatusQueryResponse;
        ApsUpgradeStatusQueryResponse(pUpgradeQueryResponse_t);
        zc_free_mem(pUpgradeQueryResponse_t);
        //post_app_work(work);
    }
}

void upgrade_sta_station_info_proc(STATION_INFO_QUERY_IND_t *pUpgradeStationInfoInd)
{
    STATION_INFO_QUERY_RSP_t *pStationInfoQueryResponse_t = NULL;
    pStationInfoQueryResponse_t = zc_malloc_mem(sizeof(work_t)+sizeof(STATION_INFO_QUERY_RSP_t)+200, "StationInfoRsq",MEM_TIME_OUT);

	app_printf("upgrade_sta_station_info_proc:SlaveUpgradeFileInfo.UpgradeID : %d\n", SlaveUpgradeFileInfo.UpgradeID);
    
    //uint32_t  ImageBaseAddr;
    //imghdr_t  imageHdr;
    int i;
    int byLen = 0;
    U8  infoListId;
    U8 *FileDataBuff = NULL;
    /*if(BoardCfg_t.active == 1)   // Get current run image addr
    {
        ImageBaseAddr = IMAGE0_ADDR;
    }
    else if(BoardCfg_t.active == 2)
    {
        ImageBaseAddr = IMAGE1_ADDR;
    }
    else
    {
        ImageBaseAddr = IMAGE0_ADDR;
    }
    app_printf("ImageBaseAddr : 0x%08x  &imageHdr : 0x%08x size : %d\n",ImageBaseAddr,&imageHdr,sizeof(imghdr_t));

    //os_sleep(500);
    FLASH_OK == zc_flash_read(FLASH, ImageBaseAddr, (U32 *)&imageHdr,  sizeof(imghdr_t))
            ? macs_printf("UPGRADE_INFO zc_flash_read OK!\n") : macs_printf("zc_flash_read ERROR!\n");
    */
    app_printf("pUpgradeStationInfoInd->InfoListNum %d\n",pUpgradeStationInfoInd->InfoListNum);
    FileDataBuff = &pStationInfoQueryResponse_t->InfoData[0];
    for(i=0; i<pUpgradeStationInfoInd->InfoListNum; i++)
    {
        infoListId = pUpgradeStationInfoInd->InfoList[i];
        FileDataBuff[byLen++] = infoListId;
        app_printf("infoListId % d\n",infoListId);

        if(infoListId == e_INFO_ID_FACTORYCODE)
        {
            // info data length
            FileDataBuff[byLen++] = 2;

            FileDataBuff[byLen++] = VersionInfo.ManufactorCode[0];
            FileDataBuff[byLen++] = VersionInfo.ManufactorCode[1];
        }
        else if(infoListId == e_INFO_ID_VERSION)
        {
            // info data length
            FileDataBuff[byLen++] = 2;
            
            FileDataBuff[byLen++] = VersionInfo.SoftVerNum[0];
            FileDataBuff[byLen++] = VersionInfo.SoftVerNum[1];
        }
        else if(infoListId == e_INFO_ID_BOOTVER)
        {
            // info data length
            FileDataBuff[byLen++] = 1;

            FileDataBuff[byLen++] = 0;
        }
        else if(infoListId == e_INFO_ID_CRC32)
        {
            // info data length
            FileDataBuff[byLen++] = 4;

            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileCrc & 0xFF);  //imageHdr.crc
            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileCrc  >> 8);
            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileCrc >> 16);
            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileCrc >> 24);
        }
        else if(infoListId == e_INFO_ID_FILELENGTH)
        {
            // info data length
            FileDataBuff[byLen++] = 4;

            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileSize & 0xFF);  //imageHdr
            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileSize >> 8);
            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileSize>> 16);
            FileDataBuff[byLen++] = (U8)(SlaveUpgradeFileInfo.FileSize>> 24);
        }
        else if(infoListId == e_INFO_ID_DEVICETYPE)
        {
            // info data length
            FileDataBuff[byLen++] = 1;

            FileDataBuff[byLen++] = 0;  // device type  temp value
        }
        else
        {
            ;
        }
    }
    
    pStationInfoQueryResponse_t->UpgradeID = SlaveUpgradeFileInfo.UpgradeID;
    pStationInfoQueryResponse_t->InfoListNum = pUpgradeStationInfoInd->InfoListNum;
    pStationInfoQueryResponse_t->InfoDataLen = byLen;
    __memcpy(pStationInfoQueryResponse_t->DstMacAddr, pUpgradeStationInfoInd->SrcMacAddr, 6);
    __memcpy(pStationInfoQueryResponse_t->SrcMacAddr, pUpgradeStationInfoInd->DstMacAddr, 6);
    
    //work->work = ApsStationInfoQueryResponse;
    ApsStationInfoQueryResponse(pStationInfoQueryResponse_t);
    zc_free_mem(pStationInfoQueryResponse_t);
    //post_app_work(work);
}

/*
void UpgradeFiletransBroadcastRequest(uint8_t pTaskPrmt)
{
    
}
*/
void  upgrade_sta_clean_ram_data_temp(void)
{
    upgrade_sta_write_flash_var.FirstBlockSeq = -1;
    upgrade_sta_write_flash_var.TemBlockNum = 0;
    upgrade_sta_write_flash_var.Block4KSeq = -1;
    memset(FileUpgradeData,0x00,sizeof(FileUpgradeData));
}

void  upgrade_sta_flash_to_ram(S16 Seq, U16 BlockSize, U8 *Data, U16 DataLen)
{
    if(((upgrade_sta_write_flash_var.LastBlockSeq+1) != Seq&&(upgrade_sta_write_flash_var.LastBlockSeq!=-1)) || (BlockSize*(upgrade_sta_write_flash_var.TemBlockNum+1)>=4*1024))//检查包序号是否连续，或缓存长度足够4K
    {
        upgrade_sta_write_image_reserve(upgrade_sta_write_flash_var.FirstBlockSeq,BlockSize,(U32)&FileUpgradeData, BlockSize* upgrade_sta_write_flash_var.TemBlockNum);//多包存储
        upgrade_sta_clean_ram_data_temp();
    }
    
    if(upgrade_sta_write_flash_var.FirstBlockSeq == -1)
    {
        upgrade_sta_write_flash_var.FirstBlockSeq = Seq;
    }
    
    upgrade_sta_write_flash_var.LastBlockSeq = Seq;
    upgrade_sta_write_flash_var.TemBlockNum ++;
    upgrade_sta_write_flash_var.Block4KSeq ++;
    timer_start(WriteTimeOutTimer);
    
    if(Seq < (MAX_UPGRADE_FILE_SIZE/BlockSize))
    {
        __memcpy(&FileUpgradeData[upgrade_sta_write_flash_var.Block4KSeq*BlockSize] , Data, DataLen);
    }
	else
    {
        app_printf("the bin is too big !\n");
    }      
    
    //SlaveFileTransBitMap[Seq / 8] |= (0x01 << (Seq % 8));
}

void  upgrade_sta_write_last_data(U16 BlockSize)
{
    if(upgrade_sta_write_flash_var.TemBlockNum >0)
    {
        upgrade_sta_write_image_reserve(upgrade_sta_write_flash_var.FirstBlockSeq,BlockSize,(U32)&FileUpgradeData, BlockSize* upgrade_sta_write_flash_var.TemBlockNum);//多包存储
        upgrade_sta_clean_ram_data_temp();
    }
}

void upgrade_sta_file_trans_proc(FILE_TRANS_IND_t *pFileTransInd)
{
    //转发
    if(pFileTransInd->TransMode == e_UNICAST_TO_LOCAL_BROADCAST)
    {
        app_printf("pFileTransInd->BlockSize is %d\n", pFileTransInd->BlockSize);

        FILE_TRANS_REQ_t *pFileTransRequest_t = NULL;
        pFileTransRequest_t = (FILE_TRANS_REQ_t*)zc_malloc_mem(sizeof(FILE_TRANS_REQ_t)+pFileTransInd->BlockSize,
                                                                           "UpgradeQueryReq",MEM_TIME_OUT);

        //app_printf("FileTransReqAps.\n");

        pFileTransRequest_t->IsCjq = pFileTransInd->IsCjq;
        pFileTransRequest_t->UpgradeID = pFileTransInd->UpgradeID;        
        pFileTransRequest_t->BlockSize   = pFileTransInd->BlockSize;
        
        pFileTransRequest_t->BlockSeq    = pFileTransInd->BlockSeq;
        pFileTransRequest_t->TransMode  = e_FILE_TRANS_UNICAST;

        memset(pFileTransRequest_t->DstMacAddr, 0xFF, 6);
#if defined(ZC3750STA)
        __memcpy(pFileTransRequest_t->SrcMacAddr, DevicePib_t.DevMacAddr, 6);
#elif defined(ZC3750CJQ2)
        __memcpy(pFileTransRequest_t->SrcMacAddr, CollectInfo_st.CollectAddr, 6);
#endif

        //给copy载荷数据到pMDB的载荷位置
        __memcpy(pFileTransRequest_t->DataBlock, pFileTransInd->DataBlock, pFileTransRequest_t->BlockSize); 
    
       
        ApsFileTransRequest(pFileTransRequest_t);

        zc_free_mem(pFileTransRequest_t);
    }
    
	//  if(pFileTransInd->UpgradeID == SlaveUpgradeFileInfo.UpgradeID &&           //自用升级
	app_printf("BSeq=%d(%d),size=%d,cID=%d,sID=%d\n", pFileTransInd->BlockSeq, SlaveUpgradeFileInfo.TotolBlockNum, pFileTransInd->BlockSize,pFileTransInd->UpgradeID,SlaveUpgradeFileInfo.UpgradeID);
    //dump_buf(pFileTransInd->DataBlock, pFileTransInd->BlockSize);
    if(pFileTransInd->BlockSeq < STA_BITMAP_FILE_SIZE/(SlaveUpgradeFileInfo.BlockSize == 0? FILE_TRANS_BLOCK_SIZE: SlaveUpgradeFileInfo.BlockSize))
    {
        if( SlaveUpgradeFileInfo.StaUpgradeStatus == e_FILE_DATA_RECEIVING && 0==get_bit_state(SlaveFileTransBitMap,pFileTransInd->BlockSeq))
        //if(SlaveUpgradeFileInfo.StaUpgradeStatus != e_FILE_RCEV_FINISHED &&
                  //SlaveUpgradeFileInfo.StaUpgradeStatus != e_UPGRADE_PERFORMING&&
                  //0==get_bit_state(SlaveFileTransBitMap,pFileTransInd->BlockSeq))
        {
        	if( SlaveUpgradeFileInfo.TotolBlockNum == 0 || SlaveUpgradeFileInfo.TotolBlockNum==0xffffffff)
        	{
    			SlaveUpgradeFileInfo.TotolBlockNum=MAX_UPGRADE_FILE_SIZE/FILE_TRANS_BLOCK_SIZE;//0xffffffff;未收到开始升级，初始化最大包数，防止越界
        	}
    	
    		if(0==pFileTransInd->BlockSeq)
    		{
    			imghdr_t   *pImghdr = (imghdr_t*)pFileTransInd->DataBlock;
    			imghdr_t *ih = (imghdr_t *)pFileTransInd->DataBlock;
                __memcpy(upgrade_sta_write_flash_var.imageblock0,pFileTransInd->DataBlock,pFileTransInd->BlockSize);
                dump_buf(upgrade_sta_write_flash_var.imageblock0, 50);// sizeof(upgrade_sta_write_flash_var.imageblock0));
    			if (is_tri_image(ih))
                {
                    if(0x01U != ih->ext_info_flag)
                    {
    				SlaveUpgradeFileInfo.BlockSize = pFileTransInd->BlockSize;
    	            SlaveUpgradeFileInfo.FileSize  = pImghdr->sz_file ;//+ sizeof(imghdr_t);
    				    
    				SlaveUpgradeFileInfo.TotolBlockNum = (SlaveUpgradeFileInfo.FileSize / SlaveUpgradeFileInfo.BlockSize);
    	            if(SlaveUpgradeFileInfo.FileSize % SlaveUpgradeFileInfo.BlockSize)
    	            {
    	                SlaveUpgradeFileInfo.TotolBlockNum += 1;
    	            }
                        app_printf("Bin no ext_info!!!");
                    }

                    app_printf("The bin is Right ;BlockSize : %d FileSize : %d\n",SlaveUpgradeFileInfo.BlockSize,SlaveUpgradeFileInfo.FileSize);
    			}
                
    			app_printf("SlaveUpgradeFileInfo.TotolBlockNum : %d\n",SlaveUpgradeFileInfo.TotolBlockNum);
    		}

            //__memcpy(FileDataBuff, pFileTransInd->DataBlock.pPayload, pFileTransInd->BlockSize);
            SlaveUpgradeFileInfo.UpgradeID = pFileTransInd->UpgradeID ;
            
            upgrade_sta_flash_to_ram(pFileTransInd->BlockSeq , SlaveUpgradeFileInfo.BlockSize , pFileTransInd->DataBlock , pFileTransInd->BlockSize);

            app_printf("----------------------patch -------------------\n");
            
            SlaveFileTransBitMap[pFileTransInd->BlockSeq / 8] |= (0x01 << (pFileTransInd->BlockSeq % 8));
            if(SlaveUpgradeFileInfo.CurrValidBlockNum < SlaveUpgradeFileInfo.TotolBlockNum)
            {
                SlaveUpgradeFileInfo.CurrValidBlockNum += 1;
            }	
            
    		if(pFileTransInd->BlockSeq && pFileTransInd->BlockSeq%10==0)
    		{
    			upgrade_show_bit_map(SlaveFileTransBitMap, SlaveUpgradeFileInfo.TotolBlockNum);
    		}
    		else
            {

            }      

            if(SlaveUpgradeFileInfo.TotolBlockNum == CheckFileTransFinish(SlaveFileTransBitMap, SlaveUpgradeFileInfo.TotolBlockNum))
            {
                upgrade_sta_write_last_data(SlaveUpgradeFileInfo.BlockSize);
                
                os_sleep(20);
            		
    	           U8 state = 0 ;
                   state = check_image_block(IMAGE0_RESERVE_ADDR,4*1024);//check_image((U32)FileUpgradeData, SlaveUpgradeFileInfo.FileSize);
                   app_printf("Crc check state is %d\n", state);
                   //if(state == IMAGE_OK)
                   
                   if(state == IMAGE_OK)
                   {
                      app_printf("Crc check is OK.\n");
                       
                      //state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
                      upgrade_sta_set_dst_flash_base_addr();
    	              SlaveUpgradeFileInfo.StaUpgradeStatus = e_FILE_RCEV_FINISHED;
    				 
                      app_printf("-------StaUpgradeStatus = %d,UpgradeTimeout=%d!\n",SlaveUpgradeFileInfo.StaUpgradeStatus,SlaveUpgradeFileInfo.UpgradeTimeout);
                   }
                   else if(state == IMAGE_UNSAFE)
                   {
                       imghdr_t *ih_test = (imghdr_t *)upgrade_sta_write_flash_var.imageblock0;
                       if(is_test_image(ih_test) == 1)
                       {
                           SlaveUpgradeFileInfo.StaUpgradeStatus = e_FILE_RCEV_FINISHED;
                           app_printf("is_test_image StaUpgrade Status = %d,Timeout=%d!\n",SlaveUpgradeFileInfo.StaUpgradeStatus,SlaveUpgradeFileInfo.UpgradeTimeout);
                       }
                   }
                   else
                   {
                        memset(SlaveFileTransBitMap, 0x00, sizeof(SlaveFileTransBitMap));
                   }
            }
        }
        
        else
        {
            app_printf("SlaveUpgradeFileInfo.UpgradeID is %d, SlaveUpgradeFileInfo.StaUpgradeStatus is %d \n", 
                                                    SlaveUpgradeFileInfo.UpgradeID, SlaveUpgradeFileInfo.StaUpgradeStatus);
        }
    }
    else
    {
        app_printf("the bin beyond 512K \n");
    }
    //app_printf("Free file trans indication Mdb\n", pFileTransInd->BlockSize);
}

void upgrade_sta_stop_proc(uint32_t UpgradeID)
{
    if(UpgradeID == SlaveUpgradeFileInfo.UpgradeID || UpgradeID == 0)
    {
        memset(SlaveFileTransBitMap, 0x00, sizeof(SlaveFileTransBitMap));

        if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_UPGRADE_TEST_RUN||SlaveUpgradeFileInfo.OnlineStatus == TRUE)
        {
            upgrade_sta_reset_run_image();
            SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
			SlaveUpgradeFileInfo.OnlineStatus = FALSE;
			timer_stop(UpgradeTestRunTimer, TMR_NULL);
			UpgradeStaFlag = TRUE;
            reset_run_image = TRUE;
            mutex_post(&mutexSch_t);
        }
        else if(SlaveUpgradeFileInfo.StaUpgradeStatus != e_UPGRADE_IDLE)
        {
            SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
			//upgrade_sta_clean_sta_upgrade_status();
            //SaveUpgradeStatusInfo();
            UpgradeStaFlag = TRUE;
        }
        else
        {

        }
    }
}

void upgrade_sta_perform_proc(UPGRADE_PERFORM_IND_t *pUpgradePerformInd)
{
    U16 waitResetTime = pUpgradePerformInd->WaitResetTime;

    if(waitResetTime>15)
    {
            waitResetTime-=15;
    }
	app_printf("UpgradePerformIndication:pUpgradePerformInd->UpgradeID : %d,SlaveUpgradeFileInfo.UpgradeID : %d\n",pUpgradePerformInd->UpgradeID,SlaveUpgradeFileInfo.UpgradeID);
    if(pUpgradePerformInd->UpgradeID == SlaveUpgradeFileInfo.UpgradeID &&
		             SlaveUpgradeFileInfo.StaUpgradeStatus == e_FILE_RCEV_FINISHED)
    {
        app_printf("Slave node upgrade perform.\n");

	     if(UpgradeWaitResetTimer)
		 {
			if(TMR_STOPPED==zc_timer_query(UpgradeWaitResetTimer))
			{
				timer_modify(UpgradeWaitResetTimer,
								waitResetTime * 1000,
	                            0,
	                            TMR_TRIGGER ,//TMR_TRIGGER
                                upgrade_sta_wait_reset_timer_cb,
	                            NULL,
                                "upgrade_sta_wait_reset_timer_cb",
                                TRUE);
	        	timer_start(UpgradeWaitResetTimer);
			}
		}
        else
          sys_panic("<UpgradeWaitResetTimer fail!> %s: %d\n");

        
	    SlaveUpgradeFileInfo.UpgradeTestRunTime = pUpgradePerformInd->TestRunTime;
	    SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_PERFORMING;
        app_printf("pUpgradePerformInd->TestRunTime = %d.\n",pUpgradePerformInd->TestRunTime);
   
	}
	else if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_FILE_DATA_RECEIVING)
	{
		//SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
		//upgrade_sta_clean_sta_upgrade_status();		
	}
    else
    {

    }
}

static void upgrade_sta_clean_sta_upgrade_status(void)
{
	//SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
	memset(SlaveFileTransBitMap, 0x00, sizeof(SlaveFileTransBitMap));
	memset(FileUpgradeData, 0x00, sizeof(FileUpgradeData));
}

void upgrade_sta_chage_run_image(void)
{
    uint32_t  state;
    uint32_t  DstImageAddr;
    U8    dstActiveImg;

    state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
    if(BoardCfg_t.active == 2) // now runing is iamge1
    {
        dstActiveImg = 1;
        DstImageAddr = IMAGE0_ADDR;
    }
    else
    {
        dstActiveImg = 2;
        DstImageAddr = IMAGE1_ADDR;
    }

    //state = zc_flash_read(FLASH, DstImageAddr, (U32 *)&FileUpgradeData, sizeof(FileUpgradeData));
    state = check_image_block(IMAGE0_RESERVE_ADDR,4*1024);
    app_printf("Crc check state is %d\n", state);
    if(state != IMAGE_OK)
    {
        app_printf("IMAGE0_RESERVE_ADDR check is err.\n");
        return;
    }
    state = write_image_block(DstImageAddr,4*1024);
    if(state != IMAGE_OK)
    {
        app_printf("DstImageAddr write is err.\n");
        return;
    }
    os_sleep(100);
    //state = check_image((U32)FileUpgradeData, sizeof(FileUpgradeData));
    app_printf("Crc check state is %d\n", state);
    state = check_image_block(DstImageAddr,4*1024);
    if(state == IMAGE_OK)
    {
        app_printf("Crc check is OK.\n");
		
        SaveUpgradeStatusInfo();
		
        BoardCfg_t.active = dstActiveImg;
        
        state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&ImghdrData, sizeof(ImghdrData));
        if(state != FLASH_OK)
        {
            app_printf("Flash read ImghdrData Fail.\n");
            return ;
        }
        
        ReadBinOutVersion(ImghdrData);
        os_sleep(100);
        state=zc_flash_write(FLASH, (U32)BOARD_CFG  ,(U32)&BoardCfg_t ,sizeof(board_cfg_t));
        OK==state ? app_printf("BOARD_CFG zc_flash_write OK!\n") : app_printf("BOARD_CFG zc_flash_write ERROR!\n");
		print_s("-----sys_reboot------\n");		

        app_ext_info_write_after_upgrade();

		os_sleep(100);

		app_reboot(500);
    }
    else
    {
		SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        upgrade_sta_clean_sta_upgrade_status();
        app_printf("Dst upgrade image CRC check Fail.\n");
    }
}

void upgrade_sta_reset_run_image(void)
{
    uint32_t  state;
    uint32_t  DstImageAddr;
    U8    dstActiveImg;

    state=zc_flash_read(FLASH ,(U32)BOARD_CFG ,(U32 *)&BoardCfg_t,sizeof(board_cfg_t));
    if(BoardCfg_t.active == 2) // now runing is iamge1
    {
        dstActiveImg = 1;
        DstImageAddr = IMAGE0_ADDR;
    }
    else
    {
        dstActiveImg = 2;
        DstImageAddr = IMAGE1_ADDR;
    }

    //state = zc_flash_read(FLASH, DstImageAddr, (U32 *)&FileUpgradeData, sizeof(FileUpgradeData));
    state = check_image_block(DstImageAddr,4*1024);
    app_printf("Crc check state is %d\n", state);
    if(state != IMAGE_OK)
    {
        app_printf("IMAGE0_RESERVE_ADDR check is err.\n");
        return;
    }
    
    if(state == IMAGE_OK)
    {
        app_printf("Crc check is OK.\n");
		
        SaveUpgradeStatusInfo();
		
        BoardCfg_t.active = dstActiveImg;
        
        state = zc_flash_read(FLASH, (DstImageAddr), (U32 *)&ImghdrData, sizeof(ImghdrData));
        if(state != FLASH_OK)
        {
            app_printf("Flash read ImghdrData Fail.\n");
            return ;
        }
        
        ReadBinOutVersion(ImghdrData);
        os_sleep(100);
        state=zc_flash_write(FLASH, (U32)BOARD_CFG  ,(U32)&BoardCfg_t ,sizeof(board_cfg_t));
        OK==state ? app_printf("BOARD_CFG zc_flash_write OK!\n") : app_printf("BOARD_CFG zc_flash_write ERROR!\n");
		print_s("-----sys_reboot------\n");		
		os_sleep(100);

		app_reboot(500);
    }
    else
    {
		SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        upgrade_sta_clean_sta_upgrade_status();
        app_printf("Dst upgrade image CRC check Fail.\n");
    }
}

void upgrade_sta_clean_image_reserve(void)
{
    if(FLASH_OK==zc_flash_erase(FLASH,IMAGE0_RESERVE_ADDR,IMAGE0_RESERVE_LEN))
	{
		app_printf("erase IMAGE0_RESERVE_ADDR ok!\n");
	}
	else
	{
		app_printf("erase IMAGE0_RESERVE_ADDR error!\n");
	}
}

static U8 upgrade_sta_write_image_reserve(U16 Seq, U16 UnitLen, U32 Data, U16 DataLen)
{
    U32 ImageResBaseAddr;
    if((Seq*UnitLen+DataLen) >= MAX_UPGRADE_FILE_SIZE)
    {
        app_printf("too big %d\n",Seq*UnitLen);
        return FALSE;
    }

    ImageResBaseAddr = IMAGE0_RESERVE_ADDR + Seq*UnitLen;
    app_printf("ImageResBaseAddr = %08x,Seq = %d,UnitLen = %d,DataLen = %d\n",ImageResBaseAddr,Seq,UnitLen,DataLen);
    // dump_buf((U8 *)&Data,100);
    if(FLASH_OK == zc_flash_write(FLASH, ImageResBaseAddr, Data, DataLen))
    {
        app_printf("WIROK\n");
        return TRUE;
    }
    else
    {
        app_printf("WIRERR\n");
    }
    return FALSE;
}

static void upgrade_sta_out_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    //uint32_t  state;

    if(UpgradeOutTimer)
    {
        if(0 != timer_stop(UpgradeOutTimer, TMR_NULL))
        {
            app_printf("timer_stop UpgradeOutTimer fail!\n");
        }
    }

    if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_FILE_DATA_RECEIVING)
    {
        // change to idle status
        SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        upgrade_sta_clean_sta_upgrade_status();
        //SaveUpgradeStatusInfo();
        //UpgradeStaFlag = TRUE;
    }
    else if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_FILE_RCEV_FINISHED)
    {
		SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_TEST_RUN;

        SlaveUpgradeFileInfo.UpgradeTestRunTime = TEST_RUN_TIME;
		ImageWriteFlag = TRUE;
		UpgradeStaFlag = TRUE;
		check_upgrade_info = TRUE;
		
		app_printf("\n\nUpgradeOutTimerCB\n\n");
        mutex_post(&mutexSch_t);
    }
	else
	{
		SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        upgrade_sta_clean_sta_upgrade_status();
	}
}

static void upgrade_sta_wait_reset_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    //uint32_t  state;
	if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_UPGRADE_PERFORMING)
	{
	    if(UpgradeWaitResetTimer)
	    {
	        if(0 != timer_stop(UpgradeWaitResetTimer, TMR_NULL))
	        {
	            app_printf("timer_stop UpgradeWaitResetTimer fail!\n");
	        }
	    }
		if(SlaveUpgradeFileInfo.UpgradeTestRunTime==0)
        {
            SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        }
        else
        {
		    SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_TEST_RUN;
        }
        imghdr_t *ih_test = (imghdr_t *)upgrade_sta_write_flash_var.imageblock0;
        if(is_test_image(ih_test) == 1)
        {
            printf_s("is_test_image reboot!\n");
            app_reboot(500);
            return;
        }
		check_upgrade_info  =  TRUE;
		UpgradeStaFlag = TRUE;
		ImageWriteFlag = TRUE;
        mutex_post(&mutexSch_t);
		/*
	    SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_TEST_RUN;
	    //SaveUpgradeStatusInfo();
	    UpgradeStaFlag = TRUE;
        mutex_post(&mutexSch_t);

	    if(BoardCfg_t.active == 1)  // now runing is iamge0
	    {
	        BoardCfg_t.active = 2;
	    }
	    else if(BoardCfg_t.active == 2) // now runing is iamge1
	    {
	        BoardCfg_t.active = 1;
	    }
        state=zc_flash_write(FLASH, (U32)BOARD_CFG  ,(U32)&BoardCfg_t ,sizeof(board_cfg_t));
	    OK==state ? app_printf("zc_flash_read OK!\n") : app_printf("zc_flash_read ERROR!\n");
	    os_sleep(400);
	    appreboot();
	    */
	    return;
	}
	else
	{
		SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        upgrade_sta_clean_sta_upgrade_status();
	}
}

static void upgrade_sta_test_run_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    //uint32_t  state;
    app_printf("SlaveUpgradeFileInfo.StaUpgradeStatus = %d\n",SlaveUpgradeFileInfo.StaUpgradeStatus);
    if(UpgradeTestRunTimer)
    {
        if(0 != timer_stop(UpgradeTestRunTimer, TMR_NULL))
        {
            app_printf("timer_stop UpgradeTestRunTimer fail!\n");
        }
    }

    if(SlaveUpgradeFileInfo.OnlineStatus == TRUE)
    {
        SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
		SlaveUpgradeFileInfo.OnlineStatus = FALSE;
        upgrade_sta_clean_sta_upgrade_status();
        //SaveUpgradeStatusInfo();
        UpgradeStaFlag = TRUE;
    }
    else
    {
        SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        upgrade_sta_clean_sta_upgrade_status();
        //SaveUpgradeStatusInfo();
        UpgradeStaFlag = TRUE;
		reset_run_image = TRUE; //切换image
        mutex_post(&mutexSch_t);
        //upgrade_sta_reset_run_image();   //退回版本

        /*
        SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        //SaveUpgradeStatusInfo();
        UpgradeStaFlag = TRUE;
        mutex_post(&mutexSch_t);

        os_sleep(200);

        if(BoardCfg_t.active == 1)  // now runing is iamge0
        {
            BoardCfg_t.active = 2;
        }
        else if(BoardCfg_t.active == 2) // now runing is iamge1
        {
            BoardCfg_t.active = 1;
        }
        state=zc_flash_write(FLASH, (U32)BOARD_CFG  ,(U32)&BoardCfg_t ,sizeof(board_cfg_t));
        OK==state ? app_printf("zc_flash_read OK!\n") : app_printf("zc_flash_read ERROR!\n");
        os_sleep(200);
        appreboot();
        */
    }
}

static void upgrade_sta_write_timeout_timer_cb(struct ostimer_s *ostimer, void *arg)
{
    //timeout write image by 4K
    upgrade_sta_write_last_data(SlaveUpgradeFileInfo.BlockSize);
    /*
    if(TemBlockNum >0)
    {
        upgrade_sta_write_image_reserve(FirstBlockSeq,SlaveUpgradeFileInfo.BlockSize,(U32)&FileUpgradeData, SlaveUpgradeFileInfo.BlockSize*TemBlockNum);//多包存储
        upgrade_sta_clean_ram_data_temp();
    }*/
}

static int8_t  upgrade_sta_out_timer_init(void)
{
    if(UpgradeOutTimer == NULL)
    {	
        // upgrade timeout  timer
        UpgradeOutTimer = timer_create(1000,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            upgrade_sta_out_timer_cb,
                            NULL,
                            "upgrade_sta_out_timer_cb"
                           );
    }
    return 0;
}

static int8_t  upgrade_sta_wait_reset_timer_init(void)
{
    if(UpgradeWaitResetTimer == NULL)
    {   
        // upgrade wait for Reset timer
        UpgradeWaitResetTimer = timer_create(1000,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            upgrade_sta_wait_reset_timer_cb,
                            NULL,
                            "upgrade_sta_wait_reset_timer_cb"
                           );
    }
    return 0;
}

static int8_t  upgrade_sta_test_run_timer_init(void)
{
    if(UpgradeTestRunTimer == NULL)
    {       
        UpgradeTestRunTimer = timer_create(1000,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            upgrade_sta_test_run_timer_cb,
                            NULL,
                            "upgrade_sta_test_run_timer_cb"
                           );
    }
    return 0;
}

static int8_t  upgrade_sta_write_timeout_timer_init(void)
{
    if(WriteTimeOutTimer == NULL)
    {       
        WriteTimeOutTimer = timer_create(6*1000,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            upgrade_sta_write_timeout_timer_cb,
                            NULL,
                            "upgrade_sta_write_timeout_timer_cb"
                           );
    }
    return 0;
}

void  upgrade_sta_check_first_init(void)
{
    upgrade_sta_out_timer_init();
    upgrade_sta_wait_reset_timer_init();
    upgrade_sta_test_run_timer_init();
    upgrade_sta_write_timeout_timer_init();
    ReadStaUpgradeStatusInfo();
    SlaveUpgradeFileInfo.OnlineStatus = FALSE;
    app_printf("--------//////StaUpgradeStatus = %d,OnlineStatus = %d,UpgradeTestRunTime = %d\n",SlaveUpgradeFileInfo.StaUpgradeStatus,SlaveUpgradeFileInfo.OnlineStatus,SlaveUpgradeFileInfo.UpgradeTestRunTime);

    if(SlaveUpgradeFileInfo.StaUpgradeStatus == e_UPGRADE_TEST_RUN&&SlaveUpgradeFileInfo.UpgradeTestRunTime != 0)
    {
        // Start a Timer for upgrade timeout.
        if(SlaveUpgradeFileInfo.UpgradeTestRunTime<30)
        {
            SlaveUpgradeFileInfo.UpgradeTestRunTime = 30;
        }
        
        if(UpgradeTestRunTimer)
        {
            //if(TMR_STOPPED==zc_timer_query(UpgradeTestRunTimer))
            {
                timer_modify(UpgradeTestRunTimer,
                            SlaveUpgradeFileInfo.UpgradeTestRunTime * 1000,
                            0,
                            TMR_TRIGGER ,//TMR_TRIGGER
                            upgrade_sta_test_run_timer_cb,
                            NULL,
                            "upgrade_sta_test_run_timer_cb",
                            TRUE);
                timer_start(UpgradeTestRunTimer);
            }
        }
       else
          sys_panic("<UpgradeTestRunTimer fail!> %s: %d\n");        
    }
    else
    {
        SlaveUpgradeFileInfo.StaUpgradeStatus = e_UPGRADE_IDLE;
        //upgrade_sta_clean_sta_upgrade_status();
    }
    return ;
}

#endif















 

