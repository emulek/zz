AC_INIT([zz], [1.02], [emulek.bx@gmail.com])
dnl AC_DEFINE_UNQUOTED([HG_REVISION],["fb928cf63e86"],[mercurial revision])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_PREREQ([2.69])
AM_INIT_AUTOMAKE([-Wall -Werror gnu])

AM_SANITY_CHECK

# gettext support
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.16])

# documenations
AC_ARG_WITH(helpdir,[AC_HELP_STRING([--with-helpdir=dir],
	[Installation directory of the html help [${datadir}/doc/testproj]])],
	[helpdir="$withval"],[helpdir='${datadir}/doc/${PACKAGE}'])
AC_SUBST(helpdir)

# check programs
AC_PROG_CC
AC_PROG_CXX

AC_PROG_INSTALL
dnl AC_PROG_LIBTOOL

# documentations
can_build_documentation=yes

AC_PATH_PROG([xsltproc],[xsltproc])
if test -z "$xsltproc" ; then
	AC_MSG_WARN([xsltproc was not found. If you want to change and compile the documentation, \
please install libxslt (http://xmlsoft.org/XSLT/)])
	can_build_documentation=no
fi

AC_CHECK_FILE([doc/docbook/htmlhelp/htmlhelp.xsl], , [AC_MSG_WARN([htmlhelp sytle sheet was not found. \
If you want to change and compile the documentation, please install docbook-xsl \
(http://docbook.sourceforge.net/) and create a symlink in doc]) ; can_build_documentation=no])

AC_MSG_CHECKING([whether documentation can be changed and compiled])
AC_MSG_RESULT($can_build_documentation)
AC_SUBST(can_build_documentation)

# Checks for header files.
AC_CHECK_HEADERS([limits.h stdint.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_STRUCT_ST_BLOCKS
AC_CHECK_MEMBERS([struct stat.st_rdev])
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset memmem regcomp strerror getopt_long],,[exit 1])

# Checks for addition library functions.
AC_CHECK_LIB([z], [crc32],,[exit 1])
AC_CHECK_LIB([crypto], [SHA1_Update],,[exit 1])


AC_CONFIG_FILES([ po/Makefile.in
 Makefile
 src/Makefile
 doc/Makefile
])
AH_BOTTOM([#define HG_REVISION "5fcc105231b7"])

AC_OUTPUT

