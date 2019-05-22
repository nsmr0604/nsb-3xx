all:
	make -C libexfat CROSS_COMPILE=$(CROSS_COMPILE)
	make -C fuse  CROSS_COMPILE=$(CROSS_COMPILE) FUSE_DIR=$(FUSE_DIR)

clean:
	make -C libexfat clean
	make -C fuse clean
