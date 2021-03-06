/*	$NetBSD: ieeefp.h,v 1.1 1997/04/16 21:07:11 thorpej Exp $	*/

/* 
 * Written by J.T. Conklin, Apr 6, 1995
 * Public domain.
 */

#ifndef _POWERPC_IEEEFP_H_
#define _POWERPC_IEEEFP_H_

typedef int fp_except;
#define FP_X_IMP	0x01	/* imprecise (loss of precision) */
#define FP_X_DZ		0x02	/* divide-by-zero exception */
#define FP_X_UFL	0x04	/* underflow exception */
#define FP_X_OFL	0x08	/* overflow exception */
#define FP_X_INV	0x10	/* invalid operation exception */

typedef enum {
    FP_RN=0,			/* round to nearest representable number */
    FP_RZ=1,			/* round to zero (truncate) */
    FP_RM=2,			/* round toward negative infinity */
    FP_RP=3			/* round toward positive infinity */
} fp_rnd;

#endif /* _POWERPC_IEEEFP_H_ */
