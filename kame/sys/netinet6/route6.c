/*	$KAME: route6.c,v 1.39 2002/09/27 09:17:43 keiichi Exp $	*/

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
#include "opt_mip6.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/systm.h>
#include <sys/queue.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <netinet/icmp6.h>

#ifdef MIP6
#include <net/if_hif.h>
#include <netinet6/nd6.h>
#include <netinet6/mip6_var.h>
#include <netinet6/mip6.h>
#endif /* MIP6 */

static int ip6_rthdr0 __P((struct mbuf *, struct ip6_hdr *,
    struct ip6_rthdr0 *));
#ifdef MIP6
static int ip6_rthdr2 __P((struct mbuf *, struct ip6_hdr *,
    struct ip6_rthdr2 *));
#endif /* MIP6 */

int
route6_input(mp, offp, proto)
	struct mbuf **mp;
	int *offp, proto;	/* proto is unused */
{
	struct ip6_hdr *ip6;
	struct mbuf *m = *mp;
	struct ip6_rthdr *rh;
	int off = *offp, rhlen;
	struct mbuf *n;

	n = ip6_findaux(m);
	if (n) {
		struct ip6aux *ip6a = mtod(n, struct ip6aux *);
		/* XXX reject home-address option before rthdr */
		if (ip6a->ip6a_flags & IP6A_SWAP) {
			ip6stat.ip6s_badoptions++;
			m_freem(m);
			return IPPROTO_DONE;
		}
	}

#ifndef PULLDOWN_TEST
	IP6_EXTHDR_CHECK(m, off, sizeof(*rh), IPPROTO_DONE);
	ip6 = mtod(m, struct ip6_hdr *);
	rh = (struct ip6_rthdr *)((caddr_t)ip6 + off);
#else
	ip6 = mtod(m, struct ip6_hdr *);
	IP6_EXTHDR_GET(rh, struct ip6_rthdr *, m, off, sizeof(*rh));
	if (rh == NULL) {
		ip6stat.ip6s_tooshort++;
		return IPPROTO_DONE;
	}
#endif

	switch (rh->ip6r_type) {
	case IPV6_RTHDR_TYPE_0:
		rhlen = (rh->ip6r_len + 1) << 3;
#ifndef PULLDOWN_TEST
		/*
		 * note on option length:
		 * due to IP6_EXTHDR_CHECK assumption, we cannot handle
		 * very big routing header (max rhlen == 2048).
		 */
		IP6_EXTHDR_CHECK(m, off, rhlen, IPPROTO_DONE);
#else
		/*
		 * note on option length:
		 * maximum rhlen: 2048
		 * max mbuf m_pulldown can handle: MCLBYTES == usually 2048
		 * so, here we are assuming that m_pulldown can handle
		 * rhlen == 2048 case.  this may not be a good thing to
		 * assume - we may want to avoid pulling it up altogether.
		 */
		IP6_EXTHDR_GET(rh, struct ip6_rthdr *, m, off, rhlen);
		if (rh == NULL) {
			ip6stat.ip6s_tooshort++;
			return IPPROTO_DONE;
		}
#endif
		if (ip6_rthdr0(m, ip6, (struct ip6_rthdr0 *)rh))
			return (IPPROTO_DONE);
		break;
#ifdef MIP6
	case IPV6_RTHDR_TYPE_2:
		/* sanity check. */
		if (!MIP6_IS_MN) {
			/* the node must be a mobile node. */
			return (IPPROTO_DONE);
		}
		if (rh->ip6r_len != 2) {
			/*
			 * the length field in the RH must be exactly
			 * 2.
			 */
			return (IPPROTO_DONE);
		}
		if (rh->ip6r_segleft > 1) {
			/*
			 * the segments left field in the RH must be
			 * exactry 1.
			 */
			return (IPPROTO_DONE);
		}
		rhlen = 24; /* (rh->ip6r_len + 1) << 3 */
#ifndef PULLDOWN_TEST
		IP6_EXTHDR_CHECK(m, off, rhlen, IPPROTO_DONE);
#else
		IP6_EXTHDR_GET(rh, struct ip6_rthdr *, m, off, rhlen);
		if (rh == NULL) {
			ip6stat.ip6s_tooshort++;
			return IPPROTO_DONE;
		}
#endif
		if (ip6_rthdr2(m, ip6, (struct ip6_rthdr2 *)rh))
			return (IPPROTO_DONE);
		break;
#endif /* MIP6 */
	default:
		/* unknown routing type */
		if (rh->ip6r_segleft == 0) {
			rhlen = (rh->ip6r_len + 1) << 3;
			break;	/* Final dst. Just ignore the header. */
		}
		ip6stat.ip6s_badoptions++;
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
			    (caddr_t)&rh->ip6r_type - (caddr_t)ip6);
		return (IPPROTO_DONE);
	}

	*offp += rhlen;
	return (rh->ip6r_nxt);
}

/*
 * Type0 routing header processing
 *
 * RFC2292 backward compatibility warning: no support for strict/loose bitmap,
 * as it was dropped between RFC1883 and RFC2460.
 */
static int
ip6_rthdr0(m, ip6, rh0)
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct ip6_rthdr0 *rh0;
{
	int addrs, index;
	struct sockaddr_in6 next_sa;
	struct in6_addr *nextaddr, tmpaddr;
	struct in6_ifaddr *ifa;

	if (rh0->ip6r0_segleft == 0)
		return (0);

	if (rh0->ip6r0_len % 2
#ifdef COMPAT_RFC1883
	    || rh0->ip6r0_len > 46
#endif
		) {
		/*
		 * Type 0 routing header can't contain more than 23 addresses.
		 * RFC 2462: this limitation was removed since strict/loose
		 * bitmap field was deleted.
		 */
		ip6stat.ip6s_badoptions++;
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
			    (caddr_t)&rh0->ip6r0_len - (caddr_t)ip6);
		return (-1);
	}

	if ((addrs = rh0->ip6r0_len / 2) < rh0->ip6r0_segleft) {
		ip6stat.ip6s_badoptions++;
		icmp6_error(m, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_HEADER,
			    (caddr_t)&rh0->ip6r0_segleft - (caddr_t)ip6);
		return (-1);
	}

	index = addrs - rh0->ip6r0_segleft;
	rh0->ip6r0_segleft--;
	nextaddr = ((struct in6_addr *)(rh0 + 1)) + index;

	/*
	 * reject invalid addresses.  be proactive about malicious use of
	 * IPv4 mapped/compat address.
	 * XXX need more checks?
	 */
	if (IN6_IS_ADDR_MULTICAST(nextaddr) ||
	    IN6_IS_ADDR_UNSPECIFIED(nextaddr) ||
	    IN6_IS_ADDR_V4MAPPED(nextaddr) ||
	    IN6_IS_ADDR_V4COMPAT(nextaddr)) {
		ip6stat.ip6s_badoptions++;
		m_freem(m);
		return (-1);
	}
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_V4MAPPED(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_V4COMPAT(&ip6->ip6_dst)) {
		ip6stat.ip6s_badoptions++;
		goto bad;
	}

	/*
	 * determine the scope zone of the next hop, based on the interface
	 * of the current hop.
	 * [draft-ietf-ipngwg-scoping-arch, Section 9]
	 */
	if ((ifa = ip6_getdstifaddr(m)) == NULL)
		goto bad;
	/*
	 * construct a sockaddr_in6 for the next hop with the zone ID,
	 * then update the recorded destination address.
	 */
	bzero(&next_sa, sizeof(next_sa));
	next_sa.sin6_family = AF_INET6;
	next_sa.sin6_len = sizeof(next_sa);
	next_sa.sin6_addr = *nextaddr;
	if (in6_addr2zoneid(ifa->ia_ifp, nextaddr, &next_sa.sin6_scope_id)) {
		ip6stat.ip6s_badscope++;
		goto bad;
	}
	if (in6_embedscope(&next_sa.sin6_addr, &next_sa)) {
		/* XXX: should not happen */
		ip6stat.ip6s_badscope++;
		goto bad;
	}
	if (!ip6_setpktaddrs(m, NULL, &next_sa))
		goto bad;

	/*
	 * Swap the IPv6 destination address and nextaddr. Forward the packet.
	 */
	tmpaddr = *nextaddr;
	*nextaddr = ip6->ip6_dst;
	ip6->ip6_dst = tmpaddr;

#ifdef COMPAT_RFC1883
	if (rh0->ip6r0_slmap[index / 8] & (1 << (7 - (index % 8))))
		ip6_forward(m, IPV6_SRCRT_NEIGHBOR);
	else
		ip6_forward(m, IPV6_SRCRT_NOTNEIGHBOR);
#else
	ip6_forward(m, 1);
#endif

	return (-1);			/* m would be freed in ip6_forward() */

  bad:
	m_freem(m);
	return (-1);
}

#ifdef MIP6
/*
 * type 2 routing header processing.
 */
static int
ip6_rthdr2(m, ip6, rh2)
	struct mbuf *m;
	struct ip6_hdr *ip6;
	struct ip6_rthdr2 *rh2;
{
	int rh2_has_hoa;
	struct sockaddr_in6 *src_sa, *dst_sa, next_sa;
	struct hif_softc *sc;
	struct mip6_bu *mbu;
	struct in6_addr *nextaddr, tmpaddr;
	struct in6_ifaddr *ifa;

	rh2_has_hoa = 0;

	/* get ip src and dst addrs. */
	if (ip6_getpktaddrs(m, &src_sa, &dst_sa))
		goto bad;

	/*
	 * determine the scope zone of the next hop, based on the interface
	 * of the current hop.
	 * [draft-ietf-ipngwg-scoping-arch, Section 9]
	 */
	if ((ifa = ip6_getdstifaddr(m)) == NULL)
		goto bad;
	bzero(&next_sa, sizeof(next_sa));
	next_sa.sin6_len = sizeof(next_sa);
	next_sa.sin6_family = AF_INET6;
	bcopy((const void *)(rh2 + 1), &next_sa.sin6_addr,
	      sizeof(struct in6_addr));
	nextaddr = (struct in6_addr *)(rh2 + 1);
	if (in6_addr2zoneid(ifa->ia_ifp,
			    &next_sa.sin6_addr,
			    &next_sa.sin6_scope_id)) {
		/* should not happen. */
		ip6stat.ip6s_badscope++;
		goto bad;
	}
	if (in6_embedscope(&next_sa.sin6_addr, &next_sa)) {
		/* XXX: should not happen */
		ip6stat.ip6s_badscope++;
		goto bad;
	}

	/* check addresses in ip6_dst and rh2. */
	for (sc = TAILQ_FIRST(&hif_softc_list);
	     sc;
	     sc = TAILQ_NEXT(sc, hif_entry)) {
		for (mbu = LIST_FIRST(&sc->hif_bu_list);
		     mbu;
		     mbu = LIST_NEXT(mbu, mbu_entry)) {
			if ((mbu->mbu_flags & IP6MU_HOME) == 0)
				continue;

			/* XXX should we check a registration status? */

			if (rh2->ip6r2_segleft == 0) {
				struct mbuf *n;
				struct ip6aux *ip6a;

				/*
				 * if segleft == 0, ip6_dst must be
				 * one of our home addresses.
				 */
				if (!SA6_ARE_ADDR_EQUAL(dst_sa,
							&mbu->mbu_haddr))
					continue;

				mip6stat.mip6s_rthdr2++;

				/*
				 * if the previous hop is the coa that
				 * is corresponding to the hoa in
				 * ip6_dst, the route is optimized
				 * already.
				 */
				if (!SA6_ARE_ADDR_EQUAL(&next_sa,
							&mbu->mbu_coa)) {
					/* coa mismatch.  discard this. */
					goto bad;
				}

				/*
				 * the route is already optimized.
				 * set optimized flag in m_aux.
				 */
				n = ip6_findaux(m);
				if (n) {
					ip6a = mtod(n, struct ip6aux *);
					ip6a->ip6a_flags
						|= IP6A_ROUTEOPTIMIZED;
					return (0);
				}
				/* if n == 0 return error. */
				goto bad;
			} else {
				/*
				 * if segleft == 1, the specified
				 * intermediate node must be one of
				 * our home addresses.
				 */
				if (!SA6_ARE_ADDR_EQUAL(&next_sa,
							&mbu->mbu_haddr))
					continue;
				rh2_has_hoa++;
			}
		}
	}
	if (rh2_has_hoa == 0) {
		/*
		 * this rh2 includes an address that is not one of our
		 * home addresses.
		 */
		goto bad;
	}

	rh2->ip6r2_segleft--;

	/*
	 * reject invalid addresses.  be proactive about malicious use of
	 * IPv4 mapped/compat address.
	 * XXX need more checks?
	 */
	if (IN6_IS_ADDR_MULTICAST(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_UNSPECIFIED(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_V4MAPPED(&ip6->ip6_dst) ||
	    IN6_IS_ADDR_V4COMPAT(&ip6->ip6_dst)) {
		ip6stat.ip6s_badoptions++;
		goto bad;
	}

	if (!ip6_setpktaddrs(m, NULL, &next_sa))
		goto bad;

	/*
	 * Swap the IPv6 destination address and nextaddr. Forward the packet.
	 */
	tmpaddr = *nextaddr;
	*nextaddr = ip6->ip6_dst;
	ip6->ip6_dst = tmpaddr;
	ip6_forward(m, 1);

	return (-1);			/* m would be freed in ip6_forward() */

  bad:
	m_freem(m);
	return (-1);
}
#endif /* MIP6 */