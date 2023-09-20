
include ../../../../Makefile-common

ifeq ($(CPLANT_ARCH),alpha)
IMAGE = bootpfile
else
IMAGE = bzImage
endif
LLINK = $(CPLANT_TOP)/compute/OS/linux

KERNEL_LVER = 2.2.18
KERNEL_LOC  = /Net/local/users/pumatst

INSTALLDIR      = $(kerneldir)
INSTALLTARGETS  = vmlinux.compute

all: $(LLINK)

$(LLINK): vmlinux.compute
	rm -rf $(LLINK)
	ln -s linux-patches/linux-$(KERNEL_LVER)/linux $(LLINK)

vmlinux.compute: $(IMAGE)
	cp $(IMAGE) vmlinux.compute

$(IMAGE):
	$(CPLANT_ARCH).bash $(KERNEL_LVER) $(KERNEL_LOC) $(CPLANT_TARGET)

install uninstall cp: $(INSTALLTARGETS)
	@$($@)

clean:
	rm -rf $(IMAGE) vmlinux.compute linux $(LLINK)
