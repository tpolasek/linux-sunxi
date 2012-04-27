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
static char    *tmpPtr = (char *)&__bss_start;
static __u32 status = 0; 

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

static void restore_ccmu(void);

int main(void)
{
	/* clear bss segment */
	do{*tmpPtr ++ = 0;}while(tmpPtr <= (char *)&__bss_end);
	
#ifdef MMU_OPENED
	save_mem_status(RESUME1_START);
#else
	//busy_waiting();
	//save_mem_status_nommu(RESUME1_START);
	status = save_sun5i_mem_status_nommu(RESUME1_START);
#endif

	
#ifdef MMU_OPENED
	save_mem_status(RESUME1_START |0x02);
	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	standby_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1), sizeof(mem_para_info));
#else
	standby_preload_tlb_nommu();
	/*switch stack*/
	//save_mem_status_nommu(RESUME1_START |0x02);
	save_sun5i_mem_status_nommu(RESUME1_START |0x02);
	//move other storage to sram: saved_resume_pointer(virtual addr), saved_mmu_state
	standby_memcpy((void *)&mem_para_info, (void *)(DRAM_BACKUP_BASE_ADDR1_PA), sizeof(mem_para_info));
	/*restore mmu configuration*/
	//save_mem_status_nommu(RESUME1_START |0x03);
	save_sun5i_mem_status_nommu(RESUME1_START |0x03);
	
	//busy_waiting();
	restore_mmu_state(&(mem_para_info.saved_mmu_state));
	disable_dcache();
	save_sun5i_mem_status(RESUME1_START |0x13);

#endif

//after open mmu mapping
#ifdef FLUSH_TLB
	//busy_waiting();
	standby_flush_tlb();
	standby_preload_tlb();
#endif
	save_sun5i_mem_status(RESUME1_START |0x04);
#ifdef FLUSH_ICACHE
	//clean i cache
	flush_icache();
#endif
	
	//save_mem_status(RESUME1_START |0x05);
	save_sun5i_mem_status(RESUME1_START |0x05);
	standby_twi_init(AXP_IICBUS);
	//save_mem_status(RESUME1_START |0x06);
	save_sun5i_mem_status(RESUME1_START |0x06);
	//twi freq?
	setup_twi_env();
	//save_mem_status(RESUME1_START |0x07);
	save_sun5i_mem_status(RESUME1_START |0x07);

#ifdef POWER_OFF
	//standby_clk_pllenable();
	//save_mem_status(RESUME1_START + 0x06);
	//standby_mdelay(10);
	//standby_delay(1);
	//restore ccmu
	//busy_waiting();
	restore_ccmu();
	//busy_waiting();
#endif

	/*restore pmu config*/
	//busy_waiting();
#ifdef POWER_OFF
	mem_power_exit();
	//save_mem_status(RESUME1_START |0x8);
	save_sun5i_mem_status(RESUME1_START |0x8);
	/* disable watch-dog: coresponding with boot0 */
	mem_tmr_init();
	mem_tmr_disable_watchdog();
#endif
	
	/*restore module status, why masked?*/
	//standby_twi_exit();

//before jump to late_resume	
#ifdef FLUSH_TLB
	//busy_waiting();
	//save_mem_status(RESUME1_START |0x9);
	save_sun5i_mem_status(RESUME1_START |0x9);
	standby_flush_tlb();
#endif

#ifdef FLUSH_ICACHE
	//clean i cache
	//save_mem_status(RESUME1_START |0xa);
	save_sun5i_mem_status(RESUME1_START |0xa);
	flush_icache();
#endif
	//save_mem_status(RESUME1_START |0xb);
	
	save_sun5i_mem_status(RESUME1_START |0xb);
	//save_sun5i_mem_status(status);
	//busy_waiting();
#ifdef GET_CYCLE_CNT
#ifdef CONFIG_ARCH_SUN4I
	//record resume1 time period in 0c reg 
	start = *(volatile __u32 *)(PERMANENT_REG  + 0x0c);
	end = get_cyclecount();
	*(volatile __u32 *)(PERMANENT_REG  + 0x0c) = end - start;
	//*(volatile __u32 *)(PERMANENT_REG  + 0x0c) = get_cyclecount();
	//record start
	*(volatile __u32 *)(PERMANENT_REG  + 0x00) = get_cyclecount();
#elif defined CONFIG_ARCH_SUN5I
	start = *(volatile __u32 *)(SUN5I_STATUS_REG - 0x0C);
	end = get_cyclecount();
	*(volatile __u32 *)(SUN5I_STATUS_REG - 0x0C) = end - start;
	//*(volatile __u32 *)(PERMANENT_REG  + 0x0c) = get_cyclecount();
	//record start
	*(volatile __u32 *)(SUN5I_STATUS_REG  - 0x10) = get_cyclecount();
#endif
#endif

	//before jump, invalidate data
	jump_to_resume((void *)mem_para_info.resume_pointer, mem_para_info.saved_runtime_context_svc);
	
	return;
}

void restore_ccmu(void)
{
	/* gating off dram clock */
	//standby_clk_dramgating(0);
	int i=0;
	
	for(i=0; i<6; i++){
		dram_hostport_on_off(i, 0);
	}
	
	for(i=16; i<31; i++){
		dram_hostport_on_off(i, 0);
	}
	
	/* switch cpu clock to HOSC, and disable pll */
	standby_clk_core2hosc();
	//standby_clk_plldisable();
	mem_clk_plldisable();
	
	/* backup voltages */
	//dcdc2 = 1400;
	dcdc2 = mem_para_info.suspend_vdd;
	dcdc3 = 1200;

	/* set clock division cpu:axi:ahb:apb = 2:2:2:1 */
#if 0
	standby_clk_getdiv(&clk_div);
	tmp_clk_div.axi_div = 0;
	tmp_clk_div.ahb_div = 0;
	tmp_clk_div.apb_div = 0;
	standby_clk_setdiv(&tmp_clk_div);
#endif
	/* swtich apb1 to losc */
	//standby_clk_apb2losc();
	
	/* switch cpu to 32k */
	//standby_clk_core2losc();
	/*delay period? 10/32k= 300us*/
	//standby_mdelay(10);

	//restore

	/* switch clock to hosc */
	//standby_clk_core2hosc();
	/* swtich apb1 to hosc */
	//standby_clk_apb2hosc();
	/* restore clock division */
	//standby_clk_setdiv(&clk_div);

	//standby_mdelay(10);

	//busy_waiting();
	/* restore voltage for exit standby */
	standby_set_voltage(POWER_VOL_DCDC2, dcdc2);
	standby_set_voltage(POWER_VOL_DCDC3, dcdc3);
	//25us * 40 * 10
	standby_mdelay(400);

	/*setting clock division ratio*/
	/* set clock division cpu:axi:ahb:apb =  1:1:2:2*/
	//standby_clk_getdiv(&clk_div);
	//tmp_clk_div.axi_div = 0;
	//tmp_clk_div.ahb_div = 1;
	//tmp_clk_div.apb_div = 1;
	//standby_clk_setdiv(&tmp_clk_div);
	
	//busy_waiting();
	/*setting pll factor: 384M hz*/
	standby_clk_set_pll_factor();
	
	/* enable pll */
	standby_clk_pllenable();
	//25us * 40 * 10
	standby_mdelay(400);
	
	/* switch cpu clock to core pll */
	standby_clk_core2pll();
	//33us* 30 * 10
	standby_mdelay(300);
	//busy_waiting();

	/* gating on dram clock */
	//standby_clk_dramgating(1);
	for(i=0; i<6; i++){
		dram_hostport_on_off(i, 1);
	}
	
	for(i=16; i<31; i++){
		dram_hostport_on_off(i, 1);
	}
	
	//standby_mdelay(1000);
	return;
}

