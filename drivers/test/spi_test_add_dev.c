/*
 * \file        spi_test_add_dev.c
 * \brief       test insmod/rmmod with spi bus
 *
 * \version     1.0.0
 * \date        2012-10-12
 * \author      Sam.Wu <wuehui@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */
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

#define msg(fmt...) printk("SPI> "fmt)

static int spi_test_probe(struct spi_device *spi)
{
    msg("%s called\n", __func__);
	return 0;
}

static int spi_test_remove(struct spi_device *spi)
{
    msg("%s called\n", __func__);
	return 0;
}

static struct spi_driver spi_test_driver = {
	.probe = spi_test_probe,
	.remove = __devexit_p(spi_test_remove),
	.driver = {
		.owner = THIS_MODULE,
		.name = "spinorflash",
	},
};

static int __init spi_test_init(void)
{
    msg("add device test init\n");

	return spi_register_driver(&spi_test_driver);
}

static void __exit spi_test_exit(void)
{
    msg("add device test exit\n");

	spi_unregister_driver(&spi_test_driver);
}

module_init(spi_test_init);
module_exit(spi_test_exit);

MODULE_DESCRIPTION("test SPI driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:test");


