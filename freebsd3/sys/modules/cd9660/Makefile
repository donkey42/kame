#	$Id: Makefile,v 1.6 1998/10/16 04:30:42 peter Exp $

.PATH:	${.CURDIR}/../../isofs/cd9660
KMOD=	cd9660
SRCS=	cd9660_bmap.c cd9660_lookup.c cd9660_node.c cd9660_rrip.c \
	cd9660_util.c cd9660_vfsops.c cd9660_vnops.c
NOMAN=
VFS_KLD=
CFLAGS+= -DCD9660

.include <bsd.kmod.mk>
