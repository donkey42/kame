SRCDIR=	${.CURDIR}/../../kame/mchat

PROG=	mchat
SRCS=	mchat.c screen.c history.c

CFLAGS+=-DINET6 -DIPSEC -I${SRCDIR}
LDADD+=	-L${.CURDIR}/../../lib/libipsec -L${.CURDIR}/../../lib/libipsec/obj \
	-L${.CURDIR}/../../lib/libinet6 -L${.CURDIR}/../../lib/libinet6/obj \
	-L/usr/local/v6/lib -lipsec -linet6
DPADD+= ${.CURDIR}/../../lib/libipsec/libipsec.a \
	${.CURDIR}/../../lib/libipsec/obj/libipsec.a \
	/usr/local/v6/lib/libipsec.a
DPADD+= ${.CURDIR}/../lib/libinet6/libinet6.a \
	${.CURDIR}/../lib/libinet6/obj/libinet6.a \
	/usr/local/v6/lib/libinet6.a
LDADD+=-lcurses -ltermcap
DPADD+=${LIBCURSES} ${LIBTERMCAP}

MAN1=	mchat.1

.PATH:	${SRCDIR}

.include <bsd.prog.mk>
