# This Makefile was original distributed by Elliptic to contain no content. 
# I has been expanded to support Yocto build system which define KERNEL_SRC to point to location of Linux kernel source and expects the 
# modules_install rule to bundle and install the Linux loadable kernel modules.

include $(TOPDIR)/config.mk

#LIB     = libspacc.a
#LIB     = libpdu.a

# Yocto build system support START REGION 
all: 
	$(MAKE) PDU_ARCH=bare -C driver

SRC := $(shell pwd)

modules_install:
	$(MAKE) INSTALL_MOD_DIR=kernel/crypto/elpspacc -C $(KERNEL_SRC) M=$(SRC)/driver/src/core/kernel modules_install
	$(MAKE) INSTALL_MOD_DIR=kernel/crypto/elpspacc -C $(KERNEL_SRC) M=$(SRC)/driver/src/pdu/linux/kernel modules_install

# Yocto build system support END REGION 
	
