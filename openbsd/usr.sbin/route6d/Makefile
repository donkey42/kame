PROG=	route6d
MAN=	route6d.8

CPPFLAGS+=-DINET6 -DHAVE_GETIFADDRS -DHAVE_POLL_H
LDADD+=	-lutil
DPADD+=	${LIBUTIL}
LDADD+=	-L${.CURDIR}/../../lib/libinet6 -L${.CURDIR}/../../lib/libinet6/obj \
	-L/usr/local/v6/lib -linet6
DPADD+= ${.CURDIR}/../../lib/libinet6/libinet6.a \
	${.CURDIR}/../../lib/libinet6/obj/libinet6.a \
	/usr/local/v6/lib/libinet6.a

.PATH:	${.CURDIR}/../../kame/route6d

.include <bsd.prog.mk>
