/*
 * Contact: gqyang <gqyang <at> allwinnertech.com>                               
 *                                                                                   
 * License terms: GNU General Public License (GPL) version 2                         
 */       
/*
 * the following code need be exceute in sram
 * before dram enter self-refresh,cpu can not access dram.
 */
 
#include "./../mem_i.h"

extern char *__bss_start;
extern char *__bss_end;
extern void standby_flush_tlb(void);
extern void flush_icache(void);
extern void flush_dcache(void);
extern void invalidate_dcache(void);

extern unsigned int save_sp(void);
extern void restore_sp(unsigned int sp);
extern int jump_to_resume0(void* pointer);
extern void standby_flush_tlb(void);
extern void standby_preload_tlb(void);
void disable_cache_invalidate(void);
void disable_mmu(void);
void set_ttbr0(void);

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#define SWITCH_STACK
//#define DIRECT_RETRUN
//#define DRAM_ENTER_SELFRESH
//#define INIT_DRAM
//#define MEM_POWER_OFF
#define WITH_MMU
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#define SWITCH_STACK
//#define DIRECT_RETRUN
#define DRAM_ENTER_SELFRESH
//#define INIT_DRAM
#define PRE_DISABLE_MMU 
 			/*it is not possible to disable mmu, because 
                                 u need to keep va == pa, before disable mmu
                                 so, the va must be in 0x0000 region.so u need to 
			 creat this mapping before jump to suspend.
                                 so u need ttbr0 
                                 to keep this mapping.  */
#define SET_COPRO_DEFAULT 
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#define DISABLE_MMU
#define JUMP_WITH_NOMMU

#endif

#ifdef DIRECT_RETURN_FROM_SUSPEND
//#define SWITCH_STACK
#define DIRECT_RETRUN
#endif

#if defined(ENTER_SUPER_STANDBY) 
#define SWITCH_STACK
#undef PRE_DISABLE_MMU
//#define DIRECT_RETRUN
#define DRAM_ENTER_SELFRESH
//#define DISABLE_INVALIDATE_CACHE
//#define INIT_DRAM
//#define DISABLE_MMU
#define MEM_POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
//#define FLUSH_DCACHE  //can not flush data, if u do this, the data that u dont want save will be saved.
#define INVALIDATE_DCACHE
#endif

#if defined(ENTER_SUPER_STANDBY_WITH_NOMMU) //not support yet
#define SWITCH_STACK
#define PRE_DISABLE_MMU
#define DRAM_ENTER_SELFRESH
#define DISABLE_MMU
#define MEM_POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#define SET_COPRO_DEFAULT 
#endif

#ifdef WATCH_DOG_RESET
#define SWITCH_STACK
#define PRE_DISABLE_MMU
#define DRAM_ENTER_SELFRESH
#define DISABLE_MMU
#define START_WATCH_DOG
#define FLUSH_TLB
#define FLUSH_ICACHE
//#define FLUSH_DCACHE 
                                                        /*can not flush data, if u do this, the data that u dont want save willed. 
                                       * especillay, after u change the mapping table.
                                       */
#define INVALIDATE_DCACHE
#define SET_COPRO_DEFAULT 
#endif

/*
*********************************************************************************************************
*                                   SUPER STANDBY EXECUTE IN SRAM
*
* Description: super standby ,suspend to ram entry in sram.
*
* Arguments  : arg  pointer to the parameter that 
*
* Returns    : none
*
* Note       :
*********************************************************************************************************
*/

int main(void)
{
	char    *tmpPtr = (char *)&__bss_start;
	static __u32 sp_backup;
	__s32 dram_size;

	/* save stack pointer registger, switch stack to sram */
	//mark it, just for test 
#ifdef SWITCH_STACK
#ifdef PRE_DISABLE_MMU
	//busy_waiting();
	sp_backup = save_sp_nommu();
#else
	sp_backup = save_sp();
#endif
#endif	
	save_mem_status(SUSPEND_START);

#if 0
#ifdef FLUSH_DCACHE
	//clean & invalidate
	flush_dcache();
#endif	

#ifdef FLUSH_ICACHE
	//invalidate i cache
	flush_icache();
#endif
#endif


	/* flush data and instruction tlb, there is 32 items of data tlb and 32 items of instruction tlb,
	The TLB is normally allocated on a rotating basis. The oldest entry is always the next allocated */
#ifdef FLUSH_TLB
	standby_flush_tlb();
	save_mem_status(SUSPEND_START |0x01);	
#ifdef PRE_DISABLE_MMU
	/* preload tlb for standby */
	//busy_waiting();
	//standby_preload_tlb_nommu(); //0x0000 mapping is not large enough for preload nommu tlb
						//eg: 0x01c2.... is not in the 0x0000,0000 range.
	standby_preload_tlb();
	save_mem_status(SUSPEND_START |0x02);	
#else
	/* preload tlb for standby */
	standby_preload_tlb();
	save_mem_status(SUSPEND_START |0x03);	
#endif

#endif	
	/* clear bss segment */
	do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);
	


	save_mem_status(SUSPEND_START |0x04);	

	//busy_waiting();
	/* copy standby parameter from dram */
	//standby_memcpy(&pm_info, arg, sizeof(pm_info));
	
	/* initialise standby modules */
	standby_clk_init();
	save_mem_status(SUSPEND_START |0x05);
	standby_int_init();
	save_mem_status(SUSPEND_START |0x06);
	standby_tmr_init();
	save_mem_status(SUSPEND_START |0x07);
	standby_twi_init(AXP_IICBUS);
	save_mem_status(SUSPEND_START |0x08);
	mem_power_init();
    	save_mem_status(SUSPEND_START |0x09);
	
	//backup resume flag
	//busy_waiting();
	//save_mem_flag();
	save_mem_status(SUSPEND_START |0x0a);

	//just for test
	/*restore pmu config*/
	//busy_waiting();
#ifdef DIRECT_RETRUN
	mem_power_exit();
	save_mem_status(RESUME1_START |0x0b);
	
	return 0;
#endif

	/* dram enter self-refresh */
	//busy_waiting();
#ifdef DRAM_ENTER_SELFRESH
	dram_power_save_process();
	save_mem_status(SUSPEND_START |0x0c);
	
	/* gating off dram clock */
	//busy_waiting();
    	standby_clk_dramgating(0);
	save_mem_status(SUSPEND_START |0x0d);
#endif

#ifdef INIT_DRAM
	//busy_waiting();
	
	/*init dram*/
	dram_size = init_DRAM( );                                // ³õÊ¼»¯DRAM
	if(dram_size)
	{
		save_mem_status(SUSPEND_START |0x0e);
		//printk("dram size =%d\n", dram_size);
	}
	else
	{
		save_mem_status(SUSPEND_START |0x0f);
		//printk("initializing SDRAM Fail.\n");
	}

	save_mem_status(SUSPEND_START |0x10);
	standby_mdelay(100);

	//return 0;
#endif

#if 0
#ifdef FLUSH_TLB
	standby_flush_tlb();
#endif

#ifdef FLUSH_ICACHE
	//clean i/d cache
	flush_icache();
#endif

#ifdef INVALIDATE_DCACHE
	invalidate_dcache();
#endif
#endif

#ifdef DISABLE_INVALIDATE_CACHE
	disable_cache_invalidate();
	//busy_waiting();
#endif

#ifdef START_WATCH_DOG
	//busy_waiting();
	//start watch dog
	/* enable watch-dog to reset cpu */
    	standby_tmr_enable_watchdog();
	save_mem_status(SUSPEND_START |0x11);
	//while(1);
#endif

#ifdef DISABLE_MMU
	disable_mmu();
	standby_flush_tlb();
	save_mem_status_nommu(SUSPEND_START |0x12);
	//after disable mmu, it is time to preload nommu, need to access dram?
	standby_preload_tlb_nommu();
	//while(1);
#ifdef SET_COPRO_DEFAULT
		set_copro_default();
		save_mem_status_nommu(SUSPEND_START |0x13);
		//busy_waiting();
		//fake_busy_waiting();
#endif

#ifdef JUMP_WITH_NOMMU
		save_mem_status_nommu(SUSPEND_START |0x14);
		//before jump, disable mmu
		//busy_waiting();
		//jump_to_resume0_nommu(0x40100000);
		jump_to_resume0(0x40100000);
#endif 

#ifdef MEM_POWER_OFF
	    	/*power off*/
		//busy_waiting();
		/*NOTICE: not support to power off yet after disable mmu.
		  * because twi use virtual addr. 
		  */
	    	mem_power_off_nommu();
	    	save_mem_status_nommu(SUSPEND_START |0x15);
#endif

#else

#ifdef SET_COPRO_DEFAULT
	set_copro_default();
	save_mem_status(SUSPEND_START |0x13);
	//busy_waiting();
	//fake_busy_waiting();
#endif


#ifdef MEM_POWER_OFF
    	/*power off*/
	//busy_waiting();
    	mem_power_off();
    	save_mem_status(SUSPEND_START |0x0f);
#endif

#ifdef WITH_MMU
	//busy_waiting();
	save_mem_status(SUSPEND_START |0x0f);
	//busy_waiting();
	jump_to_resume0(0xc0100000);
#endif

#endif	
	//notice: never get here, so need watchdog, not busy_waiting.
#ifndef DIRECT_RETRUN
{
#if 0
	#define CPU_CONFIG_REG (0X01C20D3C)
	__u32 val = *(volatile __u32 *)(CPU_CONFIG_REG);
	*(volatile __u32 *)(PERMANENT_REG_PA  + 0x04) = val;
#endif
	busy_waiting();
}		
#endif
    
}
