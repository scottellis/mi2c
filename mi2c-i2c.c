/*
 * The mt9p001 isp camera driver I2C functionality.
 *
 */

#include <linux/init.h>
#define DEBUG
#include <linux/device.h>
#include <linux/i2c.h>

#include "mi2c.h"

struct i2c_client *mi2c_i2c_client;

int mi2c_i2c_write(unsigned char *buf, int count)
{
	if (!mi2c_i2c_client)
		return -ENODEV;

	return i2c_master_send(mi2c_i2c_client, buf, count);
}

int mi2c_i2c_read(unsigned char *buf, int count)
{
	if (!mi2c_i2c_client)
		return -ENODEV;

	return i2c_master_recv(mi2c_i2c_client, buf, count);
}

/*
 * An example helper function you might want. Suppose the device requires
 * us to specify a two-byte address to read a one-byte data.
 * I'm just making up the byte-ordering. 
 */
int mi2c_read_reg(unsigned short reg, unsigned char *val)
{
	int result;
	unsigned char buff[2];

	buff[0] = (reg >> 8) & 0xff;
	buff[1] = reg & 0xff;

	result = mi2c_i2c_write(buff, 2);

	if (result != 2)
		return result;

	buff[0] = 0;

	result = mi2c_i2c_read(buff, 1);

	if (result != 1)
		return result;

	if (val)
		*val = buff[0];

	return 0;
}

/*
 * A similar example helper function pretending that the deivce requires
 * writes to specify a two-byte address followed by one-byte of data.
 */
int mi2c_write_reg(unsigned short reg, unsigned char val)
{
	int result;
	char buff[4];

	buff[0] = (reg >> 8) & 0xff;
	buff[1] = reg & 0xff;
	buff[2] = val;
	
	result = mi2c_i2c_write(buff, 3);

	if (result != 3)
		return result;
	
	return 0;
}

static int __init
mi2c_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	printk(KERN_INFO "mi2c_i2c_probe\n");

	if (mi2c_i2c_client) {
		printk(KERN_WARNING "client already in use\n");
		return -EBUSY;
	}

	printk(KERN_INFO "%s registered for address 0x%x\n", 
		client->name, client->addr);

	mi2c_i2c_client = client;

	return 0;
}

static int __exit
mi2c_i2c_remove(struct i2c_client *client)
{
	printk(KERN_INFO "mi2c_i2c_remove\n");

	mi2c_i2c_client = NULL;

	return 0;
}

static const struct i2c_device_id mi2c_id[] = {
	{ DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, mi2c_id);


static struct i2c_driver mi2c_i2c_driver = {
	.driver = {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.id_table	= mi2c_id,
	.probe		= mi2c_i2c_probe,
	.remove		= mi2c_i2c_remove,
};

int __init mi2c_init_i2c(void)
{
	return i2c_add_driver(&mi2c_i2c_driver);
}

void __exit mi2c_cleanup_i2c(void)
{
	i2c_del_driver(&mi2c_i2c_driver);
}


