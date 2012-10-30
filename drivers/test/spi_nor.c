/*
 * \file        spi_nor.c
 * \brief       simple device driver for spi_nor flash
 *
 * \version     1.0.0
 * \date        2012-10-22
 * \author      Sam.Wu <wuehui@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/ioctl.h>
#include <linux/blk_types.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/dcache.h>

#define msg(fmt...) printk("INFO_NOR:"fmt)            
#define dbg(fmt...)  //do{printk("DBG_NOR %s>%d:", __func__, __LINE__); printk(fmt);}while(0)
#define err(fmt...) do{printk("ERR_NOR %s>%d:", __func__, __LINE__); printk(fmt);}while(0)
#define OK  (0)
#define FAIL (-1)    

#define SYSTEM_PAGE_SIZE     (512)
#define SPI_NOR_PAGE_SIZE    (256)
#define SPI_NOR_SECTOR_SIZE  (64*1024)
#define NPAGE_IN_1SYSPAGE    (SYSTEM_PAGE_SIZE/SPI_NOR_PAGE_SIZE)

#define SPI_NOR_READ  0x03
#define SPI_NOR_FREAD 0x0b
#define SPI_NOR_WREN  0x06
#define SPI_NOR_WRDI  0x04
#define SPI_NOR_RDSR  0x05
#define SPI_NOR_WRSR  0x01
#define SPI_NOR_PP    0x02
#define SPI_NOR_SE    0xd8
#define SPI_NOR_BE    0xc7
//#define SPI_NOR_RDID  0x90 
#define SPI_NOR_RDID  0x9F 
#define SST_AAI_PRG   0xad

static __u32 txnum;
static __u32 rxnum;

static struct spi_device* spidev[4] = {};

int spic_rw(unsigned txBytes, const char* txBuf, unsigned rxBytes, char* rxBuf, int devIndex)
{
    int ret = FAIL;
    struct spi_message  message;
    struct spi_transfer xfer[2] ;

    memset(xfer, 0, sizeof(xfer));
    spi_message_init(&message);

    if(txBuf)
    {
        xfer[0].tx_buf = txBuf;
        xfer[0].len    = txBytes;
        spi_message_add_tail(&xfer[0], &message);
    }
    if(rxBuf)
    {
        xfer[1].rx_buf = rxBuf;
        xfer[1].len    = rxBytes;
        spi_message_add_tail(&xfer[1], &message);
    }

    ret = spi_sync(spidev[devIndex], &message);
    dbg("message xfer len=%d, txBytes=%d, rxBytes=%d\n", message.actual_length, txBytes, rxBytes);

    if(message.actual_length == (rxBytes + txBytes))
    {
        return OK;
    }

    return FAIL;
}

/*  Global Function  */
s32 spi_nor_rdid(u32* id, int devIndex)
{
    s32 ret = FAIL;
    u8 sdata = SPI_NOR_RDID;
    txnum = 1;
	rxnum = 3;
	    
    ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)id, devIndex);

	return ret;
}
/*  local function  */
static s32 spi_nor_wren(int devIndex)
{
    u8 sdata = SPI_NOR_WREN;
    s32 ret = FAIL;
    
	txnum = 1;
    rxnum = 0;
	pr_debug("spi_nor_wren\n");
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)0, devIndex);
	
	return ret;
}


static s32 spi_nor_wrdi(int devIndex)
{
	u8 sdata = SPI_NOR_WRDI;
    s32 ret = FAIL;
    
	txnum = 1;
	rxnum = 0;
	pr_debug("spi_nor_wrdi\n");
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)0, devIndex);
	
	return ret;
}


static s32 spi_nor_rdsr(u8* reg, int devIndex)
{
    s32 ret = FAIL;
    u8 sdata = SPI_NOR_RDSR;
    rxnum = 1;
	txnum = 1;
	
//	pr_debug("spi_nor_rdsr\n");
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)reg, devIndex);
//	printk("spi nor get status [%p]:%02x\n", reg, *reg);
	return ret;
}

static s32  spi_nor_wrsr(u8 reg, int devIndex)
{
	u8 sdata[2] = {0};
    s32 ret = FAIL;
    u8  status = 0;
    u32 i = 0;
    
	ret = spi_nor_wren(devIndex);
	if (ret==FAIL)
	    goto err_out_;
	    
	txnum = 2;
	rxnum = 0;
	
	sdata[0] = SPI_NOR_WRSR;
	sdata[1] = reg;
	pr_debug("spi_nor_wrsr\n");
	ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0, devIndex);
	if (ret==FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status, devIndex);
	    if (ret==FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = OK;
	
err_out_:
    
    return ret;
}

static s32 spi_nor_pp(u32 page_addr, void* buf, u32 len, int devIndex)
{
    u8 sdata[260];
    u8  status = 0;
    u32 i = 0;
    s32 ret = FAIL;
    
    if (len>256)
    {
        ret = FAIL;
        err("buf len %d > maxLen 256\n", len);
        goto err_out_;
    }
    
	ret = spi_nor_wren(devIndex);
	if (ret==FAIL)
	    goto err_out_;
	    
	txnum = len+4;
	rxnum = 0;
	
	memset((void*)sdata, 0xff, 260);
	sdata[0] = SPI_NOR_PP;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
	memcpy((void*)(sdata+4), buf, len);
	dbg("spi_nor_pp\n");
	ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0, devIndex);
	if (ret==FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status, devIndex);
	    if (ret==FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = OK;
	
err_out_:
    
    return ret;
}

s32 spi_nor_bulk_erase(int devIndex)
{
    u8 sdata = 0;
    s32 ret = FAIL;
    u8  status = 0;
    u32 i = 0;
//    u32 busy = 0x55;

	ret = spi_nor_wren(devIndex);
	if (ret==FAIL)
	    goto err_out_;
	    
	txnum = 1;
	rxnum = 0;
	
	sdata = SPI_NOR_BE;
	
	//while(busy);
	
	ret = spic_rw(txnum, (void*)&sdata, rxnum, (void*)0, devIndex);
	if (ret==FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status, devIndex);
	    if (ret==FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = OK;
	
err_out_:    
    return ret;
}


s32 spi_nor_sector_erase(u32 page_num, int devIndex)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u8 sdata[4] = {0};
    s32 ret = FAIL;
    u8  status = 0;
    u32 i = 0;
    
    msg("xx %s, page_num=%d\n", __func__, page_num);
	ret = spi_nor_wren(devIndex);
	if (ret==FAIL)
	    goto err_out_;
	    
	txnum = 4;
	rxnum = 0;
	
	sdata[0] = SPI_NOR_SE;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
	
	ret = spic_rw(txnum, (void*)sdata, rxnum, (void*)0, devIndex);
	if (ret==FAIL)
        goto err_out_;
	
	do {
	    ret = spi_nor_rdsr(&status, devIndex);
	    if (ret==FAIL)
	        goto err_out_;
	        
	    for(i=0; i<100; i++);
	} while(status&0x01);
	
	ret = OK;
	
err_out_:
    
    return ret;
}

s32 spi_nor_read(u32 page_num, u32 page_cnt, void* buf, int devIndex)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u32 rbyte_cnt = page_cnt * SYSTEM_PAGE_SIZE;
    u8  sdata[4] = {0};
    s32 ret = FAIL;
    
    dbg("%s, page_num=%d, cnt-%d, buf=%p\n", __func__, page_num, page_cnt, buf);

    txnum = 4;
	rxnum = rbyte_cnt;
    
	sdata[0] = SPI_NOR_READ;
	sdata[1] = (page_addr>>16)&0xff;
	sdata[2] = (page_addr>>8)&0xff;
	sdata[3] = page_addr&0xff;
    
    ret = spic_rw(txnum, (void*)sdata, rxnum, buf, devIndex);
    
    return ret;
}

s32 spi_nor_write(u32 page_num, u32 page_cnt, const void* buf, int devIndex)
{
    u32 page_addr = page_num * SYSTEM_PAGE_SIZE;
    u32 nor_page_cnt = page_cnt * (NPAGE_IN_1SYSPAGE);
    u32 i = 0;
    s32 ret = OK;

    dbg("%s, page_num=%d, cnt-%d, buf=%p\n", __func__, page_num, page_cnt, buf);
    
    for (i=0; i<nor_page_cnt && !ret; i++)
    {
        ret = spi_nor_pp(page_addr+SPI_NOR_PAGE_SIZE*i, (void*)((u32)buf+SPI_NOR_PAGE_SIZE*i),
                SPI_NOR_PAGE_SIZE, devIndex);
    }
    
    return ret;
}
#if 0
int spi_nor_test(void)
{
    u32 test_len = 512;
    u8* wbuf = (u8*)kmalloc(test_len*sizeof(u8), GFP_KERNEL);
    u8* rbuf = (u8*)kmalloc(test_len*sizeof(u8) , GFP_KERNEL);
    u32 i = 0;
    int ret = OK;
    
    msg("xxxdd %s \n", __func__);
    /*
     *for (i=0; i<test_len; i++)
     *{
     *    wbuf[i] = ~i;
     *}
     */

    for (i=0; i<128 && !ret; i++)
    {
        memset(wbuf, i, test_len);
        memset(rbuf, 7, test_len);

        spi_nor_write(i, 1, wbuf);
        spi_nor_read(i, 1, rbuf);
        
        ret = memcmp(wbuf, rbuf, 512);
        msg("%s to test spi nor page %d !---\n\n", ret ? "FAIL" : "OK", i);
    }
  
    kfree(wbuf);
    kfree(rbuf);
    return ret;
}

/*  spi nor test  */
s32 spi_nor_rw_test(void)
{
    u32 test_len = 512;
    u8* wbuf = (u8*)kmalloc(test_len*sizeof(u8), GFP_KERNEL);
    u8* rbuf = (u8*)kmalloc(test_len*sizeof(u8) , GFP_KERNEL);
    u8* buf =(u8*)kmalloc(65536*sizeof(u8), GFP_KERNEL);
    u32 i,j, temp  = 0;
    u32 sector_num = 0;       
    int ret = FAIL;
    
    memset(buf, 0, 65536*sizeof(u8));
    
    for (i=0; i<test_len; i++)
    {
        wbuf[i] = ~i;
    }
    
   dbg("*******test 1--erase all*********\n\n");
//    spi_nor_bulk_erase();
   
   dbg("*******test 2--erase 64kbytes*********\n\n");
    //擦除0扇区，64KB
   spi_nor_sector_erase(sector_num*128);
    
    dbg("\n*******test 3--write 64kbytes*********\n\n");
    //写64KB数据,
    for (i=0; i<128; i++)
    {
        //写1KB数据
        spi_nor_write(sector_num*128+i, 1, wbuf);
        memset(rbuf, 0, 512*sizeof(u8));
        spi_nor_read(sector_num*128+i, 1, rbuf);
        
        ret = memcmp(wbuf, rbuf, 512);
        msg("%s to test spi nor page %d !---\n\n", ret ? "FAIL" : "OK", i);
    }
        
    printk("\n*******test 4--read 64kbytes*********\n\n");    
    spi_nor_read(sector_num*128, 128, buf);
    
    for(j=0;j<65536;j++)
    {
        dbg("buf[%d] = %d \n", j, buf[j]);
    }    

    msg("\n*******test 5--read id*********\n\n"); 
    spi_nor_rdid(&temp);
    msg("id = %x\n", temp);
    
    kfree(wbuf);
    kfree(rbuf);
    kfree(buf);   
    
    return OK;
}
#endif//if0

static int f_open(struct inode* inode, struct file* file)
{
    char pathBuf[256] = "";
    char* pathName = NULL;

    msg("%s\n", __func__);

    pathName = d_path(&file->f_path, pathBuf, 256);
    msg("path name is %s\n", pathName);

    return 0;
}

static int f_release(struct inode* inode, struct file* file)
{
    msg("%s\n", __func__);

    return 0;
}

static int f_read(struct file* filep, char* buff, size_t nSec, loff_t* offp)
{
    int ret = FAIL;
    const unsigned Cnt = nSec * 512;
    char* rxData = kmalloc(Cnt, GFP_KERNEL);
    char pathBuf[256] = "";
    char* pathName = NULL;
    int devIndex = 0;

    pathName = d_path(&filep->f_path, pathBuf, 256);
    devIndex = pathName[strlen(pathName) - 1] - '0';
    msg("%s, path name is %s, devIndex=%d\n", __func__, pathName, devIndex);

    dbg("%s, buff=0x%p, nSec=%d\n", __func__, buff, nSec);
    if(!rxData){
        err("Fail to alloc buffer\n");
        return FAIL;
    }
    ret = spi_nor_read(0, nSec, rxData, devIndex);
    if(!ret)
    {
        ret = copy_to_user(buff, rxData, Cnt);
        /*ret = (ret == Cnt) ? OK : ret;*/
    }

    kfree(rxData);
    return ret;
}

static int f_write(struct file* filep, const char* buff, size_t nSec, loff_t* offp)
{
    int ret = FAIL;
    const unsigned Cnt = nSec * 512;
    char* data = kmalloc(Cnt, GFP_KERNEL);
    char pathBuf[256] = "";
    char* pathName = NULL;
    int devIndex = 0;

    pathName = d_path(&filep->f_path, pathBuf, 256);
    devIndex = pathName[strlen(pathName) - 1] - '0';
    msg("%s, path name is %s, devIndex=%d\n", __func__, pathName, devIndex);

    dbg("%s, buff=0x%p, nSec=%d\n", __func__, buff, nSec);
    if(!data){
        err("Fail to alloc buffer\n");
        return FAIL;
    }
    ret = copy_from_user(data, buff, Cnt);
    if(ret){
        err("fail to copy from user, want %d, but ret = %d\n", Cnt, ret);
        goto _errout;
    }
    ret = spi_nor_write(0, nSec, data, devIndex);

_errout:
    kfree(data);
    return ret;
}

static const struct file_operations _myfops = {
    .owner   = THIS_MODULE,
    .open    = f_open,
    .release = f_release,
    .read    = f_read,
    .write   = f_write
};

static int create_dev(int devIndex)
{
    struct class* devClass;
    dev_t devid;
    int err = 0;
    static struct cdev* myCdev;
    struct device* theDevice = NULL;
    char nodeName[] = "nor0";

    nodeName[strlen(nodeName) - 1] += devIndex;
    msg("%s, nodeName=%s, len=%d, devIndex=%d\n", __func__, nodeName, strlen(nodeName), devIndex);

    alloc_chrdev_region(&devid, 0, 1,nodeName);
    myCdev = cdev_alloc();
    cdev_init(myCdev, &_myfops);
    myCdev->owner = THIS_MODULE;
    err = cdev_add(myCdev, devid, 1);
    if(err){
        err("cdev add failed\n");
        return FAIL;
    }

    devClass = class_create(THIS_MODULE, nodeName);
    if(IS_ERR(devClass)){
        err("Fail to class create\n");
        return FAIL;
    }

    theDevice = device_create(devClass, NULL, devid, NULL,nodeName);

    return 0;
}

static int spi_test_probe(struct spi_device *spi)
{
    int ret = FAIL;
    int devIndex = spi->modalias[strlen(spi->modalias) - 1] - '0';

    dbg("1\n");
    msg("\ndevice name = %s\n", spi->modalias);
    msg("device freq = %d\n", spi->max_speed_hz);
    msg("device bus_num = %d\n", spi->master->bus_num);
    msg("device cs = %d\n", spi->chip_select);
    msg("device mode = %d\n", spi->mode);
    spidev[devIndex] = spi;

    ret = create_dev(devIndex);
    /*
     *ret = spi_nor_bulk_erase();
     *if(ret){
     *    msg("Fail to erase nor flash\n");
     *    return FAIL;
     *}
     *msg("OK to erase nor flash\n");
     */

    /*ret = spi_nor_rw_test();*/
    spi_nor_sector_erase(0, devIndex);
    /*ret = spi_nor_test();*/

	return ret;
}

static int spi_test_remove(struct spi_device *spi)
{
    dbg("\n");
	return 0;
}

static struct spi_driver spi_dev_nor[3] = 
{
	[0].probe  = spi_test_probe,
	[1].probe  = spi_test_probe,
	[2].probe  = spi_test_probe,

	[0].remove = __devexit_p(spi_test_remove),
	[1].remove = __devexit_p(spi_test_remove),
	[2].remove = __devexit_p(spi_test_remove),

	[0].driver.owner = THIS_MODULE,
	[1].driver.owner = THIS_MODULE,
	[2].driver.owner = THIS_MODULE,
	[0].driver.name = "norflash0", 
	[1].driver.name = "norflash1", 
	[2].driver.name = "norflash2", 
};

static int __init spi_test_init(void)
{
    int ret = OK;
    int i = 0;

    for (i = 0; i < 3 && !ret; i++) 
    {
        msg("\n\nTo register spi dev %d\n", i);
        ret = spi_register_driver(spi_dev_nor + i);
    }

	return ret;
}

static void __exit spi_test_exit(void)
{
    int i = 0;

    for (i = 0; i < 3; i++) 
    {
        msg("To Unregister spi dev %d\n", i);
        spi_unregister_driver(spi_dev_nor + i);
    }

	return ;
}

module_init(spi_test_init);
module_exit(spi_test_exit);

MODULE_DESCRIPTION("SPI nor flash driver for sunxi_bsp test");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:nor2");

