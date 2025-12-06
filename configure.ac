# Detect version from git, VERSION file, or use stub
# This runs at autoconf time when configure.ac is processed
m4_define([_br_ar_version],
  [m4_esyscmd_s([
    # Try git first (if it's a git repository)
    if test -d .git && command -v git >/dev/null 2>&1; then
      git_version=$(git describe --tags --always --dirty 2>/dev/null | sed 's/^v//')
      if test -n "$git_version"; then
        echo "$git_version"
        exit 0
      fi
    fi
    # Fallback to VERSION file
    if test -f VERSION; then
      cat VERSION | tr -d '\n\r '
      exit 0
    fi
    # Use stub version
    echo "1.21.124"
  ])])

AC_INIT([br-ar], _br_ar_version, [])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_SRCDIR([src/br_ar.c])
AC_CONFIG_HEADERS([config.h])

# Check for programs
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_AR

# Check for standard headers
AC_C_INLINE
AC_CHECK_HEADERS([sys/types.h sys/stat.h dirent.h unistd.h stdint.h stdbool.h limits.h errno.h])

# Check for functions
AC_CHECK_FUNCS([malloc realloc free strdup memset mkdir strrchr])

# Check for types
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Check endianness
AC_C_BIGENDIAN

# Platform-specific endian operations
AC_ARG_ENABLE([platform-endian],
    [AS_HELP_STRING([--disable-platform-endian],
        [Use generic endian operations instead of platform-specific ones])],
    [use_platform_endian=$enableval],
    [use_platform_endian=yes])

if test "x$use_platform_endian" = "xyes"; then
    # Check for platform-specific endian headers and functions
    case "$host_os" in
        *darwin*)
            AC_CHECK_HEADERS([libkern/OSByteOrder.h],
                [AC_DEFINE([USE_PLATFORM_ENDIAN], [1], [Use platform-specific endian operations])
                 AC_DEFINE([HAVE_DARWIN_ENDIAN], [1], [Have Darwin endian functions])])
            ;;
        *freebsd*)
            AC_CHECK_HEADERS([sys/endian.h],
                [AC_DEFINE([USE_PLATFORM_ENDIAN], [1], [Use platform-specific endian operations])
                 AC_DEFINE([HAVE_FREEBSD_ENDIAN], [1], [Have FreeBSD endian functions])])
            ;;
        *openbsd*)
            AC_CHECK_HEADERS([sys/endian.h],
                [AC_DEFINE([USE_PLATFORM_ENDIAN], [1], [Use platform-specific endian operations])
                 AC_DEFINE([HAVE_OPENBSD_ENDIAN], [1], [Have OpenBSD endian functions])])
            ;;
        *linux*)
            AC_CHECK_HEADERS([endian.h],
                [AC_CHECK_HEADERS([byteswap.h])
                 AC_DEFINE([USE_PLATFORM_ENDIAN], [1], [Use platform-specific endian operations])
                 AC_DEFINE([HAVE_LINUX_ENDIAN], [1], [Have Linux endian functions])])
            ;;
        *)
            # Fallback to generic implementation
            AC_DEFINE([USE_PLATFORM_ENDIAN], [0], [Use generic endian operations])
            ;;
    esac
else
    AC_DEFINE([USE_PLATFORM_ENDIAN], [0], [Use generic endian operations])
fi

# Tool name configuration
AC_ARG_VAR([TOOL_NAME], [Name of the tool executable])
if test -z "$TOOL_NAME"; then
    TOOL_NAME="br-ar"
fi
AC_SUBST([TOOL_NAME])

# Symlink installation
AC_ARG_ENABLE([symlinks],
    [AS_HELP_STRING([--disable-symlinks],
        [Do not create brarchive and brarchive-cli symlinks])],
    [enable_symlinks=$enableval],
    [enable_symlinks=yes])

AM_CONDITIONAL([ENABLE_SYMLINKS], [test "x$enable_symlinks" = "xyes"])
if test "x$enable_symlinks" = "xyes"; then
    AC_DEFINE([ENABLE_SYMLINKS], [1], [Enable symlink installation])
fi
ENABLE_SYMLINKS_VAL=$enable_symlinks
AC_SUBST([ENABLE_SYMLINKS_VAL])

# Man pages
AC_CONFIG_FILES([br-ar.1 brarchive.5])

# Output files
AC_CONFIG_FILES([Makefile src/Makefile tests/Makefile src/brarchive-cli])
AC_OUTPUT

