TEXFILES = $(wildcard *.tex)
BIBFILES = $(wildcard *.bib)

# initial target files
TARGETS = report

all: $(TARGETS).pdf # a.pdf b.pdf

# initial aux file generation
%.aux: %.tex
	pdflatex $<

# bbl file generation
%.bbl: %.aux
	bibtex --min-crossrefs=20 $*

# pdf output  %%.bbl $(TEXFILES) $(BIBFILES) *.cls 
%.pdf: %.tex %.bbl
	pdflatex $<
	pdflatex $<

# generate pdf+dvi+ps
full: $(TARGETS).tex
	latex $^
	latex $^
	dvips -P pdf -G0 -t letter -o $(TARGETS).ps $(TARGETS).dvi	
	ps2pdf -dCompatibilityLevel=1.4 -dPDFSETTINGS=/prepress $(TARGETS).ps $(TARGETS).pdf

clean:
	-rm $(TARGETS).pdf *.aux *.log *.bbl *.blg *.out *.synctex.gz *.dvi *.ps *~ *.lot *.lof *.toc

# below may not be supported by default
view: 
	acroread  -geometry 1000x1000 $(TARGETS).pdf

html:	
	latex2html -nonavigation -math -html_version '4.0' -split 0 $(TARGETS).tex

check:
	pdflatex $(TARGETS).tex | grep -i -e "undefined" -e "multiply"

evince:
	pdflatex $(TARGETS).tex
	evince $(TARGETS).pdf &

acro:
	pdflatex $(TARGETS).tex
	acroread $(TARGETS).pdf &

osx:
	pdflatex $(TARGETS).tex
	open $(TARGETS).pdf

windows:
	pdflatex $(TARGETS).tex
	explorer.exe $(TARGETS).pdf &

home: evince
