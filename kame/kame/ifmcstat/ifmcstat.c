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

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <kvm.h>
#include <nlist.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/queue.h>

#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
# include <net/if_var.h>
#endif
#include <net/if_types.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <netinet/igmp.h>
#ifdef IGMP_v3_MEMBERSHIP_REPORT
#include <netinet/in_msf.h>
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
#include <net/route.h>
#include <netinet6/in6_msf.h>
#endif
#ifndef __NetBSD__
# ifdef	__FreeBSD__
#  define	KERNEL
# endif
# include <netinet/if_ether.h>
# ifdef	__FreeBSD__
#  undef	KERNEL
# endif
#else
# include <net/if_ether.h>
#endif
#include <netinet/in_var.h>
#include <netinet/icmp6.h>
#include <netinet6/mld6_var.h>
#include <arpa/inet.h>

#include <netdb.h>

kvm_t	*kvmd;

struct	nlist nl[] = {
#define	N_IFNET	0
	{ "_ifnet" },
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
#define N_IN6_MK 1
	{ "_in6_mk" },
#endif
	{ "" },
};

const char *inet6_n2a __P((struct sockaddr_in6 *));
int main __P((int, char **));
char *ifname __P((struct ifnet *));
void kread __P((u_long, void *, int));
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
void acmc __P((struct ether_multi *));
#endif
void if6_addrlist __P((struct ifaddr *));
void in6_multilist __P((struct in6_multi *));
struct in6_multi * in6_multientry __P((struct in6_multi *));
void if_addrlist(struct ifaddr *);
void in_multilist(struct in_multi *);
struct in_multi * in_multientry(struct in_multi *);
#ifdef IGMP_v3_MEMBERSHIP_REPORT
void in_addr_slistentry(struct in_addr_slist *ias, char *heading);
#endif
#ifdef MCAST_JOIN_SOURCE_GROUP
void in6_addr_slistentry(struct in6_addr_slist *ias, char *heading);
#endif

#if !defined(__NetBSD__) && !(defined(__FreeBSD__) && __FreeBSD__ >= 3) && !defined(__OpenBSD__)
#ifdef __bsdi__
struct ether_addr {
	u_int8_t ether_addr_octet[6];
};
#endif
static char *ether_ntoa __P((struct ether_addr *));
#endif

#define	KREAD(addr, buf, type) \
	kread((u_long)addr, (void *)buf, sizeof(type))

#ifdef N_IN6_MK
struct multi6_kludge {
	LIST_ENTRY(multi6_kludge) mk_entry;
	struct ifnet *mk_ifp;
	struct in6_multihead mk_head;
};
#endif

const char *inet6_n2a(p)
	struct sockaddr_in6 *p;
{
	static char buf[NI_MAXHOST];
	struct sockaddr_in6 sin6;
	u_int32_t scopeid;
	const int niflags = NI_NUMERICHOST;

	memset(&sin6, 0, sizeof(sin6));
	sin6 = *p;
	if (IN6_IS_ADDR_LINKLOCAL(&p->sin6_addr) ||
	    IN6_IS_ADDR_MC_LINKLOCAL(&p->sin6_addr) ||
	    IN6_IS_ADDR_MC_NODELOCAL(&p->sin6_addr)) {
		scopeid = ntohs(*(u_int16_t *)&sin6.sin6_addr.s6_addr[2]);
		if (scopeid) {
			sin6.sin6_scope_id = scopeid;
			sin6.sin6_addr.s6_addr[2] = 0;
			sin6.sin6_addr.s6_addr[3] = 0;
		}
	}
	if (getnameinfo((struct sockaddr *)&sin6, sin6.sin6_len,
			buf, sizeof(buf), NULL, 0, niflags) == 0)
		return buf;
	else
		return "(invalid)";
}

int main(argc, argv)
	int argc;
	char **argv;
{
	char	buf[_POSIX2_LINE_MAX], ifname[IFNAMSIZ];
	struct	ifnet	*ifp, *nifp, ifnet;
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
#ifndef __NetBSD__
	struct	arpcom	arpcom;
#else
	struct ethercom ec;
	struct sockaddr_dl sdl;
#endif
#endif
	const char *kernel = NULL;

	switch (argc) {
	case 1:
		kernel = NULL;
		break;
	case 2:
		kernel = argv[1];
		break;
	default:
		fprintf(stderr, "usage: ifmcstat [kernel]\n");
		exit(1);
	}

	if ((kvmd = kvm_openfiles(kernel, NULL, NULL, O_RDONLY, buf)) == NULL) {
		perror("kvm_openfiles");
		exit(1);
	}
	if (kvm_nlist(kvmd, nl) < 0) {
		perror("kvm_nlist");
		exit(1);
	}
	if (nl[N_IFNET].n_value == 0) {
		printf("symbol %s not found\n", nl[N_IFNET].n_name);
		exit(1);
	}
	KREAD(nl[N_IFNET].n_value, &ifp, struct ifnet *);
	while (ifp) {
		KREAD(ifp, &ifnet, struct ifnet);
		printf("%s:\n", if_indextoname(ifnet.if_index, ifname));

#if defined(__NetBSD__) || defined(__OpenBSD__)
		if_addrlist(ifnet.if_addrlist.tqh_first);
		if6_addrlist(ifnet.if_addrlist.tqh_first);
		nifp = ifnet.if_list.tqe_next;
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		if_addrlist(TAILQ_FIRST(&ifnet.if_addrhead));
		if6_addrlist(TAILQ_FIRST(&ifnet.if_addrhead));
		nifp = ifnet.if_link.tqe_next;
#else
		if6_addrlist(ifnet.if_addrlist);
		nifp = ifnet.if_next;
#endif

#ifdef __NetBSD__
		KREAD(ifnet.if_sadl, &sdl, struct sockaddr_dl);
		if (sdl.sdl_type == IFT_ETHER) {
			printf("\tenaddr %s",
			       ether_ntoa((struct ether_addr *)LLADDR(&sdl)));
			KREAD(ifp, &ec, struct ethercom);
			printf(" multicnt %d", ec.ec_multicnt);
			acmc(ec.ec_multiaddrs.lh_first);
			printf("\n");
		}
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		/* not supported */
#else
		if (ifnet.if_type == IFT_ETHER) {
			KREAD(ifp, &arpcom, struct arpcom);
			printf("\tenaddr %s",
			    ether_ntoa((struct ether_addr *)arpcom.ac_enaddr));
			KREAD(ifp, &arpcom, struct arpcom);
			printf(" multicnt %d", arpcom.ac_multicnt);
#ifdef __OpenBSD__
			acmc(arpcom.ac_multiaddrs.lh_first);
#else
			acmc(arpcom.ac_multiaddrs);
#endif
			printf("\n");
		}
#endif

		ifp = nifp;
	}

	exit(0);
	/*NOTREACHED*/
}

char *ifname(ifp)
	struct ifnet *ifp;
{
	static char buf[BUFSIZ];
	struct ifnet ifnet;
#if !(defined(__NetBSD__) || defined(__OpenBSD__))
	char ifnamebuf[IFNAMSIZ];
#endif

	KREAD(ifp, &ifnet, struct ifnet);
#if defined(__NetBSD__) || defined(__OpenBSD__)
	strlcpy(buf, ifnet.if_xname, sizeof(buf));
#else
	KREAD(ifnet.if_name, ifnamebuf, sizeof(ifnamebuf));
	snprintf(buf, sizeof(buf), "%s%d", ifnamebuf,
		 ifnet.if_unit); /* does snprintf allow overlap copy?? */
#endif
	return buf;
}

void kread(addr, buf, len)
	u_long addr;
	void *buf;
	int len;
{
	if (kvm_read(kvmd, addr, buf, len) != len) {
		perror("kvm_read");
		exit(1);
	}
}

#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
void acmc(am)
	struct ether_multi *am;
{
	struct ether_multi em;

	while (am) {
		KREAD(am, &em, struct ether_multi);
		
		printf("\n\t\t");
		printf("%s -- ", ether_ntoa((struct ether_addr *)em.enm_addrlo));
		printf("%s ", ether_ntoa((struct ether_addr *)&em.enm_addrhi));
		printf("%d", em.enm_refcount);
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
		am = em.enm_next;
#else
		am = em.enm_list.le_next;
#endif
	}
}
#endif

void
if6_addrlist(ifap)
	struct ifaddr *ifap;
{
	struct ifaddr ifa;
	struct sockaddr sa;
	struct in6_ifaddr if6a;
	struct in6_multi *mc = 0;
	struct ifaddr *ifap0;

	ifap0 = ifap;
	while (ifap) {
		KREAD(ifap, &ifa, struct ifaddr);
		if (ifa.ifa_addr == NULL)
			goto nextifap;
		KREAD(ifa.ifa_addr, &sa, struct sockaddr);
		if (sa.sa_family != PF_INET6)
			goto nextifap;
		KREAD(ifap, &if6a, struct in6_ifaddr);
		printf("\tinet6 %s\n", inet6_n2a(&if6a.ia_addr));
#if !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
		mc = mc ? mc : if6a.ia6_multiaddrs.lh_first;
#endif
	nextifap:
#if defined(__NetBSD__) || defined(__OpenBSD__)
		ifap = ifa.ifa_list.tqe_next;
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		ifap = ifa.ifa_link.tqe_next;
#else
		ifap = ifa.ifa_next;
#endif /* __FreeBSD__ >= 3 */
	}
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	if (ifap0) {
		struct ifnet ifnet;
		struct ifmultiaddr ifm, *ifmp = 0;
		struct sockaddr_in6 sin6;
		struct in6_multi in6m;
		struct sockaddr_dl sdl;
		int in6_multilist_done = 0;

		KREAD(ifap0, &ifa, struct ifaddr);
		KREAD(ifa.ifa_ifp, &ifnet, struct ifnet);
		if (ifnet.if_multiaddrs.lh_first)
			ifmp = ifnet.if_multiaddrs.lh_first;
		while (ifmp) {
			KREAD(ifmp, &ifm, struct ifmultiaddr);
			if (ifm.ifma_addr == NULL)
				goto nextmulti;
			KREAD(ifm.ifma_addr, &sa, struct sockaddr);
			if (sa.sa_family != AF_INET6)
				goto nextmulti;
			(void)in6_multientry((struct in6_multi *)
					     ifm.ifma_protospec);
			if (ifm.ifma_lladdr == 0)
				goto nextmulti;
			KREAD(ifm.ifma_lladdr, &sdl, struct sockaddr_dl);
			printf("\t\t\tmcast-macaddr %s multicnt %d\n",
			       ether_ntoa((struct ether_addr *)LLADDR(&sdl)),
			       ifm.ifma_refcount);
		    nextmulti:
			ifmp = ifm.ifma_link.le_next;
		}
	}
#else
	if (mc)
		in6_multilist(mc);
#endif
#ifdef N_IN6_MK
	if (nl[N_IN6_MK].n_value != 0) {
		LIST_HEAD(in6_mktype, multi6_kludge) in6_mk;
		struct multi6_kludge *mkp, mk;
		char *nam;

		KREAD(nl[N_IN6_MK].n_value, &in6_mk, struct in6_mktype);
		KREAD(ifap0, &ifa, struct ifaddr);

		nam = strdup(ifname(ifa.ifa_ifp));
		if (!nam) {
			fprintf(stderr, "ifmcstat: not enough core\n");
			exit(1);
		}

		for (mkp = in6_mk.lh_first; mkp; mkp = mk.mk_entry.le_next) {
			KREAD(mkp, &mk, struct multi6_kludge);
			if (strcmp(nam, ifname(mk.mk_ifp)) == 0 &&
			    mk.mk_head.lh_first) {
				printf("\t(on kludge entry for %s)\n", nam);
				in6_multilist(mk.mk_head.lh_first);
			}
		}

		free(nam);
	}
#endif
}

struct in6_multi *
in6_multientry(mc)
	struct in6_multi *mc;
{
	struct in6_multi multi;
#ifdef MCAST_JOIN_SOURCE_GROUP
	struct in6_multi_source src;
#endif
	struct router6_info rt6i;

	KREAD(mc, &multi, struct in6_multi);
	printf("\t\tgroup %s", inet6_n2a(&multi.in6m_sa));
	printf(" refcnt %u", multi.in6m_refcount);

	KREAD(multi.in6m_rti, &rt6i, struct router_info);
	switch (rt6i.rt6i_type) {
	case MLD_LISTENER_REPORT:
		printf("\tmld_ver 1");
		break;
#ifdef MCAST_JOIN_SOURCE_GROUP
	case MLDV2_LISTENER_REPORT:
		printf("\tmld_ver 2");
		break;
#endif
	default:
		printf("\tmld_ver ?(%d)", rt6i.rt6i_type);
		break;
	}


#ifdef MCAST_JOIN_SOURCE_GROUP
	if (multi.in6m_source == NULL) {
		printf("\n");
		return(multi.in6m_entry.le_next);
	}

	KREAD(multi.in6m_source, &src, struct in6_multi_source);
	printf("\tmode=%s\tgroup join=%d\n",
		src.i6ms_mode == MCAST_INCLUDE ? "include" :
		src.i6ms_mode == MCAST_EXCLUDE ? "exclude" :
		"???",
		src.i6ms_grpjoin);
	in6_addr_slistentry(src.i6ms_cur, "current");
	in6_addr_slistentry(src.i6ms_rec, "recorded");
	in6_addr_slistentry(src.i6ms_in, "included");
	in6_addr_slistentry(src.i6ms_ex, "excluded");
	in6_addr_slistentry(src.i6ms_alw, "allowed");
	in6_addr_slistentry(src.i6ms_blk, "blocked");
	in6_addr_slistentry(src.i6ms_toin, "to-include");
	in6_addr_slistentry(src.i6ms_ex, "to-exclude");
#else
	printf("\n");
#endif
	return(multi.in6m_entry.le_next);
}

#ifdef MCAST_JOIN_SOURCE_GROUP
void
in6_addr_slistentry(struct in6_addr_slist *ias, char *heading)
{
	struct in6_addr_slist slist;
	struct i6as_head head;
	struct in6_addr_source src;

	if (ias == NULL) {
		printf("\t\t\t%s (none)\n", heading);
		return;
	}
	memset(&slist, 0, sizeof(slist));
	KREAD(ias, &slist, struct in6_addr_source);
	printf("\t\t\t%s (entry num=%d)\n", heading, slist.numsrc);
	if (slist.numsrc == 0) {
		return;
	}
	KREAD(slist.head, &head, struct i6as_head);

	KREAD(head.lh_first, &src, struct in6_addr_source);
	while (1) {
		printf("\t\t\t\tsource %s (ref=%d)\n",
			inet6_n2a(&src.i6as_addr),
			src.i6as_refcount);
		if (src.i6as_list.le_next == NULL)
			break;
		KREAD(src.i6as_list.le_next, &src, struct in6_addr_source);
	}
	return;
}
#endif

void
in6_multilist(mc)
	struct in6_multi *mc;
{
	while (mc)
		mc = in6_multientry(mc);
}

void
if_addrlist(ifap)
	struct ifaddr *ifap;
{
	struct ifaddr ifa;
	struct sockaddr sa;
	struct in_ifaddr ia;
	struct in_multi *mc = 0;
	struct ifaddr *ifap0;

	ifap0 = ifap;
	while (ifap) {
		KREAD(ifap, &ifa, struct ifaddr);
		if (ifa.ifa_addr == NULL)
			goto nextifap;
		KREAD(ifa.ifa_addr, &sa, struct sockaddr);
		if (sa.sa_family != PF_INET)
			goto nextifap;
		KREAD(ifap, &ia, struct in_ifaddr);
		printf("\tinet %s\n", inet_ntoa(ia.ia_addr.sin_addr));
#ifdef __bsdi__
		mc = mc ? mc : ia.ia_multiaddrs;
#elif !(defined(__FreeBSD__) && __FreeBSD__ >= 3)
		mc = mc ? mc : ia.ia_multiaddrs.lh_first;
#endif
	nextifap:
#if defined(__NetBSD__) || defined(__OpenBSD__)
		ifap = ifa.ifa_list.tqe_next;
#elif defined(__FreeBSD__) && __FreeBSD__ >= 3
		ifap = ifa.ifa_link.tqe_next;
#else
		ifap = ifa.ifa_next;
#endif /* __FreeBSD__ >= 3 */
	}
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
	if (ifap0) {
		struct ifnet ifnet;
		struct ifmultiaddr ifm, *ifmp = 0;
		struct sockaddr_in sin;
		struct in_multi inm;
		struct sockaddr_dl sdl;
		int in_multilist_done = 0;

		KREAD(ifap0, &ifa, struct ifaddr);
		KREAD(ifa.ifa_ifp, &ifnet, struct ifnet);
		if (ifnet.if_multiaddrs.lh_first)
			ifmp = ifnet.if_multiaddrs.lh_first;
		while (ifmp) {
			KREAD(ifmp, &ifm, struct ifmultiaddr);
			if (ifm.ifma_addr == NULL)
				goto nextmulti;
			KREAD(ifm.ifma_addr, &sa, struct sockaddr);
			if (sa.sa_family != AF_INET)
				goto nextmulti;
			(void)in_multientry((struct in_multi *)
					    ifm.ifma_protospec);
			if (ifm.ifma_lladdr == 0)
				goto nextmulti;
			KREAD(ifm.ifma_lladdr, &sdl, struct sockaddr_dl);
			printf("\t\t\tmcast-macaddr %s multicnt %d\n",
			       ether_ntoa((struct ether_addr *)LLADDR(&sdl)),
			       ifm.ifma_refcount);
		    nextmulti:
			ifmp = ifm.ifma_link.le_next;
		}
	}
#else
	if (mc)
		in_multilist(mc);
#endif
}

void
in_multilist(mc)
	struct in_multi *mc;
{
	while (mc)
		mc = in_multientry(mc);
}

struct in_multi *
in_multientry(mc)
	struct in_multi *mc;
{
	struct in_multi multi;
	struct router_info rti;
#ifdef IGMP_v3_MEMBERSHIP_REPORT
	struct in_multi_source src;
#endif

	KREAD(mc, &multi, struct in_multi);
	printf("\t\tgroup %s", inet_ntoa(multi.inm_addr));

	KREAD(multi.inm_rti, &rti, struct router_info);
	switch (rti.rti_type) {
	case IGMP_v1_MEMBERSHIP_REPORT:
		printf("\tigmp_ver 1");
		break;
	case IGMP_v2_MEMBERSHIP_REPORT:
		printf("\tigmp_ver 2");
		break;
#ifdef IGMP_v3_MEMBERSHIP_REPORT
	case IGMP_v3_MEMBERSHIP_REPORT:
		printf("\tigmp_ver 3");
		break;
#endif
	default:
		printf("\tigmp_ver ?(%d)", rti.rti_type);
		break;
	}

#ifdef IGMP_v3_MEMBERSHIP_REPORT
	if (multi.inm_source == NULL) {
		printf("\n");
		return(multi.inm_list.le_next);
	}

	KREAD(multi.inm_source, &src, struct in_multi_source);
	printf("\tmode=%s\tgroup join=%d\n",
		src.ims_mode == MCAST_INCLUDE ? "include" :
		src.ims_mode == MCAST_EXCLUDE ? "exclude" :
		"???",
		src.ims_grpjoin);
	in_addr_slistentry(src.ims_cur, "current");
	in_addr_slistentry(src.ims_rec, "recorded");
	in_addr_slistentry(src.ims_in, "included");
	in_addr_slistentry(src.ims_ex, "excluded");
	in_addr_slistentry(src.ims_alw, "allowed");
	in_addr_slistentry(src.ims_blk, "blocked");
	in_addr_slistentry(src.ims_toin, "to-include");
	in_addr_slistentry(src.ims_ex, "to-exclude");
#else
	printf("\n");
#endif

#ifdef __bsdi__
	return(multi.inm_next);
#else
	return(multi.inm_list.le_next);
#endif
}

#ifdef IGMP_v3_MEMBERSHIP_REPORT
void
in_addr_slistentry(struct in_addr_slist *ias, char *heading)
{
	struct in_addr_slist slist;
	struct ias_head head;
	struct in_addr_source src;

	if (ias == NULL) {
		printf("\t\t\t%s (none)\n", heading);
		return;
	}
	memset(&slist, 0, sizeof(slist));
	KREAD(ias, &slist, struct in_addr_source);
	printf("\t\t\t%s (entry num=%d)\n", heading, slist.numsrc);
	if (slist.numsrc == 0) {
		return;
	}
	KREAD(slist.head, &head, struct ias_head);

	KREAD(head.lh_first, &src, struct in_addr_source);
	while (1) {
		printf("\t\t\t\tsource %s (ref=%d)\n",
			inet_ntoa(src.ias_addr.sin_addr), src.ias_refcount);
		if (src.ias_list.le_next == NULL)
			break;
		KREAD(src.ias_list.le_next, &src, struct in_addr_source);
	}
	return;
}
#endif


#if !defined(__NetBSD__) && !(defined(__FreeBSD__) && __FreeBSD__ >= 3) && !defined(__OpenBSD__)
static char *
ether_ntoa(e)
	struct ether_addr *e;
{
	static char buf[20];
	u_char *p;

	p = (u_char *)e;

	snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
		p[0], p[1], p[2], p[3], p[4], p[5]);
	return buf;
}
#endif
