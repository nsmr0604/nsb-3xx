
KERNEL_HEADERS=-I/home/igarashi/cortina/build_dir/linux-g2_wan/linux-3.4

AR=ar
RANLIB=arm-openwrt-linux-uclibcgnueabi-ranlib

CC=arm-openwrt-linux-uclibcgnueabi-gcc
CFLAGS = -Wall -g -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts  $(KERNEL_HEADERS)

prefix=/usr
exec_prefix=/usr
includedir=${prefix}/include
libdir=${exec_prefix}/lib

libbridge_SOURCES= \
	libbridge_devif.c \
	libbridge_if.c \
	libbridge_init.c \
	libbridge_misc.c 

libbridge_OBJECTS=$(libbridge_SOURCES:.c=.o)

all:	libbridge.a

install: libbridge.a
	mkdir -p $(DESTDIR)$(includedir)
	install -m 644 libbridge.h $(DESTDIR)$(includedir)
	mkdir -p $(DESTDIR)$(libdir)
	install -m 644 libbridge.a $(DESTDIR)$(libdir)


clean:
	rm -f *.o libbridge.a

libbridge.a:	$(libbridge_OBJECTS)
	$(AR) rcs $@ $(libbridge_OBJECTS)
	$(RANLIB) $@

%.o: %.c libbridge.h libbridge_private.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $<

libbridge_compat.o:	libbridge_compat.c if_index.c
	$(CC) $(CFLAGS) -c libbridge_compat.c

