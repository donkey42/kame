/*	$KAME: ip6_id.c,v 1.3 2003/09/06 02:49:26 itojun Exp $	*/
/*	$OpenBSD: ip_id.c,v 1.6 2002/03/15 18:19:52 millert Exp $	*/

/*
 * Copyright (C) 2003 WIDE Project.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright 1998 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Theo de Raadt <deraadt@openbsd.org> came up with the idea of using
 * such a mathematical system to generate more random (yet non-repeating)
 * ids to solve the resolver/named problem.  But Niels designed the
 * actual system based on the constraints.
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
 *      This product includes software developed by Niels Provos.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * seed = random 15bit
 * n = prime, g0 = generator to n,
 * j = random so that gcd(j,n-1) == 1
 * g = g0^j mod n will be a generator again.
 *
 * X[0] = random seed.
 * X[n] = a*X[n-1]+b mod m is a Linear Congruential Generator
 * with a = 7^(even random) mod m,
 *      b = random with gcd(b,m) == 1
 *      m = constant and a maximal period of m-1.
 *
 * The transaction id is determined by:
 * id[n] = seed xor (g^X[n] mod n)
 *
 * Effectivly the id is restricted to the lower 15 bits, thus
 * yielding two different cycles by toggling the msb on and off.
 * This avoids reuse issues caused by reseeding.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#ifdef __NetBSD__
#include <lib/libkern/libkern.h>
#endif
#include <sys/socket.h>

#include <net/if.h>
#ifdef __FreeBSD__
#include <net/route.h>
#endif
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#ifdef __OpenBSD__
#include <dev/rndvar.h>
#endif

#ifndef INT32_MAX
#define INT32_MAX	0x7fffffffU
#endif

struct randomtab {
	const long	ru_out;	/* Time after wich will be reseeded */
	const u_int32_t ru_max;	/* Uniq cycle, avoid blackjack prediction */
	const u_int32_t ru_gen;	/* Starting generator */
	const u_int32_t ru_n;	/* RU_N-1 = 2^2*3^2*59652323 */
	const u_int32_t ru_agen; /* determine ru_a as RU_AGEN^(2*rand) */
	const u_int32_t ru_m;	/* RU_M = 2^7*3^15 - don't change */
	const u_int32_t pfacts[4];	/* factors of ru_n */

	u_int32_t ru_counter;
	u_int32_t ru_msb;

	u_int32_t ru_x;
	u_int32_t ru_seed, ru_seed2;
	u_int32_t ru_a, ru_b;
	u_int32_t ru_g;
	long ru_reseed;
};

static struct randomtab randomtab_32 = {
	180,			/* Time after wich will be reseeded */
	1000000000,		/* Uniq cycle, avoid blackjack prediction */
	2,			/* Starting generator */
	2147483629,		/* RU_N-1 = 2^2*3^2*59652323 */
	7,			/* determine ru_a as RU_AGEN^(2*rand) */
	1836660096,		/* RU_M = 2^7*3^15 - don't change */
	{ 2, 3, 59652323, -1 },	/* factors of ru_n */
};

static struct randomtab randomtab_20 = {
	180,			/* Time after wich will be reseeded */
	200000,			/* Uniq cycle, avoid blackjack prediction */
	2,			/* Starting generator */
	524269,			/* RU_N-1 = 2^2*3^2*14563 */
	7,			/* determine ru_a as RU_AGEN^(2*rand) */
	279936,			/* RU_M = 2^7*3^7 - don't change */
	{ 2, 3, 14563, -1 },	/* factors of ru_n */
};

static u_int32_t pmod(u_int32_t, u_int32_t, u_int32_t);
static void initid(struct randomtab *);
static u_int32_t randomid(struct randomtab *);

/*
 * Do a fast modular exponation, returned value will be in the range
 * of 0 - (mod-1)
 */

static u_int32_t
pmod(u_int32_t gen, u_int32_t exp, u_int32_t mod)
{
	u_int64_t s, t, u;

	s = 1;
	t = gen;
	u = exp;

	while (u) {
		if (u & 1)
			s = (s * t) % mod;
		u >>= 1;
		t = (t * t) % mod;
	}
	return (s);
}

/*
 * Initalizes the seed and chooses a suitable generator. Also toggles
 * the msb flag. The msb flag is used to generate two distinct
 * cycles of random numbers and thus avoiding reuse of ids.
 *
 * This function is called from id_randomid() when needed, an
 * application does not have to worry about it.
 */
static void
initid(struct randomtab *p)
{
	u_int32_t j, i;
	int noprime = 1;

	p->ru_x = arc4random() % p->ru_m;

	/* 31 bits of random seed */
	p->ru_seed = arc4random() & INT32_MAX;
	p->ru_seed2 = arc4random() & INT32_MAX;

	/* Determine the LCG we use */
	p->ru_b = arc4random() | 1;
	p->ru_a = pmod(p->ru_agen, arc4random() & (~1U), p->ru_m);
	while (p->ru_b % 3 == 0)
		p->ru_b += 2;

	j = arc4random() % p->ru_n;

	/*
	 * Do a fast gcd(j, RU_N - 1), so we can find a j with
	 * gcd(j, RU_N - 1) == 1, giving a new generator for
	 * RU_GEN^j mod RU_N
	 */
	while (noprime) {
		for (i = 0; p->pfacts[i] > 0; i++)
			if (j % p->pfacts[i] == 0)
				break;

		if (p->pfacts[i] <= 0)
			noprime = 0;
		else
			j = (j + 1) % p->ru_n;
	}

	p->ru_g = pmod(p->ru_gen, j, p->ru_n);
	p->ru_counter = 0;

#ifdef __FreeBSD__
	p->ru_reseed = time_second + p->ru_out;
#else
	p->ru_reseed = time.tv_sec + p->ru_out;
#endif
	p->ru_msb = p->ru_msb == 0x80000000 ? 0 : 0x80000000;
}

static u_int32_t
randomid(struct randomtab *p)
{
	int i, n;
	u_int32_t tmp;

#ifdef __FreeBSD__
	if (p->ru_counter >= p->ru_max || time_second > p->ru_reseed)
#else
	if (p->ru_counter >= p->ru_max || time.tv_sec > p->ru_reseed)
#endif
		initid(p);

	tmp = arc4random();

	/* Skip a random number of ids */
	n = tmp & 0x3; tmp = tmp >> 2;
	if (p->ru_counter + n >= p->ru_max)
		initid(p);

	for (i = 0; i <= n; i++) {
		/* Linear Congruential Generator */
		p->ru_x = (p->ru_a * p->ru_x + p->ru_b) % p->ru_m;
	}

	p->ru_counter += i;

	return (p->ru_seed ^ pmod(p->ru_g, p->ru_seed2 ^ p->ru_x, p->ru_n)) |
	    p->ru_msb;
}

u_int32_t
ip6_randomid(void)
{

	return randomid(&randomtab_32);
}

u_int32_t
ip6_randomflowlabel(void)
{

	return randomid(&randomtab_20) & 0xfffff;
}
