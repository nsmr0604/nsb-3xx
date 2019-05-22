ifdef PDU_USE_KERNEL
PARTS=module
else
PARTS=lib
endif

all: ${PARTS} usr

lib:
	make -C spacc

module:
	make -C kernel
	cp kernel/*.ko ../../bin/

usr:
	make -C user

clean:
	make -C spacc clean
	make -C kernel clean
	make -C user clean

