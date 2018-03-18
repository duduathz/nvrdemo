#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)

AC_INIT(nvr-h3c, 1.0.0, chaixiaowei2008@qq.com)
AC_CONFIG_SRCDIR(README)
AC_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(nvr,1.0)

# Checks for programs.
AC_PROG_CC
AC_PROG_CPP
#AC_PROG_CXX
AM_CONDITIONAL(GCC, test "$GCC" = yes)   # let the Makefile know if we're gcc
AC_PROG_MAKE_SET
AC_PROG_LIBTOOL
AC_SUBST(LIBTOOL_DEPS)


# Checks mutilthread
AC_PROG_RANLIB 

# Checks for libraries.
AC_CHECK_LIB(pthread, pthread_kill)
AC_CHECK_LIB(crypto,CRYPTO_num_locks)
AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(ssl,SSL_free)
AC_CHECK_LIB(jthread,_ZN6JMutex4InitEv)
AC_CHECK_LIB(jrtp,_Z17RTPGetErrorStringi)
AC_CHECK_LIB(osip2,ict_create_ack)
AC_CHECK_LIB(eXosip2,RijndaelEncrypt)
AC_CHECK_LIB(osipparser2,osip_atoi)
# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h inttypes.h limits.h netdb.h netinet/in.h stddef.h sys/socket.h sys/time.h sys/vfs.h])
AC_CHECK_FUNCS([getmntent])
AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memchr])
AC_CHECK_FUNCS([memmove])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([select])
AC_CHECK_FUNCS([setlocale])
AC_CHECK_FUNCS([strchr])
AC_CHECK_FUNCS([strcspn]) 
AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strrchr]) 
AC_CHECK_FUNCS([strspn]) 
AC_CHECK_FUNCS([strstr]) 
AC_CHECK_FUNCS([strtoul]) 
AC_CHECK_HEADERS([libintl.h]) 
AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([mntent.h]) 
AC_CHECK_HEADERS([sys/file.h])
AC_CHECK_HEADERS([sys/statfs.h])
AC_CHECK_TYPES([ptrdiff_t])
AC_C_INLINE
AC_FUNC_GETMNTENT
# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_C_VOLATILE

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([dup2 gethostbyname inet_ntoa socket])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
