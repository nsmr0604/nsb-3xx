CFLAGS=-Wall -I../include -I../../pdu/${PDU_ARCH}/include/
OBJECTS=trng.o

libtrng.a: ${OBJECTS}
	ar rs $@ $^
	cp $@ ../../../bin/

clean:
	rm -f *.o .*.cmd *.a

