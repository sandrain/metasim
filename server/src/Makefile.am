bin_PROGRAMS = metasimd

noinst_LIBRARIES = libmetasimd.a

metasimd_SOURCES = metasim-server.c

libmetasimd_a_SOURCES = metasim-comm.c \
						metasim-config.c \
						metasim-log.c \
						metasim-rpc-local.c \
						metasim-rpc-remote.c

noinst_HEADERS = metasim-comm.h \
				 metasim-config.h \
				 metasim-log.h \
				 metasim-rpc-local.h \
				 metasim-rpc-remote.h

metasimd_LDFLAGS = -static $(MARGO_LDFLAGS) $(SSG_LDFLAGS)

metasimd_LDADD = $(MARGO_LIBS) $(SSG_LIBS) \
				 libmetasimd.a -lpthread -lm -lrt

AM_CPPFLAGS = -I$(top_srcdir)/common/src

AM_CFLAGS = -Wall \
            $(MARGO_CFLAGS) $(MERCURY_CFLAGS) $(ARGOBOT_CFLAGS) $(SSG_CFLAGS)

CLEANFILES = $(bin_PROGRAMS)

