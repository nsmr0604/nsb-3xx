CFLAGS += -Wall -I../include/

libpdu.a: pdu.o pdu_get_version.o
	ar rs $@ $^
	cp $@ ../../../../bin/

clean:
	rm -f *.a *.o
