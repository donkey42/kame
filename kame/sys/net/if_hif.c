/*	$KAME: if_hif.c,v 1.1 2001/08/03 10:40:19 keiichi Exp $	*/

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
 * Copyright (c) 1999, 2000 and 2001 Ericsson Radio Systems AB
 * All rights reserved.
 *
 * Authors: Conny Larsson <Conny.Larsson@era.ericsson.se>
 *          Mattias Pettersson <Mattias.Pettersson@era.ericsson.se>
 *
 */

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project and InternetCAR Projec\
t.
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
 *
 * Ryuji Wakikawa, Koshiro Mitsuya, Susumu Koshiba, Masashi Watari
 * Keio University, Endo 5322, Kanagawa, Japan
 * E-mail: mip6@sfc.wide.ad.jp
 *
 */


#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include "opt_inet.h"
#include "opt_inet6.h"
#endif
#ifdef __NetBSD__
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <sys/malloc.h>
#endif
#include <sys/mbuf.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/errno.h>
#if defined(__FreeBSD__) || __FreeBSD__ >= 3
/*nothing*/
#else
#include <sys/ioctl.h>
#endif
#include <sys/time.h>
#include <sys/syslog.h>
#include <sys/protosw.h>
#include <machine/cpu.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/netisr.h>
#include <net/route.h>
#include <net/bpf.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef	INET
#include <netinet/in_var.h>
#endif	/* INET */

#ifdef INET6
#ifndef INET
#include <netinet/in.h>
#endif
#include <netinet6/in6_var.h>
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/ip6protosw.h>
#endif /* INET6 */

#if defined(__NetBSD__) && defined(ISO)
#include <netiso/iso.h>
#include <netiso/iso_var.h>
#endif

#include <netinet/ip_encap.h>
#include <net/if_hif.h>

#include <netinet6/mip6.h>

#include "hif.h"
#if defined(__FreeBSD__) && __FreeBSD__ >= 4
#include "bpf.h"
#define NBPFILTER	NBPF
#else
#include "bpfilter.h"
#endif

#include <net/net_osdep.h>

#if NHIF > 0

extern struct mip6_subnet_list mip6_subnet_list;
extern struct mip6_prefix_list mip6_prefix_list;

static int hif_subnet_list_update_withmpfx __P((struct hif_softc *, caddr_t));
static int hif_ha_list_update_withioctl __P((struct hif_softc *, caddr_t));

#ifdef __FreeBSD__
void hif_attach __P((void *));
#else
void hif_attach __P((int));
#endif
static void hif_rtrequest __P((int, struct rtentry *, struct sockaddr *));

#ifdef __FreeBSD__
PSEUDO_SET(hif_attach, if_hif);
#endif

void
hif_attach(dummy)
#ifdef __FreeBSD__
	void *dummy;
#else
	int dummy;
#endif
{
	struct hif_softc *sc;
	int i;

	TAILQ_INIT(&hif_softc_list);
	TAILQ_INIT(&hif_coa_list);
	hif_coa = in6addr_any;

	sc = malloc(NHIF * sizeof(struct hif_softc), M_DEVBUF, M_WAIT);
	bzero(sc, NHIF * sizeof(struct hif_softc));
	for (i = 0 ; i < NHIF; sc++, i++) {
#if defined(__NetBSD__) || defined(__OpenBSD__)
		sprintf(sc->hif_if.if_xname, "hif%d", i);
#else
		sc->hif_if.if_name = "hif";
		sc->hif_if.if_unit = i;
#endif
		sc->hif_if.if_flags = IFF_MULTICAST | IFF_SIMPLEX;
		sc->hif_if.if_mtu = HIF_MTU;
		sc->hif_if.if_ioctl = hif_ioctl;
		sc->hif_if.if_output = hif_output;
		sc->hif_if.if_type = IFT_HIF;
#if defined(__FreeBSD__) && __FreeBSD__ >= 4
		IFQ_SET_MAXLEN(&sc->hif_if.if_snd, ifqmaxlen);
		IFQ_SET_READY(&sc->hif_if.if_snd);
#endif
		if_attach(&sc->hif_if);
#if NBPFILTER > 0
#ifdef HAVE_OLD_BPF
		bpfattach(&sc->hif_if, DLT_NULL, sizeof(u_int));
#else
		bpfattach(&sc->hif_if.if_bpf, &sc->mip6_if, DLT_NULL, sizeof(u_int));
#endif /* HAVE_OLD_BPF */
#endif /* NBPFILTER > 0 */

		sc->hif_location = HIF_LOCATION_UNKNOWN;

		/* Initialize home prefix list, HA list, BU list */
#ifdef HIFHR
		LIST_INIT(&sc->hif_hr_list);
#endif /* HIFHR */
		LIST_INIT(&sc->hif_pfx_list);
		LIST_INIT(&sc->hif_bu_list);
		LIST_INIT(&sc->hif_ha_list);

		TAILQ_INIT(&sc->hif_hs_list_home);
		TAILQ_INIT(&sc->hif_hs_list_foreign);
		sc->hif_hs_current = NULL;
		sc->hif_hs_prev = NULL;

		sc->hif_hadiscovid = 0;

		/* create hif_softc list */
		TAILQ_INSERT_TAIL(&hif_softc_list, sc, hif_entry);
	}
}

int
hif_ioctl(ifp, cmd, data)
	struct ifnet *ifp;
#if defined(__FreeBSD__) && __FreeBSD__ < 3
	int cmd;
#else
	u_long cmd;
#endif
	caddr_t data;
{
	int s;
	struct hif_softc *sc = (struct hif_softc *)ifp;
	struct hif_ifreq *ifr = (struct hif_ifreq *)data;
	struct ifaddr *ifa = (struct ifaddr *)data;
	int error = 0;

#ifdef __NetBSD__
	s = splsoftnet();
#else
	s = splnet();
#endif

	switch(cmd) {
	case SIOCSIFADDR:
		ifp->if_flags |= IFF_UP | IFF_RUNNING;
		ifa = (struct ifaddr *)data;
		ifa->ifa_rtrequest = hif_rtrequest;
		/*
		 * Everything else is done at a higher level.
		 */
		break;

	case SIOCAHOMEPREFIX_HIF:
		error = hif_subnet_list_update_withmpfx(sc, data);
		break;

	case SIOCGHOMEPREFIX_HIF:
		{
			struct hif_subnet *hs;
			struct mip6_subnet *ms;
			struct mip6_subnet_prefix *mspfx;
			struct mip6_prefix *mpfx = ifr->ifr_ifru.ifr_mpfx;
			int i;

			i = 0;
			TAILQ_FOREACH(hs, &sc->hif_hs_list_home, hs_entry) {
				ms = hs->hs_ms;
				if (ms == NULL) {
					return (EINVAL);
				}
				TAILQ_FOREACH(mspfx, &ms->ms_mspfx_list,
					      mspfx_entry) {
					if (mspfx->mspfx_mpfx == NULL) {
						return (EINVAL);
					}
					*mpfx = *mspfx->mspfx_mpfx;
					i++;
					if (i > ifr->ifr_count)
						goto ghomeprefix_end;
					mpfx++;
				}
			}
		ghomeprefix_end:
			ifr->ifr_count = i;
		}
		
		break;

	case SIOCAHOMEAGENT_HIF:
		error = hif_ha_list_update_withioctl(sc, data);
		break;

	case SIOCGHOMEAGENT_HIF:
		{
			struct hif_subnet_list *hs_list;
			struct hif_subnet *hs;
			struct mip6_subnet *ms;
			struct mip6_subnet_ha *msha;
			struct mip6_ha *mha = ifr->ifr_ifru.ifr_mha;
			int i;

			i = 0;
			hs_list = &sc->hif_hs_list_home;
			TAILQ_FOREACH(hs, hs_list, hs_entry) {
				ms = hs->hs_ms;
				TAILQ_FOREACH(msha, &ms->ms_msha_list, msha_entry) {
					*mha = *msha->msha_mha;
					i++;
					if (i > ifr->ifr_count)
						goto ghomeagent_done;
					mha++;
				}
			}
			hs_list = &sc->hif_hs_list_foreign;
			TAILQ_FOREACH(hs, hs_list, hs_entry) {
				ms = hs->hs_ms;
				TAILQ_FOREACH(msha, &ms->ms_msha_list, msha_entry) {
					*mha = *msha->msha_mha;
					i++;
					if (i > ifr->ifr_count)
						goto ghomeagent_done;
					mha++;
				}
			}
		ghomeagent_done:
			ifr->ifr_count = i;
		}
		break;

	case SIOCGBU_HIF:
		{
			struct mip6_bu *tmpmbu;
			struct mip6_bu *mbu = ifr->ifr_ifru.ifr_mbu;
			int i;

			i = 0;
			LIST_FOREACH(tmpmbu, &sc->hif_bu_list, mbu_entry) {
				*mbu = *tmpmbu;
				i++;
				if (i > ifr->ifr_count)
					break;
				mbu++;
			}
			ifr->ifr_count = i;
		}
		break;
	default:
		error = EINVAL;
		break;
	}

	splx(s);

	return (error);
}

struct hif_subnet *
hif_subnet_create(ms)
     struct mip6_subnet *ms;
{
	struct hif_subnet *hs;

	MALLOC(hs, struct hif_subnet *, sizeof(struct hif_subnet),
	       M_TEMP, M_NOWAIT);
	if (hs == NULL) {
		mip6log((LOG_ERR, "%s: hif_subnet memory allocation failed.\n",
			 __FUNCTION__));
		return (NULL);
	}
	hs->hs_ms = ms;

	return (hs);
}

int
hif_subnet_list_insert(hs_list, hs)
     struct hif_subnet_list *hs_list;
     struct hif_subnet *hs;
{
	struct mip6_subnet* ms;

	if ((hs_list == NULL) || (hs == NULL) || (hs->hs_ms == NULL)) {
		return (EINVAL);
	}

	ms = hs->hs_ms;
	if (ms ==  NULL) {
		mip6log((LOG_ERR, "%s: invalid mip6_subnet pointer.\n",
			 __FUNCTION__));
		return (EINVAL);
	}

	ms->ms_refcnt++;
	TAILQ_INSERT_TAIL(hs_list, hs, hs_entry);

	return (0);
}

int
hif_subnet_list_remove(hs_list, hs)
     struct hif_subnet_list *hs_list;
     struct hif_subnet *hs;
{
	struct mip6_subnet *ms;

	if ((hs_list == NULL) || (hs == NULL) || (hs->hs_ms == NULL)) {
		return (EINVAL);
	}
	
	ms = hs->hs_ms;
	TAILQ_REMOVE(hs_list, hs, hs_entry);
	FREE(hs, M_TEMP);

	/*
	 * do not remove mip6_subnet pointed from this hif_subnet,
	 * because mip6_subnet may be pointed from another hif_subnet.
	 * only refcnt is decremented.
	 * mip6_subnet is deleted by timer function.
	 */
	ms->ms_refcnt--;
	if (ms->ms_refcnt < 0) {
		/* must not happen. */
		return (EINVAL);
	}

	return (0);
}

struct hif_subnet *
hif_subnet_list_find_withprefix(hs_list, prefix, prefixlen)
     struct hif_subnet_list *hs_list;
     struct in6_addr *prefix;
     u_int8_t prefixlen;
{
	struct hif_subnet *hs;
	struct mip6_subnet *ms;

	/*
	 * walk hif_subnet_list and check each mip6_subnet (which is a
	 * member of hif_subnet as a pointer) if it contains specified
	 * prefix or not.
	 */
	TAILQ_FOREACH(hs, hs_list, hs_entry) {
		if ((ms = hs->hs_ms) == NULL) {
			/* this must not happen. */
			mip6log((LOG_ERR,
				 "%s: hs_ms is a NULL pointer.\n",
				 __FUNCTION__));
			return (NULL);
		}
		if (mip6_subnet_prefix_list_find_withprefix(&ms->ms_mspfx_list,
							    prefix,
							    prefixlen)) {
			/* found. */
			return (hs);
		}
	}

	/* not found. */
	return (NULL);
}

struct hif_subnet *
hif_subnet_list_find_withhaaddr(hs_list, haaddr)
     struct hif_subnet_list *hs_list;
     struct in6_addr *haaddr;
{
	struct hif_subnet *hs;
	struct mip6_subnet *ms;

	if ((hs_list == NULL) || (haaddr == NULL)) {
		return (NULL);
	}

	TAILQ_FOREACH(hs, hs_list, hs_entry) {
		ms = hs->hs_ms;
		if (ms == NULL) {
			/* must not happen. */
			mip6log((LOG_ERR,
				 "%s: hs_ms is a NULL pointer.\n",
				 __FUNCTION__));
			return (NULL);
		}
		if (mip6_subnet_ha_list_find_withhaaddr(&ms->ms_msha_list,
							haaddr)) {
			/* found. */
			return (hs);
		}
	}

	/* not found. */
	return (NULL);
}

#ifdef HIFHR
struct hif_hr *
hif_hr_create(hha_list)
     struct hif_ha_list *hha_list;
{
	struct hif_hr *hhr;

	hhr = malloc(sizeof(struct hif_hr), M_TEMP, M_NOWAIT);
	if (hhr == NULL) {
		mip6log((LOG_ERR, "%s: memory allocation failed\n",
			 __FUNCTION__));
	}
	if (hhr) {
		bzero(hhr, sizeof(struct hif_hr));
		hhr->hhr_state = HIF_HR_STATE_NOTREG;
		hhr->hhr_hha_list = hha_list;
	}

	return (hhr);
}

int
hif_hr_list_insert(hhr_list, hhr)
     struct hif_hr_list *hhr_list;
     struct hif_hr *hhr;
{
	LIST_INSERT_HEAD(hhr_list, hhr, hhr_entry);

	return (0);
}

int
hif_hr_list_remove(hhr_list, hhr)
     struct hif_hr_list *hhr_list;
     struct hif_hr *hhr;
{
	if (hhr_ep) {
		encap_detach(hhr_ep);
		hhr_ep = NULL;
	}

	LIST_REMOVE(hhr, hhr_entry);
	free(hhr, M_TEMP);

	return (0);
}     

#endif /* HIFHR */

struct hif_coa *
hif_coa_create(ifp)
     struct ifnet *ifp;
{
	struct hif_coa *hcoa;

	if (ifp == NULL) {
		mip6log((LOG_ERR, "%s: NULL ifp\n",
			 __FUNCTION__));
		return (NULL);
	}

	hcoa = malloc(sizeof(struct hif_coa), M_TEMP, M_NOWAIT);
	if (hcoa == NULL) {
		mip6log((LOG_ERR, "%s: memory allocation failure\n",
			 __FUNCTION__));
	}

	if (hcoa) {
		hcoa->hcoa_ifp = ifp;
	}

	return (hcoa);
}

struct in6_ifaddr *
hif_coa_get_ifaddr(hcoa)
     struct hif_coa *hcoa;
{
	struct ifaddr *ia;
	struct in6_ifaddr *ia6, *match;

	if (hcoa == NULL)
		return (NULL);
	if (hcoa->hcoa_ifp == NULL)
		return (NULL);

	match = NULL;
	for (ia = hcoa->hcoa_ifp->if_addrhead.tqh_first;
	     ia;
	     ia = ia->ifa_link.tqe_next) {
		if (ia->ifa_addr->sa_family != AF_INET6)
			continue;
		ia6 = (struct in6_ifaddr *)ia;

		if (ia6->ia6_flags &
		    (IN6_IFF_ANYCAST
		     /* XXX | IN6_IFF_TENTATIVE */
		     | IN6_IFF_DUPLICATED | IN6_IFF_DETACHED
		     | IN6_IFF_DEPRECATED))
			continue;
		if (IN6_IS_ADDR_UNSPECIFIED(&ia6->ia_addr.sin6_addr))
			continue;
		if (IN6_IS_ADDR_LOOPBACK(&ia6->ia_addr.sin6_addr))
			continue;
		if (IN6_IS_ADDR_LINKLOCAL(&ia6->ia_addr.sin6_addr))
			continue;

		/* found */
		match = ia6;
		break;
	}

	return (match);
}

int
hif_coa_list_insert(hcoa_list, hcoa)
     struct hif_coa_list *hcoa_list;
     struct hif_coa *hcoa;
{
	struct hif_coa *tmp, *tmp_next;
	int found;

	if (hcoa == NULL)
		return (-1);

	found = 0;
	for (tmp = TAILQ_FIRST(hcoa_list);
	     tmp;
	     tmp = tmp_next) {
		tmp_next = TAILQ_NEXT(tmp, hcoa_entry);

		if (tmp->hcoa_ifp == hcoa->hcoa_ifp) {
			TAILQ_REMOVE(hcoa_list, tmp, hcoa_entry);
			TAILQ_INSERT_HEAD(hcoa_list, tmp, hcoa_entry);
			found = 1;
		}
	}
	if (found == 0) {
		TAILQ_INSERT_HEAD(hcoa_list, hcoa, hcoa_entry);
	}

	return (0);
}

struct hif_coa *
hif_coa_list_find_withifp(hcoa_list, ifp)
     struct hif_coa_list *hcoa_list;
     struct ifnet *ifp;
{
	struct hif_coa *hcoa;

	if (ifp == NULL)
		return (NULL);

	TAILQ_FOREACH(hcoa, hcoa_list, hcoa_entry) {
		if (hcoa && (hcoa->hcoa_ifp == ifp))
			break;
	}

	return (hcoa);
}

struct hif_ha *
hif_ha_create(onhomelink, mha)
     u_int8_t onhomelink;
     struct mip6_ha *mha;
{
	struct hif_ha *hha;

	hha = malloc(sizeof(struct hif_ha), M_TEMP, M_NOWAIT);
	if (hha) {
		hha->hha_onhomelink = onhomelink;
		hha->hha_mha = mha;
	}

	return (hha);
}

int
hif_ha_list_insert(hha_list, hha)
     struct hif_ha_list *hha_list;
     struct hif_ha *hha;
{
	if (hha == NULL)
		return (-1);

	LIST_INSERT_HEAD(hha_list, hha, hha_entry);

	return (0);
}

int
hif_ha_list_remove_withmha(hha_list, mha)
     struct hif_ha_list *hha_list;
     struct mip6_ha *mha;
{
	struct hif_ha *hha;

	LIST_FOREACH(hha, hha_list, hha_entry) {
		if (hha->hha_mha == mha)
			break;
	}
	if (hha) {
		LIST_REMOVE(hha, hha_entry);
		free(hha, M_TEMP);
	}
	return (0);
}

int
hif_ha_list_isonhomelink(hha_list, rtaddr)
     struct hif_ha_list *hha_list;
     struct in6_addr *rtaddr;
{
	struct hif_ha *hha;
	int onhomelink = 0;

	LIST_FOREACH(hha, hha_list, hha_entry) {
		if ((hha->hha_onhomelink)
		    && (IN6_ARE_ADDR_EQUAL(&hha->hha_mha->mha_lladdr,
					   rtaddr)
			|| IN6_ARE_ADDR_EQUAL(&hha->hha_mha->mha_gaddr,
					      rtaddr))) {
			onhomelink = 1;
			break;
		}
	}

	return (onhomelink);
}

struct hif_ha *
hif_ha_list_find_onhomelink(hha_list)
     struct hif_ha_list *hha_list;
{
	struct hif_ha *hha;

	LIST_FOREACH(hha, hha_list, hha_entry) {
		if (hha->hha_onhomelink)
			break;
	}

	return (hha);
}

struct hif_ha *
hif_ha_list_find_preferable(hha_list)
     struct hif_ha_list *hha_list;
{
	struct hif_ha *hha, *best;

	best = LIST_FIRST(hha_list);
	LIST_FOREACH(hha, hha_list, hha_entry) {
		if ((best) && (best->hha_mha)
		    && (hha) && (hha->hha_mha)) {
			if (best->hha_mha->mha_pref < hha->hha_mha->mha_pref)
				best = hha;
		}
	}

	return (best);
}

struct hif_ha *
hif_ha_list_find_withaddr(hha_list, rtaddr)
     struct hif_ha_list *hha_list;
     struct in6_addr *rtaddr;
{
	struct hif_ha *hha;

	LIST_FOREACH(hha, hha_list, hha_entry) {
		if (IN6_ARE_ADDR_EQUAL(&hha->hha_mha->mha_lladdr, rtaddr)
		    || IN6_ARE_ADDR_EQUAL(&hha->hha_mha->mha_gaddr, rtaddr))
			break;
	}

	return (hha);
}

static int
hif_ha_list_update_withioctl(sc, data)
     struct hif_softc *sc;
     caddr_t data;
{
	struct hif_ifreq *ifr = (struct hif_ifreq *)data;
	struct hif_subnet *hs;
	struct mip6_subnet *ms;
	struct mip6_subnet_ha *msha;
	struct mip6_ha *nmha = (struct mip6_ha *)data;
	struct mip6_ha *mha;
	int error = 0;

	if (ifr == NULL) {
		return (EINVAL);
	}
	if ((nmha = ifr->ifr_ifru.ifr_mha) == NULL) {
		return (EINVAL);
	}

	hs = hif_subnet_list_find_withhaaddr(&sc->hif_hs_list_home,
					     &nmha->mha_lladdr);
	if (hs == NULL) {
		/* find mip6_subnet that includes this ha's prefix. */
		hs = hif_subnet_list_find_withprefix(&sc->hif_hs_list_home,
						     &nmha->mha_gaddr,
						     64); /* XXX */
		if (hs == NULL) {
			/*
			 * there is no mip6_subnet that has the same
			 * prefix with this updating homeagent.
			 */
			mip6log((LOG_ERR,
				 "%s: no hif_subnet.  you must specify "
				 "at least one prefix before setting "
				 "a home agent manually.\n",
				 __FUNCTION__));
			return (EINVAL);
		}
	}
	if ((ms = hs->hs_ms) == NULL) {
		/* must not happen. */
		return (EINVAL);
	}
	
	msha = mip6_subnet_ha_list_find_withhaaddr(&ms->ms_msha_list,
						   &nmha->mha_lladdr);
	if (msha == NULL) {
		mha = mip6_ha_create(&nmha->mha_lladdr,
				     &nmha->mha_gaddr,
				     nmha->mha_flags,
				     nmha->mha_pref,
				     nmha->mha_lifetime);
		if (mha == NULL) {
			mip6log((LOG_ERR,
				 "%s: mip6_ha memory allocation failed.\n",
				 __FUNCTION__));
			return (ENOMEM);
		}
		msha = mip6_subnet_ha_create(mha);
		if (msha == NULL) {
			mip6log((LOG_ERR,
				 "%s: mip6_subnet_ha memory allocation "
				 "failed.\n",
				 __FUNCTION__));
			return (ENOMEM);
		}
		error = mip6_subnet_ha_list_insert(&ms->ms_msha_list, msha);
		if (error) {
			return (error);
		}
	} else {
		/* there is mip6_subnet_ha. */
		mha = msha->msha_mha;
		if (mha == NULL) {
			/* must not happen. */
			return (EINVAL);
		}
	}

	mha->mha_lladdr = nmha->mha_lladdr;
	mha->mha_gaddr = nmha->mha_gaddr;
	mha->mha_flags = nmha->mha_flags;
	mha->mha_pref = nmha->mha_pref;
	mha->mha_lifetime = nmha->mha_lifetime;
	mha->mha_remain = mha->mha_lifetime;

	return (0);
}

struct hif_softc *
hif_list_find_withhaddr(haddr)
     struct in6_addr *haddr;
{
	struct hif_softc *sc;
	struct hif_subnet *hs;
	struct mip6_subnet *ms;
	struct mip6_subnet_prefix *mspfx;
	struct mip6_prefix *mpfx;

	TAILQ_FOREACH(sc, &hif_softc_list, hif_entry) {
		TAILQ_FOREACH(hs, &sc->hif_hs_list_home, hs_entry) {
			if ((ms = hs->hs_ms) == NULL)
				continue;
			TAILQ_FOREACH(mspfx, &ms->ms_mspfx_list, mspfx_entry) {
				if((mpfx = mspfx->mspfx_mpfx) == NULL)
					continue;
				if (IN6_ARE_ADDR_EQUAL(haddr,
						       &mpfx->mpfx_haddr)) {
					/* found. */
					return (sc);
				}
			}
		}
	}

	/* not found. */
	return (NULL);
}

static int
hif_subnet_list_update_withmpfx(sc, data)
     struct hif_softc *sc;
     caddr_t data;
{
	struct hif_softc *othersc;
	struct hif_ifreq *ifr = (struct hif_ifreq *)data;
	struct hif_subnet *hs;
	struct mip6_subnet *ms;
	struct mip6_prefix *nmpfx;
	struct mip6_prefix *mpfx;
	struct mip6_subnet_prefix *mspfx;
	int error = 0;

	if (ifr == NULL) {
		return (EINVAL);
	}
	if ((nmpfx = ifr->ifr_ifru.ifr_mpfx) == NULL) {
		return (EINVAL);
	}

	hs = hif_subnet_list_find_withprefix(&sc->hif_hs_list_home,
					     &nmpfx->mpfx_prefix,
					     nmpfx->mpfx_prefixlen);
	if (hs == NULL) {
		/* find mip6_subnet that includes this prefix. */
		ms = mip6_subnet_list_find_withprefix(&mip6_subnet_list,
						      &nmpfx->mpfx_prefix,
						      nmpfx->mpfx_prefixlen);
		if (ms == NULL) {
			ms = mip6_subnet_create();
			if (ms == NULL) {
				mip6log((LOG_ERR,
					 "%s: mip6_subnet memory allocation "
					 "failed.\n",
					 __FUNCTION__));
				return (ENOMEM);
			}
			error = mip6_subnet_list_insert(&mip6_subnet_list,
							ms);
			if (error) {
				return (error);
			}

			mpfx = mip6_prefix_create(&nmpfx->mpfx_prefix,
						  nmpfx->mpfx_prefixlen,
						  nmpfx->mpfx_lifetime);
			if (mpfx == NULL) {
				mip6log((LOG_ERR,
					 "%s: mip6_prefix memory allocation "
					 "failed.\n",
					 __FUNCTION__));
				return (ENOMEM);
			}
			error = mip6_prefix_list_insert(&mip6_prefix_list,
							mpfx);
			if (error) {
				return(error);
			}

			mspfx = mip6_subnet_prefix_create(mpfx);
			if (mspfx == NULL) {
				mip6log((LOG_ERR,
					 "%s: mip6_subnet_prefix memory "
					 "allocation failed.\n",
					 __FUNCTION__));
				return (ENOMEM);
			}
			error = mip6_subnet_prefix_list_insert(&ms->ms_mspfx_list,
							       mspfx);
			if (error) {
				return (error);
			}

			/*
			 * add this newly created mip6_subnet into the
			 * other hif interface's foreign subnet list.
			 */
			TAILQ_FOREACH(othersc, &hif_softc_list, hif_entry) {
				if (othersc == sc)
					continue;
				hs = hif_subnet_create(ms);
				if (hs == NULL) {
					return (ENOMEM);
				}
				error = hif_subnet_list_insert(&othersc->hif_hs_list_foreign,
							       hs);
				if (error)
					return (error);
			}
		} else {
			/*
			 * there is a mip6_subnet which contains specified
			 * prefix.
			 */
			mspfx = mip6_subnet_prefix_list_find_withprefix
				(&ms->ms_mspfx_list,
				 &nmpfx->mpfx_prefix,
				 nmpfx->mpfx_prefixlen);
			if (mspfx == NULL) {
				/* this must not happen. */
				return (EINVAL);
			}
			mpfx = mspfx->mspfx_mpfx;
			if (mpfx == NULL) {
				return (EINVAL);
			}
			mpfx->mpfx_lifetime = nmpfx->mpfx_lifetime;
			mpfx->mpfx_remain = mpfx->mpfx_lifetime;
		}
		hs = hif_subnet_create(ms);
		if (hs == NULL) {
			mip6log((LOG_ERR,
				 "%s: hif_subnet memory allocation failed.\n",
				 __FUNCTION__));
			return (ENOMEM);
		}
		error = hif_subnet_list_insert(&sc->hif_hs_list_home,
					       hs);
		if (error) {
			return (error);
		}
	} else {
		/* there is a hif_subnet that contains specified prefix. */
		ms = hs->hs_ms;
		if (ms == NULL) {
			/* must not happen. */
			return (EINVAL);
		}
		mspfx = mip6_subnet_prefix_list_find_withprefix
			(&ms->ms_mspfx_list,
			 &nmpfx->mpfx_prefix,
			 nmpfx->mpfx_prefixlen);
		if (mspfx == NULL) {
			return (EINVAL);
		}
		mpfx = mspfx->mspfx_mpfx;
		if (mpfx == NULL) {
			return (EINVAL);
		}
		mpfx->mpfx_lifetime = nmpfx->mpfx_lifetime;
		mpfx->mpfx_remain = mpfx->mpfx_lifetime;
	}
	return (0);
}

static void
hif_rtrequest(cmd, rt, sa)
	int cmd;
	struct rtentry *rt;
	struct sockaddr *sa;
{
	/* Copyed from lortrequest. */
	if (rt) {
		rt->rt_rmx.rmx_mtu = rt->rt_ifp->if_mtu; /* for ISO */
		/*
		 * For optimal performance, the send and receive buffers
		 * should be at least twice the MTU plus a little more for
		 * overhead.
		 */
		rt->rt_rmx.rmx_recvpipe =
			rt->rt_rmx.rmx_sendpipe = 3 * HIF_MTU;
	}
}

int
hif_output(ifp, m, dst, rt)
     struct ifnet *ifp;
     struct mbuf *m;
     struct sockaddr *dst;
     struct rtentry *rt;
{
	/* This function is copyed from looutput */

	if ((m->m_flags & M_PKTHDR) == 0)
		panic("hif_output no HDR");

	if (rt && rt->rt_flags & (RTF_REJECT|RTF_BLACKHOLE)) {
		m_freem(m);
		return (rt->rt_flags & RTF_BLACKHOLE ? 0 :
		        rt->rt_flags & RTF_HOST ? EHOSTUNREACH : ENETUNREACH);
	}
	/*
	 * KAME requires that the packet to be contiguous on the
	 * mbuf.  We need to make that sure.
	 * this kind of code should be avoided.
	 * XXX: fails to join if interface MTU > MCLBYTES.  jumbogram?
	 */
	if (m && m->m_next != NULL && m->m_pkthdr.len < MCLBYTES) {
		struct mbuf *n;

		MGETHDR(n, M_DONTWAIT, MT_HEADER);
		if (!n)
			goto contiguousfail;
		MCLGET(n, M_DONTWAIT);
		if (! (n->m_flags & M_EXT)) {
			m_freem(n);
			goto contiguousfail;
		}

		m_copydata(m, 0, m->m_pkthdr.len, mtod(n, caddr_t));
		n->m_pkthdr = m->m_pkthdr;
		n->m_len = m->m_pkthdr.len;
		n->m_pkthdr.aux = m->m_pkthdr.aux;
		m->m_pkthdr.aux = (struct mbuf *)NULL;
		m_freem(m);
		m = n;
	}
	if (0) {
contiguousfail:
		printf("hif_output: mbuf allocation failed\n");
	}
	ifp->if_opackets++;
	ifp->if_obytes += m->m_pkthdr.len;
#if 1
	switch (dst->sa_family) {
	case AF_INET6:
		break;
	default:
		printf("hif_output: af=%d unexpected\n", dst->sa_family);
		m_freem(m);
		return (EAFNOSUPPORT);
	}
#endif
	/* XXX encapsulate to our home link ? */
	m_freem(m);
	return(0);

}

#endif /* NHIF > 0 */
