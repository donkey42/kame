#	$NetBSD: Makefile,v 1.7 1998/06/12 23:22:47 cgd Exp $

# Makefile for x68k tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TX68K=	../x68k/tags
SX68K=	../x68k/x68k/*.[ch] ../x68k/include/*.h \
	../x68k/dev/*.[ch]
AX68K=	../x68k/x68k/*.s

# Directories in which to place tags links
DX68K=	x68k dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TX68K} ${SX68K} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AX68K} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TX68K}
	sort -o ${TX68K} ${TX68K}

links:
	-for i in ${DX68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include usr.bin usr.sbin stand

.include <bsd.subdir.mk>
