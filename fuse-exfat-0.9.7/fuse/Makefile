OBJS=main.o
TARGET=libexfat.a
CFLAGS := -Wall -O2 -I../libexfat -I$(FUSE_DIR)/usr/include -DFUSE_USE_VERSION=26 -D_FILE_OFFSET_BITS=64
CC=$(CROSS_COMPILE)gcc
LD=$(CROSS_COMPILE)ld

all: $(OBJS)
	        $(CC) -o mount.exfat-fuse main.o -L../libexfat -L$(FUSE_DIR)/usr/lib -lexfat -lfuse

clean:
	        $(RM) $(TARGET) $(OBJS)


.PHONY: clean

