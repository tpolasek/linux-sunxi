/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : mem_power.c
* By      : gqyang.young
* Version : v1.0
* Date    : 2011-5-31 14:34
* Descript:
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/
#include "mem_i.h"
//==============================================================================
// POWER CHECK FOR SYSTEM MEM
//==============================================================================


/*
*********************************************************************************************************
*                           mem_power_init
*
* Description: init power for mem.
*
* Arguments  : none;
*
* Returns    : result;
*********************************************************************************************************
*/
void mem_power_init(void)
{
	__u8 val, mask, reg_val;
	__s32   i;
	
	save_mem_status(TWI_TRANSFER_STATUS );
	#if(AXP_WAKEUP & AXP_WAKEUP_KEY)
	/* enable power key long/short */
	if(twi_byte_rw(TWI_OP_RD, AXP_ADDR,AXP20_IRQEN3, &reg_val)){
		save_mem_status(TWI_TRANSFER_STATUS | 0x01);
		busy_waiting();
	}
	reg_val |= 0x03;
	
	if(twi_byte_rw(TWI_OP_WR, AXP_ADDR,AXP20_IRQEN3, &reg_val) ){
		save_mem_status(TWI_TRANSFER_STATUS | 0x02);
		busy_waiting();
	}
	#endif
	
	#if(AXP_WAKEUP & AXP_WAKEUP_LOWBATT)
	/* enable low voltage warning */
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,AXP20_IRQEN4, &reg_val);
	reg_val |= 0x03;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,AXP20_IRQEN4, &reg_val);
	/* clear pending */
	reg_val = 0x03;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,AXP20_IRQ4, &reg_val);
	#endif
	
	return;
}

/*
*********************************************************************************************************
*                           mem_power_exit
*
* Description: exit power for mem.
*
* Arguments  : none;
*
* Returns    : result;
*********************************************************************************************************
*/
void mem_power_exit(void)
{
	__u8    reg_val;

	//setup_env();

	/*把44H寄存器的bit5, bit6(按键上升沿触发、下降沿触发)置0*/
	if(twi_byte_rw(TWI_OP_RD, AXP_ADDR,0x44, &reg_val)){
		save_mem_status(TWI_TRANSFER_STATUS |0x04);
		busy_waiting();
	}
	reg_val &= ~0x60;
	if(twi_byte_rw(TWI_OP_WR, AXP_ADDR,0x44, &reg_val)){
		save_mem_status(TWI_TRANSFER_STATUS |0x05);
		busy_waiting();
	}
	
	#if(AXP_WAKEUP & AXP_WAKEUP_KEY)
	/* disable pek long/short */
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,AXP20_IRQEN3, &reg_val);
	reg_val &= ~0x03;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,AXP20_IRQEN3, &reg_val);
	#endif
	
	#if(AXP_WAKEUP & AXP_WAKEUP_LOWBATT)
	/* disable low voltage warning */
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,AXP20_IRQEN4, &reg_val);
	reg_val &= ~0x03;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,AXP20_IRQEN4, &reg_val);
	#endif

	return;
}

/*
 * mem_power_off
 *
 * Description:config wakeup signal. 
 *             turn off power for cpu-1v2, int-1v2, vcc-3v3 , but avcc-3v , dram-1v5.
 *             turn off 2*[csi ldo(low dropout regulator)] 
 *
 * Arguments  : none;
 *
 * Returns    : result;
 */
void mem_power_off(void)
{
	__u8 reg_val = 0;
	/*config wakeup signal*/
	/*把31H寄存器的bit3(按键、gpio唤醒位)置1*/
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,0x31, &reg_val);
	reg_val |= 0x08;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,0x31, &reg_val);

	/*把44H寄存器的bit5, bit6(按键上升沿触发、下降沿触发)置1*/
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,0x44, &reg_val);
	reg_val |= 0x60;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,0x44, &reg_val);
#if 1
	/*power off*/
	/*把12H寄存器的bit0、1、3、4、6置0*/
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,0x12, &reg_val);
	reg_val &= ~0x5b;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,0x12, &reg_val);
#endif

#if 0
	/*power off*/
	/*把12H寄存器的bit1、3、4、6置0*/
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,0x12, &reg_val);
	reg_val &= ~0x5a;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,0x12, &reg_val);
#endif

	/*never get here.
	 *when reach here, mean twi transfer err, 
	 *and cpu are not shut down.
	 */
	while(1);
	
	return;
}

/*
 * mem_power_off_nommu
 *
 * Description:config wakeup signal. 
 *             turn off power for cpu-1v2, int-1v2, vcc-3v3 , but avcc-3v , dram-1v5.
 *             turn off 2*[csi ldo(low dropout regulator)] 
 *
 * Arguments  : none;
 *
 * Returns    : result;
 */
void mem_power_off_nommu(void)
{
	__u8 reg_val = 0;
	/*config wakeup signal*/
	/*把31H寄存器的bit3(按键、gpio唤醒位)置1*/
	twi_byte_rw_nommu(TWI_OP_RD, AXP_ADDR,0x31, &reg_val);
	reg_val |= 0x08;
	twi_byte_rw_nommu(TWI_OP_WR, AXP_ADDR,0x31, &reg_val);

	/*把44H寄存器的bit5, bit6(按键上升沿触发、下降沿触发)置1*/
	twi_byte_rw_nommu(TWI_OP_RD, AXP_ADDR,0x44, &reg_val);
	reg_val |= 0x60;
	twi_byte_rw_nommu(TWI_OP_WR, AXP_ADDR,0x44, &reg_val);
#if 1
	/*power off*/
	/*把12H寄存器的bit0、1、3、4、6置0*/
	twi_byte_rw_nommu(TWI_OP_RD, AXP_ADDR,0x12, &reg_val);
	reg_val &= ~0x5b;
	twi_byte_rw_nommu(TWI_OP_WR, AXP_ADDR,0x12, &reg_val);
#endif

#if 0
	/*power off*/
	/*把12H寄存器的bit1、3、4、6置0*/
	twi_byte_rw(TWI_OP_RD, AXP_ADDR,0x12, &reg_val);
	reg_val &= ~0x5a;
	twi_byte_rw(TWI_OP_WR, AXP_ADDR,0x12, &reg_val);
#endif

	/*never get here.
	 *when reach here, mean twi transfer err, 
	 *and cpu are not shut down.
	 */
	while(1);
	
	return;
}


