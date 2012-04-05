/* these code will be removed to sram.
 * function: open the mmu, and jump to dram, for continuing resume*/
#include "./../mem_i.h"

extern unsigned int save_sp(void);
extern int jump_to_resume(void* pointer, __u32 *addr);
extern void restore_mmu_state(struct mmu_state *saved_mmu_state);
extern void standby_flush_tlb(void);
extern void flush_icache(void);
extern void flush_dcache(void);
extern void invalidate_dcache(void);
extern void standby_preload_tlb_nommu(void);
extern void clear_reg_context(void);

extern void disable_cache(void);
extern void disable_program_flow_prediction(void);
extern void invalidate_branch_predictor(void);
extern void enable_cache(void);
extern void enable_program_flow_prediction(void);
extern void disable_dcache(void);
extern void disable_l2cache(void);

static struct aw_mem_para mem_para_info;

extern char *__bss_start;
extern char *__bss_end;
static __u32 dcdc2, dcdc3;
static struct sun4i_clk_div_t  clk_div;
static struct sun4i_clk_div_t  tmp_clk_div;
static __u32 sp_backup;
char    *tmpPtr = (char *)&__bss_start;

#ifdef GET_CYCLE_CNT
static int start = 0;
static int end = 0;
#endif


#ifdef RETURN_FROM_RESUME0_WITH_MMU
#define MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#undef MMU_OPENED
#undef POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

#if defined(ENTER_SUPER_STANDBY) || defined(ENTER_SUPER_STANDBY_WITH_NOMMU) || defined(WATCH_DOG_RESET)
#undef MMU_OPENED
#define POWER_OFF
#define FLUSH_TLB
#define FLUSH_ICACHE
#define INVALIDATE_DCACHE
#endif

int main(void)
{
#ifdef MMU_OPENED
	save_mem_status(RESUME1_START);
#else
	save_mem_status_nommu(RESUME1_START);
#endif

	/* clear bss segment */
	do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);
	
#ifdef MMU_OPENED
	save_mem_status(RESUME1_START |0x02);
	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	standby_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1), sizeof(mem_para_info));
#else
	standby_preload_tlb_nommu();
	/*switch stack*/
	save_mem_status_nommu(RESUME1_START |0x02);
	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	standby_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1_PA), sizeof(mem_para_info));
	/*restore mmu configuration*/
	save_mem_status_nommu(RESUME1_START |0x03);
	
	restore_mmu_state(&(mem_para_info.saved_mmu_state));

#endif

//after open mmu mapping
#ifdef FLUSH_TLB
	//busy_waiting();
	standby_flush_tlb();
	standby_preload_tlb();
#endif

#ifdef FLUSH_ICACHE
	//clean i cache
	flush_icache();
#endif
	
	save_mem_status(RESUME1_START |0x05);
	standby_twi_init(AXP_IICBUS);
	save_mem_status(RESUME1_START |0x06);
	//twi freq?
	setup_twi_env();
	save_mem_status(RESUME1_START |0x07);

#ifdef POWER_OFF
	dcdc2 = mem_para_info.suspend_vdd;
	dcdc3 = 1250;
	/* restore voltage for exit standby */
	standby_set_voltage(POWER_VOL_DCDC2, dcdc2);
	standby_set_voltage(POWER_VOL_DCDC3, dcdc3);
	//busy_waiting();
#endif

	/*restore pmu config*/
	//busy_waiting();
#ifdef POWER_OFF
	mem_power_exit();
	save_mem_status(RESUME1_START |0x8);
	/* disable watch-dog: coresponding with boot0 */
	mem_tmr_init();
	mem_tmr_disable_watchdog();
#endif
	
	/*restore module status, why masked?*/
	//standby_twi_exit();

//before jump to late_resume	
#ifdef FLUSH_TLB
	//busy_waiting();
	save_mem_status(RESUME1_START |0x9);
	standby_flush_tlb();
#endif

#ifdef FLUSH_ICACHE
	//clean i cache
	save_mem_status(RESUME1_START |0xa);
	flush_icache();
#endif
	save_mem_status(RESUME1_START |0xb);
	
	//busy_waiting();
#ifdef GET_CYCLE_CNT
	//record resume1 time period in 0c reg 
	start = *(volatile __u32 *)(PERMANENT_REG  + 0x0c);
	end = get_cyclecount();
	*(volatile __u32 *)(PERMANENT_REG  + 0x0c) = end - start;
	//*(volatile __u32 *)(PERMANENT_REG  + 0x0c) = get_cyclecount();
	//record start
	*(volatile __u32 *)(PERMANENT_REG  + 0x00) = get_cyclecount();
#endif

	//before jump, invalidate data
	jump_to_resume((void *)mem_para_info.resume_pointer, mem_para_info.saved_runtime_context_svc);
	
	return;
}

