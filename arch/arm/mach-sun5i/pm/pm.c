/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager for allwinners chips platform.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <linux/power/aw_pm.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>
#include "pm_i.h"
#include <linux/sw-uart-dbg.h>
#include "mem_cpu.h"

//#define CROSS_MAPPING_STANDBY
#define AW_PM_DBG   1
#undef PM_DBG
#if(AW_PM_DBG)
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif 

#ifdef ENTER_SUPER_STANDBY
#undef PRE_DISABLE_MMU
#endif

#ifdef ENTER_SUPER_STANDBY_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#undef PRE_DISABLE_MMU
#endif

#ifdef WATCH_DOG_RESET
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif 

//#define VERIFY_RESTORE_STATUS

/* define major number for power manager */
#define AW_PMU_MAJOR    267


extern char *standby_bin_start;
extern char *standby_bin_end;
extern char *suspend_bin_start;
extern char *suspend_bin_end;
extern char *resume0_bin_start;
extern char *resume0_bin_end;

extern int mem_arch_suspend(void);
extern int mem_arch_resume(void);
extern asmlinkage int mem_clear_runtime_context(void);
extern asmlinkage int mem_restore_runtime_context(void);
extern asmlinkage int mem_save_runtime_context(void);

extern void disable_cache(void);
extern void disable_program_flow_prediction(void);
extern void invalidate_branch_predictor(void);
extern void enable_cache(void);
extern void enable_program_flow_prediction(void);
extern void disable_dcache(void);
extern void disable_l2cache(void);
extern void set_ttbr0(void);
typedef  int (*suspend_func)(void);
void jump_to_suspend(__u32 ttbr1, suspend_func p);
extern void clear_reg_context(void);
extern void invalidate_dcache(void);

int (*mem)(void) = 0;

#if 0
//pre-allocate an area for saving dram training area
__u32 mem_dram_traning_area_back[DRAM_TRANING_SIZE];
__u32 mem_dram_backup_area[DRAM_BACKUP_SIZE];
__u32 mem_dram_backup_area1[DRAM_BACKUP_SIZE1];
__u32 mem_dram_backup_area2[DRAM_BACKUP_SIZE2]; //for training area
__u32 mem_dram_backup_compare_area2[DRAM_COMPARE_SIZE]; //for compare area
#else
static __u32 *mem_dram_traning_area_back = NULL;
static __u32 *mem_dram_backup_area = NULL;
static __u32 *mem_dram_backup_area1 = NULL;
static __u32 *mem_dram_backup_area2 = NULL;
static __u32 *mem_dram_backup_compare_area2 = NULL;
#endif

extern void cpufreq_user_event_notify(void);

static struct map_desc mem_sram_md = { 
	.virtual = MEM_SW_VA_SRAM_BASE,         
	.pfn = __phys_to_pfn(MEM_SW_PA_SRAM_BASE),         
	.length = SZ_1M, 
	.type = MT_MEMORY_ITCM,  
 	};

static struct aw_pm_info standby_info = {
    .standby_para = {
        .event = SUSPEND_WAKEUP_SRC_EXINT,
    },
    .pmu_arg = {
        .twi_port = 0,
        .dev_addr = 10,
    },
};

static struct int_state saved_int_state;
static struct clk_state saved_clk_state;
static struct tmr_state saved_tmr_state;
static struct twi_state saved_twi_state;
static struct gpio_state saved_gpio_state;
static struct sram_state saved_sram_state;

#ifdef GET_CYCLE_CNT
static int start = 0;
static int resume0_period = 0;
static int resume1_period = 0;

static int pm_start = 0;
static int invalidate_data_time = 0;
static int invalidate_instruct_time = 0;
static int before_restore_processor = 0;
static int after_restore_process = 0;
static int restore_runtime_peroid = 0;

//late_resume timing
static int late_resume_start = 0;
static int backup_area_start = 0;
static int backup_area1_start = 0;
static int backup_area2_start = 0;
static int clk_restore_start = 0;
static int gpio_restore_start = 0;
static int twi_restore_start = 0;
static int int_restore_start = 0;
static int tmr_restore_start = 0;	
static int sram_restore_start = 0;
static int late_resume_end = 0;
#endif

struct aw_mem_para mem_para_info;
standby_type_e standby_type = NON_STANDBY;
EXPORT_SYMBOL(standby_type);

//static volatile int enter_flag = 0;
volatile int print_flag = 0;
static bool mem_allocated_flag = false;
extern void create_mapping(struct map_desc *md);
extern void save_mapping(unsigned long vaddr);
extern void restore_mapping(unsigned long vaddr);
extern void save_mmu_state(struct mmu_state *saved_mmu_state);
extern void clear_mem_flag(void);
extern void save_runtime_context(__u32 *addr);
void restore_processor_ttbr0(void);
extern void flush_icache(void);
extern void flush_dcache(void);



/*
*********************************************************************************************************
*                           aw_pm_valid
*
*Description: determine if given system sleep state is supported by the platform;
*
*Arguments  : state     suspend state;
*
*Return     : if the state is valid, return 1, else return 0;
*
*Notes      : this is a call-back function, registered into PM core;
*
*********************************************************************************************************
*/
static int aw_pm_valid(suspend_state_t state)
{
    PM_DBG("valid\n");

    if(!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))){
        PM_DBG("state (%d) invalid!\n", state);
        return 0;
    }
	if(PM_SUSPEND_STANDBY == state){
#ifdef CROSS_MAPPING_STANDBY
	standby_type = SUPER_STANDBY;
#else
	standby_type = NORMAL_STANDBY;
#endif
	}else if(PM_SUSPEND_MEM == state){
#ifdef CROSS_MAPPING_STANDBY
	standby_type = NORMAL_STANDBY;
#else
	standby_type = SUPER_STANDBY;
#endif
	}
	
	//allocat space for backup dram data
	if(false == mem_allocated_flag){
		mem_dram_traning_area_back = (__u32*)kmalloc(sizeof(__u32)*DRAM_TRANING_SIZE, GFP_KERNEL);
		mem_dram_backup_area = (__u32*)kmalloc(sizeof(__u32)*DRAM_BACKUP_SIZE, GFP_KERNEL);
		mem_dram_backup_area1 = (__u32*)kmalloc(sizeof(__u32)*DRAM_BACKUP_SIZE1, GFP_KERNEL);
		mem_dram_backup_area2 = (__u32*)kmalloc(sizeof(__u32)*DRAM_BACKUP_SIZE2, GFP_KERNEL);
		mem_dram_backup_compare_area2 = (__u32*)kmalloc(sizeof(__u32)*DRAM_COMPARE_SIZE, GFP_KERNEL);	
		mem_allocated_flag = true;	
	}

	//print_flag = 0;
    return 1;
}


/*
*********************************************************************************************************
*                           aw_pm_begin
*
*Description: Initialise a transition to given system sleep state;
*
*Arguments  : state     suspend state;
*
*Return     : return 0 for process successed;
*
*Notes      : this is a call-back function, registered into PM core, and this function
*             will be called before devices suspened;
*********************************************************************************************************
*/
int aw_pm_begin(suspend_state_t state)
{
    PM_DBG("%d state begin\n", state);
	clear_mem_flag();

	if(PM_SUSPEND_STANDBY == state){
#ifdef CROSS_MAPPING_STANDBY
		standby_type = SUPER_STANDBY;
#else
		standby_type = NORMAL_STANDBY;
#endif
	}else if(PM_SUSPEND_MEM == state){
#ifdef CROSS_MAPPING_STANDBY
		standby_type = NORMAL_STANDBY;
#else
		standby_type = SUPER_STANDBY;
#endif
	}
	//set freq max
	cpufreq_user_event_notify();

#ifdef GET_CYCLE_CNT
	// init counters:
	init_perfcounters (1, 0);
#endif

	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare
*
*Description: Prepare the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core, this function
*             will be called after devices suspended, and before device late suspend
*             call-back functions;
*********************************************************************************************************
*/
int aw_pm_prepare(void)
{
    PM_DBG("prepare\n");

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_prepare_late
*
*Description: Finish preparing the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core.
*             prepare_late is called before disabling nonboot CPUs and after
*              device drivers' late suspend callbacks have been executed;
*********************************************************************************************************
*/
int aw_pm_prepare_late(void)
{
    PM_DBG("prepare_late\n");

    return 0;
}

/*
*********************************************************************************************************
*                           aw_early_suspend
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      : 
*********************************************************************************************************
*/
static void aw_early_suspend(void)
{
	save_mem_status(EARLY_SUSPEND_START | 0x101);
	//busy_waiting();
	save_mem_status(EARLY_SUSPEND_START | 0x102);
	mem_clk_save(&(saved_clk_state));

	save_mem_status(EARLY_SUSPEND_START | 0x103);
	mem_gpio_save(&(saved_gpio_state));
	
	save_mem_status(EARLY_SUSPEND_START | 0x104);
	//busy_waiting();
	mem_tmr_save(&(saved_tmr_state));

	save_mem_status(EARLY_SUSPEND_START | 0x105);
	//busy_waiting();
	mem_twi_save(&(saved_twi_state));
	
	save_mem_status(EARLY_SUSPEND_START | 0x106);
	mem_int_save(&(saved_int_state));

	mem_sram_save(&(saved_sram_state));
	
	//backup mmu
	//busy_waiting();
	save_mmu_state(&(mem_para_info.saved_mmu_state));
	//backup cpu state
	__save_processor_state(&(mem_para_info.saved_cpu_context));
	//backup 0x0000,0000 page entry, size?
	save_mem_status(EARLY_SUSPEND_START | 0x107);
	//busy_waiting();
	save_mapping(MEM_SW_VA_SRAM_BASE);

	save_mem_status(EARLY_SUSPEND_START | 0x108);
	//busy_waiting();
	//backup dram area to leave space for resume0 code
	memcpy((void *)mem_dram_backup_area, (void *)DRAM_BACKUP_BASE_ADDR, sizeof(__u32)*DRAM_BACKUP_SIZE);
	dmac_flush_range((void *)mem_dram_backup_area, (void *)(mem_dram_backup_area + (sizeof(u32)) * DRAM_BACKUP_SIZE - 1));
	
	//backup dram area to reserve space for para space
	save_mem_status(EARLY_SUSPEND_START  |0x109);
	//busy_waiting();
	memcpy((void *)mem_dram_backup_area1, (void *)DRAM_BACKUP_BASE_ADDR1, sizeof(__u32)*DRAM_BACKUP_SIZE1);
	dmac_flush_range((void *)mem_dram_backup_area1, (void *)(mem_dram_backup_area1 + (sizeof(u32)) * DRAM_BACKUP_SIZE1 - 1));

	memcpy((void *)mem_dram_backup_area2, (void *)DRAM_BACKUP_BASE_ADDR2, sizeof(__u32)*DRAM_BACKUP_SIZE2);
	dmac_flush_range((void *)mem_dram_backup_area2, (void *)(mem_dram_backup_area2 + (sizeof(u32)) * DRAM_BACKUP_SIZE2 - 1));

#if 0
	//reserved space for compare data
	memcpy((void *)mem_dram_backup_compare_area2, (void *)DRAM_COMPARE_DATA_ADDR, sizeof(__u32)*DRAM_COMPARE_SIZE);
	dmac_flush_range((void *)mem_dram_backup_compare_area2, (void *)(mem_dram_backup_compare_area2 + (sizeof(u32)) * DRAM_COMPARE_SIZE - 1));
#endif

	save_mem_status(EARLY_SUSPEND_START | 0x10a);
	//busy_waiting();
	/* backup dram traning area: waring, access dram*/
	//memcpy((void *)mem_dram_traning_area_back, (void *)DRAM_BASE_ADDR, sizeof(__u32)*DRAM_TRANING_SIZE);
	//memcpy((void *)(mem_para_info.saved_dram_training_area), (void *)DRAM_BASE_ADDR, sizeof(__u32)*DRAM_TRANING_SIZE);
		
	
	//store resume0 code in sdram's fixed addr, and boot will jump to that for resume
	//decided when link&load
	
	/*corresponding to these following steps, sth need to be done when 
	 *restore from sram when power on
	 */
	//backup resume addr

	//prepare resume0 code for resume
	if((sizeof(__u32)*DRAM_BACKUP_SIZE) < ((int)&resume0_bin_end - (int)&resume0_bin_start) ){
		//judge the reserved space for resume0 is enough or not.
		save_mem_status(EARLY_SUSPEND_START | 0x10b);
		busy_waiting();
	}
	if((sizeof(__u32)*DRAM_BACKUP_SIZE1) < sizeof(mem_para_info)){
		//judge the reserved space for mem para is enough or not.
		save_mem_status(EARLY_SUSPEND_START | 0x10c);
		busy_waiting();
	}
	//busy_waiting();
	memcpy((void *)DRAM_BACKUP_BASE_ADDR, (void *)&resume0_bin_start, (int)&resume0_bin_end - (int)&resume0_bin_start);
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR, (void *)(DRAM_BACKUP_BASE_ADDR + (sizeof(u32)) * DRAM_BACKUP_SIZE -1) );
	//clean_dcache_area((void *)DRAM_BACKUP_BASE_ADDR, (sizeof(u32)) * DRAM_BACKUP_SIZE1) );
	//busy_waiting();
	
	//clean all the data into dram
	memcpy((void *)DRAM_BACKUP_BASE_ADDR1, (void *)&mem_para_info, sizeof(mem_para_info));
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR1, (void *)(DRAM_BACKUP_BASE_ADDR1 + (sizeof(u32)) * DRAM_BACKUP_SIZE1 - 1));
	save_mem_status(EARLY_SUSPEND_START | 0x10d);
	//busy_waiting();

	//prepare dram training area data
	memcpy((void *)DRAM_BACKUP_BASE_ADDR2, (void *)DRAM_BASE_ADDR, sizeof(__u32)*DRAM_TRANING_SIZE);
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR2, (void *)(DRAM_BACKUP_BASE_ADDR2 + (sizeof(u32)) * DRAM_BACKUP_SIZE2 - 1));


	//dmac_flush_range((void *)0x00000000, (void *)(0xffffffff-1));
	save_mem_status(EARLY_SUSPEND_START | 0x10e);
	
	mem_arch_suspend();
	save_processor_state(); 
	
	
	save_mem_status(EARLY_SUSPEND_START | 0x10f);
	//busy_waiting();
	//before creating mapping, build the coherent between cache and memory

	//clean and flush
	__cpuc_flush_kern_all();
	__cpuc_flush_user_all();

	__cpuc_coherent_user_range(0x00000000, 0xc0000000-1);
	save_mem_status(EARLY_SUSPEND_START | 0x110);
	
	__cpuc_coherent_kern_range(0xc0000000, 0xffffffff-1);
	save_mem_status(EARLY_SUSPEND_START | 0x111);

	//create 0x0000,0000 mapping table: 0x0000,0000 -> 0x0000,0000 
	create_mapping(&mem_sram_md);
	
#ifdef PRE_DISABLE_MMU
	//pr_info("PRE_DISABLE_MMU %s: %s, %d. \n", __FILE__,  __func__, __LINE__);
	//busy_waiting();
	save_mem_status(EARLY_SUSPEND_START | 0x112);
	//jump to sram: dram enter selfresh, and power off.
	mem = (int (*)(void))SRAM_FUNC_START_PA;
	//move standby code to sram
	memcpy((void *)SRAM_FUNC_START, (void *)&suspend_bin_start, (int)&suspend_bin_end - (int)&suspend_bin_start);
	save_mem_status(EARLY_SUSPEND_START | 0x113);
	//busy_waiting();

	save_mem_status(EARLY_SUSPEND_START | 0x114);
	//busy_waiting();
	//set_ttbr0();
	//mem();
	//create_mapping(&mem_sram_md);
	save_mem_status(EARLY_SUSPEND_START | 0x115);
	//busy_waiting();
	save_mem_status(EARLY_SUSPEND_START | 0x116);
	
#else

	//jump to sram: dram enter selfresh, and power off.
	mem = (int (*)(void))SRAM_FUNC_START;
	//move standby code to sram
	memcpy((void *)SRAM_FUNC_START, (void *)&suspend_bin_start, (int)&suspend_bin_end - (int)&suspend_bin_start);

#endif


#if 0
/*warning: this api will effetc reg value. */
	flush_icache();
	save_mem_status(EARLY_SUSPEND_START | 0x21);
	busy_waiting();
	flush_dcache();
	busy_waiting();
	save_mem_status(EARLY_SUSPEND_START | 0x22);
	flush_tlb_all();
	
	disable_cache();
	disable_program_flow_prediction();
	invalidate_branch_predictor();
	//clean i/d cache
	flush_icache();
	busy_waiting();
	save_mem_status(EARLY_SUSPEND_START | 0x24);
	flush_dcache();
	flush_tlb_all();
#endif	

	//before suspend, backup runtime context.
	//busy_waiting();	
	//save_mem_status(EARLY_SUSPEND_START);
	//busy_waiting();


#if 0
	enable_cache();
	enable_program_flow_prediction();
#endif

	//save_mem_status(EARLY_SUSPEND_START |0x01);
	//busy_waiting();
	
	
	//save_mem_status(EARLY_SUSPEND_START |0x02);
		
	///check the stack status
	save_mem_status(EARLY_SUSPEND_START | 0x117);
	//busy_waiting();
	
#ifdef PRE_DISABLE_MMU
	//busy_waiting();
	//enable the mapping and jump
	jump_to_suspend(mem_para_info.saved_cpu_context.ttb_1r, mem);
#else
	mem();
#endif

}

/*
*********************************************************************************************************
*                           verify_restore
*
*Description: verify src and dest region is the same;
*
*Return     : 0: same;
*                -1: different;
*
*Notes      : 
*********************************************************************************************************
*/
#ifdef VERIFY_RESTORE_STATUS
static int verify_restore(void *src, void *dest, int count)
{
	volatile char *s = (volatile char *)src;
	volatile char *d = (volatile char *)dest;
	while(--count){
		if(*(s+count) != *(d+count)){
			return -1;
		}
	}

	return 0;
}
#endif

/*
*********************************************************************************************************
*                           aw_late_resume
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      : 
*********************************************************************************************************
*/
static void aw_late_resume(void)
{

#ifdef VERIFY_RESTORE_STATUS
	int ret = 0;
#endif
	
	//resume procedure
	//cpu_init();
	/*restore pmu config*/
	//busy_waiting();
#ifdef GET_CYCLE_CNT
	late_resume_start = get_cyclecount();
#endif

	save_mem_status(LATE_RESUME_START |0x04);

	//busy_waiting();
	memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1), sizeof(mem_para_info));
	mem_para_info.mem_flag = 0;
	
	/* restore dram training area: need access mmu&dram */
	save_mem_status(LATE_RESUME_START |0x05);
	//busy_waiting();
	//memcpy((void *)DRAM_BASE_ADDR, (void *)mem_dram_traning_area_back, sizeof(__u32)*DRAM_TRANING_SIZE);

	//restore dram backup area1
	//busy_waiting();
#ifdef GET_CYCLE_CNT
	backup_area2_start = get_cyclecount();
#endif
	memcpy((void *)DRAM_BACKUP_BASE_ADDR2, (void *)mem_dram_backup_area2, sizeof(__u32)*DRAM_BACKUP_SIZE2);
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR2, (void *)(DRAM_BACKUP_BASE_ADDR2 + (sizeof(u32)) * DRAM_BACKUP_SIZE2 - 1));
	
	save_mem_status(LATE_RESUME_START |0x06);
#ifdef GET_CYCLE_CNT
	backup_area1_start = get_cyclecount();
#endif
	memcpy((void *)DRAM_BACKUP_BASE_ADDR1, (void *)mem_dram_backup_area1, sizeof(__u32)*DRAM_BACKUP_SIZE1);
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR1, (void *)(DRAM_BACKUP_BASE_ADDR1 + (sizeof(u32)) * DRAM_BACKUP_SIZE1 - 1));
	
	//restore dram backup area
	//busy_waiting();
	save_mem_status(LATE_RESUME_START |0x07);
#ifdef GET_CYCLE_CNT
	backup_area_start = get_cyclecount();
#endif
	memcpy((void *)DRAM_BACKUP_BASE_ADDR, (void *)mem_dram_backup_area, sizeof(__u32)*DRAM_BACKUP_SIZE);
	dmac_flush_range((void *)DRAM_BACKUP_BASE_ADDR, (void *)(DRAM_BACKUP_BASE_ADDR + (sizeof(u32)) * DRAM_BACKUP_SIZE -1) );

#ifdef VERIFY_RESTORE_STATUS
	if(ret = verify_restore((void *)mem_dram_backup_area2, (void *)DRAM_BACKUP_BASE_ADDR2, sizeof(__u32)*DRAM_BACKUP_SIZE2)){
		save_mem_status(LATE_RESUME_START |0x21);
		busy_waiting();
	}

	if(ret = verify_restore((void *)mem_dram_backup_area1, (void *)DRAM_BACKUP_BASE_ADDR1, sizeof(__u32)*DRAM_BACKUP_SIZE1)){
		save_mem_status(LATE_RESUME_START |0x22);
		busy_waiting();
	}

	if(ret = verify_restore((void *)mem_dram_backup_area, (void *)DRAM_BACKUP_BASE_ADDR, sizeof(__u32)*DRAM_BACKUP_SIZE)){
		save_mem_status(LATE_RESUME_START |0x23);
		busy_waiting();
	}
#endif

	save_mem_status(LATE_RESUME_START |0x08);
	//busy_waiting();
#ifdef GET_CYCLE_CNT
	clk_restore_start = get_cyclecount();
#endif
	mem_clk_restore(&(saved_clk_state));
	
	save_mem_status(LATE_RESUME_START |0x09);
#ifdef GET_CYCLE_CNT
	gpio_restore_start = get_cyclecount();
#endif
	mem_gpio_restore(&(saved_gpio_state));
	//busy_waiting();
	
	save_mem_status(LATE_RESUME_START |0x0c);
#ifdef GET_CYCLE_CNT
	twi_restore_start = get_cyclecount();
#endif
	mem_twi_restore(&(saved_twi_state));
	//busy_waiting();
	
	save_mem_status(LATE_RESUME_START |0x09);
	//busy_waiting();
#ifdef GET_CYCLE_CNT
	tmr_restore_start = get_cyclecount();
#endif
	mem_tmr_restore(&(saved_tmr_state));
	
	save_mem_status(LATE_RESUME_START |0x0a);
#ifdef GET_CYCLE_CNT
	int_restore_start = get_cyclecount();
#endif
	mem_int_restore(&(saved_int_state));
	
	save_mem_status(LATE_RESUME_START |0x0b);
#ifdef GET_CYCLE_CNT
	sram_restore_start = get_cyclecount();
#endif
	mem_sram_restore(&(saved_sram_state));

#ifdef GET_CYCLE_CNT
	late_resume_end = get_cyclecount();
#endif

#ifdef GET_CYCLE_CNT
	printk("late_resume_start = 0x%x. \n", late_resume_start);
	printk("backup_area2_start = 0x%x. \n", backup_area2_start);
	printk("backup_area1_start = 0x%x. \n", backup_area1_start);
	printk("backup_area_start = 0x%x. \n", backup_area_start);
	printk("clk_restore_start = 0x%x. \n", clk_restore_start);
	printk("gpio_restore_start = 0x%x. \n", gpio_restore_start);
	printk("twi_restore_start = 0x%x. \n", twi_restore_start);
	printk("tmr_restore_start = 0x%x. \n", tmr_restore_start);
	printk("int_restore_start = 0x%x. \n", int_restore_start);
	printk("sram_restore_start = 0x%x. \n", sram_restore_start);
	printk("late_resume_end = 0x%x. \n", late_resume_end);
#endif
	
	//busy_waiting();
}

/*
*********************************************************************************************************
*                           aw_pm_enter
*
*Description: Enter the system sleep state;
*
*Arguments  : state     system sleep state;
*
*Return     : return 0 is process successed;
*
*Notes      : this function is the core function for platform sleep.
*********************************************************************************************************
*/
static int aw_pm_enter(suspend_state_t state)
{
	asm volatile ("stmfd sp!, {r1-r12, lr}" );
	int (*standby)(struct aw_pm_info *arg) = 0;
	//static int enter_flag = 0;

#if 1	
	save_mem_status(BEFORE_EARLY_SUSPEND |0x01);
	//busy_waiting();
	
	PM_DBG("enter state %d\n", state);
        
	if(NORMAL_STANDBY== standby_type){
		//busy_waiting();
		standby = (int (*)(struct aw_pm_info *arg))SRAM_FUNC_START;
		//busy_waiting();
		//move standby code to sram
		memcpy((void *)SRAM_FUNC_START, (void *)&standby_bin_start, (int)&standby_bin_end - (int)&standby_bin_start);
		//busy_waiting();
		/* config system wakeup evetn type */
		standby_info.standby_para.event = SUSPEND_WAKEUP_SRC_EXINT | SUSPEND_WAKEUP_SRC_ALARM;
		/* goto sram and run */
		standby(&standby_info);
		//udelay(10);
		//busy_waiting();
		save_mem_status(LATE_RESUME_START |0x30);
		mdelay(10);	//add delay before jump, more stable. reason? 
		//busy_waiting();
	}else if(SUPER_STANDBY == standby_type){
mem_enter:
		save_mem_status(BEFORE_EARLY_SUSPEND |0x02);
		//busy_waiting();
		//invalidate period
		
		if( 1 == mem_para_info.mem_flag){
			//check the stack status
			//busy_waiting();
#if 1
			//after disable cache, u need to sync cache
			//disable_dcache();
			//disable_cache();
			//disable_l2cache();
			//disable_program_flow_prediction();
#ifdef GET_CYCLE_CNT
			resume0_period = *(volatile __u32 *)(0xf1c20d28);
			resume1_period = *(volatile __u32 *)(0xf1c20d2c);
			pm_start = get_cyclecount();
			
			start = *(volatile __u32 *)(0xf1c20d20);
			invalidate_data_time = get_cyclecount() - start;
			//*(volatile __u32 *)(PERMANENT_REG  + 0x00) = invalidate_data_time;
			*(volatile __u32 *)(PERMANENT_REG  + 0x00) = 0;
			
			invalidate_instruct_time = get_cyclecount();
#endif
			//busy_waiting();
			
			invalidate_branch_predictor();
			//flush_icache();
			//flush_dcache();
			
			//save_mem_status(LATE_RESUME_START |0x10);
			//busy_waiting();
			//flush_tlb_all();
			//save_mem_status(LATE_RESUME_START |0x1e);
			//busy_waiting();
			
			//must be called to invalidate I-cache inner shareable?
			// I+BTB cache invalidate
			__cpuc_flush_icache_all();
			//__cpuc_flush_kern_all();
			//__cpuc_flush_user_all();
			//clean i/d cache
			//flush_icache();
			/*notice: will corrupt r0 - r11*/
			//busy_waiting();
			//invalidate_dcache();
			//construc r0-r11
			//clear_reg_context();
			//flush_tlb_all();
#ifdef GET_CYCLE_CNT
			invalidate_instruct_time = get_cyclecount() - invalidate_instruct_time;

			before_restore_processor = get_cyclecount();
#endif
			//disable 0x0000 <---> 0x0000 mapping
			//busy_waiting();
 			restore_processor_state();
 #ifdef GET_CYCLE_CNT
 			after_restore_process = get_cyclecount();
 #endif
 			
 			//destroy 0x0000 <---> 0x0000 mapping
			restore_mapping(MEM_SW_VA_SRAM_BASE);
			
			//flush_tlb_all();
			
			mem_arch_resume();

			
			//before suspend and after resume, require context be the same 
			//save_mem_status(LATE_RESUME_START |0x0c);
			//how the 0x0000,0000 space invalid? flush tlb
			//busy_waiting(); 
			

			//save_mem_status(LATE_RESUME_START |0x0d);
			//busy_waiting();	

			save_mem_status(LATE_RESUME_START |0x0e);
			//busy_waiting();

			//flush tlb?

			//
#endif	
			goto resume;
		}
		save_mem_status(BEFORE_EARLY_SUSPEND | 0x03);
		//busy_waiting();
		save_runtime_context(mem_para_info.saved_runtime_context_svc);
		mem_para_info.mem_flag = 1;
		save_mem_status(BEFORE_EARLY_SUSPEND | 0x04);
		//busy_waiting();
		save_mem_status(BEFORE_EARLY_SUSPEND | 0x05);
		//busy_waiting();
		mem_para_info.resume_pointer = (void *)&&mem_enter;
		//busy_waiting();
		save_mem_status(BEFORE_EARLY_SUSPEND | 0x06);

		//busy_waiting();
		aw_early_suspend();
		
resume:

		//here, uart not ok, need after clk restore?
		//printk("%s: %s, %d. \n", __FILE__,  __func__, __LINE__);
		save_mem_status(LATE_RESUME_START | 0x01);
		aw_late_resume();
		
#ifdef GET_CYCLE_CNT
		printk("%s: %s, %d. \n", __FILE__,  __func__, __LINE__);

		printk("resume0 time =%x, resume1 time = %x\n", resume0_period, resume1_period);
		printk("pm_start = %x after_late_resume = %x \n", pm_start, get_cyclecount());
		
		
		printk("invalidate_data_time = %x, before_restore_processor = %x, after_restore_process = %x. \
			restore_period = %x \n", invalidate_data_time, before_restore_processor, \
			after_restore_process, after_restore_process - before_restore_processor);
#endif
				
	}

#endif
	save_mem_status(LATE_RESUME_START |0x40);
#if 1
	//busy_waiting();
	//print_flag = 1;
	//busy_waiting();
	//uart_init(2, 115200);
#endif
	
	//busy_waiting();
	save_mem_status(LATE_RESUME_START |0x41);
	//usy_waiting();
	asm volatile ("ldmfd sp!, {r1-r12, lr}" );
	return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_wake
*
*Description: platform wakeup;
*
*Arguments  : none;
*
*Return     : none;
*
*Notes      : This function called when the system has just left a sleep state, right after
*             the nonboot CPUs have been enabled and before device drivers' early resume
*             callbacks are executed. This function is opposited to the aw_pm_prepare_late;
*********************************************************************************************************
*/
static void aw_pm_wake(void)
{
    PM_DBG("platform wakeup, wakesource is:0x%x\n", standby_info.standby_para.event);
}


/*
*********************************************************************************************************
*                           aw_pm_finish
*
*Description: Finish wake-up of the platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called right prior to calling device drivers' regular suspend
*              callbacks. This function is opposited to the aw_pm_prepare function.
*********************************************************************************************************
*/
void aw_pm_finish(void)
{
    PM_DBG("platform wakeup finish\n");
}


/*
*********************************************************************************************************
*                           aw_pm_end
*
*Description: Notify the platform that system is in work mode now.
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called by the PM core right after resuming devices, to indicate to
*             the platform that the system has returned to the working state or
*             the transition to the sleep state has been aborted. This function is opposited to
*             aw_pm_begin function.
*********************************************************************************************************
*/
void aw_pm_end(void)
{
	
	//standby_type = NON_STANDBY;
	//uart_init(2, 115200);
	save_mem_status(LATE_RESUME_START |0x10);
	//print_flag = 0;
	PM_DBG("aw_pm_end!\n");
}


/*
*********************************************************************************************************
*                           aw_pm_recover
*
*Description: Recover platform from a suspend failure;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function alled by the PM core if the suspending of devices fails.
*             This callback is optional and should only be implemented by platforms
*             which require special recovery actions in that situation.
*********************************************************************************************************
*/
void aw_pm_recover(void)
{
    PM_DBG("aw_pm_recover\n");
}


/*
    define platform_suspend_ops which is registered into PM core.
*/
static struct platform_suspend_ops aw_pm_ops = {
    .valid = aw_pm_valid,
    .begin = aw_pm_begin,
    .prepare = aw_pm_prepare,
    .prepare_late = aw_pm_prepare_late,
    .enter = aw_pm_enter,
    .wake = aw_pm_wake,
    .finish = aw_pm_finish,
    .end = aw_pm_end,
    .recover = aw_pm_recover,
};


/*
*********************************************************************************************************
*                           aw_pm_init
*
*Description: initial pm sub-system for platform;
*
*Arguments  : none;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_pm_init(void)
{
    PM_DBG("aw_pm_init!\n");
    suspend_set_ops(&aw_pm_ops);

    return 0;
}


/*
*********************************************************************************************************
*                           aw_pm_exit
*
*Description: exit pm sub-system on platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void __exit aw_pm_exit(void)
{
	PM_DBG("aw_pm_exit!\n");
	if(true == mem_allocated_flag){
		kfree(mem_dram_traning_area_back);
		kfree(mem_dram_backup_area);
		kfree(mem_dram_backup_area1);
		kfree(mem_dram_backup_area2);
		kfree(mem_dram_backup_compare_area2);
		mem_allocated_flag = false;
	}
	suspend_set_ops(NULL);
}

module_init(aw_pm_init);
module_exit(aw_pm_exit);

