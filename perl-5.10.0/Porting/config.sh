#!/bin/sh
#
# This file was produced by running the Configure script. It holds all the
# definitions figured out by Configure. Should you modify one of these values,
# do not forget to propagate your changes by running "Configure -der". You may
# instead choose to run each of the .SH files by yourself, or "Configure -S".
#

# Package name      : perl5
# Source directory  : .
# Configuration time: Thu Sep  6 16:06:22 CEST 2007
# Configured by     : merijn
# Target system     : linux nb09 2.6.18.8-0.5-default #1 smp fri jun 22 12:17:53 utc 2007 i686 i686 i386 gnulinux 

Author=''
Date='$Date'
Header=''
Id='$Id'
Locker=''
Log='$Log'
Mcc='Mcc'
RCSfile='$RCSfile'
Revision='$Revision'
Source=''
State=''
_a='.a'
_exe=''
_o='.o'
afs='false'
afsroot='/afs'
alignbytes='4'
ansi2knr=''
aphostname='/bin/hostname'
api_revision='5'
api_subversion='0'
api_version='10'
api_versionstring='5.10.0'
ar='ar'
archlib='/pro/lib/perl5/5.10.0/i686-linux-thread-multi-64int-ld'
archlibexp='/pro/lib/perl5/5.10.0/i686-linux-thread-multi-64int-ld'
archname64='64int'
archname='i686-linux-thread-multi-64int-ld'
archobjs=''
asctime_r_proto='REENTRANT_PROTO_B_SB'
awk='awk'
baserev='5.0'
bash=''
bin='/pro/bin'
binexp='/pro/bin'
bison='bison'
byacc='byacc'
byteorder='12345678'
c=''
castflags='0'
cat='cat'
cc='cc'
cccdlflags='-fPIC'
ccdlflags='-Wl,-E'
ccflags='-D_REENTRANT -D_GNU_SOURCE -DDEBUGGING -fno-strict-aliasing -pipe -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64'
ccflags_uselargefiles='-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64'
ccname='gcc'
ccsymbols=''
ccversion=''
cf_by='merijn'
cf_email='hmbrand@cpan.org'
cf_time='Thu Sep  6 16:06:22 CEST 2007'
chgrp=''
chmod='chmod'
chown=''
clocktype='clock_t'
comm='comm'
compress=''
contains='grep'
cp='cp'
cpio=''
cpp='cpp'
cpp_stuff='42'
cppccsymbols=''
cppflags='-D_REENTRANT -D_GNU_SOURCE -DDEBUGGING -fno-strict-aliasing -pipe'
cpplast='-'
cppminus='-'
cpprun='cc -E'
cppstdin='cc -E'
cppsymbols='__CHAR_BIT__=8 __DBL_DENORM_MIN__=4.9406564584124654e-324 __DBL_DIG__=15 __DBL_EPSILON__=2.2204460492503131e-16 __DBL_HAS_INFINITY__=1 __DBL_HAS_QUIET_NAN__=1 __DBL_MANT_DIG__=53 __DBL_MAX_10_EXP__=308 __DBL_MAX__=1.7976931348623157e+308 __DBL_MAX_EXP__=1024 __DBL_MIN_10_EXP__=(-307) __DBL_MIN__=2.2250738585072014e-308 __DBL_MIN_EXP__=(-1021) __DECIMAL_DIG__=21 __ELF__=1 _FILE_OFFSET_BITS=64 __FINITE_MATH_ONLY__=0 __FLT_DENORM_MIN__=1.40129846e-45F __FLT_DIG__=6 __FLT_EPSILON__=1.19209290e-7F __FLT_EVAL_METHOD__=2 __FLT_HAS_INFINITY__=1 __FLT_HAS_QUIET_NAN__=1 __FLT_MANT_DIG__=24 __FLT_MAX_10_EXP__=38 __FLT_MAX__=3.40282347e+38F __FLT_MAX_EXP__=128 __FLT_MIN_10_EXP__=(-37) __FLT_MIN__=1.17549435e-38F __FLT_MIN_EXP__=(-125) __FLT_RADIX__=2 __GLIBC__=2 __GLIBC_MINOR__=5 __GNUC__=4 __GNUC_MINOR__=1 __GNUC_PATCHLEVEL__=2 __GNU_LIBRARY__=6 __gnu_linux__=1 _GNU_SOURCE=1 __GXX_ABI_VERSION=1002 __i386=1 __i386__=1 i386=1 __INT_MAX__=2147483647 __INTMAX_MAX__=9223372036854775807LL __INTMAX_TYPE__=long\ long\ int _LARGEFILE64_SOURCE=1 _LARGEFILE_SOURCE=1 __LDBL_DENORM_MIN__=3.64519953188247460253e-4951L __LDBL_DIG__=18 __LDBL_EPSILON__=1.08420217248550443401e-19L __LDBL_HAS_INFINITY__=1 __LDBL_HAS_QUIET_NAN__=1 __LDBL_MANT_DIG__=64 __LDBL_MAX_10_EXP__=4932 __LDBL_MAX__=1.18973149535723176502e+4932L __LDBL_MAX_EXP__=16384 __LDBL_MIN_10_EXP__=(-4931) __LDBL_MIN__=3.36210314311209350626e-4932L __LDBL_MIN_EXP__=(-16381) __linux=1 __linux__=1 linux=1 __LONG_LONG_MAX__=9223372036854775807LL __LONG_MAX__=2147483647L _POSIX_C_SOURCE=200112L _POSIX_SOURCE=1 __PTRDIFF_TYPE__=int _REENTRANT=1 __REGISTER_PREFIX__= __SCHAR_MAX__=127 __SHRT_MAX__=32767 __SIZE_TYPE__=unsigned\ int __STDC__=1 __STDC_HOSTED__=1 __UINTMAX_TYPE__=long\ long\ unsigned\ int __unix=1 __unix__=1 unix=1 __USE_BSD=1 __USE_FILE_OFFSET64=1 __USE_GNU=1 __USE_LARGEFILE=1 __USE_LARGEFILE64=1 __USE_MISC=1 __USE_POSIX=1 __USE_POSIX199309=1 __USE_POSIX199506=1 __USE_POSIX2=1 __USE_REENTRANT=1 __USER_LABEL_PREFIX__= __USE_SVID=1 __USE_UNIX98=1 __USE_XOPEN=1 __USE_XOPEN_EXTENDED=1 __VERSION__="4.1.2\ 20061115\ (prerelease)\ (SUSE\ Linux)" __WCHAR_MAX__=2147483647 __WCHAR_TYPE__=long\ int __WINT_TYPE__=unsigned\ int _XOPEN_SOURCE=600 _XOPEN_SOURCE_EXTENDED=1'
crypt_r_proto='REENTRANT_PROTO_B_CCS'
cryptlib=''
csh='tcsh'
ctermid_r_proto='0'
ctime_r_proto='REENTRANT_PROTO_B_SB'
d_Gconvert='qgcvt((x),(n),(b))'
d_PRIEUldbl='define'
d_PRIFUldbl='define'
d_PRIGUldbl='define'
d_PRIXU64='define'
d_PRId64='define'
d_PRIeldbl='define'
d_PRIfldbl='define'
d_PRIgldbl='define'
d_PRIi64='define'
d_PRIo64='define'
d_PRIu64='define'
d_PRIx64='define'
d_SCNfldbl='define'
d__fwalk='undef'
d_access='define'
d_accessx='undef'
d_aintl='undef'
d_alarm='define'
d_archlib='define'
d_asctime_r='define'
d_atolf='undef'
d_atoll='define'
d_attribute_format='define'
d_attribute_malloc='define'
d_attribute_nonnull='define'
d_attribute_noreturn='define'
d_attribute_pure='define'
d_attribute_unused='define'
d_attribute_warn_unused_result='define'
d_bcmp='define'
d_bcopy='define'
d_bsd='undef'
d_bsdgetpgrp='undef'
d_bsdsetpgrp='undef'
d_builtin_choose_expr='define'
d_builtin_expect='undef'
d_bzero='define'
d_c99_variadic_macros='define'
d_casti32='undef'
d_castneg='define'
d_charvspr='undef'
d_chown='define'
d_chroot='define'
d_chsize='undef'
d_class='undef'
d_clearenv='define'
d_closedir='define'
d_cmsghdr_s='define'
d_const='define'
d_copysignl='define'
d_cplusplus='undef'
d_crypt='define'
d_crypt_r='define'
d_csh='define'
d_ctermid='define'
d_ctermid_r='undef'
d_ctime_r='define'
d_cuserid='define'
d_dbl_dig='define'
d_dbminitproto='undef'
d_difftime='define'
d_dir_dd_fd='undef'
d_dirfd='define'
d_dirnamlen='undef'
d_dlerror='define'
d_dlopen='define'
d_dlsymun='undef'
d_dosuid='undef'
d_drand48_r='define'
d_drand48proto='define'
d_dup2='define'
d_eaccess='define'
d_endgrent='define'
d_endgrent_r='undef'
d_endhent='define'
d_endhostent_r='undef'
d_endnent='define'
d_endnetent_r='undef'
d_endpent='define'
d_endprotoent_r='undef'
d_endpwent='define'
d_endpwent_r='undef'
d_endsent='define'
d_endservent_r='undef'
d_eofnblk='define'
d_eunice='undef'
d_faststdio='undef'
d_fchdir='define'
d_fchmod='define'
d_fchown='define'
d_fcntl='define'
d_fcntl_can_lock='define'
d_fd_macros='define'
d_fd_set='define'
d_fds_bits='define'
d_fgetpos='define'
d_finite='define'
d_finitel='define'
d_flexfnam='define'
d_flock='define'
d_flockproto='define'
d_fork='define'
d_fp_class='undef'
d_fpathconf='define'
d_fpclass='undef'
d_fpclassify='undef'
d_fpclassl='undef'
d_fpos64_t='undef'
d_frexpl='define'
d_fs_data_s='undef'
d_fseeko='define'
d_fsetpos='define'
d_fstatfs='define'
d_fstatvfs='define'
d_fsync='define'
d_ftello='define'
d_ftime='undef'
d_futimes='define'
d_getcwd='define'
d_getespwnam='undef'
d_getfsstat='undef'
d_getgrent='define'
d_getgrent_r='define'
d_getgrgid_r='define'
d_getgrnam_r='define'
d_getgrps='define'
d_gethbyaddr='define'
d_gethbyname='define'
d_gethent='define'
d_gethname='define'
d_gethostbyaddr_r='define'
d_gethostbyname_r='define'
d_gethostent_r='define'
d_gethostprotos='define'
d_getitimer='define'
d_getlogin='define'
d_getlogin_r='define'
d_getmnt='undef'
d_getmntent='define'
d_getnbyaddr='define'
d_getnbyname='define'
d_getnent='define'
d_getnetbyaddr_r='define'
d_getnetbyname_r='define'
d_getnetent_r='define'
d_getnetprotos='define'
d_getpagsz='define'
d_getpbyname='define'
d_getpbynumber='define'
d_getpent='define'
d_getpgid='define'
d_getpgrp2='undef'
d_getpgrp='define'
d_getppid='define'
d_getprior='define'
d_getprotobyname_r='define'
d_getprotobynumber_r='define'
d_getprotoent_r='define'
d_getprotoprotos='define'
d_getprpwnam='undef'
d_getpwent='define'
d_getpwent_r='define'
d_getpwnam_r='define'
d_getpwuid_r='define'
d_getsbyname='define'
d_getsbyport='define'
d_getsent='define'
d_getservbyname_r='define'
d_getservbyport_r='define'
d_getservent_r='define'
d_getservprotos='define'
d_getspnam='define'
d_getspnam_r='define'
d_gettimeod='define'
d_gmtime_r='define'
d_gnulibc='define'
d_grpasswd='define'
d_hasmntopt='define'
d_htonl='define'
d_ilogbl='define'
d_inc_version_list='define'
d_index='undef'
d_inetaton='define'
d_int64_t='define'
d_isascii='define'
d_isfinite='undef'
d_isinf='define'
d_isnan='define'
d_isnanl='define'
d_killpg='define'
d_lchown='define'
d_ldbl_dig='define'
d_libm_lib_version='define'
d_link='define'
d_localtime_r='define'
d_localtime_r_needs_tzset='define'
d_locconv='define'
d_lockf='define'
d_longdbl='define'
d_longlong='define'
d_lseekproto='define'
d_lstat='define'
d_madvise='define'
d_malloc_good_size='undef'
d_malloc_size='undef'
d_mblen='define'
d_mbstowcs='define'
d_mbtowc='define'
d_memchr='define'
d_memcmp='define'
d_memcpy='define'
d_memmove='define'
d_memset='define'
d_mkdir='define'
d_mkdtemp='define'
d_mkfifo='define'
d_mkstemp='define'
d_mkstemps='undef'
d_mktime='define'
d_mmap='define'
d_modfl='define'
d_modfl_pow32_bug='undef'
d_modflproto='define'
d_mprotect='define'
d_msg='define'
d_msg_ctrunc='define'
d_msg_dontroute='define'
d_msg_oob='define'
d_msg_peek='define'
d_msg_proxy='define'
d_msgctl='define'
d_msgget='define'
d_msghdr_s='define'
d_msgrcv='define'
d_msgsnd='define'
d_msync='define'
d_munmap='define'
d_mymalloc='undef'
d_nice='define'
d_nl_langinfo='define'
d_nv_preserves_uv='define'
d_nv_zero_is_allbits_zero='undef'
d_off64_t='define'
d_old_pthread_create_joinable='undef'
d_oldpthreads='undef'
d_oldsock='undef'
d_open3='define'
d_pathconf='define'
d_pause='define'
d_perl_otherlibdirs='undef'
d_phostname='undef'
d_pipe='define'
d_poll='define'
d_portable='define'
d_printf_format_null='define'
d_procselfexe='define'
d_pseudofork='undef'
d_pthread_atfork='define'
d_pthread_attr_setscope='define'
d_pthread_yield='define'
d_pwage='undef'
d_pwchange='undef'
d_pwclass='undef'
d_pwcomment='undef'
d_pwexpire='undef'
d_pwgecos='define'
d_pwpasswd='define'
d_pwquota='undef'
d_qgcvt='define'
d_quad='define'
d_random_r='define'
d_readdir64_r='define'
d_readdir='define'
d_readdir_r='define'
d_readlink='define'
d_readv='define'
d_recvmsg='define'
d_rename='define'
d_rewinddir='define'
d_rmdir='define'
d_safebcpy='undef'
d_safemcpy='undef'
d_sanemcmp='define'
d_sbrkproto='define'
d_scalbnl='define'
d_sched_yield='define'
d_scm_rights='define'
d_seekdir='define'
d_select='define'
d_sem='define'
d_semctl='define'
d_semctl_semid_ds='define'
d_semctl_semun='define'
d_semget='define'
d_semop='define'
d_sendmsg='define'
d_setegid='define'
d_seteuid='define'
d_setgrent='define'
d_setgrent_r='undef'
d_setgrps='define'
d_sethent='define'
d_sethostent_r='undef'
d_setitimer='define'
d_setlinebuf='define'
d_setlocale='define'
d_setlocale_r='undef'
d_setnent='define'
d_setnetent_r='undef'
d_setpent='define'
d_setpgid='define'
d_setpgrp2='undef'
d_setpgrp='define'
d_setprior='define'
d_setproctitle='undef'
d_setprotoent_r='undef'
d_setpwent='define'
d_setpwent_r='undef'
d_setregid='define'
d_setresgid='define'
d_setresuid='define'
d_setreuid='define'
d_setrgid='undef'
d_setruid='undef'
d_setsent='define'
d_setservent_r='undef'
d_setsid='define'
d_setvbuf='define'
d_sfio='undef'
d_shm='define'
d_shmat='define'
d_shmatprototype='define'
d_shmctl='define'
d_shmdt='define'
d_shmget='define'
d_sigaction='define'
d_signbit='define'
d_sigprocmask='define'
d_sigsetjmp='define'
d_sitearch='define'
d_snprintf='define'
d_sockatmark='define'
d_sockatmarkproto='define'
d_socket='define'
d_socklen_t='define'
d_sockpair='define'
d_socks5_init='undef'
d_sprintf_returns_strlen='define'
d_sqrtl='define'
d_srand48_r='define'
d_srandom_r='define'
d_sresgproto='define'
d_sresuproto='define'
d_statblks='define'
d_statfs_f_flags='undef'
d_statfs_s='define'
d_statvfs='define'
d_stdio_cnt_lval='undef'
d_stdio_ptr_lval='undef'
d_stdio_ptr_lval_nochange_cnt='undef'
d_stdio_ptr_lval_sets_cnt='undef'
d_stdio_stream_array='undef'
d_stdiobase='undef'
d_stdstdio='undef'
d_strchr='define'
d_strcoll='define'
d_strctcpy='define'
d_strerrm='strerror(e)'
d_strerror='define'
d_strerror_r='define'
d_strftime='define'
d_strlcat='undef'
d_strlcpy='undef'
d_strtod='define'
d_strtol='define'
d_strtold='define'
d_strtoll='define'
d_strtoq='define'
d_strtoul='define'
d_strtoull='define'
d_strtouq='define'
d_strxfrm='define'
d_suidsafe='undef'
d_symlink='define'
d_syscall='define'
d_syscallproto='define'
d_sysconf='define'
d_sysernlst=''
d_syserrlst='define'
d_system='define'
d_tcgetpgrp='define'
d_tcsetpgrp='define'
d_telldir='define'
d_telldirproto='define'
d_time='define'
d_times='define'
d_tm_tm_gmtoff='define'
d_tm_tm_zone='define'
d_tmpnam_r='define'
d_truncate='define'
d_ttyname_r='define'
d_tzname='define'
d_u32align='define'
d_ualarm='define'
d_umask='define'
d_uname='define'
d_union_semun='undef'
d_unordered='undef'
d_unsetenv='define'
d_usleep='define'
d_usleepproto='define'
d_ustat='define'
d_vendorarch='undef'
d_vendorbin='undef'
d_vendorlib='undef'
d_vendorscript='undef'
d_vfork='undef'
d_void_closedir='undef'
d_voidsig='define'
d_voidtty=''
d_volatile='define'
d_vprintf='define'
d_vsnprintf='define'
d_wait4='define'
d_waitpid='define'
d_wcstombs='define'
d_wctomb='define'
d_writev='define'
d_xenix='undef'
date='date'
db_hashtype='u_int32_t'
db_prefixtype='size_t'
db_version_major='4'
db_version_minor='4'
db_version_patch='20'
defvoidused='15'
direntrytype='struct dirent'
dlext='so'
dlsrc='dl_dlopen.xs'
doublesize='8'
drand01='drand48()'
drand48_r_proto='REENTRANT_PROTO_I_ST'
dynamic_ext='attrs B Compress/Raw/Zlib Cwd Data/Dumper DB_File Devel/DProf Devel/Peek Devel/PPPort Digest/MD5 Digest/SHA Encode Fcntl File/Glob Filter/Util/Call GDBM_File Hash/Util I18N/Langinfo IO IPC/SysV List/Util Math/BigInt/FastCalc MIME/Base64 Opcode PerlIO/encoding PerlIO/scalar PerlIO/via POSIX re SDBM_File Socket Storable Sys/Hostname Sys/Syslog Text/Soundex threads Time/HiRes Time/Piece Unicode/Normalize XS/APItest XS/Typemap threads/shared'
eagain='EAGAIN'
ebcdic='undef'
echo='echo'
egrep='egrep'
emacs=''
endgrent_r_proto='0'
endhostent_r_proto='0'
endnetent_r_proto='0'
endprotoent_r_proto='0'
endpwent_r_proto='0'
endservent_r_proto='0'
eunicefix=':'
exe_ext=''
expr='expr'
extensions='attrs B Compress/Raw/Zlib Cwd Data/Dumper DB_File Devel/DProf Devel/Peek Devel/PPPort Digest/MD5 Digest/SHA Encode Fcntl File/Glob Filter/Util/Call GDBM_File Hash/Util I18N/Langinfo IO IPC/SysV List/Util Math/BigInt/FastCalc MIME/Base64 Opcode PerlIO/encoding PerlIO/scalar PerlIO/via POSIX re SDBM_File Socket Storable Sys/Hostname Sys/Syslog Text/Soundex threads Time/HiRes Time/Piece Unicode/Normalize XS/APItest XS/Typemap threads/shared Compress/Zlib Errno IO_Compress_Base IO_Compress_Zlib'
extras=''
fflushNULL='define'
fflushall='undef'
find=''
firstmakefile='makefile'
flex=''
fpossize='16'
fpostype='fpos_t'
freetype='void'
from=':'
full_ar='/usr/bin/ar'
full_csh='/usr/bin/tcsh'
full_sed='/usr/bin/sed'
gccansipedantic=''
gccosandvers=''
gccversion='4.1.2 20061115 (prerelease) (SUSE Linux)'
getgrent_r_proto='REENTRANT_PROTO_I_SBWR'
getgrgid_r_proto='REENTRANT_PROTO_I_TSBWR'
getgrnam_r_proto='REENTRANT_PROTO_I_CSBWR'
gethostbyaddr_r_proto='REENTRANT_PROTO_I_TsISBWRE'
gethostbyname_r_proto='REENTRANT_PROTO_I_CSBWRE'
gethostent_r_proto='REENTRANT_PROTO_I_SBWRE'
getlogin_r_proto='REENTRANT_PROTO_I_BW'
getnetbyaddr_r_proto='REENTRANT_PROTO_I_uISBWRE'
getnetbyname_r_proto='REENTRANT_PROTO_I_CSBWRE'
getnetent_r_proto='REENTRANT_PROTO_I_SBWRE'
getprotobyname_r_proto='REENTRANT_PROTO_I_CSBWR'
getprotobynumber_r_proto='REENTRANT_PROTO_I_ISBWR'
getprotoent_r_proto='REENTRANT_PROTO_I_SBWR'
getpwent_r_proto='REENTRANT_PROTO_I_SBWR'
getpwnam_r_proto='REENTRANT_PROTO_I_CSBWR'
getpwuid_r_proto='REENTRANT_PROTO_I_TSBWR'
getservbyname_r_proto='REENTRANT_PROTO_I_CCSBWR'
getservbyport_r_proto='REENTRANT_PROTO_I_ICSBWR'
getservent_r_proto='REENTRANT_PROTO_I_SBWR'
getspnam_r_proto='REENTRANT_PROTO_I_CSBWR'
gidformat='"lu"'
gidsign='1'
gidsize='4'
gidtype='gid_t'
glibpth='/usr/shlib  /lib /usr/lib /usr/lib/386 /lib/386 /usr/ccs/lib /usr/ucblib /usr/local/lib '
gmake='gmake'
gmtime_r_proto='REENTRANT_PROTO_S_TS'
gnulibc_version='2.5'
grep='grep'
groupcat='cat /etc/group'
groupstype='gid_t'
gzip='gzip'
h_fcntl='false'
h_sysfile='true'
hint='recommended'
hostcat='cat /etc/hosts'
html1dir=' '
html1direxp=''
html3dir=' '
html3direxp=''
i16size='2'
i16type='short'
i32size='4'
i32type='long'
i64size='8'
i64type='long long'
i8size='1'
i8type='char'
i_arpainet='define'
i_bsdioctl=''
i_crypt='define'
i_db='define'
i_dbm='undef'
i_dirent='define'
i_dld='undef'
i_dlfcn='define'
i_fcntl='undef'
i_float='define'
i_fp='undef'
i_fp_class='undef'
i_gdbm='define'
i_grp='define'
i_ieeefp='undef'
i_inttypes='define'
i_langinfo='define'
i_libutil='undef'
i_limits='define'
i_locale='define'
i_machcthr='undef'
i_malloc='define'
i_math='define'
i_memory='undef'
i_mntent='define'
i_ndbm='undef'
i_netdb='define'
i_neterrno='undef'
i_netinettcp='define'
i_niin='define'
i_poll='define'
i_prot='undef'
i_pthread='define'
i_pwd='define'
i_rpcsvcdbm='undef'
i_sfio='undef'
i_sgtty='undef'
i_shadow='define'
i_socks='undef'
i_stdarg='define'
i_stddef='define'
i_stdlib='define'
i_string='define'
i_sunmath='undef'
i_sysaccess='undef'
i_sysdir='define'
i_sysfile='define'
i_sysfilio='undef'
i_sysin='undef'
i_sysioctl='define'
i_syslog='define'
i_sysmman='define'
i_sysmode='undef'
i_sysmount='define'
i_sysndir='undef'
i_sysparam='define'
i_sysresrc='define'
i_syssecrt='undef'
i_sysselct='define'
i_syssockio='undef'
i_sysstat='define'
i_sysstatfs='define'
i_sysstatvfs='define'
i_systime='define'
i_systimek='undef'
i_systimes='define'
i_systypes='define'
i_sysuio='define'
i_sysun='define'
i_sysutsname='define'
i_sysvfs='define'
i_syswait='define'
i_termio='undef'
i_termios='define'
i_time='define'
i_unistd='define'
i_ustat='define'
i_utime='define'
i_values='define'
i_varargs='undef'
i_varhdr='stdarg.h'
i_vfork='undef'
ignore_versioned_solibs='y'
inc_version_list=''
inc_version_list_init='0'
incpath=''
inews=''
initialinstalllocation='/pro/bin'
installarchlib='/pro/lib/perl5/5.10.0/i686-linux-thread-multi-64int-ld'
installbin='/pro/bin'
installhtml1dir=''
installhtml3dir=''
installman1dir='/pro/local/man/man1'
installman3dir='/pro/local/man/man3'
installprefix='/pro'
installprefixexp='/pro'
installprivlib='/pro/lib/perl5/5.10.0'
installscript='/pro/bin'
installsitearch='/pro/lib/perl5/site_perl/5.10.0/i686-linux-thread-multi-64int-ld'
installsitebin='/pro/bin'
installsitehtml1dir=''
installsitehtml3dir=''
installsitelib='/pro/lib/perl5/site_perl/5.10.0'
installsiteman1dir='/pro/local/man/man1'
installsiteman3dir='/pro/local/man/man3'
installsitescript='/pro/bin'
installstyle='lib/perl5'
installusrbinperl='undef'
installvendorarch=''
installvendorbin=''
installvendorhtml1dir=''
installvendorhtml3dir=''
installvendorlib=''
installvendorman1dir=''
installvendorman3dir=''
installvendorscript=''
intsize='4'
issymlink='test -h'
ivdformat='"Ld"'
ivsize='8'
ivtype='long long'
known_extensions='attrs B Compress/Raw/Zlib Cwd Data/Dumper DB_File Devel/DProf Devel/Peek Devel/PPPort Digest/MD5 Digest/SHA Encode Fcntl File/Glob Filter/Util/Call GDBM_File Hash/Util I18N/Langinfo IO IPC/SysV List/Util Math/BigInt/FastCalc MIME/Base64 NDBM_File ODBM_File Opcode PerlIO/encoding PerlIO/scalar PerlIO/via POSIX re SDBM_File Socket Storable Sys/Hostname Sys/Syslog Text/Soundex threads Time/HiRes Time/Piece Unicode/Normalize Win32 Win32API/File Win32CORE XS/APItest XS/Typemap threads/shared'
ksh=''
ld='cc'
lddlflags='-shared -O2 -g -L/pro/local/lib'
ldflags='-L/pro/local/lib'
ldflags_uselargefiles=''
ldlibpthname='LD_LIBRARY_PATH'
less='less'
lib_ext='.a'
libc='/lib/libc-2.5.so'
libperl='libperl.a'
libpth='/pro/local/lib /lib /usr/lib /usr/local/lib'
libs='-lnsl -lgdbm -ldb -ldl -lm -lcrypt -lutil -lpthread -lc'
libsdirs=' /usr/lib'
libsfiles=' libnsl.so libgdbm.so libdb.so libdl.so libm.so libcrypt.so libutil.so libpthread.so libc.so'
libsfound=' /usr/lib/libnsl.so /usr/lib/libgdbm.so /usr/lib/libdb.so /usr/lib/libdl.so /usr/lib/libm.so /usr/lib/libcrypt.so /usr/lib/libutil.so /usr/lib/libpthread.so /usr/lib/libc.so'
libspath=' /pro/local/lib /lib /usr/lib /usr/local/lib'
libswanted='sfio socket inet nsl nm gdbm dbm db malloc dl dld ld sun m crypt sec util pthread c cposix posix ucb BSD'
libswanted_uselargefiles=''
line=''
lint=''
lkflags=''
ln='ln'
lns='/bin/ln -s'
localtime_r_proto='REENTRANT_PROTO_S_TS'
locincpth='/pro/local/include'
loclibpth='/pro/local/lib'
longdblsize='12'
longlongsize='8'
longsize='4'
lp=''
lpr=''
ls='ls'
lseeksize='8'
lseektype='off_t'
mad='undef'
madlyh=''
madlyobj=''
madlysrc=''
mail=''
mailx=''
make='make'
make_set_make='#'
mallocobj=''
mallocsrc=''
malloctype='void *'
man1dir='/pro/local/man/man1'
man1direxp='/pro/local/man/man1'
man1ext='1'
man3dir='/pro/local/man/man3'
man3direxp='/pro/local/man/man3'
man3ext='3'
mips_type=''
mistrustnm=''
mkdir='mkdir'
mmaptype='void *'
modetype='mode_t'
more='more'
multiarch='undef'
mv=''
myarchname='i686-linux'
mydomain='.procura.nl'
myhostname='nb09'
myuname='linux nb09 2.6.18.8-0.5-default #1 smp fri jun 22 12:17:53 utc 2007 i686 i686 i386 gnulinux '
n='-n'
need_va_copy='undef'
netdb_hlen_type='size_t'
netdb_host_type='const void *'
netdb_name_type='const char *'
netdb_net_type='in_addr_t'
nm='nm'
nm_opt=''
nm_so_opt='--dynamic'
nonxs_ext='Compress/Zlib Errno IO_Compress_Base IO_Compress_Zlib'
nroff='nroff'
nvEUformat='"LE"'
nvFUformat='"LF"'
nvGUformat='"LG"'
nv_preserves_uv_bits='64'
nveformat='"Le"'
nvfformat='"Lf"'
nvgformat='"Lg"'
nvsize='12'
nvtype='long double'
o_nonblock='O_NONBLOCK'
obj_ext='.o'
old_pthread_create_joinable=''
optimize='-O2 -g'
orderlib='false'
osname='linux'
osvers='2.6.18.8-0.5-default'
otherlibdirs=' '
package='perl5'
pager='/pro/local/bin/less'
passcat='cat /etc/passwd'
patchlevel='10'
path_sep=':'
perl5='/pro/bin/perl'
perl=''
perl_patchlevel='31801'
perladmin='hmbrand@cpan.org'
perllibs='-lnsl -ldl -lm -lcrypt -lutil -lpthread -lc'
perlpath='/pro/bin/perl5.10.0'
pg='pg'
phostname='hostname'
pidtype='pid_t'
plibpth=''
pmake=''
pr=''
prefix='/pro'
prefixexp='/pro'
privlib='/pro/lib/perl5/5.10.0'
privlibexp='/pro/lib/perl5/5.10.0'
procselfexe='"/proc/self/exe"'
prototype='define'
ptrsize='4'
quadkind='3'
quadtype='long long'
randbits='48'
randfunc='drand48'
random_r_proto='REENTRANT_PROTO_I_St'
randseedtype='long'
ranlib=':'
rd_nodata='-1'
readdir64_r_proto='REENTRANT_PROTO_I_TSR'
readdir_r_proto='REENTRANT_PROTO_I_TSR'
revision='5'
rm='rm'
rm_try='/bin/rm -f try try a.out .out try.[cho] try..o core core.try* try.core*'
rmail=''
run=''
runnm='false'
sPRIEUldbl='"LE"'
sPRIFUldbl='"LF"'
sPRIGUldbl='"LG"'
sPRIXU64='"LX"'
sPRId64='"Ld"'
sPRIeldbl='"Le"'
sPRIfldbl='"Lf"'
sPRIgldbl='"Lg"'
sPRIi64='"Li"'
sPRIo64='"Lo"'
sPRIu64='"Lu"'
sPRIx64='"Lx"'
sSCNfldbl='"Lf"'
sched_yield='sched_yield()'
scriptdir='/pro/bin'
scriptdirexp='/pro/bin'
sed='sed'
seedfunc='srand48'
selectminbits='32'
selecttype='fd_set *'
sendmail=''
setgrent_r_proto='0'
sethostent_r_proto='0'
setlocale_r_proto='0'
setnetent_r_proto='0'
setprotoent_r_proto='0'
setpwent_r_proto='0'
setservent_r_proto='0'
sh='/bin/sh'
shar=''
sharpbang='#!'
shmattype='void *'
shortsize='2'
shrpenv=''
shsharp='true'
sig_count='65'
sig_name='ZERO HUP INT QUIT ILL TRAP ABRT BUS FPE KILL USR1 SEGV USR2 PIPE ALRM TERM STKFLT CHLD CONT STOP TSTP TTIN TTOU URG XCPU XFSZ VTALRM PROF WINCH IO PWR SYS NUM32 NUM33 RTMIN NUM35 NUM36 NUM37 NUM38 NUM39 NUM40 NUM41 NUM42 NUM43 NUM44 NUM45 NUM46 NUM47 NUM48 NUM49 NUM50 NUM51 NUM52 NUM53 NUM54 NUM55 NUM56 NUM57 NUM58 NUM59 NUM60 NUM61 NUM62 NUM63 RTMAX IOT CLD POLL UNUSED '
sig_name_init='"ZERO", "HUP", "INT", "QUIT", "ILL", "TRAP", "ABRT", "BUS", "FPE", "KILL", "USR1", "SEGV", "USR2", "PIPE", "ALRM", "TERM", "STKFLT", "CHLD", "CONT", "STOP", "TSTP", "TTIN", "TTOU", "URG", "XCPU", "XFSZ", "VTALRM", "PROF", "WINCH", "IO", "PWR", "SYS", "NUM32", "NUM33", "RTMIN", "NUM35", "NUM36", "NUM37", "NUM38", "NUM39", "NUM40", "NUM41", "NUM42", "NUM43", "NUM44", "NUM45", "NUM46", "NUM47", "NUM48", "NUM49", "NUM50", "NUM51", "NUM52", "NUM53", "NUM54", "NUM55", "NUM56", "NUM57", "NUM58", "NUM59", "NUM60", "NUM61", "NUM62", "NUM63", "RTMAX", "IOT", "CLD", "POLL", "UNUSED", 0'
sig_num='0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 6 17 29 31 '
sig_num_init='0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 6, 17, 29, 31, 0'
sig_size='69'
signal_t='void'
sitearch='/pro/lib/perl5/site_perl/5.10.0/i686-linux-thread-multi-64int-ld'
sitearchexp='/pro/lib/perl5/site_perl/5.10.0/i686-linux-thread-multi-64int-ld'
sitebin='/pro/bin'
sitebinexp='/pro/bin'
sitehtml1dir=''
sitehtml1direxp=''
sitehtml3dir=''
sitehtml3direxp=''
sitelib='/pro/lib/perl5/site_perl/5.10.0'
sitelib_stem='/pro/lib/perl5/site_perl'
sitelibexp='/pro/lib/perl5/site_perl/5.10.0'
siteman1dir='/pro/local/man/man1'
siteman1direxp='/pro/local/man/man1'
siteman3dir='/pro/local/man/man3'
siteman3direxp='/pro/local/man/man3'
siteprefix='/pro'
siteprefixexp='/pro'
sitescript='/pro/bin'
sitescriptexp='/pro/bin'
sizesize='4'
sizetype='size_t'
sleep=''
smail=''
so='so'
sockethdr=''
socketlib=''
socksizetype='socklen_t'
sort='sort'
spackage='Perl5'
spitshell='cat'
srand48_r_proto='REENTRANT_PROTO_I_LS'
srandom_r_proto='REENTRANT_PROTO_I_TS'
src='.'
ssizetype='ssize_t'
startperl='#!/pro/bin/perl5.10.0'
startsh='#!/bin/sh'
static_ext=' '
stdchar='char'
stdio_base='((fp)->_IO_read_base)'
stdio_bufsiz='((fp)->_IO_read_end - (fp)->_IO_read_base)'
stdio_cnt='((fp)->_IO_read_end - (fp)->_IO_read_ptr)'
stdio_filbuf=''
stdio_ptr='((fp)->_IO_read_ptr)'
stdio_stream_array=''
strerror_r_proto='REENTRANT_PROTO_B_IBW'
strings='/usr/include/string.h'
submit=''
subversion='0'
sysman='/usr/share/man/man1'
tail=''
tar=''
targetarch=''
tbl=''
tee=''
test='test'
timeincl='/usr/include/sys/time.h /usr/include/time.h '
timetype='time_t'
tmpnam_r_proto='REENTRANT_PROTO_B_B'
to=':'
touch='touch'
tr='tr'
trnl='\n'
troff=''
ttyname_r_proto='REENTRANT_PROTO_I_IBW'
u16size='2'
u16type='unsigned short'
u32size='4'
u32type='unsigned long'
u64size='8'
u64type='unsigned long long'
u8size='1'
u8type='unsigned char'
uidformat='"lu"'
uidsign='1'
uidsize='4'
uidtype='uid_t'
uname='uname'
uniq='uniq'
uquadtype='unsigned long long'
use5005threads='undef'
use64bitall='undef'
use64bitint='define'
usecrosscompile='undef'
usedl='define'
usefaststdio='undef'
useithreads='define'
uselargefiles='define'
uselongdouble='define'
usemallocwrap='define'
usemorebits='define'
usemultiplicity='define'
usemymalloc='n'
usenm='false'
useopcode='true'
useperlio='define'
useposix='true'
usereentrant='undef'
userelocatableinc='undef'
usesfio='false'
useshrplib='false'
usesitecustomize='undef'
usesocks='undef'
usethreads='define'
usevendorprefix='undef'
usevfork='false'
usrinc='/usr/include'
uuname=''
uvXUformat='"LX"'
uvoformat='"Lo"'
uvsize='8'
uvtype='unsigned long long'
uvuformat='"Lu"'
uvxformat='"Lx"'
vendorarch=''
vendorarchexp=''
vendorbin=''
vendorbinexp=''
vendorhtml1dir=' '
vendorhtml1direxp=''
vendorhtml3dir=' '
vendorhtml3direxp=''
vendorlib=''
vendorlib_stem=''
vendorlibexp=''
vendorman1dir=' '
vendorman1direxp=''
vendorman3dir=' '
vendorman3direxp=''
vendorprefix=''
vendorprefixexp=''
vendorscript=''
vendorscriptexp=''
version='5.10.0'
version_patchlevel_string='version 10 subversion 0 patch 31801'
versiononly='define'
vi=''
voidflags='15'
xlibpth='/usr/lib/386 /lib/386'
yacc='/pro/3gl/CPAN/bin/byacc'
yaccflags=''
zcat=''
zip='zip'
# Configure command line arguments.
config_arg0='./Configure'
config_args='-Dusedevel -Duseithreads -Dusemorebits -DDEBUGGING -des'
config_argc=5
config_arg1='-Dusedevel'
config_arg2='-Duseithreads'
config_arg3='-Dusemorebits'
config_arg4='-DDEBUGGING'
config_arg5='-des'
PERL_REVISION=5
PERL_VERSION=10
PERL_SUBVERSION=0
PERL_API_REVISION=5
PERL_API_VERSION=10
PERL_API_SUBVERSION=0
PERL_PATCHLEVEL=31801
PERL_CONFIG_SH=true