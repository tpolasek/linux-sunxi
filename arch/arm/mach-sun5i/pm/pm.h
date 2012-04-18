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

#endif /*_PM_H*/
