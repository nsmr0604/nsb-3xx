#
# Copyright (c) 2011 Elliptic Technologies Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

CC = gcc
LD = $(CC)
CFLAGS = -g -O2 -Wall
AR = ar
RANLIB = ranlib

DIAG_CPPFLAGS = -DCLUE_PRECOMPUTE -DDO_RSA -DDO_ECC -DDO_CLUE_EC -DDO_CLUE_ALL

PKA_CPPFLAGS = -I../include -I. $(DIAG_CPPFLAGS)
BINS = pka-opdump pka-dumbtest pka-abort pka-diag pka-simple521 pka-eccblind

LIBS = libpkauser.a

all: $(BINS)
clean:
	rm -f *.o diag/*.o $(LIBS) $(BINS)

$(BINS): $(LIBS)
	$(LD) -o $@ $^ $(LIBS)

$(LIBS):
	$(AR) rc $@ $^
	$(RANLIB) $@

libpkauser.a: pkadev.o
pka-opdump: opdump.o pka-tool-common.o
pka-dumbtest: dumbtest.o pka-tool-common.o
pka-simple521: simple521.o pka-tool-common.o
pka-eccblind: eccblind.o pka-tool-common.o
pka-abort: abort.o pka-tool-common.o
pka-diag: diag/cluediag.o diag/elpecc.o diag/elprsa.o

.c.o:
	$(CC) -o $@ -c $(CPPFLAGS) $(PKA_CPPFLAGS) $(CFLAGS) $<

.PHONY: all clean
