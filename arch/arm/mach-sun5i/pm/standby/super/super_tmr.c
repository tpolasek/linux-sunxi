/*
*********************************************************************************************************
*                                                    eMOD
*                                   the Easy Portable/Player Operation System
*                                            power manager sub-system
*
*                                     (c) Copyright 2008-2009, kevin.z China
*                                              All Rights Reserved
*
* File   : super_tmr.c
* Version: V1.0
* By     : kevin.z
* Date   : 2009-7-22 18:31
*********************************************************************************************************
*/
#include "super_i.h"


static __mem_tmr_reg_t  *TmrReg;
static __u32 TmrIntCtl, Tmr0Ctl, Tmr0IntVal, Tmr0CntVal, Tmr1Ctl, Tmr1IntVal, Tmr1CntVal;
