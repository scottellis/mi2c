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

/* 
 * An example just to demo how you would use the write/read functions.
 * I have some BlinkM leds this driver is handling. This is how you read
 * their current color value. No error handling on the val pointer.
 */
#define GET_CURRENT_RGB_COLOR			0x67
static int blinkm_read_rgb(unsigned int device_id, unsigned char *val) 
{
	int result;
	unsigned char buff[4];

	buff[0] = GET_CURRENT_RGB_COLOR;

	result = mi2c_i2c_write(device_id, buff, 1);

	if (result != 1)
		return result;

	buff[0] = 0;
	buff[1] = 0;
	buff[2] = 0;

	result = mi2c_i2c_read(device_id, buff, 3);

	if (result != 3)
		return result;

	val[0] = buff[0];
	val[1] = buff[1];
	val[2] = buff[2];

	return 0;
}

/* 
 * See the arduino_i2c_slave.pde under the arduino_i2c_slave directory
 * for the arduino code. Basically, the arduino is running a simple program
 * where he responds to 1 byte commands with a two-byte response.
 */
static int arduino_run_command(unsigned char cmd, unsigned int *val) 
{
	int result;
	unsigned char buff[2];

	buff[0] = cmd;

	/* We know the Arduino is device_id 2. It's just a demo... */
	result = mi2c_i2c_write(2, buff, 1);

	if (result != 1)
		return result;

	buff[0] = 0;
	buff[1] = 0;

	result = mi2c_i2c_read(2, buff, 2);

	if (result != 2)
		return result;

	*val = (buff[0] << 8) | buff[1];

	return 0;
}

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
	unsigned int i, addr, val;
	unsigned char rgb[4], cmd;

	/* 
	Generic user progs like cat will continue calling until we 
	return zero. So if *offp != 0, we know this is at least the
	second call.
	*/
	if (*offp > 0)
		return 0;

	if (down_interruptible(&mi2c_dev.sem)) 
		return -ERESTARTSYS;

	memset(rgb, 0, sizeof(rgb));
	memset(mi2c_dev.user_buff, 0, USER_BUFF_SIZE);
	len = 0;

	/* This is only for my test devices, a couple of BlinkM leds. */
	for (i = 0; i < 2; i++) {
		if (blinkm_read_rgb(i, rgb) < 0) {
			printk(KERN_ALERT "Read of BlinkM %d failed\n", i);
		}
		else {
			addr = mi2c_i2c_get_address(i);

			len += sprintf(mi2c_dev.user_buff + len, 
				"BlinkM at 0x%02X (r,g,b) %d %d %d\n",
				addr, rgb[0], rgb[1], rgb[2]);
		}
	}		
	/* and one Arduino device */
	addr = mi2c_i2c_get_address(2);
	val = 0;
	cmd = 0x02;

	if (arduino_run_command(cmd, &val) < 0)
		printk(KERN_ALERT "Read of Arduino at 0x%02X failed\n", addr);
	else
		len += sprintf(mi2c_dev.user_buff + len, 
			"Arduino at 0x%02X responded to 0x%02X with 0x%04X\n",
			addr, cmd, val);
		
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

