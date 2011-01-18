 mi2c kernel module
=======

This is a demo driver to augment the I2C documentation on my website. I still
need to finish the writeup, but this is the code I will be referencing.

mi2c demonstrates how you can use i2c within another driver handling devices
of different types. It exposes a char dev interface for testing.

The driver registers the devices it wants to handle when it loads. This way
kernel source modifications are not necessary. 

The driver declares the ability to handle two types of devices, "blinkm" and
"arduino" devices.

For testing, I have two BlinkM leds at addresses 0x01 and 0x03 and one Arduino
at address 0x10.

If you want duplicate the test environment you will need to change the default
address of 0x09 for at least one of the two BlinkMs. You could use my blinkm 
project on github for this. 

For the Arduino, there is a pde in the project directory to run the Arduino 
slave code. The Arduino uses the analog pins 4 (sda) and 5 (scl).

The BlinkM and Arduino are 5V, so you need level conversion from the Overo 1.8V.

I left a bunch of printk's in the driver so you can see what's going on.

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


If you wanted to make this more generic, you might want to pass the 
i2c_board_info struct to mi2c_init_i2c() since the i2c driver part of the
code doesn't really care. 

