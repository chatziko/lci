# Makefile for LCI documentation
# 
# Copyright (C) 2003 Kostas Hatzikokolakis
# This file is part of LCI
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

all:en

en: lcidoc.ps
gr: lcidoc_gr.ps


%.dvi: %.tex
	latex $<
	latex $<

%.ps: %.dvi
	dvips -Pcmz $< -o $@


clean:
	rm -f *.aux \
			*.log \
			*.toc \
			*.dvi \
			*.ps

run: en
