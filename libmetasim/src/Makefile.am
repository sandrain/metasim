lib_LTLIBRARIES = libmetasim.la

libmetasim_HEADERS = metasim.h
libmetasimdir = $(includedir)

noinst_HEADERS =

libmetasim_la_SOURCES = metasim.c

libmetasim_la_CPPFLAGS = -I$(top_srcdir)/common/src

AM_CFLAGS = -Wall -Werror $(MARGO_CFLAGS) $(MERCURY_CFLAGS) $(ARGOBOT_CFLAGS)

libmetasim_la_LDFLAGS = $(MARGO_LDFLAGS)

libmetasim_la_LIBADD = $(MARGO_LIBS)

