/*
 * \file        iic_add_or_remove_dev.c
 * \brief       test add or remove device 
 *
 * \version     1.0.0
 * \date        2012-10-9
 * \author      Sam.Wu <wuehui@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
 
#define msg(fmt...) printk("TST> "fmt)

static int i2c_driver_demo_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    msg("%s\n", __func__);
    return 0;
}
 
static int __devexit i2c_driver_demo_remove(struct i2c_client *client)
{
    msg("%s\n", __func__);
    return 0;
}
 
static const struct i2c_device_id i2c_driver_demo_id[] = {
    { "XXXX", 0 },
    {}
};
MODULE_DEVICE_TABLE(i2c, i2c_driver_demo_id);
 
int i2c_driver_demo_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    struct i2c_adapter *adapter = client->adapter;

 
    msg("%s called\n", __func__);
#if 0
    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -ENODEV;
 
    /* 方法1：获取设备特定寄存器的值，该值要能反映出设备的信息，判断设备，例如如下代码段 */
    vendor = i2c_smbus_read_byte_data(client, XXXX_REG_VENDOR);
    if (vendor != XXXX_VENDOR)
        return -ENODEV;
 
    device = i2c_smbus_read_byte_data(client, XXXX_REG_DEVICE);
    if (device != XXXX_DEVICE)
        return -ENODEV;
 
    revision = i2c_smbus_read_byte_data(client, XXXX_REG_REVISION);
    if (revision != XXXX_REVISION)
    return -ENODEV;
 
    /* 方法2：获取设备的CHIP_ID，判断设备，例如如下代码 */
    if (i2c_smbus_read_byte_data(client, XXXX_CHIP_ID_REG) != XXXX_CHIP_ID)
        return -ENODEV;
 
    const char *type_name = "XXXX";
    strlcpy(info->type, type_name, I2C_NAME_SIZE);
#endif//

    return 0;
}
 
/* 0x60为I2C设备地址 */
static const unsigned short normal_i2c[] = {0x60, I2C_CLIENT_END};
 
static struct i2c_driver i2c_driver_demo = {
    .class = I2C_CLASS_HWMON,
    .probe        = i2c_driver_demo_probe,
    .remove        = __devexit_p(i2c_driver_demo_remove),
    .id_table    = i2c_driver_demo_id,
    .driver    = {
        .name    = "XXXX",
        .owner    = THIS_MODULE,
    },
    .detect        = i2c_driver_demo_detect,
    .address_list    = normal_i2c,
};
 
static int __init i2c_driver_demo_init(void)
{
    return i2c_add_driver(&i2c_driver_demo);
}
 
static void __exit i2c_driver_demo_exit(void)
{
    msg("%s called\n", __func__);
    i2c_del_driver(&i2c_driver_demo);
}
 
module_init(i2c_driver_demo_init);
module_exit(i2c_driver_demo_exit);

MODULE_AUTHOR("anchor");
MODULE_DESCRIPTION("I2C device driver demo");
MODULE_LICENSE("GPL");


