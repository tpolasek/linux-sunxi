/*
*************************************************************************************
*                         			      Linux
*					           USB Host Controller Driver
*
*				        (c) Copyright 2006-2012, SoftWinners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: usb_msg_center.c
*
* Author 		: javen
*
* Description 	: USB 消息分发
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2011-4-14            1.0          create this file
*
*************************************************************************************
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <mach/irqs.h>

#include  "../include/sw_usb_config.h"
#include  "usb_manager.h"
#include  "usbc_platform.h"
#include  "usb_hw_scan.h"
#include  "usb_msg_center.h"
/*
extern int axp_usbvol(void );
extern int axp_usbcur(void);
extern int axp_usbvol_restore(void);
extern int axp_usbcur_restore(void);
*/
static struct usb_msg_center_info g_center_info;

void print_usb_msg(struct usb_msg_center_info * center_info)
{
	DMSG_DBG_MANAGER("hw_insmod_host   = %d\n", center_info->msg.hw_insmod_host);
	DMSG_DBG_MANAGER("hw_rmmod_host    = %d\n", center_info->msg.hw_rmmod_host);
	DMSG_DBG_MANAGER("hw_insmod_device = %d\n", center_info->msg.hw_insmod_device);
	DMSG_DBG_MANAGER("hw_rmmod_device  = %d\n", center_info->msg.hw_rmmod_device);
}

enum usb_role get_usb_role(void)
{
	return g_center_info.role;
}

static void set_usb_role(struct usb_msg_center_info *center_info, enum usb_role role)
{
	center_info->role = role;

	return;
}

/*
void app_insmod_usb_host(void)
{
	g_center_info.msg.app_insmod_host = 1;
}

void app_rmmod_usb_host(void)
{
	g_center_info.msg.app_rmmod_host = 1;
}

void app_insmod_usb_device(void)
{
	g_center_info.msg.app_insmod_device = 1;
}

void app_rmmod_usb_device(void)
{
	g_center_info.msg.app_rmmod_device = 1;
}
*/

void hw_insmod_usb_host(void)
{
	g_center_info.msg.hw_insmod_host = 1;
}

void hw_rmmod_usb_host(void)
{
	g_center_info.msg.hw_rmmod_host = 1;
}

void hw_insmod_usb_device(void)
{
	g_center_info.msg.hw_insmod_device = 1;
}

void hw_rmmod_usb_device(void)
{
	g_center_info.msg.hw_rmmod_device = 1;
}

/*
*******************************************************************************
*                     modify_msg
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void modify_msg(struct usb_msg *msg)
{
	if(msg->hw_insmod_host && msg->hw_rmmod_host){
		msg->hw_insmod_host = 0;
		msg->hw_rmmod_host  = 0;
	}

	if(msg->hw_insmod_device && msg->hw_rmmod_device){
		msg->hw_insmod_device = 0;
		msg->hw_rmmod_device  = 0;
	}

	return;
}

/*
*******************************************************************************
*                     insmod_host_driver
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void insmod_host_driver(struct usb_msg_center_info *center_info)
{
	DMSG_INFO("\n\ninsmod_host_driver\n\n");

	set_usb_role(center_info, USB_ROLE_HOST);
	sw_usb_host0_enable();

	return;
}

/*
*******************************************************************************
*                     rmmod_host_driver
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void rmmod_host_driver(struct usb_msg_center_info *center_info)
{
    int ret = 0;

	DMSG_INFO("\n\nrmmod_host_driver\n\n");

	ret = sw_usb_host0_disable();
	if(ret != 0){
    	DMSG_PANIC("err: disable hcd0 failed\n");
    	return;
	}

	set_usb_role(center_info, USB_ROLE_NULL);

	return;
}
/*
*******************************************************************************
*                     insmod_device_driver
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/

static void insmod_device_driver(struct usb_msg_center_info *center_info)
{
	DMSG_INFO("\n\ninsmod_device_driver\n\n");

    //axp_usbvol();
    //axp_usbcur();

	set_usb_role(center_info, USB_ROLE_DEVICE);
	sw_usb_device_enable();

	return;
}

/*
*******************************************************************************
*                     rmmod_device_driver
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void rmmod_device_driver(struct usb_msg_center_info *center_info)
{
	DMSG_INFO("\n\nrmmod_device_driver\n\n");

	set_usb_role(center_info, USB_ROLE_NULL);
	sw_usb_device_disable();

	//axp_usbcur_restore();
	//axp_usbvol_restore();

	return;
}

/*
*******************************************************************************
*                     do_usb_role_null
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void do_usb_role_null(struct usb_msg_center_info *center_info)
{
	if(center_info->msg.hw_insmod_host){
		insmod_host_driver(center_info);
		center_info->msg.hw_insmod_host = 0;

		goto end;
	}

	if(center_info->msg.hw_insmod_device){
		insmod_device_driver(center_info);
		center_info->msg.hw_insmod_device = 0;

		goto end;
	}

end:
	memset(&center_info->msg, 0, sizeof(struct usb_msg));

	return;
}

/*
*******************************************************************************
*                     do_usb_role_host
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void do_usb_role_host(struct usb_msg_center_info *center_info)
{
	if(center_info->msg.hw_rmmod_host){
		rmmod_host_driver(center_info);
		center_info->msg.hw_rmmod_host = 0;

		goto end;
	}

end:
	memset(&center_info->msg, 0, sizeof(struct usb_msg));

	return;
}

/*
*******************************************************************************
*                     do_usb_role_device
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
static void do_usb_role_device(struct usb_msg_center_info *center_info)
{
	if(center_info->msg.hw_rmmod_device){
		rmmod_device_driver(center_info);
		center_info->msg.hw_rmmod_device = 0;

		goto end;
	}

end:
	memset(&center_info->msg, 0, sizeof(struct usb_msg));

	return;
}

/*
*******************************************************************************
*                     usb_msg_center
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
void usb_msg_center(struct usb_cfg *cfg)
{
	enum usb_role role = USB_ROLE_NULL;
	struct usb_msg_center_info * center_info = &g_center_info;

	/* receive massage */
	print_usb_msg(center_info);

	modify_msg(&center_info->msg);

	/* execute cmd */
	role = get_usb_role();

	DMSG_DBG_MANAGER("role=%d\n", get_usb_role());

	switch(role){
		case USB_ROLE_NULL:
			do_usb_role_null(center_info);
		break;

		case USB_ROLE_HOST:
			do_usb_role_host(center_info);
		break;

		case USB_ROLE_DEVICE:
			do_usb_role_device(center_info);
		break;

		default:
			DMSG_PANIC("ERR: unkown role(%x)\n", role);
	}

	return;
}

/*
*******************************************************************************
*                     usb_msg_center_init
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
s32 usb_msg_center_init(struct usb_cfg *cfg)
{
	struct usb_msg_center_info *center_info = &g_center_info;

	memset(center_info, 0, sizeof(struct usb_msg_center_info));

	return 0;
}

/*
*******************************************************************************
*                     usb_msg_center_exit
*
* Description:
*    void
*
* Parameters:
*    void
*
* Return value:
*    void
*
* note:
*    void
*
*******************************************************************************
*/
s32 usb_msg_center_exit(struct usb_cfg *cfg)
{
	struct usb_msg_center_info *center_info = &g_center_info;

	memset(center_info, 0, sizeof(struct usb_msg_center_info));

	return 0;
}

#ifdef CONFIG_SUNXI_TEST_SELECT
int auto_scan_otg_flag = 1;

static ssize_t host_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
    char value;
    if(strlen(buf) != 2)
        return -EINVAL;
    if(buf[0] < '0' || buf[0] > '1')
		return -EINVAL;
    value = buf[0];
    switch(value)
    {
        case '1':
            hw_insmod_usb_host();
            break;
        case '0':
            hw_rmmod_usb_host();
            break;
        default:
            return -EINVAL;
    }
	return count;
}

static ssize_t device_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
    char value;
    if(strlen(buf) != 2)
        return -EINVAL;
    if(buf[0] < '0' || buf[0] > '1')
		return -EINVAL;
    value = buf[0];
    switch(value)
    {
        case '1':
            hw_insmod_usb_device();
            break;
        case '0':
            hw_rmmod_usb_device();
            break;
        default:
            return -EINVAL;
    }
	return count;
}

static ssize_t auto_scan_otg_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
    char value;
    if(strlen(buf) != 2)
        return -EINVAL;
    if(buf[0] < '0' || buf[0] > '1')
		return -EINVAL;
    value = buf[0];
    switch(value)
    {
        case '1':
            auto_scan_otg_flag = 1;
            break;
        case '0':
            auto_scan_otg_flag = 0;
            break;
        default:
            return -EINVAL;
    }
	return count;
}

static ssize_t auto_scan_otg_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	return sprintf(buf, "%d\n", auto_scan_otg_flag);
}

static ssize_t role_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	enum usb_role role = USB_ROLE_NULL;
	char *current_role;
	role = get_usb_role();

	switch(role){
		case USB_ROLE_NULL:
			current_role = "null";
			break;
		case USB_ROLE_HOST:
			current_role = "host";
			break;
		case USB_ROLE_DEVICE:
			current_role = "device";
			break;
		default:
			current_role = "unkown(error)";
	}

	return sprintf(buf, "%s\n", current_role);
}

static struct device_attribute otgtest_attrs[] = {
	__ATTR(host, 0200, NULL, host_store),
	__ATTR(device, 0200, NULL, device_store),
	__ATTR(auto_scan_otg, 0600, auto_scan_otg_show, auto_scan_otg_store),
	__ATTR(role, 0400, role_show, NULL),
};

static int otgtest_probe(struct platform_device *pdev)
{
	int ret,i;

	for (i = 0; i < ARRAY_SIZE(otgtest_attrs); i++) {
		ret = device_create_file(&pdev->dev, &otgtest_attrs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static int __exit otgtest_remove(struct platform_device *pdev)
{
	device_unregister(&pdev->dev);

	return 0;
}

static struct platform_device otgtest_device = {
	.name		= "otgtest",
};

static struct platform_driver otgtest_driver = {
	.probe		= otgtest_probe,
	.remove		= __exit_p(otgtest_remove),
	.driver		= {
		.name	= "otgtest",
		.owner	= THIS_MODULE,
	},
};

static int __init otgtest_init(void)
{
	platform_device_register(&otgtest_device);
	platform_driver_register(&otgtest_driver);

	return 0;
}
module_init(otgtest_init);

static void __exit otgtest_exit(void)
{
	platform_driver_unregister(&otgtest_driver);
	platform_device_unregister(&otgtest_device);
}
module_exit(otgtest_exit);
#endif
