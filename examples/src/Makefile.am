libexec_PROGRAMS = ping

AM_CFLAGS = -Wall $(MPI_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/libmetasim/src

AM_LDFLAGS = -static $(MPI_CLDFLAGS)

LDADD = $(top_builddir)/libmetasim/src/libmetasim.la -lrt -lm

ping_SOURCES = ping.c

