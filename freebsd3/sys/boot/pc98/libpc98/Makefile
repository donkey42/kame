#	$Id: Makefile,v 1.1.2.1 1999/02/06 07:37:14 kato Exp $
#
LIB=			pc98
NOPIC=
NOPROFILE=
INTERNALLIB=		true
INTERNALSTATICLIB=	true

SRCS=	aout_freebsd.c biosdisk.c biosmem.c biospnp.c biospci.c \
	bootinfo.c comconsole.c devicename.c elf_freebsd.c gatea20.c \
	i386_copy.c i386_module.c time.c vidconsole.c
.PATH:	${.CURDIR}/../../i386/libi386

CFLAGS+=	-DPC98

CFLAGS+=	-I${.CURDIR}/../../common -I${.CURDIR}/../btx/lib \
		-I${.CURDIR}/../../.. -I. -I${.CURDIR}/../../i386/libi386

BOOT_COMCONSOLE_PORT?= 0x238
CFLAGS+=	-DCOMPORT=${BOOT_COMCONSOLE_PORT}

BOOT_COMCONSOLE_SPEED?= 9600
CFLAGS+=	-DCOMSPEED=${BOOT_COMCONSOLE_SPEED}

# Make the disk code more talkative
#CFLAGS+= -DDISK_DEBUG

# Include simple terminal emulation (cons25-compatible)
CFLAGS+= -DTERM_EMU

# If it's not there, don't consider it a target
.if exists(${.CURDIR}/../../../i386/include)
beforedepend ${OBJS}: machine

machine:
	ln -sf ${.CURDIR}/../../../i386/include machine

.endif

CLEANFILES+=	machine

.include <bsd.lib.mk>
