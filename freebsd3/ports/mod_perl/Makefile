# New ports collection makefile for:	mod_perl
# Date created:				November 5th 1999
# Whom:	      				Sheldon Hearn <sheldonh@FreeBSD.org>
#
# $FreeBSD: ports/www/mod_perl/Makefile,v 1.8 2000/04/16 11:30:44 paul Exp $
#

PORTNAME=	mod_perl
PORTVERSION= 	1.24
CATEGORIES=	www perl5
MASTER_SITES=	${MASTER_SITE_PERL_CPAN}
MASTER_SITE_SUBDIR=	Apache

MAINTAINER=	sumikawa@kame.net

BUILD_DEPENDS=	${LOCALBASE}/sbin/apxs:../apache13
RUN_DEPENDS=	${LOCALBASE}/sbin/apxs:../apache13

USE_PERL5=	YES

MAN3=		Apache.3 Apache::Constants.3  Apache::ExtUtils.3 \
		Apache::FakeRequest.3 Apache::File.3 Apache::Leak.3 \
		Apache::Log.3 Apache::Options.3 \
		Apache::PerlRun.3 Apache::PerlRunXS.3 \
		Apache::RedirectLogFix.3 \
		Apache::Registry.3 Apache::SizeLimit.3 \
		Apache::Status.3 Apache::Symbol.3 Apache::Symdump.3 \
		Apache::Table.3 Apache::URI.3 Apache::Util.3 \
		Apache::httpd_conf.3 Apache::src.3 Bundle::Apache.3 \
		Apache::Debug.3 mod_perl.3 Apache::SIG.3 Apache::Resource.3 \
		Apache::RegistryLoader.3 Apache::StatINC.3 Apache::Include.3 \
		Apache::PerlSections.3 cgi_to_mod_perl.3 \
		mod_perl_cvs.3 mod_perl_method_handlers.3 mod_perl_traps.3 \
		mod_perl_tuning.3
MANPREFIX=	${PREFIX}/lib/perl5/${PERL_VERSION}

.include <bsd.port.pre.mk>

.if ${OSVERSION} < 400000
BROKEN=		"see PR 14924; use p5-Apache instead"
.endif

do-configure:
	@ cd ${WRKSRC}; \
	${PERL5} Makefile.PL USE_APXS=1 EVERYTHING=1 \
	    WITH_APXS=${LOCALBASE}/sbin/apxs \
	    PERL_EXTRA_CFLAGS='-DDEFAULT_PATH=\"/bin:/usr/bin:/usr/local/bin\"'

post-install:
	${MKDIR} ${PREFIX}/include/apache/modules/perl
	${INSTALL_DATA} ${WRKSRC}/src/modules/perl/*.h \
	    ${PREFIX}/include/apache/modules/perl
	${SH} ${PKGINSTALL}

.include <bsd.port.post.mk>
