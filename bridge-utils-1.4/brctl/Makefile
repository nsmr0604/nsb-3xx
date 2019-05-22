
KERNEL_HEADERS=-I/home/igarashi/cortina/build_dir/linux-g2_wan/linux-3.4

CC=arm-openwrt-linux-uclibcgnueabi-gcc
CFLAGS= -Wall -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts 
LDFLAGS=-L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib 
INCLUDE=-I../libbridge $(KERNEL_HEADERS) 
LIBS= -L ../libbridge -lbridge 

prefix=/usr
exec_prefix=/usr
bindir=/usr/bin
sbindir=/usr/sbin
mandir=/usr/man

INSTALL=/usr/bin/install -c


common_SOURCES= brctl_cmd.c brctl_disp.c 
brctl_SOURCES=  brctl.c $(common_SOURCES)

common_OBJECTS= $(common_SOURCES:.c=.o)
brctl_OBJECTS= $(brctl_SOURCES:.c=.o)

OBJECTS= $(common_OBJECTS) $(brctl_OBJECTS)

PROGRAMS= brctl 


all:	$(PROGRAMS)

install: $(PROGRAMS)
	mkdir -p $(DESTDIR)$(sbindir)
	$(INSTALL) -m 755 $(PROGRAMS) $(DESTDIR)$(sbindir)

brctl:	$(brctl_OBJECTS) ../libbridge/libbridge.a
	$(CC) $(LDFLAGS) $(brctl_OBJECTS) $(LIBS) -o brctl

%.o: %.c brctl.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< 

clean:
	rm -f *.o brctl core

