/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_tmr.c
* By      : gqyang.young
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "mem_i.h"

static __standby_tmr_reg_t  *TmrReg;
/*
*********************************************************************************************************
*                                     TIMER INIT
*
* Description: initialise timer for mem.
*
* Arguments  : none
*
* Returns    : EPDK_TRUE/EPDK_FALSE;
*********************************************************************************************************
*/
void mem_tmr_init(void)
{
    /* set timer register base */
    TmrReg = (__standby_tmr_reg_t *)SW_VA_TIMERC_IO_BASE;
    
    return;    
}

/*
*********************************************************************************************************
*                           mem_tmr_disable_watchdog
*
*Description: disable watch-dog.
*
*Arguments  : none.
*
*Return     : none;
*
*Notes      :
*
*********************************************************************************************************
*/
void mem_tmr_disable_watchdog(void)
{
	/* set timer register base */
	//TmrReg = (__standby_tmr_reg_t *)SW_VA_TIMERC_IO_BASE;
	/* disable watch-dog reset */
	TmrReg->DogMode &= ~(1<<1);
	/* disable watch-dog */
	TmrReg->DogMode &= ~(1<<0);
	
	return;
}
