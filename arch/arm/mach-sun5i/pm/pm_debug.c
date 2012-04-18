#include "pm_types.h"
#include "pm.h"

void busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 1;
	while(1 == loop_flag);
	
#endif
	return;
}

void fake_busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 2;
	while(1 == loop_flag);
	
#endif
	return;
}

/*
 * notice: when resume, boot0 need to clear the flag, 
 * in case the data in dram be destoryed result in the system is re-resume in cycle.
*/
void save_mem_flag(void)
{
	__u32 saved_flag = *(volatile __u32 *)(PERMANENT_REG);
	saved_flag &= 0xfffffffe;
	saved_flag |= 0x00000001;
	*(volatile __u32 *)(PERMANENT_REG) = saved_flag;
	return;
}

void save_mem_status(volatile __u32 val)
{
	//*(volatile __u32 *)(STATUS_REG  + 0x04) = val;
	return;
}

__u32 get_mem_status(void)
{
	return *(volatile __u32 *)(STATUS_REG  + 0x04);
}

void save_mem_status_nommu(volatile __u32 val)
{
	//*(volatile __u32 *)(STATUS_REG_PA  + 0x04) = val;
	return;
}

__u32 save_sun5i_mem_status(volatile __u32 val)
{
	__u32 tmp;
	tmp = *(volatile __u32 *)(SUN5I_STATUS_REG);
	*(volatile __u32 *)(SUN5I_STATUS_REG) = val;
	return tmp;
}

__u32 save_sun5i_mem_status_nommu(volatile __u32 val)
{
	__u32 tmp;
	tmp = *(volatile __u32 *)(SUN5I_STATUS_REG_PA);
	*(volatile __u32 *)(SUN5I_STATUS_REG_PA) = val;
	return tmp;
}


#ifdef GET_CYCLE_CNT
__u32 get_cyclecount (void)
{
  __u32 value;
  // Read CCNT Register
  asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));  
  return value;
}

void init_perfcounters (__u32 do_reset, __u32 enable_divider)
{
	// in general enable all counters (including cycle counter)
	__u32 value = 1;

	// peform reset:
	if (do_reset)
	{
		value |= 2;     // reset all counters to zero.
		value |= 4;     // reset cycle counter to zero.
	}

	if (enable_divider)
		value |= 8;     // enable "by 64" divider for CCNT.

	value |= 16;

	// program the performance-counter control-register:
	asm volatile ("mcr p15, 0, %0, c9, c12, 0" : : "r"(value));

	// enable all counters:
	value = 0x8000000f;
	asm volatile ("mcr p15, 0, %0, c9, c12, 1" : : "r"(value));

	// clear overflows:
	asm volatile ("MCR p15, 0, %0, c9, c12, 3" : : "r"(value));

	return;
}

#endif

