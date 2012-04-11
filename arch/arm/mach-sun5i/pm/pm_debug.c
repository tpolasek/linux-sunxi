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

#ifdef GET_CYCLE_CNT
__u32 get_cyclecount (void)
{
  __u32 value;
  // Read CCNT Register
  asm volatile ("MRC p15, 0, %0, c9, c13, 0\t\n": "=r"(value));  
  return value;
}
#endif

