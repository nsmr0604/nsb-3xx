EXTRA_CFLAGS += -DPDU_USE_KERNEL=1

#if KERNELDIR is not defined, set it to the modules directory
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
RD ?= $(shell pwd)

EXTRA_CFLAGS += -I${RD}/../../../include -I${RD}/../../../../pdu/include -I${RD}/../../../../core/include -I${RD}/../../../../../src/include

OBJS-TRNG_PLATFORM += testfta_elptrng.o 

module-trng=elptrng_test_fta
OUTPUT-TRNG_PLATFORM = ${module-trng}.o
${module-trng}-objs := ${OBJS-TRNG_PLATFORM}
obj-m := ${OUTPUT-TRNG_PLATFORM}

all: 
	RD=${RD} CFLAGS="${CFLAGS}" $(MAKE) -C $(KERNELDIR) SUBDIRS=$(RD) modules

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers Module.* modules.*

