#
# This program may be distributed according to the terms of the GNU
# General Public License, version 2 or (at your option) any later version.
#
# $Id: 0570-ipsec-optimaztion.patch,v 1.1 2013/11/05 10:27:44 jflee Exp $
#***********************************************************************

DESTDIR = $(INSTROOT)/usr
LIBDIR = $(DESTDIR)/lib/pppd/$(PPPDVERSION)

PPPDVERSION = $(shell awk -F '"' '/VERSION/ { print $$2; }' ../../patchlevel.h)

INSTALL	= install

COPTS=-O2 -g
CFLAGS  = $(COPTS) -I. -I../.. -I../../../include -fPIC -DPPPD_VERSION=\"$(PPPDVERSION)\"
all: pptp.so

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

pptp.so: dirutil.o orckit_quirks.o pptp.o pptp_callmgr.o pptp_ctrl.o pptp_quirks.o util.o vector.o
	$(CC) -o pptp.so -shared dirutil.o orckit_quirks.o pptp.o pptp_callmgr.o pptp_ctrl.o pptp_quirks.o util.o vector.o

install: all
	$(INSTALL) -d -m 755 $(LIBDIR)
	$(INSTALL) -c -m 4550 pptp.so $(LIBDIR)

clean:
	rm -f *.o *.so

