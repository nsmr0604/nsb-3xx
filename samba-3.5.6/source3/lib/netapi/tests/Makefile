KRB5LIBS=-L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -Wl,--enable-new-dtags -Wl,-rpath -Wl,/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -Wl,--enable-new-dtags -Wl,-rpath -Wl,/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib -lgssapi_krb5 -lkrb5 -lk5crypto -lcom_err
LDAP_LIBS=-lldap -llber 
LIBS=-lresolv -ldl -liconv -lpthread -lnetapi -ltdb -ltalloc
DEVELOPER_CFLAGS=-g -Wall -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -DDEBUG_PASSWORD -DDEVELOPER -Wstrict-prototypes
CCFLAGS=-I../ -L../../../bin  -I/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/include/krb5 -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts -I/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/include -DHAVE_KRB5_ENCTYPE_TO_STRING_WITH_SIZE_T_ARG -g -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS $(DEVELOPER_CFLAGS) $(FLAGS) -D_SAMBA_BUILD_=3 $(GTK_FLAGS)
CC=arm-openwrt-linux-uclibcgnueabi-gcc
PICFLAG=-fPIC
LDFLAGS= -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -Wl,--enable-new-dtags -Wl,-rpath -Wl,/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib -Wl,--as-needed -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/samba-3.5.6/ -L/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/samba-3.5.6/source4 -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/root-g2/usr/lib -L/home/philby/060816/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib/ -L./bin -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib/libiconv-full/lib
DYNEXP=-Wl,--export-dynamic
NETAPI_LIBS=$(LIBS) $(KRB5LIBS) $(LDAP_LIBS)
CMDLINE_LIBS=$(NETAPI_LIBS) $(POPT_OBJ)

# Compile a source file.
COMPILE_CC = $(CC) -I. $(CCFLAGS) $(PICFLAG) -c $< -o $@
COMPILE = $(COMPILE_CC)

PROGS = bin/netapitest

all: $(PROGS)

MAKEDIR = || exec false; \
	  if test -d "$$dir"; then :; else \
	  echo mkdir "$$dir"; \
	  mkdir -p "$$dir" >/dev/null 2>&1 || \
	  test -d "$$dir" || \
	  mkdir "$$dir" || \
	  exec false; fi || exec false

BINARY_PREREQS = bin/.dummy

bin/.dummy:
	@if (: >> $@ || : > $@) >/dev/null 2>&1; then :; else \
	  dir=bin $(MAKEDIR); fi
	@: >> $@ || : > $@ # what a fancy emoticon!

.c.o:
	@if (: >> $@ || : > $@) >/dev/null 2>&1; then rm -f $@; else \
	 dir=`echo $@ | sed 's,/[^/]*$$,,;s,^$$,.,'` $(MAKEDIR); fi
	@echo Compiling $*.c
	@$(COMPILE) && exit 0;\
		echo "The following command failed:" 1>&2;\
		echo "$(COMPILE_CC)" 1>&2;\
		$(COMPILE_CC) >/dev/null 2>&1

CMDLINE_OBJ = common.o
NETAPIBUFFER_OBJ = netapibuffer.o
NETAPITEST_OBJ = netapitest.o netlocalgroup.o netuser.o netgroup.o netdisplay.o netshare.o netfile.o netserver.o $(CMDLINE_OBJ)

bin/netapitest: $(BINARY_PREREQS) $(NETAPITEST_OBJ)
	@echo Linking $@
	@$(CC) $(CCFLAGS) -o $@ $(NETAPITEST_OBJ) $(LDFLAGS) $(DYNEXP) $(CMDLINE_LIBS)

clean:
	-rm -f $(PROGS)
	-rm -f core */*~ *~ \
		*/*.o */*/*.o */*/*/*.o

