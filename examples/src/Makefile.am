libexec_PROGRAMS = ping echo sum

AM_CFLAGS = -Wall $(MPI_CFLAGS)

AM_CPPFLAGS = -I$(top_srcdir)/libmetasim/src

AM_LDFLAGS = -static $(MPI_CLDFLAGS)

LDADD = $(top_builddir)/libmetasim/src/libmetasim.la -lrt -lm

ping_SOURCES = ping.c

echo_SOURCES = echo.c

sum_SOURCES = sum.c

