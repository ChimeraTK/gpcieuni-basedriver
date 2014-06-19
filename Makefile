upciedev-objs := upciedev_drv.o pciedev_ufn.o pciedev_probe_exp.o \
	pciedev_remove_exp.o pciedev_rw_exp.o pciedev_ioctl_exp.o pciedev_buffer.o
obj-m := upciedev.o 

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
	test -d /lib/modules/$(KVERSION)/upciedev || sudo mkdir -p /lib/modules/$(KVERSION)/upciedev
	sudo cp -f $(PWD)/Module.symvers /lib/modules/$(KVERSION)/upciedev/Upciedev.symvers
	cp -f $(PWD)/Module.symvers $(PWD)/Upciedev.symvers
	test -d /usr/local/include/upciedev || sudo mkdir -p /usr/local/include/upciedev
	sudo cp -f $(PWD)/pciedev_io.h /usr/local/include/upciedev/pciedev_io.h
	sudo cp -f $(PWD)/pciedev_ufn.h /usr/local/include/upciedev/pciedev_ufn.h
	sudo cp -f $(PWD)/pciedev_buffer.h /usr/local/include/upciedev/pciedev_buffer.h
debug:
	KCPPFLAGS="-DPCIEDEV_DEBUG" make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
	test -d /lib/modules/$(KVERSION)/upciedev || sudo mkdir -p /lib/modules/$(KVERSION)/upciedev
	sudo cp -f $(PWD)/Module.symvers /lib/modules/$(KVERSION)/upciedev/Upciedev.symvers
	cp -f $(PWD)/Module.symvers $(PWD)/Upciedev.symvers
	test -d /usr/local/include/upciedev || sudo mkdir -p /usr/local/include/upciedev
	sudo cp -f $(PWD)/pciedev_io.h /usr/local/include/upciedev/pciedev_io.h
	sudo cp -f $(PWD)/pciedev_ufn.h /usr/local/include/upciedev/pciedev_ufn.h
	sudo cp -f $(PWD)/pciedev_buffer.h /usr/local/include/upciedev/pciedev_buffer.h

clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean

