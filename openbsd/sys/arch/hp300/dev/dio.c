/*	$OpenBSD: dio.c,v 1.6 1997/10/06 06:44:49 millert Exp $	*/
/*	$NetBSD: dio.c,v 1.7 1997/05/05 21:00:32 thorpej Exp $	*/

/*-
 * Copyright (c) 1996, 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Autoconfiguration and mapping support for the DIO bus.
 */

#define	_HP300_INTR_H_PRIVATE

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/autoconf.h>
#include <machine/cpu.h>
#include <machine/hp300spu.h>

#include <hp300/dev/dmavar.h>

#include <hp300/dev/dioreg.h>
#include <hp300/dev/diovar.h>

#include <hp300/dev/diodevs.h>
#include <hp300/dev/diodevs_data.h>

extern	caddr_t internalhpib;

int	dio_scodesize __P((struct dio_attach_args *));
char	*dio_devinfo __P((struct dio_attach_args *, char *, size_t));

int	diomatch __P((struct device *, void *, void *));
void	dioattach __P((struct device *, struct device *, void *));
int	dioprint __P((void *, const char *));
int	diosubmatch __P((struct device *, void *, void *));

struct cfattach dio_ca = {
	sizeof(struct device), diomatch, dioattach
};

struct cfdriver dio_cd = {
	NULL, "dio", DV_DULL
};

int
diomatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	static int dio_matched = 0;

	/* Allow only one instance. */
	if (dio_matched)
		return (0);

	dio_matched = 1;
	return (1);
}

void
dioattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct dio_attach_args da;
	caddr_t pa, va;
	int scode, scmax, didmap, scodesize;

	scmax = DIO_SCMAX(machineid);
	printf(": ");
	dmainit();

	for (scode = 0; scode < scmax; ) {
		if (DIO_INHOLE(scode)) {
			scode++;
			continue;
		}

		didmap = 0;

		/*
		 * Temporarily map the space corresponding to
		 * the current select code unless:
		 *	- this is the internal hpib select code,
		 *	- this is the console select code.
		 */
		pa = dio_scodetopa(scode);
		if (scode == conscode)
			va = conaddr;
		else if ((scode == 7) && internalhpib)
			va = internalhpib = (caddr_t)IIOV(pa);
		else {
			va = iomap(pa, NBPG);
			if (va == NULL) {
				printf("%s: can't map scode %d\n",
				    self->dv_xname, scode);
				scode++;
				continue;
			}
			didmap = 1;
		}

		/* Check for hardware. */
		if (badaddr(va)) {
			if (didmap)
				iounmap(va, NBPG);
			scode++;
			continue;
		}

		/* Fill out attach args. */
		bzero(&da, sizeof(da));
		da.da_scode = scode;
		if (scode == 7 && internalhpib)
			da.da_id = DIO_DEVICE_ID_IHPIB;
		else
			da.da_id = DIO_ID(va);

		if (DIO_ISFRAMEBUFFER(da.da_id))
			da.da_secid = DIO_SECID(va);

		da.da_size = DIO_SIZE(scode, va);
		scodesize = dio_scodesize(&da);
		if (DIO_ISDIO(scode))
			da.da_size *= scodesize;

		/* No longer need the device to be mapped. */
		if (didmap)
			iounmap(va, NBPG);

		/* Attach matching device. */
		config_found_sm(self, &da, dioprint, diosubmatch);
		scode += scodesize;
	}
}

int
diosubmatch(parent, match, aux)
	struct device *parent;
	void *match, *aux;
{
	struct cfdata *cf = match;
	struct dio_attach_args *da = aux;

	if (cf->diocf_scode != DIO_UNKNOWN_SCODE &&
	    cf->diocf_scode != da->da_scode)
		return (0);

	return ((*cf->cf_attach->ca_match)(parent, cf, aux));
}

int
dioprint(aux, pnp)
	void *aux;
	const char *pnp;
{
	struct dio_attach_args *da = aux;
	char buf[128];

	if (pnp)
		printf("%s at %s", dio_devinfo(da, buf, sizeof(buf)), pnp);
	printf(" scode %d", da->da_scode);
	return (UNCONF);
}

/*
 * Convert a select code to a system physical address.
 */
void *
dio_scodetopa(scode)
	int scode;
{
	u_long rval;

	if (scode == 7 && internalhpib)
		rval = DIO_IHPIBADDR;
	else if (DIO_ISDIO(scode))
		rval = DIO_BASE + (scode * DIO_DEVSIZE);
	else if (DIO_ISDIOII(scode))
		rval = DIOII_BASE + ((scode - DIOII_SCBASE) * DIOII_DEVSIZE);
	else
		rval = 0;

	return ((void *)rval);
}

/*
 * Return the select code size for this device, defaulting to 1
 * if we don't know what kind of device we have.
 */
int
dio_scodesize(da)
	struct dio_attach_args *da;
{
	int i;

	/*
	 * Deal with lame internal HP-IB controllers which don't have
	 * consistent/reliable device ids.
	 */
	if (da->da_scode == 7 && internalhpib)
		return (1);

	/*
	 * Find the dio_devdata matchind the primary id.
	 * If we're a framebuffer, we also check the secondary id.
	 */
	for (i = 0; i < DIO_NDEVICES; i++) {
		if (da->da_id == dio_devdatas[i].dd_id) {
			if (DIO_ISFRAMEBUFFER(da->da_id)) {
				if (da->da_secid == dio_devdatas[i].dd_secid) {
					goto foundit;
				}
			} else {
			foundit:
				return (dio_devdatas[i].dd_nscode);
			}
		}
	}

	/*
	 * Device is unknown.  Print a warning and assume a default.
	 */
	printf("WARNING: select code size unknown for id = 0x%x secid = 0x%x\n",
	    da->da_id, da->da_secid);
	return (1);
}

/*
 * Return a reasonable description of a DIO device.
 */
char *
dio_devinfo(da, buf, buflen)
	struct dio_attach_args *da;
	char *buf;
	size_t buflen;
{
#ifdef DIOVERBOSE
	int i;
#endif

	bzero(buf, buflen);

	/*
	 * Deal with lame internal HP-IB controllers which don't have
	 * consistent/reliable device ids.
	 */
	if (da->da_scode == 7 && internalhpib) {
		sprintf(buf, DIO_DEVICE_DESC_IHPIB);
		return (buf);
	}

#ifdef DIOVERBOSE
	/*
	 * Find the description matching our primary id.
	 * If we're a framebuffer, we also check the secondary id.
	 */
	for (i = 0; i < DIO_NDEVICES; i++) {
		if (da->da_id == dio_devdescs[i].dd_id) {
			if (DIO_ISFRAMEBUFFER(da->da_id)) {
				if (da->da_secid == dio_devdescs[i].dd_secid) {
					goto foundit;
				}
			} else {
			foundit:
				sprintf(buf, "%s", dio_devdescs[i].dd_desc);
				return (buf);
			}
		}
	}
#endif /* DIOVERBOSE */

	/*
	 * Device is unknown.  Construct something reasonable.
	 */
	sprintf(buf, "device id = 0x%x secid = 0x%x",
	    da->da_id, da->da_secid);
	return (buf);
}

/*
 * Establish an interrupt handler for a DIO device.
 */
void *
dio_intr_establish(func, arg, ipl, priority)
	int (*func) __P((void *));
	void *arg;
	int ipl;
	int priority;
{
	void *ih;

	ih = intr_establish(func, arg, ipl, priority);

	if (priority == IPL_BIO)
		dmacomputeipl();

	return (ih);
}

/*
 * Remove an interrupt handler for a DIO device.
 */
void
dio_intr_disestablish(arg)
	void *arg;
{
	struct isr *isr = arg;
	int priority = isr->isr_priority;

	intr_disestablish(arg);

	if (priority == IPL_BIO)
		dmacomputeipl();
}
