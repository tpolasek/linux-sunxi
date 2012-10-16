/*
*************************************************************************************
*                         			      Linux
*					                 USB Host Driver
*
*				        (c) Copyright 2006-2010, All winners Co,Ld.
*							       All Rights Reserved
*
* File Name 	: sw_usb_pioneer.c
*
* Author 		: javen
*
* Description 	:
*
* History 		:
*      <author>    		<time>       	<version >    		<desc>
*       javen     	  2010-12-20           1.0          create this file
*
*************************************************************************************
*/
#include <linux/miscdevice.h>

#define  pioneer_panic(...)      (printk("ERR:L%d(%s):", __LINE__, __func__), printk(__VA_ARGS__))

//#define  PIONEER_DISK_DEBUG

#ifdef  PIONEER_DISK_DEBUG
#define  pioneer_debug(stuff...)  printk(stuff)
#else
#define  pioneer_debug(...)
#endif

#define   USB_WAIT_FOR_APP    20


#define   DIR_APP_TO_USB               0x01
#define   DIR_USB_TO_APP               0x02

//usb与application有关的状态
#define    USB_REQ_STATE_OK         0x00  //req正确执行
#define    USB_REQ_STATE_UNSETTLED  0x01  //req未处理
#define    USB_REQ_STATE_BUSY       0x02  //req正在被处理
#define    USB_REQ_STATE_LEN_ERR    0x03  //req长度错误
#define    USB_REQ_STATE_CANCEL     0x04  //被取消
#define    USB_REQ_STATE_RESET      0x05  //usb reset
#define    USB_REQ_STATE_RMMOD      0x06  //usb 即将被卸载
#define    USB_REQ_STATE_ELSE       0x07  //其他

struct usb_pioneer_req;
typedef	 void (* usb_pioneer_complete )(struct usb_pioneer_req *req, void *arg);

struct usb_pioneer_req{
    struct list_head queue;

    u8    *buf;		                    //存放数据
    u32   transfer_length;		        //期望长度，实际数据可以与期望的不一致
    u32   actual_length;		        //实际传输的长度
    s32   status;                       //req的执行状态
    u32   direction;                    //req的方向

    struct completion event;
    usb_pioneer_complete  complete;     //执行完毕后的callback

    void  *app_buf;                     //app buffer地址, 唯一标识req
    u32   cancel;
};

struct usb_pioneer_dev{
    struct fsg_dev *fsg;

	struct kref		kref;
	struct mutex	io_mutex;		/* synchronize I/O with disconnect      */
	int			    open_count;		/* count the number of openers          */

	struct list_head data_to_usb;   /* application send data to usb         */
	struct list_head data_from_usb; /* application receive data from usb    */
};

struct usb_pioneer_dev g_usb_pioneer_dev;

/*
void print_list_node(struct usb_pioneer_dev *dev, char *str)
{
	struct usb_pioneer_req *req = NULL;
	unsigned long		flags = 0;

	local_irq_save(flags);

	pioneer_debug("---------------%s-------------\n", str);

	list_for_each_entry (req, &dev->data_to_usb, queue) {
		pioneer_debug("data_to_usb:(0x%p, %d, %d, 0x%p)\n",
			         req, req->transfer_length, req->actual_length, req->app_buf);
	}

	list_for_each_entry (req, &dev->data_from_usb, queue) {
		pioneer_debug("data_from_usb:(0x%p, %d, %d, 0x%p,)\n\n",
			         req, req->transfer_length, req->actual_length, req->app_buf);
	}

	pioneer_debug("-------------------------------------\n");

	local_irq_restore(flags);

	return;
}
*/

/*
*******************************************************************************
*                     usb_pioneer_req_queue
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
static s32 usb_pioneer_req_queue (struct usb_pioneer_dev *dev, struct usb_pioneer_req *req)
{
    if(req == NULL){
		pioneer_panic("ERR: input is invalid\n");
		return -1;
	}

	req->status         = USB_REQ_STATE_UNSETTLED;
	req->actual_length  = 0;

	if(req->direction == DIR_APP_TO_USB){ //app->usb
    	list_add_tail(&req->queue, &dev->data_to_usb);
	}else if(req->direction == DIR_USB_TO_APP){ //usb->app
    	list_add_tail(&req->queue, &dev->data_from_usb);
	}else{
	    pioneer_panic("ERR: usb_pioneer_req_queue: req direction is invalid\n");
		return -1;
	}

    return 0;
}

/*
*******************************************************************************
*                     usb_pioneer_req_done
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
static void usb_pioneer_req_done(struct usb_pioneer_req *req, u32 status)
{
    list_del_init(&req->queue);

    if(!req->cancel){
        req->status = status;
    }else{
        req->status = USB_REQ_STATE_CANCEL;
    }

    req->complete(req, NULL);

    return;
}

/*
*******************************************************************************
*                     usb_pioneer_req_dequeue
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
static s32 usb_pioneer_req_dequeue(struct usb_pioneer_dev *dev, void * app_buff)
{
    struct usb_pioneer_req *req = NULL;

	list_for_each_entry (req, &dev->data_to_usb, queue) {
        if(req){
            if(req->app_buf == app_buff){
                req->cancel = 1;
                usb_pioneer_req_done(req, USB_REQ_STATE_CANCEL);
                return 0;
            }
        }
	}

	list_for_each_entry (req, &dev->data_from_usb, queue) {
        if(req){
            if(req->app_buf == app_buff){
                req->cancel = 1;
                usb_pioneer_req_done(req, USB_REQ_STATE_CANCEL);
                return 0;
            }
        }
	}

    return -1;
}

/*
*******************************************************************************
*                     req_callback
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
static void req_callback(struct usb_pioneer_req *req, void *arg)
{
    complete(&req->event);
}

/*
*******************************************************************************
*                     wait_for_complete
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
static void wait_for_complete(struct usb_pioneer_req *req)
{
    wait_for_completion(&req->event);
}

/*
*******************************************************************************
*                     alloc_pioneer_req
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
static struct usb_pioneer_req * alloc_pioneer_req(struct usb_pioneer_dev *dev,
                                                    u32 dir,
                                                    u32 buffer_size)
{
    struct usb_pioneer_req *req = NULL;

    req = kmalloc(sizeof(struct usb_pioneer_req), GFP_KERNEL);
    if(req == NULL){
        pioneer_panic("ERR: kmalloc failed\n");
        return NULL;
    }

    memset(req, 0, sizeof(struct usb_pioneer_req));

    req->buf = kmalloc(buffer_size, GFP_KERNEL);
    if(req->buf == NULL){
        pioneer_panic("ERR: kmalloc failed\n");
        kfree(req);
        return NULL;
    }

    init_completion(&req->event);

    req->transfer_length    = buffer_size;
    req->status             = USB_REQ_STATE_UNSETTLED;
    req->direction          = dir;
    req->complete           = req_callback;

    return req;
}

/*
*******************************************************************************
*                     free_pioneer_req
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
static s32 free_pioneer_req(struct usb_pioneer_dev *dev, struct usb_pioneer_req *req)
{
    if(req){
        if(req->buf){
            kfree(req->buf);
            req->buf = NULL;
        }

        kfree(req);
        req = NULL;
    }

    return 0;
}

static struct usb_pioneer_req * get_pioneer_req_from_list(struct list_head *head)
{
    return list_entry(head->next, struct usb_pioneer_req, queue);
}

/*
*******************************************************************************
*                     usb_pread
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
int usb_pread(u8 *buff, u32 offset, u32 count, u32 *arg)
{
    struct usb_pioneer_dev *dev = &g_usb_pioneer_dev;
    struct usb_pioneer_req *req = NULL;
    int xfer = 0;
	u32 cnt	= 0;
	u32 data_len_from_pc = *arg;

    /* wait for app request */
    while(1){
        if(list_empty(&dev->data_to_usb) == 0){
            cnt = 0;
			break;
        }

        cnt++;
        if(cnt > USB_WAIT_FOR_APP){
            pioneer_panic("ERR: usb_pread: lis data_to_usb is empty, cnt=%d\n", cnt);
			return 0;
        }

        msleep(5);
    }

    /* get reques */
    req = get_pioneer_req_from_list(&dev->data_to_usb);
    if(req == NULL){
        pioneer_panic("ERR: get_pioneer_req_from_list failed\n");
        return 0;
    }

    if(req->cancel){
	    usb_pioneer_req_done(req, USB_REQ_STATE_CANCEL);
    }

    xfer = min(count, (req->transfer_length - offset));
    req->actual_length += xfer;

    memcpy(buff , (req->buf + offset), xfer);

	if(req->actual_length == data_len_from_pc){
	    usb_pioneer_req_done(req, USB_REQ_STATE_OK);
	}

    return xfer;
}

/*
*******************************************************************************
*                     usb_pwrite
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
int usb_pwrite(u8 *buff, u32 offset, u32 count, u32 *arg)
{
    struct usb_pioneer_dev *dev = &g_usb_pioneer_dev;
    struct usb_pioneer_req *req = NULL;
    int xfer = 0;
	u32 cnt	= 0;
	u32 data_len_from_pc = *arg;

    /* wait for app request */
    while(1){
        if(list_empty(&dev->data_from_usb) == 0){
            cnt = 0;
			break;
        }

        cnt++;
        if(cnt > USB_WAIT_FOR_APP){
            pioneer_panic("ERR: usb_pwrite: lis data_to_usb is empty, cnt=%d\n", cnt);
			return 0;
        }

        msleep(5);
    }

    /* get reques */
    req = get_pioneer_req_from_list(&dev->data_from_usb);
    if(req == NULL){
        pioneer_panic("ERR: get_pioneer_req_from_list failed\n");
        return 0;
    }

    if(req->cancel){
        usb_pioneer_req_done(req, USB_REQ_STATE_CANCEL);
    }

    xfer = min(count, (req->transfer_length - offset));
    req->actual_length += xfer;

    memcpy((req->buf + offset), buff, xfer);

	if(req->actual_length == data_len_from_pc){
	    usb_pioneer_req_done(req, USB_REQ_STATE_OK);
	}

    return xfer;
}

/*
*******************************************************************************
*                     usb_pioneer_open
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
static int usb_pioneer_open(struct inode *inode, struct file *file)
{
	int retval = 0;

	file->private_data = &g_usb_pioneer_dev;

	return retval;
}

/*
*******************************************************************************
*                     usb_pioneer_release
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
static int usb_pioneer_release(struct inode *inode, struct file *file)
{
	int retval = 0;

	file->private_data = NULL;

	return retval;
}

/*
*******************************************************************************
*                     usb_pioneer_read
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
static ssize_t usb_pioneer_read(struct file *fp, char __user * buf,
				 size_t count, loff_t *pos)
{
    struct usb_pioneer_dev *dev = &g_usb_pioneer_dev;
    struct usb_pioneer_req *req = NULL;
    int ret = 0, xfer = 0;

    /* alloc req */
    req = alloc_pioneer_req(dev, DIR_USB_TO_APP, count);
    if(req == NULL){
        pioneer_panic("ERR: alloc_pioneer_req failed\n");
        return USB_REQ_STATE_UNSETTLED;
    }

    req->app_buf = (void*)buf;

    /* done req */
    usb_pioneer_req_queue(dev, req);

    wait_for_complete(req);

    /* copy usb data to app */
    if(req->status == USB_REQ_STATE_OK){
		xfer = (req->transfer_length < count) ? req->transfer_length : count;
        if (copy_to_user((void __user *)buf, req->buf, xfer)) {
            pioneer_panic("ERR: copy_to_user failed\n");
			req->status = -USB_REQ_STATE_ELSE;
			goto END;
		}
    }

END:
    ret = req->status;
    free_pioneer_req(dev, req);

    return ret;
}

/*
*******************************************************************************
*                     usb_pioneer_write
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
static ssize_t usb_pioneer_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
    struct usb_pioneer_dev *dev = &g_usb_pioneer_dev;
    struct usb_pioneer_req *req = NULL;
    int ret = 0, xfer = 0;

    /* alloc req */
    req = alloc_pioneer_req(dev, DIR_APP_TO_USB, count);
    if(req == NULL){
        pioneer_panic("ERR: alloc_pioneer_req failed\n");
        return USB_REQ_STATE_UNSETTLED;
    }

    req->app_buf = (void*)buf;

    /* copy app data to usb */
    xfer = (req->transfer_length < count) ? req->transfer_length : count;
    if (copy_from_user(req->buf, buf, xfer)) {
        pioneer_panic("ERR: copy_from_user failed\n");
    	req->status = -USB_REQ_STATE_ELSE;
    	goto END;
    }

    /* done req */
    usb_pioneer_req_queue(dev, req);

    wait_for_complete(req);

END:
    ret = req->status;
    free_pioneer_req(dev, req);

    return ret;
}

/*
*******************************************************************************
*                     usb_pioneer_ioctl
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
static long usb_pioneer_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
    struct usb_pioneer_dev *dev = &g_usb_pioneer_dev;
    long ret = 0;

    pioneer_debug("cmd=%d\n", cmd);

	switch (cmd){
        case 1:
        {
			ret = usb_pioneer_req_dequeue(dev, (void *)arg);
		}
		break;

		default:
			pioneer_panic("ERR: unkown cmd(%d)\n", cmd);
	}

    return ret;
}

static struct file_operations usb_pioneer_fops = {
	.owner          = THIS_MODULE,
	.read			= usb_pioneer_read,
	.write 			= usb_pioneer_write,
	.open 			= usb_pioneer_open,
	.release 		= usb_pioneer_release,
	.flush 			= NULL,
	.unlocked_ioctl = usb_pioneer_ioctl,
};

static struct miscdevice usb_pioneer_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sw_usb_pioneer",
	.fops = &usb_pioneer_fops,
};

/*
*******************************************************************************
*                     usb_pioneer_init
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
static int usb_pioneer_init(struct fsg_dev *fsg)
{
	int ret = 0;

	memset(&g_usb_pioneer_dev, 0, sizeof(struct usb_pioneer_dev));

	g_usb_pioneer_dev.fsg = fsg;

	kref_init(&g_usb_pioneer_dev.kref);
    mutex_init(&g_usb_pioneer_dev.io_mutex);

	INIT_LIST_HEAD (&g_usb_pioneer_dev.data_to_usb);
	INIT_LIST_HEAD (&g_usb_pioneer_dev.data_from_usb);

	ret = misc_register(&usb_pioneer_device);
	if (ret) {
	    pioneer_panic("L%d%s: ERR: misc_register failed\n", __LINE__, __FILE__);
	    ret = -1;
		goto err1;
    }

 err1:
	return ret;
}

/*
*******************************************************************************
*                     usb_pioneer_exit
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
static int usb_pioneer_exit(struct fsg_dev *fsg)
{
    misc_deregister(&usb_pioneer_device);

    return 0;
}














