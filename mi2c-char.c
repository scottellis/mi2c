/*
  Char device implementation for a kernel module that uses i2c.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/slab.h>

#include "mi2c.h"
#include "mi2c-i2c.h"

#define USER_BUFF_SIZE 128


struct mi2c_dev {
	dev_t devt;
	struct cdev cdev;
	struct semaphore sem;
	struct class *class;
	char *user_buff;
};

static struct mi2c_dev mi2c_dev;


static ssize_t mi2c_write(struct file *filp, const char __user *buff,
		size_t count, loff_t *f_pos)
{
	ssize_t status;
	size_t len = USER_BUFF_SIZE - 1;

	if (count == 0)
		return 0;

	if (down_interruptible(&mi2c_dev.sem))
		return -ERESTARTSYS;

	if (len > count)
		len = count;
	
	memset(mi2c_dev.user_buff, 0, USER_BUFF_SIZE);
	
	if (copy_from_user(mi2c_dev.user_buff, buff, len)) {
		status = -EFAULT;
		goto mi2c_write_done;
	}

	/* do something with the user data */

	status = len;
	*f_pos += len;

mi2c_write_done:

	up(&mi2c_dev.sem);

	return status;
}

static ssize_t mi2c_read(struct file *filp, char __user *buff, 
				size_t count, loff_t *offp)
{
	ssize_t status;
	size_t len;

	/* 
	Generic user progs like cat will continue calling until we 
	return zero. So if *offp != 0, we know this is at least the
	second call.
	*/
	if (*offp > 0)
		return 0;

	if (down_interruptible(&mi2c_dev.sem)) 
		return -ERESTARTSYS;

	strcpy(mi2c_dev.user_buff, "mi2c driver data\n");

	len = strlen(mi2c_dev.user_buff);

	if (len > count)
		len = count;

	if (copy_to_user(buff, mi2c_dev.user_buff, len)) {
		status = -EFAULT;
		goto mi2c_read_done;
	}

	*offp += len;
	status = len;

mi2c_read_done:
			
	up(&mi2c_dev.sem);
	
	return status;	
}

static int mi2c_open(struct inode *inode, struct file *filp)
{	
	int status = 0;

	if (down_interruptible(&mi2c_dev.sem)) 
		return -ERESTARTSYS;
	
	if (!mi2c_dev.user_buff) {
		mi2c_dev.user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);

		if (!mi2c_dev.user_buff) {
			printk(KERN_ALERT 
				"mi2c_open: user_buff alloc failed\n");

			status = -ENOMEM;
		}
	}

	up(&mi2c_dev.sem);

	return status;
}

static const struct file_operations mi2c_fops = {
	.owner = THIS_MODULE,
	.open =	mi2c_open,	
	.read =	mi2c_read,
	.write = mi2c_write,
};

static int __init mi2c_init_cdev(void)
{
	int error;

	mi2c_dev.devt = MKDEV(0, 0);

	error = alloc_chrdev_region(&mi2c_dev.devt, 0, 1, DRIVER_NAME);
	if (error < 0) {
		printk(KERN_ALERT 
			"alloc_chrdev_region() failed: error = %d \n", 
			error);
		
		return -1;
	}

	cdev_init(&mi2c_dev.cdev, &mi2c_fops);
	mi2c_dev.cdev.owner = THIS_MODULE;

	error = cdev_add(&mi2c_dev.cdev, mi2c_dev.devt, 1);
	if (error) {
		printk(KERN_ALERT "cdev_add() failed: error = %d\n", error);
		unregister_chrdev_region(mi2c_dev.devt, 1);
		return -1;
	}	

	return 0;
}

static int __init mi2c_init_class(void)
{
	mi2c_dev.class = class_create(THIS_MODULE, DRIVER_NAME);

	if (!mi2c_dev.class) {
		printk(KERN_ALERT "class_create(mi2c) failed\n");
		return -1;
	}

	if (!device_create(mi2c_dev.class, NULL, mi2c_dev.devt, 
				NULL, DRIVER_NAME)) {
		class_destroy(mi2c_dev.class);
		return -1;
	}

	return 0;
}

static int __init mi2c_init(void)
{
	printk(KERN_INFO "mi2c_init()\n");

	memset(&mi2c_dev, 0, sizeof(struct mi2c_dev));

	sema_init(&mi2c_dev.sem, 1);

	if (mi2c_init_cdev() < 0)
		goto init_fail_1;

	if (mi2c_init_class() < 0)
		goto init_fail_2;

	if (mi2c_init_i2c() < 0)
		goto init_fail_3;


	return 0;

init_fail_3:
	device_destroy(mi2c_dev.class, mi2c_dev.devt);
	class_destroy(mi2c_dev.class);

init_fail_2:
	cdev_del(&mi2c_dev.cdev);
	unregister_chrdev_region(mi2c_dev.devt, 1);

init_fail_1:

	return -1;
}
module_init(mi2c_init);

static void __exit mi2c_exit(void)
{
	printk(KERN_INFO "mi2c_exit()\n");

	mi2c_cleanup_i2c();

	device_destroy(mi2c_dev.class, mi2c_dev.devt);
  	class_destroy(mi2c_dev.class);

	cdev_del(&mi2c_dev.cdev);
	unregister_chrdev_region(mi2c_dev.devt, 1);

	if (mi2c_dev.user_buff)
		kfree(mi2c_dev.user_buff);
}
module_exit(mi2c_exit);


MODULE_AUTHOR("Scott Ellis");
MODULE_DESCRIPTION("mi2c driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");

