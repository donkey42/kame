/*	$NetBSD: conf.c,v 1.155.4.1 2002/12/12 23:48:38 he Exp $	*/

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: conf.c,v 1.155.4.1 2002/12/12 23:48:38 he Exp $");

#include "opt_compat_svr4.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <sys/vnode.h>

#include "wd.h"
bdev_decl(wd);
bdev_decl(sw);
#include "fdc.h"
bdev_decl(fd);
#include "wt.h"
bdev_decl(wt);
#include "sd.h"
bdev_decl(sd);
#include "st.h"
bdev_decl(st);
#include "cd.h"
bdev_decl(cd);
#include "mcd.h"
bdev_decl(mcd);
#include "vnd.h"
bdev_decl(vnd);
#include "scd.h"
bdev_decl(scd);
#include "ccd.h"
bdev_decl(ccd);
#include "raid.h"
bdev_decl(raid);
#include "md.h"
bdev_decl(md);
#include "ld.h"
bdev_decl(ld);
#include "ed_mca.h"
bdev_decl(edmca);

struct bdevsw	bdevsw[] =
{
	bdev_disk_init(NWD,wd),	/* 0: ST506/ESDI/IDE disk */
	bdev_swap_init(1,sw),		/* 1: swap pseudo-device */
	bdev_disk_init(NFDC,fd),	/* 2: floppy diskette */
	bdev_tape_init(NWT,wt),		/* 3: QIC-02/QIC-36 tape */
	bdev_disk_init(NSD,sd),		/* 4: SCSI disk */
	bdev_tape_init(NST,st),		/* 5: SCSI tape */
	bdev_disk_init(NCD,cd),		/* 6: SCSI CD-ROM */
	bdev_disk_init(NMCD,mcd),	/* 7: Mitsumi CD-ROM */
	bdev_lkm_dummy(),		/* 8 */
	bdev_lkm_dummy(),		/* 9 */
	bdev_lkm_dummy(),		/* 10 */
	bdev_lkm_dummy(),		/* 11 */
	bdev_lkm_dummy(),		/* 12 */
	bdev_lkm_dummy(),		/* 13 */
	bdev_disk_init(NVND,vnd),	/* 14: vnode disk driver */
	bdev_disk_init(NSCD,scd),	/* 15: Sony CD-ROM */
	bdev_disk_init(NCCD,ccd),	/* 16: concatenated disk driver */
	bdev_disk_init(NMD,md),		/* 17: memory disk driver */
	bdev_disk_init(NRAID,raid),	/* 18: RAIDframe disk driver */
	bdev_disk_init(NLD,ld),		/* 19: logical disk */
	bdev_disk_init(NED_MCA,edmca),	/* 20: PS/2 ESDI disk */
};
int	nblkdev = sizeof(bdevsw) / sizeof(bdevsw[0]);

#include <dev/sysmon/sysmonconf.h>
cdev_decl(sysmon);

cdev_decl(cn);
cdev_decl(ctty);
#define	mmread	mmrw
#define	mmwrite	mmrw
cdev_decl(mm);
cdev_decl(wd);
cdev_decl(sw);
#include "pty.h"
#define	ptstty		ptytty
#define	ptsioctl	ptyioctl
cdev_decl(pts);
#define	ptctty		ptytty
#define	ptcioctl	ptyioctl
cdev_decl(ptc);
cdev_decl(log);
#include "com.h"
cdev_decl(com);
cdev_decl(fd);
cdev_decl(wt);
cdev_decl(scd);
#include "pc.h"
cdev_decl(pc);
cdev_decl(sd);
cdev_decl(st);
#include "ses.h"
cdev_decl(ses);
#include "ss.h"
cdev_decl(ss);
#include "uk.h"
cdev_decl(uk);
cdev_decl(cd);
#include "lpt.h"
cdev_decl(lpt);
#include "ch.h"
cdev_decl(ch);
dev_decl(filedesc,open);
#include "bpfilter.h"
cdev_decl(bpf);
cdev_decl(md);
#include "spkr.h"
cdev_decl(spkr);
#include "omms.h"
cdev_decl(mms);
#include "olms.h"
cdev_decl(lms);
#include "cy.h"
cdev_decl(cy);
#include "cz.h"
cdev_decl(cztty);
cdev_decl(mcd);
#include "tun.h"
cdev_decl(tun);
cdev_decl(vnd);
#include "audio.h"
cdev_decl(audio);
#include "midi.h"
cdev_decl(midi);
#include "sequencer.h"
cdev_decl(music);
#include "radio.h"
cdev_decl(radio);
cdev_decl(svr4_net);
cdev_decl(ccd);
cdev_decl(raid);
cdev_decl(ld);
#include "joy.h"
cdev_decl(joy);
#include "apm.h"
cdev_decl(apm);
#include "usb.h"
cdev_decl(usb);
#include "uhid.h"
cdev_decl(uhid);
#include "ugen.h"
cdev_decl(ugen);
#include "ulpt.h"
cdev_decl(ulpt);
#include "ucom.h"
cdev_decl(ucom);
#include "urio.h"
cdev_decl(urio);
#include "uscanner.h"
cdev_decl(uscanner);
#include "vcoda.h"
cdev_decl(vc_nb_);
#include "netsmb.h"
cdev_decl(nsmb_dev_);

#include "ipfilter.h"
#include "satlink.h"
cdev_decl(satlink);

#include "rnd.h"

#include "wsdisplay.h"
cdev_decl(wsdisplay);
#include "wskbd.h"
cdev_decl(wskbd);
#include "wsmouse.h"
cdev_decl(wsmouse);
#include "wsmux.h"
cdev_decl(wsmux);
#include "esh.h"
cdev_decl(esh_fp);
#include "scsibus.h"
cdev_decl(scsibus);
#include "bktr.h"
#include "irframe.h"
cdev_decl(irframe);
#include "cir.h"
cdev_decl(cir);

#include "isdn.h"
#include "isdnctl.h"
#include "isdntrc.h"
#include "isdnbchan.h"
#include "isdntel.h"
cdev_decl(isdn);
cdev_decl(isdnctl);
cdev_decl(isdntrc);
cdev_decl(isdnbchan);
cdev_decl(isdntel);

/* open, close, read, write, ioctl, mmap */
#define cdev_vmegen_init(c,n)	cdev__ocrwim_init(c,n)

#include "vmegeneric.h"
cdev_decl(vmegeneric);
#include "iop.h"
cdev_decl(iop);
#include "mlx.h"
cdev_decl(mlx);
#include "mly.h"
cdev_decl(mly);
#include "dpti.h"
cdev_decl(dpti);
cdev_decl(edmca);
#include "agp.h"
cdev_decl(agp);
#include "dpt.h"
cdev_decl(dpt);

#include <altq/altqconf.h>

#include "wsfont.h"
cdev_decl(wsfont);

#include "pci.h"
cdev_decl(pci);

#include "clockctl.h"
cdev_decl(clockctl);

#include "pf.h"
cdev_decl(pf);

struct cdevsw	cdevsw[] =
{
	cdev_cn_init(1,cn),		/* 0: virtual console */
	cdev_ctty_init(1,ctty),		/* 1: controlling terminal */
	cdev_mm_init(1,mm),		/* 2: /dev/{null,mem,kmem,...} */
	cdev_disk_init(NWD,wd),		/* 3: ST506/ESDI/IDE disk */
	cdev_swap_init(1,sw),		/* 4: /dev/drum (swap pseudo-device) */
	cdev_tty_init(NPTY,pts),	/* 5: pseudo-tty slave */
	cdev_ptc_init(NPTY,ptc),	/* 6: pseudo-tty master */
	cdev_log_init(1,log),		/* 7: /dev/klog */
	cdev_tty_init(NCOM,com),	/* 8: serial port */
	cdev_disk_init(NFDC,fd),	/* 9: floppy disk */
	cdev_tape_init(NWT,wt),		/* 10: QIC-02/QIC-36 tape */
	cdev_disk_init(NSCD,scd),	/* 11: Sony CD-ROM */
	cdev_pc_init(NPC,pc),		/* 12: PC console */
	cdev_disk_init(NSD,sd),		/* 13: SCSI disk */
	cdev_tape_init(NST,st),		/* 14: SCSI tape */
	cdev_disk_init(NCD,cd),		/* 15: SCSI CD-ROM */
	cdev_lpt_init(NLPT,lpt),	/* 16: parallel printer */
	cdev_ch_init(NCH,ch),		/* 17: SCSI autochanger */
	cdev_disk_init(NCCD,ccd),	/* 18: concatenated disk driver */
	cdev_scanner_init(NSS,ss),	/* 19: SCSI scanner */
	cdev_uk_init(NUK,uk),		/* 20: SCSI unknown */
	cdev_apm_init(NAPM,apm),	/* 21: Advancded Power Management */
	cdev_fd_init(1,filedesc),	/* 22: file descriptor pseudo-device */
	cdev_bpftun_init(NBPFILTER,bpf),/* 23: Berkeley packet filter */
	cdev_disk_init(NMD,md),		/* 24: memory disk driver */
	cdev_notdef(),			/* 25 */
	cdev_joy_init(NJOY,joy),        /* 26: joystick */
	cdev_spkr_init(NSPKR,spkr),	/* 27: PC speaker */
	cdev_lkm_init(NLKM,lkm),	/* 28: loadable module driver */
	cdev_lkm_dummy(),		/* 29 */
	cdev_lkm_dummy(),		/* 30 */
	cdev_lkm_dummy(),		/* 31 */
	cdev_lkm_dummy(),		/* 32 */
	cdev_lkm_dummy(),		/* 33 */
	cdev_lkm_dummy(),		/* 34 */
	cdev_mouse_init(NOMMS,mms),	/* 35: Microsoft mouse */
	cdev_mouse_init(NOLMS,lms),	/* 36: Logitech mouse */
	cdev_notdef(),			/* 37: was: opms (PS/2 mouse) */
	cdev_tty_init(NCY,cy),		/* 38: Cyclom serial port */
	cdev_disk_init(NMCD,mcd),	/* 39: Mitsumi CD-ROM */
	cdev_bpftun_init(NTUN,tun),	/* 40: network tunnel */
	cdev_disk_init(NVND,vnd),	/* 41: vnode disk driver */
	cdev_audio_init(NAUDIO,audio),	/* 42: generic audio I/O */
#ifdef COMPAT_SVR4
	cdev_svr4_net_init(1,svr4_net),	/* 43: svr4 net pseudo-device */
#else
	cdev_notdef(),			/* 43 */
#endif
	cdev_ipf_init(NIPFILTER,ipl),	/* 44: ip-filter device */
	cdev_satlink_init(NSATLINK,satlink), /* 45: planetconnect satlink */
	cdev_rnd_init(NRND,rnd),	/* 46: random source pseudo-device */
	cdev_wsdisplay_init(NWSDISPLAY,
			    wsdisplay), /* 47: frame buffers, etc. */

	cdev_mouse_init(NWSKBD, wskbd), /* 48: keyboards */
	cdev_mouse_init(NWSMOUSE,
			wsmouse),       /* 49: mice */
	cdev_isdn_init(NISDN, isdn),		/* 50: isdn main device */
	cdev_isdnctl_init(NISDNCTL, isdnctl),	/* 51: isdn control device */
	cdev_isdnbchan_init(NISDNBCHAN, isdnbchan), /* 52: isdn raw b-channel access */
	cdev_isdntrc_init(NISDNTRC, isdntrc),	/* 53: isdn trace device */
	cdev_isdntel_init(NISDNTEL, isdntel),	/* 54: isdn phone device */
	cdev_usb_init(NUSB,usb),	/* 55: USB controller */
	cdev_usbdev_init(NUHID,uhid),	/* 56: USB generic HID */
	cdev_lpt_init(NULPT,ulpt),	/* 57: USB printer */
	cdev_midi_init(NMIDI,midi),	/* 58: MIDI I/O */
	cdev_midi_init(NSEQUENCER,sequencer),	/* 59: sequencer I/O */
	cdev_vc_nb_init(NVCODA,vc_nb_), /* 60: coda file system psdev */
	cdev_scsibus_init(NSCSIBUS,scsibus), /* 61: SCSI bus */
	cdev_disk_init(NRAID,raid),	/* 62: RAIDframe disk driver */
	cdev_esh_init(NESH, esh_fp),	/* 63: HIPPI (esh) raw device */
	cdev_ugen_init(NUGEN,ugen),	/* 64: USB generic driver */
	cdev_mouse_init(NWSMUX,	wsmux), /* 65: ws multiplexor */
	cdev_tty_init(NUCOM, ucom),	/* 66: USB tty */
	cdev_sysmon_init(NSYSMON, sysmon),/* 67: System Monitor */
	cdev_vmegen_init(NVMEGENERIC, vmegeneric), /* 68: generic VME access */
	cdev_disk_init(NLD, ld),	/* 69: logical disk */
	cdev_usbdev_init(NURIO,urio),	/* 70: Diamond Rio 500 */
	cdev_bktr_init(NBKTR, bktr),    /* 71: Bt848 video capture device */
	cdev_notdef(),			/* 72 */
	cdev_tty_init(NCZ,cztty),	/* 73: Cyclades-Z serial port */
	cdev_ses_init(NSES,ses),	/* 74: SCSI SES/SAF-TE */
	cdev_ugen_init(NUSCANNER,uscanner),/* 75: USB scanner */
	cdev__oci_init(NIOP,iop),	/* 76: I2O IOP control interface */
	cdev_altq_init(NALTQ,altq),	/* 77: ALTQ control interface */
	cdev__oci_init(NMLX,mlx),	/* 78: Mylex DAC960 control interface */
	cdev_disk_init(NED_MCA,edmca),	/* 79: PS/2 ESDI disk */
	cdev__oci_init(NMLY,mly),	/* 80: Newer Mylex control interface */
	cdev__oci_init(NWSFONT,wsfont),	/* 81: wsfont pseudo-device */
	cdev__ocim_init(NAGP,agp),	/* 82: AGP graphics aperture device */
	cdev_pci_init(NPCI,pci),	/* 83: PCI bus access device */
	cdev__oci_init(NDPTI,dpti),	/* 84: DPT/Adaptec RAID management */
	cdev_ir_init(NIRFRAMEDRV,irframe),/* 85: IrDA frame driver */
	cdev_ir_init(NCIR,cir),		/* 86: Consumer Ir */
	cdev_radio_init(NRADIO,radio),	/* 87: generic radio I/O */
	cdev_netsmb_init(NNETSMB,nsmb_dev_),/* 88: SMB */
	cdev_clockctl_init(NCLOCKCTL, clockctl),/* 89: clockctl pseudo device */
	cdev_notdef(),			/* 90 */
	cdev_notdef(),			/* 91 */
	cdev_notdef(),			/* 92 */
	cdev_notdef(),			/* 93 */
	cdev_notdef(),			/* 94 */
	cdev_notdef(),			/* 95 */
	cdev__oci_init(NDPT,dpt),	/* 96: DPT/Adaptec RAID management */
	cdev_notdef(),			/* 97 */
	cdev_notdef(),			/* 98 */
	cdev_notdef(),			/* 99 */
	cdev_notdef(),			/* 100 */
	cdev_notdef(),			/* 101 */
	cdev_notdef(),			/* 102 */
	cdev_notdef(),			/* 103 */
	cdev_notdef(),			/* 104 */
	cdev_notdef(),			/* 105 */
	cdev_notdef(),			/* 106 */
	cdev_notdef(),			/* 107 */
	cdev__oci_init(NPF,pf),		/* 108: PF packet filter */
};
int	nchrdev = sizeof(cdevsw) / sizeof(cdevsw[0]);

int	mem_no = 2; 	/* major device number of memory special file */

/*
 * Swapdev is a fake device implemented
 * in sw.c used only internally to get to swstrategy.
 * It cannot be provided to the users, because the
 * swstrategy routine munches the b_dev and b_blkno entries
 * before calling the appropriate driver.  This would horribly
 * confuse, e.g. the hashing routines. Instead, /dev/drum is
 * provided as a character (raw) device.
 */
dev_t	swapdev = makedev(1, 0);

/*
 * Returns true if dev is /dev/mem or /dev/kmem.
 */
int
iskmemdev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && (minor(dev) < 2 || minor(dev) == 14));
}

/*
 * Returns true if dev is /dev/zero.
 */
int
iszerodev(dev)
	dev_t dev;
{

	return (major(dev) == mem_no && minor(dev) == 12);
}

static int chrtoblktbl[] = {
	/* XXXX This needs to be dynamic for LKMs. */
	/*VCHR*/	/*VBLK*/
	/*  0 */	NODEV,
	/*  1 */	NODEV,
	/*  2 */	NODEV,
	/*  3 */	0,
	/*  4 */	NODEV,
	/*  5 */	NODEV,
	/*  6 */	NODEV,
	/*  7 */	NODEV,
	/*  8 */	NODEV,
	/*  9 */	2,
	/* 10 */	3,
	/* 11 */	15,
	/* 12 */	NODEV,
	/* 13 */	4,
	/* 14 */	5,
	/* 15 */	6,
	/* 16 */	NODEV,
	/* 17 */	NODEV,
	/* 18 */	16,
	/* 19 */	NODEV,
	/* 20 */	NODEV,
	/* 21 */	NODEV,
	/* 22 */	NODEV,
	/* 23 */	NODEV,
	/* 24 */	17,
	/* 25 */	NODEV,
	/* 26 */	NODEV,
	/* 27 */	NODEV,
	/* 28 */	NODEV,
	/* 29 */	NODEV,
	/* 30 */	NODEV,
	/* 31 */	NODEV,
	/* 32 */	NODEV,
	/* 33 */	NODEV,
	/* 34 */	NODEV,
	/* 35 */	NODEV,
	/* 36 */	NODEV,
	/* 37 */	NODEV,
	/* 38 */	NODEV,
	/* 39 */	7,
	/* 40 */	NODEV,
	/* 41 */	14,
	/* 42 */	NODEV,
	/* 43 */	NODEV,
	/* 44 */	NODEV,
	/* 45 */	NODEV,
	/* 46 */	NODEV,
	/* 47 */	NODEV,
	/* 48 */	NODEV,
	/* 49 */	NODEV,
	/* 50 */	NODEV,
	/* 51 */	NODEV,
	/* 52 */	NODEV,
	/* 53 */	NODEV,
	/* 54 */	NODEV,
	/* 55 */	NODEV,
	/* 56 */	NODEV,
	/* 57 */	NODEV,
	/* 58 */	NODEV,
	/* 59 */	NODEV,
	/* 60 */	NODEV,
	/* 61 */	NODEV,
	/* 62 */	18,
	/* 63 */	NODEV,
	/* 64 */	NODEV,
	/* 65 */	NODEV,
	/* 66 */	NODEV,
	/* 67 */	NODEV,
	/* 68 */	NODEV,
	/* 69 */	19,
	/* 70 */	NODEV,
	/* 71 */	NODEV,
	/* 72 */	NODEV,
	/* 73 */	NODEV,
	/* 74 */	NODEV,
	/* 75 */	NODEV,
	/* 76 */	NODEV,
	/* 77 */	NODEV,
	/* 78 */	NODEV,
	/* 79 */	20,
	/* 80 */	NODEV,
	/* 81 */	NODEV,
	/* 82 */	NODEV,
	/* 83 */	NODEV,
	/* 84 */	NODEV,
	/* 85 */	NODEV,
	/* 86 */	NODEV,
	/* 87 */	NODEV,
	/* 88 */	NODEV,
	/* 89 */	NODEV,
	/* 90 */	NODEV,
	/* 91 */	NODEV,
	/* 92 */	NODEV,
	/* 93 */	NODEV,
	/* 94 */	NODEV,
	/* 95 */	NODEV,
	/* 96 */	NODEV,
};

/*
 * Convert a character device number to a block device number.
 */
dev_t
chrtoblk(dev)
	dev_t dev;
{
	int blkmaj;

	if (major(dev) >= nchrdev)
		return (NODEV);
	blkmaj = chrtoblktbl[major(dev)];
	if (blkmaj == NODEV)
		return (NODEV);
	return (makedev(blkmaj, minor(dev)));
}
