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

//#define RETURN_FROM_RESUME0_WITH_MMU    //suspend: 0xf000, resume0: 0xc010, resume1: 0xf000
//#define RETURN_FROM_RESUME0_WITH_NOMMU // suspend: 0x0000, resume0: 0x4010, resume1: 0x0000
//#define DIRECT_RETURN_FROM_SUSPEND //not support yet
//#define ENTER_SUPER_STANDBY    //suspend: 0xf000, resume0: 0x4010, resume1: 0x0000
#define ENTER_SUPER_STANDBY_WITH_NOMMU //not support yet, suspend: 0x0000, resume0: 0x4010, resume1: 0x0000
//#define WATCH_DOG_RESET

/**start address for function run in sram*/
#define SRAM_FUNC_START     SW_VA_SRAM_BASE
#define SRAM_FUNC_START_PA (0x00000000)

#define DRAM_BASE_ADDR      0xc0000000
#define DRAM_BASE_ADDR_PA      0x40000000
#define DRAM_TRANING_SIZE   (16)

#define DRAM_BACKUP_BASE_ADDR (0xc0100000) //1Mbytes offset
#define DRAM_BACKUP_SIZE (0x4000) //16K*4 bytes = 64K.

#define DRAM_BACKUP_BASE_ADDR1 (0xc0110000)
#define DRAM_BACKUP_BASE_ADDR1_PA (0x40110000)
#define DRAM_BACKUP_SIZE1 (0x0100) // 2^8 * 4 = 1K bytes.

#define DRAM_BACKUP_BASE_ADDR2 (0xc0110400)
#define DRAM_BACKUP_BASE_ADDR2_PA (0x40110400)
#define DRAM_BACKUP_SIZE2 (0x0100) // 2^8 * 4 = 1K bytes. 

#define RUNTIME_CONTEXT_SIZE (14 * sizeof(__u32)) //r0-r13

#define DRAM_COMPARE_DATA_ADDR (0xc0100000) //1Mbytes offset
#define DRAM_COMPARE_SIZE (0x200000) //16K*4 bytes = 64K.


//for mem mapping
#define MEM_SW_VA_SRAM_BASE (0x00000000)
#define MEM_SW_PA_SRAM_BASE (0x00000000)

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
