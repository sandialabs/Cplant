
LATEXMK      = latexmk
LATEXMKFLAGS = -ps
LATEXMKCLEAN = -c

SOURCES      = portals3.tex
EXECS        = $(SOURCES:.tex=.dvi)

$(EXECS): %.dvi : %.tex
	$(LATEXMK) $(LATEXMKFLAGS) $^

clean:
	$(LATEXMK) $(LATEXMKCLEAN)
	-rm -f *~

