/* Copyright: (c) 2016-2020, 2017 zcpower Technology, Inc.
 * All Rights Reserved.
 *
 * File:        MacsTask.c
 * Purpose:     procedure for using
 * History:
 */
#include "ZCsystem.h"
#include "flash.h"
#include "phy_sal.h"
#include "app_rdctrl.h"
#include "dev_manager.h"
#include "app_dltpro.h"

param_t cmd_macs_param_tab[] =
{
    {PARAM_ARG | PARAM_TOGGLE | PARAM_OPTIONAL , "", "readmacaddr|timeread|recordmeter|HPLC" },
		
    PARAM_EMPTY
};
CMD_DESC_ENTRY(cmd_macs_desc, "", cmd_macs_param_tab);

void docmd_macs(command_t *cmd, xsh_t *xsh)
{
    tty_t *term = xsh->term;
    if(xsh->argc == 1)
    {
        return;
    }

    if (__strcmp(xsh->argv[1], "readmacaddr") == 0)
    {
        term->op->tprintf(term,"sta macaddr : %02x%02x%02x%02x%02x%02x\n"
                          ,nnib.MacAddr[0]
                          ,nnib.MacAddr[1]
                          ,nnib.MacAddr[2]
                          ,nnib.MacAddr[3]
                          ,nnib.MacAddr[4]
                          ,nnib.MacAddr[5]);
#if defined(ZC3750STA) || defined(ZC3750CJQ2)
        term->op->tprintf(term,"dev macaddr : %02x%02x%02x%02x%02x%02x  tei %04x snid %08x type %s prtcl %d\n",
	#if defined(ZC3750STA)         
			DevicePib_t.DevMacAddr[5],
			DevicePib_t.DevMacAddr[4],
			DevicePib_t.DevMacAddr[3],
			DevicePib_t.DevMacAddr[2],
			DevicePib_t.DevMacAddr[1],
			DevicePib_t.DevMacAddr[0],
	#elif defined(ZC3750CJQ2)		
			CollectInfo_st.CollectAddr[5],
			CollectInfo_st.CollectAddr[4],
            CollectInfo_st.CollectAddr[3], 
           	CollectInfo_st.CollectAddr[2], 
            CollectInfo_st.CollectAddr[1], 
            CollectInfo_st.CollectAddr[0],
	#endif
			HPLC.tei,
			HPLC.snid,
			(DevicePib_t.DevType == e_SET_ADDR_APP)?"setaddr":"normaladdr",
			DevicePib_t.Prtcl);
#elif defined(STATIC_MASTER)
        term->op->tprintf(term,"cco macaddr : %02x%02x%02x%02x%02x%02x   tei %04x snid %08x\n",
            FlashInfo_t.ucJZQMacAddr[0],
            FlashInfo_t.ucJZQMacAddr[1],
            FlashInfo_t.ucJZQMacAddr[2],
            FlashInfo_t.ucJZQMacAddr[3],
            FlashInfo_t.ucJZQMacAddr[4],
            FlashInfo_t.ucJZQMacAddr[5],
            HPLC.tei,
            HPLC.snid);
#endif
    }
	else if(__strcmp(xsh->argv[1], "timeread") == 0)
	{
        U32 ii=0;
        term->op->tprintf(term,"----------------------time-----------------\n");
        do
        {
            if(record_time_t.s[ii]==NULL)
            {
                term->op->tprintf(term,"totol num = %d !\n",ii);
                break;
            }
            term->op->tprintf(term,"%-2d || %-25s  0x%08x %d\n",
                ii,record_time_t.s[ii],record_time_t.time[ii],record_time_t.dur[ii]/25);
				 
            ii++;
			
        }while(ii<MAXNUMMEM);
	}
	else if(__strcmp(xsh->argv[1], "recordmeter") == 0 )
	{
		extern void showrecordmeter(tty_t *term);
		showrecordmeter(term);

	}
	else if(__strcmp(xsh->argv[1], "HPLC") == 0 )
	{
        term->op->tprintf(term,"sw_phase=%d\n",HPLC.sw_phase);
	}
}

