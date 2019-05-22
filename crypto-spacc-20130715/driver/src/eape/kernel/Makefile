EXTRA_CFLAGS += -DPDU_USE_KERNEL=1

#if KERNELDIR is not defined, set it to the modules directory
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
RD ?= $(shell pwd)

include ${RD}/../../pdu/${PDU_ARCH}/kernel/Makefile.opts
include ${RD}/Makefile.opts

EXTRA_CFLAGS += -I${RD}/../include -I${RD}/../../pdu/${PDU_ARCH}/include -I${RD}/../../core/include

ifdef PCI_INDIRECT
EXTRA_CFLAGS += -DPCI_INDIRECT
endif

OBJS-eape += eape.o ../eape/eape_build_sa.o ../eape/eape_deinit.o ../eape/eape_go.o \
../eape/eape_init.o ../eape/eape_open.o ../eape/eape_close.o ../eape/eape_done.o ../eape/eape_pop_packets.o

module-eape=elpeape
OUTPUT-eape = ${module-eape}.o
${module-eape}-objs := ${OBJS-eape}
obj-m := ${OUTPUT-eape}

OBJS-EAPEDIAG += eapediag.o
module-eapediag=elpeapediag
OUTPUT-EAPEDIAG = ${module-eapediag}.o
${module-eapediag}-objs := ${OBJS-EAPEDIAG}
obj-m += ${OUTPUT-EAPEDIAG}

OBJS-EAPEOFF += eapeoff.o
module-eapeoff=elpeapeoff
OUTPUT-EAPEOFF = ${module-eapeoff}.o
${module-eapeoff}-objs := ${OBJS-EAPEOFF}
obj-m += ${OUTPUT-EAPEOFF}

obj-m += elpxfrm.o
elpxfrm-objs := xfrm.o

all: 
	RD=${RD} CFLAGS="${CFLAGS}" $(MAKE) -C $(KERNELDIR) SUBDIRS=$(RD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers Module.* modules.*
