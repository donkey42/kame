# $FreeBSD: src/sys/modules/oldcard/Makefile,v 1.10 2002/11/06 13:46:59 nyan Exp $

.PATH: ${.CURDIR}/../../pccard ${.CURDIR}/../../pci

KMOD=	oldcard
SRCS=	pccard.c pcic.c pccard_beep.c pccard_nbk.c pcic_pci.c pcic_isa.c \
	opt_pcic.h device_if.h bus_if.h isa_if.h pci_if.h \
	power_if.h card_if.h 

EXPORT_SYMS=	YES	# XXX evaluate

.if ${MACHINE} == "pc98"
SRCS += mecia.c
.endif

.include <bsd.kmod.mk>
