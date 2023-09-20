
include ../../../../Makefile-common

CPLANT_ARCH = i386

ifeq ($(CPLANT_ARCH),alpha)
IMAGE = bootpfile
else
IMAGE = bzImage
TARGET = BULK
endif
LLINK = $(CPLANT_TOP)/compute/OS/linux

KERNEL_LVER = 2.2.14
KERNEL_LOC  = /Net/local/users/jsotto

INSTALLDIR      = $(kerneldir)
INSTALLTARGETS  = vmlinux.compute

all: $(LLINK)

$(LLINK): vmlinux.compute $(KERNEL_LOC)/linux-$(KERNEL_LVER)
	$(CPLANT_ARCH).bash $(KERNEL_LVER) $(KERNEL_LOC) $(TARGET)
	cp $(IMAGE) vmlinux.compute
	rm -rf $(LLINK)
	ln -s linux-patches/linux-$(KERNEL_LVER)/linux $(LLINK)

install: $(INSTALLTARGETS)
	@$($@)

clean:
	rm -rf $(IMAGE) linux-$(KERNEL_LVER).tar.gz linux $(LLINK)
