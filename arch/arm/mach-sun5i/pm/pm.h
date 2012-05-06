#ifndef _PM_H
#define _PM_H

/*
 * Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
 
#include "pm_config.h"
#include "pm_errcode.h"
#include "pm_debug.h"
#include "mem_cpu.h"


struct mmu_state {
	/* CR0 */
	__u32 cssr;	/* Cache Size Selection */
	/* CR1 */
	__u32 cr;		/* Control */
	__u32 cacr;	/* Coprocessor Access Control */
	/* CR2 */
	__u32  ttb_0r;	/* Translation Table Base 0 */
	__u32  ttb_1r;	/* Translation Table Base 1 */
	__u32  ttbcr;	/* Translation Talbe Base Control */
	
	/* CR3 */
	__u32 dacr;	/* Domain Access Control */

	/*cr10*/
	__u32 prrr;	/* Primary Region Remap Register */
	__u32 nrrr;	/* Normal Memory Remap Register */
};

/**
*@brief struct of super mem
*/
struct aw_mem_para{
	void **resume_pointer;
	__u32 mem_flag;
	__u32 suspend_vdd;
	__u32 suspend_freq;
	__u32 saved_runtime_context_svc[RUNTIME_CONTEXT_SIZE];
	__u32 saved_dram_training_area[DRAM_TRANING_SIZE];
	struct mmu_state saved_mmu_state;
	struct saved_context saved_cpu_context;
};


static inline __u32 raw_lib_udiv(__u32 dividend, __u32 divisior)
{
    __u32   tmpDiv = (__u32)divisior;
    __u32   tmpQuot = 0;
    __s32   shift = 0;

    if(!divisior)
    {
        /* divide 0 error abort */
        return 0;
    }

    while(!(tmpDiv & ((__u32)1<<31)))
    {
        tmpDiv <<= 1;
        shift ++;
    }

    do
    {
        if(dividend >= tmpDiv)
        {
            dividend -= tmpDiv;
            tmpQuot = (tmpQuot << 1) | 1;
        }
        else
        {
            tmpQuot = (tmpQuot << 1) | 0;
        }
        tmpDiv >>= 1;
        shift --;
    } while(shift >= 0);

    return tmpQuot;
}


#endif /*_PM_H*/
