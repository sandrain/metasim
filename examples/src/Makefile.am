libexec_PROGRAMS = pingpong \
				   rpingpong

AM_CFLAGS = -Wall -Werror

AM_CPPFLAGS = -I$(top_srcdir)/libmetasim/src

AM_LDFLAGS = -static

LDADD = $(top_builddir)/libmetasim/src/libmetasim.la -lrt -lm

pingpong_SOURCES = pingpong.c

rpingpong_SOURCES = rpingpong.c

