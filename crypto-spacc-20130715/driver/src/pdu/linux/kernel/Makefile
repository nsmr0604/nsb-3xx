#
# Proprietary Information of Elliptic Semiconductor
# Copyright (C) 2002-2009, all rights reserved, Elliptic Semiconductor, Inc.
#
# As part of our confidentiality  agreement, Elliptic Semiconductor and the
# Company, as  a  Receiving Party, of  this  information  agrees to keep
# strictly  confidential  all Proprietary Information  so received from
# Elliptic Semiconductor. Such Proprietary Information can be used solely for
# the  purpose  of evaluating  and/or conducting a proposed business
# relationship  or  transaction  between  the  parties.  Each Party  agrees
# that  any  and  all  Proprietary  Information  is  and shall remain
# confidential and the property of Elliptic Semiconductor.  The  Company  may
# not  use  any of  the  Proprietary  Information of Elliptic Semiconductor for
# any purpose other  than  the  above-stated purpose  without the prior written
# consent of Elliptic Semiconductor.
#
# file    : Makefile
# description: This Makefile builds up a kernel module using the spacc interface files
# 						 and optionally includes the record engine interface.
# 						 It may also make up a user space driver if RE_USER_DRIVER is defined.
#

#if KERNELDIR is not defined, set it to the modules directory
KERNELDIR ?= /lib/modules/$(shell uname -r)/build
ifndef RD
RD       := $(shell pwd)
endif

include ${RD}/Makefile.opts

ifdef PDU_MAX_DDT
EXTRA_CFLAGS += -DPDU_MAX_DDT=${PDU_MAX_DDT}
endif

ifdef PDU_DUAL_MPM
EXTRA_CFLAGS += -DPDU_DUAL_MPM
endif

ifdef PDU_BASE_ADDR
EXTRA_CFLAGS += -DPDU_BASE_ADDR=${PDU_BASE_ADDR}
endif

ifdef PDU_BASE_IRQ
EXTRA_CFLAGS += -DPDU_BASE_IRQ=${PDU_BASE_IRQ}
endif

ifdef PDU_ZYNQ_RESET
EXTRA_CFLAGS += -DPDU_ZYNQ_RESET
endif

ifdef PDU_CORTINA_G2_FORCE_3RD_VIRTUAL_INTERFACE_USAGE
EXTRA_CFLAGS += -DPDU_CORTINA_G2_FORCE_3RD_VIRTUAL_INTERFACE_USAGE
endif

EXTRA_CFLAGS += -I${RD}/../include -I${RD}/../../../../common_sdk/include/
EXTRA_CFLAGS += -I${RD}/../../../trng/include
EXTRA_CFLAGS += -I${RD}/../../../saspa/include

#build up the object list depending on options
OBJS-PDU += pdu.o spdu.o ../pdu/pdu_get_version.o
#../../../../common_sdk/common/elputils.o

ifdef PDU_SINGLE_CORE
EXTRA_CFLAGS += -DPDU_SINGLE_CORE
endif
ifdef PDU_SINGLE_TRNG
EXTRA_CFLAGS += -DPDU_SINGLE_TRNG
endif
ifdef PDU_SINGLE_TRNG3
EXTRA_CFLAGS += -DPDU_SINGLE_TRNG3
endif
ifdef PDU_SINGLE_EAPE
EXTRA_CFLAGS += -DPDU_SINGLE_EAPE
endif
ifdef CLP36_ENABLED
EXTRA_CFLAGS += -DCLP36_ENABLED=1
endif
ifdef PDU_SINGLE_PKA
EXTRA_CFLAGS += -DPDU_SINGLE_PKA
endif
ifdef PCI_INDIRECT
EXTRA_CFLAGS += -DPCI_INDIRECT
endif

# creates the actual kernel module name based on input or uses a default name
module-pdu=elppdu
module-pci=elppci
module-mem=elpmem
module-standalone=elppci_standalone

ifdef PDU_USE_PCI
OUTPUT-PCI = ${module-pci}.o ${module-standalone}.o
endif

ifndef PDU_USE_PCI
OUTPUT-MEM = ${module-mem}.o
endif

OUTPUT-PDU = ${module-pdu}.o 
${module-pdu}-objs := $(OBJS-PDU)
ifdef PDU_USE_SPACC
${module-pci}-objs := spacc_pci.o
${module-mem}-objs := spacc_mem.o
else
${module-pci}-objs := saspa_pci.o
${module-mem}-objs := saspa_mem.o
endif

obj-m := ${OUTPUT-PDU} ${OUTPUT-PCI} ${OUTPUT-MEM}

${module-standalone}-objs := standalone_pci.o

all: 
	RD=${RD} CFLAGS="${CFLAGS}" $(MAKE) -C $(KERNELDIR) SUBDIRS=$(RD) modules

clean:
	rm -rf *.o *.ko *.mod.? Module.* modules.* .*.cmd .tmp*
