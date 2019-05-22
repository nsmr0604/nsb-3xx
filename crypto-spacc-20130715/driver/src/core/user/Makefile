#
# Proprietary Information of Elliptic Semiconductor
# Copyright (C) 2003-2007, all rights reserved, Elliptic Semiconductor, Inc.
#
# As part of our confidentiality  agreement, Elliptic Semiconductor and the
# Company, as  a  Receiving Party, of  this  information  agrees to keep
# strictly  confidential  all Proprietary Information  so received from
# Elliptic Semiconductor. Such Proprietary Information can be used solely for
# the  purpose  of evaluating  and/or conducting a proposed business
# relationship  or  transaction  between  the  parties.  Each Party  agrees
# that  any  and  all  Proprietary  Information  is  and shall remain
# confidential and the property of Elliptic Semiconductor.  The  Company  may
# not  use  any of  the  Proprietary  Information of Elliptic Semiconductor for
# any purpose other  than  the  above-stated purpose  without the prior written
# consent of Elliptic Semiconductor.
#
# file    : Makefile
#
ARCH = bare

ELPCRYPTO_ROOT=../..

#include ${ELPCRYPTO_ROOT}/Makefile.rules

SINCLUDE = -I../../include  -I../include -I../../pdu/${ARCH}/include
BINS = ../../../bin
LIBS = ../../../lib

CFLAGS +=  $(SINCLUDE) -g3


SPACCDIAGOBJS = spaccdev.o
SPACCDIAG=${BINS}/spaccdev

SPACCVECTOROBJS = spaccdevvector.o
SPACCVECTOR=${BINS}/spaccdevvector

SPACCDEVOFFOBJS = spaccdevoff.o
SPACCDEVOFF=${BINS}/spaccdevoff

SPACCIRQOBJS = spaccirq.o
SPACCIRQ=${BINS}/spaccirq
CC=$(CROSS_COMPILE)gcc

OBJS = ${SPACCDIAGOBJS} ${SPACCVECTOROBJS} ${SPACCDEVOFFOBJS} ${SPACCIRQOBJS}

all: clean_obj ${OBJS}  ${SPACCDIAG} ${SPACCVECTOR} ${SPACCDEVOFF} ${SPACCIRQ}

clean_obj:
	rm -f ${OBJS} ${SPACCDIAG} ${SPACCVECTOR} ${SPACCDEVOFF} ${SPACCIRQ}

clean:
	rm -f $(SPACCDIAG) *.o *.core core *.bak *~

${SPACCDIAG}: spaccdevtest.o
	$(CC)  -o ${SPACCDIAG}  $(SPACCDIAGOBJS) $^ -lpthread

${SPACCDEVOFF}: spaccdevoff.o
	$(CC)  -o ${SPACCDEVOFF}  $(SPACCDIAGOBJS) $^ -lpthread

${SPACCVECTOR}: ${SPACCVECTOROBJS}
	$(CC)  -o ${SPACCVECTOR}  $(SPACCDIAGOBJS) $^

${SPACCIRQ}: ${SPACCIRQOBJS}
	$(CC) -o ${SPACCIRQ} $^

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<



