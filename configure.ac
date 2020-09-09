dnl
dnl Process this file with autoconf to produce a configure script.

AC_LANG([C])

AC_INIT([metasim],
        m4_esyscmd([git describe --always |
                   awk '/.*/{sub(/^v/,""); printf "%s",$1; exit}']),
        [unifycr@llnl.gov])
AC_PREREQ(2.60)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign tar-pax subdir-objects])
AM_SILENT_RULES([yes])

AM_MAINTAINER_MODE([disable])

AC_PROG_CC_STDC
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

dnl Need to do Fortran checks before initializing LIBTOOL
LT_INIT

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_CHECK_TYPES([ptrdiff_t])

# Checks for header files.
AC_CHECK_HEADERS([stddef.h stdint.h stdlib.h string.h unistd.h])
AC_CHECK_HEADERS([fcntl.h inttypes.h libgen.h limits.h mntent.h strings.h syslog.h])
AC_CHECK_HEADERS([wchar.h wctype.h])
AC_CHECK_HEADERS([sys/mount.h sys/socket.h sys/statfs.h sys/time.h])
AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([ftruncate getpagesize gettimeofday memset socket floor])
AC_CHECK_FUNCS([gethostbyname strcasecmp strdup strerror strncasecmp strrchr])
AC_CHECK_FUNCS([gethostname strstr strtoumax strtol uname posix_fallocate])

# PMPI Init/Fini mount/unmount option
AC_ARG_ENABLE([mpi-mount],[AS_HELP_STRING([--enable-mpi-mount],[Enable transparent mount/unmount at MPI_Init/Finalize.])])
AS_IF([test "x$enable_mpi_mount" = "xyes"],[
    AM_CONDITIONAL([USE_PMPI_WRAPPERS], [true])
],[
    AM_CONDITIONAL([USE_PMPI_WRAPPERS], [false])
])

# PMIx support build option
AC_ARG_ENABLE([pmix],[AS_HELP_STRING([--enable-pmix],[Enable PMIx build options.])])
AS_IF([test "x$enable_pmix" = "xyes"],[
    AC_CHECK_HEADERS([pmix.h],
                     [AM_CONDITIONAL([USE_PMIX], [true])],
                     [AM_CONDITIONAL([USE_PMIX], [false])])
],[
    AM_CONDITIONAL([USE_PMIX], [false])
])

# PMI2 support build option
AC_ARG_ENABLE([pmi],[AS_HELP_STRING([--enable-pmi],[Enable PMI2 build options.])])
AS_IF([test "x$enable_pmi" = "xyes"],[
    AC_CHECK_HEADERS([pmi2.h],
                     [AM_CONDITIONAL([USE_PMI2], [true])],
                     [AM_CONDITIONAL([USE_PMI2], [false])])
],[
    AM_CONDITIONAL([USE_PMI2], [false])
])

AC_ARG_WITH(pkgconfigdir,
    [AS_HELP_STRING([--with-pkgconfigdir=DIR],[pkgconfig file in DIR @<:@LIBDIR/pkgconfig@:>@])],
            [pkgconfigdir=$withval],
            [pkgconfigdir='${libdir}/pkgconfig'])
AC_SUBST(pkgconfigdir)

## metasim options
AC_ARG_ENABLE([cuserid],[AS_HELP_STRING([--disable-cuserid],[Disables attempted use of cuserid() at run time])],[
        AS_IF([test "x$enableval" = "xno"],[
            AC_DEFINE(CRUISE_DISABLE_CUSERID, 1, Define if cuserid() should be disabled),],[])]
,)

LX_FIND_MPI

# look for MPI and set flags
AS_IF([test "$have_C_mpi" != "yes"],
    AC_MSG_ERROR(["Couldn't find MPI"]),
    []
)

UNIFYFS_AC_MARGO

UNIFYFS_AC_SSG

# checks to see how we can print 64 bit values on this architecture
gt_INTTYPES_PRI

dnl Check byte ordering
AC_C_BIGENDIAN

dnl temporarily set large file flags just for this test; we don't want
dnl it to propagate to the makefile because of zlib bugs
AC_MSG_CHECKING(for off64_t)
AC_TRY_COMPILE(
    [
    #define _FILE_OFFSET_BITS 64
    #define _LARGEFILE64_SOURCE
    #include <sys/types.h>
    ],
    [ off64_t off; ],
    AC_MSG_RESULT(yes)
    AC_DEFINE(HAVE_OFF64_T, 1, Define if off64_t type is defined),
    AC_MSG_RESULT(no)
)

AC_MSG_CHECKING(if linker supports -wrap)
OLD_LDFLAGS=$LDFLAGS
LDFLAGS=$LDFLAGS
LDFLAGS+="-Wl,-wrap,malloc"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <stdlib.h>]],[[void* __wrap_malloc(size_t size);]],[[int *test = malloc(sizeof(int));]])],
[
    AC_MSG_RESULT([yes])
    AM_CONDITIONAL([HAVE_LD_WRAP],[true])
],[
    AC_MSG_RESULT([no])
    AM_CONDITIONAL([HAVE_LD_WRAP],[false])
])
LDFLAGS=$OLD_LDFLAGS


# We need to know the value of the $libdir and $bindir variables so that
# we can reference the correct path in the metasim compiler wrappers.
# Unfortunately, those two variables are not normally evaluated by autoconf.
# They are evaluated at build time using Makefile variable substitutions.
#
# The following logic was copied from mpich2 1.3.1 to resolve the $libdir
# variable at configure time.
#
# Temporarily replace the default NONE value for exec_prefix
# and prefix with the actual, default values.
savePrefix=$prefix
saveExecprefix=$exec_prefix
test "x$prefix" = xNONE && prefix=$ac_default_prefix
test "x$exec_prefix" = xNONE && exec_prefix=$prefix
eval metasim_lib_path=$libdir
eval metasim_bin_path=$bindir
prefix=$savePrefix
exec_prefix=$saveExecprefix

# autoconf<=2.69, runstatedir is not configurable.
if test "x$runstatedir" = x; then
        AC_SUBST([runstatedir], ['${localstatedir}/run'])
fi

AC_SUBST(metasim_lib_path)
AC_SUBST(metasim_bin_path)
AC_SUBST(LDFLAGS)
AC_SUBST(__CP_LOG_PATH)
AC_SUBST(CP_WRAPPERS)
AC_SUBST(DISABLE_LDPRELOAD)

AC_CONFIG_FILES([Makefile
                 common/Makefile
                 common/src/Makefile
                 examples/Makefile
                 examples/src/Makefile
                 libmetasim/Makefile
                 libmetasim/src/Makefile
                 server/Makefile
                 server/src/Makefile])

METASIM_VERSION=${PACKAGE_VERSION}
AC_SUBST(METASIM_VERSION)
AC_SUBST([LIBMETASIM_LT_VERSION], [1:0:0])

# pkgconfig versioning
LIBMETASIM_API_VERSION="1.0.0"
AC_SUBST(LIBMETASIM_API_VERSION)

AC_OUTPUT

AC_MSG_RESULT([

    ==========================
    METASIM 
    ==========================
    prefix      ${prefix}
    compiler    ${CC}
    CFLAGS      ${CFLAGS}
    ==========================

])

