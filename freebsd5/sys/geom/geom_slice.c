/*-
 * Copyright (c) 2002 Poul-Henning Kamp
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Poul-Henning Kamp
 * and NAI Labs, the Security Research Division of Network Associates, Inc.
 * under DARPA/SPAWAR contract N66001-01-C-8035 ("CBOSS"), as part of the
 * DARPA CHATS research program.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: src/sys/geom/geom_slice.c,v 1.28.2.2 2002/12/22 18:00:18 phk Exp $
 */


#include <sys/param.h>
#include <sys/stdint.h>
#ifndef _KERNEL
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <err.h>
#else
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/bio.h>
#include <sys/sysctl.h>
#include <sys/proc.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#endif
#include <sys/errno.h>
#include <sys/sbuf.h>
#include <geom/geom.h>
#include <geom/geom_slice.h>
#include <machine/stdarg.h>

static g_orphan_t g_slice_orphan;
static g_access_t g_slice_access;
static g_start_t g_slice_start;

static struct g_slicer *
g_slice_init(unsigned nslice, unsigned scsize)
{
	struct g_slicer *gsp;

	gsp = g_malloc(sizeof *gsp, M_WAITOK | M_ZERO);
	gsp->softc = g_malloc(scsize, M_WAITOK | M_ZERO);
	gsp->slices = g_malloc(nslice * sizeof(struct g_slice),
	    M_WAITOK | M_ZERO);
	gsp->nslice = nslice;
	return (gsp);
}

static int
g_slice_access(struct g_provider *pp, int dr, int dw, int de)
{
	int error;
	u_int u;
	struct g_geom *gp;
	struct g_consumer *cp;
	struct g_provider *pp2;
	struct g_slicer *gsp;
	struct g_slice *gsl, *gsl2;

	gp = pp->geom;
	cp = LIST_FIRST(&gp->consumer);
	KASSERT (cp != NULL, ("g_slice_access but no consumer"));
	gsp = gp->softc;
	gsl = &gsp->slices[pp->index];
	for (u = 0; u < gsp->nslice; u++) {
		gsl2 = &gsp->slices[u];
		if (gsl2->length == 0)
			continue;
		if (u == pp->index)
			continue;
		if (gsl->offset + gsl->length <= gsl2->offset)
			continue;
		if (gsl2->offset + gsl2->length <= gsl->offset)
			continue;
		/* overlap */
		pp2 = gsl2->provider;
		if ((pp->acw + dw) > 0 && pp2->ace > 0)
			return (EPERM);
		if ((pp->ace + de) > 0 && pp2->acw > 0)
			return (EPERM);
	}
	/* On first open, grab an extra "exclusive" bit */
	if (cp->acr == 0 && cp->acw == 0 && cp->ace == 0)
		de++;
	/* ... and let go of it on last close */
	if ((cp->acr + dr) == 0 && (cp->acw + dw) == 0 && (cp->ace + de) == 1)
		de--;
	error = g_access_rel(cp, dr, dw, de);
	return (error);
}

void
g_slice_finish_hot(struct bio *bp)
{
	struct bio *bp2;
	struct g_geom *gp;
	struct g_consumer *cp;
	struct g_slicer *gsp;
	struct g_slice *gsl;
	int idx;

	KASSERT(bp->bio_to != NULL, ("NULL bio_to in g_slice_finish_hot(%p)", bp));
	KASSERT(bp->bio_from != NULL, ("NULL bio_from in g_slice_finish_hot(%p)", bp));
	gp = bp->bio_to->geom;
	gsp = gp->softc;
	cp = LIST_FIRST(&gp->consumer);
	KASSERT(cp != NULL, ("NULL consumer in g_slice_finish_hot(%p)", bp));
	idx = bp->bio_to->index;
	gsl = &gsp->slices[idx];

	bp2 = g_clone_bio(bp);
	if (bp2 == NULL) {
		g_io_deliver(bp, ENOMEM);
		return;
	}
	if (bp2->bio_offset + bp2->bio_length > gsl->length)
		bp2->bio_length = gsl->length - bp2->bio_offset;
	bp2->bio_done = g_std_done;
	bp2->bio_offset += gsl->offset;
	g_io_request(bp2, cp);
	return;
}

static void
g_slice_start(struct bio *bp)
{
	struct bio *bp2;
	struct g_provider *pp;
	struct g_geom *gp;
	struct g_consumer *cp;
	struct g_slicer *gsp;
	struct g_slice *gsl, *gmp;
	int idx, error;
	u_int m_index;
	off_t t;

	pp = bp->bio_to;
	gp = pp->geom;
	gsp = gp->softc;
	cp = LIST_FIRST(&gp->consumer);
	idx = pp->index;
	gsl = &gsp->slices[idx];
	switch(bp->bio_cmd) {
	case BIO_READ:
	case BIO_WRITE:
	case BIO_DELETE:
		if (bp->bio_offset > gsl->length) {
			g_io_deliver(bp, EINVAL); /* XXX: EWHAT ? */
			return;
		}
		/*
		 * Check if we collide with any hot spaces, and call the
		 * method once if so.
		 */
		t = bp->bio_offset + gsl->offset;
		/* .ctl devices may take us negative */
		if (t < 0 || (t + bp->bio_length) < 0) {
			g_io_deliver(bp, EINVAL);
			return;
		}
		for (m_index = 0; m_index < gsp->nhot; m_index++) {
			gmp = &gsp->hot[m_index];
			if (t >= gmp->offset + gmp->length)
				continue;
			if (t + bp->bio_length <= gmp->offset)
				continue;
			error = gsp->start(bp);
			if (error == EJUSTRETURN)
				return;
			else if (error) {
				g_io_deliver(bp, error);
				return;
			}
			break;
		}
		bp2 = g_clone_bio(bp);
		if (bp2 == NULL) {
			g_io_deliver(bp, ENOMEM);
			return;
		}
		if (bp2->bio_offset + bp2->bio_length > gsl->length)
			bp2->bio_length = gsl->length - bp2->bio_offset;
		bp2->bio_done = g_std_done;
		bp2->bio_offset += gsl->offset;
		g_io_request(bp2, cp);
		return;
	case BIO_GETATTR:
	case BIO_SETATTR:
		/* Give the real method a chance to override */
		if (gsp->start(bp))
			return;
		if (!strcmp("GEOM::frontstuff", bp->bio_attribute)) {
			t = gsp->cfrontstuff;
			if (gsp->frontstuff > t)
				t = gsp->frontstuff;
			t -= gsl->offset;
			if (t < 0)
				t = 0;
			if (t > gsl->length)
				t = gsl->length;
			g_handleattr_off_t(bp, "GEOM::frontstuff", t);
			return;
		}
#ifdef _KERNEL
		if (!strcmp("GEOM::kerneldump", bp->bio_attribute)) {
			struct g_kerneldump *gkd;

			gkd = (struct g_kerneldump *)bp->bio_data;
			gkd->offset += gsp->slices[idx].offset;
			if (gkd->length > gsp->slices[idx].length)
				gkd->length = gsp->slices[idx].length;
			/* now, pass it on downwards... */
		}
#endif
		bp2 = g_clone_bio(bp);
		if (bp2 == NULL) {
			g_io_deliver(bp, ENOMEM);
			return;
		}
		bp2->bio_done = g_std_done;
		g_io_request(bp2, cp);
		break;
	default:
		g_io_deliver(bp, EOPNOTSUPP);
		return;
	}
}

void
g_slice_dumpconf(struct sbuf *sb, const char *indent, struct g_geom *gp, struct g_consumer *cp, struct g_provider *pp)
{
	struct g_slicer *gsp;

	gsp = gp->softc;
	if (indent == NULL) {
		sbuf_printf(sb, " i %u", pp->index);
		sbuf_printf(sb, " o %ju", 
		    (uintmax_t)gsp->slices[pp->index].offset);
		return;
	}
	if (gp != NULL && (pp == NULL && cp == NULL)) {
		sbuf_printf(sb, "%s<frontstuff>%ju</frontstuff>\n",
		    indent, (intmax_t)gsp->frontstuff);
	}
	if (pp != NULL) {
		sbuf_printf(sb, "%s<index>%u</index>\n", indent, pp->index);
		sbuf_printf(sb, "%s<length>%ju</length>\n",
		    indent, (uintmax_t)gsp->slices[pp->index].length);
		sbuf_printf(sb, "%s<seclength>%ju</seclength>\n", indent,
		    (uintmax_t)gsp->slices[pp->index].length / 512);
		sbuf_printf(sb, "%s<offset>%ju</offset>\n", indent,
		    (uintmax_t)gsp->slices[pp->index].offset);
		sbuf_printf(sb, "%s<secoffset>%ju</secoffset>\n", indent,
		    (uintmax_t)gsp->slices[pp->index].offset / 512);
	}
}

int
g_slice_config(struct g_geom *gp, u_int idx, int how, off_t offset, off_t length, u_int sectorsize, const char *fmt, ...)
{
	struct g_provider *pp;
	struct g_slicer *gsp;
	struct g_slice *gsl;
	va_list ap;
	struct sbuf *sb;
	int error, acc;

	g_trace(G_T_TOPOLOGY, "g_slice_config(%s, %d, %d)",
	     gp->name, idx, how);
	g_topology_assert();
	gsp = gp->softc;
	error = 0;
	if (idx >= gsp->nslice)
		return(EINVAL);
	gsl = &gsp->slices[idx];
	pp = gsl->provider;
	if (pp != NULL)
		acc = pp->acr + pp->acw + pp->ace;
	else
		acc = 0;
	if (acc != 0 && how != G_SLICE_CONFIG_FORCE) {
		if (length < gsl->length)
			return(EBUSY);
		if (offset != gsl->offset)
			return(EBUSY);
	}
	/* XXX: check offset + length <= MEDIASIZE */
	if (how == G_SLICE_CONFIG_CHECK)
		return (0);
	gsl->length = length;
	gsl->offset = offset;
	gsl->sectorsize = sectorsize;
	if (length == 0) {
		if (pp == NULL)
			return (0);
		if (bootverbose)
			printf("GEOM: Deconfigure %s\n", pp->name);
		g_orphan_provider(pp, ENXIO);
		gsl->provider = NULL;
		gsp->nprovider--;
		return (0);
	}
	if (pp != NULL) {
		if (bootverbose)
			printf("GEOM: Reconfigure %s, start %jd length %jd end %jd\n",
			    pp->name, (intmax_t)offset, (intmax_t)length,
			    (intmax_t)(offset + length - 1));
		pp->mediasize = gsl->length;
		return (0);
	}
	va_start(ap, fmt);
	sb = sbuf_new(NULL, NULL, 0, SBUF_AUTOEXTEND);
	sbuf_vprintf(sb, fmt, ap);
	sbuf_finish(sb);
	pp = g_new_providerf(gp, sbuf_data(sb));
	if (bootverbose)
		printf("GEOM: Configure %s, start %jd length %jd end %jd\n",
		    pp->name, (intmax_t)offset, (intmax_t)length,
		    (intmax_t)(offset + length - 1));
	pp->index = idx;
	pp->mediasize = gsl->length;
	pp->sectorsize = gsl->sectorsize;
	gsl->provider = pp;
	gsp->nprovider++;
	g_error_provider(pp, 0);
	sbuf_delete(sb);
	return(0);
}

int
g_slice_conf_hot(struct g_geom *gp, u_int idx, off_t offset, off_t length)
{
	struct g_slicer *gsp;
	struct g_slice *gsl, *gsl2;

	g_trace(G_T_TOPOLOGY, "g_slice_conf_hot()");
	g_topology_assert();
	gsp = gp->softc;
	gsl = gsp->hot;
	if(idx >= gsp->nhot) {
		gsl2 = g_malloc((idx + 1) * sizeof *gsl2, M_WAITOK | M_ZERO);
		if (gsp->hot != NULL)
			bcopy(gsp->hot, gsl2, gsp->nhot * sizeof *gsl2);
		gsp->hot = gsl2;
		if (gsp->hot != NULL)
			g_free(gsl);
		gsl = gsl2;
		gsp->nhot = idx + 1;
	}
	if (bootverbose)
		printf("GEOM: Add %s hot[%d] start %jd length %jd end %jd\n",
		    gp->name, idx, (intmax_t)offset, (intmax_t)length,
		    (intmax_t)(offset + length - 1));
	gsl[idx].offset = offset;
	gsl[idx].length = length;
	return (0);
}

struct g_geom *
g_slice_new(struct g_class *mp, u_int slices, struct g_provider *pp, struct g_consumer **cpp, void *extrap, int extra, g_slice_start_t *start)
{
	struct g_geom *gp;
	struct g_slicer *gsp;
	struct g_consumer *cp;
	void **vp;
	int error, i;

	g_topology_assert();
	vp = (void **)extrap;
	gp = g_new_geomf(mp, "%s", pp->name);
	gsp = g_slice_init(slices, extra);
	gsp->start = start;
	gp->access = g_slice_access;
	gp->orphan = g_slice_orphan;
	gp->softc = gsp;
	gp->start = g_slice_start;
	gp->spoiled = g_std_spoiled;
	gp->dumpconf = g_slice_dumpconf;
	cp = g_new_consumer(gp);
	error = g_attach(cp, pp);
	if (error == 0)
		error = g_access_rel(cp, 1, 0, 0);
	if (error) {
		if (cp->provider != NULL)
			g_detach(cp);
		g_destroy_consumer(cp);
		g_free(gsp->slices);
		g_free(gp->softc);
		g_destroy_geom(gp);
		return (NULL);
	}
	/* Find out if there are any magic bytes on the consumer */
	i = sizeof gsp->cfrontstuff;
	error = g_io_getattr("GEOM::frontstuff", cp, &i, &gsp->cfrontstuff);
	if (error)
		gsp->cfrontstuff = 0;
	*vp = gsp->softc;
	*cpp = cp;
	return (gp);
}

static void
g_slice_orphan(struct g_consumer *cp)
{
	struct g_geom *gp;
	struct g_provider *pp;
	int error;

	g_trace(G_T_TOPOLOGY, "g_slice_orphan(%p/%s)", cp, cp->provider->name);
	g_topology_assert();
	KASSERT(cp->provider->error != 0,
	    ("g_slice_orphan with error == 0"));

	gp = cp->geom;
	/* XXX: Not good enough we leak the softc and its suballocations */
	gp->flags |= G_GEOM_WITHER;
	error = cp->provider->error;
	LIST_FOREACH(pp, &gp->provider, provider)
		g_orphan_provider(pp, error);
	return;
}
