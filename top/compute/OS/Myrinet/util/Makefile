#
# $Id: Makefile,v 1.53 2002/02/05 17:18:49 pumatst Exp $
# Makefile for mcpload and other utilities
#

include		../../../../Makefile-common

CC		= gcc -Wall -g -O4
INCLUDES	= -I../../../../include/sys -I../include -I../../linux/include
INCLUDES       += -I../../../../include -I../MCP
PING_INCL	= -I../../../../include -I../../../../include/portals
CFLAGS		= -Wall $(INCLUDES)
PUMALIBDIR      = $(CPLANT_TOP)/lib/puma/$(CPLANT_ARCH)-$(OS)
LIB_DIR		= -L../lib
LIBS		= -lLanaiDevice -lmyriFiles
#LIBS2		= -lLanaiDevice

ifeq ($(CPLANT_ARCH),i386) 
LOAD_OBJ	= common.o timing.o route_util.o ../lib/stat.o
else
LOAD_OBJ	= common.o timing.o route_util.o
endif
STAT_OBJ	= common.o disp_info.o

EXECS = mcpload mcpstat mcpmemtst hstmemtst
#EXECS = mcpload mcpstat mcpmemtst mcpnettst hstmemtst

all:	$(EXECS)

.alllib:	../lib/libLanaiDevice.a ../lib/libbfd.a
	@touch $@

../lib/libLanaiDevice.a:
	@(cd ../lib; make; cd -; touch $@)

hstmemtst:	hstmemtst.c ../../../../include/sys/defines.h
	$(CC) $(CFLAGS) -o $@ $<

mcpmemtst:	mcpmemtst.c common.o ../include/lanai_device.h \
		../../../../include/sys/defines.h
	$(CC) $(CFLAGS) -o $@ $< common.o $(LIB_DIR) $(LIBS) -lbfd

mcpnettst:	mcpnettst.c common.o ../include/lanai_device.h \
		../../../../include/sys/defines.h
	$(CC) $(CFLAGS) -o $@ $< common.o $(LIB_DIR) $(LIBS) -lbfd

mcpload:	mcpload.o $(LOAD_OBJ) .alllib ../include/lanai_device.h \
		../../../../include/sys/defines.h ../MCP/MCPshmem.h \
		common.h route_util.h
	$(CC) $(CFLAGS) -o $@ $< $(LOAD_OBJ) $(LIB_DIR) $(LIBS) -lbfd

mcpstat:	mcpstatus.o $(STAT_OBJ) .alllib ../include/lanai_device.h \
		../../../../include/sys/defines.h ../MCP/MCPshmem.h \
		common.h disp_info.h
	$(CC) $(CFLAGS) -o $@ $< $(STAT_OBJ) $(LIB_DIR) $(LIBS)

mcpload.o:	mcpload.c
	$(CC) $(CFLAGS) -c -o $@ $<

mcpstat.o:	mcpstat.c
	$(CC) $(CFLAGS) -c -o $@ $<

route_util.o:	route_util.c route_util.h arch_asm.h ../MCP/MCPshmem.h
	$(CC) $(CFLAGS) -c -o $@ $<

disp_info.o:	disp_info.c disp_info.h ../MCP/MCPshmem.h arch_asm.h \
		    ../../../../include/sys/defines.h ../include/lanai_device.h
	$(CC) $(CFLAGS) -c -o $@ $<

common.o:	common.c common.h ../MCP/MCPshmem.h arch_asm.h \
		    ../../../../include/sys/defines.h ../include/lanai_device.h
	$(CC) $(CFLAGS) -c -o $@ $<

timing.o:	timing.c timing.h
	$(CC) -c -o $@ $<

#-------------------------------------------------------------------------------

.PHONY: clean cp install depend

clean:
	"rm" -f *.o *.lst mcpload mcpstat mcpmemtst mcpnettst .alllib

install cp:
	"cp" -p -f $(EXECS) $(sbindir)

%.lst:	%.c
	$(CC) $(CFLAGS) -c $*.c -g -Wa,-adhnls=$*.lst

%.s:	%.c
	$(CC) $(CFLAGS) -S $*.c -o $*.s

depend:
	@echo "No need for depend. Explicit dependencies in Makefile"
