# Makefile to compile/install LCI
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

# This is a simple Makefile used to compile and install the program from the
# root directory. Compilation is done by invoking make in the src directory.
# Install just copies the executable to $(bindir) (default is /usr/local/bin)

prefix=/usr/local
bindir=$(prefix)/bin


all: lci

lci:
	$(MAKE) -C src


install: lci
	cp src/lci $(bindir)

clean:
	$(MAKE) -C src clean

