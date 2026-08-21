# Out-of-tree build of the PL310 lockdown module.
#
# PL310 is the ARM L2C-310's L2 cache periperhal controller.
#
# Build against the same kernel the rootfs produced,
# cross-compiled for the Zynq-7030.
# Point KDIR at the rootfs kernel build output, e.g.:
#
#   make KDIR=../../rootfs/build/linux \
#        ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-


# run kernal build tree's makefile
KDIR ?= /lib/modules/$(shell uname -r)/build
# build module in my directory
PWD  := $(shell pwd)


# modules: produces the .ko
.PHONY: all clean
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
