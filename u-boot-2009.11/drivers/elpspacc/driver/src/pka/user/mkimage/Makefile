FLEX = flex
BISON = bison
CC = gcc -std=gnu99
CFLAGS = -g

OBJS = mkimage.o readmemh.o readmemh-scan.o readmemh-parse.o \
	symbol.o symbol-scan.o symbol-parse.o pkaelf.o

LIBS = ../pka-tool-common.o -lelf

all: pka-mkimage
clean:
	rm -f pka-mkimage *.o

pka-mkimage: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

%.c %.h: %.l
	$(FLEX) -o $*.c --header-file=$*.h $(FLEXFLAGS) $<

%.c %.h: %.y
	$(BISON) -o $*.c --defines=$*.h $(BISONFLAGS) $<

%.o: %.c
	$(CC) -I.. $(CPPFLAGS) $(CFLAGS) -c $<

mkimage.o: symbol.h readmemh.h

readmemh-scan.o: readmemh-parse.h
readmemh-parse.o: readmemh-scan.h
readmemh.o: readmemh-scan.h readmemh-parse.h

symbol-scan.o: symbol-scan.h
symbol-parse.o: symbol-parse.h
symbol.o: symbol-scan.h symbol-parse.h

.SECONDARY: readmemh-scan.c readmemh-parse.c symbol-scan.c symbol-parse.c
