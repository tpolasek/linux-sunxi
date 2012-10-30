#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/cache.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#define DATA_LEN (1024)
#define msg(fmt...) printk("SPI Msg> "fmt)
#define err(fmt...) printk("SPI Err> "fmt)
#define dbg(fmt...) printk("SPI dbg> "fmt)
#define FAIL (-1)
#define OK   (0)    

static int spi_transfer_test(struct spi_device *spi)
{
    struct spi_message	m;
	int i;
    int ret = 0;
	unsigned int* cmd  = (int*)kmalloc(DATA_LEN*sizeof(int)*2, GFP_KERNEL);
	unsigned int* data = cmd + DATA_LEN; 

    struct spi_transfer t = {
        .tx_buf = cmd,
        .rx_buf = data,
        .len    = DATA_LEN * sizeof(int),
    };

    msg("spi DUPLEXING testing started, len=%dBytes\n\n", DATA_LEN*4);
    if(!cmd){
        err("Fail to alloc memeory\n");
        return FAIL;
    }
    for (i = 0; i < DATA_LEN; i++) 
    {
        cmd[i] = ~i;
    }

    spi_message_init(&m);
    spi_message_add_tail(&t, &m);

    spi_sync(spi, &m);

	msg("tx data: \n");

    /*
	 *for(i=1;i<=DATA_LEN;i++){
	 *    msg("0x%x ", cmd[i-1]);
	 *    if(i%10 == 0)
	 *        msg("\n");
	 *}
     */

	msg("rx data: \n");

    /*
	 *for(i=1;i<=DATA_LEN;i++){
	 *    printk("0x%x ", data[i-1]);
	 *    if(i%16 == 0)
	 *        msg("\n");
	 *}
     */

	msg("\n");

    ret = memcmp(cmd, data, DATA_LEN);
    msg("%s to test duplexing transfer.\n", ret ? "FAIL" : "OK");

    kfree(cmd);
    return ret;
}

static int spi_test_probe(struct spi_device *spi)
{
    return spi_transfer_test(spi);
}

static int spi_test_remove(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver spi_test_driver = {
	.probe = spi_test_probe,
	.remove = __devexit_p(spi_test_remove),
	.driver = {
		.owner = THIS_MODULE,
		.name = "spitest",
	},
};

static int __init spi_test_init(void)
{
    msg("test init\n");

	return spi_register_driver(&spi_test_driver);
}

static void __exit spi_test_exit(void)
{
    msg("test exit\n");

	spi_unregister_driver(&spi_test_driver);
}

module_init(spi_test_init);
module_exit(spi_test_exit);

MODULE_DESCRIPTION("test SPI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:test");

