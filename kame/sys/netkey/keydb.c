/*	$KAME: keydb.c,v 1.67 2002/07/08 07:10:21 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include "opt_inet.h"
#include "opt_inet6.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#include "opt_ipsec.h"
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/errno.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>

#include <net/pfkeyv2.h>
#include <netkey/keydb.h>
#include <netinet6/ipsec.h>

#include <net/net_osdep.h>

#if defined(__FreeBSD__) && __FreeBSD__ >= 3
MALLOC_DEFINE(M_SECA, "key mgmt", "security associations, key management");
#endif

extern TAILQ_HEAD(_sptailq, secpolicy) sptailq;

static void keydb_delsecasvar __P((struct secasvar *));

/*
 * secpolicy management
 */
struct secpolicy *
keydb_newsecpolicy()
{
	struct secpolicy *p, *np;

	p = (struct secpolicy *)malloc(sizeof(*p), M_SECA, M_NOWAIT);
	if (!p)
		return p;
	bzero(p, sizeof(*p));
#ifdef TAILQ_EMPTY
	if (TAILQ_EMPTY(&sptailq))
#else
	if (TAILQ_FIRST(&sptailq) == NULL)
#endif
	{
		p->id = 1;
		TAILQ_INSERT_HEAD(&sptailq, p, tailq);
		return p;
	} else if (TAILQ_LAST(&sptailq, _sptailq)->id < 0xffffffff) {
		p->id = TAILQ_LAST(&sptailq, _sptailq)->id + 1;
		TAILQ_INSERT_TAIL(&sptailq, p, tailq);
		return p;
	} else {
		TAILQ_FOREACH(np, &sptailq, tailq) {
			if (np->id + 1 != TAILQ_NEXT(np, tailq)->id) {
				p->id = np->id + 1;
				TAILQ_INSERT_AFTER(&sptailq, np, p, tailq);
				break;
			}
		}
		if (!np) {
			free(p, M_SECA);
			return NULL;
		}
	}

	return p;
}

void
keydb_delsecpolicy(p)
	struct secpolicy *p;
{

	TAILQ_REMOVE(&sptailq, p, tailq);
	if (p->spidx)
		free(p->spidx, M_SECA);
	free(p, M_SECA);
}

int
keydb_setsecpolicyindex(p, idx)
	struct secpolicy *p;
	struct secpolicyindex *idx;
{

	if (!p->spidx)
		p->spidx = (struct secpolicyindex *)malloc(sizeof(*p->spidx),
		    M_SECA, M_NOWAIT);
	if (!p->spidx)
		return ENOMEM;
	memcpy(p->spidx, idx, sizeof(*p->spidx));
	return 0;
}

/*
 * secashead management
 */
struct secashead *
keydb_newsecashead()
{
	struct secashead *p;
	int i;

	p = (struct secashead *)malloc(sizeof(*p), M_SECA, M_NOWAIT);
	if (!p)
		return p;
	bzero(p, sizeof(*p));
	for (i = 0; i < sizeof(p->savtree)/sizeof(p->savtree[0]); i++)
		LIST_INIT(&p->savtree[i]);
	return p;
}

void
keydb_delsecashead(p)
	struct secashead *p;
{

	free(p, M_SECA);
}

/*
 * secasvar management (reference counted)
 */
struct secasvar *
keydb_newsecasvar()
{
	struct secasvar *p;

	p = (struct secasvar *)malloc(sizeof(*p), M_SECA, M_NOWAIT);
	if (!p)
		return p;
	bzero(p, sizeof(*p));
	p->refcnt = 1;
	return p;
}

void
keydb_refsecasvar(p)
	struct secasvar *p;
{
	int s;

#ifdef __NetBSD__
	s = splsoftnet();
#else
	s = splnet();
#endif
	p->refcnt++;
	splx(s);
}

void
keydb_freesecasvar(p)
	struct secasvar *p;
{
	int s;

#ifdef __NetBSD__
	s = splsoftnet();
#else
	s = splnet();
#endif
	p->refcnt--;
	/* negative refcnt will cause panic intentionally */
	if (p->refcnt <= 0)
		keydb_delsecasvar(p);
	splx(s);
}

static void
keydb_delsecasvar(p)
	struct secasvar *p;
{

	if (p->refcnt)
		panic("keydb_delsecasvar called with refcnt != 0");

	free(p, M_SECA);
}

/*
 * secreplay management
 */
struct secreplay *
keydb_newsecreplay(wsize)
	size_t wsize;
{
	struct secreplay *p;

	p = (struct secreplay *)malloc(sizeof(*p), M_SECA, M_NOWAIT);
	if (!p)
		return p;

	bzero(p, sizeof(*p));
	if (wsize != 0) {
		p->bitmap = (caddr_t)malloc(wsize, M_SECA, M_NOWAIT);
		if (!p->bitmap) {
			free(p, M_SECA);
			return NULL;
		}
		bzero(p->bitmap, wsize);
	}
	p->wsize = wsize;
	return p;
}

void
keydb_delsecreplay(p)
	struct secreplay *p;
{

	if (p->bitmap)
		free(p->bitmap, M_SECA);
	free(p, M_SECA);
}

/*
 * secreg management
 */
struct secreg *
keydb_newsecreg()
{
	struct secreg *p;

	p = (struct secreg *)malloc(sizeof(*p), M_SECA, M_NOWAIT);
	if (p)
		bzero(p, sizeof(*p));
	return p;
}

void
keydb_delsecreg(p)
	struct secreg *p;
{

	free(p, M_SECA);
}