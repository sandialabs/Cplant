
#
# $Id: Makefile,v 1.2 2001/01/11 00:21:56 rbbrigh Exp $
#
# Makefile for generating man pages from qwertz sgml
#

MAN3SOURCES   =  PtlACEntry.sgml PtlEQGet.sgml PtlGetId.sgml PtlMDUnlink.sgml      \
                 PtlMEUnlink.sgml PtlNIHandle.sgml PtlTransId.sgml PtlEQAlloc.sgml \
                 PtlEQWait.sgml PtlInit.sgml PtlMDUpdate.sgml PtlNIBarrier.sgml    \
                 PtlNIInit.sgml PtlEQCount.sgml PtlFini.sgml PtlMDAttach.sgml      \
                 PtlMEAttach.sgml PtlNIDist.sgml PtlNIStatus.sgml PtlEQFree.sgml   \
                 PtlGet.sgml PtlMDInsert.sgml PtlMEInsert.sgml PtlNIFini.sgml      \
                 PtlPut.sgml PtlMDBind.sgml
MAN3DIR       = ../man/man3
MAN3TARGETS   = $(addprefix $(MAN3DIR)/,$(MAN3SOURCES:.sgml=.3))

SGML2MAN      = format
SGML2MANFLAGS = -T pumaman

all: $(MAN3TARGETS)


$(MAN3DIR):
	mkdir $@

$(MAN3DIR)/%.3: %.sgml $(MAN3DIR)
	$(SGML2MAN) $(SGML2MANFLAGS) $< > $@

clean:
	-rm -f $(MAN3TARGETS) *~

realclean:
	-rm -rf $(MAN3DIR)
