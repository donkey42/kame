#	$Id: Makefile,v 1.24.2.1 1999/01/27 20:56:24 julian Exp $

.PATH:	${.CURDIR}/../../i386/linux
KMOD=	linux
SRCS=	linux_file.c linux_ioctl.c linux_misc.c linux_signal.c \
	linux_ipc.c linux_socket.c linux_stats.c \
	linux_dummy.c linux_sysent.c linux_sysvec.c linux_util.c \
	imgact_linux.c opt_compat.h opt_linux.h opt_vmpage.h vnode_if.h
OBJS=	linux_locore.o
MAN8=	linux.8

CFLAGS+= -DCOMPAT_LINUX -DVM_STACK #-DDEBUG
EXPORT_SYMS=_linux_mod
CLEANFILES+= vnode_if.h vnode_if.c linux_genassym.o linux_genassym \
	linux_assym.h opt_compat.h opt_linux.h opt_vmpage.h

build-tools: linux_genassym

linux_assym.h:	linux_genassym
	./linux_genassym > linux_assym.h

linux_locore.o:	linux_locore.s linux_assym.h
	${CC} -c -x assembler-with-cpp -DLOCORE -DKERNEL ${CFLAGS} \
		${.IMPSRC} -o ${.TARGET}

linux_genassym.o:	linux_genassym.c linux.h @ machine
	${CC} -c ${CFLAGS} -UKERNEL ${.IMPSRC}

linux_genassym:	linux_genassym.o
	${CC} -static ${CFLAGS} ${LDFLAGS} -o ${.TARGET} ${.ALLSRC}

opt_compat.h:
	echo "#define COMPAT_43 1" > opt_compat.h

opt_linux.h opt_vmpage.h:
	touch ${.TARGET}

afterinstall:
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${.CURDIR}/linux.sh ${DESTDIR}/usr/bin/linux

.include <bsd.kmod.mk>
