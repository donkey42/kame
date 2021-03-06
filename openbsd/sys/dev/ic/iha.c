/*	$OpenBSD: iha.c,v 1.3 2001/02/20 00:47:33 krw Exp $ */
/*
 * Initio INI-9xxxU/UW SCSI Device Driver
 *
 * Copyright (c) 2000 Ken Westerback
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR HIS RELATIVES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF MIND, USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *-------------------------------------------------------------------------
 *
 * Ported from i91u.c, provided by Initio Corporation, which credits:
 *
 * Device driver for the INI-9XXXU/UW or INIC-940/950  PCI SCSI Controller.
 *
 * FreeBSD
 *
 *  Written for 386bsd and FreeBSD by
 *	Winston Hung		<winstonh@initio.com>
 *
 * Copyright (c) 1997-99 Initio Corp.  All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <scsi/scsi_all.h>
#include <scsi/scsiconf.h>
#include <scsi/scsi_message.h>

#include <dev/ic/iha.h>

struct cfdriver iha_cd = {
	NULL, "iha", DV_DULL
};

struct scsi_adapter iha_switch = {
	iha_scsi_cmd,	/*  int (*scsi_cmd) __P((struct scsi_xfer *)); */
	iha_minphys,	/* void (*scsi_minphys) __P((struct buf *));   */
	NULL,		/*  int (*open_target_lu) __P((void));         */
	NULL		/*  int (*close_target_lu) __P((void));        */
};

struct scsi_device iha_dev = {
	NULL,		/* Use default error handler    */
	NULL,		/* have a queue, served by this */
	NULL,		/* have no async handler	*/
	NULL,		/* Use default 'done' routine   */
};

/*
 * SCSI Rate Table, indexed by FLAG_SCSI_RATE field of
 * TCS_Flags.
 */
static u_int8_t tul_rate_tbl[8] = {
	/* fast 20		  */
	/* nanosecond divide by 4 */
	12,	/* 50ns,  20M	  */
	18,	/* 75ns,  13.3M	  */
	25,	/* 100ns, 10M	  */
	31,	/* 125ns, 8M	  */
	37,	/* 150ns, 6.6M	  */
	43,	/* 175ns, 5.7M	  */
	50,	/* 200ns, 5M	  */
	62	/* 250ns, 4M	  */
};

static struct nvram tul_nvram;

static u_int8_t tul_dftNvRam[64] = {
	/* -- Header ------------------------------------ */
	0x25, 0xc9,		/* NVM_Signature          */
	0x40,			/* NVM_Size	          */
	0x01,			/* NVM_Revision	          */

	/* -- Host Adapter Structure -------------------- */
	0x95,			/* NVM_ModelByte0	  */
	0x00,			/* NVM_ModelByte1	  */
	0x00,			/* NVM_ModelInfo	  */
	0x01,			/* NVM_NumOfCh	          */
	BIOSCFG_DEFAULT,	/* NVM_BIOSConfig1	  */
	0,			/* NVM_BIOSConfig2	  */
	0,			/* NVM_HAConfig1	  */
	0,			/* NVM_HAConfig2	  */

	/* -- NVM_Scsi[0] ------------------------------- */
	7,			/* NVM_SCSI_Id            */
	CFG_DEFAULT,		/* NVM_SCSI_Cfg           */
	0,			/* NVM_SCSI_CfgByte2      */
	8,			/* NVM_SCSI_Targets       */
				/* NVM_SCSI_TargetFlags   */
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,

	/* -- NVM_Scsi[1] ------------------------------- */
	7,			/* NVM_SCSI_Id            */
	CFG_DEFAULT,		/* NVM_SCSI_Cfg           */
	0,			/* NVM_SCSI_CfgByte2      */
	8,			/* NVM_SCSI_Targets       */
				/* NVM_SCSI_TargetFlags   */
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
	FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT, FLAG_DEFAULT,
				
	0,0,0,0,0,0,0,0,0,0,	/* NVM_Reserved[10]       */
	0,0			/* NVM_CheckSum           */
};

static u_int8_t tul_data_over_run __P((struct iha_scsi_req_q *));

static void tul_push_sense_request __P((struct iha_softc *,
					   struct iha_scsi_req_q *));
static void tul_timeout __P((void *));
static int  tul_alloc_scbs __P((struct iha_softc *));

static void	 tul_read_eeprom    __P((bus_space_tag_t, bus_space_handle_t));
static void	 tul_se2_update_all __P((bus_space_tag_t, bus_space_handle_t));
static int	 tul_se2_rd_all	    __P((bus_space_tag_t, bus_space_handle_t));
static void	 tul_se2_wr	    __P((bus_space_tag_t, bus_space_handle_t,
					    u_int8_t, u_int16_t));
static void	 tul_se2_instr	    __P((bus_space_tag_t, bus_space_handle_t,
					    u_int8_t));
static u_int16_t tul_se2_rd	    __P((bus_space_tag_t, bus_space_handle_t,
					    u_int8_t));

static void tul_reset_scsi_bus __P((struct iha_softc *));
static void tul_reset_chip     __P((struct iha_softc *,
				       bus_space_tag_t, bus_space_handle_t));
static void tul_reset_dma      __P((bus_space_tag_t, bus_space_handle_t));

static void tul_reset_tcs  __P((struct tcs *, u_int8_t));

static void tul_print_info __P((struct iha_softc *, int));

static void tul_done_scb __P((struct iha_softc *, struct iha_scsi_req_q *));
static void tul_exec_scb __P((struct iha_softc *, struct iha_scsi_req_q *));

static void tul_main __P((struct iha_softc *,
			     bus_space_tag_t, bus_space_handle_t));
static void tul_scsi __P((struct iha_softc *,
			     bus_space_tag_t, bus_space_handle_t));

static int  tul_wait __P((struct iha_softc *,
			     bus_space_tag_t, bus_space_handle_t, u_int8_t));

static void tul_mark_busy_scb	__P((struct iha_scsi_req_q *));

static void tul_append_free_scb __P((struct iha_softc *,
					struct iha_scsi_req_q *));
static struct iha_scsi_req_q *tul_pop_free_scb __P((struct iha_softc *));

static void tul_append_done_scb __P((struct iha_softc *,
					struct iha_scsi_req_q *, u_int8_t));
static struct iha_scsi_req_q *tul_pop_done_scb	  __P((struct iha_softc *));

static void tul_append_pend_scb __P((struct iha_softc *,
					struct iha_scsi_req_q *));
static void tul_push_pend_scb	__P((struct iha_softc *,
					struct iha_scsi_req_q *));
static void tul_del_pend_scb	__P((struct iha_softc *,
					struct iha_scsi_req_q *));
static struct iha_scsi_req_q *tul_find_pend_scb	  __P((struct iha_softc *));

static void tul_sync_done __P((struct iha_softc *,
				  bus_space_tag_t, bus_space_handle_t));
static void tul_wdtr_done __P((struct iha_softc *,
				  bus_space_tag_t, bus_space_handle_t));
static void tul_bad_seq	  __P((struct iha_softc *));

static int  tul_next_state __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_1	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_2	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_3	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_4	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_5	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_6	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));
static int  tul_state_8	   __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));

static void tul_set_ssig __P((bus_space_tag_t,
				 bus_space_handle_t, u_int8_t, u_int8_t));

static int  tul_xpad_in	 __P((struct iha_softc *,
				 bus_space_tag_t, bus_space_handle_t));
static int  tul_xpad_out __P((struct iha_softc *,
				 bus_space_tag_t, bus_space_handle_t));

static int  tul_xfer_data __P((struct iha_scsi_req_q *,
				  bus_space_tag_t, bus_space_handle_t,
				  int direction));

static int  tul_status_msg __P((struct iha_softc *,
				   bus_space_tag_t, bus_space_handle_t));

static int  tul_msgin        __P((struct iha_softc *,
				     bus_space_tag_t, bus_space_handle_t));
static int  tul_msgin_sync   __P((struct iha_softc *));
static int  tul_msgin_extend __P((struct iha_softc *,
				     bus_space_tag_t, bus_space_handle_t));
static int  tul_msgin_ignore_wid_resid __P((struct iha_softc *,
					       bus_space_tag_t,
					       bus_space_handle_t));

static int  tul_msgout	      __P((struct iha_softc *,
				      bus_space_tag_t, bus_space_handle_t,
				      u_int8_t));
static void tul_msgout_abort  __P((struct iha_softc *,
				      bus_space_tag_t, bus_space_handle_t,
				      u_int8_t));
static int  tul_msgout_reject __P((struct iha_softc *,
				      bus_space_tag_t, bus_space_handle_t));
static int  tul_msgout_sync   __P((struct iha_softc *,
				      bus_space_tag_t, bus_space_handle_t));
static int  tul_msgout_wide   __P((struct iha_softc *,
				      bus_space_tag_t, bus_space_handle_t));

static void tul_select __P((struct iha_softc *,
			       bus_space_tag_t, bus_space_handle_t,
			       struct iha_scsi_req_q *, u_int8_t));

static void tul_busfree	 __P((struct iha_softc *,
				 bus_space_tag_t, bus_space_handle_t));
static int  tul_resel	 __P((struct iha_softc *,
				 bus_space_tag_t, bus_space_handle_t));

static void tul_abort_xs __P((struct iha_softc *,
				 struct scsi_xfer *, u_int8_t));

/*
 * iha_intr - the interrupt service routine for the iha driver
 */
int
iha_intr(arg)
	void *arg;
{
	bus_space_handle_t ioh;
	struct iha_softc *sc;
	bus_space_tag_t iot;
	int s;

	sc  = (struct iha_softc *)arg;
	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	if ((bus_space_read_1(iot, ioh, TUL_STAT0) & INTPD) == 0)
		return (0);

	s = splbio(); /* XXX - Or are interrupts off when ISR's are called? */

	if (sc->HCS_Semaph != SEMAPH_IN_MAIN) {
		/* XXX - need these inside a splbio()/splx()? */
		bus_space_write_1(iot, ioh, TUL_IMSK, MASK_ALL);
		sc->HCS_Semaph = SEMAPH_IN_MAIN;

		tul_main(sc, iot, ioh);

		sc->HCS_Semaph = ~SEMAPH_IN_MAIN;
		bus_space_write_1(iot, ioh, TUL_IMSK, (MASK_ALL & ~MSCMP));
	}

	splx(s);

	return (1);
}

/*
 * iha_scsi_cmd - start execution of a SCSI command. This is called
 *		  from the generic SCSI driver via the field
 *		  sc_adapter.scsi_cmd of iha_softc.
 */
int
iha_scsi_cmd(xs)
	struct scsi_xfer *xs;
{
	struct iha_scsi_req_q *pScb;
	struct iha_sg_element *sg;
	struct scsi_link *sc_link = xs->sc_link;
	struct iha_softc *sc = sc_link->adapter_softc;
	bus_dmamap_t dm;
	int error, nseg, i;

	if ((xs->cmdlen > 12) || (sc_link->target >= IHA_MAX_TARGETS)) {
		xs->error = XS_DRIVER_STUFFUP;
		return (COMPLETE);
	}

	pScb = tul_pop_free_scb(sc);
	if (pScb == NULL) {
		/* XXX - different xs->error/return if
		 * SCSI_POLL/_NOSLEEP? */
		xs->error = XS_BUSY;
		return (TRY_AGAIN_LATER);
	}

	pScb->SCB_Target = sc_link->target;
	pScb->SCB_Lun	 = sc_link->lun;
	pScb->SCB_Tcs	 = &sc->HCS_Tcs[pScb->SCB_Target];
	pScb->SCB_Flags	 = xs->flags;
	pScb->SCB_Ident  = MSG_IDENTIFYFLAG |
		(pScb->SCB_Lun & MSG_IDENTIFY_LUNMASK);

	if ((xs->cmd->opcode != REQUEST_SENSE)
	    && ((pScb->SCB_Flags & SCSI_POLL) == 0))
		pScb->SCB_Ident |= MSG_IDENTIFY_DISCFLAG;

	pScb->SCB_Xs	 = xs;
	pScb->SCB_CDBLen = xs->cmdlen;
	bcopy(xs->cmd, &pScb->SCB_CDB, xs->cmdlen);

	pScb->SCB_BufLen = xs->datalen;

	if (pScb->SCB_BufLen > 0) {
#ifdef TFS
		if (xs->flags & SCSI_DATA_UIO)
			error = bus_dmamap_load_uio(sc->sc_dmat,
			    pScb->SCB_Dmamap, (struct uio *)xs->data,
			    (xs->flags & SCSI_NOSLEEP) ?
			    BUS_DMA_NOWAIT : BUS_DMA_WAITOK);
		else
#endif /* TFS */
			error = bus_dmamap_load(sc->sc_dmat, pScb->SCB_Dmamap,
			    xs->data, pScb->SCB_BufLen, NULL,
			    (xs->flags & SCSI_NOSLEEP) ?
			    BUS_DMA_NOWAIT : BUS_DMA_WAITOK);

		if (error) {
			if (error == EFBIG)
				printf("%s: buffer needs >%d dma segments\n",
				    sc->sc_dev.dv_xname, IHA_MAX_SG_ENTRIES);
			else
				printf("%s: error %d loading dma map\n",
				    sc->sc_dev.dv_xname, error);

			tul_append_free_scb(sc, pScb); 

			xs->error = XS_DRIVER_STUFFUP;
			return (COMPLETE);
		}

		dm   = pScb->SCB_Dmamap;
		nseg = dm->dm_nsegs;

		if (nseg > 1) {
			sg = pScb->SCB_SGList;

			for (i=0; i < nseg; i++) {
				sg[i].SG_Len = dm->dm_segs[i].ds_len;
				sg[i].SG_Ptr = dm->dm_segs[i].ds_addr;
			}

			pScb->SCB_Flags	  |= FLAG_SG;
			pScb->SCB_SGLen	   = nseg;

			pScb->SCB_BufPAddr = pScb->SCB_SGPAddr;
		} else
			pScb->SCB_BufPAddr = dm->dm_segs[0].ds_addr;

		bus_dmamap_sync(sc->sc_dmat, pScb->SCB_Dmamap,
		    (pScb->SCB_Flags & SCSI_DATA_IN) ?
		    BUS_DMASYNC_PREREAD : BUS_DMASYNC_PREWRITE);
	}

	pScb->SCB_SGMax	  = pScb->SCB_SGLen;
	pScb->SCB_Timeout = xs->timeout;

	/*
	 * Always initialize the stimeout structure as it may 
	 * contain garbage that confuses timeout_del() later on.
	 * But, timeout_add() ONLY if we are not polling.
	 */
	timeout_set(&xs->stimeout, tul_timeout, pScb);
	if ((xs->flags & SCSI_POLL) == 0)
		timeout_add(&xs->stimeout, (xs->timeout/1000) * hz);

	tul_exec_scb(sc, pScb);

	if ((xs->flags & ITSDONE) == 0)
		return (SUCCESSFULLY_QUEUED);
	else
		return (COMPLETE);
}

/*
 * iha_init_tulip - initialize the inic-950 card and the rest of the
 *		    IHA_SOFTC structure supplied
 */
int
iha_init_tulip(sc)
	struct iha_softc *sc;
{
	struct iha_scsi_req_q *pScb;
	bus_space_handle_t ioh;
	struct nvram_scsi *pScsi;
	bus_space_tag_t iot;
	int i, error;

	iot = sc->sc_iot;
	ioh = sc->sc_ioh;

	tul_read_eeprom(iot, ioh);

	pScsi = &tul_nvram.NVM_Scsi[0];

	/*
	 * fill in the prototype scsi_link.
	 */
	sc->sc_link.adapter_softc    = sc;
	sc->sc_link.adapter	     = &iha_switch;
	sc->sc_link.device	     = &iha_dev;
	sc->sc_link.openings	     = 4; /* # xs's allowed per device */
	sc->sc_link.adapter_target   = pScsi->NVM_SCSI_Id;
	sc->sc_link.adapter_buswidth = pScsi->NVM_SCSI_Targets;

	/*
	 * fill in the rest of the IHA_SOFTC fields
	 */
	sc->HCS_Semaph	  = ~SEMAPH_IN_MAIN;
	sc->HCS_JSStatus0 = 0;
	sc->HCS_ActScb	  = NULL;

	TAILQ_INIT(&sc->HCS_FreeScb);
	TAILQ_INIT(&sc->HCS_PendScb);
	TAILQ_INIT(&sc->HCS_DoneScb);

	error = tul_alloc_scbs(sc);
	if (error != 0)
		return (error);

	for (i = 0, pScb = sc->HCS_Scb; i < IHA_MAX_SCB; i++, pScb++) {
		pScb->SCB_TagId = i;
		pScb->SCB_SGPAddr = sc->sc_dmamap->dm_segs[0].ds_addr
		    + i*sizeof(struct iha_scsi_req_q)
		    + OFFSETOF(struct iha_scsi_req_q, SCB_SGList);

		pScb->SCB_SenseLen   = sizeof(struct scsi_sense_data);
		pScb->SCB_SensePAddr = sc->sc_dmamap->dm_segs[0].ds_addr
		    + i*sizeof(struct iha_scsi_req_q)
		    + OFFSETOF(struct iha_scsi_req_q, SCB_ScsiSenseData);

		error = bus_dmamap_create(sc->sc_dmat,
		    (IHA_MAX_SG_ENTRIES-1) * PAGE_SIZE, IHA_MAX_SG_ENTRIES,
		    (IHA_MAX_SG_ENTRIES-1) * PAGE_SIZE, 0,
		    BUS_DMA_NOWAIT | BUS_DMA_ALLOCNOW, &pScb->SCB_Dmamap);

		if (error != 0) {
			printf("%s: couldn't create SCB DMA map, error = %d\n",
			    sc->sc_dev.dv_xname, error);
			return (error);
		}
		TAILQ_INSERT_TAIL(&sc->HCS_FreeScb, pScb, SCB_ScbList);
	}

	/* Mask all the interrupts */
	bus_space_write_1(iot, ioh, TUL_IMSK, MASK_ALL);

	/* Stop any I/O and reset the scsi module */
	tul_reset_dma(iot, ioh);
	bus_space_write_1(iot, ioh, TUL_SCTRL0, RSMOD);

	/* Program HBA's SCSI ID */
	bus_space_write_1(iot, ioh, TUL_SID, sc->sc_link.adapter_target << 4);

	/*
	 * Configure the channel as requested by the NVRAM settings read 
	 * into tul_nvram by tul_read_eeprom() above.
	 */

	if ((pScsi->NVM_SCSI_Cfg & CFG_EN_PAR) != 0)
		sc->HCS_SConf1 = (SCONFIG0DEFAULT | SPCHK);
	else
		sc->HCS_SConf1 = (SCONFIG0DEFAULT);
	bus_space_write_1(iot, ioh, TUL_SCONFIG0, sc->HCS_SConf1);

	/* selection time out in units of 1.6385 millisecond = 250 ms */
	bus_space_write_1(iot, ioh, TUL_STIMO, 153);

	/* Enable desired SCSI termination configuration read from eeprom */
	bus_space_write_1(iot, ioh, TUL_DCTRL0,
	    (pScsi->NVM_SCSI_Cfg & (CFG_ACT_TERM1 | CFG_ACT_TERM2)));

	bus_space_write_1(iot, ioh, TUL_GCTRL1,
	    ((pScsi->NVM_SCSI_Cfg & CFG_AUTO_TERM) >> 4) 
	        | (bus_space_read_1(iot, ioh, TUL_GCTRL1) & (~ATDEN)));

	for (i = 0; i < IHA_MAX_TARGETS; i++) {
		sc->HCS_Tcs[i].TCS_Flags = pScsi->NVM_SCSI_TargetFlags[i];
		tul_reset_tcs(&sc->HCS_Tcs[i], sc->HCS_SConf1);
	}

	tul_reset_chip(sc, iot, ioh);
	bus_space_write_1(iot, ioh, TUL_SIEN, ALL_INTERRUPTS);

	return (0);
}

/*
 * iha_minphys - reduce bp->b_bcount to something less than
 *		 or equal to the largest I/O possible through
 *		 the adapter. Called from higher layers
 *		 via sc->sc_adapter.scsi_minphys.
 */
void
iha_minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > ((IHA_MAX_SG_ENTRIES - 1) * PAGE_SIZE))
		bp->b_bcount = ((IHA_MAX_SG_ENTRIES - 1) * PAGE_SIZE);

	minphys(bp);
}

/*
 * tul_reset_dma - abort any active DMA xfer, reset tulip FIFO.
 */
static void
tul_reset_dma(iot, ioh)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	if ((bus_space_read_1(iot, ioh, TUL_ISTUS1) & XPEND) != 0) {
		/* if DMA xfer is pending, abort DMA xfer */
		bus_space_write_1(iot, ioh, TUL_DCMD, ABTXFR);
		/* wait Abort DMA xfer done */
		while ((bus_space_read_1(iot, ioh, TUL_ISTUS0) & DABT) == 0)
			;
	}

	bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
}

/*
 * tul_pop_free_scb - return the first free SCB, or NULL if there are none.
 */
static struct iha_scsi_req_q *
tul_pop_free_scb(sc)
	struct iha_softc *sc;
{
	struct iha_scsi_req_q *pScb;
	int s;

	s = splbio();

	pScb = TAILQ_FIRST(&sc->HCS_FreeScb);

	if (pScb != NULL) {
		pScb->SCB_Status = STATUS_RENT;
		TAILQ_REMOVE(&sc->HCS_FreeScb, pScb, SCB_ScbList);
	}

	splx(s);

	return (pScb);
}

/*
 * tul_append_free_scb - append the supplied SCB to the tail of the
 *                       HCS_FreeScb queue after clearing and resetting
 *			 everything possible.
 */
static void
tul_append_free_scb(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	int s;

	s = splbio();

	if (pScb->SCB_Xs != NULL)
		timeout_del(&pScb->SCB_Xs->stimeout);

	if (pScb == sc->HCS_ActScb)
		sc->HCS_ActScb = NULL;

	pScb->SCB_Status  = STATUS_QUEUED;
	pScb->SCB_HaStat  = HOST_OK;
	pScb->SCB_TaStat  = SCSI_OK;

	pScb->SCB_NxtStat  = 0;
	pScb->SCB_SGIdx	   = 0;
	pScb->SCB_SGMax	   = 0;
	pScb->SCB_Flags	   = 0;
	pScb->SCB_Target   = 0;
	pScb->SCB_Lun	   = 0;
	pScb->SCB_BufLen   = 0;
	pScb->SCB_SGLen	   = 0;
	pScb->SCB_CDBLen   = 0;
	pScb->SCB_Ident	   = 0;
	pScb->SCB_TagMsg   = 0;
	pScb->SCB_Timeout  = 0;
	pScb->SCB_BufPAddr = 0;

	pScb->SCB_Xs  = NULL;
	pScb->SCB_Tcs = NULL;

	bzero( pScb->SCB_CDB,		sizeof(pScb->SCB_CDB));
	bzero(&pScb->SCB_ScsiSenseData, sizeof(pScb->SCB_ScsiSenseData));
	bzero( pScb->SCB_SGList,	sizeof(pScb->SCB_SGList));

	/*
	 * SCB_TagId, SCB_SGPAddr, SCB_SenseLen, SCB_SGList
	 * SCB_SensePtr are set at initialization
	 * and never change
	 */

	TAILQ_INSERT_TAIL(&sc->HCS_FreeScb, pScb, SCB_ScbList);

	splx(s);
}

static void
tul_append_pend_scb(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	/* ASSUMPTION: only called within a splbio()/splx() pair */

	if (pScb == sc->HCS_ActScb)
		sc->HCS_ActScb = NULL;

	pScb->SCB_Status = STATUS_QUEUED;

	TAILQ_INSERT_TAIL(&sc->HCS_PendScb, pScb, SCB_ScbList);
}

static void
tul_push_pend_scb(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	int s;

	s = splbio();

	if (pScb == sc->HCS_ActScb)
		sc->HCS_ActScb = NULL;

	pScb->SCB_Status = STATUS_QUEUED;

	TAILQ_INSERT_HEAD(&sc->HCS_PendScb, pScb, SCB_ScbList);

	splx(s);
}

/*
 * tul_find_pend_scb - scan the pending queue for a SCB that can be
 *		       processed immediately. Return NULL if none found
 *		       and a pointer to the SCB if one is found. If there
 *		       is an active SCB, return NULL!
 */
static struct iha_scsi_req_q *
tul_find_pend_scb(sc)
	struct iha_softc *sc;
{
	struct iha_scsi_req_q *pScb;
	struct tcs *pTcs;
	int s;

	s = splbio();

	if (sc->HCS_ActScb != NULL)
		pScb = NULL;

	else
		TAILQ_FOREACH(pScb, &sc->HCS_PendScb, SCB_ScbList) {
			if ((pScb->SCB_Flags & SCSI_RESET) != 0)
				/* ALWAYS willing to reset a device */
				break;

			pTcs = pScb->SCB_Tcs;

			if ((pScb->SCB_TagMsg) != 0) {
				/*
				 * A Tagged I/O. OK to start If no
				 * non-tagged I/O is active on the same
				 * target
				 */
				if (pTcs->TCS_NonTagScb == NULL)
					break;

			} else	if (pScb->SCB_CDB[0] == REQUEST_SENSE) {
				/*
				 * OK to do a non-tagged request sense
				 * even if a non-tagged I/O has been
				 * started, 'cuz we don't allow any
				 * disconnect during a request sense op
				 */
				break;

			} else	if (pTcs->TCS_TagCnt == 0) {
				/*
				 * No tagged I/O active on this target,
				 * ok to start a non-tagged one if one
				 * is not already active
				 */
				if (pTcs->TCS_NonTagScb == NULL)
					break;
			}
		}

	splx(s);

	return (pScb);
}

/*
 * tul_del_pend_scb - remove pScb from HCS_PendScb
 */
static void
tul_del_pend_scb(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	int s;

	s = splbio();

	TAILQ_REMOVE(&sc->HCS_PendScb, pScb, SCB_ScbList);

	splx(s);
}

static void
tul_mark_busy_scb(pScb)
	struct iha_scsi_req_q *pScb;
{
	int  s;

	s = splbio();

	pScb->SCB_Status = STATUS_BUSY;

	if (pScb->SCB_TagMsg == 0)
		pScb->SCB_Tcs->TCS_NonTagScb = pScb;
	else
		pScb->SCB_Tcs->TCS_TagCnt++;

	splx(s);
}

static void
tul_append_done_scb(sc, pScb, hastat)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
	u_int8_t hastat;
{
	struct tcs *pTcs;
	int s;

	s = splbio();

	if (pScb->SCB_Xs != NULL)
		timeout_del(&pScb->SCB_Xs->stimeout);

	if (pScb == sc->HCS_ActScb)
		sc->HCS_ActScb = NULL;

	pTcs = pScb->SCB_Tcs;

	if (pScb->SCB_TagMsg != 0) {
		if (pTcs->TCS_TagCnt)
			pTcs->TCS_TagCnt--;
	} else if (pTcs->TCS_NonTagScb == pScb)
		pTcs->TCS_NonTagScb = NULL;

	pScb->SCB_Status = STATUS_QUEUED;
	pScb->SCB_HaStat = hastat;

	TAILQ_INSERT_TAIL(&sc->HCS_DoneScb, pScb, SCB_ScbList);

	splx(s);
}

static struct iha_scsi_req_q *
tul_pop_done_scb(sc)
	struct iha_softc *sc;
{
	struct iha_scsi_req_q *pScb;
	int s;

	s = splbio();

	pScb = TAILQ_FIRST(&sc->HCS_DoneScb);

	if (pScb != NULL) {
		pScb->SCB_Status = STATUS_RENT;
		TAILQ_REMOVE(&sc->HCS_DoneScb, pScb, SCB_ScbList);
	}

	splx(s);

	return (pScb);
}

/*
 * tul_abort_xs - find the SCB associated with the supplied xs and
 *                stop all processing on it, moving it to the done
 *                queue with the supplied host status value.
 */
static void
tul_abort_xs(sc, xs, hastat)
	struct iha_softc *sc;
	struct scsi_xfer *xs;
	u_int8_t hastat;
{
	struct iha_scsi_req_q *pScb;
	int i, s;

	s = splbio();

	/* Check the pending queue for the SCB pointing to xs */

	TAILQ_FOREACH(pScb, &sc->HCS_PendScb, SCB_ScbList)
		if (pScb->SCB_Xs == xs) {
			tul_del_pend_scb(sc, pScb);
			tul_append_done_scb(sc, pScb, hastat);
			splx(s);
			return;
		}

	/*
	 * If that didn't work, check all BUSY/SELECTING SCB's for one
	 * pointing to xs
	 */

	for (i = 0, pScb = sc->HCS_Scb; i < IHA_MAX_SCB; i++, pScb++)
		switch (pScb->SCB_Status) {
		case STATUS_BUSY:
		case STATUS_SELECT:
			if (pScb->SCB_Xs == xs) {
				tul_append_done_scb(sc, pScb, hastat);
				splx(s);
				return;
			}
			break;
		default:
			break;
		}
	
	splx(s);
}

/*
 * tul_bad_seq - a SCSI bus phase was encountered out of the
 *               correct/expected sequence. Reset the SCSI bus.
 */
static void
tul_bad_seq(sc)
	struct iha_softc *sc;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;

	if (pScb != NULL)
		tul_append_done_scb(sc, pScb, HOST_BAD_PHAS);

	tul_reset_scsi_bus(sc);
	tul_reset_chip(sc, sc->sc_iot, sc->sc_ioh);
}

/*
 * tul_push_sense_request - obtain auto sense data by pushing the
 *                          SCB needing it back onto the pending
 *			    queue with a REQUEST_SENSE CDB.
 */
static void
tul_push_sense_request(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	pScb->SCB_BufLen   = pScb->SCB_SenseLen;
	pScb->SCB_BufPAddr = pScb->SCB_SensePAddr;

	pScb->SCB_Flags &= ~(FLAG_SG | FLAG_DIR);
	pScb->SCB_Flags |= FLAG_RSENS | SCSI_DATA_IN;

	pScb->SCB_Ident &= ~MSG_IDENTIFY_DISCFLAG;

	pScb->SCB_TagMsg = 0;
	pScb->SCB_TaStat = SCSI_OK;

	bzero(pScb->SCB_CDB, sizeof(pScb->SCB_CDB));

	pScb->SCB_CDBLen = 6;
	pScb->SCB_CDB[0] = REQUEST_SENSE;
	pScb->SCB_CDB[4] = pScb->SCB_SenseLen;

	if ((pScb->SCB_Flags & SCSI_POLL) == 0)
		timeout_add(&pScb->SCB_Xs->stimeout,
		    (pScb->SCB_Timeout/1000) * hz);

	tul_push_pend_scb(sc, pScb);
}

/*
 * tul_main - process the active SCB, taking one off pending and making it
 *            active if necessary, and any done SCB's created as
 *            a result until there are no interrupts pending and no pending
 *            SCB's that can be started.
 */
static void
tul_main(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb;

	for (;;) {
tul_scsi_label:
		tul_scsi(sc, iot, ioh);

		while ((pScb = tul_pop_done_scb(sc)) != NULL) {

			switch (pScb->SCB_TaStat) {
			case SCSI_CMD_TERMINATED:
			case SCSI_ACA_ACTIVE:
			case SCSI_CHECK:
				pScb->SCB_Tcs->TCS_Flags &= 
				    ~(FLAG_SYNC_DONE | FLAG_WIDE_DONE);

				if ((pScb->SCB_Flags & FLAG_RSENS) != 0)
					/* Check condition on check condition*/
					pScb->SCB_HaStat = HOST_BAD_PHAS;

				else {
					tul_push_sense_request(sc, pScb);
					goto tul_scsi_label;
				}
				break;

			default:
				if ((pScb->SCB_Flags & FLAG_RSENS) != 0)
					/*
					 * Return the original SCSI_CHECK, not
					 * the status of the request sense
					 * command!		
					 */
					pScb->SCB_TaStat = SCSI_CHECK;
				break;
			}

			tul_done_scb(sc, pScb);
		}

		/*
		 * If there are no interrupts pending, or we can't start
		 * a pending sc, break out of the for(;;). Otherwise
		 * continue the good work with another call to
		 * tul_scsi().
		 */
		if (((bus_space_read_1(iot, ioh, TUL_STAT0) & INTPD) == 0) 
		    && (tul_find_pend_scb(sc) == NULL))
			break;
	}
}

/*
 * tul_scsi - service any outstanding interrupts. If there are none, try to
 *            start another SCB currently in the pending queue.
 */
static void
tul_scsi(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb;
	struct tcs *pTcs;
	u_int8_t stat;

	/* service pending interrupts asap */

	stat = bus_space_read_1(iot, ioh, TUL_STAT0);
	if ((stat & INTPD) != 0) {
		sc->HCS_JSStatus0 = stat;
		sc->HCS_JSStatus1 = bus_space_read_1(iot, ioh, TUL_STAT1);
		sc->HCS_JSInt     = bus_space_read_1(iot, ioh, TUL_SISTAT);

		sc->HCS_Phase = sc->HCS_JSStatus0 & PH_MASK;
		
		if ((sc->HCS_JSInt & SRSTD) != 0) {
			tul_reset_scsi_bus(sc);
			return;
		}
		
		if ((sc->HCS_JSInt & RSELED) != 0) {
			tul_resel(sc, iot, ioh);
			return;
		}

		if ((sc->HCS_JSInt & (STIMEO | DISCD)) != 0) {
			tul_busfree(sc, iot, ioh);
			return;
		}

		if ((sc->HCS_JSInt & (SCMDN | SBSRV)) != 0) {
			tul_next_state(sc, iot, ioh);
			return;
		}

		if ((sc->HCS_JSInt & SELED) != 0)
			tul_set_ssig(iot, ioh, 0, 0);
	}

	/*
	 * There were no interrupts pending which required action elsewhere, so
	 * see if it is possible to start the selection phase on a pending SCB
	 */
	if ((pScb = tul_find_pend_scb(sc)) == NULL)
		return;

	pTcs = pScb->SCB_Tcs;

	/* program HBA's SCSI ID & target SCSI ID */
	bus_space_write_1(iot, ioh, TUL_SID,
	    (sc->sc_link.adapter_target << 4) | pScb->SCB_Target);

	if ((pScb->SCB_Flags & SCSI_RESET) == 0) {
		bus_space_write_1(iot, ioh, TUL_SYNCM, pTcs->TCS_JS_Period);

		if ((pTcs->TCS_Flags & FLAG_NO_NEGOTIATE) == 0)
			tul_select(sc, iot, ioh, pScb, SELATNSTOP);

		else if (pScb->SCB_TagMsg != 0)
			tul_select(sc, iot, ioh, pScb, SEL_ATN3);

		else
			tul_select(sc, iot, ioh, pScb, SEL_ATN);

	} else {
		tul_select(sc, iot, ioh, pScb, SELATNSTOP);
		pScb->SCB_NxtStat = 8;
	}

	if ((pScb->SCB_Flags & SCSI_POLL) != 0) {
		for (; pScb->SCB_Timeout > 0; pScb->SCB_Timeout--) {
			if (tul_wait(sc, iot, ioh, NO_OP) == -1)
				break;
			if (tul_next_state(sc, iot, ioh) == -1)
				break;
			delay(1000); /* Only happens in boot, so it's ok */
		}

		/*
		 * Since done queue processing not done until AFTER this
		 * function returns, pScb is on the done queue, not
		 * the free queue at this point and still has valid data
		 *
		 * Conversely, xs->error has not been set yet
		 */
		if (pScb->SCB_Timeout == 0)
			tul_timeout(pScb);

		else if ((pScb->SCB_CDB[0] == INQUIRY)
		    && (pScb->SCB_Lun == 0)
		    && (pScb->SCB_HaStat == HOST_OK)
		    && (pScb->SCB_TaStat == SCSI_OK))
			tul_print_info(sc, pScb->SCB_Target);
	}
}

/*
 * tul_data_over_run - return HOST_OK for all SCSI opcodes where BufLen
 *                     is an 'Allocation Length'. All other SCSI opcodes
 *		       get HOST_DO_DU as they SHOULD have xferred all the
 *		       data requested.
 * 
 *		       The list of opcodes using 'Allocation Length' was
 * 		       found by scanning all the SCSI-3 T10 drafts. See
 *		       www.t10.org for the curious with a .pdf reader.
 */
static u_int8_t
tul_data_over_run(pScb)
	struct iha_scsi_req_q *pScb;
{
	switch (pScb->SCB_CDB[0]) {
	case 0x03: /* Request Sense                   SPC-2 */
	case 0x12: /* Inquiry                         SPC-2 */
	case 0x1a: /* Mode Sense (6 byte version)     SPC-2 */
	case 0x1c: /* Receive Diagnostic Results      SPC-2 */
	case 0x23: /* Read Format Capacities          MMC-2 */
	case 0x29: /* Read Generation                 SBC   */
	case 0x34: /* Read Position                   SSC-2 */
	case 0x37: /* Read Defect Data                SBC   */
	case 0x3c: /* Read Buffer                     SPC-2 */
	case 0x42: /* Read Sub Channel                MMC-2 */
	case 0x43: /* Read TOC/PMA/ATIP               MMC   */

	/* XXX - 2 with same opcode of 0x44? */
	case 0x44: /* Read Header/Read Density Suprt  MMC/SSC*/

	case 0x46: /* Get Configuration               MMC-2 */
	case 0x4a: /* Get Event/Status Notification   MMC-2 */
	case 0x4d: /* Log Sense                       SPC-2 */
	case 0x51: /* Read Disc Information           MMC   */
	case 0x52: /* Read Track Information          MMC   */
	case 0x59: /* Read Master CUE                 MMC   */
	case 0x5a: /* Mode Sense (10 byte version)    SPC-2 */
	case 0x5c: /* Read Buffer Capacity            MMC   */
	case 0x5e: /* Persistant Reserve In           SPC-2 */
	case 0x84: /* Receive Copy Results            SPC-2 */
	case 0xa0: /* Report LUNs                     SPC-2 */
	case 0xa3: /* Various Report requests         SBC-2/SCC-2*/
	case 0xa4: /* Report Key                      MMC-2 */
	case 0xad: /* Read DVD Structure              MMC-2 */
	case 0xb4: /* Read Element Status (Attached)  SMC   */
	case 0xb5: /* Request Volume Element Address  SMC   */
	case 0xb7: /* Read Defect Data (12 byte ver.) SBC   */
	case 0xb8: /* Read Element Status (Independ.) SMC   */
	case 0xba: /* Report Redundancy               SCC-2 */
	case 0xbd: /* Mechanism Status                MMC   */
	case 0xbe: /* Report Basic Redundancy         SCC-2 */

		return (HOST_OK);
		break;
		
	default:
		return (HOST_DO_DU);
		break;
	}
}

/*
 * tul_next_state - prcess the current SCB as requested in it's 
 *                  SCB_NxtStat member.
 */
static int
tul_next_state(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	if (sc->HCS_ActScb == NULL)
		return (-1);

	switch (sc->HCS_ActScb->SCB_NxtStat) {
	case 1:
		if (tul_state_1(sc, iot, ioh) == 3)
			goto state_3;
		break;

	case 2:
		switch (tul_state_2(sc, iot, ioh)) {
		case 3:	 goto state_3;
		case 4:	 goto state_4;
		default: break;
		}
		break;

	case 3:
	state_3:
		if (tul_state_3(sc, iot, ioh) == 4)
			goto state_4;
		break;

	case 4:
	state_4:
		switch (tul_state_4(sc, iot, ioh)) {
		case 0:	 return (0);
		case 6:	 goto state_6;
		default: break;
		}
		break;

	case 5:
		switch (tul_state_5(sc, iot, ioh)) {
		case 4:	 goto state_4;
		case 6:	 goto state_6;
		default: break;
		}
		break;

	case 6:
	state_6:
		tul_state_6(sc, iot, ioh);
		break;

	case 8:
		tul_state_8(sc, iot, ioh);
		break;

	default:
#ifdef IHA_DEBUG_STATE
		printf("[debug] -unknown state: %i-\n",
		    sc->HCS_ActScb->SCB_NxtStat);
#endif
		tul_bad_seq(sc);
		break;
	}

	return (-1);
}

/*
 * tul_state_1 - selection is complete after a SELATNSTOP. If the target
 *               has put the bus into MSG_OUT phase start wide/sync
 *               negotiation. Otherwise clear the FIFO and go to state 3,
 *	    	 which will send the SCSI CDB to the target.
 */
static int
tul_state_1(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;
	struct tcs *pTcs;
	u_int16_t flags;

	tul_mark_busy_scb(pScb);

	pTcs = pScb->SCB_Tcs;

	bus_space_write_1(iot, ioh, TUL_SCONFIG0, pTcs->TCS_SConfig0);

	/*
	 * If we are in PHASE_MSG_OUT, send
	 *     a) IDENT message (with tags if appropriate)
	 *     b) WDTR if the target is configured to negotiate wide xfers
	 *     ** OR **
	 *     c) SDTR if the target is configured to negotiate sync xfers
	 *	  but not wide ones
	 *
	 * If we are NOT, then the target is not asking for anything but
	 * the data/command, so go straight to state 3.
	 */
	if (sc->HCS_Phase == PHASE_MSG_OUT) {
		bus_space_write_1(iot, ioh, TUL_SCTRL1, (ESBUSIN | EHRSL));
		bus_space_write_1(iot, ioh, TUL_SFIFO,	pScb->SCB_Ident);

		if (pScb->SCB_TagMsg != 0) {
			bus_space_write_1(iot, ioh, TUL_SFIFO,
			    pScb->SCB_TagMsg);
			bus_space_write_1(iot, ioh, TUL_SFIFO,
			    pScb->SCB_TagId);
		}

		flags = pTcs->TCS_Flags;
		if ((flags & FLAG_NO_NEG_WIDE) == 0) {
			if (tul_msgout_wide(sc, iot, ioh) == -1)
				return (-1);
		} else if ((flags & FLAG_NO_NEG_SYNC) == 0) {
			if (tul_msgout_sync(sc, iot, ioh) == -1)
				return (-1);
		}

	} else {
		bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
		tul_set_ssig(iot, ioh, REQ | BSY | SEL | ATN, 0);
	}

	return (3);
}

/*
 * tul_state_2 - selection is complete after a SEL_ATN or SEL_ATN3. If the SCSI
 *		 CDB has already been send, go to state 4 to start the data
 *               xfer. Otherwise reset the FIFO and go to state 3, sending 
 *		 the SCSI CDB.
 */
static int
tul_state_2(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;

	tul_mark_busy_scb(pScb);

	bus_space_write_1(iot, ioh, TUL_SCONFIG0, pScb->SCB_Tcs->TCS_SConfig0);

	if ((sc->HCS_JSStatus1 & CPDNE) != 0)
		return (4);

	bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);

	tul_set_ssig(iot, ioh, REQ | BSY | SEL | ATN, 0);

	return (3);
}

/*
 * tul_state_3 - send the SCSI CDB to the target, processing any status
 *		 or other messages received until that is done or
 *               abandoned.
 */
static int
tul_state_3(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;
	u_int16_t flags;

	for (;;)
		switch (sc->HCS_Phase) {
		case PHASE_CMD_OUT:
			bus_space_write_multi_1(iot, ioh, TUL_SFIFO,
			    pScb->SCB_CDB, pScb->SCB_CDBLen);
			if (tul_wait(sc, iot, ioh, XF_FIFO_OUT) == -1)
				return (-1);
			else if (sc->HCS_Phase == PHASE_CMD_OUT) {
				tul_bad_seq(sc);
				return (-1);
			} else
				return (4);

		case PHASE_MSG_IN:
			pScb->SCB_NxtStat = 3;
			if (tul_msgin(sc, iot, ioh) == -1)
				return (-1);
			break;

		case PHASE_STATUS_IN:
			if (tul_status_msg(sc, iot, ioh) == -1)
				return (-1);
			break;

		case PHASE_MSG_OUT:
			flags = pScb->SCB_Tcs->TCS_Flags;
			if ((flags & FLAG_NO_NEG_SYNC) != 0) {
				if (tul_msgout(sc, iot, ioh, MSG_NOOP) == -1)
					return (-1);
			} else if (tul_msgout_sync(sc, iot, ioh) == -1)
				return (-1);
			break;

		default:
			printf("[debug] -s3- bad phase = %d\n", sc->HCS_Phase);
			tul_bad_seq(sc);
			return (-1);
		}
}

/* 
 * tul_state_4 - start a data xfer. Handle any bus state
 *               transitions until PHASE_DATA_IN/_OUT
 *               or the attempt is abandoned. If there is 
 *               no data to xfer, go to state 6 and finish
 *               processing the current SCB.
 */
static int
tul_state_4(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;

	if ((pScb->SCB_Flags & FLAG_DIR) == FLAG_DIR)
		return (6); /* Both dir flags set => NO xfer was requested */

	for (;;) {
		if (pScb->SCB_BufLen == 0)
			return (6);

		switch (sc->HCS_Phase) {
		case PHASE_STATUS_IN:
			if ((pScb->SCB_Flags & FLAG_DIR) != 0)
				pScb->SCB_HaStat = tul_data_over_run(pScb);
			if ((tul_status_msg(sc, iot, ioh)) == -1)
				return (-1);
			break;

		case PHASE_MSG_IN:
			pScb->SCB_NxtStat = 4;
			if (tul_msgin(sc, iot, ioh) == -1)
				return (-1);
			break;

		case PHASE_MSG_OUT:
			if ((sc->HCS_JSStatus0 & SPERR) != 0) {
				pScb->SCB_BufLen = 0;
				pScb->SCB_HaStat = HOST_SPERR;
				if (tul_msgout(sc, iot, ioh,
					MSG_INITIATOR_DET_ERR) == -1)
					return (-1);
				else
					return (6);
			} else {
				if (tul_msgout(sc, iot, ioh, MSG_NOOP) == -1)
					return (-1);
			}
			break;

		case PHASE_DATA_IN:
			return (tul_xfer_data(pScb, iot, ioh, SCSI_DATA_IN));

		case PHASE_DATA_OUT:
			return (tul_xfer_data(pScb, iot, ioh, SCSI_DATA_OUT));

		default:
			tul_bad_seq(sc);
			return (-1);
		}
	}
}

/*
 * tul_state_5 - handle the partial or final completion of the current
 *               data xfer. If DMA is still active stop it. If there is 
 *		 more data to xfer, go to state 4 and start the xfer.
 *               If not go to state 6 and finish the SCB.
 */
static int
tul_state_5(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;
	struct iha_sg_element *pSg;
	u_int32_t cnt;
	u_int16_t period;
	u_int8_t stat;
	long xcnt;  /* cannot use unsigned!! see code: if (xcnt < 0) */
	int i;

	cnt = bus_space_read_4(iot, ioh, TUL_STCNT0) & TCNT;
	
	/*
	 * Stop any pending DMA activity and check for parity error.
	 */

	if ((bus_space_read_1(iot, ioh, TUL_DCMD) & XDIR) != 0) {
		/* Input Operation */
		if ((sc->HCS_JSStatus0 & SPERR) != 0)
			pScb->SCB_HaStat = HOST_SPERR;

		if ((bus_space_read_1(iot, ioh, TUL_ISTUS1) & XPEND) != 0) {
			bus_space_write_1(iot, ioh, TUL_DCTRL0,
			    bus_space_read_1(iot, ioh, TUL_DCTRL0) | SXSTP);
			while (bus_space_read_1(iot, ioh, TUL_ISTUS1) & XPEND)
				;
		}

	} else {
		/* Output Operation */
		if ((sc->HCS_JSStatus1 & SXCMP) == 0) {
			period = pScb->SCB_Tcs->TCS_JS_Period; 
			if ((period & PERIOD_WIDE_SCSI) != 0)
				cnt += (bus_space_read_1(iot, ioh,
					    TUL_SFIFOCNT) & FIFOC) << 1;
			else
				cnt += (bus_space_read_1(iot, ioh,
					    TUL_SFIFOCNT) & FIFOC);
		}

		if ((bus_space_read_1(iot, ioh, TUL_ISTUS1) & XPEND) != 0) {
			bus_space_write_1(iot, ioh, TUL_DCMD, ABTXFR);
			do
				stat = bus_space_read_1(iot, ioh, TUL_ISTUS0);
			while ((stat & DABT) == 0);
		}

		if ((cnt == 1) && (sc->HCS_Phase == PHASE_DATA_OUT)) {
			if (tul_wait(sc, iot, ioh, XF_FIFO_OUT) == -1)
				return (-1);
			cnt = 0;

		} else if ((sc->HCS_JSStatus1 & SXCMP) == 0)
			bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
	}

	if (cnt == 0) {
		pScb->SCB_BufLen = 0;
		return (6);
	}

	/* Update active data pointer and restart the I/O at the new point */

	xcnt		 = pScb->SCB_BufLen - cnt; /* xcnt == bytes xferred */
	pScb->SCB_BufLen = cnt;			   /* cnt  == bytes left    */

	if ((pScb->SCB_Flags & FLAG_SG) != 0) {
		pSg = &pScb->SCB_SGList[pScb->SCB_SGIdx];
		for (i = pScb->SCB_SGIdx; i < pScb->SCB_SGMax; pSg++, i++) {
			xcnt -= pSg->SG_Len;
			if (xcnt < 0) {
				xcnt += pSg->SG_Len;

				pSg->SG_Ptr += xcnt;
				pSg->SG_Len -= xcnt;

				pScb->SCB_BufPAddr += (i - pScb->SCB_SGIdx)
					* sizeof(struct iha_sg_element);
				pScb->SCB_SGLen	  = pScb->SCB_SGMax - i;
				pScb->SCB_SGIdx	  = i;

				return (4);
			}
		}
		return (6);

	} else
		pScb->SCB_BufPAddr += xcnt;

	return (4);
}

/*
 * tul_state_6 - finish off the active scb (may require several
 *               iterations if PHASE_MSG_IN) and return -1 to indicate 
 *		 the bus is free.
 */
static int
tul_state_6(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	for (;;)
		switch (sc->HCS_Phase) {
		case PHASE_STATUS_IN:
			if (tul_status_msg(sc, iot, ioh) == -1)
				return (-1);
			break;

		case PHASE_MSG_IN:
			sc->HCS_ActScb->SCB_NxtStat = 6;
			if ((tul_msgin(sc, iot, ioh)) == -1)
				return (-1);
			break;

		case PHASE_MSG_OUT:
			if ((tul_msgout(sc, iot, ioh, MSG_NOOP)) == -1)
				return (-1);
			break;

		case PHASE_DATA_IN:
			if (tul_xpad_in(sc, iot, ioh) == -1)
				return (-1);
			break;

		case PHASE_DATA_OUT:
			if (tul_xpad_out(sc, iot, ioh) == -1)
				return (-1);
			break;

		default:
			tul_bad_seq(sc);
			return (-1);
		}
}

/*
 * tul_state_8 - reset the active device and all busy SCBs using it
 */
static int
tul_state_8(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb;
	u_int32_t i;
	u_int8_t tar;

	if (sc->HCS_Phase == PHASE_MSG_OUT) {
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_BUS_DEV_RESET);

		pScb = sc->HCS_ActScb;
		
		/* This SCB finished correctly -- resetting the device */
		tul_append_done_scb(sc, pScb, HOST_OK);

		tul_reset_tcs(pScb->SCB_Tcs, sc->HCS_SConf1);

		tar = pScb->SCB_Target;
		for (i = 0, pScb = sc->HCS_Scb; i < IHA_MAX_SCB; i++, pScb++)
			if (pScb->SCB_Target == tar)
				switch (pScb->SCB_Status) {
				case STATUS_BUSY:
					tul_append_done_scb(sc,
					    pScb, HOST_DEV_RST);
					break;

				case STATUS_SELECT:
					tul_push_pend_scb(sc, pScb);
					break;
					
				default:
					break;
				}

		sc->HCS_Flags |= FLAG_EXPECT_DISC;

		if (tul_wait(sc, iot, ioh, XF_FIFO_OUT) == -1)
			return (-1);
	}

	tul_bad_seq(sc);
	return (-1);
}

/*
 * tul_xfer_data - initiate the DMA xfer of the data
 */
static int
tul_xfer_data(pScb, iot, ioh, direction)
	struct iha_scsi_req_q *pScb;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	int direction;
{
	u_int32_t xferlen;
	u_int8_t xfertype;

	if ((pScb->SCB_Flags & FLAG_DIR) != direction)
		return (6); /* wrong direction, abandon I/O */

	bus_space_write_4(iot, ioh, TUL_STCNT0, pScb->SCB_BufLen);

	if ((pScb->SCB_Flags & FLAG_SG) == 0) {
		xferlen  = pScb->SCB_BufLen;
		xfertype = (direction == SCSI_DATA_IN) ? ST_X_IN : ST_X_OUT;

	} else {
		xferlen  = pScb->SCB_SGLen * sizeof(struct iha_sg_element);
		xfertype = (direction == SCSI_DATA_IN) ? ST_SG_IN : ST_SG_OUT;
	}

	bus_space_write_4(iot, ioh, TUL_DXC,  xferlen);
	bus_space_write_4(iot, ioh, TUL_DXPA, pScb->SCB_BufPAddr);
	bus_space_write_1(iot, ioh, TUL_DCMD, xfertype);

	bus_space_write_1(iot, ioh, TUL_SCMD,
	    (direction == SCSI_DATA_IN) ? XF_DMA_IN : XF_DMA_OUT);

	pScb->SCB_NxtStat = 5;

	return (0);
}

static int
tul_xpad_in(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;

	if ((pScb->SCB_Flags & FLAG_DIR) != 0)
		pScb->SCB_HaStat = HOST_DO_DU;

	for (;;) {
		if ((pScb->SCB_Tcs->TCS_JS_Period & PERIOD_WIDE_SCSI) != 0)
			bus_space_write_4(iot, ioh, TUL_STCNT0, 2);
		else
			bus_space_write_4(iot, ioh, TUL_STCNT0, 1);

		switch (tul_wait(sc, iot, ioh, XF_FIFO_IN)) {
		case -1:
			return (-1);

		case PHASE_DATA_IN:
			bus_space_read_1(iot, ioh, TUL_SFIFO);
			break;

		default:
			bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
			return (6);
		}
	}
}

static int
tul_xpad_out(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb = sc->HCS_ActScb;

	if ((pScb->SCB_Flags & FLAG_DIR) != 0)
		pScb->SCB_HaStat = HOST_DO_DU;

	for (;;) {
		if ((pScb->SCB_Tcs->TCS_JS_Period & PERIOD_WIDE_SCSI) != 0)
			bus_space_write_4(iot, ioh, TUL_STCNT0, 2);
		else
			bus_space_write_4(iot, ioh, TUL_STCNT0, 1);

		bus_space_write_1(iot, ioh, TUL_SFIFO, 0);

		switch (tul_wait(sc, iot, ioh, XF_FIFO_OUT)) {
		case -1:
			return (-1);

		case PHASE_DATA_OUT:
			break;

		default:
			/* Disable wide CPU to allow read 16 bits */
			bus_space_write_1(iot, ioh, TUL_SCTRL1, EHRSL);
			bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
			return (6);
		}
	}
}

static int
tul_status_msg(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb;
	u_int8_t msg;
	int phase;

	if ((phase = tul_wait(sc, iot, ioh, CMD_COMP)) == -1)
		return (-1);

	pScb = sc->HCS_ActScb;
	
	pScb->SCB_TaStat = bus_space_read_1(iot, ioh, TUL_SFIFO);

	if (phase == PHASE_MSG_OUT) {
		if ((sc->HCS_JSStatus0 & SPERR) == 0)
			bus_space_write_1(iot, ioh, TUL_SFIFO,
			    MSG_NOOP);
		else
			bus_space_write_1(iot, ioh, TUL_SFIFO,
			    MSG_PARITY_ERROR);

		return (tul_wait(sc, iot, ioh, XF_FIFO_OUT));

	} else if (phase == PHASE_MSG_IN) {
		msg = bus_space_read_1(iot, ioh, TUL_SFIFO);

		if ((sc->HCS_JSStatus0 & SPERR) != 0)
			switch (tul_wait(sc, iot, ioh, MSG_ACCEPT)) {
			case -1:
				return (-1);
			case PHASE_MSG_OUT:
				bus_space_write_1(iot, ioh, TUL_SFIFO,
				    MSG_PARITY_ERROR);
				return (tul_wait(sc, iot, ioh, XF_FIFO_OUT));
			default:
				tul_bad_seq(sc);
				return (-1);
			}

		if (msg == MSG_CMDCOMPLETE) {
			if ((pScb->SCB_TaStat 
				& (SCSI_INTERM | SCSI_BUSY)) == SCSI_INTERM) {
				tul_bad_seq(sc);
				return (-1);
			}
			sc->HCS_Flags |= FLAG_EXPECT_DONE_DISC;
			bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
			return (tul_wait(sc, iot, ioh, MSG_ACCEPT));
		}

		if ((msg == MSG_LINK_CMD_COMPLETE)
		    || (msg == MSG_LINK_CMD_COMPLETEF)) {
			if ((pScb->SCB_TaStat 
				 & (SCSI_INTERM | SCSI_BUSY)) == SCSI_INTERM)
				return (tul_wait(sc, iot, ioh, MSG_ACCEPT));
		}
	}

	tul_bad_seq(sc);
	return (-1);
}

/*
 * tul_busfree - SCSI bus free detected as a result of a TIMEOUT or
 *		 DISCONNECT interrupt. Reset the tulip FIFO and
 *		 SCONFIG0 and enable hardware reselect. Move any active
 *		 SCB to HCS_DoneScb list. Return an appropriate host status
 *		 if an I/O was active.
 */
static void
tul_busfree(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb;

	bus_space_write_1(iot, ioh, TUL_SCTRL0,	  RSFIFO);
	bus_space_write_1(iot, ioh, TUL_SCONFIG0, SCONFIG0DEFAULT);
	bus_space_write_1(iot, ioh, TUL_SCTRL1,	  EHRSL);

	pScb = sc->HCS_ActScb;

	if (pScb != NULL) {
		if (pScb->SCB_Status == STATUS_SELECT)
			/* selection timeout   */
			tul_append_done_scb(sc, pScb, HOST_SEL_TOUT);
		else
			/* Unexpected bus free */
			tul_append_done_scb(sc, pScb, HOST_BAD_PHAS);

	}
}

static void
tul_reset_scsi_bus(sc)
	struct iha_softc *sc;
{
	struct iha_scsi_req_q *pScb;
	struct tcs *pTcs;
	int i, s;

	s = splbio();

	tul_reset_dma(sc->sc_iot, sc->sc_ioh);

	for (i = 0, pScb = sc->HCS_Scb; i < IHA_MAX_SCB; i++, pScb++)
		switch (pScb->SCB_Status) {
		case STATUS_BUSY:
			tul_append_done_scb(sc, pScb, HOST_SCSI_RST);
			break;

		case STATUS_SELECT:
			tul_push_pend_scb(sc, pScb);
			break;

		default:
			break;
		}

	for (i = 0, pTcs = sc->HCS_Tcs; i < IHA_MAX_TARGETS; i++, pTcs++)
		tul_reset_tcs(pTcs, sc->HCS_SConf1);

	splx(s);
}

/*
 * tul_resel - handle a detected SCSI bus reselection request.
 */
static int
tul_resel(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct iha_scsi_req_q *pScb;
	struct tcs *pTcs;
	u_int8_t tag, target, lun, msg, abortmsg;

	if (sc->HCS_ActScb != NULL) {
		if ((sc->HCS_ActScb->SCB_Status == STATUS_SELECT))
			tul_push_pend_scb(sc, sc->HCS_ActScb); /* sets ActScb to NULL */
		else
			sc->HCS_ActScb = NULL;
	}

	target = bus_space_read_1(iot, ioh, TUL_SBID);
	lun    = bus_space_read_1(iot, ioh, TUL_SALVC) & MSG_IDENTIFY_LUNMASK;

	pTcs = &sc->HCS_Tcs[target];

	bus_space_write_1(iot, ioh, TUL_SCONFIG0, pTcs->TCS_SConfig0);
	bus_space_write_1(iot, ioh, TUL_SYNCM,	  pTcs->TCS_JS_Period);

	abortmsg = MSG_ABORT; /* until a valid tag has been obtained */

	if (pTcs->TCS_NonTagScb != NULL)
		/* There is a non-tagged I/O active on the target */
		pScb = pTcs->TCS_NonTagScb;

	else {
		/* 
		 * Since there is no active non-tagged operation
		 * read the tag type, the tag itself, and find
		 * the appropriate pScb by indexing HCS_Scb with
		 * the tag.
		 */

		switch (tul_wait(sc, iot, ioh, MSG_ACCEPT)) {
		case -1:
			return (-1);
		case PHASE_MSG_IN:
			bus_space_write_4(iot, ioh, TUL_STCNT0, 1);
			if ((tul_wait(sc, iot, ioh, XF_FIFO_IN)) == -1)
				return (-1);
			break;
		default:
			goto abort;
		}

		msg = bus_space_read_1(iot, ioh, TUL_SFIFO); /* Read Tag Msg */

		if ((msg < MSG_SIMPLE_Q_TAG) || (msg > MSG_ORDERED_Q_TAG))
			goto abort;

		switch (tul_wait(sc, iot, ioh, MSG_ACCEPT)) {
		case -1:
			return (-1);
		case PHASE_MSG_IN:
			bus_space_write_4(iot, ioh, TUL_STCNT0, 1);
			if ((tul_wait(sc, iot, ioh, XF_FIFO_IN)) == -1)
				return (-1);
			break;
		default:
			goto abort;
		}

		tag  = bus_space_read_1(iot, ioh, TUL_SFIFO); /* Read Tag ID */
		pScb = &sc->HCS_Scb[tag];

		abortmsg = MSG_ABORT_TAG; /* Now that we have valdid tag! */
	}

	if ((pScb->SCB_Target != target)  
	    || (pScb->SCB_Lun != lun)
	    || (pScb->SCB_Status != STATUS_BUSY)) {
abort:
		tul_msgout_abort(sc, iot, ioh, abortmsg);
		return (-1);
	}

	sc->HCS_ActScb = pScb;

	if (tul_wait(sc, iot, ioh, MSG_ACCEPT) == -1)
		return (-1);

	return(tul_next_state(sc, iot, ioh));
}

static int
tul_msgin(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	u_int16_t flags;
	u_int8_t msg;
	int phase;

	for (;;) {
		if ((bus_space_read_1(iot, ioh, TUL_SFIFOCNT) & FIFOC) > 0)
			bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);

		bus_space_write_4(iot, ioh, TUL_STCNT0, 1);

		phase = tul_wait(sc, iot, ioh, XF_FIFO_IN);
		msg   = bus_space_read_1(iot, ioh, TUL_SFIFO);

		switch (msg) {
		case MSG_DISCONNECT:
			sc->HCS_Flags |= FLAG_EXPECT_DISC;
			if (tul_wait(sc, iot, ioh, MSG_ACCEPT) != -1)
				tul_bad_seq(sc);
			phase = -1;
			break;
		case MSG_SAVEDATAPOINTER:
		case MSG_RESTOREPOINTERS:
		case MSG_NOOP:
			phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);
			break;
		case MSG_MESSAGE_REJECT:
			/* XXX - need to clear FIFO like other 'Clear ATN'?*/
			tul_set_ssig(iot, ioh, REQ | BSY | SEL | ATN, 0);
			flags = sc->HCS_ActScb->SCB_Tcs->TCS_Flags;
			if ((flags & FLAG_NO_NEG_SYNC) == 0)
				tul_set_ssig(iot, ioh, REQ | BSY | SEL, ATN);
			phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);
			break;
		case MSG_EXTENDED:
			phase = tul_msgin_extend(sc, iot, ioh);
			break;
		case MSG_IGN_WIDE_RESIDUE:
			phase = tul_msgin_ignore_wid_resid(sc, iot, ioh);
			break;
		case MSG_CMDCOMPLETE:
			sc->HCS_Flags |= FLAG_EXPECT_DONE_DISC;
			bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
			phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);
			if (phase != -1) {
				tul_bad_seq(sc);
				return (-1);
			}
			break;
		default:
			printf("[debug] tul_msgin: bad msg type: %d\n", msg);
			phase = tul_msgout_reject(sc, iot, ioh);
			break;
		}

		if (phase != PHASE_MSG_IN)
			return (phase);
	}
	/* NOTREACHED */
}

static int
tul_msgin_ignore_wid_resid(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	int phase;

	phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);

	if (phase == PHASE_MSG_IN) {
		if (tul_wait(sc, iot, ioh, XF_FIFO_IN) == -1)
			return (-1);

		bus_space_write_1(iot, ioh, TUL_SFIFO, 0); /* put pad	     */
		bus_space_read_1 (iot, ioh, TUL_SFIFO);	   /* get IGNORE     */
		bus_space_read_1 (iot, ioh, TUL_SFIFO);	   /* get pad	     */

		return (tul_wait(sc, iot, ioh, MSG_ACCEPT));
	}
	else
		return (phase);
}

static int
tul_msgin_extend(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	u_int16_t flags;
	int i, phase, msglen, msgcode;

	/* XXX - can we just stop reading and reject, or do we have to
	 *	 read all input, discarding the excess, and then reject
	 */
	for (i = 0; i < IHA_MAX_EXTENDED_MSG; i++) {
		phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);

		if (phase != PHASE_MSG_IN)
			return (phase);

		bus_space_write_4(iot, ioh, TUL_STCNT0, 1);

		if (tul_wait(sc, iot, ioh, XF_FIFO_IN) == -1)
			return (-1);

		sc->HCS_Msg[i] = bus_space_read_1(iot, ioh, TUL_SFIFO);

		if (sc->HCS_Msg[0] == i)
			break;
	}

	msglen	= sc->HCS_Msg[0];
	msgcode = sc->HCS_Msg[1];

	if ((msglen == MSG_EXT_SDTR_LEN) && (msgcode == MSG_EXT_SDTR)) {
		if (tul_msgin_sync(sc) == 0) {
			tul_sync_done(sc, iot, ioh);
			return (tul_wait(sc, iot, ioh, MSG_ACCEPT));
		}

		tul_set_ssig(iot, ioh, REQ | BSY | SEL, ATN);

		phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);
		if (phase != PHASE_MSG_OUT)
			return (phase);

		/* Clear FIFO for important message - final SYNC offer */
		bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);

		tul_sync_done(sc, iot, ioh); /* This is our final offer */

		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXTENDED);
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_SDTR_LEN);
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_SDTR);
		bus_space_write_1(iot, ioh, TUL_SFIFO, sc->HCS_Msg[2]);
		bus_space_write_1(iot, ioh, TUL_SFIFO, sc->HCS_Msg[3]);

	} else if ((msglen == MSG_EXT_WDTR_LEN) && (msgcode == MSG_EXT_WDTR)) {

		flags = sc->HCS_ActScb->SCB_Tcs->TCS_Flags;

		if ((flags & FLAG_NO_WIDE) != 0)
			sc->HCS_Msg[2] = 0;	/* Offer async xfers only    */

		else if (sc->HCS_Msg[2] > 2)	/* BAD MSG: 2 is max  value  */
			return (tul_msgout_reject(sc, iot, ioh));

		else if (sc->HCS_Msg[2] == 2)	/* a request for 32 bit xfers*/
			sc->HCS_Msg[2] = 1;	/* Offer 16 instead	     */

		else {
			tul_wdtr_done(sc, iot, ioh);
			if ((flags & FLAG_NO_NEG_SYNC) == 0)
				tul_set_ssig(iot, ioh, REQ | BSY | SEL, ATN);
			return (tul_wait(sc, iot, ioh, MSG_ACCEPT));
		}

		tul_set_ssig(iot, ioh, REQ | BSY | SEL, ATN);

		phase = tul_wait(sc, iot, ioh, MSG_ACCEPT);
		if (phase != PHASE_MSG_OUT)
			return (phase);

		/* WDTR msg out */
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXTENDED);
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_WDTR_LEN);
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_WDTR);
		bus_space_write_1(iot, ioh, TUL_SFIFO, sc->HCS_Msg[2]);

	} else
		return (tul_msgout_reject(sc, iot, ioh));

	return (tul_wait(sc, iot, ioh, XF_FIFO_OUT));
}

/*
 * tul_msgin_sync - check SDTR msg in HCS_Msg. If the offer is
 *                  acceptable leave HCS_Msg as is and return 0.
 *                  If the negotiation must continue, modify HCS_Msg
 *                  as needed and return 1. Else return 0.
 */
static int
tul_msgin_sync(sc)
	struct iha_softc *sc;
{
	u_int16_t flags;
	u_int8_t default_period;
	int newoffer;

	flags = sc->HCS_ActScb->SCB_Tcs->TCS_Flags;

	default_period = tul_rate_tbl[flags & FLAG_SCSI_RATE];

	if (sc->HCS_Msg[3] == 0) /* target offered async only. Accept it. */
		return (0);

	newoffer = 0;

	if ((flags & FLAG_NO_SYNC) != 0) {
		sc->HCS_Msg[3] = 0;
		newoffer   = 1;
	}

	if (sc->HCS_Msg[3] > IHA_MAX_TARGETS-1) {
		sc->HCS_Msg[3] = IHA_MAX_TARGETS-1;
		newoffer   = 1;
	}

	if (sc->HCS_Msg[2] < default_period) {
		sc->HCS_Msg[2] = default_period;
		newoffer   = 1;
	}

	if (sc->HCS_Msg[2] >= 59) {
		sc->HCS_Msg[3] = 0;
		newoffer   = 1;
	}

	return (newoffer);
}

static int
tul_msgout(sc, iot, ioh, msg)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t   msg;
{
	bus_space_write_1(iot, ioh, TUL_SFIFO, msg);

	return (tul_wait(sc, iot, ioh, XF_FIFO_OUT));
}

static void
tul_msgout_abort(sc, iot, ioh,	aborttype)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t	   aborttype;
{
	tul_set_ssig(iot, ioh, REQ | BSY | SEL, ATN);

	switch (tul_wait(sc, iot, ioh, MSG_ACCEPT)) {
	case -1:
		break;

	case PHASE_MSG_OUT:
		bus_space_write_1(iot, ioh, TUL_SFIFO, aborttype );

		sc->HCS_Flags |= FLAG_EXPECT_DISC;

		if (tul_wait(sc, iot, ioh, XF_FIFO_OUT) != -1)
			tul_bad_seq(sc);
		break;

	default:
		tul_bad_seq(sc);
		break;
	}
}

static int
tul_msgout_reject(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	int phase;

	tul_set_ssig(iot, ioh, REQ | BSY | SEL, ATN);

	if ((phase = tul_wait(sc, iot, ioh, MSG_ACCEPT)) == -1)
		return (-1);

	if (phase == PHASE_MSG_OUT) {
		bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_MESSAGE_REJECT);
		return (tul_wait(sc, iot, ioh, XF_FIFO_OUT));
	}

	return (phase);
}

static int
tul_msgout_wide(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	int phase;

	sc->HCS_ActScb->SCB_Tcs->TCS_Flags |= FLAG_WIDE_DONE;

	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXTENDED);
	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_WDTR_LEN);
	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_WDTR);
	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_WDTR_BUS_16_BIT);

	phase = tul_wait(sc, iot, ioh, XF_FIFO_OUT);

	bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
	tul_set_ssig(iot, ioh, REQ | BSY | SEL | ATN, 0);

	return (phase);
}

static int
tul_msgout_sync(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	u_int16_t rateindex;
	u_int8_t sync_rate;
	int phase;

	rateindex = sc->HCS_ActScb->SCB_Tcs->TCS_Flags & FLAG_SCSI_RATE;

	sync_rate = tul_rate_tbl[rateindex];

	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXTENDED);
	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_SDTR_LEN);
	bus_space_write_1(iot, ioh, TUL_SFIFO, MSG_EXT_SDTR);
	bus_space_write_1(iot, ioh, TUL_SFIFO, sync_rate);
	bus_space_write_1(iot, ioh, TUL_SFIFO, IHA_MAX_TARGETS-1); /* REQ/ACK*/

	phase = tul_wait(sc, iot, ioh, XF_FIFO_OUT);

	bus_space_write_1(iot, ioh, TUL_SCTRL0, RSFIFO);
	tul_set_ssig(iot, ioh, REQ | BSY | SEL | ATN, 0);

	return (phase);
}

static void
tul_wdtr_done(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct tcs *pTcs = sc->HCS_ActScb->SCB_Tcs;

	pTcs->TCS_JS_Period = 0;

	if (sc->HCS_Msg[2] != 0)
		pTcs->TCS_JS_Period |= PERIOD_WIDE_SCSI;

	pTcs->TCS_SConfig0 &= ~ALTPD;
	pTcs->TCS_Flags	   &= ~FLAG_SYNC_DONE;
	pTcs->TCS_Flags	   |=  FLAG_WIDE_DONE;

	bus_space_write_1(iot, ioh, TUL_SCONFIG0, pTcs->TCS_SConfig0);
	bus_space_write_1(iot, ioh, TUL_SYNCM,	  pTcs->TCS_JS_Period);
}

static void
tul_sync_done(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	struct tcs *pTcs = sc->HCS_ActScb->SCB_Tcs;
	int i;

	if ((pTcs->TCS_Flags & FLAG_SYNC_DONE) == 0) {
		if (sc->HCS_Msg[3] != 0) {
			pTcs->TCS_JS_Period |= sc->HCS_Msg[3];

			/* pick the highest possible rate */
			for (i = 0; i < 8; i++)
				if (tul_rate_tbl[i] >= sc->HCS_Msg[2])
					break;

			pTcs->TCS_JS_Period |= (i << 4);
			pTcs->TCS_SConfig0  |= ALTPD;
		}

		pTcs->TCS_Flags |= FLAG_SYNC_DONE;

		bus_space_write_1(iot, ioh, TUL_SCONFIG0, pTcs->TCS_SConfig0);
		bus_space_write_1(iot, ioh, TUL_SYNCM,	  pTcs->TCS_JS_Period);
	}
}

void
tul_reset_chip(sc, iot, ioh)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	int i;

	/* reset tulip chip */

	bus_space_write_1(iot, ioh, TUL_SCTRL0, RSCSI);

	do
		sc->HCS_JSInt = bus_space_read_1(iot, ioh, TUL_SISTAT);
	while((sc->HCS_JSInt & SRSTD) == 0);

	tul_set_ssig(iot, ioh, 0, 0);

	/*
	 * Stall for 2 seconds, wait for target's firmware ready.
	 */
	for (i = 0; i < 2000; i++)
		DELAY (1000);

	bus_space_read_1(iot, ioh, TUL_SISTAT); /* Clear any active interrupt*/
}

static void
tul_select(sc, iot, ioh, pScb, select_type)
	struct iha_softc   *sc;
	bus_space_tag_t	    iot;
	bus_space_handle_t  ioh;
	struct iha_scsi_req_q *pScb;
	u_int8_t	select_type;
{
	switch (select_type) {
	case SEL_ATN:
		bus_space_write_1(iot, ioh, TUL_SFIFO, pScb->SCB_Ident);
		bus_space_write_multi_1(iot, ioh, TUL_SFIFO,
		    pScb->SCB_CDB, pScb->SCB_CDBLen);

		pScb->SCB_NxtStat = 2;
		break;

	case SELATNSTOP:
		pScb->SCB_NxtStat = 1;
		break;

	case SEL_ATN3:
		bus_space_write_1(iot, ioh, TUL_SFIFO, pScb->SCB_Ident);
		bus_space_write_1(iot, ioh, TUL_SFIFO, pScb->SCB_TagMsg);
		bus_space_write_1(iot, ioh, TUL_SFIFO, pScb->SCB_TagId);

		bus_space_write_multi_1(iot, ioh, TUL_SFIFO, pScb->SCB_CDB,
		    pScb->SCB_CDBLen);

		pScb->SCB_NxtStat = 2;
		break;

	default:
		printf("[debug] tul_select() - unknown select type = 0x%02x\n",
		    select_type);
		return;
	}

	tul_del_pend_scb(sc, pScb);
	pScb->SCB_Status = STATUS_SELECT;

	sc->HCS_ActScb = pScb;

	bus_space_write_1(iot, ioh, TUL_SCMD, select_type);
}

/*
 * tul_wait - wait for an interrupt to service or a SCSI bus phase change
 *            after writing the supplied command to the tulip chip. If 
 *            the command is NO_OP, skip the command writing.
 */
static int
tul_wait(sc, iot, ioh, cmd)
	struct iha_softc  *sc;
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t	   cmd;
{
	if (cmd != NO_OP)
		bus_space_write_1(iot, ioh, TUL_SCMD, cmd);

	/* 
	 * Have to do this here, in addition to in iha_isr, because
	 * interrupts might be turned off when we get here.
	 */
	do
		sc->HCS_JSStatus0 = bus_space_read_1(iot, ioh, TUL_STAT0);
	while ((sc->HCS_JSStatus0 & INTPD) == 0);

	sc->HCS_JSStatus1 = bus_space_read_1(iot, ioh, TUL_STAT1);
	sc->HCS_JSInt     = bus_space_read_1(iot, ioh, TUL_SISTAT);

	sc->HCS_Phase = sc->HCS_JSStatus0 & PH_MASK;

	if ((sc->HCS_JSInt & SRSTD) != 0) {
		/* SCSI bus reset interrupt */
		tul_reset_scsi_bus(sc);
		return (-1);
	}

	if ((sc->HCS_JSInt & RSELED) != 0)
		/* Reselection interrupt */
		return (tul_resel(sc, iot, ioh));

	if ((sc->HCS_JSInt & STIMEO) != 0) {
		/* selected/reselected timeout interrupt */
		tul_busfree(sc, iot, ioh);
		return (-1);
	}

	if ((sc->HCS_JSInt & DISCD) != 0) {
		/* BUS disconnection interrupt */
		if ((sc->HCS_Flags & FLAG_EXPECT_DONE_DISC) != 0) {
			bus_space_write_1(iot, ioh, TUL_SCTRL0,	  RSFIFO);
			bus_space_write_1(iot, ioh, TUL_SCONFIG0,
			    SCONFIG0DEFAULT);
			bus_space_write_1(iot, ioh, TUL_SCTRL1,	  EHRSL);
			tul_append_done_scb(sc, sc->HCS_ActScb, HOST_OK);
			sc->HCS_Flags &= ~FLAG_EXPECT_DONE_DISC;

		} else if ((sc->HCS_Flags & FLAG_EXPECT_DISC) != 0) {
			bus_space_write_1(iot, ioh, TUL_SCTRL0,	  RSFIFO);
			bus_space_write_1(iot, ioh, TUL_SCONFIG0,
			    SCONFIG0DEFAULT);
			bus_space_write_1(iot, ioh, TUL_SCTRL1,	  EHRSL);
			sc->HCS_ActScb = NULL;
			sc->HCS_Flags &= ~FLAG_EXPECT_DISC;

		} else
			tul_busfree(sc, iot, ioh);

		return (-1);
	}

	return (sc->HCS_Phase);
}

/*
 * tul_done_scb - We have a scb which has been processed by the
 *                adaptor, now we look to see how the operation went.
 */
static void
tul_done_scb(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	struct scsi_sense_data *s1, *s2;
	struct scsi_xfer *xs = pScb->SCB_Xs;

	if (xs != NULL) {
		xs->status = pScb->SCB_TaStat;

		switch (pScb->SCB_HaStat) {
		case HOST_OK:
			switch (pScb->SCB_TaStat) {
			case SCSI_OK:
			case SCSI_CONDITION_MET:
			case SCSI_INTERM:
			case SCSI_INTERM_COND_MET:
				xs->resid = pScb->SCB_BufLen;
				xs->error = XS_NOERROR;
				break;

			case SCSI_RSERV_CONFLICT:
			case SCSI_BUSY:
			case SCSI_QUEUE_FULL:
				xs->error = XS_BUSY;
				break;

			case SCSI_CMD_TERMINATED:
			case SCSI_ACA_ACTIVE:
			case SCSI_CHECK:
				s1 = &pScb->SCB_ScsiSenseData;
				s2 = &xs->sense;
				*s2 = *s1;

				xs->error = XS_SENSE;
				break;

			default:
				xs->error = XS_DRIVER_STUFFUP;
				break;
			}
			break;

		case HOST_SEL_TOUT:
			xs->error = XS_SELTIMEOUT;
			break;

		case HOST_SCSI_RST:
		case HOST_DEV_RST:
			xs->error = XS_RESET;
			break;

		case HOST_SPERR:
			printf("%s: SCSI Parity error detected\n",
			    sc->sc_dev.dv_xname);
			xs->error = XS_DRIVER_STUFFUP;
			break;

		case HOST_TIMED_OUT:
			xs->error = XS_TIMEOUT;
			break;

		case HOST_DO_DU:
		case HOST_BAD_PHAS:
		default:
			xs->error = XS_DRIVER_STUFFUP;
			break;
		}

		if (xs->datalen > 0) {
			bus_dmamap_sync(sc->sc_dmat, pScb->SCB_Dmamap
			    ,(xs->flags & SCSI_DATA_IN) ? 
			    BUS_DMASYNC_POSTREAD : BUS_DMASYNC_POSTWRITE);
			bus_dmamap_unload(sc->sc_dmat, pScb->SCB_Dmamap);
		}

		xs->flags |= ITSDONE;
		scsi_done(xs);
	}

	tul_append_free_scb(sc, pScb);
}

static void
tul_timeout(arg)
	void *arg;
{
	struct iha_scsi_req_q *pScb = (struct iha_scsi_req_q *)arg;
	struct scsi_xfer *xs = pScb->SCB_Xs;
	struct iha_softc *sc;

	if (xs == NULL)
		printf("[debug] tul_timeout called with xs == NULL\n");

	else {
		sc = xs->sc_link->adapter_softc;
		sc_print_addr(xs->sc_link);
		printf("SCSI OpCode 0x%02x timed out\n", xs->cmd->opcode);

		tul_abort_xs(xs->sc_link->adapter_softc, xs, HOST_TIMED_OUT);
	}
}

static void
tul_exec_scb(sc, pScb)
	struct iha_softc *sc;
	struct iha_scsi_req_q *pScb;
{
	bus_space_handle_t ioh;
	bus_space_tag_t iot;
	int s;

	s = splbio();

	if (((pScb->SCB_Flags & SCSI_RESET) != 0)
	    || (pScb->SCB_CDB[0] == REQUEST_SENSE))
		tul_push_pend_scb(sc, pScb);   /* Insert SCB at head of Pend */
	else
		tul_append_pend_scb(sc, pScb); /* Append SCB to tail of Pend */

	/*
	 * Run through tul_main() to ensure something is active, if
	 * only this new SCB.
	 */
	if (sc->HCS_Semaph != SEMAPH_IN_MAIN) {
		iot = sc->sc_iot;
		ioh = sc->sc_ioh;

		bus_space_write_1(iot, ioh, TUL_IMSK, MASK_ALL);
		sc->HCS_Semaph = SEMAPH_IN_MAIN;;

		splx(s);
		tul_main(sc, iot, ioh);
		s = splbio();

		sc->HCS_Semaph = ~SEMAPH_IN_MAIN;;
		bus_space_write_1(iot, ioh, TUL_IMSK, (MASK_ALL & ~MSCMP));
	}

	splx(s);
}


/*
 * tul_set_ssig - read the current scsi signal mask, then write a new
 *		  one which turns off/on the specified signals.
 */
static void
tul_set_ssig( iot, ioh, offsigs, onsigs)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t	   offsigs, onsigs;
{
	u_int8_t currsigs;

	currsigs = bus_space_read_1(iot, ioh, TUL_SSIGI);
	bus_space_write_1(iot, ioh, TUL_SSIGO, (currsigs & ~offsigs) | onsigs);
}

static void
tul_print_info(sc, target)
	struct iha_softc *sc;
	int target;
{
	u_int8_t period = sc->HCS_Tcs[target].TCS_JS_Period;
	u_int8_t config = sc->HCS_Tcs[target].TCS_SConfig0;
	int rate;

	printf("%s: target %d using %d bit ", sc->sc_dev.dv_xname, target,
	    (period & PERIOD_WIDE_SCSI) ? 16 : 8);

	if ((period & PERIOD_SYOFS) == 0)
		printf("async ");
	else {
		rate = (period & PERIOD_SYXPD) >> 4;
		if ((config & ALTPD) == 0)
			rate = 100 + rate * 50;
		else
			rate =	50 + rate * 25;
		rate = 1000000000 / rate;
		printf("%d.%d MHz %d REQ/ACK offset ", rate / 1000000,
		    (rate % 1000000 + 99999) / 100000, period & PERIOD_SYOFS);
	}
	
	printf("xfers\n");
}


/*
 * tul_alloc_scbs - allocate and map the SCB's for the supplied IHA_SOFTC
 */
static int
tul_alloc_scbs(sc)
	struct iha_softc *sc;
{
	bus_dma_segment_t seg;
	int error, rseg;

	/*
	 * Allocate dma-safe memory for the SCB's
	 */
	if ((error = bus_dmamem_alloc(sc->sc_dmat,
		 sizeof(struct iha_scsi_req_q)*IHA_MAX_SCB,
		 NBPG, 0, &seg, 1, &rseg, BUS_DMA_NOWAIT))
	    != 0) {
		printf("%s: unable to allocate SCBs,"
		       " error = %d\n", sc->sc_dev.dv_xname, error);
		return (error);
	}
	if ((error = bus_dmamem_map(sc->sc_dmat,
		 &seg, rseg, sizeof(struct iha_scsi_req_q)*IHA_MAX_SCB,
		 (caddr_t *)&sc->HCS_Scb, BUS_DMA_NOWAIT | BUS_DMA_COHERENT))
	    != 0) {
		printf("%s: unable to map SCBs, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}

	/*
	 * Create and load the DMA map used for the SCBs
	 */
	if ((error = bus_dmamap_create(sc->sc_dmat,
		 sizeof(struct iha_scsi_req_q)*IHA_MAX_SCB,
		 1, sizeof(struct iha_scsi_req_q)*IHA_MAX_SCB,
		 0, BUS_DMA_NOWAIT, &sc->sc_dmamap))
	    != 0) {
		printf("%s: unable to create control DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}
	if ((error = bus_dmamap_load(sc->sc_dmat, sc->sc_dmamap,
		 sc->HCS_Scb, sizeof(struct iha_scsi_req_q)*IHA_MAX_SCB,
		 NULL, BUS_DMA_NOWAIT))
	    != 0) {
		printf("%s: unable to load control DMA map, error = %d\n",
		       sc->sc_dev.dv_xname, error);
		return (error);
	}

	bzero(sc->HCS_Scb, sizeof(struct iha_scsi_req_q)*IHA_MAX_SCB);

	return (0);
}

/*
 * tul_read_eeprom - read Serial EEPROM value & set to defaults
 *		     if required. XXX - Writing does NOT work!
 */
void
tul_read_eeprom(iot, ioh)
	bus_space_tag_t iot;
	bus_space_handle_t ioh;
{
	u_int8_t gctrl;

	/*------Enable EEProm programming ---*/
	gctrl = bus_space_read_1(iot, ioh, TUL_GCTRL0) | EEPRG;
	bus_space_write_1(iot, ioh, TUL_GCTRL0, gctrl);

	/*------ Program default pattern ----*/
	if (tul_se2_rd_all(iot, ioh) == 0) {
		tul_se2_update_all(iot, ioh);
		if(tul_se2_rd_all(iot, ioh) == 0)
			panic("could not program iha Tulip EEPROM\n");
	}

	/*------ Disable EEProm programming ---*/
	gctrl = bus_space_read_1(iot, ioh, TUL_GCTRL0) & ~EEPRG;
	bus_space_write_1(iot, ioh, TUL_GCTRL0, gctrl);
}

/*
 * tul_se2_update_all - Update SCSI H/A configuration parameters from
 *			serial EEPROM Setup default pattern. Only
 *			change those values different from the values
 *			in tul_nvram.
 */
void
tul_se2_update_all(iot, ioh)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	u_int16_t *np, *np1;
	u_int32_t chksum;
	u_int8_t i;

	/* Enable erase/write state of EEPROM */
	tul_se2_instr(iot, ioh, ENABLE_ERASE);
	bus_space_write_1(iot, ioh, TUL_NVRAM, 0);
	DELAY(5);

	np  = (u_int16_t *)tul_dftNvRam;
	np1 = (u_int16_t *)&tul_nvram;

	for (i = 0, chksum = 0; i < 31; i++, np++, np1++) {
		if (*np != *np1)
			tul_se2_wr(iot, ioh, i, *np);
		chksum += *np;
	}

	chksum &= 0x0000ffff;
	tul_se2_wr(iot, ioh, 31, chksum);

	/* Disable erase/write state of EEPROM */
	tul_se2_instr(iot, ioh, 0);
	bus_space_write_1(iot, ioh, TUL_NVRAM, 0);
	DELAY(5);
}

/*
 * tul_se2_wr - write the given 16 bit value into the Serial EEPROM
 *		at the specified offset
 */
void
tul_se2_wr(iot, ioh, addr, writeWord)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t	   addr;
	u_int16_t	   writeWord;
{
	u_int8_t bit;
	int i;

	/* send 'WRITE' Instruction == address | WRITE bit */
	tul_se2_instr(iot, ioh, (addr | WRITE));

	for (i = 15; i >= 0; i--, writeWord <<= 1) {
		if (writeWord & 0x8000)
			bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS | NVRDO);
		else
			bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS);
		DELAY(5);
		bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS | NVRCK);
		DELAY(5);
	}

	bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS);
	DELAY(5);
	bus_space_write_1(iot, ioh, TUL_NVRAM, 0);
	DELAY(5);
	bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS);
	DELAY(5);

	for (;;) {
		bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS | NVRCK);
		DELAY(5);
		bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS);
		DELAY(5);
		bit = bus_space_read_1(iot, ioh, TUL_NVRAM) & NVRDI;
		DELAY(5);
		if (bit != 0)
			break; /* write complete */
	}

	bus_space_write_1(iot, ioh, TUL_NVRAM, 0);
	DELAY(5);
}

/*
 * tul_se2_rd - read & return the 16 bit value at the specified
 *		offset in the Serial E2PROM
 *
 */
u_int16_t
tul_se2_rd(iot, ioh, addr)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t	   addr;
{
	u_int16_t readWord;
	u_int8_t bit;
	int i;

	/* Send 'READ' instruction == address | READ bit */
	tul_se2_instr(iot, ioh, (addr | READ));

	readWord = 0;
	for (i = 15; i >= 0; i--) {
		bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS | NVRCK);
		DELAY(5);

		bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS);
		DELAY(5);

		/* sample data after the following edge of clock     */
		bit = bus_space_read_1(iot, ioh, TUL_NVRAM) & NVRDI;
		DELAY(5);

		readWord += bit << i;
	}

	bus_space_write_1(iot, ioh, TUL_NVRAM, 0);
	DELAY(5);

	return (readWord);
}

/*
 * tul_se2_rd_all - Read SCSI H/A config parameters from serial EEPROM
 *		    into tul_nvram static variable.
 */
int
tul_se2_rd_all(iot, ioh)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
{
	u_int16_t *np, *np1;
	u_int32_t chksum;
	u_int8_t i;

	np = (u_int16_t *)&tul_nvram;
	np1 = (u_int16_t *)&tul_dftNvRam;

	for (i = 0, chksum = 0; i < 31; i++, np++, np1++) {
		*np = tul_se2_rd(iot, ioh, i);
		chksum += *np;
	}
	*np = tul_se2_rd(iot, ioh, 31); /* just read checksum		    */

	chksum &= 0x0000ffff;		/* checksum is lower 16 bits of sum */

	return (tul_nvram.NVM_Signature == SIGNATURE)
	       &&
	       (tul_nvram.NVM_CheckSum	== chksum);
}

/*
 * tul_se2_instr - write an octet to serial E2PROM one bit at a time
 */
void
tul_se2_instr(iot, ioh, instr)
	bus_space_tag_t	   iot;
	bus_space_handle_t ioh;
	u_int8_t	   instr;
{
	u_int8_t b;
	int i;

	b = NVRCS | NVRDO; /* Write the start bit (== 1) */

	bus_space_write_1(iot, ioh, TUL_NVRAM, b);
	DELAY(5);
	bus_space_write_1(iot, ioh, TUL_NVRAM, b | NVRCK);
	DELAY(5);

	for (i = 0; i < 8; i++, instr <<= 1) {
		if (instr & 0x80)
			b = NVRCS | NVRDO; /* Write a 1 bit */
		else
			b = NVRCS;	   /* Write a 0 bit */

		bus_space_write_1(iot, ioh, TUL_NVRAM, b);
		DELAY(5);
		bus_space_write_1(iot, ioh, TUL_NVRAM, b | NVRCK);
		DELAY(5);
	}

	bus_space_write_1(iot, ioh, TUL_NVRAM, NVRCS);
	DELAY(5);

	return;
}

/*
 * tul_reset_tcs - reset the target control structure pointed
 *		   to by pTcs to default values. TCS_Flags
 *		   only has the negotiation done bits reset as
 *		   the other bits are fixed at initialization.
 */
void
tul_reset_tcs(pTcs, config0)
	struct tcs *pTcs;
	u_int8_t config0;
{
	pTcs->TCS_Flags	    &= ~(FLAG_SYNC_DONE | FLAG_WIDE_DONE);
	pTcs->TCS_JS_Period  = 0;
	pTcs->TCS_SConfig0   = config0;
	pTcs->TCS_TagCnt     = 0;
	pTcs->TCS_NonTagScb  = NULL;
}
