/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/* YIPS @(#)$Id: sockmisc.h,v 1.3 2000/01/11 01:02:19 itojun Exp $ */

extern const int niflags;

extern int cmpsaddrwop __P((struct sockaddr *addr1, struct sockaddr *addr2));
extern int cmpsaddr __P((struct sockaddr *addr1, struct sockaddr *addr2));

extern struct sockaddr *getlocaladdr __P((struct sockaddr *remote));

extern int recvfromto __P((int s, void *buf, size_t buflen, int flags,
	struct sockaddr *from, int *fromlen, struct sockaddr *to , int *tolen));
extern int sendfromto __P((int s, const void *buf, size_t buflen,
	struct sockaddr *src, struct sockaddr *dst));

extern int setsockopt_bypass __P((int so, int family));

extern struct sockaddr *newsaddr __P((int len));
extern struct sockaddr *dupsaddr __P((struct sockaddr *src));
extern char *saddr2str __P((struct sockaddr *saddr));
extern char *saddrwop2str __P((struct sockaddr *saddr));
extern void mask_sockaddr __P((struct sockaddr *a, struct sockaddr *b,
	size_t l));
