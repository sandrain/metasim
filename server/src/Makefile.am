bin_PROGRAMS = metasimd

noinst_LIBRARIES = libmetasimd.a

metasimd_SOURCES = metasim-server.c

libmetasimd_a_SOURCES = metasim-log.c \
                        metasim-rpc.c \
                        metasim-server.c

noinst_HEADERS = metasim-log.h \
                 metasim-rpc.h \
                 metasim-server.h

AM_LDFLAGS = -static $(MARGO_LDFLAGS) $(MPI_CLDFLAGS)

LDADD = $(MARGO_LIBS) libmetasimd.a -lpthread -lm -lrt

AM_CPPFLAGS = -I$(top_srcdir)/common/src

AM_CFLAGS = -Wall \
            $(MARGO_CFLAGS) $(MERCURY_CFLAGS) $(ARGOBOT_CFLAGS) \
            $(SSG_CFLAGS) $(MPI_CFLAGS)

CLEANFILES = $(bin_PROGRAMS)

