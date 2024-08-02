



#include "netnnib.h"
#include "types.h"
#include "DataLinkGlobal.h"
#include "DatalinkTask.h"
#include "Scanbandmange.h"
#include "printf_zc.h"
#include "app_data_freeze_cco.h"


extern int32_t pid_app_work;
extern S32 PidDatalinkTask;
#if defined(STATIC_MASTER)

U8 DeviceList_ioctl(uint32_t cmd,void *in_arg, void *out_arg)
{
		//U16 TEI;
		U8 result;
		U16 TEI,num;


        if(task_query() == PidDatalinkTask)
        {
            net_printf("D_ioctl %d\n", task_query());
            return FALSE;
        }

        mutex_pend(&task_work_mutex, 0);
		switch (cmd)
		{
			case DEV_SET_MODEID:
				result= SaveModeId((U8*)in_arg,(U8*)out_arg);
			break;
			case DEV_GET_TEI_BYMAC://DeviceList_ioctl(DEV_GET_TEI_BYMAC,in_arg,out_arg)
				TEI =SearchTEIDeviceTEIList((U8*)in_arg);
				__memcpy(out_arg,&TEI,sizeof(U16));
				result= (TEI !=INVALID)?TRUE:FALSE;
			break;
			case DEV_SET_AREA: //DeviceList_ioctl(DEV_SET_AERE,in_arg,out_arg);
				result= Set_DeviceList_AREA(*(U16*)in_arg,*(U8*)out_arg);
			break;
			case DEV_GET_AREA:
				result = get_devicelist_area(*(U16*)in_arg,(U8*)out_arg);
			break;
			case DEV_SET_EDGE://DeviceList_ioctl(DEV_GET_LISTNUM,in_arg,out_arg);
				result= SaveEdgeinfo(*(U16*)in_arg,*(U8*)out_arg);
			break;
			case DEV_SET_CURVE:
				result= SaveCurveCfg((U8*)in_arg,(U8*)out_arg);
			break;	
			case DEV_GET_LISTNUM: //DeviceList_ioctl(DEV_GET_LISTNUM,NULL,out_arg);
				num=GetDeviceNum();
				__memcpy(out_arg,&num,sizeof(U16));
				result= TRUE;
			break;
            case DEV_APP_GET_LISTNUM: //DeviceList_ioctl(DEV_GET_LISTNUM,NULL,out_arg);安徽
                num=GetDeviceNumForApp();
                __memcpy(out_arg,&num,sizeof(U16));
                result = TRUE;
            break;
			case DEV_GET_ALL_BYSEQ://DeviceList_ioctl(DEV_GET_ALL_BYSEQ,*in_arg, void *out_arg)
				result= Get_DeviceList_All(*(U16*)in_arg,(DEVICE_TEI_LIST_t*)out_arg);
			break;
			case DEV_GET_ALL_BYMAC://DeviceList_ioctl(DEV_GET_ALL_BYMAC,*in_arg, void *out_arg)
				result= Get_DeviceList_All_ByAdd((U8*)in_arg,(DEVICE_TEI_LIST_t*)out_arg);
			break;

			case DEV_GET_VAILD: 
				result= Get_DeviceListVaild(*(U16*)in_arg);
			break;
            case DEV_RESET_AREA: 
                Reset_DeviceList_AREA_ERR();
                result =TRUE;
            break;
            case DEV_SET_COLLECT: //DeviceList_ioctl(DEV_SET_AERE,in_arg,out_arg);
				result= Set_DeviceList_COLLECT(*(U16*)in_arg,*(U8*)out_arg);
			break;
            case DEV_SET_POWER_OFF:	//离网感知停电原因记录
				result= set_device_list_power_off(*(U16*)in_arg);
			break;
			case DEV_RESET_POWER_OFF:
				result= reset_device_list_power_off(*(U16*)in_arg);
			break;
			case DEV_APP_SET_WHITESEQ:
				result = set_device_list_white_seq(*(U16*)in_arg,*(U16*)out_arg);
			break;
			default:
				result =FALSE;
			break;

		}


        mutex_post(&task_work_mutex);

//        if(task_query() != pid_app_work)
//            net_printf("D_ioctl\n");

		return result;

}

U8 DeviceList_ioctl_notsafe(uint32_t cmd,void *in_arg, void *out_arg)
{
		//U16 TEI;
		U8 result;
		U16 TEI,num;


        if(task_query() == PidDatalinkTask)
        {
            net_printf("D_ioctl %d\n", task_query());
            return FALSE;
        }

        //mutex_pend(task_work_mutex, 0);
		switch (cmd)
		{
			case DEV_SET_MODEID:
				result= SaveModeId((U8*)in_arg,(U8*)out_arg);
			break;
			case DEV_GET_TEI_BYMAC://DeviceList_ioctl(DEV_GET_TEI_BYMAC,in_arg,out_arg)
				TEI =SearchTEIDeviceTEIList((U8*)in_arg);
				__memcpy(out_arg,&TEI,sizeof(U16));
				result= (TEI !=INVALID)?TRUE:FALSE;
			break;
			case DEV_SET_AREA: //DeviceList_ioctl(DEV_SET_AERE,in_arg,out_arg);
				result= Set_DeviceList_AREA(*(U16*)in_arg,*(U8*)out_arg);
			break;
			case DEV_SET_EDGE://DeviceList_ioctl(DEV_GET_LISTNUM,in_arg,out_arg);
				result= SaveEdgeinfo(*(U16*)in_arg,*(U8*)out_arg);
			break;
			case DEV_GET_LISTNUM: //DeviceList_ioctl(DEV_GET_LISTNUM,NULL,out_arg);
				num=GetDeviceNum();
				__memcpy(out_arg,&num,sizeof(U16));
				result= TRUE;
			break;
			case DEV_GET_ALL_BYSEQ://DeviceList_ioctl(DEV_GET_ALL_BYSEQ,*in_arg, void *out_arg)
				result= Get_DeviceList_All(*(U16*)in_arg,(DEVICE_TEI_LIST_t*)out_arg);
			break;
			case DEV_GET_ALL_BYMAC://DeviceList_ioctl(DEV_GET_ALL_BYMAC,*in_arg, void *out_arg)
				result= Get_DeviceList_All_ByAdd((U8*)in_arg,(DEVICE_TEI_LIST_t*)out_arg);
			break;

			case DEV_GET_VAILD: 
				result= Get_DeviceListVaild(*(U16*)in_arg);
			break;
            case DEV_RESET_AREA: 
                Reset_DeviceList_AREA_ERR();
                result =TRUE;
            break;
            case DEV_SET_COLLECT: //DeviceList_ioctl(DEV_SET_AERE,in_arg,out_arg);
				result= Set_DeviceList_COLLECT(*(U16*)in_arg,*(U8*)out_arg);
			break;
			default:
				result =FALSE;
			break;

		}


        //mutex_post(&task_work_mutex);

//        if(task_query() != pid_app_work)
//            net_printf("D_ioctl\n");

		return result;

}

U8 WhiteList_ioctrl(uint32_t cmd,void *in_arg, void *out_arg)
{

    if(task_query() == PidDatalinkTask)
    {
        net_printf("D_ioctl %d\n", task_query());
        return FALSE;
    }

    mutex_pend(&task_work_mutex, 0);

    switch(cmd)
    {
    case WHITE_GET_ONE_BYSEQ :
        __memcpy(out_arg, &WhiteMacAddrList[*(U16 *)in_arg], sizeof(WHITE_MAC_LIST_t));
        break;
    case WHITE_CLEAR_BYESEQ  :
        memset(&WhiteMacAddrList[*(U16 *)in_arg], 0x00,  sizeof(WHITE_MAC_LIST_t));
        break;
    case WHITE_CLEAR_ALL     :
        memset(WhiteMacAddrList, 0x00,  sizeof(WhiteMacAddrList));
        break;
    case WHITE_GET_MACADDR   :
        __memcpy(out_arg, WhiteMacAddrList[*(U16 *)in_arg].MacAddr, MACADRDR_LENGTH);
        break;
    case WHITE_GET_METERINFO :
        __memcpy(out_arg, &WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t, sizeof(METER_INFO_t));
        break;
    case WHITE_GET_MDLID     :
        __memcpy(out_arg, &WhiteMacAddrList[*(U16 *)in_arg].ModuleID, 11);
        break;
    case WHITE_GET_CNMADDR   :
        __memcpy(out_arg, WhiteMacAddrList[*(U16 *)in_arg].CnmAddr, MACADRDR_LENGTH);
        break;
    case WHITE_GET_WRFTH     :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].waterRfTh;
        break;
    case WHITE_GET_SETRSLT   :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].SetResult;
        break;
    case WHITE_GET_GUIYUE    :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t.ucGUIYUE;
        break;
    case WHITE_GET_PHASE     :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t.ucPhase;
        break;
    case WHITE_GET_LNERR     :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t.LNerr;
        break;
    case WHITE_GET_IDSTATE   :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].IDState;
        break;
    case WHITE_GET_RESULT    :
        *(U8 *)out_arg = WhiteMacAddrList[*(U16 *)in_arg].Result;
        break;
    case WHITE_SET_MACADDR   :
        __memcpy(WhiteMacAddrList[*(U16 *)in_arg].MacAddr, out_arg, MACADRDR_LENGTH);
        break;
    case WHITE_SET_METERINFO :
        __memcpy(&WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t, out_arg, sizeof(METER_INFO_t));
        break;
    case WHITE_SET_MDLID     :
        __memcpy(&WhiteMacAddrList[*(U16 *)in_arg].ModuleID, out_arg, 11);
        break;
    case WHITE_SET_CNMADDR   :
        __memcpy(WhiteMacAddrList[*(U16 *)in_arg].CnmAddr, out_arg, MACADRDR_LENGTH);
        break;
    case WHITE_SET_WRFTH     :
        WhiteMacAddrList[*(U16 *)in_arg].waterRfTh = *(U8 *)out_arg;
        break;
    case WHITE_SET_SETRSLT   :
        WhiteMacAddrList[*(U16 *)in_arg].SetResult = *(U8 *)out_arg;
        break;
    case WHITE_SET_GUIYUE    :
        WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t.ucGUIYUE = *(U8 *)out_arg;
        break;
    case WHITE_SET_PHASE     :
        WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t.ucPhase = *(U8 *)out_arg;
        break;
    case WHITE_SET_LNERR     :
        WhiteMacAddrList[*(U16 *)in_arg].MeterInfo_t.LNerr = *(U8 *)out_arg;
        break;
    case WHITE_SET_IDSTATE   :
        WhiteMacAddrList[*(U16 *)in_arg].IDState = *(U8 *)out_arg;
        break;
    case WHITE_SET_RESULT    :
        WhiteMacAddrList[*(U16 *)in_arg].Result = *(U8 *)out_arg;
        break;
    default:
        break;

    }

    mutex_post(&task_work_mutex);

//    if(task_query() != pid_app_work)
//        net_printf("D_ioctl\n");

    return TRUE;
}


#endif
void net_nnib_ioctl(uint32_t cmd, void *arg)
{
    if(task_query() == PidDatalinkTask)
    {
        net_printf("n_ctl %d\n", task_query());
        return;
    }

    mutex_pend(&task_work_mutex, 0);
	U8  band,testmode;
	U16 tei;
	U32 snid;
	switch (cmd)
		{
		case NET_GET_SNID://net_nnib_ioctl(NET_GET_SNID,arg)
			snid=  HPLC.snid;
			__memcpy((U8*)arg,(U8*)&snid,sizeof(U32));		
			break;
		case NET_GET_TEI://net_nnib_ioctl(NET_GET_TEI,arg)
			tei = HPLC.tei;
			__memcpy((U8*)arg,(U8*)&tei,sizeof(U16));
			break;
		case NET_GET_NODESTATE://net_nnib_ioctl(NET_GET_NODESTATE,arg)
			__memcpy((U8*)arg,(U8*)&nnib.NodeState,sizeof(U8));
			break;
		case NET_GET_CCOADDR://net_nnib_ioctl(NET_GET_CCOADDR,arg)
			__memcpy((U8*)arg,GetCCOAddr(),6);
			break;
		case NET_GET_DVTYPE://net_nnib_ioctl(NET_GET_DVTYPE,arg)
			__memcpy((U8*)arg,(U8*)&nnib.DeviceType,sizeof(U8));
			break;
		case NET_GET_EDGE://net_nnib_ioctl(NET_GET_EDGE,arg)
			__memcpy((U8*)arg,(U8*)&nnib.Edge,sizeof(U8));
			break;
		case NET_GET_PWRTYPE://net_nnib_ioctl(NET_GET_PWRTYPE,arg)
			__memcpy((U8*)arg,(U8*)&nnib.PowerType,sizeof(U8));
			break;
		case NET_GET_TSTMODE://net_nnib_ioctl(NET_GET_TSTMODE,arg)
			testmode = HPLC.testmode;
			__memcpy((U8*)arg,&testmode,sizeof(U8));
			break;
		case NET_GET_BAND://net_nnib_ioctl(NET_GET_BAND,arg)
			band =GetHplcBand();
			__memcpy((U8*)arg,&band,sizeof(U8));
			break;
		case NET_SET_BAND://net_nnib_ioctl(NET_SET_BAND,arg)
			net_printf("arg=%d\n",*(U8*)arg);
			changeband(*(U8*)arg);
			break;

		case NET_SET_DVTYPE://net_nnib_ioctl(NET_SET_DVTYPE,arg)
			SetnnibDevicetype(*(U8*)arg); 
			break;
		case NET_SET_TSTMODE://net_nnib_ioctl(NET_SET_TSTMODE,arg)
			setHplcTestMode(*(U8*)arg);
            break;
        case NET_SET_EDGE://net_nnib_ioctl(NET_SET_TSTMODE,arg)
            SetnnibEdgetype(*(U8*)arg);
            break;
        case NET_SET_OFFLINE://net_nnib_ioctl(NET_SET_OFFLINE,arg)
            ClearNNIB();
            break;
        case NET_SET_MACTYPE://net_nnib_ioctl(NET_SET_OFFLINE,arg)
            __memcpy((U8*)&nnib.MacAddrType,(U8*)arg,sizeof(U8));
            break;
		default:
			break;
		


		}
	
    mutex_post(&task_work_mutex);

//    if(task_query() != pid_app_work)
//        net_printf("n_ctl\n");
}





















