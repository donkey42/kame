/*	$KAME: sctputil.h,v 1.7 2002/10/09 18:01:22 itojun Exp $	*/
/*	Header: /home/sctpBsd/netinet/sctputil.h,v 1.36 2002/04/01 21:59:20 randall Exp	*/

#ifndef __sctputil_h__
#define __sctputil_h__

/*
 * Copyright (C) 2002 Cisco Systems Inc,
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

#ifdef _KERNEL
u_int32_t sctp_select_initial_TSN(struct sctp_pcb *);

u_int32_t sctp_select_a_tag(struct sctp_inpcb *);

void sctp_init_asoc(struct sctp_inpcb *, struct sctp_association *, int);

void sctp_fill_random_store(struct sctp_pcb *);

int sctp_timer_start(int, struct sctp_inpcb *, struct sctp_tcb *,
	struct sctp_nets *);

int sctp_timer_stop(int, struct sctp_inpcb *, struct sctp_tcb *,
	struct sctp_nets *);

u_int32_t sctp_calculate_sum(struct mbuf *, int32_t *, u_int32_t);

void sctp_mtu_size_reset(struct sctp_inpcb *, struct sctp_association *,
	u_long);

int find_next_best_mtu(int);

u_int32_t sctp_calculate_rto(struct sctp_tcb *, struct sctp_association *,
	struct sctp_nets *, struct timeval *);

caddr_t sctp_m_getptr(struct mbuf *, int, int, u_int8_t *);

struct sctp_paramhdr *sctp_get_next_param(struct mbuf *, int,
	struct sctp_paramhdr *, int);

int sctp_add_pad_tombuf(struct mbuf *, int);

int sctp_pad_lastmbuf(struct mbuf *, int);

void sctp_ulp_notify(u_int32_t, struct sctp_tcb *, u_int32_t, void *);

void sctp_report_all_outbound(struct sctp_tcb *);

void sctp_abort_notification(struct sctp_tcb *, int);

/* We abort responding to an IP packet for some reason */
void sctp_abort_association(struct sctp_inpcb *, struct sctp_tcb *,
	struct mbuf *, int, struct mbuf *);

/* We choose to abort via user input */
void sctp_abort_an_association(struct sctp_inpcb *, struct sctp_tcb *, int,
	struct mbuf *);

void sctp_handle_ootb(struct sctp_inpcb *, struct mbuf *, int, int, int,
	struct mbuf *);

int sctp_is_there_an_abort_here(struct mbuf *, int);
uint32_t sctp_is_same_scope(struct sockaddr_in6 *, struct sockaddr_in6 *);
struct sockaddr_in6 *sctp_recover_scope(struct sockaddr_in6 *,
	struct sockaddr_in6 *);

int sctp_cmpaddr(struct sockaddr *, struct sockaddr *);

const char *sctp_ntop4(const u_char *, char *, size_t);
const char *sctp_ntop6(const u_char *, char *, size_t);
void sctp_print_address(struct sockaddr *);

int sbappendaddr_nocheck __P((struct sockbuf *, struct sockaddr *,
	struct mbuf *, struct mbuf *, u_int32_t));

struct rtentry *rtalloc_alternate __P((struct sockaddr *, struct rtentry *,
	int));


struct mbuf *sctp_generate_invmanparam(int);

/* this is an evil layer violation that I think
 * is a hack.. but I stand alone on the tsvwg
 * in this thought... everyone else considers
 * it part of the sockets layer (along with
 * all of the peeloff code :<
 */
void sctp_grub_through_socket_buffer(struct sctp_inpcb *,
	struct socket *, struct socket *, struct sctp_tcb *);


void sctp_free_bufspace(struct sctp_tcb *, struct sctp_association *,
	struct sctp_tmit_chunk *);

#endif /* KERNEL */
#endif