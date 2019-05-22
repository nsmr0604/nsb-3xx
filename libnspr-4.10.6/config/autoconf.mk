# -*- Mode: Makefile -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.


INCLUDED_AUTOCONF_MK = 1
USE_AUTOCONF	= 1

MOZILLA_CLIENT	= 

prefix		= /usr
exec_prefix	= /usr
bindir		= /usr/bin
includedir	= ${prefix}/include/nspr
libdir		= ${exec_prefix}/lib
datarootdir	= ${prefix}/share
datadir		= /usr/share

dist_prefix	= ${MOD_DEPTH}/dist
dist_bindir	= ${dist_prefix}/bin
dist_includedir = ${dist_prefix}/include/nspr
dist_libdir	= ${dist_prefix}/lib

DIST		= $(dist_prefix)

RELEASE_OBJDIR_NAME = Linux._arm_glibc_PTH_OPT.OBJ
OBJDIR_NAME	= .
OBJDIR		= $(OBJDIR_NAME)
# We do magic with OBJ_SUFFIX in config.mk, the following ensures we don't
# manually use it before config.mk inclusion
OBJ_SUFFIX	= $(error config/config.mk needs to be included before using OBJ_SUFFIX)
_OBJ_SUFFIX	= o
LIB_SUFFIX	= a
DLL_SUFFIX	= so
ASM_SUFFIX	= s
MOD_NAME	= nspr20

MOD_MAJOR_VERSION = 4
MOD_MINOR_VERSION = 10
MOD_PATCH_VERSION = 6

LIBNSPR		= -L$(dist_libdir) -lnspr$(MOD_MAJOR_VERSION)
LIBPLC		= -L$(dist_libdir) -lplc$(MOD_MAJOR_VERSION)

CROSS_COMPILE	= 1
MOZ_OPTIMIZE	= 1
MOZ_DEBUG	= 
MOZ_DEBUG_SYMBOLS = 

USE_CPLUS	= 
USE_IPV6	= 
USE_N32		= 
USE_X32		= 
USE_64		= 
ENABLE_STRIP	= 

USE_PTHREADS	= 1
USE_BTHREADS	= 
PTHREADS_USER	= 
CLASSIC_NSPR	= 

AS		= $(CC)
ASFLAGS		= $(CFLAGS)
CC		= arm-openwrt-linux-uclibcgnueabi-gcc
CCC		= arm-openwrt-linux-uclibcgnueabi-g++
NS_USE_GCC	= 1
GCC_USE_GNU_LD	= 
MSC_VER		= 
AR		= /usr/bin/ar
AR_FLAGS	= cr $@
LD		= /usr/bin/ld
RANLIB		= arm-openwrt-linux-uclibcgnueabi-ranlib
PERL		= /usr/bin/perl
RC		= 
RCFLAGS		= 
STRIP		= /usr/bin/strip
NSINSTALL	= $(MOD_DEPTH)/config/$(OBJDIR_NAME)/nsinstall
FILTER		= 
IMPLIB		= 
CYGWIN_WRAPPER	= 
MT		= 

OS_CPPFLAGS	= -I/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/include -I/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/include -I/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/include -I/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/include 
OS_CFLAGS	= $(OS_CPPFLAGS) -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts  -Wall -pthread -O2 $(DSO_CFLAGS)
OS_CXXFLAGS	= $(OS_CPPFLAGS) -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts  -Wall -pthread -O2 $(DSO_CFLAGS)
OS_LIBS         = -lpthread -ldl 
OS_LDFLAGS	= -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/target-arm_uClibc-0.9.32_eabi/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/usr/lib -L/home/igarashi/cortina/staging_dir/toolchain-arm_gcc-4.5.1+l_uClibc-0.9.32_eabi/lib 
OS_DLLFLAGS	= 
DLLFLAGS	= 
EXEFLAGS  = 
OPTIMIZER	= 

PROFILE_GEN_CFLAGS  = -fprofile-generate
PROFILE_GEN_LDFLAGS = -fprofile-generate
PROFILE_USE_CFLAGS  = -fprofile-use -fprofile-correction -Wcoverage-mismatch
PROFILE_USE_LDFLAGS = -fprofile-use

MKSHLIB		= $(CC) $(DSO_LDOPTS) -o $@
WRAP_LDFLAGS	= 
DSO_CFLAGS	= -fPIC
DSO_LDOPTS	= -shared -Wl,-soname -Wl,$(notdir $@)

RESOLVE_LINK_SYMBOLS = 

HOST_CC		= arm-openwrt-linux-uclibcgnueabi-gcc
HOST_CFLAGS	= -O -pipe -march=armv7-a -mtune=cortex-a9 -funit-at-a-time -mfpu=neon -ftree-vectorize -mfloat-abi=softfp -fhonour-copts  -DXP_UNIX
HOST_LDFLAGS	= 

DEFINES		=  -UDEBUG -DPACKAGE_NAME=\"\" -DPACKAGE_TARNAME=\"\" -DPACKAGE_VERSION=\"\" -DPACKAGE_STRING=\"\" -DPACKAGE_BUGREPORT=\"\" -DPACKAGE_URL=\"\" -DNDEBUG=1 -DHAVE_VISIBILITY_HIDDEN_ATTRIBUTE=1 -DHAVE_VISIBILITY_PRAGMA=1 -DXP_UNIX=1 -D_GNU_SOURCE=1 -DHAVE_FCNTL_FILE_LOCKING=1 -DLINUX=1 -DHAVE_DLADDR=1 -DHAVE_LCHOWN=1 -DHAVE_SETPRIORITY=1 -DHAVE_STRERROR=1 -DHAVE_SYSCALL=1 -D_REENTRANT=1

MDCPUCFG_H	= _linux.cfg
PR_MD_CSRCS	= linux.c
PR_MD_ASFILES	= 
PR_MD_ARCH_DIR	= unix
CPU_ARCH	= arm

OS_TARGET	= Linux
OS_ARCH		= Linux
OS_RELEASE	= .
OS_TEST		= arm

NOSUCHFILE	= /no-such-file
AIX_LINK_OPTS	= 
MOZ_OBJFORMAT	= 
ULTRASPARC_LIBRARY = 

OBJECT_MODE	= 
ifdef OBJECT_MODE
export OBJECT_MODE
endif

VISIBILITY_FLAGS = -fvisibility=hidden
WRAP_SYSTEM_INCLUDES = 

MACOSX_DEPLOYMENT_TARGET = 
ifdef MACOSX_DEPLOYMENT_TARGET
export MACOSX_DEPLOYMENT_TARGET
endif

MACOS_SDK_DIR	= 

SYMBIAN_SDK_DIR = 

NEXT_ROOT	= 
ifdef NEXT_ROOT
export NEXT_ROOT
endif
