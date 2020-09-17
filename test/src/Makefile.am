libexec_PROGRAMS = echo-server echo-client

noinst_HEADERS = echo.h

echo_server_SOURCES = echo-server.c

echo_client_SOURCES = echo-client.c

AM_CFLAGS = -Wall $(MARGO_CFLAGS) $(MPI_CFLAGS)

AM_LDFLAGS = -static $(MPI_CLDFLAGS)

LDADD = $(MARGO_LIBS) $(MPI_LIBS)

CLEANFILES = $(bin_PROGRAMS)


