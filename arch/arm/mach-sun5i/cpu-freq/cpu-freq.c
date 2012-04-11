/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : cpu-freq.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-6-18 18:13
* Descript: cpufreq driver on allwinner chips;
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include "cpu-freq.h"
#include <linux/pm.h>
#include "../pm/pm_i.h"
#include <mach/ccmu_regs.h>

static __ccmu_reg_list_t   CmuReg;

extern struct aw_mem_para mem_para_info;
static struct sun4i_cpu_freq_t  cpu_cur;    /* current cpu frequency configuration  */
static unsigned int last_target = ~0;       /* backup last target frequency         */

static struct clk *clk_pll; /* pll clock handler */
static struct clk *clk_cpu; /* cpu clock handler */
static struct clk *clk_axi; /* axi clock handler */
static struct clk *clk_ahb; /* ahb clock handler */
static struct clk *clk_apb; /* apb clock handler */


#ifdef CONFIG_CPU_FREQ_DVFS
struct cpufreq_dvfs {
    unsigned int    freq;   /* cpu frequency    */
    unsigned int    volt;   /* voltage for the frequency    */
};
static struct cpufreq_dvfs dvfs_table[] = {
    {.freq = 1104000000, .volt = 1500}, /* core vdd is 1.40v if cpu frequency is (1008Mhz, 1104Mhz] */
    {.freq = 1008000000, .volt = 1400}, /* core vdd is 1.35v if cpu frequency is (912Mhz, 1008Mhz]  */
    {.freq = 912000000,  .volt = 1350}, /* core vdd is 1.30v if cpu frequency is (864Mhz, 912Mhz]   */
    {.freq = 864000000,  .volt = 1300}, /* core vdd is 1.20v if cpu frequency is (624Mhz, 864Mhz]   */
    {.freq = 624000000,  .volt = 1200}, /* core vdd is 1.10v if cpu frequency is (576Mhz, 624Mhz]   */
    {.freq = 576000000,  .volt = 1100}, /* core vdd is 1.10v if cpu frequency is (432Mhz, 576Mhz]   */
    {.freq = 432000000,  .volt = 1000}, /* core vdd is 1.00v if cpu frequency is (240Mhz, 432Mhz]   */
//    {.freq = 240000000,  .volt = 900 }, /* core vdd is 0.90v if cpu frequency is (150Mhz, 240Mhz    */
//    {.freq = 150000000,  .volt = 800 }, /* core vdd is 0.80v if cpu frequency is (0Mhz, 150Mhz]     */
    {.freq = 0,          .volt = 700 }, /* end of cpu dvfs table                                    */
};
static struct regulator *corevdd;
static unsigned int last_vdd    = 1400;     /* backup last target voltage, default is 1.4v  */
#endif
void ccmu_reg_backup(__ccmu_reg_list_t *pReg)
{
    // CmuReg.Pll1Ctl      = pReg->Pll1Ctl;
    // CmuReg.Pll1Tune     = pReg->Pll1Tune;
    CmuReg.Pll2Ctl      = pReg->Pll2Ctl;
    CmuReg.Pll2Tune     = pReg->Pll2Tune;
    CmuReg.Pll3Ctl      = pReg->Pll3Ctl;
    CmuReg.Pll4Ctl      = pReg->Pll4Ctl;
     // CmuReg.Pll5Ctl      = pReg->Pll5Ctl;
    // CmuReg.Pll5Tune     = pReg->Pll5Tune;
    CmuReg.Pll6Ctl      = pReg->Pll6Ctl;
    //CmuReg.Pll6Tune     = pReg->Pll6Tune;
    CmuReg.Pll7Ctl      = pReg->Pll7Ctl;
    //CmuReg.Pll7Tune     = pReg->Pll7Tune;
    // CmuReg.Pll1Tune2    = pReg->Pll1Tune2;
    // CmuReg.Pll5Tune2    = pReg->Pll5Tune2;
    CmuReg.HoscCtl      = pReg->HoscCtl;
    CmuReg.SysClkDiv    = pReg->SysClkDiv;
    CmuReg.Apb1ClkDiv   = pReg->Apb1ClkDiv;
    CmuReg.AxiGate      = pReg->AxiGate;
    CmuReg.AhbGate0     = pReg->AhbGate0;
    CmuReg.AhbGate1     = pReg->AhbGate1;
    CmuReg.Apb0Gate     = pReg->Apb0Gate;
    CmuReg.Apb1Gate     = pReg->Apb1Gate;
    CmuReg.NandClk      = pReg->NandClk;
    CmuReg.MsClk        = pReg->MsClk;
    CmuReg.SdMmc0Clk    = pReg->SdMmc0Clk;
    CmuReg.SdMmc1Clk    = pReg->SdMmc1Clk;
    CmuReg.SdMmc2Clk    = pReg->SdMmc2Clk;
    //CmuReg.SdMmc3Clk    = pReg->SdMmc3Clk;
    CmuReg.TsClk        = pReg->TsClk;
    CmuReg.SsClk        = pReg->SsClk;
    CmuReg.Spi0Clk      = pReg->Spi0Clk;
    CmuReg.Spi1Clk      = pReg->Spi1Clk;
    CmuReg.Spi2Clk      = pReg->Spi2Clk;
    //CmuReg.PataClk      = pReg->PataClk;
    CmuReg.Ir0Clk       = pReg->Ir0Clk;
    //CmuReg.Ir1Clk       = pReg->Ir1Clk;
    CmuReg.I2sClk       = pReg->I2sClk;
    //CmuReg.Ac97Clk      = pReg->Ac97Clk;
    CmuReg.SpdifClk     = pReg->SpdifClk;
    CmuReg.KeyPadClk    = pReg->KeyPadClk;
    //CmuReg.SataClk      = pReg->SataClk;
    CmuReg.UsbClk       = pReg->UsbClk;
    CmuReg.GpsClk       = pReg->GpsClk;
    //CmuReg.Spi3Clk      = pReg->Spi3Clk;
    CmuReg.DramGate     = pReg->DramGate;
    CmuReg.DeBe0Clk     = pReg->DeBe0Clk;
    //CmuReg.DeBe1Clk     = pReg->DeBe1Clk;
    CmuReg.DeFe0Clk     = pReg->DeFe0Clk;
    //CmuReg.DeFe1Clk     = pReg->DeFe1Clk;
    //CmuReg.DeMpClk      = pReg->DeMpClk;
    CmuReg.Lcd0Ch0Clk   = pReg->Lcd0Ch0Clk;
    //CmuReg.Lcd1Ch0Clk   = pReg->Lcd1Ch0Clk;
    //CmuReg.CsiIspClk    = pReg->CsiIspClk;
    //CmuReg.TvdClk       = pReg->TvdClk;
    CmuReg.Lcd0Ch1Clk   = pReg->Lcd0Ch1Clk;
    //CmuReg.Lcd1Ch1Clk   = pReg->Lcd1Ch1Clk;
    CmuReg.Csi0Clk      = pReg->Csi0Clk;
    //CmuReg.Csi1Clk      = pReg->Csi1Clk;
    CmuReg.VeClk        = pReg->VeClk;
    CmuReg.AddaClk      = pReg->AddaClk;
    CmuReg.AvsClk       = pReg->AvsClk;
    //CmuReg.AceClk       = pReg->AceClk;
    CmuReg.LvdsClk      = pReg->LvdsClk;
    CmuReg.HdmiClk      = pReg->HdmiClk;
    CmuReg.MaliClk      = pReg->MaliClk;
}

void ccmu_reg_restore(__ccmu_reg_list_t *pReg)
{
    //1. pll(pll1/pll5)
    pReg->Pll2Ctl       = CmuReg.Pll2Ctl;
    pReg->Pll2Tune      = CmuReg.Pll2Tune;
    pReg->Pll3Ctl       = CmuReg.Pll3Ctl;
    pReg->Pll4Ctl       = CmuReg.Pll4Ctl;
    //pReg->Pll4Tune      = CmuReg.Pll4Tune;
    pReg->Pll6Ctl       = CmuReg.Pll6Ctl;
    //pReg->Pll6Tune      = CmuReg.Pll6Tune;
    pReg->Pll7Ctl       = CmuReg.Pll7Ctl;
    //pReg->Pll7Tune      = CmuReg.Pll7Tune;

    //2. mod clk-src , div;
    pReg->HoscCtl       = CmuReg.HoscCtl;
    pReg->SysClkDiv     = CmuReg.SysClkDiv;
    pReg->Apb1ClkDiv    = CmuReg.Apb1ClkDiv;

    //3. mod gating;
    pReg->AxiGate       = CmuReg.AxiGate;
    pReg->AhbGate0      = CmuReg.AhbGate0;
    pReg->AhbGate1      = CmuReg.AhbGate1;
    pReg->Apb0Gate      = CmuReg.Apb0Gate;
    pReg->Apb1Gate      = CmuReg.Apb1Gate;
    pReg->DramGate      = CmuReg.DramGate;

    //4. mod reset;
    pReg->NandClk       = CmuReg.NandClk;
    pReg->MsClk         = CmuReg.MsClk;
    pReg->SdMmc0Clk     = CmuReg.SdMmc0Clk;
    pReg->SdMmc1Clk     = CmuReg.SdMmc1Clk;
    pReg->SdMmc2Clk     = CmuReg.SdMmc2Clk;
    //pReg->SdMmc3Clk     = CmuReg.SdMmc3Clk;
    pReg->TsClk         = CmuReg.TsClk;
    pReg->SsClk         = CmuReg.SsClk;
    pReg->Spi0Clk       = CmuReg.Spi0Clk;
    pReg->Spi1Clk       = CmuReg.Spi1Clk;
    pReg->Spi2Clk       = CmuReg.Spi2Clk;
    //pReg->PataClk       = CmuReg.PataClk;
    pReg->Ir0Clk        = CmuReg.Ir0Clk;
    //pReg->Ir1Clk        = CmuReg.Ir1Clk;
    pReg->I2sClk        = CmuReg.I2sClk;
    //pReg->Ac97Clk       = CmuReg.Ac97Clk;
    pReg->SpdifClk      = CmuReg.SpdifClk;
    pReg->KeyPadClk     = CmuReg.KeyPadClk;
    //pReg->SataClk       = CmuReg.SataClk;
    pReg->UsbClk        = CmuReg.UsbClk;
    pReg->GpsClk        = CmuReg.GpsClk;
    //pReg->Spi3Clk       = CmuReg.Spi3Clk;
    pReg->DeBe0Clk      = CmuReg.DeBe0Clk;
    //pReg->DeBe1Clk      = CmuReg.DeBe1Clk;
    pReg->DeFe0Clk      = CmuReg.DeFe0Clk;
    //pReg->DeFe1Clk      = CmuReg.DeFe1Clk;
    //pReg->DeMpClk       = CmuReg.DeMpClk;
    pReg->Lcd0Ch0Clk    = CmuReg.Lcd0Ch0Clk;
    //pReg->Lcd1Ch0Clk    = CmuReg.Lcd1Ch0Clk;
    //pReg->CsiIspClk     = CmuReg.CsiIspClk;
    //pReg->TvdClk        = CmuReg.TvdClk;
    pReg->Lcd0Ch1Clk    = CmuReg.Lcd0Ch1Clk;
    //pReg->Lcd1Ch1Clk    = CmuReg.Lcd1Ch1Clk;
    pReg->Csi0Clk       = CmuReg.Csi0Clk;
    //pReg->Csi1Clk       = CmuReg.Csi1Clk;
    pReg->VeClk         = CmuReg.VeClk;
    pReg->AddaClk       = CmuReg.AddaClk;
    pReg->AvsClk        = CmuReg.AvsClk;
    //pReg->AceClk        = CmuReg.AceClk;
    pReg->LvdsClk       = CmuReg.LvdsClk;
    pReg->HdmiClk       = CmuReg.HdmiClk;
    pReg->MaliClk       = CmuReg.MaliClk;
}

/*
*********************************************************************************************************
*                           sun4i_cpufreq_verify
*
*Description: check if the cpu frequency policy is valid;
*
*Arguments  : policy    cpu frequency policy;
*
*Return     : result, return if verify ok, else return -EINVAL;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_verify(struct cpufreq_policy *policy)
{
	if (policy->cpu != 0)
		return -EINVAL;

	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_show
*
*Description: show cpu frequency information;
*
*Arguments  : pfx   name;
*
*
*Return     :
*
*Notes      :
*
*********************************************************************************************************
*/
static void sun4i_cpufreq_show(const char *pfx, struct sun4i_cpu_freq_t *cfg)
{
	CPUFREQ_DBG("%s: pll=%u, cpudiv=%u, axidiv=%u, ahbdiv=%u, apb=%u\n",
        pfx, cfg->pll, cfg->div.cpu_div, cfg->div.axi_div, cfg->div.ahb_div, cfg->div.apb_div);
}


#ifdef CONFIG_CPU_FREQ_DVFS
/*
*********************************************************************************************************
*                           __get_vdd_value
*
*Description: get vdd with cpu frequency.
*
*Arguments  : freq  cpu frequency;
*
*Return     : vdd value;
*
*Notes      :
*
*********************************************************************************************************
*/
static inline unsigned int __get_vdd_value(unsigned int freq)
{
    struct cpufreq_dvfs *dvfs_inf = &dvfs_table[0];
    while((dvfs_inf+1)->freq >= freq) dvfs_inf++;

    return dvfs_inf->volt;
}
#endif


/*
*********************************************************************************************************
*                           __set_cpufreq_hw
*
*Description: set cpu frequency configuration to hardware.
*
*Arguments  : freq  frequency configuration;
*
*Return     : result
*
*Notes      :
*
*********************************************************************************************************
*/
static inline int __set_cpufreq_hw(struct sun4i_cpu_freq_t *freq)
{
    int             ret;
    unsigned int    frequency;

    /* try to adjust pll frequency */
    ret = clk_set_rate(clk_pll, freq->pll);
    /* try to adjust cpu frequency */
    frequency = freq->pll / freq->div.cpu_div;
    ret |= clk_set_rate(clk_cpu, frequency);
    /* try to adjuxt axi frequency */
    frequency /= freq->div.axi_div;
    ret |= clk_set_rate(clk_axi, frequency);
    /* try to adjust ahb frequency */
    frequency /= freq->div.ahb_div;
    ret |= clk_set_rate(clk_ahb, frequency);
    /* try to adjust apb frequency */
    frequency /= freq->div.apb_div;
    ret |= clk_set_rate(clk_apb, frequency);

    return ret;
}


/*
*********************************************************************************************************
*                           __set_cpufreq_target
*
*Description: set target frequency, the frequency limitation of axi is 450Mhz, the frequency
*             limitation of ahb is 250Mhz, and the limitation of apb is 150Mhz. for usb connecting,
*             the frequency of ahb must not lower than 60Mhz.
*
*Arguments  : old   cpu/axi/ahb/apb frequency old configuration.
*             new   cpu/axi/ahb/apb frequency new configuration.
*
*Return     : result, 0 - set frequency successed, !0 - set frequency failed;
*
*Notes      : we check two frequency point: 204Mhz, 408Mhz, 816Mhz and 1200Mhz.
*             if increase cpu frequency, the flow should be:
*               low(1:1:1:2) -> 204Mhz(1:1:1:2) -> 204Mhz(1:1:2:2) -> 408Mhz(1:1:2:2)
*               -> 408Mhz(1:2:2:2) -> 816Mhz(1:2:2:2) -> 816Mhz(1:3:2:2) -> 1200Mhz(1:3:2:2)
*               -> 1200Mhz(1:4:2:2) -> target(1:4:2:2) -> target(x:x:x:x)
*             if decrease cpu frequency, the flow should be:
*               high(x:x:x:x) -> target(1:4:2:2) -> 1200Mhz(1:4:2:2) -> 1200Mhz(1:3:2:2)
*               -> 816Mhz(1:3:2:2) -> 816Mhz(1:2:2:2) -> 408Mhz(1:2:2:2) -> 408Mhz(1:1:2:2)
*               -> 204Mhz(1:1:2:2) -> 204Mhz(1:1:1:2) -> target(1:1:1:2)
*********************************************************************************************************
*/
static int __set_cpufreq_target(struct sun4i_cpu_freq_t *old, struct sun4i_cpu_freq_t *new)
{
    int     ret = 0;
    struct sun4i_cpu_freq_t old_freq, new_freq;

    if(!old || !new) {
        return -EINVAL;
    }

    old_freq = *old;
    new_freq = *new;

    CPUFREQ_INF("cpu: %dMhz->%dMhz\n", old_freq.pll/1000000, new_freq.pll/1000000);

    if(new_freq.pll > old_freq.pll) {
        if((old_freq.pll <= 204000000) && (new_freq.pll >= 204000000)) {
            /* set to 204Mhz (1:1:1:2) */
            old_freq.pll = 204000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 1;
            old_freq.div.ahb_div = 1;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 204Mhz (1:1:2:2) */
            old_freq.div.ahb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
        }
        if((old_freq.pll <= 408000000) && (new_freq.pll >= 408000000)) {
            /* set to 408Mhz (1:1:2:2) */
            old_freq.pll = 408000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 1;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 408Mhz (1:2:2:2) */
            old_freq.div.axi_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
        }
        if((old_freq.pll <= 816000000) && (new_freq.pll >= 816000000)) {
            /* set to 816Mhz (1:2:2:2) */
            old_freq.pll = 816000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 2;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 816Mhz (1:3:2:2) */
            old_freq.div.axi_div = 3;
            ret |= __set_cpufreq_hw(&old_freq);
        }
        if((old_freq.pll <= 1200000000) && (new_freq.pll >= 1200000000)) {
            /* set to 1200Mhz (1:3:2:2) */
            old_freq.pll = 1200000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 3;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 1200Mhz (1:4:2:2) */
            old_freq.div.axi_div = 4;
            ret |= __set_cpufreq_hw(&old_freq);
        }

        /* adjust to target frequency */
        ret |= __set_cpufreq_hw(&new_freq);
    }
    else if(new_freq.pll < old_freq.pll) {
        if((old_freq.pll > 1200000000) && (new_freq.pll <= 1200000000)) {
            /* set to 1200Mhz (1:3:2:2) */
            old_freq.pll = 1200000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 3;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
        }
        if((old_freq.pll > 816000000) && (new_freq.pll <= 816000000)) {
            /* set to 816Mhz (1:3:2:2) */
            old_freq.pll = 816000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 3;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 816Mhz (1:2:2:2) */
            old_freq.div.axi_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
        }
        if((old_freq.pll > 408000000) && (new_freq.pll <= 408000000)) {
            /* set to 408Mhz (1:2:2:2) */
            old_freq.pll = 408000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 2;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 816Mhz (1:1:2:2) */
            old_freq.div.axi_div = 1;
            ret |= __set_cpufreq_hw(&old_freq);
        }
        if((old_freq.pll > 204000000) && (new_freq.pll <= 204000000)) {
            /* set to 204Mhz (1:1:2:2) */
            old_freq.pll = 204000000;
            old_freq.div.cpu_div = 1;
            old_freq.div.axi_div = 1;
            old_freq.div.ahb_div = 2;
            old_freq.div.apb_div = 2;
            ret |= __set_cpufreq_hw(&old_freq);
            /* set to 204Mhz (1:1:1:2) */
            old_freq.div.ahb_div = 1;
            ret |= __set_cpufreq_hw(&old_freq);
        }

        /* adjust to target frequency */
        ret |= __set_cpufreq_hw(&new_freq);
    }

    if(ret) {
        unsigned int    frequency;

        CPUFREQ_ERR("try to set target frequency failed!\n");

        /* try to restore frequency configuration */
        frequency = clk_get_rate(clk_cpu);
        frequency /= 4;
        clk_set_rate(clk_axi, frequency);
        frequency /= 2;
        clk_set_rate(clk_ahb, frequency);
        frequency /= 2;
        clk_set_rate(clk_apb, frequency);

        clk_set_rate(clk_pll, old->pll);
        frequency = old->pll / old->div.cpu_div;
        clk_set_rate(clk_cpu, frequency);
        frequency /= old->div.axi_div;
        clk_set_rate(clk_axi, frequency);
        frequency /= old->div.ahb_div;
        clk_set_rate(clk_ahb, frequency);
        frequency /= old->div.apb_div;
        clk_set_rate(clk_apb, frequency);

        CPUFREQ_ERR(KERN_ERR "no compatible settings cpu freq for %d\n", new_freq.pll);
        return -1;
    }

    return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_settarget
*
*Description: adjust cpu frequency;
*
*Arguments  : policy    cpu frequency policy, to mark if need notify;
*             cpu_freq  new cpu frequency configuration;
*
*Return     : return 0 if set successed, otherwise, return -EINVAL
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_settarget(struct cpufreq_policy *policy, struct sun4i_cpu_freq_t *cpu_freq)
{
    struct cpufreq_freqs    freqs;
    struct sun4i_cpu_freq_t cpu_new;

    #ifdef CONFIG_CPU_FREQ_DVFS
    unsigned int    new_vdd;
    #endif

    /* show current cpu frequency configuration, just for debug */
	sun4i_cpufreq_show("cur", &cpu_cur);

    /* get new cpu frequency configuration */
	cpu_new = *cpu_freq;
	sun4i_cpufreq_show("new", &cpu_new);

    /* notify that cpu clock will be adjust if needed */
	if (policy) {
	    freqs.cpu = 0;
	    freqs.old = cpu_cur.pll / 1000;
	    freqs.new = cpu_new.pll / 1000;
		cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
	}

    #ifdef CONFIG_CPU_FREQ_DVFS
    /* get vdd value for new frequency */
    new_vdd = __get_vdd_value(cpu_new.pll);

    if(corevdd && (new_vdd > last_vdd)) {
        CPUFREQ_INF("set core vdd to %d\n", new_vdd);
        if(regulator_set_voltage(corevdd, new_vdd*1000, new_vdd*1000)) {
            CPUFREQ_INF("try to set voltage failed!\n");

            /* notify everyone that clock transition finish */
    	    if (policy) {
	            freqs.cpu = 0;
	            freqs.old = freqs.new;
	            freqs.new = cpu_cur.pll / 1000;
		        cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	        }
            return -EINVAL;
        }
    }
    #endif

    if(__set_cpufreq_target(&cpu_cur, &cpu_new)){

        /* try to set cpu frequency failed */

        #ifdef CONFIG_CPU_FREQ_DVFS
        if(corevdd && (new_vdd > last_vdd)) {
            CPUFREQ_INF("set core vdd to %d\n", last_vdd);
            if(regulator_set_voltage(corevdd, last_vdd*1000, last_vdd*1000)){
                CPUFREQ_INF("try to set voltage failed!\n");
                last_vdd = new_vdd;
            }
        }
        #endif

        /* notify everyone that clock transition finish */
    	if (policy) {
	        freqs.cpu = 0;
	        freqs.old = freqs.new;
	        freqs.new = cpu_cur.pll / 1000;
		    cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	    }

        return -EINVAL;
    }

    #ifdef CONFIG_CPU_FREQ_DVFS
    if(corevdd && (new_vdd < last_vdd)) {
        CPUFREQ_INF("set core vdd to %d\n", new_vdd);
        if(regulator_set_voltage(corevdd, new_vdd*1000, new_vdd*1000)) {
            CPUFREQ_INF("try to set voltage failed!\n");
            new_vdd = last_vdd;
        }
    }
    last_vdd = new_vdd;
    #endif

	/* update our current settings */
	cpu_cur = cpu_new;

	/* notify everyone we've done this */
	if (policy) {
		cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	}

	CPUFREQ_DBG("%s: finished\n", __func__);
	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_target
*
*Description: adjust the frequency that cpu is currently running;
*
*Arguments  : policy    cpu frequency policy;
*             freq      target frequency to be set, based on khz;
*             relation  method for selecting the target requency;
*
*Return     : result, return 0 if set target frequency successed,
*                     else, return -EINVAL;
*
*Notes      : this function is called by the cpufreq core;
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_target(struct cpufreq_policy *policy, __u32 freq, __u32 relation)
{
    int                     ret;
    unsigned int            index;
    struct sun4i_cpu_freq_t freq_cfg;

	/* avoid repeated calls which cause a needless amout of duplicated
	 * logging output (and CPU time as the calculation process is
	 * done) */
	if (freq == last_target) {
		return 0;
	}

    /* try to look for a valid frequency value from cpu frequency table */
    if (cpufreq_frequency_table_target(policy, sun4i_freq_tbl, freq, relation, &index)) {
        CPUFREQ_ERR("%s: try to look for a valid frequency for %u failed!\n", __func__, freq);
		return -EINVAL;
	}

	if (sun4i_freq_tbl[index].frequency == last_target) {
        /* frequency is same as the value last set, need not adjust */
		return 0;
	}
	freq = sun4i_freq_tbl[index].frequency;

    /* update the target frequency */
    freq_cfg.pll = sun4i_freq_tbl[index].frequency * 1000;
    freq_cfg.div = *(struct sun4i_clk_div_t *)&sun4i_freq_tbl[index].index;
    CPUFREQ_DBG("%s: target frequency find is %u, entry %u\n", __func__, freq_cfg.pll, index);

    /* try to set target frequency */
    ret = sun4i_cpufreq_settarget(policy, &freq_cfg);
    if(!ret) {
        last_target = freq;
    }

    return ret;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_get
*
*Description: get the frequency that cpu currently is running;
*
*Arguments  : cpu   cpu number;
*
*Return     : cpu frequency, based on khz;
*
*Notes      :
*
*********************************************************************************************************
*/
static unsigned int sun4i_cpufreq_get(unsigned int cpu)
{
	return clk_get_rate(clk_cpu) / 1000;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_init
*
*Description: cpu frequency initialise a policy;
*
*Arguments  : policy    cpu frequency policy;
*
*Return     : result, return 0 if init ok, else, return -EINVAL;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_init(struct cpufreq_policy *policy)
{
	CPUFREQ_DBG(KERN_INFO "%s: initialising policy %p\n", __func__, policy);

	if (policy->cpu != 0)
		return -EINVAL;

	policy->cur = sun4i_cpufreq_get(0);
	policy->min = policy->cpuinfo.min_freq = SUN4I_CPUFREQ_MIN / 1000;
	policy->max = policy->cpuinfo.max_freq = SUN4I_CPUFREQ_MAX / 1000;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	/* feed the latency information from the cpu driver */
	policy->cpuinfo.transition_latency = SUN4I_FREQTRANS_LATENCY;

	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_getcur
*
*Description: get current cpu frequency configuration;
*
*Arguments  : cfg   cpu frequency cofniguration;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_getcur(struct sun4i_cpu_freq_t *cfg)
{
    unsigned int    freq, freq0;

    if(!cfg) {
        return -EINVAL;
    }

	cfg->pll = clk_get_rate(clk_pll);
    freq = clk_get_rate(clk_cpu);
    cfg->div.cpu_div = cfg->pll / freq;
    freq0 = clk_get_rate(clk_axi);
    cfg->div.axi_div = freq / freq0;
    freq = clk_get_rate(clk_ahb);
    cfg->div.ahb_div = freq0 / freq;
    freq0 = clk_get_rate(clk_apb);
    cfg->div.apb_div = freq /freq0;

	return 0;
}



#ifdef CONFIG_PM

/* variable for backup cpu frequency configuration */
static struct sun4i_cpu_freq_t suspend_freq;
static __u32 suspend_vdd = 0;

/*
*********************************************************************************************************
*                           sun4i_cpufreq_suspend
*
*Description: back up cpu frequency configuration for suspend;
*
*Arguments  : policy    cpu frequency policy;
*             pmsg      power management message;
*
*Return     : return 0,
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_suspend(struct cpufreq_policy *policy)
{
    struct sun4i_cpu_freq_t suspend;

    CPUFREQ_DBG("%s, set cpu frequency to 60Mhz to prepare enter standby\n", __func__);

	if (NORMAL_STANDBY == standby_type) {
		/*process for normal standby*/
		printk("[%s] normal standby enter\n", __FUNCTION__);
    sun4i_cpufreq_getcur(&suspend_freq);

    /* set cpu frequency to 60M hz for standby */
    suspend.pll = 60000000;
    suspend.div.cpu_div = 1;
    suspend.div.axi_div = 1;
    suspend.div.ahb_div = 1;
    suspend.div.apb_div = 2;
    __set_cpufreq_target(&suspend_freq, &suspend);
	
	} else if(SUPER_STANDBY == standby_type) {
		/*process for super standby*/	
		CPUFREQ_DBG("[%s] super standby enter: \n", __FUNCTION__);
#if 1
		//backup suspend freq
		sun4i_cpufreq_getcur(&suspend_freq);

		/*backup suspend vdd*/
		//suspend_vdd = regulator_get_voltage(corevdd);
		suspend_vdd = last_vdd;
		mem_para_info.suspend_vdd = suspend_vdd;
		mem_para_info.suspend_freq = suspend.pll;
		CPUFREQ_DBG("backup suspend_vdd = %d. freq = %u. \n", suspend_vdd, suspend_freq.pll);

		ccmu_reg_backup((__ccmu_reg_list_t *)(SW_VA_CCM_IO_BASE));
#else 
		printk("do nothing. \n");
#endif
	}


    return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_resume
*
*Description: cpu frequency configuration resume;
*
*Arguments  : policy    cpu frequency policy;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int sun4i_cpufreq_resume(struct cpufreq_policy *policy)
{
    struct sun4i_cpu_freq_t suspend;

    /* invalidate last_target setting */
	last_target = ~0;

	CPUFREQ_DBG("%s: resuming with policy %p\n", __func__, policy);
	if (NORMAL_STANDBY == standby_type) {
		/*process for normal standby*/
		printk("[%s] normal standby resume\n", __FUNCTION__);
    sun4i_cpufreq_getcur(&suspend);

    /* restore cpu frequency configuration */
    __set_cpufreq_target(&suspend, &suspend_freq);

	CPUFREQ_DBG("%s: resuming done\n", __func__);
	} else if(SUPER_STANDBY == standby_type) {
		/*process for super standby*/	
		CPUFREQ_DBG("[%s] super standby resume: to %u hz\n", __FUNCTION__, suspend_freq.pll);
#if 0
		sun4i_cpufreq_getcur(&suspend);

		/*restore vdd*/
		if(regulator_set_voltage(corevdd, suspend_vdd*1000, suspend_vdd*1000)) {
			CPUFREQ_INF("try to set voltage failed!\n");
			new_vdd = last_vdd;
		}
		last_vdd = new_vdd;
		
		printk("restore suspend_vdd = %d. succeed. \n", suspend_vdd);
		
		/* restore cpu frequency configuration */
		__set_cpufreq_target(&suspend, &suspend_freq);
#endif		
		//last_vdd = 1400;
		cpu_cur.pll = 384000000;
		cpu_cur.div.cpu_div = 1;
		cpu_cur.div.axi_div = 1;
		cpu_cur.div.ahb_div = 2;
		cpu_cur.div.apb_div = 2;

		sun4i_cpufreq_getcur(&suspend);
		/* restore cpu frequency configuration */
		__set_cpufreq_target(&suspend, &suspend_freq);

		ccmu_reg_restore((__ccmu_reg_list_t *)(SW_VA_CCM_IO_BASE));

#ifdef CONFIG_CPU_FREQ_DVFS	

#if 0
		//type corevdd is not visible in the file.
		corevdd.max_uV = 0;
		corevdd->rdev = 0;
		((struct regulator *)corevdd)->min_uV = 1000000;
		(*corevdd).max_uV = 1400000;
#endif

#if 0
		/*can not all this interface at this point, why?*/
		regulator_set_voltage(corevdd, last_vdd*1000, last_vdd*1000);
#endif

#endif
		//print_flag = 1;
		CPUFREQ_DBG("%s: resuming done\n", __func__);
	}

	return 0;
}


#else   /* #ifdef CONFIG_PM */

#define sun4i_cpufreq_suspend   NULL
#define sun4i_cpufreq_resume    NULL

#endif  /* #ifdef CONFIG_PM */


static struct cpufreq_driver sun4i_cpufreq_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= sun4i_cpufreq_verify,
	.target		= sun4i_cpufreq_target,
	.get		= sun4i_cpufreq_get,
	.init		= sun4i_cpufreq_init,
	.suspend	= sun4i_cpufreq_suspend,
	.resume		= sun4i_cpufreq_resume,
	.name		= "sun4i",
};


/*
*********************************************************************************************************
*                           sun4i_cpufreq_initclks
*
*Description: init cpu frequency clock resource;
*
*Arguments  : none
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static __init int sun4i_cpufreq_initclks(void)
{
    clk_pll = clk_get(NULL, "core_pll");
    clk_cpu = clk_get(NULL, "cpu");
    clk_axi = clk_get(NULL, "axi");
    clk_ahb = clk_get(NULL, "ahb");
    clk_apb = clk_get(NULL, "apb");

	if (IS_ERR(clk_pll) || IS_ERR(clk_cpu) || IS_ERR(clk_axi) ||
	    IS_ERR(clk_ahb) || IS_ERR(clk_apb)) {
		CPUFREQ_INF(KERN_ERR "%s: could not get clock(s)\n", __func__);
		return -ENOENT;
	}

	CPUFREQ_INF("%s: clocks pll=%lu,cpu=%lu,axi=%lu,ahp=%lu,apb=%lu\n", __func__,
	       clk_get_rate(clk_pll), clk_get_rate(clk_cpu), clk_get_rate(clk_axi),
	       clk_get_rate(clk_ahb), clk_get_rate(clk_apb));

    #ifdef CONFIG_CPU_FREQ_DVFS
    corevdd = regulator_get(NULL, "axp20_core");
    if(!corevdd) {
        CPUFREQ_INF("try to get regulator failed, core vdd will not changed!\n");
    }
    else {
        CPUFREQ_INF("try to get regulator(0x%x) successed.\n", (__u32)corevdd);
        last_vdd = regulator_get_voltage(corevdd) / 1000;
    }
    #endif

	return 0;
}


/*
*********************************************************************************************************
*                           sun4i_cpufreq_initcall
*
*Description: cpu frequency driver initcall
*
*Arguments  : none
*
*Return     : result
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init sun4i_cpufreq_initcall(void)
{
	int ret = 0;

    /* initialise some clock resource */
    ret = sun4i_cpufreq_initclks();
    if(ret) {
        return ret;
    }

    /* initialise current frequency configuration */
	sun4i_cpufreq_getcur(&cpu_cur);
	sun4i_cpufreq_show("cur", &cpu_cur);

    /* register cpu frequency driver */
    ret = cpufreq_register_driver(&sun4i_cpufreq_driver);
    /* register cpu frequency table to cpufreq core */
    cpufreq_frequency_table_get_attr(sun4i_freq_tbl, 0);
    /* update policy for boot cpu */
    cpufreq_update_policy(0);

	return ret;
}
late_initcall(sun4i_cpufreq_initcall);

