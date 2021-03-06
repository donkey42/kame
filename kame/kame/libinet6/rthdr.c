/*	$KAME: rthdr.c,v 1.22 2006/02/09 08:18:58 keiichi Exp $	*/

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

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ip6.h>

#include <string.h>
#include <stdio.h>

/*
 * RFC2292 API
 */

size_t
inet6_rthdr_space(type, seg)
	int type, seg;
{
	switch (type) {
	case IPV6_RTHDR_TYPE_0:
		if (seg < 1 || seg > 23)
			return (0);
#ifdef COMPAT_RFC2292
		return (CMSG_SPACE(sizeof(struct in6_addr) * (seg - 1) +
		    sizeof(struct ip6_rthdr0)));
#else
		return (CMSG_SPACE(sizeof(struct in6_addr) * seg +
		    sizeof(struct ip6_rthdr0)));
#endif 
	default:
		return (0);
	}
}

struct cmsghdr *
inet6_rthdr_init(bp, type)
	void *bp;
	int type;
{
	struct cmsghdr *ch = (struct cmsghdr *)bp;
	struct ip6_rthdr *rthdr;

	rthdr = (struct ip6_rthdr *)CMSG_DATA(ch);

	ch->cmsg_level = IPPROTO_IPV6;
	ch->cmsg_type = IPV6_RTHDR;

	switch (type) {
	case IPV6_RTHDR_TYPE_0:
#ifdef COMPAT_RFC2292
		ch->cmsg_len = CMSG_LEN(sizeof(struct ip6_rthdr0) -
		    sizeof(struct in6_addr));
#else
		ch->cmsg_len = CMSG_LEN(sizeof(struct ip6_rthdr0));
#endif 

		bzero(rthdr, sizeof(struct ip6_rthdr0));
		rthdr->ip6r_type = IPV6_RTHDR_TYPE_0;
		return (ch);
	default:
		return (NULL);
	}
}

/* ARGSUSED */
int
inet6_rthdr_add(cmsg, addr, flags)
	struct cmsghdr *cmsg;
	const struct in6_addr *addr;
	u_int flags;
{
	struct ip6_rthdr *rthdr;

	rthdr = (struct ip6_rthdr *)CMSG_DATA(cmsg);

	switch (rthdr->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
	{
		struct ip6_rthdr0 *rt0 = (struct ip6_rthdr0 *)rthdr;
		if (flags != IPV6_RTHDR_LOOSE && flags != IPV6_RTHDR_STRICT)
			return (-1);
		if (rt0->ip6r0_segleft == 23)
			return (-1);
		if (flags != IPV6_RTHDR_LOOSE)
			return (-1);
		rt0->ip6r0_segleft++;
		bcopy(addr, (caddr_t)rt0 + ((rt0->ip6r0_len + 1) << 3),
		    sizeof(struct in6_addr));
		rt0->ip6r0_len += sizeof(struct in6_addr) >> 3;
		cmsg->cmsg_len = CMSG_LEN((rt0->ip6r0_len + 1) << 3);
		break;
	}
	default:
		return (-1);
	}

	return (0);
}

/* ARGSUSED */
int
inet6_rthdr_lasthop(cmsg, flags)
	struct cmsghdr *cmsg;
	unsigned int flags;
{
	struct ip6_rthdr *rthdr;

	rthdr = (struct ip6_rthdr *)CMSG_DATA(cmsg);

	switch (rthdr->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
	{
		struct ip6_rthdr0 *rt0 = (struct ip6_rthdr0 *)rthdr;
		if (rt0->ip6r0_segleft > 23)
			return (-1);
		if (flags != IPV6_RTHDR_LOOSE)
			return (-1);
		break;
	}
	default:
		return (-1);
	}

	return (0);
}

#if 0
int
inet6_rthdr_reverse(in, out)
	const struct cmsghdr *in;
	struct cmsghdr *out;
{

	return (-1);
}
#endif

int
inet6_rthdr_segments(cmsg)
	const struct cmsghdr *cmsg;
{
	struct ip6_rthdr *rthdr;

	rthdr = (struct ip6_rthdr *)CMSG_DATA(cmsg);

	switch (rthdr->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
	{
		struct ip6_rthdr0 *rt0 = (struct ip6_rthdr0 *)rthdr;

		if (rt0->ip6r0_len % 2 || 46 < rt0->ip6r0_len)
			return (-1);

		return (rt0->ip6r0_len * 8) / sizeof(struct in6_addr);
	}

	default:
		return (-1);
	}
}

struct in6_addr *
inet6_rthdr_getaddr(cmsg, idx)
	struct cmsghdr *cmsg;
	int idx;
{
	struct ip6_rthdr *rthdr;

	rthdr = (struct ip6_rthdr *)CMSG_DATA(cmsg);

	switch (rthdr->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
	{
		struct ip6_rthdr0 *rt0 = (struct ip6_rthdr0 *)rthdr;
		int naddr;

		if (rt0->ip6r0_len % 2 || 46 < rt0->ip6r0_len)
			return NULL;
		naddr = (rt0->ip6r0_len * 8) / sizeof(struct in6_addr);
		if (idx <= 0 || naddr < idx)
			return NULL;
#ifdef COMPAT_RFC2292
		return (((struct in6_addr *)(rt0 + 1)) + idx - 1);
#else
		return (((struct in6_addr *)(rt0 + 1)) + idx);
#endif
	}

	default:
		return NULL;
	}
}

int
inet6_rthdr_getflags(cmsg, idx)
	const struct cmsghdr *cmsg;
	int idx;
{
	struct ip6_rthdr *rthdr;

	rthdr = (struct ip6_rthdr *)CMSG_DATA(cmsg);

	switch (rthdr->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
	{
		struct ip6_rthdr0 *rt0 = (struct ip6_rthdr0 *)rthdr;
		int naddr;

		if (rt0->ip6r0_len % 2 || 46 < rt0->ip6r0_len)
			return (-1);
		naddr = (rt0->ip6r0_len * 8) / sizeof(struct in6_addr);
		if (idx < 0 || naddr < idx)
			return (-1);
		return IPV6_RTHDR_LOOSE;
	}

	default:
		return (-1);
	}
}

/*
 * RFC3542 (2292bis) API
 */

socklen_t
inet6_rth_space(int type, int segments)
{
	switch (type) {
	case IPV6_RTHDR_TYPE_0:
		return (((segments * 2) + 1) << 3);
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		if (segments != 1)
			return (0); /* XXX is 0 OK? */
		return (((segments * 2) + 1) << 3);
#endif /* MIP6 */
	default:
		return (0);	/* type not suppported */
	}
}

void *
inet6_rth_init(void *bp, socklen_t bp_len, int type, int segments)
{
	struct ip6_rthdr *rth = (struct ip6_rthdr *)bp;
	struct ip6_rthdr0 *rth0;
#ifdef MIP6
	struct ip6_rthdr2 *rth2;
#endif /* MIP6 */

	switch (type) {
	case IPV6_RTHDR_TYPE_0:
		/* length validation */
		if (bp_len < inet6_rth_space(IPV6_RTHDR_TYPE_0, segments))
			return (NULL);

		memset(bp, 0, bp_len);
		rth0 = (struct ip6_rthdr0 *)rth;
		rth0->ip6r0_len = segments * 2;
		rth0->ip6r0_type = IPV6_RTHDR_TYPE_0;
		rth0->ip6r0_segleft = 0;
		rth0->ip6r0_reserved = 0;
		break;
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		if (segments != 1)
			return (NULL); /* segments must be 1. */

		/* length validation */
		if (bp_len < inet6_rth_space(IPV6_RTHDR_TYPE_2, segments))
			return (NULL);

		memset(bp, 0, bp_len);
		rth2 = (struct ip6_rthdr2 *)rth;
		rth2->ip6r2_len = segments * 2;
		rth2->ip6r2_type = IPV6_RTHDR_TYPE_2;
		rth2->ip6r2_segleft = 0;
		rth2->ip6r2_reserved = 0;
		break;
#endif /* MIP6 */
	default:
		return (NULL);	/* type not supported */
	}

	return (bp);
}

int
inet6_rth_add(void *bp, const struct in6_addr *addr)
{
	struct ip6_rthdr *rth = (struct ip6_rthdr *)bp;
	struct ip6_rthdr0 *rth0;
#ifdef MIP6
	struct ip6_rthdr2 *rth2;
#endif /* MIP6 */
	struct in6_addr *nextaddr;

	switch (rth->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
		rth0 = (struct ip6_rthdr0 *)rth;
		nextaddr = (struct in6_addr *)(rth0 + 1) + rth0->ip6r0_segleft;
		*nextaddr = *addr;
		rth0->ip6r0_segleft++;
		break;
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		rth2 = (struct ip6_rthdr2 *)rth;
		if (rth2->ip6r2_segleft != 0)
			return (-1); /* rthdr2 can contain just one address. */
		nextaddr = (struct in6_addr *)(rth2 + 1) + rth2->ip6r2_segleft;
		*nextaddr = *addr;
		rth2->ip6r2_segleft++;
		break;
#endif /* MIP6 */
	default:
		return (-1);	/* type not supported */
	}

	return (0);
}

int
inet6_rth_reverse(const void *in, void *out)
{
	struct ip6_rthdr *rth_in = (struct ip6_rthdr *)in;
	struct ip6_rthdr0 *rth0_in, *rth0_out;
	int i, segments;

	switch (rth_in->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
		rth0_in = (struct ip6_rthdr0 *)in;
		rth0_out = (struct ip6_rthdr0 *)out;

		/* parameter validation XXX too paranoid? */
		if (rth0_in->ip6r0_len % 2)
			return (-1);
		segments = rth0_in->ip6r0_len / 2;

		/* we can't use memcpy here, since in and out may overlap */
		memmove((void *)rth0_out, (void *)rth0_in,
			((rth0_in->ip6r0_len) + 1) << 3);
		rth0_out->ip6r0_segleft = segments;

		/* reverse the addresses */
		for (i = 0; i < segments / 2; i++) {
			struct in6_addr addr_tmp, *addr1, *addr2;

			addr1 = (struct in6_addr *)(rth0_out + 1) + i;
			addr2 = (struct in6_addr *)(rth0_out + 1) +
				(segments - i - 1);
			addr_tmp = *addr1;
			*addr1 = *addr2;
			*addr2 = addr_tmp;
		}
		
		break;
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		/* reversing operation is not supported for type 2. */
		return (-1);
#endif /* MIP6 */
	default:
		return (-1);	/* type not supported */
	}

	return (0);
}

int
inet6_rth_segments(const void *bp)
{
	struct ip6_rthdr *rh = (struct ip6_rthdr *)bp;
	struct ip6_rthdr0 *rh0;
#ifdef MIP6
	struct ip6_rthdr2 *rh2;
#endif /* MIP6 */
	int addrs;

	switch (rh->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
		rh0 = (struct ip6_rthdr0 *)bp;

		/*
		 * Validation for a type-0 routing header.
		 * Is this too strict?
		 */
		if ((rh0->ip6r0_len % 2) != 0 ||
		    (addrs = (rh0->ip6r0_len >> 1)) < rh0->ip6r0_segleft)
			return (-1);

		return (addrs);
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		rh2 = (struct ip6_rthdr2 *)bp;

		/*
		 * Validation for a type-2 routing header.
		 */
		if ((rh2->ip6r2_len % 2) != 0 ||
		    (addrs = (rh2->ip6r2_len >> 1)) < rh2->ip6r2_segleft)
			return (-1);

		return (addrs);
#endif /* MIP6 */
	default:
		return (-1);	/* unknown type */
	}
}

struct in6_addr *
inet6_rth_getaddr(const void *bp, int idx)
{
	struct ip6_rthdr *rh = (struct ip6_rthdr *)bp;
	struct ip6_rthdr0 *rh0;
#ifdef MIP6
	struct ip6_rthdr2 *rh2;
#endif /* MIP6 */
	int addrs;

	switch (rh->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
		 rh0 = (struct ip6_rthdr0 *)bp;
		 
		/*
		 * Validation for a type-0 routing header.
		 * Is this too strict?
		 */
		if ((rh0->ip6r0_len % 2) != 0 ||
		    (addrs = (rh0->ip6r0_len >> 1)) < rh0->ip6r0_segleft)
			return (NULL);

		if (idx < 0 || addrs <= idx)
			return (NULL);

		return (((struct in6_addr *)(rh0 + 1)) + idx);
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		rh2 = (struct ip6_rthdr2 *)bp;

		/* rthdr2 contains just one address. */
		if (idx != 1)
			return (NULL); /* rthdr2 contains just one address. */

		/*
		 * Validation for a type-2 routing header.
		 */
		if ((rh2->ip6r2_len % 2) != 0 ||
		    (addrs = (rh2->ip6r2_len >> 1)) < rh2->ip6r2_segleft)
			return (NULL);

		if (idx < 0 || addrs <= idx)
			return (NULL);

		return (((struct in6_addr *)(rh2 + 1)) + idx);
#endif /* MIP6 */
	default:
		return (NULL);	/* unknown type */
		break;
	}
}
