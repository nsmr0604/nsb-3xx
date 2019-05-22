EXTRA_CFLAGS += -DPDU_USE_KERNEL=1

#if KERNELDIR is not defined, set it to the modules directory
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
RD ?= $(shell pwd)

include ${RD}/../../pdu/${PDU_ARCH}/kernel/Makefile.opts
include ${RD}/Makefile.opts

EXTRA_CFLAGS += -I${RD}/../include -I${RD}/../../pdu/${PDU_ARCH}/include -I${RD}/../../../../common_sdk/include -I${RD}/../../core/include
#export KBUILD_EXTRA_SYMBOLS = ${RD}/../../pdu/kernel/Module.symvers

ifdef PCI_INDIRECT
EXTRA_CFLAGS += -DPCI_INDIRECT
endif

OBJS-PKA += pka.o class.o firmware.o ../pka/elppka.o

module-pka=elppka
OUTPUT-PKA = ${module-pka}.o
${module-pka}-objs := ${OBJS-PKA}
obj-m := ${OUTPUT-PKA}
ccflags-$(CONFIG_PKA_DEBUG) := -DDEBUG

all: 
	RD=${RD} CFLAGS="${CFLAGS}" $(MAKE) -C $(KERNELDIR) SUBDIRS=$(RD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers Module.* modules.*

