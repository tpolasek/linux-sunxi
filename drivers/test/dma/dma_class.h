#ifndef __DMA_CLASS_H__
#define __DMA_CLASS_H__

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/gfp.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/dma.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/dma-mapping.h>
#include <linux/wait.h>
#include <linux/random.h>

#include <mach/dma.h>

/*
 * dma test case id
 */
enum dma_test_case_e {
	DTC_NORMAL = 0,		/* case for normal channel */
	DTC_NORMAL_CONT_MODE,	/* case for normal channel continue mode */
	DTC_DEDICATE,		/* case for dedicate channel */
	DTC_DEDICATE_CONT_MODE,	/* case for dedicate channel continue mode */
	/*
	 * for dedicate below
	 */
	DTC_ENQ_AFT_DONE,	/* enqueued buffer after dma last done, to see if can cotinue auto start */
	DTC_MANY_ENQ, 		/* many buffer enqueued, function test */
	DTC_CMD_STOP,		/* stop when dma running */
	DTC_M2M_TWO_THREAD,	/* two-thread run simutalously, pressure test and memory leak test */
	DTC_MAX
};

struct test_result{
	char	*name;
	int		result;
	void	*vsrcp;
	void	*vdstp;
};

extern wait_queue_head_t g_dtc_queue[];
extern atomic_t g_adma_done;
extern struct test_result dma_test_result[8];
/*dedicate test*/
/*-----------------------------------*/
/* total length and one buf length */
#define TOTAL_LEN_DEDICATE	SZ_256K
#define ONE_LEN_DEDICATE	SZ_16K

u32 __dtc_dedicate(int id);
u32 __dtc_dedicate_conti(int id);
/*-----------------------------------*/

/*normal test*/
/*-----------------------------------*/
/* total length and one buf length */
#define TOTAL_LEN_NORMAL	SZ_128K
#define ONE_LEN_NORMAL		SZ_8K

u32 __dtc_normal(int id);
u32 __dtc_normal_conti(int id);

/*-----------------------------------*/

/*other test*/
/*-----------------------------------*/
/* total length and one buf length */
#define TOTAL_LEN_EAD		SZ_256K
#define ONE_LEN_EAD		SZ_32K

#define TOTAL_LEN_MANY_ENQ	SZ_256K
#define ONE_LEN_MANY_ENQ	SZ_4K

#define TOTAL_LEN_STOP		SZ_512K
#define ONE_LEN_STOP		SZ_4K

u32 __dtc_enq_aftdone(int id);
u32 __dtc_many_enq(int id);
u32 __dtc_stop(int id);
/*-----------------------------------*/

/*muti_thread test*/
/*-----------------------------------*/
/* total length and one buf length */
#define TOTAL_LEN_2		SZ_512K
#define ONE_LEN_2		SZ_16K

#define TOTAL_LEN_1		SZ_512K
#define ONE_LEN_1		SZ_64K

u32 __dtc_two_thread(int id);
/*-----------------------------------*/
#endif
