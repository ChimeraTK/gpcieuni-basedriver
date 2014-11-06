gpcieuni-objs := gpcieuni_drv.o pcieuni_ufn.o pcieuni_probe_exp.o \
	pcieuni_remove_exp.o pcieuni_rw_no_struct_exp.o pcieuni_ioctl_exp.o pcieuni_buffer.o
obj-m := gpcieuni.o 

KVERSION = $(shell uname -r)
all:
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules

install: all
	test -d /lib/modules/$(KVERSION)/gpcieuni || sudo mkdir -p /lib/modules/$(KVERSION)/gpcieuni
	sudo cp -f $(PWD)/Module.symvers /lib/modules/$(KVERSION)/gpcieuni/Gpcieuni.symvers
	cp -f $(PWD)/Module.symvers $(PWD)/Gpcieuni.symvers
	test -d /usr/local/include/gpcieuni || sudo mkdir -p /usr/local/include/gpcieuni
	sudo cp -f $(PWD)/pcieuni_io.h /usr/local/include/gpcieuni/pcieuni_io.h
	sudo cp -f $(PWD)/pcieuni_ufn.h /usr/local/include/gpcieuni/pcieuni_ufn.h
	sudo cp -f $(PWD)/pcieuni_buffer.h /usr/local/include/gpcieuni/pcieuni_buffer.h

debug:
	KCPPFLAGS="-DPCIEUNI_DEBUG" make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
	test -d /lib/modules/$(KVERSION)/gpcieuni || sudo mkdir -p /lib/modules/$(KVERSION)/gpcieuni
	sudo cp -f $(PWD)/Module.symvers /lib/modules/$(KVERSION)/gpcieuni/Gpcieuni.symvers
	cp -f $(PWD)/Module.symvers $(PWD)/Gpcieuni.symvers
	test -d /usr/local/include/gpcieuni || sudo mkdir -p /usr/local/include/gpcieuni
	sudo cp -f $(PWD)/pcieuni_io.h /usr/local/include/gpcieuni/pcieuni_io.h
	sudo cp -f $(PWD)/pcieuni_ufn.h /usr/local/include/gpcieuni/pcieuni_ufn.h
	sudo cp -f $(PWD)/pcieuni_buffer.h /usr/local/include/gpcieuni/pcieuni_buffer.h

clean:
	test ! -d /lib/modules/$(KVERSION) || make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean
	rm -rf Gpcieuni.symvers