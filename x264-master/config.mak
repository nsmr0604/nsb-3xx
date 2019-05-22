SRCPATH=.
prefix=/usr
exec_prefix=/usr
bindir=/usr/bin
libdir=${exec_prefix}/lib
includedir=${prefix}/include
ARCH=ARM
SYS=LINUX
CC=arm-openwrt-linux-uclibcgnueabi-gcc
CFLAGS=-Wshadow -O3 -fno-fast-math -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts  -Wall -I. -I$(SRCPATH) -std=gnu99 -fPIC -fomit-frame-pointer -fno-tree-vectorize
DEPMM=-MM -g0
DEPMT=-MT
LD=arm-openwrt-linux-uclibcgnueabi-gcc -o 
LDFLAGS=-L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib  -lm -lpthread
LIBX264=libx264.a
AR=arm-openwrt-linux-uclibcgnueabi-ar rc 
RANLIB=arm-openwrt-linux-uclibcgnueabi-ranlib
STRIP=arm-openwrt-linux-uclibcgnueabi-strip
AS=arm-openwrt-linux-uclibcgnueabi-gcc -c -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts
ASFLAGS= -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts  -Wall -I. -I$(SRCPATH) -std=gnu99 -c -DPIC -DHIGH_BIT_DEPTH=0 -DBIT_DEPTH=8
RC=
RCFLAGS=
EXE=
HAVE_GETOPT_LONG=1
DEVNULL=/dev/null
PROF_GEN_CC=-fprofile-generate
PROF_GEN_LD=-fprofile-generate
PROF_USE_CC=-fprofile-use
PROF_USE_LD=-fprofile-use
default: cli
install: install-cli
LDFLAGSCLI = 
CLI_LIBX264 = $(LIBX264)
