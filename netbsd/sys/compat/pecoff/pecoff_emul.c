/*	$NetBSD: pecoff_emul.c,v 1.1 2002/03/25 06:44:46 kent Exp $	*/

/*
 * Copyright (c) 2000 Masaru OKI
 * Copyright (c) 1994, 1995, 1998 Scott Bartram
 * Copyright (c) 1994 Adam Glass
 * Copyright (c) 1993, 1994 Christopher G. Demetriou
 * All rights reserved.
 *
 * from compat/ibcs2/ibcs2_exec.c
 * originally from kern/exec_ecoff.c
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Scott Bartram.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pecoff_emul.c,v 1.1 2002/03/25 06:44:46 kent Exp $");

/*#define DEBUG_PECOFF*/

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <sys/exec.h>
#include <sys/syscall.h>
#include <sys/signalvar.h>
#include <sys/resourcevar.h>
#include <sys/stat.h>

#include <sys/exec_coff.h>
#include <machine/coff_machdep.h>

#include <compat/pecoff/pecoff_exec.h>
#include <compat/pecoff/pecoff_util.h>
#include <compat/pecoff/pecoff_syscall.h>

extern struct sysent pecoff_sysent[];
#ifdef SYSCALL_DEBUG
extern const char * const pecoff_syscallnames[];
#endif
extern char sigcode[], esigcode[];
#ifdef __HAVE_SYSCALL_INTERN
void syscall_intern(struct proc *);
#else
void syscall(void);
#endif

const struct emul emul_pecoff = {
	"pecoff",
	"/emul/pecoff",
#ifndef __HAVE_MINIMAL_EMUL
	EMUL_HAS_SYS___syscall,
	0,
	PECOFF_SYS_syscall,
	PECOFF_SYS_MAXSYSCALL,
#endif
	pecoff_sysent,
#ifdef SYSCALL_DEBUG
	pecoff_syscallnames,
#else
	NULL,
#endif
	sendsig,
	trapsignal,
	sigcode,
	esigcode,
	setregs,
	NULL,
	NULL,
	NULL,
#ifdef __HAVE_SYSCALL_INTERN
	syscall_intern,
#else
	syscall,
#endif
};
