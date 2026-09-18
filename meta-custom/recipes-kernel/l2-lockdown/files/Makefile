# OE out-of-tree kernel module build. The `module` bbclass runs `make` here with
# KERNEL_SRC set; the default target hands off to the kernel build (obj-m is in
# Kbuild). Mirrors poky's meta-skeleton hello-mod Makefile.
obj-m := l2_lockdown.o

SRC := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c *.mod
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions
