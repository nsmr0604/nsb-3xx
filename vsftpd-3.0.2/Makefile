# Makefile for systems with GNU tools
CC 	=	gcc
INSTALL	=	install
IFLAGS  = -idirafter dummyinc
#CFLAGS = -g
CFLAGS	+=	-O2 -fPIE -fstack-protector --param=ssp-buffer-size=4 \
	-Wall -W -Wshadow -Werror -Wformat-security \
	-D_FORTIFY_SOURCE=2 \
	#-pedantic -Wconversion

LIBS	=	-lcrypt -lnsl -lssl -lcrypto
LINK	=
LDFLAGS	+=	-fPIE -pie -Wl,-z,relro -Wl,-z,now

OBJS	=	main.o utility.o prelogin.o ftpcmdio.o postlogin.o privsock.o \
		tunables.o ftpdataio.o secbuf.o ls.o \
		postprivparent.o logging.o str.o netstr.o sysstr.o strlist.o \
    banner.o filestr.o parseconf.o secutil.o \
    ascii.o oneprocess.o twoprocess.o privops.o standalone.o hash.o \
    tcpwrap.o ipaddrparse.o access.o features.o readwrite.o opts.o \
    ssl.o sslslave.o ptracesandbox.o ftppolicy.o sysutil.o sysdeputil.o \
    seccompsandbox.o


DESTDIR =

.c.o:
	$(CC) -c $*.c $(CFLAGS) $(IFLAGS) $(EXTRA_CFLAGS)

vsftpd: $(OBJS) 
	$(CC) -o vsftpd $(OBJS) $(LINK) $(LDFLAGS) $(LIBS)

install:
	mkdir -p $(DESTDIR)/usr/sbin
	$(INSTALL) -m 755 vsftpd $(DESTDIR)/usr/sbin/
	mkdir -p $(DESTDIR)/usr/share/man/man8
	$(INSTALL) -m 644 vsftpd.8 $(DESTDIR)/usr/share/man/man8/
	mkdir -p $(DESTDIR)/usr/share/man/man5
	$(INSTALL) -m 644 vsftpd.conf.5 $(DESTDIR)/usr/share/man/man5/
	mkdir -p $(DESTDIR)/etc/xinetd.d
	$(INSTALL) -m 644 xinetd.d/vsftpd $(DESTDIR)/etc/xinetd.d/

uninstall:
	rm -f $(DESTDIR)/usr/sbin/vsftpd
	rm -f $(DESTDIR)/usr/share/man/man8/vsftpd.8
	rm -f $(DESTDIR)/usr/share/man/man5/vsftpd.conf.5
	rm -f $(DESTDIR)/etc/xinetd.d/vsftpd

clean:
	rm -f *.o *.swp vsftpd

