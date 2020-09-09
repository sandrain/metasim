AC_DEFUN([UNIFYFS_AC_SSG], [
  # preserve state of flags
  SSG_OLD_CFLAGS=$CFLAGS
  SSG_OLD_CXXFLAGS=$CXXFLAGS
  SSG_OLD_LDFLAGS=$LDFLAGS

  PKG_CHECK_MODULES([SSG],[ssg],
   [
    AC_SUBST(SSG_CFLAGS)
    AC_SUBST(SSG_LIBS)
   ],
   [AC_MSG_ERROR(m4_normalize([
     couldn't find a suitable ssg, set environment variable
     PKG_CONFIG_PATH=paths/for/{ssg}/lib/pkgconfig
   ]))])

  # restore flags
  CFLAGS=$SSG_OLD_CFLAGS
  CXXFLAGS=$SSG_OLD_CXXFLAGS
  LDFLAGS=$SSG_OLD_LDFLAGS
])

