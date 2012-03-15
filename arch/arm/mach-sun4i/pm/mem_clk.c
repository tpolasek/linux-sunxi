#include "pm_types.h"
#include "pm_i.h"

/*
*********************************************************************************************************
*                           mem_clk_init
*
*Description: ccu init for platform mem
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 mem_clk_save(struct clk_state *pclk_state)
{
	__ccmu_reg_list_t   *CmuReg;
	pclk_state->CmuReg = CmuReg = (__ccmu_reg_list_t *)SW_VA_CCM_IO_BASE;

	/*backup clk src and ldo*/
	pclk_state->ccu_reg_back[0] = *(volatile __u32 *)&CmuReg->HoscCtl;
	pclk_state->ccu_reg_back[1] = *(volatile __u32 *)&CmuReg->SysClkDiv;
	pclk_state->ccu_reg_back[2] = *(volatile __u32 *)&CmuReg->Apb1ClkDiv;
	
	/* backup pll registers and tuning?*/
	pclk_state->ccu_reg_back[3] = *(volatile __u32 *)&CmuReg->Pll1Ctl;
	pclk_state->ccu_reg_back[4] = *(volatile __u32 *)&CmuReg->Pll2Ctl;
	pclk_state->ccu_reg_back[5] = *(volatile __u32 *)&CmuReg->Pll3Ctl;
	pclk_state->ccu_reg_back[6] = *(volatile __u32 *)&CmuReg->Pll4Ctl;
	pclk_state->ccu_reg_back[7] = *(volatile __u32 *)&CmuReg->Pll5Ctl;
	pclk_state->ccu_reg_back[8] = *(volatile __u32 *)&CmuReg->Pll6Ctl;
	pclk_state->ccu_reg_back[9] = *(volatile __u32 *)&CmuReg->Pll7Ctl;

	/*backup axi, ahb, apb gating*/
	pclk_state->ccu_reg_back[10] = *(volatile __u32 *)&CmuReg->AxiGate;
	pclk_state->ccu_reg_back[11] = *(volatile __u32 *)&CmuReg->AhbGate0;
	pclk_state->ccu_reg_back[12] = *(volatile __u32 *)&CmuReg->AhbGate1;
	pclk_state->ccu_reg_back[13] = *(volatile __u32 *)&CmuReg->Apb0Gate;
	pclk_state->ccu_reg_back[14] = *(volatile __u32 *)&CmuReg->Apb1Gate;
	
	return 0;
}


/*
*********************************************************************************************************
*                           mem_clk_exit
*
*Description: ccu exit for platform mem
*
*Arguments  : none
*
*Return     : result,
*
*Notes      :
*
*********************************************************************************************************
*/
__s32 mem_clk_restore(struct clk_state *pclk_state)
{
	/* initialise the CCU io base */
	__ccmu_reg_list_t   *CmuReg = pclk_state->CmuReg;
	
	/*restore clk src and ldo*/
	*(volatile __u32 *)&CmuReg->HoscCtl    = pclk_state->ccu_reg_back[0];    
	*(volatile __u32 *)&CmuReg->SysClkDiv  = pclk_state->ccu_reg_back[1];  
	*(volatile __u32 *)&CmuReg->Apb1ClkDiv = pclk_state->ccu_reg_back[2];

	/*restore axi, ahb, apb gating*/	
	*(volatile __u32 *)&CmuReg->AxiGate    = pclk_state->ccu_reg_back[10];  
	*(volatile __u32 *)&CmuReg->AhbGate0   = pclk_state->ccu_reg_back[11]; 
	*(volatile __u32 *)&CmuReg->AhbGate1   = pclk_state->ccu_reg_back[12]; 
	*(volatile __u32 *)&CmuReg->Apb0Gate   = pclk_state->ccu_reg_back[13]; 
	*(volatile __u32 *)&CmuReg->Apb1Gate   = pclk_state->ccu_reg_back[14]; 
	
	/* restore pll registers and tuning? latency?*/                                
	*(volatile __u32 *)&CmuReg->Pll1Ctl    = pclk_state->ccu_reg_back[3]; 
	*(volatile __u32 *)&CmuReg->Pll2Ctl    = pclk_state->ccu_reg_back[4]; 
	*(volatile __u32 *)&CmuReg->Pll3Ctl    = pclk_state->ccu_reg_back[5]; 
	*(volatile __u32 *)&CmuReg->Pll4Ctl    = pclk_state->ccu_reg_back[6]; 
	*(volatile __u32 *)&CmuReg->Pll5Ctl    = pclk_state->ccu_reg_back[7]; 
	*(volatile __u32 *)&CmuReg->Pll6Ctl    = pclk_state->ccu_reg_back[8]; 
	*(volatile __u32 *)&CmuReg->Pll7Ctl    = pclk_state->ccu_reg_back[9]; 
	                                         
	/* config the CCU to default status */
	//needed?
#if 0
	if(MAGIC_VER_C == sw_get_ic_ver()) {
		/* switch PLL4 to PLL6 */
		#if(USE_PLL6M_REPLACE_PLL4)
		CmuReg->VeClk.PllSwitch = 1;
		#else
		CmuReg->VeClk.PllSwitch = 0;
		#endif
	}
#endif
	
	return 0;
}
