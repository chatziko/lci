AM_CFLAGS = -DDATADIR=\"$(datadir)\"

bin_PROGRAMS = lci
lci_SOURCES = main.c decllist.c  grammar.c  parser.c  run.c  termproc.c kazlib/list.c

noinst_HEADERS = decllist.h  grammar.h  parser.h  run.h  termproc.h kazlib/list.h

dist_pkgdata_DATA = .lcirc queens.lci

