/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_tmr.h
* By      : gqyang.young
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_TMR_H__
#define __MEM_TMR_H__
#include "mem_i.h"

void mem_tmr_init(void);
void mem_tmr_disable_watchdog(void);

#endif  /* __MEM_TMR_H__ */
