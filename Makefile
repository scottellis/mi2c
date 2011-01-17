# Makefile for the mi2c driver

DRIVERNAME=mi2c

ifneq ($(KERNELRELEASE),)
	obj-m += mi2c.o
	mi2c-objs := mi2c-char.o mi2c-i2c.o  
else
    PWD := $(shell pwd)

default:
ifeq ($(strip $(KERNELDIR)),)
	$(error "KERNELDIR is undefined!")
else
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules 
endif


install:
	sudo cp mi2c.ko /exports/overo/home/root

clean:
	rm -rf *~ *.ko *.o *.mod.c modules.order Module.symvers .${DRIVERNAME}* .tmp_versions

endif

