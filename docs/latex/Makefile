# Build all Durin's Code LaTeX documents
DOCS = 01_language_reference 02_architecture 03_team_reflection 04_test_report
LATEXMK = latexmk -pdf -interaction=nonstopmode -halt-on-error

.PHONY: all clean

all: $(addsuffix .pdf, $(DOCS))

%.pdf: %.tex preamble.tex
	$(LATEXMK) $<

clean:
	latexmk -CA
	rm -f *.aux *.log *.out *.toc *.fls *.fdb_latexmk *.synctex.gz
