/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	Id: syscalls.isc,v 1.1 1995/10/10 07:59:26 swallace Exp 
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <i386/ibcs2/ibcs2_types.h>
#include <i386/ibcs2/ibcs2_signal.h>
#include <i386/ibcs2/ibcs2_statfs.h>
#include <i386/ibcs2/ibcs2_proto.h>
#include <i386/ibcs2/ibcs2_xenix.h>

#ifdef COMPAT_43
#define compat(n, name) n, (sy_call_t *)__CONCAT(o,name)
#else
#define compat(n, name) 0, (sy_call_t *)nosys
#endif

/* The casts are bogus but will do for now. */
struct sysent isc_sysent[] = {
	{ 0, (sy_call_t *)nosys },			/* 0 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 1 = isc_setostype */
	{ 2, (sy_call_t *)ibcs2_rename },		/* 2 = ibcs2_rename */
	{ 3, (sy_call_t *)ibcs2_sigaction },		/* 3 = ibcs2_sigaction */
	{ 3, (sy_call_t *)ibcs2_sigprocmask },		/* 4 = ibcs2_sigprocmask */
	{ 1, (sy_call_t *)ibcs2_sigpending },		/* 5 = ibcs2_sigpending */
	{ 2, (sy_call_t *)getgroups },			/* 6 = getgroups */
	{ 2, (sy_call_t *)setgroups },			/* 7 = setgroups */
	{ 2, (sy_call_t *)ibcs2_pathconf },		/* 8 = ibcs2_pathconf */
	{ 2, (sy_call_t *)ibcs2_fpathconf },		/* 9 = ibcs2_fpathconf */
	{ 0, (sy_call_t *)nosys },			/* 10 = nosys */
	{ 3, (sy_call_t *)ibcs2_wait },			/* 11 = ibcs2_wait */
	{ 0, (sy_call_t *)setsid },			/* 12 = setsid */
	{ 0, (sy_call_t *)getpid },			/* 13 = getpid */
	{ 0, (sy_call_t *)nosys },			/* 14 = isc_adduser */
	{ 0, (sy_call_t *)nosys },			/* 15 = isc_setuser */
	{ 1, (sy_call_t *)ibcs2_sysconf },		/* 16 = ibcs2_sysconf */
	{ 1, (sy_call_t *)ibcs2_sigsuspend },		/* 17 = ibcs2_sigsuspend */
	{ 2, (sy_call_t *)ibcs2_symlink },		/* 18 = ibcs2_symlink */
	{ 3, (sy_call_t *)ibcs2_readlink },		/* 19 = ibcs2_readlink */
	{ 0, (sy_call_t *)nosys },			/* 20 = isc_getmajor */
};


