/*	$OpenBSD: uba.c,v 1.3 2001/09/11 20:05:25 miod Exp $	*/
/*	$NetBSD: uba.c,v 1.52 2000/06/04 02:14:12 matt Exp $	   */
/*
 * Copyright (c) 1996 Jonathan Stone.
 * Copyright (c) 1994, 1996 Ludd, University of Lule}, Sweden.
 * Copyright (c) 1982, 1986 The Regents of the University of California.
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
 *	@(#)uba.c	7.10 (Berkeley) 12/16/90
 *	@(#)autoconf.c	7.20 (Berkeley) 5/9/91
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/buf.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/dkstat.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/device.h>

#include <vm/vm.h>

#include <machine/bus.h>
#include <machine/scb.h>
#include <machine/cpu.h>

#include <arch/vax/qbus/ubavar.h>

static int ubasearch (struct device *, struct cfdata *, void *);
static int ubaprint (void *, const char *);

struct 	cfdriver uba_cd = {
	NULL, "uba", DV_DULL
};

/*
 * If we failed to allocate uba resources, put us on a queue to wait
 * until there is available resources. Resources to compete about
 * are map registers and BDPs. This is normally only a problem on
 * Unibus systems, Qbus systems have more map registers than usable.
 */
void
uba_enqueue(struct uba_unit *uu)
{
	struct uba_softc *uh;
	int s;

	uh = (void *)((struct device *)(uu->uu_softc))->dv_parent;

	s = splimp();
	SIMPLEQ_INSERT_TAIL(&uh->uh_resq, uu, uu_resq);
	splx(s);
}

/*
 * When a routine that uses resources is finished, the next device
 * in queue for map registers etc is called. If it succeeds to get
 * resources, call next, and next, and next...
 * This routine must be called at splimp.
 */
void
uba_done(struct uba_softc *uh)
{
	struct uba_unit *uu;
 
	while ((uu = SIMPLEQ_FIRST(&uh->uh_resq))) {
		SIMPLEQ_REMOVE_HEAD(&uh->uh_resq, uu, uu_resq);
		if ((*uu->uu_ready)(uu) == 0) {
			SIMPLEQ_INSERT_HEAD(&uh->uh_resq, uu, uu_resq);
			break;
		}
	}
}

/*
 * Each device that needs some handling if an ubareset occurs must
 * register for reset first through this routine.
 */
void
uba_reset_establish(void (*reset)(struct device *), struct device *dev)
{
	struct uba_softc *uh = (void *)dev->dv_parent;
	struct uba_reset *ur;

	ur = malloc(sizeof(struct uba_reset), M_DEVBUF, M_NOWAIT);
	ur->ur_dev = dev;
	ur->ur_reset = reset;

	SIMPLEQ_INSERT_TAIL(&uh->uh_resetq, ur, ur_resetq);
}

/*
 * Generate a reset on uba number uban.	 Then
 * call each device that asked to be called during attach,
 * giving it a chance to clean up so as to be able to continue.
 */
void
ubareset(struct uba_softc *uh)
{
	struct uba_reset *ur;
	int s;

	s = splimp();
	SIMPLEQ_INIT(&uh->uh_resq);
	printf("%s: reset", uh->uh_dev.dv_xname);
	(*uh->uh_ubainit)(uh);

	ur = SIMPLEQ_FIRST(&uh->uh_resetq);
	if (ur) do {
		printf(" %s", ur->ur_dev->dv_xname);
		(*ur->ur_reset)(ur->ur_dev);
	} while ((ur = SIMPLEQ_NEXT(ur, ur_resetq)));

	printf("\n");
	splx(s);
}

/*
 * The common attach routine:
 *   Calls the scan routine to search for uba devices.
 */
void
uba_attach(struct uba_softc *sc, paddr_t iopagephys)
{

	/*
	 * Set last free interrupt vector for devices with
	 * programmable interrupt vectors.  Use is to decrement
	 * this number and use result as interrupt vector.
	 */
	sc->uh_lastiv = 0x200;
	SIMPLEQ_INIT(&sc->uh_resq);
	SIMPLEQ_INIT(&sc->uh_resetq);

	/*
	 * Allocate place for unibus I/O space in virtual space.
	 */
	if (bus_space_map(sc->uh_iot, iopagephys, UBAIOSIZE, 0, &sc->uh_ioh))
		return;

	if (sc->uh_beforescan)
		(*sc->uh_beforescan)(sc);
	/*
	 * Now start searching for devices.
	 */
	config_search((cfmatch_t)ubasearch,(struct device *)sc, NULL);

	if (sc->uh_afterscan)
		(*sc->uh_afterscan)(sc);
}

int
ubasearch(struct device *parent, struct cfdata *cf, void *aux)
{
	struct	uba_softc *sc = (struct uba_softc *)parent;
	struct	uba_attach_args ua;
	int	i, vec, br;

	ua.ua_ioh = ubdevreg(cf->cf_loc[0]) + sc->uh_ioh;
	ua.ua_iot = sc->uh_iot;
	ua.ua_dmat = sc->uh_dmat;

	if (badaddr((caddr_t)ua.ua_ioh, 2) ||
	    (sc->uh_errchk ? (*sc->uh_errchk)(sc):0))
		goto forgetit;

	scb_vecref(0, 0); /* Clear vector ref */
	i = (*cf->cf_attach->ca_match) (parent, cf, &ua);

	if (sc->uh_errchk)
		if ((*sc->uh_errchk)(sc))
			goto forgetit;
	if (i == 0)
		goto forgetit;

	i = scb_vecref(&vec, &br);
	if (i == 0)
		goto fail;
	if (vec == 0)
		goto fail;

	ua.ua_br = br;
	ua.ua_cvec = vec;
	ua.ua_iaddr = cf->cf_loc[0];

	config_attach(parent, cf, &ua, ubaprint);
	return 0;

fail:
	printf("%s%d at %s csr %o %s\n",
	    cf->cf_driver->cd_name, cf->cf_unit, parent->dv_xname,
	    cf->cf_loc[0], (i ? "zero vector" : "didn't interrupt"));

forgetit:
	return 0;
}

/*
 * Print out some interesting info common to all unibus devices.
 */
int
ubaprint(void *aux, const char *uba)
{
	struct uba_attach_args *ua = aux;

	printf(" csr %o vec %o ipl %x", ua->ua_iaddr,
	    ua->ua_cvec & 511, ua->ua_br);
	return UNCONF;
}

/*
 * Move to machdep eventually
 */
void
uba_intr_establish(icookie, vec, ifunc, iarg, ev)
	void *icookie;
	int vec;
	void (*ifunc)(void *iarg);
	void *iarg;
	struct evcnt *ev;
{
	scb_vecalloc(vec, ifunc, iarg, SCB_ISTACK, ev);
}
