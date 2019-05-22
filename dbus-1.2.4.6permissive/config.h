/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.in by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* poll doesn't work on devices */
/* #undef BROKEN_POLL */

/* Directory for installing the binaries */
#define DBUS_BINDIR "/usr/bin"

/* Build test code */
/* #undef DBUS_BUILD_TESTS */

/* Build X11-dependent code */
/* #undef DBUS_BUILD_X11 */

/* whether -export-dynamic was passed to libtool */
/* #undef DBUS_BUILT_R_DYNAMIC */

/* Use dnotify on Linux */
/* #undef DBUS_BUS_ENABLE_DNOTIFY_ON_LINUX */

/* Use inotify */
#define DBUS_BUS_ENABLE_INOTIFY 1

/* Use kqueue */
/* #undef DBUS_BUS_ENABLE_KQUEUE */

/* Directory to check for console ownerhip */
#define DBUS_CONSOLE_AUTH_DIR "/var/run/console/"

/* File to check for console ownerhip */
#define DBUS_CONSOLE_OWNER_FILE ""

/* Directory for installing the DBUS daemon */
#define DBUS_DAEMONDIR "/usr/sbin"

/* Directory for installing DBUS data files */
#define DBUS_DATADIR "/usr/share"

/* Disable assertion checking */
#define DBUS_DISABLE_ASSERT 1

/* Disable public API sanity checking */
/* #undef DBUS_DISABLE_CHECKS */

/* Build with caching of user data */
#define DBUS_ENABLE_USERDB_CACHE 1

/* Support a verbose mode */
/* #undef DBUS_ENABLE_VERBOSE_MODE */

/* Defined if gcov is enabled to force a rebuild due to config.h changing */
/* #undef DBUS_GCOV_ENABLED */

/* Always defined; expands to 1 if we have an atomic integer implementation,
   else 0 */
#define DBUS_HAVE_ATOMIC_INT_COND 0

/* Defined if we have gcc 3.3 and thus the new gcov format */
/* #undef DBUS_HAVE_GCC33_GCOV */

/* Directory for installing the libexec binaries */
#define DBUS_LIBEXECDIR "/usr/lib/dbus-1"

/* Where per-session bus puts its sockets */
#define DBUS_SESSION_SOCKET_DIR "/tmp"

/* The default D-Bus address of the system bus */
#define DBUS_SYSTEM_BUS_DEFAULT_ADDRESS "unix:path=/var/run/dbus/system_bus_socket"

/* The name of the socket the system bus listens on by default */
#define DBUS_SYSTEM_SOCKET "/var/run/dbus/system_bus_socket"

/* Full path to the launch helper test program in the builddir */
#define DBUS_TEST_LAUNCH_HELPER_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/bus/dbus-daemon-launch-helper-test"

/* Where to put test sockets */
#define DBUS_TEST_SOCKET_DIR "/tmp"

/* Defined on UNIX and Linux systems and not on Windows */
#define DBUS_UNIX "1"

/* User for running the system BUS daemon */
#define DBUS_USER "root"

/* Always defined; expands to 1 if we should use atomic integer implementation
   for 486, else 0 */
#define DBUS_USE_ATOMIC_INT_486_COND 0

/* A 'va_copy' style function */
#define DBUS_VA_COPY va_copy

/* 'va_lists' cannot be copies as values */
/* #undef DBUS_VA_COPY_AS_ARRAY */

/* The name of the gettext domain */
#define GETTEXT_PACKAGE "dbus-1"

/* Disable GLib assertion macros */
#define G_DISABLE_ASSERT 1

/* Disable GLib public API sanity checking */
/* #undef G_DISABLE_CHECKS */

/* Have abstract socket namespace */
/* #undef HAVE_ABSTRACT_SOCKETS */

/* Adt audit API */
/* #undef HAVE_ADT */

/* Define to 1 if you have the `backtrace' function. */
/* #undef HAVE_BACKTRACE */

/* Define to 1 if you have the `clearenv' function. */
#define HAVE_CLEARENV 1

/* Have cmsgcred structure */
/* #undef HAVE_CMSGCRED */

/* Have console owner file */
/* #undef HAVE_CONSOLE_OWNER_FILE */

/* Define to 1 if you have the <crt_externs.h> header file. */
/* #undef HAVE_CRT_EXTERNS_H */

/* Have the ddfd member of DIR */
/* #undef HAVE_DDFD */

/* Have dirfd function */
#define HAVE_DIRFD 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <errno.h> header file. */
#define HAVE_ERRNO_H 1

/* Define to 1 if you have the <execinfo.h> header file. */
/* #undef HAVE_EXECINFO_H */

/* Define to 1 if you have the <expat.h> header file. */
#define HAVE_EXPAT_H 1

/* Define to 1 if you have the `fpathconf' function. */
#define HAVE_FPATHCONF 1

/* Define to 1 if you have the `getgrouplist' function. */
#define HAVE_GETGROUPLIST 1

/* Define to 1 if you have the `getpeereid' function. */
/* #undef HAVE_GETPEEREID */

/* Define to 1 if you have the `getpeerucred' function. */
/* #undef HAVE_GETPEERUCRED */

/* Have GNU-style varargs macros */
#define HAVE_GNUC_VARARGS 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Have ISO C99 varargs macros */
#define HAVE_ISO_VARARGS 1

/* audit daemon SELinux support */
/* #undef HAVE_LIBAUDIT */

/* Define to 1 if you have the `nsl' library (-lnsl). */
/* #undef HAVE_LIBNSL */

/* Define to 1 if you have the `socket' library (-lsocket). */
/* #undef HAVE_LIBSOCKET */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `nanosleep' function. */
#define HAVE_NANOSLEEP 1

/* Have non-POSIX function getpwnam_r */
/* #undef HAVE_NONPOSIX_GETPWNAM_R */

/* Define if your system needs _NSGetEnviron to set up the environment */
/* #undef HAVE_NSGETENVIRON */

/* Define to 1 if you have the `poll' function. */
#define HAVE_POLL 1

/* Have POSIX function getpwnam_r */
#define HAVE_POSIX_GETPWNAM_R 1

/* SELinux support */
/* #undef HAVE_SELINUX */

/* Define to 1 if you have the `setenv' function. */
#define HAVE_SETENV 1

/* Define to 1 if you have the `setrlimit' function. */
#define HAVE_SETRLIMIT 1

/* Define to 1 if you have the `socketpair' function. */
#define HAVE_SOCKETPAIR 1

/* Have socklen_t type */
#define HAVE_SOCKLEN_T 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/inotify.h> header file. */
#define HAVE_SYS_INOTIFY_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/syslimits.h> header file. */
/* #undef HAVE_SYS_SYSLIMITS_H */

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
#define HAVE_SYS_UIO_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `unsetenv' function. */
#define HAVE_UNSETENV 1

/* Define to 1 if you have the `usleep' function. */
#define HAVE_USLEEP 1

/* Define to 1 if you have the `vasprintf' function. */
#define HAVE_VASPRINTF 1

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `writev' function. */
#define HAVE_WRITEV 1

/* Define to the sub-directory in which libtool stores uninstalled libraries.
   */
#define LT_OBJDIR ".libs/"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "dbus"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME "dbus"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "dbus 1.2.4.6permissive"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "dbus"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.2.4.6permissive"

/* The size of `char', as computed by sizeof. */
#define SIZEOF_CHAR 1

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `long long', as computed by sizeof. */
#define SIZEOF_LONG_LONG 8

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `void *', as computed by sizeof. */
#define SIZEOF_VOID_P 4

/* The size of `__int64', as computed by sizeof. */
#define SIZEOF___INT64 0

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Full path to the daemon in the builddir */
#define TEST_BUS_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/bus/dbus-daemon"

/* Full path to test file test/test-exit in builddir */
#define TEST_EXIT_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/test-exit"

/* Full path to test file test/data/invalid-service-files in builddir */
#define TEST_INVALID_SERVICE_DIR "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/data/invalid-service-files"

/* Full path to test file test/data/invalid-service-files-system in builddir
   */
#define TEST_INVALID_SERVICE_SYSTEM_DIR "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/data/invalid-service-files-system"

/* Full path to test file test/name-test/test-privserver in builddir */
#define TEST_PRIVSERVER_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/name-test/test-privserver"

/* Full path to test file test/test-segfault in builddir */
#define TEST_SEGFAULT_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/test-segfault"

/* Full path to test file test/test-service in builddir */
#define TEST_SERVICE_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/test-service"

/* Full path to test file test/test-shell-service in builddir */
#define TEST_SHELL_SERVICE_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/test-shell-service"

/* Full path to test file test/test-sleep-forever in builddir */
#define TEST_SLEEP_FOREVER_BINARY "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/test-sleep-forever"

/* Full path to test file test/data/valid-service-files in builddir */
#define TEST_VALID_SERVICE_DIR "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/data/valid-service-files"

/* Full path to test file test/data/valid-service-files-system in builddir */
#define TEST_VALID_SERVICE_SYSTEM_DIR "/home/igarashi/cortina/build_dir/target-arm_uClibc-0.9.32_eabi/dbus-1.2.4.6permissive/test/data/valid-service-files-system"

/* Version number of package */
#define VERSION "1.2.4.6permissive"

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif


			/* Use the compiler-provided endianness defines to allow universal compiling. */
			#if defined(__BIG_ENDIAN__)
			#define WORDS_BIGENDIAN 1
			#endif
		

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1


#if defined(HAVE_NSGETENVIRON) && defined(HAVE_CRT_EXTERNS_H)
# include <sys/time.h>
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#endif


/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif
