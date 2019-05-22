


#-DCFG_FRONTEND_DEV -DCFG_DEMUX_DEV -DSYSTEM_TYPE_DVB
CFLAGS = -Wall -g -O3 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
     -DSYSTEM_TYPE_ATSC -DG2_SPEC_DEMUX\
	 -I$(STAGING_DIR)/usr/include \
	 -I$(STAGING_DIR)/usr/include/FLAC \
	 -I$(STAGING_DIR)/usr/include/libavcodec \
	 -I$(STAGING_DIR)/usr/include/libavformat \
	 -I$(STAGING_DIR)/usr/include/libavutil \
	 -I$(STAGING_DIR)/usr/include/libexif \
	 -I$(STAGING_DIR)/usr/include/uuid \
	 -I$(STAGING_DIR)/usr/include/vorbis \
	 -I$(STAGING_DIR)/usr/lib/libiconv/include \
	 -I$(STAGING_DIR)/usr/lib/libintl/include
LDFLAGS = -L$(STAGING_DIR)/usr/lib \
          -L$(STAGING_DIR)/usr/lib/libintl/lib \
          -L$(STAGING_DIR)/usr/lib/libiconv/lib \
          -Wl,-rpath=$(STAGING_DIR)/usr/lib \
          -Wl,-rpath-link=$(STAGING_DIR)/usr/lib

CC = gcc
RM = rm -f
INSTALL = install

INSTALLPREFIX ?= $(PREFIX)/usr
SBININSTALLDIR = $(INSTALLPREFIX)/sbin
ETCINSTALLDIR = $(PREFIX)/etc

#BASEOBJS = parsetab.o cs_ts.o crc32.o ts2pesdescr.o tidlist.o chinfo.o scan.o dump-vdr.o lnb.o atsc_psip_section.o dump-zap.o section.o diseqc.o
BASEOBJS = scan.o dump-vdr.o lnb.o atsc_psip_section.o dump-zap.o section.o diseqc.o log.o ts_driver.o lnb.o

ALLOBJS = $(BASEOBJS)

CORTINA_CSTS=libcsts.a


.PHONY:	all clean install depend

all:	$(CORTINA_CSTS)
#all:	$(CORTINA_CSTS)

clean:
	$(RM) $(ALLOBJS)
	$(RM) $(CORTINA_CSTS)

install:	$(CORTINA_CSTS)
	$(INSTALL) -d $(SBININSTALLDIR)
	$(INSTALL) minidlna $(SBININSTALLDIR)
	$(INSTALL) -d $(ETCINSTALLDIR)


$(CORTINA_CSTS):	$(BASEOBJS)
	@echo Linking $@
	ar rcs $@ $^
	ranlib $@

#tsmain: tsmain.o libcsts.a
#	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ tsmain.o libcsts.a
 
# DO NOT DELETE

##cortina ts APIs
#cs_ts.o: debug.h list.h linux/dvb/dmx.h linux/dvb/frontend.h
#crc32.o: crc32.h
#ts2pesdescr.o: ts2pesdescr.h

scan.o: scan.h log.h

.SUFFIXES: .c .o

.c.o:
	@echo Compiling $*.c
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< && exit 0;\
		echo "The following command failed:" 1>&2;\
		echo "$(CC) $(CFLAGS) -o $@ -c $<";\
		$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $< 
