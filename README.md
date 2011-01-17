 mi2c kernel module
=======

This is a demo driver to augment the I2C documentation on my website. I still
need to finish the i2c driver segment on the website. This is the code I will
be referencing though.

mi2c is a character device driver that demonstrates how you can use i2c within
another driver for multiple devices of different types.

Instead of statically declaring i2c devices in the kernel board files, this 
driver dynamically register devices on module load avoiding any kernel rebuilds
at least on stock Overo kernels. 

The driver declares the ability to handle two types of devices, "blinkm" and
"arduino" devices.

For testing, I have two BlinkM leds at addresses 0x01 and 0x03 and one Arduino
at address 0x10. The Overo was a Tide.

If you want duplicate the test environment (don't know why you would) you can 
use my blinkm project on github to configure the BlinkM's and the included
Arduino pde under the arduino_i2c_slave_pde directory in this project for the
Arduino slave code.

There is a bunch of printk's in the driver to see what's going on.

Here is a sample session.

	root@overo:~# insmod mi2c.ko 
	[ 1771.114501] mi2c_init()
	[ 1771.117645] mi2c_i2c_probe
	[ 1771.120391] blinkm driver registered for device at address 0x01
	[ 1771.126495] mi2c_i2c_probe
	[ 1771.129211] blinkm driver registered for device at address 0x03
	[ 1771.135314] mi2c_i2c_probe
	[ 1771.138031] arduino driver registered for device at address 0x10


If you do a read of the mi2c device, you'll get some simple output
	root@overo:~# cat /dev/mi2c 
	BlinkM at 0x01 (r,g,b) 240 120 40
	BlinkM at 0x03 (r,g,b) 50 70 200
	Arduino at 0x10 responded to 0x02 with 0xBEEF


And then unloading the module

	root@overo:~# rmmod mi2c
	[ 1802.070312] mi2c_exit()
	[ 1802.072875] mi2c_i2c_remove
	[ 1802.075805] mi2c_i2c_remove
	[ 1802.078796] mi2c_i2c_remove

