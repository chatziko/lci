# Makefile for LCI
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

CC=gcc
CFLAGS=-Wall -O3 -fno-strict-aliasing


all: lci

lci: main.o parser.o grammar.o termproc.o decllist.o run.o
	$(CC) $(CFLAGS) main.o parser.o grammar.o termproc.o decllist.o run.o -o lci


# the following rules can be generated automaticcaly with following command
# 		gcc -MM *.c
decllist.o: decllist.c decllist.h grammar.h termproc.h parser.h
grammar.o: 	grammar.c parser.h grammar.h termproc.h decllist.h run.h
main.o: 		main.c parser.h grammar.h run.h
parser.o: 	parser.c parser.h grammar.h
run.o: 		run.c run.h grammar.h parser.h termproc.h decllist.h
termproc.o: termproc.c termproc.h grammar.h decllist.h parser.h run.h


clean:
	rm -f *.o
	rm -f lci lci.exe

run: lci
	./lci

tar:
	tar -cf lci.tar *.c *.h .lcirc *.lci Makefile LICENCE
	gzip lci.tar


