EXTRA_DIST = lcidoc.tex lcidoc.pdf
CLEANFILES = lcidoc.aux lcidoc.log lcidoc.dvi lcidoc.pdf \
			 lcidoc_gr.aux lcidoc_gr.log lcidoc_gr.dvi lcidoc_gr.pdf

en: lcidoc.pdf
gr: lcidoc_gr.pdf

lcidoc.pdf: lcidoc.dvi
lcidoc.dvi: lcidoc.tex

lcidoc_gr.pdf: lcidoc_gr.dvi
lcidoc_gr.dvi: lcidoc_gr.tex


SUFFIXES: .pdf .dvi

.dvi.pdf:
	dvipdf $<
.tex.dvi:
	latex $<
	latex $<
