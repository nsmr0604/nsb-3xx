srcdir=/home/igarashi/cortina/build_dir/linux-g2_wan/libreswan-3.9/
OBJDIR=OBJ.linux.arm
OBJDIRTOP=/home/igarashi/cortina/build_dir/linux-g2_wan/libreswan-3.9/OBJ.linux.arm
VPATH+=${srcdir}
OSDEP=linux
BUILDENV=linux
LIBRESWANSRCDIR=/home/igarashi/cortina/build_dir/linux-g2_wan/libreswan-3.9
include ${srcdir}Makefile.top
programs config man install clean checkprograms:: 
	@for d in $(SUBDIRS) ; \
	do \
		(cd $$d && $(MAKE) LIBRESWANSRCDIR=${LIBRESWANSRCDIR} $@ ) || exit 1; \
	done; 
