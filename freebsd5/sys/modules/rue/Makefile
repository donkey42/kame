# $FreeBSD: src/sys/modules/rue/Makefile,v 1.1 2003/05/03 10:16:56 akiyama Exp $

S=	${.CURDIR}/../..
.PATH: $S/dev/usb

KMOD=	if_rue
SRCS=	if_rue.c opt_bdg.h opt_usb.h device_if.h bus_if.h
SRCS+=	miibus_if.h

.include <bsd.kmod.mk>
