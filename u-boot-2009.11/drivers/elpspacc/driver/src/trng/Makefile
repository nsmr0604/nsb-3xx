ifdef PDU_USE_KERNEL
PARTS=module
else
PARTS=lib
endif

PARTS_TEST = 

ifdef PDU_TEST_LEVEL
ifeq ($(PDU_TEST_LEVEL),2)
PARTS_TEST += module_test_crypto_api module_fta_test
else ifeq ($(PDU_TEST_LEVEL),1)
PARTS_TEST += module_test_crypto_api
endif
endif

all: ${PARTS} ${PARTS_TEST}

lib:
	make -C trng

module:
	make -C kernel
	cp kernel/*.ko ../../bin/
	cp kernel/*.symvers test/
	
module_test_crypto_api:	
	make -C test
	cp test/*.ko ../../bin/	

module_fta_test:	
	make -C test/private/fta
	cp test/private/fta/*.ko ../../bin/	


clean:
	make -C trng clean
	make -C kernel clean
	make -C test clean
	make -C test/private/fta clean

