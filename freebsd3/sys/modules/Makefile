# $FreeBSD: src/sys/modules/Makefile,v 1.47.2.4 1999/11/22 07:08:09 julian Exp $

# XXX present but broken: ip_mroute_mod
# XXX not yet completed porting: acd
# XXX builds but not updated: atapi

SUBDIR=	ccd cd9660 coda fdesc if_disc if_ppp if_sl if_tun ipfw kernfs \
	mfs msdos netgraph nfs ntfs nullfs portal procfs umapfs union vinum 

# XXX some of these can move to the general case when de-i386'ed
.if ${MACHINE_ARCH} == "i386"
SUBDIR+=atapi coff fpu gnufpu ibcs2 joy linux lkm splash syscons vesa # pcic
.endif

SUBDIR+=altq

.include <bsd.subdir.mk>
