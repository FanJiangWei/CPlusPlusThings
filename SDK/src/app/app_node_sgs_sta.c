#include "types.h"
#include "mbuf.h"
#include"DataLinkInterface.h"
#include"aps_layer.h"
#include"mem_zc.h"
#include"app_common.h"
#include"app_node_sgs_ex.h"
#include"app_dltpro.h"
#include"SchTask.h"
#include"app.h"


#ifdef NODE_SGS

void sta_sgs_ind(work_t *work)
{
#if defined(STATIC_NODE)

    MSDU_DATA_IND_t   *pMsduDataInd = (MSDU_DATA_IND_t *)work->buf;
    APDU_HEADER_t     *pApsHeader = (APDU_HEADER_t *)pMsduDataInd->Msdu;
    NODE_SGS_HEADER_t  *pNodeSGSUpHeader = (NODE_SGS_HEADER_t *)pApsHeader->Apdu;
	if((0 == pNodeSGSUpHeader->Data[0] || 1 == pNodeSGSUpHeader->Data[0]) 
		&& DevicePib_t.HNOnLineFlg != pNodeSGSUpHeader->Data[0])
	{
    	DevicePib_t.HNOnLineFlg = pNodeSGSUpHeader->Data[0];
		staflag = TRUE;
	}
#endif

}



/*  定义但为使用
#if defined(STATIC_NODE)

static void node_pro_mgm_mgs(U8 *Result, U8* ID)
{
	U8	AppID[2] = {APPLICATION_ID0,APPLICATION_ID1};
	if((*Result == ASSOC_SUCCESS || *Result == RE_ASSOC_SUCCESS) && DevicePib_t.HNOnLineFlg == TRUE && memcmp(ID,AppID,2)!=0)
	{
		*Result = NO_WHITELIST;
	}
}

#endif
*/


#endif


