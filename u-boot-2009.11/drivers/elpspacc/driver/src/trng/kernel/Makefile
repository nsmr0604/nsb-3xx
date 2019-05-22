EXTRA_CFLAGS += -DPDU_USE_KERNEL=1

#if KERNELDIR is not defined, set it to the modules directory
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
RD ?= $(shell pwd)

include ${RD}/../../pdu/${PDU_ARCH}/kernel/Makefile.opts
include ${RD}/Makefile.opts

ifdef TRNG_DEBUG_API
EXTRA_CFLAGS += -DELPTRNG_DEBUG_API
endif

ifdef TRNG_HW_RANDOM
EXTRA_CFLAGS += -DELPTRNG_HWRANDOM
endif

ifdef PCI_INDIRECT
EXTRA_CFLAGS += -DPCI_INDIRECT=1
endif

ifdef TRNG_CRYPTO_API
EXTRA_CFLAGS += -DELPTRNG_CRYPTO_API
endif

EXTRA_CFLAGS += -I${RD}/../include -I${RD}/../../pdu/${PDU_ARCH}/include -I${RD}/../../core/include -I${RD}/../../include

OBJS-TRNG_PLATFORM = ../trng/trng.o
OBJS-TRNG_PLATFORM += elptrng_driver.o 

module-trng=elptrng
OUTPUT-TRNG_PLATFORM = ${module-trng}.o
${module-trng}-objs := ${OBJS-TRNG_PLATFORM}
obj-m := ${OUTPUT-TRNG_PLATFORM}

obj-m += elptrng3.o
elptrng3-objs := trng3.o trng3_debug.o ../trng/trng3.o

# obj-m += elpclp890.o
# elpclp890-objs := clp890.o ../trng/clp890.o

all: 
	RD=${RD} CFLAGS="${CFLAGS}" $(MAKE) -C $(KERNELDIR) SUBDIRS=$(RD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers Module.* modules.*

