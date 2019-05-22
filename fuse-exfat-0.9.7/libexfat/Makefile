OBJS=cluster.o io.o log.o lookup.o mount.o node.o time.o utf.o utils.o
TARGET=libexfat.a
CFLAGS := -Wall -O2 -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64
CC :=$(CROSS_COMPILE)gcc

all: $(OBJS)
	$(AR) rc $(TARGET) $^
	ranlib $(TARGET)

clean:
	$(RM) $(TARGET) $(OBJS)


.PHONY: clean
