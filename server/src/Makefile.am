bin_PROGRAMS = metasimd margotree

noinst_LIBRARIES = libmetasimd.a

metasimd_SOURCES = metasim-server.c

libmetasimd_a_SOURCES = metasim-log.c \
                        metasim-rpc.c \
                        metasim-rpc-tree.c \
                        metasim-listener.c

margotree_SOURCES = margotree.c

noinst_HEADERS = metasim-log.h \
                 metasim-rpc.h \
                 metasim-rpc-tree.h \
                 metasim-server.h \
                 metasim-listener.h

AM_LDFLAGS = -static $(MARGO_LDFLAGS) $(MPI_CLDFLAGS)

LDADD = $(MARGO_LIBS) libmetasimd.a -lpthread -lm -lrt

AM_CPPFLAGS = -I$(top_srcdir)/common/src

AM_CFLAGS = -Wall \
            $(MARGO_CFLAGS) $(MERCURY_CFLAGS) $(ARGOBOT_CFLAGS) \
            $(SSG_CFLAGS) $(MPI_CFLAGS)

CLEANFILES = $(bin_PROGRAMS)

