gpcieuni-objs := gpcieuni_drv.o pcieuni_ufn.o pcieuni_probe_exp.o \
	pcieuni_remove_exp.o pcieuni_rw_no_struct_exp.o pcieuni_ioctl_exp.o pcieuni_buffer.o
obj-m := gpcieuni.o

#build for the running kernel
KVERSION = $(shell uname -r)

ccflags-y = -Wall -Wuninitialized

#define the package/module version (the same for this driver)
GPCIEUNI_PACKAGE_VERSION=0.1.5

GPCIEUNI_DKMS_SOURCE_DIR=/usr/src/gpcieuni-${GPCIEUNI_PACKAGE_VERSION}
HEADER_INSTALL_DIR=/usr/local/include/gpcieuni

#The normal compile step for development
all: configure-source-files
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) modules
#copy the symvers file so it survives the dkms cleanup
	cp Module.symvers gpcieuni.symvers

#Performs a dkms install
install: dkms-prepare install-headers
	dkms install -m gpcieuni -v ${GPCIEUNI_PACKAGE_VERSION} -k $(KVERSION)

#Performs a dmks remove
# || true makes it always report success (you would get an error if already uninstalled otheriwse)
uninstall:
	dkms remove -m gpcieuni -v ${GPCIEUNI_PACKAGE_VERSION} -k $(KVERSION) || true

#Compile with debug flag, causes lots of kernel output.
#In addition the driver is compiled with code coverage. It only loads on
#on a kernel with code coverage enabled.
#FIXME: Should both options be separate, so you can get debug messages on a 
#standard kernel?
debug:
	KCPPFLAGS="-DPCIEUNI_DEBUG -fprofile-arcs -ftest-coverage" make all

clean:
	make -C /lib/modules/$(KVERSION)/build V=1 M=$(PWD) clean
	rm -f gpcieuni_drv.c
#do not clean up the gpcieuni.symvers

#Perform a dkms uninstall and remove the headers and
purge: uninstall
	rm -rf ${GPCIEUNI_DKMS_SOURCE_DIR} ${HEADER_INSTALL_DIR}

#This target will only succeed on debian machines with the debian packaging tools installed
debian_package: configure-package-files
	./make_debian_package.sh ${GPCIEUNI_PACKAGE_VERSION}

##### Internal targets usually not called by the user: #####

#A target which replaces the version number in the source files
configure-source-files:
	cat gpcieuni_drv.c.in | sed "{s/@GPCIEUNI_PACKAGE_VERSION@/${GPCIEUNI_PACKAGE_VERSION}/}" > gpcieuni_drv.c

#A target which replaces the version number in the control files for
#dkms and debian packaging
configure-package-files:
	cat dkms.conf.in | sed "{s/@GPCIEUNI_PACKAGE_VERSION@/${GPCIEUNI_PACKAGE_VERSION}/}" > dkms.conf
	test -d debian_from_template || mkdir debian_from_template
	cp dkms.conf debian_from_template/gpcieuni-dkms.dkms
	cp dkms.post_* debian_from_template/
	(cd debian.in; cp compat  control  copyright ../debian_from_template)
	cat debian.in/rules.in | sed "{s/@GPCIEUNI_PACKAGE_VERSION@/${GPCIEUNI_PACKAGE_VERSION}/}" > debian_from_template/rules
	chmod +x debian_from_template/rules

#copies the package sources to the place needed by dkms
dkms-prepare: configure-source-files configure-package-files
	test -d ${GPCIEUNI_DKMS_SOURCE_DIR} || mkdir ${GPCIEUNI_DKMS_SOURCE_DIR}
	cp *.h *.c gpcieuni_drv.c.in Makefile dkms.conf dkms.post_* ${GPCIEUNI_DKMS_SOURCE_DIR}

#install the headers so depedent drivers can find them
install-headers:
	test -d ${HEADER_INSTALL_DIR} || mkdir ${HEADER_INSTALL_DIR}
	cp *.h ${HEADER_INSTALL_DIR}
