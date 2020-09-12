bin_PROGRAMS = metasimd

noinst_LIBRARIES = libmetasimd.a

metasimd_SOURCES = metasim-server.c

libmetasimd_a_SOURCES = metasim-log.c \
						metasim-rpc.c \
						metasim-rpc-tree.c \
						metasim-listener.c

noinst_HEADERS = metasim-log.h \
				 metasim-rpc.h \
				 metasim-rpc-tree.h \
				 metasim-server.h \
				 metasim-listener.h

metasimd_LDFLAGS = -static $(MARGO_LDFLAGS) $(SSG_LDFLAGS) $(MPI_CLDFLAGS)

metasimd_LDADD = $(MARGO_LIBS) $(SSG_LIBS) \
				 libmetasimd.a -lpthread -lm -lrt

AM_CPPFLAGS = -I$(top_srcdir)/common/src

AM_CFLAGS = -Wall \
            $(MARGO_CFLAGS) $(MERCURY_CFLAGS) $(ARGOBOT_CFLAGS) \
			$(SSG_CFLAGS) $(MPI_CFLAGS)

CLEANFILES = $(bin_PROGRAMS)

