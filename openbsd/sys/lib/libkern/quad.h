/*	$OpenBSD: quad.h,v 1.5 1997/07/25 18:25:38 mickey Exp $	*/
/*	$NetBSD: quad.h,v 1.7 1996/04/18 02:20:04 cgd Exp $	*/

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)quad.h	8.1 (Berkeley) 6/4/93
 */

/*
 * Quad arithmetic.
 *
 * This library makes the following assumptions:
 *
 *  - The type long long (aka quad_t) exists.
 *
 *  - A quad variable is exactly twice as long as `long'.
 *
 *  - The machine's arithmetic is two's complement.
 *
 * This library can provide 128-bit arithmetic on a machine with 128-bit
 * quads and 64-bit longs, for instance, or 96-bit arithmetic on machines
 * with 48-bit longs.
 */

#include <sys/types.h>
#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <limits.h>
#else
#include <machine/limits.h>
#endif

/*
 * Depending on the desired operation, we view a `long long' (aka quad_t) in
 * one or more of the following formats.
 */
union uu {
	quad_t	q;		/* as a (signed) quad */
	u_quad_t uq;		/* as an unsigned quad */
	long	sl[2];		/* as two signed longs */
	u_long	ul[2];		/* as two unsigned longs */
};

/*
 * Define high and low longwords.
 */
#define	H		_QUAD_HIGHWORD
#define	L		_QUAD_LOWWORD

/*
 * Total number of bits in a quad_t and in the pieces that make it up.
 * These are used for shifting, and also below for halfword extraction
 * and assembly.
 */
#define	QUAD_BITS	(sizeof(quad_t) * CHAR_BIT)
#define	LONG_BITS	(sizeof(long) * CHAR_BIT)
#define	HALF_BITS	(sizeof(long) * CHAR_BIT / 2)

/*
 * Extract high and low shortwords from longword, and move low shortword of
 * longword to upper half of long, i.e., produce the upper longword of
 * ((quad_t)(x) << (number_of_bits_in_long/2)).  (`x' must actually be u_long.)
 *
 * These are used in the multiply code, to split a longword into upper
 * and lower halves, and to reassemble a product as a quad_t, shifted left
 * (sizeof(long)*CHAR_BIT/2).
 */
#define	HHALF(x)	((u_long)(x) >> HALF_BITS)
#define	LHALF(x)	((u_long)(x) & (((long)1 << HALF_BITS) - 1))
#define	LHUP(x)		((u_long)(x) << HALF_BITS)

extern u_quad_t __qdivrem __P((u_quad_t u, u_quad_t v, u_quad_t *rem));

/*
 * XXX
 * Compensate for gcc 1 vs gcc 2.  Gcc 1 defines ?sh?di3's second argument
 * as u_quad_t, while gcc 2 correctly uses int.  Unfortunately, we still use
 * both compilers.
 */
#if __GNUC__ >= 2
typedef unsigned int	qshift_t;
#else
typedef u_quad_t	qshift_t;
#endif

__BEGIN_DECLS
quad_t	__adddi3	__P((quad_t, quad_t));
quad_t	__anddi3	__P((quad_t, quad_t));
quad_t	__ashldi3	__P((quad_t, qshift_t));
quad_t	__ashrdi3	__P((quad_t, qshift_t));
int	__cmpdi2	__P((quad_t, quad_t));
quad_t	__divdi3	__P((quad_t, quad_t));
quad_t	__iordi3	__P((quad_t, quad_t));
quad_t	__lshldi3	__P((quad_t, qshift_t));
quad_t	__lshrdi3	__P((quad_t, qshift_t));
quad_t	__moddi3	__P((quad_t, quad_t));
quad_t	__muldi3	__P((quad_t, quad_t));
quad_t	__negdi2	__P((quad_t));
quad_t	__one_cmpldi2	__P((quad_t));
quad_t	__subdi3	__P((quad_t, quad_t));
int	__ucmpdi2	__P((u_quad_t, u_quad_t));
u_quad_t __udivdi3	__P((u_quad_t, u_quad_t));
u_quad_t __umoddi3	__P((u_quad_t, u_quad_t));
quad_t	__xordi3	__P((quad_t, quad_t));
__END_DECLS