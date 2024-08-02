/*
 * Copyright: (c) 2009-2020, 2011 ZC Technology, Inc.
 * All Rights Reserved.
 *
 * File:	printf_zc.c
 * Purpose:	
 * History:
 * Author : PanYu
 */

#include "sys.h"
#include "Types.h"
#include "Trios.h"




void dump_zc(uint8_t delay,char *s,uint32_t debug,uint8_t * buf,uint32_t len)
{
	uint32_t i;

	if(s)debug_printf(&dty, debug, "%s\t",s);
	if(delay)os_sleep(1);
	for (i = 0; i < len; i++)
	{
	  	if (i && (i % 4) == 0)
		   debug_printf(&dty, debug, " ");
		
	   if (i && (i % 32) == 0)
	   	{
	   	   if(delay)os_sleep(2);
		   debug_printf(&dty, debug, "\n\t");
	   	}
	 
	   
	   debug_printf(&dty, debug, "%02x", *(buf + i));
	}
	debug_printf(&dty, debug, "\n");
}

void dump_buf(uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i+=10)
    {
        if (i && (i % 100) == 0)
            debug_printf(&dty, DEBUG_APP, "\n");
        if(len>10*(i+1))
        {
            debug_printf(&dty, DEBUG_APP, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x "
                , *(buf + i),*(buf + i+1),*(buf + i+2),*(buf + i+3),*(buf + i+4)
                , *(buf + i+5),*(buf + i+6),*(buf + i+7),*(buf + i+8),*(buf + i+9));
        }
        else
        {
            for (; i < len; i++)
            {
                debug_printf(&dty, DEBUG_APP, "%02x ", *(buf + i));
            }
        }
        //debug_printf(&dty, DEBUG_APP, "%02x %02x %02x %02x ", *(buf + i),*(buf + i+1),*(buf + i+2),*(buf + i+3));
    }
    debug_printf(&dty, DEBUG_APP, "\n");
}

void dump_level_buf(uint32_t level, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        if (i && (i % 100) == 0)
            debug_printf(&dty, level, "\n");
        debug_printf(&dty, level, "%02x ", *(buf + i));
    }
    debug_printf(&dty, level, "\n");
}

void dump_printfs(uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        if (i && (i % 100) == 0)
            printf_s("\n");
        printf_s("%02x ", *(buf + i));
    }
    printf_s("\n");
}
void dump_tprintf(tty_t *term, uint8_t *buf, uint32_t len)
{
    uint32_t i;

    for (i = 0; i < len; i++)
    {
        if (i && (i % 100) == 0)
            term->op->tprintf(term,"\n");
        term->op->tprintf(term,"%02x ", *(buf + i));
    }
    term->op->tprintf(term,"\n");
}



