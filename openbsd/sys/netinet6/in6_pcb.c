/*	$OpenBSD: in6_pcb.c,v 1.28 2002/03/14 01:27:12 millert Exp $	*/

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

/*
 *	@(#)COPYRIGHT	1.1 (NRL) 17 January 1995
 * 
 * NRL grants permission for redistribution and use in source and binary
 * forms, with or without modification, of the software and documentation
 * created at NRL provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgements:
 * 	This product includes software developed by the University of
 * 	California, Berkeley and its contributors.
 * 	This product includes software developed at the Information
 * 	Technology Division, US Naval Research Laboratory.
 * 4. Neither the name of the NRL nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THE SOFTWARE PROVIDED BY NRL IS PROVIDED BY NRL AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL NRL OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation
 * are those of the authors and should not be interpreted as representing
 * official policies, either expressed or implied, of the US Naval
 * Research Laboratory (NRL).
 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993, 1995
 *	Regents of the University of California.  All rights reserved.
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
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/domain.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/proc.h>

#include <net/if.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/in_pcb.h>

#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/scope6_var.h>

/*
 * External globals
 */

#include <dev/rndvar.h>

extern struct in6_ifaddr *in6_ifaddr;
extern struct in_ifaddr *in_ifaddr;

/*
 * Globals
 */

struct in6_addr zeroin6_addr;

extern int ipport_firstauto;
extern int ipport_lastauto;
extern int ipport_hifirstauto;
extern int ipport_hilastauto;

/*
 * Keep separate inet6ctlerrmap, because I may remap some of these.
 * I also put it here, because, quite frankly, it belongs here, not in
 * ip{v6,}_input().
 */
#if 0
u_char inet6ctlerrmap[PRC_NCMDS] = {
	0,		0,		0,		0,
	0,		EMSGSIZE,	EHOSTDOWN,	EHOSTUNREACH,
	EHOSTUNREACH,	EHOSTUNREACH,	ECONNREFUSED,	ECONNREFUSED,
	EMSGSIZE,	EHOSTUNREACH,	0,		0,
	0,		0,		0,		0,
	ENOPROTOOPT
};
#endif

/*
 * Bind an address (or at least a port) to an PF_INET6 socket.
 */
int
in6_pcbbind(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct socket *so = inp->inp_socket;

	struct inpcbtable *head = inp->inp_table;
	struct sockaddr_in6 *sin6;
	struct proc *p = curproc;		/* XXX */
	u_short lport = 0;
	int wild = INPLOOKUP_IPV6, reuseport = (so->so_options & SO_REUSEPORT);
	int error;

	/*
	 * REMINDER:  Once up to speed, flow label processing should go here,
	 * too.  (Same with in6_pcbconnect.)
	 */
	if (in6_ifaddr == 0 || in_ifaddr == 0)
		return EADDRNOTAVAIL;

	if (inp->inp_lport != 0 || !SA6_IS_ADDR_UNSPECIFIED(&inp->in6p_lsa))
		return EINVAL;	/* If already bound, EINVAL! */

	if ((so->so_options & (SO_REUSEADDR | SO_REUSEPORT)) == 0 &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	     (so->so_options & SO_ACCEPTCONN) == 0))
		wild |= INPLOOKUP_WILDCARD;

	/*
	 * If I did get a sockaddr passed in...
	 */
	if (nam) {
		sin6 = mtod(nam, struct sockaddr_in6 *);
		if (nam->m_len != sizeof (*sin6))
			return EINVAL;

		/*
		 * Unlike v4, I have no qualms about EAFNOSUPPORT if the
		 * wretched family is not filled in!
		 */
		if (sin6->sin6_family != AF_INET6)
			return EAFNOSUPPORT;

		if ((error = scope6_check_id(sin6, ip6_use_defzone)) != 0)
			return(error);

		lport = sin6->sin6_port;

		/* reject IPv4 mapped address, we have no support for it */
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
			return EADDRNOTAVAIL;

		if (IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr)) {
			/*
			 * Treat SO_REUSEADDR as SO_REUSEPORT for multicast;
			 * allow complete duplication of binding if
			 * SO_REUSEPORT is set, or if SO_REUSEADDR is set
			 * and a multicast address is bound on both
			 * new and duplicated sockets.
			 */
			if (so->so_options & SO_REUSEADDR)
				reuseport = SO_REUSEADDR | SO_REUSEPORT;
		} else if (!SA6_IS_ADDR_UNSPECIFIED(sin6)) {
			struct ifaddr *ia = NULL;
#ifndef SCOPEDROUTING
			u_int32_t lzone; 
#endif

			sin6->sin6_port = 0;  /*
					       * Yechhhh, because of upcoming
					       * call to ifa_ifwithaddr(), which
					       * does bcmp's over the PORTS as
					       * well.  (What about flow?)
					       */
			sin6->sin6_flowinfo = 0;
#ifndef SCOPEDROUTING
			lzone = sin6->sin6_scope_id;
			sin6->sin6_scope_id = 0; /* XXX: for ifa_ifwithaddr */
#endif
			if ((ia = ifa_ifwithaddr((struct sockaddr *)sin6))
			    == NULL)
				return EADDRNOTAVAIL;
#ifndef SCOPEDROUTING
			sin6->sin6_scope_id = lzone;
#endif

			/*
			 * XXX: bind to an anycast address might accidentally
			 * cause sending a packet with anycast source address.
			 * We should allow to bind to a deprecated address,
			 * since the application dare to use it.
			 */
			if (ia &&
			    ((struct in6_ifaddr *)ia)->ia6_flags &
			    (IN6_IFF_ANYCAST|IN6_IFF_NOTREADY|IN6_IFF_DETACHED))
				return(EADDRNOTAVAIL);
		}
		if (lport) {
			struct inpcb *t;

			/*
			 * Question:  Do we wish to continue the Berkeley
			 * tradition of ports < IPPORT_RESERVED be only for
			 * root?
			 * Answer: For now yes, but IMHO, it should be REMOVED!
			 * OUCH: One other thing, is there no better way of
			 * finding a process for a socket instead of using
			 * curproc?  (Marked with BSD's {in,}famous XXX ?
			 */
			if (ntohs(lport) < IPPORT_RESERVED &&
			    (error = suser(p->p_ucred, &p->p_acflag)))
				return error;

			t = in_pcblookup(head, (void *)&sa6_any, 0, sin6, lport,
			    wild);

			if (t && (reuseport & t->inp_socket->so_options) == 0)
				return EADDRINUSE;
		}
		sa6_copy_addr(sin6, &inp->in6p_lsa);

#if 0
		if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
			inp->inp_ipv6.ip6_flow = htonl(0x60000000) |
			    (sin6->sin6_flowinfo & htonl(0x0fffffff));
		}
#endif
	}

	if (lport == 0) {
		error = in6_pcbsetport(&inp->in6p_lsa, inp, p);
		if (error != 0)
			return error;
	} else
		inp->inp_lport = lport;

	in_pcbrehash(inp);

	return 0;
}

int
in6_pcbsetport(laddr, inp, p)
	struct sockaddr_in6 *laddr;
	struct inpcb *inp;
	struct proc *p;
{
	struct socket *so = inp->inp_socket;
	struct inpcbtable *table = inp->inp_table;
	u_int16_t first, last, old = 0;
	u_int16_t *lastport = &inp->inp_table->inpt_lastport;
	u_int16_t lport = 0;
	int count;
	int loopcount = 0;
	int wild = INPLOOKUP_IPV6;
	int error;

	/* XXX we no longer support IPv4 mapped address, so no tweaks here */

	if ((so->so_options & (SO_REUSEADDR|SO_REUSEPORT)) == 0 &&
	    ((so->so_proto->pr_flags & PR_CONNREQUIRED) == 0 ||
	     (so->so_options & SO_ACCEPTCONN) == 0))
		wild |= INPLOOKUP_WILDCARD;

	if (inp->inp_flags & INP_HIGHPORT) {
		first = ipport_hifirstauto;	/* sysctl */
		last = ipport_hilastauto;
	} else if (inp->inp_flags & INP_LOWPORT) {
		if ((error = suser(p->p_ucred, &p->p_acflag)))
			return (EACCES);
		first = IPPORT_RESERVED-1; /* 1023 */
		last = 600;		   /* not IPPORT_RESERVED/2 */
	} else {
		first = ipport_firstauto;	/* sysctl */
		last  = ipport_lastauto;
	}

	/*
	 * Simple check to ensure all ports are not used up causing
	 * a deadlock here.
	 *
	 * We split the two cases (up and down) so that the direction
	 * is not being tested on each round of the loop.
	 */

portloop:
	if (first > last) {
		/*
		 * counting down
		 */
		if (loopcount == 0) {	/* only do this once. */
			old = first;
			first -= (arc4random() % (first - last));
		}
		count = first - last;
		*lastport = first;		/* restart each time */

		do {
			if (count-- <= 0) {	/* completely used? */
				if (loopcount == 0) {
					last = old;
					loopcount++;
					goto portloop;
				}
				return (EADDRNOTAVAIL);
			}
			--*lastport;
			if (*lastport > first || *lastport < last)
				*lastport = first;
			lport = htons(*lastport);
		} while (in_baddynamic(*lastport, so->so_proto->pr_protocol) ||
		    in_pcblookup(table, (void *)&sa6_any, 0,
		    &inp->in6p_lsa, lport, wild));
	} else {
		/*
		 * counting up
		 */
		if (loopcount == 0) {	/* only do this once. */
			old = first;
			first += (arc4random() % (last - first));
		}
		count = last - first;
		*lastport = first;		/* restart each time */

		do {
			if (count-- <= 0) {	/* completely used? */
				if (loopcount == 0) {
					first = old;
					loopcount++;
					goto portloop;
				}
				return (EADDRNOTAVAIL);
			}
			++*lastport;
			if (*lastport < first || *lastport > last)
				*lastport = first;
			lport = htons(*lastport);
		} while (in_baddynamic(*lastport, so->so_proto->pr_protocol) ||
		    in_pcblookup(table, (void *)&sa6_any, 0,
		    &inp->in6p_lsa, lport, wild));
	}

	inp->inp_lport = lport;
	in_pcbrehash(inp);

	return 0;
}

/*
 * Connect from a socket to a specified address.
 * Both address and port must be specified in argument sin6.
 * Eventually, flow labels will have to be dealt with here, as well.
 *
 * If don't have a local address for this socket yet,
 * then pick one.
 *
 * I believe this has to be called at splnet().
 */
int
in6_pcbconnect(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in6 *src6 = NULL;
#ifndef SCOPEDROUTING
	struct sockaddr_in6 src6_storage;
#endif
	struct sockaddr_in6 *sin6 = mtod(nam, struct sockaddr_in6 *);
	struct ifnet *ifp = NULL;	/* outgoing interface */
	int error = 0;
	struct sockaddr_in6 tmp, mapped_sa6;

	(void)&src6;				/* XXX fool gcc */

	if (nam->m_len != sizeof(*sin6))
		return(EINVAL);
	if (sin6->sin6_family != AF_INET6)
		return(EAFNOSUPPORT);
	if (sin6->sin6_port == 0)
		return(EADDRNOTAVAIL);

	/* reject IPv4 mapped address, we have no support for it */
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr))
		return EADDRNOTAVAIL;

	/* sanity check for mapped address case */
	if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr)) {
		if (SA6_IS_ADDR_UNSPECIFIED(&inp->in6p_lsa))
			inp->inp_laddr6.s6_addr16[5] = htons(0xffff);
		if (!IN6_IS_ADDR_V4MAPPED(&inp->inp_laddr6))
			return EINVAL;
	} else {
		if (IN6_IS_ADDR_V4MAPPED(&inp->inp_laddr6))
			return EINVAL;
	}

	/* protect *sin6 from overwrites */
	tmp = *sin6;
	sin6 = &tmp;

	if ((error = scope6_check_id(sin6, ip6_use_defzone)) != 0)
		return(error);

	/* Source address selection. */
	if (IN6_IS_ADDR_V4MAPPED(&inp->inp_laddr6) &&
	    inp->inp_laddr6.s6_addr32[3] == 0) {
		struct sockaddr_in sin, *sinp;

		bzero(&sin, sizeof(sin));
		sin.sin_len = sizeof(sin);
		sin.sin_family = AF_INET;
		bcopy(&sin6->sin6_addr.s6_addr32[3], &sin.sin_addr,
		    sizeof(sin.sin_addr));
		sinp = in_selectsrc(&sin, (struct route *)&inp->inp_route6,
		    inp->inp_socket->so_options, NULL, &error);
		if (sinp == 0) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			return(error);
		}
		bzero(&mapped_sa6, sizeof(mapped_sa6));
		mapped_sa6.sin6_family = AF_INET6;
		mapped_sa6.sin6_len = sizeof(mapped_sa6);
		mapped_sa6.sin6_addr.s6_addr16[5] = htons(0xffff);
		bcopy(&sinp->sin_addr, &mapped_sa6.sin6_addr.s6_addr32[3],
		      sizeof(sinp->sin_addr));
		src6 = &mapped_sa6;
	} else {
		/*
		 * XXX: in6_selectsrc might replace the bound local address
		 * with the address specified by setsockopt(IPV6_PKTINFO).
		 * Is it the intended behavior?
		 */
		src6 = in6_selectsrc(sin6, inp->inp_outputopts6,
		    inp->inp_moptions6, &inp->inp_route6, &inp->in6p_lsa,
		    &ifp, &error);
		if (src6 == 0) {
			if (error == 0)
				error = EADDRNOTAVAIL;
			return(error);
		}
		if (ifp && sin6->sin6_scope_id == 0 &&
		    (error = scope6_setzoneid(ifp, sin6)) != 0) {
			return(error);
		}
#ifndef SCOPEDROUTING	 /* XXX: addr6 may not have a valid zone id */
		src6_storage = *src6;
		if ((error = in6_recoverscope(&src6_storage,
					      &src6->sin6_addr, NULL)) != 0) {
			return (error);
		}
		/* XXX: also recover the embedded zone ID */
		src6_storage.sin6_addr = src6->sin6_addr;
		src6 = &src6_storage;
#endif
	}
	if (ifp == NULL && inp->inp_route6.ro_rt)
		ifp = inp->inp_route6.ro_rt->rt_ifp;

	inp->inp_ipv6.ip6_hlim = (u_int8_t)in6_selecthlim(inp, ifp);

	if (in_pcblookup(inp->inp_table, sin6, sin6->sin6_port,
	    SA6_IS_ADDR_UNSPECIFIED(&inp->in6p_lsa) ? src6 : &inp->in6p_lsa,
	    inp->inp_lport, INPLOOKUP_IPV6)) {
		return(EADDRINUSE);
	}
	if (SA6_IS_ADDR_UNSPECIFIED(&inp->in6p_lsa) ||
	    (IN6_IS_ADDR_V4MAPPED(&inp->inp_laddr6) &&
	     inp->inp_laddr6.s6_addr32[3] == 0)) {
		if (inp->inp_lport == 0)
			(void)in6_pcbbind(inp, (struct mbuf *)0);
		inp->inp_laddr6 = src6->sin6_addr;
	}
	sa6_copy_addr(sin6, &inp->in6p_fsa);
	inp->inp_fport = sin6->sin6_port;
	/* update flowinfo - draft-itojun-ipv6-flowlabel-api-00 */
	inp->inp_flowinfo &= ~IPV6_FLOWLABEL_MASK;
	if (inp->inp_flags & IN6P_AUTOFLOWLABEL)
		inp->inp_flowinfo |=
		    (htonl(ip6_flow_seq++) & IPV6_FLOWLABEL_MASK);
	in_pcbrehash(inp);
	return(0);
}

/*
 * Pass some notification to all connections of a protocol
 * associated with address dst.  The local address and/or port numbers
 * may be specified to limit the search.  The "usual action" will be
 * taken, depending on the ctlinput cmd.  The caller must filter any
 * cmds that are uninteresting (e.g., no error in the map).
 * Call the protocol specific routine (if any) to report
 * any errors for each matching socket.
 *
 * Also perform input-side security policy check
 *    once PCB to be notified has been located.
 *
 * Must be called at splnet.
 */
int
in6_pcbnotify(head, dst, fport_arg, src, lport_arg, cmd, cmdarg, notify)
	struct inpcbtable *head;
	struct sockaddr *dst, *src;
	uint fport_arg;
	uint lport_arg;
	int cmd;
	void *cmdarg;
	void (*notify)(struct inpcb *, int);
{
	struct inpcb *inp, *ninp;
	u_short fport = fport_arg, lport = lport_arg;
	struct sockaddr_in6 sa6_src, *sa6_dst;
	int errno, nmatch = 0;
	u_int32_t flowinfo;

	if ((unsigned)cmd > PRC_NCMDS || dst->sa_family != AF_INET6)
		return 1;

	sa6_dst = (struct sockaddr_in6 *)dst;
	if (SA6_IS_ADDR_UNSPECIFIED(sa6_dst))
		return 1;
	if (IN6_IS_ADDR_V4MAPPED(&sa6_dst->sin6_addr))
		printf("Huh?  Thought in6_pcbnotify() never got "
		       "called with mapped!\n");

	/*
	 * note that src can be NULL when we get notify by local fragmentation.
	 */
	sa6_src = (src == NULL) ? sa6_any : *(struct sockaddr_in6 *)src;
	flowinfo = sa6_src.sin6_flowinfo;

	/*
	 * Redirects go to all references to the destination,
	 * and use in_rtchange to invalidate the route cache.
	 * Dead host indications: also use in_rtchange to invalidate
	 * the cache, and deliver the error to all the sockets.
	 * Otherwise, if we have knowledge of the local port and address,
	 * deliver only to that socket.
	 */
	if (PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) {
		fport = 0;
		lport = 0;
		sa6_src.sin6_addr = in6addr_any;

		if (cmd != PRC_HOSTDEAD)
			notify = in_rtchange;
	}
	errno = inet6ctlerrmap[cmd];

	for (inp = head->inpt_queue.cqh_first;
	     inp != (struct inpcb *)&head->inpt_queue; inp = ninp) {
		ninp = inp->inp_queue.cqe_next;

		if ((inp->inp_flags & INP_IPV6) == 0)
			continue;

		/*
		 * If the error designates a new path MTU for a destination
		 * and the application (associated with this socket) wanted to
		 * know the value, notify. Note that we notify for all
		 * disconnected sockets if the corresponding application
		 * wanted. This is because some UDP applications keep sending
		 * sockets disconnected.
		 * XXX: should we avoid to notify the value to TCP sockets?
		 */
		if (cmd == PRC_MSGSIZE && (inp->inp_flags & IN6P_MTU) != 0 &&
		    (SA6_IS_ADDR_UNSPECIFIED(&inp->in6p_fsa) ||
		     SA6_ARE_ADDR_EQUAL(&inp->in6p_fsa,
					sa6_dst))) {
			ip6_notify_pmtu(inp, (struct sockaddr_in6 *)dst,
			    (u_int32_t *)cmdarg);
		}

		/*
		 * XXX See comment in NetBSD sys/netinet6/in6_pcb.c
		 */
		if ((PRC_IS_REDIRECT(cmd) || cmd == PRC_HOSTDEAD) &&
		    SA6_IS_ADDR_UNSPECIFIED(&inp->in6p_lsa) &&
		    inp->inp_route.ro_rt &&
		    !(inp->inp_route.ro_rt->rt_flags & RTF_HOST)) {
			struct sockaddr_in6 *dst6;

			dst6 = (struct sockaddr_in6 *)&inp->inp_route.ro_dst;
#ifdef SCOPEDROUTING		/* XXX */
			if (SA6_ARE_ADDR_EQUAL(dst6, sa6_dst))
#else
			if (IN6_ARE_ADDR_EQUAL(&dst6->sin6_addr,
			    &sa6_dst->sin6_addr))
#endif
				goto do_notify;
		}

		/*
		 * Detect if we should notify the error. If no source and
		 * destination ports are specifed, but non-zero flowinfo and
		 * local address match, notify the error. This is the case
		 * when the error is delivered with an encrypted buffer
		 * by ESP. Otherwise, just compare addresses and ports
		 * as usual.
		 */
		if (lport == 0 && fport == 0 && flowinfo &&
		    inp->inp_socket != NULL &&
		    flowinfo == (inp->inp_flowinfo & IPV6_FLOWLABEL_MASK) &&
		    SA6_ARE_ADDR_EQUAL(&inp->in6p_lsa, &sa6_src))
			goto do_notify;
		else if (!SA6_ARE_ADDR_EQUAL(&inp->in6p_fsa, sa6_dst) ||
			 inp->inp_socket == 0 ||
			 (lport && inp->inp_lport != lport) ||
			 (!SA6_IS_ADDR_UNSPECIFIED(&sa6_src) &&
			  !SA6_ARE_ADDR_EQUAL(&inp->in6p_lsa, &sa6_src)) ||
			 (fport && inp->inp_fport != fport)) {
			continue;
		}
	  do_notify:
		nmatch++;
		if (notify)
			(*notify)(inp, errno);
	}
	return 0;
}

/*
 * Get the local address/port, and put it in a sockaddr_in6.
 * This services the getsockname(2) call.
 */
int
in6_setsockaddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in6 *sin6;

	nam->m_len = sizeof(struct sockaddr_in6);
	sin6 = mtod(nam,struct sockaddr_in6 *);

	bzero ((caddr_t)sin6,sizeof(struct sockaddr_in6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_port = inp->inp_lport;
	sa6_copy_addr(&inp->in6p_lsa, sin6);
#ifndef SCOPEDROUTING
	in6_clearscope(&sin6->sin6_addr);
#endif

	return 0;
}

/*
 * Get the foreign address/port, and put it in a sockaddr_in6.
 * This services the getpeername(2) call.
 */
int
in6_setpeeraddr(inp, nam)
	struct inpcb *inp;
	struct mbuf *nam;
{
	struct sockaddr_in6 *sin6;

	nam->m_len = sizeof(struct sockaddr_in6);
	sin6 = mtod(nam,struct sockaddr_in6 *);

	bzero ((caddr_t)sin6,sizeof(struct sockaddr_in6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_port = inp->inp_fport;
	sa6_copy_addr(&inp->in6p_fsa, sin6);
#ifndef SCOPEDROUTING
	in6_clearscope(&sin6->sin6_addr);
#endif

	return 0;
}