include ../../../../Makefile-common

#INSTALLTARGETS = bootpfile
VML = axplinux-2.0.35-0.2/arch/alpha/boot/vmlinux.compute
INSTALLTARGETS = axplinux-2.0.35-0.2/arch/alpha/boot/bootpfile $(VML)
INSTALLDIR     = $(kerneldir)

all:

install cp: $(INSTALLTARGETS)
	@$($@)

$(VML):	
	cp axplinux-2.0.35-0.2/arch/alpha/boot/bootpfile axplinux-2.0.35-0.2/arch/alpha/boot/vmlinux.compute
