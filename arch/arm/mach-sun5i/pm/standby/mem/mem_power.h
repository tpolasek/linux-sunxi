/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_power.h
* By      : gqyang.young
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#ifndef __MEM_POWER_H__
#define __MEM_POWER_H__
#include "mem_i.h"

void mem_power_init(void);
void mem_power_exit(void);
void mem_power_off(void);

#endif  /* __MEM_POWER_H__ */
